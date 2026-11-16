#include "opensc4/sse_float32.h"
#include "opensc4/tract_developer.h"
#include <algorithm>
#include <bit>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <limits>
#include <queue>
#include <vector>

namespace opensc4 {

namespace {

constexpr Uint32 kPropMaxOverbuildFactor = 0x47e2c500u;
constexpr Uint32 kPropAbandonedTakeoverFactors = 0x47e2c501u;
constexpr Uint32 kPropTunableFloat84 = 0x47e2c502u;
constexpr Uint32 kPropExistingCapacityWeights = 0x47e2c510u;
constexpr Uint32 kPropCandidateTypeWeights = 0x47e2c511u;
constexpr Uint32 kPropPopulationNoiseHalfRange = 0x47e2c530u;
constexpr Uint32 kPropPopulationOverflowRange = 0x47e2c531u;
constexpr Uint32 kPropAllowLowerWealthRedevelopment = 0x47e2c540u;
constexpr Uint32 kPropDefaultActiveStyles = 0x47e2c550u;
constexpr Uint32 kPropYearsBetweenStyleChanges = 0x47e2c551u;

constexpr Uint32 kCanCreateLotOutcomeTable[4] = {
    kTractLotOutcomeNone,
    kTractLotOutcomeInvalidLotState,
    kTractLotOutcomeWrongZone,
    kTractLotOutcomeNoCandidates,
};

constexpr Uint32 kZoneToDeveloperClassTable[9] = {
    1u, 1u, 1u, 2u, 2u, 2u, 3u, 3u, 3u,
};

constexpr Uint32 kIndustrialPurposeBaseZoneTable[4] = {
    7u, 8u, 8u, 8u,
};

constexpr Float32 kNetworkType0Or10Weights[3] = {1.0f, -2.0f, -1.0f};
constexpr Float32 kNetworkType6Weights[3] = {2.0f, -5.0f, -1.0f};
constexpr Float32 kNetworkType1Weights[3] = {10.0f, 0.0f, -5.0f};

constexpr bool HasNetworkType(Uint16 mask, Uint32 type) noexcept {
    return (mask & static_cast<Uint16>(1u << type)) != 0u;
}

void ClassifyNetworkMask(
    TractCandidateScoreInput& candidate,
    std::size_t side,
    Uint16 mask) noexcept {
    if (HasNetworkType(mask, 3u) || HasNetworkType(mask, 11u)) {
        ++candidate.networkType3Or11[side];
    }
    if (HasNetworkType(mask, 0u) || HasNetworkType(mask, 10u)) {
        ++candidate.networkType0Or10[side];
    }
    if (HasNetworkType(mask, 6u)) {
        ++candidate.networkType6[side];
    }
    if (HasNetworkType(mask, 1u)) {
        ++candidate.networkType1[side];
    }
}

} // namespace

void AccumulateExistingCapacity(
    TractCandidateScoreInput& candidate,
    std::span<const TractExistingLotCapacityInput> sourceLots) noexcept {
    if (candidate.existingCapacityB >= 0 && candidate.existingCapacityA >= 0) {
        return;
    }

    candidate.existingCapacityA = 0;
    candidate.existingCapacityB = 0;
    candidate.maxExistingGrowthStage = 0;
    candidate.maxExistingBuildingWealth = 0;

    const auto accumulateLot = [&](const TractExistingLotCapacityInput& lot) {
        const Sint32 cap = static_cast<Sint32>(lot.capacity);
        if (lot.lotState == 4u) {
            candidate.existingCapacityB += cap;
        } else {
            candidate.existingCapacityA += cap;
        }
        const Sint32 stage = static_cast<Sint32>(lot.growthStage);
        if (candidate.maxExistingGrowthStage < stage) {
            candidate.maxExistingGrowthStage = stage;
        }
        if (lot.buildingWealth != 0 &&
            candidate.maxExistingBuildingWealth < lot.buildingWealth) {
            candidate.maxExistingBuildingWealth = lot.buildingWealth;
        }
    };

    if (candidate.candidateType == 1u || candidate.candidateType == 2u ||
        candidate.candidateType == 4u) {
        if (!sourceLots.empty()) {
            accumulateLot(sourceLots.front());
        }
    } else if (candidate.candidateType == 3u) {
        for (const TractExistingLotCapacityInput& lot : sourceLots) {
            accumulateLot(lot);
        }
    }
}

bool IsRedevelopmentGrowthStageCompatible(
    const TractCandidateScoreInput& candidate,
    Uint8 configGrowthStage,
    bool configMatchesMaxExistingWealth) noexcept {
    const Sint32 targetStage = static_cast<Sint32>(configGrowthStage);
    if (configMatchesMaxExistingWealth) {
        return candidate.maxExistingGrowthStage < targetStage;
    }
    return candidate.maxExistingGrowthStage <= targetStage;
}

TractCellRect ComputeCandidateBounds(
    Sint32 originX,
    Sint32 originZ,
    Uint8 dimensionA,
    Uint8 dimensionB,
    Sint32 orientation) noexcept {
    const Sint32 a = static_cast<Sint32>(dimensionA);
    const Sint32 b = static_cast<Sint32>(dimensionB);
    switch (orientation) {
    case 0:
        return {originX + 1 - a, originZ, originX, originZ + b - 1};
    case 1:
        return {originX + 1 - b, originZ + 1 - a, originX, originZ};
    case 2:
        return {originX, originZ + 1 - b, originX + a - 1, originZ};
    case 3:
        return {originX, originZ, originX + b - 1, originZ + a - 1};
    default:
        return {-1, -1, -1, -1};
    }
}

void ClassifyPerimeterNetworks(
    TractCandidateScoreInput& candidate,
    const TractCellRect& candidateBounds,
    const TractCellRect& sourceLotBounds,
    Sint32 cityCellsX,
    Sint32 cityCellsZ,
    std::span<const Uint16> networkTypeMasks) noexcept {
    const TractCellRect& bounds =
        candidate.candidateType == 4u ? sourceLotBounds : candidateBounds;
    const auto classify = [&](Sint32 x, Sint32 z, std::size_t side) {
        if (x < 0 || z < 0 || x >= cityCellsX || z >= cityCellsZ) {
            return;
        }
        const std::size_t index =
            static_cast<std::size_t>(z) * static_cast<std::size_t>(cityCellsX) +
            static_cast<std::size_t>(x);
        if (index < networkTypeMasks.size()) {
            ClassifyNetworkMask(candidate, side, networkTypeMasks[index]);
        }
    };

    // The side indices and traversal order are those used at 0x10013a5f2:
    // min-Z, max-X, max-Z, min-X.
    if (bounds.minZ > 0) {
        for (Sint32 x = bounds.minX; x <= bounds.maxX; ++x) {
            classify(x, bounds.minZ - 1, 0u);
        }
    }
    if (bounds.maxX < cityCellsX - 1) {
        for (Sint32 z = bounds.minZ; z <= bounds.maxZ; ++z) {
            classify(bounds.maxX + 1, z, 1u);
        }
    }
    if (bounds.maxZ < cityCellsZ - 1) {
        for (Sint32 x = bounds.minX; x <= bounds.maxX; ++x) {
            classify(x, bounds.maxZ + 1, 2u);
        }
    }
    if (bounds.minX > 0) {
        for (Sint32 z = bounds.minZ; z <= bounds.maxZ; ++z) {
            classify(bounds.minX - 1, z, 3u);
        }
    }
}

bool TestForCornerLot(
    const TractCandidateScoreInput& candidate,
    Sint32 candidateOrientation,
    Uint8 cornerMask) noexcept {
    for (Sint32 i = 0; i < 4; ++i) {
        if ((cornerMask & static_cast<Uint8>(1u << i)) == 0u) {
            continue;
        }

        // Exact signed modulo-4 instruction sequence from 0x10013b37c.
        Sint32 eax = candidateOrientation + (i + 1);
        if (eax < 0) {
            eax = 0;
        }
        eax = (eax + (2 - i) - candidateOrientation) & -4;
        Sint32 r15d = candidateOrientation + eax;
        Sint32 edx = (-2 - i) - candidateOrientation - eax;
        if (edx <= -5) {
            edx = -4;
        }
        const Sint32 ecx = (candidateOrientation + edx + (5 + i) + eax) & -4;
        r15d -= ecx;
        const Sint32 side = (5 + i) + r15d - 4;
        if (side < 0 || side >= 4) {
            return false;
        }

        const std::size_t sideIndex = static_cast<std::size_t>(side);
        if (candidate.networkType6[sideIndex] == 0 &&
            candidate.networkType0Or10[sideIndex] == 0 &&
            candidate.networkType3Or11[sideIndex] == 0) {
            return false;
        }
    }
    return true;
}

bool EvaluateCornerLotCompatibility(
    const TractCandidateScoreInput& candidate,
    Sint32 candidateOrientation,
    Uint8 cornerMask,
    bool requireCornerConfiguration,
    bool& candidateHasCornerConfig) noexcept {
    const bool hasCornerRequirement = (cornerMask & 0x7u) != 0u;
    if (!hasCornerRequirement && requireCornerConfiguration) {
        return false;
    }
    if (!TestForCornerLot(candidate, candidateOrientation, cornerMask)) {
        return false;
    }
    if (hasCornerRequirement) {
        candidateHasCornerConfig = true;
    }
    return true;
}

bool IsAggregationLegal(
    const TractCellRect& candidateBounds,
    Sint32 candidateOrientation,
    Sint32 cityCellsX,
    Sint32 cityCellsZ,
    std::span<const Sint32> cellLotIndices,
    std::span<const TractSourceLotInfo> sourceLots,
    Uint64& outSourceLotCount) noexcept {
    outSourceLotCount = 0u;
    if (candidateBounds.minX < 0 || candidateBounds.minZ < 0 ||
        candidateBounds.maxX >= cityCellsX ||
        candidateBounds.maxZ >= cityCellsZ) {
        return false;
    }
    if (candidateBounds.minX > candidateBounds.maxX) {
        return false;
    }

    Uint32 zoneType = 0u;
    bool hasMatchingOrientation = false;
    std::vector<Sint32> uniqueLots;

    for (Sint32 x = candidateBounds.minX; x <= candidateBounds.maxX; ++x) {
        if (candidateBounds.minZ <= candidateBounds.maxZ) {
            for (Sint32 z = candidateBounds.minZ; z <= candidateBounds.maxZ; ++z) {
                const std::size_t cellIndex =
                    static_cast<std::size_t>(z) *
                        static_cast<std::size_t>(cityCellsX) +
                    static_cast<std::size_t>(x);
                if (cellIndex >= cellLotIndices.size()) {
                    return false;
                }
                const Sint32 lotIndex = cellLotIndices[cellIndex];
                if (lotIndex < 0 ||
                    static_cast<std::size_t>(lotIndex) >= sourceLots.size()) {
                    return false;
                }

                const TractSourceLotInfo& lot =
                    sourceLots[static_cast<std::size_t>(lotIndex)];
                if (lot.suitabilityOutcome == kTractLotOutcomeSuitable) {
                    if (lot.orientation == candidateOrientation) {
                        hasMatchingOrientation = true;
                    }
                } else if (lot.suitabilityOutcome !=
                           kTractLotOutcomeIneligibleRoad) {
                    return false;
                }

                if (zoneType == 0u) {
                    zoneType = lot.zoneType;
                }
                if (lot.zoneType != zoneType) {
                    return false;
                }

                const auto it = std::lower_bound(
                    uniqueLots.begin(), uniqueLots.end(), lotIndex);
                if (it == uniqueLots.end() || *it != lotIndex) {
                    uniqueLots.insert(it, lotIndex);
                }
            }
        }
    }

    outSourceLotCount = static_cast<Uint64>(uniqueLots.size());
    if (!hasMatchingOrientation || uniqueLots.size() <= 1u) {
        return false;
    }

    for (const Sint32 lotIndex : uniqueLots) {
        const TractCellRect& lotBounds =
            sourceLots[static_cast<std::size_t>(lotIndex)].bounds;
        if (lotBounds.minZ < candidateBounds.minZ ||
            lotBounds.maxZ > candidateBounds.maxZ ||
            lotBounds.minX < candidateBounds.minX ||
            lotBounds.maxX > candidateBounds.maxX) {
            return false;
        }
    }

    return true;
}

bool IsAggregationSizeCompatible(
    Uint8 lotDimensionA,
    Uint8 lotDimensionB,
    Uint32 configDimensionA,
    Uint32 configDimensionB) noexcept {
    if (configDimensionA < static_cast<Uint32>(lotDimensionA) ||
        configDimensionB < static_cast<Uint32>(lotDimensionB)) {
        return false;
    }
    return configDimensionA != static_cast<Uint32>(lotDimensionA) ||
           configDimensionB != static_cast<Uint32>(lotDimensionB);
}

void ComputeAggregationShiftedOrigin(
    Sint32 lotOriginX,
    Sint32 lotOriginZ,
    Uint8 lotDimensionA,
    Uint32 configDimensionA,
    Sint32 orientation,
    Sint32& outOriginX,
    Sint32& outOriginZ) noexcept {
    outOriginX = lotOriginX;
    outOriginZ = lotOriginZ;
    const Sint32 delta =
        static_cast<Sint32>(configDimensionA) -
        static_cast<Sint32>(lotDimensionA);
    if (delta <= 0) {
        return;
    }
    switch (orientation) {
    case 0:
        outOriginX += delta;
        break;
    case 1:
        outOriginZ += delta;
        break;
    case 2:
        outOriginX -= delta;
        break;
    case 3:
        outOriginZ -= delta;
        break;
    default:
        break;
    }
}

Uint8 ComputeSubdivisionCount(
    Uint8 lotDimensionA,
    Uint8 lotDimensionB,
    Uint32 configDimensionA,
    Uint32 configDimensionB) noexcept {
    const Uint32 lotA = static_cast<Uint32>(lotDimensionA);
    const Uint32 lotB = static_cast<Uint32>(lotDimensionB);
    if (configDimensionA == 0u || configDimensionA > lotA ||
        configDimensionB > lotB) {
        return 0u;
    }
    if (configDimensionA == lotA && configDimensionB == lotB) {
        return 0u;
    }
    if ((lotA % configDimensionA) != 0u || configDimensionB != lotB) {
        return 0u;
    }
    return static_cast<Uint8>(lotA / configDimensionA);
}

bool GrowthStageFitsInZone(Sint32 stage, Uint32 zoneType) noexcept {
    if (zoneType > 9u) {
        return false;
    }
    const Uint32 bit = 1u << zoneType;
    if ((0x3c8u & bit) != 0u) {
        return true;
    }
    if ((0x12u & bit) != 0u) {
        return stage < 4;
    }
    if ((0x24u & bit) != 0u) {
        return stage < 7;
    }
    return false;
}

bool BuildingFitsInZone(Uint32 buildingZoneType, Uint32 cellZoneType) noexcept {
    if (buildingZoneType == cellZoneType) {
        return true;
    }
    switch (cellZoneType) {
    case 2u:
        return buildingZoneType == 1u;
    case 3u:
        return (buildingZoneType - 1u) < 2u;
    case 5u:
        return buildingZoneType == 4u;
    case 6u:
        return (buildingZoneType | 1u) == 5u;
    case 9u:
        return buildingZoneType == 8u;
    case 11u:
    case 12u:
    case 15u:
        return buildingZoneType == 0u;
    default:
        return false;
    }
}

Uint32 ConvertZoneToDeveloperClass(Uint32 zoneType) noexcept {
    if (zoneType < 1u || zoneType > 9u) {
        return 0u;
    }
    return kZoneToDeveloperClassTable[zoneType - 1u];
}

Uint32 ComputeConstructionBaseZoneType(
    Uint32 developerClass,
    Uint32 purpose) noexcept {
    if (developerClass == 1u) {
        return 1u;
    }
    if (developerClass == 2u) {
        return 4u;
    }
    if (developerClass == 3u) {
        const Uint32 idx = purpose - 5u;
        if (idx < 4u) {
            return kIndustrialPurposeBaseZoneTable[idx];
        }
    }
    return 0u;
}

Uint32 EvaluateLotSuitability(
    const TractLotSuitabilityInput& input,
    Sint32& outRejectedLandValue) noexcept {
    const Uint32 cellZone =
        static_cast<Uint32>(static_cast<Sint32>(input.cellZoneType));
    const bool fitsInZone =
        BuildingFitsInZone(input.developerBaseZoneType, cellZone);

    if (fitsInZone && input.checkForNewDevelopment && input.lotState == 3u) {
        if (input.daysSinceLastStateChange < 180u) {
            return kTractLotOutcomeOccupancyCooldown;
        }
    }

    if (!fitsInZone) {
        return kTractLotOutcomeWrongZone;
    }

    if (input.minLandValue != 0u || input.maxLandValue != 0u) {
        if (input.cellLandValue < input.minLandValue ||
            input.cellLandValue > input.maxLandValue) {
            outRejectedLandValue = static_cast<Sint32>(input.cellLandValue);
            return kTractLotOutcomeWrongLandValue;
        }
    }

    if (input.lotState == 2u || input.lotState > 4u) {
        return kTractLotOutcomeInvalidLotState;
    }

    if ((input.canCreateLotResult - 7u) >= 2u) {
        if (input.canCreateLotResult <= 3u) {
            return kCanCreateLotOutcomeTable[input.canCreateLotResult];
        }
        return kTractLotOutcomeNone;
    }

    const bool hasCapacity = ((input.lotState & ~1u) == 2u);
    if (hasCapacity) {
        if (input.existingBuildingType == 0u) {
            return kTractLotOutcomeInvalidLotState;
        }
        if (input.existingBuildingPurpose == 5u) {
            return kTractLotOutcomeCannotRedevelopPurpose;
        }
        if (input.existingBuildingPurpose == 7u &&
            input.developerPurpose == 6u) {
            return kTractLotOutcomeCannotRedevelopPurpose;
        }

        if (input.checkForNewDevelopment) {
            Uint32 cmpBuildingWealth = input.existingBuildingWealth;
            if (input.allowLowerWealthRedevelopment) {
                if (input.developerWealth < cmpBuildingWealth) {
                    return kTractLotOutcomeCannotRedevelopWealthOrPurpose;
                }
            } else {
                if (input.developerWealth != cmpBuildingWealth) {
                    return kTractLotOutcomeCannotRedevelopWealthOrPurpose;
                }
                cmpBuildingWealth = input.developerWealth;
            }

            if (input.developerWealth <= cmpBuildingWealth) {
                if (input.existingBuildingPurpose != input.developerPurpose) {
                    return kTractLotOutcomeCannotRedevelopWealthOrPurpose;
                }
            }
        }

        if (input.isHistorical) {
            return kTractLotOutcomeHistorical;
        }

        if (input.hasExistingLotConfiguration) {
            const Sint32 existingStage =
                static_cast<Sint32>(input.existingLotConfigGrowthStage);
            const bool stageOk =
                (input.existingBuildingWealth == input.developerWealth)
                    ? (input.targetGrowthStage > existingStage)
                    : (input.targetGrowthStage >= existingStage);
            if (!stageOk) {
                return kTractLotOutcomeTargetStageTooLowForExisting;
            }
        }
    }

    return input.hasRoadOrJobAccess ? kTractLotOutcomeSuitable
                                    : kTractLotOutcomeIneligibleRoad;
}

Uint32 EvaluateLotConfigurationSuitability(
    const TractLotConfigSuitabilityInput& input) noexcept {
    if (!input.purposeCompatible) {
        return kTractLotOutcomeWrongPurpose;
    }
    if (!input.wealthCompatible) {
        return kTractLotOutcomeWrongWealth;
    }
    const Sint32 stage = static_cast<Sint32>(input.configGrowthStage);
    if (input.targetGrowthStage < stage) {
        return kTractLotOutcomeStageExceedsTarget;
    }

    if ((input.developerPurpose - 1u) <= 2u && input.hasActiveStyles) {
        const bool styleOk = input.cycleThroughStyles
                                 ? input.matchesCurrentStyle
                                 : input.matchesAnyActiveStyle;
        if (!styleOk) {
            return kTractLotOutcomeWrongStyle;
        }
    }

    if (!input.hasPrimaryBuildingObject) {
        return kTractLotOutcomeNone;
    }

    const Sint32 minCap =
        (input.configGrowthStage == 1u)
            ? 0
            : static_cast<Sint32>(input.minBuildingCapacity);
    const Sint32 maxCap = static_cast<Sint32>(input.maxBuildingCapacity);

    if (!input.allowSingleUnitMinCapacity) {
        if (input.maxCapacityPerStep > maxCap) {
            return kTractLotOutcomeMaxCapacityTooSmall;
        }
    }

    if (minCap > input.tractCurrentPopulation + input.maxOverbuildCapacity) {
        return kTractLotOutcomeMinCapacityTooLarge;
    }

    return kTractLotOutcomeSuitable;
}

Uint32 EvaluateLotConfigurationCompatibility(
    const TractCandidateScoreInput& candidate,
    const TractLotConfigCompatibilityInput& input,
    bool& candidateHasCornerConfig) noexcept {
    if (input.configSizeX != static_cast<Uint32>(input.candidateDimensionA) ||
        input.configSizeZ != static_cast<Uint32>(input.candidateDimensionB)) {
        return kTractLotOutcomeNone;
    }
    if (!input.zoneCompatible) {
        return kTractLotOutcomeWrongZone;
    }
    if (!IsRedevelopmentGrowthStageCompatible(
            candidate,
            input.configGrowthStage,
            input.configMatchesMaxExistingWealth)) {
        return kTractLotOutcomeStageNotHigher;
    }
    if (input.configMinSlope > input.candidateSlope ||
        input.candidateSlope > input.configMaxSlope) {
        return kTractLotOutcomeWrongSlope;
    }

    if (!input.cheatIgnoreCaps) {
        if (input.configGrowthStage > input.powerSupplyCap &&
            !input.cheatIgnorePower && input.powerGridValue <= 1u) {
            return kTractLotOutcomeNoPower;
        }
        if (input.configGrowthStage > input.waterSupplyCap &&
            !input.cheatIgnorePlumbing && !input.isWatered) {
            return kTractLotOutcomeNoWater;
        }
        if (input.configGrowthStage > input.fireCoverageCap &&
            input.fireProtectionGridValue == 0u) {
            return kTractLotOutcomeNoFireCoverage;
        }
        if (input.configGrowthStage > input.parkCap &&
            input.parkGridValue == 0u) {
            return kTractLotOutcomeNoPark;
        }
    }

    if (input.developerClass == 3u) {
        return kTractLotOutcomeSuitable;
    }

    if (!EvaluateCornerLotCompatibility(
            candidate,
            input.candidateOrientation,
            input.cornerMask,
            input.requireCornerConfiguration,
            candidateHasCornerConfig)) {
        return kTractLotOutcomeIneligibleRoad;
    }

    return kTractLotOutcomeSuitable;
}

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
    Sint32& outMaxBuildingCapacity) noexcept {
    Sint32 minCap = 0;
    if (allowSingleUnitMinCapacity) {
        minCap = existingCapacityA + 1;
    } else {
        minCap = maxCapacityPerStep - builtCapacityThisGrow + existingCapacityA;
        if (candidateType == 4u) {
            minCap += subdivisionBuiltCapacity;
        }
    }
    if (minCap < existingCapacityA) {
        minCap = existingCapacityA;
    }
    if (candidateType == 4u && subdivisionCount != 0u) {
        minCap /= static_cast<Sint32>(subdivisionCount);
    }

    const Sint32 maxTarget =
        maxOverbuildCapacity - builtCapacityThisGrow + existingCapacityA;
    const Sint32 maxCap = std::max(minCap + 1, maxTarget);

    outMinBuildingCapacity = minCap;
    outMaxBuildingCapacity = maxCap;
}

Uint32 EvaluateBuildingCompatibility(
    const TractBuildingCompatibilityInput& input) noexcept {
    if (input.buildingPurpose != input.developerPurpose) {
        return kTractLotOutcomeWrongPurpose;
    }
    if (input.buildingWealth != input.developerWealth) {
        return kTractLotOutcomeWrongWealth;
    }
    if (input.hasActiveStyles && (input.buildingPurpose - 1u) <= 2u &&
        !input.buildingMatchesCurrentStyle) {
        return kTractLotOutcomeWrongStyle;
    }

    const Sint32 capacity = static_cast<Sint32>(input.buildingCapacity);
    if (input.minBuildingCapacity > capacity) {
        return kTractLotOutcomeMaxCapacityTooSmall;
    }
    if (input.maxBuildingCapacity < capacity) {
        return kTractLotOutcomeMinCapacityTooLarge;
    }

    if (input.developerMonopolyUse < 2u) {
        return input.isMonopolyPiece ? kTractLotOutcomeMonopolyMismatch
                                     : kTractLotOutcomeSuitable;
    }
    if (input.developerMonopolyUse == 2u) {
        return input.isMonopolyPiece ? kTractLotOutcomeSuitable
                                     : kTractLotOutcomeMonopolyMismatch;
    }
    return kTractLotOutcomeSuitable;
}

Uint32 SelectPickBuildingStyle(
    bool needsAnchorBuilding,
    std::span<const TractStylePercentageEntry> stylePercentages,
    Uint32 randomRoll0To99) noexcept {
    if (needsAnchorBuilding) {
        return kTractAnchorBuildingFamily;
    }
    if (stylePercentages.empty()) {
        return 0u;
    }

    Uint32 roll = randomRoll0To99;
    constexpr Float32 kPercentScale = 100.0f;
    constexpr Float32 kRoundMagic = 12582912.0f; // 0x4b400000
    constexpr Uint32 kRoundMagicSubtract = 0xb4c00000u;

    for (const TractStylePercentageEntry& entry : stylePercentages) {
        const Float32 scaled = entry.percentage * kPercentScale + kRoundMagic;
        Uint32 bits = 0;
        std::memcpy(&bits, &scaled, sizeof(Uint32));
        const Uint32 weightInt = bits + kRoundMagicSubtract;
        if (roll < weightInt) {
            return entry.styleId;
        }
        roll -= weightInt;
    }

    return 0u;
}

void ComputeTakeOverAbandonedThresholds(
    Sint32 maxCapacityPerStep,
    Sint32 maxOverbuildCapacity,
    Sint32 builtCapacityThisGrow,
    Float32 abandonedTakeoverMinFactor,
    Float32 abandonedTakeoverMaxFactor,
    Sint32& outMinTotalAvailableThreshold,
    Sint32& outMaxTakeoverCapacityLimit) noexcept {
    const Float32 minThresholdFloat =
        static_cast<Float32>(maxCapacityPerStep - builtCapacityThisGrow) *
        abandonedTakeoverMinFactor;
    const Float32 maxLimitFloat =
        static_cast<Float32>(maxOverbuildCapacity - builtCapacityThisGrow) *
        abandonedTakeoverMaxFactor;
    outMinTotalAvailableThreshold = static_cast<Sint32>(minThresholdFloat);
    outMaxTakeoverCapacityLimit = static_cast<Sint32>(maxLimitFloat);
}

bool EvaluateTakeOverAbandonedLot(
    const TractTakeOverAbandonedLotInput& input,
    Sint32& outTakeoverCapacity,
    Uint32& inoutBestOutcome) noexcept {
    outTakeoverCapacity = 0;

    if (input.lotState != 4u) {
        if (input.developerWealth > input.existingBuildingWealth ||
            input.occupantWealth >= input.developerWealth ||
            input.lotState != 3u || !input.allowLowerWealthRedevelopment) {
            return false;
        }
    }

    if (ConvertZoneToDeveloperClass(input.lotZoneType) !=
        input.developerClass) {
        return false;
    }

    if (input.totalCapacityForDeveloper == 0u) {
        if (inoutBestOutcome <= kTractLotOutcomeCannotRedevelopPurpose) {
            inoutBestOutcome = kTractLotOutcomeCannotRedevelopPurpose;
        }
        return false;
    }

    if (input.suitabilityOutcome != kTractLotOutcomeHistorical &&
        input.suitabilityOutcome != kTractLotOutcomeSuitable) {
        if (inoutBestOutcome <= input.suitabilityOutcome) {
            inoutBestOutcome = input.suitabilityOutcome;
        }
        return false;
    }

    Sint32 capacityB =
        (input.lotState == 4u)
            ? static_cast<Sint32>(input.totalCapacityForDeveloper)
            : 0;
    if (capacityB == 0) {
        capacityB = static_cast<Sint32>(input.availableCapacityForDeveloper);
        if (input.lotState == 3u && capacityB == 0) {
            capacityB = static_cast<Sint32>(input.totalCapacityForDeveloper);
        }
    }

    bool eligible = (capacityB > 0);
    if (eligible && !input.cheatIgnoreCaps &&
        input.hasExistingLotConfiguration) {
        const Uint8 stage = input.existingLotConfigGrowthStage;
        if (stage > input.existingDeveloperPowerSupplyCap &&
            !input.cheatIgnorePower && input.powerGridValue <= 1u) {
            if (inoutBestOutcome <= kTractLotOutcomeNoPower) {
                inoutBestOutcome = kTractLotOutcomeNoPower;
            }
            eligible = false;
        }
        if (stage > input.existingDeveloperWaterSupplyCap &&
            !input.cheatIgnorePlumbing && !input.isWatered) {
            if (inoutBestOutcome <= kTractLotOutcomeNoWater) {
                inoutBestOutcome = kTractLotOutcomeNoWater;
            }
            eligible = false;
        }
        if (stage > input.currentDeveloperFireCoverageCap &&
            input.fireProtectionGridValue == 0u) {
            if (inoutBestOutcome <= kTractLotOutcomeNoFireCoverage) {
                inoutBestOutcome = kTractLotOutcomeNoFireCoverage;
            }
            eligible = false;
        }
        if (stage > input.currentDeveloperParkCap &&
            input.parkGridValue == 0u) {
            if (inoutBestOutcome <= kTractLotOutcomeNoPark) {
                inoutBestOutcome = kTractLotOutcomeNoPark;
            }
            eligible = false;
        }
    }

    if (!eligible) {
        return false;
    }

    outTakeoverCapacity = capacityB;
    return true;
}

