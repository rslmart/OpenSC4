#pragma once

#include "opensc4/property_scalar_uint32.h"

#include <bit>
#include <cstdint>

namespace opensc4::property_scalar {

// cRZVariant::GetValSint32 @ 0x10004f5c8, scoped to runtime tags 6/7/A.
// Output bits only: masked FP exceptions; flags/traps are outside this adapter.
inline ReadStatus ReadSint32(const Scalar32& source, std::int32_t& output) noexcept {
    switch (source.runtimeTag) {
    case 0x7:
        // Same-tag raw copy @ 0x10004f5df..0x10004f5e4.
        output = std::bit_cast<std::int32_t>(source.bits);
        return ReadStatus::Value;
    case 0x6:
        // Zero-extend @ 0x1000503b3; target7 retains low32 @ 0x10004f001.
        output = std::bit_cast<std::int32_t>(source.bits);
        return ReadStatus::Value;
    case 0xa:
        // CVTTSS2SI r64 @ 0x1000503c8; low32 store @ 0x10004f001.
        // Reuse the independently approved output-bit conversion, including
        // low32 zero for masked-invalid NaN/inf/overflow, then interpret signed.
        output = std::bit_cast<std::int32_t>(Float32ToUint32LowWord(source.bits));
        return ReadStatus::Value;
    default:
        // Adapter-domain exclusion, not the original conversion-false path.
        // Unsupported inputs preserve caller output.
        return ReadStatus::Unsupported;
    }
}

} // namespace opensc4::property_scalar
