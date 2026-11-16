#pragma once

#include "opensc4/gzcom.h"
#include "opensc4/simgrid.h"
#include "opensc4/dbpf.h"
#include "opensc4/formats.h"
#include "opensc4/tract_developer.h"
#include "opensc4/budget_simulator.h"
#include "opensc4/civic_simulators.h"
#include "opensc4/sc4_subfiles.h"
#include "opensc4/sc4_subfiles_batch2.h"
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

namespace opensc4 {
namespace loaded_aging_transition { class State; }

// Integration outcomes over saved snapshots, not original ABI return values.
enum class SavedBuildingLotStatus { Unavailable, Unsupported, NoLot, Lot };
struct SavedBuildingLotResult {
    SavedBuildingLotStatus status = SavedBuildingLotStatus::Unavailable;
    Sint32 cellX = 0;
    Sint32 cellZ = 0;
    SimSerializableRef lotRef{};
    Uint8 savedFlagsCD = 0;
    // Meaningful only for NoLot/Lot. Saved flags do not establish current
    // original-game post-load power or simulation updates.
    bool savedPowered = false;
};

enum class ZoneType : Uint8 {
    None            = 0,
    ResidentialLow  = 1,
    ResidentialMed  = 2,
    ResidentialHigh = 3,
    CommercialLow   = 4,
    CommercialMed   = 5,
    CommercialHigh  = 6,
    IndustrialAg    = 7,
    IndustrialMed   = 8,
    IndustrialHigh  = 9,
    Military        = 10,
    Airport         = 11,
    Seaport         = 12,
    Spaceport       = 13,
    Landfill        = 14,
    PloppedCivic    = 15
};

enum class NetworkType : Uint8 {
    Road       = 0,
    Rail       = 1,
    Highway    = 2,
    Street     = 3,
    Pipe       = 4,
    PowerLine  = 5,
    Avenue     = 6,
    Subway     = 7,
    LightRail  = 8,
    Monorail   = 9,
    OneWayRoad = 10,
    DirtRoad   = 11
};

enum class ToolType : Uint8 {
    Inspect         = 0,
    ZoneResLow      = 1,
    ZoneResMed      = 2,
    ZoneResHigh     = 3,
    ZoneComLow      = 4,
    ZoneComMed      = 5,
    ZoneComHigh     = 6,
    ZoneIndAg       = 7,
    ZoneIndMed      = 8,
    ZoneIndHigh     = 9,
    BuildRoad       = 10,
    BuildStreet     = 11,
    BuildAvenue     = 12,
    BuildHighway    = 13,
    BuildRail       = 14,
    PlopPowerPlant  = 15,
    PlopWaterPump   = 16,
    PlopPolice      = 17,
    PlopFire        = 18,
    PlopPark        = 19,
    Bulldoze        = 20,
    TerraformRaise  = 21,
    TerraformLower  = 22
};

enum class DataOverlay : Uint8 {
    None        = 0,
    Zones       = 1,
    LandValue   = 2,
    Pollution   = 3,
    Crime       = 4,
    Traffic     = 5,
    Power       = 6,
    Water       = 7,
    MayorRating = 8
};

struct SC4LotRecord {
    Uint32 memAddr              = 0;
    Uint32 configInstanceId     = 0;
    Uint32 buildingExemplarId   = 0;
    Uint8 flags                 = 0; // 0x10=powered, 0x40=watered
    Uint8 minX                  = 0;
    Uint8 minZ                  = 0;
    Uint8 maxX                  = 0;
    Uint8 maxZ                  = 0;
    Uint8 sizeX                 = 1;
    Uint8 sizeZ                 = 1;
    Float32 altitudeY           = 250.0f;
    Float32 slope1              = 0.0f;
    Float32 slope2              = 0.0f;
    Uint8 facing                = 0;
    ZoneType zoneType           = ZoneType::None;
    Uint8 wealth                = 1; // 1=$, 2=$$, 3=$$$
    Uint32 capacity             = 0;
};

struct SC4BuildingRecord {
    Uint32 memAddr        = 0;
    cSC4TGI exemplarTGI{};
    std::string name;
    Uint32 s3dInstanceId  = 0;
    Uint32 s3dGroupId     = 0; // 0 = unknown; LoadAssetByTGI when set (park/plop models)
    Uint32 fshTextureId   = 0;
    Float32 minX          = 0.0f;
    Float32 minY          = 0.0f;
    Float32 minZ          = 0.0f;
    Float32 maxX          = 16.0f;
    Float32 maxY          = 10.0f;
    Float32 maxZ          = 16.0f;
    Uint8 orientation     = 0;
    ZoneType zoneHint     = ZoneType::None;
    Uint8 wealthHint      = 1;
};

struct SC4NetworkVertex {
    Float32 x    = 0.0f;
    Float32 y    = 0.0f;
    Float32 z    = 0.0f;
    Float32 u    = 0.0f;
    Float32 v    = 0.0f;
    Uint32 rgba  = 0xFFFFFFFFu;
};

struct SC4NetworkRecord {
    Uint32 memAddr          = 0;
    Float32 centerX         = 0.0f;
    Float32 centerY         = 0.0f;
    Float32 centerZ         = 0.0f;
    SC4NetworkVertex verts[4]{};
    Uint32 textureId        = 0;
    Uint8 orientation       = 0;
    Uint8 tileX             = 0;
    Uint8 tileZ             = 0;
    NetworkType networkType = NetworkType::Road;
    Uint32 networkTypeMask  = 1u; // One bit per cISC4NetworkManager::eNetworkType
};

struct SC4PropRecord {
    cSC4TGI exemplarTGI{};
    Float32 minX = 0.0f, minY = 0.0f, minZ = 0.0f;
    Float32 maxX = 1.0f, maxY = 1.0f, maxZ = 1.0f;
    Uint8 orientation = 0;
    Uint32 s3dInstanceId = 0; // Zoom 5, rotation 0; 0 when the exemplar names no S3D model
};

struct SC4FloraRecord {
    cSC4TGI exemplarTGI{};
    Float32 x = 0.0f;
    Float32 y = 0.0f;
    Float32 z = 0.0f;
    Uint8 orientation = 0;
    Uint32 s3dInstanceId = 0;
};

// One cell of a cSC4LotBaseTextureOccupant: an FSH ground texture laid on the terrain.
struct SC4LotTextureRecord {
    Uint32 textureId = 0; // Zoom 1 FSH instance; +4 is Zoom 5
    Uint8 cellX = 0;
    Uint8 cellZ = 0;
    Uint8 orientation = 0;
};

// A saved cSC4Vehicle / cSC4Pedestrian, positioned where the game left it.
struct SC4AutomatonRecord {
    enum class Kind : Uint8 { Vehicle, Pedestrian };
    Kind kind = Kind::Vehicle;
    Float32 x = 0.0f;
    Float32 y = 0.0f;
    Float32 z = 0.0f;
    Float32 headingX = 1.0f;
    Float32 headingZ = 0.0f;
    Float32 speed = 0.0f;
    // Vehicles: S3D model. Pedestrians: sprite animation (type 0x29a5d1ec).
    cSC4TGI model{};
};

struct RCIDemandState {
    Sint32 resLow   = 1200; // -6000 .. +6000
    Sint32 resMed   = 800;
    Sint32 resHigh  = 400;
    Sint32 comLow   = 900;
    Sint32 comMed   = 700;
    Sint32 comHigh  = 350;
    Sint32 indAg    = 600;
    Sint32 indMed   = 1100;
    Sint32 indHigh  = 500;
};

struct SC4DemandRecord {
    Uint32 memAddr = 0;
    Uint32 id = 0;
    Float32 supplyValue = 0.0f;
    Float32 demandValue = 0.0f;
    Float32 newSupply = 0.0f;
    Float32 newDemand = 0.0f;
    Float32 serializedValue2C = 0.0f;
    Float32 serializedValue30 = 0.0f;
    Float32 activeDemandMin = 0.0f;
    Float32 activeDemandMax = 0.0f;
    Float32 activeDemand = 0.0f;
    bool enabled = false;
    Float32 economyModifier = 0.0f;
    Float32 taxModifier = 0.0f;
    Float32 demandCap = 0.0f;

