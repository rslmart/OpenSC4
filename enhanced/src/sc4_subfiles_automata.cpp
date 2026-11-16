// Exact decompilation of the automata .sc4 save subfiles. See
// include/opensc4/sc4_subfiles_automata.h for the binary address of every routine.

#include "opensc4/sc4_subfiles_automata.h"
#include "opensc4/sc4_subfiles_batch3.h"

#include <cstring>

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

// Uint32 count + count refs (std::list / std::vector of cISC4* pointers).
bool RRefList(cIGZIStream& s, std::vector<Ref>& out) noexcept {
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 4)) {
        return false;
    }
    out.assign(n, Ref{});
    for (Ref& r : out) {
        if (!RRef(s, r)) {
            return false;
        }
    }
    return s.Good();
}
void WRefList(cIGZOStream& s, const std::vector<Ref>& v) {
    s << static_cast<Uint32>(v.size());
    for (const Ref& r : v) {
        WRef(s, r);
    }
}

// operator>>/<<(cIGZ?Stream&, std::vector<unsigned char>&) @ 0x1000cf09e / 0x1000cf12e.
bool RBytes(cIGZIStream& s, std::vector<Uint8>& v) noexcept {
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 1)) {
        return false;
    }
    v.assign(n, 0);
    for (Uint8& b : v) {
        s >> b;
    }
    return s.Good();
}
void WBytes(cIGZOStream& s, const std::vector<Uint8>& v) {
    s << static_cast<Uint32>(v.size());
    for (Uint8 b : v) {
        s << b;
    }
}

} // namespace

// ===========================================================================
// tAutomataState (+ subclasses chosen by cSC4Automaton::CreateAutomataState)
// ===========================================================================
bool AutomataStateRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 3) {
        return false;
    }
    if (!RRef(s, ref)) {
        return false;
    }
    s >> field18 >> field1C >> field20 >> field24 >> field28;
    if (version >= 3) {
        s >> field2C;
    }
    if (!s.Good()) {
        return false;
    }
    switch (type) {
    case 8: // tAutomataStateRiotLeader
        s >> riot38 >> riot3C >> riot40;
        break;
    case 11: // tAutomataStateRandomWalk
        vecs[0].Read(s);
        s >> floats[0] >> floats[1];
        break;
    case 14: // tAutomataStatePlummet
        for (int i = 0; i < 5; ++i) {
            s >> floats[i];
        }
        break;
    case 15: // tAutomataStatePlopJump
        s >> floats[0] >> floats[1];
        break;
    case 16: // tAutomataStatePlayerDrive
    case 21: // tAutomataStatePullOver
        s >> floats[0];
        break;
    case 17: // tAutomataStateBumped
        for (S3DVector3& v : vecs) {
            v.Read(s);
        }
        break;
    case 19: // tAutomataStateAttachAnim
        if (!RRef(s, subRef)) {
            return false;
        }
        s >> anim44 >> anim40;
        break;
    case 0x34: // tAutomataStateChase
        if (!RRef(s, subRef)) {
            return false;
        }
        break;
    default: // tAutomataState / tAutomataStateCollide (20)
        break;
    }
    return s.Good();
}

bool AutomataStateRecord::Write(cIGZOStream& s) const {
    s << type << version;
    WRef(s, ref);
    s << field18 << field1C << field20 << field24 << field28;
    if (version >= 3) {
        s << field2C;
    }
    switch (type) {
    case 8:
        s << riot38 << riot3C << riot40;
        break;
    case 11:
        vecs[0].Write(s);
        s << floats[0] << floats[1];
        break;
    case 14:
        for (int i = 0; i < 5; ++i) {
            s << floats[i];
        }
        break;
    case 15:
        s << floats[0] << floats[1];
        break;
    case 16:
    case 21:
        s << floats[0];
        break;
    case 17:
        for (const S3DVector3& v : vecs) {
            v.Write(s);
        }
        break;
    case 19:
        WRef(s, subRef);
        s << anim44 << anim40;
        break;
    case 0x34:
        WRef(s, subRef);
        break;
    default:
        break;
    }
    return true;
}

// ===========================================================================
// cSC4Automaton
// ===========================================================================
bool AutomatonSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 8 || !base.Read(s)) {
        return false;
    }
    vec74.Read(s);
    vec00.Read(s);
    vec8C.Read(s);
    s >> fieldB0 >> fieldB4 >> fieldB8 >> fieldBC;
    if (!s.Good() || !RRef(s, refD0)) {
        return false;
    }
    s >> fieldD8 >> fieldDC;
    vecE0.Read(s);
    if (!s.Good() || !RRef(s, refF0)) {
        return false;
    }
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 30)) {
        return false;
    }
    states.assign(n, AutomataStateRecord{});
    for (AutomataStateRecord& st : states) {
        s >> st.type;
        if (!s.Good() || !st.Read(s)) {
            return false;
        }
    }
    s >> n;
    if (!CountOk(s, n, 12)) {
        return false;
    }
    keys.assign(n, {});
    for (auto& k : keys) {
        RKey(s, k);
    }
    RKey(s, key1C8);
    s >> field1D4;
    if (!s.Good() || !RRefList(s, messageTargets)) {
        return false;
    }
    for (Float32& f : tweens) {
        s >> f;
    }
    s >> fieldC4;
    if (version >= 8) {
        s >> field2DC;
    }
    return s.Good();
}

