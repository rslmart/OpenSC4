#pragma once

// Tract developer, growth developer, building-development simulator, and
// demand simulator. Definitions stay in tract_developer.h. These aliases
// group that surface under opensc4::developers.

#include "opensc4/tract_developer.h"

namespace opensc4::developers {

using TractCandidateScoreInput = opensc4::TractCandidateScoreInput;
using TractExistingLotCapacityInput = opensc4::TractExistingLotCapacityInput;
using TractCellRect = opensc4::TractCellRect;
using TractSourceLotInfo = opensc4::TractSourceLotInfo;
using TractLotSuitabilityInput = opensc4::TractLotSuitabilityInput;
using TractLotConfigSuitabilityInput = opensc4::TractLotConfigSuitabilityInput;
using TractLotConfigCompatibilityInput = opensc4::TractLotConfigCompatibilityInput;
using TractBuildingCompatibilityInput = opensc4::TractBuildingCompatibilityInput;
using TractStylePercentageEntry = opensc4::TractStylePercentageEntry;
using TractTakeOverAbandonedLotInput = opensc4::TractTakeOverAbandonedLotInput;
using TractDeclineLotInput = opensc4::TractDeclineLotInput;
using TractDeveloperTuning = opensc4::TractDeveloperTuning;
using TractDeveloperState = opensc4::TractDeveloperState;

using cSC4ResponseCurve = opensc4::cSC4ResponseCurve;
using GrowthDeveloperSimGridIDs = opensc4::GrowthDeveloperSimGridIDs;
using GrowthDesirabilityInputs = opensc4::GrowthDesirabilityInputs;
using GrowthDeveloperTuning = opensc4::GrowthDeveloperTuning;
using GrowthDeveloperItem = opensc4::GrowthDeveloperItem;
using GrowthDeveloperSaveRecord = opensc4::GrowthDeveloperSaveRecord;
using LotCapacityUpdateInput = opensc4::LotCapacityUpdateInput;

using BuildingDevelopmentSimSaveRecord = opensc4::BuildingDevelopmentSimSaveRecord;

using DemandEconomyLinkType = opensc4::DemandEconomyLinkType;
using DemandEconomyLink = opensc4::DemandEconomyLink;
using DemandExemplarSpec = opensc4::DemandExemplarSpec;
using DemandSimulatorTuning = opensc4::DemandSimulatorTuning;
using DemandSimulatorSaveRecord = opensc4::DemandSimulatorSaveRecord;
using CensusCellInput = opensc4::CensusCellInput;
using CensusCellContribution = opensc4::CensusCellContribution;
using DemandNodeState = opensc4::DemandNodeState;
using DemandAgentEffect = opensc4::DemandAgentEffect;
using ConnectionUtilizationInputs = opensc4::ConnectionUtilizationInputs;
using ConnectionUtilizationOutputs = opensc4::ConnectionUtilizationOutputs;
using ExtrapolatedResidentialCensus = opensc4::ExtrapolatedResidentialCensus;
using ExtrapolatedBusinessCensus = opensc4::ExtrapolatedBusinessCensus;

}  // namespace opensc4::developers
