// Exact decompilation of the Batch 1 .sc4 save subfiles. See
// include/opensc4/sc4_subfiles.h for the binary address of every routine.

#include "opensc4/sc4_subfiles.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace opensc4 {
namespace {

using Ref = SimSerializableRef;

bool RRef(cIGZIStream& s, Ref& r) noexcept { return ReadSimSerializableRef(s, r); }
void WRef(cIGZOStream& s, const Ref& r) { WriteSimSerializableRef(s, r); }

bool RRefVec(cIGZIStream& s, std::vector<Ref>& v) noexcept {
    Uint32 n = 0;
    s >> n;
    if (!s.Good() || n > s.Remaining() / 4u) {
        return false;
    }
    v.assign(n, Ref{});
    for (Ref& r : v) {
        if (!RRef(s, r)) {
            return false;
        }
    }
    return true;
}

void WRefVec(cIGZOStream& s, const std::vector<Ref>& v) {
    s << static_cast<Uint32>(v.size());
    for (const Ref& r : v) {
        WRef(s, r);
    }
}

// Guard against corrupt counts: every element consumes at least minBytes.
bool CountOk(const cIGZIStream& s, Uint32 n, std::size_t minBytes) noexcept {
    return s.Good() && (minBytes == 0 || n <= s.Remaining() / minBytes);
}

// SC4ReadGZPersistRef / SC4WriteGZPersistRef: Uint32 type, group, instance.
void RPersist(cIGZIStream& s, cSC4TGI& k) noexcept { s >> k.type >> k.group >> k.instance; }
void WPersist(cIGZOStream& s, const cSC4TGI& k) { s << k.type << k.group << k.instance; }

bool ReadRecordFromSC4Bytes(std::span<const Uint8> bytes, Uint32& memAddr,
                            const auto& readFn) noexcept {
    if (bytes.size() < 12u) {
        return false;
    }
    Uint32 recSize = 0;
    std::memcpy(&recSize, bytes.data(), 4);
    if (recSize < 12u || recSize > bytes.size()) {
        return false;
    }
    std::memcpy(&memAddr, bytes.data() + 8, 4);
    cIGZIStream s(bytes.data() + 12, recSize - 12u);
    return readFn(s) && s.Good();
}

// Float-magic round used by cSC4TrafficNetworkMap::Read to narrow the
// per-direction capacity floats into Uint16 (clamped to 0xffff).
Uint16 RoundCapacityToUint16(Float32 f) noexcept {
    Float32 m = f + 12582912.0f;
    Uint32 bits = 0;
    std::memcpy(&bits, &m, 4);
    const Sint32 v = static_cast<Sint32>(bits - 0x4b400000u);
    if (v < 0) {
        return 0;
    }
    return static_cast<Uint16>(std::min<Sint32>(v, 0xffff));
}

} // namespace

// ===========================================================================
// S3D primitives
// ===========================================================================
bool S3DVector2::Read(cIGZIStream& s) noexcept { s >> x >> y; return s.Good(); }
bool S3DVector2::Write(cIGZOStream& s) const { s << x << y; return true; }

bool S3DVector3::Read(cIGZIStream& s) noexcept { s >> x >> y >> z; return s.Good(); }
bool S3DVector3::Write(cIGZOStream& s) const { s << x << y << z; return true; }

bool S3DBoundingBox::Read(cIGZIStream& s) noexcept { return min.Read(s) && max.Read(s); }
bool S3DBoundingBox::Write(cIGZOStream& s) const { return min.Write(s) && max.Write(s); }

bool S3DMatrix4::Read(cIGZIStream& s) noexcept {
    for (Float32& f : m) {
        s >> f;
    }
    return s.Good();
}
bool S3DMatrix4::Write(cIGZOStream& s) const {
    for (Float32 f : m) {
        s << f;
    }
    return true;
}

// operator>>(cIGZIStream&, cS3DTransform&) @ 0x1000cf71d and
// S3DTransformLegacyLoad @ 0x1000cf954.
bool S3DTransform::Read(cIGZIStream& s, bool legacyLoad) noexcept {
    if (legacyLoad) {
        for (Float32& f : rotation) {
            s >> f;
        }
        translation.Read(s);
        s >> scale;
        Uint32 counter = 0;
        s >> counter; // low byte copied into the transform's change counter
        flags = 0x07;
        return s.Good();
    }
    s >> flags;
    if (flags & 0x04u) {
        for (Float32& f : rotation) {
            s >> f;
        }
    }
    if (flags & 0x01u) {
        translation.Read(s);
    }
    if (flags & 0x02u) {
        s >> scale;
    }
    return s.Good();
}

// operator<<(cIGZOStream&, cS3DTransform const&) @ 0x1000cf895.
bool S3DTransform::Write(cIGZOStream& s) const {
    s << flags;
    if (flags & 0x04u) {
        for (Float32 f : rotation) {
            s << f;
        }
    }
    if (flags & 0x01u) {
        translation.Write(s);
    }
    if (flags & 0x02u) {
        s << scale;
    }
    return true;
}

bool S3DVertexV3FC4UBT2F::Read(cIGZIStream& s) noexcept {
    s >> x >> y >> z >> u >> v >> r >> g >> b >> a;
    return s.Good();
}
bool S3DVertexV3FC4UBT2F::Write(cIGZOStream& s) const {
    s << x << y << z << u << v << r << g << b << a;
    return true;
}

bool ReadVertexVectorFromStream(cIGZIStream& s, std::vector<S3DVertexV3FC4UBT2F>& out) noexcept {
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 24)) {
        return false;
    }
    out.assign(n, S3DVertexV3FC4UBT2F{});
    for (auto& vtx : out) {
        if (!vtx.Read(s)) {
            return false;
        }
    }
    return true;
}

bool WriteVertexVectorToStream(cIGZOStream& s, std::span<const S3DVertexV3FC4UBT2F> verts) {
    s << static_cast<Uint32>(verts.size());
    for (const auto& vtx : verts) {
        vtx.Write(s);
    }
    return true;
}

// ===========================================================================
// App-level records
// ===========================================================================
bool SC4AppLoadSaveVersionRecord::Read(cIGZIStream& s) noexcept { s >> version; return s.Good(); }
bool SC4AppLoadSaveVersionRecord::Write(cIGZOStream& s) const { s << version; return true; }
bool SC4AppLoadSaveVersionRecord::ReadFromBytes(std::span<const Uint8> b) noexcept {
    cIGZIStream s(b.data(), b.size());
    return Read(s);
}