bool AutomatonSaveRecord::Write(cIGZOStream& s) const {
    s << version;
    if (!base.Write(s)) {
        return false;
    }
    vec74.Write(s);
    vec00.Write(s);
    vec8C.Write(s);
    s << fieldB0 << fieldB4 << fieldB8 << fieldBC;
    WRef(s, refD0);
    s << fieldD8 << fieldDC;
    vecE0.Write(s);
    WRef(s, refF0);
    s << static_cast<Uint32>(states.size());
    for (const AutomataStateRecord& st : states) {
        st.Write(s);
    }
    s << static_cast<Uint32>(keys.size());
    for (const auto& k : keys) {
        WKey(s, k);
    }
    WKey(s, key1C8);
    s << field1D4;
    WRefList(s, messageTargets);
    for (Float32 f : tweens) {
        s << f;
    }
    s << fieldC4;
    if (version >= 8) {
        s << field2DC;
    }
    return true;
}

// ===========================================================================
// cSC4TrafficAutomaton
// ===========================================================================
bool TrafficAutomatonSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 2 || !automaton.Read(s) || !RRef(s, refF0)) {
        return false;
    }
    s >> field308 >> field30C;
    if (!s.Good() || !RRef(s, ref310)) {
        return false;
    }
    s >> field320 >> field330;
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 3)) {
        return false;
    }
    route.assign(n, {});
    for (auto& r : route) {
        s >> r[0] >> r[1] >> r[2];
    }
    return s.Good();
}

bool TrafficAutomatonSaveRecord::Write(cIGZOStream& s) const {
    s << version;
    if (!automaton.Write(s)) {
        return false;
    }
    WRef(s, refF0);
    s << field308 << field30C;
    WRef(s, ref310);
    s << field320 << field330 << static_cast<Uint32>(route.size());
    for (const auto& r : route) {
        s << r[0] << r[1] << r[2];
    }
    return true;
}

// ===========================================================================
// cSC4Pedestrian
// ===========================================================================
bool PedestrianSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 5 || !traffic.Read(s)) {
        return false;
    }
    s >> field39C;
    vec3A0.Read(s);
    s >> field3B0 >> field398;
    return s.Good();
}

bool PedestrianSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{5};
    if (!traffic.Write(s)) {
        return false;
    }
    s << field39C;
    vec3A0.Write(s);
    s << field3B0 << field398;
    return true;
}

// ===========================================================================
// cSC4Vehicle and simple subclasses
// ===========================================================================
bool VehicleSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 2 || !traffic.Read(s)) {
        return false;
    }
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 12)) {
        return false;
    }
    entries.assign(n, Entry{});
    for (Entry& e : entries) {
        s >> e.key >> e.a >> e.b;
    }
    s >> field0;
    vec3B4.Read(s);
    uninit.Read(s);
    s >> field3B0;
    return s.Good();
}

bool VehicleSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    if (!traffic.Write(s)) {
        return false;
    }
    s << static_cast<Uint32>(entries.size());
    for (const Entry& e : entries) {
        s << e.key << e.a << e.b;
    }
    s << field0;
    vec3B4.Write(s);
    uninit.Write(s);
    s << field3B0;
    return true;
}

bool VehicleSubclassSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    return s.Good() && version == expectedVersion && vehicle.Read(s);
}
bool VehicleSubclassSaveRecord::Write(cIGZOStream& s) const {
    s << expectedVersion;
    return vehicle.Write(s);
}

bool EmergencyVehicleSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 2 || !vehicle.Read(s)) {
        return false;
    }
    vec424.Read(s);
    if (!s.Good() || !RRef(s, ref430)) {
        return false;
    }
    s >> field438 >> field43A >> field43C;
    return s.Good();
}
bool EmergencyVehicleSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    if (!vehicle.Write(s)) {
        return false;
    }
    vec424.Write(s);
    WRef(s, ref430);
    s << field438 << field43A << field43C;
    return true;
}

bool PatrolCarSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    return s.Good() && version == 2 && emergency.Read(s) && RRef(s, ref450) && s.Good();
}
bool PatrolCarSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    if (!emergency.Write(s)) {
        return false;
    }
    WRef(s, ref450);
    return true;
}

bool TrailerVehicleSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 2 || !vehicle.Read(s) || !RRefList(s, trailers)) {
        return false;
    }
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 4)) {
        return false;
    }
    list430.assign(n, 0u);
    for (Uint32& v : list430) {
        s >> v;
    }
    vec458.Read(s);
    if (!s.Good() || !RRef(s, ref468)) {
        return false;
    }
    s >> field470;
    return s.Good();
}
bool TrailerVehicleSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    if (!vehicle.Write(s)) {
        return false;
    }
    WRefList(s, trailers);
    s << static_cast<Uint32>(list430.size());
    for (Uint32 v : list430) {
        s << v;
    }
    vec458.Write(s);
    WRef(s, ref468);
    s << field470;
    return true;
}

bool TrainSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 2) {
        return false;
    }
    s >> trainCarVersion;
    if (!s.Good() || trainCarVersion != 2 || !vehicle.Read(s) || !RRefList(s, cars)) {
        return false;
    }
    s >> field430;
    vec450.Read(s);
    if (!s.Good() || !RRef(s, ref460)) {
        return false;
    }
    s >> field468;
    return s.Good();
}
bool TrainSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2} << Uint16{2};
    if (!vehicle.Write(s)) {
        return false;
    }
    WRefList(s, cars);
    s << field430;
    vec450.Write(s);
    WRef(s, ref460);
    s << field468;
    return true;
}

bool MassTransitTrainSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    return s.Good() && version == 1 && train.Read(s);
}
bool MassTransitTrainSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1};
    return train.Write(s);
}

bool AutomatonSubclassSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    return s.Good() && version == expectedVersion && automaton.Read(s);
}
bool AutomatonSubclassSaveRecord::Write(cIGZOStream& s) const {
    s << expectedVersion;
    return automaton.Write(s);
}

