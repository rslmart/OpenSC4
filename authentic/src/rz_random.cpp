#include "opensc4/rz_random.h"

#include <bit>
#include <cmath>
#include <ctime>

namespace opensc4 {

// Exact cRZRandom::RandomUint32Uniform @ 0x10003d366.
Uint32 RandomUint32Uniform(RZRandomState& rng, Uint32 upperBound) noexcept {
    if (upperBound == 0u) {
        return 0u;
    }

    // The binary shifts the full 64-bit sum, not the truncated stored seed.
    const Uint64 next = static_cast<Uint64>(rng.seedA) * 0x41c64e6dULL + 0x3039ULL;
    rng.seedA = static_cast<Uint32>(next);
    const Uint32 randomBits = static_cast<Uint32>(next >> 16);
    return static_cast<Uint32>(
        (static_cast<Uint64>(upperBound) * randomBits) >> 32);
}

// Exact cRZRandom::RandomDoubleUniform @ 0x10003d500. A zero seed is
// initialized through the original _clock call stub @ 0x10092aff4.
Float64 RandomDoubleUniform(RZRandomState& rng) noexcept {
    if (rng.seedB == 0u) {
        rng.seedB = static_cast<Uint32>(std::clock());
    }
    rng.seedB *= 0x278dde6du;

    // cvtsi2ss consumes the new state as signed int32; scaling and rounding
    // occur in Float32 before conversion to Float64 and the +0.5 adjustment.
    const Float32 scaled =
        static_cast<Float32>(std::bit_cast<Sint32>(rng.seedB)) * 0x1p-32f;
    return static_cast<Float64>(scaled) + 0.5;
}

// Exact cRZRandom::RandomDoubleGaussian @ 0x10003d540.
Float64 RandomDoubleGaussian(RZRandomState& rng) noexcept {
    if (rng.field08 != 0) {
        rng.field08 = 0;
        return rng.field10;
    }

    Float64 x = 0.0;
    Float64 y = 0.0;
    Float64 radiusSquared = 0.0;
    do {
        x = RandomDoubleUniform(rng);
        x += x;
        x += -1.0;
        y = RandomDoubleUniform(rng);
        y += y;
        y += -1.0;
        radiusSquared = y * y + x * x;
    } while (radiusSquared >= 1.0 || radiusSquared == 0.0);

    const Float64 scale = std::sqrt(
        (-2.0 * std::log(radiusSquared)) / radiusSquared);
    rng.field10 = x * scale;
    rng.field08 = 1;
    return y * scale;
}

} // namespace opensc4
