#!/bin/bash
# compile the Project Oberon sources with OP2 (rv32) in dependency order,
# link them to a boot image, build a disk image and run it on the emulator;
# the tools are expected next to this directory, override with the variables
op2=${op2:-../bin/op2}            # the OP2 compiler (from OP2 project)
mbl=${mbl:-../bin/multibootlinker} # the linker (from ActiveOberon project)
emu=${emu:-../vm/build/rv32vm} # the emulator
files=${files:-../files}      # the files to put on the disk image
size=${size:-1024}            # size of the disk image in KBytes

rm -rf build && mkdir build && cd build
# from here on we are in the build directory of the source tree
cp ../inner/*.Mod ../outer/*.Mod ../graph/*.Mod ../apps/*.Mod .
inner="SYS Kernel FileDir Files Modules"
outer="Display Fonts Texts Viewers Input Oberon MenuViewers TextFrames Edit System"
graph="Graphics GraphicFrames Curves Draw GraphTool Rectangles"
apps="Hilbert Sierpinski Stars Hennessy"
case "$1" in
  inner) list="$inner";;
  outer) list="$inner $outer";;
  graph) list="$inner $outer $graph";;
  *) list="$inner $outer $graph $apps";;
esac
rc=0
for m in $list; do
  out=$($op2 -s $m.Mod 2>&1 | tail -n +3)
  if echo "$out" | grep -q "err "; then echo "$out"; rc=1; fi
done
[ $rc = 0 ] && echo "OK: compiled $(echo $list | wc -w) modules"
[ $rc = 0 ] || exit $rc

case "$1" in
  link|disk|run) ;;
  *) exit 0;;
esac

# the image is linked to address 0, where Kernel expects the header words, and
# the RAM ends at the frame buffer; the bodies run in link order, Oberon.Loop last
$mbl --arch rv32 --base 0x0 --autofix --ram-size 0xE7EF0 --command Oberon.Loop \
  --log po.link -o po.bin $list || exit 1
echo "OK: linked po.bin"

[ "$1" = link ] && exit 0

# compile mkdisk.c, result is in the build directory
cc ../tools/mkdisk.c -o mkdisk

# mkdisk takes the host paths of the files to include, one per line
find $files -maxdepth 1 -type f | sort > list.txt
./mkdisk --size $size list.txt disk.img || exit 1

[ "$1" = disk ] && exit 0

exec $emu --base 0x0 --disk disk.img po.bin
