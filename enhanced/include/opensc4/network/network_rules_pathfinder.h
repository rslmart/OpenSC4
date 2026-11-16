#pragma once

// Network rule tables, tile evaluator, network-lot configuration, and the
// pathfinder. Definitions stay in tract_developer.h. These aliases group
// that surface under opensc4::network.

#include "opensc4/tract_developer.h"

namespace opensc4::network {

using SC4NetworkType = opensc4::SC4NetworkType;
using cSC4TileNeighborhoodCompact = opensc4::cSC4TileNeighborhoodCompact;
using cSC4TileInfo = opensc4::cSC4TileInfo;
using tCompactNeighborInfo = opensc4::tCompactNeighborInfo;
using cSC4TileRule = opensc4::cSC4TileRule;
using cSC4TileEvaluator = opensc4::cSC4TileEvaluator;

using NetworkTextureRemapEntry = opensc4::NetworkTextureRemapEntry;
using NetworkShadowRemapByEdgesEntry = opensc4::NetworkShadowRemapByEdgesEntry;
using NetworkPathRemapEntry = opensc4::NetworkPathRemapEntry;
using NetworkTileConflictRule = opensc4::NetworkTileConflictRule;
using MixedTypeIntersectionSolution = opensc4::MixedTypeIntersectionSolution;
using NetworkSolvedCell = opensc4::NetworkSolvedCell;
using cSC4NetworkRuleTables = opensc4::cSC4NetworkRuleTables;
using NetworkManagerSaveRecord = opensc4::NetworkManagerSaveRecord;

using cSC4NetworkLotConfiguration = opensc4::cSC4NetworkLotConfiguration;
using NetworkRoadStyleResult = opensc4::NetworkRoadStyleResult;
using NetworkLotSaveEntry = opensc4::NetworkLotSaveEntry;
using NetworkDamageEffectEntry = opensc4::NetworkDamageEffectEntry;
using NetworkLotManagerSaveRecord = opensc4::NetworkLotManagerSaveRecord;

using SC4PathKeyHelper = opensc4::SC4PathKeyHelper;
using SC4PathFindResult = opensc4::SC4PathFindResult;
using cSC4PathFinderEngine = opensc4::cSC4PathFinderEngine;

}  // namespace opensc4::network