bool SC4AppSaveVersionInfoRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    for (Uint32& v : moduleVersions) {
        s >> v;
    }
    return s.Good();
}
bool SC4AppSaveVersionInfoRecord::Write(cIGZOStream& s) const {
    s << Uint16{3};
    for (Uint32 v : moduleVersions) {
        s << v;
    }
    return true;
}
bool SC4AppSaveVersionInfoRecord::ReadFromBytes(std::span<const Uint8> b) noexcept {
    cIGZIStream s(b.data(), b.size());
    return Read(s);
}

bool SC4AppCityLinkRecord::Read(cIGZIStream& s) noexcept {
    s >> streamVersion >> appVersion;
    return s.Good() && RRef(s, cityRef);
}
bool SC4AppCityLinkRecord::Write(cIGZOStream& s) const {
    s << Uint16{1} << Uint16{2};
    WRef(s, cityRef);
    return true;
}
bool SC4AppCityLinkRecord::ReadFromBytes(std::span<const Uint8> b) noexcept {
    cIGZIStream s(b.data(), b.size());
    return Read(s);
}

bool SC4CustomContentRecord::Read(cIGZIStream& s) noexcept {
    Uint32 size = 0;
    s >> version >> size;
    if (!s.Good() || size > s.Remaining()) {
        return false;
    }
    payload.resize(size);
    return s.ReadBytes(payload.data(), size);
}
bool SC4CustomContentRecord::Write(cIGZOStream& s) const {
    s << Uint16{1} << static_cast<Uint32>(payload.size());
    s.WriteBytes(payload.data(), payload.size());
    return true;
}
bool SC4CustomContentRecord::ReadFromBytes(std::span<const Uint8> b) noexcept {
    cIGZIStream s(b.data(), b.size());
    return Read(s);
}

bool COMSerializerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version >> classCount >> totalObjectCount;
    if (!CountOk(s, classCount, 8)) {
        return false;
    }
    classes.assign(classCount, {});
    for (auto& [clsid, count] : classes) {
        s >> clsid >> count;
    }
    return s.Good();
}
bool COMSerializerSaveRecord::Write(cIGZOStream& s) const {
    s << version << static_cast<Uint32>(classes.size()) << totalObjectCount;
    for (const auto& [clsid, count] : classes) {
        s << clsid << count;
    }
    return true;
}
bool COMSerializerSaveRecord::ReadFromBytes(std::span<const Uint8> b) noexcept {
    cIGZIStream s(b.data(), b.size());
    return Read(s);
}

// ===========================================================================
// cSC4City (Read @ 0x1006be88c / Write @ 0x1006be1c8)
// ===========================================================================
bool CitySaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version < 8 || version > 12) {
        return false;
    }
    if (!ReadSCPropertyHolder(s, properties)) {
        return false;
    }
    s >> field50 >> field54 >> field58 >> field5C >> field60 >> field64 >> field68 >> field45;
    if (version >= 10) {
        s >> field48;
    } else {
        field48 = field45 != 0 ? 2 : 0;
    }
    for (std::string& str : strings) {
        if (!s.ReadGZString(str)) {
            return false;
        }
    }
    s >> field1B0 >> field1B4 >> field1B8 >> field1BC;
    for (Float32& f : floats1C4) {
        s >> f;
    }
    s >> field1DC >> field1E0;
    for (Ref& r : baseSubsystemRefs) {
        if (!RRef(s, r)) {
            return false;
        }
    }
    if (version >= 9 && !RRef(s, ref250)) {
        return false;
    }
    if (version >= 11 && !RRef(s, ref380)) {
        return false;
    }
    if (version >= 12 && !RRef(s, ref3A8)) {
        return false;
    }
    s >> field3B0;
    return s.Good();
}

bool CitySaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 v) const {
    s << v;
    WriteSCPropertyHolder(s, properties);
    s << field50 << field54 << field58 << field5C << field60 << field64 << field68 << field45;
    if (v >= 10) {
        s << field48;
    }
    for (const std::string& str : strings) {
        s.WriteGZString(str);
    }
    s << field1B0 << field1B4 << field1B8 << field1BC;
    for (Float32 f : floats1C4) {
        s << f;
    }
    s << field1DC << field1E0;
    for (const Ref& r : baseSubsystemRefs) {
        WRef(s, r);
    }
    if (v >= 9) {
        WRef(s, ref250);
    }
    if (v >= 11) {
        WRef(s, ref380);
    }
    if (v >= 12) {
        WRef(s, ref3A8);
    }
    s << field3B0;
    return true;
}

bool CitySaveRecord::Write(cIGZOStream& s) const { return WriteWithVersion(s, 12); }

bool CitySaveRecord::ReadFromSC4Record(std::span<const Uint8> b) noexcept {
    return ReadRecordFromSC4Bytes(b, memAddr, [this](cIGZIStream& s) { return Read(s); });
}

// ===========================================================================
// cSC4OccupantManager (Read @ 0x1002472f2 / Write @ 0x10024712a)
// ===========================================================================
bool OccupantManagerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 1) {
        return false;
    }
    s >> citySizeX >> citySizeZ >> cellCountX >> cellCountZ >> field40 >> field44;
    Uint32 rows = 0;
    s >> rows;
    if (!CountOk(s, rows, 4)) {
        return false;
    }
    grid.assign(rows, {});
    for (auto& row : grid) {
        Uint32 cols = 0;
        s >> cols;
        if (!CountOk(s, cols, 4)) {
            return false;
        }
        row.assign(cols, {});
        for (auto& cell : row) {
            if (!RRefVec(s, cell)) {
                return false;
            }
        }
    }
    return s.Good();
}

bool OccupantManagerSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1} << citySizeX << citySizeZ << cellCountX << cellCountZ << field40 << field44;
    s << static_cast<Uint32>(grid.size());
    for (const auto& row : grid) {
        s << static_cast<Uint32>(row.size());
        for (const auto& cell : row) {
            WRefVec(s, cell);
        }
    }
    return true;
}

bool OccupantManagerSaveRecord::ReadFromSC4Record(std::span<const Uint8> b) noexcept {
    return ReadRecordFromSC4Bytes(b, memAddr, [this](cIGZIStream& s) { return Read(s); });
}

