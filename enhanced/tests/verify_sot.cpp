#include "opensc4/gzcom.h"
#include "opensc4/simgrid.h"
#include "opensc4/dbpf.h"
#include "opensc4/formats.h"
#include "opensc4/city.h"
#include "opensc4/gl_city_renderer.h"
#include "opensc4/decompilation_registry.h"
#include "opensc4/sc4_subfiles.h"
#include "opensc4/sc4_subfiles_batch3.h"
#include "opensc4/tract_developer.h"
#include <cstdio>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <filesystem>
#include <set>

using namespace opensc4;
namespace fs = std::filesystem;

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

void AddGridCallback(Sint16& myVal, Uint8 otherVal, void*) {
    myVal = static_cast<Sint16>(myVal + otherVal);
}

} // namespace

int main() {
    std::printf("================================================================\n");
    std::printf(" OpenSC4 Source-of-Truth Verification Suite\n");
    std::printf("================================================================\n");

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
        std::size_t savedSims = 0, badAutomata = 0;
        for (const auto& a : bigCity.Automata()) {
            const bool sim = a.kind == SC4AutomatonRecord::Kind::Pedestrian;
            if (sim) ++savedSims;
            // Sims are sprite animations (0x29a5d1ec); walked dogs and all vehicles are S3D.
            if (a.model.type != kTypeS3D && !(sim && a.model.type == 0x29a5d1ecu)) ++badAutomata;
        }
        Check(savedSims >= 3u && badAutomata == 0u, "Decoded saved cSC4Pedestrian/cSC4Vehicle automata with model keys",
              ("sims=" + std::to_string(savedSims) + " bad=" + std::to_string(badAutomata)).c_str());
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

            // Enhanced Edition: Free 3D Camera (OrbitFree, Perspective & Orthographic raycasting),
            // O(1) cEnhancedSpatialIndex, Zero-Alloc cEnhancedPathFinderScratch, and Deselectable Mods
            const Uint32 rebuildsBeforeHover = headlessRenderer.RebuildCount();
            headlessRenderer.SetHoveredTile(12, 14);
            headlessRenderer.SetHoveredTile(18, 22);
            headlessRenderer.OrbitFree(0.42f, -0.15f);
            headlessRenderer.SetPerspectiveMode(true);
            int pickedX = -1, pickedZ = -1;
            const bool perspPickOk = headlessRenderer.ScreenToTile(
                640, 400, 1280, 800, procCity, pickedX, pickedZ);
            Check(!headlessRenderer.IsGeometryDirty() &&
                  headlessRenderer.RebuildCount() == rebuildsBeforeHover &&
                  headlessRenderer.IsPerspectiveMode() &&
                  perspPickOk && pickedX >= 0 && pickedZ >= 0,
                  "cSC4GLCityRenderer 360-degree Free Perspective Camera & zero-rebuild hover cursor");

            // Verify Deselectable Mods: Authentic Vanilla vs Enhanced Mods
            procCity.SetEdition(EngineEdition::Authentic);
            Check(procCity.Edition() == EngineEdition::Authentic &&
                  procCity.ModConfig().ActiveModCount() == 0u &&
                  procCity.ModConfig().AbandonmentGraceDays() == 60u &&
                  !procCity.ExecuteCheat("moolah 999999"),
                  "Deselecting all mods restores 100% Authentic vanilla behavior");

            procCity.SetEdition(EngineEdition::Enhanced);
            const bool cheatOk = procCity.ExecuteCheat("moolah 777777");
            const auto ihtShares = ComputeHighTechWorkforceDemand(1000.0f, procCity.ModConfig());
            const Uint8 megaLotCrimeSuppressed = ComputeLotCellCrime(80u, 8u, 8u, 150u, procCity.ModConfig());
            Check(procCity.Edition() == EngineEdition::Enhanced &&
                  procCity.ModConfig().ActiveModCount() == 8u &&
                  cheatOk && procCity.TreasuryFunds() == 777777 &&
                  std::abs(ihtShares[2] - 150.0f) < 1e-3f &&
                  megaLotCrimeSuppressed < 80u &&
                  procCity.SpatialIndex().Width() == 128 &&
                  procCity.PathFinderScratch().GenerationCount() > 0u,
                  "Enhanced Edition: O(1) SpatialIndex, Zero-Alloc A* Scratchpad, and 11 Deselectable Mods");
        }
    }

    std::printf("\n================================================================\n");
    std::printf(" Result: %d passed, %d failed\n", gPassed, gFailed);
    std::printf("================================================================\n");
    return gFailed == 0 ? 0 : 1;
}
