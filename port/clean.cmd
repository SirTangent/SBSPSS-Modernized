@echo off
rem Remove all build outputs and regenerable debris.
rem Everything deleted here is gitignored and rebuilt by
rem port\build-data.cmd + port\build-psx.cmd + port\build-pc.cmd.
setlocal
cd /d "%~dp0.."

rem data build + PSX build outputs
if exist out rmdir /s /q out
if exist data\Scripts\defs\trans.scr del /q data\Scripts\defs\trans.scr
if exist sn.ini del /q sn.ini
del /q "\\?\%CD%\nul" 2>nul

rem PC build: CMake/Ninja trees for every variant
if exist port\build rmdir /s /q port\build

rem PSX build debris: generated info.mip trees and slink reports
if exist source\system\DEBUG rmdir /s /q source\system\DEBUG
if exist source\system\FINAL rmdir /s /q source\system\FINAL
if exist stats.txt del /q stats.txt
if exist statcov.txt del /q statcov.txt

echo Build outputs cleaned.
endlocal & exit /b 0