    // cSC4Demand::EndOfCycle followed by CalculateActiveDemand(false).
    // Returns true when this cycle contributed new supply or demand.
    bool EndCycleAndCalculate() noexcept;
};

// Events emitted by cSC4Simulator::SimulateTimePassage after advancing the
// calendar. Counts are relative to the current load/generation session.
struct SimulationCadenceState {
    Uint32 dailyEvents = 0;
    Uint32 weeklyEvents = 0;
    Uint32 monthlyEvents = 0;
    Uint32 yearlyEvents = 0;
    Uint32 lastWeekOfYear = 0;
};

struct TileQueryReport {
    int cellX = 0;
    int cellZ = 0;
    float elevationY = 250.0f;
    bool underwater = false;
    ZoneType zoneType = ZoneType::None;
    bool hasNetwork = false;
    NetworkType networkType = NetworkType::Road;
    bool hasBuilding = false;
    std::string buildingName;
    Uint8 buildingWealth = 0;
    Uint32 lotCapacity = 0;
    Uint8 landValue = 0;
    Uint8 pollution = 0;
    Uint8 crime = 0;
    Uint8 traffic = 0;
    Uint8 power = 0;
    Uint8 water = 0;
    Sint8 mayorRating = 0;
};

struct SC4CommuteRoute {
    Sint32 startLotIndex = -1;
    Sint32 destLotIndex = -1;
    Uint32 commuters = 0;
    Float32 tripLength = 0.0f;
    std::vector<Sint32> cellX;
    std::vector<Sint32> cellZ;
    std::vector<Uint8> compressedRoute;
};

struct BuildingDevelopmentState {
    Sint32 minimumConstructionResources = 100;
    Sint32 maximumConstructionResources = 2000;
    Float32 demandToResourceMultiplier = 0.5f;
    Uint32 idlePlanningSteps = 10;
    std::vector<Float32> declineThresholds;
    Float32 constructionResources = 0.0f;
    Uint32 dailyCycles = 0;
    Uint32 monthlyCycles = 0;
    Uint32 growthAttempts = 0;
    Uint32 growthExecutions = 0;
    Uint32 declinePlanCycles = 0;
    Uint32 declinePlansCreated = 0;
    Uint32 pendingDeclinePlans = 0;
    Uint32 declineExecutions = 0;
    Uint32 declinePlansDropped = 0;
    bool loadedTunablesFromExemplar = false;

