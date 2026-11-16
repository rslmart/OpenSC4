#pragma once

// ============================================================================
// OpenSC4 Enhanced Edition Engine (opensc4_enhanced_core)
//
// Provides:
//   1. Modern C++20 Data-Oriented Performance Subsystems:
//      - cEnhancedSpatialIndex: O(1) contiguous cell-to-lot, cell-to-building,
//        and per-lot occupancy lookup tables (replaces O(N*M) linear scans).
//      - cEnhancedPathFinderScratch: Zero-allocation generation-counter A*
//        pathfinder scratchpad with a precomputed 256-entry NAM congestion LUT.
//   2. 11 Optimized, Individually Deselectable Community Mods & Bugfixes:
//      - Every mod is designed natively for the Enhanced architecture (using
//        precomputed LUTs, O(1) spatial queries, and zero-alloc scratchpads).
//      - Every mod can be toggled on or off at runtime. When EngineEdition is
//        Authentic (or all mods are deselected), 100% of simulation formulas
//        revert to exact SimCity 4 Deluxe vanilla behavior.
// ============================================================================

#include "opensc4/gzcom.h"
#include "opensc4/budget_simulator.h"
#include "opensc4/civic_simulators.h"
#include "opensc4/tract_developer.h"

#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace opensc4 {

class cSC4City;
struct SC4LotRecord;
struct SC4BuildingRecord;
struct SC4NetworkRecord;

enum class EngineEdition : Uint8 {
    Authentic = 0, // 100% 1:1 original SimCity 4 Deluxe binary behavior
    Enhanced  = 1  // Modernized performance + toggleable community mods/fixes
};

enum class EnhancedModId : Uint8 {
    NAMTrafficSimulator      = 0,  // Network Addon Mod (NAM) Traffic Simulator Z
    IHMissingJobsFix         = 1,  // High-Tech Industry R$$$ missing jobs fix
    BudgetTaxTableBugfix     = 2,  // Fix cSC4BudgetSimulator +0x83c/+0x844 overwrite bug
    OperaHouseAndCivicCapFix = 3,  // Fix Opera House & civic capacity overflow strikes
    CrimeInMegaLotsFix       = 4,  // Police foot-patrol reach inside >4x4 mega-lots
    PropPoxSaveGuard         = 5,  // Prevent DBPF Prop Pox save corruption
    LessAbandonmentMod       = 6,  // 180-day abandonment grace & anti-dilapidation
    ExtraCheatsBuiltIn       = 7,  // Maxis ExtraCheats.dll (moolah, fightthepower, etc.)
    CAMExtendedGrowthStages  = 8,  // Colossus Addon Mod (Stages 1..15, 3000 resource cap)
    CivicRadiusDoubler       = 9,  // 2x Police/Fire/Health/School/Power relay radius
    IndustryJobQuadrupler    = 10, // 4x Industrial / 2x Commercial job capacities
    Count                    = 11
};

inline constexpr std::size_t kEnhancedModCount =
    static_cast<std::size_t>(EnhancedModId::Count);

struct EnhancedModDescriptor {
    EnhancedModId id = EnhancedModId::NAMTrafficSimulator;
    std::string_view slug;
    std::string_view name;
    std::string_view category; // "Bugfix", "Traffic & Transit", "Overhaul", "QoL & Sandbox"
    std::string_view description;
    bool defaultOnInEnhanced = true;
};

std::span<const EnhancedModDescriptor> GetEnhancedModCatalog() noexcept;
const EnhancedModDescriptor* FindEnhancedModBySlug(std::string_view slug) noexcept;

struct EnhancedModConfig {
    EngineEdition edition = EngineEdition::Enhanced;
    std::array<bool, kEnhancedModCount> enabledMods{};
    // Precomputed LUTs updated whenever mod flags change:
    std::array<Float32, 256> namCongestionPenaltyLUT{};
    std::array<Uint32, 16> zoneCapacityMultiplierLUT{};

    static constexpr EnhancedModConfig AuthenticVanilla() noexcept {
        EnhancedModConfig cfg{};
        cfg.edition = EngineEdition::Authentic;
        cfg.enabledMods.fill(false);
        cfg.RebuildLUTs();
        return cfg;
    }

