@echo off

call buildrelease.bat

pushd .
cd ..
set hh=%time:~-11,2%
set /a hh=%hh%+100
set hh=%hh:~1%
set mydir= BUILD%date:~10,4%-%date:~4,2%-%date:~7,2%-%hh%-%time:~3,2%-%time:~6,2%
mkdir %mydir%
cd %mydir%

mkdir build

mkdir client
mkdir client\assets

mkdir editor
mkdir editor\assets
mkdir editor\definition

mkdir server
mkdir server\assets

copy ..\build\asset_builder.exe build
copy ..\build\forg_server.dll build
copy ..\build\win32_server.exe build


copy ..\build\forg_client.dll client
copy ..\build\win32_client.exe client
copy ..\client\assets\forgivenessF.upak client\assets


xcopy ..\editor\definition editor\definition /E /Y /I /V


xcopy ..\editor\assets server\assets /E /Y /I /V

popd