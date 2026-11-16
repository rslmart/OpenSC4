#pragma once
#include "opensc4/city.h"
#include <span>

namespace opensc4 {
// Bounded saved-snapshot integration outcomes, not original ABI booleans.
enum class SavedCapacityStatus {
    Value, MissingCapacity, Unavailable, Unsupported, CurveStorageFault
};
struct SavedCapacityResult {
    SavedCapacityStatus status = SavedCapacityStatus::Unavailable;
    // Defined only for Value/MissingCapacity. MissingCapacity has value zero,
    // matching original GetCurrentCapacity's false-return output.
    Uint32 value = 0;
};

// Invoked only if the original capacity method reaches its power query.
using SavedCapacityPowerQuery = SavedBuildingLotResult (*)(const void*);

// Synchronous owning property adapter over a validated unique-key, null-parent
// EQZB snapshot. Raw source records remain unchanged. Unsupported domain is
// distinct from a missing original property. No persistent borrowed pointers.
SavedCapacityResult EvaluateSavedBuildingCapacitySnapshot(
    const BuildingOccupantSaveRecord& building,
    const PollutionSimulatorSaveRecord& pollution,
    std::span<const Uint8> exemplarBytes, Uint32 capacityProperty,
    SavedCapacityPowerQuery powerQuery, const void* powerContext);

// Uses city-owned canonical records and exact-TGI assets for this synchronous
// call. Saved power/properties do not establish original post-load state.
SavedCapacityResult QuerySavedBuildingCapacity(
    const cSC4City& city, const SimSerializableRef& buildingRef,
    Uint32 capacityProperty, const cDBPFSystem& assets);
} // namespace opensc4