Uint32 ComputeUpdatePopulationSeed(Sint32 tractX, Sint32 tractZ) noexcept {
    return (static_cast<Uint32>(tractX) << 20u) +
           (static_cast<Uint32>(tractZ) << 4u);
}

Uint16 ComputeUpdatedLotPopulation(
    Sint16 maxCapacityTractValue,
    Uint16 availableCapacity,
    Float64 uniformNoiseSample,
    Float64 uniformOverflowTrimSample) noexcept {
    if (availableCapacity == 0u) {
        return 0u;
    }

    constexpr Float32 kOneHundredth = 0.009999999776482582f;
    const Float64 baseRatio = static_cast<Float64>(
        static_cast<Float32>(static_cast<Sint32>(maxCapacityTractValue)) *
        kOneHundredth);
    const Float32 noisyRatio =
        static_cast<Float32>((uniformNoiseSample + 1.0) * baseRatio);
    Float32 clampedRatio = std::max(0.0f, noisyRatio);
    if (clampedRatio >= 1.0f) {
        clampedRatio = static_cast<Float32>(1.0 - uniformOverflowTrimSample);
    }

    constexpr Float32 kRoundMagic = 12582912.0f; // 0x4b400000
    const Float32 rounded =
        static_cast<Float32>(availableCapacity) * clampedRatio + kRoundMagic;
    Uint32 bits = 0;
    std::memcpy(&bits, &rounded, sizeof(Uint32));
    return static_cast<Uint16>(bits & 0xffffu);
}

bool IsLotEligibleForDecline(const TractDeclineLotInput& input) noexcept {
    if (input.lotState != 3u) {
        return false;
    }
    if (input.daysSinceLastStateChange < 60u) {
        return false;
    }
    if (input.existingBuildingType == 0u) {
        return false;
    }
    if (input.existingBuildingPurpose != input.developerPurpose) {
        return false;
    }
    return input.occupantWealth == input.developerWealth;
}

void ComputeGrowInitialParameters(
    Float32 activeDemandValue,
    Float32 maxOverbuildFactor,
    Sint32 cellsPerTractSide,
    Sint32& outMaxCapacityPerStep,
    Sint32& outMaxOverbuildCapacity,
    Sint32& outMaxPassRetries) noexcept {
    const Sint32 truncatedDemand = static_cast<Sint32>(activeDemandValue);
    const Sint32 maxPerStep = (truncatedDemand > 0) ? truncatedDemand : 1;
    const Sint32 maxOverbuild = static_cast<Sint32>(
        static_cast<Float32>(maxPerStep) * maxOverbuildFactor);
    outMaxCapacityPerStep = maxPerStep;
    outMaxOverbuildCapacity = maxOverbuild;
    outMaxPassRetries = cellsPerTractSide * cellsPerTractSide + 1;
}

bool CanStartFarmBuild(
    Uint32 connectedFarmLotCount,
    Uint16 freeConstructionResources,
    Uint16& outInitialClaimedResources) noexcept {
    outInitialClaimedResources = 0u;
    if (connectedFarmLotCount < 15u) {
        return false;
    }
    const Uint16 required =
        static_cast<Uint16>(connectedFarmLotCount + 30u);
    if (freeConstructionResources < required) {
        return false;
    }
    outInitialClaimedResources = required;
    return true;
}

bool TractDeveloperTuning::LoadFromExemplar(
    const cGZExemplar& exemplar) noexcept {
    bool loaded = false;

    auto overbuild = exemplar.properties.find(kPropMaxOverbuildFactor);
    if (overbuild != exemplar.properties.end() &&
        !overbuild->second.floatValues.empty()) {
        maxOverbuildFactor = overbuild->second.floatValues[0];
        loaded = true;
    }

    auto takeover = exemplar.properties.find(kPropAbandonedTakeoverFactors);
    if (takeover != exemplar.properties.end() &&
        takeover->second.floatValues.size() >= 2) {
        abandonedTakeoverMinFactor = takeover->second.floatValues[0];
        abandonedTakeoverMaxFactor = takeover->second.floatValues[1];
        loaded = true;
    }

    auto float84 = exemplar.properties.find(kPropTunableFloat84);
    if (float84 != exemplar.properties.end() &&
        !float84->second.floatValues.empty()) {
        tunableFloat84 = float84->second.floatValues[0];
        loaded = true;
    }

    auto capacityWeights = exemplar.properties.find(kPropExistingCapacityWeights);
    if (capacityWeights != exemplar.properties.end() &&
        capacityWeights->second.floatValues.size() >= 2) {
        existingCapacityAWeight = capacityWeights->second.floatValues[0];
        existingCapacityBWeight = capacityWeights->second.floatValues[1];
        loaded = true;
    }

    auto candidateWeights = exemplar.properties.find(kPropCandidateTypeWeights);
    if (candidateWeights != exemplar.properties.end() &&
        candidateWeights->second.floatValues.size() >= 2) {
        sourceLotCountWeight = candidateWeights->second.floatValues[0];
        candidateType4Bonus = candidateWeights->second.floatValues[1];
        loaded = true;
    }

    auto popNoise = exemplar.properties.find(kPropPopulationNoiseHalfRange);
    if (popNoise != exemplar.properties.end() &&
        !popNoise->second.floatValues.empty()) {
        populationNoiseHalfRange = popNoise->second.floatValues[0];
        loaded = true;
    }

    auto popOverflow = exemplar.properties.find(kPropPopulationOverflowRange);
    if (popOverflow != exemplar.properties.end() &&
        popOverflow->second.floatValues.size() >= 2) {
        populationOverflowMin = popOverflow->second.floatValues[0];
        populationOverflowMax = popOverflow->second.floatValues[1];
        loaded = true;
    }

    auto allowLowerWealth =
        exemplar.properties.find(kPropAllowLowerWealthRedevelopment);
    if (allowLowerWealth != exemplar.properties.end() &&
        !allowLowerWealth->second.intValues.empty()) {
        allowLowerWealthRedevelopment =
            (allowLowerWealth->second.intValues[0] != 0);
        loaded = true;
    }

    auto styles = exemplar.properties.find(kPropDefaultActiveStyles);
    if (styles != exemplar.properties.end() &&
        !styles->second.intValues.empty()) {
        defaultActiveStyles.clear();
        defaultActiveStyles.reserve(styles->second.intValues.size());
        for (Sint64 v : styles->second.intValues) {
            defaultActiveStyles.push_back(static_cast<Uint32>(v));
        }
        loaded = true;
    }

    auto years = exemplar.properties.find(kPropYearsBetweenStyleChanges);
    if (years != exemplar.properties.end() &&
        !years->second.intValues.empty()) {
        yearsBetweenStyleChanges =
            static_cast<Sint32>(years->second.intValues[0]);
        loaded = true;
    }

    return loaded;
}

Float32 TractDeveloperTuning::ScoreCandidate(
    const TractCandidateScoreInput& candidate,
    Uint32 developerWealth) const noexcept {
    Float32 score = 0.0f;

    if (candidate.candidateType != 1u) {
        const Float32 capacityA =
            static_cast<Float32>(candidate.existingCapacityA) *
            existingCapacityAWeight;
        const Float32 capacityB =
            static_cast<Float32>(candidate.existingCapacityB) *
            existingCapacityBWeight;
        score = capacityB + capacityA;

        if (candidate.candidateType == 4u) {
            score += candidateType4Bonus;
        } else if (candidate.candidateType == 3u) {
            const Float32 additionalLots =
                static_cast<Float32>(candidate.sourceLotCount - 1u);
            score += additionalLots * sourceLotCountWeight;
        }
    }

    if (developerWealth >= 1u && developerWealth <= 3u) {
        const std::size_t wealthIndex =
            static_cast<std::size_t>(developerWealth - 1u);
        for (std::size_t i = 0; i < 4; ++i) {
            const Float32 type0Or10 =
                static_cast<Float32>(candidate.networkType0Or10[i]) *
                kNetworkType0Or10Weights[wealthIndex];
            const Float32 type6 =
                static_cast<Float32>(candidate.networkType6[i]) *
                kNetworkType6Weights[wealthIndex];
            const Float32 types0Or10And6 = type6 + type0Or10;
            const Float32 type1 =
                static_cast<Float32>(candidate.networkType1[i]) *
                kNetworkType1Weights[wealthIndex];
            const Float32 networkScore = type1 + types0Or10And6;
            score += networkScore;
        }
    }

    return score;
}

bool TractDeveloperState::Read(cIGZIStream& stream) noexcept {
    Uint16 version = 0;
    stream >> version;
    if (!stream.Good() || version == 0u || version > 3u) {
        return false;
    }

    stream >> currentStyleIndex;
    stream >> yearsSinceLastStyleChange;

    if (version >= 2u) {
        Uint8 cycleByte = 0;
        stream >> cycleByte;
        cycleThroughStyles = (cycleByte != 0u);

        Uint32 styleCount = 0;
        stream >> styleCount;
        if (!stream.Good() ||
            static_cast<std::size_t>(styleCount) * sizeof(Uint32) >
                stream.Remaining()) {
            return false;
        }
        activeStyles.resize(styleCount);
        for (Uint32 i = 0; i < styleCount; ++i) {
            stream >> activeStyles[i];
        }
    } else {
        cycleThroughStyles = true;
        activeStyles = {0x2000u, 0x2001u, 0x2002u};
    }

    if (version >= 3u) {
        stream >> yearsBetweenStyleChanges;
    }

    return stream.Good();
}

bool TractDeveloperState::Write(cIGZOStream& stream) const {
    stream << static_cast<Uint16>(3u);
    stream << currentStyleIndex;
    stream << yearsSinceLastStyleChange;
    stream << static_cast<Uint8>(cycleThroughStyles ? 1u : 0u);
    stream << static_cast<Uint32>(activeStyles.size());
    for (Uint32 style : activeStyles) {
        stream << style;
    }
    stream << yearsBetweenStyleChanges;
    return true;
}

bool TractDeveloperState::ReadFromSC4Record(
    std::span<const Uint8> recordBytes) noexcept {
    if (recordBytes.size() < 2u) {
        return false;
    }
    std::size_t offset = 0;
    if (recordBytes.size() >= 14u) {
        Uint32 recordSize = 0;
        std::memcpy(&recordSize, recordBytes.data(), sizeof(Uint32));
        if (recordSize == recordBytes.size()) {
            offset = 12u;
        }
    }
    cIGZIStream stream(
        recordBytes.data() + offset, recordBytes.size() - offset);
    return Read(stream);
}

void TractDeveloperState::ApplyTuningDefaults(
    const TractDeveloperTuning& tuning) {
    if (activeStyles.empty()) {
        currentStyleIndex = 0;
        activeStyles = tuning.defaultActiveStyles;
    }
    if (currentStyleIndex < 0 ||
        static_cast<std::size_t>(currentStyleIndex) >= activeStyles.size()) {
        currentStyleIndex = 0;
    }
    if (yearsBetweenStyleChanges == 0) {
        yearsBetweenStyleChanges = tuning.yearsBetweenStyleChanges;
    }
}

void TractDeveloperState::OnNewYear() noexcept {
    ++yearsSinceLastStyleChange;
    if (yearsSinceLastStyleChange >= yearsBetweenStyleChanges) {
        yearsSinceLastStyleChange = 0;
        ++currentStyleIndex;
        if (currentStyleIndex < 0 ||
            static_cast<std::size_t>(currentStyleIndex) >= activeStyles.size()) {
            currentStyleIndex = 0;
        }
    }
}

Uint32 TractDeveloperState::GetCurrentStyle() const noexcept {
    if (activeStyles.empty() || currentStyleIndex < 0 ||
        static_cast<std::size_t>(currentStyleIndex) >= activeStyles.size()) {
        return 0u;
    }
    return activeStyles[static_cast<std::size_t>(currentStyleIndex)];
}

void TractDeveloperState::SetActiveStyles(std::span<const Uint32> styles) {
    // Exact cSC4TractDeveloper::SetActiveStyles @ 0x10013c0fa:
    // preserves the index of the previously selected style if still present,
    // otherwise resets currentStyleIndex to 0.
    Sint32 nextIndex = 0;
    if (!activeStyles.empty() && currentStyleIndex >= 0 &&
        static_cast<std::size_t>(currentStyleIndex) < activeStyles.size()) {
        const Uint32 prevStyle =
            activeStyles[static_cast<std::size_t>(currentStyleIndex)];
        for (std::size_t i = 0; i < styles.size(); ++i) {
            if (styles[i] == prevStyle) {
                nextIndex = static_cast<Sint32>(i);
                break;
            }
        }
    }
    activeStyles.assign(styles.begin(), styles.end());
    currentStyleIndex = nextIndex;
}

// Exact cSC4ResponseCurve::SetResponseCurveData @ 0x1000ac220.
bool cSC4ResponseCurve::SetResponseCurveData(std::span<const Point> points) {
    mSegments.clear();
    if (points.empty()) {
        return false;
    }

    constexpr Float64 kEpsilon = 1e-10;
    const Float32 negMaxFloat = -std::numeric_limits<Float32>::max();

    for (std::size_t i = 0; i < points.size(); ++i) {
        const Float32 x = points[i].x;
        const Float32 y = points[i].y;
        const Float32 prevX = (i == 0u) ? negMaxFloat : points[i - 1u].x;
        const Float32 prevY = (i == 0u) ? y : points[i - 1u].y;
        const Float32 dy = sse_float32::sub(y, prevY);
        // andps @ 0x1000ac2bf clears the binary32 sign BEFORE cvtss2sd.
        const Float32 absDyFloat = std::bit_cast<Float32>(
            std::bit_cast<Uint32>(dy) & 0x7fffffffu);
        const Float64 absDy = static_cast<Float64>(absDyFloat);

        Float32 slope = 0.0f;
        // Both jbe branches include unordered operands.
        if (!(absDy < kEpsilon)) {
            const Float32 dx = sse_float32::sub(x, prevX);
            if (!(static_cast<Float64>(dx) > static_cast<Float64>(dy) * kEpsilon)) {
                continue;
            }
            slope = sse_float32::div(dy, dx);
        }

        mSegments.push_back(Segment{x, y, prevX, prevY, slope});
        // First-slot writes @ 0x1000ac308..0x1000ac32a replace retained Y.
        // Clearing logical contents does not erase original backing storage.
        if (mSegments.size() == 1u) {
            mRetainedFirstY = y;
            mHasRetainedFirstY = true;
        }
    }

    return true;
}

bool cSC4ResponseCurve::SetResponseCurveData(
    std::span<const Float32> flatXYPairs) {
    const std::size_t pointCount = flatXYPairs.size() / 2u;
    std::vector<Point> points;
    points.reserve(pointCount);
    for (std::size_t i = 0; i < pointCount; ++i) {
        points.push_back(Point{flatXYPairs[i * 2u], flatXYPairs[i * 2u + 1u]});
    }
    return SetResponseCurveData(points);
}

// Exact present-property path @ 0x1000ac37a; shr @ 0x1000ac3cb
// floors the scalar count, and even zero pairs call the clearing setter.
bool cSC4ResponseCurve::SetResponseCurveData(const ExemplarProperty& prop) {
    return SetResponseCurveData(std::span<const Float32>(prop.floatValues));
}

Float32 cSC4ResponseCurve::GetOutputValue(Float32 input) const noexcept {
    Float32 out;
    if (TryGetOutputValue(input, out)) return out;
    // clean-room fallback (not authentic): legacy callers get zero only for
    // missing backing storage. Authentic adapters must propagate Try's fault.
    return 0.0f;
}

// Exact evaluation @ 0x1000ac4f6, with an explicit portable fault boundary
// for the null backing-slot read @ 0x1000ac542 (out remains untouched).
bool cSC4ResponseCurve::TryGetOutputValue(Float32 input, Float32& out) const noexcept {
    if (mSegments.empty()) {
        if (!mHasRetainedFirstY) return false;
        out = mRetainedFirstY;
        return true;
    }
    if (mSegments.size() < 2u) {
        out = mSegments.front().y;
        return true;
    }

    const Sint32 last = static_cast<Sint32>(mSegments.size() - 1u);
    Sint32 low = 0;
    Sint32 high = last;
    while (high > low) {
        const Sint32 mid = (low + high) >> 1;
        const Float32 midX = mSegments[static_cast<std::size_t>(mid)].x;
        // Complementary cmova/cmovbe @ 0x1000ac527: unordered advances low.
        if (midX > input) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }

    if (low <= 0) {
        out = mSegments.front().y;
        return true;
    }
    if (low >= last) {
        const Segment& endSeg = mSegments[static_cast<std::size_t>(last)];
        // jae @ 0x1000ac554 excludes unordered comparisons.
        if (input >= endSeg.x) {
            out = endSeg.y;
            return true;
        }
    }

    const Segment& seg = mSegments[static_cast<std::size_t>(low)];
    // subss, mulss, addss @ 0x1000ac55c..0x1000ac568; build with no FMA.
    const Float32 dx = sse_float32::sub(input, seg.prevX);
    const Float32 dy = sse_float32::mul(dx, seg.slope);
    out = sse_float32::add(dy, seg.prevY);
    return true;
}

bool cSC4ResponseCurve::GetMaxInputValue(Float32& outMaxX) const noexcept {
    if (mSegments.empty()) {
        return false;
    }
    outMaxX = mSegments.back().x;
    return true;
}

bool GrowthDeveloperTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    bool loaded = false;

    const auto readU32 = [&](Uint32 propId, Uint32& outVal) -> bool {
        auto it = exemplar.properties.find(propId);
        if (it == exemplar.properties.end() || it->second.intValues.empty()) {
            return false;
        }
        outVal = static_cast<Uint32>(it->second.intValues[0]);
        return true;
    };

    const auto readS32 = [&](Uint32 propId, Sint32& outVal) -> bool {
        auto it = exemplar.properties.find(propId);
        if (it == exemplar.properties.end() || it->second.intValues.empty()) {
            return false;
        }
        outVal = static_cast<Sint32>(it->second.intValues[0]);
        return true;
    };

    const auto readU8 = [&](Uint32 propId, Uint8& outVal) -> bool {
        auto it = exemplar.properties.find(propId);
        if (it == exemplar.properties.end() || it->second.intValues.empty()) {
            return false;
        }
        outVal = static_cast<Uint8>(it->second.intValues[0]);
        return true;
    };

    const auto readBool = [&](Uint32 propId, bool& outVal) -> bool {
        auto it = exemplar.properties.find(propId);
        if (it == exemplar.properties.end() || it->second.intValues.empty()) {
            return false;
        }
        outVal = (it->second.intValues[0] != 0);
        return true;
    };

    const auto readCurve = [&](Uint32 propId, cSC4ResponseCurve& curve) -> bool {
        auto it = exemplar.properties.find(propId);
        if (it == exemplar.properties.end()) {
            curve = cSC4ResponseCurve{};
            return false;
        }
        return curve.SetResponseCurveData(it->second);
    };

    if (readU32(0x00000021u, subType)) loaded = true;

    Uint8 devClassByte = 0;
    if (readU8(0x87cd6300u, devClassByte)) {
        developerClass = static_cast<Uint32>(devClassByte);
        loaded = true;
    }

    if (readU32(0x87cd6301u, sourceDemandId)) loaded = true;
    if (readU8(0x87cd6310u, tractSizeShift)) loaded = true;

    Uint8 wealthByte = 0;
    if (readU8(0x87cd6330u, wealthByte)) {
        wealth = static_cast<Uint32>(wealthByte);
        loaded = true;
    }

    Uint8 purposeByte = 0;
    if (readU8(0x87cd6332u, purposeByte)) {
        purpose = static_cast<Uint32>(purposeByte);
        loaded = true;
    }

    auto lvRange = exemplar.properties.find(0x87cd6333u);
    if (lvRange != exemplar.properties.end() &&
        lvRange->second.intValues.size() >= 2u) {
        minLandValue = static_cast<Uint8>(lvRange->second.intValues[0]);
        maxLandValue = static_cast<Uint8>(lvRange->second.intValues[1]);
        loaded = true;
    }

    useIntrinsicLandValue = false;
    if (readBool(0x87cd633fu, useIntrinsicLandValue)) loaded = true;

    if (readCurve(0x87cd6340u, landValueCurve)) loaded = true;
    if (readCurve(0x87cd6398u, slopeCurve)) loaded = true;
    if (readCurve(0x87cd6343u, resProximityCurves[0])) loaded = true;
    if (readCurve(0x87cd6344u, resProximityCurves[1])) loaded = true;
    if (readCurve(0x87cd6345u, resProximityCurves[2])) loaded = true;
    if (readCurve(0x87cd6391u, co2ProximityCurve)) loaded = true;
    if (readCurve(0x87cd6392u, co3ProximityCurve)) loaded = true;
    if (readCurve(0x87cd6341u, parkEffectCurve)) loaded = true;
    for (std::size_t i = 0; i < 4u; ++i) {
        if (readCurve(static_cast<Uint32>(0x87cd634bu + i), pollutionCurves[i])) {
            loaded = true;
        }
    }
    if (readCurve(0x87cd6348u, schoolEffectCurve)) loaded = true;
    if (readCurve(0x87cd6349u, hospitalEffectCurve)) loaded = true;
    if (readCurve(0x87cd634au, crimeEffectCurve)) loaded = true;
    if (readCurve(0x87cd6347u, tripLengthCurve)) loaded = true;
    if (readCurve(0x87cd6346u, trafficEffectCurve)) loaded = true;
    if (readCurve(0x87cd6399u, landmarkEffectCurve)) loaded = true;
    if (readCurve(0x87cd6397u, transientAuraCurve)) loaded = true;

    baseDesirability = 80;
    if (readS32(0x87cd6357u, baseDesirability)) loaded = true;

    desirabilityThreshold = 80;
    if (readS32(0x87cd6355u, desirabilityThreshold)) loaded = true;

    if (!readS32(0x87cd6354u, declineDesirabilityThreshold)) {
        declineDesirabilityThreshold = desirabilityThreshold;
    } else {
        loaded = true;
    }

    desirabilityFeedsDemandSupply = false;
    if (readBool(0x87cd6356u, desirabilityFeedsDemandSupply)) loaded = true;

    Uint8 stageCount = 0;
    readU8(0x87cd6360u, stageCount);
    stageCurves.clear();
    stageCurves.resize(stageCount);
    for (Uint8 i = 0; i < stageCount; ++i) {
        auto it = exemplar.properties.find(static_cast<Uint32>(0x87cd6361u + i));
        if (it != exemplar.properties.end()) {
            stageCurves[i].SetResponseCurveData(it->second);
            loaded = true;
        } else {
            const cSC4ResponseCurve::Point zeroPoint{0.0f, 0.0f};
            stageCurves[i].SetResponseCurveData(
                std::span<const cSC4ResponseCurve::Point>(&zeroPoint, 1u));
        }
    }

    if (readU8(0x87cd6373u, powerSupplyCap)) loaded = true;
    if (readU8(0x87cd6370u, waterSupplyCap)) loaded = true;
    if (readU8(0x87cd6371u, fireCoverageCap)) loaded = true;
    if (readU8(0x87cd6372u, parkCap)) loaded = true;

    abandonmentThresholds = {0.2f, 0.2f, 0.2f};
    auto abandonIt = exemplar.properties.find(0x87cd6380u);
    if (abandonIt != exemplar.properties.end() &&
        abandonIt->second.floatValues.size() >= 3u) {
        abandonmentThresholds[0] = abandonIt->second.floatValues[0];
        abandonmentThresholds[1] = abandonIt->second.floatValues[1];
        abandonmentThresholds[2] = abandonIt->second.floatValues[2];
        loaded = true;
    }

    Uint8 monopolyByte = 1u;
    if (readU8(0x87cd6381u, monopolyByte)) loaded = true;
    monopolyUse = static_cast<Uint32>(monopolyByte);

    stylePercentages.clear();
    auto styleIt = exemplar.properties.find(0x87cd63a0u);
    if (styleIt != exemplar.properties.end() &&
        styleIt->second.intValues.size() >= 2u) {
        constexpr Float32 kPercentScale = 0.009999999776482582f;
        for (std::size_t i = 0; i + 1u < styleIt->second.intValues.size(); i += 2u) {
            const Uint32 styleId =
                static_cast<Uint32>(styleIt->second.intValues[i]);
            const Uint32 rawPct =
                static_cast<Uint32>(styleIt->second.intValues[i + 1u]);
            stylePercentages.push_back(TractStylePercentageEntry{
                styleId,
                static_cast<Float32>(static_cast<Uint64>(rawPct)) * kPercentScale,
            });
        }
        loaded = true;
    }

    subjectiveThresholdLow.clear();
    auto subLowIt = exemplar.properties.find(0xcab1ce3au);
    if (subLowIt != exemplar.properties.end()) {
        for (Sint64 v : subLowIt->second.intValues) {
            subjectiveThresholdLow.push_back(static_cast<Sint32>(v));
        }
    }

    subjectiveThresholdHigh.clear();
    auto subHighIt = exemplar.properties.find(0xcab1ce3cu);
    if (subHighIt != exemplar.properties.end()) {
        for (Sint64 v : subHighIt->second.intValues) {
            subjectiveThresholdHigh.push_back(static_cast<Sint32>(v));
        }
    }

    return loaded;
}

Sint16 GrowthDeveloperTuning::EvaluateDesirability(
    const GrowthDesirabilityInputs& inputs,
    bool includeTripLength) const noexcept {
    Sint32 total = baseDesirability;

    if (!landValueCurve.Empty()) {
        total += static_cast<Sint32>(
            landValueCurve.GetOutputValue(static_cast<Float32>(inputs.landValue)));
    }
    if (!slopeCurve.Empty()) {
        total += static_cast<Sint32>(slopeCurve.GetOutputValue(inputs.slope));
    }
    for (std::size_t i = 0; i < 3u; ++i) {
        if (!resProximityCurves[i].Empty()) {
            total += static_cast<Sint32>(resProximityCurves[i].GetOutputValue(
                static_cast<Float32>(inputs.resProximity[i])));
        }
    }
    if (!co2ProximityCurve.Empty()) {
        total += static_cast<Sint32>(co2ProximityCurve.GetOutputValue(
            static_cast<Float32>(inputs.co2Proximity)));
    }
    if (!co3ProximityCurve.Empty()) {
        total += static_cast<Sint32>(co3ProximityCurve.GetOutputValue(
            static_cast<Float32>(inputs.co3Proximity)));
    }
    if (!parkEffectCurve.Empty()) {
        total += static_cast<Sint32>(parkEffectCurve.GetOutputValue(
            static_cast<Float32>(inputs.parkEffect)));
    }
    for (std::size_t i = 0; i < 4u; ++i) {
        if (!pollutionCurves[i].Empty()) {
            total += static_cast<Sint32>(pollutionCurves[i].GetOutputValue(
                static_cast<Float32>(inputs.pollution[i])));
        }
    }
    if (!schoolEffectCurve.Empty()) {
        total += static_cast<Sint32>(
            schoolEffectCurve.GetOutputValue(inputs.schoolEffect));
    }
    if (!hospitalEffectCurve.Empty()) {
        total += static_cast<Sint32>(
            hospitalEffectCurve.GetOutputValue(inputs.hospitalEffect));
    }
    if (!crimeEffectCurve.Empty()) {
        total += static_cast<Sint32>(
            crimeEffectCurve.GetOutputValue(static_cast<Float32>(inputs.crime)));
    }
    if (includeTripLength && !tripLengthCurve.Empty()) {
        total += static_cast<Sint32>(tripLengthCurve.GetOutputValue(
            static_cast<Float32>(inputs.minNonZeroTripLength)));
    }
    if (!trafficEffectCurve.Empty()) {
        total += static_cast<Sint32>(trafficEffectCurve.GetOutputValue(
            static_cast<Float32>(inputs.traffic)));
    }
    if (!landmarkEffectCurve.Empty()) {
        total += static_cast<Sint32>(landmarkEffectCurve.GetOutputValue(
            static_cast<Float32>(inputs.landmarkEffect)));
    }
    if (!transientAuraCurve.Empty()) {
        total += static_cast<Sint32>(transientAuraCurve.GetOutputValue(
            static_cast<Float32>(inputs.transientAura)));
    }

    return static_cast<Sint16>(total);
}

