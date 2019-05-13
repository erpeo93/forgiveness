@echo off

set buildpath = ..\..\build

set hh=%time:~-11,2%
set /a hh=%hh%+100
set hh=%hh:~1%


set buildname=%date:~10,4%-%date:~4,2%-%date:~7,2%-%hh%-%time:~3,2%-%time:~6,2%
if NOT "%~1"=="" set buildname=%~1

set mydir=BUILD_%buildname%
set mypdbdir=PDBS_%buildname%


if EXIST ..\release\%mydir% (
echo Version %buildname% already present!
exit /B 1
)

call buildrelease.bat

pushd .
cd ..\release
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

copy %buildpath%\asset_builder.exe build
copy %buildpath%\forg_server.dll build
copy %buildpath%\win32_server.exe build

copy ..\..\code\readme.txt .
copy ..\..\code\changelog.txt .


copy %buildpath%\forg_client.dll client
copy %buildpath%\win32_client.exe client
copy %buildpath%\assets\forgivenessF.upak client\assets


xcopy ..\..\editor\definition editor\definition /E /Y /I /V

xcopy ..\..\editor\assets server\assets /E /Y /I /V

popd



pushd .
cd ..\release
mkdir %mypdbdir%
cd %mypdbdir%
copy ..\..\build\*.pdb .
popd