@echo off
rem Build the original PlayStation executable (Spongey.cpe) on Windows.
rem Requires MSYS2 (C:\msys64) with make installed, and a prior data build
rem (the generated headers in out\<TERRITORY>\include are compile inputs).
rem Usage: port\build-psx.cmd [TERRITORY] [VERSION]      (defaults: USA DEBUG)
setlocal
if not defined MSYS2_WIN set "MSYS2_WIN=C:\msys64"
set "SCRIPT=%~dp0build-psx.sh"
set "SCRIPT=%SCRIPT:\=/%"
"%MSYS2_WIN%\usr\bin\bash.exe" -l "%SCRIPT%" %*
rem propagate the build's exit code to the caller
endlocal & exit /b %ERRORLEVEL%
