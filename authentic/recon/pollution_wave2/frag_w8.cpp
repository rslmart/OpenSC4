// =============================================================================
// Swarm wave 1 — W8: cSC4PollutionSimulator recycling/capacity cluster
// Fragment file: spliced into the wave build by the conductor. DO NOT edit repo
// files from here.
//
// Methods covered (all addresses fresh from /tmp/swarm2/w8_capacity_asm.md):
//   GetRecyclingCenterMaxCapacity        @ 0x10009921a
//   GetRecyclingCenterCurrentCapacity    @ 0x10009924a
//   GetMaxCapacity                       @ 0x10009929e
//   GetCurrentCapacity                   @ 0x1000992cc
//   GetIncineratorCurrentCapacity        @ 0x100099414
//   GetWasteToEnergyCurrentCapacity     @ 0x10009943e
//   GetWaterTreatmentMaxCapacity         @ 0x100099468
//   GetWaterTreatmentCurrentCapacity     @ 0x100099492
//   GetWaterTreatmentUsedCapacity        @ 0x1000994bc
//   GetCurrentOccupantCondition           @ 0x1000994e6
//   GetCurrentOccupantEfficiency         @ 0x1000995b2
//   GetUnusedCapacity                    @ 0x10009962c
//   GetIncineratorUsedCapacity           @ 0x100099698
//   GetWasteToEnergyUsedCapacity         @ 0x1000996c6
//   CalculateRecyclingCenterEffect       @ 0x100099954
//
// STATUS: SKELETON — seam names provisional (OccupantSeam /
// OccupantPropertyHolderSeam names pending header_contract.md); constants
// pending shared_constants.md (NEVER retype bit patterns from memory).
// =============================================================================

#include "opensc4/civic_simulators.h"  // Seam contracts ("Swarm wave 1")

#include <cmath>
#include <cstdint>

