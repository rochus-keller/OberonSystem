This is a version of the [Project Oberon System](https://projectoberon.net/) migrated
from Oberon 07 to the more common Oberon 90, using a recent version of the
[OP2 compiler](https://github.com/rochus-keller/op2/) with a Risc-V (RV32) backend; 
the repository also includes an emulation of a machine (VM) very similar to the one described by Wirth in his
[Project Oberon Book](http://www.inf.ethz.ch/personal/wirth/ProjectOberon/PO.Computer.pdf), 
based on the well-known [RV32 emulator](https://github.com/sysprog21/rv32emu);
the latter is a stripped-down and slightly modified version with only the interpreter,
combined with my own implementation of the machine and the peripherals; 
the memory map of Wirth's machine is reproduced 1:1 so that Kernel.Mod, Display.Mod and 
Input.Mod are unchanged.

Here is a screenshot of the system running natively on the Risc-V VM:

![Project Oberon System Screenshot](http://software.rochus-keller.ch/project_oberon_system_rv32.png)

### Migration Details

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

Migrated all Oberon-07 sources to Oberon 90 so that they compile with the 
ActiveOberon project o2c compiler and OP2. Some notes:

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
no dynamic loading.

Added additional apps, see readme in corresponding subdirectory. 

### Precompiled versions

So far, the following version is available

- [Linux x64](http://software.rochus-keller.ch/rv32_oberonsystem_linux_x64.tar.gz)

Note that the included po.bin and disk.img files work on all platforms. Only the rv32vm executable is
platform dependent. If you therefore just want to build the vm on another platform, you can reuse the other files.

### How to build

There is a build.sh in the vm subdirectory which has to be run first. Then the build.sh in the root can be executed like

- `./build.sh run` to build, pack and run the system in the VM
- `./build.sh disk` to build and pack the system without running it
- `./build.sh link` to build and link the system without creating the disk nor running it
- `./build.sh` just compile all Oberon modules and stop

The build scripts were implemented and tested on Debian Bookworm. There is also a vm.pro qmake project which is likely
to work on Windows and macOS as well, but it has only been tested on Linux so far.
If there is demand I can also provide a BUSY file for the VM.

### Credits

- See oberon_license.txt which applies to the Oberon source code.
- See vm/rv32emu/README_orig.md and vm/rv32emu/LICENSE for more information about the Risc-V emulator.
- See vm/softfloat/README_orig.md and vm/softfloat/COPYING.txt for more information about the Berkeley SoftFloat library.
- The machine in the vm subdirectory is available under the terms of the GNU General Public License (GPL) versions 2.0 or 3.0 as published by the Free Software Foundation.

