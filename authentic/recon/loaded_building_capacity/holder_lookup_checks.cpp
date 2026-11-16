#include "holder_lookup_fragment.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstdio>
#include <span>
#include <vector>

using opensc4::property_lookup::Entry;
using opensc4::property_lookup::FindSorted;

int main() {
    // Fixture values are test inputs, not reconstructed game constants.
    const std::array<int, 8> objects{};
    assert(FindSorted({}, 0) == nullptr);
    assert(FindSorted({}, UINT32_MAX) == nullptr);

    const std::array<Entry, 1> single{{{7, &objects[0]}}};
    assert(FindSorted(single, 6) == nullptr);
    assert(FindSorted(single, 7) == &objects[0]);
    assert(FindSorted(single, 8) == nullptr);
    assert(FindSorted(std::span<const Entry>(single).first(0), 7) == nullptr);

    const std::array<Entry, 4> bounds{{
        {1, &objects[0]}, {3, &objects[1]},
        {0x7fffffffu, &objects[2]}, {0x80000000u, &objects[3]}
    }};
    assert(FindSorted(bounds, 1) == &objects[0]);
    assert(FindSorted(bounds, 0x80000000u) == &objects[3]);
    assert(FindSorted(bounds, 0) == nullptr);
    assert(FindSorted(bounds, 2) == nullptr);
    assert(FindSorted(bounds, 0x7ffffffeu) == nullptr);
    assert(FindSorted(bounds, 0x7fffffffu) == &objects[2]);
    assert(FindSorted(bounds, 0x80000001u) == nullptr);
    assert(FindSorted(bounds, UINT32_MAX) == nullptr);

    const std::array<Entry, 8> repeated{{
        {0, &objects[0]}, {0, &objects[1]},
        {0x7fffffffu, nullptr}, {0x7fffffffu, &objects[2]},
        {0x80000000u, &objects[3]}, {0x80000000u, &objects[4]},
        {UINT32_MAX, nullptr}, {UINT32_MAX, &objects[5]}
    }};
    assert(FindSorted(repeated, 0) == &objects[0]);
    assert(FindSorted(repeated, 0x7fffffffu) == nullptr);
    assert(FindSorted(repeated, 0x80000000u) == &objects[3]);
    assert(FindSorted(repeated, UINT32_MAX) == nullptr);
    const std::array<Entry, 2> maximum{{
        {UINT32_MAX, &objects[6]}, {UINT32_MAX, &objects[7]}
    }};
    assert(FindSorted(maximum, UINT32_MAX) == &objects[6]);

    // Independent linear first-equal oracle across odd/even lengths, duplicate
    // runs and midpoint transitions. Every generated vector is sorted.
    std::size_t comparisons = 0;
    for (std::size_t length = 0; length <= 128; ++length) {
        std::vector<Entry> entries;
        for (std::size_t i = 0; i < length; ++i) {
            entries.push_back({static_cast<std::uint32_t>((i / 3) * 2),
                               i % 5 == 0 ? nullptr : &objects[i % objects.size()]});
        }
        for (std::uint32_t key = 0; key <= 90; ++key) {
            const void* expected = nullptr;
            for (const auto& entry : entries) {
                if (entry.key == key) {
                    expected = entry.property;
                    break;
                }
            }
            assert(FindSorted(entries, key) == expected);
            ++comparisons;
        }
    }
    std::printf("holder lookup: boundary checks and %zu linear-oracle comparisons passed\n",
                comparisons);
}
