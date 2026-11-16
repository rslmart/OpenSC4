#pragma once

// Traffic simulator, transit switches, commute blocks, and ferry terminals.
// Definitions stay in civic_simulators.h. These aliases group that surface
// under opensc4::sim.

#include "opensc4/civic_simulators.h"

namespace opensc4::sim {

using TrafficEdgeDensityRecord = opensc4::TrafficEdgeDensityRecord;
using TransitSwitchRuleRecord = opensc4::TransitSwitchRuleRecord;
using TransitSwitchRecord = opensc4::TransitSwitchRecord;
using TrafficStatEntry20 = opensc4::TrafficStatEntry20;
using TrafficSubStatEntry20 = opensc4::TrafficSubStatEntry20;
using TrafficCommuteBlock = opensc4::TrafficCommuteBlock;
using TrafficTunnelInfoRecord = opensc4::TrafficTunnelInfoRecord;
using FerryTerminalLinkRecord = opensc4::FerryTerminalLinkRecord;
using FerryNeighborConnectionLinkRecord = opensc4::FerryNeighborConnectionLinkRecord;
using FerryTerminalRecord = opensc4::FerryTerminalRecord;
using TrafficSimulatorSaveRecord = opensc4::TrafficSimulatorSaveRecord;
using TrafficTuning = opensc4::TrafficTuning;

}  // namespace opensc4::sim
