#pragma once

// Exact decompilation of the Batch 2 .sc4 save subfiles (developers, budget,
// ordinances, neighbours, terrain/camera/render/clock, and managers) from the
// macOS x86_64 SimCity 4 Deluxe binary. Every record mirrors the binary's
// Write routine field-for-field (stream order, widths, raw bool bytes):
//
//   - cSC4OrdinanceSimple (20 ordinance CLSIDs, Read @ 0x100798828, Write @ 0x1007986d0)
//   - cSC4Demolition (0x0990c38a, Read @ 0x1000eb508, Write @ 0x1000eb4b0)
//   - cSC4AutomataScriptSystem (0x4a1b664e, Read @ 0x1001d71d2, Write @ 0x1001d717a)
//   - cSC4WealthRequester (0x89c49986, Read @ 0x10012620c, Write @ 0x1001261fc)
//   - cSC4CityDetailManager (0xead4ba36, Read @ 0x100197328, Write @ 0x1001972d0)
//   - cSTETerrainView3D (0xc9b84e10, Read @ 0x10074e0ee, Write @ 0x1007503de)
//   - cSTETerrain (0xe98f9525, Read @ 0x10073f0a2, Write @ 0x10073efda)
//   - cSC4PortDeveloper (Read @ 0x1000cc62a, Write @ 0x1000cc564) via
//     cSC4AirportDeveloper (0xaa2049a1, Read @ 0x1000c8656, Write @ 0x1000c85d0) and
//     cSC4SeaportDeveloper (0xea230a28, Read @ 0x1000ca8b2, Write @ 0x1000ca82c)
//   - cSC4LandfillDeveloper (0xea09d4ae, Read @ 0x100061146, Write @ 0x1000610dc),
//     cSC4LandfillZone (Read @ 0x1000ad66a, Write @ 0x1000ad56e),
//     cGarbageHeap (Read @ 0x1000acaa4, Write @ 0x1000aca32)
//   - cSC4ParkManager (0xaa3fe6bc, Read @ 0x1000c9730, Write @ 0x1000c96dc)
//   - cSC4PropManager (0xa990bd10, Read @ 0x100193dba, Write @ 0x100193d66)
//   - cSC4BusinessUnit (0x4a232da8, Read @ 0x10018386a, Write @ 0x1001837ea)
//   - cSC4TutorialSystem (0xaa3836fb, Read @ 0x10005fc1a, Write @ 0x10005fba6)
//   - cSC4CitySituationManager (0x0bb14381, Read @ 0x1006e5a4c, Write @ 0x1006e59ba)
//   - cSC424HourClock (0xa7e7f929, Read @ 0x10018589c, Write @ 0x10018580e)
//   - cSC4CameraControl (0xc9c628ec, Read @ 0x10076b480, Write @ 0x10076b3e6)
//   - cSC4NeighborConnection (0x0a134f24, Read @ 0x100074ad0, Write @ 0x100074a28)
//   - cSC4NeighborDeal (0x2a134f42, Read @ 0x100075844, Write @ 0x100075712)
//   - cSC4ZoneDeveloper (0x498f9b01, Read @ 0x100132ffc, Write @ 0x100132f32)
//   - cSC4Occupant standalone (0xa9bc9ab6, Read @ 0x1000bc86c, Write @ 0x1000bc7a8)
//   - cSC43DRender (0xe9c622d8, Read @ 0x1007612a2, Write @ 0x1007611e2)
//   - cSC4DispatchManager (0x4990bd93, Read @ 0x100215ed6, Write @ 0x100215de8),
//     operator<<(cIGZOStream&, cISC4DispatchManager::DispatchInfo const&) @ 0x100215d52
//   - cSC4LineItem (0xaa313c9f, Read @ 0x100245c0a, Write @ 0x100245b2c)
//   - cSC4DepartmentBudget (0xe990bffc, Read @ 0x100243996, Write @ 0x1002437e4)
//   - cSC4HistoryWarehouse (0x89efa536, Read @ 0x1000efd14, Write @ 0x1000efbf8)
//   - cSC4Demand (0x8990c12d, Read @ 0x100201252, Write @ 0x100201148)
//   - cSC4ModelMaker (0xa9c741b3, Read @ 0x10030d36e, Write @ 0x10030d1d2)
//   - cSC4PropDeveloper (0x89c48f47, Read @ 0x1001225d0, Write @ 0x1001222c8)
//   - cSC4ZoneManager (0x298f9b2d, Read @ 0x100198bf6, Write @ 0x1001988ee)

#include "opensc4/sc4_subfiles.h"

#include <algorithm>
#include <array>
#include <string>
#include <utility>
#include <vector>

namespace opensc4 {

constexpr GZCLSID kCLSID_cSC4AutomataScriptSystem = 0x4a1b664eu;
constexpr GZCLSID kCLSID_cSC4WealthRequester      = 0x89c49986u;
constexpr GZCLSID kCLSID_cSC4CitySituationManager = 0x0bb14381u;
constexpr GZCLSID kCLSID_cSC4CameraControl        = 0xc9c628ecu;
constexpr GZCLSID kCLSID_cSC43DRenderSave         = 0xe9c622d8u;
constexpr GZCLSID kCLSID_cSC4DispatchManager      = 0x4990bd93u;
constexpr GZCLSID kCLSID_cSC4LineItem             = 0xaa313c9fu;
constexpr GZCLSID kCLSID_cSC4DepartmentBudget     = 0xe990bffcu;
constexpr GZCLSID kCLSID_cSC4HistoryWarehouse     = 0x89efa536u;
constexpr GZCLSID kCLSID_cSC4DemandObject         = 0x8990c12du; // cSC4Demand instance
constexpr GZCLSID kCLSID_cSC4ModelMaker           = 0xa9c741b3u;
constexpr GZCLSID kCLSID_cSC4PropDeveloper        = 0x89c48f47u;
constexpr GZCLSID kCLSID_cSC4BusinessUnit         = 0x4a232da8u;
constexpr GZCLSID kCLSID_cSC4PortDeveloper        = 0x0a2c4eeeu;

// cSC4OrdinanceSimple (0x8bd4fc74) and the 20 derived ordinance classes saved in .sc4 files.
inline constexpr std::array<GZCLSID, 21> kOrdinanceSimpleCLSIDs = {
    0x8bd4fc74u,
    0x00d0723du, 0x02bf1dfau, 0x22f6e80cu, 0x22f6e81bu, 0x40d07236u,
    0x42bf1e18u, 0x62bf1daau, 0x62bf1db9u, 0x62f6e7cfu, 0x815b4cefu,
    0x82b9999bu, 0xa0d07129u, 0xa2bf1ddcu, 0xa2bf1de5u, 0xa2bf1e43u,
    0xc2bf1dc5u, 0xc2bf1e04u, 0xc2f6e81fu, 0xe0d0722eu, 0xe0d07233u,
};

// ---------------------------------------------------------------------------
// cSC4OrdinanceSimple (current version 4; Read accepts <= 4).
// ---------------------------------------------------------------------------
struct OrdinanceSimpleSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 4;
    Uint8 field4C = 0;            // +0x4c (bool, normalised on Read)
    Uint32 clsid5C = 0;           // +0x5c
    std::string name60;           // +0x60
    std::string description88;    // +0x88
    Uint32 fieldB0 = 0;           // +0xb0
    Float32 fieldB4 = 0.0f;       // +0xb4 (Read rescales when version < 4)
    Sint64 fieldB8 = 0;           // +0xb8
    Sint64 fieldC0 = 0;           // +0xc0
    Sint64 fieldC8 = 0;           // +0xc8
    Float32 fieldD0 = 0.0f;       // +0xd0
    Uint32 fieldD4 = 0;           // +0xd4
    Uint8 fieldD9 = 0;            // +0xd9
    Uint8 fieldDA = 0;            // +0xda
    Uint8 fieldDB = 0;            // +0xdb
    Sint64 fieldE0 = 0;           // +0xe0
    Uint8 fieldD8 = 0;            // +0xd8 (version >= 2)
    ExemplarPropertyHolderSaveRecord exemplarHolder;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const { return WriteWithVersion(stream, 4); }
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;

    // Exact cSC4OrdinanceSimple methods:
    //   cSC4OrdinanceSimple() @ 0x100797e6e, ~cSC4OrdinanceSimple() @ 0x100797f7c / 0x100798002 / 0x10079801a,
    //   QueryInterface @ 0x100798056, AddRef @ 0x1007980ca, Release @ 0x100798102,
    //   Init @ 0x10079818e, Shutdown @ 0x1007981b4, SetKey @ 0x1007981ce,
    //   FromData @ 0x1007981e8, SetAvailable @ 0x100798566, SetOn @ 0x100798590,
    //   CheckConditions @ 0x1007985fc, GetCurrentMonthlyIncome @ 0x10079865a,
    //   Simulate @ 0x1007986b0,
    //   GetYearFirstAvailable @ 0x100795fb0, GetChanceAvailability @ 0x100795fbc,
    //   GetMonthlyIncomeFactor @ 0x100795fd4, IsOn @ 0x100795fe4,
    //   IsIncomeOrdinance @ 0x10079600c, GetID @ 0x100796480,
    //   GetDescription @ 0x100796494, GetEnactmentIncome @ 0x1007964ac,
    //   GetAdvisorID @ 0x1007964c4, GetMiscProperties @ 0x1007964d0,
    //   IsEnabled @ 0x1007964e4, GetMonthlyAdjustedIncome @ 0x1007964f0,
    //   GetName @ 0x100797370, GetRetracmentIncome @ 0x100797380,
    //   IsAvailable @ 0x100797390, SetEnabled @ 0x1007973a0,
    //   ForceAvailable @ 0x1007973b0, ForceOn @ 0x1007973c0,
    //   ForceMonthlyAdjustedIncome @ 0x1007973d0, GetMonthlyConstantIncome @ 0x100797ad0.
    // Specialized derived ordinances:
    //   cSC4OrdinanceLegalizeGambling: GetCurrentMonthlyIncome @ 0x100796e88, SetOn @ 0x100796e96
    //   cSC4OrdinanceNeighborhoodWatch: CheckConditions @ 0x100797304
    //   cSC4OrdinanceFreeClinics: Init @ 0x100795ed4, Shutdown @ 0x100795ede, GetCurrentMonthlyIncome @ 0x100795f12
    //   cSC4OrdinanceProReadingCampaign: Shutdown @ 0x1007979c4, GetCurrentMonthlyIncome @ 0x1007979f8
    //   cSC4OrdinanceJuniorSportsLeague: Shutdown @ 0x1007962d4, GetCurrentMonthlyIncome @ 0x100796308, CheckConditions @ 0x1007963d4
    //   cSC4OrdinanceTouristPromotion: Init @ 0x10079aba4, Shutdown @ 0x10079abae, Simulate @ 0x10079abe2, GetCurrentMonthlyIncome @ 0x10079ac02
    //   cSC4OrdinanceTrashPresort: Init @ 0x10079af68, Shutdown @ 0x10079af72, Simulate @ 0x10079afc4, GetCurrentMonthlyIncome @ 0x10079afe4, CheckConditions @ 0x10079b07c
    //   cSC4OrdinanceLandfillGasRecovery: Init @ 0x1007967c8, Shutdown @ 0x10079685a, Simulate @ 0x1007968ac, GetCurrentMonthlyIncome @ 0x1007968cc, CheckConditions @ 0x10079693a
    bool IsAvailable() const noexcept { return fieldD9 != 0u; }
    bool IsOn() const noexcept { return fieldD9 != 0u && fieldDA != 0u; }
    bool IsEnabled() const noexcept { return fieldDB != 0u; }
    bool IsIncomeOrdinance() const noexcept { return fieldD8 != 0u; }
    void SetAvailable(bool available) noexcept;
    void SetOn(bool on) noexcept;
    bool CheckConditions(Uint32 currentYear) const noexcept;
    Sint64 GetCurrentMonthlyIncome(Sint64 scalingBaseValue) const noexcept;
    Sint64 Simulate(Sint64 scalingBaseValue) noexcept;
};

// Records whose body is only the leading Uint16 version
// (cSC4Demolition, cSC4AutomataScriptSystem).
struct VersionOnlySaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4WealthRequester: Read/Write stream nothing.
struct EmptySaveRecord {
    Uint32 memAddr = 0;

    bool Read(cIGZIStream&) noexcept { return true; }
    bool Write(cIGZOStream&) const { return true; }
};

struct CityDetailManagerSaveRecord {
    Uint32 memAddr = 0;
    Uint32 version = 0;   // constant Uint32 0
    Uint32 field10 = 0;   // +0x10

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct TerrainView3DSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    Uint8 displayGrid = 0; // sbDisplayGrid

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct TerrainSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    SimSerializableRef terrainView{}; // cSTETerrainView3D

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct PortDeveloperEntry {
    Uint32 field0 = 0;
    Uint16 field4 = 0;
    Sint16 field6 = 0;
    Sint16 field8 = 0;
};

// ---------------------------------------------------------------------------
// cSC4PortDeveloper (base of Airport/Seaport):
//   cSC4PortDeveloper() @ 0x1000cbe6a, ~cSC4PortDeveloper() @ 0x1000cbf1c /
//   0x1000cbf92 / 0x1000cbf98, Init @ 0x1000cbf9e, readTunableVars @ 0x1000cc128,
//   Shutdown @ 0x1000cc26e, AddRef @ 0x1000cc44e, Release @ 0x1000cc478,
//   FinalRelease @ 0x1000cc4d4, QueryInterface @ 0x1000cc4fe,
//   Write @ 0x1000cc564, Read @ 0x1000cc62a, DoMessage @ 0x1000cc7ba,
//   doMonthlySimulation @ 0x1000cc950, doLotStateChanged @ 0x1000ccc0e,
//   DoLotInserted @ 0x1000ccc72, GetRequiredBudget @ 0x1000ccddc,
//   GetAvgEfficiency @ 0x1000cce10, GetPortUsageValues @ 0x1000cce78,
//   RecordHistoryData @ 0x1000ccfaa, GrowPortsToNextStage @ 0x1000cd04e,
//   swapLot @ 0x1000cd116, removeLot @ 0x1000cd28a, findPortLot @ 0x1000cd5fc,
//   loadLotConfiguration @ 0x1000cd68a, GetNumPortsBySize @ 0x1000cdb42,
//   GetNumPorts @ 0x1000cdb62, GetLeastEfficientPort @ 0x1000cdb98,
//   GetAllPortLots @ 0x1000cdbf6, GetPortLot @ 0x1000cdc74,
//   SendAdvisorMessage @ 0x1000cddea.
// ---------------------------------------------------------------------------
struct PortDeveloperSaveRecord {
    Uint16 version = 1;
    Uint32 field20 = 0; // +0x20
    Uint16 field58 = 0; // +0x58
    std::vector<PortDeveloperEntry> entries;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;

    static Float32 ComputePortEfficiency(Uint32 currentTrips,
                                         Uint32 stageMaxCapacity,
                                         Float32 fundingFraction) noexcept {
        // Exact cSC4PortDeveloper::doMonthlySimulation @ 0x1000cc950:
        // effCapacity = (Float32)stageMaxCapacity * fundingFraction;
        // if (currentTrips == 0) return 1.0f;
        // if (effCapacity <= 0.0f || (Float32)currentTrips > effCapacity) return 0.0f;
        // return 1.0f - (Float32)currentTrips / effCapacity;
        if (currentTrips == 0u) return 1.0f;
        const Float32 effCap =
            static_cast<Float32>(stageMaxCapacity) * fundingFraction;
        if (effCap <= 0.0f || static_cast<Float32>(currentTrips) > effCap) {
            return 0.0f;
        }
        return 1.0f - (static_cast<Float32>(currentTrips) / effCap);
    }
};

// cSC4AirportDeveloper (CLSID 0xaa2049a1) / cSC4SeaportDeveloper (CLSID 0xea230a28):
//   cSC4AirportDeveloper:
//     ~cSC4AirportDeveloper() @ 0x1000c80de / 0x1000c817a, Init @ 0x1000c81fc,
//     Shutdown @ 0x1000c8362, QI @ 0x1000c8430, AddRef @ 0x1000c84c4,
//     Release @ 0x1000c84f8, FinalRelease @ 0x1000c852c, DoMessage @ 0x1000c8544,
//     Write @ 0x1000c85d0, Read @ 0x1000c8656, GetAvgEfficiency @ 0x1000c872a,
//     OnStageChange @ 0x1000c8742, OnNewLotCreated @ 0x1000c874e,
//     SendAdvisorUpgradeMessage @ 0x1000c875a, GetNumAirports @ 0x1000c87c2,
//     GetTripsForPort @ 0x1000c87dc, GetLeastEfficientPort @ 0x1000c87f0,
//     UpdateBudget @ 0x1000c8808, GetAllocatedFunding @ 0x1000c8880,
//     GetAirportUsageValues @ 0x1000c88c0, GetNumAirportTrips @ 0x1000c895e,
//     GetMaintenanceCost @ 0x1000c89a2, DoMonthlyTick @ 0x1000c8a46,
//     GetSimulatorType @ 0x1000c8e5c, GetAllPortLots @ 0x1000c8e68,
//     GetNumPorts @ 0x1000c8e76, AsISC4PortDeveloper @ 0x1000c8e84.
//   cSC4SeaportDeveloper:
//     cSC4SeaportDeveloper() @ 0x1000ca01e, ~cSC4SeaportDeveloper() @ 0x1000ca0cc /
//     0x1000ca140 / 0x1000ca158, Init @ 0x1000ca194, BuildWaterAccessMap @ 0x1000ca290,
//     Shutdown @ 0x1000ca624, QI @ 0x1000ca6a6, AddRef @ 0x1000ca73a,
//     Release @ 0x1000ca76e, FinalRelease @ 0x1000ca7a2, DoMessage @ 0x1000ca7ba,
//     Write @ 0x1000ca82c, Read @ 0x1000ca8b2, SendAdvisorUpgradeMessage @ 0x1000ca986,
//     GetAvgEfficiency @ 0x1000ca9aa, UpdateTripCapacity @ 0x1000ca9c2,
//     OnStageChange @ 0x1000caa8e, OnNewLotCreated @ 0x1000caaa6,
//     GetTripsForPort @ 0x1000cae76, GetLeastEfficientPort @ 0x1000caeee,
//     DoMonthlyTick @ 0x1000caf06, CalculateWaterAccessible @ 0x1000cafdc,
//     SetLotPropsState @ 0x1000cb058, UpdateBudget @ 0x1000cb146,
//     GetAllocatedFunding @ 0x1000cb1be, GetSeaportUsageValues @ 0x1000cb1fe,
//     CityHasSea @ 0x1000cb29c, GetCellSeaAccess @ 0x1000cb2a8,
//     GetLotProps @ 0x1000cb2c2, GetNumSeaportTrips @ 0x1000cb4ae,
//     GetMaintenanceCost @ 0x1000cb4f2, GetSimulatorType @ 0x1000cb596,
//     GetAllPortLots @ 0x1000cb5a2, GetNumPorts @ 0x1000cb5b0,
//     AsISC4PortDeveloper @ 0x1000cb5be, GetNumSeaports @ 0x1000cb5c8.
struct PortDeveloperSubclassSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    PortDeveloperSaveRecord port;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct GarbageHeapSaveRecord {
    Uint16 version = 2;
    Uint32 field8 = 0;   // +0x08
    Uint32 fieldC = 0;   // +0x0c
    Uint16 field10 = 0;  // +0x10 (byte widened to Uint16)
    Uint16 field11 = 0;  // +0x11 (byte widened to Uint16)
    Uint32 field1C = 0;  // +0x1c
    Uint32 field18 = 0;  // +0x18

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct LandfillZoneSaveRecord {
    Uint16 version = 4;
    SC4Rect<Sint32> rect{};
    std::vector<Uint8> cells;   // cRZCellMap<bool> over rect, x outer, y inner
    Sint32 field20 = 0;         // +0x20 heap-grid dimension
    Sint32 field1C = 0;         // +0x1c heap-grid dimension
    std::vector<GarbageHeapSaveRecord> heaps; // +0x28, count +0x18 = +0x1c * +0x20
    Uint8 field41 = 0;          // +0x41 (version 3: Uint8; version >= 4: bool)
    SimSerializableRef ref60{}; // +0x60 (version >= 4)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const { return WriteWithVersion(stream, 4); }
    // Legacy-preserving encoder (the binary always writes version 4).
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;
};

// cSC4LandfillDeveloper (CLSID 0xea09d4ae):
//   UpdateCapacityValues @ 0x1000609ec, QI @ 0x100060a94, AddRef @ 0x100060af4,
//   Release @ 0x100060b1e, FinalRelease @ 0x100060b3a, Init @ 0x100060c6c,
//   ReadTuningParams @ 0x100060e34, Shutdown @ 0x100060f4c, Write @ 0x1000610dc,
//   Read @ 0x100061146, DoMessage @ 0x100061236, DoMonthlySimulation @ 0x1000614e4,
//   DoZoneChange @ 0x1000616a2, UpdateBudgetRequirements @ 0x10006197a,
//   DoTerrainChanged @ 0x100061a2c, RemoveZone @ 0x100061b22,
//   HandleZoneOverlap @ 0x100061c12, GetAltitudeVariance @ 0x100061cb8,
//   GetZoningCost @ 0x100061e70, GetCurrentBudget @ 0x100062012,
//   RecordHistoryData @ 0x100062032, DezoneArea @ 0x100062120,
//   GetLandfillZoneFromCell @ 0x1000621d8, GetEfficiency @ 0x10006221a,
//   GetLandfillTileCount @ 0x100062252, GetSimulatorType @ 0x100062270,
//   GetMaintenanceCosts @ 0x10006227c, GetLandfillUsage @ 0x100062286.
struct LandfillDeveloperSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 3;
    std::vector<LandfillZoneSaveRecord> zones;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    // Re-encodes every zone at its stored version (byte-exact for legacy saves).
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;

    static Float32 ComputeEfficiency(Sint64 currentBudget,
                                     Sint64 requiredMaintenanceCost) noexcept {
        // Exact cSC4LandfillDeveloper::GetEfficiency @ 0x10006221a.
        if (requiredMaintenanceCost <= 0) return 1.0f;
        return static_cast<Float32>(currentBudget) /
               static_cast<Float32>(requiredMaintenanceCost);
    }
};

struct ParkManagerSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    std::vector<SimSerializableRef> list28; // ilist<cISC4Occupant*>
    std::vector<SimSerializableRef> list40;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct PropManagerSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    SimSerializableRef refA{};
    SimSerializableRef refB{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct BusinessUnitSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    std::vector<SimSerializableRef> refs;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct TutorialSystemSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    Uint8 field2F = 0;
    Sint32 field30 = 0;
    Sint32 field34 = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct CitySituationManagerSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    std::vector<std::pair<Uint32, Uint16>> situations;
    Uint8 fieldA18 = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC424HourClock (current version 2; version <= 1 also carries a cRZTimer,
// operator>>(cIGZIStream&, cRZTimer&) @ 0x1000ce7ca: Uint8 units,
// Uint64 elapsed, Uint8 running).
struct TwentyFourHourClockSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    Sint32 field40 = 0;
    Float32 field48 = 0.0f;
    Float32 field50 = 0.0f;
    Uint32 field54 = 0;
    Uint8 legacyTimerUnits = 0;
    Uint64 legacyTimerElapsed = 0;
    Uint8 legacyTimerRunning = 0;
    Uint32 field78 = 0;
    Uint32 field7C = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const { return WriteWithVersion(stream, 2); }
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;
};

struct CameraControlSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    S3DVector3 field4C{};
    std::array<Sint32, 6> field108{}; // +0x108..+0x11c

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct NeighborConnectionSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    Uint32 field44 = 0;
    Sint32 field48 = 0;
    Sint32 field4C = 0;
    Sint32 field50 = 0;
    Sint32 field54 = 0;
    Uint8 field58 = 0;
    SerializedPropertyHolder properties;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4NeighborDeal v5. Note that the binary streams +0x64 twice.
struct NeighborDealSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 5;
    Uint32 field3C = 0;
    Sint32 field40 = 0;
    Sint32 field44 = 0;
    Sint32 field50 = 0;
    Sint32 field64 = 0;
    Sint64 field68 = 0;
    Sint32 fieldB0 = 0;
    Sint32 field48 = 0;
    Sint32 field4C = 0;
    Sint32 field54 = 0;
    Sint32 field5C = 0;
    Sint32 field64Again = 0;
    Uint32 field98 = 0;
    Uint32 field78 = 0;
    Uint32 field88 = 0;
    SerializedPropertyHolder properties; // +0x10
    Uint32 fieldA8 = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const { return WriteWithVersion(stream, 5); }
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;
};
// count (size of the first row), then every row's refs.
// Exact cSC4ZoneDeveloper (CLSID 0x498f9b01):
//   cSC4ZoneDeveloper() @ 0x100128098, ~cSC4ZoneDeveloper() @ 0x10012826e /
//   0x100128426 / 0x100128430, QI @ 0x10012845e, AddRef @ 0x1001284be, Release @ 0x1001284e8,
//   Init @ 0x10012853c, InitBoundaryRules @ 0x100128b94, Shutdown @ 0x1001291fe,
//   ClearLotMap @ 0x1001294de, DoMessage @ 0x10012959a,
//   CreateLotRemnants @ 0x100129956, ConnectRoadToAnyAdjacentStreets @ 0x10012a040,
//   ConnectStreetsAcrossRail @ 0x10012a534, PlopAllLotTypes @ 0x10012abe4,
//   PlopAllBuildingTypes @ 0x10012ad80, DoParcellization @ 0x10012b046,
//   ClearHighlight @ 0x10012b0aa, OnZone @ 0x10012b160, OnDezone @ 0x10012b976,
//   DeferOccupantOperations @ 0x10012bdac, DeleteLot @ 0x10012be32,
//   DeleteLots @ 0x10012c130, GetLotVertexAltitudeAdjustmentRange @ 0x10012c1d0,
//   SimpleParcellize @ 0x10012c320, DrawStreet @ 0x10012c61c,
//   CreateLotToFaceRoad @ 0x10012cbcc, ExamineBoundaries @ 0x10012ce50,
//   CreateParcellizableRegion @ 0x10012d184, CompressRegion @ 0x10012d3ac,
//   DetermineLotSize @ 0x10012d606, ExtendAsStreets @ 0x10012dec2,
//   LayInJogStreets @ 0x10012e0e8, SubdivideTheZone @ 0x10012ecc2,
//   Parcellize @ 0x10012ee06, ExtractConnectedCells @ 0x10012fa04,
//   UpdateZoneType @ 0x10012feac, ExistsConnectionOfType @ 0x100130274,
//   DoesCellHaveLot @ 0x100130354, ExamineCell @ 0x1001303ea,
//   GetNetwork @ 0x100130604, CanCreateLotOnCell @ 0x1001307bc,
//   IsCellWater @ 0x1001308ea, HighlightParcels @ 0x100130914,
//   UpdateHighlightTextures @ 0x100130994, GetBoundaryLotTexture @ 0x100130e66,
//   ExistsNetworkOfType @ 0x100130f02, GetRandomLot @ 0x100131040,
//   GetOccupantLot @ 0x1001310b4, GetLot(int, int, bool) @ 0x100131192,
//   GetLot(cS3DVector3 const&) @ 0x10013122a, GetLotsInCellRect @ 0x1001312a8,
//   GetLotsSurroundingRect @ 0x100131514,
//   GetLotsSurroundingLot(cISC4Lot const*, list&, int) @ 0x100131800,
//   GetLotsSurroundingLot(int, int, list&, int) @ 0x100131896,
//   GetVerticesAtEdgeOfLot @ 0x100131946,
//   GetNeighborCellsAtEdgeOfLot @ 0x1001319a6,
//   GetLotSavvyTerrainVertexAltitude @ 0x100131a5e,
//   GetLotSavvyTerrainAltitude @ 0x100131c10,
//   GetLotSavvyCellAltitude @ 0x100131d20,
//   CanCreateLot @ 0x100131de6, CanCreatePloppedLot @ 0x100131ecc,
//   CheckLotTerrainConstraints @ 0x100132396,
//   CreateLotIfPossible @ 0x100132b32, CreateLot @ 0x100132cde,
//   Write @ 0x100132f32, Read @ 0x100132ffc.
struct ZoneDeveloperSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    std::vector<std::vector<SimSerializableRef>> rows;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct OccupantStandaloneSaveRecord {
    Uint32 memAddr = 0;
    OccupantBaseSaveRecord base;

    bool Read(cIGZIStream& stream) noexcept { return base.Read(stream); }
    bool Write(cIGZOStream& stream) const { return base.Write(stream); }
};

struct ThreeDRenderSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    Sint32 fieldB4 = 0;
    Sint32 fieldB8 = 0;
    std::array<Sint32, 4> fieldBC{};
    SimSerializableRef refE0{};
    Uint32 fieldE8 = 0;
    Uint32 fieldEC = 0;
    Sint32 field138 = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct DispatchInfoRecord {
    Sint32 field0 = 0;
    Uint32 field4 = 0;
    Uint32 field8 = 0;
    Uint8 field10 = 0;
    Float32 field14 = 0.0f;
    Float32 field18 = 0.0f;
    Float32 field1C = 0.0f;
    SimSerializableRef ref20{};
    SimSerializableRef ref28{};
    SimSerializableRef ref30{};
};

struct DispatchManagerSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    std::vector<DispatchInfoRecord> dispatches; // +0x40
    Uint32 field50 = 0;
    SimSerializableRef ref60{};
    std::vector<SimSerializableRef> refs;
    Uint32 field80 = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct LineItemSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 4;
    std::string name18;
    Sint64 field50 = 0;
    Sint64 field58 = 0;
    Sint64 field60 = 0;
    Sint64 field68 = 0;
    Uint32 field74 = 0;
    Uint32 field78 = 0;
    Uint8 field7C = 0;
    Uint16 field88 = 0;
    SimSerializableRef ref80{};
    Uint32 field40 = 0;
    Uint32 field44 = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;

    // GetID @ 0x100245e4e
    Uint32 GetID() const noexcept { return field74; }

    // GetType @ 0x100245e58. Live object reads the dword at +0x88; the save
    // image stores that value as a Uint16.
    Uint32 GetType() const noexcept { return field88; }

    // SetType @ 0x100245e64
    void SetType(Uint32 type) noexcept { field88 = static_cast<Uint16>(type); }

    // GetSecondaryInfoField @ 0x100245e70
    Sint64 GetSecondaryInfoField() const noexcept { return field50; }

    // SetSecondaryInfoField @ 0x100245e7a
    void SetSecondaryInfoField(Sint64 value) noexcept { field50 = value; }

    // GetFullExpenses @ 0x100245e84
    Sint64 GetFullExpenses() const noexcept { return field58; }

    // GetCurrentExpenses @ 0x100245e8e
    Sint64 GetCurrentExpenses() const noexcept { return field60; }

    // GetIncome @ 0x100245e98
    Sint64 GetIncome() const noexcept { return field68; }

    // GetDisplayFlags @ 0x100245ea2
    Uint32 GetDisplayFlags() const noexcept { return field78; }

    // SetDisplayFlag @ 0x100245eac. Flag is a bit mask. enable ORs it in;
    // a clear ANDs it out.
    void SetDisplayFlag(Uint32 flag, bool enable) noexcept {
        const Uint32 withBit = field78 | flag;
        const Uint32 withoutBit = field78 & ~flag;
        field78 = enable ? withBit : withoutBit;
    }

    // IsLocalFundingItem @ 0x100245ec6
    bool IsLocalFundingItem() const noexcept { return field7C != 0u; }

    // SetFullExpenses @ 0x100245a6c. Stores full expenses at +0x58.
    // When the item is not locally funded, the live method also sets current
    // expenses to trunc(amount * departmentFundingFactor) and posts
    // SendBudgetChangeMessage(0x08dac3e8, 0) on the department at +0x80.
    // This record keeps the serialized current expenses.
    void SetFullExpenses(Sint64 amount) noexcept { field58 = amount; }

    // AddToFullExpenses @ 0x100245ad8. Tail-calls SetFullExpenses(full + delta).
    void AddToFullExpenses(Sint64 delta) noexcept { SetFullExpenses(field58 + delta); }

    // SetIncome @ 0x100245aea. Negative income is clamped to 0, then the live
    // method posts SendBudgetChangeMessage(0x08dac3e8, 0).
    void SetIncome(Sint64 income) noexcept { field68 = income < 0 ? Sint64{0} : income; }

    // AddToIncome @ 0x100245b10. Adds delta with no clamp, then posts the same
    // budget-change message.
    void AddToIncome(Sint64 delta) noexcept { field68 += delta; }

    // SetName @ 0x100245d36. groupId is stored at +0x44 and instanceId at +0x40.
    // A non-zero group resolves through cSLString; a zero group copies the
    // building-development sim name for instanceId into the name at +0x18.
    void SetName(Uint32 groupId, Uint32 instanceId) noexcept {
        field44 = groupId;
        field40 = instanceId;
    }

    // GetName @ 0x100245dc0. Copies the serialized name. The live object
    // refreshes it from the stored ids when the runtime flag at +0x48 is set.
    void GetName(std::string& out) const { out = name18; }
};

// cSC4DepartmentBudget (current version 10; version 9 has the same layout but
// Read rescales the two floats).
// Complete method index:
//   cSC4DepartmentBudget() @ 0x10024347a, ~cSC4DepartmentBudget() @ 0x10024352c /
//   0x100243592 / 0x10024359c, QI @ 0x1002435ca, AddRef @ 0x100243618, Release @ 0x100243634,
//   Init @ 0x100243678, Shutdown @ 0x10024368a, Write @ 0x1002437e4, Read @ 0x100243996,
//   GetDepartmentID @ 0x100243d08, SetDepartmentID @ 0x100243d12,
//   GetDepartmentName @ 0x100243d1c, SetDepartmentName @ 0x100243d34,
//   GetFundingPercentage @ 0x100243d94, SetFundingPercentage @ 0x100243d9e / 0x100245e26,
//   DoDepartmentFundingChanged @ 0x100243ee4,
//   SendBudgetChangeMessage @ 0x1002443a8,
//   GetMaxAllowedFundingPercentage @ 0x1002444a2,
//   SetMaxAllowedFundingPercentage @ 0x1002444ac,
//   GetIdealMonthlyFunding @ 0x1002444b8, GetTotalExpenses @ 0x10024452c,
//   GetTotalIncome @ 0x1002445a0, GetTotalSpending @ 0x100244616,
//   SetTotalSpending @ 0x100244620, CreateLineItem @ 0x10024462a,
//   CreateLineItemForBuildingType @ 0x100244714, RemoveLineItem @ 0x100244744,
//   GetLineItem @ 0x1002447e2, GetAllLineItems @ 0x100244822,
//   AddLocallyFundedObject @ 0x10024499e,
//   RemoveLocallyFundedObject @ 0x100244b00,
//   IsLocallyFundedObjectInDepartment @ 0x100244ba4,
//   SetLocalFundingPercent @ 0x100244bd4,
//   RecalculateLineItemCurrentExpenses @ 0x100244e28,
//   RecalculateAllLineItemCurrentExpenses @ 0x100244fba,
//   SetLocalFullFunding @ 0x100245482, GetLocalFundingPercent @ 0x10024563a,
//   GetLocalFullFunding @ 0x10024569c,
//   GetLocallyFundedItemsByPurpose @ 0x100245700,
//   HasLocallyFundedItemsByPurpose @ 0x100245830,
//   GetBudgetGroup @ 0x100245dfe, SetBudgetGroup @ 0x100245e08,
//   GetIsFixedFunding @ 0x100245e12, SetFixedFunding @ 0x100245e1c.
struct DepartmentBudgetSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 10;
    Uint8 field10 = 0;
    Uint32 field18 = 0;
    std::string name20;
    Uint32 field50 = 0;
    Float32 field54 = 0.0f;
    Float32 field58 = 0.0f;
    Sint64 field60 = 0;
    Uint8 field68 = 0;
    std::vector<SimSerializableRef> lineItems;                  // map<Uint32, cSC4LineItem*>
    std::vector<std::pair<SimSerializableRef, Uint32>> holders; // set<pair<cISCPropertyHolder*, Uint32>>
    Uint32 field48 = 0;
    Uint32 field4C = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const { return WriteWithVersion(stream, 10); }
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;

    Uint32 GetDepartmentID() const noexcept { return field18; }
    void SetDepartmentID(Uint32 id) noexcept { field18 = id; }
    Float32 GetFundingPercentage() const noexcept { return field54; }
    Float32 GetMaxAllowedFundingPercentage() const noexcept { return field58; }
    void SetMaxAllowedFundingPercentage(Float32 pct) noexcept { field58 = pct; }
    Sint64 GetTotalSpending() const noexcept { return field60; }
    void SetTotalSpending(Sint64 s) noexcept { field60 = s; }
    Uint32 GetBudgetGroup() const noexcept { return field50; }
    void SetBudgetGroup(Uint32 g) noexcept { field50 = g; }
    bool GetIsFixedFunding() const noexcept { return field68 != 0u; }
    void SetFixedFunding(bool fixed) noexcept { field68 = fixed ? 1u : 0u; }
    bool SetFundingPercentage(Float32 pct) noexcept {
        if (field68 != 0u) return false;
        const Float32 clamped = std::clamp(pct, 0.0f, field58);
        if (clamped == field54) return true;
        field54 = clamped;
        return true;
    }
};

