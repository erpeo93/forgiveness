@echo off

call buildcommon.bat

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build

REM preprocessor
cl %commoncompilerflags% ..\code\simple_preprocessor.cpp /link %commonlinkerflags%
pushd ..\code
..\build\simple_preprocessor.exe > client_generated.h
popd

REM compressor
REM cl %commoncompilerflags% ..\code\simple_compressor.cpp /link %commonlinkerflags% 


REM asset builder
cl %commoncompilerflags% ..\code\asset_builder.cpp /link %commonlinkerflags% Gdi32.lib User32.lib Shell32.lib


echo WAITING FOR PDB > lock.tmp

cl %commoncompilerflags% -I ..\..\iaca-win64\ -Fmforg_client.map ..\code\forg_client.cpp -LD /link %commonlinkerflags% -PDB:forg_client%random%.pdb /EXPORT:GameUpdateAndRender /EXPORT:GameGetSoundOutput /EXPORT:GameDEBUGFrameEnd
del lock.tmp


cl %commoncompilerflags% -I "..\..\openssl\include" -Fmwin32_client.map ..\code\win32_client.cpp /link "C:\work\openssl\libssl.lib" "C:\work\openssl\libcrypto.lib" %commonlinkerflags% User32.lib Gdi32.lib Winmm.lib opengl32.lib Ws2_32.lib Shell32.lib

popd

