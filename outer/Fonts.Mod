MODULE Fonts; (*JG 18.11.90; PDR 8.6.12; NW 25.3.2013*)
  IMPORT SYSTEM, Files;

  CONST FontFileId = 0DBH;

  TYPE Font* = POINTER TO FontDesc;
    FontDesc* = RECORD
      name*: ARRAY 32 OF CHAR;
      height*, minX*, maxX*, minY*, maxY*: LONGINT;
      next*: Font;
      T: ARRAY 128 OF LONGINT;
      raster: ARRAY 2360 OF SYSTEM.BYTE
    END ;

    LargeFontDesc = RECORD (FontDesc) ext: ARRAY 2560 OF SYSTEM.BYTE END ;
    LargeFont = POINTER TO LargeFontDesc;

  (* raster sizes: Syntax8 1367, Syntax10 1628, Syntax12 1688, Syntax14 1843, Syntax14b 1983,
      Syntax16 2271, Syntax20 3034, Syntac24 4274, Syntax24b 4302  *)

VAR Default*, root*: Font;

PROCEDURE GetPat*(fnt: Font; ch: CHAR; VAR dx, x, y, w, h, patadr: LONGINT);
  VAR pa: LONGINT;  dxb, xb, yb, wb, hb: SYSTEM.BYTE;
BEGIN pa := fnt.T[ORD(ch) MOD 80H]; patadr := pa;
  SYSTEM.GET(pa-3, dxb); SYSTEM.GET(pa-2, xb); SYSTEM.GET(pa-1, yb); SYSTEM.GET(pa, wb); SYSTEM.GET(pa+1, hb);
  dx := ORD(dxb); x := ORD(xb); y := ORD(yb); w := ORD(wb); h := ORD(hb);
  IF ORD(yb) < 128 THEN y := ORD(yb) ELSE y := ORD(yb) - 256 END
END GetPat;

PROCEDURE This*(name: ARRAY OF CHAR): Font;

  TYPE RunRec = RECORD beg, end: SYSTEM.BYTE END ;
    BoxRec = RECORD dx, x, y, w, h: SYSTEM.BYTE END ;
    
  VAR F: Font; LF: LargeFont;
    f: Files.File; R: Files.Rider;
    NofRuns, NofBoxes: SYSTEM.BYTE;
    NofBytes: LONGINT;
    height, minX, maxX, minY, maxY: SYSTEM.BYTE;
    i, j, k, m, n: LONGINT;
    a, a0: LONGINT;
    b, beg, end: SYSTEM.BYTE;
    run: ARRAY 16 OF RunRec;
    box: ARRAY 512 OF BoxRec;

  PROCEDURE RdInt16(VAR R: Files.Rider; VAR b0: SYSTEM.BYTE);
    VAR b1: SYSTEM.BYTE;
  BEGIN Files.ReadByte(R, b0); Files.ReadByte(R, b1)
  END RdInt16;

BEGIN F := root;
  WHILE (F # NIL) & (name # F.name) DO F := F.next END;
  IF F = NIL THEN
    f := Files.Old(name);
    IF f # NIL THEN
      Files.Set(R, f, 0); Files.ReadByte(R, b);
      IF ORD(b) = FontFileId THEN
        Files.ReadByte(R, b); (*abstraction*)
        Files.ReadByte(R, b); (*family*)
        Files.ReadByte(R, b); (*variant*)
        NEW(F); COPY(name, F.name);
        RdInt16(R, height); RdInt16(R, minX); RdInt16(R, maxX); RdInt16(R, minY); RdInt16(R, maxY); RdInt16(R, NofRuns);
        NofBoxes := 0; k := 0;
        WHILE k # ORD(NofRuns) DO
          RdInt16(R, beg);
          run[k].beg := beg; RdInt16(R, end);
          run[k].end := end; NofBoxes := CHR(ORD(NofBoxes) + ORD(end) - ORD(beg)); INC(k)
        END;
        NofBytes := 5; j := 0;
        WHILE j # ORD(NofBoxes) DO
          RdInt16(R, box[j].dx); RdInt16(R, box[j].x); RdInt16(R, box[j].y);
          RdInt16(R, box[j].w); RdInt16(R, box[j].h);
          NofBytes := NofBytes + 5 + (ORD(box[j].w) + 7) DIV 8 * ORD(box[j].h);
          INC(j)
        END;
        IF NofBytes < 2300 THEN NEW(F) ELSE NEW(LF); F := LF END ;
        COPY(name, F.name);
        F.height := ORD(height); F.minX := ORD(minX); F.maxX := ORD(maxX); F.maxY := ORD(maxY);
        IF ORD(minY) >= 80H THEN F.minY := ORD(minY) - 100H ELSE F.minY := ORD(minY) END ;
        a0 := SYSTEM.ADR(F.raster);
        SYSTEM.PUT(a0, 0X); SYSTEM.PUT(a0+1, 0X); SYSTEM.PUT(a0+2, 0X); SYSTEM.PUT(a0+3, 0X); SYSTEM.PUT(a0+4, 0X);
        (*null pattern for characters not in a run*)
        INC(a0, 2); a := a0+3; j := 0; k := 0; m := 0;
        WHILE k < ORD(NofRuns) DO
          WHILE (m < ORD(run[k].beg)) & (m < 128) DO F.T[m] := a0; INC(m) END;
          WHILE (m < ORD(run[k].end)) & (m < 128) DO
            F.T[m] := a+3;
            SYSTEM.PUT(a, box[j].dx); SYSTEM.PUT(a+1, box[j].x); SYSTEM.PUT(a+2, box[j].y);
            SYSTEM.PUT(a+3, box[j].w); SYSTEM.PUT(a+4, box[j].h); INC(a, 5);
            n := (ORD(box[j].w) + 7) DIV 8 * ORD(box[j].h);
            WHILE n # 0 DO DEC(n); Files.ReadByte(R, b); SYSTEM.PUT(a, b); INC(a) END ;
            INC(j); INC(m)
          END;
          INC(k)
        END;
        WHILE m < 128 DO F.T[m] := a0; INC(m) END ;
        F.next := root; root := F
      ELSE (*bad file id*) F := Default
      END
    ELSE (*font file not available*) F := Default
    END
  END;
  RETURN F
END This;

PROCEDURE Free*;  (*remove all but first two from font list*)
  VAR f: Font;
BEGIN f := root.next;
  IF f # NIL THEN f := f.next END ;
  f.next := NIL
END Free;

BEGIN root := NIL; Default := This("Oberon10.Scn.Fnt")
END Fonts.
