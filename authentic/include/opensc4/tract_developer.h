#pragma once

#include "opensc4/formats.h"
#include "opensc4/gzcom.h"
#include "opensc4/sc4_subfiles.h"
#include <array>
#include <limits>
#include <map>
#include <span>
#include <vector>

namespace opensc4 {

// Exact fields consumed by cSC4TractDeveloper::ScoreCandidateLots.
// Neutral names are intentional where the interface symbol does not expose
// the original Maxis member name.
struct TractCandidateScoreInput {
    Uint32 candidateType = 0;       // CandidateLot +0x00
    Sint32 existingCapacityA = 0;   // CandidateLot +0x38
    Sint32 existingCapacityB = 0;   // CandidateLot +0x3c
    Sint32 maxExistingGrowthStage = 0;  // CandidateLot +0x40
    Sint32 maxExistingBuildingWealth = 0; // CandidateLot +0x44
    Uint64 sourceLotCount = 0;      // size of CandidateLot set at +0x20
    std::array<Sint32, 4> networkType3Or11{}; // vector at +0x88
    std::array<Sint32, 4> networkType0Or10{}; // vector at +0xa0
    std::array<Sint32, 4> networkType6{};     // vector at +0xb8
    std::array<Sint32, 4> networkType1{};     // vector at +0xd0
};

struct TractExistingLotCapacityInput {
    Uint16 capacity = 0;        // cISC4Lot vtable +0x198(purpose, 1)
    Uint32 lotState = 0;        // cISC4Lot vtable +0x108()
    Uint8 growthStage = 0;      // existing cISC4LotConfiguration vtable +0x58()
    Sint32 buildingWealth = 0;  // existing building descriptor +0x04 (0 if none)
};

// Exact cSC4TractDeveloper::FindExistingCapacity @ 0x1001390aa.
void AccumulateExistingCapacity(
    TractCandidateScoreInput& candidate,
    std::span<const TractExistingLotCapacityInput> sourceLots) noexcept;

// Exact growth-stage redevelopment comparison in
// cSC4TractDeveloper::IsLotConfigurationCompatible @ 0x10013aad4.
bool IsRedevelopmentGrowthStageCompatible(
    const TractCandidateScoreInput& candidate,
    Uint8 configGrowthStage,
    bool configMatchesMaxExistingWealth) noexcept;

struct TractCellRect {
    Sint32 minX = 0;
    Sint32 minZ = 0;
    Sint32 maxX = -1;
    Sint32 maxZ = -1;
};

// Exact orientation branches in cSC4TractDeveloper::GetBoundingRect for
// orientations 0..3. The byte-sized dimensions are CandidateLot +0x1c/+0x1d.
TractCellRect ComputeCandidateBounds(
    Sint32 originX,
    Sint32 originZ,
    Uint8 dimensionA,
    Uint8 dimensionB,
    Sint32 orientation) noexcept;

// Exact cSC4TractDeveloper::FindNetworks perimeter scan. Each grid element is
// a bit mask where bit N reports cISC4NetworkOccupant::IsNetworkType(N).
void ClassifyPerimeterNetworks(
    TractCandidateScoreInput& candidate,
    const TractCellRect& candidateBounds,
    const TractCellRect& sourceLotBounds,
    Sint32 cityCellsX,
    Sint32 cityCellsZ,
    std::span<const Uint16> networkTypeMasks) noexcept;

// Exact cSC4TractDeveloper::TestForCornerLot @ 0x10013b318.
// cornerMask bits 0..3 correspond to the bit vector returned by
// cISC4LotConfiguration vtable +0xa8.
bool TestForCornerLot(
    const TractCandidateScoreInput& candidate,
    Sint32 candidateOrientation,
    Uint8 cornerMask) noexcept;

// Exact corner-lot compatibility gate in
// cSC4TractDeveloper::IsLotConfigurationCompatible @ 0x10013ad24.
// Updates candidateHasCornerConfig (+0xe8) when any of bits 0..2 is set in
// cornerMask and TestForCornerLot succeeds.
bool EvaluateCornerLotCompatibility(
    const TractCandidateScoreInput& candidate,
    Sint32 candidateOrientation,
    Uint8 cornerMask,
    bool requireCornerConfiguration,
    bool& candidateHasCornerConfig) noexcept;

// Exact cISC4TractDeveloper::Outcome values (0..27) from machine code.
inline constexpr Uint32 kTractLotOutcomeNone = 0x00u;
inline constexpr Uint32 kTractLotOutcomeNoAbandonedTakenOver = 0x01u;
inline constexpr Uint32 kTractLotOutcomeMonopolyMismatch = 0x02u;
inline constexpr Uint32 kTractLotOutcomeNoCandidates = 0x03u;
inline constexpr Uint32 kTractLotOutcomeInvalidLotState = 0x04u;
inline constexpr Uint32 kTractLotOutcomeWrongSize = 0x05u;
inline constexpr Uint32 kTractLotOutcomeWrongZone = 0x06u;
inline constexpr Uint32 kTractLotOutcomeWrongPurpose = 0x07u;
inline constexpr Uint32 kTractLotOutcomeWrongWealth = 0x08u;
inline constexpr Uint32 kTractLotOutcomeWrongStyle = 0x09u;
inline constexpr Uint32 kTractLotOutcomeWrongSlope = 0x0au;
inline constexpr Uint32 kTractLotOutcomeMaxCapacityTooSmall = 0x0bu;
inline constexpr Uint32 kTractLotOutcomeStageNotHigher = 0x0cu;
inline constexpr Uint32 kTractLotOutcomeMinCapacityTooLarge = 0x0du;
inline constexpr Uint32 kTractLotOutcomeStageExceedsTarget = 0x0eu;
inline constexpr Uint32 kTractLotOutcomeCannotRedevelopPurpose = 0x0fu;
inline constexpr Uint32 kTractLotOutcomeCannotRedevelopWealthOrPurpose = 0x10u;
inline constexpr Uint32 kTractLotOutcomeTargetStageTooLowForExisting = 0x11u;
inline constexpr Uint32 kTractLotOutcomeHistorical = 0x12u;
inline constexpr Uint32 kTractLotOutcomeOccupancyCooldown = 0x13u;
inline constexpr Uint32 kTractLotOutcomeInsufficientResources = 0x14u;
inline constexpr Uint32 kTractLotOutcomeIneligibleRoad = 0x15u;
inline constexpr Uint32 kTractLotOutcomeWrongLandValue = 0x16u;
inline constexpr Uint32 kTractLotOutcomeNoFireCoverage = 0x17u;
inline constexpr Uint32 kTractLotOutcomeNoPark = 0x18u;
inline constexpr Uint32 kTractLotOutcomeNoWater = 0x19u;
inline constexpr Uint32 kTractLotOutcomeNoPower = 0x1au;
inline constexpr Uint32 kTractLotOutcomeSuitable = 0x1bu;

struct TractSourceLotInfo {
    Uint32 suitabilityOutcome = 0; // IsLotSuitable(lot, true) return value
    Sint32 orientation = -1;       // cISC4Lot vtable +0xb8
    Uint32 zoneType = 0;           // cISC4Lot vtable +0xf8
    TractCellRect bounds{};        // cISC4Lot vtable +0x70
};

// Exact cSC4TractDeveloper::IsAggregationLegal @ 0x10013ae4a.
// cellLotIndices maps each (x, z) cell to an index in sourceLots, or -1 if no
// lot exists at that cell. Populates outSourceLotCount with the number of
// unique source lots encountered before returning.
bool IsAggregationLegal(
    const TractCellRect& candidateBounds,
    Sint32 candidateOrientation,
    Sint32 cityCellsX,
    Sint32 cityCellsZ,
    std::span<const Sint32> cellLotIndices,
    std::span<const TractSourceLotInfo> sourceLots,
    Uint64& outSourceLotCount) noexcept;

// Exact dimension filter in cSC4TractDeveloper::ListCandidateLots_Aggregation
// @ 0x100137e55: configDimensions >= lotDimensions and not both equal.
bool IsAggregationSizeCompatible(
    Uint8 lotDimensionA,
    Uint8 lotDimensionB,
    Uint32 configDimensionA,
    Uint32 configDimensionB) noexcept;

// Exact second-candidate origin shift in
// cSC4TractDeveloper::ListCandidateLots_Aggregation @ 0x100137d78.
void ComputeAggregationShiftedOrigin(
    Sint32 lotOriginX,
    Sint32 lotOriginZ,
    Uint8 lotDimensionA,
    Uint32 configDimensionA,
    Sint32 orientation,
    Sint32& outOriginX,
    Sint32& outOriginZ) noexcept;

// Exact dimension filter and subdivision count (+0x80) in
// cSC4TractDeveloper::ListCandidateLots_Subdivision @ 0x10013808d.
// Returns 0 when the configuration cannot subdivide the source lot.
Uint8 ComputeSubdivisionCount(
    Uint8 lotDimensionA,
    Uint8 lotDimensionB,
    Uint32 configDimensionA,
    Uint32 configDimensionB) noexcept;

// Exact cSC4BuildingDevelopmentSimulator::GrowthStageFitsInZone @ 0x100150a4c.
bool GrowthStageFitsInZone(Sint32 stage, Uint32 zoneType) noexcept;

// Exact cSC4BuildingDevelopmentSimulator::BuildingFitsInZone @ 0x100150a86.
bool BuildingFitsInZone(Uint32 buildingZoneType, Uint32 cellZoneType) noexcept;

// Exact cSC4GrowthDeveloper::ConvertZoneToType @ 0x10015a6bc.
Uint32 ConvertZoneToDeveloperClass(Uint32 zoneType) noexcept;

// Exact baseZoneType mapping in cSC4GrowthDeveloper::GetConstructionInfo
// @ 0x10015a6da.
Uint32 ComputeConstructionBaseZoneType(
    Uint32 developerClass,
    Uint32 purpose) noexcept;

struct TractLotSuitabilityInput {
    Uint32 developerBaseZoneType = 0; // ConstructionProfile +0x00
    Uint32 developerWealth = 0;       // ConstructionProfile +0x08 (+0xe4)
    Uint32 developerPurpose = 0;      // ConstructionProfile +0x0c (+0xe8)
    Uint8 minLandValue = 0;           // ConstructionProfile +0x10 (+0xec)
    Uint8 maxLandValue = 0;           // ConstructionProfile +0x11 (+0xed)
    Sint32 targetGrowthStage = 0;     // cSC4TractDeveloper +0xd8
    bool allowLowerWealthRedevelopment = true; // cSC4TractDeveloper +0xa4
    bool checkForNewDevelopment = true;

    Sint8 cellZoneType = 0;           // mZoneManager->GetZoneType(x, z)
    Uint8 cellLandValue = 0;          // mLandValueSim->GetLandValue(x, z)
    Uint32 lotState = 1;              // cSC4Lot::GetState() (+0xd3)
    Uint32 daysSinceLastStateChange = 0; // cSC4Lot::GetDaysSinceLastStateChange()
    Uint32 canCreateLotResult = 7;    // mLotManager->CanCreateLot(...) (7 or 8 = ok)
    Uint32 existingBuildingType = 0;  // cSC4Lot::GetBuildingType(false)
    Uint32 existingBuildingPurpose = 0; // BuildingProfile +0x00
    Uint32 existingBuildingWealth = 0;  // BuildingProfile +0x04
    bool isHistorical = false;        // cSC4Lot::IsHistorical()
    bool hasExistingLotConfiguration = false;
    Uint8 existingLotConfigGrowthStage = 0; // existing config->GetGrowthStage()
    bool hasRoadOrJobAccess = true;   // HasOrganicRoadAccess / JobAccess / HasRoadAccess
};

// Exact cSC4TractDeveloper::IsLotSuitable @ 0x100138b10.
// Sets outRejectedLandValue (+0x16c) when failing with kTractLotOutcomeWrongLandValue.
Uint32 EvaluateLotSuitability(
    const TractLotSuitabilityInput& input,
    Sint32& outRejectedLandValue) noexcept;

struct TractLotConfigSuitabilityInput {
    Uint32 developerPurpose = 0;        // mCurrentDeveloper->GetPurpose()
    Sint32 targetGrowthStage = 0;       // cSC4TractDeveloper +0xd8
    Sint32 tractCurrentPopulation = 0;  // cSC4TractDeveloper +0x100
    Sint32 maxCapacityPerStep = 1;      // cSC4TractDeveloper +0x104
    Sint32 maxOverbuildCapacity = 1;    // cSC4TractDeveloper +0x108
    bool allowSingleUnitMinCapacity = false; // cSC4TractDeveloper +0x114
    bool cycleThroughStyles = true;     // cSC4TractDeveloper +0x1c0
    Uint32 currentStyle = 0;            // mActiveStyles[mCurrentStyleIndex]
    bool hasActiveStyles = false;       // !mActiveStyles.empty()

    bool purposeCompatible = true;      // config->IsCompatibleWithPurposeType
    bool wealthCompatible = true;       // config->IsCompatibleWithWealthType
    Uint8 configGrowthStage = 1;        // config->GetGrowthStage()
    bool matchesCurrentStyle = true;    // config->IsCompatibleWithStyle(currentStyle)
    bool matchesAnyActiveStyle = true;  // any style in mActiveStyles matches
    bool hasPrimaryBuildingObject = true; // config->GetPrimaryBuildingConfigurationObject() != nullptr
    Uint16 minBuildingCapacity = 0;     // config->GetMinBuildingCapacity()
    Uint16 maxBuildingCapacity = 0;     // config->GetMaxBuildingCapacity()
};

// Exact cSC4TractDeveloper::IsLotConfigurationSuitable @ 0x10013a43c.
Uint32 EvaluateLotConfigurationSuitability(
    const TractLotConfigSuitabilityInput& input) noexcept;

struct TractLotConfigCompatibilityInput {
    Uint32 configSizeX = 0;             // config->GetSize()
    Uint32 configSizeZ = 0;
    Uint8 candidateDimensionA = 0;      // CandidateLot +0x1c
    Uint8 candidateDimensionB = 0;      // CandidateLot +0x1d
    Sint32 candidateOrientation = 0;    // CandidateLot +0x18
    bool zoneCompatible = true;         // config->IsCompatibleWithZoneType(cellZone)
    Uint8 configGrowthStage = 1;        // config->GetGrowthStage()
    bool configMatchesMaxExistingWealth = false; // config->IsCompatibleWithWealthType(+0x44)
    Float32 candidateSlope = 0.0f;      // lot->GetSlope() or GetAreaSlope()
    Float32 configMinSlope = 0.0f;      // config->GetMinimumSlope()
    Float32 configMaxSlope = 100.0f;    // config->GetMaximumSlope()

    bool cheatIgnoreCaps = false;       // mBuildingDevelopmentSim->CheatIgnoreCaps()
    bool cheatIgnorePower = false;      // mBuildingDevelopmentSim->CheatIgnorePower()
    bool cheatIgnorePlumbing = false;   // mBuildingDevelopmentSim->CheatIgnorePlumbing()
    Uint8 powerSupplyCap = 255u;        // mCurrentDeveloper->GetPowerSupplyCap()
    Uint8 waterSupplyCap = 255u;        // mCurrentDeveloper->GetWaterSupplyCap()
    Uint8 fireCoverageCap = 255u;       // mCurrentDeveloper->GetFireCoverageCap()
    Uint8 parkCap = 255u;               // mCurrentDeveloper->GetParkCap()
    Uint8 powerGridValue = 2u;          // mPowerSim->GetPowerGrid()->GetCellValue(x, z)
    bool isWatered = true;              // mPlumbingSim->IsWatered(x, z)
    Uint8 fireProtectionGridValue = 1u; // spFireProtectionSim->GetFireProtectionGrid()->GetCellValue(x, z)
    Uint16 parkGridValue = 1u;          // spAuraSim->GetParkGrid()->GetCellValue(x, z)

