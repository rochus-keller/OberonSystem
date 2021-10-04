MODULE Files;  (*NW 11.1.86 / 22.9.93 / 25.5.95 / 25.12.95 / 15.8.2013*)
  IMPORT SYSTEM, Kernel, FileDir, ObsFfi;

  (*A file consists of a sequence of pages. The first page
    contains the header. Part of the header is the page table, an array
    of disk addresses to the pages. A file is referenced through riders.
    A rider indicates a current position and refers to a file*)

  CONST MaxBufs    = 4;
      HS        = FileDir.HeaderSize;
      SS        = FileDir.SectorSize;
      STS       = FileDir.SecTabSize;
      XS        = FileDir.IndexSize;

  TYPE  DiskAdr = INTEGER;
      File*    = POINTER TO FileDesc;
      Buffer  = POINTER TO BufferRecord;
      Index   = POINTER TO IndexRecord;

    Rider* =
      RECORD eof*: BOOLEAN;
        res*: INTEGER;
        file: File;
        apos, bpos: INTEGER;
        buf: Buffer
      END ;

    FileDesc =
      RECORD next: File; (*list of files invisible to the GC*)
        nofbufs, aleng, bleng: INTEGER;
        modH, registered: BOOLEAN;
        firstbuf: Buffer;
        sechint: DiskAdr;
        name: FileDir.FileName;
        date: INTEGER;
        ext:  ARRAY FileDir.ExTabSize OF Index;
        sec: FileDir.SectorTable
      END ;

    BufferRecord =
      RECORD apos, lim: INTEGER;
        mod: BOOLEAN;
        next: Buffer;
        data: FileDir.DataSector
      END ;

    IndexRecord =
      RECORD adr: DiskAdr;
        mod: BOOLEAN;
        sec: FileDir.IndexSector
      END ;

    (*aleng * SS + bleng = length (including header)
      apos * SS + bpos = current position
      0 <= bpos <= lim <= SS
      0 <= apos <= aleng < PgTabSize
      (apos < aleng) & (lim = SS) OR (apos = aleng) *)


  PROCEDURE Old*(IN name: ARRAY OF CHAR): File;
	  VAR f: File; fb: INTEGER
  BEGIN
    fb := ObsFfi.openFile(name);
    IF fb < 0 THEN RETURN NIL END;
    NEW(f);
    f.name := name;
    f.sechint := fb;
    RETURN f
  END Old;

  PROCEDURE New*(IN name: ARRAY OF CHAR): File;
	  VAR f: File; fb: INTEGER
  BEGIN
    fb := ObsFfi.newFile();
    NEW(f);
    f.name := name;
    f.sechint := fb;
    RETURN f
  END New;

  PROCEDURE Register*(f: File);
	  VAR res: BOOLEAN;
  BEGIN
	  res := ObsFfi.saveFile(f.name, f.sechint);
	  Close(f)
  END Register;

  PROCEDURE Close*(f: File);
  BEGIN
    ObsFfi.freeFile(f.sechint);
    f.sechint := -1
  END Close;

  PROCEDURE Purge*(f: File);
  BEGIN
    Close(f)
  END Purge;

  PROCEDURE Delete*(IN name: ARRAY OF CHAR; VAR res: INTEGER);
  BEGIN
	  IF ObsFfi.removeFile(name) THEN res := 0 ELSE res := 1 END
  END Delete;

  PROCEDURE Rename*(IN old, new: ARRAY OF CHAR; VAR res: INTEGER);
  BEGIN
	  IF ObsFfi.renameFile(old, new) THEN res := 0 ELSE res := 1 END
  END Rename;

  PROCEDURE Length*(f: File): INTEGER;
    RETURN ObsFfi.length(f.sechint)
  END Length;

  PROCEDURE Date*(f: File): INTEGER;
  BEGIN RETURN f.date
  END Date;

  (*---------------------------Read---------------------------*)

  PROCEDURE Set*(VAR r: Rider; f: File; pos: INTEGER);
    VAR a, b: INTEGER;
  BEGIN  r.eof := FALSE; r.res := 0;
    IF f # NIL THEN
      r.file := f; r.apos := pos
    ELSE r.file:= NIL
    END
  END Set;

  PROCEDURE Pos*(VAR r: Rider): INTEGER;
  BEGIN RETURN r.apos
  END Pos;

  PROCEDURE Base*(VAR r: Rider): File;
  BEGIN RETURN r.file
  END Base;

  PROCEDURE ReadByte*(VAR r: Rider; VAR x: BYTE);
	  VAR pos: INTEGER;
  BEGIN
    x := 0;
    IF r.file # NIL THEN
      IF ObsFfi.setPos(r.file.sechint, r.apos) THEN 
	      r.res := 0; r.eof := FALSE;
	      IF ObsFfi.atEnd(r.file.sechint) THEN r.eof := TRUE; r.res := 1
	      ELSE
	        x := SHORT(SHORT(ObsFfi.readByte(r.file.sechint)))
	        pos := ObsFfi.getPos(r.file.sechint)
	        IF pos = r.apos THEN r.res := 1 ELSE r.apos := pos END
	      END
	   ELSE r.res := 1 
	   END
    END
  END ReadByte;

  PROCEDURE ReadBytes*(VAR r: Rider; VAR x: ARRAY OF BYTE; n: INTEGER);
    VAR i: INTEGER;
  BEGIN i := 0;  (*this implementation is to be improved*)
    WHILE i < n DO ReadByte(r, x[i]); INC(i) END
  END ReadBytes;

  PROCEDURE Read*(VAR r: Rider; VAR ch: CHAR);
  BEGIN
    ReadByte(r, ch)
  END Read;

  PROCEDURE ReadInt*(VAR R: Rider; VAR x: INTEGER);
    VAR x0, x1, x2, x3: BYTE;
  BEGIN ReadByte(R, x0); ReadByte(R, x1); ReadByte(R, x2); ReadByte(R, x3);
    x := ((x3 * 100H + x2) * 100H + x1) * 100H + x0
  END ReadInt;

  PROCEDURE ReadSet*(VAR R: Rider; VAR s: SET);
    VAR n: INTEGER;
  BEGIN ReadInt(R, n); s := VAL(SET,n)
  END ReadSet;

  PROCEDURE ReadReal*(VAR R: Rider; VAR x: REAL);
    VAR n: INTEGER;
  BEGIN ReadInt(R, n); x := n
  END ReadReal;

  PROCEDURE ReadString*(VAR R: Rider; VAR x: ARRAY OF CHAR);
    VAR i: INTEGER; ch: CHAR;
  BEGIN i := 0; Read(R, ch);
    WHILE ch # 0X DO
      IF i < LEN(x)-1 THEN x[i] := ch; INC(i) END ;
      Read(R, ch)
    END ;
    x[i] := 0X
  END ReadString;

  PROCEDURE ReadNum*(VAR R: Rider; VAR x: INTEGER);
    VAR n, y: INTEGER; b: BYTE;
  BEGIN n := 32; y := 0; ReadByte(R, b);
    WHILE b >= 80H DO y := ROR(y + b-80H, 7); DEC(n, 7); ReadByte(R, b) END ;
    IF n <= 4 THEN x := ROR(y + b MOD 10H, 4) ELSE x := ASR(ROR(y + b, 7), n-7) END
  END ReadNum;
        
  (*---------------------------Write---------------------------*)

  PROCEDURE NewExt(f: File);
    VAR i, k: INTEGER; ext: Index;
  BEGIN k := (f.aleng - STS) DIV XS;
    NEW(ext); ext.adr := 0; ext.mod := TRUE; f.ext[k] := ext; i := XS;
    REPEAT DEC(i); ext.sec[i] := 0 UNTIL i = 0
  END NewExt;

  PROCEDURE WriteByte*(VAR r: Rider; x: BYTE);
    VAR pos: INTEGER
  BEGIN
    IF r.file # NIL THEN
      IF ObsFfi.setPos(r.file.sechint, r.apos) THEN 
	      r.res := 0; r.eof := FALSE;
	      IF ObsFfi.writeByte(r.file.sechint,x) THEN
	        pos := ObsFfi.getPos(r.file.sechint);
	        IF pos = r.apos THEN r.res := r.res + 1 ELSE r.apos := pos END
	      ELSE r.res := r.res + 1
	      END
	   ELSE r.res := r.res + 1 
	   END
    END
  END WriteByte;

  PROCEDURE WriteBytes*(VAR r: Rider; IN x: ARRAY OF BYTE; n: INTEGER);
    VAR i: INTEGER;
  BEGIN i := 0; (*this implementation is to be improed*)
    WHILE i < n DO WriteByte(r, x[i]); INC(i) END
  END WriteBytes;

  PROCEDURE Write*(VAR r: Rider; ch: CHAR);
  BEGIN WriteByte(r,ch)
  END Write;

  PROCEDURE WriteInt*(VAR R: Rider; x: INTEGER);
  BEGIN WriteByte(R, x MOD 100H);
    WriteByte(R, x DIV 100H MOD 100H);
    WriteByte(R, x DIV 10000H MOD 100H);
    WriteByte(R, x DIV 1000000H MOD 100H)
  END WriteInt;

  PROCEDURE WriteSet*(VAR R: Rider; s: SET);
  BEGIN WriteInt(R, ORD(s))
  END WriteSet;

  PROCEDURE WriteReal*(VAR R: Rider; x: REAL);
  BEGIN  WriteInt(R, ORD(x))
  END WriteReal;

  PROCEDURE WriteString*(VAR R: Rider; IN x: ARRAY OF CHAR);
    VAR i: INTEGER; ch: CHAR;
  BEGIN i := 0;
    REPEAT ch := x[i]; Write(R, ch); INC(i) UNTIL ch = 0X
  END WriteString;

  PROCEDURE WriteNum*(VAR R: Rider; x: INTEGER);
  BEGIN
    WHILE (x < -40H) OR (x >= 40H) DO WriteByte(R, x MOD 80H + 80H); x := ASR(x, 7) END ;
    WriteByte(R, x MOD 80H)
  END WriteNum;

  (*---------------------------System use---------------------------*)

  PROCEDURE Init*;
  BEGIN Kernel.Init; FileDir.Init
  END Init;

  PROCEDURE RestoreList*; (*after mark phase of garbage collection*)
    // NOP
  END RestoreList;

END Files.