// ===========================================================================
// cSC4AutomataManager
// ===========================================================================
bool AutomataManagerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    return s.Good() && ReadInternal(s, version);
}

bool AutomataManagerSaveRecord::ReadInternal(cIGZIStream& s, Uint16 ver) noexcept {
    version = ver;
    if (ver > 6) {
        return false;
    }
    s >> field4C >> field74 >> fieldA85;
    if (!s.Good() || !RRefList(s, automata)) {
        return false;
    }
    counts.clear();
    if (ver > 4) {
        Uint32 n = 0;
        s >> n;
        if (!CountOk(s, n, 8)) {
            return false;
        }
        counts.assign(n, {});
        for (auto& [k, v] : counts) {
            s >> k >> v;
        }
    }
    return s.Good();
}

bool AutomataManagerSaveRecord::WriteInternal(cIGZOStream& s, Uint16 ver) const {
    s << field4C << field74 << fieldA85;
    WRefList(s, automata);
    if (ver > 4) {
        s << static_cast<Uint32>(counts.size());
        for (const auto& [k, v] : counts) {
            s << k << v;
        }
    }
    return true;
}

bool AutomataManagerSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    s << ver;
    return WriteInternal(s, ver);
}

// ===========================================================================
// cSC4TrafficAutomataManager (+ cSC4TrafficEffectManager)
// ===========================================================================
bool TrafficAutomataManagerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || (version & 0xfffeu) != 2u || !manager.Read(s)) {
        return false;
    }
    s >> fieldB10 >> current;
    effects.clear();
    if (version >= 3) {
        s >> effectVersion;
        if (!s.Good() || effectVersion != 1) {
            return false;
        }
        Uint32 n = 0;
        s >> n;
        if (!CountOk(s, n, 57)) {
            return false;
        }
        effects.assign(n, TrafficEffectRecord{});
        for (TrafficEffectRecord& e : effects) {
            s >> e.a >> e.b >> e.c;
            for (Sint32& v : e.rect1C) {
                s >> v;
            }
            for (Sint32& v : e.rect2C) {
                s >> v;
            }
            if (!RRef(s, e.ref40)) {
                return false;
            }
            s >> e.flag48 >> e.field4C;
        }
    }
    return s.Good();
}

bool TrafficAutomataManagerSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    s << ver;
    if (!manager.WriteWithVersion(s, manager.version)) {
        return false;
    }
    s << fieldB10 << current;
    if (ver >= 3) {
        s << Uint16{1} << static_cast<Uint32>(effects.size());
        for (const TrafficEffectRecord& e : effects) {
            s << e.a << e.b << e.c;
            for (Sint32 v : e.rect1C) {
                s << v;
            }
            for (Sint32 v : e.rect2C) {
                s << v;
            }
            WRef(s, e.ref40);
            s << e.flag48 << e.field4C;
        }
    }
    return true;
}

// ===========================================================================
// cSC4AircraftManager
// ===========================================================================
bool AircraftManagerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good()) {
        return false;
    }
    map.clear();
    list.clear();
    if (version <= 4) {
        return manager.ReadInternal(s, version);
    }
    if (version != 5 || !manager.Read(s)) {
        return false;
    }
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 8)) {
        return false;
    }
    map.assign(n, {});
    for (auto& [a, b] : map) {
        if (!RRef(s, a) || !RRef(s, b)) {
            return false;
        }
    }
    s >> n;
    if (!CountOk(s, n, 13)) {
        return false;
    }
    list.assign(n, Entry{});
    for (Entry& e : list) {
        s >> e.x >> e.y >> e.z >> e.flag;
    }
    return s.Good();
}

bool AircraftManagerSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    if (ver <= 4) {
        return manager.WriteWithVersion(s, ver);
    }
    s << Uint16{5};
    if (!manager.WriteWithVersion(s, manager.version)) {
        return false;
    }
    s << static_cast<Uint32>(map.size());
    for (const auto& [a, b] : map) {
        WRef(s, a);
        WRef(s, b);
    }
    s << static_cast<Uint32>(list.size());
    for (const Entry& e : list) {
        s << e.x << e.y << e.z << e.flag;
    }
    return true;
}

// ===========================================================================
// cSC4WatercraftManager
// ===========================================================================
bool WatercraftManagerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good()) {
        return false;
    }
    lotToOccupant.clear();
    watercraft.clear();
    terminals.clear();
    if (version <= 5) {
        return manager.ReadInternal(s, version);
    }
    if (version != 6 || !manager.Read(s)) {
        return false;
    }
    Sint32 n = 0;
    s >> n;
    if (n < 0 || !CountOk(s, static_cast<Uint32>(n), 8)) {
        return false;
    }
    lotToOccupant.assign(static_cast<std::size_t>(n), {});
    for (auto& [a, b] : lotToOccupant) {
        if (!RRef(s, a) || !RRef(s, b)) {
            return false;
        }
    }
    s >> n;
    if (n < 0 || !CountOk(s, static_cast<Uint32>(n), 4)) {
        return false;
    }
    watercraft.assign(static_cast<std::size_t>(n), Ref{});
    for (Ref& r : watercraft) {
        if (!RRef(s, r)) {
            return false;
        }
    }
    s >> n;
    if (n < 0 || !CountOk(s, static_cast<Uint32>(n), 12)) {
        return false;
    }
    terminals.assign(static_cast<std::size_t>(n), Terminal{});
    for (Terminal& t : terminals) {
        if (!RRef(s, t.lot) || !RRef(s, t.occupant)) {
            return false;
        }
        Sint32 m = 0;
        s >> m;
        if (m < 0 || !CountOk(s, static_cast<Uint32>(m), 33)) {
            return false;
        }
        t.routes.assign(static_cast<std::size_t>(m), Route{});
        for (Route& r : t.routes) {
            if (!RRef(s, r.a) || !RRef(s, r.b)) {
                return false;
            }
            s >> r.dir;
            for (Uint16& u : r.u16) {
                s >> u;
            }
            s >> r.flag >> r.s26 >> r.s28;
            if (!s.Good() || !RBytes(s, r.bytes30) || !RBytes(s, r.bytes48)) {
                return false;
            }
        }
    }
    return s.Good();
}

