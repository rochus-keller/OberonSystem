MODULE Kernel;  (*NW/PR  11.4.86 / 27.12.95 / 4.2.2014*)
  IMPORT SYSTEM, SYS;
  CONST SectorLength* = 1024;
    timer = -64; spiData = -48; spiCtrl = -44;
    CARD0 = 1; SPIFAST = 4;
    HdrLen = 16; (*heap block header, see New*)
    RecBlk = 0; ArrBlk = 1; SysBlk = 2; FreeBlk = -1;
    FSoffset = 0; (*original: 80000H, 256MB in 512-byte blocks, to skip the FAT
      partition of Wirth's SD card; the emulator serves a plain disk image*)
    mapsize = 10000H; (*1K sectors, 64MB*)

  TYPE Sector* = ARRAY SectorLength OF SYSTEM.BYTE;

    Name* = ARRAY 32 OF CHAR;

    Command* = RECORD name*: Name; adr*: LONGINT END;
    Commands* = POINTER TO ARRAY OF Command;
    Entries* = POINTER TO ARRAY OF LONGINT;
    Imports* = POINTER TO ARRAY OF Module;

    Exports* = POINTER TO ARRAY OF ExportDesc;
    ExportDesc* = RECORD fp*, adr*: LONGINT; nofExp*, pad*: INTEGER; dsc*: Exports END;

    Module* = POINTER TO ModuleDesc;
    ModuleDesc* = RECORD
      link*: Module;
      name*: Name;
      init*, trapped*: SYSTEM.BYTE; pad*: INTEGER;
      refcnt*, sb*: LONGINT;
      entries*: Entries;
      cmds*: Commands;
      ptrTab*, tdescs*: Entries;
      imports*: Imports;
      data*, code*, refs*, publics*, privates*: LONGINT;
      nofimp*, import*, nofstrc*, strct*, nofreimp*, reimp*: LONGINT;
      export*: ExportDesc;
      term*: LONGINT
    END;

  VAR allocated*, NofSectors*: LONGINT;
    modules*: Module;  (*module list, built by the boot linker; deviation from
      Wirth's design, whose Modules.Mod loads and links the outer core itself*)
    heapOrg*, heapLim*: LONGINT; 
    stackOrg* ,  stackSize*, MemLim*: LONGINT;
    clock: LONGINT;
    ctrl: LONGINT;  (*Wirth's original polls the SPI status with SYSTEM.BIT(spiCtrl, 0)
      which OP2 relocates as a module data offset when the address is a constant *)
    list0, list1, list2, list3: LONGINT;  (*lists of free blocks of size n*256, 128, 64, 32 bytes*)
    data: LONGINT; (*SPI data in*)
    sectorMap: ARRAY mapsize DIV 32 OF SET;
    
(* ---------- New: heap allocation ----------*)

  PROCEDURE GetBlock(VAR p: LONGINT; len: LONGINT);
    (*len is multiple of 256*)
    VAR q0, q1, q2, size: LONGINT; done: BOOLEAN;
  BEGIN q0 := 0; q1 := list0; done := FALSE;
    WHILE ~done & (q1 # 0) DO
      SYSTEM.GET(q1, size); SYSTEM.GET(q1+8, q2);
      IF size < len THEN (*no fit*) q0 := q1; q1 := q2
      ELSIF size = len THEN (*extract -> p*)
        done := TRUE; p := q1;
        IF q0 # 0 THEN SYSTEM.PUT(q0+8, q2) ELSE list0 := q2 END
      ELSE (*reduce size*)
        done := TRUE; p := q1; q1 := q1 + len;
        SYSTEM.PUT(q1, size-len); SYSTEM.PUT(q1+4, SYSTEM.VAL(LONGINT, -1)); SYSTEM.PUT(q1+8, q2);
        IF q0 # 0 THEN SYSTEM.PUT(q0+8, q1) ELSE list0 := q1 END
      END
    END ;
    IF ~done THEN p := 0 END
  END GetBlock;

  PROCEDURE GetBlock128(VAR p: LONGINT);
    VAR q: LONGINT;
  BEGIN
    IF list1 # 0 THEN p := list1; SYSTEM.GET(list1+8, list1)
    ELSE GetBlock(q, 256); SYSTEM.PUT(q+128, SYSTEM.VAL(LONGINT, 128)); SYSTEM.PUT(q+132, SYSTEM.VAL(LONGINT, -1)); SYSTEM.PUT(q+136, list1);
      list1 := q + 128; p := q
    END
  END GetBlock128;

  PROCEDURE GetBlock64(VAR p: LONGINT);
    VAR q: LONGINT;
  BEGIN
    IF list2 # 0 THEN p := list2; SYSTEM.GET(list2+8, list2)
    ELSE GetBlock128(q); SYSTEM.PUT(q+64, SYSTEM.VAL(LONGINT, 64)); SYSTEM.PUT(q+68, SYSTEM.VAL(LONGINT, -1)); SYSTEM.PUT(q+72, list2);
      list2 := q + 64; p := q
    END
  END GetBlock64;

  PROCEDURE GetBlock32(VAR p: LONGINT);
    VAR q: LONGINT;
  BEGIN
    IF list3 # 0 THEN p := list3; SYSTEM.GET(list3+8, list3)
    ELSE GetBlock64(q); SYSTEM.PUT(q+32, SYSTEM.VAL(LONGINT, 32)); SYSTEM.PUT(q+36, SYSTEM.VAL(LONGINT, -1)); SYSTEM.PUT(q+40, list3);
      list3 := q + 32; p := q
    END
  END GetBlock32;

  PROCEDURE New(VAR ptr: LONGINT; tag, kind, len: LONGINT);
    (*OP2 expects the type tag at ptr-4 and a type descriptor holding the plain record size. 
      The block therefore carries a 16 bytes header. Free blocks keep Wirth's layout*)
    VAR p, size, lim: LONGINT;
  BEGIN size := (len + HdrLen + 7) DIV 8 * 8;
    IF size <= 32 THEN size := 32; GetBlock32(p)
    ELSIF size <= 64 THEN size := 64; GetBlock64(p)
    ELSIF size <= 128 THEN size := 128; GetBlock128(p)
    ELSE size := (size+255) DIV 256 * 256; GetBlock(p, size)
    END ;
    IF p = 0 THEN ptr := 0
    ELSE ptr := p + HdrLen;
      SYSTEM.PUT(p, size); SYSTEM.PUT(p+4, kind); SYSTEM.PUT(p+8, SYSTEM.VAL(LONGINT, 0)); SYSTEM.PUT(p+12, tag);
      lim := p + size; INC(p, HdrLen); INC(allocated, size);
      WHILE p < lim DO SYSTEM.PUT(p, SYSTEM.VAL(LONGINT, 0)); INC(p, 4) END
    END
  END New;

  PROCEDURE NewRec*(VAR ptr: LONGINT; tag: LONGINT);
    (*called by NEW; ptr and tag are pointers*)
    VAR size: LONGINT;
  BEGIN SYSTEM.GET(tag, size); New(ptr, tag, RecBlk, size)
  END NewRec;

  PROCEDURE NewSys*(VAR ptr: LONGINT; size: LONGINT);
    (*called by SYSTEM.NEW; the block has no type descriptor*)
  BEGIN New(ptr, 0, SysBlk, size)
  END NewSys;

  PROCEDURE NewArr*(VAR ptr: LONGINT; eltag, nofelem, nofdim: LONGINT);
    (*called by NEW for open arrays of records, pointers or delegates*)
    VAR p, elsize, first, size, i: LONGINT;
  BEGIN SYSTEM.GET(eltag, elsize);
    first := 16 + 8 * (nofdim DIV 2); (*as generated by the compiler*)
    size := first + nofelem * elsize;
    New(p, eltag, ArrBlk, size);
    IF p # 0 THEN
      SYSTEM.PUT(p, p + size); SYSTEM.PUT(p+4, SYSTEM.VAL(LONGINT, 0));
      SYSTEM.PUT(p+8, p + first); SYSTEM.PUT(p+12, nofelem);
      i := 1;
      WHILE i < nofdim DO SYSTEM.PUT(p+12 + i*4, SYSTEM.VAL(LONGINT, 0)); INC(i) END
    END ;
    ptr := p
  END NewArr;

(* ---------- Garbage collector ----------*)

  PROCEDURE Mark*(pref: LONGINT);
    (*deviation from Wirth's original*)
    VAR pvadr, offadr, offset, tag, kind, p, q, r: LONGINT;
  BEGIN SYSTEM.GET(pref, pvadr); (*pointers < heapOrg considered NIL*)
    WHILE pvadr # 0 DO
      SYSTEM.GET(pvadr, p); SYSTEM.GET(p-8, offadr);
      IF (p >= heapOrg) & (offadr = 0) THEN
        SYSTEM.GET(p-12, kind);
        IF kind # RecBlk THEN SYSTEM.PUT(p-8, p) (*mark only*)
        ELSE q := p;   (*mark elements in data structure with root p*)
          REPEAT SYSTEM.GET(p-8, offadr);
            IF offadr = 0 THEN SYSTEM.GET(p-4, tag); offadr := tag + 4 ELSE INC(offadr, 4) END ;
            SYSTEM.PUT(p-8, offadr); SYSTEM.GET(offadr, offset);
            IF offset >= 0 THEN (*down*)
              SYSTEM.GET(p+offset, r); SYSTEM.GET(r-8, offadr);
              IF (r >= heapOrg) & (offadr = 0) THEN SYSTEM.PUT(p+offset, q); q := p; p := r END
            ELSE (*up*) SYSTEM.GET(q-8, offadr); SYSTEM.GET(offadr, offset);
              IF p # q THEN SYSTEM.GET(q+offset, r); SYSTEM.PUT(q+offset, p); p := q; q := r END
            END
          UNTIL (p = q) & (offset < 0)
        END
      END ;
      INC(pref, 4); SYSTEM.GET(pref, pvadr)
    END
  END Mark;

  PROCEDURE Scan*;
    (*deviation from Wirth's original*)
    VAR p, q, mark, kind, size: LONGINT;
  BEGIN p := heapOrg;
    REPEAT SYSTEM.GET(p+4, kind); SYSTEM.GET(p+8, mark);
      IF kind = FreeBlk THEN mark := -1 END ;
      q := p;
      WHILE mark = 0 DO
        SYSTEM.GET(p, size); INC(p, size); SYSTEM.GET(p+4, kind); SYSTEM.GET(p+8, mark);
        IF kind = FreeBlk THEN mark := -1 END
      END ;
      size := p - q; DEC(allocated, size);  (*size of free block*)
      IF size > 0 THEN
        IF size MOD 64 # 0 THEN
          SYSTEM.PUT(q, SYSTEM.VAL(LONGINT, 32)); SYSTEM.PUT(q+4, SYSTEM.VAL(LONGINT, -1)); SYSTEM.PUT(q+8, list3); list3 := q; INC(q, 32); DEC(size, 32)
        END ;
        IF size MOD 128 # 0 THEN
          SYSTEM.PUT(q, SYSTEM.VAL(LONGINT, 64)); SYSTEM.PUT(q+4, SYSTEM.VAL(LONGINT, -1)); SYSTEM.PUT(q+8, list2); list2 := q; INC(q, 64); DEC(size, 64)
        END ;
        IF size MOD 256 # 0 THEN
          SYSTEM.PUT(q, SYSTEM.VAL(LONGINT, 128)); SYSTEM.PUT(q+4, SYSTEM.VAL(LONGINT, -1)); SYSTEM.PUT(q+8,  list1); list1 := q; INC(q, 128); DEC(size, 128)
        END ;
        IF size > 0 THEN
          SYSTEM.PUT(q, size); SYSTEM.PUT(q+4, SYSTEM.VAL(LONGINT, -1)); SYSTEM.PUT(q+8, list0); list0 := q; INC(q, size)
        END
      END ;
      IF mark > 0 THEN SYSTEM.GET(p, size); SYSTEM.PUT(p+8, SYSTEM.VAL(LONGINT, 0)); INC(p, size)
      ELSE (*free*) SYSTEM.GET(p, size); INC(p, size)
      END
    UNTIL p >= heapLim
  END Scan;

(* ---------- Disk storage management ----------*)

  PROCEDURE SPIIdle(n: LONGINT); (*send n FFs slowly with no card selected*)
  BEGIN SYSTEM.PUT(spiCtrl, SYSTEM.VAL(LONGINT, 0));
    WHILE n > 0 DO DEC(n); SYSTEM.PUT(spiData, SYSTEM.VAL(LONGINT, -1));
      REPEAT SYSTEM.GET(spiCtrl, ctrl) UNTIL ODD(ctrl);
      SYSTEM.GET(spiData, data)
    END
  END SPIIdle;

  PROCEDURE SPI(n: LONGINT); (*send&rcv byte slowly with card selected*)
  BEGIN SYSTEM.PUT(spiCtrl, SYSTEM.VAL(LONGINT, CARD0)); SYSTEM.PUT(spiData, n);
    REPEAT SYSTEM.GET(spiCtrl, ctrl) UNTIL ODD(ctrl);
    SYSTEM.GET(spiData, data)
  END SPI;

  PROCEDURE SPICmd(n, arg: LONGINT);
    VAR i, crc: LONGINT;
  BEGIN (*send cmd*)
    REPEAT SPIIdle(1) UNTIL data = 255; (*flush while unselected*)
    REPEAT SPI(255) UNTIL data = 255; (*flush while selected*)
    IF n = 8 THEN crc := 135 ELSIF n = 0 THEN crc := 149 ELSE crc := 255 END;
    SPI(n MOD 64 + 64); (*send command*)
    FOR i := 24 TO 0 BY -8 DO SPI(SYSTEM.ROT(arg, -i)) END; (*send arg*)
    SPI(crc); i := 32;
    REPEAT SPI(255); DEC(i) UNTIL (data < 80H) OR (i = 0)
  END SPICmd;

  PROCEDURE SDShift(VAR n: LONGINT);
    VAR data: LONGINT;
  BEGIN SPICmd(58, 0);  (*CMD58 get card capacity bit*)
    SYSTEM.GET(spiData, data); SPI(-1);
    SYSTEM.GET(spiData, ctrl);
    IF (data # 0) OR ~ODD(ctrl DIV 40H) THEN n := n * 512 END ;  (*non-SDHC card*)
    SPI(-1); SPI(-1); SPIIdle(1)  (*flush response*)
  END SDShift;

  PROCEDURE ReadSD(src, dst: LONGINT);
    VAR i: LONGINT;
  BEGIN SDShift(src); SPICmd(17, src); SYS.ASSERT(data = 0); (*CMD17 read one block*)
    i := 0; (*wait for start data marker*)
    REPEAT SPI(-1); INC(i) UNTIL data = 254;
    SYSTEM.PUT(spiCtrl, SYSTEM.VAL(LONGINT, SPIFAST + CARD0));
    FOR i := 0 TO 508 BY 4 DO
      SYSTEM.PUT(spiData, SYSTEM.VAL(LONGINT, -1));
      REPEAT SYSTEM.GET(spiCtrl, ctrl) UNTIL ODD(ctrl);
      SYSTEM.GET(spiData, data); SYSTEM.PUT(dst, data); INC(dst, 4)
    END;
    SPI(255); SPI(255); SPIIdle(1) (*may be a checksum; deselect card*)
  END ReadSD;

  PROCEDURE WriteSD(dst, src: LONGINT);
    VAR i, n: LONGINT; x: SYSTEM.BYTE;
  BEGIN SDShift(dst); SPICmd(24, dst); SYS.ASSERT(data = 0); (*CMD24 write one block*)
    SPI(254); (*write start data marker*)
    SYSTEM.PUT(spiCtrl, SYSTEM.VAL(LONGINT, SPIFAST + CARD0));
    FOR i := 0 TO 508 BY 4 DO
      SYSTEM.GET(src, n); INC(src, 4); SYSTEM.PUT(spiData, n);
      REPEAT SYSTEM.GET(spiCtrl, ctrl) UNTIL ODD(ctrl)
    END;
    SPI(255); SPI(255); (*dummy checksum*) i := 0;
    REPEAT SPI(-1); INC(i); UNTIL (data MOD 32 = 5) OR (i = 10000);
    SYS.ASSERT(data MOD 32 = 5); SPIIdle(1) (*deselect card*)
  END WriteSD;

  PROCEDURE InitSecMap*;
    VAR i: LONGINT;
  BEGIN NofSectors := 0; sectorMap[0] := {0 .. 31}; sectorMap[1] := {0 .. 31};
    FOR i := 2 TO mapsize DIV 32 - 1 DO sectorMap[i] := {} END
  END InitSecMap;

  PROCEDURE MarkSector*(sec: LONGINT);
  BEGIN sec := sec DIV 29; SYS.ASSERT(SYS.H(0) = 0);
    INCL(sectorMap[sec DIV 32], sec MOD 32); INC(NofSectors)
  END MarkSector;

  PROCEDURE FreeSector*(sec: LONGINT);
  BEGIN sec := sec DIV 29; SYS.ASSERT(SYS.H(0) = 0);
    EXCL(sectorMap[sec DIV 32], sec MOD 32); DEC(NofSectors)
  END FreeSector;

  PROCEDURE AllocSector*(hint: LONGINT; VAR sec: LONGINT);
    VAR s: LONGINT;
  BEGIN (*find free sector, starting after hint*)
    hint := hint DIV 29; SYS.ASSERT(SYS.H(0) = 0); s := hint;
    REPEAT INC(s);
      IF s = mapsize THEN s := 1 END ;
    UNTIL ~(s MOD 32 IN sectorMap[s DIV 32]);
    INCL(sectorMap[s DIV 32], s MOD 32); INC(NofSectors); sec := s * 29
  END AllocSector;

  PROCEDURE GetSector*(src: LONGINT; VAR dst: ARRAY OF SYSTEM.BYTE);
  BEGIN src := src DIV 29; SYS.ASSERT(SYS.H(0) = 0);
    src := src * 2 + FSoffset;
    ReadSD(src, SYSTEM.ADR(dst)); ReadSD(src+1, SYSTEM.ADR(dst)+512) 
  END GetSector;
  
  PROCEDURE PutSector*(dst: LONGINT; VAR src: ARRAY OF SYSTEM.BYTE);
  BEGIN dst := dst DIV 29; SYS.ASSERT(SYS.H(0) =  0);
    dst := dst * 2 + FSoffset;
    WriteSD(dst, SYSTEM.ADR(src)); WriteSD(dst+1, SYSTEM.ADR(src)+512)
  END PutSector;

(*-------- Miscellaneous procedures----------*)

  PROCEDURE Time*(): LONGINT;
    VAR t: LONGINT;
  BEGIN SYSTEM.GET(timer, t); RETURN t
  END Time;

  PROCEDURE Clock*(): LONGINT;
  BEGIN RETURN clock
  END Clock;

  PROCEDURE SetClock*(dt: LONGINT);
  BEGIN clock := dt
  END SetClock;

  PROCEDURE Install*(Padr, at: LONGINT);
  BEGIN SYSTEM.PUT(at, 0E7000000H + (Padr - at) DIV 4 -1)
  END Install;

  PROCEDURE Trap(VAR a: LONGINT; b: LONGINT);
    (*deviation from Wirth's original*)
    VAR u, v, w: LONGINT;
  BEGIN u := SYS.REG(15); SYSTEM.GET(u - 4, v); w := v DIV 10H MOD 10H; (*trap number*)
    IF w = 0 THEN NewRec(a, b)
    ELSE (*stop*) SYS.LED(w + 192); REPEAT UNTIL FALSE
    END
  END Trap;

  PROCEDURE Init*;
    (*deviation from Wirth's original*)
  BEGIN
    SYSTEM.GET(16, MemLim); SYSTEM.GET(8, heapOrg);
    stackSize := 8000H; INC(heapOrg, stackSize);
    stackOrg := heapOrg; heapLim := MemLim;
    list1 := 0; list2 := 0; list3 := 0; list0 := heapOrg;
    SYSTEM.PUT(list0, heapLim - heapOrg); SYSTEM.PUT(list0+4, SYSTEM.VAL(LONGINT, -1)); SYSTEM.PUT(list0+8, SYSTEM.VAL(LONGINT, 0));
    allocated := 0; clock := 0; InitSecMap
  END Init;

END Kernel.