    Uint32 developerClass = 1u;         // mCurrentDeveloper->GetType() (3 = Industrial)
    Uint8 cornerMask = 0u;              // config->GetRequiredRoads() bits 0..3
    bool requireCornerConfiguration = false;
};

// Exact cSC4TractDeveloper::IsLotConfigurationCompatible @ 0x10013aa5c.
Uint32 EvaluateLotConfigurationCompatibility(
    const TractCandidateScoreInput& candidate,
    const TractLotConfigCompatibilityInput& input,
    bool& candidateHasCornerConfig) noexcept;

// Exact capacity range calculation in cSC4TractDeveloper::PickBuilding
// @ 0x10013960c-0x100139674 (sets +0x170 and +0x174).
void ComputePickBuildingCapacityRange(
    Uint32 candidateType,
    Sint32 existingCapacityA,
    Uint8 subdivisionCount,
    bool allowSingleUnitMinCapacity,
    Sint32 maxCapacityPerStep,
    Sint32 maxOverbuildCapacity,
    Sint32 builtCapacityThisGrow,
    Sint32 subdivisionBuiltCapacity,
    Sint32& outMinBuildingCapacity,
    Sint32& outMaxBuildingCapacity) noexcept;

struct TractBuildingCompatibilityInput {
    Uint32 buildingPurpose = 0;         // BuildingProfile +0x00
    Uint32 buildingWealth = 0;          // BuildingProfile +0x04
    Uint16 buildingCapacity = 0;        // BuildingProfile +0x08
    bool buildingMatchesCurrentStyle = true; // IsOccupantGroup(key, currentStyle)
    bool isMonopolyPiece = false;       // exemplar property 0x2781283f

    Uint32 developerPurpose = 0;        // mCurrentDeveloper->GetPurpose()
    Uint32 developerWealth = 0;         // mCurrentDeveloper->GetWealth()
    Uint32 developerMonopolyUse = 0;    // mCurrentDeveloper->GetMonopolyUse()
    bool hasActiveStyles = false;       // !mActiveStyles.empty()
    Sint32 minBuildingCapacity = 0;     // cSC4TractDeveloper +0x170
    Sint32 maxBuildingCapacity = 0;     // cSC4TractDeveloper +0x174
};

// Exact cSC4TractDeveloper::IsBuildingCompatible @ 0x10013b436.
Uint32 EvaluateBuildingCompatibility(
    const TractBuildingCompatibilityInput& input) noexcept;

inline constexpr Uint32 kTractAnchorBuildingFamily = 0x3000u;
inline constexpr Uint32 kTractFarmCropDeveloperType = 0x4100u;

struct TractStylePercentageEntry {
    Uint32 styleId = 0;         // std::map<Uint32, Float32> key (+0x1c)
    Float32 percentage = 0.0f;  // std::map<Uint32, Float32> value (+0x20, 0.0..1.0)
};

// Exact style/anchor selection in cSC4TractDeveloper::PickBuilding
// @ 0x10013934d-0x100139423.
// randomRoll0To99 is sRandom.RandomUint32Uniform(100).
Uint32 SelectPickBuildingStyle(
    bool needsAnchorBuilding,
    std::span<const TractStylePercentageEntry> stylePercentages,
    Uint32 randomRoll0To99) noexcept;

// Exact capacity thresholds in cSC4TractDeveloper::TakeOverAbandoned
// @ 0x1001369e2-0x100136a15, 0x100136efe, 0x100136f51.
void ComputeTakeOverAbandonedThresholds(
    Sint32 maxCapacityPerStep,
    Sint32 maxOverbuildCapacity,
    Sint32 builtCapacityThisGrow,
    Float32 abandonedTakeoverMinFactor,
    Float32 abandonedTakeoverMaxFactor,
    Sint32& outMinTotalAvailableThreshold,
    Sint32& outMaxTakeoverCapacityLimit) noexcept;

struct TractTakeOverAbandonedLotInput {
    Uint32 lotState = 0;                // lot->GetState() (+0x108)
    Uint32 occupantWealth = 0;          // lot->GetOccupantWealth() (+0x148)
    Uint32 existingBuildingType = 0;    // lot->GetBuildingType(false) (+0x170)
    Uint32 existingBuildingWealth = 0;  // BuildingProfile +0x04
    Uint32 developerWealth = 0;         // mCurrentDeveloper->GetWealth() (+0x40)
    Uint32 developerClass = 0;          // mCurrentDeveloper->GetType() (+0x28)
    Uint32 lotZoneType = 0;             // lot->GetZoneType() (+0xf8)
    bool allowLowerWealthRedevelopment = true; // cSC4TractDeveloper +0xa4
    Uint16 totalCapacityForDeveloper = 0;      // lot->GetCapacity(devType, true)
    Uint16 availableCapacityForDeveloper = 0;  // lot->GetAvailableCapacity(devType, true)
    Uint32 suitabilityOutcome = 0;      // IsLotSuitable(lot, false)

    bool cheatIgnoreCaps = false;
    bool cheatIgnorePower = false;
    bool cheatIgnorePlumbing = false;
    bool hasExistingLotConfiguration = false;
    Uint8 existingLotConfigGrowthStage = 1u;
    Uint8 existingDeveloperPowerSupplyCap = 255u;
    Uint8 existingDeveloperWaterSupplyCap = 255u;
    Uint8 currentDeveloperFireCoverageCap = 255u;
    Uint8 currentDeveloperParkCap = 255u;
    Uint8 powerGridValue = 2u;
    bool isWatered = true;
    Uint8 fireProtectionGridValue = 1u;
    Uint16 parkGridValue = 1u;
};

// Exact per-lot eligibility and takeover capacity in
// cSC4TractDeveloper::TakeOverAbandoned @ 0x100136aa2-0x100136e55.
// Returns true when the lot is added as a candidateType == 1 takeover
// candidate, populating outTakeoverCapacity (+0x3c) and updating
// inoutBestOutcome.
bool EvaluateTakeOverAbandonedLot(
    const TractTakeOverAbandonedLotInput& input,
    Sint32& outTakeoverCapacity,
    Uint32& inoutBestOutcome) noexcept;

// Exact cSC4TractDeveloper::UpdatePopulation @ 0x10013beb6.
// Computes the deterministic PRNG seed ((tractX << 20) + (tractZ << 4)) and
// the exact IEEE-754 float/double population update for a single lot.
Uint32 ComputeUpdatePopulationSeed(Sint32 tractX, Sint32 tractZ) noexcept;

Uint16 ComputeUpdatedLotPopulation(
    Sint16 maxCapacityTractValue,
    Uint16 availableCapacity,
    Float64 uniformNoiseSample,
    Float64 uniformOverflowTrimSample) noexcept;

struct TractDeclineLotInput {
    Uint32 lotState = 0;                 // lot->GetState() (+0x108)
    Uint32 daysSinceLastStateChange = 0; // lot->GetDaysSinceLastStateChange() (+0x140)
    Uint32 existingBuildingType = 0;     // lot->GetBuildingType(false) (+0x170)
    Uint32 existingBuildingPurpose = 0;  // BuildingProfile +0x00
    Uint32 occupantWealth = 0;           // lot->GetOccupantWealth() (+0x148)
    Uint32 developerPurpose = 0;         // profile.developer->GetPurpose() (+0x38)
    Uint32 developerWealth = 0;          // profile.developer->GetWealth() (+0x40)
};

// Exact per-lot eligibility in cSC4TractDeveloper::Decline @ 0x10013bd4a-0x10013be88.
bool IsLotEligibleForDecline(const TractDeclineLotInput& input) noexcept;

// Exact initial capacity/retry setup in cSC4TractDeveloper::Grow
// @ 0x100136104-0x1001362de.
void ComputeGrowInitialParameters(
    Float32 activeDemandValue,
    Float32 maxOverbuildFactor,
    Sint32 cellsPerTractSide,
    Sint32& outMaxCapacityPerStep,
    Sint32& outMaxOverbuildCapacity,
    Sint32& outMaxPassRetries) noexcept;

// Exact cSC4TractDeveloper::BuildFarm @ 0x10013860d-0x100138646 requirement:
// requires >= 15 farm lots and (farmLotCount + 30) free construction resources.
bool CanStartFarmBuild(
    Uint32 connectedFarmLotCount,
    Uint16 freeConstructionResources,
    Uint16& outInitialClaimedResources) noexcept;

struct TractDeveloperTuning {
    // Defaults and property IDs are from ReadSimulatorTunableValues @ 0x1001353a6.
    Float32 maxOverbuildFactor = 1.5f;              // 0x47e2c500 (+0x78)
    Float32 abandonedTakeoverMinFactor = 0.0f;      // 0x47e2c501[0] (+0x7c)
    Float32 abandonedTakeoverMaxFactor = 1.2f;      // 0x47e2c501[1] (+0x80)
    Float32 tunableFloat84 = 1.5f;                  // 0x47e2c502 (+0x84)
    Float32 existingCapacityAWeight = 1.0f;         // 0x47e2c510[0] (+0x88)
    Float32 existingCapacityBWeight = 0.2f;         // 0x47e2c510[1] (+0x8c)
    Float32 sourceLotCountWeight = 10.0f;           // 0x47e2c511[0] (+0x90)
    Float32 candidateType4Bonus = 5.0f;             // 0x47e2c511[1] (+0x94)
    Float32 populationNoiseHalfRange = 0.2f;        // 0x47e2c530 (+0x98)
    Float32 populationOverflowMin = 0.01f;          // 0x47e2c531[0] (+0x9c)
    Float32 populationOverflowMax = 0.1f;           // 0x47e2c531[1] (+0xa0)
    bool allowLowerWealthRedevelopment = true;      // 0x47e2c540 (+0xa4)
    std::vector<Uint32> defaultActiveStyles;        // 0x47e2c550 (+0xa8)
    Sint32 yearsBetweenStyleChanges = 40;           // 0x47e2c551 (+0xc0)

    bool LoadFromExemplar(const cGZExemplar& exemplar) noexcept;

    // Exact operation order from ScoreCandidateLots @ 0x100138296.
    Float32 ScoreCandidate(const TractCandidateScoreInput& candidate,
                           Uint32 developerWealth) const noexcept;

    // Exact CompareCandidateLots predicate: lower scores sort first.
    static bool CompareCandidates(Float32 lhsScore, Float32 rhsScore) noexcept {
        return rhsScore > lhsScore;
    }
};

// Exact serialized state of cSC4TractDeveloper (CLSID 0x2990c142):
// Read @ 0x100135c32, Write @ 0x100135b62, DoMessage @ 0x100135fe4.
// Complete method index:
//   cSC4TractDeveloper() @ 0x100134bfa, ~cSC4TractDeveloper() @ 0x100134ddc /
//   0x100134f84 / 0x100134f8e, QI @ 0x100134fbc, AddRef @ 0x10013501c, Release @ 0x100135046,
//   Init @ 0x1001350fe, ReadSimulatorTunableValues @ 0x1001353a6, Shutdown @ 0x1001356a8,
//   FreeGrowthDatabase @ 0x100135a12, CandidateLot::~CandidateLot() @ 0x100135ae2,
//   ClearCandidateList @ 0x10013667c, ListExistingLots @ 0x1001366f6,
//   RecordBuildAttempt @ 0x100137018, RecordGrowthAttempt @ 0x1001372b4,
//   ListPossibleLotConfigurations @ 0x100137588,
//   ListCandidateLots_Existing @ 0x1001377a0,
//   Build @ 0x1001388c4, NewCandidateLot @ 0x100138eae,
//   DeleteCandidateLot @ 0x1001392b6, PerformAggregation @ 0x100139dd2,
//   PerformSubdivision @ 0x10013a04e, GetBoundingRect @ 0x10013b130,
//   CompareCandidateLots @ 0x10013b64e, FreeBuildAttempt @ 0x10013b6bc,
//   FreeGrowthAttempt @ 0x10013b77c, GetLastSuccessfulBuild @ 0x10013b9d4,
//   GetLastFailedBuild @ 0x10013baf2, SetActiveStyles @ 0x10013c0fa,
//   GetCurrentStyle @ 0x10013c166, GetActiveStyles @ 0x10013c18a,
//   IsUsingAllStylesAtOnce @ 0x10013c198, UseAllStylesAtOnce @ 0x10013c1a8,
//   GetYearsBetweenStyleChanges @ 0x10013c1ba,
//   SetYearsBetweenStyleChanges @ 0x10013c1c6,
//   GrowthRecord::~GrowthRecord() @ 0x10013c40a,
//   CandidateLot::CandidateLot() @ 0x10013c550,
//   BuildAttemptRecord(BuildAttemptRecord const&) @ 0x10013d008.
struct TractDeveloperState {
    Sint32 currentStyleIndex = 0;        // +0x1b8
    Sint32 yearsSinceLastStyleChange = 0; // +0x1bc
    bool cycleThroughStyles = true;      // +0x1c0 (!IsUsingAllStylesAtOnce)
    std::vector<Uint32> activeStyles;    // +0x1a0
    Sint32 yearsBetweenStyleChanges = 0; // +0xc0

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;

    void ApplyTuningDefaults(const TractDeveloperTuning& tuning);
    void OnNewYear() noexcept;
    Uint32 GetCurrentStyle() const noexcept;
    void SetActiveStyles(std::span<const Uint32> styles);
    const std::vector<Uint32>& GetActiveStyles() const noexcept { return activeStyles; }
    Sint32 GetYearsBetweenStyleChanges() const noexcept { return yearsBetweenStyleChanges; }
    void SetYearsBetweenStyleChanges(Sint32 years) noexcept { yearsBetweenStyleChanges = years; }
    bool IsUsingAllStylesAtOnce() const noexcept { return !cycleThroughStyles; }
    void UseAllStylesAtOnce(bool value) noexcept { cycleThroughStyles = !value; }
};

// Exact cSC4ResponseCurve @ 0x1000ac220 - 0x1000ac577.
class cSC4ResponseCurve {
public:
    struct Point {
        Float32 x = 0.0f;
        Float32 y = 0.0f;
    };

    struct Segment {
        Float32 x = 0.0f;
        Float32 y = 0.0f;
        Float32 prevX = 0.0f;
        Float32 prevY = 0.0f;
        Float32 slope = 0.0f;
    };

    bool SetResponseCurveData(std::span<const Point> points);
    bool SetResponseCurveData(std::span<const Float32> flatXYPairs);
    bool SetResponseCurveData(const ExemplarProperty& prop);

    Float32 GetOutputValue(Float32 input) const noexcept;
    // Explicit portable boundary for original GetOutputValue @ 0x1000ac542:
    // false means no backing slot exists (original reads address 4), with
    // out untouched. Consumers must propagate this fault, not a valid zero.
    bool TryGetOutputValue(Float32 input, Float32& out) const noexcept;
    bool GetMaxInputValue(Float32& outMaxX) const noexcept;

    bool Empty() const noexcept { return mSegments.empty(); }
    const std::vector<Segment>& Segments() const noexcept { return mSegments; }

private:
    std::vector<Segment> mSegments;
    // Original vector clear @ 0x1000ac237 retains its backing allocation.
    // Model the readable first-slot Y without accessing destroyed vector elements.
    bool mHasRetainedFirstY = false;
    Float32 mRetainedFirstY = 0.0f;
};

// Exact SimGrid data IDs assigned in cSC4GrowthDeveloper::Init @ 0x1001567d0-0x100156a8e.
inline constexpr Uint32 kExemplarGroup_GrowthDeveloper = 0x67cd5fa1u;
inline constexpr std::array<Uint32, 12> kGrowthDeveloperSubTypes = {
    0x1010u, 0x1020u, 0x1030u, // R$, R$$, R$$$
    0x3110u, 0x3120u, 0x3130u, // Cs$, Cs$$, Cs$$$
    0x3320u, 0x3330u,          // Co$$, Co$$$
    0x4100u, 0x4200u, 0x4300u, 0x4400u // I-R, I-D, I-M, I-HT
};

struct GrowthDeveloperSimGridIDs {
    static constexpr Uint32 ZonableCellsGridId(Uint32 subType) noexcept {
        return 0x01000000u + subType;
    }
    static constexpr Uint32 DesirabilityGridId(Uint32 subType) noexcept {
        return 0x02000000u + subType;
    }
    static constexpr Uint32 TempDesirabilityGridId(Uint32 subType) noexcept {
        return 0x02100000u + subType;
    }
    static constexpr Uint32 PopulationGridId(Uint32 subType) noexcept {
        return 0x03000000u + subType;
    }
    static constexpr Uint32 CapacityGridId(Uint32 subType) noexcept {
        return 0x05000000u + subType;
    }
    static constexpr Uint32 MaxCapacityGridId(Uint32 subType) noexcept {
        return 0x06000000u + subType;
    }
};

struct GrowthDesirabilityInputs {
    Uint8 landValue = 0;                 // Intrinsic or total land value
    Float32 slope = 0.0f;                // Terrain slope grid
    std::array<Uint8, 3> resProximity{}; // R$, R$$, R$$$ proximity grids (1..3)
    Uint8 co2Proximity = 0;              // Co$$ (0x3320) proximity grid
    Uint8 co3Proximity = 0;              // Co$$$ (0x3330) proximity grid
    Sint16 parkEffect = 0;               // Park aura grid
    std::array<Sint16, 4> pollution{};   // Air, water, garbage, radiation grids (0..3)
    Float32 schoolEffect = 0.0f;         // School coverage grid
    Float32 hospitalEffect = 0.0f;       // Hospital coverage grid
    Uint8 crime = 0;                     // Crime grid
    Uint8 minNonZeroTripLength = 255u;   // Minimum non-zero trip length in tract (255 if none)
    Uint8 traffic = 0;                   // Traffic congestion grid
    Sint16 landmarkEffect = 0;           // Landmark aura grid
    Sint8 transientAura = 0;             // Mayor rating / transient aura grid
};

// Exact cSC4GrowthDeveloper::ReadSimulatorTunableValues @ 0x100156c0c
// and EstimateDesirability @ 0x10015aace.
struct GrowthDeveloperTuning {
    Uint32 subType = 0;                  // 0x00000021 (+0x170)
    Uint32 developerClass = 0;           // 0x87cd6300 (+0x174: 1=R, 2=C, 3=I)
    Uint32 sourceDemandId = 0;           // 0x87cd6301 (+0x60)
    Uint8 tractSizeShift = 2u;           // 0x87cd6310 (+0x74)
    Uint32 wealth = 0;                   // 0x87cd6330 (+0x6c)
    Uint32 purpose = 0;                  // 0x87cd6332 (+0x68)
    Uint8 minLandValue = 0;              // 0x87cd6333[0] (+0x75)
    Uint8 maxLandValue = 0;              // 0x87cd6333[1] (+0x76)
    bool useIntrinsicLandValue = false;  // 0x87cd633f (+0x77)

