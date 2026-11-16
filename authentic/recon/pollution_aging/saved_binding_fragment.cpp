#include "opensc4/city.h"
#ifndef OPENSC4_SAVED_BINDING_CHECKS
namespace opensc4 {
// Integration-only identity binding over canonical saved records. This is not
// a reconstructed game method; the canonical readers provide the provenance.
const BuildingOccupantSaveRecord* cSC4City::FindSavedBuildingOccupant(
    const SimSerializableRef& ref) const noexcept {
    if (!mSavedBuildingOccupantsAvailable || ref.memAddr == 0 ||
        ref.clsid != kCLSID_cSC4BuildingOccupant) return nullptr;
    const BuildingOccupantSaveRecord* match = nullptr;
    for (const auto& record : mSavedBuildingOccupants) {
        if (record.memAddr != ref.memAddr) continue;
        if (match) return nullptr; // Do not choose between duplicate identities.
        match = &record;
    }
    return match;
}

}
#else
#include <algorithm>
#include <cstdio>
#include <cstring>
using namespace opensc4;
static int passed=0, failed=0;
void Check(bool ok, const char* name) { std::printf("[%s] %s\n",ok?"PASS":"FAIL",name); ok ? ++passed : ++failed; }
// Saved-reference integration only. F0 is a connected saved reference here;
// this fixture makes no claim that F0 is an aging list.
void CheckSavedBuildingBindings(const std::string& steamApp) {
    const std::string rushPath = steamApp +
        "/Contents/GameData/Regions/Timbuktu/City - Rush Hour Tutorial.sc4";
    cDBPFArchive archive;
    std::vector<Uint8> bytes;
    std::vector<BuildingOccupantSaveRecord> independent;
    const bool decoded = archive.Open(rushPath) &&
        archive.ReadFirstByType(kCLSID_cSC4BuildingOccupant, bytes) &&
        DecodeMultiRecordSubfile<BuildingOccupantSaveRecord>(bytes, independent);
    cSC4City city;
    const bool loaded = city.LoadFromSC4File(rushPath);
    Check(decoded && loaded && independent.size() == 249 &&
          city.HasDecodedSavedBuildingOccupants() && city.SavedBuildingOccupants().size() == 249,
          "Rush Hour retains all 249 canonical building save records");
    const auto& refs = city.PollutionSaveRecord().listF0;
    const SimSerializableRef ref = refs.size() == 1 ? refs.front() : SimSerializableRef{};
    const auto* saved = city.FindSavedBuildingOccupant(ref);
    Check(ref.memAddr == 0x138a8240u && ref.clsid == kCLSID_cSC4BuildingOccupant && saved &&
          saved->base.exemplarHolder.exemplarKey == cSC4TGI{0x6534284au, 0xc8dbccbau, 0x1f470000u},
          "Actual Rush Hour pollution F0 reference resolves exact building identity/exemplar");
    auto expected = std::find_if(independent.begin(), independent.end(),
        [&](const auto& record) { return record.memAddr == ref.memAddr; });
    bool propertiesMatch = saved && expected != independent.end();
    if (propertiesMatch) {
        const auto& actual = saved->base.exemplarHolder.properties.properties;
        const auto& wanted = expected->base.exemplarHolder.properties.properties;
        propertiesMatch = actual.size() == 9 && actual.size() == wanted.size();
        for (std::size_t i = 0; propertiesMatch && i < actual.size(); ++i) {
            const auto& a = actual[i]; const auto& b = wanted[i];
            propertiesMatch = a.holderPropId == b.holderPropId && a.propertyId == b.propertyId &&
                a.variantTag == b.variantTag && a.unknownRef == b.unknownRef &&
                a.dbType == b.dbType && a.arrayCount == b.arrayCount &&
                a.stringValue == b.stringValue && a.rawValueBytes == b.rawValueBytes;
        }
    }
    Check(propertiesMatch, "Resolved raw saved entries retain canonical order, types and payloads");
    bool ageBytes = false;
    if (saved) for (const auto& prop : saved->base.exemplarHolder.properties.properties) {
        if (prop.holderPropId == 0xc910b4cfu && prop.propertyId == 0xc910b4cfu &&
            prop.variantTag == 0 && prop.dbType == 9 && prop.arrayCount == 0 &&
            prop.rawValueBytes == std::vector<Uint8>{0x30,0x9d,0x9e,0x39}) ageBytes = true;
    }
    Check(ageBytes, "Resolved saved age entry preserves literal 309d9e39 bytes without interpretation");
    Check(!city.FindSavedBuildingOccupant({}) &&
          !city.FindSavedBuildingOccupant({0, kCLSID_cSC4BuildingOccupant}) &&
          !city.FindSavedBuildingOccupant({ref.memAddr, kCLSID_cSC4FloraOccupant}) &&
          !city.FindSavedBuildingOccupant({0xffffffffu, kCLSID_cSC4BuildingOccupant}),
          "Saved binding rejects null, zero, wrong-class and absent references");
    cSC4City copied = city;
    const auto* copyRecord = copied.FindSavedBuildingOccupant(ref);
    Check(saved && copyRecord && copyRecord != saved && copyRecord->memAddr == saved->memAddr,
          "Copied city resolves into its own canonical record storage");
    cSC4City moved = std::move(copied);
    Check(moved.FindSavedBuildingOccupant(ref) && moved.HasDecodedSavedBuildingOccupants(),
          "Moved city resolves canonical records without a self-pointer index");
    Check(city.LoadFromSC4File(steamApp +
              "/Contents/GameData/Regions/Timbuktu/City - Big City Tutorial.sc4") &&
          city.HasDecodedSavedBuildingOccupants() && city.SavedBuildingOccupants().size() == 3427 &&
          !city.FindSavedBuildingOccupant(ref),
          "Real-city reload replaces canonical collection and prior reference binding");
    moved.GenerateProceduralCity(8, 8, 1);
    Check(!moved.HasDecodedSavedBuildingOccupants() && moved.SavedBuildingOccupants().empty() &&
          !moved.FindSavedBuildingOccupant(ref), "Procedural reset invalidates saved bindings");

    // Mutate only an in-memory copy of the shipped archive. Existing DBPF
    // reader accepts raw payloads; relocate the building entry to appended bytes.
    // No asset files are created or changed, no serializer implementation changes.
    auto withBuildings = [&](const std::vector<Uint8>& payload, cDBPFArchive& out) {
        std::vector<Uint8> raw;
        if (!archive.ReadRawFile(raw)) return false;
        for (std::size_t i = 0; i < archive.Entries().size(); ++i) {
            if (archive.Entries()[i].tgi.type != kCLSID_cSC4BuildingOccupant) continue;
            const Uint32 offset = static_cast<Uint32>(raw.size());
            const Uint32 size = static_cast<Uint32>(payload.size());
            const std::size_t index = archive.Header().indexOffset + i * 20u;
            std::memcpy(raw.data() + index + 12u, &offset, 4);
            std::memcpy(raw.data() + index + 16u, &size, 4);
            raw.insert(raw.end(), payload.begin(), payload.end());
            return out.OpenFromMemory(std::move(raw), "saved-binding-fixture.sc4");
        }
        return false;
    };
    auto truncated = bytes;
    if (!truncated.empty()) truncated.pop_back();
    std::vector<BuildingOccupantSaveRecord> partial;
    const bool partialFailure = !DecodeMultiRecordSubfile<BuildingOccupantSaveRecord>(truncated, partial)
        && !partial.empty();
    cDBPFArchive badArchive;
    Check(partialFailure && withBuildings(truncated, badArchive) &&
          city.LoadFromSC4Archive(badArchive) && !city.HasDecodedSavedBuildingOccupants() &&
          city.SavedBuildingOccupants().empty() && !city.FindSavedBuildingOccupant(ref),
          "Canonical decode failure after valid records exposes no partial binding collection");
    cDBPFArchive emptyArchive;
    Check(withBuildings({}, emptyArchive) && city.LoadFromSC4Archive(emptyArchive) &&
          city.HasDecodedSavedBuildingOccupants() && city.SavedBuildingOccupants().empty(),
          "Successfully decoded empty collection is distinguishable from unavailable storage");
    if (expected != independent.end()) {
        auto duplicate = *expected;
        auto& props = duplicate.base.exemplarHolder.properties.properties;
        props.push_back(props.front());
        std::vector<Uint8> payload;
        const std::vector<BuildingOccupantSaveRecord> single{duplicate};
        cDBPFArchive propsArchive;
        const bool propsLoaded = EncodeMultiRecordSubfile<BuildingOccupantSaveRecord>(single, payload) &&
            withBuildings(payload, propsArchive) && city.LoadFromSC4Archive(propsArchive);
        const auto* bound = city.FindSavedBuildingOccupant(ref);
        Check(propsLoaded && bound && bound->base.exemplarHolder.properties.properties.size() == 10 &&
              bound->base.exemplarHolder.properties.properties.back().holderPropId == props.front().holderPropId &&
              bound->base.exemplarHolder.properties.properties.back().rawValueBytes == props.front().rawValueBytes,
              "Raw saved property duplicates are retained without inventing precedence");
        const std::vector<BuildingOccupantSaveRecord> duplicates{duplicate, duplicate};
        cDBPFArchive dupArchive;
        Check(EncodeMultiRecordSubfile<BuildingOccupantSaveRecord>(duplicates, payload) &&
              withBuildings(payload, dupArchive) && city.LoadFromSC4Archive(dupArchive) &&
              city.HasDecodedSavedBuildingOccupants() && city.SavedBuildingOccupants().size() == 2 &&
              !city.FindSavedBuildingOccupant(ref), "Duplicate building identities are ambiguous, never first-match resolved");
    } else {
        Check(false, "Raw duplicate-property fixture requires independent canonical record");
        Check(false, "Duplicate-identity fixture requires independent canonical record");
    }
    cDBPFArchive unopened;
    Check(!city.LoadFromSC4Archive(unopened) && !city.HasDecodedSavedBuildingOccupants() &&
          city.SavedBuildingOccupants().empty(), "Rejected archive reload invalidates canonical bindings");
}

int main() { CheckSavedBuildingBindings(cDBPFSystem::DefaultSteamSC4AppPath()); std::printf("%d passed, %d failed\n",passed,failed); return failed != 0; }
#endif
