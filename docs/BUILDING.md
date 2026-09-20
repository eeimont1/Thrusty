# Building Thrusty

The release binaries are already built; these steps are for contributors.

## Ubuntu 24.04 / WSL

```sh
sudo apt-get update
sudo apt-get install g++ g++-mingw-w64-x86-64-posix nsis python3
bash build.sh
```

The script verifies embedded dependencies, runs the portable math tests, cross-compiles the Windows app and native test executable, builds the installer, and packages releases under `dist/`.

## Windows with MSYS2

Install [MSYS2](https://www.msys2.org/) and [NSIS](https://nsis.sourceforge.io/Download). Install the UCRT64 compiler, binutils and Python through MSYS2. Put `C:\msys64\ucrt64\bin` and the NSIS installation directory on your command prompt's PATH, along with Python, then run:

```bat
build-windows.cmd
```

The command script fails on test/compiler/installer errors. No Visual Studio project or NuGet restore is required.

## Build layout

| Path | Purpose |
| --- | --- |
| `VERSION` | Single release-version input |
| `src/` | App, force/mapping math, resource icon and tests |
| `scripts/version.py` | Generates `src/version.h` and `version.nsh` |
| `build/` | Temporary object files and Linux tests |
| `bin/` | Windows app, runtime DLL and native mapping tests |
| `dist/` | Setup EXE, portable ZIP, source ZIP and checksums |
| `scripts/verify_vendor.py` | Hash-pinned embedded DLL/driver verification |
| `scripts/package.py` | Explicit source/runtime packaging |

Generated folders are ignored by git. The small ViGEm DLL and official driver installer are intentional, hash-pinned source-repository dependencies so the build does not fetch unversioned executables.

## Checks

```sh
# Linux math tests only
g++ -std=c++17 -O2 -Wall -Wextra -Werror src/tests.cpp -o /tmp/thrusty-tests
/tmp/thrusty-tests
```

On Windows, `bin\mapping-tests.exe` runs the same assertions. `bin\Thrusty.exe --self-test` checks DLL loading/exports, allocation, DirectInput creation and XInput availability without connecting a controller. `--ui-smoke` creates the normal UI without wheel enumeration, then closes itself. Exit code 0 is success; native dependency failures use codes 10–15.

The included Actions workflow builds on Ubuntu and runs the native checks plus installer/uninstaller smoke checks on a Windows runner. Those checks do **not** replace physical wheel testing. The local build uses warnings as errors; source formatting follows `.clang-format`.

Changing `VERSION` updates app metadata, installer metadata and output names. Also update README badges, changelog and the matching release-notes file. The workflow refuses to create a release when the tag disagrees with VERSION. Builds from different compiler/NSIS versions are not claimed to be byte-identical.
