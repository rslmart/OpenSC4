#include "opensc4/weather_coordinates.h"

#include <cmath>
#include <limits>

#pragma STDC FENV_ACCESS ON

namespace opensc4::weather_coordinates {
static_assert(sizeof(float) == 4 && std::numeric_limits<float>::is_iec559);

Status CellToPosition(const WeatherSimulatorSaveRecord& r, std::int32_t x,
                      std::int32_t z, Position& out) noexcept {
    // Adapter policy only: native @ 0x1000e4896 has no width/domain guards.
    if (!std::isfinite(r.cellWidthM) || !(r.cellWidthM > 0.0f) ||
        !std::isfinite(r.cellHeightM) || !(r.cellHeightM > 0.0f))
        return Status::unsupported;

    // cvtsi2ss @ 0x1000e489a, mulss [rdi+0x58] @ 0x1000e489e.
    // Explicit binary32 materializations preserve conversion-before-product.
    volatile float cellX = static_cast<float>(x);
    volatile float worldX = cellX * r.cellWidthM;
    // cvtsi2ss @ 0x1000e48aa, mulss [rdi+0x5c] @ 0x1000e48ae.
    volatile float cellZ = static_cast<float>(z);
    volatile float worldZ = cellZ * r.cellHeightM;
    const float resultX = worldX;
    const float resultZ = worldZ;

    // Adapter policy: defer native stores @ 0x1000e48a3 / 0x1000e48b3
    // until both products are accepted. Rejection leaves both outputs intact.
    if (!std::isfinite(resultX) || !std::isfinite(resultZ))
        return Status::unsupported;
    out = Position{resultX, resultZ};
    return Status::applied;
}
} // namespace opensc4::weather_coordinates
