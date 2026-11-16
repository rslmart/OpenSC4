#include "opensc4/property_scalar_sint32.h"

#include <bit>
#include <cstdint>
#include <type_traits>
#if defined(__x86_64__)
#include <xmmintrin.h>
#endif
#if defined(OPENSC4_SINT32_GETTER_STANDALONE)
#include <cstdio>
#endif

namespace {
using opensc4::property_scalar::ReadSint32;
using opensc4::property_scalar::ReadStatus;
using opensc4::property_scalar::Scalar32;
using Check = void (*)(bool, const char*);
static_assert(noexcept(ReadSint32(Scalar32{}, std::declval<std::int32_t&>())));
static_assert(std::is_same_v<decltype(ReadSint32(Scalar32{},
                            std::declval<std::int32_t&>())), ReadStatus>);

struct Golden {
    std::uint32_t input, bits;
    std::int32_t value;
};
// Fixed expected signed values AND bits; no ReadUint32 expected-value oracle.
constexpr Golden floatCases[] = {
    {0x00000000u, 0, 0}, {0x80000000u, 0, 0},
    {0x00000001u, 0, 0}, {0x80000001u, 0, 0},
    {0x007fffffu, 0, 0}, {0x807fffffu, 0, 0},
    {0x00800000u, 0, 0}, {0x80800000u, 0, 0},
    {0x3f000000u, 0, 0}, {0xbf000000u, 0, 0},
    {0x3f7fffffu, 0, 0}, {0xbf7fffffu, 0, 0},
    {0x3f800000u, 1, 1}, {0xbf800000u, 0xffffffffu, -1},
    {0x3f800001u, 1, 1}, {0xbf800001u, 0xffffffffu, -1},
    {0x3fc00000u, 1, 1}, {0xbfc00000u, 0xffffffffu, -1},
    {0x3fffffffu, 1, 1}, {0xbfffffffu, 0xffffffffu, -1},
    {0x40000000u, 2, 2}, {0xc0000000u, 0xfffffffeu, -2},
    {0x4effffffu, 0x7fffff80u, 2147483520},
    {0x4f000000u, 0x80000000u, -2147483647 - 1},
    {0x4f000001u, 0x80000100u, -2147483392},
    {0xceffffffu, 0x80000080u, -2147483520},
    {0xcf000000u, 0x80000000u, -2147483647 - 1},
    {0xcf000001u, 0x7fffff00u, 2147483392},
    {0x4f7fffffu, 0xffffff00u, -256}, {0x4f800000u, 0, 0},
    {0x4f800001u, 512, 512}, {0xcf7fffffu, 256, 256},
    {0xcf800000u, 0, 0}, {0xcf800001u, 0xfffffe00u, -512},
    {0x5effffffu, 0, 0}, {0x5f000000u, 0, 0}, {0x5f000001u, 0, 0},
    {0xdeffffffu, 0, 0}, {0xdf000000u, 0, 0}, {0xdf000001u, 0, 0},
    {0x7f7fffffu, 0, 0}, {0xff7fffffu, 0, 0},
    {0x7f800000u, 0, 0}, {0xff800000u, 0, 0},
    {0x7fc00000u, 0, 0}, {0xffc12345u, 0, 0},
    {0x7f800001u, 0, 0}, {0xff800001u, 0, 0},
    {0x7fffffffu, 0, 0}, {0xffffffffu, 0, 0},
};
constexpr Golden integerCases[] = {
    {0, 0, 0}, {1, 1, 1},
    {0x7ffffffeu, 0x7ffffffeu, 2147483646},
    {0x7fffffffu, 0x7fffffffu, 2147483647},
    {0x80000000u, 0x80000000u, -2147483647 - 1},
    {0x80000001u, 0x80000001u, -2147483647},
    {0xfffffffeu, 0xfffffffeu, -2}, {0xffffffffu, 0xffffffffu, -1},
    {0xdeadbeefu, 0xdeadbeefu, -559038737},
};

void CheckValue(Check check, std::uint16_t tag, const Golden& golden) {
    Scalar32 source{tag, golden.input};
    std::int32_t output = 123456789;
    const auto status = ReadSint32(source, output);
    check(status == ReadStatus::Value, "Sint32 supported source returns Value");
    check(output == golden.value, "Sint32 signed value matches independent golden");
    check(std::bit_cast<std::uint32_t>(output) == golden.bits,
          "Sint32 output bits match independent golden");
    check(source.runtimeTag == tag && source.bits == golden.input,
          "Sint32 getter preserves borrowed source");
}
#if defined(__x86_64__)
std::uint32_t SSEOracle(std::uint32_t bits) {
    const float input = std::bit_cast<float>(bits);
    std::int64_t converted;
    // Independent hardware signed64 truncation, including masked-invalid.
    asm volatile("cvttss2si %1, %0" : "=r"(converted) : "x"(input));
    return static_cast<std::uint32_t>(converted);
}
void CheckSSE(Check check, std::uint32_t bits) {
    const std::uint32_t expectedBits = SSEOracle(bits);
    // Independent signed interpretation via representable int64 arithmetic.
    const std::int64_t expectedValue = expectedBits <= 0x7fffffffu
        ? std::int64_t{expectedBits} : std::int64_t{expectedBits} - 4294967296LL;
    CheckValue(check, 0xa, {bits, expectedBits, static_cast<std::int32_t>(expectedValue)});
}
#endif
} // namespace

