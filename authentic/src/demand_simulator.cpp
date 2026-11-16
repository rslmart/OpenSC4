#include "opensc4/demand_simulator.h"

#include <algorithm>
#include <cmath>

namespace opensc4 {
namespace {

constexpr std::array<Uint32, 12> kDeveloperTypes{
    0x1010u, 0x1020u, 0x1030u,
    0x3110u, 0x3120u, 0x3130u,
    0x3320u, 0x3330u,
    0x4100u, 0x4200u, 0x4300u, 0x4400u,
};

void AppendHistory(std::vector<cSC4DemandSimulator::HistorySample>& samples,
                   Uint32 historyId, Sint32 value) {
    samples.push_back({historyId, value});
}

} // namespace

DemandNodeState& cSC4DemandSimulator::Demand(Uint32 demandId) noexcept {
    DemandNodeState& node = demands[demandId];
    node.demandId = demandId;
    return node;
}

const DemandNodeState* cSC4DemandSimulator::FindDemand(Uint32 demandId) const noexcept {
    const auto it = demands.find(demandId);
    return it == demands.end() ? nullptr : &it->second;
}

void cSC4DemandSimulator::SetEconomyLinks(std::span<const DemandEconomyLink> links) {
    economyLinks.clear();
    for (const DemandEconomyLink& link : links) {
        // std::multimap preserves insertion order among equivalent source keys,
        // matching the original EconomyLink multimap traversal.
        economyLinks.emplace(link.sourceDemandId, link);
    }
}

Uint32 cSC4DemandSimulator::DoDemandIndexLinks() {
    // Exact cSC4DemandSimulator::DoDemandIndexLinks @ 0x100225380.
    if (demands.empty()) {
        return 0;
    }

    Uint32 passes = 0;
    for (;;) {
        ++passes;
        bool converged = true;

        // std::map order is part of the original fixed-point algorithm.
        for (auto& [sourceId, source] : demands) {
            Float32 aggregateSupply = 0.0f;
            Float32 aggregateDemand = 0.0f;
            bool hasAggregateLink = false;

            const auto [linkIt, linkEnd] = economyLinks.equal_range(sourceId);
            for (auto it = linkIt; it != linkEnd; ++it) {
                const DemandEconomyLink& link = it->second;
                DemandNodeState& target = Demand(link.targetDemandId);

                switch (link.linkType) {
                case DemandEconomyLinkType::SupplyDrivesSupply: {
                    // source.QueryNewSupply * coefficient -> target.newSupply
                    // @ 0x100225488-0x1002254bb.
                    const Float32 delta = source.newSupply * link.coefficient;
                    if (std::abs(delta) > 0.01f) {
                        target.newSupply += delta;
                        converged = false;
                    }
                    break;
                }
                case DemandEconomyLinkType::DemandDrivesDemand:
                case DemandEconomyLinkType::DemandDrivesSupply: {
                    // Both opcodes read QueryNewDemand and call AddToDemandValue;
                    // 0x47bb3f32 does not write newSupply despite its enum name.
                    // @ 0x1002254c0-0x1002254c9 / 0x10022552b-0x10022555a.
                    const Float32 delta = source.newDemand * link.coefficient;
                    if (std::abs(delta) > 0.01f) {
                        target.newDemand += delta;
                        converged = false;
                    }
                    break;
                }
                case DemandEconomyLinkType::SatisfactionLink: {
                    // clamp(max(0,demand)/max(0.01,supply),0,1), with the
                    // original ordered comparison selecting 1 at/above one.
                    // @ 0x1002254cb-0x100225523.
                    const Float32 denominator = std::max(0.01f, source.supplyValue);
                    const Float32 numerator = std::max(0.0f, source.demandValue);
                    const Float32 ratio = numerator < denominator
                                              ? numerator / denominator
                                              : 1.0f;
                    target.demandCap = ratio;
                    break;
                }
                case DemandEconomyLinkType::AggregateChildren:
                    // Addition order is link-multimap order @ 0x100225440-0x100225468.
                    aggregateSupply += target.supplyValue;
                    aggregateDemand += target.demandValue;
                    hasAggregateLink = true;
                    break;
                }
            }

            // Every source ends its cycle after its outgoing links, before
            // aggregate reconciliation @ 0x1002255bc-0x1002255c8.
            source.EndOfCycle();

            if (hasAggregateLink) {
                const Float32 supplyDelta = aggregateSupply - source.supplyValue;
                const Float32 demandDelta = aggregateDemand - source.demandValue;
                if (std::abs(supplyDelta) > 0.01f) {
                    source.newSupply += supplyDelta;
                    converged = false;
                }
                if (std::abs(demandDelta) > 0.01f) {
                    source.newDemand += demandDelta;
                    converged = false;
                }
            }
        }

        if (converged) {
            break;
        }
    }
    return passes;
}

void cSC4DemandSimulator::ZeroCensusInfo(std::map<Uint32, Float32>& censusInfo) {
    // Exact cSC4DemandSimulator::ZeroCensusInfo @ 0x100224a42: assign zero to
    // the 12 EQ keys followed by the 12 primary keys; do not clear the map.
    constexpr std::array<Uint32, 12> kPrimaryCensusKeys{
        0x1011u, 0x1021u, 0x1031u,
        0x3111u, 0x3121u, 0x3131u, 0x3321u, 0x3331u,
        0x4101u, 0x4201u, 0x4301u, 0x4401u,
    };
    for (const Uint32 key : kSalaryDemandTypes) {
        censusInfo[key] = 0.0f;
    }
    for (const Uint32 key : kPrimaryCensusKeys) {
        censusInfo[key] = 0.0f;
    }
}

Float32 cSC4DemandSimulator::GetNeutralTaxRate() {
    // Exact cSC4DemandSimulator::GetNeutralTaxRate @ 0x100225e44.
    const Float32 resSupply = Demand(0x1001u).supplyValue;
    const Float32 comSupply = Demand(0x3001u).supplyValue;
    const Float32 indSupply = Demand(0x4001u).supplyValue;
    const Float32 totalSupply = (comSupply + resSupply) + indSupply;
    return tuning.neutralTaxRateCurve.GetOutputValue(totalSupply);
}

void cSC4DemandSimulator::UpdateTaxAndEconomyModifiers() {
    // Exact cSC4DemandSimulator::UpdateTaxAndEconomyModifiers @ 0x10022718a.
    // The neutral rate and its denominator are computed once and reused by all
    // twelve categories (@ 0x1002271c5 / 0x10022722b-0x100227256).
    const Float32 neutralTaxRate = GetNeutralTaxRate();
    const Float32 denominator = std::max(0.1f, neutralTaxRate);

    for (std::size_t i = 0; i < kActiveRCIDemandIds.size(); ++i) {
        DemandNodeState& demand = Demand(kActiveRCIDemandIds[i]);

        const Float32 ratio = (taxEconomyInputs.averageTaxRates[i] * 0.1f) / denominator;
        const Float32 curveModifier = tuning.taxModifierCurve.GetOutputValue(ratio);
        const Float32 ordinanceModifier = taxEconomyInputs.ordinanceTaxRates[i] + -1.0f;
        demand.taxModifier = curveModifier + ordinanceModifier;

        // Nation::GetEconomyModifier(developerType) is called after the tax
        // modifier store. Co and industrial categories add their connection
        // bonus before SetEconomyModifier (@ 0x10022771f-0x100227751 and
        // 0x1002278c5-0x1002278f7).
        Float32 economyModifier = taxEconomyInputs.economyModifiers[i];
        if (i == 6u || i == 7u) {
            economyModifier = commercialConnectionBonus + economyModifier;
        } else if (i >= 8u) {
            economyModifier = industrialConnectionBonus + economyModifier;
        }
        demand.economyModifier = economyModifier;
        (void)kDeveloperTypes[i]; // Documents the corresponding nation query key.
    }
}

void cSC4DemandSimulator::PerformCensus(Sint32 x, Sint32 z) {
    // Building/development/occupant filtering precedes the numerical body in
    // cSC4DemandSimulator::PerformCensus(int,int) @ 0x100227bfe-0x100227c87.
    if (censusCellProvider == nullptr) {
        return;
    }
    CensusCellInput input{};
    if (censusCellProvider(censusCellContext, x, z, input)) {
        PerformCensus(input);
    }
}

void cSC4DemandSimulator::PerformCensus(const CensusCellInput& input) {
    // Exact arithmetic and key selection @ 0x100227c72-0x100227e9a.
    const CensusCellContribution contribution = ComputeCensusCellContribution(input);
    if (contribution.eqKey != 0u) {
        // Residential stores retained population to EQ before base population.
        censusInfo[contribution.eqKey] += contribution.eqValue;
    }
    if (contribution.primaryKey != 0u) {
        censusInfo[contribution.primaryKey] += contribution.primaryValue;
    }
}

void cSC4DemandSimulator::DoIdle(Uint32 timeBudgetMilliseconds) {
    // Exact cSC4DemandSimulator::DoIdle state ordering @ 0x1002264b2.
    // The original uses this value only for its dynamic wall-clock deadline;
    // this deterministic entry point runs an unexpired slice to completion.
    (void)timeBudgetMilliseconds;

    switch (idleState) {
    case 0:
        // @ 0x100226833-0x100226866.
        UpdateTaxAndEconomyModifiers();
        ZeroCensusInfo(censusInfo);
        censusIterator.x = 0;
        censusIterator.z = 0;
        censusIterator.status = 0;
        // Exact bound tests @ 0x100226853-0x100226863 and
        // 0x100226983-0x100226997: positive minima or negative maxima complete
        // the iterator immediately; the valid scan itself starts at (0,0).
        if (censusIterator.minX > 0 || censusIterator.minZ > 0 ||
            censusIterator.maxX < 0 || censusIterator.maxZ < 0) {
            censusIterator.status = 2;
        }
        idleState = 1;
        break;

    case 1: {
        // ScanningIterator traversal and PerformCensus calls
        // @ 0x10022690b-0x100226952.
        while (censusIterator.status != 2) {
            if (censusIterator.status == 0) {
                censusIterator.status = 1;
            } else {
                censusIterator.x += censusIterator.step;
                if (censusIterator.x > censusIterator.maxX) {
                    censusIterator.x = censusIterator.minX;
                    censusIterator.z += censusIterator.step;
                    if (censusIterator.z > censusIterator.maxZ) {
                        censusIterator.status = 2;
                        break;
                    }
                }
            }
            PerformCensus(censusIterator.x, censusIterator.z);
        }

        // Reconcile current census (+0x30) against saved census (+0x48), then
        // route each changed value by cISC4Demand::GetRegionUse:
        // 1 -> AddToSupplyValue, 2 -> AddToDemandValue.
        // @ 0x1002269b5-0x100226b08.
        for (const auto& [key, currentValue] : censusInfo) {
            const auto previousIt = previousCensusInfo.find(key);
            const Float32 previousValue = previousIt == previousCensusInfo.end()
                                              ? 0.0f
                                              : previousIt->second;
            const Float32 delta = currentValue - previousValue;
            if (delta == 0.0f) {
                continue;
            }
            auto demandIt = demands.find(key);
            if (demandIt == demands.end()) {
                continue;
            }
            DemandNodeState& demand = demandIt->second;
            if (demand.regionUse == 1u) {
                demand.newSupply += delta;
            } else if (demand.regionUse == 2u) {
                demand.newDemand += delta;
            }
            previousCensusInfo[key] = currentValue;
        }
        idleState = 2;
        break;
    }

    case 2: {
        // History writes @ 0x100226503-0x10022682e. Their order is preserved.
        historySamples.clear();
        AppendHistory(historySamples, 0x2a4e2003u,
                      static_cast<Sint32>(Demand(0x1001u).supplyValue));
        AppendHistory(historySamples, 0x4a5e9485u,
                      static_cast<Sint32>(Demand(0x1011u).supplyValue));
        AppendHistory(historySamples, 0xea5e948au,
                      static_cast<Sint32>(Demand(0x1021u).supplyValue));
        AppendHistory(historySamples, 0x2a5e948eu,
                      static_cast<Sint32>(Demand(0x1031u).supplyValue));

        constexpr std::array<Uint32, 5> kCommercialHistory{
            0x8a5e950eu, 0x0a5e9515u, 0x8a5e954au, 0x8a5e9552u, 0xea5e9557u,
        };
        Sint32 commercialTotal = 0;
        for (std::size_t i = 0; i < kCommercialHistory.size(); ++i) {
            AppendHistory(historySamples, kCommercialHistory[i], jobsByCensus[i]);
            commercialTotal += jobsByCensus[i];
        }
        AppendHistory(historySamples, 0x0a4e2056u, commercialTotal);

        constexpr std::array<Uint32, 4> kIndustrialHistory{
            0x2a5e95a1u, 0x6a5e95a7u, 0x0a5e95acu, 0xaa5e95b1u,
        };
        Sint32 industrialTotal = 0;
        for (std::size_t i = 0; i < kIndustrialHistory.size(); ++i) {
            const Sint32 jobs = jobsByCensus[i + 5u];
            AppendHistory(historySamples, kIndustrialHistory[i], jobs);
            industrialTotal += jobs;
        }
        AppendHistory(historySamples, 0x4a4e206bu, industrialTotal);

        const Float32 workforceDemand = Demand(0x2000u).demandValue;
        AppendHistory(historySamples, 0x6a931184u,
                      static_cast<Sint32>(workforceDemand));
        const Float32 commercialFloat = static_cast<Float32>(commercialTotal);
        const Float32 industrialFloat = static_cast<Float32>(industrialTotal);
        const Float32 unemployment = std::max(
            0.0f, (workforceDemand - commercialFloat) - industrialFloat);
        AppendHistory(historySamples, 0x8a931172u,
                      static_cast<Sint32>(unemployment));

        // Twelve QuerySupplyValue/truncate/weight operations
        // @ 0x1002267be-0x10022682c.
        std::array<Float32, 12> eqSupplies{};
        for (std::size_t i = 0; i < kSalaryDemandTypes.size(); ++i) {
            eqSupplies[i] = Demand(kSalaryDemandTypes[i]).supplyValue;
        }
        averageCitySalary = ComputeAverageCitySalary(tuning, eqSupplies);
        AppendHistory(historySamples, 0x8a63f30cu,
                      static_cast<Sint32>(averageCitySalary));
        idleState = 3;
        break;
    }

    default:
        break;
    }
}

} // namespace opensc4
