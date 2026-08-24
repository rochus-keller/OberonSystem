# The Micron Project Oberon System — design description

This is the design description of the Micron port of Project Oberon (Wirth/Gutknecht, edition 2013). 
It records what differs from Wirth's implementation, so that the sources themselves can stay
as sparsely commented as the originals. Everything not mentioned here is meant to be Wirth's design,
module by module and procedure by procedure.

The port targets the RV32 machine of the Project Oberon emulator, is linked statically into one freestanding ELF file and needs no C library.

---

## 1. Build and startup

Wirth's system is built in two stages: `ORP` compiles the modules into `.rsc` files, and a *boot linker* (`BootLoad`/`ORL`)
statically links the inner core into a boot image which the machine loads from the disk's boot sectors;
the outer core is then loaded dynamically by `Modules`.

The Micron system has one stage. `micc` compiles and links all 27 modules into a single ELF file which the emulator loads directly:

```
micc --target rv32 -g --base 0 --runtime MIC+.mil -I . -o system.bin Main.mic
```

and the emulator runs the result with:

```
rv32vm --elf --ram 4 --disk disk.img system.bin
```

`micc` calls the module bodies in the order of the import dependencies, so the module bodies replace the initialization the boot linker generates.
`Main` is the last body to run; it plays the role of Wirth's boot linker for the outer core:

- it registers the commands of `System`, `Edit`, `Draw`, `Rectangles`, `Curves`, `GraphTool` and the example modules with `Modules.Register`,
- it enters the `Oberon.Loop`.

`Oberon.Loop` is therefore called from `Main` and not, as in Wirth's system, at the end of Oberon's own body.

## 2. Modules: no loader, no boot linker

`Modules` keeps its interface but the original function is no longer required: 
there is no `.rsc` symbol format in Micron, and no dynamic loading. 
Micron instead generates relocatable ELF objects per module with integrated symbol information
which the compiler can use for separate compilation; but this machinery is only used for
cross-compilation in this project, not for evaluation at runtime. Consequently

- `Modules.Load` does not load; it finds the module descriptor of an already linked module, 
  and creates the descriptor on demand (`ThisMod`), since no boot linker generated it;
- commands live in one table `cmdTab` filled by `Modules.Register`. A command is a *procedure value* (`Kernel.Cmd`), 
  not an address as in the original, so `ThisCommand` returns a procedure and `System.ShowCommands` lists the registered names;
- `Modules.Free` and `Modules.FreeMod` do nothing that unloads code; they only remove descriptors;
- `Kernel.modules` is the list of these descriptors, not a list built by the boot linker.

The two tables of `Modules` end with a sentinel instead of being bounded by `LEN(t^)`: 
Micron follows C in its philosophy and has no `LEN` for a dynamic array. The sentinel also gives a well-defined end to a future precise collector.

## 3. Memory: the Kernel owns the heap, NEW is redirected to it

In Wirth's system the compiler emits calls to `Kernel.New*` through the fixups the boot linker resolves (`OP2`'s `sysfix`). 
`micc` instead emits calls to its runtime interface `MIC$`, which the port implements itself, since the machine has no C library: 
`MIC+.mil` declares the `MIC$` names the generated code refers to and forwards each of them to `MIC` (`MIC.mic`), 
where the operations are written in Micron. The file is called `MIC+.mil` (`$` are translated to `+` on the disk), 
so that no module can collide with a runtime symbol.

The four allocation entry points are:

| Micron   | MIL                  | RV32 symbol |
| ---      | ---                  | --- |
| `new`    | `newobj`/`newarr`    | `MIC$$alloc` |
| `newinit`| `newobj0`/`newarr0`  | `MIC$$calloc` |
| `newgc`  | `newobjgc`/`newarrgc` | `MIC$$gcalloc` |
| `dispose`| `free`               | `MIC$$free` |

There is no compiler feature to redirect these, so the port uses callbacks and keeps `micc` unchanged:

- `MIC` holds two procedure variables of type `procedure(size: int32): AnyPtr`
- `Kernel.Init` installs its own allocators into them, by an ordinary import and call:

```
MIC.Install(newSys, newGc)
```

`MIC` must not depend on anything that the generated code needs from `MIC$`, or the runtime would call itself; 
that is the only rule the arrangement imposes.

So the Kernel owns all of memory, as in the original, but now including every allocation the compiler generates. 
Two kinds of block exist:

- `SysBlk` allocated by `newSys`, never reclaimed. Used where a block must live as long as the system: 
  the Kernel's own structures, `Files`' sector buffers, module descriptors
- `GcBlk` allocated by `newGc`, may be reclaimed by the collector. `newGc` collects once and retries when the heap is full.

