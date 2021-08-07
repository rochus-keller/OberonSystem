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

  PROCEDURE error(n: INTEGER; name: ARRAY OF CHAR);
  BEGIN res := n; importing := name
  END error;

  PROCEDURE Check(s: ARRAY OF CHAR);
    VAR i: INTEGER; ch: CHAR;
  BEGIN ch := s[0]; res := 1; i := 1;
    IF (ch >= "A") & (ch <= "Z") OR (ch >= "a") & (ch <= "z") THEN
      REPEAT ch := s[i]; INC(i)
      UNTIL ~((ch >= "0") & (ch <= "9") OR (ch >= "A") & (ch <= "Z")
        OR (ch >= "a") & (ch <= "z") OR (ch = ".")) OR (i = 32);
      IF (i < 32) & (ch = 0X) THEN res := 0 END
    END
  END Check;

  PROCEDURE Load*(IN name: ARRAY OF CHAR; VAR newmod: Module);
    (*search module in list; if not found, load module.
      res = 0: already present or loaded; res = 2: file not available; res = 3: key conflict;
      res = 4: bad file version; res = 5: corrupted file; res = 7: no space*)
    VAR mod: Module;
  BEGIN mod := root; res := 0; 
    WHILE (mod # NIL) & (name # mod.name) DO mod := mod.next END ;
    IF mod = NIL THEN (*load*)
      Check(name);
      IF res = 0 THEN 
	     error(1, name)
      ELSE 
	      // TODO load module
	      NEW(mod);
	      mod.name := name;
	      mod.next := root;
	      root := mod
      END ;
    END ;
    newmod :=  mod
  END Load;

  PROCEDURE ThisCommand*(mod: Module; name: ARRAY OF CHAR): Command;
	  // TODO
    RETURN NIL
  END ThisCommand;

  PROCEDURE Free*(name: ARRAY OF CHAR);
    VAR mod: Module; 
  BEGIN mod := root; res := 0;
    WHILE (mod # NIL) & (mod.name # name) DO mod := mod.next END ;
    IF mod # NIL THEN
      IF mod.refcnt = 0 THEN
        mod.name[0] := 0X; 
      ELSE res := 1
      END
    END
  END Free;

  PROCEDURE Init*;
  BEGIN Files.Init; 
	  AllocPtr := 0
  END Init;

BEGIN Init; Load("Oberon", M);
    LED(res); REPEAT UNTIL FALSE  (*only if load fails*)
END Modules.