    static bool IsGrowthDemandEligible(Float32 demand) noexcept {
        return static_cast<Sint32>(demand) >= 6;
    }
    static bool IsDeclinePlanningEligible(Float32 demand) noexcept {
        return static_cast<Sint32>(demand) <= -21;
    }
    static bool IsDeclineExecutionEligible(Float32 demand) noexcept {
        return static_cast<Sint32>(demand) <= -20;
    }

    Sint32 CalculateMonthlyResources(Float32 residentialDemand,
                                     Float32 commercialDemand,
                                     Float32 industrialDemand) const noexcept;
};

class cSC4City {
public:
    cSC4City();

    // Explicit imported runtime state; no serialized/native layout claim.
    std::shared_ptr<loaded_aging_transition::State>& LoadedAgingState() noexcept { return mLoadedAgingState; }
    const std::shared_ptr<loaded_aging_transition::State>& LoadedAgingState() const noexcept { return mLoadedAgingState; }

    // Load an authentic .sc4 DBPF city save from disk or memory
    bool LoadFromSC4File(const std::string& sc4Path, const cDBPFSystem* dbpfSys = nullptr);
    bool LoadFromSC4Archive(const cDBPFArchive& arc, const cDBPFSystem* dbpfSys = nullptr);

    // OpenGFX-style clean-room procedural city generator when no .sc4 is loaded
    void GenerateProceduralCity(Uint32 cellCountX = 128, Uint32 cellCountZ = 128, Uint32 seed = 0x53433400u);

    // Save current city state to a valid DBPF .sc4 archive buffer
    bool SaveToSC4Buffer(std::vector<Uint8>& outBuffer) const;
    // Clone a loaded save and patch only records dirtied by live city controls.
    bool SaveToSC4File(const std::string& path);

    // Advance cSC4Simulator by N simulated days
    void StepSimulation(int days = 1);

    // Apply player interactive tool on a tile rectangle
    bool ApplyTool(ToolType tool, int x0, int z0, int x1, int z1);

