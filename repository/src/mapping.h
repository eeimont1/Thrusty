#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
namespace thrusty {
inline double clamp(double x, double lo, double hi) {
    return std::max(lo, std::min(hi, x));
}
inline double steering(int raw, int lo, int mid, int hi, double dead, double curve, bool invert) {
    double span = raw >= mid ? hi - mid : mid - lo;
    double x = span > 100 ? (raw - mid) / span : 0;
    x = clamp(x, -1, 1);
    if (invert)
        x = -x;
    double a = std::abs(x);
    if (a <= dead)
        return 0;
    return std::copysign(std::pow(clamp((a - dead) / (1 - dead), 0, 1), curve), x);
}
inline uint8_t pedal(int raw, int rest, int full, double dead, bool invert) {
    if (std::abs(full - rest) < 100)
        return 0;
    double v = clamp(double(raw - rest) / (full - rest), 0, 1);
    if (invert)
        v = 1 - v;
    return uint8_t(std::lround(clamp((v - dead) / (1 - dead), 0, 1) * 255));
}
inline int16_t stick(double v) {
    return int16_t(std::lround(clamp(v, -1, 1) * (v < 0 ? 32768 : 32767)));
}
inline uint16_t pov(uint32_t angle) {
    if ((angle & 0xffff) == 0xffff || angle > 35999)
        return 0;
    const uint16_t values[] = {1, 1 | 8, 8, 8 | 2, 2, 2 | 4, 4, 4 | 1};
    return values[((angle + 2250) / 4500) % 8];
}
// All torque is synthesized locally; there is no vehicle-speed/tire telemetry.
// Velocity is normalized center-to-lock travel per second, filtered over 40 ms.
struct WheelMotion {
    double previous = 0, seconds = 0, velocity = 0;
    bool initialized = false;
    void reset() {
        initialized = false;
        velocity = 0;
    }
    double sample(double position, double now) {
        double dt = now - seconds;
        if (!initialized || dt <= 0 || dt > .1) {
            velocity = 0;
            initialized = true;
        } else {
            double raw = clamp((position - previous) / dt, -20, 20);
            velocity += (raw - velocity) * (1 - std::exp(-dt / .04));
        }
        previous = position;
        seconds = now;
        return velocity;
    }
};
// Full selected strength is reached at 'reach' percent of center-to-lock travel.
// Assisted mode has a light on-center zone and progressively firmer loading.
// Return control trims at most 3% of selected strength on fast inward motion;
// it never adds resistance while steering out, creates a static friction floor,
// or reverses the restoring torque. It is not a physical power-steering model.
inline double centerForce(double x, int spring, int reach, bool assisted = false, double velocity = 0,
                          int centerBoost = 0) {
    double u = clamp(std::abs(x) / (clamp(reach, 5, 100) / 100.0), 0, 1);
    // Boost only the first 40% of the spring reach; smoothly rejoin the
    // original curve afterward. At maximum boost, center slope is 4x.
    // The warp stays monotonic (its minimum derivative is zero at 100%).
    if (u < .4) {
        double t = u / .4;
        u += 3 * (clamp(centerBoost, 0, 100) / 100.0) * u * (1 - t) * (1 - t);
    }
    double shape = assisted ? (.35 * u + .65 * u * u) : u;
    double magnitude = shape * (clamp(spring, 0, 100) / 100.0);
    if (assisted && x * velocity < 0) {
        double trim = .03 * (spring / 100.0) * clamp((std::abs(velocity) - .3) / 2, 0, 1);
        magnitude = std::max(0.0, magnitude - trim);
    }
    return x == 0 ? 0 : -std::copysign(magnitude, x);
}
inline int force(double x, int low, int high, double seconds, int gain, int spring, bool reverse,
                 int reach = 25, bool assisted = false, double velocity = 0, int centerBoost = 0) {
    double rumble = .65 * (low / 255.0) * std::sin(seconds * 6.28318530718 * 9) +
                    .35 * (high / 255.0) * std::sin(seconds * 6.28318530718 * 23);
    double f = rumble * (gain / 100.0) + centerForce(x, spring, reach, assisted, velocity, centerBoost);
    return int(std::lround(clamp(f, -1, 1) * 10000)) * (reverse ? -1 : 1);
}
} // namespace thrusty