// ===========================================================================
// cSC4TrafficNetworkMap (Read @ 0x10029eeaa / Write @ 0x10029e972)
// ===========================================================================
bool TrafficNetworkMapSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version < 3) {
        return false;
    }
    Uint32 tileCount = 0;
    s >> totalTiles >> tileCount;
    if (!CountOk(s, tileCount, 16)) {
        return false;
    }
    tiles.assign(tileCount, {});
    for (auto& t : tiles) {
        s >> t.cellIndex;
        if (!RRef(s, t.networkOccupantRef)) {
            return false;
        }
        Uint32 laneCount = 0;
        s >> laneCount;
        if (!CountOk(s, laneCount, 8)) {
            return false;
        }
        t.lanes.assign(laneCount, {});
        for (auto& [laneIdx, ticks] : t.lanes) {
            Uint32 n = 0;
            s >> laneIdx >> n;
            if (!CountOk(s, n, 8)) {
                return false;
            }
            ticks.assign(n, {});
            for (auto& e : ticks) {
                s >> e.field0 >> e.field4;
            }
        }
        if (!RRefVec(s, t.automatons)) {
            return false;
        }
        s >> t.fieldBool >> t.field50 >> t.field54 >> t.field58;
        for (std::size_t d = 0; d < 4; ++d) {
            s >> t.dirCapacities[d];
            for (Uint16& w : t.subCapacities[d]) {
                if (version <= 3) {
                    Float32 f = 0.0f;
                    s >> f;
                    w = RoundCapacityToUint16(f);
                } else {
                    s >> w;
                }
            }
        }
        if (version <= 3) {
            s >> t.legacyTailFloat;
            t.tailWord = RoundCapacityToUint16(t.legacyTailFloat);
        } else {
            s >> t.tailWord;
        }
        if (version >= 5) {
            Uint32 n = 0;
            s >> n;
            if (!CountOk(s, n, 10)) {
                return false;
            }
            t.frequencies.assign(n, {});
            for (auto& f : t.frequencies) {
                s >> f.field0 >> f.field4 >> f.field8;
            }
        }
        if (!s.Good()) {
            return false;
        }
    }

    Uint32 nInter = 0;
    s >> nInter;
    if (!CountOk(s, nInter, 4)) {
        return false;
    }
    intersections.assign(nInter, {});
    for (auto& in : intersections) {
        if (!RRef(s, in.networkOccupantRef)) {
            return false;
        }
        if (version > 6) {
            s >> in.field68;
            if (!RRefVec(s, in.automatonRefs)) {
                return false;
            }
        }
        for (auto& side : in.sides) {
            s >> side.field0 >> side.field4 >> side.field8 >> side.fieldC >> side.field10;
        }
    }

    if (version <= 1) {
        if (!RRefVec(s, legacyPostInitRefs)) {
            return false;
        }
    } else {
        Uint32 n = 0;
        s >> n;
        if (!CountOk(s, n, 8)) {
            return false;
        }
        postInitMap.assign(n, {});
        for (auto& [key, refs] : postInitMap) {
            s >> key;
            if (!RRefVec(s, refs)) {
                return false;
            }
        }
    }

    s >> bounds.x0 >> bounds.y0 >> bounds.x1 >> bounds.y1;
    s >> fieldABC >> fieldAC0 >> fieldAC4 >> fieldAC8;

    if (version >= 6) {
        Uint32 n = 0;
        s >> n;
        if (!CountOk(s, n, 4)) {
            return false;
        }
        trainJunctions.assign(n, {});
        for (auto& j : trainJunctions) {
            if (!RRef(s, j.networkOccupantRef)) {
                return false;
            }
            for (auto& side : j.sides) {
                Uint32 m = 0;
                s >> m;
                if (!CountOk(s, m, 4)) {
                    return false;
                }
                side.values.assign(m, 0);
                for (Uint32& v : side.values) {
                    s >> v;
                }
                s >> side.word;
            }
        }
    }
    return s.Good();
}

bool TrafficNetworkMapSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 v) const {
    s << v << totalTiles << static_cast<Uint32>(tiles.size());
    for (const auto& t : tiles) {
        s << t.cellIndex;
        WRef(s, t.networkOccupantRef);
        s << static_cast<Uint32>(t.lanes.size());
        for (const auto& [laneIdx, ticks] : t.lanes) {
            s << laneIdx << static_cast<Uint32>(ticks.size());
            for (const auto& e : ticks) {
                s << e.field0 << e.field4;
            }
        }
        WRefVec(s, t.automatons);
        // Write @ 0x10029e972 always emits false for this flag.
        s << false << t.field50 << t.field54 << t.field58;
        for (std::size_t d = 0; d < 4; ++d) {
            s << t.dirCapacities[d];
            for (Uint16 w : t.subCapacities[d]) {
                if (v <= 3) {
                    s << static_cast<Float32>(w);
                } else {
                    s << w;
                }
            }
        }
        if (v <= 3) {
            s << static_cast<Float32>(t.tailWord);
        } else {
            s << t.tailWord;
        }
        if (v >= 5) {
            s << static_cast<Uint32>(t.frequencies.size());
            for (const auto& f : t.frequencies) {
                s << f.field0 << f.field4 << f.field8;
            }
        }
    }

    s << static_cast<Uint32>(intersections.size());
    for (const auto& in : intersections) {
        WRef(s, in.networkOccupantRef);
        if (v > 6) {
            s << in.field68;
            WRefVec(s, in.automatonRefs);
        }
        for (const auto& side : in.sides) {
            s << side.field0 << side.field4 << side.field8 << side.fieldC << side.field10;
        }
    }

    if (v <= 1) {
        WRefVec(s, legacyPostInitRefs);
    } else {
        s << static_cast<Uint32>(postInitMap.size());
        for (const auto& [key, refs] : postInitMap) {
            s << key;
            WRefVec(s, refs);
        }
    }

    s << bounds.x0 << bounds.y0 << bounds.x1 << bounds.y1;
    s << fieldABC << fieldAC0 << fieldAC4 << fieldAC8;

    if (v >= 6) {
        s << static_cast<Uint32>(trainJunctions.size());
        for (const auto& j : trainJunctions) {
            WRef(s, j.networkOccupantRef);
            for (const auto& side : j.sides) {
                s << static_cast<Uint32>(side.values.size());
                for (Uint32 val : side.values) {
                    s << val;
                }
                s << side.word;
            }
        }
    }
    return true;
}

bool TrafficNetworkMapSaveRecord::Write(cIGZOStream& s) const { return WriteWithVersion(s, 7); }