All modules therefore use `newgc` where Wirth's `NEW` allocates a collectable object (which raised their language level),
and `newinit` only where the block must survive. `Kernel.New` keeps Wirth's free-list structure 
(blocks of 32, 64, 128 and n·256 bytes), and keeps a 16-byte header, so that `OP2`'s layout expectation 
(type tag at `ptr-4`) remains describable; the kind and the mark bits live in that header.

`Kernel.NewRec`/`NewArr`/`NewSys`, the entry points of the traps which Wirth's compiler generates for `NEW` were removed: 
every allocation reaches the Kernel through the callbacks, and the block header is the same for all kinds of block.

## 4. The conservative collector

Wirth's collector is _precise_: `Kernel.GC(roots)` marks from the table of global pointers 
which the boot linker generates for each module, 
and follows the pointer offsets in each object's type descriptor at `ptr-4`.

`micc` generates neither of the two, and extending it for this single example is not worth it 
The Micron Kernel therefore collects _conservatively_ (Boehm-style) in about 120 lines:

- roots: every word of the static data area (`dataOrg` up to the stack) and of the _whole_ stack,
  including the dead frames above the current stack top;
  no stack pointer is needed, at the price of retaining what those frames still contain
- objects: every word of a reachable block is a candidate
- a candidate keeps a block alive also when it addresses the block's _interior_, which is required here:
  e.g. the glyph pointers in `Fonts` point into the middle of a block
- address to block is one index into a *block map* (`mapBlocks`: for each 1 KB chunk the block covering its first byte,
  rebuilt at the start of every collection) plus a walk forward of at most 32 blocks. 
  A block whose size is not a positive multiple of 8 ends the walk, so a candidate which addresses a region the heap walk 
  does not describe cannot create a pseudo block
- marking uses an explicit stack of 1024 entries; on overflow the heap is rescanned for blocks that are marked but not yet expanded 
  (mark 1 vs. mark 2), so marking is complete but may take several passes
- sweeping rebuilds the four free lists exactly as Wirth's `Scan` does, coalescing runs of free and unmarked blocks (`freeArea`).

Deliberate consequences:

- a word that only _looks_ like a heap address retains a block
- no interior-pointer restriction, no type information and no compiler support are needed
- `Oberon.GC` calls `Kernel.Collect()` instead of `Kernel.GC(roots)`; there is no root set to pass.
  As in the original it runs as a task of the central loop, i.e. only between commands, 
  so an object reachable only from a local variable of a running command can never be collected.

Also as in the original, the GC task runs when the heap is nearly full or after a number of input actions; 
`System.Watch` prints `Kernel.Collections` in addition to Wirth's figures.

## 5. Registers as roots

A conservative collector must also see pointers that the compiler keeps in the registers.
`SYS.spillRegs` stores `ra`, `sp` and `s0` to `s11`; 
these are the registers of the RV32 calling convention that a callee must preserve, 
i.e. the only ones which can hold a pointer across a call; 
these fourteen words the collector passes in, and `Kernel.Collect` marks that area first, as its first statement.

The procedure is written in Micron with `getreg`, which is available at language level 0 and 1, 
so `SYS` is a `level=1` module; it needs no MIL and no `extern`. 
Since `spillRegs` is a real call, `getreg(8, ...)` may read the callee's `s0` rather than the caller's;
the caller's value is then in `spillRegs`' own frame, which is part of the stack the collector scans anyway.

## 6. Memory layout, display and the boot header

Wirth's machine has a fixed memory map: the display frame is at `0E7F00H` and `Kernel.MemLim` at `0E7EF0H`.
Since the conservative collector needs a bigger heap than the resulting 164 KB, the emulator takes a `--ram <MB>` option, 
places the framebuffer at the top of RAM and passes the layout in the boot header:

