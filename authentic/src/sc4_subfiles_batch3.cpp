// Exact decompilation of the Batch 3 .sc4 save subfiles. See
// include/opensc4/sc4_subfiles_batch3.h for the binary address of every routine.

#include "opensc4/sc4_subfiles_batch3.h"
#include "opensc4/sc4_subfiles_automata.h"

#include <algorithm>

namespace opensc4 {
namespace {

using Ref = SimSerializableRef;

bool RRef(cIGZIStream& s, Ref& r) noexcept { return ReadSimSerializableRef(s, r); }
void WRef(cIGZOStream& s, const Ref& r) { WriteSimSerializableRef(s, r); }

bool CountOk(const cIGZIStream& s, Uint32 n, std::size_t minBytes) noexcept {
    return s.Good() && (minBytes == 0 || n <= s.Remaining() / minBytes);
}

void RKey(cIGZIStream& s, std::array<Uint32, 3>& k) noexcept { s >> k[0] >> k[1] >> k[2]; }
void WKey(cIGZOStream& s, const std::array<Uint32, 3>& k) { s << k[0] << k[1] << k[2]; }

} // namespace

// ===========================================================================
// cSC4Advice
// ===========================================================================
bool AdviceSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 4) {
        return false;
    }
    s >> fieldD8 >> fieldDC;
    for (Sint32& v : fieldC0) {
        s >> v;
    }
    list128.clear();
    if (version >= 4) {
        Uint32 n = 0;
        s >> n;
        if (!CountOk(s, n, 4)) {
            return false;
        }
        list128.assign(n, 0u);
        for (Uint32& v : list128) {
            s >> v;
        }
    }
    return s.Good() && RRef(s, subject110);
}
bool AdviceSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    s << ver << fieldD8 << fieldDC;
    for (Sint32 v : fieldC0) {
        s << v;
    }
    if (ver >= 4) {
        s << static_cast<Sint32>(list128.size());
        for (Uint32 v : list128) {
            s << v;
        }
    }
    WRef(s, subject110);
    return true;
}

bool AdviceNeghborDealSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 2 || !advice.Read(s)) {
        return false;
    }
    s >> field140;
    return s.Good();
}
bool AdviceNeghborDealSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    advice.Write(s);
    s << field140;
    return true;
}
bool AdviceNeghborDealSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    s << ver;
    advice.WriteWithVersion(s, advice.version);
    s << field140;
    return true;
}

bool AdviceSubjectSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 4) {
        return false;
    }
    s >> field20;
    if (field20 != 0) {
        s >> field24 >> field28;
    }
    s >> field2C >> field30;
    return s.Good() && RRef(s, ref38);
}
bool AdviceSubjectSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{4} << field20;
    if (field20 != 0) {
        s << field24 << field28;
    }
    s << field2C << field30;
    WRef(s, ref38);
    return true;
}

// ===========================================================================
// cSC4Advisor / cSC4AdvisorCityPlanning / cSC4AdvisorSystem
// ===========================================================================
bool AdvisorSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 3) {
        return false;
    }
    s >> field28;
    if (!s.ReadGZString(name30) || !s.ReadGZString(name60)) {
        return false;
    }
    for (Sint32& v : field90) {
        s >> v;
    }
    Sint32 n = 0;
    s >> n;
    if (!s.Good() || n < 0 || !CountOk(s, static_cast<Uint32>(n), 21)) {
        return false;
    }
    advice.assign(static_cast<std::size_t>(n), AdvisorAdviceInfo{});
    for (auto& a : advice) {
        if (!RRef(s, a.advice)) {
            return false;
        }
        s >> a.fieldC >> a.field10 >> a.field8 >> a.flag >> a.value;
    }
    Sint32 m = 0;
    s >> m;
    if (!s.Good() || m < 0 || !CountOk(s, static_cast<Uint32>(m), 8)) {
        return false;
    }
    subjects.assign(static_cast<std::size_t>(m), {});
    for (auto& [id, ref] : subjects) {
        s >> id;
        if (!RRef(s, ref)) {
            return false;
        }
    }
    return s.Good();
}
bool AdvisorSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{3} << field28;
    s.WriteGZString(name30);
    s.WriteGZString(name60);
    for (Sint32 v : field90) {
        s << v;
    }
    s << static_cast<Sint32>(advice.size());
    for (const auto& a : advice) {
        WRef(s, a.advice);
        s << a.fieldC << a.field10 << a.field8 << a.flag << a.value;
    }
    s << static_cast<Sint32>(subjects.size());
    for (const auto& [id, ref] : subjects) {
        s << id;
        WRef(s, ref);
    }
    return true;
}

bool AdvisorCityPlanningSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 3 || !advisor.Read(s)) {
        return false;
    }
    s >> field14C >> field150 >> field154;
    map158.clear();
    if (version >= 3) {
        Sint32 n = 0;
        s >> n;
        if (!s.Good() || n < 0 || !CountOk(s, static_cast<Uint32>(n), 8)) {
            return false;
        }
        map158.assign(static_cast<std::size_t>(n), {});
        for (auto& [k, v] : map158) {
            s >> k >> v;
        }
    }
    return s.Good();
}
bool AdvisorCityPlanningSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    s << ver;
    advisor.Write(s);
    s << field14C << field150 << field154;
    if (ver >= 3) {
        s << static_cast<Sint32>(map158.size());
        for (const auto& [k, v] : map158) {
            s << k << v;
        }
    }
    return true;
}

