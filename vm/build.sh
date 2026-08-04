#!/bin/sh
# build the Oberon RISC machine emulator (rv32emu interpreter + Berkeley SoftFloat + SDL2)
set -e
cd `dirname $0`
ROOT=.
OUT=${OUT:-build}
CC=${CC:-cc}
CFLAGS="-O2 -g -std=gnu11 -Wall"

RVFLAGS="-DHAVE_MMAP=0 -DRV32_MMIO_CALLBACKS -DRV32_FEATURE_EXT_M=1 -DRV32_FEATURE_EXT_A=1 -DRV32_FEATURE_EXT_F=1 \
-DRV32_FEATURE_EXT_C=0 -DRV32_FEATURE_EXT_V=0 \
-DRV32_FEATURE_Zba=0 -DRV32_FEATURE_Zbb=0 -DRV32_FEATURE_Zbc=0 -DRV32_FEATURE_Zbs=0 \
-DRV32_FEATURE_Zicond=0 -DRV32_FEATURE_JIT=0 -DRV32_FEATURE_T2C=0 \
-DRV32_FEATURE_SDL=0 -DRV32_FEATURE_SDL_MIXER=0 -DRV32_FEATURE_GDBSTUB=0 \
-DRV32_FEATURE_SYSTEM=0 -DRV32_FEATURE_ELF_LOADER=0 \
-DRV32_FEATURE_MOP_FUSION=1 -DRV32_FEATURE_BLOCK_CHAINING=1 \
-DRV32_FEATURE_LOG_COLOR=0 -DRV32_FEATURE_ARCH_TEST=0 \
-DRV32_FEATURE_RV32E=0 -DRV32_FEATURE_FULL4G=0 \
-include $ROOT/rv32emu/common.h \
-I$ROOT -I$ROOT/rv32emu -I$ROOT/softfloat/include -I$ROOT/softfloat/RISCV"

SFFLAGS="-DLITTLEENDIAN=1 -DINLINE_LEVEL=5 -DSOFTFLOAT_ROUND_ODD \
-DSOFTFLOAT_FAST_INT64 -DSOFTFLOAT_FAST_DIV64TO32 '-DINLINE=static inline' \
-I$ROOT/softfloat/include -I$ROOT/softfloat/RISCV"

mkdir -p $OUT
OBJ=

for f in $ROOT/softfloat/*.c $ROOT/softfloat/RISCV/*.c; do
  o=$OUT/sf_`basename $f .c`.o
  [ $o -nt $f ] || eval $CC $CFLAGS $SFFLAGS -c $f -o $o
  OBJ="$OBJ $o"
done

# rv32_template.c and rv32_constopt.c are included by emulate.c, not compiled on their own
for f in emulate decode riscv io map mpool cache utils log elf; do
  f=$ROOT/rv32emu/$f.c
  o=$OUT/rv_`basename $f .c`.o
  [ $o -nt $f ] || $CC $CFLAGS $RVFLAGS -c $f -o $o
  OBJ="$OBJ $o"
done

for f in *.c; do
  o=$OUT/`basename $f .c`.o
  $CC $CFLAGS $RVFLAGS `sdl2-config --cflags` -c $f -o $o
  OBJ="$OBJ $o"
done

$CC $CFLAGS -o $OUT/rv32vm $OBJ `sdl2-config --libs` -lm -lpthread