| address | contents |
| ---     | --- |
| `08H`   | heap origin |
| `0CH`   | origin of the writable data (the collector's static root area) |
| `010H`  | `displayBase - 16`, i.e. the end of usable RAM |

Therefore

- `Display` derives its base from `010H` instead of the constant `0E7F00H` (`memLimAdr`)
- `Kernel.Init` reads `MemLim` from `010H`, `dataOrg` from `0CH` and `heapOrg` from `08H`
- the stack is 32 KB below `stackOrg`, and the collector scans exactly that area.

All device addresses stay fixed (timer, SPI, LEDs, RS232, mouse and keyboard), 
so with `--ram 1`, the layout is identical with Wirth's and an original image still boots. 
The system runs with 4 MB, which gives about 3.2 MB of heap.

## 7. Machine access, SYS

`SYSTEM` has no counterpart as a module: what Wirth expresses with `SYSTEM.PUT`/`GET`/`ADR`/`VAL` Micron expresses in the language, 
with pointer literals, `ptroff`, `cast`, `sig`/`usig` and `val`.
What is left is the small module `SYS`:

- `ROT` for `SYSTEM.ROT`: Micron has no rotation operator, so `x` is rotated by two shifts and a modulo,
  which also handles a negative and an out-of-range count
- `spillRegs` (see above)
- `TRACE`/`TRACEHEX` write to the RS232 data register, which serves as a debug console before the display and the text system are up.
  Wirth's original has no counterpart; these were essential during the port and are kept.

Dropped, with the reason:

- `SYSTEM.H`: the remainder of the preceding division of the RISC-5 processor, which Micron expresses by `mod`.
  Wirth's four `ASSERT(SYSTEM.H(0) = 0)` in `Kernel` disk sector arithmetic became real checks (`assert(sec mod 29 = 0)`)
- `SYSTEM.REG`/`REGADR`: they serve the trap handler, and `SYSTEM.LDREG` resets the stack pointer after a trap; the port has no traps (see below).

The absolute addresses of the port are concentrated in `SYS`, `Kernel`, `MIC` and `Display`, each with a comment.

## 8. Arithmetic: signedness and Oberon's division

Micron's signed division truncates toward zero, like C. Where a negative operand can occur, 
the port uses the built-ins `obdiv`/`obmod`, which produce Oberon's result; 
the plain operators are used otherwise.

The bigger difference is _signedness_. Project Oberon has one integer type, so Wirth's code reinterprets values freely and uses
`DIV`/`MOD` by powers of two where a shift is meant. Micron separates signed and unsigned types, so the port

- works in the unsigned domain where bits are meant 
  (`Files`/`Texts` serialization, the formatting in `SYS`, the pattern arithmetic in `Display`), 
  with one reinterpretation instead of round trips through `CHR`/`ORD`
- uses shifts where Wirth uses `DIV`/`MOD` by a power of two, and the set operations in `Display` where he uses arithmetic on bit patterns
- omits explicit conversions where Micron adapts an integer literal to the other operand.

The conversions themselves are distinguished, which Oberon cannot:

| what is meant | Micron |
| --- | --- |
| a narrow unsigned value as a number, e.g. a byte of a font metric | `sigl(x)`, or `+x` |
| a signed bit pattern, e.g. `FileDir.DirMark`, a word read from a file | `sigc(x)` |
| a signed value as a bit pattern, before shifting or masking | `usig(x)` |
| a value in a different width | `val(int32, x)` |

`sigc` and `usig` keep the width and reinterpret;`sigc` is a cast; `sigl` (and unary `+`) 
convert to the next larger signed type, as Oberon's `LONG` did. 

The intention was that the result reads like true Micron, not like machine transpiled Oberon, 
while keeping a 1:1 structural correspondence to the original.

## 9. Types and language level

- Oberon's untyped `SYSTEM.PTR`/address arithmetic becomes the helper types `AnyPtr` (`pointer to byte`), `AnyPtrPtr` and `Int32Ptr` plus `ptroff`
- record extension and type tests are used as in the original; the type-case narrowing of Micron is used which is closer to the Oberon-07 than the Oberon 90 source
- `Modules` and `Kernel` keep the descriptor records of the original (`ModuleDesc`, `Command`, `ExportDesc`) 
  although some fields are unused, but the correspondence to the book remains visible
- modules are declared at the level their constructs need: `level=4` for the object-oriented parts, `level=5` for `newgc`, `level=1` for the machine concerns. 
  
## 10. Disk, files and fonts

- the emulator serves a plain disk image, so `Kernel.FSoffset` is 0 instead of `80000H` (Wirth's offset that skips the FAT partition)
- `Files`/`FileDir` are unchanged in design; only the serialization was rewritten in the unsigned domain, 
  and `Files.ReadInt`/`WriteInt` no longer go through `CHR`/`ORD`
- `Fonts` imports `Display` for the `Pattern` type, so a glyph can be passed to `Display.CopyPattern` without a reinterpretation, 
  and it allocates a font only when its size is known (the original allocates before reading the header).

## 11. Traps

The port doesn't support traps. Wirth's system reaches the Kernel through traps: trap 0 is `NEW`, further traps
report an array bound, a type guard or a failed `ASSERT`, 
and `Kernel.Install`/`Kernel.Trap`, `System.Trap`/`Abort` and `Oberon.Reset` implement and recover from them. 
Here `NEW` is a callback (see above) and `micc` generates no trap instructions, 
and a failing `assert` reports on the serial console and halts the machine. 
All of that machinery is therefore dropped rather than kept as dead code.

## 12. What the port does not implement

- dynamic module loading, the `.rsc` format, `ORP`/`ORL`; there is no compiler in the system; everything is cross-compiled
- `Kernel.GC` in Wirth's precise form, and with it the module pointer tables
- the fixed memory map
- `Oberon.Collect`'s type-descriptor-based marking, which the conservative collector replaces
- traps and their

Everything else follows the original: the display, the viewer and menu machinery, the text system, `Edit`, `Draw` with
`Graphics`/`GraphicFrames`/`GraphTool`, `Files`/`FileDir`, `Fonts`, `Input`, the example application modules.
