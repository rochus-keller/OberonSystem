This is the original version of the Project Oberon source code as downloaded from 
https://www.projectoberon.net/ on 2026-04-14.

I particularly downloaded the following archives:
- http://www.projectoberon.net/zip/inner.zip
- http://www.projectoberon.net/zip/outer.zip
- http://www.projectoberon.net/zip/systools.zip
- http://www.projectoberon.net/zip/graph.zip
- http://www.projectoberon.net/zip/apptools.zip

The latest file modification date is 2018-11-28. Each subdirectory of this repository
corresponds to the archive of the same name, besides apptools and systools which
have been merged in files.

Migrated all Oberon-07 sources to Oberon 90 so that they comply with the 
ActiveOberon project o2c compiler and OP2.

Migration notes:
- INTEGER renamed to LONGINT throughout
- SYS.Mod provides the Oberon 07 built-ins not present in Oberon 90 
- byte-sized data stays SYSTEM.BYTE
- type case statements are expressed as IF with IS relation and type guards
- array assignments that Oberon 90 rejects use COPY
- Oberon 07 byte-string literals ($..$) are initialized at runtime via SYS.PutHex

