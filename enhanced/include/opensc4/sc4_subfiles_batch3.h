#pragma once

// Exact decompilation of the Batch 3 .sc4 save subfiles (advisors, advice,
// MySims) from the macOS x86_64 SimCity 4 Deluxe binary:
//
//   - cSC4Advice (Read @ 0x10043e2fa, Write @ 0x10043e1dc), shared by
//     cSC4Advice (0x8a09f5f4), cSC4MySimAdvice (0x6a9335de),
//     cSC4CitySituationAdvice (0x8be3753b), cSC4RewardAdvice (0xaa371c32) and
//     cSC4MySimBalloonAdvice (0x6b70efad, Write @ 0x1006aac20 tail-calls Advice)
//   - cSC4AdviceNeghborDeal (0x6a3848db, Read @ 0x1004414b8, Write @ 0x100441460)
//   - cSC4AdviceSubject (0x6a26d049, Read @ 0x10044bfec, Write @ 0x10044bf4c)
//   - cSC4Advisor (Read @ 0x10044406a, Write @ 0x100443eaa), used directly by
//     cSC4Advisor (0xeac937b4), cSC4AdvisorTraffic (0x4a3ad3e1),
//     cSC4AdvisorCitySituations (0x4be372cd), cSC4AdvisorHQ (0x6a5f8755),
//     cSC4AdvisorFinance (0xaa5292d7), cSC4AdvisorUtility (0xca2c2a2f),
//     cSC4AdvisorSafety (0xea19e825)
//   - cSC4AdvisorCityPlanning (0x2a5f877d, Read @ 0x1004025aa, Write @ 0x1004024ac)
//   - cSC4AdvisorSystem (0x4a0dd616, Read @ 0x100297764, Write @ 0x1002976a6)
//   - cSC4MySim (0x4a1dbbbf, Read @ 0x1007a2b1c, Write @ 0x1007a27ce; derives
//     from cSC4Advisor) and cSC4MyStreetSim (0x0bf606a5, Write @ 0x1006b9e4c)

#include "opensc4/sc4_subfiles.h"

#include <array>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace opensc4 {

// CLSIDs of every class whose save record is exactly cSC4Advice::Write.
inline constexpr std::array<GZCLSID, 5> kAdviceRecordCLSIDs = {
    0x8a09f5f4u, // cSC4Advice
    0x6a9335deu, // cSC4MySimAdvice
    0x8be3753bu, // cSC4CitySituationAdvice
    0xaa371c32u, // cSC4RewardAdvice
    0x6b70efadu, // cSC4MySimBalloonAdvice
};

// CLSIDs of every advisor whose save record is exactly cSC4Advisor::Write.
inline constexpr std::array<GZCLSID, 7> kAdvisorRecordCLSIDs = {
    0xeac937b4u, // cSC4Advisor
    0x4a3ad3e1u, // cSC4AdvisorTraffic
    0x4be372cdu, // cSC4AdvisorCitySituations
    0x6a5f8755u, // cSC4AdvisorHQ
    0xaa5292d7u, // cSC4AdvisorFinance
    0xca2c2a2fu, // cSC4AdvisorUtility
    0xea19e825u, // cSC4AdvisorSafety
};

constexpr GZCLSID kCLSID_cSC4AdviceNeghborDeal    = 0x6a3848dbu;
constexpr GZCLSID kCLSID_cSC4AdviceSubject        = 0x6a26d049u;
constexpr GZCLSID kCLSID_cSC4AdvisorCityPlanning  = 0x2a5f877du;
constexpr GZCLSID kCLSID_cSC4AdvisorSystem        = 0x4a0dd616u;
constexpr GZCLSID kCLSID_cSC4MySim                = 0x4a1dbbbfu;
constexpr GZCLSID kCLSID_cSC4MyStreetSim          = 0x0bf606a5u;

// cSC4Advice (current version 4; version 3 lacks the Uint32 list at +0x128).
struct AdviceSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 4;
    Sint32 fieldD8 = 0;
    Sint32 fieldDC = 0;
    std::array<Sint32, 6> fieldC0{}; // +0xc0..+0xd4
    std::vector<Uint32> list128;     // std::list<Uint32> (version >= 4)
    SimSerializableRef subject110{}; // cISC4AdviceSubject (IID 0x6a26d050)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const { return WriteWithVersion(stream, 4); }
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;
};

struct AdviceNeghborDealSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    AdviceSaveRecord advice;
    Sint32 field140 = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;
};

struct AdviceSubjectSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 4;
    Sint8 field20 = 0;     // when non-zero the two floats follow
    Float32 field24 = 0.0f;
    Float32 field28 = 0.0f;
    Uint32 field2C = 0;
    Uint32 field30 = 0;
    SimSerializableRef ref38{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4Advisor::cAdviceInfo as streamed (0x18-byte vector element).
struct AdvisorAdviceInfo {
    SimSerializableRef advice{}; // +0x00
    Sint32 fieldC = 0;           // +0x0c
    Sint32 field10 = 0;          // +0x10
    Sint32 field8 = 0;           // +0x08
    Sint8 flag = 0;              // advice->vtbl+0x78(2)
    Sint32 value = 0;            // advice->vtbl+0xd0()
};

struct AdvisorSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 3;
    Sint32 field28 = 0;
    std::string name30;
    std::string name60;
    std::array<Sint32, 4> field90{}; // +0x90..+0x9c
    std::vector<AdvisorAdviceInfo> advice;                   // +0xd8
    std::vector<std::pair<Sint32, SimSerializableRef>> subjects; // list +0x128

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4AdvisorCityPlanning (current version 3; version 2 lacks the map).
struct AdvisorCityPlanningSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 3;
    AdvisorSaveRecord advisor;
    Sint32 field14C = 0;
    Sint32 field150 = 0;
    Sint32 field154 = 0;
    std::vector<std::pair<Sint32, Sint32>> map158; // std::map<int,int> (version >= 3)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const { return WriteWithVersion(stream, 3); }
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;
};

struct AdvisorSystemSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    std::vector<std::pair<SimSerializableRef, Sint32>> advisors; // (advisor, advisor->vtbl+0x28())

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cRZTimer stream form (operator<< @ 0x1000ce852 / operator>> @ 0x1000ce7ca).
struct RZTimerRecord {
    Uint8 units = 0;
    Uint64 elapsed = 0;
    Uint8 running = 0;
};

// cSC4MySim (current version 17). Versions in the shipped saves: 10, 14, 16, 17.
struct MySimSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 17;
    Uint8 hasSim = 0;                 // low byte of +0x154
    Uint8 field150 = 0;
    SimSerializableRef ref158{};
    Uint32 field160 = 0;
    Uint32 field164 = 0;
    SimSerializableRef ref180{};
    std::array<Sint32, 4> field190{};
    Uint32 field1A8 = 0;
    std::array<Uint32, 3> key1AC{};   // cGZPersistResourceKey
    Uint8 field1B8 = 0;
    Uint16 field1BC = 0;
    Uint8 field1BE = 0;
    Uint8 field1BF = 0;
    std::string name1C0;
    Uint8 field1E8 = 0;
    Uint8 field1E9 = 0;
    Uint32 field1EC = 0;
    std::string name1F0;
    Uint32 field234 = 0;
    Uint8 field238 = 0;
    Uint32 field23C = 0;
    std::array<Float32, 3> field168{};
    Uint8 field2D8 = 0;
    Float32 field2DC = 0.0f;
    std::array<Float32, 6> field2A8{};
    Uint8 field2D9 = 0;
    Float32 field2E0 = 0.0f;
    std::array<Float32, 6> field2C0{};
    std::array<Sint32, 5> legacy11To14{};   // versions 11..14 (discarded on Read)
    std::array<Uint32, 3> key3C0{};         // version > 14
    std::array<Uint32, 3> key3CC{};         // version > 14
    Sint32 field31C = 0;                    // version >= 17
    Sint32 field320 = 0;                    // version >= 17
    SimSerializableRef dispatch328{};       // version > 12 (cSC4MySimDispatch)
    RZTimerRecord timer3E0{};               // version >= 12
    Uint8 hasRef3B8 = 0;                    // version >= 12
    SimSerializableRef ref3B8{};            // when hasRef3B8
    Uint8 field3B1 = 0;                     // version >= 14
    Sint32 field410 = 0;                    // version >= 16
    AdvisorSaveRecord advisor;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const { return WriteWithVersion(stream, 17); }
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;
};

// Returns false if typeId is not a Batch 3 class.
bool RoundTripSC4SubfileBatch3(Uint32 typeId, std::span<const Uint8> subfileBytes,
                               SC4SubfileRoundTripStats& stats);


// Method index for the advisor, MySim, tutorial, disaster, and audio
// classes. Each address is a leaf (`// @`) or algorithmic (`###`) entry
// point. Advice flag tests, MySim sign/attribute reads, and disaster
// phase/state tests below match those entry points.

struct Batch3EntryPoint {
    Uint64 address; // macOS image addresses sit above 4 GiB
    const char* symbol;
};

// cSC4Advice::CheckFlags @ 0x10043dfe6: (flags & mask) == mask.
// cSC4Advice::SetFlags @ 0x10043df2a tests bit 0x2 and bit 0x200.
inline constexpr Uint32 kAdviceFlagSetFlagsBit = 0x2u;
inline constexpr Uint32 kAdviceFlagSetFlagsHighBit = 0x200u;

// Popup thresholds live in a 3-slot table. init_advice_popup_priority_thresholds
// @ 0x100295eea fills it from tuning keys of length 3 ("low"), 6 ("medium"),
// and 4 ("high"; "low\0" sits immediately before "high").
// GetAdvicePopupPriorityThreshold @ 0x100297530 selects a slot from a city
// query compared with 1 and with 2; any other result uses slot 0.
inline constexpr Uint32 kAdvicePopupThresholdSlots = 3u;

// cSC4CitySituationAdvice::CheckSituationFlag @ 0x1006a9dd6: any masked bit.
// cSC4MySim::GetSign @ 0x1007a7f30 is the byte at +0x1be. MySimZodiacName labels
// 0..11 in tropical order starting at Aries.
// cSC4MySim::GetEQAsText @ 0x1007a8394 truncates the float at +0x270 (cvttss2si).
// cSC4MySim::GetHQAsText @ 0x1007a83e4 reads the float at +0x274.
// cSC4MySim::IsMale @ 0x1007a845c is the byte at +0x1bf (0 selects the night
// sound 0x4a5fc52a; any other value selects 0x6a5fc4f2). PlayNightSound
// @ 0x1007a912a only runs when presence (+0x154) equals 2.
// IsActive @ 0x1007a8472 is presence != 0.
inline constexpr Uint8 kMySimZodiacCount = 12;
inline constexpr Sint32 kMySimPresenceInactive = 0;
inline constexpr Sint32 kMySimPresenceNightSound = 2;
inline constexpr Uint32 kMySimNightSoundIdWhenSexClear = 0x4a5fc52au;
inline constexpr Uint32 kMySimNightSoundIdWhenSexSet = 0x6a5fc4f2u;
inline constexpr Sint32 kMySimBalloonQueueEvent = 1;  // GetNextBalloonEventAdvice
inline constexpr Sint32 kMySimBalloonQueueAdvice = 2; // GetNextBalloonAdvice

// cSC4DisasterInstanceFire::UpdatePhase @ 0x100450942 compares phase (+0x150)
// with 1: fires still burning at phase 1 stay put; no fires at phase 1 ends
// the instance; any other phase with fires advances.
// SetCause @ 0x10044fc18 writes only while the stored cause is 0.
// cSC4DisasterInstanceVolcano::SetState @ 0x100457e92 accepts states 0..5.
// GetPhase @ 0x1004577f2 maps states 0..4 through a 5-entry table and
// returns 2 for every later state.
// cSC4DisasterLayer::DisastersEnabled @ 0x1004620e6 is true when +0x68 is 0.
// IsADisasterOngoing @ 0x100462c60 is true when +0x70 is nonzero.
inline constexpr Sint32 kDisasterPhaseOngoing = 1;
inline constexpr Sint32 kVolcanoStateMax = 5;
inline constexpr Sint32 kVolcanoPhaseTableCount = 5;
inline constexpr Sint32 kVolcanoPhasePastTable = 2;

[[nodiscard]] inline constexpr bool AdviceCheckFlags(Sint32 flags, Sint32 mask) noexcept {
    const auto bits = static_cast<Uint32>(flags);
    const auto need = static_cast<Uint32>(mask);
    return (bits & need) == need;
}

[[nodiscard]] inline constexpr bool AdviceHasSetFlagsBit(Sint32 flags) noexcept {
    return AdviceCheckFlags(flags, static_cast<Sint32>(kAdviceFlagSetFlagsBit));
}

[[nodiscard]] inline constexpr bool AdviceHasSetFlagsHighBit(Sint32 flags) noexcept {
    return AdviceCheckFlags(flags, static_cast<Sint32>(kAdviceFlagSetFlagsHighBit));
}

[[nodiscard]] inline constexpr Sint32 AdvicePopupThresholdForCityLevel(
    const Sint32 (&thresholds)[kAdvicePopupThresholdSlots], Uint32 cityLevel) noexcept {
    const Uint32 slot = cityLevel < kAdvicePopupThresholdSlots ? cityLevel : 0u;
    return thresholds[slot];
}

[[nodiscard]] inline constexpr bool SituationCheckFlag(Uint32 flags, Uint32 bit) noexcept {
    return (flags & bit) != 0u;
}

enum class MySimZodiac : Uint8 {
    Aries = 0,
    Taurus = 1,
    Gemini = 2,
    Cancer = 3,
    Leo = 4,
    Virgo = 5,
    Libra = 6,
    Scorpio = 7,
    Sagittarius = 8,
    Capricorn = 9,
    Aquarius = 10,
    Pisces = 11,
};

[[nodiscard]] inline constexpr bool MySimZodiacValid(Uint8 sign) noexcept {
    return sign < kMySimZodiacCount;
}

[[nodiscard]] inline constexpr const char* MySimZodiacName(Uint8 sign) noexcept {
    switch (sign) {
    case 0: return "Aries";
    case 1: return "Taurus";
    case 2: return "Gemini";
    case 3: return "Cancer";
    case 4: return "Leo";
    case 5: return "Virgo";
    case 6: return "Libra";
    case 7: return "Scorpio";
    case 8: return "Sagittarius";
    case 9: return "Capricorn";
    case 10: return "Aquarius";
    case 11: return "Pisces";
    default: return nullptr;
    }
}

[[nodiscard]] inline constexpr Sint32 MySimDisplayedEq(Float32 eq) noexcept {
    return static_cast<Sint32>(eq);
}

[[nodiscard]] inline constexpr bool MySimIsMale(Uint8 sexByte) noexcept {
    return sexByte != 0;
}

[[nodiscard]] inline constexpr bool MySimIsActive(Sint32 presence) noexcept {
    return presence != kMySimPresenceInactive;
}

[[nodiscard]] inline constexpr Uint32 MySimNightSoundId(Sint32 presence, Uint8 sexByte) noexcept {
    if (presence != kMySimPresenceNightSound) {
        return 0u;
    }
    return sexByte == 0 ? kMySimNightSoundIdWhenSexClear : kMySimNightSoundIdWhenSexSet;
}

[[nodiscard]] inline constexpr bool FirePhaseShouldAdvance(Sint32 phase, bool anyFires) noexcept {
    return anyFires && phase != kDisasterPhaseOngoing;
}

[[nodiscard]] inline constexpr bool FirePhaseShouldEnd(Sint32 phase, bool anyFires) noexcept {
    return !anyFires && phase == kDisasterPhaseOngoing;
}

[[nodiscard]] inline constexpr Uint32 FireCauseAfterSet(Uint32 current, Uint32 requested) noexcept {
    return current == 0u ? requested : current;
}

[[nodiscard]] inline constexpr bool VolcanoStateAccepted(Sint32 state) noexcept {
    return state >= 0 && state <= kVolcanoStateMax;
}

[[nodiscard]] inline constexpr Sint32 VolcanoPhaseFromState(
    Sint32 state, const Sint32 (&phaseByState)[kVolcanoPhaseTableCount]) noexcept {
    if (state < 0 || state >= kVolcanoPhaseTableCount) {
        return kVolcanoPhasePastTable;
    }
    return phaseByState[state];
}

[[nodiscard]] inline constexpr bool DisastersAreEnabled(Sint32 enabledField) noexcept {
    return enabledField == 0;
}

[[nodiscard]] inline constexpr bool DisasterIsOngoing(Sint32 ongoingField) noexcept {
    return ongoingField != 0;
}

// 806 entry points across the 19 classes.
[[maybe_unused]] inline constexpr std::array<Batch3EntryPoint, 806> kBatch3EntryPoints = {{
    // cSC4MySim (122)
    {0x1006bbbe2u, "cSC4MySim::GetModelButtonBitmapID(bool)"},
    {0x10079f23cu, "cSC4MySim::~cSC4MySim() [GZCOM boilerplate]"},
    {0x10079f7b0u, "cSC4MySim::~cSC4MySim()"},
    {0x10079f7bau, "cSC4MySim::~cSC4MySim() [GZCOM boilerplate]"},
    {0x10079f7e8u, "cSC4MySim::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x10079f85eu, "cSC4MySim::AddRef()"},
    {0x10079f8a4u, "cSC4MySim::Release()"},
    {0x1007a3260u, "cSC4MySim::GetNextBalloonAdvice()"},
    {0x1007a3286u, "cSC4MySim::GetNextBalloonEventAdvice()"},
    {0x1007a32acu, "cSC4MySim::GetCurrentBalloonAdvice() const"},
    {0x1007a3308u, "cSC4MySim::GetCurrentBalloonEventAdvice() const"},
    {0x1007a7e06u, "cSC4MySim::GetResidence() const"},
    {0x1007a7e1eu, "cSC4MySim::GetJob() const"},
    {0x1007a7ef2u, "cSC4MySim::GetVehicleKey() const"},
    {0x1007a7f30u, "cSC4MySim::GetSign() const"},
    {0x1007a7f64u, "cSC4MySim::GetSlot() const"},
    {0x1007a7f7cu, "cSC4MySim::GetWealth() const"},
    {0x1007a7f92u, "cSC4MySim::GetBitmapID() const"},
    {0x1007a7faau, "cSC4MySim::GetBitmapFile() const"},
    {0x1007a81f8u, "cSC4MySim::GetHomeName() const"},
    {0x1007a845cu, "cSC4MySim::IsMale() const"},
    {0x1007a8472u, "cSC4MySim::IsActive() const"},
    {0x1007a8490u, "cSC4MySim::GetBio() const"},
    {0x1007a912au, "cSC4MySim::PlayNightSound()"},
    {0x1007a9804u, "cSC4MySim::DoBuildingChanged(cISC4BuildingOccupant*)"},
    {0x1007a9e64u, "cSC4MySim::DoNeighborConnectionRemoved(int, int)"},
    {0x1007aa4aeu, "cSC4MySim::DispatchToolIsPylonLocked()"},
    {0x1007aa4bau, "cSC4MySim::DispatchToolGetPylon()"},
    {0x1007aa4c8u, "cSC4MySim::DispatchToolSetFollowCamera(bool)"},
    {0x1007aab7cu, "cSC4MySim::get_track_handle()"},
    {0x1007aac66u, "cSC4MySim::CanUDriveIt()"},
    {0x1007aad14u, "cSC4MySim::ExitUDriveIt()"},
    {0x1007aadf0u, "cSC4MySim::AsAdvisor()"},
    {0x10079e9c6u, "cSC4MySim::ActivateLuaContext()"},
    {0x10079ea2au, "cSC4MySim::CreateLuaTable()"},
    {0x10079ebccu, "cSC4MySim::cSC4MySim()"},
    {0x10079f8eau, "cSC4MySim::Init(int)"},
    {0x10079fcb2u, "cSC4MySim::UpdateVariables()"},
    {0x1007a10f0u, "cSC4MySim::init_balloons()"},
    {0x1007a191cu, "cSC4MySim::InitFromTuning()"},
    {0x1007a1fe8u, "cSC4MySim::Shutdown()"},
    {0x1007a2502u, "cSC4MySim::RemoveHomeSound()"},
    {0x1007a25aeu, "cSC4MySim::CancelViewInput()"},
    {0x1007a261au, "cSC4MySim::GetPersonalName()"},
    {0x1007a2fc2u, "cSC4MySim::empty_all_balloon_queues()"},
    {0x1007a3038u, "cSC4MySim::pop_balloon_queue(int, int)"},
    {0x1007a3118u, "cSC4MySim::push_balloon_queue(int, int)"},
    {0x1007a31b6u, "cSC4MySim::get_next_balloon(int)"},
    {0x1007a3364u, "cSC4MySim::DoMessage(cIGZMessage2*)"},
    {0x1007a3bd6u, "cSC4MySim::simulate_balloons(bool)"},
    {0x1007a3d3au, "cSC4MySim::PickIdleOffset()"},
    {0x1007a3e66u, "cSC4MySim::UpdateTripVariables(int, int, int, int, bool)"},
    {0x1007a5690u, "cSC4MySim::is_pylon_location(cSC4MySimDispatch::cLocation const&)"},
    {0x1007a56eeu, "cSC4MySim::FailCommute()"},
    {0x1007a5786u, "cSC4MySim::FindNewJob()"},
    {0x1007a5800u, "cSC4MySim::UpdateQuickVariables()"},
    {0x1007a5d5eu, "cSC4MySim::UpdateLocalBuildings()"},
    {0x1007a6160u, "cSC4MySim::UpdateJobTitle()"},
    {0x1007a6344u, "cSC4MySim::simulate_day()"},
    {0x1007a64f2u, "cSC4MySim::CheckCommute()"},
    {0x1007a6676u, "cSC4MySim::simulate_day_events()"},
    {0x1007a6830u, "cSC4MySim::DistanceToClosest(unsigned int)"},
    {0x1007a6896u, "cSC4MySim::DistanceToClosestSpecial(unsigned int)"},
    {0x1007a69eeu, "cSC4MySim::LocalSchoolGrade(unsigned int)"},
    {0x1007a6c06u, "cSC4MySim::LocalHospitalGrade(unsigned int)"},
    {0x1007a6e26u, "cSC4MySim::simulate_month()"},
    {0x1007a6fa6u, "cSC4MySim::MoveIn(unsigned char, cRZString&, bool, unsigned char, cRZString&, unsigned int, cRZString&)"},
    {0x1007a7112u, "cSC4MySim::MoveOut()"},
    {0x1007a718cu, "cSC4MySim::FindNewResidence()"},
    {0x1007a73f4u, "cSC4MySim::TestForResidence(cISC4BuildingOccupant*, bool, bool)"},
    {0x1007a7556u, "cSC4MySim::ChangeResidence(cISC4BuildingOccupant*)"},
    {0x1007a768au, "cSC4MySim::ChangeJob(cISC4BuildingOccupant*)"},
    {0x1007a7722u, "cSC4MySim::DeleteFromGame()"},
    {0x1007a77b8u, "cSC4MySim::ResetAdvisorState()"},
    {0x1007a7918u, "cSC4MySim::SetResidence(cISC4BuildingOccupant*)"},
    {0x1007a7becu, "cSC4MySim::DispatchCommute()"},
    {0x1007a7e36u, "cSC4MySim::GetJobTitle(cISC4BuildingOccupant*, cIGZString&)"},
    {0x1007a7f48u, "cSC4MySim::GetAge() const"},
    {0x1007a7fc6u, "cSC4MySim::GetBitmapBuffer(cIGZBuffer**) const"},
    {0x1007a8240u, "cSC4MySim::GetJobName() const"},
    {0x1007a82eau, "cSC4MySim::GetJobTitle() const"},
    {0x1007a8394u, "cSC4MySim::GetEQAsText(cRZString&)"},
    {0x1007a83e4u, "cSC4MySim::GetHQAsText(cRZString&)"},
    {0x1007a84a8u, "cSC4MySim::Find()"},
    {0x1007a8876u, "cSC4MySim::ShowResidence()"},
    {0x1007a8af8u, "cSC4MySim::HideResidence()"},
    {0x1007a8b64u, "cSC4MySim::UpdateResidence()"},
    {0x1007a8c6cu, "cSC4MySim::ShowLocation()"},
    {0x1007a8d5cu, "cSC4MySim::HideLocation()"},
    {0x1007a8dbau, "cSC4MySim::SetPylonVisibility(bool)"},
    {0x1007a8e36u, "cSC4MySim::GetCurrentScreenLocationXY(int*, int*)"},
    {0x1007a8ebeu, "cSC4MySim::QuitJob()"},
    {0x1007a8f4eu, "cSC4MySim::CommandWouldTrigger(unsigned int)"},
    {0x1007a8fc2u, "cSC4MySim::RequestToPlaySound()"},
    {0x1007a9042u, "cSC4MySim::PlayHomeSound(unsigned int)"},
    {0x1007a9182u, "cSC4MySim::LeaveForWork()"},
    {0x1007a9202u, "cSC4MySim::ReturnHome()"},
    {0x1007a928eu, "cSC4MySim::AnnounceMoveIn()"},
    {0x1007a93a0u, "cSC4MySim::DoScriptCommand(int)"},
    {0x1007a9480u, "cSC4MySim::Spawn()"},
    {0x1007a94feu, "cSC4MySim::IncreaseWealth()"},
    {0x1007a953eu, "cSC4MySim::Die()"},
    {0x1007a9600u, "cSC4MySim::LeaveCity()"},
    {0x1007a96b2u, "cSC4MySim::DoBuildingAdded(cISC4BuildingOccupant*)"},
    {0x1007a9858u, "cSC4MySim::DoBuildingRemoved(cISC4BuildingOccupant*, unsigned int)"},
    {0x1007a9934u, "cSC4MySim::StartGhostEffect()"},
    {0x1007a9a34u, "cSC4MySim::DoFireAdded(float, float)"},
    {0x1007a9b1au, "cSC4MySim::DoPipeBurst(float, float)"},
    {0x1007a9c00u, "cSC4MySim::DoTornado(float, float)"},
    {0x1007a9ce6u, "cSC4MySim::DoDisaster(unsigned int, int, int)"},
    {0x1007a9e9eu, "cSC4MySim::DispatchToolBegin()"},
    {0x1007aa0fau, "cSC4MySim::DispatchToolEnd()"},
    {0x1007aa262u, "cSC4MySim::DispatchToolLockPylon(bool)"},
    {0x1007aa4dau, "cSC4MySim::DispatchToolCanDispatch(float, float)"},
    {0x1007aa520u, "cSC4MySim::GetLocation(float*) const"},
    {0x1007aa5f0u, "cSC4MySim::SetCar(cISCResExemplar const*)"},
    {0x1007aa740u, "cSC4MySim::SetPedestrian(cISCResExemplar const*)"},
    {0x1007aa8d0u, "cSC4MySim::GetRandomWalkModelKey()"},
    {0x1007aa9dau, "cSC4MySim::CreateDispatchTool()"},
    {0x1007aaab4u, "cSC4MySim::CreateChangeHouseTool()"},
    {0x1007aab9eu, "cSC4MySim::CreateChangeJobTool()"},
    {0x1007aaca2u, "cSC4MySim::EnterUDriveIt()"},
    // cSC4AdvisorSystem (45)
    {0x10029525eu, "cSC4AdvisorSystem::~cSC4AdvisorSystem() [GZCOM boilerplate]"},
    {0x100295408u, "cSC4AdvisorSystem::~cSC4AdvisorSystem()"},
    {0x100295420u, "cSC4AdvisorSystem::~cSC4AdvisorSystem() [GZCOM boilerplate]"},
    {0x1002962e4u, "cSC4AdvisorSystem::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x100296374u, "cSC4AdvisorSystem::AddRef()"},
    {0x1002963b6u, "cSC4AdvisorSystem::Release()"},
    {0x10029717eu, "cSC4AdvisorSystem::GetAdvisors()"},
    {0x100297188u, "cSC4AdvisorSystem::GetScriptingContext()"},
    {0x100297512u, "cSC4AdvisorSystem::GetContextAdvice()"},
    {0x100297570u, "cSC4AdvisorSystem::DoMessage(cGZMessage&)"},
    {0x100297580u, "cSC4AdvisorSystem::GetRandom()"},
    {0x100297618u, "cSC4AdvisorSystem::IsAdvisorWindowVisible()"},
    {0x100297632u, "cSC4AdvisorSystem::ShowAdvisorWindow()"},
    {0x10029764au, "cSC4AdvisorSystem::HideAdvisorWindow()"},
    {0x100297f88u, "cSC4AdvisorSystem::GetDataTag(int)"},
    {0x100297fa2u, "cSC4AdvisorSystem::GetAdvisorWinProc()"},
    {0x100294eb0u, "cSC4AdvisorSystem::gc_tag_method(lua_State*)"},
    {0x100295048u, "cSC4AdvisorSystem::cSC4AdvisorSystem()"},
    {0x10029545cu, "cSC4AdvisorSystem::kill_advisors()"},
    {0x10029558eu, "cSC4AdvisorSystem::Init()"},
    {0x1002959d0u, "cSC4AdvisorSystem::register_api(bool)"},
    {0x100295ae8u, "cSC4AdvisorSystem::reload_data()"},
    {0x100295d1au, "cSC4AdvisorSystem::load_advisors()"},
    {0x100295dbeu, "cSC4AdvisorSystem::update_advisors()"},
    {0x100295eeau, "cSC4AdvisorSystem::init_advice_popup_priority_thresholds()"},
    {0x10029613cu, "cSC4AdvisorSystem::Shutdown()"},
    {0x1002963f8u, "cSC4AdvisorSystem::simulate_all_advisors(int)"},
    {0x100296468u, "cSC4AdvisorSystem::tick_self()"},
    {0x1002964ecu, "cSC4AdvisorSystem::DoMessage(cIGZMessage2*)"},
    {0x100296d2cu, "cSC4AdvisorSystem::AddAdvisor(cISC4Advisor*)"},
    {0x100296f68u, "cSC4AdvisorSystem::RemoveAdvisor(int)"},
    {0x10029713eu, "cSC4AdvisorSystem::GetAdvisor(int)"},
    {0x100297192u, "cSC4AdvisorSystem::DoLink(cIGZString const&)"},
    {0x100297420u, "cSC4AdvisorSystem::PushContextAdvice(cISC4Advice*)"},
    {0x1002974bcu, "cSC4AdvisorSystem::PopContextAdvice()"},
    {0x100297530u, "cSC4AdvisorSystem::GetAdvicePopupPriorityThreshold() const"},
    {0x10029758au, "cSC4AdvisorSystem::startAdvisorWindow()"},
    {0x100297662u, "cSC4AdvisorSystem::RemoveWinProcs()"},
    {0x1002978b0u, "cSC4AdvisorSystem::load_advisor(int)"},
    {0x100297a1cu, "cSC4AdvisorSystem::SetGlobalValue(char const*, cIGZString*)"},
    {0x100297ab8u, "cSC4AdvisorSystem::SetGlobalValue(char const*, double)"},
    {0x100297b30u, "cSC4AdvisorSystem::SetLocalValue(char const*, double, cISC4AdviceSubject*)"},
    {0x100297c98u, "cSC4AdvisorSystem::PushEvent(int, cISC4AdviceSubject*)"},
    {0x100297d4eu, "cSC4AdvisorSystem::SubscribeToEvent(int, cISC4Advisor*)"},
    {0x100297ed8u, "cSC4AdvisorSystem::UnsubscribeFromEvent(int, cISC4Advisor*)"},
    // cSC4Advisor (32)
    {0x100441ab2u, "cSC4Advisor::~cSC4Advisor() [GZCOM boilerplate]"},
    {0x100441beau, "cSC4Advisor::~cSC4Advisor()"},
    {0x100441bf4u, "cSC4Advisor::~cSC4Advisor() [GZCOM boilerplate]"},
    {0x100442cc0u, "cSC4Advisor::GetID() const"},
    {0x100442cd8u, "cSC4Advisor::GetName() const"},
    {0x100442cecu, "cSC4Advisor::SetName(cIGZString const&)"},
    {0x100442d08u, "cSC4Advisor::GetPersonalName()"},
    {0x100442d1cu, "cSC4Advisor::SetPersonalName(cIGZString const&)"},
    {0x100442d38u, "cSC4Advisor::GetCurrentAdvices() const"},
    {0x100442d54u, "cSC4Advisor::GetCurrentAdvices()"},
    {0x10044317eu, "cSC4Advisor::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x1004431e8u, "cSC4Advisor::AddRef()"},
    {0x10044321cu, "cSC4Advisor::Release()"},
    {0x1004449acu, "cSC4Advisor::simulate_month()"},
    {0x100441942u, "cSC4Advisor::cSC4Advisor()"},
    {0x100441c22u, "cSC4Advisor::Init(int)"},
    {0x10044291eu, "cSC4Advisor::load_heads()"},
    {0x100442b42u, "cSC4Advisor::Shutdown()"},
    {0x100442d70u, "cSC4Advisor::GetAdviceByGUID(int) const"},
    {0x100442e20u, "cSC4Advisor::GetAdviceBySessionID(int) const"},
    {0x100442ed0u, "cSC4Advisor::Simulate(int)"},
    {0x100442f40u, "cSC4Advisor::GetHeadAnimCount(bool)"},
    {0x100442f8cu, "cSC4Advisor::SetHeadAnim(bool, int)"},
    {0x10044303cu, "cSC4Advisor::GetHeadAnimID(int)"},
    {0x100443250u, "cSC4Advisor::DoMessage(cIGZMessage2*)"},
    {0x100443cf0u, "cSC4Advisor::on_advice_activation(cSC4Advisor::cAdviceInfo&)"},
    {0x1004444c0u, "cSC4Advisor::on_advice_deactivation(cSC4Advisor::cAdviceInfo&)"},
    {0x100444538u, "cSC4Advisor::simulate_day()"},
    {0x100444798u, "cSC4Advisor::simulate_day_events()"},
    {0x1004449b2u, "cSC4Advisor::PushEvent(int, cISC4AdviceSubject*)"},
    {0x100444a7au, "cSC4Advisor::CommandWouldTrigger(unsigned int)"},
    {0x100444b38u, "cSC4Advisor::RetireAdvice(cISC4Advice*)"},
    // cSC4Advice (36)
    {0x10043d04cu, "cSC4Advice::~cSC4Advice() [GZCOM boilerplate]"},
    {0x10043d188u, "cSC4Advice::~cSC4Advice()"},
    {0x10043d1a0u, "cSC4Advice::~cSC4Advice() [GZCOM boilerplate]"},
    {0x10043dd2eu, "cSC4Advice::GetMood() const"},
    {0x10043dd3au, "cSC4Advice::GetPriority() const"},
    {0x10043dd46u, "cSC4Advice::GetTriggerString()"},
    {0x10043defau, "cSC4Advice::GetGUID() const"},
    {0x10043df06u, "cSC4Advice::GetAdvisorID() const"},
    {0x10043df12u, "cSC4Advice::SetAdvisorID(int)"},
    {0x10043df1eu, "cSC4Advice::GetSessionID() const"},
    {0x10043dfe6u, "cSC4Advice::CheckFlags(int) const"},
    {0x10043e00eu, "cSC4Advice::GetFrequencyInDays() const"},
    {0x10043e01au, "cSC4Advice::GetTimeOutInDays() const"},
    {0x10043e026u, "cSC4Advice::GetTextTitle() const"},
    {0x10043e062u, "cSC4Advice::GetTextMessage() const"},
    {0x10043e140u, "cSC4Advice::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x10043e190u, "cSC4Advice::AddRef()"},
    {0x10043e1b6u, "cSC4Advice::Release()"},
    {0x10043e990u, "cSC4Advice::GetEventSubject()"},
    {0x10043e99eu, "cSC4Advice::GetEventID()"},
    {0x10043e9aau, "cSC4Advice::GetCommandID()"},
    {0x10043ce9eu, "cSC4Advice::cSC4Advice()"},
    {0x10043d1dcu, "cSC4Advice::Init(int)"},
    {0x10043db70u, "cSC4Advice::Clone(cISC4Advice**)"},
    {0x10043dcdcu, "cSC4Advice::Shutdown()"},
    {0x10043dd54u, "cSC4Advice::get_raw_trigger() const"},
    {0x10043deb6u, "cSC4Advice::GetTrigger() const"},
    {0x10043df2au, "cSC4Advice::SetFlags(int, bool)"},
    {0x10043dffau, "cSC4Advice::IsCurrent() const"},
    {0x10043e030u, "cSC4Advice::GetTextTitleGUID() const"},
    {0x10043e070u, "cSC4Advice::GetTextMessageGUID() const"},
    {0x10043e0a4u, "cSC4Advice::DoLink(cIGZString const&)"},
    {0x10043e48eu, "cSC4Advice::refresh_text()"},
    {0x10043e934u, "cSC4Advice::SetEventSubject(cISC4AdviceSubject*)"},
    {0x10043e9b6u, "cSC4Advice::NotifyGlobalEffectTriggerChanged()"},
    {0x10043ea2eu, "cSC4Advice::UpdateText()"},
    // cSC4CitySituationAdvice (67)
    {0x1006a835cu, "cSC4CitySituationAdvice::~cSC4CitySituationAdvice() [GZCOM boilerplate]"},
    {0x1006a8442u, "cSC4CitySituationAdvice::~cSC4CitySituationAdvice()"},
    {0x1006a8468u, "cSC4CitySituationAdvice::~cSC4CitySituationAdvice() [GZCOM boilerplate]"},
    {0x1006a900eu, "cSC4CitySituationAdvice::GetTrigger() const"},
    {0x1006a9734u, "cSC4CitySituationAdvice::GetSituationImageId() const"},
    {0x1006a9c56u, "cSC4CitySituationAdvice::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x1006a9ceau, "cSC4CitySituationAdvice::AddRef()"},
    {0x1006a9d1eu, "cSC4CitySituationAdvice::Release()"},
    {0x1006a9d52u, "cSC4CitySituationAdvice::AsAdvice()"},
    {0x1006a9d5cu, "cSC4CitySituationAdvice::GetSuccessCondition() const"},
    {0x1006a9d68u, "cSC4CitySituationAdvice::GetTimeLimit() const"},
    {0x1006a9d76u, "cSC4CitySituationAdvice::GetSuccessDistance() const"},
    {0x1006a9d84u, "cSC4CitySituationAdvice::GetMinTargetDistance() const"},
    {0x1006a9d92u, "cSC4CitySituationAdvice::GetSuccessTimeout() const"},
    {0x1006a9da0u, "cSC4CitySituationAdvice::GetDelayAfterDisabled() const"},
    {0x1006a9daeu, "cSC4CitySituationAdvice::GetAutomataGroups() const"},
    {0x1006a9dbcu, "cSC4CitySituationAdvice::GetTargetGroups() const"},
    {0x1006a9dcau, "cSC4CitySituationAdvice::GetSituationFlags() const"},
    {0x1006a9dd6u, "cSC4CitySituationAdvice::CheckSituationFlag(unsigned int) const"},
    {0x1006a9de6u, "cSC4CitySituationAdvice::GetTargetSequence() const"},
    {0x1006a9e06u, "cSC4CitySituationAdvice::GetEvadeList() const"},
    {0x1006a9e14u, "cSC4CitySituationAdvice::HasEvadeList() const"},
    {0x1006a9e2cu, "cSC4CitySituationAdvice::GetEvadeDistance() const"},
    {0x1006a9e3au, "cSC4CitySituationAdvice::GetEvadeTimeout() const"},
    {0x1006a9e48u, "cSC4CitySituationAdvice::GetScriptTableRef() const"},
    {0x1006a9e54u, "cSC4CitySituationAdvice::GetEvilTwinGuid() const"},
    {0x1006a9e60u, "cSC4CitySituationAdvice::GetRewardGuid() const"},
    {0x1006a9e6cu, "cSC4CitySituationAdvice::GetCoverageType() const"},
    {0x1006a9e78u, "cSC4CitySituationAdvice::GetCoverageZone() const"},
    {0x1006a9e90u, "cSC4CitySituationAdvice::GetCoverageNetworkType() const"},
    {0x1006a9e9cu, "cSC4CitySituationAdvice::GetCoverageRect() const"},
    {0x1006a9eaau, "cSC4CitySituationAdvice::GetCellCoverageList() const"},
    {0x1006a9eb8u, "cSC4CitySituationAdvice::GetCoverageCellsMin() const"},
    {0x1006a9ec4u, "cSC4CitySituationAdvice::GetCoverageCellsMax() const"},
    {0x1006a9ed0u, "cSC4CitySituationAdvice::GetCoverageSuccessCount() const"},
    {0x1006a9edcu, "cSC4CitySituationAdvice::GetCoverageSuccessPct() const"},
    {0x1006a9eeau, "cSC4CitySituationAdvice::GetSuccessAura(float&, float&) const"},
    {0x1006a9f48u, "cSC4CitySituationAdvice::GetSuccessMoney(long long&) const"},
    {0x1006a9f5eu, "cSC4CitySituationAdvice::GetFailureAura(float&, float&) const"},
    {0x1006a9fbcu, "cSC4CitySituationAdvice::GetFailureMoney(long long&) const"},
    {0x1006a9fd2u, "cSC4CitySituationAdvice::GetActiveRadius() const"},
    {0x1006a81f2u, "cSC4CitySituationAdvice::cSC4CitySituationAdvice()"},
    {0x1006a84b2u, "cSC4CitySituationAdvice::Init(int)"},
    {0x1006a88c6u, "cSC4CitySituationAdvice::UpdateSituationFields()"},
    {0x1006a8fb4u, "cSC4CitySituationAdvice::Shutdown()"},
    {0x1006a9026u, "cSC4CitySituationAdvice::GetTextTitle() const"},
    {0x1006a9040u, "cSC4CitySituationAdvice::ReadStringFromTable(char const*) const"},
    {0x1006a9254u, "cSC4CitySituationAdvice::GetSuccessConditionText(cIGZString&) const"},
    {0x1006a92e0u, "cSC4CitySituationAdvice::GetTextMessage() const"},
    {0x1006a9700u, "cSC4CitySituationAdvice::GetSuccessText() const"},
    {0x1006a971au, "cSC4CitySituationAdvice::GetFailureText() const"},
    {0x1006a9746u, "cSC4CitySituationAdvice::ReadUint32FromTable(char const*) const"},
    {0x1006a97dau, "cSC4CitySituationAdvice::GetSuccessImageId() const"},
    {0x1006a980cu, "cSC4CitySituationAdvice::GetRewardUnlockedImageId() const"},
    {0x1006a983eu, "cSC4CitySituationAdvice::GetSuccessMood() const"},
    {0x1006a9872u, "cSC4CitySituationAdvice::GetFailureImageId() const"},
    {0x1006a98a4u, "cSC4CitySituationAdvice::GetFailureMood() const"},
    {0x1006a98d8u, "cSC4CitySituationAdvice::GetRewardProgressText() const"},
    {0x1006a98f2u, "cSC4CitySituationAdvice::GetRewardUnlockedText() const"},
    {0x1006a990cu, "cSC4CitySituationAdvice::GetProgressText(unsigned short) const"},
    {0x1006a9b62u, "cSC4CitySituationAdvice::GetMaxTargetDistance() const"},
    {0x1006a9bb0u, "cSC4CitySituationAdvice::IsAutomatonRelevant(cISC4Automaton*) const"},
    {0x1006a9c08u, "cSC4CitySituationAdvice::IsGroupRelevant(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>> const&) const"},
    {0x1006a9df4u, "cSC4CitySituationAdvice::HasTargetSequence() const"},
    {0x1006a9e84u, "cSC4CitySituationAdvice::GetCoverageOccupantGroup() const"},
    {0x1006a9f0eu, "cSC4CitySituationAdvice::GetSuccessEffectName(cRZString&) const"},
    {0x1006a9f82u, "cSC4CitySituationAdvice::GetFailureEffectName(cRZString&) const"},
    // cSC4TutorialTask (41)
    {0x10011d622u, "cSC4TutorialTask::cSC4TutorialTask()"},
    {0x10011d62cu, "cSC4TutorialTask::~cSC4TutorialTask() [GZCOM boilerplate]"},
    {0x10011d762u, "cSC4TutorialTask::~cSC4TutorialTask()"},
    {0x10011d77au, "cSC4TutorialTask::~cSC4TutorialTask() [GZCOM boilerplate]"},
    {0x10011e526u, "cSC4TutorialTask::DoLink(cIGZString const&)"},
    {0x10011e52eu, "cSC4TutorialTask::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x10011e576u, "cSC4TutorialTask::AddRef()"},
    {0x10011e58eu, "cSC4TutorialTask::Release()"},
    {0x10011e702u, "cSC4TutorialTask::Shutdown()"},
    {0x10011e70au, "cSC4TutorialTask::GetTargetGUID() const"},
    {0x10011e716u, "cSC4TutorialTask::GetGUID() const"},
    {0x10011e722u, "cSC4TutorialTask::GetZoomLevel() const"},
    {0x10011e760u, "cSC4TutorialTask::GetExtraData() const"},
    {0x10011e76cu, "cSC4TutorialTask::GetWhichFunction() const"},
    {0x10011e778u, "cSC4TutorialTask::GetCatalogButtonItemID() const"},
    {0x10011e794u, "cSC4TutorialTask::GetCongratulationMsg() const"},
    {0x10011e79eu, "cSC4TutorialTask::GetInstructionMsg() const"},
    {0x10011e7a8u, "cSC4TutorialTask::GetTryAgainMsg() const"},
    {0x10011e7b2u, "cSC4TutorialTask::GetRegion() const"},
    {0x10011f472u, "cSC4TutorialTask::GetWindowOffsetX() const"},
    {0x10011f47eu, "cSC4TutorialTask::GetWindowOffsetY() const"},
    {0x10011f48au, "cSC4TutorialTask::IsEdgeScrollingAllowed() const"},
    {0x10011d466u, "cSC4TutorialTask::cSC4TutorialTask()"},
    {0x10011d7b6u, "cSC4TutorialTask::Init(cISCLua*, int)"},
    {0x10011df28u, "cSC4TutorialTask::GetString(cRZString const&, cRZString&)"},
    {0x10011e05au, "cSC4TutorialTask::GetNumber(cRZString const&)"},
    {0x10011e10eu, "cSC4TutorialTask::GetString(cRZString const&, cRZString&, cRZString const&)"},
    {0x10011e200u, "cSC4TutorialTask::GetTaskInformation()"},
    {0x10011e35cu, "cSC4TutorialTask::GetEnableButtonList(cRZString const&)"},
    {0x10011e5a6u, "cSC4TutorialTask::checkGridDataJobFinished(int, int)"},
    {0x10011e680u, "cSC4TutorialTask::GetTargetObjectGuid(cRZString const&)"},
    {0x10011e72eu, "cSC4TutorialTask::HasButtonGUID(unsigned int)"},
    {0x10011e7c0u, "cSC4TutorialTask::GetTargetWindow()"},
    {0x10011eb7eu, "cSC4TutorialTask::ExecuteTask()"},
    {0x10011ecf2u, "cSC4TutorialTask::JumpToLocation(int, int)"},
    {0x10011edfeu, "cSC4TutorialTask::EndTask()"},
    {0x10011eebau, "cSC4TutorialTask::handleButtons(unsigned int)"},
    {0x10011eedcu, "cSC4TutorialTask::FindNearestBuilding(SC4Rect<int> const&)"},
    {0x10011f04cu, "cSC4TutorialTask::TriggerFire(SC4Rect<int> const&)"},
    {0x10011f1a4u, "cSC4TutorialTask::ShowHintsForUsers()"},
    {0x10011f1e2u, "cSC4TutorialTask::drawDecals(int, int, int, int)"},
    // cSC4TutorialSystem (27)
    {0x10005f1bcu, "cSC4TutorialSystem::~cSC4TutorialSystem() [GZCOM boilerplate]"},
    {0x10005f2a8u, "cSC4TutorialSystem::~cSC4TutorialSystem()"},
    {0x10005f2c0u, "cSC4TutorialSystem::~cSC4TutorialSystem() [GZCOM boilerplate]"},
    {0x10005fa92u, "cSC4TutorialSystem::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x10005fb22u, "cSC4TutorialSystem::AddRef()"},
    {0x10005fb64u, "cSC4TutorialSystem::Release()"},
    {0x10005ff88u, "cSC4TutorialSystem::ShowUIHintsForUsers()"},
    {0x10005ff9eu, "cSC4TutorialSystem::FilterLeftMouseClick(int&, int&, int)"},
    {0x1000600e0u, "cSC4TutorialSystem::IsTutorial() const"},
    {0x100060106u, "cSC4TutorialSystem::GetTutorialTaskNumber() const"},
    {0x100060110u, "cSC4TutorialSystem::GetTutorialGUID() const"},
    {0x100060126u, "cSC4TutorialSystem::ResetTutorialIndex(int)"},
    {0x100060130u, "cSC4TutorialSystem::IncreaseTutorialNumber()"},
    {0x10005f0d4u, "cSC4TutorialSystem::cSC4TutorialSystem()"},
    {0x10005f2fcu, "cSC4TutorialSystem::startTutorial()"},
    {0x10005f3aeu, "cSC4TutorialSystem::endTutorial(bool)"},
    {0x10005f452u, "cSC4TutorialSystem::DoMessage(cIGZMessage2*)"},
    {0x10005f528u, "cSC4TutorialSystem::DoMessage(cGZMessage&)"},
    {0x10005fcc8u, "cSC4TutorialSystem::Init()"},
    {0x10005fe5au, "cSC4TutorialSystem::Shutdown()"},
    {0x10005ff0cu, "cSC4TutorialSystem::FilterMessage(cGZMessage&)"},
    {0x10005ff56u, "cSC4TutorialSystem::ResetTutorialNumAndIndex(int, int)"},
    {0x10005ffbau, "cSC4TutorialSystem::HandleCatalogMenu(cISC4WinCatalogView*)"},
    {0x10005ffd2u, "cSC4TutorialSystem::GetScriptingContext()"},
    {0x100060042u, "cSC4TutorialSystem::OnTutorialEvent(cISC4TutorialSystem::eTutorialEvent)"},
    {0x100060058u, "cSC4TutorialSystem::IsEdgeScrollingAllowed() const"},
    {0x1000600eeu, "cSC4TutorialSystem::GetCurrentTask() const"},
    // cSC4DisasterInstanceFire (43)
    {0x10044f946u, "cSC4DisasterInstanceFire::~cSC4DisasterInstanceFire() [GZCOM boilerplate]"},
    {0x10044faa4u, "cSC4DisasterInstanceFire::~cSC4DisasterInstanceFire()"},
    {0x10044faaeu, "cSC4DisasterInstanceFire::~cSC4DisasterInstanceFire() [GZCOM boilerplate]"},
    {0x10044fadcu, "cSC4DisasterInstanceFire::AddRef()"},
    {0x10044fb10u, "cSC4DisasterInstanceFire::Release() [GZCOM boilerplate]"},
    {0x10044fbaeu, "cSC4DisasterInstanceFire::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x10044fc18u, "cSC4DisasterInstanceFire::SetCause(unsigned int)"},
    {0x10044fc48u, "cSC4DisasterInstanceFire::GetCause()"},
    {0x10045289eu, "cSC4DisasterInstanceFire::GetFireCount()"},
    {0x10045295cu, "cSC4DisasterInstanceFire::GetLocation(int&, int&) const"},
    {0x1004529c0u, "cSC4DisasterInstanceFire::GetPhase() const"},
    {0x1004529d8u, "cSC4DisasterInstanceFire::GetDamagedOccupantsCount() const"},
    {0x1004529e4u, "cSC4DisasterInstanceFire::GetCost() const"},
    {0x1004529f8u, "cSC4DisasterInstanceFire::GetID() const"},
    {0x10044f6e8u, "cSC4DisasterInstanceFire::cSC4DisasterInstanceFire()"},
    {0x10044fc60u, "cSC4DisasterInstanceFire::Init()"},
    {0x10044fe9eu, "cSC4DisasterInstanceFire::ReadTunableValues()"},
    {0x100450402u, "cSC4DisasterInstanceFire::Shutdown()"},
    {0x1004505a8u, "cSC4DisasterInstanceFire::KillAllFires()"},
    {0x1004506c8u, "cSC4DisasterInstanceFire::DoMessage(cIGZMessage2*)"},
    {0x1004507b6u, "cSC4DisasterInstanceFire::Update()"},
    {0x1004508ceu, "cSC4DisasterInstanceFire::ProcessToastList()"},
    {0x100450942u, "cSC4DisasterInstanceFire::UpdatePhase()"},
    {0x100450aa6u, "cSC4DisasterInstanceFire::UpdateAllFires()"},
    {0x100450b38u, "cSC4DisasterInstanceFire::ToastOccupant(cISC4Occupant*)"},
    {0x1004510aeu, "cSC4DisasterInstanceFire::SetSeverity(unsigned int)"},
    {0x100451156u, "cSC4DisasterInstanceFire::SetPhase(cISC4DisasterInstance::ePhase)"},
    {0x10045121cu, "cSC4DisasterInstanceFire::UpdateFire(cSC4FireOccupant*)"},
    {0x100451600u, "cSC4DisasterInstanceFire::KillFire(cSC4FireOccupant*)"},
    {0x1004517a0u, "cSC4DisasterInstanceFire::ChooseSpreadCell(cSC4FireOccupant*, int*, int*)"},
    {0x100451926u, "cSC4DisasterInstanceFire::Ignite(cISC4Occupant*, bool, unsigned int, bool)"},
    {0x100451e82u, "cSC4DisasterInstanceFire::SendIgnitionMessage(cISC4Occupant*, bool)"},
    {0x100451f44u, "cSC4DisasterInstanceFire::IgniteCellRadius(unsigned int, unsigned int, unsigned int, unsigned int)"},
    {0x10045216eu, "cSC4DisasterInstanceFire::AddToToastList(cISC4Occupant*)"},
    {0x10045220au, "cSC4DisasterInstanceFire::IgniteCell(unsigned int, unsigned int, bool, bool, unsigned int)"},
    {0x1004523c8u, "cSC4DisasterInstanceFire::ExtinguishCell(unsigned int, unsigned int, int)"},
    {0x100452594u, "cSC4DisasterInstanceFire::QuickBurnCell(unsigned int, unsigned int)"},
    {0x1004526c2u, "cSC4DisasterInstanceFire::Extinguish(cISC4Occupant*)"},
    {0x10045273eu, "cSC4DisasterInstanceFire::BurnDown(cISC4Occupant*)"},
    {0x1004527bau, "cSC4DisasterInstanceFire::ExtinguishFire(cSC4FireOccupant*)"},
    {0x100452816u, "cSC4DisasterInstanceFire::BurnDownFire(cSC4FireOccupant*)"},
    {0x1004528b6u, "cSC4DisasterInstanceFire::GetFireCountInCellRect(SC4Rect<int>&)"},
    {0x10045296cu, "cSC4DisasterInstanceFire::GetManager() const"},
    // cSC4DisasterLayer (35)
    {0x100461a62u, "cSC4DisasterLayer::AddRef()"},
    {0x100461bacu, "cSC4DisasterLayer::~cSC4DisasterLayer() [GZCOM boilerplate]"},
    {0x100461cd4u, "cSC4DisasterLayer::~cSC4DisasterLayer()"},
    {0x100461cdeu, "cSC4DisasterLayer::~cSC4DisasterLayer() [GZCOM boilerplate]"},
    {0x1004620e6u, "cSC4DisasterLayer::DisastersEnabled() const"},
    {0x100462c60u, "cSC4DisasterLayer::IsADisasterOngoing() const"},
    {0x100462c7cu, "cSC4DisasterLayer::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x100462cd4u, "cSC4DisasterLayer::Release()"},
    {0x1004639f2u, "cSC4DisasterLayer::GetDisasterCost()"},
    {0x100461a88u, "cSC4DisasterLayer::cSC4DisasterLayer()"},
    {0x100461d0cu, "cSC4DisasterLayer::DisasterHasBegun(cISC4DisasterInstance*)"},
    {0x100461e68u, "cSC4DisasterLayer::StartDamageAssessment()"},
    {0x100461f06u, "cSC4DisasterLayer::DisasterHasEnded()"},
    {0x100461f82u, "cSC4DisasterLayer::EndDamageAssessment()"},
    {0x10046200au, "cSC4DisasterLayer::PostMessageA(unsigned int, unsigned long, unsigned long, unsigned long)"},
    {0x100462102u, "cSC4DisasterLayer::DoMessage(cIGZMessage2*)"},
    {0x10046215au, "cSC4DisasterLayer::DoOnOccupantRemoval(cIGZMessage2Standard*)"},
    {0x100462412u, "cSC4DisasterLayer::DoDispatchMessage(cIGZMessage2Standard*)"},
    {0x10046256au, "cSC4DisasterLayer::ClearDispatchTimers()"},
    {0x10046269au, "cSC4DisasterLayer::StartDispatchTimers()"},
    {0x1004626d0u, "cSC4DisasterLayer::EndDispatchTimers()"},
    {0x10046295au, "cSC4DisasterLayer::EnableDisasters(bool)"},
    {0x1004629acu, "cSC4DisasterLayer::GetDisasterManager(unsigned int) const"},
    {0x100462a38u, "cSC4DisasterLayer::Init()"},
    {0x100462bbcu, "cSC4DisasterLayer::ResetDisasterCost()"},
    {0x100462cfau, "cSC4DisasterLayer::Shutdown()"},
    {0x100462ddau, "cSC4DisasterLayer::GetNextDisasterLocation(int*, int*, int*)"},
    {0x100462e90u, "cSC4DisasterLayer::ResolveDisasterLocation(cSC4DisasterLayer::tDisasterLocation*)"},
    {0x100462f66u, "cSC4DisasterLayer::GetMostRecentDisasterLocation(cISC4DisasterInstance*, int*, int*)"},
    {0x100463006u, "cSC4DisasterLayer::GoToNextDisaster()"},
    {0x10046313cu, "cSC4DisasterLayer::RegisterDisasterLocation(cISC4DisasterInstance*, int, int, cISC4Occupant*, int, bool)"},
    {0x100463228u, "cSC4DisasterLayer::UnregisterDisasterLocations(cISC4DisasterInstance*)"},
    {0x100463308u, "cSC4DisasterLayer::GetDisasterLocationCount() const"},
    {0x100463338u, "cSC4DisasterLayer::GetDisasterLocationByIndex(int, cISC4DisasterInstance**, int*, int*) const"},
    {0x1004633b8u, "cSC4DisasterLayer::UnregisterDisasterLocationByOccupant(cISC4DisasterInstance*, cISC4Occupant*)"},
    // cSC4DisasterInstanceEarthquake (33)
    {0x100452fe8u, "cSC4DisasterInstanceEarthquake::AddRef()"},
    {0x10045300eu, "cSC4DisasterInstanceEarthquake::AsISC4DisasterInstance()"},
    {0x10045379cu, "cSC4DisasterInstanceEarthquake::~cSC4DisasterInstanceEarthquake() [GZCOM boilerplate]"},
    {0x10045388eu, "cSC4DisasterInstanceEarthquake::~cSC4DisasterInstanceEarthquake()"},
    {0x100453898u, "cSC4DisasterInstanceEarthquake::~cSC4DisasterInstanceEarthquake() [GZCOM boilerplate]"},
    {0x1004540bau, "cSC4DisasterInstanceEarthquake::GetDamagedOccupantsCount() const"},
    {0x1004540c2u, "cSC4DisasterInstanceEarthquake::GetLocation(int&, int&) const"},
    {0x1004540d2u, "cSC4DisasterInstanceEarthquake::GetManager() const"},
    {0x1004540feu, "cSC4DisasterInstanceEarthquake::GetPhase() const"},
    {0x100454350u, "cSC4DisasterInstanceEarthquake::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x10045455au, "cSC4DisasterInstanceEarthquake::Release()"},
    {0x1004549a8u, "cSC4DisasterInstanceEarthquake::StaticShutdown(void*)"},
    {0x1004549b8u, "cSC4DisasterInstanceEarthquake::GetID() const"},
    {0x100452ca4u, "cSC4DisasterInstanceEarthquake::Init(int, int)"},
    {0x100453018u, "cSC4DisasterInstanceEarthquake::ComputeFaultLine()"},
    {0x10045351cu, "cSC4DisasterInstanceEarthquake::GetTypicalFaultLineSlope()"},
    {0x10045355eu, "cSC4DisasterInstanceEarthquake::CreateParticleEffect(char const*, cS3DVector2 const&, bool, cISC4VisualEffect*&)"},
    {0x10045365cu, "cSC4DisasterInstanceEarthquake::cSC4DisasterInstanceEarthquake()"},
    {0x1004538c6u, "cSC4DisasterInstanceEarthquake::DoAnimationTick(unsigned int)"},
    {0x100453948u, "cSC4DisasterInstanceEarthquake::DoMouseDownUpdate(unsigned int)"},
    {0x1004539b8u, "cSC4DisasterInstanceEarthquake::DoDeformation(unsigned int)"},
    {0x100453d8au, "cSC4DisasterInstanceEarthquake::GetNextPointsOnFaultLine(cSC4DisasterInstanceEarthquake::tPtOnFaultLine&, cSC4DisasterInstanceEarthquake::tPtOnFaultLine&)"},
    {0x100453ea2u, "cSC4DisasterInstanceEarthquake::PlaceParticleEffect(cISC4VisualEffect*, cS3DVector2&)"},
    {0x100453f48u, "cSC4DisasterInstanceEarthquake::DoMessage(cIGZMessage2*)"},
    {0x100453f92u, "cSC4DisasterInstanceEarthquake::EndEarthquakeAction()"},
    {0x100454018u, "cSC4DisasterInstanceEarthquake::EvalBezier(std::__1::vector<cS3DVector2, std::__1::allocator<cS3DVector2>> const&, float, cS3DVector2&)"},
    {0x100454116u, "cSC4DisasterInstanceEarthquake::StaticInit()"},
    {0x100454150u, "cSC4DisasterInstanceEarthquake::PlaceAutomataRepulsor(cS3DVector2 const&)"},
    {0x100454274u, "cSC4DisasterInstanceEarthquake::PlaceAutomataRepulsors()"},
    {0x1004543d4u, "cSC4DisasterInstanceEarthquake::ReadTuningParameters()"},
    {0x100454580u, "cSC4DisasterInstanceEarthquake::ReleaseAutomataRepulsors()"},
    {0x1004545f2u, "cSC4DisasterInstanceEarthquake::StartDeformation()"},
    {0x100454852u, "cSC4DisasterInstanceEarthquake::Shutdown()"},
    // cSC4DisasterInstancePipeBurst (32)
    {0x1004558d2u, "cSC4DisasterInstancePipeBurst::AddRef()"},
    {0x1004558f8u, "cSC4DisasterInstancePipeBurst::AsISC4DisasterInstance()"},
    {0x100455e80u, "cSC4DisasterInstancePipeBurst::~cSC4DisasterInstancePipeBurst() [GZCOM boilerplate]"},
    {0x100455f0eu, "cSC4DisasterInstancePipeBurst::~cSC4DisasterInstancePipeBurst()"},
    {0x100455f18u, "cSC4DisasterInstancePipeBurst::~cSC4DisasterInstancePipeBurst() [GZCOM boilerplate]"},
    {0x1004562f4u, "cSC4DisasterInstancePipeBurst::GetDamagedOccupantsCount() const"},
    {0x1004562fcu, "cSC4DisasterInstancePipeBurst::GetLocation(int&, int&) const"},
    {0x10045630cu, "cSC4DisasterInstancePipeBurst::GetManager() const"},
    {0x100456338u, "cSC4DisasterInstancePipeBurst::GetPhase() const"},
    {0x10045647au, "cSC4DisasterInstancePipeBurst::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x1004565ccu, "cSC4DisasterInstancePipeBurst::Release()"},
    {0x1004567acu, "cSC4DisasterInstancePipeBurst::StaticShutdown(void*)"},
    {0x1004553f4u, "cSC4DisasterInstancePipeBurst::Init(cSC4PipeOccupant*)"},
    {0x100455902u, "cSC4DisasterInstancePipeBurst::BurstPipe(int, int)"},
    {0x100455ad2u, "cSC4DisasterInstancePipeBurst::GetPipeAtLocation(int, int)"},
    {0x100455b32u, "cSC4DisasterInstancePipeBurst::CreateParticleEffect(char const*, cS3DVector3 const&, bool, cISC4VisualEffect*&)"},
    {0x100455c5cu, "cSC4DisasterInstancePipeBurst::PostMessageA(unsigned int, unsigned long, unsigned long, unsigned long)"},
    {0x100455d2au, "cSC4DisasterInstancePipeBurst::CreateSinkHole(int, int)"},
    {0x100455dccu, "cSC4DisasterInstancePipeBurst::CreateLeakEffectForPipe(cSC4DisasterInstancePipeBurst::tLeakyPipe&)"},
    {0x100455f46u, "cSC4DisasterInstancePipeBurst::DoAnimationTick(unsigned int)"},
    {0x100456022u, "cSC4DisasterInstancePipeBurst::TickLeaksInWait()"},
    {0x10045607au, "cSC4DisasterInstancePipeBurst::DoMessage(cIGZMessage2*)"},
    {0x100456132u, "cSC4DisasterInstancePipeBurst::DoViewLevelChange(cSC4ViewLevelInformation*)"},
    {0x10045617eu, "cSC4DisasterInstancePipeBurst::DoOccupantInserted(cISC4Occupant*)"},
    {0x1004561f4u, "cSC4DisasterInstancePipeBurst::DoOccupantRemoved(cISC4Occupant*)"},
    {0x10045627cu, "cSC4DisasterInstancePipeBurst::StopLeakAtCell(int, int)"},
    {0x100456350u, "cSC4DisasterInstancePipeBurst::StaticInit()"},
    {0x10045638au, "cSC4DisasterInstancePipeBurst::StartLeakAtPipe(cSC4PipeOccupant*)"},
    {0x1004564feu, "cSC4DisasterInstancePipeBurst::ReadTuningParameters()"},
    {0x1004565f2u, "cSC4DisasterInstancePipeBurst::Shutdown()"},
    {0x10045675au, "cSC4DisasterInstancePipeBurst::StartLeakAtCells(std::__1::vector<SC4Point<int>, std::__1::allocator<SC4Point<int>>> const&)"},
    {0x100456802u, "cSC4DisasterInstancePipeBurst::GetID() const"},
    // cSC4DisasterInstanceVolcano (29)
    {0x100457374u, "cSC4DisasterInstanceVolcano::AddRef()"},
    {0x10045739eu, "cSC4DisasterInstanceVolcano::AsISC4DisasterInstance() const"},
    {0x1004573b2u, "cSC4DisasterInstanceVolcano::AsISC4DisasterInstance()"},
    {0x1004573c8u, "cSC4DisasterInstanceVolcano::~cSC4DisasterInstanceVolcano() [GZCOM boilerplate]"},
    {0x100457404u, "cSC4DisasterInstanceVolcano::~cSC4DisasterInstanceVolcano() [GZCOM boilerplate]"},
    {0x1004577ccu, "cSC4DisasterInstanceVolcano::GetDamagedOccupantsCount() const"},
    {0x1004577d4u, "cSC4DisasterInstanceVolcano::GetLocation(int&, int&) const"},
    {0x1004577dcu, "cSC4DisasterInstanceVolcano::GetManager() const"},
    {0x1004577f2u, "cSC4DisasterInstanceVolcano::GetPhase() const"},
    {0x100457cd6u, "cSC4DisasterInstanceVolcano::GetState() const"},
    {0x100457ceau, "cSC4DisasterInstanceVolcano::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x100457d98u, "cSC4DisasterInstanceVolcano::Release() [GZCOM boilerplate]"},
    {0x1004581a2u, "cSC4DisasterInstanceVolcano::GetID() const"},
    {0x1004581aeu, "cSC4DisasterInstanceVolcano::GetLavaHeightByCityCell(int, int)"},
    {0x100457466u, "cSC4DisasterInstanceVolcano::DoMessage(cIGZMessage2*)"},
    {0x100457538u, "cSC4DisasterInstanceVolcano::DoAnimationTick(unsigned int)"},
    {0x10045767cu, "cSC4DisasterInstanceVolcano::EndVolcanoLava()"},
    {0x1004576f0u, "cSC4DisasterInstanceVolcano::InitializeVolcanoLava()"},
    {0x100457772u, "cSC4DisasterInstanceVolcano::StartLavaProduction()"},
    {0x100457814u, "cSC4DisasterInstanceVolcano::Init()"},
    {0x100457a20u, "cSC4DisasterInstanceVolcano::StaticInit(void*)"},
    {0x100457abcu, "cSC4DisasterInstanceVolcano::ReadTuningParameters()"},
    {0x100457be4u, "cSC4DisasterInstanceVolcano::CreateParticleEffect(char const*, cS3DVector2 const&, bool, cISC4VisualEffect*&)"},
    {0x100457df4u, "cSC4DisasterInstanceVolcano::SetLocation(cRZPoint const&)"},
    {0x100457e92u, "cSC4DisasterInstanceVolcano::SetState(cISC4DisasterInstanceVolcano::eState)"},
    {0x100457fa2u, "cSC4DisasterInstanceVolcano::StartDeformation()"},
    {0x100457fdcu, "cSC4DisasterInstanceVolcano::EndDeformation()"},
    {0x100458020u, "cSC4DisasterInstanceVolcano::StaticShutdown(void*)"},
    {0x1004580a0u, "cSC4DisasterInstanceVolcano::Shutdown()"},
    // cSC4AudioScapeManager (50)
    {0x1003f20c8u, "cSC4AudioScapeManager::DoMessage(cIGZMessage2*)"},
    {0x1003f20e0u, "cSC4AudioScapeManager::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x1003f2130u, "cSC4AudioScapeManager::AddRef()"},
    {0x1003f215au, "cSC4AudioScapeManager::Release() [GZCOM boilerplate]"},
    {0x1003f25c0u, "cSC4AudioScapeManager::cSC4AudioScapeManager()"},
    {0x1003f25cau, "cSC4AudioScapeManager::~cSC4AudioScapeManager() [GZCOM boilerplate]"},
    {0x1003f26e6u, "cSC4AudioScapeManager::~cSC4AudioScapeManager()"},
    {0x1003f26f0u, "cSC4AudioScapeManager::~cSC4AudioScapeManager() [GZCOM boilerplate]"},
    {0x1003f2794u, "cSC4AudioScapeManager::tLayer::~tLayer() [GZCOM boilerplate]"},
    {0x1003f27e4u, "cSC4AudioScapeManager::GetServicePriority() const"},
    {0x1003f27f0u, "cSC4AudioScapeManager::IsServiceRunning() const"},
    {0x1003f27fcu, "cSC4AudioScapeManager::SetServiceRunning(bool)"},
    {0x1003f280au, "cSC4AudioScapeManager::OnIdle(unsigned int)"},
    {0x1003ded76u, "cSC4AudioScapeManager::Create()"},
    {0x1003f0196u, "cSC4AudioScapeManager::Init()"},
    {0x1003f034au, "cSC4AudioScapeManager::Shutdown()"},
    {0x1003f0450u, "cSC4AudioScapeManager::CreateLayer(cIGZString const&, unsigned int, unsigned int, float)"},
    {0x1003f05d8u, "cSC4AudioScapeManager::DestroyLayer(unsigned int)"},
    {0x1003f06c2u, "cSC4AudioScapeManager::tLayer::UnregisterAllSounds()"},
    {0x1003f072au, "cSC4AudioScapeManager::ClearLayer(unsigned int)"},
    {0x1003f07a6u, "cSC4AudioScapeManager::tLayer::Clear()"},
    {0x1003f085eu, "cSC4AudioScapeManager::ClearAllLayers(unsigned int, bool)"},
    {0x1003f0942u, "cSC4AudioScapeManager::tGroup::RemoveAllInstances()"},
    {0x1003f09acu, "cSC4AudioScapeManager::_DeleteGroup(cSC4AudioScapeManager::tLayer*, std::__1::__list_iterator<cSC4AudioScapeManager::tGroup*, void*>&)"},
    {0x1003f0a2au, "cSC4AudioScapeManager::GetDistributedGroupPositionsAvgIntersected(unsigned int, unsigned int, SC4Rect<float>&, cS3DVector3*, int&, int&, bool)"},
    {0x1003f0da6u, "cSC4AudioScapeManager::_GetDistributedGroupPositionsAvg(cSC4AudioScapeManager::tLayer*, SC4Rect<float>&, cS3DVector3*, int&, int&, bool)"},
    {0x1003f10fau, "cSC4AudioScapeManager::GetDistributedGroupPositionsAvg(unsigned int, SC4Rect<float>&, cS3DVector3*, int&, int&, bool)"},
    {0x1003f1212u, "cSC4AudioScapeManager::GetGroupPositions(unsigned int, cS3DVector3*, int&, int&)"},
    {0x1003f12d6u, "cSC4AudioScapeManager::InsertInstance(unsigned int, unsigned int, cS3DVector3 const&, float)"},
    {0x1003f12eeu, "cSC4AudioScapeManager::InsertInstance(unsigned int, unsigned int, bool, unsigned int, cS3DVector3 const&, float)"},
    {0x1003f1522u, "cSC4AudioScapeManager::_AddInstanceToCorrectGroup(cSC4AudioScapeManager::tLayer*, cSC4AudioScapeManager::tSoundDef*, cSC4AudioScapeManager::tInstance*, cS3DVector3 const&, float, bool)"},
    {0x1003f16ccu, "cSC4AudioScapeManager::RemoveInstance(unsigned int, unsigned int)"},
    {0x1003f17eeu, "cSC4AudioScapeManager::tGroup::RemoveInstance(unsigned int)"},
    {0x1003f1866u, "cSC4AudioScapeManager::_UpdateLayerGroup(cSC4AudioScapeManager::tLayer*, cSC4AudioScapeManager::tSoundDef*, cSC4AudioScapeManager::tGroup*)"},
    {0x1003f18c8u, "cSC4AudioScapeManager::UpdateInstance(unsigned int, unsigned int, cS3DVector3 const&, float)"},
    {0x1003f1ac4u, "cSC4AudioScapeManager::tGroup::CenterRemovePosition(cS3DVector3 const&)"},
    {0x1003f1b30u, "cSC4AudioScapeManager::tGroup::CenterAddPosition(cS3DVector3 const&)"},
    {0x1003f1bc8u, "cSC4AudioScapeManager::RegisterLayerSound(unsigned int, unsigned int, bool, int, int, int)"},
    {0x1003f1cfau, "cSC4AudioScapeManager::UnregisterLayerSound(unsigned int, unsigned int)"},
    {0x1003f1dd4u, "cSC4AudioScapeManager::OnTick(unsigned int)"},
    {0x1003f1fcau, "cSC4AudioScapeManager::tGroup::StopAllSounds()"},
    {0x1003f205eu, "cSC4AudioScapeManager::tGroup::SetVolume(unsigned int, unsigned int)"},
    {0x1003f21c8u, "cSC4AudioScapeManager::tGroup::HandleSound(cSC4AudioScapeManager::tSoundDef*)"},
    {0x1003f21ecu, "cSC4AudioScapeManager::tGroup::UpdatePositions()"},
    {0x1003f2248u, "cSC4AudioScapeManager::tGroup::_Start(cSC4AudioScapeManager::tSoundDef*)"},
    {0x1003f2316u, "cSC4AudioScapeManager::tGroup::_Stop(cSC4AudioScapeManager::tSoundDef*)"},
    {0x1003f242cu, "cSC4AudioScapeManager::cSC4AudioScapeManager()"},
    {0x1003f2720u, "cSC4AudioScapeManager::tGroup::tGroup()"},
    {0x1003f27ccu, "cSC4AudioScapeManager::GetServiceID() const"},
    {0x1003f27d8u, "cSC4AudioScapeManager::SetServiceID(unsigned int)"},
    // cSC4AudioAmbienceLayer (48)
    {0x10040c272u, "cSC4AudioAmbienceLayer::Enable(bool)"},
    {0x10040e288u, "cSC4AudioAmbienceLayer::SetScoreMasterVolume(unsigned int)"},
    {0x10040e41cu, "cSC4AudioAmbienceLayer::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x10040e43eu, "cSC4AudioAmbienceLayer::AddRef()"},
    {0x10040e44cu, "cSC4AudioAmbienceLayer::Release() [GZCOM boilerplate]"},
    {0x10040edfcu, "cSC4AudioAmbienceLayer::cSC4AudioAmbienceLayer(unsigned int)"},
    {0x10040ee06u, "cSC4AudioAmbienceLayer::~cSC4AudioAmbienceLayer() [GZCOM boilerplate]"},
    {0x10040eff6u, "cSC4AudioAmbienceLayer::GetName()"},
    {0x10040f02au, "cSC4AudioAmbienceLayer::SetLayerId(unsigned int)"},
    {0x10040f036u, "cSC4AudioAmbienceLayer::GetLayerId()"},
    {0x10040f042u, "cSC4AudioAmbienceLayer::GetAveragePosition(cS3DVector3&)"},
    {0x10040f062u, "cSC4AudioAmbienceLayer::CanHandleOccupant(cISC4Occupant*)"},
    {0x10040b266u, "cSC4AudioAmbienceLayer::Init()"},
    {0x10040b34au, "cSC4AudioAmbienceLayer::_StaticInit()"},
    {0x10040b438u, "cSC4AudioAmbienceLayer::_ReadTuningValues(unsigned int)"},
    {0x10040ba4cu, "cSC4AudioAmbienceLayer::_CreateFilters()"},
    {0x10040bb98u, "cSC4AudioAmbienceLayer::_CreateLayers()"},
    {0x10040bd2cu, "cSC4AudioAmbienceLayer::Shutdown()"},
    {0x10040bddcu, "cSC4AudioAmbienceLayer::_RemoveLayers()"},
    {0x10040be6eu, "cSC4AudioAmbienceLayer::_RemoveAllFilters()"},
    {0x10040c036u, "cSC4AudioAmbienceLayer::_RemoveAllFreshnessScores()"},
    {0x10040c13eu, "cSC4AudioAmbienceLayer::_RemoveAllAmbienceLoops()"},
    {0x10040c22au, "cSC4AudioAmbienceLayer::_StaticShutdown()"},
    {0x10040c27cu, "cSC4AudioAmbienceLayer::CanHandle()"},
    {0x10040c4fau, "cSC4AudioAmbienceLayer::HandleCell(int, int)"},
    {0x10040c82eu, "cSC4AudioAmbienceLayer::HandleOccupant(cISC4Occupant*)"},
    {0x10040caecu, "cSC4AudioAmbienceLayer::InsertOccupant(cISC4Occupant*)"},
    {0x10040cb7cu, "cSC4AudioAmbienceLayer::MoveOccupant(cISC4Occupant*)"},
    {0x10040cbfau, "cSC4AudioAmbienceLayer::RemoveOccupant(cISC4Occupant*)"},
    {0x10040cc36u, "cSC4AudioAmbienceLayer::BeginUpdate(SC4Rect<float>&)"},
    {0x10040cd3cu, "cSC4AudioAmbienceLayer::Update(unsigned int, cS3DVector3&, cS3DVector3&)"},
    {0x10040d1f6u, "cSC4AudioAmbienceLayer::_GetEquallyDistributedPositions(std::__1::list<cS3DVector3, std::__1::allocator<cS3DVector3>>&, int, cS3DVector3*, int&)"},
    {0x10040d3aeu, "cSC4AudioAmbienceLayer::_UpdateAmbienceScores(unsigned int)"},
    {0x10040d958u, "cSC4AudioAmbienceLayer::_UpdateAmbienceLoops(unsigned int)"},
    {0x10040dd56u, "cSC4AudioAmbienceLayer::EndUpdate()"},
    {0x10040dd8eu, "cSC4AudioAmbienceLayer::LoadFreshnessScore()"},
    {0x10040de80u, "cSC4AudioAmbienceLayer::AddFreshnessScore(cGZPersistResourceKey const&)"},
    {0x10040e140u, "cSC4AudioAmbienceLayer::RemoveFreshnessScore(unsigned int)"},
    {0x10040e298u, "cSC4AudioAmbienceLayer::Pause(bool)"},
    {0x10040e2beu, "cSC4AudioAmbienceLayer::AddAmbienceLoop(unsigned int)"},
    {0x10040e34au, "cSC4AudioAmbienceLayer::RemoveAmbienceLoop(unsigned int)"},
    {0x10040e39au, "cSC4AudioAmbienceLayer::DoMessage(cIGZMessage2*)"},
    {0x10040e49au, "cSC4AudioAmbienceLayer::_UpdateFreshnessParameters()"},
    {0x10040e548u, "cSC4AudioAmbienceLayer::_UpdateLoopParameters()"},
    {0x10040e5a0u, "cSC4AudioAmbienceLayer::_StaticRemoveAllFilters()"},
    {0x10040e69cu, "cSC4AudioAmbienceLayer::_AddFilter(unsigned int, int)"},
    {0x10040ea7au, "cSC4AudioAmbienceLayer::cSC4AudioAmbienceLayer(unsigned int)"},
    {0x10040f004u, "cSC4AudioAmbienceLayer::SetName(cIGZString&)"},
    // cSC4AudioAmbienceManager (47)
    {0x1003e92acu, "cSC4AudioAmbienceManager::cSC4AudioAmbienceManager()"},
    {0x1003e92b6u, "cSC4AudioAmbienceManager::~cSC4AudioAmbienceManager() [GZCOM boilerplate]"},
    {0x1003e94b4u, "cSC4AudioAmbienceManager::~cSC4AudioAmbienceManager()"},
    {0x1003e94beu, "cSC4AudioAmbienceManager::~cSC4AudioAmbienceManager() [GZCOM boilerplate]"},
    {0x1003e9a2eu, "cSC4AudioAmbienceManager::GetIndustrialPopulation()"},
    {0x1003eb210u, "cSC4AudioAmbienceManager::OnIdle(unsigned int)"},
    {0x1003eb286u, "cSC4AudioAmbienceManager::AddRef()"},
    {0x1003eb2beu, "cSC4AudioAmbienceManager::Release() [GZCOM boilerplate]"},
    {0x1003eb33au, "cSC4AudioAmbienceManager::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x1003ec678u, "cSC4AudioAmbienceManager::GetServiceID() const"},
    {0x1003ec682u, "cSC4AudioAmbienceManager::SetServiceID(unsigned int)"},
    {0x1003ec68cu, "cSC4AudioAmbienceManager::GetServicePriority() const"},
    {0x1003ec694u, "cSC4AudioAmbienceManager::IsServiceRunning() const"},
    {0x1003ec69eu, "cSC4AudioAmbienceManager::SetServiceRunning(bool)"},
    {0x1003dee26u, "cSC4AudioAmbienceManager::Create()"},
    {0x1003e9050u, "cSC4AudioAmbienceManager::cSC4AudioAmbienceManager()"},
    {0x1003e94ecu, "cSC4AudioAmbienceManager::Init()"},
    {0x1003e97e4u, "cSC4AudioAmbienceManager::Shutdown()"},
    {0x1003e99bau, "cSC4AudioAmbienceManager::_RemoveAllLayers()"},
    {0x1003e9a3au, "cSC4AudioAmbienceManager::DoEvent(cIGZSndEventStd*)"},
    {0x1003e9dd8u, "cSC4AudioAmbienceManager::_HandleSetMasterVolume(cIGZSndEventStd*)"},
    {0x1003e9e4au, "cSC4AudioAmbienceManager::DoMessage(cIGZMessage2*)"},
    {0x1003e9f96u, "cSC4AudioAmbienceManager::_HandleFocusChange(cIGZMessage2Standard*)"},
    {0x1003e9ffcu, "cSC4AudioAmbienceManager::_ReadTuningValues()"},
    {0x1003ea11au, "cSC4AudioAmbienceManager::_HandleInitMessage()"},
    {0x1003ea282u, "cSC4AudioAmbienceManager::_HandleShutdownMessage()"},
    {0x1003ea2f4u, "cSC4AudioAmbienceManager::_UpdateAttentionRect()"},
    {0x1003ea4ccu, "cSC4AudioAmbienceManager::_HandleOccupantMessages(cIGZMessage2Standard*)"},
    {0x1003ea70cu, "cSC4AudioAmbienceManager::_HandleAutomatonMessage(cIGZMessage2Standard*)"},
    {0x1003ea900u, "cSC4AudioAmbienceManager::_HandleRailwayCrossingMessage(cIGZMessage2Standard*)"},
    {0x1003eaa8eu, "cSC4AudioAmbienceManager::OnTick(unsigned int)"},
    {0x1003eabd6u, "cSC4AudioAmbienceManager::_UpdateAlarms()"},
    {0x1003eaf44u, "cSC4AudioAmbienceManager::_DoStageUpdate(unsigned int)"},
    {0x1003eb38eu, "cSC4AudioAmbienceManager::_CreateAmbienceLayersFromExemplarIds()"},
    {0x1003eb54cu, "cSC4AudioAmbienceManager::_IterateOccupantsThatNeedToBeTracked()"},
    {0x1003eb696u, "cSC4AudioAmbienceManager::_UpdateLotOccupantsForConditionChange(cISC4Lot*, int, int)"},
    {0x1003eb95eu, "cSC4AudioAmbienceManager::HandleInsertOccupant(cISC4Occupant*)"},
    {0x1003ebb40u, "cSC4AudioAmbienceManager::HandleMoveOccupant(cISC4Occupant*)"},
    {0x1003ebc36u, "cSC4AudioAmbienceManager::HandleRemoveOccupant(cISC4Occupant*)"},
    {0x1003ebd6au, "cSC4AudioAmbienceManager::_AddOccupantSndInstance(cISC4Occupant*, unsigned int, bool)"},
    {0x1003ebeb8u, "cSC4AudioAmbienceManager::_AddAlarmOccupant(cISC4Occupant*, unsigned int, std::__1::vector<float, std::__1::allocator<float>>&, bool)"},
    {0x1003ec048u, "cSC4AudioAmbienceManager::HandleInitOccupant(cISC4Occupant*)"},
    {0x1003ec1aau, "cSC4AudioAmbienceManager::_MoveOccupantSndInstance(cISC4Occupant*)"},
    {0x1003ec228u, "cSC4AudioAmbienceManager::_RemoveOccupantSndInstance(cISC4Occupant*, unsigned int)"},
    {0x1003ec2d2u, "cSC4AudioAmbienceManager::_RemoveAlarmOccupant(cISC4Occupant*, unsigned int)"},
    {0x1003ec332u, "cSC4AudioAmbienceManager::_PlayOccupantSound(cISC4Occupant*, bool)"},
    {0x1003ec4dcu, "cSC4AudioAmbienceManager::HandleOccupant(cISC4Occupant*)"},
    // cSC4AudioSoundManager (39)
    {0x1003b79ccu, "cSC4AudioSoundManager::cSC4AudioSoundManager()"},
    {0x1003b79d6u, "cSC4AudioSoundManager::~cSC4AudioSoundManager() [GZCOM boilerplate]"},
    {0x1003b821eu, "cSC4AudioSoundManager::GetMasterVolume()"},
    {0x1003b8ffeu, "cSC4AudioSoundManager::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x1003b9092u, "cSC4AudioSoundManager::AddRef()"},
    {0x1003b90cau, "cSC4AudioSoundManager::Release() [GZCOM boilerplate]"},
    {0x1003b9ae0u, "cSC4AudioSoundManager::GenerateNewInstanceId()"},
    {0x1003b7880u, "cSC4AudioSoundManager::cSC4AudioSoundManager()"},
    {0x1003b7ac0u, "cSC4AudioSoundManager::Init()"},
    {0x1003b7d2au, "cSC4AudioSoundManager::_ReadTuningValues()"},
    {0x1003b7f62u, "cSC4AudioSoundManager::Shutdown()"},
    {0x1003b80dcu, "cSC4AudioSoundManager::StopAllSounds(bool)"},
    {0x1003b817cu, "cSC4AudioSoundManager::SetMasterVolume(unsigned int)"},
    {0x1003b8236u, "cSC4AudioSoundManager::StartSound(unsigned int, unsigned int)"},
    {0x1003b82dau, "cSC4AudioSoundManager::SetSoundPosition(unsigned int, float, float, float)"},
    {0x1003b8388u, "cSC4AudioSoundManager::UpdateAllSounds()"},
    {0x1003b8426u, "cSC4AudioSoundManager::Update(int)"},
    {0x1003b84c6u, "cSC4AudioSoundManager::DoEvent(cIGZSndEventStd*)"},
    {0x1003b8622u, "cSC4AudioSoundManager::_HandleStartSound(cIGZSndEventStd*)"},
    {0x1003b86cau, "cSC4AudioSoundManager::_HandleStopSound(cIGZSndEventStd*)"},
    {0x1003b8754u, "cSC4AudioSoundManager::_HandleSetSoundPosition(cIGZSndEventStd*)"},
    {0x1003b880au, "cSC4AudioSoundManager::_HandleSetSoundVolume(cIGZSndEventStd*)"},
    {0x1003b888eu, "cSC4AudioSoundManager::_HandleResetSound(cIGZSndEventStd*)"},
    {0x1003b88fau, "cSC4AudioSoundManager::_HandleEnumerateSounds(cIGZSndEventStd*)"},
    {0x1003b8a00u, "cSC4AudioSoundManager::_HandleSetMasterVolume(cIGZSndEventStd*)"},
    {0x1003b8a6au, "cSC4AudioSoundManager::_UpdateAllSounds()"},
    {0x1003b8ab6u, "cSC4AudioSoundManager::_StopAllSounds(bool)"},
    {0x1003b8af4u, "cSC4AudioSoundManager::_HandleFocusChangeEvent(cIGZSndEventStd*)"},
    {0x1003b8becu, "cSC4AudioSoundManager::DoCompletionFunction(cIGZSnd*)"},
    {0x1003b8cacu, "cSC4AudioSoundManager::_StopSound(unsigned int, bool)"},
    {0x1003b8e3au, "cSC4AudioSoundManager::DoMessage(cIGZMessage2*)"},
    {0x1003b8ef4u, "cSC4AudioSoundManager::_HandleFocusChangeMessage(cIGZMessage2*)"},
    {0x1003b9146u, "cSC4AudioSoundManager::_ResetSound(unsigned int)"},
    {0x1003b925eu, "cSC4AudioSoundManager::_StartSound(unsigned int, unsigned int, unsigned int)"},
    {0x1003b9828u, "cSC4AudioSoundManager::_SetSoundPosition(unsigned int, float, float, float)"},
    {0x1003b98d2u, "cSC4AudioSoundManager::_SetSoundVolume(unsigned int, unsigned int)"},
    {0x1003b9a74u, "cSC4AudioSoundManager::_GetPlayingSoundMinPlayTime(unsigned int, tSound**, int&)"},
    {0x1003b9d82u, "cSC4AudioSoundManager::_RemoveFromSoundList(tSound*)"},
    {0x1003dee5eu, "cSC4AudioSoundManager::Create()"},
    // cSC4AudioAEMSEventSystem (28)
    {0x1007b58ecu, "cSC4AudioAEMSEventSystem::cSC4AudioAEMSEventSystem()"},
    {0x1007b58f6u, "cSC4AudioAEMSEventSystem::~cSC4AudioAEMSEventSystem() [GZCOM boilerplate]"},
    {0x1007b59d4u, "cSC4AudioAEMSEventSystem::~cSC4AudioAEMSEventSystem()"},
    {0x1007b59fau, "cSC4AudioAEMSEventSystem::~cSC4AudioAEMSEventSystem() [GZCOM boilerplate]"},
    {0x1007b6694u, "cSC4AudioAEMSEventSystem::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x1007b6a22u, "cSC4AudioAEMSEventSystem::AddRef()"},
    {0x1007b6a30u, "cSC4AudioAEMSEventSystem::Release()"},
    {0x1007b56a0u, "cSC4AudioAEMSEventSystem::cSC4AudioAEMSEventSystem()"},
    {0x1007b5a44u, "cSC4AudioAEMSEventSystem::Init()"},
    {0x1007b5c5cu, "cSC4AudioAEMSEventSystem::ReadTuningValues()"},
    {0x1007b5f2au, "cSC4AudioAEMSEventSystem::UpdateVolume()"},
    {0x1007b5f64u, "cSC4AudioAEMSEventSystem::LoadFromResource(cGZPersistResourceKey&)"},
    {0x1007b5ff6u, "cSC4AudioAEMSEventSystem::Shutdown()"},
    {0x1007b6106u, "cSC4AudioAEMSEventSystem::StopAllSounds()"},
    {0x1007b621cu, "cSC4AudioAEMSEventSystem::RemoveAllBanks()"},
    {0x1007b62e8u, "cSC4AudioAEMSEventSystem::LoadBankFromResource(cGZPersistResourceKey&)"},
    {0x1007b6378u, "cSC4AudioAEMSEventSystem::AddBank(cSC4AudioAEMSSoundBank*)"},
    {0x1007b6414u, "cSC4AudioAEMSEventSystem::RemoveBank(unsigned int)"},
    {0x1007b6516u, "cSC4AudioAEMSEventSystem::LoadFromFile(cIGZFile*)"},
    {0x1007b65cau, "cSC4AudioAEMSEventSystem::DoEvent(cIGZSndEventStd*)"},
    {0x1007b663au, "cSC4AudioAEMSEventSystem::DoMessage(cIGZMessage2*)"},
    {0x1007b670eu, "cSC4AudioAEMSEventSystem::LoadSoundBank(cISCPropertyHolder const*, unsigned int)"},
    {0x1007b6798u, "cSC4AudioAEMSEventSystem::UnloadSoundBank(cISCPropertyHolder const*, unsigned int)"},
    {0x1007b680au, "cSC4AudioAEMSEventSystem::StartSound(cISC4AEMSEvents::AEMSEventType)"},
    {0x1007b687eu, "cSC4AudioAEMSEventSystem::EndSound(cISC4AEMSEvents::AEMSEventType)"},
    {0x1007b68eeu, "cSC4AudioAEMSEventSystem::SetParamVelocity(SC4Percentage)"},
    {0x1007b694au, "cSC4AudioAEMSEventSystem::SetParamThrottle(SC4Percentage)"},
    {0x1007b69aau, "cSC4AudioAEMSEventSystem::SetParamWobble(SC4Percentage)"},
    // cSC4AudioEventHandler (23)
    {0x1003ed2d6u, "cSC4AudioEventHandler::cSC4AudioEventHandler()"},
    {0x1003ed2e0u, "cSC4AudioEventHandler::~cSC4AudioEventHandler() [GZCOM boilerplate]"},
    {0x1003ed3b0u, "cSC4AudioEventHandler::~cSC4AudioEventHandler()"},
    {0x1003ed3bau, "cSC4AudioEventHandler::~cSC4AudioEventHandler() [GZCOM boilerplate]"},
    {0x1003efcf2u, "cSC4AudioEventHandler::AddRef()"},
    {0x1003efd2au, "cSC4AudioEventHandler::Release() [GZCOM boilerplate]"},
    {0x1003efda6u, "cSC4AudioEventHandler::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x1003dedb2u, "cSC4AudioEventHandler::Create()"},
    {0x1003ed1a2u, "cSC4AudioEventHandler::cSC4AudioEventHandler()"},
    {0x1003ed3e8u, "cSC4AudioEventHandler::Init()"},
    {0x1003ed8ceu, "cSC4AudioEventHandler::Shutdown()"},
    {0x1003edb82u, "cSC4AudioEventHandler::DoMessage(cGZMessage&)"},
    {0x1003edb9cu, "cSC4AudioEventHandler::HandleModalMessages(cGZMessage&)"},
    {0x1003eddf0u, "cSC4AudioEventHandler::DoMessage(cIGZMessage2*)"},
    {0x1003ee2f6u, "cSC4AudioEventHandler::HandleMouseMessagesForModalWindow(cIGZMessage2*)"},
    {0x1003ee462u, "cSC4AudioEventHandler::HandleRadioMessages(cIGZMessage2*)"},
    {0x1003ee568u, "cSC4AudioEventHandler::HandleConstructionMessages(cIGZMessage2*)"},
    {0x1003eea44u, "cSC4AudioEventHandler::HandleCityInitShutdownMessages(cIGZMessage2*)"},
    {0x1003eeddau, "cSC4AudioEventHandler::HandleAutomataMessages(cIGZMessage2*)"},
    {0x1003ef31eu, "cSC4AudioEventHandler::HandleDispatchManagerMessages(cIGZMessage2*)"},
    {0x1003ef5aeu, "cSC4AudioEventHandler::_SetRadioStation(char const*, unsigned int, bool)"},
    {0x1003ef652u, "cSC4AudioEventHandler::DoEvent(cIGZSndEventStd*)"},
    {0x1003ef790u, "cSC4AudioEventHandler::HandleTorchControlMessages(cIGZSndEventStd*)"},
    // cSC4VisualEffect (29)
    {0x1005fa5ccu, "cSC4VisualEffect::cSC4VisualEffect()"},
    {0x1005fa5d6u, "cSC4VisualEffect::~cSC4VisualEffect() [GZCOM boilerplate]"},
    {0x1005fa6cau, "cSC4VisualEffect::~cSC4VisualEffect()"},
    {0x1005fa6d4u, "cSC4VisualEffect::~cSC4VisualEffect() [GZCOM boilerplate]"},
    {0x1005fa756u, "cSC4VisualEffect::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x1005fa782u, "cSC4VisualEffect::AddRef()"},
    {0x1005fa79eu, "cSC4VisualEffect::Release() [GZCOM boilerplate]"},
    {0x1005faa2au, "cSC4VisualEffect::IsStarted() const"},
    {0x1005faa3eu, "cSC4VisualEffect::IsStopping() const"},
    {0x1005fb304u, "cSC4VisualEffect::SetFinishNotifyMessage(unsigned int, unsigned int)"},
    {0x1005fb740u, "cSC4VisualEffect::CheckEffectFlag(unsigned int) const"},
    {0x1005fa460u, "cSC4VisualEffect::cSC4VisualEffect()"},
    {0x1005fa70eu, "cSC4VisualEffect::Shutdown()"},
    {0x1005fa7dau, "cSC4VisualEffect::Start(cISC4VisualEffect::tTransitionType)"},
    {0x1005fa922u, "cSC4VisualEffect::Stop(cISC4VisualEffect::tTransitionType)"},
    {0x1005faa48u, "cSC4VisualEffect::SetEffectTransform(cS3DTransform const&)"},
    {0x1005faa66u, "cSC4VisualEffect::SetSourceTransform(cS3DTransform const&)"},
    {0x1005faa7cu, "cSC4VisualEffect::SetTransforms(cS3DTransform const&, cS3DTransform const&)"},
    {0x1005fb104u, "cSC4VisualEffect::ApplyEffect(float)"},
    {0x1005fb162u, "cSC4VisualEffect::SetModelFromOccupant(cISC4Occupant*)"},
    {0x1005fb310u, "cSC4VisualEffect::SetVectorParams(cISC4VisualEffect::tParamType, cS3DVector3 const*, int)"},
    {0x1005fb322u, "cSC4VisualEffect::SetFloatParams(cISC4VisualEffect::tParamType, float const*, int)"},
    {0x1005fb4a6u, "cSC4VisualEffect::SetVoidParams(cISC4VisualEffect::tParamType, void**, int)"},
    {0x1005fb526u, "cSC4VisualEffect::GetEffectTransform(cS3DTransform&)"},
    {0x1005fb588u, "cSC4VisualEffect::GetSourceTransform(cS3DTransform&)"},
    {0x1005fb60eu, "cSC4VisualEffect::SetDescription(cSC4EffectDescription const&)"},
    {0x1005fb750u, "cSC4VisualEffect::ApplyParams()"},
    {0x1005fb7e4u, "cSC4VisualEffect::CheckIfStopped()"},
    {0x1005fb928u, "cSC4VisualEffect::AddComponentEffect(cISC4ComponentEffect*, cS3DTransform*)"},
}};

[[nodiscard]] inline constexpr Uint32 Batch3EntryPointCount() noexcept {
    return static_cast<Uint32>(kBatch3EntryPoints.size());
}

[[nodiscard]] inline constexpr const char* Batch3EntryPointName(Uint64 address) noexcept {
    for (const Batch3EntryPoint& entry : kBatch3EntryPoints) {
        if (entry.address == address) {
            return entry.symbol;
        }
    }
    return nullptr;
}

static_assert(kBatch3EntryPoints.size() == 806);
static_assert(AdviceCheckFlags(0x3, 0x2));
static_assert(!AdviceCheckFlags(0x1, 0x2));
static_assert(SituationCheckFlag(0x5u, 0x1u));
static_assert(!SituationCheckFlag(0x5u, 0x2u));
static_assert(MySimZodiacName(0)[0] == 'A');
static_assert(MySimDisplayedEq(2.9f) == 2);
static_assert(MySimNightSoundId(2, 0) == kMySimNightSoundIdWhenSexClear);
static_assert(MySimNightSoundId(2, 1) == kMySimNightSoundIdWhenSexSet);
static_assert(MySimNightSoundId(0, 1) == 0u);
static_assert(FirePhaseShouldAdvance(0, true));
static_assert(FirePhaseShouldEnd(kDisasterPhaseOngoing, false));
static_assert(!FirePhaseShouldAdvance(kDisasterPhaseOngoing, true));
static_assert(FireCauseAfterSet(0u, 7u) == 7u);
static_assert(FireCauseAfterSet(3u, 7u) == 3u);
static_assert(VolcanoStateAccepted(5));
static_assert(!VolcanoStateAccepted(6));
static_assert(VolcanoPhaseFromState(6, {0, 1, 2, 3, 4}) == kVolcanoPhasePastTable);
static_assert(DisastersAreEnabled(0));
static_assert(!DisastersAreEnabled(1));
static_assert(DisasterIsOngoing(1));
static_assert(AdvicePopupThresholdForCityLevel({10, 20, 30}, 2u) == 30);
static_assert(AdvicePopupThresholdForCityLevel({10, 20, 30}, 9u) == 10);
static_assert(Batch3EntryPointName(0x10043dfe6u) != nullptr);

// ---------------------------------------------------------------------------
// Wave 3 Part 4 method index
// 75 classes, 452 macOS x86_64 entry points (0x100xxxxxx) from the
// consolidated workpack. Addresses are image VAs; sizeBytes is the
// decoded function extent.
// ---------------------------------------------------------------------------

struct Wave3Part4Method {
    Uint64 address;
    Uint32 sizeBytes;
    const char* name;
};

inline constexpr std::array<Wave3Part4Method, 452> kWave3Part4Methods = {{
    // cSC4SubBudgetWindow (37)
    {0x100330e40u, 3980u, "cSC4SubBudgetWindow::set_comboBox(cIGZWin*, unsigned int, unsigned int, cISC4NeighborDeal*, unsigned int, int, cSC4SubBudgetWindow::ComboInfo&)"},
    {0x100331dccu, 1248u, "cSC4SubBudgetWindow::add_comboBox(cIGZWin*, unsigned int, unsigned int, unsigned int, int)"},
    {0x1003322acu, 268u, "cSC4SubBudgetWindow::add_slider(cIGZWin*, unsigned int, unsigned int, unsigned int, unsigned int, int, int, int, int)"},
    {0x1003323b8u, 110u, "cSC4SubBudgetWindow::set_window_department(cIGZWin*, unsigned int, cISC4DepartmentBudget*)"},
    {0x100332426u, 110u, "cSC4SubBudgetWindow::get_window_department(cIGZWin*, unsigned int)"},
    {0x100332494u, 110u, "cSC4SubBudgetWindow::set_window_indiv_item(cIGZWin*, unsigned int, cISCPropertyHolder*)"},
    {0x100332502u, 110u, "cSC4SubBudgetWindow::get_window_indiv_item(cIGZWin*, unsigned int)"},
    {0x100332570u, 644u, "cSC4SubBudgetWindow::add_text(cIGZWin*, unsigned int, int, int, cIGZString const&, unsigned int, unsigned int, unsigned char, unsigned char, unsigned char)"},
    {0x1003327f4u, 380u, "cSC4SubBudgetWindow::add_fixed_text(cIGZWin*, unsigned int, int, int, int, cRZString const&, unsigned int, unsigned int, unsigned char, unsigned char, unsigned char)"},
    {0x100332970u, 594u, "cSC4SubBudgetWindow::add_text_button(cIGZWin*, unsigned int, int, int, int, cRZString const&, cIGZWinBtn::tCapStyle, unsigned int, unsigned char, unsigned char, unsigned char)"},
    {0x100332bc2u, 590u, "cSC4SubBudgetWindow::add_image_button1(cIGZWin*, unsigned int, int, int, unsigned int, char, bool)"},
    {0x100332e10u, 1132u, "cSC4SubBudgetWindow::add_image_button2(cIGZWin*, unsigned int, int, int, unsigned int, char, bool)"},
    {0x10033327cu, 784u, "cSC4SubBudgetWindow::cSC4SubBudgetWindow()"},
    {0x100333826u, 500u, "cSC4SubBudgetWindow::CreateSubBudgetWindow(cIGZWin*&, unsigned int, unsigned int, unsigned int)"},
    {0x100333a1au, 284u, "cSC4SubBudgetWindow::DestroySubBudgetWindow()"},
    {0x100333b36u, 142u, "cSC4SubBudgetWindow::end_dialog(cIGZWin*, unsigned int)"},
    {0x100333bc4u, 440u, "cSC4SubBudgetWindow::add_Bitmap(unsigned int, unsigned int, unsigned int, cIGZWin*, unsigned int)"},
    {0x100333d7cu, 318u, "cSC4SubBudgetWindow::GetBudgetSegment(unsigned int, unsigned int, unsigned int, cIGZWin*)"},
    {0x100333ebau, 634u, "cSC4SubBudgetWindow::SetupWindow(cIGZWin*, unsigned int, unsigned int)"},
    {0x100334134u, 634u, "cSC4SubBudgetWindow::SetupWindowBig(cIGZWin*, unsigned int, unsigned int)"},
    {0x1003343aeu, 512u, "cSC4SubBudgetWindow::SetupWindowIndiv(cIGZWin*, unsigned int)"},
    {0x1003345aeu, 10400u, "cSC4SubBudgetWindow::on_init(cIGZWin*)"},
    {0x100336e4eu, 5628u, "cSC4SubBudgetWindow::initOrdinanceBudget(cIGZWin*)"},
    {0x10033844au, 19362u, "cSC4SubBudgetWindow::initNeighborBudget(cIGZWin*)"},
    {0x10033cfecu, 4468u, "cSC4SubBudgetWindow::initIndividualBudget(cIGZWin*)"},
    {0x10033e160u, 1624u, "cSC4SubBudgetWindow::SetEmptyWindow(cIGZWin*, cRZString const&, cRZString const&)"},
    {0x10033e7b8u, 152u, "cSC4SubBudgetWindow::RestoreStartSettings(cIGZWin*)"},
    {0x10033e850u, 758u, "cSC4SubBudgetWindow::SetOccupantFundingLevel(cISCPropertyHolder*, bool, float)"},
    {0x10033eb46u, 6358u, "cSC4SubBudgetWindow::UpdateValues(cIGZWin*)"},
    {0x10034041cu, 19948u, "cSC4SubBudgetWindow::updateNeighborBudget(cIGZWin*)"},
    {0x100345208u, 350u, "cSC4SubBudgetWindow::RestoreDeals()"},
    {0x100345366u, 2766u, "cSC4SubBudgetWindow::updateOrdinanceBudget(cIGZWin*)"},
    {0x100345e34u, 444u, "cSC4SubBudgetWindow::DoIndividualBudgetWindow(unsigned int)"},
    {0x100345ff0u, 3166u, "cSC4SubBudgetWindow::updateIndividualBudget(cIGZWin*, bool)"},
    {0x100346c4eu, 448u, "cSC4SubBudgetWindow::on_shutdown(cIGZWin*)"},
    {0x100346e0eu, 2398u, "cSC4SubBudgetWindow::DoWinMsg(cIGZWin*, unsigned int, unsigned long, unsigned long, unsigned long)"},
    {0x10034776cu, 3060u, "cSC4SubBudgetWindow::on_button_activated(cIGZWin*, unsigned int)"},
    // cSC4NetworkToolUI (26)
    {0x10071ff90u, 364u, "cSC4NetworkToolUI::cSC4NetworkToolUI()"},
    {0x10072048eu, 190u, "cSC4NetworkToolUI::Init(cISC4NetworkTool*, cISC4NetworkManager::eNetworkType)"},
    {0x10072054cu, 60u, "cSC4NetworkToolUI::Shutdown()"},
    {0x100720588u, 264u, "cSC4NetworkToolUI::AbortPlacement()"},
    {0x100720690u, 72u, "cSC4NetworkToolUI::DoMessage(cIGZMessage2*)"},
    {0x1007206d8u, 286u, "cSC4NetworkToolUI::UpdateIndicators(bool)"},
    {0x1007207f6u, 558u, "cSC4NetworkToolUI::UpdatePreview()"},
    {0x100720a32u, 18u, "cSC4NetworkToolUI::ViewInputControlId()"},
    {0x100720a44u, 18u, "cSC4NetworkToolUI::CursorId()"},
    {0x100720a56u, 22u, "cSC4NetworkToolUI::PenWidth(int&, int&)"},
    {0x100720a6cu, 58u, "cSC4NetworkToolUI::EnterUI()"},
    {0x100720aa6u, 58u, "cSC4NetworkToolUI::LeaveUI()"},
    {0x100720ae0u, 452u, "cSC4NetworkToolUI::OnMouseDownL(int, int, unsigned int)"},
    {0x100720ca4u, 420u, "cSC4NetworkToolUI::ScreenToCell(int, int, SC4Point<unsigned int>&, float*, bool, float, float, float)"},
    {0x100720e48u, 746u, "cSC4NetworkToolUI::OnMouseMove(int, int, unsigned int)"},
    {0x100721132u, 298u, "cSC4NetworkToolUI::LiesOnSegment(SC4Point<unsigned int>&, SC4Point<unsigned int>&, SC4Point<unsigned int>&)"},
    {0x10072125cu, 608u, "cSC4NetworkToolUI::UpdateAnchorPoint(SC4Point<unsigned int> const&)"},
    {0x1007214bcu, 988u, "cSC4NetworkToolUI::OnMouseUpL(int, int, unsigned int)"},
    {0x100721b78u, 396u, "cSC4NetworkToolUI::TestPlacement(bool)"},
    {0x100721d04u, 1460u, "cSC4NetworkToolUI::AddBridgeEntry(cRZString const&, long long, unsigned int, float)"},
    {0x1007222b8u, 612u, "cSC4NetworkToolUI::OnButtonActivated(unsigned int)"},
    {0x10072251cu, 644u, "cSC4NetworkToolUI::DoPlacement()"},
    {0x1007227b4u, 18u, "cSC4NetworkToolUI::DirectionalCursorBase()"},
    {0x1007227c6u, 218u, "cSC4NetworkToolUI::LastDragDirection()"},
    {0x1007228a0u, 27u, "cSC4NetworkToolUI::AttachToView(cISC4View3DWin*)"},
    {0x1007229beu, 456u, "cSC4NetworkToolUI::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    // cSC4MySimWinProc (24)
    {0x100315330u, 508u, "cSC4MySimWinProc::cSC4MySimWinProc(cIGZWinGen*)"},
    {0x10031571eu, 1026u, "cSC4MySimWinProc::Init()"},
    {0x100315b20u, 198u, "cSC4MySimWinProc::SetDialogState(unsigned int)"},
    {0x100315eb2u, 580u, "cSC4MySimWinProc::Shutdown()"},
    {0x100316192u, 1902u, "cSC4MySimWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x100316900u, 2152u, "cSC4MySimWinProc::DisplayVehicleIcons(cIGZWin*)"},
    {0x100317168u, 464u, "cSC4MySimWinProc::SetMySimMainWindow()"},
    {0x100317338u, 934u, "cSC4MySimWinProc::SetMySimModeWindow(unsigned int)"},
    {0x1003176deu, 270u, "cSC4MySimWinProc::SetMySimLargeWindow()"},
    {0x1003177ecu, 264u, "cSC4MySimWinProc::SetMySimSmallWindow()"},
    {0x1003178f4u, 98u, "cSC4MySimWinProc::BootSim()"},
    {0x100317956u, 1378u, "cSC4MySimWinProc::SetMySimStatWindow()"},
    {0x100317eb8u, 1320u, "cSC4MySimWinProc::ShowChooseTransportDialog(bool)"},
    {0x1003183e0u, 204u, "cSC4MySimWinProc::SetCurrentSim(int, bool, bool)"},
    {0x1003184acu, 654u, "cSC4MySimWinProc::DoMessage(cIGZMessage2*)"},
    {0x10031873au, 1754u, "cSC4MySimWinProc::GetCurrentSims()"},
    {0x100318e14u, 8496u, "cSC4MySimWinProc::updateBodyTextCtrl()"},
    {0x10031af44u, 150u, "cSC4MySimWinProc::RemoveMessage(cISC4Advice*)"},
    {0x10031afdau, 126u, "cSC4MySimWinProc::AddMessage(cISC4Advice*)"},
    {0x10031b06au, 238u, "cSC4MySimWinProc::AddSim(unsigned int)"},
    {0x10031b158u, 210u, "cSC4MySimWinProc::SetTransportButtonImage(unsigned int, bool)"},
    {0x10031b22au, 686u, "cSC4MySimWinProc::GetMessageList(cRZAutoRefCount<cISC4Advisor>)"},
    {0x10031b4d8u, 114u, "cSC4MySimWinProc::Hide()"},
    {0x10031b54au, 122u, "cSC4MySimWinProc::Show()"},
    // cSC4TutorialUIWinProc (23)
    {0x1000b697cu, 164u, "cSC4TutorialUIWinProc::cSC4TutorialUIWinProc()"},
    {0x1000b6c6eu, 122u, "cSC4TutorialUIWinProc::LoadArrow(int, int, cIGZWin**)"},
    {0x1000b6ce8u, 164u, "cSC4TutorialUIWinProc::KillArrow()"},
    {0x1000b6d8cu, 862u, "cSC4TutorialUIWinProc::Init()"},
    {0x1000b70eau, 402u, "cSC4TutorialUIWinProc::load_task_script()"},
    {0x1000b727cu, 500u, "cSC4TutorialUIWinProc::ReStartCurrentTask()"},
    {0x1000b7470u, 625u, "cSC4TutorialUIWinProc::Shutdown()"},
    {0x1000b78c4u, 594u, "cSC4TutorialUIWinProc::FilterLeftMouseClick(int&, int&, int)"},
    {0x1000b7bbau, 252u, "cSC4TutorialUIWinProc::finish_tutorial(bool)"},
    {0x1000b7cb6u, 806u, "cSC4TutorialUIWinProc::exit_to_region()"},
    {0x1000b7fdcu, 1042u, "cSC4TutorialUIWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x1000b83eeu, 1046u, "cSC4TutorialUIWinProc::moveToNext()"},
    {0x1000b883eu, 414u, "cSC4TutorialUIWinProc::OnTutorialEvent(cISC4TutorialSystem::eTutorialEvent)"},
    {0x1000b89dcu, 6040u, "cSC4TutorialUIWinProc::DoMessage(cIGZMessage2*)"},
    {0x1000ba174u, 260u, "cSC4TutorialUIWinProc::IsRightBuildingPosition(float, float)"},
    {0x1000ba278u, 250u, "cSC4TutorialUIWinProc::tick_self(int, int, int, void*, cIGZUnknown*)"},
    {0x1000ba372u, 162u, "cSC4TutorialUIWinProc::DrawArrow(cIGZWin*)"},
    {0x1000ba414u, 578u, "cSC4TutorialUIWinProc::DrawArrow(int, int, int, int)"},
    {0x1000ba668u, 210u, "cSC4TutorialUIWinProc::ResetTutorialNumAndIndex(int, int)"},
    {0x1000ba7b0u, 2288u, "cSC4TutorialUIWinProc::DoSpecialDocumentProcessing(cRZString const&)"},
    {0x1000bb0a0u, 116u, "cSC4TutorialUIWinProc::handleButtons(unsigned int)"},
    {0x1000bb114u, 378u, "cSC4TutorialUIWinProc::HandleCatalogView(cISC4WinCatalogView*)"},
    {0x1000bb28eu, 883u, "cSC4TutorialUIWinProc::ShowUIHintsForUsers()"},
    // cSC4AdvisorUIAdvisorWinProc (21)
    {0x1002b528cu, 164u, "cSC4AdvisorUIAdvisorWinProc::cSC4AdvisorUIAdvisorWinProc()"},
    {0x1002b5502u, 396u, "cSC4AdvisorUIAdvisorWinProc::Init()"},
    {0x1002b568eu, 398u, "cSC4AdvisorUIAdvisorWinProc::SetupAllAdvisorWinProcs()"},
    {0x1002b581cu, 408u, "cSC4AdvisorUIAdvisorWinProc::Shutdown()"},
    {0x1002b59b4u, 96u, "cSC4AdvisorUIAdvisorWinProc::RemoveAllAdvisorWinProcs()"},
    {0x1002b5a14u, 260u, "cSC4AdvisorUIAdvisorWinProc::RegisterMessageNofitication(bool)"},
    {0x1002b5bb2u, 390u, "cSC4AdvisorUIAdvisorWinProc::SetupSingleAdvisorWinProc(unsigned int, unsigned int)"},
    {0x1002b5d38u, 448u, "cSC4AdvisorUIAdvisorWinProc::get_advisor_mood(cISC4Advisor const*)"},
    {0x1002b5ef8u, 94u, "cSC4AdvisorUIAdvisorWinProc::RemoveSingleAdvisorWinProc(unsigned int)"},
    {0x1002b5f56u, 120u, "cSC4AdvisorUIAdvisorWinProc::SetAllMoods()"},
    {0x1002b5fceu, 1514u, "cSC4AdvisorUIAdvisorWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x1002b65b8u, 1206u, "cSC4AdvisorUIAdvisorWinProc::SetIndividualAdvisorWindow(int)"},
    {0x1002b6a6eu, 226u, "cSC4AdvisorUIAdvisorWinProc::SetAllAdvisorsWindow()"},
    {0x1002b6b50u, 320u, "cSC4AdvisorUIAdvisorWinProc::SetLargeIndividualAdvisorWindow()"},
    {0x1002b6c90u, 320u, "cSC4AdvisorUIAdvisorWinProc::SetSmallIndividualAdvisorWindow()"},
    {0x1002b6dd0u, 266u, "cSC4AdvisorUIAdvisorWinProc::HideWindow()"},
    {0x1002b6edau, 760u, "cSC4AdvisorUIAdvisorWinProc::DoMessage(cIGZMessage2*)"},
    {0x1002b71d2u, 8522u, "cSC4AdvisorUIAdvisorWinProc::updateBodyTextCtrl()"},
    {0x1002b931cu, 102u, "cSC4AdvisorUIAdvisorWinProc::AddMessage(cISC4Advice*)"},
    {0x1002b939au, 586u, "cSC4AdvisorUIAdvisorWinProc::GetMessageList(cISC4Advisor*)"},
    {0x1002b9634u, 222u, "cSC4AdvisorUIAdvisorWinProc::ShowWindow()"},
    // cSC4ResourceLibCOMDirector (20)
    {0x100522c00u, 188u, "cSC4ResourceLibCOMDirector::PreAppInit()"},
    {0x100522daeu, 128u, "cSC4ResourceLibCOMDirector::OnStart(cIGZCOM*)"},
    {0x100522e68u, 582u, "cSC4ResourceLibCOMDirector::cSC4ResourceLibCOMDirector()"},
    {0x1005230aeu, 56u, "cSC4ResourceLibCOMDirector::CreateSCResExemplarFactory()"},
    {0x1005230e6u, 56u, "cSC4ResourceLibCOMDirector::CreateSCResExemplarCohortFactory()"},
    {0x10052311eu, 60u, "cSC4ResourceLibCOMDirector::CreateSCExemplarCache()"},
    {0x10052315au, 56u, "cSC4ResourceLibCOMDirector::CreatecSCResourceCatalogView()"},
    {0x100523192u, 134u, "cSC4ResourceLibCOMDirector::CreatecSCHappinessCurveEntryView()"},
    {0x100523218u, 62u, "cSC4ResourceLibCOMDirector::CreatecSCGeneralResponseCurveEntryView()"},
    {0x100523256u, 56u, "cSC4ResourceLibCOMDirector::CreatecSCResCatalogPickerView()"},
    {0x10052328eu, 90u, "cSC4ResourceLibCOMDirector::CreatecSCStringEntryView()"},
    {0x1005232e8u, 90u, "cSC4ResourceLibCOMDirector::CreatecSCSint64TextEntryView()"},
    {0x100523342u, 90u, "cSC4ResourceLibCOMDirector::CreatecSCUint32TextEntryView()"},
    {0x10052339cu, 90u, "cSC4ResourceLibCOMDirector::CreatecSCSint32TextEntryView()"},
    {0x1005233f6u, 90u, "cSC4ResourceLibCOMDirector::CreatecSCSint8TextEntryView()"},
    {0x100523450u, 90u, "cSC4ResourceLibCOMDirector::CreatecSCUint8TextEntryView()"},
    {0x1005234aau, 90u, "cSC4ResourceLibCOMDirector::CreatecSCFloat32TextEntryView()"},
    {0x100523504u, 90u, "cSC4ResourceLibCOMDirector::CreatecSCUint32SpinnerEntryView()"},
    {0x10052355eu, 90u, "cSC4ResourceLibCOMDirector::CreatecSCBoolTextEntryView()"},
    {0x1005235b8u, 56u, "cSC4ResourceLibCOMDirector::CreateResSpriteAnimFactory()"},
    // cSC4WinNetworkLotEditor (13)
    {0x1003d3642u, 448u, "cSC4WinNetworkLotEditor::cSC4WinNetworkLotEditor()"},
    {0x1003d380cu, 1058u, "cSC4WinNetworkLotEditor::Refresh3DView()"},
    {0x1003d3c2eu, 356u, "cSC4WinNetworkLotEditor::UpdateNetworkLotConfiguration()"},
    {0x1003d3d92u, 112u, "cSC4WinNetworkLotEditor::GetDefaultPath(cRZString&)"},
    {0x1003d3e02u, 54u, "cSC4WinNetworkLotEditor::CreateNewLotConfiguration()"},
    {0x1003d3eaeu, 358u, "cSC4WinNetworkLotEditor::UpdateRegularLotConfiguration()"},
    {0x1003d4014u, 132u, "cSC4WinNetworkLotEditor::ReadLotConfigurationExemplar(cGZPersistResourceKey const&)"},
    {0x1003d4098u, 4500u, "cSC4WinNetworkLotEditor::DoEditAreaMenu(int, int)"},
    {0x1003d522cu, 542u, "cSC4WinNetworkLotEditor::DoSetNetworkTileUI()"},
    {0x1003d544au, 4332u, "cSC4WinNetworkLotEditor::DoNetworkCompatibilitiesUI()"},
    {0x1003d6536u, 50u, "cSC4WinNetworkLotEditor::Close(bool, bool)"},
    {0x1003d6568u, 50u, "cSC4WinNetworkLotEditor::WriteLotData()"},
    {0x1003d659au, 50u, "cSC4WinNetworkLotEditor::SaveAs(bool, bool, bool)"},
    // cSC4PropOccupantVisualEffect (12)
    {0x1001606f2u, 212u, "cSC4PropOccupantVisualEffect::cSC4PropOccupantVisualEffect()"},
    {0x100160950u, 574u, "cSC4PropOccupantVisualEffect::Init()"},
    {0x100160b8eu, 654u, "cSC4PropOccupantVisualEffect::RestartEffect()"},
    {0x100160e40u, 94u, "cSC4PropOccupantVisualEffect::Shutdown()"},
    {0x100161216u, 32u, "cSC4PropOccupantVisualEffect::SetPosition(float const*)"},
    {0x100161236u, 318u, "cSC4PropOccupantVisualEffect::UpdateVisualEffectTransform()"},
    {0x1001613b4u, 34u, "cSC4PropOccupantVisualEffect::SetOrientation(int)"},
    {0x1001613feu, 118u, "cSC4PropOccupantVisualEffect::SetAllFlags(unsigned int)"},
    {0x1001614c2u, 362u, "cSC4PropOccupantVisualEffect::SetEffectType(char const*)"},
    {0x10016162cu, 78u, "cSC4PropOccupantVisualEffect::SetEffectType(unsigned int)"},
    {0x10016167au, 82u, "cSC4PropOccupantVisualEffect::StartEffect(bool)"},
    {0x1001616ccu, 70u, "cSC4PropOccupantVisualEffect::StopEffect(bool)"},
    // cSC4PatrolCar (12)
    {0x10050e98cu, 120u, "cSC4PatrolCar::cSC4PatrolCar()"},
    {0x10050ea04u, 28u, "cSC4PatrolCar::Shutdown()"},
    {0x10050ea20u, 98u, "cSC4PatrolCar::ReleaseRiotPolice()"},
    {0x10050eab0u, 100u, "cSC4PatrolCar::GetEffectiveRadius() const"},
    {0x10050eb1eu, 40u, "cSC4PatrolCar::HandleCrewFinished()"},
    {0x10050eb46u, 320u, "cSC4PatrolCar::SetDispatchState(cISC4DispatchManager::tDispatchState)"},
    {0x10050ec86u, 322u, "cSC4PatrolCar::CreateRiotPolice()"},
    {0x10050edc8u, 112u, "cSC4PatrolCar::DoHandlingEventTick(float)"},
    {0x10050ef76u, 68u, "cSC4PatrolCar::HandleEnterState(tAutomataState*)"},
    {0x10050efbau, 84u, "cSC4PatrolCar::DoStateTick(float)"},
    {0x10050f00eu, 636u, "cSC4PatrolCar::DoArrestTick(float)"},
    {0x10050f28au, 78u, "cSC4PatrolCar::HandleExitState(tAutomataState const*)"},
    // cSC4NetworkPreview (12)
    {0x100718ce0u, 172u, "cSC4NetworkPreview::cSC4NetworkPreview()"},
    {0x100718e6cu, 588u, "cSC4NetworkPreview::Init()"},
    {0x1007190b8u, 50u, "cSC4NetworkPreview::Shutdown()"},
    {0x1007190eau, 246u, "cSC4NetworkPreview::AddUntexturedSelection(SC4Point<int> const&, unsigned int, float const*, float, bool)"},
    {0x1007191e0u, 342u, "cSC4NetworkPreview::SetVertexPositions(cSC4NetworkPreview::cBaseCell&, SC4Point<int> const&, unsigned int, float const*, int, bool, float, bool)"},
    {0x100719336u, 514u, "cSC4NetworkPreview::AddTexturedSelection(SC4Point<int> const&, unsigned int, float const*, cGZPersistResourceKey const&, int, bool, float, bool)"},
    {0x100719538u, 132u, "cSC4NetworkPreview::Erase(SC4Point<int> const&)"},
    {0x1007195bcu, 86u, "cSC4NetworkPreview::Clear()"},
    {0x100719612u, 292u, "cSC4NetworkPreview::AdjustCellHeights(SC4Point<int> const&, float const*, bool)"},
    {0x100719736u, 302u, "cSC4NetworkPreview::SetVertexHeights(cSC4NetworkPreview::cBaseCell&, SC4Point<int> const&, float const*, bool)"},
    {0x100719864u, 126u, "cSC4NetworkPreview::Update(SC4DrawContext*)"},
    {0x1007198e2u, 438u, "cSC4NetworkPreview::DrawCells(SC4DrawContext*)"},
    // cSC4DisasterInstanceRiot (11)
    {0x10044cc56u, 318u, "cSC4DisasterInstanceRiot::Init()"},
    {0x10044cd94u, 728u, "cSC4DisasterInstanceRiot::AddRiotLeader(int, int, bool)"},
    {0x10044d4feu, 254u, "cSC4DisasterInstanceRiot::cSC4DisasterInstanceRiot()"},
    {0x10044d732u, 42u, "cSC4DisasterInstanceRiot::DoMessage(cIGZMessage2*)"},
    {0x10044d75cu, 1254u, "cSC4DisasterInstanceRiot::Update()"},
    {0x10044dcccu, 954u, "cSC4DisasterInstanceRiot::ReadTuningParameters()"},
    {0x10044e086u, 156u, "cSC4DisasterInstanceRiot::MarkRiotNetworkOccupants(cISC4NetworkOccupant*, int, int)"},
    {0x10044e122u, 230u, "cSC4DisasterInstanceRiot::UpdateRiotEffects(cISC4Automaton*, int)"},
    {0x10044e24au, 184u, "cSC4DisasterInstanceRiot::UpdatePhase()"},
    {0x10044e364u, 428u, "cSC4DisasterInstanceRiot::FindNearestRoadTile(int&, int&)"},
    {0x10044e5beu, 126u, "cSC4DisasterInstanceRiot::Shutdown()"},
    // cSC4MyStreetSim (11)
    {0x1006b9ae4u, 264u, "cSC4MyStreetSim::cSC4MyStreetSim()"},
    {0x1006b9df2u, 52u, "cSC4MyStreetSim::Init(int)"},
    {0x1006b9ee4u, 262u, "cSC4MyStreetSim::DoMessage(cIGZMessage2*)"},
    {0x1006b9feau, 3892u, "cSC4MyStreetSim::update_trip_variables()"},
    {0x1006baf2cu, 54u, "cSC4MyStreetSim::simulate_day()"},
    {0x1006baf62u, 52u, "cSC4MyStreetSim::simulate_day_events()"},
    {0x1006bafacu, 2090u, "cSC4MyStreetSim::AttachAutomaton(cISC4Automaton*)"},
    {0x1006bb7eau, 230u, "cSC4MyStreetSim::DetachAutomaton()"},
    {0x1006bb8e4u, 258u, "cSC4MyStreetSim::HideLocation()"},
    {0x1006bb9f8u, 92u, "cSC4MyStreetSim::ShowLocation()"},
    {0x1006bba66u, 148u, "cSC4MyStreetSim::CreateMyStreetSimTool()"},
    // nSC4Networks (10)
    {0x1001b4568u, 76u, "nSC4Networks::cIntRule::GetTileInfo(int, int) const"},
    {0x1001cac66u, 1768u, "nSC4Networks::cIntRule::operator=(nSC4Networks::cIntRule const&)"},
    {0x10022c20au, 580u, "nSC4Networks::cIntRule::Transpose()"},
    {0x10022c576u, 821u, "nSC4Networks::cIntRule::Rotate(int)"},
    {0x10022cab2u, 206u, "nSC4Networks::cIntRule::Translate(int, int)"},
    {0x10022cb80u, 1234u, "nSC4Networks::cIntRule::ParseCellsFromASCIIArt(std::__1::list<cRZString, std::__1::allocator<cRZString>> const&)"},
    {0x10022d0b4u, 504u, "nSC4Networks::cIntRule::ParseConstraintsFromASCIIArt(std::__1::list<cRZString, std::__1::allocator<cRZString>> const&)"},
    {0x10022d2d8u, 398u, "nSC4Networks::cIntRule::DoAutoComplete(int, int, cSC4NetworkTool*) const"},
    {0x10022d466u, 1238u, "nSC4Networks::cIntRule::Test(int, int, cSC4NetworkTool*) const"},
    {0x10022d93cu, 78u, "nSC4Networks::cIntRule::GetTypeAtOrigin() const"},
    // cSC4PolygonClipper<sVF_V3F_T2F> (10)
    {0x1007150a0u, 196u, "cSC4PolygonClipper<sVF_V3F_T2F>::Allocate()"},
    {0x100715164u, 160u, "cSC4PolygonClipper<sVF_V3F_T2F>::AddPrimitives(cS3DPrimitiveData*, unsigned short const*, int)"},
    {0x100715204u, 174u, "cSC4PolygonClipper<sVF_V3F_T2F>::MoveTo(std::__1::vector<unsigned char, cRZFastSTLAllocatorThreadSafe<unsigned char>>&, std::__1::vector<unsigned short, cRZFastSTLAllocatorThreadSafe<unsigned short>>&)"},
    {0x1007152b2u, 182u, "cSC4PolygonClipper<sVF_V3F_T2F>::Free(cSC4PolygonClipper<sVF_V3F_T2F>*)"},
    {0x100715e20u, 160u, "cSC4PolygonClipper<sVF_V3F_T2F>::StaticShutdown(void*)"},
    {0x10071611cu, 722u, "cSC4PolygonClipper<sVF_V3F_T2F>::AddPrimitive(unsigned short const*, int, int)"},
    {0x1007163eeu, 892u, "cSC4PolygonClipper<sVF_V3F_T2F>::ClipPolygonY(unsigned short const*, int const*, int)"},
    {0x10071676au, 2246u, "cSC4PolygonClipper<sVF_V3F_T2F>::ClipPolygonXZ(unsigned short const*, int const*, int)"},
    {0x100717030u, 676u, "cSC4PolygonClipper<sVF_V3F_T2F>::TranslateOriginalVertIndex(unsigned short)"},
    {0x1007172d4u, 340u, "cSC4PolygonClipper<sVF_V3F_T2F>::ComputeLineToRectIntersection(unsigned int, sVF_V3F_T2F const&, sVF_V3F_T2F const&)"},
    // cSC4AutomataAttractor (9)
    {0x1000f7872u, 274u, "cSC4AutomataAttractor::TestAutomataFilter(cISC4Occupant*) const"},
    {0x1000f7984u, 132u, "cSC4AutomataAttractor::GetCurrentStrength(tControllerTime const&, float) const"},
    {0x1000fa0b6u, 200u, "cSC4AutomataAttractor::cSC4AutomataAttractor()"},
    {0x1000fa188u, 60u, "cSC4AutomataAttractor::Init()"},
    {0x1000fa26cu, 156u, "cSC4AutomataAttractor::HandleEnterState(cISC4Automaton*, unsigned int)"},
    {0x1000fa308u, 140u, "cSC4AutomataAttractor::HandleExitState(cISC4Automaton*, unsigned int)"},
    {0x1000fa3acu, 52u, "cSC4AutomataAttractor::SetAutomataGroups(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>> const&)"},
    {0x1000fba4au, 42u, "cSC4AutomataAttractor::FinalRelease()"},
    {0x1000fbe8eu, 348u, "cSC4AutomataAttractor::tTemplate::tTemplate()"},
    // cSC4BudgetUIExpenseWinProc (9)
    {0x100242930u, 236u, "cSC4BudgetUIExpenseWinProc::cSC4BudgetUIExpenseWinProc()"},
    {0x100242b86u, 274u, "cSC4BudgetUIExpenseWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x100242c98u, 496u, "cSC4BudgetUIExpenseWinProc::InitDialog()"},
    {0x100242e88u, 186u, "cSC4BudgetUIExpenseWinProc::SaveChanges()"},
    {0x100242f42u, 140u, "cSC4BudgetUIExpenseWinProc::EndDialog()"},
    {0x100242fceu, 324u, "cSC4BudgetUIExpenseWinProc::SetInfoPerSlider(unsigned int)"},
    {0x100243130u, 208u, "cSC4BudgetUIExpenseWinProc::DisplayDepartmentInfo(cSC4BudgetUIExpenseWinProc::cUIDepartmentInfo&)"},
    {0x100243200u, 236u, "cSC4BudgetUIExpenseWinProc::SetFormattedAmountInTextControl(unsigned int, long long)"},
    {0x1002432ecu, 228u, "cSC4BudgetUIExpenseWinProc::RecalculateTotals()"},
    // cSC4WinAdviceList (9)
    {0x1005c7a54u, 294u, "cSC4WinAdviceList::Init()"},
    {0x1005c7b7au, 32u, "cSC4WinAdviceList::Shutdown()"},
    {0x1005c7b9au, 114u, "cSC4WinAdviceList::SetArea(int, int, int, int)"},
    {0x1005c7c48u, 1062u, "cSC4WinAdviceList::DoMessage(cGZMessage&)"},
    {0x1005c806eu, 426u, "cSC4WinAdviceList::AddEntry(cIGZString const&, cIGZString const&, cIGZString const&, cIGZString const&, bool, cISC4Advice*, cIGZString const&, bool, unsigned int)"},
    {0x1005c82a0u, 1394u, "cSC4WinAdviceList::RebuildHTML(bool)"},
    {0x1005c8848u, 22u, "cSC4WinAdviceList::ScrollToAnchor(cIGZString const&)"},
    {0x1005c8870u, 30u, "cSC4WinAdviceList::DisableVerticalScrollbar()"},
    {0x1005c8a1cu, 464u, "cSC4WinAdviceList::cAdviceEntry::cAdviceEntry(cSC4WinAdviceList::cAdviceEntry const&)"},
    // cSC4SubwayOccupant (8)
    {0x1001a15a8u, 126u, "cSC4SubwayOccupant::cSC4SubwayOccupant()"},
    {0x1001a1704u, 60u, "cSC4SubwayOccupant::Init()"},
    {0x1001a1740u, 306u, "cSC4SubwayOccupant::StaticInit()"},
    {0x1001a1884u, 72u, "cSC4SubwayOccupant::Init(float const (*) [3])"},
    {0x1001a1b28u, 48u, "cSC4SubwayOccupant::GetSortValue(unsigned int&) const"},
    {0x1001a1b90u, 540u, "cSC4SubwayOccupant::GetPathInfo()"},
    {0x1001a1dacu, 154u, "cSC4SubwayOccupant::GetAltitude(float, float) const"},
    {0x1001a1e46u, 880u, "cSC4SubwayOccupant::InitPathBottomQuad()"},
    // cSC4EstablishCityWinProc (8)
    {0x100376430u, 136u, "cSC4EstablishCityWinProc::cSC4EstablishCityWinProc(cIGZWinGen*)"},
    {0x1003764b8u, 132u, "cSC4EstablishCityWinProc::InitDialog()"},
    {0x100376626u, 380u, "cSC4EstablishCityWinProc::GZOnCharacter(char)"},
    {0x1003767a2u, 616u, "cSC4EstablishCityWinProc::ValidateCityAndMayorNames()"},
    {0x100376a0au, 240u, "cSC4EstablishCityWinProc::EndDialog(bool)"},
    {0x100376afau, 254u, "cSC4EstablishCityWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x100376bf8u, 280u, "cSC4EstablishCityWinProc::DoCommand(cGZMessage&)"},
    {0x100376d10u, 28u, "cSC4EstablishCityWinProc::Shutdown()"},
    // cSC4RiotPolice (8)
    {0x100502700u, 236u, "cSC4RiotPolice::cSC4RiotPolice(cSC4PatrolCar*)"},
    {0x1005027f6u, 608u, "cSC4RiotPolice::Init()"},
    {0x100502a56u, 248u, "cSC4RiotPolice::PostMessageA(unsigned int, cISC4Occupant*, unsigned long, unsigned long, unsigned long, unsigned long)"},
    {0x100502b5cu, 78u, "cSC4RiotPolice::Shutdown()"},
    {0x100502bb8u, 162u, "cSC4RiotPolice::ClampToCityBounds(cS3DVector3&)"},
    {0x100502c5au, 1348u, "cSC4RiotPolice::SetupRiotPolice()"},
    {0x1005031acu, 294u, "cSC4RiotPolice::DoAnimationTick(unsigned int, float)"},
    {0x1005032d2u, 118u, "cSC4RiotPolice::SetZoomAndRotation(int, int)"},
    // cSC4MySimInputControlMoveIn (8)
    {0x1006b8a1cu, 68u, "cSC4MySimInputControlMoveIn::cSC4MySimInputControlMoveIn()"},
    {0x1006b8b92u, 34u, "cSC4MySimInputControlMoveIn::Init()"},
    {0x1006b8bb4u, 1112u, "cSC4MySimInputControlMoveIn::Init(cSC4MySim*)"},
    {0x1006b900cu, 356u, "cSC4MySimInputControlMoveIn::Shutdown()"},
    {0x1006b9178u, 2160u, "cSC4MySimInputControlMoveIn::OnMouseMove(int, int, unsigned int)"},
    {0x1006b99e8u, 76u, "cSC4MySimInputControlMoveIn::OnMouseDownL(int, int, unsigned int)"},
    {0x1006b9a34u, 26u, "cSC4MySimInputControlMoveIn::OnKeyDown(unsigned int, unsigned int)"},
    {0x1006b9a4eu, 66u, "cSC4MySimInputControlMoveIn::OnMouseExit()"},
    // cSC4NewRegionDialog (7)
    {0x1003d6ecau, 805u, "cSC4NewRegionDialog::GZOnAttach()"},
    {0x1003d7236u, 118u, "cSC4NewRegionDialog::NameConflicts(cIGZString const&)"},
    {0x1003d72acu, 332u, "cSC4NewRegionDialog::EnableControls()"},
    {0x1003d73f8u, 26u, "cSC4NewRegionDialog::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x1003d7412u, 174u, "cSC4NewRegionDialog::GZOnButtonActivate(unsigned int)"},
    {0x1003d74c0u, 52u, "cSC4NewRegionDialog::GZOnCancel()"},
    {0x1003d74f4u, 667u, "cSC4NewRegionDialog::GZOnOK()"},
    // cSC4AdvisorBase (7)
    {0x100444c9cu, 94u, "cSC4AdvisorBase::cSC4AdvisorBase()"},
    {0x100444d3cu, 164u, "cSC4AdvisorBase::Init(int)"},
    {0x100444deeu, 134u, "cSC4AdvisorBase::Shutdown()"},
    {0x100444e86u, 804u, "cSC4AdvisorBase::DoMessage(cIGZMessage2*)"},
    {0x1004451aau, 180u, "cSC4AdvisorBase::post_self_update(unsigned int, unsigned int)"},
    {0x100445276u, 52u, "cSC4AdvisorBase::simulate_day()"},
    {0x1004452aau, 14u, "cSC4AdvisorBase::simulate_month()"},
    // cSC4TrainCar (7)
    {0x100509470u, 268u, "cSC4TrainCar::cSC4TrainCar(cSC4VehicleManager*, cGZPersistResourceKey)"},
    {0x1005095c6u, 174u, "cSC4TrainCar::Init()"},
    {0x10050969cu, 86u, "cSC4TrainCar::DoCollideTick(float)"},
    {0x1005096fau, 150u, "cSC4TrainCar::SetPath(cISC4NetworkOccupant*, unsigned int, std::__1::vector<cS3DVector3, std::__1::allocator<cS3DVector3>> const*, bool)"},
    {0x100509a28u, 56u, "cSC4TrainCar::GetCollidingAutomaton(float)"},
    {0x100509a8au, 82u, "cSC4TrainCar::HandleCollision(cISC4Automaton*, cS3DVector3 const&)"},
    {0x100509adcu, 250u, "cSC4TrainCar::HandleEnterState(tAutomataState*)"},
    // cSC4DisasterManagerToxic (7)
    {0x1006fc4c0u, 114u, "cSC4DisasterManagerToxic::GetToxicCountInCellRect(SC4Rect<int>&)"},
    {0x1006fc532u, 124u, "cSC4DisasterManagerToxic::GetInstance()"},
    {0x1006fc64au, 82u, "cSC4DisasterManagerToxic::GetToxicCount()"},
    {0x1006fc6bau, 118u, "cSC4DisasterManagerToxic::AttemptDisasterStart(int, int)"},
    {0x1006fc88au, 82u, "cSC4DisasterManagerToxic::Init()"},
    {0x1006fc8eau, 70u, "cSC4DisasterManagerToxic::Shutdown()"},
    {0x1006fc978u, 252u, "cSC4DisasterManagerToxic::OnStandardMessage(cIGZMessage2Standard*)"},
    // cSC4StringSpecificationOccupantFilter (6)
    {0x1000a716au, 294u, "cSC4StringSpecificationOccupantFilter::cSC4StringSpecificationOccupantFilter()"},
    {0x1000a7290u, 158u, "cSC4StringSpecificationOccupantFilter::BuildSpecification(cIGZString const&)"},
    {0x1000a732eu, 222u, "cSC4StringSpecificationOccupantFilter::cSC4StringSpecificationOccupantFilter(std::__1::vector<cRZString, std::__1::allocator<cRZString>> const&)"},
    {0x1000a740cu, 5650u, "cSC4StringSpecificationOccupantFilter::BuildSpecification(std::__1::vector<cRZString, std::__1::allocator<cRZString>> const&)"},
    {0x1000a8a34u, 54u, "cSC4StringSpecificationOccupantFilter::GetOccupantsByBBox(ilist<cISC4Occupant*>&, unsigned int)"},
    {0x1000a8a6au, 434u, "cSC4StringSpecificationOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cSC4PowerLineOccupant (6)
    {0x100324412u, 16u, "cSC4PowerLineOccupant::operator new(unsigned long)"},
    {0x100324422u, 152u, "cSC4PowerLineOccupant::cSC4PowerLineOccupant()"},
    {0x100324578u, 40u, "cSC4PowerLineOccupant::Shutdown()"},
    {0x1003247beu, 98u, "cSC4PowerLineOccupant::GetConnectedPoles(cISC4PowerPoleOccupant*&, cISC4PowerPoleOccupant*&, bool&)"},
    {0x100324820u, 122u, "cSC4PowerLineOccupant::SetConnectedPoles(cISC4PowerPoleOccupant*, cISC4PowerPoleOccupant*, bool)"},
    {0x1003248dau, 60u, "cSC4PowerLineOccupant::SetPosition(float const*)"},
    // cSC4FeatureManager (6)
    {0x1006bf86eu, 438u, "cSC4FeatureManager::StaticInit()"},
    {0x1006bfa24u, 90u, "cSC4FeatureManager::StaticShutdown(void*)"},
    {0x1006bfa7eu, 398u, "cSC4FeatureManager::EnableFeatures(cRZString const&, cRZString const&, void*)"},
    {0x1006bfc0cu, 24u, "cSC4FeatureManager::Init()"},
    {0x1006bfc36u, 86u, "cSC4FeatureManager::IsFeatureEnabled(unsigned int) const"},
    {0x1006bfca0u, 48u, "cSC4FeatureManager::GetCurrentExpansionPack() const"},
    // cSC4TiledScrollingCanvas (6)
    {0x10075a6beu, 324u, "cSC4TiledScrollingCanvas::Init(int, int, int, int, cISC4TiledScrollingCanvasSource*)"},
    {0x10075a874u, 134u, "cSC4TiledScrollingCanvas::Shutdown()"},
    {0x10075a8fau, 676u, "cSC4TiledScrollingCanvas::ScrollTo(int, int)"},
    {0x10075ab9eu, 286u, "cSC4TiledScrollingCanvas::InvalidateRect(cRZRect const&)"},
    {0x10075acbcu, 382u, "cSC4TiledScrollingCanvas::Render(cIGZBuffer*, int, int)"},
    {0x10075ae3au, 396u, "cSC4TiledScrollingCanvas::Update(SC4DrawContext*)"},
    // cSC4UndergroundCustomView (5)
    {0x1001a96f0u, 142u, "cSC4UndergroundCustomView::InitCustomView()"},
    {0x1001a977eu, 402u, "cSC4UndergroundCustomView::SetRoadsTranslucent(int, int)"},
    {0x1001a9922u, 126u, "cSC4UndergroundCustomView::ShutdownCustomView()"},
    {0x1001a99b2u, 56u, "cSC4UndergroundCustomView::DoMessage(cIGZMessage2*)"},
    {0x1001a99eau, 178u, "cSC4UndergroundCustomView::DoOccupantInserted(cISC4Occupant*)"},
    // cSC4ImportCityDialog (5)
    {0x1003d79f0u, 1297u, "cSC4ImportCityDialog::GZOnAttach()"},
    {0x1003d7f48u, 682u, "cSC4ImportCityDialog::AddCitiesToList(cIGZWinTreeView*, ilist<cRZAutoRefCount<cISC4RegionalCity>>&, cIGZString const&, int, bool)"},
    {0x1003d81f2u, 138u, "cSC4ImportCityDialog::GZOnDetach()"},
    {0x1003d8310u, 50u, "cSC4ImportCityDialog::GZOnCancel()"},
    {0x1003d8342u, 208u, "cSC4ImportCityDialog::GZOnOK()"},
    // cSC4AdvisorTraffic (5)
    {0x100448096u, 1530u, "cSC4AdvisorTraffic::update_occupant_variables()"},
    {0x100448690u, 4544u, "cSC4AdvisorTraffic::update_variables()"},
    {0x100449850u, 528u, "cSC4AdvisorTraffic::DoMessage(cIGZMessage2*)"},
    {0x100449a6eu, 76u, "cSC4AdvisorTraffic::Init(int)"},
    {0x100449ac8u, 64u, "cSC4AdvisorTraffic::Shutdown()"},
    // cSC4Flame (5)
    {0x10045e7ccu, 116u, "cSC4Flame::cSC4Flame()"},
    {0x10045e840u, 236u, "cSC4Flame::Update(unsigned int)"},
    {0x10045e92cu, 244u, "cSC4Flame::UpdateTransform()"},
    {0x10045ea20u, 78u, "cSC4Flame::Shutdown()"},
    {0x10045ea6eu, 58u, "cSC4Flame::SetVisible(bool)"},
    // cSC4OccupantGroupOccupantFilter (4)
    {0x1000a8cf8u, 166u, "cSC4OccupantGroupOccupantFilter::cSC4OccupantGroupOccupantFilter(unsigned int)"},
    {0x1000a8d9eu, 164u, "cSC4OccupantGroupOccupantFilter::cSC4OccupantGroupOccupantFilter(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>> const&)"},
    {0x1000a8e4cu, 72u, "cSC4OccupantGroupOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    {0x1000a8e94u, 54u, "cSC4OccupantGroupOccupantFilter::AddGroupId(unsigned int)"},
    // cSC4NetworkTunnelOccupant (4)
    {0x1001a22deu, 160u, "cSC4NetworkTunnelOccupant::cSC4NetworkTunnelOccupant()"},
    {0x1001a2692u, 62u, "cSC4NetworkTunnelOccupant::Init()"},
    {0x1001a26deu, 76u, "cSC4NetworkTunnelOccupant::Shutdown()"},
    {0x1001a2738u, 84u, "cSC4NetworkTunnelOccupant::SetOtherEndOccupant(cISC4NetworkOccupant*)"},
    // cSC4AttractorEffect (4)
    {0x100575204u, 144u, "cSC4AttractorEffect::cSC4AttractorEffect(cSC4AttractorDescription*)"},
    {0x10057539cu, 888u, "cSC4AttractorEffect::Start(cISC4VisualEffect::tTransitionType)"},
    {0x100575714u, 478u, "cSC4AttractorEffect::Stop(cISC4VisualEffect::tTransitionType)"},
    {0x1005758fcu, 146u, "cSC4AttractorEffect::SetTransforms(cS3DTransform const&, cS3DTransform const&)"},
    // cSC4AutoSaveRecord (3)
    {0x1000428eau, 228u, "cSC4AutoSaveRecord::openRecord(cIGZPersistDBSegment*, unsigned int, unsigned int, unsigned int, unsigned short)"},
    {0x1000429f2u, 204u, "cSC4AutoSaveRecord::cSC4AutoSaveRecord(cIGZOStream&, unsigned int, unsigned int, unsigned int, unsigned short)"},
    {0x100042b44u, 108u, "cSC4AutoSaveRecord::Close()"},
    // cSC4NetworkTileNeighborhood (3)
    {0x10019048cu, 106u, "cSC4NetworkTileNeighborhood::ComputeCompactRepresentation(unsigned char, cSC4TileNeighborhoodCompact&)"},
    {0x1001904f6u, 164u, "cSC4NetworkTileNeighborhood::cSC4NetworkTileNeighborhood(int, int, cSC4NetworkTool&, cISC4NetworkManager::eNetworkType, bool)"},
    {0x1001905a8u, 92u, "cSC4NetworkTileNeighborhood::IsValidSolutionPos(unsigned char)"},
    // cSC4MissingPluginPackOccupantFilter (3)
    {0x10028c5eau, 66u, "cSC4MissingPluginPackOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    {0x10028c62cu, 190u, "cSC4MissingPluginPackOccupantFilter::AreModelsAvailable(cISC4Occupant*)"},
    {0x10028c6eau, 276u, "cSC4MissingPluginPackOccupantFilter::AreModelsAvailableForZoomAndRotation(cISC4Occupant*, int, int, unsigned int, int, bool)"},
    // cSC4LoadRegionDialog (3)
    {0x1003d8a66u, 508u, "cSC4LoadRegionDialog::GZOnAttach()"},
    {0x1003d8c62u, 52u, "cSC4LoadRegionDialog::GZOnCancel()"},
    {0x1003d8c96u, 122u, "cSC4LoadRegionDialog::GZOnOK()"},
    // cSC4Bus (3)
    {0x100506b34u, 132u, "cSC4Bus::cSC4Bus()"},
    {0x100506be8u, 204u, "cSC4Bus::HandleExitNetwork(cISC4NetworkOccupant*, unsigned int, cISC4Lot*, SC4Rect<int> const&)"},
    {0x100506cb4u, 568u, "cSC4Bus::DoStopAtStationTick(float)"},
    // cSC4NetworkLayoutView (3)
    {0x100712a84u, 406u, "cSC4NetworkLayoutView::Init()"},
    {0x100712c1au, 234u, "cSC4NetworkLayoutView::Update(SC4DrawContext*)"},
    {0x100712d04u, 4602u, "cSC4NetworkLayoutView::RebuildMap()"},
    // cSC4CompressedRouteReader (2)
    {0x1000d4ef6u, 92u, "cSC4CompressedRouteReader::SetData(std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>*)"},
    {0x1000d4f52u, 456u, "cSC4CompressedRouteReader::GetNext(cSC4CompressedRouteReader::UncompressedData&)"},
    // cSC4InsideLotOccupantFilter (2)
    {0x100181938u, 112u, "cSC4InsideLotOccupantFilter::SetLot(cISC4Lot const*)"},
    {0x100183ddau, 162u, "cSC4InsideLotOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cSC4MissingPluginPacksDialog (2)
    {0x10028c2d4u, 588u, "cSC4MissingPluginPacksDialog::DoDialog(std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>> const&)"},
    {0x10028c56cu, 126u, "cSC4MissingPluginPacksDialog::DoMessage(cIGZWin*, cGZMessage&)"},
    // cSC4PipeConnectionOccupant (2)
    {0x1003dde60u, 126u, "cSC4PipeConnectionOccupant::cSC4PipeConnectionOccupant()"},
    {0x1003de09cu, 138u, "cSC4PipeConnectionOccupant::QueryOccupantInterface(cISC4Occupant*, cSC4UndergroundOccupant**)"},
    // cAmbienceOccupantFilterIsIndustrialClean (2)
    {0x1003df204u, 50u, "cAmbienceOccupantFilterIsIndustrialClean::Create()"},
    {0x1003e503au, 332u, "cAmbienceOccupantFilterIsIndustrialClean::IsTrueOccupant(cISC4Occupant*)"},
    // cAmbienceOccupantFilterIsBuilding (2)
    {0x1003df2ccu, 50u, "cAmbienceOccupantFilterIsBuilding::Create()"},
    {0x1003e578au, 132u, "cAmbienceOccupantFilterIsBuilding::IsTrueOccupant(cISC4Occupant*)"},
    // cAmbienceOccupantFilterIsCivillianVehicle (2)
    {0x1003df394u, 50u, "cAmbienceOccupantFilterIsCivillianVehicle::Create()"},
    {0x1003e5382u, 76u, "cAmbienceOccupantFilterIsCivillianVehicle::IsTrueOccupant(cISC4Occupant*)"},
    // cAmbienceOccupantFilterIsSoccerMom (2)
    {0x1003df45cu, 50u, "cAmbienceOccupantFilterIsSoccerMom::Create()"},
    {0x1003e54b2u, 76u, "cAmbienceOccupantFilterIsSoccerMom::IsTrueOccupant(cISC4Occupant*)"},
    // cAmbienceOccupantFilterIsMovingVan (2)
    {0x1003df524u, 50u, "cAmbienceOccupantFilterIsMovingVan::Create()"},
    {0x1003e55fcu, 76u, "cAmbienceOccupantFilterIsMovingVan::IsTrueOccupant(cISC4Occupant*)"},
    // cAmbienceOccupantFilterIsCommercial (2)
    {0x1003df5ecu, 50u, "cAmbienceOccupantFilterIsCommercial::Create()"},
    {0x1003e4d9cu, 332u, "cAmbienceOccupantFilterIsCommercial::IsTrueOccupant(cISC4Occupant*)"},
    // cAmbienceOccupantFilterIsConstructionVehicle (2)
    {0x1003df6b4u, 50u, "cAmbienceOccupantFilterIsConstructionVehicle::Create()"},
    {0x1003e52d0u, 102u, "cAmbienceOccupantFilterIsConstructionVehicle::IsTrueOccupant(cISC4Occupant*)"},
    // cAmbienceOccupantFilterIsPowered (2)
    {0x1003df77cu, 50u, "cAmbienceOccupantFilterIsPowered::Create()"},
    {0x1003e5b90u, 162u, "cAmbienceOccupantFilterIsPowered::IsTrueOccupant(cISC4Occupant*)"},
    // cSC4NetworkLotConfiguration (2)
    {0x10040932au, 292u, "cSC4NetworkLotConfiguration::cSC4NetworkLotConfiguration()"},
    {0x100409552u, 24u, "cSC4NetworkLotConfiguration::GetName(cIGZString&) const"},
    // cSC4GetawayVan (2)
    {0x100507244u, 132u, "cSC4GetawayVan::cSC4GetawayVan()"},
    {0x1005072c8u, 390u, "cSC4GetawayVan::PostInit()"},
    // cSC4LimitTimerCalibrator (2)
    {0x10070e58eu, 86u, "cSC4LimitTimerCalibrator::cSC4LimitTimerCalibrator()"},
    {0x10070e5e4u, 124u, "cSC4LimitTimerCalibrator::Tick()"},
    // cSC4ConfigRule (2)
    {0x10076d040u, 70u, "cSC4ConfigRule::cSC4ConfigRule()"},
    {0x10076d086u, 334u, "cSC4ConfigRule::cSC4ConfigRule(cSC4ConfigRule const&)"},
    // cSC4RealisticOccupantFilter (1)
    {0x1000a68f6u, 502u, "cSC4RealisticOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cSC4FloraBlockingOccupantFilter (1)
    {0x1000c254au, 188u, "cSC4FloraBlockingOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cSC4AutomataTuningParams (1)
    {0x1000fd1d8u, 1092u, "cSC4AutomataTuningParams::cSC4AutomataTuningParams()"},
    // cSC4WealthRequester (1)
    {0x1001262ceu, 378u, "cSC4WealthRequester::SelectProp(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>> const&, int, int) const"},
    // cSC4PropOccupantFilter (1)
    {0x100196138u, 272u, "cSC4PropOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cSC4AutomataFrequencyMarkedCellView (1)
    {0x1001ea140u, 64u, "cSC4AutomataFrequencyMarkedCellView::Shutdown()"},
    // tCrimeSimOccupantFilter (1)
    {0x1001fff82u, 60u, "tCrimeSimOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cSC4QueriableOccupantFilter (1)
    {0x1002dbe6eu, 62u, "cSC4QueriableOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cSC4LineGraph (1)
    {0x1002e2cf6u, 42u, "cSC4LineGraph::DrawLine(cIGZBuffer*, int, int, int, int, int, unsigned int)"},
    // cSC4VisibleAnimatableOccupantFilter (1)
    {0x100359d7cu, 94u, "cSC4VisibleAnimatableOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cSC4DemolishableOccupantFilter (1)
    {0x1003cd272u, 342u, "cSC4DemolishableOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cSC4DisasterManagerEarthquake (1)
    {0x100452bfcu, 168u, "cSC4DisasterManagerEarthquake::AttemptDisasterStart(int, int)"},
    // cSC4DummyOccupant (1)
    {0x100575a4au, 54u, "cSC4DummyOccupant::GetBoundingBox(float*, float*)"},
    // cSC4GDriversCOMDirector (1)
    {0x1006bf71eu, 102u, "cSC4GDriversCOMDirector::cSC4GDriversCOMDirector()"},
    // cSC4SignpostFilter (1)
    {0x1006cb30eu, 116u, "cSC4SignpostFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cTrafficNetworkFilter_PlayerDrive (1)
    {0x1006fbc84u, 86u, "cTrafficNetworkFilter_PlayerDrive::IsOccupantIncluded(cISC4Occupant*)"},
    // cSC4MySimAdvice (1)
    {0x10079eb12u, 102u, "cSC4MySimAdvice::DoLink(cIGZString const&)"},
}};

[[nodiscard]] inline constexpr Uint32 Wave3Part4MethodCount() noexcept {
    return static_cast<Uint32>(kWave3Part4Methods.size());
}

[[nodiscard]] inline constexpr const Wave3Part4Method* FindWave3Part4Method(Uint64 address) noexcept {
    for (const Wave3Part4Method& method : kWave3Part4Methods) {
        if (method.address == address) {
            return &method;
        }
    }
    return nullptr;
}

static_assert(kWave3Part4Methods.size() == 452);
static_assert(Wave3Part4MethodCount() == 452u);
static_assert(FindWave3Part4Method(0x100330e40u) != nullptr);
static_assert(FindWave3Part4Method(0x10079eb12u) != nullptr);
static_assert(FindWave3Part4Method(0x100330e40u)->sizeBytes == 3980u);
static_assert(FindWave3Part4Method(0x10079eb12u)->name[0] != '\0');

// ---------------------------------------------------------------------------
// Wave 4 Part 4 method index
// 86 classes, 808 macOS x86_64 entry points (0x100xxxxxx) from the
// consolidated workpack. Addresses are image VAs; sizeBytes is the
// decoded function extent.
// ---------------------------------------------------------------------------

struct Wave4Part4Method {
    Uint64 address;
    Uint32 sizeBytes;
    const char* name;
};

inline constexpr std::array<Wave4Part4Method, 808> kWave4Part4Methods = {{
    // cSC43DRender (89)
    {0x10075f3e6u, 142u, "cSC43DRender::InsertIntoViewObjectList(std::__1::list<cSC43DRender::ViewObjectInfo, std::__1::allocator<cSC43DRender::ViewObjectInfo>>&, cISC4ViewObject3D*, unsigned int)"},
    {0x10075f474u, 90u, "cSC43DRender::RemoveFromViewObjectList(std::__1::list<cSC43DRender::ViewObjectInfo, std::__1::allocator<cSC43DRender::ViewObjectInfo>>&, cISC4ViewObject3D*)"},
    {0x10075f4ceu, 48u, "cSC43DRender::ClearViewObjectList(std::__1::list<cSC43DRender::ViewObjectInfo, std::__1::allocator<cSC43DRender::ViewObjectInfo>>&)"},
    {0x10075f4feu, 1518u, "cSC43DRender::cSC43DRender()"},
    {0x10075ff58u, 52u, "cSC43DRender::StaticInit(void*)"},
    {0x10075ff8cu, 158u, "cSC43DRender::StaticShutdown(void*)"},
    {0x10076002au, 1176u, "cSC43DRender::Init()"},
    {0x1007604c2u, 132u, "cSC43DRender::CreateQuadGrid()"},
    {0x100760546u, 2166u, "cSC43DRender::ReadProperties()"},
    {0x100760dbcu, 894u, "cSC43DRender::Shutdown()"},
    {0x10076113au, 168u, "cSC43DRender::ClearModelChangeQueue()"},
    {0x100761392u, 170u, "cSC43DRender::SetGraphicSystem(cIGZGraphicSystem2*)"},
    {0x10076143cu, 138u, "cSC43DRender::SetGDriver(cIGZGDriver*)"},
    {0x1007614c6u, 240u, "cSC43DRender::StaticSetGDriver(cIGZGDriver*)"},
    {0x10076160eu, 208u, "cSC43DRender::SetClipRect(cRZRect const&)"},
    {0x1007616deu, 66u, "cSC43DRender::UnsetClipRect()"},
    {0x100761720u, 144u, "cSC43DRender::SetCamera(cS3DCamera*)"},
    {0x1007617b0u, 94u, "cSC43DRender::SetCameraControl(cSC4CameraControl*)"},
    {0x10076180eu, 136u, "cSC43DRender::IsModelInserted(cIS3DModelInstance*)"},
    {0x100761896u, 1474u, "cSC43DRender::ProcessModelChangeQueue()"},
    {0x100761e58u, 620u, "cSC43DRender::InsertModel(cIS3DModelInstance*)"},
    {0x1007620c4u, 916u, "cSC43DRender::WorldToContainerRect(float const*, float const*, SC4Rect<float>&, int)"},
    {0x100762458u, 224u, "cSC43DRender::UpdateGridElementObjectFlags(cIS3DModelInstance*, SC43DGridElement*)"},
    {0x100762538u, 136u, "cSC43DRender::ModelShouldGoIntoBackingStore(SC43DGridElement const*)"},
    {0x1007625c0u, 104u, "cSC43DRender::DoFullViewUpdate(cISC43DRender::ViewUpdateReason)"},
    {0x100762628u, 372u, "cSC43DRender::RemoveModel(cIS3DModelInstance*)"},
    {0x10076279cu, 314u, "cSC43DRender::MoveModel(cIS3DModelInstance*)"},
    {0x1007628d6u, 146u, "cSC43DRender::MoveGridElement(SC43DGridElement*, cIS3DModelInstance*)"},
    {0x100762968u, 462u, "cSC43DRender::ChangeModel(cIS3DModelInstance*, int)"},
    {0x100762b36u, 1428u, "cSC43DRender::DoMessage(cIGZMessage2*)"},
    {0x1007630cau, 1940u, "cSC43DRender::UpdateVisibleTerrainMetrics()"},
    {0x10076385eu, 216u, "cSC43DRender::DoViewRotationChange2(int, int, int, int)"},
    {0x100763936u, 270u, "cSC43DRender::DoViewZoomChange3(int, int, int, int)"},
    {0x100763a56u, 112u, "cSC43DRender::ClearDirtyRectangleQueue()"},
    {0x100763ac6u, 1436u, "cSC43DRender::DoTranslatedViewUpdate(int, int)"},
    {0x100764062u, 52u, "cSC43DRender::TurnOnLighting(bool)"},
    {0x100764096u, 286u, "cSC43DRender::UpdateViewportRect(cRZRect const&)"},
    {0x1007641b4u, 140u, "cSC43DRender::SaveBackingStore()"},
    {0x100764240u, 2092u, "cSC43DRender::DrawDynamicView()"},
    {0x100764a6cu, 128u, "cSC43DRender::DoPartialViewUpdate(cRZRect const&, cISC43DRender::ViewUpdateReason)"},
    {0x100764aecu, 360u, "cSC43DRender::SaveBackingStore(int, int, int, int)"},
    {0x100764c54u, 1148u, "cSC43DRender::DrawStaticView()"},
    {0x1007650d0u, 196u, "cSC43DRender::DoSingleModelStaticDrawAddition(cIS3DModelInstance*, cRZRect const&)"},
    {0x100765194u, 266u, "cSC43DRender::GetVisibleContainerRect(SC4Rect<float>&, cISC43DRender::FrustumType)"},
    {0x100765592u, 564u, "cSC43DRender::RebuildQuadGrid(int, int, int, int)"},
    {0x1007657c6u, 232u, "cSC43DRender::ReevaluateBackingStoreObjectFlags()"},
    {0x1007658beu, 312u, "cSC43DRender::AttemptTranslatedViewUpdate(int, int)"},
    {0x1007659f6u, 1638u, "cSC43DRender::Draw()"},
    {0x10076605cu, 146u, "cSC43DRender::RestoreBackingStore()"},
    {0x1007660eeu, 204u, "cSC43DRender::DoViewCleanup()"},
    {0x1007661bau, 494u, "cSC43DRender::ProcessDirtyRectangleQueue()"},
    {0x1007663a8u, 1658u, "cSC43DRender::SetShakeOffsets()"},
    {0x100766a22u, 72u, "cSC43DRender::Set3DCameraView()"},
    {0x100766a6au, 54u, "cSC43DRender::Set2DCameraView()"},
    {0x100766aa0u, 130u, "cSC43DRender::SetUICameraView()"},
    {0x100766b22u, 270u, "cSC43DRender::GetViewFrustumContainerRect(SC4Rect<float>&)"},
    {0x100766c30u, 302u, "cSC43DRender::GetDrawFrustumContainerRect(SC4Rect<float>&)"},
    {0x100766d5eu, 2488u, "cSC43DRender::ClearViewport(bool)"},
    {0x100767716u, 196u, "cSC43DRender::ClearViewportWithSolidColor(float const*, bool, bool)"},
    {0x1007677dau, 474u, "cSC43DRender::DrawViewportRectangle(cRZRect const&, float const*, bool, bool)"},
    {0x1007679b4u, 350u, "cSC43DRender::GetViewportRayContainerRect(int, int, SC4Rect<float>&)"},
    {0x100767b12u, 532u, "cSC43DRender::PickTerrain(float const*, float const*, float*, bool, bool)"},
    {0x100767d26u, 260u, "cSC43DRender::PickTerrain(int, int, float*, bool, bool)"},
    {0x100767e2au, 1666u, "cSC43DRender::PickTerrainAlways(float const*, float const*, float*, bool)"},
    {0x1007687deu, 248u, "cSC43DRender::PickTerrainAlways(int, int, float*, bool)"},
    {0x1007688d6u, 156u, "cSC43DRender::Pick(int, int, cISC4OccupantFilter*, cIS3DModelInstance*&)"},
    {0x100768972u, 1002u, "cSC43DRender::Pick(int, int, float const*, float const*, cISC4OccupantFilter*, cIS3DModelInstance*&)"},
    {0x100768d64u, 102u, "cSC43DRender::IsModelVisible(cIS3DModelInstance*, cISC43DRender::FrustumType)"},
    {0x100768dcau, 100u, "cSC43DRender::IsModelVisible(SC43DGridElement*, cISC43DRender::FrustumType)"},
    {0x100768eb0u, 436u, "cSC43DRender::StartScreenShake(float, float)"},
    {0x100769064u, 346u, "cSC43DRender::StartScreenShake(cSC4ShakeDescription const&, cS3DVector2 const*)"},
    {0x10076929au, 140u, "cSC43DRender::StopScreenShake(unsigned int)"},
    {0x1007693d0u, 128u, "cSC43DRender::AddViewObject(cISC4ViewObject3D*, int, unsigned int)"},
    {0x100769450u, 184u, "cSC43DRender::RemoveViewObject(cISC4ViewObject3D*)"},
    {0x100769508u, 154u, "cSC43DRender::FindViewObject(cISC4ViewObject3D*) const"},
    {0x1007695a2u, 374u, "cSC43DRender::DrawTexturedViewportRectangle(cRZRect const&, unsigned int, float, bool, bool)"},
    {0x100769718u, 72u, "cSC43DRender::TakeSnapshot(cIGZBuffer**, bool)"},
    {0x100769760u, 150u, "cSC43DRender::TakeSnapshot(cIGZString&, bool)"},
    {0x10076981eu, 236u, "cSC43DRender::SetBackgroundPicture(cIGZBuffer*, bool)"},
    {0x10076990au, 140u, "cSC43DRender::SetBackgroundPicture(unsigned int, int, int, bool)"},
    {0x1007699eeu, 302u, "cSC43DRender::CreateBackingStore()"},
    {0x100769b1cu, 178u, "cSC43DRender::RestoreBackingStore(int, int, int, int, int, int)"},
    {0x100769bdau, 40u, "cSC43DRender::SetViewFogEnabled(bool)"},
    {0x100769c0eu, 54u, "cSC43DRender::SetLightingEnabled(bool)"},
    {0x100769c44u, 134u, "cSC43DRender::AddDirtyRectangle(int, cRZRect const&, cIS3DModelInstance*, int)"},
    {0x100769ccau, 812u, "cSC43DRender::AddDirtyRectangleInternal(int, cRZRect const&, cIS3DModelInstance*, int)"},
    {0x10076a006u, 126u, "cSC43DRender::ReturnRenderSystem()"},
    {0x10076a084u, 110u, "cSC43DRender::GetStatsString1(cIGZString&)"},
    {0x10076a0f2u, 132u, "cSC43DRender::GetStatsString2(cIGZString&)"},
    // cSC4RenderProperties (85)
    {0x10028c8ceu, 1352u, "cSC4RenderProperties::cSC4RenderProperties()"},
    {0x10028d196u, 18u, "cSC4RenderProperties::NumBoolProperties() const"},
    {0x10028d1a8u, 24u, "cSC4RenderProperties::NumIntProperties() const"},
    {0x10028d1c0u, 30u, "cSC4RenderProperties::NumFloatProperties() const"},
    {0x10028d25au, 568u, "cSC4RenderProperties::BoolPropertyIDFromName(char const*) const"},
    {0x10028d492u, 580u, "cSC4RenderProperties::IntPropertyIDFromName(char const*) const"},
    {0x10028d6d6u, 580u, "cSC4RenderProperties::FloatPropertyIDFromName(char const*) const"},
    {0x10028db18u, 708u, "cSC4RenderProperties::cProperties::cProperties()"},
    {0x10028dddcu, 290u, "cSC4RenderProperties::cProperties::cRegkForceSoftwareDriver::cRegkForceSoftwareDriver()"},
    {0x10028e1f6u, 290u, "cSC4RenderProperties::cProperties::cRegkRenderDynamicView::cRegkRenderDynamicView()"},
    {0x10028e318u, 290u, "cSC4RenderProperties::cProperties::cRegkRenderStaticView::cRegkRenderStaticView()"},
    {0x10028e43au, 290u, "cSC4RenderProperties::cProperties::cRegkRenderWaterEffects::cRegkRenderWaterEffects()"},
    {0x10028e55cu, 290u, "cSC4RenderProperties::cProperties::cRegkRenderCloudsAndFog::cRegkRenderCloudsAndFog()"},
    {0x10028e67eu, 290u, "cSC4RenderProperties::cProperties::cRegkForcePointSampling::cRegkForcePointSampling()"},
    {0x10028e7a0u, 290u, "cSC4RenderProperties::cProperties::cRegkForceBilinear::cRegkForceBilinear()"},
    {0x10028e8c2u, 290u, "cSC4RenderProperties::cProperties::cRegkForceNoAlphaBlend::cRegkForceNoAlphaBlend()"},
    {0x10028e9e4u, 290u, "cSC4RenderProperties::cProperties::cRegkForceNoAlphaTest::cRegkForceNoAlphaTest()"},
    {0x10028eb06u, 290u, "cSC4RenderProperties::cProperties::cRegkRenderShadows::cRegkRenderShadows()"},
    {0x10028ec28u, 290u, "cSC4RenderProperties::cProperties::cRegkRenderDecals::cRegkRenderDecals()"},
    {0x10028ed4au, 290u, "cSC4RenderProperties::cProperties::cRegkUseSecondStage::cRegkUseSecondStage()"},
    {0x10028ee6cu, 290u, "cSC4RenderProperties::cProperties::cRegkForceDebugTexture::cRegkForceDebugTexture()"},
    {0x10028ef8eu, 290u, "cSC4RenderProperties::cProperties::cRegkRenderParticleEffects::cRegkRenderParticleEffects()"},
    {0x10028f0b0u, 290u, "cSC4RenderProperties::cProperties::cRegkRenderFloraShadows::cRegkRenderFloraShadows()"},
    {0x10028f1d2u, 290u, "cSC4RenderProperties::cProperties::cRegkRenderTerrain::cRegkRenderTerrain()"},
    {0x10028f2f4u, 290u, "cSC4RenderProperties::cProperties::cRegkShowUpdateRegions::cRegkShowUpdateRegions()"},
    {0x10028f416u, 290u, "cSC4RenderProperties::cProperties::cRegkUseCursorEffects::cRegkUseCursorEffects()"},
    {0x10028f538u, 290u, "cSC4RenderProperties::cProperties::cRegkDrawLotTextures::cRegkDrawLotTextures()"},
    {0x10028f65au, 290u, "cSC4RenderProperties::cProperties::cRegkDrawLotFoundations::cRegkDrawLotFoundations()"},
    {0x10028f77cu, 290u, "cSC4RenderProperties::cProperties::cRegkDrawLotWalls::cRegkDrawLotWalls()"},
    {0x10028f89eu, 290u, "cSC4RenderProperties::cProperties::cRegkForceTextureRepeat::cRegkForceTextureRepeat()"},
    {0x10028f9c0u, 290u, "cSC4RenderProperties::cProperties::cRegkUseScreenShake::cRegkUseScreenShake()"},
    {0x10028fae2u, 290u, "cSC4RenderProperties::cProperties::cRegkRenderUI::cRegkRenderUI()"},
    {0x10028fc04u, 290u, "cSC4RenderProperties::cProperties::cRegkUseUpdateRectClipping::cRegkUseUpdateRectClipping()"},
    {0x10028fd26u, 290u, "cSC4RenderProperties::cProperties::cRegkUseVertexBuffers::cRegkUseVertexBuffers()"},
    {0x10028fe48u, 290u, "cSC4RenderProperties::cProperties::cRegkShowBoundingBoxes::cRegkShowBoundingBoxes()"},
    {0x10028ff6au, 290u, "cSC4RenderProperties::cProperties::cRegkUseOverlayCulling::cRegkUseOverlayCulling()"},
    {0x10029008cu, 290u, "cSC4RenderProperties::cProperties::cRegkRenderSnapshot::cRegkRenderSnapshot()"},
    {0x1002901aeu, 290u, "cSC4RenderProperties::cProperties::cRegkNoAddSigned::cRegkNoAddSigned()"},
    {0x1002902d0u, 290u, "cSC4RenderProperties::cProperties::cRegkNoPartialBackingStoreCopies::cRegkNoPartialBackingStoreCopies()"},
    {0x1002903f2u, 290u, "cSC4RenderProperties::cProperties::cRegkUseScreenFlash::cRegkUseScreenFlash()"},
    {0x100290514u, 290u, "cSC4RenderProperties::cProperties::cRegkRenderAutomata::cRegkRenderAutomata()"},
    {0x100290636u, 290u, "cSC4RenderProperties::cProperties::cRegkPreferOpaque::cRegkPreferOpaque()"},
    {0x100290758u, 290u, "cSC4RenderProperties::cProperties::cRegkDisplayAlertBorders::cRegkDisplayAlertBorders()"},
    {0x10029087au, 290u, "cSC4RenderProperties::cProperties::cRegkDrawLotOpaqueTextures::cRegkDrawLotOpaqueTextures()"},
    {0x10029099cu, 290u, "cSC4RenderProperties::cProperties::cRegkUseViewCleanup::cRegkUseViewCleanup()"},
    {0x100290abeu, 290u, "cSC4RenderProperties::cProperties::cRegkDoAutoZoomAndRotationChanges::cRegkDoAutoZoomAndRotationChanges()"},
    {0x100290be0u, 294u, "cSC4RenderProperties::cProperties::cRegkViewOffsetY::cRegkViewOffsetY()"},
    {0x100291014u, 294u, "cSC4RenderProperties::cProperties::cRegkViewOffsetX::cRegkViewOffsetX()"},
    {0x10029113au, 294u, "cSC4RenderProperties::cProperties::cRegkTextureBits::cRegkTextureBits()"},
    {0x100291260u, 294u, "cSC4RenderProperties::cProperties::cRegkLODOffset::cRegkLODOffset()"},
    {0x100291386u, 294u, "cSC4RenderProperties::cProperties::cRegkShadowDebug::cRegkShadowDebug()"},
    {0x1002914acu, 294u, "cSC4RenderProperties::cProperties::cRegkMaxParticlesTarget::cRegkMaxParticlesTarget()"},
    {0x1002915d2u, 294u, "cSC4RenderProperties::cProperties::cRegkParticleLODOffset::cRegkParticleLODOffset()"},
    {0x1002916f8u, 300u, "cSC4RenderProperties::cProperties::cRegkEffectPriorityLevel::cRegkEffectPriorityLevel()"},
    {0x100291824u, 294u, "cSC4RenderProperties::cProperties::cRegkBackingStoreMaxZoomAll::cRegkBackingStoreMaxZoomAll()"},
    {0x10029194au, 294u, "cSC4RenderProperties::cProperties::cRegkBackingStoreMaxZoomBuildingsProps::cRegkBackingStoreMaxZoomBuildingsProps()"},
    {0x100291a70u, 294u, "cSC4RenderProperties::cProperties::cRegkBackingStoreMaxZoomTerrain::cRegkBackingStoreMaxZoomTerrain()"},
    {0x100291b96u, 294u, "cSC4RenderProperties::cProperties::cRegkBackingStoreMaxZoomFlora::cRegkBackingStoreMaxZoomFlora()"},
    {0x100291cbcu, 294u, "cSC4RenderProperties::cProperties::cRegkShadowQuality::cRegkShadowQuality()"},
    {0x100291de2u, 294u, "cSC4RenderProperties::cProperties::cRegkMinVehicleZoom::cRegkMinVehicleZoom()"},
    {0x100291f08u, 294u, "cSC4RenderProperties::cProperties::cRegkMinPedZoom::cRegkMinPedZoom()"},
    {0x10029202eu, 294u, "cSC4RenderProperties::cProperties::cRegkCursorType::cRegkCursorType()"},
    {0x100292154u, 294u, "cSC4RenderProperties::cProperties::cRegkDepthOffset::cRegkDepthOffset()"},
    {0x10029227au, 294u, "cSC4RenderProperties::cProperties::cRegkDebugTexture::cRegkDebugTexture()"},
    {0x1002923a0u, 294u, "cSC4RenderProperties::cProperties::cRegkDirtyRectMergeFrames::cRegkDirtyRectMergeFrames()"},
    {0x1002924c6u, 294u, "cSC4RenderProperties::cProperties::cRegkSoundDetail::cRegkSoundDetail()"},
    {0x1002925ecu, 294u, "cSC4RenderProperties::cProperties::cRegkBuildingVariety::cRegkBuildingVariety()"},
    {0x100292712u, 294u, "cSC4RenderProperties::cProperties::cRegkBuildingLoaderSpeed::cRegkBuildingLoaderSpeed()"},
    {0x100292838u, 294u, "cSC4RenderProperties::cProperties::cRegkModelLODOffset::cRegkModelLODOffset()"},
    {0x10029295eu, 294u, "cSC4RenderProperties::cProperties::cRegkStringTextureCacheSize::cRegkStringTextureCacheSize()"},
    {0x100292a84u, 300u, "cSC4RenderProperties::cProperties::cRegkParticleEffectsLOD::cRegkParticleEffectsLOD()"},
    {0x100292ebeu, 300u, "cSC4RenderProperties::cProperties::cRegkSplatTextureScale::cRegkSplatTextureScale()"},
    {0x100292feau, 294u, "cSC4RenderProperties::cProperties::cRegkTexBindMaxFree::cRegkTexBindMaxFree()"},
    {0x100293110u, 294u, "cSC4RenderProperties::cProperties::cRegkParticleDensity::cRegkParticleDensity()"},
    {0x100293236u, 294u, "cSC4RenderProperties::cProperties::cRegkParticleDamping::cRegkParticleDamping()"},
    {0x10029335cu, 294u, "cSC4RenderProperties::cProperties::cRegkParticleScale::cRegkParticleScale()"},
    {0x100293482u, 294u, "cSC4RenderProperties::cProperties::cRegkLightingUpdateDelta::cRegkLightingUpdateDelta()"},
    {0x1002935a8u, 294u, "cSC4RenderProperties::cProperties::cRegkShadowModelThreshold::cRegkShadowModelThreshold()"},
    {0x1002936ceu, 294u, "cSC4RenderProperties::cProperties::cRegkDemolishModelThreshold::cRegkDemolishModelThreshold()"},
    {0x1002937f4u, 294u, "cSC4RenderProperties::cProperties::cRegkRenderModelThreshold::cRegkRenderModelThreshold()"},
    {0x10029391au, 294u, "cSC4RenderProperties::cProperties::cRegkRenderModelThresholdScroll::cRegkRenderModelThresholdScroll()"},
    {0x100293a40u, 294u, "cSC4RenderProperties::cProperties::cRegkParticleSizeThreshold::cRegkParticleSizeThreshold()"},
    {0x100293b66u, 294u, "cSC4RenderProperties::cProperties::cRegkMaxVehiclePct::cRegkMaxVehiclePct()"},
    {0x100293c8cu, 294u, "cSC4RenderProperties::cProperties::cRegkMaxPedPct::cRegkMaxPedPct()"},
    {0x100293db2u, 294u, "cSC4RenderProperties::cProperties::cRegkOpaqueUIAlphaValue::cRegkOpaqueUIAlphaValue()"},
    // cHitChannel (54)
    {0x100474874u, 226u, "cHitChannel::operator=(cHitChannel const&)"},
    {0x100482a00u, 508u, "cHitChannel::cHitChannel()"},
    {0x100483014u, 94u, "cHitChannel::NoteOn()"},
    {0x100483072u, 76u, "cHitChannel::UpdatePatch()"},
    {0x1004830beu, 66u, "cHitChannel::Load()"},
    {0x100483100u, 24u, "cHitChannel::Unload()"},
    {0x100483118u, 232u, "cHitChannel::UpdatePitch()"},
    {0x100483200u, 532u, "cHitChannel::UpdateVolPan()"},
    {0x100483414u, 70u, "cHitChannel::SetDataSource(cIGZSndDataSource*, cIGZSnd::SndStreamingType)"},
    {0x1004834a0u, 468u, "cHitChannel::CreateSnd()"},
    {0x100483674u, 1062u, "cHitChannel::HandleNotifySetPropertyValue(int)"},
    {0x100483a9au, 22u, "cHitChannel::GetPan() const"},
    {0x100483ab0u, 26u, "cHitChannel::SetPan(int)"},
    {0x100483acau, 22u, "cHitChannel::GetFrequency() const"},
    {0x100483ae0u, 26u, "cHitChannel::SetFrequency(int)"},
    {0x100483b16u, 26u, "cHitChannel::SetVolume(int)"},
    {0x100483b30u, 22u, "cHitChannel::GetVolume() const"},
    {0x100483b78u, 132u, "cHitChannel::GetPosition(float*, float*, float*) const"},
    {0x100483c0au, 96u, "cHitChannel::SetPosition(float, float, float)"},
    {0x100483cdcu, 132u, "cHitChannel::GetVelocity(float*, float*, float*) const"},
    {0x100483d6eu, 96u, "cHitChannel::SetVelocity(float, float, float)"},
    {0x100483e40u, 260u, "cHitChannel::GetAll(float*, float*, float*, float*, float*, float*) const"},
    {0x100483f52u, 188u, "cHitChannel::SetAll(float, float, float, float, float, float)"},
    {0x100484020u, 22u, "cHitChannel::GetMinimumDistance(float*) const"},
    {0x100484052u, 28u, "cHitChannel::SetMinimumDistance(float)"},
    {0x100484090u, 22u, "cHitChannel::GetMaximumDistance(float*) const"},
    {0x1004840c2u, 28u, "cHitChannel::SetMaximumDistance(float)"},
    {0x100484100u, 28u, "cHitChannel::SetHeadRelative(bool)"},
    {0x10048413eu, 26u, "cHitChannel::GetHeadRelative() const"},
    {0x100484176u, 126u, "cHitChannel::SetRegisterVal(int, int, bool)"},
    {0x100484206u, 130u, "cHitChannel::SetRegisterFloat(unsigned int, float, bool)"},
    {0x1004842a0u, 158u, "cHitChannel::GetRegisterVal(int) const"},
    {0x10048434cu, 164u, "cHitChannel::GetRegisterFloat(unsigned int) const"},
    {0x1004843feu, 72u, "cHitChannel::ClearRegisterVal(int, bool)"},
    {0x100484456u, 68u, "cHitChannel::HasRegisterVal(int) const"},
    {0x1004844a8u, 120u, "cHitChannel::SetRolloffFactor(float)"},
    {0x100484520u, 114u, "cHitChannel::GetRolloffFactor(float&) const"},
    {0x100484592u, 132u, "cHitChannel::SetInputDryLevel(float)"},
    {0x100484616u, 132u, "cHitChannel::SetInputEffectLevel(float)"},
    {0x10048469au, 124u, "cHitChannel::SetLowPassCutoff(int)"},
    {0x100484716u, 124u, "cHitChannel::SetHighPassCutoff(int)"},
    {0x100484792u, 132u, "cHitChannel::SetTimeMultiplier(float)"},
    {0x100484826u, 34u, "cHitChannel::Shutdown()"},
    {0x100484848u, 14u, "cHitChannel::Play(int, bool, cIGZSnd::SndDup)"},
    {0x100484856u, 14u, "cHitChannel::Play(int, int, int, int)"},
    {0x100484864u, 14u, "cHitChannel::IsPlaying() const"},
    {0x100484872u, 14u, "cHitChannel::Stop()"},
    {0x100484880u, 14u, "cHitChannel::Kill()"},
    {0x1004848bau, 30u, "cHitChannel::Pause(bool)"},
    {0x1004848d8u, 26u, "cHitChannel::Unpause()"},
    {0x1004848f2u, 26u, "cHitChannel::IsPaused() const"},
    {0x100484978u, 16u, "cHitChannel::GetDataSource()"},
    {0x100484994u, 14u, "cHitChannel::NoteOff()"},
    {0x1004849a2u, 24u, "cHitChannel::NoteKill()"},
    // cSC4EffectsManager (46)
    {0x1005dad3eu, 1326u, "cSC4EffectsManager::cSC4EffectsManager()"},
    {0x1005dba0eu, 2125u, "cSC4EffectsManager::Init()"},
    {0x1005dc2a2u, 158u, "cSC4EffectsManager::CreateFlashMaterials()"},
    {0x1005dc340u, 1417u, "cSC4EffectsManager::LoadAllEffects()"},
    {0x1005dcb24u, 574u, "cSC4EffectsManager::sCreateEffect(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*)"},
    {0x1005dcd62u, 2228u, "cSC4EffectsManager::Shutdown()"},
    {0x1005dd78eu, 42u, "cSC4EffectsManager::FinalRelease()"},
    {0x1005dd7b8u, 1380u, "cSC4EffectsManager::DoMessage(cIGZMessage2*)"},
    {0x1005ddd1cu, 484u, "cSC4EffectsManager::DoAnimationTick(unsigned int)"},
    {0x1005ddf00u, 260u, "cSC4EffectsManager::DoZoomChanged(int)"},
    {0x1005de004u, 80u, "cSC4EffectsManager::DoRotationChanged()"},
    {0x1005de054u, 232u, "cSC4EffectsManager::DoEffectsTick()"},
    {0x1005de13cu, 356u, "cSC4EffectsManager::UpdateSnowline()"},
    {0x1005de2b6u, 84u, "cSC4EffectsManager::UpdateActiveVisualEffects()"},
    {0x1005de30au, 898u, "cSC4EffectsManager::DoRenderPropertiesChanged()"},
    {0x1005de68cu, 214u, "cSC4EffectsManager::DoOccupantInserted(cISC4Occupant*)"},
    {0x1005de762u, 370u, "cSC4EffectsManager::DoOccupantRemoved(cISC4Occupant*)"},
    {0x1005de8d4u, 242u, "cSC4EffectsManager::DoSnowCheat(cRZCmdLine&)"},
    {0x1005de9c6u, 2742u, "cSC4EffectsManager::DoEffectCheat(cRZCmdLine&)"},
    {0x1005df48au, 1754u, "cSC4EffectsManager::Update(SC4DrawContext*)"},
    {0x1005dfb64u, 934u, "cSC4EffectsManager::DrawScreenFlashes(SC4DrawContext*, float)"},
    {0x1005dff0au, 504u, "cSC4EffectsManager::UpdateLightingTint(float)"},
    {0x1005e0102u, 1450u, "cSC4EffectsManager::DeactivateUnusedVisualEffects()"},
    {0x1005e06ceu, 142u, "cSC4EffectsManager::HasVisualEffect(char const*) const"},
    {0x1005e075cu, 576u, "cSC4EffectsManager::CreateVisualEffect(char const*, cISC4VisualEffect**)"},
    {0x1005e099cu, 402u, "cSC4EffectsManager::NewVisualEffect()"},
    {0x1005e0b2eu, 218u, "cSC4EffectsManager::ActivateNewVisualEffects()"},
    {0x1005e0c08u, 298u, "cSC4EffectsManager::CreateVisualEffect(unsigned int, unsigned int, cISC4VisualEffect**)"},
    {0x1005e0d32u, 166u, "cSC4EffectsManager::BeginCollection()"},
    {0x1005e0deau, 1210u, "cSC4EffectsManager::EndCollection()"},
    {0x1005e1318u, 332u, "cSC4EffectsManager::GetStatsString(cRZString&)"},
    {0x1005e1464u, 458u, "cSC4EffectsManager::StartAncilliary(cSC4VisualEffect*)"},
    {0x1005e162eu, 176u, "cSC4EffectsManager::StartScreenFlash(cSC4LightDescription const&, cS3DVector2*, float)"},
    {0x1005e16deu, 108u, "cSC4EffectsManager::StartLightingTint(cSC4LightDescription const&)"},
    {0x1005e174au, 262u, "cSC4EffectsManager::StopAncilliary(cSC4VisualEffect*)"},
    {0x1005e1850u, 2472u, "cSC4EffectsManager::UpdateVisualEffect(cSC4VisualEffect*, bool)"},
    {0x1005e21f8u, 140u, "cSC4EffectsManager::UpdateLODScale(cSC4EffectDescription::cDescriptionRec const&, cSC4VisualEffect::cComponentRec*)"},
    {0x1005e23c6u, 982u, "cSC4EffectsManager::CreateComponentFromDescription(cSC4EffectDescription::cDescriptionRec const&, cSC4VisualEffect::cComponentRec*, cSC4ModelBasedParticlesEffect**)"},
    {0x1005e279cu, 364u, "cSC4EffectsManager::NewModelBasedParticlesEffect()"},
    {0x1005e2908u, 364u, "cSC4EffectsManager::NewGeometrySourcedParticlesEffect()"},
    {0x1005e2a74u, 414u, "cSC4EffectsManager::NewParticlesEffect()"},
    {0x1005e2c12u, 366u, "cSC4EffectsManager::NewDynamicParticleEffect()"},
    {0x1005e2d80u, 346u, "cSC4EffectsManager::NewDecalEffect()"},
    {0x1005e30f8u, 100u, "cSC4EffectsManager::KillOrphanedEffects()"},
    {0x1005e31ccu, 126u, "cSC4EffectsManager::SetSnowlineState(bool)"},
    {0x1005e324au, 237u, "cSC4EffectsManager::ParseEffectsFiles()"},
    // cSC4ParticlesEffect (39)
    {0x1005f27e2u, 698u, "cSC4ParticlesEffect::cSC4ParticlesEffect()"},
    {0x1005f2aa6u, 224u, "cSC4ParticlesEffect::Init()"},
    {0x1005f2b94u, 112u, "cSC4ParticlesEffect::Shutdown()"},
    {0x1005f2c12u, 1102u, "cSC4ParticlesEffect::Start(cISC4VisualEffect::tTransitionType)"},
    {0x1005f3060u, 844u, "cSC4ParticlesEffect::UpdateRandomWalk(float)"},
    {0x1005f33ccu, 870u, "cSC4ParticlesEffect::SetTransforms(cS3DTransform const&, cS3DTransform const&)"},
    {0x1005f3738u, 418u, "cSC4ParticlesEffect::SetFloatParams(cISC4VisualEffect::tParamType, float const*, int)"},
    {0x1005f38dau, 26u, "cSC4ParticlesEffect::SetVoidParams(cISC4VisualEffect::tParamType, void**, int)"},
    {0x1005f38f4u, 796u, "cSC4ParticlesEffect::Update(float, SC4DrawContext*)"},
    {0x1005f3c10u, 248u, "cSC4ParticlesEffect::SetCitySideEmissionVolume(int)"},
    {0x1005f3d08u, 1076u, "cSC4ParticlesEffect::UpdateAttractor()"},
    {0x1005f413cu, 234u, "cSC4ParticlesEffect::LaunchTimedEffects()"},
    {0x1005f4226u, 988u, "cSC4ParticlesEffect::SetDescription(cSC4ParticlesDescription*)"},
    {0x1005f4602u, 518u, "cSC4ParticlesEffect::SetDrawMode(cSC4ParticlesDescriptionBase::tDrawMode)"},
    {0x1005f4808u, 210u, "cSC4ParticlesEffect::SetTexture(unsigned int)"},
    {0x1005f48dau, 72u, "cSC4ParticlesEffect::SetModel(cIS3DModelInstance*)"},
    {0x1005f4922u, 892u, "cSC4ParticlesEffect::SetTarget(cS3DVector3 const&)"},
    {0x1005f4cbau, 84u, "cSC4ParticlesEffect::SetVertexBufferExtension(cIGZGDriverVertexBufferExtension*)"},
    {0x1005f4d34u, 364u, "cSC4ParticlesEffect::PrepareForRendering(SC4DrawContext*)"},
    {0x1005f4ea0u, 140u, "cSC4ParticlesEffect::GenerateNewParticles()"},
    {0x1005f4f2cu, 212u, "cSC4ParticlesEffect::MaintainParticles(float)"},
    {0x1005f5000u, 648u, "cSC4ParticlesEffect::InjectParticles(float)"},
    {0x1005f5288u, 966u, "cSC4ParticlesEffect::AddRateCurveParticles(float)"},
    {0x1005f564eu, 170u, "cSC4ParticlesEffect::LaunchTimeZeroEffects(cSCParticle*)"},
    {0x1005f56f8u, 638u, "cSC4ParticlesEffect::RenderActiveParticles(SC4DrawContext*)"},
    {0x1005f5976u, 3026u, "cSC4ParticlesEffect::ProcessParticle(cSC4EffectsParticle*, int, cS3DVector3&, cS3DVector3&, cS3DVector3&, cS3DColorARGB8&)"},
    {0x1005f6548u, 1386u, "cSC4ParticlesEffect::RenderActiveParticlesVB(SC4DrawContext*)"},
    {0x1005f6ab2u, 2942u, "cSC4ParticlesEffect::ProcessParticle(cSC4EffectsParticle*, int, cS3DVector3&, cS3DVector3&, cS3DVector3&, cS3DColorARGB32&)"},
    {0x1005f7630u, 650u, "cSC4ParticlesEffect::UpdateActiveParticles()"},
    {0x1005f78bau, 258u, "cSC4ParticlesEffect::UpdateParticlePhysics(cSC4EffectsParticle*, float)"},
    {0x1005f79bcu, 1066u, "cSC4ParticlesEffect::UpdateParticlePhysicsExtra(cSC4EffectsParticle*, float)"},
    {0x1005f7de6u, 1524u, "cSC4ParticlesEffect::ApplyTractorForce(cSC4EffectsParticle*, float)"},
    {0x1005f83dau, 2474u, "cSC4ParticlesEffect::ApplyTerrainForces(cSC4EffectsParticle*, cS3DVector3 const&, float)"},
    {0x1005f8d84u, 660u, "cSC4ParticlesEffect::TestBuildingCollision(cSC4EffectsParticle*)"},
    {0x1005f9018u, 386u, "cSC4ParticlesEffect::TriggerEffect(char const*, cS3DVector3 const&)"},
    {0x1005f91a4u, 128u, "cSC4ParticlesEffect::SetPrimitivePoolFormat(cSC4PrimitivePool*)"},
    {0x1005f9224u, 100u, "cSC4ParticlesEffect::CreateParticle()"},
    {0x1005f9288u, 2092u, "cSC4ParticlesEffect::InitEffectsParticle(cSC4EffectsParticle*)"},
    {0x1005f9ac2u, 726u, "cSC4ParticlesEffect::DoHeightBasedEmit(cSC4EffectsParticle*)"},
    // cSC4WinMapView (34)
    {0x1003494a6u, 630u, "cSC4WinMapView::cSC4WinMapView()"},
    {0x1003499a4u, 392u, "cSC4WinMapView::Init()"},
    {0x100349b3au, 682u, "cSC4WinMapView::Shutdown()"},
    {0x100349df6u, 358u, "cSC4WinMapView::DoMessage(cIGZMessage2*)"},
    {0x100349f5cu, 2284u, "cSC4WinMapView::UpdateHighlights()"},
    {0x10034a848u, 9468u, "cSC4WinMapView::Update()"},
    {0x10034cd56u, 140u, "cSC4WinMapView::OnViewModeButtonSelected(unsigned int, int)"},
    {0x10034ce8cu, 2078u, "cSC4WinMapView::DisplayLegend(unsigned int)"},
    {0x10034d6aau, 180u, "cSC4WinMapView::ShowLargeWindow()"},
    {0x10034d75eu, 72u, "cSC4WinMapView::OnDesirabilityModeButtonSelected(unsigned int, int)"},
    {0x10034d7a6u, 72u, "cSC4WinMapView::OnTrafficModeButtonSelected(unsigned int, int)"},
    {0x10034d7eeu, 2178u, "cSC4WinMapView::DoMessage(cGZMessage&)"},
    {0x10034e070u, 188u, "cSC4WinMapView::ShowDesirableWindow()"},
    {0x10034e12cu, 198u, "cSC4WinMapView::ShowTrafficWindow()"},
    {0x10034e1f2u, 134u, "cSC4WinMapView::ShowSmallWindow()"},
    {0x10034e316u, 258u, "cSC4WinMapView::ResetDataView()"},
    {0x10034e426u, 778u, "cSC4WinMapView::HookUpdateMessages()"},
    {0x10034e730u, 2522u, "cSC4WinMapView::DisplayStandardUint8Grid(std::__1::vector<std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>, std::__1::allocator<std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>>> const&, cIGZBuffer*, float, float)"},
    {0x10034f10au, 2496u, "cSC4WinMapView::DisplayStandardSint8Grid(std::__1::vector<std::__1::vector<signed char, std::__1::allocator<signed char>>, std::__1::allocator<std::__1::vector<signed char, std::__1::allocator<signed char>>>> const&, cIGZBuffer*, float, float)"},
    {0x10034facau, 330u, "cSC4WinMapView::DisplayStandardFloat32GridAsUint8(std::__1::vector<std::__1::vector<float, std::__1::allocator<float>>, std::__1::allocator<std::__1::vector<float, std::__1::allocator<float>>>> const&, cIGZBuffer*, float, float)"},
    {0x10034fc14u, 370u, "cSC4WinMapView::DisplayStandardSint16GridScaled(std::__1::vector<std::__1::vector<short, std::__1::allocator<short>>, std::__1::allocator<std::__1::vector<short, std::__1::allocator<short>>>> const&, cIGZBuffer*, float, float)"},
    {0x10034fd86u, 416u, "cSC4WinMapView::DrawTransportationPotholeDecals(cIGZBuffer*)"},
    {0x10034ff26u, 356u, "cSC4WinMapView::DisplayStandardUint16GridScaled(std::__1::vector<std::__1::vector<unsigned short, std::__1::allocator<unsigned short>>, std::__1::allocator<std::__1::vector<unsigned short, std::__1::allocator<unsigned short>>>> const&, cIGZBuffer*, float, float)"},
    {0x1003503fau, 970u, "cSC4WinMapView::UpdateOccupancyMap(cISC4GrowthDeveloper const*)"},
    {0x1003507c4u, 526u, "cSC4WinMapView::AddNewHighlight(cISC4Occupant*, float)"},
    {0x100350a1cu, 1416u, "cSC4WinMapView::MergeNewHighlights()"},
    {0x100350fa4u, 343u, "cSC4WinMapView::ActivateHighlight(cSC4WinMapView::cHighlightInfo&)"},
    {0x100351790u, 78u, "cSC4WinMapView::PushView(unsigned int)"},
    {0x1003517deu, 104u, "cSC4WinMapView::PopView(unsigned int)"},
    {0x100351858u, 3524u, "cSC4WinMapView::SetDataView(unsigned int)"},
    {0x10035261cu, 68u, "cSC4WinMapView::IsVisible() const"},
    {0x10035266eu, 322u, "cSC4WinMapView::InitWindow()"},
    {0x1003527b0u, 688u, "cSC4WinMapView::ShowWindow()"},
    {0x100352a6eu, 194u, "cSC4WinMapView::HideWindow()"},
    // nsISC4GameDataRegistry (27)
    {0x1000e92d7u, 278u, "unsigned long nsISC4GameDataRegistry::AddNode<cSC4CivicBuildingSimulator, cIGZUnknown>(cSC4CivicBuildingSimulator*, char const*, cIGZUnknown*, int, void*, cISC4GameDataRegistry*)"},
    {0x1000e93edu, 223u, "unsigned long nsISC4GameDataRegistry::AddValue<cSC4CivicBuildingSimulator>(void*, char const*, bool (*)(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*), cSC4CivicBuildingSimulator*, int, void*, cISC4GameDataRegistry*)"},
    {0x10010c4e4u, 278u, "unsigned long nsISC4GameDataRegistry::AddNode<cIGZUnknown, cISC4AutomataScriptSystem>(cIGZUnknown*, char const*, cISC4AutomataScriptSystem*, int, void*, cISC4GameDataRegistry*)"},
    {0x10010c5fau, 222u, "unsigned long nsISC4GameDataRegistry::AddValue<cIGZUnknown>(void*, char const*, bool (*)(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*), cIGZUnknown*, int, void*, cISC4GameDataRegistry*)"},
    {0x1001116ccu, 222u, "unsigned long nsISC4GameDataRegistry::AddValue<cSC4Automaton>(void*, char const*, bool (*)(void*, cIGZVariant&, bool, cISC4GameDataRegistry*), cSC4Automaton*, int, void*, cISC4GameDataRegistry*)"},
    {0x1001117aau, 49u, "nsISC4GameDataRegistry::GenericValueExchnageFuncFloat32(void*, cIGZVariant&, bool, cISC4GameDataRegistry*)"},
    {0x1001d634eu, 278u, "unsigned long nsISC4GameDataRegistry::AddNode<cSC4AutomataScriptSystem, cIGZUnknown>(cSC4AutomataScriptSystem*, char const*, cIGZUnknown*, int, void*, cISC4GameDataRegistry*)"},
    {0x1001d6464u, 222u, "unsigned long nsISC4GameDataRegistry::AddValue<cSC4AutomataScriptSystem>(void*, char const*, bool (*)(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*), cSC4AutomataScriptSystem*, int, void*, cISC4GameDataRegistry*)"},
    {0x10021902bu, 278u, "unsigned long nsISC4GameDataRegistry::AddNode<cSC4AuraSimulator, cIGZUnknown>(cSC4AuraSimulator*, char const*, cIGZUnknown*, int, void*, cISC4GameDataRegistry*)"},
    {0x100219141u, 223u, "unsigned long nsISC4GameDataRegistry::AddValue<cSC4AuraSimulator>(void*, char const*, bool (*)(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*), cSC4AuraSimulator*, int, void*, cISC4GameDataRegistry*)"},
    {0x10021ee1bu, 278u, "unsigned long nsISC4GameDataRegistry::AddNode<cSC4BudgetSimulator, cIGZUnknown>(cSC4BudgetSimulator*, char const*, cIGZUnknown*, int, void*, cISC4GameDataRegistry*)"},
    {0x10021ef31u, 223u, "unsigned long nsISC4GameDataRegistry::AddValue<cSC4BudgetSimulator>(void*, char const*, bool (*)(void*, cIGZVariant&, bool, cISC4GameDataRegistry*), cSC4BudgetSimulator*, int, void*, cISC4GameDataRegistry*)"},
    {0x10021f0a8u, 222u, "unsigned long nsISC4GameDataRegistry::AddValue<cSC4BudgetSimulator>(void*, char const*, bool (*)(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*), cSC4BudgetSimulator*, int, void*, cISC4GameDataRegistry*)"},
    {0x100315be6u, 278u, "unsigned long nsISC4GameDataRegistry::AddNode<cSC4MySimWinProc, cIGZUnknown>(cSC4MySimWinProc*, char const*, cIGZUnknown*, int, void*, cISC4GameDataRegistry*)"},
    {0x100315cfcu, 222u, "unsigned long nsISC4GameDataRegistry::AddValue<cSC4MySimWinProc>(void*, char const*, bool (*)(void*, cIGZVariant&, bool, cISC4GameDataRegistry*), cSC4MySimWinProc*, int, void*, cISC4GameDataRegistry*)"},
    {0x1005dc930u, 278u, "unsigned long nsISC4GameDataRegistry::AddNode<cSC4EffectsManager, cIGZUnknown>(cSC4EffectsManager*, char const*, cIGZUnknown*, int, void*, cISC4GameDataRegistry*)"},
    {0x1005dca46u, 222u, "unsigned long nsISC4GameDataRegistry::AddValue<cSC4EffectsManager>(void*, char const*, bool (*)(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*), cSC4EffectsManager*, int, void*, cISC4GameDataRegistry*)"},
    {0x1006d7da3u, 278u, "unsigned long nsISC4GameDataRegistry::AddNode<cSC4CitySituationManager, cIGZUnknown>(cSC4CitySituationManager*, char const*, cIGZUnknown*, int, void*, cISC4GameDataRegistry*)"},
    {0x1006d7eb9u, 223u, "unsigned long nsISC4GameDataRegistry::AddValue<cSC4CitySituationManager>(void*, char const*, bool (*)(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*), cSC4CitySituationManager*, int, void*, cISC4GameDataRegistry*)"},
    {0x1006e2e4au, 278u, "unsigned long nsISC4GameDataRegistry::AddNode<cISC4Occupant, cSC4CitySituationManager>(cISC4Occupant*, char const*, cSC4CitySituationManager*, int, void*, cISC4GameDataRegistry*)"},
    {0x1006e6628u, 222u, "unsigned long nsISC4GameDataRegistry::AddValue<cSC4CitySituationManager>(void*, char const*, bool (*)(void*, cIGZVariant&, bool, cISC4GameDataRegistry*), cSC4CitySituationManager*, int, void*, cISC4GameDataRegistry*)"},
    {0x1006e6706u, 50u, "nsISC4GameDataRegistry::GenericValueExchnageFuncUint32(void*, cIGZVariant&, bool, cISC4GameDataRegistry*)"},
    {0x1007a1c7du, 278u, "unsigned long nsISC4GameDataRegistry::AddNode<cISC4MySimAgentSimulator, cIGZUnknown>(cISC4MySimAgentSimulator*, char const*, cIGZUnknown*, int, void*, cISC4GameDataRegistry*)"},
    {0x1007a1d93u, 278u, "unsigned long nsISC4GameDataRegistry::AddNode<cSC4MySim, cISC4MySimAgentSimulator>(cSC4MySim*, char const*, cISC4MySimAgentSimulator*, int, void*, cISC4GameDataRegistry*)"},
    {0x1007a1ea9u, 222u, "unsigned long nsISC4GameDataRegistry::AddValue<cSC4MySim>(void*, char const*, bool (*)(void*, cIGZVariant&, bool, cISC4GameDataRegistry*), cSC4MySim*, int, void*, cISC4GameDataRegistry*)"},
    {0x1007ab4c4u, 44u, "nsISC4GameDataRegistry::GenericValueExchnageFuncUint8(void*, cIGZVariant&, bool, cISC4GameDataRegistry*)"},
    {0x1007abf92u, 222u, "unsigned long nsISC4GameDataRegistry::AddValue<cSC4MySimAgentSimulator>(void*, char const*, bool (*)(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*), cSC4MySimAgentSimulator*, int, void*, cISC4GameDataRegistry*)"},
    // cSLTokenSerialRecordIO_Text (25)
    {0x1004e64d0u, 244u, "cSLTokenSerialRecordIO_Text::GetToken(cRZString&, char)"},
    {0x1004e65c4u, 278u, "cSLTokenSerialRecordIO_Text::ReadTokenDef(cGZPersistTokenDef&)"},
    {0x1004e66dau, 178u, "cSLTokenSerialRecordIO_Text::Read(unsigned int&, char)"},
    {0x1004e818au, 46u, "cSLTokenSerialRecordIO_Text::Read(unsigned char&)"},
    {0x1004e81b8u, 46u, "cSLTokenSerialRecordIO_Text::Read(signed char&)"},
    {0x1004e81e6u, 48u, "cSLTokenSerialRecordIO_Text::Read(unsigned short&)"},
    {0x1004e8216u, 48u, "cSLTokenSerialRecordIO_Text::Read(short&)"},
    {0x1004e828eu, 168u, "cSLTokenSerialRecordIO_Text::Read(long long&)"},
    {0x1004e8336u, 180u, "cSLTokenSerialRecordIO_Text::Read(float&)"},
    {0x1004e83eau, 168u, "cSLTokenSerialRecordIO_Text::Read(double&)"},
    {0x1004e84a2u, 100u, "cSLTokenSerialRecordIO_Text::Read(cGZPersistResourceKey&)"},
    {0x1004e8506u, 218u, "cSLTokenSerialRecordIO_Text::WriteTokenDef(cRZString&, cGZPersistTokenDef&)"},
    {0x1004e85e0u, 176u, "cSLTokenSerialRecordIO_Text::Write(unsigned char&)"},
    {0x1004e8690u, 176u, "cSLTokenSerialRecordIO_Text::Write(signed char&)"},
    {0x1004e8740u, 176u, "cSLTokenSerialRecordIO_Text::Write(unsigned short&)"},
    {0x1004e87f0u, 176u, "cSLTokenSerialRecordIO_Text::Write(short&)"},
    {0x1004e88a0u, 176u, "cSLTokenSerialRecordIO_Text::Write(unsigned int&)"},
    {0x1004e8950u, 176u, "cSLTokenSerialRecordIO_Text::Write(int&)"},
    {0x1004e8a00u, 176u, "cSLTokenSerialRecordIO_Text::Write(unsigned long long&)"},
    {0x1004e8ab0u, 176u, "cSLTokenSerialRecordIO_Text::Write(long long&)"},
    {0x1004e8b60u, 178u, "cSLTokenSerialRecordIO_Text::Write(float&)"},
    {0x1004e8c12u, 178u, "cSLTokenSerialRecordIO_Text::Write(double&)"},
    {0x1004e8cc4u, 190u, "cSLTokenSerialRecordIO_Text::Write(cRZString&)"},
    {0x1004e8d82u, 118u, "cSLTokenSerialRecordIO_Text::Write(cGZPersistResourceKey&)"},
    {0x1004e91dcu, 164u, "cSLTokenSerialRecordIO_Text::Read(int&, char)"},
    // cSC4EffectsCollection (23)
    {0x100576350u, 620u, "cSC4EffectsCollection::cSC4EffectsCollection()"},
    {0x1005767d8u, 138u, "cSC4EffectsCollection::BeginCollection()"},
    {0x100576868u, 336u, "cSC4EffectsCollection::AddParticleDescription(char const*, cSC4ParticlesDescription const&)"},
    {0x100576a42u, 176u, "cSC4EffectsCollection::GetParticleDescription(char const*, cSC4ParticlesDescription*)"},
    {0x100576af2u, 354u, "cSC4EffectsCollection::AddDynamicParticleDescription(char const*, cSC4DynamicParticleDescription const&)"},
    {0x100576c54u, 182u, "cSC4EffectsCollection::GetDynamicParticleDescription(char const*, cSC4DynamicParticleDescription*)"},
    {0x100576d0au, 430u, "cSC4EffectsCollection::AddDecalDescription(char const*, cSC4DecalDescription const&)"},
    {0x100576f8eu, 376u, "cSC4EffectsCollection::GetDecalDescription(char const*, cSC4DecalDescription*)"},
    {0x100577106u, 286u, "cSC4EffectsCollection::AddShakeDescription(char const*, cSC4ShakeDescription const&)"},
    {0x1005772a6u, 236u, "cSC4EffectsCollection::GetShakeDescription(char const*, cSC4ShakeDescription*)"},
    {0x100577392u, 288u, "cSC4EffectsCollection::AddLightDescription(char const*, cSC4LightDescription const&)"},
    {0x1005774b2u, 226u, "cSC4EffectsCollection::GetLightDescription(char const*, cSC4LightDescription*)"},
    {0x100577594u, 1130u, "cSC4EffectsCollection::AddEffectDescription(char const*, cSC4EffectDescription const&)"},
    {0x1005779feu, 262u, "cSC4EffectsCollection::GetEffectDescription(char const*, cSC4EffectDescription*)"},
    {0x100577b04u, 544u, "cSC4EffectsCollection::AddComponentDescription(char const*, cISC4ComponentDescription*)"},
    {0x100577d24u, 210u, "cSC4EffectsCollection::GetComponentDescription(char const*, unsigned int, cISC4ComponentDescription**)"},
    {0x100577df6u, 72u, "cSC4EffectsCollection::SetCameraParams(cSC4CameraParams const&)"},
    {0x100577e3eu, 352u, "cSC4EffectsCollection::AddEffectBinding(char const*, unsigned int, unsigned int)"},
    {0x100577f9eu, 78u, "cSC4EffectsCollection::AddMessageTrigger(cSC4MessageTriggerDescription const&)"},
    {0x100577fecu, 624u, "cSC4EffectsCollection::UpdateEffectPriority(char const*, unsigned int, char const*)"},
    {0x10057825cu, 940u, "cSC4EffectsCollection::AddToCollection(cSC4EffectsCollection*)"},
    {0x100578608u, 542u, "cSC4EffectsCollection::AddToCollectionWithNames(cISC4EffectsCollection*)"},
    {0x100578826u, 1238u, "cSC4EffectsCollection::AddToCollectionWithRenumbering(cSC4EffectsCollection*)"},
    // cSTEWaterRipple (22)
    {0x100755072u, 388u, "cSTEWaterRipple::CreateRipples()"},
    {0x1007551f6u, 294u, "cSTEWaterRipple::ComputeBoundRects()"},
    {0x10075531cu, 634u, "cSTEWaterRipple::ComputeBoundRectsForRipple(cSTEWaterRipple::tRipple&)"},
    {0x1007555c6u, 704u, "cSTEWaterRipple::ComputeSplinePoints(cSTEWaterRipple::tShore&, int, float)"},
    {0x100755886u, 334u, "cSTEWaterRipple::DetectShores()"},
    {0x1007559d4u, 1700u, "cSTEWaterRipple::CreateRipplesForShore(cSTEWaterRipple::tShore&)"},
    {0x100756078u, 394u, "cSTEWaterRipple::StartNewRipple(cSTEWaterRipple::tShore&, cSTEWaterRipple::tRipple*&)"},
    {0x100756256u, 448u, "cSTEWaterRipple::PrepareRippleForDisplay(cSTEWaterRipple::tRipple&)"},
    {0x100756416u, 550u, "cSTEWaterRipple::CreateShoreObject(std::__1::deque<int, std::__1::allocator<int>>&)"},
    {0x10075663cu, 144u, "cSTEWaterRipple::cSTEWaterRipple()"},
    {0x1007566d6u, 314u, "cSTEWaterRipple::DoAnimationTick(unsigned int)"},
    {0x100756810u, 88u, "cSTEWaterRipple::TickRipple(cSTEWaterRipple::tRipple&, unsigned int)"},
    {0x100756868u, 164u, "cSTEWaterRipple::DoMessage(cIGZMessage2*)"},
    {0x10075690cu, 244u, "cSTEWaterRipple::DoOnTerrainEdit(cIGZMessage2Standard*)"},
    {0x100756a12u, 186u, "cSTEWaterRipple::IntersectsRipples(SC4Rect<int> const&)"},
    {0x100756accu, 136u, "cSTEWaterRipple::DrawRipple(cSTEWaterRipple::tRipple const&, SC4DrawContext*)"},
    {0x100756b54u, 402u, "cSTEWaterRipple::Init()"},
    {0x100756ce6u, 58u, "cSTEWaterRipple::StaticInit()"},
    {0x100756d20u, 88u, "cSTEWaterRipple::IntersectsRipple(SC4Rect<int> const&, cSTEWaterRipple::tRipple const&) const"},
    {0x100756e4au, 646u, "cSTEWaterRipple::ReadTuningParameters()"},
    {0x1007570f6u, 174u, "cSTEWaterRipple::Shutdown()"},
    {0x100757278u, 340u, "cSTEWaterRipple::Update(SC4DrawContext*)"},
    // cSC4ViewInputControlTerrainScrubber (19)
    {0x100395b10u, 412u, "cSC4ViewInputControlTerrainScrubber::cSC4ViewInputControlTerrainScrubber(unsigned int, bool)"},
    {0x100395df0u, 242u, "cSC4ViewInputControlTerrainScrubber::DoMessage(cIGZMessage2*)"},
    {0x100395ee2u, 152u, "cSC4ViewInputControlTerrainScrubber::PushDelayedAction(SC4Point<float> const&, SC4Point<float> const&)"},
    {0x100395f7au, 470u, "cSC4ViewInputControlTerrainScrubber::ProcessDelayedActions(bool)"},
    {0x100396150u, 528u, "cSC4ViewInputControlTerrainScrubber::SetCursorEchoPos(cS3DVector3 const&, bool)"},
    {0x100396372u, 242u, "cSC4ViewInputControlTerrainScrubber::Init()"},
    {0x100396464u, 58u, "cSC4ViewInputControlTerrainScrubber::StaticInit()"},
    {0x10039649eu, 366u, "cSC4ViewInputControlTerrainScrubber::SetCursorEchoState(bool)"},
    {0x10039660cu, 334u, "cSC4ViewInputControlTerrainScrubber::OnKeyDown(unsigned int, unsigned int)"},
    {0x10039675au, 114u, "cSC4ViewInputControlTerrainScrubber::OnKeyUp(unsigned int, unsigned int)"},
    {0x1003967ccu, 588u, "cSC4ViewInputControlTerrainScrubber::OnMouseDownL(int, int, unsigned int)"},
    {0x100396a18u, 122u, "cSC4ViewInputControlTerrainScrubber::SetCursorAnchor(int, int)"},
    {0x100396a92u, 262u, "cSC4ViewInputControlTerrainScrubber::GetCursorPositionInWorld(int, int, float*)"},
    {0x100396b98u, 204u, "cSC4ViewInputControlTerrainScrubber::OnMouseMove(int, int, unsigned int)"},
    {0x100396c64u, 218u, "cSC4ViewInputControlTerrainScrubber::OnMouseUpL(int, int, unsigned int)"},
    {0x100396d3eu, 40u, "cSC4ViewInputControlTerrainScrubber::OnMouseExit()"},
    {0x100396daeu, 266u, "cSC4ViewInputControlTerrainScrubber::ReadTuningParameters()"},
    {0x100396eb8u, 94u, "cSC4ViewInputControlTerrainScrubber::RecordBrushApplication()"},
    {0x100396f16u, 392u, "cSC4ViewInputControlTerrainScrubber::Shutdown()"},
    // cS3DModelInstanceAnimated (18)
    {0x1001a2b7cu, 154u, "cS3DModelInstanceAnimated::cS3DModelInstanceAnimated()"},
    {0x1001a2de4u, 3480u, "cS3DModelInstanceAnimated::SetOrientation(cS3DVector3 const&)"},
    {0x1001a3b7cu, 316u, "cS3DModelInstanceAnimated::SetOrientation(cS3DVector3 const&, cS3DVector3 const&)"},
    {0x1001a3cb8u, 200u, "cS3DModelInstanceAnimated::ModelPointToWorldPoint(cS3DVector3&)"},
    {0x1001a3d80u, 172u, "cS3DModelInstanceAnimated::ModelPointToWorldPoint(float*)"},
    {0x1001a3e2cu, 128u, "cS3DModelInstanceAnimated::ModelVectorToWorldVector(cS3DVector3&)"},
    {0x1001a3eacu, 128u, "cS3DModelInstanceAnimated::ModelVectorToWorldVector(float*)"},
    {0x1001a3f2cu, 182u, "cS3DModelInstanceAnimated::SetZoomAndRotation(int, int)"},
    {0x1001a3fe2u, 148u, "cS3DModelInstanceAnimated::SetFrameset(int)"},
    {0x1001a4076u, 194u, "cS3DModelInstanceAnimated::SetFrame(int)"},
    {0x1001a4164u, 38u, "cS3DModelInstanceAnimated::GetFrameCount() const"},
    {0x1001a418au, 322u, "cS3DModelInstanceAnimated::IncrementFrame()"},
    {0x1001a42ccu, 66u, "cS3DModelInstanceAnimated::SetAutoFrameRate(float)"},
    {0x1001a430eu, 302u, "cS3DModelInstanceAnimated::DoAutoFrameUpdate(float)"},
    {0x1001a445au, 16u, "cS3DModelInstanceAnimated::GetAnimInfo() const"},
    {0x1001a446au, 114u, "cS3DModelInstanceAnimated::SetOpacity(bool, float)"},
    {0x1001a44dcu, 72u, "cS3DModelInstanceAnimated::SetHighlight(int)"},
    {0x1001a4524u, 56u, "cS3DModelInstanceAnimated::SetSelfIlluminated(bool)"},
    // nS3D (17)
    {0x1001c60b8u, 129u, "nS3D::MakeBoxCorners(cS3DBoundingBox const&, cS3DVector3*)"},
    {0x1001c6139u, 623u, "nS3D::AddBoxMesh(cS3DMeshBuilder*, cS3DVector3*)"},
    {0x1002e944bu, 254u, "nS3D::TriangleDoubleAreaNormal(cS3DVector3 const&, cS3DVector3 const&, cS3DVector3 const&, cS3DVector3&)"},
    {0x1002e9549u, 196u, "nS3D::RandomDirection2(cRZRandom&)"},
    {0x1002e960du, 277u, "nS3D::RandomDirection3(cRZRandom&)"},
    {0x1002e9722u, 224u, "nS3D::AlignZToDirWithYUp(cS3DVector3 const&, cS3DMatrix3&)"},
    {0x1002e9802u, 237u, "nS3D::AlignXToDirWithYUp(cS3DVector3 const&, cS3DMatrix3&)"},
    {0x1002e98efu, 372u, "nS3D::AlignYToDir(cS3DVector3 const&, cS3DMatrix3&)"},
    {0x1002e9a63u, 226u, "nS3D::AlignDirToZWithYUp(cS3DVector3 const&, cS3DMatrix3&)"},
    {0x1002e9b45u, 235u, "nS3D::AlignDirToXWithYUp(cS3DVector3 const&, cS3DMatrix3&)"},
    {0x10032aac0u, 460u, "nS3D::CreateTextureFromBuffer(cIGZBuffer*, cS3DTextureBinding**, cRZRect const*)"},
    {0x10032acc8u, 961u, "nS3D::CreateTextureFromText(char const*, cS3DTextureBinding**, cIGZFont*, cRZPoint*, cS3DRect2D*, bool)"},
    {0x10032b09du, 683u, "nS3D::CreateCompressedTextureFromTextBuffer(cIGZBuffer*, cS3DTextureBinding**)"},
    {0x1003f4940u, 48u, "nS3D::S3DBlockTag::S3DBlockTag(float)"},
    {0x1003f7d10u, 32u, "nS3D::S3DBlockTag::S3DBlockTag(float)"},
    {0x1003f7e90u, 32u, "nS3D::S3DBlockTag::operator float() const"},
    {0x1003f8150u, 32u, "nS3D::sqrt(float)"},
    // cSTESelectedTerrainArea (17)
    {0x100743c50u, 138u, "cSTESelectedTerrainArea::AddTexturedSelection(int, int, cSTESelectedTerrainArea::tTexturedSelectionInfo&)"},
    {0x100743db2u, 214u, "cSTESelectedTerrainArea::AssignTextureCoordinates(sVF_V3F_T2F*, cSTESelectedTerrainArea::tTexturedSelectionInfo const&)"},
    {0x100743e88u, 86u, "cSTESelectedTerrainArea::ClearCurrentSelections()"},
    {0x100743edeu, 718u, "cSTESelectedTerrainArea::InitRenderState(bool)"},
    {0x1007441acu, 156u, "cSTESelectedTerrainArea::cSTESelectedTerrainArea()"},
    {0x100744248u, 272u, "cSTESelectedTerrainArea::DetermineQuadCoordinates(sVF_V3F*, int)"},
    {0x100744358u, 272u, "cSTESelectedTerrainArea::DetermineQuadCoordinates(sVF_V3F_T2F*, int)"},
    {0x100744468u, 1020u, "cSTESelectedTerrainArea::DrawTexturedSelections(SC4DrawContext*)"},
    {0x100744864u, 630u, "cSTESelectedTerrainArea::DrawUntexturedSelections(SC4DrawContext*)"},
    {0x100744adau, 244u, "cSTESelectedTerrainArea::Init(cSTETerrain*)"},
    {0x100744bceu, 154u, "cSTESelectedTerrainArea::MarkSelected(std::__1::vector<SC4Point<int>, std::__1::allocator<SC4Point<int>>>&, cISTETerrain::eHilightColorType, bool)"},
    {0x100744c68u, 200u, "cSTESelectedTerrainArea::MarkSelected(SC4Rect<int>&, cISTETerrain::eHilightColorType, bool)"},
    {0x100744d30u, 102u, "cSTESelectedTerrainArea::MarkSelected(SC4Point<int> const&, cISTETerrain::eHilightColorType, bool)"},
    {0x100744d96u, 168u, "cSTESelectedTerrainArea::MarkSelected(SC4Point<int> const&, cGZPersistResourceKey const&, unsigned char, bool, cISTETerrain::eHilightColorType, unsigned char, bool)"},
    {0x100744e3eu, 252u, "cSTESelectedTerrainArea::MarkSelected(SC4Point<int> const&, cGZPersistResourceKey const&, unsigned char, bool, unsigned int, cGDCombiner::tOp, bool)"},
    {0x100744f3au, 76u, "cSTESelectedTerrainArea::Shutdown()"},
    {0x100744f86u, 104u, "cSTESelectedTerrainArea::Update(SC4DrawContext*)"},
    // cSLTokenSerialRecordIO (15)
    {0x1004e8e3cu, 94u, "cSLTokenSerialRecordIO::ReadTokenDef(cGZPersistTokenDef&)"},
    {0x1004e8feau, 82u, "cSLTokenSerialRecordIO::Read(cRZString&)"},
    {0x1004e9064u, 88u, "cSLTokenSerialRecordIO::WriteTokenDef(cRZString&, cGZPersistTokenDef&)"},
    {0x1004e90bcu, 24u, "cSLTokenSerialRecordIO::Write(unsigned char&)"},
    {0x1004e90d4u, 24u, "cSLTokenSerialRecordIO::Write(signed char&)"},
    {0x1004e90ecu, 24u, "cSLTokenSerialRecordIO::Write(unsigned short&)"},
    {0x1004e9104u, 24u, "cSLTokenSerialRecordIO::Write(short&)"},
    {0x1004e911cu, 24u, "cSLTokenSerialRecordIO::Write(unsigned int&)"},
    {0x1004e9134u, 24u, "cSLTokenSerialRecordIO::Write(int&)"},
    {0x1004e914cu, 24u, "cSLTokenSerialRecordIO::Write(unsigned long long&)"},
    {0x1004e9164u, 24u, "cSLTokenSerialRecordIO::Write(long long&)"},
    {0x1004e917cu, 26u, "cSLTokenSerialRecordIO::Write(float&)"},
    {0x1004e9196u, 26u, "cSLTokenSerialRecordIO::Write(double&)"},
    {0x1004e91b0u, 22u, "cSLTokenSerialRecordIO::Write(cRZString&)"},
    {0x1004e91c6u, 22u, "cSLTokenSerialRecordIO::Write(cGZPersistResourceKey&)"},
    // cSC4ModelBasedParticlesEffect (14)
    {0x1005eda08u, 224u, "cSC4ModelBasedParticlesEffect::cSC4ModelBasedParticlesEffect()"},
    {0x1005edb1eu, 124u, "cSC4ModelBasedParticlesEffect::Shutdown()"},
    {0x1005edba8u, 56u, "cSC4ModelBasedParticlesEffect::Stop(cISC4VisualEffect::tTransitionType)"},
    {0x1005edc04u, 822u, "cSC4ModelBasedParticlesEffect::SetDescription(cSC4ParticlesDescription*)"},
    {0x1005edf3au, 84u, "cSC4ModelBasedParticlesEffect::SetChainSystem(cSC4ModelBasedParticlesEffect*)"},
    {0x1005edf8eu, 694u, "cSC4ModelBasedParticlesEffect::SetModel(unsigned int)"},
    {0x1005ee28cu, 311u, "cSC4ModelBasedParticlesEffect::SetModel(cIS3DModelInstance*)"},
    {0x1005ee41au, 2520u, "cSC4ModelBasedParticlesEffect::RenderActiveParticles(SC4DrawContext*)"},
    {0x1005eedf2u, 1018u, "cSC4ModelBasedParticlesEffect::ApplyFakePerspective(float, SC4ViewUtilities*, cSC4MBParticle*, cS3DVector3&, float&)"},
    {0x1005ef1ecu, 3180u, "cSC4ModelBasedParticlesEffect::ProcessModelParticle(cSC4MBParticle*, float, float, cS3DVector3&, cS3DTransform&)"},
    {0x1005efe58u, 1380u, "cSC4ModelBasedParticlesEffect::UpdateActiveParticles()"},
    {0x1005f03bcu, 452u, "cSC4ModelBasedParticlesEffect::CreateParticle()"},
    {0x1005f058eu, 584u, "cSC4ModelBasedParticlesEffect::DestroyParticle(cSCParticle*)"},
    {0x1005f07d6u, 942u, "cSC4ModelBasedParticlesEffect::HandOffParticleToChainSystem(cSC4MBParticle*)"},
    // cSC4GraphsViewWinProc (13)
    {0x1002dfbf6u, 166u, "cSC4GraphsViewWinProc::cSC4GraphsViewWinProc(cIGZWin*)"},
    {0x1002dfdc0u, 294u, "cSC4GraphsViewWinProc::Shutdown()"},
    {0x1002dffdau, 652u, "cSC4GraphsViewWinProc::Init()"},
    {0x1002e0266u, 219u, "cSC4GraphsViewWinProc::SetTimeScale(unsigned int)"},
    {0x1002e0358u, 448u, "cSC4GraphsViewWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x1002e0518u, 82u, "cSC4GraphsViewWinProc::SetExpanded(bool)"},
    {0x1002e056au, 286u, "cSC4GraphsViewWinProc::SelectOption(unsigned int)"},
    {0x1002e0688u, 5112u, "cSC4GraphsViewWinProc::ShowGraph(unsigned int)"},
    {0x1002e1a80u, 3876u, "cSC4GraphsViewWinProc::RefreshGraph()"},
    {0x1002e29b2u, 70u, "cSC4GraphsViewWinProc::DoMessage(cIGZMessage2*)"},
    {0x1002e2a7au, 88u, "cSC4GraphsViewWinProc::ShowWindow()"},
    {0x1002e2ad2u, 92u, "cSC4GraphsViewWinProc::HideWindow()"},
    {0x1002e2b2eu, 88u, "cSC4GraphsViewWinProc::ShowGraphType(unsigned int)"},
    // nsGZWinUtils (13)
    {0x1006a1f30u, 68u, "nsGZWinUtils::FindSiblingAs(cIGZWin*, unsigned int, unsigned int, void**)"},
    {0x1006a1f74u, 116u, "nsGZWinUtils::SetWindowVisible(cIGZWin*, unsigned int, bool, bool)"},
    {0x1006a1fe8u, 43u, "nsGZWinUtils::GetWindowVisible(cIGZWin*, unsigned int)"},
    {0x1006a2013u, 55u, "nsGZWinUtils::SetWindowEnabled(cIGZWin*, unsigned int, bool)"},
    {0x1006a204au, 65u, "nsGZWinUtils::SetWindowCaption(cIGZWin*, unsigned int, cIGZString const&)"},
    {0x1006a208bu, 51u, "nsGZWinUtils::SetWindowNotificationTarget(cIGZWin*, unsigned int, cIGZWin*)"},
    {0x1006a20beu, 35u, "nsGZWinUtils::SetWindowNotificationTargetRecursive(cIGZWin*, cIGZWin*)"},
    {0x1006a211fu, 62u, "nsGZWinUtils::SetWindowNotificationTargetRecursive(cIGZWin*, unsigned int, cIGZWin*)"},
    {0x1006a215du, 142u, "nsGZWinUtils::SetButtonToggleState(cIGZWin*, unsigned int, bool)"},
    {0x1006a21ebu, 109u, "nsGZWinUtils::GetButtonToggleState(cIGZWin*, unsigned int)"},
    {0x1006a2258u, 79u, "nsGZWinUtils::ExcludeButtonByRange(cIGZWin*, unsigned int, unsigned int, unsigned int)"},
    {0x1006a22a7u, 140u, "nsGZWinUtils::SetEditText(cIGZWin*, unsigned int, cIGZString const&)"},
    {0x1006a2333u, 122u, "nsGZWinUtils::SetSpinnerValue(cIGZWin*, unsigned int, int)"},
    // MacDisplay (12)
    {0x10063ec18u, 49u, "MacDisplay::SwapContext(OpaqueContextRef*)"},
    {0x10063ec49u, 2795u, "MacDisplay::Initialize()"},
    {0x10063f734u, 417u, "MacDisplay::SetupDisplay(unsigned char, int, int)"},
    {0x10063f8d5u, 50u, "MacDisplay::ReleaseDisplay()"},
    {0x10063f955u, 76u, "MacDisplay::PtInDisplay(Point)"},
    {0x10063f9a1u, 230u, "MacDisplay::GetCurrentMode(int&, int&, int&)"},
    {0x10063fa87u, 169u, "MacDisplay::SetMode(int, int, int)"},
    {0x10063fb30u, 30u, "MacDisplay::GetNumModes()"},
    {0x10063fb99u, 293u, "MacDisplay::CreateScreenContext(int, unsigned char&)"},
    {0x10063fcbeu, 65u, "MacDisplay::ReleaseContext(OpaqueContextRef*&)"},
    {0x10063fd16u, 100u, "MacDisplay::GetVideoMemoryInfo(unsigned int&, unsigned int&)"},
    {0x10063fd7au, 102u, "MacDisplay::GetClientRect(HWND__*, tagRECT*)"},
    // SC4QuadGrid (11)
    {0x100366e7au, 94u, "SC4QuadGrid::SC4QuadGrid(float, float)"},
    {0x100366ed8u, 180u, "SC4QuadGrid::SetSize(float, float)"},
    {0x100366fd4u, 90u, "SC4QuadGrid::RemoveAllElements()"},
    {0x100367038u, 270u, "SC4QuadGrid::AddElement(SC4GridElement*, unsigned int)"},
    {0x100367146u, 228u, "SC4QuadGrid::RemoveElement(SC4GridElement*)"},
    {0x10036722au, 192u, "SC4QuadGrid::MoveElement(SC4GridElement*, SC4Rect<float> const&, unsigned int)"},
    {0x1003672eau, 352u, "SC4QuadGrid::GetAllElements(SC4GridElement**, int&)"},
    {0x10036744au, 172u, "SC4QuadGrid::GetQuadrantsOfScreenRect(SC4Rect<float> const&, SC4QuadGridQuadrant**, int&)"},
    {0x1003674f6u, 200u, "SC4QuadGrid::GetQuadrantsOfScreenRect(SC4Rect<float> const&, int&, int&, int&, int&)"},
    {0x1003675beu, 142u, "SC4QuadGrid::SetGridRectForElement(SC4Rect<float> const&, SC4GridElement*)"},
    {0x10036764cu, 152u, "SC4QuadGrid::SetQuadrantFlagsForElement(SC4GridElement const*, unsigned int)"},
    // cSC4WinTrendBar (11)
    {0x1004653c0u, 220u, "cSC4WinTrendBar::cSC4WinTrendBar()"},
    {0x100465682u, 112u, "cSC4WinTrendBar::Init()"},
    {0x100465704u, 116u, "cSC4WinTrendBar::Shutdown()"},
    {0x100465786u, 204u, "cSC4WinTrendBar::SetArtwork(cIGZBuffer*, cIGZBuffer*)"},
    {0x100465852u, 356u, "cSC4WinTrendBar::SetDataSource(unsigned int, float, float)"},
    {0x1004659b6u, 152u, "cSC4WinTrendBar::UpdateValue(float)"},
    {0x100465a4eu, 202u, "cSC4WinTrendBar::SetDataSource(unsigned int, cSC4ResponseCurve const&)"},
    {0x100465b18u, 194u, "cSC4WinTrendBar::DoMessage(cIGZMessage2*)"},
    {0x100465bdau, 184u, "cSC4WinTrendBar::UpdateTrend(float)"},
    {0x100465c92u, 146u, "cSC4WinTrendBar::InterpolateValue(float)"},
    {0x100465d38u, 906u, "cSC4WinTrendBar::GZPaint()"},
    // cSC4DynamicParticleSystem (11)
    {0x1006fd300u, 278u, "cSC4DynamicParticleSystem::cSC4DynamicParticleSystem()"},
    {0x1006fd5c6u, 100u, "cSC4DynamicParticleSystem::Init()"},
    {0x1006fd62au, 84u, "cSC4DynamicParticleSystem::Shutdown()"},
    {0x1006fd67eu, 60u, "cSC4DynamicParticleSystem::DoMessage(cIGZMessage2*)"},
    {0x1006fd6bau, 1468u, "cSC4DynamicParticleSystem::DoAnimationTick(unsigned int)"},
    {0x1006fdc84u, 1948u, "cSC4DynamicParticleSystem::ActivateSectors(int, int, int)"},
    {0x1006fe420u, 232u, "cSC4DynamicParticleSystem::DeactivateSectors(int, int, int)"},
    {0x1006fe508u, 248u, "cSC4DynamicParticleSystem::Update(SC4DrawContext*)"},
    {0x1006fe600u, 444u, "cSC4DynamicParticleSystem::AddParticle(cSC4RigidBodySimulator2D::ObjectDesc const&, cIS3DModelInstance*)"},
    {0x1006fe894u, 118u, "cSC4DynamicParticleSystem::DeleteParticle(cSC4DynamicParticle*)"},
    {0x1006fe94cu, 156u, "cSC4DynamicParticleSystem::LinkParticles(unsigned int, unsigned int)"},
    // cSC4WinRegionScroll (9)
    {0x100234e56u, 192u, "cSC4WinRegionScroll::cSC4WinRegionScroll()"},
    {0x100234facu, 84u, "cSC4WinRegionScroll::Init()"},
    {0x100235000u, 64u, "cSC4WinRegionScroll::Shutdown()"},
    {0x100235040u, 116u, "cSC4WinRegionScroll::GZOnMouseDownR(int, int, unsigned int)"},
    {0x1002350b4u, 594u, "cSC4WinRegionScroll::SetMouseScroll(int, int)"},
    {0x100235306u, 108u, "cSC4WinRegionScroll::GZOnMouseUpR(int, int, unsigned int)"},
    {0x100235372u, 82u, "cSC4WinRegionScroll::SetCursorByID(unsigned int)"},
    {0x1002353c4u, 282u, "cSC4WinRegionScroll::GZOnMouseMove(int, int, unsigned int)"},
    {0x1002354deu, 994u, "cSC4WinRegionScroll::GZPaint()"},
    // cSLWinLocationSaver (9)
    {0x1002a649au, 50u, "cSLWinLocationSaver::cSLWinLocationSaver()"},
    {0x1002a6594u, 168u, "cSLWinLocationSaver::Init()"},
    {0x1002a663cu, 226u, "cSLWinLocationSaver::Shutdown()"},
    {0x1002a671eu, 122u, "cSLWinLocationSaver::SaveWindowLocation(cIGZWin*)"},
    {0x1002a6798u, 58u, "cSLWinLocationSaver::SaveWindowLocation(unsigned int, int, int)"},
    {0x1002a688au, 102u, "cSLWinLocationSaver::GetWindowRestoreLocation(cIGZWin*, int&, int&, bool)"},
    {0x1002a68f0u, 78u, "cSLWinLocationSaver::GetWindowRestoreLocation(unsigned int, int&, int&, bool)"},
    {0x1002a693eu, 254u, "cSLWinLocationSaver::RestoreWindowLocation(cIGZWin*, bool)"},
    {0x1002a6a3cu, 66u, "cSLWinLocationSaver::RestoreWindowLocation(unsigned int, bool)"},
    // cGZPropertySetT (9)
    {0x100472b20u, 160u, "cGZPropertySetT<cKeyStandardTraitsT<unsigned int, 3954248925ul, 1806839612ul>>::SetValue(unsigned int const&, cIGZAny const&)"},
    {0x1004737a4u, 336u, "cGZPropertySetT<cKeyStandardTraitsT<unsigned int, 3954248925ul, 1806839612ul>>::operator=(cGZPropertySetT<cKeyStandardTraitsT<unsigned int, 3954248925ul, 1806839612ul>> const&)"},
    {0x100481b00u, 96u, "cGZPropertySetT<cKeyStandardTraitsT<unsigned int, 3954248925ul, 1806839612ul>>::ClearValue(unsigned int const&)"},
    {0x100481b60u, 136u, "cGZPropertySetT<cKeyStandardTraitsT<unsigned int, 3954248925ul, 1806839612ul>>::ClearAll()"},
    {0x100484ad0u, 112u, "cGZPropertySetT<cKeyStandardTraitsT<unsigned int, 3954248925ul, 1806839612ul>>::NextKeyAndValue(unsigned int*, cIGZAny**)"},
    {0x1004c2a94u, 60u, "cGZPropertySetT<cKeyStandardTraitsT<unsigned int, 3954248925ul, 1806839612ul>>::FirstKeyAndValue(unsigned int*, cIGZAny**)"},
    {0x1004c2ad0u, 80u, "cGZPropertySetT<cKeyStandardTraitsT<unsigned int, 3954248925ul, 1806839612ul>>::Value(unsigned int const&, cIGZAny*) const"},
    {0x1004c2b20u, 68u, "cGZPropertySetT<cKeyStandardTraitsT<unsigned int, 3954248925ul, 1806839612ul>>::HasValue(unsigned int const&) const"},
    {0x1004c2b64u, 380u, "cGZPropertySetT<cKeyStandardTraitsT<unsigned int, 3954248925ul, 1806839612ul>>::InsertNewValue(unsigned int const&, cIGZAny const&)"},
    // cSC4WinResizableOutline (8)
    {0x100371674u, 446u, "cSC4WinResizableOutline::Init()"},
    {0x100371832u, 52u, "cSC4WinResizableOutline::Shutdown()"},
    {0x100371866u, 106u, "cSC4WinResizableOutline::GZOnMouseDownL(int, int, unsigned int)"},
    {0x1003718d0u, 338u, "cSC4WinResizableOutline::GetResizeTypeAndCursorForPosition(int, int, cSC4WinResizableOutline::ResizeType&, unsigned int&)"},
    {0x100371a22u, 64u, "cSC4WinResizableOutline::GZOnMouseUpL(int, int, unsigned int)"},
    {0x100371a62u, 566u, "cSC4WinResizableOutline::GZOnMouseMove(int, int, unsigned int)"},
    {0x100371c98u, 608u, "cSC4WinResizableOutline::GZPaint()"},
    {0x100371ef8u, 146u, "cSC4WinResizableOutline::SetResizingAndMoving(bool, bool)"},
    // cSCExemplarPropertyHolder (7)
    {0x10004ab48u, 56u, "cSCExemplarPropertyHolder::cSCExemplarPropertyHolder()"},
    {0x10004acccu, 68u, "cSCExemplarPropertyHolder::HasProperty(unsigned int) const"},
    {0x10004ad10u, 304u, "cSCExemplarPropertyHolder::GetPropertyList(cIGZUnknownList**) const"},
    {0x10004ae40u, 66u, "cSCExemplarPropertyHolder::GetProperty(unsigned int) const"},
    {0x10004ae82u, 290u, "cSCExemplarPropertyHolder::EnumProperties(void (*)(cISCProperty*, void*), void*) const"},
    {0x10004afa4u, 334u, "cSCExemplarPropertyHolder::EnumProperties(bool (*)(cISCProperty*, void*), void (*)(cISCProperty*, void*), void*) const"},
    {0x1000b35b0u, 80u, "cSCExemplarPropertyHolder::SetDefaultExemplar(cISCResExemplar*)"},
    // cSC4ViewInputControlFlora (7)
    {0x1003b6e6eu, 62u, "cSC4ViewInputControlFlora::cSC4ViewInputControlFlora(unsigned int)"},
    {0x1003b6eacu, 120u, "cSC4ViewInputControlFlora::Init()"},
    {0x1003b6f24u, 74u, "cSC4ViewInputControlFlora::Shutdown()"},
    {0x1003b6f6eu, 14u, "cSC4ViewInputControlFlora::OnKeyDown(unsigned int, unsigned int)"},
    {0x1003b6f7cu, 544u, "cSC4ViewInputControlFlora::OnMouseDownL(int, int, unsigned int)"},
    {0x1003b719cu, 76u, "cSC4ViewInputControlFlora::OnMouseUpL(int, int, unsigned int)"},
    {0x1003b71e8u, 592u, "cSC4ViewInputControlFlora::OnMouseMove(int, int, unsigned int)"},
    // cSC4ViewInputControlSignLabelDemolish (7)
    {0x1006c9a8cu, 42u, "cSC4ViewInputControlSignLabelDemolish::cSC4ViewInputControlSignLabelDemolish()"},
    {0x1006c9ab6u, 202u, "cSC4ViewInputControlSignLabelDemolish::Init()"},
    {0x1006c9b80u, 56u, "cSC4ViewInputControlSignLabelDemolish::Shutdown()"},
    {0x1006c9bb8u, 132u, "cSC4ViewInputControlSignLabelDemolish::OnMouseUpL(int, int, unsigned int)"},
    {0x1006c9c3cu, 34u, "cSC4ViewInputControlSignLabelDemolish::OnMouseMove(int, int, unsigned int)"},
    {0x1006c9c5eu, 470u, "cSC4ViewInputControlSignLabelDemolish::SetSelection(cISC4PropOccupant*)"},
    {0x1006c9e34u, 416u, "cSC4ViewInputControlSignLabelDemolish::PickSignOrLabel(int, int)"},
    // MacOpenGL (6)
    {0x100638f58u, 82u, "MacOpenGL::OpenGLTextureFormat::GetImageSizeInBytes(unsigned int, unsigned int) const"},
    {0x10063cd9fu, 64u, "MacOpenGL::IsVertexArraySupported()"},
    {0x10063cdf8u, 79u, "MacOpenGL::OpenGLTextureFormat::GetExternalFormatPixelSizeInBytes() const"},
    {0x10063ce47u, 3028u, "MacOpenGL::InitializeOpenGL(int, unsigned int, unsigned int, unsigned int, unsigned int, IDirectDraw7*, IDirect3DDevice7*&, IDirectDrawSurface7*&, IDirectDrawSurface7*&, IDirectDrawSurface7*&)"},
    {0x10063da1bu, 148u, "MacOpenGL::ShutdownOpenGL(IDirect3DDevice7*&, IDirectDrawSurface7*&, IDirectDrawSurface7*&, IDirectDrawSurface7*&)"},
    {0x10063daafu, 243u, "MacOpenGL::GetOpenGLFormat(_DDPIXELFORMAT const&)"},
    // cS3DMeshBuilder (5)
    {0x1001c5c4cu, 54u, "cS3DMeshBuilder::cS3DMeshBuilder()"},
    {0x1001c5c8cu, 56u, "cS3DMeshBuilder::Clear()"},
    {0x1001c5cc4u, 312u, "cS3DMeshBuilder::Begin(eGDPrimType)"},
    {0x1001c5dfcu, 376u, "cS3DMeshBuilder::P(cS3DVector3 const&)"},
    {0x1001c5feau, 206u, "cS3DMeshBuilder::Mesh()"},
    // cSC4AudioEmanationModel3DEffects (5)
    {0x1003f390cu, 84u, "cSC4AudioEmanationModel3DEffects::Init()"},
    {0x1003f396eu, 34u, "cSC4AudioEmanationModel3DEffects::Shutdown()"},
    {0x1003f39b2u, 834u, "cSC4AudioEmanationModel3DEffects::UpdateVolPan(cIGZSndRegisterSet*, cIGZSndRegisterSet*) const"},
    {0x1003f3cf4u, 522u, "cSC4AudioEmanationModel3DEffects::ConfigureChannel(cIGZSndRegisterSet*, cIGZSndRegisterSet*) const"},
    {0x1003f48dau, 28u, "cSC4AudioEmanationModel3DEffects::GetName(cIGZString&) const"},
    // cHitPropertySetTrackResource (5)
    {0x10046ff0cu, 4730u, "cHitPropertySetTrackResource::ReadLegacyFormat(cIGZIStream&, unsigned int, cIGZUnknown*)"},
    {0x1004711c8u, 3290u, "cHitPropertySetTrackResource::Write_OldFormat(cIGZOStream&, unsigned int, cIGZUnknown*)"},
    {0x100471ea2u, 64u, "cHitPropertySetTrackResource::Write_XmlFormat(cIGZOStream&, unsigned int, cIGZUnknown*)"},
    {0x100473042u, 164u, "cHitPropertySetTrackResource::CreatePrivateCopy(unsigned int, void**)"},
    {0x1004c2d94u, 112u, "cHitPropertySetTrackResource::cHitPropertySetTrackResource(cGZPersistResourceKey const&)"},
    // cSCUint8ComboBoxEntryView (5)
    {0x100526308u, 108u, "cSCUint8ComboBoxEntryView::Create()"},
    {0x100526374u, 474u, "cSCUint8ComboBoxEntryView::Init()"},
    {0x10052654eu, 228u, "cSCUint8ComboBoxEntryView::populateIndexValue()"},
    {0x100526632u, 596u, "cSCUint8ComboBoxEntryView::createComboBox(cIGZWinCombo*&, cRZVariant const&)"},
    {0x1005268a6u, 358u, "cSCUint8ComboBoxEntryView::GetValue(cIGZVariant&)"},
    // CQTPlayer (5)
    {0x100723c76u, 228u, "CQTPlayer::Close()"},
    {0x100723d64u, 1446u, "CQTPlayer::Open(char const*)"},
    {0x100724454u, 436u, "CQTPlayer::Play(unsigned char)"},
    {0x100724608u, 22u, "CQTPlayer::Stop()"},
    {0x10072461eu, 44u, "CQTPlayer::IsPlaying() const"},
    // cRZStackAllocator (4)
    {0x1000ab7cau, 72u, "cRZStackAllocator<unsigned int, 64, 1>::New(unsigned int)"},
    {0x1000b4eb0u, 72u, "cRZStackAllocator<cIS3DModelInstance*, 32, 1>::New(unsigned int)"},
    {0x10015ed68u, 72u, "cRZStackAllocator<cIS3DModelInstance*, 8, 1>::New(unsigned int)"},
    {0x1003c1950u, 72u, "cRZStackAllocator<cIS3DModelInstance*, 10, 1>::New(unsigned int)"},
    // tControllerQInfo (4)
    {0x1000f6436u, 56u, "tControllerQInfo::Clear()"},
    {0x1000f7efeu, 890u, "tControllerQInfo::InitQueuePoint(cISC4Occupant*)"},
    {0x1000f9064u, 150u, "tControllerQInfo::AddToQueue(cISC4Automaton*)"},
    {0x1000f9144u, 158u, "tControllerQInfo::RemoveFromQueue(cISC4Automaton*)"},
    // cSC4QueryDialogWinProc (4)
    {0x1002db86eu, 152u, "cSC4QueryDialogWinProc::cSC4QueryDialogWinProc(cIGZWinGen*)"},
    {0x1002dba4eu, 66u, "cSC4QueryDialogWinProc::SetOccupant(cISC4Occupant*)"},
    {0x1002dba90u, 606u, "cSC4QueryDialogWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x1002dbceeu, 124u, "cSC4QueryDialogWinProc::EndDialog(unsigned int)"},
    // cHitManCOMDirector (4)
    {0x10043adc2u, 279u, "cHitManCOMDirector::cHitManCOMDirector()"},
    {0x10043b11cu, 212u, "cHitManCOMDirector::OnStart(cIGZCOM*)"},
    {0x10043b22au, 348u, "cHitManCOMDirector::PreAppInit()"},
    {0x10043b386u, 198u, "cHitManCOMDirector::PreAppShutdown()"},
    // AnyString (4)
    {0x10047328eu, 328u, "AnyString::AnyString(char const*)"},
    {0x10047354eu, 28u, "AnyString::CompareTo(cIGZAny const&) const"},
    {0x10047356au, 32u, "AnyString::CompareTo(void const*) const"},
    {0x10047358au, 300u, "AnyString::AnyString(cIGZString const&)"},
    // cSC4WinAuraBar (4)
    {0x1005c4194u, 152u, "cSC4WinAuraBar::cSC4WinAuraBar()"},
    {0x1005c43b2u, 356u, "cSC4WinAuraBar::GZPaint()"},
    {0x1005c4516u, 86u, "cSC4WinAuraBar::SetImage(cIGZBuffer*)"},
    {0x1005c457eu, 52u, "cSC4WinAuraBar::SetValue(double)"},
    // cS3DModelInstanceBasic (3)
    {0x1000d9296u, 70u, "cS3DModelInstanceBasic::cS3DModelInstanceBasic()"},
    {0x1000d9340u, 56u, "cS3DModelInstanceBasic::SetOwner(cIGZUnknown*)"},
    {0x1000d93fau, 14u, "cS3DModelInstanceBasic::GetBounds(cS3DBoundingBox&)"},
    // nsTexUtil (3)
    {0x1002ff2ddu, 733u, "nsTexUtil::SetupTexture(cIGZBuffer*, cIGZGDriver*, unsigned int&, bool, cRZRect const*, unsigned int*, unsigned int*)"},
    {0x1002ff5ceu, 40u, "nsTexUtil::ReleaseTexture(cIGZGDriver*, unsigned int)"},
    {0x1002ff5f6u, 570u, "nsTexUtil::TextureResourceToGZBuffer(cGZPersistResourceKey&, cIGZBuffer*&)"},
    // cGroup_SC4RH (3)
    {0x10040fdaau, 132u, "cGroup_SC4RH::Clear()"},
    {0x10041027eu, 80u, "cGroup_SC4RH::RemoveInstance(cInstance*)"},
    {0x10041062eu, 78u, "cGroup_SC4RH::AddInstance(cInstance*)"},
    // cS3DFSHTextureFactory (3)
    {0x1004662feu, 20u, "cS3DFSHTextureFactory::CreateInstance(unsigned int, unsigned int, void**)"},
    {0x100466312u, 124u, "cS3DFSHTextureFactory::CreateInstance(unsigned int, unsigned int, void**, unsigned int, cIGZUnknown*)"},
    {0x10046638eu, 934u, "cS3DFSHTextureFactory::SerialRead(cIGZPersistResource&, cIGZPersistDBSerialRecord&)"},
    // cHitSem3dDefault (3)
    {0x1004c2706u, 204u, "cHitSem3dDefault::cHitSem3dDefault()"},
    {0x1004d176eu, 612u, "cHitSem3dDefault::UpdateVolPan(cIGZSndRegisterSet*, cIGZSndRegisterSet*) const"},
    {0x1004d19d2u, 476u, "cHitSem3dDefault::ConfigureChannel(cIGZSndRegisterSet*, cIGZSndRegisterSet*) const"},
    // cSC4EffectsResourceFactory (3)
    {0x10056bde8u, 156u, "cSC4EffectsResourceFactory::CreateInstance(unsigned int, unsigned int, void**)"},
    {0x10056be84u, 155u, "cSC4EffectsResourceFactory::SerialRead(cIGZPersistResource&, cIGZPersistDBSerialRecord&)"},
    {0x10056bf66u, 155u, "cSC4EffectsResourceFactory::SerialWrite(cIGZPersistResource&, cIGZPersistDBSerialRecord&)"},
    // cSTEWaterWave (3)
    {0x100758250u, 218u, "cSTEWaterWave::AddProducer(float, float, float)"},
    {0x10075832au, 406u, "cSTEWaterWave::ComputeWaveBoundRectForProducer(cSTEWaterWave::tWaveProducer&)"},
    {0x100758572u, 118u, "cSTEWaterWave::Shutdown()"},
    // SC4CellRegion (2)
    {0x1000ae73cu, 706u, "SC4CellRegion<int>::Inflate(SC4CellRegion<int>::NeighborType)"},
    {0x1007084f2u, 758u, "SC4CellRegion<int>::SetRectInclusion(SC4Rect<int> const&, bool)"},
    // cSC4SubwayTool (2)
    {0x100192b10u, 494u, "cSC4SubwayTool::CreateNetworkOccupant(cSC4NetworkCellInfo*, cSC4UndergroundOccupant*&, float (*) [3], unsigned int)"},
    {0x100192cfeu, 66u, "cSC4SubwayTool::cSC4SubwayTool()"},
    // cAmbienceFilterIsNight (2)
    {0x1003def66u, 38u, "cAmbienceFilterIsNight::Create()"},
    {0x1003e48a0u, 22u, "cAmbienceFilterIsNight::IsTrue()"},
    // cAmbienceFilterIsZoom2 (2)
    {0x1003deffeu, 38u, "cAmbienceFilterIsZoom2::Create()"},
    {0x1003e4958u, 46u, "cAmbienceFilterIsZoom2::IsTrue()"},
    // cAmbienceFilterIsSurfaceView (2)
    {0x1003df0bcu, 38u, "cAmbienceFilterIsSurfaceView::Create()"},
    {0x1003e49fcu, 32u, "cAmbienceFilterIsSurfaceView::IsTrue()"},
    // cAmbienceFilterIsSimSpeedFast (2)
    {0x1003df154u, 38u, "cAmbienceFilterIsSimSpeedFast::Create()"},
    {0x1003e4a90u, 32u, "cAmbienceFilterIsSimSpeedFast::IsTrue()"},
    // cAmbienceCellFilterIsRail (2)
    {0x1003df812u, 50u, "cAmbienceCellFilterIsRail::Create()"},
    {0x1003e5e5au, 116u, "cAmbienceCellFilterIsRail::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsResidential (2)
    {0x1003df8dau, 50u, "cAmbienceCellFilterIsResidential::Create()"},
    {0x1003e6020u, 94u, "cAmbienceCellFilterIsResidential::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsSeaport (2)
    {0x1003df9a2u, 50u, "cAmbienceCellFilterIsSeaport::Create()"},
    {0x1003e6196u, 92u, "cAmbienceCellFilterIsSeaport::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsMediumGrowthStage (2)
    {0x1003dfa9cu, 50u, "cAmbienceCellFilterIsMediumGrowthStage::Create()"},
    {0x1003e6572u, 272u, "cAmbienceCellFilterIsMediumGrowthStage::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsLowCrime (2)
    {0x1003dfb64u, 50u, "cAmbienceCellFilterIsLowCrime::Create()"},
    {0x1003e6862u, 110u, "cAmbienceCellFilterIsLowCrime::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsUnzoned (2)
    {0x1003dfc2cu, 50u, "cAmbienceCellFilterIsUnzoned::Create()"},
    {0x1003e5c32u, 90u, "cAmbienceCellFilterIsUnzoned::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsDistressed (2)
    {0x1003dfcf4u, 50u, "cAmbienceCellFilterIsDistressed::Create()"},
    {0x1003e6b4au, 154u, "cAmbienceCellFilterIsDistressed::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsAbandoned (2)
    {0x1003dfdbcu, 50u, "cAmbienceCellFilterIsAbandoned::Create()"},
    {0x1003e6f20u, 114u, "cAmbienceCellFilterIsAbandoned::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsHighWealth (2)
    {0x1003dfe84u, 66u, "cAmbienceCellFilterIsHighWealth::Create()"},
    {0x1003e6400u, 114u, "cAmbienceCellFilterIsHighWealth::IsTrueCell(int, int)"},
    // cSC4WinGenTransparent (2)
    {0x1004640a6u, 46u, "cSC4WinGenTransparent::cSC4WinGenTransparent()"},
    {0x10046413cu, 226u, "cSC4WinGenTransparent::IsPointInWindowWindowCoordinates(int, int) const"},
    // SpiralIterator (2)
    {0x1004f45e0u, 176u, "SpiralIterator::GetNext(int&, int&)"},
    {0x1004fe1f0u, 484u, "SpiralIterator::DoXZEnd()"},
    // cSCUint32TextEntryView (2)
    {0x100524388u, 412u, "cSCUint32TextEntryView::Init()"},
    {0x100524544u, 450u, "cSCUint32TextEntryView::GetValue(cIGZVariant&)"},
    // cSCFloat32TextEntryView (2)
    {0x1005255eeu, 416u, "cSCFloat32TextEntryView::Init()"},
    {0x1005257aeu, 472u, "cSCFloat32TextEntryView::GetValue(cIGZVariant&)"},
    // cSC4DecalDescription (2)
    {0x10056c540u, 320u, "cSC4DecalDescription::cSC4DecalDescription()"},
    {0x10057af84u, 292u, "cSC4DecalDescription::cSC4DecalDescription(cSC4DecalDescription const&)"},
    // cSC4ScrubberDescription (2)
    {0x100573a6cu, 130u, "cSC4ScrubberDescription::cSC4ScrubberDescription()"},
    {0x100573ba8u, 72u, "cSC4ScrubberDescription::Instantiate()"},
    // FastATIVertexProgram (2)
    {0x100637704u, 720u, "FastATIVertexProgram::Init()"},
    {0x1006379d4u, 110u, "FastATIVertexProgram::Shutdown()"},
    // cSTEEdgeReconcileInformMismatch (2)
    {0x100726bbcu, 544u, "cSTEEdgeReconcileInformMismatch::DoDialog()"},
    {0x100726ddcu, 130u, "cSTEEdgeReconcileInformMismatch::DoMessage(cIGZWin*, cGZMessage&)"},
    // SC4AppPreferences (1)
    {0x1000138c8u, 116u, "SC4AppPreferences::SC4AppPreferences()"},
    // cS3DMaterial (1)
    {0x1000d979eu, 66u, "cS3DMaterial::cS3DMaterial()"},
    // tHtRange (1)
    {0x1001bda76u, 94u, "tHtRange::Intersect(tHtRange const&, bool&)"},
    // CompareCitiesForExport (1)
    {0x10022aa14u, 262u, "CompareCitiesForExport::CompareCitiesForExport(CompareCitiesForExport const&)"},
    // cSC4LegendDialogWinProc (1)
    {0x1002c1bfau, 464u, "cSC4LegendDialogWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    // MiscGraphRect (1)
    {0x10037c092u, 138u, "MiscGraphRect::DrawSelf(cIGZGraph*, cIGZBuffer*)"},
    // cAmbienceFilterIsAlways (1)
    {0x1003df04au, 38u, "cAmbienceFilterIsAlways::Create()"},
    // iset (1)
    {0x100481580u, 288u, "iset<unsigned int, std::__1::less<unsigned int>>::iset(iset<unsigned int, std::__1::less<unsigned int>> const&)"},
    // TVNode (1)
    {0x10054a436u, 310u, "TVNode::TVNode(TVNode const&)"},
    // cSC4EffectsResource (1)
    {0x10056a246u, 182u, "cSC4EffectsResource::cSC4EffectsResource()"},
    // cSC4EffectsModelProcessor (1)
    {0x1005ed5e4u, 1060u, "cSC4EffectsModelProcessor::DeformModel(float)"},
    // CTextureStage (1)
    {0x100637a42u, 320u, "CTextureStage::SetTexture(OpenGLTextureState&, IDirectDrawSurface7*)"},
    // MacLogFile (1)
    {0x100641ad1u, 35u, "MacLogFile::WriteMessage(char const*)"},
    // cDumpSubTreeVisitor (1)
    {0x1006c1710u, 650u, "cDumpSubTreeVisitor::sVisit(unsigned long, void*)"},
    // cS3DMatrix4 (1)
    {0x1007749c0u, 272u, "cS3DMatrix4::operator*=(cS3DMatrix4 const&)"},
}};

[[nodiscard]] inline constexpr Uint32 Wave4Part4MethodCount() noexcept {
    return static_cast<Uint32>(kWave4Part4Methods.size());
}

[[nodiscard]] inline constexpr const Wave4Part4Method* FindWave4Part4Method(Uint64 address) noexcept {
    for (const Wave4Part4Method& method : kWave4Part4Methods) {
        if (method.address == address) {
            return &method;
        }
    }
    return nullptr;
}

static_assert(kWave4Part4Methods.size() == 808);
static_assert(Wave4Part4MethodCount() == 808u);
static_assert(FindWave4Part4Method(0x10075f3e6u) != nullptr);
static_assert(FindWave4Part4Method(0x1007749c0u) != nullptr);
static_assert(FindWave4Part4Method(0x10075f3e6u)->sizeBytes == 142u);
static_assert(FindWave4Part4Method(0x1007749c0u)->name[0] != '\0');
static_assert(kWave4Part4Methods[0].address == 0x10075f3e6u);
static_assert(kWave4Part4Methods[807].address == 0x1007749c0u);

// ---------------------------------------------------------------------------
// Phase 4 free functions
// 421 macOS x86_64 __TEXT,__text helpers with no owning class. Addresses are
// image VAs from the unstripped SimCity 4 Deluxe binary. The routines below
// are the constexpr forms of the pure index and string helpers in this list.
// ---------------------------------------------------------------------------

// GetCharLength @ 0x10001b75a with code page -2. One UTF-8 sequence when the
// lead byte is 0xC0..0xFD and the next byte is a continuation; otherwise one
// byte. Callers compare the returned width against a byte budget.
[[nodiscard]] inline constexpr std::size_t SC4EncodedCharBytes(std::string_view bytes) noexcept {
    if (bytes.empty()) {
        return 0;
    }
    const auto lead = static_cast<unsigned char>(bytes[0]);
    if (lead < 0xC0u || lead > 0xFDu || bytes.size() < 2) {
        return 1;
    }
    const auto next = static_cast<unsigned char>(bytes[1]);
    if (next < 0x80u || next > 0xBFu) {
        return 1;
    }
    if (lead < 0xE0u) {
        return 2;
    }
    if (lead < 0xF0u) {
        return 3;
    }
    if (lead < 0xF8u) {
        return 4;
    }
    return lead < 0xFCu ? 5u : 6u;
}

// SC4LimitStringLength @ 0x100039404 and @ 0x100039659. Walks encoded
// characters and keeps the longest prefix whose byte length is <= maxLen.
[[nodiscard]] inline constexpr std::string_view SC4LimitStringLength(
    std::string_view str, std::size_t maxLen) noexcept {
    const std::size_t len = str.size();
    std::size_t offset = 0;
    while (offset < len) {
        const std::size_t width = SC4EncodedCharBytes(str.substr(offset));
        const std::size_t end = offset + width;
        if (end <= maxLen) {
            offset = end;
        }
        if (offset >= len || end > maxLen) {
            break;
        }
    }
    return offset >= len ? str : str.substr(0, offset);
}

// CRC-24/OpenPGP table entry. cRZHashStringCRCInitializer @ 0x100368154 fills
// gCRC24Table with polynomial 0x01864CFB, eight non-reflected bits per index.
[[nodiscard]] inline constexpr Uint32 SC4Crc24TableEntry(Uint32 index) noexcept {
    Uint32 crc = index << 16;
    for (int bit = 0; bit < 8; ++bit) {
        const Uint32 shifted = crc << 1;
        crc = (shifted & 0x01000000u) != 0u ? (shifted ^ 0x01864CFBu) : shifted;
    }
    return crc;
}

// nRZHash::CRC24 @ 0x1003681dc, case folding off. Init 0x00B704CE.
[[nodiscard]] inline constexpr Uint32 SC4Crc24(std::string_view str) noexcept {
    Uint32 crc = 0x00B704CEu;
    for (const char ch : str) {
        const auto byte = static_cast<unsigned char>(ch);
        const Uint32 index = ((crc >> 16) ^ byte) & 0xFFu;
        crc = (crc << 8) ^ SC4Crc24TableEntry(index);
    }
    return crc & 0x00FFFFFFu;
}

// SC4GetResourceIDFromString @ 0x10003a532. 21 bytes: CRC24(str) | 0xFF000000.
// Named instance ids use this group byte; the body does not parse digits.
[[nodiscard]] inline constexpr Uint32 SC4GetResourceIDFromString(std::string_view str) noexcept {
    return SC4Crc24(str) | 0xFF000000u;
}

// SC4ZoomAndRotationToIndex @ 0x100039ca4: lea eax, [rotation + zoom*4].
[[nodiscard]] inline constexpr int SC4ZoomAndRotationToIndex(int zoom, int rotation) noexcept {
    return zoom * 4 + rotation;
}

// SC4SanitizeFileName @ 0x1000394a1. MakeFileNameValid drops the filename
// rejects, caps a long result at 256 bytes on a character boundary, then the
// caller collapses "  " and trims space and tab.
[[nodiscard]] inline constexpr bool SC4IsInvalidFileNameChar(char ch) noexcept {
    switch (ch) {
    case '/':
    case '\\':
    case ':':
    case '*':
    case '?':
    case '"':
    case '<':
    case '>':
    case '|':
        return true;
    default:
        return false;
    }
}

[[nodiscard]] inline constexpr std::string SC4SanitizeFileName(std::string_view input) {
    constexpr std::size_t kMaxFileNameBytes = 256;
    std::string filtered;
    filtered.reserve(input.size());
    for (const char ch : input) {
        if (!SC4IsInvalidFileNameChar(ch)) {
            filtered.push_back(ch);
        }
    }
    const std::size_t kept = SC4LimitStringLength(filtered, kMaxFileNameBytes).size();
    filtered.resize(kept);

    std::string collapsed;
    collapsed.reserve(filtered.size());
    for (const char ch : filtered) {
        if (ch == ' ' && !collapsed.empty() && collapsed.back() == ' ') {
            continue;
        }
        collapsed.push_back(ch);
    }

    std::size_t begin = 0;
    std::size_t end = collapsed.size();
    while (begin < end && (collapsed[begin] == ' ' || collapsed[begin] == '\t')) {
        ++begin;
    }
    while (end > begin && (collapsed[end - 1] == ' ' || collapsed[end - 1] == '\t')) {
        --end;
    }
    return collapsed.substr(begin, end - begin);
}

// SC4MakeMoneyString @ 0x1000392b0 passes the Simoleon sign to
// cRZLanguageUtility::MakeMoneyString. The default locale form is a grouped
// integer, "§" before the digits, and '-' before the sign when negative.
[[nodiscard]] inline constexpr std::string SC4MakeMoneyString(Sint64 amount) {
    const bool negative = amount < 0;
    Uint64 magnitude = negative
        ? static_cast<Uint64>(-(amount + 1)) + 1u
        : static_cast<Uint64>(amount);

    char reversed[20];
    std::size_t count = 0;
    do {
        reversed[count++] = static_cast<char>('0' + static_cast<int>(magnitude % 10u));
        magnitude /= 10u;
    } while (magnitude != 0u);

    std::string out;
    out.reserve(count + count / 3u + 4u);
    if (negative) {
        out.push_back('-');
    }
    out += "\u00A7";
    while (count > 0) {
        --count;
        out.push_back(reversed[count]);
        if (count > 0 && (count % 3u) == 0u) {
            out.push_back(',');
        }
    }
    return out;
}

[[maybe_unused]] inline constexpr std::array<Batch3EntryPoint, 421> kPhase4FreeFunctions = {{
    {0x100003930u, "SC4AppStart(cRZCmdLine const&, bool)"},
    {0x100038d80u, "SC4SetupFPU()"},
    {0x100038d8eu, "SC4GetPopupModalDialogsSafe()"},
    {0x100038e6bu, "SC4GetPopupModalDialogsEnabled()"},
    {0x100038e8cu, "SC4SetPopupModalDialogsEnabled(bool)"},
    {0x100038eb5u, "SC4FlushInputMessageQueue()"},
    {0x100038facu, "SC4GameDelay(unsigned int, bool)"},
    {0x10003903eu, "SC4GamePause(bool, bool, bool, bool, bool)"},
    {0x1000392b0u, "SC4MakeMoneyString(long long, cIGZString&)"},
    {0x100039355u, "SC4MakeFormattedMoneyString(double, cIGZString&, cIGZString const&)"},
    {0x100039404u, "SC4LimitStringLength(cIGZString&, unsigned int)"},
    {0x100039486u, "SC4DisableOSMessageBoxAlerts()"},
    {0x10003948cu, "SC4OSMessageBoxAlert(cIGZString const&, cIGZString const&)"},
    {0x1000394a1u, "SC4SanitizeFileName(cIGZString const&, cIGZString&)"},
    {0x100039659u, "SC4LimitStringLength(cIGZString const&, cIGZString&, unsigned int)"},
    {0x10003972cu, "SC4GetUserDataDirectory(cIGZString&, bool)"},
    {0x1000399fau, "SC4GetDesktopDirectory(cIGZString&)"},
    {0x100039a97u, "SC4GetSystemFontsDirectory(cIGZString&)"},
    {0x100039a9fu, "SC4CreateWindowFromScript(cGZPersistResourceKey const&, cIGZWin*, unsigned int)"},
    {0x100039b38u, "SC4CreateWindowFromScript(unsigned int, cIGZWin*, unsigned int, unsigned int, void**)"},
    {0x100039bf7u, "SC4CreateWindowFromScript(cGZPersistResourceKey const&, cIGZWin*, unsigned int, unsigned int, void**)"},
    {0x100039ca4u, "SC4ZoomAndRotationToIndex(int, int)"},
    {0x100039cb4u, "SC4DoPopupStringListMenu(cIGZWin*, int, int, std::__1::vector<cRZString, std::__1::allocator<cRZString>> const&, cRZString*, unsigned int)"},
    {0x10003a244u, "SC4TakeSnapshot(int, int, unsigned int, unsigned int)"},
    {0x10003a2b8u, "SC4TakeSnapshot(cIGZString const&, int, int, unsigned int, unsigned int)"},
    {0x10003a31cu, "SC4GetExemplarProperty(unsigned int, unsigned int, unsigned int)"},
    {0x10003a3d2u, "SC4GetResourceAsFile(cGZPersistResourceKey const&, cIGZFile**, bool)"},
    {0x10003a532u, "SC4GetResourceIDFromString(char const*)"},
    {0x10003a547u, "SC4GetInstalledLanguageAndCountryIDs(unsigned int&, unsigned int&)"},
    {0x10003a555u, "SC4SetCPUUsageCount(int)"},
    {0x10003a565u, "SC4ViewExternalWebBrowser(cRZString const&, bool)"},
    {0x10003a5bau, "SC4PostMessage2(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, cIGZUnknown*)"},
    {0x10003a5e7u, "SC4BroadcastMessage2Inl(bool, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, cIGZUnknown*)"},
    {0x10003a70au, "SC4SendMessage2(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, cIGZUnknown*)"},
    {0x10003a742u, "SC4SendProgressMessage(unsigned int, cIGZString const&)"},
    {0x10003a8a8u, "SC4DisplayResourceError(unsigned int)"},
    {0x10004135bu, "GZCOM_cRZMessage2StandardCLSID()"},
    {0x1000432feu, "SC4ReadGZPersistRef(cISC4DBSegmentIStream&, unsigned int, void**)"},
    {0x100043388u, "SC4ReadGZPersistRef(cIGZIStream&, unsigned int, void**)"},
    {0x100043417u, "SC4WriteGZPersistRef(cISC4DBSegmentOStream&, cIGZUnknown const*)"},
    {0x1000434ffu, "SC4WriteGZPersistRef(cIGZOStream&, cIGZUnknown const*)"},
    {0x100047732u, "GZCOM_cSC4CityCLSID()"},
    {0x10005319eu, "GZCOM_cGZMessageQueueManager2CLSID()"},
    {0x100053a24u, "GZCOM_cGZMessageServer2CLSID()"},
    {0x10005f0c8u, "GZCOM_cSC4TutorialSystemCLSID()"},
    {0x1000609dfu, "GZCOM_cSC4LandfillDeveloperCLSID()"},
    {0x10006e612u, "GZCOM_cSC4NeighborsSimulatorCLSID()"},
    {0x100070529u, "void sReportDealStatus<cISC4NeighborDeal>(bool, int, cISC4NeighborDeal*)"},
    {0x1000747cau, "GZCOM_cSC4NeighborConnectionCLSID()"},
    {0x10007514bu, "GZCOM_cSC4NeighborDealCLSID()"},
    {0x1000753a9u, "void sReportDealStatus<cSC4NeighborDeal>(bool, int, cSC4NeighborDeal*)"},
    {0x1000767ccu, "GZCOM_cSC4TrafficSimulatorCLSID()"},
    {0x100094692u, "GZCOM_cSC4PollutionSimulatorCLSID()"},
    {0x10009bc92u, "GZCOM_cSC4PlumbingSimulatorCLSID()"},
    {0x1000a2372u, "GZCOM_cSC4PoliceSimulatorCLSID()"},
    {0x1000a5cd5u, "GZCOM_cSC4OccupantFilterCLSID()"},
    {0x1000a5f49u, "GZCOM_cSC4DefaultOccupantFilterCLSID()"},
    {0x1000a6198u, "GZCOM_cSC4OccupantTypeFilterCLSID()"},
    {0x1000a62ebu, "GZCOM_cSC4OccupantTypesFilterCLSID()"},
    {0x1000a650cu, "GZCOM_cSC4OccupantTypesFilterHashedCLSID()"},
    {0x1000a68ebu, "GZCOM_cSC4RealisticOccupantFilterCLSID()"},
    {0x1000a6c32u, "GZCOM_cSC4NetworkOccupantFilterCLSID()"},
    {0x1000a6dceu, "GZCOM_cSC4ViewLevelOccupantFilterCLSID()"},
    {0x1000a6df8u, "SC4GetOccupantViewLevel(cISC4Occupant*)"},
    {0x1000a715eu, "GZCOM_cSC4StringSpecificationOccupantFilterCLSID()"},
    {0x1000b09c0u, "SC4AdjustPathSeparatorsForPlatform(cRZString const&, cRZString&)"},
    {0x1000b2ecbu, "GZCOM_cSC4LandfillOccupantCLSID()"},
    {0x1000b379bu, "GZCOM_cSC4LotBaseTextureOccupantCLSID()"},
    {0x1000b4ef8u, "cRZAutoRefCount<cISC4CellTextureModel> GZCOM_Interface<cIS3DModelInstance, cISC4CellTextureModel>(cIS3DModelInstance const*, unsigned int, cISC4CellTextureModel*)"},
    {0x1000bc5b1u, "GZCOM_cSC4OccupantCLSID()"},
    {0x1000bd5dau, "GZCOM_cSC4FloraSimulatorCLSID()"},
    {0x1000c2a62u, "GZCOM_cSC4FlammabilitySimulatorCLSID()"},
    {0x1000c4f02u, "GZCOM_cSC4LandValueSimulatorCLSID()"},
    {0x1000c80d3u, "GZCOM_cSC4AirportDeveloperCLSID()"},
    {0x1000c8f72u, "GZCOM_cSC4ParkManagerCLSID()"},
    {0x1000ca012u, "GZCOM_cSC4SeaportDeveloperCLSID()"},
    {0x1000cb414u, "cRZAutoRefCount<cISC4PropOccupant> GZCOM_Interface<cISC4Occupant, cISC4PropOccupant>(cISC4Occupant const*, unsigned int, cISC4PropOccupant*)"},
    {0x1000cb77bu, "GZCOM_cSC4FloraOccupantCLSID()"},
    {0x1000cf51fu, "operator<<(cIGZOStream&, cS3DVector2 const&)"},
    {0x1000cf54eu, "operator>>(cIGZIStream&, cS3DVector3&)"},
    {0x1000cf586u, "operator<<(cIGZOStream&, cS3DVector3 const&)"},
    {0x1000cf5c4u, "operator>>(cIGZIStream&, cS3DBoundingBox&)"},
    {0x1000cf623u, "operator<<(cIGZOStream&, cS3DBoundingBox const&)"},
    {0x1000cf68eu, "operator>>(cIGZIStream&, cS3DMatrix4&)"},
    {0x1000cf6d5u, "operator<<(cIGZOStream&, cS3DMatrix4 const&)"},
    {0x1000cfa93u, "S3DTransformLegacySave(cIGZOStream&, cS3DTransform const&)"},
    {0x1000cfbd7u, "operator<<(cIGZOStream&, ilist<cISC4Occupant*> const&)"},
    {0x1000cfc24u, "operator>>(cIGZIStream&, cSC4ResponseCurve&)"},
    {0x1000cfd0cu, "operator<<(cIGZOStream&, cSC4ResponseCurve const&)"},
    {0x1000d8656u, "SC4FloodFillDefaultTestFunction(int, int, int, void*)"},
    {0x1000e11dfu, "CanDriveOnCell(int, int, cISC4LotManager*, unsigned int, unsigned int, float, float)"},
    {0x1000e3202u, "GZCOM_cSC4WeatherSimulatorCLSID()"},
    {0x1000e629au, "MakeWaterAreaList(cISTETerrain*, std::__1::vector<cRZRect, std::__1::allocator<cRZRect>>&, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, cRZRect const*)"},
    {0x1000e8dd9u, "GZCOM_cSC4CivicBuildingSimulatorCLSID()"},
    {0x1000eaf58u, "GZCOM_cSC4DemolitionCLSID()"},
    {0x1000eee91u, "GZCOM_cSC4HistoryWarehouseCLSID()"},
    {0x1000f32ecu, "cRZAutoRefCount<cISC4DisasterManagerFire> GZCOM_Interface<cISC4DisasterManager, cISC4DisasterManagerFire>(cISC4DisasterManager const*, unsigned int, cISC4DisasterManagerFire*)"},
    {0x100120e10u, "SC4LoadString(cIGZString&, unsigned int, unsigned int, cISCStringDetokenizer*)"},
    {0x100121132u, "SC4LoadExemplarString(cIGZString&, cISCPropertyHolder*, unsigned int, cISCStringDetokenizer*)"},
    {0x100121184u, "SC4LoadExemplarString(cIGZString&, unsigned int, unsigned int, unsigned int, cISCStringDetokenizer*)"},
    {0x1001217c2u, "GZCOM_cSC4PropDeveloperCLSID()"},
    {0x1001261e6u, "GZCOM_cSC4WealthRequesterCLSID()"},
    {0x10012808cu, "GZCOM_cSC4ZoneDeveloperCLSID()"},
    {0x100130758u, "IsConnectable(cISC4NetworkOccupant*)"},
    {0x100134bd2u, "GZCOM_cSC4TractDeveloperCLSID()"},
    {0x10013eb12u, "GZCOM_cSC4LotDeveloperCLSID()"},
    {0x10014bdd0u, "GZCOM_cSC4BuildingDevelopmentSimulatorCLSID()"},
    {0x100156062u, "GZCOM_cSC4GrowthDeveloperCLSID()"},
    {0x10015a615u, "CompareDesirability(cISC4GrowthDeveloper::GrowthItem const&, cISC4GrowthDeveloper::GrowthItem const&)"},
    {0x10015a64eu, "CompareRandomize(cISC4GrowthDeveloper::GrowthItem const&, cISC4GrowthDeveloper::GrowthItem const&)"},
    {0x10015caebu, "GZCOM_cSC4LotRetainingWallOccupantCLSID()"},
    {0x10015d41fu, "cRZAutoRefCount<cISC4LotRetainingWallModel> GZCOM_Interface<cIS3DModelInstance, cISC4LotRetainingWallModel>(cIS3DModelInstance const*, unsigned int, cISC4LotRetainingWallModel*)"},
    {0x10015e451u, "GZCOM_cSC4PropPlacementPropertiesFactoryCLSID()"},
    {0x10015e4dbu, "GZCOM_cSC4PropOccupantCLSID()"},
    {0x10015f91fu, "GZCOM_cSC4PropOccupantTerrainDecalCLSID()"},
    {0x1001607d7u, "GZCOM_cSC4PropOccupantVisualEffectCLSID()"},
    {0x100161897u, "GZCOM_cSC4PropOccupantBurntBuildingCLSID()"},
    {0x100162597u, "GZCOM_cSC4PropOccupantAutomataControllerCLSID()"},
    {0x100163b08u, "GZCOM_cS3DCameraCLSID()"},
    {0x10016d9bdu, "GZCOM_cSC4FoundationOccupantCLSID()"},
    {0x10016dff2u, "GZCOM_cSC4NetworkManagerCLSID()"},
    {0x10017761bu, "GZCOM_cSC4ConstructionOccupantCLSID()"},
    {0x10017ed6bu, "GZCOM_cSC4LotCLSID()"},
    {0x100182e4cu, "cIGZOStream& operator<<<unsigned char>(cIGZOStream&, SC4Rect<unsigned char> const&)"},
    {0x1001835f2u, "GZCOM_cSC4BusinessUnitCLSID()"},
    {0x100184c52u, "GZCOM_cSC424HourClockCLSID()"},
    {0x10019383eu, "GZCOM_cSC4PropManagerCLSID()"},
    {0x1001971f9u, "GZCOM_cSC4CityDetailManagerCLSID()"},
    {0x100197e6fu, "AdjustNetworkLotCityDetail(unsigned int, unsigned int, cISC4NetworkLot*, void*)"},
    {0x100197e9du, "IncreaseCityDetailIterationFunction(cISC4Occupant*, void*)"},
    {0x1001980fcu, "GZCOM_cSC4ZoneManagerCLSID()"},
    {0x10019ae3bu, "GZCOM_cSC4BuildingOccupantCLSID()"},
    {0x10019be4bu, "GZCOM_cSC4NetworkOccupantCLSID()"},
    {0x1001a0f61u, "GZCOM_cSC4PipeOccupantCLSID()"},
    {0x1001a1637u, "GZCOM_cSC4SubwayOccupantCLSID()"},
    {0x1001a239bu, "GZCOM_cSC4NetworkTunnelOccupantCLSID()"},
    {0x1001a6ed4u, "GetSpriteAnimFromResourceKey(cGZPersistResourceKey const&, cIS3DModelInstanceAnimated**)"},
    {0x1001a6f4du, "GetMeshAnimFromResourceKey(cGZPersistResourceKey const&, cIS3DModelInstanceAnimated**)"},
    {0x1001a6fb7u, "GetAnimatedModelFromResourceKey(cGZPersistResourceKey const&, cIS3DModelInstanceAnimated**)"},
    {0x1001a92dfu, "IsStraight(cSC4NetworkCellInfo*, cISC4NetworkManager::eNetworkType)"},
    {0x1001a94aeu, "GZCOM_cSC4PipeCustomViewCLSID()"},
    {0x1001a953au, "GZCOM_cSC4SubwayCustomViewCLSID()"},
    {0x1001ba0dbu, "GZCOM_cSC4NetworkOccupantWithPreBuiltModelCLSID()"},
    {0x1001c2465u, "IsStepGoodForBridge(cISC4OccupantManager*, cISC4OccupantFilter*, int, int, int, int)"},
    {0x1001c398du, "AddSuspendedTileStructures(cISC4NetworkOccupant*, float (*) [3], float (*) [2], float, float, float, float, float, unsigned int, unsigned int)"},
    {0x1001c6bcbu, "AppendNetworkName(cRZString&, cISC4NetworkManager::eNetworkType)"},
    {0x1001d26bbu, "GZCOM_cSC4NetworkBridgeOccupantCLSID()"},
    {0x1001d2cf3u, "SC4ReadCityRegionViewThumbnails(cIGZString const&, cIGZBuffer**, cIGZBuffer**, cIGZBuffer**, cIGZBuffer**)"},
    {0x1001d2d56u, "SC4ReadCityRegionViewThumbnails(cIGZPersistDBSegment*, cIGZBuffer**, cIGZBuffer**, cIGZBuffer**, cIGZBuffer**)"},
    {0x1001d3609u, "SC4ReadCityRegionViewThumbnailBitmapFile(cIGZString const&, cIGZBuffer**)"},
    {0x1001d3671u, "SC4WriteCityRegionViewThumbnails(cIGZPersistDBSegment*, cIGZBuffer*, cIGZBuffer*, cIGZBuffer*, cIGZBuffer*)"},
    {0x1001d3c68u, "SC4WriteCityRegionViewThumbnail(cIGZPersistDBSegment*)"},
    {0x1001d4eccu, "SC4RenderRegionBorderMasks()"},
    {0x1001d541du, "SC4Make4PtAntialiasedImage(cIGZBuffer**)"},
    {0x1001d575du, "SC4CropImage(cIGZBuffer*, cRZRect const&)"},
    {0x1001d589au, "SC4WriteDefaultCityRegionViewThumbnail(cIGZPersistDBSegment*)"},
    {0x1001e9090u, "SC4OpenSaveGame(cIGZString const&, bool, bool)"},
    {0x1001e91b6u, "SC4CloseSaveGame(cIGZPersistDBSegment*)"},
    {0x1001e91f5u, "SC4ReadBinaryRecord(cIGZPersistDBSegment*, cGZPersistResourceKey const&, std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>&, unsigned int)"},
    {0x1001e927bu, "SC4ReadBinaryRecord(cIGZPersistDBSegment*, cGZPersistResourceKey const&, cIGZString const&)"},
    {0x1001e93c7u, "SC4ReadBinaryRecord(cGZPersistResourceKey const&, cIGZString const&)"},
    {0x1001e98a4u, "SC4CreateSavePathForCity(cIGZString const&, unsigned int, cRZString&)"},
    {0x1001e9aa9u, "SC4CreateSavePathForCity(cIGZString const&, cIGZString const&, unsigned int, cRZString&)"},
    {0x1001e9ff4u, "SC4CreateCitySerialNumber()"},
    {0x1001fcadcu, "GZCOM_cSC4CrimeSimulatorCLSID()"},
    {0x100200ab9u, "GZCOM_cSC4DemandCLSID()"},
    {0x100201582u, "GZCOM_cSC4ResidentialSimulatorCLSID()"},
    {0x10020e212u, "GZCOM_cSC4PowerSimulatorCLSID()"},
    {0x1002157aeu, "GZCOM_cSC4DispatchManagerCLSID()"},
    {0x100215cb1u, "operator>>(cIGZIStream&, cISC4DispatchManager::DispatchInfo&)"},
    {0x100218012u, "GZCOM_cSC4AuraSimulatorCLSID()"},
    {0x100219ed7u, "ReadCurveProperty(cISCPropertyHolder*, unsigned int, cSC4ResponseCurve**)"},
    {0x10021bf07u, "void ProcessTwoGridsForROnly<float>(cISC4SimGrid<signed char>*, cISC4SimGrid<float> const*, void (*)(signed char&, float, void*), void*)"},
    {0x10021c5b0u, "void ProcessTwoGridsForROnly<unsigned char>(cISC4SimGrid<signed char>*, cISC4SimGrid<unsigned char> const*, void (*)(signed char&, unsigned char, void*), void*)"},
    {0x10021dd62u, "GZCOM_cSC4BudgetSimulatorCLSID()"},
    {0x100223f5eu, "GZCOM_cSC4DemandSimulatorCLSID()"},
    {0x10022e0d2u, "GZCOM_cSC4PylonOccupantCLSID()"},
    {0x10023ce10u, "SC4MoveMaskToAlpha(cIGZBuffer*, cIGZBuffer*)"},
    {0x10023cf28u, "SC4CreateSpanList(cIGZBuffer*, int, std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&, bool)"},
    {0x10024351au, "GZCOM_cSC4DepartmentBudgetCLSID()"},
    {0x100245911u, "GZCOM_cSC4LineItemCLSID()"},
    {0x100246b84u, "GZCOM_cSC4OccupantManagerCLSID()"},
    {0x100247098u, "InitOccupant(cISC4Occupant*, void*)"},
    {0x10029503cu, "GZCOM_cSC4AdvisorSystemCLSID()"},
    {0x100296e6fu, "void sNotify<cISC4Advisor>(bool, int, cISC4Advisor*)"},
    {0x1002989b2u, "GZCOM_cSC4TrafficNetworkMapCLSID()"},
    {0x1002c94ceu, "WriteConnectionInfo(cISC4NeighborsSimulator::ConnectionType, int, int)"},
    {0x1002d3961u, "SafeGetAltitude(cISTETerrain*, float, float)"},
    {0x1002d92f8u, "ConvertConnectionTypeToCommodityType(cISC4NeighborsSimulator::ConnectionType)"},
    {0x1002fa4c2u, "GZCOM_cSC4FireProtectionSimulatorCLSID()"},
    {0x10030273eu, "InterpretEntry((anonymous namespace)::cExemplarReader&, cRZString const&, cGZPersistResourceKey&)"},
    {0x1003029aau, "InterpretEntry((anonymous namespace)::cExemplarReader&, cRZString const&, unsigned int&)"},
    {0x100302a9cu, "InterpretEntry((anonymous namespace)::cExemplarReader&, unsigned int&, cIGZVariant&)"},
    {0x100306552u, "GZCOM_cS3DTextureBindingFactoryCLSID()"},
    {0x100309380u, "GZCOM_cGZWinDebugDisplayCLSID()"},
    {0x10030a9cbu, "SC4GetChildWindowForCursorPosEnumProc(cIGZWin*, unsigned int, void*, void*)"},
    {0x10030c5f2u, "GZCOM_cSC4ModelMakerCLSID()"},
    {0x100314d5fu, "cRZAutoRefCount<cISC4BuildingOccupant> GZCOM_Interface<cISC4Occupant, cISC4BuildingOccupant>(cISC4Occupant const*, unsigned int, cISC4BuildingOccupant*)"},
    {0x100314dadu, "cRZAutoRefCount<cISC4FloraOccupant> GZCOM_Interface<cISC4Occupant, cISC4FloraOccupant>(cISC4Occupant const*, unsigned int, cISC4FloraOccupant*)"},
    {0x100315ddau, "s_sim_select_car(void*, cIGZVariant&, bool, cISC4GameDataRegistry*)"},
    {0x100315e46u, "s_sim_select_pedestrian(void*, cIGZVariant&, bool, cISC4GameDataRegistry*)"},
    {0x10031fc88u, "void Bezier3D4ControlsArray<cS3DVector3>(cS3DVector3&, cS3DVector3&, cS3DVector3&, cS3DVector3&, cS3DVector3*, int)"},
    {0x100320ee7u, "GZCOM_cSC4PowerPoleOccupantCLSID()"},
    {0x1003244cbu, "GZCOM_cSC4PowerLineOccupantCLSID()"},
    {0x10034cde2u, "SC4DoButtonExclusion(cIGZWin*, unsigned int, unsigned int, unsigned int)"},
    {0x10035008au, "SC4MapViewRotatedAlphaBlt(cIGZBuffer*, cIGZBuffer*, int, int, int)"},
    {0x1003511afu, "GZCOM_cSC4ComplexViewOccupantFilterCLSID()"},
    {0x100358485u, "SC4InsertNumberIntoString(cRZString&, long long)"},
    {0x100364ac2u, "GZCOM_cSC4AnimationTickManagerCLSID()"},
    {0x1003ad93cu, "GZCOM_cSC4RegionalCityCLSID()"},
    {0x1003b1f7eu, "OrderAlongEdge(cSC4RegionalCity::ConnectionChange const&, cSC4RegionalCity::ConnectionChange const&)"},
    {0x1003d6e82u, "cRZAutoRefCount<cIGZWinBtn> GZCOM_Interface<cIGZWin, cIGZWinBtn>(cIGZWin const*, unsigned int, cIGZWinBtn*)"},
    {0x1003d71efu, "cRZAutoRefCount<cIGZWinTextEdit> GZCOM_Interface<cIGZWin, cIGZWinTextEdit>(cIGZWin const*, unsigned int, cIGZWinTextEdit*)"},
    {0x1003d778fu, "SC4DisplayNewRegionDialog(cIGZWin*)"},
    {0x1003d7856u, "SC4DisplayLoadRegionDialog(cIGZWin*)"},
    {0x1003d791du, "SC4DisplayDeleteRegionDialog(cIGZWin*)"},
    {0x1003d7f01u, "cRZAutoRefCount<cIGZWinTreeView> GZCOM_Interface<cIGZWin, cIGZWinTreeView>(cIGZWin const*, unsigned int, cIGZWinTreeView*)"},
    {0x1003d8412u, "SC4DisplayImportCityDialog(cIGZWin*, int, int, int&, int&, int&, cRZString&)"},
    {0x1003d8598u, "SC4DisplayDeleteCityDialog(cIGZWin*)"},
    {0x1003d8d8cu, "cRZAutoRefCount<cIGZWinListBox> GZCOM_Interface<cIGZWin, cIGZWinListBox>(cIGZWin const*, unsigned int, cIGZWinListBox*)"},
    {0x1003ddedeu, "GZCOM_cSC4PipeConnectionOccupantCLSID()"},
    {0x1003de28eu, "GZCOM_cSC4SubwayConnectionOccupantCLSID()"},
    {0x1003e0091u, "GetSC4AudioCOMDirector()"},
    {0x1003eb218u, "IterateOccupantsOnTickCallback(cISC4Occupant*, void*)"},
    {0x1003ec544u, "IterateOccupantsOnInitCallback(cISC4Occupant*, void*)"},
    {0x1003ece2au, "IterateFireOccupantsOnInitCallback(cISC4Occupant*, void*)"},
    {0x1003f49b0u, "operator*(cS3DVector3 const&, float)"},
    {0x1003f7620u, "operator-(cS3DVector3 const&, cS3DVector3 const&)"},
    {0x1003f7700u, "cross(cS3DVector3 const&, cS3DVector3 const&)"},
    {0x1003f78f0u, "operator-(cS3DVector3 const&)"},
    {0x1003fb0a0u, "gAdviceDetokenizerFunction(cIGZString const&, cIGZString&, void*)"},
    {0x1003fbec0u, "gAdviceMessageBox(cIGZString const&, cIGZString const&)"},
    {0x100400a69u, "GZCOM_cSC4AdvisorCityPlanningCLSID()"},
    {0x100404dc2u, "GZCOM_cSC4NetworkLotManagerCLSID()"},
    {0x100411921u, "GZCOM_cGZBufferCLSID()"},
    {0x10042fc0bu, "GZCOM_cGZRealtimeDebugDisplayCLSID()"},
    {0x100435b50u, "SC4GetCOMDirector()"},
    {0x10043ce92u, "GZCOM_cSC4AdviceCLSID()"},
    {0x10043ecbau, "GZCOM_cSC4AdvisorUtilityCLSID()"},
    {0x100440562u, "gNeighborDealAdviceDetokenizerFunction(cIGZString const&, cIGZString&, void*)"},
    {0x1004411dcu, "GZCOM_cSC4AdviceNeghborDealCLSID()"},
    {0x100441936u, "GZCOM_cSC4AdvisorCLSID()"},
    {0x100442757u, "void sNotify<cSC4Advisor>(bool, int, cSC4Advisor*, int)"},
    {0x100443d8eu, "void sNotify<cISC4Advice>(bool, int, cISC4Advice*, int)"},
    {0x100444c90u, "GZCOM_cSC4AdvisorBaseCLSID()"},
    {0x100445a06u, "GZCOM_cSC4AdvisorHQCLSID()"},
    {0x1004479dcu, "GZCOM_cSC4AdvisorFinanceCLSID()"},
    {0x100447fa6u, "GZCOM_cSC4AdvisorTrafficCLSID()"},
    {0x100449ce1u, "GZCOM_cSC4AdvisorSafetyCLSID()"},
    {0x10044ba3du, "GZCOM_cSC4AdviceSubjectCLSID()"},
    {0x10044c15du, "GZCOM_cSC4DisasterManagerEffectCLSID()"},
    {0x10044cb7fu, "GZCOM_cSC4DisasterManagerRiotCLSID()"},
    {0x10044d4b5u, "GZCOM_cSC4DisasterInstanceRiotCLSID()"},
    {0x10044f020u, "GZCOM_cSC4DisasterManagerFireCLSID()"},
    {0x10044f6ddu, "GZCOM_cSC4DisasterInstanceFireCLSID()"},
    {0x100452bd6u, "GZCOM_cSC4DisasterManagerEarthquakeCLSID()"},
    {0x100452fdcu, "GZCOM_cSC4DisasterInstanceEarthquakeCLSID()"},
    {0x1004550d6u, "GZCOM_cSC4DisasterManagerPipeBurstCLSID()"},
    {0x1004558c7u, "GZCOM_cSC4DisasterInstancePipeBurstCLSID()"},
    {0x100456e1du, "GZCOM_cSC4DisasterManagerVolcanoCLSID()"},
    {0x100457369u, "GZCOM_cSC4DisasterInstanceVolcanoCLSID()"},
    {0x100458231u, "GZCOM_cSC4PathInfoFactoryCLSID()"},
    {0x10045cd4eu, "VerifyPathPoint(unsigned char, cS3DVector3&, float*, float*)"},
    {0x10045ebd4u, "GZCOM_cSC4FireOccupantCLSID()"},
    {0x10045f892u, "GetPointsOnModel(unsigned int, cISC4Occupant*, std::__1::vector<FirePoint, std::__1::allocator<FirePoint>>&)"},
    {0x100461a56u, "GZCOM_cSC4DisasterLayerCLSID()"},
    {0x100466111u, "GZCOM_cS3DFSHTextureFactoryCLSID()"},
    {0x1004667f1u, "GZCOM_cS3DResModelFactoryCLSID()"},
    {0x10046ce42u, "GZCOM_cSC4RewardAdviceCLSID()"},
    {0x1004d8f1cu, "GZCOM_cGZWinTestServiceCLSID()"},
    {0x1004db5b0u, "SC4AutomataGetCOMDirector()"},
    {0x100522d2cu, "GetSC4ResourceLibCOMDirector()"},
    {0x100566a28u, "GZCOM_cGZWinHTMLCLSID()"},
    {0x100569e68u, "GZCOM_cS3DMaterialManagerCLSID()"},
    {0x10056a112u, "GZCOM_cSC4EffectsResourceCLSID()"},
    {0x10056a16du, "GZCOM_cSC4EffectsResourceFactoryCLSID()"},
    {0x10056a178u, "operator>>(cIGZIStream&, cSC4ShakeDescription&)"},
    {0x10056a1e3u, "operator<<(cIGZOStream&, cSC4ShakeDescription const&)"},
    {0x10056b160u, "operator>>(cIGZIStream&, cSC4DecalDescription&)"},
    {0x10056bc62u, "operator<<(cIGZOStream&, cSC4DecalDescription const&)"},
    {0x10056bf1fu, "cRZAutoRefCount<cIGZIStream> GZCOM_Interface<cIGZPersistDBSerialRecord, cIGZIStream>(cIGZPersistDBSerialRecord const*, unsigned int, cIGZIStream*)"},
    {0x10056c001u, "cRZAutoRefCount<cIGZOStream> GZCOM_Interface<cIGZPersistDBSerialRecord, cIGZOStream>(cIGZPersistDBSerialRecord const*, unsigned int, cIGZOStream*)"},
    {0x100570bcau, "(anonymous namespace)::cOptionGroupCommand::EndBlock(cSC4RuleParser*)"},
    {0x10057163cu, "(anonymous namespace)::cVendorCommand::EndBlock(cSC4RuleParser*)"},
    {0x1005729b0u, "operator>>(cIGZIStream&, cSC4SequenceDescription::cItemRec&)"},
    {0x100574b78u, "cRZAutoRefCount<cISC4EffectMaps> GZCOM_Interface<cISC4EffectsManager, cISC4EffectMaps>(cISC4EffectsManager const*, unsigned int, cISC4EffectMaps*)"},
    {0x100574e38u, "(anonymous namespace)::cScrubberFilter::IsOccupantTypeIncluded(unsigned int)"},
    {0x100574edfu, "cRZAutoRefCount<cISC4SignpostOccupant> GZCOM_Interface<cISC4Occupant, cISC4SignpostOccupant>(cISC4Occupant const*, unsigned int, cISC4SignpostOccupant*)"},
    {0x100574f26u, "cRZAutoRefCount<cISC4TextDecalOccupant> GZCOM_Interface<cISC4Occupant, cISC4TextDecalOccupant>(cISC4Occupant const*, unsigned int, cISC4TextDecalOccupant*)"},
    {0x100574f6du, "cRZAutoRefCount<cISC4NetworkOccupant> GZCOM_Interface<cISC4Occupant, cISC4NetworkOccupant>(cISC4Occupant const*, unsigned int, cISC4NetworkOccupant*)"},
    {0x1005b596cu, "operator>>(cIGZIStream&, cSC4ParticlesDescriptionBase::cTractorPoint&)"},
    {0x1005b59aeu, "operator<<(cIGZOStream&, cSC4ParticlesDescriptionBase::cTractorPoint const&)"},
    {0x1005b59f1u, "operator>>(cIGZIStream&, cSC4ParticlesDescriptionBase::cTimedEffect&)"},
    {0x1005b5aabu, "operator>>(cIGZIStream&, cSC4ParticlesDescription&)"},
    {0x1005b6107u, "operator<<(cIGZOStream&, cSC4ParticlesDescription const&)"},
    {0x1005c2210u, "operator>>(cIGZIStream&, cSC4CameraParams&)"},
    {0x1005c2276u, "operator<<(cIGZOStream&, cSC4CameraParams const&)"},
    {0x1005c22d9u, "operator>>(cIGZIStream&, cSC4EffectDescription::cDescriptionRec&)"},
    {0x1005c2453u, "operator<<(cIGZOStream&, cSC4EffectDescription::cDescriptionRec const&)"},
    {0x1005c253eu, "operator>>(cIGZIStream&, cSC4EffectDescription::cEventRec&)"},
    {0x1005c261fu, "operator<<(cIGZOStream&, cSC4EffectDescription::cEventRec const&)"},
    {0x1005c266fu, "operator>>(cIGZIStream&, cSC4EffectDescription&)"},
    {0x1005c296fu, "operator<<(cIGZOStream&, cSC4EffectDescription const&)"},
    {0x1005c2a69u, "operator>>(cIGZIStream&, cSC4LightDescription&)"},
    {0x1005c2ae5u, "operator<<(cIGZOStream&, cSC4LightDescription const&)"},
    {0x1005c2b61u, "operator>>(cIGZIStream&, cSC4MessageTriggerDescription&)"},
    {0x1005c2c0eu, "operator<<(cIGZOStream&, cSC4MessageTriggerDescription const&)"},
    {0x1005c3331u, "GZCOM_cSC4WinTextCLSID()"},
    {0x1005c3cf8u, "GZCOM_cSC4WinAlertBorderCLSID()"},
    {0x1005c4188u, "GZCOM_cSC4WinAuraBarCLSID()"},
    {0x1005c4658u, "GZCOM_cSC4WinMiniMapCLSID()"},
    {0x1005c7902u, "GZCOM_cSC4WinAdviceListCLSID()"},
    {0x1005c90bcu, "GZCOM_cSC4EffectsParserCLSID()"},
    {0x1005ca01fu, "cRZAutoRefCount<cISC4EffectsCollection> GZCOM_Interface<cISC4EffectsManager, cISC4EffectsCollection>(cISC4EffectsManager const*, unsigned int, cISC4EffectsCollection*)"},
    {0x1005cbed2u, "(anonymous namespace)::cEffectGroupIDCommand::EndBlock(cSC4EffectsParser*)"},
    {0x1005cd3d7u, "cRZAutoRefCount<cISC4EffectsCollection> GZCOM_Interface<cISC4EffectsResource, cISC4EffectsCollection>(cISC4EffectsResource const*, unsigned int, cISC4EffectsCollection*)"},
    {0x1005d0df2u, "(anonymous namespace)::cGroupSystemSequenceCommand::EndBlock(cSC4EffectsParser*)"},
    {0x1005d64e2u, "(anonymous namespace)::cParticleTerrainRepelCommand::~cParticleTerrainRepelCommand()"},
    {0x1005d64e8u, "(anonymous namespace)::cParticleTerrainRepelCommand::~cParticleTerrainRepelCommand()"},
    {0x1005dad32u, "GZCOM_cSC4EffectsManagerCLSID()"},
    {0x1005dc25bu, "cRZAutoRefCount<cIGZGDriverVertexBufferExtension> GZCOM_Interface<cIGZGDriver, cIGZGDriverVertexBufferExtension>(cIGZGDriver const*, unsigned int, cIGZGDriverVertexBufferExtension*)"},
    {0x1005de2a0u, "InsertOccupantIterationFunction(cISC4Occupant*, void*)"},
    {0x1005e3337u, "IsStaticBuilding(cISC4Occupant*)"},
    {0x1005ee244u, "cRZAutoRefCount<cIS3DMeshAndMaterial> GZCOM_Interface<cIS3DModelInstance, cIS3DMeshAndMaterial>(cIS3DModelInstance const*, unsigned int, cIS3DMeshAndMaterial*)"},
    {0x1005ee3c3u, "cRZAutoRefCount<cIS3DModelInstanceAnimated> GZCOM_Interface<cIS3DModelInstance, cIS3DModelInstanceAnimated>(cIS3DModelInstance const*, unsigned int, cIS3DModelInstanceAnimated*)"},
    {0x1005f9fdcu, "(anonymous namespace)::cDestroyBuildingFilter::IsOccupantTypeIncluded(unsigned int)"},
    {0x100600e6cu, "GZCOM_nSCRes_CreatecFileParser()"},
    {0x100600eaeu, "GZCOM_nSCRes_cFileParserCLSID()"},
    {0x100603686u, "GZCOM_cSGLDriverDX7CLSID()"},
    {0x100628be0u, "operator+(cS3DVector3 const&, cS3DVector3 const&)"},
    {0x100628c90u, "dot(cS3DVector3 const&, cS3DVector3 const&)"},
    {0x100643fd2u, "GZCOM_cGZLuaScriptServerCLSID()"},
    {0x1006479dfu, "GZCOM_cGZWinLuaScriptConsoleCLSID()"},
    {0x100648ea4u, "GZCOM_cGZWinLuaScriptViewCLSID()"},
    {0x10064bb8bu, "GZCOM_cGZWinLuaScriptViewCallStackCLSID()"},
    {0x10064c12bu, "GZCOM_cGZWinLuaScriptViewVariablesCLSID()"},
    {0x10064c8cbu, "GZCOM_cGZWinLuaScriptViewBreakpointsCLSID()"},
    {0x10064ee0eu, "GZCOM_cGZDrawContextCLSID()"},
    {0x100652ea0u, "GZCOM_cGZDrawContextAsBufferCLSID()"},
    {0x1006572c2u, "GZCOM_cGZDrawFontRendererCLSID()"},
    {0x100659492u, "GZCOM_cGZDrawSystemCLSID()"},
    {0x1006a3062u, "RZDrawBltTiled(cIGZBuffer*, cIGZBuffer*, cRZRect const&, cRZRect const&, int, int, cRZRect const*)"},
    {0x1006a7c9du, "GZCOM_cSC4AdvisorCitySituationsCLSID()"},
    {0x1006a81e6u, "GZCOM_cSC4CitySituationAdviceCLSID()"},
    {0x1006aa0a8u, "GZCOM_cSC4MySimBalloonAdviceCLSID()"},
    {0x1006acb46u, "GZCOM_cSC4MySimDispatchCLSID()"},
    {0x1006b9ad8u, "GZCOM_cSC4MyStreetSimCLSID()"},
    {0x1006bf1c0u, "SC4GetSC4ScriptedWindowCOMDirector()"},
    {0x1006bf242u, "GetSC4GDriversCOMDirector()"},
    {0x1006c302eu, "GZCOM_cSC4LotConfigurationManagerCLSID()"},
    {0x1006fc2ddu, "GZCOM_cSC4DisasterManagerToxicCLSID()"},
    {0x1006fd0d8u, "operator>>(cIGZIStream&, cSC4DynamicParticleDescription&)"},
    {0x1006fd207u, "operator<<(cIGZOStream&, cSC4DynamicParticleDescription const&)"},
    {0x100709272u, "SC4CreateGZLog(cRZString const&, cRZString const&, bool, bool)"},
    {0x10070e93bu, "GZCOM_cSC4SignpostOccupantCLSID()"},
    {0x1007114bbu, "GZCOM_cSC4TextDecalOccupantCLSID()"},
    {0x1007129c9u, "GZCOM_cSC4NetworkLayoutCustomViewCLSID()"},
    {0x1007228bbu, "GetViewportPointIntersectionWithPlane(cISC4View3DWin*, int, int, float, float*)"},
    {0x100722ed0u, "GZCOM_cSC4FirePlaneDispatchSourceCLSID()"},
    {0x10072472au, "GZCOM_cSC4PoliceHelicopterDispatchSourceCLSID()"},
    {0x10072c5fcu, "cofactors(cS3DMatrix4 const&)"},
    {0x10072e104u, "GZCOM_cSTESurfaceWaterCLSID()"},
    {0x10072f63du, "cRZAutoRefCount<cISC4NetworkOccupant> GZCOM_Interface<cIGZUnknown, cISC4NetworkOccupant>(cIGZUnknown const*, unsigned int, cISC4NetworkOccupant*)"},
    {0x1007312f2u, "GZCOM_cSTETerrainCLSID()"},
    {0x100746086u, "GZCOM_cSTETerrainView3DCLSID()"},
    {0x100749684u, "operator*(cS3DMatrix4 const&, cS3DMatrix4 const&)"},
    {0x10075f3b2u, "GZCOM_cSC43DRenderCLSID()"},
    {0x10076aff2u, "GZCOM_cSC4CameraControlCLSID()"},
    {0x10076baebu, "IntersectWithGroundPlane(cS3DVector3 const&, cS3DVector3 const&, cS3DVector3&)"},
    {0x10076ce82u, "GZCOM_cSC4ConfigRuleManagerCLSID()"},
    {0x10076fec1u, "cRZAutoRefCount<cIGZGraphicSystem2> GZCOM_Interface<cIGZGraphicSystem, cIGZGraphicSystem2>(cIGZGraphicSystem const*, unsigned int, cIGZGraphicSystem2*)"},
    {0x10077505fu, "cRZAutoRefCount<cISC4Occupant> GZCOM_Interface<cIGZUnknown, cISC4Occupant>(cIGZUnknown const*, unsigned int, cISC4Occupant*)"},
    {0x100777646u, "HillToTerrainRect(int, int, int, SC4Rect<int> const&, SC4Rect<int>&)"},
    {0x100795b8bu, "GZCOM_cSC4OrdinanceCarpoolIncentiveCLSID()"},
    {0x100795c97u, "GZCOM_cSC4OrdinanceCleanAirCLSID()"},
    {0x100795da3u, "GZCOM_cSC4OrdinanceCommunityCPRTrainingCLSID()"},
    {0x100795ec8u, "GZCOM_cSC4OrdinanceFreeClinicsCLSID()"},
    {0x1007962c8u, "GZCOM_cSC4OrdinanceJuniorSportsLeagueCLSID()"},
    {0x1007967bdu, "GZCOM_cSC4OrdinanceLandfillGasRecoveryCLSID()"},
    {0x100796e7du, "GZCOM_cSC4OrdinanceLegalizeGamblingCLSID()"},
    {0x1007971d9u, "GZCOM_cSC4OrdinanceMandatorySmokeDetectorsCLSID()"},
    {0x1007972f8u, "GZCOM_cSC4OrdinanceNeighborhoodWatchCLSID()"},
    {0x10079767du, "GZCOM_cSC4OrdinanceNuclearFreeZoneCLSID()"},
    {0x100797789u, "GZCOM_cSC4OrdinancePaperReductionActCLSID()"},
    {0x100797895u, "GZCOM_cSC4OrdinancePowerConservationCLSID()"},
    {0x1007979b8u, "GZCOM_cSC4OrdinanceProReadingCampaignCLSID()"},
    {0x100797d7fu, "GZCOM_cSC4OrdinanceShuttleServiceCLSID()"},
    {0x100797e62u, "GZCOM_cSC4OrdinanceSimpleCLSID()"},
    {0x10079aa79u, "GZCOM_cSC4OrdinanceTireRecyclingCLSID()"},
    {0x10079ab98u, "GZCOM_cSC4OrdinanceTouristPromotionCLSID()"},
    {0x10079af5du, "GZCOM_cSC4OrdinanceTrashPresortCLSID()"},
    {0x10079b3afu, "GZCOM_cSC4OrdinanceVehicleEmissionStandardCLSID()"},
    {0x10079b4bbu, "GZCOM_cSC4OrdinanceWaterConservationCLSID()"},
    {0x10079b5c7u, "GZCOM_cSC4OrdinanceYouthCurfewCLSID()"},
    {0x10079eb07u, "GZCOM_cSC4MySimAdviceCLSID()"},
    {0x10079ebc0u, "GZCOM_cSC4MySimCLSID()"},
    {0x1007a1f87u, "s_sim_name_exch(void*, cIGZVariant&, bool, cISC4GameDataRegistry*)"},
    {0x1007a5be9u, "SetTableBuilding(cISCLua*, char const*, cISC4BuildingOccupant*)"},
    {0x1007a60fcu, "GetCongestion(cISC4TrafficSimulator*, int, int)"},
    {0x1007a61d0u, "SetTableOccupantSubject(cISCLua*, char const*, cISC4Occupant*)"},
    {0x1007a787bu, "SendMySimsChangedMessage()"},
    {0x1007ab822u, "GZCOM_cSC4MySimAgentSimulatorCLSID()"},
    {0x1007e937cu, "_FSHWpixel8To5"},
    {0x1007e93b2u, "_FSHWpixel8To565"},
    {0x1007e93dau, "_FSHWpixel8To655"},
    {0x1007e9603u, "_FSHWpixel8To4444"},
    {0x1007e969bu, "_FSHWpixel8To6666"},
    {0x1007e9735u, "_FSHWpixel8To2101010"},
    {0x1007eaca4u, "_FSH_about"},
    {0x1007ead85u, "_FSH_is"},
    {0x1007eae1eu, "_FSH_open"},
    {0x1007eaef8u, "_FSH_info"},
    {0x1007eb35cu, "_FSH_read"},
    {0x1007eb51cu, "_FSH_close"},
    {0x1007eb571u, "_FSH_wopen"},
    {0x1007eb603u, "_FSH_write"},
    {0x1007eb63au, "_FSH_wclose"},
    {0x1008aba40u, "_SDL_ShowSimpleMessageBox_REAL"},
    {0x1008d65f0u, "_SDL_ShowSimpleMessageBox"},
    {0x10090cd30u, "_TT_Process_Simple_Glyph"},
    {0x10090d3e0u, "_TT_Load_Simple_Glyph"}
}};

static_assert(kPhase4FreeFunctions.size() == 421);
static_assert(kPhase4FreeFunctions[0].address == 0x100003930u);
static_assert(kPhase4FreeFunctions[0].symbol[0] == 'S');
static_assert(kPhase4FreeFunctions[8].address == 0x1000392b0u);
static_assert(kPhase4FreeFunctions[21].address == 0x100039ca4u);
static_assert(kPhase4FreeFunctions[27].address == 0x10003a532u);
static_assert(kPhase4FreeFunctions[420].address == 0x10090d3e0u);
static_assert(kPhase4FreeFunctions[420].symbol[0] == '_');
static_assert(SC4ZoomAndRotationToIndex(0, 0) == 0);
static_assert(SC4ZoomAndRotationToIndex(3, 2) == 14);
static_assert(SC4ZoomAndRotationToIndex(1, 0) == 4);
static_assert(SC4GetResourceIDFromString("") == 0xFFB704CEu);
static_assert(SC4GetResourceIDFromString("a") == 0xFFF25713u);
static_assert(SC4GetResourceIDFromString("123456789") == 0xFF21CF02u);
static_assert(SC4Crc24("123456789") == 0x21CF02u);
static_assert(SC4LimitStringLength("abcd", 3) == "abc");
static_assert(SC4LimitStringLength("abcd", 0).empty());
static_assert(SC4LimitStringLength("abcd", 4) == "abcd");
static_assert(SC4LimitStringLength("\u00A7x", 1).empty());
static_assert(SC4LimitStringLength("\u00A7x", 2) == "\u00A7");
static_assert(SC4EncodedCharBytes("\u00A7") == 2u);
static_assert(SC4MakeMoneyString(0) == "\u00A70");
static_assert(SC4MakeMoneyString(1234) == "\u00A71,234");
static_assert(SC4MakeMoneyString(-1234) == "-\u00A71,234");
static_assert(SC4MakeMoneyString(1000000) == "\u00A71,000,000");
static_assert(SC4MakeMoneyString(INT64_MIN) == "-\u00A79,223,372,036,854,775,808");
static_assert(SC4SanitizeFileName("a/b:c\\d*e?\"f<g>h|i") == "abcdefghi");
static_assert(SC4SanitizeFileName("  a  b  ") == "a b");
static_assert(SC4SanitizeFileName("\t keep \t") == "keep");

} // namespace opensc4
