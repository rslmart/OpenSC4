#pragma once

// Residential, commercial, industrial, land value, aura, and civic-building
// simulators. Definitions stay in civic_simulators.h. These aliases group
// that surface under opensc4::sim.

#include "opensc4/civic_simulators.h"

namespace opensc4::sim {

using Sint32Triple = opensc4::Sint32Triple;

using LandValueSimulatorSaveRecord = opensc4::LandValueSimulatorSaveRecord;
using LandValueTuning = opensc4::LandValueTuning;

using CommercialSimulatorSaveRecord = opensc4::CommercialSimulatorSaveRecord;
using IndustrialSimulatorSaveRecord = opensc4::IndustrialSimulatorSaveRecord;

using AuraSimulatorSaveRecord = opensc4::AuraSimulatorSaveRecord;
using AuraTuning = opensc4::AuraTuning;

using ResidentialDepartmentData = opensc4::ResidentialDepartmentData;
using ResidentialSimulatorSaveRecord = opensc4::ResidentialSimulatorSaveRecord;
using ResidentialTuning = opensc4::ResidentialTuning;

using ConditionalBuildingStatusRecord = opensc4::ConditionalBuildingStatusRecord;
using CivicBuildingSimulatorSaveRecord = opensc4::CivicBuildingSimulatorSaveRecord;

}  // namespace opensc4::sim
