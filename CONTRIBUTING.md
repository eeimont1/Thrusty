# Contributing

Useful contributions include T300/pedal compatibility reports, reproducible bug reports, small fixes and tests for force or calibration edge cases.

Before a pull request, run the build or at least `src/tests.cpp`, follow `.clang-format`, and explain the user-visible change. Include Windows/driver/wheel details for hardware behavior. Changes to force math should test direction, symmetry, zero-center behavior, bounds and continuity where applicable.

Keep new dependencies explicit and licensed. Do not replace the vendored binaries without checking provenance, license and pinned hashes. Do not introduce automatic driver installation or automatic resumption of force after device loss.

Maintainer: Ethan Eimont. Use GitHub issues for normal bugs and feature requests; see SECURITY.md for sensitive reports. Be respectful, stay specific, and do not post personal information or unreviewed logs.
