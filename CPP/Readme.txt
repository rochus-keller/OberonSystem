This is a C++ version of the Oberon System. I implemented it
before the LuaJIT based implementation to better understand the 
system and to be able to use the existing C++ development 
tools (the Oberon IDE came later). 

To create this version I first added the missing Oberon syntax
elements to my Oberon to C++ transpiler and then converted the
Oberon System sources using my OberonViewer application. I then
manually modified the generated C++ sources until they compiled
and run without crash. 

The ObSysInnerLib.cpp is based on this implementation.

This code is published for documentation purposes only. I will 
not change it further.

RK/2020-02-15
