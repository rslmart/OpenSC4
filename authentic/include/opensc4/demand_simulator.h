#pragma once

#include "opensc4/tract_developer.h"

#include <array>
#include <map>
#include <vector>

namespace opensc4 {

// Runtime census/demand-index slice of cSC4DemandSimulator.
//
// Implemented here: ZeroCensusInfo @ 0x100224a42, DoDemandIndexLinks
// @ 0x100225380, GetNeutralTaxRate @ 0x100225e44, DoIdle @ 0x1002264b2,
// UpdateTaxAndEconomyModifiers @ 0x10022718a, and PerformCensus
// @ 0x100227bfe. City/lot/history/nation/ordinance interfaces are represented
// by explicit inputs so the numerical chain can be exercised without GZCOM.
class cSC4DemandSimulator {
public:
    using CensusCellProvider = bool (*)(void* context, Sint32 x, Sint32 z,
                                        CensusCellInput& outInput);

    struct CensusIterator {
        Uint8 status = 0; // 0=reset, 1=scanning, 2=complete
        Sint32 x = 0;
        Sint32 z = 0;
        Sint32 step = 1;
        Sint32 minX = 0;
        Sint32 minZ = 0;
        Sint32 maxX = -1;
        Sint32 maxZ = -1;
    };

    // Results of the history/nation/ordinance queries made by
    // UpdateTaxAndEconomyModifiers @ 0x10022718a. Array order is
    // kActiveRCIDemandIds / kTaxModifierIds.
    struct TaxEconomyInputs {
        std::array<Float32, 12> averageTaxRates{};
        std::array<Float32, 12> ordinanceTaxRates{};
        std::array<Float32, 12> economyModifiers{};
    };

    struct HistorySample {
        Uint32 historyId = 0;
        Sint32 value = 0;
    };

    cSC4DemandSimulator() = default;

    DemandNodeState& Demand(Uint32 demandId) noexcept;
    const DemandNodeState* FindDemand(Uint32 demandId) const noexcept;
    void SetEconomyLinks(std::span<const DemandEconomyLink> links);

    // Exact fixed-point traversal @ 0x100225380. The return value is
    // instrumentation only: the original method returns void.
    Uint32 DoDemandIndexLinks();

    // Exact key initialization @ 0x100224a42. Other map entries are preserved.
    static void ZeroCensusInfo(std::map<Uint32, Float32>& censusInfo);

    // Exact supply summation and response-curve call @ 0x100225e44.
    Float32 GetNeutralTaxRate();

    // Exact modifier order for the 12 active RCI demands @ 0x10022718a.
    void UpdateTaxAndEconomyModifiers();

    // Exact per-cell census accumulation @ 0x100227bfe. The coordinate overload
    // obtains the already-filtered building/developer inputs from the city adapter.
    void PerformCensus(Sint32 x, Sint32 z);
    void PerformCensus(const CensusCellInput& input);

    // Exact state order @ 0x1002264b2. The wall-clock limiter is an adapter
    // concern; one invocation drains the selected state, equivalent to an
    // unexpired original idle slice.
    void DoIdle(Uint32 timeBudgetMilliseconds);

    DemandSimulatorTuning tuning{};
    TaxEconomyInputs taxEconomyInputs{};

    // Results of GetJobsBySensus @ 0x1002270de for
    // {3111,3121,3131,3321,3331,4101,4201,4301,4401}. That conversion remains
    // a separate ticket; DoIdle consumes its results in the recovered order.
    std::array<Sint32, 9> jobsByCensus{};

    void* censusCellContext = nullptr;
    CensusCellProvider censusCellProvider = nullptr;

    Uint32 idleState = 3; // +0x20; constructor value @ 0x1002240e1
    CensusIterator censusIterator{};

    Float32 commercialConnectionBonus = 0.0f; // +0x128
    Float32 industrialConnectionBonus = 0.0f; // +0x12c

    std::map<Uint32, DemandNodeState> demands;             // +0x98
    std::multimap<Uint32, DemandEconomyLink> economyLinks; // +0xb0
    std::map<Uint32, Float32> censusInfo;                  // +0x30
    std::map<Uint32, Float32> previousCensusInfo;          // +0x48

    Uint32 averageCitySalary = 0;
    std::vector<HistorySample> historySamples;

    // Honest decomposition of the remaining class; no silent substitute
    // implementations are provided for these methods.
    // TODO lifecycle/exemplar tickets: Init @ 0x100224520,
    // ReadSimulatorTunableValues @ 0x100224cb2, Shutdown @ 0x100225776,
    // CreateDemandFromExemplar @ 0x1002297e2, AddDemandToSimulator
    // @ 0x100229568, CreateNewDemand @ 0x100229458.
    // TODO effects ticket: AddToAgentEffectMap @ 0x100225ef4,
    // AddAgentEffect @ 0x100226ed2, UpdateOccupantEffects @ 0x10022a7c6,
    // UpdateCapacity @ 0x100226fd6.
    // TODO regional tickets: ResetRegionalDemands @ 0x1002250a4,
    // EvaluateConnectionUtilization @ 0x100227ea6, ExportDemand
    // @ 0x100228f18, ExtrapolateResidentialCensus @ 0x10022a2ec,
    // ExtrapolateBusinessCensus @ 0x10022a588.
    // TODO reporting ticket: DoDailySimulation @ 0x100226b92,
    // GetJobsBySensus @ 0x1002270de, CalculateJobsPerUnitOfDemand
    // @ 0x100229678, GetLocalPopulationSummary @ 0x10022a096.
};

} // namespace opensc4