struct HistoryWarehouseEntry {
    Uint32 key = 0;
    Uint32 field40 = 0;
    std::vector<std::pair<Uint32, Sint32>> nodes; // (date, value)
};

struct HistoryWarehouseSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    std::vector<HistoryWarehouseEntry> histories;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const { return WriteWithVersion(stream, 2); }
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;
};

// cSC4Demand (current version 2; version 1 stores +0x4c as a Uint8).
struct DemandSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    Uint32 field18 = 0;
    Float32 field1C = 0.0f;
    Float32 field20 = 0.0f;
    Float32 field24 = 0.0f;
    Float32 field28 = 0.0f;
    Float32 field2C = 0.0f;
    Float32 field30 = 0.0f;
    Float32 field38 = 0.0f;
    Float32 field34 = 0.0f;
    Float32 field3C = 0.0f;
    Uint8 field40 = 0;
    Float32 field44 = 0.0f;
    Float32 field48 = 0.0f;
    Float32 field4C = 0.0f;
    Uint8 legacyField4C = 0; // version 1 byte

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const { return WriteWithVersion(stream, 2); }
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;
};

struct ModelMakerSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    SimSerializableRef ref20{};
    std::array<Sint32, 4> field38{};
    SC4Rect<Sint32> rect48{};
    Uint32 field58 = 0;
    SimSerializableRef ref70{};
    SimSerializableRef ref98{};
    SimSerializableRef refA0{};
    SimSerializableRef refA8{};
    std::array<Float32, 4> field15C{};
    std::array<Sint32, 4> field16C{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct PropDeveloperHashEntry {
    Uint32 key = 0;
    std::vector<SimSerializableRef> props;
};

// cSC4PropDeveloper (current version 5; version 4 lacks the +0xb00 bool).
// Complete method index:
//   cSC4PropDeveloper() @ 0x1001217ce, ~cSC4PropDeveloper() @ 0x1001219fc /
//   0x100121ae2 / 0x100121aec, AddRef @ 0x100121b1a, Release @ 0x100121b44,
//   QueryInterface @ 0x100121ba2, Init @ 0x100121c04, Shutdown @ 0x100121f7e,
//   Write @ 0x1001222c8, Read @ 0x1001225d0, AddPropRequester @ 0x100122d70,
//   RemovePropRequester @ 0x100122dc8, DoMessage @ 0x100122e36,
//   DoIdle @ 0x1001235a0, HandleInsertOccupant @ 0x100123632,
//   HandleRemoveOccupant @ 0x100123edc,
//   HandleRemoveNetworkOccupant @ 0x10012428c,
//   DoAnimationTick @ 0x1001243ae, SwitchNightChangeProps @ 0x1001245e6,
//   RemoveOptionalPropsInLot @ 0x10012471c, DoPowerChanged @ 0x100124842,
//   UpdateLotStateProps @ 0x100124b06, UpdateSignpostHeights @ 0x100124bfe,
//   UpdateTract @ 0x100124d32, SelectAndCreateProp @ 0x100125d30,
//   UpdateAllOptionalProps @ 0x1001260b2, ShowSignsAndLabels @ 0x10012612a,
//   ToggleSignsAndLabels @ 0x100126448, AreSignsAndLabelsVisible @ 0x100126464.
struct PropDeveloperSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 5;
    Sint8 field20 = 0;
    ScanningIteratorState scanningIterator{}; // +0x24
    Uint8 fieldA18 = 0;
    std::vector<SimSerializableRef> requestersA20;
    SimSerializableRef refA38{};
    SimSerializableRef refA40{};
    SimSerializableRef refA50{};
    Uint32 fieldA70 = 0;
    Uint32 fieldA74 = 0;
    Uint32 fieldA78 = 0;
    Uint32 fieldA7C = 0;
    Uint8 fieldA80 = 0;
    std::vector<SimSerializableRef> propsA58;
    std::vector<PropDeveloperHashEntry> hashA90;
    std::vector<SimSerializableRef> listAD8;
    std::vector<SimSerializableRef> listAC0;
    std::vector<SimSerializableRef> listAF0;
    Uint8 fieldB00 = 0; // version >= 5 (with listB10)
    std::vector<SimSerializableRef> listB10;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const { return WriteWithVersion(stream, 5); }
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;

    bool AreSignsAndLabelsVisible() const noexcept { return fieldB00 != 0u; }
    void ShowSignsAndLabels(bool visible) noexcept { fieldB00 = visible ? 1u : 0u; }
    void ToggleSignsAndLabels() noexcept { ShowSignsAndLabels(!AreSignsAndLabelsVisible()); }
};

struct ZoneManagerMapEntry {
    Sint8 field1C = 0;
    Sint8 field1D = 0;
    Uint32 field20 = 0;
};

struct ZoneManagerSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    SimSerializableRef ref70{};
    std::array<Sint32, 16> field78{};
    std::array<Sint32, 16> fieldB8{};
    std::array<Sint32, 16> fieldF8{};
    Sint32 field138 = 0;
    Sint32 field13C = 0;
    Sint32 field140 = 0;
    Sint32 field144 = 0;
    std::array<Sint32, 16> field148{};
    std::array<Sint32, 16> field188{};
    std::array<Sint64, 16> field208{};
    std::array<Sint64, 16> field288{};
    std::array<Uint32, 16> field308{};
    std::array<Uint32, 16> field348{};
    Uint8 field388 = 0;
    std::vector<ZoneManagerMapEntry> map390;
    std::vector<ZoneManagerMapEntry> map3A8;
    std::array<SimSerializableRef, 5> refs30{}; // +0x30..+0x50

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// Returns false if typeId is not a Batch 2 class.
bool RoundTripSC4SubfileBatch2(Uint32 typeId, std::span<const Uint8> subfileBytes,
                               SC4SubfileRoundTripStats& stats);

