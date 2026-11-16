#include "opensc4/weather_coordinates.h"
#include <cmath>
#include <limits>

#pragma STDC FENV_ACCESS ON

namespace opensc4::weather_coordinates {
static_assert(std::numeric_limits<float>::is_iec559 &&
              std::numeric_limits<float>::digits == 24 && sizeof(float) == 4);

Status PositionToCell(const WeatherSimulatorSaveRecord& r, float x, float z,
                      Cell& out) noexcept {
    // Adapter policy, not branches in the original @ 0x1000e487a.
    if (!std::isfinite(x) || !std::isfinite(z) ||
        !std::isfinite(r.cellWidthM) || !(r.cellWidthM > 0.0f) ||
        !std::isfinite(r.cellHeightM) || !(r.cellHeightM > 0.0f))
        return Status::unsupported;

    // Native divss @ 0x1000e487e (+0x58), then @ 0x1000e4889 (+0x5c).
    // Each division rounds to binary32; do not replace with reciprocals.
    const float cellX = x / r.cellWidthM;
    const float cellZ = z / r.cellHeightM;

    // Adapter policy: validate BOTH rounded quotients before any cast/write.
    // Original cvttss2si @ 0x1000e4883 / 0x1000e488e has excluded invalid
    // conversion cases; no claim is made about native partial writes there.
    if (!std::isfinite(cellX) || !std::isfinite(cellZ) ||
        cellX < -0x1p31f || cellX >= 0x1p31f ||
        cellZ < -0x1p31f || cellZ >= 0x1p31f)
        return Status::unsupported;

    // Native cvttss2si truncates toward zero; stores @ 0x1000e4887/0x1000e4892.
    out = Cell{static_cast<std::int32_t>(cellX), static_cast<std::int32_t>(cellZ)};
    return Status::applied;
}
} // namespace opensc4::weather_coordinates
