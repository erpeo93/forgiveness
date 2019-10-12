@echo off

set buildpath=..\..\build

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

call build.bat

pushd .
cd ..\release
mkdir %mydir%
cd %mydir%

mkdir build

mkdir errors

mkdir assets

mkdir editor
mkdir editor\assets
mkdir editor\definition
mkdir editor\errors

mkdir server
mkdir server\assets
mkdir server\errors

copy %buildpath%\forg_server.dll build
copy %buildpath%\win32_server.exe build

copy ..\..\code\readme.txt .
copy ..\..\code\changelog.txt .


copy %buildpath%\forg_client.dll .
copy %buildpath%\win32_client.exe .
rename win32_client.exe forgiveness.exe
copy %buildpath%\asset_builder.exe .
copy ..\..\client\assets\forgivenessF.upak assets


xcopy ..\..\client\editor\definition editor\definition /E /Y /I /V

xcopy ..\..\client\editor\assets server\assets /E /Y /I /V

popd



pushd .
cd ..\release
mkdir %mypdbdir%
cd %mypdbdir%

mkdir dumps

copy %buildpath%\asset_builder.exe .
copy %buildpath%\forg_server.dll .
copy %buildpath%\win32_server.exe .
copy %buildpath%\forg_client.dll .
copy %buildpath%\win32_client.exe .
copy %buildpath%\win32_client.exe .
copy %buildpath%\win32_client.exe forgiveness.exe
copy %buildpath%\asset_builder.exe .
copy %buildpath%\*.pdb .

mkdir code
cd code

xcopy ..\..\..\code . /E /Y /I /V
popd