// Callback is borrowed for this call only; no retained source/output references.
void RunSavedSint32Checks(void (*check)(bool, const char*)) {
    for (const auto& golden : floatCases) CheckValue(check, 0xa, golden);
    for (const auto& golden : integerCases) {
        CheckValue(check, 6, golden);
        CheckValue(check, 7, golden);
    }
    // Every excluded runtime tag, including DB codes which differ from tags.
    for (std::uint32_t tag = 0; tag <= 0xffffu; ++tag) {
        if (tag == 6 || tag == 7 || tag == 0xa) continue;
        Scalar32 source{static_cast<std::uint16_t>(tag), 0xdeadbeefu};
        std::int32_t output = -123456789;
        const auto status = ReadSint32(source, output);
        check(status == ReadStatus::Unsupported && output == -123456789
                  && source.runtimeTag == tag && source.bits == 0xdeadbeefu,
              "Sint32 adapter exclusion preserves output and source");
    }
#if defined(__x86_64__)
    const auto saved = _mm_getcsr();
    constexpr unsigned controls[] = {0x1f80u, 0x9fc0u};
    constexpr std::uint32_t fractions[] = {0, 1, 0x003fffffu, 0x00400000u, 0x007fffffu};
    constexpr std::uint32_t signs[] = {0, 0x80000000u};
    for (const auto control : controls) {
        _mm_setcsr(control); // Masked exceptions/RNE; DAZ+FTZ off, then on.
        for (const auto& golden : floatCases) CheckSSE(check, golden.input);
        for (std::uint32_t exponent = 0; exponent < 256; ++exponent)
            for (const auto fraction : fractions)
                for (const auto sign : signs)
                    CheckSSE(check, sign | (exponent << 23u) | fraction);
    }
    _mm_setcsr(saved);
    check(_mm_getcsr() == saved, "Sint32 SSE oracle restores caller MXCSR");
#endif
}

#if defined(OPENSC4_SINT32_GETTER_STANDALONE)
namespace {
std::uint64_t count = 0, failures = 0;
void StandaloneCheck(bool passed, const char* label) {
    ++count;
    if (!passed) {
        if (failures < 12) std::printf("FAIL: %s\n", label);
        ++failures;
    }
}
}
int main() {
    RunSavedSint32Checks(StandaloneCheck);
    std::printf("Sint32 getter: %llu checks, %llu failures; SSE oracle %s\n",
                static_cast<unsigned long long>(count),
                static_cast<unsigned long long>(failures),
#if defined(__x86_64__)
                "enabled (two MXCSR environments)"
#else
                "unavailable (fixed goldens only)"
#endif
    );
    return failures == 0 ? 0 : 1;
}
#endif
