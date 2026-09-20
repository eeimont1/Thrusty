# v1.0 hardware acceptance record

These checks require a Windows PC and the actual wheel. They are **not marked passed** by the release-preparation build. Record results here or in a GitHub issue before describing a configuration as verified.

Environment: Windows build / T300 firmware / driver version / pedal set / rotation / GeForce NOW client / game / test date.

| Check | Expected result | Recorded result |
| --- | --- | --- |
| Clean install | App starts; driver install stays separate | Not recorded |
| Upgrade from 0.4 | Calibration and settings retained; new app reports v1.0.0 | Not recorded |
| Window at 100%, 125%, 150% scaling | All controls and stop button visible | Not recorded |
| Input calibration | Center near zero; both pedals 0 released / 255 pressed | Not recorded |
| Buttons and POV | Each selected assignment and diagonal works | Not recorded |
| Direct wheel pulse | One-second pulse stops completely | Not recorded |
| Xbox rumble test | Incoming counter advances and effect plays | Not recorded |
| Feedback tuning | Linear/assisted, reach and boost act as documented | Not recorded |
| Return direction | Off-center pull is toward center; no center-force step | Not recorded |
| STOP ALL / global shortcut | Target output and active effects stop | Not recorded |
| Unplug / suspend / wake | Output stops; force does not automatically restart | Not recorded |
| GeForce NOW session | Controller detected, input usable, returned rumble recorded if supported | Not recorded |
| Close/reopen and uninstall | No stale virtual pad; settings retained; bus driver retained | Not recorded |

Start force testing at low strength. A wheel/control-panel gain setting can reduce actual torque even at 100% application strength. Full-load torque is not measured by these software tests.

Do not replace unrecorded rows with “passed” based only on a successful build or CI run. Distinguish app input failures, local driver failures and cloud rumble limitations when reporting results.
