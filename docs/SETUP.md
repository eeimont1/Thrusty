# Setup and troubleshooting

## Install or upgrade

Run `Thrusty-1.0.0-Setup.exe`; it installs for your Windows user and creates Start menu and desktop shortcuts. Upgrades preserve `%LOCALAPPDATA%\Thrusty\settings.ini` and the separately installed ViGEmBus driver.

For portable use, extract the full Windows ZIP. `Thrusty.exe` needs its neighboring `ViGEmClient.dll`; do not run it inside the ZIP or move the executable alone. Preferences still live in Local AppData.

Install the official [T300 driver](https://support.thrustmaster.com/en/product/t300rs-en/), make sure your wheel is in PC-mode, and let startup calibration finish. Use separate accelerator and brake axes. Install the bundled ViGEmBus from **Feedback & setup** if it is not already present; restart Windows if that installer requests it.

## Calibrate and map

On **Buttons & axes**, move one control at a time and watch the raw readings. Choose three distinct axes. Initial choices and button numbers are only defaults and can differ by pedal set or driver configuration.

Save, center the wheel and release the pedals. On **Drive**, click **Calibrate**, sweep left/right fully, press/release both pedals fully, then finish. Check center is near 0, and each trigger is 0 released / 255 pressed. Pedal direction is detected by calibration; normally leave pedal inversion off afterward.

Press wheel buttons and read their numbers on Drive, then choose their Xbox assignments. The first POV hat maps automatically to D-pad directions and diagonals. Changing axes resets the affected calibration. Recalibrate after changing the wheel's rotation range in the Thrustmaster panel. Thrusty does not configure firmware rotation or physical end stops.

## Feedback tests

1. Stop the bridge, enable wheel feedback, save, then use **Test wheel pulse**. Close the Thrustmaster test panel and other wheel apps first; Thrusty needs exclusive access for effects.
2. Start the bridge and click **Test Xbox rumble**. The incoming message counter should advance. This checks XInput → virtual controller → notification → wheel effect.
3. Start your GeForce NOW session with controller vibration enabled. Watch received messages. If both local tests work but the session sends nothing, the missing feedback is upstream of Thrusty.

For assisted steering, try centering 20–30%, reach 25%, and boost 40%. Adjust while stopped, save, restart. A smaller reach increases force sooner; boost firms only small corrections. Leave your wheel securely mounted and start low, especially after reversing force direction. **Ctrl + Alt + F12** stops all output.

## Common problems

| Symptom | What to check |
| --- | --- |
| No wheel found | USB/power, T300 driver and PC mode. Use Windows Game Controllers, close it, then Refresh. Only Thrustmaster devices are listed. |
| Busy wheel / no effects | Close wheel testers, other remappers and local racing games. Try input with feedback disabled to isolate exclusive-access problems. |
| ViGEmBus not ready | Complete the bundled installer, restart if requested, check Device Manager. Do not disable Windows security protections. |
| Double inputs / wrong pad | Disconnect unused controllers and close other remappers. Optional [HidHide](https://docs.nefarius.at/projects/HidHide/Simple-Setup-Guide/) can hide the physical wheel while allowing Thrusty; it is not bundled. |
| Very weak feedback | Check app strength/reach, near-center boost and the Thrustmaster master/constant-force settings. Full software commands cannot override firmware limits. |
| Wheel pulls outward | Stop, reverse force direction, and retest at low strength. |
| Strong pulse cuts out | Effects expire after 100 ms without refresh; driver/UI stalls can interrupt them. Unchanged rumble also times out after five seconds. |
| Wheel disconnected / PC woke | Click Refresh and Start manually; reconnection never automatically resumes force. |
| DLL missing | Re-extract the complete x64 package. Do not fetch substitute DLLs from generic DLL-download sites. |

## Logs, settings and removal

Open the diagnostic log from Feedback & setup. Settings and logs live in `%LOCALAPPDATA%\Thrusty`. Review logs before posting them publicly. The app does not transmit them.

Uninstall from Windows Apps. Preferences and ViGEmBus remain; remove that driver separately only if other controller tools do not need it. The app and installer are unsigned. Driver installation is a separate operation with its own Windows approval prompt.
