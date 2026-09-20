# Credits and dependencies

**Thrusty** is maintained by **Ethan Eimont**. The application, documentation and original wheel icon are released under the MIT license in [LICENSE.txt](LICENSE.txt). Development used AI assistance; the verification status is documented in [VALIDATION.txt](VALIDATION.txt).

| Component | Purpose | License / source |
| --- | --- | --- |
| ViGEmClient x64 | Creates a virtual Xbox 360 target and receives rumble | [MIT](licenses/ViGEmClient-LICENSE.txt); [upstream](https://github.com/nefarius/ViGEmClient) |
| ViGEmBus 1.22.0 | Windows virtual-controller driver; installed separately | [BSD-3-Clause](licenses/ViGEmBus-LICENSE.txt); [official release](https://github.com/nefarius/ViGEmBus/releases/tag/v1.22.0) |
| MinGW-w64 / GCC | Builds the Windows executable with a static C++ runtime | GCC runtime library exception and applicable MinGW licenses; compiler packages are not bundled |
| NSIS | Builds the Windows installer | [NSIS license](https://nsis.sourceforge.io/License) |
| Microsoft DirectInput, XInput and Win32 | Windows input, feedback and user interface APIs | Supplied by Windows, not bundled |

The included ViGEmClient DLL was extracted from the [vgamepad 0.1.0 source distribution](https://pypi.org/project/vgamepad/0.1.0/); no Python runtime or vgamepad code is required to run Thrusty. Original dependency hashes and license notices are in [licenses/THIRD-PARTY.txt](licenses/THIRD-PARTY.txt); the build checks those hashes before packaging.

ViGEmBus and ViGEmClient are retired upstream projects. See the maintainer's [end-of-life notice](https://docs.nefarius.at/projects/ViGEm/End-of-Life/). Their inclusion does not imply continued upstream maintenance.

Thrusty is an independent community project. Thrustmaster, Xbox, Microsoft and NVIDIA GeForce NOW are referenced to describe compatibility; their owners do not sponsor or endorse Thrusty.
