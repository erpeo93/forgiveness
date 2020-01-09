@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
DEL *.pdb > NUl 2> NUL
popd

call buildcommon.bat
set commoncompilerflags= -O2 -DFORGIVENESS_STREAMING=0 -DFORGIVENESS_SLOW=1 -DFORGIVENESS_INTERNAL=1 %commoncompilerflags%
call buildclient.bat
call buildserver.bat
