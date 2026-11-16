#include "opensc4/weather_coordinates.h"

#include <bit>
#include <cstdint>

namespace opensc4::weather_coordinates {

Status CellToStandardCityCell(const WeatherSimulatorSaveRecord& record,
                             std::int32_t x,
                             std::int32_t z,
                             Cell& out) noexcept {
    // Exact cSC4WeatherSimulator::CellToStandardCityCell @ 0x1000e48ba.
    // Native instructions use inputX for both products; inputZ is ignored.
    // Unsigned products preserve the native low-32-bit IMUL wrap without
    // invoking signed-overflow undefined behavior.
    (void)z; // EDX is unused by the original body.
    const auto input_bits = std::bit_cast<std::uint32_t>(x);
    const auto standard_x_bits = input_bits *
        std::bit_cast<std::uint32_t>(record.cellsPerStandardCellX);
    const auto standard_z_bits = input_bits *
        std::bit_cast<std::uint32_t>(record.cellsPerStandardCellZ);
    out.x = std::bit_cast<std::int32_t>(standard_x_bits);
    out.z = std::bit_cast<std::int32_t>(standard_z_bits);
    return Status::applied;
}

} // namespace opensc4::weather_coordinates