namespace {

// ---------------------------------------------------------------------------
// Constants — values MUST come verbatim from /tmp/swarm2/shared_constants.md.
// TODO(w8): fill exact bit patterns from the shared table, never from memory.
// ---------------------------------------------------------------------------

// kSC4PercentageZero @ 0x100b78a18 (raw 4-byte copy at 0x100099223 / 0x10009928b / ...)
constexpr float kSC4PercentageZero = 0.0f;        // TODO(w8) confirm from shared_constants.md
// kSC4PercentageOneHundred @ 0x100b78a1c (raw 4-byte copy at 0x1000994f8)
constexpr float kSC4PercentageOneHundred = 100.0f; // TODO(w8) confirm from shared_constants.md
// 0x100a15928 (mulss at 0x10009927c): 0x3c23d70a
constexpr float kRecyclingPercentScale = 0.009999999776482582f; // TODO(w8) verify bits from shared table
// 0x100a158b8 (addss at 0x1000993bc): 0x4b400000 = 2^23 float-rounding magic add
constexpr float kFloatRoundMagicAdd = 8388608.0f;  // TODO(w8) verify bits from shared table
// integer add to the bit pattern at 0x1000993c8: 0xb4c00000 (== -0x4b400000 mod 2^32)
constexpr std::uint32_t kFloatRoundMagicSubBits = 0xb4c00000u;

// Property IDs (uint32 selectors, verbatim from the asm packet).
constexpr std::uint32_t kPropRecyclingCapacity     = 0x88ed1059u; // Max+Current recycling capacity
constexpr std::uint32_t kPropIncineratorCurrent    = 0xa8ebcf0cu; // @ 0x10009942e
constexpr std::uint32_t kPropWasteToEnergyCurrent  = 0x48ebd0b1u; // @ 0x100099458
constexpr std::uint32_t kPropWaterTreatmentCap     = 0xc8ebd131u; // @ 0x100099482 / 0x1000994ac
constexpr std::uint32_t kPropWaterTreatmentUsed    = 0x28ecf510u; // @ 0x1000994d6
constexpr std::uint32_t kPropConditionCurve        = 0x8a1e07eeu; // @ 0x10009935c / 0x100099510 (cISCProperty* via holder vt+0x28)
constexpr std::uint32_t kPropConditionCurveInput   = 0xc910b4cfu; // @ 0x100099393 / 0x100099544 (float)
constexpr std::uint32_t kPropIncineratorUsed       = 0xe8ecf641u; // @ 0x1000996b0
constexpr std::uint32_t kPropWasteToEnergyUsed    = 0x88ecf76cu; // @ 0x1000996de
constexpr std::uint32_t kPropRecyclingTonsCapacity = 0x68ed1736u; // @ 0x100099ace (float, effect loop)

// ---------------------------------------------------------------------------
// cvttss2si (64-bit form) semantics @ 0x100099344: invalid (NaN / out of
// int64 range) -> 0x8000000000000000; the store at 0x100099349 keeps the low
// 32 bits, so e.g. 2^31+5 wraps to 5 and NaN lands at 0. Modeled without UB.
// ---------------------------------------------------------------------------
inline std::uint32_t cvttss2si_r64_low32(float f)
{
    // Exact cvttss2si r64, xmm0 @ 0x100099344 (truncate toward zero).
    if (std::isnan(f) || f >= 9223372036854775808.0f || f < -9223372036854775808.0f) {
        return 0u; // 0x8000000000000000 low 32 bits
    }
    return static_cast<std::uint32_t>(static_cast<std::int64_t>(f));
}

// Bit-exact float-round idiom @ 0x1000993b8-0x1000993cd:
//   v' = v * capacity + 2^23f (addss, round-to-nearest-even ULP at 2^23)
//   out = bitcast(v') + 0xb4c00000 (INTEGER add on the bit pattern == -2^23)
inline std::uint32_t round_via_magic_add(float v)
{
    const float t = v + kFloatRoundMagicAdd;                  // addss @ 0x1000993bc
    std::uint32_t bits;
    static_assert(sizeof(bits) == sizeof(t), "");
    std::memcpy(&bits, &t, sizeof(bits));
    return bits + kFloatRoundMagicSubBits;                    // add eax, 0xb4c00000 @ 0x1000993c8
}

} // namespace

// =============================================================================
// GetRecyclingCenterMaxCapacity @ 0x10009921a (48 bytes)
// out = kSC4PercentageZero; return GetPropertyValue(holder, 0x88ed1059, out)
// (tail jmp @ 0x100099245). SC4Percentage& modeled as Float32& per contract.
// =============================================================================
bool cSC4PollutionSimulator::GetRecyclingCenterMaxCapacity(
    cISC4Occupant* occupant, Float32& outPercentage) const
{
    outPercentage = kSC4PercentageZero;              // 0x100099223-0x100099229 (raw copy @ 0x100b78a18)
    cISCPropertyHolder* holder =
        occupant->OccupantPropertyHolderSeam();      // 0x10009922b-0x100099231 (occupant vt+0x30) TODO(w8) seam name
    return cSCPropertyHelp::GetPropertyValue(holder, kPropRecyclingCapacity,
                                            outPercentage); // 0x100099234-0x100099245
}

// =============================================================================
// GetRecyclingCenterCurrentCapacity @ 0x10009924a (84 bytes)
// raw(0x88ed1059) via this->vt+0x1a8 (GetCurrentCapacity virtual); success:
// out = (float)(uint32)raw * 0x3c23d70a; failure: out = kSC4PercentageZero,
// return the callee's false. NOTE cvtsi2ss @ 0x10009927c uses rcx where ecx was
// a 32-bit load => ZERO-extension: (float)(uint32)raw, never sign-extended.
// =============================================================================
bool cSC4PollutionSimulator::GetRecyclingCenterCurrentCapacity(
    cISC4Occupant* occupant, Float32& outPercentage) const
{
    std::uint32_t raw = 0;                           // [rbp-0x14] (callee writes both paths)
    if (GetCurrentCapacity(kPropRecyclingCapacity, occupant, raw)) { // this->vt+0x1a8 @ 0x10009926a
        outPercentage =
            static_cast<float>(raw) * kRecyclingPercentScale;      // 0x100099274-0x100099284
        return true;                                 // al from the virtual call @ 0x100099270
    }
    outPercentage = kSC4PercentageZero;              // 0x10009928b-0x100099291 (raw copy)
    return false;
}