    static constexpr EnhancedModConfig DefaultEnhanced() noexcept {
        EnhancedModConfig cfg{};
        cfg.edition = EngineEdition::Enhanced;
        cfg.enabledMods[static_cast<std::size_t>(EnhancedModId::NAMTrafficSimulator)]      = true;
        cfg.enabledMods[static_cast<std::size_t>(EnhancedModId::IHMissingJobsFix)]         = true;
        cfg.enabledMods[static_cast<std::size_t>(EnhancedModId::BudgetTaxTableBugfix)]     = true;
        cfg.enabledMods[static_cast<std::size_t>(EnhancedModId::OperaHouseAndCivicCapFix)] = true;
        cfg.enabledMods[static_cast<std::size_t>(EnhancedModId::CrimeInMegaLotsFix)]       = true;
        cfg.enabledMods[static_cast<std::size_t>(EnhancedModId::PropPoxSaveGuard)]         = true;
        cfg.enabledMods[static_cast<std::size_t>(EnhancedModId::LessAbandonmentMod)]       = true;
        cfg.enabledMods[static_cast<std::size_t>(EnhancedModId::ExtraCheatsBuiltIn)]       = true;
        cfg.enabledMods[static_cast<std::size_t>(EnhancedModId::CAMExtendedGrowthStages)]  = false;
        cfg.enabledMods[static_cast<std::size_t>(EnhancedModId::CivicRadiusDoubler)]       = false;
        cfg.enabledMods[static_cast<std::size_t>(EnhancedModId::IndustryJobQuadrupler)]    = false;
        cfg.RebuildLUTs();
        return cfg;
    }

    static constexpr EnhancedModConfig AllModsEnabled() noexcept {
        EnhancedModConfig cfg{};
        cfg.edition = EngineEdition::Enhanced;
        cfg.enabledMods.fill(true);
        cfg.RebuildLUTs();
        return cfg;
    }

    constexpr void RebuildLUTs() noexcept {
        const bool nam = IsModActive(EnhancedModId::NAMTrafficSimulator);
        const Float32 invCap = nam ? (1.0f / (255.0f * 2.5f)) : (1.0f / 255.0f);
        for (std::size_t c = 0; c < 256u; ++c) {
            if (!nam) {
                namCongestionPenaltyLUT[c] = 1.0f;
            } else {
                const Float32 ratio = static_cast<Float32>(c) * invCap;
                namCongestionPenaltyLUT[c] = 1.0f + 1.5f * ratio * ratio;
            }
        }

        const bool quad = IsModActive(EnhancedModId::IndustryJobQuadrupler);
        for (std::size_t z = 0; z < 16u; ++z) {
            if (!quad) {
                zoneCapacityMultiplierLUT[z] = 1u;
            } else if (z >= 4u && z <= 6u) {
                zoneCapacityMultiplierLUT[z] = 2u; // CommercialLow..High
            } else if (z >= 7u && z <= 9u) {
                zoneCapacityMultiplierLUT[z] = 4u; // IndustrialAg..High
            } else {
                zoneCapacityMultiplierLUT[z] = 1u;
            }
        }
    }

    constexpr bool IsModActive(EnhancedModId mod) const noexcept {
        if (edition != EngineEdition::Enhanced) {
            return false;
        }
        const auto idx = static_cast<std::size_t>(mod);
        return idx < kEnhancedModCount && enabledMods[idx];
    }

    constexpr void SetModEnabled(EnhancedModId mod, bool enabled) noexcept {
        const auto idx = static_cast<std::size_t>(mod);
        if (idx < kEnhancedModCount) {
            enabledMods[idx] = enabled;
            if (enabled) {
                edition = EngineEdition::Enhanced;
            }
            RebuildLUTs();
        }
    }

    constexpr Uint32 ActiveModCount() const noexcept {
        if (edition != EngineEdition::Enhanced) {
            return 0u;
        }
        Uint32 count = 0u;
        for (bool on : enabledMods) {
            if (on) ++count;
        }
        return count;
    }

    constexpr Sint32 PowerRelayRadiusCells() const noexcept {
        return IsModActive(EnhancedModId::CivicRadiusDoubler) ? 4 : 2;
    }

    constexpr Sint32 WaterPumpRadiusCells() const noexcept {
        return IsModActive(EnhancedModId::CivicRadiusDoubler) ? 12 : 6;
    }

    constexpr Float32 MaxCommuteTripLength() const noexcept {
        return IsModActive(EnhancedModId::NAMTrafficSimulator) ? 1800.0f : 180.0f;
    }

    constexpr Float32 NetworkCapacityMultiplier() const noexcept {
        return IsModActive(EnhancedModId::NAMTrafficSimulator) ? 2.5f : 1.0f;
    }

    constexpr Uint32 AbandonmentGraceDays() const noexcept {
        return IsModActive(EnhancedModId::LessAbandonmentMod) ? 180u : 60u;
    }

    constexpr Sint32 MaxMonthlyConstructionResources(Sint32 vanillaMax = 2000) const noexcept {
        return IsModActive(EnhancedModId::CAMExtendedGrowthStages)
            ? (vanillaMax * 3) / 2
            : vanillaMax;
    }

