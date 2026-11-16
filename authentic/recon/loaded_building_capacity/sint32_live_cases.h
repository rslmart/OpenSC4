#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace sint32_live {
// Diagnostic input corpus only; raw words are not expected conversion results.
// All words are exercised under each supported runtime tag (6, 7, A).
inline constexpr std::array<std::uint32_t, 60> kRawWords{{
    0x00000000u, 0x80000000u, 0x00000001u, 0x80000001u,
    0x007fffffu, 0x807fffffu, 0x00800000u, 0x80800000u,
    0x3effffffu, 0xbeffffffu, 0x3f000000u, 0xbf000000u,
    0x3f7fffffu, 0xbf7fffffu, 0x3f800000u, 0xbf800000u,
    0x3f800001u, 0xbf800001u, 0x3fc00000u, 0xbfc00000u,
    0x3fffffffu, 0xbfffffffu, 0x40000000u, 0xc0000000u,
    0x4effffffu, 0xceffffffu, 0x4f000000u, 0xcf000000u,
    0x4f000001u, 0xcf000001u, 0x4f7fffffu, 0xcf7fffffu,
    0x4f800000u, 0xcf800000u, 0x4f800001u, 0xcf800001u,
    0x5effffffu, 0xdeffffffu, 0x5f000000u, 0xdf000000u,
    0x5f000001u, 0xdf000001u, 0x7f7fffffu, 0xff7fffffu,
    0x7f800000u, 0xff800000u, 0x7fc00000u, 0xffc00000u,
    0x7f800001u, 0xff800001u, 0x7fffffffu, 0xffffffffu,
    0x7ffffffeu, 0x80000002u, 0x12345678u, 0x87654321u,
    0x01000001u, 0x81000001u, 0x4b800001u, 0xcb800001u
}};
inline constexpr std::array<std::uint16_t, 3> kTags{{0x6, 0x7, 0xa}};
inline constexpr std::size_t kCount = kRawWords.size() * kTags.size();
struct Case { std::uint16_t tag; std::uint32_t bits; };
constexpr Case CaseAt(std::size_t index) {
    return {kTags[index / kRawWords.size()], kRawWords[index % kRawWords.size()]};
}
static_assert(kCount == 180);
} // namespace sint32_live
