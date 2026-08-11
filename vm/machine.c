/*
* Copyright 2026 Rochus Keller <mailto:me@rochus-keller.ch>
*
* This file is part of the Project Oberon System project.
*
* The following is the license that applies to this copy of the
* file. For a license to use the file under conditions
* other than those described here, please email to me@rochus-keller.ch.
*
* GNU General Public License Usage
* This file may be used under the terms of the GNU General Public
* License (GPL) versions 2.0 or 3.0 as published by the Free Software
* Foundation and appearing in the file LICENSE.GPL included in
* the packaging of this file. Please review the following information
* to ensure GNU General Public Licensing requirements will be met:
* http://www.fsf.org/licensing/licenses/info/GPLv2.html and
* http://www.gnu.org/copyleft/gpl.html.
*/

/* Oberon RISC machine on top of the rv32emu interpreter: memory map,
  devices and run loop. The devices are the ones of Wirth's RISC-5 computer
  (see PO.Computer), so that Kernel.Mod, Display.Mod and Input.Mod of
   Project Oberon can be used unchanged */

#include "machine.h"
#include "Screen.h"
#include "io.h"
#include "riscv.h"
#include "riscv_private.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

/* event types and mouse buttons of the screen adapter, must match Screen.mic */
enum { EVT_MOUSE_DOWN = 2, EVT_MOUSE_UP = 3 };
enum { BTN_LEFT = 1, BTN_MIDDLE = 2, BTN_RIGHT = 3 };

enum { RAM_SIZE = 0x100000 };   /* 1 MByte, as on the RISC-5 computer */
enum { DISPLAY_GAP = RAM_SIZE - DISPLAY_BASE }; /* frame buffer and I/O area on top of the RAM */
enum { CYCLES_PER_SLICE = 20000 };
enum { STACK_SIZE = 0x8000 };   /* Kernel.stackSize; the stack lies between the
                                   image and the heap and grows down */

static uint8_t* ram = NULL;
static uint32_t ramSize = 0;
/* the frame buffer sits at the top of the RAM, as on the RISC-5 computer; with more
   RAM than 1 MByte it moves up with it, and Display derives its base from word 16 */
static uint32_t displayBase = DISPLAY_BASE;
static riscv_t* rv = NULL;
static int halted = 0;

/* device state */
static uint32_t spiCtrl = 0;
static uint32_t spiData = 0;   /* received word of the last transfer */
static uint32_t kbdCode = 0;
static int kbdReady = 0;
static uint32_t leds = 0;

/* dirty region of the display, in scan lines; Display.Mod writes into the
   frame buffer directly, there is nothing like an update call to hook into */
static int dirtyLo = DISPLAY_HEIGHT, dirtyHi = -1;

static void markDirty(uint32_t adr, uint32_t len)
{
    const uint32_t off = adr - displayBase;
    const int lo = (int)(off / DISPLAY_SPAN);
    const int hi = (int)((off + len - 1) / DISPLAY_SPAN);
    if( lo < dirtyLo )
        dirtyLo = lo;
    if( hi > dirtyHi )
        dirtyHi = hi;
}

static int isDisplay(uint32_t adr)
{
    return adr >= displayBase && adr < displayBase + DISPLAY_LEN;
}

/* the keyboard is a byte FIFO; the screen adapter delivers PS/2 set 2 codes */
static void kbdPoll(void)
{
    int32_t code;
    if( kbdReady )
        return;
    code = Screen$NextScanCode();
    if( code >= 0 )
    {
        kbdCode = (uint32_t)code;
        kbdReady = 1;
    }
}

/* Oberon polls the mouse word; the transitions are taken from the event queue and each of them is
   presented for KEY_HOLD milliseconds, like the real hardware does with a button which is pressed by a human */
enum { KEYQ_LEN = 64, KEY_HOLD = 30 };
static uint8_t keyQueue[KEYQ_LEN];
static int keyHead = 0, keyTail = 0, keyCount = 0;
static uint32_t curKeys = 0;   /* the state of the host buttons */
static uint32_t shownKeys = 0; /* the state the guest sees */
static uint32_t shownSince = 0;

