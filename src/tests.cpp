#include "mapping.h"
#include <cassert>
#include <iostream>
using namespace thrusty;
int main() {
    assert(steering(32767, 0, 32767, 65535, .03, 1, false) == 0);
    assert(steering(0, 0, 32767, 65535, .03, 1, false) == -1);
    assert(steering(65535, 0, 32767, 65535, .03, 1, false) == 1);
    assert(steering(32768, 0, 32767, 65535, .03, 1, false) == 0);
    assert(steering(0, 0, 32767, 65535, 0, 1, true) == 1);
    assert(steering(12, 12, 12, 12, 0, 1, false) == 0);
    assert(pedal(65535, 65535, 0, .02, false) == 0);
    assert(pedal(0, 65535, 0, .02, false) == 255);
    assert(pedal(100, 100, 65000, 0, false) == 0);
    assert(pedal(65000, 100, 65000, 0, false) == 255);
    assert(pedal(300, 300, 300, 0, false) == 0);
    assert(pedal(65535, 65535, 0, 0, true) == 255);
    assert(stick(-1) == -32768 && stick(1) == 32767 && stick(0) == 0);
    assert(pov(0) == 1 && pov(4500) == 9 && pov(9000) == 8 && pov(31500) == 5 && pov(0xffffffff) == 0);
    for (int i = 0; i < 10000; ++i) {
        assert(std::abs(force(1, 255, 255, i * .01, 35, 100, false)) <= 10000);
    }
    assert(force(1, 0, 0, 0, 0, 10, false) == -1000);
    assert(force(-1, 0, 0, 0, 0, 10, false) == 1000);
    assert(force(0, 0, 0, 10, 35, 0, false) == 0);
    assert(centerForce(.25, 25, 25) == -.25);
    assert(centerForce(.25, 25, 100) == -.0625);
    assert(centerForce(0, 60, 5) == 0);
    assert(centerForce(-.25, 25, 25) == .25);
    assert(force(.25, 0, 0, 0, 0, 25, true) == 2500);
    for (int i = 0; i <= 1000; ++i) {
        double x = i / 1000.0;
        assert(centerForce(x, 60, 25) <= 0);
        assert(std::abs(centerForce(x, 60, 25)) <= .60);
        assert(std::abs(centerForce(x, 60, 25) + centerForce(-x, 60, 25)) < 1e-12);
        if (i > 0)
            assert(centerForce(x, 60, 25) <= centerForce(x - .001, 60, 25) + 1e-12);
    }
    // Full requested driver torque remains attainable in both profiles.
    for (bool assisted : {false, true}) {
        assert(force(.25, 0, 0, 0, 0, 100, false, 25, assisted, 0) == -10000);
        assert(force(-.25, 0, 0, 0, 0, 100, false, 25, assisted, 0) == 10000);
        assert(force(.25, 0, 0, 0, 0, 100, true, 25, assisted, 0) == 10000);
        assert(force(.25, 0, 0, 0, 0, 0, false, 25, assisted, 0) == 0);
    }
    assert(std::abs(centerForce(.025, 100, 25, true)) < std::abs(centerForce(.025, 100, 25, false)));
    for (int i = 0; i <= 1000; i++) {
        double x = i / 1000.0;
        for (double speed : {-10., -1., 0., 1., 10.}) {
            double f = centerForce(x, 100, 25, true, speed);
            assert(f <= 0 && f >= -1);
            assert(std::abs(f + centerForce(-x, 100, 25, true, -speed)) < 1e-12);
            assert(centerForce(0, 100, 25, true, speed) == 0);
            assert(centerForce(x, 0, 25, true, speed) == 0);
        }
    }
    assert(centerForce(.1, 100, 25, true, 5) == centerForce(.1, 100, 25, true, 0));
    assert(centerForce(.1, 100, 25, true, -5) > centerForce(.1, 100, 25, true, 0));
    WheelMotion m;
    assert(m.sample(.2, 1) == 0);
    assert(m.sample(.21, 1.01) > 0);
    assert(m.sample(.5, 2) == 0);
    m.reset();
    assert(m.sample(-.5, 3) == 0);
    // Near-center boost is local, symmetric and monotonic, with no center kick.
    for (bool assisted : {false, true}) {
        for (int boost : {0, 40, 100}) {
            double last = 0;
            for (int i = 0; i <= 10000; i++) {
                double x = i / 10000.0;
                double f = centerForce(x, 100, 25, assisted, 0, boost);
                assert(f <= last + 1e-12);
                last = f;
                assert(f >= -1 && f <= 0);
                assert(std::abs(f + centerForce(-x, 100, 25, assisted, 0, boost)) < 1e-12);
                if (x >= .1)
                    assert(std::abs(f - centerForce(x, 100, 25, assisted, 0, 0)) < 1e-12);
            }
            assert(centerForce(0, 100, 25, assisted, 0, boost) == 0);
            assert(centerForce(.01, 0, 25, assisted, 0, boost) == 0);
            assert(force(.25, 0, 0, 0, 0, 100, false, 25, assisted, 0, boost) == -10000);
        }
        assert(std::abs(centerForce(.01, 30, 25, assisted, 0, 40)) >
               std::abs(centerForce(.01, 30, 25, assisted, 0, 0)));
        assert(std::abs(centerForce(.01, 30, 25, assisted, 0, 100)) >
               std::abs(centerForce(.01, 30, 25, assisted, 0, 40)));
    }
    std::cout << "PASS: calibration, inversion, deadzones, saturation, POV diagonals and force limits\n";
}