// ---------------------------------------------------------------------------
// Wave 2C method index.
// Every algorithmic and leaf entry point from the twenty assigned workpacks.
// 951 citations, 951 unique addresses.
// ---------------------------------------------------------------------------
//
// cSC4CitySituationManager (117 entry points)
//   cSC4CitySituationManager::cSC4CitySituationManager() @ 0x1006d6d06 / 0x1006d6f7c
//   cSC4CitySituationManager::~cSC4CitySituationManager() @ 0x1006d6f86 / 0x1006d732e / 0x1006d7338
//   cSC4CitySituationManager::Init() @ 0x1006d7366
//   cSC4CitySituationManager::sGetSuccessCount(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x1006d7f98
//   cSC4CitySituationManager::sActivateSituation(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x1006d805e
//   cSC4CitySituationManager::sGetCurrentTarget(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x1006d81b8
//   cSC4CitySituationManager::sGetActiveAuto(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x1006d8248
//   cSC4CitySituationManager::sGetTargetName(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x1006d82d8
//   cSC4CitySituationManager::sShowCSI(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x1006d83ec
//   cSC4CitySituationManager::sGetNumTargetsRemaining(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x1006d84c8
//   cSC4CitySituationManager::Shutdown() @ 0x1006d85c6
//   cSC4CitySituationManager::FreeCoverageCells() @ 0x1006d8946
//   cSC4CitySituationManager::ClearTrackedAutomata() @ 0x1006d899a
//   cSC4CitySituationManager::ClearSituationTrackList() @ 0x1006d89d8
//   cSC4CitySituationManager::GetMissionSuccessCount(unsigned int) const @ 0x1006d8a60
//   cSC4CitySituationManager::GetCurrentTargetName() const @ 0x1006d8a94
//   cSC4CitySituationManager::GetSituationAura(float&, float&) @ 0x1006d8b54
//   cSC4CitySituationManager::GetSituationMoney(long long&) @ 0x1006d8b8a
//   cSC4CitySituationManager::DoMessage(cGZMessage&) @ 0x1006d8bc0
//   cSC4CitySituationManager::DoMessage(cIGZMessage2*) @ 0x1006d8bd0
//   cSC4CitySituationManager::HandleSituationTriggered(cISC4CitySituationAdvice*) @ 0x1006d9318
//   cSC4CitySituationManager::HandleSituationUntriggered(cISC4CitySituationAdvice*) @ 0x1006d96c4
//   cSC4CitySituationManager::DoAnimationTick(float) @ 0x1006d988c
//   cSC4CitySituationManager::HandleInsertOccupant(cISC4Occupant*) @ 0x1006d9c1c
//   cSC4CitySituationManager::HandleRemoveOccupant(cISC4Occupant*) @ 0x1006d9cb0
//   cSC4CitySituationManager::HandleAutomatonMoved(cISC4Automaton*) @ 0x1006d9dac
//   cSC4CitySituationManager::HandleAutomatonDisabled(cISC4Automaton*) @ 0x1006d9dfc
//   cSC4CitySituationManager::EndDialog(cIGZWin*) @ 0x1006d9ece
//   cSC4CitySituationManager::ShowSituationPopup(cISC4CitySituationAdvice*, cISC4Automaton*) @ 0x1006da008
//   cSC4CitySituationManager::ResetTutorials() @ 0x1006da36c
//   cSC4CitySituationManager::HandleSituationFailure(cISC4CitySituationAdvice*) @ 0x1006da3ca
//   cSC4CitySituationManager::HandleSituationSuccess(cISC4CitySituationAdvice*) @ 0x1006da64e
//   cSC4CitySituationManager::PostMessageToSelf(unsigned int, cISC4CitySituationAdvice*, cISC4Automaton*) @ 0x1006dab92
//   cSC4CitySituationManager::CheckForSuccess(float) @ 0x1006dac36
//   cSC4CitySituationManager::HandleStatusChanged(cISC4CitySituationManager::eSuccessResult) @ 0x1006dae70
//   cSC4CitySituationManager::UpdatePursuit(float) @ 0x1006daeec
//   cSC4CitySituationManager::SetMaxCSI(unsigned char) @ 0x1006db0de
//   cSC4CitySituationManager::GetMaxCSI() const @ 0x1006db10e
//   cSC4CitySituationManager::SetCSIVisible(bool) @ 0x1006db122
//   cSC4CitySituationManager::HandleTuningParametersChanged() @ 0x1006db14a
//   cSC4CitySituationManager::FindAutomatonForSituation(cISC4CitySituationAdvice*) @ 0x1006db16a
//   cSC4CitySituationManager::GetInstancesOfGroup(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>> const&, std::__1::unordered_set<cISC4Automaton*, cISC4AutomataManager::tAutomatonPtrHash, std::__1::equal_to<cISC4Automaton*>, std::__1::allocator<cISC4Automaton*>>&) @ 0x1006db27c
//   cSC4CitySituationManager::CanPlayerDrive(cISC4Automaton*) const @ 0x1006db35c
//   cSC4CitySituationManager::tSituationTrack::operator<(cSC4CitySituationManager::tSituationTrack const&) const @ 0x1006db496
//   cSC4CitySituationManager::TrackAutomaton(cISC4Automaton*, unsigned int&, bool) @ 0x1006db4ec
//   cSC4CitySituationManager::TreatAutomatonAsGroup(cISC4Automaton*, unsigned int) @ 0x1006db912
//   cSC4CitySituationManager::HandleInsertAutomaton(cISC4Automaton*) @ 0x1006db962
//   cSC4CitySituationManager::HandleRemoveAutomaton(cISC4Automaton*) @ 0x1006dbade
//   cSC4CitySituationManager::HandleTargetRemoved() @ 0x1006dbb72
//   cSC4CitySituationManager::ChooseNextTarget() @ 0x1006dbc26
//   cSC4CitySituationManager::FindTarget(bool) @ 0x1006dbcc6
//   cSC4CitySituationManager::SetTarget(cISC4Occupant*) @ 0x1006dc3f8
//   cSC4CitySituationManager::SendSituationMessage(unsigned int, cISC4CitySituationAdvice*, unsigned int) @ 0x1006dcf04
//   cSC4CitySituationManager::CallScriptFunction(int, int) @ 0x1006dcfa2
//   cSC4CitySituationManager::RetireSituation(cISC4CitySituationAdvice*) @ 0x1006dd074
//   cSC4CitySituationManager::IsSituationAvailable(cISC4Occupant*) const @ 0x1006dd0e4
//   cSC4CitySituationManager::IsSituationAvailable(unsigned int) const @ 0x1006dd270
//   cSC4CitySituationManager::InitiateSituationByGuid(unsigned int, bool, unsigned int) @ 0x1006dd3ce
//   cSC4CitySituationManager::CreateAutomatonForPlayerDrive(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>> const&) @ 0x1006dd6be
//   cSC4CitySituationManager::FindOccupantByGroup(unsigned int) @ 0x1006dd910
//   cSC4CitySituationManager::InitiateSituationForOccupant(cISC4Occupant*, cISC4CitySituationAdvice*) @ 0x1006ddbf0
//   cSC4CitySituationManager::InitiateSituationByAutomataGroup(unsigned int, bool, bool) @ 0x1006ddce4
//   cSC4CitySituationManager::StartJoyride(cISC4Automaton*, bool, cIGZMessageTarget2*) @ 0x1006ddf0e
//   cSC4CitySituationManager::InitiateSituation(cISC4CitySituationAdvice*, cISC4Automaton*, bool) @ 0x1006ddf50
//   cSC4CitySituationManager::SetActiveAutomaton(cISC4Automaton*) @ 0x1006de1a0
//   cSC4CitySituationManager::InitScriptRefs(cISC4CitySituationAdvice*) @ 0x1006de266
//   cSC4CitySituationManager::InitCoverageCells() @ 0x1006de570
//   cSC4CitySituationManager::CreateTarget(bool) @ 0x1006df0f0
//   cSC4CitySituationManager::ActivateSituation() @ 0x1006df3da
//   cSC4CitySituationManager::GetTimeLimitSecondsFromScript() @ 0x1006df5da
//   cSC4CitySituationManager::DeactivateSituation() @ 0x1006df78a
//   cSC4CitySituationManager::GetCurrentTargetGroups(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&) @ 0x1006df850
//   cSC4CitySituationManager::ChooseRandomOccupant(ilist<cISC4Occupant*>&) @ 0x1006df900
//   cSC4CitySituationManager::CallTargetScriptFunction(int, std::__1::vector<unsigned int, std::__1::allocator<unsigned int>> const&) @ 0x1006df942
//   cSC4CitySituationManager::InitLotCoverageCells() @ 0x1006dfb24
//   cSC4CitySituationManager::InitNetworkCoverageCells() @ 0x1006dfee2
//   cSC4CitySituationManager::InitZoneCoverageCells() @ 0x1006e0526
//   cSC4CitySituationManager::UpdateCoverageTarget() @ 0x1006e0e76
//   cSC4CitySituationManager::FindClosestTargetCell(SC4Point<unsigned short> const&, SC4Point<unsigned short>&) @ 0x1006e0fd8
//   cSC4CitySituationManager::WalkCoverageNetwork(SC4Point<unsigned short>, unsigned char, unsigned int, unsigned int&) @ 0x1006e1060
//   cSC4CitySituationManager::AddCellsToCoverage(SC4Rect<int>&, unsigned int, unsigned int) @ 0x1006e1460
//   cSC4CitySituationManager::FloodFillWaterZone(int, int, cISC4ZoneManager::ZoneType, std::__1::unordered_set<SC4Point<unsigned short>, cSC4CitySituationManager::hash_tCellPoint, std::__1::equal_to<SC4Point<unsigned short>>, std::__1::allocator<SC4Point<unsigned short>>>&, unsigned int&, unsigned int, int, int, cISC4SimGrid<signed char> const*) @ 0x1006e2020
//   cSC4CitySituationManager::FloodFillIsGoodCell(SC4Point<unsigned short>&, cISC4ZoneManager::ZoneType, std::__1::unordered_set<SC4Point<unsigned short>, cSC4CitySituationManager::hash_tCellPoint, std::__1::equal_to<SC4Point<unsigned short>>, std::__1::allocator<SC4Point<unsigned short>>>&, unsigned int&, unsigned int, int, int, cISC4SimGrid<signed char> const*) @ 0x1006e2d9a
//   cSC4CitySituationManager::GetTargetAsAutomaton() const @ 0x1006e2e08
//   cSC4CitySituationManager::CheckEvadeListSuccess(float) @ 0x1006e2f60
//   cSC4CitySituationManager::CheckReachTargetSuccess(float) @ 0x1006e313c
//   cSC4CitySituationManager::CheckEscapeCitySuccess(float) @ 0x1006e326a
//   cSC4CitySituationManager::CheckCellCoverageSuccess(float) @ 0x1006e350a
//   cSC4CitySituationManager::GetDistanceToTarget(bool) @ 0x1006e3884
//   cSC4CitySituationManager::GetActivePosition(cS3DVector3&) @ 0x1006e3c40
//   cSC4CitySituationManager::EndSituation() @ 0x1006e3d46
//   cSC4CitySituationManager::InitiatePlayerDriveMode(bool) @ 0x1006e3ea8
//   cSC4CitySituationManager::SetupDialog(cIGZWin*, cISC4CitySituationAdvice*, cISC4Automaton*) @ 0x1006e3f1a
//   cSC4CitySituationManager::DoWinProcMessage(cIGZWin*, cGZMessage&) @ 0x1006e5536
//   cSC4CitySituationManager::HandleJoyride(cIGZWin*) @ 0x1006e5628
//   cSC4CitySituationManager::HandleSituationCancelled() @ 0x1006e56dc
//   cSC4CitySituationManager::InitScriptFuncRef(char const*, int, int&) @ 0x1006e5774
//   cSC4CitySituationManager::QueryInterface(unsigned int, void**) @ 0x1006e57be
//   cSC4CitySituationManager::AddRef() @ 0x1006e5848
//   cSC4CitySituationManager::Release() @ 0x1006e588e
//   cSC4CitySituationManager::GetActiveSituation() const @ 0x1006e5d70
//   cSC4CitySituationManager::GetCurrentStatus() const @ 0x1006e5d7e
//   cSC4CitySituationManager::GetActiveAuto() const @ 0x1006e5d8a
//   cSC4CitySituationManager::GetCurrentTarget() const @ 0x1006e5d98
//   cSC4CitySituationManager::GetCurrentTargetIndex() const @ 0x1006e5da6
//   cSC4CitySituationManager::GetNumTargets() const @ 0x1006e5db2
//   cSC4CitySituationManager::GetNumTargetsRemaining() const @ 0x1006e5dbe
//   cSC4CitySituationManager::GetCurrentTimeLimitSeconds() const @ 0x1006e5de6
//   cSC4CitySituationManager::GetCurrentSituationTime() const @ 0x1006e5df4
//   cSC4CitySituationManager::GetCurrentTimeRemainingSeconds() const @ 0x1006e5e02
//   cSC4CitySituationManager::GetTrackedAutomataCount() const @ 0x1006e5e20
//   cSC4CitySituationManager::GetCSIVisible() const @ 0x1006e5e2c
//   cSC4CitySituationManager::CountdownIsInProgress() const @ 0x1006e5e38
//   cSC4CitySituationManager::tSituationTrack::tSituationTrack(cISC4CitySituationAdvice*) @ 0x1006e60e0
//
// cSC4AutomataScriptSystem (62 entry points)
//   cSC4AutomataScriptSystem::cSC4AutomataScriptSystem() @ 0x1001d5a90 / 0x1001d5bcc
//   cSC4AutomataScriptSystem::~cSC4AutomataScriptSystem() @ 0x1001d5bd6
//   cSC4AutomataScriptSystem::Init() @ 0x1001d5d0a
//   cSC4AutomataScriptSystem::CreateDefaultOccupantGroups() @ 0x1001d60d4
//   cSC4AutomataScriptSystem::ExecuteScripts() @ 0x1001d61e6
//   cSC4AutomataScriptSystem::sCreateAutomaton(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x1001d6542
//   cSC4AutomataScriptSystem::sCreateController(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x1001d6752
//   cSC4AutomataScriptSystem::sAttachController(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x1001d68ec
//   cSC4AutomataScriptSystem::sAttachControllerGroup(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x1001d6a7a
//   cSC4AutomataScriptSystem::sDetachController(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x1001d6bb6
//   cSC4AutomataScriptSystem::sDetachControllerGroup(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x1001d6cf0
//   cSC4AutomataScriptSystem::sGetSourceBuildingCount(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x1001d6e2a
//   cSC4AutomataScriptSystem::Shutdown() @ 0x1001d6f5a
//   cSC4AutomataScriptSystem::ClearTemplates() @ 0x1001d7086
//   cSC4AutomataScriptSystem::ClearOccupantGroups() @ 0x1001d70c4
//   cSC4AutomataScriptSystem::DoMessage(cIGZMessage2*) @ 0x1001d7256
//   cSC4AutomataScriptSystem::DoMessage(cGZMessage&) @ 0x1001d7280
//   cSC4AutomataScriptSystem::GetAutomataGroup(cRZString const&) @ 0x1001d72a0
//   cSC4AutomataScriptSystem::GetAutomataGroup(unsigned int) @ 0x1001d72c0
//   cSC4AutomataScriptSystem::CreateControllerTemplate(cRZString const&, unsigned int) @ 0x1001d72ce
//   cSC4AutomataScriptSystem::GetControllerTemplate(cRZString const&) @ 0x1001d771a
//   cSC4AutomataScriptSystem::GetOccupantGroup(cRZString const&) @ 0x1001d773e
//   cSC4AutomataScriptSystem::GetOccupantGroup(unsigned int) @ 0x1001d775e
//   cSC4AutomataScriptSystem::CreateOccupantGroup(cRZString const&, unsigned int, bool) @ 0x1001d7798
//   cSC4AutomataScriptSystem::InitScripts() @ 0x1001d8210
//   cSC4AutomataScriptSystem::ImportAttractorTemplates() @ 0x1001d8364
//   cSC4AutomataScriptSystem::ImportGeneratorTemplates() @ 0x1001d88be
//   cSC4AutomataScriptSystem::ImportAutomataGroups() @ 0x1001d8f52
//   cSC4AutomataScriptSystem::ImportOccupantGroups() @ 0x1001d99c4
//   cSC4AutomataScriptSystem::LinkControllerTemplates() @ 0x1001d9b9e
//   cSC4AutomataScriptSystem::LinkOccupantGroups() @ 0x1001d9c74
//   cSC4AutomataScriptSystem::ImportDriveCommandKeyMap() @ 0x1001d9f84
//   cSC4AutomataScriptSystem::ImportTrackBuildingGroups() @ 0x1001da37c
//   cSC4AutomataScriptSystem::ImportSingleOccupantGroup(cRZString const&, short) @ 0x1001da596
//   cSC4AutomataScriptSystem::ImportAirplaneParams(int, tAutomataDriveParams*) @ 0x1001daec4
//   cSC4AutomataScriptSystem::ImportHelicopterParams(int, tAutomataDriveParams*) @ 0x1001db158
//   cSC4AutomataScriptSystem::ImportWatercraftParams(int, tAutomataDriveParams*) @ 0x1001db34c
//   cSC4AutomataScriptSystem::ImportPlayerDriveParams(int, tAutomataPlayerDriveParams*) @ 0x1001db8b8
//   cSC4AutomataScriptSystem::ImportKeyEffectsTable(int, cSC4OccupantGroupTemplate*) @ 0x1001dbe26
//   cSC4AutomataScriptSystem::ReadGUIDTable(short, std::__1::list<cGZPersistResourceKey, std::__1::allocator<cGZPersistResourceKey>>&) @ 0x1001dc52e
//   cSC4AutomataScriptSystem::ImportAutomataGroupNameList(int, std::__1::list<cRZString, std::__1::allocator<cRZString>>&) @ 0x1001dc6ae
//   cSC4AutomataScriptSystem::ImportTimeOfDayCurve(int, tControllerTemplate*) @ 0x1001dc704
//   cSC4AutomataScriptSystem::ImportCalendarField(int, tControllerTemplate*) @ 0x1001dc902
//   cSC4AutomataScriptSystem::ImportBehaviorTable(int, tControllerTemplate*) @ 0x1001dc9ce
//   cSC4AutomataScriptSystem::ImportEffectTriggerList(int, tControllerTemplate*) @ 0x1001dd250
//   cSC4AutomataScriptSystem::ImportControllerLifetime(int, tControllerTemplate*) @ 0x1001dd504
//   cSC4AutomataScriptSystem::ImportLotFlags(int, tControllerTemplate*) @ 0x1001dd56c
//   cSC4AutomataScriptSystem::ImportSFXList(int, tControllerTemplate*) @ 0x1001dd7be
//   cSC4AutomataScriptSystem::ImportIgnoreStateList(int, tControllerTemplate*) @ 0x1001dd8ca
//   cSC4AutomataScriptSystem::GetHexFromVariant(cIGZVariant&) @ 0x1001ddc44
//   cSC4AutomataScriptSystem::GetDriveCommand(unsigned int) const @ 0x1001ddcfe
//   cSC4AutomataScriptSystem::QueryInterface(unsigned int, void**) @ 0x1001ddd30
//   cSC4AutomataScriptSystem::AddRef() @ 0x1001dddb0
//   cSC4AutomataScriptSystem::Release() @ 0x1001ddde8
//   cSC4AutomataScriptSystem::FinalRelease() @ 0x1001dde04
//   cSC4AutomataScriptSystem::GetAutomataGroupMap() const @ 0x1001de2b8
//   cSC4AutomataScriptSystem::GetOccupantGroupNameMap() const @ 0x1001de2c2
//   cSC4AutomataScriptSystem::GetOccupantGroupIDMap() const @ 0x1001de2cc
//   cSC4AutomataScriptSystem::GetSpecialFilterOccupantGroups() const @ 0x1001de2da
//   cSC4AutomataScriptSystem::GetControllerTemplateMap() const @ 0x1001de2e8
//   cSC4AutomataScriptSystem::GetTrackBuildingGroups() const @ 0x1001de2f6
//
// cSC4PropManager (56 entry points)
//   cSC4PropManager::~cSC4PropManager() @ 0x100193866 / 0x100193898
//   cSC4PropManager::AddRef() @ 0x1001938a2
//   cSC4PropManager::Release() @ 0x1001938cc
//   cSC4PropManager::QueryInterface(unsigned int, void**) @ 0x100193984
//   cSC4PropManager::Init() @ 0x1001939ea
//   cSC4PropManager::StaticInit(void*) @ 0x100193b80
//   cSC4PropManager::StaticShutdown(void*) @ 0x100193bc6
//   cSC4PropManager::LoadAllStandardPropExemplarData() @ 0x100193c06
//   cSC4PropManager::Shutdown() @ 0x100193c68
//   cSC4PropManager::AddPropExemplarData(cGZPersistResourceKey const&, cISCResExemplar*) @ 0x100193e38
//   cSC4PropManager::LoadPropProperties(cISCPropertyHolder*, unsigned int, bool) @ 0x100193f5a
//   cSC4PropManager::AddPropExemplarDataStatic(cGZPersistResourceKey const&, void*) @ 0x100194548
//   cSC4PropManager::PostPropExemplarDataUpdatedMessage() @ 0x1001945cc
//   cSC4PropManager::LoadPropProperties(cISC4PropOccupant*, unsigned int) @ 0x10019470c
//   cSC4PropManager::DoMessage(cIGZMessage2*) @ 0x1001948a2
//   cSC4PropManager::DoPostViewFilterChange(cSC4ViewFilterInformation*, cSC4ViewFilterInformation*) @ 0x100194bb2
//   cSC4PropManager::GetAllPropTypes(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&) @ 0x100194c06
//   cSC4PropManager::GetAllPropFamilyTypes(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&) @ 0x100194c64
//   cSC4PropManager::GetPropKeyFromType(unsigned int, cGZPersistResourceKey&) @ 0x100194cc2
//   cSC4PropManager::IsPropTypeValid(unsigned int) @ 0x100194d04
//   cSC4PropManager::IsPropFamilyTypeValid(unsigned int) @ 0x100194d2e
//   cSC4PropManager::GetPropTypesByFamily(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&, unsigned int) @ 0x100194d72
//   cSC4PropManager::IsPropInFamily(unsigned int, unsigned int) @ 0x100194e0a
//   cSC4PropManager::GetPropName(unsigned int) const @ 0x100194e66
//   cSC4PropManager::GetPropProfile(unsigned int) const @ 0x100194e8e
//   cSC4PropManager::GetPropProfilePtr(unsigned int) const @ 0x100194eb2
//   cSC4PropManager::CreateProp(unsigned int, cISC4PropOccupant*&) @ 0x100194ee0
//   cSC4PropManager::CreateGenericProp(cGZPersistResourceKey&, cISC4PropOccupant*&) @ 0x10019501a
//   cSC4PropManager::GetCityProps(ilist<cISC4Occupant*>&, SC4Rect<int> const&) @ 0x100195206
//   cSC4PropManager::GetCityProps(ilist<cISC4Occupant*>&, SC4Rect<float> const&) @ 0x100195244
//   cSC4PropManager::AddCityProp(unsigned int, float const*, int) @ 0x1001952d6
//   cSC4PropManager::SetPropUsage(cISC4Occupant*, unsigned int) @ 0x100195386
//   cSC4PropManager::AddCityProp(cISC4Occupant*) @ 0x1001953c8
//   cSC4PropManager::RemoveCityProps(SC4Rect<int> const&) @ 0x10019543c
//   cSC4PropManager::RemoveAndReleaseOccupants(ilist<cISC4Occupant*>&) @ 0x1001954a0
//   cSC4PropManager::RemoveCityProps(SC4Rect<float> const&) @ 0x1001954e2
//   cSC4PropManager::GetStreetProps(ilist<cISC4Occupant*>&, int, int) @ 0x100195546
//   cSC4PropManager::GetStreetProps(ilist<cISC4Occupant*>&, SC4Rect<int> const&) @ 0x10019565a
//   cSC4PropManager::GetLotProps(ilist<cISC4Occupant*>&, cISC4Lot*) @ 0x100195698
//   cSC4PropManager::GetLotProps(ilist<cISC4Occupant*>&, int, int) @ 0x1001957f6
//   cSC4PropManager::AddLotProp(unsigned int, float const*, int) @ 0x100195840
//   cSC4PropManager::AddPropWithUsage(unsigned int, float const*, int, unsigned int) @ 0x100195850
//   cSC4PropManager::AddLotProp(cISC4Lot*, cISC4Occupant*) @ 0x10019591e
//   cSC4PropManager::AddStreetProp(unsigned int, float const*, int) @ 0x100195992
//   cSC4PropManager::RemoveStreetPropsFromTile(int, int) @ 0x1001959a2
//   cSC4PropManager::RemovePropA(cISC4Occupant*) @ 0x100195a04
//   cSC4PropManager::RemoveOccupant(cISC4Occupant*) @ 0x100195a12
//   cSC4PropManager::RemoveLotProps(cISC4Lot*) @ 0x100195ae6
//   cSC4PropManager::RemoveLotProps(int, int) @ 0x100195b4a
//   cSC4PropManager::GetBuildingProps(ilist<cISC4Occupant*>&, cISC4Occupant*) @ 0x100195bb2
//   cSC4PropManager::GetBuildingProps(ilist<cISC4Occupant*>&, int, int) @ 0x100195c5c
//   cSC4PropManager::AddBuildingProp(cISC4Occupant*, cISC4Occupant*) @ 0x100195cbc
//   cSC4PropManager::RemoveBuildingProps(cISC4Occupant*) @ 0x100195e24
//   cSC4PropManager::RemoveBuildingProps(int, int) @ 0x100195f08
//   cSC4PropManager::RemoveBuildingProp(cISC4Occupant*, cISC4Occupant*) @ 0x100195f64
//
// cSC4LandfillZone (42 entry points)
//   cSC4LandfillZone::cSC4LandfillZone() @ 0x1000acbac
//   cSC4LandfillZone::cSC4LandfillZone(SC4CellRegion<int> const&) @ 0x1000acc32 / 0x1000ace6a
//   cSC4LandfillZone::~cSC4LandfillZone() @ 0x1000ace74 / 0x1000acec6 / 0x1000aced0
//   cSC4LandfillZone::Init() @ 0x1000acefe
//   cSC4LandfillZone::InitZoneEffects() @ 0x1000ad096
//   cSC4LandfillZone::Shutdown() @ 0x1000ad2de
//   cSC4LandfillZone::ShutdownZoneEffects() @ 0x1000ad442
//   cSC4LandfillZone::RemoveZot() @ 0x1000ad510
//   cSC4LandfillZone::RecalculateCapacity() @ 0x1000ad8e2
//   cSC4LandfillZone::GetNumValidHeaps() @ 0x1000ad986
//   cSC4LandfillZone::CalculateGarbageHeapCapacity(int) @ 0x1000ad9b6
//   cSC4LandfillZone::GetCellCount() @ 0x1000ada0a
//   cSC4LandfillZone::AddRef() @ 0x1000adaaa
//   cSC4LandfillZone::Release() @ 0x1000adab8
//   cSC4LandfillZone::QueryInterface(unsigned int, void**) @ 0x1000adaea
//   cSC4LandfillZone::DoMessage(cIGZMessage2*) @ 0x1000adb0c
//   cSC4LandfillZone::UpdateSurroundingTiles(cGarbageHeap&) @ 0x1000adb22
//   cSC4LandfillZone::GetMaintenanceCost() const @ 0x1000adfd4
//   cSC4LandfillZone::getLandfillTileCost(int, int) const @ 0x1000ae062
//   cSC4LandfillZone::UpdateZone(int&) @ 0x1000ae124
//   cSC4LandfillZone::IsZoneConnected() @ 0x1000ae238
//   cSC4LandfillZone::AddZot() @ 0x1000ae42a
//   cSC4LandfillZone::RedistributeGarbage() @ 0x1000ae682
//   cSC4LandfillZone::GetCapacity(int&, int&, int&) @ 0x1000ae6f8
//   cSC4LandfillZone::AddRegionToZone(SC4CellRegion<int> const&) @ 0x1000ae9fe
//   cSC4LandfillZone::InvalidateSurroundingHeaps(int, int) @ 0x1000aee92
//   cSC4LandfillZone::RemoveLandfillTile(int, int) @ 0x1000aef82
//   cSC4LandfillZone::CreateLandfillOccupant(int, int, unsigned int, int) @ 0x1000af00c
//   cSC4LandfillZone::SetEmptyCellTexture(int, int) @ 0x1000af28e
//   cSC4LandfillZone::RemoveIncompatibleOccupants(int, int) @ 0x1000af418
//   cSC4LandfillZone::RemoveBaseTexture(int, int) @ 0x1000af508
//   cSC4LandfillZone::GetFirstRoadTileNeighbor(SC4Point<int>&) @ 0x1000af650
//   cSC4LandfillZone::DezoneCell(int, int, bool, bool) @ 0x1000af886
//   cSC4LandfillZone::HandleTerrainChange(SC4Rect<int> const&) @ 0x1000afa7e
//   cSC4LandfillZone::HandleCellTerrainChange(int, int) @ 0x1000afb1c
//   cSC4LandfillZone::IsCompletelyDezoned() @ 0x1000afd60
//   cSC4LandfillZone::LevelRect(SC4Rect<int> const&, float, bool) @ 0x1000afdf8
//   cSC4LandfillZone::PopulateZone() @ 0x1000aff38
//   cSC4LandfillZone::GetAvailableCapacity() @ 0x1000b011a
//
// cSC4City (120 entry points)
//   cSC4City::cSC4City() @ 0x100047728 / 0x10004773e
//   cSC4City::~cSC4City() @ 0x100047afc / 0x100047cea / 0x100047d02
//   cSC4City::QueryInterface(unsigned int, void**) @ 0x100047d3e
//   cSC4City::AddRef() @ 0x100047dbe
//   cSC4City::Release() @ 0x100047df6
//   cSC4City::GetCitySizeType() const @ 0x100047ee8
//   cSC4City::SetSize(float, float) @ 0x100047f0e
//   cSC4City::PositionToCell(float, float, int&, int&) const @ 0x100047f54
//   cSC4City::CellCornerToPosition(int, int, float&, float&) const @ 0x100047fd0
//   cSC4City::CellCenterToPosition(int, int, float&, float&) const @ 0x100047ffa
//   cSC4City::LocationIsInBounds(float, float) const @ 0x100048034
//   cSC4City::CellIsInBounds(int, int) const @ 0x100048064
//   cSC4City::CellCornerIsInBounds(int, int) const @ 0x100048086
//   cSC4City::GetCitySerialNumber() const @ 0x1000480a8
//   cSC4City::SetCitySerialNumber(unsigned int) @ 0x1000480b2
//   cSC4City::GetNewOccupantSerialNumber() @ 0x1000480bc
//   cSC4City::GetOriginalLanguageAndCountry(int&, int&) const @ 0x1000480ca
//   cSC4City::GetLastLanguageAndCountry(int&, int&) @ 0x1000480dc
//   cSC4City::GetCitySaveFilePath(cIGZString&) const @ 0x1000480ee
//   cSC4City::SetCitySaveFilePath(cIGZString const&) @ 0x100048106
//   cSC4City::GetCityName(cIGZString&) const @ 0x100048114
//   cSC4City::SetCityName(cIGZString const&) @ 0x100048154
//   cSC4City::GetCityNameChanged() const @ 0x100048250
//   cSC4City::SetCityNameChanged(bool) @ 0x10004825a
//   cSC4City::GetMayorName(cIGZString&) const @ 0x100048264
//   cSC4City::SetMayorName(cIGZString const&) @ 0x1000482a4
//   cSC4City::GetCityDescription(cIGZString&) const @ 0x1000483b2
//   cSC4City::SetCityDescription(cIGZString const&) @ 0x1000483f2
//   cSC4City::GetBirthDate() const @ 0x10004847e
//   cSC4City::SetBirthDate(unsigned int) @ 0x100048488
//   cSC4City::GetWorldPosition(float&, float&) @ 0x100048492
//   cSC4City::SetWorldPosition(float, float) @ 0x1000484a8
//   cSC4City::GetWorldBaseElevation() @ 0x1000484ee
//   cSC4City::SetWorldBaseElevation(float) @ 0x1000484fc
//   cSC4City::GetWorldHemisphere() @ 0x10004850a
//   cSC4City::GetEstablished() const @ 0x10004852a
//   cSC4City::SetEstablished(bool) @ 0x100048538
//   cSC4City::GetDemolitionUtility() const @ 0x100048620
//   cSC4City::GetHistoryWarehouse() const @ 0x10004862e
//   cSC4City::GetSurfaceWater() const @ 0x10004863c
//   cSC4City::GetTerrain() const @ 0x10004864a
//   cSC4City::Get24HourClock() const @ 0x100048658
//   cSC4City::GetLotManager() const @ 0x100048666
//   cSC4City::GetOccupantManager() const @ 0x100048674
//   cSC4City::GetPropManager() const @ 0x100048682
//   cSC4City::GetEffectsManager() const @ 0x100048690
//   cSC4City::GetNetworkManager() const @ 0x10004869e
//   cSC4City::GetZoneManager() const @ 0x1000486ac
//   cSC4City::GetVehicleManager() const @ 0x1000486ba
//   cSC4City::GetAircraftManager() const @ 0x1000486c8
//   cSC4City::GetPedestrianManager() const @ 0x1000486d6
//   cSC4City::GetTrafficNetwork() const @ 0x1000486e4
//   cSC4City::GetWatercraftManager() const @ 0x1000486f2
//   cSC4City::GetAutomataControllerManager() const @ 0x100048700
//   cSC4City::GetAutomataScriptSystem() const @ 0x10004870e
//   cSC4City::GetCitySituationManager() const @ 0x10004871c
//   cSC4City::GetLotConfigurationManager() const @ 0x10004872a
//   cSC4City::GetNetworkLotManager() const @ 0x100048738
//   cSC4City::GetPropDeveloper() const @ 0x100048746
//   cSC4City::GetDispatchManager() const @ 0x100048754
//   cSC4City::GetSimulator() const @ 0x100048762
//   cSC4City::GetAuraSimulator() const @ 0x100048770
//   cSC4City::GetBudgetSimulator() const @ 0x10004877e
//   cSC4City::GetBuildingDevelopmentSimulator() const @ 0x10004878c
//   cSC4City::GetCommercialSimulator() const @ 0x10004879a
//   cSC4City::GetCrimeSimulator() const @ 0x1000487a8
//   cSC4City::GetDemandSimulator() const @ 0x1000487b6
//   cSC4City::GetFireProtectionSimulator() const @ 0x1000487c4
//   cSC4City::GetFlammabilitySimulator() const @ 0x1000487d2
//   cSC4City::GetFloraSimulator() const @ 0x1000487e0
//   cSC4City::GetIndustrialSimulator() const @ 0x1000487ee
//   cSC4City::GetLandValueSimulator() const @ 0x1000487fc
//   cSC4City::GetNeighborsSimulator() const @ 0x10004880a
//   cSC4City::GetOrdinanceSimulator() const @ 0x100048818
//   cSC4City::GetPlumbingSimulator() const @ 0x100048826
//   cSC4City::GetPoliceSimulator() const @ 0x100048834
//   cSC4City::GetPollutionSimulator() const @ 0x100048842
//   cSC4City::GetPowerSimulator() const @ 0x100048850
//   cSC4City::GetResidentialSimulator() const @ 0x10004885e
//   cSC4City::GetTrafficSimulator() const @ 0x10004886c
//   cSC4City::GetWeatherSimulator() const @ 0x10004887a
//   cSC4City::GetMySimAgentSimulator() const @ 0x100048888
//   cSC4City::GetCivicBuildingSimulator() const @ 0x100048896
//   cSC4City::GetDisasterLayer() const @ 0x1000488a4
//   cSC4City::GetZoneDeveloper() const @ 0x1000488b2
//   cSC4City::GetLandfillDeveloper() const @ 0x1000488c0
//   cSC4City::GetAirportDeveloper() const @ 0x1000488ce
//   cSC4City::GetSeaportDeveloper() const @ 0x1000488dc
//   cSC4City::GetLotDeveloper() const @ 0x1000488ea
//   cSC4City::GetTractDeveloper() const @ 0x1000488f8
//   cSC4City::GetParkManager() const @ 0x100048906
//   cSC4City::GetAdvisorSystem() const @ 0x100048914
//   cSC4City::GetTutorialSystem() const @ 0x100048922
//   cSC4City::IsInCityTimeSimulationMode() @ 0x100048930
//   cSC4City::ToggleSimulationMode() @ 0x100048954
//   cSC4City::WriteRegionViewThumbnail(cIGZPersistDBSegment*, cIGZString const&, bool) @ 0x1000489a0
//   cSC4City::ReadRetainedRecords(cIGZPersistDBSegment*) @ 0x100048c16
//   cSC4City::WriteRetainedRecords(cIGZPersistDBSegment*) const @ 0x100048e80
//   cSC4City::EnableSave() @ 0x100048ed0
//   cSC4City::DisableSave() @ 0x100048ee8
//   cSC4City::IsSaveDisabled() @ 0x100048ef4
//   cSC4City::UIIncreaseLockCount() @ 0x100048f04
//   cSC4City::UIDecreaseLockCount() @ 0x100048f10
//   cSC4City::UIGetLockCount() @ 0x100048f28
//   cSC4City::SaveObliterated(cIGZPersistDBSegment*) const @ 0x100048f34
//   cSC4City::GetDifficultyLevel() const @ 0x100048f78
//   cSC4City::SetDifficultyLevel(cISC4City::DifficultyLevel) @ 0x100048f82
//   cSC4City::SizeX() const @ 0x100048f8c
//   cSC4City::SizeZ() const @ 0x100048f9a
//   cSC4City::CellWidthX() const @ 0x100048fa8
//   cSC4City::CellWidthZ() const @ 0x100048fb6
//   cSC4City::CellCountX() const @ 0x100048fc4
//   cSC4City::CellCountZ() const @ 0x100048fd0
//   cSC4City::CreateCityComponents() @ 0x1006bc260
//   cSC4City::Init() @ 0x1006bcaa2
//   cSC4City::Shutdown() @ 0x1006bd18c
//   cSC4City::DoMessage(cIGZMessage2*) @ 0x1006bdcd2
//
// cSC4DispatchManager (34 entry points)
//   cSC4DispatchManager::~cSC4DispatchManager() @ 0x1002157bc / 0x100215848 / 0x100215852
//   cSC4DispatchManager::AddRef() @ 0x100215880
//   cSC4DispatchManager::Release() @ 0x1002158aa
//   cSC4DispatchManager::QueryInterface(unsigned int, void**) @ 0x100215962
//   cSC4DispatchManager::Init() @ 0x1002159c8
//   cSC4DispatchManager::ClearAvailablePylonList() @ 0x100215b80
//   cSC4DispatchManager::Shutdown() @ 0x100215bca
//   cSC4DispatchManager::DoMessage(cIGZMessage2*) @ 0x100216066
//   cSC4DispatchManager::HandleAutomataEventMessage(cIGZMessage2Standard*) @ 0x10021628e
//   cSC4DispatchManager::UpdateDispatchState(cISC4DispatchManager::DispatchInfo&, cISC4DispatchManager::tDispatchState) @ 0x100216352
//   cSC4DispatchManager::RegisterDispatchSource(cISC4DispatchSource*) @ 0x100216454
//   cSC4DispatchManager::UnregisterDispatchSource(cISC4DispatchSource*) @ 0x1002164f0
//   cSC4DispatchManager::GetDispatchSource(unsigned int) const @ 0x1002165ba
//   cSC4DispatchManager::RevokeDispatchByPylon(cISC4PylonOccupant*) @ 0x10021660c
//   cSC4DispatchManager::ReturnPylonToStation(cISC4DispatchSource*, cISC4PylonOccupant*) @ 0x10021667c
//   cSC4DispatchManager::RevokeDispatchByID(unsigned int) @ 0x10021695a
//   cSC4DispatchManager::EndDispatch(cISC4DispatchManager::DispatchInfo&) @ 0x1002169c2
//   cSC4DispatchManager::RevokeAllDispatches(cISC4DispatchManager::tDispatchType) @ 0x100216b58
//   cSC4DispatchManager::GetDispatchInfo(unsigned int, cISC4DispatchManager::DispatchInfo&) @ 0x100216b84
//   cSC4DispatchManager::GetDispatchCount(cISC4DispatchManager::tDispatchType) @ 0x100216be0
//   cSC4DispatchManager::GetDispatchIDArray(cISC4DispatchManager::tDispatchType, unsigned int*, unsigned int&) @ 0x100216c06
//   cSC4DispatchManager::SetActivePylon(cISC4PylonOccupant*) @ 0x100216c38
//   cSC4DispatchManager::GetAvailablePylonCount(cISC4DispatchManager::tDispatchType) const @ 0x100216c96
//   cSC4DispatchManager::GetAutomatonFromPylon(cISC4PylonOccupant*) const @ 0x100216da8
//   cSC4DispatchManager::BeginDispatchTool(cISC4DispatchManager::tDispatchType) @ 0x100216dd0
//   cSC4DispatchManager::EndDispatchTool(cISC4DispatchManager::tDispatchType) @ 0x1002171dc
//   cSC4DispatchManager::CanDispatch(cISC4DispatchManager::tDispatchType, float, float) @ 0x10021720c
//   cSC4DispatchManager::Dispatch(cISC4DispatchManager::tDispatchType, float, float) @ 0x10021724e
//   cSC4DispatchManager::InitDispatch(cISC4DispatchManager::DispatchInfo&) @ 0x100217430
//   cSC4DispatchManager::BeginDispatch(cISC4DispatchManager::DispatchInfo&) @ 0x100217684
//   cSC4DispatchManager::GetActivePylon() const @ 0x100217a90
//   cSC4DispatchManager::GetDispatchVehicleView() const @ 0x100217a9a
//
// cSC4Occupant (41 entry points)
//   cSC4Occupant::GetType() const @ 0x1000b3580
//   cSC4Occupant::cSC4Occupant() @ 0x1000bc5bc
//   cSC4Occupant::~cSC4Occupant() @ 0x1000bc65c / 0x1000bc6d6 / 0x1000bc6ee
//   cSC4Occupant::Init() @ 0x1000bc72a
//   cSC4Occupant::Shutdown() @ 0x1000bc778
//   cSC4Occupant::AddRef() @ 0x1000bc94e
//   cSC4Occupant::Release() @ 0x1000bc996
//   cSC4Occupant::FinalRelease() @ 0x1000bc9ca
//   cSC4Occupant::QueryInterface(unsigned int, void**) @ 0x1000bca6a
//   cSC4Occupant::GetPosition(float*) @ 0x1000bcace
//   cSC4Occupant::SetPosition(float const*) @ 0x1000bcae2
//   cSC4Occupant::GetBoundingBox(float*, float*) @ 0x1000bcae8
//   cSC4Occupant::GetBoundingCityCells(SC4Rect<int>&) @ 0x1000bcafc
//   cSC4Occupant::SetRemovalFlags(int) @ 0x1000bcbb6
//   cSC4Occupant::UnsetRemovalFlags(int) @ 0x1000bcbf8
//   cSC4Occupant::CanRemove(int) @ 0x1000bcc3e
//   cSC4Occupant::IsOccupantGroup(unsigned int) const @ 0x1000bcc68
//   cSC4Occupant::GetOccupantGroups(std::__1::set<unsigned int, std::__1::less<unsigned int>, std::__1::allocator<unsigned int>>&) const @ 0x1000bcce8
//   cSC4Occupant::AddOccupantGroup(unsigned int) @ 0x1000bce7e
//   cSC4Occupant::IsOccupantGroup(cGZPersistResourceKey const&, unsigned int) @ 0x1000bcf5a
//   cSC4Occupant::GetOccupantGroups(cGZPersistResourceKey const&, std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&) @ 0x1000bd03a
//   cSC4Occupant::PostOccupantMessage(unsigned int, unsigned int) @ 0x1000bd12a
//   cSC4Occupant::SetHighlight(int, bool) @ 0x1000bd256
//   cSC4Occupant::SetVisibility(bool, bool) @ 0x1000bd38a
//   cSC4Occupant::GetPlaceableObject() const @ 0x1000bd3be
//   cSC4Occupant::GetOrCreatePlaceableObject() @ 0x1000bd3c8
//   cSC4Occupant::SetPlaceableObject(cISC43DPlaceableObject*) @ 0x1000bd44c
//   cSC4Occupant::SetFlag(unsigned int) @ 0x1000bd49a
//   cSC4Occupant::SetAllFlags(unsigned int) @ 0x1000bd4b8
//   cSC4Occupant::ClearFlag(unsigned int) @ 0x1000bd4c2
//   cSC4Occupant::IsFlagSet(unsigned int) @ 0x1000bd4e2
//   cSC4Occupant::GetFlags() @ 0x1000bd4f4
//   cSC4Occupant::GetOccupantManagerBBox(unsigned char*) const @ 0x1000bd4fe
//   cSC4Occupant::SetOccupantManagerBBox(unsigned char*) @ 0x1000bd526
//   cSC4Occupant::GetLotTag(unsigned int&) const @ 0x1000bd53e
//   cSC4Occupant::SetLotTag(unsigned int) @ 0x1000bd546
//   cSC4Occupant::IsInitialized() const @ 0x100163370
//   cSC4Occupant::GetHighlight() const @ 0x100163380
//   cSC4Occupant::AsPropertyHolder() @ 0x10019bb80
//
// cSC4NeighborDeal (45 entry points)
//   cSC4NeighborDeal::cSC4NeighborDeal() @ 0x100072e24 / 0x100075156
//   cSC4NeighborDeal::~cSC4NeighborDeal() @ 0x10007522e / 0x10007529e
//   cSC4NeighborDeal::SetIncAmount2(int) @ 0x10007530a
//   cSC4NeighborDeal::SetIncAmount(int) @ 0x100075314
//   cSC4NeighborDeal::SetLastAmount(int) @ 0x10007531e
//   cSC4NeighborDeal::SetLastIncAmount(int) @ 0x100075328
//   cSC4NeighborDeal::SetAmount(int) @ 0x100075332
//   cSC4NeighborDeal::QueryInterface(unsigned int, void**) @ 0x1000754ac
//   cSC4NeighborDeal::AddRef() @ 0x1000754fc
//   cSC4NeighborDeal::Release() @ 0x100075526
//   cSC4NeighborDeal::Copy(cISC4NeighborDeal const&) @ 0x10007558e
//   cSC4NeighborDeal::GetCityOnOtherSideOfConnection() const @ 0x1000759d8
//   cSC4NeighborDeal::UpdateCost() @ 0x100075a1c
//   cSC4NeighborDeal::SetFlags(int, bool) @ 0x100075ad8
//   cSC4NeighborDeal::UpdateState() @ 0x100075b46
//   cSC4NeighborDeal::Cancel() @ 0x100075be4
//   cSC4NeighborDeal::Activate() @ 0x100075c28
//   cSC4NeighborDeal::Clone(cISC4NeighborDeal**, bool) const @ 0x100075ca2
//   cSC4NeighborDeal::SetRegionCellOnCitySideOfConnection(int, int) @ 0x100075f98
//   cSC4NeighborDeal::GetRegionCellOnCitySideOfConnection(int&, int&) const @ 0x100075fa4
//   cSC4NeighborDeal::SetRegionCellOnOtherSideOfConnection(int, int) @ 0x100075fb4
//   cSC4NeighborDeal::GetRegionCellOnOtherSideOfConnection(int&, int&) const @ 0x100075fc0
//   cSC4NeighborDeal::GetType() const @ 0x100075fd0
//   cSC4NeighborDeal::SetType(int) @ 0x100075fda
//   cSC4NeighborDeal::GetAmount() const @ 0x100075fe4
//   cSC4NeighborDeal::GetLastAmount() const @ 0x100075fee
//   cSC4NeighborDeal::GetLastIncAmount() const @ 0x100075ff8
//   cSC4NeighborDeal::GetIncAmount() const @ 0x100076002
//   cSC4NeighborDeal::GetIncAmount2() const @ 0x10007600c
//   cSC4NeighborDeal::GetAmountUsed() const @ 0x100076016
//   cSC4NeighborDeal::SetAmountUsed(int) @ 0x100076020
//   cSC4NeighborDeal::GetCost() const @ 0x10007602a
//   cSC4NeighborDeal::GetStartDate(cIGZDate&) const @ 0x100076034
//   cSC4NeighborDeal::SetStartDate(cIGZDate const&) @ 0x10007604a
//   cSC4NeighborDeal::GetExpirationDate(cIGZDate&) const @ 0x100076074
//   cSC4NeighborDeal::SetExpirationDate(cIGZDate const&) @ 0x10007608e
//   cSC4NeighborDeal::GetCurrentDate(cIGZDate&) const @ 0x1000760b8
//   cSC4NeighborDeal::SetCurrentDate(cIGZDate const&) @ 0x1000760d2
//   cSC4NeighborDeal::GetSuspensionDate(cIGZDate&) const @ 0x1000760fe
//   cSC4NeighborDeal::SetSuspensionDate(cIGZDate const&) @ 0x100076118
//   cSC4NeighborDeal::GetFlags() const @ 0x100076144
//   cSC4NeighborDeal::CheckFlagsAnd(int) const @ 0x100076150
//   cSC4NeighborDeal::CheckFlagsOr(int) const @ 0x100076164
//
// cSC4HistoryWarehouse (21 entry points)
//   cSC4HistoryWarehouse::Init() @ 0x1000ef516
//   cSC4HistoryWarehouse::Shutdown() @ 0x1000ef62e
//   cSC4HistoryWarehouse::FinalRelease() @ 0x1000ef6a0
//   cSC4HistoryWarehouse::QueryInterface(unsigned int, void**) @ 0x1000ef6fc
//   cSC4HistoryWarehouse::AddRef() @ 0x1000ef74c
//   cSC4HistoryWarehouse::Release() @ 0x1000ef776
//   cSC4HistoryWarehouse::DoMessage(cIGZMessage2*) @ 0x1000ef85e
//   cSC4HistoryWarehouse::Store(unsigned int, int) @ 0x1000ef91c
//   cSC4HistoryWarehouse::SetSize(unsigned int, int) @ 0x1000efa1e
//   cSC4HistoryWarehouse::DoDailySimulation(unsigned int) @ 0x1000efa82
//   cSC4HistoryWarehouse::VariableCoalesce(unsigned int) @ 0x1000f0040
//   cSC4HistoryWarehouse::GetKeyVector(unsigned int) @ 0x1000f0270
//   cSC4HistoryWarehouse::GetCount(unsigned int, cIGZDate const&, cIGZDate const&, float&) @ 0x1000f03f4
//   cSC4HistoryWarehouse::GetTrend(unsigned int, cIGZDate const&, cIGZDate const&, float&, float&) @ 0x1000f0466
//   cSC4HistoryWarehouse::GetMinMaxAverage(unsigned int, cIGZDate const&, cIGZDate const&, float&, float&, float&) @ 0x1000f0546
//   cSC4HistoryWarehouse::GetPoint(unsigned int, cIGZDate const&, float&) @ 0x1000f07e4
//   cSC4HistoryWarehouse::Coalesce(unsigned int, cIGZDate const&, cIGZDate const&, int) @ 0x1000f08ac
//   cSC4HistoryWarehouse::Dispose(unsigned int, cIGZDate const&, cIGZDate const&) @ 0x1000f0964
//   cSC4HistoryWarehouse::Dispose(unsigned int) @ 0x1000f0a16
//   cSC4HistoryWarehouse::RecordHistoryData(unsigned int, int) @ 0x1000f0a60
//   cSC4HistoryWarehouse::SetHistoryDataQueueLength(unsigned int, unsigned int) @ 0x1000f0af8
//
// cSC4Demolition (20 entry points)
//   cSC4Demolition::~cSC4Demolition() @ 0x1000eaf70 / 0x1000eafa2
//   cSC4Demolition::AddRef() @ 0x1000eafac
//   cSC4Demolition::Release() @ 0x1000eafd6
//   cSC4Demolition::QueryInterface(unsigned int, void**) @ 0x1000eb084
//   cSC4Demolition::Init() @ 0x1000eb0ea
//   cSC4Demolition::ReadProperties() @ 0x1000eb262
//   cSC4Demolition::Shutdown() @ 0x1000eb3d2
//   cSC4Demolition::DoMessage(cIGZMessage2*) @ 0x1000eb58c
//   cSC4Demolition::SetDefaultOccupantFilter(cISC4OccupantFilter*) @ 0x1000eb86e
//   cSC4Demolition::DemolishRegion(bool, SC4CellRegion<int> const&, cISC4Demolition::PrivilegeType, unsigned int, bool, cISC4OccupantFilter*, long long*, std::__1::set<cRZAutoRefCount<cISC4Occupant>, std::__1::less<cISC4Occupant*>, std::__1::allocator<cRZAutoRefCount<cISC4Occupant>>>*, cISC4Occupant*, int, int) @ 0x1000eb874
//   cSC4Demolition::GetDemolishableLotOccupants(cISC4Lot*, ilist<cISC4Occupant*>*) @ 0x1000ece32
//   cSC4Demolition::GetDemolishableRectOccupants(SC4Rect<int> const&, ilist<cISC4Occupant*>*) @ 0x1000ece9a
//   cSC4Demolition::UpdateNetworks(std::__1::set<cRZAutoRefCount<cISC4Occupant>, std::__1::less<cISC4Occupant*>, std::__1::allocator<cRZAutoRefCount<cISC4Occupant>>>&) @ 0x1000ecfba
//   cSC4Demolition::DemolishLots(bool, std::__1::list<cRZAutoRefCount<cISC4Lot>, std::__1::allocator<cRZAutoRefCount<cISC4Lot>>> const&, cISC4Demolition::PrivilegeType, unsigned int, bool, cISC4OccupantFilter*, long long*, std::__1::set<cRZAutoRefCount<cISC4Occupant>, std::__1::less<cISC4Occupant*>, std::__1::allocator<cRZAutoRefCount<cISC4Occupant>>>*) @ 0x1000ed13e
//   cSC4Demolition::DemolishOccupant(bool, cISC4Occupant*, cISC4Demolition::PrivilegeType, unsigned int, long long*, bool, ilist<cISC4Occupant*>*) @ 0x1000ed496
//   cSC4Demolition::PostMessageA(unsigned int, cISC4Occupant*, bool) @ 0x1000eda3e
//   cSC4Demolition::ModifyTerrainHeight(bool, bool, int, int, int, int, float, cISC4Lot*, unsigned int, long long*) @ 0x1000edb00
//   cSC4Demolition::DoDemolishOccupantEffect(cISC4Occupant*, unsigned int, cISC4Lot*, bool, int, int) @ 0x1000ee0e2
//   cSC4Demolition::DoDemolishGroupEffect(unsigned int, int, int, SC4Rect<int> const&) @ 0x1000ee4e6
//
// cSC4Nation (25 entry points)
//   cSC4Nation::cSC4Nation() @ 0x1002887ac
//   cSC4Nation::~cSC4Nation() @ 0x100288810 / 0x100288876 / 0x100288880
//   cSC4Nation::QueryInterface(unsigned int, void**) @ 0x1002888ae
//   cSC4Nation::AddRef() @ 0x1002888fe
//   cSC4Nation::Release() @ 0x100288928
//   cSC4Nation::StaticInit() @ 0x1002889e0
//   cSC4Nation::Init() @ 0x100288a9a
//   cSC4Nation::LoadRegions() @ 0x100289362
//   cSC4Nation::CopyNewUserRegions() @ 0x100289582
//   cSC4Nation::Shutdown() @ 0x100289748
//   cSC4Nation::DoMessage(cIGZMessage2*) @ 0x100289a2c
//   cSC4Nation::SetActiveRegion(unsigned int) @ 0x100289a88
//   cSC4Nation::GetIndexOfCurrentRegionSettingInPreferences(int&) @ 0x100289ac6
//   cSC4Nation::DeleteRegion(unsigned int) @ 0x100289d54
//   cSC4Nation::AddNewRegion(cIGZString const&) @ 0x100289f04
//   cSC4Nation::ResetTutorialCities() @ 0x100289fa6
//   cSC4Nation::GetEconomyModifier(cISC4DemandSimulator::DemandType) @ 0x10028b0d0
//   cSC4Nation::GetRegionDirectory() @ 0x10028b4d0
//   cSC4Nation::GetRegionCount() const @ 0x10028b4de
//   cSC4Nation::GetActiveRegionIndex() const @ 0x10028b4f0
//   cSC4Nation::GetActiveRegion() const @ 0x10028b4fa
//   cSC4Nation::GetRegion(unsigned int) @ 0x10028b50c
//   cSC4Nation::GetRegionDownloadDirectory() @ 0x10028b51c
//
// cSC4Region (37 entry points)
//   cSC4Region::cSC4Region() @ 0x1002a246c
//   cSC4Region::~cSC4Region() @ 0x1002a251a / 0x1002a25cc / 0x1002a25d6
//   cSC4Region::QueryInterface(unsigned int, void**) @ 0x1002a2604
//   cSC4Region::AddRef() @ 0x1002a2626
//   cSC4Region::Release() @ 0x1002a2634
//   cSC4Region::Init() @ 0x1002a2678
//   cSC4Region::LoadCityLocations() @ 0x1002a26ce
//   cSC4Region::LoadCities() @ 0x1002a2982
//   cSC4Region::MoveOverflowCitiesToDownloadDirectory() @ 0x1002a2c1c
//   cSC4Region::Shutdown() @ 0x1002a2e06
//   cSC4Region::SetName(cIGZString const&) @ 0x1002a3672
//   cSC4Region::SetDirectoryName(cIGZString const&) @ 0x1002a3680
//   cSC4Region::LoadConfig() @ 0x1002a368e
//   cSC4Region::LoadCitiesFromDirectory(ilist<cRZAutoRefCount<cISC4RegionalCity>>&, cRZString const&) @ 0x1002a3fd4
//   cSC4Region::IsCitySpaceOccupied(cISC4RegionalCity*) @ 0x1002a428e
//   cSC4Region::Delete() @ 0x1002a4336
//   cSC4Region::GetCity(int, int) @ 0x1002a461a
//   cSC4Region::GetAllCities(ilist<cRZAutoRefCount<cISC4RegionalCity>>&) const @ 0x1002a464e
//   cSC4Region::InsertCity(cISC4RegionalCity*) @ 0x1002a4812
//   cSC4Region::RemoveCity(cRZAutoRefCount<cISC4RegionalCity>*&) @ 0x1002a4948
//   cSC4Region::ReloadCity(cRZAutoRefCount<cISC4RegionalCity>*&) @ 0x1002a4a5c
//   cSC4Region::DeleteCity(cRZAutoRefCount<cISC4RegionalCity>*&) @ 0x1002a4b6a
//   cSC4Region::MoveCity(cISC4Region*, cISC4RegionalCity*, int, int) @ 0x1002a4c28
//   cSC4Region::CreateRegion(cIGZString const&) @ 0x1002a4ec8
//   cSC4Region::CreateStarterRegion() @ 0x1002a5390
//   cSC4Region::ResetTutorialCity(int) @ 0x1002a53e4
//   cSC4Region::GetBoundingRect(SC4Rect<int>&) @ 0x1002a557a
//   cSC4Region::GetCityLocations(std::__1::vector<cISC4Region::cLocation, std::__1::allocator<cISC4Region::cLocation>>&) @ 0x1002a5604
//   cSC4Region::ParseCityLocations(cIGZBuffer*) @ 0x1002a5630
//   cSC4Region::GetName() @ 0x1002a588e
//   cSC4Region::GetDirectoryName() @ 0x1002a5898
//   cSC4Region::GetBaseTerrainType() @ 0x1002a58a2
//   cSC4Region::SetBaseTerrainType(cISC4Region::eBaseTerrainType) @ 0x1002a58ac
//   cSC4Region::GetBaseTerrainHeight() @ 0x1002a58b6
//   cSC4Region::GetWaterPrefs(unsigned char&, unsigned char&) @ 0x1002a58ca
//
// cSC4App (116 entry points)
//   cSC4App::cSC4App(char const*) @ 0x1000039e2
//   cSC4App::~cSC4App() @ 0x10000447c / 0x100004ab0 / 0x100004ad6
//   cSC4App::PreFrameWorkInit() @ 0x100004b20
//   cSC4App::AddCOMDirectors() @ 0x1000062aa
//   cSC4App::AddSystemServices() @ 0x100006372
//   cSC4App::AddCodePlugins(cRZString const&) @ 0x1000073de
//   cSC4App::FindAndRegisterAllAvailableLanguages() @ 0x100007d56
//   cSC4App::DoPreferencesChanged(bool) @ 0x1000080b2
//   cSC4App::Init() @ 0x10000869e
//   cSC4App::PostSC4AppMessage(unsigned int, bool) @ 0x10000985c
//   cSC4App::PostFrameWorkInit() @ 0x10000997e
//   cSC4App::FindTestScript(cRZString&) @ 0x10000a2be
//   cSC4App::PreFrameWorkShutdown() @ 0x10000a5f4
//   cSC4App::Shutdown() @ 0x10000a74c
//   cSC4App::SetFontAliasStatic(cRZString const&, cRZString const&, void*) @ 0x10000ab6c
//   cSC4App::SetFontStyleStatic(cRZString const&, cRZString const&, void*) @ 0x10000aba8
//   cSC4App::SetFontStyle(cRZString const&, cRZString const&) @ 0x10000abbc
//   cSC4App::FixStartupResolutionValuesIfNeeded(unsigned int&, unsigned int&) @ 0x10000acba
//   cSC4App::IsStartupResolutionOK(unsigned int, unsigned int) @ 0x10000ad7a
//   cSC4App::QueryInterface(unsigned int, void**) @ 0x10000afb8
//   cSC4App::AddRef() @ 0x10000b06c
//   cSC4App::Release() @ 0x10000b0ca
//   cSC4App::SetupWinCtrlMgr() @ 0x10000b128
//   cSC4App::ShutdownWinCtrlMgr() @ 0x10000c7ca
//   cSC4App::SetupFontSet() @ 0x10000c7d0
//   cSC4App::RunMessageServerPump(unsigned int, unsigned int, unsigned int) @ 0x10000efc0
//   cSC4App::RunMessageServer2Pump(unsigned int, unsigned int, unsigned int) @ 0x10000f07e
//   cSC4App::LaunchExceptionReportUploader() @ 0x10000f146
//   cSC4App::EnableFullGamePauseOnAppFocusLoss(bool) @ 0x10000f2ba
//   cSC4App::GainLoseFocus(bool) @ 0x10000f2f2
//   cSC4App::FullGamePause(bool) @ 0x10000f322
//   cSC4App::ReadPreferences() @ 0x10000f35c
//   cSC4App::SavePreferences() @ 0x10000f670
//   cSC4App::FindAndPossiblyFixIniFiles() @ 0x100010c06
//   cSC4App::AddDynamicLibraryByName(cIGZString const&, cRZString*, bool) @ 0x10001136a
//   cSC4App::AddDynamicLibraryByPath(cIGZString const&, bool) @ 0x100011656
//   cSC4App::DoMessage(cGZMessage&) @ 0x100011a14
//   cSC4App::DoMessage(cIGZMessage2*) @ 0x100011df8
//   cSC4App::PostCityMessage(cISC4City*, unsigned int, unsigned int, unsigned int, bool) @ 0x100011f14
//   cSC4App::OnIdle(unsigned int) @ 0x100012020
//   cSC4App::OnTick(unsigned int) @ 0x1000121ea
//   cSC4App::WindowSetup() @ 0x1000121fa
//   cSC4App::WindowShutdown() @ 0x10001226e
//   cSC4App::GetAppName(cIGZString&) const @ 0x1000122b2
//   cSC4App::GetCity() const @ 0x10001231a
//   cSC4App::GetPreferences() const @ 0x100012336
//   cSC4App::GetNewCitySpecification() @ 0x100012352
//   cSC4App::GetAppDirectory(cIGZString&) @ 0x10001236e
//   cSC4App::GetCDAppDirectory(cIGZString&) @ 0x1000123d6
//   cSC4App::GetDataDirectory(cIGZString&, int) @ 0x10001243e
//   cSC4App::GetCDDataDirectory(cIGZString&, int) @ 0x10001253e
//   cSC4App::GetPluginDirectory(cIGZString&) @ 0x10001263e
//   cSC4App::GetCDPluginDirectory(cIGZString&) @ 0x1000126a6
//   cSC4App::GetSkuSpecificDirectory(cIGZString&) @ 0x10001270e
//   cSC4App::GetUserDataDirectory(cIGZString&) @ 0x100012776
//   cSC4App::GetUserPluginDirectory(cIGZString&) @ 0x1000127de
//   cSC4App::GetRegionsDirectory(cIGZString&) @ 0x10001288e
//   cSC4App::GetMySimDirectory(cIGZString&) @ 0x10001293e
//   cSC4App::GetAlbumDirectory(cIGZString&) @ 0x1000129ee
//   cSC4App::GetHTTPCacheDirectory(cIGZString&) @ 0x100012a9e
//   cSC4App::GetTempDirectory(cIGZString&) @ 0x100012b4e
//   cSC4App::GetExceptionReportsDirectory(cIGZString&) @ 0x100012bfe
//   cSC4App::GetTestScriptDirectory(cIGZString&) @ 0x100012cae
//   cSC4App::GetAppIniFileName(cIGZString&) const @ 0x100012d8a
//   cSC4App::GetAppIniFilePath(cIGZString&) const @ 0x100012e1e
//   cSC4App::GetAppPreferencesFileName(cIGZString&) const @ 0x100012fa4
//   cSC4App::GetAppPreferencesFilePath(cIGZString&) const @ 0x100013038
//   cSC4App::RegisterShutdownCallbackFunction(bool (*)(void*), void*) @ 0x10001311e
//   cSC4App::UnregisterShutdownCallbackFunction(bool (*)(void*), void*) @ 0x1000131ce
//   cSC4App::ApplyVideoPreferences(SC4VideoPreferences const&) @ 0x10001323c
//   cSC4App::GetAutoVideoPreferences(SC4VideoPreferences*) @ 0x100013430
//   cSC4App::GetDebugFunctionalityEnabled() const @ 0x1000135cc
//   cSC4App::SetDebugFunctionalityEnabled(bool) @ 0x1000135e4
//   cSC4App::GetPopupDialogsEnabled() const @ 0x100013600
//   cSC4App::SetPopupDialogsEnabled(bool) @ 0x100013618
//   cSC4App::GetAppState() const @ 0x100013634
//   cSC4App::GetMainWindow() const @ 0x10001363e
//   cSC4App::GetFeatureManager() const @ 0x10001364c
//   cSC4App::GetCheatCodeManager() const @ 0x10001365a
//   cSC4App::GetNation() @ 0x100013668
//   cSC4App::GetRegion() @ 0x100013676
//   cSC4App::GetRegionalCity() @ 0x100013688
//   cSC4App::GetDebugConsole() const @ 0x1000136c2
//   cSC4App::GetGimexFactory() const @ 0x1000136d0
//   cSC4App::GetStringDetokenizer() const @ 0x1000136de
//   cSC4App::GetWinLocationSaver() const @ 0x1000136ec
//   cSC4App::GetRenderProperties() const @ 0x1000136fa
//   cSC4App::GetGlyphTextureManager() const @ 0x100013708
//   cSC4App::GetLuaInterpreter() const @ 0x100013716
//   cSC4App::GetTutorialRegistry() @ 0x100013724
//   cSC4App::IsRunFirstTimeAfterInstall() @ 0x100013732
//   cSC4App::RequestNewCity(cRZAutoRefCount<cISC4RegionalCity>*) @ 0x10004464e
//   cSC4App::RequestLoadCity() @ 0x1000446f0
//   cSC4App::DoLoadCityUI(cRZString&) @ 0x100044836
//   cSC4App::RequestCloseCity(bool) @ 0x1000448fa
//   cSC4App::RequestSaveCity(bool, bool) @ 0x100044a68
//   cSC4App::RequestQuit(bool, bool) @ 0x100044f12
//   cSC4App::RequestQuitFromRegion(bool) @ 0x1000450ce
//   cSC4App::RequestGoToRegionView(bool) @ 0x1000451e6
//   cSC4App::Quit() @ 0x100045366
//   cSC4App::NewCity() @ 0x1000453d4
//   cSC4App::UnregisterCityDBSegment() @ 0x100045be8
//   cSC4App::CloseCity() @ 0x100045c58
//   cSC4App::PartialCloseCity() @ 0x100045e68
//   cSC4App::SaveCity(bool) @ 0x100045f6e
//   cSC4App::RegisterCityDBSegment(cRZString const&) @ 0x100046d9e
//   cSC4App::DoMessage(cIGZWin*, cGZMessage&) @ 0x100046e70
//   cSC4App::DoCityObliteration() @ 0x100047350
//   cSC4App::SaveCityForObliteration(cRZString const&) @ 0x1000475cc
//   cSC4App::SetupResources() @ 0x1000b0a3c
//   cSC4App::CreateDBSegmentCallback(cIGZString const&, cIGZString const&, void*) @ 0x1000b12e6
//   cSC4App::LoadRegistry() @ 0x1000b20be
//   cSC4App::RegisterDBSegmentCallback(cRZString const&, cRZString const&, void*) @ 0x1000b269a
//   cSC4App::RegisterDBSegmentDirectoryFilesCallback(cRZString const&, cRZString const&, void*) @ 0x1000b2c22
//
// cSC4GameDataRegistry (47 entry points)
//   cSC4GameDataRegistry::QueryInterface(unsigned int, void**) @ 0x1006bfdd0
//   cSC4GameDataRegistry::cSC4GameDataRegistry() @ 0x1006bfe1a / 0x1006c00cc
//   cSC4GameDataRegistry::~cSC4GameDataRegistry() @ 0x1006c00d6 / 0x1006c01cc / 0x1006c01f2
//   cSC4GameDataRegistry::Init() @ 0x1006c023c
//   cSC4GameDataRegistry::Shutdown() @ 0x1006c0284
//   cSC4GameDataRegistry::OnTick(unsigned int) @ 0x1006c03b2
//   cSC4GameDataRegistry::AddChildEntry(cIGZUnknown*, char const*, unsigned long, int, void*) @ 0x1006c03c2
//   cSC4GameDataRegistry::AddChildEntry(void*, char const*, bool (*)(void*, cIGZVariant&, bool, cISC4GameDataRegistry*), unsigned long, int, void*) @ 0x1006c0578
//   cSC4GameDataRegistry::AddChildEntry(void*, char const*, bool (*)(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*), unsigned long, int, void*) @ 0x1006c0842
//   cSC4GameDataRegistry::RemoveChildEntry(cIGZUnknown*) @ 0x1006c0a54
//   cSC4GameDataRegistry::RemoveEntry(cSC4GameDataRegistry::cEntry*) @ 0x1006c0a82
//   cSC4GameDataRegistry::RemoveChildEntry(void*) @ 0x1006c0c1e
//   cSC4GameDataRegistry::GetEntryID(cIGZUnknown*) const @ 0x1006c0c64
//   cSC4GameDataRegistry::GetEntryID(void*) const @ 0x1006c0d50
//   cSC4GameDataRegistry::GetNode(unsigned long) const @ 0x1006c0d92
//   cSC4GameDataRegistry::GetValue(unsigned long) const @ 0x1006c0dac
//   cSC4GameDataRegistry::IsValidEntry(unsigned long) const @ 0x1006c0dc6
//   cSC4GameDataRegistry::IsNodeEntry(unsigned long) const @ 0x1006c0e6e
//   cSC4GameDataRegistry::GetEntryName(unsigned long) const @ 0x1006c0e84
//   cSC4GameDataRegistry::DoValueExchange(unsigned long, cIGZVariant&, bool) @ 0x1006c0e96
//   cSC4GameDataRegistry::cEntry::DoValueExchange(cIGZVariant&, bool, cISC4GameDataRegistry*) @ 0x1006c0ec4
//   cSC4GameDataRegistry::DoValueCall(unsigned long, cIGZCommandParameterSet*, cIGZCommandParameterSet*) @ 0x1006c0efc
//   cSC4GameDataRegistry::CheckEntryFlags(unsigned long, int) const @ 0x1006c0f24
//   cSC4GameDataRegistry::SetEntryContext(unsigned long, void*) @ 0x1006c0f3e
//   cSC4GameDataRegistry::GetEntryContext(unsigned long) const @ 0x1006c0f54
//   cSC4GameDataRegistry::GetParentNodeEntry(unsigned long) const @ 0x1006c0f68
//   cSC4GameDataRegistry::GetRootNodeEntry() const @ 0x1006c0f7c
//   cSC4GameDataRegistry::InitialTargetNotify(unsigned long, void*) @ 0x1006c0f86
//   cSC4GameDataRegistry::AddNotificationTarget(cISC4GameDataRegistryNotificationTarget*) @ 0x1006c0faa
//   cSC4GameDataRegistry::VisitDepthFirst(unsigned long, bool (*)(unsigned long, void*), void*) @ 0x1006c10e4
//   cSC4GameDataRegistry::InitialMessageTargetNotify(unsigned long, void*) @ 0x1006c1200
//   cSC4GameDataRegistry::AddNotificationTarget(cIGZMessageTarget2*) @ 0x1006c12ba
//   cSC4GameDataRegistry::RemoveNotificationTarget(cISC4GameDataRegistryNotificationTarget*) @ 0x1006c1398
//   cSC4GameDataRegistry::RemoveNotificationTarget(cIGZMessageTarget2*) @ 0x1006c1446
//   cSC4GameDataRegistry::VisitBreadthFirst(unsigned long, bool (*)(unsigned long, void*), void*) @ 0x1006c14b8
//   cSC4GameDataRegistry::VisitBreadthFirstReverse(unsigned long, bool (*)(unsigned long, void*), void*) @ 0x1006c15f6
//   cSC4GameDataRegistry::VisitImmediateChildren(unsigned long, bool (*)(unsigned long, void*), void*) @ 0x1006c1674
//   cSC4GameDataRegistry::DebugDumpRecursive(unsigned long) @ 0x1006c199a
//   cSC4GameDataRegistry::AddRef() @ 0x1006c19c0
//   cSC4GameDataRegistry::Release() @ 0x1006c19ce
//   cSC4GameDataRegistry::cEntry::cEntry(cIGZUnknown*, cIGZString const*, cSC4GameDataRegistry::cEntry*, int) @ 0x1006c1a14
//   cSC4GameDataRegistry::cEntry::~cEntry() @ 0x1006c1ba2
//   cSC4GameDataRegistry::cEntry::cEntry(void*, cIGZString const*, bool (*)(void*, cIGZVariant&, bool, cISC4GameDataRegistry*), cSC4GameDataRegistry::cEntry*, int) @ 0x1006c1cc6
//   cSC4GameDataRegistry::cEntry::cEntry(void*, cIGZString const*, bool (*)(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*), cSC4GameDataRegistry::cEntry*, int) @ 0x1006c1d4e
//
// cSC4ObliterateCity (27 entry points)
//   cSC4ObliterateCity::cSC4ObliterateCity() @ 0x100376d2c
//   cSC4ObliterateCity::~cSC4ObliterateCity() @ 0x100376dce / 0x100376e52 / 0x100376e5c
//   cSC4ObliterateCity::Init() @ 0x100376e8a
//   cSC4ObliterateCity::StaticInit() @ 0x1003770ec
//   cSC4ObliterateCity::PostMessageA(unsigned int, unsigned long, unsigned long, unsigned long) @ 0x10037712c
//   cSC4ObliterateCity::Shutdown() @ 0x1003771fa
//   cSC4ObliterateCity::OnKeyDown(unsigned int, unsigned int) @ 0x100377340
//   cSC4ObliterateCity::OnMouseDownL(int, int, unsigned int) @ 0x100377356
//   cSC4ObliterateCity::OnMouseMove(int, int, unsigned int) @ 0x10037735e
//   cSC4ObliterateCity::OnMouseUpL(int, int, unsigned int) @ 0x100377366
//   cSC4ObliterateCity::DoMessage(cIGZMessage2*) @ 0x10037736e
//   cSC4ObliterateCity::DoAnimationTick(unsigned int) @ 0x1003773d4
//   cSC4ObliterateCity::GoToEndState() @ 0x10037740a
//   cSC4ObliterateCity::QueryInterface(unsigned int, void**) @ 0x100377448
//   cSC4ObliterateCity::DoIsolatedRemoval() @ 0x100377490
//   cSC4ObliterateCity::DoSmokeScreenForming(unsigned int) @ 0x1003776d6
//   cSC4ObliterateCity::DoFinalCleanUp(unsigned int) @ 0x100377738
//   cSC4ObliterateCity::IsCellRectOccupied(SC4Rect<int>&) const @ 0x1003777a4
//   cSC4ObliterateCity::CreateParticleEffect(char const*, cS3DVector2 const&, float, bool, cISC4VisualEffect*&) @ 0x100377870
//   cSC4ObliterateCity::ObliterateCellRect(SC4Rect<int>&, bool) @ 0x100377998
//   cSC4ObliterateCity::StartSmokeScreenFormingState() @ 0x100377c06
//   cSC4ObliterateCity::ReadTuningParameters() @ 0x100377ca0
//   cSC4ObliterateCity::StaticShutdown(void*) @ 0x100377d70
//   cSC4ObliterateCity::AddRef() @ 0x100377da2
//   cSC4ObliterateCity::Release() @ 0x100377dac
//
// cSC43DPlaceableObject (27 entry points)
//   cSC43DPlaceableObject::cSC43DPlaceableObject() @ 0x10012055c
//   cSC43DPlaceableObject::~cSC43DPlaceableObject() @ 0x1001205a0 / 0x100120634 / 0x10012063e
//   cSC43DPlaceableObject::operator delete(void*) @ 0x10012067c
//   cSC43DPlaceableObject::operator new(unsigned long) @ 0x10012068e
//   cSC43DPlaceableObject::GetModelCount() @ 0x10012069e
//   cSC43DPlaceableObject::GetModels(cIS3DModelInstance**, unsigned int&) @ 0x1001206a8
//   cSC43DPlaceableObject::AddModel(cIS3DModelInstance*) @ 0x1001206ee
//   cSC43DPlaceableObject::ResizeModelInstanceArray(int) @ 0x100120752
//   cSC43DPlaceableObject::RemoveModel(cIS3DModelInstance*) @ 0x1001207ba
//   cSC43DPlaceableObject::AddOverlay(unsigned int) @ 0x10012090a
//   cSC43DPlaceableObject::SetVisibility(bool) @ 0x1001209c8
//   cSC43DPlaceableObject::DoOverlayVisibilityChange() @ 0x100120a16
//   cSC43DPlaceableObject::AddModelsToRenderer() @ 0x100120a7e
//   cSC43DPlaceableObject::RemoveModelsFromRenderer() @ 0x100120b04
//   cSC43DPlaceableObject::SetOverlaysVisible(bool) @ 0x100120b6c
//   cSC43DPlaceableObject::IsVisible() const @ 0x100120bb4
//   cSC43DPlaceableObject::RemoveAll() @ 0x100120bc2
//   cSC43DPlaceableObject::AddRef() @ 0x100120cae
//   cSC43DPlaceableObject::Release() @ 0x100120cc0
//   cSC43DPlaceableObject::FinalRelease() @ 0x100120cde
//   cSC43DPlaceableObject::QueryInterface(unsigned int, void**) @ 0x100120d3e
//   cSC43DPlaceableObject::GetZoomLevelsCreatedFor() const @ 0x100120d60
//   cSC43DPlaceableObject::SetZoomLevelsCreatedFor(unsigned int) @ 0x100120d70
//   cSC43DPlaceableObject::GetRotationsCreatedFor() const @ 0x100120d84
//   cSC43DPlaceableObject::SetRotationsCreatedFor(unsigned int) @ 0x100120d94
//
// cSC4AnimationCoordinator (26 entry points)
//   cSC4AnimationCoordinator::cSC4AnimationCoordinator() @ 0x10035871c
//   cSC4AnimationCoordinator::~cSC4AnimationCoordinator() @ 0x100358784 / 0x100358a1e / 0x100358a28
//   cSC4AnimationCoordinator::Shutdown() @ 0x1003588c4
//   cSC4AnimationCoordinator::Init() @ 0x100358a56
//   cSC4AnimationCoordinator::RectToOccupantSetRect(SC4Rect<float> const&, SC4Rect<int>&) @ 0x100358e50
//   cSC4AnimationCoordinator::EvaluatePreferences() @ 0x100358ecc
//   cSC4AnimationCoordinator::QueryInterface(unsigned int, void**) @ 0x100358f16
//   cSC4AnimationCoordinator::AddRef() @ 0x100358f38
//   cSC4AnimationCoordinator::Release() @ 0x100358f46
//   cSC4AnimationCoordinator::DoMessage(cIGZMessage2*) @ 0x100358f62
//   cSC4AnimationCoordinator::UpdateAnimatableOccupantsCityCellRect(SC4Rect<int> const&) @ 0x100359114
//   cSC4AnimationCoordinator::DoOccupantBudgetChanged(cISC4Occupant*) @ 0x10035914c
//   cSC4AnimationCoordinator::UpdateVisibleOccupantSetRect() @ 0x10035918e
//   cSC4AnimationCoordinator::UpdateAllAnimatableOccupants() @ 0x1003592ca
//   cSC4AnimationCoordinator::SetOccupantAnimation(cISC4Occupant*, bool) @ 0x1003592fa
//   cSC4AnimationCoordinator::GetOccupantSetCell(cISC4Occupant*, int&, int&) @ 0x1003596e2
//   cSC4AnimationCoordinator::ShouldAnimate(cISC4Occupant*) @ 0x1003597e8
//   cSC4AnimationCoordinator::AddToTickManager(cISC4Occupant*, bool) @ 0x100359872
//   cSC4AnimationCoordinator::CityCellRectToOccupantSetRect(SC4Rect<int> const&, SC4Rect<int>&) @ 0x100359926
//   cSC4AnimationCoordinator::GetCellRectDifferences(std::__1::vector<SC4Point<int>, std::__1::allocator<SC4Point<int>>>&, SC4Rect<int> const&, SC4Rect<int> const&) @ 0x1003599b4
//   cSC4AnimationCoordinator::UpdateAnimatableOccupants(std::__1::unordered_set<cRZAutoRefCount<cISC4Occupant>, cSC4AnimationCoordinator::VoidPtrHash, std::__1::equal_to<cRZAutoRefCount<cISC4Occupant>>, std::__1::allocator<cRZAutoRefCount<cISC4Occupant>>>&, int, int) @ 0x100359ace
//   cSC4AnimationCoordinator::UpdateAnimatableOccupants(SC4Rect<int> const&) @ 0x100359b76
//   cSC4AnimationCoordinator::RequiresFundingToAnimate(cISCPropertyHolder const*, unsigned int&, unsigned int&) @ 0x100359c40
//   cSC4AnimationCoordinator::GetActualFundingForOccupant(cISC4Occupant*, unsigned int) @ 0x100359cae
//
// cSC4LightingManager (40 entry points)
//   cSC4LightingManager::cSC4LightingManager() @ 0x100775628 / 0x10077563e
//   cSC4LightingManager::~cSC4LightingManager() @ 0x1007757cc / 0x1007758c8 / 0x1007758d2
//   cSC4LightingManager::Init() @ 0x100775900
//   cSC4LightingManager::ReadProperties() @ 0x100775b44
//   cSC4LightingManager::Shutdown() @ 0x100776674
//   cSC4LightingManager::QueryInterface(unsigned int, void**) @ 0x10077680c
//   cSC4LightingManager::AddRef() @ 0x10077684e
//   cSC4LightingManager::Release() @ 0x10077686a
//   cSC4LightingManager::DoMessage(cIGZMessage2*) @ 0x1007768ae
//   cSC4LightingManager::DoZoomAndRotationChange(int, int) @ 0x100776e40
//   cSC4LightingManager::FindHillShadows() @ 0x1007772f4
//   cSC4LightingManager::GetTimeOfDay() const @ 0x1007776fe
//   cSC4LightingManager::SetTimeOfDay(float) @ 0x10077770a
//   cSC4LightingManager::SetTimeDependentLightingValues() @ 0x100777a4c
//   cSC4LightingManager::SetLightingTint(cS3DVector3 const&, float) @ 0x100777fb6
//   cSC4LightingManager::ResetLightingTint() @ 0x100778008
//   cSC4LightingManager::IsNight() const @ 0x100778038
//   cSC4LightingManager::GetGlobalLight(cS3DVector3*) @ 0x100778042
//   cSC4LightingManager::GetModelLight(cIS3DModelInstance*, cS3DVector3*) @ 0x100778062
//   cSC4LightingManager::GetModelColor(cS3DVector3 const&) @ 0x1007788e6
//   cSC4LightingManager::GetTerrainLighting(cS3DVector3 const&, unsigned char&, unsigned char&, unsigned char&) @ 0x100778a90
//   cSC4LightingManager::GetShadowDirection() @ 0x100778d48
//   cSC4LightingManager::GetShadowParams(float*, float*, cS3DVector3*) @ 0x100778d6c
//   cSC4LightingManager::GetShadowStrengthFromLocation(cS3DVector2 const&) @ 0x100778d9c
//   cSC4LightingManager::GetDecalShadowParams(cISCPropertyHolder*, unsigned int*, float*, float*, float*, cS3DVector2*, cS3DVector3*) @ 0x100778e8a
//   cSC4LightingManager::CalcShadowProjection(int, cS3DVector3 const*, cS3DVector2 const*, cS3DTransform const&, cS3DMatrix4*) @ 0x10077909a
//   cSC4LightingManager::CalcShadowTerrainRect(int, cS3DVector3 const*, cS3DBoundingBox const&, cRZRect*) @ 0x1007798dc
//   cSC4LightingManager::GetColor() @ 0x100779f6e
//   cSC4LightingManager::GetColor(unsigned char&, unsigned char&, unsigned char&) @ 0x100779fa8
//   cSC4LightingManager::GetColor(cS3DVector3 const&) @ 0x100779fe4
//   cSC4LightingManager::GetColor(cS3DVector3 const&, unsigned char&, unsigned char&, unsigned char&) @ 0x10077a292
//   cSC4LightingManager::GetColor(cS3DVector3 const&, cS3DVector3 const&) @ 0x10077a334
//   cSC4LightingManager::GetColor(cS3DVector3 const&, cS3DVector3 const&, unsigned char&, unsigned char&, unsigned char&) @ 0x10077a4d2
//   cSC4LightingManager::GetTerrainColor(cS3DVector3 const&, cS3DVector3 const&, unsigned char&, unsigned char&, unsigned char&) @ 0x10077a6e8
//   cSC4LightingManager::GetTerrainColorFloor(cS3DVector3 const&, cS3DVector3 const&, unsigned char&, unsigned char&, unsigned char&) @ 0x10077a9ca
//   cSC4LightingManager::GetVertexColors(int, cS3DVector3 const*, int, cS3DColorARGB8*, int, cS3DMatrix3 const&) @ 0x10077ab0a
//   cSC4LightingManager::GetTerrainVertexColors(int, int, int, cS3DVector3 const*, int, unsigned char*, int) @ 0x10077ad3a
//
// cSC4ModelBackgroundLoader (25 entry points)
//   cSC4ModelBackgroundLoader::cSC4ModelBackgroundLoader() @ 0x100361ce0 / 0x1003620cc
//   cSC4ModelBackgroundLoader::~cSC4ModelBackgroundLoader() @ 0x1003620d6 / 0x10036245a / 0x100362464
//   cSC4ModelBackgroundLoader::Shutdown() @ 0x1003623a0
//   cSC4ModelBackgroundLoader::Init(cSC4ModelMaker*) @ 0x10036249a
//   cSC4ModelBackgroundLoader::ReadTunableValues() @ 0x1003625c4
//   cSC4ModelBackgroundLoader::ClearResourceSet() @ 0x100362c62
//   cSC4ModelBackgroundLoader::AddRef() @ 0x100362caa
//   cSC4ModelBackgroundLoader::Release() @ 0x100362cb8
//   cSC4ModelBackgroundLoader::QueryInterface(unsigned int, void**) @ 0x100362cd4
//   cSC4ModelBackgroundLoader::ThreadFunction() @ 0x100362cf2
//   cSC4ModelBackgroundLoader::BeginLoadTask(cSC4ModelBackgroundLoader::tTask&) @ 0x100362ec8
//   cSC4ModelBackgroundLoader::ContinueLoadTask(cSC4ModelBackgroundLoader::tTask&, float&) @ 0x100362fa4
//   cSC4ModelBackgroundLoader::AddMainThreadMessage(cSC4ModelBackgroundLoader::eMainThreadMessageID) @ 0x10036322e
//   cSC4ModelBackgroundLoader::StartLoadingForZoomAndRotation(int, int, SC4Rect<int>&, SC4Point<int>&) @ 0x100363274
//   cSC4ModelBackgroundLoader::RefocusLoadToNewViewLocation(SC4Rect<int>&, SC4Point<int>&) @ 0x100363436
//   cSC4ModelBackgroundLoader::CancelAllLoadTasks() @ 0x100363592
//   cSC4ModelBackgroundLoader::SuspendLoading() @ 0x100363604
//   cSC4ModelBackgroundLoader::ResumeLoading() @ 0x10036360e
//   cSC4ModelBackgroundLoader::LoadResourcesForOccupant(cISC4Occupant*, int, int) @ 0x100363618
//   cSC4ModelBackgroundLoader::ProcessBackgroundThreadToMainThreadCommunications() @ 0x1003639ee
//   cSC4ModelBackgroundLoader::ProcessLoadedCells() @ 0x100363b70
//   cSC4ModelBackgroundLoader::ExpandLoadRect(SC4Rect<int>&, int) @ 0x100363da8
//
// cSC4LineItem (23 entry points)
//   cSC4LineItem::~cSC4LineItem() @ 0x100245924 / 0x100245992 / 0x10024599c
//   cSC4LineItem::QueryInterface(unsigned int, void**) @ 0x1002459ca
//   cSC4LineItem::AddRef() @ 0x100245a0c
//   cSC4LineItem::Release() @ 0x100245a28
//   cSC4LineItem::SetFullExpenses(long long) @ 0x100245a6c
//   cSC4LineItem::AddToFullExpenses(long long) @ 0x100245ad8
//   cSC4LineItem::SetIncome(long long) @ 0x100245aea
//   cSC4LineItem::AddToIncome(long long) @ 0x100245b10
//   cSC4LineItem::SetName(unsigned int, unsigned int) @ 0x100245d36
//   cSC4LineItem::GetName(cRZString&) @ 0x100245dc0
//   cSC4LineItem::GetID() const @ 0x100245e4e
//   cSC4LineItem::GetType() const @ 0x100245e58
//   cSC4LineItem::SetType(cISC4LineItem::LineItemType) @ 0x100245e64
//   cSC4LineItem::GetSecondaryInfoField() const @ 0x100245e70
//   cSC4LineItem::SetSecondaryInfoField(long long) @ 0x100245e7a
//   cSC4LineItem::GetFullExpenses() const @ 0x100245e84
//   cSC4LineItem::GetCurrentExpenses() const @ 0x100245e8e
//   cSC4LineItem::GetIncome() const @ 0x100245e98
//   cSC4LineItem::GetDisplayFlags() @ 0x100245ea2
//   cSC4LineItem::SetDisplayFlag(unsigned int, bool) @ 0x100245eac
//   cSC4LineItem::IsLocalFundingItem() const @ 0x100245ec6

