@echo off
rem Build the PC (Win32) port via MSYS2 bash. Usage: port\build-pc.cmd [debug^|final^|all]
setlocal
set "SCRIPT=%~dp0build-pc.sh"
set "SCRIPT=%SCRIPT:\=/%"
C:\msys64\usr\bin\bash.exe -l "%SCRIPT%" %*
endlocal
