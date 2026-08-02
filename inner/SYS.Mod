MODULE SYS;  (*Oberon-07 built-in procedures which have no counterpart in Oberon-90*)
  IMPORT SYSTEM;

  CONST LEDadr = -60;

  PROCEDURE ASSERT*(cond: BOOLEAN);
  BEGIN
    IF ~cond THEN HALT(20) END
  END ASSERT;

  PROCEDURE LED*(n: LONGINT);
  BEGIN SYSTEM.PUT(LEDadr, n)
  END LED;

  PROCEDURE H*(n: LONGINT): LONGINT;  (*auxiliary register H of the RISC processor*)
    VAR res: LONGINT;
  BEGIN res := 0;
    RETURN res
  END H;

  PROCEDURE REG*(n: LONGINT): LONGINT;  (*value of register n of the RISC processor*)
    VAR res: LONGINT;
  BEGIN res := 0;
    RETURN res
  END REG;

  PROCEDURE LDREG*(n, val: LONGINT);  (*load register n of the RISC processor*)
  END LDREG;

  PROCEDURE PutHex*(VAR dst: ARRAY OF SYSTEM.BYTE; pos: LONGINT; hex: ARRAY OF CHAR);
    (*replaces the hexadecimal string literal $..$ of Oberon-07;
      hex holds pairs of hexadecimal digits, blanks are separators*)
    VAR i, d, b, n: LONGINT; ch: CHAR;
  BEGIN i := 0; n := 0; b := 0;
    WHILE hex[i] # 0X DO ch := hex[i];
      IF ch # " " THEN
        IF ch <= "9" THEN d := ORD(ch) - ORD("0")
        ELSIF ch <= "F" THEN d := ORD(ch) - ORD("A") + 10
        ELSE d := ORD(ch) - ORD("a") + 10
        END ;
        b := b*10H + d; INC(n);
        IF ~ODD(n) THEN dst[pos] := CHR(b); INC(pos); b := 0 END
      END ;
      INC(i)
    END
  END PutHex;

END SYS.
