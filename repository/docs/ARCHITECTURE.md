# Architecture and force model

The Win32 UI owns the DirectInput device, ViGEm target and effect lifecycle. A 10 ms Windows timer polls input and submits an Xbox report; display meters refresh approximately every 80 ms. These are scheduling targets, not real-time guarantees.

| Direction | Processing |
| --- | --- |
| Input | DirectInput state → calibrated axes/deadzone/curve/buttons → 12-byte XUSB report → ViGEm target |
| Feedback | ViGEm callback → atomic motor strengths/timestamp → synthesized torque → DirectInput constant-force effect |

The callback never calls UI or wheel APIs. The main thread reads its atomics. Effect parameters carry a 100 ms duration and are refreshed while output is active. Closing, input loss, target-update failure, suspend and emergency stop stop output; resuming requires user action. A hard UI/driver hang can retain the last Xbox input until the process or target exits; there is no independent worker watchdog.

## Local centering

Physical position is normalized around the calibrated center, separately from game-axis inversion and response curves. Let `u` be absolute position divided by selected spring reach, clamped to 0–1.

Near-center boost warps only `u < 0.4`:

```text
u = u + 3 * boost * u * (1 - u / 0.4)^2
```

Here boost is 0–1. The warp is monotonic, starts at zero, and rejoins the original curve smoothly; maximum near-center slope is four times the baseline. The linear profile uses `u`. Assisted steering uses `0.35*u + 0.65*u*u`. Selected centering strength scales the result from 0 to 100%.

Filtered local wheel velocity trims at most 3% of selected strength during fast inward motion in assisted mode. This never reverses the restoring torque or adds outward-turn damping. The filter resets on stale samples/device changes. This is a tunable local approximation, not a hydraulic/electric steering or tire model.

## Rumble and limits

Large/small Xbox motor strengths drive 9 Hz and 23 Hz sine components at weights 0.65/0.35. Rumble and centering sum into a signed force capped at ±10,000 DirectInput units. App effect/device gain requests 100%; physical driver/firmware gains still apply. Initial output fades in over 500 ms. Unchanged rumble times out after five seconds even if a game intended a longer constant vibration.

Settings use a per-user INI file and migrate naturally: new keys default to their documented values; existing mappings and calibrated endpoints remain. UI edits are applied while stopped. Same-axis steering/pedal assignments are rejected. Numeric edits are limited to three characters and clamped to supported ranges.
