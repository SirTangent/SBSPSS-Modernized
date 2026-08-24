@echo off
rem Remove all build outputs (out\) and regenerable debris.
rem Everything deleted here is gitignored and rebuilt by
rem port\build-data.cmd + port\build-psx.cmd.
setlocal
cd /d "%~dp0.."
if exist out rmdir /s /q out
if exist data\Scripts\defs\trans.scr del /q data\Scripts\defs\trans.scr
if exist sn.ini del /q sn.ini
del /q "\\?\%CD%\nul" 2>nul
echo Build outputs cleaned.
endlocal