// ---------------------------------------------------------------------------
// Wave 3 Part 2 method index
// Seventy-three classes and every 0x100xxxxxx entry point from the Wave 3
// Part 2 workpack. Addresses and sizes are the macOS x86_64 SimCity 4
// Deluxe leaves recorded in kWave3Part2Methods.
// ---------------------------------------------------------------------------

struct Wave3Part2Method {
    Uint64 address;
    Uint32 sizeBytes;
    const char* name;
};

inline constexpr std::array<Wave3Part2Method, 452> kWave3Part2Methods = {{
    // cSC4WinLotConfigurationEditor (90 entry points)
    {0x100417602u, 162u, "cSC4WinLotConfigurationEditor::Open(cGZPersistResourceKey const&)"},
    {0x1004176a4u, 108u, "cSC4WinLotConfigurationEditor::GetIdentityInfoForLotConfigurationType(unsigned int, cSC4LotConfigurationIdentityInfo&)"},
    {0x100417890u, 376u, "cSC4WinLotConfigurationEditor::cSC4WinLotConfigurationEditor()"},
    {0x100417dacu, 434u, "cSC4WinLotConfigurationEditor::Init()"},
    {0x100417f5eu, 570u, "cSC4WinLotConfigurationEditor::StaticInit()"},
    {0x100418198u, 770u, "cSC4WinLotConfigurationEditor::LoadCursors()"},
    {0x10041849au, 64u, "cSC4WinLotConfigurationEditor::AddPropFamilyName(cRZString const&, cRZString const&, void*)"},
    {0x1004184dau, 64u, "cSC4WinLotConfigurationEditor::AddBuildingFamilyName(cRZString const&, cRZString const&, void*)"},
    {0x10041851au, 68u, "cSC4WinLotConfigurationEditor::AddFloraFamilyName(cRZString const&, cRZString const&, void*)"},
    {0x10041855eu, 536u, "cSC4WinLotConfigurationEditor::PostInit()"},
    {0x100418776u, 422u, "cSC4WinLotConfigurationEditor::Shutdown()"},
    {0x10041891cu, 186u, "cSC4WinLotConfigurationEditor::GetIdentityInfoForLotConfigurationExemplarKey(cGZPersistResourceKey const&, cSC4LotConfigurationIdentityInfo&)"},
    {0x1004189d6u, 366u, "cSC4WinLotConfigurationEditor::SetTitle()"},
    {0x100418b44u, 570u, "cSC4WinLotConfigurationEditor::SetLotSize(unsigned int, unsigned int, bool)"},
    {0x100418d7eu, 556u, "cSC4WinLotConfigurationEditor::RemoveOutOfBoundsLotConfigurationObjects()"},
    {0x100418faau, 1116u, "cSC4WinLotConfigurationEditor::Prepare3DView()"},
    {0x100419406u, 196u, "cSC4WinLotConfigurationEditor::GoTo3DView()"},
    {0x1004194cau, 372u, "cSC4WinLotConfigurationEditor::Close(bool, bool)"},
    {0x10041963eu, 180u, "cSC4WinLotConfigurationEditor::MessageBoxA(cRZString const&, unsigned int)"},
    {0x1004196f2u, 518u, "cSC4WinLotConfigurationEditor::Save(bool, bool)"},
    {0x1004198f8u, 294u, "cSC4WinLotConfigurationEditor::VerifyObjectsAreWithinLotBounds(bool, bool)"},
    {0x100419a30u, 54u, "cSC4WinLotConfigurationEditor::WriteLotData()"},
    {0x100419a66u, 84u, "cSC4WinLotConfigurationEditor::GetDefaultPath(cRZString&)"},
    {0x100419abau, 1860u, "cSC4WinLotConfigurationEditor::SaveAs(bool, bool, bool)"},
    {0x10041a1feu, 950u, "cSC4WinLotConfigurationEditor::GetIdentityInfoForLotConfigurationExemplarPath(cRZString const&, cSC4LotConfigurationIdentityInfo&)"},
    {0x10041a5b4u, 74u, "cSC4WinLotConfigurationEditor::DoMessage(cGZMessage&)"},
    {0x10041a5feu, 140u, "cSC4WinLotConfigurationEditor::DoMessage(cIGZWin*, cGZMessage&)"},
    {0x10041a68au, 394u, "cSC4WinLotConfigurationEditor::DoDialogBoxGridSort(unsigned int)"},
    {0x10041a814u, 288u, "cSC4WinLotConfigurationEditor::DoDialogBoxScrollToLetter(cIGZWin*, char)"},
    {0x10041a946u, 780u, "cSC4WinLotConfigurationEditor::GZOnKeyDown(unsigned int, unsigned int)"},
    {0x10041ac52u, 120u, "cSC4WinLotConfigurationEditor::DoAddNewBuilding()"},
    {0x10041accau, 120u, "cSC4WinLotConfigurationEditor::DoAddNewLotBaseTexture()"},
    {0x10041ad42u, 132u, "cSC4WinLotConfigurationEditor::DoAddNewProp(bool, bool)"},
    {0x10041adc6u, 120u, "cSC4WinLotConfigurationEditor::DoAddNewFlora()"},
    {0x10041ae3eu, 320u, "cSC4WinLotConfigurationEditor::DoShowHelpUI(int)"},
    {0x10041af7eu, 434u, "cSC4WinLotConfigurationEditor::MoveObject(cSC4LotConfigurationObject*, unsigned int, unsigned int)"},
    {0x10041b130u, 146u, "cSC4WinLotConfigurationEditor::RotateObject(cSC4LotConfigurationObject*, unsigned int, unsigned int)"},
    {0x10041b1c2u, 70u, "cSC4WinLotConfigurationEditor::MirrorObject(cSC4LotConfigurationObject*, unsigned int, unsigned int)"},
    {0x10041b208u, 602u, "cSC4WinLotConfigurationEditor::DuplicateObject(cSC4LotConfigurationObject*, unsigned int, unsigned int)"},
    {0x10041b462u, 546u, "cSC4WinLotConfigurationEditor::DeleteLotConfigurationObject(cSC4LotConfigurationObject*, bool)"},
    {0x10041b684u, 758u, "cSC4WinLotConfigurationEditor::CycleSelectedObject()"},
    {0x10041b982u, 222u, "cSC4WinLotConfigurationEditor::GZOnMouseDownL(int, int, unsigned int)"},
    {0x10041ba60u, 408u, "cSC4WinLotConfigurationEditor::DoEditMouseDownL(int, int, unsigned int)"},
    {0x10041bbf8u, 218u, "cSC4WinLotConfigurationEditor::GZOnMouseDownR(int, int, unsigned int)"},
    {0x10041bcd2u, 178u, "cSC4WinLotConfigurationEditor::DoEditMouseDownR(int, int, unsigned int)"},
    {0x10041bd84u, 1600u, "cSC4WinLotConfigurationEditor::DoOutsideAreaMenu(int, int)"},
    {0x10041c3c4u, 292u, "cSC4WinLotConfigurationEditor::GZOnMouseUpL(int, int, unsigned int)"},
    {0x10041c4e8u, 260u, "cSC4WinLotConfigurationEditor::GZOnMouseMove(int, int, unsigned int)"},
    {0x10041c5ecu, 316u, "cSC4WinLotConfigurationEditor::DoEditMouseMove(int, int, unsigned int)"},
    {0x10041c728u, 1430u, "cSC4WinLotConfigurationEditor::DoHitTest(int, int, unsigned int, cSC4LotConfigurationObject*&)"},
    {0x10041ccbeu, 194u, "cSC4WinLotConfigurationEditor::SetSelectedLotConfigurationObject(cSC4LotConfigurationObject*)"},
    {0x10041cd80u, 1596u, "cSC4WinLotConfigurationEditor::AddNewLotConfigurationObject(int, int, unsigned int)"},
    {0x10041d3bcu, 72u, "cSC4WinLotConfigurationEditor::LotFoundationIsPresent()"},
    {0x10041d404u, 1058u, "cSC4WinLotConfigurationEditor::DoSetFoundationUI(unsigned int, cSC4LotConfigurationObject*)"},
    {0x10041d826u, 260u, "cSC4WinLotConfigurationEditor::ToggleFence(int, int, unsigned int)"},
    {0x10041d92au, 1458u, "cSC4WinLotConfigurationEditor::DoLotFoundationMenu(int, int)"},
    {0x10041dedcu, 968u, "cSC4WinLotConfigurationEditor::DoFenceMenu(int, int)"},
    {0x10041e2a4u, 24884u, "cSC4WinLotConfigurationEditor::DoLotConfigurationObjectMenu(int, int, cSC4LotConfigurationObject*)"},
    {0x1004243d8u, 142u, "cSC4WinLotConfigurationEditor::MoveObject(cSC4LotConfigurationObject*, float, float)"},
    {0x100424466u, 990u, "cSC4WinLotConfigurationEditor::DoSet3DViewSlopeUI()"},
    {0x100424844u, 692u, "cSC4WinLotConfigurationEditor::DoSetNetworkNodePathUI(cSC4LotConfigurationObject*)"},
    {0x100424af8u, 8052u, "cSC4WinLotConfigurationEditor::DoEditAreaMenu(int, int)"},
    {0x100426a6cu, 578u, "cSC4WinLotConfigurationEditor::DoSetLotNameUI()"},
    {0x100426caeu, 502u, "cSC4WinLotConfigurationEditor::DoSetLotSizeUI()"},
    {0x100426ea4u, 1478u, "cSC4WinLotConfigurationEditor::DoSetRetainingWallUI()"},
    {0x10042746au, 11360u, "cSC4WinLotConfigurationEditor::DoSetLotCompatibilitiesUI()"},
    {0x10042a0cau, 2174u, "cSC4WinLotConfigurationEditor::DoSelectBuildingFamiliesUI(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&, std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>*)"},
    {0x10042a948u, 1910u, "cSC4WinLotConfigurationEditor::DoSelectFloraFamiliesUI(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&, std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>*)"},
    {0x10042b0beu, 2198u, "cSC4WinLotConfigurationEditor::DoSelectPropFamiliesUI(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&, std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>*, bool, bool)"},
    {0x10042b954u, 128u, "cSC4WinLotConfigurationEditor::DoSelectLotBaseTexturesUI(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&, std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>*)"},
    {0x10042b9d4u, 602u, "cSC4WinLotConfigurationEditor::AddLotFoundation(unsigned int, bool)"},
    {0x10042bc2eu, 408u, "cSC4WinLotConfigurationEditor::GetFoundationNameAndImplementationType(unsigned int, cRZString&, cISC4BuildingDevelopmentSimulator*)"},
    {0x10042bdc6u, 288u, "cSC4WinLotConfigurationEditor::GetRetainingWallName(unsigned int, cRZString&, cISC4BuildingDevelopmentSimulator*)"},
    {0x10042bee6u, 466u, "cSC4WinLotConfigurationEditor::GetBuildingFamilyName(unsigned int, cRZString&, cISC4BuildingDevelopmentSimulator*)"},
    {0x10042c0b8u, 194u, "cSC4WinLotConfigurationEditor::GetBuildingName(unsigned int, cRZString&, cISC4BuildingDevelopmentSimulator*)"},
    {0x10042c17au, 182u, "cSC4WinLotConfigurationEditor::GetFloraFamilyName(unsigned int, cRZString&)"},
    {0x10042c230u, 474u, "cSC4WinLotConfigurationEditor::GetPropFamilyName(unsigned int, cRZString&, cISC4PropManager*)"},
    {0x10042c40au, 182u, "cSC4WinLotConfigurationEditor::GetPropName(unsigned int, cRZString&, cISC4PropManager*)"},
    {0x10042c4c0u, 718u, "cSC4WinLotConfigurationEditor::GetAllLotBaseTextureTypes(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&)"},
    {0x10042c78eu, 1110u, "cSC4WinLotConfigurationEditor::DoSelectTexturesUI(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&, std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>*, int (*)(cIGZWinGrid::CellData const*, cIGZWinGrid::CellData const*))"},
    {0x10042cbe4u, 106u, "cSC4WinLotConfigurationEditor::LotTextureSort(cIGZWinGrid::CellData const*, cIGZWinGrid::CellData const*)"},
    {0x10042cc4eu, 1394u, "cSC4WinLotConfigurationEditor::DrawCurrentLotConfiguration()"},
    {0x10042d1c0u, 2046u, "cSC4WinLotConfigurationEditor::DrawLotConfigurationObject(cSC4LotConfigurationObject*, cIGZBuffer*)"},
    {0x10042d9beu, 412u, "cSC4WinLotConfigurationEditor::CreateNewLotConfiguration()"},
    {0x10042db5au, 548u, "cSC4WinLotConfigurationEditor::ReadLotConfigurationExemplar(cGZPersistResourceKey const&)"},
    {0x10042dd7eu, 944u, "cSC4WinLotConfigurationEditor::GetLotConfigurationObjectName(cSC4LotConfigurationObject*, cRZString&, bool)"},
    {0x10042e12eu, 206u, "cSC4WinLotConfigurationEditor::ToggleFence(SC4Rect<signed char> const&)"},
    {0x10042e1fcu, 506u, "cSC4WinLotConfigurationEditor::MoveObjectToWithinLotBounds(cSC4LotConfigurationObject*, bool)"},
    {0x10042e7c4u, 114u, "cSC4WinLotConfigurationEditor::IsObjectWithinLotBounds(cSC4LotConfigurationObject*, bool)"},
    {0x10042e836u, 1520u, "cSC4WinLotConfigurationEditor::Refresh3DView()"},

    // cSC4BudgetUIWinProc (23 entry points)
    {0x1002e3966u, 488u, "cSC4BudgetUIWinProc::cSC4BudgetUIWinProc()"},
    {0x1002e3b4eu, 3102u, "cSC4BudgetUIWinProc::InitBalanceSheet()"},
    {0x1002e4872u, 42u, "cSC4BudgetUIWinProc::Shutdown()"},
    {0x1002e489cu, 232u, "cSC4BudgetUIWinProc::RegisterMessageNotification(bool)"},
    {0x1002e4984u, 354u, "cSC4BudgetUIWinProc::EndDialog()"},
    {0x1002e4b80u, 3388u, "cSC4BudgetUIWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x1002e58bcu, 246u, "cSC4BudgetUIWinProc::SetFormattedAmountInTextControl(unsigned int, long long)"},
    {0x1002e59b2u, 190u, "cSC4BudgetUIWinProc::PrintPercentageString(int, int, cRZString&)"},
    {0x1002e5a70u, 270u, "cSC4BudgetUIWinProc::SetTaxes()"},
    {0x1002e5b7eu, 1730u, "cSC4BudgetUIWinProc::UpdateBalanceSheetInfo()"},
    {0x1002e6240u, 122u, "cSC4BudgetUIWinProc::RemoveLoanWindow()"},
    {0x1002e62bau, 2054u, "cSC4BudgetUIWinProc::UpdateLoanDisplay()"},
    {0x1002e6ac0u, 166u, "cSC4BudgetUIWinProc::TakePendingLoans()"},
    {0x1002e6b66u, 154u, "cSC4BudgetUIWinProc::CancelTaxesWindow()"},
    {0x1002e6c00u, 122u, "cSC4BudgetUIWinProc::RemoveTaxesWindow()"},
    {0x1002e6c7au, 912u, "cSC4BudgetUIWinProc::DisplayTaxesWindow()"},
    {0x1002e700au, 540u, "cSC4BudgetUIWinProc::DoSubBudgetWindow(unsigned int)"},
    {0x1002e7226u, 3204u, "cSC4BudgetUIWinProc::DisplayLoanWindow()"},
    {0x1002e7eb8u, 78u, "cSC4BudgetUIWinProc::DoMessage(cIGZMessage2*)"},
    {0x1002e7f18u, 1424u, "cSC4BudgetUIWinProc::Init()"},
    {0x1002e84a8u, 258u, "cSC4BudgetUIWinProc::ShowWindow()"},
    {0x1002e85aau, 190u, "cSC4BudgetUIWinProc::HideWindow()"},
    {0x1002e8674u, 1326u, "cSC4BudgetUIWinProc::CalculateValues()"},

    // cSC4DBSegmentIStream (20 entry points)
    {0x1003cdc1cu, 292u, "cSC4DBSegmentIStream::Open(cISC4DBSegment*, cGZPersistResourceKey const&)"},
    {0x1003cdd4eu, 154u, "cSC4DBSegmentIStream::Close()"},
    {0x1003cde52u, 74u, "cSC4DBSegmentIStream::ReadGZSerializable(cIGZSerializable**, bool)"},
    {0x1003cdeeau, 42u, "cSC4DBSegmentIStream::ReadResKey(cGZPersistResourceKey&)"},
    {0x1003ce066u, 216u, "cSC4DBSegmentIStream::Skip(unsigned int)"},
    {0x1003ce14cu, 40u, "cSC4DBSegmentIStream::GetSint8(signed char&)"},
    {0x1003ce19cu, 40u, "cSC4DBSegmentIStream::GetUint8(unsigned char&)"},
    {0x1003ce1ecu, 40u, "cSC4DBSegmentIStream::GetSint16(short&)"},
    {0x1003ce23cu, 40u, "cSC4DBSegmentIStream::GetUint16(unsigned short&)"},
    {0x1003ce28cu, 40u, "cSC4DBSegmentIStream::GetSint32(int&)"},
    {0x1003ce2dcu, 40u, "cSC4DBSegmentIStream::GetUint32(unsigned int&)"},
    {0x1003ce32cu, 42u, "cSC4DBSegmentIStream::GetSint64(long long&)"},
    {0x1003ce380u, 42u, "cSC4DBSegmentIStream::GetUint64(unsigned long long&)"},
    {0x1003ce3d4u, 42u, "cSC4DBSegmentIStream::GetFloat32(float&)"},
    {0x1003ce428u, 42u, "cSC4DBSegmentIStream::GetFloat64(double&)"},
    {0x1003ce47cu, 206u, "cSC4DBSegmentIStream::GetRZCharStr(char*, unsigned int)"},
    {0x1003ce558u, 42u, "cSC4DBSegmentIStream::GetGZStr(cIGZString&)"},
    {0x1003ce5acu, 46u, "cSC4DBSegmentIStream::GetGZSerializable(cIGZSerializable&)"},
    {0x1003ce608u, 42u, "cSC4DBSegmentIStream::GetVoid(void*, unsigned int)"},
    {0x1003ce678u, 78u, "cSC4DBSegmentIStream::SetUserData(cIGZVariant*)"},

    // cSC4ViewInputControlZone (18 entry points)
    {0x1003c37aau, 338u, "cSC4ViewInputControlZone::cSC4ViewInputControlZone(cISC4ZoneManager::ZoneType)"},
    {0x1003c3906u, 260u, "cSC4ViewInputControlZone::Init()"},
    {0x1003c3a0au, 282u, "cSC4ViewInputControlZone::Shutdown()"},
    {0x1003c3b6cu, 164u, "cSC4ViewInputControlZone::DoMessage(cIGZMessage2*)"},
    {0x1003c3c10u, 412u, "cSC4ViewInputControlZone::DoCursorUpdate()"},
    {0x1003c3dbau, 860u, "cSC4ViewInputControlZone::FloodFillRegion(SC4Point<int>)"},
    {0x1003c4116u, 2668u, "cSC4ViewInputControlZone::ComputeAndMarkSelectedArea()"},
    {0x1003c4b82u, 358u, "cSC4ViewInputControlZone::GetFailureReasonString(int, cRZString&)"},
    {0x1003c4ce8u, 494u, "cSC4ViewInputControlZone::ClearHighlights()"},
    {0x1003c4ed6u, 126u, "cSC4ViewInputControlZone::OnKeyDown(unsigned int, unsigned int)"},
    {0x1003c4f54u, 82u, "cSC4ViewInputControlZone::OnKeyUp(unsigned int, unsigned int)"},
    {0x1003c4fa6u, 304u, "cSC4ViewInputControlZone::EndInput()"},
    {0x1003c50d6u, 484u, "cSC4ViewInputControlZone::OnMouseDownL(int, int, unsigned int)"},
    {0x1003c52bau, 556u, "cSC4ViewInputControlZone::OnMouseMove(int, int, unsigned int)"},
    {0x1003c54e6u, 1164u, "cSC4ViewInputControlZone::OnMouseUpL(int, int, unsigned int)"},
    {0x1003c5972u, 266u, "cSC4ViewInputControlZone::PlaceZone(cISC4ZoneManager::ZoneType, SC4CellRegion<int>&, bool, bool, long long*, int*)"},
    {0x1003c5a7cu, 140u, "cSC4ViewInputControlZone::OnMouseExit()"},
    {0x1003c5b08u, 332u, "cSC4ViewInputControlZone::TestCell(SC4Point<int> const&, SC4Point<int> const&, int, std::__1::list<SC4Point<int>, std::__1::allocator<SC4Point<int>>> const&)"},

    // cSC4AnimationTickManager (13 entry points)
    {0x100364aceu, 526u, "cSC4AnimationTickManager::cSC4AnimationTickManager()"},
    {0x100365010u, 218u, "cSC4AnimationTickManager::Init()"},
    {0x1003650fcu, 296u, "cSC4AnimationTickManager::Shutdown()"},
    {0x1003655e6u, 18u, "cSC4AnimationTickManager::DoMessage(cIGZMessage2*)"},
    {0x100365624u, 1338u, "cSC4AnimationTickManager::AddTickableObject(cIGZMessageTarget2*, unsigned int)"},
    {0x100365b5eu, 856u, "cSC4AnimationTickManager::RemoveTickableObject(cIGZMessageTarget2*, unsigned int)"},
    {0x100365eb6u, 152u, "cSC4AnimationTickManager::FindTickableObject(cIGZMessageTarget2*, unsigned int)"},
    {0x100365f4eu, 668u, "cSC4AnimationTickManager::DoAnimationTick()"},
    {0x1003661eau, 150u, "cSC4AnimationTickManager::DoDelayedAdditionsAndRemovals()"},
    {0x100366280u, 76u, "cSC4AnimationTickManager::PauseAnimationSystem(bool)"},
    {0x10036630au, 56u, "cSC4AnimationTickManager::StepAnimationSystem()"},
    {0x10036634eu, 58u, "cSC4AnimationTickManager::EnableAutoStep(bool, unsigned int)"},
    {0x100366388u, 80u, "cSC4AnimationTickManager::SetNewPriority(cIGZMessageTarget2*, unsigned int)"},

    // cSC4LotRetainingWallOccupant (12 entry points)
    {0x10015caf6u, 168u, "cSC4LotRetainingWallOccupant::cSC4LotRetainingWallOccupant()"},
    {0x10015ccc6u, 66u, "cSC4LotRetainingWallOccupant::Shutdown()"},
    {0x10015d1ceu, 146u, "cSC4LotRetainingWallOccupant::GetBoundingBox(float*, float*)"},
    {0x10015d290u, 78u, "cSC4LotRetainingWallOccupant::SetLot(cISC4Lot*)"},
    {0x10015d30eu, 273u, "cSC4LotRetainingWallOccupant::FindRetainingWallModel(cISC43DPlaceableObject*, int, cSC4LotRetainingWallModel*&)"},
    {0x10015d466u, 924u, "cSC4LotRetainingWallOccupant::AddRetainingWallModel(cISC4Occupant*, cISC4LotRetainingWallOccupant*, int, bool, bool)"},
    {0x10015d802u, 200u, "cSC4LotRetainingWallOccupant::AddRetainingWallModels(cISC4Occupant*, bool, bool)"},
    {0x10015d8cau, 578u, "cSC4LotRetainingWallOccupant::GetLotWallEdgeFlags(cISC4Lot*, float, SC4Rect<int> const&, int*, float&)"},
    {0x10015db0cu, 86u, "cSC4LotRetainingWallOccupant::GetRetainingWallType(cISC4Lot*)"},
    {0x10015db62u, 756u, "cSC4LotRetainingWallOccupant::AddRetainingWallOccupant(cISC4Lot*, bool)"},
    {0x10015de56u, 430u, "cSC4LotRetainingWallOccupant::UpdateRetainingWalls(cISC4Lot*)"},
    {0x10015e004u, 128u, "cSC4LotRetainingWallOccupant::UpdateSurroundingRetainingWalls(cISC4Lot*)"},

    // cSC4COMDirector (12 entry points)
    {0x100435bd2u, 4648u, "cSC4COMDirector::cSC4COMDirector()"},
    {0x100436dfau, 88u, "cSC4COMDirector::CreateSC4SimGridBool8()"},
    {0x100436e52u, 88u, "cSC4COMDirector::CreateSC4SimGridUint8()"},
    {0x100436eaau, 88u, "cSC4COMDirector::CreateSC4SimGridSint8()"},
    {0x100436f02u, 88u, "cSC4COMDirector::CreateSC4SimGridUint16()"},
    {0x100436f5au, 88u, "cSC4COMDirector::CreateSC4SimGridSint16()"},
    {0x100436fb2u, 88u, "cSC4COMDirector::CreateSC4SimGridUint32()"},
    {0x10043700au, 88u, "cSC4COMDirector::CreateSC4SimGridSint32()"},
    {0x100437062u, 88u, "cSC4COMDirector::CreateSC4SimGridUint64()"},
    {0x1004370bau, 88u, "cSC4COMDirector::CreateSC4SimGridSint64()"},
    {0x100437112u, 88u, "cSC4COMDirector::CreateSC4SimGridFloat32()"},
    {0x10043716au, 106u, "cSC4COMDirector::CreateSC4SimGridFloat64()"},

    // cSC4MySimInputControlDispatch (12 entry points)
    {0x1006b7750u, 316u, "cSC4MySimInputControlDispatch::cSC4MySimInputControlDispatch(cSC4MySim*)"},
    {0x1006b79b4u, 338u, "cSC4MySimInputControlDispatch::Init()"},
    {0x1006b7b06u, 388u, "cSC4MySimInputControlDispatch::Shutdown()"},
    {0x1006b7c8au, 14u, "cSC4MySimInputControlDispatch::OnKeyDown(unsigned int, unsigned int)"},
    {0x1006b7c98u, 1110u, "cSC4MySimInputControlDispatch::OnMouseDownL(int, int, unsigned int)"},
    {0x1006b80eeu, 316u, "cSC4MySimInputControlDispatch::PickPylonOccupant(int, int, cISC4PylonOccupant*&)"},
    {0x1006b822au, 98u, "cSC4MySimInputControlDispatch::SetNewMySim(cSC4MySim*)"},
    {0x1006b828cu, 172u, "cSC4MySimInputControlDispatch::CalculateDragPylonOffset(cISC4PylonOccupant*, int, int)"},
    {0x1006b8338u, 580u, "cSC4MySimInputControlDispatch::OnMouseMove(int, int, unsigned int)"},
    {0x1006b857cu, 186u, "cSC4MySimInputControlDispatch::SetHoverPylon(cISC4PylonOccupant*)"},
    {0x1006b86e6u, 46u, "cSC4MySimInputControlDispatch::FinalRelease()"},
    {0x1006b8714u, 442u, "cSC4MySimInputControlDispatch::DoMessage(cIGZMessage2*)"},

    // cSC4AudioListener (11 entry points)
    {0x1003dee96u, 55u, "cSC4AudioListener::Create()"},
    {0x1003f4af0u, 2592u, "cSC4AudioListener::cSC4AudioListener()"},
    {0x1003f55f0u, 32u, "cSC4AudioListener::cSC4AudioListener()"},
    {0x1003f57d0u, 384u, "cSC4AudioListener::Init()"},
    {0x1003f5a60u, 272u, "cSC4AudioListener::Shutdown()"},
    {0x1003f5c00u, 640u, "cSC4AudioListener::DoMessage(cIGZMessage2*)"},
    {0x1003f5e80u, 256u, "cSC4AudioListener::HandleCheatCodeMessage(cIGZMessage2*)"},
    {0x1003f5f80u, 832u, "cSC4AudioListener::HandleCityInitShutdownMessages(cIGZMessage2*)"},
    {0x1003f62c0u, 2160u, "cSC4AudioListener::UpdateListener()"},
    {0x1003f6b30u, 2048u, "cSC4AudioListener::_ReadTuningValues()"},
    {0x1003f7330u, 176u, "cSC4AudioListener::_CalculateZoomValues()"},

    // cSC4MySimBalloonWinProc (11 entry points)
    {0x1006ab090u, 492u, "cSC4MySimBalloonWinProc::start_balloon_sound()"},
    {0x1006ab27cu, 162u, "cSC4MySimBalloonWinProc::stop_balloon_sound(bool)"},
    {0x1006ab31eu, 326u, "cSC4MySimBalloonWinProc::cSC4MySimBalloonWinProc(cISC4MySim*)"},
    {0x1006ab5b4u, 446u, "cSC4MySimBalloonWinProc::Init()"},
    {0x1006ab772u, 114u, "cSC4MySimBalloonWinProc::tick_self(cIGZMessage2*)"},
    {0x1006ab7e4u, 122u, "cSC4MySimBalloonWinProc::Shutdown()"},
    {0x1006ab908u, 86u, "cSC4MySimBalloonWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x1006ab95eu, 524u, "cSC4MySimBalloonWinProc::on_paint()"},
    {0x1006abb6au, 662u, "cSC4MySimBalloonWinProc::DoMessage(cIGZMessage2*)"},
    {0x1006abe00u, 168u, "cSC4MySimBalloonWinProc::check_enabled()"},
    {0x1006abea8u, 888u, "cSC4MySimBalloonWinProc::set_new_balloon()"},

    // cSC4PedestrianManager (10 entry points)
    {0x1000f0cc0u, 380u, "cSC4PedestrianManager::cSC4PedestrianManager()"},
    {0x1000f0e46u, 86u, "cSC4PedestrianManager::Init()"},
    {0x1000f0e9cu, 62u, "cSC4PedestrianManager::Shutdown()"},
    {0x1000f0edau, 248u, "cSC4PedestrianManager::DoMessage(cIGZMessage2*)"},
    {0x1000f0fd2u, 412u, "cSC4PedestrianManager::HandleCrowdReactionEvent(cISC4Occupant*, unsigned int, unsigned int)"},
    {0x1000f121cu, 332u, "cSC4PedestrianManager::HandleNewBuilding(cISC4BuildingOccupant*)"},
    {0x1000f1368u, 130u, "cSC4PedestrianManager::AddAutomaton(cISC4Automaton*)"},
    {0x1000f1434u, 50u, "cSC4PedestrianManager::HandleTuningParametersChanged()"},
    {0x1000f1466u, 166u, "cSC4PedestrianManager::HandleRenderPropertiesChanged()"},
    {0x1000f1554u, 56u, "cSC4PedestrianManager::GetCommotionMultiplier(cISC4NetworkOccupant*, cISC4AutomataManager::PopulateLayer)"},

    // cSC4TutorialRegistry (10 entry points)
    {0x10064251cu, 2228u, "cSC4TutorialRegistry::Init()"},
    {0x10064307au, 110u, "cSC4TutorialRegistry::GetItem(int)"},
    {0x1006430e8u, 106u, "cSC4TutorialRegistry::Shutdown()"},
    {0x100643152u, 102u, "cSC4TutorialRegistry::SetLua(cISCLua*)"},
    {0x1006431b8u, 112u, "cSC4TutorialRegistry::GetRegionDirNames(std::__1::list<cRZString, std::__1::allocator<cRZString>>&)"},
    {0x100643228u, 454u, "cSC4TutorialRegistry::GetItems(std::__1::list<cSC4TutorialRegistry::cItem, std::__1::allocator<cSC4TutorialRegistry::cItem>>&, cISC4Region*)"},
    {0x1006433eeu, 142u, "cSC4TutorialRegistry::SetActive(int, bool)"},
    {0x10064347cu, 382u, "cSC4TutorialRegistry::GetNextActive(cISC4Region*)"},
    {0x1006435fau, 434u, "cSC4TutorialRegistry::GetRegionalCity(int)"},
    {0x1006437acu, 200u, "cSC4TutorialRegistry::GetTutorialRegionDefaultDirName(int)"},

    // cSC4CellTextureModel (9 entry points)
    {0x1000b5146u, 268u, "cSC4CellTextureModel::cSC4CellTextureModel()"},
    {0x1000b5252u, 86u, "cSC4CellTextureModel::Init(cISC4LotBaseTextureOccupant::LotBaseTextureSpecification const&)"},
    {0x1000b54f4u, 682u, "cSC4CellTextureModel::Draw(SC4DrawContext*, bool)"},
    {0x1000b579eu, 72u, "cSC4CellTextureModel::DoZoomChange(int)"},
    {0x1000b583au, 302u, "cSC4CellTextureModel::SetupPackedRenderState()"},
    {0x1000b5968u, 44u, "cSC4CellTextureModel::UpdateTextures(int, cISC4LotBaseTextureOccupant::LotBaseTextureSpecification const&)"},
    {0x1000b5994u, 234u, "cSC4CellTextureModel::LoadTextures(int)"},
    {0x1000b5a7eu, 226u, "cSC4CellTextureModel::UpdateTextureRotation()"},
    {0x1000b5b60u, 870u, "cSC4CellTextureModel::SetupVertices(bool, bool, cISTETerrain*, cISC4LightingManager*)"},

    // cSC4LotRetainingWallModel (9 entry points)
    {0x1001671acu, 216u, "cSC4LotRetainingWallModel::cSC4LotRetainingWallModel()"},
    {0x100167510u, 282u, "cSC4LotRetainingWallModel::Draw(SC4DrawContext*, bool)"},
    {0x10016762au, 80u, "cSC4LotRetainingWallModel::DoZoomChange(int, int, SC4DrawContext*)"},
    {0x100167692u, 102u, "cSC4LotRetainingWallModel::SetupPackedRenderState()"},
    {0x1001676f8u, 242u, "cSC4LotRetainingWallModel::SetupTextures(int, int)"},
    {0x1001677eau, 3662u, "cSC4LotRetainingWallModel::SetupVertices(bool, bool, bool&, cISC4LotManager*, cISTETerrain*, cISC4LightingManager*)"},
    {0x100168638u, 156u, "cSC4LotRetainingWallModel::SegmentIsAdjacentToFlatLot(int, int, cISC4LotManager*, cISTETerrain*)"},
    {0x1001686d4u, 86u, "cSC4LotRetainingWallModel::Init(cS3DBoundingBox const&, int, bool&)"},
    {0x10016872au, 222u, "cSC4LotRetainingWallModel::SetupVerticesFromBBoxAndWallEdge(cS3DBoundingBox const&, int, SC4Point<int>&, SC4Point<int>&)"},

    // cSC4DBSegment (9 entry points)
    {0x1003cf318u, 340u, "cSC4DBSegment::cSC4DBSegment()"},
    {0x1003cf602u, 200u, "cSC4DBSegment::OpenIStream(cGZPersistResourceKey const&, cISC4DBSegmentIStream**)"},
    {0x1003cf6cau, 18u, "cSC4DBSegment::CloseIStream(cISC4DBSegmentIStream*)"},
    {0x1003cf6fau, 216u, "cSC4DBSegment::OpenOStream(cGZPersistResourceKey const&, cISC4DBSegmentOStream**, bool)"},
    {0x1003cf7d2u, 18u, "cSC4DBSegment::CloseOStream(cISC4DBSegmentOStream*)"},
    {0x1003cf8e0u, 94u, "cSC4DBSegment::Shutdown()"},
    {0x1003cf95au, 110u, "cSC4DBSegment::Open(bool, bool)"},
    {0x1003cfa1au, 92u, "cSC4DBSegment::Close()"},
    {0x1003cfaa0u, 208u, "cSC4DBSegment::SetPercentageCompletion(float)"},

    // cSC4PropOccupantBurntBuilding (8 entry points)
    {0x100161760u, 294u, "cSC4PropOccupantBurntBuilding::cSC4PropOccupantBurntBuilding()"},
    {0x100161b86u, 380u, "cSC4PropOccupantBurntBuilding::Init()"},
    {0x100161d02u, 278u, "cSC4PropOccupantBurntBuilding::Rebuild()"},
    {0x100161e2au, 144u, "cSC4PropOccupantBurntBuilding::Shutdown()"},
    {0x100161fd8u, 144u, "cSC4PropOccupantBurntBuilding::SetAllFlags(unsigned int)"},
    {0x100162076u, 68u, "cSC4PropOccupantBurntBuilding::SetBoundsFromOccupant(cISC4Occupant*)"},
    {0x1001620bau, 60u, "cSC4PropOccupantBurntBuilding::GetWorldBounds(cS3DBoundingBox&)"},
    {0x10016216eu, 552u, "cSC4PropOccupantBurntBuilding::Draw(SC4DrawContext*, bool)"},

    // cSC4FoundationModel (8 entry points)
    {0x10035f09cu, 256u, "cSC4FoundationModel::cSC4FoundationModel()"},
    {0x10035f2e2u, 56u, "cSC4FoundationModel::GetSortValue(unsigned int&) const"},
    {0x10035f31au, 380u, "cSC4FoundationModel::Draw(SC4DrawContext*, bool)"},
    {0x10035f496u, 108u, "cSC4FoundationModel::DoZoomChange(int, int, SC4DrawContext*)"},
    {0x10035f502u, 102u, "cSC4FoundationModel::SetupPackedRenderState()"},
    {0x10035f568u, 296u, "cSC4FoundationModel::SetupTextures(int, int)"},
    {0x10035f690u, 1442u, "cSC4FoundationModel::SetupVertices(bool, bool, cISC4LightingManager*)"},
    {0x10035fc5cu, 534u, "cSC4FoundationModel::AdjustHeightToTerrain(bool)"},

    // cSC4Aircraft (8 entry points)
    {0x10050190au, 160u, "cSC4Aircraft::cSC4Aircraft()"},
    {0x1005019b4u, 50u, "cSC4Aircraft::Init()"},
    {0x100501afcu, 246u, "cSC4Aircraft::PlaceSelf()"},
    {0x100501dc2u, 116u, "cSC4Aircraft::HandleEnterState(tAutomataState*)"},
    {0x100501e52u, 38u, "cSC4Aircraft::DoAnimationTick(unsigned int, float)"},
    {0x100501e78u, 544u, "cSC4Aircraft::DoPlayerDriveTick(float)"},
    {0x100502098u, 200u, "cSC4Aircraft::GetAltitude(cS3DVector3 const&) const"},
    {0x100502160u, 228u, "cSC4Aircraft::ClipToWorld(cS3DVector3&) const"},

    // cSC4MySimInputControlChangeJob (8 entry points)
    {0x1006b612cu, 68u, "cSC4MySimInputControlChangeJob::cSC4MySimInputControlChangeJob()"},
    {0x1006b62a2u, 34u, "cSC4MySimInputControlChangeJob::Init()"},
    {0x1006b62c4u, 1098u, "cSC4MySimInputControlChangeJob::Init(cSC4MySim*)"},
    {0x1006b670eu, 356u, "cSC4MySimInputControlChangeJob::Shutdown()"},
    {0x1006b687au, 2964u, "cSC4MySimInputControlChangeJob::OnMouseMove(int, int, unsigned int)"},
    {0x1006b740eu, 78u, "cSC4MySimInputControlChangeJob::OnMouseDownL(int, int, unsigned int)"},
    {0x1006b745cu, 26u, "cSC4MySimInputControlChangeJob::OnKeyDown(unsigned int, unsigned int)"},
    {0x1006b7476u, 72u, "cSC4MySimInputControlChangeJob::OnMouseExit()"},

    // cSC4AdvisorPopupTracker (7 entry points)
    {0x10032f2acu, 72u, "cSC4AdvisorPopupTracker::cSC4AdvisorPopupTracker()"},
    {0x10032f38cu, 212u, "cSC4AdvisorPopupTracker::Init()"},
    {0x10032f460u, 96u, "cSC4AdvisorPopupTracker::Shutdown()"},
    {0x10032f550u, 1196u, "cSC4AdvisorPopupTracker::DoMessage(cIGZMessage2*)"},
    {0x10032fa3cu, 1974u, "cSC4AdvisorPopupTracker::SetupDialog(cIGZWin*, cISC4Advice*)"},
    {0x1003301f2u, 348u, "cSC4AdvisorPopupTracker::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x10033034eu, 124u, "cSC4AdvisorPopupTracker::EndDialog(cIGZWin*)"},

    // cSC4AdvisorUtility (7 entry points)
    {0x10043ed6cu, 2080u, "cSC4AdvisorUtility::update_occupant_variables()"},
    {0x10043f58cu, 1176u, "cSC4AdvisorUtility::update_neighbor_deal_variables()"},
    {0x10043fa78u, 1950u, "cSC4AdvisorUtility::update_variables()"},
    {0x100440216u, 584u, "cSC4AdvisorUtility::DoMessage(cIGZMessage2*)"},
    {0x10044046cu, 246u, "cSC4AdvisorUtility::Init(int)"},
    {0x100440846u, 166u, "cSC4AdvisorUtility::Shutdown()"},
    {0x1004408feu, 2172u, "cSC4AdvisorUtility::simulate_day_events()"},

    // cSC4DisasterManager (7 entry points)
    {0x10046349eu, 166u, "cSC4DisasterManager::AddInstanceToList(cISC4DisasterInstance*)"},
    {0x100463578u, 120u, "cSC4DisasterManager::cSC4DisasterManager()"},
    {0x10046369cu, 110u, "cSC4DisasterManager::DoMessage(cIGZMessage2*)"},
    {0x100463718u, 80u, "cSC4DisasterManager::GetActiveInstanceCount() const"},
    {0x1004637e4u, 104u, "cSC4DisasterManager::ProcessTempAddRemoveLists()"},
    {0x1004638cau, 158u, "cSC4DisasterManager::RemoveInstanceFromList(cISC4DisasterInstance*)"},
    {0x100463976u, 106u, "cSC4DisasterManager::Shutdown()"},

    // cSC4ViewInputControlCitySituations (7 entry points)
    {0x1006c813cu, 70u, "cSC4ViewInputControlCitySituations::cSC4ViewInputControlCitySituations()"},
    {0x1006c81acu, 204u, "cSC4ViewInputControlCitySituations::Init()"},
    {0x1006c8278u, 108u, "cSC4ViewInputControlCitySituations::Shutdown()"},
    {0x1006c82e4u, 110u, "cSC4ViewInputControlCitySituations::DoMessage(cIGZMessage2*)"},
    {0x1006c8374u, 150u, "cSC4ViewInputControlCitySituations::OnMouseDownL(int, int, unsigned int)"},
    {0x1006c840au, 142u, "cSC4ViewInputControlCitySituations::OnMouseMove(int, int, unsigned int)"},
    {0x1006c8498u, 180u, "cSC4ViewInputControlCitySituations::OnMouseUpL(int, int, unsigned int)"},

    // cSC4NetworkWorldCache (7 entry points)
    {0x10079d6feu, 50u, "cSC4NetworkWorldCache::cSC4NetworkWorldCache()"},
    {0x10079d774u, 48u, "cSC4NetworkWorldCache::GetCell(unsigned int)"},
    {0x10079d7a4u, 28u, "cSC4NetworkWorldCache::CreateCell(unsigned int)"},
    {0x10079d868u, 32u, "cSC4NetworkWorldCache::Clear()"},
    {0x10079d888u, 684u, "cSC4NetworkWorldCache::Push()"},
    {0x10079db34u, 650u, "cSC4NetworkWorldCache::Pop()"},
    {0x10079ddbeu, 264u, "cSC4NetworkWorldCache::GetAllCells(std::__1::vector<cSC4NetworkCellInfo, std::__1::allocator<cSC4NetworkCellInfo>>&)"},

    // cSC4DisasterManagerPipeBurst (6 entry points)
    {0x1004550ecu, 402u, "cSC4DisasterManagerPipeBurst::AttemptDisasterStart()"},
    {0x10045527eu, 252u, "cSC4DisasterManagerPipeBurst::CreateDisasterInstance(cISC4Occupant*)"},
    {0x100455388u, 84u, "cSC4DisasterManagerPipeBurst::AttemptDisasterStart(int, int)"},
    {0x1004556e0u, 12u, "cSC4DisasterManagerPipeBurst::GetID() const"},
    {0x1004556f8u, 172u, "cSC4DisasterManagerPipeBurst::Init()"},
    {0x1004557b6u, 88u, "cSC4DisasterManagerPipeBurst::Shutdown()"},

    // SC4TrainJunctionView (6 entry points)
    {0x1006f7ac6u, 848u, "SC4TrainJunctionView::Init()"},
    {0x1006f8572u, 244u, "SC4TrainJunctionView::Shutdown()"},
    {0x1006faf6au, 382u, "SC4TrainJunctionView::Update(SC4DrawContext*)"},
    {0x1006fb0e8u, 1400u, "SC4TrainJunctionView::DrawTrainJunction(SC4DrawContext*, int, int, cISC4NetworkOccupant*, unsigned int, std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&, unsigned short)"},
    {0x1006fb660u, 300u, "SC4TrainJunctionView::GetPathBitmap(unsigned char, unsigned char, cISC4PathInfo const*, std::__1::vector<cS3DVector3, std::__1::allocator<cS3DVector3>> const*, bool)"},
    {0x1006fb78cu, 358u, "SC4TrainJunctionView::GetArrowBitmapQuad(sVF_V3F_T2F*, cS3DVector3 const&, unsigned char, unsigned char, cISC4PathInfo const*, std::__1::vector<cS3DVector3, std::__1::allocator<cS3DVector3>> const*, unsigned int)"},

    // cSC4NeighborConnection (5 entry points)
    {0x10007146eu, 166u, "cSC4NeighborConnection::cSC4NeighborConnection()"},
    {0x100074b9au, 68u, "cSC4NeighborConnection::GetCityOnOtherSideOfConnection() const"},
    {0x100074bdeu, 182u, "cSC4NeighborConnection::GetConnectionInnerCell(int&, int&) const"},
    {0x100074cb2u, 944u, "cSC4NeighborConnection::UpdateTrafficVolume()"},
    {0x100075f16u, 18u, "cSC4NeighborConnection::GetIsActive() const"},

    // cSC4PipeOccupant (5 entry points)
    {0x1001a0ec8u, 136u, "cSC4PipeOccupant::cSC4PipeOccupant()"},
    {0x1001a102eu, 60u, "cSC4PipeOccupant::Init()"},
    {0x1001a106au, 306u, "cSC4PipeOccupant::StaticInit()"},
    {0x1001a11aeu, 82u, "cSC4PipeOccupant::Init(float const (*) [3])"},
    {0x1001a13a0u, 48u, "cSC4PipeOccupant::GetSortValue(unsigned int&) const"},

    // cSC4MissingPluginDataReporter (5 entry points)
    {0x10028bcb6u, 54u, "cSC4MissingPluginDataReporter::cSC4MissingPluginDataReporter()"},
    {0x10028bd40u, 108u, "cSC4MissingPluginDataReporter::Shutdown()"},
    {0x10028bde4u, 166u, "cSC4MissingPluginDataReporter::Init()"},
    {0x10028bf02u, 42u, "cSC4MissingPluginDataReporter::DoMessage(cIGZMessage2*)"},
    {0x10028bf2cu, 882u, "cSC4MissingPluginDataReporter::DoPostCityInit()"},

    // cSC4AdvisorHQ (5 entry points)
    {0x100445ab8u, 4852u, "cSC4AdvisorHQ::update_occupant_variables()"},
    {0x100446dacu, 1204u, "cSC4AdvisorHQ::update_variables()"},
    {0x100447260u, 166u, "cSC4AdvisorHQ::DoMessage(cIGZMessage2*)"},
    {0x100447314u, 396u, "cSC4AdvisorHQ::Init(int)"},
    {0x1004476ccu, 170u, "cSC4AdvisorHQ::Shutdown()"},

    // cSC4PathInfoFactory (5 entry points)
    {0x10045ce24u, 226u, "cSC4PathInfoFactory::CreateInstance(unsigned int, unsigned int, void**)"},
    {0x10045cf06u, 264u, "cSC4PathInfoFactory::ReadString(cIGZPersistDBSerialRecord&, cRZString&, unsigned int&)"},
    {0x10045d00eu, 136u, "cSC4PathInfoFactory::ReadUint32(cIGZPersistDBSerialRecord&, unsigned int&, unsigned int&)"},
    {0x10045d096u, 424u, "cSC4PathInfoFactory::ReadVector3(cIGZPersistDBSerialRecord&, cS3DVector3&, unsigned int&)"},
    {0x10045d23eu, 1836u, "cSC4PathInfoFactory::SerialRead(cIGZPersistResource&, cIGZPersistDBSerialRecord&)"},

    // cSC4CoverageRadiusEffectHelper (5 entry points)
    {0x100770f80u, 72u, "cSC4CoverageRadiusEffectHelper::SetCategories(unsigned int)"},
    {0x100770fd2u, 940u, "cSC4CoverageRadiusEffectHelper::RecalcCoverage()"},
    {0x10077137eu, 68u, "cSC4CoverageRadiusEffectHelper::Shutdown()"},
    {0x1007713c2u, 570u, "cSC4CoverageRadiusEffectHelper::UpdateEffectForOccupant(cISC4Occupant*, float, char const*)"},
    {0x10077167cu, 168u, "cSC4CoverageRadiusEffectHelper::GetCategoriesForOccupant(cISC4Occupant*)"},

    // cSC4AudioCOMDirector (4 entry points)
    {0x1003de520u, 2134u, "cSC4AudioCOMDirector::cSC4AudioCOMDirector()"},
    {0x1003e0108u, 178u, "cSC4AudioCOMDirector::OnStart(cIGZCOM*)"},
    {0x1003e01bau, 1246u, "cSC4AudioCOMDirector::PostAppInit()"},
    {0x1003e06a6u, 484u, "cSC4AudioCOMDirector::PreAppShutdown()"},

    // cSC4SlotArray<cSTEOverlayManager (4 entry points)
    {0x100727cacu, 128u, "cSC4SlotArray<cSTEOverlayManager::cOverlay>::Clear()"},
    {0x1007285fcu, 176u, "cSC4SlotArray<cSTEOverlayManager::cOverlay>::AddSlot(cSTEOverlayManager::cOverlay const&)"},
    {0x100728e24u, 270u, "cSC4SlotArray<cSTEOverlayManager::cDecal>::AddSlot(cSTEOverlayManager::cDecal const&)"},
    {0x100729170u, 222u, "cSC4SlotArray<cSTEOverlayManager::cRingDecal>::AddSlot(cSTEOverlayManager::cRingDecal const&)"},

    // cSC4PropPlacementPropertiesFactory (3 entry points)
    {0x10016305cu, 160u, "cSC4PropPlacementPropertiesFactory::CreateInstance(unsigned int, unsigned int, void**)"},
    {0x1001630fcu, 202u, "cSC4PropPlacementPropertiesFactory::SerialRead(cIGZPersistResource&, cIGZPersistDBSerialRecord&)"},
    {0x1001631c6u, 216u, "cSC4PropPlacementPropertiesFactory::SerialWrite(cIGZPersistResource&, cIGZPersistDBSerialRecord&)"},

    // cSC4CityDetailManager (3 entry points)
    {0x1001973d0u, 66u, "cSC4CityDetailManager::Init()"},
    {0x100197412u, 1154u, "cSC4CityDetailManager::ReduceCityDetail()"},
    {0x100197894u, 1482u, "cSC4CityDetailManager::IncreaseCityDetail()"},

    // cSC4NetworkORFilter (3 entry points)
    {0x1002eee0au, 66u, "cSC4NetworkORFilter::cSC4NetworkORFilter()"},
    {0x1002eee4cu, 60u, "cSC4NetworkORFilter::cSC4NetworkORFilter(unsigned int)"},
    {0x1002eee88u, 62u, "cSC4NetworkORFilter::IsOccupantIncluded(cISC4Occupant*)"},

    // cSC4AdviceSubject (3 entry points)
    {0x10044bb32u, 150u, "cSC4AdviceSubject::validate_location()"},
    {0x10044bc50u, 136u, "cSC4AdviceSubject::SetObject(cIGZUnknown*)"},
    {0x10044bd4eu, 342u, "cSC4AdviceSubject::Clone(cISC4AdviceSubject**)"},

    // cSC4MassTransitTrain (3 entry points)
    {0x1006f2b74u, 132u, "cSC4MassTransitTrain::cSC4MassTransitTrain()"},
    {0x1006f2c28u, 336u, "cSC4MassTransitTrain::ChooseNewDestination()"},
    {0x1006f2d78u, 240u, "cSC4MassTransitTrain::HandleExitNetwork(cISC4NetworkOccupant*, unsigned int, cISC4Lot*, SC4Rect<int> const&)"},

    // cSC4AutoFlushInputMessageQueue (2 entry points)
    {0x100038ed2u, 70u, "cSC4AutoFlushInputMessageQueue::cSC4AutoFlushInputMessageQueue(unsigned int)"},
    {0x100038f60u, 66u, "cSC4AutoFlushInputMessageQueue::Flush()"},

    // cSC4OccupantTypesFilterHashed (2 entry points)
    {0x1000a910eu, 38u, "cSC4OccupantTypesFilterHashed::IsOccupantTypeIncluded(unsigned int)"},
    {0x10030d52cu, 558u, "cSC4OccupantTypesFilterHashed::AddOccupantType(unsigned int)"},

    // cSC4SafeList<cISC4Lot*> (2 entry points)
    {0x10012c0a6u, 124u, "cSC4SafeList<cISC4Lot*>::Remove(cISC4Lot* const&)"},
    {0x100133cc0u, 102u, "cSC4SafeList<cISC4Lot*>::GetIterator()"},

    // cSC4FileTokenizer (2 entry points)
    {0x1001aeca2u, 374u, "cSC4FileTokenizer::GetToken()"},
    {0x1001b0502u, 108u, "cSC4FileTokenizer::Init(cGZPersistResourceKey const&)"},

    // cSC4ScriptServiceResourceFactory (2 entry points)
    {0x100306406u, 118u, "cSC4ScriptServiceResourceFactory::GZUIScriptLoadBuffer(cGZPersistResourceKey const&)"},
    {0x10030647cu, 106u, "cSC4ScriptServiceResourceFactory::GZUIScriptLoadString(cGZPersistResourceKey const&)"},

    // cAmbienceOccupantFilterIsDistressed (2 entry points)
    {0x1003df1a0u, 50u, "cAmbienceOccupantFilterIsDistressed::Create()"},
    {0x1003e4ac6u, 258u, "cAmbienceOccupantFilterIsDistressed::IsTrueOccupant(cISC4Occupant*)"},

    // cAmbienceOccupantFilterIsTraffic (2 entry points)
    {0x1003df268u, 50u, "cAmbienceOccupantFilterIsTraffic::Create()"},
    {0x1003e56e0u, 100u, "cAmbienceOccupantFilterIsTraffic::IsTrueOccupant(cISC4Occupant*)"},

    // cAmbienceOccupantFilterIsEmergencyVehicle (2 entry points)
    {0x1003df330u, 50u, "cAmbienceOccupantFilterIsEmergencyVehicle::Create()"},
    {0x1003e5256u, 122u, "cAmbienceOccupantFilterIsEmergencyVehicle::IsTrueOccupant(cISC4Occupant*)"},

    // cAmbienceOccupantFilterIsRichVehicle (2 entry points)
    {0x1003df3f8u, 50u, "cAmbienceOccupantFilterIsRichVehicle::Create()"},
    {0x1003e541au, 76u, "cAmbienceOccupantFilterIsRichVehicle::IsTrueOccupant(cISC4Occupant*)"},

    // cAmbienceOccupantFilterIsTrain (2 entry points)
    {0x1003df4c0u, 50u, "cAmbienceOccupantFilterIsTrain::Create()"},
    {0x1003e554au, 76u, "cAmbienceOccupantFilterIsTrain::IsTrueOccupant(cISC4Occupant*)"},

    // cAmbienceOccupantFilterIsMySim (2 entry points)
    {0x1003df588u, 50u, "cAmbienceOccupantFilterIsMySim::Create()"},
    {0x1003e5694u, 76u, "cAmbienceOccupantFilterIsMySim::IsTrueOccupant(cISC4Occupant*)"},

    // cAmbienceOccupantFilterIsOnFire (2 entry points)
    {0x1003df650u, 50u, "cAmbienceOccupantFilterIsOnFire::Create()"},
    {0x1003e595au, 80u, "cAmbienceOccupantFilterIsOnFire::IsTrueOccupant(cISC4Occupant*)"},

    // cAmbienceOccupantFilterIsMediumWealth (2 entry points)
    {0x1003df718u, 50u, "cAmbienceOccupantFilterIsMediumWealth::Create()"},
    {0x1003e5a4cu, 162u, "cAmbienceOccupantFilterIsMediumWealth::IsTrueOccupant(cISC4Occupant*)"},

    // cSC4AudioAmbienceManagerOccupantFilter (2 entry points)
    {0x1003ec5a6u, 40u, "cSC4AudioAmbienceManagerOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    {0x1003ec5ceu, 86u, "cSC4AudioAmbienceManagerOccupantFilter::IsOccupantTypeIncluded(unsigned int)"},

    // cSC4AdvisorFinance (2 entry points)
    {0x100447a8cu, 892u, "cSC4AdvisorFinance::update_variables()"},
    {0x100447e08u, 152u, "cSC4AdvisorFinance::DoMessage(cIGZMessage2*)"},

    // cSC4CarjackedVehicle (2 entry points)
    {0x1006d6864u, 132u, "cSC4CarjackedVehicle::cSC4CarjackedVehicle()"},
    {0x1006d68e8u, 98u, "cSC4CarjackedVehicle::PostInit()"},

    // cSC4PolygonClipperCommon (2 entry points)
    {0x100717428u, 62u, "cSC4PolygonClipperCommon::Interpolate(sVF_V3F_T2F const&, sVF_V3F_T2F const&, float)"},
    {0x100718ba8u, 312u, "cSC4PolygonClipperCommon::Interpolate(sVF_V3F_C4UB_T2F const&, sVF_V3F_C4UB_T2F const&, float)"},

    // nsSC4TutorialSystem (1 entry points)
    {0x10006008eu, 82u, "nsSC4TutorialSystem::FilterUICommandMessage(cGZMessage&)"},

    // cSC4ViewLevelOccupantFilter (1 entry points)
    {0x1000a6ddau, 30u, "cSC4ViewLevelOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},

    // cSC4DemolitionOccupantFilter (1 entry points)
    {0x1000ee662u, 582u, "cSC4DemolitionOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},

    // tAutomataStatePlayerDrive (1 entry points)
    {0x10010e63cu, 60u, "tAutomataStatePlayerDrive::Shutdown(cISC4Automaton*)"},

    // cZoneDeveloperLotFilter (1 entry points)
    {0x100133be4u, 96u, "cZoneDeveloperLotFilter::IsLotConfigurationIncluded(cISC4LotConfiguration*)"},

    // cSC4NetworkBridgeTypeInfo (1 entry points)
    {0x1001ce07eu, 1282u, "cSC4NetworkBridgeTypeInfo::cSC4NetworkBridgeTypeInfo(cSC4NetworkBridgeTypeInfo const&)"},

    // cSC4HTMLLinkHandler (1 entry points)
    {0x1001f22b6u, 682u, "cSC4HTMLLinkHandler::DoURL(cIGZString const&, void*)"},

    // cTrafficNetworkFilter_NetworkMap (1 entry points)
    {0x1002a073cu, 86u, "cTrafficNetworkFilter_NetworkMap::IsOccupantIncluded(cISC4Occupant*)"},

    // cSC4TrafficQueryOccupantFilter (1 entry points)
    {0x1002dc006u, 358u, "cSC4TrafficQueryOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},

    // cSC4CatalogItem (1 entry points)
    {0x1002fcdfcu, 86u, "cSC4CatalogItem::cSC4CatalogItem()"},

    // cSC4LotConfigurationFilterByBuildingType (1 entry points)
    {0x1003c1f3cu, 30u, "cSC4LotConfigurationFilterByBuildingType::IsLotConfigurationIncluded(cISC4LotConfiguration*)"},

    // cLotConfigurationEditorWinFileBrowser (1 entry points)
    {0x10042ef4cu, 172u, "cLotConfigurationEditorWinFileBrowser::SendMsg(cIGZWin*, cGZMessage&)"},

    // cSC4ActiveFireOccupantFilter (1 entry points)
    {0x10045e770u, 92u, "cSC4ActiveFireOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},

    // cSC4DummyAutoOccupant (1 entry points)
    {0x1005f9ebeu, 54u, "cSC4DummyAutoOccupant::GetBoundingBox(float*, float*)"},

    // cSC4DecalFilter (1 entry points)
    {0x1006c99d4u, 116u, "cSC4DecalFilter::IsOccupantIncluded(cISC4Occupant*)"},

    // cSC4OccupantFilterByGroupAndDistance (1 entry points)
    {0x1006e6164u, 122u, "cSC4OccupantFilterByGroupAndDistance::IsOccupantIncluded(cISC4Occupant*)"},

    // cSC4PackedRenderState (1 entry points)
    {0x100771d00u, 394u, "cSC4PackedRenderState::PackRenderState(bool, bool, bool, bool, bool, int, bool, int, int, int, int, float, int, int)"},
}};

constexpr std::size_t Wave3Part2MethodCount() noexcept {
    return kWave3Part2Methods.size();
}

constexpr std::size_t Wave3Part2NamedCount() noexcept {
    std::size_t count = 0;
    for (const Wave3Part2Method& method : kWave3Part2Methods) {
        if (method.name != nullptr && method.address != 0) {
            ++count;
        }
    }
    return count;
}

constexpr bool Wave3Part2AddressesUnique() noexcept {
    for (std::size_t i = 0; i < kWave3Part2Methods.size(); ++i) {
        for (std::size_t j = i + 1; j < kWave3Part2Methods.size(); ++j) {
            if (kWave3Part2Methods[i].address == kWave3Part2Methods[j].address) {
                return false;
            }
        }
    }
    return true;
}

constexpr const Wave3Part2Method* FindWave3Part2Method(Uint64 address) noexcept {
    for (const Wave3Part2Method& method : kWave3Part2Methods) {
        if (method.address == address) {
            return &method;
        }
    }
    return nullptr;
}

static_assert(Wave3Part2MethodCount() == 452u);
static_assert(Wave3Part2NamedCount() == 452u);
static_assert(Wave3Part2AddressesUnique());
static_assert(FindWave3Part2Method(0x100417602u) != nullptr);
static_assert(FindWave3Part2Method(0x100771d00u) != nullptr);
static_assert(FindWave3Part2Method(0x100417602u)->sizeBytes == 162u);
static_assert(FindWave3Part2Method(0x100771d00u)->sizeBytes == 394u);

// ---------------------------------------------------------------------------
// Wave 4 Part 2 method index
// Eighty-four classes and every 0x100xxxxxx entry point from the Wave 4
// Part 2 workpack. Addresses and sizes are the macOS x86_64 SimCity 4
// Deluxe leaves recorded in kWave4Part2Methods.
// ---------------------------------------------------------------------------

struct Wave4Part2Method {
    Uint64 address;
    Uint32 sizeBytes;
    const char* name;
};

inline constexpr std::array<Wave4Part2Method, 808> kWave4Part2Methods = {{
    // _GLOBAL__N_1 (179 entry points)
    {0x100019768u, 60u, "(anonymous namespace)::cStackString<char, 1024u>::append(char const&)"},
    {0x1000197a4u, 100u, "(anonymous namespace)::cStackString<char, 1024u>::append(unsigned int, char)"},
    {0x100019808u, 96u, "(anonymous namespace)::cStackString<char, 1024u>::append(char const*, unsigned int)"},
    {0x100019a36u, 118u, "(anonymous namespace)::cStackString<char, 1024u>::reserve(unsigned int)"},
    {0x100132a04u, 288u, "(anonymous namespace)::DecodeNetworkConnectionObject(cISC4LotConfiguration*, cSC4LotConfigurationObject const&, SC4Rect<int> const&, int, int&, int&, unsigned int&, unsigned int&, cISC4NetworkManager::eNetworkType&)"},
    {0x10016ee80u, 211u, "(anonymous namespace)::AddTextureRemapEntryCallback(cRZString const&, cRZString const&, void*)"},
    {0x10016ef53u, 211u, "(anonymous namespace)::AddShadowRemapEntryCallback(cRZString const&, cRZString const&, void*)"},
    {0x10016f026u, 274u, "(anonymous namespace)::AddShadowRemapByEdgesEntryCallback(cRZString const&, cRZString const&, void*)"},
    {0x1001c66f6u, 358u, "(anonymous namespace)::ReadIntersectionRule(cRZString const&, cRZString const&, void*)"},
    {0x1001c6fbdu, 283u, "(anonymous namespace)::LoadOverrideCallback(cRZString const&, cRZString const&, void*)"},
    {0x1001c7e1bu, 581u, "(anonymous namespace)::AddRuleOverrides(cRZString const&, cRZString const&, void*)"},
    {0x1001c8177u, 979u, "(anonymous namespace)::LoadVerifiersCallback(cRZString const&, cRZString const&, void*)"},
    {0x1001c8b8bu, 1110u, "(anonymous namespace)::ReadIntersectionOrderingEntries(cRZString const&, cRZString const&, void*)"},
    {0x1001c8fe1u, 6351u, "(anonymous namespace)::ReadIntersectionEntries(cRZString const&, cRZString const&, void*)"},
    {0x1001cb5aau, 978u, "(anonymous namespace)::cParser::NextToken()"},
    {0x1001cc870u, 312u, "(anonymous namespace)::NetworkNameToID(cRZString const&)"},
    {0x1001ccbcau, 5042u, "(anonymous namespace)::ReadBridgeTypeInfo(cRZString const&, cRZString const&, void*)"},
    {0x1001cedcau, 202u, "(anonymous namespace)::cBridgeSpecializationParser::PushScope()"},
    {0x1001cf2beu, 3158u, "(anonymous namespace)::cBridgeSpecializationParser::ParseBridgeSpecialization(cSC4NetworkBridgeTypeInfo*, cSC4NetworkBridgeSpecialization&)"},
    {0x1001d0654u, 662u, "(anonymous namespace)::ReadSupportStructureInfo(cGZPersistResourceKey const&, void*)"},
    {0x10022c44eu, 296u, "(anonymous namespace)::TransposeMap(std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>&, int&, SC4Point<int>&, bool)"},
    {0x10022c8abu, 62u, "void (anonymous namespace)::RotatePoint<SC4Point<int>>(SC4Point<int>&, int)"},
    {0x10022c8e9u, 457u, "(anonymous namespace)::RotateMap(std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>&, int&, SC4Point<int>&, int, bool)"},
    {0x10023c642u, 1998u, "(anonymous namespace)::Resample(cIGZBuffer*, cIGZBuffer**, double, double)"},
    {0x10023d625u, 115u, "void (anonymous namespace)::ConvertKernelToInteger<float*, int*, int>(float*, float*, int*, int)"},
    {0x10023dbb4u, 67u, "(anonymous namespace)::cCityDrawingOrderSort::operator()(cISC4Region::cLocation const&, cISC4Region::cLocation const&) const"},
    {0x10028cfa9u, 243u, "(anonymous namespace)::MakeShortcut(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>&)"},
    {0x1003026c2u, 124u, "(anonymous namespace)::cExemplarReader::cExemplarReader(cIGZPersistDBSerialRecord&)"},
    {0x100305e2cu, 222u, "(anonymous namespace)::cExemplarReader::GetDataEntry(cRZString&, cRZString&)"},
    {0x100353458u, 242u, "(anonymous namespace)::PutAntialiasedPixel(cIGZBuffer*, int, int, unsigned int, unsigned int)"},
    {0x100360a47u, 270u, "void (anonymous namespace)::expand_2<unsigned short>(unsigned short*, long, unsigned int, unsigned short const*)"},
    {0x1003ca2deu, 538u, "void (anonymous namespace)::GenerateKernelArray<int*, double (*)(double)>(int, int, float, int*, double (*)(double))"},
    {0x10045b303u, 111u, "(anonymous namespace)::GetEdgeFromPoint(SC4Rect<float> const&, cS3DVector3 const&)"},
    {0x10045b372u, 271u, "(anonymous namespace)::ComputeLineToRectIntersection(SC4Rect<float> const&, unsigned int, cS3DVector3 const&, cS3DVector3 const&, unsigned char&)"},
    {0x10045b481u, 113u, "(anonymous namespace)::ComputePathLength(cISC4PathInfo::tPath*)"},
    {0x1004ecad3u, 171u, "(anonymous namespace)::MergeRectangle(std::__1::list<cRZRect, std::__1::allocator<cRZRect>>&, cRZRect const&, bool)"},
    {0x10052c894u, 396u, "(anonymous namespace)::FillWithTransparencyGrid(cIGZBuffer*, cRZRect const&, unsigned int)"},
    {0x10056f17cu, 320u, "(anonymous namespace)::cRuleCommand::RegisterCommands()"},
    {0x10056f2e6u, 600u, "(anonymous namespace)::cRuleCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4RuleParser*)"},
    {0x10056f53eu, 20u, "(anonymous namespace)::cRuleCommand::EndBlock(cSC4RuleParser*)"},
    {0x10056f59au, 234u, "(anonymous namespace)::cPropertyCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4RuleParser*)"},
    {0x10056f684u, 364u, "(anonymous namespace)::AddOptionToSet(cISC4RenderProperties*, cSC4PropertySet&, cRZString const&, cRZString const&)"},
    {0x10056fa84u, 250u, "(anonymous namespace)::cOptionCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4RuleParser*)"},
    {0x10056fb8eu, 206u, "(anonymous namespace)::cHasCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4RuleParser*)"},
    {0x10056fcf0u, 200u, "(anonymous namespace)::cHasNoCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4RuleParser*)"},
    {0x10056fe40u, 514u, "(anonymous namespace)::cStringMatchCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4RuleParser*)"},
    {0x100570340u, 514u, "(anonymous namespace)::cStringNotMatchCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4RuleParser*)"},
    {0x100570552u, 254u, "(anonymous namespace)::cAtLeastCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4RuleParser*)"},
    {0x100570836u, 264u, "(anonymous namespace)::cAtMostCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4RuleParser*)"},
    {0x100570a9au, 92u, "(anonymous namespace)::cOptionGroupCommand::RegisterCommands()"},
    {0x100570af6u, 212u, "(anonymous namespace)::cOptionGroupCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4RuleParser*)"},
    {0x100570beau, 204u, "(anonymous namespace)::cOptionGroupOptionCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4RuleParser*)"},
    {0x100570fdcu, 240u, "(anonymous namespace)::cOptionGroupPropertyCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4RuleParser*)"},
    {0x1005714e0u, 54u, "(anonymous namespace)::cVendorCommand::RegisterCommands()"},
    {0x100571516u, 294u, "(anonymous namespace)::cVendorCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4RuleParser*)"},
    {0x100571652u, 215u, "(anonymous namespace)::cCardCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4RuleParser*)"},
    {0x100574c5au, 478u, "(anonymous namespace)::cScrubberFilter::IsOccupantIncluded(cISC4Occupant*)"},
    {0x1005c6266u, 276u, "(anonymous namespace)::RotateBlt(cIGZBuffer*, unsigned int const*, int, unsigned int, int)"},
    {0x1005c722cu, 660u, "(anonymous namespace)::Downsample16(unsigned int*, long, unsigned int const*, long, unsigned int, unsigned int)"},
    {0x1005c74c0u, 260u, "(anonymous namespace)::Downsample4(unsigned int*, long, unsigned int const*, long, unsigned int, unsigned int)"},
    {0x1005c75c4u, 86u, "(anonymous namespace)::Downsample1(unsigned int*, long, unsigned int const*, long, unsigned int, unsigned int)"},
    {0x1005c761au, 119u, "(anonymous namespace)::Upsample4(unsigned int*, long, unsigned int const*, long, unsigned int, unsigned int)"},
    {0x1005c7691u, 179u, "(anonymous namespace)::Upsample16(unsigned int*, long, unsigned int const*, long, unsigned int, unsigned int)"},
    {0x1005cb3e2u, 332u, "(anonymous namespace)::cRenderPropCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cb53eu, 266u, "(anonymous namespace)::cTextureIDCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cb8e4u, 266u, "(anonymous namespace)::cModelIDCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cb9feu, 266u, "(anonymous namespace)::cSoundIDCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cbb18u, 266u, "(anonymous namespace)::cBrushIDCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cbc32u, 312u, "(anonymous namespace)::cEffectIDCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cbddau, 54u, "(anonymous namespace)::cEffectGroupIDCommand::RegisterCommands()"},
    {0x1005cbe3au, 152u, "(anonymous namespace)::cEffectGroupIDCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cbee8u, 298u, "(anonymous namespace)::cEffectGroupIDInstanceCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cc022u, 1504u, "(anonymous namespace)::cTestEffectCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cc7e6u, 192u, "(anonymous namespace)::cMessageTriggerCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cc8b6u, 798u, "(anonymous namespace)::cCameraParamsCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005ccbe4u, 558u, "(anonymous namespace)::cEffectRemapCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cd062u, 448u, "(anonymous namespace)::cEffectsResourceCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cd222u, 437u, "(anonymous namespace)::cEffectsResourceCommand::EndBlock(cSC4EffectsParser*)"},
    {0x1005cd42eu, 258u, "(anonymous namespace)::cLoadResourceCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cd568u, 758u, "(anonymous namespace)::cGroupEffectCommand::RegisterCommands()"},
    {0x1005cd85eu, 1280u, "(anonymous namespace)::cGroupEffectCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cdd5eu, 182u, "(anonymous namespace)::cGroupEffectCommand::EndBlock(cSC4EffectsParser*)"},
    {0x1005cde24u, 982u, "(anonymous namespace)::cGroupParticlesCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005ce20au, 692u, "(anonymous namespace)::cGroupDynamicParticleCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005ce4ceu, 786u, "(anonymous namespace)::cGroupDecalCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005ce7f0u, 548u, "(anonymous namespace)::cGroupShakeCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005ceb1eu, 584u, "(anonymous namespace)::cGroupFlashCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005ced76u, 468u, "(anonymous namespace)::cGroupTintCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cef5au, 874u, "(anonymous namespace)::cGroupSoundCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cf42eu, 200u, "(anonymous namespace)::cGroupChainCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cf506u, 1466u, "(anonymous namespace)::cGroupBrushCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005cfad0u, 2006u, "(anonymous namespace)::cGroupScrubberCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d02b6u, 460u, "(anonymous namespace)::cGroupAttractorCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d0492u, 176u, "(anonymous namespace)::cGroupSequenceCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d0552u, 686u, "(anonymous namespace)::cGroupCameraCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d0810u, 786u, "(anonymous namespace)::cGroupGroupCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d0b5au, 212u, "(anonymous namespace)::cGroupSelectCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d0c2eu, 222u, "(anonymous namespace)::cGroupSelectCommand::EndBlock(cSC4EffectsParser*)"},
    {0x1005d0d44u, 174u, "(anonymous namespace)::cGroupSystemSequenceCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d13dau, 1022u, "(anonymous namespace)::cParticleEffectCommand::RegisterCommands()"},
    {0x1005d17d8u, 786u, "(anonymous namespace)::cParticleEffectCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d1aeau, 182u, "(anonymous namespace)::cParticleEffectCommand::EndBlock(cSC4EffectsParser*)"},
    {0x1005d1bb0u, 102u, "(anonymous namespace)::cParticleZoomCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d1c26u, 762u, "(anonymous namespace)::cParticleColorCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d1f30u, 582u, "(anonymous namespace)::cParticleAlphaCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d2186u, 360u, "(anonymous namespace)::cParticleSizeCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d22feu, 352u, "(anonymous namespace)::cParticleAspectCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d246eu, 418u, "(anonymous namespace)::cParticleRotateCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d2620u, 2316u, "(anonymous namespace)::cParticleSourceCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d2f3cu, 2376u, "(anonymous namespace)::cParticleEmitCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d3894u, 3562u, "(anonymous namespace)::cParticleForceCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d4850u, 1396u, "(anonymous namespace)::cParticleWarpCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d4f4cu, 746u, "(anonymous namespace)::cParticleRandomWalkCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d5246u, 120u, "(anonymous namespace)::cParticleStretchCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d52ceu, 232u, "(anonymous namespace)::cParticleLifeCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d53c6u, 313u, "(anonymous namespace)::cParticleRateCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d54ffu, 685u, "(anonymous namespace)::ParseRateCurveCommands(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d57bcu, 106u, "(anonymous namespace)::cParticleInjectCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d5836u, 240u, "(anonymous namespace)::cParticleMaintainCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d5936u, 440u, "(anonymous namespace)::cParticleTextureCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d5aeeu, 222u, "(anonymous namespace)::ParseDrawOptions(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4ParticlesDescription*)"},
    {0x1005d5bdcu, 1172u, "(anonymous namespace)::cParticleModelCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d6080u, 440u, "(anonymous namespace)::cParticleAlignmentCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d6248u, 666u, "(anonymous namespace)::cParticleCollisionCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d64f2u, 320u, "(anonymous namespace)::cParticleTerrainRepelCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d6642u, 200u, "(anonymous namespace)::cParticleEffectBaseCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d671au, 312u, "(anonymous namespace)::cParticleTimedEffectCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d6e66u, 168u, "(anonymous namespace)::cDynamicParticleEffectCommand::RegisterCommands()"},
    {0x1005d6f0eu, 864u, "(anonymous namespace)::cDynamicParticleEffectCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d726eu, 182u, "(anonymous namespace)::cDynamicParticleEffectCommand::EndBlock(cSC4EffectsParser*)"},
    {0x1005d7334u, 200u, "(anonymous namespace)::cDynamicParticleEffectBaseCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d740cu, 742u, "(anonymous namespace)::cDynamicParticleModelCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d7702u, 198u, "(anonymous namespace)::cDynamicParticleMassCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d77d8u, 314u, "(anonymous namespace)::cDynamicParticleFrictionCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d794au, 404u, "(anonymous namespace)::cDecalEffectCommand::RegisterCommands()"},
    {0x1005d7adeu, 1110u, "(anonymous namespace)::cDecalEffectCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d7f34u, 182u, "(anonymous namespace)::cDecalEffectCommand::EndBlock(cSC4EffectsParser*)"},
    {0x1005d7ffau, 480u, "(anonymous namespace)::cDecalColorCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d81eau, 582u, "(anonymous namespace)::cDecalAlphaCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d8440u, 388u, "(anonymous namespace)::cDecalSizeCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d85d4u, 196u, "(anonymous namespace)::cDecalAspectCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d86a8u, 352u, "(anonymous namespace)::cDecalRotateCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d8818u, 228u, "(anonymous namespace)::cDecalLifeCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d890cu, 710u, "(anonymous namespace)::cDecalTextureCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d8c0au, 206u, "(anonymous namespace)::cShakeEffectCommand::RegisterCommands()"},
    {0x1005d8cd8u, 958u, "(anonymous namespace)::cShakeEffectCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d9096u, 182u, "(anonymous namespace)::cShakeEffectCommand::EndBlock(cSC4EffectsParser*)"},
    {0x1005d915cu, 236u, "(anonymous namespace)::cShakeLengthCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d9258u, 196u, "(anonymous namespace)::cShakeAmplitudeCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d932cu, 196u, "(anonymous namespace)::cShakeFrequencyCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d9400u, 120u, "(anonymous namespace)::cShakeAspectCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d9488u, 126u, "(anonymous namespace)::cShakeBaseTableCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d953eu, 222u, "(anonymous namespace)::cLightEffectCommand::RegisterCommands()"},
    {0x1005d961cu, 766u, "(anonymous namespace)::cLightEffectCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d991au, 332u, "(anonymous namespace)::cLightEffectCommand::EndBlock(cSC4EffectsParser*)"},
    {0x1005d9a76u, 196u, "(anonymous namespace)::cLightStrengthCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d9b4au, 188u, "(anonymous namespace)::cLightLengthCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d9c16u, 480u, "(anonymous namespace)::cLightColorCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005d9e2eu, 92u, "(anonymous namespace)::cSequenceEffectCommand::RegisterCommands()"},
    {0x1005d9e8au, 444u, "(anonymous namespace)::cSequenceEffectCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005da046u, 214u, "(anonymous namespace)::cSequenceEffectCommand::EndBlock(cSC4EffectsParser*)"},
    {0x1005da12cu, 256u, "(anonymous namespace)::cSequenceWaitCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005da3dau, 286u, "(anonymous namespace)::cSequencePlayCommand::Parse(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectsParser*)"},
    {0x1005f9fa4u, 56u, "(anonymous namespace)::cDestroyBuildingFilter::IsOccupantIncluded(cISC4Occupant*)"},
    {0x1005fee71u, 134u, "(anonymous namespace)::ParseExpression(char const*, char const**)"},
    {0x1005ff8bbu, 172u, "(anonymous namespace)::EvalExpression(char const*&)"},
    {0x1005ff967u, 233u, "(anonymous namespace)::EvalTerm(char const*&)"},
    {0x1005ffa50u, 141u, "(anonymous namespace)::EvalFactor(char const*&)"},
    {0x1005ffaddu, 3068u, "(anonymous namespace)::EvalSignedNumber(char const*&)"},
    {0x1006006d9u, 57u, "(anonymous namespace)::EvalParentheses(char const*&)"},
    {0x100600712u, 91u, "(anonymous namespace)::EvalParentheses2(char const*&, double&, double&)"},
    {0x10060076du, 157u, "(anonymous namespace)::EvalCharConst(char const*&, char)"},
    {0x100601827u, 219u, "(anonymous namespace)::FindParentDir(char const*, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>*)"},
    {0x100656a82u, 124u, "(anonymous namespace)::cRectFloat32::Intersect((anonymous namespace)::cRectFloat32 const&, cRZRect const&)"},
    {0x100722b94u, 148u, "(anonymous namespace)::IsOneSegment(SC4Point<unsigned int>&, SC4Point<unsigned int>&)"},
    {0x10075e634u, 120u, "(anonymous namespace)::SetupScript(cIGZWin*, unsigned int, void*, void*)"},
    {0x10076529eu, 200u, "(anonymous namespace)::SC4ComputeViewportZProjection(float*, SC4ViewUtilities*)"},
    {0x100765366u, 278u, "(anonymous namespace)::cModelSort<(anonymous namespace)::cSortModelsFrontToBack>::sort(cSC4ModelSortElement*, int)"},
    {0x10076547cu, 278u, "(anonymous namespace)::cModelSort<(anonymous namespace)::cSortModelsBackToFront>::sort(cSC4ModelSortElement*, int)"},

    // cSC4ModelMaker (52 entry points)
    {0x10030c5feu, 398u, "cSC4ModelMaker::cSC4ModelMaker()"},
    {0x10030c958u, 1054u, "cSC4ModelMaker::Shutdown()"},
    {0x10030cd76u, 326u, "cSC4ModelMaker::Init()"},
    {0x10030cebcu, 574u, "cSC4ModelMaker::CreateOccupantFilters()"},
    {0x10030d0fau, 216u, "cSC4ModelMaker::DetermineLoadRadiusAroundVisibleArea()"},
    {0x10030d75au, 1850u, "cSC4ModelMaker::DoMessage(cIGZMessage2*)"},
    {0x10030de94u, 288u, "cSC4ModelMaker::DoOccupantCreated(cISC4Occupant*, bool, unsigned int)"},
    {0x10030dfb4u, 228u, "cSC4ModelMaker::DoOccupantDestroyed(cISC4Occupant*, bool, unsigned int)"},
    {0x10030e098u, 384u, "cSC4ModelMaker::DoOccupantChanged(cISC4Occupant*, bool, unsigned int)"},
    {0x10030e218u, 666u, "cSC4ModelMaker::DoOccupantMoved(cISC4Occupant*, bool, unsigned int)"},
    {0x10030e4b2u, 264u, "cSC4ModelMaker::DoLotStateChanged(cISC4Lot*, unsigned int, unsigned int)"},
    {0x10030e5bau, 650u, "cSC4ModelMaker::DoOccupantHilighted(cISC4Occupant*, int, int)"},
    {0x10030e844u, 468u, "cSC4ModelMaker::UpdateModelLoadRect()"},
    {0x10030ea18u, 748u, "cSC4ModelMaker::DoZoomAndRotationChange(int, int, int, int)"},
    {0x10030ed04u, 412u, "cSC4ModelMaker::DoViewTranslationChange()"},
    {0x10030eea0u, 318u, "cSC4ModelMaker::DoViewFilterChange(cSC4ViewFilterInformation*, cSC4ViewFilterInformation*)"},
    {0x10030efdeu, 258u, "cSC4ModelMaker::DoOccupantInserted(cISC4Occupant*, bool, unsigned int)"},
    {0x10030f0e0u, 238u, "cSC4ModelMaker::DoOccupantRemoved(cISC4Occupant*, bool, unsigned int)"},
    {0x10030f1ceu, 414u, "cSC4ModelMaker::DoOccupantShown(cISC4Occupant*, bool, unsigned int)"},
    {0x10030f36cu, 1870u, "cSC4ModelMaker::DoPostCityInit()"},
    {0x10030fabau, 918u, "cSC4ModelMaker::DoBuildingCompletionChanged(cISC4BuildingOccupant*)"},
    {0x10030fe50u, 2656u, "cSC4ModelMaker::ApplyLotStateEffect(cISC4Occupant*, unsigned int, unsigned int, unsigned int)"},
    {0x1003108b0u, 138u, "cSC4ModelMaker::DoPropStateChanged(cISC4PropOccupant*, unsigned char)"},
    {0x10031093au, 56u, "cSC4ModelMaker::EnableDeferredOccupantOperations(bool)"},
    {0x100310972u, 652u, "cSC4ModelMaker::DoLightingChanged()"},
    {0x100310bfeu, 682u, "cSC4ModelMaker::UnloadUnnecessaryModels()"},
    {0x100310ea8u, 384u, "cSC4ModelMaker::ChangeModels(cISC4Occupant*, unsigned int, int)"},
    {0x100311028u, 150u, "cSC4ModelMaker::RemoveModels(cISC4Occupant*, unsigned int)"},
    {0x1003110beu, 1332u, "cSC4ModelMaker::AddModels(cISC4Occupant*, int, int, unsigned int, bool)"},
    {0x1003115f2u, 230u, "cSC4ModelMaker::DoTerrainHidingForOccupantCells(cISC4Occupant*, unsigned int, bool)"},
    {0x1003116d8u, 1258u, "cSC4ModelMaker::RebuildDynamicTexture(int)"},
    {0x100311bc2u, 802u, "cSC4ModelMaker::CommitDeferredOccupantOperations()"},
    {0x100311ee4u, 298u, "cSC4ModelMaker::OccupantFilterIterationFunction(cISC4Occupant*)"},
    {0x10031200eu, 2014u, "cSC4ModelMaker::DoTerrainHidingForCells(SC4Rect<int> const&, bool)"},
    {0x1003127ecu, 214u, "cSC4ModelMaker::RemoveModelsInappropriateForRotation(int)"},
    {0x1003128c2u, 1528u, "cSC4ModelMaker::LoadOccupantModelsForZoomAndRotation(cISC4Occupant*, int, int)"},
    {0x100312ebau, 116u, "cSC4ModelMaker::UpdateFocusOfBackgroundLoad()"},
    {0x100312f2eu, 222u, "cSC4ModelMaker::GetOccupantLotCondition(cISC4Occupant*, unsigned int&)"},
    {0x10031300cu, 64u, "cSC4ModelMaker::LoadBuildingModels(cISC4Occupant*, int, int, unsigned int)"},
    {0x10031304cu, 520u, "cSC4ModelMaker::LoadFoundationModels(cISC4Occupant*, int, int, unsigned int)"},
    {0x100313254u, 216u, "cSC4ModelMaker::ApplyStateEffectToModels(cISC4Occupant*, unsigned int, unsigned int, unsigned int)"},
    {0x10031332cu, 64u, "cSC4ModelMaker::LoadFloraModels(cISC4Occupant*, int, int, unsigned int)"},
    {0x10031336cu, 1160u, "cSC4ModelMaker::LoadModelsCommon(cISC4Occupant*, int, int, unsigned int)"},
    {0x1003137f4u, 100u, "cSC4ModelMaker::LoadPropModels(cISC4Occupant*, int, int, unsigned int)"},
    {0x100313858u, 108u, "cSC4ModelMaker::LoadLotBaseTextureModels(cISC4Occupant*, int, int, unsigned int)"},
    {0x1003138c4u, 316u, "cSC4ModelMaker::LoadNetworkModels(cISC4Occupant*, int, int)"},
    {0x100313a00u, 2080u, "cSC4ModelMaker::CreateOccupantShadow(cISC4Occupant*, int, int)"},
    {0x100314220u, 264u, "cSC4ModelMaker::LoadUndergroundConnectionModels(cISC4Occupant*, int, int)"},
    {0x100314328u, 998u, "cSC4ModelMaker::AddModel(cISC4Occupant*, cGZPersistResourceKey const&, int, int, bool, float const*, float const*)"},
    {0x10031470eu, 276u, "cSC4ModelMaker::CreatePlaceHolderModelInstance(cISC4Occupant*, int, int, int, bool)"},
    {0x100314822u, 1222u, "cSC4ModelMaker::FindModelTransform(cISCPropertyHolder*, bool, int, float const*, float const*, cS3DTransform*)"},
    {0x100314e90u, 96u, "cSC4ModelMaker::DeferredOccupantOperation::DeferredOccupantOperation(cISC4Occupant*, cSC4ModelMaker::DeferredOccupantOperation::OperationType)"},

    // cHitMan (44 entry points)
    {0x1004be400u, 20u, "cHitMan::HandleTrackFlowError(cIHitTrackPlayer*, char const*)"},
    {0x1004be428u, 932u, "cHitMan::cHitMan()"},
    {0x1004bede6u, 1156u, "cHitMan::Init()"},
    {0x1004bf27cu, 550u, "cHitMan::Shutdown()"},
    {0x1004bf4b4u, 84u, "cHitMan::CreateTrackPlayer(unsigned int, void**)"},
    {0x1004bf576u, 36u, "cHitMan::UpdateView()"},
    {0x1004bf5beu, 78u, "cHitMan::GetSequenceGroupSnd(unsigned int, cIGZSnd**)"},
    {0x1004bf65au, 88u, "cHitMan::SetSequenceGroupSnd(unsigned int, cIGZSnd*)"},
    {0x1004bf790u, 18u, "cHitMan::Pause()"},
    {0x1004bf7c8u, 72u, "cHitMan::GetRegisterVal(int) const"},
    {0x1004bf854u, 70u, "cHitMan::SetRegisterVal(int, int)"},
    {0x1004bf8deu, 68u, "cHitMan::AddToActiveTrackList(cIGZSnd*)"},
    {0x1004bf922u, 52u, "cHitMan::RemoveFromActiveTrackList(cIGZSnd const*)"},
    {0x1004bf956u, 278u, "cHitMan::GetHitList(unsigned int, unsigned int, void**)"},
    {0x1004bfa7au, 180u, "cHitMan::AllocChannel(unsigned int, void**)"},
    {0x1004bfb3cu, 100u, "cHitMan::FreeChannel(cIGZSnd*)"},
    {0x1004bfbaeu, 212u, "cHitMan::DuckMapSetSndobPri(cIGZSnd const*, int)"},
    {0x1004bfd48u, 94u, "cHitMan::DuckMapMaxPri()"},
    {0x1004bfdb4u, 98u, "cHitMan::DuckMapSndobPri(cIGZSnd const*)"},
    {0x1004bfe24u, 96u, "cHitMan::DuckMapRemoveAll()"},
    {0x1004bfe92u, 22u, "cHitMan::GetSourceDataField(int, int, int*) const"},
    {0x1004bfebeu, 254u, "cHitMan::ControlGroup(int)"},
    {0x1004c0082u, 76u, "cHitMan::RegisterSourceDataRequestHandler(cIHitAppDataRequestHandler*)"},
    {0x1004c011au, 140u, "cHitMan::AddToControlGroup(int, int)"},
    {0x1004c01b4u, 196u, "cHitMan::RegisterTrackDataBlock(cIHitTrackDataBlock*)"},
    {0x1004c02eau, 124u, "cHitMan::RemoveTrackDataBlock(cIHitTrackDataBlock*)"},
    {0x1004c0430u, 140u, "cHitMan::GetTrackDataBlockAndOffset(unsigned int, cIHitTrackDataBlock**, unsigned int&)"},
    {0x1004c04cau, 178u, "cHitMan::TrackDataEntryPoint(unsigned int) const"},
    {0x1004c06eau, 758u, "cHitMan::LoadHitFile(cIGZString const&)"},
    {0x1004c09eeu, 686u, "cHitMan::ParseHitFile(cIGZFile&)"},
    {0x1004c0c9cu, 494u, "cHitMan::FilepathHasEmbeddedKey(cRZString&, cGZPersistResourceKey&)"},
    {0x1004c0e8au, 422u, "cHitMan::ParseHitFileFromIGZPersistRecordAsIGZFile(cGZPersistResourceKey&)"},
    {0x1004c1320u, 1642u, "cHitMan::UnloadHitFile(cIGZString const&)"},
    {0x1004c199cu, 88u, "cHitMan::AddDBSegment(unsigned int, cIGZPersistDBSegment*)"},
    {0x1004c1ad2u, 82u, "cHitMan::FindDBSegment(unsigned int, unsigned int, void**)"},
    {0x1004c1b32u, 78u, "cHitMan::RemoveDBSegment(unsigned int)"},
    {0x1004c1b8eu, 208u, "cHitMan::AddTypeToGroupAssociation(unsigned int, unsigned int)"},
    {0x1004c1d24u, 82u, "cHitMan::GetSem(unsigned int, unsigned int, void**) const"},
    {0x1004c1d84u, 70u, "cHitMan::GetSem(unsigned int)"},
    {0x1004c1e10u, 88u, "cHitMan::GetSemAt(unsigned int)"},
    {0x1004c1e76u, 110u, "cHitMan::RegisterSem(cIHitSem*)"},
    {0x1004c1fc2u, 82u, "cHitMan::RemoveSem(unsigned int)"},
    {0x1004c2022u, 66u, "cHitMan::RemoveAllSems()"},
    {0x1004c2548u, 32u, "cHitMan::Random(int, int)"},

    // cFreshPlayer (38 entry points)
    {0x10043abb2u, 60u, "cFreshPlayer::Create()"},
    {0x1004ccd8eu, 718u, "cFreshPlayer::cFreshPlayer()"},
    {0x1004cd1f6u, 422u, "cFreshPlayer::Init(cRZString const&)"},
    {0x1004cd39cu, 502u, "cFreshPlayer::Init(cRZString const&, cRZString const&)"},
    {0x1004cd592u, 398u, "cFreshPlayer::Init(cGZPersistResourceKey const&)"},
    {0x1004cd720u, 134u, "cFreshPlayer::Init(cFreshScore*, cRZString const&)"},
    {0x1004cd7a6u, 410u, "cFreshPlayer::Init()"},
    {0x1004cd940u, 256u, "cFreshPlayer::Shutdown()"},
    {0x1004cda40u, 30u, "cFreshPlayer::Play(int)"},
    {0x1004cda5eu, 304u, "cFreshPlayer::Play()"},
    {0x1004cdb8eu, 284u, "cFreshPlayer::UpdatePlayerInfo()"},
    {0x1004cdcaau, 102u, "cFreshPlayer::Stop()"},
    {0x1004cdd10u, 254u, "cFreshPlayer::StopNoLock()"},
    {0x1004cde0eu, 110u, "cFreshPlayer::StopFade()"},
    {0x1004cde7cu, 1124u, "cFreshPlayer::Update(int)"},
    {0x1004ce2e0u, 372u, "cFreshPlayer::CalculateCellplayer3DPosition(cFreshCellPlayer*)"},
    {0x1004ce462u, 532u, "cFreshPlayer::ProbOfCellStarting(int, int, int)"},
    {0x1004ce6a0u, 622u, "cFreshPlayer::GetCellToAdd(int&, int&, int)"},
    {0x1004ce90eu, 156u, "cFreshPlayer::ToggleCell(int, int)"},
    {0x1004ce9aau, 956u, "cFreshPlayer::GetCellToSubtract(int&, int&)"},
    {0x1004ced66u, 84u, "cFreshPlayer::SubtractCell(int, int)"},
    {0x1004cedbau, 108u, "cFreshPlayer::SetVolume(int)"},
    {0x1004cee26u, 118u, "cFreshPlayer::SetVolumeNoLock(int)"},
    {0x1004ceea6u, 236u, "cFreshPlayer::ProcessVolumeFadeTimerCallback()"},
    {0x1004cef92u, 390u, "cFreshPlayer::FadeVolume(int, int, bool)"},
    {0x1004cf118u, 158u, "cFreshPlayer::SetTempo(int)"},
    {0x1004cf1b6u, 116u, "cFreshPlayer::SetXPos(int)"},
    {0x1004cf22au, 116u, "cFreshPlayer::SetYPos(int)"},
    {0x1004cf2deu, 128u, "cFreshPlayer::NumCellsBusy()"},
    {0x1004cf35eu, 84u, "cFreshPlayer::NumCellsDying()"},
    {0x1004cf3b2u, 128u, "cFreshPlayer::NumCellsPlaying()"},
    {0x1004cf432u, 62u, "cFreshPlayer::ShowUsage()"},
    {0x1004cf490u, 28u, "cFreshPlayer::GetDataDir(cRZString&)"},
    {0x1004cf4beu, 114u, "cFreshPlayer::GetMinCellsPlaying()"},
    {0x1004cf5c0u, 88u, "cFreshPlayer::Reseed(unsigned int)"},
    {0x1004cf63eu, 56u, "cFreshPlayer::SetDivision3DPosition(unsigned int, unsigned int, int, int, int)"},
    {0x1004cf6a6u, 84u, "cFreshPlayer::SetExternalSoundSystemInterface(cIExternalSoundSystem*)"},
    {0x1004cf6fau, 260u, "cFreshPlayer::LoadScore(cIGZString const&, unsigned int)"},

    // cSTEVolcanoLava (36 entry points)
    {0x100751d20u, 460u, "cSTEVolcanoLava::AddDecal(int, int)"},
    {0x100751fd0u, 414u, "cSTEVolcanoLava::AddSteamEffect(int, int)"},
    {0x100752228u, 118u, "cSTEVolcanoLava::AssignSurfaceTextureCoordinates(int)"},
    {0x10075229eu, 362u, "cSTEVolcanoLava::BurnSurroundingStuff()"},
    {0x100752408u, 86u, "cSTEVolcanoLava::ComputeBoundRectBasedOnTerrainVisibility(SC4DrawContext*, SC4Rect<int>&)"},
    {0x10075245eu, 300u, "cSTEVolcanoLava::ComputeVisibleRect(SC4DrawContext*, SC4Rect<int>&) const"},
    {0x10075258au, 196u, "cSTEVolcanoLava::CreateBurnAroundRegion(int, int)"},
    {0x10075264eu, 340u, "cSTEVolcanoLava::cSTEVolcanoLava()"},
    {0x1007527acu, 146u, "cSTEVolcanoLava::DoAnimationTick(unsigned int)"},
    {0x10075283eu, 1314u, "cSTEVolcanoLava::DoLavaFlow(unsigned int)"},
    {0x100752d60u, 348u, "cSTEVolcanoLava::TickDecals(unsigned int)"},
    {0x100752ebcu, 320u, "cSTEVolcanoLava::GetNeighborWithLowestLavaLevel(int)"},
    {0x100752ffcu, 278u, "cSTEVolcanoLava::SetThingsOnFire(int, float, float)"},
    {0x100753112u, 660u, "cSTEVolcanoLava::PrepareForDisplay()"},
    {0x1007533a6u, 108u, "cSTEVolcanoLava::DoMessage(cIGZMessage2*)"},
    {0x100753412u, 322u, "cSTEVolcanoLava::Shutdown()"},
    {0x100753554u, 78u, "cSTEVolcanoLava::MarkSteamEnded(unsigned int)"},
    {0x1007535b4u, 150u, "cSTEVolcanoLava::DrawLavaSurface(SC4DrawContext*, cSTEVertBufExtn*)"},
    {0x10075364au, 308u, "cSTEVolcanoLava::DrawLavaSurfaceWithVertBufExtension(SC4DrawContext*, cSTEVertBufExtn*)"},
    {0x10075377eu, 460u, "cSTEVolcanoLava::DrawLavaSurfaceWithoutVertBufExtension(SC4DrawContext*)"},
    {0x10075394au, 232u, "cSTEVolcanoLava::DrawSides(SC4DrawContext*)"},
    {0x100753a32u, 450u, "cSTEVolcanoLava::DrawXSide(int, int, int, SC4DrawContext*)"},
    {0x100753bf4u, 466u, "cSTEVolcanoLava::DrawZSide(int, int, int, SC4DrawContext*)"},
    {0x100753dc6u, 396u, "cSTEVolcanoLava::Init()"},
    {0x100753f52u, 428u, "cSTEVolcanoLava::StaticInit()"},
    {0x1007540feu, 74u, "cSTEVolcanoLava::InitTriangleIndices(int)"},
    {0x100754148u, 276u, "cSTEVolcanoLava::InitSurfacePoints()"},
    {0x10075425cu, 126u, "cSTEVolcanoLava::IsGoodVertexForPlacingSteam(int) const"},
    {0x1007542dau, 318u, "cSTEVolcanoLava::UpdateSteamEffects()"},
    {0x100754492u, 446u, "cSTEVolcanoLava::ReadTuningParameters()"},
    {0x100754676u, 190u, "cSTEVolcanoLava::RemoveDecals()"},
    {0x100754734u, 180u, "cSTEVolcanoLava::RemoveSteamEffects()"},
    {0x100754804u, 316u, "cSTEVolcanoLava::StartLavaFlow(float, float, float, bool)"},
    {0x100754940u, 128u, "cSTEVolcanoLava::StaticShutdown(void*)"},
    {0x1007549c0u, 390u, "cSTEVolcanoLava::Update(SC4DrawContext*)"},
    {0x100754b46u, 52u, "cSTEVolcanoLava::GetLavaHeightByCityCell(int, int)"},

    // cSC4CameraControl (27 entry points)
    {0x10076affeu, 406u, "cSC4CameraControl::cSC4CameraControl()"},
    {0x10076b24eu, 88u, "cSC4CameraControl::Init(cS3DVector3 const&, cS3DCamera*)"},
    {0x10076b2a6u, 104u, "cSC4CameraControl::AttachToCamera(cS3DCamera*)"},
    {0x10076b30eu, 176u, "cSC4CameraControl::UpdateCameraPosition(unsigned int)"},
    {0x10076b3beu, 40u, "cSC4CameraControl::Shutdown()"},
    {0x10076b540u, 304u, "cSC4CameraControl::ScrollByViewportAngle(float, float)"},
    {0x10076b670u, 170u, "cSC4CameraControl::ConvertWorldPointToCameraPlanePoint(cS3DVector3 const&, cS3DVector2&)"},
    {0x10076b71au, 754u, "cSC4CameraControl::ClampScrollToBounds(cS3DVector2 const&, cS3DVector2 const&, cS3DVector2&)"},
    {0x10076ba0cu, 223u, "cSC4CameraControl::ConvertCameraPlanePointToWorldPoint(cS3DVector2 const&, cS3DVector3&)"},
    {0x10076bb7cu, 70u, "cSC4CameraControl::ZoomIn()"},
    {0x10076bbe2u, 282u, "cSC4CameraControl::SetZoom(float)"},
    {0x10076bcfcu, 52u, "cSC4CameraControl::ZoomOut()"},
    {0x10076bd3eu, 60u, "cSC4CameraControl::SetCustomMagnification(float)"},
    {0x10076bd7au, 66u, "cSC4CameraControl::SetRotation(int)"},
    {0x10076be08u, 290u, "cSC4CameraControl::SetZoomAndRotation(int, int)"},
    {0x10076bf2au, 210u, "cSC4CameraControl::SetViewTargetPosition(cS3DVector3 const&)"},
    {0x10076bffcu, 126u, "cSC4CameraControl::SetViewTargetVelocity(cS3DVector3 const&)"},
    {0x10076c0d6u, 62u, "cSC4CameraControl::SetCameraPositionTweakAdd(float, float)"},
    {0x10076c114u, 52u, "cSC4CameraControl::SetCitySize(float, float)"},
    {0x10076c148u, 104u, "cSC4CameraControl::GetBaseTargetPositionForRotation(float*, int)"},
    {0x10076c1b0u, 246u, "cSC4CameraControl::SendPreViewChangeMessage(unsigned int)"},
    {0x10076c2a6u, 498u, "cSC4CameraControl::UpdateCameraZoomAndRotationParams()"},
    {0x10076c498u, 780u, "cSC4CameraControl::UpdateCameraPositionParams()"},
    {0x10076c7a4u, 872u, "cSC4CameraControl::UpdateScrollBounds()"},
    {0x10076cb0cu, 446u, "cSC4CameraControl::SendPostViewChangeMessage(unsigned int)"},
    {0x10076cccau, 318u, "cSC4CameraControl::ClampTargetToScrollBounds()"},
    {0x10076ce08u, 36u, "cSC4CameraControl::SendViewChangeMessage()"},

    // cSC4WinAnimationRecorder (24 entry points)
    {0x100371f8au, 932u, "cSC4WinAnimationRecorder::cSC4WinAnimationRecorder()"},
    {0x10037232eu, 1398u, "cSC4WinAnimationRecorder::ReadPreferences()"},
    {0x100372aa0u, 4206u, "cSC4WinAnimationRecorder::Init()"},
    {0x100373b0eu, 1172u, "cSC4WinAnimationRecorder::GetGXDirectory()"},
    {0x100373fa2u, 292u, "cSC4WinAnimationRecorder::UpdateStatusText()"},
    {0x1003740c6u, 564u, "cSC4WinAnimationRecorder::Shutdown()"},
    {0x1003742fau, 706u, "cSC4WinAnimationRecorder::Stop()"},
    {0x1003745bcu, 118u, "cSC4WinAnimationRecorder::DestroyInternalBitmaps()"},
    {0x100374632u, 1442u, "cSC4WinAnimationRecorder::WritePreferences()"},
    {0x100374bd4u, 56u, "cSC4WinAnimationRecorder::GZOnMouseDownL(int, int, unsigned int)"},
    {0x100374c0cu, 36u, "cSC4WinAnimationRecorder::GZOnMouseUpL(int, int, unsigned int)"},
    {0x100374c30u, 102u, "cSC4WinAnimationRecorder::GZOnMouseMove(int, int, unsigned int)"},
    {0x100374c96u, 176u, "cSC4WinAnimationRecorder::GZPaint()"},
    {0x100374d46u, 306u, "cSC4WinAnimationRecorder::DoMessage(cGZMessage&)"},
    {0x100374e78u, 280u, "cSC4WinAnimationRecorder::RecordCurrentFrame()"},
    {0x100374f90u, 178u, "cSC4WinAnimationRecorder::GZOnCommand(unsigned int, unsigned long)"},
    {0x100375042u, 1200u, "cSC4WinAnimationRecorder::Start()"},
    {0x1003754f2u, 190u, "cSC4WinAnimationRecorder::SetAnimationOutputDirectory(cIGZString const&, bool)"},
    {0x1003755b0u, 118u, "cSC4WinAnimationRecorder::GetCurrentCaptureArea(cRZRect&)"},
    {0x100375626u, 368u, "cSC4WinAnimationRecorder::FixAnimationName(cRZString&)"},
    {0x100375796u, 324u, "cSC4WinAnimationRecorder::CreateInternalBitmaps()"},
    {0x1003758dau, 304u, "cSC4WinAnimationRecorder::WriteOutFinalBitmaps()"},
    {0x100375a0au, 984u, "cSC4WinAnimationRecorder::WriteOutputBitmaps()"},
    {0x100375de2u, 526u, "cSC4WinAnimationRecorder::WriteAnimatedGraphicFile()"},

    // cSC424HourClock (22 entry points)
    {0x100184c5eu, 350u, "cSC424HourClock::cSC424HourClock()"},
    {0x100184eceu, 610u, "cSC424HourClock::Init()"},
    {0x100185200u, 234u, "cSC424HourClock::Shutdown()"},
    {0x1001854f6u, 774u, "cSC424HourClock::DoMessage(cIGZMessage2*)"},
    {0x100185982u, 106u, "cSC424HourClock::Pause(bool)"},
    {0x1001859ecu, 242u, "cSC424HourClock::PostMessageServerMessage(unsigned int, unsigned int, unsigned int)"},
    {0x100185aecu, 106u, "cSC424HourClock::HiddenPause(bool)"},
    {0x100185b64u, 86u, "cSC424HourClock::IsNight() const"},
    {0x100185bbau, 138u, "cSC424HourClock::EnableDayNightTransitionNotifications(bool)"},
    {0x100185c44u, 62u, "cSC424HourClock::SetVisualTimeMode(cISC424HourClock::EVisualTimeMode)"},
    {0x100185c82u, 440u, "cSC424HourClock::ChangeVisualTime(unsigned int)"},
    {0x100185e66u, 50u, "cSC424HourClock::SetTimePercentage(SC4Percentage const&)"},
    {0x100185e98u, 22u, "cSC424HourClock::GetHour() const"},
    {0x100185eaeu, 22u, "cSC424HourClock::GetVisualHour() const"},
    {0x100185ec4u, 126u, "cSC424HourClock::SetHour(unsigned int)"},
    {0x100185f42u, 44u, "cSC424HourClock::GetMinute() const"},
    {0x100185f6eu, 44u, "cSC424HourClock::GetVisualMinute() const"},
    {0x100185f9au, 64u, "cSC424HourClock::SetMinute(unsigned int)"},
    {0x100185fdau, 60u, "cSC424HourClock::GetSeconds() const"},
    {0x100186016u, 60u, "cSC424HourClock::GetVisualSeconds() const"},
    {0x100186052u, 88u, "cSC424HourClock::SetSeconds(unsigned int)"},
    {0x100186124u, 314u, "cSC424HourClock::OnIdle(unsigned int)"},

    // nsGZLua5ThreadLib (22 entry points)
    {0x100697760u, 623u, "nsGZLua5ThreadLib::OpenLib(cIGZLua5*)"},
    {0x1006979cfu, 178u, "nsGZLua5ThreadLib::Resume(lua_State*)"},
    {0x100697a81u, 363u, "nsGZLua5ThreadLib::Create(lua_State*)"},
    {0x100697becu, 46u, "nsGZLua5ThreadLib::Wrap(lua_State*)"},
    {0x100697c1au, 253u, "nsGZLua5ThreadLib::Destroy(lua_State*)"},
    {0x100697d17u, 80u, "nsGZLua5ThreadLib::Valid(lua_State*)"},
    {0x100697d67u, 58u, "nsGZLua5ThreadLib::ID(lua_State*)"},
    {0x100697da1u, 95u, "nsGZLua5ThreadLib::Status(lua_State*)"},
    {0x100697e00u, 97u, "nsGZLua5ThreadLib::Class(lua_State*)"},
    {0x100697e61u, 74u, "nsGZLua5ThreadLib::Parent(lua_State*)"},
    {0x100697eccu, 252u, "nsGZLua5ThreadLib::cThreadRegistry::ResumeThread(lua_State*, int, int)"},
    {0x100697fc8u, 144u, "nsGZLua5ThreadLib::wrap_internal(lua_State*)"},
    {0x100698058u, 560u, "nsGZLua5ThreadLib::cThreadRegistry::CreateThread(lua_State*, int, int)"},
    {0x100698288u, 108u, "nsGZLua5ThreadLib::thread_id_param(lua_State*)"},
    {0x1006982f4u, 62u, "nsGZLua5ThreadLib::cThreadRegistry::IsValidThread(int)"},
    {0x100698332u, 378u, "nsGZLua5ThreadLib::cThreadRegistry::DestroyThread(lua_State*, int)"},
    {0x1006984acu, 154u, "nsGZLua5ThreadLib::cThreadRegistry::GetThreadStatus(int)"},
    {0x100698546u, 118u, "nsGZLua5ThreadLib::cThreadRegistry::GetParentID(int)"},
    {0x1006985bcu, 52u, "nsGZLua5ThreadLib::cThreadRegistry::cThreadRegistry()"},
    {0x10069863au, 352u, "nsGZLua5ThreadLib::cThreadRegistry::Init(cIGZLua5*, lua_State*)"},
    {0x100698820u, 78u, "nsGZLua5ThreadLib::cThreadRegistry::Shutdown()"},
    {0x10069886eu, 42u, "nsGZLua5ThreadLib::cThreadRegistry::GetThreadContext(int)"},

    // cSC4WinRegionView (20 entry points)
    {0x10075afc6u, 160u, "cSC4WinRegionView::cSC4WinRegionView()"},
    {0x10075b1e0u, 672u, "cSC4WinRegionView::Init()"},
    {0x10075b4f0u, 214u, "cSC4WinRegionView::Shutdown()"},
    {0x10075b5c6u, 140u, "cSC4WinRegionView::ClearCities()"},
    {0x10075b652u, 94u, "cSC4WinRegionView::SetArea(int, int, int, int)"},
    {0x10075b6b0u, 246u, "cSC4WinRegionView::InvalidateRegion(cSC4RegionViewCity&)"},
    {0x10075b7a6u, 860u, "cSC4WinRegionView::UpdateSignpost(cSC4RegionViewCity&)"},
    {0x10075bb02u, 92u, "cSC4WinRegionView::InvalidateAllRegions()"},
    {0x10075bb66u, 6550u, "cSC4WinRegionView::RenderTile(cIGZBuffer*, int, int)"},
    {0x10075d4fcu, 126u, "cSC4WinRegionView::filter_tutorial_mayor_name(cISC4RegionalCity*, cIGZString*) const"},
    {0x10075d58cu, 90u, "cSC4WinRegionView::AddCity(cSC4RegionViewCity*)"},
    {0x10075d5e6u, 1014u, "cSC4WinRegionView::UpdateCity(cSC4RegionViewCity*)"},
    {0x10075d9dcu, 262u, "cSC4WinRegionView::SetScrollPos(int, int)"},
    {0x10075dae2u, 168u, "cSC4WinRegionView::GetRegionScreenRect(cSC4RegionViewCity const&, cRZRect&) const"},
    {0x10075db8au, 350u, "cSC4WinRegionView::GetCityAtPoint(int, int)"},
    {0x10075dd66u, 76u, "cSC4WinRegionView::SelectCity(int, int)"},
    {0x10075ddc0u, 210u, "cSC4WinRegionView::GetRegionPlayfieldRect(cSC4RegionViewCity const&, cRZRect&) const"},
    {0x10075de92u, 78u, "cSC4WinRegionView::SetSignpostType(unsigned int, unsigned int)"},
    {0x10075dee0u, 1876u, "cSC4WinRegionView::InitSignpost(cSC4RegionViewCity*, cRZAutoRefCount<cISC4RegionalCity>*)"},
    {0x10075e6acu, 82u, "cSC4WinRegionView::is_point_transparent(int, int) const"},

    // cSC4ModelRegEffects (18 entry points)
    {0x1001e3d42u, 22u, "cSC4ModelRegEffects::Init()"},
    {0x1001e3d58u, 46u, "cSC4ModelRegEffects::Shutdown()"},
    {0x1001e3d86u, 160u, "cSC4ModelRegEffects::ClearEffectsList()"},
    {0x1001e3e26u, 94u, "cSC4ModelRegEffects::SetModel(cIS3DModelInstance*)"},
    {0x1001e3e84u, 788u, "cSC4ModelRegEffects::Update()"},
    {0x1001e455au, 910u, "cSC4ModelRegEffects::Update(cS3DVector3 const&, cS3DVector3 const&, float)"},
    {0x1001e48e8u, 132u, "cSC4ModelRegEffects::StartAllEffects(cISC4VisualEffect::tTransitionType)"},
    {0x1001e496cu, 132u, "cSC4ModelRegEffects::StopAllEffects(cISC4VisualEffect::tTransitionType)"},
    {0x1001e49f0u, 260u, "cSC4ModelRegEffects::StartNamedEffects(cRZString const&, cISC4VisualEffect::tTransitionType, bool)"},
    {0x1001e4af4u, 190u, "cSC4ModelRegEffects::StopNamedEffects(cRZString const&, cISC4VisualEffect::tTransitionType)"},
    {0x1001e4bb2u, 288u, "cSC4ModelRegEffects::ToggleNamedEffects(cRZString const&, cISC4VisualEffect::tTransitionType, bool)"},
    {0x1001e4cd2u, 356u, "cSC4ModelRegEffects::RecreateNamedEffects(cRZString const&, cISC4VisualEffect::tTransitionType, bool)"},
    {0x1001e4e36u, 118u, "cSC4ModelRegEffects::AddRegEffect(cRZString const&, cRZString const&)"},
    {0x1001e4eacu, 42u, "cSC4ModelRegEffects::FindRegEffect(cRZString const&)"},
    {0x1001e4ed6u, 108u, "cSC4ModelRegEffects::RemoveRegEffect(cRZString const&)"},
    {0x1001e4f42u, 570u, "cSC4ModelRegEffects::InitEffectsList()"},
    {0x1001e517cu, 660u, "cSC4ModelRegEffects::AddRegEffect(cRZString const&, cRZString const&, std::__1::vector<cS3DRegPoints::cTransform, std::__1::allocator<cS3DRegPoints::cTransform>> const*)"},
    {0x1001e5410u, 878u, "cSC4ModelRegEffects::GetCurrentEffects(std::__1::vector<cSC4ModelRegEffects::CurrentEffectItem, std::__1::allocator<cSC4ModelRegEffects::CurrentEffectItem>>&)"},

    // cSCExemplarResourceView (17 entry points)
    {0x10035c52cu, 78u, "cSCExemplarResourceView::cSCExemplarResourceView()"},
    {0x10035c600u, 2806u, "cSCExemplarResourceView::Init()"},
    {0x10035d0f6u, 46u, "cSCExemplarResourceView::Shutdown()"},
    {0x10035d146u, 154u, "cSCExemplarResourceView::doButtonPress(unsigned int)"},
    {0x10035d1e0u, 486u, "cSCExemplarResourceView::addProperty()"},
    {0x10035d3c6u, 792u, "cSCExemplarResourceView::editProperty()"},
    {0x10035d6deu, 522u, "cSCExemplarResourceView::deleteProperty()"},
    {0x10035d8e8u, 452u, "cSCExemplarResourceView::selectAndAssignCohort()"},
    {0x10035daacu, 204u, "cSCExemplarResourceView::assignCohort(cISCResExemplarCohort*)"},
    {0x10035db78u, 132u, "cSCExemplarResourceView::SetKeyToView(cGZPersistResourceKey const&)"},
    {0x10035dc0eu, 372u, "cSCExemplarResourceView::PromptToSave()"},
    {0x10035dd94u, 1162u, "cSCExemplarResourceView::updateView(bool)"},
    {0x10035e21eu, 380u, "cSCExemplarResourceView::showFileName()"},
    {0x10035e39au, 492u, "cSCExemplarResourceView::addPropertyToListBox(cIGZWinGrid*, cISCProperty*, bool, bool)"},
    {0x10035e586u, 370u, "cSCExemplarResourceView::editProperty(cISCProperty*)"},
    {0x10035e6f8u, 1112u, "cSCExemplarResourceView::saveResource()"},
    {0x10035ed56u, 632u, "cSCExemplarResourceView::revertResource()"},

    // nSC4UI (16 entry points)
    {0x100309e70u, 742u, "nSC4UI::CreateCatalogItemFromExemplar(cISCResExemplar*, cIGZWin*, unsigned int, cISCStringDetokenizer*)"},
    {0x10030a156u, 549u, "nSC4UI::PropertyTokenReplacementFunction(cIGZString const&, cIGZString&, void*)"},
    {0x10030a37bu, 883u, "nSC4UI::CreateCatalogItemList(std::__1::multimap<int, cISC4CatalogItem*, std::__1::less<int>, std::__1::allocator<std::__1::pair<int const, cISC4CatalogItem*>>>&, cISC4WinCatalogView*, unsigned int, unsigned int, unsigned int, unsigned int)"},
    {0x10030a6f4u, 252u, "nSC4UI::CreateCatalogView(unsigned int, unsigned int, unsigned int, unsigned int)"},
    {0x10030a7f0u, 414u, "nSC4UI::InterpolateRampToPalette(unsigned int*, int, unsigned int const*, int)"},
    {0x10030a98eu, 61u, "nSC4UI::GetChildWindowForCursorPos(cIGZWin*, int, int)"},
    {0x10030aa41u, 105u, "nSC4UI::GetChildWindowForCursorPosRecursive(cIGZWin*, int, int)"},
    {0x10030aaaau, 480u, "nSC4UI::AlignWindowToWindow(unsigned int, unsigned int, unsigned int, cIGZWin*)"},
    {0x10030ac8au, 136u, "nSC4UI::AlignWindowToWindow(cIGZWin*, cIGZWin*, cIGZWin*)"},
    {0x10030ad12u, 130u, "nSC4UI::AlignWindowSetToWindow(std::__1::set<cIGZWin*, std::__1::less<cIGZWin*>, std::__1::allocator<cIGZWin*>> const&, cIGZWin*)"},
    {0x10030ae2du, 86u, "nSC4UI::GetWindowChildrenSet(cIGZWin*, std::__1::set<cIGZWin*, std::__1::less<cIGZWin*>, std::__1::allocator<cIGZWin*>>&)"},
    {0x10030ae83u, 266u, "nSC4UI::GetWindowChildrenNotInSet(cIGZWin*, std::__1::set<cIGZWin*, std::__1::less<cIGZWin*>, std::__1::allocator<cIGZWin*>> const&, std::__1::set<cIGZWin*, std::__1::less<cIGZWin*>, std::__1::allocator<cIGZWin*>>&)"},
    {0x10030b220u, 60u, "nSC4UI::ClearAcceptFocusFlagRecursive(cIGZWin*)"},
    {0x10030b2dau, 891u, "nSC4UI::GenericNotificationDialog(cIGZString const&, cIGZString const&)"},
    {0x10030b655u, 1060u, "nSC4UI::GenericConfirmationDialog(cIGZString const&, cIGZString const&, cIGZString const*, cIGZString const*)"},
    {0x10030ba79u, 811u, "nSC4UI::DoScriptedModalDialog(unsigned int, unsigned int)"},

    // cSC4PickSimWinProc (15 entry points)
    {0x1003549dcu, 168u, "cSC4PickSimWinProc::cSC4PickSimWinProc(unsigned int)"},
    {0x100354b88u, 4770u, "cSC4PickSimWinProc::Init()"},
    {0x100355e2au, 718u, "cSC4PickSimWinProc::SetupPickSims(unsigned int)"},
    {0x1003560f8u, 1146u, "cSC4PickSimWinProc::PickHead(unsigned int)"},
    {0x100356572u, 3378u, "cSC4PickSimWinProc::SetScrollButtons()"},
    {0x1003572a4u, 184u, "cSC4PickSimWinProc::TickSelf()"},
    {0x10035735cu, 298u, "cSC4PickSimWinProc::Shutdown()"},
    {0x100357520u, 1776u, "cSC4PickSimWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x100357c10u, 342u, "cSC4PickSimWinProc::ValidateText()"},
    {0x100357d66u, 136u, "cSC4PickSimWinProc::EndDialog()"},
    {0x100357deeu, 1094u, "cSC4PickSimWinProc::MoveIn()"},
    {0x100358234u, 138u, "cSC4PickSimWinProc::SortByGender(bool)"},
    {0x1003582beu, 184u, "cSC4PickSimWinProc::SortBySign(bool)"},
    {0x100358376u, 271u, "cSC4PickSimWinProc::SortByName(bool)"},
    {0x100358576u, 50u, "cSC4PickSimWinProc::DoMessage(cIGZMessage2*)"},

    // cSC4WinCalloutBox (14 entry points)
    {0x1003c5e30u, 300u, "cSC4WinCalloutBox::cSC4WinCalloutBox()"},
    {0x1003c6218u, 364u, "cSC4WinCalloutBox::Init()"},
    {0x1003c6396u, 224u, "cSC4WinCalloutBox::Shutdown()"},
    {0x1003c6484u, 218u, "cSC4WinCalloutBox::SetBackgroundImage(cIGZBuffer*, int, int)"},
    {0x1003c656cu, 96u, "cSC4WinCalloutBox::SetTitle(cIGZString const&)"},
    {0x1003c65dau, 96u, "cSC4WinCalloutBox::SetText(cIGZString const&)"},
    {0x1003c6648u, 118u, "cSC4WinCalloutBox::SetTitleFontStyle(unsigned int)"},
    {0x1003c66beu, 118u, "cSC4WinCalloutBox::SetTextFontStyle(unsigned int)"},
    {0x1003c6734u, 174u, "cSC4WinCalloutBox::SetIcon(cIGZBuffer*)"},
    {0x1003c67f0u, 302u, "cSC4WinCalloutBox::SetIconMeter(cIGZBuffer*, unsigned int, float)"},
    {0x1003c691eu, 60u, "cSC4WinCalloutBox::SetPointOfInterest(cRZPoint const&)"},
    {0x1003c6968u, 92u, "cSC4WinCalloutBox::SetAvoidanceRect(cRZRect const&)"},
    {0x1003c69d2u, 92u, "cSC4WinCalloutBox::SetConstraintRect(cRZRect const&)"},
    {0x1003c6a3cu, 6100u, "cSC4WinCalloutBox::GZPaint()"},

    // cS3DModelInstance (13 entry points)
    {0x1000d8966u, 214u, "cS3DModelInstance::cS3DModelInstance()"},
    {0x1000d8a3cu, 212u, "cS3DModelInstance::cS3DModelInstance()"},
    {0x1000d8c3eu, 32u, "cS3DModelInstance::operator new(unsigned long)"},
    {0x1000d8ca2u, 50u, "cS3DModelInstance::SetDrawAttributes(SC4DrawAttributes&)"},
    {0x1000d8d82u, 78u, "cS3DModelInstance::SetOwner(cIGZUnknown*)"},
    {0x1000d8e6cu, 116u, "cS3DModelInstance::GetBounds(cS3DBoundingBox&)"},
    {0x1000d8ee0u, 46u, "cS3DModelInstance::RecalculateBounds()"},
    {0x1000d8f20u, 130u, "cS3DModelInstance::GetWorldBounds(cS3DBoundingBox&)"},
    {0x1000d8fb4u, 98u, "cS3DModelInstance::GetTransform(cS3DTransform&)"},
    {0x1000d9028u, 100u, "cS3DModelInstance::SetTransform(cS3DTransform const&)"},
    {0x1000d909eu, 32u, "cS3DModelInstance::Pick(cSC4PickContext*)"},
    {0x1000d90e4u, 50u, "cS3DModelInstance::Draw(SC4DrawContext*, bool)"},
    {0x1000d91eau, 172u, "cS3DModelInstance::SetModelResource(cIS3DResModel*)"},

    // cFreshCellPlayer (13 entry points)
    {0x10043ac9au, 82u, "cFreshCellPlayer::Create()"},
    {0x1004cb5b6u, 136u, "cFreshCellPlayer::cFreshCellPlayer()"},
    {0x1004cb648u, 144u, "cFreshCellPlayer::cFreshCellPlayer(cFreshPlayer*)"},
    {0x1004cb76eu, 226u, "cFreshCellPlayer::Update()"},
    {0x1004cb85cu, 332u, "cFreshCellPlayer::FilenameHasGuid(cIGZString&)"},
    {0x1004cb9a8u, 1592u, "cFreshCellPlayer::Start()"},
    {0x1004cbfe0u, 1032u, "cFreshCellPlayer::StartCellExternal(cFreshCell*)"},
    {0x1004cc3e8u, 344u, "cFreshCellPlayer::Stop()"},
    {0x1004cc540u, 84u, "cFreshCellPlayer::StopCellExternal(cFreshCell*)"},
    {0x1004cc594u, 52u, "cFreshCellPlayer::Kill()"},
    {0x1004cc5c8u, 102u, "cFreshCellPlayer::SetVol(int)"},
    {0x1004cc62eu, 422u, "cFreshCellPlayer::AutoPan()"},
    {0x1004cc7d4u, 78u, "cFreshCellPlayer::SetExternalSoundSystemInterface(cIExternalSoundSystem*)"},

    // cGroupLayer (12 entry points)
    {0x10040fccau, 68u, "cGroupLayer::cGroupLayer(float)"},
    {0x10040fd16u, 16u, "cGroupLayer::Shutdown()"},
    {0x10040fd26u, 132u, "cGroupLayer::Clear(bool)"},
    {0x10040fe2eu, 112u, "cGroupLayer::ClearEmptyGroups()"},
    {0x10040fe9eu, 14u, "cGroupLayer::InsertInstance(cS3DVector3&, float)"},
    {0x10040feacu, 308u, "cGroupLayer::_AddPositionToGroup(cS3DVector3&, float)"},
    {0x10040ffe0u, 156u, "cGroupLayer::InsertInstance(unsigned int, cS3DVector3&, float)"},
    {0x1004100deu, 318u, "cGroupLayer::_AddInstanceToGroup(cInstance*)"},
    {0x10041021cu, 98u, "cGroupLayer::RemoveInstance(unsigned int)"},
    {0x1004102ceu, 58u, "cGroupLayer::UpdateInstance(unsigned int, cS3DVector3&, float)"},
    {0x100410308u, 450u, "cGroupLayer::Intersect(cGroupLayer*, cGroupLayer*, bool)"},
    {0x1004104cau, 228u, "cGroupLayer::GetPositions(SC4Rect<float>&, std::__1::list<cS3DVector3, std::__1::allocator<cS3DVector3>>&, int&, int&, bool)"},

    // cSC4DynamicParticleEffect (12 entry points)
    {0x1006fe9e8u, 100u, "cSC4DynamicParticleEffect::InitDynamicParticleSystem()"},
    {0x1006fea4cu, 46u, "cSC4DynamicParticleEffect::ShutdownDynamicParticleSystem()"},
    {0x1006fea7au, 238u, "cSC4DynamicParticleEffect::cSC4DynamicParticleEffect()"},
    {0x1006fecc6u, 846u, "cSC4DynamicParticleEffect::Start(cISC4VisualEffect::tTransitionType)"},
    {0x1006ff014u, 158u, "cSC4DynamicParticleEffect::Stop(cISC4VisualEffect::tTransitionType)"},
    {0x1006ff0beu, 184u, "cSC4DynamicParticleEffect::SetTransforms(cS3DTransform const&, cS3DTransform const&)"},
    {0x1006ff176u, 722u, "cSC4DynamicParticleEffect::ApplyEffect(float)"},
    {0x1006ff474u, 102u, "cSC4DynamicParticleEffect::SetVoidParams(cISC4VisualEffect::tParamType, void**, int)"},
    {0x1006ff4dau, 192u, "cSC4DynamicParticleEffect::AttachSlaveEffect(cISC4VisualEffect*, cS3DTransform*)"},
    {0x1006ff59au, 84u, "cSC4DynamicParticleEffect::SetModel(cIS3DModelInstance*)"},
    {0x1006ff5eeu, 204u, "cSC4DynamicParticleEffect::SetModel(unsigned int)"},
    {0x1006ff6c6u, 348u, "cSC4DynamicParticleEffect::SetDescription(cSC4DynamicParticleDescription*)"},

    // cSCLua (11 entry points)
    {0x1005e669cu, 140u, "cSCLua::cSCLua()"},
    {0x1005e681cu, 302u, "cSCLua::Open(int)"},
    {0x1005e6966u, 206u, "cSCLua::Close()"},
    {0x1005e72cau, 188u, "cSCLua::GetN(int)"},
    {0x1005e766eu, 1060u, "cSCLua::OnGameDataRegistryNotification(cISC4GameDataRegistry*, cISC4GameDataRegistry::eNotficationType, unsigned long)"},
    {0x1005e7a92u, 1506u, "cSCLua::GDRCallExtention(lua_State*)"},
    {0x1005e8074u, 1208u, "cSCLua::GDRExchangeExtention(lua_State*)"},
    {0x1005e852cu, 102u, "cSCLua::GDRUnregisterNodeVisitor(unsigned long, void*)"},
    {0x1005e85a4u, 166u, "cSCLua::ToDataRegistryEntryID(cISC4GameDataRegistry*, int)"},
    {0x1005e8658u, 86u, "cSCLua::ToDataRegistryNode(cISC4GameDataRegistry*, int)"},
    {0x1005e86bcu, 70u, "cSCLua::ToDataRegistryValue(cISC4GameDataRegistry*, int)"},

    // cSC4ViewInputControl (10 entry points)
    {0x1000a94f8u, 172u, "cSC4ViewInputControl::Init()"},
    {0x1000a95a4u, 98u, "cSC4ViewInputControl::Shutdown()"},
    {0x1000a9624u, 106u, "cSC4ViewInputControl::SetCursor(cIGZCursor*)"},
    {0x1000a968eu, 80u, "cSC4ViewInputControl::SetCursor(unsigned int)"},
    {0x1000a96e8u, 26u, "cSC4ViewInputControl::EndInput()"},
    {0x1000a9752u, 50u, "cSC4ViewInputControl::PlayResultSound(int)"},
    {0x1000a97bau, 46u, "cSC4ViewInputControl::FinalRelease()"},
    {0x1000a980au, 34u, "cSC4ViewInputControl::SetCapture()"},
    {0x1000a984eu, 48u, "cSC4ViewInputControl::AmCapturing()"},
    {0x1000a987eu, 36u, "cSC4ViewInputControl::IsOnTop()"},

    // SC4MarkedCellView (10 entry points)
    {0x1003537f0u, 120u, "void SC4MarkedCellView::AddMarkedCells<SC4Point<int>>(std::__1::vector<SC4Point<int>, std::__1::allocator<SC4Point<int>>> const&)"},
    {0x1003cd402u, 186u, "void SC4MarkedCellView::AddMarkedCellRegion<int>(SC4CellRegion<int> const&)"},
    {0x10077b62cu, 136u, "SC4MarkedCellView::SC4MarkedCellView()"},
    {0x10077b7b8u, 364u, "SC4MarkedCellView::Init()"},
    {0x10077b924u, 50u, "SC4MarkedCellView::Shutdown()"},
    {0x10077b956u, 276u, "SC4MarkedCellView::Update(SC4DrawContext*)"},
    {0x10077ba6au, 2122u, "SC4MarkedCellView::DrawCells(SC4DrawContext*, unsigned int const*)"},
    {0x10077c2d2u, 154u, "SC4MarkedCellView::SetCellColor(float const*)"},
    {0x10077c36cu, 162u, "SC4MarkedCellView::SetCellColorDepthInvert(float const*)"},
    {0x10077c45cu, 58u, "SC4MarkedCellView::AddMarkedCell(int, int)"},

    // cSC4ViewInputControlLevelTerrain (9 entry points)
    {0x1003b7470u, 100u, "cSC4ViewInputControlLevelTerrain::ComputeAndMarkSelectedArea()"},
    {0x1003b74d4u, 60u, "cSC4ViewInputControlLevelTerrain::cSC4ViewInputControlLevelTerrain()"},
    {0x1003b7510u, 104u, "cSC4ViewInputControlLevelTerrain::Init()"},
    {0x1003b7578u, 148u, "cSC4ViewInputControlLevelTerrain::LevelTerrain()"},
    {0x1003b760cu, 14u, "cSC4ViewInputControlLevelTerrain::OnKeyDown(unsigned int, unsigned int)"},
    {0x1003b761au, 150u, "cSC4ViewInputControlLevelTerrain::OnMouseDownL(int, int, unsigned int)"},
    {0x1003b76b0u, 142u, "cSC4ViewInputControlLevelTerrain::OnMouseMove(int, int, unsigned int)"},
    {0x1003b773eu, 160u, "cSC4ViewInputControlLevelTerrain::OnMouseUpL(int, int, unsigned int)"},
    {0x1003b77deu, 106u, "cSC4ViewInputControlLevelTerrain::Shutdown()"},

    // MacPreferences (9 entry points)
    {0x100641b5cu, 154u, "MacPreferences::GetString(char const*, char*, int, char const*)"},
    {0x100641bf6u, 93u, "MacPreferences::PutString(char const*, char const*)"},
    {0x100641c53u, 107u, "MacPreferences::GetInteger(char const*, int)"},
    {0x100641cbeu, 103u, "MacPreferences::PutInteger(char const*, int)"},
    {0x100641d25u, 124u, "MacPreferences::GetFloat(char const*, float)"},
    {0x100641da1u, 20u, "MacPreferences::GetBoolean(char const*, bool)"},
    {0x100641db5u, 103u, "MacPreferences::PutBoolean(char const*, bool)"},
    {0x100641e1cu, 352u, "MacPreferences::GetRect(char const*, CGRect&)"},
    {0x100641f7cu, 441u, "MacPreferences::PutRect(char const*, CGRect const&)"},

    // cSC4WinLoadHeightField (8 entry points)
    {0x1003d1e8eu, 96u, "cSC4WinLoadHeightField::cSC4WinLoadHeightField()"},
    {0x1003d1f5cu, 246u, "cSC4WinLoadHeightField::DoMessage(cGZMessage&)"},
    {0x1003d2074u, 68u, "cSC4WinLoadHeightField::GetInitialDirectory(cRZString&)"},
    {0x1003d20b8u, 64u, "cSC4WinLoadHeightField::SetInitialDirectory(cRZString const&)"},
    {0x1003d20f8u, 984u, "cSC4WinLoadHeightField::Init()"},
    {0x1003d24d0u, 70u, "cSC4WinLoadHeightField::Shutdown()"},
    {0x1003d2516u, 110u, "cSC4WinLoadHeightField::GZPaint()"},
    {0x1003d2584u, 72u, "cSC4WinLoadHeightField::GZOnKeyDown(unsigned int, unsigned int)"},

    // cSC4WinRegionClouds (7 entry points)
    {0x1003b4380u, 316u, "cSC4WinRegionClouds::cSC4WinRegionClouds()"},
    {0x1003b45e0u, 394u, "cSC4WinRegionClouds::Init()"},
    {0x1003b476au, 46u, "cSC4WinRegionClouds::Shutdown()"},
    {0x1003b47a8u, 324u, "cSC4WinRegionClouds::SetCloudArea(float, float, float, float)"},
    {0x1003b48ecu, 240u, "cSC4WinRegionClouds::InitCloud(cSC4WinRegionClouds::cCloud&)"},
    {0x1003b49dcu, 68u, "cSC4WinRegionClouds::SetDriftVector(float, float)"},
    {0x1003b4a34u, 1002u, "cSC4WinRegionClouds::GZPaint()"},

    // AspyrDXTTools (7 entry points)
    {0x10047924cu, 332u, "AspyrDXTTools::GetColorBlockColors(AspyrDXTTools::DXTColBlock*, AspyrDXTTools::Color8888*, AspyrDXTTools::Color8888*, AspyrDXTTools::Color8888*, AspyrDXTTools::Color8888*, bool)"},
    {0x100479398u, 138u, "AspyrDXTTools::DecodeAlphaExplicit(unsigned int*, AspyrDXTTools::DXTAlphaBlockExplicit*, int, int, unsigned int)"},
    {0x100479422u, 218u, "AspyrDXTTools::DecodeColorBlock(unsigned int*, AspyrDXTTools::DXTColBlock*, int, int, unsigned int*, unsigned int*, unsigned int*, unsigned int*)"},
    {0x1004794fcu, 305u, "AspyrDXTTools::DecompressDXT1_8888(unsigned char*, unsigned int, unsigned int, unsigned int)"},
    {0x10047962du, 369u, "AspyrDXTTools::DecompressDXT3_8888(unsigned char*, unsigned int, unsigned int, unsigned int)"},
    {0x10047979eu, 369u, "AspyrDXTTools::DecompressDXT5_8888(unsigned char*, unsigned int, unsigned int, unsigned int)"},
    {0x10047990fu, 737u, "AspyrDXTTools::DecodeAlpha3BitLinear(unsigned int*, AspyrDXTTools::DXTAlphaBlock3BitLinear*, int, int, unsigned int)"},

    // cSC4TimeDisplayMiniWindowWinProc (6 entry points)
    {0x1002f2c10u, 214u, "cSC4TimeDisplayMiniWindowWinProc::cSC4TimeDisplayMiniWindowWinProc()"},
    {0x1002f2e1au, 218u, "cSC4TimeDisplayMiniWindowWinProc::Shutdown()"},
    {0x1002f2ef4u, 138u, "cSC4TimeDisplayMiniWindowWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x1002f2f7eu, 210u, "cSC4TimeDisplayMiniWindowWinProc::EndDialog()"},
    {0x1002f3050u, 334u, "cSC4TimeDisplayMiniWindowWinProc::UpdateTimeDisplay()"},
    {0x1002f319eu, 674u, "cSC4TimeDisplayMiniWindowWinProc::DoModelessWin()"},

    // cSC4WinRCI (6 entry points)
    {0x100464bfeu, 106u, "cSC4WinRCI::cSC4WinRCI()"},
    {0x100464e14u, 82u, "cSC4WinRCI::Init()"},
    {0x100464e78u, 82u, "cSC4WinRCI::Shutdown()"},
    {0x100464f14u, 322u, "cSC4WinRCI::RethinkBar()"},
    {0x100465056u, 108u, "cSC4WinRCI::DoMessage(cIGZMessage2*)"},
    {0x1004650d6u, 728u, "cSC4WinRCI::GZPaint()"},

    // cSLBuffer (5 entry points)
    {0x10004367cu, 70u, "cSLBuffer::cSLBuffer(bool)"},
    {0x1000436c2u, 158u, "cSLBuffer::cSLBuffer(cGZPersistResourceKey const&, cGZBufferColorType::eColorType, bool)"},
    {0x100043760u, 1134u, "cSLBuffer::Load()"},
    {0x100043bdcu, 150u, "cSLBuffer::cSLBuffer(unsigned int, unsigned int, unsigned int, cGZBufferColorType::eColorType, bool)"},
    {0x100043c80u, 188u, "cSLBuffer::cSLBuffer(cRZString const&, cGZBufferColorType::eColorType, bool)"},

    // MatrixRow (5 entry points)
    {0x10025d912u, 328u, "MatrixRow<cIGZWinGrid::CellFormatExtra>::MatrixRow(MatrixRow<cIGZWinGrid::CellFormatExtra> const&)"},
    {0x10025def0u, 284u, "MatrixRow<cIGZWinGrid::CellData*>::MatrixRow(MatrixRow<cIGZWinGrid::CellData*> const&)"},
    {0x10054b3a0u, 302u, "MatrixRow<WinGridCellData>::MatrixRow(MatrixRow<WinGridCellData> const&)"},
    {0x10054b896u, 284u, "MatrixRow<WinGridColData>::MatrixRow(MatrixRow<WinGridColData> const&)"},
    {0x10054bd7au, 284u, "MatrixRow<WinGridRowData>::MatrixRow(MatrixRow<WinGridRowData> const&)"},

    // cS3DResModel (5 entry points)
    {0x100468b86u, 886u, "cS3DResModel::GetModelFrame(unsigned int, cS3DModelData*) const"},
    {0x10046905au, 308u, "cS3DResModel::GetModelBounds(cS3DBoundingBox&) const"},
    {0x10046918eu, 154u, "cS3DResModel::GetModelInstance(unsigned int, cIS3DModelInstance**)"},
    {0x100469228u, 144u, "cS3DResModel::GetModelInstance(unsigned int, cIS3DModelInstanceAnimated**)"},
    {0x1004692b8u, 124u, "cS3DResModel::GetMeshAndMaterial(unsigned int, cIS3DMeshAndMaterial**)"},

    // cSLTokenResourceFactory (5 entry points)
    {0x1004e7ee0u, 42u, "cSLTokenResourceFactory::cSLTokenResourceFactory()"},
    {0x1004e7f42u, 72u, "cSLTokenResourceFactory::CreateInstance(unsigned int, unsigned int, void**)"},
    {0x1004e7f8au, 222u, "cSLTokenResourceFactory::SerialRead(cIGZPersistResource&, cIGZPersistDBSerialRecord&)"},
    {0x1004e8068u, 230u, "cSLTokenResourceFactory::SerialWrite(cIGZPersistResource&, cIGZPersistDBSerialRecord&)"},
    {0x1004e814eu, 50u, "cSLTokenResourceFactory::ReadCookie(cIGZPersistDBSerialRecord&)"},

    // cFontCOMDirector (5 entry points)
    {0x10068eca2u, 140u, "cFontCOMDirector::cFontCOMDirector()"},
    {0x10068ed2eu, 56u, "cFontCOMDirector::CreateFontSys()"},
    {0x10068ed66u, 56u, "cFontCOMDirector::CreateFontA()"},
    {0x10068ed9eu, 78u, "cFontCOMDirector::CreateBufferFontRenderer()"},
    {0x10068edecu, 110u, "cFontCOMDirector::OnStart(cIGZCOM*)"},

    // cSC4ToolTipManager (5 entry points)
    {0x1007930bau, 138u, "cSC4ToolTipManager::cSC4ToolTipManager()"},
    {0x10079327cu, 692u, "cSC4ToolTipManager::Init(cIGZFont*, signed char, signed char)"},
    {0x100793530u, 116u, "cSC4ToolTipManager::static_tip_timer_callback(void*)"},
    {0x1007935a4u, 310u, "cSC4ToolTipManager::Shutdown()"},
    {0x1007936dau, 982u, "cSC4ToolTipManager::DoMessage(cGZMessage&)"},

    // cSC4PipeTool (4 entry points)
    {0x1001930c0u, 494u, "cSC4PipeTool::CreateNetworkOccupant(cSC4NetworkCellInfo*, cSC4UndergroundOccupant*&, float (*) [3], unsigned int)"},
    {0x1001932aeu, 66u, "cSC4PipeTool::cSC4PipeTool()"},
    {0x1001932f0u, 274u, "cSC4PipeTool::FindPipeOccupant(int, int, cSC4PipeOccupant*&)"},
    {0x100193402u, 274u, "cSC4PipeTool::FindPipeConnectionOccupant(int, int, cSC4PipeConnectionOccupant*&)"},

    // cSCEditValueDlg (4 entry points)
    {0x10035f040u, 92u, "cSCEditValueDlg::cSCEditValueDlg(cISCProperty*)"},
    {0x1003e08a0u, 88u, "cSCEditValueDlg::GZPaint()"},
    {0x1003e08f8u, 1620u, "cSCEditValueDlg::Init()"},
    {0x1003e0f6cu, 1810u, "cSCEditValueDlg::doButtonPress(unsigned int)"},

    // cHitSegmentTrackCreator (4 entry points)
    {0x10043b468u, 288u, "cHitSegmentTrackCreator::cHitSegmentTrackCreator()"},
    {0x10043b602u, 552u, "cHitSegmentTrackCreator::Create(unsigned int, unsigned int, cIGZVariant*, unsigned int, void**)"},
    {0x10043b862u, 70u, "cHitSegmentTrackCreator::RegisterSoundDef(unsigned int, cIGZString const&, unsigned int, cIGZVariant const*)"},
    {0x10043b8a8u, 110u, "cHitSegmentTrackCreator::RegisterTrackDefParser(cIGZSndDefParser*)"},

    // cHitTrackDataBlock (4 entry points)
    {0x1004c1030u, 752u, "cHitTrackDataBlock::SetFilename(cIGZString&)"},
    {0x1004c244au, 50u, "cHitTrackDataBlock::GetEntryPointOffset(unsigned int, unsigned int&)"},
    {0x1004c247cu, 28u, "cHitTrackDataBlock::GetFilename(cIGZString&)"},
    {0x1004c24a8u, 38u, "cHitTrackDataBlock::AddEntryPoint(unsigned int, unsigned int)"},

    // SND (4 entry points)
    {0x1008f55c6u, 258u, "SND::decodexac(SND::MXAPACKETF*)"},
    {0x1008f56c8u, 14u, "SND::CEAXABLKDecf::operator new(unsigned long)"},
    {0x1008f56d6u, 14u, "SND::CEAXABLKDecf::operator delete(void*)"},
    {0x1008f5738u, 378u, "SND::CEAXABLKDecf::Decode(float**, int)"},

    // SC4Rect (3 entry points)
    {0x100212e68u, 76u, "SC4Rect<int>::UnionRect(SC4Rect<int> const&, SC4Rect<int> const&)"},
    {0x100314ce8u, 119u, "SC4Rect<int>::IntersectRect(SC4Rect<int> const&, SC4Rect<int> const&)"},
    {0x1007584e4u, 142u, "SC4Rect<float>::IntersectRect(SC4Rect<float> const&, SC4Rect<float> const&)"},

    // MiscGraphTextInfo (3 entry points)
    {0x10037be12u, 308u, "MiscGraphTextInfo::Init()"},
    {0x10037bf46u, 86u, "MiscGraphTextInfo::SetFont(cIGZFont*, unsigned int)"},
    {0x10037bf9cu, 246u, "MiscGraphTextInfo::DrawSelf(cIGZGraph*, cIGZBuffer*)"},

    // cFreshCell (3 entry points)
    {0x10043ac62u, 56u, "cFreshCell::Create()"},
    {0x1004c9fe0u, 156u, "cFreshCell::cFreshCell()"},
    {0x1004cc904u, 18u, "cFreshCell::SetFilename(cIGZString&)"},

    // cHitControlGroup (3 entry points)
    {0x1004c062au, 160u, "cHitControlGroup::AddTrack(int)"},
    {0x1004c2282u, 18u, "cHitControlGroup::SetName(cIGZString const&)"},
    {0x1004c2294u, 28u, "cHitControlGroup::GetName(cIGZString&) const"},

    // cSC16ColumnGridEntryView (3 entry points)
    {0x100526a1eu, 86u, "cSC16ColumnGridEntryView::Create()"},
    {0x100526a74u, 2502u, "cSC16ColumnGridEntryView::Init()"},
    {0x10052745au, 1222u, "cSC16ColumnGridEntryView::GetValue(cIGZVariant&)"},

    // cSC4WinAlertBorder (3 entry points)
    {0x1005c3d04u, 248u, "cSC4WinAlertBorder::cSC4WinAlertBorder()"},
    {0x1005c3facu, 204u, "cSC4WinAlertBorder::GZPaint()"},
    {0x1005c4078u, 136u, "cSC4WinAlertBorder::SetImage(cIGZBuffer*)"},

    // cSC4WinGridPopupStringListMenu (2 entry points)
    {0x10003ac1au, 44u, "cSC4WinGridPopupStringListMenu::GZOnKeyDown(unsigned int, unsigned int)"},
    {0x10003ac46u, 164u, "cSC4WinGridPopupStringListMenu::GZOnMouseUpL(int, int, unsigned int)"},

    // FileChangeNotification (2 entry points)
    {0x10004428eu, 62u, "FileChangeNotification::FileChangeNotification()"},
    {0x10004430au, 26u, "FileChangeNotification::GetWatchedDirectory(cIGZString&) const"},

    // cVecRange (2 entry points)
    {0x1000eee9cu, 540u, "cVecRange::cVecRange(std::__1::list<cSC4HistoryNode, std::__1::allocator<cSC4HistoryNode>>&, cIGZDate const&, cIGZDate const&, bool)"},
    {0x1000ef0b8u, 494u, "cVecRange::Coalesce(unsigned int, cRZDate const&, cRZDate const&, cISC4HistoryWarehouse::eCoalesce)"},

    // cEaseSound (2 entry points)
    {0x1003ba9aau, 50u, "cEaseSound::SetPosition(cS3DVector3&, bool)"},
    {0x1003baa14u, 216u, "cEaseSound::Update(float)"},

    // cAmbienceFilterIsZoom4 (2 entry points)
    {0x1003defb2u, 38u, "cAmbienceFilterIsZoom4::Create()"},
    {0x1003e48fcu, 46u, "cAmbienceFilterIsZoom4::IsTrue()"},

    // cAmbienceFilterIsPipeView (2 entry points)
    {0x1003df070u, 38u, "cAmbienceFilterIsPipeView::Create()"},
    {0x1003e49bcu, 32u, "cAmbienceFilterIsPipeView::IsTrue()"},

    // cAmbienceFilterIsSimSpeedSlow (2 entry points)
    {0x1003df108u, 38u, "cAmbienceFilterIsSimSpeedSlow::Create()"},
    {0x1003e4a52u, 30u, "cAmbienceFilterIsSimSpeedSlow::IsTrue()"},

    // cAmbienceCellFilterIsEmpty (2 entry points)
    {0x1003df7aeu, 50u, "cAmbienceCellFilterIsEmpty::Create()"},
    {0x1003e5c8cu, 114u, "cAmbienceCellFilterIsEmpty::IsTrueCell(int, int)"},

    // cAmbienceCellFilterIsPipe (2 entry points)
    {0x1003df876u, 50u, "cAmbienceCellFilterIsPipe::Create()"},
    {0x1003e5f42u, 116u, "cAmbienceCellFilterIsPipe::IsTrueCell(int, int)"},

    // cAmbienceCellFilterIsIndustrial (2 entry points)
    {0x1003df93eu, 50u, "cAmbienceCellFilterIsIndustrial::Create()"},
    {0x1003e60dcu, 94u, "cAmbienceCellFilterIsIndustrial::IsTrueCell(int, int)"},

    // cAmbienceCellFilterIsContruction (2 entry points)
    {0x1003dfa38u, 50u, "cAmbienceCellFilterIsContruction::Create()"},
    {0x1003e62aau, 114u, "cAmbienceCellFilterIsContruction::IsTrueCell(int, int)"},

    // cAmbienceCellFilterIsHighCrime (2 entry points)
    {0x1003dfb00u, 50u, "cAmbienceCellFilterIsHighCrime::Create()"},
    {0x1003e6784u, 110u, "cAmbienceCellFilterIsHighCrime::IsTrueCell(int, int)"},

    // cAmbienceCellFilterIsMediumDesirability (2 entry points)
    {0x1003dfbc8u, 50u, "cAmbienceCellFilterIsMediumDesirability::Create()"},
    {0x1003e693eu, 112u, "cAmbienceCellFilterIsMediumDesirability::IsTrueCell(int, int)"},

    // cAmbienceCellFilterIsHighway (2 entry points)
    {0x1003dfc90u, 50u, "cAmbienceCellFilterIsHighway::Create()"},
    {0x1003e5de6u, 116u, "cAmbienceCellFilterIsHighway::IsTrueCell(int, int)"},

    // cAmbienceCellFilterIsIndustrialClean (2 entry points)
    {0x1003dfd58u, 50u, "cAmbienceCellFilterIsIndustrialClean::Create()"},
    {0x1003e6d4au, 354u, "cAmbienceCellFilterIsIndustrialClean::IsTrueCell(int, int)"},

    // cAmbienceCellFilterIsLowWealth (2 entry points)
    {0x1003dfe20u, 50u, "cAmbienceCellFilterIsLowWealth::Create()"},
    {0x1003e631cu, 114u, "cAmbienceCellFilterIsLowWealth::IsTrueCell(int, int)"},

    // SC4Percentage (2 entry points)
    {0x1003f4ab0u, 64u, "SC4Percentage::SC4Percentage(float, bool)"},
    {0x1003f7db0u, 96u, "SC4Percentage::SC4Percentage(float, bool)"},

    // cHitResource (2 entry points)
    {0x100473dceu, 326u, "cHitResource<cHitTrackPlayer>::CreatePrivateCopy(unsigned int, void**)"},
    {0x100475612u, 228u, "cHitResource<cHitList>::CreatePrivateCopy(unsigned int, void**)"},

    // cSCStringEntryView (2 entry points)
    {0x100523e12u, 226u, "cSCStringEntryView::Init()"},
    {0x100523f14u, 206u, "cSCStringEntryView::GetValue(cIGZVariant&)"},

    // cSCSint8TextEntryView (2 entry points)
    {0x100524ec6u, 420u, "cSCSint8TextEntryView::Init()"},
    {0x10052508au, 446u, "cSCSint8TextEntryView::GetValue(cIGZVariant&)"},

    // <free> (2 entry points)
    {0x10052e33au, 504u, "cSCColorSequenceEntryView::DoMessage(cGZMessage&)::cPositionSorter::go(cIGZWinGrid::CellData const*, cIGZWinGrid::CellData const*)"},
    {0x10072430au, 167u, "invocation function for block in CQTPlayer::Open(char const*)"},

    // cSC4SoundDescription (2 entry points)
    {0x100572390u, 110u, "cSC4SoundDescription::cSC4SoundDescription()"},
    {0x1005724b8u, 72u, "cSC4SoundDescription::Instantiate()"},

    // cSC4BrushDescription (2 entry points)
    {0x100575b1cu, 132u, "cSC4BrushDescription::cSC4BrushDescription()"},
    {0x100575c5au, 132u, "cSC4BrushDescription::Instantiate()"},

    // cSC4DynamicParticleDescription (2 entry points)
    {0x1006fd016u, 114u, "cSC4DynamicParticleDescription::operator=(cSC4DynamicParticleDescription const&)"},
    {0x1006fd088u, 80u, "cSC4DynamicParticleDescription::cSC4DynamicParticleDescription()"},

    // cSTEEdgeReconcileInformDamage (2 entry points)
    {0x10072710au, 380u, "cSTEEdgeReconcileInformDamage::DoDialog()"},
    {0x100727286u, 122u, "cSTEEdgeReconcileInformDamage::DoMessage(cIGZWin*, cGZMessage&)"},

    // cFramesetInfo (1 entry points)
    {0x1001a2aecu, 144u, "cFramesetInfo::IncrementFrame()"},

    // SignalPlatformData (1 entry points)
    {0x1001fbe26u, 106u, "SignalPlatformData::SignalPlatformData()"},

    // nsIGZLua5 (1 entry points)
    {0x100298208u, 92u, "nsIGZLua5::cStackPosSaver::cStackPosSaver(cIGZLua5Thread*)"},

    // cSC4PanelUIWinProc (1 entry points)
    {0x100358638u, 228u, "cSC4PanelUIWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},

    // cPylonFilter (1 entry points)
    {0x1003c36a0u, 210u, "cPylonFilter::IsOccupantIncluded(cISC4Occupant*)"},

    // TriangleRec (1 entry points)
    {0x10045f708u, 394u, "TriangleRec::TriangleRec(float, cS3DTransform const*, cS3DVector3 const*, cS3DVector3 const*, cS3DVector3 const*)"},

    // RZFileTimeSort (1 entry points)
    {0x1004c9230u, 315u, "RZFileTimeSort::operator()(cRZString const&, cRZString const&)"},

    // cS3DResSpriteAnim (1 entry points)
    {0x10054fef2u, 194u, "cS3DResSpriteAnim::GetModelInstance(cIS3DModelInstanceAnimated**)"},

    // cSC4EffectDescription (1 entry points)
    {0x1005c2800u, 367u, "cSC4EffectDescription::ReadVersion1(cIGZIStream&)"},

    // cLUAExtensionsCollectorUI (1 entry points)
    {0x10063696cu, 785u, "cLUAExtensionsCollectorUI::cLUAExtensionsCollectorUI()"},

    // CVertexArray (1 entry points)
    {0x10063ada0u, 56u, "CVertexArray::SetPointer(int, unsigned int, int, void const*)"},

    // ibasic_string (1 entry points)
    {0x100692d3au, 166u, "ibasic_string<char>::ibasic_string(char const*)"},

    // FastATIVertexCopiers (1 entry points)
    {0x1007519c3u, 99u, "FastATIVertexCopiers::CopyRowNoAltivec(float*, float const*, unsigned int)"},
}};

constexpr std::size_t Wave4Part2MethodCount() noexcept {
    return kWave4Part2Methods.size();
}

constexpr std::size_t Wave4Part2NamedCount() noexcept {
    std::size_t count = 0;
    for (const Wave4Part2Method& method : kWave4Part2Methods) {
        if (method.name != nullptr && method.address != 0) {
            ++count;
        }
    }
    return count;
}

constexpr bool Wave4Part2AddressesUnique() noexcept {
    // Radix sort, then an adjacent compare. The pairwise scan used by the
    // smaller Wave 3 table exceeds the default constexpr step limit here.
    std::array<Uint64, kWave4Part2Methods.size()> addresses{};
    for (std::size_t i = 0; i < addresses.size(); ++i) {
        addresses[i] = kWave4Part2Methods[i].address;
    }

    std::array<Uint64, addresses.size()> scratch{};
    for (unsigned shift = 0; shift < 32; shift += 8) {
        std::array<std::size_t, 256> count{};
        for (const Uint64 address : addresses) {
            ++count[static_cast<std::size_t>((address >> shift) & 0xffu)];
        }
        std::size_t sum = 0;
        for (std::size_t bucket = 0; bucket < count.size(); ++bucket) {
            const std::size_t n = count[bucket];
            count[bucket] = sum;
            sum += n;
        }
        for (const Uint64 address : addresses) {
            const std::size_t digit = static_cast<std::size_t>((address >> shift) & 0xffu);
            scratch[count[digit]] = address;
            ++count[digit];
        }
        addresses = scratch;
    }

    for (std::size_t i = 1; i < addresses.size(); ++i) {
        if (addresses[i] == addresses[i - 1]) {
            return false;
        }
    }
    return true;
}

constexpr const Wave4Part2Method* FindWave4Part2Method(Uint64 address) noexcept {
    for (const Wave4Part2Method& method : kWave4Part2Methods) {
        if (method.address == address) {
            return &method;
        }
    }
    return nullptr;
}

static_assert(Wave4Part2MethodCount() == 808u);
static_assert(Wave4Part2NamedCount() == 808u);
static_assert(Wave4Part2AddressesUnique());
static_assert(FindWave4Part2Method(0x100019768u) != nullptr);
static_assert(FindWave4Part2Method(0x1007519c3u) != nullptr);
static_assert(FindWave4Part2Method(0x100019768u)->sizeBytes == 60u);
static_assert(FindWave4Part2Method(0x1007519c3u)->sizeBytes == 99u);
} // namespace opensc4