    // Accessors matching Maxis ISC4City interface
    const std::string& CityName() const noexcept { return mCityName; }
    const std::string& MayorName() const noexcept { return mMayorName; }
    const std::string& SourcePath() const noexcept { return mSourcePath; }
    bool IsFromOriginalSave() const noexcept { return mIsFromOriginalSave; }

    int CellCountX() const noexcept { return mCellCountX; }
    int CellCountZ() const noexcept { return mCellCountZ; }
    float CellWidthX() const noexcept { return 16.0f; }
    float CellWidthZ() const noexcept { return 16.0f; }
    float SeaLevel() const noexcept { return mSeaLevel; }

    Uint32 RegionTileX() const noexcept { return mRegionTileX; }
    Uint32 RegionTileZ() const noexcept { return mRegionTileZ; }

    Uint32 ResidentialPop() const noexcept { return mResidentialPop; }
    Uint32 CommercialPop() const noexcept { return mCommercialPop; }
    Uint32 IndustrialPop() const noexcept { return mIndustrialPop; }
    Uint32 TotalPop() const noexcept { return mResidentialPop; }
    Sint64 TreasuryFunds() const noexcept { return mFunds; }
    const BudgetSimulatorSaveRecord& BudgetSaveRecord() const noexcept { return mBudgetSaveRecord; }
    bool HasBudgetSaveRecord() const noexcept { return mHasBudgetSaveRecord; }
    const BudgetSimulatorTuning& BudgetTuning() const noexcept { return mBudgetTuning; }

    // Civic & utility simulator save records decoded from the .sc4 subfiles
    // (exact binary Read formats; see civic_simulators.h).
    const LandValueSimulatorSaveRecord& LandValueSaveRecord() const noexcept { return mLandValueSaveRecord; }
    const CommercialSimulatorSaveRecord& CommercialSaveRecord() const noexcept { return mCommercialSaveRecord; }
    const IndustrialSimulatorSaveRecord& IndustrialSaveRecord() const noexcept { return mIndustrialSaveRecord; }
    const FireProtectionSimulatorSaveRecord& FireProtectionSaveRecord() const noexcept { return mFireProtectionSaveRecord; }
    const PoliceSimulatorSaveRecord& PoliceSaveRecord() const noexcept { return mPoliceSaveRecord; }
    const FlammabilitySimulatorSaveRecord& FlammabilitySaveRecord() const noexcept { return mFlammabilitySaveRecord; }
    const CrimeSimulatorSaveRecord& CrimeSaveRecord() const noexcept { return mCrimeSaveRecord; }
    const AuraSimulatorSaveRecord& AuraSaveRecord() const noexcept { return mAuraSaveRecord; }
    const PowerSimulatorSaveRecord& PowerSaveRecord() const noexcept { return mPowerSaveRecord; }
    const PlumbingSimulatorSaveRecord& PlumbingSaveRecord() const noexcept { return mPlumbingSaveRecord; }
    const PollutionSimulatorSaveRecord& PollutionSaveRecord() const noexcept { return mPollutionSaveRecord; }
    const ResidentialSimulatorSaveRecord& ResidentialSaveRecord() const noexcept { return mResidentialSaveRecord; }
    const TrafficSimulatorSaveRecord& TrafficSaveRecord() const noexcept { return mTrafficSaveRecord; }
    const CivicBuildingSimulatorSaveRecord& CivicBuildingSaveRecord() const noexcept { return mCivicBuildingSaveRecord; }
    const FloraSimulatorSaveRecord& FloraSaveRecord() const noexcept { return mFloraSaveRecord; }
    const MySimAgentSimulatorSaveRecord& MySimAgentSaveRecord() const noexcept { return mMySimAgentSaveRecord; }
    const NeighborsSimulatorSaveRecord& NeighborsSaveRecord() const noexcept { return mNeighborsSaveRecord; }
    const OrdinanceSimulatorSaveRecord& OrdinanceSaveRecord() const noexcept { return mOrdinanceSaveRecord; }
    const WeatherSimulatorSaveRecord& WeatherSaveRecord() const noexcept { return mWeatherSaveRecord; }
    const SimulatorSaveRecord& SimulatorRecord() const noexcept { return mSimulatorSaveRecord; }
    const SimulatorTuning& SimulatorTuningValues() const noexcept { return mSimulatorTuning; }
    bool HasCivicSaveRecord(Uint32 clsid) const noexcept;

