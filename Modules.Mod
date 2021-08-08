MODULE Modules;  (*Link and load on RISC; NW 20.10.2013 / 9.4.2016*)
  IMPORT SYSTEM, Files;
  CONST versionkey = 1X; MT = 12; DescSize = 80;

  TYPE Module* = POINTER TO ModDesc;
    Command* = PROCEDURE;
    ModuleName* = ARRAY 32 OF CHAR;

    ModDesc* = RECORD
        name*: ModuleName;
        next*: Module;
        key*, num*, size*, refcnt*: INTEGER;
        data*, code*, imp*, cmd*, ent*, ptr*, unused: INTEGER  (*addresses*)
      END ;

  VAR root*, M: Module;
    AllocPtr*, res*: INTEGER;
    importing*, imported*: ModuleName;

  PROCEDURE error(n: INTEGER; IN name: ARRAY OF CHAR);
  BEGIN res := n; importing := name
  END error;
  
  PROCEDURE Find(IN name: ARRAY OF CHAR): Module;
	  VAR cur: Module;
  BEGIN
	  cur := root;
	  WHILE cur # NIL DO
	    IF cur.name = name THEN RETURN cur END;
	    cur := cur.next
	  END;
	  RETURN NIL
  END Find;

  PROCEDURE Load*(IN name: ARRAY OF CHAR; VAR newmod: Module);
    (*search module in list; if not found, load module.
      res = 0: already present or loaded; res = 2: file not available; res = 3: key conflict;
      res = 4: bad file version; res = 5: corrupted file; res = 7: no space*)
    VAR mod: Module;
  BEGIN res := 0; 
    importing := name;
    mod := Find(name);
    IF mod = NIL THEN  
      IF ~LDMOD(name) THEN res := 2 
      ELSE imported := name;
        NEW(mod);
        mod.name := name;
        mod.next := root;
        root := mod;
      END
    END
    newmod := mod;
  END Load;

  PROCEDURE ThisCommand*(mod: Module; IN name: ARRAY OF CHAR): Command;
	 VAR p: PROCEDURE;
  BEGIN
    RETURN LDCMD(mod.name,name)
  END ThisCommand;

  PROCEDURE Free*(IN name: ARRAY OF CHAR);
  BEGIN res := 0;
    // NOP
  END Free;

  PROCEDURE Init*;
  BEGIN Files.Init; 
	  AllocPtr := 0
  END Init;

BEGIN Init; // Load("Oberon", M);
    LED(res); // REPEAT UNTIL FALSE  (*only if load fails*)
END Modules.
