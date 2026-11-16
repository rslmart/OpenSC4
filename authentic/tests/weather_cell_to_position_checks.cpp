#include "opensc4/weather_coordinates.h"

#include <array>
#include <bit>
#include <cfenv>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

#pragma STDC FENV_ACCESS ON

namespace {
struct ScopedEnvironment {
    std::fenv_t saved{};
    bool captured = std::fegetenv(&saved) == 0;
    bool ready = captured && std::fesetenv(FE_DFL_ENV) == 0 &&
                 std::fesetround(FE_TONEAREST) == 0;
    ~ScopedEnvironment() { if (captured) std::fesetenv(&saved); }
};
float Float(std::uint32_t bits) { return std::bit_cast<float>(bits); }
std::uint32_t Bits(float value) { return std::bit_cast<std::uint32_t>(value); }
}

int RunWeatherCellToPositionChecks() {
    using namespace opensc4;
    using namespace opensc4::weather_coordinates;
    ScopedEnvironment environment;
    if (!environment.ready) {
        std::fprintf(stderr, "CellToPosition: FP environment setup failed\n");
        return 1;
    }
    int checks = 0, failures = 0;
    auto check = [&](bool ok, const char* name) {
        ++checks;
        if (!ok) { ++failures; std::fprintf(stderr, "FAIL: %s\n", name); }
    };
    WeatherSimulatorSaveRecord r;
    r.gridCellsX = -17;
    r.invCellWidth = Float(0x7fc01234);
    r.grid98.cells = {1, 2, 3};
    r.boolGridB0.cells = {4, 5};
    r.moistureGrid138.cells = {6, 7};
    r.moistureVarGrid150.cells = {8, 9};
    r.rectsC8 = {{{-1, 2, -3, 4}}};
    auto run = [&](std::int32_t x, std::int32_t z, std::uint32_t width,
                   std::uint32_t height, Status expected,
                   std::uint32_t expectedX, std::uint32_t expectedZ) {
        r.cellWidthM = Float(width);
        r.cellHeightM = Float(height);
        std::array<unsigned char, sizeof(r)> before{};
        std::memcpy(before.data(), &r, sizeof(r));
        const auto grid = r.grid98.cells, bools = r.boolGridB0.cells;
        const auto moisture = r.moistureGrid138.cells, variance = r.moistureVarGrid150.cells;
        const auto rects = r.rectsC8;
        Position out{Float(0x7fc12345), Float(0x80000000)};
        check(CellToPosition(r, x, z, out) == expected, "status");
        check(Bits(out.x) == expectedX && Bits(out.z) == expectedZ, "output bits");
        check(std::memcmp(before.data(), &r, sizeof(r)) == 0 &&
              grid == r.grid98.cells && bools == r.boolGridB0.cells &&
              moisture == r.moistureGrid138.cells && variance == r.moistureVarGrid150.cells &&
              rects == r.rectsC8, "record and backing storage unchanged");
    };
    // Fixed expected binary32 results for cvtsi2ss/mulss @ 0x1000e4896.
    struct Boundary { std::int32_t cell; std::uint32_t bits; };
    constexpr Boundary boundaries[] = {
        {16777215, 0x4b7fffff}, {16777216, 0x4b800000},
        {16777217, 0x4b800000}, {16777218, 0x4b800001},
        {16777219, 0x4b800002}, {16777220, 0x4b800002}
    };
    for (const auto b : boundaries) {
        run(b.cell, -b.cell, 0x3f800000, 0x3f800000, Status::applied,
            b.bits, b.bits | 0x80000000);
        run(-b.cell, b.cell, 0x3f800000, 0x3f800000, Status::applied,
            b.bits | 0x80000000, b.bits);
    }
    constexpr auto min = std::numeric_limits<std::int32_t>::min();
    constexpr auto max = std::numeric_limits<std::int32_t>::max();
    run(min, max, 0x3f800000, 0x3f800000, Status::applied, 0xcf000000, 0x4f000000);
    run(max, min, 0x3f800000, 0x3f800000, Status::applied, 0x4f000000, 0xcf000000);
    run(0, 0, 0x7f7fffff, 1, Status::applied, 0, 0);
    run(-3, 7, 0x40200000, 0x3e800000, Status::applied, 0xc0f00000, 0x3fe00000);
    // Widening 16777217*3 gives 0x4c400001 instead of 0x4c400000.
    // Rounding 16777219 to binary32 first gives 16777220, then *3.
    run(16777217, -16777219, 0x40400000, 0x40400000, Status::applied,
        0x4c400000, 0xcc400003);
    run(-16777217, 16777219, 0x40400000, 0x40400000, Status::applied,
        0xcc400000, 0x4c400003);
    run(1, -1, 1, 1, Status::applied, 1, 0x80000001);
    run(3, -2, 1, 1, Status::applied, 3, 0x80000002);
    run(1, -1, 0x7f7fffff, 0x7f7fffff, Status::applied, 0x7f7fffff, 0xff7fffff);
    // Product rounding: 3*(1+2^-23) is a tie, rounded to the even mantissa.
    run(3, -5, 0x3f800001, 0x3f800001, Status::applied, 0x40400002, 0xc0a00001);
    run(max, min, 0x6fffffff, 0x6fffffff, Status::applied, 0x7f7fffff, 0xff7fffff);
    auto reject = [&](std::int32_t x, std::int32_t z, std::uint32_t w, std::uint32_t h) {
        run(x, z, w, h, Status::unsupported, 0x7fc12345, 0x80000000);
    };
    // Rejection is adapter policy, not a native branch or return value.
    for (auto invalid : {0u, 0x80000000u, 0xbf800000u, 0x80000001u,
                         0x7f800000u, 0xff800000u, 0x7fc01234u, 0x7f800001u}) {
        reject(2, 3, invalid, 0x3f800000);
        reject(2, 3, 0x3f800000, invalid);
        reject(0, 0, invalid, invalid);
    }
    reject(max, 1, 0x70000000, 0x3f800000);
    reject(1, min, 0x3f800000, 0x70000000);
    reject(2, 1, 0x7f7fffff, 0x3f800000);
    reject(1, 2, 0x3f800000, 0x7f7fffff);
    reject(-2, -2, 0x7f7fffff, 0x7f7fffff);
    reject(max, min, 0x7f7fffff, 0x7f7fffff);
    std::printf("CellToPosition: %d checks, %d failures\n", checks, failures);
    return failures;
}

#ifdef OPENSC4_WEATHER_COORDINATES_STANDALONE
int main() { return RunWeatherCellToPositionChecks() == 0 ? 0 : 1; }
#endif