bool AdvisorSystemSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 1) {
        return false;
    }
    Sint32 n = 0;
    s >> n;
    if (!s.Good() || n < 0 || !CountOk(s, static_cast<Uint32>(n), 8)) {
        return false;
    }
    advisors.assign(static_cast<std::size_t>(n), {});
    for (auto& [ref, id] : advisors) {
        if (!RRef(s, ref)) {
            return false;
        }
        s >> id;
    }
    return s.Good();
}
bool AdvisorSystemSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1} << static_cast<Sint32>(advisors.size());
    for (const auto& [ref, id] : advisors) {
        WRef(s, ref);
        s << id;
    }
    return true;
}

// ===========================================================================
// cSC4MySim
// ===========================================================================
bool MySimSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    // Version 12 streams cSC4MySimDispatch inline (cSC4MySimDispatch::Read);
    // no shipped save uses it.
    if (!s.Good() || version > 17 || version == 12) {
        return false;
    }
    s >> hasSim;
    if (hasSim != 0) {
        s >> field150;
        if (!RRef(s, ref158)) {
            return false;
        }
        s >> field160 >> field164;
        if (!RRef(s, ref180)) {
            return false;
        }
        for (Sint32& v : field190) {
            s >> v;
        }
        s >> field1A8;
        RKey(s, key1AC);
        s >> field1B8 >> field1BC >> field1BE >> field1BF;
        if (!s.ReadGZString(name1C0)) {
            return false;
        }
        s >> field1E8 >> field1E9 >> field1EC;
        if (!s.ReadGZString(name1F0)) {
            return false;
        }
        s >> field234 >> field238 >> field23C;
        for (Float32& f : field168) {
            s >> f;
        }
        s >> field2D8 >> field2DC;
        for (Float32& f : field2A8) {
            s >> f;
        }
        s >> field2D9 >> field2E0;
        for (Float32& f : field2C0) {
            s >> f;
        }
        if (version >= 11) {
            if (version <= 14) {
                for (Sint32& v : legacy11To14) {
                    s >> v;
                }
            } else {
                RKey(s, key3C0);
                RKey(s, key3CC);
            }
            if (version >= 17) {
                s >> field31C >> field320;
            }
            if (version >= 12) {
                if (!RRef(s, dispatch328)) {
                    return false;
                }
                s >> timer3E0.units >> timer3E0.elapsed >> timer3E0.running >> hasRef3B8;
                if (hasRef3B8 != 0 && !RRef(s, ref3B8)) {
                    return false;
                }
            }
        }
        if (version >= 14) {
            s >> field3B1;
            if (version >= 16) {
                s >> field410;
            }
        }
    }
    return s.Good() && advisor.Read(s);
}

bool MySimSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    s << ver << hasSim;
    if (hasSim != 0) {
        s << field150;
        WRef(s, ref158);
        s << field160 << field164;
        WRef(s, ref180);
        for (Sint32 v : field190) {
            s << v;
        }
        s << field1A8;
        WKey(s, key1AC);
        s << field1B8 << field1BC << field1BE << field1BF;
        s.WriteGZString(name1C0);
        s << field1E8 << field1E9 << field1EC;
        s.WriteGZString(name1F0);
        s << field234 << field238 << field23C;
        for (Float32 f : field168) {
            s << f;
        }
        s << field2D8 << field2DC;
        for (Float32 f : field2A8) {
            s << f;
        }
        s << field2D9 << field2E0;
        for (Float32 f : field2C0) {
            s << f;
        }
        if (ver >= 11) {
            if (ver <= 14) {
                for (Sint32 v : legacy11To14) {
                    s << v;
                }
            } else {
                WKey(s, key3C0);
                WKey(s, key3CC);
            }
            if (ver >= 17) {
                s << field31C << field320;
            }
            if (ver >= 12) {
                WRef(s, dispatch328);
                s << timer3E0.units << timer3E0.elapsed << timer3E0.running << hasRef3B8;
                if (hasRef3B8 != 0) {
                    WRef(s, ref3B8);
                }
            }
        }
        if (ver >= 14) {
            s << field3B1;
            if (ver >= 16) {
                s << field410;
            }
        }
    }
    return advisor.Write(s);
}

// ===========================================================================
// Registry
// ===========================================================================
bool RoundTripSC4SubfileBatch3(Uint32 typeId, std::span<const Uint8> bytes,
                               SC4SubfileRoundTripStats& st) {
    using sc4_detail::RoundTripMulti;
    if (RoundTripSC4SubfileAutomata(typeId, bytes, st)) {
        return true;
    }
    if (std::find(kAdviceRecordCLSIDs.begin(), kAdviceRecordCLSIDs.end(), typeId) !=
        kAdviceRecordCLSIDs.end()) {
        RoundTripMulti<AdviceSaveRecord>(bytes, 4, st);
        return true;
    }
    if (std::find(kAdvisorRecordCLSIDs.begin(), kAdvisorRecordCLSIDs.end(), typeId) !=
        kAdvisorRecordCLSIDs.end()) {
        RoundTripMulti<AdvisorSaveRecord>(bytes, 3, st);
        return true;
    }
    switch (typeId) {
    case kCLSID_cSC4AdviceNeghborDeal:
        RoundTripMulti<AdviceNeghborDealSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4AdviceSubject:
        RoundTripMulti<AdviceSubjectSaveRecord>(bytes, 4, st);
        return true;
    case kCLSID_cSC4AdvisorCityPlanning:
        RoundTripMulti<AdvisorCityPlanningSaveRecord>(bytes, 3, st);
        return true;
    case kCLSID_cSC4AdvisorSystem:
        RoundTripMulti<AdvisorSystemSaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSC4MySim:
    case kCLSID_cSC4MyStreetSim:
        RoundTripMulti<MySimSaveRecord>(bytes, 17, st);
        return true;
    default:
        return false;
    }
}

} // namespace opensc4
