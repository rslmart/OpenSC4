#include "opensc4/dbpf.h"
#include "opensc4/sc4_subfiles_batch2.h"
#include <bit>
#include <cstdio>
#include <map>
#include <set>
using namespace opensc4;
// Read-only fixture inspection. No reconstructed coordinate/power query.
int main() {
    cDBPFArchive archive;
    const auto path = cDBPFSystem::DefaultSteamSC4AppPath() +
        "/Contents/GameData/Regions/Timbuktu/City - Rush Hour Tutorial.sc4";
    if (!archive.Open(path)) return 2;
    std::vector<Uint8> bytes;
    std::vector<ZoneDeveloperSaveRecord> grids;
    std::vector<LotFullSaveRecord> lots;
    std::vector<BuildingOccupantSaveRecord> buildings;
    if (!archive.ReadFirstByType(kCLSID_cSC4ZoneDeveloper, bytes) ||
        !DecodeMultiRecordSubfile<ZoneDeveloperSaveRecord>(bytes, grids)) return 3;
    if (!archive.ReadFirstByType(kCLSID_cSC4Lot, bytes) ||
        !DecodeMultiRecordSubfile<LotFullSaveRecord>(bytes, lots)) return 4;
    if (!archive.ReadFirstByType(kCLSID_cSC4BuildingOccupant, bytes) ||
        !DecodeMultiRecordSubfile<BuildingOccupantSaveRecord>(bytes, buildings)) return 5;
    const BuildingOccupantSaveRecord* target = nullptr;
    for (const auto& b : buildings) if (b.memAddr == 0x138a8240u) {
        if (target) return 6;
        target = &b;
    }
    if (!target || grids.size() != 1) return 7;
    std::printf("building=%08x bounds_bytes=", target->memAddr);
    for (auto v : target->base.cellBounds) std::printf("%02x", v);
    std::puts("");
    // Raw object fields, not evaluation of GetPosition or rounding.
    const auto dumpVector = [](const char* label, const S3DVector3& v) {
        std::printf("%s=%08x,%08x,%08x\n", label,
            std::bit_cast<Uint32>(v.x), std::bit_cast<Uint32>(v.y),
            std::bit_cast<Uint32>(v.z));
    };
    dumpVector("min_bits", target->minBounds);
    dumpVector("max_bits", target->maxBounds);
    std::map<Uint32, const LotFullSaveRecord*> byAddress;
    for (const auto& lot : lots)
        if (!byAddress.emplace(lot.memAddr, &lot).second) return 8;
    const auto& grid = grids.front();
    std::printf("grid=%08x version=%u outer=%zu inner=%zu lots=%zu\n",
        grid.memAddr, grid.version, grid.rows.size(),
        grid.rows.empty() ? 0 : grid.rows.front().size(), lots.size());
    std::size_t populated=0, unresolved=0, wrongClass=0;
    std::set<Uint32> printed;
    for (std::size_t x=0; x<grid.rows.size(); ++x) {
        for (std::size_t z=0; z<grid.rows[x].size(); ++z) {
            const auto ref=grid.rows[x][z];
            if (!ref.memAddr) continue;
            ++populated;
            if (ref.clsid != kCLSID_cSC4Lot) { ++wrongClass; continue; }
            auto found=byAddress.find(ref.memAddr);
            if (found==byAddress.end()) { ++unresolved; continue; }
            // Fixed inspection window near the raw position's midpoint.
            // Not an implementation or proof of exact coordinate rounding.
            // Saved base.cellBounds are printed without interpreting layout.
            if (x<36 || x>40 || z<113 || z>117) continue;
            std::printf("window cell=%zu,%zu ref=%08x:%08x\n",x,z,ref.clsid,ref.memAddr);
            const auto& lot=*found->second;
            if (printed.insert(lot.memAddr).second)
                std::printf("lot=%08x config=%08x saved_flagsCD=%02x bounds=%u,%u,%u,%u\n",
                    lot.memAddr,lot.configId,lot.flagsCD,lot.minX,lot.minZ,lot.maxX,lot.maxZ);
        }
    }
    std::printf("populated=%zu unresolved=%zu wrong_class=%zu window_unique_lots=%zu\n",
        populated,unresolved,wrongClass,printed.size());
    return unresolved || wrongClass ? 9 : 0;
}
