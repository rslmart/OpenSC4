#pragma once
#include <bit>
#include <cstdint>
#include <limits>

namespace opensc4::sse_float32 {
// Operand-ordered binary32 arithmetic for original SSE instructions:
// curve SUBSS @ 0x1000ac2b8/0x1000ac2d9/0x1000ac55c,
// DIVSS @ 0x1000ac2f5, MULSS @ 0x1000ac562, ADDSS @ 0x1000ac568.
// Validated boundary: masked exceptions, round-to-nearest, gradual underflow.
// Exception flags/traps, alternate rounding, FTZ/DAZ are not modeled here.
// Compile without fast-math and with -ffp-contract=off.
static_assert(std::numeric_limits<float>::is_iec559 && sizeof(float) == 4);
enum class Operation { Sub, Mul, Add, Div };
inline bool is_nan_bits(std::uint32_t bits) noexcept {
    return (bits & 0x7fffffffu) > 0x7f800000u;
}
template<Operation op>
inline float arithmetic(float destination, float source) noexcept {
    const auto a = std::bit_cast<std::uint32_t>(destination);
    const auto b = std::bit_cast<std::uint32_t>(source);
    // SSE keeps the destination NaN when both operands are NaNs, even if
    // only the source is signaling. Quiet without losing sign or payload.
    if (is_nan_bits(a)) return std::bit_cast<float>(a | 0x00400000u);
    if (is_nan_bits(b)) return std::bit_cast<float>(b | 0x00400000u);
    // One separately rounded binary32 operation; never a contracted multiply-add.
    float result;
    if constexpr (op == Operation::Sub) result = destination - source;
    if constexpr (op == Operation::Mul) result = destination * source;
    if constexpr (op == Operation::Add) result = destination + source;
    if constexpr (op == Operation::Div) result = destination / source;
    // No input NaNs remain: a NaN here is SSE's generated negative indefinite.
    if (is_nan_bits(std::bit_cast<std::uint32_t>(result)))
        return std::bit_cast<float>(0xffc00000u);
    return result;
}
inline float sub(float a, float b) noexcept { return arithmetic<Operation::Sub>(a,b); }
inline float mul(float a, float b) noexcept { return arithmetic<Operation::Mul>(a,b); }
inline float add(float a, float b) noexcept { return arithmetic<Operation::Add>(a,b); }
inline float div(float a, float b) noexcept { return arithmetic<Operation::Div>(a,b); }
} // namespace opensc4::sse_float32