bool WatercraftManagerSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    if (ver <= 5) {
        return manager.WriteWithVersion(s, ver);
    }
    s << Uint16{6};
    if (!manager.WriteWithVersion(s, manager.version)) {
        return false;
    }
    s << static_cast<Sint32>(lotToOccupant.size());
    for (const auto& [a, b] : lotToOccupant) {
        WRef(s, a);
        WRef(s, b);
    }
    s << static_cast<Sint32>(watercraft.size());
    for (const Ref& r : watercraft) {
        WRef(s, r);
    }
    s << static_cast<Sint32>(terminals.size());
    for (const Terminal& t : terminals) {
        WRef(s, t.lot);
        WRef(s, t.occupant);
        s << static_cast<Sint32>(t.routes.size());
        for (const Route& r : t.routes) {
            WRef(s, r.a);
            WRef(s, r.b);
            s << r.dir;
            for (Uint16 u : r.u16) {
                s << u;
            }
            s << r.flag << r.s26 << r.s28;
            WBytes(s, r.bytes30);
            WBytes(s, r.bytes48);
        }
    }
    return true;
}

// ===========================================================================
// cSC4AutomataControllerManager
// ===========================================================================
bool AutomataControllerManagerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || !RRef(s, ref28)) {
        return false;
    }
    s >> field58 >> field5C >> field60 >> field64;
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 8)) {
        return false;
    }
    groups.assign(n, Group{});
    for (Group& g : groups) {
        if (!RRef(s, g.key) || !RRefList(s, g.controllers)) {
            return false;
        }
    }
    s >> n;
    if (!CountOk(s, n, 33)) {
        return false;
    }
    queues.assign(n, Queue{});
    for (Queue& q : queues) {
        if (!RRef(s, q.key) || !RRefList(s, q.automata)) {
            return false;
        }
        q.vec34.Read(s);
        q.vec40.Read(s);
        s >> q.flag4C;
    }
    return s.Good();
}

bool AutomataControllerManagerSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1};
    WRef(s, ref28);
    s << field58 << field5C << field60 << field64 << static_cast<Uint32>(groups.size());
    for (const Group& g : groups) {
        WRef(s, g.key);
        WRefList(s, g.controllers);
    }
    s << static_cast<Uint32>(queues.size());
    for (const Queue& q : queues) {
        WRef(s, q.key);
        WRefList(s, q.automata);
        q.vec34.Write(s);
        q.vec40.Write(s);
        s << q.flag4C;
    }
    return true;
}

// ===========================================================================
// cSC4AutomataController / Generator / Attractor
// ===========================================================================
bool AutomataControllerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || !RRef(s, ref18) || !s.ReadGZString(name)) {
        return false;
    }
    vec28.Read(s);
    s >> field98 >> field9C >> field68 >> field6C;
    return s.Good() && RRefList(s, automata);
}

bool AutomataControllerSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{5};
    WRef(s, ref18);
    s.WriteGZString(name);
    vec28.Write(s);
    s << field98 << field9C << field68 << field6C;
    WRefList(s, automata);
    return true;
}

bool AutomataGeneratorSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 4 || !controller.Read(s)) {
        return false;
    }
    s >> fieldC0 >> fieldC4 >> fieldC8 >> fieldCC >> fieldD8 >> fieldDC;
    return s.Good();
}

bool AutomataGeneratorSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{4};
    if (!controller.Write(s)) {
        return false;
    }
    s << fieldC0 << fieldC4 << fieldC8 << fieldCC << fieldD8 << fieldDC;
    return true;
}

bool AutomataAttractorSaveRecord::Read(cIGZIStream& s) noexcept {
    if (!controller.Read(s)) {
        return false;
    }
    s >> version;
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 8)) {
        return false;
    }
    curve.assign(n, {});
    for (auto& [x, y] : curve) {
        s >> x >> y;
    }
    return s.Good();
}

bool AutomataAttractorSaveRecord::Write(cIGZOStream& s) const {
    if (!controller.Write(s)) {
        return false;
    }
    s << Uint16{1} << static_cast<Uint32>(curve.size());
    for (const auto& [x, y] : curve) {
        s << x << y;
    }
    return true;
}

// ===========================================================================
// Non-COM / cSC4AutoSaveStream subfiles
// ===========================================================================
bool TerrainMiscSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version == 0 || version > 2) {
        return false;
    }
    s >> width50 >> height54 >> field24 >> field90 >> field94 >> field98;
    return s.Good();
}
bool TerrainMiscSaveRecord::Write(cIGZOStream& s) const {
    s << version << width50 << height54 << field24 << field90 << field94 << field98;
    return true;
}

bool TerrainVertexPropsSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 1 || (s.Remaining() % 2u) != 0u) {
        return false;
    }
    const std::size_t n = s.Remaining() / 2u;
    props.assign(n, 0);
    for (Uint16& p : props) {
        s >> p;
    }
    return s.Good();
}
bool TerrainVertexPropsSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1};
    for (Uint16 p : props) {
        s << p;
    }
    return true;
}

