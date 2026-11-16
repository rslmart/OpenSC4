#include "opensc4/weather_coordinates.h"

#include <array>
#include <cstdio>
#include <cstring>
#include <limits>

namespace {
using opensc4::WeatherSimulatorSaveRecord;
using opensc4::weather_coordinates::Cell;
using opensc4::weather_coordinates::StandardCityCellToCell;
using opensc4::weather_coordinates::Status;
constexpr auto lo = std::numeric_limits<std::int32_t>::min();
constexpr auto hi = std::numeric_limits<std::int32_t>::max();
constexpr Cell sentinel{123456789, -987654321};

struct Case {
    const char* name;
    std::int32_t x, z, scaleX, scaleZ;
    Status status;
    Cell expected;
};

// Fixed quotient fixtures for idiv semantics @ 0x1000e48d0; no computed oracle.
// Unsupported fixtures test the adapter policy, never native fault execution.
constexpr std::array<Case, 24> cases{{
    {"asymmetric axes", 53, -89, 7, 9, Status::applied, {7, -9}},
    {"negative exact", -54, 90, 6, 10, Status::applied, {-9, 9}},
    {"negative nonmultiples", -55, -91, 6, 10, Status::applied, {-9, -9}},
    {"negative scales", 55, 91, -6, -10, Status::applied, {-9, -9}},
    {"two negative operands", -55, -91, -6, -10, Status::applied, {9, 9}},
    {"zero inputs", 0, 0, -7, 11, Status::applied, {0, 0}},
    {"truncate through zero", -1, 1, 3, -2, Status::applied, {0, 0}},
    {"unit scales", 1, -1, 1, -1, Status::applied, {1, 1}},
    {"limits identity", lo, hi, 1, 1, Status::applied, {lo, hi}},
    {"safe unit negation", hi, lo, -1, 1, Status::applied, {-2147483647, lo}},
    {"minimum halved", lo, lo, 2, -2, Status::applied, {-1073741824, 1073741824}},
    {"limit divisors exact", hi, lo, hi, lo, Status::applied, {1, 1}},
    {"limit divisors nonexact", lo, hi, hi, lo, Status::applied, {-1, 0}},
    {"limit divisors exchanged", hi, lo, lo, hi, Status::applied, {0, -1}},
    {"large negative divisors", hi, lo, -2147483647, lo, Status::applied, {-1, 1}},
    {"large mixed divisors", lo, hi, -2147483647, 1073741824, Status::applied, {1, 1}},
    {"X zero divisor", 53, -89, 0, 9, Status::unsupported, sentinel},
    {"Z zero divisor", 53, -89, 7, 0, Status::unsupported, sentinel},
    {"both zero divisors", 0, 0, 0, 0, Status::unsupported, sentinel},
    {"X overflow pair", lo, -89, -1, 9, Status::unsupported, sentinel},
    {"Z overflow pair", 53, lo, 7, -1, Status::unsupported, sentinel},
    {"both overflow pairs", lo, lo, -1, -1, Status::unsupported, sentinel},
    {"X overflow Z zero", lo, 0, -1, 0, Status::unsupported, sentinel},
    {"X zero Z overflow", 0, lo, 0, -1, Status::unsupported, sentinel},
}};

WeatherSimulatorSaveRecord MakeRecord() {
    WeatherSimulatorSaveRecord r;
    r.version = 1;
    r.memAddr = 0x12345678;
    r.gridCellsX = 31;
    r.gridCellsZ = -17;
    r.cellWidthM = 3.5f;
    r.cellHeightM = 7.25f;
    r.invCellWidth = -0.25f;
    r.invCellHeight = 0.5f;
    r.worldWidthM = 1024.0f;
    r.worldHeightM = 2048.0f;
    r.field78 = -101;
    r.field1D8 = 202;
    r.ambientWindSpeed = 9.5f;
    r.ambientWindDir = {0.25f, -0.75f};
    r.grid98 = {2, 3, {1, 19, 37, 55, 73, 255}};
    r.boolGridB0 = {1, 4, {1, 0, 1, 0}};
    r.moistureGrid138 = {2, 2, {201, 202, 203, 204}};
    r.moistureVarGrid150 = {3, 1, {9, 8, 7}};
    r.rectsC8 = {{{-11, 12, 13, -14}}, {{21, -22, 23, 24}}};
    r.randomIterator.indices = {3, 1, 4, 2};
    r.randomIterator.values = {-7, 8, 9, 10, 11};
    r.rngState.seedA = 0xabcdef01;
    r.rngState.seedB = 0x10203040;
    return r;
}
} // namespace

int RunWeatherStandardCityCellToCellChecks() {
    int failures = 0;
    for (const auto& c : cases) {
        auto r = MakeRecord();
        r.cellsPerStandardCellX = c.scaleX;
        r.cellsPerStandardCellZ = c.scaleZ;
        const auto before = r; // Deep copies preserve every dynamic payload.
        // Host object representation snapshot detects changes to all inline
        // fields/vector metadata; it is not a serialization or layout oracle.
        std::array<unsigned char, sizeof(r)> bytes{};
        std::memcpy(bytes.data(), &r, sizeof(r));
        Cell out = sentinel; // Distinct live storage, disjoint from the record.
        const auto status = StandardCityCellToCell(r, c.x, c.z, out);
        const bool unchanged = std::memcmp(bytes.data(), &r, sizeof(r)) == 0 &&
            r.grid98.cells == before.grid98.cells &&
            r.boolGridB0.cells == before.boolGridB0.cells &&
            r.moistureGrid138.cells == before.moistureGrid138.cells &&
            r.moistureVarGrid150.cells == before.moistureVarGrid150.cells &&
            r.rectsC8 == before.rectsC8 &&
            r.randomIterator.indices == before.randomIterator.indices;
        if (status != c.status || out.x != c.expected.x ||
            out.z != c.expected.z || !unchanged) {
            ++failures;
            std::fprintf(stderr, "FAIL StandardCityCellToCell: %s "
                         "status=%d out=(%d,%d) record_unchanged=%d\n",
                         c.name, static_cast<int>(status), out.x, out.z,
                         static_cast<int>(unchanged));
        }
    }
    std::printf("StandardCityCellToCell: %zu cases, %d failures\n",
                cases.size(), failures);
    return failures;
}

#ifdef OPENSC4_WEATHER_COORDINATES_STANDALONE
int main() { return RunWeatherStandardCityCellToCellChecks(); }
#endif
