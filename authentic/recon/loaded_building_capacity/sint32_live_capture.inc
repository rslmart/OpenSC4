// Bounded diagnostic: ORIGINAL cRZVariant::GetValSint32 @ 0x10004f5c8.
// Include at namespace scope after get_app; never calls it. Main-thread only.
// Local numeric fixture, not city/aging evidence or a Phase B substitution.
#include "sint32_live_cases.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <limits>
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <pthread.h>
#include <xmmintrin.h>
#if !defined(__x86_64__) || defined(__FAST_MATH__)
#error "Sint32 original capture requires strict-FP x86_64."
#endif

namespace sint32_live_capture {
// ABI reviewed in SINT32_GETTER_REVIEW.md: payload +0x08, flags +0x10,
// runtime type +0x12, count +0x14; getter @ 0x10004f5c8.
// Trivial POD only: never pass to ownership/deletion APIs.
struct alignas(8) NumericVariant {
    std::uint64_t vptr;
    std::uint64_t payload;
    std::uint16_t flags;
    std::uint16_t type;
    std::uint32_t count;
};
static_assert(sizeof(NumericVariant) == 24 && alignof(NumericVariant) == 8);
static_assert(std::is_trivial<NumericVariant>::value &&
              std::is_standard_layout<NumericVariant>::value);
static_assert(offsetof(NumericVariant, vptr) == 0x00);
static_assert(offsetof(NumericVariant, payload) == 0x08);
static_assert(offsetof(NumericVariant, flags) == 0x10);
static_assert(offsetof(NumericVariant, type) == 0x12);
static_assert(offsetof(NumericVariant, count) == 0x14);

// Diagnostic guards only; not reconstructed game behavior.
static bool read_bytes(std::uint64_t address, void* out, std::size_t size) {
    mach_vm_size_t actual = 0;
    return mach_vm_read_overwrite(mach_task_self(), address, size,
        reinterpret_cast<mach_vm_address_t>(out), &actual) == KERN_SUCCESS && actual == size;
}
struct MxcsrScope {
    unsigned saved = _mm_getcsr();
    MxcsrScope() { _mm_setcsr(0x1f80u); }
    ~MxcsrScope() { _mm_setcsr(saved); }
    void restore() const { _mm_setcsr(saved); }
};
} // namespace sint32_live_capture

static void sint32_capture_exec(void*) {
    using namespace sint32_live_capture;
    logf_("SINT32_BEGIN origin=ORIGINAL expected=%u mxcsr=00001f80",
          static_cast<unsigned>(sint32_live::kCount));
    if (!pthread_main_np()) {
        logf_("SINT32_FAIL reason=not_main_thread");
        return;
    }
    constexpr std::uint64_t getterAddress = 0x10004f5c8;
    constexpr std::uint64_t vptrAddress = 0x100a6a7b0;
    if (g_slide > std::numeric_limits<std::uint64_t>::max() - vptrAddress) {
        logf_("SINT32_FAIL reason=slide_overflow");
        return;
    }
    // Fresh --dump 0x10004f5c8:11:bytes; exact saved prologue.
    constexpr unsigned char expected[]{0x55,0x48,0x89,0xe5,0x41,0x56,
                                       0x53,0x48,0x83,0xec,0x20};
    unsigned char actual[sizeof(expected)]{};
    std::uint64_t addressPointBytes = 0;
    if (!read_bytes(getterAddress + g_slide, actual, sizeof(actual)) ||
        std::memcmp(expected, actual, sizeof(actual)) != 0 ||
        !read_bytes(vptrAddress + g_slide, &addressPointBytes, sizeof(addressPointBytes))) {
        logf_("SINT32_FAIL reason=address_guard");
        return;
    }
    // No invented vtable slot: bounded supported paths do not dispatch through
    // source vptr. Original temporary Destroy @ 0x10004e2a2 bypasses cleanup
    // for its numeric tags 7/8/9 (focused independent ABI review).
    using Getter = bool (*)(const void*, std::int32_t&);
    const auto getter = reinterpret_cast<Getter>(getterAddress + g_slide);
    MxcsrScope environment;
    if (_mm_getcsr() != 0x1f80u) {
        logf_("SINT32_FAIL reason=mxcsr_setup");
        return; // RAII restores on every normal/C++ exception exit.
    }
    unsigned completed = 0;
    try {
        for (std::size_t index = 0; index < sint32_live::kCount; ++index) {
            const auto input = sint32_live::CaseAt(index);
            NumericVariant source{};
            source.vptr = vptrAddress + g_slide;
            source.payload = input.bits;
            source.type = input.tag;
            unsigned char before[sizeof(source)];
            std::memcpy(before, &source, sizeof(source));
            std::int32_t output = 0x13579bdf;
            const bool ok = getter(&source, output);
            std::uint32_t outputBits = 0;
            std::memcpy(&outputBits, &output, sizeof(outputBits));
            const bool unchanged = std::memcmp(before, &source, sizeof(source)) == 0;
            logf_("SINT32_CASE index=%u tag=%04x bits=%08x ok=%u output=%08x signed=%d unchanged=%u",
                  static_cast<unsigned>(index), static_cast<unsigned>(input.tag),
                  static_cast<unsigned>(input.bits), static_cast<unsigned>(ok),
                  static_cast<unsigned>(outputBits), static_cast<int>(output),
                  static_cast<unsigned>(unchanged));
            ++completed;
            if (!ok || !unchanged) {
                logf_("SINT32_FAIL reason=case index=%u", static_cast<unsigned>(index));
                return;
            }
        }
    } catch (...) {
        // C++ exceptions only. A Mach fault is NOT caught; missing END is invalid.
        logf_("SINT32_FAIL reason=cpp_exception actual=%u", completed);
        return;
    }
    environment.restore(); // Restore and verify BEFORE emitting the completion marker.
    const unsigned restored = _mm_getcsr();
    if (restored != environment.saved) {
        logf_("SINT32_FAIL reason=mxcsr_restore");
        return;
    }
    logf_("SINT32_END complete=1 actual=%u mxcsr_saved=%08x mxcsr_restored=%08x restored=1",
          completed, environment.saved, restored);
}