void GrowthDeveloperTuning::GetSubjectiveThresholds(
    Sint32 index,
    Sint32& outLow,
    Sint32& outHigh) const noexcept {
    if (index < 0) {
        outLow = -4;
        outHigh = 20;
        return;
    }
    const std::size_t idx = static_cast<std::size_t>(index);
    outLow = (idx < subjectiveThresholdLow.size())
                 ? subjectiveThresholdLow[idx]
                 : -4;
    outHigh = (idx < subjectiveThresholdHigh.size())
                  ? subjectiveThresholdHigh[idx]
                  : 20;
}

Sint16 ComputeGrowthTractTargetPopulation(
    Sint16 desirability,
    Uint16 capacity) noexcept {
    Float32 ratio = static_cast<Float32>(static_cast<Sint32>(desirability)) / 100.0f;
    ratio = std::max(0.0f, ratio);
    ratio = std::min(1.0f, ratio);
    constexpr Float32 kRoundMagic = 12582912.0f; // 0x4b400000
    const Float32 rounded =
        static_cast<Float32>(capacity) * ratio + kRoundMagic;
    Uint32 bits = 0;
    std::memcpy(&bits, &rounded, sizeof(Uint32));
    return static_cast<Sint16>(bits & 0xffffu);
}

Sint32 ComputeDesirabilityDemandSupply(
    bool desirabilityFeedsDemandSupply,
    Sint32 desirabilityThreshold,
    std::span<const GrowthDeveloperItem> growthItems,
    std::span<const Sint16> tempDesirabilityGrid,
    std::span<const Sint16> populationGrid,
    Sint32 gridTractsZ) noexcept {
    if (!desirabilityFeedsDemandSupply || gridTractsZ <= 0) {
        return 0;
    }

    Sint32 totalSupply = 0;
    for (const GrowthDeveloperItem& item : growthItems) {
        if (item.tractX < 0 || item.tractZ < 0 || item.tractZ >= gridTractsZ) {
            continue;
        }
        const std::size_t idx =
            static_cast<std::size_t>(item.tractX) *
                static_cast<std::size_t>(gridTractsZ) +
            static_cast<std::size_t>(item.tractZ);
        if (idx >= tempDesirabilityGrid.size() || idx >= populationGrid.size()) {
            continue;
        }

        const Sint32 delta =
            desirabilityThreshold - static_cast<Sint32>(tempDesirabilityGrid[idx]);
        if (delta <= 0) {
            continue;
        }
        const Float32 ratio =
            std::min(1.0f, static_cast<Float32>(delta) / 100.0f);
        const Float32 pop = static_cast<Float32>(populationGrid[idx]);
        totalSupply += static_cast<Sint32>(pop * ratio);
    }

    return totalSupply;
}

Float32 ComputeStageLookupPopulation(
    Uint32 developerClass,
    Float32 residentialDemand1001Supply,
    Float32 commercialDemand3001Supply,
    Float32 commercialRegionDemand3301Supply,
    Float32 industrialDemand4001Supply,
    Sint32 connectedNeighborCitiesCount,
    Sint32 neighborConnectionsCount,
    Float32 stagePopulationMultiplier) noexcept {
    Float32 basePop = 0.0f;
    if (developerClass == 3u) {
        basePop = industrialDemand4001Supply;
    } else if (developerClass == 2u) {
        Float32 neighborBonus = 0.0f;
        if (neighborConnectionsCount > 0) {
            neighborBonus =
                (static_cast<Float32>(connectedNeighborCitiesCount) /
                 static_cast<Float32>(neighborConnectionsCount)) *
                commercialRegionDemand3301Supply * 0.5f;
        }
        basePop = commercialDemand3001Supply + neighborBonus;
    } else if (developerClass == 1u) {
        Float32 neighborBonus = 0.0f;
        if (neighborConnectionsCount > 0) {
            neighborBonus =
                (static_cast<Float32>(connectedNeighborCitiesCount) /
                 static_cast<Float32>(neighborConnectionsCount)) *
                commercialRegionDemand3301Supply * 0.5f;
        }
        basePop = std::max(
            residentialDemand1001Supply,
            commercialDemand3001Supply + neighborBonus);
    }

    return basePop * stagePopulationMultiplier;
}

void BuildGrowthAndDeclineLists(
    const GrowthDeveloperTuning& tuning,
    std::span<const Sint8> zonableCellsGrid,
    std::span<const Sint16> tempDesirabilityGrid,
    Sint32 gridTractsX,
    Sint32 gridTractsZ,
    Float32 scaledStageLookupPopulation,
    std::vector<GrowthDeveloperItem>& outGrowthList,
    std::vector<GrowthDeveloperItem>& outDeclineList) {
    outGrowthList.clear();
    outDeclineList.clear();
    if (gridTractsX <= 0 || gridTractsZ <= 0) {
        return;
    }

    const auto getDes = [&](Sint32 x, Sint32 z) noexcept -> Sint16 {
        const std::size_t idx =
            static_cast<std::size_t>(x) * static_cast<std::size_t>(gridTractsZ) +
            static_cast<std::size_t>(z);
        return idx < tempDesirabilityGrid.size() ? tempDesirabilityGrid[idx] : 0;
    };

    // Step 17: enumerate all tracts with zonableCellsGrid(x, z) != 0.
    for (Sint32 x = 0; x < gridTractsX; ++x) {
        for (Sint32 z = 0; z < gridTractsZ; ++z) {
            const std::size_t idx =
                static_cast<std::size_t>(x) *
                    static_cast<std::size_t>(gridTractsZ) +
                static_cast<std::size_t>(z);
            if (idx < zonableCellsGrid.size() && zonableCellsGrid[idx] != 0) {
                outGrowthList.push_back(GrowthDeveloperItem{x, z, 1});
            }
        }
    }

    // Step 20.1: sort descending by desirability.
    std::stable_sort(
        outGrowthList.begin(),
        outGrowthList.end(),
        [&](const GrowthDeveloperItem& a, const GrowthDeveloperItem& b) {
            return getDes(a.tractX, a.tractZ) > getDes(b.tractX, b.tractZ);
        });

    // Step 20.2: populate outDeclineList from back to front (lowest desirability first).
    for (auto it = outGrowthList.rbegin(); it != outGrowthList.rend(); ++it) {
        if (static_cast<Sint32>(getDes(it->tractX, it->tractZ)) <
            tuning.declineDesirabilityThreshold) {
            outDeclineList.push_back(*it);
        }
    }

    // Step 20.4: erase items below desirabilityThreshold from outGrowthList.
    outGrowthList.erase(
        std::remove_if(
            outGrowthList.begin(),
            outGrowthList.end(),
            [&](const GrowthDeveloperItem& item) {
                return static_cast<Sint32>(getDes(item.tractX, item.tractZ)) <
                       tuning.desirabilityThreshold;
            }),
        outGrowthList.end());

    // Step 20.6: assign growthStage (1..N) across sorted outGrowthList.
    const Sint32 numStages = static_cast<Sint32>(tuning.stageCurves.size());
    if (numStages <= 0 || outGrowthList.empty()) {
        return;
    }

    const Float32 totalItemsFloat =
        static_cast<Float32>(outGrowthList.size());
    Sint32 currentStage = numStages;
    Float32 cumulativePct =
        tuning.stageCurves[static_cast<std::size_t>(numStages - 1)]
            .GetOutputValue(scaledStageLookupPopulation);

    for (std::size_t i = 0; i < outGrowthList.size(); ++i) {
        if (currentStage >= 2) {
            const Float32 percentile =
                static_cast<Float32>(static_cast<Sint32>(i) * 100) /
                totalItemsFloat;
            if (percentile >= cumulativePct) {
                Sint32 stageWalk = currentStage;
                while (true) {
                    const Float32 nextPct =
                        tuning.stageCurves[static_cast<std::size_t>(stageWalk - 2)]
                            .GetOutputValue(scaledStageLookupPopulation);
                    currentStage = stageWalk - 1;
                    cumulativePct += nextPct;
                    if (stageWalk < 3) {
                        break;
                    }
                    stageWalk = currentStage;
                    if (percentile < cumulativePct) {
                        break;
                    }
                }
            }
        }
        outGrowthList[i].growthStage = currentStage;
    }
}

namespace {

bool ReadSerializableRef(
    cIGZIStream& stream,
    GrowthDeveloperSaveRecord::GridRef& ref) noexcept {
    stream >> ref.memAddr;
    if (!stream.Good()) {
        return false;
    }
    if (ref.memAddr != 0u) {
        stream >> ref.clsid;
    } else {
        ref.clsid = 0u;
    }
    return stream.Good();
}

void WriteSerializableRef(
    cIGZOStream& stream,
    const GrowthDeveloperSaveRecord::GridRef& ref) {
    stream << ref.memAddr;
    if (ref.memAddr != 0u) {
        stream << ref.clsid;
    }
}

} // namespace

bool GrowthDeveloperSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version != 1u) {
        return false;
    }
    stream >> suppliedDemand;
    stream >> totalCapacity;
    return ReadSerializableRef(stream, zonableCellsGrid) &&
           ReadSerializableRef(stream, desirabilityGrid) &&
           ReadSerializableRef(stream, populationGrid) &&
           ReadSerializableRef(stream, capacityGrid) &&
           ReadSerializableRef(stream, maxCapacityGrid);
}

bool GrowthDeveloperSaveRecord::Write(cIGZOStream& stream) const {
    stream << static_cast<Uint16>(1u);
    stream << suppliedDemand;
    stream << totalCapacity;
    WriteSerializableRef(stream, zonableCellsGrid);
    WriteSerializableRef(stream, desirabilityGrid);
    WriteSerializableRef(stream, populationGrid);
    WriteSerializableRef(stream, capacityGrid);
    WriteSerializableRef(stream, maxCapacityGrid);
    return true;
}

bool ParseGrowthDeveloperSubfile(
    std::span<const Uint8> subfileBytes,
    std::vector<GrowthDeveloperSaveRecord>& outRecords) {
    outRecords.clear();
    std::size_t pos = 0;
    while (pos + 12u <= subfileBytes.size()) {
        Uint32 recSize = 0;
        std::memcpy(&recSize, subfileBytes.data() + pos, sizeof(Uint32));
        if (recSize < 42u || pos + recSize > subfileBytes.size()) {
            break;
        }
        GrowthDeveloperSaveRecord rec{};
        std::memcpy(&rec.memAddr, subfileBytes.data() + pos + 8u, sizeof(Uint32));
        cIGZIStream stream(subfileBytes.data() + pos + 12u, recSize - 12u);
        if (!rec.Read(stream)) {
            return false;
        }
        outRecords.push_back(rec);
        pos += recSize;
    }
    return !outRecords.empty();
}

bool BuildingDevelopmentSimSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version != 4u) {
        return false;
    }
    if (!ReadSerializableRef(stream, developmentMapGrid) ||
        !ReadSerializableRef(stream, energyDemand)) {
        return false;
    }

    Uint32 devCount = 0;
    stream >> devCount;
    if (!stream.Good() || devCount > 64u) {
        return false;
    }
    developers.clear();
    developers.reserve(devCount);
    for (Uint32 i = 0; i < devCount; ++i) {
        DeveloperMapEntry entry{};
        stream >> entry.subType;
        if (!stream.Good() || !ReadSerializableRef(stream, entry.developer)) {
            return false;
        }
        developers.push_back(entry);
    }

    Uint32 specialCount = 0;
    stream >> specialCount;
    if (!stream.Good() || specialCount > 65536u) {
        return false;
    }
    specialBuildings.clear();
    specialBuildings.reserve(specialCount);
    for (Uint32 i = 0; i < specialCount; ++i) {
        ObjectRef ref{};
        if (!ReadSerializableRef(stream, ref)) {
            return false;
        }
        specialBuildings.push_back(ref);
    }

    Uint32 stageCount = 0;
    stream >> stageCount;
    if (!stream.Good() || stageCount > 64u) {
        return false;
    }
    targetGrowthStages.clear();
    targetGrowthStages.reserve(stageCount);
    for (Uint32 i = 0; i < stageCount; ++i) {
        TargetGrowthStageEntry entry{};
        stream >> entry.subType >> entry.maxGrowthStageSeen;
        if (!stream.Good()) {
            return false;
        }
        targetGrowthStages.push_back(entry);
    }

    Uint32 markMapCount = 0;
    stream >> markMapCount;
    if (!stream.Good() || markMapCount > 1024u) {
        return false;
    }
    growthMarks.clear();
    growthMarks.reserve(markMapCount);
    for (Uint32 i = 0; i < markMapCount; ++i) {
        GrowthMarkEntry entry{};
        Uint32 coordCount = 0;
        stream >> entry.markKey >> coordCount;
        if (!stream.Good() || coordCount > 65536u) {
            return false;
        }
        entry.tracts.reserve(coordCount);
        for (Uint32 j = 0; j < coordCount; ++j) {
            Sint32 tx = 0;
            Sint32 tz = 0;
            stream >> tx >> tz;
            if (!stream.Good()) {
                return false;
            }
            entry.tracts.emplace_back(tx, tz);
        }
        growthMarks.push_back(std::move(entry));
    }

    Uint32 declineCount = 0;
    stream >> declineCount;
    if (!stream.Good() || declineCount > 65536u) {
        return false;
    }
    declinePlans.clear();
    declinePlans.reserve(declineCount);
    for (Uint32 i = 0; i < declineCount; ++i) {
        DeclinePlanEntry entry{};
        if (!ReadSerializableRef(stream, entry.developer)) {
            return false;
        }
        stream >> entry.tractX >> entry.tractZ >> entry.wealthOrStage >>
            entry.targetJulianDay;
        if (!stream.Good()) {
            return false;
        }
        declinePlans.push_back(entry);
    }

    Uint8 step10 = 0;
    Uint8 stage7 = 0;
    Uint8 stage8 = 0;
    stream >> step10 >> stage7 >> stage8;
    if (!stream.Good()) {
        return false;
    }
    hasReachedStep10 = (step10 != 0u);
    hasSeenStage7 = (stage7 != 0u);
    hasSeenStage8 = (stage8 != 0u);
    return true;
}

bool BuildingDevelopmentSimSaveRecord::Write(cIGZOStream& stream) const {
    stream << static_cast<Uint16>(4u);
    WriteSerializableRef(stream, developmentMapGrid);
    WriteSerializableRef(stream, energyDemand);

    stream << static_cast<Uint32>(developers.size());
    for (const auto& entry : developers) {
        stream << entry.subType;
        WriteSerializableRef(stream, entry.developer);
    }

    stream << static_cast<Uint32>(specialBuildings.size());
    for (const auto& ref : specialBuildings) {
        WriteSerializableRef(stream, ref);
    }

    stream << static_cast<Uint32>(targetGrowthStages.size());
    for (const auto& entry : targetGrowthStages) {
        stream << entry.subType << entry.maxGrowthStageSeen;
    }

    stream << static_cast<Uint32>(growthMarks.size());
    for (const auto& entry : growthMarks) {
        stream << entry.markKey << static_cast<Uint32>(entry.tracts.size());
        for (const auto& [tx, tz] : entry.tracts) {
            stream << tx << tz;
        }
    }

    stream << static_cast<Uint32>(declinePlans.size());
    for (const auto& entry : declinePlans) {
        WriteSerializableRef(stream, entry.developer);
        stream << entry.tractX << entry.tractZ << entry.wealthOrStage
               << entry.targetJulianDay;
    }

    stream << static_cast<Uint8>(hasReachedStep10 ? 1u : 0u)
           << static_cast<Uint8>(hasSeenStage7 ? 1u : 0u)
           << static_cast<Uint8>(hasSeenStage8 ? 1u : 0u);
    return true;
}

bool BuildingDevelopmentSimSaveRecord::ReadFromSC4Record(
    std::span<const Uint8> recordBytes) noexcept {
    if (recordBytes.size() < 14u) {
        return false;
    }
    Uint32 recSize = 0;
    std::memcpy(&recSize, recordBytes.data(), sizeof(Uint32));
    if (recSize < 14u || recSize > recordBytes.size()) {
        return false;
    }
    std::memcpy(&memAddr, recordBytes.data() + 8u, sizeof(Uint32));
    cIGZIStream stream(recordBytes.data() + 12u, recSize - 12u);
    return Read(stream);
}

bool DemandExemplarSpec::LoadFromExemplar(
    const cGZExemplar& exemplar,
    Uint32 fallbackId) {
    demandId = fallbackId;
    auto idIt = exemplar.properties.find(0x00000021u);
    if (idIt != exemplar.properties.end() && !idIt->second.intValues.empty()) {
        demandId = static_cast<Uint32>(idIt->second.intValues[0]);
    }
    if (demandId == 0u) {
        return false;
    }

    const auto readOptS32 = [&](Uint32 propId, Sint32& outVal) -> bool {
        auto it = exemplar.properties.find(propId);
        if (it == exemplar.properties.end() || it->second.intValues.empty()) {
            return false;
        }
        outVal = static_cast<Sint32>(it->second.intValues[0]);
        return true;
    };

    hasActiveDemandMax = readOptS32(0x47bb3f20u, activeDemandMax);
    hasActiveDemandMin = readOptS32(0x47bb3f21u, activeDemandMin);
    hasHysteresisBase = readOptS32(0x47bb3f10u, hysteresisBase);
    hasHysteresisMax = readOptS32(0x47bb3f11u, hysteresisMax);

    regionUse = 0u;
    auto regIt = exemplar.properties.find(0x47bb3f60u);
    if (regIt != exemplar.properties.end() && !regIt->second.intValues.empty()) {
        regionUse = static_cast<Uint8>(regIt->second.intValues[0]);
    }

    links.clear();
    Uint32 serialCounter = 0u;

    const auto parsePairedLinks = [&](Uint32 propId,
                                      Uint32 serialBase,
                                      DemandEconomyLinkType type) {
        auto it = exemplar.properties.find(propId);
        if (it == exemplar.properties.end()) {
            return;
        }
        for (std::size_t i = 0; i + 1u < it->second.intValues.size(); i += 2u) {
            DemandEconomyLink link{};
            link.sourceDemandId = demandId;
            link.linkSerial = serialBase + serialCounter++;
            link.targetDemandId = static_cast<Uint32>(it->second.intValues[i]);
            link.linkType = type;
            link.coefficient =
                static_cast<Float32>(it->second.intValues[i + 1u]) * 0.01f;
            links.push_back(link);
        }
    };

    const auto parseSingleLinks = [&](Uint32 propId,
                                      Uint32 serialBase,
                                      DemandEconomyLinkType type) {
        auto it = exemplar.properties.find(propId);
        if (it == exemplar.properties.end()) {
            return;
        }
        for (const Sint64 targetId : it->second.intValues) {
            DemandEconomyLink link{};
            link.sourceDemandId = demandId;
            link.linkSerial = serialBase + serialCounter;
            link.targetDemandId = static_cast<Uint32>(targetId);
            link.linkType = type;
            link.coefficient = 1.0f;
            links.push_back(link);
        }
    };

    // Exact order from cSC4DemandSimulator::CreateDemandFromExemplar @ 0x100229954:
    parsePairedLinks(0x47bb3f31u, 10000u, DemandEconomyLinkType::SupplyDrivesSupply);
    parsePairedLinks(0x47bb3f30u, 20000u, DemandEconomyLinkType::DemandDrivesDemand);
    parsePairedLinks(0x47bb3f32u, 30000u, DemandEconomyLinkType::DemandDrivesSupply);
    parseSingleLinks(0x47bb3f41u, 40000u, DemandEconomyLinkType::SatisfactionLink);
    parseSingleLinks(0x47bb3f50u, 50000u, DemandEconomyLinkType::AggregateChildren);

    return true;
}

bool DemandSimulatorTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    bool loaded = false;

    auto curveB5 = exemplar.properties.find(0x4a1f38b5u);
    if (curveB5 != exemplar.properties.end() &&
        neutralTaxRateCurve.SetResponseCurveData(curveB5->second)) {
        loaded = true;
    }
    auto curveB6 = exemplar.properties.find(0x4a1f38b6u);
    if (curveB6 != exemplar.properties.end() &&
        taxModifierCurve.SetResponseCurveData(curveB6->second)) {
        loaded = true;
    }

    taxHistoryLookbackDays = 90u;
    auto b7It = exemplar.properties.find(0x4a1f38b7u);
    if (b7It != exemplar.properties.end() && !b7It->second.intValues.empty()) {
        taxHistoryLookbackDays = static_cast<Uint32>(b7It->second.intValues[0]);
        loaded = true;
    }

    neighborCoefficients.fill(0.0f);
    auto c5It = exemplar.properties.find(0x4a1f38c5u);
    if (c5It != exemplar.properties.end() &&
        c5It->second.floatValues.size() >= 13u) {
        for (std::size_t i = 0; i < 13u; ++i) {
            neighborCoefficients[i] = c5It->second.floatValues[i];
        }
        loaded = true;
    }

    const auto readF32 = [&](Uint32 propId, Float32 defaultVal, Float32& outVal) {
        outVal = defaultVal;
        auto it = exemplar.properties.find(propId);
        if (it != exemplar.properties.end() && !it->second.floatValues.empty()) {
            outVal = it->second.floatValues[0];
            loaded = true;
        }
    };
    readF32(0x4a1f38c6u, 1.0f, coeffC6);
    readF32(0x4a1f38c7u, 2.0f, coeffC7);
    readF32(0x4a1f38c8u, 20.0f, passengerConnScale);
    readF32(0x4a1f38c9u, 0.1f, coeffC9);
    readF32(0x4a1f38cau, 20.0f, coeffCA);

    salaryWeights.fill(100u);
    auto salIt = exemplar.properties.find(0x8a63f2a1u);
    if (salIt != exemplar.properties.end() &&
        salIt->second.intValues.size() >= 12u) {
        for (std::size_t i = 0; i < 12u; ++i) {
            salaryWeights[i] = static_cast<Uint32>(salIt->second.intValues[i]);
        }
        loaded = true;
    }

    return loaded;
}

Float32 DemandSimulatorTuning::ComputeNeutralTaxRate(
    Float32 resSupply1001,
    Float32 comSupply3001,
    Float32 indSupply4001) const noexcept {
    // Exact cSC4DemandSimulator::GetNeutralTaxRate @ 0x100225e44:
    // (comSupply3001 + resSupply1001) + indSupply4001
    const Float32 totalSupply = (comSupply3001 + resSupply1001) + indSupply4001;
    return neutralTaxRateCurve.GetOutputValue(totalSupply);
}

Float32 DemandSimulatorTuning::ComputeTaxRateModifier(
    Float32 avgTaxRate,
    Float32 neutralTaxRate) const noexcept {
    // Exact UpdateTaxAndEconomyModifiers @ 0x10022722b-0x10022725b:
    const Float32 denom = std::max(0.1f, neutralTaxRate);
    const Float32 ratio = (avgTaxRate * 0.1f) / denom;
    return taxModifierCurve.GetOutputValue(ratio);
}

bool DemandSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version > 2u) {
        return false;
    }
    if (version != 2u) {
        Uint32 unused = 0;
        for (int i = 0; i < 4; ++i) {
            stream >> unused;
        }
    }
    stream >> commercialConnectionBonus
           >> industrialConnectionBonus
           >> connectionUtilA
           >> passengerConnectionSupply;
    if (!stream.Good()) {
        return false;
    }

    Uint32 demandCount = 0;
    stream >> demandCount;
    if (!stream.Good() || demandCount > 4096u) {
        return false;
    }
    demands.clear();
    demands.reserve(demandCount);
    for (Uint32 i = 0; i < demandCount; ++i) {
        DemandMapEntry entry{};
        stream >> entry.demandId;
        if (!stream.Good() || !ReadSerializableRef(stream, entry.demand)) {
            return false;
        }
        demands.push_back(entry);
    }

    Uint32 censusCount = 0;
    stream >> censusCount;
    if (!stream.Good() || censusCount > 4096u) {
        return false;
    }
    census.clear();
    census.reserve(censusCount);
    for (Uint32 i = 0; i < censusCount; ++i) {
        CensusEntry entry{};
        stream >> entry.censusKey >> entry.value;
        if (!stream.Good()) {
            return false;
        }
        census.push_back(entry);
    }

    return stream.Good();
}

bool DemandSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << static_cast<Uint16>(2u);
    stream << commercialConnectionBonus
           << industrialConnectionBonus
           << connectionUtilA
           << passengerConnectionSupply;

    stream << static_cast<Uint32>(demands.size());
    for (const auto& entry : demands) {
        stream << entry.demandId;
        WriteSerializableRef(stream, entry.demand);
    }

    stream << static_cast<Uint32>(census.size());
    for (const auto& entry : census) {
        stream << entry.censusKey << entry.value;
    }
    return true;
}

bool DemandSimulatorSaveRecord::ReadFromSC4Record(
    std::span<const Uint8> recordBytes) noexcept {
    if (recordBytes.size() < 14u) {
        return false;
    }
    Uint32 recSize = 0;
    std::memcpy(&recSize, recordBytes.data(), sizeof(Uint32));
    if (recSize < 14u || recSize > recordBytes.size()) {
        return false;
    }
    std::memcpy(&memAddr, recordBytes.data() + 8u, sizeof(Uint32));
    cIGZIStream stream(recordBytes.data() + 12u, recSize - 12u);
    return Read(stream);
}

CensusCellContribution ComputeCensusCellContribution(
    const CensusCellInput& input) noexcept {
    CensusCellContribution out{};

    // Exact purpose bitmask check from PerformCensus @ 0x100227c72-0x100227c87:
    // non-residential branch when purpose <= 8 and ((0x1ec >> purpose) & 1) != 0
    // (purposes 2=Cs, 3=Co, 5=I-R, 6=I-D, 7=I-M, 8=I-HT).
    if (input.buildingPurpose <= 8u &&
        ((0x1ecu >> input.buildingPurpose) & 1u) != 0u) {
        Uint32 subType = 0;
        switch (input.buildingPurpose) {
            case 2u: subType = 0x3100u + (input.buildingWealth << 4u); break;
            case 3u: subType = 0x3300u + (input.buildingWealth << 4u); break;
            case 5u: subType = 0x4100u; break;
            case 6u: subType = 0x4200u; break;
            case 7u: subType = 0x4300u; break;
            case 8u: subType = 0x4400u; break;
            default: return out;
        }
        const Float32 ratio = std::clamp(
            static_cast<Float32>(input.developerPopulation) / 100.0f,
            0.0f,
            1.0f);
        out.primaryKey = subType + 1u;
        out.primaryValue = static_cast<Float32>(input.buildingCapacity) * ratio;
        return out;
    }

    if (input.buildingPurpose == 1u) {
        const Uint32 subType = 0x1000u + (input.buildingWealth << 4u);
        const Float32 ratio = std::clamp(
            static_cast<Float32>(input.developerPopulation) / 100.0f,
            0.0f,
            1.0f);
        const Float32 basePop =
            static_cast<Float32>(input.buildingCapacity) * ratio;
        const Float32 retainedPop = basePop * input.populationRetention;

        // Exact EQ bracket formula @ 0x100227e26-0x100227e5a:
        const Uint8 eqByte =
            static_cast<Uint8>(static_cast<Sint32>(input.educationQuotient));
        const Sint32 rawBracket =
            static_cast<Sint32>(static_cast<Float32>(eqByte) / 50.09999847f + 1.0f);
        const Sint32 eqBracket = std::clamp(rawBracket, 1, 4);

        out.eqKey = 0x2100u + (input.buildingWealth << 4u) +
                    static_cast<Uint32>(eqBracket);
        out.eqValue = retainedPop;
        out.primaryKey = subType + 1u;
        out.primaryValue = basePop;
    }

    return out;
}

bool DemandNodeState::EndOfCycle() noexcept {
    // Exact cSC4Demand::EndOfCycle @ 0x100200eda + CalculateActiveDemand(false) @ 0x100200f36:
    const bool changed = (newSupply != 0.0f) || (newDemand != 0.0f);
    supplyValue += newSupply;
    demandValue += newDemand;
    newSupply = 0.0f;
    newDemand = 0.0f;

    const Float32 rawDemand = std::clamp(
        demandValue - supplyValue,
        activeDemandMin,
        activeDemandMax);
    const Float32 modifier = economyModifier + taxModifier;
    const Float32 modifiedDemand =
        rawDemand + std::min(demandValue * modifier, activeDemandMax * modifier);
    const Float32 clampedModified = std::clamp(
        modifiedDemand,
        activeDemandMin,
        activeDemandMax);
    const Float32 capMaximum = activeDemandMax - demandCap * activeDemandMax;
    activeDemand = std::max(activeDemandMin, std::min(clampedModified, capMaximum));
    return changed;
}

