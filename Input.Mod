DEFINITION Input; (*NW 5.10.86 / 15.11.90 Ceres-2; PDR 21.4.12 / NW 15.5.2013 Ceres-4 / RK 25.1.20*)

	TYPE
		MouseHandler = PROCEDURE( keys: SET; x, y: INTEGER );
		CharHandler = PROCEDURE( ch: CHAR );
		IdleHandler = PROCEDURE;

	PROCEDURE RegisterMouseHandler( h : MouseHandler ); (* called whenever the mouse pos or state changes *)
	PROCEDURE RegisterCharHandler( h: CharHandler ); (* called whenever an ASCII char was hit *)
	PROCEDURE RegisterIdleHandler(h: IdleHandler); (* called whenever no window events are pending *)
	
	(* obsolete, use MouseHandler and CharHandler instead *)
	PROCEDURE Available(): INTEGER; 
	PROCEDURE Read(VAR ch: CHAR);

	(* last known mouse position and button state *)
	PROCEDURE Mouse(VAR keys: SET; VAR x, y: INTEGER); 

	PROCEDURE SetMouseLimits(w, h: INTEGER);

END Input.