    Uint32 SimYear() const noexcept { return mYear; }
    Uint32 SimMonth() const noexcept { return mMonth; }
    Uint32 SimDay() const noexcept { return mDay; }
    Uint32 TotalSimDays() const noexcept { return mTotalDays; }
    Uint32 SimJulianDay() const noexcept { return mJulianDay; }
    const SimulationCadenceState& SimulationCadence() const noexcept { return mCadence; }
    const BuildingDevelopmentState& BuildingDevelopment() const noexcept {
        return mBuildingDevelopment;
    }

    const RCIDemandState& Demand() const noexcept { return mDemand; }
    const std::unordered_map<Uint32, Float32>& LoadedDemandValues() const noexcept {
        return mLoadedDemandValues;
    }
    const std::unordered_map<Uint32, SC4DemandRecord>& LoadedDemandRecords() const noexcept {
        return mLoadedDemandRecords;
    }
    Float32 LoadedDemandValue(Uint32 demandId) const noexcept {
        auto it = mLoadedDemandValues.find(demandId);
        return it != mLoadedDemandValues.end() ? it->second : 0.0f;
    }

    // Vertex height at (vx, vz) in [0..CellCountX] x [0..CellCountZ]
    float GetVertexHeight(int vx, int vz) const noexcept;
    void SetVertexHeight(int vx, int vz, float h) noexcept;
    float GetCellCenterHeight(int cx, int cz) const noexcept;
    bool IsCellUnderwater(int cx, int cz) const noexcept;

    const std::vector<float>& Heightmap() const noexcept { return mHeightmap; }
    const std::vector<SC4LotRecord>& Lots() const noexcept { return mLots; }
    const std::vector<SC4BuildingRecord>& Buildings() const noexcept { return mBuildings; }
    // Integration storage, not an original game lookup. Unavailable includes
    // absent/unreadable subfiles and failed canonical decoding; empty + available
    // means a successfully decoded empty subfile. Raw entries retain order/types.
    bool HasDecodedSavedBuildingOccupants() const noexcept {
        return mSavedBuildingOccupantsAvailable;
    }
    std::span<const BuildingOccupantSaveRecord> SavedBuildingOccupants() const noexcept {
        return mSavedBuildingOccupants;
    }
    // Null for invalid, absent or ambiguous identities; consult availability to
    // distinguish unavailable storage. Borrow valid until reload/reset/destruction
    // (or assignment/move of this City). No effective-property resolution.
    const BuildingOccupantSaveRecord* FindSavedBuildingOccupant(
        const SimSerializableRef& ref) const noexcept;
    // Exact position/cell arithmetic and saved grid lookup, with explicit
    // adapter-domain failures. No inferred lot from bounds or live-state claim.
    SavedBuildingLotResult QuerySavedBuildingLot(
        const SimSerializableRef& ref) const noexcept;
    const std::vector<SC4NetworkRecord>& Networks() const noexcept { return mNetworks; }
    const std::vector<SC4PropRecord>& Props() const noexcept { return mProps; }
    const std::vector<SC4FloraRecord>& Flora() const noexcept { return mFlora; }
    const std::vector<SC4LotTextureRecord>& LotTextures() const noexcept { return mLotTextures; }
    const std::vector<SC4AutomatonRecord>& Automata() const noexcept { return mAutomata; }
    const std::vector<Uint8>& PngThumbnail() const noexcept { return mPngThumbnail; }

    Uint32 PipeOccupantCount() const noexcept { return mPipeCount; }
    Uint32 SubwayOccupantCount() const noexcept { return mSubwayCount; }
    Uint32 PowerLineOccupantCount() const noexcept { return mPowerLineCount; }

    // Simulation grids (cSC4SimGrid<T>)
    const cSC4SimGrid<Uint8>& LandValueGrid() const noexcept { return mLandValueGrid; }
    const cSC4SimGrid<Uint8>& PollutionGrid() const noexcept { return mPollutionGrid; }
    const cSC4SimGrid<Uint8>& CrimeGrid() const noexcept { return mCrimeGrid; }
    const cSC4SimGrid<Uint8>& TrafficGrid() const noexcept { return mTrafficGrid; }
    const cSC4SimGrid<Uint8>& PowerGrid() const noexcept { return mPowerGrid; }
    const cSC4SimGrid<Uint8>& WaterGrid() const noexcept { return mWaterGrid; }
    const cSC4SimGrid<Sint8>& MayorRatingGrid() const noexcept { return mMayorRatingGrid; }

