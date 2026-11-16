#include "opensc4/enhanced/enhanced_engine.h"
#include "opensc4/city.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <limits>

namespace opensc4 {

namespace {

constexpr std::array<EnhancedModDescriptor, kEnhancedModCount> kModCatalog = {{
    {
        EnhancedModId::NAMTrafficSimulator,
        "nam_traffic",
        "Network Addon Mod (NAM) Traffic Simulator Z",
        "Traffic & Transit",
        "Precomputes a 256-entry congestion LUT for zero-alloc A* travel-time routing, increases network capacities 2.5x, and extends max commute distance 10x.",
        true
    },
    {
        EnhancedModId::IHMissingJobsFix,
        "ih_missing_jobs_fix",
        "I-HT High-Tech $$$ Jobs Bugfix",
        "Bugfix",
        "Fixes the vanilla Maxis rounding bug where High-Tech Industry (0x4400) failed to employ R$$$ wealthy Sims (restores 10% R$ / 75% R$$ / 15% R$$$).",
        true
    },
    {
        EnhancedModId::BudgetTaxTableBugfix,
        "budget_tax_offset_fix",
        "Budget Co$$$ Tax Multiplier Bugfix (@ 0x10021ea9e)",
        "Bugfix",
        "Fixes the binary bug in cSC4BudgetSimulator::ReadSimulatorTunableValues where offset +0x844 was written twice and +0x83c (index 5) was left at 0.0f.",
        true
    },
    {
        EnhancedModId::OperaHouseAndCivicCapFix,
        "opera_house_fix",
        "Opera House & Civic Capacity Overflow Fix",
        "Bugfix",
        "Prevents reward/civic buildings from wrapping 16-bit capacity or triggering phantom education/health funding strikes in large cities.",
        true
    },
    {
        EnhancedModId::CrimeInMegaLotsFix,
        "crime_mega_lots_fix",
        "Crime in Mega-Lots Foot-Patrol Fix",
        "Bugfix",
        "Uses the O(1) spatial index to propagate perimeter-road police protection across interior cells of >4x4 mega-lots (Airports, Universities, Zoos, Country Clubs).",
        true
    },
    {
        EnhancedModId::PropPoxSaveGuard,
        "prop_pox_guard",
        "Prop Pox DBPF Save Corruption Guard",
        "Bugfix",
        "Deduplicates subfile resource keys and validates record CRCs when saving .sc4 archives to prevent Prop Pox corruption.",
        true
    },
    {
        EnhancedModId::LessAbandonmentMod,
        "less_abandonment",
        "Less Abandonment & Dilapidation Hysteresis",
        "Bugfix",
        "Extends the abandonment cooldown gate from 60 days to 180 days to prevent rapid R$$$/Co$$$ wealth-cycling dilapidation.",
        true
    },
    {
        EnhancedModId::ExtraCheatsBuiltIn,
        "extra_cheats",
        "Maxis ExtraCheats.dll Built-In",
        "QoL & Sandbox",
        "Enables developer cheat commands: moolah <funds>, weaknesspays (+1,000), fightthepower, howdryiam.",
        true
    },
    {
        EnhancedModId::CAMExtendedGrowthStages,
        "cam_extended_stages",
        "Colossus Addon Mod (CAM) Stages 1-15",
        "Overhaul",
        "Extends RCI growth stages from Stage 8 up to Stage 15 and raises monthly construction resource cap by 1.5x (3,000).",
        false
    },
    {
        EnhancedModId::CivicRadiusDoubler,
        "civic_radius_doubler",
        "Civic & Utility Service Radius Doubler (2x)",
        "Overhaul",
        "Doubles the effective coverage and relay radius of Police, Fire, Water Pumps, and Power distribution.",
        false
    },
    {
        EnhancedModId::IndustryJobQuadrupler,
        "industry_quadrupler",
        "Industrial & Commercial Job Multiplier (4x/2x)",
        "Overhaul",
        "Uses a branchless zone LUT to quadruple Industrial job capacity and double Commercial job capacity per lot.",
        false
    },
}};

} // namespace

std::span<const EnhancedModDescriptor> GetEnhancedModCatalog() noexcept {
    return kModCatalog;
}

const EnhancedModDescriptor* FindEnhancedModBySlug(std::string_view slug) noexcept {
    for (const auto& mod : kModCatalog) {
        if (mod.slug == slug) {
            return &mod;
        }
    }
    return nullptr;
}

// ============================================================================
// 1. cEnhancedSpatialIndex
// ============================================================================
void cEnhancedSpatialIndex::Rebuild(
    int cellsX, int cellsZ,
    std::span<const SC4LotRecord> lots,
    std::span<const SC4BuildingRecord> buildings) {
    mWidth = std::max(0, cellsX);
    mHeight = std::max(0, cellsZ);
    const std::size_t total =
        static_cast<std::size_t>(mWidth) * static_cast<std::size_t>(mHeight);
    mCellToLot.assign(total, -1);
    mCellToBuilding.assign(total, -1);
    mLotHasBuilding.assign(lots.size(), 0u);

    if (mWidth == 0 || mHeight == 0) {
        return;
    }

    for (std::size_t i = 0; i < lots.size(); ++i) {
        const auto& lot = lots[i];
        const int x0 = std::clamp<int>(lot.minX, 0, mWidth - 1);
        const int x1 = std::clamp<int>(lot.maxX, 0, mWidth - 1);
        const int z0 = std::clamp<int>(lot.minZ, 0, mHeight - 1);
        const int z1 = std::clamp<int>(lot.maxZ, 0, mHeight - 1);
        for (int z = z0; z <= z1; ++z) {
            const std::size_t rowOff = static_cast<std::size_t>(z) * mWidth;
            for (int x = x0; x <= x1; ++x) {
                mCellToLot[rowOff + x] = static_cast<Sint32>(i);
            }
        }
    }

    for (std::size_t b = 0; b < buildings.size(); ++b) {
        const auto& bld = buildings[b];
        const int cx = std::clamp(
            static_cast<int>((bld.minX + bld.maxX) * 0.5f / 16.0f), 0, mWidth - 1);
        const int cz = std::clamp(
            static_cast<int>((bld.minZ + bld.maxZ) * 0.5f / 16.0f), 0, mHeight - 1);
        const Sint32 centerLot = mCellToLot[static_cast<std::size_t>(cz) * mWidth + cx];
        if (centerLot >= 0 && static_cast<std::size_t>(centerLot) < mLotHasBuilding.size()) {
            mLotHasBuilding[static_cast<std::size_t>(centerLot)] = 1u;
        }

        const int bx0 = std::clamp(
            static_cast<int>(std::floor(bld.minX / 16.0f)), 0, mWidth - 1);
        const int bz0 = std::clamp(
            static_cast<int>(std::floor(bld.minZ / 16.0f)), 0, mHeight - 1);
        const int bx1 = std::clamp(
            static_cast<int>(std::floor((bld.maxX - 0.1f) / 16.0f)), 0, mWidth - 1);
        const int bz1 = std::clamp(
            static_cast<int>(std::floor((bld.maxZ - 0.1f) / 16.0f)), 0, mHeight - 1);
        for (int z = bz0; z <= bz1; ++z) {
            const std::size_t rowOff = static_cast<std::size_t>(z) * mWidth;
            for (int x = bx0; x <= bx1; ++x) {
                mCellToBuilding[rowOff + x] = static_cast<Sint32>(b);
                const Sint32 lotIdx = mCellToLot[rowOff + x];
                if (lotIdx >= 0 && static_cast<std::size_t>(lotIdx) < mLotHasBuilding.size()) {
                    mLotHasBuilding[static_cast<std::size_t>(lotIdx)] = 1u;
                }
            }
        }
    }
}

Sint32 cEnhancedSpatialIndex::LotIndexAtCell(int cx, int cz) const noexcept {
    if (cx < 0 || cx >= mWidth || cz < 0 || cz >= mHeight) {
        return -1;
    }
    return mCellToLot[static_cast<std::size_t>(cz) * mWidth + cx];
}

Sint32 cEnhancedSpatialIndex::BuildingIndexAtCell(int cx, int cz) const noexcept {
    if (cx < 0 || cx >= mWidth || cz < 0 || cz >= mHeight) {
        return -1;
    }
    return mCellToBuilding[static_cast<std::size_t>(cz) * mWidth + cx];
}

bool cEnhancedSpatialIndex::LotHasBuildingFast(std::size_t lotIndex) const noexcept {
    return lotIndex < mLotHasBuilding.size() && mLotHasBuilding[lotIndex] != 0u;
}

bool cEnhancedSpatialIndex::LotBoundsHaveBuilding(
    Uint8 minX, Uint8 minZ, Uint8 maxX, Uint8 maxZ) const noexcept {
    if (mWidth <= 0 || mHeight <= 0) return false;
    const int x0 = std::clamp<int>(minX, 0, mWidth - 1);
    const int z0 = std::clamp<int>(minZ, 0, mHeight - 1);
    const Sint32 lotIdx = mCellToLot[static_cast<std::size_t>(z0) * mWidth + x0];
    if (lotIdx >= 0 && static_cast<std::size_t>(lotIdx) < mLotHasBuilding.size()) {
        return mLotHasBuilding[static_cast<std::size_t>(lotIdx)] != 0u;
    }
    const int x1 = std::clamp<int>(maxX, 0, mWidth - 1);
    const int z1 = std::clamp<int>(maxZ, 0, mHeight - 1);
    for (int z = z0; z <= z1; ++z) {
        const std::size_t rowOff = static_cast<std::size_t>(z) * mWidth;
        for (int x = x0; x <= x1; ++x) {
            if (mCellToBuilding[rowOff + x] >= 0) return true;
        }
    }
    return false;
}

bool cEnhancedSpatialIndex::IsMegaLotCell(
    int cx, int cz, std::span<const SC4LotRecord> lots) const noexcept {
    const Sint32 lotIdx = LotIndexAtCell(cx, cz);
    if (lotIdx < 0 || static_cast<std::size_t>(lotIdx) >= lots.size()) {
        return false;
    }
    const auto& lot = lots[static_cast<std::size_t>(lotIdx)];
    return lot.sizeX > 4u || lot.sizeZ > 4u;
}

// ============================================================================
// 2. cEnhancedPathFinderScratch (Zero-Allocation Generation-Counter A*)
// ============================================================================
void cEnhancedPathFinderScratch::EnsureCapacity(std::size_t cellCount) {
    if (mVisitedGen.size() < cellCount) {
        mVisitedGen.assign(cellCount, 0u);
        mClosedGen.assign(cellCount, 0u);
        mGScore.assign(cellCount, 0.0f);
        mParent.assign(cellCount, -1);
    }
    if (mCurrentGen >= 0xFFFFFFFEu) {
        std::fill(mVisitedGen.begin(), mVisitedGen.end(), 0u);
        std::fill(mClosedGen.begin(), mClosedGen.end(), 0u);
        mCurrentGen = 1u;
    } else {
        ++mCurrentGen;
    }
}

bool cEnhancedPathFinderScratch::FindPathFast(
    const SC4Rect<Sint32>& startRect,
    const SC4Rect<Sint32>& endRect,
    Sint32 gridCellsX,
    Sint32 gridCellsZ,
    std::span<const Float32> baseTravelCost,
    std::span<const Uint8> travelType,
    std::span<const Uint8> congestionByCell,
    Float32 minStepHint,
    const EnhancedModConfig& modConfig,
    SC4PathFindResult& outResult) {
    outResult.found = false;
    outResult.tripLength = 0.0f;
    outResult.cellX.clear();
    outResult.cellZ.clear();
    outResult.travelType.clear();
    outResult.compressedRoute.clear();

    if (gridCellsX <= 0 || gridCellsZ <= 0) return false;
    const std::size_t width = static_cast<std::size_t>(gridCellsX);
    const std::size_t height = static_cast<std::size_t>(gridCellsZ);
    const std::size_t cellCount = width * height;
    if (baseTravelCost.size() < cellCount) return false;

    EnsureCapacity(cellCount);
    const Uint32 gen = mCurrentGen;

    const Float32 minStep = (minStepHint > 0.0f && std::isfinite(minStepHint))
        ? minStepHint
        : 0.45f;
    const Float32 maxTripLength = modConfig.MaxCommuteTripLength();
    const bool namActive = modConfig.IsModActive(EnhancedModId::NAMTrafficSimulator);

    auto heapLess = [](const OpenNode& a, const OpenNode& b) noexcept {
        if (a.f != b.f) return a.f > b.f;
        if (a.z != b.z) return a.z > b.z;
        return a.x > b.x;
    };

    mOpenHeap.clear();
    const Sint32 minStartX = std::max<Sint32>(0, std::min(startRect.x0, startRect.x1));
    const Sint32 maxStartX = std::min<Sint32>(gridCellsX - 1, std::max(startRect.x0, startRect.x1));
    const Sint32 minStartZ = std::max<Sint32>(0, std::min(startRect.y0, startRect.y1));
    const Sint32 maxStartZ = std::min<Sint32>(gridCellsZ - 1, std::max(startRect.y0, startRect.y1));

    for (Sint32 z = minStartZ; z <= maxStartZ; ++z) {
        for (Sint32 x = minStartX; x <= maxStartX; ++x) {
            const std::size_t idx = static_cast<std::size_t>(z) * width + static_cast<std::size_t>(x);
            mVisitedGen[idx] = gen;
            mGScore[idx] = 0.0f;
            mParent[idx] = -1;
            const Float32 h =
                static_cast<Float32>(cSC4PathFinderEngine::DistanceToRectangle(x, z, endRect)) *
                minStep;
            mOpenHeap.push_back(OpenNode{h, 0.0f, x, z});
            std::push_heap(mOpenHeap.begin(), mOpenHeap.end(), heapLess);
        }
    }
    if (mOpenHeap.empty()) return false;

    const Sint32 minEndX = std::min(endRect.x0, endRect.x1);
    const Sint32 maxEndX = std::max(endRect.x0, endRect.x1);
    const Sint32 minEndZ = std::min(endRect.y0, endRect.y1);
    const Sint32 maxEndZ = std::max(endRect.y0, endRect.y1);

    constexpr Sint32 kDx[4] = {1, -1, 0, 0};
    constexpr Sint32 kDz[4] = {0, 0, 1, -1};
    bool reached = false;
    Sint32 goalX = 0;
    Sint32 goalZ = 0;

    while (!mOpenHeap.empty()) {
        std::pop_heap(mOpenHeap.begin(), mOpenHeap.end(), heapLess);
        const OpenNode node = mOpenHeap.back();
        mOpenHeap.pop_back();

        const std::size_t idx =
            static_cast<std::size_t>(node.z) * width + static_cast<std::size_t>(node.x);
        if (mClosedGen[idx] == gen) continue;
        if (mVisitedGen[idx] == gen && node.g != mGScore[idx]) continue;
        mClosedGen[idx] = gen;

        if (node.x >= minEndX && node.x <= maxEndX &&
            node.z >= minEndZ && node.z <= maxEndZ) {
            reached = true;
            goalX = node.x;
            goalZ = node.z;
            break;
        }

        for (Sint32 dir = 0; dir < 4; ++dir) {
            const Sint32 nx = node.x + kDx[dir];
            const Sint32 nz = node.z + kDz[dir];
            if (nx < 0 || nz < 0 || nx >= gridCellsX || nz >= gridCellsZ) continue;
            const std::size_t nIdx =
                static_cast<std::size_t>(nz) * width + static_cast<std::size_t>(nx);
            if (mClosedGen[nIdx] == gen) continue;

            Float32 step = baseTravelCost[nIdx];
            if (!(step > 0.0f) || !std::isfinite(step)) continue;
            if (namActive && nIdx < congestionByCell.size()) {
                step *= modConfig.namCongestionPenaltyLUT[congestionByCell[nIdx]];
            }

            const Float32 nextG = node.g + step;
            if (maxTripLength >= 0.0f && nextG > maxTripLength) continue;
            if (mVisitedGen[nIdx] == gen && !(nextG < mGScore[nIdx])) continue;

            mVisitedGen[nIdx] = gen;
            mGScore[nIdx] = nextG;
            mParent[nIdx] = static_cast<Sint32>(idx);
            const Float32 h =
                static_cast<Float32>(cSC4PathFinderEngine::DistanceToRectangle(nx, nz, endRect)) *
                minStep;
            mOpenHeap.push_back(OpenNode{nextG + h, nextG, nx, nz});
            std::push_heap(mOpenHeap.begin(), mOpenHeap.end(), heapLess);
        }
    }

    if (!reached) return false;

    mReversedPath.clear();
    Sint32 cursor = static_cast<Sint32>(
        static_cast<std::size_t>(goalZ) * width + static_cast<std::size_t>(goalX));
    const Sint32 stepGuard = static_cast<Sint32>(cellCount) + 1;
    for (Sint32 steps = 0; cursor >= 0 && steps < stepGuard; ++steps) {
        mReversedPath.push_back(cursor);
        cursor = mParent[static_cast<std::size_t>(cursor)];
    }
    std::reverse(mReversedPath.begin(), mReversedPath.end());

    outResult.found = true;
    outResult.tripLength =
        mGScore[static_cast<std::size_t>(goalZ) * width + static_cast<std::size_t>(goalX)];
    outResult.cellX.reserve(mReversedPath.size());
    outResult.cellZ.reserve(mReversedPath.size());
    outResult.travelType.reserve(mReversedPath.size());

    for (Sint32 cell : mReversedPath) {
        const Sint32 x = static_cast<Sint32>(static_cast<std::size_t>(cell) % width);
        const Sint32 z = static_cast<Sint32>(static_cast<std::size_t>(cell) / width);
        const std::size_t cIdx = static_cast<std::size_t>(cell);
        outResult.cellX.push_back(x);
        outResult.cellZ.push_back(z);
        outResult.travelType.push_back(
            cIdx < travelType.size() ? travelType[cIdx] : static_cast<Uint8>(1u));
    }

    // Encode GetCompressedRoute (travelType << 2 | dir, length)
    for (std::size_t i = 1; i < outResult.cellX.size(); ++i) {
        const Sint32 dx = outResult.cellX[i] - outResult.cellX[i - 1u];
        const Sint32 dz = outResult.cellZ[i] - outResult.cellZ[i - 1u];
        Uint8 dir = 0u;
        if (dx > 0) dir = 0u;
        else if (dx < 0) dir = 1u;
        else if (dz > 0) dir = 2u;
        else dir = 3u;
        const Uint8 header = static_cast<Uint8>((outResult.travelType[i] << 2u) | (dir & 0x3u));
        if (outResult.compressedRoute.size() >= 2u &&
            outResult.compressedRoute[outResult.compressedRoute.size() - 2u] == header &&
            outResult.compressedRoute.back() < 255u) {
            ++outResult.compressedRoute.back();
        } else {
            outResult.compressedRoute.push_back(header);
            outResult.compressedRoute.push_back(1u);
        }
    }
    return true;
}

// ============================================================================
// 3. Optimized Mod Patchers
// ============================================================================
void ApplyEnhancedBudgetTuningFixes(
    BudgetSimulatorTuning& inOutTuning,
    const EnhancedModConfig& config) noexcept {
    if (config.IsModActive(EnhancedModId::BudgetTaxTableBugfix)) {
        if (inOutTuning.taxTypeMultipliers[5] == 0.0f) {
            inOutTuning.taxTypeMultipliers[5] =
                inOutTuning.taxTypeMultipliers[4] > 0.0f
                    ? inOutTuning.taxTypeMultipliers[4]
                    : 1.0f;
        }
    } else {
        // Authentic vanilla quirk: +0x83c (index 5) is left at 0.0f
        inOutTuning.taxTypeMultipliers[5] = 0.0f;
    }
}

std::array<Float32, 3> ComputeHighTechWorkforceDemand(
    Float32 activeHighTechJobs,
    const EnhancedModConfig& config) noexcept {
    if (activeHighTechJobs <= 0.0f) {
        return {0.0f, 0.0f, 0.0f};
    }
    if (config.IsModActive(EnhancedModId::IHMissingJobsFix)) {
        return {
            activeHighTechJobs * 0.10f,
            activeHighTechJobs * 0.75f,
            activeHighTechJobs * 0.15f
        };
    }
    return {
        activeHighTechJobs * 0.10f,
        activeHighTechJobs * 0.80f,
        0.0f
    };
}

Uint8 ComputeLotCellCrime(
    Uint8 baseCrime,
    Uint8 lotSizeX,
    Uint8 lotSizeZ,
    Uint8 policeCoverageAtLotEdge,
    const EnhancedModConfig& config) noexcept {
    const bool isMegaLot = (lotSizeX > 4u || lotSizeZ > 4u);
    if (isMegaLot && !config.IsModActive(EnhancedModId::CrimeInMegaLotsFix)) {
        return baseCrime;
    }
    const int suppressed =
        static_cast<int>(baseCrime) - static_cast<int>(policeCoverageAtLotEdge) / 3;
    return static_cast<Uint8>(std::clamp(suppressed, 0, 255));
}

Float32 ComputeCivicOccupantGrade(
    Uint32 currentUsage,
    Uint32 baseCapacity,
    Float32 fundingPercent,
    const EnhancedModConfig& config) noexcept {
    if (baseCapacity == 0u) {
        return 1.0f;
    }
    if (config.IsModActive(EnhancedModId::OperaHouseAndCivicCapFix)) {
        // Prevent 16-bit capacity wrap-around and scale reward capacity gracefully
        const Float32 safeFunding = std::clamp(fundingPercent, 0.25f, 1.50f);
        const Float32 effectiveCap =
            std::max<Float32>(static_cast<Float32>(baseCapacity) * safeFunding,
                              static_cast<Float32>(currentUsage));
        return std::clamp(effectiveCap / std::max<Float32>(1.0f, static_cast<Float32>(currentUsage)),
                          0.85f, 1.20f);
    }
    // Authentic vanilla formula: when usage exceeds funded capacity, grade drops sharply
    const Float32 fundedCap = std::max(1.0f, static_cast<Float32>(baseCapacity) * fundingPercent);
    if (static_cast<Float32>(currentUsage) > fundedCap) {
        return std::clamp(fundedCap / static_cast<Float32>(currentUsage), 0.10f, 1.0f);
    }
    return std::clamp(fundingPercent, 0.0f, 1.20f);
}

bool ExecuteEnhancedCheatCommand(
    cSC4City& city,
    std::string_view commandLine,
    const EnhancedModConfig& config) {
    if (!config.IsModActive(EnhancedModId::ExtraCheatsBuiltIn)) {
        return false;
    }
    while (!commandLine.empty() && std::isspace(static_cast<unsigned char>(commandLine.front()))) {
        commandLine.remove_prefix(1);
    }
    if (commandLine.empty()) {
        return false;
    }

    auto startsWithIgnoreCase = [](std::string_view s, std::string_view prefix) {
        if (s.size() < prefix.size()) return false;
        for (std::size_t i = 0; i < prefix.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(s[i])) !=
                std::tolower(static_cast<unsigned char>(prefix[i]))) {
                return false;
            }
        }
        return true;
    };

    if (startsWithIgnoreCase(commandLine, "weaknesspays")) {
        city.SetTotalFunds(city.TreasuryFunds() + 1000);
        return true;
    }
    if (startsWithIgnoreCase(commandLine, "moolah")) {
        std::string_view rest = commandLine.substr(6);
        while (!rest.empty() && std::isspace(static_cast<unsigned char>(rest.front()))) {
            rest.remove_prefix(1);
        }
        if (rest.empty()) return false;
        std::string numStr(rest);
        char* endPtr = nullptr;
        const long long targetFunds = std::strtoll(numStr.c_str(), &endPtr, 10);
        if (endPtr == numStr.c_str() || targetFunds < 0) {
            return false;
        }
        city.SetTotalFunds(static_cast<Sint64>(targetFunds));
        return true;
    }
    if (startsWithIgnoreCase(commandLine, "fightthepower")) {
        city.SetFreePower(!city.FreePower());
        return true;
    }
    if (startsWithIgnoreCase(commandLine, "howdryiam")) {
        city.SetFreeWater(!city.FreeWater());
        return true;
    }
    return false;
}

} // namespace opensc4
