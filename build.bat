@echo off

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
DEL *.pdb > NUl 2> NUL
popd

call buildclient.bat
call buildserver.bat