Uint32 PropagateDemandEconomyLinks(
    std::span<const DemandEconomyLink> links,
    std::unordered_map<Uint32, DemandNodeState>& inOutDemands,
    Uint32 maxIterations) {
    // Collect ordered demand IDs to match std::map<Uint32, cISC4Demand*> traversal
    std::vector<Uint32> orderedIds;
    orderedIds.reserve(inOutDemands.size());
    for (const auto& [id, node] : inOutDemands) {
        (void)node;
        orderedIds.push_back(id);
    }
    std::sort(orderedIds.begin(), orderedIds.end());

    Uint32 iterations = 0u;
    while (iterations < maxIterations) {
        ++iterations;
        bool converged = true;

        for (const Uint32 sourceId : orderedIds) {
            auto srcIt = inOutDemands.find(sourceId);
            if (srcIt == inOutDemands.end()) {
                continue;
            }
            DemandNodeState& source = srcIt->second;

            Float32 aggSupply = 0.0f;
            Float32 aggDemand = 0.0f;
            bool hasAggregateLink = false;

            for (const auto& link : links) {
                if (link.sourceDemandId != sourceId) {
                    continue;
                }
                auto tgtIt = inOutDemands.find(link.targetDemandId);
                if (tgtIt == inOutDemands.end()) {
                    continue;
                }
                DemandNodeState& target = tgtIt->second;

                switch (link.linkType) {
                    case DemandEconomyLinkType::SupplyDrivesSupply: {
                        const Float32 delta = source.newSupply * link.coefficient;
                        if (std::abs(delta) > 0.01f) {
                            target.newSupply += delta;
                            converged = false;
                        }
                        break;
                    }
                    case DemandEconomyLinkType::DemandDrivesDemand: {
                        const Float32 delta = source.newDemand * link.coefficient;
                        if (std::abs(delta) > 0.01f) {
                            target.newDemand += delta;
                            converged = false;
                        }
                        break;
                    }
                    case DemandEconomyLinkType::DemandDrivesSupply: {
                        const Float32 delta = source.newDemand * link.coefficient;
                        if (std::abs(delta) > 0.01f) {
                            target.newDemand += delta;
                            converged = false;
                        }
                        break;
                    }
                    case DemandEconomyLinkType::SatisfactionLink: {
                        const Float32 denom = std::max(0.01f, source.supplyValue);
                        const Float32 numer = std::max(0.0f, source.demandValue);
                        const Float32 ratio =
                            (numer < denom) ? (numer / denom) : 1.0f;
                        target.demandCap = ratio;
                        break;
                    }
                    case DemandEconomyLinkType::AggregateChildren: {
                        aggSupply += target.supplyValue;
                        aggDemand += target.demandValue;
                        hasAggregateLink = true;
                        break;
                    }
                }
            }

            source.EndOfCycle();

            if (hasAggregateLink) {
                const Float32 diffSupply = aggSupply - source.supplyValue;
                const Float32 diffDemand = aggDemand - source.demandValue;
                if (std::abs(diffSupply) > 0.01f) {
                    source.newSupply += diffSupply;
                    converged = false;
                }
                if (std::abs(diffDemand) > 0.01f) {
                    source.newDemand += diffDemand;
                    converged = false;
                }
            }
        }

        if (converged) {
            break;
        }
    }

    return iterations;
}

Uint32 GetDeveloperTypeFromPurposeAndWealth(
    Uint32 purpose,
    Uint32 wealth) noexcept {
    // Exact cSC4BuildingDevelopmentSimulator::GetDeveloperType_Static @ 0x1001530e6:
    switch (purpose) {
        case 1u:
            if (wealth >= 1u && wealth <= 3u) {
                return 0x1010u + ((wealth - 1u) << 4u);
            }
            return 0u;
        case 2u:
            if (wealth >= 1u && wealth <= 3u) {
                return 0x3110u + ((wealth - 1u) << 4u);
            }
            return 0u;
        case 3u:
            if (wealth == 2u) return 0x3320u;
            if (wealth == 3u) return 0x3330u;
            return 0u;
        case 5u:
            return 0x4100u;
        case 6u:
            return 0x4200u;
        case 7u:
            return 0x4300u;
        case 8u:
            return 0x4400u;
        default:
            return 0u;
    }
}

Uint32 GetWealthFromDeveloperType(Uint32 developerType) noexcept {
    // Exact cSC4BuildingDevelopmentSimulator::GetWealth_Static @ 0x1001534d8:
    switch (developerType) {
        case 0x1010u:
        case 0x3110u:
        case 0x4100u:
            return 1u;
        case 0x1020u:
        case 0x3120u:
        case 0x3320u:
        case 0x4200u:
        case 0x4300u:
            return 2u;
        case 0x1030u:
        case 0x3130u:
        case 0x3330u:
        case 0x4400u:
            return 3u;
        default:
            return 0u;
    }
}

Float32 ComputeLotMaxCapacityPerCell(
    Uint16 buildingCapacity,
    Sint32 lotSizeX,
    Sint32 lotSizeZ) noexcept {
    // Exact cSC4BuildingDevelopmentSimulator::CalculateMaxCapacity @ 0x10014ccd9-0x10014cd12:
    const Sint32 area = lotSizeX * lotSizeZ;
    if (area <= 0) {
        return 0.0f;
    }
    const Float32 rawPerCell =
        static_cast<Float32>(buildingCapacity) / static_cast<Float32>(area) +
        0.5f;
    if (buildingCapacity != 0u) {
        return std::max(1.0f, rawPerCell);
    }
    return rawPerCell;
}

bool ParseDemandAgentEffectsFromExemplar(
    const cGZExemplar& exemplar,
    std::vector<DemandAgentEffect>& outEffects,
    bool* outIsConditionalEffect) {
    outEffects.clear();
    if (outIsConditionalEffect) {
        *outIsConditionalEffect = false;
        auto condIt = exemplar.properties.find(0x27812844u);
        if (condIt != exemplar.properties.end() &&
            !condIt->second.intValues.empty()) {
            *outIsConditionalEffect = (condIt->second.intValues[0] != 0);
        }
    }

    // 1. Property 0x27812841 (Demand Created, optional float array 0x27812843)
    // @ 0x100225f59-0x100226095 -> sets demandEffect (+0x18)
    auto prop41 = exemplar.properties.find(0x27812841u);
    if (prop41 != exemplar.properties.end()) {
        auto prop43 = exemplar.properties.find(0x27812843u);
        const bool hasFloat43 =
            (prop43 != exemplar.properties.end() &&
             !prop43->second.floatValues.empty());
        for (std::size_t i = 0; i + 1u < prop41->second.intValues.size(); i += 2u) {
            DemandAgentEffect eff{};
            eff.demandId = static_cast<Uint32>(prop41->second.intValues[i]);
            eff.supplyEffect = 0.0f;
            if (hasFloat43 && (i / 2u) < prop43->second.floatValues.size()) {
                eff.demandEffect = prop43->second.floatValues[i / 2u];
            } else {
                eff.demandEffect =
                    static_cast<Float32>(prop41->second.intValues[i | 1u]);
            }
            outEffects.push_back(eff);
        }
    }

    // 2. Property 0x27812840 (Demand Satisfied / Cap Relief, optional float array 0x27812842)
    // @ 0x1002260a8-0x1002261eb -> sets supplyEffect (+0x14)
    auto prop40 = exemplar.properties.find(0x27812840u);
    if (prop40 != exemplar.properties.end()) {
        auto prop42 = exemplar.properties.find(0x27812842u);
        const bool hasFloat42 =
            (prop42 != exemplar.properties.end() &&
             !prop42->second.floatValues.empty());
        for (std::size_t i = 0; i + 1u < prop40->second.intValues.size(); i += 2u) {
            DemandAgentEffect eff{};
            eff.demandId = static_cast<Uint32>(prop40->second.intValues[i]);
            if (hasFloat42 && (i / 2u) < prop42->second.floatValues.size()) {
                eff.supplyEffect = prop42->second.floatValues[i / 2u];
            } else {
                eff.supplyEffect =
                    static_cast<Float32>(prop40->second.intValues[i | 1u]);
            }
            eff.demandEffect = 0.0f;
            outEffects.push_back(eff);
        }
    }

    return !outEffects.empty();
}

void ApplyDemandAgentEffects(
    std::span<const DemandAgentEffect> effects,
    bool addOrRemove,
    Float32 percentage,
    std::unordered_map<Uint32, DemandNodeState>& inOutDemands) noexcept {
    // Exact cSC4DemandSimulator::AddAgentEffect @ 0x100226ed2:
    const Float32 sign = addOrRemove ? 1.0f : -1.0f;
    for (const auto& eff : effects) {
        DemandNodeState& target = inOutDemands[eff.demandId];
        target.demandId = eff.demandId;
        if (eff.supplyEffect != 0.0f) {
            target.newSupply += (eff.supplyEffect * percentage) * sign;
        }
        if (eff.demandEffect != 0.0f) {
            target.newDemand += (eff.demandEffect * percentage) * sign;
        }
    }
}

ConnectionUtilizationOutputs EvaluateDemandConnectionUtilization(
    const DemandSimulatorTuning& tuning,
    const ConnectionUtilizationInputs& inputs) noexcept {
    ConnectionUtilizationOutputs out{};

    // Step 1: passenger trip supply @ 0x100227f8e-0x100227fa0
    out.newPassengerConnectionSupply =
        static_cast<Float32>(inputs.totalPassengerTrips) *
        tuning.passengerConnScale;

    // Step 2: log2 sum of the 6 neighbor connection categories @ 0x100228a89-0x100228b94
    const Float32 countA = static_cast<Float32>(std::max(1, inputs.railAndMonorailCountA));
    const Float32 countB = static_cast<Float32>(std::max(1, inputs.subwayCountB));
    const Float32 countC = static_cast<Float32>(std::max(1, inputs.highwayCountC));
    const Float32 countD = static_cast<Float32>(std::max(1, inputs.groundHighwayCountD));
    const Float32 countE = static_cast<Float32>(std::max(1, inputs.lightRailCountE));
    const Float32 countF = static_cast<Float32>(std::max(1, inputs.roadAvenueFerryCountF));

    Float32 logSum = std::log(countA) * tuning.neighborCoefficients[2];
    logSum += std::log(countB) * tuning.neighborCoefficients[6];
    logSum += std::log(countC) * tuning.neighborCoefficients[0];
    logSum += std::log(countD) * tuning.neighborCoefficients[11];
    logSum += std::log(countE) * tuning.neighborCoefficients[1];
    logSum += std::log(countF) * tuning.neighborCoefficients[7];

    // Multiply by 1.44269502f (1 / ln(2)) @ 0x100228b7c
    out.newConnectionUtilA = logSum * 1.44269502f;

    // Step 3: seaport trips @ 0x100228b94-0x100228be8
    const Float32 seaportFloat = static_cast<Float32>(inputs.seaportTrips);
    out.newConnectionUtilA += seaportFloat * tuning.coeffC6;
    out.newPassengerConnectionSupply += seaportFloat * tuning.coeffC9;

    // Step 4: airport trips @ 0x100228be8-0x100228c41
    const Float32 airportFloat = static_cast<Float32>(inputs.airportTrips);
    out.newPassengerConnectionSupply += airportFloat * tuning.coeffC7;
    out.newConnectionUtilA += airportFloat * tuning.coeffCA;

    // Step 5: deltas applied to 0x4800, 0x4900, and 0x3800 @ 0x100228c48-0x100228ccf
    out.deltaSupply4800And4900 =
        out.newPassengerConnectionSupply - inputs.prevPassengerConnectionSupply;
    out.deltaSupply3800 =
        out.newConnectionUtilA - inputs.prevConnectionUtilA;
    return out;
}

Uint32 ComputeAverageCitySalary(
    const DemandSimulatorTuning& tuning,
    const std::array<Float32, 12>& eqBracketSupplies) noexcept {
    // Exact DoIdle step 2 @ 0x1002267be-0x10022682c:
    Uint64 totalPop = 0;
    Uint64 weightedSum = 0;
    for (std::size_t i = 0; i < 12u; ++i) {
        const Sint64 pop = static_cast<Sint64>(
            static_cast<Sint32>(eqBracketSupplies[i]));
        totalPop += static_cast<Uint64>(pop);
        const Sint64 prod = static_cast<Sint64>(
            static_cast<Sint32>(pop) *
            static_cast<Sint32>(tuning.salaryWeights[i]));
        weightedSum += static_cast<Uint64>(prod);
    }
    if (totalPop == 0u) {
        return 0u;
    }
    return static_cast<Uint32>(weightedSum / totalPop);
}

bool ExtrapolateResidentialCensusValues(
    Uint32 subType,
    Float32 exportedUnits,
    Sint32 neighborBasePop,
    Sint32 neighborActivePop,
    Float32 neighborRetention,
    const std::array<Sint32, 4>& neighborEqCounts,
    ExtrapolatedResidentialCensus& outResult) noexcept {
    // Exact bitmask 0x100010001 check on (subType - 0x1010) @ 0x10022a303-0x10022a321:
    if (subType != 0x1010u && subType != 0x1020u && subType != 0x1030u) {
        return false;
    }

    const Float32 activeRatio =
        (neighborBasePop > 0)
            ? (static_cast<Float32>(neighborActivePop) /
               static_cast<Float32>(neighborBasePop))
            : 0.80000001f;
    const Float32 scaledTotal = activeRatio * neighborRetention * exportedUnits;

    Sint32 eq0 = neighborEqCounts[0];
    const Sint32 eq1 = neighborEqCounts[1];
    const Sint32 eq2 = neighborEqCounts[2];
    const Sint32 eq3 = neighborEqCounts[3];
    Sint32 sumEq = eq0 + eq1 + eq2 + eq3;
    if (sumEq == 0) {
        sumEq = 1;
        eq0 += 1;
    }

    const Float32 denom = static_cast<Float32>(sumEq);
    const std::array<Sint32, 4> counts{eq0, eq1, eq2, eq3};
    for (std::size_t i = 0; i < 4u; ++i) {
        outResult.eqKeys[i] = subType + 0x1101u + static_cast<Uint32>(i);
        outResult.regionalDemandKeys[i] =
            subType + 0x11101u + static_cast<Uint32>(i);
        outResult.extrapolatedValues[i] =
            (static_cast<Float32>(counts[i]) * scaledTotal) / denom;
    }
    return true;
}

bool ExtrapolateBusinessCensusValues(
    Uint32 subType,
    Float32 exportedUnits,
    Sint32 neighborBaseJobs,
    Sint32 neighborActiveJobs,
    const std::array<Float32, 3>& jobsPerUnitByWealth,
    ExtrapolatedBusinessCensus& outResult) noexcept {
    // Exact subType validation @ 0x10022a5af-0x10022a612:
    const bool valid =
        (subType == 0x3110u || subType == 0x3120u || subType == 0x3130u ||
         subType == 0x3320u || subType == 0x3330u ||
         subType == 0x4100u || subType == 0x4200u ||
         subType == 0x4300u || subType == 0x4400u);
    if (!valid) {
        return false;
    }

    const Float32 activeRatio =
        (neighborBaseJobs > 0)
            ? (static_cast<Float32>(neighborActiveJobs) /
               static_cast<Float32>(neighborBaseJobs))
            : 0.80000001f;
    outResult.regionalSupplyKey = subType + 0x10001u;
    outResult.extrapolatedActiveJobs = activeRatio * exportedUnits;
    for (std::size_t i = 0; i < 3u; ++i) {
        outResult.workforceDemandByWealth[i] =
            jobsPerUnitByWealth[i] * outResult.extrapolatedActiveJobs;
    }
    return true;
}

bool UpdateDeveloperCapacityGrid(
    const LotCapacityUpdateInput& input,
    std::span<Uint16> inOutCapacityGrid,
    Uint32& inOutTotalCapacity) noexcept {
    // Exact cSC4GrowthDeveloper::UpdatePopulationCount @ 0x100158140-0x10015837a:
    if (input.developerPurpose != input.buildingPurpose) {
        return false;
    }
    const Sint32 area =
        static_cast<Sint32>(input.sizeX) * static_cast<Sint32>(input.sizeZ);
    if (area <= 0 || input.gridTractsX <= 0 || input.gridTractsZ <= 0) {
        return false;
    }

    const Sint32 signedCap = input.addOrRemove
        ? static_cast<Sint32>(input.lotCapacity)
        : -static_cast<Sint32>(input.lotCapacity);
    const Sint32 perCellQuotient = signedCap / area;
    const Sint32 remainder = signedCap - perCellQuotient * area;

    const Sint32 maxX = std::max(input.minX, input.maxX);
    const Sint32 maxZ = std::max(input.minZ, input.maxZ);

    const auto addToTract = [&](Sint32 cellX, Sint32 cellZ, Sint32 delta) {
        const Sint32 tx = cellX >> input.tractShift;
        const Sint32 tz = cellZ >> input.tractShift;
        if (tx < 0 || tx >= input.gridTractsX ||
            tz < 0 || tz >= input.gridTractsZ) {
            return;
        }
        const std::size_t idx =
            static_cast<std::size_t>(tx) *
                static_cast<std::size_t>(input.gridTractsZ) +
            static_cast<std::size_t>(tz);
        if (idx >= inOutCapacityGrid.size()) {
            return;
        }
        const Sint32 updated =
            static_cast<Sint32>(inOutCapacityGrid[idx]) + delta;
        inOutCapacityGrid[idx] =
            (updated < 0) ? 0u : static_cast<Uint16>(updated);
    };

    for (Sint32 z = input.minZ; z <= maxZ; ++z) {
        for (Sint32 x = input.minX; x <= maxX; ++x) {
            addToTract(x, z, perCellQuotient);
        }
    }
    addToTract(input.locationX, input.locationZ, remainder);

    inOutTotalCapacity = static_cast<Uint32>(
        static_cast<Sint32>(inOutTotalCapacity) + signedCap);
    return true;
}

bool SelectNextUnmarkedGrowthTract(
    std::span<const GrowthDeveloperItem> growthList,
    std::vector<std::pair<Sint32, Sint32>>& inOutMarkedTracts,
    GrowthDeveloperItem& outSelected) noexcept {
    // Exact cSC4BuildingDevelopmentSimulator::Grow @ 0x10014fe55-0x10014feef:
    if (growthList.empty()) {
        return false;
    }

    const auto isUnmarked = [&](const GrowthDeveloperItem& item) noexcept {
        for (const auto& [mx, mz] : inOutMarkedTracts) {
            if (mx == item.tractX && mz == item.tractZ) {
                return false;
            }
        }
        return true;
    };

    const GrowthDeveloperItem* found = nullptr;
    for (const auto& item : growthList) {
        if (isUnmarked(item)) {
            found = &item;
            break;
        }
    }

    if (!found) {
        // All tracts in mGrowthList were marked -> clear mGrowthMarks[subType]
        // and restart at mGrowthList.begin() (@ 0x10014fe99-0x10014fed7).
        inOutMarkedTracts.clear();
        found = &growthList.front();
    }

    outSelected = *found;
    inOutMarkedTracts.emplace_back(outSelected.tractX, outSelected.tractZ);
    return true;
}

bool cSC4LotConfiguration::LoadFromExemplar(
    const cGZExemplar& exemplar,
    Uint32 instanceId,
    Uint8 detailLevel) {
    id = instanceId;
    name = exemplar.GetString(kPropExemplarName, "");

    version = 0u;
    auto verIt = exemplar.properties.find(0x88edc789u);
    if (verIt != exemplar.properties.end() && !verIt->second.intValues.empty()) {
        version = static_cast<Uint8>(verIt->second.intValues[0]);
    }
    if (version == 0u) {
        return false;
    }

    sizeX = 0u;
    sizeZ = 0u;
    auto sizeIt = exemplar.properties.find(0x88edc790u);
    if (sizeIt != exemplar.properties.end() &&
        sizeIt->second.intValues.size() >= 2u) {
        sizeX = static_cast<Uint8>(sizeIt->second.intValues[0]);
        sizeZ = static_cast<Uint8>(sizeIt->second.intValues[1]);
    }

    familyIds.clear();
    auto famIt = exemplar.properties.find(0x88edc791u);
    if (famIt != exemplar.properties.end()) {
        familyIds.reserve(famIt->second.intValues.size());
        for (const Sint64 v : famIt->second.intValues) {
            familyIds.push_back(static_cast<Uint32>(v));
        }
    }

    maxSlopeBeforeFoundation = exemplar.GetFloat(0x88edc792u, 90.0f);
    maxSlope = exemplar.GetFloat(0xe99b068cu, 90.0f);
    minSlope = exemplar.GetFloat(0x699b08a4u, 0.0f);

    growthStage = 1u;
    auto gsIt = exemplar.properties.find(0x27812837u);
    if (gsIt != exemplar.properties.end() && !gsIt->second.intValues.empty()) {
        growthStage = static_cast<Uint8>(gsIt->second.intValues[0]);
    }

    isCompatibleWithWater = false;
    auto waterIt = exemplar.properties.find(0x89136440u);
    if (waterIt != exemplar.properties.end() &&
        !waterIt->second.intValues.empty()) {
        isCompatibleWithWater = (waterIt->second.intValues[0] != 0);
    }

    zoneTypeMask = 0u;
    auto ztIt = exemplar.properties.find(0x88edc793u);
    if (ztIt != exemplar.properties.end()) {
        for (const Sint64 v : ztIt->second.intValues) {
            const Uint32 bit = static_cast<Uint8>(v);
            if (bit < 16u) {
                zoneTypeMask = static_cast<Uint16>(zoneTypeMask | (1u << bit));
            }
        }
    }

    wealthTypeMask = 0u;
    auto wtIt = exemplar.properties.find(0x88edc795u);
    if (wtIt != exemplar.properties.end()) {
        for (const Sint64 v : wtIt->second.intValues) {
            const Uint32 bit = static_cast<Uint8>(v);
            if (bit < 4u) {
                wealthTypeMask = static_cast<Uint8>(wealthTypeMask | (1u << bit));
            }
        }
    }

    purposeTypeMask = 0u;
    auto ptIt = exemplar.properties.find(0x88edc796u);
    if (ptIt != exemplar.properties.end()) {
        for (const Sint64 v : ptIt->second.intValues) {
            const Uint32 bit = static_cast<Uint8>(v);
            if (bit < 9u) {
                purposeTypeMask =
                    static_cast<Uint16>(purposeTypeMask | (1u << bit));
            }
        }
    }

    doConstruction = false;
    auto dcIt = exemplar.properties.find(0xe99b068du);
    if (dcIt != exemplar.properties.end() && !dcIt->second.intValues.empty()) {
        doConstruction = (dcIt->second.intValues[0] != 0);
    }

    retainingWallIds.clear();
    auto rwIt = exemplar.properties.find(0x88edc798u);
    if (rwIt != exemplar.properties.end()) {
        for (const Sint64 v : rwIt->second.intValues) {
            retainingWallIds.push_back(static_cast<Uint32>(v));
        }
    }

    fenceIds.clear();
    fenceSegments.clear();
    auto fnIt = exemplar.properties.find(0x88edc797u);
    if (fnIt != exemplar.properties.end() &&
        fnIt->second.intValues.size() >= 2u) {
        const auto& fvec = fnIt->second.intValues;
        const Uint32 countA = static_cast<Uint32>(fvec[0]);
        std::size_t idx = 2u;
        if (countA > 0u) {
            for (Uint32 i = 0; i < countA && idx < fvec.size(); ++i, ++idx) {
                fenceIds.push_back(static_cast<Uint32>(fvec[idx]));
            }
        }
        if (idx < fvec.size()) {
            const Uint32 countB = static_cast<Uint32>(fvec[idx++]);
            for (Uint32 i = 0;
                 i < countB && idx + 3u < fvec.size();
                 ++i, idx += 4u) {
                fenceSegments.push_back({
                    static_cast<Sint8>(fvec[idx]),
                    static_cast<Sint8>(fvec[idx + 1u]),
                    static_cast<Sint8>(fvec[idx + 2u]),
                    static_cast<Sint8>(fvec[idx + 3u])
                });
            }
        }
    }

    roadRequirementFlags = 0x08u;
    auto rrIt = exemplar.properties.find(0x4a4a88f0u);
    if (rrIt != exemplar.properties.end() && !rrIt->second.intValues.empty()) {
        roadRequirementFlags = static_cast<Uint8>(rrIt->second.intValues[0]);
    }
    // Exact check at 0x10040373e-0x100403799: south edge bit (0x08) is forced on.
    roadRequirementFlags = static_cast<Uint8>(roadRequirementFlags | 0x08u);

    // ConvertPropertiesToObjectArray @ 0x100403a60:
    objects.clear();
    const Uint32 detailThreshold = static_cast<Uint32>(detailLevel) << 4u;
    Uint32 syntheticGuid = 1u;
    constexpr double kFixed16Scale = 1.52587890625e-05; // 1.0 / 65536.0
    for (Uint32 propId = 0x88edc900u; propId < 0x88edce00u; ++propId) {
        auto objIt = exemplar.properties.find(propId);
        if (objIt == exemplar.properties.end() ||
            objIt->second.intValues.size() < 10u) {
            continue;
        }
        const auto& v = objIt->second.intValues;
        cSC4LotConfigurationObject obj{};
        obj.type = static_cast<Uint8>(v[0]);
        obj.lodAndFlags = static_cast<Uint8>(v[1]);
        const Uint32 rawOrient = static_cast<Uint32>(v[2]);
        obj.orientation = static_cast<Uint8>(rawOrient);
        obj.mirrorFlag = static_cast<Uint8>(rawOrient >> 31u);

        obj.posX = static_cast<Float32>(
            static_cast<double>(static_cast<Sint32>(v[3])) * kFixed16Scale);
        obj.posY = static_cast<Float32>(
            static_cast<double>(static_cast<Sint32>(v[4])) * kFixed16Scale);
        obj.posZ = static_cast<Float32>(
            static_cast<double>(static_cast<Sint32>(v[5])) * kFixed16Scale);
        obj.maxX = static_cast<Float32>(
            static_cast<double>(static_cast<Sint32>(v[6])) * kFixed16Scale);
        obj.maxZ = static_cast<Float32>(
            static_cast<double>(static_cast<Sint32>(v[7])) * kFixed16Scale);
        obj.minX = static_cast<Float32>(
            static_cast<double>(static_cast<Sint32>(v[8])) * kFixed16Scale);
        obj.minZ = static_cast<Float32>(
            static_cast<double>(static_cast<Sint32>(v[9])) * kFixed16Scale);
        obj.usageFlags =
            (v.size() > 10u) ? static_cast<Uint32>(v[10]) : 0u;

        std::size_t descStart = 11u;
        if (version > 1u && v.size() > 11u) {
            obj.objectId = static_cast<Uint32>(v[11]);
            descStart = 12u;
        } else {
            obj.objectId = syntheticGuid++;
        }

        if (v.size() > descStart) {
            obj.descriptorIds.reserve(v.size() - descStart);
            for (std::size_t i = descStart; i < v.size(); ++i) {
                obj.descriptorIds.push_back(static_cast<Uint32>(v[i]));
            }
        }

        if (obj.type != 0u &&
            detailThreshold <=
                (static_cast<Uint32>(obj.lodAndFlags) & 0x30u)) {
            continue;
        }
        objects.push_back(std::move(obj));
    }

    CorrectObjectPositions(
        objects,
        static_cast<Float32>(static_cast<Uint32>(sizeX) << 4u),
        static_cast<Float32>(static_cast<Uint32>(sizeZ) << 4u));

    // Optional foundation property 0x88fcd877 @ 0x10040380d:
    auto fndIt = exemplar.properties.find(0x88fcd877u);
    if (fndIt != exemplar.properties.end() &&
        !fndIt->second.intValues.empty()) {
        cSC4LotConfigurationObject fndObj{};
        fndObj.type = static_cast<Uint8>(LotConfigObjectType::Foundation);
        fndObj.posX = 0.0f;
        fndObj.posY = 0.0f;
        fndObj.posZ = 0.0f;
        fndObj.minX = 0.0f;
        fndObj.minZ = 0.6f;
        fndObj.maxX = 0.4f;
        fndObj.maxZ = 0.4f;
        fndObj.usageFlags = static_cast<Uint32>(fndIt->second.intValues[0]);
        objects.push_back(std::move(fndObj));
    }

    return true;
}

