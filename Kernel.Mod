DEFINITION Kernel;  (*NW/PR  11.4.86 / 27.12.95 / 4.2.2014 / RK 25.1.20 / 16.2.20 *)
	
	VAR allocated, NofSectors: INTEGER;
	heapOrg, heapLim: INTEGER; 
	stackOrg : INTEGER;


	(*PROCEDURE New(VAR ptr: INTEGER; tag: INTEGER);*)
	(*PROCEDURE Mark(pref: INTEGER);*)
	(*PROCEDURE Scan;*)

	PROCEDURE Time(): INTEGER;
	PROCEDURE Clock(): INTEGER;
	PROCEDURE SetClock(dt: INTEGER);

	(*PROCEDURE Install(Padr, at: INTEGER);*)

END Kernel.