    constexpr Uint8 MaxAllowedGrowthStage(Uint8 vanillaMaxStage = 8u) const noexcept {
        return IsModActive(EnhancedModId::CAMExtendedGrowthStages) ? 15u : vanillaMaxStage;
    }

    constexpr Uint32 ScaleLotCapacity(Uint32 baseCapacity, Uint8 zoneType) const noexcept {
        return baseCapacity * zoneCapacityMultiplierLUT[zoneType & 0x0Fu];
    }
};

// ============================================================================
// 1. O(1) Contiguous Spatial Grid Index (replaces O(N*M) linear scans)
// ============================================================================
class cEnhancedSpatialIndex {
public:
    void Rebuild(int cellsX, int cellsZ,
                 std::span<const SC4LotRecord> lots,
                 std::span<const SC4BuildingRecord> buildings);

    Sint32 LotIndexAtCell(int cx, int cz) const noexcept;
    Sint32 BuildingIndexAtCell(int cx, int cz) const noexcept;
    bool LotHasBuildingFast(std::size_t lotIndex) const noexcept;
    bool LotBoundsHaveBuilding(Uint8 minX, Uint8 minZ, Uint8 maxX, Uint8 maxZ) const noexcept;
    bool IsMegaLotCell(int cx, int cz, std::span<const SC4LotRecord> lots) const noexcept;

    int Width() const noexcept { return mWidth; }
    int Height() const noexcept { return mHeight; }
    std::size_t LotCount() const noexcept { return mLotHasBuilding.size(); }

private:
    int mWidth = 0;
    int mHeight = 0;
    std::vector<Sint32> mCellToLot;      // -1 if no lot
    std::vector<Sint32> mCellToBuilding; // -1 if no building
    std::vector<Uint8> mLotHasBuilding;  // 1 if lot has >= 1 building
};

// ============================================================================
// 2. Zero-Allocation Generation-Counter A* Pathfinder Scratchpad
// ============================================================================
class cEnhancedPathFinderScratch {
public:
    bool FindPathFast(
        const SC4Rect<Sint32>& startRect,
        const SC4Rect<Sint32>& endRect,
        Sint32 gridCellsX,
        Sint32 gridCellsZ,
        std::span<const Float32> baseTravelCost,
        std::span<const Uint8> travelType,
        std::span<const Uint8> congestionByCell,
        Float32 minStepHint,
        const EnhancedModConfig& modConfig,
        SC4PathFindResult& outResult);

    Uint32 GenerationCount() const noexcept { return mCurrentGen; }

private:
    struct OpenNode {
        Float32 f = 0.0f;
        Float32 g = 0.0f;
        Sint32 x = 0;
        Sint32 z = 0;
    };

    void EnsureCapacity(std::size_t cellCount);

    Uint32 mCurrentGen = 0;
    std::vector<Uint32> mVisitedGen;
    std::vector<Uint32> mClosedGen;
    std::vector<Float32> mGScore;
    std::vector<Sint32> mParent;
    std::vector<OpenNode> mOpenHeap;
    std::vector<Sint32> mReversedPath;
};

// ============================================================================
// 3. Optimized Mod Helpers & Patchers
// ============================================================================

// Applies or reverts the BudgetTaxTableBugfix (+0x83c Co$$$ multiplier) on a
// BudgetSimulatorTuning struct without needing disk I/O.
void ApplyEnhancedBudgetTuningFixes(
    BudgetSimulatorTuning& inOutTuning,
    const EnhancedModConfig& config) noexcept;

// Computes I-HT (0x4400) workforce demand distribution across R$, R$$, R$$$.
std::array<Float32, 3> ComputeHighTechWorkforceDemand(
    Float32 activeHighTechJobs,
    const EnhancedModConfig& config) noexcept;

// Computes crime in a lot cell, applying CrimeInMegaLotsFix when active.
Uint8 ComputeLotCellCrime(
    Uint8 baseCrime,
    Uint8 lotSizeX,
    Uint8 lotSizeZ,
    Uint8 policeCoverageAtLotEdge,
    const EnhancedModConfig& config) noexcept;

// Evaluates civic building grade (Opera House & Civic Capacity Overflow Fix).
Float32 ComputeCivicOccupantGrade(
    Uint32 currentUsage,
    Uint32 baseCapacity,
    Float32 fundingPercent,
    const EnhancedModConfig& config) noexcept;

// Executes a Maxis ExtraCheats command ("moolah 500000", "weaknesspays",
// "fightthepower", "howdryiam") when ExtraCheatsBuiltIn is active.
bool ExecuteEnhancedCheatCommand(
    cSC4City& city,
    std::string_view commandLine,
    const EnhancedModConfig& config);

} // namespace opensc4
