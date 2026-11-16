#pragma once

// Power, plumbing, and pollution simulators.
// Definitions stay in civic_simulators.h so existing translation units keep
// compiling. These aliases group that surface under opensc4::sim.

#include "opensc4/civic_simulators.h"

namespace opensc4::sim {

using PowerSimulatorSaveRecord = opensc4::PowerSimulatorSaveRecord;
using PowerTuning = opensc4::PowerTuning;

using WaterProducerRecord = opensc4::WaterProducerRecord;
using PlumbingSimulatorSaveRecord = opensc4::PlumbingSimulatorSaveRecord;
using PlumbingTuning = opensc4::PlumbingTuning;

using PollutionSimulatorSaveRecord = opensc4::PollutionSimulatorSaveRecord;
using PollutionTuning = opensc4::PollutionTuning;

}  // namespace opensc4::sim