    cSC4ResponseCurve landValueCurve;    // 0x87cd6340 (+0x78)
    cSC4ResponseCurve slopeCurve;        // 0x87cd6398 (+0x80)
    std::array<cSC4ResponseCurve, 4> pollutionCurves{}; // 0x87cd634b..4e (+0x88)
    cSC4ResponseCurve parkEffectCurve;   // 0x87cd6341 (+0xa0)
    std::array<cSC4ResponseCurve, 3> resProximityCurves{}; // 0x87cd6343..45 (+0xa8..+0xb8)
    cSC4ResponseCurve co2ProximityCurve; // 0x87cd6391 (+0xc0)
    cSC4ResponseCurve co3ProximityCurve; // 0x87cd6392 (+0xc8)
    cSC4ResponseCurve schoolEffectCurve; // 0x87cd6348 (+0xd0)
    cSC4ResponseCurve hospitalEffectCurve; // 0x87cd6349 (+0xd8)
    cSC4ResponseCurve crimeEffectCurve;  // 0x87cd634a (+0xe0)
    cSC4ResponseCurve tripLengthCurve;   // 0x87cd6347 (+0xe8)
    cSC4ResponseCurve trafficEffectCurve; // 0x87cd6346 (+0xf0)
    cSC4ResponseCurve landmarkEffectCurve; // 0x87cd6399 (+0xf8)
    cSC4ResponseCurve transientAuraCurve; // 0x87cd6397 (+0x100)

    Sint32 baseDesirability = 80;        // 0x87cd6357 (+0x108)
    Sint32 desirabilityThreshold = 80;   // 0x87cd6355 (+0x10c)
    Sint32 declineDesirabilityThreshold = 80; // 0x87cd6354 (+0x110)
    bool desirabilityFeedsDemandSupply = false; // 0x87cd6356 (+0x114)

    std::vector<cSC4ResponseCurve> stageCurves; // 0x87cd6360 count, 0x87cd6361+i (+0x118)

    Uint8 powerSupplyCap = 0;            // 0x87cd6373 (+0x130)
    Uint8 waterSupplyCap = 0;            // 0x87cd6370 (+0x131)
    Uint8 fireCoverageCap = 0;           // 0x87cd6371 (+0x132)
    Uint8 parkCap = 0;                   // 0x87cd6372 (+0x133)

    std::array<Float32, 3> abandonmentThresholds{0.2f, 0.2f, 0.2f}; // 0x87cd6380 (+0x138, densities 1..3)
    Uint32 monopolyUse = 1u;             // 0x87cd6381 (+0x150)
    std::vector<TractStylePercentageEntry> stylePercentages; // 0x87cd63a0 (+0x158)
    std::vector<Sint32> subjectiveThresholdLow;  // 0xcab1ce3a (+0x218)
    std::vector<Sint32> subjectiveThresholdHigh; // 0xcab1ce3c (+0x230)

    bool LoadFromExemplar(const cGZExemplar& exemplar);

    // Exact point-estimate desirability at a cell/tract from
    // cSC4GrowthDeveloper::EstimateDesirability @ 0x10015aace
    // (plus tripLengthCurve applied in DoIdle step 12 @ 0x1001587c6).
    Sint16 EvaluateDesirability(
        const GrowthDesirabilityInputs& inputs,
        bool includeTripLength = false) const noexcept;

    // Exact cSC4GrowthDeveloper::GetSubjectiveThresholds @ 0x10015b0cc.
    void GetSubjectiveThresholds(
        Sint32 index,
        Sint32& outLow,
        Sint32& outHigh) const noexcept;

    // Exact cSC4GrowthDeveloper::GetAbandonmentThreshold @ 0x10015b21a
    // (also cSC4GrowthDeveloper() @ 0x10015606e, Shutdown @ 0x1001578b6,
    // SetTractSize @ 0x100157dc0, DoMessage @ 0x100157fd6,
    // UpdateMaxCapacity @ 0x100159800).
    Float32 GetAbandonmentThreshold(Uint32 densityType) const noexcept {
        if (densityType >= 1u && densityType <= 3u) {
            return abandonmentThresholds[densityType - 1u];
        }
        return 0.0f;
    }
};

// Exact 12-byte cISC4GrowthDeveloper::GrowthItem used in mGrowthList (+0x1b0)
// and mDeclineList (+0x1c8).
struct GrowthDeveloperItem {
    Sint32 tractX = 0;
    Sint32 tractZ = 0;
    Sint32 growthStage = 1;
};

// Exact DoIdle step 18 tract target population formula @ 0x100158f99-0x100158fea.
Sint16 ComputeGrowthTractTargetPopulation(
    Sint16 desirability,
    Uint16 capacity) noexcept;

// Exact DoIdle step 19 unmet-desirability demand supply contribution
// @ 0x10015900b-0x1001590e8.
Sint32 ComputeDesirabilityDemandSupply(
    bool desirabilityFeedsDemandSupply,
    Sint32 desirabilityThreshold,
    std::span<const GrowthDeveloperItem> growthItems,
    std::span<const Sint16> tempDesirabilityGrid,
    std::span<const Sint16> populationGrid,
    Sint32 gridTractsZ) noexcept;

// Exact DoIdle step 20 stage-lookup population calculation @ 0x100159288-0x100159520.
Float32 ComputeStageLookupPopulation(
    Uint32 developerClass,
    Float32 residentialDemand1001Supply,
    Float32 commercialDemand3001Supply,
    Float32 commercialRegionDemand3301Supply,
    Float32 industrialDemand4001Supply,
    Sint32 connectedNeighborCitiesCount,
    Sint32 neighborConnectionsCount,
    Float32 stagePopulationMultiplier) noexcept;

// Exact DoIdle steps 17 & 20 @ 0x100158cb0 & 0x1001590f8:
// populates growthList (sorted descending by desirability, filtered by
// >= desirabilityThreshold, with assigned growthStages 1..N) and declineList
// (sorted ascending by desirability, filtered by < declineDesirabilityThreshold).
void BuildGrowthAndDeclineLists(
    const GrowthDeveloperTuning& tuning,
    std::span<const Sint8> zonableCellsGrid,
    std::span<const Sint16> tempDesirabilityGrid,
    Sint32 gridTractsX,
    Sint32 gridTractsZ,
    Float32 scaledStageLookupPopulation,
    std::vector<GrowthDeveloperItem>& outGrowthList,
    std::vector<GrowthDeveloperItem>& outDeclineList);

// Exact serialized record of cSC4GrowthDeveloper (CLSID 0x0990c138):
// Read @ 0x10015afe2, Write @ 0x10015af3a.
struct GrowthDeveloperSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1u;
    Sint32 suppliedDemand = 0; // +0x1e0
    Uint32 totalCapacity = 0;  // +0x1e4
    struct GridRef {
        Uint32 memAddr = 0;
        Uint32 clsid = 0;
    };
    GridRef zonableCellsGrid{};  // +0x1e8 (0x49b9e602)
    GridRef desirabilityGrid{};  // +0x1f8 (0x49b9e605)
    GridRef populationGrid{};    // +0x200 (0x49b9e605)
    GridRef capacityGrid{};      // +0x208 (0x49b9e604)
    GridRef maxCapacityGrid{};   // +0x210 (0x49b9e604)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

bool ParseGrowthDeveloperSubfile(
    std::span<const Uint8> subfileBytes,
    std::vector<GrowthDeveloperSaveRecord>& outRecords);

// Exact serialized record of cSC4BuildingDevelopmentSimulator (CLSID 0x8990be7a):
// Read @ 0x10014e064, Write @ 0x10014dd20.
// Complete method index:
//   cSC4BuildingDevelopmentSimulator() @ 0x10014bddc,
//   ~cSC4BuildingDevelopmentSimulator() @ 0x10014c00c / 0x10014c17e / 0x10014c188,
//   QI @ 0x10014c1b6, AddRef @ 0x10014c21c, Release @ 0x10014c246,
//   StaticInit @ 0x10014c312, StaticShutdown @ 0x10014c3a6,
//   InitStaticDataFromExemplar @ 0x10014c46a,
//   PostBuildingTypesUpdatedMessage @ 0x10014c534,
//   Init @ 0x10014ce64, InitDynamicDataFromExemplar @ 0x10014d666,
//   ReadSimulatorTunableValues @ 0x10014d71e, Shutdown @ 0x10014d8ec,
//   CreateDeveloper @ 0x10014e664, DoMessage @ 0x10014e84c,
//   DoIdle @ 0x10014ecba, DoDailySimulation @ 0x10014edcc,
//   DoMonthlySimulation @ 0x10014ee44, DoBuildingChanged @ 0x10014efda,
//   DoTerrainChanged @ 0x10014f222, ExecuteGrowthPlan @ 0x10014f8dc,
//   ExecuteDeclinePlan @ 0x10015001a, PlanGrowth @ 0x10015017e,
//   PlanDecline @ 0x10015060c, IsSuitableForConstruction @ 0x100150b0c,
//   PlopBuilding @ 0x100150c0c, LocationIsOKForNewBuilding @ 0x100150c2a,
//   GetStandardCityCellArea @ 0x100150cc6,
//   AddNewBuildingOccupant(cISC4BuildingOccupant*) @ 0x100150d3e,
//   AddNewBuildingOccupant(unsigned int, float*, int) @ 0x100150d7a,
//   LoadBuildingProperties(cISC4BuildingOccupant*, unsigned int) @ 0x100150fd8,
//   AddNewBuildingOccupant(unsigned int, int, int, int) @ 0x100151146,
//   GetCostOfOccupantDemolition @ 0x1001511da,
//   GetAllBuildingTypes(unsigned int*, unsigned int&) @ 0x100151218,
//   GetAllBuildingTypes(vector&) @ 0x10015125e,
//   GetAllBuildingFamilyTypes @ 0x1001512bc,
//   GetBuildingKeyFromType @ 0x10015131a,
//   IsBuildingTypeValid @ 0x10015135c, IsBuildingFamilyTypeValid @ 0x100151386,
//   GetBuildingTypesByFilter @ 0x1001513ca,
//   GetBuildingTypesByFamily(unordered_set&, unsigned int) @ 0x100151654,
//   GetBuildingTypesByFamily(vector&, unsigned int) @ 0x100151b08,
//   IsBuildingInFamily @ 0x100151ba0, GetAllFoundationTypes @ 0x100151bfc,
//   GetFoundationKeyFromType @ 0x100151c60,
//   GetAllLotRetainingWallTypes @ 0x100151ca2,
//   GetLotRetainingWallKeyFromType @ 0x100151d06,
//   AddNewFoundationOccupant(cISC4FoundationOccupant*) @ 0x100151d48,
//   AddNewFoundationOccupant(unsigned int, int, float*, float, float, int, float) @ 0x100151d84,
//   LoadFoundationProperties @ 0x100151fc4, AddFoundationData @ 0x1001520ee,
//   LoadLotRetainingWallProperties @ 0x100152162,
//   AddLotRetainingWallData @ 0x10015225a,
//   DemolishFoundationOccupant @ 0x1001522ce,
//   AddBuildingData @ 0x10015230a,
//   LoadBuildingProperties(cISCPropertyHolder*, unsigned int, bool) @ 0x1001523a4,
//   GetDeveloperType_Static @ 0x1001530e6,
//   SetBuildingCategoryFlags @ 0x100153180, GetMaxCapacity @ 0x1001533ee,
//   GetBuildingProfile @ 0x10015343e, GetBuildingProfilePtr @ 0x100153462,
//   GetBuildingName @ 0x100153490, GetExemplarName @ 0x1001534b4,
//   GetWealth_Static @ 0x1001534d8,
//   GetDeveloper(PurposeType, WealthType) @ 0x100153572,
//   GetDeveloper(DeveloperType) @ 0x10015359a,
//   GetFreeConstructionResources @ 0x1001535e4,
//   GetTotalConstructionResources @ 0x10015362a,
//   ClaimConstructionResources @ 0x100153644,
//   ReleaseConstructionResources @ 0x1001536b2,
//   GetStagePopulationMultiplier @ 0x1001536e8,
//   GetDayOfLastSuccessfulBuild @ 0x100153714,
//   MarkTract @ 0x100153758, IsTractUnmarked @ 0x1001537f4,
//   GetBuildingMapKey @ 0x100153896.
struct BuildingDevelopmentSimSaveRecord {
    using ObjectRef = GrowthDeveloperSaveRecord::GridRef;

    struct DeveloperMapEntry {
        Uint32 subType = 0;       // e.g. 0x1010..0x4400
        ObjectRef developer{};    // cSC4GrowthDeveloper (0x0990c138)
    };
    struct TargetGrowthStageEntry {
        Uint32 subType = 0;
        Uint8 maxGrowthStageSeen = 0;
    };
    struct GrowthMarkEntry {
        Uint32 markKey = 0;
        std::vector<std::pair<Sint32, Sint32>> tracts;
    };
    struct DeclinePlanEntry {
        ObjectRef developer{};    // cSC4GrowthDeveloper (0x0990c138)
        Sint32 tractX = 0;
        Sint32 tractZ = 0;
        Uint8 wealthOrStage = 0;
        Uint32 targetJulianDay = 0;
    };