bool cSC4LotConfiguration::CorrectObjectPositions(
    std::vector<cSC4LotConfigurationObject>& inOutObjects,
    Float32 lotWidthX,
    Float32 lotDepthZ) noexcept {
    // Exact cSC4LotConfiguration::CorrectObjectPositions @ 0x100403d8e:
    if (inOutObjects.empty()) {
        return false;
    }
    constexpr Float32 kMargin = 0.040000003f;
    const Float32 minAllowedX = kMargin;
    const Float32 maxAllowedX = lotWidthX - kMargin;
    const Float32 minAllowedZ = kMargin;
    const Float32 maxAllowedZ = lotDepthZ - kMargin;

    bool anyCorrected = false;
    for (auto& obj : inOutObjects) {
        bool correctedX = false;
        Float32 targetX = obj.posX;
        if (minAllowedX > obj.posX) {
            targetX = minAllowedX;
            correctedX = true;
        } else if (obj.posX > maxAllowedX) {
            targetX = maxAllowedX;
            correctedX = true;
        }
        if (correctedX) {
            const Float32 deltaX = targetX - obj.posX;
            obj.posX += deltaX;
            obj.maxX += deltaX;
            obj.minX += deltaX;
            anyCorrected = true;
        }

        bool correctedZ = false;
        Float32 targetZ = obj.posZ;
        if (minAllowedZ > obj.posZ) {
            targetZ = minAllowedZ;
            correctedZ = true;
        } else if (obj.posZ > maxAllowedZ) {
            targetZ = maxAllowedZ;
            correctedZ = true;
        }
        if (correctedZ) {
            const Float32 deltaZ = targetZ - obj.posZ;
            obj.posZ += deltaZ;
            obj.maxZ += deltaZ;
            obj.minZ += deltaZ;
            anyCorrected = true;
        }
    }
    return anyCorrected;
}

bool cSC4LotConfiguration::BelongsToFamily(Uint32 familyId) const noexcept {
    for (const Uint32 fid : familyIds) {
        if (fid == familyId) {
            return true;
        }
    }
    return false;
}

const cSC4LotConfigurationObject*
cSC4LotConfiguration::GetPrimaryBuildingObject() const noexcept {
    for (const auto& obj : objects) {
        if (obj.type == static_cast<Uint8>(LotConfigObjectType::Building)) {
            return &obj;
        }
    }
    return nullptr;
}

const cSC4LotConfigurationObject*
cSC4LotConfiguration::GetPrimaryBaseTextureObject() const noexcept {
    for (const auto& obj : objects) {
        if (obj.type == static_cast<Uint8>(LotConfigObjectType::BaseTexture)) {
            return &obj;
        }
    }
    return nullptr;
}

const cSC4LotConfigurationObject*
cSC4LotConfiguration::GetPrimaryFoundationObject() const noexcept {
    for (const auto& obj : objects) {
        if (obj.type == static_cast<Uint8>(LotConfigObjectType::Foundation)) {
            return &obj;
        }
    }
    return nullptr;
}

bool LotConfigurationManagerSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version != 1u) {
        return false;
    }
    return ReadSerializableRef(stream, buildingDevelopmentSim) &&
           ReadSerializableRef(stream, floraSim) &&
           ReadSerializableRef(stream, propManager);
}

bool LotConfigurationManagerSaveRecord::Write(cIGZOStream& stream) const {
    stream << static_cast<Uint16>(1u);
    WriteSerializableRef(stream, buildingDevelopmentSim);
    WriteSerializableRef(stream, floraSim);
    WriteSerializableRef(stream, propManager);
    return true;
}

bool LotConfigurationManagerSaveRecord::ReadFromSC4Record(
    std::span<const Uint8> recordBytes) noexcept {
    if (recordBytes.size() < 14u) {
        return false;
    }
    Uint32 recSize = 0;
    std::memcpy(&recSize, recordBytes.data(), sizeof(Uint32));
    if (recSize < 14u || recSize > recordBytes.size()) {
        return false;
    }
    std::memcpy(&memAddr, recordBytes.data() + 8u, sizeof(Uint32));
    cIGZIStream stream(recordBytes.data() + 12u, recSize - 12u);
    return Read(stream);
}

bool LotDeveloperTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    bool loaded = false;

    auto p400 = exemplar.properties.find(0x47e2c400u);
    if (p400 != exemplar.properties.end() && !p400->second.intValues.empty()) {
        constructionResourcePerTick =
            static_cast<Uint16>(p400->second.intValues[0]);
        loaded = true;
    }

    stageTimeFractions = {0.2f, 0.4f, 0.6f};
    auto p401 = exemplar.properties.find(0x47e2c401u);
    if (p401 != exemplar.properties.end() &&
        p401->second.floatValues.size() >= 3u) {
        stageTimeFractions[0] = p401->second.floatValues[0];
        stageTimeFractions[1] = p401->second.floatValues[1];
        stageTimeFractions[2] = p401->second.floatValues[2];
        loaded = true;
    }

    const auto readF32 = [&](Uint32 propId, Float32& outVal) {
        auto it = exemplar.properties.find(propId);
        if (it != exemplar.properties.end() && !it->second.floatValues.empty()) {
            outVal = it->second.floatValues[0];
            loaded = true;
        }
    };
    const auto readU32 = [&](Uint32 propId, Uint32& outVal) {
        auto it = exemplar.properties.find(propId);
        if (it != exemplar.properties.end() && !it->second.intValues.empty()) {
            outVal = static_cast<Uint32>(it->second.intValues[0]);
            loaded = true;
        }
    };
    const auto readBool = [&](Uint32 propId, bool& outVal) {
        auto it = exemplar.properties.find(propId);
        if (it != exemplar.properties.end() && !it->second.intValues.empty()) {
            outVal = (it->second.intValues[0] != 0);
            loaded = true;
        }
    };

    readF32(0xe900ea24u, minFoundationSlope);
    readF32(0xe900ea25u, foundationSlopeThreshold);
    readU32(0xe900ea26u, defaultFoundationId);
    readU32(0x6916516bu, defaultRetainingWallId);
    readBool(0xe900ea27u, attemptSlopeLevelling);
    readBool(0xe900ea28u, attemptFlatLevelling);
    readF32(0xe900ea29u, maxFlatSlope);
    readF32(0xe900ea2au, maxLevellingDelta);
    readU32(0xe900ea2bu, propUpdateBatchSize);
    readU32(0xe900ea2cu, buildingUpdateBatchSize);

    auto curveIt = exemplar.properties.find(0x2a2d7824u);
    if (curveIt != exemplar.properties.end() &&
        floraElevationCurve.SetResponseCurveData(curveIt->second)) {
        loaded = true;
    }

    return loaded;
}

BoundaryLotTextureRule LookupBoundaryLotTexture(
    const std::array<Sint32, 4>& boundaryTypes) noexcept {
    // Exact cSC4LotDeveloper::InitBoundaryRules @ 0x10013f67e and
    // GetBoundaryLotTexture @ 0x10014658c:
    const Sint32 key =
        (((((boundaryTypes[0] << 4) + boundaryTypes[1]) << 4) +
          boundaryTypes[2]) << 4) +
        boundaryTypes[3];

    switch (key) {
        case 0x0000: return {0x3000u, 0u, false};
        case 0x1000: return {0x2000u, 0u, false};
        case 0x0100: return {0x2000u, 1u, false};
        case 0x0010: return {0x2000u, 2u, false};
        case 0x0001: return {0x2000u, 3u, false};
        case 0x2000: return {0x2600u, 0u, false};
        case 0x0200: return {0x2600u, 1u, false};
        case 0x0020: return {0x2600u, 2u, false};
        case 0x0002: return {0x2600u, 3u, false};
        case 0x1100: return {0x2100u, 1u, false};
        case 0x0110: return {0x2100u, 2u, false};
        case 0x0011: return {0x2100u, 3u, false};
        case 0x1001: return {0x2100u, 0u, false};
        case 0x1200: return {0x2400u, 1u, false};
        case 0x0120: return {0x2400u, 2u, false};
        case 0x0012: return {0x2400u, 3u, false};
        case 0x2001: return {0x2400u, 0u, false};
        case 0x2100: return {0x2400u, 0u, true};
        case 0x0210: return {0x2400u, 1u, true};
        case 0x0021: return {0x2400u, 2u, true};
        case 0x1002: return {0x2400u, 3u, true};
        case 0x1010: return {0x2500u, 0u, false};
        case 0x0101: return {0x2500u, 1u, false};
        case 0x1020: return {0x2900u, 2u, false};
        case 0x0102: return {0x2900u, 3u, false};
        case 0x2010: return {0x2900u, 0u, false};
        case 0x0201: return {0x2900u, 1u, false};
        case 0x1110: return {0x2200u, 1u, false};
        case 0x0111: return {0x2200u, 2u, false};
        case 0x1011: return {0x2200u, 3u, false};
        case 0x1101: return {0x2200u, 0u, false};
        case 0x1120: return {0x2700u, 2u, false};
        case 0x0112: return {0x2700u, 3u, false};
        case 0x2011: return {0x2700u, 0u, false};
        case 0x1201: return {0x2700u, 1u, false};
        case 0x1210: return {0x2800u, 1u, false};
        case 0x0121: return {0x2800u, 2u, false};
        case 0x1012: return {0x2800u, 3u, false};
        case 0x2101: return {0x2800u, 0u, false};
        case 0x2110: return {0x2700u, 0u, true};
        case 0x0211: return {0x2700u, 1u, true};
        case 0x1021: return {0x2700u, 2u, true};
        case 0x1102: return {0x2700u, 3u, true};
        case 0x1112: return {0x2300u, 3u, false};
        case 0x2111: return {0x2300u, 0u, false};
        case 0x1211: return {0x2300u, 1u, false};
        case 0x1121: return {0x2300u, 2u, false};
        default:     return {0x3000u, 0u, false};
    }
}

LotObjectWorldPlacement ComputeLotObjectWorldPlacement(
    Sint32 lotMinCellX,
    Sint32 lotMinCellZ,
    Sint32 lotMaxCellX,
    Sint32 lotMaxCellZ,
    Sint32 lotFacing,
    Float32 baseAltitudeY,
    const cSC4LotConfigurationObject& obj) noexcept {
    // Exact cSC4LotDeveloper::GetPositionForLotConfigurationObject @ 0x1001442e0:
    LotObjectWorldPlacement out{};
    const Float32 worldMinX = static_cast<Float32>(lotMinCellX) * 16.0f;
    const Float32 worldMinZ = static_cast<Float32>(lotMinCellZ) * 16.0f;
    const Float32 worldMaxX = static_cast<Float32>(lotMaxCellX + 1) * 16.0f;
    const Float32 worldMaxZ = static_cast<Float32>(lotMaxCellZ + 1) * 16.0f;

    Float32 localX = obj.posX;
    Float32 localZ = obj.posZ;
    if (obj.type == static_cast<Uint8>(LotConfigObjectType::BaseTexture)) {
        const Sint32 cellX = static_cast<Sint32>(localX * 0.0625f);
        const Sint32 cellZ = static_cast<Sint32>(localZ * 0.0625f);
        localX = (static_cast<Float32>(cellX) + 0.5f) * 16.0f;
        localZ = (static_cast<Float32>(cellZ) + 0.5f) * 16.0f;
    }

    switch (lotFacing) {
        case 0:
            out.worldX = worldMaxX - localX;
            out.worldZ = worldMaxZ - localZ;
            out.boxMinX = worldMaxX - obj.minX;
            out.boxMinZ = worldMaxZ - obj.minZ;
            out.boxMaxX = worldMaxX - obj.maxX;
            out.boxMaxZ = worldMaxZ - obj.maxZ;
            break;
        case 1:
            out.worldX = worldMinX + localZ;
            out.worldZ = worldMaxZ - localX;
            out.boxMinX = worldMinX + obj.maxZ;
            out.boxMinZ = worldMaxZ - obj.minX;
            out.boxMaxX = worldMinX + obj.minZ;
            out.boxMaxZ = worldMaxZ - obj.maxX;
            break;
        case 2:
            out.worldX = worldMinX + localX;
            out.worldZ = worldMinZ + localZ;
            out.boxMinX = worldMinX + obj.maxX;
            out.boxMinZ = worldMinZ + obj.maxZ;
            out.boxMaxX = worldMinX + obj.minX;
            out.boxMaxZ = worldMinZ + obj.minZ;
            break;
        case 3:
            out.worldX = worldMaxX - localZ;
            out.worldZ = worldMinZ + localX;
            out.boxMinX = worldMaxX - obj.minZ;
            out.boxMinZ = worldMinZ + obj.maxX;
            out.boxMaxX = worldMaxX - obj.maxZ;
            out.boxMaxZ = worldMinZ + obj.minX;
            break;
        default:
            out.worldX = 0.0f;
            out.worldZ = 0.0f;
            out.boxMinX = 0.0f;
            out.boxMinZ = 1.0f;
            out.boxMaxX = 0.0f;
            out.boxMaxZ = 1.0f;
            break;
    }

    out.worldY = baseAltitudeY + obj.posY;
    const Sint32 sumOrient =
        lotFacing + static_cast<Sint32>(obj.orientation);
    out.orientation = ((sumOrient % 4) + 4) % 4;
    return out;
}

bool LotDeveloperSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version != 1u) {
        return false;
    }
    stream >> field78 >> field7C >> field80;
    Uint32 queueSize = 0;
    stream >> queueSize;
    if (!stream.Good() || queueSize > 65536u) {
        return false;
    }
    buildingQueue.clear();
    buildingQueue.reserve(queueSize);
    for (Uint32 i = 0; i < queueSize; ++i) {
        ObjectRef ref{};
        if (!ReadSerializableRef(stream, ref)) {
            return false;
        }
        buildingQueue.push_back(ref);
    }
    Uint8 forceEmpty = 0;
    stream >> currentQueueIndex >> nightChangeMode >> forceEmpty;
    if (!stream.Good()) {
        return false;
    }
    forceEmptyLotBaseTextures = (forceEmpty != 0u);
    return true;
}

bool LotDeveloperSaveRecord::Write(cIGZOStream& stream) const {
    stream << static_cast<Uint16>(1u);
    stream << field78 << field7C << field80;
    stream << static_cast<Uint32>(buildingQueue.size());
    for (const auto& ref : buildingQueue) {
        WriteSerializableRef(stream, ref);
    }
    stream << currentQueueIndex
           << nightChangeMode
           << static_cast<Uint8>(forceEmptyLotBaseTextures ? 1u : 0u);
    return true;
}

bool LotDeveloperSaveRecord::ReadFromSC4Record(
    std::span<const Uint8> recordBytes) noexcept {
    if (recordBytes.size() < 14u) {
        return false;
    }
    Uint32 recSize = 0;
    std::memcpy(&recSize, recordBytes.data(), sizeof(Uint32));
    if (recSize < 14u || recSize > recordBytes.size()) {
        return false;
    }
    std::memcpy(&memAddr, recordBytes.data() + 8u, sizeof(Uint32));
    cIGZIStream stream(recordBytes.data() + 12u, recSize - 12u);
    return Read(stream);
}

namespace {

// Exact helpers shared by the network RUL/RUL2 text parsers.
inline std::string TrimAscii(std::string_view text) {
    std::size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }
    std::size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return std::string(text.substr(begin, end - begin));
}

inline std::string_view TrimView(std::string_view text) noexcept {
    std::size_t begin = 0;
    while (begin < text.size() && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
        ++begin;
    }
    std::size_t end = text.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }
    return text.substr(begin, end - begin);
}

inline bool IEquals(std::string_view a, std::string_view b) noexcept {
    if (a.size() != b.size()) {
        return false;
    }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) !=
            std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

inline std::vector<std::string> SplitComma(std::string_view text) {
    std::vector<std::string> tokens;
    std::size_t start = 0;
    while (start <= text.size()) {
        const std::size_t comma = text.find(',', start);
        if (comma == std::string_view::npos) {
            tokens.push_back(TrimAscii(text.substr(start)));
            break;
        }
        tokens.push_back(TrimAscii(text.substr(start, comma - start)));
        start = comma + 1;
    }
    return tokens;
}

inline Sint32 ParseDecimalSint32(std::string_view token) noexcept {
    const std::string trimmed{TrimView(token)};
    if (trimmed.empty()) {
        return 0;
    }
    char* end = nullptr;
    const long value = std::strtol(trimmed.c_str(), &end, 10);
    return static_cast<Sint32>(value);
}

inline Uint32 ParseHexUint32(std::string_view token) noexcept {
    const std::string trimmed{TrimView(token)};
    if (trimmed.empty()) {
        return 0;
    }
    char* end = nullptr;
    const unsigned long value = std::strtoul(trimmed.c_str(), &end, 16);
    return static_cast<Uint32>(value);
}

inline bool ParseHexUint64(std::string_view token, Uint64& outValue) noexcept {
    const std::string trimmed{TrimView(token)};
    if (trimmed.empty()) {
        outValue = 0;
        return false;
    }
    char* end = nullptr;
    outValue = std::strtoull(trimmed.c_str(), &end, 16);
    return end != trimmed.c_str();
}


} // namespace

const char* GetEnglishNetworkName(SC4NetworkType type) noexcept {
    switch (type) {
        case SC4NetworkType::Road: return "Road";
        case SC4NetworkType::Rail: return "Rail";
        case SC4NetworkType::Highway: return "Highway";
        case SC4NetworkType::Street: return "Street";
        case SC4NetworkType::Pipe: return "Pipe";
        case SC4NetworkType::PowerLine: return "PowerLine";
        case SC4NetworkType::Avenue: return "Avenue";
        case SC4NetworkType::Subway: return "Subway";
        case SC4NetworkType::LightRail: return "LightRail";
        case SC4NetworkType::Monorail: return "Monorail";
        case SC4NetworkType::OneWayRoad: return "OneWayRoad";
        case SC4NetworkType::DirtRoad: return "DirtRoad";
        case SC4NetworkType::GroundHighway: return "GroundHighway";
        default: return "";
    }
}

bool ParseNetworkNameToId(std::string_view name, SC4NetworkType& outType) noexcept {
    const std::string_view trimmed = TrimView(name);
    for (Uint32 i = 0; i < 13u; ++i) {
        const auto candidate = static_cast<SC4NetworkType>(i);
        if (IEquals(trimmed, GetEnglishNetworkName(candidate))) {
            outType = candidate;
            return true;
        }
    }
    return false;
}

void ConvertMetaToRelativePos(Uint8 metaPos, Sint8& outRelX, Sint8& outRelZ) noexcept {
    // Exact sRelativeXForMetaPos / sRelativeZForMetaPos @ 0x100a1a320 / 0x100a1a340.
    constexpr std::array<Sint8, 25> kRelativeXForMetaPos{
        0, -1, -1, 0, 1, 1, 1, 0, -1, -2, -2, -2, -1,
        0, 1, 2, 2, 2, 2, 2, 1, 0, -1, -2, -2
    };
    constexpr std::array<Sint8, 25> kRelativeZForMetaPos{
        0, 0, -1, -1, -1, 0, 1, 1, 1, 0, -1, -2, -2,
        -2, -2, -2, -1, 0, 1, 2, 2, 2, 2, 2, 1
    };
    if (metaPos < 25u) {
        outRelX = kRelativeXForMetaPos[metaPos];
        outRelZ = kRelativeZForMetaPos[metaPos];
    } else {
        outRelX = 0;
        outRelZ = 0;
    }
}

Uint8 ConvertRelativePosToMeta(Sint8 relX, Sint8 relZ) noexcept {
    // Exact sMetaForRelativePos @ 0x100a1a360: 5x5 grid indexed [relZ+2][relX+2].
    constexpr std::array<Uint8, 25> kMetaForRelativePos{
        11, 12, 13, 14, 15,
        10, 2, 3, 4, 16,
        9, 1, 0, 5, 17,
        24, 8, 7, 6, 18,
        23, 22, 21, 20, 19
    };
    const Sint32 ix = static_cast<Sint32>(relX) + 2;
    const Sint32 iz = static_cast<Sint32>(relZ) + 2;
    if (ix < 0 || ix >= 5 || iz < 0 || iz >= 5) {
        return 0;
    }
    return kMetaForRelativePos[static_cast<std::size_t>(iz * 5 + ix)];
}

Uint8 RotateMetaPosition(Uint8 metaPos, Sint32 rotation) noexcept {
    // Exact rotation branch in cSC4TileInfo::GetRotationalEquivalent @ 0x1001aeb82.
    Sint32 result = metaPos;
    if (metaPos != 0u) {
        if (metaPos <= 8u) {
            const Sint32 m = metaPos - 1 + rotation * 2;
            result = ((m % 8 + 8) % 8) + 1;
        } else if (metaPos <= 24u) {
            const Sint32 m = metaPos - 9 + rotation * 4;
            result = ((m % 16 + 16) % 16) + 9;
        }
    }
    return static_cast<Uint8>(result);
}

Uint8 MirrorMetaPosition(Uint8 metaPos) noexcept {
    // Exact mirror branch in GetMirrorImageEquivalent paths: relX = -relX.
    Sint8 relX = 0;
    Sint8 relZ = 0;
    ConvertMetaToRelativePos(metaPos, relX, relZ);
    return ConvertRelativePosToMeta(static_cast<Sint8>(-relX), relZ);
}

Uint8 cSC4TileNeighborhoodCompact::GetEdgeCondition(Sint32 edge) const noexcept {
    // Exact GetEdgeCondition @ 0x1001aee24.
    return static_cast<Uint8>((packed >> (edge * 8)) & 0xffu);
}

void cSC4TileNeighborhoodCompact::SetEdgeCondition(Sint32 edge, Uint8 value) noexcept {
    // Exact SetEdgeCondition @ 0x1001aeeee.
    const Uint32 shift = static_cast<Uint32>(edge * 8);
    packed = (packed & ~(0xffu << shift)) | (static_cast<Uint32>(value) << shift);
}

void cSC4TileNeighborhoodCompact::SetEdgeTypeMinor(Sint32 edge, Uint8 minorType) noexcept {
    // Exact SetEdgeTypeMinor @ 0x1001af0d0: newByte = (oldByte & 0x07) | (minor * 8).
    const Uint32 shift = static_cast<Uint32>(edge * 8);
    const Uint32 oldByte = (packed >> shift) & 0xffu;
    const Uint32 newByte = (oldByte & 0x07u) | ((static_cast<Uint32>(minorType) & 0x1fu) << 3);
    packed = (packed & ~(0xffu << shift)) | (newByte << shift);
}

void cSC4TileNeighborhoodCompact::ResetEdgeMinorTypes() noexcept {
    // Exact ResetEdgeMinorTypes @ 0x1001af0a4.
    for (Sint32 edge = 0; edge < 4; ++edge) {
        SetEdgeTypeMinor(edge, 0u);
    }
}

void cSC4TileNeighborhoodCompact::SetEdgeConnectionDirection(Sint32 edge, Uint8 direction) noexcept {
    // Exact SetEdgeConnectionDirection @ 0x1001af0fe.
    constexpr std::array<Uint8, 8> kEdgeDirTable{2u, 3u, 0u, 1u, 2u, 3u, 0u, 1u}; // 0xa1a3a8
    const Uint32 oldByte = GetEdgeCondition(edge);
    const Uint32 combined = (static_cast<Uint32>(direction) + static_cast<Uint32>(edge) * 2u) & 0x07u;
    const Uint8 dirBits = kEdgeDirTable[combined & 0x07u];
    const Uint32 flagBit = ((oldByte & 0x04u) != 0u) ? 1u : 0u;
    const Uint32 newByte = ((flagBit << 2) | static_cast<Uint32>(dirBits)) & 0xffu;
    SetEdgeCondition(edge, static_cast<Uint8>(newByte));
}

bool cSC4TileNeighborhoodCompact::HasConnectionOnEdge(Sint32 edge) const noexcept {
    // Exact HasConnectionOnEdge @ 0x1001aef60.
    const Uint32 shift = static_cast<Uint32>(edge * 8);
    return ((packed & (0x03u << shift)) != 0u);
}

void cSC4TileNeighborhoodCompact::GetStraightAndDiagonalEdges(
    std::array<Sint32, 4>& outStraightEdges,
    std::array<Sint32, 4>& outDiagonalEdges,
    Sint32& outStraightCount,
    Sint32& outDiagonalCount) const noexcept {
    // Exact GetStraightAndDiagonalEdges @ 0x1001aef0e.
    outStraightCount = 0;
    outDiagonalCount = 0;
    for (Sint32 edge = 0; edge < 4; ++edge) {
        const Uint32 condition = (packed >> (edge * 8)) & 0x03u;
        if (condition == 3u || condition == 1u) {
            outDiagonalEdges[static_cast<std::size_t>(outDiagonalCount++)] = edge;
        } else if (condition == 2u) {
            outStraightEdges[static_cast<std::size_t>(outStraightCount++)] = edge;
        }
    }
}

cSC4TileNeighborhoodCompact
cSC4TileNeighborhoodCompact::GetMirrorImageEquivalent() const noexcept {
    // Exact cSC4TileNeighborhoodCompact::GetMirrorImageEquivalent @ 0x1001aee42.
    cSC4TileNeighborhoodCompact result{};
    for (Sint32 edge = 0; edge < 4; ++edge) {
        const Uint8 byteVal = static_cast<Uint8>((packed >> (edge * 8)) & 0xffu);
        const Uint8 oneWayBit = byteVal & 0x04u;
        Uint8 minorBits = byteVal & 0xf8u;
        Uint8 low3 = byteVal & 0x07u;
        if (low3 == 1u) {
            low3 = 3u;
        } else if (low3 == 2u) {
            // Exact bit swap at 0x1001aee7c: preserve bit 3, swap bits 4 and 5.
            const Uint8 bit3 = byteVal & 0x08u;
            const Uint8 bit5To4 = static_cast<Uint8>((byteVal >> 1) & 0x10u);
            const Uint8 bit4To5 = static_cast<Uint8>((byteVal << 1) & 0x20u);
            minorBits = static_cast<Uint8>(bit4To5 | bit3 | bit5To4);
            low3 = 2u;
        } else if (low3 == 3u) {
            low3 = 1u;
        }
        const Uint8 transformed = static_cast<Uint8>(low3 | oneWayBit | minorBits);
        // Exact edge mapping at 0x1001aee9d: if (edge | 2) == 3 (edges 1 and 3,
        // North and South), keep edge; otherwise (edges 0 and 2, West and East),
        // swap via (edge + 2) % 4.
        const Sint32 mirroredEdge = ((edge | 2) == 3) ? edge : ((edge + 2) & 3);
        result.packed |= static_cast<Uint32>(transformed) << (mirroredEdge * 8);
    }
    return result;
}

cSC4TileNeighborhoodCompact
cSC4TileNeighborhoodCompact::GetRotationalEquivalent(Sint32 rotation) const noexcept {
    // Exact rotation branch in cSC4TileRule::GetRotationalEquivalent @ 0x1001af374.
    cSC4TileNeighborhoodCompact result{};
    for (Sint32 edge = 0; edge < 4; ++edge) {
        const Uint32 byteVal = (packed >> (edge * 8)) & 0xffu;
        const Sint32 rotatedEdge = ((edge + rotation) % 4 + 4) % 4;
        result.packed |= (byteVal & 0xffu) << (rotatedEdge * 8);
    }
    return result;
}

void cSC4TileNeighborhoodCompact::Merge(const cSC4TileNeighborhoodCompact& other) noexcept {
    // Exact Merge @ 0x1001aef90.
    if (other.packed == 0u) {
        return;
    }
    for (Sint32 edge = 0; edge < 4; ++edge) {
        const Uint32 shift = static_cast<Uint32>(edge * 8);
        const Uint32 myByte = (packed >> shift) & 0xffu;
        const Uint32 theirByte = (other.packed >> shift) & 0xffu;
        const Uint32 myMajor = myByte & 0x07u;
        const Uint32 theirMajor = theirByte & 0x07u;
        Uint32 merged = myByte;
        if (myMajor == 0u) {
            merged = theirByte;
        } else if (myMajor == 1u && (theirMajor & ~0x07u) == 0u && theirMajor == 2u) {
            merged = (myByte & ~0x07u) | 2u;
        } else if (myMajor == 3u && (theirMajor == 1u || theirMajor == 2u)) {
            merged = (myByte & ~0x07u) | 2u;
        }
        packed = (packed & ~(0xffu << shift)) | ((merged & 0xffu) << shift);
    }
}

Uint8 cSC4TileNeighborhoodCompact::EncodeRulEdgeToken(Sint32 decimalToken) noexcept {
    // Exact conversion in cSC4TileNeighborhoodCompact::Read @ 0x1001af00c:
    // encoded = (token % 10) | ((token / 10) << 3).
    const Sint32 token = decimalToken < 0 ? 0 : decimalToken;
    const Uint32 encoded = static_cast<Uint32>(token % 10) |
        (static_cast<Uint32>(token / 10) << 3);
    return static_cast<Uint8>(encoded);
}

