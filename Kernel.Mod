MODULE Kernel;  (*NW/PR  11.4.86 / 27.12.95 / 4.2.2014*)
  IMPORT SYSTEM, OBX;
  CONST SectorLength* = 1024;
    FSoffset = 80000H; (*256MB in 512-byte blocks*)
    mapsize = 10000H; (*1K sectors, 64MB*)

  TYPE Sector* = ARRAY SectorLength OF BYTE;

  VAR allocated*, NofSectors*: INTEGER;
    heapOrg*, heapLim*: INTEGER; 
    stackOrg* ,  stackSize*, MemLim*: INTEGER;
    clock: INTEGER;
    list0, list1, list2, list3: INTEGER;  (*lists of free blocks of size n*256, 128, 64, 32 bytes*)
    data: INTEGER; (*SPI data in*)
    sectorMap: ARRAY mapsize DIV 32 OF SET;
    
(* ---------- New: heap allocation ----------*)

  PROCEDURE GetBlock(VAR p: INTEGER; len: INTEGER);
    (*len is multiple of 256*)
    VAR q0, q1, q2, size: INTEGER; done: BOOLEAN;
  BEGIN q0 := 0; q1 := list0; done := FALSE;
    WHILE ~done & (q1 # 0) DO
      SYSTEM.GETI(q1, size); SYSTEM.GETI(q1+8, q2);
      IF size < len THEN (*no fit*) q0 := q1; q1 := q2
      ELSIF size = len THEN (*extract -> p*)
        done := TRUE; p := q1;
        IF q0 # 0 THEN SYSTEM.PUTI(q0+8, q2) ELSE list0 := q2 END
      ELSE (*reduce size*)
        done := TRUE; p := q1; q1 := q1 + len;
        SYSTEM.PUTI(q1, size-len); SYSTEM.PUTI(q1+4, -1); SYSTEM.PUTI(q1+8, q2);
        IF q0 # 0 THEN SYSTEM.PUTI(q0+8, q1) ELSE list0 := q1 END
      END
    END ;
    IF ~done THEN p := 0 END
  END GetBlock;

  PROCEDURE GetBlock128(VAR p: INTEGER);
    VAR q: INTEGER;
  BEGIN
    IF list1 # 0 THEN p := list1; SYSTEM.GETI(list1+8, list1)
    ELSE GetBlock(q, 256); SYSTEM.PUTI(q+128, 128); SYSTEM.PUTI(q+132, -1); SYSTEM.PUTI(q+136, list1);
      list1 := q + 128; p := q
    END
  END GetBlock128;

  PROCEDURE GetBlock64(VAR p: INTEGER);
    VAR q: INTEGER;
  BEGIN
    IF list2 # 0 THEN p := list2; SYSTEM.GETI(list2+8, list2)
    ELSE GetBlock128(q); SYSTEM.PUTI(q+64, 64); SYSTEM.PUTI(q+68, -1); SYSTEM.PUTI(q+72, list2);
      list2 := q + 64; p := q
    END
  END GetBlock64;

  PROCEDURE GetBlock32(VAR p: INTEGER);
    VAR q: INTEGER;
  BEGIN
    IF list3 # 0 THEN p := list3; SYSTEM.GETI(list3+8, list3)
    ELSE GetBlock64(q); SYSTEM.PUTI(q+32, 32); SYSTEM.PUTI(q+36, -1); SYSTEM.PUTI(q+40, list3);
      list3 := q + 32; p := q
    END
  END GetBlock32;

  PROCEDURE New*(VAR ptr: INTEGER; tag: INTEGER);
    (*called by NEW via MT[0]; ptr and tag are pointers*)
    VAR p, size, lim: INTEGER;
  BEGIN SYSTEM.GETI(tag, size);
    IF size = 32 THEN GetBlock32(p)
    ELSIF size = 64 THEN GetBlock64(p)
    ELSIF size = 128 THEN GetBlock128(p)
    ELSE GetBlock(p, (size+255) DIV 256 * 256)
    END ;
    IF p = 0 THEN ptr := 0
    ELSE ptr := p+8; SYSTEM.PUTI(p, tag); lim := p + size; INC(p, 4); INC(allocated, size);
      WHILE p < lim DO SYSTEM.PUTI(p, 0); INC(p, 4) END
    END
  END New;

(* ---------- Garbage collector ----------*)

  PROCEDURE Mark*(pref: INTEGER);
    VAR pvadr, offadr, offset, tag, p, q, r: INTEGER;
  BEGIN SYSTEM.GETI(pref, pvadr); (*pointers < heapOrg considered NIL*)
    WHILE pvadr # 0 DO
      SYSTEM.GETI(pvadr, p); SYSTEM.GETI(p-4, offadr);
      IF (p >= heapOrg) & (offadr = 0) THEN q := p;   (*mark elements in data structure with root p*)
        REPEAT SYSTEM.GETI(p-4, offadr);
          IF offadr = 0 THEN SYSTEM.GETI(p-8, tag); offadr := tag + 16 ELSE INC(offadr, 4) END ;
          SYSTEM.PUTI(p-4, offadr); SYSTEM.GETI(offadr, offset);
          IF offset # -1 THEN (*down*)
            SYSTEM.GETI(p+offset, r); SYSTEM.GETI(r-4, offadr);
            IF (r >= heapOrg) & (offadr = 0) THEN SYSTEM.PUTI(p+offset, q); q := p; p := r END
          ELSE (*up*) SYSTEM.GETI(q-4, offadr); SYSTEM.GETI(offadr, offset);
            IF p # q THEN SYSTEM.GETI(q+offset, r); SYSTEM.PUTI(q+offset, p); p := q; q := r END
          END
        UNTIL (p = q) & (offset = -1)
      END ;
      INC(pref, 4); SYSTEM.GETI(pref, pvadr)
    END
  END Mark;

  PROCEDURE Scan*;
    VAR p, q, mark, tag, size: INTEGER;
  BEGIN p := heapOrg;
    REPEAT SYSTEM.GETI(p+4, mark); q := p;
      WHILE mark = 0 DO
        SYSTEM.GETI(p, tag); SYSTEM.GETI(tag, size); INC(p, size); SYSTEM.GETI(p+4, mark)
      END ;
      size := p - q; DEC(allocated, size);  (*size of free block*)
      IF size > 0 THEN
        IF size MOD 64 # 0 THEN
          SYSTEM.PUTI(q, 32); SYSTEM.PUTI(q+4, -1); SYSTEM.PUTI(q+8, list3); list3 := q; INC(q, 32); DEC(size, 32)
        END ;
        IF size MOD 128 # 0 THEN
          SYSTEM.PUTI(q, 64); SYSTEM.PUTI(q+4, -1); SYSTEM.PUTI(q+8, list2); list2 := q; INC(q, 64); DEC(size, 64)
        END ;
        IF size MOD 256 # 0 THEN
          SYSTEM.PUTI(q, 128); SYSTEM.PUTI(q+4, -1); SYSTEM.PUTI(q+8,  list1); list1 := q; INC(q, 128); DEC(size, 128)
        END ;
        IF size > 0 THEN
          SYSTEM.PUTI(q, size); SYSTEM.PUTI(q+4, -1); SYSTEM.PUTI(q+8, list0); list0 := q; INC(q, size)
        END
      END ;
      IF mark > 0 THEN SYSTEM.GETI(p, tag); SYSTEM.GETI(tag, size); SYSTEM.PUTI(p+4, 0); INC(p, size)
      ELSE (*free*) SYSTEM.GETI(p, size); INC(p, size)
      END
    UNTIL p >= heapLim
  END Scan;

(* ---------- Disk storage management ----------*)

  PROCEDURE InitSecMap*;
    VAR i: INTEGER;
  BEGIN NofSectors := 0; sectorMap[0] := {0 .. 31}; sectorMap[1] := {0 .. 31};
    FOR i := 2 TO mapsize DIV 32 - 1 DO sectorMap[i] := {} END
  END InitSecMap;

  PROCEDURE MarkSector*(sec: INTEGER);
  BEGIN sec := sec DIV 29; ASSERT(SYSTEM.H(0) = 0);
    INCL(sectorMap[sec DIV 32], sec MOD 32); INC(NofSectors)
  END MarkSector;

  PROCEDURE FreeSector*(sec: INTEGER);
  BEGIN sec := sec DIV 29; ASSERT(SYSTEM.H(0) = 0);
    EXCL(sectorMap[sec DIV 32], sec MOD 32); DEC(NofSectors)
  END FreeSector;

  PROCEDURE AllocSector*(hint: INTEGER; VAR sec: INTEGER);
    VAR s: INTEGER;
  BEGIN (*find free sector, starting after hint*)
    hint := hint DIV 29; ASSERT(SYSTEM.H(0) = 0); s := hint;
    REPEAT INC(s);
      IF s = mapsize THEN s := 1 END ;
    UNTIL ~(s MOD 32 IN sectorMap[s DIV 32]);
    INCL(sectorMap[s DIV 32], s MOD 32); INC(NofSectors); sec := s * 29
  END AllocSector;

  PROCEDURE GetSector*(src: INTEGER; VAR dst: Sector);
  BEGIN src := src DIV 29; 
    ASSERT(
      SYSTEM.H(0) = 0
      );
    src := src * 2 + FSoffset;
    OBX.readSector(src, ADR(dst) );
  END GetSector;
  
  PROCEDURE PutSector*(dst: INTEGER; VAR src: Sector);
  BEGIN dst := dst DIV 29; ASSERT(SYSTEM.H(0) =  0);
    dst := dst * 2 + FSoffset;
    OBX.writeSector(dst, ADR(src) );
  END PutSector;

(*-------- Miscellaneous procedures----------*)

  PROCEDURE Time*(): INTEGER;
    RETURN OBX.getTime()
  END Time;

  PROCEDURE Clock*(): INTEGER;
    RETURN clock
  END Clock;

  PROCEDURE SetClock*(dt: INTEGER);
  BEGIN clock := dt
  END SetClock;

  PROCEDURE Install*(Padr, at: INTEGER);
  BEGIN SYSTEM.PUTI(at, 0E7000000H + (Padr - at) DIV 4 -1)
  END Install;

  PROCEDURE Trap(VAR a: INTEGER; b: INTEGER);
    VAR u, v, w: INTEGER;
  BEGIN u := SYSTEM.REG(15); SYSTEM.GETI(u - 4, v); w := v DIV 10H MOD 10H; (*trap number*)
    IF w = 0 THEN New(a, b)
    ELSE (*stop*) LED(w + 192); REPEAT UNTIL FALSE
    END
  END Trap;

  PROCEDURE Init*;
  BEGIN // TODO Install(ADR(Trap), 20H);  (*install temporary trap*)
    SYSTEM.GETI(12, MemLim); SYSTEM.GETI(24, heapOrg);
    stackOrg := heapOrg; stackSize := 8000H; heapLim := MemLim;
    list1 := 0; list2 := 0; list3 := 0; list0 := heapOrg;
    SYSTEM.PUTI(list0, heapLim - heapOrg); SYSTEM.PUTI(list0+4, -1); SYSTEM.PUTI(list0+8, 0);
    allocated := 0; clock := 0; InitSecMap
  END Init;

END Kernel.
