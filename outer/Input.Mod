MODULE Input; (*NW 5.10.86 / 15.11.90 Ceres-2; PDR 21.4.12 / NW 15.5.2013 Ceres-4*)
  IMPORT SYSTEM, SYS;

  CONST msAdr = -40; kbdAdr = -36;
  VAR kbdCode: SYSTEM.BYTE; (*last keyboard code read*)
    Recd, Up, Shift, Ctrl, Ext: BOOLEAN;
    KTabAdr: LONGINT;  (*keyboard code translation table*)
    MW, MH, MX, MY: LONGINT; (*mouse limits and coords*)
    MK: SET; (*mouse keys*)
    KTab: ARRAY 256 OF SYSTEM.BYTE;

(*FIFO implemented in hardware, because every read must be handled,
  including tracking the state of the Shift and Ctrl keys*)
  
  PROCEDURE Peek();
  BEGIN
    IF SYSTEM.BIT(msAdr, 28) THEN
      SYSTEM.GET(kbdAdr, kbdCode);
      IF ORD(kbdCode) = 0F0H THEN Up := TRUE
      ELSIF ORD(kbdCode) = 0E0H THEN Ext := TRUE
      ELSE
        IF (ORD(kbdCode) = 12H) OR (ORD(kbdCode) = 59H) THEN (*shift*) Shift := ~Up
        ELSIF ORD(kbdCode) = 14H THEN (*ctrl*) Ctrl := ~Up
        ELSIF ~Up THEN Recd := TRUE (*real key going down*)
        END ;
        Up := FALSE; Ext := FALSE
      END
    END;
  END Peek;

  PROCEDURE Available*(): LONGINT;
    VAR n: LONGINT;
  BEGIN Peek();
    IF Recd THEN n := 1 ELSE n := 0 END ;
    RETURN n
  END Available;

  PROCEDURE Read*(VAR ch: CHAR);
  BEGIN
    WHILE ~Recd DO Peek() END ;
    IF Shift OR Ctrl THEN kbdCode := CHR(ORD(kbdCode) + 80H) END; (*ctrl implies shift*)
  (* ch := kbdTab[kbdCode]; *)
    SYSTEM.GET(KTabAdr + ORD(kbdCode), ch);
    IF Ctrl THEN ch := CHR(ORD(ch) MOD 20H) END;
    Recd := FALSE
  END Read;

  PROCEDURE Mouse*(VAR keys: SET; VAR x, y: LONGINT);
    VAR w: LONGINT;
  BEGIN SYSTEM.GET(msAdr, w);
    keys := SYSTEM.VAL(SET, w DIV 1000000H MOD 8);
    x := w MOD 400H; y := (w DIV 1000H) MOD 400H;
    IF y >= MH THEN y := MH-1 END
  END Mouse;

  PROCEDURE SetMouseLimits*(w, h: LONGINT);
  BEGIN MW := w; MH := h
  END SetMouseLimits;

  PROCEDURE Init*;
  BEGIN Up := FALSE; Shift := FALSE; Ctrl := FALSE; Recd := FALSE;
    SYS.PutHex(KTab, 0,   "00 00 00 00 00 1A 00 00  00 00 00 00 00 09 60 00");
    SYS.PutHex(KTab, 16,  "00 00 00 00 00 71 31 00  00 00 7A 73 61 77 32 00");
    SYS.PutHex(KTab, 32,  "00 63 78 64 65 34 33 00  00 20 76 66 74 72 35 00");
    SYS.PutHex(KTab, 48,  "00 6E 62 68 67 79 36 00  00 00 6D 6A 75 37 38 00");
    SYS.PutHex(KTab, 64,  "00 2C 6B 69 6F 30 39 00  00 2E 2F 6C 3B 70 2D 00");
    SYS.PutHex(KTab, 80,  "00 00 27 00 5B 3D 00 00  00 00 0D 5D 00 5C 00 00");
    SYS.PutHex(KTab, 96,  "00 00 00 00 00 00 08 00  00 00 00 00 00 00 00 00");
    SYS.PutHex(KTab, 112, "00 7F 00 00 00 00 1B 00  00 00 00 00 00 00 00 00");
    SYS.PutHex(KTab, 128, "00 00 00 00 00 00 00 00  00 00 00 00 00 09 7E 00");
    SYS.PutHex(KTab, 144, "00 00 00 00 00 51 21 00  00 00 5A 53 41 57 40 00");
    SYS.PutHex(KTab, 160, "00 43 58 44 45 24 23 00  00 20 56 46 54 52 25 00");
    SYS.PutHex(KTab, 176, "00 4E 42 48 47 59 5E 00  00 00 4D 4A 55 26 2A 00");
    SYS.PutHex(KTab, 192, "00 3C 4B 49 4F 29 28 00  00 3E 3F 4C 3A 50 5F 00");
    SYS.PutHex(KTab, 208, "00 00 22 00 7B 2B 00 00  00 00 0D 7D 00 7C 00 00");
    SYS.PutHex(KTab, 224, "00 00 00 00 00 00 08 00  00 00 00 00 00 00 00 00");
    SYS.PutHex(KTab, 240, "00 7F 00 00 00 00 1B 00  00 00 00 00 00 00 00 00");
    KTabAdr := SYSTEM.ADR(KTab)
  END Init;

BEGIN Init
END Input.