void cSC4TileInfo::GetCell(Uint32 centerX, Uint32 centerZ, Uint32& outX, Uint32& outZ) const noexcept {
    // Exact GetCell @ 0x1001aeb4a.
    Sint8 relX = 0;
    Sint8 relZ = 0;
    ConvertMetaToRelativePos(metaPos, relX, relZ);
    outX = static_cast<Uint32>(static_cast<Sint32>(centerX) + relX);
    outZ = static_cast<Uint32>(static_cast<Sint32>(centerZ) + relZ);
}

cSC4TileInfo cSC4TileInfo::GetRotationalEquivalent(Sint32 rotation) const noexcept {
    // Exact GetRotationalEquivalent @ 0x1001aeb82.
    cSC4TileInfo result{};
    result.metaPos = RotateMetaPosition(metaPos, rotation);
    result.pieceId = pieceId;
    constexpr std::array<Uint8, 16> kResultRotation{
        0u, 3u, 2u, 1u, 1u, 0u, 3u, 2u, 2u, 1u, 0u, 3u, 3u, 2u, 1u, 0u
    };
    const Uint8 flipBit = rotationAndFlip & 0x80u;
    const Uint32 lowBits = rotationAndFlip & 0x7fu;
    Uint8 newLowBits = 0;
    if (flipBit == 0u) {
        newLowBits = static_cast<Uint8>((lowBits + static_cast<Uint32>(rotation)) & 0x03u);
    } else {
        const std::size_t index = static_cast<std::size_t>(lowBits * 4u + static_cast<Uint32>(rotation & 0x03));
        newLowBits = index < kResultRotation.size() ? kResultRotation[index] : static_cast<Uint8>(lowBits & 0x03u);
    }
    result.rotationAndFlip = static_cast<Uint8>(flipBit | (newLowBits & 0x7fu));
    return result;
}

cSC4TileInfo cSC4TileInfo::GetMirrorImageEquivalent() const noexcept {
    // Exact mirror branch in cSC4TileRule::GetMirrorImageEquivalent @ 0x1001af1d0.
    cSC4TileInfo result{};
    result.pieceId = pieceId;
    Sint8 relX = 0;
    Sint8 relZ = 0;
    ConvertMetaToRelativePos(metaPos, relX, relZ);
    result.metaPos = ConvertRelativePosToMeta(static_cast<Sint8>(-relX), relZ);
    result.rotationAndFlip = static_cast<Uint8>(rotationAndFlip ^ 0x80u);
    return result;
}

tCompactNeighborInfo tCompactNeighborInfo::GetMirrorImageEquivalent() const noexcept {
    // Exact tCompactNeighborInfo::GetMirrorImageEquivalent @ 0x1001af2ae.
    tCompactNeighborInfo result{};
    Sint8 relX = 0;
    Sint8 relZ = 0;
    ConvertMetaToRelativePos(metaPos, relX, relZ);
    result.metaPos = ConvertRelativePosToMeta(static_cast<Sint8>(-relX), relZ);
    result.expected = expected.GetMirrorImageEquivalent();
    result.mask = mask.GetMirrorImageEquivalent();
    return result;
}

tCompactNeighborInfo tCompactNeighborInfo::GetRotationalEquivalent(Sint32 rotation) const noexcept {
    // Exact tCompactNeighborInfo::GetRotationalEquivalent @ 0x1001af470.
    tCompactNeighborInfo result{};
    result.metaPos = RotateMetaPosition(metaPos, rotation);
    result.expected = expected.GetRotationalEquivalent(rotation);
    result.mask = mask.GetRotationalEquivalent(rotation);
    return result;
}

cSC4TileRule cSC4TileRule::GetMirrorImageEquivalent() const noexcept {
    // Exact cSC4TileRule::GetMirrorImageEquivalent @ 0x1001af1d0.
    cSC4TileRule result{};
    result.transformFlags = transformFlags;
    result.centerCondition = centerCondition.GetMirrorImageEquivalent();
    for (const auto& cond : conditions) {
        result.conditions.push_back(cond.GetMirrorImageEquivalent());
    }
    for (const auto& sol : solutions) {
        result.solutions.push_back(sol.GetMirrorImageEquivalent());
    }
    return result;
}

cSC4TileRule cSC4TileRule::GetRotationalEquivalent(Sint32 rotation) const noexcept {
    // Exact cSC4TileRule::GetRotationalEquivalent @ 0x1001af374.
    cSC4TileRule result{};
    result.transformFlags = transformFlags;
    result.centerCondition = centerCondition.GetRotationalEquivalent(rotation);
    for (const auto& cond : conditions) {
        result.conditions.push_back(cond.GetRotationalEquivalent(rotation));
    }
    for (const auto& sol : solutions) {
        result.solutions.push_back(sol.GetRotationalEquivalent(rotation));
    }
    return result;
}