    Uint32 memAddr = 0;
    Uint16 version = 4u;
    ObjectRef developmentMapGrid{}; // +0x80 (cSC4SimGrid<Uint8>, 0x49b9e602)
    ObjectRef energyDemand{};       // +0xe0 (cSC4Demand, 0x8990c12d)
    std::vector<DeveloperMapEntry> developers;              // +0xb8
    std::vector<ObjectRef> specialBuildings;                // +0x88
    std::vector<TargetGrowthStageEntry> targetGrowthStages; // +0xa0
    std::vector<GrowthMarkEntry> growthMarks;               // static mGrowthMarks
    std::vector<DeclinePlanEntry> declinePlans;             // +0x158 (mFailedGrowthCooldownMap)
    bool hasReachedStep10 = false;                          // +0x1c9 (CheatIgnorePlumbing)
    bool hasSeenStage7 = false;                             // +0x1ca (CheatIgnorePower)
    bool hasSeenStage8 = false;                             // +0x1cb (CheatIgnoreCaps)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;
};

// Exact static helpers from cSC4BuildingDevelopmentSimulator:
// GetDeveloperType_Static @ 0x1001530e6, GetWealth_Static @ 0x1001534d8,
// GetBuildingMapKey @ 0x1001538c6, DoBuildingChanged @ 0x10014efda,
// CalculateMaxCapacity @ 0x10014ccd9, GetMaxCapacity @ 0x1001533ee,
// ExecuteGrowthPlan @ 0x10014fa7d.
Uint32 GetDeveloperTypeFromPurposeAndWealth(
    Uint32 purpose,
    Uint32 wealth) noexcept;

Uint32 GetWealthFromDeveloperType(Uint32 developerType) noexcept;

constexpr Uint8 GetDevelopmentMapCellValue(
    Uint32 purpose,
    Uint32 wealth,
    bool isOccupied = true) noexcept {
    return isOccupied
        ? static_cast<Uint8>(1u + wealth + purpose * 4u)
        : 0u;
}

constexpr bool IsFailureReasonEligibleForGrowthCooldown(
    Uint32 failureReason) noexcept {
    // Exact bitmask 0x47dc00 @ 0x10014fa7d-0x10014fa8a:
    // bits 10, 11, 12, 14, 15, 16, 17, 18, 22.
    return failureReason <= 0x16u &&
           ((0x47dc00u >> failureReason) & 1u) != 0u;
}

constexpr bool ShouldKeepGrowthPlanAfterFailure(
    Uint32 failureReason) noexcept {
    // Exact branch @ 0x10014f95c: failureReason == 0x14 (out of construction resources)
    // retains the plan in mGrowthPlans and stops ExecuteGrowthPlan.
    return !IsFailureReasonEligibleForGrowthCooldown(failureReason) &&
           failureReason == 0x14u;
}

Float32 ComputeLotMaxCapacityPerCell(
    Uint16 buildingCapacity,
    Sint32 lotSizeX,
    Sint32 lotSizeZ) noexcept;

// ============================================================================
// cSC4DemandSimulator (CLSID 0x2990bff2):
// Exact decompilation of Read/Write, ReadSimulatorTunableValues,
// CreateDemandFromExemplar, DoDemandIndexLinks, ZeroCensusInfo,
// PerformCensus, GetNeutralTaxRate, and UpdateTaxAndEconomyModifiers.
// ============================================================================
inline constexpr cSC4TGI kDemandSimulatorTuningExemplarTGI{
    kTypeExemplar, 0xe7e2c2dbu, 0x4a1f3ce6u
};

inline constexpr std::array<Uint32, 24> kDemandCensusKeys = {
    0x1011u, 0x1021u, 0x1031u,
    0x2111u, 0x2112u, 0x2113u, 0x2114u,
    0x2121u, 0x2122u, 0x2123u, 0x2124u,
    0x2131u, 0x2132u, 0x2133u, 0x2134u,
    0x3111u, 0x3121u, 0x3131u,
    0x3321u, 0x3331u,
    0x4101u, 0x4201u, 0x4301u, 0x4401u
};

inline constexpr std::array<Uint32, 12> kSalaryDemandTypes = {
    0x2111u, 0x2112u, 0x2113u, 0x2114u,
    0x2121u, 0x2122u, 0x2123u, 0x2124u,
    0x2131u, 0x2132u, 0x2133u, 0x2134u
};

inline constexpr std::array<Uint32, 12> kActiveRCIDemandIds = {
    0x21010u, 0x21020u, 0x21030u, // R$, R$$, R$$$
    0x23110u, 0x23120u, 0x23130u, // Cs$, Cs$$, Cs$$$
    0x23320u, 0x23330u,           // Co$$, Co$$$
    0x24100u, 0x24200u, 0x24300u, 0x24400u // I-R, I-D, I-M, I-HT
};

inline constexpr std::array<Uint32, 12> kTaxModifierIds = {
    0x2a651010u, 0x2a651020u, 0x2a651030u,
    0x2a653110u, 0x2a653120u, 0x2a653130u,
    0x2a653320u, 0x2a653330u,
    0x2a654100u, 0x2a654200u, 0x2a654300u, 0x2a654400u
};

enum class DemandEconomyLinkType : Uint32 {
    DemandDrivesDemand = 0x47bb3f30u, // source.QueryNewDemand() * coeff -> target.AddToDemandValue()
    SupplyDrivesSupply = 0x47bb3f31u, // source.QueryNewSupply() * coeff -> target.AddToSupplyValue()
    DemandDrivesSupply = 0x47bb3f32u, // source.QueryNewDemand() * coeff -> target.AddToDemandValue()
    SatisfactionLink   = 0x47bb3f41u, // clamp(max(0,demand)/max(0.01,supply),0,1) -> target.SetDemandCap()
    AggregateChildren  = 0x47bb3f50u  // sum(target.QuerySupplyValue(), target.QueryDemandValue()) -> source
};

struct DemandEconomyLink {
    Uint32 sourceDemandId = 0;           // +0x1c / +0x24
    Uint32 linkSerial = 0;               // +0x20
    Uint32 targetDemandId = 0;           // +0x28
    DemandEconomyLinkType linkType = DemandEconomyLinkType::DemandDrivesDemand; // +0x2c
    Float32 coefficient = 1.0f;          // +0x30
};

struct DemandExemplarSpec {
    Uint32 demandId = 0;                 // 0x00000021
    Sint32 activeDemandMax = 0;          // 0x47bb3f20 (optional)
    Sint32 activeDemandMin = 0;          // 0x47bb3f21 (optional)
    bool hasActiveDemandMax = false;
    bool hasActiveDemandMin = false;
    Sint32 hysteresisBase = 0;           // 0x47bb3f10 (optional)
    Sint32 hysteresisMax = 0;            // 0x47bb3f11 (optional)
    bool hasHysteresisBase = false;
    bool hasHysteresisMax = false;
    Uint8 regionUse = 0;                 // 0x47bb3f60
    std::vector<DemandEconomyLink> links;

    bool LoadFromExemplar(const cGZExemplar& exemplar, Uint32 fallbackId = 0);
};

// Exact cSC4DemandSimulator::ReadSimulatorTunableValues @ 0x100224cb2.
struct DemandSimulatorTuning {
    cSC4ResponseCurve neutralTaxRateCurve; // 0x4a1f38b5 (+0x138)
    cSC4ResponseCurve taxModifierCurve;    // 0x4a1f38b6 (+0x140)
    Uint32 taxHistoryLookbackDays = 90u;   // 0x4a1f38b7 (+0x148)
    std::array<Float32, 13> neighborCoefficients{}; // 0x4a1f38c5 (+0x150)
    Float32 coeffC6 = 1.0f;                // 0x4a1f38c6 (+0x168)
    Float32 coeffC7 = 2.0f;                // 0x4a1f38c7 (+0x16c)
    Float32 passengerConnScale = 20.0f;    // 0x4a1f38c8 (+0x170)
    Float32 coeffC9 = 0.1f;                // 0x4a1f38c9 (+0x174)
    Float32 coeffCA = 20.0f;               // 0x4a1f38ca (+0x178)
    std::array<Uint32, 12> salaryWeights{
        100u, 100u, 100u, 100u, 100u, 100u,
        100u, 100u, 100u, 100u, 100u, 100u
    }; // 0x8a63f2a1 (+0xc8)

    bool LoadFromExemplar(const cGZExemplar& exemplar);

    // Exact GetNeutralTaxRate @ 0x100225e44:
    // evaluates neutralTaxRateCurve(supply(0x1001) + supply(0x3001) + supply(0x4001)).
    Float32 ComputeNeutralTaxRate(
        Float32 resSupply1001,
        Float32 comSupply3001,
        Float32 indSupply4001) const noexcept;

    // Exact tax modifier formula from UpdateTaxAndEconomyModifiers @ 0x10022722b:
    // evaluates taxModifierCurve((avgTaxRate * 0.1f) / max(0.1f, neutralTaxRate)).
    Float32 ComputeTaxRateModifier(
        Float32 avgTaxRate,
        Float32 neutralTaxRate) const noexcept;
};

// Exact serialized record of cSC4DemandSimulator (CLSID 0x2990bff2):
// Read @ 0x100225bc6, Write @ 0x100225a28.
// Complete method index:
//   cSC4DemandSimulator() @ 0x100223f6a, ~cSC4DemandSimulator() @ 0x10022417c /
//   0x10022422a / 0x100224234, QI @ 0x100224262, AddRef @ 0x1002242c8, Release @ 0x1002242f2,
//   StaticInit @ 0x1002243a0, StaticShutdown @ 0x10022446e, Init @ 0x100224520,
//   InitDynamicDataFromExemplar @ 0x100224982, ZeroCensusInfo @ 0x100224a42,
//   ReadSimulatorTunableValues @ 0x100224cb2, ResetRegionalDemands @ 0x1002250a4,
//   DoDemandIndexLinks @ 0x100225380, Shutdown @ 0x100225776,
//   GetNeutralTaxRate @ 0x100225e44, GetSimulatorType @ 0x100225eda,
//   AddToAgentEffectMap @ 0x100225ef4, DoMessage @ 0x100226278,
//   DoIdle @ 0x1002264b2, DoDailySimulation @ 0x100226b92,
//   DoMonthlySimulation @ 0x100226e40, SimulatorBegin @ 0x100226e6a,
//   AddAgentEffect @ 0x100226ed2, UpdateCapacity @ 0x100226fd6,
//   GetJobsBySensus @ 0x1002270de, UpdateTaxAndEconomyModifiers @ 0x10022722b,
//   PerformCensus @ 0x100227bfe, EvaluateConnectionUtilization @ 0x100227ea6,
//   ExportDemand @ 0x100228f18, CreateNewDemand @ 0x100229458,
//   AddDemandToSimulator @ 0x100229568, GetDemand @ 0x100229608,
//   CalculateJobsPerUnitOfDemand @ 0x100229678,
//   CreateDemandFromExemplar @ 0x1002297e2,
//   GetLocalPopulationSummary @ 0x10022a096,
//   ExtrapolateResidentialCensus @ 0x10022a2ec,
//   ExtrapolateBusinessCensus @ 0x10022a588.
struct DemandSimulatorSaveRecord {
    using ObjectRef = GrowthDeveloperSaveRecord::GridRef;

    struct DemandMapEntry {
        Uint32 demandId = 0;
        ObjectRef demand{}; // cSC4Demand (0x8990c12d)
    };
    struct CensusEntry {
        Uint32 censusKey = 0;
        Float32 value = 0.0f;
    };

    Uint32 memAddr = 0;
    Uint16 version = 2u;
    Float32 commercialConnectionBonus = 0.0f; // +0x128 (Co$$ 0x3320, Co$$$ 0x3330)
    Float32 industrialConnectionBonus = 0.0f; // +0x12c (I-R 0x4100..I-HT 0x4400)
    Float32 connectionUtilA = 0.0f;           // +0x130
    Float32 passengerConnectionSupply = 0.0f; // +0x134
    std::vector<DemandMapEntry> demands;      // +0x98
    std::vector<CensusEntry> census;          // +0x48

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;
};

// Exact PerformCensus cell contribution @ 0x100227bfe-0x100227e9a.
struct CensusCellInput {
    Uint32 buildingPurpose = 0;      // 1=R, 2=Cs, 3=Co, 5=I-R, 6=I-D, 7=I-M, 8=I-HT
    Uint32 buildingWealth = 0;       // 1..3 (or 0 for I-R)
    Sint16 developerPopulation = 0;  // from cSC4GrowthDeveloper::GetPopulationGrid(subType)
    Uint16 buildingCapacity = 0;     // from cISC4BuildingOccupant::GetCapacity(subType, 0)
    Float32 populationRetention = 1.0f; // from residential simulator (+0x90 vtable +0xf0)
    Float32 educationQuotient = 0.0f;   // from residential simulator (+0x90 vtable +0x118, 0..200)
};

struct CensusCellContribution {
    Uint32 primaryKey = 0;      // e.g. 0x1011..0x4401
    Float32 primaryValue = 0.0f;
    Uint32 eqKey = 0;           // 0x2111..0x2134 for residential (0 if non-residential)
    Float32 eqValue = 0.0f;
};

CensusCellContribution ComputeCensusCellContribution(
    const CensusCellInput& input) noexcept;

// Exact DoDemandIndexLinks propagation @ 0x100225380-0x1002256b6.
struct DemandNodeState {
    Uint32 demandId = 0;
    Uint8 regionUse = 0;       // 1=Residential, 2=Commercial/Industrial, 0=Internal
    Float32 supplyValue = 0.0f;      // +0x1c
    Float32 demandValue = 0.0f;      // +0x20
    Float32 newSupply = 0.0f;        // +0x24
    Float32 newDemand = 0.0f;        // +0x28
    Float32 activeDemandMax = 3.40282347e+38f;  // +0x34 (0x7f7fffff)
    Float32 activeDemandMin = -3.40282347e+38f; // +0x38 (0xff7fffff)
    Float32 activeDemand = 0.0f;     // +0x3c
    Float32 economyModifier = 0.0f;  // +0x44
    Float32 taxModifier = 0.0f;      // +0x48
    Float32 demandCap = 0.0f;        // +0x4c

