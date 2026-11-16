#include "opensc4/gzcom.h"

#include <algorithm>

namespace opensc4 {

Uint32 ComputeMaxisCRC32(const Uint8* data, std::size_t length) noexcept {
    static Uint32 table[256];
    static bool initialized = false;
    if (!initialized) {
        for (Uint32 i = 0; i < 256; ++i) {
            Uint32 crc = i << 24;
            for (int j = 0; j < 8; ++j) {
                if (crc & 0x80000000u) {
                    crc = (crc << 1) ^ 0x04C11DB7u;
                } else {
                    crc <<= 1;
                }
            }
            table[i] = crc;
        }
        initialized = true;
    }

    Uint32 crc = 0xFFFFFFFFu;
    for (std::size_t i = 0; i < length; ++i) {
        Uint8 idx = static_cast<Uint8>((crc >> 24) ^ data[i]);
        crc = (crc << 8) ^ table[idx];
    }
    return crc;
}

Uint32 ComputeSC4RecordCRC(Uint32 memAddr, const Uint8* body, std::size_t bodyLength) noexcept {
    const std::size_t total = std::min<std::size_t>(bodyLength + 4u, kSC4RecordCRCMaxBytes);
    std::vector<Uint8> buf(total);
    std::memcpy(buf.data(), &memAddr, std::min<std::size_t>(4u, total));
    if (total > 4u) {
        std::memcpy(buf.data() + 4, body, total - 4u);
    }
    return ComputeMaxisCRC32(buf.data(), buf.size());
}

const char* GetGZClassName(Uint32 clsid) noexcept {
    switch (clsid) {
        case kTypeUI:                        return "UI Layout (0x00000000)";
        case kTypeCohort:                    return "Cohort (CQZB1/CQZT1)";
        case kTypeLTEXT:                     return "LTEXT Localized String";
        case kTypeS3D:                       return "cS3DModel (3DMD)";
        case kTypeExemplar:                  return "cGZExemplar (EQZB1/EQZT1)";
        case kTypeFSH:                       return "cFSHImage (SHPI)";
        case kTypePNG:                       return "PNG Image Snapshot";
        case kTypeLUA:                       return "cGZLuaScript";
        case kTypeWAV:                       return "WAV Audio";
        case kTypeDIR:                       return "DBPF Directory (DIR)";
        case kGZCLSID_cSC4SimGridBool8:      return "cSC4SimGrid<Bool8>";
        case kGZCLSID_cSC4SimGridSint8:      return "cSC4SimGrid<Sint8>";
        case kGZCLSID_cSC4SimGridUint8:      return "cSC4SimGrid<Uint8>";
        case kGZCLSID_cSC4SimGridSint16:     return "cSC4SimGrid<Sint16>";
        case kGZCLSID_cSC4SimGridUint16:     return "cSC4SimGrid<Uint16>";
        case kGZCLSID_cSC4SimGridSint32:     return "cSC4SimGrid<Sint32>";
        case kGZCLSID_cSC4SimGridUint32:     return "cSC4SimGrid<Uint32>";
        case kGZCLSID_cSC4SimGridSint64:     return "cSC4SimGrid<Sint64>";
        case kGZCLSID_cSC4SimGridUint64:     return "cSC4SimGrid<Uint64>";
        case kGZCLSID_cSC4SimGridFloat32:    return "cSC4SimGrid<Float32>";
        case kGZCLSID_cSC4SimGridFloat64:    return "cSC4SimGrid<Float64>";
        case kCLSID_cSC4ReadRegionalCity:    return "cSC4ReadRegionalCity";
        case kCLSID_cSC4RegionalCity:        return "cSC4RegionalCity";
        case kCLSID_cSC4City:                return "cSC4City";
        case kCLSID_cSTETerrain:             return "cSTETerrain";
        case kCLSID_cSTETerrainView3D:       return "cSTETerrainView3D";
        case kCLSID_cSTETerrainHeightmap:    return "cSTETerrainHeightmap";
        case kCLSID_cSC4OccupantManager:     return "cSC4OccupantManager";
        case kCLSID_cSC4Occupant:            return "cSC4Occupant";
        case kCLSID_cSC4Lot:                 return "cSC4Lot";
        case kCLSID_cSC4BuildingOccupant:    return "cSC4BuildingOccupant";
        case kCLSID_cSC4NetworkOccupant:     return "cSC4NetworkOccupant";
        case kCLSID_cSC4NetworkPreBuilt:     return "cSC4NetworkOccupantWithPreBuiltModel";
        case kCLSID_cSC4NetworkBridge:       return "cSC4NetworkBridgeOccupant";
        case kCLSID_cSC4NetworkTunnel:       return "cSC4NetworkTunnelOccupant";
        case kCLSID_cSC4PropOccupant:        return "cSC4PropOccupant";
        case kCLSID_cSC4FloraOccupant:       return "cSC4FloraOccupant";
        case kCLSID_cSC4PipeOccupant:        return "cSC4PipeOccupant";
        case kCLSID_cSC4SubwayOccupant:      return "cSC4SubwayOccupant";
        case kCLSID_cSC4PowerLineOccupant:   return "cSC4PowerLineOccupant";
        case kCLSID_cSC4PowerPoleOccupant:   return "cSC4PowerPoleOccupant";
        case kCLSID_cSC4PylonOccupant:       return "cSC4PylonOccupant";
        case kCLSID_cSC4FoundationOccupant:  return "cSC4FoundationOccupant";
        case kCLSID_cSC4RetainingWall:       return "cSC4LotRetainingWallOccupant";
        case kCLSID_cSC4LotBaseTexture:      return "cSC4LotBaseTextureOccupant";
        case kCLSID_cSC4LandfillOccupant:    return "cSC4LandfillOccupant";
        case kCLSID_cSC4ZoneManager:         return "cSC4ZoneManager";
        case kCLSID_cSC4NetworkManager:      return "cSC4NetworkManager";
        case kCLSID_cSC4NetworkLotManager:   return "cSC4NetworkLotManager";
        case kCLSID_cSC4Simulator:           return "cSC4Simulator";
        case kCLSID_cSC424HourClock:         return "cSC424HourClock";
        case kCLSID_cSC4DemandSimulator:     return "cSC4DemandSimulator";
        case kCLSID_cSC4Demand:              return "cSC4Demand";
        case kCLSID_cSC4ResidentialSimulator:return "cSC4ResidentialSimulator";
        case kCLSID_cSC4CommercialSimulator: return "cSC4CommercialSimulator";
        case kCLSID_cSC4IndustrialSimulator: return "cSC4IndustrialSimulator";
        case kCLSID_cSC4TrafficSimulator:    return "cSC4TrafficSimulator";
        case kCLSID_cSC4PollutionSimulator:  return "cSC4PollutionSimulator";
        case kCLSID_cSC4PlumbingSimulator:   return "cSC4PlumbingSimulator";
        case kCLSID_cSC4PowerSimulator:      return "cSC4PowerSimulator";
        case kCLSID_cSC4PoliceSimulator:     return "cSC4PoliceSimulator";
        case kCLSID_cSC4FireProtectionSim:   return "cSC4FireProtectionSimulator";
        case kCLSID_cSC4CrimeSimulator:      return "cSC4CrimeSimulator";
        case kCLSID_cSC4AuraSimulator:       return "cSC4AuraSimulator";
        case kCLSID_cSC4BudgetSimulator:     return "cSC4BudgetSimulator";
        case kCLSID_cSC4LandValueSimulator:  return "cSC4LandValueSimulator";
        case kCLSID_cSC4FlammabilitySim:     return "cSC4FlammabilitySimulator";
        case kCLSID_cSC4FloraSimulator:      return "cSC4FloraSimulator";
        case kCLSID_cSC4WeatherSimulator:    return "cSC4WeatherSimulator";
        case kCLSID_cSC4CivicBuildingSim:    return "cSC4CivicBuildingSimulator";
        case kCLSID_cSC4NeighborsSimulator:  return "cSC4NeighborsSimulator";
        case kCLSID_cSC4NeighborConnection:  return "cSC4NeighborConnection";
        case kCLSID_cSC4NeighborDeal:        return "cSC4NeighborDeal";
        case kCLSID_cSC4OrdinanceSimulator:  return "cSC4OrdinanceSimulator";
        case kCLSID_cSC4BuildingDevSim:      return "cSC4BuildingDevelopmentSimulator";
        case kCLSID_cSC4LotDeveloper:        return "cSC4LotDeveloper";
        case kCLSID_cSC4ZoneDeveloper:       return "cSC4ZoneDeveloper";
        case kCLSID_cSC4TractDeveloper:      return "cSC4TractDeveloper";
        case kCLSID_cSC4GrowthDeveloper:     return "cSC4GrowthDeveloper";
        case kCLSID_cSC4PropManager:         return "cSC4PropManager";
        case kCLSID_cSC4ParkManager:         return "cSC4ParkManager";
        case kCLSID_cSC4AirportDeveloper:    return "cSC4AirportDeveloper";
        case kCLSID_cSC4SeaportDeveloper:    return "cSC4SeaportDeveloper";
        case kCLSID_cSC4LandfillDeveloper:   return "cSC4LandfillDeveloper";
        case kCLSID_cSC4TutorialSystem:      return "cSC4TutorialSystem";
        case kCLSID_cSC4Demolition:          return "cSC4Demolition";
        case kCLSID_cSC4CityDetailManager:   return "cSC4CityDetailManager";
        case kCLSID_cSC43DRender:            return "cSC43DRender";
        default:                             return "cIGZSerializable";
    }
}

} // namespace opensc4
