#pragma once

// Lot configuration, lot developer, zone parcellizer, and vertex-height
// solver. Definitions stay in tract_developer.h. These aliases group that
// surface under opensc4::developers.

#include "opensc4/tract_developer.h"

namespace opensc4::developers {

using LotConfigObjectType = opensc4::LotConfigObjectType;
using cSC4LotConfigurationObject = opensc4::cSC4LotConfigurationObject;
using cSC4LotConfiguration = opensc4::cSC4LotConfiguration;
using LotConfigurationManagerSaveRecord = opensc4::LotConfigurationManagerSaveRecord;
using LotDeveloperTuning = opensc4::LotDeveloperTuning;
using BoundaryLotTextureRule = opensc4::BoundaryLotTextureRule;
using LotObjectWorldPlacement = opensc4::LotObjectWorldPlacement;
using LotDeveloperSaveRecord = opensc4::LotDeveloperSaveRecord;

using ZoneParcel = opensc4::ZoneParcel;
using ZoneLotSize = opensc4::ZoneLotSize;
using cSC4ZoneParcellizer = opensc4::cSC4ZoneParcellizer;
using cSC4VertexHeightSolver = opensc4::cSC4VertexHeightSolver;

}  // namespace opensc4::developers