static void keyEnqueue(uint32_t keys)
{
    if( keyCount == KEYQ_LEN )
        return;
    keyQueue[keyHead] = (uint8_t)keys;
    keyHead = (keyHead + 1) % KEYQ_LEN;
    keyCount++;
}

static uint32_t keyState(void)
{
    const uint32_t now = Screen$GetTicks();
    if( keyCount > 0 && now - shownSince >= KEY_HOLD )
    {
        shownKeys = keyQueue[keyTail];
        keyTail = (keyTail + 1) % KEYQ_LEN;
        keyCount--;
        shownSince = now;
    }
    else if( keyCount == 0 )
        shownKeys = curKeys;
    return shownKeys;
}

static void mousePoll(void)
{
    int32_t e = Screen$NextEvent();
    while( e != 0 )
    {
        const int type = (e >> 24) & 0xFF;
        const int btn = e & 0xFFFFFF;
        if( type == EVT_MOUSE_DOWN || type == EVT_MOUSE_UP )
        {
            /* the mouse word has right in bit 24, middle in 25, left in 26 */
            const uint32_t bit = btn == BTN_LEFT ? 4 : btn == BTN_MIDDLE ? 2 :
                                 btn == BTN_RIGHT ? 1 : 0;
            if( type == EVT_MOUSE_DOWN )
                curKeys |= bit;
            else
                curKeys &= ~bit;
            keyEnqueue(curKeys);
        }
        e = Screen$NextEvent();
    }
}

/* the word Input.Mouse reads: bits 0..9 x, bits 12..21 y, bits 24..26 the
   keys (right, middle, left), bit 28 a keyboard byte is available */
static uint32_t mouseState(void)
{
    int32_t x = 0, y = 0;
    int32_t keys;
    uint32_t w;

    Screen$GetMouseState(&x, &y);
    mousePoll();
    keys = (int32_t)keyState();

    if( x < 0 ) x = 0;
    if( y < 0 ) y = 0;
    if( x > DISPLAY_WIDTH - 1 ) x = DISPLAY_WIDTH - 1;
    if( y > DISPLAY_HEIGHT - 1 ) y = DISPLAY_HEIGHT - 1;

    /* Oberon counts the y axis from the bottom */
    y = DISPLAY_HEIGHT - 1 - y;

    kbdPoll();

    w = ((uint32_t)x & 0x3FF) | (((uint32_t)y & 0x3FF) << 12) |
        (((uint32_t)keys & 7) << 24);
    if( kbdReady )
        w |= 1u << 28;
    return w;
}

static uint32_t ioRead(uint32_t adr)
{
    switch( adr )
    {
    case IO_MSTIMER:
        return Screen$GetTicks();

    case IO_LEDS:
        return 0; /* switches */

    case IO_RS232D:
        return 0;

    case IO_RS232S:
        return 2; /* transmitter ready, no byte received */

    case IO_SPIDATA:
        return spiData;

    case IO_SPICTRL:
        return 1; /* the transfer is always finished when it is looked at */

    case IO_MOUSE:
        return mouseState();

    case IO_KBD:
        kbdReady = 0;
        return kbdCode;

    default:
        return 0;
    }
}

static void ioWrite(uint32_t adr, uint32_t val)
{
    switch( adr )
    {
    case IO_LEDS:
        leds = val;
        break;

    case IO_RS232D:
        putchar((int)(val & 0xFF));
        fflush(stdout);
        break;

    case IO_SPIDATA:
        /* a transfer is started by writing and its result is read afterwards */
        if( spiCtrl & 4 ) /* SPIFAST */
            spiData = diskTransfer32(val);
        else
            spiData = diskTransfer(val);
        break;

    case IO_SPICTRL:
        spiCtrl = val;
        diskSelect((val & 3) != 0); /* bit 0 selects card 0 */
        break;

    default:
        break;
    }
}

/* memory interface handed to the interpreter; everything below RAM_SIZE is
   plain memory, the topmost 64 bytes are the devices */

