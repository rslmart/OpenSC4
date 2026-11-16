#pragma once

// Weather, flora, neighbors, MySim agents, ordinances, and the city
// simulator clock. Definitions stay in civic_simulators.h. These aliases
// group that surface under opensc4::sim.

#include "opensc4/civic_simulators.h"

namespace opensc4::sim {

using FloraSimulatorSaveRecord = opensc4::FloraSimulatorSaveRecord;
using FloraTuning = opensc4::FloraTuning;
using RZRandomState = opensc4::RZRandomState;

using MySimAgentSimulatorSaveRecord = opensc4::MySimAgentSimulatorSaveRecord;
using NeighborsSimulatorSaveRecord = opensc4::NeighborsSimulatorSaveRecord;
using OrdinanceSimulatorSaveRecord = opensc4::OrdinanceSimulatorSaveRecord;

using WeatherByteGrid2D = opensc4::WeatherByteGrid2D;
using WeatherSimulatorSaveRecord = opensc4::WeatherSimulatorSaveRecord;
using WeatherTuning = opensc4::WeatherTuning;

using SimulatorTuning = opensc4::SimulatorTuning;
using SimulatorAgentInfo = opensc4::SimulatorAgentInfo;
using SimulatorSaveRecord = opensc4::SimulatorSaveRecord;

using cSC4GameOfLifeSimulator = opensc4::cSC4GameOfLifeSimulator;
using RigidBody2DObject = opensc4::RigidBody2DObject;
using cSC4RigidBodySimulator2D = opensc4::cSC4RigidBodySimulator2D;

}  // namespace opensc4::sim