    // Exact cSC4Demand::EndOfCycle @ 0x100200eda.
    bool EndOfCycle() noexcept;
};

// Runs the exact fixed-point economy-link propagation loop from
// cSC4DemandSimulator::DoDemandIndexLinks @ 0x100225380.
// Returns the number of outer passes executed until convergence.
Uint32 PropagateDemandEconomyLinks(
    std::span<const DemandEconomyLink> links,
    std::unordered_map<Uint32, DemandNodeState>& inOutDemands,
    Uint32 maxIterations = 32u);

// Exact AgentEffect parsed in cSC4DemandSimulator::AddToAgentEffectMap @ 0x100225ef4
// and applied in AddAgentEffect @ 0x100226ed2 / UpdateOccupantEffects @ 0x10022a7c6.
struct DemandAgentEffect {
    Uint32 demandId = 0;       // +0x10
    Float32 supplyEffect = 0.0f; // +0x14 (from 0x27812840 / 0x27812842, calls AddToSupplyValue)
    Float32 demandEffect = 0.0f; // +0x18 (from 0x27812841 / 0x27812843, calls AddToDemandValue)
};

bool ParseDemandAgentEffectsFromExemplar(
    const cGZExemplar& exemplar,
    std::vector<DemandAgentEffect>& outEffects,
    bool* outIsConditionalEffect = nullptr);

void ApplyDemandAgentEffects(
    std::span<const DemandAgentEffect> effects,
    bool addOrRemove,
    Float32 percentage,
    std::unordered_map<Uint32, DemandNodeState>& inOutDemands) noexcept;

// Exact EvaluateConnectionUtilization @ 0x100227ea6-0x100228ccf.
constexpr bool IsPassengerConnectionNetworkType(Uint32 networkType) noexcept {
    // Exact bitmask 0x150e @ 0x100227f2a-0x100227f37:
    // bits 1 (LightRail), 2 (Highway), 3 (Rail), 8 (Subway), 10 (GroundHighway), 12 (Monorail).
    return networkType <= 12u && ((0x150eu >> networkType) & 1u) != 0u;
}

struct ConnectionUtilizationInputs {
    Float32 prevConnectionUtilA = 0.0f;           // +0x130
    Float32 prevPassengerConnectionSupply = 0.0f; // +0x134
    Uint32 totalPassengerTrips = 0;               // sum of spTrafficSim->GetConnectionTripCount(conn, 3)
    Sint32 railAndMonorailCountA = 1;             // max(1, list3.size() + uniqueCities(list3 U list12))
    Sint32 subwayCountB = 1;                      // max(1, list8.size() + uniqueCities(list8 \ setA))
    Sint32 highwayCountC = 1;                     // max(1, list2.size() + uniqueCities(list2))
    Sint32 groundHighwayCountD = 1;               // max(1, list10.size() + uniqueCities(list10))
    Sint32 lightRailCountE = 1;                   // max(1, list1.size() + uniqueCities(list1))
    Sint32 roadAvenueFerryCountF = 1;             // max(1, list7.size() + list6.size() + uniqueCities(list7 U list11 U list6))
    Uint32 seaportTrips = 0;                      // +0x60->vtable +0x268->GetTotalTrips()
    Uint32 airportTrips = 0;                      // +0x60->vtable +0x260->GetTotalTrips()
};

struct ConnectionUtilizationOutputs {
    Float32 newConnectionUtilA = 0.0f;           // +0x130
    Float32 newPassengerConnectionSupply = 0.0f; // +0x134
    Float32 deltaSupply3800 = 0.0f;              // added to demand 0x3800 supply
    Float32 deltaSupply4800And4900 = 0.0f;       // added to demand 0x4800 and 0x4900 supply
};

ConnectionUtilizationOutputs EvaluateDemandConnectionUtilization(
    const DemandSimulatorTuning& tuning,
    const ConnectionUtilizationInputs& inputs) noexcept;

// Exact DoIdle step 2 average city salary calculation @ 0x1002267be-0x10022682c:
// sums truncated supplyValue of the 12 EQ brackets (0x2111..0x2134) weighted by
// salaryWeights (0x8a63f2a1).
Uint32 ComputeAverageCitySalary(
    const DemandSimulatorTuning& tuning,
    const std::array<Float32, 12>& eqBracketSupplies) noexcept;

// Exact ExtrapolateResidentialCensus @ 0x10022a2ec-0x10022a587.
struct ExtrapolatedResidentialCensus {
    Uint32 eqKeys[4]{};       // subType + 0x1101..0x1104 (e.g. 0x2111..0x2114)
    Uint32 regionalDemandKeys[4]{}; // subType + 0x11101..0x11104 (e.g. 0x12111..0x12114)
    Float32 extrapolatedValues[4]{};
};

bool ExtrapolateResidentialCensusValues(
    Uint32 subType,             // 0x1010, 0x1020, or 0x1030
    Float32 exportedUnits,
    Sint32 neighborBasePop,     // neighbor->GetStatistic(subType)
    Sint32 neighborActivePop,   // neighbor->GetStatistic(subType + 1)
    Float32 neighborRetention,  // *(neighbor->GetRetentionPtr())
    const std::array<Sint32, 4>& neighborEqCounts, // neighbor->GetStatistic(subType + 0x1101..0x1104)
    ExtrapolatedResidentialCensus& outResult) noexcept;

// Exact ExtrapolateBusinessCensus @ 0x10022a588-0x10022a7bf.
struct ExtrapolatedBusinessCensus {
    Uint32 regionalSupplyKey = 0; // subType + 0x10001 (e.g. 0x13111..0x14401)
    Float32 extrapolatedActiveJobs = 0.0f;
    std::array<Float32, 3> workforceDemandByWealth{}; // added to 0x12010, 0x12020, 0x12030
};

bool ExtrapolateBusinessCensusValues(
    Uint32 subType,             // 0x3110..0x3130, 0x3320..0x3330, 0x4100..0x4400
    Float32 exportedUnits,
    Sint32 neighborBaseJobs,    // neighbor->GetStatistic(subType)
    Sint32 neighborActiveJobs,  // neighbor->GetStatistic(subType + 1)
    const std::array<Float32, 3>& jobsPerUnitByWealth, // from 0x47bb3f30 / 100.0f
    ExtrapolatedBusinessCensus& outResult) noexcept;

// Exact cSC4GrowthDeveloper::UpdatePopulationCount @ 0x100158140-0x10015837a:
// distributes lot capacity across capacityGrid (+0x208) cells (quotient per cell +
// remainder at lot anchor cell), clamping underflow to 0, and updates totalCapacity (+0x1e4).
struct LotCapacityUpdateInput {
    Uint32 developerPurpose = 0; // +0x68
    Uint32 buildingPurpose = 0;  // profile.purpose (+0x00)
    Uint16 lotCapacity = 0;      // lot->GetCapacity(subType, 1)
    bool addOrRemove = true;     // true = add, false = remove
    Uint8 sizeX = 1;             // lot->GetSize()
    Uint8 sizeZ = 1;
    Sint32 minX = 0;             // lot->GetBoundingRect()
    Sint32 minZ = 0;
    Sint32 maxX = 0;
    Sint32 maxZ = 0;
    Sint32 locationX = 0;        // lot->GetLocation()
    Sint32 locationZ = 0;
    Sint32 tractShift = 2;       // capacityGrid->GetTractShift()
    Sint32 gridTractsX = 0;
    Sint32 gridTractsZ = 0;
};

bool UpdateDeveloperCapacityGrid(
    const LotCapacityUpdateInput& input,
    std::span<Uint16> inOutCapacityGrid,
    Uint32& inOutTotalCapacity) noexcept;

// Exact cSC4BuildingDevelopmentSimulator::Grow tract-mark selection & wrap-around
// @ 0x10014fe55-0x10014feef (with IsTractUnmarked @ 0x1001537f4 and MarkTract @ 0x100153758).
bool SelectNextUnmarkedGrowthTract(
    std::span<const GrowthDeveloperItem> growthList,
    std::vector<std::pair<Sint32, Sint32>>& inOutMarkedTracts,
    GrowthDeveloperItem& outSelected) noexcept;

// ============================================================================
// cSC4LotConfiguration, cSC4LotConfigurationManager (CLSID 0x299062c0),
// and cSC4LotDeveloper (CLSID 0xa990bfe0):
// Exact decompilation of Load @ 0x1004032ee, ConvertPropertiesToObjectArray @ 0x100403a60,
// CorrectObjectPositions @ 0x100403d8e, ReadSimulatorTunableValues @ 0x10013f4ae,
// InitBoundaryRules @ 0x10013f67e, GetBoundaryLotTexture @ 0x10014658c,
// GetPositionForLotConfigurationObject @ 0x1001442e0, and Read/Write serialization.
// ============================================================================
enum class LotConfigObjectType : Uint8 {
    Building    = 0,
    Prop        = 1,
    BaseTexture = 2,
    Foundation  = 3,
    Flora       = 4,
    NetworkNode = 5,
    Other       = 6
};

// Exact 0x40-byte cSC4LotConfigurationObject decoded in
// cSC4LotConfiguration::ConvertPropertiesToObjectArray @ 0x100403a60.
struct cSC4LotConfigurationObject {
    Uint8 type = 0;          // +0x00 (LotConfigObjectType)
    Uint8 lodAndFlags = 0;   // +0x01
    Uint8 orientation = 0;   // +0x02 (0..3)
    Uint8 mirrorFlag = 0;    // +0x03 (vec[2] >> 31)
    Float32 posX = 0.0f;     // +0x04 (vec[3] / 65536.0)
    Float32 posY = 0.0f;     // +0x08 (vec[4] / 65536.0)
    Float32 posZ = 0.0f;     // +0x0c (vec[5] / 65536.0)
    Float32 minX = 0.0f;     // +0x10 (vec[8] / 65536.0)
    Float32 minZ = 0.0f;     // +0x14 (vec[9] / 65536.0)
    Float32 maxX = 1.0f;     // +0x18 (vec[6] / 65536.0)
    Float32 maxZ = 1.0f;     // +0x1c (vec[7] / 65536.0)
    Uint32 usageFlags = 0;   // +0x20 (vec[10])
    Uint32 objectId = 0;     // +0x24 (vec[11] when version > 1)
    std::vector<Uint32> descriptorIds; // +0x28 (vec[12..N-1])
};

// Exact cSC4LotConfiguration (0x138 bytes) decoded in Load @ 0x1004032ee.
struct cSC4LotConfiguration {
    Uint32 id = 0;                               // +0x0c (key.instance)
    std::string name;                            // +0x10 (0x00000020)
    Uint8 version = 0;                           // 0x88edc789
    Uint8 sizeX = 0;                             // +0x38 (0x88edc790[0])
    Uint8 sizeZ = 0;                             // +0x39 (0x88edc790[1])
    std::vector<Uint32> familyIds;               // +0x40 (0x88edc791)
    Float32 maxSlope = 90.0f;                    // +0x58 (0xe99b068c)
    Float32 minSlope = 0.0f;                     // +0x5c (0x699b08a4)
    Float32 maxSlopeBeforeFoundation = 90.0f;    // +0x60 (0x88edc792)
    Uint8 growthStage = 1u;                      // +0x64 (0x27812837)
    Uint16 minBuildingCapacity = 0xffffu;        // +0x66
    Uint16 maxBuildingCapacity = 0u;             // +0x68
    bool isCompatibleWithWater = false;          // +0x6a (0x89136440)
    Uint16 zoneTypeMask = 0u;                    // +0x70 (0x88edc793, 16 bits)
    Uint8 wealthTypeMask = 0u;                   // +0x88 (0x88edc795, 4 bits)
    Uint16 purposeTypeMask = 0u;                 // +0xa0 (0x88edc796, 9 bits)
    std::vector<Uint32> styleIds;                // +0xb8
    bool doConstruction = false;                 // +0xd1 (0xe99b068d)
    Uint8 roadRequirementFlags = 0x08u;          // +0xd2 (0x4a4a88f0, bit 3 forced on)
    std::vector<cSC4LotConfigurationObject> objects; // +0xd8 (0x88edc900..0x88edcdff)
    std::vector<Uint32> retainingWallIds;        // +0xf0 (0x88edc798)
    std::vector<Uint32> fenceIds;                // +0x108 (0x88edc797)
    std::vector<std::array<Sint8, 4>> fenceSegments; // +0x120 (0x88edc797)

    bool LoadFromExemplar(
        const cGZExemplar& exemplar,
        Uint32 instanceId,
        Uint8 detailLevel = 2u);

    static bool CorrectObjectPositions(
        std::vector<cSC4LotConfigurationObject>& inOutObjects,
        Float32 lotWidthX,
        Float32 lotDepthZ) noexcept;

    bool BelongsToFamily(Uint32 familyId) const noexcept;
    bool IsCompatibleWithZoneType(Sint32 zoneType) const noexcept {
        return zoneType >= 0 && zoneType < 16 &&
               ((zoneTypeMask >> zoneType) & 1u) != 0u;
    }
    bool IsCompatibleWithWealthType(Sint32 wealthType) const noexcept {
        return wealthType >= 0 && wealthType < 4 &&
               ((wealthTypeMask >> wealthType) & 1u) != 0u;
    }
    bool IsCompatibleWithPurposeType(Sint32 purposeType) const noexcept {
        return purposeType >= 0 && purposeType < 9 &&
               ((purposeTypeMask >> purposeType) & 1u) != 0u;
    }
    std::array<bool, 4> GetRequiredRoads() const noexcept {
        return {
            (roadRequirementFlags & 0x01u) != 0u,
            (roadRequirementFlags & 0x02u) != 0u,
            (roadRequirementFlags & 0x04u) != 0u,
            (roadRequirementFlags & 0x08u) != 0u
        };
    }

    const cSC4LotConfigurationObject* GetPrimaryBuildingObject() const noexcept;
    const cSC4LotConfigurationObject* GetPrimaryBaseTextureObject() const noexcept;
    const cSC4LotConfigurationObject* GetPrimaryFoundationObject() const noexcept;
};

// Exact serialized record of cSC4LotConfigurationManager (CLSID 0x299062c0):
// Read @ 0x1006c3586, Write @ 0x1006c3526.
struct LotConfigurationManagerSaveRecord {
    using ObjectRef = GrowthDeveloperSaveRecord::GridRef;

    Uint32 memAddr = 0;
    Uint16 version = 1u;
    ObjectRef buildingDevelopmentSim{}; // +0x20 (0x8990be7a)
    ObjectRef floraSim{};               // +0x28 (0xc990c034)
    ObjectRef propManager{};            // +0x30 (0xa990bd10)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;
};

// Exact cSC4LotDeveloper::ReadSimulatorTunableValues @ 0x10013f4ae.
struct LotDeveloperTuning {
    Uint16 constructionResourcePerTick = 0; // 0x47e2c400 (+0x118)
    std::array<Float32, 3> stageTimeFractions{0.2f, 0.4f, 0.6f}; // 0x47e2c401 (+0x124, +0x120, +0x11c)
    Float32 minFoundationSlope = 0.0f;      // 0xe900ea24 (+0x128)
    Float32 foundationSlopeThreshold = 0.0f; // 0xe900ea25 (+0x12c)
    Uint32 defaultFoundationId = 0;         // 0xe900ea26 (+0x130)
    Uint32 defaultRetainingWallId = 0;      // 0x6916516b (+0x134)
    bool attemptSlopeLevelling = false;     // 0xe900ea27 (+0x138)
    bool attemptFlatLevelling = false;      // 0xe900ea28 (+0x139)
    Float32 maxFlatSlope = 0.0f;            // 0xe900ea29 (+0x13c)
    Float32 maxLevellingDelta = 0.0f;       // 0xe900ea2a (+0x140)
    Uint32 propUpdateBatchSize = 0;         // 0xe900ea2b (+0xf0)
    Uint32 buildingUpdateBatchSize = 0;     // 0xe900ea2c (+0xec)
    cSC4ResponseCurve floraElevationCurve;  // 0x2a2d7824 (+0x148)

    bool LoadFromExemplar(const cGZExemplar& exemplar);
};

// Exact cSC4LotDeveloper::InitBoundaryRules @ 0x10013f67e and
// GetBoundaryLotTexture @ 0x10014658c.
struct BoundaryLotTextureRule {
    Uint32 textureId = 0x3000u;
    Uint8 rotation = 0u;
    bool mirror = false;
};

BoundaryLotTextureRule LookupBoundaryLotTexture(
    const std::array<Sint32, 4>& boundaryTypes) noexcept;

// Exact cSC4LotDeveloper::GetPositionForLotConfigurationObject @ 0x1001442e0.
struct LotObjectWorldPlacement {
    Float32 worldX = 0.0f;
    Float32 worldY = 0.0f;
    Float32 worldZ = 0.0f;
    Float32 boxMinX = 0.0f;
    Float32 boxMinZ = 0.0f;
    Float32 boxMaxX = 0.0f;
    Float32 boxMaxZ = 0.0f;
    Sint32 orientation = 0;
};

LotObjectWorldPlacement ComputeLotObjectWorldPlacement(
    Sint32 lotMinCellX,
    Sint32 lotMinCellZ,
    Sint32 lotMaxCellX,
    Sint32 lotMaxCellZ,
    Sint32 lotFacing,
    Float32 baseAltitudeY,
    const cSC4LotConfigurationObject& obj) noexcept;

// Exact serialized record of cSC4LotDeveloper (CLSID 0xa990bfe0):
// Read @ 0x100140242, Write @ 0x10014009a.
// Complete method index:
//   cSC4LotDeveloper() @ 0x10013eb1e, ~cSC4LotDeveloper() @ 0x10013ed08 /
//   0x10013edc2 / 0x10013edcc, QI @ 0x10013edfa, AddRef @ 0x10013ee60, Release @ 0x10013ee8a,
//   Init @ 0x10013eef0, ReadSimulatorTunableValues @ 0x10013f4ae,
//   InitBoundaryRules @ 0x10013f67e, Shutdown @ 0x10013fcda,
//   DoMessage @ 0x10014053c, DoDailySimulation @ 0x100140b64,
//   DoAnimationTick @ 0x100140db0, DoOccupantInserted @ 0x100140ea4,
//   DoOccupantRemoved @ 0x100141052, SwitchNightChangeBuildings @ 0x1001412dc,
//   AddLotProps @ 0x1001415a4, AddLotFlora @ 0x10014162e,
//   AddNetworkConnectionNodes @ 0x100141724, DoPowerChanged @ 0x1001419da,
//   UpdateLot @ 0x100141cb8, LightBuildingAndProps @ 0x10014217a,
//   RemoveBuildingZotProp @ 0x1001426a2, AddBuildingZotProp @ 0x100142762,
//   StartDemolishLot @ 0x100142966, RemoveLotProps @ 0x1001429ec,
//   RemoveNetworkConnectionNodes @ 0x100142a46,
//   ClearLotBuildings @ 0x100142bb8, EndDemolishLot @ 0x100142c8a,
//   ClearLot @ 0x100142cfc, ExplodeLot @ 0x100142db4,
//   AddLotRubbleProps @ 0x1001430bc, UpdateSurroundingLotFoundations @ 0x100143452,
//   StartConstruction @ 0x1001434f8, PickUnitToJoin @ 0x1001439a2,
//   ClearLotBlockingObjects @ 0x100143d56, ConstructDefaultLot @ 0x100143ebc,
//   GetFirstBuilding @ 0x100144218, GetPositionForLotConfigurationObject @ 0x1001442e0,
//   GetPositionForBuilding @ 0x1001446e0, ChangeBuilding @ 0x10014475a,
//   AbandonBuilding @ 0x10014491e, ReoccupyBuilding @ 0x100144942,
//   UpdateLotElements @ 0x100144a46, UpdateLotObjectAlitudes @ 0x100144b74,
//   GetFoundationByOccupant @ 0x100144e12, GetFoundationByStandardCityCell @ 0x100144ea2,
//   GetFoundationByPosition @ 0x100144fc6, GetLotFoundation @ 0x100145134,
//   GetLotFoundationBottomAltitude @ 0x10014517e,
//   PickLotFoundationAltitude @ 0x100145252,
//   AddLotFoundation(cISC4Lot*, unsigned int, bool, bool, bool) @ 0x1001453b2,
//   GetCornerAltitudes @ 0x1001455ce,
//   AddLotFoundation(cISC4Lot*, cISC4LotConfiguration*, bool, bool, bool) @ 0x100145658,
//   RemoveFoundation @ 0x100145724,
//   RemoveLotFoundation(int, int) @ 0x100145820,
//   RemoveLotFoundation(cISC4Lot*) @ 0x1001458d2,
//   UpdateLotFoundation @ 0x10014593e,
//   IsLotObjectFoundationRequired(float*, float*) @ 0x1001459da,
//   IsLotObjectFoundationRequired(cISC4Occupant*) @ 0x100145a62,
//   AddBuildingFoundation(cISC4BuildingOccupant*, unsigned int) @ 0x100145a8a,
//   AddBuildingFoundation(cISC4BuildingOccupant*, cISC4LotConfiguration*) @ 0x100145b88,
//   RemoveBuildingFoundation @ 0x100145c0a, RemoveLotProp @ 0x100145c82,
//   PickFloraTypeForLot @ 0x100145d6e, GetLotBaseTextureOccupant @ 0x100145e4c,
//   UpdateLotBaseTextureOccupant @ 0x100145f70,
//   SetForceEmptyLotBaseTextures @ 0x100145fb8,
//   SetEmptyLotBaseTexture @ 0x100146066, RemoveLotBaseTexture @ 0x10014652e,
//   GetBoundaryLotTexture @ 0x10014658c, SetNormalLotBaseTexture @ 0x1001466e6,
//   PickLotBaseTextureForLot @ 0x1001469d8, GetLotBaseTextures @ 0x100146b10,
//   RemoveAndReleaseOccupants @ 0x100146b72, SelectAndCreateLotProp @ 0x100146bdc,
//   CreateLotProp @ 0x100146d36, GetTerrainAltitudeByVertex @ 0x100146e0a,
//   GetLowestOrHighestAreaAltitude(SC4Rect<int> const&, bool, bool) @ 0x100146e64,
//   GetLowestOrHighestAreaAltitude(SC4Rect<float> const&, bool, bool) @ 0x100146f24,
//   GetAverageAreaAltitude @ 0x100147318, LevelAreaInternalVertices @ 0x1001473c2,
//   LevelBorderingVertices @ 0x1001474da, AttemptAreaLevelling @ 0x100147516,
//   GetOccupantCellCoverage @ 0x1001475e0, IsLotDeveloped @ 0x1001476c4,
//   IsAreaFlat @ 0x1001476fa, IsLotFlat @ 0x1001477ac,
//   GetAreaAltitudeRange @ 0x1001477ee, GetLotAltitudeRange @ 0x1001478a6,
//   GetAreaSlope @ 0x1001478e8, GetLotSlope @ 0x100147a92,
//   GetOccupantSlope @ 0x100147ad4, TestPower @ 0x100147b22,
//   TestTravel @ 0x100147dd0, TestDesirability @ 0x100148090,
//   TestWater @ 0x100148222, TestCondition @ 0x1001483c8,
//   TestCoffeeBar @ 0x100148540, NeedsAnchorBuilding @ 0x100148754,
//   UpdateTravelDesignates @ 0x100148924, UpdateTravelPopulations @ 0x100148bb8,
//   GetLotConfigurationObjectOccupant @ 0x100148c3c,
//   GetLotDevelopmentCost @ 0x100148e18,
//   RemoveLotFromItsBusinessUnit @ 0x100148e90.
struct LotDeveloperSaveRecord {
    using ObjectRef = GrowthDeveloperSaveRecord::GridRef;

