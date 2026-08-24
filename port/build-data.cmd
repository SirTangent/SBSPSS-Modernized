@echo off
rem Build the SBSPSS game data (BigLump.Bin + generated headers) on Windows.
rem Requires MSYS2 (C:\msys64) with make and perl installed:
rem     pacman -S --needed make perl
rem Usage: port\build-data.cmd [TERRITORY] [VERSION]     (defaults: USA DEBUG)
setlocal
set "SCRIPT=%~dp0build-data.sh"
set "SCRIPT=%SCRIPT:\=/%"
C:\msys64\usr\bin\bash.exe -l "%SCRIPT%" %*
endlocal
