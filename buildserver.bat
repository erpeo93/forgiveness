@echo off 

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

echo WAITING FOR PDB > lockS.tmp
cl %commoncompilerflags% -DFORG_SERVER=1 -I "..\..\openssl\include" -I "C:\Program Files\MySQL\MySQL Server 5.7\include" -LD -Fmforg_server.map ..\code\forg_server.cpp /link "C:\Program Files\MySQL\MySQL Server 5.7\lib\libmysql.lib" "..\..\openssl\libssl.lib" "..\..\openssl\libcrypto.lib" "C:\work\sqlite3\sqlite3.lib" %commonlinkerflags% -PDB:forg_server%random%.pdb /EXPORT:NetworkStuff /EXPORT:InitializeServer /EXPORT:SimulateWorlds /EXPORT:ServerFrameEnd
del lockS.tmp

cl %commoncompilerflags% -DFORG_SERVER=1 -I "..\..\openssl\include" -I "C:\Program Files\MySQL\MySQL Server 5.7\include" -Fmwin32_server.map ..\code\win32_server.cpp /link "C:\Program Files\MySQL\MySQL Server 5.7\lib\libmysql.lib" "C:\work\openssl\libssl.lib" "C:\work\openssl\libcrypto.lib" "C:\work\sqlite3\sqlite3.lib" Ws2_32.lib Shell32.lib

popd

