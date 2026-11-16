#pragma once
#include <bit>
#include <cstdint>

namespace opensc4::aging_math {
// AgeOccupantList @ 0x100099e35..0x100099e4f; request values
// @ 0x100099e9d..0x100099ebb. Arithmetic selection only: the full method
// repeats the comparison after lot lookup; that lifecycle is not modeled here.
enum class Request : unsigned { none = 0, lower = 2, upper = 4 };
inline Request select_request(float updated, float lower, float upper) {
    if (updated >= upper) return Request::upper;
    if (updated >= lower) return Request::lower;
    return Request::none;
}
} // namespace opensc4::aging_math

#include "opensc4/sse_float32.h"
namespace opensc4::aging_math {
// Output-bit contract: IEEE binary32, masked RNE, gradual underflow;
// no flags/traps, alternate rounding or FTZ/DAZ. Build without fast-math,
// with -ffp-contract=off. This fragment does not set the caller's FP environment.
namespace detail {
inline std::uint64_t round_right_even(std::uint64_t v, unsigned shift) {
    if (shift >= 64) return 0; // Here v has at most 53 significant bits.
    if (shift == 0) return v;
    const auto q = v >> shift;
    const auto remainder = v & ((std::uint64_t{1} << shift) - 1);
    const auto half = std::uint64_t{1} << (shift - 1);
    return q + (remainder > half || (remainder == half && (q & 1)));
}
} // namespace detail
// CVTSD2SS @ 0x100099d85: integer bit adaptation also defines x86 NaN
// narrowing on arm64: sign, high 23 payload bits, and forced quiet bit.
// Finite RNE conversion is done in bits to avoid out-of-range float casts.
inline float narrow_double(std::uint64_t raw) {
    const auto sign = static_cast<std::uint32_t>(raw >> 32) & 0x80000000u;
    const auto exponent = static_cast<unsigned>((raw >> 52) & 0x7ffu);
    const auto fraction = raw & 0x000fffffffffffffull;
    if (exponent == 0x7ffu)
        return std::bit_cast<float>(sign | 0x7f800000u |
            (fraction ? (static_cast<std::uint32_t>(fraction >> 29) | 0x00400000u) : 0u));
    if (exponent == 0) return std::bit_cast<float>(sign);
    const int e = static_cast<int>(exponent) - 1023;
    const auto significand = fraction | (std::uint64_t{1} << 52);
    if (e < -126) {
        const auto rounded = detail::round_right_even(significand, static_cast<unsigned>(-e - 97));
        return std::bit_cast<float>(sign | static_cast<std::uint32_t>(rounded));
    }
    auto rounded = detail::round_right_even(significand, 29);
    int target_e = e;
    if (rounded == (std::uint64_t{1} << 24)) { rounded >>= 1; ++target_e; }
    if (target_e > 127) return std::bit_cast<float>(sign | 0x7f800000u);
    return std::bit_cast<float>(sign | (static_cast<std::uint32_t>(target_e + 127) << 23) |
                               (static_cast<std::uint32_t>(rounded) & 0x007fffffu));
}
// AgeOccupantList @ 0x100099d22..0x100099da2. Explicit inputs are already
// resolved; no property lookup/default, capacity dispatch, or lifecycle here.
// Initial normal-global bits 00000000: initializer @ 0x10009b92f..0x10009b931.
inline float factor(std::uint32_t used_property_id, std::int32_t used,
                    std::uint32_t capacity, std::uint32_t count208,
                    std::uint64_t raw_float64_bits268,
                    std::uint32_t initial_factor_bits = 0) {
    float numerator;
    std::uint32_t denominator;
    if (used_property_id != 0) {
        if (capacity == 0 || used <= 0) return std::bit_cast<float>(initial_factor_bits);
        numerator = static_cast<float>(used); // CVTSI2SS ecx @ 0x100099d69
        denominator = capacity;
    } else {
        if (count208 == 0) return std::bit_cast<float>(initial_factor_bits);
        numerator = narrow_double(raw_float64_bits268);
        denominator = count208;
    }
    // Zero-extended uint32 -> signed64 CVTSI2SS @ 0x100099d8d..0x100099d92.
    return sse_float32::div(numerator, static_cast<float>(denominator));
}
// AgeOccupantList @ 0x100099df4..0x100099e1e. Callback bodies and runtime
// curves are caller-owned. Separate statements preserve call and operand order.
template<class FundingCurve, class FactorCurve>
inline float update_age(float funding, float multiplier, float factor_value,
                        float previous, FundingCurve&& funding_curve,
                        FactorCurve&& factor_curve) {
    const float funding_result = funding_curve(funding);
    const float product = sse_float32::mul(funding_result, multiplier);
    const float factor_result = factor_curve(factor_value);
    const float increment = sse_float32::mul(factor_result, product);
    return sse_float32::add(increment, previous);
}
} // namespace opensc4::aging_math