bool TrafficNetworkMapSaveRecord::ReadFromSC4Record(std::span<const Uint8> b) noexcept {
    return ReadRecordFromSC4Bytes(b, memAddr, [this](cIGZIStream& s) { return Read(s); });
}

// ===========================================================================
// cSC4TripData (Read @ 0x10008dd32 / Write @ 0x10008dc70)
// ===========================================================================
namespace {
// operator>>/operator<< for std::vector<Uint8> (@ 0x1000cf12e): Uint32 count + bytes.
bool RByteVec(cIGZIStream& s, std::vector<Uint8>& v) noexcept {
    Uint32 n = 0;
    s >> n;
    if (!s.Good() || n > s.Remaining()) {
        return false;
    }
    v.resize(n);
    return s.ReadBytes(v.data(), n);
}
void WByteVec(cIGZOStream& s, const std::vector<Uint8>& v) {
    s << static_cast<Uint32>(v.size());
    s.WriteBytes(v.data(), v.size());
}
} // namespace

bool TripDataRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || !RByteVec(s, pathA) || !RByteVec(s, pathB)) {
        return false;
    }
    s >> field30 >> field34 >> field35 >> field36 >> field37 >> field38 >> field3A >> field3C;
    if (version >= 2) {
        s >> field40;
    }
    return s.Good();
}

bool TripDataRecord::Write(cIGZOStream& s) const {
    s << Uint32{2};
    WByteVec(s, pathA);
    WByteVec(s, pathB);
    s << field30 << field34 << field35 << field36 << field37 << field38 << field3A << field3C
      << field40;
    return true;
}

// ===========================================================================
// cSC4Lot (Read @ 0x100182eb2 / Write @ 0x100182af0)
// ===========================================================================
bool LotFullSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 8) {
        return false;
    }
    const bool legacy = version <= 5;
    auto readSmall = [&](Uint8& out) {
        if (legacy) {
            Sint32 v = 0;
            s >> v;
            out = static_cast<Uint8>(v);
        } else {
            s >> out;
        }
    };
    s >> configId >> flagsCD;
    readSmall(minX);
    readSmall(minZ);
    readSmall(maxX);
    readSmall(maxZ);
    readSmall(anchorX);
    readSmall(anchorZ);
    s >> altitude44 >> slope4C >> slope48;
    readSmall(facingD0);
    readSmall(zoneTypeD1);
    s >> wealthD2 >> fieldD3 >> fieldD4 >> fieldD5 >> fieldD6;
    if (legacy) {
        s >> legacyBool1;
    }
    s >> dateCreated60 >> buildingType64;
    if (legacy) {
        s >> legacyBool2 >> legacyBool3 >> legacyBool4;
    }
    s >> fieldD7;
    if (!RRef(s, ref80) || !RRef(s, ref88)) {
        return false;
    }
    Uint8 nMaps = 0;
    s >> nMaps;
    demandSourceMaps.assign(nMaps, {});
    for (auto& m : demandSourceMaps) {
        Uint8 n = 0;
        s >> n;
        m.assign(n, {});
        for (auto& [k, v] : m) {
            s >> k >> v;
        }
    }
    Uint8 nDest = 0;
    s >> nDest;
    travelDestMap.assign(nDest, {});
    for (auto& [k, v] : travelDestMap) {
        s >> k >> v;
    }
    s >> fieldC0 >> fieldC4 >> fieldC8;
    if (!s.Good() || !ReadSCPropertyHolder(s, properties)) {
        return false;
    }
    if (version >= 7) {
        Uint32 n = 0;
        s >> n;
        if (!CountOk(s, n, 8)) {
            return false;
        }
        trips.assign(n, {});
        for (auto& t : trips) {
            if (!t.Read(s)) {
                return false;
            }
        }
    }
    if (version >= 8) {
        s >> fieldF0;
    }
    return s.Good();
}

bool LotFullSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{8} << configId << flagsCD << minX << minZ << maxX << maxZ << anchorX << anchorZ
      << altitude44 << slope4C << slope48 << facingD0 << zoneTypeD1 << wealthD2 << fieldD3
      << fieldD4 << fieldD5 << fieldD6 << dateCreated60 << buildingType64 << fieldD7;
    WRef(s, ref80);
    WRef(s, ref88);
    s << static_cast<Uint8>(demandSourceMaps.size());
    for (const auto& m : demandSourceMaps) {
        s << static_cast<Uint8>(m.size());
        for (const auto& [k, v] : m) {
            s << k << v;
        }
    }
    s << static_cast<Uint8>(travelDestMap.size());
    for (const auto& [k, v] : travelDestMap) {
        s << k << v;
    }
    s << fieldC0 << fieldC4 << fieldC8;
    WriteSCPropertyHolder(s, properties);
    s << static_cast<Uint32>(trips.size());
    for (const auto& t : trips) {
        t.Write(s);
    }
    s << fieldF0;
    return true;
}

// ===========================================================================
// cSCExemplarPropertyHolder & cSC4Occupant
// ===========================================================================
bool ExemplarPropertyHolderSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 2 || !ReadSCPropertyHolder(s, properties)) {
        return false;
    }
    s >> exemplarKey.group >> exemplarKey.type >> exemplarKey.instance;
    return s.Good();
}

bool ExemplarPropertyHolderSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    WriteSCPropertyHolder(s, properties);
    s << exemplarKey.group << exemplarKey.type << exemplarKey.instance;
    return true;
}

bool OccupantBaseSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 4) {
        return false;
    }
    s >> field50 >> viewLevelByte >> flags53 >> occupantType48;
    for (Uint8& b : cellBounds) {
        s >> b;
    }
    return s.Good() && exemplarHolder.Read(s);
}

bool OccupantBaseSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{4} << field50 << viewLevelByte << static_cast<Uint8>(flags53 & 0xddu)
      << occupantType48;
    for (Uint8 b : cellBounds) {
        s << b;
    }
    return exemplarHolder.Write(s);
}

// ===========================================================================
// Building / Flora
// ===========================================================================
bool BuildingOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 2 || !base.Read(s)) {
        return false;
    }
    s >> field68 >> field85;
    minBounds.Read(s);
    maxBounds.Read(s);
    s >> orientation >> field88;
    return s.Good();
}

bool BuildingOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    base.Write(s);
    s << field68 << field85;
    minBounds.Write(s);
    maxBounds.Write(s);
    s << orientation << field88;
    return true;
}

bool FloraOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 3 || !base.Read(s)) {
        return false;
    }
    s >> field68;
    position.Read(s);
    s >> date7C >> date80 >> orientation >> state91 >> floraType6C;
    return s.Good();
}

