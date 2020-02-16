DEFINITION FileDir;   (*NW 12.1.86 / 23.8.90 / 15.8.2013 / RK 25.1.20 / 16.2.20 *)

	CONST FnLength    = 32;

	TYPE 
	FileName       = ARRAY FnLength OF CHAR;

	FileHeader =
		RECORD (*first page of each file on disk*)
			leng, date: INTEGER;
		END ;
		
	EntryHandler   = PROCEDURE (name: FileName; fh: FileHeader; VAR continue: BOOLEAN);

	PROCEDURE Enumerate(prefix: ARRAY OF CHAR; proc: EntryHandler);

END FileDir.
