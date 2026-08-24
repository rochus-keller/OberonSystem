This is a version of the [Project Oberon System](https://projectoberon.net/) written entirely in the
[Micron programming language](https://github.com/micron-language/specification) and compiled with the
[micc compiler](https://github.com/rochus-keller/micron/) with its RISC-V (RV32) backend.

The branch started from the [op2-rv32 branch](https://github.com/rochus-keller/OberonSystem/tree/op2-rv32) of this
repository, which migrated the system from Oberon-07 to Oberon 90 for the OP2 compiler; see its
[Readme](https://github.com/rochus-keller/OberonSystem/blob/op2-rv32/Readme.md) for the history of Project Oberon,
the reasons for choosing RISC-V, and the description of the emulated machine.
Everything said there about the machine still applies here: the VM, the disk image tools and the font/file
resources are unchanged; only the Oberon modules have been replaced by their Micron counterparts.

Here is a screenshot of the Project Oberon system running natively on the RISC-V VM 
(looks identical in the Oberon 90 and Micron versions):

![Project Oberon System Screenshot](http://software.rochus-keller.ch/project_oberon_system_rv32.png)

### What changed compared to op2-rv32

- All Oberon modules (\*.Mod) were replaced by Micron modules (*.mic). The module structure and the
  design follow the book as closely as the language allows, so the Project Oberon book remains the documentation of this system.
- The initial migration was done with [the o2m transpiler from the ActiveOberon Project](https://github.com/rochus-keller/activeoberon/)
  and then each module was manually edited considering all TODOs left by the transpiler, until the result
  correspondet to the intended Micron architecture and style.
- The system is compiled with [micc](https://github.com/rochus-keller/micron/) instead of OP2. 
  The *.obpro project files became *.micpro files, and build.sh in the root builds the whole system using micc.
- The vm subdirectory, the file tools and the resource files (fonts, graphics libraries, disk layout) are untouched; 
  the hardware/software contract, i.e. the memory map plus the RV32 instruction set, is the same as in op2-rv32.

### Migration notes

Micron is a systems programming language in the Oberon tradition, but it deviates from Oberon in ways
that show in the source:

- Micron can take the address of variables, fields and elements; so VAR parameters are no longer necessary; 
  procedures that modify an argument take a pointer, and the caller passes the address explicitly with `@`.
- Signed and unsigned integers are separate type families with explicit widths (int32, uint8, ...);
  all conversions between them are written out.
- Garbage collection is implemented by the Kernel, not the compiler. The compiled code calls the `MIC$` runtime for
  `new`, `newgc`, `println` etc.; on this machine there is no C library underneath, so the runtime is
  implemented in Micron itself: the MIL module MIC+.mil declares the `MIC$` symbols and forwards them
  to the module MIC, and the Kernel installs its allocators in MIC at startup. 
  Every allocation in the system therefore runs through the Kernel, without compiler fixups or boot linker magic.
- micc generates neither module pointer tables nor type descriptors for a precise collector, 
  so the Kernel replaces Wirth's precise garbage collector with a conservative one (in the style of the Boehm collector):
  the roots are the module data, the whole stack and the spilled registers, and anything
  that looks like a pointer into the heap keeps its block alive. Wirth's free list structure and the sweep are kept unchanged.
- Wirth's fixed memory map is generalized: the VM takes a `--ram <MB>` option, places the frame buffer
  at the top of RAM, and passes the layout (heap origin, data origin, memory limit) to the software in
  a small boot header at the bottom of memory, from which Kernel.Init reads its world. With `--ram 1`
  the layout coincides with Wirth's original.

As in op2-rv32, all modules are linked into the boot image and their bodies are executed at startup;
there is no dynamic loading. See Design.md for more information.

### Precompiled versions

So far, the following version is available

- [Linux x64](http://software.rochus-keller.ch/rv32_micronsystem_linux_x64.tar.gz)
- [Windows x86](http://software.rochus-keller.ch/rv32_micronsystem_win32_x86.zip)

Note that the included system.bin and disk.img files work on all platforms. Only the rv32vm executable is
platform dependent. If you therefore just want to build the vm on another platform, you can reuse the other files.

### How to build

The VM is built as described in the [op2-rv32 Readme](https://github.com/rochus-keller/OberonSystem/blob/op2-rv32/Readme.md)
(build.sh in the vm subdirectory, or the qmake/BUSY alternatives described there).
Then the build.sh in the root can be executed like

- `./build.sh run` to build, pack and run the system in the VM
- `./build.sh disk` to build and pack the system without running it
- `./build.sh` to build and link the system without creating the disk nor running it

The build scripts were implemented and tested on Debian Bookworm Linux.

### Credits

- See oberon_license.txt which applies to the Oberon source code and documentation, and to the Micron source code.
- See vm/rv32emu/README_orig.md and vm/rv32emu/LICENSE for more information about the RISC-V emulator.
- See vm/softfloat/README_orig.md and vm/softfloat/COPYING.txt for more information about the Berkeley SoftFloat library.
- The machine in the vm subdirectory is available under the terms of the GNU General Public License (GPL) versions 2.0 or 3.0 as published by the Free Software Foundation.



