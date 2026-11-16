#include "opensc4/weather_coordinates.h"

#include <cstdint>
#include <cstdio>
#include <limits>

namespace opensc4::weather_coordinates {
namespace {

bool check(const WeatherSimulatorSaveRecord& record, std::int32_t x,
           std::int32_t z, std::int32_t expected_x,
           std::int32_t expected_z) noexcept {
    Cell out{123, -456};
    const auto status = CellToStandardCityCell(record, x, z, out);
    return status == Status::applied && out.x == expected_x &&
           out.z == expected_z;
}

} // namespace

int RunWeatherCellToStandardCityCellChecks() {
    int failures = 0, checks = 0;
    WeatherSimulatorSaveRecord record{};
    record.cellsPerStandardCellX = 3;
    record.cellsPerStandardCellZ = -5;
    record.gridCellsX = 71;
    record.gridCellsZ = -93;
    record.cellWidthM = 4.25f;
    record.worldHeightM = 987.0f;
    const auto original = record;

    const auto expect = [&](bool ok) {
        ++checks;
        if (!ok) {
            ++failures;
            std::fprintf(stderr, "CellToStandardCityCell: failed check %d\n", checks);
        }
    };
    expect(check(record, 7, 91, 21, -35)); // Asymmetric axes: input Z is ignored.
    expect(check(record, 0, 99, 0, 0));
    expect(check(record, 1, -99, 3, -5));
    expect(check(record, -1, 99, -3, 5));
    expect(check(record, std::numeric_limits<std::int32_t>::min(), 12,
                 std::numeric_limits<std::int32_t>::min(),
                 std::numeric_limits<std::int32_t>::min()));
    expect(check(record, std::numeric_limits<std::int32_t>::max(), -12,
                 2147483645, -2147483643));

    record.cellsPerStandardCellX = std::numeric_limits<std::int32_t>::max();
    record.cellsPerStandardCellZ = std::numeric_limits<std::int32_t>::min();
    expect(check(record, 2, 123, -2, 0)); // Both products wrap in 32 bits.
    expect(check(record, -2, 123, 2, 0));
    expect(check(record, 1, 123, std::numeric_limits<std::int32_t>::max(),
                 std::numeric_limits<std::int32_t>::min()));

    record.cellsPerStandardCellX = 0;
    record.cellsPerStandardCellZ = -1;
    expect(check(record, std::numeric_limits<std::int32_t>::min(), 8, 0,
                 std::numeric_limits<std::int32_t>::min()));
    record.cellsPerStandardCellX = -1;
    record.cellsPerStandardCellZ = 0;
    expect(check(record, std::numeric_limits<std::int32_t>::min(), 8,
                 std::numeric_limits<std::int32_t>::min(), 0));

    // The first fixed fixture rejects the wrong-Z-input result (21, -455).
    expect(record.version == original.version && record.memAddr == original.memAddr &&
           record.gridCellsX == original.gridCellsX &&
           record.gridCellsZ == original.gridCellsZ &&
           record.cellWidthM == original.cellWidthM &&
           record.worldHeightM == original.worldHeightM);
    std::printf("CellToStandardCityCell: %d checks, %d failures\n", checks, failures);
    return failures;
}

} // namespace opensc4::weather_coordinates

#ifdef OPENSC4_WEATHER_COORDINATES_STANDALONE
int main() {
    return opensc4::weather_coordinates::RunWeatherCellToStandardCityCellChecks();
}
#endif