bool FloraOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{3};
    base.Write(s);
    s << field68;
    position.Write(s);
    s << date7C << date80 << orientation << state91 << floraType6C;
    return true;
}

// ===========================================================================
// cSC4PropOccupant and derived props
// ===========================================================================
bool PropOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || !base.Read(s)) {
        return false;
    }
    s >> propTypeId;
    minBounds.Read(s);
    maxBounds.Read(s);
    s >> orientation >> field9B >> field96 >> field97 >> hasTiming;
    if (hasTiming) {
        s >> timing.field0 >> timing.field4 >> timing.startDateJDN >> timing.stopDateJDN;
    }
    s >> field9A >> field9C >> field90;
    if (version >= 6) {
        s >> field98;
    } else {
        field98 = 0x0f;
    }
    return s.Good();
}

bool PropOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{6};
    base.Write(s);
    s << propTypeId;
    minBounds.Write(s);
    maxBounds.Write(s);
    s << orientation << field9B << field96 << field97 << hasTiming;
    if (hasTiming) {
        s << timing.field0 << timing.field4 << timing.startDateJDN << timing.stopDateJDN;
    }
    s << field9A << field9C << field90 << field98;
    return true;
}

bool PropOccupantVisualEffectSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 3 || !prop.Read(s)) {
        return false;
    }
    s >> fieldE4 >> fieldE5;
    if (!s.ReadGZString(effectName)) {
        return false;
    }
    s >> fieldD8;
    return s.Good();
}

bool PropOccupantVisualEffectSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{3};
    prop.Write(s);
    s << fieldE4 << fieldE5;
    s.WriteGZString(effectName);
    s << fieldD8;
    return true;
}

bool PropOccupantTerrainDecalSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 3 || !prop.Read(s)) {
        return false;
    }
    s >> fieldA8 >> textureKey.type >> textureKey.group >> textureKey.instance >> fieldB8;
    uvOffset.Read(s);
    s >> fieldC4 >> fieldC8 >> fieldCC >> fieldD0 >> fieldD4 >> fieldD5 >> fieldD6;
    return s.Good();
}

bool PropOccupantTerrainDecalSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{3};
    prop.Write(s);
    s << fieldA8 << textureKey.type << textureKey.group << textureKey.instance << fieldB8;
    uvOffset.Write(s);
    s << fieldC4 << fieldC8 << fieldCC << fieldD0 << fieldD4 << fieldD5 << fieldD6;
    return true;
}

bool PropOccupantBurntBuildingSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    return s.Good() && version == 3 && prop.Read(s) && bounds.Read(s);
}

bool PropOccupantBurntBuildingSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{3};
    prop.Write(s);
    return bounds.Write(s);
}

bool PropOccupantAutomataControllerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 3 || !prop.Read(s)) {
        return false;
    }
    s >> fieldD8;
    return s.ReadGZString(controllerName);
}

bool PropOccupantAutomataControllerSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{3};
    prop.Write(s);
    s << fieldD8;
    s.WriteGZString(controllerName);
    return true;
}

// cSC4SignpostOccupant::Read @ 0x100710ae4 / Write @ 0x100710a0c.
bool SignpostOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || !prop.Read(s) || !s.ReadGZString(text)) {
        return false;
    }
    if (version == 1) {
        s >> legacyField;
    }
    labelPos.Read(s);
    return s.Good();
}

bool SignpostOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    prop.Write(s);
    s.WriteGZString(text);
    labelPos.Write(s);
    return true;
}

// cSC4TextDecalOccupant::Read @ 0x1007124b0 / Write @ 0x100712360.
bool TextDecalOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || !prop.Read(s) || !s.ReadGZString(text)) {
        return false;
    }
    if (version <= 2) {
        s >> legacyField;
    }
    s >> preE4 >> preE8;
    if (version == 1) {
        Uint32 v = 0;
        s >> v;
        fieldD0 = static_cast<Float32>(v);
    } else {
        s >> fieldD0;
    }
    s >> fieldE0 >> fieldE4 >> fieldE8 >> fieldEC >> fieldF0 >> fieldF4 >> fieldF8
      >> orientationFC;
    return s.Good();
}

bool TextDecalOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{3};
    prop.Write(s);
    s.WriteGZString(text);
    s << fieldE4 << fieldE8 << fieldD0 << fieldE0 << fieldE4 << fieldE8 << fieldEC << fieldF0
      << fieldF4 << fieldF8 << orientationFC;
    return true;
}

// ===========================================================================
// Lot base texture / foundation / retaining wall / landfill
// ===========================================================================
bool LotBaseTextureOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 2 || !base.Read(s)) {
        return false;
    }
    minBounds.Read(s);
    maxBounds.Read(s);
    Uint32 n = 0;
    s >> field80 >> n;
    if (!CountOk(s, n, 14)) {
        return false;
    }
    specs.assign(n, {});
    for (auto& sp : specs) {
        s >> sp.textureId;
        for (Uint8& b : sp.bytes) {
            s >> b;
        }
    }
    return s.Good();
}

bool LotBaseTextureOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    base.Write(s);
    minBounds.Write(s);
    maxBounds.Write(s);
    s << field80 << static_cast<Uint32>(specs.size());
    for (const auto& sp : specs) {
        s << sp.textureId;
        for (Uint8 b : sp.bytes) {
            s << b;
        }
    }
    return true;
}

bool FoundationOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 2 || !base.Read(s)) {
        return false;
    }
    s >> field68 >> field6C;
    minBounds.Read(s);
    maxBounds.Read(s);
    s >> field88 >> field89;
    return s.Good();
}

bool FoundationOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    base.Write(s);
    s << field68 << field6C;
    minBounds.Write(s);
    maxBounds.Write(s);
    s << field88 << field89;
    return true;
}

bool LotRetainingWallOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 2 || !base.Read(s)) {
        return false;
    }
    s >> field68 >> field78;
    for (Sint32& e : edges7C) {
        s >> e;
    }
    minBounds.Read(s);
    maxBounds.Read(s);
    return s.Good() && RRef(s, occupantRef);
}

bool LotRetainingWallOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    base.Write(s);
    s << field68 << field78;
    for (Sint32 e : edges7C) {
        s << e;
    }
    minBounds.Write(s);
    maxBounds.Write(s);
    WRef(s, occupantRef);
    return true;
}