    const std::vector<cSC4SimGrid<Sint8>>& LoadedSint8Grids() const noexcept { return mLoadedSint8Grids; }
    const std::vector<cSC4SimGrid<Uint8>>& LoadedUint8Grids() const noexcept { return mLoadedUint8Grids; }
    const std::vector<cSC4SimGrid<Sint16>>& LoadedSint16Grids() const noexcept { return mLoadedSint16Grids; }
    const std::vector<cSC4SimGrid<Uint16>>& LoadedUint16Grids() const noexcept { return mLoadedUint16Grids; }
    const std::vector<cSC4SimGrid<Float32>>& LoadedFloat32Grids() const noexcept { return mLoadedFloat32Grids; }

    // Decompiled cSC4TractDeveloper, cSC4BuildingDevelopmentSimulator,
    // cSC4GrowthDeveloper, and cSC4DemandSimulator state loaded from .sc4 saves
    const TractDeveloperState& TractDeveloper() const noexcept { return mTractDeveloperState; }
    const TractDeveloperTuning& TractTuning() const noexcept { return mTractDeveloperTuning; }
    const BuildingDevelopmentSimSaveRecord& BuildingDevSimRecord() const noexcept { return mBuildingDevSimRecord; }
    const std::vector<GrowthDeveloperSaveRecord>& GrowthDeveloperRecords() const noexcept { return mGrowthDeveloperRecords; }
    const std::unordered_map<Uint32, GrowthDeveloperTuning>& GrowthDeveloperTunings() const noexcept { return mGrowthDeveloperTunings; }
    const std::unordered_map<Uint32, std::vector<GrowthDeveloperItem>>& GrowthDeveloperGrowthLists() const noexcept { return mGrowthDeveloperGrowthLists; }
    const std::unordered_map<Uint32, std::vector<GrowthDeveloperItem>>& GrowthDeveloperDeclineLists() const noexcept { return mGrowthDeveloperDeclineLists; }
    const DemandSimulatorSaveRecord& DemandSimRecord() const noexcept { return mDemandSimRecord; }
    const DemandSimulatorTuning& DemandSimTuning() const noexcept { return mDemandSimTuning; }
    const LotDeveloperTuning& LotDevTuning() const noexcept { return mLotDeveloperTuning; }
    const LotDeveloperSaveRecord& LotDevRecord() const noexcept { return mLotDeveloperRecord; }
    const LotConfigurationManagerSaveRecord& LotConfigManagerRecord() const noexcept { return mLotConfigManagerRecord; }

    ZoneType GetCellZone(int cx, int cz) const noexcept;
    bool HasCellNetwork(int cx, int cz, NetworkType* outType = nullptr) const noexcept;

    // Interactive mayor controls & tile query (Phase 1/3)
    bool SetTaxRate(Sint32 purpose, Sint32 wealth, Float32 rate) noexcept;
    Float32 GetTaxRate(Sint32 purpose, Sint32 wealth) const noexcept;
    bool IssueBond(Sint32 amount) noexcept;
    Sint64 GetTotalMonthlyBondPayments() const noexcept;
    TileQueryReport QueryTile(int cx, int cz) const noexcept;