bool cSC4TileRule::IsEquivalentByCondition(const cSC4TileRule& other) const noexcept {
    // Exact IsEquivalentByCondition @ 0x1001af576.
    if (centerCondition.packed != other.centerCondition.packed) {
        return false;
    }
    for (const auto& a : conditions) {
        bool found = false;
        for (const auto& b : other.conditions) {
            if (a.metaPos == b.metaPos &&
                a.expected.packed == b.expected.packed &&
                a.mask.packed == b.mask.packed) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    for (const auto& b : other.conditions) {
        bool found = false;
        for (const auto& a : conditions) {
            if (a.metaPos == b.metaPos &&
                a.expected.packed == b.expected.packed &&
                a.mask.packed == b.mask.packed) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

namespace {

struct RulTokenIndex {
    std::vector<std::string> tokens;
    std::size_t pos = 0;

    const std::string* Next() noexcept {
        if (pos >= tokens.size()) {
            return nullptr;
        }
        return &tokens[pos++];
    }
};

inline std::vector<std::string> TokenizeRulText(std::string_view text) {
    // cSC4FileTokenizer::GetToken equivalent: one continuous token stream over
    // the whole resource (rules span lines); tokens split by ',', CR, LF and
    // ';' comments run to end-of-line (ReadLine @ 0x1001b05eb).
    std::vector<std::string> tokens;
    std::string current;
    bool inComment = false;
    const auto flush = [&]() {
        const std::string tok = TrimAscii(current);
        if (!tok.empty()) {
            tokens.push_back(tok);
        }
        current.clear();
    };
    for (const char ch : text) {
        if (inComment) {
            if (ch == '\n') {
                inComment = false;
            }
            continue;
        }
        if (ch == ';') {
            flush();
            inComment = true;
        } else if (ch == ',' || ch == '\r' || ch == '\n') {
            flush();
        } else {
            current.push_back(ch);
        }
    }
    flush();
    return tokens;
}

inline bool ParseTileNeighborhoodCompactTokens(
    RulTokenIndex& idx,
    cSC4TileNeighborhoodCompact& outCompact,
    cSC4TileNeighborhoodCompact* outMask = nullptr) noexcept {
    outCompact.packed = 0;
    if (outMask != nullptr) {
        outMask->packed = 0xffffffffu;
    }
    for (Sint32 edge = 0; edge < 4; ++edge) {
        const std::string* token = idx.Next();
        if (token == nullptr) {
            return false;
        }
        if (*token == "?") {
            const Uint32 shift = static_cast<Uint32>(edge * 8);
            outCompact.packed &= ~(0xffu << shift);
            if (outMask != nullptr) {
                outMask->packed &= ~(0xffu << shift);
            }
            continue;
        }
        const Sint32 decimal = ParseDecimalSint32(*token);
        const Uint8 encoded = cSC4TileNeighborhoodCompact::EncodeRulEdgeToken(decimal);
        outCompact.SetEdgeCondition(edge, encoded);
    }
    return true;
}

inline bool ParseTileInfoTokens(RulTokenIndex& idx, cSC4TileInfo& outInfo) noexcept {
    const std::string* t0 = idx.Next();
    if (t0 == nullptr) {
        return false;
    }
    outInfo.metaPos = static_cast<Uint8>(ParseDecimalSint32(*t0));
    const std::string* t1 = idx.Next();
    if (t1 == nullptr) {
        return false;
    }
    outInfo.pieceId = ParseHexUint32(*t1);
    const std::string* t2 = idx.Next();
    if (t2 == nullptr) {
        return false;
    }
    const Uint8 rotation = static_cast<Uint8>(ParseDecimalSint32(*t2) & 0x7f);
    const std::string* t3 = idx.Next();
    if (t3 == nullptr) {
        return false;
    }
    const bool flipped = ParseDecimalSint32(*t3) != 0;
    outInfo.rotationAndFlip = static_cast<Uint8>(rotation | (flipped ? 0x80u : 0u));
    return true;
}

inline bool ParseSingleTileRule(RulTokenIndex& idx, cSC4TileRule& outRule) {
    // Exact cSC4TileRule::Read @ 0x1001af69a: sequential token parsing from the
    // continuous cSC4FileTokenizer stream; a rule ends when a token that is not
    // a 2/3 section marker is peeked (that token starts the next rule).
    outRule = cSC4TileRule{};

    const std::string* first = idx.Next();
    if (first == nullptr) {
        return false;
    }
    if (IEquals(*first, "noflip")) {
        outRule.transformFlags = 0x0fu;
        first = idx.Next();
    } else if (IEquals(*first, "notrans")) {
        outRule.transformFlags = 0x01u;
        first = idx.Next();
    } else {
        outRule.transformFlags = 0xffu;
    }
    if (first == nullptr) {
        return false;
    }
    if (ParseDecimalSint32(*first) != 1) {
        return false;
    }
    if (!ParseTileNeighborhoodCompactTokens(idx, outRule.centerCondition)) {
        return false;
    }
    while (const std::string* marker = idx.Next()) {
        const Sint32 markerValue = ParseDecimalSint32(*marker);
        if (markerValue == 2) {
            // Exact Read @ 0x1001af69a: after marker 2, metaPos token comes
            // first, then the 4 edge tokens of the expected compact.
            tCompactNeighborInfo cond{};
            const std::string* metaTok = idx.Next();
            if (metaTok == nullptr) {
                return false;
            }
            cond.metaPos = static_cast<Uint8>(ParseDecimalSint32(*metaTok));
            if (!ParseTileNeighborhoodCompactTokens(idx, cond.expected, &cond.mask)) {
                return false;
            }
            outRule.conditions.push_back(cond);
        } else if (markerValue == 3) {
            cSC4TileInfo info{};
            if (!ParseTileInfoTokens(idx, info)) {
                return false;
            }
            outRule.solutions.push_back(info);
        } else {
            // Exact Read @ 0x1001af8f5: non-2/3 digit-leading token is peeked,
            // not consumed; it begins the next rule in the stream.
            if (idx.pos > 0) {
                --idx.pos;
            }
            break;
        }
    }
    return !outRule.solutions.empty();
}

} // namespace

bool cSC4TileEvaluator::LoadFromText(std::string_view rulText) {
    // Exact cSC4TileEvaluator::AddRulesFromResource @ 0x1001b015a:
    // first token "#Transmogrify" selects transmogrify mode; any other leading
    // '#' header (e.g. "#RoadRules#") is a plain rule set; anything else aborts.
    if (rulText.empty()) {
        return false;
    }
    RulTokenIndex idx{};
    idx.tokens = TokenizeRulText(rulText);
    idx.pos = 0;
    const std::string* first = idx.Next();
    if (first == nullptr) {
        return false;
    }
    bool transmogrify = false;
    if (IEquals(*first, "#Transmogrify")) {
        transmogrify = true;
    } else if ((*first)[0] != '#') {
        return false;
    }

    std::vector<cSC4TileRule> rawRules;
    while (idx.pos < idx.tokens.size()) {
        cSC4TileRule rule{};
        if (!ParseSingleTileRule(idx, rule)) {
            break;
        }
        rawRules.push_back(std::move(rule));
    }
    if (rawRules.empty()) {
        return false;
    }

    mTransmogrified = transmogrify;
    if (transmogrify) {
        TransmogrifyRuleSet(mRules, rawRules);
    } else {
        mRules = rawRules;
    }
    std::sort(mRules.begin(), mRules.end(), [](const cSC4TileRule& a, const cSC4TileRule& b) {
        return a.centerCondition.packed < b.centerCondition.packed;
    });
    return !mRules.empty();
}

const std::vector<cSC4TileInfo>* cSC4TileEvaluator::Lookup(
    cSC4TileNeighborhoodCompact centerCondition,
    const std::vector<std::pair<Uint8, cSC4TileNeighborhoodCompact>>& neighborConditions) const noexcept {
    // Exact cSC4TileEvaluator::Lookup @ 0x1001af97c + cSC4TileRule::Match @ 0x1001af600.
    for (const auto& rule : mRules) {
        if (rule.centerCondition.packed != centerCondition.packed) {
            continue;
        }
        bool matched = true;
        for (const auto& cond : rule.conditions) {
            bool found = false;
            for (const auto& nb : neighborConditions) {
                if (nb.first != cond.metaPos) {
                    continue;
                }
                if (((nb.second.packed ^ cond.expected.packed) & cond.mask.packed) == 0u) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                matched = false;
                break;
            }
        }
        if (matched) {
            return &rule.solutions;
        }
    }
    return nullptr;
}

void cSC4TileEvaluator::TransmogrifyRuleSet(
    std::vector<cSC4TileRule>& outRules,
    std::span<const cSC4TileRule> rawRules) {
    // Exact cSC4TileEvaluator::TransmogrifyRuleSet @ 0x1001afa46:
    // 8 dihedral symmetries (4 rotations x mirrored/unmirrored), deduplicated by condition.
    outRules.clear();
    for (const auto& raw : rawRules) {
        if ((raw.transformFlags & 0x04u) != 0u) {
            cSC4TileRule mirrored = raw.GetMirrorImageEquivalent();
            for (Sint32 rot = 0; rot < 4; ++rot) {
                cSC4TileRule candidate = mirrored.GetRotationalEquivalent(rot);
                bool duplicate = false;
                for (const auto& existing : outRules) {
                    if (existing.IsEquivalentByCondition(candidate)) {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate) {
                    outRules.push_back(candidate);
                }
            }
        }
        for (Sint32 rot = 0; rot < 4; ++rot) {
            cSC4TileRule candidate = raw.GetRotationalEquivalent(rot);
            bool duplicate = false;
            for (const auto& existing : outRules) {
                if (existing.IsEquivalentByCondition(candidate)) {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate) {
                outRules.push_back(candidate);
            }
        }
    }
}

Uint32 PatchNetworkTilePair(
    std::span<const NetworkTileConflictRule> rulesForPieceA,
    NetworkSolvedCell& inOutCellA,
    NetworkSolvedCell& inOutCellB,
    Sint32 dirFromAToB) noexcept {
    // Exact cSC4NetworkTool::PatchTilePair @ 0x1001b9124.
    if (rulesForPieceA.empty()) {
        return 0u;
    }
    Uint8 r9b = inOutCellA.rotationAndFlip;
    const Uint32 r11d = inOutCellB.pieceId;
    Uint8 r10b = inOutCellB.rotationAndFlip;
    const Uint8 al0 = static_cast<Uint8>(6 - dirFromAToB);
    const Uint8 bl0 = static_cast<Uint8>(8 - al0);
    const Uint8 deltaA = (static_cast<Sint8>(r9b) < 0) ? bl0 : al0;
    const Uint8 deltaB = (static_cast<Sint8>(r10b) < 0) ? bl0 : al0;
    const Uint8 r8b = static_cast<Uint8>(dirFromAToB + 2);
    r9b = static_cast<Uint8>(r9b + deltaA);
    const Uint8 normA = static_cast<Uint8>(r9b & 0x83u);
    r10b = static_cast<Uint8>((r10b + deltaB) & 0x83u);
    const Uint8 flipA = static_cast<Uint8>(normA ^ 0x82u);
    const Uint8 flipB = static_cast<Uint8>(r10b ^ 0x82u);

    for (const auto& rule : rulesForPieceA) {
        if (rule.pieceA != inOutCellA.pieceId || rule.pieceB != r11d) {
            continue;
        }
        const Uint8 ruleRotA = rule.rotFlipA;
        const Uint8 r13b = static_cast<Uint8>(ruleRotA ^ r9b);
        const bool signMismatch = static_cast<Sint8>(r13b) < 0;
        const Uint8 expectedA = signMismatch ? flipA : normA;
        const Uint8 expectedB = signMismatch ? flipB : r10b;
        if (expectedA != ruleRotA) {
            continue;
        }
        if (r11d != 0u && expectedB != rule.rotFlipB) {
            continue;
        }
        if (rule.replacementA == 0u) {
            return 2u;
        }
        inOutCellA.pieceId = rule.replacementA;
        inOutCellB.pieceId = rule.replacementB;
        Uint8 outRotA = rule.replacementRotFlipA;
        Uint8 outRotB = rule.replacementRotFlipB;
        if (signMismatch) {
            outRotA ^= 0x82u;
            outRotB ^= 0x82u;
        }
        const Uint8 invDir = static_cast<Uint8>(8 - r8b);
        const Uint8 addA = (static_cast<Sint8>(outRotA) < 0) ? invDir : r8b;
        inOutCellA.rotationAndFlip = static_cast<Uint8>((outRotA + addA) & 0x83u);
        const Uint8 addB = (static_cast<Sint8>(outRotB) < 0) ? invDir : r8b;
        inOutCellB.rotationAndFlip = static_cast<Uint8>((outRotB + addB) & 0x83u);
        return 1u;
    }
    return 0u;
}

namespace {

inline void ParseRemapLine(
    std::string_view key,
    std::string_view value,
    std::vector<NetworkTextureRemapEntry>& outEntries,
    bool isPathTable) {
    // Exact sscanf format: " %lx , %lx , %lx , %lx , %d , %d , %d , %lx" (8 fields max).
    // Key is ignored (section enumeration drives parsing).
    (void)key;
    const auto tokens = SplitComma(value);
    if (tokens.size() < 4) {
        return;
    }
    Uint64 matchValue = 0;
    Uint64 matchMask = 0;
    Uint64 addOffset = 0;
    Uint64 keepMask = 0;
    if (!ParseHexUint64(tokens[0], matchValue) ||
        !ParseHexUint64(tokens[1], matchMask) ||
        !ParseHexUint64(tokens[2], addOffset) ||
        !ParseHexUint64(tokens[3], keepMask)) {
        return;
    }
    NetworkTextureRemapEntry entry{};
    entry.matchValue = static_cast<Uint32>(matchValue);
    entry.matchMask = static_cast<Uint32>(matchMask);
    entry.addOffset = static_cast<Uint32>(addOffset);
    entry.keepMask = static_cast<Uint32>(keepMask);
    entry.subtractMask = 0;
    if (tokens.size() > 7) {
        Uint64 subtractMask = 0;
        if (ParseHexUint64(tokens[7], subtractMask)) {
            entry.subtractMask = static_cast<Uint32>(subtractMask);
        }
    }
    Sint32 delta = 0;
    bool continueRemap = false;
    if (tokens.size() > 4) {
        delta = ParseDecimalSint32(tokens[4]);
    }
    if (tokens.size() > 5) {
        continueRemap = ParseDecimalSint32(tokens[5]) != 0;
    }
    (void)isPathTable;
    entry.rotFlipDelta = static_cast<Uint8>((delta & 0x03) | (continueRemap ? 0x80 : 0));
    entry.continueRemap = continueRemap;
    outEntries.push_back(entry);
}

inline bool ParseIniSections(
    std::string_view iniText,
    const std::function<void(std::string_view, std::string_view, std::string_view)>& onEntry) {
    std::string currentSection;
    std::size_t pos = 0;
    while (pos < iniText.size()) {
        std::size_t lineEnd = iniText.find('\n', pos);
        if (lineEnd == std::string_view::npos) {
            lineEnd = iniText.size();
        }
        std::string_view line = TrimView(iniText.substr(pos, lineEnd - pos));
        pos = lineEnd + 1;
        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }
        if (line.front() == '[') {
            const std::size_t close = line.find(']');
            if (close != std::string_view::npos) {
                currentSection = TrimAscii(line.substr(1, close - 1));
            }
            continue;
        }
        const std::size_t eq = line.find('=');
        if (eq == std::string_view::npos) {
            continue;
        }
        const std::string key = TrimAscii(line.substr(0, eq));
        const std::string value = TrimAscii(line.substr(eq + 1));
        onEntry(currentSection, key, value);
    }
    return true;
}

} // namespace

bool cSC4NetworkRuleTables::LoadNetworkIni(std::string_view iniText) {
    // Exact cSC4NetworkManager::Reload @ 0x10016e1ec loading
    // cGZPersistResourceKey{0x0a5bcf4b, 0xaa5bcf57, 0x8a5993b9}.
    textureRemaps.clear();
    shadowRemaps.clear();
    shadowRemapsByEdges.clear();
    ukPathRemaps.clear();
    bool foundAny = false;
    ParseIniSections(iniText, [&](std::string_view section, std::string_view key, std::string_view value) {
        if (IEquals(section, "Texture Remapping")) {
            foundAny = true;
            ParseRemapLine(key, value, textureRemaps, false);
        } else if (IEquals(section, "Shadow Remapping")) {
            foundAny = true;
            ParseRemapLine(key, value, shadowRemaps, false);
        } else if (IEquals(section, "Shadow Edge Remapping")) {
            foundAny = true;
            // Format: " %64[^, ] , %lx , %lx , %lx , %d , %d" (6 fields).
            const std::string keyStr{key};
            const auto valueTokens = SplitComma(value);
            if (valueTokens.size() >= 4) {
                NetworkTextureRemapEntry dummy{};
                (void)dummy;
                NetworkShadowRemapByEdgesEntry entry{};
                SC4NetworkType type{};
                if (ParseNetworkNameToId(keyStr, type)) {
                    entry.networkType = static_cast<Uint32>(type);
                } else {
                    return;
                }
                Uint64 v0 = 0;
                Uint64 v1 = 0;
                Uint64 v2 = 0;
                if (!ParseHexUint64(valueTokens[0], v0) ||
                    !ParseHexUint64(valueTokens[1], v1) ||
                    !ParseHexUint64(valueTokens[2], v2)) {
                    return;
                }
                entry.pieceId = static_cast<Uint32>(v0);
                entry.addOffset = static_cast<Uint32>(v1);
                entry.keepMask = static_cast<Uint32>(v2);
                Sint32 rot = 0;
                bool flipped = false;
                if (valueTokens.size() > 3) {
                    rot = ParseDecimalSint32(valueTokens[3]);
                }
                if (valueTokens.size() > 4) {
                    flipped = ParseDecimalSint32(valueTokens[4]) != 0;
                }
                entry.rotFlip = static_cast<Uint8>((rot & 0x03) | (flipped ? 0x80 : 0));
                shadowRemapsByEdges.push_back(entry);
            }
        } else if (IEquals(section, "UK Path Remapping")) {
            foundAny = true;
            // Format: " %lx , %lx , %lx , %lx" (4 fields).
            const auto tokens = SplitComma(value);
            if (tokens.size() < 4) {
                return;
            }
            NetworkPathRemapEntry entry{};
            Uint64 a = 0;
            Uint64 b = 0;
            Uint64 c = 0;
            Uint64 d = 0;
            if (!ParseHexUint64(tokens[0], a) || !ParseHexUint64(tokens[1], b) ||
                !ParseHexUint64(tokens[2], c) || !ParseHexUint64(tokens[3], d)) {
                return;
            }
            entry.matchValue = static_cast<Uint32>(a);
            entry.matchMask = static_cast<Uint32>(b);
            entry.addOffset = static_cast<Uint32>(c);
            entry.keepMask = static_cast<Uint32>(d);
            ukPathRemaps.push_back(entry);
        }
    });
    return foundAny;
}

bool cSC4NetworkRuleTables::LoadMultitypeRul(std::string_view rulText) {
    // Exact SC4NetworkLoadOverrideMatrix @ 0x1001c6f23 ([AllowedOverrides] section)
    // plus ReadAllMultiTypeIntersectionSolutions @ 0x1001c685c section naming.
    allowedOverridesMatrix.fill(0);
    mixedTypeSolutions.clear();
    bool found = false;
    ParseIniSections(rulText, [&](std::string_view section, std::string_view key, std::string_view value) {
        if (IEquals(section, "AllowedOverrides")) {
            found = true;
            // Entries are "NameA, NameB" pairs enumerated as key=value lines or bare lines.
            const std::string combined = std::string(key) + "," + std::string(value);
            const auto parts = SplitComma(combined);
            if (parts.size() >= 2) {
                SC4NetworkType a{};
                SC4NetworkType b{};
                if (ParseNetworkNameToId(parts[0], a) && ParseNetworkNameToId(parts[1], b)) {
                    const auto ai = static_cast<std::size_t>(a);
                    const auto bi = static_cast<std::size_t>(b);
                    if (ai < allowedOverridesMatrix.size() && bi < allowedOverridesMatrix.size()) {
                        allowedOverridesMatrix[ai] |= (1u << static_cast<Uint32>(b));
                        allowedOverridesMatrix[bi] |= (1u << static_cast<Uint32>(a));
                    }
                }
            }
        }
    });

    // Parse [XxxYyyIntersectionSolutions] sections: keys "x, z" -> "pieceId, rotFlip, ..."
    ParseIniSections(rulText, [&](std::string_view section, std::string_view key, std::string_view value) {
        (void)key; // key holds the "x, z" edge-condition pair; solution fields are in value
        const std::string sectionStr{section};
        const std::string suffix = "IntersectionSolutions";
        if (sectionStr.size() <= suffix.size() ||
            !IEquals(std::string_view(sectionStr).substr(sectionStr.size() - suffix.size()), suffix)) {
            return;
        }
        const std::string prefix = sectionStr.substr(0, sectionStr.size() - suffix.size());
        // Prefix is two concatenated network names; split by trying all type pairs.
        for (Uint32 ai = 0; ai < 13u && prefix.size() > 0; ++ai) {
            const auto typeA = static_cast<SC4NetworkType>(ai);
            const std::string nameA{GetEnglishNetworkName(typeA)};
            if (prefix.size() <= nameA.size() ||
                !IEquals(std::string_view(prefix).substr(0, nameA.size()), nameA)) {
                continue;
            }
            const std::string rest = prefix.substr(nameA.size());
            SC4NetworkType typeB{};
            if (!ParseNetworkNameToId(rest, typeB)) {
                continue;
            }
            const auto valueTokens = SplitComma(value);
            if (valueTokens.size() < 3) {
                continue;
            }
            // AddMultiTypeIntersectionSolution @ 0x1001c6d42 expects 5 comma fields:
            // keyXY -> pieceId, rotation, flip(+0x80), ...
            MixedTypeIntersectionSolution sol{};
            sol.pieceId = ParseHexUint32(valueTokens[0]);
            const Sint32 rotation = ParseDecimalSint32(valueTokens[1]);
            const bool flipped = valueTokens.size() > 2 && ParseDecimalSint32(valueTokens[2]) != 0;
            sol.rotationAndFlip = static_cast<Uint8>((rotation & 0x03) | (flipped ? 0x80 : 0));
            mixedTypeSolutions.push_back({{typeA, typeB}, sol});
            break;
        }
    });
    return found || !mixedTypeSolutions.empty();
}

bool cSC4NetworkRuleTables::LoadOverrideRul(std::string_view rulText) {
    // Exact LoadTileConflictRules @ 0x1001c7d10 ([RuleOverrides] section).
    // Each entry: key & value each parse with " %i , %i , %d , %i , %i , %d" (6 fields).
    tileConflictRules.clear();
    bool found = false;
    ParseIniSections(rulText, [&](std::string_view section, std::string_view key, std::string_view value) {
        if (!IEquals(section, "RuleOverrides")) {
            return;
        }
        found = true;
        const auto keyTokens = SplitComma(key);
        const auto valueTokens = SplitComma(value);
        if (keyTokens.size() < 6 || valueTokens.size() < 6) {
            return;
        }
        NetworkTileConflictRule rule{};
        rule.pieceA = static_cast<Uint32>(ParseDecimalSint32(keyTokens[0]));
        rule.pieceB = static_cast<Uint32>(ParseDecimalSint32(valueTokens[0]));
        const Sint32 keyRot = ParseDecimalSint32(keyTokens[1]);
        const bool keyFlipFlag = ParseDecimalSint32(keyTokens[2]) != 0;
        rule.rotFlipA = static_cast<Uint8>((keyRot & 0x7f) | (keyFlipFlag ? 0x80 : 0));
        rule.replacementA = static_cast<Uint32>(ParseDecimalSint32(keyTokens[3]));
        const Sint32 keyRepRot = ParseDecimalSint32(keyTokens[4]);
        const bool keyRepFlip = ParseDecimalSint32(keyTokens[5]) != 0;
        rule.replacementRotFlipA = static_cast<Uint8>((keyRepRot & 0x7f) | (keyRepFlip ? 0x80 : 0));
        const Sint32 valRot = ParseDecimalSint32(valueTokens[1]);
        const bool valFlipFlag = ParseDecimalSint32(valueTokens[2]) != 0;
        rule.rotFlipB = static_cast<Uint8>((valRot & 0x7f) | (valFlipFlag ? 0x80 : 0));
        rule.replacementB = static_cast<Uint32>(ParseDecimalSint32(valueTokens[3]));
        const Sint32 valRepRot = ParseDecimalSint32(valueTokens[4]);
        const bool valRepFlip = ParseDecimalSint32(valueTokens[5]) != 0;
        rule.replacementRotFlipB = static_cast<Uint8>((valRepRot & 0x7f) | (valRepFlip ? 0x80 : 0));
        tileConflictRules.push_back(rule);
    });
    return found;
}

Uint32 cSC4NetworkRuleTables::MapPreviewTexture(Uint32 textureId, Uint8& inOutRotFlip) const noexcept {
    // Exact MapPreviewTexture @ 0x10016ec20:
    //   result = ((id & keepMask) + addOffset) - (id & subtractMask)
    //   rotFlip += (flipBit ? 8 - (delta&3) : (delta&3)); rotFlip &= 0x83
    //   stop unless entry.continueRemap, in which case result feeds the next entry.
    for (const auto& entry : textureRemaps) {
        if ((textureId & entry.matchMask) != entry.matchValue) {
            continue;
        }
        const Uint32 result = ((textureId & entry.keepMask) + entry.addOffset) -
            (textureId & entry.subtractMask);
        if ((entry.rotFlipDelta & 0x80u) != 0u) {
            inOutRotFlip = static_cast<Uint8>(inOutRotFlip + 8u - (entry.rotFlipDelta & 0x03u));
        } else {
            inOutRotFlip = static_cast<Uint8>(inOutRotFlip + (entry.rotFlipDelta & 0x03u));
        }
        inOutRotFlip &= 0x83u;
        if (!entry.continueRemap) {
            return result;
        }
        textureId = result;
    }
    return textureId;
}

Uint32 cSC4NetworkRuleTables::MapShadowTexture(
    Uint32 shadowId,
    Uint8& inOutRotFlip,
    Uint32 networkTypeFlags,
    Uint32 edgeKey) const noexcept {
    // Exact MapShadowTexture @ 0x10016ece4.
    (void)edgeKey;
    for (const auto& entry : shadowRemaps) {
        if ((shadowId & entry.matchMask) != entry.matchValue) {
            continue;
        }
        if (entry.addOffset != 0u) {
            Uint32 value = (shadowId & entry.keepMask) + entry.addOffset;
            value -= (shadowId & entry.subtractMask);
            if ((entry.rotFlipDelta & 0x80u) != 0u) {
                inOutRotFlip = static_cast<Uint8>(inOutRotFlip + 8u - (entry.rotFlipDelta & 0x03u));
            } else {
                inOutRotFlip = static_cast<Uint8>(inOutRotFlip + (entry.rotFlipDelta & 0x03u));
            }
            inOutRotFlip &= 0x83u;
            if (!entry.continueRemap) {
                return value;
            }
            shadowId = value;
            continue;
        }
        // Edge-based fallback: match pieceId == edgeKey-derived key and network type flag.
        for (const auto& edgeEntry : shadowRemapsByEdges) {
            if (edgeEntry.pieceId != shadowId) {
                continue;
            }
            if ((GetFlagForNetworkType(static_cast<SC4NetworkType>(edgeEntry.networkType & 0x0fu)) &
                 networkTypeFlags) == 0u &&
                edgeEntry.networkType >= 13u) {
                continue;
            }
            Uint32 value = (shadowId & edgeEntry.keepMask) + edgeEntry.addOffset;
            inOutRotFlip = edgeEntry.rotFlip;
            return value;
        }
    }
    return shadowId;
}

Uint32 cSC4NetworkRuleTables::MapPath(Uint32 pathId, bool leftHandDrive) const noexcept {
    // Exact MapPath @ 0x10016ee44 (active only when leftHandDrive != 0).
    if (!leftHandDrive) {
        return pathId;
    }
    for (const auto& entry : ukPathRemaps) {
        if ((pathId & entry.matchMask) == entry.matchValue) {
            return (pathId & entry.keepMask) + entry.addOffset;
        }
    }
    return pathId;
}

bool NetworkManagerSaveRecord::Read(cIGZIStream& stream) noexcept {
    // Exact cSC4NetworkManager::Read @ 0x100171cb2.
    connectionNodes.clear();
    if (!stream.Good()) {
        return false;
    }
    const std::size_t startPos = stream.Position();
    stream >> version;
    if (version != 1u) {
        stream.Seek(startPos);
        return false;
    }
    Uint32 count = 0;
    stream >> count;
    for (Uint32 i = 0; i < count; ++i) {
        ObjectRef ref{};
        if (!ReadSerializableRef(stream, ref)) {
            return false;
        }
        connectionNodes.push_back(ref);
    }
    return stream.Good();
}

bool NetworkManagerSaveRecord::Write(cIGZOStream& stream) const {
    // Exact cSC4NetworkManager::Write @ 0x100171d8c.
    stream << static_cast<Uint16>(1u);
    stream << static_cast<Uint32>(connectionNodes.size());
    for (const auto& ref : connectionNodes) {
        WriteSerializableRef(stream, ref);
    }
    return true;
}

bool NetworkManagerSaveRecord::ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept {
    // Exact DBPF subfile wrapper: [recordSize:Uint32][flags:Uint32][memAddr:Uint32][chunk].
    if (recordBytes.size() < 14u) {
        return false;
    }
    Uint32 recSize = 0;
    std::memcpy(&recSize, recordBytes.data(), sizeof(Uint32));
    if (recSize < 14u || recSize > recordBytes.size()) {
        return false;
    }
    std::memcpy(&memAddr, recordBytes.data() + 8u, sizeof(Uint32));
    cIGZIStream stream(recordBytes.data() + 12u, recSize - 12u);
    return Read(stream);
}

bool cSC4NetworkLotConfiguration::LoadFromExemplar(const cGZExemplar& exemplar, Uint8 detailLevel) {
    // Exact cSC4NetworkLotManager::ReadLotConfiguration @ 0x100405946.
    version = 1u;
    objects.clear();
    name.clear();
    networkPieceId = 0;
    const auto findProp = [&exemplar](Uint32 propId) -> const ExemplarProperty* {
        const auto it = exemplar.properties.find(propId);
        return it != exemplar.properties.end() ? &it->second : nullptr;
    };
    const auto propInt = [&findProp](Uint32 propId, std::size_t idx, Sint64 fallback) -> Sint64 {
        const auto* prop = findProp(propId);
        if (prop == nullptr || idx >= prop->intValues.size()) {
            return fallback;
        }
        return prop->intValues[idx];
    };
    const auto propFloat = [&findProp](Uint32 propId, std::size_t idx, Float32 fallback) -> Float32 {
        const auto* prop = findProp(propId);
        if (prop == nullptr) {
            return fallback;
        }
        if (idx < prop->floatValues.size()) {
            return prop->floatValues[idx];
        }
        if (idx < prop->intValues.size()) {
            return static_cast<Float32>(prop->intValues[idx]);
        }
        return fallback;
    };
    {
        const auto* prop = findProp(0xc9a5a1beu);
        if (prop == nullptr || prop->intValues.empty()) {
            return false;
        }
        networkPieceId = static_cast<Uint32>(prop->intValues[0]);
    }
    name = exemplar.GetString(0x00000020u);
    id = static_cast<Uint32>(propInt(0x00000021u, 0, 0));
    if (const auto* prop = findProp(0x88edc793u)) {
        zoneTypeMask = 0;
        for (const Sint64 value : prop->intValues) {
            if (value >= 0 && value < 16) {
                zoneTypeMask = static_cast<Uint16>(zoneTypeMask |
                    static_cast<Uint16>(1u << static_cast<Uint32>(value)));
            }
        }
    }
    if (const auto* prop = findProp(0x88edc795u)) {
        wealthTypeMask = 0;
        for (const Sint64 value : prop->intValues) {
            if (value >= 0 && value < 4) {
                wealthTypeMask = static_cast<Uint8>(wealthTypeMask |
                    static_cast<Uint8>(1u << static_cast<Uint32>(value)));
            }
        }
    }
    if (const auto* prop = findProp(0x49d55951u)) {
        if (prop->intValues.size() >= 4) {
            for (std::size_t i = 0; i < 4; ++i) {
                positionStrideMask[i] = static_cast<Uint8>(prop->intValues[i]);
            }
        }
    }
    positionModulo = static_cast<Uint8>(propInt(0xca81b8d4u, 0, 1));
    minLandValue = propFloat(0xaa120972u, 0, 0.0f);
    maxLandValue = propFloat(0xaa120973u, 0, 1000.0f);
    rotationMask = static_cast<Uint8>(propInt(0xec3bd470u, 0, 0x0f));
    flipRequirement = static_cast<Uint8>(propInt(0xcc3e4755u, 0, 0));
    version = static_cast<Uint8>(propInt(0x88edc789u, 0, 1));
    // Reuse exact lot object decoding (ConvertPropertiesToObjectArray).
    cSC4LotConfiguration tempConfig{};
    if (!tempConfig.LoadFromExemplar(exemplar, id, detailLevel)) {
        return false;
    }
    objects = tempConfig.objects;
    cSC4LotConfiguration::CorrectObjectPositions(objects, 1.0f, 1.0f);
    return true;
}

bool cSC4NetworkLotConfiguration::IsCompatible(
    Sint32 cellX,
    Sint32 cellZ,
    Uint32 zoneType,
    Uint32 wealthType,
    Uint8 pieceRotation,
    bool pieceFlipped,
    Float32 cellLandValue) const noexcept {
    // Exact cSC4NetworkLotManager::IsLotCompatible @ 0x100407750.
    if (positionModulo == 0u) {
        return false;
    }
    const Sint32 idxX = ((cellX % positionModulo) + positionModulo) % positionModulo;
    const Sint32 idxZ = ((cellZ % positionModulo) + positionModulo) % positionModulo;
    // Binary @ 0x100407750: byte index = cellZ % modulo, bit = cellX % modulo.
    const Uint8 strideByte = positionStrideMask[static_cast<std::size_t>(idxZ & 0x03)];
    if (((strideByte >> idxX) & 1u) == 0u) {
        return false;
    }
    if (zoneType >= 16u || ((zoneTypeMask >> zoneType) & 1u) == 0u) {
        return false;
    }
    if (wealthType >= 4u || ((wealthTypeMask >> wealthType) & 1u) == 0u) {
        return false;
    }
    if (((rotationMask >> (pieceRotation & 0x03u)) & 1u) == 0u) {
        return false;
    }
    if (flipRequirement == 1u && !pieceFlipped) {
        return false;
    }
    if (flipRequirement == 2u && pieceFlipped) {
        return false;
    }
    if (cellLandValue < minLandValue || cellLandValue > maxLandValue) {
        return false;
    }
    return true;
}

NetworkRoadStyleResult ComputeNetworkRoadStyle(
    Uint32 zoneType,
    Uint32 wealthType,
    bool isRailOnDirt) noexcept {
    // Exact cSC4NetworkLotManager::UpdateRoadStyle @ 0x1004074f4.
    NetworkRoadStyleResult result{};
    if (isRailOnDirt) {
        result.underTextureId = 0x08400000u;
        result.hasUnderTextureFlag = true;
    } else {
        constexpr std::array<std::array<Uint32, 4>, 16> kNetworkUnderTextureId{{
            {0x00000000u, 0x08100000u, 0x08200000u, 0x08300000u},
            {0x08200000u, 0x08100000u, 0x08200000u, 0x08300000u},
            {0x08200000u, 0x08100000u, 0x08200000u, 0x08300000u},
            {0x08200000u, 0x08100000u, 0x08200000u, 0x08300000u},
            {0x08200000u, 0x08100000u, 0x08200000u, 0x08300000u},
            {0x08200000u, 0x08100000u, 0x08200000u, 0x08300000u},
            {0x08200000u, 0x08100000u, 0x08200000u, 0x08300000u},
            {0x08400000u, 0x08400000u, 0x08400000u, 0x08400000u},
            {0x08200000u, 0x08100000u, 0x08200000u, 0x08200000u},
            {0x08200000u, 0x08100000u, 0x08200000u, 0x08200000u},
            {0x08200000u, 0x08100000u, 0x08200000u, 0x08300000u},
            {0x08200000u, 0x08100000u, 0x08200000u, 0x08300000u},
            {0x08200000u, 0x08100000u, 0x08200000u, 0x08300000u},
            {0x08200000u, 0x08100000u, 0x08200000u, 0x08300000u},
            {0x08400000u, 0x08400000u, 0x08400000u, 0x08400000u},
            {0x08200000u, 0x08100000u, 0x08200000u, 0x08300000u},
        }};
        const std::size_t zIdx = zoneType < 16u ? zoneType : 0u;
        const std::size_t wIdx = wealthType < 4u ? wealthType : 0u;
        result.underTextureId = kNetworkUnderTextureId[zIdx][wIdx];
        result.hasUnderTextureFlag = result.underTextureId != 0u;
    }
    {
        constexpr std::array<std::array<Uint8, 3>, 16> kNetworkVariationTable{{
            {0u, 0u, 0u},
            {1u, 2u, 3u},
            {4u, 5u, 6u},
            {4u, 5u, 6u},
            {1u, 2u, 3u},
            {4u, 5u, 6u},
            {4u, 5u, 6u},
            {7u, 7u, 7u},
            {1u, 1u, 2u},
            {1u, 1u, 2u},
            {1u, 1u, 1u},
            {1u, 1u, 1u},
            {1u, 1u, 1u},
            {1u, 1u, 1u},
            {7u, 7u, 7u},
            {1u, 1u, 1u},
        }};
        const std::size_t zIdx = zoneType < 16u ? zoneType : 0u;
        const std::size_t wIdx = wealthType > 0u ? (wealthType - 1u < 3u ? wealthType - 1u : 2u) : 0u;
        result.variation = kNetworkVariationTable[zIdx][wIdx];
    }
    return result;
}

Uint32 ComputeZoneTypeForNetworkCell(
    Sint32 cellX,
    Sint32 cellZ,
    Sint32 citySizeX,
    Sint32 citySizeZ,
    std::span<const Sint8> zoneGrid) noexcept {
    // Exact cSC4NetworkLotManager::GetZoneTypeForNetwork @ 0x10040800c.
    constexpr std::array<Sint32, 8> kDXArr{-1, 0, 1, 0, -1, -1, 1, 1};
    constexpr std::array<Sint32, 8> kDZArr{0, -1, 0, 1, -1, 1, -1, 1};
    constexpr std::array<Sint32, 16> kZonePriorityTable{
        18, 4, 7, 6, 5, 4, 3, 8, 2, 1, 16, 16, 16, 16, 17, 16
    };
    const auto index = [&](Sint32 x, Sint32 z) noexcept -> Sint8 {
        if (x < 0 || z < 0 || x >= citySizeX || z >= citySizeZ) {
            return 0;
        }
        const std::size_t idx = static_cast<std::size_t>(z) * static_cast<std::size_t>(citySizeX) +
            static_cast<std::size_t>(x);
        return idx < zoneGrid.size() ? zoneGrid[idx] : 0;
    };
    Sint8 center = index(cellX, cellZ);
    if (center != 0) {
        return static_cast<Uint32>(center);
    }
    Sint32 best = 0;
    bool scanDiagonals = false;
    for (Sint32 dir = 0; dir < 8; ++dir) {
        if (dir >= 4 && !scanDiagonals) {
            // Exact early-break at 0x1004080a2: diagonal neighbors (4..7) are
            // only scanned when the cardinal pass (0..3) left best in the
            // bitmask 0x4081 (Unzoned=0, Military=7, Plopped=14).
            break;
        }
        const Sint32 nx = cellX + kDXArr[static_cast<std::size_t>(dir)];
        const Sint32 nz = cellZ + kDZArr[static_cast<std::size_t>(dir)];
        if (nx >= 0 && nz >= 0 && nx < citySizeX && nz < citySizeZ) {
            const Sint8 neighbor = index(nx, nz);
            const Sint32 neighborPrio = (neighbor >= 0 && neighbor < 16)
                ? kZonePriorityTable[static_cast<std::size_t>(neighbor)] : 0;
            const Sint32 bestPrio = (best >= 0 && best < 16)
                ? kZonePriorityTable[static_cast<std::size_t>(best)] : 0;
            if (neighborPrio < bestPrio) {
                best = neighbor;
            }
        }
        if (dir == 3 && best >= 0 && best <= 14 &&
            ((0x4081u >> best) & 1u) != 0u) {
            scanDiagonals = true;
        }
    }
    return static_cast<Uint32>(best);
}

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
    Sint32& outOrientation) noexcept {
    // Exact cSC4NetworkLotManager::GetPositionForLotConfigurationObject @ 0x100407c02.
    outWorldY = networkAltitudeY + obj.posY;
    const Float32 cellCenterX = static_cast<Float32>(cellX * 16);
    const Float32 cellCenterZ = static_cast<Float32>(cellZ * 16);
    const Float32 halfCell = 8.0f;
    const Uint8 effectiveRotation = pieceRotation & 0x03u;
    Float32 x = 0.0f;
    Float32 z = 0.0f;
    switch (effectiveRotation) {
        case 0u:
            x = (cellCenterX + halfCell) - obj.posX;
            z = (cellCenterZ + halfCell) - obj.posZ;
            break;
        case 1u:
            x = (cellCenterX + halfCell) + obj.posZ;
            z = (cellCenterZ + halfCell) - obj.posX;
            break;
        case 2u:
            x = (cellCenterX + halfCell) + obj.posX;
            z = (cellCenterZ + halfCell) + obj.posZ;
            break;
        case 3u:
            x = (cellCenterX + halfCell) - obj.posZ;
            z = (cellCenterZ + halfCell) + obj.posX;
            break;
        default:
            x = 0.0f;
            z = 0.0f;
            break;
    }
    outWorldX = x;
    outWorldZ = z;
    if (pieceFlipped) {
        const Float32 mirrored = (16.0f - (x - cellCenterX) * 2.0f) + x;
        outWorldX = mirrored;
        constexpr std::array<std::array<Sint32, 4>, 4> kFlippedRotation{{
            {0, 3, 2, 1},
            {3, 2, 1, 0},
            {2, 1, 0, 3},
            {1, 0, 3, 2},
        }};
        const std::size_t lodIdx = obj.lodAndFlags < 4u ? obj.lodAndFlags : 0u;
        outOrientation = kFlippedRotation[lodIdx][effectiveRotation];
    } else {
        outOrientation = static_cast<Sint32>((obj.orientation + effectiveRotation) & 0x03u);
    }
}

bool NetworkLotManagerSaveRecord::Read(cIGZIStream& stream) noexcept {
    // Exact cSC4NetworkLotManager::Read @ 0x10040562a.
    networkLots.clear();
    damageEffects.clear();
    if (!stream.Good()) {
        return false;
    }
    stream >> version;
    if (version != 2u) {
        return false;
    }
    Uint32 lotCount = 0;
    stream >> lotCount;
    for (Uint32 i = 0; i < lotCount; ++i) {
        NetworkLotSaveEntry entry{};
        stream >> entry.packedCellXZ;
        if (!ReadSerializableRef(stream, entry.networkOccupant)) {
            return false;
        }
        stream >> entry.configurationId;
        if (!stream.Good()) {
            return false;
        }
        networkLots.push_back(entry);
    }
    Uint32 effectCount = 0;
    stream >> effectCount;
    for (Uint32 i = 0; i < effectCount; ++i) {
        NetworkDamageEffectEntry entry{};
        stream >> entry.packedCellXZ;
        if (!ReadSerializableRef(stream, entry.visualEffectOccupant)) {
            return false;
        }
        damageEffects.push_back(entry);
    }
    return stream.Good();
}

bool NetworkLotManagerSaveRecord::Write(cIGZOStream& stream) const {
    // Exact cSC4NetworkLotManager::Write @ 0x10040555a.
    stream << static_cast<Uint16>(2u);
    stream << static_cast<Uint32>(networkLots.size());
    for (const auto& entry : networkLots) {
        stream << entry.packedCellXZ;
        WriteSerializableRef(stream, entry.networkOccupant);
        stream << entry.configurationId;
    }
    stream << static_cast<Uint32>(damageEffects.size());
    for (const auto& entry : damageEffects) {
        stream << entry.packedCellXZ;
        WriteSerializableRef(stream, entry.visualEffectOccupant);
    }
    return true;
}

bool NetworkLotManagerSaveRecord::ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept {
    // Exact DBPF subfile wrapper: [recordSize:Uint32][flags:Uint32][memAddr:Uint32][chunk].
    if (recordBytes.size() < 14u) {
        return false;
    }
    Uint32 recSize = 0;
    std::memcpy(&recSize, recordBytes.data(), sizeof(Uint32));
    if (recSize < 14u || recSize > recordBytes.size()) {
        return false;
    }
    std::memcpy(&memAddr, recordBytes.data() + 8u, sizeof(Uint32));
    cIGZIStream stream(recordBytes.data() + 12u, recSize - 12u);
    return Read(stream);
}

namespace {

Uint8 FirstLotEdge(Uint8 edge) noexcept {
    if ((edge & kLotEdgeNorth) != 0u) return kLotEdgeNorth;
    if ((edge & kLotEdgeEast) != 0u) return kLotEdgeEast;
    if ((edge & kLotEdgeSouth) != 0u) return kLotEdgeSouth;
    if ((edge & kLotEdgeWest) != 0u) return kLotEdgeWest;
    return 0u;
}

Uint8 OppositeLotEdge(Uint8 edge) noexcept {
    switch (edge) {
        case kLotEdgeNorth: return kLotEdgeSouth;
        case kLotEdgeSouth: return kLotEdgeNorth;
        case kLotEdgeEast: return kLotEdgeWest;
        case kLotEdgeWest: return kLotEdgeEast;
        default: return edge;
    }
}

// cSC4BuildingOccupant::ConvertZoneToDensity: types 1..9 cycle low/med/high.
Sint32 ZoneDensity(Uint32 zoneType) noexcept {
    if (zoneType < 1u || zoneType > 9u) return 0;
    return static_cast<Sint32>((zoneType - 1u) % 3u) + 1;
}

Sint32 RectWidth(const SC4Rect<Sint32>& rect) noexcept {
    return rect.x1 - rect.x0;
}

Sint32 RectDepth(const SC4Rect<Sint32>& rect) noexcept {
    return rect.y1 - rect.y0;
}

Sint32 PerpendicularSpan(const SC4Rect<Sint32>& rect, Uint8 edge) noexcept {
    if (edge == kLotEdgeNorth || edge == kLotEdgeSouth) return RectDepth(rect);
    if (edge == kLotEdgeEast || edge == kLotEdgeWest) return RectWidth(rect);
    return 0;
}

Sint32 FrontageSpan(const SC4Rect<Sint32>& rect, Uint8 edge) noexcept {
    if (edge == kLotEdgeNorth || edge == kLotEdgeSouth) return RectWidth(rect);
    if (edge == kLotEdgeEast || edge == kLotEdgeWest) return RectDepth(rect);
    return 0;
}

void ShrinkFromEdge(SC4Rect<Sint32>& rect, Uint8 edge, Sint32 cells) noexcept {
    switch (edge) {
        case kLotEdgeNorth: rect.y0 += cells; break;
        case kLotEdgeSouth: rect.y1 -= cells; break;
        case kLotEdgeWest: rect.x0 += cells; break;
        case kLotEdgeEast: rect.x1 -= cells; break;
        default: break;
    }
}

void EmitFacingStrip(
    const SC4Rect<Sint32>& rect,
    Uint8 edge,
    Sint32 frontage,
    Sint32 depth,
    std::vector<ZoneParcel>& out) {
    if (frontage < 1 || depth < 1) return;
    const auto pushParcel = [&](Sint32 x0, Sint32 z0, Sint32 x1, Sint32 z1, Sint32 face) {
        ZoneParcel parcel;
        parcel.cells.x0 = x0;
        parcel.cells.y0 = z0;
        parcel.cells.x1 = x1;
        parcel.cells.y1 = z1;
        parcel.facingEdge = edge;
        parcel.frontage = face;
        parcel.depth = depth;
        out.push_back(parcel);
    };
    if (edge == kLotEdgeNorth) {
        const Sint32 z0 = rect.y0;
        const Sint32 z1 = z0 + depth;
        for (Sint32 x = rect.x0; x < rect.x1;) {
            const Sint32 face = std::min(frontage, rect.x1 - x);
            pushParcel(x, z0, x + face, z1, face);
            x += face;
        }
    } else if (edge == kLotEdgeSouth) {
        const Sint32 z1 = rect.y1;
        const Sint32 z0 = z1 - depth;
        for (Sint32 x = rect.x0; x < rect.x1;) {
            const Sint32 face = std::min(frontage, rect.x1 - x);
            pushParcel(x, z0, x + face, z1, face);
            x += face;
        }
    } else if (edge == kLotEdgeWest) {
        const Sint32 x0 = rect.x0;
        const Sint32 x1 = x0 + depth;
        for (Sint32 z = rect.y0; z < rect.y1;) {
            const Sint32 face = std::min(frontage, rect.y1 - z);
            pushParcel(x0, z, x1, z + face, face);
            z += face;
        }
    } else if (edge == kLotEdgeEast) {
        const Sint32 x1 = rect.x1;
        const Sint32 x0 = x1 - depth;
        for (Sint32 z = rect.y0; z < rect.y1;) {
            const Sint32 face = std::min(frontage, rect.y1 - z);
            pushParcel(x0, z, x1, z + face, face);
            z += face;
        }
    }
}

bool InclusiveContains(const SC4Rect<Sint32>& rect, Sint32 x, Sint32 z) noexcept {
    const Sint32 minX = std::min(rect.x0, rect.x1);
    const Sint32 maxX = std::max(rect.x0, rect.x1);
    const Sint32 minZ = std::min(rect.y0, rect.y1);
    const Sint32 maxZ = std::max(rect.y0, rect.y1);
    return x >= minX && x <= maxX && z >= minZ && z <= maxZ;
}

Float32 SampleResponseCurve(
    Float32 x,
    std::span<const Float32> inputs,
    std::span<const Float32> outputs,
    Float32 fallback) noexcept {
    if (inputs.empty() || outputs.empty() || inputs.size() != outputs.size()) {
        return fallback;
    }
    if (x <= inputs.front()) return outputs.front();
    if (x >= inputs.back()) return outputs.back();
    for (std::size_t i = 1; i < inputs.size(); ++i) {
        if (x <= inputs[i]) {
            const Float32 x0 = inputs[i - 1u];
            const Float32 x1 = inputs[i];
            const Float32 span = x1 - x0;
            if (!(span > 0.0f)) return outputs[i];
            const Float32 t = (x - x0) / span;
            return outputs[i - 1u] + t * (outputs[i] - outputs[i - 1u]);
        }
    }
    return outputs.back();
}

struct PathOpenNode {
    Float32 f = 0.0f;
    Float32 g = 0.0f;
    Sint32 x = 0;
    Sint32 z = 0;
};

struct PathOpenWorse {
    bool operator()(const PathOpenNode& a, const PathOpenNode& b) const noexcept {
        if (a.f != b.f) return a.f > b.f;
        if (a.g != b.g) return a.g > b.g;
        if (a.z != b.z) return a.z > b.z;
        return a.x > b.x;
    }
};

void AppendCompressedRoute(SC4PathFindResult& result) {
    result.compressedRoute.clear();
    if (result.cellX.size() < 2u || result.cellZ.size() != result.cellX.size()) return;
    Uint8 runDir = 0;
    Uint8 runType = 0;
    Uint8 runLen = 0;
    const auto flush = [&]() {
        if (runLen == 0u) return;
        result.compressedRoute.push_back(
            static_cast<Uint8>((static_cast<Uint32>(runType) << 2u) | (runDir & 0x3u)));
        result.compressedRoute.push_back(runLen);
        runLen = 0;
    };
    for (std::size_t i = 1; i < result.cellX.size(); ++i) {
        const Sint32 dx = result.cellX[i] - result.cellX[i - 1u];
        const Sint32 dz = result.cellZ[i] - result.cellZ[i - 1u];
        Uint8 dir = 0;
        if (dx == 1 && dz == 0) dir = 0;
        else if (dx == -1 && dz == 0) dir = 1;
        else if (dx == 0 && dz == 1) dir = 2;
        else if (dx == 0 && dz == -1) dir = 3;
        else {
            flush();
            continue;
        }
        const Uint8 type = i < result.travelType.size() ? result.travelType[i] : static_cast<Uint8>(0);
        if (runLen > 0u && dir == runDir && type == runType && runLen < 255u) {
            ++runLen;
        } else {
            flush();
            runDir = dir;
            runType = type;
            runLen = 1;
        }
    }
    flush();
}

} // namespace

ZoneLotSize cSC4ZoneParcellizer::DetermineLotSize(
    const SC4Rect<Sint32>& region,
    Uint32 zoneType,
    Uint8 roadEdge) noexcept {
    // Exact cSC4ZoneDeveloper::DetermineLotSize @ 0x10012d606.
    // Density selects a preferred depth (low 2, medium 3, high 4) clamped to
    // the block. Frontage is the largest of 1..4 that tiles the road edge.
    // High density prefers a 3-wide lot when 3 divides the frontage, matching
    // the `imul ..., 3` building-row path. Shallow low-density blocks cap
    // frontage at 2.
    ZoneLotSize size;
    const Sint32 width = RectWidth(region);
    const Sint32 depth = RectDepth(region);
    if (width <= 0 || depth <= 0) return size;
    Uint8 edge = FirstLotEdge(roadEdge);
    if (edge == 0u) edge = kLotEdgeNorth;
    const Sint32 frontSpan = FrontageSpan(region, edge);
    const Sint32 depthSpan = PerpendicularSpan(region, edge);
    if (frontSpan <= 0 || depthSpan <= 0) return size;

    const Sint32 density = ZoneDensity(zoneType);
    Sint32 desiredDepth = 2;
    if (density == 2) desiredDepth = 3;
    else if (density >= 3) desiredDepth = 4;
    else if (density <= 0) desiredDepth = 1;
    if (desiredDepth > depthSpan) desiredDepth = depthSpan;
    if (desiredDepth < 1) desiredDepth = 1;

    Sint32 frontage = 1;
    for (Sint32 candidate = 1; candidate <= 4 && candidate <= frontSpan; ++candidate) {
        if (frontSpan % candidate == 0) frontage = candidate;
    }
    if (density <= 1 && depthSpan <= 1 && frontage > 2) frontage = 2;
    if (density >= 3 && frontSpan >= 3 && (frontSpan % 3) == 0) frontage = 3;
    if (frontage > frontSpan) frontage = frontSpan;
    if (frontage < 1) frontage = 1;

    size.frontage = frontage;
    size.depth = desiredDepth;
    return size;
}

void cSC4ZoneParcellizer::SimpleParcellize(
    const SC4Rect<Sint32>& region,
    Uint32 zoneType,
    Uint8 roadEdge,
    Sint32 maxDepth,
    std::vector<ZoneParcel>& outParcels) {
    // Exact cSC4ZoneDeveloper::SimpleParcellize @ 0x10012c320.
    // Walks the road edge and emits CreateLotToFaceRoad parcels. When the
    // block is at least two cells deeper than the lot, DrawStreet consumes
    // one cell and the far side is parcelled facing that street. A positive
    // maxDepth caps the lot depth chosen by DetermineLotSize.
    outParcels.clear();
    Uint8 facing = FirstLotEdge(roadEdge);
    if (facing == 0u) facing = kLotEdgeNorth;
    const ZoneLotSize natural = DetermineLotSize(region, zoneType, facing);
    if (natural.frontage < 1 || natural.depth < 1) return;

    Sint32 depthCap = natural.depth;
    if (maxDepth > 0 && maxDepth < depthCap) depthCap = maxDepth;
    if (depthCap < 1) return;

    SC4Rect<Sint32> cursor = region;
    const Sint32 guardLimit = RectWidth(region) + RectDepth(region) + 2;
    for (Sint32 guard = 0; guard < guardLimit; ++guard) {
        const Sint32 available = PerpendicularSpan(cursor, facing);
        if (available <= 0 || FrontageSpan(cursor, facing) <= 0) break;
        Sint32 useDepth = depthCap;
        if (available < useDepth + 2) useDepth = available;
        if (useDepth > available) useDepth = available;
        if (useDepth < 1) break;
        EmitFacingStrip(cursor, facing, natural.frontage, useDepth, outParcels);
        ShrinkFromEdge(cursor, facing, useDepth);
        const Sint32 left = PerpendicularSpan(cursor, facing);
        if (left < 2) {
            if (left == 1) {
                EmitFacingStrip(cursor, facing, natural.frontage, 1, outParcels);
            }
            break;
        }
        ShrinkFromEdge(cursor, facing, 1);
        facing = OppositeLotEdge(facing);
    }
}

bool cSC4ZoneParcellizer::GetVerticesAtEdgeOfLot(
    const SC4Rect<Sint32>& lot,
    Uint8 edge,
    SC4Rect<Sint32>& outVertices) noexcept {
    // Exact cSC4ZoneDeveloper::GetVerticesAtEdgeOfLot @ 0x100131946.
    // The vertex span is inclusive: a half-open cell edge is the vertex index.
    if (lot.x1 <= lot.x0 || lot.y1 <= lot.y0) return false;
    const Uint8 bit = FirstLotEdge(edge);
    switch (bit) {
        case kLotEdgeNorth:
            outVertices = SC4Rect<Sint32>{lot.x0, lot.y0, lot.x1, lot.y0};
            return true;
        case kLotEdgeEast:
            outVertices = SC4Rect<Sint32>{lot.x1, lot.y0, lot.x1, lot.y1};
            return true;
        case kLotEdgeSouth:
            outVertices = SC4Rect<Sint32>{lot.x0, lot.y1, lot.x1, lot.y1};
            return true;
        case kLotEdgeWest:
            outVertices = SC4Rect<Sint32>{lot.x0, lot.y0, lot.x0, lot.y1};
            return true;
        default:
            return false;
    }
}

bool cSC4ZoneParcellizer::GetNeighborCellsAtEdgeOfLot(
    const SC4Rect<Sint32>& lot,
    Uint8 edge,
    SC4Rect<Sint32>& outCells) const noexcept {
    // Exact cSC4ZoneDeveloper::GetNeighborCellsAtEdgeOfLot @ 0x1001319a6.
    // One-cell strip just outside the lot, clipped to the city extent.
    if (lot.x1 <= lot.x0 || lot.y1 <= lot.y0) return false;
    const Uint8 bit = FirstLotEdge(edge);
    SC4Rect<Sint32> neighbor = lot;
    switch (bit) {
        case kLotEdgeNorth:
            if (lot.y0 <= 0) return false;
            neighbor.y1 = lot.y0;
            neighbor.y0 = lot.y0 - 1;
            break;
        case kLotEdgeEast:
            if (cityCellsX <= 0 || lot.x1 >= cityCellsX) return false;
            neighbor.x0 = lot.x1;
            neighbor.x1 = lot.x1 + 1;
            break;
        case kLotEdgeSouth:
            if (cityCellsZ <= 0 || lot.y1 >= cityCellsZ) return false;
            neighbor.y0 = lot.y1;
            neighbor.y1 = lot.y1 + 1;
            break;
        case kLotEdgeWest:
            if (lot.x0 <= 0) return false;
            neighbor.x1 = lot.x0;
            neighbor.x0 = lot.x0 - 1;
            break;
        default:
            return false;
    }
    outCells = neighbor;
    return true;
}

Sint32 cSC4PathFinderEngine::DistanceToRectangle(
    Sint32 x, Sint32 z, const SC4Rect<Sint32>& rect) noexcept {
    // Exact cSC4PathFinder::DistanceToRectangle @ 0x1000d3b32.
    // Inside the inclusive rectangle the distance is 0. Outside it is the
    // Manhattan distance to the nearest cell of the rectangle (the grid
    // heuristic used by FindPath; an axis-aligned LengthOfLine reduces to
    // this, and the A* step cost is 4-connected).
    const Sint32 minX = std::min(rect.x0, rect.x1);
    const Sint32 maxX = std::max(rect.x0, rect.x1);
    const Sint32 minZ = std::min(rect.y0, rect.y1);
    const Sint32 maxZ = std::max(rect.y0, rect.y1);
    Sint32 dx = 0;
    if (x < minX) dx = minX - x;
    else if (x > maxX) dx = x - maxX;
    Sint32 dz = 0;
    if (z < minZ) dz = minZ - z;
    else if (z > maxZ) dz = z - maxZ;
    return dx + dz;
}

bool cSC4PathFinderEngine::FindPath(
    const SC4Rect<Sint32>& startRect,
    const SC4Rect<Sint32>& endRect,
    Sint32 gridCellsX,
    Sint32 gridCellsZ,
    std::span<const Float32> travelCost,
    std::span<const Uint8> travelType,
    Float32 maxTripLength,
    SC4PathFindResult& out) {
    // Exact cSC4PathFinder::FindPath @ 0x1000d2644 plus GetCompressedRoute
    // @ 0x1000d434e. 4-connected A* ordered by travel cost + Manhattan
    // distance to endRect. The heuristic is scaled by the cheapest positive
    // tile cost so it stays admissible.
    out = SC4PathFindResult{};
    if (gridCellsX <= 0 || gridCellsZ <= 0) return false;
    const std::size_t width = static_cast<std::size_t>(gridCellsX);
    const std::size_t height = static_cast<std::size_t>(gridCellsZ);
    if (width != 0u && height > (std::numeric_limits<std::size_t>::max() / width)) return false;
    const std::size_t cellCount = width * height;
    if (travelCost.size() < cellCount) return false;

    const auto indexOf = [width](Sint32 x, Sint32 z) noexcept -> std::size_t {
        return static_cast<std::size_t>(z) * width + static_cast<std::size_t>(x);
    };
    const auto typeAt = [&](Sint32 x, Sint32 z) noexcept -> Uint8 {
        const std::size_t index = indexOf(x, z);
        return index < travelType.size() ? travelType[index] : static_cast<Uint8>(0);
    };

    Float32 minStep = std::numeric_limits<Float32>::infinity();
    for (std::size_t i = 0; i < cellCount; ++i) {
        const Float32 cost = travelCost[i];
        if (cost > 0.0f && cost < minStep) minStep = cost;
    }
    if (!std::isfinite(minStep)) minStep = 0.0f;

    const Float32 kInf = std::numeric_limits<Float32>::infinity();
    std::vector<Float32> gScore(cellCount, kInf);
    std::vector<Sint32> parent(cellCount, -1);
    std::vector<char> closed(cellCount, 0);

    std::priority_queue<PathOpenNode, std::vector<PathOpenNode>, PathOpenWorse> open;
    const Sint32 minStartX = std::min(startRect.x0, startRect.x1);
    const Sint32 maxStartX = std::max(startRect.x0, startRect.x1);
    const Sint32 minStartZ = std::min(startRect.y0, startRect.y1);
    const Sint32 maxStartZ = std::max(startRect.y0, startRect.y1);
    for (Sint32 z = minStartZ; z <= maxStartZ; ++z) {
        if (z < 0 || z >= gridCellsZ) continue;
        for (Sint32 x = minStartX; x <= maxStartX; ++x) {
            if (x < 0 || x >= gridCellsX) continue;
            const std::size_t index = indexOf(x, z);
            gScore[index] = 0.0f;
            const Float32 h = static_cast<Float32>(DistanceToRectangle(x, z, endRect)) * minStep;
            open.push(PathOpenNode{h, 0.0f, x, z});
        }
    }
    if (open.empty()) return false;

    constexpr Sint32 kDx[4] = {1, -1, 0, 0};
    constexpr Sint32 kDz[4] = {0, 0, 1, -1};
    bool reached = false;
    Sint32 goalX = 0;
    Sint32 goalZ = 0;

    while (!open.empty()) {
        const PathOpenNode node = open.top();
        open.pop();
        if (node.x < 0 || node.z < 0 || node.x >= gridCellsX || node.z >= gridCellsZ) continue;
        const std::size_t index = indexOf(node.x, node.z);
        if (closed[index] != 0) continue;
        if (node.g != gScore[index]) continue;
        closed[index] = 1;
        if (InclusiveContains(endRect, node.x, node.z)) {
            reached = true;
            goalX = node.x;
            goalZ = node.z;
            break;
        }
        for (Sint32 dir = 0; dir < 4; ++dir) {
            const Sint32 nx = node.x + kDx[dir];
            const Sint32 nz = node.z + kDz[dir];
            if (nx < 0 || nz < 0 || nx >= gridCellsX || nz >= gridCellsZ) continue;
            const std::size_t nIndex = indexOf(nx, nz);
            if (closed[nIndex] != 0) continue;
            const Float32 step = travelCost[nIndex];
            if (!(step > 0.0f) || !std::isfinite(step)) continue;
            const Float32 nextG = node.g + step;
            if (maxTripLength >= 0.0f && nextG > maxTripLength) continue;
            if (!(nextG < gScore[nIndex])) continue;
            gScore[nIndex] = nextG;
            parent[nIndex] = static_cast<Sint32>(index);
            const Float32 h = static_cast<Float32>(DistanceToRectangle(nx, nz, endRect)) * minStep;
            open.push(PathOpenNode{nextG + h, nextG, nx, nz});
        }
    }
    if (!reached) return false;

    std::vector<Sint32> reversed;
    Sint32 cursor = static_cast<Sint32>(indexOf(goalX, goalZ));
    const Sint32 stepGuard = static_cast<Sint32>(cellCount) + 1;
    for (Sint32 steps = 0; cursor >= 0 && steps < stepGuard; ++steps) {
        reversed.push_back(cursor);
        cursor = parent[static_cast<std::size_t>(cursor)];
    }
    std::reverse(reversed.begin(), reversed.end());
    out.found = true;
    out.tripLength = gScore[indexOf(goalX, goalZ)];
    out.cellX.reserve(reversed.size());
    out.cellZ.reserve(reversed.size());
    out.travelType.reserve(reversed.size());
    for (Sint32 cell : reversed) {
        const Sint32 x = static_cast<Sint32>(static_cast<std::size_t>(cell) % width);
        const Sint32 z = static_cast<Sint32>(static_cast<std::size_t>(cell) / width);
        out.cellX.push_back(x);
        out.cellZ.push_back(z);
        out.travelType.push_back(typeAt(x, z));
    }
    AppendCompressedRoute(out);
    return true;
}

bool cSC4PathFinderEngine::WasTripReversible(
    std::span<const Sint32> cellX,
    std::span<const Sint32> cellZ,
    std::span<const Uint8> travelType,
    std::span<const Uint8> networkFlags,
    Sint32 cityCellsX,
    Sint32 cityCellsZ) noexcept {
    // Exact cSC4PathFinder::WasTripReversible @ 0x1000d3dc0.
    // A trip with fewer than two nodes is not reversible. Each step must stay
    // inside the city, move to a cardinal neighbor, and must not have the
    // one-way bit (0x2 at network tile +0x2c) set on both ends. A travel-type
    // change on a one-way end is not reversible either.
    if (cellX.size() < 2u || cellZ.size() != cellX.size()) return false;
    if (cityCellsX <= 0 || cityCellsZ <= 0) return false;
    const auto typeAt = [&](std::size_t i) noexcept -> Uint8 {
        return i < travelType.size() ? travelType[i] : static_cast<Uint8>(0);
    };
    const auto flagsAt = [&](std::size_t i) noexcept -> Uint8 {
        return i < networkFlags.size() ? networkFlags[i] : static_cast<Uint8>(0);
    };
    for (std::size_t i = 1; i < cellX.size(); ++i) {
        const Sint32 x0 = cellX[i - 1u];
        const Sint32 z0 = cellZ[i - 1u];
        const Sint32 x1 = cellX[i];
        const Sint32 z1 = cellZ[i];
        if (x0 < 0 || z0 < 0 || x1 < 0 || z1 < 0) return false;
        if (x0 >= cityCellsX || z0 >= cityCellsZ || x1 >= cityCellsX || z1 >= cityCellsZ) {
            return false;
        }
        const Sint32 dx = x1 - x0;
        const Sint32 dz = z1 - z0;
        const bool cardinal =
            (dx == 0 && (dz == 1 || dz == -1)) || (dz == 0 && (dx == 1 || dx == -1));
        if (!cardinal) return false;
        const bool oneWayA = (flagsAt(i - 1u) & 0x2u) != 0u;
        const bool oneWayB = (flagsAt(i) & 0x2u) != 0u;
        if (oneWayA && oneWayB) return false;
        if (typeAt(i - 1u) != typeAt(i) && (oneWayA || oneWayB)) return false;
    }
    return true;
}

Float32 cSC4PathFinderEngine::GetTripLengthScaledForDisplay(
    Float32 tripLength, Float32 displayScale) noexcept {
    // Exact cSC4PathFinder::GetTripLengthScaledForDisplay @ 0x1000d4326:
    // displayScale (traffic simulator vtable +0x78) times the trip length at +0x6c.
    return tripLength * displayScale;
}

cSC4VertexHeightSolver::Vertex& cSC4VertexHeightSolver::Ensure(Sint32 vertex) {
    const auto it = mVertices.find(vertex);
    if (it != mVertices.end()) {
        if (it->second.parent < 0) it->second.parent = vertex;
        return it->second;
    }
    Vertex created;
    created.parent = vertex;
    return mVertices.emplace(vertex, created).first->second;
}

Sint32 cSC4VertexHeightSolver::FindRoot(Sint32 vertex) {
    Sint32 root = vertex;
    const int hopLimit = static_cast<int>(mVertices.size()) + 1;
    int hops = 0;
    while (mVertices.at(root).parent != root) {
        root = mVertices.at(root).parent;
        if (++hops > hopLimit) return vertex;
    }
    Sint32 cursor = vertex;
    while (cursor != root) {
        const Sint32 next = mVertices.at(cursor).parent;
        mVertices.at(cursor).parent = root;
        cursor = next;
    }
    return root;
}

bool cSC4VertexHeightSolver::ApplyRange(
    Sint32 vertex, Float32 minHeight, Float32 maxHeight) {
    // Exact range intersection @ InsertRangeConstraint 0x1001bd6b2 and
    // IntersectVertexRange 0x1001bda28. A width below kHeightEpsilon collapses
    // to a point. An inverted range is a contradiction.
    if (!std::isfinite(minHeight) || !std::isfinite(maxHeight) ||
        maxHeight < minHeight - kHeightEpsilon) {
        mConsistent = false;
        return false;
    }
    if (maxHeight < minHeight) maxHeight = minHeight;
    Ensure(vertex);
    const Sint32 root = FindRoot(vertex);
    const Vertex& rootVertex = mVertices.at(root);
    Float32 lo = std::max(rootVertex.minH, minHeight);
    Float32 hi = std::min(rootVertex.maxH, maxHeight);
    if (hi < lo - kHeightEpsilon) {
        mConsistent = false;
        return false;
    }
    if (hi < lo) hi = lo;
    for (auto& entry : mVertices) {
        if (FindRoot(entry.first) != root) continue;
        entry.second.minH = lo;
        entry.second.maxH = hi;
        entry.second.solved = false;
    }
    return true;
}

bool cSC4VertexHeightSolver::InsertFixedVertex(Sint32 vertex, Float32 height) {
    // Exact cSC4VertexHtConstraintSatisfier::InsertFixedVertex @ 0x1001bd67c:
    // AddVertexToSolve then InsertRangeConstraint(height, height).
    if (!std::isfinite(height)) {
        mConsistent = false;
        return false;
    }
    return InsertRangeConstraint(vertex, height, height);
}

bool cSC4VertexHeightSolver::InsertRangeConstraint(
    Sint32 vertex, Float32 minHeight, Float32 maxHeight) {
    return ApplyRange(vertex, minHeight, maxHeight);
}

bool cSC4VertexHeightSolver::InsertEqualityConstraint(Sint32 vertexA, Sint32 vertexB) {
    // Exact cSC4VertexHtConstraintSatisfier::InsertEqualityConstraint @ 0x1001bd178.
    // Unions the two equality sets (flag 0x8) and intersects their ranges.
    Ensure(vertexA);
    Ensure(vertexB);
    if (!mConsistent) return false;
    const Sint32 rootA = FindRoot(vertexA);
    const Sint32 rootB = FindRoot(vertexB);
    const Vertex& a = mVertices.at(rootA);
    const Vertex& b = mVertices.at(rootB);
    Float32 lo = std::max(a.minH, b.minH);
    Float32 hi = std::min(a.maxH, b.maxH);
    if (hi < lo - kHeightEpsilon) {
        mConsistent = false;
        return false;
    }
    if (hi < lo) hi = lo;
    if (rootA != rootB) {
        if (rootA < rootB) mVertices.at(rootB).parent = rootA;
        else mVertices.at(rootA).parent = rootB;
    }
    const Sint32 root = FindRoot(vertexA);
    for (auto& entry : mVertices) {
        if (FindRoot(entry.first) != root) continue;
        entry.second.minH = lo;
        entry.second.maxH = hi;
        entry.second.solved = false;
    }
    return true;
}

void cSC4VertexHeightSolver::InsertSlopeConstraint(
    Sint32 vertexA, Sint32 vertexB, Float32 maxAbsDelta) {
    // Exact cSC4VertexHtConstraintSatisfier::InsertSlopeConstraint @ 0x1001bd80a.
    // |height(A) - height(B)| <= delta. A duplicate pair keeps the tighter delta.
    if (vertexA == vertexB) return;
    if (!std::isfinite(maxAbsDelta)) return;
    Ensure(vertexA);
    Ensure(vertexB);
    const Float32 delta = std::fabs(maxAbsDelta);
    for (SlopeLink& link : mSlopes) {
        const bool same =
            (link.a == vertexA && link.b == vertexB) ||
            (link.a == vertexB && link.b == vertexA);
        if (!same) continue;
        if (delta < link.maxDelta) link.maxDelta = delta;
        return;
    }
    SlopeLink link;
    link.a = vertexA;
    link.b = vertexB;
    link.maxDelta = delta;
    mSlopes.push_back(link);
}

bool cSC4VertexHeightSolver::AssignSolved(Sint32 vertex, Float32 height) {
    const Sint32 root = FindRoot(vertex);
    for (auto& entry : mVertices) {
        if (FindRoot(entry.first) != root) continue;
        if (height < entry.second.minH - kHeightEpsilon ||
            height > entry.second.maxH + kHeightEpsilon) {
            return false;
        }
        Float32 solved = height;
        if (solved < entry.second.minH) solved = entry.second.minH;
        if (solved > entry.second.maxH) solved = entry.second.maxH;
        entry.second.minH = solved;
        entry.second.maxH = solved;
        entry.second.solution = solved;
        entry.second.solved = true;
    }
    return true;
}

bool cSC4VertexHeightSolver::PropagateSlopes() {
    // Exact UpdateSlopeConstraints @ 0x1001be488: a solved (or bounded) vertex
    // intersects its neighbor with [height - delta, height + delta].
    if (mSlopes.empty()) return true;
    const int hopLimit = static_cast<int>(mSlopes.size() * (mVertices.size() + 1u) + 1u);
    bool changed = true;
    for (int iter = 0; changed; ++iter) {
        if (iter > hopLimit) return false;
        changed = false;
        for (const SlopeLink& link : mSlopes) {
            const auto tighten = [&](Sint32 src, Sint32 dst) -> bool {
                const Sint32 srcRoot = FindRoot(src);
                const Sint32 dstRoot = FindRoot(dst);
                if (srcRoot == dstRoot) return true;
                const Vertex& from = mVertices.at(srcRoot);
                const Vertex& to = mVertices.at(dstRoot);
                const Float32 loBound = from.minH - link.maxDelta;
                const Float32 hiBound = from.maxH + link.maxDelta;
                if (to.solved) {
                    if (to.solution < loBound - kHeightEpsilon ||
                        to.solution > hiBound + kHeightEpsilon) {
                        return false;
                    }
                    return true;
                }
                const Float32 lo = std::max(to.minH, loBound);
                Float32 hi = std::min(to.maxH, hiBound);
                if (hi < lo - kHeightEpsilon) return false;
                if (hi < lo) hi = lo;
                if (lo == to.minH && hi == to.maxH) return true;
                changed = true;
                for (auto& entry : mVertices) {
                    if (FindRoot(entry.first) != dstRoot) continue;
                    entry.second.minH = lo;
                    entry.second.maxH = hi;
                }
                return true;
            };
            if (!tighten(link.a, link.b) || !tighten(link.b, link.a)) return false;
        }
    }
    return true;
}

Float32 cSC4VertexHeightSolver::PickHeight(const Vertex& vertex) noexcept {
    // Exact PickASolution @ 0x1001bdbd8: a collapsed range uses that point.
    // A finite span uses its midpoint. A one-sided bound uses the finite edge.
    const bool loOk = std::isfinite(vertex.minH);
    const bool hiOk = std::isfinite(vertex.maxH);
    if (loOk && hiOk) {
        if (vertex.maxH - vertex.minH <= kHeightEpsilon) return vertex.minH;
        return 0.5f * (vertex.minH + vertex.maxH);
    }
    if (loOk) return vertex.minH;
    if (hiOk) return vertex.maxH;
    return 0.0f;
}

bool cSC4VertexHeightSolver::Solve() {
    // Exact cSC4VertexHtConstraintSatisfier::Solve @ 0x1001bdebe.
    // Initial propagation, then repeatedly PickASolution on the tightest
    // range (lowest id wins ties), MarkSolution, and propagate.
    if (!mConsistent) return false;
    for (auto& entry : mVertices) entry.second.solved = false;
    if (!PropagateSlopes()) {
        mConsistent = false;
        return false;
    }
    const int stepLimit = static_cast<int>(mVertices.size()) + 1;
    for (int step = 0; step < stepLimit; ++step) {
        bool have = false;
        Sint32 bestId = 0;
        Float32 bestWidth = 0.0f;
        for (const auto& entry : mVertices) {
            if (entry.second.solved) continue;
            if (entry.second.maxH < entry.second.minH - kHeightEpsilon) {
                mConsistent = false;
                return false;
            }
            Float32 width = entry.second.maxH - entry.second.minH;
            if (!std::isfinite(width) || width < 0.0f) {
                width = std::numeric_limits<Float32>::infinity();
            }
            if (!have || width < bestWidth || (width == bestWidth && entry.first < bestId)) {
                have = true;
                bestId = entry.first;
                bestWidth = width;
            }
        }
        if (!have) return true;
        const Float32 height = PickHeight(mVertices.at(bestId));
        if (!AssignSolved(bestId, height) || !PropagateSlopes()) {
            mConsistent = false;
            return false;
        }
    }
    mConsistent = false;
    return false;
}

bool cSC4VertexHeightSolver::GetHeight(Sint32 vertex, Float32& outHeight) const noexcept {
    // Exact cSC4VertexHtConstraintSatisfier::GetHeight @ 0x1001bd0ea.
    const auto it = mVertices.find(vertex);
    if (it == mVertices.end() || !it->second.solved) return false;
    outHeight = it->second.solution;
    return true;
}

Float32 ComputeTrafficTravelTimeRatio(
    Sint32 cellX,
    Sint32 cellZ,
    Uint8 travelType,
    bool travelTypeAffectedByTraffic,
    bool cellHasNetwork,
    Float32 congestionTimesSpeed,
    std::span<const Float32> curveInput,
    std::span<const Float32> curveOutput) noexcept {
    // Exact cSC4TrafficSimulator::GetTravelTimeRatio @ 0x100089f14.
    // mbTravelTypeAffectedByTraffic gates the lookup. With no network tile the
    // binary returns the rodata constant 1. Otherwise it evaluates
    // mCongestionToInverseSpeedCurve at volume * speed.
    if (cellX < 0 || cellZ < 0 || travelType > 9u ||
        !travelTypeAffectedByTraffic || !cellHasNetwork ||
        !std::isfinite(congestionTimesSpeed)) {
        return 1.0f;
    }
    return SampleResponseCurve(congestionTimesSpeed, curveInput, curveOutput, 1.0f);
}

Float32 ComputeTransitSwitchEntryCost(
    Float32 accumulatedCost,
    Float32 entryBase,
    Float32 divisor,
    Float32 globalScale,
    Float32 minimumRatio,
    Float32 belowThresholdCost,
    std::span<const Float32> curveInput,
    std::span<const Float32> curveOutput) noexcept {
    // Exact cSC4TrafficSimulator::TransitSwitchEntryCost @ 0x10008c282.
    // ratio = switch+0x8 / switch+0x10. At or below the threshold the function
    // returns the fallback constant. Above it: curve(ratio) * switch+0xc * this+0x8c8.
    if (!(divisor > 0.0f) || !std::isfinite(accumulatedCost) || !std::isfinite(divisor)) {
        return belowThresholdCost;
    }
    const Float32 ratio = accumulatedCost / divisor;
    if (!std::isfinite(ratio) || !(ratio > minimumRatio)) return belowThresholdCost;
    const Float32 curved = SampleResponseCurve(ratio, curveInput, curveOutput, ratio);
    return curved * entryBase * globalScale;
}

} // namespace opensc4
