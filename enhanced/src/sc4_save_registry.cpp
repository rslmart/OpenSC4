// Registry of every decompiled .sc4 subfile decoder, used by opensc4_verify to
// prove byte-exact Read/Write against the shipped saves.

#include "opensc4/budget_simulator.h"
#include "opensc4/civic_simulators.h"
#include "opensc4/sc4_subfiles.h"
#include "opensc4/sc4_subfiles_batch2.h"
#include "opensc4/sc4_subfiles_batch3.h"
#include "opensc4/simgrid.h"
#include "opensc4/tract_developer.h"

namespace opensc4 {
namespace {

using sc4_detail::kCompareAllVersions;
using sc4_detail::RoundTripMulti;

template <typename T>
struct SimGridRecord {
    cSC4SimGrid<T> grid{DefaultSimGridClassID<T>()};
    bool Read(cIGZIStream& s) { return grid.Read(s); }
    bool Write(cIGZOStream& s) const { return grid.Write(s); }
};

} // namespace

bool RoundTripAnySC4Subfile(Uint32 typeId, std::span<const Uint8> b, SC4SubfileRoundTripStats& st) {
    if (RoundTripSC4Subfile(typeId, b, st)) {
        return true;
    }
    if (RoundTripSC4SubfileBatch2(typeId, b, st)) {
        return true;
    }
    if (RoundTripSC4SubfileBatch3(typeId, b, st)) {
        return true;
    }
    constexpr Uint32 kAll = kCompareAllVersions;
    switch (typeId) {
        // Simulators
        case kCLSID_cSC4Simulator:            RoundTripMulti<SimulatorSaveRecord>(b, 4, st); return true;
        case kCLSID_cSC4BudgetSimulator:      RoundTripMulti<BudgetSimulatorSaveRecord>(b, 5, st); return true;
        case kCLSID_cSC4PowerSimulator:       RoundTripMulti<PowerSimulatorSaveRecord>(b, 6, st); return true;
        case kCLSID_cSC4PlumbingSimulator:    RoundTripMulti<PlumbingSimulatorSaveRecord>(b, kAll, st); return true;
        case kCLSID_cSC4AuraSimulator:        RoundTripMulti<AuraSimulatorSaveRecord>(b, 9, st); return true;
        case kCLSID_cSC4CivicBuildingSim:     RoundTripMulti<CivicBuildingSimulatorSaveRecord>(b, kAll, st); return true;
        case kCLSID_cSC4FireProtectionSim:    RoundTripMulti<FireProtectionSimulatorSaveRecord>(b, 3, st); return true;
        case kCLSID_cSC4WeatherSimulator:     RoundTripMulti<WeatherSimulatorSaveRecord>(b, 2, st); return true;
        case kCLSID_cSC4ResidentialSimulator: RoundTripMulti<ResidentialSimulatorSaveRecord>(b, 7, st); return true;
        case kCLSID_cSC4PoliceSimulator:      RoundTripMulti<PoliceSimulatorSaveRecord>(b, 8, st); return true;
        case kCLSID_cSC4TrafficSimulator:     RoundTripMulti<TrafficSimulatorSaveRecord>(b, 41, st); return true;
        case kCLSID_cSC4PollutionSimulator:   RoundTripMulti<PollutionSimulatorSaveRecord>(b, 4, st); return true;
        case kCLSID_cSC4CommercialSimulator:  RoundTripMulti<CommercialSimulatorSaveRecord>(b, kAll, st); return true;
        case kCLSID_cSC4CrimeSimulator:       RoundTripMulti<CrimeSimulatorSaveRecord>(b, kAll, st); return true;
        case kCLSID_cSC4IndustrialSimulator:  RoundTripMulti<IndustrialSimulatorSaveRecord>(b, kAll, st); return true;
        case kCLSID_cSC4OrdinanceSimulator:   RoundTripMulti<OrdinanceSimulatorSaveRecord>(b, kAll, st); return true;
        case kCLSID_cSC4FloraSimulator:       RoundTripMulti<FloraSimulatorSaveRecord>(b, kAll, st); return true;
        case kCLSID_cSC4NeighborsSimulator:   RoundTripMulti<NeighborsSimulatorSaveRecord>(b, kAll, st); return true;
        case kCLSID_cSC4FlammabilitySim:      RoundTripMulti<FlammabilitySimulatorSaveRecord>(b, kAll, st); return true;
        case kCLSID_cSC4LandValueSimulator:   RoundTripMulti<LandValueSimulatorSaveRecord>(b, kAll, st); return true;
        case kCLSID_cSC4MySimAgentSimulator:  RoundTripMulti<MySimAgentSimulatorSaveRecord>(b, kAll, st); return true;
        // Developers / managers
        case kCLSID_cSC4GrowthDeveloper:      RoundTripMulti<GrowthDeveloperSaveRecord>(b, kAll, st); return true;
        case kCLSID_cSC4TractDeveloper:       RoundTripMulti<TractDeveloperState>(b, 3, st); return true;
        case kCLSID_cSC4BuildingDevSim:       RoundTripMulti<BuildingDevelopmentSimSaveRecord>(b, kAll, st); return true;
        case kCLSID_cSC4DemandSimulator:      RoundTripMulti<DemandSimulatorSaveRecord>(b, 2, st); return true;
        case kCLSID_cSC4LotConfigurationManager: RoundTripMulti<LotConfigurationManagerSaveRecord>(b, kAll, st); return true;
        case kCLSID_cSC4LotDeveloper:         RoundTripMulti<LotDeveloperSaveRecord>(b, kAll, st); return true;
        case kCLSID_cSC4NetworkManager:       RoundTripMulti<NetworkManagerSaveRecord>(b, kAll, st); return true;
        case kCLSID_cSC4NetworkLotManager:    RoundTripMulti<NetworkLotManagerSaveRecord>(b, kAll, st); return true;
        // SimGrids
        case kGZCLSID_cSC4SimGridSint8:   RoundTripMulti<SimGridRecord<Sint8>>(b, kAll, st); return true;
        case kGZCLSID_cSC4SimGridUint8:   RoundTripMulti<SimGridRecord<Uint8>>(b, kAll, st); return true;
        case kGZCLSID_cSC4SimGridSint16:  RoundTripMulti<SimGridRecord<Sint16>>(b, kAll, st); return true;
        case kGZCLSID_cSC4SimGridUint16:  RoundTripMulti<SimGridRecord<Uint16>>(b, kAll, st); return true;
        case kGZCLSID_cSC4SimGridSint32:  RoundTripMulti<SimGridRecord<Sint32>>(b, kAll, st); return true;
        case kGZCLSID_cSC4SimGridUint32:  RoundTripMulti<SimGridRecord<Uint32>>(b, kAll, st); return true;
        case kGZCLSID_cSC4SimGridSint64:  RoundTripMulti<SimGridRecord<Sint64>>(b, kAll, st); return true;
        case kGZCLSID_cSC4SimGridUint64:  RoundTripMulti<SimGridRecord<Uint64>>(b, kAll, st); return true;
        case kGZCLSID_cSC4SimGridFloat32: RoundTripMulti<SimGridRecord<Float32>>(b, kAll, st); return true;
        case kGZCLSID_cSC4SimGridFloat64: RoundTripMulti<SimGridRecord<Float64>>(b, kAll, st); return true;
        default:
            return false;
    }
}

} // namespace opensc4
