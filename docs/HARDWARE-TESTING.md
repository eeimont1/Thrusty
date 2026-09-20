# v1.0 hardware acceptance record

Environment: Windows 10 21H2 / T300 firmware / GeForce NOW / BeamNG.Drive/ 9/20/2026 .

| Check | Expected result | Recorded result |
| --- | --- | --- |
| Clean install | App starts; driver install stays separate | Pass |
| Upgrade from 0.4 | Calibration and settings retained; new app reports v1.0.0 | Pass |
| Window at 100%, 125%, 150% scaling | All controls and stop button visible | Pass |
| Input calibration | Center near zero; both pedals 0 released / 255 pressed | Pass |
| Buttons and POV | Each selected assignment and diagonal works | Pass, needs user edits |
| Direct wheel pulse | One-second pulse stops completely | Partial Functionality |
| Xbox rumble test | Incoming counter advances and effect plays | Partial Functionality |
| Feedback tuning | Linear/assisted, reach and boost act as documented | Pass |
| Return direction | Off-center pull is toward center; no center-force step | Pass |
| STOP ALL / global shortcut | Target output and active effects stop | Pass |
| Unplug / suspend / wake | Output stops; force does not automatically restart | Pass |
| GeForce NOW session | Controller detected, input usable, returned rumble recorded if supported | Pass |
| Close/reopen and uninstall | No stale virtual pad; settings retained; bus driver retained | Pass |
