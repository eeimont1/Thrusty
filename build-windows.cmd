@echo off
setlocal
cd /d "%~dp0"
where g++.exe >nul 2>&1
if errorlevel 1 (
 echo Install MSYS2 UCRT64 gcc, binutils, Python and NSIS, then run this
 echo from a terminal with C:\msys64\ucrt64\bin and NSIS on PATH.
 exit /b 1
)
python scripts\version.py
if errorlevel 1 exit /b 1
python scripts\verify_vendor.py
if errorlevel 1 exit /b 1
if not exist build mkdir build
if not exist bin mkdir bin
if not exist dist mkdir dist
g++ -std=c++17 -O2 -Wall -Wextra -Werror -static src\tests.cpp -o bin\mapping-tests.exe
if errorlevel 1 exit /b 1
bin\mapping-tests.exe
if errorlevel 1 exit /b 1
windres -I src src\thrusty.rc -o build\thrusty-resource.o
if errorlevel 1 exit /b 1
g++ -std=c++17 -O2 -Wall -Wextra -Werror -Wno-cast-function-type -static -municode -mwindows src\thrusty.cpp build\thrusty-resource.o -o bin\Thrusty.exe -ldinput8 -ldxguid -lcomctl32 -lole32 -lshell32 -lwinmm -lmsimg32 -luuid
if errorlevel 1 exit /b 1
copy /y ViGEmClient.dll bin\ViGEmClient.dll >nul
makensis -WX installer.nsi
if errorlevel 1 exit /b 1
python scripts\package.py
