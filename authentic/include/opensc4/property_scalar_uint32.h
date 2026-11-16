#pragma once

#include "opensc4/property_scalar_float32.h"

#include <cstdint>

namespace opensc4::property_scalar {

// Output-bit emulation of CVTTSS2SI r64 @ 0x1000503c8 followed by the
// low-word store @ 0x10004f001. Masked exceptions only; flags/traps are out
// of scope. Integer decoding inspects raw NaN bits without FP operations
// and avoids cast UB; the integer result does not retain NaN payloads.
inline std::uint32_t Float32ToUint32LowWord(std::uint32_t bits) noexcept {
    const std::uint32_t exponent = (bits >> 23u) & 0xffu;
    if (exponent < 127u || exponent >= 190u) {
        // Magnitudes below one truncate to zero. At/above 2^63 and for
        // NaN/inf, the signed64 indefinite value has a zero low word;
        // the one valid endpoint (-2^63) has the same zero low word.
        return 0u;
    }
    const unsigned shiftExponent = exponent - 127u;
    const std::uint64_t significand = (bits & 0x007fffffu) | 0x00800000u;
    const std::uint64_t magnitude = shiftExponent >= 23u
        ? significand << (shiftExponent - 23u)
        : significand >> (23u - shiftExponent);
    const std::uint32_t low = static_cast<std::uint32_t>(magnitude);
    return (bits & 0x80000000u) != 0u ? std::uint32_t{0} - low : low;
}

// cRZVariant::GetValUint32 @ 0x10004f4d2, scoped to runtime tags 6/7/A.
inline ReadStatus ReadUint32(const Scalar32& source, std::uint32_t& output) noexcept {
    switch (source.runtimeTag) {
    case 0x6:
        // Same-tag raw copy @ 0x10004f4e9..0x10004f4ee.
        output = source.bits;
        return ReadStatus::Value;
    case 0x7:
        // MOVSXD @ 0x1000503a0 then low32 store @ 0x10004f001:
        // sign extension leaves the original low word unchanged.
        output = source.bits;
        return ReadStatus::Value;
    case 0xa:
        output = Float32ToUint32LowWord(source.bits);
        return ReadStatus::Value;
    default:
        // Adapter-domain result; deliberately leaves output untouched.
        return ReadStatus::Unsupported;
    }
}

} // namespace opensc4::property_scalar