    Uint32 memAddr = 0;
    Uint16 version = 1u;
    Sint32 field78 = 0;                       // +0x78
    Float32 field7C = 0.0f;                   // +0x7c
    Float32 field80 = 0.0f;                   // +0x80
    std::vector<ObjectRef> buildingQueue;     // +0xa8..+0xd0 (cSC4BuildingOccupant, 0xa9bd882d)
    Uint32 currentQueueIndex = 0;             // iterator offset in buildingQueue
    Uint8 nightChangeMode = 0;                // +0xe8
    bool forceEmptyLotBaseTextures = false;   // +0xf4

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;
};

// ============================================================================
// cSC4NetworkManager (CLSID 0xc990bd46), cSC4NetworkLotManager (CLSID 0xe9a6cd3b),
// cSC4TileNeighborhoodCompact, cSC4TileInfo, cSC4TileRule, cSC4TileEvaluator,
// and cSC4NetworkTool RUL0 / RUL1 / RUL2 Network Auto-Tiler:
// Exact decompilation of cSC4TileInfo (0x1001aeb20..0x1001aeca0),
// cSC4TileNeighborhoodCompact (0x1001aee18..0x1001af140),
// cSC4TileRule (0x1001af1d0..0x1001af8f4),
// cSC4TileEvaluator (0x1001af904..0x1001b041c),
// cSC4NetworkManager (0x10016dfb0..0x100171e1e),
// cSC4NetworkTool::ReloadRules (0x100186d56), SolveNetwork (0x1001b5e8e),
// and cSC4NetworkLotManager (0x100404d80..0x100408320).
// ============================================================================
enum class SC4NetworkType : Uint8 {
    Road          = 0,
    Rail          = 1,
    Highway       = 2,
    Street        = 3,
    Pipe          = 4,
    PowerLine     = 5,
    Avenue        = 6,
    Subway        = 7,
    LightRail     = 8,
    Monorail      = 9,
    OneWayRoad    = 10,
    DirtRoad      = 11,
    GroundHighway = 12,
    Count         = 13
};

inline constexpr Uint32 GetFlagForNetworkType(SC4NetworkType type) noexcept {
    const auto idx = static_cast<Uint8>(type);
    return idx < 13u ? (1u << idx) : 0u;
}

inline constexpr Sint32 GetNetworkMultitypePriority(SC4NetworkType type) noexcept {
    constexpr std::array<Sint32, 13> kPriorities{
        3, 5, 6, 2, 1, 0, 4, 1, 7, 7, 3, 0, 5
    };
    const auto idx = static_cast<Uint8>(type);
    return idx < 13u ? kPriorities[idx] : 0;
}

const char* GetEnglishNetworkName(SC4NetworkType type) noexcept;
bool ParseNetworkNameToId(std::string_view name, SC4NetworkType& outType) noexcept;

// Exact cSC4TileInfo::ConvertMetaToRelativePos @ 0x1001aeb20 and inverse
// sMetaForRelativePos lookup from 0x100a1a320..0x100a1a360.
void ConvertMetaToRelativePos(Uint8 metaPos, Sint8& outRelX, Sint8& outRelZ) noexcept;
Uint8 ConvertRelativePosToMeta(Sint8 relX, Sint8 relZ) noexcept;
Uint8 RotateMetaPosition(Uint8 metaPos, Sint32 rotation) noexcept;
Uint8 MirrorMetaPosition(Uint8 metaPos) noexcept;

// Exact cSC4TileNeighborhoodCompact (0x1001aee18..0x1001af140):
// 32-bit packed representation of W(0..7), N(8..15), E(16..23), S(24..31) edges.
// Within each 8-bit edge byte:
//   bits 0..1: major connection direction (0=none, 1=diagonal CW, 2=straight, 3=diagonal CCW)
//   bit 2:     one-way / direction flag
//   bits 3..7: minor subtype (encoded in RUL text as (minor * 10 + major))
struct cSC4TileNeighborhoodCompact {
    Uint32 packed = 0;

    constexpr bool operator==(const cSC4TileNeighborhoodCompact&) const noexcept = default;
    constexpr bool operator<(const cSC4TileNeighborhoodCompact& other) const noexcept {
        return packed < other.packed;
    }

    Uint8 GetEdgeCondition(Sint32 edge) const noexcept;
    void SetEdgeCondition(Sint32 edge, Uint8 value) noexcept;
    void SetEdgeTypeMinor(Sint32 edge, Uint8 minorType) noexcept;
    void ResetEdgeMinorTypes() noexcept;
    void SetEdgeConnectionDirection(Sint32 edge, Uint8 direction) noexcept;
    bool HasConnectionOnEdge(Sint32 edge) const noexcept;
    void GetStraightAndDiagonalEdges(
        std::array<Sint32, 4>& outStraightEdges,
        std::array<Sint32, 4>& outDiagonalEdges,
        Sint32& outStraightCount,
        Sint32& outDiagonalCount) const noexcept;
    cSC4TileNeighborhoodCompact GetMirrorImageEquivalent() const noexcept;
    cSC4TileNeighborhoodCompact GetRotationalEquivalent(Sint32 rotation) const noexcept;
    void Merge(const cSC4TileNeighborhoodCompact& other) noexcept;

    static Uint8 EncodeRulEdgeToken(Sint32 decimalToken) noexcept;
};

// Exact cSC4TileInfo (8 bytes, 0x1001aeb20..0x1001aec9f):
// Represents a solved piece placement at metaPos (0..24) with pieceId and rotationAndFlip.
struct cSC4TileInfo {
    Uint32 pieceId = 0;         // +0x00
    Uint8 metaPos = 0;          // +0x04 (0..24 in 5x5 neighborhood)
    Uint8 rotationAndFlip = 0;  // +0x05 (bits 0..1 = rot 0..3, bit 7 = flip)

    void GetCell(Uint32 centerX, Uint32 centerZ, Uint32& outX, Uint32& outZ) const noexcept;
    cSC4TileInfo GetRotationalEquivalent(Sint32 rotation) const noexcept;
    cSC4TileInfo GetMirrorImageEquivalent() const noexcept;
    Uint8 GetRotation() const noexcept { return rotationAndFlip & 0x03u; }
    bool GetFlip() const noexcept { return (rotationAndFlip & 0x80u) != 0u; }
};

// Exact tCompactNeighborInfo (12 bytes, 0x1001af2ae..0x1001af575):
// Condition on neighbor at metaPos: ((actual.packed ^ expected.packed) & mask.packed) == 0.
struct tCompactNeighborInfo {
    Uint8 metaPos = 0;                      // +0x00
    cSC4TileNeighborhoodCompact expected{}; // +0x04
    cSC4TileNeighborhoodCompact mask{};     // +0x08 (0xff per non-'?' edge, 0x00 for '?')

    tCompactNeighborInfo GetMirrorImageEquivalent() const noexcept;
    tCompactNeighborInfo GetRotationalEquivalent(Sint32 rotation) const noexcept;
};

// Exact cSC4TileRule (0x40 bytes, 0x1001af1d0..0x1001af8f4):
// Single RUL2/RUL1 rule with center condition, optional neighbor conditions (type 2),
// piece solutions (type 3), and symmetry mask (+0x38: 0xff=all, 0x0f=noflip, 0x01=notrans).
struct cSC4TileRule {
    cSC4TileNeighborhoodCompact centerCondition{}; // +0x00
    std::vector<tCompactNeighborInfo> conditions;  // +0x08
    std::vector<cSC4TileInfo> solutions;           // +0x20
    Uint32 transformFlags = 0xffu;                 // +0x38 (0xff, 0x0f="noflip", 0x01="notrans")

    cSC4TileRule GetMirrorImageEquivalent() const noexcept;
    cSC4TileRule GetRotationalEquivalent(Sint32 rotation) const noexcept;
    bool IsEquivalentByCondition(const cSC4TileRule& other) const noexcept;
};

// Exact cSC4TileEvaluator (0x1001af904..0x1001b041c):
// Loads RUL text resources, transmogrifies rules across 8 dihedral symmetries when
// "#Transmogrify" header is present, and evaluates neighborhood matches.
class cSC4TileEvaluator {
public:
    bool LoadFromText(std::string_view rulText);

    // Evaluates all rules matching centerCondition and optional neighbor callback.
    const std::vector<cSC4TileInfo>* Lookup(
        cSC4TileNeighborhoodCompact centerCondition,
        const std::vector<std::pair<Uint8, cSC4TileNeighborhoodCompact>>& neighborConditions = {}) const noexcept;

    std::size_t GetRuleCount() const noexcept { return mRules.size(); }
    bool WasTransmogrified() const noexcept { return mTransmogrified; }

    static void TransmogrifyRuleSet(
        std::vector<cSC4TileRule>& outRules,
        std::span<const cSC4TileRule> rawRules);

private:
    bool mTransmogrified = false;
    std::vector<cSC4TileRule> mRules; // sorted by centerCondition.packed
};

// Exact cSC4NetworkManager::TextureRemapEntry (24 bytes, 0x10016ec20..0x10016ee35)
// and PathRemapEntry (16 bytes, 0x10016ee44..0x10016f1d4).
struct NetworkTextureRemapEntry {
    Uint32 matchValue = 0;      // +0x00
    Uint32 matchMask = 0;       // +0x04
    Uint32 addOffset = 0;       // +0x08
    Uint32 keepMask = 0;        // +0x0c
    Uint32 subtractMask = 0;    // +0x10
    Uint8 rotFlipDelta = 0;     // +0x14 (bits 0..1 = rot, bit 7 = flip)
    bool continueRemap = false; // +0x15
};

struct NetworkShadowRemapByEdgesEntry {
    Uint32 pieceId = 0;         // +0x00
    Uint32 networkType = 0;     // +0x04 (0..12)
    Uint32 addOffset = 0;       // +0x08
    Uint32 keepMask = 0;        // +0x0c
    Uint8 rotFlip = 0;          // +0x10
};

struct NetworkPathRemapEntry {
    Uint32 matchValue = 0;      // +0x00
    Uint32 matchMask = 0;       // +0x04
    Uint32 addOffset = 0;       // +0x08
    Uint32 keepMask = 0;        // +0x0c
};

struct NetworkTileConflictRule {
    Uint32 pieceA = 0;          // +0x1c / +0x20
    Uint8 rotFlipA = 0;         // +0x24
    Uint32 pieceB = 0;          // +0x28
    Uint8 rotFlipB = 0;         // +0x2c
    Uint32 replacementA = 0;    // +0x30 (0 = invalidate & reslove)
    Uint8 replacementRotFlipA = 0; // +0x34
    Uint32 replacementB = 0;    // +0x38
    Uint8 replacementRotFlipB = 0; // +0x3c
};

struct MixedTypeIntersectionSolution {
    Uint32 edgeConditionA = 0;
    Uint32 edgeConditionB = 0;
    Uint32 pieceId = 0;
    Uint8 rotationAndFlip = 0;
};

struct NetworkSolvedCell {
    Uint32 pieceId = 0;         // +0x00
    Uint8 rotationAndFlip = 0;  // +0x04
    Uint32 packedCellXZ = 0;    // +0x08 ((z << 16) | x)
};

// Exact PatchTilePair @ 0x1001b9124 (returns 0=no match, 1=patched, 2=invalidated).
Uint32 PatchNetworkTilePair(
    std::span<const NetworkTileConflictRule> rulesForPieceA,
    NetworkSolvedCell& inOutCellA,
    NetworkSolvedCell& inOutCellB,
    Sint32 dirFromAToB) noexcept;

// Exact cSC4NetworkManager::MapPreviewTexture @ 0x10016ec20,
// MapShadowTexture @ 0x10016ece4, and MapPath @ 0x10016ee44.
struct cSC4NetworkRuleTables {
    std::vector<NetworkTextureRemapEntry> textureRemaps;
    std::vector<NetworkTextureRemapEntry> shadowRemaps;
    std::vector<NetworkShadowRemapByEdgesEntry> shadowRemapsByEdges;
    std::vector<NetworkPathRemapEntry> ukPathRemaps;
    std::array<Uint32, 13> allowedOverridesMatrix{};
    std::vector<NetworkTileConflictRule> tileConflictRules;
    std::vector<std::pair<std::pair<SC4NetworkType, SC4NetworkType>, MixedTypeIntersectionSolution>>
        mixedTypeSolutions;

    bool LoadNetworkIni(std::string_view iniText);
    bool LoadMultitypeRul(std::string_view rulText);
    bool LoadOverrideRul(std::string_view rulText);

    Uint32 MapPreviewTexture(Uint32 textureId, Uint8& inOutRotFlip) const noexcept;
    Uint32 MapShadowTexture(
        Uint32 shadowId,
        Uint8& inOutRotFlip,
        Uint32 networkTypeFlags,
        Uint32 edgeKey) const noexcept;
    Uint32 MapPath(Uint32 pathId, bool leftHandDrive) const noexcept;
};

// Exact serialized record of cSC4NetworkManager (CLSID 0xc990bd46):
// Read @ 0x100171cb2, Write @ 0x100171d8c.
struct NetworkManagerSaveRecord {
    using ObjectRef = GrowthDeveloperSaveRecord::GridRef;

    Uint32 memAddr = 0;
    Uint16 version = 1u;
    std::vector<ObjectRef> connectionNodes; // cISC4NetworkOccupant (GZIID 0xa821ef94)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;
};

// Exact cSC4NetworkLotConfiguration (0xa0 bytes) loaded in
// cSC4NetworkLotManager::ReadLotConfiguration @ 0x100405946 (ExemplarType 0x10 == 0x21).
struct cSC4NetworkLotConfiguration {
    Uint32 id = 0;                              // +0x20 (0x00000021)
    std::string name;                           // +0x28 (0x00000020)
    Uint8 version = 1u;                         // 0x88edc789
    Uint32 networkPieceId = 0;                  // +0x50 (0xc9a5a1be)
    std::array<Uint8, 4> positionStrideMask{0xffu, 0xffu, 0xffu, 0xffu}; // +0x54 (0x49d55951)
    Uint8 positionModulo = 1u;                  // +0x58 (0xca81b8d4)
    Uint16 zoneTypeMask = 0u;                   // +0x60 (0x88edc793)
    Uint8 wealthTypeMask = 0u;                  // +0x78 (0x88edc795)
    Float32 minLandValue = 0.0f;                // +0x90 (0xaa120972)
    Float32 maxLandValue = 1000.0f;             // +0x94 (0xaa120973)
    Uint8 rotationMask = 0x0fu;                 // +0x98 (0xec3bd470)
    Uint8 flipRequirement = 0u;                 // +0x99 (0xcc3e4755: 0=any, 1=flipped, 2=unflipped)
    std::vector<cSC4LotConfigurationObject> objects; // +0x08

