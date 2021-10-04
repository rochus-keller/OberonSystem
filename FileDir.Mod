MODULE FileDir;   (*NW 12.1.86 / 23.8.90 / 15.8.2013*)
  IMPORT SYSTEM, Kernel, ObsFfi;

  (*File Directory is a B-tree with its root page at DirRootAdr.
    Each entry contains a file name and the disk address of the file's head sector*)

  CONST FnLength*    = 32;
        SecTabSize*   = 64;
        ExTabSize*   = 12;
        SectorSize*   = 1024;
        IndexSize*   = SectorSize DIV 4;
        HeaderSize*  = 352;
        DirRootAdr*  = 29;
        DirPgSize*   = 24;
        N = DirPgSize DIV 2;
        DirMark*    = 9B1EA38DH;
        HeaderMark* = 9BA71D86H;
        FillerSize = 52;

  TYPE DiskAdr      = INTEGER;
    FileName*       = ARRAY FnLength OF CHAR;
    SectorTable*    = ARRAY SecTabSize OF DiskAdr;
    ExtensionTable* = ARRAY ExTabSize OF DiskAdr;
    EntryHandler*   = PROCEDURE (IN name: ARRAY OF CHAR; sec: DiskAdr; VAR continue: BOOLEAN);

    FileHeader* =
      RECORD (*first page of each file on disk*)
        mark*: INTEGER;
        name*: FileName;
        aleng*, bleng*, date*: INTEGER;
        ext*:  ExtensionTable;
        sec*: SectorTable;
        fill: ARRAY SectorSize - HeaderSize OF BYTE;
      END ;

    FileHd* = POINTER TO FileHeader;
    IndexSector* = ARRAY IndexSize OF DiskAdr;
    DataSector* = ARRAY SectorSize OF BYTE;

    DirEntry* =  (*B-tree node*)
      RECORD
        name*: FileName;
        adr*:  DiskAdr; (*sec no of file header*)
        p*:    DiskAdr  (*sec no of descendant in directory*)
      END ;

    DirPage*  =
      RECORD mark*:  INTEGER;
        m*:     INTEGER;
        p0*:    DiskAdr;  (*sec no of left descendant in directory*)
        fill:  ARRAY FillerSize OF BYTE;
        e*:  ARRAY DirPgSize OF DirEntry
      END ;

  (*Exported procedures: Search, Insert, Delete, Enumerate, Init*)

  PROCEDURE Search*(name: FileName; VAR A: DiskAdr);
	// NOP
  END Search;
  
  PROCEDURE Insert*(name: FileName; fad: DiskAdr);
	  // NOP
  END Insert;

  PROCEDURE Delete*(name: FileName; VAR fad: DiskAdr);
	  // NOP
  END Delete;

  PROCEDURE Enumerate*(IN prefix: ARRAY OF CHAR; proc: EntryHandler);
    VAR n,i: INTEGER;
	    b: BOOLEAN;
  BEGIN
		n := ObsFfi.listFiles();
		FOR i := 0 TO n-1 DO
		  proc(ObsFfi.fileName(i),0,b);
		END
  END Enumerate;

(* ----- initialization ----- *)

  PROCEDURE Init*;
	 // NOP
  END Init; 
  
END FileDir.
