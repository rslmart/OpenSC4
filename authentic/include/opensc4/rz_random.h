#pragma once

#include "opensc4/gzcom.h"

namespace opensc4 {

// cRZRandomSubclass serialized state (operator>> @ 0x1000ce8b3 /
// cRZRandomSubclass::Read @ 0x1000ce8d2, Write @ 0x1000ce948: 20 bytes).
struct RZRandomState {
    Uint32 seedA = 0;      // +0x00
    Uint32 seedB = 0;      // +0x04
    Sint32 field08 = 0;    // +0x08: nonzero when field10 contains a cached Gaussian
    Float64 field10 = 0.0; // +0x10: cached Gaussian value
};

Uint32 RandomUint32Uniform(RZRandomState& rng, Uint32 upperBound) noexcept;
Float64 RandomDoubleUniform(RZRandomState& rng) noexcept;
Float64 RandomDoubleGaussian(RZRandomState& rng) noexcept;

} // namespace opensc4
