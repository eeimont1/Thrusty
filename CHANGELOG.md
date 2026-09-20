# Changelog

## 1.0.0

First packaged public release, combining the 0.x steering features with repository and release infrastructure.

- T300-to-Xbox input bridge with calibrated steering, independent pedals, remappable buttons and POV diagonals.
- Xbox rumble conversion, 0–100% centering, assisted steering profile and near-center boost.
- Wheel-pulse and Xbox-rumble diagnostics, finite-duration effects and a global stop shortcut.
- Window scales to the available display at launch; smaller screens can reach all controls.
- Reject duplicate axis assignments, limit numeric input and report settings-save or launch failures.
- Installer detects a running Thrusty instance before upgrade/uninstall.
- Single-source version metadata, hash-verified dependencies, repeatable build/package scripts and GitHub Actions.
- Native Windows dependency/UI/installer smoke checks are provided for CI; physical hardware acceptance remains separately documented.

## Development history

- **0.4:** independent 0–100% near-center boost.
- **0.3:** full-range 0–100% centering and simulated power-steering response.
- **0.2:** adjustable spring reach and stronger centering buildup.
- **0.1:** first functional bridge and classic Windows control panel.
