#pragma once
#include <cstdint>

namespace opensc4::pollution_grid_kernel {
// Adapter contract: each callback resolves the current grid and vtable from its
// context. For a live object, context should identify the owning field/slot,
// not cache its pointee: the binary reloads the grid between calls. The kernel
// likewise rereads these descriptors after callbacks (which may mutate State).
// setTract receives the complete signed ECX value; a ushort-grid adapter stores
// only its low16 bits (original SetTractValue @ 0x10015b500, no bounds guard).
struct Grid {
    void* context;
    std::uint32_t (*getCell)(void*, std::uint32_t, std::uint32_t);
    std::int32_t (*getTractShift)(void*);
    void (*setTract)(void*, std::uint32_t, std::uint32_t, std::int32_t);
};
// Non-owning view. All callbacks and storage used by an invoked method must be
// valid; there are deliberately no invented missing-grid/callback guards.
struct State {
    Grid radioactive, air, water, garbage;
    std::uint8_t *radioactiveDirty, *airDirty, *waterDirty, *garbageDirty;
    std::uint32_t* activeLandfillCount;
    void* modeContext;
    std::uint32_t (*getMode)(void*);
    void* activeContext;
    std::uint32_t (*queryActiveLandfill)(void*, std::uint32_t, std::uint32_t);
};
// x86 SHR r32,CL masks the count to five bits (e.g. @ 0x10009a769).
inline std::uint32_t shifted(std::uint32_t v, std::int32_t count) {
    return v >> (static_cast<std::uint32_t>(count) & 31u);
}
// Exact cSC4PollutionSimulator::SetIsRadioactive @ 0x10009a738.
inline void SetIsRadioactive(State& s, std::uint32_t x, std::uint32_t z, bool value) {
    const auto shift = s.radioactive.getTractShift(s.radioactive.context);
    s.radioactive.setTract(s.radioactive.context, shifted(x, shift), shifted(z, shift), value ? 1 : 0);
    *s.radioactiveDirty = 1;
}
// MOVSX ECX,word semantics, without implementation-defined unsigned-to-signed casts.
// Used at 0x10009a7c6, 0x10009a81e, 0x10009a895 and 0x10009a9f9.
inline std::int32_t signedLow16(std::uint32_t v) {
    return static_cast<std::int32_t>(v & 0x7fffu) -
           static_cast<std::int32_t>(v & 0x8000u);
}
// Exact cSC4PollutionSimulator::SetAirValue @ 0x10009a790.
inline void SetAirValue(State& s, std::uint32_t x, std::uint32_t z, std::int32_t value) {
    const auto shift = s.air.getTractShift(s.air.context);
    s.air.setTract(s.air.context, shifted(x, shift), shifted(z, shift),
                   signedLow16(static_cast<std::uint32_t>(value)));
    *s.airDirty = 1;
}
// Exact cSC4PollutionSimulator::SetWaterValue @ 0x10009a7e8.
inline void SetWaterValue(State& s, std::uint32_t x, std::uint32_t z, std::int32_t value) {
    const auto shift = s.water.getTractShift(s.water.context);
    s.water.setTract(s.water.context, shifted(x, shift), shifted(z, shift),
                     signedLow16(static_cast<std::uint32_t>(value)));
    *s.waterDirty = 1;
}
// Exact cSC4PollutionSimulator::SetGarbageValue @ 0x10009a840.
inline void SetGarbageValue(State& s, std::uint32_t x, std::uint32_t z, std::int32_t value) {
    const auto shift = s.garbage.getTractShift(s.garbage.context);
    const auto old = s.garbage.getCell(s.garbage.context, x, z);
    const auto raw = (old & 0xffff8000u) | static_cast<std::uint32_t>(value);
    s.garbage.setTract(s.garbage.context, shifted(x, shift), shifted(z, shift), signedLow16(raw));
    *s.garbageDirty = 1;
}
// Exact cSC4PollutionSimulator::IsActiveLandfill @ 0x10009a924.
inline bool IsActiveLandfill(const State& s, std::uint32_t x, std::uint32_t z) {
    return ((s.garbage.getCell(s.garbage.context, x, z) & 0x8000u) >> 15) != 0;
}
// Exact cSC4PollutionSimulator::SetIsActiveLandfill @ 0x10009a940.
inline void SetIsActiveLandfill(State& s, std::uint32_t x, std::uint32_t z, bool active) {
    const auto mode = s.getMode(s.modeContext); // Unconditional call @ 0x10009a95e.
    if (mode != 14u && active) return;
    // TEST/CMP AL (@ 0x10009a981 / 0x10009a9bc), not EAX or normalized bool.
    const auto wasActive = s.queryActiveLandfill(s.activeContext, x, z) & 0xffu;
    if (wasActive == 0 && active) {
        const auto old = s.garbage.getCell(s.garbage.context, x, z);
        // OR EAX,0xffff8000 @ 0x10009a99d forces all upper bits to one.
        const auto raw = old | 0xffff8000u;
        ++*s.activeLandfillCount; // INC dword wraps, before dispatch.
        s.garbage.setTract(s.garbage.context, x, z, signedLow16(raw));
        *s.garbageDirty = 1;
    } else if (wasActive == 1 && !active) {
        const auto raw = s.garbage.getCell(s.garbage.context, x, z) & 0x7fffu;
        if (*s.activeLandfillCount != 0) --*s.activeLandfillCount;
        s.garbage.setTract(s.garbage.context, x, z, signedLow16(raw));
        *s.garbageDirty = 1;
    }
}
} // namespace opensc4::pollution_grid_kernel