bool TerrainAltitudesSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good()) {
        return false;
    }
    if (version == 1) {
        s >> edgeCount;
        if (!CountOk(s, edgeCount, 4) || s.Remaining() != static_cast<std::size_t>(edgeCount) * 4u) {
            return false;
        }
        altitudes.assign(edgeCount, 0.0f);
    } else if (version == 2) {
        if ((s.Remaining() % 4u) != 0u) {
            return false;
        }
        altitudes.assign(s.Remaining() / 4u, 0.0f);
    } else {
        return false;
    }
    for (Float32& f : altitudes) {
        s >> f;
    }
    return s.Good();
}
bool TerrainAltitudesSaveRecord::Write(cIGZOStream& s) const {
    s << version;
    if (version == 1) {
        s << edgeCount;
    }
    for (Float32 f : altitudes) {
        s << f;
    }
    return true;
}

bool View3DWinSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version == 0 || version > 4) {
        return false;
    }
    if (!RRef(s, cameraControl) || !RRef(s, render3D) || !RRef(s, modelMaker) ||
        !RRef(s, lighting)) {
        return false;
    }
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 8)) {
        return false;
    }
    map678.assign(n, {});
    for (auto& [k, v] : map678) {
        s >> k >> v;
    }
    return s.Good();
}
bool View3DWinSaveRecord::Write(cIGZOStream& s) const {
    s << version;
    WRef(s, cameraControl);
    WRef(s, render3D);
    WRef(s, modelMaker);
    WRef(s, lighting);
    s << static_cast<Uint32>(map678.size());
    for (const auto& [k, v] : map678) {
        s << k << v;
    }
    return true;
}

bool RegionalCitySaveRecord::Read(cIGZIStream& s) noexcept {
    s >> streamVersion >> version;
    if (!s.Good() || streamVersion != 1 || version == 0 || version > 13) {
        return false;
    }
    for (Sint32& v : field20) {
        s >> v;
    }
    if (version >= 10) {
        s >> field3C;
    }
    s >> field40;
    if (version >= 13) {
        s >> field44;
    }
    s >> fieldF0 >> fieldF4;
    for (Sint32& v : fieldF8) {
        s >> v;
    }
    s >> field108 >> field260;
    for (std::string& str : strings) {
        if (!s.ReadGZString(str)) {
            return false;
        }
    }
    for (Float32& f : field228) {
        s >> f;
    }
    s >> field264;
    for (auto& m : maps) {
        Uint32 n = 0;
        s >> n;
        if (!CountOk(s, n, 8)) {
            return false;
        }
        m.assign(n, {});
        for (auto& [k, v] : m) {
            s >> k >> v;
        }
    }
    Uint32 n = 0;
    s >> n;
    if (n > 12 || !CountOk(s, n, 4)) {
        return false;
    }
    floats90.assign(n, 0.0f);
    for (Float32& f : floats90) {
        s >> f;
    }
    s >> n;
    if (!CountOk(s, n, 23)) {
        return false;
    }
    connections.assign(n, NeighborConnectionSaveRecord{});
    for (NeighborConnectionSaveRecord& c : connections) {
        if (!c.Read(s)) {
            return false;
        }
    }
    s >> n;
    if (!CountOk(s, n, 13)) {
        return false;
    }
    connectionChanges.assign(n, ConnectionChange{});
    for (ConnectionChange& cc : connectionChanges) {
        s >> cc.a >> cc.b >> cc.c >> cc.d;
    }
    s >> field268 >> field270;
    Sint32 sn = 0;
    s >> sn;
    if (sn < 0 || !CountOk(s, static_cast<Uint32>(sn), 44)) {
        return false;
    }
    commodities.assign(static_cast<std::size_t>(sn), CommodityStats{});
    for (CommodityStats& cs : commodities) {
        s >> cs.key;
        for (Float64& d : cs.v) {
            s >> d;
        }
    }
    auto readDeals = [&](std::vector<NeighborDealSaveRecord>& out) {
        Sint32 dn = 0;
        s >> dn;
        if (dn < 0 || !CountOk(s, static_cast<Uint32>(dn), 60)) {
            return false;
        }
        out.assign(static_cast<std::size_t>(dn), NeighborDealSaveRecord{});
        for (NeighborDealSaveRecord& nd : out) {
            if (!nd.Read(s)) {
                return false;
            }
        }
        return s.Good();
    };
    if (!readDeals(dealsA) || !readDeals(dealsB)) {
        return false;
    }
    poi.clear();
    if (version >= 12) {
        s >> n;
        if (!CountOk(s, n, 8)) {
            return false;
        }
        poi.assign(n, PointOfInterestGroup{});
        for (PointOfInterestGroup& g : poi) {
            Uint32 m = 0;
            s >> g.type >> m;
            if (!CountOk(s, m, 12)) {
                return false;
            }
            g.points.assign(m, {});
            for (auto& pt : g.points) {
                s >> pt[0] >> pt[1] >> pt[2];
            }
        }
    }
    return s.Good();
}

