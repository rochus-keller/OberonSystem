MODULE Input; (*NW 5.10.86 / 15.11.90 Ceres-2; PDR 21.4.12 / NW 15.5.2013 Ceres-4*)
  IMPORT SYSTEM;

  CONST msAdr = -40; kbdAdr = -36;
  VAR kbdCode: BYTE; (*last keyboard code read*)
    Recd, Up, Shift, Ctrl, Ext: BOOLEAN;
    KTabAdr: ARRAY 256 OF BYTE;  (*keyboard code translation table*)
    MW, MH, MX, MY: INTEGER; (*mouse limits and coords*)
    MK: SET; (*mouse keys*)
  
  PROCEDURE Available*(): INTEGER;
  BEGIN 
    RETURN 0 // 1 if real key (i.e. no SHIFT, CTRL etc.) is pressed or released
  END Available;

  PROCEDURE Read*(VAR ch: CHAR);
  BEGIN
    // WHILE ~Recd DO Peek() END ; // wait for real key
    IF Shift OR Ctrl THEN INC(kbdCode, 80H) END; (*ctrl implies shift*)
    ch := CHR(KTabAdr[kbdCode]);
    IF Ctrl THEN ch := CHR(ORD(ch) MOD 20H) END;
    Recd := FALSE
  END Read;

  PROCEDURE Mouse*(VAR keys: SET; VAR x, y: INTEGER);
    VAR w: INTEGER;
  BEGIN // SYSTEM.GETI(msAdr, w);
    keys := VAL(SET, w DIV 1000000H MOD 8);
    x := w MOD 400H; y := (w DIV 1000H) MOD 400H;
    IF y >= MH THEN y := MH-1 END
  END Mouse;

  PROCEDURE SetMouseLimits*(w, h: INTEGER);
  BEGIN MW := w; MH := h
  END SetMouseLimits;

  PROCEDURE Init*;
  BEGIN Up := FALSE; Shift := FALSE; Ctrl := FALSE; Recd := FALSE;
  END Init;

BEGIN Init
END Input.
