#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
python3 scripts/version.py
python3 scripts/verify_vendor.py
mkdir -p build bin dist
g++ -std=c++17 -O2 -Wall -Wextra -Werror src/tests.cpp -o build/mapping-tests
./build/mapping-tests
x86_64-w64-mingw32-g++ -std=c++17 -O2 -static src/tests.cpp -o bin/mapping-tests.exe
x86_64-w64-mingw32-windres -I src src/thrusty.rc -o build/thrusty-resource.o
x86_64-w64-mingw32-g++ -std=c++17 -O2 -Wall -Wextra -Werror -Wno-cast-function-type -static -municode -mwindows src/thrusty.cpp build/thrusty-resource.o -o bin/Thrusty.exe -ldinput8 -ldxguid -lcomctl32 -lole32 -lshell32 -lwinmm -lmsimg32 -luuid
cp ViGEmClient.dll bin/ViGEmClient.dll
makensis -WX installer.nsi
python3 scripts/package.py
