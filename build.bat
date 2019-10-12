@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
DEL *.pdb > NUl 2> NUL
popd

call buildcommon.bat
call buildclient.bat
call buildserver.bat
