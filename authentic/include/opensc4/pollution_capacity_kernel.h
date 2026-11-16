#pragma once
#include <cstdint>
#include <cstring>
#include <limits>

namespace opensc4::pollution_capacity {
static_assert(std::numeric_limits<float>::is_iec559 &&
              std::numeric_limits<float>::digits == 24, "requires IEEE binary32");
// cSC4PollutionSimulator::GetCurrentCapacity @ 0x1000992cc.
// Opaque adapter contexts; curve storage belongs to the caller, not this kernel.
struct State { void* context; void* curve_storage; };
struct Callbacks {
    void* (*get_holder)(void*, void* occupant);
    bool (*read_capacity)(void*, void* holder, std::uint32_t, std::uint32_t&);
    bool (*is_powered)(void*, void* occupant);
    float (*read_scale)(void*);
    void* (*lookup_curve)(void*, void* holder, std::uint32_t);
    void (*construct_curve)(void*, void* storage);
    void (*load_curve)(void*, void* storage, void* property);
    bool (*read_input)(void*, void* holder, std::uint32_t, float&);
    float (*evaluate_curve)(void*, void* storage, float);
    void (*destroy_curve)(void*, void* storage) noexcept;
};

// CVTTSS2SI r64 @ 0x100099344, then low32 store @ 0x100099349.
inline std::uint32_t truncate_low32(float value) noexcept {
    // Exact power-of-two bounds: +2^63 is invalid; -2^63 is valid.
    if (!(value >= -0x1p63f && value < 0x1p63f)) return 0;
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(value));
}

// Compile without fast-math and with -ffp-contract=off; preserve SSE float steps.
inline bool get_current_capacity(const State& state, const Callbacks& cb,
                                 std::uint32_t property, void* occupant,
                                 std::uint32_t& output) {
    void* holder = cb.get_holder(state.context, occupant);
    if (!cb.read_capacity(state.context, holder, property, output)) {
        output = 0; // 0x100099322
        return false;
    }
    float base;
    if (cb.is_powered(state.context, occupant)) {
        base = static_cast<float>(static_cast<std::uint64_t>(output));
    } else {
        output = 0; // 0x100099331: visible before scale read (including 0 * inf).
        base = 0.0f;
    }
    const float scale = cb.read_scale(state.context);
    const float scaled = base * scale;
    output = truncate_low32(scaled);
    holder = cb.get_holder(state.context, occupant); // 0x10009934c
    void* curve = cb.lookup_curve(state.context, holder, 0x8a1e07eeu);
    if (!curve) return true;
    cb.construct_curve(state.context, state.curve_storage); // 0x100099379
    struct Cleanup {
        const State& state;
        const Callbacks& cb;
        ~Cleanup() { cb.destroy_curve(state.context, state.curve_storage); }
    } cleanup{state, cb};
    cb.load_curve(state.context, state.curve_storage, curve); // 0x100099384
    float input = 0.0f;
    (void)cb.read_input(state.context, holder, 0xc910b4cfu, input);
    const std::uint32_t saved_capacity = output; // 0x1000993a0, BEFORE evaluate
    const float response = cb.evaluate_curve(state.context, state.curve_storage, input);
    const float capacity = static_cast<float>(static_cast<std::uint64_t>(saved_capacity));
    float product = response * capacity; // MULSS @ 0x1000993b8
    // SSE's masked invalid infinity*zero result is negative indefinite NaN
    // (0xffc00000). ARM64 generates a positive NaN instead; the MOVD/add at
    // 0x1000993c4/0x1000993c8 makes that sign observable. Preserve supplied
    // NaN payloads; normalize only this generated-NaN case. FP flags/traps
    // remain outside this portable kernel's validated contract.
    std::uint32_t response_bits;
    std::memcpy(&response_bits, &response, sizeof(response_bits));
    if (saved_capacity == 0 && (response_bits & 0x7fffffffu) == 0x7f800000u) {
        const std::uint32_t indefinite = 0xffc00000u;
        std::memcpy(&product, &indefinite, sizeof(product));
    }
    const float biased = product + 12582912.0f; // 0x100a158b8: 0x4b400000
    std::uint32_t bits;
    static_assert(sizeof(bits) == sizeof(biased), "requires IEEE binary32");
    std::memcpy(&bits, &biased, sizeof(bits));
    output = bits + 0xb4c00000u; // modular correction @ 0x1000993c8
    return true;
}
} // namespace opensc4::pollution_capacity