bool RegionalCitySaveRecord::Write(cIGZOStream& s) const {
    s << streamVersion << version;
    for (Sint32 v : field20) {
        s << v;
    }
    if (version >= 10) {
        s << field3C;
    }
    s << field40;
    if (version >= 13) {
        s << field44;
    }
    s << fieldF0 << fieldF4;
    for (Sint32 v : fieldF8) {
        s << v;
    }
    s << field108 << field260;
    for (const std::string& str : strings) {
        s.WriteGZString(str);
    }
    for (Float32 f : field228) {
        s << f;
    }
    s << field264;
    for (const auto& m : maps) {
        s << static_cast<Uint32>(m.size());
        for (const auto& [k, v] : m) {
            s << k << v;
        }
    }
    s << static_cast<Uint32>(floats90.size());
    for (Float32 f : floats90) {
        s << f;
    }
    s << static_cast<Uint32>(connections.size());
    for (const NeighborConnectionSaveRecord& c : connections) {
        if (!c.Write(s)) {
            return false;
        }
    }
    s << static_cast<Uint32>(connectionChanges.size());
    for (const ConnectionChange& cc : connectionChanges) {
        s << cc.a << cc.b << cc.c << cc.d;
    }
    s << field268 << field270 << static_cast<Sint32>(commodities.size());
    for (const CommodityStats& cs : commodities) {
        s << cs.key;
        for (Float64 d : cs.v) {
            s << d;
        }
    }
    auto writeDeals = [&](const std::vector<NeighborDealSaveRecord>& v) {
        s << static_cast<Sint32>(v.size());
        for (const NeighborDealSaveRecord& nd : v) {
            if (!nd.WriteWithVersion(s, nd.version)) {
                return false;
            }
        }
        return true;
    };
    if (!writeDeals(dealsA) || !writeDeals(dealsB)) {
        return false;
    }
    if (version >= 12) {
        s << static_cast<Uint32>(poi.size());
        for (const PointOfInterestGroup& g : poi) {
            s << g.type << static_cast<Uint32>(g.points.size());
            for (const auto& pt : g.points) {
                s << pt[0] << pt[1] << pt[2];
            }
        }
    }
    return true;
}

bool StringRecordSaveRecord::Read(cIGZIStream& s) noexcept {
    if (s.Remaining() < 4u) {
        return false;
    }
    raw.resize(s.Remaining());
    for (Uint8& b : raw) {
        s >> b;
    }
    if (!s.Good()) {
        return false;
    }
    Uint32 header = 0;
    std::memcpy(&header, raw.data(), 4);
    const Uint32 len = header & 0x00ffffffu;
    const Uint32 flags = header >> 24u;
    if (flags == 0u) {
        return raw.size() == 4u + static_cast<std::size_t>(len);
    }
    if (flags == 0x10u) {
        return raw.size() == 4u + static_cast<std::size_t>(len) * 2u;
    }
    return true;
}
bool StringRecordSaveRecord::Write(cIGZOStream& s) const {
    s.WriteBytes(raw.data(), raw.size());
    return true;
}

bool RegionViewThumbnailSaveRecord::Read(cIGZIStream& s) noexcept {
    if (s.Remaining() < 8u) {
        return false;
    }
    pngBytes.resize(s.Remaining());
    for (Uint8& b : pngBytes) {
        s >> b;
    }
    static constexpr Uint8 kPngSig[8] = {0x89u, 0x50u, 0x4eu, 0x47u, 0x0du, 0x0au, 0x1au, 0x0au};
    return s.Good() && std::memcmp(pngBytes.data(), kPngSig, 8) == 0;
}
bool RegionViewThumbnailSaveRecord::Write(cIGZOStream& s) const {
    s.WriteBytes(pngBytes.data(), pngBytes.size());
    return true;
}

bool DBPFCompressedSetSaveRecord::Read(cIGZIStream& s) noexcept {
    if ((s.Remaining() % 16u) != 0u) {
        return false;
    }
    entries.assign(s.Remaining() / 16u, Entry{});
    for (Entry& e : entries) {
        s >> e.type >> e.group >> e.instance >> e.uncompressedSize;
    }
    return s.Good();
}
bool DBPFCompressedSetSaveRecord::Write(cIGZOStream& s) const {
    for (const Entry& e : entries) {
        s << e.type << e.group << e.instance << e.uncompressedSize;
    }
    return true;
}

bool AirplaneSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version >> aircraftVersion;
    if (!s.Good() || version != 1 || aircraftVersion != 1 || !automaton.Read(s)) {
        return false;
    }
    return vec48C.Read(s);
}
bool AirplaneSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1} << Uint16{1};
    return automaton.Write(s) && vec48C.Write(s);
}

bool HelicopterSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version >> aircraftVersion;
    if (!s.Good() || version != 1 || aircraftVersion != 1 || !automaton.Read(s)) {
        return false;
    }
    return vec310.Read(s);
}
bool HelicopterSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1} << Uint16{1};
    return automaton.Write(s) && vec310.Write(s);
}

bool AmbulanceSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    return s.Good() && version == 1 && emergency.Read(s);
}
bool AmbulanceSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1};
    return emergency.Write(s);
}

bool GetawayVanSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 3 || !vehicle.Read(s)) {
        return false;
    }
    s >> flag >> field;
    return s.Good();
}
bool GetawayVanSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{3};
    if (!vehicle.Write(s)) {
        return false;
    }
    s << flag << field;
    return true;
}

bool TankSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 2 || !vehicle.Read(s)) {
        return false;
    }
    if (version >= 2) {
        s >> idxA >> idxB;
    }
    return s.Good();
}
bool TankSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    if (!vehicle.Write(s)) {
        return false;
    }
    s << idxA << idxB;
    return true;
}

bool RiotPoliceSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 2 || !pedestrian.Read(s) || !RRef(s, ref3B8)) {
        return false;
    }
    s >> field3C0 >> field3C4 >> field3C5;
    return s.Good();
}
bool RiotPoliceSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    if (!pedestrian.Write(s)) {
        return false;
    }
    WRef(s, ref3B8);
    s << field3C0 << field3C4 << field3C5;
    return true;
}

bool SimFireCrewSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 2 || !pedestrian.Read(s) || !RRef(s, ref3B8)) {
        return false;
    }
    s >> field3C0 >> field3C4 >> field410 >> field414;
    vec418.Read(s);
    s >> field424;
    vec428.Read(s);
    return s.Good();
}
bool SimFireCrewSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    if (!pedestrian.Write(s)) {
        return false;
    }
    WRef(s, ref3B8);
    s << field3C0 << field3C4 << field410 << field414;
    vec418.Write(s);
    s << field424;
    vec428.Write(s);
    return true;
}

