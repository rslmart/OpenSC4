#include "uint32_getter_fragment.h"

#include <array>
#include <bit>
#include <cstdint>
#include <cstdio>
#if defined(__x86_64__)
#include <xmmintrin.h>
#endif

using opensc4::property_scalar::ReadStatus;
using opensc4::property_scalar::ReadUint32;
using opensc4::property_scalar::Scalar32;

namespace {
unsigned failures = 0;
std::uint64_t checks = 0;
void Check(std::uint16_t tag, std::uint32_t input, std::uint32_t expected) {
    std::uint32_t output = 0xdeadbeefu;
    const auto status = ReadUint32(Scalar32{tag, input}, output);
    ++checks;
    if (status != ReadStatus::Value || output != expected) {
        if (failures < 12)
            std::printf("FAIL tag=%x input=%08x expected=%08x actual=%08x status=%d\n",
                        unsigned(tag), input, expected, output, int(status));
        ++failures;
    }
}
#if defined(__x86_64__)
std::uint32_t SSEOracle(std::uint32_t bits) {
    const float input = std::bit_cast<float>(bits);
    std::int64_t converted;
    // Independent hardware instruction, including masked-invalid indefinite.
    asm volatile("cvttss2si %1, %0" : "=r"(converted) : "x"(input));
    return static_cast<std::uint32_t>(converted);
}
#endif
}

int main() {
    // Fixed bit expectations: no out-of-range C++ float casts in the oracle.
    struct Case { std::uint32_t input, expected; };
    constexpr Case cases[] = {
        {0x00000000u, 0}, {0x80000000u, 0},
        {0x00000001u, 0}, {0x80000001u, 0},
        {0x007fffffu, 0}, {0x807fffffu, 0},
        {0x00800000u, 0}, {0x80800000u, 0},
        {0x3f000000u, 0}, {0xbf000000u, 0},
        {0x3f7fffffu, 0}, {0xbf7fffffu, 0},
        {0x3f800000u, 1}, {0xbf800000u, 0xffffffffu},
        {0x3f800001u, 1}, {0xbf800001u, 0xffffffffu},
        {0x3fc00000u, 1}, {0xbfc00000u, 0xffffffffu},
        {0x3fffffffu, 1}, {0xbfffffffu, 0xffffffffu},
        {0x40000000u, 2}, {0xc0000000u, 0xfffffffeu},
        // Float32 neighbors of +/-2^31 (spacing changes at the boundary).
        {0x4effffffu, 0x7fffff80u}, {0x4f000000u, 0x80000000u},
        {0x4f000001u, 0x80000100u}, {0xceffffffu, 0x80000080u},
        {0xcf000000u, 0x80000000u}, {0xcf000001u, 0x7fffff00u},
        // Neighbors of +/-2^32, including required 2^32+512 -> 512.
        {0x4f7fffffu, 0xffffff00u}, {0x4f800000u, 0},
        {0x4f800001u, 512}, {0xcf7fffffu, 256},
        {0xcf800000u, 0}, {0xcf800001u, 0xfffffe00u},
        // Neighbors of +/-2^63: valid multiples and invalid all have low32=0.
        {0x5effffffu, 0}, {0x5f000000u, 0}, {0x5f000001u, 0},
        {0xdeffffffu, 0}, {0xdf000000u, 0}, {0xdf000001u, 0},
        {0x7f7fffffu, 0}, {0xff7fffffu, 0},
        {0x7f800000u, 0}, {0xff800000u, 0},
        {0x7fc00000u, 0}, {0xffc12345u, 0},
        {0x7f800001u, 0}, {0xff800001u, 0},
        {0x7fffffffu, 0}, {0xffffffffu, 0},
    };
    for (const auto& test : cases) Check(0xa, test.input, test.expected);
    constexpr std::uint32_t integers[] = {
        0, 1, 0x7ffffffeu, 0x7fffffffu, 0x80000000u,
        0x80000001u, 0xfffffffeu, 0xffffffffu, 0xdeadbeefu,
    };
    for (auto tag : {std::uint16_t{6}, std::uint16_t{7}})
        for (auto bits : integers) Check(tag, bits, bits);
    // Entire adapter tag domain excluding the three supported tags.
    for (std::uint32_t tag = 0; tag <= 0xffffu; ++tag) {
        if (tag == 6 || tag == 7 || tag == 0xa) continue;
        std::uint32_t output = 0xa55a6996u;
        const auto status = ReadUint32(Scalar32{static_cast<std::uint16_t>(tag),
                                               0xdeadbeefu}, output);
        ++checks;
        if (status != ReadStatus::Unsupported || output != 0xa55a6996u) ++failures;
    }
#if defined(__x86_64__)
    const auto saved = _mm_getcsr();
    // Mask exceptions, RNE; also check both DAZ/FTZ settings, since subnormal
    // inputs truncate to zero in either environment in this output-bit scope.
    for (const unsigned control : {0x1f80u, 0x9fc0u}) {
        _mm_setcsr(control);
        for (const auto& test : cases) Check(0xa, test.input, SSEOracle(test.input));
        // Every exponent with discriminating mantissa edges and both signs.
        constexpr std::uint32_t fractions[] = {
            0, 1, 2, 0x003fffffu, 0x00400000u, 0x007ffffeu, 0x007fffffu,
        };
        for (std::uint32_t e = 0; e < 256; ++e)
            for (auto f : fractions)
                for (const auto sign : {0u, 0x80000000u}) {
                    const auto bits = sign | (e << 23u) | f;
                    Check(0xa, bits, SSEOracle(bits));
                }
        // Deterministic sample of raw payloads, independent of implementation.
        std::uint32_t state = 0x731295abu;
        for (unsigned i = 0; i < 1000000; ++i) {
            state ^= state << 13u;
            state ^= state >> 17u;
            state ^= state << 5u;
            Check(0xa, state, SSEOracle(state));
        }
    }
    _mm_setcsr(saved);
    std::puts("Independent CVTTSS2SI r64 oracle: two MXCSR environments");
#else
    std::puts("Independent SSE oracle unavailable on this architecture; fixed cases checked");
#endif
    std::printf("uint32 getter: %llu checks, %u failures\n",
                static_cast<unsigned long long>(checks), failures);
    return failures == 0 ? 0 : 1;
}
