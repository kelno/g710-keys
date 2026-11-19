@echo off
rem Builds g710.exe next to this file. Needs Visual Studio (or its Build Tools) with the C++ tools.
setlocal
set VSWHERE=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe
if not exist "%VSWHERE%" (echo vswhere.exe not found: install Visual Studio with the C++ tools & exit /b 1)
for /f "usebackq delims=" %%i in (`"%VSWHERE%" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do set VSDIR=%%i
if not defined VSDIR (echo Visual Studio C++ tools not found & exit /b 1)
rem vcvars64.bat looks for vswhere.exe on PATH itself.
set "PATH=%ProgramFiles(x86)%\Microsoft Visual Studio\Installer;%PATH%"
call "%VSDIR%\VC\Auxiliary\Build\vcvars64.bat" >nul || exit /b 1
cd /d "%~dp0"
if not exist build mkdir build
rc /nologo /fo build\g710.res src\g710.rc || exit /b 1
cl /nologo /O1 /W4 /MT /Fo"build\\" /Fe"g710.exe" src\g710.c build\g710.res /link /SUBSYSTEM:WINDOWS || exit /b 1
