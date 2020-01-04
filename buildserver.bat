@echo off 

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

echo WAITING FOR PDB > lockS.tmp
cl %commoncompilerflags% -DFORG_SERVER=1 -I "..\..\openssl\include" -I "C:\Program Files\MySQL\MySQL Server 5.7\include" -LD -Fmforg_server.map ..\code\forg_server.cpp /link Gdi32.lib User32.lib Shell32.lib %commonlinkerflags% -PDB:forg_server%random%.pdb /EXPORT:SimulateWorlds /EXPORT:ServerFrameEnd
del lockS.tmp

cl %commoncompilerflags% -DFORG_SERVER=1 -I "..\..\openssl\include" -I "C:\Program Files\MySQL\MySQL Server 5.7\include" -Fmwin32_server.map ..\code\win32_server.cpp /link Ws2_32.lib Shell32.lib Dbghelp.lib

popd