static riscv_word_t memIfetch(riscv_t* vm, riscv_word_t adr)
{
    (void)vm;
    if( adr + 4 > ramSize )
    {
        fprintf(stderr, "fetch outside memory at %08X\n", adr);
        halted = 1;
        return 0;
    }
    return *(uint32_t*)(ram + adr);
}

static riscv_word_t memReadW(riscv_t* vm, riscv_word_t adr)
{
    (void)vm;
    if( adr >= IO_START )
        return ioRead(adr);
    if( adr + 4 > ramSize )
        return 0;
    return *(uint32_t*)(ram + adr);
}

static riscv_half_t memReadS(riscv_t* vm, riscv_word_t adr)
{
    (void)vm;
    if( adr >= IO_START )
        return (riscv_half_t)ioRead(adr & ~3u);
    if( adr + 2 > ramSize )
        return 0;
    return *(uint16_t*)(ram + adr);
}

static riscv_byte_t memReadB(riscv_t* vm, riscv_word_t adr)
{
    (void)vm;
    if( adr >= IO_START )
        return (riscv_byte_t)ioRead(adr & ~3u);
    if( adr >= ramSize )
        return 0;
    return ram[adr];
}

static void memWriteW(riscv_t* vm, riscv_word_t adr, riscv_word_t val)
{
    (void)vm;
    if( adr >= IO_START )
    {
        ioWrite(adr, val);
        return;
    }
    if( adr + 4 > ramSize )
        return;
    *(uint32_t*)(ram + adr) = val;
    if( isDisplay(adr) )
        markDirty(adr, 4);
}

static void memWriteS(riscv_t* vm, riscv_word_t adr, riscv_half_t val)
{
    (void)vm;
    if( adr >= IO_START )
    {
        ioWrite(adr & ~3u, val);
        return;
    }
    if( adr + 2 > ramSize )
        return;
    *(uint16_t*)(ram + adr) = val;
    if( isDisplay(adr) )
        markDirty(adr, 2);
}

static void memWriteB(riscv_t* vm, riscv_word_t adr, riscv_byte_t val)
{
    (void)vm;
    if( adr >= IO_START )
    {
        ioWrite(adr & ~3u, val);
        return;
    }
    if( adr >= ramSize )
        return;
    ram[adr] = val;
    if( isDisplay(adr) )
        markDirty(adr, 1);
}

/* RISC-V semihosting, as used by the RawOut module: the EBREAK is embedded
    in the magic sequence SLLI x0,x0,31 / EBREAK / SRAI x0,x0,7 */
static void semihosting(riscv_t* vm)
{
    const uint32_t pc = vm->PC; /* the EBREAK itself */
    const uint32_t op = rv_get_reg(vm, rv_reg_a0);
    const uint32_t arg = rv_get_reg(vm, rv_reg_a1);

    if( pc < 4 || pc + 8 > ramSize ||
        *(uint32_t*)(ram + pc - 4) != 0x01F01013u ||
        *(uint32_t*)(ram + pc + 4) != 0x40705013u )
    {
        /* HALT(n) is compiled to LUI x0,n followed by EBREAK */
        const uint32_t prev = pc >= 4 ? *(uint32_t*)(ram + pc - 4) : 0;
        if( (prev & 0xFFFu) == 0x037u )
            fprintf(stderr, "HALT %u at %08X, called from %08X\n",
                    prev >> 12, pc, rv_get_reg(vm, rv_reg_ra));
        else
            fprintf(stderr, "unexpected breakpoint at %08X, ra %08X\n",
                    pc, rv_get_reg(vm, rv_reg_ra));
        halted = 1;
        rv_halt(vm);
        return;
    }

    switch( op )
    {
    case 0x03: /* SYS_WRITEC */
        if( arg < ramSize )
        {
            putchar(ram[arg]);
            fflush(stdout);
        }
        break;

    case 0x04: /* SYS_WRITE0 */
        {
            uint32_t p = arg;
            while( p < ramSize && ram[p] )
                putchar(ram[p++]);
            fflush(stdout);
        }
        break;

    case 0x18: /* SYS_EXIT */
        halted = 1;
        break;

    default:
        break;
    }

    rv_set_pc(vm, pc + 8); /* skip the SRAI too */
}

