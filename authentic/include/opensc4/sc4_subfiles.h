#pragma once

// Exact decompilation of SimCity 4 Deluxe (Sim City 4 Deluxe Editionsub)
// .sc4 save subfiles & cIGZSerializable classes — Batch 1:
//   - SC4AppLoadSaveVersion (0xea3ff4f5, Read @ 0x1001e9148, Write @ 0x1001e9585)
//   - SC4AppSaveVersionInfo (0x0b7d45e0, SaveCity @ 0x100046573)
//   - SC4AppCityLink (0x49dd6f4c, cSC4App::Read @ 0x100044576, Write @ 0x100044510)
//   - SC4CustomContentRecord (0x89d9b9ee, cSC4City::Read @ 0x1006bef7c, Write @ 0x1006be6fd)
//   - cSC4COMSerializer (0x499b23fe, LoadClassObjects @ 0x1003d09b6, SaveClassObjects @ 0x1003d10c4)
//   - cSC4City (0x8990c372, Read @ 0x1006be88c, Write @ 0x1006be1c8)
//   - cSC4OccupantManager (0x098f964d, Read @ 0x1002472f2, Write @ 0x10024712a)
//   - cSC4TrafficNetworkMap (0x6a0f82b2, Read @ 0x10029eeaa, Write @ 0x10029e972)
//   - cSC4TripData (Read @ 0x10008dd32, Write @ 0x10008dc70)
//   - cSC4Lot (0xc9bd5d4a, Read @ 0x100182eb2, Write @ 0x100182af0)
//   - cSCExemplarPropertyHolder (Read @ 0x10004b34a, Write @ 0x10004b0f2)
//   - cSC4Occupant (0xa9bc9ab6, Read @ 0x1000bc86c, Write @ 0x1000bc7a8)
//   - cSC4BuildingOccupant (0xa9bd882d, Read @ 0x10019b350, Write @ 0x10019b28a)
//   - cSC4FloraOccupant (0xa9c05c85, Read @ 0x1000cba0e, Write @ 0x1000cb940)
//   - cSC4PropOccupant (0x2977aa47, Read @ 0x10015e82e, Write @ 0x10015e694)
//   - cSC4PropOccupantVisualEffect (0x2977aa49, Read @ 0x100160ffa, Write @ 0x100160ec2)
//   - cSC4PropOccupantTerrainDecal (Read @ 0x10015fe20, Write @ 0x10015fcea)
//   - cSC4PropOccupantBurntBuilding (Read @ 0x100161f46, Write @ 0x100161ecc)
//   - cSC4PropOccupantAutomataController (Read @ 0x100162a6e, Write @ 0x1001629d4)
//   - cSC4SignpostOccupant (Read @ 0x100710ae4, Write @ 0x100710a0c)
//   - cSC4TextDecalOccupant (Read @ 0x1007124b0, Write @ 0x100712360)
//   - cSC4LotBaseTextureOccupant (0xc97f987c, Read @ 0x1000b3acc, Write @ 0x1000b3926)
//   - cSC4FoundationOccupant (0x49c05c8f, Read @ 0x10016db0a, Write @ 0x10016da4c)
//   - cSC4LotRetainingWallOccupant (0x49c05c9f, Read @ 0x10015cde8, Write @ 0x10015cd1a)
//   - cSC4LandfillOccupant (0x8a22bebe, Read @ 0x1000b3244, Write @ 0x1000b318c)
//   - cSC4ConstructionOccupant (0xa97f909e, Read @ 0x100178918, Write @ 0x1001785ac)
//   - cSC4ToxicOccupant (Read @ 0x1006c7ec6, Write @ 0x1006c7e44)
//   - cSC4Flame (Read @ 0x10045eb08, Write @ 0x10045eaa8)
//   - cSC4FireOccupant (0x29d25f1d, Read @ 0x10046034e, Write @ 0x100460262)
//   - cSC4PowerLineOccupant (0xc9c05c5d, Read @ 0x1003246a6, Write @ 0x1003245c8)
//   - cSC4PowerPoleOccupant (0x09c05c6a, Read @ 0x10032315c, Write @ 0x100322af6)
//   - cSC4PylonOccupant (0x49b6d69b, Read @ 0x10022e92a, Write @ 0x10022e7fc)
//   - cSC4NetworkOccupant (0xc9c05c6e, Read @ 0x10019cfca, Write @ 0x10019cbb2)
//   - cSC4NetworkOccupantWithPreBuiltModel (0x49c1a034 / 0x49cc1bcd, Read @ 0x1001bc5e4, Write @ 0x1001bc4c0)
//   - cSC4NetworkTunnelOccupant (0x8a4bd52b, Read @ 0x1001a2888, Write @ 0x1001a279a)
//   - cSC4UndergroundOccupant (Read @ 0x1001ab0f2, Write @ 0x1001aae32)
//   - cSC4PipeOccupant (0x49c05b9f, Read @ 0x1001a1290, Write @ 0x1001a1218)
//   - cSC4SubwayOccupant (0xca16374f, Read @ 0x1001a19d8, Write @ 0x1001a18e4)
//   - cSC4UndergroundConnectionOccupant (0xca15ed6b / 0xea0a5acf, Read @ 0x1002f0178, Write @ 0x1002efec8)

// cRZRect-style rectangle (4 x T streamed in x0, y0, x1, y1 order).
// Hoisted ABOVE the civic_simulators.h include: tract_developer.h (reached
// from civic_simulators.h) needs this template, and when a TU enters via
// civic_simulators.h first, the include guard below would otherwise block
// re-entry before this definition is reached (include-cycle, Nov 2026).
template <typename T>
struct SC4Rect {
    T x0{};
    T y0{};
    T x1{};
    T y1{};

    bool operator==(const SC4Rect&) const noexcept = default;
};

#include "opensc4/civic_simulators.h"
#include "opensc4/gzcom.h"

#include <array>
#include <cstddef>
#include <cstring>
#include <map>
#include <span>
#include <string>
#include <utility>
#include <vector>

namespace opensc4 {

constexpr GZCLSID kCLSID_cSC4COMSerializer         = 0x499b23feu;
constexpr GZCLSID kCLSID_SC4AppLoadSaveVersion     = 0xea3ff4f5u;
constexpr GZCLSID kCLSID_SC4AppSaveVersionInfo     = 0x0b7d45e0u;
constexpr GZCLSID kCLSID_SC4AppCityLink            = 0x49dd6f4cu;
constexpr GZCLSID kCLSID_SC4CustomContentRecord    = 0x89d9b9eeu;
constexpr GZCLSID kCLSID_cSC4TrafficNetworkMap     = 0x6a0f82b2u;
constexpr GZCLSID kCLSID_cSC4PropVisualEffect      = 0x2977aa49u;
constexpr GZCLSID kCLSID_cSC4ConstructionOccupant  = 0xa97f909eu;
constexpr GZCLSID kCLSID_cSC4FireOccupant          = 0x29d25f1du;
constexpr GZCLSID kCLSID_cSC4SubwayConnOccupant    = 0xca15ed6bu;
constexpr GZCLSID kCLSID_cSC4PipeConnOccupant      = 0xea0a5acfu;
constexpr GZCLSID kCLSID_cSC4UndergroundConnOccupant = 0x8a139184u;
constexpr GZCLSID kCLSID_cSC4PropTerrainDecal      = 0x2977aa48u;
constexpr GZCLSID kCLSID_cSC4PropBurntBuilding     = 0xc9efb016u;
constexpr GZCLSID kCLSID_cSC4PropAutomataController = 0x2a4017a0u;
constexpr GZCLSID kCLSID_cSC4SignpostOccupant      = 0xab72fbb3u;
constexpr GZCLSID kCLSID_cSC4TextDecalOccupant     = 0x8b79c707u;
constexpr GZCLSID kCLSID_cSC4ToxicOccupant         = 0xabf98cb5u;
// Byte-exact verification of a decoded .sc4 subfile: every record is decoded
// (and must consume its body exactly); records whose stream version equals the
// binary's current Write version are re-encoded (including the
// [size][crc][memAddr] header) and compared byte-for-byte.
struct SC4SubfileRoundTripStats {
    Uint32 records = 0;
    Uint32 decoded = 0;
    Uint32 currentVersion = 0;
    Uint32 byteExact = 0;
};

// Returns false if typeId is not handled by a decompiled decoder.
bool RoundTripSC4Subfile(Uint32 typeId, std::span<const Uint8> subfileBytes,
                         SC4SubfileRoundTripStats& stats);

// (SC4Rect template hoisted above the civic_simulators.h include above.)

// ---------------------------------------------------------------------------
// S3D / Math stream primitives (0x1000cf4f4..0x1000cf954)
// ---------------------------------------------------------------------------
struct S3DVector2 {
    Float32 x = 0.0f;
    Float32 y = 0.0f;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct S3DVector3 {
    Float32 x = 0.0f;
    Float32 y = 0.0f;
    Float32 z = 0.0f;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct S3DBoundingBox {
    S3DVector3 min{};
    S3DVector3 max{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct S3DMatrix4 {
    std::array<Float32, 16> m{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cS3DTransform (operator>> @ 0x1000cf71d, operator<< @ 0x1000cf895,
// S3DTransformLegacyLoad @ 0x1000cf954).
struct S3DTransform {
    Uint8 flags = 0; // bit 2 (0x04) = 3x3 rotation, bit 0 (0x01) = translation, bit 1 (0x02) = scale
    std::array<Float32, 9> rotation{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    S3DVector3 translation{};
    Float32 scale = 1.0f;

    bool Read(cIGZIStream& stream, bool legacyLoad = false) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// sVF_V3F_C4UB_T2F vertex format used by cSC4UndergroundOccupant,
// cSC4UndergroundConnectionOccupant, cSC4PowerPoleOccupant, and
// cSC4NetworkOccupant (ReadFromStream @ 0x1001ab2c2, WriteToStream @ 0x1001aafc9).
// Stream order per vertex: Float32 x, y, z, u, v, then Uint8 r, g, b, a.
struct S3DVertexV3FC4UBT2F {
    Float32 x = 0.0f;
    Float32 y = 0.0f;
    Float32 z = 0.0f;
    Float32 u = 0.0f;
    Float32 v = 0.0f;
    Uint8 r = 255;
    Uint8 g = 255;
    Uint8 b = 255;
    Uint8 a = 255;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

bool ReadVertexVectorFromStream(cIGZIStream& stream,
                                std::vector<S3DVertexV3FC4UBT2F>& out) noexcept;
bool WriteVertexVectorToStream(cIGZOStream& stream,
                               std::span<const S3DVertexV3FC4UBT2F> verts);

// ---------------------------------------------------------------------------
// App-level .sc4 save metadata subfiles (no 12-byte [size][crc][mem] header)
// ---------------------------------------------------------------------------

// 0xea3ff4f5: SC4ReadAppLoadSaveVersion @ 0x1001e9148 /
// SC4WriteAppLoadSaveVersion @ 0x1001e9585.
struct SC4AppLoadSaveVersionRecord {
    Uint32 version = 0x42u; // 65 (0x41) or 66 (0x42)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromBytes(std::span<const Uint8> bytes) noexcept;
};

// 0x0b7d45e0: cSC4App::SaveCity @ 0x100046573 (cSC4AutoSaveStream version 3 +
// 4 x Uint32 cRZVersion::GetModuleVersion(0..3)).
struct SC4AppSaveVersionInfoRecord {
    Uint16 version = 3;
    std::array<Uint32, 4> moduleVersions{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromBytes(std::span<const Uint8> bytes) noexcept;
};

// 0x49dd6f4c: cSC4App::LoadCity @ 0x1000457a5 / SaveCity @ 0x10004665a
// (cSC4AutoSaveStream version 1 + cSC4App::Write @ 0x100044510 version 2 +
// SimSerializableRef to cSC4City).
struct SC4AppCityLinkRecord {
    Uint16 streamVersion = 1;
    Uint16 appVersion = 2;
    SimSerializableRef cityRef{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromBytes(std::span<const Uint8> bytes) noexcept;
};

// 0x89d9b9ee: cSC4City::Read @ 0x1006bef7c / Write @ 0x1006be6fd
// (cSC4AutoSaveRecord version 1 + Uint32 size + raw bytes from spCCM).
struct SC4CustomContentRecord {
    Uint16 version = 1;
    std::vector<Uint8> payload;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromBytes(std::span<const Uint8> bytes) noexcept;
};

// 0x499b23fe: cSC4COMSerializer::LoadClassObjects @ 0x1003d09b6 /
// SaveClassObjects @ 0x1003d10c4.
struct COMSerializerSaveRecord {
    Uint32 version = 1;
    Uint32 classCount = 0;
    Sint32 totalObjectCount = 0;
    std::vector<std::pair<Uint32, Uint32>> classes; // (clsid, objectCount)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromBytes(std::span<const Uint8> bytes) noexcept;
};

// ---------------------------------------------------------------------------
// cSC4City (0x8990c372, Read @ 0x1006be88c, Write @ 0x1006be1c8)
// ---------------------------------------------------------------------------
struct CitySaveRecord {
    Uint16 version = 12;
    Uint32 memAddr = 0;
    SerializedPropertyHolder properties; // +0x18
    Uint32 field50 = 0;                  // +0x50
    Uint32 field54 = 0;                  // +0x54
    Sint32 field58 = 0;                  // +0x58
    Sint32 field5C = 0;                  // +0x5c
    Sint32 field60 = 0;                  // +0x60
    Sint32 field64 = 0;                  // +0x64
    Uint32 field68 = 0;                  // +0x68
    Uint8 field45 = 0;                   // +0x45
    Uint8 field48 = 0;                   // +0x48 (streamed in version >= 10)
    std::array<std::string, 6> strings{}; // +0xc0, +0xe8, +0x110, +0x138, +0x160, +0x188
    Float32 field1B0 = 0.0f;             // +0x1b0
    Float32 field1B4 = 0.0f;             // +0x1b4
    Float32 field1B8 = 0.0f;             // +0x1b8
    Uint32 field1BC = 0;                 // +0x1bc
    std::array<Float32, 6> floats1C4{};  // +0x1c4..+0x1d8
    Sint32 field1DC = 0;                 // +0x1dc
    Sint32 field1E0 = 0;                 // +0x1e0
    std::array<SimSerializableRef, 50> baseSubsystemRefs{};
    SimSerializableRef ref250{};         // +0x250 (version >= 9)
    SimSerializableRef ref380{};         // +0x380 (version >= 11)
    SimSerializableRef ref3A8{};         // +0x3a8 (version >= 12)
    Uint8 field3B0 = 0;                  // +0x3b0

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool WriteWithVersion(cIGZOStream& stream, Uint16 targetVersion) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;
};

// ---------------------------------------------------------------------------
// cSC4OccupantManager (0x098f964d, Read @ 0x1002472f2, Write @ 0x10024712a)
// ---------------------------------------------------------------------------
struct OccupantManagerSaveRecord {
    Uint16 version = 1;
    Uint32 memAddr = 0;
    Float32 citySizeX = 0.0f; // +0x30
    Float32 citySizeZ = 0.0f; // +0x34
    Sint32 cellCountX = 0;    // +0x38
    Sint32 cellCountZ = 0;    // +0x3c
    Sint32 field40 = 0;       // +0x40
    Sint32 field44 = 0;       // +0x44
    std::vector<std::vector<std::vector<SimSerializableRef>>> grid;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;
};

// ---------------------------------------------------------------------------
// cSC4TrafficNetworkMap (0x6a0f82b2, Read @ 0x10029eeaa, Write @ 0x10029e972)
// ---------------------------------------------------------------------------
struct TrafficNetworkLaneTickEntry {
    Uint32 field0 = 0;
    Float32 field4 = 0.0f;
};

struct TrafficNetworkFrequencyEntry {
    Sint32 field0 = 0;
    Float32 field4 = 0.0f;
    Uint16 field8 = 0;
};

struct TrafficNetworkTileInfoRecord {
    Uint32 cellIndex = 0;
    SimSerializableRef networkOccupantRef{}; // IID 0xa821ef94
    std::vector<std::pair<Uint32, std::vector<TrafficNetworkLaneTickEntry>>> lanes;
    std::vector<SimSerializableRef> automatons; // IID 0xa9b40f05
    bool fieldBool = false;
    Uint32 field50 = 0;
    Uint32 field54 = 0;
    Uint32 field58 = 0;
    std::array<Float32, 4> dirCapacities{};
    std::array<std::array<Uint16, 4>, 4> subCapacities{};
    Uint16 tailWord = 0;
    Float32 legacyTailFloat = 0.0f;
    std::vector<TrafficNetworkFrequencyEntry> frequencies; // version >= 5
};

struct TrafficNetworkIntersectionSide {
    Uint32 field0 = 0;
    bool field4 = false;
    Float32 field8 = 0.0f;
    Float32 fieldC = 0.0f;
    Float32 field10 = 0.0f;
};

struct TrafficNetworkIntersectionRecord {
    SimSerializableRef networkOccupantRef{}; // IID 0xa821ef94
    // version > 6. Streamed as a raw byte: the binary writes the in-memory
    // bool storage verbatim, and shipped saves contain non-0/1 values here.
    Uint8 field68 = 0;
    std::vector<SimSerializableRef> automatonRefs; // version > 6, IID 0x8b99ee7c
    std::array<TrafficNetworkIntersectionSide, 4> sides{};
};

struct TrafficNetworkTrainJunctionSide {
    std::vector<Uint32> values;
    Uint16 word = 0;
};

struct TrafficNetworkTrainJunctionRecord {
    SimSerializableRef networkOccupantRef{}; // IID 0xa821ef94
    std::array<TrafficNetworkTrainJunctionSide, 4> sides{};
};

struct TrafficNetworkMapSaveRecord {
    Uint16 version = 7;
    Uint32 memAddr = 0;
    Uint32 totalTiles = 0;
    std::vector<TrafficNetworkTileInfoRecord> tiles;
    std::vector<TrafficNetworkIntersectionRecord> intersections;
    std::vector<SimSerializableRef> legacyPostInitRefs; // version <= 1
    std::vector<std::pair<Uint32, std::vector<SimSerializableRef>>> postInitMap; // version >= 2
    SC4Rect<Sint32> bounds{}; // +0xa4..+0xb0
    Float32 fieldABC = 0.0f;  // +0xabc
    bool fieldAC0 = false;    // +0xac0
    Float32 fieldAC4 = 0.0f;  // +0xac4
    bool fieldAC8 = false;    // +0xac8
    std::vector<TrafficNetworkTrainJunctionRecord> trainJunctions; // version >= 6

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool WriteWithVersion(cIGZOStream& stream, Uint16 targetVersion) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;
};

// ---------------------------------------------------------------------------
// cSC4TripData (Read @ 0x10008dd32, Write @ 0x10008dc70) &
// cSC4Lot (0xc9bd5d4a, Read @ 0x100182eb2, Write @ 0x100182af0)
// ---------------------------------------------------------------------------
struct TripDataRecord {
    Uint32 version = 2;
    std::vector<Uint8> pathA; // +0x00
    std::vector<Uint8> pathB; // +0x18
    Uint32 field30 = 0;       // +0x30
    Uint8 field34 = 0;        // +0x34
    Uint8 field35 = 0;        // +0x35
    Uint8 field36 = 0;        // +0x36
    Uint8 field37 = 0;        // +0x37
    Sint16 field38 = 0;       // +0x38
    Sint16 field3A = 0;       // +0x3a
    Float32 field3C = 0.0f;   // +0x3c
    Uint32 field40 = 0;       // +0x40 (version >= 2)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct LotFullSaveRecord {
    Uint16 version = 8;
    Uint32 memAddr = 0;
    Uint32 configId = 0;            // +0x50
    Uint8 flagsCD = 0;              // +0xcd
    Uint8 minX = 0;                 // +0x40
    Uint8 minZ = 0;                 // +0x41
    Uint8 maxX = 0;                 // +0x42
    Uint8 maxZ = 0;                 // +0x43
    // A cell inside [min, max] (usually a corner), not the lot's footprint.
    Uint8 anchorX = 0;              // +0xce
    Uint8 anchorZ = 0;              // +0xcf
    Float32 altitude44 = 0.0f;      // +0x44
    Float32 slope4C = 0.0f;         // +0x4c
    Float32 slope48 = 0.0f;         // +0x48
    Uint8 facingD0 = 0;             // +0xd0
    Uint8 zoneTypeD1 = 0;           // +0xd1
    Uint8 wealthD2 = 0;             // +0xd2
    Uint8 fieldD3 = 0;              // +0xd3
    Uint8 fieldD4 = 0;              // +0xd4
    Uint8 fieldD5 = 0;              // +0xd5
    Uint8 fieldD6 = 0;              // +0xd6
    bool legacyBool1 = false;       // version <= 5
    Uint32 dateCreated60 = 0;       // +0x60
    Uint32 buildingType64 = 0;      // +0x64
    bool legacyBool2 = false;       // version <= 5
    bool legacyBool3 = false;       // version <= 5
    bool legacyBool4 = false;       // version <= 5
    Uint8 fieldD7 = 0;              // +0xd7
    SimSerializableRef ref80{};     // +0x80, IID 0x6a233069
    SimSerializableRef ref88{};     // +0x88, IID 0xe818fced
    std::vector<std::vector<std::pair<Uint32, Uint16>>> demandSourceMaps; // +0x90
    std::vector<std::pair<Uint32, Uint16>> travelDestMap;                 // +0xa8
    Float32 fieldC0 = 0.0f;         // +0xc0
    Float32 fieldC4 = 0.0f;         // +0xc4
    Float32 fieldC8 = 0.0f;         // +0xc8
    SerializedPropertyHolder properties; // +0x10
    std::vector<TripDataRecord> trips;   // +0xd8 (version >= 7)
    Uint8 fieldF0 = 0;              // +0xf0 (version >= 8)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// ---------------------------------------------------------------------------
// cSCExemplarPropertyHolder (Read @ 0x10004b34a, Write @ 0x10004b0f2) &
// cSC4Occupant (0xa9bc9ab6, Read @ 0x1000bc86c, Write @ 0x1000bc7a8)
// ---------------------------------------------------------------------------
struct ExemplarPropertyHolderSaveRecord {
    Uint16 version = 2;
    SerializedPropertyHolder properties;
    cSC4TGI exemplarKey{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct OccupantBaseSaveRecord {
    Uint16 version = 4;
    Uint16 field50 = 0;             // +0x50
    Uint8 viewLevelByte = 0;        // +0x52 (stream byte)
    Uint8 flags53 = 0;              // +0x53
    Uint32 occupantType48 = 0;      // +0x48
    std::array<Uint8, 4> cellBounds{}; // +0x4c..+0x4f
    ExemplarPropertyHolderSaveRecord exemplarHolder;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// ---------------------------------------------------------------------------
// cSC4BuildingOccupant (0xa9bd882d, Read @ 0x10019b350, Write @ 0x10019b28a)
// ---------------------------------------------------------------------------
struct BuildingOccupantSaveRecord {
    Uint16 version = 2;
    Uint32 memAddr = 0;
    OccupantBaseSaveRecord base;
    Uint32 field68 = 0;     // +0x68
    Uint8 field85 = 0;      // +0x85
    S3DVector3 minBounds{}; // +0x6c
    S3DVector3 maxBounds{}; // +0x78
    Sint8 orientation = 0;  // +0x84
    Float32 field88 = 0.0f; // +0x88

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// ---------------------------------------------------------------------------
// cSC4FloraOccupant (0xa9c05c85, Read @ 0x1000cba0e, Write @ 0x1000cb940)
// ---------------------------------------------------------------------------
struct FloraOccupantSaveRecord {
    Uint16 version = 3;
    Uint32 memAddr = 0;
    OccupantBaseSaveRecord base;
    Uint32 field68 = 0;     // +0x68
    S3DVector3 position{};  // +0x70
    Uint32 date7C = 0;      // +0x7c
    Uint32 date80 = 0;      // +0x80
    Sint8 orientation = 0;  // +0x90
    Uint8 state91 = 0;      // +0x91
    Uint32 floraType6C = 0; // +0x6c

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// ---------------------------------------------------------------------------
// cSC4PropOccupant (0x2977aa47, Read @ 0x10015e82e, Write @ 0x10015e694) &
// derived PropOccupant classes
// ---------------------------------------------------------------------------
struct PropTimingData {
    Uint32 field0 = 0;
    Uint32 field4 = 0;
    Uint32 startDateJDN = 0;
    Uint32 stopDateJDN = 0;
};

struct PropOccupantSaveRecord {
    Uint16 version = 6;
    Uint32 memAddr = 0;
    OccupantBaseSaveRecord base;
    Uint32 propTypeId = 0;
    S3DVector3 minBounds{}; // +0x70
    S3DVector3 maxBounds{}; // +0x7c
    Sint8 orientation = 0;  // +0x99
    Uint8 field9B = 0;      // +0x9b
    Uint8 field96 = 0;      // +0x96
    Uint8 field97 = 0;      // +0x97
    bool hasTiming = false; // +0x88 != nullptr
    PropTimingData timing{};
    Uint8 field9A = 0;      // +0x9a
    Uint8 field9C = 0;      // +0x9c
    Uint32 field90 = 0;     // +0x90
    Uint8 field98 = 0x0f;   // +0x98 (version >= 6)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4PropOccupantVisualEffect (0x2977aa49, Read @ 0x100160ffa, Write @ 0x100160ec2)
struct PropOccupantVisualEffectSaveRecord {
    Uint16 version = 3;
    Uint32 memAddr = 0;
    PropOccupantSaveRecord prop;
    Uint8 fieldE4 = 0;
    Uint8 fieldE5 = 0;
    std::string effectName;
    Uint32 fieldD8 = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4PropOccupantTerrainDecal (Read @ 0x10015fe20, Write @ 0x10015fcea)
struct PropOccupantTerrainDecalSaveRecord {
    Uint16 version = 3;
    Uint32 memAddr = 0;
    PropOccupantSaveRecord prop;
    Uint32 fieldA8 = 0;
    cSC4TGI textureKey{};
    Float32 fieldB8 = 0.0f;
    S3DVector2 uvOffset{};
    Float32 fieldC4 = 0.0f;
    Float32 fieldC8 = 0.0f;
    Float32 fieldCC = 0.0f;
    Float32 fieldD0 = 0.0f;
    Sint8 fieldD4 = 0;
    Uint8 fieldD5 = 0;
    Uint8 fieldD6 = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4PropOccupantBurntBuilding (Read @ 0x100161f46, Write @ 0x100161ecc)
struct PropOccupantBurntBuildingSaveRecord {
    Uint16 version = 3;
    Uint32 memAddr = 0;
    PropOccupantSaveRecord prop;
    S3DBoundingBox bounds{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4PropOccupantAutomataController (Read @ 0x100162a6e, Write @ 0x1001629d4)
struct PropOccupantAutomataControllerSaveRecord {
    Uint16 version = 3;
    Uint32 memAddr = 0;
    PropOccupantSaveRecord prop;
    bool fieldD8 = false;
    std::string controllerName;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4SignpostOccupant (Read @ 0x100710ae4, Write @ 0x100710a0c)
struct SignpostOccupantSaveRecord {
    Uint16 version = 2;
    Uint32 memAddr = 0;
    PropOccupantSaveRecord prop;
    std::string text;
    Uint32 legacyField = 0; // version == 1
    S3DVector3 labelPos{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4TextDecalOccupant (Read @ 0x1007124b0, Write @ 0x100712360)
struct TextDecalOccupantSaveRecord {
    Uint16 version = 3;
    Uint32 memAddr = 0;
    PropOccupantSaveRecord prop;
    std::string text;       // +0xa8
    Uint32 legacyField = 0; // version <= 2
    // Read streams +0xe4/+0xe8 twice (the first pair is overwritten); Write
    // emits the final +0xe4/+0xe8 values in both positions.
    Float32 preE4 = 0.0f;
    Float32 preE8 = 0.0f;
    Float32 fieldD0 = 0.0f; // version == 1: Uint32 converted to float
    Float32 fieldE0 = 0.0f;
    Float32 fieldE4 = 0.0f;
    Float32 fieldE8 = 0.0f;
    Float32 fieldEC = 0.0f;
    Float32 fieldF0 = 0.0f;
    Float32 fieldF4 = 0.0f;
    Float32 fieldF8 = 0.0f;
    Uint8 orientationFC = 0; // +0xfc (Uint8 in stream, widened to Uint32 on Read)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// ---------------------------------------------------------------------------
// cSC4LotBaseTextureOccupant (0xc97f987c, Read @ 0x1000b3acc, Write @ 0x1000b3926)
// ---------------------------------------------------------------------------
struct LotBaseTextureSpec {
    Uint32 textureId = 0;
    std::array<Uint8, 10> bytes{};
};

struct LotBaseTextureOccupantSaveRecord {
    Uint16 version = 2;
    Uint32 memAddr = 0;
    OccupantBaseSaveRecord base;
    S3DVector3 minBounds{}; // +0x68
    S3DVector3 maxBounds{}; // +0x74
    Sint8 field80 = 0;      // +0x80
    std::vector<LotBaseTextureSpec> specs;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// ---------------------------------------------------------------------------
// cSC4FoundationOccupant (0x49c05c8f, Read @ 0x10016db0a, Write @ 0x10016da4c)
// ---------------------------------------------------------------------------
struct FoundationOccupantSaveRecord {
    Uint16 version = 2;
    Uint32 memAddr = 0;
    OccupantBaseSaveRecord base;
    Uint32 field68 = 0;     // +0x68
    Float32 field6C = 0.0f; // +0x6c
    S3DVector3 minBounds{}; // +0x70
    S3DVector3 maxBounds{}; // +0x7c
    Sint8 field88 = 0;      // +0x88
    Sint8 field89 = 0;      // +0x89

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// ---------------------------------------------------------------------------
// cSC4LotRetainingWallOccupant (0x49c05c9f, Read @ 0x10015cde8, Write @ 0x10015cd1a)
// ---------------------------------------------------------------------------
struct LotRetainingWallOccupantSaveRecord {
    Uint16 version = 2;
    Uint32 memAddr = 0;
    OccupantBaseSaveRecord base;
    Uint32 field68 = 0;              // +0x68
    Sint32 field78 = 0;              // +0x78
    std::array<Sint32, 4> edges7C{}; // +0x7c..+0x88
    S3DVector3 minBounds{};          // +0x8c
    S3DVector3 maxBounds{};          // +0x98
    SimSerializableRef occupantRef{}; // +0x70, IID 0xe818fced

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// ---------------------------------------------------------------------------
// cSC4LandfillOccupant (0x8a22bebe, Read @ 0x1000b3244, Write @ 0x1000b318c)
// ---------------------------------------------------------------------------
struct LandfillOccupantSaveRecord {
    Uint16 version = 3;
    Uint32 memAddr = 0;
    OccupantBaseSaveRecord base;
    Uint32 field8C = 0;     // +0x8c
    Sint32 field90 = 0;     // +0x90
    S3DVector3 position{};  // +0x68
    Uint8 field94 = 0;      // +0x94

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// ---------------------------------------------------------------------------
// cSC4ConstructionOccupant (0xa97f909e, Read @ 0x100178918, Write @ 0x1001785ac)
// ---------------------------------------------------------------------------
struct ConstructionAutomatonEntry {
    bool flag0 = false;
    bool flag1 = false;
    SimSerializableRef automatonRef{}; // IID 0xa9b40f05
};

struct ConstructionOccupantSaveRecord {
    Uint16 version = 5;
    Uint32 memAddr = 0;
    OccupantBaseSaveRecord base;
    SimSerializableRef buildingRef{}; // +0xa8, IID 0xe818fced
    SimSerializableRef lotRef{};      // +0xb0, IID 0x87dda3a9
    S3DTransform transform{};         // +0xb8
    S3DBoundingBox boundsF0{};        // +0xf0
    S3DBoundingBox bounds108{};       // +0x108
    bool field120 = false;            // +0x120
    S3DVector3 vec178{};              // +0x178
    std::vector<SimSerializableRef> list1B0; // IID 0xe9793a65
    std::vector<SimSerializableRef> list1C8; // IID 0xe9793a65
    std::vector<SimSerializableRef> vec1E0;  // IID 0x2977aa49
    SimSerializableRef ref1F8{};             // IID 0x2977aa49
    Sint8 field200 = 0;
    Uint32 field204 = 0;
    Uint32 field208 = 0;
    Uint32 field20C = 0;
    Uint32 field210 = 0;
    Uint32 field214 = 0;
    Float32 field218 = 0.0f;
    Float32 field21C = 0.0f;
    Uint32 field220 = 0;
    Uint32 field224 = 0;
    Uint32 field228 = 0;
    Uint32 field22C = 0;
    Uint32 field230 = 0;
    Sint8 field234 = 0;
    Float32 field238 = 0.0f;
    Float32 field23C = 0.0f;
    bool field240 = false;
    Uint32 field244 = 0;
    Uint32 field248 = 0;
    Float32 field24C = 0.0f;
    Float32 field250 = 0.0f;
    Uint8 field1A8 = 0;
    std::vector<ConstructionAutomatonEntry> automatons;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// ---------------------------------------------------------------------------
// cSC4ToxicOccupant (Read @ 0x1006c7ec6, Write @ 0x1006c7e44) &
// cSC4Flame / cSC4FireOccupant (0x29d25f1d, Read @ 0x10046034e, Write @ 0x100460262)
// ---------------------------------------------------------------------------
struct ToxicOccupantSaveRecord {
    Uint16 version = 1;
    Uint32 memAddr = 0;
    OccupantBaseSaveRecord base;
    Uint32 field68 = 0;
    Uint32 field6C = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct FlameSaveRecord {
    Uint16 version = 3;
    bool field02 = false;
    Uint32 field04 = 0;
    S3DVector3 position{};
    S3DTransform transform{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct FireOccupantSaveRecord {
    Uint16 version = 3;
    Uint32 memAddr = 0;
    OccupantBaseSaveRecord base;
    Uint32 field70 = 0;
    Uint32 field74 = 0;
    Uint32 field78 = 0;
    SimSerializableRef occupantRef{}; // IID 0x26bb767e
    std::vector<FlameSaveRecord> flames;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// ---------------------------------------------------------------------------
// cSC4PowerLineOccupant (0xc9c05c5d), cSC4PowerPoleOccupant (0x09c05c6a),
// cSC4PylonOccupant (0x49b6d69b)
// ---------------------------------------------------------------------------
struct PowerLineOccupantSaveRecord {
    Uint16 version = 2;
    Uint32 memAddr = 0;
    OccupantBaseSaveRecord base;
    Float32 field68 = 0.0f;
    Float32 field6C = 0.0f;
    Float32 field70 = 0.0f;
    Sint32 field74 = 0;
    Sint32 field78 = 0;
    bool field90 = false;
    SimSerializableRef poleRefA{}; // +0x80, IID 0xa8c86142
    SimSerializableRef poleRefB{}; // +0x88, IID 0xa8c86142

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct PowerPoleConnectionRecord {
    SimSerializableRef poleRef{}; // IID 0x09c05c6a
    std::vector<SimSerializableRef> lineRefs; // IID 0xc9c05c5d
    bool field20 = false;
    bool field21 = false;
    Sint32 field24 = 0;
    Sint32 field28 = 0;
    std::vector<std::vector<S3DVector3>> wirePointsA; // +0x30
    std::vector<std::vector<S3DVector3>> wirePointsB; // +0x48
};

struct PowerPoleOccupantSaveRecord {
    Uint16 version = 5;
    Uint32 memAddr = 0;
    OccupantBaseSaveRecord base;
    S3DTransform transform{};   // +0xa4
    Float32 fieldDC = 0.0f;     // +0xdc
    Float32 fieldE0 = 0.0f;     // +0xe0
    Float32 fieldE4 = 0.0f;     // +0xe4
    std::vector<PowerPoleConnectionRecord> connections; // +0xe8
    Uint32 field100 = 0;        // +0x100
    Sint32 field110 = 0;        // +0x110
    Sint32 field114 = 0;        // +0x114
    Sint32 field118 = 0;        // +0x118
    Sint32 field11C = 0;        // +0x11c
    std::array<S3DVector3, 4> connectionPoints{}; // +0x138 (4 x 12 = 48 bytes)
    std::vector<S3DVertexV3FC4UBT2F> verts140;    // +0x140
    std::vector<S3DVertexV3FC4UBT2F> verts158;    // +0x158
    std::vector<S3DVertexV3FC4UBT2F> verts170;    // +0x170
    Float32 field188 = 0.0f;    // +0x188
    bool field18C = false;      // +0x18c
    Uint8 field90 = 0;          // +0x90
    Uint8 field91 = 0;          // +0x91
    Float32 fieldA0 = 0.0f;     // +0xa0
    Uint8 field18E = 0;         // +0x18e

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct PylonOccupantSaveRecord {
    Uint16 version = 5;
    Uint32 memAddr = 0;
    OccupantBaseSaveRecord base;
    bool fieldA4 = false;
    Sint8 fieldA8 = 0;
    Sint8 fieldAC = 0;
    Sint8 fieldB0 = 0;
    bool field1EC = false;
    Uint16 field194 = 0;
    SimSerializableRef refB8{}; // IID 0x26bb767e
    S3DTransform transform{};   // +0xc0
    bool field1ED = false;
    Sint32 field1F0 = 0;        // version >= 5
    std::string name1F8;        // version >= 5
    SimSerializableRef ref220{}; // version >= 5, IID 0x1

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// ---------------------------------------------------------------------------
// cSC4NetworkOccupant (0xc9c05c6e) & derived Network/Underground Occupants
// ---------------------------------------------------------------------------
struct EdgeConnectionEntryRecord {
    Uint8 networkType = 0;      // cISC4NetworkManager::eNetworkType
    Uint32 connectionBits = 0;  // per-edge connection bytes
};

struct EdgeConnectionStoreSaveRecord {
    Uint16 version = 2;
    Uint8 count = 0;                 // +0x140: number of secondary entries
    Uint8 primaryNetworkType = 0;    // +0x144: cISC4NetworkManager::eNetworkType
    Uint32 primaryEdgeFlags = 0;     // +0x148: per-edge connection bytes for the primary type
    std::vector<EdgeConnectionEntryRecord> entries; // +0x150: secondary {type, edgeFlags}

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct NetworkAddOnPartRecord {
    Uint32 partId = 0;
    std::array<S3DVertexV3FC4UBT2F, 3> verts{};
};

struct NetworkOccupantSaveRecord {
    Uint16 version = 8;
    Uint32 memAddr = 0;
    OccupantBaseSaveRecord base;
    S3DTransform transform{};                         // +0x9c
    std::array<S3DVertexV3FC4UBT2F, 4> cornerVerts{}; // +0xd4..+0x133
    Uint32 pieceId134 = 0;                            // +0x134
    Uint8 orientFlip13D = 0;                          // +0x13d
    Uint32 variation138 = 0;                          // +0x138
    Uint8 wealthTexture13C = 0;                       // +0x13c
    EdgeConnectionStoreSaveRecord edgeStore{};        // +0x140
    std::vector<NetworkAddOnPartRecord> addOnParts;   // +0x158
    std::array<Float32, 3> minElev170{};              // +0x170..+0x178
    std::array<Float32, 3> maxElev17C{};              // +0x17c..+0x184
    Uint32 flags188 = 0;                              // +0x188
    Uint32 field1B8 = 0;                              // +0x1b8
    Float32 legacyFloatA = 0.0f;                      // version <= 6 && field1B8 != 0
    Float32 legacyFloatB = 0.0f;                      // version <= 6 && field1B8 != 0
    cSC4TGI persistRef190{};                          // +0x190 (SC4ReadGZPersistRef)
    Uint64 field1C8 = 0;                              // +0x1c8

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4NetworkOccupantWithPreBuiltModel (0x49c1a034) & cSC4NetworkBridgeOccupant (0x49cc1bcd)
struct NetworkPreBuiltOccupantSaveRecord {
    Uint16 version = 4;
    Uint32 memAddr = 0;
    NetworkOccupantSaveRecord network;
    Float32 field1D0 = 0.0f;
    Float32 field1D4 = 0.0f;
    Float32 field1D8 = 0.0f;
    std::array<Uint32, 6> legacyWords{}; // version < 4
    Uint32 field208 = 0;
    Uint8 field20C = 0;
    Uint32 field1F4 = 0;
    std::array<Float32, 4> floats1F8{};  // present when field1F4 != 0
    Uint32 legacyTailWord = 0;           // version <= 3

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4NetworkTunnelOccupant (0x8a4bd52b, Read @ 0x1001a2888, Write @ 0x1001a279a)
struct NetworkTunnelOccupantSaveRecord {
    Uint16 version = 2;
    Uint32 memAddr = 0;
    NetworkPreBuiltOccupantSaveRecord preBuilt;
    Uint8 hasPeer = 0;
    SimSerializableRef peerTunnelRef{}; // IID 0x8a4bd52b

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4UndergroundOccupant (Read @ 0x1001ab0f2, Write @ 0x1001aae32)
struct UndergroundOccupantSaveRecord {
    Uint16 version = 3;
    NetworkOccupantSaveRecord network;
    Uint8 field88 = 0;
    Uint8 field89 = 0;
    Float32 field98 = 0.0f;
    Uint32 field1D8 = 0;
    std::array<std::vector<S3DVertexV3FC4UBT2F>, 6> meshVerts{}; // +0x1e0..+0x260
    S3DMatrix4 matrix2D8{}; // +0x2d8
    Sint32 cellX318 = 0;    // +0x318
    Sint32 cellZ31C = 0;    // +0x31c
    bool field328 = false;
    bool field329 = false;
    bool field32A = false;
    std::array<Float32, 4> floats32C{};
    Float32 field33C = 0.0f;
    Uint32 field344 = 0;
    Uint32 field348 = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4PipeOccupant (0x49c05b9f, Read @ 0x1001a1290, Write @ 0x1001a1218)
struct PipeOccupantSaveRecord {
    Uint16 version = 3;
    Uint32 memAddr = 0;
    UndergroundOccupantSaveRecord underground;
    Uint32 wateredState358 = 0; // +0x358

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4SubwayOccupant (0xca16374f, Read @ 0x1001a19d8, Write @ 0x1001a18e4)
struct SubwayOccupantSaveRecord {
    Uint16 version = 2;
    Uint32 memAddr = 0;
    UndergroundOccupantSaveRecord underground;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4UndergroundConnectionOccupant (0xca15ed6b / 0xea0a5acf, Read @ 0x1002f0178, Write @ 0x1002efec8)
struct UndergroundConnectionOccupantSaveRecord {
    Uint16 version = 2;
    Uint32 memAddr = 0;
    OccupantBaseSaveRecord base;
    Uint32 fieldA8 = 0;
    std::array<std::vector<S3DVertexV3FC4UBT2F>, 7> meshVerts{}; // +0xb0..+0x150
    Sint32 field1F8 = 0;
    Sint32 field1FC = 0;
    std::array<Float32, 4> floats210{};
    Float32 field220 = 0.0f;
    Sint32 field228 = 0;
    bool field22C = false;
    SimSerializableRef ref168{}; // IID 0x26bb767e
    SimSerializableRef ref170{}; // IID 0x26bb767e
    Uint32 field2F8 = 0;
    std::array<Float32, 3> floats2E0{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// Helper to decode a multi-record DBPF subfile where each record is prefixed
// by [Uint32 recordSize][Uint32 crc32][Uint32 memAddr].
template <typename RecordT>
bool DecodeMultiRecordSubfile(std::span<const Uint8> subfileBytes,
                              std::vector<RecordT>& outRecords) noexcept {
    outRecords.clear();
    std::size_t pos = 0;
    while (pos < subfileBytes.size()) {
        if (pos + 12 > subfileBytes.size()) {
            return false;
        }
        Uint32 recSize = 0;
        Uint32 crc = 0;
        Uint32 memAddr = 0;
        std::memcpy(&recSize, subfileBytes.data() + pos, 4);
        std::memcpy(&crc, subfileBytes.data() + pos + 4, 4);
        std::memcpy(&memAddr, subfileBytes.data() + pos + 8, 4);
        (void)crc;
        if (recSize < 12 || pos + recSize > subfileBytes.size()) {
            return false;
        }
        cIGZIStream stream(subfileBytes.data() + pos + 12, recSize - 12);
        RecordT rec{};
        rec.memAddr = memAddr;
        if (!rec.Read(stream) || !stream.Good() || stream.Remaining() != 0) {
            return false;
        }
        outRecords.push_back(std::move(rec));
        pos += recSize;
    }
    return true;
}

template <typename RecordT>
bool EncodeMultiRecordSubfile(std::span<const RecordT> records,
                              std::vector<Uint8>& outBytes) {
    cIGZOStream out;
    for (const auto& rec : records) {
        cIGZOStream body;
        if (!rec.Write(body)) {
            return false;
        }
        const Uint32 recSize = static_cast<Uint32>(body.Size() + 12u);
        const Uint32 crc = ComputeSC4RecordCRC(rec.memAddr, body.Buffer().data(), body.Size());
        out << recSize << crc << rec.memAddr;
        out.WriteBytes(body.Buffer().data(), body.Size());
    }
    outBytes = std::move(out.Buffer());
    return true;
}

namespace sc4_detail {

// Sentinel: compare every record regardless of its leading stream version.
constexpr Uint32 kCompareAllVersions = 0xffffffffu;

// Optional diagnostics hook invoked on every byte mismatch (original record
// bytes vs re-encoded record bytes, both including the 12-byte header).
inline void (*gRoundTripMismatchHook)(Uint16 version, std::span<const Uint8> original,
                                      std::span<const Uint8> reencoded) = nullptr;

// Decode every [size][crc][memAddr]+body record of a subfile with R, then
// re-encode (via WriteWithVersion(stored version) when R provides it, else
// Write) and compare byte-for-byte including the recomputed CRC.
template <typename R>
void RoundTripMulti(std::span<const Uint8> bytes, Uint32 currentVersion,
                    SC4SubfileRoundTripStats& st) {
    std::size_t pos = 0;
    while (pos + 12u <= bytes.size()) {
        Uint32 recSize = 0;
        Uint32 memAddr = 0;
        std::memcpy(&recSize, bytes.data() + pos, 4);
        std::memcpy(&memAddr, bytes.data() + pos + 8, 4);
        ++st.records;
        if (recSize < 12u || pos + recSize > bytes.size()) {
            return;
        }
        const Uint8* body = bytes.data() + pos + 12;
        const std::size_t n = recSize - 12u;
        R rec{};
        if constexpr (requires(R& r) { r.memAddr = memAddr; }) {
            rec.memAddr = memAddr;
        }
        cIGZIStream s(body, n);
        if (rec.Read(s) && s.Good() && s.Remaining() == 0) {
            ++st.decoded;
            Uint16 ver = 0;
            if (n >= 2u) {
                std::memcpy(&ver, body, 2);
            }
            constexpr bool kHasVersionedWrite =
                requires(const R& r, cIGZOStream& o) { r.WriteWithVersion(o, Uint16{}); };
            if (kHasVersionedWrite || currentVersion == kCompareAllVersions ||
                ver == currentVersion) {
                ++st.currentVersion;
                cIGZOStream out;
                bool wrote = false;
                if constexpr (kHasVersionedWrite) {
                    wrote = rec.WriteWithVersion(out, ver);
                } else {
                    wrote = rec.Write(out);
                }
                if (wrote) {
                    const Uint32 crc =
                        ComputeSC4RecordCRC(memAddr, out.Buffer().data(), out.Size());
                    cIGZOStream full;
                    full << static_cast<Uint32>(out.Size() + 12u) << crc << memAddr;
                    full.WriteBytes(out.Buffer().data(), out.Size());
                    if (full.Size() == recSize &&
                        std::memcmp(full.Buffer().data(), bytes.data() + pos, recSize) == 0) {
                        ++st.byteExact;
                    } else if (gRoundTripMismatchHook != nullptr) {
                        gRoundTripMismatchHook(ver, bytes.subspan(pos, recSize),
                                               std::span<const Uint8>(full.Buffer()));
                    }
                }
            }
        }
        pos += recSize;
    }
}

// Raw (headerless) subfile: one Read must consume the subfile exactly, and
// Write must reproduce it byte-for-byte.
template <typename R>
void RoundTripRaw(std::span<const Uint8> bytes, SC4SubfileRoundTripStats& st) {
    ++st.records;
    R rec{};
    cIGZIStream s(bytes.data(), bytes.size());
    if (!rec.Read(s) || !s.Good() || s.Remaining() != 0) {
        return;
    }
    ++st.decoded;
    ++st.currentVersion;
    cIGZOStream out;
    if (rec.Write(out) && out.Size() == bytes.size() &&
        std::memcmp(out.Buffer().data(), bytes.data(), bytes.size()) == 0) {
        ++st.byteExact;
    }
}

} // namespace sc4_detail

// Full registry: Batch 1 decoders plus every previously decompiled simulator /
// developer / manager / SimGrid record (src/sc4_save_registry.cpp).
bool RoundTripAnySC4Subfile(Uint32 typeId, std::span<const Uint8> subfileBytes,
                            SC4SubfileRoundTripStats& stats);

// ---------------------------------------------------------------------------
// Wave 2B method index
// Terrain, surface water, lots, regional cities, occupants, networks, and
// zones. Every leaf (// @ 0x100......) and algorithmic (###) entry point from
// the eighteen Wave 2B workpacks is recorded in kWave2BMethods. The helpers
// below are the pure field, mask, and coordinate operations those leaves
// implement.
// ---------------------------------------------------------------------------

// cSC4Lot habitation byte at +0xd3 (GetState @ 0x100183a68).
//   IsInLivableState        @ 0x100183a84: state == 3
//   HasCapacity            @ 0x100183a94: (state & ~1) == 2
//   IsInDemolishableState  @ 0x100183aa8: unsigned(state - 2) < 3 || state == 6
// Flag byte at +0xcd:
//   SetHasPower            @ 0x100183b62: bit 0x10
//   SetHistorical          @ 0x100183ae6: bit 0x20
//   SetVerticesAreLocked   @ 0x100183b92: bit 0x40
constexpr Uint8 kLotStateLivable = 3;
constexpr Uint8 kLotStateDeveloped = 2;
constexpr Uint8 kLotStateDemolishExtra = 6;
constexpr Uint8 kLotFlagHasPower = 0x10u;
constexpr Uint8 kLotFlagHistorical = 0x20u;
constexpr Uint8 kLotFlagVerticesLocked = 0x40u;

constexpr bool LotIsInLivableState(Uint8 state) noexcept {
    return state == kLotStateLivable;
}

constexpr bool LotHasCapacity(Uint8 state) noexcept {
    return static_cast<Uint8>(state & static_cast<Uint8>(~Uint8{1})) == kLotStateDeveloped;
}

constexpr bool LotIsInDemolishableState(Uint8 state) noexcept {
    const Uint8 lowered = static_cast<Uint8>(state - kLotStateDeveloped);
    return lowered < 3u || state == kLotStateDemolishExtra;
}

// GetCapacity(developer, ignoreState) @ 0x100181462 requires state == 2 when
// the ignore-state flag is clear.
constexpr bool LotDeveloperCapacityReadable(Uint8 state, bool ignoreState) noexcept {
    return ignoreState || state == kLotStateDeveloped;
}

constexpr Uint8 SetLotFlagBit(Uint8 flags, Uint8 bit, bool enabled) noexcept {
    if (enabled) {
        return static_cast<Uint8>(flags | bit);
    }
    return static_cast<Uint8>(flags & static_cast<Uint8>(~bit));
}

constexpr bool LotFlagBit(Uint8 flags, Uint8 bit) noexcept {
    return (flags & bit) != 0;
}

// SetFacing @ 0x1001839e8 stores the low 8 bits. GetFacing @ 0x1001839da
// returns that byte. SetBuildingType @ 0x100183b12 clears wealth at +0xd6.
constexpr Uint8 StoreLotFacing(int facing) noexcept {
    return static_cast<Uint8>(facing);
}

constexpr Uint8 LotWealthAfterBuildingTypeChange() noexcept {
    return 0;
}

// GetBoundingRect @ 0x10017fa80 zero-extends the four cell bytes into SC4Rect.
constexpr SC4Rect<int> LotBoundingRect(Uint8 x0, Uint8 z0, Uint8 x1, Uint8 z1) noexcept {
    return SC4Rect<int>{
        static_cast<int>(x0),
        static_cast<int>(z0),
        static_cast<int>(x1),
        static_cast<int>(z1),
    };
}

// GetSourceCells @ 0x100183bca returns ((z0 << 32) | x0, (z1 << 32) | x1).
struct LotSourceCells {
    Uint64 origin = 0;
    Uint64 extent = 0;
};

constexpr LotSourceCells PackLotSourceCells(Uint8 x0, Uint8 z0, Uint8 x1, Uint8 z1) noexcept {
    return LotSourceCells{
        (static_cast<Uint64>(z0) << 32) | x0,
        (static_cast<Uint64>(z1) << 32) | x1,
    };
}

struct LotView {
    Uint8 state = 0;
    Uint8 wealth = 0;
    Uint8 facing = 0;
    Uint8 flags = 0;
    Uint8 cellX0 = 0;
    Uint8 cellZ0 = 0;
    Uint8 cellX1 = 0;
    Uint8 cellZ1 = 0;

    constexpr bool IsLivable() const noexcept { return LotIsInLivableState(state); }
    constexpr bool HasCapacity() const noexcept { return LotHasCapacity(state); }
    constexpr bool IsDemolishable() const noexcept { return LotIsInDemolishableState(state); }
    constexpr bool IsHistorical() const noexcept { return LotFlagBit(flags, kLotFlagHistorical); }
    constexpr bool HasPower() const noexcept { return LotFlagBit(flags, kLotFlagHasPower); }
    constexpr bool VerticesAreLocked() const noexcept {
        return LotFlagBit(flags, kLotFlagVerticesLocked);
    }
    constexpr Uint8 Facing() const noexcept { return facing; }
    constexpr Uint8 Wealth() const noexcept { return wealth; }
    constexpr SC4Rect<int> Bounds() const noexcept {
        return LotBoundingRect(cellX0, cellZ0, cellX1, cellZ1);
    }
    constexpr LotSourceCells SourceCells() const noexcept {
        return PackLotSourceCells(cellX0, cellZ0, cellX1, cellZ1);
    }
    constexpr LotView WithFacing(int newFacing) const noexcept {
        LotView next = *this;
        next.facing = StoreLotFacing(newFacing);
        return next;
    }
    constexpr LotView WithHistorical(bool on) const noexcept {
        LotView next = *this;
        next.flags = SetLotFlagBit(flags, kLotFlagHistorical, on);
        return next;
    }
    constexpr LotView WithPower(bool on) const noexcept {
        LotView next = *this;
        next.flags = SetLotFlagBit(flags, kLotFlagHasPower, on);
        return next;
    }
    constexpr LotView WithVerticesLocked(bool on) const noexcept {
        LotView next = *this;
        next.flags = SetLotFlagBit(flags, kLotFlagVerticesLocked, on);
        return next;
    }
};

// cSC4RegionalCity scalars.
//   GetPopulation          @ 0x1003b2c4c  +0x30
//   GetCommercialJobs      @ 0x1003b2c56  +0x34
//   GetIndustrialJobs      @ 0x1003b2c60  +0x38
//   GetBudget / SetBudget  @ 0x1003b21dc / 0x1003b21ea  +0x268
//   GetIncome / SetIncome  @ 0x1003b21f8 / 0x1003b2206  +0x270
//   GetWorldHemisphere     @ 0x1003b2e34: 2 - (worldZ > 0)
//   IsTutorial             @ 0x1003b2c0a: tutorial GUID != -1
// Commodity stats (cCommodityStats via map::operator[]):
//   produced +0x00, demanded +0x08, exported +0x10, imported +0x18, cost +0x20
constexpr int WorldHemisphereFromLatitude(float worldZ) noexcept {
    return worldZ > 0.0f ? 1 : 2;
}

constexpr bool RegionalCityIsTutorial(int tutorialGuid) noexcept {
    return tutorialGuid != -1;
}

struct WealthPopulation {
    Uint32 wealth = 0;
    Uint32 population = 0;
};

// GetPopulation(unsigned int) @ 0x1003b1cf2 walks a map of wealth -> count
// and yields 0 when the wealth key is absent.
constexpr Uint32 PopulationForWealth(std::span<const WealthPopulation> table,
                                     Uint32 wealth) noexcept {
    for (const WealthPopulation& entry : table) {
        if (entry.wealth == wealth) {
            return entry.population;
        }
    }
    return 0u;
}

struct RegionalCommodityStats {
    Float64 produced = 0.0;
    Float64 demanded = 0.0;
    Float64 exported = 0.0;
    Float64 imported = 0.0;
    Float64 costPerUnit = 0.0;

    constexpr Float64 Produced() const noexcept { return produced; }
    constexpr Float64 Demanded() const noexcept { return demanded; }
    constexpr Float64 Exported() const noexcept { return exported; }
    constexpr Float64 Imported() const noexcept { return imported; }
    constexpr Float64 CostPerUnit() const noexcept { return costPerUnit; }
};

// FindConnection(int, int, int) @ 0x1003b1d78 matches x, z, and connection type.
struct NeighborConnectionRef {
    Sint32 x = 0;
    Sint32 z = 0;
    Sint32 connectionType = 0;

    constexpr bool Matches(Sint32 cx, Sint32 cz, Sint32 type) const noexcept {
        return x == cx && z == cz && connectionType == type;
    }
};

constexpr const NeighborConnectionRef* FindRegionalConnection(
    std::span<const NeighborConnectionRef> connections,
    Sint32 x, Sint32 z, Sint32 type) noexcept {
    for (const NeighborConnectionRef& connection : connections) {
        if (connection.Matches(x, z, type)) {
            return &connection;
        }
    }
    return nullptr;
}

struct RegionalCityMetrics {
    Sint32 posX = 0;
    Sint32 posZ = 0;
    Sint32 sizeX = 0;
    Sint32 sizeZ = 0;
    Uint32 population = 0;
    Uint32 commercialJobs = 0;
    Uint32 industrialJobs = 0;
    Sint8 mayorRating = 0;
    Uint32 difficulty = 0;
    Float64 budget = 0.0;
    Float64 income = 0.0;
    bool established = false;
    Float32 worldX = 0.0f;
    Float32 worldZ = 0.0f;
    Float32 worldBaseElevation = 0.0f;
    Sint32 tutorialGuid = -1;

    constexpr Uint32 Population() const noexcept { return population; }
    constexpr Uint32 CommercialJobs() const noexcept { return commercialJobs; }
    constexpr Uint32 IndustrialJobs() const noexcept { return industrialJobs; }
    constexpr Float64 Budget() const noexcept { return budget; }
    constexpr Float64 Income() const noexcept { return income; }
    constexpr int Hemisphere() const noexcept { return WorldHemisphereFromLatitude(worldZ); }
    constexpr bool IsTutorial() const noexcept { return RegionalCityIsTutorial(tutorialGuid); }
    constexpr bool Established() const noexcept { return established; }
};

// cSC4ZoneManager.
//   GetTextureForZone @ 0x100199eb4: zone type > 15 fails; otherwise the key's
//   first qword is 0x1abe787d7ab50e44 (FSH type 0x7ab50e44, group 0x1abe787d)
//   and the instance is the per-type dword at +0x308.
//   IsRCIZone @ 0x100199e98: cmp type, 0x10; ja -> not an RCI candidate.
//   GetZoneDragColor @ 0x10019a000 indexes the dword table at +0x348.
//   Constructor stores display alpha 0xFF at +0x388.
constexpr Uint32 kZoneTextureTypeId = 0x7ab50e44u;
constexpr Uint32 kZoneTextureGroupId = 0x1abe787du;
constexpr unsigned kZoneTextureIndexMax = 0x0fu;
constexpr unsigned kZoneRciCompareLimit = 0x10u;
constexpr Uint8 kZoneDisplayAlphaOpaque = 0xffu;

constexpr bool ZoneTypeHasTexture(int zoneType) noexcept {
    return static_cast<unsigned>(zoneType) <= kZoneTextureIndexMax;
}

constexpr bool ZoneTypeWithinRciCompare(int zoneType) noexcept {
    return static_cast<unsigned>(zoneType) <= kZoneRciCompareLimit;
}

struct ZoneTextureKey {
    Uint32 typeId = 0;
    Uint32 groupId = 0;
    Uint32 instanceId = 0;
};

constexpr bool MakeZoneTextureKey(int zoneType, Uint32 instanceId, ZoneTextureKey& out) noexcept {
    if (!ZoneTypeHasTexture(zoneType)) {
        return false;
    }
    out.typeId = kZoneTextureTypeId;
    out.groupId = kZoneTextureGroupId;
    out.instanceId = instanceId;
    return true;
}

constexpr Uint32 ZoneTableValue(std::span<const Uint32> table, int zoneType) noexcept {
    const auto index = static_cast<std::size_t>(static_cast<unsigned>(zoneType));
    if (index >= table.size()) {
        return 0u;
    }
    return table[index];
}

struct ZoneStyle {
    Uint8 displayAlpha = kZoneDisplayAlphaOpaque;
    Uint8 defaultDisplayAlpha = kZoneDisplayAlphaOpaque;

    constexpr Uint8 DisplayAlpha() const noexcept { return displayAlpha; }
    constexpr Uint8 DefaultDisplayAlpha() const noexcept { return defaultDisplayAlpha; }
    constexpr Uint32 DragColor(std::span<const Uint32> dragColors, int zoneType) const noexcept {
        return ZoneTableValue(dragColors, zoneType);
    }
    constexpr bool TextureKey(std::span<const Uint32> instances, int zoneType,
                              ZoneTextureKey& out) const noexcept {
        if (!ZoneTypeHasTexture(zoneType)) {
            return false;
        }
        return MakeZoneTextureKey(zoneType, ZoneTableValue(instances, zoneType), out);
    }
};

// cSC4NetworkOccupant edge / rotation / flag leaves.
//   HasNetworkFlag     @ 0x1001a296c: (flags & mask) == mask
//   HasAnyNetworkFlag  @ 0x1001a2980: (flags & mask) != 0
//   GetRotation        @ 0x1001a29f4: packed & 0x7f
//   GetFlip            @ 0x1001a2a04: high bit of the same packed byte
//   GetActiveEdges     @ 0x10019e9a8: imul edgeBytes, 0x01020408
//   HasProperty        @ 0x10019f14e: property 0x88696402 is answered directly
//   GetVertexHeight    @ 0x10019ef0c: record stride 24 bytes, 4 corners
constexpr Uint32 kNetworkDirectPropertyId = 0x88696402u;
constexpr Uint8 kNetworkRotationMask = 0x7fu;
constexpr Uint8 kNetworkFlipBit = 0x80u;
constexpr Uint32 kEdgeByteSpread = 0x01020408u;
constexpr int kNetworkVertexStrideBytes = 24;
constexpr int kNetworkCornerCount = 4;

constexpr bool HasNetworkFlag(Uint32 flags, Uint32 required) noexcept {
    return (flags & required) == required;
}

constexpr bool HasAnyNetworkFlag(Uint32 flags, Uint32 mask) noexcept {
    return (flags & mask) != 0u;
}

constexpr Uint8 NetworkRotation(Uint8 rotationAndFlip) noexcept {
    return static_cast<Uint8>(rotationAndFlip & kNetworkRotationMask);
}

constexpr bool NetworkFlip(Uint8 rotationAndFlip) noexcept {
    return (rotationAndFlip & kNetworkFlipBit) != 0;
}

constexpr Uint8 PackNetworkRotationAndFlip(Uint8 rotation, bool flip) noexcept {
    const Uint8 packed = static_cast<Uint8>(rotation & kNetworkRotationMask);
    if (!flip) {
        return packed;
    }
    return static_cast<Uint8>(packed | kNetworkFlipBit);
}

constexpr bool NetworkHasDirectProperty(Uint32 propertyId) noexcept {
    return propertyId == kNetworkDirectPropertyId;
}

constexpr Uint32 SpreadEdgeTypeBytes(Uint32 edgeTypeBytes) noexcept {
    return edgeTypeBytes * kEdgeByteSpread;
}

constexpr std::size_t NetworkVertexByteOffset(int vertexIndex) noexcept {
    return static_cast<std::size_t>(vertexIndex) * static_cast<std::size_t>(kNetworkVertexStrideBytes);
}

// Edge-config records are 8 bytes: type at +0, flags at +4
// (GetEdgeTypesFlag @ 0x10019eb50, GetActiveEdgesForType @ 0x10019e9ee).
struct EdgeTypeConfig {
    Uint8 networkType = 0;
    Uint32 flags = 0;
};

constexpr Uint32 EdgeTypesFlag(Uint8 primaryType, Uint32 primaryFlags,
                               std::span<const EdgeTypeConfig> extra,
                               Uint8 queryType) noexcept {
    if (primaryType == queryType) {
        return primaryFlags;
    }
    for (const EdgeTypeConfig& config : extra) {
        if (config.networkType == queryType) {
            return config.flags;
        }
    }
    return 0u;
}

struct NetworkEdgeView {
    Uint32 networkFlags = 0;
    Uint8 rotationAndFlip = 0;
    Uint8 primaryType = 0;
    Uint32 primaryEdgeFlags = 0;

    constexpr bool HasFlag(Uint32 mask) const noexcept {
        return HasNetworkFlag(networkFlags, mask);
    }
    constexpr bool HasAnyFlag(Uint32 mask) const noexcept {
        return HasAnyNetworkFlag(networkFlags, mask);
    }
    constexpr Uint8 Rotation() const noexcept { return NetworkRotation(rotationAndFlip); }
    constexpr bool Flipped() const noexcept { return NetworkFlip(rotationAndFlip); }
    constexpr Uint32 SpreadEdges() const noexcept {
        return SpreadEdgeTypeBytes(primaryEdgeFlags);
    }
    constexpr Uint32 FlagsForType(std::span<const EdgeTypeConfig> extra,
                                  Uint8 queryType) const noexcept {
        return EdgeTypesFlag(primaryType, primaryEdgeFlags, extra, queryType);
    }
};

// cSTETerrain grid leaves.
//   GetVertIndex        @ 0x100740e50: z * stride + x
//   GetVertex           @ 0x100740e5e: unsigned div/mod by the vertex stride
//   LocationIsInBounds  @ 0x100740e7a: 0 <= x <= extentX and 0 <= z <= extentZ
constexpr int TerrainVertexIndex(int x, int z, int stride) noexcept {
    return static_cast<int>(static_cast<Uint32>(z) * static_cast<Uint32>(stride) +
                            static_cast<Uint32>(x));
}

constexpr int TerrainVertexX(int index, int stride) noexcept {
    return static_cast<int>(static_cast<Uint32>(index) % static_cast<Uint32>(stride));
}

constexpr int TerrainVertexZ(int index, int stride) noexcept {
    return static_cast<int>(static_cast<Uint32>(index) / static_cast<Uint32>(stride));
}

constexpr bool TerrainLocationInBounds(float x, float z, float extentX, float extentZ) noexcept {
    return x >= 0.0f && z >= 0.0f && extentX >= x && extentZ >= z;
}

// cSC4OccupantManager coordinate leaves. Cell size immediate 0x42800000 is 64.0f.
//   GetWorldCellCount                    @ 0x100247636: 192 x 192
//   WorldCellToOccupantManagerCell       @ 0x100247656: cell - 64
//   OccupantManagerCellToWorldCell       @ 0x100247668: cell + 64
//   WorldCellToPosition                  @ 0x1002476ba: cell * 64 - 4096
//   OccupantManagerCellToPosition        @ 0x100247744: cell * 64
//   StandardCityCellToPosition           @ 0x1002477b0: cell * 16
//   WorldCellToStandardCityCell          @ 0x10024767a: 4 * worldCell - 256
//   GetOccupantManagerCellBounds         @ 0x100247852: [64, 64, count+63, count+63]
constexpr Float32 kOccupantManagerCellSize = 64.0f;
constexpr Uint32 kOccupantManagerCellSizeBits = 0x42800000u;
constexpr int kOccupantWorldCellCount = 192;
constexpr int kOccupantWorldCellBias = 0x40;
constexpr int kOccupantWorldOriginMeters = 4096;
constexpr int kStandardCityCellMeters = 16;

constexpr int WorldCellToOccupantManagerCell(int worldCell) noexcept {
    return worldCell - kOccupantWorldCellBias;
}

constexpr int OccupantManagerCellToWorldCell(int occupantCell) noexcept {
    return occupantCell + kOccupantWorldCellBias;
}

constexpr int WorldCellToStandardCityCell(int worldCell) noexcept {
    return worldCell * 4 - 256;
}

constexpr Float32 WorldCellToMeters(int worldCell) noexcept {
    const Uint32 shifted = static_cast<Uint32>(worldCell) << 6;
    const Uint32 biasedBits = shifted - static_cast<Uint32>(kOccupantWorldOriginMeters);
    return static_cast<Float32>(static_cast<int>(biasedBits));
}

constexpr Float32 OccupantManagerCellToMeters(int occupantCell) noexcept {
    const Uint32 shifted = static_cast<Uint32>(occupantCell) << 6;
    return static_cast<Float32>(static_cast<int>(shifted));
}

constexpr Float32 StandardCityCellToMeters(int cityCell) noexcept {
    const Uint32 shifted = static_cast<Uint32>(cityCell) << 4;
    return static_cast<Float32>(static_cast<int>(shifted));
}

struct OccupantCellBounds {
    int minX = kOccupantWorldCellBias;
    int minZ = kOccupantWorldCellBias;
    int maxX = 0;
    int maxZ = 0;
};

constexpr OccupantCellBounds OccupantManagerCellBounds(int countX, int countZ) noexcept {
    return OccupantCellBounds{
        kOccupantWorldCellBias,
        kOccupantWorldCellBias,
        countX + (kOccupantWorldCellBias - 1),
        countZ + (kOccupantWorldCellBias - 1),
    };
}

struct OccupantGrid {
    int cellCountX = 0;
    int cellCountZ = 0;

    constexpr Float32 CellSize() const noexcept { return kOccupantManagerCellSize; }
    constexpr int WorldCellCount() const noexcept { return kOccupantWorldCellCount; }
    constexpr int ToOccupantCell(int worldCell) const noexcept {
        return WorldCellToOccupantManagerCell(worldCell);
    }
    constexpr int ToWorldCell(int occupantCell) const noexcept {
        return OccupantManagerCellToWorldCell(occupantCell);
    }
    constexpr int ToStandardCityCell(int worldCell) const noexcept {
        return WorldCellToStandardCityCell(worldCell);
    }
    constexpr Float32 WorldMeters(int worldCell) const noexcept {
        return WorldCellToMeters(worldCell);
    }
    constexpr OccupantCellBounds Bounds() const noexcept {
        return OccupantManagerCellBounds(cellCountX, cellCountZ);
    }
};

static_assert(LotIsInLivableState(3));
static_assert(!LotIsInLivableState(2));
static_assert(LotHasCapacity(2));
static_assert(LotHasCapacity(3));
static_assert(!LotHasCapacity(1));
static_assert(!LotHasCapacity(4));
static_assert(LotIsInDemolishableState(2));
static_assert(LotIsInDemolishableState(3));
static_assert(LotIsInDemolishableState(4));
static_assert(!LotIsInDemolishableState(5));
static_assert(LotIsInDemolishableState(6));
static_assert(!LotIsInDemolishableState(0));
static_assert(LotDeveloperCapacityReadable(2, false));
static_assert(!LotDeveloperCapacityReadable(3, false));
static_assert(LotDeveloperCapacityReadable(3, true));
static_assert(SetLotFlagBit(0, kLotFlagHistorical, true) == 0x20u);
static_assert(SetLotFlagBit(0xffu, kLotFlagHistorical, false) == 0xdfu);
static_assert(SetLotFlagBit(0xffu, kLotFlagHasPower, false) == 0xefu);
static_assert(SetLotFlagBit(0xffu, kLotFlagVerticesLocked, false) == 0xbfu);
static_assert(StoreLotFacing(0x123) == 0x23u);
static_assert(LotWealthAfterBuildingTypeChange() == 0);
static_assert((LotBoundingRect(1, 2, 3, 4) == SC4Rect<int>{1, 2, 3, 4}));
static_assert(PackLotSourceCells(1, 2, 3, 4).origin == ((Uint64{2} << 32) | 1u));
static_assert(PackLotSourceCells(1, 2, 3, 4).extent == ((Uint64{4} << 32) | 3u));
static_assert(WorldHemisphereFromLatitude(1.0f) == 1);
static_assert(WorldHemisphereFromLatitude(0.0f) == 2);
static_assert(WorldHemisphereFromLatitude(-1.0f) == 2);
static_assert(RegionalCityIsTutorial(0));
static_assert(!RegionalCityIsTutorial(-1));
static_assert(ZoneTypeHasTexture(0));
static_assert(ZoneTypeHasTexture(15));
static_assert(!ZoneTypeHasTexture(16));
static_assert(ZoneTypeWithinRciCompare(0x10));
static_assert(!ZoneTypeWithinRciCompare(0x11));
static_assert(HasNetworkFlag(0x5u, 0x1u));
static_assert(!HasNetworkFlag(0x5u, 0x2u));
static_assert(HasAnyNetworkFlag(0x5u, 0x4u));
static_assert(!HasAnyNetworkFlag(0x5u, 0x2u));
static_assert(NetworkRotation(0x83u) == 0x03u);
static_assert(NetworkFlip(0x83u));
static_assert(!NetworkFlip(0x03u));
static_assert(PackNetworkRotationAndFlip(0x83u, true) == 0x83u);
static_assert(NetworkHasDirectProperty(kNetworkDirectPropertyId));
static_assert(SpreadEdgeTypeBytes(1u) == kEdgeByteSpread);
static_assert(NetworkVertexByteOffset(2) == 48u);
static_assert(kNetworkCornerCount == 4);
static_assert(kOccupantWorldOriginMeters == 4096);
static_assert(TerrainVertexIndex(3, 2, 10) == 23);
static_assert(TerrainVertexX(23, 10) == 3);
static_assert(TerrainVertexZ(23, 10) == 2);
static_assert(TerrainLocationInBounds(0.0f, 0.0f, 1.0f, 1.0f));
static_assert(!TerrainLocationInBounds(-0.1f, 0.0f, 1.0f, 1.0f));
static_assert(kOccupantManagerCellSize == 64.0f);
static_assert(kOccupantManagerCellSizeBits == 0x42800000u);
static_assert(WorldCellToOccupantManagerCell(64) == 0);
static_assert(OccupantManagerCellToWorldCell(0) == 64);
static_assert(WorldCellToStandardCityCell(64) == 0);
static_assert(WorldCellToMeters(64) == 0.0f);
static_assert(OccupantManagerCellToMeters(2) == 128.0f);
static_assert(StandardCityCellToMeters(2) == 32.0f);
static_assert(OccupantManagerCellBounds(4, 5).maxX == 67);
static_assert(OccupantManagerCellBounds(4, 5).maxZ == 68);

struct Wave2BMethod {
    Uint64 address;
    Uint32 sizeBytes;
    const char* name;
};

inline constexpr std::array<Wave2BMethod, 1097> kWave2BMethods = {{
    // cSTETerrain — 210 entry points
    {0x10073083eu, 68u, "cSTETerrain::BrushFnRain(int, cSTETerrain::tBrushContext const&)"},
    {0x100731338u, 182u, "cSTETerrain::AddLandTypeAtVertex(unsigned int, unsigned short, float)"},
    {0x1007313eeu, 14u, "cSTETerrain::AddRef()"},
    {0x100731418u, 390u, "cSTETerrain::AddTemperatureAndMoistureToSurfaceCoverEntry(cRZString const&, cRZString const&, void*)"},
    {0x10073159eu, 138u, "cSTETerrain::AddToTerrainTypeTextureList(int, unsigned int)"},
    {0x100731628u, 274u, "cSTETerrain::AddVolcanicAshRegion(SC4Rect<int>&)"},
    {0x10073173au, 196u, "cSTETerrain::AdjustLandTypeRatios(SC4Rect<int> const&)"},
    {0x1007317feu, 542u, "cSTETerrain::AnalyzeShoreAndCreateCliffs(std::__1::deque<int, std::__1::allocator<int>>&)"},
    {0x100731a1cu, 148u, "cSTETerrain::GetNeighboringSeaVertex(int) const"},
    {0x100731ab0u, 416u, "cSTETerrain::CreateShoreCliff(std::__1::vector<cSTETerrain::tCliffVertInfo, std::__1::allocator<cSTETerrain::tCliffVertInfo>>&)"},
    {0x100731c50u, 192u, "cSTETerrain::ApplyBrush(unsigned int, cISTETerrain::tBrushParameters&)"},
    {0x100731d10u, 1192u, "cSTETerrain::ApplyBrushComponents(cSTETerrain::tBrushDescription&, cISTETerrain::tBrushParameters&)"},
    {0x1007321b8u, 932u, "cSTETerrain::ApplyBrushAtLocation(cS3DTextureBufferInfo const*, int, int, float, float, bool (cSTETerrain::*)(int, cSTETerrain::tBrushContext const&), cSTETerrain::tBrushContext&, SC4Rect<int>&)"},
    {0x10073255cu, 250u, "cSTETerrain::ComputeRectAroundPt(SC4Point<float>*, float, float, float, float, float)"},
    {0x100732656u, 392u, "cSTETerrain::ComputeSpansForRectangle(SC4Point<float>*, int*, int*, int&, int&) const"},
    {0x1007327deu, 494u, "cSTETerrain::ApplyBrushAtLocation(int, int, float, float, bool (cSTETerrain::*)(int, cSTETerrain::tBrushContext const&), unsigned int, cSTETerrain::tBrushContext&, SC4Rect<int>&)"},
    {0x1007329ccu, 66u, "cSTETerrain::BrushFnRaiseLower(int, cSTETerrain::tBrushContext const&)"},
    {0x100732a0eu, 452u, "cSTETerrain::BrushFnFlora(int, cSTETerrain::tBrushContext const&)"},
    {0x100732bd2u, 336u, "cSTETerrain::BrushFnSmoothen(int, cSTETerrain::tBrushContext const&)"},
    {0x100732d22u, 624u, "cSTETerrain::BrushFnPaint(int, cSTETerrain::tBrushContext const&)"},
    {0x100732f92u, 670u, "cSTETerrain::BrushFnPlaceEffect(int, cSTETerrain::tBrushContext const&)"},
    {0x100733230u, 126u, "cSTETerrain::BrushFnLevel(int, cSTETerrain::tBrushContext const&)"},
    {0x1007332aeu, 138u, "cSTETerrain::ApplyWaterErosionBrush(cSTETerrain::tBrushContext&, int, SC4Rect<int>&)"},
    {0x100733338u, 40u, "cSTETerrain::BrushFnVolcanoAsh(int, cSTETerrain::tBrushContext const&)"},
    {0x100733360u, 174u, "cSTETerrain::BrushFnLakeBed(int, cSTETerrain::tBrushContext const&)"},
    {0x10073340eu, 40u, "cSTETerrain::DoPostBrushApplicationOperation(int, cSTETerrain::tBrushContext&, SC4Rect<int>&)"},
    {0x100733436u, 352u, "cSTETerrain::GetTextureValue(int, int, cSTETerrain::tBrushContext&)"},
    {0x100733596u, 80u, "cSTETerrain::PadRectangle(SC4Rect<int>&, int)"},
    {0x1007335e6u, 1328u, "cSTETerrain::DoWaterErosion(SC4Rect<int> const&, int)"},
    {0x100733b16u, 142u, "cSTETerrain::AreSeaVertices(SC4Rect<int> const&) const"},
    {0x100733ba4u, 36u, "cSTETerrain::IsSeaVertex(int) const"},
    {0x100733bc8u, 154u, "cSTETerrain::AssignSurfaceTexturesAndColors(SC4Rect<int>&)"},
    {0x100733c62u, 310u, "cSTETerrain::ComputeWaterProximityValues(SC4Rect<int> const&)"},
    {0x100733d98u, 60u, "cSTETerrain::UpdateMoistureValues(SC4Rect<int> const&)"},
    {0x100733dd4u, 116u, "cSTETerrain::ClearSurfaceTextures(SC4Rect<int> const&)"},
    {0x100733e48u, 2268u, "cSTETerrain::DecideSurfaceCover(SC4Rect<int> const&)"},
    {0x100734724u, 822u, "cSTETerrain::SprayVolcanicAsh(SC4Rect<int> const&)"},
    {0x100734a5au, 36u, "cSTETerrain::IsWaterVertex(int) const"},
    {0x100734a7eu, 252u, "cSTETerrain::BeginBrushScrub(unsigned int, unsigned int)"},
    {0x100734b7au, 24u, "cSTETerrain::BeginLongTerrainOperation()"},
    {0x100734b92u, 234u, "cSTETerrain::PostMessageA(unsigned int, unsigned long, unsigned long, unsigned long, unsigned long)"},
    {0x100734c7cu, 98u, "cSTETerrain::LowerVertex(unsigned int, float)"},
    {0x100734cdeu, 100u, "cSTETerrain::RaiseVertex(unsigned int, float)"},
    {0x100734d42u, 44u, "cSTETerrain::GetNeighbors(int, int) const"},
    {0x100734d6eu, 90u, "cSTETerrain::BrushRadius(unsigned int) const"},
    {0x100734dc8u, 1216u, "cSTETerrain::CalculateNormalsAndAssignCliffTextures(SC4Rect<int> const&)"},
    {0x100735288u, 104u, "cSTETerrain::ClearVertexPropertyFlags(SC4Rect<int> const&, unsigned short)"},
    {0x1007352f0u, 168u, "cSTETerrain::ClearVertexCliffFlipTriangulationFlags(SC4Rect<int> const&)"},
    {0x100735398u, 660u, "cSTETerrain::MarkCliff(int, bool, bool)"},
    {0x10073562cu, 118u, "cSTETerrain::CellHasWater(unsigned int, unsigned int) const"},
    {0x1007356a2u, 114u, "cSTETerrain::CellHasLand(unsigned int, unsigned int) const"},
    {0x100735714u, 50u, "cSTETerrain::ChangeSeaLevel(bool)"},
    {0x100735746u, 202u, "cSTETerrain::ChangeTerrainLevel(bool, float)"},
    {0x100735810u, 54u, "cSTETerrain::ClearVertexPropertyFlags(unsigned short)"},
    {0x100735846u, 348u, "cSTETerrain::ComputeBooleanFnForRect(SC4Point<float>*, bool (cSTETerrain::*)(int) const, float) const"},
    {0x1007359a2u, 182u, "cSTETerrain::ComputeBoundsOfMarkedRegion(SC4Rect<int>&, unsigned short) const"},
    {0x100735a58u, 508u, "cSTETerrain::ComputeMinAndMaxAltitudeForTerrainRedisplay(SC4Rect<int> const&, float&, float&)"},
    {0x100735c54u, 168u, "cSTETerrain::FindMinAndMaxHeights(SC4Rect<int> const&, float&, float&)"},
    {0x100735cfcu, 596u, "cSTETerrain::Create()"},
    {0x100735f50u, 444u, "cSTETerrain::CreateFromGrayScaleImage(cIGZBuffer*)"},
    {0x10073610cu, 148u, "cSTETerrain::CreateFromGrayScaleImage(cRZString const&)"},
    {0x1007361a0u, 118u, "cSTETerrain::EnsureMinAllowedHeight()"},
    {0x100736216u, 158u, "cSTETerrain::DoErosion(unsigned int, bool)"},
    {0x1007362b4u, 114u, "cSTETerrain::FindMinAndMaxHeights()"},
    {0x100736326u, 454u, "cSTETerrain::CreateFlatAreas(bool)"},
    {0x1007364ecu, 572u, "cSTETerrain::LevelAreaStartingAt(int, float, unsigned short)"},
    {0x100736728u, 520u, "cSTETerrain::MarkRegion(int, bool (cSTETerrain::*)(int) const, bool (cSTETerrain::*)(int) const, unsigned short)"},
    {0x100736930u, 28u, "cSTETerrain::IsVertexWithMark1(int) const"},
    {0x10073694cu, 46u, "cSTETerrain::IsLandAndNotMark1(int) const"},
    {0x10073697au, 560u, "cSTETerrain::SmoothenMarkedRegion(SC4Rect<int> const&, int, unsigned short)"},
    {0x100736baau, 334u, "cSTETerrain::DoWindErosion(int)"},
    {0x100736cf8u, 70u, "cSTETerrain::CreateVertRectAroundVertex(SC4Rect<int>&, int, int, int)"},
    {0x100736d3eu, 434u, "cSTETerrain::CreateShoreCliffs(bool)"},
    {0x100736ef0u, 110u, "cSTETerrain::DetectShoreVertices()"},
    {0x100736f5eu, 118u, "cSTETerrain::GetShoreStartingAtVert(int, std::__1::deque<int, std::__1::allocator<int>>&, unsigned short)"},
    {0x100736fd4u, 30u, "cSTETerrain::IsVertexWithMark3(int) const"},
    {0x100736ff2u, 46u, "cSTETerrain::IsLandAndNotMark3(int) const"},
    {0x100737020u, 648u, "cSTETerrain::cSTETerrain()"},
    {0x1007372a8u, 330u, "cSTETerrain::~cSTETerrain() [GZCOM boilerplate]"},
    {0x1007373f2u, 10u, "cSTETerrain::~cSTETerrain()"},
    {0x1007373fcu, 52u, "cSTETerrain::~cSTETerrain() [GZCOM boilerplate]"},
    {0x100737430u, 234u, "cSTETerrain::DetermineTextureFamilyMembersForGridCells()"},
    {0x10073751au, 132u, "cSTETerrain::IsShoreVertex(int) const"},
    {0x10073759eu, 230u, "cSTETerrain::DoAnimationTick(unsigned int)"},
    {0x100737684u, 142u, "cSTETerrain::PostMessageToTarget(unsigned int, cIGZMessageTarget2*)"},
    {0x100737712u, 684u, "cSTETerrain::DoFloodFlowStep()"},
    {0x1007379beu, 284u, "cSTETerrain::TickVolcanicAshRegions(unsigned int)"},
    {0x100737adau, 118u, "cSTETerrain::RestoreWaterLevelsFromTempArray(SC4Rect<int>&)"},
    {0x100737b50u, 362u, "cSTETerrain::GetNeighborWithLowestWaterLevel(int)"},
    {0x100737cbau, 132u, "cSTETerrain::DoLighting(SC4Rect<int> const&)"},
    {0x100737d3eu, 1838u, "cSTETerrain::DoMessage(cIGZMessage2*)"},
    {0x10073847eu, 356u, "cSTETerrain::DoPostCityInit()"},
    {0x1007385e2u, 266u, "cSTETerrain::InitFloraBrushInfo()"},
    {0x1007386ecu, 24u, "cSTETerrain::EndLongTerrainOperation()"},
    {0x100738704u, 324u, "cSTETerrain::EndBrushScrub(unsigned int, unsigned int)"},
    {0x100738848u, 112u, "cSTETerrain::FlipInternalTriangulation(unsigned int, unsigned int, bool)"},
    {0x1007388b8u, 112u, "cSTETerrain::FlipExternalTriangulation(unsigned int, unsigned int, bool)"},
    {0x100738928u, 348u, "cSTETerrain::GetAltitude(float, float) const"},
    {0x100738a84u, 34u, "cSTETerrain::GetAltitude(int) const"},
    {0x100738aa6u, 42u, "cSTETerrain::GetAltitudeAtVertex(int, int) const"},
    {0x100738ad0u, 122u, "cSTETerrain::GetAltitudeAtNearestGrid(float, float)"},
    {0x100738b4au, 138u, "cSTETerrain::GetAltitudeCheckSum(SC4Rect<int> const&) const"},
    {0x100738bd4u, 140u, "cSTETerrain::GetAltitudes(SC4Rect<int> const&, float*, long, long)"},
    {0x100738c60u, 142u, "cSTETerrain::GetWaterDepths(SC4Rect<int> const&, float*, long, long)"},
    {0x100738ceeu, 26u, "cSTETerrain::GetBrushExemplarID(cISTETerrain::eBrushTypes, unsigned int&)"},
    {0x100738d08u, 110u, "cSTETerrain::GetCellCornerAltitudes(unsigned int, unsigned int, float*) const"},
    {0x100738d76u, 86u, "cSTETerrain::GetCostPerBrushApplication(unsigned int) const"},
    {0x100738dccu, 132u, "cSTETerrain::GetEffectIDProperty(cISCPropertyHolder*, unsigned int, unsigned int*)"},
    {0x100738e50u, 426u, "cSTETerrain::GetFaceNormal(float, float, cS3DVector3&)"},
    {0x100739012u, 658u, "cSTETerrain::GetLerpedNormal(float, float, cS3DVector3&)"},
    {0x1007392a4u, 342u, "cSTETerrain::GetLerpedWaterLevel(float, float) const"},
    {0x1007393fau, 188u, "cSTETerrain::GetMaxSurfaceLevel(unsigned int, unsigned int)"},
    {0x1007394b6u, 12u, "cSTETerrain::GetWaterSurfaceDisplayState()"},
    {0x1007394c2u, 12u, "cSTETerrain::GetPlumbingInfoDisplayState()"},
    {0x1007394ceu, 14u, "cSTETerrain::GetSeaLevel() const"},
    {0x1007394dcu, 192u, "cSTETerrain::GetSedimentValue(float, float)"},
    {0x100739610u, 392u, "cSTETerrain::TrackShoreWithLandToTheRight(int, std::__1::deque<int, std::__1::allocator<int>>&, unsigned short)"},
    {0x100739798u, 402u, "cSTETerrain::TrackShoreWithLandToTheLeft(int, std::__1::deque<int, std::__1::allocator<int>>&, unsigned short)"},
    {0x10073992au, 76u, "cSTETerrain::GetTerrainCellFlags(unsigned int, unsigned int) const"},
    {0x100739976u, 114u, "cSTETerrain::GetUIElementsForBrush(unsigned int, cISTETerrain::tUIElementsForBrush&) const"},
    {0x1007399e8u, 54u, "cSTETerrain::GetVertexNormal(unsigned int, unsigned int, cS3DVector3&)"},
    {0x100739a1eu, 24u, "cSTETerrain::GetView()"},
    {0x100739a36u, 50u, "cSTETerrain::GetWaterLevel(int) const"},
    {0x100739a68u, 206u, "cSTETerrain::GetWaterLevel(float, float) const"},
    {0x100739b36u, 34u, "cSTETerrain::GetWaterDepthAtVertex(int) const"},
    {0x100739b58u, 44u, "cSTETerrain::GetWaterDepthAtVertex(int, int) const"},
    {0x100739b84u, 100u, "cSTETerrain::HideEvenTriangle(unsigned int, unsigned int, bool)"},
    {0x100739be8u, 100u, "cSTETerrain::HideOddTriangle(unsigned int, unsigned int, bool)"},
    {0x100739c4cu, 100u, "cSTETerrain::HideTerrainCell(unsigned int, unsigned int, bool)"},
    {0x100739cb0u, 1686u, "cSTETerrain::Init()"},
    {0x10073a346u, 100u, "cSTETerrain::StaticInit()"},
    {0x10073a3aau, 282u, "cSTETerrain::InitBrushParameters()"},
    {0x10073a4c4u, 50u, "cSTETerrain::IsEvenTriangleHidden(unsigned int, unsigned int) const"},
    {0x10073a4f6u, 650u, "cSTETerrain::IsGoodWaveProducerLocation(float, float, float) const"},
    {0x10073a780u, 32u, "cSTETerrain::IsLandVertex(int) const"},
    {0x10073a7a0u, 52u, "cSTETerrain::IsLandVertex(unsigned int, unsigned int) const"},
    {0x10073a7d4u, 50u, "cSTETerrain::IsOddTriangleHidden(unsigned int, unsigned int) const"},
    {0x10073a806u, 56u, "cSTETerrain::IsSeaVertex(unsigned int, unsigned int) const"},
    {0x10073a83eu, 48u, "cSTETerrain::IsTriangulationFlipped(unsigned int, unsigned int) const"},
    {0x10073a86eu, 56u, "cSTETerrain::IsWaterVertex(unsigned int, unsigned int) const"},
    {0x10073a8a6u, 58u, "cSTETerrain::IsWholeCellHidden(unsigned int, unsigned int) const"},
    {0x10073a8e0u, 290u, "cSTETerrain::IsPointHidden(float, float) const"},
    {0x10073aa02u, 144u, "cSTETerrain::NumberOfWaterVertices(SC4Rect<int> const&)"},
    {0x10073aa92u, 80u, "cSTETerrain::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x10073aafeu, 1024u, "cSTETerrain::ReadBrushComponentProperties(cISCPropertyHolder*, cSTETerrain::tBrushDescription&)"},
    {0x10073aefeu, 876u, "cSTETerrain::ReadBrushDescription(cGZPersistResourceKey const&, void*)"},
    {0x10073b26au, 334u, "cSTETerrain::ReadBrushWholeProperties(cISCPropertyHolder*, cSTETerrain::tBrushDescription&)"},
    {0x10073b3b8u, 228u, "cSTETerrain::ReadBrushUIProperties(cISCPropertyHolder*, cSTETerrain::tBrushDescription&)"},
    {0x10073b49cu, 172u, "cSTETerrain::ReadBrushDescriptions()"},
    {0x10073b548u, 186u, "cSTETerrain::ReleaseBrushDescriptions()"},
    {0x10073b602u, 1428u, "cSTETerrain::ReadMiscTexturesForTerrainTypes(cRZIniFile&)"},
    {0x10073bb96u, 5780u, "cSTETerrain::ReadTerrainIniFile()"},
    {0x10073d22au, 1658u, "cSTETerrain::ReadTextureInfo(cGZPersistResourceKey const&, void*)"},
    {0x10073d8a4u, 186u, "cSTETerrain::ReadTextureInfos()"},
    {0x10073d95eu, 1192u, "cSTETerrain::ReadTuningParameters()"},
    {0x10073de06u, 124u, "cSTETerrain::RecomputeWaterLevel(SC4Rect<int>&)"},
    {0x10073de82u, 120u, "cSTETerrain::RedisplayTerrain(bool, bool, SC4Point<int> const&, SC4Point<int> const&, unsigned int)"},
    {0x10073defau, 448u, "cSTETerrain::RedisplayTerrain(bool, bool, SC4Rect<int> const&, unsigned int)"},
    {0x10073e0bau, 32u, "cSTETerrain::Release() [GZCOM boilerplate]"},
    {0x10073e122u, 112u, "cSTETerrain::SaveWaterLevelsInTempArray(SC4Rect<int>&)"},
    {0x10073e192u, 96u, "cSTETerrain::SetAltitude(int, float)"},
    {0x10073e1f2u, 192u, "cSTETerrain::SetAltitude(SC4Rect<int>&, float, bool, unsigned int)"},
    {0x10073e2b2u, 70u, "cSTETerrain::SetAltitudeAtVertex(int, int, float)"},
    {0x10073e2f8u, 116u, "cSTETerrain::LockAltitude(SC4Rect<int>&, bool)"},
    {0x10073e36cu, 172u, "cSTETerrain::SetWaterSurfaceDisplayState(bool)"},
    {0x10073e418u, 14u, "cSTETerrain::SetPlumbingInfoDisplayState(bool)"},
    {0x10073e426u, 192u, "cSTETerrain::SetTerrainCellFlags(unsigned int, unsigned int, unsigned int)"},
    {0x10073e4e6u, 112u, "cSTETerrain::SetTerrainType(int)"},
    {0x10073e556u, 112u, "cSTETerrain::SetSeaLevel(float)"},
    {0x10073e5c6u, 840u, "cSTETerrain::Shutdown()"},
    {0x10073e90eu, 36u, "cSTETerrain::StartAccumulatingTextureUpdates()"},
    {0x10073e932u, 172u, "cSTETerrain::StartFlood()"},
    {0x10073e9deu, 86u, "cSTETerrain::StaticShutdown(void*)"},
    {0x10073ea34u, 114u, "cSTETerrain::StopAccumulatingTextureUpdates()"},
    {0x10073eb1eu, 546u, "cSTETerrain::TryToCreateWaveProducerForShore(std::__1::deque<int, std::__1::allocator<int>>&, int, int, float&, float&, float&)"},
    {0x10073ed40u, 666u, "cSTETerrain::UpdateWaveProducers()"},
    {0x10073f17au, 78u, "cSTETerrain::Save(cIGZPersistDBSegment*) const"},
    {0x10073f29eu, 174u, "cSTETerrain::SaveAltitudes(cIGZPersistDBSegment*) const"},
    {0x10073f3fcu, 344u, "cSTETerrain::SaveEdgeAltitudes(int, cIGZPersistDBSegment*) const"},
    {0x10073f554u, 66u, "cSTETerrain::Load(cIGZPersistDBSegment*)"},
    {0x10073f686u, 276u, "cSTETerrain::LoadAltitudes(cIGZPersistDBSegment*)"},
    {0x10073f85eu, 184u, "cSTETerrain::LoadEdgeAltitudes(int, cIGZPersistDBSegment*, std::__1::vector<float, std::__1::allocator<float>>&) const"},
    {0x10073f916u, 620u, "cSTETerrain::ReconcileEdgesWithNeighboringCities(bool, bool, bool)"},
    {0x10073fb82u, 1278u, "cSTETerrain::ComputeEdgeVerticesToReconcile(std::__1::map<int, float, std::__1::less<int>, std::__1::allocator<std::__1::pair<int const, float>>>&, bool)"},
    {0x100740080u, 1620u, "cSTETerrain::ComputeCellsAndVertsAffectedByReconcilliation(std::__1::map<int, float, std::__1::less<int>, std::__1::allocator<std::__1::pair<int const, float>>> const&, std::__1::unordered_set<int, std::__1::hash<int>, std::__1::equal_to<int>, std::__1::allocator<int>>&, std::__1::unordered_set<int, std::__1::hash<int>, std::__1::equal_to<int>, std::__1::allocator<int>>&)"},
    {0x1007406d4u, 232u, "cSTETerrain::HighlightCellsAffectedByEdgeReconcilliation(std::__1::unordered_set<int, std::__1::hash<int>, std::__1::equal_to<int>, std::__1::allocator<int>> const&)"},
    {0x1007407bcu, 372u, "cSTETerrain::LockEdgeVerticesAndSmoothen(std::__1::map<int, float, std::__1::less<int>, std::__1::allocator<std::__1::pair<int const, float>>> const&, std::__1::unordered_set<int, std::__1::hash<int>, std::__1::equal_to<int>, std::__1::allocator<int>> const&)"},
    {0x100740930u, 98u, "cSTETerrain::GetCityVertRect(cISC4RegionalCity const&, SC4Rect<int>&)"},
    {0x100740992u, 328u, "cSTETerrain::ComputeMaxHeightsForEdgeVertices(int, SC4Rect<int> const&, cISC4RegionalCity const*, std::__1::map<int, float, std::__1::less<int>, std::__1::allocator<std::__1::pair<int const, float>>>&)"},
    {0x100740adau, 228u, "cSTETerrain::UpdateMaxHeightsForEdgeVerticesAssumingDefaultNeighborHt(int, int, int, int, int, float, std::__1::map<int, float, std::__1::less<int>, std::__1::allocator<std::__1::pair<int const, float>>>&)"},
    {0x100740bbeu, 112u, "cSTETerrain::GetEdgeSharedWithNeighboringCity(SC4Rect<int> const&, SC4Rect<int> const&, SC4Point<int>&, SC4Point<int>&)"},
    {0x100740c2eu, 250u, "cSTETerrain::ComputeMaxHeightsForEdgeVertices(SC4Rect<int> const&, SC4Rect<int> const&, SC4Point<int> const&, SC4Point<int> const&, std::__1::vector<float, std::__1::allocator<float>> const&, std::__1::map<int, float, std::__1::less<int>, std::__1::allocator<std::__1::pair<int const, float>>>&)"},
    {0x100740de0u, 50u, "cSTETerrain::SaveObliterated(cIGZPersistDBSegment*)"},
    {0x100740e12u, 18u, "cSTETerrain::LoadObliterated(cIGZPersistDBSegment*)"},
    {0x100740e24u, 10u, "cSTETerrain::IsInitialized()"},
    {0x100740e2eu, 14u, "cSTETerrain::CellWidth()"},
    {0x100740e3cu, 10u, "cSTETerrain::CellCountX()"},
    {0x100740e46u, 10u, "cSTETerrain::CellCountZ()"},
    {0x100740e50u, 14u, "cSTETerrain::GetVertIndex(int, int) const"},
    {0x100740e5eu, 28u, "cSTETerrain::GetVertex(int, int&, int&) const"},
    {0x100740e7au, 42u, "cSTETerrain::LocationIsInBounds(float, float) const"},
    {0x100740ea4u, 14u, "cSTETerrain::MinHeight()"},
    {0x100740eb2u, 14u, "cSTETerrain::MaxHeight()"},
    {0x100740ec0u, 14u, "cSTETerrain::MinAllowedHeight()"},
    {0x100740eceu, 14u, "cSTETerrain::MaxAllowedHeight()"},
    {0x100740edcu, 36u, "cSTETerrain::GetTerrainGridIndex(float, float)"},
    {0x100740f00u, 10u, "cSTETerrain::TerrainType() const"},
    {0x100740f0au, 14u, "cSTETerrain::GetDefaultTerrainRaiseLowerAmount() const"},
    {0x100740f18u, 12u, "cSTETerrain::NumberOfWaterVertices() const"},
    {0x100740f24u, 12u, "cSTETerrain::IsAccumulatingTextureUpdates()"},

    // cSTESurfaceWater — 33 entry points
    {0x10072e11cu, 150u, "cSTESurfaceWater::~cSTESurfaceWater() [GZCOM boilerplate]"},
    {0x10072e1b2u, 10u, "cSTESurfaceWater::~cSTESurfaceWater()"},
    {0x10072e1bcu, 46u, "cSTESurfaceWater::~cSTESurfaceWater() [GZCOM boilerplate]"},
    {0x10072e1eau, 52u, "cSTESurfaceWater::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x10072e23au, 14u, "cSTESurfaceWater::AddRef()"},
    {0x10072e264u, 62u, "cSTESurfaceWater::Release() [GZCOM boilerplate]"},
    {0x10072e312u, 748u, "cSTESurfaceWater::Init()"},
    {0x10072e648u, 144u, "cSTESurfaceWater::ResetWaterToSeaLevel(SC4Rect<int> const&)"},
    {0x10072e6d8u, 402u, "cSTESurfaceWater::Shutdown()"},
    {0x10072e91cu, 314u, "cSTESurfaceWater::SaveWaterInfo(cIGZPersistDBSegment*) const"},
    {0x10072eb28u, 292u, "cSTESurfaceWater::LoadWaterInfo(cIGZPersistDBSegment*)"},
    {0x10072ec5au, 880u, "cSTESurfaceWater::DoMessage(cIGZMessage2*)"},
    {0x10072efcau, 1344u, "cSTESurfaceWater::UpdateNeighborConnections()"},
    {0x10072f50au, 230u, "cSTESurfaceWater::UpdateBodiesOfWater()"},
    {0x10072f5f0u, 77u, "cSTESurfaceWater::AnimateWater()"},
    {0x10072f696u, 70u, "cSTESurfaceWater::EndBrushScrub(unsigned int, unsigned int)"},
    {0x10072f6dcu, 22u, "cSTESurfaceWater::GetWaterDepthAtVertex(int) const"},
    {0x10072f6f2u, 124u, "cSTESurfaceWater::GetWaterDepthForCell(int, int) const"},
    {0x10072f76eu, 22u, "cSTESurfaceWater::HasDraftClearance(int, int) const"},
    {0x10072f784u, 104u, "cSTESurfaceWater::IsNavigable(cS3DVector3, int, cISC4Lot*) const"},
    {0x10072f7ecu, 880u, "cSTESurfaceWater::IsNavigable(int, int, int, cISC4Lot*) const"},
    {0x10072fb5cu, 2132u, "cSTESurfaceWater::UpdateFloodWaters()"},
    {0x1007303b0u, 226u, "cSTESurfaceWater::FindLocalMinimumOrWater(int)"},
    {0x100730492u, 910u, "cSTESurfaceWater::GetBasinVertices(int, float, float, std::__1::list<int, std::__1::allocator<int>>&, std::__1::list<int, std::__1::allocator<int>>&, std::__1::list<int, std::__1::allocator<int>>&)"},
    {0x100730820u, 30u, "cSTESurfaceWater::Rain(int, cSTETerrain::tBrushContext&, SC4Rect<int>&, float)"},
    {0x100730882u, 660u, "cSTESurfaceWater::Rain_Static(int, cSTETerrain::tBrushContext&, SC4Rect<int>&, float)"},
    {0x100730b16u, 42u, "cSTESurfaceWater::CreateSpring(int)"},
    {0x100730b40u, 264u, "cSTESurfaceWater::AltitudeAtWhichFloodingWillOccur(int, int) const"},
    {0x100730c48u, 42u, "cSTESurfaceWater::GetWaterVelocity(int, int) const"},
    {0x100730c72u, 94u, "cSTESurfaceWater::GetWaterType(int, int) const"},
    {0x100730cd0u, 86u, "cSTESurfaceWater::IsFlatWater(int) const"},
    {0x100730d26u, 530u, "cSTESurfaceWater::FloodFillBodyOfWater(int, int, unsigned short)"},
    {0x100730f38u, 14u, "cSTESurfaceWater::GetBodyOfWaterMap() const"},

    // cSC4Lot — 88 entry points
    {0x10017ed2eu, 26u, "cSC4Lot::operator new(unsigned long)"},
    {0x10017ed48u, 35u, "cSC4Lot::operator delete(void*)"},
    {0x10017ed76u, 622u, "cSC4Lot::cSC4Lot(int, int, int, int, int)"},
    {0x10017efe4u, 10u, "cSC4Lot::cSC4Lot(int, int, int, int, int)"},
    {0x10017efeeu, 264u, "cSC4Lot::cSC4Lot()"},
    {0x10017f0f6u, 842u, "cSC4Lot::cSC4Lot(cSC4Lot const&)"},
    {0x10017f440u, 262u, "cSC4Lot::~cSC4Lot() [GZCOM boilerplate]"},
    {0x10017f546u, 10u, "cSC4Lot::~cSC4Lot()"},
    {0x10017f55eu, 62u, "cSC4Lot::~cSC4Lot() [GZCOM boilerplate]"},
    {0x10017f5aau, 52u, "cSC4Lot::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x10017f608u, 14u, "cSC4Lot::AddRef()"},
    {0x10017f640u, 68u, "cSC4Lot::Release() [GZCOM boilerplate]"},
    {0x10017f732u, 312u, "cSC4Lot::Init()"},
    {0x10017f86au, 228u, "cSC4Lot::Shutdown()"},
    {0x10017f94eu, 28u, "cSC4Lot::PostCityInit()"},
    {0x10017f96au, 232u, "cSC4Lot::SetFlags(int)"},
    {0x10017fa52u, 24u, "cSC4Lot::GetLocation(int&, int&) const"},
    {0x10017fa6au, 22u, "cSC4Lot::GetSize(unsigned char&, unsigned char&) const"},
    {0x10017fa80u, 34u, "cSC4Lot::GetBoundingRect(SC4Rect<int>&) const"},
    {0x10017faa2u, 98u, "cSC4Lot::GetCenter(float*) const"},
    {0x10017fb04u, 96u, "cSC4Lot::SetAltitude(float)"},
    {0x10017fb64u, 58u, "cSC4Lot::UpdateSlope()"},
    {0x10017fb9eu, 58u, "cSC4Lot::GetDaysSinceLastStateChange() const"},
    {0x10017fbd8u, 1458u, "cSC4Lot::SetState(cISC4Lot::HabitationState, cISC4BuildingOccupant::WealthType, cISC4Lot::VacateReason)"},
    {0x10018018au, 416u, "cSC4Lot::UpdatePrimaryOccupantWealth()"},
    {0x10018032au, 370u, "cSC4Lot::UpdateNeedsPower()"},
    {0x10018049cu, 376u, "cSC4Lot::SetCondition(cISC4Lot::BuildingCondition)"},
    {0x100180614u, 78u, "cSC4Lot::SetLotConfiguration(cISC4LotConfiguration*)"},
    {0x100180662u, 266u, "cSC4Lot::GetLotConfigurationObject(cISC4Occupant*)"},
    {0x10018076cu, 92u, "cSC4Lot::GetPopulation(cISC4BuildingDevelopmentSimulator::DeveloperType) const"},
    {0x1001807c8u, 72u, "cSC4Lot::GetTravelPopulation(cISC4BuildingDevelopmentSimulator::DeveloperType) const"},
    {0x100180810u, 84u, "cSC4Lot::SetTravelDesignate(cISC4Lot*)"},
    {0x100180864u, 88u, "cSC4Lot::ResetTravelPopulation()"},
    {0x1001808bcu, 392u, "cSC4Lot::UpdateJobs()"},
    {0x100180a44u, 424u, "cSC4Lot::AddTravelPopulation(cISC4Lot*)"},
    {0x100180becu, 516u, "cSC4Lot::GetJobs(float*) const"},
    {0x100180df0u, 52u, "cSC4Lot::GetTravelJobs(cISC4BuildingOccupant::WealthType) const"},
    {0x100180e24u, 446u, "cSC4Lot::GetOccupancy() const"},
    {0x100180fe2u, 850u, "cSC4Lot::GetOccupancyByType(std::__1::map<unsigned int, SC4Percentage, std::__1::less<unsigned int>, std::__1::allocator<std::__1::pair<unsigned int const, SC4Percentage>>>&) const"},
    {0x1001813ecu, 118u, "cSC4Lot::GetCapacity(cISC4BuildingOccupant::WealthType) const"},
    {0x100181462u, 182u, "cSC4Lot::GetCapacity(cISC4BuildingDevelopmentSimulator::DeveloperType, bool) const"},
    {0x100181518u, 450u, "cSC4Lot::GetAvailableCapacity(cISC4BuildingDevelopmentSimulator::DeveloperType, bool) const"},
    {0x1001816dau, 48u, "cSC4Lot::GetBuildingType(bool) const"},
    {0x10018170au, 180u, "cSC4Lot::GetBuilding() const"},
    {0x1001817beu, 378u, "cSC4Lot::GetLotOccupants(ilist<cISC4Occupant*>&) const"},
    {0x1001819a8u, 254u, "cSC4Lot::SetPopulation(cISC4BuildingDevelopmentSimulator::DeveloperType, unsigned short)"},
    {0x100181aa6u, 52u, "cSC4Lot::UpdateZoneType()"},
    {0x100181adau, 134u, "cSC4Lot::GetLandValue() const"},
    {0x100181b60u, 76u, "cSC4Lot::HasJobAccess() const"},
    {0x100181bacu, 182u, "cSC4Lot::HasOrganicRoadAccess(cISC4BuildingOccupant::PurposeType) const"},
    {0x100181c62u, 690u, "cSC4Lot::CalculateRoadAccess()"},
    {0x100181f14u, 380u, "cSC4Lot::CalculateOrganicRoadAccess(cISC4BuildingOccupant::PurposeType)"},
    {0x100182090u, 116u, "cSC4Lot::UpdateRoadAccess()"},
    {0x100182104u, 538u, "cSC4Lot::CalculateFerryTerminalRoadAccess()"},
    {0x1001823d2u, 1646u, "cSC4Lot::UpdateTrafficVolume()"},
    {0x10018397eu, 10u, "cSC4Lot::AsPropertyHolder()"},
    {0x100183988u, 10u, "cSC4Lot::AsTrafficSource()"},
    {0x100183992u, 12u, "cSC4Lot::IsInitialized() const"},
    {0x10018399eu, 10u, "cSC4Lot::GetLotConfiguration()"},
    {0x1001839a8u, 14u, "cSC4Lot::GetFlags() const"},
    {0x1001839b6u, 12u, "cSC4Lot::GetAltitude() const"},
    {0x1001839c2u, 12u, "cSC4Lot::GetSlope() const"},
    {0x1001839ceu, 12u, "cSC4Lot::GetSlopeAtTimeOfConstruction() const"},
    {0x1001839dau, 14u, "cSC4Lot::GetFacing() const"},
    {0x1001839e8u, 14u, "cSC4Lot::SetFacing(int)"},
    {0x1001839f6u, 14u, "cSC4Lot::GetUnit()"},
    {0x100183a04u, 66u, "cSC4Lot::SetUnit(cISC4BusinessUnit*)"},
    {0x100183a46u, 20u, "cSC4Lot::HasRoadAccess() const"},
    {0x100183a5au, 14u, "cSC4Lot::GetZoneType() const"},
    {0x100183a68u, 14u, "cSC4Lot::GetState() const"},
    {0x100183a76u, 14u, "cSC4Lot::GetCondition() const"},
    {0x100183a84u, 16u, "cSC4Lot::IsInLivableState() const"},
    {0x100183a94u, 20u, "cSC4Lot::HasCapacity() const"},
    {0x100183aa8u, 30u, "cSC4Lot::IsInDemolishableState() const"},
    {0x100183ac6u, 14u, "cSC4Lot::GetOccupantWealth() const"},
    {0x100183ad4u, 18u, "cSC4Lot::IsHistorical() const"},
    {0x100183ae6u, 30u, "cSC4Lot::SetHistorical(bool)"},
    {0x100183b04u, 14u, "cSC4Lot::GetVacateReason() const"},
    {0x100183b12u, 16u, "cSC4Lot::SetBuildingType(unsigned int)"},
    {0x100183b22u, 14u, "cSC4Lot::GetPopulations() const"},
    {0x100183b30u, 14u, "cSC4Lot::GetTravelDesignate() const"},
    {0x100183b3eu, 18u, "cSC4Lot::GetNeedsPower() const"},
    {0x100183b50u, 18u, "cSC4Lot::GetHasPower() const"},
    {0x100183b62u, 30u, "cSC4Lot::SetHasPower(bool)"},
    {0x100183b80u, 18u, "cSC4Lot::GetVerticesAreLocked() const"},
    {0x100183b92u, 30u, "cSC4Lot::SetVerticesAreLocked(bool)"},
    {0x100183bbcu, 14u, "cSC4Lot::GetTripDataList()"},
    {0x100183bcau, 36u, "cSC4Lot::GetSourceCells() const"},

    // cSC4RegionalCity — 87 entry points
    {0x1003ad932u, 10u, "cSC4RegionalCity::cSC4RegionalCity()"},
    {0x1003ad948u, 1860u, "cSC4RegionalCity::cSC4RegionalCity()"},
    {0x1003ae08cu, 426u, "cSC4RegionalCity::~cSC4RegionalCity() [GZCOM boilerplate]"},
    {0x1003ae236u, 10u, "cSC4RegionalCity::~cSC4RegionalCity()"},
    {0x1003ae240u, 46u, "cSC4RegionalCity::~cSC4RegionalCity() [GZCOM boilerplate]"},
    {0x1003ae26eu, 74u, "cSC4RegionalCity::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x1003ae2d4u, 14u, "cSC4RegionalCity::AddRef()"},
    {0x1003ae2feu, 68u, "cSC4RegionalCity::Release() [GZCOM boilerplate]"},
    {0x1003ae3b6u, 142u, "cSC4RegionalCity::Init()"},
    {0x1003ae444u, 180u, "cSC4RegionalCity::Shutdown()"},
    {0x1003af8f8u, 104u, "cSC4RegionalCity::SetupPreferences(SC4NewCityPreferences*)"},
    {0x1003af960u, 1082u, "cSC4RegionalCity::UpdateCityCache(SC4NewCityPreferences*)"},
    {0x1003afd9au, 224u, "cSC4RegionalCity::SetupCity(cISC4City*)"},
    {0x1003afe7au, 1662u, "cSC4RegionalCity::UpdateCityCache(cISC4City*)"},
    {0x1003b04f8u, 492u, "cSC4RegionalCity::DoMessage(cIGZMessage2*)"},
    {0x1003b06e4u, 182u, "cSC4RegionalCity::CalculatePotentialPopulations(cISC4City*)"},
    {0x1003b079au, 138u, "cSC4RegionalCity::UpdateExtrapolatedPopulations(cISC4City*)"},
    {0x1003b0824u, 292u, "cSC4RegionalCity::MarkConnectionsForRemoval()"},
    {0x1003b0948u, 408u, "cSC4RegionalCity::TreatAllIncomingConnectionsAsNew()"},
    {0x1003b0ae0u, 4608u, "cSC4RegionalCity::CreateSymmetricConnections()"},
    {0x1003b1cf2u, 62u, "cSC4RegionalCity::GetPopulation(unsigned int) const"},
    {0x1003b1d30u, 72u, "cSC4RegionalCity::GetExtrapolatedPopulation(unsigned int) const"},
    {0x1003b1d78u, 126u, "cSC4RegionalCity::FindConnection(int, int, int) const"},
    {0x1003b1df6u, 62u, "cSC4RegionalCity::GetAllConnections(ilist<cRZAutoRefCount<cISC4NeighborConnection>>&) const"},
    {0x1003b1e34u, 202u, "cSC4RegionalCity::ChangeSymmetricConnection(cISC4NeighborConnection*, bool)"},
    {0x1003b1efeu, 128u, "cSC4RegionalCity::RepairConnectionsAfterImport()"},
    {0x1003b2040u, 274u, "cSC4RegionalCity::GetTaxRate(unsigned int) const"},
    {0x1003b2152u, 70u, "cSC4RegionalCity::GetAllowableExtrapolation(unsigned int) const"},
    {0x1003b2198u, 68u, "cSC4RegionalCity::ExtrapolateGrowth(unsigned int, float)"},
    {0x1003b21dcu, 14u, "cSC4RegionalCity::GetBudget() const"},
    {0x1003b21eau, 14u, "cSC4RegionalCity::SetBudget(double)"},
    {0x1003b21f8u, 14u, "cSC4RegionalCity::GetIncome() const"},
    {0x1003b2206u, 14u, "cSC4RegionalCity::SetIncome(double)"},
    {0x1003b2214u, 40u, "cSC4RegionalCity::GetExported(int) const"},
    {0x1003b2306u, 56u, "cSC4RegionalCity::SetExported(int, double)"},
    {0x1003b233eu, 40u, "cSC4RegionalCity::GetImported(int) const"},
    {0x1003b2366u, 56u, "cSC4RegionalCity::SetImported(int, double)"},
    {0x1003b239eu, 40u, "cSC4RegionalCity::GetProduced(int) const"},
    {0x1003b23c6u, 56u, "cSC4RegionalCity::SetProduced(int, double)"},
    {0x1003b23feu, 40u, "cSC4RegionalCity::GetDemanded(int) const"},
    {0x1003b2426u, 56u, "cSC4RegionalCity::SetDemanded(int, double)"},
    {0x1003b245eu, 40u, "cSC4RegionalCity::GetCostPerUnit(int) const"},
    {0x1003b2486u, 56u, "cSC4RegionalCity::SetCostPerUnit(int, double)"},
    {0x1003b24beu, 118u, "cSC4RegionalCity::GetCommodityBalance(int) const"},
    {0x1003b2534u, 550u, "cSC4RegionalCity::ExtractLocalDeals(ilist<cRZAutoRefCount<cISC4NeighborDeal>>&, ilist<cRZAutoRefCount<cISC4NeighborDeal>>&, bool)"},
    {0x1003b275au, 72u, "cSC4RegionalCity::SetLocalDeals(ilist<cRZAutoRefCount<cISC4NeighborDeal>>&)"},
    {0x1003b27a2u, 40u, "cSC4RegionalCity::GetLocalDeals(ilist<cRZAutoRefCount<cISC4NeighborDeal>>&)"},
    {0x1003b27cau, 92u, "cSC4RegionalCity::UpdateLocalDeals()"},
    {0x1003b2826u, 326u, "cSC4RegionalCity::UpdateImportExport(cISC4NeighborDeal const*)"},
    {0x1003b296cu, 218u, "cSC4RegionalCity::UpdateImportExport()"},
    {0x1003b2a46u, 92u, "cSC4RegionalCity::GetPointsOfInterest(cISC4RegionalCity::PointOfInterestType, std::__1::vector<cISC4RegionalCity::PointOfInterest, std::__1::allocator<cISC4RegionalCity::PointOfInterest>>&)"},
    {0x1003b2aa2u, 360u, "cSC4RegionalCity::SetPosition(int, int, bool)"},
    {0x1003b2c0au, 22u, "cSC4RegionalCity::IsTutorial() const"},
    {0x1003b2c20u, 16u, "cSC4RegionalCity::GetPosition(int&, int&) const"},
    {0x1003b2c30u, 16u, "cSC4RegionalCity::GetCitySize(int&, int&) const"},
    {0x1003b2c40u, 12u, "cSC4RegionalCity::SetCitySize(int, int)"},
    {0x1003b2c4cu, 10u, "cSC4RegionalCity::GetPopulation() const"},
    {0x1003b2c56u, 10u, "cSC4RegionalCity::GetCommercialJobs() const"},
    {0x1003b2c60u, 10u, "cSC4RegionalCity::GetIndustrialJobs() const"},
    {0x1003b2c6au, 10u, "cSC4RegionalCity::GetWorkforcePercentage() const"},
    {0x1003b2c74u, 10u, "cSC4RegionalCity::GetMayorRating() const"},
    {0x1003b2c7eu, 10u, "cSC4RegionalCity::GetDifficultyLevel() const"},
    {0x1003b2c88u, 12u, "cSC4RegionalCity::GetCitySerialNumber() const"},
    {0x1003b2c94u, 12u, "cSC4RegionalCity::SetCitySerialNumber(unsigned int)"},
    {0x1003b2ca0u, 24u, "cSC4RegionalCity::GetOriginalLanguageAndCountry(int&, int&) const"},
    {0x1003b2cb8u, 24u, "cSC4RegionalCity::GetLastLanguageAndCountry(int&, int&)"},
    {0x1003b2cd0u, 28u, "cSC4RegionalCity::GetCitySaveFilePath(cIGZString&) const"},
    {0x1003b2cecu, 18u, "cSC4RegionalCity::SetCitySaveFilePath(cIGZString const&)"},
    {0x1003b2cfeu, 28u, "cSC4RegionalCity::GetCityName(cIGZString&) const"},
    {0x1003b2d1au, 18u, "cSC4RegionalCity::SetCityName(cIGZString const&)"},
    {0x1003b2d2cu, 28u, "cSC4RegionalCity::GetMayorName(cIGZString&) const"},
    {0x1003b2d48u, 18u, "cSC4RegionalCity::SetMayorName(cIGZString const&)"},
    {0x1003b2d5au, 28u, "cSC4RegionalCity::GetUtilityAdvisorName(cIGZString&) const"},
    {0x1003b2d76u, 18u, "cSC4RegionalCity::SetUtilityAdvisorName(cIGZString const&)"},
    {0x1003b2d88u, 28u, "cSC4RegionalCity::GetCityDescription(cIGZString&) const"},
    {0x1003b2da4u, 18u, "cSC4RegionalCity::SetCityDescription(cIGZString const&)"},
    {0x1003b2db6u, 12u, "cSC4RegionalCity::GetBirthDate() const"},
    {0x1003b2dc2u, 12u, "cSC4RegionalCity::SetBirthDate(unsigned int)"},
    {0x1003b2dceu, 16u, "cSC4RegionalCity::GetEstablished() const"},
    {0x1003b2ddeu, 14u, "cSC4RegionalCity::SetEstablished(bool)"},
    {0x1003b2decu, 22u, "cSC4RegionalCity::GetWorldPosition(float&, float&) const"},
    {0x1003b2e02u, 22u, "cSC4RegionalCity::SetWorldPosition(float, float)"},
    {0x1003b2e18u, 14u, "cSC4RegionalCity::GetWorldBaseElevation() const"},
    {0x1003b2e26u, 14u, "cSC4RegionalCity::SetWorldBaseElevation(float)"},
    {0x1003b2e34u, 32u, "cSC4RegionalCity::GetWorldHemisphere() const"},
    {0x1003b2e54u, 12u, "cSC4RegionalCity::GetTutorialGUID() const"},
    {0x1003b2e60u, 12u, "cSC4RegionalCity::SetTutorialGUID(int)"},

    // cSC4OccupantManager — 63 entry points
    {0x1002465e0u, 166u, "cSC4OccupantManager::InternalOccupantHashSet::InternalOccupantHashSet()"},
    {0x100246686u, 122u, "cSC4OccupantManager::InternalOccupantHashSet::~InternalOccupantHashSet() [GZCOM boilerplate]"},
    {0x100246700u, 72u, "cSC4OccupantManager::InternalOccupantHashSet::clear()"},
    {0x100246748u, 416u, "cSC4OccupantManager::InternalOccupantHashSet::insert(cISC4Occupant*)"},
    {0x1002468e8u, 234u, "cSC4OccupantManager::InternalOccupantHashSet::RehashVectorToHashTable()"},
    {0x100246a4cu, 202u, "cSC4OccupantManager::InternalOccupantHashSet::RehashHashTable()"},
    {0x100246b90u, 170u, "cSC4OccupantManager::cSC4OccupantManager()"},
    {0x100246c3au, 132u, "cSC4OccupantManager::~cSC4OccupantManager() [GZCOM boilerplate]"},
    {0x100246cbeu, 10u, "cSC4OccupantManager::~cSC4OccupantManager()"},
    {0x100246cc8u, 46u, "cSC4OccupantManager::~cSC4OccupantManager() [GZCOM boilerplate]"},
    {0x100246cf6u, 62u, "cSC4OccupantManager::AddRef() [GZCOM boilerplate]"},
    {0x100246db0u, 108u, "cSC4OccupantManager::Release() [GZCOM boilerplate]"},
    {0x100246e38u, 68u, "cSC4OccupantManager::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x100246e98u, 338u, "cSC4OccupantManager::Init()"},
    {0x1002470a6u, 132u, "cSC4OccupantManager::Shutdown()"},
    {0x100247588u, 18u, "cSC4OccupantManager::DoMessage(cIGZMessage2*)"},
    {0x1002475acu, 122u, "cSC4OccupantManager::SetCitySize(float, float)"},
    {0x100247626u, 16u, "cSC4OccupantManager::GetOccupantManagerCellSizes(float&, float&) const"},
    {0x100247636u, 16u, "cSC4OccupantManager::GetWorldCellCount(int&, int&) const"},
    {0x100247646u, 16u, "cSC4OccupantManager::GetOccupantManagerCellCount(int&, int&) const"},
    {0x100247656u, 18u, "cSC4OccupantManager::WorldCellToOccupantManagerCell(int, int, int&, int&) const"},
    {0x100247668u, 18u, "cSC4OccupantManager::OccupantManagerCellToWorldCell(int, int, int&, int&) const"},
    {0x10024767au, 20u, "cSC4OccupantManager::WorldCellToStandardCityCell(int, int, int&, int&) const"},
    {0x10024768eu, 44u, "cSC4OccupantManager::StandardCityCellToWorldCell(int, int, int&, int&) const"},
    {0x1002476bau, 44u, "cSC4OccupantManager::WorldCellToPosition(int, int, float&, float&) const"},
    {0x1002476e6u, 94u, "cSC4OccupantManager::PositionToWorldCell(float, float, int&, int&) const"},
    {0x100247744u, 32u, "cSC4OccupantManager::OccupantManagerCellToPosition(int, int, float&, float&) const"},
    {0x100247764u, 76u, "cSC4OccupantManager::PositionToOccupantManagerCell(float, float, int&, int&) const"},
    {0x1002477b0u, 32u, "cSC4OccupantManager::StandardCityCellToPosition(int, int, float&, float&) const"},
    {0x1002477d0u, 130u, "cSC4OccupantManager::PositionToStandardCityCell(float, float, int&, int&) const"},
    {0x100247852u, 40u, "cSC4OccupantManager::GetOccupantManagerCellBounds(int&, int&, int&, int&) const"},
    {0x10024787au, 60u, "cSC4OccupantManager::IsCellEmpty(int, int) const"},
    {0x1002478b6u, 730u, "cSC4OccupantManager::InsertOccupant(cISC4Occupant*, unsigned int)"},
    {0x100247b90u, 80u, "cSC4OccupantManager::GetOccupantListNoClip(int, int, bool)"},
    {0x100247be0u, 522u, "cSC4OccupantManager::RemoveOccupant(cISC4Occupant*, bool, unsigned int)"},
    {0x100247deau, 32u, "cSC4OccupantManager::RemoveOccupants(int, bool, unsigned int)"},
    {0x100247e0au, 826u, "cSC4OccupantManager::RemoveOccupants(cISC4OccupantFilter*, bool, unsigned int)"},
    {0x100248144u, 1166u, "cSC4OccupantManager::MoveOccupant(cISC4Occupant*, bool)"},
    {0x1002485d2u, 110u, "cSC4OccupantManager::GetOccupantList(int, int, bool)"},
    {0x100248640u, 334u, "cSC4OccupantManager::FindOccupant(cISC4Occupant*, bool, int*, int*)"},
    {0x10024878eu, 246u, "cSC4OccupantManager::GetOccupantList(cISC4Occupant*, int&, int&)"},
    {0x100248884u, 236u, "cSC4OccupantManager::GetBoundingCells(int, int, int (*) [2], int&)"},
    {0x100248970u, 38u, "cSC4OccupantManager::GetFirstOccupantByPosition(cISC4Occupant*&, float, float, unsigned int)"},
    {0x100248996u, 352u, "cSC4OccupantManager::GetFirstOccupantByPosition(cISC4Occupant*&, float, float, cISC4OccupantFilter*)"},
    {0x100248af6u, 40u, "cSC4OccupantManager::GetFirstOccupant(cISC4Occupant*&, int, int, unsigned int)"},
    {0x100248b1eu, 236u, "cSC4OccupantManager::GetFirstOccupant(cISC4Occupant*&, int, int, cISC4OccupantFilter*)"},
    {0x100248c0au, 40u, "cSC4OccupantManager::GetFirstOccupantByStandardCityCell(cISC4Occupant*&, int, int, unsigned int)"},
    {0x100248c32u, 452u, "cSC4OccupantManager::GetFirstOccupantByStandardCityCell(cISC4Occupant*&, int, int, cISC4OccupantFilter*)"},
    {0x100248df6u, 40u, "cSC4OccupantManager::GetFirstOccupantByStandardCityCells(cISC4Occupant*&, int const*, int const*, unsigned int)"},
    {0x100248e1eu, 758u, "cSC4OccupantManager::GetFirstOccupantByStandardCityCells(cISC4Occupant*&, int const*, int const*, cISC4OccupantFilter*)"},
    {0x100249114u, 60u, "cSC4OccupantManager::GetOccupantsByBBox(ilist<cISC4Occupant*>&, float const*, float const*, unsigned int, unsigned int)"},
    {0x100249150u, 1226u, "cSC4OccupantManager::GetOccupantsByBBox(ilist<cISC4Occupant*>&, float const*, float const*, cISC4OccupantFilter*, unsigned int)"},
    {0x10024961au, 60u, "cSC4OccupantManager::GetOccupantsByOccupantManagerCells(ilist<cISC4Occupant*>&, int const*, int const*, unsigned int, unsigned int)"},
    {0x100249656u, 778u, "cSC4OccupantManager::GetOccupantsByOccupantManagerCells(ilist<cISC4Occupant*>&, int const*, int const*, cISC4OccupantFilter*, unsigned int)"},
    {0x100249960u, 60u, "cSC4OccupantManager::GetOccupantsByStandardCityCells(ilist<cISC4Occupant*>&, int const*, int const*, unsigned int, unsigned int)"},
    {0x10024999cu, 390u, "cSC4OccupantManager::GetOccupantsByStandardCityCells(ilist<cISC4Occupant*>&, int const*, int const*, cISC4OccupantFilter*, unsigned int)"},
    {0x100249b22u, 60u, "cSC4OccupantManager::IterateOccupantsByBBox(bool (*)(cISC4Occupant*, void*), void*, float const*, float const*, unsigned int)"},
    {0x100249b5eu, 1436u, "cSC4OccupantManager::IterateOccupantsByBBox(bool (*)(cISC4Occupant*, void*), void*, float const*, float const*, cISC4OccupantFilter*)"},
    {0x10024a0fau, 60u, "cSC4OccupantManager::IterateOccupants(bool (*)(cISC4Occupant*, void*), void*, int const*, int const*, unsigned int)"},
    {0x10024a136u, 1074u, "cSC4OccupantManager::IterateOccupants(bool (*)(cISC4Occupant*, void*), void*, int const*, int const*, cISC4OccupantFilter*)"},
    {0x10024a568u, 60u, "cSC4OccupantManager::IterateOccupantsByStandardCityCell(bool (*)(cISC4Occupant*, void*), void*, int const*, int const*, unsigned int)"},
    {0x10024a5a4u, 388u, "cSC4OccupantManager::IterateOccupantsByStandardCityCell(bool (*)(cISC4Occupant*, void*), void*, int const*, int const*, cISC4OccupantFilter*)"},
    {0x10024a728u, 48u, "cSC4OccupantManager::ReleaseOccupantList(ilist<cISC4Occupant*>&)"},

    // cSC4TrafficNetworkMap — 80 entry points
    {0x100298a0eu, 398u, "cSC4TrafficNetworkMap::cSC4TrafficNetworkMap()"},
    {0x100298b9cu, 158u, "cSC4TrafficNetworkMap::~cSC4TrafficNetworkMap() [GZCOM boilerplate]"},
    {0x100298c3au, 10u, "cSC4TrafficNetworkMap::~cSC4TrafficNetworkMap()"},
    {0x100298c44u, 46u, "cSC4TrafficNetworkMap::~cSC4TrafficNetworkMap() [GZCOM boilerplate]"},
    {0x100298c72u, 696u, "cSC4TrafficNetworkMap::Init()"},
    {0x100298f2au, 220u, "cSC4TrafficNetworkMap::InitializeTunnelPaths()"},
    {0x100299006u, 376u, "cSC4TrafficNetworkMap::Shutdown()"},
    {0x10029917eu, 154u, "cSC4TrafficNetworkMap::ClearNetworkTileInfo()"},
    {0x100299218u, 112u, "cSC4TrafficNetworkMap::ClearIntersectionMap()"},
    {0x100299288u, 58u, "cSC4TrafficNetworkMap::ClearTrainJunctionMap()"},
    {0x1002992c2u, 80u, "cSC4TrafficNetworkMap::ClearUnusableRoadsMap()"},
    {0x100299312u, 8u, "cSC4TrafficNetworkMap::DoMessage(cGZMessage&)"},
    {0x100299322u, 1146u, "cSC4TrafficNetworkMap::DoMessage(cIGZMessage2*)"},
    {0x10029979cu, 468u, "cSC4TrafficNetworkMap::UpdateIntersectionProps(cISC4NetworkOccupant*, cISC4TrafficNetworkMap::IntersectionArray&, bool)"},
    {0x100299982u, 350u, "cSC4TrafficNetworkMap::DoAnimationTick(unsigned int, float)"},
    {0x100299ae0u, 364u, "cSC4TrafficNetworkMap::UpdateRoadNeighborInfo(cISC4NetworkOccupant*, cISC4TrafficNetworkMap::TileInfo&)"},
    {0x100299c4cu, 372u, "cSC4TrafficNetworkMap::UpdateNetworkOccupancyInfo(cISC4NetworkOccupant*, cISC4TrafficNetworkMap::TileInfo&)"},
    {0x100299dc0u, 454u, "cSC4TrafficNetworkMap::GarbageCollectPathInfo()"},
    {0x100299f86u, 512u, "cSC4TrafficNetworkMap::HandleViewChanged()"},
    {0x10029a186u, 162u, "cSC4TrafficNetworkMap::InitializeIntersection(cISC4NetworkOccupant*, std::__1::__hash_map_iterator<std::__1::__hash_iterator<std::__1::__hash_node<std::__1::__hash_value_type<cISC4NetworkOccupant*, cISC4TrafficNetworkMap::IntersectionArray>, void*>*>>)"},
    {0x10029a228u, 962u, "cSC4TrafficNetworkMap::HandleInsertNetworkOccupant(cISC4NetworkOccupant*)"},
    {0x10029a5eau, 422u, "cSC4TrafficNetworkMap::HandleNetworkOccupantUsable(cISC4NetworkOccupant*, bool)"},
    {0x10029a790u, 226u, "cSC4TrafficNetworkMap::RemoveUnusableNetworkOccupant(cISC4NetworkOccupant*, SC4Rect<int>*)"},
    {0x10029a872u, 1196u, "cSC4TrafficNetworkMap::AddIntersection(cISC4NetworkOccupant*)"},
    {0x10029ad1eu, 1252u, "cSC4TrafficNetworkMap::AddTrainJunction(cISC4NetworkOccupant*)"},
    {0x10029b202u, 280u, "cSC4TrafficNetworkMap::HandleRemoveNetworkOccupant(cISC4NetworkOccupant*)"},
    {0x10029b31au, 176u, "cSC4TrafficNetworkMap::RemoveIntersection(cISC4NetworkOccupant*)"},
    {0x10029b3cau, 74u, "cSC4TrafficNetworkMap::RemoveTrainJunction(cISC4NetworkOccupant*)"},
    {0x10029b414u, 80u, "cSC4TrafficNetworkMap::ReleaseTileInfo(cISC4TrafficNetworkMap::TileInfo*)"},
    {0x10029b464u, 452u, "cSC4TrafficNetworkMap::HandlePostCityInit()"},
    {0x10029b628u, 390u, "cSC4TrafficNetworkMap::InitIntersectionProps(cISC4NetworkOccupant*, cISC4TrafficNetworkMap::IntersectionArray&)"},
    {0x10029b7aeu, 46u, "cSC4TrafficNetworkMap::HandleTuningParametersChanged()"},
    {0x10029b7dcu, 516u, "cSC4TrafficNetworkMap::TickIntersections(float)"},
    {0x10029b9e0u, 64u, "cSC4TrafficNetworkMap::UpdateIntersectionMap()"},
    {0x10029ba20u, 138u, "cSC4TrafficNetworkMap::TickSingleIntersection(cISC4NetworkOccupant*, cISC4TrafficNetworkMap::IntersectionArray&, float)"},
    {0x10029baaau, 1964u, "cSC4TrafficNetworkMap::UpdateIntersectionTiming(cISC4NetworkOccupant*, cISC4TrafficNetworkMap::IntersectionArray&)"},
    {0x10029c256u, 74u, "cSC4TrafficNetworkMap::UpdateIntersection(cISC4NetworkOccupant*)"},
    {0x10029c2a0u, 44u, "cSC4TrafficNetworkMap::IsIntersection(cISC4NetworkOccupant*)"},
    {0x10029c2ccu, 350u, "cSC4TrafficNetworkMap::CheckIntersection(cISC4NetworkOccupant*, cSC4TrafficAutomaton*, unsigned int, float&)"},
    {0x10029c42au, 860u, "cSC4TrafficNetworkMap::InitializeIntersectionEdge(cISC4NetworkOccupant*, cISC4NetworkOccupant*, cISC4TrafficNetworkMap::IntersectionArray&, cISC4TrafficNetworkMap::IntersectionInfo&, unsigned char)"},
    {0x10029c786u, 348u, "cSC4TrafficNetworkMap::MarkTrainInCell(cSC4TrafficAutomaton*, int, int)"},
    {0x10029c8e2u, 240u, "cSC4TrafficNetworkMap::UnmarkTrainInCell(cSC4TrafficAutomaton*, int, int)"},
    {0x10029c9d2u, 76u, "cSC4TrafficNetworkMap::GetTrainJunctionPath(cISC4NetworkOccupant*, unsigned char) const"},
    {0x10029ca1eu, 146u, "cSC4TrafficNetworkMap::GetTrainJunctionKeys(cISC4NetworkOccupant*, unsigned char, std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&, unsigned short&) const"},
    {0x10029cab0u, 102u, "cSC4TrafficNetworkMap::ToggleTrainJunction(cISC4NetworkOccupant*)"},
    {0x10029cb16u, 268u, "cSC4TrafficNetworkMap::SwitchStoplightStates(cISC4PropOccupant*)"},
    {0x10029cc22u, 410u, "cSC4TrafficNetworkMap::CheckCellOccupancy(int, int, unsigned char, cISC4TrafficNetworkMap::TileInfo&)"},
    {0x10029cdbcu, 390u, "cSC4TrafficNetworkMap::WalkNeighborOccupancy(cISC4NetworkOccupant*, unsigned char, unsigned char, unsigned short&)"},
    {0x10029cf42u, 562u, "cSC4TrafficNetworkMap::ConstrainPointToNetworkTile(cS3DVector3&, unsigned int) const"},
    {0x10029d174u, 964u, "cSC4TrafficNetworkMap::ConstrainVectorToNetworkTile(cS3DVector3 const&, cS3DVector3&, unsigned int) const"},
    {0x10029d538u, 466u, "cSC4TrafficNetworkMap::GetRoadAltitude(float, float) const"},
    {0x10029d70au, 114u, "cSC4TrafficNetworkMap::ConnectionExists(int, int, unsigned char, unsigned char, cISC4PathInfo::tPathType) const"},
    {0x10029d77cu, 42u, "cSC4TrafficNetworkMap::ConnectionExists(cISC4NetworkOccupant*, unsigned char, unsigned char, cISC4PathInfo::tPathType) const"},
    {0x10029d7a6u, 36u, "cSC4TrafficNetworkMap::SafeGetTileInfoAtCell(int, int) const"},
    {0x10029d7cau, 486u, "cSC4TrafficNetworkMap::GetTileInfoAtNeighborEdge(cISC4NetworkOccupant*, unsigned char, bool, cISC4PathInfo::tPathType) const"},
    {0x10029d9b0u, 98u, "cSC4TrafficNetworkMap::GetTileInfoByNetwork(cISC4NetworkOccupant*) const"},
    {0x10029da12u, 24u, "cSC4TrafficNetworkMap::FindNetworkOccupant(int, int) const"},
    {0x10029da2au, 24u, "cSC4TrafficNetworkMap::FindNetworkOccupant(int, int, float, float) const"},
    {0x10029da42u, 148u, "cSC4TrafficNetworkMap::FindNetworkOccupant(float const*) const"},
    {0x10029dad6u, 194u, "cSC4TrafficNetworkMap::FindNetworkOccupant(int, int, unsigned int, bool) const"},
    {0x10029db98u, 56u, "cSC4TrafficNetworkMap::FindNetworkOccupant(float const*, unsigned int, bool) const"},
    {0x10029dbd0u, 132u, "cSC4TrafficNetworkMap::FindNetworkTile(int, int, float, float) const"},
    {0x10029dc54u, 316u, "cSC4TrafficNetworkMap::GetLaneTick(cISC4NetworkOccupant*, cISC4AutomataManager::PopulateLayer, cISC4PathInfo::tPathType, unsigned char, unsigned char)"},
    {0x10029de12u, 306u, "cSC4TrafficNetworkMap::GetAutosPerSecond(cISC4NetworkOccupant*, cISC4AutomataManager::PopulateLayer, cISC4PathInfo::tPathType, unsigned char, unsigned char)"},
    {0x10029df44u, 28u, "cSC4TrafficNetworkMap::GetAutomataCount(cISC4NetworkOccupant*, cISC4AutomataManager::PopulateLayer, cISC4PathInfo::tPathType, unsigned char, unsigned char)"},
    {0x10029df60u, 290u, "cSC4TrafficNetworkMap::GetAutomataCount(cISC4NetworkOccupant*, cISC4AutomataManager::PopulateLayer, unsigned int)"},
    {0x10029e082u, 130u, "cSC4TrafficNetworkMap::GetAutomataCountForLane(cISC4NetworkOccupant*, unsigned int)"},
    {0x10029e104u, 164u, "cSC4TrafficNetworkMap::GetAutomataCount(cISC4NetworkOccupant*, cISC4PathInfo::tPathType)"},
    {0x10029e1a8u, 164u, "cSC4TrafficNetworkMap::GetAutosPerSecond(cISC4NetworkOccupant*, cISC4PathInfo::tPathType)"},
    {0x10029e24cu, 450u, "cSC4TrafficNetworkMap::ChooseWeightedEdge(float const*, unsigned char*)"},
    {0x10029e40eu, 630u, "cSC4TrafficNetworkMap::GetSurroundingRoads(std::__1::vector<cISC4NetworkOccupant*, std::__1::allocator<cISC4NetworkOccupant*>>&, SC4Rect<int> const&, int, int, unsigned int)"},
    {0x10029e684u, 198u, "cSC4TrafficNetworkMap::GetSurroundingRoads(std::__1::vector<cISC4NetworkOccupant*, std::__1::allocator<cISC4NetworkOccupant*>>&, cISC4Occupant*, int, int, unsigned int)"},
    {0x10029e74au, 280u, "cSC4TrafficNetworkMap::EnterNetwork(cSC4TrafficAutomaton*, cISC4NetworkOccupant*)"},
    {0x10029e862u, 272u, "cSC4TrafficNetworkMap::ExitNetwork(cSC4TrafficAutomaton*, cISC4NetworkOccupant*, unsigned int)"},
    {0x1002a034cu, 74u, "cSC4TrafficNetworkMap::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x1002a03c0u, 14u, "cSC4TrafficNetworkMap::AddRef()"},
    {0x1002a03f8u, 68u, "cSC4TrafficNetworkMap::Release() [GZCOM boilerplate]"},
    {0x1002a04eau, 24u, "cSC4TrafficNetworkMap::GetTileInfoAtCell(int, int) const"},
    {0x1002a0502u, 14u, "cSC4TrafficNetworkMap::GetVisibleCellRect() const"},
    {0x1002a0510u, 10u, "cSC4TrafficNetworkMap::GetIntersectionMap()"},

    // cSC4NetworkOccupant — 105 entry points
    {0x10019bc06u, 16u, "cSC4NetworkOccupant::operator new(unsigned long)"},
    {0x10019bc16u, 526u, "cSC4NetworkOccupant::cSC4NetworkOccupant()"},
    {0x10019be24u, 39u, "cSC4NetworkOccupant::operator delete(void*)"},
    {0x10019be56u, 508u, "cSC4NetworkOccupant::cSC4NetworkOccupant()"},
    {0x10019c052u, 308u, "cSC4NetworkOccupant::~cSC4NetworkOccupant() [GZCOM boilerplate]"},
    {0x10019c186u, 18u, "cSC4NetworkOccupant::~cSC4NetworkOccupant()"},
    {0x10019c1dau, 68u, "cSC4NetworkOccupant::~cSC4NetworkOccupant() [GZCOM boilerplate]"},
    {0x10019c248u, 194u, "cSC4NetworkOccupant::Init()"},
    {0x10019c30au, 704u, "cSC4NetworkOccupant::StaticInit()"},
    {0x10019c5d8u, 186u, "cSC4NetworkOccupant::Init(float const (*) [3])"},
    {0x10019c692u, 14u, "cSC4NetworkOccupant::Shutdown()"},
    {0x10019c6aau, 132u, "cSC4NetworkOccupant::StaticShutdown(void*)"},
    {0x10019c72eu, 248u, "cSC4NetworkOccupant::SetNetworkFlag(unsigned int)"},
    {0x10019c826u, 212u, "cSC4NetworkOccupant::ClearNetworkFlag(unsigned int)"},
    {0x10019c8fau, 76u, "cSC4NetworkOccupant::GetConnectionNodeEdgeTypes(unsigned int&, unsigned int&)"},
    {0x10019c946u, 190u, "cSC4NetworkOccupant::SetConnectionNodeEdgeTypes(unsigned int, unsigned int)"},
    {0x10019ca04u, 54u, "cSC4NetworkOccupant::GetConnectionPathIDOverride()"},
    {0x10019ca3au, 194u, "cSC4NetworkOccupant::SetConnectionPathIDOverride(unsigned int)"},
    {0x10019cafcu, 28u, "cSC4NetworkOccupant::CheckForRetrofit()"},
    {0x10019cb18u, 154u, "cSC4NetworkOccupant::SetRetrofitFlag(bool)"},
    {0x10019d70au, 14u, "cSC4NetworkOccupant::AddRef()"},
    {0x10019d760u, 14u, "cSC4NetworkOccupant::Release()"},
    {0x10019d7b6u, 112u, "cSC4NetworkOccupant::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x10019d86eu, 192u, "cSC4NetworkOccupant::AddPart(float const (*) [3], float const (*) [2], unsigned int const*, unsigned int)"},
    {0x10019d92eu, 72u, "cSC4NetworkOccupant::ChangeZoomLevel(SC4DrawContext*)"},
    {0x10019d976u, 1324u, "cSC4NetworkOccupant::GetTexture(unsigned int, unsigned int)"},
    {0x10019dea2u, 420u, "cSC4NetworkOccupant::ComputeBoundingBox(float*, float*)"},
    {0x10019e046u, 84u, "cSC4NetworkOccupant::ConnectsToNeighbor(int, cISC4NetworkManager::eNetworkType) const"},
    {0x10019e09au, 1394u, "cSC4NetworkOccupant::Draw(SC4DrawContext*, bool)"},
    {0x10019e624u, 642u, "cSC4NetworkOccupant::DrawOneWayArrow(SC4DrawContext*, bool, cSC4PackedRenderState*, unsigned char)"},
    {0x10019e8a6u, 258u, "cSC4NetworkOccupant::SetTextureCoordinates(unsigned char)"},
    {0x10019e9a8u, 70u, "cSC4NetworkOccupant::GetActiveEdges()"},
    {0x10019e9eeu, 90u, "cSC4NetworkOccupant::GetActiveEdgesForType(cISC4NetworkManager::eNetworkType)"},
    {0x10019ea48u, 88u, "cSC4NetworkOccupant::GetActiveEdgesForTypeNoGroup(cISC4NetworkManager::eNetworkType)"},
    {0x10019eaa0u, 60u, "cSC4NetworkOccupant::GetGroupedEdges()"},
    {0x10019eadcu, 58u, "cSC4NetworkOccupant::GetBoundingBox(float*, float*)"},
    {0x10019eb50u, 64u, "cSC4NetworkOccupant::GetEdgeTypesFlag(cISC4NetworkManager::eNetworkType) const"},
    {0x10019eb90u, 172u, "cSC4NetworkOccupant::GetModelInstances(int, int, cIS3DModelInstance**, int, int&)"},
    {0x10019ec3cu, 72u, "cSC4NetworkOccupant::GetOccupiedCell(unsigned int&, unsigned int&)"},
    {0x10019ec84u, 34u, "cSC4NetworkOccupant::GetOwner()"},
    {0x10019ecbau, 126u, "cSC4NetworkOccupant::GetPosition(float*) const"},
    {0x10019ed38u, 82u, "cSC4NetworkOccupant::GetSortValue(unsigned int&) const"},
    {0x10019eddcu, 136u, "cSC4NetworkOccupant::GetTransform(cS3DTransform&)"},
    {0x10019ee72u, 140u, "cSC4NetworkOccupant::SetTransform(cS3DTransform const&)"},
    {0x10019ef0cu, 22u, "cSC4NetworkOccupant::GetVertexHeight(int) const"},
    {0x10019ef22u, 34u, "cSC4NetworkOccupant::GetVertexHeights(float*) const"},
    {0x10019ef44u, 26u, "cSC4NetworkOccupant::GetWorldBounds(cS3DBoundingBox&)"},
    {0x10019ef78u, 448u, "cSC4NetworkOccupant::GetBounds(cS3DBoundingBox&)"},
    {0x10019f146u, 8u, "cSC4NetworkOccupant::HasViewDependentModel() const"},
    {0x10019f14eu, 26u, "cSC4NetworkOccupant::HasProperty(unsigned int) const"},
    {0x10019f17eu, 26u, "cSC4NetworkOccupant::IsOfType(cISC4NetworkManager::eNetworkType) const"},
    {0x10019f198u, 20u, "cSC4NetworkOccupant::MarkLightingUpdateNeeded()"},
    {0x10019f1acu, 182u, "cSC4NetworkOccupant::Pick(cSC4PickContext*)"},
    {0x10019f270u, 104u, "cSC4NetworkOccupant::SetEdgeType(cISC4NetworkManager::eNetworkType, int, unsigned char)"},
    {0x10019f41cu, 18u, "cSC4NetworkOccupant::SetEdgeTypesFlag(cISC4NetworkManager::eNetworkType, unsigned int)"},
    {0x10019f42eu, 6u, "cSC4NetworkOccupant::SetPosition(float const*)"},
    {0x10019f43au, 152u, "cSC4NetworkOccupant::SetQuadDraw(unsigned int, unsigned char)"},
    {0x10019f4d2u, 34u, "cSC4NetworkOccupant::SetVertexHeights(float const*)"},
    {0x10019f4f4u, 558u, "cSC4NetworkOccupant::GetPathInfo()"},
    {0x10019f722u, 46u, "cSC4NetworkOccupant::ReleasePathInfo()"},
    {0x10019f750u, 224u, "cSC4NetworkOccupant::PathConnectionExists(cISC4PathInfo::tPathType, unsigned char, unsigned char)"},
    {0x10019f830u, 72u, "cSC4NetworkOccupant::AddPathConnection(cISC4PathInfo::tPathType, unsigned char, unsigned char)"},
    {0x10019f878u, 12u, "cSC4NetworkOccupant::SetPathIDOverride(unsigned int)"},
    {0x10019f884u, 12u, "cSC4NetworkOccupant::GetPathIDOverride()"},
    {0x10019f890u, 112u, "cSC4NetworkOccupant::GetAltitude(float, float) const"},
    {0x10019f900u, 120u, "cSC4NetworkOccupant::GetNormal(float, float) const"},
    {0x10019f978u, 500u, "cSC4NetworkOccupant::UpdateLighting(cISC4LightingManager*)"},
    {0x10019fb6cu, 170u, "cSC4NetworkOccupant::SetTranslucency(bool, unsigned char)"},
    {0x10019fc16u, 18u, "cSC4NetworkOccupant::SetHighlight(int, bool)"},
    {0x10019fc36u, 220u, "cSC4NetworkOccupant::SetVariation(unsigned char)"},
    {0x10019fd12u, 14u, "cSC4NetworkOccupant::GetVariation() const"},
    {0x10019fd20u, 220u, "cSC4NetworkOccupant::SetUnderTexture(unsigned int)"},
    {0x10019fdfcu, 18u, "cSC4NetworkOccupant::IsLit() const"},
    {0x10019fe0eu, 222u, "cSC4NetworkOccupant::SetLit(bool)"},
    {0x10019feecu, 104u, "cSC4NetworkOccupant::IsPartOfTheSameStructure(cISC4NetworkOccupant*)"},
    {0x10019ff54u, 68u, "cSC4NetworkOccupant::GetPassageCeiling(float&) const"},
    {0x10019ff98u, 136u, "cSC4NetworkOccupant::SetPassageCeiling(float)"},
    {0x1001a0020u, 58u, "cSC4NetworkOccupant::GetOneWayDirections() const"},
    {0x1001a005au, 238u, "cSC4NetworkOccupant::SetOneWayDirections(unsigned char)"},
    {0x1001a0148u, 80u, "cSC4NetworkOccupant::GetComplexPieceOrigin(int&, int&)"},
    {0x1001a0198u, 232u, "cSC4NetworkOccupant::SetComplexPieceOrigin(int, int)"},
    {0x1001a0280u, 18u, "cSC4NetworkOccupant::IsSupportPylonPresent() const"},
    {0x1001a0292u, 84u, "cSC4NetworkOccupant::IsIntersection() const"},
    {0x1001a03a8u, 32u, "cSC4NetworkOccupant::SetUsable(bool)"},
    {0x1001a03c8u, 32u, "cSC4NetworkOccupant::SetEdgeStore(cSC4EdgeConnectionStore const&)"},
    {0x1001a03e8u, 16u, "cSC4NetworkOccupant::AsOccupant()"},
    {0x1001a03f8u, 20u, "cSC4NetworkOccupant::SetCostOfDemolition(long long)"},
    {0x1001a0418u, 16u, "cSC4NetworkOccupant::SetRotationAndFlip(unsigned char)"},
    {0x1001a296cu, 20u, "cSC4NetworkOccupant::HasNetworkFlag(unsigned int) const"},
    {0x1001a2980u, 16u, "cSC4NetworkOccupant::HasAnyNetworkFlag(unsigned int) const"},
    {0x1001a2990u, 12u, "cSC4NetworkOccupant::GetNetworkFlag()"},
    {0x1001a299cu, 20u, "cSC4NetworkOccupant::IsImmovable() const"},
    {0x1001a29b0u, 32u, "cSC4NetworkOccupant::SetImmovable(bool)"},
    {0x1001a29d0u, 36u, "cSC4NetworkOccupant::IsPlaced()"},
    {0x1001a29f4u, 16u, "cSC4NetworkOccupant::GetRotation() const"},
    {0x1001a2a04u, 16u, "cSC4NetworkOccupant::GetFlip() const"},
    {0x1001a2a14u, 16u, "cSC4NetworkOccupant::GetRotationAndFlip() const"},
    {0x1001a2a24u, 24u, "cSC4NetworkOccupant::GetEdgeStore() const"},
    {0x1001a2a3cu, 8u, "cSC4NetworkOccupant::CanBeLit() const"},
    {0x1001a2a44u, 28u, "cSC4NetworkOccupant::GetCostOfDemolition() const"},
    {0x1001ae690u, 24u, "cSC4NetworkOccupant::PieceId() const"},
    {0x1001ae6a8u, 24u, "cSC4NetworkOccupant::GetPosition(float*)"},
    {0x1001d2ac8u, 28u, "cSC4NetworkOccupant::IsUsable() const"},
    {0x1001d2b08u, 8u, "cSC4NetworkOccupant::IsManagedGlobally() const"},
    {0x1001d2b10u, 18u, "cSC4NetworkOccupant::TransformNode(cS3DVector3&)"},

    // cSC4NetworkOccupantWithPreBuiltModel — 32 entry points
    {0x1001a2964u, 8u, "cSC4NetworkOccupantWithPreBuiltModel::IsManagedGlobally() const"},
    {0x1001a2a60u, 32u, "cSC4NetworkOccupantWithPreBuiltModel::~cSC4NetworkOccupantWithPreBuiltModel()"},
    {0x1001a2a8cu, 20u, "cSC4NetworkOccupantWithPreBuiltModel::UpdateBaseModel(cIS3DModelInstance*)"},
    {0x1001b9fc6u, 16u, "cSC4NetworkOccupantWithPreBuiltModel::operator new(unsigned long)"},
    {0x1001b9fd6u, 228u, "cSC4NetworkOccupantWithPreBuiltModel::cSC4NetworkOccupantWithPreBuiltModel()"},
    {0x1001ba0bau, 33u, "cSC4NetworkOccupantWithPreBuiltModel::operator delete(void*)"},
    {0x1001ba0e6u, 124u, "cSC4NetworkOccupantWithPreBuiltModel::Init()"},
    {0x1001ba162u, 302u, "cSC4NetworkOccupantWithPreBuiltModel::RecomputeShearTransform()"},
    {0x1001ba29eu, 106u, "cSC4NetworkOccupantWithPreBuiltModel::Shutdown()"},
    {0x1001ba316u, 100u, "cSC4NetworkOccupantWithPreBuiltModel::Draw(SC4DrawContext*, bool)"},
    {0x1001ba37au, 1274u, "cSC4NetworkOccupantWithPreBuiltModel::UpdateShadow(bool)"},
    {0x1001ba886u, 330u, "cSC4NetworkOccupantWithPreBuiltModel::AddModel(cISCResExemplar*, float const*, unsigned char)"},
    {0x1001ba9d0u, 1378u, "cSC4NetworkOccupantWithPreBuiltModel::ComputeBoundingBox(float*, float*)"},
    {0x1001baf32u, 210u, "cSC4NetworkOccupantWithPreBuiltModel::cSC4NetworkOccupantWithPreBuiltModel()"},
    {0x1001bb004u, 46u, "cSC4NetworkOccupantWithPreBuiltModel::AddSupportModel(unsigned int, float, cS3DVector3)"},
    {0x1001bb032u, 2968u, "cSC4NetworkOccupantWithPreBuiltModel::GetModelInstances(int, int, cIS3DModelInstance**, int, int&)"},
    {0x1001bbbcau, 562u, "cSC4NetworkOccupantWithPreBuiltModel::GetPathInfo()"},
    {0x1001bbdfcu, 346u, "cSC4NetworkOccupantWithPreBuiltModel::SetTranslucency(bool, unsigned char)"},
    {0x1001bbf56u, 8u, "cSC4NetworkOccupantWithPreBuiltModel::HasViewDependentModel() const"},
    {0x1001bbf5eu, 326u, "cSC4NetworkOccupantWithPreBuiltModel::TransformNode(cS3DVector3&)"},
    {0x1001bc0a4u, 212u, "cSC4NetworkOccupantWithPreBuiltModel::GetAltitude(float, float) const"},
    {0x1001bc178u, 230u, "cSC4NetworkOccupantWithPreBuiltModel::GetNormal(float, float) const"},
    {0x1001bc25eu, 42u, "cSC4NetworkOccupantWithPreBuiltModel::SetVisibility(bool, bool)"},
    {0x1001bc2aeu, 34u, "cSC4NetworkOccupantWithPreBuiltModel::SetAllFlags(unsigned int)"},
    {0x1001bc2f0u, 34u, "cSC4NetworkOccupantWithPreBuiltModel::UpdateLighting(cISC4LightingManager*)"},
    {0x1001bc312u, 38u, "cSC4NetworkOccupantWithPreBuiltModel::SetLit(bool)"},
    {0x1001bc338u, 24u, "cSC4NetworkOccupantWithPreBuiltModel::IsSupportPylonPresent() const"},
    {0x1001bc350u, 30u, "cSC4NetworkOccupantWithPreBuiltModel::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x1001bc41cu, 10u, "cSC4NetworkOccupantWithPreBuiltModel::AddRef()"},
    {0x1001bc46eu, 10u, "cSC4NetworkOccupantWithPreBuiltModel::Release() [GZCOM boilerplate]"},
    {0x1001bc78cu, 74u, "cSC4NetworkOccupantWithPreBuiltModel::~cSC4NetworkOccupantWithPreBuiltModel() [GZCOM boilerplate]"},
    {0x1001d26d0u, 222u, "cSC4NetworkOccupantWithPreBuiltModel::~cSC4NetworkOccupantWithPreBuiltModel() [GZCOM boilerplate]"},

    // cSC4UndergroundOccupant — 52 entry points
    {0x1001a1462u, 62u, "cSC4UndergroundOccupant::IsPlaced()"},
    {0x1001aa4b0u, 364u, "cSC4UndergroundOccupant::cSC4UndergroundOccupant()"},
    {0x1001aa61cu, 258u, "cSC4UndergroundOccupant::~cSC4UndergroundOccupant() [GZCOM boilerplate]"},
    {0x1001aa71eu, 6u, "cSC4UndergroundOccupant::~cSC4UndergroundOccupant() [GZCOM boilerplate]"},
    {0x1001aa736u, 6u, "cSC4UndergroundOccupant::~cSC4UndergroundOccupant() [GZCOM boilerplate]"},
    {0x1001aa750u, 74u, "cSC4UndergroundOccupant::Init()"},
    {0x1001aa79au, 50u, "cSC4UndergroundOccupant::StaticInit()"},
    {0x1001aa7deu, 998u, "cSC4UndergroundOccupant::Init(float const (*) [3], unsigned int, cSC4UndergroundOccupant::tCustomGeometryInfo*)"},
    {0x1001aabc4u, 316u, "cSC4UndergroundOccupant::Shutdown()"},
    {0x1001aad0eu, 292u, "cSC4UndergroundOccupant::SetupPackedRenderState()"},
    {0x1001ab3e2u, 6u, "cSC4UndergroundOccupant::AddModel(cISCResExemplar*, float const*, unsigned char)"},
    {0x1001ab3e8u, 148u, "cSC4UndergroundOccupant::ChangeZoomLevel(SC4DrawContext*)"},
    {0x1001ab47cu, 1264u, "cSC4UndergroundOccupant::Draw(SC4DrawContext*, bool)"},
    {0x1001ab97eu, 190u, "cSC4UndergroundOccupant::GetBoundingBox(float*, float*)"},
    {0x1001aba4au, 32u, "cSC4UndergroundOccupant::GetModelInstances(int, int, cIS3DModelInstance**, int, int&)"},
    {0x1001aba6au, 30u, "cSC4UndergroundOccupant::SetTuningParams(float, bool, float)"},
    {0x1001aba88u, 70u, "cSC4UndergroundOccupant::SetTerrainHeights(float*)"},
    {0x1001abaceu, 530u, "cSC4UndergroundOccupant::ComputeModelYPosition()"},
    {0x1001abce0u, 28u, "cSC4UndergroundOccupant::GetTerrainHeights(float*)"},
    {0x1001abcfcu, 190u, "cSC4UndergroundOccupant::ComputeBoundingBox(float*, float*)"},
    {0x1001abdbau, 112u, "cSC4UndergroundOccupant::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x1001abe72u, 62u, "cSC4UndergroundOccupant::GetSortValue(unsigned int&) const"},
    {0x1001abee4u, 342u, "cSC4UndergroundOccupant::PostOccupantMessage(unsigned int, unsigned int)"},
    {0x1001ac048u, 506u, "cSC4UndergroundOccupant::AddWallQuad(unsigned int, unsigned int, std::__1::vector<sVF_V3F_C4UB_T2F, std::__1::allocator<sVF_V3F_C4UB_T2F>>&)"},
    {0x1001ac242u, 218u, "cSC4UndergroundOccupant::GetHookupData(float&, bool&, int)"},
    {0x1001ac31cu, 1180u, "cSC4UndergroundOccupant::MakeCustomUndergroundPiece(bool, bool, std::__1::vector<sVF_V3F_C4UB_T2F, std::__1::allocator<sVF_V3F_C4UB_T2F>>&, sVF_V3F_C4UB_T2F*, int, int*, int, int, int*, int, int, float*)"},
    {0x1001ac7b8u, 22u, "cSC4UndergroundOccupant::SetQuadDraw(unsigned int, unsigned char)"},
    {0x1001ac7ceu, 274u, "cSC4UndergroundOccupant::GetZipperCandidateCount(cSC4UndergroundOccupant*, int, int)"},
    {0x1001ac8e0u, 512u, "cSC4UndergroundOccupant::GetZipperPieceType(bool&)"},
    {0x1001acae0u, 3420u, "cSC4UndergroundOccupant::CreateUndergroundModel(int, bool, bool)"},
    {0x1001ad83cu, 452u, "cSC4UndergroundOccupant::SetQuadFlipAndTriHide()"},
    {0x1001ada00u, 122u, "cSC4UndergroundOccupant::IsImaginaryConnection(int)"},
    {0x1001ada7au, 468u, "cSC4UndergroundOccupant::CreateWalls(bool)"},
    {0x1001adc4eu, 204u, "cSC4UndergroundOccupant::CreateFloor(bool)"},
    {0x1001add1au, 6u, "cSC4UndergroundOccupant::CreateHighlight(bool)"},
    {0x1001add20u, 1138u, "cSC4UndergroundOccupant::CreateTransformationMatrix()"},
    {0x1001ae192u, 132u, "cSC4UndergroundOccupant::CreateGeometry(bool)"},
    {0x1001ae216u, 84u, "cSC4UndergroundOccupant::SetVertexHeights(float*)"},
    {0x1001ae26au, 34u, "cSC4UndergroundOccupant::GetVertexHeights(float*) const"},
    {0x1001ae28cu, 30u, "cSC4UndergroundOccupant::GetHiddenTriangles(bool&, bool&, bool&)"},
    {0x1001ae2aau, 128u, "cSC4UndergroundOccupant::GetPosition(float*)"},
    {0x1001ae338u, 58u, "cSC4UndergroundOccupant::GetQueryData(unsigned int&, int&, float*, float&)"},
    {0x1001ae372u, 6u, "cSC4UndergroundOccupant::SetHighlight(int, bool)"},
    {0x1001ae37eu, 320u, "cSC4UndergroundOccupant::ModelStateChanged(bool)"},
    {0x1001ae4beu, 10u, "cSC4UndergroundOccupant::AddRef()"},
    {0x1001ae510u, 10u, "cSC4UndergroundOccupant::Release()"},
    {0x1001ae562u, 30u, "cSC4UndergroundOccupant::GetDrawColor()"},
    {0x1001ae580u, 120u, "cSC4UndergroundOccupant::GetCurrentTextureBindings(cS3DTextureBinding*&, cS3DTextureBinding*&)"},
    {0x1001ae5f8u, 74u, "cSC4UndergroundOccupant::SetNetworkFlag(unsigned int)"},
    {0x1001ae642u, 78u, "cSC4UndergroundOccupant::ClearNetworkFlag(unsigned int)"},
    {0x1001ae6c0u, 8u, "cSC4UndergroundOccupant::HasViewDependentModel() const"},
    {0x1001ae6c8u, 24u, "cSC4UndergroundOccupant::GetGZIID() const"},

    // cSC4UndergroundConnectionOccupant — 40 entry points
    {0x1002ef17au, 626u, "cSC4UndergroundConnectionOccupant::cSC4UndergroundConnectionOccupant()"},
    {0x1002ef3ecu, 284u, "cSC4UndergroundConnectionOccupant::~cSC4UndergroundConnectionOccupant() [GZCOM boilerplate]"},
    {0x1002ef508u, 6u, "cSC4UndergroundConnectionOccupant::~cSC4UndergroundConnectionOccupant() [GZCOM boilerplate]"},
    {0x1002ef51au, 6u, "cSC4UndergroundConnectionOccupant::~cSC4UndergroundConnectionOccupant() [GZCOM boilerplate]"},
    {0x1002ef53au, 140u, "cSC4UndergroundConnectionOccupant::Init()"},
    {0x1002ef5c6u, 80u, "cSC4UndergroundConnectionOccupant::StaticInit()"},
    {0x1002ef616u, 282u, "cSC4UndergroundConnectionOccupant::SetupPackedRenderState()"},
    {0x1002ef730u, 1510u, "cSC4UndergroundConnectionOccupant::Init(unsigned int, cSC4UndergroundConnectionOccupant::tCustomGeometryInfo*)"},
    {0x1002efd16u, 396u, "cSC4UndergroundConnectionOccupant::Shutdown()"},
    {0x1002efea2u, 38u, "cSC4UndergroundConnectionOccupant::StaticShutdown(void*)"},
    {0x1002f0482u, 132u, "cSC4UndergroundConnectionOccupant::ChangeZoomLevel(SC4DrawContext*)"},
    {0x1002f0506u, 1786u, "cSC4UndergroundConnectionOccupant::Draw(SC4DrawContext*, bool)"},
    {0x1002f0c00u, 1346u, "cSC4UndergroundConnectionOccupant::CreateGeometry()"},
    {0x1002f1154u, 184u, "cSC4UndergroundConnectionOccupant::GetBoundingBox(float*, float*)"},
    {0x1002f120cu, 36u, "cSC4UndergroundConnectionOccupant::GetModelInstances(int, int, cIS3DModelInstance**, int, int&)"},
    {0x1002f1230u, 184u, "cSC4UndergroundConnectionOccupant::ComputeBoundingBox(float*, float*)"},
    {0x1002f12e8u, 110u, "cSC4UndergroundConnectionOccupant::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x1002f139eu, 24u, "cSC4UndergroundConnectionOccupant::GetSortValue(unsigned int&) const"},
    {0x1002f13c2u, 500u, "cSC4UndergroundConnectionOccupant::AddWallQuad(unsigned int, unsigned int, std::__1::vector<sVF_V3F_C4UB_T2F, std::__1::allocator<sVF_V3F_C4UB_T2F>>&)"},
    {0x1002f15b6u, 256u, "cSC4UndergroundConnectionOccupant::FindBuildingOccupant(int, int, cISC4Occupant*&)"},
    {0x1002f16b6u, 1650u, "cSC4UndergroundConnectionOccupant::CreatePieceModel()"},
    {0x1002f1d28u, 98u, "cSC4UndergroundConnectionOccupant::ModelStateChanged()"},
    {0x1002f1d8au, 234u, "cSC4UndergroundConnectionOccupant::CreateWalls()"},
    {0x1002f1e74u, 202u, "cSC4UndergroundConnectionOccupant::CreateFloor()"},
    {0x1002f1f3eu, 224u, "cSC4UndergroundConnectionOccupant::CreateHighlight()"},
    {0x1002f201eu, 16u, "cSC4UndergroundConnectionOccupant::GetHiddenTriangles(bool&, bool&, bool&)"},
    {0x1002f202eu, 336u, "cSC4UndergroundConnectionOccupant::PostOccupantMessage(unsigned int, unsigned int)"},
    {0x1002f217eu, 32u, "cSC4UndergroundConnectionOccupant::GetPosition(float*)"},
    {0x1002f219eu, 32u, "cSC4UndergroundConnectionOccupant::SetPosition(float const*)"},
    {0x1002f21beu, 62u, "cSC4UndergroundConnectionOccupant::SetHighlight(int, bool)"},
    {0x1002f21fcu, 10u, "cSC4UndergroundConnectionOccupant::AddRef()"},
    {0x1002f224eu, 10u, "cSC4UndergroundConnectionOccupant::Release()"},
    {0x1002f22a0u, 444u, "cSC4UndergroundConnectionOccupant::GetBounds(cS3DBoundingBox&)"},
    {0x1002f246au, 20u, "cSC4UndergroundConnectionOccupant::GetWorldBounds(cS3DBoundingBox&)"},
    {0x1002f2496u, 136u, "cSC4UndergroundConnectionOccupant::GetTransform(cS3DTransform&)"},
    {0x1002f252cu, 140u, "cSC4UndergroundConnectionOccupant::SetTransform(cS3DTransform const&)"},
    {0x1002f25c6u, 36u, "cSC4UndergroundConnectionOccupant::UpdateNetworkOccupant()"},
    {0x1002f25eau, 590u, "cSC4UndergroundConnectionOccupant::DoMessage(cIGZMessage2*)"},
    {0x1002f284au, 102u, "cSC4UndergroundConnectionOccupant::SetBuildingOccupant(cISC4Occupant*)"},
    {0x1002f28bcu, 8u, "cSC4UndergroundConnectionOccupant::HasViewDependentModel() const"},

    // cSC4PowerPoleOccupant — 60 entry points
    {0x10031f406u, 164u, "cSC4PowerPoleOccupant::UpdateLineOccupants(cSC4PowerPoleOccupant::tConnection&, std::__1::vector<cSC4PowerLineOccupant::tLineInfo, std::__1::allocator<cSC4PowerLineOccupant::tLineInfo>>*)"},
    {0x10031f4aau, 1536u, "cSC4PowerPoleOccupant::AddConnection(cSC4PowerPoleOccupant*, std::__1::vector<cSC4PowerLineOccupant::tLineInfo, std::__1::allocator<cSC4PowerLineOccupant::tLineInfo>>*)"},
    {0x10031faaau, 122u, "cSC4PowerPoleOccupant::GetLineConnectionPoints(unsigned char, cS3DVector3 (&) [4])"},
    {0x10031fb24u, 356u, "cSC4PowerPoleOccupant::GetControlPoints(cS3DVector3, cS3DVector3, cS3DVector3&, cS3DVector3&, int&)"},
    {0x10031fe0cu, 1124u, "cSC4PowerPoleOccupant::UpdateConnection(cSC4PowerPoleOccupant*)"},
    {0x100320270u, 208u, "cSC4PowerPoleOccupant::RemoveConnection(cSC4PowerPoleOccupant::tConnection&, bool)"},
    {0x100320340u, 356u, "cSC4PowerPoleOccupant::RemoveConnection(cSC4PowerPoleOccupant*, bool)"},
    {0x1003204a4u, 160u, "cSC4PowerPoleOccupant::DrawConnection(cSC4PowerPoleOccupant*, bool)"},
    {0x100320544u, 88u, "cSC4PowerPoleOccupant::BreakAllConnectionsAtDestination(bool)"},
    {0x10032059cu, 28u, "cSC4PowerPoleOccupant::GetConnectionOccupant(int)"},
    {0x1003205b8u, 230u, "cSC4PowerPoleOccupant::ChangeZoomLevel(SC4DrawContext*)"},
    {0x10032069eu, 568u, "cSC4PowerPoleOccupant::LoadModel()"},
    {0x1003208d6u, 786u, "cSC4PowerPoleOccupant::CreateFloor()"},
    {0x100320be8u, 193u, "cSC4PowerPoleOccupant::CreateWalls()"},
    {0x100320ce6u, 16u, "cSC4PowerPoleOccupant::operator new(unsigned long)"},
    {0x100320cf6u, 480u, "cSC4PowerPoleOccupant::cSC4PowerPoleOccupant()"},
    {0x100320ed6u, 17u, "cSC4PowerPoleOccupant::operator delete(void*)"},
    {0x100320efau, 496u, "cSC4PowerPoleOccupant::~cSC4PowerPoleOccupant() [GZCOM boilerplate]"},
    {0x1003210eau, 18u, "cSC4PowerPoleOccupant::~cSC4PowerPoleOccupant()"},
    {0x10032113eu, 68u, "cSC4PowerPoleOccupant::~cSC4PowerPoleOccupant() [GZCOM boilerplate]"},
    {0x1003211acu, 1222u, "cSC4PowerPoleOccupant::Draw(SC4DrawContext*, bool)"},
    {0x100321672u, 1150u, "cSC4PowerPoleOccupant::DrawPowerlines(SC4DrawContext*)"},
    {0x100321b08u, 352u, "cSC4PowerPoleOccupant::GetBoundingBox(float*, float*)"},
    {0x100321c76u, 112u, "cSC4PowerPoleOccupant::InitConnectionPoints()"},
    {0x100321ce6u, 348u, "cSC4PowerPoleOccupant::SetModelPosition()"},
    {0x100321e42u, 158u, "cSC4PowerPoleOccupant::SetDefaultExemplar(cISCResExemplar*)"},
    {0x100321efcu, 172u, "cSC4PowerPoleOccupant::GetModelInstances(int, int, cIS3DModelInstance**, int, int&)"},
    {0x100321fa8u, 30u, "cSC4PowerPoleOccupant::GetNumberOfConnections()"},
    {0x100321fc6u, 34u, "cSC4PowerPoleOccupant::GetOwner()"},
    {0x100321ffcu, 32u, "cSC4PowerPoleOccupant::GetPosition(float*)"},
    {0x10032203cu, 76u, "cSC4PowerPoleOccupant::GetWireCells(cSC4PowerPoleOccupant*, std::__1::vector<SC4Point<unsigned int>, std::__1::allocator<SC4Point<unsigned int>>>&)"},
    {0x100322088u, 480u, "cSC4PowerPoleOccupant::GetWireCells(int, std::__1::vector<SC4Point<unsigned int>, std::__1::allocator<SC4Point<unsigned int>>>&)"},
    {0x100322268u, 32u, "cSC4PowerPoleOccupant::GetBounds(cS3DBoundingBox&)"},
    {0x1003222a4u, 26u, "cSC4PowerPoleOccupant::GetWorldBounds(cS3DBoundingBox&)"},
    {0x1003222d8u, 136u, "cSC4PowerPoleOccupant::GetTransform(cS3DTransform&)"},
    {0x10032236eu, 140u, "cSC4PowerPoleOccupant::SetTransform(cS3DTransform const&)"},
    {0x100322408u, 530u, "cSC4PowerPoleOccupant::AddWallQuad(unsigned int, unsigned int, std::__1::vector<sVF_V3F_C4UB_T2F, std::__1::allocator<sVF_V3F_C4UB_T2F>>&)"},
    {0x10032261au, 154u, "cSC4PowerPoleOccupant::Init()"},
    {0x1003226c6u, 228u, "cSC4PowerPoleOccupant::Init(cISCResExemplar*, int)"},
    {0x1003227aau, 592u, "cSC4PowerPoleOccupant::StaticInit()"},
    {0x1003229fau, 234u, "cSC4PowerPoleOccupant::Shutdown()"},
    {0x10032392au, 184u, "cSC4PowerPoleOccupant::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x100323a38u, 14u, "cSC4PowerPoleOccupant::AddRef()"},
    {0x100323a9cu, 14u, "cSC4PowerPoleOccupant::Release()"},
    {0x100323b00u, 36u, "cSC4PowerPoleOccupant::SetHighlight(int, bool)"},
    {0x100323b48u, 24u, "cSC4PowerPoleOccupant::GetSortValue(unsigned int&) const"},
    {0x100323b6cu, 366u, "cSC4PowerPoleOccupant::SetPosition(float const*)"},
    {0x100323ce8u, 128u, "cSC4PowerPoleOccupant::InsertAllPowerLineOccupants()"},
    {0x100323d68u, 86u, "cSC4PowerPoleOccupant::DoOccupantInserted(cISC4Occupant*)"},
    {0x100323dbeu, 56u, "cSC4PowerPoleOccupant::DoMessage(cIGZMessage2*)"},
    {0x100323e08u, 280u, "cSC4PowerPoleOccupant::InitPowerLineControlVariables()"},
    {0x100323f20u, 132u, "cSC4PowerPoleOccupant::StaticShutdown(void*)"},
    {0x100324012u, 22u, "cSC4PowerPoleOccupant::GetCell(int&, int&) const"},
    {0x100324028u, 466u, "cSC4PowerPoleOccupant::Pick(cSC4PickContext*)"},
    {0x100324208u, 16u, "cSC4PowerPoleOccupant::GetDistressed()"},
    {0x100324218u, 282u, "cSC4PowerPoleOccupant::SetDistressed(bool)"},
    {0x100324332u, 12u, "cSC4PowerPoleOccupant::IsPreview()"},
    {0x10032433eu, 13u, "cSC4PowerPoleOccupant::SetPreview(bool)"},
    {0x100324aa4u, 10u, "cSC4PowerPoleOccupant::AsOccupant()"},
    {0x100324aaeu, 14u, "cSC4PowerPoleOccupant::GetCostOfDemolition() const"},

    // cSC4PylonOccupant — 47 entry points
    {0x10022def8u, 474u, "cSC4PylonOccupant::cSC4PylonOccupant()"},
    {0x10022e0ecu, 320u, "cSC4PylonOccupant::~cSC4PylonOccupant() [GZCOM boilerplate]"},
    {0x10022e22cu, 18u, "cSC4PylonOccupant::~cSC4PylonOccupant()"},
    {0x10022e280u, 54u, "cSC4PylonOccupant::~cSC4PylonOccupant() [GZCOM boilerplate]"},
    {0x10022e2e0u, 14u, "cSC4PylonOccupant::AddRef() [GZCOM boilerplate]"},
    {0x10022e344u, 14u, "cSC4PylonOccupant::Release() [GZCOM boilerplate]"},
    {0x10022e3a8u, 120u, "cSC4PylonOccupant::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x10022e476u, 334u, "cSC4PylonOccupant::Init()"},
    {0x10022e5d6u, 260u, "cSC4PylonOccupant::Shutdown()"},
    {0x10022e6dau, 194u, "cSC4PylonOccupant::PlaceInOccupantManager(bool)"},
    {0x10022e79cu, 78u, "cSC4PylonOccupant::ReleaseCurrentTexture()"},
    {0x10022eaf6u, 306u, "cSC4PylonOccupant::DoMessage(cIGZMessage2*)"},
    {0x10022ec28u, 80u, "cSC4PylonOccupant::DoAnimationTick(unsigned int)"},
    {0x10022ec78u, 1470u, "cSC4PylonOccupant::RebuildModel()"},
    {0x10022f24cu, 34u, "cSC4PylonOccupant::SetUIState(cISC4PylonOccupant::tUIState)"},
    {0x10022f26eu, 12u, "cSC4PylonOccupant::GetUIState() const"},
    {0x10022f27au, 34u, "cSC4PylonOccupant::SetDispatchState(cISC4DispatchManager::tDispatchState)"},
    {0x10022f29cu, 12u, "cSC4PylonOccupant::GetDispatchState() const"},
    {0x10022f2a8u, 34u, "cSC4PylonOccupant::SetDispatchType(cISC4DispatchManager::tDispatchType)"},
    {0x10022f2cau, 12u, "cSC4PylonOccupant::GetDispatchType() const"},
    {0x10022f2d6u, 84u, "cSC4PylonOccupant::SetSourceOccupant(cISC4Occupant*)"},
    {0x10022f32au, 68u, "cSC4PylonOccupant::GetBounds(cS3DBoundingBox&)"},
    {0x10022f3aau, 68u, "cSC4PylonOccupant::GetWorldBounds(cS3DBoundingBox&)"},
    {0x10022f42au, 136u, "cSC4PylonOccupant::GetTransform(cS3DTransform&)"},
    {0x10022f4c0u, 140u, "cSC4PylonOccupant::SetTransform(cS3DTransform const&)"},
    {0x10022f55au, 24u, "cSC4PylonOccupant::GetSortValue(unsigned int&) const"},
    {0x10022f580u, 434u, "cSC4PylonOccupant::Draw(SC4DrawContext*, bool)"},
    {0x10022f744u, 276u, "cSC4PylonOccupant::RebuildCurrentTexture()"},
    {0x10022f858u, 1088u, "cSC4PylonOccupant::BuildCallNumberTexture(float, float)"},
    {0x10022fc98u, 200u, "cSC4PylonOccupant::PlaceInRenderer(bool)"},
    {0x10022fd60u, 30u, "cSC4PylonOccupant::GetPosition(float*)"},
    {0x10022fd9cu, 112u, "cSC4PylonOccupant::SetPosition(float const*)"},
    {0x10022fe1au, 58u, "cSC4PylonOccupant::GetBoundingBox(float*, float*)"},
    {0x10022fe8eu, 52u, "cSC4PylonOccupant::SetVisibility(bool, bool)"},
    {0x10022fed8u, 258u, "cSC4PylonOccupant::Pick(cSC4PickContext*)"},
    {0x10022ffe8u, 84u, "cSC4PylonOccupant::SetPylonOwner(cIGZUnknown*)"},
    {0x10023003cu, 14u, "cSC4PylonOccupant::GetPylonOwner() const"},
    {0x10023004au, 44u, "cSC4PylonOccupant::SetBitmapInfo(int, cIGZString const&)"},
    {0x100230076u, 1086u, "cSC4PylonOccupant::BuildMySimTexture()"},
    {0x1002304b4u, 10u, "cSC4PylonOccupant::AsOccupant()"},
    {0x1002304beu, 20u, "cSC4PylonOccupant::SetCallNumber(unsigned short)"},
    {0x1002304d2u, 14u, "cSC4PylonOccupant::GetCallNumber() const"},
    {0x1002304e0u, 14u, "cSC4PylonOccupant::SetDragState(bool)"},
    {0x1002304eeu, 12u, "cSC4PylonOccupant::GetDragState() const"},
    {0x1002304fau, 14u, "cSC4PylonOccupant::GetSourceOccupant() const"},
    {0x100230514u, 10u, "cSC4PylonOccupant::GetOwner()"},
    {0x10023051eu, 20u, "cSC4PylonOccupant::SetOwner(cIGZUnknown*)"},

    // cSC4ConstructionOccupant — 51 entry points
    {0x100177318u, 16u, "cSC4ConstructionOccupant::operator new(unsigned long)"},
    {0x100177328u, 714u, "cSC4ConstructionOccupant::cSC4ConstructionOccupant()"},
    {0x1001775f2u, 41u, "cSC4ConstructionOccupant::operator delete(void*)"},
    {0x100177626u, 14u, "cSC4ConstructionOccupant::AddRef()"},
    {0x10017768au, 14u, "cSC4ConstructionOccupant::Release()"},
    {0x1001776eeu, 34u, "cSC4ConstructionOccupant::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x1001777feu, 584u, "cSC4ConstructionOccupant::Init()"},
    {0x100177a46u, 1256u, "cSC4ConstructionOccupant::StaticInit()"},
    {0x100177feeu, 1000u, "cSC4ConstructionOccupant::Shutdown()"},
    {0x1001783d6u, 196u, "cSC4ConstructionOccupant::SendConstructionMessage(unsigned int, bool)"},
    {0x100178ea2u, 504u, "cSC4ConstructionOccupant::DoMessage(cIGZMessage2*)"},
    {0x10017909au, 622u, "cSC4ConstructionOccupant::DoDailySim()"},
    {0x100179308u, 476u, "cSC4ConstructionOccupant::DoAnimationTick(unsigned int)"},
    {0x1001794e4u, 2412u, "cSC4ConstructionOccupant::RebuildModel()"},
    {0x100179e50u, 968u, "cSC4ConstructionOccupant::CreateCrane(float, float, int)"},
    {0x10017a218u, 280u, "cSC4ConstructionOccupant::AddToRenderer()"},
    {0x10017a342u, 762u, "cSC4ConstructionOccupant::RepositionSparks()"},
    {0x10017a63cu, 212u, "cSC4ConstructionOccupant::SetBuildingCompletion(SC4Percentage const&)"},
    {0x10017a710u, 806u, "cSC4ConstructionOccupant::SetScaffoldHeight(float)"},
    {0x10017aa36u, 426u, "cSC4ConstructionOccupant::AnimateCrane(unsigned int)"},
    {0x10017abe0u, 408u, "cSC4ConstructionOccupant::SetCraneYaw(float)"},
    {0x10017ad78u, 906u, "cSC4ConstructionOccupant::SetLot(cISC4Lot*)"},
    {0x10017b102u, 570u, "cSC4ConstructionOccupant::DivideLot(cS3DBoundingBox*)"},
    {0x10017b33cu, 134u, "cSC4ConstructionOccupant::AttemptAddCrane(cS3DBoundingBox*)"},
    {0x10017b3c2u, 838u, "cSC4ConstructionOccupant::AddGroundProps(cS3DBoundingBox*)"},
    {0x10017b708u, 604u, "cSC4ConstructionOccupant::CreateConstructionCrew(cS3DBoundingBox*, cS3DBoundingBox&)"},
    {0x10017b964u, 544u, "cSC4ConstructionOccupant::AddRoofProps(cS3DBoundingBox&)"},
    {0x10017bb84u, 362u, "cSC4ConstructionOccupant::CreateDustClouds()"},
    {0x10017bceeu, 424u, "cSC4ConstructionOccupant::CreateSparks()"},
    {0x10017be96u, 588u, "cSC4ConstructionOccupant::CreateLotTexture()"},
    {0x10017c0e2u, 478u, "cSC4ConstructionOccupant::GetBounds(cS3DBoundingBox&)"},
    {0x10017c2ceu, 68u, "cSC4ConstructionOccupant::GetWorldBounds(cS3DBoundingBox&)"},
    {0x10017c34eu, 136u, "cSC4ConstructionOccupant::GetTransform(cS3DTransform&)"},
    {0x10017c3e4u, 140u, "cSC4ConstructionOccupant::SetTransform(cS3DTransform const&)"},
    {0x10017c47eu, 44u, "cSC4ConstructionOccupant::GetSortValue(unsigned int&) const"},
    {0x10017c4d2u, 1132u, "cSC4ConstructionOccupant::Draw(SC4DrawContext*, bool)"},
    {0x10017c93eu, 504u, "cSC4ConstructionOccupant::DoRotationChange(int)"},
    {0x10017cb4eu, 88u, "cSC4ConstructionOccupant::GetPosition(float*)"},
    {0x10017cbfeu, 6u, "cSC4ConstructionOccupant::SetPosition(float const*)"},
    {0x10017cc0au, 58u, "cSC4ConstructionOccupant::GetBoundingBox(float*, float*)"},
    {0x10017cc7eu, 1414u, "cSC4ConstructionOccupant::AttemptAddOuterCrane(cS3DBoundingBox*)"},
    {0x10017d204u, 736u, "cSC4ConstructionOccupant::AttemptAddInnerCrane()"},
    {0x10017d4e4u, 342u, "cSC4ConstructionOccupant::RemoveRectangle(cS3DBoundingBox&, cS3DBoundingBox&)"},
    {0x10017d63au, 956u, "cSC4ConstructionOccupant::CreateAndPlaceProp(cS3DBoundingBox&, unsigned int, cISC4PropOccupant*&, bool)"},
    {0x10017d9f6u, 582u, "cSC4ConstructionOccupant::CreateAutomata(cS3DBoundingBox&, bool, unsigned int)"},
    {0x10017dd04u, 10u, "cSC4ConstructionOccupant::AsOccupant()"},
    {0x10017dd1au, 10u, "cSC4ConstructionOccupant::GetOwner()"},
    {0x10017dd24u, 6u, "cSC4ConstructionOccupant::SetOwner(cIGZUnknown*)"},
    {0x10017dd2au, 18u, "cSC4ConstructionOccupant::~cSC4ConstructionOccupant()"},
    {0x10017dd3cu, 82u, "cSC4ConstructionOccupant::~cSC4ConstructionOccupant() [GZCOM boilerplate]"},
    {0x10017df2cu, 372u, "cSC4ConstructionOccupant::~cSC4ConstructionOccupant() [GZCOM boilerplate]"},

    // cSC4SignpostOccupant — 30 entry points
    {0x10070e69eu, 16u, "cSC4SignpostOccupant::operator new(unsigned long)"},
    {0x10070e6aeu, 612u, "cSC4SignpostOccupant::cSC4SignpostOccupant()"},
    {0x10070e912u, 41u, "cSC4SignpostOccupant::operator delete(void*)"},
    {0x10070e952u, 14u, "cSC4SignpostOccupant::AddRef()"},
    {0x10070e9c8u, 14u, "cSC4SignpostOccupant::Release()"},
    {0x10070ea3eu, 438u, "cSC4SignpostOccupant::Init()"},
    {0x10070ebf4u, 280u, "cSC4SignpostOccupant::LoadTextures()"},
    {0x10070ed0cu, 4708u, "cSC4SignpostOccupant::RebuildModel()"},
    {0x10070ff94u, 240u, "cSC4SignpostOccupant::Shutdown()"},
    {0x1007100a8u, 38u, "cSC4SignpostOccupant::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x100710136u, 300u, "cSC4SignpostOccupant::DoMessage(cIGZMessage2*)"},
    {0x10071027eu, 192u, "cSC4SignpostOccupant::SetAllFlags(unsigned int)"},
    {0x10071034cu, 116u, "cSC4SignpostOccupant::SetText(cIGZString&)"},
    {0x1007103c0u, 274u, "cSC4SignpostOccupant::SetDynamic(bool)"},
    {0x100710510u, 488u, "cSC4SignpostOccupant::Draw(SC4DrawContext*, bool)"},
    {0x100710712u, 248u, "cSC4SignpostOccupant::Pick(cSC4PickContext*)"},
    {0x10071081cu, 320u, "cSC4SignpostOccupant::SetPosition(float const*)"},
    {0x100710978u, 148u, "cSC4SignpostOccupant::SetEditMode(bool)"},
    {0x100710c10u, 10u, "cSC4SignpostOccupant::AsOccupant()"},
    {0x100710c1au, 28u, "cSC4SignpostOccupant::GetText(cIGZString&)"},
    {0x100710c36u, 10u, "cSC4SignpostOccupant::GetOwner()"},
    {0x100710c40u, 6u, "cSC4SignpostOccupant::SetOwner(cIGZUnknown*)"},
    {0x100710c46u, 60u, "cSC4SignpostOccupant::GetBounds(cS3DBoundingBox&)"},
    {0x100710c82u, 60u, "cSC4SignpostOccupant::GetWorldBounds(cS3DBoundingBox&)"},
    {0x100710cbeu, 24u, "cSC4SignpostOccupant::GetTransform(cS3DTransform&)"},
    {0x100710cd6u, 18u, "cSC4SignpostOccupant::SetTransform(cS3DTransform const&)"},
    {0x100710ce8u, 12u, "cSC4SignpostOccupant::GetSortValue(unsigned int&) const"},
    {0x100710d00u, 18u, "cSC4SignpostOccupant::~cSC4SignpostOccupant()"},
    {0x100710d12u, 68u, "cSC4SignpostOccupant::~cSC4SignpostOccupant() [GZCOM boilerplate]"},
    {0x1007110cau, 446u, "cSC4SignpostOccupant::~cSC4SignpostOccupant() [GZCOM boilerplate]"},

    // cSC4PropOccupant — 40 entry points
    {0x10015e4a4u, 16u, "cSC4PropOccupant::operator new(unsigned long)"},
    {0x10015e4b4u, 10u, "cSC4PropOccupant::cSC4PropOccupant()"},
    {0x10015e4beu, 29u, "cSC4PropOccupant::operator delete(void*)"},
    {0x10015e4e6u, 220u, "cSC4PropOccupant::cSC4PropOccupant()"},
    {0x10015e5c2u, 44u, "cSC4PropOccupant::Init()"},
    {0x10015e612u, 116u, "cSC4PropOccupant::Shutdown()"},
    {0x10015ea90u, 14u, "cSC4PropOccupant::AddRef()"},
    {0x10015eae0u, 14u, "cSC4PropOccupant::Release()"},
    {0x10015eb30u, 60u, "cSC4PropOccupant::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x10015ebb2u, 438u, "cSC4PropOccupant::DoMessage(cIGZMessage2*)"},
    {0x10015edc2u, 6u, "cSC4PropOccupant::SetPropPlacementProperties(cSC4PropPlacementProperties*)"},
    {0x10015edc8u, 144u, "cSC4PropOccupant::PlaceInOccupantManager(bool)"},
    {0x10015ee58u, 174u, "cSC4PropOccupant::PlaceInRenderer(bool)"},
    {0x10015ef06u, 60u, "cSC4PropOccupant::GetPosition(float*)"},
    {0x10015ef7cu, 126u, "cSC4PropOccupant::SetPosition(float const*)"},
    {0x10015f008u, 46u, "cSC4PropOccupant::GetBoundingBox(float*, float*)"},
    {0x10015f05eu, 54u, "cSC4PropOccupant::SetBoundingBox(float const*, float const*)"},
    {0x10015f094u, 24u, "cSC4PropOccupant::GetPropType() const"},
    {0x10015f0acu, 30u, "cSC4PropOccupant::SetPropType(unsigned int)"},
    {0x10015f0cau, 14u, "cSC4PropOccupant::GetOrientation() const"},
    {0x10015f0d8u, 28u, "cSC4PropOccupant::SetOrientation(int)"},
    {0x10015f0f4u, 562u, "cSC4PropOccupant::SetState(unsigned int)"},
    {0x10015f326u, 220u, "cSC4PropOccupant::SetTimeOfDayRange(float, float)"},
    {0x10015f402u, 56u, "cSC4PropOccupant::GetTimeOfDayRange(float&, float&) const"},
    {0x10015f43au, 510u, "cSC4PropOccupant::SetSimulatorDateRange(unsigned int, unsigned int, unsigned char, unsigned char)"},
    {0x10015f638u, 14u, "cSC4PropOccupant::SetRandomChance(unsigned char)"},
    {0x10015f646u, 140u, "cSC4PropOccupant::SetPropTimerState(cISC4PropOccupant::tPropTimerMask, bool)"},
    {0x10015f6d2u, 374u, "cSC4PropOccupant::TickSimulatorDate(cIGZDate const&)"},
    {0x10015f848u, 26u, "cSC4PropOccupant::GetLotTag(unsigned int&) const"},
    {0x10015f87cu, 12u, "cSC4PropOccupant::SetLotTag(unsigned int)"},
    {0x10015f894u, 14u, "cSC4PropOccupant::GetPropUsage() const"},
    {0x10015f8a2u, 13u, "cSC4PropOccupant::SetPropUsage(unsigned char)"},
    {0x1001632d6u, 10u, "cSC4PropOccupant::AsOccupant()"},
    {0x1001632e0u, 8u, "cSC4PropOccupant::GetPropPlacementProperties()"},
    {0x1001632e8u, 28u, "cSC4PropOccupant::GetState() const"},
    {0x100163304u, 16u, "cSC4PropOccupant::GetPropTimerBitfield() const"},
    {0x100163320u, 14u, "cSC4PropOccupant::~cSC4PropOccupant()"},
    {0x10016332eu, 66u, "cSC4PropOccupant::~cSC4PropOccupant() [GZCOM boilerplate]"},
    {0x1007126d4u, 28u, "cSC4PropOccupant::GetRandomChance() const"},
    {0x1007126f0u, 18u, "cSC4PropOccupant::GetPropTimerState(cISC4PropOccupant::tPropTimerMask) const"},

    // cSC4TextDecalOccupant — 28 entry points
    {0x10071147eu, 16u, "cSC4TextDecalOccupant::operator new(unsigned long)"},
    {0x10071148eu, 10u, "cSC4TextDecalOccupant::cSC4TextDecalOccupant()"},
    {0x100711498u, 35u, "cSC4TextDecalOccupant::operator delete(void*)"},
    {0x1007114c6u, 304u, "cSC4TextDecalOccupant::cSC4TextDecalOccupant()"},
    {0x1007115f6u, 14u, "cSC4TextDecalOccupant::AddRef()"},
    {0x100711654u, 14u, "cSC4TextDecalOccupant::Release()"},
    {0x1007116b2u, 252u, "cSC4TextDecalOccupant::Init()"},
    {0x1007117aeu, 192u, "cSC4TextDecalOccupant::UpdateOverlayManager()"},
    {0x100711892u, 284u, "cSC4TextDecalOccupant::Shutdown()"},
    {0x1007119d2u, 38u, "cSC4TextDecalOccupant::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x100711a4cu, 340u, "cSC4TextDecalOccupant::DoMessage(cIGZMessage2*)"},
    {0x100711ba0u, 650u, "cSC4TextDecalOccupant::UpdateDecal()"},
    {0x100711e46u, 84u, "cSC4TextDecalOccupant::RemoveDecals()"},
    {0x100711e9au, 100u, "cSC4TextDecalOccupant::MoveDecal()"},
    {0x100711efeu, 112u, "cSC4TextDecalOccupant::SetPosition(float const*)"},
    {0x100711f8au, 36u, "cSC4TextDecalOccupant::SetText(cIGZString&)"},
    {0x100711faeu, 188u, "cSC4TextDecalOccupant::UpdateTexture()"},
    {0x10071206au, 42u, "cSC4TextDecalOccupant::SetTextHeight(float)"},
    {0x100712094u, 34u, "cSC4TextDecalOccupant::SetDynamic(bool)"},
    {0x1007120b6u, 308u, "cSC4TextDecalOccupant::SetTextColor(cS3DVector3)"},
    {0x1007121eau, 134u, "cSC4TextDecalOccupant::SetAlpha(float)"},
    {0x100712270u, 26u, "cSC4TextDecalOccupant::SetOrientation(cISC4TextDecalOccupant::TextDecalOrientation)"},
    {0x10071228au, 200u, "cSC4TextDecalOccupant::SetAllFlags(unsigned int)"},
    {0x10071264au, 10u, "cSC4TextDecalOccupant::AsOccupant()"},
    {0x100712654u, 28u, "cSC4TextDecalOccupant::GetText(cIGZString&)"},
    {0x10071267cu, 10u, "cSC4TextDecalOccupant::~cSC4TextDecalOccupant()"},
    {0x100712686u, 62u, "cSC4TextDecalOccupant::~cSC4TextDecalOccupant() [GZCOM boilerplate]"},
    {0x100712800u, 308u, "cSC4TextDecalOccupant::~cSC4TextDecalOccupant() [GZCOM boilerplate]"},

    // cSC4LotBaseTextureOccupant — 26 entry points
    {0x1000b36d0u, 16u, "cSC4LotBaseTextureOccupant::operator new(unsigned long)"},
    {0x1000b36e0u, 150u, "cSC4LotBaseTextureOccupant::cSC4LotBaseTextureOccupant()"},
    {0x1000b3776u, 37u, "cSC4LotBaseTextureOccupant::operator delete(void*)"},
    {0x1000b37acu, 128u, "cSC4LotBaseTextureOccupant::~cSC4LotBaseTextureOccupant() [GZCOM boilerplate]"},
    {0x1000b382cu, 10u, "cSC4LotBaseTextureOccupant::~cSC4LotBaseTextureOccupant()"},
    {0x1000b3852u, 62u, "cSC4LotBaseTextureOccupant::~cSC4LotBaseTextureOccupant() [GZCOM boilerplate]"},
    {0x1000b38acu, 14u, "cSC4LotBaseTextureOccupant::Init()"},
    {0x1000b38c4u, 52u, "cSC4LotBaseTextureOccupant::Shutdown()"},
    {0x1000b3cbau, 34u, "cSC4LotBaseTextureOccupant::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x1000b3d74u, 14u, "cSC4LotBaseTextureOccupant::AddRef()"},
    {0x1000b3dbau, 32u, "cSC4LotBaseTextureOccupant::Release() [GZCOM boilerplate]"},
    {0x1000b3e66u, 70u, "cSC4LotBaseTextureOccupant::GetPosition(float*)"},
    {0x1000b3ef2u, 6u, "cSC4LotBaseTextureOccupant::SetPosition(float const*)"},
    {0x1000b3efeu, 40u, "cSC4LotBaseTextureOccupant::GetBoundingBox(float*, float*)"},
    {0x1000b3f4eu, 190u, "cSC4LotBaseTextureOccupant::SetAllFlags(unsigned int)"},
    {0x1000b401au, 244u, "cSC4LotBaseTextureOccupant::GetCellCoverage(std::__1::vector<SC4Point<int>, std::__1::allocator<SC4Point<int>>>&, std::__1::vector<bool, std::__1::allocator<bool>>*) const"},
    {0x1000b410eu, 122u, "cSC4LotBaseTextureOccupant::IsCellCovered(int, int, bool*) const"},
    {0x1000b4188u, 1036u, "cSC4LotBaseTextureOccupant::CalculateBoundingBox(bool*)"},
    {0x1000b4594u, 1600u, "cSC4LotBaseTextureOccupant::SetTextureSpecification(cISC4LotBaseTextureOccupant::LotBaseTextureSpecification const*, unsigned int, bool*)"},
    {0x1000b4bd4u, 732u, "cSC4LotBaseTextureOccupant::UpdateExistingModels(bool)"},
    {0x1000b4f40u, 120u, "cSC4LotBaseTextureOccupant::UpdateVertexAltitudes(bool, bool, bool*)"},
    {0x1000b4fb8u, 398u, "cSC4LotBaseTextureOccupant::LoadModels(bool)"},
    {0x1000b52a8u, 102u, "cSC4LotBaseTextureOccupant::UnloadModels()"},
    {0x1000b5ec6u, 10u, "cSC4LotBaseTextureOccupant::AsOccupant()"},
    {0x1000b5ed0u, 14u, "cSC4LotBaseTextureOccupant::GetUsage() const"},
    {0x1000b5edeu, 14u, "cSC4LotBaseTextureOccupant::SetUsage(int)"},

    // cSC4ZoneManager — 25 entry points
    {0x100198108u, 200u, "cSC4ZoneManager::cSC4ZoneManager()"},
    {0x1001981d0u, 100u, "cSC4ZoneManager::~cSC4ZoneManager() [GZCOM boilerplate]"},
    {0x100198234u, 104u, "cSC4ZoneManager::~cSC4ZoneManager() [GZCOM boilerplate]"},
    {0x10019829cu, 74u, "cSC4ZoneManager::QueryInterface(unsigned int, void**) [GZCOM boilerplate]"},
    {0x100198302u, 14u, "cSC4ZoneManager::AddRef()"},
    {0x10019832cu, 68u, "cSC4ZoneManager::Release() [GZCOM boilerplate]"},
    {0x1001983e4u, 464u, "cSC4ZoneManager::Init()"},
    {0x100198810u, 222u, "cSC4ZoneManager::Shutdown()"},
    {0x100198ffeu, 676u, "cSC4ZoneManager::DoMessage(cIGZMessage2*)"},
    {0x1001992bau, 14u, "cSC4ZoneManager::GetZoneCount(int)"},
    {0x1001992c8u, 12u, "cSC4ZoneManager::GetUndevelopedTileCount()"},
    {0x1001992d4u, 16u, "cSC4ZoneManager::GetUndevelopedTileCount(int)"},
    {0x1001992e4u, 54u, "cSC4ZoneManager::GetDevelopmentFailureCount(int, int)"},
    {0x10019939cu, 12u, "cSC4ZoneManager::GetAbandonedTileCount()"},
    {0x1001993a8u, 2748u, "cSC4ZoneManager::PlaceZone(SC4CellRegion<int>&, int, bool, bool, bool, bool, bool, long long*, int*, std::__1::set<cRZAutoRefCount<cISC4Lot>, std::__1::less<cISC4Lot*>, std::__1::allocator<cRZAutoRefCount<cISC4Lot>>>*)"},
    {0x100199e64u, 52u, "cSC4ZoneManager::GetZoneType(int, int)"},
    {0x100199e98u, 28u, "cSC4ZoneManager::IsRCIZone(int)"},
    {0x100199eb4u, 44u, "cSC4ZoneManager::GetTextureForZone(int, cGZPersistResourceKey&)"},
    {0x100199ee0u, 14u, "cSC4ZoneManager::GetDefaultZoneDisplayAlpha() const"},
    {0x100199eeeu, 264u, "cSC4ZoneManager::SetZoneDisplayAlpha(unsigned char)"},
    {0x100199ff6u, 10u, "cSC4ZoneManager::GetZoneGrid() const"},
    {0x10019a000u, 16u, "cSC4ZoneManager::GetZoneDragColor(int) const"},
    {0x10019a010u, 14u, "cSC4ZoneManager::GetZoneDisplayAlpha() const"},
    {0x10019a01eu, 16u, "cSC4ZoneManager::GetMinZoneSize(int) const"},
    {0x10019a02eu, 16u, "cSC4ZoneManager::GetMaxZoneSize(int) const"},
}};

constexpr std::size_t Wave2BMethodCount() noexcept {
    return kWave2BMethods.size();
}

constexpr const Wave2BMethod* FindWave2BMethod(Uint64 address) noexcept {
    for (const Wave2BMethod& method : kWave2BMethods) {
        if (method.address == address) {
            return &method;
        }
    }
    return nullptr;
}

static_assert(Wave2BMethodCount() == 1097u);
static_assert(FindWave2BMethod(0x100183a68u) != nullptr);
static_assert(FindWave2BMethod(0x100183a84u) != nullptr);
static_assert(FindWave2BMethod(0x1003b1cf2u) != nullptr);
static_assert(FindWave2BMethod(0x1003b2e34u) != nullptr);
static_assert(FindWave2BMethod(0x100199e98u) != nullptr);
static_assert(FindWave2BMethod(0x100199eb4u) != nullptr);
static_assert(FindWave2BMethod(0x10019a000u) != nullptr);
static_assert(FindWave2BMethod(0x1001a296cu) != nullptr);
static_assert(FindWave2BMethod(0x10019e9a8u) != nullptr);
static_assert(FindWave2BMethod(0x100740e50u) != nullptr);
static_assert(FindWave2BMethod(0x100247656u) != nullptr);
static_assert(FindWave2BMethod(0x10073083eu) != nullptr);
static_assert(FindWave2BMethod(0x100198108u) != nullptr);

// ---------------------------------------------------------------------------
// Wave 3 Part 1 method index
// Networks, view input, water paths, buildings, effects, parks, vehicles,
// advisors, and ambience filters. Every 0x100xxxxxx entry point from the
// Wave 3 Part 1 workpack (72 classes, 453 symbols) is recorded in
// kWave3Part1Methods.
// ---------------------------------------------------------------------------

struct Wave3Part1Method {
    Uint64 address;
    Uint32 sizeBytes;
    const char* name;
};

inline constexpr std::array<Wave3Part1Method, 453> kWave3Part1Methods = {{
    // cSC4NetworkTool — 138 entry points
    {0x1001862bcu, 1154u, "cSC4NetworkTool::cSC4NetworkTool(cISC4NetworkManager::eNetworkType)"},
    {0x100186d04u, 58u, "cSC4NetworkTool::StaticInit()"},
    {0x100186f9cu, 1968u, "cSC4NetworkTool::ReloadNetworkTypeInfo(cRZIniFile&)"},
    {0x100187752u, 1430u, "cSC4NetworkTool::Init()"},
    {0x100187ce8u, 316u, "cSC4NetworkTool::Shutdown()"},
    {0x100187e24u, 100u, "cSC4NetworkTool::GetUITool(cISC4NetworkToolUI**)"},
    {0x100187e88u, 64u, "cSC4NetworkTool::GetPossibleBridges(std::__1::vector<int, std::__1::allocator<int>>&)"},
    {0x100187efcu, 70u, "cSC4NetworkTool::GetBridgeName(cIGZString&, int)"},
    {0x100187fc8u, 66u, "cSC4NetworkTool::GetBridgeBitmapKey(int, unsigned int&, unsigned int&)"},
    {0x10018800au, 52u, "cSC4NetworkTool::GetPossibleNeighborConnection(cISC4NetworkTool::NeighborConnectionInfo&)"},
    {0x10018803eu, 218u, "cSC4NetworkTool::ClearCellPreviews()"},
    {0x100188118u, 514u, "cSC4NetworkTool::ClearNetworkOccsFromCell(unsigned int, unsigned int, cISC4OccupantFilter*)"},
    {0x10018831au, 376u, "cSC4NetworkTool::CopyTunnelElevationConstraintsIntoCellInfo(std::__1::vector<cSC4NetworkTool::tImmovablePortion, std::__1::allocator<cSC4NetworkTool::tImmovablePortion>>&)"},
    {0x100188492u, 778u, "cSC4NetworkTool::CreateNetworkOccupant(cSC4NetworkCellInfo*, cISC4NetworkOccupant**, float (*) [3], unsigned int)"},
    {0x10018879cu, 134u, "cSC4NetworkTool::DetermineSupportType(cSC4NetworkCellInfo&)"},
    {0x100188822u, 54u, "cSC4NetworkTool::DoMessage(cIGZMessage2*)"},
    {0x10018888eu, 560u, "cSC4NetworkTool::GetCostOfSolution()"},
    {0x100188abeu, 110u, "cSC4NetworkTool::GetOccupantExemplar(unsigned int, unsigned int)"},
    {0x100188b2cu, 74u, "cSC4NetworkTool::GetPaddedRectAroundCell(unsigned int, unsigned int, SC4Rect<unsigned int>&, unsigned int)"},
    {0x100188b76u, 152u, "cSC4NetworkTool::GetNetworkOccupant(unsigned int, unsigned int, cISC4OccupantFilter*, cISC4NetworkOccupant**)"},
    {0x100188c0eu, 50u, "cSC4NetworkTool::GetSupportStructureInfo(unsigned int)"},
    {0x100188c40u, 618u, "cSC4NetworkTool::InsertHighwayIntersectionPiece(cSC4NetworkCellInfo*, std::__1::list<cISC4NetworkOccupant*, std::__1::allocator<cISC4NetworkOccupant*>>*, bool, unsigned int, unsigned int, int, int, unsigned int)"},
    {0x100188eaau, 176u, "cSC4NetworkTool::SetExemplarForOccupant(cISC4NetworkOccupant*, unsigned int, float const*, unsigned char, unsigned int)"},
    {0x100188f5au, 308u, "cSC4NetworkTool::InsertHighwayIntersection(int, int, unsigned int, std::__1::list<cISC4NetworkOccupant*, std::__1::allocator<cISC4NetworkOccupant*>>*, bool, bool)"},
    {0x10018908eu, 108u, "cSC4NetworkTool::InsertHighwayIntersectionPieces(std::__1::list<cISC4NetworkOccupant*, std::__1::allocator<cISC4NetworkOccupant*>>*)"},
    {0x1001890fau, 496u, "cSC4NetworkTool::PlaceNetworkOccupantById(int, int, unsigned int, bool, cISC4NetworkManager::eNetworkType)"},
    {0x1001892eau, 130u, "cSC4NetworkTool::AdjacentHorizontallyToMultitype(unsigned int, unsigned int, unsigned int)"},
    {0x10018936cu, 134u, "cSC4NetworkTool::AdjacentVerticallyToMultitype(unsigned int, unsigned int, unsigned int)"},
    {0x1001893f2u, 3402u, "cSC4NetworkTool::InsertNetworkOccupant(cSC4NetworkCellInfo&, cSC4NetworkTool::tSolvedCell const&, std::__1::list<cISC4NetworkOccupant*, std::__1::allocator<cISC4NetworkOccupant*>>*)"},
    {0x10018a13cu, 1570u, "cSC4NetworkTool::InsertOccupantsForSolvedCells(std::__1::vector<cSC4NetworkTool::tSolvedCell, std::__1::allocator<cSC4NetworkTool::tSolvedCell>>&, std::__1::list<cISC4NetworkOccupant*, std::__1::allocator<cISC4NetworkOccupant*>>*, bool)"},
    {0x10018a75eu, 1096u, "cSC4NetworkTool::InsertTunnelPiece(unsigned char, unsigned char, cSC4NetworkCellInfo*)"},
    {0x10018aba6u, 1274u, "cSC4NetworkTool::InsertTunnelPieces(cISC4NetworkManager::eNetworkType)"},
    {0x10018b0a0u, 102u, "cSC4NetworkTool::IsSimpleDiagonalNetwork(cSC4NetworkCellInfo&, cISC4NetworkManager::eNetworkType)"},
    {0x10018b106u, 1732u, "cSC4NetworkTool::QueryUserForPotentialIntersections()"},
    {0x10018b7cau, 1532u, "cSC4NetworkTool::InsertIsolatedHighwayIntersection(int, int, unsigned int, bool)"},
    {0x10018bdc6u, 1970u, "cSC4NetworkTool::AttemptNeighborConnections(bool, cISC4NetworkManager::eNetworkType)"},
    {0x10018c578u, 200u, "cSC4NetworkTool::MarkNeighborConnections()"},
    {0x10018c640u, 210u, "cSC4NetworkTool::PlaceNeighborConnections()"},
    {0x10018c712u, 408u, "cSC4NetworkTool::MarkPositionChangedExistingOccupantsAsNewlySolved()"},
    {0x10018c8aau, 570u, "cSC4NetworkTool::MarkTerrainChangedExistingOccupantsAsNewlySolved(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>> const&)"},
    {0x10018cae4u, 100u, "cSC4NetworkTool::AddEdgesToTile(int, int, unsigned int, cISC4NetworkManager::eNetworkType, unsigned char, unsigned char)"},
    {0x10018cb48u, 476u, "cSC4NetworkTool::AddCellPreview(cSC4NetworkCellInfo&, cSC4NetworkTool::tSolvedCell const&)"},
    {0x10018cd24u, 264u, "cSC4NetworkTool::SetTerrainHilight(int)"},
    {0x10018ce2cu, 388u, "cSC4NetworkTool::SetQuadTrianglesToDraw(cISC4NetworkOccupant*, cSC4TileNeighborhoodCompact const&)"},
    {0x10018cfb0u, 9574u, "cSC4NetworkTool::DestroyOccupantsAndResolve(std::__1::vector<cISC4NetworkOccupant*, std::__1::allocator<cISC4NetworkOccupant*>> const&)"},
    {0x10018f5b2u, 2040u, "cSC4NetworkTool::PlaceNetworkOccupants(int, int, int, int, unsigned int, cISC4NetworkTool*, bool, int)"},
    {0x10018fdaau, 132u, "cSC4NetworkTool::Reset()"},
    {0x10018fe2eu, 76u, "cSC4NetworkTool::OccupantExists(int, int, unsigned int, unsigned int&)"},
    {0x10018fe7au, 248u, "cSC4NetworkTool::GetFailureReasonString(int, cRZString&)"},
    {0x10018ff72u, 564u, "cSC4NetworkTool::UpdateOccupantHighlights(int)"},
    {0x1001901a6u, 268u, "cSC4NetworkTool::ClearOccupantHighlights()"},
    {0x1001902b2u, 212u, "cSC4NetworkTool::DestroyHilightedOccupants()"},
    {0x1001903b4u, 216u, "cSC4NetworkTool::UpdatePositionChangedPreviewsForPreviousSegments()"},
    {0x100190668u, 56u, "cSC4NetworkTool::GetLastValidPoint(SC4Point<unsigned int>&)"},
    {0x100193678u, 56u, "cSC4NetworkTool::GetDraggedCells(std::__1::vector<SC4Point<unsigned int>, std::__1::allocator<SC4Point<unsigned int>>>&)"},
    {0x1001b14a0u, 300u, "cSC4NetworkTool::SmoothenNetwork(int)"},
    {0x1001b15ccu, 374u, "cSC4NetworkTool::InsertIntersectionCellsIntoSolvedSet()"},
    {0x1001b1742u, 7348u, "cSC4NetworkTool::ComputeInitialSmoothingSet(int)"},
    {0x1001b3434u, 136u, "cSC4NetworkTool::InsertConstraintsForCellsToBeSmoothed(std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>> const&)"},
    {0x1001b34bcu, 808u, "cSC4NetworkTool::InsertSlopeAndSmoothnessConstraintsForCell(unsigned int)"},
    {0x1001b37e4u, 796u, "cSC4NetworkTool::InsertRangeConstraintsForCell(unsigned int)"},
    {0x1001b3b00u, 824u, "cSC4NetworkTool::GetCrossSections(cSC4NetworkCellInfo const&, cISC4NetworkManager::eNetworkType, int, cSC4NetworkTool::tCrossSection&, cSC4NetworkTool::tCrossSection&, cSC4NetworkTool::tCrossSection&, bool&)"},
    {0x1001b3e60u, 872u, "cSC4NetworkTool::ApplyPlaneConstraintToRegularCells(std::__1::vector<cSC4NetworkTool::tImmovablePortion, std::__1::allocator<cSC4NetworkTool::tImmovablePortion>>&)"},
    {0x1001b41c8u, 922u, "cSC4NetworkTool::ComputeReferencePlaneForVertexMovement(float*, int, int, float, float)"},
    {0x1001b45b4u, 576u, "cSC4NetworkTool::GetElevationForReferencePlaneVertex(int, float&, bool&)"},
    {0x1001b4bc0u, 48u, "cSC4NetworkTool::GetCellInfo(unsigned int)"},
    {0x1001b4bf0u, 716u, "cSC4NetworkTool::ComputeAndStoreCellInfo(unsigned int)"},
    {0x1001b4ebcu, 960u, "cSC4NetworkTool::CreateCellInfosForCurrentPlacementTiles(cISC4NetworkManager::eNetworkType)"},
    {0x1001b527cu, 508u, "cSC4NetworkTool::CreatePlaceholderCellInfo(unsigned int, unsigned int, cISC4NetworkManager::eNetworkType)"},
    {0x1001b5478u, 70u, "cSC4NetworkTool::GetNetworkVertexHt(int) const"},
    {0x1001b54beu, 844u, "cSC4NetworkTool::CanTilesBeSupportedOnTerrain(cISC4NetworkManager::eNetworkType)"},
    {0x1001b580au, 1022u, "cSC4NetworkTool::InsertIntersectionConstraints(cSC4VertexHtConstraintSatisfier&)"},
    {0x1001b5c08u, 492u, "cSC4NetworkTool::UpdateTerrainHeights(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&)"},
    {0x1001b6144u, 770u, "cSC4NetworkTool::BuildPaddedNetworkSet(std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>> const&, std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>>&, unsigned int)"},
    {0x1001b6446u, 1000u, "cSC4NetworkTool::AssignEdgeSubtypes(std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>>&, std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>>&, bool)"},
    {0x1001b682eu, 1420u, "cSC4NetworkTool::SolveMultiTypeIntersections(std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>> const&, std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&, int)"},
    {0x1001b6dbau, 674u, "cSC4NetworkTool::SolveUsingEvaluator(std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>>&, std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>>&, cSC4TileEvaluator*, cISC4OccupantManager*, cISC4NetworkManager::eNetworkType)"},
    {0x1001b705cu, 584u, "cSC4NetworkTool::AddToSet(std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>>&, std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>> const&)"},
    {0x1001b72a4u, 1488u, "cSC4NetworkTool::AdjustTileSubsets(std::__1::vector<cSC4NetworkTool::tSolvedCell, std::__1::allocator<cSC4NetworkTool::tSolvedCell>>&, bool)"},
    {0x1001b7874u, 1288u, "cSC4NetworkTool::VerifyTileSubsets(std::__1::vector<cSC4NetworkTool::tSolvedCell, std::__1::allocator<cSC4NetworkTool::tSolvedCell>>&)"},
    {0x1001b7d7cu, 442u, "cSC4NetworkTool::SolveMultiTypeSeparation(SC4Point<int> const&, cISC4NetworkManager::eNetworkType, unsigned int&, unsigned char&, bool&)"},
    {0x1001b7f36u, 1942u, "cSC4NetworkTool::ApplyNetworkRules(std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>>&, std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>>&, std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>>&, cSC4TileEvaluator*, cISC4OccupantManager*, cISC4NetworkManager::eNetworkType)"},
    {0x1001b86ccu, 1590u, "cSC4NetworkTool::BuildSetOfCellsToBeSolved()"},
    {0x1001b8d02u, 1058u, "cSC4NetworkTool::ExpandToSolveSetBasedOnConnectivity(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&, int)"},
    {0x1001b926au, 446u, "cSC4NetworkTool::MarkExtendingDiagonalEdges(cSC4NetworkCellInfo const&, bool*, cISC4NetworkManager::eNetworkType)"},
    {0x1001b9428u, 554u, "cSC4NetworkTool::AssignSimpleEdgeSubtypes(cSC4NetworkCellInfo*, cISC4NetworkManager::eNetworkType, std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>> const&, bool)"},
    {0x1001b9652u, 536u, "cSC4NetworkTool::AssignComplexEdgeSubtypes(cSC4NetworkCellInfo*, cISC4NetworkManager::eNetworkType, std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>> const&, bool)"},
    {0x1001c0300u, 376u, "cSC4NetworkTool::CopyBridgeElevationConstraintsIntoCellInfo(std::__1::vector<cSC4NetworkTool::tImmovablePortion, std::__1::allocator<cSC4NetworkTool::tImmovablePortion>>&)"},
    {0x1001c0478u, 774u, "cSC4NetworkTool::CreateLandBridge(int, int, float, cISC4NetworkManager::eNetworkType)"},
    {0x1001c077eu, 670u, "cSC4NetworkTool::FindBestBridgeType(int&, int&, bool, float, cISC4NetworkManager::eNetworkType)"},
    {0x1001c0a1cu, 1190u, "cSC4NetworkTool::ConstructSpecializedBridge(cSC4NetworkTool::tBridge&, cSC4NetworkBridgeSpecialization const&)"},
    {0x1001c0ec2u, 4278u, "cSC4NetworkTool::ConstructParametricBridge(cSC4NetworkTool::tBridge&)"},
    {0x1001c1f78u, 312u, "cSC4NetworkTool::PadStepsToMakeMinBridgeSpan(int, int&, int&, cISC4NetworkManager::eNetworkType)"},
    {0x1001c20b0u, 949u, "cSC4NetworkTool::IdentifyBridgeCells(int&, cISC4NetworkManager::eNetworkType)"},
    {0x1001c2546u, 252u, "cSC4NetworkTool::CheckBridgeStepEdgeTypes(int, int)"},
    {0x1001c2642u, 476u, "cSC4NetworkTool::SelectBridgeStartAndEnd(int, int&, int&, int&, float&, cISC4NetworkManager::eNetworkType)"},
    {0x1001c281eu, 294u, "cSC4NetworkTool::SelectBridgeElevation(cSC4NetworkTool::tBridge&)"},
    {0x1001c2944u, 716u, "cSC4NetworkTool::IdentifyLandBridgeCells(int, cISC4NetworkManager::eNetworkType)"},
    {0x1001c2c10u, 214u, "cSC4NetworkTool::IdentifyWaterAndLandBridgeTypes(std::__1::vector<cSC4NetworkBridgeTypeInfo, std::__1::allocator<cSC4NetworkBridgeTypeInfo>> const*, std::__1::vector<int, std::__1::allocator<int>>&, std::__1::vector<int, std::__1::allocator<int>>&)"},
    {0x1001c2ce6u, 1850u, "cSC4NetworkTool::InsertBridgePiece(cSC4NetworkTool::tBridgePiece const&, cSC4NetworkCellInfo*, cSC4NetworkBridgeTypeInfo const&, std::__1::list<cISC4NetworkOccupant*, std::__1::allocator<cISC4NetworkOccupant*>>*)"},
    {0x1001c3420u, 1212u, "cSC4NetworkTool::AddSuspendedBridgeStructures(cISC4NetworkOccupant*, cSC4NetworkTool::tBridgePiece const&, cSC4NetworkCellInfo*, cSC4NetworkBridgeTypeInfo const&)"},
    {0x1001c38dcu, 177u, "cSC4NetworkTool::InsertBridgePieces(std::__1::list<cISC4NetworkOccupant*, std::__1::allocator<cISC4NetworkOccupant*>>*)"},
    {0x1001c3cacu, 2146u, "cSC4NetworkTool::AdjustBridgeSpan(int&, int&, float&, float&, cISC4NetworkManager::eNetworkType)"},
    {0x1001c450eu, 662u, "cSC4NetworkTool::PlotBridgeSuspensionWires(cSC4NetworkTool::tBridgePiece*, int, int, float, cSC4NetworkTool::BridgeSuspensionMode)"},
    {0x1001c47a4u, 428u, "cSC4NetworkTool::PlotBridgeCables(cSC4NetworkTool::tBridgePiece*, int, int, cS3DVector3 const&, cS3DVector3 const&, cS3DVector3 const&, cS3DVector3 const&, int, unsigned int, float)"},
    {0x1001c4950u, 1776u, "cSC4NetworkTool::PlotBridgeCable(cSC4NetworkTool::tBridgePiece*, int, int, cS3DVector3 const&, cS3DVector3 const&, unsigned int, float)"},
    {0x1001c6648u, 174u, "cSC4NetworkTool::ReadHighwayIntersectionRules(cRZIniFile&, std::__1::unordered_map<unsigned int, nSC4Networks::cIntRule, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<std::__1::pair<unsigned int const, nSC4Networks::cIntRule>>>*)"},
    {0x1001c6a66u, 190u, "cSC4NetworkTool::ReadMultiTypeIntersectionSolutions(cRZIniFile&, cISC4NetworkManager::eNetworkType, cISC4NetworkManager::eNetworkType, std::__1::unordered_map<SC4Point<unsigned int>, cSC4NetworkTool::tMixedTypeIntxSolution, cSC4NetworkTool::MixedTypeIntxSolutionHash, std::__1::equal_to<SC4Point<unsigned int>>, std::__1::allocator<std::__1::pair<SC4Point<unsigned int> const, cSC4NetworkTool::tMixedTypeIntxSolution>>>*)"},
    {0x1002ed400u, 172u, "cSC4NetworkTool::GetFailureReasonString(cIGZString&)"},
    {0x1002ed4e8u, 444u, "cSC4NetworkTool::GetPlacementParams(cISC4NetworkTool::tPlacementParams&)"},
    {0x100415312u, 432u, "cSC4NetworkTool::AddNetworkSideLip(cISC4NetworkOccupant*, float const*, float const*)"},
    {0x1004154c2u, 440u, "cSC4NetworkTool::AddRetainingWall(cISC4NetworkOccupant*, cSC4NetworkTool::tSupportInstance&, float const*, float const*, float)"},
    {0x10041567au, 702u, "cSC4NetworkTool::AddSupportColumnToOccupant(cISC4NetworkOccupant*, float (*) [3], float (*) [3], bool)"},
    {0x100415938u, 530u, "cSC4NetworkTool::AddSupportPylon(cISC4NetworkOccupant*, cSC4NetworkTool::tSupportInstance&, cSC4NetworkCellInfo&)"},
    {0x100415b4au, 1370u, "cSC4NetworkTool::AddSupportStructures(cISC4NetworkOccupant*, cSC4NetworkCellInfo&, cSC4NetworkSupportStructInfo*, int)"},
    {0x100719e10u, 460u, "cSC4NetworkTool::PlaceAllSegments(bool, bool, bool, cISC4NetworkManager::eNetworkType)"},
    {0x100719fdcu, 2286u, "cSC4NetworkTool::PlaceNetwork(bool, cSC4NetworkConstructionCrew*, bool, bool, cISC4NetworkManager::eNetworkType)"},
    {0x10071a8cau, 410u, "cSC4NetworkTool::DrawNetworkLine(SC4Point<unsigned int>&, SC4Point<unsigned int>&, bool, cISC4NetworkManager::eNetworkType)"},
    {0x10071aa64u, 566u, "cSC4NetworkTool::BreakIntoStraightAndDiagSegments(SC4Point<unsigned int>&, SC4Point<unsigned int>&, int&, int&, int&, int&, int&, int&, bool, int, int)"},
    {0x10071ac9au, 4156u, "cSC4NetworkTool::ComputeDraggedCells(SC4Point<unsigned int>&, SC4Point<unsigned int>&, int, int, cISC4NetworkManager::eNetworkType)"},
    {0x10071bcd6u, 230u, "cSC4NetworkTool::TestForAutoConnectOccupant(int, int)"},
    {0x10071c4acu, 872u, "cSC4NetworkTool::IdentifyTunnelCells(int&)"},
    {0x10071c814u, 818u, "cSC4NetworkTool::AssignEdgeMajorTypes(cISC4NetworkManager::eNetworkType)"},
    {0x10071cb46u, 380u, "cSC4NetworkTool::AddGroupingInfo(cISC4NetworkManager::eNetworkType)"},
    {0x10071ccc2u, 252u, "cSC4NetworkTool::DoAdditionalDraws()"},
    {0x10071cdbeu, 1160u, "cSC4NetworkTool::IdentifyHighwayIntersections(int, cISC4NetworkManager::eNetworkType)"},
    {0x10071d246u, 4578u, "cSC4NetworkTool::ResolveOneWayConflicts(cISC4NetworkManager::eNetworkType)"},
    {0x10071e428u, 82u, "cSC4NetworkTool::MarkHighwayIntersectionPieces(bool)"},
    {0x10071e47au, 80u, "cSC4NetworkTool::IdentifyStartingConditions(int&)"},
    {0x10071e4cau, 84u, "cSC4NetworkTool::IsTerrainOK(unsigned int, unsigned int)"},
    {0x10071e51eu, 296u, "cSC4NetworkTool::CheckForReservedTiles(int&, cISC4NetworkManager::eNetworkType, bool)"},
    {0x10071e646u, 394u, "cSC4NetworkTool::CheckForReservedTiles(int, int, cISC4NetworkManager::eNetworkType)"},
    {0x10071e7d0u, 608u, "cSC4NetworkTool::FindTunnelEnd(cSC4NetworkTool::tTunnel&)"},
    {0x10071ea30u, 242u, "cSC4NetworkTool::CheckForGoodTerrain(int&)"},
    {0x10071eb22u, 138u, "cSC4NetworkTool::MarkIntersection(cSC4NetworkTool::tIntersection&, bool)"},
    {0x10071ec4au, 142u, "cSC4NetworkTool::RemoveBridgePiecesFromSetOfCellsToBeSolved(std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>>&) const"},
    {0x10071ecd8u, 252u, "cSC4NetworkTool::RemoveHighwayIntersectionPiecesFromSetOfCellsToBeSolved(std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>>&)"},
    {0x10071edd4u, 234u, "cSC4NetworkTool::RemoveTunnelPiecesFromSetOfCellsToBeSolved(std::__1::unordered_set<unsigned int, std::__1::hash<unsigned int>, std::__1::equal_to<unsigned int>, std::__1::allocator<unsigned int>>&) const"},

    // cSC4ViewInputControlDispatch — 14 entry points
    {0x1003c2520u, 204u, "cSC4ViewInputControlDispatch::cSC4ViewInputControlDispatch(unsigned int)"},
    {0x1003c25f6u, 358u, "cSC4ViewInputControlDispatch::Init()"},
    {0x1003c275cu, 202u, "cSC4ViewInputControlDispatch::UpdateCursorText()"},
    {0x1003c2826u, 338u, "cSC4ViewInputControlDispatch::Shutdown()"},
    {0x1003c2978u, 26u, "cSC4ViewInputControlDispatch::OnKeyDown(unsigned int, unsigned int)"},
    {0x1003c2992u, 846u, "cSC4ViewInputControlDispatch::OnMouseDownL(int, int, unsigned int)"},
    {0x1003c2ce0u, 396u, "cSC4ViewInputControlDispatch::DropActivePylon()"},
    {0x1003c2e6cu, 222u, "cSC4ViewInputControlDispatch::SendDispatchMessage(unsigned int, cISC4PylonOccupant*, unsigned int, unsigned int, unsigned int, unsigned int)"},
    {0x1003c2f4au, 136u, "cSC4ViewInputControlDispatch::PickPylonOccupant(int, int, cISC4PylonOccupant*&)"},
    {0x1003c2fd2u, 174u, "cSC4ViewInputControlDispatch::CalculateDragPylonOffset(cISC4PylonOccupant*, int, int)"},
    {0x1003c3080u, 592u, "cSC4ViewInputControlDispatch::OnMouseMove(int, int, unsigned int)"},
    {0x1003c32d0u, 102u, "cSC4ViewInputControlDispatch::SetHoverPylon(cISC4PylonOccupant*)"},
    {0x1003c33e6u, 46u, "cSC4ViewInputControlDispatch::FinalRelease()"},
    {0x1003c3414u, 404u, "cSC4ViewInputControlDispatch::DoMessage(cIGZMessage2*)"},

    // cSC4WaterPathFinder — 13 entry points
    {0x1007254b6u, 70u, "cSC4WaterPathFinder::cSC4WaterPathFinder()"},
    {0x1007255a0u, 60u, "cSC4WaterPathFinder::FinalRelease()"},
    {0x1007255dcu, 314u, "cSC4WaterPathFinder::Init()"},
    {0x100725716u, 208u, "cSC4WaterPathFinder::Shutdown()"},
    {0x1007257e6u, 124u, "cSC4WaterPathFinder::WaterNodeHashMap::Clear()"},
    {0x100725862u, 482u, "cSC4WaterPathFinder::DoMessage(cIGZMessage2*)"},
    {0x100725a44u, 364u, "cSC4WaterPathFinder::UpdateDepthMap()"},
    {0x100725bb0u, 886u, "cSC4WaterPathFinder::FindPath(int, int, int, int, int&, std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>*, std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>*)"},
    {0x100725f26u, 316u, "cSC4WaterPathFinder::WaterNodeHashMap::GetNode(unsigned short)"},
    {0x100726062u, 96u, "cSC4WaterPathFinder::WaterNodePriorityQueue::Push(cSC4WaterPathFinder::WaterNode*)"},
    {0x1007260c2u, 1062u, "cSC4WaterPathFinder::MakeFinishedPath(cSC4WaterPathFinder::WaterNode*, std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>&, int&, bool)"},
    {0x1007264e8u, 64u, "cSC4WaterPathFinder::WaterNodePriorityQueue::UpdateNode(cSC4WaterPathFinder::WaterNode*)"},
    {0x100726528u, 174u, "cSC4WaterPathFinder::WaterNodeHashMap::WaterNodeHashMap()"},

    // cSC4BuildingOccupant — 12 entry points
    {0x10019ad88u, 136u, "cSC4BuildingOccupant::cSC4BuildingOccupant()"},
    {0x10019ae10u, 43u, "cSC4BuildingOccupant::operator delete(void*)"},
    {0x10019b222u, 44u, "cSC4BuildingOccupant::Init()"},
    {0x10019b430u, 60u, "cSC4BuildingOccupant::GetPosition(float*)"},
    {0x10019b4a6u, 120u, "cSC4BuildingOccupant::SetPosition(float const*)"},
    {0x10019b5eau, 168u, "cSC4BuildingOccupant::GetExemplarName() const"},
    {0x10019b692u, 162u, "cSC4BuildingOccupant::GetBuildingName() const"},
    {0x10019b734u, 212u, "cSC4BuildingOccupant::GetBuildingProfile() const"},
    {0x10019b84eu, 194u, "cSC4BuildingOccupant::SetLit(bool)"},
    {0x10019b910u, 118u, "cSC4BuildingOccupant::SetName(cRZString&)"},
    {0x10019b986u, 86u, "cSC4BuildingOccupant::GetName(cRZString&)"},
    {0x10019b9fau, 342u, "cSC4BuildingOccupant::PostOccupantMessage(unsigned int, unsigned int)"},

    // cSC4ScrubberEffect — 12 entry points
    {0x100573e02u, 190u, "cSC4ScrubberEffect::cSC4ScrubberEffect(cSC4ScrubberDescription*)"},
    {0x100573f78u, 138u, "cSC4ScrubberEffect::Start(cISC4VisualEffect::tTransitionType)"},
    {0x100574002u, 140u, "cSC4ScrubberEffect::Stop(cISC4VisualEffect::tTransitionType)"},
    {0x100574098u, 574u, "cSC4ScrubberEffect::SetTransforms(cS3DTransform const&, cS3DTransform const&)"},
    {0x1005742f4u, 110u, "cSC4ScrubberEffect::ApplyEffect(float)"},
    {0x100574362u, 324u, "cSC4ScrubberEffect::DoScrubberEffect()"},
    {0x1005744a6u, 178u, "cSC4ScrubberEffect::DoBurn()"},
    {0x100574558u, 78u, "cSC4ScrubberEffect::DoToxic()"},
    {0x1005745a6u, 892u, "cSC4ScrubberEffect::DoDemolish()"},
    {0x100574922u, 160u, "cSC4ScrubberEffect::DoExtinguish()"},
    {0x1005749c2u, 208u, "cSC4ScrubberEffect::RemoveLastBlob()"},
    {0x100574a92u, 230u, "cSC4ScrubberEffect::AddBlobToMap()"},

    // cSC4ParkManager — 11 entry points
    {0x1000c8f7eu, 254u, "cSC4ParkManager::cSC4ParkManager()"},
    {0x1000c911cu, 492u, "cSC4ParkManager::Init()"},
    {0x1000c9308u, 348u, "cSC4ParkManager::ReadTuningParams()"},
    {0x1000c9464u, 270u, "cSC4ParkManager::Shutdown()"},
    {0x1000c9572u, 50u, "cSC4ParkManager::ReleaseOccupantList(ilist<cISC4Occupant*>&)"},
    {0x1000c97a0u, 162u, "cSC4ParkManager::DoMessage(cIGZMessage2*)"},
    {0x1000c9842u, 258u, "cSC4ParkManager::DoMonthlySimulation()"},
    {0x1000c9944u, 526u, "cSC4ParkManager::DoOccupantInserted(cISC4Occupant*)"},
    {0x1000c9b52u, 302u, "cSC4ParkManager::DoOccupantRemoved(cISC4Occupant*)"},
    {0x1000c9c92u, 150u, "cSC4ParkManager::GetLotWhereOccupantResides(cISC4Occupant*)"},
    {0x1000c9d28u, 370u, "cSC4ParkManager::BeginZooEscape()"},

    // cSC4WinNetworkLotChooser — 11 entry points
    {0x1003d25ccu, 132u, "cSC4WinNetworkLotChooser::cSC4WinNetworkLotChooser()"},
    {0x1003d2768u, 170u, "cSC4WinNetworkLotChooser::Init()"},
    {0x1003d2812u, 534u, "cSC4WinNetworkLotChooser::PostInit()"},
    {0x1003d2a28u, 782u, "cSC4WinNetworkLotChooser::SetupLotConfigurationList()"},
    {0x1003d2d36u, 488u, "cSC4WinNetworkLotChooser::Shutdown()"},
    {0x1003d2f1eu, 352u, "cSC4WinNetworkLotChooser::DoMessage(cGZMessage&)"},
    {0x1003d307eu, 94u, "cSC4WinNetworkLotChooser::DoRequestClose()"},
    {0x1003d30dcu, 926u, "cSC4WinNetworkLotChooser::DoRequestEditLotConfiguration(bool)"},
    {0x1003d347au, 180u, "cSC4WinNetworkLotChooser::GetSelectedLotConfigurationType()"},
    {0x1003d352eu, 164u, "cSC4WinNetworkLotChooser::IsSelectedLotBeingEdited()"},
    {0x1003d35d2u, 112u, "cSC4WinNetworkLotChooser::DoLotConfigurationEditorClosed(cSC4WinNetworkLotEditor*)"},

    // cSC4AircraftManager — 11 entry points
    {0x10050044cu, 178u, "cSC4AircraftManager::cSC4AircraftManager()"},
    {0x1005004feu, 146u, "cSC4AircraftManager::Init()"},
    {0x100500590u, 240u, "cSC4AircraftManager::Shutdown()"},
    {0x100500a48u, 172u, "cSC4AircraftManager::DoMessage(cIGZMessage2*)"},
    {0x100500af4u, 1046u, "cSC4AircraftManager::HandleOccupantInserted(cISC4Occupant*)"},
    {0x100500f0au, 444u, "cSC4AircraftManager::HandleOccupantRemoved(cISC4Occupant*)"},
    {0x1005010dcu, 114u, "cSC4AircraftManager::AddAutomaton(cISC4Automaton*)"},
    {0x10050114eu, 200u, "cSC4AircraftManager::HandleTuningParametersChanged()"},
    {0x1005012a0u, 700u, "cSC4AircraftManager::Dispatch(unsigned int, tDispatchLocation const&, tDispatchLocation const&, unsigned int, unsigned char, cGZPersistResourceKey const&, unsigned char)"},
    {0x10050155cu, 510u, "cSC4AircraftManager::Dispatch(cISC4Automaton*, tDispatchLocation const&, tDispatchLocation const&, unsigned int, unsigned char)"},
    {0x10050175au, 432u, "cSC4AircraftManager::GetRunwayList(cS3DVector3 const&, std::__1::vector<cS3DVector3, std::__1::allocator<cS3DVector3>>&)"},

    // cSC4VehicleManager — 10 entry points
    {0x1000d9c40u, 812u, "cSC4VehicleManager::cSC4VehicleManager()"},
    {0x1000d9f76u, 208u, "cSC4VehicleManager::Init()"},
    {0x1000da046u, 224u, "cSC4VehicleManager::Shutdown()"},
    {0x1000da13eu, 746u, "cSC4VehicleManager::DoMessage(cIGZMessage2*)"},
    {0x1000da428u, 1102u, "cSC4VehicleManager::HandleFundsChanged()"},
    {0x1000da884u, 50u, "cSC4VehicleManager::HandleTuningParametersChanged()"},
    {0x1000da8b6u, 166u, "cSC4VehicleManager::HandleRenderPropertiesChanged()"},
    {0x1000da9a0u, 462u, "cSC4VehicleManager::HandleNewBuilding(cISC4BuildingOccupant*)"},
    {0x1000dab6eu, 130u, "cSC4VehicleManager::AddAutomaton(cISC4Automaton*)"},
    {0x1000dac3au, 46u, "cSC4VehicleManager::DoAnimationTick(unsigned int, float)"},

    // cSC4PrimitivePool — 10 entry points
    {0x1005fd0deu, 102u, "cSC4PrimitivePool::QuadVertexInit_V3F_C4UB_T2F(void*, unsigned int, unsigned int)"},
    {0x1005fd144u, 100u, "cSC4PrimitivePool::VFlip_QuadVertexInit_V3F_C4UB_T2F(void*, unsigned int, unsigned int)"},
    {0x1005fd1a8u, 104u, "cSC4PrimitivePool::HFlip_QuadVertexInit_V3F_C4UB_T2F(void*, unsigned int, unsigned int)"},
    {0x1005fd210u, 82u, "cSC4PrimitivePool::QuadTriListIndexInit(unsigned short*, unsigned int, unsigned int)"},
    {0x1005fd262u, 72u, "cSC4PrimitivePool::cSC4PrimitivePool()"},
    {0x1005fd2aau, 160u, "cSC4PrimitivePool::cSC4PrimitivePool(cSC4PrimitivePool const&)"},
    {0x1005fd3d0u, 208u, "cSC4PrimitivePool::Draw(SC4DrawContext*)"},
    {0x1005fd4a0u, 88u, "cSC4PrimitivePool::SetFormat(eGDPrimType, unsigned int, unsigned int, eGDVertexFormat, void (*)(void*, unsigned int, unsigned int), void (*)(unsigned short*, unsigned int, unsigned int))"},
    {0x1005fd516u, 104u, "cSC4PrimitivePool::GetPrimVertices(unsigned int)"},
    {0x1005fd57eu, 116u, "cSC4PrimitivePool::GetPrimIndices(unsigned int)"},

    // cSC4PathView — 10 entry points
    {0x10077c4acu, 112u, "cSC4PathView::cSC4PathView()"},
    {0x10077c60cu, 240u, "cSC4PathView::Init()"},
    {0x10077c70cu, 174u, "cSC4PathView::Update(SC4DrawContext*)"},
    {0x10077c7bau, 1910u, "cSC4PathView::DrawPath(SC4DrawContext*, cSC4PathView::PathInfo*)"},
    {0x10077cf3cu, 32u, "cSC4PathView::ClearPaths()"},
    {0x10077cf5cu, 222u, "cSC4PathView::NewPath(cRZColor, float, unsigned int)"},
    {0x10077d0f6u, 114u, "cSC4PathView::SetPathTexture(unsigned int, float)"},
    {0x10077d168u, 172u, "cSC4PathView::AddPathVertex(cS3DVector3 const&)"},
    {0x10077d2c6u, 354u, "cSC4PathView::CalcMiterJoint(cS3DVector3 const&, cS3DVector3 const&, cS3DVector3 const&, cS3DVector3&, cS3DVector3&)"},
    {0x10077d428u, 600u, "cSC4PathView::DrawArrowhead(SC4DrawContext*, cS3DVector3&, cS3DVector3&, unsigned int)"},

    // cSC4ViewInputControlNetworkTool — 9 entry points
    {0x10038dea0u, 172u, "cSC4ViewInputControlNetworkTool::cSC4ViewInputControlNetworkTool(cISC4NetworkManager::eNetworkType)"},
    {0x10038e0b8u, 374u, "cSC4ViewInputControlNetworkTool::Init()"},
    {0x10038e22eu, 278u, "cSC4ViewInputControlNetworkTool::Shutdown()"},
    {0x10038e344u, 80u, "cSC4ViewInputControlNetworkTool::OnKeyDown(unsigned int, unsigned int)"},
    {0x10038e394u, 152u, "cSC4ViewInputControlNetworkTool::OnMouseDownL(int, int, unsigned int)"},
    {0x10038e42cu, 142u, "cSC4ViewInputControlNetworkTool::OnMouseUpL(int, int, unsigned int)"},
    {0x10038e4bau, 922u, "cSC4ViewInputControlNetworkTool::OnMouseMove(int, int, unsigned int)"},
    {0x10038e854u, 98u, "cSC4ViewInputControlNetworkTool::OnMouseExit()"},
    {0x10038e8b6u, 120u, "cSC4ViewInputControlNetworkTool::DoMessage(cIGZMessage2*)"},

    // cSC4DecalEffect — 9 entry points
    {0x1005f0fd0u, 332u, "cSC4DecalEffect::cSC4DecalEffect()"},
    {0x1005f125au, 82u, "cSC4DecalEffect::Init()"},
    {0x1005f1314u, 1432u, "cSC4DecalEffect::Start(cISC4VisualEffect::tTransitionType)"},
    {0x1005f18acu, 96u, "cSC4DecalEffect::Stop(cISC4VisualEffect::tTransitionType)"},
    {0x1005f1916u, 638u, "cSC4DecalEffect::SetTransforms(cS3DTransform const&, cS3DTransform const&)"},
    {0x1005f1b94u, 1488u, "cSC4DecalEffect::Update(float)"},
    {0x1005f2164u, 194u, "cSC4DecalEffect::SetFloatParams(cISC4VisualEffect::tParamType, float const*, int)"},
    {0x1005f2226u, 36u, "cSC4DecalEffect::SetVoidParams(cISC4VisualEffect::tParamType, void**, int)"},
    {0x1005f224au, 638u, "cSC4DecalEffect::SetDescription(cSC4DecalDescription const&)"},

    // cSC4BudgetUIIncomeWinProc — 8 entry points
    {0x100241e00u, 268u, "cSC4BudgetUIIncomeWinProc::cSC4BudgetUIIncomeWinProc()"},
    {0x100242076u, 406u, "cSC4BudgetUIIncomeWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x10024220cu, 300u, "cSC4BudgetUIIncomeWinProc::InitDialog()"},
    {0x100242338u, 140u, "cSC4BudgetUIIncomeWinProc::EndDialog()"},
    {0x1002423c4u, 456u, "cSC4BudgetUIIncomeWinProc::SetInfoPerSpinner(unsigned int)"},
    {0x1002425aau, 388u, "cSC4BudgetUIIncomeWinProc::SetInfoPerTaxType(cISC4BudgetSimulator::TaxType)"},
    {0x10024272eu, 236u, "cSC4BudgetUIIncomeWinProc::SetFormattedAmountInTextControl(unsigned int, long long)"},
    {0x10024281au, 278u, "cSC4BudgetUIIncomeWinProc::RecalculateTotals()"},

    // cSC4ViewInputControlNetworkIntxTool — 8 entry points
    {0x10038e940u, 268u, "cSC4ViewInputControlNetworkIntxTool::cSC4ViewInputControlNetworkIntxTool(unsigned int)"},
    {0x10038ea56u, 160u, "cSC4ViewInputControlNetworkIntxTool::Init()"},
    {0x10038eaf6u, 226u, "cSC4ViewInputControlNetworkIntxTool::Shutdown()"},
    {0x10038ebd8u, 342u, "cSC4ViewInputControlNetworkIntxTool::OnKeyDown(unsigned int, unsigned int)"},
    {0x10038ed2eu, 406u, "cSC4ViewInputControlNetworkIntxTool::OnMouseDownL(int, int, unsigned int)"},
    {0x10038eec4u, 158u, "cSC4ViewInputControlNetworkIntxTool::OnMouseUpL(int, int, unsigned int)"},
    {0x10038ef62u, 3168u, "cSC4ViewInputControlNetworkIntxTool::OnMouseMove(int, int, unsigned int)"},
    {0x10038fbc2u, 174u, "cSC4ViewInputControlNetworkIntxTool::OnMouseExit()"},

    // cSCGeneralResponseCurveEntryView — 8 entry points
    {0x100529c00u, 82u, "cSCGeneralResponseCurveEntryView::cSCGeneralResponseCurveEntryView()"},
    {0x100529d5cu, 2604u, "cSCGeneralResponseCurveEntryView::Init()"},
    {0x10052a7b4u, 1086u, "cSCGeneralResponseCurveEntryView::UpdateCurveDisplay()"},
    {0x10052abf2u, 114u, "cSCGeneralResponseCurveEntryView::GZPaint()"},
    {0x10052ac7cu, 1136u, "cSCGeneralResponseCurveEntryView::DoButtonPress(unsigned int)"},
    {0x10052b108u, 158u, "cSCGeneralResponseCurveEntryView::GetValue(cIGZVariant&)"},
    {0x10052b1b8u, 240u, "cSCGeneralResponseCurveEntryView::GetPropertyValueString(unsigned int, cIGZVariant const&, cIGZString&)"},
    {0x10052b5a2u, 300u, "cSCGeneralResponseCurveEntryView::SetCurrentPoint(float, float)"},

    // cSC4ControllerGrid — 7 entry points
    {0x1000f4322u, 150u, "cSC4ControllerGrid::Init()"},
    {0x1000f47cau, 276u, "cSC4ControllerGrid::AddController(cISC4AutomataController*)"},
    {0x1000f5f7cu, 114u, "cSC4ControllerGrid::RemoveController(cISC4AutomataController*)"},
    {0x1000f64f6u, 538u, "cSC4ControllerGrid::MoveController(cISC4AutomataController*)"},
    {0x1000f6e10u, 444u, "cSC4ControllerGrid::GetGridBounds(cS3DVector3 const&, float, SC4Rect<unsigned char>&)"},
    {0x1000f74a2u, 174u, "cSC4ControllerGrid::GetTractByWorldPos(float, float)"},
    {0x1000f8452u, 350u, "cSC4ControllerGrid::GetGridBounds(cISC4AutomataController*, SC4Rect<unsigned char>&)"},

    // cSC4ProgressScreen — 7 entry points
    {0x10032cce0u, 492u, "cSC4ProgressScreen::cSC4ProgressScreen(cRZRect const&, bool)"},
    {0x10032d0a8u, 240u, "cSC4ProgressScreen::Shutdown()"},
    {0x10032d23eu, 1236u, "cSC4ProgressScreen::Init()"},
    {0x10032d812u, 226u, "cSC4ProgressScreen::SetCaption(unsigned int, cIGZString const&)"},
    {0x10032d8f4u, 1714u, "cSC4ProgressScreen::Repaint()"},
    {0x10032dfa6u, 546u, "cSC4ProgressScreen::DoMessage(cIGZMessage2*)"},
    {0x10032e1c8u, 532u, "cSC4ProgressScreen::RepaintForObliterationProgress()"},

    // cSC4AdvisorCityPlanning — 7 entry points
    {0x100400b56u, 250u, "cSC4AdvisorCityPlanning::update_occupant_variables()"},
    {0x100400c50u, 4906u, "cSC4AdvisorCityPlanning::update_variables()"},
    {0x100401f7au, 728u, "cSC4AdvisorCityPlanning::DoMessage(cIGZMessage2*)"},
    {0x100402260u, 68u, "cSC4AdvisorCityPlanning::GetLandmarkCount(int)"},
    {0x1004022a4u, 70u, "cSC4AdvisorCityPlanning::HasLandmark(int)"},
    {0x1004022eau, 222u, "cSC4AdvisorCityPlanning::Init(int)"},
    {0x1004023d6u, 200u, "cSC4AdvisorCityPlanning::Shutdown()"},

    // cSC4DisasterManagerRiot — 7 entry points
    {0x10044cba4u, 178u, "cSC4DisasterManagerRiot::AttemptDisasterStart(int, int)"},
    {0x10044d166u, 32u, "cSC4DisasterManagerRiot::Init()"},
    {0x10044d186u, 138u, "cSC4DisasterManagerRiot::ReadTuningParameters()"},
    {0x10044d234u, 38u, "cSC4DisasterManagerRiot::Shutdown()"},
    {0x10044d25au, 188u, "cSC4DisasterManagerRiot::ClearRiotNetworkOccupants()"},
    {0x10044d324u, 278u, "cSC4DisasterManagerRiot::AddRiotNetworkOccupant(cISC4NetworkOccupant*)"},
    {0x10044d43au, 42u, "cSC4DisasterManagerRiot::RemoveInstanceFromList(cISC4DisasterInstance*)"},

    // cSC4MyStreetSimInputControl — 7 entry points
    {0x1006bbc4cu, 48u, "cSC4MyStreetSimInputControl::cSC4MyStreetSimInputControl()"},
    {0x1006bbd22u, 34u, "cSC4MyStreetSimInputControl::Init()"},
    {0x1006bbd44u, 114u, "cSC4MyStreetSimInputControl::Init(cSC4MyStreetSim*)"},
    {0x1006bbdb6u, 50u, "cSC4MyStreetSimInputControl::Shutdown()"},
    {0x1006bbdf0u, 968u, "cSC4MyStreetSimInputControl::OnMouseMove(int, int, unsigned int)"},
    {0x1006bc1b8u, 120u, "cSC4MyStreetSimInputControl::OnMouseDownL(int, int, unsigned int)"},
    {0x1006bc230u, 26u, "cSC4MyStreetSimInputControl::OnKeyDown(unsigned int, unsigned int)"},

    // cSCParticleSimulatorBase — 7 entry points
    {0x100702942u, 62u, "cSCParticleSimulatorBase::cSCParticleSimulatorBase()"},
    {0x1007029ecu, 46u, "cSCParticleSimulatorBase::Shutdown()"},
    {0x100702a4cu, 120u, "cSCParticleSimulatorBase::Stop(bool, bool)"},
    {0x100702aceu, 74u, "cSCParticleSimulatorBase::CheckForStop()"},
    {0x100702b18u, 72u, "cSCParticleSimulatorBase::AddParticle(cSCParticle*)"},
    {0x100702b6au, 84u, "cSCParticleSimulatorBase::CreateParticle()"},
    {0x100702c06u, 42u, "cSCParticleSimulatorBase::FinalRelease()"},

    // cSC4ViewInputControlEffectDisaster — 6 entry points
    {0x1003bb280u, 204u, "cSC4ViewInputControlEffectDisaster::cSC4ViewInputControlEffectDisaster(cISCResExemplar*)"},
    {0x1003bb404u, 144u, "cSC4ViewInputControlEffectDisaster::Init()"},
    {0x1003bb494u, 124u, "cSC4ViewInputControlEffectDisaster::Shutdown()"},
    {0x1003bb510u, 72u, "cSC4ViewInputControlEffectDisaster::OnMouseUpL(int, int, unsigned int)"},
    {0x1003bb558u, 212u, "cSC4ViewInputControlEffectDisaster::SetTerrainControlledEffectState(bool)"},
    {0x1003bb62cu, 142u, "cSC4ViewInputControlEffectDisaster::SetDisasterManager(unsigned int)"},

    // cSC4ToxicOccupant — 6 entry points
    {0x1006c78eeu, 142u, "cSC4ToxicOccupant::cSC4ToxicOccupant()"},
    {0x1006c7a80u, 232u, "cSC4ToxicOccupant::Init()"},
    {0x1006c7b68u, 98u, "cSC4ToxicOccupant::UpdateVisualEffectTransform()"},
    {0x1006c7bdcu, 96u, "cSC4ToxicOccupant::Shutdown()"},
    {0x1006c7c70u, 184u, "cSC4ToxicOccupant::SetCell(int, int)"},
    {0x1006c7d28u, 110u, "cSC4ToxicOccupant::GetBoundingBox(float*, float*)"},

    // cSC4MySimPool — 6 entry points
    {0x1007ad72cu, 84u, "cSC4MySimPool::cSC4MySimPool()"},
    {0x1007ad80cu, 858u, "cSC4MySimPool::EnumKeysCallback(cGZPersistResourceKey const&, void*)"},
    {0x1007adb66u, 142u, "cSC4MySimPool::GetMySimTemplates(std::__1::vector<cISC4MySimPool::tMySimTemplate, std::__1::allocator<cISC4MySimPool::tMySimTemplate>>&)"},
    {0x1007adbf4u, 1600u, "cSC4MySimPool::LoadImports()"},
    {0x1007ae234u, 318u, "cSC4MySimPool::FindTheSimsInstallation()"},
    {0x1007ae372u, 8782u, "cSC4MySimPool::Import(unsigned int&)"},

    // cSC4NetworkBridgeOccupant — 5 entry points
    {0x1001d25e6u, 178u, "cSC4NetworkBridgeOccupant::cSC4NetworkBridgeOccupant()"},
    {0x1001d2698u, 35u, "cSC4NetworkBridgeOccupant::operator delete(void*)"},
    {0x1001d291eu, 48u, "cSC4NetworkBridgeOccupant::Init()"},
    {0x1001d297cu, 80u, "cSC4NetworkBridgeOccupant::Shutdown()"},
    {0x1001d29dau, 238u, "cSC4NetworkBridgeOccupant::UpdateBaseModel(cIS3DModelInstance*)"},

    // cSC4AudioEmanationModel2D — 5 entry points
    {0x1003f301eu, 96u, "cSC4AudioEmanationModel2D::Init()"},
    {0x1003f308cu, 34u, "cSC4AudioEmanationModel2D::Shutdown()"},
    {0x1003f30d0u, 200u, "cSC4AudioEmanationModel2D::UpdateVolPan(cIGZSndRegisterSet*, cIGZSndRegisterSet*) const"},
    {0x1003f3198u, 392u, "cSC4AudioEmanationModel2D::ConfigureChannel(cIGZSndRegisterSet*, cIGZSndRegisterSet*) const"},
    {0x1003f475au, 28u, "cSC4AudioEmanationModel2D::GetName(cIGZString&) const"},

    // cSC4AdvisorSafety — 5 entry points
    {0x100449d96u, 2070u, "cSC4AdvisorSafety::update_occupant_variables()"},
    {0x10044a5acu, 886u, "cSC4AdvisorSafety::update_variables()"},
    {0x10044a922u, 3470u, "cSC4AdvisorSafety::DoMessage(cIGZMessage2*)"},
    {0x10044b6beu, 376u, "cSC4AdvisorSafety::Init(int)"},
    {0x10044b844u, 336u, "cSC4AdvisorSafety::Shutdown()"},

    // cSC4SoundEffect — 5 entry points
    {0x1005725beu, 174u, "cSC4SoundEffect::cSC4SoundEffect(cSC4SoundDescription*)"},
    {0x100572750u, 132u, "cSC4SoundEffect::Start(cISC4VisualEffect::tTransitionType)"},
    {0x1005727d4u, 86u, "cSC4SoundEffect::Stop(cISC4VisualEffect::tTransitionType)"},
    {0x100572834u, 96u, "cSC4SoundEffect::SetTransforms(cS3DTransform const&, cS3DTransform const&)"},
    {0x1005728a4u, 150u, "cSC4SoundEffect::ApplyEffect(float)"},

    // cSC4LocalGridHelper — 4 entry points
    {0x1000a98ceu, 42u, "cSC4LocalGridHelper::Init()"},
    {0x1000a98f8u, 72u, "cSC4LocalGridHelper::Shutdown()"},
    {0x1000a9940u, 364u, "cSC4LocalGridHelper::SetPosition(cS3DVector3 const&)"},
    {0x1000a9aacu, 100u, "cSC4LocalGridHelper::ShowEffect(bool)"},

    // cSC4RegionalTerrainMap — 4 entry points
    {0x100234bbcu, 62u, "cSC4RegionalTerrainMap::LocationIsInBounds(float, float) const"},
    {0x100234c1eu, 86u, "cSC4RegionalTerrainMap::GetAltitudes(SC4Rect<int> const&, float*, long, long)"},
    {0x100234c74u, 78u, "cSC4RegionalTerrainMap::GetWaterDepths(SC4Rect<int> const&, float*, long, long)"},
    {0x100234d28u, 124u, "cSC4RegionalTerrainMap::GetTerrainGridIndex(float, float)"},

    // cSC4PlacedNetworkOccupantFilter — 4 entry points
    {0x10071eebeu, 102u, "cSC4PlacedNetworkOccupantFilter::cSC4PlacedNetworkOccupantFilter(cISC4OccupantFilter*)"},
    {0x10071ef24u, 46u, "cSC4PlacedNetworkOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    {0x10071ef52u, 18u, "cSC4PlacedNetworkOccupantFilter::IsPropertyHolderIncluded(cISCPropertyHolder*)"},
    {0x10071f042u, 122u, "cSC4PlacedNetworkOccupantFilter::IsOccPlaced(cISC4Occupant*)"},

    // cSC4PedestrianOneShot — 3 entry points
    {0x1000f3884u, 142u, "cSC4PedestrianOneShot::cSC4PedestrianOneShot(cSC4PedestrianManager*, cGZPersistResourceKey)"},
    {0x1000f3912u, 288u, "cSC4PedestrianOneShot::Init()"},
    {0x1000f3aaeu, 210u, "cSC4PedestrianOneShot::DoAnimationTick(unsigned int, float)"},

    // cSC4SubwayOccupantFilter — 3 entry points
    {0x100192d40u, 146u, "cSC4SubwayOccupantFilter::IsOccupantSubwayStation(cISC4Occupant*)"},
    {0x100192dd2u, 230u, "cSC4SubwayOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    {0x100192eb8u, 166u, "cSC4SubwayOccupantFilter::IsPropertyHolderIncluded(cISCPropertyHolder*)"},

    // cSC4NetworkRoutines — 3 entry points
    {0x1002ee7cau, 60u, "cSC4NetworkRoutines::Get0To3Direction(unsigned int, unsigned int, unsigned int, unsigned int)"},
    {0x1002ee806u, 998u, "cSC4NetworkRoutines::ComputeDemolitionRegionForPlacementCells(std::__1::vector<SC4Point<int>, std::__1::allocator<SC4Point<int>>> const&, SC4CellRegion<int>*&, long long&, std::__1::set<cRZAutoRefCount<cISC4Occupant>, std::__1::less<cISC4Occupant*>, std::__1::allocator<cRZAutoRefCount<cISC4Occupant>>>&, cISC4OccupantFilter*)"},
    {0x1002eebecu, 542u, "cSC4NetworkRoutines::UpdateHighlights(std::__1::set<cRZAutoRefCount<cISC4Occupant>, std::__1::less<cISC4Occupant*>, std::__1::allocator<cRZAutoRefCount<cISC4Occupant>>> const&, std::__1::set<cRZAutoRefCount<cISC4Occupant>, std::__1::less<cISC4Occupant*>, std::__1::allocator<cRZAutoRefCount<cISC4Occupant>>> const&)"},

    // cSC4AudioSemManager — 3 entry points
    {0x1003f4020u, 1124u, "cSC4AudioSemManager::Init()"},
    {0x1003f4484u, 486u, "cSC4AudioSemManager::Shutdown()"},
    {0x1003f46b6u, 60u, "cSC4AudioSemManager::Create()"},

    // cSC4Blimp — 3 entry points
    {0x1006d65bau, 134u, "cSC4Blimp::cSC4Blimp()"},
    {0x1006d6640u, 64u, "cSC4Blimp::Init()"},
    {0x1006d669au, 246u, "cSC4Blimp::GetNewDirection(cS3DVector3 const&, cS3DVector3&)"},

    // cSC4AudioAEMSSoundBank — 3 entry points
    {0x1007b547cu, 58u, "cSC4AudioAEMSSoundBank::cSC4AudioAEMSSoundBank()"},
    {0x1007b54b6u, 164u, "cSC4AudioAEMSSoundBank::LoadFromResource(cGZPersistResourceKey&)"},
    {0x1007b555au, 138u, "cSC4AudioAEMSSoundBank::LoadFromFile(cIGZFile*)"},

    // cTutorialString — 2 entry points
    {0x10011e012u, 72u, "cTutorialString::FromScriptText(char const*)"},
    {0x10011f384u, 180u, "cTutorialString::init(char const*, int)"},

    // cSC4WaterOccupantFilter — 2 entry points
    {0x100193514u, 144u, "cSC4WaterOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    {0x1001935a4u, 102u, "cSC4WaterOccupantFilter::IsPropertyHolderIncluded(cISCPropertyHolder*)"},

    // cSC4ZoomRotationModelChangeOccupantFilter — 2 entry points
    {0x1002c119au, 60u, "cSC4ZoomRotationModelChangeOccupantFilter::cSC4ZoomRotationModelChangeOccupantFilter()"},
    {0x1002c11f6u, 62u, "cSC4ZoomRotationModelChangeOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},

    // cSC4SubwayConnectionOccupant — 2 entry points
    {0x1003de210u, 126u, "cSC4SubwayConnectionOccupant::cSC4SubwayConnectionOccupant()"},
    {0x1003de44cu, 138u, "cSC4SubwayConnectionOccupant::QueryOccupantInterface(cISC4Occupant*, cSC4UndergroundOccupant**)"},

    // cAmbienceOccupantFilterIsNature — 2 entry points
    {0x1003df236u, 50u, "cAmbienceOccupantFilterIsNature::Create()"},
    {0x1003e5186u, 132u, "cAmbienceOccupantFilterIsNature::IsTrueOccupant(cISC4Occupant*)"},

    // cAmbienceOccupantFilterIsVehicle — 2 entry points
    {0x1003df2feu, 50u, "cAmbienceOccupantFilterIsVehicle::Create()"},
    {0x1003e520au, 76u, "cAmbienceOccupantFilterIsVehicle::IsTrueOccupant(cISC4Occupant*)"},

    // cAmbienceOccupantFilterIsTaxi — 2 entry points
    {0x1003df3c6u, 50u, "cAmbienceOccupantFilterIsTaxi::Create()"},
    {0x1003e53ceu, 76u, "cAmbienceOccupantFilterIsTaxi::IsTrueOccupant(cISC4Occupant*)"},

    // cAmbienceOccupantFilterIsTvReporter — 2 entry points
    {0x1003df48eu, 50u, "cAmbienceOccupantFilterIsTvReporter::Create()"},
    {0x1003e54feu, 76u, "cAmbienceOccupantFilterIsTvReporter::IsTrueOccupant(cISC4Occupant*)"},

    // cAmbienceOccupantFilterIsGarbageTruck — 2 entry points
    {0x1003df556u, 50u, "cAmbienceOccupantFilterIsGarbageTruck::Create()"},
    {0x1003e5648u, 76u, "cAmbienceOccupantFilterIsGarbageTruck::IsTrueOccupant(cISC4Occupant*)"},

    // cAmbienceOccupantFilterIsAgriculture — 2 entry points
    {0x1003df61eu, 50u, "cAmbienceOccupantFilterIsAgriculture::Create()"},
    {0x1003e580eu, 332u, "cAmbienceOccupantFilterIsAgriculture::IsTrueOccupant(cISC4Occupant*)"},

    // cAmbienceOccupantFilterIsLowWealth — 2 entry points
    {0x1003df6e6u, 50u, "cAmbienceOccupantFilterIsLowWealth::Create()"},
    {0x1003e59aau, 162u, "cAmbienceOccupantFilterIsLowWealth::IsTrueOccupant(cISC4Occupant*)"},

    // cAmbienceCellFilterIsLandfill — 2 entry points
    {0x1003dfa06u, 50u, "cAmbienceCellFilterIsLandfill::Create()"},
    {0x1003e624eu, 92u, "cAmbienceCellFilterIsLandfill::IsTrueCell(int, int)"},

    // cSC4AdviceNeghborDeal — 2 entry points
    {0x1004412c6u, 154u, "cSC4AdviceNeghborDeal::Init(int)"},
    {0x1004413ccu, 148u, "cSC4AdviceNeghborDeal::Clone(cISC4Advice**)"},

    // cSC4AdvisorCitySituations — 2 entry points
    {0x1006a7db0u, 396u, "cSC4AdvisorCitySituations::UpdateMySimAdvices()"},
    {0x1006a7f3cu, 248u, "cSC4AdvisorCitySituations::RetireAdvice(cISC4Advice*)"},

    // cSC4NetworkLayoutCustomView — 2 entry points
    {0x100714034u, 1840u, "cSC4NetworkLayoutCustomView::InitCustomView()"},
    {0x100714764u, 138u, "cSC4NetworkLayoutCustomView::ShutdownCustomView()"},

    // SC4TutorialPreferences2 — 1 entry point
    {0x10001393cu, 68u, "SC4TutorialPreferences2::SC4TutorialPreferences2()"},

    // cSC4AutoGamePause — 1 entry point
    {0x100039240u, 54u, "cSC4AutoGamePause::cSC4AutoGamePause(bool, bool, bool, bool)"},

    // cSC4NetworkOccupantFilter — 1 entry point
    {0x1000a6c3eu, 118u, "cSC4NetworkOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},

    // cSC4LotPropFilter — 1 entry point
    {0x1000cb624u, 162u, "cSC4LotPropFilter::IsOccupantIncluded(cISC4Occupant*)"},

    // tAutomataState — 1 entry point
    {0x10010e128u, 60u, "tAutomataState::Shutdown(cISC4Automaton*)"},

    // cSC4SafeListIterator<cISC4Lot*> — 1 entry point
    {0x100132e98u, 140u, "cSC4SafeListIterator<cISC4Lot*>::AddAfter(cISC4Lot* const&)"},

    // cSC4TileNeighborhood — 1 entry point
    {0x1001af142u, 142u, "cSC4TileNeighborhood::GetCompactRepresentation(unsigned char, cSC4TileNeighborhoodCompact&)"},

    // cAutomataOccupantFilter — 1 entry point
    {0x1001ea1aau, 86u, "cAutomataOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},

    // cSC4RegionViewCity — 1 entry point
    {0x10023e27au, 596u, "cSC4RegionViewCity::cSC4RegionViewCity(cSC4RegionViewCity const&)"},

    // cSC4HoverQueriableOccupantFilter — 1 entry point
    {0x1002dbef4u, 122u, "cSC4HoverQueriableOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},

    // cSC4DemolishByNetworksOccupantFilter — 1 entry point
    {0x1002eeec6u, 96u, "cSC4DemolishByNetworksOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},

    // cSC4LotConfigurationFilterMonopoly — 1 entry point
    {0x1003c1da4u, 352u, "cSC4LotConfigurationFilterMonopoly::IsLotConfigurationIncluded(cISC4LotConfiguration*)"},

    // cSC4LotConfigurationEditor — 1 entry point
    {0x1004160a4u, 60u, "cSC4LotConfigurationEditor::Shutdown()"},

    // cSC4WateredPipeFilter — 1 entry point
    {0x1004568bau, 140u, "cSC4WateredPipeFilter::IsOccupantIncluded(cISC4Occupant*)"},

    // cSC4SurfaceNetworkOccupantFilter — 1 entry point
    {0x1005c7782u, 70u, "cSC4SurfaceNetworkOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},

    // cSC4ActiveToxicOccupantFilter — 1 entry point
    {0x1006c78d0u, 30u, "cSC4ActiveToxicOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},

    // cSC4Ambulance — 1 entry point
    {0x1006d5e3cu, 108u, "cSC4Ambulance::cSC4Ambulance()"},

    // cSC4CachedStringTexture — 1 entry point
    {0x10070dec8u, 78u, "cSC4CachedStringTexture::SetManager(cSC4GlyphTextureManager*)"},

    // cMySimBuildingFilter — 1 entry point
    {0x1007aaf18u, 240u, "cMySimBuildingFilter::IsOccupantIncluded(cISC4Occupant*)"},
}};

constexpr std::size_t Wave3Part1MethodCount() noexcept {
    return kWave3Part1Methods.size();
}

constexpr const Wave3Part1Method* FindWave3Part1Method(Uint64 address) noexcept {
    for (const Wave3Part1Method& method : kWave3Part1Methods) {
        if (method.address == address) {
            return &method;
        }
    }
    return nullptr;
}

static_assert(Wave3Part1MethodCount() == 453u);
static_assert(FindWave3Part1Method(0x1001862bcu) != nullptr);
static_assert(FindWave3Part1Method(0x1001862bcu)->sizeBytes == 1154u);
static_assert(FindWave3Part1Method(0x1003c2520u) != nullptr);
static_assert(FindWave3Part1Method(0x1003c2520u)->sizeBytes == 204u);
static_assert(FindWave3Part1Method(0x1007254b6u) != nullptr);
static_assert(FindWave3Part1Method(0x1007254b6u)->sizeBytes == 70u);
static_assert(FindWave3Part1Method(0x10077c4acu) != nullptr);
static_assert(FindWave3Part1Method(0x10077c4acu)->sizeBytes == 112u);
static_assert(FindWave3Part1Method(0x1006bbc4cu) != nullptr);
static_assert(FindWave3Part1Method(0x1006bbc4cu)->sizeBytes == 48u);
static_assert(FindWave3Part1Method(0x1002c119au) != nullptr);
static_assert(FindWave3Part1Method(0x1002c119au)->sizeBytes == 60u);
static_assert(FindWave3Part1Method(0x1007aaf18u) != nullptr);
static_assert(FindWave3Part1Method(0x1007aaf18u)->sizeBytes == 240u);
static_assert(FindWave3Part1Method(0) == nullptr);
static_assert(kWave3Part1Methods.front().address == 0x1001862bcu);
static_assert(kWave3Part1Methods.back().address == 0x1007aaf18u);

// ---------------------------------------------------------------------------
// Wave 4 Part 1 method index
// View queries, overlays, token I/O, player drive, power lines, geometry,
// Lua helpers, texture bindings, and the remaining Wave 4 Part 1 classes.
// Every 0x100xxxxxx entry point from the Wave 4 Part 1 workpack (85 classes,
// 809 symbols) is recorded in kWave4Part1Methods.
// ---------------------------------------------------------------------------

struct Wave4Part1Method {
    Uint64 address;
    Uint32 sizeBytes;
    const char* name;
};

inline constexpr std::array<Wave4Part1Method, 809> kWave4Part1Methods = {{
    // cSC4ViewInputControlQuery — 202 entry points
    {0x1002c1dcau, 52u, "cSC4ViewInputControlQuery::UpdatePaths()"},
    {0x1002c1dfeu, 196u, "cSC4ViewInputControlQuery::SetLegendMinimized(bool)"},
    {0x1002c1ec2u, 266u, "cSC4ViewInputControlQuery::cSC4ViewInputControlQuery(cSC4ViewInputControlQuery::EQueryType, unsigned int)"},
    {0x1002c2078u, 1310u, "cSC4ViewInputControlQuery::Init()"},
    {0x1002c2596u, 172u, "cSC4ViewInputControlQuery::MouseTipCallback(void*)"},
    {0x1002c2642u, 692u, "cSC4ViewInputControlQuery::Shutdown()"},
    {0x1002c28f6u, 280u, "cSC4ViewInputControlQuery::SetHighlightState(int)"},
    {0x1002c2a0eu, 1676u, "cSC4ViewInputControlQuery::DoMessage(cIGZMessage2*)"},
    {0x1002c309au, 1710u, "cSC4ViewInputControlQuery::GetBuildingOccupantTrafficInfo(cRZString&, cRZString&, unsigned int&)"},
    {0x1002c3748u, 2078u, "cSC4ViewInputControlQuery::GetNetworkOccupantTrafficInfo(cRZString&, cRZString&)"},
    {0x1002c3f66u, 4598u, "cSC4ViewInputControlQuery::GetBuildingOccupantTipInfo(cRZString&, cRZString&, unsigned int&, unsigned int&, float&)"},
    {0x1002c515cu, 1020u, "cSC4ViewInputControlQuery::GetNetworkOccupantSummaryInfo(cRZString&, cRZString&)"},
    {0x1002c5558u, 86u, "cSC4ViewInputControlQuery::GetPropOccupantSummaryInfo(cRZString&, cRZString&)"},
    {0x1002c55aeu, 86u, "cSC4ViewInputControlQuery::GetFloraOccupantSummaryInfo(cRZString&, cRZString&)"},
    {0x1002c5604u, 1380u, "cSC4ViewInputControlQuery::GetTerrainSummaryInfo(cRZString&, cRZString&)"},
    {0x1002c5b68u, 294u, "cSC4ViewInputControlQuery::SetTip(cIGZString const*, cIGZString const*)"},
    {0x1002c5ca8u, 12432u, "cSC4ViewInputControlQuery::WriteDetailedDebugInfo(int, int)"},
    {0x1002c8e20u, 1710u, "cSC4ViewInputControlQuery::DescribePopulation(int, int, cRZString&)"},
    {0x1002c95d0u, 2786u, "cSC4ViewInputControlQuery::WriteDesirabilityEquation(int, int, cISC4BuildingDevelopmentSimulator::DeveloperType)"},
    {0x1002ca0b2u, 1626u, "cSC4ViewInputControlQuery::WriteBuildInfo(cSC4TractDeveloper::GrowthAttemptRecord const*)"},
    {0x1002ca70cu, 434u, "cSC4ViewInputControlQuery::WriteOutcomeStats(cSC4TractDeveloper::GrowthAttemptRecord const*)"},
    {0x1002cb3ecu, 4020u, "cSC4ViewInputControlQuery::DescribeCell(cISC4ZoneManager::ZoneType, float*, cRZString&)"},
    {0x1002cc3a0u, 2456u, "cSC4ViewInputControlQuery::DescribeBuilding(cISC4Occupant*, cRZString&)"},
    {0x1002ccd38u, 4166u, "cSC4ViewInputControlQuery::DescribePick(int, int, cRZString&, cRZString&, cRZString&, bool)"},
    {0x1002cdd7eu, 142u, "cSC4ViewInputControlQuery::PickQueriableOccupant(int, int, cISC4Occupant*&, cSC4OccupantFilter*)"},
    {0x1002cde0cu, 360u, "cSC4ViewInputControlQuery::PlayOccupantQuerySound(cISC4Occupant*)"},
    {0x1002cdf74u, 1692u, "cSC4ViewInputControlQuery::DoNeighborConnectionQueryDialog(cISC4Occupant*)"},
    {0x1002ce610u, 1004u, "cSC4ViewInputControlQuery::DoQueryDialog(cISC4Occupant*)"},
    {0x1002ce9fcu, 792u, "cSC4ViewInputControlQuery::DoZoneBasedQueryDialog(unsigned int, unsigned int, unsigned int)"},
    {0x1002ced14u, 1534u, "cSC4ViewInputControlQuery::DescribeDemand(int, int, cRZString&)"},
    {0x1002cf312u, 124u, "cSC4ViewInputControlQuery::IsDebugQueryEnabled()"},
    {0x1002cf38eu, 250u, "cSC4ViewInputControlQuery::GetSubjectivePollution(cSC4GrowthDeveloper const*, int, int, int&)"},
    {0x1002cf488u, 188u, "cSC4ViewInputControlQuery::GetSubjectiveGarbage(cSC4GrowthDeveloper const*, int, int, int&)"},
    {0x1002cf544u, 186u, "cSC4ViewInputControlQuery::GetSubjectiveSchoolGrade(cSC4GrowthDeveloper const*, int, int, int&)"},
    {0x1002cf5feu, 186u, "cSC4ViewInputControlQuery::GetSubjectiveHospitalGrade(cSC4GrowthDeveloper const*, int, int, int&)"},
    {0x1002cf6b8u, 190u, "cSC4ViewInputControlQuery::GetSubjectiveCrimeLevel(cSC4GrowthDeveloper const*, int, int, int&)"},
    {0x1002cf776u, 262u, "cSC4ViewInputControlQuery::GetSubjectiveTrafficLevel(cSC4GrowthDeveloper const*, int, int, int&)"},
    {0x1002cf87cu, 204u, "cSC4ViewInputControlQuery::GetSubjectiveTripLength(cSC4GrowthDeveloper const*, int, int, int&)"},
    {0x1002cf948u, 408u, "cSC4ViewInputControlQuery::FindBestDeveloper(cISC4Lot*)"},
    {0x1002cfae0u, 176u, "cSC4ViewInputControlQuery::GetBuildingWealth(cISC4Lot*)"},
    {0x1002cfb90u, 10534u, "cSC4ViewInputControlQuery::GetLotHeadlines(cISC4Lot*, std::__1::multimap<int, cRZString, std::__1::less<int>, std::__1::allocator<std::__1::pair<int const, cRZString>>>&)"},
    {0x1002d24b6u, 64u, "cSC4ViewInputControlQuery::GetBuildingOccupantHeadlines(cISC4Occupant*, std::__1::multimap<int, cRZString, std::__1::less<int>, std::__1::allocator<std::__1::pair<int const, cRZString>>>&)"},
    {0x1002d24f6u, 288u, "cSC4ViewInputControlQuery::GetBuildingOccupantHeadlines(cISC4Occupant*, cRZString&)"},
    {0x1002d2616u, 80u, "cSC4ViewInputControlQuery::GetOccupantName(cISC4Occupant*, cRZString&)"},
    {0x1002d2666u, 648u, "cSC4ViewInputControlQuery::CalculateTripVolumeFromTrafficPaths(cISC4Lot*, cRZString&)"},
    {0x1002d28eeu, 1166u, "cSC4ViewInputControlQuery::CalculateTripVolumeFromTrafficPaths(SC4Rect<int> const&, cRZString&, bool)"},
    {0x1002d2d7cu, 322u, "cSC4ViewInputControlQuery::DoesRouteCrossCell(cSC4TripData&, SC4Rect<int> const&, bool)"},
    {0x1002d2ebeu, 106u, "cSC4ViewInputControlQuery::GetRouteTravelTypes(cSC4TripData&, bool)"},
    {0x1002d2f28u, 2617u, "cSC4ViewInputControlQuery::MakeRoutePath(cISC4TrafficSource&, cSC4TripData&, cS3DVector3, bool, unsigned int)"},
    {0x1002d3a04u, 1566u, "cSC4ViewInputControlQuery::ShowAllRoutesThatIntersectCityRect(SC4Rect<int> const&)"},
    {0x1002d4022u, 170u, "cSC4ViewInputControlQuery::GetBuildingOccupantPaths()"},
    {0x1002d40ccu, 484u, "cSC4ViewInputControlQuery::GetNetworkOccupantPaths()"},
    {0x1002d42b0u, 146u, "cSC4ViewInputControlQuery::GetOccupantPaths()"},
    {0x1002d4342u, 698u, "cSC4ViewInputControlQuery::OnMouseDownL(int, int, unsigned int)"},
    {0x1002d45fcu, 270u, "cSC4ViewInputControlQuery::PickAutomaton(int, int, cISC4Automaton*&)"},
    {0x1002d470au, 270u, "cSC4ViewInputControlQuery::PickStoplightProp(int, int, cISC4PropOccupant*&)"},
    {0x1002d4818u, 74u, "cSC4ViewInputControlQuery::OnMouseUpL(int, int, unsigned int)"},
    {0x1002d4862u, 1480u, "cSC4ViewInputControlQuery::OnMouseMove(int, int, unsigned int)"},
    {0x1002d4e2au, 126u, "cSC4ViewInputControlQuery::OnMouseExit()"},
    {0x1002d4eb0u, 144u, "cSC4ViewInputControlQuery::CoverageRingCallback(void*)"},
    {0x1002d4f40u, 114u, "cSC4ViewInputControlQuery::SetEditControlText(cIGZWin*, unsigned int, cIGZString&)"},
    {0x1002d4fb2u, 226u, "cSC4ViewInputControlQuery::GetEditControlText(cIGZWin*, unsigned int, cIGZString&, unsigned int&)"},
    {0x1002d5094u, 72u, "cSC4ViewInputControlQuery::SetStaticControlText(cIGZWin*, unsigned int, cIGZString const&)"},
    {0x1002d50dcu, 140u, "cSC4ViewInputControlQuery::SetStaticControlNumeric(cIGZWin*, unsigned int, unsigned int)"},
    {0x1002d5168u, 140u, "cSC4ViewInputControlQuery::SetStaticControlNumericPercent(cIGZWin*, unsigned int, unsigned int)"},
    {0x1002d51f4u, 134u, "cSC4ViewInputControlQuery::SetStaticControlSimoleons(cIGZWin*, unsigned int, long long)"},
    {0x1002d527au, 550u, "cSC4ViewInputControlQuery::SetStaticControlSimoleonsFloat(cIGZWin*, unsigned int, double)"},
    {0x1002d54a0u, 696u, "cSC4ViewInputControlQuery::SetSliderControlValue(cIGZWin*, unsigned int, int, int, int, long long, cISC4Occupant*, bool (*)(cIGZWin*, unsigned int, bool, cISC4Occupant*), unsigned int*, unsigned int)"},
    {0x1002d5758u, 72u, "cSC4ViewInputControlQuery::GetSliderControlValue(cIGZWin*, unsigned int, int&)"},
    {0x1002d57a0u, 390u, "cSC4ViewInputControlQuery::DrawPerformanceMeter(cIGZWinBMP*, float)"},
    {0x1002d5926u, 228u, "cSC4ViewInputControlQuery::SetPerformanceMeterValue(cIGZWin*, unsigned int, float)"},
    {0x1002d5a0au, 438u, "cSC4ViewInputControlQuery::SetOrGetName(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d5bc0u, 548u, "cSC4ViewInputControlQuery::SetOrGetWhyOccupantAbandoned(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d5de4u, 58u, "cSC4ViewInputControlQuery::SetOrGetFireStationEffectiveness(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d5e1eu, 228u, "cSC4ViewInputControlQuery::SetOrGetFireStationLocalFunding(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d5f02u, 128u, "cSC4ViewInputControlQuery::SetOrGetFireStationEngineCount(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d5f82u, 86u, "cSC4ViewInputControlQuery::SetOrGetFireStationMaintenanceCost(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d5fd8u, 62u, "cSC4ViewInputControlQuery::SetOrGetPoliceStationEffectiveness(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d6016u, 64u, "cSC4ViewInputControlQuery::SetOrGetPoliceStationArrestCount(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d6056u, 64u, "cSC4ViewInputControlQuery::SetOrGetPoliceStationCrimeCount(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d6096u, 228u, "cSC4ViewInputControlQuery::SetOrGetPoliceStationLocalFunding(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d617au, 128u, "cSC4ViewInputControlQuery::SetOrGetPoliceStationPatrolCarCount(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d61fau, 86u, "cSC4ViewInputControlQuery::SetOrGetPoliceStationMaintenanceCost(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d6250u, 204u, "cSC4ViewInputControlQuery::SetOrGetJailCondition(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d631cu, 78u, "cSC4ViewInputControlQuery::SetOrGetJailCellCount(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d636au, 124u, "cSC4ViewInputControlQuery::SetOrGetJailInmateCount(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d63e6u, 64u, "cSC4ViewInputControlQuery::SetOrGetJailMaintenanceCost(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d6434u, 294u, "cSC4ViewInputControlQuery::SetOrGetOccupantCost(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d655au, 84u, "cSC4ViewInputControlQuery::SetOrGetAirportOccupantAirTrafficVolume(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d65aeu, 220u, "cSC4ViewInputControlQuery::SetOrGetAirportAirTrafficVolume(cIGZWin*, unsigned int, bool, int, int)"},
    {0x1002d668au, 84u, "cSC4ViewInputControlQuery::SetOrGetAirportOccupantNoisePollution(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d66deu, 408u, "cSC4ViewInputControlQuery::SetOrGetAirportNoisePollution(cIGZWin*, unsigned int, bool, int, int)"},
    {0x1002d6876u, 70u, "cSC4ViewInputControlQuery::SetOrGetAirportOccupantLostLuggage(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d68bcu, 136u, "cSC4ViewInputControlQuery::SetOrGetAirportLostLuggage(cIGZWin*, unsigned int, bool, int, int)"},
    {0x1002d6944u, 346u, "cSC4ViewInputControlQuery::SetOrGetGoofyStat(cIGZWin*, unsigned int, bool, cISCPropertyHolder*, unsigned int)"},
    {0x1002d6a9eu, 84u, "cSC4ViewInputControlQuery::SetOrGetAirportOccupantMaintenanceCost(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d6af2u, 124u, "cSC4ViewInputControlQuery::SetOrGetAirportMaintenanceCost(cIGZWin*, unsigned int, bool, int, int)"},
    {0x1002d6b6eu, 84u, "cSC4ViewInputControlQuery::SetOrGetAirportOccupantUsage(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d6bc2u, 138u, "cSC4ViewInputControlQuery::SetOrGetAirportUsage(cIGZWin*, unsigned int, bool, int, int)"},
    {0x1002d6c4cu, 84u, "cSC4ViewInputControlQuery::SetOrGetAirportOccupantCapacity(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d6ca0u, 138u, "cSC4ViewInputControlQuery::SetOrGetAirportCapacity(cIGZWin*, unsigned int, bool, int, int)"},
    {0x1002d6d2au, 84u, "cSC4ViewInputControlQuery::SetOrGetSeaportOccupantUsage(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d6d7eu, 138u, "cSC4ViewInputControlQuery::SetOrGetSeaportUsage(cIGZWin*, unsigned int, bool, int, int)"},
    {0x1002d6e08u, 84u, "cSC4ViewInputControlQuery::SetOrGetSeaportOccupantCapacity(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d6e5cu, 138u, "cSC4ViewInputControlQuery::SetOrGetSeaportCapacity(cIGZWin*, unsigned int, bool, int, int)"},
    {0x1002d6ee6u, 84u, "cSC4ViewInputControlQuery::SetOrGetSeaportOccupantSeaTrafficVolume(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d6f3au, 220u, "cSC4ViewInputControlQuery::SetOrGetSeaportSeaTrafficVolume(cIGZWin*, unsigned int, bool, int, int)"},
    {0x1002d7016u, 84u, "cSC4ViewInputControlQuery::SetOrGetSeaportOccupantMaintenanceCost(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d706au, 124u, "cSC4ViewInputControlQuery::SetOrGetSeaportMaintenanceCost(cIGZWin*, unsigned int, bool, int, int)"},
    {0x1002d70e6u, 92u, "cSC4ViewInputControlQuery::SetOrGetWaterPollutionVolume(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d7142u, 186u, "cSC4ViewInputControlQuery::SetOrGetSchoolGrade(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d71fcu, 330u, "cSC4ViewInputControlQuery::SetOrGetSchoolLocalStaffFunding(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d7346u, 326u, "cSC4ViewInputControlQuery::SetOrGetSchoolLocalBusFunding(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d748cu, 82u, "cSC4ViewInputControlQuery::SetOrGetSchoolTeacherCount(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d74deu, 82u, "cSC4ViewInputControlQuery::SetOrGetSchoolDeskCount(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d7530u, 172u, "cSC4ViewInputControlQuery::SetOrGetSchoolStudentCount(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d75eau, 172u, "cSC4ViewInputControlQuery::SetOrGetLibraryBooksBorrowedCount(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d7696u, 82u, "cSC4ViewInputControlQuery::SetOrGetLibraryBooksInCollectionCount(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d76e8u, 186u, "cSC4ViewInputControlQuery::SetOrGetHospitalGrade(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d77a2u, 326u, "cSC4ViewInputControlQuery::SetOrGetHospitalLocalStaffFunding(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d78e8u, 324u, "cSC4ViewInputControlQuery::SetOrGetHospitalLocalAmbulanceFunding(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d7a2cu, 82u, "cSC4ViewInputControlQuery::SetOrGetHospitalDoctorCount(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d7a7eu, 82u, "cSC4ViewInputControlQuery::SetOrGetHospitalBedCount(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d7ad0u, 172u, "cSC4ViewInputControlQuery::SetOrGetHospitalPatientCount(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d7b7cu, 166u, "cSC4ViewInputControlQuery::SetOrGetLandfillUsage(cIGZWin*, unsigned int, bool, int, int)"},
    {0x1002d7c22u, 112u, "cSC4ViewInputControlQuery::SetOrGetLandfillNetTrashThisMonth(cIGZWin*, unsigned int, bool, int, int)"},
    {0x1002d7c92u, 108u, "cSC4ViewInputControlQuery::SetOrGetIncineratorEfficiency(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d7cfeu, 134u, "cSC4ViewInputControlQuery::SetOrGetIncineratorPercentCapacity(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d7d84u, 124u, "cSC4ViewInputControlQuery::SetOrGetIncineratorTrashIncinerated(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d7e00u, 92u, "cSC4ViewInputControlQuery::SetOrGetAirPollutionVolume(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d7e5cu, 154u, "cSC4ViewInputControlQuery::SetOrGetRecyclingCenterEfficiency(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d7ef6u, 210u, "cSC4ViewInputControlQuery::SetOrGetRecyclingCenterPercentCapacity(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d7fc8u, 156u, "cSC4ViewInputControlQuery::SetOrGetRecyclingCenterTrashRecycled(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d8064u, 62u, "cSC4ViewInputControlQuery::SetOrGetWasteToEnergyEfficiency(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d80a2u, 62u, "cSC4ViewInputControlQuery::SetOrGetPowerPlantEfficiency(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d80eeu, 116u, "cSC4ViewInputControlQuery::SetOrGetPowerPlantPercentCapacity(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d8162u, 76u, "cSC4ViewInputControlQuery::SetOrGetWasteToEnergyMaxPowerOutputCapacity(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d81aeu, 76u, "cSC4ViewInputControlQuery::SetOrGetPowerPlantMaxPowerOutputCapacity(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d8208u, 92u, "cSC4ViewInputControlQuery::SetOrGetPowerPlantCurrentPowerOutputCapacity(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d8272u, 88u, "cSC4ViewInputControlQuery::SetOrGetPowerPlantPowerUsed(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d82d8u, 160u, "cSC4ViewInputControlQuery::SetOrGetPowerPlantCostPerUnit(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d8378u, 124u, "cSC4ViewInputControlQuery::SetOrGetWasteToEnergyTrashIncinerated(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d83f4u, 74u, "cSC4ViewInputControlQuery::SetOrGetWasteToEnergyPercentCondition(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d843eu, 74u, "cSC4ViewInputControlQuery::SetOrGetPowerPlantPercentCondition(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d8488u, 278u, "cSC4ViewInputControlQuery::SetOrGetPowerPlantLocalFunding(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d859eu, 112u, "cSC4ViewInputControlQuery::SetOrGetWaterBuildingEfficiency(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d860eu, 128u, "cSC4ViewInputControlQuery::SetOrGetWaterBuildingUsagePercent(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d868eu, 100u, "cSC4ViewInputControlQuery::SetOrGetWaterBuildingMaxCapacity(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d86f2u, 104u, "cSC4ViewInputControlQuery::SetOrGetWaterBuildingCurrentCapacity(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d875au, 102u, "cSC4ViewInputControlQuery::SetOrGetWaterBuildingWaterUsed(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d87c0u, 160u, "cSC4ViewInputControlQuery::SetOrGetWaterBuildingCostPerUnit(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d8860u, 236u, "cSC4ViewInputControlQuery::SetOrGetWaterBuildingPumpCondition(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d894cu, 100u, "cSC4ViewInputControlQuery::SetOrGetWaterBuildingPercentCondition(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d89b0u, 254u, "cSC4ViewInputControlQuery::SetOrGetTransitStationServiceQuality(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d8aaeu, 244u, "cSC4ViewInputControlQuery::SetOrGetTransitStationUsagePercent(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d8ba2u, 222u, "cSC4ViewInputControlQuery::SetOrGetTransitStationMaintenanceCost(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d8c80u, 222u, "cSC4ViewInputControlQuery::SetOrGetTransitStationActualUsage(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d8d5eu, 222u, "cSC4ViewInputControlQuery::SetOrGetTransitStationMaxCapacity(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d8e3cu, 98u, "cSC4ViewInputControlQuery::SetOrGetTransitStationGlobalFunding(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d8e9eu, 118u, "cSC4ViewInputControlQuery::SetOrGetLandmarkFunding(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d8f14u, 176u, "cSC4ViewInputControlQuery::SetOrGetLandmarkMaintenance(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d8fc4u, 170u, "cSC4ViewInputControlQuery::SetOrGetLandmarkHomage(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d906eu, 104u, "cSC4ViewInputControlQuery::FindCityOnOtherSideOfConnection(cISC4NeighborsSimulator::ConnectionType, int, int)"},
    {0x1002d90d6u, 174u, "cSC4ViewInputControlQuery::SetOrGetNeighborConnectionNeighborCityName(cIGZWin*, unsigned int, bool, int, int, cISC4NeighborsSimulator::ConnectionType)"},
    {0x1002d9184u, 170u, "cSC4ViewInputControlQuery::SetOrGetNeighborConnectionNeighborCityMayorName(cIGZWin*, unsigned int, bool, int, int, cISC4NeighborsSimulator::ConnectionType)"},
    {0x1002d922eu, 202u, "cSC4ViewInputControlQuery::SetOrGetNeighborConnectionCongestion(cIGZWin*, unsigned int, bool, int, int, cISC4NeighborsSimulator::ConnectionType)"},
    {0x1002d931eu, 408u, "cSC4ViewInputControlQuery::SetOrGetNeighborConnectionDealExists(cIGZWin*, unsigned int, bool, int, int, cISC4NeighborsSimulator::ConnectionType)"},
    {0x1002d94b6u, 444u, "cSC4ViewInputControlQuery::SetOrGetNeighborConnectionDealCommodityAmount(cIGZWin*, unsigned int, bool, int, int, cISC4NeighborsSimulator::ConnectionType, float)"},
    {0x1002d9672u, 342u, "cSC4ViewInputControlQuery::SetOrGetNeighborConnectionDealValue(cIGZWin*, unsigned int, bool, int, int, cISC4NeighborsSimulator::ConnectionType)"},
    {0x1002d97c8u, 502u, "cSC4ViewInputControlQuery::SetOrGetNeighborConnectionDealDirection(cIGZWin*, unsigned int, bool, int, int, cISC4NeighborsSimulator::ConnectionType)"},
    {0x1002d99beu, 212u, "cSC4ViewInputControlQuery::GetSubjectiveOccupantRatingString(cISC4Occupant*, unsigned int (*)(cSC4GrowthDeveloper const*, int, int, int&), unsigned int const*, cRZString&)"},
    {0x1002d9a92u, 280u, "cSC4ViewInputControlQuery::GetCurrentOccupancyValue(cISC4Occupant*)"},
    {0x1002d9baau, 254u, "cSC4ViewInputControlQuery::GetOccupantCapacity(cISC4Occupant*)"},
    {0x1002d9ca8u, 182u, "cSC4ViewInputControlQuery::SetOrGetRCIBuildingCurrentOccupancy(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d9d5eu, 162u, "cSC4ViewInputControlQuery::SetOrGetRCIBuildingMaxOccupancy(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002d9e00u, 606u, "cSC4ViewInputControlQuery::SetOrGetRCIBuildingWealthLevel(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002da05eu, 132u, "cSC4ViewInputControlQuery::GetLandValue(cISC4Occupant*)"},
    {0x1002da0e2u, 142u, "cSC4ViewInputControlQuery::SetOrGetRCIBuildingPollution(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002da170u, 142u, "cSC4ViewInputControlQuery::SetOrGetRCIBuildingGarbage(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002da1feu, 142u, "cSC4ViewInputControlQuery::SetOrGetRCIBuildingCrime(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002da28cu, 254u, "cSC4ViewInputControlQuery::SetOrGetRCIBuildingLandValue(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002da38au, 216u, "cSC4ViewInputControlQuery::SetOrGetResidentialBuildingCommute(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002da462u, 142u, "cSC4ViewInputControlQuery::SetOrGetRCIBuildingTraffic(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002da4f0u, 336u, "cSC4ViewInputControlQuery::GetMayorRatingPercent(cISC4Occupant*)"},
    {0x1002da640u, 82u, "cSC4ViewInputControlQuery::GetRatingIn3DegreesFromPercent(cRZString&, float)"},
    {0x1002da692u, 146u, "cSC4ViewInputControlQuery::SetOrGetResidentialBuildingMayorApproval(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002da724u, 142u, "cSC4ViewInputControlQuery::SetOrGetResidentialBuildingEQ(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002da7b2u, 142u, "cSC4ViewInputControlQuery::SetOrGetResidentialBuildingHQ(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002da840u, 998u, "cSC4ViewInputControlQuery::SetOrGetResidentialBuildingMySimicon(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002dac26u, 112u, "cSC4ViewInputControlQuery::SetOrGetCommercialServiceBuildingCustomers(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002dac96u, 60u, "cSC4ViewInputControlQuery::SetOrGetOccupantGoofyStat(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002dacd2u, 260u, "cSC4ViewInputControlQuery::SetOrGetOccupantIsPowered(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002dadd6u, 256u, "cSC4ViewInputControlQuery::SetOrGetOccupantIsWatered(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002daed6u, 172u, "cSC4ViewInputControlQuery::SetOrGetOccupantIsHistorical(cIGZWin*, unsigned int, bool, cISC4Occupant*)"},
    {0x1002daf96u, 1000u, "cSC4ViewInputControlQuery::SetOrGetControlValue(cIGZWin*, unsigned int, bool, cSC4ViewInputControlQuery::tCellInfo*)"},
    {0x1002db37eu, 62u, "cSC4ViewInputControlQuery::EnumFuncSetFilter(cIGZWin*, unsigned int, void*, void*)"},
    {0x1002db3bcu, 426u, "cSC4ViewInputControlQuery::EnumFuncClearFilter(cIGZWin*, unsigned int, void*, void*)"},
    {0x1002db566u, 114u, "cSC4ViewInputControlQuery::EnumFuncSetValue(cIGZWin*, unsigned int, void*, void*)"},
    {0x1002db5d8u, 198u, "cSC4ViewInputControlQuery::EnumFuncUpdateValue(cIGZWin*, unsigned int, void*, void*)"},
    {0x1002db69eu, 112u, "cSC4ViewInputControlQuery::EnumFuncGetValue(cIGZWin*, unsigned int, void*, void*)"},
    {0x1002db70eu, 58u, "cSC4ViewInputControlQuery::EnumFuncNeighborDialogSetValue(cIGZWin*, unsigned int, void*, void*)"},
    {0x1002db748u, 54u, "cSC4ViewInputControlQuery::EnumFuncNeighborDialogGetValue(cIGZWin*, unsigned int, void*, void*)"},
    // cSTEOverlayManager — 50 entry points
    {0x100727300u, 198u, "cSTEOverlayManager::cOverlay::cOverlay()"},
    {0x1007273c6u, 398u, "cSTEOverlayManager::cSTEOverlayManager()"},
    {0x100727792u, 656u, "cSTEOverlayManager::Init(cSTETerrainView3D*, bool, bool)"},
    {0x100727a22u, 476u, "cSTEOverlayManager::InitDrawStates()"},
    {0x100727bfeu, 174u, "cSTEOverlayManager::Shutdown()"},
    {0x100727e28u, 84u, "cSTEOverlayManager::DoMessage(cIGZMessage2*)"},
    {0x100727ed0u, 14u, "cSTEOverlayManager::Update(SC4DrawContext*)"},
    {0x100727edeu, 930u, "cSTEOverlayManager::DrawOverlays(SC4DrawContext*)"},
    {0x1007282a2u, 138u, "cSTEOverlayManager::AddDecal(cGZPersistResourceKey const&, cS3DVector2 const&, float, float)"},
    {0x10072832cu, 138u, "cSTEOverlayManager::AddDecal(unsigned int, cS3DVector2 const&, float, float)"},
    {0x1007283ceu, 472u, "cSTEOverlayManager::AddDecal(cISC4CachedStringTexture const*, cS3DTextureBinding const*, cS3DVector2 const&, float, float)"},
    {0x1007286acu, 1838u, "cSTEOverlayManager::UpdateDecal(cSTEOverlayManager::cDecal const&, bool)"},
    {0x100728ddau, 74u, "cSTEOverlayManager::AddOverlay(unsigned int)"},
    {0x100728f32u, 574u, "cSTEOverlayManager::AddRingDecal(unsigned int, cS3DVector2 const&, float, float)"},
    {0x10072924eu, 120u, "cSTEOverlayManager::AddShadowDecal(unsigned int, cS3DVector2 const&, float, float, bool)"},
    {0x1007292c6u, 770u, "cSTEOverlayManager::AddShadowDecal(cGZPersistResourceKey const&, cS3DVector2 const&, float, float, bool)"},
    {0x1007295c8u, 160u, "cSTEOverlayManager::AddHeightMap(unsigned int)"},
    {0x100729668u, 136u, "cSTEOverlayManager::AddOverlay(unsigned int, cRZRect const&, cS3DMatrix4 const&, std::__1::bitset<8ul>)"},
    {0x1007296f0u, 550u, "cSTEOverlayManager::CalcShadowTerrainRect(cS3DVector3 const&, int, cS3DVector3 const*, cS3DBoundingBox const&, cRZRect*)"},
    {0x100729916u, 704u, "cSTEOverlayManager::AddShadow(cS3DTextureBinding const*, int, cS3DVector3 const*, cS3DVector2 const*, cS3DBoundingBox const&, cS3DTransform const&)"},
    {0x100729bd6u, 320u, "cSTEOverlayManager::AddOverlay(cS3DTextureBinding const*, cRZRect const&, cS3DMatrix4 const&, std::__1::bitset<8ul>)"},
    {0x100729d16u, 180u, "cSTEOverlayManager::MoveDecal(unsigned int, cS3DVector2 const&)"},
    {0x100729dcau, 102u, "cSTEOverlayManager::UpdateDecalInfo(unsigned int, cISTEOverlayManager::cDecalInfo const&)"},
    {0x100729e30u, 80u, "cSTEOverlayManager::DecalInfo(unsigned int, cISTEOverlayManager::cDecalInfo*) const"},
    {0x100729e80u, 436u, "cSTEOverlayManager::RemoveOverlay(unsigned int)"},
    {0x10072a034u, 152u, "cSTEOverlayManager::SDSRemove(unsigned int, cRZRect const&)"},
    {0x10072a0ccu, 106u, "cSTEOverlayManager::RedisplayStaticOverlay(unsigned int)"},
    {0x10072a136u, 192u, "cSTEOverlayManager::SetOverlayEnabled(unsigned int, bool)"},
    {0x10072a1f6u, 252u, "cSTEOverlayManager::SetOverlayAlpha(unsigned int, float)"},
    {0x10072a2f2u, 194u, "cSTEOverlayManager::SetOverlayColor(unsigned int, cS3DVector3 const&)"},
    {0x10072a3b4u, 168u, "cSTEOverlayManager::SetOverlayDrawMode(unsigned int, cISTEOverlayManager::tDrawMode)"},
    {0x10072a45cu, 150u, "cSTEOverlayManager::SetOverlayFlags(unsigned int, unsigned int)"},
    {0x10072a4f2u, 314u, "cSTEOverlayManager::SetHeightMapParams(unsigned int, float, float)"},
    {0x10072a62cu, 92u, "cSTEOverlayManager::OverlayIsEnabled(unsigned int) const"},
    {0x10072a688u, 96u, "cSTEOverlayManager::OverlayAlpha(unsigned int) const"},
    {0x10072a6e8u, 130u, "cSTEOverlayManager::OverlayColor(unsigned int) const"},
    {0x10072a76au, 90u, "cSTEOverlayManager::OverlayDrawMode(unsigned int) const"},
    {0x10072a7c4u, 96u, "cSTEOverlayManager::OverlayFlags(unsigned int) const"},
    {0x10072a824u, 46u, "cSTEOverlayManager::GetStatsString(cRZString&)"},
    {0x10072a852u, 188u, "cSTEOverlayManager::SDSAdd(unsigned int, cRZRect const&)"},
    {0x10072a90eu, 328u, "cSTEOverlayManager::SDSFindOverlaysToDraw(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>*)"},
    {0x10072aa56u, 600u, "cSTEOverlayManager::DrawDecals(cS3DMatrix4 const&, SC4DrawContext*, std::__1::vector<unsigned int, std::__1::allocator<unsigned int>> const&)"},
    {0x10072acaeu, 562u, "cSTEOverlayManager::DrawShadowsRough(cS3DMatrix4 const&, SC4DrawContext*, std::__1::vector<unsigned int, std::__1::allocator<unsigned int>> const&)"},
    {0x10072aee0u, 1122u, "cSTEOverlayManager::DrawShadows(cS3DMatrix4 const&, SC4DrawContext*, std::__1::vector<unsigned int, std::__1::allocator<unsigned int>> const&)"},
    {0x10072b342u, 172u, "cSTEOverlayManager::DrawRect(SC4DrawContext*, cRZRect const&)"},
    {0x10072b3eeu, 84u, "cSTEOverlayManager::DrawMixedWaterAndLandRect(SC4DrawContext*, cRZRect const&)"},
    {0x10072b442u, 3588u, "cSTEOverlayManager::UpdateRingDecalOverlays(cSTEOverlayManager::cRingDecal const&, SC4Rect<float> const&, bool)"},
    {0x10072c246u, 356u, "cSTEOverlayManager::UpdateStaticOverlayRect(unsigned int, cRZRect const&)"},
    {0x10072ccc8u, 474u, "cSTEOverlayManager::cOverlay::operator=(cSTEOverlayManager::cOverlay const&)"},
    {0x10072cf78u, 518u, "cSTEOverlayManager::cOverlay::cOverlay(cSTEOverlayManager::cOverlay const&)"},
    // cSLTokenField — 40 entry points
    {0x1004e6914u, 50u, "cSLTokenField<cGZPersistResourceKey>::Get(cGZPersistResourceKey&, unsigned int)"},
    {0x1004e6946u, 50u, "cSLTokenField<cGZPersistResourceKey>::Set(cGZPersistResourceKey const&, unsigned int)"},
    {0x1004e6978u, 66u, "cSLTokenField<cRZString>::Get(cIGZString&, unsigned int)"},
    {0x1004e69bau, 50u, "cSLTokenField<cRZString>::Set(cIGZString const&, unsigned int)"},
    {0x1004e74d0u, 88u, "cSLTokenField<unsigned int>::WriteData(cSLTokenSerialRecordIO&)"},
    {0x1004e92e8u, 106u, "cSLTokenField<unsigned int>::cSLTokenField(cGZPersistTokenDef const&)"},
    {0x1004e9380u, 84u, "cSLTokenField<unsigned int>::ReadData(cSLTokenSerialRecordIO&)"},
    {0x1004e94a8u, 102u, "cSLTokenField<unsigned char>::cSLTokenField(cGZPersistTokenDef const&)"},
    {0x1004e9546u, 74u, "cSLTokenField<unsigned char>::ReadData(cSLTokenSerialRecordIO&)"},
    {0x1004e9590u, 78u, "cSLTokenField<unsigned char>::WriteData(cSLTokenSerialRecordIO&)"},
    {0x1004e96a6u, 102u, "cSLTokenField<signed char>::cSLTokenField(cGZPersistTokenDef const&)"},
    {0x1004e9744u, 74u, "cSLTokenField<signed char>::ReadData(cSLTokenSerialRecordIO&)"},
    {0x1004e978eu, 78u, "cSLTokenField<signed char>::WriteData(cSLTokenSerialRecordIO&)"},
    {0x1004e98b4u, 106u, "cSLTokenField<unsigned short>::cSLTokenField(cGZPersistTokenDef const&)"},
    {0x1004e9956u, 84u, "cSLTokenField<unsigned short>::ReadData(cSLTokenSerialRecordIO&)"},
    {0x1004e99aau, 88u, "cSLTokenField<unsigned short>::WriteData(cSLTokenSerialRecordIO&)"},
    {0x1004e9acau, 106u, "cSLTokenField<short>::cSLTokenField(cGZPersistTokenDef const&)"},
    {0x1004e9b6cu, 84u, "cSLTokenField<short>::ReadData(cSLTokenSerialRecordIO&)"},
    {0x1004e9bc0u, 88u, "cSLTokenField<short>::WriteData(cSLTokenSerialRecordIO&)"},
    {0x1004e9cf0u, 106u, "cSLTokenField<int>::cSLTokenField(cGZPersistTokenDef const&)"},
    {0x1004e9d92u, 84u, "cSLTokenField<int>::ReadData(cSLTokenSerialRecordIO&)"},
    {0x1004e9de6u, 88u, "cSLTokenField<int>::WriteData(cSLTokenSerialRecordIO&)"},
    {0x1004e9f06u, 106u, "cSLTokenField<unsigned long long>::cSLTokenField(cGZPersistTokenDef const&)"},
    {0x1004e9fa8u, 84u, "cSLTokenField<unsigned long long>::ReadData(cSLTokenSerialRecordIO&)"},
    {0x1004e9ffcu, 88u, "cSLTokenField<unsigned long long>::WriteData(cSLTokenSerialRecordIO&)"},
    {0x1004ea12cu, 106u, "cSLTokenField<long long>::cSLTokenField(cGZPersistTokenDef const&)"},
    {0x1004ea1ceu, 84u, "cSLTokenField<long long>::ReadData(cSLTokenSerialRecordIO&)"},
    {0x1004ea222u, 88u, "cSLTokenField<long long>::WriteData(cSLTokenSerialRecordIO&)"},
    {0x1004ea342u, 106u, "cSLTokenField<float>::cSLTokenField(cGZPersistTokenDef const&)"},
    {0x1004ea3e4u, 84u, "cSLTokenField<float>::ReadData(cSLTokenSerialRecordIO&)"},
    {0x1004ea438u, 88u, "cSLTokenField<float>::WriteData(cSLTokenSerialRecordIO&)"},
    {0x1004ea558u, 106u, "cSLTokenField<double>::cSLTokenField(cGZPersistTokenDef const&)"},
    {0x1004ea5fau, 84u, "cSLTokenField<double>::ReadData(cSLTokenSerialRecordIO&)"},
    {0x1004ea64eu, 88u, "cSLTokenField<double>::WriteData(cSLTokenSerialRecordIO&)"},
    {0x1004ea77eu, 184u, "cSLTokenField<cRZString>::cSLTokenField(cGZPersistTokenDef const&)"},
    {0x1004ea86eu, 84u, "cSLTokenField<cRZString>::ReadData(cSLTokenSerialRecordIO&)"},
    {0x1004ea8c2u, 88u, "cSLTokenField<cRZString>::WriteData(cSLTokenSerialRecordIO&)"},
    {0x1004eaa1eu, 188u, "cSLTokenField<cGZPersistResourceKey>::cSLTokenField(cGZPersistTokenDef const&)"},
    {0x1004eab12u, 84u, "cSLTokenField<cGZPersistResourceKey>::ReadData(cSLTokenSerialRecordIO&)"},
    {0x1004eab66u, 88u, "cSLTokenField<cGZPersistResourceKey>::WriteData(cSLTokenSerialRecordIO&)"},
    // cSLTokenRecord — 36 entry points
    {0x1004e6a34u, 360u, "cSLTokenRecord::Shutdown()"},
    {0x1004e6c24u, 80u, "cSLTokenRecord::GetKey(cGZPersistResourceKey&) const"},
    {0x1004e6c88u, 246u, "cSLTokenRecord::InitRead(cIGZPersistDBSerialRecord&, unsigned int)"},
    {0x1004e6d7eu, 116u, "cSLTokenRecord::ReadTokensFromSerialRecord(cSLTokenSerialRecordIO&)"},
    {0x1004e6dfeu, 272u, "cSLTokenRecord::InitRead(cIGZPersistDBSerialRecord&)"},
    {0x1004e6f0eu, 48u, "cSLTokenRecord::InitWriteText(cIGZPersistDBSerialRecord&)"},
    {0x1004e6f3eu, 48u, "cSLTokenRecord::InitWriteBinary(cIGZPersistDBSerialRecord&)"},
    {0x1004e6f7eu, 580u, "cSLTokenRecord::WriteTokensToSerialRecord(cSLTokenSerialRecordIO&)"},
    {0x1004e71c2u, 782u, "cSLTokenRecord::CreateField(cGZPersistTokenDef const&, cSLTokenFieldBase**)"},
    {0x1004e7532u, 106u, "cSLTokenRecord::GetField(cGZPersistTokenDef const&, cSLTokenFieldBase**)"},
    {0x1004e759cu, 96u, "cSLTokenRecord::IsTokenAvailable(cGZPersistTokenDef const&)"},
    {0x1004e75fcu, 270u, "cSLTokenRecord::SetTokenString(cGZPersistTokenDef const&, cIGZString&)"},
    {0x1004e770au, 66u, "cSLTokenRecord::GetFieldUint8(cGZPersistTokenDef const&, unsigned char&, unsigned int)"},
    {0x1004e774cu, 100u, "cSLTokenRecord::SetFieldUint8(cGZPersistTokenDef const&, unsigned char const&, unsigned int)"},
    {0x1004e77b0u, 66u, "cSLTokenRecord::GetFieldSint8(cGZPersistTokenDef const&, signed char&, unsigned int)"},
    {0x1004e77f2u, 100u, "cSLTokenRecord::SetFieldSint8(cGZPersistTokenDef const&, signed char const&, unsigned int)"},
    {0x1004e7856u, 66u, "cSLTokenRecord::GetFieldUint16(cGZPersistTokenDef const&, unsigned short&, unsigned int)"},
    {0x1004e7898u, 100u, "cSLTokenRecord::SetFieldUint16(cGZPersistTokenDef const&, unsigned short const&, unsigned int)"},
    {0x1004e78fcu, 66u, "cSLTokenRecord::GetFieldSint16(cGZPersistTokenDef const&, short&, unsigned int)"},
    {0x1004e793eu, 100u, "cSLTokenRecord::SetFieldSint16(cGZPersistTokenDef const&, short const&, unsigned int)"},
    {0x1004e79a2u, 66u, "cSLTokenRecord::GetFieldUint32(cGZPersistTokenDef const&, unsigned int&, unsigned int)"},
    {0x1004e79e4u, 100u, "cSLTokenRecord::SetFieldUint32(cGZPersistTokenDef const&, unsigned int const&, unsigned int)"},
    {0x1004e7a48u, 66u, "cSLTokenRecord::GetFieldSint32(cGZPersistTokenDef const&, int&, unsigned int)"},
    {0x1004e7a8au, 102u, "cSLTokenRecord::SetFieldSint32(cGZPersistTokenDef const&, int const&, unsigned int)"},
    {0x1004e7af0u, 66u, "cSLTokenRecord::GetFieldUint64(cGZPersistTokenDef const&, unsigned long long&, unsigned int)"},
    {0x1004e7b32u, 102u, "cSLTokenRecord::SetFieldUint64(cGZPersistTokenDef const&, unsigned long long const&, unsigned int)"},
    {0x1004e7b98u, 66u, "cSLTokenRecord::GetFieldSint64(cGZPersistTokenDef const&, long long&, unsigned int)"},
    {0x1004e7bdau, 102u, "cSLTokenRecord::SetFieldSint64(cGZPersistTokenDef const&, long long const&, unsigned int)"},
    {0x1004e7c40u, 66u, "cSLTokenRecord::GetFieldFloat32(cGZPersistTokenDef const&, float&, unsigned int)"},
    {0x1004e7c82u, 102u, "cSLTokenRecord::SetFieldFloat32(cGZPersistTokenDef const&, float const&, unsigned int)"},
    {0x1004e7ce8u, 66u, "cSLTokenRecord::GetFieldFloat64(cGZPersistTokenDef const&, double&, unsigned int)"},
    {0x1004e7d2au, 102u, "cSLTokenRecord::SetFieldFloat64(cGZPersistTokenDef const&, double const&, unsigned int)"},
    {0x1004e7d90u, 66u, "cSLTokenRecord::GetFieldString(cGZPersistTokenDef const&, cIGZString&, unsigned int)"},
    {0x1004e7dd2u, 102u, "cSLTokenRecord::SetFieldString(cGZPersistTokenDef const&, cIGZString const&, unsigned int)"},
    {0x1004e7e38u, 66u, "cSLTokenRecord::GetFieldResKey(cGZPersistTokenDef const&, cGZPersistResourceKey&, unsigned int)"},
    {0x1004e7e7au, 102u, "cSLTokenRecord::SetFieldResKey(cGZPersistTokenDef const&, cGZPersistResourceKey const&, unsigned int)"},
    // cSC4ViewInputControlPlayerDrive — 28 entry points
    {0x1006f7200u, 396u, "cSC4ViewInputControlPlayerDrive::cSC4ViewInputControlPlayerDrive()"},
    {0x1006f73ceu, 1784u, "cSC4ViewInputControlPlayerDrive::Init()"},
    {0x1006f7e16u, 1178u, "cSC4ViewInputControlPlayerDrive::Shutdown()"},
    {0x1006f82b0u, 308u, "cSC4ViewInputControlPlayerDrive::StopTracking()"},
    {0x1006f83e4u, 240u, "cSC4ViewInputControlPlayerDrive::PauseEverything(bool)"},
    {0x1006f84d4u, 158u, "cSC4ViewInputControlPlayerDrive::HideUI()"},
    {0x1006f8666u, 608u, "cSC4ViewInputControlPlayerDrive::DoMessage(cIGZMessage2*)"},
    {0x1006f88c6u, 2704u, "cSC4ViewInputControlPlayerDrive::Update()"},
    {0x1006f9368u, 168u, "cSC4ViewInputControlPlayerDrive::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x1006f941au, 162u, "cSC4ViewInputControlPlayerDrive::EndInput()"},
    {0x1006f94bcu, 232u, "cSC4ViewInputControlPlayerDrive::SendCallbackMessage(cSC4ViewInputControlPlayerDrive::eCallbackEvent, unsigned int)"},
    {0x1006f95a4u, 156u, "cSC4ViewInputControlPlayerDrive::OnKeyDown(unsigned int, unsigned int)"},
    {0x1006f9640u, 680u, "cSC4ViewInputControlPlayerDrive::HandleKeyEffects(cISC4Automaton::ePlayerDriveCommand, bool)"},
    {0x1006f98e8u, 904u, "cSC4ViewInputControlPlayerDrive::HandleDriveCommandKeyDown(cISC4Automaton::ePlayerDriveCommand)"},
    {0x1006f9c70u, 140u, "cSC4ViewInputControlPlayerDrive::OnKeyUp(unsigned int, unsigned int)"},
    {0x1006f9cfcu, 736u, "cSC4ViewInputControlPlayerDrive::HandleDriveCommandKeyUp(cISC4Automaton::ePlayerDriveCommand)"},
    {0x1006f9fdcu, 68u, "cSC4ViewInputControlPlayerDrive::IsDriveAutoValid() const"},
    {0x1006fa020u, 444u, "cSC4ViewInputControlPlayerDrive::OnMouseDownL(int, int, unsigned int)"},
    {0x1006fa1dcu, 138u, "cSC4ViewInputControlPlayerDrive::PickNetworkOccupant(int, int, cISC4NetworkOccupant*&)"},
    {0x1006fa266u, 136u, "cSC4ViewInputControlPlayerDrive::PickAutomaton(int, int, cSC4Automaton*&)"},
    {0x1006fa2eeu, 866u, "cSC4ViewInputControlPlayerDrive::EnterDrivingMode(cSC4Automaton*, bool)"},
    {0x1006fa650u, 110u, "cSC4ViewInputControlPlayerDrive::OnMouseUpL(int, int, unsigned int)"},
    {0x1006fa6beu, 86u, "cSC4ViewInputControlPlayerDrive::OnMouseDownR(int, int, unsigned int)"},
    {0x1006fa714u, 82u, "cSC4ViewInputControlPlayerDrive::OnMouseUpR(int, int, unsigned int)"},
    {0x1006fa766u, 1460u, "cSC4ViewInputControlPlayerDrive::ShowUI(unsigned int)"},
    {0x1006fad1au, 80u, "cSC4ViewInputControlPlayerDrive::EnterChoosingMode()"},
    {0x1006fad6au, 130u, "cSC4ViewInputControlPlayerDrive::AddCallbackTarget(cIGZMessageTarget2*)"},
    {0x1006fadecu, 382u, "cSC4ViewInputControlPlayerDrive::OnMouseMove(int, int, unsigned int)"},
    // cSC4PowerLineTool — 26 entry points
    {0x1002e9c30u, 162u, "cSC4PowerLineTool::AddConnection(cSC4PowerLineTool::tPoleInfo&, cSC4PowerLineTool::tPoleInfo&, int)"},
    {0x1002e9cd2u, 86u, "cSC4PowerLineTool::AddPowerPoleInfo(cRZString const&, cRZString const&, void*)"},
    {0x1002e9d28u, 260u, "cSC4PowerLineTool::ComputePolePosition(int, int, float*)"},
    {0x1002e9e2cu, 370u, "cSC4PowerLineTool::CreatePowerPole(cSC4PowerLineTool::tPoleInfo&)"},
    {0x1002e9f9eu, 398u, "cSC4PowerLineTool::FindPoleOccupant(int, int, cISC4PowerPoleOccupant*&)"},
    {0x1002ea12cu, 234u, "cSC4PowerLineTool::cSC4PowerLineTool()"},
    {0x1002ea238u, 340u, "cSC4PowerLineTool::PlacePole(int, bool, int, cSC4PowerLineTool::tIntersectionInfo&)"},
    {0x1002ea38cu, 278u, "cSC4PowerLineTool::CellIsOccupiedByPowerLine(unsigned int, unsigned int, cISC4PowerLineOccupant*&)"},
    {0x1002ea4a2u, 80u, "cSC4PowerLineTool::IntersectionIsInList(cSC4PowerLineTool::tIntersectionInfo&)"},
    {0x1002ea4f2u, 162u, "cSC4PowerLineTool::ReleasePoleInfos()"},
    {0x1002ea594u, 130u, "cSC4PowerLineTool::GetPrimaryCell(int, int)"},
    {0x1002ea616u, 104u, "cSC4PowerLineTool::PoleIsInList(cSC4PowerPoleOccupant*)"},
    {0x1002ea67eu, 3330u, "cSC4PowerLineTool::DeterminePolePositions()"},
    {0x1002eb380u, 438u, "cSC4PowerLineTool::CellIsOKForPlacement(int, int)"},
    {0x1002eb536u, 680u, "cSC4PowerLineTool::FindNewPolePosition(SC4Rect<int>, int, int, int, std::__1::vector<SC4Point<unsigned int>, std::__1::allocator<SC4Point<unsigned int>>>&, cISC4PowerPoleOccupant*, cISC4PowerPoleOccupant*)"},
    {0x1002eb7deu, 3044u, "cSC4PowerLineTool::PlacePoles(bool)"},
    {0x1002ec3c2u, 1834u, "cSC4PowerLineTool::UpdateOnZoneChange(SC4Rect<int>, unsigned int)"},
    {0x1002ecaecu, 404u, "cSC4PowerLineTool::CheckForReservedTiles(int&)"},
    {0x1002ecc80u, 356u, "cSC4PowerLineTool::SetTerrainHilight(int)"},
    {0x1002ecde4u, 326u, "cSC4PowerLineTool::DoNetworkSpecificPlacement(bool, bool)"},
    {0x1002ecf2au, 378u, "cSC4PowerLineTool::Init()"},
    {0x1002ed0a4u, 34u, "cSC4PowerLineTool::StaticInit()"},
    {0x1002ed0c6u, 322u, "cSC4PowerLineTool::ClearCellPreviews()"},
    {0x1002ed208u, 290u, "cSC4PowerLineTool::ReadPowerPoleInfos()"},
    {0x1002ed348u, 52u, "cSC4PowerLineTool::GetCostOfSolution()"},
    {0x1002ed37cu, 100u, "cSC4PowerLineTool::GetFailureReasonString(int, cRZString&)"},
    // SC4GeometryUtilities — 23 entry points
    {0x1000d5e7eu, 200u, "SC4GeometryUtilities::RaysIntersect2D(float const*, float const*, float const*, float const*, float*)"},
    {0x1000d5f46u, 394u, "SC4GeometryUtilities::RayIntersectsBoundingBox(float const*, float const*, float const*, float const*, float*)"},
    {0x1000d60d0u, 210u, "SC4GeometryUtilities::LinesIntersect2D(float const*, float const*, float const*, float const*, float*)"},
    {0x1000d61a2u, 524u, "SC4GeometryUtilities::RayIntersectsTriangle(float const*, float const*, float const*, float const*, float const*, float*)"},
    {0x1000d63aeu, 278u, "SC4GeometryUtilities::TriangleToPlaneEquation(float const*, float const*, float const*, float*)"},
    {0x1000d64c4u, 228u, "SC4GeometryUtilities::PointDistanceToRay(float const*, float const*, float const*)"},
    {0x1000d65a8u, 156u, "SC4GeometryUtilities::PointDistanceToRectangle(float const*, float const*, float const*)"},
    {0x1000d6644u, 234u, "SC4GeometryUtilities::RayIntersectsPlane(float const*, float const*, float const*, float*)"},
    {0x1000d6756u, 50u, "SC4GeometryUtilities::PointInRectangle2D(float const*, float const*, float const*)"},
    {0x1000d6788u, 164u, "SC4GeometryUtilities::GetBoundingBoxOfVertices(float const (*) [3], int, float*, float*)"},
    {0x1000d682cu, 116u, "SC4GeometryUtilities::GetBoundingBoxOfVertices2D(float const (*) [2], int, float*, float*)"},
    {0x1000d68a0u, 226u, "SC4GeometryUtilities::BoundingBoxesIntersect(float const*, float const*, float const*, float const*, float*, float*)"},
    {0x1000d6982u, 126u, "SC4GeometryUtilities::GetNormalToTriangle(float const*, float const*, float const*, float*)"},
    {0x1000d6a00u, 88u, "SC4GeometryUtilities::CrossProduct(float const*, float const*, float*)"},
    {0x1000d6a58u, 126u, "SC4GeometryUtilities::GetAreaOfTriangle(float const*, float const*, float const*)"},
    {0x1000d6ad6u, 70u, "SC4GeometryUtilities::PointAndVectorToPlaneEquation(float const*, float const*, float*)"},
    {0x1000d6b1cu, 64u, "SC4GeometryUtilities::LengthOfLine(float const*, float const*)"},
    {0x1000d6b5cu, 60u, "SC4GeometryUtilities::LengthOfLineSquared(float const*, float const*)"},
    {0x1000d6bd8u, 66u, "SC4GeometryUtilities::ScaleVector2D(float*, float)"},
    {0x1000d6c1au, 178u, "SC4GeometryUtilities::RotatePoint(float const*, float const*, float, int, float*)"},
    {0x1000d6cccu, 134u, "SC4GeometryUtilities::RotatePoint2D(float const*, float const*, float, float*)"},
    {0x1000d6d52u, 560u, "SC4GeometryUtilities::ClipPolygonToEdge2D(float const*, float const*, float const (*) [2], int, float (*) [2], int, int&)"},
    {0x1000d6f82u, 278u, "SC4GeometryUtilities::ClipPolygonToPolygon2D(float const (*) [2], int, float const (*) [2], int, float (*) [2], int, int&)"},
    // SCLuaHelp — 22 entry points
    {0x1001e61e0u, 396u, "SCLuaHelp::ReadTableField(cISCLua*, char const*, cRZString&, int)"},
    {0x1001e636cu, 130u, "SCLuaHelp::ReadTableField(cISCLua*, char const*, float&, int)"},
    {0x1001e63eeu, 126u, "SCLuaHelp::ReadTableField(cISCLua*, char const*, unsigned int&, int)"},
    {0x1001e646cu, 126u, "SCLuaHelp::ReadTableField(cISCLua*, char const*, int&, int)"},
    {0x1001e64eau, 126u, "SCLuaHelp::ReadTableField(cISCLua*, char const*, bool&, int)"},
    {0x1001e6568u, 896u, "SCLuaHelp::ReadTableAsArray(cISCLua*, std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, int)"},
    {0x1001e68e8u, 286u, "SCLuaHelp::ReadTableAsArray(cISCLua*, std::__1::vector<float, std::__1::allocator<float>>&, int)"},
    {0x1001e6a06u, 282u, "SCLuaHelp::ReadTableAsArray(cISCLua*, std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&, int)"},
    {0x1001e6b20u, 296u, "SCLuaHelp::ReadTableAsArray(cISCLua*, std::__1::vector<int, std::__1::allocator<int>>&, int)"},
    {0x1001e6c48u, 398u, "SCLuaHelp::ReadTableAsList(cISCLua*, std::__1::list<cRZString, std::__1::allocator<cRZString>>&, int)"},
    {0x1001e6dd6u, 184u, "SCLuaHelp::ReadTableAsList(cISCLua*, std::__1::list<unsigned int, std::__1::allocator<unsigned int>>&, int)"},
    {0x1001e6e8eu, 1338u, "SCLuaHelp::ReadTableAsMap(cISCLua*, std::__1::unordered_map<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>, int, std::__1::hash<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>, std::__1::equal_to<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>>, std::__1::allocator<std::__1::pair<std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const, int>>>&, int)"},
    {0x1001e73c8u, 84u, "SCLuaHelp::GetField(cISCLua*, char const*, int)"},
    {0x1001e741cu, 66u, "SCLuaHelp::GetGlobal(cISCLua*, char const*)"},
    {0x1001e745eu, 238u, "SCLuaHelp::ReadTableRange(cISCLua*, char const*, float*, int)"},
    {0x1001e754cu, 238u, "SCLuaHelp::ReadTableRange(cISCLua*, char const*, int*, int)"},
    {0x1001e78f8u, 210u, "SCLuaHelp::LoadResourceScript(cISCLua*, cGZPersistResourceKey const&, char const*)"},
    {0x1001e79cau, 530u, "SCLuaHelp::LoadResourceScript(cISCLua*, unsigned int, char const*)"},
    {0x1001e7bdcu, 286u, "SCLuaHelp::LUAExtension_dofile_from_resource(lua_State*)"},
    {0x1001e7cfau, 1672u, "SCLuaHelp::UpdateResourcesFromDirectory(unsigned int, char const*)"},
    {0x1001e8382u, 1614u, "SCLuaHelp::LoadPackageScripts(cISCLua*, unsigned int, unsigned int)"},
    {0x1001e89d0u, 264u, "SCLuaHelp::LUAExtension_print(lua_State*)"},
    // cS3DTextureBindingFactory — 20 entry points
    {0x10030655eu, 302u, "cS3DTextureBindingFactory::cS3DTextureBindingFactory()"},
    {0x1003068deu, 412u, "cS3DTextureBindingFactory::Init()"},
    {0x100306a88u, 166u, "cS3DTextureBindingFactory::Shutdown()"},
    {0x100306b40u, 292u, "cS3DTextureBindingFactory::DoMessage(cIGZMessage2*)"},
    {0x100306c82u, 84u, "cS3DTextureBindingFactory::SetResGroup(unsigned int)"},
    {0x100306ce8u, 44u, "cS3DTextureBindingFactory::FlushFreePool()"},
    {0x100306d14u, 122u, "cS3DTextureBindingFactory::AdjustFreePoolSize()"},
    {0x100306d8eu, 80u, "cS3DTextureBindingFactory::FlushAll()"},
    {0x100306ddeu, 140u, "cS3DTextureBindingFactory::DestroyBinding(cS3DTextureBindingFactory::cBindingRec*)"},
    {0x100306e6au, 116u, "cS3DTextureBindingFactory::GetStatsString(cIGZString&)"},
    {0x100306edeu, 658u, "cS3DTextureBindingFactory::CreateNewBinding(cS3DTextureBufferInfo const*)"},
    {0x100307170u, 122u, "cS3DTextureBindingFactory::FindBindingParams(unsigned int, unsigned int, unsigned int, unsigned int&, unsigned int&, unsigned int&)"},
    {0x10030724eu, 312u, "cS3DTextureBindingFactory::SetBindingImageData(cS3DTextureBinding*, cS3DTextureBufferInfo const*)"},
    {0x100307386u, 438u, "cS3DTextureBindingFactory::GetFreeBinding(cS3DTextureBufferInfo const*)"},
    {0x10030753cu, 86u, "cS3DTextureBindingFactory::SetGDriver(cIGZGDriver*)"},
    {0x100307592u, 102u, "cS3DTextureBindingFactory::GetBinding(cS3DTextureBufferInfo const*, cS3DTextureBinding**)"},
    {0x1003075f8u, 370u, "cS3DTextureBindingFactory::GetBinding(cGZPersistResourceKey const&, cS3DTextureBinding**)"},
    {0x1003078aau, 44u, "cS3DTextureBindingFactory::GetBinding(unsigned int, cS3DTextureBinding**)"},
    {0x1003078d6u, 104u, "cS3DTextureBindingFactory::GetBindingInfo(cS3DTextureBinding const*, cS3DTextureBindingInfo*)"},
    {0x10030793eu, 148u, "cS3DTextureBindingFactory::FreeBinding(cS3DTextureBinding*)"},
    // cFreshDimensionPlayer — 19 entry points
    {0x10043ac26u, 60u, "cFreshDimensionPlayer::Create()"},
    {0x1004cf900u, 764u, "cFreshDimensionPlayer::cFreshDimensionPlayer()"},
    {0x1004cfcb2u, 84u, "cFreshDimensionPlayer::Init()"},
    {0x1004cfd14u, 98u, "cFreshDimensionPlayer::Play()"},
    {0x1004cfd88u, 52u, "cFreshDimensionPlayer::Stop()"},
    {0x1004cfe20u, 80u, "cFreshDimensionPlayer::SetVolume(int)"},
    {0x1004cfec8u, 390u, "cFreshDimensionPlayer::LoadScore(cIGZString const&, unsigned int)"},
    {0x1004d01fcu, 92u, "cFreshDimensionPlayer::GetScore(unsigned int)"},
    {0x1004d0266u, 354u, "cFreshDimensionPlayer::SetScoreEnabled(unsigned int, bool)"},
    {0x1004d03c8u, 152u, "cFreshDimensionPlayer::Reseed(unsigned int)"},
    {0x1004d0472u, 128u, "cFreshDimensionPlayer::SetXPos(int)"},
    {0x1004d0504u, 128u, "cFreshDimensionPlayer::SetYPos(int)"},
    {0x1004d0596u, 128u, "cFreshDimensionPlayer::SetZoom(int)"},
    {0x1004d0628u, 620u, "cFreshDimensionPlayer::UpdateCellPlayerMap()"},
    {0x1004d0a04u, 546u, "cFreshDimensionPlayer::Update(int)"},
    {0x1004d0c26u, 376u, "cFreshDimensionPlayer::CalculateCellplayer3DPosition(cIFreshCellPlayer*)"},
    {0x1004d0df8u, 56u, "cFreshDimensionPlayer::SetDivision3DPosition(unsigned int, unsigned int, int, int, int)"},
    {0x1004d0f38u, 20u, "cFreshDimensionPlayer::SetPlayPercent(int)"},
    {0x1004d0feeu, 32u, "cFreshDimensionPlayer::SetKey(cGZPersistResourceKey const&)"},
    // cSCExemplarCache — 18 entry points
    {0x10052ed40u, 252u, "cSCExemplarCache::cSCExemplarCache()"},
    {0x10052ef72u, 140u, "cSCExemplarCache::Init()"},
    {0x10052effeu, 258u, "cSCExemplarCache::Shutdown()"},
    {0x10052f100u, 194u, "cSCExemplarCache::DoMessage(cIGZMessage2*)"},
    {0x10052f1d0u, 264u, "cSCExemplarCache::processCohortChange(unsigned int, unsigned int)"},
    {0x10052f2d8u, 278u, "cSCExemplarCache::processExemplarAdd(unsigned int, bool)"},
    {0x10052f3eeu, 192u, "cSCExemplarCache::exemplarPluginCallback(cGZPersistResourceKey const&, void*)"},
    {0x10052f556u, 304u, "cSCExemplarCache::AddExemplar(cISCResExemplar*, bool)"},
    {0x10052f6fau, 166u, "cSCExemplarCache::RemoveExemplar(cISCResExemplar*, bool)"},
    {0x10052f7f4u, 274u, "cSCExemplarCache::AddCohort(cISCResExemplarCohort*)"},
    {0x10052f976u, 310u, "cSCExemplarCache::RemoveCohort(cISCResExemplarCohort*, bool)"},
    {0x10052fb00u, 324u, "cSCExemplarCache::PerformQuery(cISCExemplarCacheQueryDef*, cIGZUnknownList**)"},
    {0x10052fd52u, 606u, "cSCExemplarCache::RegisterRetainedQuery(cISCExemplarCacheQueryDef*)"},
    {0x10052ffbeu, 82u, "cSCExemplarCache::UnregisterRetainedQuery(unsigned int)"},
    {0x10053001eu, 130u, "cSCExemplarCache::PerformRetainedQuery(unsigned int)"},
    {0x1005300aeu, 86u, "cSCExemplarCache::GetRetainedQueryResult(unsigned int, cIGZUnknownList**)"},
    {0x100530178u, 146u, "cSCExemplarCache::updateRetainedQueries()"},
    {0x10053020au, 82u, "cSCExemplarCache::performQuery(cISCExemplarCacheQueryDef*, cIGZUnknownList*)"},
    // cHitIniFile — 17 entry points
    {0x1003e1a70u, 296u, "cHitIniFile::cHitIniFile(cIGZFile*)"},
    {0x1003e1ba2u, 306u, "cHitIniFile::cHitIniFile()"},
    {0x1003e1e20u, 60u, "cHitIniFile::Create()"},
    {0x1003e1e5cu, 1383u, "cHitIniFile::EnumDefaultEntries()"},
    {0x1003e240au, 392u, "cHitIniFile::InitPackedDBSegments()"},
    {0x1003e2592u, 592u, "cHitIniFile::InitTrackDataBlocks()"},
    {0x1003e27e2u, 712u, "cHitIniFile::InitHitManTypeGroups()"},
    {0x1003e2aaau, 286u, "cHitIniFile::InitDataSourceCreators()"},
    {0x1003e2bc8u, 884u, "cHitIniFile::InitSndCreators()"},
    {0x1003e2f3cu, 500u, "cHitIniFile::InitResourceFactories()"},
    {0x1003e315au, 246u, "cHitIniFile::SetFilename(cIGZString const&)"},
    {0x1003e3288u, 130u, "cHitIniFile::ProcessTrackDataBlockEntry(cRZString&)"},
    {0x1003e330au, 1810u, "cHitIniFile::ProcessPackedDBSegmentEntry(unsigned int, cRZString&)"},
    {0x1003e3a1cu, 1434u, "cHitIniFile::InitDBSegments()"},
    {0x1003e3fb6u, 302u, "cHitIniFile::ProcessDBSegmentsEntryCreate(unsigned int, cRZString&)"},
    {0x1003e40e4u, 644u, "cHitIniFile::ProcessDBSegmentDirectoryFilesEntry(unsigned int, cRZString&)"},
    {0x1003e4368u, 1008u, "cHitIniFile::ProcessDBSegmentsEntryInit(unsigned int, cRZString&)"},
    // CPCMPlayer — 16 entry points
    {0x10043c290u, 80u, "CPCMPlayer::CPCMPlayer()"},
    {0x10043c2e0u, 32u, "CPCMPlayer::CPCMPlayer()"},
    {0x10043c320u, 96u, "CPCMPlayer::Close()"},
    {0x10043c3a0u, 256u, "CPCMPlayer::Open(unsigned short, unsigned short, unsigned int, unsigned char*, int)"},
    {0x10043c4a0u, 208u, "CPCMPlayer::UpdateVolumeAndBalance()"},
    {0x10043c570u, 80u, "CPCMPlayer::UpdateFrequency()"},
    {0x10043c5c0u, 32u, "CPCMPlayer::IsOpen() const"},
    {0x10043c5e0u, 96u, "CPCMPlayer::Stop()"},
    {0x10043c640u, 176u, "CPCMPlayer::Play(unsigned char)"},
    {0x10043c6f0u, 80u, "CPCMPlayer::SetVolume(float)"},
    {0x10043c740u, 128u, "CPCMPlayer::SetVolumes(float, float)"},
    {0x10043c7c0u, 80u, "CPCMPlayer::SetBalance(float)"},
    {0x10043c810u, 64u, "CPCMPlayer::SetFrequency(unsigned int)"},
    {0x10043c850u, 32u, "CPCMPlayer::IsPlaying() const"},
    {0x10043c870u, 32u, "CPCMPlayer::SetIs3D(bool)"},
    {0x10043c890u, 32u, "CPCMPlayer::GetOriginalFrequency() const"},
    // cSC4ViewInputControlTorch — 14 entry points
    {0x100398290u, 156u, "cSC4ViewInputControlTorch::cSC4ViewInputControlTorch()"},
    {0x10039837eu, 140u, "cSC4ViewInputControlTorch::Init()"},
    {0x10039840au, 202u, "cSC4ViewInputControlTorch::SendTorchToolAudioMessage(unsigned int, unsigned int, unsigned int, unsigned int)"},
    {0x1003984d4u, 120u, "cSC4ViewInputControlTorch::Shutdown()"},
    {0x10039854cu, 92u, "cSC4ViewInputControlTorch::ShutdownTorchEffect()"},
    {0x1003985a8u, 14u, "cSC4ViewInputControlTorch::OnKeyDown(unsigned int, unsigned int)"},
    {0x1003985b6u, 240u, "cSC4ViewInputControlTorch::OnMouseDownL(int, int, unsigned int)"},
    {0x1003986a6u, 188u, "cSC4ViewInputControlTorch::ActivateTorch(bool)"},
    {0x100398762u, 86u, "cSC4ViewInputControlTorch::UpdateTorchEffect()"},
    {0x1003987b8u, 114u, "cSC4ViewInputControlTorch::OnMouseUpL(int, int, unsigned int)"},
    {0x10039882au, 218u, "cSC4ViewInputControlTorch::OnMouseMove(int, int, unsigned int)"},
    {0x100398904u, 74u, "cSC4ViewInputControlTorch::InitTorchEffect()"},
    {0x10039894eu, 14u, "cSC4ViewInputControlTorch::OnMouseExit()"},
    {0x10039895cu, 88u, "cSC4ViewInputControlTorch::DoMessage(cIGZMessage2*)"},
    // cSC4ViewInputControlLabelTool — 14 entry points
    {0x1006c869cu, 74u, "cSC4ViewInputControlLabelTool::cSC4ViewInputControlLabelTool()"},
    {0x1006c86e6u, 68u, "cSC4ViewInputControlLabelTool::Init()"},
    {0x1006c872au, 42u, "cSC4ViewInputControlLabelTool::Shutdown()"},
    {0x1006c8754u, 138u, "cSC4ViewInputControlLabelTool::SetSelection(cISC4TextDecalOccupant*)"},
    {0x1006c87deu, 280u, "cSC4ViewInputControlLabelTool::OnKeyDown(unsigned int, unsigned int)"},
    {0x1006c88f6u, 406u, "cSC4ViewInputControlLabelTool::UpdateTextDecal(cS3DVector3, cISC4TextDecalOccupant*)"},
    {0x1006c8a8cu, 558u, "cSC4ViewInputControlLabelTool::OnMouseDownL(int, int, unsigned int)"},
    {0x1006c8cbau, 266u, "cSC4ViewInputControlLabelTool::GetDecalOccupantAt(cS3DVector3&)"},
    {0x1006c8dc4u, 600u, "cSC4ViewInputControlLabelTool::DoEditLabelUI(cISC4TextDecalOccupant*)"},
    {0x1006c901cu, 236u, "cSC4ViewInputControlLabelTool::OnMouseUpL(int, int, unsigned int)"},
    {0x1006c9108u, 336u, "cSC4ViewInputControlLabelTool::OnMouseMove(int, int, unsigned int)"},
    {0x1006c9258u, 600u, "cSC4ViewInputControlLabelTool::FindAvenueCenterAndOrientation(cS3DVector3&, int&)"},
    {0x1006c94b0u, 728u, "cSC4ViewInputControlLabelTool::FindRoadCenterAndOrientation(cS3DVector3&, int&)"},
    {0x1006c9788u, 310u, "cSC4ViewInputControlLabelTool::DoMessage(cIGZWin*, cGZMessage&)"},
    // cSC4NewsTickerWinProc — 13 entry points
    {0x10031b896u, 406u, "cSC4NewsTickerWinProc::cSC4NewsTickerWinProc()"},
    {0x10031bc44u, 784u, "cSC4NewsTickerWinProc::Init(cIGZWinGen*)"},
    {0x10031c016u, 218u, "cSC4NewsTickerWinProc::Shutdown()"},
    {0x10031c0f0u, 208u, "cSC4NewsTickerWinProc::RegisterMessageNotification(bool)"},
    {0x10031c1c0u, 270u, "cSC4NewsTickerWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x10031c2e0u, 868u, "cSC4NewsTickerWinProc::DoMessage(cIGZMessage2*)"},
    {0x10031c644u, 284u, "cSC4NewsTickerWinProc::ShowTicker()"},
    {0x10031c760u, 128u, "cSC4NewsTickerWinProc::HideTicker()"},
    {0x10031c808u, 292u, "cSC4NewsTickerWinProc::AddMessage(cISC4Advice*)"},
    {0x10031c9a0u, 2180u, "cSC4NewsTickerWinProc::updateBodyTextCtrl()"},
    {0x10031d224u, 2300u, "cSC4NewsTickerWinProc::DisplayNewsFlipperLine(cISC4Advice*)"},
    {0x10031db20u, 70u, "cSC4NewsTickerWinProc::RemoveInactive()"},
    {0x10031db66u, 1348u, "cSC4NewsTickerWinProc::DoFlipperMessage()"},
    // cSC4ViewInputControlEffect — 12 entry points
    {0x100397140u, 198u, "cSC4ViewInputControlEffect::cSC4ViewInputControlEffect()"},
    {0x1003973aau, 102u, "cSC4ViewInputControlEffect::Init()"},
    {0x100397410u, 328u, "cSC4ViewInputControlEffect::Shutdown()"},
    {0x100397558u, 14u, "cSC4ViewInputControlEffect::OnKeyDown(unsigned int, unsigned int)"},
    {0x100397566u, 446u, "cSC4ViewInputControlEffect::OnMouseDownL(int, int, unsigned int)"},
    {0x100397724u, 308u, "cSC4ViewInputControlEffect::PlayTriggerEffect(cS3DTransform const&)"},
    {0x100397858u, 254u, "cSC4ViewInputControlEffect::OnMouseMove(int, int, unsigned int)"},
    {0x100397956u, 238u, "cSC4ViewInputControlEffect::OnMouseUpL(int, int, unsigned int)"},
    {0x100397a44u, 44u, "cSC4ViewInputControlEffect::OnMouseExit()"},
    {0x100397a70u, 1292u, "cSC4ViewInputControlEffect::SetEffectName(char const*)"},
    {0x100397f7cu, 704u, "cSC4ViewInputControlEffect::SetEffectGroupID(unsigned int)"},
    {0x10039823cu, 64u, "cSC4ViewInputControlEffect::SetTerrainControlledEffectState(bool)"},
    // cS3DVector3 — 12 entry points
    {0x1003f4970u, 64u, "cS3DVector3::cS3DVector3(float, float, float)"},
    {0x1003f5550u, 32u, "cS3DVector3::cS3DVector3()"},
    {0x1003f75c0u, 48u, "cS3DVector3::cS3DVector3(cS3DVector3 const&)"},
    {0x1003f7680u, 128u, "cS3DVector3::Normalize()"},
    {0x1003f78b0u, 64u, "cS3DVector3::operator=(cS3DVector3 const&)"},
    {0x1003f7d30u, 64u, "cS3DVector3::cS3DVector3(float, float, float)"},
    {0x1003f7e30u, 96u, "cS3DVector3::cS3DVector3()"},
    {0x1003f80d0u, 64u, "cS3DVector3::cS3DVector3(cS3DVector3 const&)"},
    {0x1003f8110u, 64u, "cS3DVector3::GetLength() const"},
    {0x1003f8170u, 32u, "cS3DVector3::operator[](int) const"},
    {0x100628210u, 64u, "cS3DVector3::SetValues(float, float, float)"},
    {0x100628c40u, 80u, "cS3DVector3::operator*=(float)"},
    // cSC4ScriptedWindowCOMDirector — 12 entry points
    {0x1006bf2c4u, 430u, "cSC4ScriptedWindowCOMDirector::cSC4ScriptedWindowCOMDirector()"},
    {0x1006bf472u, 62u, "cSC4ScriptedWindowCOMDirector::CreateSC4WinMapView()"},
    {0x1006bf4b0u, 62u, "cSC4ScriptedWindowCOMDirector::CreateSC4WinLotConfigurationChooser()"},
    {0x1006bf4eeu, 62u, "cSC4ScriptedWindowCOMDirector::CreateSC4WinLotConfigurationEditor()"},
    {0x1006bf53cu, 62u, "cSC4ScriptedWindowCOMDirector::CreateSC4WinNetworkLotChooser()"},
    {0x1006bf57au, 62u, "cSC4ScriptedWindowCOMDirector::CreateSC4WinNetworkLotEditor()"},
    {0x1006bf5b8u, 60u, "cSC4ScriptedWindowCOMDirector::CreateSC4WinRCI()"},
    {0x1006bf5f4u, 60u, "cSC4ScriptedWindowCOMDirector::CreateSC4WinTrendBar()"},
    {0x1006bf630u, 56u, "cSC4ScriptedWindowCOMDirector::CreateSC4WinGenTransparent()"},
    {0x1006bf668u, 60u, "cSC4ScriptedWindowCOMDirector::CreateSC4WinIMECandidateList()"},
    {0x1006bf6a4u, 64u, "cSC4ScriptedWindowCOMDirector::CreateGZWinKnob()"},
    {0x1006bf6e4u, 12u, "cSC4ScriptedWindowCOMDirector::GetDirectorID() const"},
    // cSC4EffectsParser — 11 entry points
    {0x1005c8c2au, 1170u, "cSC4EffectsParser::cSC4EffectsParser()"},
    {0x1005c97e4u, 268u, "cSC4EffectsParser::ParseBegin()"},
    {0x1005c9902u, 88u, "cSC4EffectsParser::ParseEndSuccess()"},
    {0x1005c996cu, 526u, "cSC4EffectsParser::ParseEnd()"},
    {0x1005c9b8cu, 952u, "cSC4EffectsParser::RegisterCommands()"},
    {0x1005c9f44u, 18u, "cSC4EffectsParser::ReadyToParse() const"},
    {0x1005c9f72u, 173u, "cSC4EffectsParser::SetManager(cISC4EffectsManager*)"},
    {0x1005ca0eau, 88u, "cSC4EffectsParser::StartTestEffects()"},
    {0x1005ca156u, 756u, "cSC4EffectsParser::AddDescription(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, char const*, tSC4ComponentEffectType, bool)"},
    {0x1005ca44au, 2970u, "cSC4EffectsParser::ParseDescRecOptions(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cSC4EffectDescription::cDescriptionRec&)"},
    {0x1005cafe4u, 596u, "cSC4EffectsParser::AddAnonDescription(std::__1::vector<cRZString, std::__1::allocator<cRZString>>&, cISC4ComponentDescription*, bool)"},
    // cSCStringDetokenizer — 10 entry points
    {0x10005d530u, 132u, "cSCStringDetokenizer::CaseInsensitiveStringCompare::operator()(cRZString const&, cRZString const&) const"},
    {0x10005d5b4u, 102u, "cSCStringDetokenizer::cSCStringDetokenizer()"},
    {0x10005d6e2u, 450u, "cSCStringDetokenizer::SetTokenReplacementValue(cIGZString const&, cIGZString const&)"},
    {0x10005d962u, 270u, "cSCStringDetokenizer::SetTokenReplacementMethod(cIGZString const&, bool (*)(cIGZString const&, cIGZString&, void*), void*)"},
    {0x10005db18u, 190u, "cSCStringDetokenizer::AddUnknownTokenReplacementMethod(bool (*)(cIGZString const&, cIGZString&, void*), void*, bool)"},
    {0x10005dbd6u, 14u, "cSCStringDetokenizer::Detokenize(cIGZString const&, cIGZString&)"},
    {0x10005dbe4u, 142u, "cSCStringDetokenizer::Detokenize(cGZPersistResourceKey const&, cIGZString&)"},
    {0x10005dc72u, 444u, "cSCStringDetokenizer::MakeHTMLClean(cIGZString&)"},
    {0x10005de2eu, 268u, "cSCStringDetokenizer::ConvertToken(cRZString const&, cRZString&)"},
    {0x10005df3au, 1652u, "cSCStringDetokenizer::PerformDetokenize(cIGZString const&, cIGZString&)"},
    // cSC4TipOfTheDayWinProc — 10 entry points
    {0x10032e516u, 76u, "cSC4TipOfTheDayWinProc::EntriesPresent()"},
    {0x10032e562u, 104u, "cSC4TipOfTheDayWinProc::LoadAllExemplarData(cSC4TipOfTheDayWinProc*)"},
    {0x10032e5cau, 382u, "cSC4TipOfTheDayWinProc::DoModelessWin(cSC4TipOfTheDayWinProc**)"},
    {0x10032e748u, 108u, "cSC4TipOfTheDayWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x10032e7b4u, 400u, "cSC4TipOfTheDayWinProc::InitDialog()"},
    {0x10032e944u, 102u, "cSC4TipOfTheDayWinProc::DoCommand(cGZMessage&)"},
    {0x10032e9aau, 468u, "cSC4TipOfTheDayWinProc::EndDialog()"},
    {0x10032eb7eu, 104u, "cSC4TipOfTheDayWinProc::GoToNextTip(bool)"},
    {0x10032ebe6u, 366u, "cSC4TipOfTheDayWinProc::GoToTip(cGZPersistResourceKey const&)"},
    {0x10032ed54u, 268u, "cSC4TipOfTheDayWinProc::LoadAllExemplarDataCallback(cGZPersistResourceKey const&, void*)"},
    // cSC4ModelMakerUtility — 9 entry points
    {0x1002c0280u, 328u, "cSC4ModelMakerUtility::GetModelInstanceID(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>> const&, unsigned int, unsigned int, int, int, int, bool, unsigned int*, unsigned int*)"},
    {0x1002c03dau, 210u, "cSC4ModelMakerUtility::GetModelInstanceID(cISC4Occupant*, int, int, int, bool, unsigned int*, unsigned int*)"},
    {0x1002c04acu, 144u, "cSC4ModelMakerUtility::GetAllModelInstanceIDs(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&, std::__1::vector<float, std::__1::allocator<float>>*, cISC4Occupant*, int, int, int, bool, unsigned int, unsigned int*, unsigned int*)"},
    {0x1002c053cu, 1610u, "cSC4ModelMakerUtility::GetAllModelInstanceIDs(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&, std::__1::vector<float, std::__1::allocator<float>>*, cISCPropertyHolder*, bool, int, int, int, bool, unsigned int, unsigned int*, unsigned int*)"},
    {0x1002c0ca4u, 60u, "cSC4ModelMakerUtility::FoundationOccupantUsesBuildingStyleModels(cISC4Occupant*)"},
    {0x1002c0ce0u, 144u, "cSC4ModelMakerUtility::OccupantUsesBuildingStyleModels(cISC4Occupant*)"},
    {0x1002c0d70u, 274u, "cSC4ModelMakerUtility::GetOccupantOrientationAndState(cISC4Occupant*, unsigned int&, int&)"},
    {0x1002c0e82u, 358u, "cSC4ModelMakerUtility::GetOccupantModelClass(cISC4Occupant*)"},
    {0x1002c0fe8u, 434u, "cSC4ModelMakerUtility::HasOccupantModelClass(cISC4Occupant*, unsigned int const*, unsigned int)"},
    // cHitDebugInfo — 9 entry points
    {0x100484b40u, 172u, "cHitDebugInfo::cHitDebugInfo()"},
    {0x100484dc4u, 224u, "cHitDebugInfo::AddSourceFile(cIGZString&, unsigned char*, int)"},
    {0x100484ea4u, 82u, "cHitDebugInfo::AddSourceLine(int, int)"},
    {0x100484ef6u, 246u, "cHitDebugInfo::GetDataAddressFromFilenameAndLineNum(cIGZString&, int, unsigned char**)"},
    {0x100484fecu, 150u, "cHitDebugInfo::GetFilenameAndLineNumFromDataAddress(unsigned char*, cIGZString&, int&)"},
    {0x100485082u, 196u, "cHitDebugInfo::GetDataAddressFromEntryPoint(unsigned int, unsigned char**)"},
    {0x100485146u, 184u, "cHitDebugInfo::GetHitFilenameFromEntryPoint(unsigned int, cIGZString&)"},
    {0x1004851feu, 1720u, "cHitDebugInfo::LoadDebugInfo(cIGZString&, unsigned char*, int)"},
    {0x1004858b6u, 446u, "cHitDebugInfo::LoadSymbolFile(cIGZString&)"},
    // cSC4VideoPreferencesWinProc — 8 entry points
    {0x1003a3aa0u, 360u, "cSC4VideoPreferencesWinProc::DoModalWin()"},
    {0x1003a3c08u, 212u, "cSC4VideoPreferencesWinProc::EndDialog()"},
    {0x1003a3cdcu, 2596u, "cSC4VideoPreferencesWinProc::ShowDialog(cSC4VideoPreferencesWinProc::eVideoWindowType)"},
    {0x1003a4700u, 6782u, "cSC4VideoPreferencesWinProc::SetButtonsBasedOnPreferences()"},
    {0x1003a61dau, 82u, "cSC4VideoPreferencesWinProc::FinalRelease()"},
    {0x1003a622cu, 232u, "cSC4VideoPreferencesWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x1003a6314u, 6660u, "cSC4VideoPreferencesWinProc::HandleControlMessage(unsigned int, cIGZWin*)"},
    {0x1003a7d18u, 3240u, "cSC4VideoPreferencesWinProc::SetPreferenceOptions()"},
    // cS3DTriangleIterator — 7 entry points
    {0x1001a9eb8u, 112u, "cS3DTriangleIterator::GetTriangleCountFromPrimitiveType(eGDPrimType, int)"},
    {0x1001a9f28u, 152u, "cS3DTriangleIterator::Init(cS3DMesh const&, int)"},
    {0x1001a9fc0u, 144u, "cS3DTriangleIterator::GetNextTrianglePoints(cS3DVector3*)"},
    {0x1001aa050u, 204u, "cS3DTriangleIterator::GetTriangleIndices(eGDPrimType, int, int*)"},
    {0x1001aa11cu, 576u, "cS3DTriangleIterator::GetIndicesFromIndexData(int const*, int*)"},
    {0x1001aa35cu, 118u, "cS3DTriangleIterator::GetPointsFromVertexData(int const*, cS3DVector3*)"},
    {0x1001aa3d2u, 132u, "cS3DTriangleIterator::GetNextTriangleIndices(int*)"},
    // cSC4DialogWinProcBase — 7 entry points
    {0x1003d6a5au, 386u, "cSC4DialogWinProcBase::Display(cIGZWin*)"},
    {0x1003d6bdcu, 192u, "cSC4DialogWinProcBase::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x1003d6c9cu, 42u, "cSC4DialogWinProcBase::GZOnButtonActivate(unsigned int)"},
    {0x1003d6cc6u, 376u, "cSC4DialogWinProcBase::TranslateTokensInCaption(unsigned int)"},
    {0x1003d6e3eu, 68u, "cSC4DialogWinProcBase::GetBtnControl(unsigned int)"},
    {0x1003d8676u, 50u, "cSC4DialogWinProcBase::GZOnCancel()"},
    {0x1003d86a8u, 52u, "cSC4DialogWinProcBase::GZOnOK()"},
    // cSC4UIHTMLHook — 6 entry points
    {0x1001f1c7cu, 50u, "cSC4UIHTMLHook::cSC4UIHTMLHook()"},
    {0x1001f1dbcu, 156u, "cSC4UIHTMLHook::Init()"},
    {0x1001f1e6au, 34u, "cSC4UIHTMLHook::Shutdown()"},
    {0x1001f1eaeu, 132u, "cSC4UIHTMLHook::TestURL(cRZURL&)"},
    {0x1001f1f3cu, 706u, "cSC4UIHTMLHook::LoadDocument(cRZURL&, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>&)"},
    {0x1001f21feu, 170u, "cSC4UIHTMLHook::LoadDocument(cGZPersistResourceKey const&, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>&)"},
    // cSC4CreditsWinProc — 6 entry points
    {0x1003cd546u, 202u, "cSC4CreditsWinProc::DoModalWin()"},
    {0x1003cd610u, 152u, "cSC4CreditsWinProc::ShowDialog(cSC4CreditsWinProc::eCreditsWindowType)"},
    {0x1003cd6a8u, 736u, "cSC4CreditsWinProc::InitDialog()"},
    {0x1003cd9d4u, 72u, "cSC4CreditsWinProc::FinalRelease()"},
    {0x1003cda1cu, 112u, "cSC4CreditsWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x1003cda8cu, 188u, "cSC4CreditsWinProc::EndDialog()"},
    // CQTRemotePlayer — 6 entry points
    {0x10069933eu, 160u, "CQTRemotePlayer::Open(char const*)"},
    {0x1006993deu, 40u, "CQTRemotePlayer::Close()"},
    {0x100699406u, 56u, "CQTRemotePlayer::Play(bool)"},
    {0x10069943eu, 22u, "CQTRemotePlayer::Stop()"},
    {0x100699454u, 28u, "CQTRemotePlayer::IsPlaying() const"},
    {0x100699470u, 22u, "CQTRemotePlayer::SetVolume(float)"},
    // cFreshScore — 5 entry points
    {0x10043abeeu, 56u, "cFreshScore::Create()"},
    {0x1004ca104u, 92u, "cFreshScore::cFreshScore()"},
    {0x1004ca25au, 220u, "cFreshScore::LoadScore(cIGZFile&)"},
    {0x1004ca336u, 4428u, "cFreshScore::ParseScore(cRZString&)"},
    {0x1004cb482u, 308u, "cFreshScore::LoadScore(cRZString)"},
    // cHitPropertySetResource — 5 entry points
    {0x10047204cu, 56u, "cHitPropertySetResource::Write_XML(cIGZOStream&, unsigned int, cIGZUnknown*)"},
    {0x100472084u, 60u, "cHitPropertySetResource::Write_Binary(cIGZOStream&, unsigned int, cIGZUnknown*)"},
    {0x1004720fcu, 54u, "cHitPropertySetResource::Read_XML(cIGZIStream&, unsigned int, cIGZUnknown*)"},
    {0x100472132u, 54u, "cHitPropertySetResource::Read_Binary(cIGZIStream&, unsigned int, cIGZUnknown*)"},
    {0x100472c9au, 164u, "cHitPropertySetResource::CreatePrivateCopy(unsigned int, void**)"},
    // cS3DResSpriteAnimFactory — 5 entry points
    {0x1005500a0u, 344u, "cS3DResSpriteAnimFactory::CreateInstance(unsigned int, unsigned int, void**)"},
    {0x1005501f8u, 82u, "cS3DResSpriteAnimFactory::SerialRead(cIGZPersistResource&, cIGZPersistDBSerialRecord&)"},
    {0x10055024au, 490u, "cS3DResSpriteAnimFactory::ReadAnim(cS3DResSpriteAnim&, cIGZPersistDBSerialRecord&)"},
    {0x100550434u, 158u, "cS3DResSpriteAnimFactory::ReadTable(cS3DResSpriteAnimTable&, cIGZPersistDBSerialRecord&)"},
    {0x1005504d2u, 36u, "cS3DResSpriteAnimFactory::cS3DResSpriteAnimFactory()"},
    // cSTEVertBufExtn — 5 entry points
    {0x1007514d4u, 570u, "cSTEVertBufExtn::DrawPrimsIndexedUsingVertBuf(int, int, int)"},
    {0x10075170eu, 138u, "cSTEVertBufExtn::LoadVertRowIntoBuf(int, int, int)"},
    {0x100751798u, 220u, "cSTEVertBufExtn::FlushBuffer()"},
    {0x100751874u, 329u, "cSTEVertBufExtn::GetVertexBufferExtension(cIGZGDriver*)"},
    {0x100751a26u, 36u, "cSTEVertBufExtn::ReleaseVertexBufferExtension()"},
    // cStatVecRange — 4 entry points
    {0x1000ef2fcu, 74u, "cStatVecRange::cStatVecRange(std::__1::list<cSC4HistoryNode, std::__1::allocator<cSC4HistoryNode>>&, cIGZDate const&, cIGZDate const&)"},
    {0x1000ef346u, 138u, "cStatVecRange::ComputeMeans()"},
    {0x1000ef3d0u, 158u, "cStatVecRange::ComputeStandardDeviations()"},
    {0x1000ef46eu, 168u, "cStatVecRange::ComputePearsonsCorrelation()"},
    // cS3DTransform — 4 entry points
    {0x10017670cu, 816u, "cS3DTransform::TransformPoints(cS3DVector3 const*, cS3DVector3*, int) const"},
    {0x1001a717eu, 98u, "cS3DTransform::operator=(cS3DTransform const&)"},
    {0x1001e4198u, 962u, "cS3DTransform::Rotate(cS3DVector3 const&, float)"},
    {0x1005f0c20u, 596u, "cS3DTransform::operator*=(cS3DTransform const&)"},
    // cSC4CompassWinProc — 4 entry points
    {0x10032ef20u, 86u, "cSC4CompassWinProc::cSC4CompassWinProc()"},
    {0x10032f10au, 148u, "cSC4CompassWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x10032f19eu, 116u, "cSC4CompassWinProc::DoMessage(cIGZMessage2*)"},
    {0x10032f212u, 140u, "cSC4CompassWinProc::UpdateRotation()"},
    // cPropertySetUint32XmlReader — 4 entry points
    {0x10043b062u, 111u, "cPropertySetUint32XmlReader::Create()"},
    {0x100472178u, 812u, "cPropertySetUint32XmlReader::HandleStartElement(char const*, char const**, unsigned int, unsigned int)"},
    {0x100472562u, 1118u, "cPropertySetUint32XmlReader::HandleEndElement(char const*)"},
    {0x1004729c0u, 248u, "cPropertySetUint32XmlReader::HandleCharData(char const*, unsigned int)"},
    // cGZPropertySetSerializerXmlT — 4 entry points
    {0x100476a58u, 1254u, "cGZPropertySetSerializerXmlT<cKeyStandardTraitsT<unsigned int, 3954248925ul, 1806839612ul>>::HandleStartElement(char const*, char const**, unsigned int, unsigned int)"},
    {0x100476f3eu, 722u, "cGZPropertySetSerializerXmlT<cKeyStandardTraitsT<unsigned int, 3954248925ul, 1806839612ul>>::HandleEndElement(char const*)"},
    {0x100477210u, 58u, "cGZPropertySetSerializerXmlT<cKeyStandardTraitsT<unsigned int, 3954248925ul, 1806839612ul>>::HandleCharData(char const*, unsigned int)"},
    {0x100477438u, 150u, "cGZPropertySetSerializerXmlT<cKeyStandardTraitsT<unsigned int, 3954248925ul, 1806839612ul>>::GetPropertySetTypeString(cRZString&, unsigned int const&)"},
    // cSC4SituationUIImageWinProc — 4 entry points
    {0x1006f3c60u, 108u, "cSC4SituationUIImageWinProc::cSC4SituationUIImageWinProc(int)"},
    {0x1006f3d6cu, 98u, "cSC4SituationUIImageWinProc::Init(cIGZWin*)"},
    {0x1006f3dceu, 88u, "cSC4SituationUIImageWinProc::Enable(bool)"},
    {0x1006f3e26u, 122u, "cSC4SituationUIImageWinProc::DoPaint()"},
    // SC4PathInfoView — 3 entry points
    {0x100164daeu, 88u, "SC4PathInfoView::SC4PathInfoView()"},
    {0x100164e06u, 188u, "SC4PathInfoView::Init()"},
    {0x100164ed2u, 2488u, "SC4PathInfoView::Update(SC4DrawContext*)"},
    // SC4QuadGridQuadrant — 3 entry points
    {0x100366d80u, 60u, "SC4QuadGridQuadrant::AddElement(SC4GridElement*)"},
    {0x100366dbcu, 108u, "SC4QuadGridQuadrant::RemoveElement(SC4GridElement*)"},
    {0x100366e28u, 82u, "SC4QuadGridQuadrant::RemoveAllElements()"},
    // cFreshnessCOMDirector — 3 entry points
    {0x10043aa60u, 130u, "cFreshnessCOMDirector::GetDirector()"},
    {0x10043aafeu, 180u, "cFreshnessCOMDirector::cFreshnessCOMDirector()"},
    {0x10043acfcu, 12u, "cFreshnessCOMDirector::GetDirectorID() const"},
    // cCompletionObj — 3 entry points
    {0x1004aa2c0u, 32u, "cCompletionObj::cCompletionObj()"},
    {0x1004afd70u, 64u, "cCompletionObj::cCompletionObj()"},
    {0x1004aff40u, 16u, "cCompletionObj::DoCompletionFunction(cIGZSnd*)"},
    // cSCBoolTextEntryView — 3 entry points
    {0x100524aa8u, 422u, "cSCBoolTextEntryView::Init()"},
    {0x100524c6eu, 416u, "cSCBoolTextEntryView::GetValue(cIGZVariant&)"},
    {0x100524e0eu, 166u, "cSCBoolTextEntryView::convertToBool(cRZString&)"},
    // cSC4ParticlesDescription — 3 entry points
    {0x1005b5406u, 44u, "cSC4ParticlesDescription::cSC4ParticlesDescription()"},
    {0x1005b5432u, 24u, "cSC4ParticlesDescription::operator=(cSC4ParticlesDescription const&)"},
    {0x1005b6740u, 1546u, "cSC4ParticlesDescription::ReadVersion1(cIGZIStream&)"},
    // iTunesPlayer — 3 entry points
    {0x1007b6c38u, 25u, "iTunesPlayer::Stop()"},
    {0x1007b6c51u, 101u, "iTunesPlayer::PlayPlaylist(__CFString const*, bool, bool)"},
    {0x1007b6cb6u, 76u, "iTunesPlayer::IsPlaying()"},
    // cS3DMeshAndMaterial — 2 entry points
    {0x1000d8802u, 50u, "cS3DMeshAndMaterial::cS3DMeshAndMaterial()"},
    {0x1000d88b2u, 24u, "cS3DMeshAndMaterial::MeshCount() const"},
    // MiscGraphPicture — 2 entry points
    {0x10037c224u, 52u, "MiscGraphPicture::SetBuffer(cIGZBuffer*)"},
    {0x10037c258u, 224u, "MiscGraphPicture::DrawSelf(cIGZGraph*, cIGZBuffer*)"},
    // cAmbienceFilterIsZoom5 — 2 entry points
    {0x1003def8cu, 38u, "cAmbienceFilterIsZoom5::Create()"},
    {0x1003e48ceu, 46u, "cAmbienceFilterIsZoom5::IsTrue()"},
    // cAmbienceFilterIsZoom1 — 2 entry points
    {0x1003df024u, 38u, "cAmbienceFilterIsZoom1::Create()"},
    {0x1003e4986u, 46u, "cAmbienceFilterIsZoom1::IsTrue()"},
    // cAmbienceFilterIsSimSpeedPaused — 2 entry points
    {0x1003df0e2u, 38u, "cAmbienceFilterIsSimSpeedPaused::Create()"},
    {0x1003e4a1cu, 54u, "cAmbienceFilterIsSimSpeedPaused::IsTrue()"},
    // cAmbienceFilterIsDrivingMode — 2 entry points
    {0x1003df17au, 38u, "cAmbienceFilterIsDrivingMode::Create()"},
    {0x1003e4ab0u, 22u, "cAmbienceFilterIsDrivingMode::IsTrue()"},
    // cAmbienceCellFilterIsSubway — 2 entry points
    {0x1003df844u, 50u, "cAmbienceCellFilterIsSubway::Create()"},
    {0x1003e5eceu, 116u, "cAmbienceCellFilterIsSubway::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsCommercial — 2 entry points
    {0x1003df90cu, 50u, "cAmbienceCellFilterIsCommercial::Create()"},
    {0x1003e607eu, 94u, "cAmbienceCellFilterIsCommercial::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsSpaceport — 2 entry points
    {0x1003df9d4u, 50u, "cAmbienceCellFilterIsSpaceport::Create()"},
    {0x1003e61f2u, 92u, "cAmbienceCellFilterIsSpaceport::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsHighGrowthStage — 2 entry points
    {0x1003dfaceu, 50u, "cAmbienceCellFilterIsHighGrowthStage::Create()"},
    {0x1003e6682u, 258u, "cAmbienceCellFilterIsHighGrowthStage::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsLowDesirability — 2 entry points
    {0x1003dfb96u, 50u, "cAmbienceCellFilterIsLowDesirability::Create()"},
    {0x1003e68d0u, 110u, "cAmbienceCellFilterIsLowDesirability::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsStreet — 2 entry points
    {0x1003dfc5eu, 50u, "cAmbienceCellFilterIsStreet::Create()"},
    {0x1003e5d72u, 116u, "cAmbienceCellFilterIsStreet::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsIndustrialDirty — 2 entry points
    {0x1003dfd26u, 50u, "cAmbienceCellFilterIsIndustrialDirty::Create()"},
    {0x1003e6be4u, 358u, "cAmbienceCellFilterIsIndustrialDirty::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsZoo — 2 entry points
    {0x1003dfdeeu, 50u, "cAmbienceCellFilterIsZoo::Create()"},
    {0x1003e6f92u, 146u, "cAmbienceCellFilterIsZoo::IsTrueCell(int, int)"},
    // cS3DColorARGB8 — 2 entry points
    {0x1003f4a00u, 96u, "cS3DColorARGB8::cS3DColorARGB8(unsigned char, unsigned char, unsigned char, unsigned char)"},
    {0x1003f7d70u, 64u, "cS3DColorARGB8::cS3DColorARGB8(unsigned char, unsigned char, unsigned char, unsigned char)"},
    // cS3DResModelFactory — 2 entry points
    {0x1004667fcu, 352u, "cS3DResModelFactory::CreateInstance(unsigned int, unsigned int, void**)"},
    {0x10046695cu, 6528u, "cS3DResModelFactory::SerialRead(cIGZPersistResource&, cIGZPersistDBSerialRecord&)"},
    // cSCStandardTypeEntryView — 2 entry points
    {0x100523b10u, 220u, "cSCStandardTypeEntryView::cSCStandardTypeEntryView()"},
    {0x100523c9cu, 374u, "cSCStandardTypeEntryView::AddBackgroundFillControl()"},
    // cSCSint32TextEntryView — 2 entry points
    {0x100524718u, 412u, "cSCSint32TextEntryView::Init()"},
    {0x1005248d4u, 450u, "cSCSint32TextEntryView::GetValue(cIGZVariant&)"},
    // cSCUint32SpinnerEntryView — 2 entry points
    {0x100525998u, 234u, "cSCUint32SpinnerEntryView::Init()"},
    {0x100525aa2u, 318u, "cSCUint32SpinnerEntryView::GetValue(cIGZVariant&)"},
    // cSC4CameraDescription — 2 entry points
    {0x100571bc8u, 102u, "cSC4CameraDescription::cSC4CameraDescription()"},
    {0x100571ce8u, 72u, "cSC4CameraDescription::Instantiate()"},
    // cSC4AttractorDescription — 2 entry points
    {0x100574fbcu, 86u, "cSC4AttractorDescription::cSC4AttractorDescription()"},
    {0x1005750aeu, 72u, "cSC4AttractorDescription::Instantiate()"},
    // MacDebug — 2 entry points
    {0x10063e89cu, 202u, "MacDebug::MessageToConsole(char const*, ...)"},
    {0x10063e966u, 538u, "MacDebug::MessageToAlert(char const*, ...)"},
    // cSTEEdgeReconcileInformNoMismatch — 2 entry points
    {0x100726ec2u, 362u, "cSTEEdgeReconcileInformNoMismatch::DoDialog()"},
    {0x10072702cu, 122u, "cSTEEdgeReconcileInformNoMismatch::DoMessage(cIGZWin*, cGZMessage&)"},
    // LinearRandomIterator — 1 entry point
    {0x10008daf0u, 384u, "LinearRandomIterator::Reset(bool)"},
    // tBehaviorTemplate — 1 entry point
    {0x10010ee00u, 146u, "tBehaviorTemplate::tBehaviorTemplate()"},
    // tKeyEffect — 1 entry point
    {0x1001e2d58u, 224u, "tKeyEffect::tKeyEffect(tKeyEffect const&)"},
    // cSC4WinIntroVideoScreen — 1 entry point
    {0x10027017au, 86u, "cSC4WinIntroVideoScreen::ResumeMovie()"},
    // cSC4GenericDialogWinProc — 1 entry point
    {0x10030bdceu, 66u, "cSC4GenericDialogWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    // MiscGraphArrow — 1 entry point
    {0x10037c11cu, 140u, "MiscGraphArrow::DrawSelf(cIGZGraph*, cIGZBuffer*)"},
    // cLUAExtensionsCollector — 1 entry point
    {0x1003ff7f8u, 3842u, "cLUAExtensionsCollector::cLUAExtensionsCollector()"},
    // cHitSem — 1 entry point
    {0x1004c26deu, 28u, "cHitSem::GetName(cIGZString&) const"},
    // WinGridCellData — 1 entry point
    {0x10054a662u, 122u, "WinGridCellData::WinGridCellData()"},
    // cSC4CameraParams — 1 entry point
    {0x100579f70u, 298u, "cSC4CameraParams::cSC4CameraParams()"},
    // CookieData — 1 entry point
    {0x100630814u, 266u, "CookieData::CookieData(CookieData const&)"},
    // CTextureCoordArray — 1 entry point
    {0x10063ad68u, 56u, "CTextureCoordArray::SetPointer(int, unsigned int, int, void const*)"},
    // CMacLogFile — 1 entry point
    {0x100641af4u, 84u, "CMacLogFile::WriteMessage(char const*)"},
    // HELI_TYPE_DATA — 1 entry point
    {0x1006f1fe6u, 198u, "HELI_TYPE_DATA::HELI_TYPE_DATA(HELI_TYPE_DATA const&)"},
    // cImageARGB8 — 1 entry point
    {0x10077b15au, 370u, "cImageARGB8::SampleColorLerp(float, float)"},
}};

constexpr std::size_t Wave4Part1MethodCount() noexcept {
    return kWave4Part1Methods.size();
}

constexpr const Wave4Part1Method* FindWave4Part1Method(Uint64 address) noexcept {
    for (const Wave4Part1Method& method : kWave4Part1Methods) {
        if (method.address == address) {
            return &method;
        }
    }
    return nullptr;
}

static_assert(Wave4Part1MethodCount() == 809u);
static_assert(FindWave4Part1Method(0x1002c1dcau) != nullptr);
static_assert(FindWave4Part1Method(0x1002c1dcau)->sizeBytes == 52u);
static_assert(FindWave4Part1Method(0x1002cfb90u) != nullptr);
static_assert(FindWave4Part1Method(0x1002cfb90u)->sizeBytes == 10534u);
static_assert(FindWave4Part1Method(0x100727300u) != nullptr);
static_assert(FindWave4Part1Method(0x100727300u)->sizeBytes == 198u);
static_assert(FindWave4Part1Method(0x1001e6e8eu) != nullptr);
static_assert(FindWave4Part1Method(0x1001e6e8eu)->sizeBytes == 1338u);
static_assert(FindWave4Part1Method(0x1007b6c38u) != nullptr);
static_assert(FindWave4Part1Method(0x1007b6c38u)->sizeBytes == 25u);
static_assert(FindWave4Part1Method(0x10077b15au) != nullptr);
static_assert(FindWave4Part1Method(0x10077b15au)->sizeBytes == 370u);
static_assert(FindWave4Part1Method(0) == nullptr);
static_assert(kWave4Part1Methods.front().address == 0x1002c1dcau);
static_assert(kWave4Part1Methods.back().address == 0x10077b15au);
} // namespace opensc4
