@echo off

set buildpath=..\..\build

set hh=%time:~-11,2%
set /a hh=%hh%+100
set hh=%hh:~1%
set buildname=%date:~10,4%-%date:~4,2%-%date:~7,2%-%hh%-%time:~3,2%-%time:~6,2%

if "%~1"=="" (
echo Enter a valid version name!
exit /B 1
)

set buildname=%~1
set mydir=BUILD_%buildname%
set mypdbdir=PDBS_%buildname%


if EXIST ..\release\%mydir% (
echo Version %buildname% already present!
exit /B 1
)

call buildcommon.bat
set commoncompilerflags= -O2 -DFORGIVENESS_STREAMING=0 -DFORGIVENESS_SLOW=1 -DFORGIVENESS_INTERNAL=1 %commoncompilerflags%
call buildclient.bat
call buildserver.bat

pushd .
cd ..\release
mkdir %mydir%
cd %mydir%

mkdir errors
mkdir assets

mkdir server
mkdir server\assets
mkdir server\errors

copy %buildpath%\forg_server.dll server
copy %buildpath%\win32_server.exe server
copy %buildpath%\forg_client.dll .
copy %buildpath%\win32_client.exe .
rename win32_client.exe forgiveness.exe

xcopy ..\..\client\assets\*.upak assets /E /Y /I /V
xcopy ..\..\server\assets\*.upak server\assets /E /Y /I /V

popd



pushd .
cd ..\release
mkdir %mypdbdir%
cd %mypdbdir%

mkdir dumps

copy %buildpath%\forg_server.dll .
copy %buildpath%\win32_server.exe .
copy %buildpath%\forg_client.dll .
copy %buildpath%\win32_client.exe .
copy %buildpath%\win32_client.exe forgiveness.exe
copy %buildpath%\*.pdb .

mkdir code
cd code

xcopy ..\..\..\code . /E /Y /I /V
popd