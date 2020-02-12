DEFINITION FileDir;   (*NW 12.1.86 / 23.8.90 / 15.8.2013 / RK 25.1.20*)

	CONST FnLength    = 32;
		SectorSize   = 1024;
		HeaderSize  = 352;

	TYPE DiskAdr      = INTEGER;
	FileName       = ARRAY FnLength OF CHAR;
	EntryHandler   = PROCEDURE (name: FileName; sec: DiskAdr; VAR continue: BOOLEAN);

	FileHeader =
		RECORD (*first page of each file on disk*)
			aleng, bleng, date: INTEGER;
		END ;

	PROCEDURE Enumerate(prefix: ARRAY OF CHAR; proc: EntryHandler);

END FileDir.
