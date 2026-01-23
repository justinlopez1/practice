#include <iostream>
#include <array>
#include <vector>
#include <algorithm>

// Velocity → Pressure chamber mapping (example assumed due to lack of specification)
// Units: velocity in m/s, pressure in bar
//
// Breakpoints (pressure is linearly interpolated between these points):
//
// velocity =   0   m/s  → pressure = 120  bar
// velocity =  50   m/s  → pressure = 200  bar
// velocity = 100   m/s  → pressure = 300  bar
// velocity = 200   m/s  → pressure = 450  bar
// velocity = 300   m/s  → pressure = 600  bar
// velocity = 400   m/s  → pressure = 720  bar
// velocity = 500   m/s  → pressure = 820  bar
// velocity = 600   m/s  → pressure = 900  bar
// velocity = 700   m/s  → pressure = 960  bar
// velocity = 800   m/s  → pressure = 1000 bar
// velocity = 900   m/s  → pressure = 1030 bar
// velocity = 1000  m/s  → pressure = 1050 bar
//
// Notes:
// - Pressure must change smoothly; no discontinuities allowed
// - For velocities between breakpoints, use linear interpolation
// - Clamp or fault if velocity is outside the defined range

struct Entry {
    int vel;
    int pressure;
};

std::array<Entry, 12> table {
    0, 120,
    50, 200,
    100, 300,
    200, 450,
    300, 600,
    400, 720,
    500, 820,
    600, 900,
    700, 960,
    800, 1000,
    900, 1030,
    1000, 1050
};

float velToPressure(int vel) {
    if (vel < table[0].vel) {
        return table[0].pressure;
    }
    if (vel > table.back().vel) {
        return table.back().pressure;
    }

    auto after = std::lower_bound(
        table.begin(),
        table.end(),
        vel,
        [](const Entry& a, int v) {
            return a.vel < v;
        }
    );

    if (after->vel == vel) return after->pressure;

    auto before = after-1;

    // interpolate using before and after
    // 100 -> 300
    // 200 -> 450
    // 120
    // (120 - 100) / (200 - 100) = .2
    // 300 + (.2*(450-300))
    float ratio = static_cast<float>((vel - before->vel)) / (after->vel- before->vel);
    return before->pressure + (ratio * (after->pressure - before->pressure));
}

int main() {

    std::cout << velToPressure(120) << std::endl;

    return 0;
}