    bool LoadFromExemplar(const cGZExemplar& exemplar, Uint8 detailLevel = 3u);

    // Exact cSC4NetworkLotManager::IsLotCompatible @ 0x100407750.
    bool IsCompatible(
        Sint32 cellX,
        Sint32 cellZ,
        Uint32 zoneType,
        Uint32 wealthType,
        Uint8 pieceRotation,
        bool pieceFlipped,
        Float32 cellLandValue = 0.0f) const noexcept;
};

// Exact cSC4NetworkLotManager::UpdateRoadStyle @ 0x1004074f4,
// GetZoneTypeForNetwork @ 0x10040800c, and GetWealthForNetwork @ 0x100408130.
struct NetworkRoadStyleResult {
    Uint32 underTextureId = 0;
    Uint8 variation = 0;
    bool hasUnderTextureFlag = false;
};

NetworkRoadStyleResult ComputeNetworkRoadStyle(
    Uint32 zoneType,
    Uint32 wealthType,
    bool isRailOnDirt) noexcept;

Uint32 ComputeZoneTypeForNetworkCell(
    Sint32 cellX,
    Sint32 cellZ,
    Sint32 citySizeX,
    Sint32 citySizeZ,
    std::span<const Sint8> zoneGrid) noexcept;

// Exact cSC4NetworkLotManager::GetPositionForLotConfigurationObject @ 0x100407c02.
void ComputeNetworkLotObjectPlacement(
    Sint32 cellX,
    Sint32 cellZ,
    Uint8 pieceRotation,
    bool pieceFlipped,
    Float32 networkAltitudeY,
    const cSC4LotConfigurationObject& obj,
    Float32& outWorldX,
    Float32& outWorldY,
    Float32& outWorldZ,
    Sint32& outOrientation) noexcept;

// Exact serialized record of cSC4NetworkLotManager (CLSID 0xe9a6cd3b):
// Read @ 0x10040562a, Write @ 0x10040555a.
struct NetworkLotSaveEntry {
    Uint16 packedCellXZ = 0; // (x << 8) | z
    GrowthDeveloperSaveRecord::GridRef networkOccupant{};
    Uint32 configurationId = 0;

    Uint8 CellX() const noexcept { return static_cast<Uint8>((packedCellXZ >> 8) & 0xffu); }
    Uint8 CellZ() const noexcept { return static_cast<Uint8>(packedCellXZ & 0xffu); }
};

struct NetworkDamageEffectEntry {
    Uint16 packedCellXZ = 0; // (x << 8) | z
    GrowthDeveloperSaveRecord::GridRef visualEffectOccupant{};
};

struct NetworkLotManagerSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2u;
    std::vector<NetworkLotSaveEntry> networkLots;
    std::vector<NetworkDamageEffectEntry> damageEffects;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;
};

// ============================================================================
// Wave 2A Method Index: cSC4LotConfiguration, cSC4LotConfigurationManager,
// cSC4NetworkManager, cSC4NetworkLotManager, cSC4PathInfo, cSC4PathFinder,
// cSC4VertexHtConstraintSatisfier, and cSC4NetworkConstructionCrew.
//
// cSC4LotConfiguration:
//   cSC4LotConfiguration() @ 0x100402820 / 0x100402c1c,
//   ~cSC4LotConfiguration() @ 0x100402c26 / 0x100402f08 / 0x100402f12,
//   Shutdown @ 0x100402dda, Init @ 0x100402f40, QI @ 0x100402f48,
//   AddRef @ 0x100402f6a, Release @ 0x100402f78, GetID @ 0x100402fba,
//   GetName @ 0x100402fc4, BelongsToFamily @ 0x100402fea,
//   GetFamilyIDArray @ 0x10040301c, GetSize @ 0x100403026,
//   GetMaximumSlope @ 0x100403046, GetMinimumSlope @ 0x10040305e,
//   GetMaximumSlopeBeforeFoundation @ 0x100403076,
//   GetGrowthStage @ 0x10040308e, IsCompatibleWithWater @ 0x1004030a2,
//   IsCompatibleWithZoneType @ 0x1004030ac,
//   IsCompatibleWithWealthType @ 0x1004030c8,
//   IsCompatibleWithPurposeType @ 0x1004030e8,
//   IsCompatibleWithStyle @ 0x100403108,
//   IsCompatibleWithMonopolyPieceUse @ 0x100403134,
//   GetRequiredRoads @ 0x10040313c,
//   GetLotConfigurationObjectCount @ 0x1004031ae,
//   GetLotConfigurationObjectArray @ 0x1004031c6,
//   GetPrimaryBuildingConfigurationObject @ 0x1004031d4,
//   GetPrimaryBaseConfigurationObject @ 0x100403216,
//   GetPrimaryFoundationConfigurationObject @ 0x10040324e,
//   HasObjectsOfType @ 0x100403286, GetRetainingWallIDArray @ 0x1004032c4,
//   GetFenceIDArray @ 0x1004032d2, GetFenceSegmentPositionArray @ 0x1004032e0,
//   Load @ 0x1004032ee, ConvertPropertiesToObjectArray @ 0x100403a60,
//   CorrectObjectPositions @ 0x100403d8e, SaveToPropertyHolder @ 0x100403e78,
//   ConvertObjectArrayToProperties @ 0x10040448a, Save @ 0x1004048a8,
//   GetMinBuildingCapacity @ 0x1004049dc,
//   GetMaxBuildingCapacity @ 0x1004049e6, IsDoConstruction @ 0x1004049f0.
//
// cSC4LotConfigurationManager (CLSID 0x299062c0):
//   ~cSC4LotConfigurationManager() @ 0x1006c30f6 / 0x1006c316c / 0x1006c3176,
//   QI @ 0x1006c31a4, AddRef @ 0x1006c31f4, Release @ 0x1006c321e,
//   StaticInit @ 0x1006c32d6, StaticShutdown @ 0x1006c334a,
//   LoadAllStandardLotConfigurations @ 0x1006c33b6, Init @ 0x1006c3410,
//   Shutdown @ 0x1006c34a6, Write @ 0x1006c3526, Read @ 0x1006c3586,
//   DoMessage @ 0x1006c3616, InitBuildingInformation @ 0x1006c3654,
//   AddLotConfiguration(cISC4LotConfiguration*, bool) @ 0x1006c3a54,
//   FlushCachedLotConfigurationSearches @ 0x1006c3f9e,
//   AddLotConfiguration(cGZPersistResourceKey const&) @ 0x1006c4028,
//   ReadLotConfiguration @ 0x1006c4076, RemoveLotConfiguration @ 0x1006c4108,
//   WriteLotConfiguration @ 0x1006c4146, GetLotConfiguration @ 0x1006c4166,
//   GetLotConfigurationsByFilter @ 0x1006c4196,
//   GetLotConfigurationsByFamily @ 0x1006c47e6,
//   GetLotConfigurationsBySize @ 0x1006c4a76,
//   ClearLotConfigurationSet @ 0x1006c4d22,
//   GetLotConfigurationIDsByFilter @ 0x1006c4d50,
//   GetLotConfigurationIDsByFamily @ 0x1006c4fee,
//   GetLotConfigurationIDsBySize @ 0x1006c5290,
//   LotConfigurationHasBuildingFamily @ 0x1006c5538,
//   LotConfigurationHasBuildingType @ 0x1006c55da,
//   GetLotConfigurationMap @ 0x1006c5762,
//   GetLotConfigurationFamilyIDSet @ 0x1006c5770,
//   CreateGenericLotConfiguration @ 0x1006c577e,
//   FixLotConfiguration @ 0x1006c5aa6,
//   GetMaxFootprintForBuildingFamilies @ 0x1006c603a,
//   GetMaxFootprintForFloraFamilies @ 0x1006c61a0,
//   GetMaxFootprintForPropFamilies @ 0x1006c62f0,
//   AddLotConfigurationStatic @ 0x1006c6446, GetObjectIDArray @ 0x1006c6464,
//   RegisterLotConfigurationFilter @ 0x1006c6918,
//   UnregisterLotConfigurationFilter @ 0x1006c69c0.
//
// cSC4NetworkManager (CLSID 0xc990bd46):
//   cSC4NetworkManager() @ 0x10016dffe, ~cSC4NetworkManager() @ 0x10016e486 /
//   0x10016e690 / 0x10016e69a, GetNetworkTool @ 0x10016e6c8,
//   DestroyNetworkOccupants @ 0x10016e8d8, QueueRemovals @ 0x10016eaee,
//   MapPreviewTexture @ 0x10016ec20, AddTextureMapping @ 0x10016ec7e,
//   MapShadowTexture @ 0x10016ece4, MapPath @ 0x10016ee44,
//   RegisterConstructionCrew @ 0x10016f1dc,
//   UnregisterConstructionCrew @ 0x10016f226,
//   IsOccupantSubwayStation @ 0x10016f2a8, FindPipeOccupant @ 0x10016f2b6,
//   FindPipeConnectionOccupant @ 0x10016f2c8,
//   GetWaterOccupantFilter @ 0x10016f2da, GetSubwayOccupantFilter @ 0x10016f332,
//   AddRef @ 0x10016f38a, DoMessage @ 0x10016f3b4,
//   DoViewRotationChange @ 0x1001700d8, Retrofit @ 0x100170230,
//   cRefreshBridgeModels::Go @ 0x100170886, Init @ 0x100170928,
//   QI @ 0x100170e1e, Release @ 0x100170e7e, Shutdown @ 0x100170ee6,
//   RemoveOrReplaceOccupant @ 0x100171064,
//   GetAllIntersectionPieces @ 0x1001711c2,
//   GetAllIntersectionPieces2 @ 0x1001712aa,
//   CreateConnectionNode @ 0x100171494, AutoconnectLotNodes @ 0x100171780,
//   GetEnglishNetworkName @ 0x100171c3c, AddToRetrofitList @ 0x100171c50,
//   Read @ 0x100171cb2, Write @ 0x100171d8c.
//
// cSC4NetworkLotManager (CLSID 0xe9a6cd3b):
//   cSC4NetworkLotManager() @ 0x100404dce, ~cSC4NetworkLotManager() @ 0x100404ebc /
//   0x100404f84 / 0x100404f8e, AddRef @ 0x100404fbc, Release @ 0x100404ff8,
//   QI @ 0x100405072, Init @ 0x1004050d2, LoadAllConfigurations @ 0x1004053c8,
//   Shutdown @ 0x100405422, Write @ 0x10040555a, Read @ 0x10040562a,
//   AddLotConfigurationStatic @ 0x100405868, ReadLotConfiguration @ 0x100405946,
//   AddLotConfiguration @ 0x100405c50, WriteLotConfiguration @ 0x100405d3e,
//   GetNetworkLotConfigurationSet @ 0x1004062a6,
//   IterateAllNetworkLots @ 0x1004063de, DoMessage @ 0x100406428,
//   UpdateRoadsInCell @ 0x100406d10, DoNetworkOccupantRemoved @ 0x100406de6,
//   CreateLotObjects @ 0x100406ee8, ProcessUpdateList @ 0x100407106,
//   DoRoadDamageChange @ 0x100407166, UpdateRoadStyle @ 0x1004074f4,
//   UpdateLot @ 0x100407640, IsLotCompatible @ 0x100407750,
//   DoNetworkOccupantInserted @ 0x10040784e,
//   GetPositionForLotConfigurationObject @ 0x100407c02,
//   RedevelopLot @ 0x100407df4, CreateProp @ 0x100407e66,
//   GetNetworkLot @ 0x100407f2c, GetNetworkLotConfigurationById @ 0x100407f60,
//   SetLotConfigurationForNextPiece @ 0x100407fbe,
//   GetZoneTypeForNetwork @ 0x10040800c, GetWealthForNetwork @ 0x100408130,
//   UpdateAvenueStyle @ 0x100408330.
//
// cSC4PathInfo:
//   cSC4PathInfo(cSC4PathInfo const&) @ 0x100458240,
//   ~cSC4PathInfo() @ 0x1004582de / 0x10045833a / 0x100458352,
//   QI @ 0x1004583a0, Clone @ 0x100458424,
//   GetPath(tPathType, uchar, uchar, uchar, float*) @ 0x100458470,
//   GetPath(uint, float*) @ 0x100458492, GetBestPath @ 0x1004584ce,
//   GetRandomPath @ 0x1004586a8, GetRandomPathWildcard @ 0x1004587a8,
//   GetPathsNearStartPoint @ 0x100458936, GetPathsNearEndPoint @ 0x100458ace,
//   GetPathLength(tPathType, uchar, uchar, uchar) @ 0x100458c66,
//   GetPathLength(uint) @ 0x100458c86, NewPath @ 0x100458cc2,
//   NewStopPoint @ 0x100458fbc,
//   GetStopPointCount(tPathType, uchar, uchar, uchar) @ 0x10045930c,
//   GetStopPointCount(uint) @ 0x1004593c2,
//   GetStopPoint(cS3DVector3&, tPathType, uchar, uchar, uchar) @ 0x10045944e,
//   GetStopPoint(cS3DVector3&, uint) @ 0x100459478,
//   IsIntersection @ 0x10045954a, GetPathsFromEntryEdge @ 0x100459564,
//   GetPathsToExitEdge @ 0x1004595ec, GetLaneCount @ 0x100459670,
//   CreateTransformedCopy @ 0x1004596ac, RemoveWrongWayPaths @ 0x10045a558,
//   GetUniquePathKey @ 0x10045a8f0,
//   InitializeNetworkConnectionBitmap @ 0x10045a9e2, OffsetBy @ 0x10045aa24,
//   OffsetAndScissor @ 0x10045aa70, RemoveDuplicatePaths @ 0x10045b4f2,
//   GetDestinationPoint @ 0x10045b608, SnapPointToPath @ 0x10045b80e,
//   SnapPointToAnyPath @ 0x10045baa8, GetDispatchDestination @ 0x10045bc0a,
//   GetPathMidpoint @ 0x10045bd4e, GetRandomPathPoint @ 0x10045bef4,
//   GetJunctionKeys @ 0x10045c028, MakeTunnelPaths @ 0x10045c0bc,
//   MakeBidirectionalPaths @ 0x10045c232, CopyPaths @ 0x10045c3d0,
//   GetCenterLane @ 0x10045c5e0, GetPathIntersection @ 0x10045c870,
//   VerifyPathEdges @ 0x10045cbee, AddRef @ 0x10045d972,
//   Release @ 0x10045d980, UseElevation @ 0x10045d98e,
//   SetUseElevationFlag @ 0x10045d99a, GetPathMap @ 0x10045d9b2.
//
// cSC4PathFinder:
//   cSC4PathFinder() @ 0x1000d0c62 / 0x1000d0d80,
//   ~cSC4PathFinder() @ 0x1000d0d8a / 0x1000d0e4e / 0x1000d0e58,
//   QI @ 0x1000d0e86, AddRef @ 0x1000d0ea8, Release @ 0x1000d0eb6,
//   Init @ 0x1000d0ef2, Shutdown @ 0x1000d10ca, AtGoal @ 0x1000d1106,
//   CheckDestinationCapacity @ 0x1000d1606, ReleasePathNodes @ 0x1000d1664,
//   TripNodeHashMap::Clear @ 0x1000d169c,
//   FindNearestStandardDest @ 0x1000d171c, CreateStartNodes @ 0x1000d1e8c,
//   TripNodeHashMap::GetNode @ 0x1000d2468,
//   TripNodePriorityQueue::Push @ 0x1000d25bc, FindPath @ 0x1000d2644,
//   SetupDistanceTables @ 0x1000d3556, RecalculateNodeScores @ 0x1000d364c,
//   TripNodePriorityQueue::Pop @ 0x1000d375c, DoSuccessfulTrip @ 0x1000d382c,
//   EdgeTo @ 0x1000d3976, AddTripNode @ 0x1000d3994,
//   DistanceToRectangle @ 0x1000d3b32,
//   TripNodePriorityQueue::UpdateNode @ 0x1000d3c72,
//   ParseTripLocation @ 0x1000d3ccc, WasTripReversible @ 0x1000d3dc0,
//   ApplyTraffic @ 0x1000d3f60, GetTripLengthScaledForDisplay @ 0x1000d4326,
//   GetCompressedRoute @ 0x1000d434e,
//   TripNodePriorityQueue::TripNodePriorityQueue @ 0x1000d4d44,
//   TripNodeHashMap::TripNodeHashMap @ 0x1000d4dd4,
//   TripNodeHashMap::~TripNodeHashMap @ 0x1000d4e86,
//   SetFlag @ 0x1000d511a, ClearFlag @ 0x1000d5124,
//   ClearAllFlags @ 0x1000d5130, SetRouteInfo @ 0x1000d513e,
//   SetTripDestinationFilter @ 0x1000d514c,
//   SetDestinationFlag @ 0x1000d5158, ClearDestinationFlag @ 0x1000d516c,
//   ClearAllDestinationFlags @ 0x1000d5182, SetTravelTypeFlag @ 0x1000d5190,
//   ClearTravelTypeFlag @ 0x1000d51ae, ClearAllTravelTypeFlags @ 0x1000d51ca,
//   SetStartingCostArray @ 0x1000d51d8, SetMaxTripLength @ 0x1000d51e6,
//   SetTrafficDensity @ 0x1000d51f2, SetStartRect @ 0x1000d51fc,
//   SetEndRect @ 0x1000d520a, SetMaxWorkTime @ 0x1000d5218,
//   Reset @ 0x1000d5224, GetTrafficArrived @ 0x1000d5232,
//   GetTripLength @ 0x1000d523c, GetEndCell @ 0x1000d5248,
//   GetLastTravelType @ 0x1000d5258.
//
// cSC4VertexHtConstraintSatisfier:
//   AddEndFixedSmoothnessConstraint @ 0x1001bc840,
//   AddMidFixedSmoothnessConstraint @ 0x1001bc8fc,
//   AddVertexToSolve @ 0x1001bc9c2,
//   cSC4VertexHtConstraintSatisfier() @ 0x1001bca8c,
//   ~cSC4VertexHtConstraintSatisfier() @ 0x1001bcb6a / 0x1001bcca4 / 0x1001bccae,
//   Shutdown @ 0x1001bcc74,
//   DeleteConstraintsAndOtherTemporaryData @ 0x1001bccdc,
//   DoInitialConstraintPropagation @ 0x1001bcdac,
//   PropagateRangeConstraintsOfVerts @ 0x1001bcece,
//   FindEqualitySet @ 0x1001bd09a, GetHeight @ 0x1001bd0ea,
//   GetNextSolvedVertexAndItsHeight @ 0x1001bd11c, Init @ 0x1001bd156,
//   InsertEqualityConstraint @ 0x1001bd178, MergeIntoSet @ 0x1001bd4be,
//   InsertFixedVertex @ 0x1001bd67c, InsertRangeConstraint @ 0x1001bd6b2,
//   InsertPreferredValueConstraint @ 0x1001bd78a,
//   InsertPreferredRangeConstraint @ 0x1001bd7c2,
//   InsertSlopeConstraint @ 0x1001bd80a,
//   InsertSmoothnessConstraint @ 0x1001bd8fc,
//   IntersectVertexRange @ 0x1001bda28, MarkPotentiallySolved @ 0x1001bdad4,
//   MarkSolution @ 0x1001bdb1e, PickASolution @ 0x1001bdbd8,
//   UpdateConstraints(int, float, uint, set&) @ 0x1001bdd50,
//   UpdateConstraints(int, tHtRange const&, uint, set&) @ 0x1001bde06,
//   RewindSolvedVertexList @ 0x1001bdeac, Solve @ 0x1001bdebe,
//   DumpConstraints @ 0x1001be05e,
//   UpdateEqualityConstraints(int, float, set&) @ 0x1001be298,
//   UpdateSlopeConstraints(int, float, set&) @ 0x1001be488,
//   UpdateSmoothnessConstraints(int, float) @ 0x1001be60c,
//   UpdateMidFixedSmoothnessConstraints(int, float, set&) @ 0x1001be734,
//   UpdateEndFixedSmoothnessConstraints(int, float, set&) @ 0x1001be8ba,
//   UpdateEqualityConstraints(int, tHtRange const&, set&) @ 0x1001beb14,
//   UpdateSlopeConstraints(int, tHtRange const&, set&) @ 0x1001bec70,
//   UpdateSmoothnessConstraints(int, tHtRange const&) @ 0x1001bedbe,
//   UpdateMidFixedSmoothnessConstraints(int, tHtRange const&, set&) @ 0x1001beeb8,
//   UpdateEndFixedSmoothnessConstraints(int, tHtRange const&, set&) @ 0x1001bf006.
//
// cSC4NetworkConstructionCrew:
//   cSC4NetworkConstructionCrew() @ 0x1001ea36c,
//   ~cSC4NetworkConstructionCrew() @ 0x1001ea3d6,
//   Init @ 0x1001ea46c, CreateConstructionParticles @ 0x1001ea5fa,
//   CreateConstructionVehicle @ 0x1001ea722,
//   CreateConstructionAutomata @ 0x1001ea7fe, GetEdge @ 0x1001eaea4,
//   SearchForEdge @ 0x1001eaf40, GetPathMidpoint @ 0x1001eb056,
//   AddNetworkSegment @ 0x1001eb320, MarkOccupantsUsable @ 0x1001eb73c,
//   FindNetworkOccupantForCell @ 0x1001eb78c, PlaceVehicle @ 0x1001eb7ee,
//   MarkOccupantUsable @ 0x1001eba78, DestroyVehicle @ 0x1001ebaec,
//   PlaceLeadVehicle @ 0x1001ebb70, GetNextNetworkOccupant @ 0x1001ebc3a,
//   MoveVehicleToNextCell @ 0x1001ebd10, SetVehicleEndPoint @ 0x1001ebe7e,
//   InitConstructionCrew @ 0x1001ebfdc,
//   CreateConstructionCrewParticleEffect @ 0x1001ec050,
//   SendConstructionMessage @ 0x1001ec102, CreateCrewsForSegment @ 0x1001ec1e2,
//   AddCrewToNetworkOccupant @ 0x1001ecd56, BeginConstruction @ 0x1001ecdca,
//   Shutdown @ 0x1001ed35e, StopConstruction @ 0x1001ed510,
//   StaticInit @ 0x1001ed668, StaticShutdown @ 0x1001ed680,
//   AddRef @ 0x1001ed698, Release @ 0x1001ed6a6, FinalRelease @ 0x1001ed6c2,
//   QI @ 0x1001ed704, AnimateVehicle @ 0x1001ed722,
//   StartLeadVehicleMoving @ 0x1001ed924,
//   MoveConstructionCrewToNextCell @ 0x1001ed9b8,
//   DoAnimationTick @ 0x1001edb40,
//   RemoveCrewFromNetworkOccupant @ 0x1001edcea,
//   GetNetworkOccupantCrewCount @ 0x1001edd70,
//   DoOccupantRemoved @ 0x1001eddb6, DoMessage @ 0x1001edf86.
// ============================================================================
struct SC4PathKeyHelper {
    // Exact cSC4PathInfo::GetUniquePathKey @ 0x10045a8f0:
    // packs (pathType, entryEdge, exitEdge, classNum) into a 32-bit key.
    static constexpr Uint32 ComputeUniquePathKey(
        Uint8 pathType, Uint8 entryEdge, Uint8 exitEdge, Sint32 classNum) noexcept {
        return (static_cast<Uint32>(pathType) << 24u) |
               (static_cast<Uint32>(entryEdge) << 16u) |
               (static_cast<Uint32>(exitEdge) << 8u) |
               (static_cast<Uint32>(classNum) & 0xffu);
    }