/* the interpreter calls this one directly for the hosted binaries it usually
   runs; a bare metal Oberon image has no use for it */
void syscall_handler(riscv_t* vm)
{
    (void)vm;
    fprintf(stderr, "unexpected system call\n");
    halted = 1;
}

static void onEcall(riscv_t* vm)
{
    (void)vm;
    fprintf(stderr, "unhandled ECALL\n");
    halted = 1;
}

static void onTrap(riscv_t* vm)
{
    fprintf(stderr, "trap: cause %d, PC %08X\n", (int)vm->csr_mcause,
            (unsigned)vm->PC);
    halted = 1;
}

static void onMemset(riscv_t* vm) { (void)vm; }
static void onMemcpy(riscv_t* vm) { (void)vm; }

/* rv_create insists on an ELF file, so the located raw image produced by the
   boot linker is wrapped in the smallest possible one */
static void makeElf(const char* image, const char* elf, uint32_t base)
{
    uint8_t hdr[0x54];
    FILE* in = fopen(image, "rb");
    FILE* out;
    long len;
    uint8_t* buf;

    if( in == NULL )
    {
        fprintf(stderr, "cannot open %s\n", image);
        exit(1);
    }
    fseek(in, 0, SEEK_END);
    len = ftell(in);
    fseek(in, 0, SEEK_SET);
    buf = malloc((size_t)len);
    if( fread(buf, 1, (size_t)len, in) != (size_t)len )
    {
        fprintf(stderr, "cannot read %s\n", image);
        exit(1);
    }
    fclose(in);

    memset(hdr, 0, sizeof(hdr));
    memcpy(hdr, "\177ELF\1\1\1", 7);
    hdr[0x10] = 2;              /* ET_EXEC */
    hdr[0x12] = 0xF3;           /* EM_RISCV */
    hdr[0x14] = 1;              /* version */
    *(uint32_t*)(hdr + 0x18) = base;   /* entry */
    *(uint32_t*)(hdr + 0x1C) = 0x34;   /* program header offset */
    hdr[0x28] = 0x34;           /* ehsize */
    hdr[0x2A] = 0x20;           /* phentsize */
    hdr[0x2C] = 1;              /* phnum */
    *(uint32_t*)(hdr + 0x34) = 1;   /* PT_LOAD */
    *(uint32_t*)(hdr + 0x38) = sizeof(hdr);  /* offset */
    *(uint32_t*)(hdr + 0x3C) = base;         /* vaddr */
    *(uint32_t*)(hdr + 0x40) = base;         /* paddr */
    *(uint32_t*)(hdr + 0x44) = (uint32_t)len;   /* filesz */
    *(uint32_t*)(hdr + 0x48) = (uint32_t)len;   /* memsz */
    *(uint32_t*)(hdr + 0x4C) = 7;      /* RWX */
    *(uint32_t*)(hdr + 0x50) = 4;      /* align */

    out = fopen(elf, "wb");
    if( out == NULL )
    {
        fprintf(stderr, "cannot write %s\n", elf);
        exit(1);
    }
    fwrite(hdr, 1, sizeof(hdr), out);
    fwrite(buf, 1, (size_t)len, out);
    fclose(out);
    free(buf);
}

static uint32_t rd32(const uint8_t* p)
{
    return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}

