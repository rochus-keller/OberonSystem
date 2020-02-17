## Welcome to the Oberon System running on LuaJIT

This project is a spin-off from https://github.com/rochus-keller/Oberon. It makes use of my Oberon to LuaJIT bytecode compiler. I'm interested in re-using LuaJIT (see http://luajit.org/) as a runtime environment for strictly/statically typed languages. As proof-of-concept I have written a validating Oberon-07 compiler that directly generates LuaJIT bytecode. To test and validate the compiler I have created this version of the Oberon System which runs in the Oberon IDE on the integrated LuaJIT VM using a Qt based backend instead of the original Oberon System inner modules. Note that this is a prototype.


Screenshot of the running Oberon System:
![Oberon System Screenshot](http://software.rochus-keller.info/screenshot_oberon_system_on_luajit.png)


Screenshot of the IDE in the source level debugger at a break:
![Oberon IDE Screenshot](http://software.rochus-keller.info/screenshot_oberon_system_in_debugger.png)

Here is a binary version of the Oberon IDE for Windows: http://software.rochus-keller.info/OberonIDE_win32.zip.
Just unpack the ZIP somewhere on your drive and double-click OberonIDE.exe; Qt libraries are included, as well as this version of the Oberon System. To run it, open the project in the IDE using CTRL+O and then press CTRL+R, or right-click to open context menus and select the commands from there.

The implementation is platform independent. It can be built on all platforms where Qt 5 is available (e.g. Linux, macOS, FreeBSD, Raspberry PI, etc.).

For more information about the Oberon IDE and LuaJIT bytecode compiler see https://github.com/rochus-keller/Oberon/blob/master/README.md

For more information about the Oberon programming language see https://en.wikipedia.org/wiki/Oberon_(programming_language)

For more information about the Oberon System see https://en.wikipedia.org/wiki/Oberon_(operating_system)

For more information about LuaJIT see http://luajit.org

## Issues
I'm aware that there are still issues; e.g. Texts.WriteReal doesn't seem to work, neither System.Copy. You can post an issue or a fix to https://github.com/rochus-keller/OberonSystem/issues.