    // Live Power/Water BFS, A* Commute Routing, Ordinances & Disasters
    void StepLivePowerAndWaterBFS();
    std::size_t RecomputeCommuteRoutes(std::size_t maxRoutes = 24);
    const std::vector<SC4CommuteRoute>& ActiveCommuteRoutes() const noexcept { return mActiveCommuteRoutes; }
    Uint32 PoweredCellCount() const noexcept { return mPoweredCellCount; }
    Uint32 WateredCellCount() const noexcept { return mWateredCellCount; }
    void SetOrdinanceOn(Uint32 ordinanceId, bool enabled);
    bool IsOrdinanceOn(Uint32 ordinanceId) const noexcept;
    bool TriggerDisaster(Uint32 disasterType, int cellX, int cellZ);
    Uint32 ActiveDisasterCount() const noexcept { return mActiveDisasterCount; }

private:
    std::shared_ptr<loaded_aging_transition::State> mLoadedAgingState;
    bool SaveOriginalToSC4Buffer(std::vector<Uint8>& outBuffer) const;
    bool SerializeDirtySaveEntry(const cSC4TGI& tgi, std::vector<Uint8>& outData) const;
    void MarkSaveEntryDirty(const cSC4TGI& tgi);
    void RebuildSpatialLookups();
    void UpdateSimulationGrids();
    void RebuildOriginalSaveDeveloperLists();
    void StepOriginalSaveDailyGrowth();
    void AutoTileNetworkAt(int x, int z);
    bool DevelopOneProceduralLot();
    void PlanProceduralDecline();
    void ExecuteProceduralDeclinePlan();
    Sint32 DemandForZone(ZoneType zone) const noexcept;
    bool LotHasBuilding(const SC4LotRecord& lot) const noexcept;

    struct ProceduralDeclinePlan {
        Uint32 executionDay = 0;
        ZoneType zone = ZoneType::None;
    };

    std::string mCityName   = "New City";
    std::string mMayorName  = "Mayor";
    std::string mSourcePath;
    bool mIsFromOriginalSave = false;
    std::vector<Uint8> mOriginalSaveBytes;
    std::vector<cSC4TGI> mDirtySaveEntries;
    cSC4TGI mBudgetSaveTGI{};

    int mCellCountX    = 128;
    int mCellCountZ    = 128;
    float mSeaLevel    = 250.0f;
    Uint32 mRegionTileX = 0;
    Uint32 mRegionTileZ = 0;

    Uint32 mResidentialPop = 0;
    Uint32 mCommercialPop  = 0;
    Uint32 mIndustrialPop  = 0;
    Sint64 mFunds          = 100000;

    Uint32 mYear      = 2000;
    Uint32 mMonth     = 1;
    Uint32 mDay       = 1;
    Uint32 mJulianDay = 2451545; // 2000-01-01
    Uint32 mTotalDays = 0;
    SimulationCadenceState mCadence{};
    BuildingDevelopmentState mBuildingDevelopment{};
    std::vector<ProceduralDeclinePlan> mProceduralDeclinePlans;

    RCIDemandState mDemand{};
    std::unordered_map<Uint32, Float32> mLoadedDemandValues;
    std::unordered_map<Uint32, SC4DemandRecord> mLoadedDemandRecords;

    std::vector<float> mHeightmap; // (mCellCountX + 1) * (mCellCountZ + 1)
    std::vector<SC4LotRecord> mLots;
    std::vector<SC4BuildingRecord> mBuildings;
    std::vector<BuildingOccupantSaveRecord> mSavedBuildingOccupants;
    bool mSavedBuildingOccupantsAvailable = false;
    std::vector<LotFullSaveRecord> mSavedLots;
    std::vector<ZoneDeveloperSaveRecord> mSavedZoneDevelopers;
    bool mSavedLotsAvailable = false;
    bool mSavedZoneDevelopersAvailable = false;
    std::vector<SC4NetworkRecord> mNetworks;
    std::vector<SC4PropRecord> mProps;
    std::vector<SC4FloraRecord> mFlora;
    std::vector<SC4LotTextureRecord> mLotTextures;
    std::vector<SC4AutomatonRecord> mAutomata;
    std::vector<Uint8> mPngThumbnail;

    Uint32 mPipeCount      = 0;
    Uint32 mSubwayCount    = 0;
    Uint32 mPowerLineCount = 0;

    std::vector<ZoneType> mCellZones;
    std::vector<Sint16> mCellNetworkIdx; // -1 if none

    // Active simulation tract grids (cSC4SimGrid<T>)
    cSC4SimGrid<Uint8> mLandValueGrid;
    cSC4SimGrid<Uint8> mPollutionGrid;
    cSC4SimGrid<Uint8> mCrimeGrid;
    cSC4SimGrid<Uint8> mTrafficGrid;
    cSC4SimGrid<Uint8> mPowerGrid;
    cSC4SimGrid<Uint8> mWaterGrid;
    cSC4SimGrid<Sint8> mMayorRatingGrid;

