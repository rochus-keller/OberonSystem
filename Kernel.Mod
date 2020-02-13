DEFINITION Kernel;  (*NW/PR  11.4.86 / 27.12.95 / 4.2.2014 / RK 25.1.20*)

	CONST SectorLength = 1024;

	TYPE Sector = ARRAY SectorLength OF BYTE;

	VAR allocated, NofSectors: INTEGER;
	heapOrg, heapLim: INTEGER; 
	stackOrg : INTEGER;


	(*PROCEDURE New(VAR ptr: LONGINT; tag: LONGINT);*)
	PROCEDURE Mark(pref: LONGINT);
	PROCEDURE Scan;

	PROCEDURE GetSector(src: INTEGER; VAR dst: Sector);

	PROCEDURE Time(): INTEGER;
	PROCEDURE Clock(): INTEGER;
	PROCEDURE SetClock(dt: INTEGER);

	(*PROCEDURE Install(Padr, at: INTEGER);*)

END Kernel.
