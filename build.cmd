@echo off
rem Builds g710.exe next to this file. Needs Visual Studio's C++ tools.
setlocal
set VCVARS=C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat
if not exist "%VCVARS%" (echo Visual Studio C++ tools not found: %VCVARS% & exit /b 1)
call "%VCVARS%" >nul || exit /b 1
cd /d "%~dp0"
if not exist build mkdir build
cl /nologo /O1 /W4 /MT /Fo"build\\" /Fe"g710.exe" src\g710.c /link /SUBSYSTEM:WINDOWS
