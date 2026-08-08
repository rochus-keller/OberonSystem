This is a part of the RISC-V RV32I[MAFC] emulator.
Downloaded from https://github.com/sysprog21/rv32emu
on Aug. 1, 2026, commit be42e38

See README_orig.md and LICENSE for more information.

Only the files required for the VM are kept.

Changes:
- I added an ifndef RV32_MMIO_CALLBACKS in rv32_template.c to make
  sure that io callbacks work in non-SYSTEM mode.
- Also added an ifdef FORCE_C99 in common.h to support
  C99 build on a recent GCC.
- Updated SoftFloat path in riscv.h
- Added common.h directly to the files depending on it instead via compiler option
- A few fixes to get rid of unused Linux includes to allow Windows builds