// =============================================================================
// GetMaxCapacity @ 0x10009929e (46 bytes)
// QUIRK: no dispatch table — the uint "type" IS the property ID, forwarded
// verbatim to GetPropertyValue. *out preset to 0 before the lookup.
// =============================================================================
bool cSC4PollutionSimulator::GetMaxCapacity(
    std::uint32_t capacityType, cISC4Occupant* occupant, std::uint32_t& outCapacity) const
{
    outCapacity = 0;                                 // 0x1000992ab
    cISCPropertyHolder* holder =
        occupant->OccupantPropertyHolderSeam();      // 0x1000992b1-0x1000992b7 (occupant vt+0x30) TODO(w8) seam name
    return cSCPropertyHelp::GetPropertyValue(holder, capacityType,
                                             outCapacity); // 0x1000992ba-0x1000992c7 (tail jmp @ 0x10004b5ac)
}

// =============================================================================
// GetCurrentCapacity @ 0x1000992cc (328 bytes)
// Pipeline: property(type) -> optional field060->vt+0xf8(occupant) gate ->
// scale by this->field348 (mulss) -> cvttss2si r64 low-32 store -> condition
// curve re-scale via the 2^23 magic-add rounding idiom.
// TODO(w8): fill body from the incremental pass (seams: field060 object,
// field348 offset name, response-curve seam).
// =============================================================================
bool cSC4PollutionSimulator::GetCurrentCapacity(
    std::uint32_t capacityType, cISC4Occupant* occupant, std::uint32_t& outCapacity) const
{
    cISCPropertyHolder* holder =
        occupant->OccupantPropertyHolderSeam();      // 0x1000992e9-0x1000992ef (occupant vt+0x30)
    if (!cSCPropertyHelp::GetPropertyValue(holder, capacityType,
                                          outCapacity)) {         // 0x1000992f2-0x100099300
        outCapacity = 0;                              // 0x100099322 (explicit zero on failure)
        return false;                                // 0x100099329 (r14d = 0)
    }
    float scaled;
    if (/* this->field060->vt+0xf8(occupant) @ 0x100099304-0x100099316 */ true) {
        scaled = static_cast<float>(outCapacity);     // 0x100099318-0x100099320 (cvtsi2ss from [out])
    } else {
        outCapacity = 0;                              // 0x100099331
        scaled = 0.0f;                                // 0x100099338 (xorps)
    }
    scaled *= /* this->field348 @ 0x10009933b (mulss [r14+0x348]) */ 1.0f; // TODO(w8) seam name
    outCapacity = cvttss2si_r64_low32(scaled);        // 0x100099344-0x10009934c
    // TODO(w8): condition-curve block @ 0x10009934c-0x1000993dd (incremental pass).
    return true;                                     // r14b = 1 @ 0x10009936a
}

// =============================================================================
// Four forwarding getters (42 bytes each): *out = 0; tail-jmp to a virtual
// property getter on `this` (slot cited per method). SC4Percentage-less uint&.
// =============================================================================
// GetIncineratorCurrentCapacity @ 0x100099414: this->vt+0x1a8 (GetCurrentCapacity), prop 0xa8ebcf0c
// GetWasteToEnergyCurrentCapacity @ 0x10009943e: this->vt+0x1a8 (GetCurrentCapacity), prop 0x48ebd0b1
// GetWaterTreatmentCurrentCapacity @ 0x100099492: this->vt+0x1a8 (GetCurrentCapacity), prop 0xc8ebd131
// GetWaterTreatmentMaxCapacity @ 0x100099468: this->vt+0x1a0 (GetMaxCapacity), prop 0xc8ebd131
// GetWaterTreatmentUsedCapacity @ 0x1000994bc: this->vt+0x1a0 (GetMaxCapacity), prop 0x28ecf510
// TODO(w8): confirm slot identities via --vtable cSC4PollutionSimulator, then
// emit the five bodies here in the incremental pass.