    // Exact cSC4PathFinder::ParseTripLocation @ 0x1000d3ccc:
    // unpacks packed 32-bit node key into (travelType, cellX, cellZ).
    static constexpr void ParseTripLocation(
        Uint32 key, Uint8& outTravelType, Sint16& outX, Sint16& outZ) noexcept {
        outTravelType = static_cast<Uint8>((key >> 24u) & 0xffu);
        outX = static_cast<Sint16>((key >> 12u) & 0x0fffu);
        outZ = static_cast<Sint16>(key & 0x0fffu);
    }
};

// Lot-edge bits consumed by cSC4ZoneDeveloper::GetVerticesAtEdgeOfLot
// @ 0x100131946 and GetNeighborCellsAtEdgeOfLot @ 0x1001319a6.
// Tested in order 0x1, 0x2, 0x4, 0x8; the first set bit wins.
inline constexpr Uint8 kLotEdgeNorth = 0x1u; // min Z
inline constexpr Uint8 kLotEdgeEast  = 0x2u; // max X
inline constexpr Uint8 kLotEdgeSouth = 0x4u; // max Z
inline constexpr Uint8 kLotEdgeWest  = 0x8u; // min X

// One road-facing parcel produced by cSC4ZoneDeveloper::SimpleParcellize.
// `cells` is a half-open cell rectangle [x0, x1) x [y0, y1).
struct ZoneParcel {
    SC4Rect<Sint32> cells{};
    Uint8 facingEdge = kLotEdgeNorth;
    Sint32 frontage = 0;
    Sint32 depth = 0;
};

// Frontage (cells along the road) and depth (cells away from the road)
// chosen by cSC4ZoneDeveloper::DetermineLotSize @ 0x10012d606.
struct ZoneLotSize {
    Sint32 frontage = 0;
    Sint32 depth = 0;
};

// Exact cSC4ZoneDeveloper lot subdivision @ 0x10012d606 / 0x10012c320 /
// 0x100131946 / 0x1001319a6. Cell rectangles are half-open.
class cSC4ZoneParcellizer {
public:
    // City extent read by GetNeighborCellsAtEdgeOfLot from this+0x48 / this+0x4c.
    Sint32 cityCellsX = 0;
    Sint32 cityCellsZ = 0;

    // Density comes from cSC4BuildingOccupant::ConvertZoneToDensity. `roadEdge`
    // selects which side of `region` is the frontage.
    static ZoneLotSize DetermineLotSize(
        const SC4Rect<Sint32>& region,
        Uint32 zoneType,
        Uint8 roadEdge) noexcept;

    // `maxDepth` is SimpleParcellize's int argument. A positive value caps lot
    // depth. When the block is at least two cells deeper than that depth, one
    // cell is left as a street and the far side is parcelled facing it.
    static void SimpleParcellize(
        const SC4Rect<Sint32>& region,
        Uint32 zoneType,
        Uint8 roadEdge,
        Sint32 maxDepth,
        std::vector<ZoneParcel>& outParcels);

    static bool GetVerticesAtEdgeOfLot(
        const SC4Rect<Sint32>& lot,
        Uint8 edge,
        SC4Rect<Sint32>& outVertices) noexcept;

    bool GetNeighborCellsAtEdgeOfLot(
        const SC4Rect<Sint32>& lot,
        Uint8 edge,
        SC4Rect<Sint32>& outCells) const noexcept;
};

// A* result for cSC4PathFinder::FindPath @ 0x1000d2644.
// cellX/cellZ/travelType are parallel and run from the start cell to the goal.
// `compressedRoute` is the byte encoding of cSC4PathFinder::GetCompressedRoute
// @ 0x1000d434e: each run is (travelType << 2 | direction, length) with
// direction 0 = +X, 1 = -X, 2 = +Z, 3 = -Z.
struct SC4PathFindResult {
    bool found = false;
    std::vector<Sint32> cellX;
    std::vector<Sint32> cellZ;
    std::vector<Uint8> travelType;
    Float32 tripLength = 0.0f;
    std::vector<Uint8> compressedRoute;
};

// Exact cSC4PathFinder search @ 0x1000d2644, heuristic @ 0x1000d3b32,
// route compression @ 0x1000d434e, reversibility @ 0x1000d3dc0, and the
// display scale @ 0x1000d4326.
//
// `startRect` / `endRect` are inclusive cell rectangles (y is the Z axis).
// `travelCost` is row-major, length >= gridCellsX * gridCellsZ. A cost <= 0
// is not a network tile. `travelType` is an optional parallel array.
// `maxTripLength` < 0 means no cap; otherwise a path whose summed cost exceeds
// the cap is rejected.
class cSC4PathFinderEngine {
public:
    static Sint32 DistanceToRectangle(
        Sint32 x, Sint32 z, const SC4Rect<Sint32>& rect) noexcept;

    static bool FindPath(
        const SC4Rect<Sint32>& startRect,
        const SC4Rect<Sint32>& endRect,
        Sint32 gridCellsX,
        Sint32 gridCellsZ,
        std::span<const Float32> travelCost,
        std::span<const Uint8> travelType,
        Float32 maxTripLength,
        SC4PathFindResult& out);

    // networkFlags bit 0x2 is the one-way bit tested at tile+0x2c.
    static bool WasTripReversible(
        std::span<const Sint32> cellX,
        std::span<const Sint32> cellZ,
        std::span<const Uint8> travelType,
        std::span<const Uint8> networkFlags,
        Sint32 cityCellsX,
        Sint32 cityCellsZ) noexcept;

    static Float32 GetTripLengthScaledForDisplay(
        Float32 tripLength, Float32 displayScale) noexcept;
};

// Exact cSC4VertexHtConstraintSatisfier propagation
// @ 0x1001bd67c / 0x1001bd6b2 / 0x1001bd178 / 0x1001bd80a / 0x1001bda28 /
// 0x1001bdbd8 / 0x1001bdebe / 0x1001bd0ea.
// Heights are meters. A range that collapses below kHeightEpsilon becomes a
// point. Solve picks the tightest remaining range (lowest vertex id on ties)
// and propagates equality and slope constraints until every vertex is fixed
// or a constraint contradicts.
class cSC4VertexHeightSolver {
public:
    static constexpr Float32 kHeightEpsilon = 0.01f;

    bool InsertFixedVertex(Sint32 vertex, Float32 height);
    bool InsertRangeConstraint(Sint32 vertex, Float32 minHeight, Float32 maxHeight);
    bool InsertEqualityConstraint(Sint32 vertexA, Sint32 vertexB);
    void InsertSlopeConstraint(Sint32 vertexA, Sint32 vertexB, Float32 maxAbsDelta);
    bool Solve();
    bool GetHeight(Sint32 vertex, Float32& outHeight) const noexcept;

private:
    struct Vertex {
        Float32 minH = -std::numeric_limits<Float32>::infinity();
        Float32 maxH = std::numeric_limits<Float32>::infinity();
        Sint32 parent = -1;
        bool solved = false;
        Float32 solution = 0.0f;
    };

    struct SlopeLink {
        Sint32 a = 0;
        Sint32 b = 0;
        Float32 maxDelta = 0.0f;
    };

    std::map<Sint32, Vertex> mVertices;
    std::vector<SlopeLink> mSlopes;
    bool mConsistent = true;

    Vertex& Ensure(Sint32 vertex);
    Sint32 FindRoot(Sint32 vertex);
    bool ApplyRange(Sint32 vertex, Float32 minHeight, Float32 maxHeight);
    bool AssignSolved(Sint32 vertex, Float32 height);
    bool PropagateSlopes();
    static Float32 PickHeight(const Vertex& vertex) noexcept;
};

// Exact cSC4TrafficSimulator::GetTravelTimeRatio @ 0x100089f14.
// Returns 1 when the travel type ignores congestion or the cell has no network.
// Otherwise samples the congestion-to-inverse-speed curve at
// `congestionTimesSpeed` (the volume * network speed product the binary feeds
// to cSC4ResponseCurve::GetOutputValue). An empty curve returns 1.
Float32 ComputeTrafficTravelTimeRatio(
    Sint32 cellX,
    Sint32 cellZ,
    Uint8 travelType,
    bool travelTypeAffectedByTraffic,
    bool cellHasNetwork,
    Float32 congestionTimesSpeed,
    std::span<const Float32> curveInput,
    std::span<const Float32> curveOutput) noexcept;

// Exact cSC4TrafficSimulator::TransitSwitchEntryCost @ 0x10008c282.
// ratio = accumulatedCost / divisor. When the ratio is at or below
// `minimumRatio` (or the divisor is not positive) the binary returns the
// below-threshold constant. Otherwise it returns
// curve(ratio) * entryBase * globalScale.
Float32 ComputeTransitSwitchEntryCost(
    Float32 accumulatedCost,
    Float32 entryBase,
    Float32 divisor,
    Float32 globalScale,
    Float32 minimumRatio,
    Float32 belowThresholdCost,
    std::span<const Float32> curveInput,
    std::span<const Float32> curveOutput) noexcept;

} // namespace opensc4
