// frag_w7.cpp — swarm wave 2, worker W7
// cSC4PollutionSimulator occupant-list methods, clean-room reconstruction.
// Every behavioral fact cites the machine-code address it came from.
//
// Methods (all on cSC4PollutionSimulator @ the cited addresses):
//   ResetOccupantListCapacityUsedProperty(ilist<cISC4Occupant*>&, uint, uint) @ 0x100099bc0
//   AgeOccupantList(ilist<cISC4Occupant*>&, uint, uint)                     @ 0x100099c88
//   ComputeWasteToEnergyListCurrentCapacity()                               @ 0x100099f00
//   SetOccupantListUsedCapacity(ilist&, uint, uint, uint, uint&, uint)      @ 0x10009a004
//   AddOrRemoveOccupant(ilist&, cISC4Occupant*, bool)                       @ 0x10009aa2a
//
// ilist layout (wave 1): base = sentinel, +0=prev/tail, +8=next/head, +0x10=size.
// Occupants are COM objects; property access routes through the wave-1 seams.

#include <cstdint>
#include <cstring>
#include <vector>

#include "opensc4/civic_simulators.h"

namespace opensc4 {
namespace pollution_simulator_w7 {

// TODO(w7): fill after header contract read.
void ResetOccupantListCapacityUsedProperty(/* PollutionSimulatorSaveRecord& record, OccupantRefList& list, seam..., uint arg2, uint arg3 */)
{
    // Exact cSC4PollutionSimulator::ResetOccupantListCapacityUsedProperty @ 0x100099bc0
}

void AgeOccupantList(/* ... */)
{
    // Exact cSC4PollutionSimulator::AgeOccupantList @ 0x100099c88
}

void ComputeWasteToEnergyListCurrentCapacity(/* ... */)
{
    // Exact cSC4PollutionSimulator::ComputeWasteToEnergyListCurrentCapacity @ 0x100099f00
}

void SetOccupantListUsedCapacity(/* ... */)
{
    // Exact cSC4PollutionSimulator::SetOccupantListUsedCapacity @ 0x10009a004
}

bool AddOrRemoveOccupant(/* ... */)
{
    // Exact cSC4PollutionSimulator::AddOrRemoveOccupant @ 0x10009aa2a
    return false;
}

} // namespace pollution_simulator_w7
} // namespace opensc4
