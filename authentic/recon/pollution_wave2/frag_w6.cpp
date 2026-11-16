// frag_w6.cpp — cSC4PollutionSimulator swarm wave-2 worker W6
// Methods: SetIsRadioactive, SetAirValue, SetWaterValue, SetGarbageValue,
//          GetBuildingGarbageCapacity, IsActiveLandfill, SetIsActiveLandfill,
//          AddRadiationToCity.
// Every behavioral fact cites its address in the original Mach-O.

#include "opensc4/civic_simulators.h"  // TODO(confirm include) after shared docs

// ---------------------------------------------------------------------------
// SetIsRadioactive(unsigned int x, unsigned int y, bool radioactive)
// Exact cSC4PollutionSimulator::SetIsRadioactive @ 0x10009a738
// ---------------------------------------------------------------------------
// SysV: rdi=this, esi=x, edx=y, ecx=radioactive.
// Grid = [this+0x90] (radiation grid). shift = (uint)grid.vt58() (Uint64 slot,
// low 32 bits used as shift count, @0x10009a759/@0x10009a75c).
// Setter vtC8 called with (x >> shift, y >> shift, value) @0x10009a769-0x10009a777.
// Value widening: movzx ecx, r14b @0x10009a76e (zero-extend the bool BYTE).
// Dirty flag: byte [this+0x328] = 1 @0x10009a77d.
// NO bounds gate on x/y: the grid setter receives whatever (x>>shift, y>>shift)
// produces — unchecked, the bytes show no compare. Oddity noted, not fixed.

// ---------------------------------------------------------------------------
// SetAirValue(unsigned int x, unsigned int y, int value)
// Exact cSC4PollutionSimulator::SetAirValue @ 0x10009a790
// ---------------------------------------------------------------------------
// Same shape as SetIsRadioactive but grid=[this+0x98] (air), dirty=[this+0x329]
// @0x10009a7d5. Value widening: movsx ecx, r14w @0x10009a7c6 — the int argument
// is TRUNCATED TO 16 BITS and sign-extended. Quirk: value 0x10001 lands as +1,
// value 0x8000 lands as -32768.

// ---------------------------------------------------------------------------
// SetWaterValue(unsigned int x, unsigned int y, int value)
// Exact cSC4PollutionSimulator::SetWaterValue @ 0x10009a7e8
// ---------------------------------------------------------------------------
// Grid=[this+0xa0] (water), dirty=[this+0x32a] @0x10009a82d.
// Same movsx-from-16-bit value widening @0x10009a81e.

// ---------------------------------------------------------------------------
// SetGarbageValue(unsigned int x, unsigned int y, int value)
// Exact cSC4PollutionSimulator::SetGarbageValue @ 0x10009a840
// ---------------------------------------------------------------------------
// Grid=[this+0xa8] (garbage). Read-modify-write on the 16-bit cell:
//   cur  = grid.vt38(x, y)            @0x10009a878  (getter takes RAW x,y)
//   tmp  = (cur & 0xffff8000) | value  @0x10009a87b-0x10009a880  (OR, not store:
//          previous low bits survive if value doesn't clear them — quirk)
//   out  = (int16_t)tmp               @0x10009a895 (movsx from ax)
//   grid.vtC8(x >> shift, y >> shift, out)  @0x10009a88d-0x10009a89d
//          — SETTER takes SHIFTED coords, getter takes RAW: asymmetry is in
//          the bytes, reproduced as-is.
// Dirty flag: byte [this+0x32b] = 1 @0x10009a8a4.

// ---------------------------------------------------------------------------
// GetBuildingGarbageCapacity(cISC4Occupant* occupant) const
// Exact cSC4PollutionSimulator::GetBuildingGarbageCapacity @ 0x10009a8e2
// ---------------------------------------------------------------------------
// holder = occupant->vt30()  @0x10009a8e8-0x10009a8ee
// int out; ok = cSCPropertyHelp::GetPropertyValue(holder, 0x48ee7a44, out)
//   @0x10009a8fd
// return ok ? out : 0   (xor ecx,ecx; test al,al; cmovne ecx,[rbx]
//   @0x10009a905-0x10009a90c)

