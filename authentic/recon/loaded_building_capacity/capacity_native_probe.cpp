// Read-only fixture replay through the production public query.
#include "opensc4/saved_building_capacity.h"
#include "opensc4/building_lot_position.h"
#include <bit>
#include <cfenv>
#include <cstdio>
using namespace opensc4;
int main() {
    if (std::fesetround(FE_TONEAREST)) return 1;
    cDBPFSystem assets;
    const auto app=cDBPFSystem::DefaultSteamSC4AppPath();
    cSC4City city;
    if (!assets.MountInstall(app) || !city.LoadFromSC4File(app+
        "/Contents/GameData/Regions/Timbuktu/City - Rush Hour Tutorial.sc4", &assets)) return 2;
    const auto& refs=city.PollutionSaveRecord().listF0;
    if (refs.size()!=1) return 3;
    const auto ref=refs.front();
    const auto* b=city.FindSavedBuildingOccupant(ref);
    if (!b) return 4;
    const auto& key=b->base.exemplarHolder.exemplarKey;
    std::printf("NATIVE BEGIN cases=4\n");
    std::printf("NATIVE FIXTURE ref=%08x:%08x tgi=%08x:%08x:%08x\n",
        ref.clsid,ref.memAddr,key.type,key.group,key.instance);
    const auto bits=[](const S3DVector3& v) { return lot_position::PositionBits{
        std::bit_cast<Uint32>(v.x),std::bit_cast<Uint32>(v.y),std::bit_cast<Uint32>(v.z)}; };
    const auto min=bits(b->minBounds),max=bits(b->maxBounds);
    const auto p=lot_position::BuildingPosition(min,max);
    std::printf("NATIVE GEOMETRY position=%08x,%08x,%08x min=%08x,%08x,%08x max=%08x,%08x,%08x\n",
        p.x,p.y,p.z,min.x,min.y,min.z,max.x,max.y,max.z);
    const auto lot=city.QuerySavedBuildingLot(ref);
    if(lot.status!=SavedBuildingLotStatus::Lot) return 5;
    std::printf("NATIVE POWER cell=%d,%d lot=%08x:%08x flagsCD=%02x isPowered=%d scale_raw=%08x\n",
        lot.cellX,lot.cellZ,lot.lotRef.clsid,lot.lotRef.memAddr,lot.savedFlagsCD,
        lot.savedPowered,std::bit_cast<Uint32>(city.PollutionSaveRecord().field348));
    for (Uint32 id:{0x08fd1613u,0x48ebd0b1u,0x27812852u,1u}) {
        const auto r=QuerySavedBuildingCapacity(city,ref,id,assets);
        if(r.status!=SavedCapacityStatus::Value && r.status!=SavedCapacityStatus::MissingCapacity) return 6;
        std::printf("NATIVE CAPACITY id=%08x returned=%d output=%08x decimal=%u\n",
            id,r.status==SavedCapacityStatus::Value,r.value,r.value);
    }
    std::puts("NATIVE END complete=1 cases=4");
}
