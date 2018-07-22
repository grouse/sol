@ECHO off

SET ROOT=%~dp0
SET BUILD_DIR=%ROOT%\build

if not exist %BUILD_DIR% mkdir %BUILD_DIR%
if not exist %BUILD_DIR%\data mkdir %BUILD_DIR%\data
if not exist %BUILD_DIR%\data\shaders mkdir %BUILD_DIR%\data\shaders

set CFLAGS=-O2 -MTd -nologo -fp:fast -fp:except- -Gm- -GR- -EHa- -Zo -Oi -WX -W4 -wd4201 -wd4100 -wd4189 -wd4505 -wd4127 -FC -Z7
set CFLAGS=-DCOMPILER_MSVC -D_CRT_SECURE_NO_WARNINGS -DRAY_WIN32=1 %CFLAGS%
set LFLAGS= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib

pushd %BUILD_DIR%
cl.exe %CFLAGS% -D_CRT_SECURE_NO_WARNINGS /W4 %ROOT%/sol.cpp /link %LFLAGS%
sol.exe
start test.bmp
popd
