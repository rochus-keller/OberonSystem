#!/bin/bash
# compile & link the Micron sources with micc (rv32) in dependency order,
# the tools are expected next to this directory, override with the variables
micc=${micc:-../bin/micc}            # the micc compiler/linker (from Micron project)
emu=${emu:-../vm/build/rv32vm} # the emulator
files=${files:-../files}      # the files to put on the disk image
size=${size:-1024}            # size of the disk image in KBytes

rm -rf build && mkdir build && cd build
# from here on we are in the build directory of the source tree
cp ../inner/*.mic ../outer/*.mic ../graph/*.mic ../apps/*.mic ../*.mic ../*.mil .

if [ ! -x "$micc" ]; then
    echo "error: micc is not executable: $micc" >&2
    exit 1
fi

if ! out=$("$micc" --target rv32 -g --base 0 --runtime MIC+.mil -I . -o system.bin Main.mic 2>&1); then
    printf '%s\n' "$out"
    exit 1
fi

echo "OK: compiled & linked modules"

case "$1" in
  disk|run) ;;
  *) exit 0;;
esac

# compile mkdisk.c, result is in the build directory
cc ../tools/mkdisk.c -o mkdisk

# mkdisk takes the host paths of the files to include, one per line
find $files -maxdepth 1 -type f | sort > list.txt
./mkdisk --size $size list.txt disk.img || exit 1

[ "$1" = disk ] && exit 0

exec $emu --elf --ram 4  --disk disk.img system.bin
