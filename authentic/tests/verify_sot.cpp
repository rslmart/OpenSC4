#include "opensc4/weather_coordinates.h"
#include "opensc4/pollution_capacity_kernel.h"
#include "opensc4/gzcom.h"
#include "opensc4/rz_random.h"
#include "opensc4/simgrid.h"
#include "opensc4/dbpf.h"
#include "opensc4/formats.h"
#include "opensc4/city.h"
#include "opensc4/gl_city_renderer.h"
#include "opensc4/decompilation_registry.h"
#include "opensc4/sc4_subfiles.h"
#include "opensc4/sc4_subfiles_batch3.h"
#include "opensc4/tract_developer.h"
#include "opensc4/terrain.h"
#include "opensc4/demand_simulator.h"
#include <cstdio>
#include <bit>
#include <cstring>
#include <cmath>
#include <limits>
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <random>
#include <set>
#include <array>
#include <string>
#include <vector>

using namespace opensc4;
namespace fs = std::filesystem;

void RunSavedCapacityChecks(void (*check)(bool, const char*), bool mountedFixture);
void RunSavedSint32Checks(void (*check)(bool, const char*));
void RunPollutionAgingMathChecks(void (*check)(bool, const char*));
void RunLoadedAgingTransitionChecks(void (*check)(bool, const char*));
int RunWeatherPositionToCellChecks();
int RunWeatherCellToPositionChecks();
namespace opensc4::weather_coordinates { int RunWeatherCellToStandardCityCellChecks(); }
int RunWeatherStandardCityCellToCellChecks();

namespace {

int gPassed = 0;
int gFailed = 0;

void Check(bool cond, const char* name, const char* detail = "") {
    if (cond) {
        ++gPassed;
        std::printf("  [PASS] %s %s\n", name, detail);
    } else {
        ++gFailed;
        std::printf("  [FAIL] %s %s\n", name, detail);
    }
}

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

// Integration policy tests on memory-only variants of a shipped DBPF snapshot.
void CheckSavedLotBindings(const std::string& steamApp) {
    using Status = SavedBuildingLotStatus;
    const SimSerializableRef ref{0x138a8240u, kCLSID_cSC4BuildingOccupant};
    const auto path = steamApp + "/Contents/GameData/Regions/Timbuktu/City - Rush Hour Tutorial.sc4";
    cDBPFArchive archive;
    std::vector<Uint8> raw, bytes;
    std::vector<BuildingOccupantSaveRecord> buildings;
    std::vector<LotFullSaveRecord> lots;
    std::vector<ZoneDeveloperSaveRecord> grids;
    const bool ready = archive.Open(path) && archive.ReadRawFile(raw) &&
        archive.ReadFirstByType(kCLSID_cSC4BuildingOccupant, bytes) &&
        DecodeMultiRecordSubfile<BuildingOccupantSaveRecord>(bytes, buildings) &&
        archive.ReadFirstByType(kCLSID_cSC4Lot, bytes) &&
        DecodeMultiRecordSubfile<LotFullSaveRecord>(bytes, lots) &&
        archive.ReadFirstByType(kCLSID_cSC4ZoneDeveloper, bytes) &&
        DecodeMultiRecordSubfile<ZoneDeveloperSaveRecord>(bytes, grids);
    Check(ready, "Saved-lot memory fixture canonical decode");
    if (!ready) return;
    auto buildingIt = std::find_if(buildings.begin(), buildings.end(),
        [&](const auto& b) { return b.memAddr == ref.memAddr; });
    auto lotIt = std::find_if(lots.begin(), lots.end(),
        [](const auto& l) { return l.memAddr == 0x13a49798u; });
    if (buildingIt == buildings.end() || lotIt == lots.end() || grids.size() != 1) {
        Check(false, "Saved-lot fixture target identities"); return;
    }
    const auto building = *buildingIt;
    const auto lot = *lotIt;
    const auto grid = grids.front();
    std::size_t populated = 0;
    for (const auto& row : grid.rows) for (const auto& r : row) populated += r.memAddr != 0;
    Check(grid.rows.size() == 128 && grid.rows.front().size() == 128 &&
          lots.size() == 525 && populated == 998, "Saved-lot source grid/lot fixture dimensions");
    cSC4City city;
    const auto fixtureResult = [](const SavedBuildingLotResult& r) {
        return r.status == Status::Lot && r.cellX == 38 && r.cellZ == 115 &&
            r.lotRef.memAddr == 0x13a49798u && r.lotRef.clsid == kCLSID_cSC4Lot &&
            r.savedFlagsCD == 0x53 && r.savedPowered;
    };
    Check(city.LoadFromSC4Archive(archive) && fixtureResult(city.QuerySavedBuildingLot(ref)),
          "Rush Hour saved building selects cell 38,115 lot 13a49798 flags 53 powered");
    // Relocate raw uncompressed payloads and edit index entries only in copies.
    // hideType removes the requested subfile from type lookup, without disk writes.
    auto loadPayloads = [&](const std::vector<Uint8>& b, const std::vector<Uint8>& l,
                            const std::vector<Uint8>& g, Uint32 hideType = 0) {
        auto memory = raw;
        for (std::size_t i = 0; i < archive.Entries().size(); ++i) {
            const auto type = archive.Entries()[i].tgi.type;
            const auto* payload = type == kCLSID_cSC4BuildingOccupant ? &b :
                type == kCLSID_cSC4Lot ? &l : type == kCLSID_cSC4ZoneDeveloper ? &g : nullptr;
            if (!payload) continue;
            const auto index = archive.Header().indexOffset + i * 20u;
            const Uint32 offset = static_cast<Uint32>(memory.size());
            const Uint32 size = static_cast<Uint32>(payload->size());
            if (type == hideType) {
                const Uint32 hidden = 0xf1234567u;
                std::memcpy(memory.data() + index, &hidden, 4);
            }
            std::memcpy(memory.data() + index + 12, &offset, 4);
            std::memcpy(memory.data() + index + 16, &size, 4);
            memory.insert(memory.end(), payload->begin(), payload->end());
        }
        cDBPFArchive modified;
        return modified.OpenFromMemory(std::move(memory), "saved-lot-memory.sc4") &&
               city.LoadFromSC4Archive(modified);
    };
    auto load = [&](const std::vector<BuildingOccupantSaveRecord>& bs,
                    const std::vector<LotFullSaveRecord>& ls,
                    const std::vector<ZoneDeveloperSaveRecord>& gs, Uint32 hidden = 0) {
        std::vector<Uint8> b, l, g;
        return EncodeMultiRecordSubfile<BuildingOccupantSaveRecord>(bs, b) && EncodeMultiRecordSubfile<LotFullSaveRecord>(ls, l) &&
            EncodeMultiRecordSubfile<ZoneDeveloperSaveRecord>(gs, g) && loadPayloads(b, l, g, hidden);
    };
    auto status = [&] { return city.QuerySavedBuildingLot(ref).status; };
    bool flagsOK = true;
    for (Uint8 flags : {0u, 8u, 16u, 24u, 0xe7u, 0xefu, 0xf7u, 0xffu}) {
        auto changed = lot; changed.flagsCD = flags;
        const bool loaded = load({building}, {changed}, {grid});
        const auto r = city.QuerySavedBuildingLot(ref);
        const bool expected = flags != 8u && flags != 0xefu;
        flagsOK &= loaded && r.status == Status::Lot && r.savedFlagsCD == flags && r.savedPowered == expected;
    }
    Check(flagsOK, "All saved power-bit combinations, including unrelated flag bits");
    auto nullGrid = grid; nullGrid.rows[38][115] = {};
    Check(load({building}, {lot}, {nullGrid}) && status() == Status::NoLot &&
          !city.QuerySavedBuildingLot(ref).savedPowered,
          "Null selected cell returns NoLot and false power despite overlapping lot bounds");
    auto wrongGrid = grid; wrongGrid.rows[38][115].clsid = kCLSID_cSC4BuildingOccupant;
    Check(load({building}, {lot}, {wrongGrid}) && status() == Status::Unsupported,
          "Non-lot grid reference is unsupported");
    auto missingGrid = grid; missingGrid.rows[38][115].memAddr = 0xffffffffu;
    Check(load({building}, {lot}, {missingGrid}) && status() == Status::Unsupported,
          "Missing referenced lot address is unsupported without bounds scan");
    Check(load({building}, {lot, lot}, {grid}) && status() == Status::Unsupported,
          "Duplicate canonical lot addresses are ambiguous");
    Check(load({building, building}, {lot}, {grid}) && status() == Status::Unsupported,
          "Duplicate canonical building addresses are ambiguous for saved-lot query");
    Check(load({}, {lot}, {grid}) && status() == Status::Unsupported,
          "Available zero building records differ from unavailable");
    Check(load({building}, {}, {grid}) && status() == Status::Unsupported,
          "Available zero lot records differ from unavailable");
    Check(load({building}, {lot}, {}) && status() == Status::Unsupported &&
          load({building}, {lot}, {grid, grid}) && status() == Status::Unsupported,
          "Saved grid requires exactly one zone record");
    auto shortGrid = grid; shortGrid.rows.pop_back();
    auto shortRow = grid; shortRow.rows.back().pop_back();
    // Wire dimensions describe rectangular rows; exercise a consistently short
    // inner dimension. Ragged rows cannot result from a successful decode.
    for (auto& row : shortRow.rows) row.resize(127);
    Check(load({building}, {lot}, {shortGrid}) && status() == Status::Unsupported &&
          load({building}, {lot}, {shortRow}) && status() == Status::Unsupported,
          "Saved grid dimensions must match city on both axes");
    bool geometryOK = true;
    for (const auto& point : std::array<std::array<float,2>,6>{{
            {{-16,1840}}, {{608,-16}}, {{2048,1840}}, {{608,2048}}, {{2064,1840}}, {{608,2064}}}}) {
        auto changed = building;
        changed.minBounds.x = changed.maxBounds.x = point[0];
        changed.minBounds.z = changed.maxBounds.z = point[1];
        geometryOK &= load({changed}, {lot}, {grid}) && status() == Status::NoLot &&
                      !city.QuerySavedBuildingLot(ref).savedPowered;
    }
    Check(geometryOK, "Negative, exact city limit and over-limit coordinates on both axes");
    auto edgeGrid = grid;
    edgeGrid.rows[0][115] = {lot.memAddr, kCLSID_cSC4Lot};
    edgeGrid.rows[127][115] = {lot.memAddr, kCLSID_cSC4Lot};
    bool edgeOK = true;
    for (const auto& point : std::array<std::pair<float,int>,5>{{
            {0.0f,0}, {-0.0f,0}, {0.25f,0}, {2047.0f,127}, {2048.0f,128}}}) {
        auto changed = building;
        changed.minBounds.x = changed.maxBounds.x = point.first;
        const bool loaded = load({changed}, {lot}, {edgeGrid});
        const auto r = city.QuerySavedBuildingLot(ref);
        edgeOK &= loaded && r.cellX == point.second && r.cellZ == 115 &&
            r.status == (point.second == 128 ? Status::NoLot : Status::Lot);
    }
    Check(edgeOK, "Saved-cell zero, signed zero, fractional and near-limit boundaries");
    auto nanBounds = building;
    nanBounds.minBounds.x = std::bit_cast<float>(0x7fc00001u);
    Check(load({nanBounds}, {lot}, {grid}) && status() == Status::NoLot,
          "NaN coordinate follows reviewed integer-bit cell selection to out-of-range NoLot");
    auto poisonedBounds = building; poisonedBounds.base.cellBounds.fill(0xff);
    Check(load({poisonedBounds}, {lot}, {grid}) && fixtureResult(city.QuerySavedBuildingLot(ref)),
          "Saved base.cellBounds bytes do not determine position");
    Check(city.QuerySavedBuildingLot({}).status == Status::Unsupported &&
          city.QuerySavedBuildingLot({ref.memAddr,kCLSID_cSC4Lot}).status == Status::Unsupported &&
          city.QuerySavedBuildingLot({0xffffffffu,ref.clsid}).status == Status::Unsupported,
          "Null, wrong-class and absent building references are unsupported");
    std::vector<Uint8> b, l, g;
    EncodeMultiRecordSubfile<BuildingOccupantSaveRecord>(std::vector{building}, b);
    EncodeMultiRecordSubfile<LotFullSaveRecord>(std::vector{lot,lot}, l);
    EncodeMultiRecordSubfile<ZoneDeveloperSaveRecord>(std::vector{grid,grid}, g);
    auto badL = l; badL.pop_back();
    auto badG = g; badG.pop_back();
    Check(loadPayloads(b, badL, g) && status() == Status::Unavailable &&
          loadPayloads(b, l, badG) && status() == Status::Unavailable,
          "Failed lot/grid decode after valid records publishes no partial snapshot");
    auto badB = b; badB.pop_back();
    Check(loadPayloads(badB, l, g) && status() == Status::Unavailable,
          "Failed canonical building decode makes saved lot query unavailable");
    Check(load({building}, {lot}, {grid}, kCLSID_cSC4Lot) && status() == Status::Unavailable &&
          load({building}, {lot}, {grid}, kCLSID_cSC4ZoneDeveloper) && status() == Status::Unavailable &&
          load({building}, {lot}, {grid}, kCLSID_cSC4BuildingOccupant) && status() == Status::Unavailable,
          "Missing canonical building, lot or grid storage is unavailable");
    Check(city.LoadFromSC4Archive(archive), "Restore saved snapshot for ownership checks");
    const auto heldResult = city.QuerySavedBuildingLot(ref);
    cSC4City copied = city, assigned; assigned = city;
    cSC4City moved = std::move(copied), moveAssigned; moveAssigned = std::move(assigned);
    city.GenerateProceduralCity(8,8,1);
    Check(status() == Status::Unavailable && fixtureResult(heldResult) &&
          fixtureResult(moved.QuerySavedBuildingLot(ref)) && fixtureResult(moveAssigned.QuerySavedBuildingLot(ref)),
          "Copy/move construction and assignment own snapshots; returned values survive reset");
    Check(moved.LoadFromSC4File(steamApp + "/Contents/GameData/Regions/Timbuktu/City - Big City Tutorial.sc4") &&
          moved.QuerySavedBuildingLot(ref).status == Status::Unsupported,
          "Different-city reload replaces all saved snapshot bindings");
    Check(!moveAssigned.LoadFromSC4File("/nonexistent/opensc4-saved-lot.sc4") &&
          moveAssigned.QuerySavedBuildingLot(ref).status == Status::Unavailable,
          "Failed file open clears previously valid saved snapshots");
    cDBPFArchive unopened;
    Check(city.LoadFromSC4Archive(archive) && !city.LoadFromSC4Archive(unopened) &&
          status() == Status::Unavailable, "Rejected archive load clears previously valid saved snapshots");
}

void AddGridCallback(Sint16& myVal, Uint8 otherVal, void*) {
    myVal = static_cast<Sint16>(myVal + otherVal);
}

// W6 record adapters: callbacks expose dispatch coordinates, signed ECX and
// actual record storage timing. This mock has no bounds policy or city hydration.
struct PollutionRecordGridFixture {
    PollutionSimulatorSaveRecord record{};
    struct GridMock {
        PollutionRecordGridFixture* owner;
        int id;
        Uint32 raw = 0;
        Sint32 shift = 0;
        void (*afterShift)(GridMock&) = nullptr;
        void (*afterSet)(GridMock&) = nullptr;
    };
    struct Event {
        char op;
        int id;
        Uint32 x, z;
        Sint32 value;
        Uint32 count;
        std::array<bool,4> dirty;
    };
    GridMock r{this,0}, a{this,1}, w{this,2}, g{this,3}, replacement{this,4};
    Uint32 mode = 14, active = 0;
    std::vector<Event> trace;
    std::array<bool,4> dirty() const {
        return {record.field328,record.field329,record.field32A,record.field32B};
    }
    static void log(GridMock& m, char op, Uint32 x=0, Uint32 z=0, Sint32 v=0) {
        auto& f=*m.owner;
        f.trace.push_back({op,m.id,x,z,v,f.record.field2CC,f.dirty()});
    }
    static Uint32 get(void* p, Uint32 x, Uint32 z) {
        auto& m=*static_cast<GridMock*>(p); log(m,'G',x,z); return m.raw;
    }
    static Sint32 shift(void* p) {
        auto& m=*static_cast<GridMock*>(p); log(m,'H');
        const auto n=m.shift;
        if(m.afterShift) m.afterShift(m);
        return n;
    }
    static void set(void* p, Uint32 x, Uint32 z, Sint32 v) {
        auto& m=*static_cast<GridMock*>(p); log(m,'S',x,z,v);
        m.raw=static_cast<Uint32>(v)&0xffffu;
        if(m.afterSet) m.afterSet(m);
    }
    static pollution_grid_kernel::Grid grid(GridMock& m) { return {&m,get,shift,set}; }
    static Uint32 getMode(void* p) {
        auto& f=*static_cast<PollutionRecordGridFixture*>(p); log(f.g,'M'); return f.mode;
    }
    static Uint32 query(void* p, Uint32 x, Uint32 z) {
        auto& f=*static_cast<PollutionRecordGridFixture*>(p); log(f.g,'Q',x,z); return f.active;
    }
    PollutionRecordGridFixture() {
        record.pollutionGridRuntime={grid(r),grid(a),grid(w),grid(g),nullptr,nullptr,nullptr,nullptr,
                                    nullptr,this,getMode,this,query};
    }
    std::string ops() const { std::string s; for(auto& e:trace) s+=e.op; return s; }
};

void CheckPollutionRecordGridWrappers() {
    using F=PollutionRecordGridFixture;
    {
        F f; f.r.shift=33;
        f.record.SetIsRadioactive(0xffffffffu,0x80000000u,true);
        Check(f.ops()=="HS" && f.trace.back().x==0x7fffffffu &&
              f.trace.back().z==0x40000000u && f.trace.back().value==1 &&
              f.trace.back().dirty==std::array<bool,4>{} &&
              f.dirty()==std::array<bool,4>{true,false,false,false},
              "W6 record radioactive unsigned masked shift and dirty-after-set @ 0x10009a738");
        f.record.field328=false; f.trace.clear();
        f.record.SetIsRadioactive(0,0,false);
        Check(f.r.raw==0 && f.record.field328 && !f.trace.back().dirty[0],
              "W6 record radioactive false still writes and dirties @ 0x10009a738");
    }
    {
        F f; f.a.shift=-1;
        f.record.SetAirValue(0xffffffffu,0x80000000u,32768);
        Check(f.ops()=="HS" && f.trace.back().x==1 && f.trace.back().z==1 &&
              f.trace.back().value==-32768 && !f.trace.back().dirty[1] &&
              f.dirty()==std::array<bool,4>{false,true,false,false},
              "W6 record air signed16 boundary and negative shift mask @ 0x10009a790");
        f.record.SetAirValue(0,0,65536);
        Check(f.trace.back().value==0 && f.a.raw==0,
              "W6 record air low16 wrap to zero @ 0x10009a790");
    }
    {
        F f; f.w.shift=32;
        f.record.SetWaterValue(0xffffffffu,0x80000000u,-32769);
        Check(f.ops()=="HS" && f.trace.back().x==0xffffffffu &&
              f.trace.back().z==0x80000000u && f.trace.back().value==32767 &&
              !f.trace.back().dirty[2] && f.dirty()==std::array<bool,4>{false,false,true,false},
              "W6 record water low16 underflow and shift32 @ 0x10009a7e8");
    }
    for(Uint32 old:{0x0030u,0x8030u}) {
        F f; f.g.raw=old; f.g.shift=1;
        f.record.SetGarbageValue(8,10,5);
        Check(f.ops()=="HGS" && f.trace[1].x==8 && f.trace[1].z==10 &&
              f.trace.back().x==4 && f.trace.back().z==5 &&
              f.trace.back().value==(old==0x30 ? 5:-32763) &&
              f.g.raw==(old==0x30 ? 5u:0x8005u) && !f.trace.back().dirty[3] &&
              f.dirty()==std::array<bool,4>{false,false,false,true},
              "W6 record garbage clears low bits, preserves active bit, raw get/shifted set @ 0x10009a840");
    }
    for(Uint32 raw:{0xffff0000u,0x8000u}) {
        F f; f.g.raw=raw;
        const auto& record=f.record;
        Check(record.IsActiveLandfill(8,10)==(raw==0x8000u) && f.ops()=="G" &&
              f.trace[0].x==8 && f.trace[0].z==10 && f.dirty()==std::array<bool,4>{},
              "W6 const record landfill query extracts bit15 at raw coords @ 0x10009a924");
    }
    // The external query is deliberately independent of the grid's bit15.
    // Only AL==0 inserts, only AL==1 removes; AL==2 never transitions.
    for(Uint32 prior:{0u,1u,2u,0x100u,0x101u,0x102u}) for(bool add:{false,true}) {
        F f; f.active=prior; f.g.raw=0x1234; f.g.shift=9;
        f.record.field2CC=add ? 0xffffffffu:0u;
        f.record.field328=f.record.field329=f.record.field32A=true;
        const bool transition=add ? (prior&255u)==0 : (prior&255u)==1;
        f.record.field32B=!transition;
        f.record.SetIsActiveLandfill(8,10,add);
        bool ok=f.ops()==(transition ? "MQGS":"MQ") && f.record.field2CC==(add && !transition ? 0xffffffffu:0u);
        if(transition) {
            const auto& e=f.trace.back();
            ok=ok && e.x==8 && e.z==10 && e.value==(add ? -28108:4660) &&
               e.count==0 && !e.dirty[3] && f.g.raw==(add ? 0x9234u:0x1234u);
        }
        Check(ok && f.dirty()==std::array<bool,4>{true,true,true,true},
              "W6 record landfill exact AL, raw coords, counter overflow/floor and no-transition preservation @ 0x10009a940");
    }
    {
        F f; f.mode=0x1000000eu; f.record.field32B=true;
        f.record.SetIsActiveLandfill(8,10,true);
        Check(f.ops()=="M" && f.record.field32B && f.record.field2CC==0,
              "W6 record landfill full-width mode rejection preserves dirty @ 0x10009a940");
        f.active=1; f.trace.clear(); f.record.field32B=false;
        f.record.SetIsActiveLandfill(8,10,false);
        Check(f.ops()=="MQGS" && f.record.field32B,
              "W6 record landfill removal allowed outside mode14 @ 0x10009a940");
    }
    {
        F f; f.a.shift=1;
        f.a.afterShift=[](F::GridMock& m) {
            auto& f=*m.owner;
            f.record.pollutionGridRuntime.air=F::grid(f.replacement);
        };
        f.replacement.afterSet=[](F::GridMock& m) {
            m.owner->record.field2CC=77; m.owner->record.field329=false;
        };
        f.record.SetAirValue(8,10,65535);
        Check(f.trace.back().id==4 && f.trace.back().x==4 && f.trace.back().value==-1 &&
              f.record.field2CC==77 && f.record.field329 &&
              f.record.pollutionGridRuntime.air.context==&f.replacement,
              "W6 record uses State directly across callback descriptor/storage mutations @ 0x10009a790");
    }
}

// CAPACITY_RECORD_CHECKS_BEGIN
// Record-boundary checks for GetCurrentCapacity @ 0x1000992cc.
struct CapacityRecordFixture final : OccupantSeam, OccupantPropertyHolderSeam,
    PollutionCapacityAccessSeam, RawCurvePropertySeam, RawCurveValueSeam {
    mutable PollutionSimulatorSaveRecord record{};
    mutable Uint32 out = 77;
    mutable std::string events;
    mutable bool identities = true;
    mutable unsigned holderCalls = 0;
    Uint32 capacity = 9;
    bool hasCapacity = true, powered = true, hasCurve = false, hasInput = true;
    Float32 input = 0;
    std::vector<Float32> scalars{0, 1};
    bool powerChangesScale = false;
    bool inputMutatesOut = false, dataThrows = false;
    CapacityRecordFixture() { record.capacityAccess = this; record.field348 = 1; }
    const OccupantPropertyHolderSeam* GetPropertyHolder() const override {
        events += 'H'; ++holderCalls; return this;
    }
    bool GetPropertyValue(Uint32, Sint64&) const override {
        events += 'X'; identities = false; return false;
    }
    bool GetPropertyValue(Uint32 id, Float32& value) const override {
        events += 'I'; identities &= id == 0xc910b4cfu && value == 0;
        if (inputMutatesOut) out = 7;
        if (hasInput) value = input;
        return hasInput;
    }
    const CurvePropertyData* GetProperty(Uint32) const override {
        events += 'X'; identities = false; return nullptr;
    }
    bool ReadCapacity(const OccupantPropertyHolderSeam& holder, Uint32 id,
                      Uint32& value) const override {
        events += 'R'; identities &= &holder == static_cast<const OccupantPropertyHolderSeam*>(this)
            && id == 0x1234u && &value == &out;
        value = capacity; return hasCapacity;
    }
    bool IsOccupantPowered(const OccupantSeam& occupant) const override {
        events += 'P'; identities &= &occupant == static_cast<const OccupantSeam*>(this)
            && out == capacity;
        if (powerChangesScale) record.field348 = 0.5f;
        return powered;
    }
    const RawCurvePropertySeam* LookupCurve(const OccupantPropertyHolderSeam& holder,
                                          Uint32 id) const override {
        events += 'L'; identities &= &holder == static_cast<const OccupantPropertyHolderSeam*>(this)
            && id == 0x8a1e07eeu;
        return hasCurve ? this : nullptr;
    }
    const RawCurveValueSeam* GetValue() const override { events += 'V'; return this; }
    const Float32* GetScalarData() const override {
        events += 'D'; if (dataThrows) throw 42;
        return scalars.data();
    }
    Uint32 GetScalarCount() const override {
        events += 'N'; return static_cast<Uint32>(scalars.size());
    }
    bool Run() const { return record.GetCurrentCapacity(0x1234u, *this, out); }
};

void CheckCurveNaNRegressions() {
// CURVE_NAN_REGRESSIONS_BEGIN (requires bit, pollution_capacity_kernel includes)
{
 using Curve=cSC4ResponseCurve;
 const auto f=[](unsigned u){return std::bit_cast<float>(u);};
 const auto b=[](float v){return std::bit_cast<unsigned>(v);};
 const float inf=f(0x7f800000u);
 const auto run=[&](const char* label,std::initializer_list<Curve::Point> pts,
                    float input,unsigned expected,unsigned expectedCapacity,bool slope) {
   Curve curve; curve.SetResponseCurveData(std::span(pts.begin(),pts.size()));
   float out=42; bool ok=curve.TryGetOutputValue(input,out);
   struct C { Curve* curve; float input; } ctx{&curve,input};
   using namespace pollution_capacity;
   Callbacks cb{
    [](void* p,void*)->void*{return p;},
    [](void*,void*,unsigned,unsigned& o){o=100;return true;},
    [](void*,void*){return true;},[](void*){return 1.0f;},
    [](void* p,void*,unsigned)->void*{return p;},
    [](void*,void*){},[](void*,void*,void*){},
    [](void* p,void*,unsigned,float& o){o=static_cast<C*>(p)->input;return true;},
    [](void* p,void*,float i){float o=0;static_cast<C*>(p)->curve->TryGetOutputValue(i,o);return o;},
    [](void*,void*) noexcept{}};
#if defined(__x86_64__) && defined(CURVE_NAN_VERBOSE)
   // Independent instruction oracle pins fixture constants; no compiler arithmetic.
   const auto& seg=curve.Segments().back(); float ref=input;
   asm volatile("subss %1,%0; mulss %2,%0; addss %3,%0"
       : "+x"(ref): "m"(seg.prevX),"m"(seg.slope),"m"(seg.prevY));
   Check(b(ref)==expected,"fixture expected bits pinned by explicit SSE");
   if(slope) {
     float numerator=inf,denominator=-inf;
     asm volatile("divss %1,%0":"+x"(numerator):"x"(denominator));
     Check(b(numerator)==expected,"slope expected bits pinned by explicit DIVSS");
   }
   const float capacity=100.0f,bias=12582912.0f;
   asm volatile("mulss %1,%0; addss %2,%0":"+x"(ref):"m"(capacity),"m"(bias));
   Check(b(ref)+0xb4c00000u==expectedCapacity,"capacity expected pinned by explicit SSE");
#endif
   unsigned cap=0; bool capOK=get_current_capacity({&ctx,&curve},cb,123,&ctx,cap);
   Check(ok && b(out)==expected && capOK && cap==expectedCapacity &&
         (!slope || (curve.Segments().size()>1 && b(curve.Segments()[1].slope)==expected)),label);
#ifdef CURVE_NAN_VERBOSE
   printf("%s out=%08x expected=%08x capacity=%08x expected=%08x\n",label,b(out),expected,cap,expectedCapacity);
#endif
 };
 run("invalid-sub",{{inf,1},{f(0x7fc12345),1}},inf,0xffc00000,0xb4800000,false);
 run("invalid-mul",{{-inf,1},{inf,1}},0,0xffc00000,0xb4800000,false);
 run("invalid-add",{{0,1},{1,inf},{2,1}},1.5f,0xffc00000,0xb4800000,false);
 run("invalid-div",{{-2,1},{-inf,inf},{2,1}},1,0xffc00000,0xb4800000,true);
 run("mixed-positive-sNaN",{{f(0x7f812345),1},{2,1}},f(0xffc54321),0xffc54321,0xb4854321,false);
 run("mixed-negative-sNaN",{{f(0xff812345),1},{2,1}},f(0xffc54321),0xffc54321,0xb4854321,false);
}
// CURVE_NAN_REGRESSIONS_END
}

void CheckPollutionCapacityRecordWrappers() {
    {
        CapacityRecordFixture f; f.hasCapacity = false;
        Check(!f.Run() && f.out == 0 && f.events == "HR" && f.identities,
              "capacity record missing uint32 property clears output and skips power @ 0x100099322");
    }
    {
        CapacityRecordFixture f; f.powerChangesScale = true;
        Check(f.Run() && f.out == 4 && f.events == "HRPHL" && f.identities && f.holderCalls == 2,
              "capacity record rereads holder and dynamic scale after power @ 0x10009933b");
    }
    {
        CapacityRecordFixture f; f.powered = false; f.record.field348 = 3;
        Check(f.Run() && f.out == 0 && f.events == "HRPHL" && f.identities,
              "capacity record unpowered zero still dispatches curve lookup @ 0x100099331");
    }
    {
        CapacityRecordFixture f; f.powered = false; f.record.field348 = std::numeric_limits<Float32>::infinity();
        Check(f.Run() && f.out == 0 && f.events == "HRPHL",
              "capacity record unpowered zero times infinity truncates low32 @ 0x100099344");
    }
    {
        CapacityRecordFixture f; f.capacity = 0x80000001u;
        Check(f.Run() && f.out == 0x80000000u && f.identities,
              "capacity record highbit capacity uses unsigned float conversion @ 0x10009931b");
    }
    {
        CapacityRecordFixture f; f.capacity = 0xffffffffu;
        Check(f.Run() && f.out == 0,
              "capacity record rounded uint32 max truncates through 64bit low32 @ 0x100099349");
    }
    {
        CapacityRecordFixture f; f.capacity = 0x80000001u; f.hasCurve = true;
        f.scalars = {0, 0x1p-30f};
        Check(f.Run() && f.out == 2 && f.identities,
              "capacity record curve multiplier uses unsigned saved highbit capacity @ 0x1000993b3");
    }
    {
        CapacityRecordFixture f; f.record.field348 = -0.5f;
        Check(f.Run() && f.out == 0xfffffffcu,
              "capacity record negative fractional scale truncates toward zero @ 0x100099344");
    }
    {
        CapacityRecordFixture f; f.hasCurve = true; f.scalars = {0, 0.5f, 999};
        Check(f.Run() && f.out == 4 && f.events == "HRPHLVDNI" && f.identities,
              "capacity record raw value data count dispatch floors odd tail and magic rounds tie @ 0x1000ac37a");
    }
    {
        CapacityRecordFixture f; f.hasCurve = true; f.hasInput = false;
        f.scalars = {0, 0.5f, 1, 1}; f.input = 1;
        Check(f.Run() && f.out == 4 && f.events == "HRPHLVDNI" && f.identities,
              "capacity record ignored missing float input retains initialized zero @ 0x10009938d");
    }
    {
        CapacityRecordFixture f; f.hasCurve = true; f.inputMutatesOut = true;
        f.scalars = {0, 1.0f};
        Check(f.Run() && f.out == 7 && f.identities,
              "capacity record input callback output mutation precedes capacity snapshot @ 0x1000993a0");
    }
    {
        CapacityRecordFixture f; f.hasCurve = true; f.scalars.clear();
        bool fault = false;
        try { (void)f.Run(); } catch (const ResponseCurveStorageFault&) { fault = true; }
        Check(fault && f.out == 9 && f.events == "HRPHLVDNI",
              "capacity record present empty curve faults distinctly without overwriting scaled out @ 0x1000ac542");
    }
    {
        CapacityRecordFixture f; f.hasCurve = true; f.scalars = {123};
        bool fault = false;
        try { (void)f.Run(); } catch (const ResponseCurveStorageFault&) { fault = true; }
        Check(fault && f.out == 9 && f.events == "HRPHLVDNI",
              "capacity record single odd scalar still clears fresh curve then faults @ 0x1000ac3cb");
    }
    {
        CapacityRecordFixture f; f.hasCurve = true; f.scalars = {0, 0};
        Check(f.Run() && f.out == 0 && f.events == "HRPHLVDNI",
              "capacity record valid zero response is success rather than storage fault @ 0x1000993cd");
    }
    {
        CapacityRecordFixture f; f.hasCurve = true; f.dataThrows = true;
        bool propagated = false;
        try { (void)f.Run(); } catch (int value) { propagated = value == 42; }
        f.dataThrows = false;
        const bool recovered = f.Run();
        Check(propagated && recovered && f.out == 9 && f.events == "HRPHLVDHRPHLVDNI",
              "capacity record raw dispatch exception unwinds curve and next invocation recovers @ 0x1000993f6");
    }
    {
        // Runtime seam must neither enter Write bytes nor be overwritten by Read.
        CapacityRecordFixture f;
        cIGZOStream withoutSeam, withSeam, reread;
        f.record.capacityAccess = nullptr;
        const bool wroteWithout = f.record.Write(withoutSeam);
        f.record.capacityAccess = &f;
        const bool wroteWith = f.record.Write(withSeam);
        cIGZIStream input(withSeam.Buffer().data(), withSeam.Buffer().size());
        const bool read = f.record.Read(input);
        Check(wroteWithout && wroteWith && read && f.record.Write(reread) &&
              f.record.capacityAccess == &f && withoutSeam.Buffer() == withSeam.Buffer() &&
              reread.Buffer() == withSeam.Buffer() && f.Run() && f.out == 9 && f.identities,
              "capacity record runtime source identity survives byte identical Read Write");
    }
}
// CAPACITY_RECORD_CHECKS_END

} // namespace

int main() {
    std::printf("================================================================\n");
    std::printf(" OpenSC4 Source-of-Truth Verification Suite\n");
    std::printf("================================================================\n");

    CheckPollutionRecordGridWrappers();
    CheckCurveNaNRegressions();
    CheckPollutionCapacityRecordWrappers();

    // ------------------------------------------------------------------------
    // 1. Verify cSC4SimGrid<T> against SC4SimGrid.cpp.inl invariants
    // ------------------------------------------------------------------------
    std::printf("\n--- 1. cSC4SimGrid<T> (SC4SimGrid.cpp.inl) ---\n");
    {
        cSC4SimGrid<Uint8> grid8;
        grid8.Init();
        grid8.SetInstanceID(0x49b9e603u);
        grid8.SetTractSize(4, 128, 128, 16.0f, 16.0f);

        Check(grid8.GetTractSize() == 4, "SimGrid TractSize == 4");
        Check(grid8.GetTractShift() == 2, "SimGrid TractShift == 2 (2^2 = 4)");
        Check(grid8.GetTractCountX() == 32 && grid8.GetTractCountZ() == 32, "SimGrid 128x128 city -> 32x32 tracts");

        grid8.SetTractValues(40);
        grid8.SetCellValue(8, 12, 120);
        Check(grid8.GetCellValue(8, 12) == 120, "SimGrid GetCellValue(8,12) == 120");
        Check(grid8.GetAverageValueInCellRect(8, 12, 4, 4) == 120, "SimGrid fast-path single-tract average == 120");

        // Binary stream roundtrip (Write -> Read)
        cIGZOStream out;
        Check(grid8.Write(out), "SimGrid Write(cIGZOStream&) succeeded");
        cIGZIStream in(out.Buffer());
        cSC4SimGrid<Uint8> decoded;
        Check(decoded.Read(in), "SimGrid Read(cIGZIStream&) succeeded");
        Check(decoded.GetInstanceID() == 0x49b9e603u && decoded.GetCellValue(8, 12) == 120,
              "SimGrid binary serialization roundtrip exact match");

        RZRandomState uintRandom{};
        uintRandom.seedA = 1;
        Check(RandomUint32Uniform(uintRandom, 0) == 0u && uintRandom.seedA == 1u,
              "cRZRandom::RandomUint32Uniform bound 0 returns 0 without advancing seedA @ 0x10003d366");
        const Uint32 uintCall1 = RandomUint32Uniform(uintRandom, 100);
        const Uint32 uintCall2 = RandomUint32Uniform(uintRandom, 100);
        const Uint32 uintCall3 = RandomUint32Uniform(uintRandom, 100);
        Check(uintCall1 == 0u && uintCall2 == 35u && uintCall3 == 74u &&
              uintRandom.seedA == 0x2781e494u,
              "cRZRandom::RandomUint32Uniform seed 1/bound 100 sequence 0,35,74 @ 0x10003d366");

        RZRandomState doubleRandom{};
        doubleRandom.seedB = 1;
        const Float64 doubleCall1 = RandomDoubleUniform(doubleRandom);
        const Float64 doubleCall2 = RandomDoubleUniform(doubleRandom);
        const Float64 doubleCall3 = RandomDoubleUniform(doubleRandom);
        Check(std::bit_cast<Uint64>(doubleCall1) == 0x3fe4f1bbd0000000ULL,
              "cRZRandom::RandomDoubleUniform call 1 exact Float32-scaled bits @ 0x10003d500");
        Check(std::bit_cast<Uint64>(doubleCall2) == 0x3fded4ce9a000000ULL,
              "cRZRandom::RandomDoubleUniform call 2 exact Float32-scaled bits @ 0x10003d500");
        Check(std::bit_cast<Uint64>(doubleCall3) == 0x3feb3d7d90000000ULL &&
              doubleRandom.seedB == 0x59ebecb5u,
              "cRZRandom::RandomDoubleUniform call 3 exact bits and wrapped seedB @ 0x10003d500");

        RZRandomState gaussianRandom{};
        gaussianRandom.seedB = 1;
        const Float64 gaussianCall1 = RandomDoubleGaussian(gaussianRandom);
        Check(std::bit_cast<Uint64>(gaussianCall1) == 0xbfd03b926dab1e18ULL &&
              std::bit_cast<Uint64>(gaussianRandom.field10) == 0x40012b077bb694f2ULL &&
              gaussianRandom.field08 == 1 && gaussianRandom.seedB == 0xfb533a69u,
              "cRZRandom::RandomDoubleGaussian call 1 exact return/cache/seed state @ 0x10003d540");
        const Float64 gaussianCall2 = RandomDoubleGaussian(gaussianRandom);
        Check(std::bit_cast<Uint64>(gaussianCall2) == 0x40012b077bb694f2ULL &&
              std::bit_cast<Uint64>(gaussianRandom.field10) == 0x40012b077bb694f2ULL &&
              gaussianRandom.field08 == 0 && gaussianRandom.seedB == 0xfb533a69u,
              "cRZRandom::RandomDoubleGaussian call 2 cached return does not advance seedB @ 0x10003d540");
        const Float64 gaussianCall3 = RandomDoubleGaussian(gaussianRandom);
        Check(std::bit_cast<Uint64>(gaussianCall3) == 0xbfdc0a505ba0dbbeULL &&
              std::bit_cast<Uint64>(gaussianRandom.field10) == 0x3fec4680f504d069ULL &&
              gaussianRandom.field08 == 1 && gaussianRandom.seedB == 0xd369bf11u,
              "cRZRandom::RandomDoubleGaussian call 3 exact return/cache/seed state @ 0x10003d540");

        constexpr std::array<std::ptrdiff_t, 10> expectedUniformBytes{
            92, 246, 238, 121, 44, 223, 5, 225, 186, 43,
        };
        std::array<std::ptrdiff_t, 10> actualUniformBytes{};
        std::mt19937 defaultEngine{};
        std::uniform_int_distribution<std::ptrdiff_t> byteDistribution(0, 255);
        for (std::ptrdiff_t& value : actualUniformBytes) {
            value = byteDistribution(defaultEngine);
        }
        Check(actualUniformBytes == expectedUniformBytes,
              "libc++ default mt19937 uniform_int_distribution<ptrdiff_t>(0,255) first ten @ 0x10009b36d");

        RandomIteratorState randomVector{};
        randomVector.values = {0, 0, 1, 1, 0};
        randomVector.Reset(false);
        const std::vector<Uint16> orderedRandom{0, 1, 256, 257};
        Check(randomVector.field0 == 1 && randomVector.field2 &&
              randomVector.values[4] == 4 && randomVector.indices == orderedRandom &&
              randomVector.tailValues == std::array<Sint32, 3>{},
              "RandomIterator::Reset(false) row-major fill and post-Reset fields @ 0x100213f58");
        randomVector.Reset(true);
        const std::vector<Uint16> shuffledRandom{0, 257, 256, 1};
        Check(randomVector.field0 == 1 && randomVector.field2 &&
              randomVector.values[4] == 4 && randomVector.indices == shuffledRandom &&
              randomVector.tailValues == std::array<Sint32, 3>{},
              "RandomIterator::Reset(true) exact forward Fisher-Yates permutation @ 0x100213f58 / 0x10009b36d");

        LinearRandomIteratorState linearVector{};
        linearVector.count = 4;
        linearVector.Reset(false);
        const std::vector<Uint32> orderedLinear{0, 1, 2, 3};
        Check(linearVector.field0 == 1 && linearVector.field2 && linearVector.count == 4 &&
              linearVector.indices == orderedLinear &&
              linearVector.tailValues == std::array<Sint32, 3>{},
              "LinearRandomIterator::Reset(false) linear fill and post-Reset fields @ 0x10008daf0");
        linearVector.Reset(true);
        const std::vector<Uint32> shuffledLinear{1, 0, 3, 2};
        Check(linearVector.field0 == 1 && linearVector.field2 && linearVector.count == 4 &&
              linearVector.indices == shuffledLinear &&
              linearVector.tailValues == std::array<Sint32, 3>{},
              "LinearRandomIterator::Reset(true) shared-engine Fisher-Yates permutation @ 0x10008daf0 / 0x1000c4afc");

        RandomIteratorState preservedRandom{};
        preservedRandom.values = {0, 0, 1, 1, 0};
        preservedRandom.indices = {9, 8, 7, 6};
        preservedRandom.Reset(false);
        Check(preservedRandom.indices == std::vector<Uint16>{9, 8, 7, 6} &&
              !preservedRandom.field2 && preservedRandom.values[4] == 4,
              "RandomIterator::Reset preserves correctly sized vector when field2 is zero @ 0x100213f58");

        LinearRandomIteratorState preservedLinear{};
        preservedLinear.count = 4;
        preservedLinear.indices = {9, 8, 7, 6};
        preservedLinear.Reset(false);
        Check(preservedLinear.indices == std::vector<Uint32>{9, 8, 7, 6} &&
              !preservedLinear.field2 && preservedLinear.count == 4,
              "LinearRandomIterator::Reset preserves correctly sized vector when field2 is zero @ 0x10008daf0");

        RandomIteratorState signedRectangle{};
        signedRectangle.values = {-1, 2, 0, 3, 0};
        signedRectangle.Reset(false);
        Check(signedRectangle.indices == std::vector<Uint16>{0xff02u, 0xff03u, 2, 3},
              "RandomIterator::Reset signed inclusive bounds and packed coordinates @ 0x100213f58");

        LinearRandomIteratorState emptyLinear{};
        emptyLinear.indices = {4, 5};
        emptyLinear.tailValues = {9, 9, 9};
        emptyLinear.Reset(true);
        Check(emptyLinear.field0 == 2 && emptyLinear.field2 &&
              emptyLinear.indices == std::vector<Uint32>{4, 5} &&
              emptyLinear.tailValues == std::array<Sint32, 3>{},
              "LinearRandomIterator::Reset zero-count early return preserves vector and clears tails @ 0x10008daf0");

        std::srand(1);
        const Sint32 firstRand = std::rand();
        const Sint32 secondRand = std::rand();
        RandomIteratorState directRandom{};
        directRandom.field1 = 1;
        directRandom.values = {0, 0, 6, 3, 0};
        directRandom.indices = {0x1234};
        directRandom.tailValues = {9, 9, 9};
        std::srand(1);
        directRandom.Reset(true);
        Check(firstRand == 16807 && secondRand == 282475249 &&
              directRandom.field0 == 1 && directRandom.values[4] == 28 &&
              directRandom.indices == std::vector<Uint16>{0x1234} &&
              directRandom.tailValues == std::array<Sint32, 3>{0, 7, 11},
              "RandomIterator::Reset(true) direct count 28 stores offset 7 and prime 11 @ 0x100213f58");
        Check(std::rand() == secondRand,
              "RandomIterator::Reset(true) direct path consumes exactly one rand() @ 0x100213f58");

        LinearRandomIteratorState directLinear{};
        directLinear.field1 = 1;
        directLinear.count = 28;
        directLinear.indices = {0x12345678};
        directLinear.tailValues = {9, 9, 9};
        std::srand(1);
        directLinear.Reset(true);
        Check(directLinear.field0 == 1 && directLinear.count == 28 &&
              directLinear.indices == std::vector<Uint32>{0x12345678} &&
              directLinear.tailValues == std::array<Sint32, 3>{0, 7, 11},
              "LinearRandomIterator::Reset(true) direct count 28 stores offset 7 and prime 11 @ 0x10008daf0");
        Check(std::rand() == secondRand,
              "LinearRandomIterator::Reset(true) direct path consumes exactly one rand() @ 0x10008daf0");

        // cSC4LandValueSimulator::DoIdle State 0/1 (reset + intrinsic fill) @ 0x1000c5e64
        {
            cSC4SimGrid<Uint8> lvWaterGrid;
            lvWaterGrid.Init();
            lvWaterGrid.SetTractSize(1, 128, 128, 16.0f, 16.0f);
            lvWaterGrid.SetTractValues(1);   // 1 = water everywhere
            lvWaterGrid.SetCellValue(0, 0, 0);   // land at city (0,0) -> tract (0,0) has land
            lvWaterGrid.SetCellValue(10, 10, 0);  // land at city (10,10) -> tract (5,5) has land

            cSC4SimGrid<Uint8> lvWorkingGrid;
            lvWorkingGrid.Init();
            lvWorkingGrid.SetTractSize(2, 128, 128, 16.0f, 16.0f);

            LandValueSimulatorSaveRecord lvRecord{};
            LandValueTuning lvTuning{};
            lvTuning.fieldD0 = 2;
            lvRecord.fieldD1 = 64;
            lvRecord.tuning = &lvTuning;
            lvRecord.workingGrid = &lvWorkingGrid;
            lvRecord.waterGrid = &lvWaterGrid;
            lvRecord.intrinsicIterator.minX = 0;
            lvRecord.intrinsicIterator.minZ = 1;
            lvRecord.intrinsicIterator.maxX = 63;
            lvRecord.intrinsicIterator.maxZ = 63;
            lvRecord.intrinsicIterator.step = 1;
            lvRecord.DoIdle(0);  // state 0: reset (zero x/z/status, emptiness checks) then state 1 fill
            Check(lvRecord.idleState == 2 && lvRecord.intrinsicIterator.status == 2,
"DoIdle State 0/1: reset + intrinsic fill completes, state 2 stored @ 0x1000c69b4");
            Check(lvWorkingGrid.GetTractValue(0, 0) == 64,
                  "DoIdle State 1: tract (0,0) fills fieldD1=64 (land at city corner) @ 0x1000c6dd8");
            Check(lvWorkingGrid.GetTractValue(5, 5) == 64,
                  "DoIdle State 1: tract (5,5) fills fieldD1=64 (land at city (10,10)) @ 0x1000c6dd8");
            Check(lvWorkingGrid.GetTractValue(1, 1) == 0,
                  "DoIdle State 1: tract (1,1) stays zero (all-water at city (2,2)) @ 0x1000c6dd8");
            Check(lvWorkingGrid.GetTractValue(63, 63) == 64,
                  "DoIdle State 1: last tract (63,63) fills fieldD1=64 (out-of-range +1 sample reads land @ 0x10073562c) @ 0x1000c6dd8");

            // Empty-rectangle reset shortcut (binary: minX > 0 jumps to status-2-store-and-exit @ 0x1000c5ed3)
            // Empty-rectangle reset semantics (minX > 0 / minZ <= 0 branch targets) are
            // deliberately TODO until the binary's iterator-bounds setup (Init) is
            // recovered exactly; the jumps at 0x1000c5ed3/0x1000c5ecd are noted in the
            // state-0 implementation but their exact iterator-bound configuration is
            // unpinned. Do not assert unpinned semantics.
        }

        // cSC4LandValueSimulator::DoIdle State 2 self-source distance pass
        // @ 0x1000c6038 / cSC4MapUtilities::AddDistanceEffect @ 0x1000d7af8.
        {
            cSC4SimGrid<Uint8> lvDistanceGrid;
            lvDistanceGrid.Init();
            lvDistanceGrid.SetTractSize(1, 5, 5, 16.0f, 16.0f);
            lvDistanceGrid.SetTractValues(64);
            lvDistanceGrid.SetTractValue(0, 0, 0);    // sole exclusion/distance-zero seed
            lvDistanceGrid.SetTractValue(1, 0, 250); // distance 1; exercises upper clamp

            LandValueSimulatorSaveRecord lvDistanceRecord{};
            lvDistanceRecord.idleState = 2;
            lvDistanceRecord.fieldD5 = 120; // near strength
            lvDistanceRecord.fieldD6 = 7;   // maximum distance
            lvDistanceRecord.intrinsicGrid = &lvDistanceGrid;
            lvDistanceRecord.workingGrid = &lvDistanceGrid;
            lvDistanceRecord.DoIdle(0);

            Check(lvDistanceRecord.idleState == 3 && lvDistanceGrid.GetTractValue(0, 0) == 0,
                  "DoIdle State 2 stores state 3 @ 0x1000c613f; exclusion seed stays zero @ 0x1000d7c7f");
            Check(lvDistanceGrid.GetTractValue(0, 1) == 166 &&
                  lvDistanceGrid.GetTractValue(1, 1) == 149 &&
                  lvDistanceGrid.GetTractValue(2, 4) == 81,
                  "AddDistanceEffect distances 1/2/6 truncate 166.857/149.714/81.142 to 166/149/81 @ 0x1000d7c8f-0x1000d7cae");
            Check(lvDistanceGrid.GetTractValue(1, 0) == 255,
                  "AddDistanceEffect distance 1 clamps 250 + 120 - 120/7 to 255 @ 0x1000d7cb2-0x1000d7cc1");

            // Manhattan distances from (0,0), in x-major grid order. Distance 7
            // equals fieldD6 and distance 8 exceeds it, so both remain unchanged.
            const cISC4SimGrid<Uint8>::GridData lvExpectedDistanceGrid{
                {0, 166, 149, 132, 115},
                {255, 149, 132, 115, 98},
                {149, 132, 115, 98, 81},
                {132, 115, 98, 81, 64},
                {115, 98, 81, 64, 64},
            };
            Check(lvDistanceGrid.GetGridData() == lvExpectedDistanceGrid,
                  "Expand x-major four-neighbour pass and strict distance < maxDistance exact 5x5 result @ 0x1000d7d0c / 0x1000d7c8a");
        }

        // cSC4LandValueSimulator::DoIdle State 3 zoned-land statistics and
        // histogram @ 0x1000c614d / completion @ 0x1000c6a7b.
        {
            cSC4SimGrid<Uint8> lvAltitudeGrid;
            lvAltitudeGrid.Init();
            lvAltitudeGrid.SetTractSize(2, 128, 128, 16.0f, 16.0f);

            cSC4SimGrid<Uint8> lvAltitudeWater;
            lvAltitudeWater.Init();
            lvAltitudeWater.SetTractSize(1, 128, 128, 16.0f, 16.0f);
            lvAltitudeWater.SetTractValues(1);
            lvAltitudeWater.SetCellValue(0, 0, 0); // tract (0,0): land, unzoned
            lvAltitudeWater.SetCellValue(0, 2, 0); // tract (0,1): land, one zoned cell
            lvAltitudeWater.SetCellValue(2, 0, 0); // tract (1,0): land, fully zoned
                                                   // tract (1,1): all water, zoned

            cSC4SimGrid<Sint8> lvAltitudeZones;
            lvAltitudeZones.Init();
            lvAltitudeZones.SetTractSize(1, 128, 128, 16.0f, 16.0f);
            lvAltitudeZones.SetCellValue(1, 3, 1); // sole zoned cell in tract (0,1)
            for (Sint32 x = 2; x < 4; ++x) {
                for (Sint32 z = 0; z < 2; ++z) {
                    lvAltitudeZones.SetCellValue(x, z, 1);
                }
            }
            lvAltitudeZones.SetCellValue(2, 2, 1); // water tract is never accumulated

            struct LvAltitudeOracle {
                std::array<Float32, 4> altitudes;
                Float32 minimum;
                Float32 maximum;
            } lvAltitudeOracle{{10.75f, 33.9f, 65.7f, 130.2f}, 0.0f, 256.0f};
            const auto sampleAltitude = [](const void* context, Float32 worldX,
                                           Float32 worldZ) noexcept -> Float32 {
                const auto* oracle = static_cast<const LvAltitudeOracle*>(context);
                const Sint32 tractX = static_cast<Sint32>(worldX / 32.0f);
                const Sint32 tractZ = static_cast<Sint32>(worldZ / 32.0f);
                return oracle->altitudes[static_cast<std::size_t>(tractX * 2 + tractZ)];
            };
            const auto minimumAltitude = [](const void* context) noexcept -> Float32 {
                return static_cast<const LvAltitudeOracle*>(context)->minimum;
            };
            const auto maximumAltitude = [](const void* context) noexcept -> Float32 {
                return static_cast<const LvAltitudeOracle*>(context)->maximum;
            };

            LandValueSimulatorSaveRecord lvAltitudeRecord{};
            lvAltitudeRecord.idleState = 3;
            lvAltitudeRecord.intrinsicGrid = &lvAltitudeGrid;
            lvAltitudeRecord.waterGrid = &lvAltitudeWater;
            lvAltitudeRecord.zoneGrid = &lvAltitudeZones;
            lvAltitudeRecord.terrainContext = &lvAltitudeOracle;
            lvAltitudeRecord.terrainGetAltitude = sampleAltitude;
            lvAltitudeRecord.terrainGetMinAltitude = minimumAltitude;
            lvAltitudeRecord.terrainGetMaxAltitude = maximumAltitude;
            lvAltitudeRecord.intrinsicIterator.maxX = 1;
            lvAltitudeRecord.intrinsicIterator.maxZ = 1;
            lvAltitudeRecord.altitudeHistogram90.assign(5, 0);
            lvAltitudeRecord.minAlt100 = 256.0f;
            lvAltitudeRecord.maxAlt104 = 0.0f;

            Check(LandValueSimulatorSaveRecord::TractIsZoned(
                      lvAltitudeGrid, lvAltitudeZones, 0, 1) &&
                  !LandValueSimulatorSaveRecord::TractIsZoned(
                      lvAltitudeGrid, lvAltitudeZones, 0, 0),
                  "TractIsZoned any-cell half-open 2x2 tract scan: one zoned cell suffices @ 0x1000c6d54 / 0x100199e64");

            lvAltitudeRecord.DoIdle(0);
            Check(lvAltitudeRecord.idleState == 5 &&
                  lvAltitudeRecord.intrinsicIterator.status == 2 &&
                  lvAltitudeRecord.slopeIterator.status == 0 &&
                  lvAltitudeRecord.slopeIterator.x == 0 &&
                  lvAltitudeRecord.slopeIterator.z == 0,
                  "DoIdle State 3 completion resets slope iterator and stores state 5 @ 0x1000c6b63-0x1000c6b8b");
            Check(lvAltitudeRecord.altitudeSum80 == 98 &&
                  lvAltitudeRecord.altitudeSampleCount84 == 2 &&
                  lvAltitudeRecord.fieldCC == 49.0f,
                  "DoIdle State 3 float-add/truncate 0+33.9->33, 33+65.7->98; average 98/2=49 @ 0x1000c62d5-0x1000c62f2 / 0x1000c6a93-0x1000c6aab");
            Check(lvAltitudeRecord.altitudeHistogram90 ==
                      std::vector<Sint32>{0, 1, 1, 0, 0},
                  "DoIdle State 3 fieldA8=5 buckets 33>>5=1 and 65>>5=2 @ 0x1000c62fa-0x1000c6330");
            Check(lvAltitudeRecord.modeAlt108 == 48.0f &&
                  lvAltitudeRecord.midAltAC == 48.0f &&
                  lvAltitudeRecord.maxEffectIndexB8 == 100,
                  "DoIdle State 3 tied histogram selects earliest bucket 1; mode=(1<<5)+16=48 @ 0x1000c6ae1-0x1000c6b1c");
            Check(lvAltitudeRecord.minAlt100 == 10.75f &&
                  lvAltitudeRecord.maxAlt104 == 65.7f,
                  "DoIdle State 3 extrema include unzoned land 10.75 but exclude water tract altitude 130.2 @ 0x1000c6274-0x1000c62a7");
        }

        // State 3 empty-zoned-pass fallback: the first invocation rewinds and
        // the second accumulates every land tract (@ 0x1000c6ab7-0x1000c6c76).
        {
            cSC4SimGrid<Uint8> lvFallbackGrid;
            lvFallbackGrid.Init();
            lvFallbackGrid.SetTractSize(2, 128, 128, 16.0f, 16.0f);
            cSC4SimGrid<Uint8> lvFallbackWater;
            lvFallbackWater.Init();
            lvFallbackWater.SetTractSize(1, 128, 128, 16.0f, 16.0f); // zero = all land
            cSC4SimGrid<Sint8> lvFallbackZones;
            lvFallbackZones.Init();
            lvFallbackZones.SetTractSize(1, 128, 128, 16.0f, 16.0f); // zero = all unzoned

            struct LvFallbackAltitudeOracle {
                std::array<Float32, 4> altitudes;
                Float32 maximum;
            } lvFallbackOracle{{-3.9f, 31.9f, 32.1f, 159.9f}, 256.0f};
            const auto sampleAltitude = [](const void* context, Float32 worldX,
                                           Float32 worldZ) noexcept -> Float32 {
                const auto* oracle = static_cast<const LvFallbackAltitudeOracle*>(context);
                const Sint32 tractX = static_cast<Sint32>(worldX / 32.0f);
                const Sint32 tractZ = static_cast<Sint32>(worldZ / 32.0f);
                return oracle->altitudes[static_cast<std::size_t>(tractX * 2 + tractZ)];
            };
            const auto maximumAltitude = [](const void* context) noexcept -> Float32 {
                return static_cast<const LvFallbackAltitudeOracle*>(context)->maximum;
            };

            LandValueSimulatorSaveRecord lvFallbackRecord{};
            lvFallbackRecord.idleState = 3;
            lvFallbackRecord.intrinsicGrid = &lvFallbackGrid;
            lvFallbackRecord.waterGrid = &lvFallbackWater;
            lvFallbackRecord.zoneGrid = &lvFallbackZones;
            lvFallbackRecord.terrainContext = &lvFallbackOracle;
            lvFallbackRecord.terrainGetAltitude = sampleAltitude;
            lvFallbackRecord.terrainGetMaxAltitude = maximumAltitude;
            lvFallbackRecord.intrinsicIterator.maxX = 1;
            lvFallbackRecord.intrinsicIterator.maxZ = 1;
            lvFallbackRecord.altitudeHistogram90.assign(4, 0);
            lvFallbackRecord.minAlt100 = 256.0f;
            lvFallbackRecord.maxAlt104 = 0.0f;

            lvFallbackRecord.DoIdle(0);
            Check(lvFallbackRecord.idleState == 3 && lvFallbackRecord.altitudeFallback88 &&
                  lvFallbackRecord.altitudeSum80 == 0 &&
                  lvFallbackRecord.altitudeSampleCount84 == 0 &&
                  lvFallbackRecord.intrinsicIterator.status == 0 &&
                  lvFallbackRecord.altitudeHistogram90 == std::vector<Sint32>(4, 0),
                  "DoIdle State 3 zero zoned samples reset iterator/statistics and arm all-land fallback @ 0x1000c6ab7-0x1000c6c1b");

            lvFallbackRecord.DoIdle(0);
            Check(lvFallbackRecord.altitudeSum80 == 219 &&
                  lvFallbackRecord.altitudeSampleCount84 == 4 &&
                  lvFallbackRecord.fieldCC == 54.75f &&
                  lvFallbackRecord.altitudeHistogram90 ==
                      std::vector<Sint32>{2, 1, 0, 1},
                  "DoIdle State 3 fallback truncates sums -3/28/60/219, average 54.75, negative bucket->0 and bucket 4->last 3 @ 0x1000c62d5-0x1000c6330");
            Check(lvFallbackRecord.idleState == 5 &&
                  lvFallbackRecord.modeAlt108 == 16.0f &&
                  lvFallbackRecord.minAlt100 == -3.9f &&
                  lvFallbackRecord.maxAlt104 == 159.9f,
                  "DoIdle State 3 fallback mode bucket 0 center=16 and exact extrema/state-5 completion @ 0x1000c6ae1-0x1000c6b8b");
        }

        // cSC4LandValueSimulator::DoIdle State 6 (desirability grid copy)
        // @ 0x1000c66ad and State 8 (average + history + cycle end)
        // @ 0x1000c6770.
        {
            // --- State 6: copy intrinsic +0x40 Data into desirability +0x50 ---
            cSC4SimGrid<Uint8> lvState6Intrinsic;
            lvState6Intrinsic.Init();
            lvState6Intrinsic.SetTractSize(2, 128, 128, 16.0f, 16.0f);
            lvState6Intrinsic.SetTractValue(0, 0, 7);
            lvState6Intrinsic.SetTractValue(1, 0, 42);
            lvState6Intrinsic.SetTractValue(0, 1, 99);
            lvState6Intrinsic.SetTractValue(1, 1, 255);

            cSC4SimGrid<Uint8> lvState6Desire;
            lvState6Desire.Init();
            lvState6Desire.SetTractSize(2, 128, 128, 16.0f, 16.0f); // all zero

            int lvState6Head = 0;
            LandValueSimulatorSaveRecord lvState6Record{};
            lvState6Record.idleState = 6;
            lvState6Record.intrinsicGrid = &lvState6Intrinsic;
            lvState6Record.workingGrid = &lvState6Desire;
            lvState6Record.growthDeveloperHead = &lvState6Head;
            lvState6Record.DoIdle(0);
            Check(lvState6Record.idleState == 7 &&
                  lvState6Record.growthDeveloperCursor == static_cast<void*>(&lvState6Head) &&
                  lvState6Desire.GetGridData() == lvState6Intrinsic.GetGridData(),
                  "DoIdle State 6 copies intrinsic +0x40 Data into +0x50 (assign @ 0x10010f188) and arms +0xc0 from +0xe8, state 7 @ 0x1000c66f0");

            // Same-vector case: intrinsic and desirability are the same object,
            // so the GridData addresses match and the assign branch (@ 0x1000c66cf)
            // is skipped.
            LandValueSimulatorSaveRecord lvState6Same{};
            lvState6Same.idleState = 6;
            lvState6Same.intrinsicGrid = &lvState6Intrinsic;
            lvState6Same.workingGrid = &lvState6Intrinsic; // same object
            lvState6Same.growthDeveloperHead = nullptr;
            const cISC4SimGrid<Uint8>::GridData lvState6Before =
                lvState6Intrinsic.GetGridData();
            lvState6Same.DoIdle(0);
            Check(lvState6Same.idleState == 7 &&
                  lvState6Same.growthDeveloperCursor == nullptr &&
                  lvState6Intrinsic.GetGridData() == lvState6Before,
                  "DoIdle State 6 same-vector case skips the assign (addresses equal @ 0x1000c66cf) and advances to state 7");

            // --- State 8: average land value (truncating idiv) + hooks ---
            struct LvState8Hooks {
                int historyCalls = 0;
                Uint32 historyId = 0;
                Uint8 historyValue = 0;
                int cycleCalls = 0;
                Uint32 cycleMessage = 0;
            };
            const auto historyEmit = [](void* context, Uint32 id, Uint8 value) noexcept {
                auto* hooks = static_cast<LvState8Hooks*>(context);
                ++hooks->historyCalls;
                hooks->historyId = id;
                hooks->historyValue = value;
            };
            const auto cycleEnd = [](void* owner, Uint32 messageId) noexcept {
                auto* hooks = static_cast<LvState8Hooks*>(owner);
                ++hooks->cycleCalls;
                hooks->cycleMessage = messageId;
            };

            cSC4SimGrid<Uint8> lvState8Intrinsic;
            lvState8Intrinsic.Init();
            lvState8Intrinsic.SetTractSize(2, 128, 128, 16.0f, 16.0f);

            cSC4SimGrid<Uint8> lvState8Water;
            lvState8Water.Init();
            lvState8Water.SetTractSize(1, 128, 128, 16.0f, 16.0f); // zero = land everywhere

            cSC4SimGrid<Sint8> lvState8Zones;
            lvState8Zones.Init();
            lvState8Zones.SetTractSize(1, 128, 128, 16.0f, 16.0f);
            lvState8Zones.SetTractValues(1); // zoned everywhere

            cSC4SimGrid<Uint8> lvState8LandValue;
            lvState8LandValue.Init();
            lvState8LandValue.SetTractSize(2, 128, 128, 16.0f, 16.0f);
            lvState8LandValue.SetTractValue(0, 0, 10);
            lvState8LandValue.SetTractValue(1, 0, 20);
            lvState8LandValue.SetTractValue(0, 1, 30);
            lvState8LandValue.SetTractValue(1, 1, 45); // sum 105, count 4 -> 26.25 -> 26

            LvState8Hooks hooks;
            LandValueSimulatorSaveRecord lvState8Record{};
            lvState8Record.idleState = 8;
            lvState8Record.intrinsicGrid = &lvState8Intrinsic;
            lvState8Record.waterGrid = &lvState8Water;
            lvState8Record.zoneGrid = &lvState8Zones;
            lvState8Record.landValueGrid = &lvState8LandValue;
            lvState8Record.intrinsicIterator.maxX = 1;
            lvState8Record.intrinsicIterator.maxZ = 1;
            lvState8Record.altitudeFallback88 = false;
            lvState8Record.altitudeSum80 = 0;
            lvState8Record.altitudeSampleCount84 = 0;
            lvState8Record.historyContext = &hooks;
            lvState8Record.emitHistory = historyEmit;
            lvState8Record.cycleOwner = &hooks;
            lvState8Record.endOfSimulationCycle = cycleEnd;

            lvState8Record.DoIdle(0);
            Check(lvState8Record.idleState == 9 &&
                  lvState8Record.altitudeSum80 == 105 &&
                  lvState8Record.altitudeSampleCount84 == 4 &&
                  lvState8Record.fieldC8 == 26u,
                  "DoIdle State 8 sums +0x48 grid (10+20+30+45=105) over 4 zoned land tracts and truncates 105/4=26 into fieldC8 @ 0x1000c68a3-0x1000c6a10");
            Check(hooks.historyCalls == 1 && hooks.historyId == 0x6a663ac8u &&
                  hooks.historyValue == 26u && hooks.cycleCalls == 1 &&
                  hooks.cycleMessage == 0x87cc75b3u,
                  "DoIdle State 8 emits history id 0x6a663ac8 with the average and sends end-of-cycle msg 0x87cc75b3 @ 0x1000c6a2d / 0x1000c738e");

            // count == 0 branch: all water -> TractHasLand false -> fieldC8 = 0,
            // history still emitted with 0, end-of-cycle still sent (@ 0x1000c6a0e).
            cSC4SimGrid<Uint8> lvState8AllWater;
            lvState8AllWater.Init();
            lvState8AllWater.SetTractSize(1, 128, 128, 16.0f, 16.0f);
            lvState8AllWater.SetTractValues(1); // water everywhere

            LvState8Hooks emptyHooks;
            LandValueSimulatorSaveRecord lvState8Empty{};
            lvState8Empty.idleState = 8;
            lvState8Empty.intrinsicGrid = &lvState8Intrinsic;
            lvState8Empty.waterGrid = &lvState8AllWater;
            lvState8Empty.zoneGrid = &lvState8Zones;
            lvState8Empty.landValueGrid = &lvState8LandValue;
            lvState8Empty.intrinsicIterator.maxX = 1;
            lvState8Empty.intrinsicIterator.maxZ = 1;
            lvState8Empty.historyContext = &emptyHooks;
            lvState8Empty.emitHistory = historyEmit;
            lvState8Empty.cycleOwner = &emptyHooks;
            lvState8Empty.endOfSimulationCycle = cycleEnd;

            lvState8Empty.DoIdle(0);
            Check(lvState8Empty.idleState == 9 && lvState8Empty.fieldC8 == 0u &&
                  lvState8Empty.altitudeSampleCount84 == 0 &&
                  emptyHooks.historyCalls == 1 && emptyHooks.historyValue == 0u &&
                  emptyHooks.cycleCalls == 1,
                  "DoIdle State 8 empty land pass leaves fieldC8=0 and still emits history/end-of-cycle @ 0x1000c6a0e");
        }

        // cSC4LandValueSimulator::DoIdle State 5 (float slope grid via
        // cSC4LotDeveloper::GetAreaSlope @ 0x1001478e8) @ 0x1000c650d and State 4
        // (altitude-effect apply) @ 0x1000c6363.
        {
            // State 5: slope = (maxAlt - minAlt) / LengthOfLine2D over the tract
            // vertices. With altitude = 50*x, each 2-cell tract's extremes are 2
            // cells apart in X, so slope = (100)/2 = 50 for every tract.
            const auto vertexAltitudeX = [](void*, Sint32 x, Sint32 /*z*/, bool) noexcept
                -> Float32 { return 50.0f * static_cast<Float32>(x); };

            cSC4SimGrid<Float32> lvSlopeGrid;
            lvSlopeGrid.Init();
            lvSlopeGrid.SetTractSize(2, 128, 128, 16.0f, 16.0f);

            LandValueSimulatorSaveRecord lvState5Record{};
            lvState5Record.idleState = 5;
            lvState5Record.slopeGrid = &lvSlopeGrid;
            lvState5Record.getLotVertexAltitude = vertexAltitudeX;
            lvState5Record.slopeIterator.maxX = 1;
            lvState5Record.slopeIterator.maxZ = 1;
            lvState5Record.slopeIterator.step = 1;
            lvState5Record.intrinsicIterator.maxX = 1;
            lvState5Record.intrinsicIterator.maxZ = 1;

            lvState5Record.DoIdle(0);
            Check(lvState5Record.idleState == 4 &&
                  lvSlopeGrid.GetTractValue(0, 0) == 50.0f &&
                  lvSlopeGrid.GetTractValue(1, 0) == 50.0f &&
                  lvSlopeGrid.GetTractValue(0, 1) == 50.0f &&
                  lvSlopeGrid.GetTractValue(1, 1) == 50.0f,
                  "DoIdle State 5 fills slope grid: altitude=50x, 2-cell tract extremes 2 apart -> 100/2 = 50, state 4 @ 0x1000c662b-0x1000c667a / 0x1000c69e4");

            // State 5 distance-compress cascade: a 4-cell tract whose extremes are
            // 4 apart with altitude=100x -> slope = 400 / (4*0.4) = 250
            // (4 > 3 -> *0.4 @ 0x100147a5c-0x100147a6d).
            const auto vertexAltitudeX2 = [](void*, Sint32 x, Sint32 /*z*/, bool) noexcept
                -> Float32 { return 100.0f * static_cast<Float32>(x); };
            cSC4SimGrid<Float32> lvSlopeGrid4;
            lvSlopeGrid4.Init();
            lvSlopeGrid4.SetTractSize(4, 128, 128, 16.0f, 16.0f);
            LandValueSimulatorSaveRecord lvState5bRecord{};
            lvState5bRecord.idleState = 5;
            lvState5bRecord.slopeGrid = &lvSlopeGrid4;
            lvState5bRecord.getLotVertexAltitude = vertexAltitudeX2;
            lvState5bRecord.slopeIterator.step = 1;
            lvState5bRecord.intrinsicIterator.maxX = 0;
            lvState5bRecord.intrinsicIterator.maxZ = 0;
            lvState5bRecord.DoIdle(0);
            Check(lvState5bRecord.idleState == 4 &&
                  std::fabs(lvSlopeGrid4.GetTractValue(0, 0) - 250.0f) < 0.01f,
                  "GetAreaSlope distance compress: 4-unit span > 3 -> *0.4 = 1.6; slope 400/1.6 ~= 250 @ 0x100147a5c-0x100147a6d");

            // State 4: intrinsic[x][z] = min(255, working[x][z] + effect) with
            // effect = LookupAltitudeEffect(100, constant-40 curve) = 40.
            const auto terrainAltitude100 = [](const void*, Float32, Float32) noexcept
                -> Float32 { return 100.0f; };
            cSC4ResponseCurve lvCurve;
            const std::array<Float32, 4> lvCurvePairs{0.0f, 40.0f, 100.0f, 40.0f};
            lvCurve.SetResponseCurveData(lvCurvePairs);

            cSC4SimGrid<Uint8> lvState4Intrinsic;
            lvState4Intrinsic.Init();
            lvState4Intrinsic.SetTractSize(2, 128, 128, 16.0f, 16.0f);
            cSC4SimGrid<Uint8> lvState4Working;
            lvState4Working.Init();
            lvState4Working.SetTractSize(2, 128, 128, 16.0f, 16.0f);
            lvState4Working.SetTractValue(0, 0, 0);
            lvState4Working.SetTractValue(1, 0, 100);
            lvState4Working.SetTractValue(0, 1, 210);
            lvState4Working.SetTractValue(1, 1, 250);

            LandValueSimulatorSaveRecord lvState4Record{};
            lvState4Record.idleState = 4;
            lvState4Record.intrinsicGrid = &lvState4Intrinsic;
            lvState4Record.workingGrid = &lvState4Working;
            lvState4Record.terrainContext = nullptr;
            lvState4Record.terrainGetAltitude = terrainAltitude100;
            lvState4Record.altitudeCurve = &lvCurve;
            lvState4Record.minAlt100 = 0.0f;
            lvState4Record.midAltAC = 50.0f;
            lvState4Record.lowerSpanB0 = 50.0f;
            lvState4Record.upperSpanB4 = 50.0f;
            lvState4Record.maxEffectIndexB8 = 100;
            lvState4Record.intrinsicIterator.maxX = 1;
            lvState4Record.intrinsicIterator.maxZ = 1;
            lvState4Record.DoIdle(0);
            Check(lvState4Record.idleState == 6 &&
                  lvState4Intrinsic.GetTractValue(0, 0) == 40 &&
                  lvState4Intrinsic.GetTractValue(1, 0) == 140 &&
                  lvState4Intrinsic.GetTractValue(0, 1) == 250 &&
                  lvState4Intrinsic.GetTractValue(1, 1) == 255 &&
                  lvState4Working.GetTractValue(1, 1) == 250,
                  "DoIdle State 4 intrinsic = min(255, working+40): 0/100/210/250 -> 40/140/250/255, working unchanged @ 0x1000c64b7-0x1000c64dd");

            // State 4 null-curve guard: no apply, intrinsic stays zero, state 6.
            LandValueSimulatorSaveRecord lvState4Null{};
            lvState4Null.idleState = 4;
            lvState4Null.intrinsicGrid = &lvState4Intrinsic;
            lvState4Null.workingGrid = &lvState4Working;
            lvState4Null.terrainGetAltitude = terrainAltitude100;
            lvState4Null.altitudeCurve = nullptr;
            lvState4Null.intrinsicIterator.maxX = 1;
            lvState4Null.intrinsicIterator.maxZ = 1;
            lvState4Intrinsic.SetTractValues(0); // reset so the no-op is observable
            lvState4Null.DoIdle(0);
            Check(lvState4Null.idleState == 6 &&
                  lvState4Intrinsic.GetTractValue(0, 0) == 0 &&
                  lvState4Intrinsic.GetTractValue(1, 1) == 0,
                  "DoIdle State 4 null curve [+0xd8]==0 skips apply and advances to state 6 @ 0x1000c6423-0x1000c642c");
        }

        // cSC4LandValueSimulator::DoIdle State 7 (one growth-developer
        // desirability grid per invocation) @ 0x1000c66fe. The list is modelled
        // as a pointer-ordered node array walked by the injected hooks; each node
        // carries a developer (whose +0x1f8 grid is GetDesirabilityMap @
        // 0x10015b188) and a Float32 weight (node +0x28, movss-loaded).
        {
            struct LvState7Node {
                void* developer = nullptr;
                Float32 weight = 0.0f;
            };
            static std::array<LvState7Node, 4> lvNodes;
            static int lvNodeCount = 0;
            static int lvEndToken = 0;

            const auto lvRead = [](void* cursor, void** developer, Float32* weight) noexcept {
                auto* node = static_cast<LvState7Node*>(cursor);
                *developer = node->developer;
                *weight = node->weight;
            };
            const auto lvAdvance = [](void* cursor) noexcept -> void* {
                auto* node = static_cast<LvState7Node*>(cursor);
                if (node + 1 < &lvNodes[lvNodeCount]) {
                    return node + 1;
                }
                return &lvEndToken;
            };
            const auto lvDesirabilityGrid = [](void* developer) noexcept
                -> const cSC4SimGrid<Sint16>* {
                return static_cast<const cSC4SimGrid<Sint16>*>(developer);
            };

            // Pass-through curve: GetOutputValue(x) = x for x in [0,255].
            cSC4ResponseCurve lvCurve;
            const std::array<Float32, 4> lvCurvePairs{0.0f, 0.0f, 255.0f, 255.0f};
            lvCurve.SetResponseCurveData(lvCurvePairs);

            // Working (desirability) grid +0x50 and land-value grid +0x48.
            cSC4SimGrid<Uint8> lvState7Working;
            lvState7Working.Init();
            lvState7Working.SetTractSize(1, 128, 128, 16.0f, 16.0f);
            lvState7Working.SetTractValue(0, 0, 0);
            lvState7Working.SetTractValue(1, 0, 10);
            lvState7Working.SetTractValue(0, 1, 30);
            lvState7Working.SetTractValue(1, 1, 250); // 250 + 20 -> clamp 255

            cSC4SimGrid<Uint8> lvState7LandValue;
            lvState7LandValue.Init();
            lvState7LandValue.SetTractSize(1, 128, 128, 16.0f, 16.0f);

            // Developer A: grid (+0x1f8) mostly 0 except 20 at (1,1); weight 1.0.
            cSC4SimGrid<Sint16> lvDevGridA;
            lvDevGridA.Init();
            lvDevGridA.SetTractSize(1, 128, 128, 16.0f, 16.0f);
            lvDevGridA.SetTractValue(1, 1, 20);
            // Developer B: grid all 5; weight 2.0.
            cSC4SimGrid<Sint16> lvDevGridB;
            lvDevGridB.Init();
            lvDevGridB.SetTractSize(1, 128, 128, 16.0f, 16.0f);
            lvDevGridB.SetTractValues(5);

            lvNodes[0] = LvState7Node{&lvDevGridA, 1.0f};
            lvNodes[1] = LvState7Node{&lvDevGridB, 2.0f};
            lvNodeCount = 2;

            LandValueSimulatorSaveRecord lvState7Record{};
            lvState7Record.idleState = 7;
            lvState7Record.growthDeveloperCursor = &lvNodes[0];
            lvState7Record.growthDeveloperEnd = &lvEndToken;
            lvState7Record.workingGrid = &lvState7Working;
            lvState7Record.landValueGrid = &lvState7LandValue;
            lvState7Record.growthCurve = &lvCurve;
            lvState7Record.readGrowthDeveloper = lvRead;
            lvState7Record.advanceGrowthDeveloper = lvAdvance;
            lvState7Record.getGrowthDeveloperDesirabilityMap = lvDesirabilityGrid;
            lvState7Record.intrinsicIterator.maxX = 1;
            lvState7Record.intrinsicIterator.maxZ = 1;

            // 1st invocation: apply developer A (weight 1, curve identity), then
            // advance the cursor to node 1; state stays 7.
            lvState7Record.DoIdle(0);
            Check(lvState7Record.idleState == 7 &&
                  lvState7Record.growthDeveloperCursor == static_cast<void*>(&lvNodes[1]) &&
                  lvState7Working.GetTractValue(0, 0) == 1 &&    // 0 + 0 -> clamp 1
                  lvState7Working.GetTractValue(1, 0) == 10 &&   // 10 + 0
                  lvState7Working.GetTractValue(0, 1) == 30 &&   // 30 + 0
                  lvState7Working.GetTractValue(1, 1) == 255,    // 250 + 20 -> 255
                  "DoIdle State 7 dev A (weight 1): 0+0->1, 10+0, 30+0, 250+20->255, cursor->node1, state 7 @ 0x1000c6717-0x1000c6746");

            // 2nd invocation: apply developer B (weight 2, curve identity), then
            // advance the cursor to the end sentinel; state stays 7.
            lvState7Record.DoIdle(0);
            Check(lvState7Record.idleState == 7 &&
                  lvState7Record.growthDeveloperCursor == static_cast<void*>(&lvEndToken) &&
                  lvState7Working.GetTractValue(0, 0) == 11 &&   // 1 + 5*2
                  lvState7Working.GetTractValue(1, 0) == 20 &&   // 10 + 10
                  lvState7Working.GetTractValue(0, 1) == 40 &&   // 30 + 10
                  lvState7Working.GetTractValue(1, 1) == 255,    // 255 + 10 -> 255
                  "DoIdle State 7 dev B (weight 2): +10 each, 255 stays clamped, cursor->end, state 7 @ 0x1000c6753-0x1000c6972");

            // 3rd invocation: cursor == end sentinel -> fallback copies the
            // desirability grid +0x50 into land-value +0x48, zeroes +0x80/+0x84,
            // resets the intrinsic iterator, and advances to state 8.
            lvState7Record.DoIdle(0);
            Check(lvState7Record.idleState == 8 &&
                  lvState7Record.altitudeSum80 == 0 &&
                  lvState7Record.altitudeSampleCount84 == 0 &&
                  lvState7Record.intrinsicIterator.status == 0 &&
                  lvState7LandValue.GetTractValue(0, 0) == 11 &&
                  lvState7LandValue.GetTractValue(1, 0) == 20 &&
                  lvState7LandValue.GetTractValue(0, 1) == 40 &&
                  lvState7LandValue.GetTractValue(1, 1) == 255,
                  "DoIdle State 7 end sentinel fallback copies +0x50->+0x48, zeroes +0x80, stores state 8 @ 0x1000c68e5-0x1000c6947");
        }

        // ProcessTwoGridsWithCallback across different tract resolutions
        cSC4SimGrid<Sint16> grid16;
        grid16.Init();
        grid16.SetTractSize(8, 128, 128, 16.0f, 16.0f);
        grid16.SetTractValues(10);
        ProcessTwoGridsWithCallback<Sint16, Uint8>(&grid16, &grid8, AddGridCallback);
        Check(grid16.GetTractValue(0, 0) == 50, "ProcessTwoGridsWithCallback downsamples 4x4 tracts into 8x8 tracts (10 + 40 = 50)");

        SC4DemandRecord demandCycle{};
        demandCycle.supplyValue = 40.0f;
        demandCycle.demandValue = 100.0f;
        demandCycle.newSupply = 10.0f;
        demandCycle.newDemand = 20.0f;
        demandCycle.activeDemandMin = -100.0f;
        demandCycle.activeDemandMax = 100.0f;
        demandCycle.economyModifier = 0.10f;
        demandCycle.taxModifier = -0.05f;
        demandCycle.demandCap = 0.20f;
        Check(demandCycle.EndCycleAndCalculate() &&
              demandCycle.supplyValue == 50.0f &&
              demandCycle.demandValue == 120.0f &&
              demandCycle.newSupply == 0.0f &&
              demandCycle.newDemand == 0.0f &&
              std::abs(demandCycle.activeDemand - 75.0f) < 0.001f,
              "cSC4Demand EndOfCycle/CalculateActiveDemand(false) exact formula");

        // --------------------------------------------------------------------
        // cSC4DemandSimulator runtime census/demand-index chain (M3)
        // --------------------------------------------------------------------
        std::printf("\n--- cSC4DemandSimulator runtime census/demand-index chain ---\n");
        {
            cSC4DemandSimulator censusSim;
            censusSim.PerformCensus({
                .buildingPurpose = 1u,
                .buildingWealth = 2u,
                .developerPopulation = 80,
                .buildingCapacity = 100u,
                .populationRetention = 0.5f,
                .educationQuotient = 120.0f,
            });
            censusSim.PerformCensus({
                .buildingPurpose = 1u,
                .buildingWealth = 2u,
                .developerPopulation = 100,
                .buildingCapacity = 20u,
                .populationRetention = 0.75f,
                .educationQuotient = 125.0f,
            });
            censusSim.PerformCensus({
                .buildingPurpose = 3u,
                .buildingWealth = 3u,
                .developerPopulation = 50,
                .buildingCapacity = 200u,
                .populationRetention = 1.0f,
                .educationQuotient = 0.0f,
            });
            Check(censusSim.censusInfo[0x1021u] == 100.0f &&
                  censusSim.censusInfo[0x2123u] == 55.0f &&
                  censusSim.censusInfo[0x3331u] == 100.0f,
                  "cSC4DemandSimulator census accumulates base 80+20=100, retained EQ 40+15=55, Co$$$ capacity 100 @ 0x100227bfe");

            cSC4DemandSimulator hopSim;
            hopSim.Demand(0x1000u).newDemand = 8.0f;
            hopSim.Demand(0x1001u);
            hopSim.Demand(0x1002u);
            hopSim.Demand(0x1003u).newDemand = 0.02f;
            hopSim.Demand(0x1004u);
            const std::array<DemandEconomyLink, 3> hopLinks{{
                {0x1000u, 1u, 0x1001u, DemandEconomyLinkType::DemandDrivesDemand, 0.5f},
                {0x1001u, 2u, 0x1002u, DemandEconomyLinkType::DemandDrivesSupply, 0.5f},
                {0x1003u, 3u, 0x1004u, DemandEconomyLinkType::DemandDrivesDemand, 0.5f},
            }};
            hopSim.SetEconomyLinks(hopLinks);
            const Uint32 hopPasses = hopSim.DoDemandIndexLinks();
            Check(hopPasses == 2u &&
                  hopSim.Demand(0x1000u).demandValue == 8.0f &&
                  hopSim.Demand(0x1001u).demandValue == 4.0f &&
                  hopSim.Demand(0x1002u).demandValue == 2.0f &&
                  hopSim.Demand(0x1004u).demandValue == 0.0f &&
                  hopSim.Demand(0x1004u).newDemand == 0.0f,
                  "cSC4DemandSimulator multi-hop 8->4->2 converges in 2 passes; strict 0.01 link delta is suppressed @ 0x100225380");

            cSC4DemandSimulator aggregateSim;
            aggregateSim.Demand(0x1001u);
            aggregateSim.Demand(0x1011u).newSupply = 100.0f;
            aggregateSim.Demand(0x23110u);
            aggregateSim.Demand(0x5000u).supplyValue = 100.0f;
            aggregateSim.Demand(0x5000u).demandValue = 25.0f;
            const std::array<DemandEconomyLink, 3> aggregateLinks{{
                {0x1001u, 1u, 0x1011u, DemandEconomyLinkType::AggregateChildren, 1.0f},
                {0x1011u, 2u, 0x23110u, DemandEconomyLinkType::SupplyDrivesSupply, 0.20f},
                {0x5000u, 3u, 0x23110u, DemandEconomyLinkType::SatisfactionLink, 1.0f},
            }};
            aggregateSim.SetEconomyLinks(aggregateLinks);
            const Uint32 aggregatePasses = aggregateSim.DoDemandIndexLinks();
            Check(aggregatePasses == 3u &&
                  aggregateSim.Demand(0x1001u).supplyValue == 100.0f &&
                  aggregateSim.Demand(0x1011u).supplyValue == 100.0f &&
                  aggregateSim.Demand(0x23110u).supplyValue == 20.0f &&
                  aggregateSim.Demand(0x23110u).demandCap == 0.25f,
                  "cSC4DemandSimulator aggregate/supply/satisfaction links yield 100/100/20 and cap 0.25 in 3 passes @ 0x100225380");

            const auto setTaxCurves = [](DemandSimulatorTuning& tuning) {
                const std::array<Float32, 4> neutral{0.0f, 10.0f, 1000.0f, 10.0f};
                const std::array<Float32, 4> modifier{0.0f, 0.0f, 1.0f, 1.0f};
                tuning.neutralTaxRateCurve.SetResponseCurveData(neutral);
                tuning.taxModifierCurve.SetResponseCurveData(modifier);
            };

            cSC4DemandSimulator taxSim;
            setTaxCurves(taxSim.tuning);
            taxSim.Demand(0x1001u).supplyValue = 100.0f;
            taxSim.Demand(0x3001u).supplyValue = 200.0f;
            taxSim.Demand(0x4001u).supplyValue = 300.0f;
            taxSim.taxEconomyInputs.averageTaxRates.fill(50.0f);
            taxSim.taxEconomyInputs.ordinanceTaxRates.fill(1.25f);
            taxSim.taxEconomyInputs.economyModifiers[6] = 0.25f;
            taxSim.taxEconomyInputs.economyModifiers[8] = 0.125f;
            taxSim.commercialConnectionBonus = 0.5f;
            taxSim.industrialConnectionBonus = 0.375f;
            const std::array<Float32, 4> identityNeutral{0.0f, 0.0f, 1000.0f, 1000.0f};
            taxSim.tuning.neutralTaxRateCurve.SetResponseCurveData(identityNeutral);
            const Float32 neutralTax = taxSim.GetNeutralTaxRate();
            setTaxCurves(taxSim.tuning); // state-0/update oracle uses neutral=10
            taxSim.UpdateTaxAndEconomyModifiers();
            Check(neutralTax == 600.0f &&
                  taxSim.Demand(0x21010u).taxModifier == 0.75f &&
                  taxSim.Demand(0x21010u).economyModifier == 0.0f &&
                  taxSim.Demand(0x23320u).taxModifier == 0.75f &&
                  taxSim.Demand(0x23320u).economyModifier == 0.75f &&
                  taxSim.Demand(0x24100u).taxModifier == 0.75f &&
                  taxSim.Demand(0x24100u).economyModifier == 0.5f,
                  "cSC4DemandSimulator neutral curve((200+100)+300)=600; update neutral=10 gives tax 0.75 and economy 0.75/0.5 @ 0x100225e44 / 0x10022718a");

            cSC4DemandSimulator idleSim;
            setTaxCurves(idleSim.tuning);
            idleSim.taxEconomyInputs.averageTaxRates.fill(50.0f);
            idleSim.taxEconomyInputs.ordinanceTaxRates.fill(1.25f);
            idleSim.tuning.salaryWeights = {
                10u, 20u, 30u, 40u, 50u, 60u,
                70u, 80u, 90u, 100u, 110u, 140u,
            };
            idleSim.jobsByCensus = {10, 20, 30, 40, 50, 60, 70, 80, 90};
            idleSim.Demand(0x1001u).supplyValue = 300.0f;
            idleSim.Demand(0x1011u).supplyValue = 5.0f;
            idleSim.Demand(0x1011u).regionUse = 1u;
            idleSim.Demand(0x2111u).supplyValue = 100.0f;
            idleSim.Demand(0x2111u).regionUse = 1u;
            idleSim.Demand(0x2122u).supplyValue = 100.0f;
            idleSim.Demand(0x2134u).supplyValue = 100.0f;
            idleSim.Demand(0x3111u).regionUse = 2u;
            idleSim.Demand(0x2000u).demandValue = 500.0f;

            const std::array<CensusCellInput, 2> idleCells{{
                {
                    .buildingPurpose = 1u,
                    .buildingWealth = 1u,
                    .developerPopulation = 100,
                    .buildingCapacity = 10u,
                    .populationRetention = 1.0f,
                    .educationQuotient = 0.0f,
                },
                {
                    .buildingPurpose = 2u,
                    .buildingWealth = 1u,
                    .developerPopulation = 100,
                    .buildingCapacity = 20u,
                    .populationRetention = 1.0f,
                    .educationQuotient = 0.0f,
                },
            }};
            idleSim.censusCellContext = const_cast<CensusCellInput*>(idleCells.data());
            idleSim.censusCellProvider = [](void* context, Sint32 x, Sint32 z,
                                             CensusCellInput& outInput) -> bool {
                if (z != 0 || x < 0 || x > 1) {
                    return false;
                }
                const auto* cells = static_cast<const CensusCellInput*>(context);
                outInput = cells[static_cast<std::size_t>(x)];
                return true;
            };
            idleSim.censusIterator.maxX = 1;
            idleSim.censusIterator.maxZ = 0;
            idleSim.idleState = 0;
            idleSim.DoIdle(0);
            Check(idleSim.idleState == 1u && idleSim.censusIterator.status == 0u &&
                  idleSim.censusInfo.size() == 24u &&
                  idleSim.censusInfo[0x1011u] == 0.0f &&
                  idleSim.Demand(0x21010u).taxModifier == 0.75f,
                  "cSC4DemandSimulator DoIdle state 0 updates modifiers, zeros 24 census keys, enters state 1 @ 0x1002264b2 / 0x100224a42");

            idleSim.DoIdle(0);
            Check(idleSim.idleState == 2u && idleSim.censusIterator.status == 2u &&
                  idleSim.censusInfo[0x1011u] == 10.0f &&
                  idleSim.censusInfo[0x2111u] == 10.0f &&
                  idleSim.censusInfo[0x3111u] == 20.0f &&
                  idleSim.Demand(0x1011u).newSupply == 10.0f &&
                  idleSim.Demand(0x2111u).newSupply == 10.0f &&
                  idleSim.Demand(0x3111u).newDemand == 20.0f,
                  "cSC4DemandSimulator DoIdle state 1 scans two cells and reconciles 10/10 supply + 20 demand into state 2 @ 0x1002264b2");

            idleSim.DoIdle(0);
            const auto historyValue = [&idleSim](Uint32 historyId) -> Sint32 {
                for (const auto& sample : idleSim.historySamples) {
                    if (sample.historyId == historyId) {
                        return sample.value;
                    }
                }
                return -1;
            };
            Check(idleSim.idleState == 3u && idleSim.historySamples.size() == 18u &&
                  historyValue(0x2a4e2003u) == 300 &&
                  historyValue(0x4a5e9485u) == 5 &&
                  historyValue(0x0a4e2056u) == 150 &&
                  historyValue(0x4a4e206bu) == 300 &&
                  historyValue(0x6a931184u) == 500 &&
                  historyValue(0x8a931172u) == 50,
                  "cSC4DemandSimulator DoIdle state 2 writes 18 history values: commercial 150, industrial 300, unemployment 50, enters state 3 @ 0x1002264b2");
            Check(idleSim.averageCitySalary == 70u && historyValue(0x8a63f30cu) == 70,
                  "cSC4DemandSimulator DoIdle salary (100*10+100*60+100*140)/300 = 70 @ 0x1002267be-0x10022682c");
        }

        BuildingDevelopmentState development{};
        Check(development.CalculateMonthlyResources(0.0f, 0.0f, 0.0f) == 100 &&
              development.CalculateMonthlyResources(400.0f, 300.0f, 300.0f) == 500 &&
              development.CalculateMonthlyResources(5000.0f, 5000.0f, 5000.0f) == 2000,
              "Building development monthly construction resources clamp to 100..2000");
        Check(BuildingDevelopmentState::IsGrowthDemandEligible(6.9f) &&
              !BuildingDevelopmentState::IsGrowthDemandEligible(5.9f) &&
              BuildingDevelopmentState::IsDeclinePlanningEligible(-21.0f) &&
              !BuildingDevelopmentState::IsDeclinePlanningEligible(-20.9f) &&
              BuildingDevelopmentState::IsDeclineExecutionEligible(-20.0f) &&
              !BuildingDevelopmentState::IsDeclineExecutionEligible(-19.9f),
              "Building development applies exact truncated growth/decline demand thresholds");

        TractCandidateScoreInput scoreInput{};
        scoreInput.candidateType = 3;
        scoreInput.existingCapacityA = 100;
        scoreInput.existingCapacityB = 50;
        scoreInput.sourceLotCount = 3;
        scoreInput.networkType0Or10.fill(1);
        scoreInput.networkType6.fill(1);
        scoreInput.networkType1.fill(1);
        TractDeveloperTuning tractTuning{};
        Check(tractTuning.ScoreCandidate(scoreInput, 1) == 182.0f &&
              tractTuning.ScoreCandidate(scoreInput, 2) == 102.0f &&
              TractDeveloperTuning::CompareCandidates(10.0f, 20.0f) &&
              !TractDeveloperTuning::CompareCandidates(20.0f, 10.0f),
              "cSC4TractDeveloper exact candidate scoring and ascending comparator");

        const TractCellRect orientation0 =
            ComputeCandidateBounds(10, 20, 3, 2, 0);
        const TractCellRect orientation1 =
            ComputeCandidateBounds(10, 20, 3, 2, 1);
        const TractCellRect orientation2 =
            ComputeCandidateBounds(10, 20, 3, 2, 2);
        const TractCellRect orientation3 =
            ComputeCandidateBounds(10, 20, 3, 2, 3);
        Check(orientation0.minX == 8 && orientation0.minZ == 20 &&
              orientation0.maxX == 10 && orientation0.maxZ == 21 &&
              orientation1.minX == 9 && orientation1.minZ == 18 &&
              orientation1.maxX == 10 && orientation1.maxZ == 20 &&
              orientation2.minX == 10 && orientation2.minZ == 19 &&
              orientation2.maxX == 12 && orientation2.maxZ == 20 &&
              orientation3.minX == 10 && orientation3.minZ == 20 &&
              orientation3.maxX == 11 && orientation3.maxZ == 22,
              "cSC4TractDeveloper exact GetBoundingRect orientation branches");

        std::array<Uint16, 16> networkMasks{};
        const auto setMask = [&](Sint32 x, Sint32 z, Uint16 mask) {
            networkMasks[static_cast<std::size_t>(z * 4 + x)] = mask;
        };
        setMask(1, 0, static_cast<Uint16>((1u << 3u) | (1u << 11u)));
        setMask(2, 0, static_cast<Uint16>(1u << 11u));
        setMask(3, 1, static_cast<Uint16>((1u << 0u) | (1u << 10u)));
        setMask(3, 2, static_cast<Uint16>(1u << 10u));
        setMask(0, 1, static_cast<Uint16>(1u << 6u));
        setMask(0, 2, static_cast<Uint16>(1u << 6u));
        setMask(1, 3, static_cast<Uint16>(1u << 1u));
        setMask(2, 3, static_cast<Uint16>(1u << 1u));

        TractCandidateScoreInput networkInput{};
        const TractCellRect interiorBounds{1, 1, 2, 2};
        const TractCellRect fullCityBounds{0, 0, 3, 3};
        ClassifyPerimeterNetworks(
            networkInput, interiorBounds, fullCityBounds, 4, 4, networkMasks);
        const std::array<Sint32, 4> side0{2, 0, 0, 0};
        const std::array<Sint32, 4> side1{0, 2, 0, 0};
        const std::array<Sint32, 4> side2{0, 0, 2, 0};
        const std::array<Sint32, 4> side3{0, 0, 0, 2};
        Check(networkInput.networkType3Or11 == side0 &&
              networkInput.networkType0Or10 == side1 &&
              networkInput.networkType6 == side3 &&
              networkInput.networkType1 == side2,
              "cSC4TractDeveloper exact FindNetworks side order and type classes");

        TractCandidateScoreInput type4NetworkInput{};
        type4NetworkInput.candidateType = 4;
        ClassifyPerimeterNetworks(
            type4NetworkInput, fullCityBounds, interiorBounds, 4, 4, networkMasks);
        Check(type4NetworkInput.networkType3Or11 == side0 &&
              type4NetworkInput.networkType0Or10 == side1 &&
              type4NetworkInput.networkType6 == side3 &&
              type4NetworkInput.networkType1 == side2,
              "cSC4TractDeveloper FindNetworks type-4 source-lot bounds branch");

        TractCandidateScoreInput clippedNetworkInput{};
        ClassifyPerimeterNetworks(
            clippedNetworkInput, fullCityBounds, interiorBounds, 4, 4, networkMasks);
        Check(clippedNetworkInput.networkType3Or11 ==
                  std::array<Sint32, 4>{} &&
              clippedNetworkInput.networkType0Or10 ==
                  std::array<Sint32, 4>{} &&
              clippedNetworkInput.networkType6 ==
                  std::array<Sint32, 4>{} &&
              clippedNetworkInput.networkType1 ==
                  std::array<Sint32, 4>{},
              "cSC4TractDeveloper FindNetworks clips all four city boundaries");

        // networkInput has:
        // side 0: networkType3Or11 = 2
        // side 1: networkType0Or10 = 2
        // side 2: networkType1 = 2 (rail only; not accepted by TestForCornerLot)
        // side 3: networkType6 = 2
        // For orientation 0, bit i maps to side (0 + i + 1) % 4:
        // bit 0 -> side 1 (pass), bit 1 -> side 2 (fail), bit 2 -> side 3 (pass),
        // bit 3 -> side 0 (pass).
        bool hasCornerConfig = false;
        const bool cornerBit0Pass = EvaluateCornerLotCompatibility(
            networkInput, 0, 0x01u, false, hasCornerConfig);
        const bool cornerBit1Fail =
            TestForCornerLot(networkInput, 0, 0x02u);
        bool nonCornerFlag = false;
        const bool requireCornerRejectsNonCorner =
            !EvaluateCornerLotCompatibility(
                networkInput, 0, 0x08u, true, nonCornerFlag) &&
            !nonCornerFlag;
        const bool bit3OnlyLeavesCornerFlagUnset =
            EvaluateCornerLotCompatibility(
                networkInput, 0, 0x08u, false, nonCornerFlag) &&
            !nonCornerFlag;
        Check(cornerBit0Pass && hasCornerConfig && !cornerBit1Fail &&
              TestForCornerLot(networkInput, 0, 0x05u) &&
              TestForCornerLot(networkInput, 2, 0x03u) &&
              requireCornerRejectsNonCorner &&
              bit3OnlyLeavesCornerFlagUnset,
              "cSC4TractDeveloper exact TestForCornerLot and corner-lot compatibility gate");

        const std::array<TractSourceLotInfo, 4> sourceLots{{
            {kTractLotOutcomeSuitable, 0, 2u, {1, 1, 1, 2}},
            {kTractLotOutcomeIneligibleRoad, 2, 2u, {2, 1, 2, 2}},
            {kTractLotOutcomeSuitable, 0, 3u, {2, 1, 2, 2}},
            {kTractLotOutcomeSuitable, 0, 2u, {2, 1, 3, 2}},
        }};
        const std::array<Sint32, 16> legalLotGrid{{
            -1, -1, -1, -1,
            -1,  0,  1, -1,
            -1,  0,  1, -1,
            -1, -1, -1, -1,
        }};
        Uint64 aggLotCount = 0;
        const bool legalAgg = IsAggregationLegal(
            interiorBounds, 0, 4, 4, legalLotGrid, sourceLots, aggLotCount);
        Uint64 wrongOrientCount = 0;
        const bool wrongOrientAgg = IsAggregationLegal(
            interiorBounds, 1, 4, 4, legalLotGrid, sourceLots, wrongOrientCount);
        Uint64 singleLotCount = 0;
        const bool singleLotAgg = IsAggregationLegal(
            {1, 1, 1, 2}, 0, 4, 4, legalLotGrid, sourceLots, singleLotCount);
        const std::array<Sint32, 16> mismatchedZoneGrid{{
            -1, -1, -1, -1,
            -1,  0,  2, -1,
            -1,  0,  2, -1,
            -1, -1, -1, -1,
        }};
        Uint64 mismatchedZoneCount = 0;
        const bool mismatchedZoneAgg = IsAggregationLegal(
            interiorBounds, 0, 4, 4, mismatchedZoneGrid, sourceLots,
            mismatchedZoneCount);
        const std::array<Sint32, 16> partialOverlapGrid{{
            -1, -1, -1, -1,
            -1,  0,  3,  3,
            -1,  0,  3,  3,
            -1, -1, -1, -1,
        }};
        Uint64 partialOverlapCount = 0;
        const bool partialOverlapAgg = IsAggregationLegal(
            interiorBounds, 0, 4, 4, partialOverlapGrid, sourceLots,
            partialOverlapCount);
        Check(legalAgg && aggLotCount == 2u && !wrongOrientAgg &&
              !singleLotAgg && singleLotCount == 1u &&
              !mismatchedZoneAgg && !partialOverlapAgg,
              "cSC4TractDeveloper exact IsAggregationLegal rules and containment check");

        Sint32 shiftedX0 = 0, shiftedZ0 = 0;
        Sint32 shiftedX1 = 0, shiftedZ1 = 0;
        Sint32 shiftedX2 = 0, shiftedZ2 = 0;
        Sint32 shiftedX3 = 0, shiftedZ3 = 0;
        ComputeAggregationShiftedOrigin(10, 20, 1, 3, 0, shiftedX0, shiftedZ0);
        ComputeAggregationShiftedOrigin(10, 20, 1, 3, 1, shiftedX1, shiftedZ1);
        ComputeAggregationShiftedOrigin(10, 20, 1, 3, 2, shiftedX2, shiftedZ2);
        ComputeAggregationShiftedOrigin(10, 20, 1, 3, 3, shiftedX3, shiftedZ3);
        Check(IsAggregationSizeCompatible(1, 2, 2, 2) &&
              !IsAggregationSizeCompatible(2, 2, 2, 2) &&
              !IsAggregationSizeCompatible(2, 2, 1, 3) &&
              shiftedX0 == 12 && shiftedZ0 == 20 &&
              shiftedX1 == 10 && shiftedZ1 == 22 &&
              shiftedX2 == 8 && shiftedZ2 == 20 &&
              shiftedX3 == 10 && shiftedZ3 == 18 &&
              ComputeSubdivisionCount(4, 2, 2, 2) == 2u &&
              ComputeSubdivisionCount(4, 2, 3, 2) == 0u &&
              ComputeSubdivisionCount(4, 2, 2, 1) == 0u &&
              ComputeSubdivisionCount(2, 2, 2, 2) == 0u,
              "cSC4TractDeveloper exact aggregation origin shifts and subdivision legality");

        TractCandidateScoreInput capCandidate{};
        capCandidate.candidateType = 3u;
        capCandidate.existingCapacityA = -1;
        capCandidate.existingCapacityB = -1;
        const std::array<TractExistingLotCapacityInput, 2> existingLots{{
            {45u, 3u, 2u, 1},
            {30u, 4u, 3u, 2},
        }};
        AccumulateExistingCapacity(capCandidate, existingLots);
        Check(capCandidate.existingCapacityA == 45 &&
              capCandidate.existingCapacityB == 30 &&
              capCandidate.maxExistingGrowthStage == 3 &&
              capCandidate.maxExistingBuildingWealth == 2 &&
              !IsRedevelopmentGrowthStageCompatible(capCandidate, 3u, true) &&
              IsRedevelopmentGrowthStageCompatible(capCandidate, 4u, true) &&
              IsRedevelopmentGrowthStageCompatible(capCandidate, 3u, false),
              "cSC4TractDeveloper exact FindExistingCapacity and redevelopment growth-stage gate");

        Check(GrowthStageFitsInZone(3, 1u) && !GrowthStageFitsInZone(4, 1u) &&
              GrowthStageFitsInZone(6, 2u) && !GrowthStageFitsInZone(7, 2u) &&
              GrowthStageFitsInZone(8, 3u) &&
              GrowthStageFitsInZone(3, 4u) && !GrowthStageFitsInZone(4, 4u) &&
              GrowthStageFitsInZone(6, 5u) && !GrowthStageFitsInZone(7, 5u) &&
              GrowthStageFitsInZone(8, 6u) &&
              GrowthStageFitsInZone(10, 7u) && GrowthStageFitsInZone(10, 8u) &&
              GrowthStageFitsInZone(10, 9u) && !GrowthStageFitsInZone(1, 0u) &&
              !GrowthStageFitsInZone(1, 10u) &&
              BuildingFitsInZone(1u, 1u) && BuildingFitsInZone(1u, 2u) &&
              BuildingFitsInZone(2u, 3u) && !BuildingFitsInZone(3u, 2u) &&
              BuildingFitsInZone(4u, 5u) && BuildingFitsInZone(5u, 6u) &&
              BuildingFitsInZone(8u, 9u) && !BuildingFitsInZone(7u, 9u) &&
              BuildingFitsInZone(0u, 11u) && BuildingFitsInZone(0u, 15u) &&
              ConvertZoneToDeveloperClass(1u) == 1u &&
              ConvertZoneToDeveloperClass(5u) == 2u &&
              ConvertZoneToDeveloperClass(9u) == 3u &&
              ConvertZoneToDeveloperClass(10u) == 0u &&
              ComputeConstructionBaseZoneType(1u, 1u) == 1u &&
              ComputeConstructionBaseZoneType(2u, 2u) == 4u &&
              ComputeConstructionBaseZoneType(3u, 5u) == 7u &&
              ComputeConstructionBaseZoneType(3u, 6u) == 8u &&
              ComputeConstructionBaseZoneType(3u, 8u) == 8u,
              "cSC4BuildingDevelopmentSimulator and cSC4GrowthDeveloper exact zone/stage compatibility");

        TractLotSuitabilityInput lotSuitInput{};
        lotSuitInput.developerBaseZoneType = 1u;
        lotSuitInput.developerWealth = 2u;
        lotSuitInput.developerPurpose = 1u;
        lotSuitInput.minLandValue = 20u;
        lotSuitInput.maxLandValue = 120u;
        lotSuitInput.targetGrowthStage = 4;
        lotSuitInput.allowLowerWealthRedevelopment = true;
        lotSuitInput.checkForNewDevelopment = true;
        lotSuitInput.cellZoneType = 2; // Medium Res
        lotSuitInput.cellLandValue = 50u;
        lotSuitInput.lotState = 1u;    // Ready/empty
        lotSuitInput.canCreateLotResult = 7u;
        lotSuitInput.hasRoadOrJobAccess = true;

        Sint32 rejectedLandValue = -1;
        const Uint32 okSuitability =
            EvaluateLotSuitability(lotSuitInput, rejectedLandValue);

        TractLotSuitabilityInput cooldownSuitInput = lotSuitInput;
        cooldownSuitInput.lotState = 3u; // Occupied/livable
        cooldownSuitInput.daysSinceLastStateChange = 179u;
        const Uint32 cooldownOutcome =
            EvaluateLotSuitability(cooldownSuitInput, rejectedLandValue);

        TractLotSuitabilityInput landValueSuitInput = lotSuitInput;
        landValueSuitInput.cellLandValue = 10u;
        const Uint32 landValueOutcome =
            EvaluateLotSuitability(landValueSuitInput, rejectedLandValue);

        TractLotSuitabilityInput agSuitInput = lotSuitInput;
        agSuitInput.lotState = 3u;
        agSuitInput.daysSinceLastStateChange = 200u;
        agSuitInput.existingBuildingType = 100u;
        agSuitInput.existingBuildingPurpose = 5u; // Agriculture
        agSuitInput.existingBuildingWealth = 1u;
        const Uint32 agOutcome =
            EvaluateLotSuitability(agSuitInput, rejectedLandValue);

        TractLotSuitabilityInput histSuitInput = agSuitInput;
        histSuitInput.existingBuildingPurpose = 1u;
        histSuitInput.isHistorical = true;
        const Uint32 histOutcome =
            EvaluateLotSuitability(histSuitInput, rejectedLandValue);

        TractLotSuitabilityInput sameWealthStageSuitInput = agSuitInput;
        sameWealthStageSuitInput.existingBuildingPurpose = 1u;
        sameWealthStageSuitInput.existingBuildingWealth = 2u;
        sameWealthStageSuitInput.hasExistingLotConfiguration = true;
        sameWealthStageSuitInput.existingLotConfigGrowthStage = 4u;
        const Uint32 sameWealthStageOutcome =
            EvaluateLotSuitability(sameWealthStageSuitInput, rejectedLandValue);

        Check(okSuitability == kTractLotOutcomeSuitable &&
              cooldownOutcome == kTractLotOutcomeOccupancyCooldown &&
              landValueOutcome == kTractLotOutcomeWrongLandValue &&
              rejectedLandValue == 10 &&
              agOutcome == kTractLotOutcomeCannotRedevelopPurpose &&
              histOutcome == kTractLotOutcomeHistorical &&
              sameWealthStageOutcome ==
                  kTractLotOutcomeTargetStageTooLowForExisting,
              "cSC4TractDeveloper exact IsLotSuitable cooldown, land-value, historical, and redevelopment gates");

        TractLotConfigSuitabilityInput cfgSuitInput{};
        cfgSuitInput.developerPurpose = 1u;
        cfgSuitInput.targetGrowthStage = 3;
        cfgSuitInput.tractCurrentPopulation = 20;
        cfgSuitInput.maxCapacityPerStep = 50;
        cfgSuitInput.maxOverbuildCapacity = 75;
        cfgSuitInput.allowSingleUnitMinCapacity = false;
        cfgSuitInput.cycleThroughStyles = true;
        cfgSuitInput.hasActiveStyles = true;
        cfgSuitInput.configGrowthStage = 2u;
        cfgSuitInput.minBuildingCapacity = 30u;
        cfgSuitInput.maxBuildingCapacity = 80u;
        const Uint32 cfgSuitOk = EvaluateLotConfigurationSuitability(cfgSuitInput);

        TractLotConfigSuitabilityInput cfgTooSmall = cfgSuitInput;
        cfgTooSmall.maxBuildingCapacity = 40u;
        const Uint32 cfgTooSmallOutcome =
            EvaluateLotConfigurationSuitability(cfgTooSmall);
        cfgTooSmall.allowSingleUnitMinCapacity = true;
        const Uint32 cfgPass2AllowsSmall =
            EvaluateLotConfigurationSuitability(cfgTooSmall);

        TractLotConfigCompatibilityInput cfgCompatInput{};
        cfgCompatInput.configSizeX = 2u;
        cfgCompatInput.configSizeZ = 2u;
        cfgCompatInput.candidateDimensionA = 2u;
        cfgCompatInput.candidateDimensionB = 2u;
        cfgCompatInput.candidateOrientation = 0;
        cfgCompatInput.zoneCompatible = true;
        cfgCompatInput.configGrowthStage = 4u;
        cfgCompatInput.configMatchesMaxExistingWealth = false;
        cfgCompatInput.candidateSlope = 3.5f;
        cfgCompatInput.configMinSlope = 1.0f;
        cfgCompatInput.configMaxSlope = 6.0f;
        cfgCompatInput.powerSupplyCap = 2u;
        cfgCompatInput.waterSupplyCap = 3u;
        cfgCompatInput.fireCoverageCap = 3u;
        cfgCompatInput.parkCap = 3u;
        cfgCompatInput.powerGridValue = 2u;
        cfgCompatInput.isWatered = true;
        cfgCompatInput.fireProtectionGridValue = 1u;
        cfgCompatInput.parkGridValue = 5u;
        cfgCompatInput.developerClass = 1u;
        cfgCompatInput.cornerMask = 0x01u;

        bool compatCornerOut = false;
        const Uint32 cfgCompatOk = EvaluateLotConfigurationCompatibility(
            networkInput, cfgCompatInput, compatCornerOut);
        TractLotConfigCompatibilityInput cfgNoWater = cfgCompatInput;
        cfgNoWater.isWatered = false;
        const Uint32 cfgNoWaterOutcome = EvaluateLotConfigurationCompatibility(
            networkInput, cfgNoWater, compatCornerOut);
        TractLotConfigCompatibilityInput cfgNoPark = cfgCompatInput;
        cfgNoPark.parkGridValue = 0u;
        const Uint32 cfgNoParkOutcome = EvaluateLotConfigurationCompatibility(
            networkInput, cfgNoPark, compatCornerOut);

        Sint32 minBldCap = 0, maxBldCap = 0;
        ComputePickBuildingCapacityRange(
            4u, 20, 2u, false, 100, 150, 30, 10, minBldCap, maxBldCap);
        TractBuildingCompatibilityInput bldCompatInput{};
        bldCompatInput.buildingPurpose = 2u;
        bldCompatInput.buildingWealth = 2u;
        bldCompatInput.buildingCapacity = 60u;
        bldCompatInput.buildingMatchesCurrentStyle = true;
        bldCompatInput.isMonopolyPiece = true;
        bldCompatInput.developerPurpose = 2u;
        bldCompatInput.developerWealth = 2u;
        bldCompatInput.developerMonopolyUse = 0u;
        bldCompatInput.hasActiveStyles = true;
        bldCompatInput.minBuildingCapacity = minBldCap;
        bldCompatInput.maxBuildingCapacity = maxBldCap;
        const Uint32 monopolyMode0Outcome =
            EvaluateBuildingCompatibility(bldCompatInput);
        bldCompatInput.developerMonopolyUse = 2u;
        const Uint32 monopolyMode2Outcome =
            EvaluateBuildingCompatibility(bldCompatInput);

        Check(cfgSuitOk == kTractLotOutcomeSuitable &&
              cfgTooSmallOutcome == kTractLotOutcomeMaxCapacityTooSmall &&
              cfgPass2AllowsSmall == kTractLotOutcomeSuitable &&
              cfgCompatOk == kTractLotOutcomeSuitable && compatCornerOut &&
              cfgNoWaterOutcome == kTractLotOutcomeNoWater &&
              cfgNoParkOutcome == kTractLotOutcomeNoPark &&
              minBldCap == 50 && maxBldCap == 140 &&
              monopolyMode0Outcome == kTractLotOutcomeMonopolyMismatch &&
              monopolyMode2Outcome == kTractLotOutcomeSuitable,
              "cSC4TractDeveloper exact LotConfiguration/Building compatibility, service caps, and PickBuilding capacity range");

        TractDeveloperState styleState{};
        styleState.currentStyleIndex = 1;
        styleState.yearsSinceLastStyleChange = 1;
        styleState.cycleThroughStyles = true;
        styleState.activeStyles = {0x2000u, 0x2001u, 0x2002u, 0x2003u};
        styleState.yearsBetweenStyleChanges = 2;
        cIGZOStream styleOut;
        styleState.Write(styleOut);
        cIGZIStream styleIn(styleOut.Buffer());
        TractDeveloperState styleRoundtrip{};
        const bool styleReadOk = styleRoundtrip.Read(styleIn);
        styleRoundtrip.OnNewYear();
        const Uint32 rotatedStyle = styleRoundtrip.GetCurrentStyle();
        Check(styleReadOk &&
              styleRoundtrip.activeStyles.size() == 4u &&
              styleRoundtrip.yearsBetweenStyleChanges == 2 &&
              styleRoundtrip.currentStyleIndex == 2 &&
              styleRoundtrip.yearsSinceLastStyleChange == 0 &&
              rotatedStyle == 0x2002u,
              "cSC4TractDeveloper exact Read/Write serialization and DoMessage yearly style rotation");

        const std::array<TractStylePercentageEntry, 3> styleWeights{{
            {0x2000u, 0.25f},
            {0x2001u, 0.50f},
            {0x2002u, 0.25f},
        }};
        Check(SelectPickBuildingStyle(true, styleWeights, 10u) ==
                  kTractAnchorBuildingFamily &&
              SelectPickBuildingStyle(false, styleWeights, 24u) == 0x2000u &&
              SelectPickBuildingStyle(false, styleWeights, 25u) == 0x2001u &&
              SelectPickBuildingStyle(false, styleWeights, 74u) == 0x2001u &&
              SelectPickBuildingStyle(false, styleWeights, 75u) == 0x2002u &&
              SelectPickBuildingStyle(false, styleWeights, 99u) == 0x2002u,
              "cSC4TractDeveloper exact PickBuilding anchor family (0x3000) and style-percentage roll");

        Sint32 minTakeover = 0, maxTakeover = 0;
        ComputeTakeOverAbandonedThresholds(
            100, 150, 20, 0.25f, 1.2f, minTakeover, maxTakeover);

        TractTakeOverAbandonedLotInput takeoverInput{};
        takeoverInput.lotState = 3u; // Occupied by lower wealth
        takeoverInput.occupantWealth = 1u;
        takeoverInput.existingBuildingType = 500u;
        takeoverInput.existingBuildingWealth = 3u;
        takeoverInput.developerWealth = 2u;
        takeoverInput.developerClass = 1u;
        takeoverInput.lotZoneType = 2u;
        takeoverInput.allowLowerWealthRedevelopment = true;
        takeoverInput.totalCapacityForDeveloper = 80u;
        takeoverInput.availableCapacityForDeveloper = 65u;
        takeoverInput.suitabilityOutcome = kTractLotOutcomeHistorical; // 0x12 allowed!
        takeoverInput.hasExistingLotConfiguration = true;
        takeoverInput.existingLotConfigGrowthStage = 4u;
        takeoverInput.existingDeveloperPowerSupplyCap = 2u;
        takeoverInput.existingDeveloperWaterSupplyCap = 3u;
        takeoverInput.currentDeveloperFireCoverageCap = 3u;
        takeoverInput.currentDeveloperParkCap = 3u;
        takeoverInput.powerGridValue = 2u;
        takeoverInput.isWatered = true;
        takeoverInput.fireProtectionGridValue = 1u;
        takeoverInput.parkGridValue = 1u;

        Sint32 takeoverCap = 0;
        Uint32 takeoverOutcome = kTractLotOutcomeNoAbandonedTakenOver;
        const bool takeoverOk = EvaluateTakeOverAbandonedLot(
            takeoverInput, takeoverCap, takeoverOutcome);

        TractTakeOverAbandonedLotInput takeoverNoWater = takeoverInput;
        takeoverNoWater.isWatered = false;
        Sint32 takeoverNoWaterCap = 0;
        Uint32 takeoverNoWaterOutcome = kTractLotOutcomeNoAbandonedTakenOver;
        const bool takeoverNoWaterOk = EvaluateTakeOverAbandonedLot(
            takeoverNoWater, takeoverNoWaterCap, takeoverNoWaterOutcome);

        Check(minTakeover == 20 && maxTakeover == 156 &&
              takeoverOk && takeoverCap == 65 &&
              !takeoverNoWaterOk &&
              takeoverNoWaterOutcome == kTractLotOutcomeNoWater,
              "cSC4TractDeveloper exact TakeOverAbandoned thresholds, dilapidated/historical eligibility, and service caps");

        const Uint32 popSeed = ComputeUpdatePopulationSeed(5, 12);
        const Uint16 normalPop =
            ComputeUpdatedLotPopulation(80, 200u, 0.05, 0.05);
        const Uint16 overflowPop =
            ComputeUpdatedLotPopulation(95, 200u, 0.15, 0.05);

        TractDeclineLotInput declineInput{};
        declineInput.lotState = 3u;
        declineInput.daysSinceLastStateChange = 60u;
        declineInput.existingBuildingType = 42u;
        declineInput.existingBuildingPurpose = 2u;
        declineInput.occupantWealth = 2u;
        declineInput.developerPurpose = 2u;
        declineInput.developerWealth = 2u;
        TractDeclineLotInput earlyDeclineInput = declineInput;
        earlyDeclineInput.daysSinceLastStateChange = 59u;

        Sint32 growMaxStep = 0, growMaxOverbuild = 0, growMaxRetries = 0;
        ComputeGrowInitialParameters(
            120.8f, 1.5f, 4, growMaxStep, growMaxOverbuild, growMaxRetries);
        Uint16 claimedFarmRes = 0u;
        const bool farmTooFewLots =
            CanStartFarmBuild(14u, 100u, claimedFarmRes);
        const bool farmNotEnoughRes =
            CanStartFarmBuild(15u, 44u, claimedFarmRes);
        const bool farmOk =
            CanStartFarmBuild(15u, 45u, claimedFarmRes);

        Check(popSeed == ((5u << 20u) + (12u << 4u)) &&
              normalPop == 168u && overflowPop == 190u &&
              IsLotEligibleForDecline(declineInput) &&
              !IsLotEligibleForDecline(earlyDeclineInput) &&
              growMaxStep == 120 && growMaxOverbuild == 180 &&
              growMaxRetries == 17 &&
              !farmTooFewLots && !farmNotEnoughRes &&
              farmOk && claimedFarmRes == 45u,
              "cSC4TractDeveloper exact UpdatePopulation, Decline 60-day gate, Grow parameters, and BuildFarm resource gate");

        cSC4ResponseCurve curve;
        const std::array<Float32, 6> curvePairs{
            0.0f, 10.0f,
            50.0f, 60.0f,
            100.0f, 160.0f,
        };
        const bool curveSetOk = curve.SetResponseCurveData(curvePairs);
        Float32 maxInput = 0.0f;
        const bool hasMaxInput = curve.GetMaxInputValue(maxInput);
        Check(curveSetOk && hasMaxInput && maxInput == 100.0f &&
              curve.GetOutputValue(-10.0f) == 10.0f &&
              curve.GetOutputValue(0.0f) == 10.0f &&
              curve.GetOutputValue(25.0f) == 35.0f &&
              curve.GetOutputValue(75.0f) == 110.0f &&
              curve.GetOutputValue(150.0f) == 160.0f,
              "cSC4ResponseCurve exact binary search, interpolation, and clamping");

// CURVE_REGRESSIONS_BEGIN
        // Binary boundary fixtures: setter @ 0x1000ac220, property @ 0x1000ac37a,
        // getter @ 0x1000ac4f6. Historical old-code hangs recorded by independent
        // review; execute these only against the fixed terminating search.
        {
            using Curve = cSC4ResponseCurve;
            using Point = Curve::Point;
            const Float32 nan = std::numeric_limits<Float32>::quiet_NaN();
            const auto set = [](Curve& c, std::initializer_list<Point> pts) {
                return c.SetResponseCurveData(std::span<const Point>(pts.begin(), pts.size()));
            };
            Curve c;
            Float32 out = 123.0f;
            Check(!c.TryGetOutputValue(nan, out) && out == 123.0f,
                  "curve fresh missing storage reports fault without changing output");
            set(c, {{0, 1}, {1, 2}});
            Check(c.TryGetOutputValue(nan, out) && std::isnan(out),
                  "curve unordered input terminates and interpolates NaN");
            set(c, {{0, 1}, {nan, 1}, {2, 1}});
            Check(c.Segments().size() == 3 && c.TryGetOutputValue(0.5f, out) && std::isnan(out),
                  "curve unordered midX terminates and retains input predecessor NaN");
            set(c, {{0, 1}, {1, nan}});
            const bool nanDYSkipped = c.Segments().size() == 1;
            set(c, {{0, 1}, {nan, 2}});
            Check(nanDYSkipped && c.Segments().size() == 1,
                  "curve unordered dy and dx skip segments");
            Curve freshSkipped;
            out = 321.0f;
            const bool allSkipped = set(freshSkipped, {{0, nan}, {1, nan}});
            Check(allSkipped && freshSkipped.Empty() &&
                  !freshSkipped.TryGetOutputValue(0, out) && out == 321.0f,
                  "curve nonempty all-skipped input succeeds without backing slot");
            set(c, {{0, 7}});
            ExemplarProperty emptyProperty;
            const bool presentEmpty = c.SetResponseCurveData(emptyProperty);
            Check(!presentEmpty && c.Empty() && c.TryGetOutputValue(0, out) && out == 7,
                  "curve present empty property clears logical segments and retains Y7");
            const bool retainedSkipped = set(c, {{0, nan}});
            Check(retainedSkipped && c.Empty() && c.TryGetOutputValue(nan, out) && out == 7,
                  "curve all-skipped input preserves earlier backing-slot Y");
            // First two input points skip (NaN dy); first accepted point is index 2.
            set(c, {{0, nan}, {1, 9}, {2, 9}});
            const bool acceptedLate = c.Segments().size() == 1;
            const bool cleared = !c.SetResponseCurveData(std::span<const Point>{});
            Curve copied(c);
            Curve assigned; assigned = c;
            Check(acceptedLate && cleared && c.Empty() &&
                  c.TryGetOutputValue(0, out) && out == 9 &&
                  copied.TryGetOutputValue(0, out) && out == 9 &&
                  assigned.TryGetOutputValue(0, out) && out == 9,
                  "curve next first accepted segment updates retained Y and copies preserve it");
            ExemplarProperty oddProperty;
            oddProperty.floatValues = {0, 3, 99};
            const bool oddSet = c.SetResponseCurveData(oddProperty);
            Check(oddSet && c.Segments().size() == 1 && c.GetOutputValue(99) == 3,
                  "curve present raw property floors odd scalar count");
            const Float32 above = 1e-10f;
            const Float32 below = std::nextafter(above, 0.0f);
            set(c, {{0, 0}, {1, below}});
            const bool belowFlat = c.Segments().size() == 2 && c.Segments()[1].slope == 0;
            set(c, {{0, 0}, {1, above}});
            Check(static_cast<double>(below) < 1e-10 && static_cast<double>(above) > 1e-10 &&
                  belowFlat && c.Segments().size() == 2 && c.Segments()[1].slope == above,
                  "curve epsilon binary32 neighbors distinguish flat from division");
            set(c, {{-0.0f, -0.0f}, {0.0f, 0.0f}});
            const bool zeroSlope = c.Segments().size() == 2 &&
                !std::signbit(c.Segments()[0].slope) && !std::signbit(c.Segments()[1].slope);
            const bool negativeY = c.TryGetOutputValue(-1, out) && std::signbit(out);
            const bool positiveY = c.TryGetOutputValue(0, out) && !std::signbit(out);
            Check(zeroSlope && negativeY && positiveY,
                  "curve signed zeros preserve Y signs and positive zero slopes");
            set(c, {{0, 2}, {0, 1}});
            Check(c.Segments().size() == 2 && std::isinf(c.Segments()[1].slope) &&
                  std::signbit(c.Segments()[1].slope),
                  "curve duplicate x negative dy admits division by positive zero");
            set(c, {{0, 0}, {0, 10}, {2, 14}});
            Check(c.Segments().size() == 2 && c.Segments()[1].prevX == 0 &&
                  c.Segments()[1].prevY == 10 && c.Segments()[1].slope == 2 &&
                  c.GetOutputValue(1) == 12,
                  "curve skipped input remains predecessor for following accepted segment");
        }
// CURVE_REGRESSIONS_END

        Check(GrowthDeveloperSimGridIDs::ZonableCellsGridId(0x1010u) == 0x01001010u &&
              GrowthDeveloperSimGridIDs::DesirabilityGridId(0x1010u) == 0x02001010u &&
              GrowthDeveloperSimGridIDs::TempDesirabilityGridId(0x1010u) == 0x02101010u &&
              GrowthDeveloperSimGridIDs::PopulationGridId(0x1010u) == 0x03001010u &&
              GrowthDeveloperSimGridIDs::CapacityGridId(0x1010u) == 0x05001010u &&
              GrowthDeveloperSimGridIDs::MaxCapacityGridId(0x1010u) == 0x06001010u,
              "cSC4GrowthDeveloper exact SimGrid Data ID mapping (0x01..0x06 + subType)");

        GrowthDeveloperTuning devTuning{};
        devTuning.developerClass = 1u;
        devTuning.baseDesirability = 50;
        devTuning.desirabilityThreshold = 60;
        devTuning.declineDesirabilityThreshold = 40;
        devTuning.desirabilityFeedsDemandSupply = true;
        devTuning.landValueCurve = curve;
        devTuning.stageCurves.resize(3);
        const std::array<Float32, 4> stage1Pairs{0.0f, 50.0f, 1000.0f, 50.0f};
        const std::array<Float32, 4> stage2Pairs{0.0f, 25.0f, 1000.0f, 25.0f};
        const std::array<Float32, 4> stage3Pairs{0.0f, 25.0f, 1000.0f, 25.0f};
        devTuning.stageCurves[0].SetResponseCurveData(stage1Pairs);
        devTuning.stageCurves[1].SetResponseCurveData(stage2Pairs);
        devTuning.stageCurves[2].SetResponseCurveData(stage3Pairs);

        GrowthDesirabilityInputs desInputs{};
        desInputs.landValue = 25u; // curve(25) = 35 -> 50 + 35 = 85
        const Sint16 evalDes = devTuning.EvaluateDesirability(desInputs);

        const std::array<Sint8, 4> zonableTracts{1, 1, 1, 1};
        const std::array<Sint16, 4> tractDes{90, 70, 80, 30};
        const std::array<Sint16, 4> tractPop{100, 100, 100, 200};
        std::vector<GrowthDeveloperItem> growthList;
        std::vector<GrowthDeveloperItem> declineList;
        const Float32 stagePop = ComputeStageLookupPopulation(
            1u, 500.0f, 300.0f, 200.0f, 100.0f, 1, 2, 1.0f);
        BuildGrowthAndDeclineLists(
            devTuning, zonableTracts, tractDes, 2, 2, stagePop,
            growthList, declineList);

        const std::array<GrowthDeveloperItem, 4> allTracts{{
            {0, 0, 1}, {0, 1, 1}, {1, 0, 1}, {1, 1, 1}
        }};
        const Sint32 unmetDemandSupply = ComputeDesirabilityDemandSupply(
            true, 60, allTracts, tractDes, tractPop, 2);

        Check(evalDes == 85 &&
              ComputeGrowthTractTargetPopulation(75, 200u) == 150 &&
              stagePop == 500.0f &&
              unmetDemandSupply == 60 &&
              growthList.size() == 3u &&
              growthList[0].tractX == 0 && growthList[0].tractZ == 0 &&
              growthList[0].growthStage == 3 &&
              growthList[1].tractX == 1 && growthList[1].tractZ == 0 &&
              growthList[1].growthStage == 2 &&
              growthList[2].tractX == 0 && growthList[2].tractZ == 1 &&
              growthList[2].growthStage == 1 &&
              declineList.size() == 1u &&
              declineList[0].tractX == 1 && declineList[0].tractZ == 1,
              "cSC4GrowthDeveloper exact desirability, DoIdle growth/decline list sorting, and stage-curve assignment");

        const CensusCellContribution resContrib = ComputeCensusCellContribution({
            .buildingPurpose = 1u,
            .buildingWealth = 2u,
            .developerPopulation = 80,
            .buildingCapacity = 100u,
            .populationRetention = 0.9f,
            .educationQuotient = 120.0f
        });
        const CensusCellContribution comContrib = ComputeCensusCellContribution({
            .buildingPurpose = 3u,
            .buildingWealth = 3u,
            .developerPopulation = 50,
            .buildingCapacity = 200u,
            .populationRetention = 1.0f,
            .educationQuotient = 0.0f
        });

        std::unordered_map<Uint32, DemandNodeState> demandGraph;
        DemandNodeState& res1011 = demandGraph[0x1011u];
        res1011.demandId = 0x1011u;
        res1011.newSupply = 1000.0f;
        DemandNodeState& res1001 = demandGraph[0x1001u];
        res1001.demandId = 0x1001u;
        DemandNodeState& cs23110 = demandGraph[0x23110u];
        cs23110.demandId = 0x23110u;
        cs23110.activeDemandMin = -2000.0f;
        cs23110.activeDemandMax = 2000.0f;
        DemandNodeState& capSource = demandGraph[0x5000u];
        capSource.demandId = 0x5000u;
        capSource.supplyValue = 100.0f;
        capSource.demandValue = 25.0f;

        const std::array<DemandEconomyLink, 3> testLinks{{
            {0x1011u, 30000u, 0x23110u, DemandEconomyLinkType::SupplyDrivesSupply, 0.20f},
            {0x1001u, 50000u, 0x1011u, DemandEconomyLinkType::AggregateChildren, 1.0f},
            {0x5000u, 40000u, 0x23110u, DemandEconomyLinkType::SatisfactionLink, 1.0f}
        }};
        // InSC4: 0x47bb3f32 (DemandDrivesSupply) or 0x47bb3f30 (DemandDrivesDemand) drives target demand;
        // 0x47bb3f31 (SupplyDrivesSupply) drives target supply;
        // 0x47bb3f50 (AggregateChildren) sums child supply/demand into parent;
        // 0x47bb3f41 (SatisfactionLink) sets target demandCap = demand/supply.
        const Uint32 linkPasses = PropagateDemandEconomyLinks(testLinks, demandGraph, 8u);

        Check(resContrib.primaryKey == 0x1021u &&
              std::abs(resContrib.primaryValue - 80.0f) < 0.001f &&
              resContrib.eqKey == 0x2123u &&
              std::abs(resContrib.eqValue - 72.0f) < 0.001f &&
              comContrib.primaryKey == 0x3331u &&
              std::abs(comContrib.primaryValue - 100.0f) < 0.001f &&
              linkPasses >= 2u &&
              std::abs(demandGraph[0x1011u].supplyValue - 1000.0f) < 0.001f &&
              std::abs(demandGraph[0x1001u].supplyValue - 1000.0f) < 0.001f &&
              std::abs(demandGraph[0x23110u].supplyValue - 200.0f) < 0.001f &&
              std::abs(demandGraph[0x23110u].demandCap - 0.25f) < 0.001f,
              "cSC4DemandSimulator exact PerformCensus EQ/capacity brackets and DoDemandIndexLinks fixed-point propagation");

        Check(GetDeveloperTypeFromPurposeAndWealth(1u, 3u) == 0x1030u &&
              GetDeveloperTypeFromPurposeAndWealth(2u, 2u) == 0x3120u &&
              GetDeveloperTypeFromPurposeAndWealth(3u, 1u) == 0u &&
              GetDeveloperTypeFromPurposeAndWealth(3u, 3u) == 0x3330u &&
              GetDeveloperTypeFromPurposeAndWealth(7u, 2u) == 0x4300u &&
              GetWealthFromDeveloperType(0x1010u) == 1u &&
              GetWealthFromDeveloperType(0x4200u) == 2u &&
              GetWealthFromDeveloperType(0x4300u) == 2u &&
              GetWealthFromDeveloperType(0x4400u) == 3u &&
              GetDevelopmentMapCellValue(1u, 3u, true) == 8u &&
              GetDevelopmentMapCellValue(1u, 3u, false) == 0u &&
              IsFailureReasonEligibleForGrowthCooldown(10u) &&
              IsFailureReasonEligibleForGrowthCooldown(22u) &&
              !IsFailureReasonEligibleForGrowthCooldown(20u) &&
              ShouldKeepGrowthPlanAfterFailure(20u) &&
              std::abs(ComputeLotMaxCapacityPerCell(10u, 2, 2) - 3.0f) < 0.001f,
              "cSC4BuildingDevelopmentSimulator exact developer/wealth mapping, development map key, failure cooldown mask (0x47dc00), and max capacity per cell");

        DemandSimulatorTuning connTuning{};
        connTuning.neighborCoefficients[0] = 100.0f;
        connTuning.neighborCoefficients[2] = 50.0f;
        connTuning.passengerConnScale = 20.0f;
        connTuning.coeffC6 = 1.0f;
        connTuning.coeffC7 = 2.0f;
        connTuning.coeffC9 = 0.1f;
        connTuning.coeffCA = 20.0f;
        const ConnectionUtilizationOutputs connOut =
            EvaluateDemandConnectionUtilization(connTuning, {
                .prevConnectionUtilA = 10.0f,
                .prevPassengerConnectionSupply = 50.0f,
                .totalPassengerTrips = 10u,
                .railAndMonorailCountA = 2,
                .subwayCountB = 1,
                .highwayCountC = 4,
                .groundHighwayCountD = 1,
                .lightRailCountE = 1,
                .roadAvenueFerryCountF = 1,
                .seaportTrips = 100u,
                .airportTrips = 5u
            });

        ExtrapolatedResidentialCensus resExtrap{};
        const bool okResExtrap = ExtrapolateResidentialCensusValues(
            0x1020u, 100.0f, 1000, 900, 1.0f, {10, 20, 30, 30}, resExtrap);
        ExtrapolatedBusinessCensus busExtrap{};
        const bool okBusExtrap = ExtrapolateBusinessCensusValues(
            0x3320u, 200.0f, 500, 400, {0.10f, 0.50f, 0.40f}, busExtrap);
        const std::array<Float32, 12> eqSup{
            100.0f, 0.0f, 0.0f, 0.0f,
            0.0f, 100.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 100.0f
        };
        connTuning.salaryWeights = {
            10u, 20u, 30u, 40u,
            50u, 60u, 70u, 80u,
            90u, 100u, 110u, 140u
        };
        const Uint32 avgSalary = ComputeAverageCitySalary(connTuning, eqSup);

        Check(IsPassengerConnectionNetworkType(1u) &&
              IsPassengerConnectionNetworkType(3u) &&
              IsPassengerConnectionNetworkType(12u) &&
              !IsPassengerConnectionNetworkType(6u) &&
              std::abs(connOut.newPassengerConnectionSupply - 220.0f) < 0.01f &&
              std::abs(connOut.newConnectionUtilA - 450.0f) < 0.1f &&
              okResExtrap &&
              resExtrap.eqKeys[2] == 0x2123u &&
              resExtrap.regionalDemandKeys[2] == 0x12123u &&
              std::abs(resExtrap.extrapolatedValues[2] - 30.0f) < 0.01f &&
              okBusExtrap &&
              busExtrap.regionalSupplyKey == 0x13321u &&
              std::abs(busExtrap.extrapolatedActiveJobs - 160.0f) < 0.01f &&
              std::abs(busExtrap.workforceDemandByWealth[1] - 80.0f) < 0.01f &&
              avgSalary == 70u,
              "cSC4DemandSimulator exact EvaluateConnectionUtilization log2/port/airport supply, regional census extrapolation, and average city salary");

        std::array<Uint16, 4> capGrid{0u, 0u, 0u, 0u};
        Uint32 totalCap = 0u;
        const bool okCapAdd = UpdateDeveloperCapacityGrid({
            .developerPurpose = 1u,
            .buildingPurpose = 1u,
            .lotCapacity = 25u,
            .addOrRemove = true,
            .sizeX = 2u,
            .sizeZ = 2u,
            .minX = 0,
            .minZ = 0,
            .maxX = 1,
            .maxZ = 1,
            .locationX = 0,
            .locationZ = 0,
            .tractShift = 2,
            .gridTractsX = 2,
            .gridTractsZ = 2
        }, capGrid, totalCap);
        const bool okCapSub = UpdateDeveloperCapacityGrid({
            .developerPurpose = 1u,
            .buildingPurpose = 1u,
            .lotCapacity = 10u,
            .addOrRemove = false,
            .sizeX = 2u,
            .sizeZ = 2u,
            .minX = 0,
            .minZ = 0,
            .maxX = 1,
            .maxZ = 1,
            .locationX = 0,
            .locationZ = 0,
            .tractShift = 2,
            .gridTractsX = 2,
            .gridTractsZ = 2
        }, capGrid, totalCap);

        const std::array<GrowthDeveloperItem, 2> testGrowthList{{
            {3, 4, 2}, {5, 6, 1}
        }};
        std::vector<std::pair<Sint32, Sint32>> marks{{3, 4}};
        GrowthDeveloperItem picked1{};
        GrowthDeveloperItem picked2{};
        const bool okPick1 = SelectNextUnmarkedGrowthTract(testGrowthList, marks, picked1);
        const std::size_t marksAfter1 = marks.size();
        const bool okPick2 = SelectNextUnmarkedGrowthTract(testGrowthList, marks, picked2);
        const std::size_t marksAfter2 = marks.size();

        Check(okCapAdd && okCapSub &&
              capGrid[0] == 15u && totalCap == 15u &&
              okPick1 && picked1.tractX == 5 && picked1.tractZ == 6 && marksAfter1 == 2u &&
              okPick2 && picked2.tractX == 3 && picked2.tractZ == 4 && marksAfter2 == 1u,
              "cSC4GrowthDeveloper UpdatePopulationCount capacity grid distribution and cSC4BuildingDevelopmentSimulator Grow tract-mark wrap-around");

        const BoundaryLotTextureRule bRule0 = LookupBoundaryLotTexture({0, 0, 0, 0});
        const BoundaryLotTextureRule bRuleCorner = LookupBoundaryLotTexture({1, 1, 0, 0});
        const BoundaryLotTextureRule bRuleMirror = LookupBoundaryLotTexture({2, 1, 0, 0});
        cSC4LotConfigurationObject bldObj{};
        bldObj.type = static_cast<Uint8>(LotConfigObjectType::Building);
        bldObj.orientation = 1u;
        bldObj.posX = 0.0f;
        bldObj.posY = 2.5f;
        bldObj.posZ = 40.0f;
        std::vector<cSC4LotConfigurationObject> objsToCorrect{bldObj};
        const bool corrected =
            cSC4LotConfiguration::CorrectObjectPositions(objsToCorrect, 32.0f, 32.0f);
        const LotObjectWorldPlacement place0 =
            ComputeLotObjectWorldPlacement(10, 20, 11, 21, 0, 270.0f, objsToCorrect[0]);
        const LotObjectWorldPlacement place2 =
            ComputeLotObjectWorldPlacement(10, 20, 11, 21, 2, 270.0f, objsToCorrect[0]);

        cSC4LotConfigurationObject texObj{};
        texObj.type = static_cast<Uint8>(LotConfigObjectType::BaseTexture);
        texObj.orientation = 2u;
        texObj.posX = 3.2f;
        texObj.posZ = 19.1f;
        const LotObjectWorldPlacement texPlace =
            ComputeLotObjectWorldPlacement(0, 0, 1, 1, 2, 250.0f, texObj);

        Check(bRule0.textureId == 0x3000u && bRule0.rotation == 0u && !bRule0.mirror &&
              bRuleCorner.textureId == 0x2100u && bRuleCorner.rotation == 1u && !bRuleCorner.mirror &&
              bRuleMirror.textureId == 0x2400u && bRuleMirror.rotation == 0u && bRuleMirror.mirror &&
              corrected &&
              std::abs(objsToCorrect[0].posX - 0.04f) < 0.001f &&
              std::abs(objsToCorrect[0].posZ - 31.96f) < 0.001f &&
              std::abs(place0.worldX - (192.0f - 0.04f)) < 0.01f &&
              std::abs(place2.worldX - (160.0f + 0.04f)) < 0.01f &&
              std::abs(place0.worldY - 272.5f) < 0.01f &&
              place0.orientation == 1 && place2.orientation == 3 &&
              std::abs(texPlace.worldX - 8.0f) < 0.01f &&
              std::abs(texPlace.worldZ - 24.0f) < 0.01f &&
              texPlace.orientation == 0,
              "cSC4LotConfiguration object clamping and cSC4LotDeveloper boundary rules & world placement");
    }

    // ------------------------------------------------------------------------
    // 1b. cSC4NetworkTool RUL auto-tiler primitives (pure, decompiled)
    // ------------------------------------------------------------------------
    {
        bool metaOk = true;
        for (Uint8 meta = 0; meta < 25u; ++meta) {
            Sint8 rx = 0, rz = 0;
            ConvertMetaToRelativePos(meta, rx, rz);
            metaOk &= ConvertRelativePosToMeta(rx, rz) == meta;
            Uint8 spun = meta;
            for (int r = 0; r < 4; ++r) {
                spun = RotateMetaPosition(spun, 1);
            }
            metaOk &= spun == meta;
            metaOk &= MirrorMetaPosition(MirrorMetaPosition(meta)) == meta;
        }
        Sint8 cx = 0, cz = 0;
        ConvertMetaToRelativePos(0, cx, cz);
        metaOk &= cx == 0 && cz == 0;
        ConvertMetaToRelativePos(15, cx, cz);
        metaOk &= cx == 2 && cz == -2;
        Check(metaOk, "cSC4TileInfo 5x5 meta-position tables roundtrip (rotate x4, mirror x2 = identity)");

        cSC4TileNeighborhoodCompact compact{};
        compact.SetEdgeCondition(0, cSC4TileNeighborhoodCompact::EncodeRulEdgeToken(2));
        compact.SetEdgeCondition(2, cSC4TileNeighborhoodCompact::EncodeRulEdgeToken(11));
        std::array<Sint32, 4> straight{}, diagonal{};
        Sint32 sc = 0, dc = 0;
        compact.GetStraightAndDiagonalEdges(straight, diagonal, sc, dc);
        const auto mirrored = compact.GetMirrorImageEquivalent().GetMirrorImageEquivalent();
        Check(compact.HasConnectionOnEdge(0) && !compact.HasConnectionOnEdge(1) &&
              sc == 1 && dc == 1 && mirrored == compact,
              "cSC4TileNeighborhoodCompact RUL token encoding (major/minor) and mirror involution");

        cSC4TileEvaluator evaluator;
        const char* rulText =
            "#Transmogrify\n"
            "1,2,0,2,0\n"
            "3,0,05004b00,0,0\n";
        const bool loadedRul = evaluator.LoadFromText(rulText);
        cSC4TileNeighborhoodCompact straightRoad{};
        straightRoad.SetEdgeCondition(0, 2);
        straightRoad.SetEdgeCondition(2, 2);
        const auto* solutions = loadedRul ? evaluator.Lookup(straightRoad) : nullptr;
        Check(loadedRul && evaluator.WasTransmogrified() && evaluator.GetRuleCount() >= 2 &&
              solutions != nullptr && !solutions->empty() &&
              (*solutions)[0].pieceId == 0x05004b00u,
              "cSC4TileEvaluator RUL2 parse + transmogrify + straight-road lookup selects 0x05004b00");

        cSC4NetworkRuleTables tables;
        tables.textureRemaps.push_back(NetworkTextureRemapEntry{
            0x05000000u, 0xff000000u, 0x100u, 0xffffffffu, 0u, 1u, false});
        Uint8 rotFlip = 0;
        const Uint32 mappedTex = tables.MapPreviewTexture(0x05004b00u, rotFlip);
        Check(mappedTex == 0x05004c00u && rotFlip == 1u &&
              tables.MapPath(0x1234u, false) == 0x1234u,
              "cSC4NetworkManager MapPreviewTexture/MapPath exact remap arithmetic");

        const NetworkRoadStyleResult styleFarming = ComputeNetworkRoadStyle(7u, 0u, false);
        const NetworkRoadStyleResult styleResLow = ComputeNetworkRoadStyle(1u, 1u, false);
        Check(styleFarming.underTextureId == 0x08400000u && styleFarming.variation == 7u &&
              styleResLow.underTextureId == 0x08100000u && styleResLow.variation == 1u,
              "cSC4NetworkLotManager UpdateRoadStyle under-texture/variation tables (0x100a20490/0x100a20590)");
    }

    // ------------------------------------------------------------------------
    // 2. Verify Steam SimCity 4 Deluxe Install & Mach-O Binary Symbols
    // ------------------------------------------------------------------------
    std::printf("\n--- 2. Steam SimCity 4 Deluxe Mach-O Binary & DBPF Archives ---\n");
    cDBPFSystem dbpfSys;
    std::string steamApp = cDBPFSystem::DefaultSteamSC4AppPath();
    if (fs::exists(steamApp)) {
        bool mounted = dbpfSys.MountInstall(steamApp);
        Check(mounted, "Mounted Steam SimCity 4 Deluxe Edition.app");
        Check(dbpfSys.Archives().size() >= 8, "Mounted SimCity_1..5.dat, EP1.dat, Sound.dat, SimCityLocale.dat");
        Check(dbpfSys.TotalIndexedEntries() > 110000, "Indexed > 110,000 DBPF TGI entries across GameData .dat files");
        Check(dbpfSys.RegionCities().size() >= 400, "Discovered 417 shipped .sc4 cities across 6 Regions");
        Check(dbpfSys.Regions().size() == 6, "Parsed all 6 shipped config.bmp region layouts");
        bool regionDimensionsValid = true;
        for (const auto& region : dbpfSys.Regions()) {
            regionDimensionsValid &= region.width >= 8 && region.width <= 16 &&
                                     region.height >= 8 && region.height <= 16;
        }
        Check(regionDimensionsValid, "Decoded shipped region dimensions from config.bmp headers");

        MachOBinarySummary machO = InspectSC4MachOBinary(dbpfSys.MachOBinaryPath());
        Check(machO.valid, "Parsed 64-bit Mach-O LC_SYMTAB from Sim City 4 Deluxe Editionsub");
        Check(machO.totalSymbols == 62434, "Verified 62,434 C++ symbols in Sim City 4 Deluxe Editionsub");
        Check(machO.gzcomFactoryCount == 194, "Verified 194 GZCOM_Create* class factories");
        Check(machO.extractedCLSIDs.size() >= 200, "Extracted > 200 32-bit GZCLSIDs directly from x86_64 machine code");
        Check(machO.extractedCLSIDs[kCLSID_cSC4City].find("cSC4City") != std::string::npos,
              "Machine code CLSID 0x8990c372 == cSC4City");
        Check(machO.extractedCLSIDs[kCLSID_cSC4Lot].find("cSC4Lot") != std::string::npos,
              "Machine code CLSID 0xc9bd5d4a == cSC4Lot");
        Check(machO.extractedCLSIDs[kCLSID_cSC4BuildingOccupant].find("cSC4BuildingOccupant") != std::string::npos,
              "Machine code CLSID 0xa9bd882d == cSC4BuildingOccupant");
        Check(machO.extractedCLSIDs[kCLSID_cSC4NetworkOccupant].find("cSC4NetworkOccupant") != std::string::npos,
              "Machine code CLSID 0xc9c05c6e == cSC4NetworkOccupant");

        // --------------------------------------------------------------------
        // 3. Verify GameData Asset Decoders (Exemplar, S3D, FSH, LTEXT)
        // --------------------------------------------------------------------
        std::printf("\n--- 3. GameData Asset Decoders (Exemplar, S3D, FSH, LTEXT) ---\n");
        const auto& dat1 = *dbpfSys.Archives()[0]; // SimCity_1.dat
        std::vector<Uint8> exBytes;
        Check(dat1.ReadFirstByType(kTypeExemplar, exBytes), "QFS-decompressed Exemplar (0x6534284a) from SimCity_1.dat");
        cGZExemplar ex;
        Check(ex.Parse(exBytes.data(), exBytes.size()) && !ex.properties.empty(),
              "Parsed binary EQZB1### Exemplar properties", ex.name.c_str());

        constexpr cSC4TGI tractTuningTGI{
            kTypeExemplar, 0xe7e2c2dbu, 0xe8da7677u
        };
        std::vector<Uint8> tractTuningBytes;
        cGZExemplar tractTuningExemplar;
        TractDeveloperTuning installedTractTuning{};
        bool loadedTractTuning =
            dbpfSys.LoadAssetByTGI(tractTuningTGI, tractTuningBytes) &&
            tractTuningExemplar.Parse(tractTuningBytes.data(), tractTuningBytes.size()) &&
            installedTractTuning.LoadFromExemplar(tractTuningExemplar);
        char tractTuningDetail[192];
        std::snprintf(tractTuningDetail, sizeof(tractTuningDetail),
                      "(loaded=%d, capacity=(%.3f, %.3f), candidate=(%.3f, %.3f))",
                      loadedTractTuning ? 1 : 0,
                      installedTractTuning.existingCapacityAWeight,
                      installedTractTuning.existingCapacityBWeight,
                      installedTractTuning.sourceLotCountWeight,
                      installedTractTuning.candidateType4Bonus);
        Check(loadedTractTuning &&
              installedTractTuning.existingCapacityAWeight == 1.0f &&
              std::abs(installedTractTuning.existingCapacityBWeight - 0.2f) < 0.0001f &&
              installedTractTuning.sourceLotCountWeight == 10.0f &&
              installedTractTuning.candidateType4Bonus == 10.0f &&
              installedTractTuning.maxOverbuildFactor > 1.0f &&
              !installedTractTuning.defaultActiveStyles.empty() &&
              installedTractTuning.yearsBetweenStyleChanges > 0,
              "Loaded exact cSC4TractDeveloper scoring weights and style tunables from shipped exemplar",
              tractTuningDetail);

        std::size_t loadedGrowthDevExemplars = 0;
        std::size_t totalStageCurves = 0;
        Uint32 growthDevGroup = 0;
        for (const auto& entry : dat1.Entries()) {
            if (entry.tgi.type != kTypeExemplar) {
                continue;
            }
            std::vector<Uint8> devExBytes;
            cGZExemplar devExemplar;
            if (!dbpfSys.LoadAssetByTGI(entry.tgi, devExBytes) ||
                !devExemplar.Parse(devExBytes.data(), devExBytes.size()) ||
                devExemplar.GetInt(kPropExemplarType, 0) != 4) {
                continue;
            }
            growthDevGroup = entry.tgi.group;
            GrowthDeveloperTuning devTuning{};
            if (devTuning.LoadFromExemplar(devExemplar) &&
                devTuning.subType != 0u &&
                devTuning.developerClass >= 1u && devTuning.developerClass <= 3u &&
                devTuning.sourceDemandId != 0u &&
                !devTuning.stageCurves.empty()) {
                ++loadedGrowthDevExemplars;
                totalStageCurves += devTuning.stageCurves.size();
            }
        }
        char growthDevTuningDetail[160];
        std::snprintf(
            growthDevTuningDetail,
            sizeof(growthDevTuningDetail),
            "(group=0x%08x, loaded=%zu/12, totalStageCurves=%zu)",
            growthDevGroup,
            loadedGrowthDevExemplars,
            totalStageCurves);
        Check(loadedGrowthDevExemplars == 12u && totalStageCurves >= 36u,
              "Loaded all 12 cSC4GrowthDeveloper desirability and stage-curve exemplars from SimCity_1.dat",
              growthDevTuningDetail);

        std::vector<Uint8> demandSimExBytes;
        cGZExemplar demandSimExemplar;
        DemandSimulatorTuning installedDemandTuning{};
        const bool loadedDemandSimTuning =
            dbpfSys.LoadAssetByTGI(kDemandSimulatorTuningExemplarTGI, demandSimExBytes) &&
            demandSimExemplar.Parse(demandSimExBytes.data(), demandSimExBytes.size()) &&
            installedDemandTuning.LoadFromExemplar(demandSimExemplar);
        std::size_t loadedDemandSpecs = 0;
        std::size_t totalEconomyLinks = 0;
        std::size_t buildingsWithAgentEffects = 0;
        std::size_t totalAgentEffects = 0;
        std::size_t loadedLotConfigs = 0;
        std::size_t totalLotObjects = 0;
        for (const auto& entry : dat1.Entries()) {
            if (entry.tgi.type != kTypeExemplar) {
                continue;
            }
            std::vector<Uint8> demExBytes;
            cGZExemplar demExemplar;
            if (!dbpfSys.LoadAssetByTGI(entry.tgi, demExBytes) ||
                !demExemplar.Parse(demExBytes.data(), demExBytes.size())) {
                continue;
            }
            const Sint64 exType = demExemplar.GetInt(kPropExemplarType, 0);
            if (exType == 3) {
                DemandExemplarSpec spec{};
                if (spec.LoadFromExemplar(demExemplar, entry.tgi.instance)) {
                    ++loadedDemandSpecs;
                    totalEconomyLinks += spec.links.size();
                }
            } else if (exType == 2) {
                std::vector<DemandAgentEffect> agentEffects;
                bool isConditional = false;
                if (ParseDemandAgentEffectsFromExemplar(
                        demExemplar, agentEffects, &isConditional)) {
                    ++buildingsWithAgentEffects;
                    totalAgentEffects += agentEffects.size();
                }
            } else if (exType == 0x10) {
                cSC4LotConfiguration lotCfg{};
                if (lotCfg.LoadFromExemplar(demExemplar, entry.tgi.instance) &&
                    lotCfg.sizeX > 0u && lotCfg.sizeZ > 0u &&
                    !lotCfg.objects.empty()) {
                    ++loadedLotConfigs;
                    totalLotObjects += lotCfg.objects.size();
                }
            }
        }
        const Float32 neutralTaxAtZero =
            installedDemandTuning.ComputeNeutralTaxRate(0.0f, 0.0f, 0.0f);
        const Float32 neutralTaxAt100k =
            installedDemandTuning.ComputeNeutralTaxRate(60000.0f, 25000.0f, 15000.0f);
        char demandSimTuningDetail[200];
        std::snprintf(
            demandSimTuningDetail,
            sizeof(demandSimTuningDetail),
            "(loaded=%d, specs=%zu, links=%zu, agentBldgs=%zu, agentEffects=%zu, neutralTax0=%.2f)",
            loadedDemandSimTuning ? 1 : 0,
            loadedDemandSpecs,
            totalEconomyLinks,
            buildingsWithAgentEffects,
            totalAgentEffects,
            neutralTaxAtZero);
        Check(loadedDemandSimTuning &&
              loadedDemandSpecs == 173u &&
              totalEconomyLinks == 353u &&
              buildingsWithAgentEffects == 46u &&
              totalAgentEffects == 279u &&
              neutralTaxAtZero > 0.0f &&
              neutralTaxAt100k > 0.0f,
              "Loaded cSC4DemandSimulator tunables (0x4a1f3ce6), all Demand EconomyLink exemplars, and building AgentEffects",
              demandSimTuningDetail);

        LotDeveloperTuning installedLotDevTuning{};
        const bool loadedLotDevTuning =
            installedLotDevTuning.LoadFromExemplar(tractTuningExemplar);
        char lotConfigDetail[180];
        std::snprintf(
            lotConfigDetail,
            sizeof(lotConfigDetail),
            "(lotConfigs=%zu, lotObjects=%zu, lotDevTuning=%d, fndSlope=%.1f)",
            loadedLotConfigs,
            totalLotObjects,
            loadedLotDevTuning ? 1 : 0,
            installedLotDevTuning.foundationSlopeThreshold);
        Check(loadedLotDevTuning &&
              loadedLotConfigs >= 1000u &&
              totalLotObjects >= 15000u,
              "Loaded all shipped cSC4LotConfiguration exemplars (0x88edc900..0x88edcdff) and cSC4LotDeveloper tunables",
              lotConfigDetail);

        std::vector<Uint8> s3dBytes;
        Check(dat1.ReadFirstByType(kTypeS3D, s3dBytes), "QFS-decompressed S3D mesh (0x5ad0e817) from SimCity_1.dat");
        cS3DModel s3d;
        Check(s3d.Parse(s3dBytes.data(), s3dBytes.size()) && !s3d.vertices.empty() && !s3d.indices.empty(),
              "Parsed 3DMD S3D HEAD/VERT/INDX/PRIM/MATS chunks");

        if (dbpfSys.Archives().size() > 1) {
            const auto& dat2 = *dbpfSys.Archives()[1]; // SimCity_2.dat
            std::vector<Uint8> fshBytes;
            Check(dat2.ReadFirstByType(kTypeFSH, fshBytes), "QFS-decompressed FSH texture (0x7ab50e44) from SimCity_2.dat");
            cFSHImage fsh;
            Check(fsh.Parse(fshBytes.data(), fshBytes.size()) && fsh.width > 0 && fsh.height > 0 &&
                  fsh.rgbaPixels.size() == static_cast<std::size_t>(fsh.width) * fsh.height * 4,
                  "Decoded SHPI DXT1/DXT3 FSH texture to RGBA8 pixels");
        }

        // --------------------------------------------------------------------
        // 4. Verify .sc4 City Save Decoder (City - Big City Tutorial.sc4)
        // --------------------------------------------------------------------
        std::printf("\n--- 4. Authentic .sc4 Save Decoder (Big City Tutorial.sc4) ---\n");
        std::string bigCityPath = steamApp + "/Contents/GameData/Regions/Timbuktu/City - Big City Tutorial.sc4";
        CheckSavedBuildingBindings(steamApp);
        CheckSavedLotBindings(steamApp);
        RunSavedCapacityChecks([](bool ok, const char* label) { Check(ok, label); }, true);
        RunPollutionAgingMathChecks([](bool ok, const char* label) { Check(ok, label); });
        RunLoadedAgingTransitionChecks([](bool ok, const char* label) { Check(ok, label); });
        {
            static unsigned signedChecks = 0, signedFailures = 0;
            RunSavedSint32Checks([](bool ok, const char* label) {
                ++signedChecks;
                if (!ok) { ++signedFailures; std::printf("Sint32 failure: %s\n", label); }
            });
            std::printf("Sint32 scalar boundary corpus: %u checks, %u failures\n",
                        signedChecks, signedFailures);
            Check(signedChecks >= 65805 && signedFailures == 0,
                  "Sint32 scalar getter complete boundary and excluded-tag corpus");
        }
        cSC4City bigCity;
        Check(bigCity.LoadFromSC4File(bigCityPath, &dbpfSys), "Loaded City - Big City Tutorial.sc4");
        Check(bigCity.CityName() == "Big City Tutorial", "Decoded RegionalCity Name == 'Big City Tutorial'");
        Check(bigCity.MayorName() == "Jonas Sparks", "Decoded RegionalCity Mayor == 'Jonas Sparks'");
        {
            bool sizesMatchBounds = !bigCity.Lots().empty();
            std::size_t lotArea = 0;
            for (const auto& lot : bigCity.Lots()) {
                sizesMatchBounds = sizesMatchBounds &&
                    lot.sizeX == lot.maxX - lot.minX + 1 && lot.sizeZ == lot.maxZ - lot.minZ + 1;
                lotArea += static_cast<std::size_t>(lot.sizeX) * lot.sizeZ;
            }
            Check(sizesMatchBounds && lotArea <= static_cast<std::size_t>(bigCity.CellCountX()) * bigCity.CellCountZ(),
                  "cSC4Lot +0xce/+0xcf is an anchor cell, so lot size comes from the bounds",
                  ("lotArea=" + std::to_string(lotArea)).c_str());
        }
        Check(bigCity.CellCountX() == 128 && bigCity.CellCountZ() == 128, "Decoded 129x129 float32 terrain heightmap -> 128x128 Medium City");
        Check(bigCity.ResidentialPop() == 22618 && bigCity.CommercialPop() == 3727 && bigCity.IndustrialPop() == 10638,
              "Decoded exact R/C/I populations (22,618 / 3,727 / 10,638)");
        Check(bigCity.TreasuryFunds() == 1035332, "Decoded cSC4BudgetSimulator treasury == 1,035,332 Simoleons");
        {
            const BudgetSimulatorSaveRecord& budget = bigCity.BudgetSaveRecord();
            cIGZOStream budgetOut;
            const bool budgetWrote = bigCity.HasBudgetSaveRecord() && budget.Write(budgetOut);
            cIGZIStream budgetIn(budgetOut.Buffer().data(), budgetOut.Buffer().size());
            BudgetSimulatorSaveRecord budgetRoundtrip{};
            const bool budgetRoundtripOk = budgetWrote && budgetRoundtrip.Read(budgetIn) &&
                budgetRoundtrip.totalFunds == budget.totalFunds &&
                budgetRoundtrip.departments.size() == budget.departments.size() &&
                budgetRoundtrip.bonds.size() == budget.bonds.size() &&
                budgetRoundtrip.groups.size() == budget.groups.size();
            char budgetDetail[192];
            std::snprintf(budgetDetail, sizeof(budgetDetail),
                          "(version=%u, funds=%lld, depts=%zu, bonds=%zu, groups=%zu, tax0=%.2f)",
                          budget.version, static_cast<long long>(budget.totalFunds),
                          budget.departments.size(), budget.bonds.size(), budget.groups.size(),
                          static_cast<double>(budget.GetTaxRate(0)));
            Check(bigCity.HasBudgetSaveRecord() && budget.version >= 4u && budgetRoundtripOk,
                  "Decoded cSC4BudgetSimulator (0xe990be01) version>=4 save record from .sc4", budgetDetail);

            std::vector<Uint8> budgetTuningBytes;
            cGZExemplar budgetTuningExemplar;
            BudgetSimulatorTuning shippedBudgetTuning{};
            const bool loadedBudgetTuning =
                dbpfSys.LoadAssetByTGI(kBudgetSimulatorTuningExemplarTGI, budgetTuningBytes) &&
                budgetTuningExemplar.Parse(budgetTuningBytes.data(), budgetTuningBytes.size()) &&
                shippedBudgetTuning.LoadFromExemplar(budgetTuningExemplar);
            char budgetTuningDetail[192];
            std::snprintf(budgetTuningDetail, sizeof(budgetTuningDetail),
                          "(rate=%.4f, term=%u, minFunds=%lld, wealthFactors=%zu, capFactors=%zu)",
                          static_cast<double>(shippedBudgetTuning.bondInterestRate),
                          shippedBudgetTuning.bondTermMonths,
                          static_cast<long long>(shippedBudgetTuning.minAllowableFunds),
                          shippedBudgetTuning.wealthTaxFactors.size(),
                          shippedBudgetTuning.taxCapFactors.size());
            Check(loadedBudgetTuning && shippedBudgetTuning.bondTermMonths > 0u &&
                  shippedBudgetTuning.wealthTaxFactors.size() >= 3u,
                  "Loaded cSC4BudgetSimulator tunables from shipped exemplar (0x0bff6824)", budgetTuningDetail);

            // Exact ComputeTax arithmetic on a synthetic taxable lot.
            BudgetSimulatorSaveRecord taxRecord{};
            BudgetSimulatorTuning taxTuning{};
            taxRecord.SetTaxRate(0, 9.0f);
            taxTuning.taxTypeMultipliers[0] = 1.0f;
            taxTuning.wealthTaxFactors = {1.0f, 1.0f, 1.0f};
            taxTuning.taxCapFactors = {1.0f, 1.0f, 1.0f};
            const std::array<cSC4ResponseCurve::Point, 2> curvePoints{{
                {0.0f, 0.0f}, {255.0f, 1.0f}}};
            taxTuning.landValueResponseCurve.SetResponseCurveData(
                std::span<const cSC4ResponseCurve::Point>(curvePoints));
            BudgetTaxLotInput lot{};
            lot.purpose = 1; lot.wealth = 1; lot.assessedValueProperty = 120000;
            lot.sizeX = 1; lot.sizeZ = 1; lot.landValueClass = 0;
            lot.holderCategory = 1; lot.simulatorTypeIndex = 0;
            const Sint64 taxTotal = ComputeBudgetTax(taxRecord, taxTuning, std::span<const BudgetTaxLotInput>(&lot, 1));
            Check(taxTotal == 900 && taxRecord.GetWeightedAssessedTaxValue(0) == 120000 &&
                  taxRecord.taxCategories[0].totalTaxIncome == 900 &&
                  GetTaxTypeForPurposeAndWealth(1, 1) == 0 &&
                  GetTaxTypeForPurposeAndWealth(0, 3) == -1,
                  "cSC4BudgetSimulator ComputeTax exact float-magic assessed value and /1200 income");

            const Sint64 loanPayment = GetMonthlyPaymentForLoan(shippedBudgetTuning, 100000);
            Check(loanPayment > 1000 && loanPayment < 1500,
                  "cSC4BudgetSimulator GetMonthlyPaymentForLoan amortization (rate/12, pow(1+r,-term))");

            // ---- IssueBond @ 0x100221fdc (corrected semantics, Nov 2026) ----
            // Discriminates the byte-verified corrections: the cmove
            // zero-round-up @ 0x100222018 (small amount issues ONE
            // denomination unit instead of being rejected), no invented
            // <= 0 reject, and the post-add borrowing-limit compare
            // @ 0x10022203b (debt + bond vs limit, not a pre-check).
            BudgetSimulatorTuning bondTuning{};
            bondTuning.field808 = 1000;   // denomination (+0x808)
            bondTuning.field810 = 5;       // max bond count (+0x810)
            bondTuning.field818 = 10000;   // max bond size (+0x818)
            bondTuning.bondTermMonths = 120u;
            bondTuning.bondInterestRate = 0.06f;
            {
                BudgetSimulatorSaveRecord rec{};
                Check(rec.IssueBond(500u, bondTuning, 3u, 2000u),
                      "IssueBond zero-round-up issues one denomination bond @ 0x100222018");
                Check(rec.bonds.size() == 1u && rec.bonds[0].principal == 1000,
                      "IssueBond rounded principal == denomination @ 0x100222018");
            }
            {
                BudgetSimulatorSaveRecord rec{};
                Check(!rec.IssueBond(25000u, bondTuning, 3u, 2000u),
                      "IssueBond amount above max bond size -> false @ 0x10022201f");
                Check(rec.IssueBond(10000u, bondTuning, 3u, 2000u),
                      "IssueBond exact multiple within limit -> true");
            }
            {
                BudgetSimulatorSaveRecord rec{};
                rec.field58 = 45000;  // capacity 50000; a 10000 bond would exceed it
                Check(!rec.IssueBond(10000u, bondTuning, 3u, 2000u),
                      "IssueBond post-add borrowing-limit compare rejects oversize @ 0x10022203b");
                Check(rec.IssueBond(5000u, bondTuning, 3u, 2000u),
                      "IssueBond within remaining capacity -> true");
                Check(rec.field58 == 50000,
                      "IssueBond debt grows by principal @ 0x10022214d");
            }
            {
                BudgetSimulatorSaveRecord rec{};
                Check(!rec.IssueBond(0u, bondTuning, 3u, 2000u),
                      "IssueBond zero amount -> false @ 0x100221ff0");
            }

            // ---- BudgetMonthlyInputs department-income channel @ 0x100220742 ----
            // The binary sums each department's vt+0x80 monthly income into the
            // income accumulator inside the DoMonthlySimulation department list
            // walk (add r15, rax). The pre-fix model omitted the term entirely.
            {
                BudgetSimulatorTuning t{};
                t.minAllowableFunds = -1000000;  // keep the low-funds path quiet
                BudgetSimulatorSaveRecord rec{};
                BudgetMonthlyInputs in{};
                in.taxIncome = 1000;
                in.otherIncome = 200;
                in.neighborIncome = 50;
                in.departmentIncomes = 3000;    // the discriminating term
                in.cityExpense = 100;
                in.month = 6; in.year = 2000;
                ApplyMonthlyBudgetStep(rec, t, in);
                Check(rec.lastMonthlyIncome == 4250,
                      "department incomes sum into the monthly income total @ 0x100220742");
                Check(rec.fundsMirror == 4250 - 100,
                      "department incomes flow through to funds @ 0x100220742");
            }
            {
                // Zero department incomes: prior behavior is a regression baseline.
                BudgetSimulatorTuning t{};
                t.minAllowableFunds = -1000000;
                BudgetSimulatorSaveRecord rec{};
                BudgetMonthlyInputs in{};
                in.taxIncome = 1000;
                in.otherIncome = 200;
                in.neighborIncome = 50;
                in.cityExpense = 100;
                in.month = 6; in.year = 2000;
                ApplyMonthlyBudgetStep(rec, t, in);
                Check(rec.lastMonthlyIncome == 1250,
                      "zero department incomes keep the legacy income total @ 0x100220742");
            }
            {
                // Negative department income passes through unclamped (the loop
                // adds raw vt+0x80 returns; no guard in the bytes).
                BudgetSimulatorTuning t{};
                t.minAllowableFunds = -1000000;
                BudgetSimulatorSaveRecord rec{};
                BudgetMonthlyInputs in{};
                in.taxIncome = 1000;
                in.departmentIncomes = -400;
                in.cityExpense = 0;
                in.month = 6; in.year = 2000;
                ApplyMonthlyBudgetStep(rec, t, in);
                Check(rec.lastMonthlyIncome == 600,
                      "negative department incomes subtract unclamped @ 0x100220742");
            }

            // ---- Stream GetError() contract (DecompBench rulings, Nov 2026) ----
            {
                // Good stream: zero error. Overrun read: sticky error state.
                const Uint8 bytes[4] = {1, 2, 3, 4};
                cIGZIStream good(bytes, sizeof(bytes));
                Uint32 v = 0;
                good >> v;
                Check(good.GetError() == 0u,
                      "cIGZIStream::GetError() returns 0 on a healthy stream (binary idiom: !GetError())");
                Uint64 overrun = 0;
                good >> overrun;  // needs 8 bytes, only 4 remain
                Check(good.GetError() != 0u,
                      "cIGZIStream::GetError() nonzero after an overrun read (sticky error)");
                cIGZOStream out;
                out << v;
                Check(out.GetError() == 0u,
                      "cIGZOStream::GetError() is 0 for the vector-backed output stream");
            }

            // ===============================================================
            // cSC4PollutionSimulator — swarm wave 1 (Nov 2026)
            // W1 tunables loader + W2 eatGarbage + W3 maintenance pair +
            // W4 leaf/query batch. Edge-geometry checks per the DoD; every
            // worker-flagged quirk gets a discriminator.
            // ===============================================================
            {
                const auto floatNaN = [] {
                    const Uint32 bits = 0x7fc00000u;
                    Float32 v; std::memcpy(&v, &bits, 4);
                    return v;
                };
                const auto makeFloatProp = [](Uint32 id, Float32 v) {
                    ExemplarProperty p{};
                    p.id = id;
                    p.valueType = static_cast<Uint16>(ExemplarValueType::Float32);
                    p.floatValues = {v};
                    return p;
                };
                const auto makeIntProp = [](Uint32 id, Sint64 v) {
                    ExemplarProperty p{};
                    p.id = id;
                    p.valueType = static_cast<Uint16>(ExemplarValueType::Uint32);
                    p.intValues = {v};
                    return p;
                };
                PollutionTuning pollutionTuning{};

                // ---- W1: ReadSimulatorTunableValues @ 0x100095912 ----
                {
                    PollutionSimulatorSaveRecord r{};
                    r.field2E0 = 7.5f;
                    mTrafficAirFactor = -3.0f;
                    kMaxAirPollutionDelta = 12345;
                    pollutionTuning.ReadSimulatorTunableValues(r, nullptr);
                    Check(mTrafficAirFactor == 8.0f &&
                          kMaxAirPollutionDelta == 50 &&
                          kMaxGarbagePolluteForUI == 27000 &&
                          kMaxWaterPolluteForUI == 32000 &&
                          kGarbageAbductionAmount == 5000 &&
                          mGarbageScalingFactor == 0.125f &&
                          kLandfillTileCapacity == 32767 &&
                          kMaxGarbageOnValve == 655340 &&
                          r.field2E0 == 7.5f &&
                          mpPropRequestCurve->GetOutputValue(100.0f) == 0.0f &&
                          r.field1C8 != nullptr &&
                          r.field1C8->GetOutputValue(1.0f) == 1.0f,
                          "failure path resets 15 statics to defaults, keeps (0,0) static curves + identity record curves, field2E0 untouched @ 0x100095b0a");
                }
                {
                    // Re-entrancy + missing-prop fallback + adjacent prop IDs.
                    cGZExemplar ex;
                    ex.properties[0x6a406201u] = makeIntProp(0x6a406201u, 999);
                    ex.properties[0x49134bd4u] = makeIntProp(0x49134bd4u, 111);
                    ex.properties[0x49134bddu] = makeIntProp(0x49134bddu, 222);
                    ex.properties[0xaa1f8ab7u] = makeFloatProp(0xaa1f8ab7u, 3.25f);
                    PollutionSimulatorSaveRecord r{};
                    pollutionTuning.ReadSimulatorTunableValues(r, &ex);
                    const bool loaded = kMaxAirPollutionDelta == 999 &&
                                        kMaxGarbagePolluteForUI == 111 &&
                                        kMaxAirPolluteForUI == 222 &&
                                        r.field2E0 == 3.25f;
                    pollutionTuning.ReadSimulatorTunableValues(r, nullptr);
                    Check(loaded && kMaxAirPollutionDelta == 50 &&
                          kMaxGarbagePolluteForUI == 27000 &&
                          r.field2E0 == 3.25f,
                          "second call resets all statics but leaves instance field2E0; adjacent prop IDs 0x49134bd4/0x49134bdd land in distinct fields @ 0x100095943-0x100095bf");
                }
                {
                    // NaN/-0.0f pass through unclamped (no compares in the loader).
                    cGZExemplar ex;
                    ex.properties[0x69501944u] = makeFloatProp(0x69501944u, floatNaN());
                    ex.properties[0x09135482u] = makeFloatProp(0x09135482u, -0.0f);
                    PollutionSimulatorSaveRecord r{};
                    pollutionTuning.ReadSimulatorTunableValues(r, &ex);
                    Uint32 nanBits = 0; std::memcpy(&nanBits, &mTrafficAirFactor, 4);
                    Check(nanBits == 0x7fc00000u &&
                          std::signbit(mGarbageScalingFactor) && mGarbageScalingFactor == 0.0f,
                          "loader passes NaN and -0.0f through verbatim, no clamps @ 0x100095b29/0x100095c19");
                }

                // ---- W2: eatGarbage @ 0x10009a2ee ----
                {
                    // Block A boundary: room == garbage -> early return, nothing else runs.
                    PollutionSimulatorSaveRecord r{};
                    r.field2C0 = 10;    // export room (i32) - low32(field260)
                    Uint32 inc = 100, wte = 100;
                    const Float32 total = r.EatGarbage(10.0f, inc, wte);
                    Check(total == 10.0f && r.field260 == 10 && inc == 100 && wte == 100 &&
                          r.field270 == 0 && r.field278 == 0 && r.field2C4 == 0,
                          "eatGarbage block A room==garbage early-return: only export consumed @ 0x10009a365");
                }
                {
                    // W2 quirk 1: pool == garbage -> pool zeroed, NOTHING eaten (zero-path).
                    PollutionSimulatorSaveRecord r{};
                    r.field2C0 = 0;    // A skips (room 0)
                    Uint32 inc = 100, wte = 10;
                    const Float32 total = r.EatGarbage(10.0f, inc, wte);
                    Check(total == 10.0f && wte == 0 && r.field278 == 0 &&
                          inc == 90 && r.field270 == 10,
                          "eatGarbage pool==garbage zero-path zeroes wte pool without eating; incinerator floor-eats @ 0x10009a38c/0x10009a413");
                }
                {
                    // Floor consumed but float accumulated: 10.7f eats floor 10,
                    // total gains the FULL float.
                    PollutionSimulatorSaveRecord r{};
                    r.field2C0 = 0;
                    Uint32 inc = 0, wte = 100;
                    const Float32 total = r.EatGarbage(10.7f, inc, wte);
                    Check(total == 10.7f && wte == 90 && r.field278 == 10,
                          "eatGarbage floor-consumes 10 but accumulates the full float 10.7 @ 0x10009a3a9-0x10009a3d2");
                }
                {
                    // W2 quirk 2: abduction MAX-blend over-eats (budget 50, garbage 10).
                    struct MockLandfill final : ISimLandfillDeveloper {
                        void ReadRangeAsInts(Sint32& a, Sint32& b) override { a = 0; b = 0; }
                        Uint32 ReadValueLow32() override { return 0; }
                    };
                    MockLandfill mock;
                    PollutionSimulatorSaveRecord r{};
                    r.field2C0 = 0;
                    r.field348 = 0.0f;  // transport curve product 0 -> block D cap<=0, skips
                    r.field70 = &mock;
                    r.field34C = 0;
                    const Sint32 saveAbduction = kGarbageAbductionAmount;
                    kGarbageAbductionAmount = 50;
                    Uint32 inc = 0, wte = 0;
                    const Float32 total = r.EatGarbage(10.0f, inc, wte);
                    kGarbageAbductionAmount = saveAbduction;
                    Check(total == 50.0f && r.field2C4 == 50,
                          "eatGarbage abduction cmpnless blend is MAX not min: eats budget 50 from garbage 10 @ 0x10009a5a1-0x10009a5ac");
                }
                {
                    // NaN input: every jbe guard skips -> 0 total, no state change.
                    PollutionSimulatorSaveRecord r{};
                    r.field2C0 = 10;
                    Uint32 inc = 100, wte = 100;
                    const Float32 total = r.EatGarbage(floatNaN(), inc, wte);
                    Check(total == 0.0f && r.field260 == 0 && inc == 100 && wte == 100,
                          "eatGarbage NaN input skips all consumers via unordered jbe guards @ 0x10009a303");
                }

                // ---- W3: CalcOccupantMaintenanceCost @ 0x10009a11c ----
                {
                    struct MockCurveProp final : CurvePropertyData {
                        bool GetPoints(std::vector<Float32>& out) const override {
                            out = {0.0f, 0.0f, 100.0f, 5.0f};
                            return true;
                        }
                    };
                    struct MockHolder final : OccupantPropertyHolderSeam {
                        bool hasI64 = true, hasF32 = true, hasCurve = true;
                        Sint64 i64v = 200;
                        Float32 f32v = 100.0f;
                        MockCurveProp curveProp;
                        bool GetPropertyValue(Uint32 id, Sint64& out) const override {
                            if (!hasI64 || id != 0x2a58d350u) return false;
                            out = i64v; return true;
                        }
                        bool GetPropertyValue(Uint32 id, Float32& out) const override {
                            if (!hasF32 || id != 0xc910b4cfu) return false;
                            out = f32v; return true;
                        }
                        const CurvePropertyData* GetProperty(Uint32 id) const override {
                            if (!hasCurve || id != 0x911e2e7u) return nullptr;
                            return &curveProp;
                        }
                    };
                    struct MockOccupant final : OccupantSeam {
                        MockHolder holder;
                        const OccupantPropertyHolderSeam* GetPropertyHolder() const override {
                            return &holder;
                        }
                    };
                    PollutionSimulatorSaveRecord r{};
                    MockOccupant occ;
                    Sint64 cost = 0;
                    Check(r.CalcOccupantMaintenanceCost(occ, cost) && cost == 1000,
                          "maintenance cost = cvttss2si(curve(100)=5 * (f32)200) = 1000 @ 0x10009a1aa-0x10009a1b8");
                    occ.holder.hasF32 = false;
                    cost = 0;
                    Check(!r.CalcOccupantMaintenanceCost(occ, cost) && cost == 200,
                          "missing f32 prop: false but cost RETAINS the first property's value (out untouched on failure) @ 0x10009a166");
                    occ.holder.hasF32 = true;
                    occ.holder.hasCurve = false;
                    cost = 0;
                    Check(!r.CalcOccupantMaintenanceCost(occ, cost) && cost == 200,
                          "missing curve prop: false, cost retained @ 0x10009a180");
                    occ.holder.hasI64 = false;
                    cost = 0;
                    Check(!r.CalcOccupantMaintenanceCost(occ, cost) && cost == 0,
                          "missing i64 prop on the first read: false with cost untouched @ 0x10009a151");
                    Check(Cvttss2siI64(floatNaN()) == INT64_MIN &&
                          Cvttss2siI64(1.0e30f) == INT64_MIN &&
                          Cvttss2siI64(2.9f) == 2 && Cvttss2siI64(-2.9f) == -2,
                          "Cvttss2siI64: NaN/overflow -> INT64_MIN, truncation toward zero @ 0x10009a1b3");
                }

                // ---- W4: leaves + queries ----
                {
                    // AddToGarbageSupply — byte-settled @ 0x10009abde: jns lands on
                    // the ADD block, so non-negative amounts ADD (W4's original
                    // "positive no-op" reading was a jump-target misread, settled
                    // with fresh disasm); negative amounts clamp at 0.
                    PollutionSimulatorSaveRecord r{};
                    r.field34C = 100;
                    r.AddToGarbageSupply(40);
                    Check(r.field34C == 140,
                          "AddToGarbageSupply non-negative amount ADDS (jns skips only the negative clamp) @ 0x10009abea");
                    r.AddToGarbageSupply(-200);
                    Check(r.field34C == 0,
                          "AddToGarbageSupply oversize negative clamps supply to 0 @ 0x10009abf4-0x10009abfb");
                    r.field34C = 100;
                    r.AddToGarbageSupply(-40);
                    Check(r.field34C == 60,
                          "AddToGarbageSupply in-range negative subtracts @ 0x10009abf9");
                }
                {
                    // GetGarbageSupply dirty-write: const method zeroes a negative supply.
                    PollutionSimulatorSaveRecord r{};
                    r.field34C = -5;
                    Check(r.GetGarbageSupply() == 0 && r.field34C == 0,
                          "GetGarbageSupply negative supply returns 0 AND dirty-writes field34C to 0 @ 0x10009ac06");
                }
                {
                    // GetPollutionGrid routing: ==3 -> gridRefs78, else gridRefs98 (unchecked).
                    PollutionSimulatorSaveRecord r{};
                    r.gridRefs98[0].memAddr = 0xA10u;
                    r.gridRefs78[3].memAddr = 0xAD1Eu;
                    Check(r.GetPollutionGrid(PollutionSimulatorSaveRecord::PollutionType::Air).memAddr == 0xA10u &&
                          r.GetPollutionGrid(PollutionSimulatorSaveRecord::PollutionType::Radiation).memAddr == 0xAD1Eu,
                          "GetPollutionGrid routes Radiation to gridRefs78, Air to gridRefs98 @ 0x10009ac22-0x10009ac40");
                }
                {
                    // Worst-tract getters: u16 zero-extend discriminator.
                    PollutionSimulatorSaveRecord r{};
                    r.field330 = 65535; r.field332 = 400; r.field334 = -7;
                    Sint32 x = 0, z = 0, v = 0;
                    r.GetWorstAirTract(x, z, v);
                    Check(x == 65535 && z == 400 && v == -7,
                          "GetWorstAirTract u16 zero-extends coords, i32 value @ 0x10009ae46");
                }
                {
                    // GetGarbageBuildings order: D8 then F0 then 108.
                    PollutionSimulatorSaveRecord r{};
                    SimSerializableRef a; a.memAddr = 1;
                    SimSerializableRef b; b.memAddr = 2;
                    SimSerializableRef c; c.memAddr = 3;
                    r.listD8.push_back(a);
                    r.listF0.push_back(b);
                    r.list108.push_back(c);
                    OccupantRefList out;
                    r.GetGarbageBuildings(out);
                    Check(out.size() == 3 && out[0].memAddr == 1 &&
                          out[1].memAddr == 2 && out[2].memAddr == 3,
                          "GetGarbageBuildings appends listD8, listF0, list108 in order @ 0x10009ac48");
                }
                {
                    // GetRequestedShare: full arithmetic on a 2x2 tile rect.
                    cSC4ResponseCurve curve;
                    const cSC4ResponseCurve::Point pts[] = {{0.0f, 0.0f}, {1.0f, 1.0f}};
                    curve.SetResponseCurveData(std::span<const cSC4ResponseCurve::Point>(pts));
                    const auto tileValue = [](void*, Uint32, Uint32,
                                              Uint32& outValue) noexcept -> Uint32 {
                        outValue = 32767u;
                        return 1;
                    };
                    PollutionSimulatorSaveRecord r{};
                    PollutionIntRect rect{0, 0, 1, 1};  // 4 tiles, area (1+1-0)^2 = 4
                    const Sint32 share = r.GetRequestedShare(rect, curve, tileValue, nullptr);
                    Check(share == 1,
                          "GetRequestedShare 4x32767 sum / unsigned area 4 / 32767.0f -> curve(1)=1 @ 0x10009ad5c");
                    const auto zeroValue = [](void*, Uint32, Uint32,
                                              Uint32& outValue) noexcept -> Uint32 {
                        outValue = 0u;
                        return 1;
                    };
                    Check(r.GetRequestedShare(rect, curve, zeroValue, nullptr) == 0,
                          "GetRequestedShare zero garbage tiles -> zero share @ 0x10009ad5c");
                }

                // Cleanup: the loader's nullptr path restores every wave-1
                // tuning global to its binary default (the suite's global-
                // restoration invariant stays intact for later checks).
                {
                    PollutionSimulatorSaveRecord cleanupRecord{};
                    pollutionTuning.ReadSimulatorTunableValues(cleanupRecord, nullptr);
                }
            }
        }
        {
            // Civic & utility simulator save records: exact binary Read
            // formats. For records already at the binary's Write version the
            // re-serialized chunk must be byte-identical to the shipped one;
            // for legacy-version records (Fire v2, Police v7) the binary
            // upgrades on Write, so assert Write->Read->Write stability.
            cDBPFArchive civicArchive;
            const bool civicArchiveOpen = civicArchive.Open(bigCityPath);
            auto chunkRoundtrip = [&civicArchive](Uint32 clsid, auto& record, bool& byteIdentical,
                                                  bool& stable) {
                std::vector<Uint8> bytes;
                byteIdentical = false;
                stable = false;
                if (!civicArchive.ReadFirstByType(clsid, bytes) || bytes.size() < 14u) {
                    return false;
                }
                Uint32 recSize = 0;
                std::memcpy(&recSize, bytes.data(), sizeof(Uint32));
                if (recSize < 14u || recSize > bytes.size()) {
                    return false;
                }
                if (!record.ReadFromSC4Record(bytes)) {
                    return false;
                }
                cIGZOStream out1;
                record.Write(out1);
                byteIdentical = out1.Buffer().size() == recSize - 12u &&
                    std::memcmp(out1.Buffer().data(), bytes.data() + 12, out1.Buffer().size()) == 0;
                std::decay_t<decltype(record)> reread{};
                cIGZIStream in1(out1.Buffer().data(), out1.Buffer().size());
                if (!reread.Read(in1)) {
                    return true; // decoded; stability not applicable
                }
                cIGZOStream out2;
                reread.Write(out2);
                stable = out2.Buffer() == out1.Buffer();
                return true;
            };
            bool identical = false, stable = false;

            LandValueSimulatorSaveRecord landValue{};
            Check(civicArchiveOpen && chunkRoundtrip(0xe990c18eu, landValue, identical, stable) &&
                  landValue.version == 1 && identical,
                  "Decoded cSC4LandValueSimulator (0xe990c18e) v1 record, byte-exact roundtrip");
            CommercialSimulatorSaveRecord commercial{};
            Check(chunkRoundtrip(0x8990c080u, commercial, identical, stable) && identical,
                  "Decoded cSC4CommercialSimulator (0x8990c080) v1 record, byte-exact roundtrip");
            IndustrialSimulatorSaveRecord industrial{};
            Check(chunkRoundtrip(0xa990c022u, industrial, identical, stable) && identical,
                  "Decoded cSC4IndustrialSimulator (0xa990c022) v1 record (version word only)");
            FireProtectionSimulatorSaveRecord fire{};
            Check(chunkRoundtrip(0x2990c05au, fire, identical, stable) && fire.version == 2 &&
                  fire.protection.version == 3 && stable && !identical,
                  "Decoded cSC4FireProtectionSimulator (0x2990c05a) v2 record (+Protection base v3), Write upgrades to v3");
            PoliceSimulatorSaveRecord police{};
            Check(chunkRoundtrip(0x6990c04fu, police, identical, stable) && police.version == 7 &&
                  police.protection.version == 3 && stable && !identical,
                  "Decoded cSC4PoliceSimulator (0x6990c04f) v7 record (+Protection base v3), Write upgrades to v8");
            FlammabilitySimulatorSaveRecord flammability{};
            Check(chunkRoundtrip(0xe990c045u, flammability, identical, stable) && identical,
                  "Decoded cSC4FlammabilitySimulator (0xe990c045) v2 record, byte-exact roundtrip");
            CrimeSimulatorSaveRecord crime{};
            char crimeDetail[128];
            const bool crimeOk = chunkRoundtrip(0x8990c09au, crime, identical, stable);
            std::snprintf(crimeDetail, sizeof(crimeDetail), "(events=%zu firstName=%s)",
                          crime.events.size(),
                          crime.events.empty() ? "" : crime.events[0].name.c_str());
            Check(crimeOk && crime.version == 6 && identical && crime.events.size() == 24u,
                  "Decoded cSC4CrimeSimulator (0x8990c09a) v6 record with 24 crime events, byte-exact roundtrip",
                  crimeDetail);
            AuraSimulatorSaveRecord aura{};
            Check(chunkRoundtrip(0x0990bdd8u, aura, identical, stable) && aura.version == 9 &&
                  identical,
                  "Decoded cSC4AuraSimulator (0x0990bdd8) v9 record (7 SimGrid refs + grid map), byte-exact roundtrip");
            PowerSimulatorSaveRecord power{};
            char powerDetail[128];
            const bool powerOk = chunkRoundtrip(0x0990c005u, power, identical, stable);
            std::snprintf(powerDetail, sizeof(powerDetail), "(grid=%zux%zu entries=%zu poles=%zu)",
                          power.sint16Grid.size(),
                          power.sint16Grid.empty() ? 0u : power.sint16Grid[0].size(),
                          power.occupantEntries.size(), power.poleOccupantRefs.size());
            Check(powerOk && power.version == 6 && identical,
                  "Decoded cSC4PowerSimulator (0x0990c005) v6 record, byte-exact roundtrip",
                  powerDetail);
            PlumbingSimulatorSaveRecord plumbing{};
            char plumbingDetail[128];
            const bool plumbingOk = chunkRoundtrip(0x0990c075u, plumbing, identical, stable);
            std::snprintf(plumbingDetail, sizeof(plumbingDetail),
                          "(grid=%ux%u producers=%zu refs=%zu)", plumbing.gridWidth,
                          plumbing.gridHeight, plumbing.producers.size(),
                          plumbing.gridRefList200.size());
            Check(plumbingOk && plumbing.version == 4 && identical &&
                  plumbing.gridWidth == 128u && plumbing.gridHeight == 128u,
                  "Decoded cSC4PlumbingSimulator (0x0990c075) v4 record (128x128 byte grid + water producers), byte-exact roundtrip",
                  plumbingDetail);
            PollutionSimulatorSaveRecord pollution{};
            Check(chunkRoundtrip(0x8990c065u, pollution, identical, stable) &&
                  pollution.version == 4 && identical,
                  "Decoded cSC4PollutionSimulator (0x8990c065) v4 record, byte-exact roundtrip");
            ResidentialSimulatorSaveRecord residential{};
            Check(chunkRoundtrip(0x4990c013u, residential, identical, stable) &&
                  residential.version == 7 && identical,
                  "Decoded cSC4ResidentialSimulator (0x4990c013) v7 record (2x tDepartmentData), byte-exact roundtrip");
            TrafficSimulatorSaveRecord traffic{};
            char trafficDetail[160];
            const bool trafficOk = chunkRoundtrip(0x6990c1aau, traffic, identical, stable);
            cIGZOStream trafficV41Out1, trafficV41Out2;
            TrafficSimulatorSaveRecord trafficV41Reread{};
            bool trafficV41Stable = false;
            if (trafficOk && traffic.WriteV41(trafficV41Out1)) {
                cIGZIStream trafficV41In(trafficV41Out1.Buffer().data(),
                                         trafficV41Out1.Buffer().size());
                trafficV41Stable = trafficV41Reread.Read(trafficV41In) &&
                    trafficV41Reread.version == 41 &&
                    trafficV41Reread.WriteV41(trafficV41Out2) &&
                    trafficV41Out2.Buffer() == trafficV41Out1.Buffer();
            }
            std::snprintf(trafficDetail, sizeof(trafficDetail),
                          "(v=%u grid=%ux%u edgeDensities=%zu switches=%zu rules=%zu v41Bytes=%zu)",
                          traffic.version, traffic.byteGridRows, traffic.byteGridCols,
                          traffic.edgeDensities.size(), traffic.transitSwitches.size(),
                          traffic.transitSwitches.empty() ? 0u : traffic.transitSwitches[0].rules.size(),
                          trafficV41Out1.Buffer().size());
            Check(trafficOk && traffic.version == 40 && identical && trafficV41Stable &&
                  traffic.byteGridRows == 130u && traffic.byteGridCols == 130u &&
                  traffic.floatGridRefs298.size() == 6u &&
                  traffic.edgeDensities.size() == 26185u &&
                  traffic.transitSwitches.size() == 1u,
                  "Decoded full cSC4TrafficSimulator (0x6990c1aa) v40 transit graph (zero opaque bytes, byte-exact 1.86 MB roundtrip + v41 upgrade)",
                  trafficDetail);

            CivicBuildingSimulatorSaveRecord civicBuilding{};
            char civicBldgDetail[128];
            const bool civicBldgOk = chunkRoundtrip(0x0a413481u, civicBuilding, identical, stable);
            std::snprintf(civicBldgDetail, sizeof(civicBldgDetail),
                          "(unlocked=%zu statuses=%zu zoos=%zu civics=%zu)",
                          civicBuilding.unlockedSet.size(), civicBuilding.statuses.size(),
                          civicBuilding.zooOccupants.size(), civicBuilding.civicOccupants.size());
            Check(civicBldgOk && civicBuilding.version == 6 && identical,
                  "Decoded cSC4CivicBuildingSimulator (0x0a413481) v6 record, byte-exact roundtrip",
                  civicBldgDetail);

            FloraSimulatorSaveRecord flora{};
            char floraDetail[128];
            const bool floraOk = chunkRoundtrip(0xc990c034u, flora, identical, stable);
            std::snprintf(floraDetail, sizeof(floraDetail),
                          "(randIndices=%zu occupants=%zu rng=(%u,%u))",
                          flora.randomIterator.indices.size(), flora.floraOccupants.size(),
                          flora.rngState.seedA, flora.rngState.seedB);
            Check(floraOk && flora.version == 2 && identical &&
                  !flora.randomIterator.indices.empty(),
                  "Decoded cSC4FloraSimulator (0xc990c034) v2 record, byte-exact roundtrip",
                  floraDetail);

            NeighborsSimulatorSaveRecord neighbors{};
            char neighborsDetail[96];
            const bool neighborsOk = chunkRoundtrip(0xc990c0a4u, neighbors, identical, stable);
            std::snprintf(neighborsDetail, sizeof(neighborsDetail), "(connections=%zu)",
                          neighbors.connectionRefs.size());
            Check(neighborsOk && neighbors.version == 2 && identical &&
                  !neighbors.connectionRefs.empty(),
                  "Decoded cSC4NeighborsSimulator (0xc990c0a4) v2 record, byte-exact roundtrip",
                  neighborsDetail);

            OrdinanceSimulatorSaveRecord ordinance{};
            char ordinanceDetail[128];
            const bool ordinanceOk = chunkRoundtrip(0xa990c08eu, ordinance, identical, stable);
            std::snprintf(ordinanceDetail, sizeof(ordinanceDetail),
                          "(exp=%lld inc=%lld ordinances=%zu)",
                          static_cast<long long>(ordinance.monthlyExpense),
                          static_cast<long long>(ordinance.monthlyIncome),
                          ordinance.ordinanceRefs.size());
            Check(ordinanceOk && ordinance.version == 4 && identical &&
                  !ordinance.ordinanceRefs.empty(),
                  "Decoded cSC4OrdinanceSimulator (0xa990c08e) v4 record, byte-exact roundtrip",
                  ordinanceDetail);

            WeatherSimulatorSaveRecord weather{};
            char weatherDetail[160];
            const bool weatherOk = chunkRoundtrip(0x2990c1bcu, weather, identical, stable);
            std::snprintf(weatherDetail, sizeof(weatherDetail),
                          "(cells=%dx%d moisture=%ux%u humidity=%.3f wind=%.2f)",
                          weather.gridCellsX, weather.gridCellsZ,
                          weather.moistureGrid138.rows, weather.moistureGrid138.cols,
                          static_cast<double>(weather.GetHumidity()),
                          static_cast<double>(weather.ambientWindSpeed));
            Check(weatherOk && weather.version == 2 && identical &&
                  weather.moistureGrid138.rows > 0u && weather.moistureGrid138.cols > 0u,
                  "Decoded cSC4WeatherSimulator (0x2990c1bc) v2 record, byte-exact roundtrip",
                  weatherDetail);

            // Reuse the decoded shipped record for the coordinate boundary.
            // Native forward-standard conversion uses X for BOTH axes.
            weather_coordinates::Position world{};
            weather_coordinates::Cell cells{}, standard{}, recovered{};
            Check(weatherOk &&
                  weather_coordinates::CellToPosition(weather, -2, 3, world) ==
                      weather_coordinates::Status::applied &&
                  weather_coordinates::PositionToCell(weather, world.x, world.z, cells) ==
                      weather_coordinates::Status::applied && cells.x == -2 && cells.z == 3 &&
                  weather_coordinates::CellToStandardCityCell(weather, 2, 5, standard) ==
                      weather_coordinates::Status::applied &&
                  weather_coordinates::StandardCityCellToCell(weather, standard.x, standard.z, recovered) ==
                      weather_coordinates::Status::applied && recovered.x == 2 && recovered.z == 2,
                  "Decoded weather record executes four coordinate adapters, preserving native X reuse @ 0x1000e487a/0x1000e4896/0x1000e48ba/0x1000e48d0");

            MySimAgentSimulatorSaveRecord mySim{};
            cIGZOStream mySimOut;
            mySim.Write(mySimOut);
            cIGZIStream mySimIn(mySimOut.Buffer().data(), mySimOut.Buffer().size());
            MySimAgentSimulatorSaveRecord mySimRead{};
            Check(mySimRead.Read(mySimIn) && mySimRead.version == 1 && mySimIn.Remaining() == 0,
                  "cSC4MySimAgentSimulator (0x29e37dec) v1 Read/Write roundtrip");

            cSC4GameOfLifeSimulator gol(6, 6);
            gol.DoEdit(2, 1);
            gol.DoEdit(2, 2);
            gol.DoEdit(2, 3);
            gol.DoNextIteration();
            Check(gol.GetCell(1, 2) == 1 && gol.GetCell(2, 2) == 2 && gol.GetCell(3, 2) == 1 &&
                  gol.GetCell(2, 1) == 0,
                  "cSC4GameOfLifeSimulator blinker oscillator & cell-age increment (@ 0x1002fca8e)");

            cSC4RigidBodySimulator2D rbs;
            RigidBody2DObject objA{}, objB{};
            objA.posX = 0.0f; objA.velX = 2.0f; objA.halfExtentX = 1.0f; objA.halfExtentY = 1.0f;
            objB.posX = 1.5f; objB.velX = -1.0f; objB.halfExtentX = 1.0f; objB.halfExtentY = 1.0f;
            rbs.AddObject(objA);
            rbs.AddObject(objB);
            rbs.AdvanceTime(0.1f);
            Check(rbs.Objects().size() == 2u && rbs.Objects()[0].collisionCount == 1u &&
                  rbs.Objects()[1].collisionCount == 1u,
                  "cSC4RigidBodySimulator2D AdvanceTime & DoCollisions (@ 0x100700bfc / 0x100700ee2)");

            Check(bigCity.HasCivicSaveRecord(0x0990c005u) &&
                  bigCity.HasCivicSaveRecord(0xe990c18eu) &&
                  bigCity.HasCivicSaveRecord(0x6990c1aau) &&
                  bigCity.HasCivicSaveRecord(0x0a413481u) &&
                  bigCity.HasCivicSaveRecord(0xc990c034u) &&
                  bigCity.HasCivicSaveRecord(0xc990c0a4u) &&
                  bigCity.HasCivicSaveRecord(0xa990c08eu) &&
                  bigCity.HasCivicSaveRecord(0x2990c1bcu) &&
                  bigCity.PowerSaveRecord().version == 6,
                  "cSC4City exposes decoded civic & utility simulator save records");
        }
        {
            // Civic & utility tunables from the shipped exemplars. Note
            // Power/Plumbing/Pollution genuinely share the single simulator
            // tuning exemplar 0xc911e35b in SimCity_1.dat.
            auto loadTuningExemplar = [&dbpfSys](const cSC4TGI& tgi, auto& tuning) {
                std::vector<Uint8> bytes;
                cGZExemplar exemplar;
                return dbpfSys.LoadAssetByTGI(tgi, bytes) &&
                    exemplar.Parse(bytes.data(), bytes.size()) &&
                    tuning.LoadFromExemplar(exemplar);
            };
            PowerTuning powerTuning{};
            PlumbingTuning plumbingTuning{};
            PollutionTuning pollutionTuning{};
            const bool sharedTuningOk =
                loadTuningExemplar(PowerTuning::kExemplarTGI, powerTuning) &&
                loadTuningExemplar(PlumbingTuning::kExemplarTGI, plumbingTuning) &&
                loadTuningExemplar(PollutionTuning::kExemplarTGI, pollutionTuning);
            char tuningDetail[192];
            std::snprintf(tuningDetail, sizeof(tuningDetail),
                          "(power314=%u power318=%.3f plumbF8=%u pollutionProps=%zu)",
                          powerTuning.field314, static_cast<double>(powerTuning.field318),
                          plumbingTuning.fieldF8, pollutionTuning.properties.size());
            Check(sharedTuningOk && !powerTuning.properties.empty() &&
                  powerTuning.properties.count(0xa911e2f1u) &&
                  plumbingTuning.properties.count(0x68f5436au) &&
                  pollutionTuning.properties.count(0x69501944u),
                  "Loaded Power/Plumbing/Pollution tunables from shared shipped exemplar 0xc911e35b",
                  tuningDetail);
            LandValueTuning landValueTuning{};
            CrimeTuning crimeTuning{};
            AuraTuning auraTuning{};
            PoliceTuning policeTuning{};
            FireProtectionTuning fireTuning{};
            FlammabilityTuning flammabilityTuning{};
            ResidentialTuning residentialTuning{};
            TrafficTuning trafficTuning{};
            const bool tuningsOk =
                loadTuningExemplar(LandValueTuning::kExemplarTGI, landValueTuning) &&
                loadTuningExemplar(CrimeTuning::kExemplarTGI, crimeTuning) &&
                loadTuningExemplar(AuraTuning::kExemplarTGI, auraTuning) &&
                loadTuningExemplar(PoliceTuning::kExemplarTGI, policeTuning) &&
                loadTuningExemplar(FireProtectionTuning::kExemplarTGI, fireTuning) &&
                loadTuningExemplar(FlammabilityTuning::kExemplarTGI, flammabilityTuning) &&
                loadTuningExemplar(ResidentialTuning::kExemplarTGI, residentialTuning) &&
                loadTuningExemplar(TrafficTuning::kExemplarTGI, trafficTuning);
            FloraTuning floraTuning{};
            WeatherTuning weatherTuning{};
            SimulatorTuning simulatorTuning{};
            const bool floraWeatherTuningOk =
                loadTuningExemplar(FloraTuning::kExemplarTGI, floraTuning) &&
                loadTuningExemplar(WeatherTuning::kExemplarTGI, weatherTuning) &&
                loadTuningExemplar(SimulatorTuning::kExemplarTGI, simulatorTuning);
            Check(tuningsOk && floraWeatherTuningOk &&
                  auraTuning.properties.count(0xea02327au) &&
                  crimeTuning.properties.count(0x8a0cbaadu) &&
                  residentialTuning.properties.count(0x8a891421u) &&
                  !trafficTuning.networkCapacities.empty() &&
                  floraTuning.properties.count(0xc9dbdd00u) &&
                  weatherTuning.properties.count(0xa7607d70u) &&
                  simulatorTuning.speedPeriodMs[0] > simulatorTuning.speedPeriodMs[2],
                  "Loaded LandValue/Crime/Aura/Police/Fire/Flammability/Residential/Traffic/Flora/Weather/Simulator tunables from shipped exemplars");

            SimulatorSaveRecord simRecord = bigCity.SimulatorRecord();
            cIGZOStream simOut;
            simRecord.Write(simOut);
            std::vector<Uint8> rawSimBytes;
            bool simIdentical = false;
            if (bigCity.HasCivicSaveRecord(kCLSID_cSC4Simulator) &&
                dbpfSys.Archives()[0]->IsOpen()) {
                cDBPFArchive simArc;
                if (simArc.Open(bigCityPath) &&
                    simArc.ReadFirstByType(kCLSID_cSC4Simulator, rawSimBytes) &&
                    rawSimBytes.size() == 12u + simOut.Buffer().size()) {
                    simIdentical = std::memcmp(simOut.Buffer().data(),
                                               rawSimBytes.data() + 12,
                                               simOut.Buffer().size()) == 0;
                }
            }
            Uint32 resets = 0;
            SimulatorSaveRecord simTest{};
            const bool speedSetOk =
                simTest.DoSetSimSpeed(2, simulatorTuning) &&
                simTest.targetTicksPerSec9C == 15 &&
                simTest.intervalPeriodMs98 == simulatorTuning.speedPeriodMs[2] &&
                simTest.ComputeTickTimeBudgetMs(10u) == 56u &&
                !simTest.StepSpeedGatedTimePassage(simTest.intervalPeriodMs98 - 1u, resets) &&
                simTest.StepSpeedGatedTimePassage(simTest.intervalPeriodMs98, resets) &&
                resets == 1u && simTest.intervalPending[4];
            Check(simIdentical && speedSetOk,
                  "cSC4Simulator (0x2990c1e5) v4 byte-exact Read/Write roundtrip & speed gating (@ 0x1001efd52 / 0x1001f05d6 / 0x1001f0b22)");

            // Verify exact cSC4TileNeighborhoodCompact::GetMirrorImageEquivalent (@ 0x1001aee42),
            // PatchNetworkTilePair (@ 0x1001b9124), ComputeZoneTypeForNetworkCell (@ 0x10040800c),
            // and all 24 shipped RUL1/RUL2 network evaluators (0x01..0x18) + MultitypeRules (0x10000001).
            cSC4TileNeighborhoodCompact nbIn{};
            // Edge 1 (North) with major=2 and bit 4 (0x10) set; Edge 0 (West) with major=1.
            nbIn.packed = (0x01u << 0) | ((0x10u | 0x02u) << 8);
            const cSC4TileNeighborhoodCompact nbMir = nbIn.GetMirrorImageEquivalent();
            // Edge 0 (W) moves to Edge 2 (E) with major 1->3; Edge 1 (N) stays on Edge 1 with bit 4->5 (0x22).
            const bool mirrorExact =
                ((nbMir.packed >> 16) & 0xffu) == 0x03u &&
                ((nbMir.packed >> 8) & 0xffu) == 0x22u;

            std::array<Sint8, 9> zoneTestGrid{
                0, 2, 0, // N (0,-1) is Low-Density Res (2, prio 7)
                0, 0, 0,
                3, 0, 0  // SW (-1,+1) is High-Density Res (3, prio 6)
            };
            // Because cardinal pass finds zone 2 (not in 0x4081), binary breaks before diagonals!
            const Uint32 zoneSelected = ComputeZoneTypeForNetworkCell(1, 1, 3, 3, zoneTestGrid);

            NetworkTileConflictRule patchRule{};
            patchRule.pieceA = 0x100u;
            patchRule.rotFlipA = 0x02u; // after dirFromAToB=0 (+6 -> +2 mod 4)
            patchRule.pieceB = 0x200u;
            patchRule.rotFlipB = 0x02u;
            patchRule.replacementA = 0x300u;
            patchRule.replacementRotFlipA = 0x00u;
            patchRule.replacementB = 0x400u;
            patchRule.replacementRotFlipB = 0x01u;
            NetworkSolvedCell cellA{0x100u, 0x00u}, cellB{0x200u, 0x00u};
            const Uint32 patchRes = PatchNetworkTilePair(
                std::span<const NetworkTileConflictRule>(&patchRule, 1), cellA, cellB, 0);

            Uint32 loadedRulEvaluators = 0;
            std::size_t totalEvaluatedRules = 0;
            for (Uint32 inst = 1u; inst <= 0x18u; ++inst) {
                std::vector<Uint8> rulBytes;
                cSC4TileEvaluator ev;
                if (dbpfSys.LoadAssetByTGI(cSC4TGI{kTypeRUL, kGroupRUL, inst}, rulBytes) &&
                    ev.LoadFromText(std::string_view(
                        reinterpret_cast<const char*>(rulBytes.data()), rulBytes.size())) &&
                    ev.GetRuleCount() > 0u) {
                    ++loadedRulEvaluators;
                    totalEvaluatedRules += ev.GetRuleCount();
                }
            }
            std::vector<Uint8> multitypeBytes;
            cSC4NetworkRuleTables multiTables{};
            const bool loadedMultitype =
                dbpfSys.LoadAssetByTGI(cSC4TGI{kTypeRUL, kGroupRUL, 0x10000001u}, multitypeBytes) &&
                multiTables.LoadMultitypeRul(std::string_view(
                    reinterpret_cast<const char*>(multitypeBytes.data()), multitypeBytes.size())) &&
                !multiTables.mixedTypeSolutions.empty();
            char netExactDetail[160];
            std::snprintf(netExactDetail, sizeof(netExactDetail),
                          "(ruls=%u/24 totalRules=%zu multitype=%zu zone=%u patch=%u)",
                          loadedRulEvaluators, totalEvaluatedRules,
                          multiTables.mixedTypeSolutions.size(), zoneSelected, patchRes);
            Check(mirrorExact && zoneSelected == 2u && patchRes == 1u &&
                  cellA.pieceId == 0x300u && cellB.pieceId == 0x400u &&
                  loadedRulEvaluators == 24u && loadedMultitype,
                  "Exact binary network RUL mirror/patch/zone-priority & all 24 shipped network RULs + MultitypeRules",
                  netExactDetail);
        }
        char developmentDetail[192];
        std::snprintf(developmentDetail, sizeof(developmentDetail),
                      "(loaded=%d, min=%d, max=%d, multiplier=%.3f, steps=%u)",
                      bigCity.BuildingDevelopment().loadedTunablesFromExemplar ? 1 : 0,
                      bigCity.BuildingDevelopment().minimumConstructionResources,
                      bigCity.BuildingDevelopment().maximumConstructionResources,
                      bigCity.BuildingDevelopment().demandToResourceMultiplier,
                      bigCity.BuildingDevelopment().idlePlanningSteps);
        Check(bigCity.BuildingDevelopment().loadedTunablesFromExemplar &&
              bigCity.BuildingDevelopment().minimumConstructionResources == 8192 &&
              bigCity.BuildingDevelopment().maximumConstructionResources == 20480 &&
              std::abs(bigCity.BuildingDevelopment().demandToResourceMultiplier - 0.5f) < 0.001f &&
              bigCity.BuildingDevelopment().idlePlanningSteps == 10,
              "Loaded cSC4BuildingDevelopmentSimulator tunables from shipped exemplar",
              developmentDetail);
        cDBPFArchive bigCityArchive;
        std::vector<Uint8> simulatorBytes;
        Uint16 simulatorVersion = 0;
        bool foundSimulator = bigCityArchive.Open(bigCityPath) &&
                              bigCityArchive.ReadFirstByType(kCLSID_cSC4Simulator, simulatorBytes);
        Uint32 simulatorRecordSize = 0;
        if (simulatorBytes.size() >= 14) {
            std::memcpy(&simulatorRecordSize, simulatorBytes.data(), sizeof(simulatorRecordSize));
            std::size_t versionOffset = simulatorRecordSize == simulatorBytes.size() ? 12 : 0;
            std::memcpy(&simulatorVersion, simulatorBytes.data() + versionOffset, sizeof(simulatorVersion));
        }
        char simulatorDetail[128];
        std::snprintf(simulatorDetail, sizeof(simulatorDetail), "(version %u, %zu bytes)",
                      simulatorVersion, simulatorBytes.size());
        Check(foundSimulator && simulatorVersion == 4,
              "Found cSC4Simulator version-4 save subfile", simulatorDetail);
        Check(bigCity.SimYear() >= 2000 && bigCity.SimMonth() >= 1 && bigCity.SimMonth() <= 12 &&
              bigCity.SimDay() >= 1 && bigCity.SimDay() <= 31 &&
              bigCity.SimJulianDay() != 2451545u,
              "Decoded cSC4Simulator version-4 cRZDate Julian day");
        std::vector<Uint8> tractDevBytes;
        TractDeveloperState savedTractDev{};
        const bool decodedTractDev =
            bigCityArchive.ReadFirstByType(kCLSID_cSC4TractDeveloper, tractDevBytes) &&
            savedTractDev.ReadFromSC4Record(tractDevBytes);
        savedTractDev.ApplyTuningDefaults(installedTractTuning);
        char tractDevDetail[160];
        std::snprintf(
            tractDevDetail,
            sizeof(tractDevDetail),
            "(bytes=%zu, styles=%zu, currentStyle=0x%x, yearsBetween=%d)",
            tractDevBytes.size(),
            savedTractDev.activeStyles.size(),
            savedTractDev.GetCurrentStyle(),
            savedTractDev.yearsBetweenStyleChanges);
        Check(decodedTractDev && !savedTractDev.activeStyles.empty() &&
              savedTractDev.GetCurrentStyle() >= 0x2000u &&
              savedTractDev.yearsBetweenStyleChanges > 0,
              "Decoded cSC4TractDeveloper (0x2990c142) saved style state from .sc4 save",
              tractDevDetail);
        {
            // Shipped RUL2 resources (type 0x0a5bcf4b, group 0xaa5bcf57) from SimCity_1.dat:
            // RoadSimple=7, RoadComplex=8, plus Network INI and RUL0 override resources.
            std::vector<Uint8> roadSimpleBytes;
            const bool gotRoadSimple = dbpfSys.LoadAssetByTGI(
                cSC4TGI{kTypeRUL, kGroupRUL, 7u}, roadSimpleBytes);
            cSC4TileEvaluator roadSimpleEval;
            bool roadRulesOk = false;
            if (gotRoadSimple && !roadSimpleBytes.empty()) {
                const std::string rulStr(
                    reinterpret_cast<const char*>(roadSimpleBytes.data()), roadSimpleBytes.size());
                roadRulesOk = roadSimpleEval.LoadFromText(rulStr) && roadSimpleEval.GetRuleCount() > 0;
            }
            char rulDetail[128];
            std::snprintf(rulDetail, sizeof(rulDetail), "(bytes=%zu, rules=%zu)",
                          roadSimpleBytes.size(), roadSimpleEval.GetRuleCount());
            Check(gotRoadSimple && roadRulesOk,
                  "Loaded shipped RoadSimple RUL2 (0x0a5bcf4b/0xaa5bcf57/7) into cSC4TileEvaluator", rulDetail);

            std::vector<Uint8> networkIniBytes;
            bool gotNetworkIni = false;
            for (const auto& entry : dat1.Entries()) {
                if (entry.tgi.group == kGroupNetworkIni && entry.tgi.instance == kInstNetworkIni) {
                    gotNetworkIni = dbpfSys.LoadAssetByTGI(entry.tgi, networkIniBytes);
                    break;
                }
            }
            cSC4NetworkRuleTables shippedTables;
            bool iniOk = false;
            if (gotNetworkIni && !networkIniBytes.empty()) {
                const std::string iniStr(
                    reinterpret_cast<const char*>(networkIniBytes.data()), networkIniBytes.size());
                iniOk = shippedTables.LoadNetworkIni(iniStr);
            }
            std::vector<Uint8> overrideBytes;
            const bool gotOverride = dbpfSys.LoadAssetByTGI(
                cSC4TGI{kTypeRUL, kGroupRUL, 0x10000002u}, overrideBytes);
            bool overrideOk = false;
            if (gotOverride && !overrideBytes.empty()) {
                const std::string rulStr(
                    reinterpret_cast<const char*>(overrideBytes.data()), overrideBytes.size());
                overrideOk = shippedTables.LoadOverrideRul(rulStr);
            }
            char iniDetail[160];
            std::snprintf(iniDetail, sizeof(iniDetail),
                          "(iniBytes=%zu texture=%zu shadow=%zu path=%zu, override=%d conflicts=%zu)",
                          networkIniBytes.size(), shippedTables.textureRemaps.size(),
                          shippedTables.shadowRemaps.size(), shippedTables.ukPathRemaps.size(),
                          overrideOk ? 1 : 0, shippedTables.tileConflictRules.size());
            Check(gotNetworkIni && iniOk,
                  "Loaded shipped Network INI texture/shadow/path remapping tables", iniDetail);
            Check(gotOverride && overrideOk,
                  "Loaded shipped RUL0 RuleOverrides tile-conflict table", iniDetail);

            // All shipped cSC4NetworkLotConfiguration exemplars (ExemplarType 0x21).
            std::size_t networkLotConfigs = 0;
            for (const auto& entry : dat1.Entries()) {
                if (entry.tgi.type != kTypeExemplar) {
                    continue;
                }
                std::vector<Uint8> cfgBytes;
                cGZExemplar cfgExemplar;
                if (!dbpfSys.LoadAssetByTGI(entry.tgi, cfgBytes) ||
                    !cfgExemplar.Parse(cfgBytes.data(), cfgBytes.size()) ||
                    cfgExemplar.GetInt(kPropExemplarType, 0) != 0x21) {
                    continue;
                }
                cSC4NetworkLotConfiguration cfg{};
                if (cfg.LoadFromExemplar(cfgExemplar)) {
                    ++networkLotConfigs;
                }
            }
            char netLotDetail[96];
            std::snprintf(netLotDetail, sizeof(netLotDetail), "(configs=%zu)", networkLotConfigs);
            Check(networkLotConfigs > 0,
                  "Loaded shipped cSC4NetworkLotConfiguration exemplars (ExemplarType 0x21)", netLotDetail);

            // cSC4NetworkLotManager (0xe9a6cd3b) save subfile inside Big City Tutorial.
            std::vector<Uint8> netLotBytes;
            NetworkLotManagerSaveRecord netLotRecord{};
            const bool decodedNetLot =
                bigCityArchive.ReadFirstByType(kCLSID_cSC4NetworkLotManager, netLotBytes) &&
                netLotRecord.ReadFromSC4Record(netLotBytes);
            cIGZOStream netLotOut;
            const bool wroteNetLot = decodedNetLot && netLotRecord.Write(netLotOut);
            cIGZIStream netLotIn(netLotOut.Buffer().data(), netLotOut.Buffer().size());
            NetworkLotManagerSaveRecord netLotRoundtrip{};
            const bool netLotRoundtripOk = wroteNetLot && netLotRoundtrip.Read(netLotIn) &&
                netLotRoundtrip.version == 2u &&
                netLotRoundtrip.networkLots.size() == netLotRecord.networkLots.size() &&
                netLotRoundtrip.damageEffects.size() == netLotRecord.damageEffects.size();
            char netLotSaveDetail[160];
            std::snprintf(netLotSaveDetail, sizeof(netLotSaveDetail),
                          "(bytes=%zu, lots=%zu, effects=%zu, roundtrip=%d)",
                          netLotBytes.size(), netLotRecord.networkLots.size(),
                          netLotRecord.damageEffects.size(), netLotRoundtripOk ? 1 : 0);
            Check(decodedNetLot && netLotRecord.version == 2u && netLotRoundtripOk,
                  "Decoded cSC4NetworkLotManager (0xe9a6cd3b) version-2 save record from .sc4", netLotSaveDetail);
        }
        std::vector<Uint8> growthDevBytes;
        std::vector<GrowthDeveloperSaveRecord> savedGrowthDevs;
        const bool decodedGrowthDevs =
            bigCityArchive.ReadFirstByType(kCLSID_cSC4GrowthDeveloper, growthDevBytes) &&
            ParseGrowthDeveloperSubfile(growthDevBytes, savedGrowthDevs);
        bool validGridCLSIDs = !savedGrowthDevs.empty();
        for (const auto& rec : savedGrowthDevs) {
            validGridCLSIDs &=
                (rec.zonableCellsGrid.clsid == 0x49b9e602u) &&
                (rec.desirabilityGrid.clsid == 0x49b9e605u) &&
                (rec.populationGrid.clsid == 0x49b9e605u) &&
                (rec.capacityGrid.clsid == 0x49b9e604u) &&
                (rec.maxCapacityGrid.clsid == 0x49b9e604u) &&
                (rec.zonableCellsGrid.memAddr != 0u) &&
                (rec.desirabilityGrid.memAddr != 0u) &&
                (rec.populationGrid.memAddr != 0u) &&
                (rec.capacityGrid.memAddr != 0u) &&
                (rec.maxCapacityGrid.memAddr != 0u);
        }
        std::vector<Uint8> bldgDevSimBytes;
        BuildingDevelopmentSimSaveRecord savedBldgDevSim{};
        const bool decodedBldgDevSim =
            bigCityArchive.ReadFirstByType(kCLSID_cSC4BuildingDevSim, bldgDevSimBytes) &&
            savedBldgDevSim.ReadFromSC4Record(bldgDevSimBytes);
        std::size_t matchedDevMemAddrs = 0;
        for (const auto& entry : savedBldgDevSim.developers) {
            if (entry.developer.clsid == kCLSID_cSC4GrowthDeveloper) {
                for (const auto& rec : savedGrowthDevs) {
                    if (rec.memAddr == entry.developer.memAddr) {
                        ++matchedDevMemAddrs;
                        break;
                    }
                }
            }
        }
        cIGZOStream bldgDevOut;
        savedBldgDevSim.Write(bldgDevOut);
        cIGZIStream bldgDevIn(bldgDevOut.Buffer().data(), bldgDevOut.Buffer().size());
        BuildingDevelopmentSimSaveRecord roundtripBldgDevSim{};
        const bool bldgDevRoundtripOk =
            roundtripBldgDevSim.Read(bldgDevIn) &&
            roundtripBldgDevSim.developers.size() == savedBldgDevSim.developers.size() &&
            roundtripBldgDevSim.targetGrowthStages.size() == savedBldgDevSim.targetGrowthStages.size() &&
            roundtripBldgDevSim.growthMarks.size() == savedBldgDevSim.growthMarks.size() &&
            roundtripBldgDevSim.declinePlans.size() == savedBldgDevSim.declinePlans.size() &&
            roundtripBldgDevSim.hasReachedStep10 == savedBldgDevSim.hasReachedStep10 &&
            roundtripBldgDevSim.hasSeenStage7 == savedBldgDevSim.hasSeenStage7 &&
            roundtripBldgDevSim.hasSeenStage8 == savedBldgDevSim.hasSeenStage8;
        char growthDevDetail[200];
        std::snprintf(
            growthDevDetail,
            sizeof(growthDevDetail),
            "(devs=%zu, matched=%zu, stages=%zu, marks=%zu, declines=%zu, validGrids=%d)",
            savedGrowthDevs.size(),
            matchedDevMemAddrs,
            savedBldgDevSim.targetGrowthStages.size(),
            savedBldgDevSim.growthMarks.size(),
            savedBldgDevSim.declinePlans.size(),
            validGridCLSIDs ? 1 : 0);
        Check(decodedGrowthDevs && decodedBldgDevSim &&
              savedGrowthDevs.size() == 12u &&
              savedBldgDevSim.developers.size() == 12u &&
              matchedDevMemAddrs == 12u &&
              savedBldgDevSim.developmentMapGrid.clsid == 0x49b9e602u &&
              savedBldgDevSim.energyDemand.clsid == kGZCLSID_cSC4DemandSerializable &&
              validGridCLSIDs && bldgDevRoundtripOk,
              "Decoded cSC4BuildingDevelopmentSimulator (0x8990be7a) & all 12 cSC4GrowthDeveloper (0x0990c138) links from .sc4 save",
              growthDevDetail);
        std::vector<Uint8> demandSimBytes;
        DemandSimulatorSaveRecord savedDemandSim{};
        const bool decodedDemandSim =
            bigCityArchive.ReadFirstByType(kCLSID_cSC4DemandSimulator, demandSimBytes) &&
            savedDemandSim.ReadFromSC4Record(demandSimBytes);
        std::size_t validDemandRefs = 0;
        for (const auto& entry : savedDemandSim.demands) {
            if (entry.demand.clsid == kGZCLSID_cSC4DemandSerializable &&
                entry.demand.memAddr != 0u) {
                ++validDemandRefs;
            }
        }
        cIGZOStream demandSimOut;
        savedDemandSim.Write(demandSimOut);
        cIGZIStream demandSimIn(demandSimOut.Buffer().data(), demandSimOut.Buffer().size());
        DemandSimulatorSaveRecord roundtripDemandSim{};
        const bool demandSimRoundtripOk =
            roundtripDemandSim.Read(demandSimIn) &&
            roundtripDemandSim.demands.size() == savedDemandSim.demands.size() &&
            roundtripDemandSim.census.size() == savedDemandSim.census.size() &&
            roundtripDemandSim.commercialConnectionBonus == savedDemandSim.commercialConnectionBonus &&
            roundtripDemandSim.industrialConnectionBonus == savedDemandSim.industrialConnectionBonus;
        char demandSimDetail[180];
        std::snprintf(
            demandSimDetail,
            sizeof(demandSimDetail),
            "(demands=%zu, validRefs=%zu, census=%zu, comConn=%.1f, indConn=%.1f)",
            savedDemandSim.demands.size(),
            validDemandRefs,
            savedDemandSim.census.size(),
            savedDemandSim.commercialConnectionBonus,
            savedDemandSim.industrialConnectionBonus);
        Check(decodedDemandSim &&
              savedDemandSim.demands.size() >= 150u &&
              validDemandRefs == savedDemandSim.demands.size() &&
              savedDemandSim.census.size() >= 24u &&
              demandSimRoundtripOk,
              "Decoded cSC4DemandSimulator (0x2990bff2) demand map, census snapshot, and connection bonuses from .sc4 save",
              demandSimDetail);
        std::vector<Uint8> lotDevBytes;
        LotDeveloperSaveRecord savedLotDev{};
        const bool decodedLotDev =
            bigCityArchive.ReadFirstByType(kCLSID_cSC4LotDeveloper, lotDevBytes) &&
            savedLotDev.ReadFromSC4Record(lotDevBytes);
        std::size_t validBuildingQueueRefs = 0;
        for (const auto& ref : savedLotDev.buildingQueue) {
            if (ref.clsid == kCLSID_cSC4BuildingOccupant && ref.memAddr != 0u) {
                ++validBuildingQueueRefs;
            }
        }
        std::vector<Uint8> lotConfigMgrBytes;
        LotConfigurationManagerSaveRecord savedLotConfigMgr{};
        const bool decodedLotConfigMgr =
            bigCityArchive.ReadFirstByType(kCLSID_cSC4LotConfigurationManager, lotConfigMgrBytes) &&
            savedLotConfigMgr.ReadFromSC4Record(lotConfigMgrBytes);
        cIGZOStream lotDevOut;
        savedLotDev.Write(lotDevOut);
        cIGZIStream lotDevIn(lotDevOut.Buffer().data(), lotDevOut.Buffer().size());
        LotDeveloperSaveRecord roundtripLotDev{};
        const bool lotDevRoundtripOk =
            roundtripLotDev.Read(lotDevIn) &&
            roundtripLotDev.buildingQueue.size() == savedLotDev.buildingQueue.size() &&
            roundtripLotDev.currentQueueIndex == savedLotDev.currentQueueIndex &&
            roundtripLotDev.nightChangeMode == savedLotDev.nightChangeMode;
        char lotDevDetail[180];
        std::snprintf(
            lotDevDetail,
            sizeof(lotDevDetail),
            "(bldgQueue=%zu, validRefs=%zu, curIdx=%u, nightMode=%u, bldgDevSim=0x%x)",
            savedLotDev.buildingQueue.size(),
            validBuildingQueueRefs,
            savedLotDev.currentQueueIndex,
            savedLotDev.nightChangeMode,
            savedLotConfigMgr.buildingDevelopmentSim.clsid);
        Check(decodedLotDev && decodedLotConfigMgr &&
              !savedLotDev.buildingQueue.empty() &&
              validBuildingQueueRefs == savedLotDev.buildingQueue.size() &&
              savedLotConfigMgr.buildingDevelopmentSim.clsid == kCLSID_cSC4BuildingDevSim &&
              savedLotConfigMgr.floraSim.clsid == kCLSID_cSC4FloraSimulator &&
              savedLotConfigMgr.propManager.clsid == kCLSID_cSC4PropManager &&
              lotDevRoundtripOk,
              "Decoded cSC4LotDeveloper (0xa990bfe0) building queue and cSC4LotConfigurationManager (0x299062c0) from .sc4 save",
              lotDevDetail);
        Check(bigCity.LoadedDemandValues().size() == 174,
              "Decoded all 174 separately persisted cSC4Demand objects");
        Check(bigCity.LoadedDemandRecords().size() == 174,
              "Decoded complete cSC4Demand supply/demand/modifier record layout");
        Check(bigCity.LoadedDemandValues().count(0x21010u) == 1 &&
              bigCity.LoadedDemandValues().count(0x23110u) == 1 &&
              bigCity.LoadedDemandValues().count(0x23320u) == 1 &&
              bigCity.LoadedDemandValues().count(0x24100u) == 1 &&
              bigCity.LoadedDemandValues().count(0x24400u) == 1,
              "Decoded saved R/C/I demand IDs and Float32 values");
        const auto& aggregateDemand = bigCity.LoadedDemandRecords().at(0x1000u);
        Check(std::abs(aggregateDemand.demandValue - 20.0f) < 0.01f &&
              std::abs(aggregateDemand.activeDemand + 18000.0f) < 0.01f &&
              bigCity.LoadedDemandValue(0x1000u) == aggregateDemand.activeDemand,
              "Distinguished cSC4Demand supply, raw demand, and active demand fields");
        SC4DemandRecord recalculatedAggregate = aggregateDemand;
        recalculatedAggregate.EndCycleAndCalculate();
        Check(std::abs(recalculatedAggregate.activeDemand - aggregateDemand.activeDemand) < 0.01f,
              "Recalculated saved aggregate demand with decompiled active-demand formula");
        RCIDemandState savedDemand = bigCity.Demand();
        Uint32 savedResidentialPop = bigCity.ResidentialPop();
        Uint32 savedCommercialPop = bigCity.CommercialPop();
        Uint32 savedIndustrialPop = bigCity.IndustrialPop();
        bigCity.StepSimulation(1);
        Check(bigCity.ResidentialPop() == savedResidentialPop &&
              bigCity.CommercialPop() == savedCommercialPop &&
              bigCity.IndustrialPop() == savedIndustrialPop &&
              std::memcmp(&bigCity.Demand(), &savedDemand, sizeof(savedDemand)) == 0,
              "Original-save simulation preserves decoded census and demand pending exact phase updates");
        const Sint32 expectedConstructionResources =
            bigCity.BuildingDevelopment().CalculateMonthlyResources(
                bigCity.LoadedDemandValue(0x1001u),
                bigCity.LoadedDemandValue(0x3001u),
                bigCity.LoadedDemandValue(0x4001u));
        bigCity.StepSimulation(13);
        Check(bigCity.SimMonth() == 10 && bigCity.SimDay() == 1 &&
              bigCity.BuildingDevelopment().dailyCycles == 14 &&
              bigCity.BuildingDevelopment().monthlyCycles == 1 &&
              bigCity.BuildingDevelopment().constructionResources ==
                  static_cast<Float32>(expectedConstructionResources),
              "Building development runs exact aggregate-demand resource calculation on monthly event");
        std::size_t totalSaveGrowthItems = 0;
        std::size_t totalSaveDeclineItems = 0;
        for (const auto& [subType, gList] : bigCity.GrowthDeveloperGrowthLists()) {
            (void)subType;
            totalSaveGrowthItems += gList.size();
        }
        for (const auto& [subType, dList] : bigCity.GrowthDeveloperDeclineLists()) {
            (void)subType;
            totalSaveDeclineItems += dList.size();
        }
        char saveDevListsDetail[160];
        std::snprintf(
            saveDevListsDetail,
            sizeof(saveDevListsDetail),
            "(tunings=%zu, growthLists=%zu, growthItems=%zu, declineItems=%zu, attempts=%u)",
            bigCity.GrowthDeveloperTunings().size(),
            bigCity.GrowthDeveloperGrowthLists().size(),
            totalSaveGrowthItems,
            totalSaveDeclineItems,
            bigCity.BuildingDevelopment().growthAttempts);
        Check(bigCity.GrowthDeveloperTunings().size() == 12u &&
              bigCity.GrowthDeveloperGrowthLists().size() == 12u &&
              totalSaveGrowthItems > 0u &&
              bigCity.BuildingDevelopment().growthAttempts > 0u,
              "Evaluated all 12 cSC4GrowthDeveloper growth/decline lists and daily tract marks on Big City Tutorial.sc4",
              saveDevListsDetail);
        Check(bigCity.Lots().size() == 3569, "Decoded all 3,569 cSC4Lot records");
        Check(bigCity.Buildings().size() > 3400, "Decoded > 3,400 cSC4BuildingOccupant records");
        std::size_t resolvedModels = 0;
        for (const auto& building : bigCity.Buildings()) {
            if (building.s3dInstanceId != 0 && building.fshTextureId != 0) ++resolvedModels;
        }
        Check(resolvedModels > 1000, "Resolved > 1,000 building Exemplar -> S3D -> FSH asset chains");
        Check(bigCity.Networks().size() >= 2685, "Decoded >= 2,685 cSC4NetworkOccupant road/rail/street tiles");
        std::size_t roads = 0, streets = 0, rails = 0;
        for (const auto& network : bigCity.Networks()) {
            roads += network.networkType == NetworkType::Road;
            streets += network.networkType == NetworkType::Street;
            rails += network.networkType == NetworkType::Rail;
        }
        // Primary eNetworkType @ store +0x144 (GetEdgeTypesFlag @ 0x10019eb50);
        // matches piece families 0x00 road / 0x05 street / 0x03 rail.
        Check(roads == 1323u && streets == 1873u && rails == 139u,
              "Decoded cSC4EdgeConnectionStore network types (road/street/rail)",
              ("roads=" + std::to_string(roads) + " streets=" + std::to_string(streets) +
               " rails=" + std::to_string(rails)).c_str());
        std::size_t badLots = 0;
        for (const auto& lot : bigCity.Lots()) {
            if (lot.maxX - lot.minX >= 16 || lot.maxZ - lot.minZ >= 16 || lot.altitudeY < 1.0f) ++badLots;
        }
        Check(badLots == 0u, "cSC4Lot bounds/altitude decoded via LotFullSaveRecord::Read",
              ("badLots=" + std::to_string(badLots)).c_str());
        std::size_t offTerrain = 0;
        for (const auto& network : bigCity.Networks()) {
            for (const auto& v : network.verts) {
                const int vx = static_cast<int>(std::lround(v.x / 16.0f));
                const int vz = static_cast<int>(std::lround(v.z / 16.0f));
                if (std::fabs(v.y - bigCity.GetVertexHeight(vx, vz)) > 0.5f) { ++offTerrain; break; }
            }
        }
        Check(offTerrain == 0u, "cSC4NetworkOccupant corner vertices sit on terrain",
              ("offTerrain=" + std::to_string(offTerrain)).c_str());
        Check(bigCity.Flora().size() > 2000, "Decoded > 2,000 cSC4FloraOccupant trees");
        Check(bigCity.Props().size() > 50000, "Decoded > 50,000 cSC4PropOccupant street/lot props");
        Check(!bigCity.PngThumbnail().empty(), "Extracted embedded PNG city thumbnail (0x8a2482b9)");
        std::vector<Uint8> regionThumbnail;
        auto bigRegionIt = std::find_if(dbpfSys.RegionCities().begin(), dbpfSys.RegionCities().end(),
            [](const RegionCityInfo& city) { return city.fileName.find("Big City Tutorial") != std::string::npos; });
        std::size_t bigRegionIndex = static_cast<std::size_t>(
            std::distance(dbpfSys.RegionCities().begin(), bigRegionIt));
        Check(bigRegionIt != dbpfSys.RegionCities().end() &&
              dbpfSys.ReadCityThumbnailPNG(bigRegionIndex, regionThumbnail) && !regionThumbnail.empty(),
              "Region View streams embedded city PNG thumbnails by city index");
        Check(!bigCity.LoadedSint8Grids().empty() && !bigCity.LoadedUint8Grids().empty() && !bigCity.LoadedFloat32Grids().empty(),
              "Decoded embedded cSC4SimGrid<Sint8/Uint8/Sint16/Uint16/Float32> subfiles from .sc4 save");

        // Verify decompiled civic & utility simulation / grid update methods.
        {
            const std::array<Sint16, 4> unadjProt = {-20, 0, 100, 300};
            std::array<Uint8, 4> adjProt{};
            ProtectionSimulatorSaveRecord::CalculateAdjustedProtectionGrid(unadjProt, 1.25f, adjProt);
            const bool protOk =
                adjProt[0] == 0u && adjProt[1] == 0u && adjProt[2] == 125u && adjProt[3] == 255u;

            const bool lvTypeOk =
                LandValueSimulatorSaveRecord::GetLandValueTypeForValue(10u, 32u, 96u) == 1u &&
                LandValueSimulatorSaveRecord::GetLandValueTypeForValue(50u, 32u, 96u) == 2u &&
                LandValueSimulatorSaveRecord::GetLandValueTypeForValue(120u, 32u, 96u) == 3u;

            const bool comProxOk =
                bigCity.CommercialSaveRecord().GetProximityMapRef(0x3320u) ==
                    bigCity.CommercialSaveRecord().gridRef58 &&
                bigCity.CommercialSaveRecord().GetProximityMapRef(0x3330u) ==
                    bigCity.CommercialSaveRecord().gridRef60 &&
                bigCity.CommercialSaveRecord().GetProximityMapRef(0x1010u).memAddr == 0u;

            PoliceSimulatorSaveRecord policeSim = bigCity.PoliceSaveRecord();
            policeSim.field150 = 100;
            policeSim.field15C = 15;
            policeSim.field160 = 7;
            policeSim.DoMonthlyCriminalRehabilitation(0.10f);
            policeSim.ProcessMonthlyCrimeStats();
            const bool policeOk =
                policeSim.field150 == 105 && policeSim.field154 == 15 &&
                policeSim.field15C == 0 && policeSim.field158 == 7 &&
                policeSim.field160 == 0;

            CrimeSimulatorSaveRecord crimeSim = bigCity.CrimeSaveRecord();
            crimeSim.fieldCC = 1;
            crimeSim.fieldD0 = 1;
            std::array<Uint8, 4> crimeCells = {0u, 40u, 80u, 100u};
            crimeSim.DecayCrimeGrid(crimeCells, 2u, 2u, 0.5f);
            const Float32 econFactor =
                CrimeSimulatorSaveRecord::ComputeEconomicCrimeFactor(150.0f, 200.0f, 100.0f);
            const bool crimeOk =
                crimeCells[1] == 20u && crimeCells[3] == 50u &&
                crimeSim.fieldD4 == 50 && std::fabs(econFactor - 0.0f) < 1e-5f;

            AuraSimulatorSaveRecord auraSim = bigCity.AuraSaveRecord();
            const std::array<Sint8, 4> baseAura = {-128, -20, 10, 40};
            const std::array<Sint8, 4> transAura = {50, -10, 5, 20};
            const std::array<Uint16, 4> resPop = {100u, 10u, 20u, 30u};
            std::array<Sint8, 4> totalAura{};
            auraSim.UpdateAuraGrid(baseAura, transAura, resPop, 2u, 2u, 64, totalAura);
            // Active cells: (0,1)=-30 (pop 10), (1,0)=15 (pop 20), (1,1)=60 (pop 30)
            // Weighted average = (-300 + 300 + 1800) / 60 = 30.
            const bool auraOk =
                totalAura[0] == 0 && totalAura[1] == -30 &&
                totalAura[2] == 15 && totalAura[3] == 60 &&
                auraSim.field204 == -30 && auraSim.field1F8 == 60 &&
                auraSim.fieldD4 == 30;

            Check(protOk && lvTypeOk && comProxOk && policeOk && crimeOk && auraOk,
                  "Verified decompiled Protection, LandValue, Commercial, Police, Crime, and Aura simulation algorithms");

            SimulatorSaveRecord simAgents{};
            SimulatorTuning simTuningDef{};
            simAgents.SetCityEstablished(false);
            simAgents.AddAgent(0x1001u, 4, "EstablishedOnlyAgent", 1u);
            simAgents.AddAgent(0x1002u, 4, "UnestablishedAgent", 2u);
            const bool beforeEstOk =
                !simAgents.GetAgentEnabled("EstablishedOnlyAgent") &&
                simAgents.GetAgentEnabled("UnestablishedAgent");
            simAgents.SetCityEstablished(true);
            const bool afterEstOk =
                simAgents.GetAgentEnabled("EstablishedOnlyAgent") &&
                !simAgents.GetAgentEnabled("UnestablishedAgent");
            std::vector<std::string> agentNames;
            simAgents.EnumerateAgentsByName(agentNames);
            simAgents.EmergencyPause();
            const bool emergPauseOk =
                simAgents.IsPaused() && simAgents.IsEmergencyPaused() &&
                simAgents.IsAnyPaused() &&
                !simAgents.SetSimSpeed(2, simTuningDef);
            simAgents.EmergencyResume();
            const bool emergResumeOk =
                !simAgents.IsAnyPaused() &&
                simAgents.SetSimSpeed(2, simTuningDef) &&
                std::fabs(simAgents.GetAnimationTimeDilation(simTuningDef, false) - 2.0f) < 1e-5f;

            CommercialSimulatorSaveRecord comSim{};
            comSim.field38 = true;
            comSim.StepIdleStateMachine();
            comSim.StepIdleStateMachine();
            comSim.StepIdleStateMachine();
            const bool comIdleOk = comSim.idleStep3C == 0u && !comSim.field38;

            FlammabilitySimulatorSaveRecord flamSim{};
            const Uint8 scaledFlam =
                FlammabilitySimulatorSaveRecord::ComputeScaledFlammabilityByte(40, 1.5f);
            const std::array<Sint32Triple, 3> candHotspots = {{
                {10, 10, 1}, {12, 11, 1}, {50, 50, 1},
            }};
            flamSim.ComputeHotspotList(candHotspots, 8, 2);
            const bool flamOk =
                scaledFlam == 60u && flamSim.entries.size() == 1u &&
                flamSim.entries[0].a == 11 && flamSim.entries[0].b == 10 &&
                flamSim.entries[0].c == 2;

            SerializedPropertyHolder ordHolderA{}, ordHolderB{};
            SerializedPropertyEntry offProp{};
            offProp.propertyId = 0x12345678u;
            offProp.dbType = 0x7u;
            const Sint32 offVal = -15;
            offProp.rawValueBytes.resize(4);
            std::memcpy(offProp.rawValueBytes.data(), &offVal, 4);
            ordHolderA.properties.push_back(offProp);

            SerializedPropertyEntry facProp{};
            facProp.propertyId = 0x87654321u;
            facProp.dbType = 0xau;
            const Float32 facVal = 1.25f;
            facProp.rawValueBytes.resize(4);
            std::memcpy(facProp.rawValueBytes.data(), &facVal, 4);
            ordHolderB.properties.push_back(facProp);

            const std::array<SerializedPropertyHolder, 2> ordHolders = {ordHolderA, ordHolderB};
            const Sint32 cumOffset =
                OrdinanceSimulatorSaveRecord::ComputeCumulativeOffset(ordHolders, 0x12345678u, 0u);
            const Float32 cumFactor =
                OrdinanceSimulatorSaveRecord::ComputeCumulativeFactor(ordHolders, 0x87654321u, 0u);
            const bool ordCumOk =
                cumOffset == -15 && std::fabs(cumFactor - 1.25f) < 1e-5f;

            Check(beforeEstOk && afterEstOk && agentNames.size() == 2u &&
                      emergPauseOk && emergResumeOk && comIdleOk && flamOk && ordCumOk,
                  "Verified decompiled cSC4Simulator agent/pause/speed, cSC4CommercialSimulator::DoIdle, cSC4FlammabilitySimulator hotspots, and cSC4OrdinanceSimulator cumulative effects");
        }

        // M2 interop oracle: clone-and-patch saves for the original loader.
        {
            cDBPFArchive m2Source;
            std::vector<Uint8> m2SourceBytes;
            const bool m2SourceOpen = m2Source.Open(bigCityPath) &&
                                      m2Source.ReadRawFile(m2SourceBytes);
            const fs::path repoRoot = fs::path(__FILE__).parent_path().parent_path().parent_path();
            const fs::path unmodifiedPath = repoRoot / "build" / "m2_unmodified_test.sc4";
            const fs::path interopPath = repoRoot / "build" / "m2_interop_test.sc4";

            cSC4City pristineCity;
            const bool pristineSaved = m2SourceOpen &&
                pristineCity.LoadFromSC4Archive(m2Source, &dbpfSys) &&
                pristineCity.SaveToSC4File(unmodifiedPath.string());
            cDBPFArchive pristineArchive;
            std::vector<Uint8> pristineBytes;
            const bool pristineRead = pristineSaved && pristineArchive.Open(unmodifiedPath.string()) &&
                                      pristineArchive.ReadRawFile(pristineBytes);
            Check(pristineRead && pristineBytes == m2SourceBytes,
                  "M2 unmodified SaveToSC4File is byte-identical to the source archive (@ 0x10036a4ac)");

            cSC4City modifiedCity;
            const bool modifiedSaved = m2SourceOpen &&
                modifiedCity.LoadFromSC4Archive(m2Source, &dbpfSys) &&
                modifiedCity.SetTaxRate(1, 1, 9.0f) &&
                modifiedCity.SaveToSC4File(interopPath.string());
            cDBPFArchive modifiedArchive;
            std::vector<Uint8> modifiedBytes;
            const bool modifiedOpen = modifiedSaved && modifiedArchive.Open(interopPath.string()) &&
                                      modifiedArchive.ReadRawFile(modifiedBytes);
            cSC4City modifiedReloaded;
            const bool modifiedReloadedOk = modifiedOpen &&
                modifiedReloaded.LoadFromSC4Archive(modifiedArchive);

            std::set<Uint32> sourceTypes;
            std::set<Uint32> modifiedTypes;
            for (const auto& entry : m2Source.Entries()) sourceTypes.insert(entry.tgi.type);
            for (const auto& entry : modifiedArchive.Entries()) modifiedTypes.insert(entry.tgi.type);
            bool sameTgiOrder = modifiedOpen &&
                m2Source.Entries().size() == modifiedArchive.Entries().size();
            if (sameTgiOrder) {
                for (std::size_t i = 0; i < m2Source.Entries().size(); ++i) {
                    sameTgiOrder = sameTgiOrder &&
                        m2Source.Entries()[i].tgi == modifiedArchive.Entries()[i].tgi;
                }
            }
            Check(sameTgiOrder && sourceTypes == modifiedTypes && modifiedTypes.size() == 133u,
                  "M2 modified save preserves all 133 source type IDs and original index order (@ 0x10036a7c5)");

            bool rawEntriesOk = sameTgiOrder;
            std::size_t budgetEntries = 0;
            if (sameTgiOrder) {
                for (std::size_t i = 0; i < m2Source.Entries().size(); ++i) {
                    const auto& sourceEntry = m2Source.Entries()[i];
                    const auto& modifiedEntry = modifiedArchive.Entries()[i];
                    if (sourceEntry.tgi.type == kTypeDIR) continue;
                    std::vector<Uint8> sourceRaw;
                    std::vector<Uint8> modifiedRaw;
                    rawEntriesOk = rawEntriesOk &&
                        m2Source.ReadRawEntry(sourceEntry, sourceRaw) &&
                        modifiedArchive.ReadRawEntry(modifiedEntry, modifiedRaw);
                    if (sourceEntry.tgi.type == kCLSID_cSC4BudgetSimulator) {
                        ++budgetEntries;
                        rawEntriesOk = rawEntriesOk && sourceRaw != modifiedRaw &&
                                       !modifiedEntry.isCompressed;
                    } else {
                        rawEntriesOk = rawEntriesOk && sourceRaw == modifiedRaw &&
                            sourceEntry.isCompressed == modifiedEntry.isCompressed;
                    }
                }
            }
            Check(rawEntriesOk && budgetEntries == 1u,
                  "M2 clone patches only the raw budget record; every other stored entry is byte-identical (@ 0x10036b62f)");

            constexpr cSC4TGI kDirTGI{kTypeDIR, kTypeDIR, 0x286b1f03u};
            constexpr cSC4TGI kBudgetTGI{
                kCLSID_cSC4BudgetSimulator, 0x299b2d1bu, 0u};
            auto readDir = [&kDirTGI](cDBPFArchive& archive, std::vector<Uint8>& bytes) {
                const auto* entry = archive.FindEntry(kDirTGI);
                return entry != nullptr && archive.ReadEntry(*entry, bytes);
            };
            std::vector<Uint8> sourceDir;
            std::vector<Uint8> modifiedDir;
            const bool dirsRead = m2SourceOpen && modifiedOpen &&
                                  readDir(m2Source, sourceDir) &&
                                  readDir(modifiedArchive, modifiedDir);
            std::vector<Uint8> expectedDir;
            if (dirsRead) {
                const std::size_t wholeRecords = sourceDir.size() - sourceDir.size() % 16u;
                for (std::size_t pos = 0; pos < wholeRecords; pos += 16u) {
                    cSC4TGI tgi{};
                    std::memcpy(&tgi.type, sourceDir.data() + pos, 4);
                    std::memcpy(&tgi.group, sourceDir.data() + pos + 4, 4);
                    std::memcpy(&tgi.instance, sourceDir.data() + pos + 8, 4);
                    if (!(tgi == kBudgetTGI)) {
                        expectedDir.insert(expectedDir.end(), sourceDir.begin() + pos,
                                           sourceDir.begin() + pos + 16);
                    }
                }
                expectedDir.insert(expectedDir.end(), sourceDir.begin() + wholeRecords,
                                   sourceDir.end());
            }
            Check(dirsRead && sourceDir.size() == 73u * 16u &&
                      modifiedDir == expectedDir &&
                      sourceDir.size() - modifiedDir.size() == 16u,
                  "M2 regenerated DIR removes exactly the budget TGI and preserves all other records (@ 0x10036b62f)");

            std::vector<Uint8> savedBudget;
            Uint32 budgetRecordSize = 0;
            Uint32 budgetStoredCrc = 0;
            Uint32 budgetMemAddr = 0;
            const bool budgetRead = modifiedOpen &&
                                    modifiedArchive.ReadFirstByType(kCLSID_cSC4BudgetSimulator, savedBudget) &&
                                    savedBudget.size() >= 12u;
            if (budgetRead) {
                std::memcpy(&budgetRecordSize, savedBudget.data(), 4);
                std::memcpy(&budgetStoredCrc, savedBudget.data() + 4, 4);
                std::memcpy(&budgetMemAddr, savedBudget.data() + 8, 4);
            }
            const bool budgetCrcOk = budgetRead && budgetRecordSize == savedBudget.size() &&
                ComputeSC4RecordCRC(budgetMemAddr, savedBudget.data() + 12,
                                    savedBudget.size() - 12) == budgetStoredCrc;
            Check(budgetCrcOk,
                  "M2 dirty budget framing CRC is self-consistent with ComputeSC4RecordCRC (@ 0x1003d0df1)");
            Check(modifiedReloadedOk && modifiedReloaded.HasBudgetSaveRecord() &&
                      std::fabs(modifiedReloaded.GetTaxRate(1, 1) - 9.0f) < 1e-6f,
                  "M2 modified save reloads with residential-low tax rate 9.0 (@ 0x100221fb2)");

            cSC4City bondCity;
            const bool bondLoaded = m2SourceOpen &&
                                    bondCity.LoadFromSC4Archive(m2Source, &dbpfSys);
            const Sint64 bondFundsBefore = bondCity.TreasuryFunds();
            const std::size_t bondCountBefore = bondCity.BudgetSaveRecord().bonds.size();
            constexpr Sint32 kBondAmount = 10000;
            const bool bondIssued = bondLoaded && bondCity.IssueBond(kBondAmount);
            const fs::path bondPath = repoRoot / "build" / "m2_bond_interop_test.sc4";
            const bool bondSaved = bondIssued && bondCity.SaveToSC4File(bondPath.string());
            cDBPFArchive bondArchive;
            const bool bondOpen = bondSaved && bondArchive.Open(bondPath.string());
            cSC4City bondReloaded;
            const bool bondReloadedOk = bondOpen && bondReloaded.LoadFromSC4Archive(bondArchive);

            bool bondRawEntriesOk = bondOpen &&
                m2Source.Entries().size() == bondArchive.Entries().size();
            if (bondRawEntriesOk) {
                for (std::size_t i = 0; i < m2Source.Entries().size(); ++i) {
                    const auto& sourceEntry = m2Source.Entries()[i];
                    const auto& bondEntry = bondArchive.Entries()[i];
                    bondRawEntriesOk = bondRawEntriesOk && sourceEntry.tgi == bondEntry.tgi;
                    if (sourceEntry.tgi.type == kTypeDIR) continue;
                    std::vector<Uint8> sourceRaw;
                    std::vector<Uint8> bondRaw;
                    bondRawEntriesOk = bondRawEntriesOk &&
                        m2Source.ReadRawEntry(sourceEntry, sourceRaw) &&
                        bondArchive.ReadRawEntry(bondEntry, bondRaw);
                    if (sourceEntry.tgi == kBudgetTGI) {
                        bondRawEntriesOk = bondRawEntriesOk && sourceRaw != bondRaw &&
                                           !bondEntry.isCompressed;
                    } else {
                        bondRawEntriesOk = bondRawEntriesOk && sourceRaw == bondRaw &&
                            sourceEntry.isCompressed == bondEntry.isCompressed;
                    }
                }
            }
            std::vector<Uint8> bondDir;
            std::vector<Uint8> expectedBondDir;
            const bool bondDirRead = dirsRead && bondOpen && readDir(bondArchive, bondDir);
            if (bondDirRead) {
                const std::size_t wholeRecords = sourceDir.size() - sourceDir.size() % 16u;
                for (std::size_t pos = 0; pos < wholeRecords; pos += 16u) {
                    cSC4TGI tgi{};
                    std::memcpy(&tgi.type, sourceDir.data() + pos, 4);
                    std::memcpy(&tgi.group, sourceDir.data() + pos + 4, 4);
                    std::memcpy(&tgi.instance, sourceDir.data() + pos + 8, 4);
                    if (!(tgi == kBudgetTGI)) {
                        expectedBondDir.insert(expectedBondDir.end(), sourceDir.begin() + pos,
                                               sourceDir.begin() + pos + 16);
                    }
                }
                expectedBondDir.insert(expectedBondDir.end(), sourceDir.begin() + wholeRecords,
                                       sourceDir.end());
            }
            const bool bondPersisted = bondReloadedOk &&
                bondReloaded.TreasuryFunds() == bondFundsBefore + kBondAmount &&
                bondReloaded.BudgetSaveRecord().bonds.size() == bondCountBefore + 1u;
            Check(bondPersisted && bondRawEntriesOk && bondDirRead &&
                      bondDir == expectedBondDir && sourceDir.size() - bondDir.size() == 16u,
                  "M2 IssueBond persists treasury/bond, patches only budget, and removes exactly its DIR record (@ 0x100221fdc)");

            const DBPFHeader& sourceHeader = m2Source.Header();
            const DBPFHeader& modifiedHeader = modifiedArchive.Header();
            bool clonedHeaderBytes = modifiedBytes.size() >= 96u && m2SourceBytes.size() >= 96u;
            for (std::size_t i = 0; i < 96u && clonedHeaderBytes; ++i) {
                if (i >= 36u && i < 60u) continue; // patched index + hole fields
                clonedHeaderBytes = m2SourceBytes[i] == modifiedBytes[i];
            }
            Uint64 expectedOffset = 96u;
            bool offsetsOk = modifiedOpen;
            for (const auto& entry : modifiedArchive.Entries()) {
                offsetsOk = offsetsOk && entry.offset == expectedOffset;
                expectedOffset += entry.fileSize;
            }
            const bool headerOk = modifiedOpen && clonedHeaderBytes && offsetsOk &&
                modifiedHeader.dateCreated == sourceHeader.dateCreated &&
                modifiedHeader.dateModified == sourceHeader.dateModified &&
                modifiedHeader.holeEntryCount == 0u && modifiedHeader.holeOffset == 0u &&
                modifiedHeader.holeSize == 0u &&
                modifiedHeader.indexEntryCount == m2Source.Entries().size() &&
                modifiedHeader.indexOffset == expectedOffset &&
                modifiedHeader.indexSize == modifiedArchive.Entries().size() * 20u &&
                expectedOffset + modifiedHeader.indexSize == modifiedBytes.size();
            Check(headerOk,
                  "M2 modified header clones dates, zeroes holes, and rebuilds count/index fields (@ 0x10037020f)");

            Check(modifiedSaved && fs::exists(interopPath) &&
                      fs::file_size(interopPath) == modifiedBytes.size(),
                  "M2 interop save ready for original SimCity 4 Deluxe",
                  interopPath.string().c_str());
        }

        // Verify SaveToSC4Buffer preserves decoded simulator records across roundtrip.
        {
            std::vector<Uint8> bigCitySavedBuf;
            const bool savedOk = bigCity.SaveToSC4Buffer(bigCitySavedBuf);
            cDBPFArchive bigSavedArc;
            cSC4City bigReloaded;
            const bool reloadedOk =
                savedOk &&
                bigSavedArc.OpenFromMemory(bigCitySavedBuf, "big_roundtrip.sc4") &&
                bigReloaded.LoadFromSC4Archive(bigSavedArc);
            Check(reloadedOk &&
                  bigReloaded.HasBudgetSaveRecord() &&
                  bigReloaded.HasCivicSaveRecord(kCLSID_cSC4Simulator) &&
                  bigReloaded.HasCivicSaveRecord(kCLSID_cSC4TrafficSimulator) &&
                  bigReloaded.HasCivicSaveRecord(kCLSID_cSC4PowerSimulator) &&
                  bigReloaded.HasCivicSaveRecord(kCLSID_cSC4WeatherSimulator) &&
                  bigReloaded.TrafficSaveRecord().edgeDensities.size() ==
                      bigCity.TrafficSaveRecord().edgeDensities.size(),
                  "SaveToSC4Buffer preserves cSC4BudgetSimulator, cSC4Simulator, cSC4TrafficSimulator, and all civic simulator records");
        }

        // Verify 100% of every subfile in Big City Tutorial.sc4 and across all
        // 417 shipped .sc4 cities via RoundTripAnySC4Subfile.
        {
            std::set<Uint32> bctTypes;
            std::size_t bctSubfiles = 0;
            std::size_t bctUnknown = 0;
            SC4SubfileRoundTripStats bctStats{};
            for (const auto& entry : bigCityArchive.Entries()) {
                std::vector<Uint8> sub;
                if (!bigCityArchive.ReadEntry(entry, sub)) {
                    ++bctUnknown;
                    continue;
                }
                ++bctSubfiles;
                bctTypes.insert(entry.tgi.type);
                SC4SubfileRoundTripStats st{};
                if (!RoundTripAnySC4Subfile(entry.tgi.type, sub, st)) {
                    ++bctUnknown;
                    continue;
                }
                bctStats.records += st.records;
                bctStats.decoded += st.decoded;
                bctStats.currentVersion += st.currentVersion;
                bctStats.byteExact += st.byteExact;
            }
            char bctAllDetail[192];
            std::snprintf(bctAllDetail, sizeof(bctAllDetail),
                          "(subfiles=%zu, types=%zu, records=%u/%u, byteExact=%u/%u)",
                          bctSubfiles, bctTypes.size(), bctStats.decoded, bctStats.records,
                          bctStats.byteExact, bctStats.currentVersion);
            Check(bctUnknown == 0u && bctSubfiles == 179u && bctTypes.size() == 133u &&
                      bctStats.records == bctStats.decoded &&
                      bctStats.currentVersion == bctStats.byteExact,
                  "Decoded & byte-exact roundtripped 100% of all 133 Type IDs (179 subfiles) in Big City Tutorial.sc4",
                  bctAllDetail);

            const std::string regionsDir = steamApp + "/Contents/GameData/Regions";
            std::size_t cityFiles = 0;
            std::size_t totalSubfiles = 0;
            std::size_t unknownSubfiles = 0;
            std::set<Uint32> allTypes;
            SC4SubfileRoundTripStats allStats{};
            if (fs::exists(regionsDir)) {
                for (const auto& de : fs::recursive_directory_iterator(regionsDir)) {
                    if (de.path().extension() != ".sc4") {
                        continue;
                    }
                    cDBPFArchive arc;
                    if (!arc.Open(de.path().string())) {
                        continue;
                    }
                    ++cityFiles;
                    for (const auto& entry : arc.Entries()) {
                        std::vector<Uint8> sub;
                        if (!arc.ReadEntry(entry, sub)) {
                            ++unknownSubfiles;
                            continue;
                        }
                        ++totalSubfiles;
                        allTypes.insert(entry.tgi.type);
                        SC4SubfileRoundTripStats st{};
                        if (!RoundTripAnySC4Subfile(entry.tgi.type, sub, st)) {
                            ++unknownSubfiles;
                            continue;
                        }
                        allStats.records += st.records;
                        allStats.decoded += st.decoded;
                        allStats.currentVersion += st.currentVersion;
                        allStats.byteExact += st.byteExact;
                    }
                }
            }
            char allCitiesDetail[220];
            std::snprintf(allCitiesDetail, sizeof(allCitiesDetail),
                          "(cities=%zu, subfiles=%zu, types=%zu, records=%u/%u, byteExact=%u/%u)",
                          cityFiles, totalSubfiles, allTypes.size(), allStats.decoded,
                          allStats.records, allStats.byteExact, allStats.currentVersion);
            Check(cityFiles == 417u && unknownSubfiles == 0u && allTypes.size() == 146u &&
                      allStats.records == allStats.decoded &&
                      allStats.currentVersion == allStats.byteExact,
                  "Decoded & byte-exact roundtripped 100% of all 146 Type IDs across all 417 shipped .sc4 cities",
                  allCitiesDetail);
        }

        // Verify Mach-O symbol-indexed decompilation harness against Sim City 4 Deluxe Editionsub.
        {
            const std::string machOPath =
                steamApp + "/Contents/MacOS/Sim City 4 Deluxe Editionsub";
            DecompilationCatalogReport catReport{};
            const bool catalogOk = BuildDecompilationCatalogReport(machOPath, catReport);
            std::printf("\n--- 4b. Mach-O Symbol-Indexed Decompilation Catalog (%s) ---\n",
                        "Sim City 4 Deluxe Editionsub");
            std::printf("  [Decompilation Pipeline Tiers]\n");
            for (std::size_t i = 0; i < kSC4DecompTierCount; ++i) {
                const TierProgress& tp = catReport.byTier[i];
                const double pct = tp.totalSymbols > 0
                    ? (100.0 * static_cast<double>(tp.decompiledSymbols) /
                       static_cast<double>(tp.totalSymbols))
                    : 0.0;
                std::printf("    %-64.*s %5u / %5u syms (%5.1f%%)  %7.1f / %7.1f KB\n",
                            static_cast<int>(SC4DecompTierName(tp.tier).size()),
                            SC4DecompTierName(tp.tier).data(),
                            tp.decompiledSymbols, tp.totalSymbols, pct,
                            static_cast<double>(tp.decompiledBytes) / 1024.0,
                            static_cast<double>(tp.totalBytes) / 1024.0);
            }
            std::printf("  [Target Subsystems]\n");
            for (std::size_t i = 0; i < kSC4TargetSubsystemCount; ++i) {
                const SubsystemProgress& sp = catReport.bySubsystem[i];
                const double pct = sp.totalSymbols > 0
                    ? (100.0 * static_cast<double>(sp.decompiledSymbols) /
                       static_cast<double>(sp.totalSymbols))
                    : 0.0;
                std::printf("    %-46.*s %5u / %5u syms (%5.1f%%)  %7.1f / %7.1f KB\n",
                            static_cast<int>(SC4SubsystemName(sp.subsystem).size()),
                            SC4SubsystemName(sp.subsystem).data(),
                            sp.decompiledSymbols, sp.totalSymbols, pct,
                            static_cast<double>(sp.decompiledBytes) / 1024.0,
                            static_cast<double>(sp.totalBytes) / 1024.0);
            }
            const TierProgress& tier3 =
                catReport.byTier[static_cast<std::size_t>(SC4DecompTier::Tier3_AutoVerifiedCOMAndLeafAccessors)];
            char catDetail[240];
            std::snprintf(catDetail, sizeof(catDetail),
                          "(textSyms=%u, targetSyms=%u, decompiled=%u [%.1f KB / %.1f KB], autoLeaves=%u, validAddrs=%u/%u)",
                          catReport.totalTextSymbols, catReport.targetEngineSymbols,
                          catReport.decompiledTargetSymbols,
                          static_cast<double>(catReport.decompiledTargetBytes) / 1024.0,
                          static_cast<double>(catReport.targetEngineBytes) / 1024.0,
                          catReport.autoVerifiedLeafSymbols,
                          catReport.registeredAddressesValid,
                          catReport.registeredAddressesChecked);
            Check(catalogOk &&
                      catReport.totalTextSymbols == 43333u &&
                      catReport.targetEngineSymbols > 30000u &&
                      catReport.registeredAddressesChecked >= 500u &&
                      catReport.registeredAddressesValid == catReport.registeredAddressesChecked &&
                      tier3.totalSymbols > 6000u &&
                      tier3.decompiledSymbols == tier3.totalSymbols &&
                      catReport.autoVerifiedLeafSymbols >= 3000u &&
                      catReport.decompiledTargetSymbols >= 7000u,
                  "Verified Mach-O LC_SYMTAB 4-Tier decompilation pipeline & x86_64 leaf auto-verifier",
                  catDetail);
        }
    }

    // ------------------------------------------------------------------------
    // 5. Verify OpenGFX Procedural City, Interactive Tools & Save Roundtrip
    // ------------------------------------------------------------------------
    std::printf("\n--- 5. OpenGFX Procedural Fallback, Tools & .sc4 Save Roundtrip ---\n");
    {
        cSC4City procCity;
        procCity.GenerateProceduralCity(128, 128, 0x12345678u);
        char proceduralDetail[192];
        std::snprintf(proceduralDetail, sizeof(proceduralDetail),
                      "(lots=%zu, buildings=%zu, networks=%zu, attempts=%u, executions=%u, resources=%.1f)",
                      procCity.Lots().size(), procCity.Buildings().size(),
                      procCity.Networks().size(),
                      procCity.BuildingDevelopment().growthAttempts,
                      procCity.BuildingDevelopment().growthExecutions,
                      procCity.BuildingDevelopment().constructionResources);
        Check(procCity.CellCountX() == 128 && !procCity.Buildings().empty() && !procCity.Networks().empty(),
              "Generated playable 128x128 OpenGFX procedural city",
              proceduralDetail);

        procCity.ApplyTool(ToolType::Bulldoze, 43, 21, 46, 24);
        std::size_t prevLots = procCity.Lots().size();
        std::size_t prevBlds = procCity.Buildings().size();
        procCity.ApplyTool(ToolType::ZoneComHigh, 43, 21, 46, 24);
        Check(procCity.Lots().size() > prevLots &&
              procCity.Buildings().size() == prevBlds,
              "ApplyTool(ZoneComHigh) creates empty zoned lots");
        Uint32 previousGrowthExecutions =
            procCity.BuildingDevelopment().growthExecutions;
        procCity.StepSimulation(1);
        Check(procCity.Buildings().size() == prevBlds + 1 &&
              procCity.BuildingDevelopment().growthExecutions ==
                  previousGrowthExecutions + 1,
              "Daily growth develops one road-access lot using weighted active demand");

        // Clear two patches, then construct exact road topologies for Maxis RUL/FSH selection.
        procCity.ApplyTool(ToolType::Bulldoze, 7, 7, 23, 23);
        procCity.ApplyTool(ToolType::BuildRoad, 10, 8, 10, 12);
        procCity.ApplyTool(ToolType::BuildRoad, 8, 10, 12, 10);
        procCity.ApplyTool(ToolType::BuildRoad, 20, 18, 20, 20);
        procCity.ApplyTool(ToolType::BuildRoad, 20, 20, 22, 20);
        auto roadAt = [&](int x, int z) -> const SC4NetworkRecord* {
            for (const auto& n : procCity.Networks()) {
                if (n.tileX == x && n.tileZ == z) return &n;
            }
            return nullptr;
        };
        const SC4NetworkRecord* straight = roadAt(10, 9);
        const SC4NetworkRecord* fourWay = roadAt(10, 10);
        const SC4NetworkRecord* corner = roadAt(20, 20);
        Check(straight && straight->textureId == 0x05004b00u,
              "Road auto-tiler selects shipped Maxis straight FSH 0x05004b00");
        Check(corner && corner->textureId == 0x00000a00u && corner->orientation == 0,
              "Road auto-tiler selects and orients shipped Maxis 90-degree curve FSH");
        Check(fourWay && fourWay->textureId == 0x05020700u,
              "Road auto-tiler selects shipped Maxis four-way intersection FSH 0x05020700");

        std::vector<Uint8> savedSc4;
        Check(procCity.SaveToSC4Buffer(savedSc4) && savedSc4.size() > 96, "SaveToSC4Buffer serialized valid DBPF .sc4 archive");

        cDBPFArchive reArc;
        Check(reArc.OpenFromMemory(savedSc4, "roundtrip.sc4"), "Re-opened serialized .sc4 DBPF archive in memory");
        cSC4City reCity;
        Check(reCity.LoadFromSC4Archive(reArc) && reCity.CellCountX() == 128, "Roundtrip .sc4 city loaded cleanly");

        cSC4City calendarCity;
        calendarCity.GenerateProceduralCity(64, 64, 0x12345678u);
        calendarCity.StepSimulation(59);
        char calendarDetail[128];
        std::snprintf(calendarDetail, sizeof(calendarDetail), "(got %04u-%02u-%02u, JDN %u)",
                      calendarCity.SimYear(), calendarCity.SimMonth(), calendarCity.SimDay(),
                      calendarCity.SimJulianDay());
        Check(calendarCity.SimYear() == 2000 && calendarCity.SimMonth() == 2 && calendarCity.SimDay() == 29,
              "cSC4Simulator calendar reaches leap day after 59 days from 2000-01-01", calendarDetail);
        calendarCity.StepSimulation(1);
        std::snprintf(calendarDetail, sizeof(calendarDetail), "(got %04u-%02u-%02u, JDN %u)",
                      calendarCity.SimYear(), calendarCity.SimMonth(), calendarCity.SimDay(),
                      calendarCity.SimJulianDay());
        Check(calendarCity.SimYear() == 2000 && calendarCity.SimMonth() == 3 && calendarCity.SimDay() == 1,
              "cSC4Simulator calendar advances leap day to March 1", calendarDetail);
        Check(calendarCity.SimulationCadence().dailyEvents == 60 &&
              calendarCity.SimulationCadence().weeklyEvents == 9 &&
              calendarCity.SimulationCadence().monthlyEvents == 2 &&
              calendarCity.SimulationCadence().yearlyEvents == 0,
              "Emitted exact daily/weekly/monthly/yearly cadence across leap-day boundary");
        Check(calendarCity.BuildingDevelopment().dailyCycles == 60 &&
              calendarCity.BuildingDevelopment().declinePlanCycles == 60 &&
              calendarCity.BuildingDevelopment().monthlyCycles == 2 &&
              calendarCity.BuildingDevelopment().constructionResources >= 100.0f &&
              calendarCity.BuildingDevelopment().constructionResources <= 2000.0f,
              "Building development daily/monthly cadence follows cSC4Simulator events");

        cSC4City yearCity;
        yearCity.GenerateProceduralCity(64, 64, 0x87654321u);
        yearCity.StepSimulation(366);
        Check(yearCity.SimYear() == 2001 && yearCity.SimMonth() == 1 && yearCity.SimDay() == 1 &&
              yearCity.SimulationCadence().yearlyEvents == 1,
              "cSC4Simulator calendar emits yearly event on 2001-01-01");

        // Phase 1-4 verification: PathFinder A*, VertexHeightSolver, ZoneParcellizer,
        // SC4 Free Functions (CRC24 resource ID, Money formatting), and Mayor Controls / Tile Query.
        {
            const SC4Rect<Sint32> startR{1, 1, 1, 1};
            const SC4Rect<Sint32> endR{4, 3, 4, 3};
            Check(cSC4PathFinderEngine::DistanceToRectangle(0, 0, startR) == 2 &&
                  cSC4PathFinderEngine::DistanceToRectangle(1, 1, startR) == 0,
                  "cSC4PathFinder::DistanceToRectangle (@ 0x1000d3b32) exact Manhattan distance");

            std::vector<Float32> costGrid(6 * 6, 0.0f);
            std::vector<Uint8> typeGrid(6 * 6, 1u);
            // L-shaped road from (1,1) -> (4,1) -> (4,3)
            for (int x = 1; x <= 4; ++x) costGrid[1 * 6 + x] = 1.5f;
            for (int z = 1; z <= 3; ++z) costGrid[z * 6 + 4] = 1.5f;
            SC4PathFindResult pfRes{};
            const bool foundPath = cSC4PathFinderEngine::FindPath(
                startR, endR, 6, 6, costGrid, typeGrid, 100.0f, pfRes);
            Check(foundPath && pfRes.cellX.size() == 6u &&
                  std::abs(pfRes.tripLength - 7.5f) < 1e-3f &&
                  !pfRes.compressedRoute.empty(),
                  "cSC4PathFinder::FindPath (@ 0x1000d2644) A* search & GetCompressedRoute (@ 0x1000d434e)");

            cSC4VertexHeightSolver htSolver;
            htSolver.InsertFixedVertex(0, 270.0f);
            htSolver.InsertEqualityConstraint(0, 1);
            htSolver.InsertRangeConstraint(2, 260.0f, 290.0f);
            htSolver.InsertSlopeConstraint(1, 2, 4.0f);
            const bool htSolved = htSolver.Solve();
            Float32 h1 = 0.0f, h2 = 0.0f;
            Check(htSolved && htSolver.GetHeight(1, h1) && htSolver.GetHeight(2, h2) &&
                  std::abs(h1 - 270.0f) < 0.05f && std::abs(h2 - h1) <= 4.05f,
                  "cSC4VertexHtConstraintSatisfier::Solve (@ 0x1001bdebe) equality/range/slope propagation");

            std::vector<ZoneParcel> parcels;
            cSC4ZoneParcellizer::SimpleParcellize(
                SC4Rect<Sint32>{10, 10, 16, 14}, 1u, kLotEdgeNorth, 4, parcels);
            Check(!parcels.empty() && parcels.front().facingEdge == kLotEdgeNorth &&
                  parcels.front().depth > 0,
                  "cSC4ZoneDeveloper::SimpleParcellize (@ 0x10012c320) & DetermineLotSize (@ 0x10012d606)");

            Check(SC4GetResourceIDFromString("123456789") == 0xFF21CF02u &&
                  SC4ZoomAndRotationToIndex(3, 2) == 14 &&
                  SC4MakeMoneyString(1035332) == "\xC2\xA7" "1,035,332" &&
                  SC4SanitizeFileName("My:City*Name?.sc4") == "MyCityName.sc4",
                  "SC4 Free Functions: SC4GetResourceIDFromString CRC24 (@ 0x10003a532) & SC4MakeMoneyString (@ 0x1000392b0)");

            const Sint64 fundsBeforeBond = yearCity.TreasuryFunds();
            const bool taxOk = yearCity.SetTaxRate(1, 1, 8.5f);
            const bool bondOk = yearCity.IssueBond(10000);
            const TileQueryReport qReport = yearCity.QueryTile(32, 32);
            Check(taxOk && std::abs(yearCity.GetTaxRate(1, 1) - 8.5f) < 1e-3f &&
                  bondOk && yearCity.TreasuryFunds() == fundsBeforeBond + 10000 &&
                  yearCity.GetTotalMonthlyBondPayments() > 0 &&
                  qReport.cellX == 32 && qReport.cellZ == 32,
                  "cSC4City live Mayor Controls (SetTaxRate, IssueBond, QueryTile) & simulator cadence");

            {
                cSC4City taxCity;
                const Sint64 seeded = taxCity.TreasuryFunds();
                taxCity.SetTaxRate(0, 1, 10.0f);
                taxCity.StepSimulation(45);
                Check(seeded > 0 && taxCity.TreasuryFunds() > seeded / 2,
                      "SetTaxRate seeds both budget fund fields, so the monthly step keeps the treasury",
                      ("seeded=" + std::to_string(seeded) +
                       " after=" + std::to_string(taxCity.TreasuryFunds())).c_str());
            }

            {
                cSC4City cutCity;
                const Uint32 poweredWithPlant = cutCity.PoweredCellCount();
                for (const auto& bld : cutCity.Buildings()) {
                    if (bld.name.find("Power") == std::string::npos) continue;
                    const int cx = static_cast<int>(bld.minX / 16.0f);
                    const int cz = static_cast<int>(bld.minZ / 16.0f);
                    cutCity.ApplyTool(ToolType::Bulldoze, cx, cz, cx, cz);
                    break;
                }
                Check(poweredWithPlant > 0 && cutCity.PoweredCellCount() == 0u,
                      "Bulldozing the only power plant cuts power to the whole grid",
                      ("before=" + std::to_string(poweredWithPlant) +
                       " after=" + std::to_string(cutCity.PoweredCellCount())).c_str());
            }

            // Milestone A: Textured S3D + FSH GPU batching in cSC4GLCityRenderer
            cSC4GLCityRenderer headlessRenderer;
            headlessRenderer.AttachDBPFSystem(&dbpfSys);
            headlessRenderer.RebuildCityGeometry(procCity, DataOverlay::None);
            const std::size_t texVerts =
                headlessRenderer.BuildTexturedBatchesFromDBPF(procCity, dbpfSys);
            char rendererDetail[160];
            std::snprintf(rendererDetail, sizeof(rendererDetail),
                          "(untexturedVerts=%zu, texturedBatches=%zu, texturedVerts=%zu)",
                          headlessRenderer.TotalVertexCount(),
                          headlessRenderer.TexturedBatches().size(),
                          texVerts);
            Check(headlessRenderer.TotalVertexCount() > 0 &&
                  !headlessRenderer.TexturedBatches().empty() &&
                  texVerts > 0,
                  "cSC4GLCityRenderer S3D/FSH GPU texture batch builder",
                  rendererDetail);

            // Milestone B & C: Live Power/Water BFS & A* Commute Routing
            char commuteDetail[160];
            std::snprintf(commuteDetail, sizeof(commuteDetail),
                          "(poweredCells=%u, wateredCells=%u, commuteRoutes=%zu)",
                          procCity.PoweredCellCount(),
                          procCity.WateredCellCount(),
                          procCity.ActiveCommuteRoutes().size());
            Check(procCity.PoweredCellCount() > 0 &&
                  procCity.WateredCellCount() > 0 &&
                  !procCity.ActiveCommuteRoutes().empty(),
                  "cSC4City live Power/Water BFS & cSC4PathFinderEngine A* commute routes",
                  commuteDetail);

            // Milestone D: Ordinance toggles & Disaster spawner
            procCity.SetOrdinanceOn(0xa2bf1e43u, true);
            const bool disOk = procCity.TriggerDisaster(1u, 20, 20);
            Check(procCity.IsOrdinanceOn(0xa2bf1e43u) &&
                  disOk && procCity.ActiveDisasterCount() == 1u,
                  "cSC4City live Ordinance toggle & Disaster spawner");
        }
    }

    // Each wrapper is one suite result; internal assertion/case counts are
    // separately printed and must not be added to the suite total.
    Check(RunWeatherPositionToCellChecks() == 0,
          "Weather PositionToCell boundary checks @ 0x1000e487a");
    Check(RunWeatherCellToPositionChecks() == 0,
          "Weather CellToPosition boundary checks @ 0x1000e4896");
    Check(weather_coordinates::RunWeatherCellToStandardCityCellChecks() == 0,
          "Weather CellToStandardCityCell boundary checks @ 0x1000e48ba");
    Check(RunWeatherStandardCityCellToCellChecks() == 0,
          "Weather StandardCityCellToCell boundary checks @ 0x1000e48d0");

    std::printf("\n================================================================\n");

    std::printf("\n--- cSTETerrain runtime foundation (M3) ---\n");
    {
        cSTETerrain terrain;
        Check(!terrain.IsInitialized(), "cSTETerrain default-constructed uninitialized @ 0x100740e24");
        Check(terrain.Init(4, 4), "cSTETerrain::Init(4,4) @ 0x100739cb0");
        Check(terrain.IsInitialized(), "cSTETerrain initialized after Init @ 0x100740e24");
        // Init field layout: cellCount 4 -> vertexCount 5, maxVertexIndex 3.
        Check(terrain.CellCountX() == 4 && terrain.CellCountZ() == 4,
              "cSTETerrain::Init cell counts +0x40/+0x44 @ 0x100739d35/0x100739d48");

        // 5x5 vertex grid, indexed by vidx(x,z) = z*5 + x (stride = vertexCountX).
        auto vidx = [](Sint32 x, Sint32 z) { return z * 5 + x; };

        // ---- CellHasWater @ 0x10073562c (four-corner strictly-water test) ----
        // Cell (1,1) corners: (1,1)=6, (2,1)=7, (1,2)=11, (2,2)=12.
        terrain.SetWaterDepth(vidx(1, 1), 1.0f);
        terrain.SetWaterDepth(vidx(2, 1), 2.0f);
        terrain.SetWaterDepth(vidx(1, 2), 3.0f);
        terrain.SetWaterDepth(vidx(2, 2), 4.0f);
        Check(terrain.CellHasWater(1u, 1u),
              "CellHasWater all-four-corners-water @ 0x10073562c");
        terrain.SetWaterDepth(vidx(1, 1), 0.0f); // one corner dry, rest wet
        Check(terrain.CellHasWater(1u, 1u),
              "CellHasWater any-corner-water @ 0x10073562c");
        // make every corner of cell (1,1) <= 0 -> false
        terrain.SetWaterDepth(vidx(2, 1), 0.0f);
        terrain.SetWaterDepth(vidx(1, 2), 0.0f);
        terrain.SetWaterDepth(vidx(2, 2), 0.0f);
        Check(!terrain.CellHasWater(1u, 1u),
              "CellHasWater all-corners-dry -> false @ 0x10073562c");
        Check(!terrain.CellHasWater(4u, 0u) && !terrain.CellHasWater(0u, 4u) &&
              !terrain.CellHasWater(4u, 4u),
              "CellHasWater out-of-range -> false @ 0x10073562c");

        // ---- CellHasLand @ 0x1007356a2 (depth-grid semantics, corrected Nov 2026) ----
        // Same water-depth grid as CellHasWater ([+0x38]->[+0x28]): land iff
        // ANY corner depth <= 0 (jae = NOT depth > 0). Discriminates the
        // corrected depth-grid semantics from the earlier altitude/seaLevel
        // misreconstruction, including the NaN corner asymmetry.
        Check(terrain.CellHasLand(1u, 1u),
              "CellHasLand all-corners-dry -> true @ 0x1007356a2");
        terrain.SetWaterDepth(vidx(1, 1), 1.0f);
        terrain.SetWaterDepth(vidx(2, 1), 2.0f);
        terrain.SetWaterDepth(vidx(1, 2), 3.0f);
        terrain.SetWaterDepth(vidx(2, 2), 4.0f);
        Check(!terrain.CellHasLand(1u, 1u),
              "CellHasLand all-corners-wet -> false @ 0x1007356a2");
        terrain.SetWaterDepth(vidx(1, 1), 0.0f); // mixed corners: shoreline cell
        Check(terrain.CellHasLand(1u, 1u) && terrain.CellHasWater(1u, 1u),
              "CellHasLand mixed shoreline cell is both land and water @ 0x1007356a2");
        terrain.SetWaterDepth(vidx(1, 1),
                              std::numeric_limits<Float32>::quiet_NaN());
        Check(terrain.CellHasLand(1u, 1u),
              "CellHasLand NaN-depth corner -> land (unordered jae) @ 0x1007356a2");
        // All four corners NaN: still not water (ja requires ordered >).
        terrain.SetWaterDepth(vidx(2, 1),
                              std::numeric_limits<Float32>::quiet_NaN());
        terrain.SetWaterDepth(vidx(1, 2),
                              std::numeric_limits<Float32>::quiet_NaN());
        terrain.SetWaterDepth(vidx(2, 2),
                              std::numeric_limits<Float32>::quiet_NaN());
        Check(!terrain.CellHasWater(1u, 1u),
              "CellHasWater NaN-depth corners -> not water (ordered ja) @ 0x10073562c");
        Check(!terrain.CellHasLand(4u, 0u) && !terrain.CellHasLand(0u, 4u) &&
              !terrain.CellHasLand(4u, 4u),
              "CellHasLand out-of-range -> false @ 0x1007356a2");
        // Restore the all-dry fixture state for later sections.
        terrain.SetWaterDepth(vidx(1, 1), 0.0f);

        // ---- GetAltitude(float,float) @ 0x100738928 (planar field) ----
        // Altitude plane: alt(x,z) = 3*x + 5*z (vertex units), so a world query
        // returns 3*(wx/16) + 5*(wz/16) exactly.
        for (Sint32 z = 0; z < 5; ++z) {
            for (Sint32 x = 0; x < 5; ++x) {
                terrain.SetAltitude(vidx(x, z), 3.0f * static_cast<Float32>(x) +
                                                    5.0f * static_cast<Float32>(z));
            }
        }
        Check(terrain.GetAltitude(0.0f, 0.0f) == 0.0f,
              "GetAltitude corner (0,0) == 0 @ 0x100738928");
        Check(terrain.GetAltitude(16.0f, 0.0f) == 3.0f,
              "GetAltitude vertex (16,0) == 3 @ 0x100738928");
        Check(terrain.GetAltitude(0.0f, 16.0f) == 5.0f,
              "GetAltitude vertex (0,16) == 5 @ 0x100738928");
        Check(terrain.GetAltitude(8.0f, 8.0f) == 4.0f,
              "GetAltitude midpoint (8,8) == 4 @ 0x100738928");
        Check(terrain.GetAltitude(8.0f, 0.0f) == 1.5f,
              "GetAltitude edge midpoint (8,0) == 1.5 @ 0x100738928");
        Check(terrain.GetAltitude(0.0f, 8.0f) == 2.5f,
              "GetAltitude edge midpoint (0,8) == 2.5 @ 0x100738928");
        Check(terrain.GetAltitude(24.0f, 24.0f) == 12.0f,
              "GetAltitude interior cell (24,24) == 12 @ 0x100738928");

        // GetAltitude entry and signed-zero discriminators @ 0x100738928.
        // The old core returned +0 for the negative slope, -0 for the
        // positive slope, and interpolated negative/outside coordinates.
        {
            cSTETerrain edgeTerrain;
            Check(std::bit_cast<Uint32>(edgeTerrain.GetAltitude(0.0f, 0.0f)) == 0u,
                  "GetAltitude absent model storage returns +0 (clean-room fallback, not authentic)");
            edgeTerrain.Init(1, 1);
            edgeTerrain.SetAltitude(0, -0.0f);
            edgeTerrain.SetAltitude(2, -1.0f);
            edgeTerrain.SetAltitude(3, +0.0f);
            Check(std::bit_cast<Uint32>(edgeTerrain.GetAltitude(-0.0f, +0.0f)) == 0x80000000u,
                  "GetAltitude signed-zero max tie selects fracZ (cmpnless imm06) @ 0x100738a41");
            edgeTerrain.SetAltitude(2, +1.0f);
            Check(std::bit_cast<Uint32>(edgeTerrain.GetAltitude(-0.0f, +0.0f)) == 0u,
                  "GetAltitude signed-zero min tie selects fracX (jb not taken) @ 0x100738a52");
            edgeTerrain.SetAltitude(0, 0.0f);
            edgeTerrain.SetAltitude(1, 8.0f);
            edgeTerrain.SetAltitude(2, 16.0f);
            edgeTerrain.SetAltitude(3, 40.0f);
            Check(edgeTerrain.GetAltitude(4.0f, 12.0f) == 18.0f &&
                      edgeTerrain.GetAltitude(12.0f, 4.0f) == 14.0f &&
                      edgeTerrain.GetAltitude(8.0f, 8.0f) == 20.0f &&
                      edgeTerrain.GetAltitude(16.0f, 16.0f) == 40.0f,
                  "GetAltitude nonplanar cell exercises both diagonals, equality and final-cell clamp @ 0x100738a34");
            for (Sint32 i = 0; i < 4; ++i) edgeTerrain.SetAltitude(i, 10.0f);
            Check(edgeTerrain.GetAltitude(0.0f, 0.0f) == 10.0f &&
                      edgeTerrain.GetAltitude(-0.0f, -0.0f) == 10.0f &&
                      edgeTerrain.GetAltitude(16.0f, 0.0f) == 10.0f &&
                      edgeTerrain.GetAltitude(0.0f, 16.0f) == 10.0f &&
                      edgeTerrain.GetAltitude(16.0f, 16.0f) == 10.0f &&
                      edgeTerrain.GetAltitude(std::nextafter(16.0f, 0.0f), 8.0f) == 10.0f,
                  "GetAltitude inclusive extent and just-inside rounding crossings @ 0x100740e7a");
            const Float32 justOutside = std::nextafter(16.0f, std::numeric_limits<Float32>::infinity());
            const Float32 belowZero = std::nextafter(0.0f, -1.0f);
            bool rejected = true;
            const Float32 outside[][2] = {{-1.0f, 0.0f}, {0.0f, -1.0f},
                {belowZero, 0.0f}, {0.0f, belowZero},
                {justOutside, 8.0f}, {8.0f, justOutside}};
            for (const auto& point : outside) {
                rejected = rejected && std::bit_cast<Uint32>(
                    edgeTerrain.GetAltitude(point[0], point[1])) == 0u;
            }
            Check(rejected,
                  "GetAltitude negative and nextafter-outside coordinates return +0 @ 0x100740e7a");
            const Float32 nan = std::numeric_limits<Float32>::quiet_NaN();
            const Float32 inf = std::numeric_limits<Float32>::infinity();
            rejected = true;
            const Float32 nonfinite[][2] = {{nan, 0.0f}, {0.0f, nan}, {nan, nan},
                {inf, 0.0f}, {0.0f, inf}, {-inf, 0.0f}, {0.0f, -inf}};
            for (const auto& point : nonfinite) {
                rejected = rejected && std::bit_cast<Uint32>(
                    edgeTerrain.GetAltitude(point[0], point[1])) == 0u;
            }
            Check(rejected,
                  "GetAltitude NaN and infinity coordinates rejected before indexing @ 0x100740e7a");
            cSTETerrain rectangular;
            rectangular.Init(2, 1);
            for (Sint32 i = 0; i < 6; ++i) rectangular.SetAltitude(i, 7.0f);
            Check(rectangular.GetAltitude(32.0f, 16.0f) == 7.0f &&
                      std::bit_cast<Uint32>(rectangular.GetAltitude(16.0f, 32.0f)) == 0u,
                  "GetAltitude distinct X/Z extents preserve rectangular bounds @ 0x100740e7a");
        }

        // ---- GetAltitude(int) @ 0x100738a84 / GetAltitudeAtVertex / WaterDepth ----
        Check(terrain.GetAltitude(vidx(2, 2)) == 16.0f,
              "GetAltitude(vertex 12) == 16 @ 0x100738a84");
        Check(terrain.GetAltitude(25) == 0.0f && terrain.GetAltitude(-1) == 0.0f,
              "GetAltitude out-of-range vertex -> 0 @ 0x100738a84");
        Check(terrain.GetAltitudeAtVertex(2, 2) == 16.0f,
              "GetAltitudeAtVertex(2,2) == 16 @ 0x100738aa6");

        // ---- BrushFnRain @ 0x10073083e ----
        TerrainSurfaceWaterState& sw = terrain.SurfaceWater();
        sw.currentVertex = 3;
        terrain.SetWaterDepth(3, 5.0f);
        TerrainBrushContext ctx;
        TerrainBrushComponent comp;
        ctx.flag = 1;
        ctx.amount = 20.0f;
        comp.flag48 = 0; // differs from ctx.flag -> rain applied
        ctx.target = &comp;
        const bool rainApplied = terrain.BrushFnRain(0, ctx);
        Check(!rainApplied,
              "BrushFnRain returns false even when applied @ 0x10073083e");
        Check(terrain.GetWaterDepthAtVertex(3) == 7.0f,
              "BrushFnRain adds amount/10 (20/10=2) to depth @ 0x10073083e");
        comp.flag48 = 1; // matches ctx.flag -> no-op
        const Float32 before = terrain.GetWaterDepthAtVertex(3);
        terrain.BrushFnRain(0, ctx);
        Check(terrain.GetWaterDepthAtVertex(3) == before,
              "BrushFnRain no-op when flag matches target @ 0x10073083e");

        // ---- BrushFnVolcanoAsh @ 0x100733338 (0x400 flags-word bit-set) ----
        for (Sint32 i = 0; i < 25; ++i) {
            terrain.SetAltitude(i, 0.0f);
            terrain.SetWaterDepth(i, 0.0f);
        }
        ctx.flag = 1;
        ctx.amount = 1.0f;
        ctx.targetHeight = 0;
        Check(terrain.ApplyBrushEffect(TerrainBrushEffect::VolcanoAsh, 12, ctx),
              "BrushFnVolcanoAsh applies @ 0x100733338");
        Check((terrain.VertexFlags(12) & 0x400) != 0,
              "BrushFnVolcanoAsh sets 0x400 bit in flags word @ 0x100733338");
        ctx.flag = 0;
        Check(!terrain.ApplyBrushEffect(TerrainBrushEffect::VolcanoAsh, 12, ctx),
              "BrushFnVolcanoAsh zero flag -> false @ 0x100733338");
        ctx.flag = 1;

        // ---- BrushFnLevel @ 0x100733230 (rate-limited move toward targetHeight) ----
        // Prior "two-height accumulation" model was a clean-room guess; the
        // disassembly shows a rate-limited move: rate = flag*amount*0.00390625f
        // (const @ 0x100a1b65c), diff = altitude - ctx.targetHeight (+0x1c),
        // snap via vtable+0x188 when |diff| <= rate, otherwise move by rate.
        terrain.SetAltitude(12, 10.0f);
        terrain.SetWaterDepth(12, -10.0f);
        ctx.amount = 5.0f;
        ctx.targetHeight = 7.0f;
        Check(terrain.ApplyBrushEffect(TerrainBrushEffect::Level, 12, ctx),
              "BrushFnLevel applies @ 0x100733230");
        // rate = 1*5/256 = 0.01953125; diff = 10-7 = 3 > 0; rate < diff -> lower
        Check(terrain.GetAltitude(12) == 10.0f - 0.01953125f,
              "BrushFnLevel lowers by flag*amount/256 toward target @ 0x100733230");
        Check(terrain.GetWaterDepthAtVertex(12) == -(10.0f - 0.01953125f),
              "BrushFnLevel water depth updated on lower @ 0x100733230");
        // Snap when within rate: alt 7.5, target 7, rate = 1.0 -> diff 0.5 <= rate
        terrain.SetAltitude(12, 7.5f);
        ctx.amount = 256.0f; // rate = 256 * 0.00390625 = 1.0
        terrain.ApplyBrushEffect(TerrainBrushEffect::Level, 12, ctx);
        // vtable+0x188 is the exact cSTETerrain::SetAltitude @ 0x10073e192:
        // on dry land (oldDepth <= 0 and seaLevel <= newAlt) the depth is
        // left UNCHANGED (ja/jbe gates @ 0x10073e1d2..0x10073e1e2) — the
        // depth keeps the -(10 - 0.01953125) value from the LowerVertex
        // step; the old clean-room model wrote seaLevel - altitude.
        Check(terrain.GetAltitude(12) == 7.0f &&
                  terrain.GetWaterDepthAtVertex(12) == -(10.0f - 0.01953125f),
              "BrushFnLevel snaps to target via vtable+0x188 (exact SetAltitude leaves dry-land depth unchanged) @ 0x100733230/0x10073e192");
        // diff == 0 -> false @ 0x10073327f..0x10073329a
        Check(!terrain.ApplyBrushEffect(TerrainBrushEffect::Level, 12, ctx),
              "BrushFnLevel zero diff -> false @ 0x100733230");

        // ---- BrushFnLakeBed @ 0x100733360 (target + delta, water-gated) ----
        // Prior "multiply-then-add" model was a clean-room guess; the
        // disassembly shows: delta = (flag - flag48) * raw component strength,
        // target = ctx.targetHeight + delta, rate = |delta|/16, raising a
        // submerged vertex (depth > 0) refused, snap via vtable+0x188.
        terrain.SetAltitude(12, 10.0f);
        terrain.SetWaterDepth(12, 0.0f); // not submerged
        comp.flag48 = 0;
        comp.strengths = {16.0f};
        ctx.flag = 1;
        ctx.strengthIndex = 0;
        ctx.targetHeight = 7.0f;
        // delta = (1-0)*16 = 16; target = 7+16 = 23; rate = 16/16 = 1; diff = 23-10 = 13
        Check(terrain.ApplyBrushEffect(TerrainBrushEffect::LakeBed, 12, ctx),
              "BrushFnLakeBed applies @ 0x100733360");
        Check(terrain.GetAltitude(12) == 11.0f,
              "BrushFnLakeBed raises by |delta|/16 toward target @ 0x100733360");
        Check(terrain.GetWaterDepthAtVertex(12) == -11.0f,
              "BrushFnLakeBed water depth updated @ 0x100733360");
        // Submerged vertex (depth > 0) with diff > 0 -> refused @ 0x1007333d2
        terrain.SetAltitude(12, 10.0f);
        terrain.SetWaterDepth(12, 1.0f);
        Check(!terrain.ApplyBrushEffect(TerrainBrushEffect::LakeBed, 12, ctx),
              "BrushFnLakeBed refuses to raise a submerged vertex @ 0x100733360");
        terrain.SetWaterDepth(12, 0.0f);
        // delta == 0 (flag == flag48) -> false @ 0x10073338c
        comp.flag48 = 1;
        Check(!terrain.ApplyBrushEffect(TerrainBrushEffect::LakeBed, 12, ctx),
              "BrushFnLakeBed zero delta -> false @ 0x100733360");
        comp.flag48 = 0;

        // ---- cSTESurfaceWater::CreateSpring @ 0x100730b16 (spring accumulator) ----
        // Prior amount-based model was a clean-room guess; the disassembly shows
        // CreateSpring(int vertex) adds 1.0f (const @ 0x100a15090) to the vertex
        // record's spring accumulator (depth-array record +4) per call.
        for (Sint32 i = 0; i < 25; ++i) terrain.SetWaterDepth(i, 0.0f);
        terrain.SurfaceWater().depth.assign(25, 0.0f);
        terrain.SurfaceWater().spring.assign(25, 0.0f);
        Check(terrain.ApplyBrushEffect(TerrainBrushEffect::Spring, 12, ctx),
              "ApplyBrushEffect Spring delegates to CreateSpring @ 0x100730b16");
        terrain.ApplyBrushEffect(TerrainBrushEffect::Spring, 12, ctx);
        Check(terrain.SurfaceWater().spring[12] == 2.0f,
              "CreateSpring adds 1.0f to the spring grid per call @ 0x100730b16");
        Check(terrain.GetWaterDepthAtVertex(12) == 0.0f,
              "CreateSpring leaves the water depth grid untouched @ 0x100730b16");

        // ---- M4 evolution-oracle: pristine-vs-fresh subfile diff (Konradshohe pair) ----
        // The D state is the pristine bundle original (1,520,556 B); the D+N
        // state is the fresh original-game save (1,398,345 B, saved 17:00 Nov 11).
        // Both load through the OpenSC4 DBPF reader (reverse-interop, free).
        // The 138-changed-type inventory is the deterministic re-simulation
        // target list for the L4 evolution-parity oracle.
        {
            cDBPFArchive pristinePair;
            cDBPFArchive freshPair;
            const std::string pristinePath =
                "/Users/russellmartin/Library/Application Support/Steam/steamapps/common/SimCity 4 Deluxe/"
                "Sim City 4 Deluxe Edition.app/Contents/GameData/Regions/Berlin/City - Konradshohe.sc4";
            const std::string freshPath =
                "/Users/russellmartin/Documents/SimCity 4/Regions/Berlin/City - Konradshohe.sc4";
            if (pristinePair.Open(pristinePath) && freshPair.Open(freshPath)) {
                std::size_t pristineRecords = 0, freshRecords = 0;
                for (const auto& e : pristinePair.Entries()) ++pristineRecords;
                for (const auto& e : freshPair.Entries()) ++freshRecords;
                Check(pristineRecords > 0 && freshRecords > 0,
                      "M4 pair-diff: both Konradshohe archives parse via the OpenSC4 reader @ 0x10036a4ac");
                std::printf("M4 pair-diff inventory: pristine=%zu records, fresh=%zu records\n",
                            pristineRecords, freshRecords);
            } else {
                std::printf("M4 pair-diff: fresh Konradshohe save absent (user-side path) — skipped\n");
            }
        }

        // ---- Oracle checkpoint: live game-written save via the RPC oracle ----
        // Konradshohe_JDN2461688.sc4 was written BY THE RUNNING GAME through
        // the oracle's saveas command (SaveCityForObliteration @ 0x1000475cc)
        // while the simulation was live at JDN 2461688. If our DBPF reader
        // parses it, the round oracle loop closes: game-written saves feed
        // straight back into the authentic tooling (L3 interop for arbitrary
        // evolved states, not just hand-modified ones).
        {
            cDBPFArchive oracleSave;
            const std::string oraclePath =
                std::string(getenv("HOME") ? getenv("HOME") : "") +
                "/.opensc4-oracle/checkpoints/Konradshohe_JDN2461688.sc4";
            if (oracleSave.Open(oraclePath)) {
                std::size_t oracleRecords = 0;
                for (const auto& e : oracleSave.Entries()) ++oracleRecords;
                Check(oracleRecords > 0,
                      "Oracle checkpoint: live game-written save parses via the OpenSC4 reader");
                std::printf("Oracle checkpoint: %zu records, size %s\n",
                            oracleRecords, oraclePath.c_str());
            } else {
                std::printf("Oracle checkpoint: absent or unreadable — skipped\n");
            }
        }

        // Rain is the one implemented branch; the rest are cited TODO stubs -> false.
        Check(terrain.ApplyBrushEffect(TerrainBrushEffect::Paint, 0, ctx) == false,
              "ApplyBrushEffect Paint stub returns false @ 0x100732d22 (TODO)");

        // ---- CellHasLand @ 0x1007356a2 (depth-grid semantics, corrected Nov 2026) ----
        // Rewritten from the altitude/seaLevel misreconstruction: the binary
        // tests the SAME water-depth grid as CellHasWater, `jae` per corner.
        terrain.SetSeaLevelForTest(0.0f);
        // All four corners of cell (1,1) wet -> not land.
        terrain.SetWaterDepth(vidx(1, 1), 1.0f);
        terrain.SetWaterDepth(vidx(2, 1), 2.0f);
        terrain.SetWaterDepth(vidx(1, 2), 3.0f);
        terrain.SetWaterDepth(vidx(2, 2), 4.0f);
        Check(!terrain.CellHasLand(1u, 1u),
              "CellHasLand all-corners-wet -> false @ 0x1007356a2");
        // One dry corner -> land (any corner depth <= 0).
        terrain.SetWaterDepth(vidx(1, 1), 0.0f);
        Check(terrain.CellHasLand(1u, 1u),
              "CellHasLand any-corner-dry -> true @ 0x1007356a2");
        // Altitudes and seaLevel do NOT feed CellHasLand: push all corners
        // above/below seaLevel with depths unchanged -> result follows depths
        // only (discriminates the old altitude/seaLevel misreconstruction).
        for (Sint32 z = 0; z < 5; ++z) {
            for (Sint32 x = 0; x < 5; ++x) {
                terrain.SetAltitude(vidx(x, z), 3.0f * static_cast<Float32>(x) +
                                                    5.0f * static_cast<Float32>(z));
            }
        }
        terrain.SetSeaLevelForTest(10.0f);
        Check(terrain.CellHasLand(1u, 1u),
              "CellHasLand ignores altitudes and seaLevel (depth grid only) @ 0x1007356a2");
        // Out-of-range (same bounds as CellHasWater: cellCountX/Z <= x/z -> false)
        Check(!terrain.CellHasLand(4u, 0u) && !terrain.CellHasLand(0u, 4u) &&
              !terrain.CellHasLand(4u, 4u),
              "CellHasLand out-of-range -> false @ 0x1007356a2");

        // ---- GetWaterLevel(float,float) @ 0x100739a68 (bilinear water depth) ----
        // Set up water depth plane: depth(x,z) = 2*x + 3*z (vertex units)
        terrain.SetSeaLevelForTest(0.0f);
        for (Sint32 z = 0; z < 5; ++z) {
            for (Sint32 x = 0; x < 5; ++x) {
                terrain.SetWaterDepth(vidx(x, z), 2.0f * static_cast<Float32>(x) +
                                                        3.0f * static_cast<Float32>(z));
            }
        }
        // Vertex corners exactly (deterministic altitude fixture for the
        // single-vertex-sum semantics: altitude 0 + plane depth at the vertex)
        terrain.SetAltitude(vidx(1, 0), 0.0f);
        Check(terrain.GetWaterLevel(16.0f, 0.0f) == 2.0f,
              "GetWaterLevel vertex (16,0) == altitude+depth at vertex (1,0) == 2 @ 0x100739a68");
        terrain.SetAltitude(vidx(0, 1), 0.0f);
        Check(terrain.GetWaterLevel(0.0f, 16.0f) == 3.0f,
              "GetWaterLevel vertex (0,16) == altitude+depth at vertex (0,1) == 3 @ 0x100739a68");
        // Midpoints: the actual semantics @ 0x100739a68 are altitude[vertex] +
        // waterDepth[vertex] at the SINGLE magic-round vertex index (not bilinear
        // — corrected from the disassembly: shl/lea index*36, movss altitude,
        // addss depth at the same record). (8,0)/(0,8)/(8,8) round to vertex
        // (0,0): 0+0=0; (24,24) rounds to vertex (1,1): 8+5=13.
        Check(terrain.GetWaterLevel(8.0f, 0.0f) == 0.0f,
              "GetWaterLevel edge midpoint (8,0) == altitude+depth at vertex (0,0) == 0 @ 0x100739a68");
        Check(terrain.GetWaterLevel(0.0f, 8.0f) == 0.0f,
              "GetWaterLevel edge midpoint (0,8) == altitude+depth at vertex (0,0) == 0 @ 0x100739a68");
        Check(terrain.GetWaterLevel(8.0f, 8.0f) == 0.0f,
              "GetWaterLevel cell center (8,8) == altitude+depth at vertex (0,0) == 0 @ 0x100739a68");
        Check(terrain.GetWaterLevel(24.0f, 24.0f) == 13.0f,
              "GetWaterLevel interior cell (24,24) == altitude+depth at vertex (1,1) == 13 @ 0x100739a68");

        // ---- BrushFnRaiseLower @ 0x1007329cc (raise/lower dispatch) ----
        // Reset terrain to flat alt=10, seaLevel=0, vertexCount=25
        for (Sint32 i = 0; i < 25; ++i) {
            terrain.SetAltitude(i, 10.0f);
            terrain.SetWaterDepth(i, -10.0f); // seaLevel - altitude
        }
        TerrainBrushContext rlCtx;
        TerrainBrushComponent rlComp;
        rlCtx.flag = 2;
        rlCtx.amount = 5.0f;
        rlComp.flag48 = 1; // targetFlag < ctxFlag -> raise by amount * ctxFlag = 5 * 2 = 10
        rlCtx.target = &rlComp;
        Check(terrain.ApplyBrushEffect(TerrainBrushEffect::RaiseLower, 12, rlCtx),
              "BrushFnRaiseLower raises when ctxFlag > targetFlag @ 0x1007329cc");
        Check(terrain.GetAltitude(12) == 20.0f,
              "BrushFnRaiseLower amount = amount * ctxFlag (5*2=10) @ 0x1007329cc");
        Check(terrain.GetWaterDepthAtVertex(12) == -20.0f,
              "BrushFnRaiseLower updates water depth = seaLevel - newAlt @ 0x1007329cc");

        // Lower test: ctxFlag < targetFlag
        rlCtx.flag = 1;
        rlCtx.amount = 3.0f;
        rlComp.flag48 = 3; // targetFlag > ctxFlag -> lower by amount * targetFlag = 3 * 3 = 9
        Check(terrain.ApplyBrushEffect(TerrainBrushEffect::RaiseLower, 13, rlCtx),
              "BrushFnRaiseLower lowers when ctxFlag < targetFlag @ 0x1007329cc");
        // 10 - 9 = 1, clamped to the shipped kMinTerrainHeight 5.0f
        // (@ 0x100b17640; runtime-overridden from the terrain tuning exemplar
        // @ 0x10073da07, property 0x48cd7b27 — modeled as the settable
        // tunable). The old -2048 expectation asserted the unfaithful model.
        Check(terrain.GetAltitude(13) == 5.0f,
              "BrushFnRaiseLower lower clamps to kMinTerrainHeight (shipped 5.0f) @ 0x100734c7c/0x100b17640");
        Check(terrain.GetWaterDepthAtVertex(13) == -5.0f,
              "BrushFnRaiseLower updates water depth on lower (seaLevel 0 - 5) @ 0x100734c7c");

        // Equal flags -> no-op
        rlCtx.flag = 2;
        rlComp.flag48 = 2;
        const Float32 beforeRaise = terrain.GetAltitude(14);
        terrain.ApplyBrushEffect(TerrainBrushEffect::RaiseLower, 14, rlCtx);
        Check(terrain.GetAltitude(14) == beforeRaise,
              "BrushFnRaiseLower no-op when flags equal @ 0x1007329cc");

        // Clamp tests: kMaxTerrainHeight @ 0x100b1763c ships 200.0f and
        // kMinTerrainHeight @ 0x100b17640 ships 5.0f (fresh byte dumps,
        // nm-confirmed symbols); both are runtime-overridden from the
        // terrain tuning exemplar (@ 0x10073d9e4/0x10073da07, property ids
        // 0x48cd7b26/0x48cd7b27) and modeled as settable tunables. Fixture
        // sea level is 0.0f, so clamped depths are -200.0f / -5.0f.
        terrain.SetAltitude(15, 190.0f);
        terrain.SetWaterDepth(15, -190.0f);
        rlCtx.flag = 2;
        rlCtx.amount = 100.0f; // raise by 200 -> 390, clamps at 200
        rlComp.flag48 = 1;
        terrain.ApplyBrushEffect(TerrainBrushEffect::RaiseLower, 15, rlCtx);
        Check(terrain.GetAltitude(15) == 200.0f,
              "BrushFnRaiseLower clamps to kMaxTerrainHeight (shipped 200.0f) @ 0x100734cde/0x100b1763c");
        Check(terrain.GetWaterDepthAtVertex(15) == -200.0f,
              "BrushFnRaiseLower water depth clamped @ 0x100734cde");

        terrain.SetAltitude(16, 100.0f);
        terrain.SetWaterDepth(16, -100.0f);
        rlCtx.flag = 1;
        rlCtx.amount = 100.0f;
        rlComp.flag48 = 3; // lower by 300 -> -200, clamps at 5
        terrain.ApplyBrushEffect(TerrainBrushEffect::RaiseLower, 16, rlCtx);
        Check(terrain.GetAltitude(16) == 5.0f,
              "BrushFnRaiseLower clamps to kMinTerrainHeight (shipped 5.0f) @ 0x100734c7c/0x100b17640");
        Check(terrain.GetWaterDepthAtVertex(16) == -5.0f,
              "BrushFnRaiseLower water depth clamped on lower @ 0x100734c7c");

        // Settable-tunable semantics (documents the exemplar override wiring
        // @ 0x10073d9e4/0x10073da07): the clamp tracks the tunable, then the
        // shipped default is restored.
        cSTETerrain::MaxTerrainHeightTunable() = 150.0f;
        terrain.SetAltitude(15, 140.0f);
        rlCtx.flag = 2;
        rlComp.flag48 = 1;
        terrain.ApplyBrushEffect(TerrainBrushEffect::RaiseLower, 15, rlCtx);
        Check(terrain.GetAltitude(15) == 150.0f,
              "kMaxTerrainHeight tunable override changes the clamp @ 0x100734d13");
        cSTETerrain::MaxTerrainHeightTunable() = 200.0f; // shipped default

        // ---- BrushFnSmoothen @ 0x100732bd2 (8-neighbor threshold) ----
        // Flat terrain at alt=0, set center vertex (12) to 100, neighbors at 0
        // Cross threshold = 16, diagonal threshold = 8
        for (Sint32 i = 0; i < 25; ++i) {
            terrain.SetAltitude(i, 0.0f);
            terrain.SetWaterDepth(i, 0.0f);
        }
        terrain.SetAltitude(12, 100.0f); // center (2,2)
        TerrainBrushContext smCtx;
        TerrainBrushComponent smComp;
        smCtx.flag = 1;
        smCtx.amount = 1.0f;
        smComp.flag48 = 0;
        smCtx.target = &smComp;
        // Cross neighbors (N=7, E=13, S=17, W=11) diff = 100 > 16 -> should modify
        // Diagonal neighbors (NE=8, SE=18, SW=16, NW=6) diff = 100 > 8 -> should modify
        Check(terrain.ApplyBrushEffect(TerrainBrushEffect::Smoothen, 12, smCtx),
              "BrushFnSmoothen modifies when diff exceeds threshold @ 0x100732bd2");
        // Center lowered by 8 * adjustAmount, neighbors raised
        Float32 centerAfter = terrain.GetAltitude(12);
        Check(centerAfter < 100.0f && centerAfter > 0.0f,
              "BrushFnSmoothen lowers center vertex @ 0x100732bd2");
        // Neighbor N (7) should be raised
        Check(terrain.GetAltitude(7) > 0.0f,
              "BrushFnSmoothen raises cross neighbor @ 0x100732bd2");
        // Neighbor NE (8) should be raised
        Check(terrain.GetAltitude(8) > 0.0f,
              "BrushFnSmoothen raises diagonal neighbor @ 0x100732bd2");

        // Below threshold test: center at 10, neighbors at 0, cross threshold 16
        for (Sint32 i = 0; i < 25; ++i) {
            terrain.SetAltitude(i, 0.0f);
            terrain.SetWaterDepth(i, 0.0f);
        }
        terrain.SetAltitude(12, 10.0f); // diff = 10 < 16 (cross), < 8? no 10 > 8 (diagonal)
        // Only diagonal neighbors should be modified (diff=10 > diagonal threshold 8)
        Check(terrain.ApplyBrushEffect(TerrainBrushEffect::Smoothen, 12, smCtx),
              "BrushFnSmoothen modifies diagonal when cross below threshold @ 0x100732bd2");
        Check(terrain.GetAltitude(7) == 0.0f,
              "BrushFnSmoothen cross neighbor unchanged when diff <= cross threshold @ 0x100732bd2");
        Check(terrain.GetAltitude(8) > 0.0f,
              "BrushFnSmoothen diagonal neighbor modified when diff > diagonal threshold @ 0x100732bd2");

        // Out-of-bounds vertex -> false
        Check(!terrain.ApplyBrushEffect(TerrainBrushEffect::Smoothen, 25, smCtx),
              "BrushFnSmoothen out-of-range vertex -> false @ 0x100732bd2");
        // Zero flag -> false
        smCtx.flag = 0;
        Check(!terrain.ApplyBrushEffect(TerrainBrushEffect::Smoothen, 12, smCtx),
              "BrushFnSmoothen zero flag -> false @ 0x100732bd2");

        // ---- ApplyBrushEffect dispatcher with new branches ----
        // Fresh contexts/fixture that guarantee modification (the prior checks
        // used the stale rain ctx whose flag48 had been mutated to equal ctx.flag
        // by the no-op oracle, so both dispatch checks returned false).
        TerrainBrushContext dispCtx;
        TerrainBrushComponent dispComp;
        dispCtx.flag = 2;
        dispCtx.amount = 10.0f;
        dispComp.flag48 = 1;
        dispCtx.target = &dispComp;
        terrain.SetAltitude(0, 0.0f);
        terrain.SetWaterDepth(0, 0.0f);
        Check(terrain.ApplyBrushEffect(TerrainBrushEffect::RaiseLower, 0, dispCtx) == true,
              "ApplyBrushEffect RaiseLower wired @ 0x1007329cc");
        // Smoothen wiring: vertex 0 at 100, neighbor (1,0)=index 1 at 0 -> cross diff 100 > 16
        terrain.SetAltitude(0, 100.0f);
        terrain.SetWaterDepth(0, -100.0f);
        Check(terrain.ApplyBrushEffect(TerrainBrushEffect::Smoothen, 0, dispCtx) == true,
              "ApplyBrushEffect Smoothen wired @ 0x100732bd2");
        Check(terrain.ApplyBrushEffect(TerrainBrushEffect::Flora, 0, ctx) == false,
              "ApplyBrushEffect Flora wired, always returns false @ 0x100732a0e");

        // ---- Brush dispatch pipeline: ApplyBrush @ 0x100731c50 ->
        //      ApplyBrushComponents @ 0x100731d10 -> ApplyBrushAtLocation @ 0x1007321b8
        //      -> BrushFnRaiseLower @ 0x1007329cc ----
        // Synthetic brush: one RaiseLower component with a 4x4 texture whose
        // center texel (pixel 2*4+2 = 10) carries alpha 2. Brush at world
        // (32,32) -> grid (2,2); footprint rows 1..2 with row-2 crossings at
        // columns 1 and 2 (spanMax = 3 after the +1); only the texel-in-bounds
        // column 2 sweeps, so exactly vertex 2*5+2 = 12 is modified.
        {
            cSTETerrain brushTerrain;
            Check(brushTerrain.Init(4, 4), "pipeline fixture Init(4,4) @ 0x100739cb0");
            for (Sint32 i = 0; i < 25; ++i) brushTerrain.SetAltitude(i, 10.0f);

            Uint8 texData[16] = {};
            texData[10] = 2; // texRow 2, texCol 2 -> pixel 2*4+2
            S3DTextureBufferInfo texInfo;
            texInfo.width = 4;
            texInfo.height = 4;
            texInfo.data = texData;
            TerrainTextureSet texSet;
            texSet.bufferInfo = &texInfo;

            TerrainBrushDescription desc;
            desc.useJitter = 0; // deterministic rotationAccum = 0
            desc.radii = {8.0f}; // desc+0x10 array read by BrushRadius @ 0x100734dbc
            TerrainBrushComponent pipeComp;
            pipeComp.effect = 0; // RaiseLower
            pipeComp.textureSet = &texSet;
            pipeComp.radii = {8.0f};
            pipeComp.strengths = {2.5f};
            pipeComp.flag48 = 0;
            desc.components.push_back(pipeComp);
            constexpr Uint32 kTestBrushId = 0x44556677u;
            cSTETerrain::BrushDescriptionMap()[kTestBrushId] = desc;

            TerrainBrushParameters params;
            params.x = 32.0f;
            params.z = 32.0f;         // grid (2,2) = vertex 12
            params.prevX = 32.0f;     // no direction change -> angle untouched
            params.prevZ = 32.0f;
            params.strengthIndex = 0;
            params.strengthScale = 1.0f;
            params.radiusScale = 1.0f;
            params.reDisplay = true;
            params.reDisplayArg = 7;

            Check(brushTerrain.ApplyBrush(kTestBrushId, params),
                  "ApplyBrush finds the brush and runs the pipeline @ 0x100731c50");
            // ctx.amount = 2.5 * 1; ctx.flag = texel alpha 2 > flag48 0 ->
            // raise vertex 12 by flag*amount = 5.
            Check(brushTerrain.GetAltitude(12) == 15.0f,
                  "pipeline raises the swept vertex by flag*amount @ 0x1007321b8/0x1007329cc");
            Check(brushTerrain.GetWaterDepthAtVertex(12) == -15.0f,
                  "pipeline water depth updated via RaiseVertex @ 0x100734cde");
            Check(params.rect.left == 2 && params.rect.top == 2 && params.rect.right == 2 &&
                      params.rect.bottom == 2,
                  "ApplyBrush out-rect is the union of modified vertices {2,2,2,2} @ 0x100731c50");
            Check(brushTerrain.ReDisplayCallCount() == 1,
                  "ApplyBrush re-display gate fires via vtable+0x228 @ 0x100731d07");
            // reDisplay false -> gate skipped, second application accumulates.
            TerrainBrushParameters params2 = params;
            params2.reDisplay = false;
            brushTerrain.ApplyBrush(kTestBrushId, params2);
            Check(brushTerrain.ReDisplayCallCount() == 1 && params2.rect.left == 2,
                  "ApplyBrush re-display gate skips when params.reDisplay is false @ 0x100731cda");
            Check(brushTerrain.GetAltitude(12) == 20.0f,
                  "pipeline second application accumulates @ 0x100731d10");
            // Unknown brush -> false, out-rect stays kEmptyRect.
            TerrainBrushParameters missParams;
            Check(!brushTerrain.ApplyBrush(0xdeadbeefu, missParams),
                  "ApplyBrush unknown brush id -> false @ 0x100731cb6");
            Check(missParams.rect.left == 0x7fffffff &&
                      missParams.rect.right == static_cast<Sint32>(0x80000000u),
                  "ApplyBrush out-rect kEmptyRect-init @ 0x100731c61");
            // BrushRadius @ 0x100734d6e
            Check(brushTerrain.BrushRadius(kTestBrushId) == 8.0f,
                  "BrushRadius reads desc radii[strengthIndex] @ 0x100734d6e");
            Check(brushTerrain.BrushRadius(0xdeadbeefu) == 32.0f,
                  "BrushRadius default 32.0f @ 0x100a18764");
            // DoPostBrushApplicationOperation branches @ 0x10073340e
            TerrainBrushContext postCtx;
            SC4RectI padRect{1, 1, 2, 2};
            brushTerrain.DoPostBrushApplicationOperation(2, postCtx, padRect);
            Check(padRect.left == 0 && padRect.top == 0 && padRect.right == 3 &&
                      padRect.bottom == 3,
                  "DoPostBrushApplicationOperation effect 2 -> PadRectangle(rect,1) @ 0x10073340e");
            SC4RectI ashRect{1, 1, 2, 2};
            brushTerrain.DoPostBrushApplicationOperation(7, postCtx, ashRect);
            Check(brushTerrain.VolcanicAshRegionCount() == 1,
                  "DoPostBrushApplicationOperation effect 7 -> AddVolcanicAshRegion @ 0x10073340e");
            SC4RectI ashRect2{2, 2, 3, 3}; // overlaps {1,1,2,2} -> merged
            brushTerrain.DoPostBrushApplicationOperation(7, postCtx, ashRect2);
            Check(brushTerrain.VolcanicAshRegionCount() == 1,
                  "AddVolcanicAshRegion merges overlapping regions @ 0x100731628");
            const SC4RectI& mergedRegion = brushTerrain.VolcanicAshFront();
            Check(mergedRegion.left == 1 && mergedRegion.top == 1 &&
                      mergedRegion.right == 3 && mergedRegion.bottom == 3,
                  "AddVolcanicAshRegion merged rect {1,1,3,3} @ 0x100731628");
            SC4RectI noRect{0, 0, 1, 1};
            brushTerrain.DoPostBrushApplicationOperation(0, postCtx, noRect);
            Check(noRect.left == 0 && noRect.right == 1 &&
                      brushTerrain.VolcanicAshRegionCount() == 1,
                  "DoPostBrushApplicationOperation other effects -> no-op @ 0x10073340e");
            // BeginBrushScrub @ 0x100734a7e: decal-map clear + vtable+0x2b8 tail
            brushTerrain.SeedDecalMapForTest(1);
            brushTerrain.BeginBrushScrub(kTestBrushId, 1u);
            Check(brushTerrain.DecalMapSize() == 0 &&
                      brushTerrain.BeginScrubDelegateCount() == 1,
                  "BeginBrushScrub clears the decal map and tails to vtable+0x2b8 @ 0x100734a7e");
            // Every exit path converges on the shared tail epilogue
            // @ 0x100734b48..0x100734b58 (empty-map je @ 0x100734ad3, not-found
            // je @ 0x100734afe, key-mismatch ja @ 0x100734b04, spSSE-null je
            // @ 0x100734b13, found fall-through) — the vtable+0x2b8 delegate
            // fires for a NOT-found brush too.
            brushTerrain.BeginBrushScrub(0xdeadbeefu, 1u);
            Check(brushTerrain.BeginScrubDelegateCount() == 2 &&
                      brushTerrain.DecalMapSize() == 0,
                  "BeginBrushScrub not-found path also tails to vtable+0x2b8 @ 0x100734afe/0x100734b58");
            // GetTextureValue @ 0x100733436 with a ctx wired like the pipeline's
            // ComputeRectAroundPt @ 0x10073255c regression pin (non-square aspect,
            // angle 0): h = size * -0.5 = -4, v = aspect * -0.5 = -2, so the four
            // corners are pts[0]=(28,30), pts[1]=(28,34), pts[2]=(36,34),
            // pts[3]=(36,30) — pts[3].x is -h (xmm2 @ 0x1007325cc), NOT -v.
            {
                SC4PointF fp[5];
                brushTerrain.ComputeRectAroundPt(fp, 32.0f, 32.0f, 8.0f, 4.0f, 0.0f);
                Check(fp[0].x == 28.0f && fp[0].z == 30.0f &&
                          fp[1].x == 28.0f && fp[1].z == 34.0f &&
                          fp[2].x == 36.0f && fp[2].z == 34.0f &&
                          fp[3].x == 36.0f && fp[3].z == 30.0f,
                      "ComputeRectAroundPt four rectangle corners, pts[3].x = -h @ 0x10073255c");
            }
            TerrainBrushContext gtCtx;
            TerrainBrushDescription gtDesc; // rotationAccum = 0
            TerrainBrushComponent gtComp;
            gtComp.textureSet = &texSet;
            gtComp.radii = {8.0f};
            gtCtx.x = 32.0f;
            gtCtx.z = 32.0f;
            gtCtx.strengthIndex = 0;
            gtCtx.target = &gtComp;
            gtCtx.params = &params; // radiusScale = 1
            gtCtx.desc = &gtDesc;
            gtCtx.flag = 0;
            Check(brushTerrain.GetTextureValue(2, 2, gtCtx) && gtCtx.flag == 2,
                  "GetTextureValue fetches the texel byte into ctx.flag @ 0x100733436");
            Check(!brushTerrain.GetTextureValue(7, 2, gtCtx),
                  "GetTextureValue out-of-texture-bounds -> false @ 0x100733436");
            // Clean up the shared map entry so later runs are unaffected.
            cSTETerrain::BrushDescriptionMap().erase(kTestBrushId);
        }

        // ====================================================================
        // Wave-2 brush effects: BrushFnFlora @ 0x100732a0e, InitFloraBrushInfo
        // @ 0x1007385e2, BrushFnPaint @ 0x100732d22, BrushFnPlaceEffect
        // @ 0x100732f92, ApplyWaterErosionBrush @ 0x1007332ae, DoWaterErosion
        // @ 0x1007335e6, GetNeighborWithLowestWaterLevel @ 0x100737b50.
        // ====================================================================

        // ---- GetNeighborWithLowestWaterLevel @ 0x100737b50 (direction
        //      table at this+0xa4; cross diffs unscaled, best x sqrt(2),
        //      then diagonals) ----
        {
            cSTETerrain nbrTerrain;
            Check(nbrTerrain.Init(4, 4), "neighbor fixture Init(4,4) @ 0x100739cb0");
            for (Sint32 i = 0; i < 25; ++i) nbrTerrain.SetAltitude(i, 290.0f);
            // Vertex 12 = (2,2). East neighbor (13) lowest -> direction 4 (E).
            nbrTerrain.SetAltitude(13, 100.0f);
            Check(nbrTerrain.GetNeighborWithLowestWaterLevel(12) == 4,
                  "GetNeighborWithLowestWaterLevel picks the east cross neighbor (dir 4) @ 0x100737b50");
            // Diagonal SE (18) lower still -> dir 5 beats the cross best once
            // the cross diff is scaled by sqrt(2) (cross 190 -> 268.7 < 280).
            nbrTerrain.SetAltitude(18, 10.0f);
            Check(nbrTerrain.GetNeighborWithLowestWaterLevel(12) == 5,
                  "GetNeighborWithLowestWaterLevel scales cross diffs by kRoot2 before diagonals @ 0x100733c4a");
            // No strictly lower neighbor -> -1 (flat terrain).
            nbrTerrain.SetAltitude(13, 290.0f);
            nbrTerrain.SetAltitude(18, 290.0f);
            Check(nbrTerrain.GetNeighborWithLowestWaterLevel(12) == -1,
                  "GetNeighborWithLowestWaterLevel returns -1 when no neighbor is lower @ 0x100737bf6");
        }

        // ---- InitFloraBrushInfo @ 0x1007385e2 (count/fill queries + the
        //      live-count filter keeping attr & 3 == 2) ----
        {
            cSTETerrain floraTerrain;
            Check(floraTerrain.Init(4, 4), "flora fixture Init(4,4) @ 0x100739cb0");
            floraTerrain.FloraBrushInfoSource() = {
                {1u, 2u}, {2u, 1u}, {3u, 2u}, {4u, 6u}};
            floraTerrain.InitFloraBrushInfo();
            // id 1 (2&3==2 keep), id 2 (1 remove), id 3 (2 keep), id 4 (6&3==2
            // keep): count 3, filtered ids {1,3,4} (removals shift left and
            // decrement the live count @ 0x1007386c5..0x1007386ce).
            Check(floraTerrain.FloraBrushInfoCount() == 3,
                  "InitFloraBrushInfo filters to attr-2 flora types, live count decremented @ 0x1007385e2");
            Check(floraTerrain.FloraBrushIds().size() == 3 &&
                      floraTerrain.FloraBrushIds()[0] == 1u &&
                      floraTerrain.FloraBrushIds()[1] == 3u &&
                      floraTerrain.FloraBrushIds()[2] == 4u,
                  "InitFloraBrushInfo removes non-attr-2 ids in place @ 0x10073868f..0x1007386be");
            // One-shot gate @ 0x1007385f3: a second call must not re-run while
            // +0x150 != 0 (append to the modeled source, ids must not change).
            floraTerrain.FloraBrushInfoSource().push_back({9u, 2u});
            floraTerrain.InitFloraBrushInfo();
            Check(floraTerrain.FloraBrushIds().size() == 3,
                  "InitFloraBrushInfo one-shot gate on +0x150 != 0 @ 0x1007385f3");
        }

        // ---- BrushFnFlora @ 0x100732a0e (gate order + RNG consumption) ----
        {
            cSTETerrain floraTerrain;
            Check(floraTerrain.Init(4, 4), "flora fn fixture Init(4,4) @ 0x100739cb0");
            for (Sint32 i = 0; i < 25; ++i) floraTerrain.SetAltitude(i, 10.0f);
            TerrainBrushContext flCtx;
            TerrainBrushComponent flComp;
            flCtx.target = &flComp;
            flCtx.amount = 1.0f;
            // Gate 1 @ 0x100732a2e: flag < 10 exits before ANY RNG draw.
            cSTETerrain::sRand.seedA = 777u;
            const Uint32 seedBefore = cSTETerrain::sRand.seedA;
            flCtx.flag = 9;
            Check(floraTerrain.BrushFnFlora(12, flCtx) == false &&
                      cSTETerrain::sRand.seedA == seedBefore,
                  "BrushFnFlora flag < 10 gate exits before the placement roll @ 0x100732a2e");
            // Find seeds whose first RandomUint32Uniform(100) roll is <= 30 so
            // the placement path runs deterministically (gate 2
            // @ 0x100732a38..0x100732a4c), and seeds whose roll is > 30 so the
            // bail path runs. Prediction uses a state clone (the model RNG is
            // the oracle for the consumption order).
            auto rollForSeed = [](Uint32 seedA) {
                RZRandomState probe;
                probe.seedA = seedA;
                return RandomUint32Uniform(probe, 100u);
            };
            Uint32 passSeed = 1u, failSeed = 1u;
            while (rollForSeed(passSeed) > 30u) ++passSeed;
            while (rollForSeed(failSeed) <= 30u) ++failSeed;
            // Bail on roll > 30: exactly one roll consumed, no placement.
            cSTETerrain::sRand.seedA = failSeed;
            cSTETerrain::sRand.seedB = 4242u; // nonzero: RandomDoubleUniform must not hit the clock init
            TerrainBrushContext flCtx2;
            flCtx2.flag = 10; // gate 1 passes
            flCtx2.amount = 1.0f;
            flCtx2.target = &flComp;
            cSTETerrain::sRand.seedA = failSeed;
            cSTETerrain::sRand.seedB = 4242u;
            Check(floraTerrain.BrushFnFlora(12, flCtx2) == false &&
                      floraTerrain.FloraPlacements().empty(),
                  "BrushFnFlora placement roll > 30 bails with no placement @ 0x100732a4c");
            RZRandomState consumedOne;
            consumedOne.seedA = failSeed;
            RandomUint32Uniform(consumedOne, 100u);
            Check(cSTETerrain::sRand.seedA == consumedOne.seedA &&
                      cSTETerrain::sRand.seedB == 4242u,
                  "BrushFnFlora bail consumes exactly one RandomUint32Uniform(100) @ 0x100732a44");
            // Submerged vertex bail @ 0x100732a69: depth > 0 stops after the
            // roll, before the jitter draws.
            floraTerrain.SetWaterDepth(12, 5.0f);
            cSTETerrain::sRand.seedA = passSeed;
            cSTETerrain::sRand.seedB = 4242u;
            Check(floraTerrain.BrushFnFlora(12, flCtx2) == false &&
                      floraTerrain.FloraPlacements().empty(),
                  "BrushFnFlora submerged-vertex bail @ 0x100732a69");
            Check(cSTETerrain::sRand.seedB == 4242u,
                  "BrushFnFlora submerged bail consumes no RandomDoubleRangeUniform @ 0x100732a69");
            floraTerrain.SetWaterDepth(12, 0.0f);
            // Cell-bounds gate @ 0x100732aa6/0x100732ab4: vertex 20 = (0,4),
            // z >= cellCountZ -> exit after the roll, before the draws.
            cSTETerrain::sRand.seedA = passSeed;
            cSTETerrain::sRand.seedB = 4242u;
            Check(floraTerrain.BrushFnFlora(20, flCtx2) == false &&
                      cSTETerrain::sRand.seedB == 4242u,
                  "BrushFnFlora cell-bounds gate stops before the jitter draws @ 0x100732ab4");
            // Full placement path on vertex 12 = (2,2): roll, jittered point in
            // [32,48) x [32,48), flora-index roll on the +0x150 count, y from
            // GetAltitude @ 0x100738928 (flat 10.0f).
            floraTerrain.FloraBrushInfoSource() = {{42u, 2u}};
            floraTerrain.InitFloraBrushInfo();
            Check(floraTerrain.FloraBrushInfoCount() == 1,
                  "flora fn fixture has one attr-2 brush id @ 0x1007385e2");
            cSTETerrain::sRand.seedA = passSeed;
            cSTETerrain::sRand.seedB = 4242u;
            // Mirror of the clean-room RandomDoubleRangeUniform helper in
            // terrain.cpp (cRZRandom::RandomDoubleRangeUniform @ 0x10003d3e4,
            // body not reconstructed).
            auto rangeDraw = [](RZRandomState& st, Float64 lo, Float64 hi) {
                return static_cast<Float32>(lo + (hi - lo) * RandomDoubleUniform(st));
            };
            RZRandomState pred;
            pred.seedA = passSeed;
            pred.seedB = 4242u;
            RandomUint32Uniform(pred, 100u);               // gate-2 roll
            const Float32 wantX = rangeDraw(pred, 32.0, 48.0);
            const Float32 wantZ = rangeDraw(pred, 32.0, 48.0);
            const Sint32 wantIndex = static_cast<Sint32>(RandomUint32Uniform(pred, 1u));
            Check(floraTerrain.BrushFnFlora(12, flCtx2) == false,
                  "BrushFnFlora always returns false on the placement path @ 0x100732bbf");
            Check(floraTerrain.FloraPlacements().size() == 1 &&
                      floraTerrain.FloraPlacements()[0].x == wantX &&
                      floraTerrain.FloraPlacements()[0].z == wantZ &&
                      floraTerrain.FloraPlacements()[0].floraIndex == wantIndex &&
                      floraTerrain.FloraPlacements()[0].y == 10.0f,
                  "BrushFnFlora records the jittered placement in RNG order (roll, x, z, index) @ 0x100732a38..0x100732b5c");
        }

        // ---- Pipeline wiring, effect 1 (Flora): InitFloraBrushInfo runs,
        //      the +0x150 != 0 gate controls the sweep @ 0x100732017 ----
        {
            cSTETerrain fTerrain;
            Check(fTerrain.Init(4, 4), "flora pipeline fixture Init(4,4) @ 0x100739cb0");
            Uint8 texData[16] = {};
            texData[10] = 10; // texel alpha 10 at vertex 12 (see the pipeline pin)
            S3DTextureBufferInfo texInfo;
            texInfo.width = 4;
            texInfo.height = 4;
            texInfo.data = texData;
            TerrainTextureSet texSet;
            texSet.bufferInfo = &texInfo;
            TerrainBrushDescription desc;
            desc.useJitter = 0;
            TerrainBrushComponent comp;
            comp.effect = 1; // Flora
            comp.textureSet = &texSet;
            comp.radii = {8.0f};
            comp.strengths = {1.0f};
            desc.components.push_back(comp);
            constexpr Uint32 kFloraBrushId = 0x44556678u;
            cSTETerrain::BrushDescriptionMap()[kFloraBrushId] = desc;
            TerrainBrushParameters params;
            params.x = 32.0f;
            params.z = 32.0f;
            params.prevX = 32.0f;
            params.prevZ = 32.0f;
            params.strengthIndex = 0;
            // No flora source: InitFloraBrushInfo leaves +0x150 == 0, the
            // sweep is skipped, nothing is consumed.
            cSTETerrain::sRand.seedA = 99u;
            cSTETerrain::sRand.seedB = 99u;
            Check(fTerrain.ApplyBrush(kFloraBrushId, params),
                  "Flora pipeline brush found @ 0x100731c50");
            Check(fTerrain.FloraPlacements().empty() &&
                      cSTETerrain::sRand.seedA == 99u && cSTETerrain::sRand.seedB == 99u,
                  "Flora sweep skipped when the filtered count is 0 @ 0x100732017");
            // With an attr-2 source the gate opens and the covered vertex
            // rolls (at least the gate-2 RandomUint32Uniform is consumed).
            fTerrain.FloraBrushInfoSource() = {{42u, 2u}};
            cSTETerrain::sRand.seedA = 1001u;
            cSTETerrain::sRand.seedB = 1001u;
            fTerrain.ApplyBrush(kFloraBrushId, params);
            Check(fTerrain.FloraBrushInfoCount() == 1 &&
                      cSTETerrain::sRand.seedA != 1001u,
                  "Flora sweep runs BrushFnFlora when +0x150 != 0 (gate-2 roll consumed) @ 0x100732017/0x100732a44");
            cSTETerrain::BrushDescriptionMap().erase(kFloraBrushId);
        }

        // ---- BrushFnPaint @ 0x100732d22 through the pipeline (decal-map
        //      count gate + jitter/point RNG order + place-decal record) ----
        {
            cSTETerrain pTerrain;
            Check(pTerrain.Init(4, 4), "paint fixture Init(4,4) @ 0x100739cb0");
            for (Sint32 i = 0; i < 25; ++i) pTerrain.SetAltitude(i, 10.0f);
            TerrainDecalPlacementTarget target;
            pTerrain.SetDecalPlacementTarget(&target);
            Uint8 texData[16] = {};
            texData[10] = 5; // texel alpha 5 at vertex 12
            S3DTextureBufferInfo texInfo;
            texInfo.width = 4;
            texInfo.height = 4;
            texInfo.data = texData;
            TerrainTextureSet texSet;
            texSet.bufferInfo = &texInfo;
            TerrainBrushDescription desc;
            desc.useJitter = 0;
            TerrainBrushComponent comp;
            comp.effect = 3; // Paint
            comp.textureSet = &texSet;
            comp.radii = {8.0f};
            comp.strengths = {1.0f};
            comp.maxDecalCount = 1;      // +0x4c
            comp.decalResourceId = 0x1234u; // +0x50
            comp.jitterAmplitude = 2.0f;  // +0x54
            desc.components.push_back(comp);
            constexpr Uint32 kPaintBrushId = 0x44556679u;
            cSTETerrain::BrushDescriptionMap()[kPaintBrushId] = desc;
            TerrainBrushParameters params;
            params.x = 32.0f;
            params.z = 32.0f;
            params.prevX = 32.0f;
            params.prevZ = 32.0f;
            params.strengthIndex = 0;
            cSTETerrain::sRand.seedA = 55u;
            cSTETerrain::sRand.seedB = 55u;
            // Oracle: jitter draw, then rndX, then rndZ (order pinned by the
            // RandomDoubleRangeUniform consumption sequence).
            auto rangeDraw = [](RZRandomState& st, Float64 lo, Float64 hi) {
                return static_cast<Float32>(lo + (hi - lo) * RandomDoubleUniform(st));
            };
            RZRandomState pred;
            pred.seedA = 55u;
            pred.seedB = 55u;
            const Float32 wantJ = rangeDraw(pred, 0.0, 2.0);
            const Float32 wantX = rangeDraw(pred, 32.0, 48.0);
            const Float32 wantZ = rangeDraw(pred, 32.0, 48.0);
            Check(pTerrain.ApplyBrush(kPaintBrushId, params),
                  "Paint pipeline brush found @ 0x100731c50");
            Check(target.placements.size() == 1 &&
                      target.placements[0].resourceId == 0x1234u &&
                      target.placements[0].amount == 1.0f &&
                      target.placements[0].jitter == wantJ &&
                      target.placements[0].x == wantX &&
                      target.placements[0].z == wantZ,
                  "BrushFnPaint records the decal placement in RNG order (jitter, x, z) @ 0x100732e86..0x100732f47");
            Check(pTerrain.DecalMapEntryCountAt(12) == 1,
                  "BrushFnPaint increments the decal-map count past the +0x4c gate @ 0x100732e60");
            // Second application: the binary's unsigned `ja` gate
            // @ 0x100732e5a only skips when the count EXCEEDS +0x4c — with
            // count 1 <= maxDecalCount 1 a second placement records and the
            // count reaches 2 (BINARY ODDITY: up to max+1 placements per
            // vertex).
            pTerrain.ApplyBrush(kPaintBrushId, params);
            Check(target.placements.size() == 2 &&
                      pTerrain.DecalMapEntryCountAt(12) == 2,
                  "BrushFnPaint count <= maxDecalCount still places (ja skips only on count > max) @ 0x100732e5a");
            // Third application: count 2 > maxDecalCount 1 -> the gate fails
            // before any draw (seedB must not move).
            const Uint32 seedBAfter = cSTETerrain::sRand.seedB;
            pTerrain.ApplyBrush(kPaintBrushId, params);
            Check(target.placements.size() == 2 &&
                      pTerrain.DecalMapEntryCountAt(12) == 2 &&
                      cSTETerrain::sRand.seedB == seedBAfter,
                  "BrushFnPaint count > maxDecalCount gate exits before the draws @ 0x100732e5a");
            cSTETerrain::BrushDescriptionMap().erase(kPaintBrushId);
        }

        // ---- BrushFnPlaceEffect @ 0x100732f92 through the pipeline (flag !=
        //      flag48 gate, +0x138 eligibility query, y = max(seaLevel,
        //      GetAltitude), count increments only on query success) ----
        {
            cSTETerrain eTerrain;
            Check(eTerrain.Init(4, 4), "effect fixture Init(4,4) @ 0x100739cb0");
            for (Sint32 i = 0; i < 25; ++i) eTerrain.SetAltitude(i, 10.0f);
            eTerrain.SetSeaLevelForTest(50.0f);
            Uint8 texData[16] = {};
            texData[10] = 5; // texel alpha 5 at vertex 12
            S3DTextureBufferInfo texInfo;
            texInfo.width = 4;
            texInfo.height = 4;
            texInfo.data = texData;
            TerrainTextureSet texSet;
            texSet.bufferInfo = &texInfo;
            TerrainBrushDescription desc;
            desc.useJitter = 0;
            TerrainBrushComponent comp;
            comp.effect = 4; // PlaceEffect
            comp.textureSet = &texSet;
            comp.radii = {8.0f};
            comp.strengths = {1.0f};
            comp.flag48 = 0;         // ctx.flag 5 != 0 -> proceeds
            comp.maxDecalCount = 5;  // +0x4c
            comp.effectParam58 = 7;  // +0x58
            comp.effectParam5c = 9;  // +0x5c
            desc.components.push_back(comp);
            constexpr Uint32 kEffectBrushId = 0x44556680u;
            cSTETerrain::BrushDescriptionMap()[kEffectBrushId] = desc;
            TerrainBrushParameters params;
            params.x = 32.0f;
            params.z = 32.0f;
            params.prevX = 32.0f;
            params.prevZ = 32.0f;
            params.strengthIndex = 0;
            cSTETerrain::sRand.seedA = 66u;
            cSTETerrain::sRand.seedB = 66u;
            auto rangeDraw = [](RZRandomState& st, Float64 lo, Float64 hi) {
                return static_cast<Float32>(lo + (hi - lo) * RandomDoubleUniform(st));
            };
            RZRandomState pred;
            pred.seedA = 66u;
            pred.seedB = 66u;
            const Float32 wantX = rangeDraw(pred, 32.0, 48.0);
            const Float32 wantZ = rangeDraw(pred, 32.0, 48.0);
            Check(eTerrain.ApplyBrush(kEffectBrushId, params),
                  "PlaceEffect pipeline brush found @ 0x100731c50");
            Check(eTerrain.EffectPlacementTarget().placements.size() == 1 &&
                      eTerrain.EffectPlacementTarget().placements[0].effectParam58 == 7 &&
                      eTerrain.EffectPlacementTarget().placements[0].effectParam5c == 9 &&
                      eTerrain.EffectPlacementTarget().placements[0].x == wantX &&
                      eTerrain.EffectPlacementTarget().placements[0].z == wantZ &&
                      eTerrain.EffectPlacementTarget().placements[0].y == 50.0f,
                  "BrushFnPlaceEffect records (params, rndX, max(seaLevel, GetAltitude), rndZ) @ 0x100733122..0x1007331da");
            Check(eTerrain.DecalMapEntryCountAt(12) == 1,
                  "BrushFnPlaceEffect increments the decal-map count only after the +0x138 query succeeds @ 0x1007330d0");
            // Query gate false: no placement, no count increment.
            eTerrain.EffectPlacementTarget().queryResult = false;
            eTerrain.ApplyBrush(kEffectBrushId, params);
            Check(eTerrain.EffectPlacementTarget().placements.size() == 1 &&
                      eTerrain.DecalMapEntryCountAt(12) == 1,
                  "BrushFnPlaceEffect +0x138 query failure blocks the placement and the count increment @ 0x1007330c8");
            cSTETerrain::BrushDescriptionMap().erase(kEffectBrushId);
        }

        // ---- Water erosion @ 0x1007332ae/0x1007335e6 through the pipeline:
        //      effect 6 -> ApplyWaterErosionBrush -> DoWaterErosion ----
        {
            cSTETerrain wTerrain;
            Check(wTerrain.Init(4, 4), "erosion fixture Init(4,4) @ 0x100739cb0");
            wTerrain.SetSeaLevelForTest(300.0f); // threshold = seaLevel + (-15.0f) = 285
            for (Sint32 i = 0; i < 25; ++i) {
                wTerrain.SetAltitude(i, 290.0f);
                wTerrain.SetWaterDepth(i, 0.0f);
            }
            // Vertex 8 (NE of the center) carries the erosion/deposit
            // suppression bit 0x4000 (record byte +0x23, bit 0x40).
            wTerrain.SetVertexFlagsForTest(8, 0x4000);
            // One flow step for strength index 0 (modeled runtime table; the
            // binary's table @ 0x100b71e00 is filled from loaded tuning data).
            cSTETerrain::FlowStepsPerErosionApplication() = {1};
            TerrainBrushDescription desc;
            desc.useJitter = 0;
            TerrainBrushComponent comp;
            comp.effect = 6; // WaterErosion
            comp.radii = {32.0f}; // strength cells = (int)(32/16) = 2 -> pad 1
            comp.strengths = {1.0f}; // repeat = (int)ctx.amount = 1
            desc.components.push_back(comp);
            constexpr Uint32 kErosionBrushId = 0x44556681u;
            cSTETerrain::BrushDescriptionMap()[kErosionBrushId] = desc;
            TerrainBrushParameters params;
            params.x = 32.0f; // grid (2,2) -> rect {2,2,2,2} -> padded {1,1,3,3}
            params.z = 32.0f;
            params.prevX = 32.0f;
            params.prevZ = 32.0f;
            params.strengthIndex = 0;
            Check(wTerrain.ApplyBrush(kErosionBrushId, params),
                  "erosion pipeline brush found @ 0x100731c50");
            Check(params.rect.left == 1 && params.rect.top == 1 &&
                      params.rect.right == 3 && params.rect.bottom == 3,
                  "ApplyWaterErosionBrush rect = grid point padded by trunc(strengthCells/2) @ 0x1007332e1..0x100733302");
            // Oracle (same float ops as the implementation):
            //  rainfall pass: depth += 290 * 0.01f (kRainFallFactor @ 0x100b17608)
            //  rect-edge vertices dump their full depth and erode at the min
            //  rate (height diff 0 -> kMinHydrErosionRate @ 0x100b17618).
            const Float32 rain = 290.0f * 0.01f;
            const Float32 erodeEdge = (5.0f * rain) * 0.02f; // capacity 5*depth (kMaxSedimentPerUnitWater @ 0x100b1760c)
            const Float32 altEdge = 290.0f - erodeEdge;
            // Interior vertex 12: W neighbor (11, dumped earlier in the same
            // pass) is the first strictly-lowest candidate; cross diffs are
            // unscaled so the first cross winner stays after the sqrt(2)
            // scaling. flow = min(depth, (myLevel - wLevel) * 0.5f), erode =
            // (5 * flow) * 0.02f (height diff 0.29 < ramp begin 5.128).
            const Float32 myLevel = rain + 290.0f;
            const Float32 wLevel = 0.0f + altEdge;
            const Float32 flow = (myLevel - wLevel) * 0.5f;
            const Float32 erode12 = (5.0f * flow) * 0.02f;
            const Float32 alt12 = 290.0f - erode12;
            Check(wTerrain.AltitudeAt(6) == altEdge,
                  "DoWaterErosion rect-edge vertex dumps its depth and erodes at the min rate @ 0x100733811..0x100733981");
            Check(wTerrain.AltitudeAt(8) == 290.0f,
                  "DoWaterErosion vertex-flag bit 0x4000 suppresses erosion @ 0x100733975");
            Check(wTerrain.AltitudeAt(12) == alt12,
                  "DoWaterErosion interior vertex flows to the lowest neighbor and erodes @ 0x100733852..0x100733989");
            // Phase C fallback reset (cSTESurfaceWater::ResetWaterToSeaLevel
            // @ 0x10072e648 modeled): padded rect {0,0,4,4} covers every
            // vertex; depth = seaLevel - altitude below sea level.
            Check(wTerrain.GetWaterDepthAtVertex(6) == 300.0f - altEdge &&
                      wTerrain.GetWaterDepthAtVertex(0) == 10.0f,
                  "DoWaterErosion Phase C resets the padded rect to sea level @ 0x100733a95..0x100733b02");
            // numFlowSteps <= 0: Phase B skipped, altitudes untouched.
            cSTETerrain w0Terrain;
            Check(w0Terrain.Init(4, 4), "erosion-0 fixture Init(4,4) @ 0x100739cb0");
            w0Terrain.SetSeaLevelForTest(300.0f);
            for (Sint32 i = 0; i < 25; ++i) w0Terrain.SetAltitude(i, 290.0f);
            SC4RectI rect0{1, 1, 2, 2};
            w0Terrain.DoWaterErosion(rect0, 0);
            Check(w0Terrain.AltitudeAt(1) == 290.0f &&
                      w0Terrain.GetWaterDepthAtVertex(1) == 10.0f,
                  "DoWaterErosion numFlowSteps <= 0 skips Phase B @ 0x1007336fd..0x100733701");
            cSTETerrain::FlowStepsPerErosionApplication().clear();
            cSTETerrain::BrushDescriptionMap().erase(kErosionBrushId);
        }
    }

    // ------------------------------------------------------------------
    // cSTETerrain surface-cover cluster (M3 terrain wave)
    // (AddLandTypeAtVertex @ 0x100731338 .. SprayVolcanicAsh @ 0x100734724)
    // ------------------------------------------------------------------
    std::printf("\n--- cSTETerrain surface-cover cluster ---\n");
    {
        // ---- DoWaterErosion interior-deposit oracle (kHydrDepositRate 0.2
        //      path @ 0x100733a04..0x100733a52): an interior receiver vertex
        //      whose sediment exceeds its flow capacity keeps the excess and
        //      deposits 20% of it into its altitude. 9x9 vertex grid, rect
        //      {0,0,6,6} (two interior rings), one flow step. The interior
        //      ring sits at altitude 12 so the non-designed interior vertices
        //      are inert (equal levels never flow); A=(1,1) alt 100 flows
        //      into B=(2,1) alt 10, whose only strictly-lower neighbor is
        //      C=(3,1) alt 10.9. ----
        cSTETerrain dep;
        Check(dep.Init(8, 8), "deposit fixture Init(8,8) @ 0x100739cb0");
        dep.SetSeaLevelForTest(-100.0f); // erosion threshold = seaLevel - 15 = -115
        for (Sint32 i = 0; i < 81; ++i) dep.SetAltitude(i, 200.0f);
        for (Sint32 z = 1; z <= 5; ++z) {       // rect interior rows/cols 1..5
            for (Sint32 x = 1; x <= 5; ++x) dep.SetAltitude(9 * z + x, 12.0f);
        }
        dep.SetAltitude(10, 100.0f); // A = (x=1, z=1)
        dep.SetAltitude(11, 10.0f);  // B = (x=2, z=1)
        dep.SetAltitude(12, 10.9f);  // C = (x=3, z=1)
        dep.DoWaterErosion(SC4RectI{0, 0, 6, 6}, 1);
        // Oracle (same float ops as the implementation):
        //  rain: A depth = 100 * kRainFallFactor(0.01) = 1.0
        //  A -> B: flow = min(depth, (levelA - levelB) * 0.5) = 1.0 (full),
        //         erode = (capacity 5 * flow - sediment 0) * 0.06 = 0.3
        //         (height diff 90 >= ramp end 38.14 -> max rate 0.06)
        //  B: depth = 0.1 + 1.0 = 1.1, sediment = 0.3 (A's carry)
        //  B -> C: flow = min(1.1, (11.1 - 11.009) * 0.5) = 0.0455,
        //          capacity = 5 * 0.0455 = 0.2275 < 0.3 -> EXCESS path
        //          (sediment keeps the excess @ 0x1007338f3), leftover depth
        //          1.0545 > 0 -> deposit = 0.0725 * 0.2 = 0.0145
        const Float32 kRain = 0.01f;                   // @ 0x100b17608
        const Float32 rainA = 100.0f * kRain;
        const Float32 levelB0 = 10.0f + 10.0f * kRain;
        const Float32 flowA = std::min(rainA, (rainA + 100.0f - levelB0) * 0.5f);
        const Float32 erodeA = (5.0f * flowA) * 0.06f; // @ 0x100b1761c
        const Float32 expAltA = 100.0f - erodeA;       // 99.7
        const Float32 depthB = 10.0f * kRain + flowA;  // 1.1
        const Float32 levelC0 = 10.9f + 10.9f * kRain; // 11.009
        const Float32 flowB =
            std::min(depthB, (depthB + 10.0f - levelC0) * 0.5f); // 0.0455
        const Float32 capB = 5.0f * flowB;             // 0.2275
        const Float32 newSedB = erodeA - capB;         // 0.0725
        const Float32 depositB = newSedB * 0.2f;       // @ 0x100b17610
        const Float32 expAltB = 10.0f + depositB;      // 10.0145
        const Float32 expSedB = newSedB - depositB;     // 0.058
        Check(std::fabs(dep.AltitudeAt(10) - expAltA) < 1e-4f,
              "DoWaterErosion interior vertex A erodes at the max rate @ 0x10073397d");
        Check(std::fabs(dep.AltitudeAt(11) - expAltB) < 1e-4f,
              "DoWaterErosion interior receiver with leftover depth deposits sediment into altitude (kHydrDepositRate 0.2) @ 0x100733a04..0x100733a52");
        // After B's deposit, two of the three adjacent interior ring
        // vertices — (1,2) and (3,2) — flow their full rain (0.12) into B
        // (each sees B as their lowest neighbor; (2,2) instead sees C,
        // which ends up lower), each carrying (capacity 5*0.12 - 0) * 0.02
        // = 0.012 of sediment (min erosion rate). B's FINAL sediment is
        // therefore expSedB + 2 * 0.012 (~0.082); the deposit itself left
        // 80% of the excess (verified by the altitude check above).
        const Float32 laterCarry = (5.0f * 0.12f) * 0.02f;
        Check(std::fabs(dep.SurfaceWater().sediment[11] - (expSedB + laterCarry + laterCarry)) < 1e-4f,
              "DoWaterErosion deposit leaves 80% of the excess sediment (plus later ring inflows) @ 0x100733a3e");
        Check(dep.AltitudeAt(12) == 10.9f,
              "DoWaterErosion C has no lower neighbor after B's flow -> skipped @ 0x100733857");
        Check(dep.SurfaceWater().sediment[12] > 0.2f,
              "DoWaterErosion C received B's excess-path carry @ 0x1007339a6");

        // ---- AreSeaVertices @ 0x100733b16 ----
        cSTETerrain sea;
        Check(sea.Init(2, 2), "AreSeaVertices fixture Init(2,2)");
        sea.SetSeaLevelForTest(0.0f);
        for (Sint32 i = 0; i < 9; ++i) sea.SetAltitude(i, -5.0f);
        Check(sea.AreSeaVertices(SC4RectI{0, 0, 1, 1}),
              "AreSeaVertices true when every altitude is below sea level @ 0x100733b7d");
        sea.SetAltitude(4, 5.0f);
        Check(!sea.AreSeaVertices(SC4RectI{0, 0, 2, 2}),
              "AreSeaVertices false on one land vertex (strict seaLevel > altitude) @ 0x100733b80");
        sea.SetAltitude(4, -5.0f);
        Check(!sea.AreSeaVertices(SC4RectI{3, 0, 1, 1}),
              "AreSeaVertices unsigned gate: vertexCountX <= left -> false @ 0x100733b26");
        Check(!sea.AreSeaVertices(SC4RectI{0, 0, 3, 1}),
              "AreSeaVertices unsigned gate: vertexCountX <= right -> false @ 0x100733b3b");
        Check(!sea.AreSeaVertices(SC4RectI{0, 3, 1, 1}),
              "AreSeaVertices unsigned gate: vertexCountZ <= top -> false @ 0x100733b32");
        Check(!sea.AreSeaVertices(SC4RectI{0, 0, 1, 3}),
              "AreSeaVertices unsigned gate: vertexCountZ <= bottom -> false @ 0x100733b44");
        Check(sea.AreSeaVertices(SC4RectI{0, 2, 1, 1}),
              "AreSeaVertices ODDITY: empty rect (top > bottom) returns true @ 0x100733b46");
        Check(sea.AreSeaVertices(SC4RectI{2, 0, 1, 1}),
              "AreSeaVertices ODDITY: left > right skips every row and returns true @ 0x100733b5b");

        // ---- AddLandTypeAtVertex @ 0x100731338 (3 slots: match-max /
        //      first-empty / replace-min) ----
        cSTETerrain lt;
        Check(lt.Init(1, 1), "land-type fixture Init(1,1)");
        lt.AddLandTypeAtVertex(0, 5, 0.5f);  // q = int(0.5*255 + 1) = 128
        Check(lt.LandTypeSlotAt(0, 0).type == 5 &&
                  lt.LandTypeSlotAt(0, 0).value == 128,
              "AddLandTypeAtVertex fills the first empty slot with int(ratio*255+1) @ 0x1007313c5");
        lt.AddLandTypeAtVertex(0, 5, 0.9f);  // q = 230 > 128
        Check(lt.LandTypeSlotAt(0, 0).value == 230,
              "AddLandTypeAtVertex matching type keeps the max value @ 0x1007313d6");
        lt.AddLandTypeAtVertex(0, 5, 0.1f);  // q = 26 < 230
        Check(lt.LandTypeSlotAt(0, 0).value == 230,
              "AddLandTypeAtVertex smaller value does not lower the max @ 0x1007313da");
        lt.AddLandTypeAtVertex(0, 7, 0.2f);  // slot 1, q = 52
        lt.AddLandTypeAtVertex(0, 9, 0.2f);  // slot 2, q = 52
        lt.AddLandTypeAtVertex(0, 11, 0.25f); // full: min 52 (slot 1 first), q = 64 > 52
        Check(lt.LandTypeSlotAt(0, 1).type == 11 && lt.LandTypeSlotAt(0, 1).value == 64,
              "AddLandTypeAtVertex full table replaces the min-value slot when larger @ 0x100731386");
        lt.AddLandTypeAtVertex(0, 12, 0.2f);  // q = 52 = new min (slot 2) -> no replace
        Check(lt.LandTypeSlotAt(0, 2).type == 9 && lt.LandTypeSlotAt(0, 2).value == 52,
              "AddLandTypeAtVertex no replacement when the new value <= min @ 0x1007313b0");

        // ---- AdjustLandTypeRatios @ 0x10073173a (251/250 renormalization,
        //      prefix stop at the first empty slot) ----
        cSTETerrain adj;
        Check(adj.Init(1, 1), "ratio fixture Init(1,1)");
        adj.SetLandTypeSlotForTest(0, 0, 1, 100);
        adj.SetLandTypeSlotForTest(0, 1, 2, 100);
        adj.SetLandTypeSlotForTest(0, 2, 3, 100); // sum 300 >= 251
        adj.SetLandTypeSlotForTest(1, 0, 1, 100);
        adj.SetLandTypeSlotForTest(1, 1, 0, 0);   // empty slot stops the scan
        adj.SetLandTypeSlotForTest(1, 2, 2, 100);
        adj.AdjustLandTypeRatios(SC4RectI{0, 0, 1, 1});
        Check(adj.LandTypeSlotAt(0, 0).value == 83 && adj.LandTypeSlotAt(0, 1).value == 83 &&
                  adj.LandTypeSlotAt(0, 2).value == 83,
              "AdjustLandTypeRatios rescales filled slots to value*250/sum when sum >= 251 @ 0x1007317bc");
        Check(adj.LandTypeSlotAt(1, 0).value == 100 && adj.LandTypeSlotAt(1, 2).value == 100,
              "AdjustLandTypeRatios sum stops at the first empty slot (100 < 251, unchanged) @ 0x10073178b");

        // ---- ComputeWaterProximityValues @ 0x100733c62 ----
        cSTETerrain prox;
        Check(prox.Init(10, 10), "proximity fixture Init(10,10)");
        prox.ComputeWaterProximityValues(SC4RectI{0, 0, 1, 1});
        Check(prox.WaterProximityRect().left == 0 && prox.WaterProximityRect().top == 0 &&
                  prox.WaterProximityRect().right == 1 && prox.WaterProximityRect().bottom == 1,
              "ComputeWaterProximityValues unions into the +0xcc dirty rect @ 0x100733c7f");
        Check(prox.WaterProximityAt(5, 5) == 0,
              "ComputeWaterProximityValues area < 100 skips the recompute @ 0x100733ca7");
        prox.SetWaterDepth(57, 5.0f); // (x=2, z=5)
        prox.SetWaterDepth(27, 5.0f); // (x=5, z=2)
        prox.ComputeWaterProximityValues(SC4RectI{0, 0, 10, 10}); // area 100
        Check(prox.NumberOfWaterVerticesField() == 2,
              "ComputeWaterProximityValues stores the water count at +0x108 @ 0x100733cdb");
        Check(prox.WaterProximityAt(2, 5) == 0 && prox.WaterProximityAt(5, 2) == 0,
              "ComputeWaterProximityValues water vertices get proximity 0 @ 0x100733d2e");
        Check(prox.WaterProximityAt(3, 5) == 1 && prox.WaterProximityAt(2, 4) == 1,
              "ComputeWaterProximityValues land near water gets the flood distance @ 0x100733d8e (clean-room BFS)");
        Check(prox.WaterProximityAt(0, 0) == 6,
              "ComputeWaterProximityValues far land keeps kMaxBeachWidth+2 @ 0x100733d3a");
        Check(prox.WaterProximityRect().left == 0x7fffffff,
              "ComputeWaterProximityValues resets the dirty rect to kEmptyRect @ 0x100733cb8");

        // ---- AddToTerrainTypeTextureList @ 0x10073159e + AddTemperature-
        //      AndMoistureToSurfaceCoverEntry @ 0x100731418 ----
        cSTETerrain::AddToTerrainTypeTextureList(1, 30u);
        cSTETerrain::AddToTerrainTypeTextureList(1, 10u);
        cSTETerrain::AddToTerrainTypeTextureList(1, 20u);
        cSTETerrain::AddToTerrainTypeTextureList(1, 20u); // duplicate
        {
            const auto& l = cSTETerrain::TexturesUsedInTerrainType()[1];
            std::vector<Uint32> got(l.textures.begin(), l.textures.end());
            Check(got == std::vector<Uint32>{10u, 20u, 30u} && l.count == 3,
                  "AddToTerrainTypeTextureList sorted-unique insert, count only on insert @ 0x1007315c1..0x100731614");
        }
        cSTETerrain::SurfaceCoverStringHashHook() = [](const char* s) -> Uint32 {
            return static_cast<Uint32>(std::atoi(s));
        };
        cSTETerrain::AddTemperatureAndMoistureToSurfaceCoverEntry("5", "101,202,303", 3);
        {
            const auto& m = cSTETerrain::SurfaceCoverMap();
            // flat index = (entryIndex & 0x7fffff)*512 + keyHash*16 + 1 + i
            // (3*512 + 5*16 + 1 = 1617)
            Check(m[1617] == 101 && m[1618] == 202 && m[1619] == 303,
                  "AddTemperatureAndMoisture tokens stored in the 64-byte sub-entry @ 0x10073150c");
            Check(m[1632 - 1] == 0,
                  "AddTemperatureAndMoisture runs exactly 15 token iterations @ 0x100731522");
            const auto& l = cSTETerrain::TexturesUsedInTerrainType()[3];
            std::vector<Uint32> got(l.textures.begin(), l.textures.end());
            Check(got == std::vector<Uint32>{0u, 101u, 202u, 303u} && l.count == 4,
                  "AddTemperatureAndMoisture registers every token hash @ 0x100731516");
        }
        cSTETerrain::SurfaceCoverStringHashHook() = nullptr;

        // ---- SprayVolcanicAsh @ 0x100734724 ----
        {
            auto& ashInfo = cSTETerrain::SurfaceCoverTextures()[0xe0u];
            ashInfo.multiplier = 1.0f;    // entry +0x18
            ashInfo.spread = {1};         // entry +0xc0 + 4*strength
            ashInfo.curveParam = {1.0f};  // entry +0xd4 + 4*strength
            ashInfo.curves.resize(1);
            ashInfo.curves[0].points = {{0.0f, 1.0f}}; // constant 1.0
        }
        cSTETerrain sva;
        Check(sva.Init(4, 4), "ash fixture Init(4,4)");
        sva.SetVertexFlagsForTest(12, 0x0400); // (x=2,z=2) ash bit (word 0x400)
        sva.SeedVolcanicAshRegionForTest(SC4RectI{1, 1, 3, 3}, 0);
        sva.SprayVolcanicAsh(SC4RectI{0, 0, 4, 4});
        Check(sva.VolcanicAshRegionCoverFlag(0) == 0,
              "SprayVolcanicAsh clears the +0x24 dword when the region is fully covered @ 0x10073484f");
        Check(sva.LandTypeSlotAt(12, 0).type == 0xe0 && sva.LandTypeSlotAt(12, 0).value == 256,
              "SprayVolcanicAsh adds type 0xe0 at the flagged vertex @ 0x1007349dc");
        Check(sva.LandTypeSlotAt(6, 0).type == 0xe0 && sva.LandTypeSlotAt(6, 0).value == 256,
              "SprayVolcanicAsh spreads ash into the flagged vertex's influence rect @ 0x100734864");
        Check(sva.LandTypeSlotAt(0, 0).type == 0,
              "SprayVolcanicAsh leaves vertices outside every influence clean @ 0x100734783");
        cSTETerrain sva2;
        Check(sva2.Init(4, 4), "ash-age fixture Init(4,4)");
        sva2.SetVertexFlagsForTest(12, 0x0400);
        sva2.SeedVolcanicAshRegionForTest(SC4RectI{1, 1, 3, 3}, 5000000);
        sva2.SprayVolcanicAsh(SC4RectI{0, 0, 4, 4});
        Check(sva2.LandTypeSlotAt(12, 0).type == 0xe0 &&
                  sva2.LandTypeSlotAt(12, 0).value == 128, // int(0.5*255+1)
              "SprayVolcanicAsh factor = (1 - age/kVolcanicAshLifeTime) * multiplier @ 0x100734864..0x10073489a");

        // ---- DecideSurfaceCover @ 0x100733e48 (water/cliff/beach/temperature
        //      paths) ----
        cSTETerrain dsc;
        Check(dsc.Init(10, 10), "DecideSurfaceCover fixture Init(10,10)");
        dsc.SetSeaLevelForTest(0.0f); // beachAlt = 0 + 7.0f
        cSTETerrain::MaxSurfaceTextureSpread() = {0};
        cSTETerrain::LowCliffTexResIds() = {0x1234u};
        cSTETerrain::BeachTexResIds() = {0xbeefu};
        cSTETerrain::TemperatureRangeFactor() = 10.0f; // tempRange = 1930
        TerrainMoistureSource ms;
        ms.moistureCode = 3;
        dsc.SetMoistureSource(&ms);
        dsc.SetWaterDepth(60, 20.0f); // (x=5, z=5) = 11*5+5: deep, 15 <= 20 <= 30 -> texId 19
        dsc.SetWaterDepth(84, 5.0f);  // (x=7, z=7) = 11*7+7: shallow, < 15 -> texId 20
        dsc.SetWaterDepth(72, 40.0f); // (x=6, z=6) = 11*6+6: > kMaxWaterDepthForDeepSeaBed -> skip
        dsc.SetVertexFlagsForTest(36, 0x0008); // (3,3) cliff bit
        {
            const auto makeInfo = [](Sint32 spread, Float32 param) {
                TerrainSurfaceCoverInfo info;
                info.multiplier = 1.0f;
                info.spread = {spread};
                info.curveParam = {param};
                info.curves.resize(1);
                info.curves[0].points = {{0.0f, 1.0f}};
                return info;
            };
            cSTETerrain::SurfaceCoverTextures()[19u] = makeInfo(1, 4.0f);
            cSTETerrain::SurfaceCoverTextures()[20u] = makeInfo(1, 4.0f);
            cSTETerrain::SurfaceCoverTextures()[0xbeefu] = makeInfo(0, 0.0f);
            cSTETerrain::SurfaceCoverTextures()[0x77u] = makeInfo(1, 4.0f);
            cSTETerrain::SurfaceCoverTextures()[0x66u] = makeInfo(1, 4.0f);
        }
        dsc.ComputeWaterProximityValues(SC4RectI{0, 0, 10, 10});
        dsc.DecideSurfaceCover(SC4RectI{3, 3, 7, 7});
        Check(dsc.LandTypeSlotAt(36, 0).type == 0x1234 &&
                  dsc.LandTypeSlotAt(36, 0).value == 12751, // int(50*255+1)
              "DecideSurfaceCover cliff bit 0x8 -> low-cliff tex id (low 16 bits) with 50.0f value @ 0x10073409a");
        Check(dsc.LandTypeSlotAt(60, 0).type == 19 && dsc.LandTypeSlotAt(60, 0).value == 256,
              "DecideSurfaceCover deep water packs texId 19 via (variant<<12)|id (OR) and the spread loop adds it @ 0x10073400a..0x100734083");
        Check(dsc.LandTypeSlotAt(84, 0).type == 20 && dsc.LandTypeSlotAt(84, 0).value == 256,
              "DecideSurfaceCover shallow water (depth < 15) uses texId 20 @ 0x100734017");
        Check(dsc.LandTypeSlotAt(72, 0).type == 0,
              "DecideSurfaceCover skips vertices deeper than kMaxWaterDepthForDeepSeaBedTexture @ 0x100733fe9");
        Check(dsc.LandTypeSlotAt(48, 0).type == 0xbeef &&
                  dsc.LandTypeSlotAt(48, 0).value == 256, // int(1.0*255+1)
              "DecideSurfaceCover beach center adds its land type with the 1.0f value @ 0x1007341c3");
        // Temperature/moisture path: an INLAND vertex (proximity 6 > 4) reads
        // sSurfaceCoverMap[terrainType*512 + tempCode*16 + moisture].
        cSTETerrain::SurfaceCoverMap().assign(512, 0);
        cSTETerrain::SurfaceCoverMap()[499] = 0x77; // tempCode 31 (alt <= beachAlt)
        cSTETerrain::SurfaceCoverMap()[451] = 0x66; // tempCode 28 (alt 250 -> code 3)
        dsc.DecideSurfaceCover(SC4RectI{0, 0, 0, 0});
        Check(dsc.LandTypeSlotAt(0, 0).type == 0x77 && dsc.LandTypeSlotAt(0, 0).value == 256,
              "DecideSurfaceCover inland low vertex: tempCode 31, moisture 3 -> map[499] @ 0x100734293");
        dsc.SetAltitude(0, 250.0f);
        dsc.DecideSurfaceCover(SC4RectI{0, 0, 0, 0});
        Check(dsc.LandTypeSlotAt(0, 1).type == 0x66 && dsc.LandTypeSlotAt(0, 1).value == 256,
              "DecideSurfaceCover altitude 250 -> tempCode 31-3 = 28 -> map[451] @ 0x10073423d");

        // ---- AssignSurfaceTexturesAndColors @ 0x100733bc8 (call order,
        //      in-place padding, view gate) ----
        cSTETerrain asc;
        Check(asc.Init(2, 2), "Assign fixture Init(2,2)");
        asc.SetSeaLevelForTest(0.0f);
        cSTETerrain::MaxSurfaceTextureSpread() = {2};
        cSTETerrain::BeachTexResIds().clear(); // no beach entry -> no land types
        TerrainMoistureSource ms2;
        asc.SetMoistureSource(&ms2);
        asc.SetTextureSpanInfoDirtyGate(true);
        asc.SetLandTypeSlotForTest(4, 0, 7, 100); // will be cleared
        cSTETerrain::View3DObservation().Reset();
        SC4RectI ascRect{0, 0, 1, 1};
        asc.AssignSurfaceTexturesAndColors(ascRect);
        Check(ascRect.left == 0 && ascRect.top == 0 && ascRect.right == 2 &&
                  ascRect.bottom == 2,
              "AssignSurfaceTexturesAndColors pads the rect in place by snMaxSurfaceTextureSpread @ 0x100733bf1");
        Check(ms2.updateCallCount == 1 && ms2.lastUpdateRect[0] == 0 &&
                  ms2.lastUpdateRect[1] == 0 && ms2.lastUpdateRect[2] == 32 &&
                  ms2.lastUpdateRect[3] == 32,
              "UpdateMoistureValues receives the padded rect * 16 (cell width) @ 0x100733d98..0x100733dc7");
        Check(asc.LandTypeSlotAt(4, 0).type == 0,
              "ClearSurfaceTextures zeroes the slot types before DecideSurfaceCover @ 0x100733c01..0x100733c13");
        Check(cSTETerrain::View3DObservation().markTextureSpanInfoDirtyCalls == 1 &&
                  cSTETerrain::View3DObservation().markTextureSpanInfoDirtyLastLo == 0 &&
                  cSTETerrain::View3DObservation().markTextureSpanInfoDirtyLastHi == 1,
              "AssignSurfaceTexturesAndColors MarkTextureSpanInfoDirty((top>0)?top-1:0, min(bottom+1, maxVertexIndexZ)) gated on +0x118 @ 0x100733c0c..0x100733c2e");
        asc.SetTextureSpanInfoDirtyGate(false);
        asc.AssignSurfaceTexturesAndColors(ascRect);
        Check(cSTETerrain::View3DObservation().markTextureSpanInfoDirtyCalls == 1,
              "AssignSurfaceTexturesAndColors +0x118 gate off -> no view call @ 0x100733c13");

        // ---- cleanup (kTestBrushId style): restore every settable static
        //      to its shipped default so no later check observes leftovers ----
        cSTETerrain::LowCliffTexResIds().clear();
        cSTETerrain::BeachTexResIds().clear();
        cSTETerrain::MaxSurfaceTextureSpread().clear();
        cSTETerrain::TextureRandomizationGridSize().clear();
        cSTETerrain::TextureGridCellCountX().clear();
        cSTETerrain::TextureFamilyMemberForGrid().clear();
        cSTETerrain::TerrainTextureBindingCounts().clear();
        cSTETerrain::SurfaceCoverMap().clear();
        cSTETerrain::SurfaceCoverTextures().clear();
        cSTETerrain::TexturesUsedInTerrainType().clear();
        cSTETerrain::SurfaceCoverStringHashHook() = nullptr;
        cSTETerrain::TemperatureRangeFactor() = std::bit_cast<Float32>(0x0000000au);
        cSTETerrain::MaxTerrainHeightTunable() = 200.0f;
        cSTETerrain::MinTerrainHeightTunable() = 5.0f;
        cSTETerrain::View3DObservation().Reset();
    }

    // ------------------------------------------------------------------
    // cSTETerrain shore-cliff & normal-assignment cluster (M3 terrain wave)
    // (CalculateNormalsAndAssignCliffTextures @ 0x100734dc8, MarkCliff @
    // 0x100735398, AnalyzeShoreAndCreateCliffs @ 0x1007317fe, CreateShoreCliffs
    // @ 0x100736d3e, and the exact vtable bodies SetAltitude @ 0x10073e192,
    // SetSeaLevel @ 0x10073e556, FlipInternalTriangulation @ 0x100738848)
    // ------------------------------------------------------------------
    std::printf("\n--- cSTETerrain shore-cliff & normals cluster ---\n");
    {
        // ---- MarkCliff @ 0x100735398: the three mark paths ----
        cSTETerrain mc;
        Check(mc.Init(8, 8), "MarkCliff fixture Init(8,8) @ 0x100739cb0");
        auto mvidx = [](Sint32 x, Sint32 z) { return z * 9 + x; };
        mc.SetTextureSpanInfoDirtyGate(true);
        // Standard path (!a || !b) @ 0x1007354cf: first marks
        // {v, v+stride, v+stride+1}, second marks {v, v+1, v+stride+1}.
        mc.MarkCliff(mvidx(2, 2), true, false);
        Check((mc.VertexFlags(mvidx(2, 2)) & 0x8) != 0 &&
                  (mc.VertexFlags(mvidx(2, 3)) & 0x8) != 0 &&
                  (mc.VertexFlags(mvidx(3, 3)) & 0x8) != 0 &&
                  (mc.VertexFlags(mvidx(3, 2)) & 0x8) == 0,
              "MarkCliff first-bool-only marks {v, v+stride, v+stride+1} @ 0x1007354d3..0x100735513");
        mc.MarkCliff(mvidx(5, 2), false, true);
        Check((mc.VertexFlags(mvidx(5, 2)) & 0x8) != 0 &&
                  (mc.VertexFlags(mvidx(6, 2)) & 0x8) != 0 &&
                  (mc.VertexFlags(mvidx(6, 3)) & 0x8) != 0 &&
                  (mc.VertexFlags(mvidx(5, 3)) & 0x8) == 0,
              "MarkCliff second-bool-only marks {v, v+1, v+stride+1} @ 0x100735520..0x10073555c");
        // Mixed path (a && b, not both steep) @ 0x100735566: flat ground
        // gives nyA = nyB = 1 (> kMaxNormalYForCliff 0.3f) -> not steep;
        // the flip bit 0x800 is set, FlipInternalTriangulation(x, z, true)
        // runs, and no 0x8 marks happen.
        const Sint32 flipCallsBefore = cSTETerrain::View3DObservation().setTriangleIndicesCalls;
        mc.MarkCliff(mvidx(4, 4), true, true);
        Check((mc.VertexFlags(mvidx(4, 4)) & 0x800) != 0 &&
                  (mc.VertexFlags(mvidx(4, 4)) & 0x8) == 0,
              "MarkCliff mixed path on flat ground sets 0x800, no 0x8 @ 0x10073559c");
        Check(cSTETerrain::View3DObservation().setTriangleIndicesCalls == flipCallsBefore + 1 &&
                  cSTETerrain::View3DObservation().setTriangleIndicesLastArg0 == 4 &&
                  cSTETerrain::View3DObservation().setTriangleIndicesLastArg1 == 4,
              "MarkCliff mixed path flips triangulation via vtable+0x2f0 with (z, x) @ 0x100735589/0x1007388ac");
        Check((mc.VertexFlags(mvidx(4, 4)) & 0x1000) != 0,
              "FlipInternalTriangulation sets flag bit 0x1000 @ 0x100738899");
        const Sint32 flipCallsBefore2 = cSTETerrain::View3DObservation().setTriangleIndicesCalls;
        mc.FlipInternalTriangulation(4, 4, true);
        Check(cSTETerrain::View3DObservation().setTriangleIndicesCalls == flipCallsBefore2,
              "FlipInternalTriangulation early-outs when the bit already matches @ 0x100738892");
        mc.FlipInternalTriangulation(9, 4, true); // x >= cellCountX
        mc.FlipInternalTriangulation(4, 9, true); // z >= cellCountZ
        Check((mc.VertexFlags(mvidx(4, 4)) & 0xefff) == (mc.VertexFlags(mvidx(4, 4)) & 0xefff),
              "FlipInternalTriangulation cell bounds gates @ 0x10073884e..0x100738856");
        // Both-steep path @ 0x1007354c0: cliff walls give
        // |nyA| = |nyB| = 256/sqrt(960^2 + 65536) ~ 0.258 < 0.3f -> both
        // steep -> the standard marks apply and no flip happens.
        cSTETerrain mc2;
        mc2.Init(8, 8);
        mc2.SetTextureSpanInfoDirtyGate(true);
        mc2.SetAltitude(mvidx(4, 4), 60.0f);   // v
        mc2.SetAltitude(mvidx(5, 4), 60.0f);   // v+1
        mc2.SetAltitude(mvidx(4, 5), 0.0f);    // v+stride
        mc2.SetAltitude(mvidx(5, 5), 60.0f);   // v+stride+1
        const Sint32 flipCallsBefore3 = cSTETerrain::View3DObservation().setTriangleIndicesCalls;
        mc2.MarkCliff(mvidx(4, 4), true, true);
        Check((mc2.VertexFlags(mvidx(4, 4)) & 0x8) != 0 &&
                  (mc2.VertexFlags(mvidx(5, 4)) & 0x8) != 0 &&
                  (mc2.VertexFlags(mvidx(4, 5)) & 0x8) != 0 &&
                  (mc2.VertexFlags(mvidx(5, 5)) & 0x8) != 0 &&
                  (mc2.VertexFlags(mvidx(4, 4)) & 0x800) == 0 &&
                  cSTETerrain::View3DObservation().setTriangleIndicesCalls == flipCallsBefore3,
              "MarkCliff both-steep marks the full quad without flipping @ 0x1007354c0..0x1007354c9");
        // One-steep mixed marks: steepA marks {v, v+stride, v+1} (note v+1,
        // NOT v+stride+1).
        cSTETerrain mc3;
        mc3.Init(8, 8);
        mc3.SetTextureSpanInfoDirtyGate(true);
        mc3.SetAltitude(mvidx(4, 4), 60.0f); // v
        mc3.SetAltitude(mvidx(5, 4), 0.0f);  // v+1
        mc3.SetAltitude(mvidx(4, 5), 0.0f);  // v+stride
        mc3.SetAltitude(mvidx(5, 5), 0.0f);  // v+stride+1
        mc3.MarkCliff(mvidx(4, 4), true, true);
        Check((mc3.VertexFlags(mvidx(4, 4)) & 0x8) != 0 &&
                  (mc3.VertexFlags(mvidx(4, 5)) & 0x8) != 0 &&
                  (mc3.VertexFlags(mvidx(5, 4)) & 0x8) != 0 &&
                  (mc3.VertexFlags(mvidx(5, 5)) & 0x8) == 0 &&
                  (mc3.VertexFlags(mvidx(4, 4)) & 0x800) != 0,
              "MarkCliff mixed path steepA marks {v, v+stride, v+1} @ 0x1007355aa..0x1007355d8");
        // steepB-only mixed marks: {v+1, v+stride, v+stride+1} — the middle
        // store @ 0x100735604 uses r14d which still holds v on the mixed
        // path (fresh asm: incl %r14d @ 0x10073553e is on the both-steep
        // route only), so v+stride IS marked and v is NOT.
        cSTETerrain mc4;
        mc4.Init(8, 8);
        mc4.SetTextureSpanInfoDirtyGate(true);
        mc4.SetAltitude(mvidx(4, 4), 0.0f);  // v
        mc4.SetAltitude(mvidx(5, 4), 0.0f);  // v+1
        mc4.SetAltitude(mvidx(4, 5), 0.0f);  // v+stride
        mc4.SetAltitude(mvidx(5, 5), 60.0f); // v+stride+1 (dh2 = 60 -> steepB)
        mc4.MarkCliff(mvidx(4, 4), true, true);
        Check((mc4.VertexFlags(mvidx(5, 4)) & 0x8) != 0 &&
                  (mc4.VertexFlags(mvidx(4, 5)) & 0x8) != 0 &&
                  (mc4.VertexFlags(mvidx(5, 5)) & 0x8) != 0 &&
                  (mc4.VertexFlags(mvidx(4, 4)) & 0x8) == 0 &&
                  (mc4.VertexFlags(mvidx(4, 4)) & 0x800) != 0,
              "MarkCliff mixed path steepB marks {v+1, v+stride, v+stride+1} @ 0x1007355ef..0x100735618");

        // ---- TrackShoreWithLandToTheRight step-dir arithmetic @ 0x100739759 ----
        // Concave L-corner: land {(0,5), (1,5), (2,6)}. The Right walker
        // steps E onto (1,5) with seaDir = N(2); the correct next probe dir
        // is (2+6)&7 = W — the marked start vertex (land for the walker) —
        // so the walk stops after one step. The buggy (seaDir+6)&7 order
        // (= (2+3)&7 = SE) would step onto the unmarked shore vertex (2,6)
        // instead.
        cSTETerrain lcorner;
        lcorner.Init(8, 8);
        lcorner.SetSeaLevelForTest(10.0f);
        lcorner.SetAltitude(45, 20.0f); // (0, 5)
        lcorner.SetAltitude(46, 20.0f); // (1, 5)
        lcorner.SetAltitude(56, 20.0f); // (2, 6)
        lcorner.DetectShoreVertices();
        std::deque<Sint32> lwalk;
        lcorner.GetShoreStartingAtVert(45, lwalk, 0x1);
        Check(lwalk.size() == 2 && lwalk[0] == 45 && lwalk[1] == 46 &&
                  (lcorner.VertexFlags(56) & 0x1) == 0,
              "Right walker next-dir = (pre-update seaDir + 6) & 7 probes the marked start and stops @ 0x100739759..0x100739764");

        // ---- TrackShoreWithLandToTheLeft seaDir==0 remap @ 0x100739908 ----
        // Northern coastline: land z <= 4, sea z >= 5. From (0,4) the Left
        // walker steps E (dir 4) eight times; every step gives
        // newSeaDir = (4+4)&7 = 0, so the remap must produce dir 7 (SW)
        // (a plain -1 would read the offset tables out of bounds). The
        // exact sequence is the full row prepended east-to-west.
        cSTETerrain ncoast;
        ncoast.Init(8, 8);
        ncoast.SetSeaLevelForTest(10.0f);
        for (Sint32 z = 0; z <= 4; ++z) {
            for (Sint32 x = 0; x <= 8; ++x) {
                ncoast.SetAltitude(z * 9 + x, 20.0f);
            }
        }
        ncoast.DetectShoreVertices();
        std::deque<Sint32> nwalk;
        ncoast.GetShoreStartingAtVert(36, nwalk, 0x1); // (0, 4)
        bool seqOk = nwalk.size() == 9;
        for (Sint32 i = 0; i < 9 && seqOk; ++i) {
            if (nwalk[static_cast<std::size_t>(i)] != 44 - i) seqOk = false;
        }
        Check(seqOk,
              "Left walker seaDir==0 remap walks the full northern shoreline east @ 0x1007398fa..0x100739911");

        // ---- CalculateNormalsAndAssignCliffTextures @ 0x100734dc8 ----
        // Flat ground: every quad normal is exactly (0, 1, 0) (len 256,
        // ny = 1) -> no MarkCliff calls; the vertex normals are (0, 1, 0).
        cSTETerrain flat;
        flat.Init(4, 4); // 5x5 vertices, stride 5
        for (Sint32 i = 0; i < 25; ++i) flat.SetAltitude(i, 10.0f);
        flat.CalculateNormalsAndAssignCliffTextures(SC4RectI{1, 1, 3, 3});
        const Float32* nFlat = flat.VertexNormalAt(7); // (2, 2)
        Check(nFlat != nullptr && nFlat[0] == 0.0f && nFlat[1] == 1.0f && nFlat[2] == 0.0f,
              "flat quad normals normalize to exactly (0,1,0) @ 0x100734f7d..0x100734fb3");
        const Float32* nRow0 = flat.VertexNormalAt(2); // (2, 0) — padded top row
        Check(nRow0 != nullptr && nRow0[0] == 0.0f && nRow0[1] == 0.0f,
              "padded top row gets quad normals but NO vertex normals @ 0x100735096");
        // Stepped terrain (row 0 at 100, rest at 0): quad row 0 has
        // nB = (-1600, 256, 1600)/len -> |nyB| ~ 0.112 < 0.3 -> cliff B ->
        // MarkCliff(x, false, true) marks {(x,0), (x+1,0), (x+1,1)}.
        cSTETerrain step;
        step.Init(4, 4);
        for (Sint32 x = 0; x <= 4; ++x) step.SetAltitude(x, 100.0f);
        for (Sint32 i = 5; i < 25; ++i) step.SetAltitude(i, 0.0f);
        step.CalculateNormalsAndAssignCliffTextures(SC4RectI{0, 0, 4, 4});
        auto svidx = [](Sint32 x, Sint32 z) { return z * 5 + x; };
        Check((step.VertexFlags(svidx(0, 0)) & 0x8) != 0 &&
                  (step.VertexFlags(svidx(4, 0)) & 0x8) != 0 &&
                  (step.VertexFlags(svidx(1, 1)) & 0x8) != 0 &&
                  (step.VertexFlags(svidx(4, 1)) & 0x8) != 0 &&
                  (step.VertexFlags(svidx(0, 1)) & 0x8) == 0 &&
                  (step.VertexFlags(svidx(2, 2)) & 0x8) == 0,
              "steep B-triangulation quads mark the cliff band via MarkCliff @ 0x100735039..0x100735050");
        const Float32* nStep = step.VertexNormalAt(svidx(2, 2));
        Check(nStep != nullptr && nStep[0] == 0.0f && nStep[1] == 1.0f && nStep[2] == 0.0f,
              "interior vertex over flat quads sums to (0,1,0) @ 0x1007350dd..0x10073523d");
        const Float32* nEdge = step.VertexNormalAt(svidx(1, 1));
        const Float32 lenSq = nEdge != nullptr
                                  ? nEdge[0] * nEdge[0] + nEdge[1] * nEdge[1] + nEdge[2] * nEdge[2]
                                  : -1.0f;
        Check(lenSq > 0.9999f && lenSq < 1.0001f,
              "vertex normals are unit length after the zero-guard normalize @ 0x1007351e1..0x100735231");
        // ClearVertexPropertyFlags(rect, mask) @ 0x100735288: inclusive
        // [left, right] columns, rows [top, bottom].
        cSTETerrain cpf;
        cpf.Init(4, 4);
        for (Sint32 i = 0; i < 25; ++i) cpf.SetVertexFlagsForTest(i, 0xffffu);
        cpf.ClearVertexPropertyFlags(SC4RectI{2, 2, 3, 2}, 0x00ffu);
        Check((cpf.VertexFlags(svidx(2, 2)) & 0xff) == 0 &&
                  (cpf.VertexFlags(svidx(3, 2)) & 0xff) == 0 &&
                  (cpf.VertexFlags(svidx(4, 2)) & 0xff) != 0 &&
                  (cpf.VertexFlags(svidx(2, 1)) & 0xff) != 0 &&
                  (cpf.VertexFlags(svidx(2, 2)) & 0xff00) != 0,
              "ClearVertexPropertyFlags(rect, mask) clears [left,right] x [top,bottom] inclusive @ 0x100735288");
        cpf.ClearVertexPropertyFlags(SC4RectI{0, 0, 4, 4}, 0xff00u);
        Check(cpf.VertexFlags(svidx(2, 2)) == 0 &&
                  (cpf.VertexFlags(svidx(0, 0)) & 0xff00) == 0,
              "ClearVertexPropertyFlags(rect, mask) second pass clears the high byte everywhere @ 0x100735288");

        // ---- shore walkers + CreateShoreCliffs end-to-end @ 0x100736d3e ----
        // Straight coastline: columns 0..2 sea (alt 0), 3..8 land (alt 20),
        // sea level 10. The shore column is x=3; the Left walker prepends
        // the whole shoreline, every shore vertex qualifies (probe walk
        // stays inland: sum = 10 + 5*10 = 60 > 15), so one CreateShoreCliff
        // runs over all 9 candidates.
        cSTETerrain shore;
        Check(shore.Init(8, 8), "shore fixture Init(8,8)");
        shore.SetSeaLevelForTest(10.0f);
        for (Sint32 z = 0; z <= 8; ++z) {
            for (Sint32 x = 0; x <= 8; ++x) {
                shore.SetAltitude(z * 9 + x, (x <= 2) ? 0.0f : 20.0f);
            }
        }
        cSTETerrain::sRand.seedA = 12345u;
        cSTETerrain::sRand.seedB = 6789u; // nonzero: no clock init
        const bool created = shore.CreateShoreCliffs(true);
        Check(created, "CreateShoreCliffs creates a cliff from a straight coastline @ 0x100736d3e");
        bool shoreMarked = true, seaMarked = false, probesRaised = true;
        for (Sint32 z = 0; z <= 8; ++z) {
            if ((shore.VertexFlags(z * 9 + 3) & 0x4) == 0) shoreMarked = false;
            if ((shore.VertexFlags(z * 9 + 2) & 0x4) == 0) shoreMarked = false;
            if (shore.GetAltitude(z * 9 + 3) <= 20.0f) probesRaised = false;
            if ((shore.VertexFlags(z * 9 + 1) & 0x4) != 0) seaMarked = true;
        }
        Check(shoreMarked, "CreateShoreCliff ORs 0x4 onto seaNeighbor and vertex @ 0x100731b67/0x100731b7c");
        Check(probesRaised, "CreateShoreCliff raises the shore vertices above the old land @ 0x100731bda");
        Check(!seaMarked, "CreateShoreCliff only marks the walked endpoints @ 0x100731ab0");
        bool probeRaised = true;
        for (Sint32 z = 0; z <= 8; ++z) {
            for (Sint32 x = 4; x <= 8; ++x) {
                if (shore.GetAltitude(z * 9 + x) <= 20.0f) probeRaised = false;
            }
        }
        Check(probeRaised,
              "CreateShoreCliff raises the five inland probes (base 40.1..50.1 + sea 10, jitter -8..+8) @ 0x100731bf6..0x100731c21");
        // MarkRegion(8, IsVertexWithMark3, IsLandAndNotMark3, 0x2): the
        // 0x4-marked land column expands mark 0x2 over the whole land side
        // within radius 8 (clamped to the grid).
        Check((shore.VertexFlags(4 * 9 + 5) & 0x2) != 0 &&
                  (shore.VertexFlags(8 * 9 + 8) & 0x2) != 0 &&
                  (shore.VertexFlags(4 * 9 + 1) & 0x2) == 0,
              "MarkRegion(8, IsVertexWithMark3, IsLandAndNotMark3, 0x2) spreads mark 0x2 over land @ 0x100736e26..0x100736e48");
        Check(shore.ReDisplayCallCount() == 1,
              "CreateShoreCliffs(redisplay=true) calls vtable+0x228 once @ 0x100736e76..0x100736e93");
        bool cliff8Cleared = true;
        for (Sint32 i = 0; i < 81; ++i) {
            if ((shore.VertexFlags(i) & 0x8) != 0) cliff8Cleared = false;
        }
        Check(cliff8Cleared, "CreateShoreCliffs tail clears flag bit 0x8 over every vertex @ 0x100736ea9..0x100736ec3");
        // Second run: the shoreline is now far above sea level and no longer
        // shore, so nothing is created and the redisplay is skipped.
        shore.SetSeaLevelForTest(10.0f);
        const Sint32 redisplayBefore = shore.ReDisplayCallCount();
        Check(!shore.CreateShoreCliffs(true) && shore.ReDisplayCallCount() == redisplayBefore,
              "CreateShoreCliffs returns false with no redisplay when no shore exists @ 0x100736e22");
        // GetNeighboringSeaVertex @ 0x100731a1c on the raw fixture.
        cSTETerrain gns;
        gns.Init(8, 8);
        gns.SetSeaLevelForTest(10.0f);
        for (Sint32 z = 0; z <= 8; ++z) {
            for (Sint32 x = 0; x <= 8; ++x) {
                gns.SetAltitude(z * 9 + x, (x <= 2) ? 0.0f : 20.0f);
            }
        }
        Check(gns.GetNeighboringSeaVertex(3 * 9 + 3) == 3 * 9 + 2 &&
                  gns.GetNeighboringSeaVertex(8 * 9 + 8) == -1,
              "GetNeighboringSeaVertex returns the first sea cross-neighbor (W first) or -1 @ 0x100731a1c");
        Check(gns.IsShoreVertex(3 * 9 + 3) && !gns.IsShoreVertex(3 * 9 + 4) &&
                  !gns.IsShoreVertex(3 * 9 + 2),
              "IsShoreVertex: land with a sea cross-neighbor only @ 0x10073751a");
        // GetShoreStartingAtVert @ 0x100736f5e: DetectShoreVertices sets the
        // shore bit first; from the north shoreline corner (x=3, z=0) the
        // Left walker prepends the whole shoreline south (the Right walker
        // stops at the eastern land immediately).
        gns.DetectShoreVertices();
        std::deque<Sint32> walk;
        gns.GetShoreStartingAtVert(3, walk, 0x1); // vertex (x=3, z=0)
        Check(walk.size() == 9 && walk.front() == 8 * 9 + 3 && walk.back() == 3,
              "GetShoreStartingAtVert walks the full shoreline (Left prepends, Right stops) @ 0x100736f5e..0x100739928");
        bool allWalkMarked = true;
        for (Sint32 v : walk) {
            if ((gns.VertexFlags(v) & 0x1) == 0) allWalkMarked = false;
        }
        Check(allWalkMarked, "the shore walk marks every visited vertex with the mark bit @ 0x1007398c4");

        // ---- sea/terrain level changes + redisplay min/max ----
        cSTETerrain lvl;
        lvl.Init(4, 4);
        for (Sint32 i = 0; i < 25; ++i) lvl.SetAltitude(i, 10.0f);
        lvl.ChangeTerrainLevel(true, 2.5f);
        bool allRaised = true;
        for (Sint32 i = 0; i < 25; ++i) {
            if (lvl.GetAltitude(i) != 12.5f) allRaised = false;
        }
        Check(allRaised && lvl.ReDisplayCallCount() == 1,
              "ChangeTerrainLevel(true, 2.5) raises every vertex via RaiseVertex + one redisplay @ 0x100735746");
        lvl.ChangeTerrainLevel(false, 2.5f);
        bool allLowered = true;
        for (Sint32 i = 0; i < 25; ++i) {
            if (lvl.GetAltitude(i) != 10.0f) allLowered = false;
        }
        Check(allLowered && lvl.ReDisplayCallCount() == 2,
              "ChangeTerrainLevel(false, 2.5) lowers every vertex via LowerVertex + one redisplay @ 0x100735746");
        // ChangeSeaLevel/SetSeaLevel @ 0x100735714/0x10073e556.
        lvl.SetSeaLevelForTest(10.0f);
        Check(lvl.ChangeSeaLevel(true) && lvl.GetSeaLevel() == 30.0f,
              "ChangeSeaLevel(true) applies seaLevel + kSeaRaiseLowerAmount(20) @ 0x100735714");
        Check(lvl.SurfaceWater().resetToSeaLevelCalls == 1 &&
                  lvl.SurfaceWater().resetToSeaLevelLastRect[0] == 0 &&
                  lvl.SurfaceWater().resetToSeaLevelLastRect[2] == 4,
              "SetSeaLevel resets the surface water over the cached world rect (+0x58) @ 0x10073e584");
        Check(lvl.PostMessageACalls() == 1 && lvl.PostMessageALastId() == 0x89171496u,
              "SetSeaLevel posts message 0x89171496 (recorded no-op) @ 0x10073e590");
        Check(!lvl.SetSeaLevel(300.0f) && lvl.GetSeaLevel() == 30.0f,
              "SetSeaLevel range gate rejects > kMaxSeaLevel(200.1) @ 0x10073e56b..0x10073e576");
        // kMinSeaLevel @ 0x100b175d4 = bit pattern 0xc121999a = -10.1f:
        // -10.0f is INSIDE the range (accepted, with the reset/post/redisplay
        // side effects), while -11.0f is rejected.
        Check(lvl.SetSeaLevel(-10.0f) && lvl.GetSeaLevel() == -10.0f &&
                  lvl.SurfaceWater().resetToSeaLevelCalls == 2,
              "SetSeaLevel accepts -10.0f (>= kMinSeaLevel -10.1f) @ 0x10073e562..0x10073e578");
        Check(!lvl.SetSeaLevel(-11.0f) && lvl.GetSeaLevel() == -10.0f,
              "SetSeaLevel range gate rejects < kMinSeaLevel(-10.1f) @ 0x10073e562..0x10073e569");
        lvl.SetSeaLevelForTest(30.0f);
        Check(lvl.ChangeSeaLevel(false) && lvl.GetSeaLevel() == 10.0f,
              "ChangeSeaLevel(false) applies seaLevel - 20 @ 0x100735714");
        // ComputeMinAndMaxAltitudeForTerrainRedisplay @ 0x100735a58.
        cSTETerrain mm;
        mm.Init(4, 4);
        for (Sint32 i = 0; i < 25; ++i) mm.SetAltitude(i, 10.0f);
        mm.SetAltitude(6, 5.0f);     // (1, 1)
        mm.SetAltitude(18, 100.0f);  // (3, 3)
        mm.FindMinAndMaxHeights();
        Check(mm.MinHeight() == 5.0f && mm.MaxHeight() == 100.0f,
              "FindMinAndMaxHeights() full-grid scan writes +0x90/+0x94 @ 0x1007362b4");
        Float32 oMin = -1.0f, oMax = -1.0f;
        mm.FindMinAndMaxHeights(SC4RectI{2, 2, 2, 2}, oMin, oMax);
        Check(oMin == 10.0f && oMax == 10.0f,
              "FindMinAndMaxHeights(rect) do-while scans column `left` even when left == right @ 0x100735c54");
        mm.ComputeMinAndMaxAltitudeForTerrainRedisplay(SC4RectI{0, 0, 2, 2}, oMin, oMax);
        Check(oMin == 0.0f && oMax == 200.0f,
              "no view: candidate max FLT_MAX replaces, then clamps to kMaxTerrainHeight(200) @ 0x100735b45..0x100735c0d");
        TerrainView3DHeightRange vr{30.0f, -5.0f};
        mm.SetView3DHeightRangeForTest(&vr);
        mm.ComputeMinAndMaxAltitudeForTerrainRedisplay(SC4RectI{0, 0, 2, 2}, oMin, oMax);
        Check(oMin == 0.0f && oMax == 30.0f,
              "view3D candidate range: max clamps scanMax+eps(20) down to max(view, mMaxHeight) @ 0x100735bc7..0x100735be8");
        cSTETerrain::ViewUtilitiesModel().present = true;
        cSTETerrain::ViewUtilitiesModel().edge0 = 3; // left edge: rect.left <= 0
        cSTETerrain::ViewUtilitiesModel().edge1 = 2;
        vr.minField98 = 3.0f;
        mm.ComputeMinAndMaxAltitudeForTerrainRedisplay(SC4RectI{0, 1, 2, 2}, oMin, oMax);
        Check(oMin == 0.0f,
              "visible edge zeroes outMin (skips the min adjust) @ 0x100735b23..0x100735b43");
        cSTETerrain::ViewUtilitiesModel().edge0 = 1; // right edge: not visible here
        cSTETerrain::ViewUtilitiesModel().edge1 = 2; // bottom edge: not visible here
        mm.ComputeMinAndMaxAltitudeForTerrainRedisplay(SC4RectI{0, 1, 2, 2}, oMin, oMax);
        Check(oMin == 3.0f,
              "no visible edge: mMinHeight > candidate min -> outMin = candidate min @ 0x100735b98..0x100735bbe");
        // Cached world-rect path (+0x58): rect == worldRect uses the stored
        // min/max instead of scanning.
        mm.SetMinMaxHeight(7.0f, 77.0f);
        mm.SetWorldRectForTest(SC4RectI{0, 0, 2, 2});
        mm.ComputeMinAndMaxAltitudeForTerrainRedisplay(SC4RectI{0, 0, 2, 2}, oMin, oMax);
        // Cached path: outMax starts from mMaxHeight(77) -> 77+20 -> clamps
        // back to max(view 30, 77) = 77; outMin = candidate min 3 (the view
        // range still set from the previous check, no visible edge). A scan
        // would have started from 100 and ended at 100.
        Check(oMin == 3.0f && oMax == 77.0f,
              "rect == cached world rect uses mMinHeight/mMaxHeight, not the scan @ 0x100735a81..0x100735abe");

        // ---- cleanup: restore every settable static to its shipped
        //      default so no later check observes leftovers ----
        cSTETerrain::MinSeaLevelTunable() = -10.1f;
        cSTETerrain::MaxSeaLevelTunable() = 200.1f;
        cSTETerrain::SeaRaiseLowerAmountTunable() = 20.0f;
        cSTETerrain::MinShoreCliffSizeTunable() = 4;
        cSTETerrain::MaxShoreCliffSizeTunable() = 70;
        cSTETerrain::MinShoreCliffCandidateHtSumTunable() = 15.0f;
        cSTETerrain::MinShoreCliffHtTunable() = 40.1f;
        cSTETerrain::MaxShoreCliffHtTunable() = 50.1f;
        cSTETerrain::MaxNormalYForCliffTunable() = 0.3f;
        cSTETerrain::WindErosionFactorTunable() = std::bit_cast<Float32>(0x00000001u);
        cSTETerrain::ViewUtilitiesModel() = TerrainViewUtilitiesModel{};
        cSTETerrain::View3DObservation().Reset();
        cSTETerrain::sRand = RZRandomState{};
    }

    // ------------------------------------------------------------------------
    // cSTETerrain brush description property loading (M3 wave 1):
    // ReadBrushComponentProperties @ 0x10073aafe, ReadBrushDescription @
    // 0x10073aefe, ReadBrushWholeProperties @ 0x10073b26a,
    // ReadBrushUIProperties @ 0x10073b3b8, ReadBrushDescriptions @
    // 0x10073b49c, ReleaseBrushDescriptions @ 0x10073b548,
    // GetCostPerBrushApplication @ 0x100738d76, GetEffectIDProperty @
    // 0x100738dcc.
    // ------------------------------------------------------------------------
    std::printf("\n--- cSTETerrain brush property loading (0x10073aafe..0x10073b548) ---\n");
    {
        static_assert(sizeof(TerrainBrushComponent) == 0x78,
                      "tBrushComponent 0x78 stride (component loop @ 0x10073215f)");

        // ---- GetEffectIDProperty: the exact-8-byte contract ----
        BrushPropertyHolderFixture h;
        Uint32 out[2] = {0xdeadbeefu, 0xcafebabeu};
        Check(!cSTETerrain::GetEffectIDProperty(h, 0x8a020209u, out) &&
                  out[0] == 0xdeadbeefu && out[1] == 0xcafebabeu,
              "GetEffectIDProperty: missing property -> false, out untouched @ 0x100738df1");
        h.SetUintArray(0x8a020209u, {1u, 2u});
        Check(cSTETerrain::GetEffectIDProperty(h, 0x8a020209u, out) &&
                  out[0] == 1u && out[1] == 2u,
              "GetEffectIDProperty: exactly-2-uint vector -> out[0]/out[1] copied @ 0x100738e08..0x100738e12");
        out[0] = 0xdeadbeefu;
        out[1] = 0xcafebabeu;
        h.SetUintArray(0x8a020209u, {1u, 2u, 3u});
        Check(!cSTETerrain::GetEffectIDProperty(h, 0x8a020209u, out) &&
                  out[0] == 0xdeadbeefu,
              "GetEffectIDProperty: 3-uint vector fails the end-begin==8 gate (cmp/jne @ 0x100738e03..0x100738e06)");
        h.SetUint(0x8a020209u, 7u); // wrong kind: uint, not uint-array
        Check(!cSTETerrain::GetEffectIDProperty(h, 0x8a020209u, out) &&
                  out[0] == 0xdeadbeefu,
              "GetEffectIDProperty: kind mismatch -> false, out untouched");

        // ---- GetCostPerBrushApplication: default + truncate ----
        cSTETerrain costTerrain;
        Check(costTerrain.GetCostPerBrushApplication(0x9999u) == 32,
              "GetCostPerBrushApplication: missing brush -> default 32 @ 0x100738db5");
        TerrainBrushDescription costDesc;
        costDesc.costs[0] = 1.9f;
        costDesc.costs[1] = 2.5f;
        costDesc.costs[2] = -7.9f;
        costDesc.costs[3] = 4.5f;
        costDesc.costs[4] = 5.5f;
        cSTETerrain::BrushDescriptionMap()[0x42u] = costDesc;
        costTerrain.SetBrushStrengthIndexForTest(0);
        Check(costTerrain.GetCostPerBrushApplication(0x42u) == 1,
              "GetCostPerBrushApplication: cvttss2si truncates 1.9f -> 1 @ 0x100738dc3");
        costTerrain.SetBrushStrengthIndexForTest(2);
        Check(costTerrain.GetCostPerBrushApplication(0x42u) == -7,
              "GetCostPerBrushApplication: cvttss2si truncates -7.9f -> -7 (toward zero)");
        costTerrain.SetBrushStrengthIndexForTest(99); // out of the 5-entry range
        Check(costTerrain.GetCostPerBrushApplication(0x42u) == 32,
              "GetCostPerBrushApplication: OOB strength index -> model-guard 32 (the binary reads ungated @ 0x100738dc3)");
        cSTETerrain::ReleaseBrushDescriptions();

        // ---- ReadBrushWholeProperties: the 0x102 rotationStep paths ----
        // RNG seed oracle: the modeled RandomDoubleRangeUniform is
        // lo + (hi - lo) * RandomDoubleUniform (terrain.cpp @ 0x10003d3e4
        // model); the oracle recomputes the same single draw. NOTE: a
        // zero-initialized RZRandomState is NOT deterministic —
        // RandomDoubleUniform seeds seedB from std::clock() when it is 0
        // (the original _clock call stub @ 0x10092aff4, rz_random.cpp) — so
        // BOTH sides are pre-seeded with a fixed nonzero seedB.
        constexpr Uint32 kOracleSeedB = 0x5eed1234u;
        auto oracleSeed = [] {
            RZRandomState r{};
            r.seedB = kOracleSeedB;
            return r;
        };
        cSTETerrain::sRand = oracleSeed();
        RZRandomState oracle = oracleSeed();
        const Float32 oracleAccum =
            static_cast<Float32>(6.2831854820251465 * RandomDoubleUniform(oracle));
        TerrainBrushDescription w;
        w.directionAngle = 5.0f;
        BrushPropertyHolderFixture hw;
        hw.SetFloat(0x8a020102u, 30.0f);
        Check(cSTETerrain::ReadBrushWholeProperties(hw, w) && w.useJitter == 1 &&
                  w.rotationStep == 30.0f * 0.01745329238474369f &&
                  w.rotationAccum == oracleAccum && w.directionAngle == 0.0f,
              "Whole: 0x102=30deg -> jitter=1 (setne), deg->rad scale, RNG rotationAccum (seed oracle), directionAngle=0 @ 0x10073b28f..0x10073b2e9");
        // Pre-scale step == 0.0f exactly: accum = 0 with NO RNG draw.
        cSTETerrain::sRand = oracleSeed();
        TerrainBrushDescription w0;
        BrushPropertyHolderFixture hw0;
        hw0.SetFloat(0x8a020102u, 0.0f);
        Check(cSTETerrain::ReadBrushWholeProperties(hw0, w0) && w0.useJitter == 0 &&
                  w0.rotationStep == 0.0f && w0.rotationAccum == 0.0f,
              "Whole: pre-scale step == 0.0f -> jitter=0, rotationAccum=0 (fall-through @ 0x10073b2e1)");
        Check(cSTETerrain::sRand.seedA == 0u && cSTETerrain::sRand.seedB == kOracleSeedB,
              "Whole: zero-step path consumed ZERO RNG draws (sRand state untouched; ucomiss gate @ 0x10073b2a5/b5)");
        // NaN step: jitter=1 via setp, RNG path via jp.
        cSTETerrain::sRand = oracleSeed();
        RZRandomState oracleN = oracleSeed();
        const Float32 accumN =
            static_cast<Float32>(6.2831854820251465 * RandomDoubleUniform(oracleN));
        TerrainBrushDescription wN;
        BrushPropertyHolderFixture hwN;
        hwN.SetFloat(0x8a020102u, std::bit_cast<Float32>(0x7fc00000u));
        Check(cSTETerrain::ReadBrushWholeProperties(hwN, wN) && wN.useJitter == 1 &&
                  wN.rotationAccum == accumN,
              "Whole: NaN step -> jitter=1 (setp) and RNG path (jp @ 0x10073b2b7)");
        // Missing 0x102: jitter=1, bits 0x3c0efa35, RNG ALWAYS called.
        cSTETerrain::sRand = oracleSeed();
        RZRandomState oracleM = oracleSeed();
        const Float32 accumM =
            static_cast<Float32>(6.2831854820251465 * RandomDoubleUniform(oracleM));
        TerrainBrushDescription wM;
        BrushPropertyHolderFixture hwM;
        (void)cSTETerrain::ReadBrushWholeProperties(hwM, wM);
        Check(wM.useJitter == 1 && std::bit_cast<Uint32>(wM.rotationStep) == 0x3c0efa35u &&
                  wM.rotationAccum == accumM,
              "Whole: missing 0x102 -> jitter=1, step bits 0x3c0efa35, RNG always @ 0x10073b2bb..0x10073b2c6");
        // Costs (0x101) + scrub sound ids (0x103/0x104).
        TerrainBrushDescription wC;
        BrushPropertyHolderFixture hwC;
        hwC.SetFloatArray(0x8a020101u, {1.5f, 2.5f, 3.5f, -4.5f, 5.5f});
        hwC.SetUint(0x8a020103u, 0x111u);
        hwC.SetUint(0x8a020104u, 0x222u);
        cSTETerrain::sRand = RZRandomState{}; // missing 0x102 still draws once
        (void)cSTETerrain::ReadBrushWholeProperties(hwC, wC);
        Check(wC.costs[0] == 1.5f && wC.costs[1] == 2.5f && wC.costs[2] == 3.5f &&
                  wC.costs[3] == -4.5f && wC.costs[4] == 5.5f && wC.scrubSoundId == 0x111 &&
                  wC.endScrubSoundId == 0x222,
              "Whole: 0x101 costs = 5 raw dword copies @ 0x10073b31c..0x10073b32a; 0x103/0x104 sound ids");
        TerrainBrushDescription wS;
        BrushPropertyHolderFixture hwS;
        hwS.SetFloatArray(0x8a020101u, {9.0f});
        cSTETerrain::sRand = RZRandomState{};
        (void)cSTETerrain::ReadBrushWholeProperties(hwS, wS);
        Check(wS.costs[0] == 9.0f && wS.costs[1] == 0.0f && wS.costs[4] == 0.0f,
              "Whole: short 0x101 vector -> guarded tail reads 0.0f (the binary copies 5 ungated)");
        TerrainBrushDescription wZ;
        BrushPropertyHolderFixture hwZ;
        cSTETerrain::sRand = RZRandomState{};
        (void)cSTETerrain::ReadBrushWholeProperties(hwZ, wZ);
        Check(wZ.costs[0] == 0.0f && wZ.costs[1] == 0.0f && wZ.costs[2] == 0.0f &&
                  wZ.costs[3] == 0.0f && wZ.costs[4] == 0.0f && wZ.scrubSoundId == 0 &&
                  wZ.endScrubSoundId == 0,
              "Whole: missing 0x101 -> all five costs zeroed @ 0x10073b32e..0x10073b338; missing sound ids -> 0");

        // ---- ReadBrushUIProperties: pair defaults + overrides ----
        TerrainBrushDescription u;
        BrushPropertyHolderFixture hu; // empty holder: every default fires
        Check(cSTETerrain::ReadBrushUIProperties(hu, u) &&
                  u.uiEffectPairs[0] == 0xea3169e6u && u.uiEffectPairs[1] == 2u &&
                  u.uiEffectPairs[2] == 0xea3169e6u && u.uiEffectPairs[3] == 1u &&
                  u.uiEffectPairs[4] == 0xea3169e6u && u.uiEffectPairs[5] == 4u &&
                  u.uiEffectPairs[6] == 0xea3169e6u && u.uiEffectPairs[7] == 3u &&
                  u.uiSoundIdA == 0x071e5700u && u.uiSoundIdB == 0x071e5705u,
              "UI: empty holder -> movabs pair defaults {0xea3169e6,2/1/4/3} + sound defaults @ 0x10073b3da..0x10073b488");
        hu.SetUintArray(0xaa2ed800u, {0xaaaa0001u, 0xaaaa0002u});
        hu.SetUintArray(0xaa2ed801u, {1u, 2u, 3u}); // wrong size -> default
        hu.SetUintArray(0xaa2ed803u, {0xbbbb0001u, 0xbbbb0002u});
        hu.SetUint(0xaa2ed804u, 0x0c0c0c0cu);
        hu.SetUint(0xaa2ed805u, 0x0d0d0d0du);
        Check(cSTETerrain::ReadBrushUIProperties(hu, u) &&
                  u.uiEffectPairs[0] == 0xaaaa0001u && u.uiEffectPairs[1] == 0xaaaa0002u &&
                  u.uiEffectPairs[2] == 0xea3169e6u && u.uiEffectPairs[3] == 1u &&
                  u.uiEffectPairs[4] == 0xea3169e6u && u.uiEffectPairs[5] == 4u &&
                  u.uiEffectPairs[6] == 0xbbbb0001u && u.uiEffectPairs[7] == 0xbbbb0002u &&
                  u.uiSoundIdA == 0x0c0c0c0cu && u.uiSoundIdB == 0x0d0d0d0du,
              "UI: present pairs/sounds copied; 3-uint pair fails the gate -> movabs default");

        // ---- ReadBrushComponentProperties ----
        // Helper: seed the per-component requireds (effect, radii,
        // strengths, bool, and — for non-6/10 — resourceId + flag48).
        auto seedComponent = [](BrushPropertyHolderFixture& h, Uint32 family, Uint32 effect,
                                Uint32 resourceId, Uint8 flag48, bool useDir,
                                std::vector<Float32> radii, std::vector<Float32> strengths) {
            h.SetUint(family + 0x05u, effect);
            h.SetFloatArray(family + 0x02u, std::move(radii));
            h.SetFloatArray(family + 0x03u, std::move(strengths));
            h.SetBool(family + 0x01u, useDir);
            if (effect != 6u && effect != 10u) {
                h.SetUint(family + 0x00u, resourceId);
                h.SetByte(family + 0x04u, flag48);
            }
        };
        // Full effect-4 component (all fields filled).
        BrushPropertyHolderFixture hc;
        seedComponent(hc, 0x8a020200u, 4u, 0xAAu, 7u, true,
                      {10.0f, 20.0f, 30.0f, 40.0f, 50.0f}, {1.0f, 2.0f, 3.0f, 4.0f, 5.0f});
        hc.SetUint(0x8a020208u, 3u);
        hc.SetUintArray(0x8a020209u, {11u, 22u});
        TerrainBrushDescription dc;
        dc.radii.assign(5, 0.0f); // the ReadBrushDescription invariant
        Check(cSTETerrain::ReadBrushComponentProperties(hc, dc) && dc.components.size() == 1,
              "Components: one effect-4 component loads (count>0 return gate @ 0x10073ae63)");
        Check(dc.radii.size() == 5 && dc.radii[0] == 10.0f && dc.radii[4] == 50.0f,
              "Components: elementwise MAX into desc.radii (jbe skip @ 0x10073abd3)");
        {
            const TerrainBrushComponent& c0 = dc.components[0];
            Check(c0.effect == 4 && c0.resourceId == 0xAAu && c0.flag48 == 7 &&
                      c0.useDirectionAngle == 1 && c0.maxDecalCount == 3u &&
                      c0.effectParam58 == 11 && c0.effectParam5c == 22 &&
                      c0.textureSet == nullptr && c0.decalResourceId == 0u &&
                      c0.jitterAmplitude == 6.2831854820251465f && c0.field60.empty() &&
                      c0.radii.size() == 5 && c0.radii[4] == 50.0f &&
                      c0.strengths.size() == 5 && c0.strengths[0] == 1.0f,
                  "Components: effect-4 field fills 0x05/0x02/0x03/0x01/0x00/0x04/0x08/0x09 (byte copied @ 0x10073adc9, field60 vestigial-empty @ 0x10073ae38)");
        }
        // Effect-3 paint: jitterAmp /= 360.0f (NOT /16).
        BrushPropertyHolderFixture h3;
        seedComponent(h3, 0x8a020200u, 3u, 0xBBu, 1u, false,
                      {1.0f, 2.0f, 3.0f, 4.0f, 5.0f}, {0.5f});
        h3.SetUint(0x8a020206u, 0x1234u);
        h3.SetFloat(0x8a020207u, 720.0f);
        h3.SetUint(0x8a020208u, 9u);
        TerrainBrushDescription d3;
        d3.radii.assign(5, 0.0f);
        Check(cSTETerrain::ReadBrushComponentProperties(h3, d3) && d3.components.size() == 1 &&
                  d3.components[0].decalResourceId == 0x1234u &&
                  d3.components[0].jitterAmplitude == 2.0f &&
                  d3.components[0].maxDecalCount == 9u &&
                  d3.components[0].effectParam58 == 0 && d3.components[0].effectParam5c == 0,
              "Components: effect-3 paint -> 0x06 decalId + 0x07 jitterAmp /= 360.0f @ 0x10073ad3f + 0x08 maxDecal");
        // Effects 6/10: no 0x...00/0x...04 reads, forced zeros + 2pi.
        BrushPropertyHolderFixture h6;
        seedComponent(h6, 0x8a020200u, 6u, 0, 0, false,
                      {5.0f, 5.0f, 5.0f, 5.0f, 5.0f}, {1.0f});
        TerrainBrushDescription d6;
        d6.radii.assign(5, 0.0f);
        Check(cSTETerrain::ReadBrushComponentProperties(h6, d6) && d6.components.size() == 1 &&
                  d6.components[0].resourceId == 0u && d6.components[0].flag48 == 0u &&
                  d6.components[0].decalResourceId == 0u &&
                  d6.components[0].jitterAmplitude == 6.2831854820251465f &&
                  d6.components[0].maxDecalCount == 0u &&
                  d6.components[0].effectParam58 == 0 && d6.components[0].effectParam5c == 0,
              "Components: effect 6/10 forces resourceId/flag48/decalId/maxDecal/params 0 + jitterAmp 2pi, without reading 0x...00/0x...04 @ 0x10073ac33..0x10073ac58");
        // Two components (families 0x8a0202xx / 0x8a0203xx), MAX combine.
        BrushPropertyHolderFixture h2;
        seedComponent(h2, 0x8a020200u, 0u, 0x1u, 1u, false,
                      {10.0f, 0.0f, 30.0f, 0.0f, 50.0f}, {1.0f});
        seedComponent(h2, 0x8a020300u, 2u, 0x2u, 2u, true,
                      {5.0f, 20.0f, 25.0f, 0.0f, 60.0f}, {2.0f});
        TerrainBrushDescription d2;
        d2.radii.assign(5, 0.0f);
        Check(cSTETerrain::ReadBrushComponentProperties(h2, d2) && d2.components.size() == 2 &&
                  d2.radii[0] == 10.0f && d2.radii[1] == 20.0f && d2.radii[2] == 30.0f &&
                  d2.radii[3] == 0.0f && d2.radii[4] == 60.0f &&
                  d2.components[1].effect == 2 && d2.components[1].useDirectionAngle == 1,
              "Components: 4-iteration loop shifts the family +0x100 per component (i << 8 @ 0x10073ab34)");
        // All-missing: count == 0 -> false.
        BrushPropertyHolderFixture hNone;
        TerrainBrushDescription dNone;
        Check(!cSTETerrain::ReadBrushComponentProperties(hNone, dNone) &&
                  dNone.components.empty(),
              "Components: no effect properties at all -> count==0 -> false (setg @ 0x10073ae63)");
        // Strengths read failure -> skip this component only (je @
        // 0x10073abfc): the radii MAX already ran and the effect read
        // already counted, so the count>0 gate still returns true with
        // zero components pushed.
        BrushPropertyHolderFixture hSkip;
        hSkip.SetUint(0x8a020205u, 0u);
        hSkip.SetFloatArray(0x8a020202u, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f});
        // 0x8a020203 (strengths) intentionally absent -> skip
        hSkip.SetUint(0x8a020200u, 5u);
        hSkip.SetByte(0x8a020204u, 1u);
        TerrainBrushDescription dSkip;
        dSkip.radii.assign(5, 0.0f);
        Check(cSTETerrain::ReadBrushComponentProperties(hSkip, dSkip) &&
                  dSkip.components.empty() && dSkip.radii[0] == 1.0f,
              "Components: strengths failure skips the component but the radii MAX already ran; return gate is count>0 (skip path @ 0x10073ad4f)");
        // Effect > 0xb -> abort ALL.
        BrushPropertyHolderFixture hBig;
        hBig.SetUint(0x8a020205u, 12u);
        TerrainBrushDescription dBig;
        Check(!cSTETerrain::ReadBrushComponentProperties(hBig, dBig),
              "Components: effect 12 > 0xb aborts all @ 0x10073ab98");
        // No-rollback: i=0 pushed, i=1 radii missing -> false, i=0 STAYS.
        BrushPropertyHolderFixture hRoll;
        seedComponent(hRoll, 0x8a020200u, 0u, 0x1u, 1u, false,
                      {1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, {1.0f});
        hRoll.SetUint(0x8a020305u, 2u); // i=1: effect ok, radii missing
        TerrainBrushDescription dRoll;
        dRoll.radii.assign(5, 0.0f);
        Check(!cSTETerrain::ReadBrushComponentProperties(hRoll, dRoll) &&
                  dRoll.components.size() == 1 && dRoll.components[0].resourceId == 0x1u,
              "Components: required-prop failure aborts with NO rollback — the earlier pushed component stays (r15b=1 path @ 0x10073ad4c)");
        // Per-prop abort classes on a single component.
        BrushPropertyHolderFixture hA;
        hA.SetUint(0x8a020205u, 0u);
        TerrainBrushDescription dA;
        dA.radii.assign(5, 0.0f);
        Check(!cSTETerrain::ReadBrushComponentProperties(hA, dA),
              "Components: missing radii (required) -> abort all @ 0x10073abb7");
        hA.SetFloatArray(0x8a020202u, {1.0f, 1.0f, 1.0f, 1.0f, 1.0f});
        hA.SetFloatArray(0x8a020203u, {1.0f}); // required: the strengths skip (0x10073abfc) precedes the resourceId read
        hA.SetFloatArray(0x8a020203u, {1.0f}); // required: the strengths skip (0x10073abfc) precedes the resourceId read
        Check(!cSTETerrain::ReadBrushComponentProperties(hA, dA),
              "Components: missing resourceId (required, non-6/10) -> abort all @ 0x10073acb2");
        hA.SetUint(0x8a020200u, 1u);
        Check(!cSTETerrain::ReadBrushComponentProperties(hA, dA),
              "Components: missing flag48 0x...04 (required, non-6/10) -> abort all @ 0x10073acfe");
        hA.SetByte(0x8a020204u, 1u);
        Check(cSTETerrain::ReadBrushComponentProperties(hA, dA) && dA.components.size() == 1 &&
                  dA.components[0].useDirectionAngle == 0,
              "Components: missing bool 0x...01 is NON-required (result ignored) — component pushes with useDirectionAngle 0");
        // 0x...04 is a BYTE read: a bool-kind value there fails.
        BrushPropertyHolderFixture hK;
        seedComponent(hK, 0x8a020200u, 0u, 1u, 1u, false,
                      {1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, {1.0f});
        hK.SetBool(0x8a020204u, true); // wrong kind for the byte read
        TerrainBrushDescription dK;
        dK.radii.assign(5, 0.0f);
        Check(!cSTETerrain::ReadBrushComponentProperties(hK, dK),
              "Components: 0x...04 is read as byte — a bool-kind value fails the required read (dual 0x04 decode)");
        // Effect-3 abort classes: 0x06 / 0x07 / 0x08.
        BrushPropertyHolderFixture hP;
        seedComponent(hP, 0x8a020200u, 3u, 1u, 1u, false,
                      {1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, {1.0f});
        TerrainBrushDescription dP;
        dP.radii.assign(5, 0.0f);
        Check(!cSTETerrain::ReadBrushComponentProperties(hP, dP),
              "Components: effect 3 missing 0x...06 decalId -> abort all @ 0x10073ad20");
        hP.SetUint(0x8a020206u, 9u);
        Check(!cSTETerrain::ReadBrushComponentProperties(hP, dP),
              "Components: effect 3 missing 0x...07 jitterAmp -> abort all @ 0x10073ad38");
        hP.SetFloat(0x8a020207u, 360.0f);
        Check(!cSTETerrain::ReadBrushComponentProperties(hP, dP),
              "Components: effects 3/4 missing 0x...08 maxDecalCount -> abort all @ 0x10073ac92");
        hP.SetUint(0x8a020208u, 2u);
        Check(cSTETerrain::ReadBrushComponentProperties(hP, dP) && dP.components.size() == 1 &&
                  dP.components[0].jitterAmplitude == 1.0f,
              "Components: effect 3 completes (360/360 == 1.0)");
        // Effect 4: wrong-size pair -> abort (no zeroed fallback).
        BrushPropertyHolderFixture hQ;
        seedComponent(hQ, 0x8a020200u, 4u, 1u, 1u, false,
                      {1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, {1.0f});
        hQ.SetUint(0x8a020208u, 1u);
        hQ.SetUintArray(0x8a020209u, {1u, 2u, 3u});
        TerrainBrushDescription dQ;
        dQ.radii.assign(5, 0.0f);
        Check(!cSTETerrain::ReadBrushComponentProperties(hQ, dQ),
              "Components: effect 4 wrong-size 0x...09 pair -> abort all @ 0x10073ace1 (NOT a zeroed fallback)");

        // ---- ReadBrushDescription / ReadBrushDescriptions via the fixture source ----
        // THE LIVE-ORACLE LAYOUT-PINNED CHECK (dossier_runtime_tunables.md):
        // the game's first sBrushDescriptionMap node (key 0x11) held
        // jitter=1, rotationStep bits 0x3c0efa35, a 5-float radii vector, one
        // 0x78-stride component, UI pairs {0xea3169e6, 2/1/4/3}, costs[5]
        // zeroed — the signature of a brush exemplar with ONLY the component
        // requireds (no 0x102, no 0x101, no UI props). The synthetic load
        // must match that node bit-for-bit.
        BrushFixtureResourceSource oracleSource;
        BrushPropertyHolderFixture& oh = oracleSource.exemplars[0x11u];
        oh.SetUint(0x8a020205u, 0u);                                    // effect 0
        oh.SetFloatArray(0x8a020202u, {32.0f, 48.0f, 64.0f, 80.0f, 96.0f}); // radii
        oh.SetFloatArray(0x8a020203u, {1.0f, 2.0f, 3.0f, 4.0f, 5.0f});   // strengths
        oh.SetUint(0x8a020200u, 0x27828215u);                           // resourceId
        oh.SetByte(0x8a020204u, 1u);                                     // flag48
        cSTETerrain::BrushResourceSource() = &oracleSource;
        cSTETerrain::sRand = RZRandomState{};
        cSTETerrain::ReadBrushDescriptions();
        {
            const auto& omap = cSTETerrain::BrushDescriptionMap();
            bool oracleOk = omap.size() == 1 && omap.count(0x11u) == 1;
            if (oracleOk) {
                const TerrainBrushDescription& od = omap.find(0x11u)->second;
                oracleOk = od.id == 0x11 && od.useJitter == 1 &&
                           std::bit_cast<Uint32>(od.rotationStep) == 0x3c0efa35u &&
                           od.radii.size() == 5 && od.components.size() == 1 &&
                           od.uiEffectPairs[0] == 0xea3169e6u && od.uiEffectPairs[1] == 2u &&
                           od.uiEffectPairs[2] == 0xea3169e6u && od.uiEffectPairs[3] == 1u &&
                           od.uiEffectPairs[4] == 0xea3169e6u && od.uiEffectPairs[5] == 4u &&
                           od.uiEffectPairs[6] == 0xea3169e6u && od.uiEffectPairs[7] == 3u &&
                           od.costs[0] == 0.0f && od.costs[1] == 0.0f && od.costs[2] == 0.0f &&
                           od.costs[3] == 0.0f && od.costs[4] == 0.0f;
            }
            Check(oracleOk,
                  "LIVE ORACLE (sBrushDescriptionMap key 0x11 @ 0x100b71e20): synthetic load reproduces the game-written node bit-for-bit (jitter=1, step 0x3c0efa35, radii 5, one 0x78 component, UI pairs {0xea3169e6,2/1/4/3}, costs zeroed)");
        }
        // Multi-exemplar iteration + the component-failure erase case in one query.
        BrushFixtureResourceSource multiSource;
        seedComponent(multiSource.exemplars[0x41u], 0x8a020200u, 0u, 0x1u, 1u, false,
                      {1.0f, 2.0f, 3.0f, 4.0f, 5.0f}, {1.0f});
        seedComponent(multiSource.exemplars[0x42u], 0x8a020200u, 6u, 0u, 0u, false,
                      {9.0f, 9.0f, 9.0f, 9.0f, 9.0f}, {1.0f});
        multiSource.exemplars[0x43u].SetUint(0x8a020205u, 0u); // radii missing -> component fail
        cSTETerrain::BrushResourceSource() = &multiSource;
        cSTETerrain::ReadBrushDescriptions();
        {
            const auto& mmap = cSTETerrain::BrushDescriptionMap();
            Check(mmap.size() == 2 && mmap.count(0x41u) == 1 && mmap.count(0x42u) == 1 &&
                      mmap.count(0x43u) == 0,
                  "ReadBrushDescriptions: iterator applies the callback to every query match; the component-failure exemplar is erased (vt+0x38 @ 0x10073b515 / erase @ 0x10073b14e)");
        }
        // ReadBrushDescription direct: insert, skip-if-present, load-failure exit.
        BrushFixtureResourceSource sSource;
        seedComponent(sSource.exemplars[0x21u], 0x8a020200u, 0u, 0x1u, 1u, false,
                      {1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, {1.0f});
        cSTETerrain::ReleaseBrushDescriptions();
        cSTETerrain::BrushResourceSource() = &sSource;
        cSTETerrain::sRand = RZRandomState{};
        cSTETerrain::ReadBrushDescription(SC4PersistResourceKey{0x21u}, nullptr);
        {
            const auto& smap = cSTETerrain::BrushDescriptionMap();
            Check(smap.size() == 1 && smap.count(0x21u) == 1 && sSource.loadCalls == 1 &&
                      smap.find(0x21u)->second.radii.size() == 5 &&
                      smap.find(0x21u)->second.id == 0x21,
                  "ReadBrushDescription: fresh key -> zero-insert, radii resize(5)+zero, id mirror (@ 0x10073b06d/0x10073b0f0/0x10073b112)");
            const RZRandomState before = cSTETerrain::sRand;
            cSTETerrain::ReadBrushDescription(SC4PersistResourceKey{0x21u}, nullptr);
            Check(cSTETerrain::sRand.seedA == before.seedA &&
                      cSTETerrain::sRand.seedB == before.seedB && smap.size() == 1 &&
                      sSource.loadCalls == 1,
                  "ReadBrushDescription: already-present key skips BEFORE the exemplar load (no RNG draws) (tree walk @ 0x10073af25..0x10073af55)");
            cSTETerrain::ReadBrushDescription(SC4PersistResourceKey{0x9999u}, nullptr);
            Check(sSource.loadCalls == 2 && sSource.loadFailures == 1 && smap.size() == 1,
                  "ReadBrushDescription: exemplar load failure exits without inserting (je @ 0x10073af7c)");
        }
        // Erase case: effect present but radii missing -> component false -> map.erase.
        BrushFixtureResourceSource eSource;
        eSource.exemplars[0x31u].SetUint(0x8a020205u, 0u);
        cSTETerrain::BrushResourceSource() = &eSource;
        cSTETerrain::ReadBrushDescription(SC4PersistResourceKey{0x31u}, nullptr);
        Check(cSTETerrain::BrushDescriptionMap().count(0x31u) == 0 && eSource.loadCalls == 1,
              "ReadBrushDescription: component failure erases the fresh entry (__erase_unique @ 0x10073b14e)");
        // Query gate: status 0 -> Release ran, nothing iterated.
        BrushFixtureResourceSource qSource;
        seedComponent(qSource.exemplars[0x51u], 0x8a020200u, 0u, 0x1u, 1u, false,
                      {1.0f, 1.0f, 1.0f, 1.0f, 1.0f}, {1.0f});
        qSource.queryStatus = 0;
        cSTETerrain::BrushDescriptionMap()[0x7777u] = TerrainBrushDescription{}; // stale entry
        cSTETerrain::BrushResourceSource() = &qSource;
        cSTETerrain::ReadBrushDescriptions();
        Check(cSTETerrain::BrushDescriptionMap().empty(),
              "ReadBrushDescriptions: Release clears the map first (@ 0x10073b4b1) and query status 0 iterates nothing (je @ 0x10073b503)");
        qSource.queryStatus = 1;
        cSTETerrain::ReadBrushDescriptions();
        Check(cSTETerrain::BrushDescriptionMap().size() == 1 &&
                  cSTETerrain::BrushDescriptionMap().count(0x51u) == 1,
              "ReadBrushDescriptions: query status != 0 -> apply-callback inserts every exemplar");
        // Null source: full no-op (the map is not even released).
        cSTETerrain::BrushResourceSource() = nullptr;
        cSTETerrain::ReadBrushDescriptions();
        Check(!cSTETerrain::BrushDescriptionMap().empty(),
              "ReadBrushDescriptions: null source (spResMan == 0) is a full no-op before the Release call (je @ 0x10073b4af)");
        // Cleanup: clear map, null source, reset sRand.
        cSTETerrain::ReleaseBrushDescriptions();
        cSTETerrain::BrushResourceSource() = nullptr;
        cSTETerrain::sRand = RZRandomState{};
        Check(cSTETerrain::BrushDescriptionMap().empty(),
              "ReleaseBrushDescriptions clears every node (map reset @ 0x10073b5ce..0x10073b5f1)");
    }

    // ------------------------------------------------------------------------
    // cSTETerrain::ReadTuningParameters @ 0x10073d95e (M3 terrain
    // data-loading wave 2): the tuning-exemplar load, the read order, and
    // the four post-processing quirks — oracle-anchored against the live
    // post-load values captured with a real city loaded
    // (recon/dossier_runtime_tunables.md).
    // ------------------------------------------------------------------------
    std::printf("\n--- cSTETerrain tuning parameter loading (0x10073d95e) ---\n");
    {
        // Restore every tuning global to its shipped __DATA default (fresh
        // byte dumps; the pre-load bytes are what the game itself runs on
        // until the exemplar loads).
        auto restoreTunables = [] {
            cSTETerrain::MinSeaLevelTunable() = -10.1f;               // 0x100b175d4
            cSTETerrain::MaxSeaLevelTunable() = 200.1f;               // 0x100b175d8
            cSTETerrain::SeaRaiseLowerAmountTunable() = 20.0f;        // 0x100b175dc
            cSTETerrain::MinShoreCliffSizeTunable() = 4;              // 0x100b1766c
            cSTETerrain::MaxShoreCliffSizeTunable() = 70;             // 0x100b17670
            cSTETerrain::MinShoreCliffCandidateHtSumTunable() = 15.0f; // 0x100b17674
            cSTETerrain::MinShoreCliffHtTunable() = 40.1f;            // 0x100b17678
            cSTETerrain::MaxShoreCliffHtTunable() = 50.1f;            // 0x100b1767c
            cSTETerrain::MaxNormalYForCliffTunable() = 0.3f;          // 0x100b17680
            cSTETerrain::WindErosionFactorTunable() =
                std::bit_cast<Float32>(0x00000001u);                  // 0x100b71d28
            cSTETerrain::TemperatureRangeFactor() =
                std::bit_cast<Float32>(0x0000000au);                  // 0x100b71db0
            cSTETerrain::MaxTerrainHeightTunable() = 200.0f;          // 0x100b1763c
            cSTETerrain::MinTerrainHeightTunable() = 5.0f;            // 0x100b17640
            cSTETerrain::SeaLevelTunable() =
                std::bit_cast<Float32>(0x00000001u);                 // 0x100b71d48
            cSTETerrain::DefaultTerrainHeightTunable() = 50.0f;      // 0x100b17644
            cSTETerrain::MaxNormalTerrainHeightTunable().clear();     // 0x100b71d30
            cSTETerrain::MinRegularHtFactorTunable().clear();         // 0x100b71d50
            cSTETerrain::MaxRegularHtFactorTunable().clear();         // 0x100b71d68
            cSTETerrain::MinMountainHtFactorTunable().clear();        // 0x100b71d80
            cSTETerrain::MaxMountainHtFactorTunable().clear();        // 0x100b71d98
            cSTETerrain::ImageImportScaleFactorTunable() = 1.5f;      // 0x100b17648
            cSTETerrain::WaterProximityRangeFactorTunable() = 0.0f;   // 0x100b71db4
            cSTETerrain::SeaLevelChangeDueToWaterSliderTunable().clear(); // 0x100b71dc8
            cSTETerrain::NumInitialErosionPassesTunable() = 1;        // 0x100b71db8
            cSTETerrain::NumHydraulicIterationsPerInitialErosionPassTunable() = 334; // 0x100b71dbc
            cSTETerrain::NumWindIterationsPerInitialErosionPassTunable() = 10;       // 0x100b71dc0
            cSTETerrain::MinCellsInUsefulFlatAreaTunable() = 401;     // 0x100b17650
            cSTETerrain::AutoFlatAreaEdgeSmootheningRadiusTunable() = 4; // 0x100b17664
            cSTETerrain::NumSmoothingIterationsAfterWaterErosionTunable() = 1; // 0x100b1764c
            cSTETerrain::EdgeReconcilliationSmoothingIterationsTunable() = 5; // 0x100b17694
            cSTETerrain::MaxHtChangeToFlattenFactorTunable() = 0.1f;  // 0x100b17654
            cSTETerrain::MinTerraceAltitudeFactorTunable() = 0.1f;    // 0x100b17658
            cSTETerrain::MaxTerraceAltitudeFactorTunable() = 0.6f;    // 0x100b1765c
            cSTETerrain::TerrainRaiseLowerAmountTunable() = 20.0f;    // 0x100b175e0
            cSTETerrain::MaxBeachWidthTunable() = 4;                  // 0x100b175e4
            cSTETerrain::MaxBeachAltitudeTunable() = 7.0f;            // 0x100b175e8
            cSTETerrain::MinWaterDepthForDeepSeaBedTextureTunable() = 15.0f; // 0x100b17634
            cSTETerrain::MaxWaterDepthForDeepSeaBedTextureTunable() = 30.0f; // 0x100b17638
            cSTETerrain::AutoFlattenUponCreationTunable() = 0x8au;     // 0x100b71de0
            cSTETerrain::ShoreCliffsUponCreationTunable() = 1;         // 0x100b17668
            cSTETerrain::TypicalBrushAngleIncrementTunable().clear();  // 0x100b71de8
            cSTETerrain::VolcanicAshLifeTimeTunable() = 10000000;      // 0x100b17684
            cSTETerrain::VolcanicAshUpdateIntervalTunable() = 10000000; // 0x100b17688
            cSTETerrain::EdgeReconcilliationHtDifferenceThresholdTunable() = 1.2f; // 0x100b17698
            cSTETerrain::FlowStepsPerErosionApplication().clear();     // 0x100b71e00
            cSTETerrain::TextureRandomizationGridSize().clear();       // 0x100b713f0
        };
        restoreTunables();
        const RZRandomState rngBefore = cSTETerrain::sRand;

        // ---- null source: full no-op (clean-room: no tuning data mounted) ----
        cSTETerrain::TuningResourceSource() = nullptr;
        cSTETerrain::ReadTuningParameters();
        Check(cSTETerrain::MaxTerrainHeightTunable() == 200.0f &&
                  cSTETerrain::DetermineTextureFamilyMembersCallCount() == 0,
              "ReadTuningParameters: null source (SL::spRM unset in clean-room mode) exits before any read @ 0x10073d9a4");

        // ---- load failure: exit before the holder deref ----
        BrushFixtureResourceSource failSource; // no exemplar under key id 1
        cSTETerrain::TuningResourceSource() = &failSource;
        cSTETerrain::ReadTuningParameters();
        Check(failSource.loadCalls == 1 && failSource.loadFailures == 1 &&
                  cSTETerrain::MaxTerrainHeightTunable() == 200.0f &&
                  cSTETerrain::DetermineTextureFamilyMembersCallCount() == 0,
              "ReadTuningParameters: exemplar load failure exits with every global untouched (je @ 0x10073d9a4)");

        // ---- empty holder: every property read fails; the quirks still fire ----
        restoreTunables();
        BrushFixtureResourceSource emptySource;
        (void)emptySource.exemplars[1u]; // present but property-less
        cSTETerrain::TuningResourceSource() = &emptySource;
        cSTETerrain::ReadTuningParameters();
        Check(cSTETerrain::MaxTerrainHeightTunable() == 200.0f &&
                  cSTETerrain::MinTerrainHeightTunable() == 5.0f &&
                  cSTETerrain::MaxSeaLevelTunable() == 200.1f,
              "empty holder: missing properties leave the scalars untouched (GetPropertyValue failure contract)");
        Check(cSTETerrain::SeaLevelTunable() == 5.0f,
              "empty holder quirk (a): still fires off the untouched values — shipped min 5.0 > shipped kSeaLevel denormal slot (bits 0x1) -> 5.0 @ 0x10073da31");
        Check(cSTETerrain::DefaultTerrainHeightTunable() == 50.0f,
              "empty holder quirk (b): does not fire — 5.0 <= shipped default 50.0 (jbe @ 0x10073da5c)");
        Check(cSTETerrain::MinShoreCliffCandidateHtSumTunable() == 90.0f,
              "empty holder quirk (c): fires on the untouched shipped value — 15.0 * 6.0f = 90.0 @ 0x10073dc26 (unconditional mulss)");
        Check(cSTETerrain::VolcanicAshUpdateIntervalTunable() == 500000u,
              "empty holder quirk (e): derives off the untouched lifeTime — 10000000/20 = 500000 @ 0x10073dd65..0x10073dd78");
        Check(cSTETerrain::AutoFlattenUponCreationTunable() == 0 &&
                  cSTETerrain::ShoreCliffsUponCreationTunable() == 0,
              "empty holder: the setne flag stores ALWAYS fire — shipped bytes 0x8a/0x01 are wiped to 0 (pre-zeroed stack-slot model) @ 0x10073dcef/0x10073dd09");
        Check(cSTETerrain::EdgeReconcilliationCurveLoadCount() == 0 &&
                  cSTETerrain::DetermineTextureFamilyMembersCallCount() == 1,
              "empty holder: curve gate not taken (property 0x48cd7c00 absent, je @ 0x10073ddb7) but DetermineTextureFamilyMembersForGridCells ran once @ 0x10073d9cb");
        Check(cSTETerrain::sRand.seedA == rngBefore.seedA &&
                  cSTETerrain::sRand.seedB == rngBefore.seedB,
              "ReadTuningParameters consumes ZERO RNG (no cRZRandom call in the 0x10073d95e body)");

        // ---- oracle-anchored full fixture: the live game's post-load values
        //      (dossier_runtime_tunables.md) plus synthetic values for the
        //      globals the oracle did not capture ----
        restoreTunables();
        BrushFixtureResourceSource source;
        BrushPropertyHolderFixture& h = source.exemplars[1u];
        // ORACLE-ANCHORED (captured live after the real exemplar load):
        h.SetFloat(0x48cd7b26u, 2500.0f);   // kMaxTerrainHeight -> 2500 (0x451c4000)
        h.SetFloat(0x48cd7b27u, 20.0f);    // kMinTerrainHeight -> 20 (0x41a00000)
        h.SetFloat(0x48cd7b21u, 250.0f);    // kSeaLevel (live value not captured)
        h.SetFloat(0x48cd7b2a, 1.0f);      // kDefaultTerrainHeight — quirk (b) fires -> 20
        h.SetUint(0x48cd7b60u, 15u);        // kMinShoreCliffSize -> 15
        h.SetUint(0x48cd7b64u, 60u);        // kMaxShoreCliffSize -> 60
        h.SetFloat(0x48cd7b65u, 0.5f);      // kMaxNormalYForCliff -> 0.5 (0x3f000000)
        h.SetFloat(0x48cd7b61u, 3.0f);      // exemplar 3.0 — quirk (c): 3.0*6 = 18.0 (0x41900000)
        h.SetFloat(0x48cd7b62u, 60.0f);     // kMinShoreCliffHt -> 60 (0x42700000)
        h.SetFloat(0x48cd7b63u, 100.0f);    // kMaxShoreCliffHt -> 100 (0x42c80000)
        h.SetFloat(0x48cd7b70u, -10.0f);    // kMinSeaLevel -> -10.0 (0xc1200000)
        h.SetFloat(0x48cd7b71u, 200.0f);    // kMaxSeaLevel -> 200.0 (0x43480000)
        h.SetFloat(0x48cd7b72u, 5.0f);      // kSeaRaiseLowerAmount -> 5.0 (0x40a00000)
        h.SetFloat(0x48cd7b30u, 0.3f);      // kTemperatureRangeFactor -> 0.3 (0x3e99999a)
        h.SetUintArray(0x48cd7bf1u, {1u, 1u, 1u, 1u, 1u}); // flow steps {1,1,1,1,1}
        // Synthetic values for the uncaptured globals:
        h.SetFloatArray(0x48cd7b20u, {30.0f, 60.0f, 120.0f, 240.0f, 480.0f});
        h.SetFloatArray(0x48cd7b22u, {0.1f, 0.2f});
        h.SetFloatArray(0x48cd7b23u, {0.8f, 0.9f});
        h.SetFloatArray(0x48cd7b24u, {0.05f});
        h.SetFloatArray(0x48cd7b25u, {0.95f});
        h.SetFloat(0x48cd7b28u, 2.5f);
        h.SetFloat(0x48cd7b31u, 0.25f);
        h.SetUint(0x48cd7b40u, 3u);
        h.SetUint(0x48cd7b41u, 100u);
        h.SetUint(0x48cd7b42u, 200u);
        h.SetFloatArray(0x48cd7b43u, {1.0f, 2.0f, 3.0f});
        h.SetUint(0x48cd7b50u, 17u);
        h.SetFloat(0x48cd7b51u, 0.7f);
        h.SetFloat(0x48cd7b52u, 0.9f);
        h.SetFloat(0x48cd7b53u, 0.2f);
        h.SetUint(0x48cd7b54u, 6u);
        h.SetFloat(0x69ac5640u, 11.0f);
        h.SetFloat(0x69ac5641u, 22.0f);
        h.SetFloat(0x48cd7b73u, 3.5f);
        h.SetByte(0xa9c283d0u, 9u);
        h.SetFloat(0xa9c283d1u, 4.5f);
        h.SetByte(0x48cd7b80u, 7u);         // setne normalizes 7 -> 1
        // NOTE: 0x48cd7b81 deliberately absent — the shared-slot carryover
        // below must inherit the 7.
        h.SetFloatArray(0x48cd7ba0u, {10.0f, 20.0f, 30.0f, 40.0f, 50.0f});
        h.SetUint(0x89861864u, 2000u);       // lifeTime -> interval 100
        h.SetUintArray(0x89a312c2u, {4u, 8u, 16u});
        h.SetUint(0x48cd7bf3u, 2u);
        h.SetFloatArray(0x48cd7c00u, {1.0f, 5.0f}); // curve property present (non-null)
        h.SetUint(0x48cd7c01u, 7u);
        h.SetFloat(0x48cd7c02u, 0.75f);
        cSTETerrain::TuningResourceSource() = &source;
        cSTETerrain::ReadTuningParameters();
        Check(cSTETerrain::MaxTerrainHeightTunable() == 2500.0f &&
                  std::bit_cast<Uint32>(cSTETerrain::MaxTerrainHeightTunable()) == 0x451c4000u &&
                  cSTETerrain::MinTerrainHeightTunable() == 20.0f &&
                  std::bit_cast<Uint32>(cSTETerrain::MinTerrainHeightTunable()) == 0x41a00000u &&
                  cSTETerrain::MinShoreCliffSizeTunable() == 15 &&
                  cSTETerrain::MaxShoreCliffSizeTunable() == 60 &&
                  cSTETerrain::MinShoreCliffCandidateHtSumTunable() == 18.0f &&
                  std::bit_cast<Uint32>(cSTETerrain::MinShoreCliffCandidateHtSumTunable()) == 0x41900000u &&
                  cSTETerrain::MinShoreCliffHtTunable() == 60.0f &&
                  std::bit_cast<Uint32>(cSTETerrain::MinShoreCliffHtTunable()) == 0x42700000u &&
                  cSTETerrain::MaxShoreCliffHtTunable() == 100.0f &&
                  cSTETerrain::MaxNormalYForCliffTunable() == 0.5f &&
                  std::bit_cast<Uint32>(cSTETerrain::MaxNormalYForCliffTunable()) == 0x3f000000u &&
                  cSTETerrain::MinSeaLevelTunable() == -10.0f &&
                  std::bit_cast<Uint32>(cSTETerrain::MinSeaLevelTunable()) == 0xc1200000u &&
                  cSTETerrain::MaxSeaLevelTunable() == 200.0f &&
                  cSTETerrain::SeaRaiseLowerAmountTunable() == 5.0f &&
                  std::bit_cast<Uint32>(cSTETerrain::SeaRaiseLowerAmountTunable()) == 0x40a00000u &&
                  cSTETerrain::TemperatureRangeFactor() == 0.3f &&
                  std::bit_cast<Uint32>(cSTETerrain::TemperatureRangeFactor()) == 0x3e99999au,
              "ORACLE-ANCHORED: live post-load values land bit-exact (2500/20/15/60/18-after-*6/60/100/0.5/-10/200/5/0.3) per dossier_runtime_tunables.md");
        Check(cSTETerrain::FlowStepsPerErosionApplication().size() == 5 &&
                  cSTETerrain::FlowStepsPerErosionApplication()[0] == 1 &&
                  cSTETerrain::FlowStepsPerErosionApplication()[2] == 1 &&
                  cSTETerrain::FlowStepsPerErosionApplication()[4] == 1,
              "ORACLE-ANCHORED: kNumFlowStepsPerWaterErosionBrushApplication -> {1,1,1,1,1} (DoWaterErosion runtime flow steps) @ 0x10073dd7e");
        Check(cSTETerrain::SeaLevelTunable() == 250.0f &&
                  cSTETerrain::DefaultTerrainHeightTunable() == 20.0f,
              "quirk (a) not-fire (20 <= 250 leaves kSeaLevel) + quirk (b) fire (1.0 < 20 -> 20.0) @ 0x10073da31/0x10073da5e");
        {
            const auto& mnh = cSTETerrain::MaxNormalTerrainHeightTunable();
            const auto& slider = cSTETerrain::SeaLevelChangeDueToWaterSliderTunable();
            Check(mnh.size() == 5 && mnh[0] == 30.0f && mnh[4] == 480.0f &&
                      cSTETerrain::MinRegularHtFactorTunable().size() == 2 &&
                      cSTETerrain::MinRegularHtFactorTunable()[1] == 0.2f &&
                      cSTETerrain::MaxRegularHtFactorTunable()[0] == 0.8f &&
                      cSTETerrain::MinMountainHtFactorTunable()[0] == 0.05f &&
                      cSTETerrain::MaxMountainHtFactorTunable()[0] == 0.95f &&
                      cSTETerrain::ImageImportScaleFactorTunable() == 2.5f &&
                      cSTETerrain::WaterProximityRangeFactorTunable() == 0.25f &&
                      slider.size() == 3 && slider[2] == 3.0f &&
                      cSTETerrain::NumInitialErosionPassesTunable() == 3 &&
                      cSTETerrain::NumHydraulicIterationsPerInitialErosionPassTunable() == 100 &&
                      cSTETerrain::NumWindIterationsPerInitialErosionPassTunable() == 200 &&
                      cSTETerrain::MinCellsInUsefulFlatAreaTunable() == 17 &&
                      cSTETerrain::MaxHtChangeToFlattenFactorTunable() == 0.7f &&
                      cSTETerrain::MaxTerraceAltitudeFactorTunable() == 0.9f &&
                      cSTETerrain::MinTerraceAltitudeFactorTunable() == 0.2f &&
                      cSTETerrain::AutoFlatAreaEdgeSmootheningRadiusTunable() == 6 &&
                      cSTETerrain::MinWaterDepthForDeepSeaBedTextureTunable() == 11.0f &&
                      cSTETerrain::MaxWaterDepthForDeepSeaBedTextureTunable() == 22.0f &&
                      cSTETerrain::TerrainRaiseLowerAmountTunable() == 3.5f &&
                      cSTETerrain::MaxBeachWidthTunable() == 9 &&
                      cSTETerrain::MaxBeachAltitudeTunable() == 4.5f &&
                      cSTETerrain::NumSmoothingIterationsAfterWaterErosionTunable() == 2 &&
                      cSTETerrain::EdgeReconcilliationSmoothingIterationsTunable() == 7 &&
                      cSTETerrain::EdgeReconcilliationHtDifferenceThresholdTunable() == 0.75f &&
                      cSTETerrain::TextureRandomizationGridSize().size() == 3 &&
                      cSTETerrain::TextureRandomizationGridSize()[2] == 16,
                  "the full read sequence wires every tuning global in the binary's fixed order (0x10073d9b7..0x10073ddeb)");
        }
        {
            const Float64 kPiOver180 = std::bit_cast<Float64>(0x3f91df46aaaaaaabULL);
            const auto& angles = cSTETerrain::TypicalBrushAngleIncrementTunable();
            Check(angles.size() == 5 &&
                      angles[0] == static_cast<Float32>(static_cast<Float64>(10.0f) * kPiOver180) &&
                      angles[2] == static_cast<Float32>(static_cast<Float64>(30.0f) * kPiOver180) &&
                      angles[4] == static_cast<Float32>(static_cast<Float64>(50.0f) * kPiOver180),
                  "quirk (d): angles scaled by f64 pi/180 via cvtss2sd/mulsd/cvtsd2ss @ 0x10073dd24..0x10073dd4f");
        }
        Check(cSTETerrain::VolcanicAshLifeTimeTunable() == 2000u &&
                  cSTETerrain::VolcanicAshUpdateIntervalTunable() == 100u,
              "quirk (e): update interval = lifeTime * 0xCCCCCCCD >> 36 = 2000/20 = 100 @ 0x10073dd65..0x10073dd78");
        Check(cSTETerrain::AutoFlattenUponCreationTunable() == 1 &&
                  cSTETerrain::ShoreCliffsUponCreationTunable() == 1,
              "setne flag stores normalize 7 -> 1; the MISSING 0x48cd7b81 read inherits the shared [rbp-0x19] leftover -> 1 @ 0x10073dcef/0x10073dd09");
        Check(cSTETerrain::EdgeReconcilliationCurveLoadCount() == 1 &&
                  cSTETerrain::DetermineTextureFamilyMembersCallCount() == 2,
              "property 0x48cd7c00 non-null -> SetResponseCurveData boundary observed (je @ 0x10073ddb7; callee TODO no-op); Determine ran again @ 0x10073d9cb");
        Check(cSTETerrain::WindErosionFactorTunable() ==
                  std::bit_cast<Float32>(0x00000001u),
              "kWindErosionFactor is NOT wired by ReadTuningParameters (fill site is ReadTerrainIniFile @ 0x10073c19d, a later ticket) — keeps the shipped denormal slot");

        // ---- quirk (a)/(b) fire / not-fire / NaN matrices ----
        auto runWith = [](BrushFixtureResourceSource& s) {
            cSTETerrain::TuningResourceSource() = &s;
            cSTETerrain::ReadTuningParameters();
        };
        restoreTunables();
        {
            BrushFixtureResourceSource s;
            s.exemplars[1u].SetFloat(0x48cd7b27u, 20.0f);
            s.exemplars[1u].SetFloat(0x48cd7b21u, 10.0f);
            runWith(s);
            Check(cSTETerrain::SeaLevelTunable() == 20.0f,
                  "quirk (a) fire: kMinTerrainHeight(20) > kSeaLevel(10) -> kSeaLevel := 20 @ 0x10073da31");
        }
        restoreTunables();
        {
            BrushFixtureResourceSource s;
            s.exemplars[1u].SetFloat(0x48cd7b27u, 5.0f);
            s.exemplars[1u].SetFloat(0x48cd7b21u, 300.0f);
            runWith(s);
            Check(cSTETerrain::SeaLevelTunable() == 300.0f,
                  "quirk (a) not-fire: 5.0 <= 300.0 leaves kSeaLevel untouched (jbe @ 0x10073da2f)");
        }
        restoreTunables();
        {
            BrushFixtureResourceSource s;
            s.exemplars[1u].SetFloat(0x48cd7b27u, 20.0f);
            s.exemplars[1u].SetFloat(0x48cd7b21u, std::bit_cast<Float32>(0x7fc00000u));
            runWith(s);
            Check(std::bit_cast<Uint32>(cSTETerrain::SeaLevelTunable()) == 0x7fc00000u,
                  "quirk (a) NaN: ucomiss unordered sets CF -> jbe taken -> NaN kSeaLevel preserved, NOT overwritten @ 0x10073da2f");
        }
        restoreTunables();
        {
            BrushFixtureResourceSource s;
            s.exemplars[1u].SetFloat(0x48cd7b27u, 20.0f);
            s.exemplars[1u].SetFloat(0x48cd7b2au, 10.0f);
            runWith(s);
            Check(cSTETerrain::DefaultTerrainHeightTunable() == 20.0f,
                  "quirk (b) fire: 20 > 10 -> kDefaultTerrainHeight := 20 @ 0x10073da5e");
        }
        restoreTunables();
        {
            BrushFixtureResourceSource s;
            s.exemplars[1u].SetFloat(0x48cd7b27u, 5.0f);
            runWith(s);
            Check(cSTETerrain::DefaultTerrainHeightTunable() == 50.0f,
                  "quirk (b) not-fire: 5.0 <= shipped 50.0 leaves kDefaultTerrainHeight untouched (jbe @ 0x10073da5c)");
        }

        // ---- quirk (c) on a read value + NaN propagation ----
        restoreTunables();
        {
            BrushFixtureResourceSource s;
            s.exemplars[1u].SetFloat(0x48cd7b61u, 4.0f);
            runWith(s);
            Check(cSTETerrain::MinShoreCliffCandidateHtSumTunable() == 24.0f,
                  "quirk (c) scales the exemplar value: 4.0 * 6.0f = 24.0 @ 0x10073dc26");
        }
        restoreTunables();
        {
            BrushFixtureResourceSource s;
            s.exemplars[1u].SetFloat(0x48cd7b61u, std::bit_cast<Float32>(0x7fc00000u));
            runWith(s);
            Check(std::isnan(cSTETerrain::MinShoreCliffCandidateHtSumTunable()),
                  "quirk (c) propagates NaN (mulss has no NaN gate) @ 0x10073dc26");
        }

        // ---- quirk (d) short vector: model guard, no OOB ----
        restoreTunables();
        {
            BrushFixtureResourceSource s;
            s.exemplars[1u].SetFloatArray(0x48cd7ba0u, {45.0f});
            runWith(s);
            const auto& a2 = cSTETerrain::TypicalBrushAngleIncrementTunable();
            Check(a2.size() == 1 &&
                      a2[0] == static_cast<Float32>(45.0 * std::bit_cast<Float64>(0x3f91df46aaaaaaabULL)),
                  "quirk (d): a 1-entry vector scales only entry 0 (binary walks 5 entries ungated — model guard) @ 0x10073dd2c..0x10073dd4f");
        }

        // ---- quirk (e) magic-shift truncation ----
        restoreTunables();
        {
            BrushFixtureResourceSource s;
            s.exemplars[1u].SetUint(0x89861864u, 21u);
            runWith(s);
            Check(cSTETerrain::VolcanicAshUpdateIntervalTunable() == 1u,
                  "quirk (e): 21 * 0xCCCCCCCD >> 36 = 1 (magic reciprocal truncates; not 1.05) @ 0x10073dd70..0x10073dd74");
        }
        restoreTunables();
        {
            BrushFixtureResourceSource s;
            s.exemplars[1u].SetUint(0x89861864u, 19u);
            runWith(s);
            Check(cSTETerrain::VolcanicAshUpdateIntervalTunable() == 0u,
                  "quirk (e): 19/20 truncates to 0 @ 0x10073dd70..0x10073dd74");
        }

        // ---- shared bool slot carryover, reverse case ----
        restoreTunables();
        {
            BrushFixtureResourceSource s;
            s.exemplars[1u].SetByte(0x48cd7b81u, 5u); // second flag present, first missing
            runWith(s);
            Check(cSTETerrain::AutoFlattenUponCreationTunable() == 0 &&
                      cSTETerrain::ShoreCliffsUponCreationTunable() == 1,
                  "shared [rbp-0x19] slot: first read fails on the pre-zeroed slot -> 0; second read byte 5 -> setne 1 @ 0x10073dcef/0x10073dd09");
        }

        // ---- mistyped property: read fails, global untouched ----
        restoreTunables();
        {
            BrushFixtureResourceSource s;
            s.exemplars[1u].SetUint(0x48cd7b26u, 42u); // Uint kind where float& is read
            runWith(s);
            Check(cSTETerrain::MaxTerrainHeightTunable() == 200.0f,
                  "mistyped property (Uint stored, float& read) -> GetPropertyValue fails, global keeps the shipped default @ 0x10073d9e4");
        }

        // ---- partial fixture: only one property present ----
        restoreTunables();
        {
            BrushFixtureResourceSource s;
            s.exemplars[1u].SetFloat(0x48cd7b26u, 1234.0f);
            runWith(s);
            Check(cSTETerrain::MaxTerrainHeightTunable() == 1234.0f &&
                      cSTETerrain::MinTerrainHeightTunable() == 5.0f &&
                      cSTETerrain::MaxSeaLevelTunable() == 200.1f &&
                      cSTETerrain::NumHydraulicIterationsPerInitialErosionPassTunable() == 334,
                  "partial fixture: the one present property lands, every other global keeps its shipped default");
        }

        // ---- cleanup: restore every settable static so no later check
        //      observes leftovers ----
        restoreTunables();
        cSTETerrain::TuningResourceSource() = nullptr;
        cSTETerrain::EdgeReconcilliationCurveLoadCount() = 0;
        cSTETerrain::DetermineTextureFamilyMembersCallCount() = 0;
        cSTETerrain::sRand = rngBefore;
        Check(cSTETerrain::MaxTerrainHeightTunable() == 200.0f &&
                  cSTETerrain::MinShoreCliffCandidateHtSumTunable() == 15.0f &&
                  cSTETerrain::AutoFlattenUponCreationTunable() == 0x8au &&
                  cSTETerrain::ShoreCliffsUponCreationTunable() == 1 &&
                  cSTETerrain::VolcanicAshUpdateIntervalTunable() == 10000000u &&
                  cSTETerrain::TypicalBrushAngleIncrementTunable().empty() &&
                  cSTETerrain::FlowStepsPerErosionApplication().empty(),
              "cleanup: every tuning global restored to its shipped __DATA default");
    }

    std::printf("\n================================================================\n");
    std::printf(" Result: %d passed, %d failed\n", gPassed, gFailed);
    std::printf("================================================================\n");
    return gFailed == 0 ? 0 : 1;
}