// ---------------------------------------------------------------------------
// IsActiveLandfill(unsigned int x, unsigned int y) const
// Exact cSC4PollutionSimulator::IsActiveLandfill @ 0x10009a924
// ---------------------------------------------------------------------------
// cell = grid([this+0xa8]).vt38(x, y)   @0x10009a928-0x10009a932 — RAW coords,
//   no shift applied (contrast SetGarbageValue's shifted setter call).
// return (cell >> 15) & 1   @0x10009a935-0x10009a93a

// ---------------------------------------------------------------------------
// SetIsActiveLandfill(unsigned int x, unsigned int y, bool active)
// Exact cSC4PollutionSimulator::SetIsActiveLandfill @ 0x10009a940
// ---------------------------------------------------------------------------
// gate40 = [this+0x40]->vt60() compared (as int32) to 14  @0x10009a957-0x10009a964
//   if (gate40 != 14 && active) return;   @0x10009a966-0x10009a969
// ret = this->vt188(x, y)  @0x10009a96f-0x10009a97b
// SET path (ret == 0 && active):
//   cell = grid.vt38(x, y); cell |= 0xffff8000; ++[this+0x2cc];
//   grid.vtC8(x, y, cell)  — RAW coords, FULL 32-bit value (mov ecx,eax
//   @0x10009a9b8), dirty [this+0x32b]=1  @0x10009aa09
// CLEAR path (ret == 1 exactly && !active):
//   cell = grid.vt38(x, y); cell &= 0x7fff;
//   if ([this+0x2cc] != 0) --[this+0x2cc];     @0x10009a9dd-0x10009a9e7
//   grid.vtC8(x, y, (int16_t)cell) — movsx @0x10009a9f9, dirty [this+0x32b]=1
// Otherwise: return with no effect.

// ---------------------------------------------------------------------------
// AddRadiationToCity(float* pos, float radius, float strength)
// Exact cSC4PollutionSimulator::AddRadiationToCity @ 0x10009aaf4
// ---------------------------------------------------------------------------
// Gate 1 (radius): ucomiss radius, 0; jne/jnp @0x10009ab0e-0x10009ab13 —
//   proceed iff radius != ±0 OR radius is NaN (NaN falls through BOTH jumps:
//   unordered sets ZF=1 so jne is not taken, PF=1 so jnp is not taken).
// Gate 2 (strength): ucomiss strength, 0; jbe @0x10009ab1c —
//   proceed iff strength > 0 AND not NaN (jbe taken on unordered → NaN aborts).
//   Asymmetry is deliberate in the bytes: NaN radius proceeds, NaN strength
//   does not.
// grid = [this+0x90] (radiation)
// xmm0 = pos[0], xmm1 = pos[2]  @0x10009ab2c-0x10009ab35 (pos is float*,
//   +0 and +8 read — x and z of a 3-float position)
// grid.vt98(&outX, &outY, pos[0], pos[2])  @0x10009ab4d — world→grid coord
//   conversion writing two int32 locals ([rbp-0x38], [rbp-0x34])
// A = (int)grid.vt60()  @0x10009ab5d-0x10009ab60
// B = (int)grid.vt68()  @0x10009ab6d-0x10009ab70
// grid.vt50() called, result DISCARDED  @0x10009ab7d-0x10009ab80 (oddity:
//   a Sint64-slot getter whose return is never used — call reproduced for
//   fidelity in case the slot has side effects)
// data = grid.vtd8()  @0x10009ab8a — std::vector<std::vector<short>>&
// cSC4MapUtilities::AddRadiusEffect(data, A, B, outX, outY, strength, radius, 0.0f)
//   @0x10009abac — exact arg order: xmm0=strength([rbp-0x30]), xmm1=radius
//   ([rbp-0x2c]), xmm2=0 (xorps @0x10009ab96)
// UpdatePollutionVisualEffect()  @0x10009abb4
// SC4PostMessage2(0xda6dd35d, 0, 0, 0, 0, nullptr)  @0x10009abb9-0x10009abca