bool LandfillOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 3 || !base.Read(s)) {
        return false;
    }
    s >> field8C >> field90;
    position.Read(s);
    s >> field94;
    return s.Good();
}

bool LandfillOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{3};
    base.Write(s);
    s << field8C << field90;
    position.Write(s);
    s << field94;
    return true;
}

// ===========================================================================
// cSC4ConstructionOccupant (Read @ 0x100178918 / Write @ 0x1001785ac)
// ===========================================================================
bool ConstructionOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version < 4 || !base.Read(s)) {
        return false;
    }
    if (!RRef(s, buildingRef) || !RRef(s, lotRef) || !transform.Read(s, version <= 4)) {
        return false;
    }
    boundsF0.Read(s);
    bounds108.Read(s);
    s >> field120;
    vec178.Read(s);
    if (!RRefVec(s, list1B0) || !RRefVec(s, list1C8) || !RRefVec(s, vec1E0) ||
        !RRef(s, ref1F8)) {
        return false;
    }
    s >> field200 >> field204 >> field208 >> field20C >> field210 >> field214 >> field218 >>
        field21C >> field220 >> field224 >> field228 >> field22C >> field230 >> field234 >>
        field238 >> field23C >> field240 >> field244 >> field248 >> field24C >> field250 >>
        field1A8;
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 6)) {
        return false;
    }
    automatons.assign(n, {});
    for (auto& a : automatons) {
        s >> a.flag0 >> a.flag1;
        if (!RRef(s, a.automatonRef)) {
            return false;
        }
    }
    return s.Good();
}

bool ConstructionOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{5};
    base.Write(s);
    WRef(s, buildingRef);
    WRef(s, lotRef);
    transform.Write(s);
    boundsF0.Write(s);
    bounds108.Write(s);
    s << field120;
    vec178.Write(s);
    WRefVec(s, list1B0);
    WRefVec(s, list1C8);
    WRefVec(s, vec1E0);
    WRef(s, ref1F8);
    s << field200 << field204 << field208 << field20C << field210 << field214 << field218
      << field21C << field220 << field224 << field228 << field22C << field230 << field234
      << field238 << field23C << field240 << field244 << field248 << field24C << field250
      << field1A8;
    s << static_cast<Uint32>(automatons.size());
    for (const auto& a : automatons) {
        s << a.flag0 << a.flag1;
        WRef(s, a.automatonRef);
    }
    return true;
}

// ===========================================================================
// Toxic / Flame / Fire
// ===========================================================================
bool ToxicOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 1 || !base.Read(s)) {
        return false;
    }
    s >> field68 >> field6C;
    return s.Good();
}

bool ToxicOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1};
    base.Write(s);
    s << field68 << field6C;
    return true;
}

bool FlameSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version < 2) {
        return false;
    }
    s >> field02 >> field04;
    position.Read(s);
    return transform.Read(s, version <= 2);
}

bool FlameSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{3} << field02 << field04;
    position.Write(s);
    return transform.Write(s);
}

bool FireOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 3 || !base.Read(s)) {
        return false;
    }
    s >> field70 >> field74 >> field78;
    if (!RRef(s, occupantRef)) {
        return false;
    }
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 8)) {
        return false;
    }
    flames.assign(n, {});
    for (auto& f : flames) {
        if (!f.Read(s)) {
            return false;
        }
    }
    return s.Good();
}

bool FireOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{3};
    base.Write(s);
    s << field70 << field74 << field78;
    WRef(s, occupantRef);
    s << static_cast<Uint32>(flames.size());
    for (const auto& f : flames) {
        f.Write(s);
    }
    return true;
}

// ===========================================================================
// Power line / pole / pylon
// ===========================================================================
bool PowerLineOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 2 || !base.Read(s)) {
        return false;
    }
    s >> field68 >> field6C >> field70 >> field74 >> field78 >> field90;
    return s.Good() && RRef(s, poleRefA) && RRef(s, poleRefB);
}

bool PowerLineOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    base.Write(s);
    s << field68 << field6C << field70 << field74 << field78 << field90;
    WRef(s, poleRefA);
    WRef(s, poleRefB);
    return true;
}

namespace {
bool RWireLists(cIGZIStream& s, std::vector<std::vector<S3DVector3>>& lists) noexcept {
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 4)) {
        return false;
    }
    lists.assign(n, {});
    for (auto& l : lists) {
        Uint32 m = 0;
        s >> m;
        if (!CountOk(s, m, 12)) {
            return false;
        }
        l.assign(m, {});
        for (auto& p : l) {
            p.Read(s);
        }
    }
    return s.Good();
}
void WWireLists(cIGZOStream& s, const std::vector<std::vector<S3DVector3>>& lists) {
    s << static_cast<Uint32>(lists.size());
    for (const auto& l : lists) {
        s << static_cast<Uint32>(l.size());
        for (const auto& p : l) {
            p.Write(s);
        }
    }
}
} // namespace

bool PowerPoleOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version < 4 || !base.Read(s) || !transform.Read(s, version <= 4)) {
        return false;
    }
    s >> fieldDC >> fieldE0 >> fieldE4;
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 4)) {
        return false;
    }
    connections.assign(n, {});
    for (auto& c : connections) {
        if (!RRef(s, c.poleRef) || !RRefVec(s, c.lineRefs)) {
            return false;
        }
        s >> c.field20 >> c.field21 >> c.field24 >> c.field28;
        if (!RWireLists(s, c.wirePointsA) || !RWireLists(s, c.wirePointsB)) {
            return false;
        }
    }
    s >> field100 >> field110 >> field114 >> field118 >> field11C;
    for (auto& p : connectionPoints) {
        p.Read(s);
    }
    if (!ReadVertexVectorFromStream(s, verts140) || !ReadVertexVectorFromStream(s, verts158) ||
        !ReadVertexVectorFromStream(s, verts170)) {
        return false;
    }
    s >> field188 >> field18C >> field90 >> field91 >> fieldA0 >> field18E;
    return s.Good();
}

bool PowerPoleOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{5};
    base.Write(s);
    transform.Write(s);
    s << fieldDC << fieldE0 << fieldE4 << static_cast<Uint32>(connections.size());
    for (const auto& c : connections) {
        WRef(s, c.poleRef);
        WRefVec(s, c.lineRefs);
        s << c.field20 << c.field21 << c.field24 << c.field28;
        WWireLists(s, c.wirePointsA);
        WWireLists(s, c.wirePointsB);
    }
    s << field100 << field110 << field114 << field118 << field11C;
    for (const auto& p : connectionPoints) {
        p.Write(s);
    }
    WriteVertexVectorToStream(s, verts140);
    WriteVertexVectorToStream(s, verts158);
    WriteVertexVectorToStream(s, verts170);
    s << field188 << field18C << field90 << field91 << fieldA0 << field18E;
    return true;
}

