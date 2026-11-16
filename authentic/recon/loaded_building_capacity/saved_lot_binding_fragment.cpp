// Standalone integration checks; body mirrors verify_sot.cpp CheckSavedLotBindings.
#include "opensc4/city.h"
#include <algorithm>
#include <array>
#include <bit>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>
using namespace opensc4;
namespace {
int failed = 0, passed = 0;
void Check(bool ok, const char* name) {
    ok ? ++passed : ++failed;
    std::printf("[%s] %s\n", ok ? "PASS" : "FAIL", name);
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
}
int main() {
    CheckSavedLotBindings(cDBPFSystem::DefaultSteamSC4AppPath());
    std::printf("saved-lot integration: %d passed, %d failed\n", passed, failed);
    return failed ? 1 : 0;
}
