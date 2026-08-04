This is a version of the [Project Oberon System](https://projectoberon.net/) migrated
from Oberon 07 to the more common Oberon 90, using a recent version of 
[the OP2 compiler](https://github.com/rochus-keller/op2/) with its Risc-V (RV32) backend,
running on a machine very similar to the one described by Wirth in his
[Project Oberon Book](http://www.inf.ethz.ch/personal/wirth/ProjectOberon/PO.Computer.pdf), 
but based on the well-known [RV32 emulator](https://github.com/sysprog21/rv32emu). 
The latter is a stripped-down and slightly modified version with only the interpreter,
combined with my own implementation of the machine and the peripherals; 
the emory map of Wirth's machine is reproduced 1:1 so that Kernel.Mod, Display.Mod and 
Input.Mod are unchanged

The original version of the Project Oberon source code was downloaded from 
https://www.projectoberon.net/ on 2026-04-14.

I particularly downloaded the following archives:
- http://www.projectoberon.net/zip/inner.zip
- http://www.projectoberon.net/zip/outer.zip
- http://www.projectoberon.net/zip/systools.zip
- http://www.projectoberon.net/zip/graph.zip
- http://www.projectoberon.net/zip/apptools.zip

The latest file modification date is 2018-11-28. Each subdirectory of this repository
corresponds to the archive of the same name, besides apptools and systools which
have been merged in files.

See also oberon_license.txt which applies to the Oberon source code.

Migrated all Oberon-07 sources to Oberon 90 so that they compile with the 
ActiveOberon project o2c compiler and OP2.

Migration notes:
- INTEGER renamed to LONGINT throughout
- SYS.Mod provides the Oberon 07 built-ins not present in Oberon 90 
- byte-sized data stays SYSTEM.BYTE
- type case statements are expressed as IF with IS relation and type guards
- array assignments that Oberon 90 rejects use COPY
- Oberon 07 byte-string literals ($..$) are initialized at runtime via SYS.PutHex
- ORD(SYSTEM.BYTE) is signed in OP2, so we use CHAR instead where necessary
- SYSTEM.BIT with variable instead of constant address because of OP2 issue

Implemented a Risc-V machine based on rv32emu similar to Wirth's RISC-5 described in PO book
and made the necessary (minimal) changes to the Oberon code to run it.

All modules are linked into the boot image by the boot linker and their bodies have been executed at startup,
no dynamic loading. Instead 
