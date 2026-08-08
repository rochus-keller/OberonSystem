This is a version of the [Project Oberon System](https://projectoberon.net/) migrated
from Oberon 07 to the more common Oberon 90, using a recent version of the
[OP2 compiler](https://github.com/rochus-keller/op2/) with a RISC-V (RV32) backend; 
the repository also includes an emulation of a machine (VM) very similar to the one described by Wirth in his
[Project Oberon Book](http://www.inf.ethz.ch/personal/wirth/ProjectOberon/PO.Computer.pdf), 
based on the well-known [RV32 emulator](https://github.com/sysprog21/rv32emu);
the latter is a stripped-down and slightly modified version with only the interpreter,
combined with my own implementation of the machine and the peripherals; 
the memory map of Wirth's machine is reproduced 1:1 so that Kernel.Mod, Display.Mod and 
Input.Mod are unchanged.

Here is a screenshot of the system running natively on the RISC-V VM:

![Project Oberon System Screenshot](http://software.rochus-keller.ch/project_oberon_system_rv32.png)

### What is the Project Oberon System 2013

As you might know, between 1986 and 1989, Niklaus Wirth and Jürg Gutknecht at ETH Zürich designed
and implemented an entire computer system, including an operating system, compiler,
programming language, text and graphics editors, essentially by themselves, 
and then documented everything in the book _Project Oberon - The Design of an Operating System and Compiler_ (1992).

Wirth continued this journey after his retirement. The sources published on [projectoberon.net](https://projectoberon.net)
are written in _Oberon-07_, which is Wirth's last and most radical simplification of the language. 
There is also a free 2013 revision of the Project Oberon book available.
The declared purpose of the 2013 project is unchanged from 1992: 
_to provide a single book that serves as an example of a system that exists, is in actual use, and is explained in all detail_. 

The 1992 book and project used the _National Semiconductor NS32032_ processor, which "is now neither available nor is its architecture recommendable". 
Instead of retargeting the compiler to some other commercial architecture, Wirth decided to design his own processor, 
which he called _RISC-5_, 
"in order to extend the desire for simplicity and regularity to the hardware". 
He even implemented it with a programmable gate array (FPGA) and turned his design into 
"a real, functioning processor on a single chip". The whole system runs on a low-cost development board (Xilinx Spartan-3 by
Digilent, with 1 MB of static RAM) that "easily accommodates the entire Oberon System, including its compiler".
So for the first time not only the software but also the hardware of the Oberon System is described completely
and rigorously. The hardware modules are implemented in _Verilog_ , also available on projectoberon.net.

Thanks to the simplifications of language and processor, all parts that in 1992 had been written in assembly code (and
were not in the book) are now expressed in Oberon as well, from device drivers to raster operations. Wirth based his 
new system directly on the original Ceres version, discarding the features of the later Oberon lines.
"It has been my desire to present the system essentially as it existed 25 years ago, without embellishments".
The result is a small and sufficiently complete system which is well documented and easy to migrate to other architectures.
The entire hardware/software contract of Project Oberon consists of a memory map plus the instruction set.

### Why the migration

The name collision with _RISC-V_ is amusing, but the kinship is real at the level of RISC design philosophy: 
RISC-V, developed at UC Berkeley from 2010 on, is the fifth RISC architecture of the Berkeley line (RISC-I, RISC-II, SOAR, SPUR), 
and both Berkeley's and Wirth's share common design goals and features:
e.g. a regular 32-bit load/store, compiler-friendly ISA, with fixed 32-bit base instruction encodings. 

Migrating the Project Oberon System from RISC-5 to RISC-V is a pragmatic way to bring the system to widely available contemporary hardware 
while preserving the principles that make Oberon valuable. Espressif offers inexpensive, readily available microcontrollers in several ESP32 families, 
and board makers such as Olimex build practical development boards around them, for example the ESP32-P4-PC which provides all resources needed 
by the Oberon System at a very attractive price.
Since the Oberon system does not require an MMU, it is well-suited for this type of microcontroller. 
So far, this migration runs on an emulated RISC-V machine, which helps both debugging and keeping the code close to the book.
Future iterations will migrate this (and also System 3) to the mentioned Olimex board.

Wirth's own compiler (OR) targets his RISC-5 architecture and compiles Oberon-07. 
I could have added an RV32 back end to it; instead, this project reuses the OP2 compiler
which I already used for the [migration of Oberon System 3 to the Raspberry Pi](https://github.com/rochus-keller/oberonsystem3native).
OP2 is itself part of the ETH Oberon heritage; its front end/back end separation was designed exactly for this: 
the same front end has produced code for SPARC, MIPS, i386, and recently ARMv7 and RV32.
[My OP2 modifications](https://github.com/rochus-keller/op2/), the ARMv7 backend and boot linker have proven themselves in the System 3 migration.
Extending OR would have meant maintaining another compiler. Migrating the system to the 1990 language 
keeps one compiler for both migrated systems, and the source code is still close enough to the book to keep it useful.

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
- byte-sized data becomes SYSTEM.BYTE where possible, or CHAR where unavoidable
- type case statements are expressed as IF with IS relation and type guards
- array assignments that Oberon 90 rejects use COPY
- Oberon 07 byte-string literals ($..$) are initialized at runtime via SYS.PutHex
- ORD(SYSTEM.BYTE) is signed in OP2, so we use CHAR instead where necessary
- SYSTEM.BIT with variable instead of constant address because of OP2 issue

Implemented a RISC-V machine based on rv32emu similar to Wirth's RISC-5 described in PO book
and made the necessary (minimal) changes to the Oberon code to run it.

All modules are linked into the boot image by the boot linker and their bodies have been executed at startup,
no dynamic loading.

Added additional apps, see readme in corresponding subdirectory. 

### Precompiled versions

So far, the following version is available

- [Linux x64](http://software.rochus-keller.ch/rv32_oberonsystem_linux_x64.tar.gz)
- [Windows x86](http://software.rochus-keller.ch/rv32_oberonsystem_win32_x86.zip)

Note that the included po.bin and disk.img files work on all platforms. Only the rv32vm executable is
platform dependent. If you therefore just want to build the vm on another platform, you can reuse the other files.

### How to build

There is a build.sh in the vm subdirectory which has to be run first. Then the build.sh in the root can be executed like

- `./build.sh run` to build, pack and run the system in the VM
- `./build.sh disk` to build and pack the system without running it
- `./build.sh link` to build and link the system without creating the disk nor running it
- `./build.sh` just compile all Oberon modules and stop

The build scripts were implemented and tested on Debian Bookworm Linux. 

There is also a qmake (vm.pro) project to build the VM which 
is likely to work on macOS as well, but it has only been tested on Linux so far.

The VM can also be built using the [BUSY build system](https://github.com/rochus-keller/BUSY);
it only requires a C99 compiler and SDL2 and has successfully been tested on Linux and Windows. 
An SDL2 development package matching your toolchain [has to be downloaded](https://github.com/libsdl-org/SDL/releases). 
Use `-P win_sdl_dir=<path>` with path pointing to the root of the SDL2 directory (where `SDL2/include` and `SDL2/lib` exist).
Run the system on Windows with `rv32vm.exe --base 0x0 --disk disk.img po.bin`

### Credits

- See oberon_license.txt which applies to the Oberon source code and documentation.
- See vm/rv32emu/README_orig.md and vm/rv32emu/LICENSE for more information about the RISC-V emulator.
- See vm/softfloat/README_orig.md and vm/softfloat/COPYING.txt for more information about the Berkeley SoftFloat library.
- The machine in the vm subdirectory is available under the terms of the GNU General Public License (GPL) versions 2.0 or 3.0 as published by the Free Software Foundation.

