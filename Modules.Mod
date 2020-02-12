DEFINITION Modules;  (*Link and load on RISC; NW 20.10.2013 / 9.4.2016 / RK 25.1.20*)

	TYPE 
	Module = POINTER TO ModDesc;
	Command = PROCEDURE;
	ModuleName = ARRAY 32 OF CHAR;

	ModDesc = 
		RECORD
			name: ModuleName;
			next: Module;
			key, num, size, refcnt: INTEGER;
			code, imp, cmd, ent, ptr: INTEGER  (*addresses*)
		END ;

	VAR 
	root: Module;
	AllocPtr, res: INTEGER;
	importing, imported: ModuleName;


	PROCEDURE Load(name: ARRAY OF CHAR; VAR newmod: Module);

	PROCEDURE ThisCommand(mod: Module; name: ARRAY OF CHAR): Command;

	PROCEDURE Free(name: ARRAY OF CHAR);

	(* BEGIN 
	Load("Oberon", M);
	LED(res); REPEAT UNTIL FALSE  (*only if load fails*)
	*)
END Modules.
