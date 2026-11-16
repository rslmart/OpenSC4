#pragma once

#include <cstdint>
#include <cstring>

namespace opensc4::terrain_altitude {

// Exact cSTETerrain::LocationIsInBounds @ 0x100740e7a. The jb/setae
// comparisons reject unordered operands and include both world-size edges.
inline bool LocationIsInBounds(float x, float z, float width,
                               float height) noexcept {
    if (!(x >= 0.0f)) return false;
    if (!(z >= 0.0f)) return false;
    if (!(width >= x)) return false;
    return height >= z;
}

// Exact GetAltitude @ 0x10073896b..0x100738997. Input is cell-space;
// constants @ 0x100a158b4/0x100a158b8 have bits bf000000/4b400000.
inline std::int32_t FloatToVertexIndex(float cell) noexcept {
    float rounded = cell + -0.5f;
    rounded = rounded + 12582912.0f;
    std::uint32_t bits;
    std::memcpy(&bits, &rounded, sizeof(bits));
    const std::uint32_t lo = bits - 0x4b400000u;
    const std::uint32_t hi = bits + 0xb4c00001u;
    std::int32_t signedHi;
    std::memcpy(&signedHi, &hi, sizeof(hi));
    const float roundTrip = static_cast<float>(signedHi);
    const std::uint32_t result = cell != roundTrip ? lo : hi;
    std::int32_t signedResult;
    std::memcpy(&signedResult, &result, sizeof(result));
    return signedResult;
}

// Exact cSTETerrain::GetAltitude @ 0x10073894d..0x100738a78, after
// successful virtual bounds dispatch. Accessor adapts only grid storage;
// the standalone model and live oracle instantiate this same algorithm.
template <typename AltitudeAt>
inline float Interpolate(float x, float z, std::uint32_t maxX,
                         std::uint32_t maxZ, std::uint32_t stride,
                         AltitudeAt altitudeAt) noexcept {
    constexpr float inverseWidth = 0.0625f; // 0x100a158b0, bits 3d800000
    constexpr float cellWidth = 16.0f;      // 0x100a16904, bits 41800000
    std::uint32_t sx = static_cast<std::uint32_t>(
        FloatToVertexIndex(x * inverseWidth));
    std::uint32_t sz = static_cast<std::uint32_t>(
        FloatToVertexIndex(z * inverseWidth));
    // Unsigned cmova clamps @ 0x1007389d5 / 0x1007389da.
    if (sx > maxX) sx = maxX;
    if (sz > maxZ) sz = maxZ;
    const float v00 = altitudeAt(sz * stride + sx);
    const float fracX = x - static_cast<float>(sx) * cellWidth;
    const float fracZ = z - static_cast<float>(sz) * cellWidth;
    const float v11 = altitudeAt((sz + 1u) * stride + sx + 1u);
    // ucomiss/cmovae @ 0x100738a34..0x100738a3b: ordered >=.
    const std::uint32_t diagonal = fracZ >= fracX
        ? (sz + 1u) * stride + sx : sz * stride + sx + 1u;
    const float vDiag = altitudeAt(diagonal);
    // cmpnless imm06 @ 0x100738a41 means > OR unordered, NOT >=.
    // Equal fractions select fracZ for max and fracX for min, including -0.
    const bool unordered = fracX != fracX || fracZ != fracZ;
    const float maxF = fracX > fracZ || unordered ? fracX : fracZ;
    const float minF = fracZ < fracX || unordered ? fracZ : fracX;
    // Preserve sub/sub/mul/mul/add/mul/add @ 0x100738a60..0x100738a78.
    const float diffHi = v11 - vDiag;
    const float diffLo = vDiag - v00;
    float acc = diffLo * maxF;
    acc = acc + diffHi * minF;
    acc = acc * inverseWidth;
    return acc + v00;
}

} // namespace opensc4::terrain_altitude
