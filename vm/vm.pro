TEMPLATE = app
TARGET = rv32vm

CONFIG -= qt
CONFIG += console
CONFIG += link_pkgconfig

# QMAKE_CFLAGS += -std=gnu11 # only if __HAVE_TYPEOF 1 in common.h is enabled
QMAKE_CFLAGS += -std=c99 -D_POSIX_C_SOURCE=200809L # _POSIX_C_SOURCE=200809L because otherwise struct timespec in utils.c is unknown
DEFINES += FORCE_C99

INCLUDEPATH += \
    $$PWD \
    $$PWD/rv32emu \
    $$PWD/softfloat/include \
    $$PWD/softfloat/RISCV

LIBS += -lSDL2

QMAKE_CFLAGS += -Wno-unused-label

DEFINES += 'INLINE="static inline"'

QMAKE_CFLAGS += -include $$PWD/rv32emu/common.h

DEFINES += \
    HAVE_MMAP=0 \
    RV32_MMIO_CALLBACKS \
    RV32_FEATURE_EXT_M=1 \
    RV32_FEATURE_EXT_A=1 \
    RV32_FEATURE_EXT_F=1 \
    RV32_FEATURE_EXT_C=0 \
    RV32_FEATURE_EXT_V=0 \
    RV32_FEATURE_Zba=0 \
    RV32_FEATURE_Zbb=0 \
    RV32_FEATURE_Zbc=0 \
    RV32_FEATURE_Zbs=0 \
    RV32_FEATURE_Zicond=0 \
    RV32_FEATURE_JIT=0 \
    RV32_FEATURE_T2C=0 \
    RV32_FEATURE_SDL=0 \
    RV32_FEATURE_SDL_MIXER=0 \
    RV32_FEATURE_GDBSTUB=0 \
    RV32_FEATURE_SYSTEM=0 \
    RV32_FEATURE_ELF_LOADER=0 \
    RV32_FEATURE_MOP_FUSION=1 \
    RV32_FEATURE_BLOCK_CHAINING=1 \
    RV32_FEATURE_LOG_COLOR=0 \
    RV32_FEATURE_ARCH_TEST=0 \
    RV32_FEATURE_RV32E=0 \
    RV32_FEATURE_FULL4G=0

DEFINES += \
    LITTLEENDIAN=1 \
    INLINE_LEVEL=5 \
    SOFTFLOAT_ROUND_ODD \
    SOFTFLOAT_FAST_INT64 \
    SOFTFLOAT_FAST_DIV64TO32

# rv32_template.c and rv32_constopt.c are included in emulate.c
SOURCES += \
    $$PWD/rv32emu/emulate.c \
    $$PWD/rv32emu/decode.c \
    $$PWD/rv32emu/riscv.c \
    $$PWD/rv32emu/io.c \
    $$PWD/rv32emu/map.c \
    $$PWD/rv32emu/mpool.c \
    $$PWD/rv32emu/cache.c \
    $$PWD/rv32emu/utils.c \
    $$PWD/rv32emu/log.c \
    $$PWD/rv32emu/elf.c

SOURCES += \
    $$files($$PWD/softfloat/*.c) \
    $$files($$PWD/softfloat/RISCV/*.c)

SOURCES += \
    $$files($$PWD/*.c)

HEADERS += \
    $$files($$PWD/*.h) \
    $$files($$PWD/rv32emu/*.h) \
    $$files($$PWD/softfloat/include/*.h) \
    $$files($$PWD/softfloat/RISCV/*.h)
