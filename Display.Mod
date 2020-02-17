DEFINITION Display;  (*NW 5.11.2013 / 3.7.2016 / RK 25.1.20*)

  CONST black = 0; white = 1;  (*black = background*)
    replace = 0; paint = 1; invert = 2;  (*modes*)
    PatternLen = 15 * 15 DIV 8 + 2;

  TYPE Frame = POINTER TO FrameDesc;
    FrameMsg = RECORD END ;
    Handler = PROCEDURE (F: Frame; VAR M: FrameMsg);
    FrameDesc = 
	    RECORD 
		    next, dsc: Frame;
		        X, Y, W, H: INTEGER;
		        handle: Handler
	      END ;
	Pattern = ARRAY PatternLen OF BYTE;
    (*a pattern is an array of bytes; the first is its width (< 32), the second its height, the rest the raster*)

  VAR Width, Height: INTEGER;
    arrow, star, hook, updown, block: Pattern;

  PROCEDURE ReplConst(color, x, y, w, h, mode: INTEGER);
  (*PROCEDURE ReplPattern(col, patadr, x, y, w, h, mode: INTEGER);*)
  PROCEDURE CopyPattern(color: INTEGER; patadr: ARRAY OF BYTE; x, y, mode: INTEGER);  (*only for modes = paint, invert*)
  PROCEDURE CopyBlock(sx, sy, w, h, dx, dy, mode: INTEGER); (*only for mode = replace*)
  PROCEDURE Dot(col, x, y, mode: INTEGER);

(*Width := 1024; Height := 768;
  arrow := SYSTEM.ADR($0F0F 0060 0070 0038 001C 000E 0007 8003 C101 E300 7700 3F00 1F00 3F00 7F00 FF00$);
  star := SYSTEM.ADR($0F0F 8000 8220 8410 8808 9004 A002 C001 7F7F C001 A002 9004 8808 8410 8220 8000$);
  hook := SYSTEM.ADR($0C0C 070F 8707 C703 E701 F700 7F00 3F00 1F00 0F00 0700 0300 01$);
  updown := SYSTEM.ADR($080E 183C 7EFF 1818 1818 1818 FF7E3C18$);
  block := SYSTEM.ADR($0808 FFFF C3C3 C3C3 FFFF$);
  *)
END Display.
