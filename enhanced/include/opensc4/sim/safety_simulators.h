#pragma once

// Protection, fire, police, flammability, and crime simulators.
// Definitions stay in civic_simulators.h. These aliases group that surface
// under opensc4::sim.

#include "opensc4/civic_simulators.h"

namespace opensc4::sim {

using Sint32Triple = opensc4::Sint32Triple;

using ProtectionSimulatorSaveRecord = opensc4::ProtectionSimulatorSaveRecord;

using FireProtectionSimulatorSaveRecord = opensc4::FireProtectionSimulatorSaveRecord;
using FireProtectionTuning = opensc4::FireProtectionTuning;

using PoliceSimulatorSaveRecord = opensc4::PoliceSimulatorSaveRecord;
using PoliceTuning = opensc4::PoliceTuning;

using FlammabilitySimulatorSaveRecord = opensc4::FlammabilitySimulatorSaveRecord;
using FlammabilityTuning = opensc4::FlammabilityTuning;

using CrimeEventRecord = opensc4::CrimeEventRecord;
using CrimeSimulatorSaveRecord = opensc4::CrimeSimulatorSaveRecord;
using CrimeTuning = opensc4::CrimeTuning;

}  // namespace opensc4::sim