// =============================================================================
// GetCurrentOccupantCondition @ 0x1000994e6 (204 bytes)
// out = kSC4PercentageOneHundred; if the condition response-curve property
// (0x8a1e07ee) exists: out = curve(GetPropertyValue(0xc910b4cf)) * 100.0f.
// Return = (curve property != null). GetPropertyValue bool IGNORED @ 0x10009954c.
// =============================================================================
bool cSC4PollutionSimulator::GetCurrentOccupantCondition(
    cISC4Occupant* occupant, Float32& outPercentage) const
{
    outPercentage = kSC4PercentageOneHundred;        // 0x1000994f8-0x1000994fe (raw copy @ 0x100b78a1c)
    cISCPropertyHolder* holder =
        occupant->OccupantPropertyHolderSeam();      // 0x100099501-0x100099507 (occupant vt+0x30) TODO(w8) seam name
    cISCProperty* curveProp =
        holder-> /* vt+0x28 (property lookup) @ 0x10009950d-0x100099518 */ nullptr; // TODO(w8) seam name
    if (curveProp == nullptr) {                      // 0x10009951e-0x100099521
        return false;                                // cl = 0 @ 0x10009957f
    }
    cSC4ResponseCurve curve;                          // ctor @ 0x10009952a (0x1000ac10e)
    curve.SetResponseCurveData(curveProp);           // 0x10009952f (0x1000ac37a)
    float curveInput = 0.0f;                          // [rbp-0x24] @ 0x10009953e
    cSCPropertyHelp::GetPropertyValue(holder, kPropConditionCurveInput,
                                      curveInput);   // 0x100099544 (bool result IGNORED)
    outPercentage =
        curve.GetOutputValue(curveInput) * outPercentage; // 0x100099551-0x100099564 (mulss by [r14] = 100.0f)
    return true;                                     // cl = 1 @ 0x10009957b
}

// =============================================================================
// GetCurrentOccupantEfficiency @ 0x1000995b2 (122 bytes)
// Gate on SL::spPowerSim (global @ 0x100b21470) ->vt+0xf8(occupant); NOTE the
// power-sim call receives `occupant` in rsi (live from entry, never reloaded).
// Then out = this->field348; virtual this->vt+0x1c0 (GetCurrentOccupantCondition)
// -> out *= f. Any failure: out = kSC4PercentageZero, return false.
// =============================================================================
bool cSC4PollutionSimulator::GetCurrentOccupantEfficiency(
    cISC4Occupant* occupant, Float32& outPercentage) const
{
    if (!/* SL::spPowerSim->vt+0xf8(occupant) @ 0x1000995c5-0x1000995da */ true) { // TODO(w8) seam name
        outPercentage = kSC4PercentageZero;          // 0x100099615-0x10009961b
        return false;                                // 0x10009961e
    }
    outPercentage = /* this->field348 raw float @ 0x1000995dc-0x1000995e2 */ 0.0f; // TODO(w8) seam name
    Float32 condition = 0.0f;                        // [rbp-0x20] @ 0x1000995e9
    if (!/* this->vt+0x1c0 (GetCurrentOccupantCondition) @ 0x1000995ef-0x100099600 */ true) {
        outPercentage = kSC4PercentageZero;          // 0x100099615-0x10009961b
        return false;                                // 0x10009961e
    }
    outPercentage = condition * outPercentage;       // 0x100099602-0x10009960c (mulss xmm0,[r14])
    return true;                                     // 0x100099611
}