bool DispatchSourceSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 1 || !RRefList(s, occupants)) {
        return false;
    }
    s >> field50;
    return s.Good();
}
bool DispatchSourceSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1};
    WRefList(s, occupants);
    s << field50;
    return true;
}

bool AnimationTickManagerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 1) {
        return false;
    }
    for (auto& b : buckets) {
        if (!RRefList(s, b)) {
            return false;
        }
    }
    return s.Good();
}
bool AnimationTickManagerSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1};
    for (const auto& b : buckets) {
        WRefList(s, b);
    }
    return true;
}

bool DisasterInstanceRiotSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 2) {
        return false;
    }
    s >> field58 >> timerUnits >> timerElapsed >> timerRunning >> field78;
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 20)) {
        return false;
    }
    leaders.assign(n, RiotLeader{});
    for (RiotLeader& rl : leaders) {
        if (!RRef(s, rl.leader)) {
            return false;
        }
        s >> rl.a >> rl.b >> rl.c;
        if (!s.Good() || !RRef(s, rl.target)) {
            return false;
        }
    }
    return s.Good();
}
bool DisasterInstanceRiotSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2} << field58 << timerUnits << timerElapsed << timerRunning << field78
      << static_cast<Uint32>(leaders.size());
    for (const RiotLeader& rl : leaders) {
        WRef(s, rl.leader);
        s << rl.a << rl.b << rl.c;
        WRef(s, rl.target);
    }
    return true;
}

bool MySimDispatchSaveRecord::Location::Read(cIGZIStream& s) noexcept {
    if (!RRef(s, ref)) {
        return false;
    }
    s >> x >> y;
    for (Sint32& v : ints) {
        s >> v;
    }
    s >> b20 >> b21 >> s24 >> s28;
    return s.Good();
}
bool MySimDispatchSaveRecord::Location::Write(cIGZOStream& s) const {
    WRef(s, ref);
    s << x << y;
    for (Sint32 v : ints) {
        s << v;
    }
    s << b20 << b21 << s24 << s28;
    return true;
}

bool MySimDispatchSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 1) {
        return false;
    }
    s >> field20;
    for (Sint32& v : fields24) {
        s >> v;
    }
    for (Location& l : locs) {
        if (!l.Read(s)) {
            return false;
        }
    }
    if (!RRef(s, ref130)) {
        return false;
    }
    s >> field138;
    if (!s.ReadGZString(name140) || !RRef(s, ref168) || !RRef(s, ref170) || !RRef(s, ref178)) {
        return false;
    }
    Sint32 n = 0;
    s >> n;
    if (n < 0 || !CountOk(s, static_cast<Uint32>(n), 3)) {
        return false;
    }
    route.assign(static_cast<std::size_t>(n), {});
    for (auto& r : route) {
        s >> r[0] >> r[1] >> r[2];
    }
    s >> n;
    if (n < 0 || !CountOk(s, static_cast<Uint32>(n), 20)) {
        return false;
    }
    map1B8.assign(static_cast<std::size_t>(n), MapEntry{});
    for (MapEntry& e : map1B8) {
        s >> e.key;
        RKey(s, e.resKey);
        s >> e.val;
    }
    s >> field1D0 >> n;
    if (n < 0 || !CountOk(s, static_cast<Uint32>(n), 38)) {
        return false;
    }
    extraLocs.assign(static_cast<std::size_t>(n), Location{});
    for (Location& l : extraLocs) {
        if (!l.Read(s)) {
            return false;
        }
    }
    s >> field10;
    return s.Good();
}

bool MySimDispatchSaveRecord::Write(cIGZOStream& s) const {
    s << Sint32{1} << field20;
    for (Sint32 v : fields24) {
        s << v;
    }
    for (const Location& l : locs) {
        l.Write(s);
    }
    WRef(s, ref130);
    s << field138;
    s.WriteGZString(name140);
    WRef(s, ref168);
    WRef(s, ref170);
    WRef(s, ref178);
    s << static_cast<Sint32>(route.size());
    for (const auto& r : route) {
        s << r[0] << r[1] << r[2];
    }
    s << static_cast<Sint32>(map1B8.size());
    for (const MapEntry& e : map1B8) {
        s << e.key;
        WKey(s, e.resKey);
        s << e.val;
    }
    s << field1D0 << static_cast<Sint32>(extraLocs.size());
    for (const Location& l : extraLocs) {
        l.Write(s);
    }
    s << field10;
    return true;
}

bool ResKeyTableSaveRecord::Read(cIGZIStream& s) noexcept {
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 12)) {
        return false;
    }
    keys.assign(n, {});
    for (auto& k : keys) {
        RKey(s, k);
    }
    return s.Good();
}
bool ResKeyTableSaveRecord::Write(cIGZOStream& s) const {
    s << static_cast<Uint32>(keys.size());
    for (const auto& k : keys) {
        WKey(s, k);
    }
    return true;
}

// ===========================================================================
// Registry
// ===========================================================================
namespace {

template <Uint16 kVersion>
struct VehicleSubclassV : VehicleSubclassSaveRecord {
    VehicleSubclassV() { expectedVersion = kVersion; }
};
template <Uint16 kVersion>
struct AutomatonSubclassV : AutomatonSubclassSaveRecord {
    AutomatonSubclassV() { expectedVersion = kVersion; }
};

} // namespace

