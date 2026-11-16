// Bounded saved-snapshot integration, not a reconstruction of the factory's
// filtering, inheritance, or failure rollback. See loaded_building_capacity/
// ARRAY_REVIEW.md and pollution_properties/CONTRACT.md.
#include "opensc4/saved_building_capacity.h"
#include "opensc4/property_lookup.h"
#include "opensc4/property_scalar_uint32.h"
#include <algorithm>
#include <bit>
#include <cstring>
#include <utility>

namespace opensc4 {
namespace {
struct TypedDomain { SavedCapacityStatus status = SavedCapacityStatus::Unsupported; };
[[noreturn]] void Unsupported() { throw TypedDomain{}; }

Uint32 Word(std::span<const Uint8> bytes) {
    if (bytes.size() < 4) Unsupported();
    return Uint32(bytes[0]) | (Uint32(bytes[1]) << 8) |
           (Uint32(bytes[2]) << 16) | (Uint32(bytes[3]) << 24);
}
struct Reader {
    std::span<const Uint8> bytes;
    std::span<const Uint8> Take(std::size_t n) {
        if (n > bytes.size()) Unsupported();
        auto result = bytes.first(n);
        bytes = bytes.subspan(n);
        return result;
    }
    Uint32 U32() { return Word(Take(4)); }
};
// Bounded byte delimitation from GetFieldVariant @ 0x10038beea, tables
// @ 0x10038c958/0x10038c98c (VariantDbElementSize in civic_simulators.cpp).
// A recognized width grants storage only, never a numeric getter conversion.
int Width(Uint32 code) {
    switch (code) {
    case 0: return 0;
    case 1: case 5: case 0xb: case 0xc: case 0x11: return 1;
    case 2: case 6: case 0x10: return 2;
    case 3: case 7: case 9: return 4;
    case 4: case 8: case 0xa: return 8;
    default: Unsupported();
    }
}
std::vector<SerializedPropertyEntry> ParseDefaults(std::span<const Uint8> bytes) {
    Reader r{bytes};
    auto signature = r.Take(8);
    if (std::memcmp(signature.data(), "EQZB1###", 8) != 0) Unsupported();
    // This adapter deliberately excludes cohort/full inheritance.
    const auto parent = r.Take(12);
    for (auto b : parent) if (b != 0) Unsupported();
    const Uint32 count = r.U32();
    // Minimum property header is ID + reference flag + DB code. Bound before
    // reserve, and each payload multiplication is bounded by remaining bytes.
    if (count > r.bytes.size() / 9) Unsupported();
    std::vector<SerializedPropertyEntry> entries;
    entries.reserve(count);
    for (Uint32 i = 0; i < count; ++i) {
        SerializedPropertyEntry p;
        p.holderPropId = p.propertyId = r.U32();
        // Factory @ 0x100301f68: ID, byte reference flag, Uint32 DB code.
        if (r.Take(1)[0] != 0) Unsupported();
        p.dbType = r.U32();
        const bool array = (p.dbType & 0x8000u) != 0;
        const Uint32 base = array ? p.dbType - 0x8000u : p.dbType;
        // Limit to well-delimited known DB codes, no guessed skipping.
        if ((!array && base > 0xc) || (array && base == 0)) Unsupported();
        if (array && base == 0xc) {
            // ReadGZString: Uint32 byte length then exactly those bytes.
            const Uint32 n = r.U32();
            auto raw = r.Take(n);
            p.stringValue.assign(raw.begin(), raw.end());
            p.rawValueBytes.assign(raw.begin(), raw.end());
        } else {
            const auto width = static_cast<std::size_t>(Width(base));
            const Uint32 n = array ? (p.arrayCount = r.U32()) : 1;
            if (width && n > r.bytes.size() / width) Unsupported();
            auto raw = r.Take(std::size_t(n) * width);
            p.rawValueBytes.assign(raw.begin(), raw.end());
        }
        entries.push_back(std::move(p));
    }
    if (!r.bytes.empty()) Unsupported();
    return entries;
}
struct Property final : RawCurvePropertySeam, RawCurveValueSeam {
    SerializedPropertyEntry raw;
    std::vector<Float32> floats;
    explicit Property(const SerializedPropertyEntry& p) : raw(p) {
        if (p.variantTag == 0 && p.dbType == 0x8009 &&
            p.rawValueBytes.size() % 4 == 0 &&
            p.rawValueBytes.size() / 4 == p.arrayCount) {
            // RefFloat32 @ 0x10004e94c -> runtime800A; retain every raw
            // element (including zero/one/odd counts), not pairs.
            floats.resize(p.arrayCount);
            for (std::size_t i = 0; i < floats.size(); ++i) {
                const Uint32 bits = Word(std::span(p.rawValueBytes).subspan(i * 4, 4));
                std::memcpy(&floats[i], &bits, 4);
            }
        }
    }
    property_scalar::Scalar32 Scalar() const {
        if (raw.variantTag != 0 || raw.arrayCount != 0 || raw.rawValueBytes.size() != 4)
            Unsupported();
        Uint16 tag;
        switch (raw.dbType) {
        case 3: tag = 6; break;
        case 7: tag = 7; break;
        case 9: tag = 0xa; break;
        default: Unsupported();
        }
        return {tag, Word(raw.rawValueBytes)};
    }
    const RawCurveValueSeam* GetValue() const override {
        if (raw.variantTag != 0 || raw.dbType != 0x8009 ||
            raw.rawValueBytes.size() % 4 != 0 ||
            raw.rawValueBytes.size() / 4 != raw.arrayCount) Unsupported();
        return this;
    }
    // RefFloat32 @ 0x10004c804, GetCount @ 0x10004c682.
    const Float32* GetScalarData() const override { return floats.data(); }
    Uint32 GetScalarCount() const override { return raw.arrayCount; }
};
struct PropertySet {
    std::vector<Property> owned;
    std::vector<property_lookup::Entry> index;
    explicit PropertySet(const std::vector<SerializedPropertyEntry>& entries) {
        owned.reserve(entries.size());
        for (const auto& p : entries) owned.emplace_back(p);
        // Construct pointers only after owned storage is stable.
        index.reserve(owned.size());
        for (const auto& p : owned) index.push_back({p.raw.holderPropId, &p});
        std::sort(index.begin(), index.end(), [](auto a, auto b) { return a.key < b.key; });
        for (std::size_t i = 1; i < index.size(); ++i)
            if (index[i-1].key == index[i].key) Unsupported();
    }
    const Property* Find(Uint32 key) const {
        return static_cast<const Property*>(property_lookup::FindSorted(index, key));
    }
};
struct Holder final : OccupantPropertyHolderSeam {
    PropertySet local, defaults;
    Holder(const SerializedPropertyHolder& saved, const std::vector<SerializedPropertyEntry>& base)
        : local(saved.properties), defaults(base) {
        if (saved.version != 2) Unsupported();
    }
    const Property* Find(Uint32 id) const {
        // cSCExemplarPropertyHolder::GetProperty @ 0x10004ae40: nonnull
        // local wins, otherwise default. Keys are not embedded property IDs.
        if (auto p = local.Find(id)) return p;
        return defaults.Find(id);
    }
    bool GetPropertyValue(Uint32, Sint64&) const override { Unsupported(); }
    const CurvePropertyData* GetProperty(Uint32) const override { Unsupported(); }
    bool GetPropertyValue(Uint32 id, Float32& out) const override {
        const auto* p = Find(id);
        if (!p) return false; // helper @ 0x10004b636: preserve output
        Uint32 bits;
        if (property_scalar::ReadFloat32Bits(p->Scalar(), bits) != property_scalar::ReadStatus::Value)
            Unsupported();
        std::memcpy(&out, &bits, 4);
        return true;
    }
};
struct Occupant final : OccupantSeam {
    Holder holder;
    Occupant(const SerializedPropertyHolder& saved, const std::vector<SerializedPropertyEntry>& base)
        : holder(saved, base) {}
    const OccupantPropertyHolderSeam* GetPropertyHolder() const override { return &holder; }
};
struct Access final : PollutionCapacityAccessSeam {
    SavedCapacityPowerQuery power;
    const void* context;
    Access(SavedCapacityPowerQuery p, const void* c) : power(p), context(c) {}
    bool ReadCapacity(const OccupantPropertyHolderSeam& h, Uint32 id, Uint32& out) const override {
        const auto* p = static_cast<const Holder&>(h).Find(id);
        if (!p) return false; // Uint32 helper @ 0x10004b5ac
        if (property_scalar::ReadUint32(p->Scalar(), out) != property_scalar::ReadStatus::Value)
            Unsupported();
        return true;
    }
    const RawCurvePropertySeam* LookupCurve(const OccupantPropertyHolderSeam& h, Uint32 id) const override {
        return static_cast<const Holder&>(h).Find(id);
    }
    bool IsOccupantPowered(const OccupantSeam&) const override {
        if (!power) throw TypedDomain{SavedCapacityStatus::Unavailable};
        const auto result = power(context);
        switch (result.status) {
        case SavedBuildingLotStatus::NoLot: return false;
        case SavedBuildingLotStatus::Lot: return result.savedPowered;
        case SavedBuildingLotStatus::Unavailable: throw TypedDomain{SavedCapacityStatus::Unavailable};
        case SavedBuildingLotStatus::Unsupported: Unsupported();
        }
        Unsupported();
    }
};
} // namespace

SavedCapacityResult EvaluateSavedBuildingCapacitySnapshot(
    const BuildingOccupantSaveRecord& building, const PollutionSimulatorSaveRecord& pollution,
    std::span<const Uint8> exemplarBytes, Uint32 capacityProperty,
    SavedCapacityPowerQuery powerQuery, const void* powerContext) {
    try {
        const auto defaults = ParseDefaults(exemplarBytes);
        Occupant occupant(building.base.exemplarHolder.properties, defaults);
        Access access(powerQuery, powerContext);
        auto record = pollution;
        record.capacityAccess = &access; // local copy only; never retained
        Uint32 output = 0;
        // Exact capacity algorithm @ 0x1000992cc remains in existing method.
        const bool found = record.GetCurrentCapacity(capacityProperty, occupant, output);
        return {found ? SavedCapacityStatus::Value : SavedCapacityStatus::MissingCapacity, output};
    } catch (const TypedDomain& e) {
        return {e.status, 0};
    } catch (const ResponseCurveStorageFault&) {
        return {SavedCapacityStatus::CurveStorageFault, 0};
    }
}

SavedCapacityResult QuerySavedBuildingCapacity(const cSC4City& city,
    const SimSerializableRef& buildingRef, Uint32 capacityProperty, const cDBPFSystem& assets) {
    if (!city.HasDecodedSavedBuildingOccupants() ||
        !city.HasCivicSaveRecord(kCLSID_cSC4PollutionSimulator)) return {};
    const auto* building = city.FindSavedBuildingOccupant(buildingRef);
    if (!building) return {SavedCapacityStatus::Unsupported, 0};
    std::vector<Uint8> bytes;
    if (!assets.LoadAssetByTGI(building->base.exemplarHolder.exemplarKey, bytes)) return {};
    struct Context { const cSC4City& city; const SimSerializableRef& ref; } context{city, buildingRef};
    return EvaluateSavedBuildingCapacitySnapshot(*building, city.PollutionSaveRecord(), bytes,
        capacityProperty, [](const void* p) {
            const auto& c = *static_cast<const Context*>(p);
            return c.city.QuerySavedBuildingLot(c.ref);
        }, &context);
}
} // namespace opensc4