static uint16_t rd16(const uint8_t* p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

static void wr32(uint8_t* p, uint32_t v)
{
    p[0] = v & 0xFF; p[1] = (v >> 8) & 0xFF; p[2] = (v >> 16) & 0xFF; p[3] = (v >> 24) & 0xFF;
}

/* JAL x0, imm, i.e. an unconditional jump relative to the instruction */
static uint32_t jal(int32_t imm)
{
    const uint32_t u = (uint32_t)imm;
    return ((u >> 20 & 1) << 31) | ((u >> 1 & 0x3FF) << 21) | ((u >> 11 & 1) << 20) |
           ((u >> 12 & 0xFF) << 12) | 0x6F;
}

static void readElf(const char* path, uint32_t* entry, uint32_t* memEnd, uint32_t* dataOrg)
{
    uint8_t hdr[0x34];
    uint32_t phoff, i;
    uint16_t phentsize, phnum;
    FILE* f = fopen(path, "rb");

    if( f == NULL )
    {
        fprintf(stderr, "cannot open %s\n", path);
        exit(1);
    }
    if( fread(hdr, 1, sizeof(hdr), f) != sizeof(hdr) ||
        memcmp(hdr, "\177ELF\1\1\1", 7) != 0 )
    {
        fprintf(stderr, "%s is not a 32 bit little endian ELF file\n", path);
        exit(1);
    }
    *entry = rd32(hdr + 0x18);
    phoff = rd32(hdr + 0x1C);
    phentsize = rd16(hdr + 0x2A);
    phnum = rd16(hdr + 0x2C);

    *memEnd = 0;
    *dataOrg = 0xFFFFFFFFu;
    for( i = 0; i < phnum; i++ )
    {
        uint8_t ph[0x20];
        if( fseek(f, (long)(phoff + i * phentsize), SEEK_SET) != 0 ||
            fread(ph, 1, sizeof(ph), f) != sizeof(ph) )
        {
            fprintf(stderr, "%s is truncated\n", path);
            exit(1);
        }
        if( rd32(ph) != 1 ) /* PT_LOAD */
            continue;
        if( rd32(ph + 8) + rd32(ph + 20) > *memEnd )
            *memEnd = rd32(ph + 8) + rd32(ph + 20); /* vaddr + memsz */
        if( (rd32(ph + 24) & 2) && rd32(ph + 8) < *dataOrg ) /* PF_W */
            *dataOrg = rd32(ph + 8);
    }
    fclose(f);
    if( *memEnd == 0 )
    {
        fprintf(stderr, "no loadable segment in %s\n", path);
        exit(1);
    }
    /* the heap starts after the uninitialized data, which the machine zeroes */
    *memEnd = (*memEnd + 31) & ~31u;
    if( *dataOrg == 0xFFFFFFFFu )
        *dataOrg = *memEnd;
    *dataOrg &= ~3u;
}

static volatile int interrupted = 0;

static void onInterrupt(int sig)
{
    (void)sig;
    interrupted = 1;
}

static void usage(const char* prog)
{
    fprintf(stderr,
            "usage: %s [options] <boot image>\n"
            "  --base <hex>    load address of the image (default 1000)\n"
            "  --elf           the file is an ELF executable, not a raw image\n"
            "  --disk <file>   disk image served as SD card\n"
            "  --ram <MB>      RAM in MByte (default 1, as on the RISC-5 computer)\n"
            "  --noscreen      run without display, for the inner core\n",
            prog);
    exit(1);
}

int main(int argc, char** argv)
{
    const char* imageFile = NULL;
    const char* diskFile = NULL;
    uint32_t base = 0x1000;
    int noScreen = 0;
    int elfMode = 0;
    uint32_t entry, memEnd = 0, dataOrg = 0, ramWanted = RAM_SIZE;
    char elfFile[256];
    vm_attr_t attr;
    riscv_io_t io;
    int i;

    for( i = 1; i < argc; i++ )
    {
        if( strcmp(argv[i], "--base") == 0 && i + 1 < argc )
            base = (uint32_t)strtoul(argv[++i], NULL, 16);
        else if( strcmp(argv[i], "--elf") == 0 )
            elfMode = 1;
        else if( strcmp(argv[i], "--disk") == 0 && i + 1 < argc )
            diskFile = argv[++i];
        else if( strcmp(argv[i], "--ram") == 0 && i + 1 < argc )
            ramWanted = (uint32_t)strtoul(argv[++i], NULL, 10) * 0x100000u;
        else if( strcmp(argv[i], "--noscreen") == 0 )
            noScreen = 1;
        else if( argv[i][0] == '-' )
            usage(argv[0]);
        else
            imageFile = argv[i];
    }
    if( imageFile == NULL )
        usage(argv[0]);

    if( elfMode )
    {
        snprintf(elfFile, sizeof(elfFile), "%s", imageFile);
        readElf(elfFile, &entry, &memEnd, &dataOrg);
        base = entry;
    }else
    {
        snprintf(elfFile, sizeof(elfFile), "%s.elf", imageFile);
        makeElf(imageFile, elfFile, base);
    }

    memset(&attr, 0, sizeof(attr));
    if( ramWanted < RAM_SIZE )
        ramWanted = RAM_SIZE;
    displayBase = ramWanted - DISPLAY_GAP;
    attr.mem_size = ramWanted;
    attr.stack_size = 0;
    attr.cycle_per_step = CYCLES_PER_SLICE;
    attr.allow_misalign = true;
    attr.log_level = 4; /* fatal only */
    attr.data.user.elf_program = elfFile;

    rv = rv_create(&attr);
    if( rv == NULL )
    {
        fprintf(stderr, "cannot create the interpreter\n");
        return 1;
    }
    ram = attr.mem->mem_base;
    ramSize = (uint32_t)attr.mem->mem_size;

    if( elfMode )
    {
        /* the boot header the raw image carries in its first 32 bytes; the boot
           linker leaves this area free, an ELF has its own header there */
        memset(ram, 0, 32);
        wr32(ram, jal((int32_t)entry));
        wr32(ram + 8, memEnd);
        wr32(ram + 12, dataOrg);  /* start of the writable data, the root area of the collector */
        wr32(ram + 16, displayBase - 16); /* the RAM ends at the frame buffer, as in the image */
        printf("%s: entry %08X, data %08X, heap origin %08X, memory limit %08X\n",
               imageFile, entry, dataOrg, memEnd, rd32(ram + 16));
    }

    memset(&io, 0, sizeof(io));
    io.mem_ifetch = memIfetch;
    io.mem_read_w = memReadW;
    io.mem_read_s = memReadS;
    io.mem_read_b = memReadB;
    io.mem_write_w = memWriteW;
    io.mem_write_s = memWriteS;
    io.mem_write_b = memWriteB;
    io.on_ecall = onEcall;
    io.on_ebreak = semihosting;
    io.on_memset = onMemset;
    io.on_memcpy = onMemcpy;
    io.on_trap = onTrap;
    memcpy(&rv->io, &io, sizeof(riscv_io_t));

    diskOpen(diskFile);

    if( !noScreen )
    {
        if( !Screen$Open(ram + displayBase, DISPLAY_LEN,
                         DISPLAY_WIDTH, DISPLAY_HEIGHT, 1 + 2) ) /* LSB first, bottom up */
        {
            fprintf(stderr, "cannot open the screen\n");
            return 1;
        }
        /* Oberon draws its own cursor into the frame buffer */
        Screen$ShowCursor(0);
    }

    rv_set_pc(rv, base);
    /* word 8 of the image header is the heap start, i.e. the end of the image;
       Kernel.Init derives the same stack and heap bounds from it */
    rv_set_reg(rv, rv_reg_sp, *(uint32_t*)(ram + 8) + STACK_SIZE);

    signal(SIGINT, onInterrupt);

    while( !halted && !rv_has_halted(rv) )
    {
        rv_step(rv);

        if( interrupted )
        {
            fprintf(stderr, "interrupted at %08X\n", rv->PC);
            break;
        }

        if( !noScreen )
        {
            if( dirtyHi >= dirtyLo )
            {
                /* the frame buffer is bottom up, the screen adapter counts its lines from the top */
                const int top = DISPLAY_HEIGHT - 1 - dirtyHi;
                Screen$UpdateArea(0, top, DISPLAY_WIDTH, dirtyHi - dirtyLo + 1);
                dirtyLo = DISPLAY_HEIGHT;
                dirtyHi = -1;
            }
            if( Screen$ProcessEvents(0) < 0 )
                break;
        }
    }

    if( !noScreen )
        Screen$Close();
    diskClose();
    return 0;
}
