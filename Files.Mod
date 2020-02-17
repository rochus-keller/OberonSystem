DEFINITION Files;  (*NW 11.1.86 / 22.9.93 / 25.5.95 / 25.12.95 / 15.8.2013 / RK 25.1.20*)

	TYPE   
	File   = POINTER TO FileDesc;

	Rider =
		RECORD eof: BOOLEAN;
		res: INTEGER;
		END ;

	FileDesc = RECORD END ;

	PROCEDURE Old(name: ARRAY OF CHAR): File;
	PROCEDURE New(name: ARRAY OF CHAR): File;
	PROCEDURE Register(f: File);

	(*PROCEDURE Close(f: File);*)
	PROCEDURE Delete(name: ARRAY OF CHAR; VAR res: INTEGER);
	PROCEDURE Rename(old, new: ARRAY OF CHAR; VAR res: INTEGER);

	PROCEDURE Length(f: File): INTEGER;
	(*PROCEDURE Date(f: File): INTEGER;*)

	(*---------------------------Read---------------------------*)

	PROCEDURE Set(VAR r: Rider; f: File; pos: INTEGER);
	PROCEDURE Pos(VAR r: Rider): INTEGER;
	PROCEDURE Base(VAR r: Rider): File;

	PROCEDURE ReadByte(VAR r: Rider; VAR x: BYTE);
	(*PROCEDURE ReadBytes(VAR r: Rider; VAR x: ARRAY OF BYTE; n: INTEGER);*)
	PROCEDURE Read(VAR r: Rider; VAR ch: CHAR);
	PROCEDURE ReadInt(VAR R: Rider; VAR x: INTEGER);
	(*PROCEDURE ReadSet(VAR R: Rider; VAR s: SET);*)
	(*PROCEDURE ReadReal(VAR R: Rider; VAR x: REAL);*)
	PROCEDURE ReadString(VAR R: Rider; VAR x: ARRAY OF CHAR);
	PROCEDURE ReadNum(VAR R: Rider; VAR x: INTEGER);

	(*---------------------------Write---------------------------*)

	PROCEDURE WriteByte(VAR r: Rider; x: BYTE);
	(*PROCEDURE WriteBytes(VAR r: Rider; x: ARRAY OF BYTE; n: INTEGER);*)
	PROCEDURE Write(VAR r: Rider; ch: CHAR);
	PROCEDURE WriteInt(VAR R: Rider; x: INTEGER);
	(*PROCEDURE WriteSet(VAR R: Rider; s: SET);*)
	(*PROCEDURE WriteReal(VAR R: Rider; x: REAL);*)
	PROCEDURE WriteString(VAR R: Rider; x: ARRAY OF CHAR);
	PROCEDURE WriteNum(VAR R: Rider; x: INTEGER);

	PROCEDURE RestoreList; (*after mark phase of garbage collection*)

END Files.