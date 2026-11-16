#pragma once
#include <bit>
#include <cstdint>
#include "opensc4/sse_float32.h"

namespace opensc4::lot_position {
struct PositionBits { std::uint32_t x, y, z; };
struct Cell { std::int32_t x, z; };
// Representation adaptation; masked exceptions, RNE, gradual underflow only.
// Flags/traps and FTZ/DAZ are outside this output-bit contract.
// Exact cSC4BuildingOccupant::GetPosition @ 0x10019b430..0x10019b46c.
inline PositionBits BuildingPosition(PositionBits min, PositionBits max) noexcept {
    using namespace sse_float32;
    const float half = std::bit_cast<float>(0x3f000000u); // raw @ 0x100a158bc
    return {std::bit_cast<std::uint32_t>(mul(add(std::bit_cast<float>(min.x),
                                               std::bit_cast<float>(max.x)), half)),
            min.y,
            std::bit_cast<std::uint32_t>(mul(add(std::bit_cast<float>(min.z),
                                               std::bit_cast<float>(max.z)), half))};
}
namespace detail {
// MOVD / LEA / ADD / CVTSI2SS / UCOMISS / CMOVNE / CMOVP @
// 0x10013110b..0x10013112b and 0x100131138..0x100131158.
inline std::int32_t select_cell(float scaled, float biased) noexcept {
    const auto bits = std::bit_cast<std::uint32_t>(biased);
    const auto lower = bits - 0x4b400000u;
    const auto incremented = bits + 0xb4c00001u;
    const auto candidate = std::bit_cast<std::int32_t>(incremented);
    const float converted = static_cast<float>(candidate);
    // Bit test avoids evaluating a signaling NaN in the host comparison.
    const bool equal = !sse_float32::is_nan_bits(std::bit_cast<std::uint32_t>(scaled))
                       && scaled == converted;
    return std::bit_cast<std::int32_t>(equal ? incremented : lower);
}
}
// Coordinate portion only, cSC4ZoneDeveloper::GetOccupantLot @
// 0x1001310e0..0x100131158. No occupant call or grid lookup adaptation here.
inline Cell OccupantCell(PositionBits position) noexcept {
    using namespace sse_float32;
    // Raw @ 0x100a158b0: 00 00 80 3d 00 00 00 bf 00 00 40 4b
    const float scale = std::bit_cast<float>(0x3d800000u);
    const float minus_half = std::bit_cast<float>(0xbf000000u);
    const float bias = std::bit_cast<float>(0x4b400000u);
    const float x = mul(std::bit_cast<float>(position.x), scale);
    const float xb = add(add(x, minus_half), bias);
    const auto xc = detail::select_cell(x, xb);
    const float z = mul(scale, std::bit_cast<float>(position.z));
    const float zb = add(add(minus_half, z), bias);
    return {xc, detail::select_cell(z, zb)};
}
} // namespace opensc4::lot_position
