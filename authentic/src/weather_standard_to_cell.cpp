#include "opensc4/weather_coordinates.h"

#include <limits>

namespace opensc4::weather_coordinates {

Status StandardCityCellToCell(const WeatherSimulatorSaveRecord& r,
                             std::int32_t x, std::int32_t z,
                             Cell& out) noexcept {
    const std::int32_t scaleX = r.cellsPerStandardCellX;
    const std::int32_t scaleZ = r.cellsPerStandardCellZ;
    // Adapter policy, not original-game branches: reject both zero divisors
    // and both signed-division overflow pairs before any division or output.
    // Native idiv @ 0x1000e48da / 0x1000e48e3 faults in those domains;
    // a second-axis native fault can follow an already-written X output.
    constexpr auto min = std::numeric_limits<std::int32_t>::min();
    if (scaleX == 0 || scaleZ == 0 ||
        (x == min && scaleX == -1) || (z == min && scaleZ == -1)) {
        return Status::unsupported;
    }

    // cSC4WeatherSimulator::StandardCityCellToCell @ 0x1000e48d0:
    // independent signed X/+0x70 and Z/+0x74 divisions truncate toward zero.
    // Record adapter only: no native object-layout or serialization claim.
    out.x = x / scaleX;
    out.z = z / scaleZ;
    return Status::applied;
}

} // namespace opensc4::weather_coordinates
