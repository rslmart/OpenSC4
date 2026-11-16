#include "float32_getter_fragment.h"

#include <cfenv>
#include <cstdio>
#include <initializer_list>

using opensc4::property_scalar::ReadFloat32Bits;
using opensc4::property_scalar::ReadStatus;
using opensc4::property_scalar::Scalar32;

// Fixed IEEE-754 binary32 results, independent of the implementation's casts.
// At 2^24, spacing is 2: +1 ties to even downward, +3 ties upward.
// At 2^31, spacing is 256: +128 ties downward, +384 ties upward.
struct Case {
    const char* label;
    std::uint16_t tag;
    std::uint32_t input;
    std::uint32_t expected;
};

[[gnu::noinline]] ReadStatus Invoke(const Scalar32& input,
                                   std::uint32_t& output) noexcept {
    return ReadFloat32Bits(input, output);
}

int main() {
    if (std::fegetround() != FE_TONEAREST) {
        std::fputs("FAIL: checks require default round-to-nearest-even\n", stderr);
        return 1;
    }
    constexpr Case cases[] = {
        {"float +zero", 0xa, 0x00000000, 0x00000000},
        {"float -zero", 0xa, 0x80000000, 0x80000000},
        {"float +min subnormal", 0xa, 0x00000001, 0x00000001},
        {"float -min subnormal", 0xa, 0x80000001, 0x80000001},
        {"float max subnormal", 0xa, 0x007fffff, 0x007fffff},
        {"float qNaN payload", 0xa, 0x7fc12345, 0x7fc12345},
        {"float negative qNaN", 0xa, 0xffc54321, 0xffc54321},
        {"float sNaN payload", 0xa, 0x7f812345, 0x7f812345},
        {"float negative sNaN", 0xa, 0xff812345, 0xff812345},
        {"float +infinity", 0xa, 0x7f800000, 0x7f800000},
        {"float -infinity", 0xa, 0xff800000, 0xff800000},
        {"float fraction", 0xa, 0x3eaaaaab, 0x3eaaaaab},
        {"uint zero", 6, 0x00000000, 0x00000000},
        {"uint one", 6, 0x00000001, 0x3f800000},
        {"uint 2^24-1", 6, 0x00ffffff, 0x4b7fffff},
        {"uint 2^24", 6, 0x01000000, 0x4b800000},
        {"uint 2^24+1", 6, 0x01000001, 0x4b800000},
        {"uint 2^24+2", 6, 0x01000002, 0x4b800001},
        {"uint 2^24+3", 6, 0x01000003, 0x4b800002},
        {"uint signed max", 6, 0x7fffffff, 0x4f000000},
        {"uint high bit", 6, 0x80000000, 0x4f000000},
        {"uint high bit+1", 6, 0x80000001, 0x4f000000},
        {"uint high bit+128", 6, 0x80000080, 0x4f000000},
        {"uint high bit+129", 6, 0x80000081, 0x4f000001},
        {"uint high bit+384", 6, 0x80000180, 0x4f000002},
        {"uint max", 6, 0xffffffff, 0x4f800000},
        {"sint zero", 7, 0x00000000, 0x00000000},
        {"sint one", 7, 0x00000001, 0x3f800000},
        {"sint minus one", 7, 0xffffffff, 0xbf800000},
        {"sint 2^24-1", 7, 0x00ffffff, 0x4b7fffff},
        {"sint 2^24", 7, 0x01000000, 0x4b800000},
        {"sint 2^24+1", 7, 0x01000001, 0x4b800000},
        {"sint 2^24+3", 7, 0x01000003, 0x4b800002},
        {"sint -(2^24-1)", 7, 0xff000001, 0xcb7fffff},
        {"sint -(2^24)", 7, 0xff000000, 0xcb800000},
        {"sint -(2^24+1)", 7, 0xfeffffff, 0xcb800000},
        {"sint -(2^24+3)", 7, 0xfefffffd, 0xcb800002},
        {"sint max", 7, 0x7fffffff, 0x4f000000},
        {"sint min", 7, 0x80000000, 0xcf000000},
        {"sint min+1", 7, 0x80000001, 0xcf000000},
    };
    unsigned checked = 0;
    unsigned failed = 0;
    for (const auto& test : cases) {
        std::uint32_t output = 0xa5c39e17;
        const auto status = Invoke({test.tag, test.input}, output);
        ++checked;
        if (status != ReadStatus::Value || output != test.expected) {
            ++failed;
            std::fprintf(stderr, "FAIL %s: status=%u bits=%08x expected=%08x\n",
                         test.label, static_cast<unsigned>(status), output,
                         test.expected);
        }
    }
    // Every excluded runtime tag class plus wide values that must not truncate
    // into a supported tag. These are adapter exclusions, not conversion-false.
    constexpr std::uint16_t unsupported[] = {
        0, 1, 2, 3, 4, 5, 8, 9, 0xb, 0xc, 0xd, 0xe, 0xf,
        0x1006, 0x1007, 0x100a, 0xffff
    };
    for (const auto tag : unsupported) {
        for (const auto payload : {0x00000000u, 0xffffffffu, 0x7f812345u}) {
            std::uint32_t output = 0xa5c39e17;
            const auto status = Invoke({tag, payload}, output);
            ++checked;
            if (status != ReadStatus::Unsupported || output != 0xa5c39e17) {
                ++failed;
                std::fprintf(stderr, "FAIL unsupported tag=%04x payload=%08x\n",
                             static_cast<unsigned>(tag), payload);
            }
        }
    }
    std::printf("Float32 getter: %u checks, %u failed\n", checked, failed);
    return failed == 0 ? 0 : 1;
}
