// Required integration includes: civic_simulators.h, tract_developer.h,
// pollution_capacity_kernel.h, <new>. Fragment belongs in namespace opensc4.
namespace {
// Exact cSC4PollutionSimulator::GetCurrentCapacity @ 0x1000992cc.
struct PollutionCapacityRecordContext {
    const PollutionSimulatorSaveRecord& record;
};
const pollution_capacity::Callbacks kPollutionCapacityRecordCallbacks{
    [](void*, void* occupant) -> void* {
        return const_cast<OccupantPropertyHolderSeam*>(
            static_cast<const OccupantSeam*>(occupant)->GetPropertyHolder());
    },
    [](void* context, void* holder, Uint32 id, Uint32& out) {
        const auto& record = static_cast<PollutionCapacityRecordContext*>(context)->record;
        return record.capacityAccess->ReadCapacity(
            *static_cast<const OccupantPropertyHolderSeam*>(holder), id, out);
    },
    [](void* context, void* occupant) {
        const auto& record = static_cast<PollutionCapacityRecordContext*>(context)->record;
        return record.capacityAccess->IsOccupantPowered(
            *static_cast<const OccupantSeam*>(occupant));
    },
    [](void* context) {
        // Live field348 read AFTER power dispatch @ 0x10009933b.
        return static_cast<PollutionCapacityRecordContext*>(context)->record.field348;
    },
    [](void* context, void* holder, Uint32 id) -> void* {
        const auto& record = static_cast<PollutionCapacityRecordContext*>(context)->record;
        return const_cast<RawCurvePropertySeam*>(record.capacityAccess->LookupCurve(
            *static_cast<const OccupantPropertyHolderSeam*>(holder), id));
    },
    [](void*, void* storage) { ::new (storage) cSC4ResponseCurve; },
    [](void*, void* storage, void* property) {
        // SetResponseCurveData @ 0x1000ac37a: value, data, count in that order.
        const auto* value = static_cast<const RawCurvePropertySeam*>(property)->GetValue();
        const Float32* data = value->GetScalarData();
        const Uint32 count = value->GetScalarCount();
        (void)static_cast<cSC4ResponseCurve*>(storage)->SetResponseCurveData(
            std::span<const Float32>(data, count));
    },
    [](void*, void* holder, Uint32 id, Float32& out) {
        return static_cast<const OccupantPropertyHolderSeam*>(holder)->GetPropertyValue(id, out);
    },
    [](void*, void* storage, Float32 input) {
        Float32 out;
        if (!static_cast<const cSC4ResponseCurve*>(storage)->TryGetOutputValue(input, out)) {
            // Portable boundary for null backing read @ 0x1000ac542.
            throw ResponseCurveStorageFault{};
        }
        return out;
    },
    [](void*, void* storage) noexcept {
        static_cast<cSC4ResponseCurve*>(storage)->~cSC4ResponseCurve();
    }
};
} // namespace

// Exact cSC4PollutionSimulator::GetCurrentCapacity @ 0x1000992cc.
bool PollutionSimulatorSaveRecord::GetCurrentCapacity(
    Uint32 propertyId, const OccupantSeam& occupant, Uint32& out) const {
    PollutionCapacityRecordContext context{*this};
    alignas(cSC4ResponseCurve) unsigned char storage[sizeof(cSC4ResponseCurve)];
    return pollution_capacity::get_current_capacity(
        {&context, storage}, kPollutionCapacityRecordCallbacks, propertyId,
        const_cast<OccupantSeam*>(&occupant), out);
}
