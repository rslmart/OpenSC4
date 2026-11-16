#include "opensc4/weather_coordinates.h"
#include <array>
#include <bit>
#include <cfenv>
#include <cstdio>
#include <cstring>
#include <limits>
#if defined(__x86_64__)
#include <xmmintrin.h>
#endif

#pragma STDC FENV_ACCESS ON

namespace {
// Test harness policy: establish masked RNE/gradual underflow, restore on exit.
struct FPEnvironment {
    std::fenv_t saved{};
    bool ready = false;
    bool savedValid = false;
#if defined(__aarch64__)
    unsigned long long control = 0;
#elif defined(__x86_64__)
    unsigned int control = 0;
#endif
    FPEnvironment() {
#if defined(__aarch64__)
        asm volatile("mrs %0, fpcr" : "=r"(control));
#elif defined(__x86_64__)
        control = _mm_getcsr();
#endif
        if (std::fegetenv(&saved) != 0) return;
        savedValid = true;
        ready = std::fesetenv(FE_DFL_ENV) == 0;
#if defined(__aarch64__)
        const unsigned long long defaults = 0;
        asm volatile("msr fpcr, %0" : : "r"(defaults));
#elif defined(__x86_64__)
        _mm_setcsr(0x1f80u);
#endif
    }
    ~FPEnvironment() {
        if (savedValid) std::fesetenv(&saved);
#if defined(__aarch64__)
        asm volatile("msr fpcr, %0" : : "r"(control));
#elif defined(__x86_64__)
        _mm_setcsr(control);
#endif
    }
};
float Bits(std::uint32_t bits) { return std::bit_cast<float>(bits); }
}

int RunWeatherPositionToCellChecks() {
    FPEnvironment environment;
    if (!environment.ready) {
        std::fprintf(stderr, "PositionToCell: FP setup failed\n");
        return 1;
    }
    using namespace opensc4;
    using namespace opensc4::weather_coordinates;
    int checks = 0, failures = 0, cases = 0;
    auto check = [&](bool passed, const char* name, const char* property) {
        ++checks;
        if (!passed) {
            ++failures;
            std::fprintf(stderr, "FAIL %s: %s\n", name, property);
        }
    };
    WeatherSimulatorSaveRecord r;
    r.memAddr = 0x13579bdfu;
    r.gridCellsX = -73;
    r.invCellWidth = 123.0f; // Deliberately unrelated to the width.
    r.invCellHeight = -987.0f;
    r.rectsC8.push_back({17, -19, 23, -29});
    const auto rects = r.rectsC8;
    auto run = [&](const char* name, float width, float height, float x, float z,
                   Status expected, std::int32_t wantX, std::int32_t wantZ) {
        ++cases;
        r.cellWidthM = width;
        r.cellHeightM = height;
        std::array<unsigned char, sizeof(r)> before{};
        std::memcpy(before.data(), &r, sizeof(r));
        Cell out{1234567, -7654321};
        const auto status = PositionToCell(r, x, z, out);
        check(status == expected, name, "status");
        check(out.x == wantX && out.z == wantZ, name, "entire output");
        check(std::memcmp(before.data(), &r, sizeof(r)) == 0 && r.rectsC8 == rects,
              name, "record and unrelated storage unchanged");
    };
    auto accept = [&](const char* name, float w, float h, float x, float z,
                      std::int32_t cx, std::int32_t cz) {
        run(name, w, h, x, z, Status::applied, cx, cz);
    };
    auto reject = [&](const char* name, float w, float h, float x, float z) {
        run(name, w, h, x, z, Status::unsupported, 1234567, -7654321);
    };
    // Fixed expected results for divss/cvttss2si @ 0x1000e487a.
    // Rejected cases below test adapter policy, not native return behavior.
    accept("negative fractions/asymmetric", 2.0f, 4.0f, -3.5f, -11.0f, -1, -2);
    accept("positive fractions/asymmetric", 4.0f, 2.0f, 11.0f, 3.5f, 2, 1);
    accept("signed zeros", 1.0f, 2.0f, Bits(0x80000000u), 0.0f, 0, 0);
    accept("fraction around zero", 1.0f, 1.0f, Bits(0xbf7fffffu), Bits(0x3f7fffffu), 0, 0);
    accept("integer boundary", 1.0f, 1.0f, Bits(0x3fffffffu), Bits(0x40000001u), 1, 2);
    // Exact rational magnitude 25165826/8388609 < 3 rounds to binary32 3.
    accept("binary32 rounding before truncation", Bits(0x3f800001u), Bits(0x3f800001u),
           Bits(0x40400001u), Bits(0xc0400001u), 3, -3);
    // Rounded reciprocal of 41 times 41 is 0x3f7fffff, unlike direct division.
    accept("division not reciprocal", 41.0f, 41.0f, 41.0f, -41.0f, 1, -1);
    accept("int32 boundaries", 1.0f, 1.0f, Bits(0xcf000000u), Bits(0x4effffffu),
           std::numeric_limits<std::int32_t>::min(), 2147483520);
    accept("swapped boundaries", 1.0f, 1.0f, Bits(0x4effffffu), Bits(0xcf000000u),
           2147483520, std::numeric_limits<std::int32_t>::min());
    accept("gradual subnormal division", Bits(1), Bits(2), Bits(3), Bits(5), 3, 2);
    accept("underflow signed tiny", 2.0f, 2.0f, Bits(1), Bits(0x80000001u), 0, 0);
    accept("largest equal finite", Bits(0x7f7fffffu), Bits(0x7f7fffffu),
           Bits(0x7f7fffffu), Bits(0xff7fffffu), 1, -1);
    reject("positive limit X", 1.0f, 1.0f, Bits(0x4f000000u), 5.0f);
    reject("positive limit Z preserves X", 1.0f, 1.0f, 5.0f, Bits(0x4f000000u));
    reject("below negative limit X", 1.0f, 1.0f, Bits(0xcf000001u), 5.0f);
    reject("below negative limit Z", 1.0f, 1.0f, 5.0f, Bits(0xcf000001u));
    reject("finite overflow X", Bits(1), 1.0f, Bits(0x7f7fffffu), 5.0f);
    reject("finite overflow Z", 1.0f, Bits(1), 5.0f, Bits(0xff7fffffu));
    for (const auto invalid : {0x7f800000u, 0xff800000u, 0x7fc01234u}) {
        reject("nonfinite X", 1.0f, 1.0f, Bits(invalid), 5.0f);
        reject("nonfinite Z", 1.0f, 1.0f, 5.0f, Bits(invalid));
    }
    for (const auto invalid : {0u, 0x80000000u, 0xbf800000u, 0x80000001u,
                              0x7f800000u, 0xff800000u, 0x7fc01234u}) {
        reject("invalid width", Bits(invalid), 1.0f, 5.0f, 7.0f);
        reject("invalid height", 1.0f, Bits(invalid), 5.0f, 7.0f);
    }
    std::printf("PositionToCell: %d cases, %d checks, %d failures\n", cases, checks, failures);
    return failures;
}

#ifdef OPENSC4_WEATHER_COORDINATES_STANDALONE
int main() { return RunWeatherPositionToCellChecks() == 0 ? 0 : 1; }
#endif