    // Raw SimGrids decoded from .sc4 save
    std::vector<cSC4SimGrid<Sint8>> mLoadedSint8Grids;
    std::vector<cSC4SimGrid<Uint8>> mLoadedUint8Grids;
    std::vector<cSC4SimGrid<Sint16>> mLoadedSint16Grids;
    std::vector<cSC4SimGrid<Uint16>> mLoadedUint16Grids;
    std::vector<cSC4SimGrid<Float32>> mLoadedFloat32Grids;

    // Decompiled RCI development & demand simulator states
    TractDeveloperTuning mTractDeveloperTuning{};
    TractDeveloperState mTractDeveloperState{};
    BuildingDevelopmentSimSaveRecord mBuildingDevSimRecord{};
    std::vector<GrowthDeveloperSaveRecord> mGrowthDeveloperRecords;
    std::unordered_map<Uint32, GrowthDeveloperTuning> mGrowthDeveloperTunings;
    std::unordered_map<Uint32, std::vector<GrowthDeveloperItem>> mGrowthDeveloperGrowthLists;
    std::unordered_map<Uint32, std::vector<GrowthDeveloperItem>> mGrowthDeveloperDeclineLists;
    DemandSimulatorSaveRecord mDemandSimRecord{};
    DemandSimulatorTuning mDemandSimTuning{};
    LotDeveloperTuning mLotDeveloperTuning{};
    LotDeveloperSaveRecord mLotDeveloperRecord{};
    LotConfigurationManagerSaveRecord mLotConfigManagerRecord{};
    BudgetSimulatorSaveRecord mBudgetSaveRecord{};
    BudgetSimulatorTuning mBudgetTuning{};
    bool mHasBudgetSaveRecord = false;
    LandValueSimulatorSaveRecord mLandValueSaveRecord{};
    CommercialSimulatorSaveRecord mCommercialSaveRecord{};
    IndustrialSimulatorSaveRecord mIndustrialSaveRecord{};
    FireProtectionSimulatorSaveRecord mFireProtectionSaveRecord{};
    PoliceSimulatorSaveRecord mPoliceSaveRecord{};
    FlammabilitySimulatorSaveRecord mFlammabilitySaveRecord{};
    CrimeSimulatorSaveRecord mCrimeSaveRecord{};
    AuraSimulatorSaveRecord mAuraSaveRecord{};
    PowerSimulatorSaveRecord mPowerSaveRecord{};
    PlumbingSimulatorSaveRecord mPlumbingSaveRecord{};
    PollutionSimulatorSaveRecord mPollutionSaveRecord{};
    ResidentialSimulatorSaveRecord mResidentialSaveRecord{};
    TrafficSimulatorSaveRecord mTrafficSaveRecord{};
    CivicBuildingSimulatorSaveRecord mCivicBuildingSaveRecord{};
    FloraSimulatorSaveRecord mFloraSaveRecord{};
    MySimAgentSimulatorSaveRecord mMySimAgentSaveRecord{};
    NeighborsSimulatorSaveRecord mNeighborsSaveRecord{};
    OrdinanceSimulatorSaveRecord mOrdinanceSaveRecord{};
    WeatherSimulatorSaveRecord mWeatherSaveRecord{};
    std::unordered_map<Uint32, bool> mCivicSaveRecordPresent;
    LandValueTuning mLandValueTuning{};
    FireProtectionTuning mFireProtectionTuning{};
    PoliceTuning mPoliceTuning{};
    FlammabilityTuning mFlammabilityTuning{};
    CrimeTuning mCrimeTuning{};
    AuraTuning mAuraTuning{};
    PowerTuning mPowerTuning{};
    PlumbingTuning mPlumbingTuning{};
    PollutionTuning mPollutionTuning{};
    ResidentialTuning mResidentialTuning{};
    TrafficTuning mTrafficTuning{};
    FloraTuning mFloraTuning{};
    WeatherTuning mWeatherTuning{};
    SimulatorSaveRecord mSimulatorSaveRecord{};
    SimulatorTuning mSimulatorTuning{};
    std::vector<SC4CommuteRoute> mActiveCommuteRoutes;
    Uint32 mPoweredCellCount = 0;
    Uint32 mWateredCellCount = 0;
    std::unordered_map<Uint32, bool> mActiveOrdinances;
    Uint32 mActiveDisasterCount = 0;
};

} // namespace opensc4
