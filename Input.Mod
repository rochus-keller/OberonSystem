DEFINITION Input; (*NW 5.10.86 / 15.11.90 Ceres-2; PDR 21.4.12 / NW 15.5.2013 Ceres-4 / RK 25.1.20*)

  PROCEDURE Available(): INTEGER;

  PROCEDURE Read(VAR ch: CHAR);

  PROCEDURE Mouse(VAR keys: SET; VAR x, y: INTEGER);

  PROCEDURE SetMouseLimits(w, h: INTEGER);

END Input.
