// W6 production wrapper artifact. Runtime callbacks are supplied by adapters.
// This state is nonserialized; Read/Write field order remains unchanged.
#include "opensc4/pollution_grid_kernel.h"
#include <type_traits>

namespace {
void BindPollutionGridRuntime(PollutionSimulatorSaveRecord& record) noexcept {
    static_assert(sizeof(bool) == 1);
    static_assert(std::is_same_v<std::uint8_t, unsigned char>);
    auto& state = record.pollutionGridRuntime;
    state.radioactiveDirty = reinterpret_cast<unsigned char*>(&record.field328);
    state.airDirty = reinterpret_cast<unsigned char*>(&record.field329);
    state.waterDirty = reinterpret_cast<unsigned char*>(&record.field32A);
    state.garbageDirty = reinterpret_cast<unsigned char*>(&record.field32B);
    state.activeLandfillCount = &record.field2CC;
}
}

// Exact cSC4PollutionSimulator::SetIsRadioactive @ 0x10009a738.
void PollutionSimulatorSaveRecord::SetIsRadioactive(Uint32 x, Uint32 z, bool value) noexcept {
    BindPollutionGridRuntime(*this);
    pollution_grid_kernel::SetIsRadioactive(pollutionGridRuntime, x, z, value);
}
// Exact cSC4PollutionSimulator::SetAirValue @ 0x10009a790.
void PollutionSimulatorSaveRecord::SetAirValue(Uint32 x, Uint32 z, Sint32 value) noexcept {
    BindPollutionGridRuntime(*this);
    pollution_grid_kernel::SetAirValue(pollutionGridRuntime, x, z, value);
}
// Exact cSC4PollutionSimulator::SetWaterValue @ 0x10009a7e8.
void PollutionSimulatorSaveRecord::SetWaterValue(Uint32 x, Uint32 z, Sint32 value) noexcept {
    BindPollutionGridRuntime(*this);
    pollution_grid_kernel::SetWaterValue(pollutionGridRuntime, x, z, value);
}
// Exact cSC4PollutionSimulator::SetGarbageValue @ 0x10009a840.
void PollutionSimulatorSaveRecord::SetGarbageValue(Uint32 x, Uint32 z, Sint32 value) noexcept {
    BindPollutionGridRuntime(*this);
    pollution_grid_kernel::SetGarbageValue(pollutionGridRuntime, x, z, value);
}
// Exact cSC4PollutionSimulator::IsActiveLandfill @ 0x10009a924.
bool PollutionSimulatorSaveRecord::IsActiveLandfill(Uint32 x, Uint32 z) const noexcept {
    return pollution_grid_kernel::IsActiveLandfill(pollutionGridRuntime, x, z);
}
// Exact cSC4PollutionSimulator::SetIsActiveLandfill @ 0x10009a940.
void PollutionSimulatorSaveRecord::SetIsActiveLandfill(Uint32 x, Uint32 z, bool value) noexcept {
    BindPollutionGridRuntime(*this);
    pollution_grid_kernel::SetIsActiveLandfill(pollutionGridRuntime, x, z, value);
}
