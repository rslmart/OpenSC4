#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace opensc4::property_lookup {

struct Entry {
    std::uint32_t key;
    const void* property;
};

// cSCPropertyHolder::GetProperty(unsigned int) const @ 0x100049fc4.
// Representation adaptation: indices replace the binary's 16-byte entry
// pointers, avoiding null-pointer subtraction for an empty span. For valid
// sorted vectors the nonnegative count makes the signed midpoint sequence
// (count + signbit(count)) >> 1 equivalent to count / 2.
// Entry::key is the lookup key; no property ID is read from the pointee.
inline const void* FindSorted(std::span<const Entry> entries,
                              std::uint32_t key) noexcept {
    std::size_t first = 0;
    std::size_t count = entries.size();
    while (count != 0) {
        const std::size_t mid = count / 2;
        const std::size_t next = first + mid + 1;
        count = count - 1 - mid;
        // Unsigned cmp/cmovae/cmovb @ 0x100049ffb..0x10004a002.
        if (entries[next - 1].key >= key) {
            count = mid;
        } else {
            first = next;
        }
    }
    if (first == entries.size()) return nullptr;
    if (entries[first].key != key) return nullptr;
    return entries[first].property; // First equal entry, including nullptr.
}

} // namespace opensc4::property_lookup
