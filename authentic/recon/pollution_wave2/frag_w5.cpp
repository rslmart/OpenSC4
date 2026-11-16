// W5 fragment: cSC4PollutionSimulator lifecycle methods.
// Bodies only — no includes. Style per /tmp/swarm2/header_contract.md.

// Exact cSC4PollutionSimulator::DoMessage(cIGZMessage2*) @ 0x100096a58
// Decode notes:
//   - message type id via vtable+0x20 (Sint8 -> 32-bit id in ebx).
//   - dispatch on raw ids (NO invented names):
//     0xca1b151c -> visual-effect toggle path (vtable+0x78 GZString)
//     0x26d31ec1 / 0x66956813 -> UpdatePollutionVisualEffect()
//     0x66956810 -> SimulationBegin()
//     0x66956814 -> DoDailySimulation()
//     0x66956816 -> DoMonthlySimulation()
//     0xc6cdbf1a -> ReadSimulatorTunableValues()
//     {0x99ef1142, 0x99ef1143} band (lea/add trick, below) -> DoOccupantChanged(occupant, isCreate)
//   - the "lea eax, [rbx + 0x6610eebe]; cmp eax, 2; jb" encodes two adjacent
//     ids: 0x99ef1142 and 0x99ef1143 (0x99ef1142 + 0x6610eebe = 0 wraps to
//     0; +1 -> 1; jb 2 accepts both) — verify exact band edges below.
//   - always returns true (mov al, 0x1) @ 0x100096b4c.

// Exact cSC4PollutionSimulator::SimulationBegin() @ 0x100098006
// Decode notes:
//   - local cSC4ResponseCurve ctor @ 0x1000ac10e.
//   - resource key {0xE7E2C28D9A34284Aull? NO — movabs -0x181d3d249acbd7b6
//     = 0xE7E2C28D9A34284A? verify: -0x181d3d249acbd7b6 as u64 = 0xE7E2C2D B...
//     (compute exact in body comment); instance 0x6911e2df; group 0xc911e35b}.
//   - reads Sint64 (vtable+0x50) from field1c0, treats result as float*,
//     dereferences [rax] -> xmm0; mulss by 100.0f (0x42c80000 @ 0x100a161f4);
//     GetOutputValue; mulss by 0.01f (0x3c23d70a @ 0x100a15928); store field348.

// Exact cSC4PollutionSimulator::EndOfCycle() @ 0x1000996f4
// Decode notes:
//   - CalculateRecyclingCenterEffect() first.
//   - field244 = (field248 != 0).
//   - if field22c (population denom) != 0: field20c = field214 / field22c
//     (u32 div), field210 = field218 / field22c, field234(u16) = field248 /
//     field22c, r14 = field230 * 100 / field22c; else all four = 0.
//   - field224(qword) = field21c(qword).
//   - SL::spHistoryWarehouse->Set(0x0a1100e8, field20c), (0xea110130,
//     field210), (0xea110134, field234 zero-ext), (0xea110137, field230),
//     (0xea110138, field20c + garbage-avg term r14).
//   - zero qwords: field22c, field21c, field214 (all zeroed via rax=0).
//   - flag-post block: field328 -> SC4PostMessage2(0xda6dd35d,0,0,0,0,null);
//     field329 -> 0xfa6dd0bb; field32a -> 0x5a6dd1aa; field32b -> 0xda6dd2cb.
//   - field328 = 0 (byte).
//   - rebuild curve (same key as SimulationBegin) -> field348.
//   - SC4PostMessage2(0xea418d8f, 8, 0, 0, 0, null).

// === bodies appended below after decode ===
