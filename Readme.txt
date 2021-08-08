This is a modified version of the original Oberon System downloaded from
http://www.projectoberon.com/ (files inner.zip and outer.zip) on 2020-01-13

Here are the names, sizes and dates of the original files:
18662 Nov 28  2018 Texts.Mod
33377 Mar 27  2017 TextFrames.Mod
 8355 Jul  5  2016 Display.Mod
15665 Jun 19  2016 System.Mod
 9361 Apr 10  2016 Modules.Mod
 8064 Nov 29  2015 Edit.Mod
12709 Nov 29  2015 Oberon.Mod
10257 Feb  4  2014 Kernel.Mod
 7065 Nov 29  2013 Viewers.Mod
 2803 Nov 29  2013 Input.Mod
 8164 Nov 29  2013 MenuViewers.Mod
13002 Nov 29  2013 FileDir.Mod
16900 Nov 29  2013 Files.Mod
 4249 Nov 29  2013 Fonts.Mod


Modifications:
- renamed all files from *.Mod.txt to *.Mod
- changed windows to unix line endings in all files
- replaced all LONGINT by INTEGER, fixes ROR(SYSTEM.VAL(INTEGER,x),y) in Display.Mod
- removed SYSTEM calls, removed all implementations depending on VAR ARRAY OF BYTE trick
- replaced backend by FFI calls to OBXLJ runtime functions (implemented in C)