bool PylonOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version < 3 || !base.Read(s)) {
        return false;
    }
    s >> fieldA4 >> fieldA8 >> fieldAC >> fieldB0 >> field1EC >> field194;
    if (!RRef(s, refB8) || !transform.Read(s, version <= 3)) {
        return false;
    }
    s >> field1ED;
    if (version >= 5) {
        s >> field1F0;
        if (!s.ReadGZString(name1F8) || !RRef(s, ref220)) {
            return false;
        }
    }
    return s.Good();
}

bool PylonOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{5};
    base.Write(s);
    s << fieldA4 << fieldA8 << fieldAC << fieldB0 << field1EC << field194;
    WRef(s, refB8);
    transform.Write(s);
    s << field1ED << field1F0;
    s.WriteGZString(name1F8);
    WRef(s, ref220);
    return true;
}

// ===========================================================================
// cSC4EdgeConnectionStore (Read @ 0x10019d5a0 / Write @ 0x10019cf0c)
// ===========================================================================
bool EdgeConnectionStoreSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version >> count >> primaryNetworkType >> primaryEdgeFlags;
    if (!s.Good()) {
        return false;
    }
    entries.assign(count, {});
    for (auto& e : entries) {
        s >> e.networkType >> e.connectionBits;
    }
    return s.Good();
}

bool EdgeConnectionStoreSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2} << static_cast<Uint8>(entries.size()) << primaryNetworkType << primaryEdgeFlags;
    for (const auto& e : entries) {
        s << e.networkType << e.connectionBits;
    }
    return true;
}

// ===========================================================================
// cSC4NetworkOccupant (Read @ 0x10019cfca / Write @ 0x10019cbb2)
// ===========================================================================
bool NetworkOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version < 4 || !base.Read(s) || !transform.Read(s, version <= 4)) {
        return false;
    }
    for (auto& v : cornerVerts) {
        v.Read(s);
    }
    s >> pieceId134 >> orientFlip13D >> variation138 >> wealthTexture13C;
    if (!s.Good() || !edgeStore.Read(s)) {
        return false;
    }
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 4 + 3 * 24)) {
        return false;
    }
    addOnParts.assign(n, {});
    for (auto& p : addOnParts) {
        s >> p.partId;
        for (auto& v : p.verts) {
            v.Read(s);
        }
    }
    s >> minElev170[0] >> maxElev17C[0] >> minElev170[1] >> maxElev17C[1] >> minElev170[2] >>
        maxElev17C[2];
    s >> flags188 >> field1B8;
    if (field1B8 != 0 && version <= 6) {
        s >> legacyFloatA >> legacyFloatB;
    }
    RPersist(s, persistRef190);
    s >> field1C8;
    return s.Good();
}

bool NetworkOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{8};
    base.Write(s);
    transform.Write(s);
    for (const auto& v : cornerVerts) {
        v.Write(s);
    }
    s << pieceId134 << orientFlip13D << variation138 << wealthTexture13C;
    edgeStore.Write(s);
    s << static_cast<Uint32>(addOnParts.size());
    for (const auto& p : addOnParts) {
        s << p.partId;
        for (const auto& v : p.verts) {
            v.Write(s);
        }
    }
    s << minElev170[0] << maxElev17C[0] << minElev170[1] << maxElev17C[1] << minElev170[2]
      << maxElev17C[2];
    s << flags188 << field1B8;
    WPersist(s, persistRef190);
    s << field1C8;
    return true;
}

bool NetworkPreBuiltOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version < 3 || version > 4 || !network.Read(s)) {
        return false;
    }
    s >> field1D0 >> field1D4 >> field1D8;
    if (version < 4) {
        for (Uint32& w : legacyWords) {
            s >> w;
        }
    }
    s >> field208 >> field20C >> field1F4;
    if (field1F4 != 0) {
        for (Float32& f : floats1F8) {
            s >> f;
        }
    }
    if (version <= 3) {
        s >> legacyTailWord;
    }
    return s.Good();
}

bool NetworkPreBuiltOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{4};
    network.Write(s);
    s << field1D0 << field1D4 << field1D8 << field208 << field20C << field1F4;
    if (field1F4 != 0) {
        for (Float32 f : floats1F8) {
            s << f;
        }
    }
    return true;
}

bool NetworkTunnelOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 2 || !preBuilt.Read(s)) {
        return false;
    }
    s >> hasPeer;
    if (hasPeer != 0 && !RRef(s, peerTunnelRef)) {
        return false;
    }
    return s.Good();
}

bool NetworkTunnelOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    preBuilt.Write(s);
    s << hasPeer;
    if (hasPeer != 0) {
        WRef(s, peerTunnelRef);
    }
    return true;
}

// ===========================================================================
// Underground occupants
// ===========================================================================
bool UndergroundOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 3 || !network.Read(s)) {
        return false;
    }
    s >> field88 >> field89 >> field98 >> field1D8;
    for (auto& mv : meshVerts) {
        if (!ReadVertexVectorFromStream(s, mv)) {
            return false;
        }
    }
    matrix2D8.Read(s);
    s >> cellX318 >> cellZ31C >> field328 >> field329 >> field32A;
    for (Float32& f : floats32C) {
        s >> f;
    }
    s >> field33C >> field344 >> field348;
    return s.Good();
}

bool UndergroundOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{3};
    network.Write(s);
    s << field88 << field89 << field98 << field1D8;
    for (const auto& mv : meshVerts) {
        WriteVertexVectorToStream(s, mv);
    }
    matrix2D8.Write(s);
    s << cellX318 << cellZ31C << field328 << field329 << field32A;
    for (Float32 f : floats32C) {
        s << f;
    }
    s << field33C << field344 << field348;
    return true;
}

bool PipeOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 3 || !underground.Read(s)) {
        return false;
    }
    s >> wateredState358;
    return s.Good();
}

bool PipeOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{3};
    underground.Write(s);
    s << wateredState358;
    return true;
}

bool SubwayOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    return s.Good() && version == 2 && underground.Read(s);
}

bool SubwayOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    return underground.Write(s);
}

bool UndergroundConnectionOccupantSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 2 || !base.Read(s)) {
        return false;
    }
    s >> fieldA8;
    for (auto& mv : meshVerts) {
        if (!ReadVertexVectorFromStream(s, mv)) {
            return false;
        }
    }
    s >> field1F8 >> field1FC;
    for (Float32& f : floats210) {
        s >> f;
    }
    s >> field220 >> field228 >> field22C;
    if (!RRef(s, ref168) || !RRef(s, ref170)) {
        return false;
    }
    s >> field2F8;
    for (Float32& f : floats2E0) {
        s >> f;
    }
    return s.Good();
}

bool UndergroundConnectionOccupantSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    base.Write(s);
    s << fieldA8;
    for (const auto& mv : meshVerts) {
        WriteVertexVectorToStream(s, mv);
    }
    s << field1F8 << field1FC;
    for (Float32 f : floats210) {
        s << f;
    }
    s << field220 << field228 << field22C;
    WRef(s, ref168);
    WRef(s, ref170);
    s << field2F8;
    for (Float32 f : floats2E0) {
        s << f;
    }
    return true;
}

// ===========================================================================
// Round-trip verification registry
// ===========================================================================
using sc4_detail::RoundTripMulti;
using sc4_detail::RoundTripRaw;

bool RoundTripSC4Subfile(Uint32 typeId, std::span<const Uint8> b, SC4SubfileRoundTripStats& st) {
    switch (typeId) {
        case kCLSID_SC4AppLoadSaveVersion: RoundTripRaw<SC4AppLoadSaveVersionRecord>(b, st); return true;
        case kCLSID_SC4AppSaveVersionInfo: RoundTripRaw<SC4AppSaveVersionInfoRecord>(b, st); return true;
        case kCLSID_SC4AppCityLink:        RoundTripRaw<SC4AppCityLinkRecord>(b, st); return true;
        case kCLSID_SC4CustomContentRecord: RoundTripRaw<SC4CustomContentRecord>(b, st); return true;
        case kCLSID_cSC4COMSerializer:     RoundTripRaw<COMSerializerSaveRecord>(b, st); return true;
        case kCLSID_cSC4City:              RoundTripMulti<CitySaveRecord>(b, 12, st); return true;
        case kCLSID_cSC4OccupantManager:   RoundTripMulti<OccupantManagerSaveRecord>(b, 1, st); return true;
        case kCLSID_cSC4TrafficNetworkMap: RoundTripMulti<TrafficNetworkMapSaveRecord>(b, 7, st); return true;
        case kCLSID_cSC4Lot:               RoundTripMulti<LotFullSaveRecord>(b, 8, st); return true;
        case kCLSID_cSC4BuildingOccupant:  RoundTripMulti<BuildingOccupantSaveRecord>(b, 2, st); return true;
        case kCLSID_cSC4FloraOccupant:     RoundTripMulti<FloraOccupantSaveRecord>(b, 3, st); return true;
        case kCLSID_cSC4PropOccupant:      RoundTripMulti<PropOccupantSaveRecord>(b, 6, st); return true;
        case kCLSID_cSC4PropVisualEffect:  RoundTripMulti<PropOccupantVisualEffectSaveRecord>(b, 3, st); return true;
        case kCLSID_cSC4PropTerrainDecal:  RoundTripMulti<PropOccupantTerrainDecalSaveRecord>(b, 3, st); return true;
        case kCLSID_cSC4PropBurntBuilding: RoundTripMulti<PropOccupantBurntBuildingSaveRecord>(b, 3, st); return true;
        case kCLSID_cSC4PropAutomataController: RoundTripMulti<PropOccupantAutomataControllerSaveRecord>(b, 3, st); return true;
        case kCLSID_cSC4SignpostOccupant:  RoundTripMulti<SignpostOccupantSaveRecord>(b, 2, st); return true;
        case kCLSID_cSC4TextDecalOccupant: RoundTripMulti<TextDecalOccupantSaveRecord>(b, 3, st); return true;
        case kCLSID_cSC4ToxicOccupant:     RoundTripMulti<ToxicOccupantSaveRecord>(b, 1, st); return true;
        case kCLSID_cSC4LotBaseTexture: RoundTripMulti<LotBaseTextureOccupantSaveRecord>(b, 2, st); return true;
        case kCLSID_cSC4FoundationOccupant: RoundTripMulti<FoundationOccupantSaveRecord>(b, 2, st); return true;
        case kCLSID_cSC4RetainingWall: RoundTripMulti<LotRetainingWallOccupantSaveRecord>(b, 2, st); return true;
        case kCLSID_cSC4LandfillOccupant:  RoundTripMulti<LandfillOccupantSaveRecord>(b, 3, st); return true;
        case kCLSID_cSC4ConstructionOccupant: RoundTripMulti<ConstructionOccupantSaveRecord>(b, 5, st); return true;
        case kCLSID_cSC4FireOccupant:      RoundTripMulti<FireOccupantSaveRecord>(b, 3, st); return true;
        case kCLSID_cSC4PowerLineOccupant: RoundTripMulti<PowerLineOccupantSaveRecord>(b, 2, st); return true;
        case kCLSID_cSC4PowerPoleOccupant: RoundTripMulti<PowerPoleOccupantSaveRecord>(b, 5, st); return true;
        case kCLSID_cSC4PylonOccupant:     RoundTripMulti<PylonOccupantSaveRecord>(b, 5, st); return true;
        case kCLSID_cSC4NetworkOccupant:   RoundTripMulti<NetworkOccupantSaveRecord>(b, 8, st); return true;
        case kCLSID_cSC4NetworkPreBuilt:
        case kCLSID_cSC4NetworkBridge:
            RoundTripMulti<NetworkPreBuiltOccupantSaveRecord>(b, 4, st); return true;
        case kCLSID_cSC4NetworkTunnel: RoundTripMulti<NetworkTunnelOccupantSaveRecord>(b, 2, st); return true;
        case kCLSID_cSC4PipeOccupant:      RoundTripMulti<PipeOccupantSaveRecord>(b, 3, st); return true;
        case kCLSID_cSC4SubwayOccupant:    RoundTripMulti<SubwayOccupantSaveRecord>(b, 2, st); return true;
        case kCLSID_cSC4UndergroundConnOccupant:
        case kCLSID_cSC4SubwayConnOccupant:
        case kCLSID_cSC4PipeConnOccupant:
            RoundTripMulti<UndergroundConnectionOccupantSaveRecord>(b, 2, st); return true;
        default:
            return false;
    }
}

} // namespace opensc4