bool RoundTripSC4SubfileAutomata(Uint32 typeId, std::span<const Uint8> bytes,
                                 SC4SubfileRoundTripStats& st) {
    using sc4_detail::RoundTripMulti;
    using sc4_detail::RoundTripRaw;
    switch (typeId) {
    case kCLSID_cSC4AdvisorBase:
        RoundTripMulti<AdvisorSaveRecord>(bytes, 3, st);
        return true;
    case kSC4TypeIdTerrainMisc:
        RoundTripRaw<TerrainMiscSaveRecord>(bytes, st);
        return true;
    case kSC4TypeIdTerrainVertexProps:
        RoundTripRaw<TerrainVertexPropsSaveRecord>(bytes, st);
        return true;
    case kSC4TypeIdTerrainAltitudes:
        RoundTripRaw<TerrainAltitudesSaveRecord>(bytes, st);
        return true;
    case kSC4TypeIdView3DWin:
        RoundTripRaw<View3DWinSaveRecord>(bytes, st);
        return true;
    case kSC4TypeIdRegionalCity:
        RoundTripRaw<RegionalCitySaveRecord>(bytes, st);
        return true;
    case kSC4TypeIdStringRecord:
        RoundTripRaw<StringRecordSaveRecord>(bytes, st);
        return true;
    case kSC4TypeIdRegionViewThumbnail:
        RoundTripRaw<RegionViewThumbnailSaveRecord>(bytes, st);
        return true;
    case kSC4TypeIdDBPFCompressedSet:
        RoundTripRaw<DBPFCompressedSetSaveRecord>(bytes, st);
        return true;
    case kCLSID_cSC4AutomatonPedestrian:
    case kCLSID_cSC4PedestrianOneShot:
        RoundTripMulti<PedestrianSaveRecord>(bytes, 5, st);
        return true;
    case kCLSID_cSC4Airplane:
        RoundTripMulti<AirplaneSaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSC4Helicopter:
        RoundTripMulti<HelicopterSaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSC4Ambulance:
        RoundTripMulti<AmbulanceSaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSC4FireTruck:
        RoundTripMulti<FireTruckSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4GetawayVan:
        RoundTripMulti<GetawayVanSaveRecord>(bytes, 3, st);
        return true;
    case kCLSID_cSC4Tank:
        RoundTripMulti<TankSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4Bus:
        RoundTripMulti<VehicleSubclassV<2>>(bytes, 2, st);
        return true;
    case kCLSID_cSC4MayorLimo:
        RoundTripMulti<VehicleSubclassV<1>>(bytes, 1, st);
        return true;
    case kCLSID_cSC4RiotPolice:
        RoundTripMulti<RiotPoliceSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4SimFireCrew:
        RoundTripMulti<SimFireCrewSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4FirePlaneDispatchSource:
    case kCLSID_cSC4PoliceHelicopterDispatchSource:
        RoundTripMulti<DispatchSourceSaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSC4AnimationTickManager:
        RoundTripMulti<AnimationTickManagerSaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSC4DisasterInstanceRiot:
        RoundTripMulti<DisasterInstanceRiotSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4MySimDispatch:
        RoundTripMulti<MySimDispatchSaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSCResKeyTable:
        RoundTripMulti<ResKeyTableSaveRecord>(bytes, sc4_detail::kCompareAllVersions, st);
        return true;
    case kCLSID_cSC4Nation:
    case kCLSID_cSC4StringSpecificationOccupantFilter:
        RoundTripMulti<VersionOnlySaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSC4AutomatonVehicle:
        RoundTripMulti<VehicleSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4AutomatonPatrolCar:
        RoundTripMulti<PatrolCarSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4AutomatonTrailerVehicle:
        RoundTripMulti<TrailerVehicleSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4AutomatonCarjacked:
        RoundTripMulti<VehicleSubclassV<1>>(bytes, 1, st);
        return true;
    case kCLSID_cSC4AutomatonTrainCar:
        RoundTripMulti<VehicleSubclassV<2>>(bytes, 2, st);
        return true;
    case kCLSID_cSC4AutomatonTrain:
        RoundTripMulti<TrainSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4AutomatonMassTransitTrain:
        RoundTripMulti<MassTransitTrainSaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSC4AutomatonAircraft:
        RoundTripMulti<AutomatonSubclassV<1>>(bytes, 1, st);
        return true;
    case kCLSID_cSC4AutomatonWatercraft:
        RoundTripMulti<AutomatonSubclassV<2>>(bytes, 2, st);
        return true;
    case kCLSID_cSC4AutomataManagerBase:
        RoundTripMulti<AutomataManagerSaveRecord>(bytes, 6, st);
        return true;
    case kCLSID_cSC4VehicleManager:
    case kCLSID_cSC4PedestrianManager:
        RoundTripMulti<TrafficAutomataManagerSaveRecord>(bytes, 3, st);
        return true;
    case kCLSID_cSC4AircraftManager:
        RoundTripMulti<AircraftManagerSaveRecord>(bytes, 5, st);
        return true;
    case kCLSID_cSC4WatercraftManager:
        RoundTripMulti<WatercraftManagerSaveRecord>(bytes, 6, st);
        return true;
    case kCLSID_cSC4AutomataControllerManager:
        RoundTripMulti<AutomataControllerManagerSaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSC4AutomataControllerBase:
        RoundTripMulti<AutomataControllerSaveRecord>(bytes, 5, st);
        return true;
    case kCLSID_cSC4AutomataGenerator:
        RoundTripMulti<AutomataGeneratorSaveRecord>(bytes, 4, st);
        return true;
    case kCLSID_cSC4AutomataAttractor:
        RoundTripMulti<AutomataAttractorSaveRecord>(bytes, 5, st);
        return true;
    default:
        return false;
    }
}

} // namespace opensc4
