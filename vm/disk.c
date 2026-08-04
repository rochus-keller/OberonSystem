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

/* SD card connected via SPI, as expected by Kernel.ReadSD/WriteSD of
   Project Oberon; the card is backed by a disk image file */
   
#include "machine.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { BLOCK_LEN = 512, CMD_LEN = 6 };

/* what the card is doing with the bytes it receives next */
enum State {
    ST_IDLE,  /* waiting for a command */
    ST_CMD,   /* collecting the remaining command bytes */
    ST_RESPOND,  /* handing out the response/data bytes queued in tx */
    ST_TOKEN,   /* waiting for the data token of a write command */
    ST_RECEIVE,  /* collecting the data block of a write command */
    ST_CRC    /* swallowing the two checksum bytes of a write command */
};

static FILE* image = NULL;
static int selected = 0;
static int state = ST_IDLE;
static uint8_t cmd[CMD_LEN];
static int cmdLen = 0;
static uint8_t tx[BLOCK_LEN + 8];
static int txLen = 0, txPos = 0;
static uint8_t rx[BLOCK_LEN];
static int rxPos = 0;
static uint32_t blockAdr = 0;
static uint8_t last = 0xFF;

static void txReset(void)
{
    txLen = 0;
    txPos = 0;
}

static void txPut(uint8_t b)
{
    if( txLen < (int)sizeof(tx) )
        tx[txLen++] = b;
}

void diskOpen(const char* path)
{
    diskClose();
    if( path == NULL )
        return;
    image = fopen(path, "r+b");
    if( image == NULL )
    {
        fprintf(stderr, "cannot open disk image %s\n", path);
        exit(1);
    }
}

void diskClose(void)
{
    if( image )
        fclose(image);
    image = NULL;
    selected = 0;
    state = ST_IDLE;
    cmdLen = 0;
    txReset();
}

void diskSelect(int on)
{
    if( !on && selected )
    {
        /* deselecting aborts whatever is in flight */
        if( state == ST_CMD )
            cmdLen = 0;
    }
    selected = on;
}

static void readBlock(uint32_t adr)
{
    /* the card sends the start token, the block and a (here fake) checksum */
    txReset();
    txPut(0xFE);
    if( image && fseek(image, (long)adr * BLOCK_LEN, SEEK_SET) == 0 )
    {
        uint8_t buf[BLOCK_LEN];
        const size_t n = fread(buf, 1, BLOCK_LEN, image);
        memset(buf + n, 0, BLOCK_LEN - n);
        for( int i = 0; i < BLOCK_LEN; i++ )
            txPut(buf[i]);
    } else
    {
        for( int i = 0; i < BLOCK_LEN; i++ )
            txPut(0);
    }
    txPut(0xFF);
    txPut(0xFF);
    state = ST_RESPOND;
}

static void writeBlock(uint32_t adr)
{
    if( image == NULL )
        return;
    if( fseek(image, (long)adr * BLOCK_LEN, SEEK_SET) == 0 )
    {
        fwrite(rx, 1, BLOCK_LEN, image);
        fflush(image);
    }
}

static void runCommand(void)
{
    const uint8_t n = cmd[0] & 0x3F;
    const uint32_t arg = ((uint32_t)cmd[1] << 24) | ((uint32_t)cmd[2] << 16) |
                         ((uint32_t)cmd[3] << 8) | (uint32_t)cmd[4];

    txReset();
    switch( n )
    {
    case 0: /* GO_IDLE_STATE */
        txPut(0x01);
        state = ST_RESPOND;
        break;

    case 8: /* SEND_IF_COND; answer as a version 2 card */
        txPut(0x01);
        txPut(0x00); txPut(0x00); txPut(0x01); txPut((uint8_t)(arg & 0xFF));
        state = ST_RESPOND;
        break;

    case 55: /* APP_CMD */
    case 41: /* SEND_OP_COND (ACMD41), initialization completed */
        txPut(0x00);
        state = ST_RESPOND;
        break;

    case 58: /* READ_OCR; bit 30 (CCS) set */
        txPut(0x00);
        txPut(0xC0); txPut(0xFF); txPut(0x80); txPut(0x00);
        state = ST_RESPOND;
        break;

    case 17: /* READ_SINGLE_BLOCK */
        readBlock(arg);
        /* the R1 response precedes the start token */
        memmove(tx + 1, tx, (size_t)txLen);
        tx[0] = 0x00;
        txLen++;
        state = ST_RESPOND;
        break;

    case 24: /* WRITE_BLOCK */
        blockAdr = arg;
        txPut(0x00);
        state = ST_RESPOND;
        break;

    default:
        txPut(0x00);
        state = ST_RESPOND;
        break;
    }
}

static uint8_t transferByte(uint8_t out)
{
    uint8_t in = 0xFF;

    if( !selected )
        return 0xFF;

    switch( state )
    {
    case ST_IDLE:
        if( (out & 0xC0) == 0x40 )
        {
            cmd[0] = out;
            cmdLen = 1;
            state = ST_CMD;
        }
        break;

    case ST_CMD:
        cmd[cmdLen++] = out;
        if( cmdLen == CMD_LEN )
        {
            cmdLen = 0;
            runCommand();
        }
        break;

    case ST_RESPOND:
        if( txPos < txLen )
            in = tx[txPos++];
        if( txPos >= txLen )
        {
            txReset();
            /* after the acknowledge of a write command the host sends the block */
            state = (cmd[0] & 0x3F) == 24 ? ST_TOKEN : ST_IDLE;
        }
        break;

    case ST_TOKEN:
        if( out == 0xFE )
        {
            rxPos = 0;
            state = ST_RECEIVE;
        }
        break;

    case ST_RECEIVE:
        if( rxPos < BLOCK_LEN )
            rx[rxPos++] = out;
        if( rxPos == BLOCK_LEN )
        {
            writeBlock(blockAdr);
            rxPos = 0;
            state = ST_CRC;
        }
        break;

    case ST_CRC:
        rxPos++;
        if( rxPos >= 2 )
        {
            /* the data response: 0bxxx00101 means accepted */
            txReset();
            txPut(0x05);
            rxPos = 0;
            state = ST_RESPOND;
            /* the write command is done, don't wait for another block */
            cmd[0] = 0;
        }
        break;
    }

    return in;
}

uint32_t diskTransfer(uint32_t out)
{
    last = transferByte((uint8_t)(out & 0xFF));
    return last;
}

uint32_t diskTransfer32(uint32_t out)
{
    uint32_t in = 0;
    for( int i = 0; i <= 24; i += 8 )
        in = in | ((uint32_t)transferByte((uint8_t)((out >> i) & 0xFF)) << i);
    last = (uint8_t)(in >> 24);
    return in;
}