// =============================================================================
// GetUnusedCapacity @ 0x10009962c (108 bytes)
// QUIRKS: (1) *out is NEVER pre-zeroed and NEVER written on any failure path —
// caller-visible garbage passes through (0x10009968a / 0x100099678 both return
// without touching [r8]). (2) The unsigned sub + cmovb @ 0x10009967d-0x100099680
// clamps current<used to 0. current comes from this->vt+0x1a8 (GetCurrentCapacity)
// with the FIRST uint as its property ID; used from GetPropertyValue(holder,
// second uint). The `used` local IS pre-zeroed @ 0x10009963f (the `current`
// local is not, but the callee writes both paths).
// =============================================================================
bool cSC4PollutionSimulator::GetUnusedCapacity(
    std::uint32_t capacityType, std::uint32_t usedType,
    cISC4Occupant* occupant, std::uint32_t& outCapacity) const
{
    std::uint32_t current = 0;                        // [rbp-0x20] (callee-written)
    if (!GetCurrentCapacity(capacityType, occupant, current)) { // this->vt+0x1a8 @ 0x100099646-0x100099658
        return false;                                 // 0x10009968a (out untouched)
    }
    cISCPropertyHolder* holder =
        occupant->OccupantPropertyHolderSeam();       // 0x10009965a-0x100099660 (occupant vt+0x30) TODO(w8) seam name
    std::uint32_t used = 0;                           // [rbp-0x1c] pre-zeroed @ 0x10009963f
    if (!cSCPropertyHelp::GetPropertyValue(holder, usedType, used)) { // 0x100099663-0x100099678
        return false;                                 // out untouched
    }
    std::uint32_t unused = current - used;            // sub @ 0x10009967d (unsigned, sets CF)
    if (current < used) {                             // cmovb @ 0x100099680 (CF => 0)
        unused = 0;
    }
    outCapacity = unused;                             // 0x100099683
    return true;                                      // 0x100099686
}

// =============================================================================
// GetIncineratorUsedCapacity @ 0x100099698 / GetWasteToEnergyUsedCapacity @
// 0x1000996c6 (46 bytes each): *out = 0; tail-jmp GetPropertyValue(holder,
// {0xe8ecf641 | 0x88ecf76c}, out).
// =============================================================================
bool cSC4PollutionSimulator::GetIncineratorUsedCapacity(
    cISC4Occupant* occupant, std::uint32_t& outCapacity) const
{
    outCapacity = 0;                                   // 0x1000996a1
    cISCPropertyHolder* holder =
        occupant->OccupantPropertyHolderSeam();       // 0x1000996a7-0x1000996ad TODO(w8) seam name
    return cSCPropertyHelp::GetPropertyValue(holder, kPropIncineratorUsed,
                                             outCapacity); // 0x1000996b0-0x1000996c1 (tail jmp)
}

bool cSC4PollutionSimulator::GetWasteToEnergyUsedCapacity(
    cISC4Occupant* occupant, std::uint32_t& outCapacity) const
{
    outCapacity = 0;                                   // 0x1000996cf
    cISCPropertyHolder* holder =
        occupant->OccupantPropertyHolderSeam();       // 0x1000996d5-0x1000996db TODO(w8) seam name
    return cSCPropertyHelp::GetPropertyValue(holder, kPropWasteToEnergyUsed,
                                             outCapacity); // 0x1000996de-0x1000996ef (tail jmp)
}

// =============================================================================
// CalculateRecyclingCenterEffect @ 0x100099954 (620 bytes)
// Full pipeline: unconditional field050->vt+0x80() need value; empty-member
// guard on field208; back-walk the field108 member list, insertion-sort
// occupants by this->vt+0x1b8 (GetRecyclingCenterCurrentCapacity, float key)
// into a local list; assign into the member list; greedy accumulate
// 0x68ed1736 prop values until the need is met; field240 = weighted / max(need,
// sum). ucomiss+maxss NaN paths cited inline below.
// TODO(w8): fill body in the incremental pass once the list-walk direction
// (libc++ __list_node next/prev offsets) is settled with fresh tool output.
// =============================================================================
void cSC4PollutionSimulator::CalculateRecyclingCenterEffect()
{
    // TODO(w8) incremental pass: full faithful body.
}
