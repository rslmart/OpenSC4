#pragma once

// Exact decompilation of the automata (.sc4 Batch 3b) save subfiles from the
// macOS x86_64 SimCity 4 Deluxe binary:
//
//   States (cSC4Automaton::CreateAutomataState @ 0x10010d96a):
//     tAutomataState            (Write @ 0x10010e164, Read @ 0x10010e214)
//     tAutomataStateRiotLeader  (type 8,    Write @ 0x10010e390)
//     tAutomataStateRandomWalk  (type 11,   Write @ 0x10010e2e2)
//     tAutomataStatePlummet     (type 14,   Write @ 0x10010e4d4)
//     tAutomataStatePlopJump    (type 15,   Write @ 0x10010e43e)
//     tAutomataStatePlayerDrive (type 16,   Write @ 0x10010e678)
//     tAutomataStateBumped      (type 17,   Write @ 0x10010e70a)
//     tAutomataStateAttachAnim  (type 19,   Write @ 0x10010e884)
//     tAutomataStateCollide     (type 20,   Write @ 0x10010e7ca)
//     tAutomataStatePullOver    (type 21,   Write @ 0x10010e5c2)
//     tAutomataStateChase       (type 0x34, Write @ 0x10010e984)
//     every other type id creates a plain tAutomataState.
//   Automata:
//     cSC4Automaton             (Write @ 0x100106266, Read @ 0x10010662c)
//     cSC4TrafficAutomaton      (Write @ 0x10011aa6a, Read @ 0x10011ac58)
//     cSC4Pedestrian 0x896e75af (Read @ 0x1000f36ce)
//     cSC4Vehicle 0x896e714a    (Write @ 0x1000dd022, Read @ 0x1000dd1b6)
//     cSC4EmergencyVehicle      (Read @ 0x1002f7fbc)
//     cSC4PatrolCar 0x4a222b98  (Read @ 0x10050eeb0)
//     cSC4TrailerVehicle 0x4a5b69ed (Read @ 0x100508b78)
//     cSC4CarjackedVehicle 0xcbb32b0a, cSC4TrainCar 0x4a416254,
//     cSC4Train 0x8a41625a, cSC4MassTransitTrain 0x4c22ae53,
//     cSC4Aircraft 0xc96e75dc, cSC4Watercraft 0x096e75f5
//   Managers:
//     cSC4AutomataManager 0x498f9cb5 (Write @ 0x1001047a0, ReadInternal @ 0x1001048aa)
//     cSC4TrafficAutomataManager (Write @ 0x100116078, Read @ 0x1001161b4) used by
//       cSC4VehicleManager 0xa98f9d3b and cSC4PedestrianManager 0xc98f9d17
//     cSC4TrafficEffectManager (Write @ 0x1001160f2, Read @ 0x10011627c)
//     cSC4AircraftManager 0xc98f9d4a (Write @ 0x100500680, Read @ 0x1005007c2)
//     cSC4WatercraftManager 0xa98f9d28 (Write @ 0x1004ef61a, Read @ 0x1004ef802)
//     cSC4AutomataControllerManager 0xca14ca03 (Write @ 0x1000f48de, Read @ 0x1000f4af8)
//   Controllers:
//     cSC4AutomataController 0x4a18672b (Write @ 0x1000f9b44, Read @ 0x1000f9c82)
//     cSC4AutomataGenerator 0x4a1628c5 (Write @ 0x1000fb662, Read @ 0x1000fb710)
//     cSC4AutomataAttractor 0x6a1628ba (Write @ 0x1000fa1c4, Read @ 0x1000fa214)

#include "opensc4/sc4_subfiles.h"
#include "opensc4/sc4_subfiles_batch2.h"

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace opensc4 {

constexpr GZCLSID kCLSID_cSC4AutomatonVehicle         = 0x896e714au; // cSC4Vehicle
constexpr GZCLSID kCLSID_cSC4AutomatonPedestrian      = 0x896e75afu; // cSC4Pedestrian
constexpr GZCLSID kCLSID_cSC4AutomatonPatrolCar       = 0x4a222b98u;
constexpr GZCLSID kCLSID_cSC4AutomatonTrailerVehicle  = 0x4a5b69edu;
constexpr GZCLSID kCLSID_cSC4AutomatonCarjacked       = 0xcbb32b0au;
constexpr GZCLSID kCLSID_cSC4AutomatonTrainCar        = 0x4a416254u;
constexpr GZCLSID kCLSID_cSC4AutomatonTrain           = 0x8a41625au;
constexpr GZCLSID kCLSID_cSC4AutomatonMassTransitTrain = 0x4c22ae53u;
constexpr GZCLSID kCLSID_cSC4AutomatonAircraft        = 0xc96e75dcu;
constexpr GZCLSID kCLSID_cSC4AutomatonWatercraft      = 0x096e75f5u;
constexpr GZCLSID kCLSID_cSC4AutomataManagerBase      = 0x498f9cb5u;
constexpr GZCLSID kCLSID_cSC4VehicleManager           = 0xa98f9d3bu;
constexpr GZCLSID kCLSID_cSC4PedestrianManager        = 0xc98f9d17u;
constexpr GZCLSID kCLSID_cSC4AircraftManager          = 0xc98f9d4au;
constexpr GZCLSID kCLSID_cSC4WatercraftManager        = 0xa98f9d28u;
constexpr GZCLSID kCLSID_cSC4AutomataControllerManager = 0xca14ca03u;
constexpr GZCLSID kCLSID_cSC4AutomataControllerBase   = 0x4a18672bu;
constexpr GZCLSID kCLSID_cSC4AutomataGenerator        = 0x4a1628c5u;
constexpr GZCLSID kCLSID_cSC4AutomataAttractor        = 0x6a1628bau;
constexpr GZCLSID kCLSID_cSC4AdvisorBase              = 0xca09f543u; // cSC4AdvisorBase (Write @ 0x100443eaa)
constexpr Uint32  kSC4TypeIdTerrainMisc               = 0x8a91e7e3u; // cSTETerrain::SaveMiscTerrainData @ 0x10073f1c8
constexpr Uint32  kSC4TypeIdTerrainVertexProps        = 0x8a91e7e0u; // cSTETerrain::SaveVertexProperties @ 0x10073f34c
constexpr Uint32  kSC4TypeIdTerrainAltitudes          = 0xa9dd6ff4u; // cSTETerrain::SaveAltitudes / SaveEdgeAltitudes
constexpr Uint32  kSC4TypeIdView3DWin                 = 0xa9c8647cu; // cSC4View3DWin::DoSave @ 0x1007871e4
constexpr Uint32  kSC4TypeIdRegionalCity              = 0xca027edbu; // SC4WriteRegionalCity @ 0x1001e97dc
constexpr Uint32  kSC4TypeIdStringRecord              = 0x2026960bu; // SC4WriteStringRecord @ 0x1001e94ea
constexpr Uint32  kSC4TypeIdRegionViewThumbnail       = 0x8a2482b9u; // SC4WriteCityRegionViewThumbnailBitmapFile @ 0x1001d3b5d
constexpr Uint32  kSC4TypeIdDBPFCompressedSet         = 0xe86b1eefu; // cGZDBSegmentPackedFile::WriteCompressedSetRecord @ 0x10036af30

// Additional cIGZSerializable classes in the binary (active-event / filter / resource CLSIDs):
constexpr GZCLSID kCLSID_cSC4Airplane                        = 0xcbeb4f62u; // Write @ 0x1006ce480, Read @ 0x1006ce51e
constexpr GZCLSID kCLSID_cSC4Ambulance                       = 0x6c1acde1u; // Write @ 0x1006d5eb6, Read @ 0x1006d5f6a
constexpr GZCLSID kCLSID_cSC4AnimationTickManager            = 0xa9c73857u; // Write @ 0x100365236, Read @ 0x1003652d8
constexpr GZCLSID kCLSID_cSC4Bus                             = 0xaa9f4a3du; // Write @ 0x100506eec, Read @ 0x100506fa0
constexpr GZCLSID kCLSID_cSC4DisasterInstanceRiot            = 0x4a38206du; // Write @ 0x10044e65e, Read @ 0x10044e738
constexpr GZCLSID kCLSID_cSC4FirePlaneDispatchSource         = 0x4c0ae114u; // Write @ 0x100723256, Read @ 0x1007232ac
constexpr GZCLSID kCLSID_cSC4FireTruck                       = 0xaa222b83u; // Write @ 0x1005106d4, Read @ 0x10051074c
constexpr GZCLSID kCLSID_cSC4GetawayVan                      = 0xea863423u; // Write @ 0x10050744e, Read @ 0x1005074c4
constexpr GZCLSID kCLSID_cSC4Helicopter                      = 0x8ba98edau; // Write @ 0x1006ea8b2, Read @ 0x1006ea92c
constexpr GZCLSID kCLSID_cSC4MayorLimo                       = 0x4c0dd224u; // Write @ 0x1006f3814, Read @ 0x1006f38c8
constexpr GZCLSID kCLSID_cSC4MySimDispatch                   = 0xcbc14674u; // Write @ 0x1006ad8aa, Read @ 0x1006adcee
constexpr GZCLSID kCLSID_cSC4Nation                          = 0xc9dc0477u; // Write @ 0x10028988c, Read @ 0x1002898e4
constexpr GZCLSID kCLSID_cSC4PedestrianOneShot               = 0xaa09dc28u; // inherits cSC4Pedestrian
constexpr GZCLSID kCLSID_cSC4PoliceHelicopterDispatchSource  = 0x0c0b06e8u; // Write @ 0x100724ab0, Read @ 0x100724b06
constexpr GZCLSID kCLSID_cSC4RiotPolice                      = 0x8a6e4fa4u; // Write @ 0x100503438, Read @ 0x1005034e2
constexpr GZCLSID kCLSID_cSC4SimFireCrew                     = 0x4a222e4du; // Write @ 0x100506580, Read @ 0x100506670
constexpr GZCLSID kCLSID_cSC4Tank                            = 0xebe0dcadu; // Write @ 0x1006f43fe, Read @ 0x1006f44e2
constexpr GZCLSID kCLSID_cSCResKeyTable                      = 0x67336285u; // Write @ 0x100528314, Read @ 0x10052838e
constexpr GZCLSID kCLSID_cSTESurfaceWater                    = 0x0b8da7dbu; // Write @ 0x10072e86a, Read @ 0x10072ea64
constexpr GZCLSID kCLSID_cSC4OccupantFilter                  = 0xa9c74dc9u; // Write @ 0x1000a5dfe, Read @ 0x1000a5e56
constexpr GZCLSID kCLSID_cSC4DefaultOccupantFilter           = 0xc9c75eaeu; // Write @ 0x1000a6082, Read @ 0x1000a60f6
constexpr GZCLSID kCLSID_cSC4OccupantTypeFilter              = 0xe9c75ed1u; // Write @ 0x1000a61b0, Read @ 0x1000a6218
constexpr GZCLSID kCLSID_cSC4OccupantTypesFilter             = 0xe9c75ee8u; // Write @ 0x1000a631a, Read @ 0x1000a63dc
constexpr GZCLSID kCLSID_cSC4OccupantTypesFilterHashed       = 0xaa25cb90u; // Write @ 0x1000a6518, Read @ 0x1000a65a2
constexpr GZCLSID kCLSID_cSC4RealisticOccupantFilter         = 0x8a7a1c2cu; // Write @ 0x1000a6aec, Read @ 0x1000a6b6c
constexpr GZCLSID kCLSID_cSC4NetworkOccupantFilter           = 0xeac3a3d2u; // Write @ 0x1000a6cb4, Read @ 0x1000a6d2c
constexpr GZCLSID kCLSID_cSC4ViewLevelOccupantFilter         = 0xaac3c01du; // Write @ 0x1000a7038, Read @ 0x1000a70b0
constexpr GZCLSID kCLSID_cSC4StringSpecificationOccupantFilter = 0x0bceb6c2u; // Write @ 0x1000a8c1c, Read @ 0x1000a8c74
constexpr GZCLSID kCLSID_cSC4ComplexViewOccupantFilter       = 0xca0e3852u; // Write @ 0x1003514a6, Read @ 0x1003515c2

// One tAutomataState as streamed by cSC4Automaton::Write: the Uint32 type
// (state +0x08) followed by the state's own (polymorphic) Write.
struct AutomataStateRecord {
    Uint32 type = 0;                 // +0x08 (selects the concrete class)
    Uint16 version = 3;
    SimSerializableRef ref{};        // +0x10 (null when flagged vt+0x70(8))
    Uint32 field18 = 0;
    Float32 field1C = 0.0f;
    Uint8 field20 = 0;               // bool
    Uint32 field24 = 0;
    Uint32 field28 = 0;
    Uint32 field2C = 0;              // version >= 3
    // Subclass payloads (meaning depends on type).
    Sint32 riot38 = 0;               // RiotLeader +0x38
    Sint32 riot3C = 0;               // RiotLeader +0x3c
    Uint8 riot40 = 0;                // RiotLeader +0x40
    std::array<S3DVector3, 4> vecs{}; // RandomWalk[0] / Bumped[0..3]
    std::array<Float32, 5> floats{}; // RandomWalk 2 / Plummet 5 / PlopJump 2 / PlayerDrive 1 / PullOver 1
    SimSerializableRef subRef{};     // AttachAnim +0x38 / Chase +0x38
    Uint16 anim44 = 0;               // AttachAnim +0x44
    Uint8 anim40 = 0;                // AttachAnim +0x40 (bool)

    bool Read(cIGZIStream& stream) noexcept;   // type already consumed
    bool Write(cIGZOStream& stream) const;     // writes type + state
};

struct AutomatonSaveRecord {
    Uint16 version = 8;
    OccupantBaseSaveRecord base;
    S3DVector3 vec74{};
    S3DVector3 vec00{};              // first member of the automaton (+0x00 of r14)
    S3DVector3 vec8C{};
    Float32 fieldB0 = 0.0f;
    Float32 fieldB4 = 0.0f;
    Uint32 fieldB8 = 0;
    Uint32 fieldBC = 0;
    SimSerializableRef refD0{};      // IID 0x6a1857f8
    Float32 fieldD8 = 0.0f;
    Float32 fieldDC = 0.0f;
    S3DVector3 vecE0{};
    SimSerializableRef refF0{};      // IID 0xa9b40f05
    std::vector<AutomataStateRecord> states;           // vector +0xf8
    std::vector<std::array<Uint32, 3>> keys;           // vector<cGZPersistResourceKey> +0x110
    std::array<Uint32, 3> key1C8{};
    Uint32 field1D4 = 0;
    std::vector<SimSerializableRef> messageTargets;    // list +0x278
    std::array<Float32, 8> tweens{};                   // TweenStruct +0x2b8, +0x2c8
    Uint8 fieldC4 = 0;
    Uint32 field2DC = 0;             // version >= 8

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct TrafficAutomatonSaveRecord {
    Uint16 version = 2;
    AutomatonSaveRecord automaton;
    SimSerializableRef refF0{};
    Uint32 field308 = 0;
    Uint8 field30C = 0;
    SimSerializableRef ref310{};
    Uint32 field320 = 0;
    Uint32 field330 = 0;
    std::vector<std::array<Uint8, 3>> route; // deque<cISC4PathFinder::RouteData> +0x370

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct PedestrianSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 5;
    TrafficAutomatonSaveRecord traffic;
    Uint8 field39C = 0; // bool
    S3DVector3 vec3A0{};
    Float32 field3B0 = 0.0f;
    Uint32 field398 = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct VehicleSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    TrafficAutomatonSaveRecord traffic;
    struct Entry {
        Uint32 key = 0;   // element +0x08
        Sint32 a = 0;     // element->vt+0x1d8 out[0]
        Sint32 b = 0;     // element->vt+0x1d8 out[1]
    };
    std::vector<Entry> entries;      // deque +0x3d8 (discarded on Read)
    Uint32 field0 = 0;               // always written 0 (discarded on Read)
    S3DVector3 vec3B4{};
    S3DVector3 uninit{};             // cSC4Automaton::kUninit3DVector (discarded on Read)
    Float32 field3B0 = 0.0f;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// Uint16 version + cSC4Vehicle (cSC4CarjackedVehicle v1, cSC4TrainCar v2).
struct VehicleSubclassSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    Uint16 expectedVersion = 1;
    VehicleSaveRecord vehicle;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct EmergencyVehicleSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    VehicleSaveRecord vehicle;
    S3DVector3 vec424{};
    SimSerializableRef ref430{};
    Uint8 field438 = 0;
    Uint16 field43A = 0;
    Uint32 field43C = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct PatrolCarSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    EmergencyVehicleSaveRecord emergency;
    SimSerializableRef ref450{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct TrailerVehicleSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    VehicleSaveRecord vehicle;
    std::vector<SimSerializableRef> trailers; // vector<cSC4Vehicle*> +0x440
    std::vector<Uint32> list430;               // vector<Uint32> +0x430
    S3DVector3 vec458{};
    SimSerializableRef ref468{};
    Uint32 field470 = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4Train (inlines cSC4TrainCar::Write) and cSC4MassTransitTrain (v1 + Train).
struct TrainSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    Uint16 trainCarVersion = 2;
    VehicleSaveRecord vehicle;
    std::vector<SimSerializableRef> cars; // vector<cSC4TrainCar*> +0x440
    Uint32 field430 = 0;
    S3DVector3 vec450{};
    SimSerializableRef ref460{};
    Uint32 field468 = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct MassTransitTrainSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    TrainSaveRecord train;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// Uint16 version + cSC4Automaton (cSC4Aircraft v1, cSC4Watercraft v2).
struct AutomatonSubclassSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    Uint16 expectedVersion = 1;
    AutomatonSaveRecord automaton;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4AutomataManager body (after its Uint16 version).
struct AutomataManagerSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 6;
    Sint32 field4C = 0;
    Uint8 field74 = 0;  // bool
    Uint8 fieldA85 = 0; // bool
    std::vector<SimSerializableRef> automata;           // list +0x58
    std::vector<std::pair<Uint32, Uint32>> counts;      // unordered_map<Uint32,Uint32> (version > 4)

    bool Read(cIGZIStream& stream) noexcept;            // reads version too
    bool ReadInternal(cIGZIStream& stream, Uint16 ver) noexcept;
    bool Write(cIGZOStream& stream) const { return WriteWithVersion(stream, 6); }
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;
    bool WriteInternal(cIGZOStream& stream, Uint16 ver) const; // body only
};

struct TrafficEffectRecord {
    Sint32 a = 0;
    Sint32 b = 0;
    Uint32 c = 0;
    std::array<Sint32, 4> rect1C{};
    std::array<Sint32, 4> rect2C{};
    SimSerializableRef ref40{};
    Uint8 flag48 = 0;
    Uint32 field4C = 0;
};

struct TrafficAutomataManagerSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 3;
    AutomataManagerSaveRecord manager;
    Uint32 fieldB10 = 0;
    Uint32 current = 0;         // key of the current element of the list at +0xb18 (0 if none)
    Uint16 effectVersion = 1;   // version >= 3
    std::vector<TrafficEffectRecord> effects;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const { return WriteWithVersion(stream, 3); }
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;
};

struct AircraftManagerSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 5;
    AutomataManagerSaveRecord manager;
    std::vector<std::pair<SimSerializableRef, SimSerializableRef>> map;  // map<cISC4Occupant*, cISC4Automaton*>
    struct Entry {
        Float32 x = 0.0f, y = 0.0f, z = 0.0f;
        Uint8 flag = 0;
    };
    std::vector<Entry> list;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const { return WriteWithVersion(stream, 5); }
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;
};

struct WatercraftManagerSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 6;
    AutomataManagerSaveRecord manager;
    std::vector<std::pair<SimSerializableRef, SimSerializableRef>> lotToOccupant; // +0xb40
    std::vector<SimSerializableRef> watercraft;                                   // +0xb68
    struct Route {
        SimSerializableRef a{};
        SimSerializableRef b{};
        Sint32 dir = 0;                 // -1 / 0 / 1
        std::array<Uint16, 4> u16{};    // +0x1c..+0x22
        Uint8 flag = 0;                 // +0x24
        Sint16 s26 = 0;
        Sint16 s28 = 0;
        std::vector<Uint8> bytes30;
        std::vector<Uint8> bytes48;
    };
    struct Terminal {
        SimSerializableRef lot{};       // +0x10
        SimSerializableRef occupant{};  // +0x40
        std::vector<Route> routes;      // vector<FerryRouteInformation> +0x28
    };
    std::vector<Terminal> terminals;    // +0xb18

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const { return WriteWithVersion(stream, 6); }
    bool WriteWithVersion(cIGZOStream& stream, Uint16 ver) const;
};

struct AutomataControllerManagerSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    SimSerializableRef ref28{};
    Float32 field58 = 0.0f;
    Sint16 field5C = 0;
    Float32 field60 = 0.0f;
    Sint16 field64 = 0;
    struct Group {
        SimSerializableRef key{};
        std::vector<SimSerializableRef> controllers;
    };
    std::vector<Group> groups;          // unordered_map<ulong, list<cISC4AutomataController*>> +0x80
    struct Queue {
        SimSerializableRef key{};
        std::vector<SimSerializableRef> automata;
        S3DVector3 vec34{};
        S3DVector3 vec40{};
        Uint8 flag4C = 0;
    };
    std::vector<Queue> queues;          // unordered_map<ulong, tControllerQInfo> +0xa8

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct AutomataControllerSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 5;
    SimSerializableRef ref18{};
    std::string name;
    S3DVector3 vec28{};
    Float32 field98 = 0.0f;
    Float32 field9C = 0.0f;
    Uint32 field68 = 0;
    Uint32 field6C = 0;
    std::vector<SimSerializableRef> automata; // list +0x80

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct AutomataGeneratorSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 4;
    AutomataControllerSaveRecord controller;
    Float32 fieldC0 = 0.0f;
    Sint16 fieldC4 = 0;
    Float32 fieldC8 = 0.0f;
    Sint16 fieldCC = 0;
    Uint32 fieldD8 = 0;
    Uint32 fieldDC = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

struct AutomataAttractorSaveRecord {
    Uint32 memAddr = 0;
    AutomataControllerSaveRecord controller;
    Uint16 version = 1;
    std::vector<std::pair<Float32, Float32>> curve; // cSC4ResponseCurve +0xa8

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// ===========================================================================
// Non-COM / cSC4AutoSaveStream subfiles
// ===========================================================================

// cSTETerrain::SaveMiscTerrainData (0x8a91e7e3, Write @ 0x10073f1c8, Load @ 0x10073f596).
struct TerrainMiscSaveRecord {
    Uint16 version = 2;
    Uint32 width50 = 0;
    Uint32 height54 = 0;
    Sint32 field24 = 0;
    Float32 field90 = 0.0f;
    Float32 field94 = 0.0f;
    Float32 field98 = 0.0f;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSTETerrain::SaveVertexProperties (0x8a91e7e0, Write @ 0x10073f34c, Load @ 0x10073f79a).
struct TerrainVertexPropsSaveRecord {
    Uint16 version = 1;
    std::vector<Uint16> props; // vertex[i].flags (+0x22 of each 0x24-byte vertex)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSTETerrain::SaveAltitudes (0xa9dd6ff4 i1, v2) and SaveEdgeAltitudes (i100..103, v1).
struct TerrainAltitudesSaveRecord {
    Uint16 version = 2;
    Uint32 edgeCount = 0;            // present when version == 1
    std::vector<Float32> altitudes;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4View3DWin::DoSave (0xa9c8647c, Write @ 0x1007871e4, Load @ 0x100787092).
struct View3DWinSaveRecord {
    Uint16 version = 4;
    SimSerializableRef cameraControl{};  // 0xc9c628ec
    SimSerializableRef render3D{};       // 0xe9c622d8
    SimSerializableRef modelMaker{};     // 0xa9c741b3
    SimSerializableRef lighting{};       // 0x8695664e
    std::vector<std::pair<Uint32, Uint32>> map678;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// SC4WriteRegionalCity / cSC4RegionalCity (0xca027edb, Write @ 0x1003ae4f8, Read @ 0x1003aec4a).
struct RegionalCitySaveRecord {
    Uint16 streamVersion = 1;
    Uint16 version = 13;
    std::array<Sint32, 7> field20{};     // +0x20..+0x38
    Float32 field3C = 0.5f;              // version >= 10
    Sint8 field40 = 0;
    Uint8 field44 = 2;                   // version >= 13
    Uint8 fieldF0 = 0;                   // bool
    Uint32 fieldF4 = 0;
    std::array<Sint32, 4> fieldF8{};     // +0xf8..+0x104
    Uint32 field108 = 0;
    Uint8 field260 = 0;
    std::array<std::string, 7> strings;  // +0x138, 0x160, 0x188, 0x1b0, 0x238, 0x1d8, 0x200
    std::array<Float32, 3> field228{};
    Sint32 field264 = 0;
    std::array<std::vector<std::pair<Uint32, Sint32>>, 3> maps; // +0x58, +0x70, +0x88
    std::vector<Float32> floats90;       // 12 floats at +0x90
    std::vector<NeighborConnectionSaveRecord> connections;
    struct ConnectionChange {
        Uint32 a = 0;
        Sint32 b = 0;
        Sint32 c = 0;
        Uint8 d = 0;
    };
    std::vector<ConnectionChange> connectionChanges;
    Float64 field268 = 0.0;
    Float64 field270 = 0.0;
    struct CommodityStats {
        Sint32 key = 0;
        std::array<Float64, 5> v{};      // +0x28, 0x30, 0x40, 0x38, 0x48
    };
    std::vector<CommodityStats> commodities;
    std::vector<NeighborDealSaveRecord> dealsA;
    std::vector<NeighborDealSaveRecord> dealsB;
    struct PointOfInterestGroup {
        Uint32 type = 0;
        std::vector<std::array<Float32, 3>> points;
    };
    std::vector<PointOfInterestGroup> poi; // version >= 12

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// SC4WriteStringRecord / cGZUString (0x2026960b, Write @ 0x1001e94ea, Read @ 0x1001e9453).
struct StringRecordSaveRecord {
    std::vector<Uint8> raw;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// SC4WriteCityRegionViewThumbnailBitmapFile (0x8a2482b9, Write @ 0x1001d3b5d).
struct RegionViewThumbnailSaveRecord {
    std::vector<Uint8> pngBytes;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cGZDBSegmentPackedFile::WriteCompressedSetRecord (0xe86b1eef, Write @ 0x10036af30, Read @ 0x10036aae4).
struct DBPFCompressedSetSaveRecord {
    struct Entry {
        Uint32 type = 0;
        Uint32 group = 0;
        Uint32 instance = 0;
        Uint32 uncompressedSize = 0;
    };
    std::vector<Entry> entries;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4Airplane (0xcbeb4f62, Write @ 0x1006ce480, Read @ 0x1006ce51e).
struct AirplaneSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    Uint16 aircraftVersion = 1;
    AutomatonSaveRecord automaton;
    S3DVector3 vec48C{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4Helicopter (0x8ba98eda, Write @ 0x1006ea8b2, Read @ 0x1006ea92c).
struct HelicopterSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    Uint16 aircraftVersion = 1;
    AutomatonSaveRecord automaton;
    S3DVector3 vec310{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4Ambulance (0x6c1acde1, Write @ 0x1006d5eb6, Read @ 0x1006d5f6a).
struct AmbulanceSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    EmergencyVehicleSaveRecord emergency;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4FireTruck (0xaa222b83, Write @ 0x1005106d4, Read @ 0x10051074c) — identical layout to PatrolCar.
using FireTruckSaveRecord = PatrolCarSaveRecord;

// cSC4GetawayVan (0xea863423, Write @ 0x10050744e, Read @ 0x1005074c4).
struct GetawayVanSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 3;
    VehicleSaveRecord vehicle;
    Uint8 flag = 0;
    Sint32 field = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4Tank (0xebe0dcad, Write @ 0x1006f43fe, Read @ 0x1006f44e2).
struct TankSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    VehicleSaveRecord vehicle;
    Sint8 idxA = -1;
    Sint8 idxB = -1;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4RiotPolice (0x8a6e4fa4, Write @ 0x100503438, Read @ 0x1005034e2).
struct RiotPoliceSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    PedestrianSaveRecord pedestrian;
    SimSerializableRef ref3B8{};
    Float32 field3C0 = 0.0f;
    Uint8 field3C4 = 0;
    Uint8 field3C5 = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4SimFireCrew (0x4a222e4d, Write @ 0x100506580, Read @ 0x100506670).
struct SimFireCrewSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    PedestrianSaveRecord pedestrian;
    SimSerializableRef ref3B8{};
    Float32 field3C0 = 0.0f;
    Uint8 field3C4 = 0;
    Sint32 field410 = 0;
    Uint8 field414 = 0;
    S3DVector3 vec418{};
    Float32 field424 = 0.0f;
    S3DVector3 vec428{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4FirePlaneDispatchSource (0x4c0ae114) & cSC4PoliceHelicopterDispatchSource (0x0c0b06e8).
struct DispatchSourceSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    std::vector<SimSerializableRef> occupants; // ilist<cISC4Occupant*> +0x30
    Uint32 field50 = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4AnimationTickManager (0xa9c73857, Write @ 0x100365236, Read @ 0x1003652d8).
struct AnimationTickManagerSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 1;
    std::array<std::vector<SimSerializableRef>, 4> buckets{};

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4DisasterInstanceRiot (0x4a38206d, Write @ 0x10044e65e, Read @ 0x10044e738).
struct DisasterInstanceRiotSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 2;
    Uint32 field58 = 0;
    Uint8 timerUnits = 0;
    Uint64 timerElapsed = 0;
    Uint8 timerRunning = 0;
    Uint32 field78 = 0;
    struct RiotLeader {
        SimSerializableRef leader{};
        Sint32 a = 0, b = 0, c = 0;
        SimSerializableRef target{};
    };
    std::vector<RiotLeader> leaders;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSC4MySimDispatch (0xcbc14674, Write @ 0x1006ad8aa, Read @ 0x1006adcee) and cLocation (@ 0x1006ac930).
struct MySimDispatchSaveRecord {
    Uint32 memAddr = 0;
    Sint32 version = 1;
    Uint8 field20 = 0;
    std::array<Sint32, 18> fields24{}; // +0x24..+0x68
    struct Location {
        SimSerializableRef ref{};
        Float32 x = 0.0f, y = 0.0f;
        std::array<Sint32, 4> ints{};
        Uint8 b20 = 0, b21 = 0;
        Sint32 s24 = 0, s28 = 0;
        bool Read(cIGZIStream& s) noexcept;
        bool Write(cIGZOStream& s) const;
    };
    std::array<Location, 4> locs{};
    SimSerializableRef ref130{};
    Sint32 field138 = 0;
    std::string name140;
    SimSerializableRef ref168{};
    SimSerializableRef ref170{};
    SimSerializableRef ref178{};
    std::vector<std::array<Uint8, 3>> route;
    struct MapEntry {
        Sint32 key = 0;
        std::array<Uint32, 3> resKey{};
        Sint32 val = 0;
    };
    std::vector<MapEntry> map1B8;
    Sint32 field1D0 = 0;
    std::vector<Location> extraLocs;
    Uint8 field10 = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// cSCResKeyTable (0x67336285, Write @ 0x100528314, Read @ 0x10052838e).
struct ResKeyTableSaveRecord {
    Uint32 memAddr = 0;
    std::vector<std::array<Uint32, 3>> keys;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
};

// Returns false if typeId is not an automata class.
bool RoundTripSC4SubfileAutomata(Uint32 typeId, std::span<const Uint8> subfileBytes,
                                 SC4SubfileRoundTripStats& stats);


// =============================================================================
// Wave 2D method index — automata, traffic, watercraft, aircraft, dispatch.
//
// Each array entry is one workpack entry point. Leaf thunks (`// @ 0x100...`)
// and algorithmic bodies (`### Method @ 0x100...`) are both cited. The comment
// on every entry is `MethodName @ 0x100xxxxxx`.
// =============================================================================

namespace automata_index {

enum class AutomataStateType : Uint32 {
    Plain = 0,          // every type id other than the specialized ids below
    RiotLeader = 8,
    RandomWalk = 11,
    Plummet = 14,
    PlopJump = 15,
    PlayerDrive = 16,
    Bumped = 17,
    AttachAnim = 19,
    Collide = 20,
    PullOver = 21,
    Chase = 0x34,
};

enum class AutomataStackOp : Uint8 { Set, Push, Pop };

struct AutomataTransition {
    AutomataStackOp op;
    Uint32 fromType;
    Uint32 toType;
    bool crossesTickTable;
    bool specializedClassChanges;
};

// Priority byte is AutomatonSaveRecord::fieldC4 (+0xc4). The flag word is the
// Uint32 at +0xb8. Visibility is the separate SetVisible argument.
struct AutomatonPriorityVisibility {
    Uint8 priority;
    Uint32 flags;
    bool visible;
};

struct VehicleMotionView {
    Float32 defaultSpeed;   // +0xb4, GetDefaultSpeed
    Float32 speedSample;    // +0x2c0, loaded by GetCurrentSpeed
    bool headlights;
    bool motionBlur;
    bool emergencyLights;
};

// eW_ChangeState values 0..4 are accepted by SetStateInternal (cmp esi, 4; ja).
struct WatercraftStateView {
    Uint32 changeState;
    bool onLand;
    bool outOfBounds;
};

enum class AirplaneAemsSlot : Uint8 { None, Slot28, Slot30, Slot30Then28 };

// SetAirplaneState (+0x310) side effects. Phase +0x368 is written to 3 when
// the next state is 1. AEMS slots follow the branches on the previous state.
struct AirplaneStateStep {
    bool writePhaseThree;
    AirplaneAemsSlot aems;
};

// SetHelicopterState writes HELI_DATA+0x214 only for states 0, 1, and 4.
struct HelicopterPhysicsUpdate {
    bool rewrite;
    Uint32 mode;
};

struct AircraftStateView {
    Uint32 airplaneState;    // +0x310
    Uint32 airplanePhase;    // +0x368
    Uint32 helicopterState;  // HELI_DATA +0x8
    bool aemsArmed;          // +0x30c
};

// setState stores the old value at +0x54 and the new value at +0x50.
// State 3 is the value that also calls setFlags. Flags live at +0x58.
struct MySimStateView {
    Uint32 current;
    Uint32 previous;
    Uint32 flags;
};

inline constexpr AutomataStateType ClassifyAutomataState(Uint32 type) noexcept {
    switch (type) {
    case 8u: return AutomataStateType::RiotLeader;
    case 11u: return AutomataStateType::RandomWalk;
    case 14u: return AutomataStateType::Plummet;
    case 15u: return AutomataStateType::PlopJump;
    case 16u: return AutomataStateType::PlayerDrive;
    case 17u: return AutomataStateType::Bumped;
    case 19u: return AutomataStateType::AttachAnim;
    case 20u: return AutomataStateType::Collide;
    case 21u: return AutomataStateType::PullOver;
    case 0x34u: return AutomataStateType::Chase;
    default: return AutomataStateType::Plain;
    }
}

// cSC4Automaton::DoStateTick @ 0x100108230 jump-tables type ids 0..0x13.
inline constexpr bool BaseDoStateTickCovers(Uint32 type) noexcept {
    return type <= 0x13u;
}

inline constexpr AutomataTransition MakeAutomataTransition(AutomataStackOp op, Uint32 fromType,
                                                           Uint32 toType) noexcept {
    const bool fromTable = BaseDoStateTickCovers(fromType);
    const bool toTable = BaseDoStateTickCovers(toType);
    const bool classChanges = ClassifyAutomataState(fromType) != ClassifyAutomataState(toType);
    return AutomataTransition{op, fromType, toType, fromTable != toTable, classChanges};
}

// PushState appends one tAutomataState* on the vector at +0xf8.
inline constexpr Uint32 StackDepthAfterPush(Uint32 depth) noexcept {
    return depth + 1u;
}

// PopState removes the back state when the vector is non-empty.
inline constexpr Uint32 StackDepthAfterPop(Uint32 depth) noexcept {
    return depth == 0u ? 0u : depth - 1u;
}

// SetAutomataFlag @ 0x1006d6b30: enabled selects (flags | mask), otherwise
// (flags & ~mask). CheckAutomataFlag @ 0x1006d6b50 is a non-zero overlap test.
inline constexpr Uint32 ApplyAutomataFlagWord(Uint32 flags, Uint32 mask, bool enabled) noexcept {
    const Uint32 setMask = flags | mask;
    const Uint32 clearMask = flags & static_cast<Uint32>(~mask);
    return enabled ? setMask : clearMask;
}

inline constexpr bool CheckAutomataFlagWord(Uint32 flags, Uint32 mask) noexcept {
    return (flags & mask) != 0u;
}

inline constexpr Uint8 PriorityOf(AutomatonPriorityVisibility const& view) noexcept {
    return view.priority;
}

inline constexpr int PriorityRank(AutomatonPriorityVisibility const& view) noexcept {
    return static_cast<int>(view.priority);
}

inline constexpr bool VisibleOf(AutomatonPriorityVisibility const& view) noexcept {
    return view.visible;
}

inline constexpr AutomatonPriorityVisibility WithVisibility(AutomatonPriorityVisibility view,
                                                            bool visible) noexcept {
    view.visible = visible;
    return view;
}

inline constexpr AutomatonPriorityVisibility WithFlag(AutomatonPriorityVisibility view, Uint32 mask,
                                                      bool enabled) noexcept {
    view.flags = ApplyAutomataFlagWord(view.flags, mask, enabled);
    return view;
}

// BoatPriority @ 0x1004f3802 takes its priority path for these group ids.
inline constexpr bool WatercraftGroupTakesPriorityPath(Uint32 automataGroup) noexcept {
    return automataGroup == 0x4400u || automataGroup == 0x4402u;
}

inline constexpr bool WatercraftChangeStateInRange(Uint32 state) noexcept {
    return state <= 4u;
}

inline constexpr bool WatercraftAcceptsChange(WatercraftStateView const& view) noexcept {
    return WatercraftChangeStateInRange(view.changeState);
}

inline constexpr bool WatercraftOnLand(WatercraftStateView const& view) noexcept {
    return view.onLand;
}

inline constexpr bool WatercraftOutOfBounds(WatercraftStateView const& view) noexcept {
    return view.outOfBounds;
}

inline constexpr Float32 VehicleDefaultSpeed(VehicleMotionView const& view) noexcept {
    return view.defaultSpeed;
}

inline constexpr Float32 VehicleSpeedSample(VehicleMotionView const& view) noexcept {
    return view.speedSample;
}

inline constexpr bool VehicleShowsHeadlights(VehicleMotionView const& view) noexcept {
    return view.headlights;
}

inline constexpr bool VehicleShowsMotionBlur(VehicleMotionView const& view) noexcept {
    return view.motionBlur;
}

inline constexpr bool VehicleShowsEmergencyLights(VehicleMotionView const& view) noexcept {
    return view.emergencyLights;
}

inline constexpr AirplaneStateStep StepAirplaneState(Uint32 previous, Uint32 next,
                                                     bool aemsArmed) noexcept {
    AirplaneStateStep step{false, AirplaneAemsSlot::None};
    if (next == 1u) {
        step.writePhaseThree = true;
        if (aemsArmed && previous == 0u) {
            step.aems = AirplaneAemsSlot::Slot28;
        } else if (aemsArmed && previous == 3u) {
            step.aems = AirplaneAemsSlot::Slot30Then28;
        }
    } else if (next == 0u && aemsArmed) {
        step.aems = AirplaneAemsSlot::Slot30;
    }
    return step;
}

inline constexpr bool AirplaneStateAtLeastTwo(Uint32 state) noexcept {
    return state >= 2u;
}

inline constexpr AirplaneStateStep AirplaneStep(AircraftStateView const& view, Uint32 next) noexcept {
    return StepAirplaneState(view.airplaneState, next, view.aemsArmed);
}

inline constexpr HelicopterPhysicsUpdate HelicopterPhysicsForState(Uint32 state) noexcept {
    if (state == 0u) {
        return HelicopterPhysicsUpdate{true, 0u};
    }
    if (state == 1u || state == 4u) {
        return HelicopterPhysicsUpdate{true, 1u};
    }
    return HelicopterPhysicsUpdate{false, 0u};
}

inline constexpr HelicopterPhysicsUpdate HelicopterStep(AircraftStateView const& view) noexcept {
    return HelicopterPhysicsForState(view.helicopterState);
}

inline constexpr bool AirplanePastTakeoff(AircraftStateView const& view) noexcept {
    return AirplaneStateAtLeastTwo(view.airplaneState);
}

inline constexpr MySimStateView CommitMySimState(MySimStateView view, Uint32 next) noexcept {
    view.previous = view.current;
    view.current = next;
    return view;
}

inline constexpr bool MySimStateChanged(MySimStateView const& view) noexcept {
    return view.current != view.previous;
}

inline constexpr bool MySimStateRequestsFlagUpdate(Uint32 next) noexcept {
    return next == 3u;
}

inline constexpr Uint32 MySimFlags(MySimStateView const& view) noexcept {
    return view.flags;
}

template <std::size_t N>
inline constexpr bool SitesContain(Uint64 address, Uint64 const (&sites)[N]) noexcept {
    for (std::size_t i = 0; i < N; ++i) {
        if (sites[i] == address) {
            return true;
        }
    }
    return false;
}

template <std::size_t N>
inline constexpr Uint32 SiteCount(Uint64 const (&)[N]) noexcept {
    return static_cast<Uint32>(N);
}

template <typename... Arrays>
inline constexpr bool AnySite(Uint64 address, Arrays const&... sites) noexcept {
    return (SitesContain(address, sites) || ...);
}

template <typename... Arrays>
inline constexpr Uint32 SiteTotal(Arrays const&... sites) noexcept {
    return (SiteCount(sites) + ...);
}

// cSC4Automaton — 154 entry points (44 leaf, 110 body).
inline constexpr Uint64 k_cSC4Automaton[] = {
    0x1000e24f0u, // [leaf] cSC4Automaton::GetCallNumber() const @ 0x1000e24f0
    0x1000e2500u, // [leaf] cSC4Automaton::GetDispatchInfoID() const @ 0x1000e2500
    0x1000e2540u, // [leaf] cSC4Automaton::SetPriority(unsigned char) @ 0x1000e2540
    0x1000f3c10u, // [leaf] cSC4Automaton::SetDispatchInfoID(unsigned int) @ 0x1000f3c10
    0x1000fd88eu, // [leaf] cSC4Automaton::AutomataOBB::AutomataOBB(cSC4Automaton*, cS3DVector3, float) @ 0x1000fd88e
    0x100105d6eu, // [leaf] cSC4Automaton::~cSC4Automaton() [GZCOM boilerplate] @ 0x100105d6e
    0x100105fd4u, // [leaf] cSC4Automaton::~cSC4Automaton() [GZCOM boilerplate] @ 0x100105fd4
    0x100105fe6u, // [leaf] cSC4Automaton::~cSC4Automaton() [GZCOM boilerplate] @ 0x100105fe6
    0x100106e62u, // [leaf] cSC4Automaton::GetPosition(float*) @ 0x100106e62
    0x1001070e6u, // [leaf] cSC4Automaton::GetOrientation(float*) @ 0x1001070e6
    0x100107108u, // [leaf] cSC4Automaton::GetDestination(float*) @ 0x100107108
    0x100107128u, // [leaf] cSC4Automaton::SetDestination(float const*) @ 0x100107128
    0x100107cd8u, // [leaf] cSC4Automaton::FindDrivingAutomaton() @ 0x100107cd8
    0x10010b956u, // [leaf] cSC4Automaton::UpdateEffects() @ 0x10010b956
    0x10010b9a4u, // [leaf] cSC4Automaton::FindRegEffect(cRZString const&) @ 0x10010b9a4
    0x10010b9b6u, // [leaf] cSC4Automaton::RemoveRegEffect(cRZString const&) @ 0x10010b9b6
    0x10010baccu, // [leaf] cSC4Automaton::GetCollidingAutomaton(float) @ 0x10010bacc
    0x10010bad4u, // [leaf] cSC4Automaton::RecalculateLeaders() @ 0x10010bad4
    0x10010bb88u, // [leaf] cSC4Automaton::GetMaxSpeed() const @ 0x10010bb88
    0x10010beb2u, // [leaf] cSC4Automaton::RemoveCallback(cIGZMessageTarget2*) @ 0x10010beb2
    0x10010d088u, // [leaf] cSC4Automaton::GetState() @ 0x10010d088
    0x10010d0a6u, // [leaf] cSC4Automaton::FindState(unsigned int) @ 0x10010d0a6
    0x10010d0d4u, // [leaf] cSC4Automaton::PrevState(tAutomataState const*) @ 0x10010d0d4
    0x10010d0f8u, // [leaf] cSC4Automaton::GetBehavior() const @ 0x10010d0f8
    0x10010d400u, // [leaf] cSC4Automaton::HandleCollision(cISC4Automaton*, cS3DVector3 const&) @ 0x10010d400
    0x10010ded6u, // [leaf] cSC4Automaton::QueryInterface(unsigned int, void**) [GZCOM boilerplate] @ 0x10010ded6
    0x10010df6eu, // [leaf] cSC4Automaton::AddRef() @ 0x10010df6e
    0x10010dfc2u, // [leaf] cSC4Automaton::Release() [GZCOM boilerplate] @ 0x10010dfc2
    0x10010ec20u, // [leaf] cSC4Automaton::AsOccupant() const @ 0x10010ec20
    0x10010ec30u, // [leaf] cSC4Automaton::SetCallNumber(unsigned short) @ 0x10010ec30
    0x10010ec40u, // [leaf] cSC4Automaton::ReturnToSourceOccupant() @ 0x10010ec40
    0x10011b6f0u, // [leaf] cSC4Automaton::StartNamedEffects(cRZString const&, cISC4VisualEffect::tTransitionType, bool) @ 0x10011b6f0
    0x10011b710u, // [leaf] cSC4Automaton::GetLeader() const @ 0x10011b710
    0x1004fdee0u, // [leaf] cSC4Automaton::GetDispatchState() const @ 0x1004fdee0
    0x1004fdef0u, // [leaf] cSC4Automaton::SetSpeed(float, float) @ 0x1004fdef0
    0x100502370u, // [leaf] cSC4Automaton::AsOccupant() @ 0x100502370
    0x1005023f0u, // [leaf] cSC4Automaton::GetActualSpeed() const @ 0x1005023f0
    0x100506820u, // [leaf] cSC4Automaton::GetModelInstance() @ 0x100506820
    0x1005070c0u, // [leaf] cSC4Automaton::GetRadius() const @ 0x1005070c0
    0x1006d6b20u, // [leaf] cSC4Automaton::SetDispatchState(cISC4DispatchManager::tDispatchState) @ 0x1006d6b20
    0x1006d6b30u, // [leaf] cSC4Automaton::SetAutomataFlag(unsigned int, bool) @ 0x1006d6b30
    0x1006d6b50u, // [leaf] cSC4Automaton::CheckAutomataFlag(unsigned int) const @ 0x1006d6b50
    0x1006d6bc0u, // [leaf] cSC4Automaton::GetPriority() const @ 0x1006d6bc0
    0x1006f1ee0u, // [leaf] cSC4Automaton::GetDefaultSpeed() const @ 0x1006f1ee0
    0x1000e2510u, // [body] cSC4Automaton::ToggleNamedEffects(cRZString const&, cISC4VisualEffect::tTransitionType, bool) @ 0x1000e2510
    0x1000e25b0u, // [body] cSC4Automaton::GetMarkedForRemove() const @ 0x1000e25b0
    0x1000f1728u, // [body] cSC4Automaton::TweenStruct::SetDesired(float, float) @ 0x1000f1728
    0x1000f3c80u, // [body] cSC4Automaton::GetReachedDestination() const @ 0x1000f3c80
    0x1000f3ca0u, // [body] cSC4Automaton::SetSelfControlling(bool) @ 0x1000f3ca0
    0x1000fd61cu, // [body] cSC4Automaton::AutomataOBB::AutomataOBB(cSC4Automaton*, cS3DVector3, float) @ 0x1000fd61c
    0x1000fd898u, // [body] cSC4Automaton::AutomataOBB::TestForCollision(cSC4Automaton::AutomataOBB&) @ 0x1000fd898
    0x100100a94u, // [body] cSC4Automaton::AddModelResourceKey(cGZPersistResourceKey const&) @ 0x100100a94
    0x1001059eeu, // [body] cSC4Automaton::cSC4Automaton(cSC4AutomataManager*, cGZPersistResourceKey) @ 0x1001059ee
    0x100105ff8u, // [body] cSC4Automaton::Init() @ 0x100105ff8
    0x10010604cu, // [body] cSC4Automaton::InitEffectsList() @ 0x10010604c
    0x1001060ceu, // [body] cSC4Automaton::Shutdown() @ 0x1001060ce
    0x100106ae4u, // [body] cSC4Automaton::DoMessage(cIGZMessage2*) @ 0x100106ae4
    0x100106b86u, // [body] cSC4Automaton::SetLeader(cISC4Automaton*) @ 0x100106b86
    0x100106bdau, // [body] cSC4Automaton::SetVisible(bool) @ 0x100106bda
    0x100106c5eu, // [body] cSC4Automaton::StartAllEffects() @ 0x100106c5e
    0x100106e92u, // [body] cSC4Automaton::SetPosition(float const*) @ 0x100106e92
    0x100106f24u, // [body] cSC4Automaton::SetOrientation(float const*) @ 0x100106f24
    0x100107006u, // [body] cSC4Automaton::PositionModelInstance(cIS3DModelInstance*) @ 0x100107006
    0x100107148u, // [body] cSC4Automaton::GetBoundingBox(float*, float*) @ 0x100107148
    0x1001071bcu, // [body] cSC4Automaton::GetModelResourceKey(int) @ 0x1001071bc
    0x100107216u, // [body] cSC4Automaton::PlaceInOccupantManager(bool) @ 0x100107216
    0x100107370u, // [body] cSC4Automaton::PlaceInRenderer(bool) @ 0x100107370
    0x100107572u, // [body] cSC4Automaton::MoveTo(float const*, float const*) @ 0x100107572
    0x100107b2au, // [body] cSC4Automaton::FinishMove(bool, bool) @ 0x100107b2a
    0x100107ce2u, // [body] cSC4Automaton::DoAnimationTick(unsigned int, float) @ 0x100107ce2
    0x100108016u, // [body] cSC4Automaton::PostInit() @ 0x100108016
    0x100108136u, // [body] cSC4Automaton::UpdateAnimation(float) @ 0x100108136
    0x100108230u, // [body] cSC4Automaton::DoStateTick(float) @ 0x100108230
    0x1001083dcu, // [body] cSC4Automaton::DoIdleTick(float) @ 0x1001083dc
    0x10010840cu, // [body] cSC4Automaton::DoRandomWalkTick(float) @ 0x10010840c
    0x1001089f2u, // [body] cSC4Automaton::DoPlopJumpTick(float) @ 0x1001089f2
    0x100108a8eu, // [body] cSC4Automaton::DoPlayerDriveTick(float) @ 0x100108a8e
    0x100108b64u, // [body] cSC4Automaton::InitGhostModel() @ 0x100108b64
    0x100108ee0u, // [body] cSC4Automaton::ShutdownGhostModel() @ 0x100108ee0
    0x100108f8cu, // [body] cSC4Automaton::UpdateGhostModel() @ 0x100108f8c
    0x1001092f8u, // [body] cSC4Automaton::DoBumpedTick(float) @ 0x1001092f8
    0x10010930eu, // [body] cSC4Automaton::DoCollideTick(float) @ 0x10010930e
    0x1001093ceu, // [body] cSC4Automaton::DoMagnetTick(float) @ 0x1001093ce
    0x1001095a4u, // [body] cSC4Automaton::DoScriptedTick(float) @ 0x1001095a4
    0x1001095bau, // [body] cSC4Automaton::DoAttachAnimTick(float) @ 0x1001095ba
    0x1001096e0u, // [body] cSC4Automaton::HandlePlayerDriveCommand(cISC4Automaton::ePlayerDriveCommand, bool) @ 0x1001096e0
    0x10010978eu, // [body] cSC4Automaton::TranslatePlayerDriveCommands(tAutomataStatePlayerDrive*, float) @ 0x10010978e
    0x1001098aeu, // [body] cSC4Automaton::HandleEnterState(tAutomataState*) @ 0x1001098ae
    0x100109d28u, // [body] cSC4Automaton::AddToCollisionSystem(tAutomataStateCollide*) @ 0x100109d28
    0x10010a0c0u, // [body] cSC4Automaton::SetSpeedFromBehaviorTemplate(tBehaviorTemplate const*) @ 0x10010a0c0
    0x10010a172u, // [body] cSC4Automaton::HandleExitState(tAutomataState const*) @ 0x10010a172
    0x10010a25cu, // [body] cSC4Automaton::RemoveModelInstance(cIS3DModelInstanceAnimated*) @ 0x10010a25c
    0x10010a3b2u, // [body] cSC4Automaton::StopKeyEffects() @ 0x10010a3b2
    0x10010a436u, // [body] cSC4Automaton::HandleReachedRouteDestination() @ 0x10010a436
    0x10010a4acu, // [body] cSC4Automaton::HandleDispatched(cS3DVector3 const&, cISC4Occupant*) @ 0x10010a4ac
    0x10010a4c4u, // [body] cSC4Automaton::HandleRemoveController(cISC4AutomataController*) @ 0x10010a4c4
    0x10010a5a2u, // [body] cSC4Automaton::Remove(bool) @ 0x10010a5a2
    0x10010a650u, // [body] cSC4Automaton::SetOpacity(float, float) @ 0x10010a650
    0x10010a6e6u, // [body] cSC4Automaton::UpdateOpacity(float) @ 0x10010a6e6
    0x10010a858u, // [body] cSC4Automaton::UpdateLighting() @ 0x10010a858
    0x10010a9b4u, // [body] cSC4Automaton::InitLighting() @ 0x10010a9b4
    0x10010aa34u, // [body] cSC4Automaton::GetTransform(cS3DTransform&) @ 0x10010aa34
    0x10010aa68u, // [body] cSC4Automaton::SetTransform(cS3DTransform const&) @ 0x10010aa68
    0x10010ab32u, // [body] cSC4Automaton::PlayAnim(cGZPersistResourceKey const&) @ 0x10010ab32
    0x10010ab5eu, // [body] cSC4Automaton::ReplaceModelInstance(cIS3DModelInstanceAnimated*, cGZPersistResourceKey const&) @ 0x10010ab5e
    0x10010ae6au, // [body] cSC4Automaton::PlayAnim(unsigned int) @ 0x10010ae6a
    0x10010aebau, // [body] cSC4Automaton::PlayAnimByName(cRZString const&) @ 0x10010aeba
    0x10010aee2u, // [body] cSC4Automaton::GetAnimOffsetByName(cRZString const&) @ 0x10010aee2
    0x10010af9cu, // [body] cSC4Automaton::AttachAnim(cGZPersistResourceKey const&, cS3DAnimInfo::ePlayMode, bool, float) @ 0x10010af9c
    0x10010b052u, // [body] cSC4Automaton::AddModelInstance(cGZPersistResourceKey const&) @ 0x10010b052
    0x10010b4e6u, // [body] cSC4Automaton::LoadModels() @ 0x10010b4e6
    0x10010b81au, // [body] cSC4Automaton::UnloadModels() @ 0x10010b81a
    0x10010b8b6u, // [body] cSC4Automaton::LoadAuxiliaryModel(cSC4Automaton::AuxiliaryModelSpecifier, cIS3DModelInstanceAnimated**) @ 0x10010b8b6
    0x10010b97eu, // [body] cSC4Automaton::AddRegEffect(cRZString const&, cRZString const&) @ 0x10010b97e
    0x10010b9c8u, // [body] cSC4Automaton::ShouldCollide(cSC4Automaton*) @ 0x10010b9c8
    0x10010ba66u, // [body] cSC4Automaton::GetCollidePosition(cS3DVector3&) @ 0x10010ba66
    0x10010badau, // [body] cSC4Automaton::GetChaseVector(cS3DVector3&) @ 0x10010bada
    0x10010bb54u, // [body] cSC4Automaton::GetSpeedModifier() const @ 0x10010bb54
    0x10010bba2u, // [body] cSC4Automaton::SetZoomAndRotation(int, int) @ 0x10010bba2
    0x10010bbfau, // [body] cSC4Automaton::UpdateAttraction() @ 0x10010bbfa
    0x10010be1cu, // [body] cSC4Automaton::AddCallback(cIGZMessageTarget2*) @ 0x10010be1c
    0x10010bf22u, // [body] cSC4Automaton::SendCallbackMessage(cISC4Automaton::tAutomataEvent, unsigned int) @ 0x10010bf22
    0x10010c00au, // [body] cSC4Automaton::RegisterInGameDataRegistry() @ 0x10010c00a
    0x10010c6d8u, // [body] cSC4Automaton::sFadeOut(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x10010c6d8
    0x10010c76eu, // [body] cSC4Automaton::sRemove(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x10010c76e
    0x10010c806u, // [body] cSC4Automaton::sAttachAnim(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x10010c806
    0x10010c916u, // [body] cSC4Automaton::sPlayAnim(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x10010c916
    0x10010caaeu, // [body] cSC4Automaton::sPlaySfx(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x10010caae
    0x10010cb64u, // [body] cSC4Automaton::sPushState(void*, cIGZCommandParameterSet*, cIGZCommandParameterSet*, cISC4GameDataRegistry*) @ 0x10010cb64
    0x10010cc76u, // [body] cSC4Automaton::UnregisterInGameDataRegistry() @ 0x10010cc76
    0x10010cd3eu, // [body] cSC4Automaton::ClearStateStack() @ 0x10010cd3e
    0x10010cda2u, // [body] cSC4Automaton::SetState(unsigned int, cISC4AutomataController*, float, bool, unsigned int, unsigned int, tBehaviorTemplate const*) @ 0x10010cda2
    0x10010ce42u, // [body] cSC4Automaton::PushState(unsigned int, cISC4AutomataController*, float, bool, unsigned int, unsigned int, tBehaviorTemplate const*) @ 0x10010ce42
    0x10010cfb2u, // [body] cSC4Automaton::PopState() @ 0x10010cfb2
    0x10010d11au, // [body] cSC4Automaton::SetBehavior(tBehaviorTemplate const*, cISC4AutomataController*) @ 0x10010d11a
    0x10010d170u, // [body] cSC4Automaton::PushBehavior(tBehaviorTemplate const*, cISC4AutomataController*) @ 0x10010d170
    0x10010d250u, // [body] cSC4Automaton::StartSFX(unsigned int) @ 0x10010d250
    0x10010d336u, // [body] cSC4Automaton::StopSFX(unsigned int) @ 0x10010d336
    0x10010d408u, // [body] cSC4Automaton::GetDebugFlagsText() const @ 0x10010d408
    0x10010d8bcu, // [body] cSC4Automaton::GetDebugExtraText() const @ 0x10010d8bc
    0x10010dcacu, // [body] cSC4Automaton::SetAutomataGroup(cSC4OccupantGroupTemplate*) @ 0x10010dcac
    0x10010dd56u, // [body] cSC4Automaton::InitKeyEffects() @ 0x10010dd56
    0x10010de86u, // [body] cSC4Automaton::GetAutomataGroup() const @ 0x10010de86
    0x10010ea18u, // [body] cSC4Automaton::EnableActiveSound(bool) @ 0x10010ea18
    0x10010ea78u, // [body] cSC4Automaton::UpdateActiveSound(float) @ 0x10010ea78
    0x10010ec50u, // [body] cSC4Automaton::StopNamedEffects(cRZString const&, cISC4VisualEffect::tTransitionType) @ 0x10010ec50
    0x1002f8090u, // [body] cSC4Automaton::GetDisabled() const @ 0x1002f8090
    0x1002f80e0u, // [body] cSC4Automaton::SetMarkedForRemove(bool) @ 0x1002f80e0
    0x100502388u, // [body] cSC4Automaton::GetCurrentSpeed() const @ 0x100502388
    0x100502450u, // [body] cSC4Automaton::GetSelfControlling() const @ 0x100502450
    0x100506840u, // [body] cSC4Automaton::GetSpecialRouteFlag() const @ 0x100506840
    0x100506860u, // [body] cSC4Automaton::SetSpecialRouteFlag(bool) @ 0x100506860
    0x1006d6b60u, // [body] cSC4Automaton::GetOpacity() const @ 0x1006d6b60
    0x1006f3050u, // [body] cSC4Automaton::RecreateNamedEffects(cRZString const&, cISC4VisualEffect::tTransitionType, bool) @ 0x1006f3050
};

// cSC4AutomataManager — 83 entry points (21 leaf, 62 body).
inline constexpr Uint64 k_cSC4AutomataManager[] = {
    0x1000e2446u, // [leaf] cSC4AutomataManager::Pause() @ 0x1000e2446
    0x1000e2450u, // [leaf] cSC4AutomataManager::Resume() @ 0x1000e2450
    0x1000e245au, // [leaf] cSC4AutomataManager::IsPaused() const @ 0x1000e245a
    0x1000e246eu, // [leaf] cSC4AutomataManager::GetMinZoom() const @ 0x1000e246e
    0x1000e248au, // [leaf] cSC4AutomataManager::GetExemplarDataMap() @ 0x1000e248a
    0x1000f3b80u, // [leaf] cSC4AutomataManager::IsActive() const @ 0x1000f3b80
    0x1000f3b90u, // [leaf] cSC4AutomataManager::GetExemplarData() @ 0x1000f3b90
    0x1000fdb9cu, // [leaf] cSC4AutomataManager::~cSC4AutomataManager() [GZCOM boilerplate] @ 0x1000fdb9c
    0x1000fdc82u, // [leaf] cSC4AutomataManager::~cSC4AutomataManager() @ 0x1000fdc82
    0x1000fdc8cu, // [leaf] cSC4AutomataManager::~cSC4AutomataManager() [GZCOM boilerplate] @ 0x1000fdc8c
    0x100100340u, // [leaf] cSC4AutomataManager::HandleInsertNetworkOccupant(cISC4NetworkOccupant*) @ 0x100100340
    0x100100348u, // [leaf] cSC4AutomataManager::HandleRemoveNetworkOccupant(cISC4NetworkOccupant*) @ 0x100100348
    0x1001005ccu, // [leaf] cSC4AutomataManager::HandleRenderPropertiesChanged() @ 0x1001005cc
    0x100100d86u, // [leaf] cSC4AutomataManager::Dispatch(unsigned int, tDispatchLocation const&, tDispatchLocation const&, unsigned int, unsigned char, cGZPersistResourceKey const&, unsigned char) @ 0x100100d86
    0x100100d8eu, // [leaf] cSC4AutomataManager::Dispatch(cISC4Automaton*, tDispatchLocation const&, tDispatchLocation const&, unsigned int, unsigned char) @ 0x100100d8e
    0x100101e38u, // [leaf] cSC4AutomataManager::IsCameraFollowUpdating() const @ 0x100101e38
    0x100102f6eu, // [leaf] cSC4AutomataManager::HandleTuningParametersChanged() @ 0x100102f6e
    0x100104544u, // [leaf] cSC4AutomataManager::QueryInterface(unsigned int, void**) [GZCOM boilerplate] @ 0x100104544
    0x1001045bcu, // [leaf] cSC4AutomataManager::AddRef() @ 0x1001045bc
    0x1001045f4u, // [leaf] cSC4AutomataManager::Release() [GZCOM boilerplate] @ 0x1001045f4
    0x1004fde70u, // [leaf] cSC4AutomataManager::GetAutomataCount() const @ 0x1004fde70
    0x1000fda34u, // [body] cSC4AutomataManager::cSC4AutomataManager() @ 0x1000fda34
    0x1000fdcbau, // [body] cSC4AutomataManager::StaticInit() @ 0x1000fdcba
    0x1000fe11cu, // [body] cSC4AutomataManager::ReadTuningParameters() @ 0x1000fe11c
    0x1000fed8au, // [body] cSC4AutomataManager::StaticShutdown() @ 0x1000fed8a
    0x1000fee58u, // [body] cSC4AutomataManager::Init() @ 0x1000fee58
    0x1000ff06au, // [body] cSC4AutomataManager::Shutdown() @ 0x1000ff06a
    0x1000ff2aau, // [body] cSC4AutomataManager::DoMessage(cGZMessage&) @ 0x1000ff2aa
    0x1000ff652u, // [body] cSC4AutomataManager::EnterPlayerDriveView(cISC4Automaton*, cIGZMessageTarget2*) @ 0x1000ff652
    0x1000ff75cu, // [body] cSC4AutomataManager::ExitPlayerDriveView() @ 0x1000ff75c
    0x1000ff79eu, // [body] cSC4AutomataManager::IsDrivingMode() const @ 0x1000ff79e
    0x1000ff7d4u, // [body] cSC4AutomataManager::DoMessage(cIGZMessage2*) @ 0x1000ff7d4
    0x1000ffba0u, // [body] cSC4AutomataManager::HandleViewChanged() @ 0x1000ffba0
    0x1000ffbf0u, // [body] cSC4AutomataManager::HandleViewZoomChanged(int, int) @ 0x1000ffbf0
    0x1000ffc06u, // [body] cSC4AutomataManager::HandlePostCityInit() @ 0x1000ffc06
    0x1000ffefeu, // [body] cSC4AutomataManager::HandleOccupantGroupChanged(cISC4Automaton*) @ 0x1000ffefe
    0x10010019cu, // [body] cSC4AutomataManager::UpdateZoomAndRotation() @ 0x10010019c
    0x10010027au, // [body] cSC4AutomataManager::HandleViewFilterChanged(cSC4ViewFilterInformation const*, cSC4ViewFilterInformation const*) @ 0x10010027a
    0x100100350u, // [body] cSC4AutomataManager::HandleNewBuilding(cISC4BuildingOccupant*) @ 0x100100350
    0x10010042eu, // [body] cSC4AutomataManager::HandleInsertOccupant(cISC4Occupant*) @ 0x10010042e
    0x1001004ceu, // [body] cSC4AutomataManager::HandleRemoveOccupant(cISC4Occupant*) @ 0x1001004ce
    0x100100586u, // [body] cSC4AutomataManager::HandleRemoveController(cISC4AutomataController*) @ 0x100100586
    0x1001005d4u, // [body] cSC4AutomataManager::Activate(bool) @ 0x1001005d4
    0x100100698u, // [body] cSC4AutomataManager::CreateAutomaton(unsigned int, cS3DVector3 const&, unsigned int, cGZPersistResourceKey const&, cIGZMessageTarget2*, unsigned char, cS3DVector3 const*) @ 0x100100698
    0x100100728u, // [body] cSC4AutomataManager::CreateAutomatonInternal(unsigned int, unsigned int, cGZPersistResourceKey const&, cISC4AutomataManager::tExemplarData const*, unsigned char) @ 0x100100728
    0x100100b3au, // [body] cSC4AutomataManager::AdjustCountForAutomaton(unsigned char) @ 0x100100b3a
    0x100100ba0u, // [body] cSC4AutomataManager::GetRandomCreationPoint(unsigned int, cS3DVector3 const&, float, float, cS3DVector3&, unsigned int, cS3DVector3*) @ 0x100100ba0
    0x100100d96u, // [body] cSC4AutomataManager::TranslateDispatchLocation(tDispatchLocation const&, SC4Rect<int>&) @ 0x100100d96
    0x100100f6cu, // [body] cSC4AutomataManager::TranslateDispatchLocation(tDispatchLocation const&, cS3DVector3&) @ 0x100100f6c
    0x100101118u, // [body] cSC4AutomataManager::ChaseAutomaton(cISC4Automaton*, cISC4Automaton*) @ 0x100101118
    0x10010127cu, // [body] cSC4AutomataManager::ChaseAutomaton(unsigned int, float, cISC4Automaton*, bool) @ 0x10010127c
    0x100101424u, // [body] cSC4AutomataManager::DoAnimationTick(unsigned int, float) @ 0x100101424
    0x1001015f2u, // [body] cSC4AutomataManager::CalculateSimSpeedModifier() @ 0x1001015f2
    0x1001016ccu, // [body] cSC4AutomataManager::GetClockMultiplier(cISC4AutomataManager::PopulateLayer) @ 0x1001016cc
    0x1001017a0u, // [body] cSC4AutomataManager::DestroyAutomata() @ 0x1001017a0
    0x100101848u, // [body] cSC4AutomataManager::AddAutomaton(cISC4Automaton*) @ 0x100101848
    0x100101bc2u, // [body] cSC4AutomataManager::AddAutomatonInternal(cISC4Automaton*) @ 0x100101bc2
    0x100101c2cu, // [body] cSC4AutomataManager::RemoveAutomaton(cISC4Automaton*, bool) @ 0x100101c2c
    0x100101df6u, // [body] cSC4AutomataManager::RemoveAutomatonInternal(cISC4Automaton*) @ 0x100101df6
    0x100101e44u, // [body] cSC4AutomataManager::SetCameraFollow(cISC4Automaton*) @ 0x100101e44
    0x100101f70u, // [body] cSC4AutomataManager::UpdateCameraFollow(float) @ 0x100101f70
    0x100102580u, // [body] cSC4AutomataManager::SetMaxCount(int) @ 0x100102580
    0x100102626u, // [body] cSC4AutomataManager::IsAutomatonOnscreen(cISC4Automaton*) @ 0x100102626
    0x100102666u, // [body] cSC4AutomataManager::GetCollidingAutomaton(cSC4Automaton*, float) @ 0x100102666
    0x10010279cu, // [body] cSC4AutomataManager::ReadTuningClock(cISCPropertyHolder*, unsigned int, cISC4AutomataManager::PopulateLayer) @ 0x10010279c
    0x100102afau, // [body] cSC4AutomataManager::ReadSpawnRecallParameters(cISCPropertyHolder*, unsigned int, cISC4NetworkManager::eNetworkType) @ 0x100102afa
    0x100102f76u, // [body] cSC4AutomataManager::SeedTrackedBuildingsMap() @ 0x100102f76
    0x100103230u, // [body] cSC4AutomataManager::ScanExemplars() @ 0x100103230
    0x1001032c6u, // [body] cSC4AutomataManager::EnumAutomatonExemplar(cGZPersistResourceKey const&, void*) @ 0x1001032c6
    0x1001037a8u, // [body] cSC4AutomataManager::InitExemplarData(cISC4AutomataManager::tExemplarData&, cISCResExemplar*) @ 0x1001037a8
    0x100103efcu, // [body] cSC4AutomataManager::GetSourceBuildingCount(unsigned int) const @ 0x100103efc
    0x100103f4eu, // [body] cSC4AutomataManager::GetSourceBuildingCountForAutomataGroup(unsigned int) const @ 0x100103f4e
    0x100103fa2u, // [body] cSC4AutomataManager::ChooseRandomExemplar(unsigned int, cISC4AutomataManager::PopulateLayer, int, int) @ 0x100103fa2
    0x10010423eu, // [body] cSC4AutomataManager::ChooseRandomExemplarByFilter(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int) @ 0x10010423e
    0x1001043aau, // [body] cSC4AutomataManager::ChooseExemplarByCrimeType(unsigned int, unsigned int, cISC4AutomataManager::PopulateLayer) @ 0x1001043aa
    0x100104506u, // [body] cSC4AutomataManager::FindExemplarFromModelKey(cGZPersistResourceKey const&) @ 0x100104506
    0x1001046e6u, // [body] cSC4AutomataManager::PostNotificationMessage(unsigned int, cISC4Automaton*, unsigned int, unsigned int) @ 0x1001046e6
    0x100104c7eu, // [body] cSC4AutomataManager::GetAutomataManagerForGroup(unsigned int) @ 0x100104c7e
    0x100104da8u, // [body] cSC4AutomataManager::StaticCreateAutomaton(unsigned int, cS3DVector3 const&, unsigned int, cGZPersistResourceKey const&, cIGZMessageTarget2*, unsigned char) @ 0x100104da8
    0x100104e1cu, // [body] cSC4AutomataManager::CanCreateGroup(unsigned int) const @ 0x100104e1c
    0x100104e62u, // [body] cSC4AutomataManager::TrackAutomataGroupInstances(unsigned int, bool) @ 0x100104e62
    0x10010543au, // [body] cSC4AutomataManager::GetInstancesOfGroup(unsigned int, std::__1::unordered_set<cISC4Automaton*, cISC4AutomataManager::tAutomatonPtrHash, std::__1::equal_to<cISC4Automaton*>, std::__1::allocator<cISC4Automaton*>>&) const @ 0x10010543a
    0x1001059bau, // [body] cSC4AutomataManager::ReleaseAutomatonSet(std::__1::unordered_set<cISC4Automaton*, cISC4AutomataManager::tAutomatonPtrHash, std::__1::equal_to<cISC4Automaton*>, std::__1::allocator<cISC4Automaton*>>&) const @ 0x1001059ba
};

// cSC4Watercraft — 62 entry points (5 leaf, 57 body).
inline constexpr Uint64 k_cSC4Watercraft[] = {
    0x1004ef26au, // [leaf] cSC4Watercraft::cSC4Watercraft() @ 0x1004ef26a
    0x1004f604eu, // [leaf] cSC4Watercraft::QueryInterface(unsigned int, void**) [GZCOM boilerplate] @ 0x1004f604e
    0x1004fdf0au, // [leaf] cSC4Watercraft::~cSC4Watercraft() @ 0x1004fdf0a
    0x1004fdf14u, // [leaf] cSC4Watercraft::~cSC4Watercraft() [GZCOM boilerplate] @ 0x1004fdf14
    0x1004fe50eu, // [leaf] cSC4Watercraft::~cSC4Watercraft() [GZCOM boilerplate] @ 0x1004fe50e
    0x1004f1050u, // [body] cSC4Watercraft::SetStateInternal(cSC4Watercraft::eW_ChangeState) @ 0x1004f1050
    0x1004f1cc8u, // [body] cSC4Watercraft::Ferry_SetStationInformation(FerryStationInformation, FerryStationInformation, bool) @ 0x1004f1cc8
    0x1004f27aau, // [body] cSC4Watercraft::IsNeighborTravelBoat() @ 0x1004f27aa
    0x1004f2818u, // [body] cSC4Watercraft::SetNbrRouteData(std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>, cISC4Occupant*) @ 0x1004f2818
    0x1004f3894u, // [body] cSC4Watercraft::InvalidateLookaheadCell(int) @ 0x1004f3894
    0x1004f4b2au, // [body] cSC4Watercraft::cSC4Watercraft() @ 0x1004f4b2a
    0x1004f4e06u, // [body] cSC4Watercraft::Init() @ 0x1004f4e06
    0x1004f54c6u, // [body] cSC4Watercraft::LoadDriveParameters() @ 0x1004f54c6
    0x1004f5b7au, // [body] cSC4Watercraft::Shutdown() @ 0x1004f5b7a
    0x1004f6116u, // [body] cSC4Watercraft::DoMessage(cIGZMessage2*) @ 0x1004f6116
    0x1004f63b0u, // [body] cSC4Watercraft::MoveTo(float const*, float const*) @ 0x1004f63b0
    0x1004f6414u, // [body] cSC4Watercraft::Crash_SetStateInternal(cSC4Watercraft::eW_ChangeState, bool) @ 0x1004f6414
    0x1004f6484u, // [body] cSC4Watercraft::DoStateTick(float) @ 0x1004f6484
    0x1004f65e0u, // [body] cSC4Watercraft::DoCrashSequenceTick(float) @ 0x1004f65e0
    0x1004f67dau, // [body] cSC4Watercraft::DoDockingTick(float) @ 0x1004f67da
    0x1004f6d9cu, // [body] cSC4Watercraft::DoFollowingTick(float) @ 0x1004f6d9c
    0x1004f72aau, // [body] cSC4Watercraft::DoUndockingTick(float) @ 0x1004f72aa
    0x1004f75c0u, // [body] cSC4Watercraft::HandleExitState(tAutomataState const*) @ 0x1004f75c0
    0x1004f76bau, // [body] cSC4Watercraft::EndTowing() @ 0x1004f76ba
    0x1004f7720u, // [body] cSC4Watercraft::Ferry_SwitchActiveStation() @ 0x1004f7720
    0x1004f77d2u, // [body] cSC4Watercraft::HandleEnterState(tAutomataState*) @ 0x1004f77d2
    0x1004f7a24u, // [body] cSC4Watercraft::EndDocking() @ 0x1004f7a24
    0x1004f7a9cu, // [body] cSC4Watercraft::Ferry_DockRequests() @ 0x1004f7a9c
    0x1004f7c50u, // [body] cSC4Watercraft::Ferry_OtherStationInformation() @ 0x1004f7c50
    0x1004f7e06u, // [body] cSC4Watercraft::OnLand() @ 0x1004f7e06
    0x1004f7ea0u, // [body] cSC4Watercraft::Engine_AIBoatMotion(float) @ 0x1004f7ea0
    0x1004f89f0u, // [body] cSC4Watercraft::Movement_PositionNew(float const&) @ 0x1004f89f0
    0x1004f8d06u, // [body] cSC4Watercraft::Engine_DetectCollision() @ 0x1004f8d06
    0x1004f910eu, // [body] cSC4Watercraft::Engine_MovementPossible() @ 0x1004f910e
    0x1004f9194u, // [body] cSC4Watercraft::DoAnimationTick(unsigned int, float) @ 0x1004f9194
    0x1004f976au, // [body] cSC4Watercraft::StopRegularEffects() @ 0x1004f976a
    0x1004f99aau, // [body] cSC4Watercraft::StartRegularEffects() @ 0x1004f99aa
    0x1004f9bf6u, // [body] cSC4Watercraft::Ferry_ClearDockRequests() @ 0x1004f9bf6
    0x1004f9ce0u, // [body] cSC4Watercraft::DoPlayerDriveTick(float) @ 0x1004f9ce0
    0x1004f9e90u, // [body] cSC4Watercraft::OutOfBounds() @ 0x1004f9e90
    0x1004f9f4au, // [body] cSC4Watercraft::Crash_SequenceRunAground(float) @ 0x1004f9f4a
    0x1004fab22u, // [body] cSC4Watercraft::Crash_SequenceSink() @ 0x1004fab22
    0x1004fb362u, // [body] cSC4Watercraft::TranslatePlayerDriveCommands(tAutomataStatePlayerDrive*, float) @ 0x1004fb362
    0x1004fbbfau, // [body] cSC4Watercraft::AccelerationFunction(float const&) @ 0x1004fbbfa
    0x1004fbd80u, // [body] cSC4Watercraft::AccelerationFunctionInverse(float const&) @ 0x1004fbd80
    0x1004fbefeu, // [body] cSC4Watercraft::BoatSize() @ 0x1004fbefe
    0x1004fbf50u, // [body] cSC4Watercraft::Crash_RegressToPreviousLocation() @ 0x1004fbf50
    0x1004fc036u, // [body] cSC4Watercraft::Crash_InanimateCollisionDetection() @ 0x1004fc036
    0x1004fc1ecu, // [body] cSC4Watercraft::Crash_DetermineCase() @ 0x1004fc1ec
    0x1004fc3e2u, // [body] cSC4Watercraft::Helper_OrientationCloseEnough(cS3DVector3) @ 0x1004fc3e2
    0x1004fc4f0u, // [body] cSC4Watercraft::Engine_AIPickLocation() @ 0x1004fc4f0
    0x1004fca96u, // [body] cSC4Watercraft::Assist_PickInitialDir() @ 0x1004fca96
    0x1004fcc3cu, // [body] cSC4Watercraft::Assist_PickNewDir(int, int, int&) @ 0x1004fcc3c
    0x1004fccb4u, // [body] cSC4Watercraft::Assist_DirIntoCell(int, int&, int&) @ 0x1004fccb4
    0x1004fcd1eu, // [body] cSC4Watercraft::GetDebugExtraText() const @ 0x1004fcd1e
    0x1004fcea0u, // [body] cSC4Watercraft::Movement_PositionTurning(float, cS3DTransform&, cS3DVector3&, cS3DTransform&) @ 0x1004fcea0
    0x1004fd4c4u, // [body] cSC4Watercraft::Movement_PositionStraight(float, cS3DTransform&, cS3DVector3&) @ 0x1004fd4c4
    0x1004fd56cu, // [body] cSC4Watercraft::Movement_Rotation(cS3DTransform&) @ 0x1004fd56c
    0x1004fda16u, // [body] cSC4Watercraft::Movement_Pitch(cS3DTransform&) @ 0x1004fda16
    0x1004fdbf0u, // [body] cSC4Watercraft::Ferry_InitializeCurrentRoute(FerryStationInformation) @ 0x1004fdbf0
    0x1004fdcc8u, // [body] cSC4Watercraft::Assist_DirLookup(int) @ 0x1004fdcc8
    0x1004fdd82u, // [body] cSC4Watercraft::Crash_PitchDetermination(bool, bool) @ 0x1004fdd82
};

// cSC4WatercraftManager — 34 entry points (6 leaf, 28 body).
inline constexpr Uint64 k_cSC4WatercraftManager[] = {
    0x1004eddacu, // [leaf] cSC4WatercraftManager::cSC4WatercraftManager() @ 0x1004eddac
    0x1004ef596u, // [leaf] cSC4WatercraftManager::QueryInterface(unsigned int, void**) [GZCOM boilerplate] @ 0x1004ef596
    0x1004f0cb2u, // [leaf] cSC4WatercraftManager::FerryRouteInformation::~FerryRouteInformation() [GZCOM boilerplate] @ 0x1004f0cb2
    0x1004fde84u, // [leaf] cSC4WatercraftManager::~cSC4WatercraftManager() @ 0x1004fde84
    0x1004fde8eu, // [leaf] cSC4WatercraftManager::~cSC4WatercraftManager() [GZCOM boilerplate] @ 0x1004fde8e
    0x1004fe3d4u, // [leaf] cSC4WatercraftManager::~cSC4WatercraftManager() [GZCOM boilerplate] @ 0x1004fe3d4
    0x1004edca0u, // [body] cSC4WatercraftManager::cSC4WatercraftManager() @ 0x1004edca0
    0x1004eddb6u, // [body] cSC4WatercraftManager::Init() @ 0x1004eddb6
    0x1004ede32u, // [body] cSC4WatercraftManager::Shutdown() @ 0x1004ede32
    0x1004edee6u, // [body] cSC4WatercraftManager::DoMessage(cIGZMessage2*) @ 0x1004edee6
    0x1004edfb4u, // [body] cSC4WatercraftManager::HandleOccupantInserted(cISC4Occupant*) @ 0x1004edfb4
    0x1004eee5eu, // [body] cSC4WatercraftManager::HandleOccupantRemoved(cISC4Occupant*) @ 0x1004eee5e
    0x1004ef14eu, // [body] cSC4WatercraftManager::LookForBoatsAtPosition(cS3DVector3, cSC4Watercraft*) @ 0x1004ef14e
    0x1004ef220u, // [body] cSC4WatercraftManager::AddAutomaton(cISC4Automaton*) @ 0x1004ef220
    0x1004ef274u, // [body] cSC4WatercraftManager::RemoveAutomaton(cISC4Automaton*, bool) @ 0x1004ef274
    0x1004ef4c2u, // [body] cSC4WatercraftManager::HandleTuningParametersChanged() @ 0x1004ef4c2
    0x1004f0e16u, // [body] cSC4WatercraftManager::GetPropPositionAndOrientation(cISC4Occupant*, cS3DVector3&, cS3DVector3&, bool) @ 0x1004f0e16
    0x1004f1144u, // [body] cSC4WatercraftManager::CreateFerry(std::__1::__hash_map_iterator<std::__1::__hash_iterator<std::__1::__hash_node<std::__1::__hash_value_type<cRZAutoRefCount<cISC4Lot>, cSC4WatercraftManager::FerryTerminalInformation>, void*>*>>, std::__1::__hash_map_iterator<std::__1::__hash_iterator<std::__1::__hash_node<std::__1::__hash_value_type<cRZAutoRefCount<cISC4Lot>, cSC4WatercraftManager::FerryTerminalInformation>, void*>*>>, std::__1::__wrap_iter<cSC4WatercraftManager::FerryRouteInformation*>) @ 0x1004f1144
    0x1004f180eu, // [body] cSC4WatercraftManager::CreateFerryStruct(std::__1::__hash_map_iterator<std::__1::__hash_iterator<std::__1::__hash_node<std::__1::__hash_value_type<cRZAutoRefCount<cISC4Lot>, cSC4WatercraftManager::FerryTerminalInformation>, void*>*>>, std::__1::__wrap_iter<cSC4WatercraftManager::FerryRouteInformation*>, FerryStationInformation&, FerryStationInformation&) @ 0x1004f180e
    0x1004f1f2eu, // [body] cSC4WatercraftManager::ClearToLaunchBoat(int, int, int) @ 0x1004f1f2e
    0x1004f2054u, // [body] cSC4WatercraftManager::CreateAutomaton(unsigned int, cS3DVector3 const&, unsigned int, cGZPersistResourceKey const&, cIGZMessageTarget2*, unsigned char, cS3DVector3 const*) @ 0x1004f2054
    0x1004f212eu, // [body] cSC4WatercraftManager::CreateMarinaBoat(std::__1::__hash_map_iterator<std::__1::__hash_iterator<std::__1::__hash_node<std::__1::__hash_value_type<cRZAutoRefCount<cISC4Occupant>, cRZAutoRefCount<cSC4Watercraft>>, void*>*>>, unsigned int) @ 0x1004f212e
    0x1004f29a2u, // [body] cSC4WatercraftManager::DoAnimationTick(unsigned int, float) @ 0x1004f29a2
    0x1004f3802u, // [body] cSC4WatercraftManager::BoatPriority(cSC4Watercraft*) @ 0x1004f3802
    0x1004f3900u, // [body] cSC4WatercraftManager::PopulateVisibleArea(float) @ 0x1004f3900
    0x1004f3d66u, // [body] cSC4WatercraftManager::GetEdgeCells(int&, std::__1::vector<std::__1::pair<int, int>, std::__1::allocator<std::__1::pair<int, int>>>&) @ 0x1004f3d66
    0x1004f3f88u, // [body] cSC4WatercraftManager::GetLotBodyOfWater(cISC4Lot*) @ 0x1004f3f88
    0x1004f4024u, // [body] cSC4WatercraftManager::GetRandomCreationPoint(unsigned int, cS3DVector3 const&, float, float, cS3DVector3&, unsigned int, cS3DVector3*) @ 0x1004f4024
    0x1004f4690u, // [body] cSC4WatercraftManager::RequestPermissionToDock(cISC4Lot*, cSC4Watercraft*) @ 0x1004f4690
    0x1004f472eu, // [body] cSC4WatercraftManager::RequestPermissionToWait(cISC4Lot*, cSC4Watercraft*) @ 0x1004f472e
    0x1004f47ccu, // [body] cSC4WatercraftManager::DoneDocking(cISC4Lot*, cSC4Watercraft*) @ 0x1004f47cc
    0x1004f4864u, // [body] cSC4WatercraftManager::DoneQueueing(cISC4Lot*, cSC4Watercraft*) @ 0x1004f4864
    0x1004f4908u, // [body] cSC4WatercraftManager::CollisionDetection(cSC4Watercraft*, cS3DVector3, float) @ 0x1004f4908
    0x1004ff2e4u, // [body] cSC4WatercraftManager::FerryTerminalInformation::FerryTerminalInformation(cSC4WatercraftManager::FerryTerminalInformation const&) @ 0x1004ff2e4
};

// cSC4TrafficAutomaton — 65 entry points (17 leaf, 48 body).
inline constexpr Uint64 k_cSC4TrafficAutomaton[] = {
    0x1000e25d0u, // [leaf] cSC4TrafficAutomaton::GetPathOffset() const @ 0x1000e25d0
    0x1000e25e0u, // [leaf] cSC4TrafficAutomaton::GetRouteInfo() const @ 0x1000e25e0
    0x1000e25f0u, // [leaf] cSC4TrafficAutomaton::GetLookaheadDeque() const @ 0x1000e25f0
    0x1000f3cc0u, // [leaf] cSC4TrafficAutomaton::GetPathPos() const @ 0x1000f3cc0
    0x100116706u, // [leaf] cSC4TrafficAutomaton::QueryInterface(unsigned int, void**) [GZCOM boilerplate] @ 0x100116706
    0x10011b72au, // [leaf] cSC4TrafficAutomaton::~cSC4TrafficAutomaton() [GZCOM boilerplate] @ 0x10011b72a
    0x10011b730u, // [leaf] cSC4TrafficAutomaton::~cSC4TrafficAutomaton() [GZCOM boilerplate] @ 0x10011b730
    0x10011b73eu, // [leaf] cSC4TrafficAutomaton::RecalculatePathOffset() @ 0x10011b73e
    0x100506880u, // [leaf] cSC4TrafficAutomaton::GetNetworkOccupant() const @ 0x100506880
    0x100507130u, // [leaf] cSC4TrafficAutomaton::SetPathOffset(float) @ 0x100507130
    0x100507160u, // [leaf] cSC4TrafficAutomaton::GetDesiredLookaheadCount() const @ 0x100507160
    0x1005075b0u, // [leaf] cSC4TrafficAutomaton::IsTrain() const @ 0x1005075b0
    0x1005075b8u, // [leaf] cSC4TrafficAutomaton::GetDispatchDestination() const @ 0x1005075b8
    0x100508e60u, // [leaf] cSC4TrafficAutomaton::GetCellLifespan() const @ 0x100508e60
    0x10050f560u, // [leaf] cSC4TrafficAutomaton::SetCellLifespan(unsigned char) @ 0x10050f560
    0x1006f3190u, // [leaf] cSC4TrafficAutomaton::GetPathKey() const @ 0x1006f3190
    0x1006f3b10u, // [leaf] cSC4TrafficAutomaton::~cSC4TrafficAutomaton() [GZCOM boilerplate] @ 0x1006f3b10
    0x100114798u, // [body] cSC4TrafficAutomaton::SetPopulateLayer(cISC4AutomataManager::PopulateLayer) @ 0x100114798
    0x100115b4au, // [body] cSC4TrafficAutomaton::FindLeader() @ 0x100115b4a
    0x100115ce8u, // [body] cSC4TrafficAutomaton::FindLeaderInTile(cISC4NetworkOccupant*, unsigned int, unsigned int, unsigned int) @ 0x100115ce8
    0x100116440u, // [body] cSC4TrafficAutomaton::cSC4TrafficAutomaton(cSC4TrafficAutomataManager*, cGZPersistResourceKey) @ 0x100116440
    0x100116558u, // [body] cSC4TrafficAutomaton::Init() @ 0x100116558
    0x1001165eeu, // [body] cSC4TrafficAutomaton::Shutdown() @ 0x1001165ee
    0x100116632u, // [body] cSC4TrafficAutomaton::ClearLookaheadDeque() @ 0x100116632
    0x1001167f2u, // [body] cSC4TrafficAutomaton::DoAnimationTick(unsigned int, float) @ 0x1001167f2
    0x10011696au, // [body] cSC4TrafficAutomaton::DoPathTick(float) @ 0x10011696a
    0x100116cb6u, // [body] cSC4TrafficAutomaton::UpdateCollision() @ 0x100116cb6
    0x100116f3eu, // [body] cSC4TrafficAutomaton::HandleEnterState(tAutomataState*) @ 0x100116f3e
    0x10011711eu, // [body] cSC4TrafficAutomaton::HandleExitState(tAutomataState const*) @ 0x10011711e
    0x1001172f8u, // [body] cSC4TrafficAutomaton::DoStateTick(float) @ 0x1001172f8
    0x10011733eu, // [body] cSC4TrafficAutomaton::DoStopAtStationTick(float) @ 0x10011733e
    0x1001173ccu, // [body] cSC4TrafficAutomaton::DoStopSignTick(float) @ 0x1001173cc
    0x1001174e2u, // [body] cSC4TrafficAutomaton::CheckIntersectionDensity() @ 0x1001174e2
    0x1001176d8u, // [body] cSC4TrafficAutomaton::DoStraightLineTick(float) @ 0x1001176d8
    0x100117a96u, // [body] cSC4TrafficAutomaton::InterpolateAlongPath(float, cS3DVector3 const&, cS3DVector3&, int*, std::__1::vector<cS3DVector3, std::__1::allocator<cS3DVector3>> const*, float*) @ 0x100117a96
    0x100117cc4u, // [body] cSC4TrafficAutomaton::SetDestination(float const*) @ 0x100117cc4
    0x100117dbeu, // [body] cSC4TrafficAutomaton::SetPath(cISC4NetworkOccupant*, unsigned int, std::__1::vector<cS3DVector3, std::__1::allocator<cS3DVector3>> const*, bool) @ 0x100117dbe
    0x1001182e4u, // [body] cSC4TrafficAutomaton::SnapToPath() @ 0x1001182e4
    0x1001187ccu, // [body] cSC4TrafficAutomaton::HandleNetworkOccupantRemoved() @ 0x1001187cc
    0x1001187dcu, // [body] cSC4TrafficAutomaton::ChooseNewDestination() @ 0x1001187dc
    0x100118b1cu, // [body] cSC4TrafficAutomaton::ChooseLookaheadTiles() @ 0x100118b1c
    0x100118d98u, // [body] cSC4TrafficAutomaton::ChooseBestPath(std::__1::__deque_iterator<cSC4TrafficAutomaton::tLookaheadNode, cSC4TrafficAutomaton::tLookaheadNode*, cSC4TrafficAutomaton::tLookaheadNode&, cSC4TrafficAutomaton::tLookaheadNode**, long, 256l>, cS3DVector3 const&, unsigned int&, float&) @ 0x100118d98
    0x1001190b6u, // [body] cSC4TrafficAutomaton::ChooseLookaheadTileByDensity(cSC4TrafficAutomaton::tLookaheadNode const&, cSC4TrafficAutomaton::tLookaheadNode&) @ 0x1001190b6
    0x100119256u, // [body] cSC4TrafficAutomaton::ChooseExitEdge(cISC4NetworkOccupant*, cISC4TrafficNetworkMap::TileInfo*, unsigned char, unsigned char*) @ 0x100119256
    0x10011932eu, // [body] cSC4TrafficAutomaton::ShouldKeepAlive() const @ 0x10011932e
    0x100119390u, // [body] cSC4TrafficAutomaton::ChooseLookaheadTileByRoute(cSC4TrafficAutomaton::tLookaheadNode const&, cSC4TrafficAutomaton::tLookaheadNode&) @ 0x100119390
    0x100119668u, // [body] cSC4TrafficAutomaton::HandleExitNetwork(cISC4NetworkOccupant*, unsigned int, cISC4Lot*, SC4Rect<int> const&) @ 0x100119668
    0x10011968eu, // [body] cSC4TrafficAutomaton::ChooseRandomPath(cISC4NetworkOccupant*, unsigned int) @ 0x10011968e
    0x10011971cu, // [body] cSC4TrafficAutomaton::ApplyAttractionWeight(cISC4NetworkOccupant*, float (*) [4]) @ 0x10011971c
    0x100119906u, // [body] cSC4TrafficAutomaton::GetEdgeFromVector(cISC4NetworkOccupant*, cS3DVector3 const&, unsigned char&, bool) @ 0x100119906
    0x100119a46u, // [body] cSC4TrafficAutomaton::SetRouteInfo(std::__1::deque<cISC4PathFinder::RouteData, std::__1::allocator<cISC4PathFinder::RouteData>> const&, bool, bool) @ 0x100119a46
    0x100119aacu, // [body] cSC4TrafficAutomaton::BeginRoute(bool) @ 0x100119aac
    0x100119db2u, // [body] cSC4TrafficAutomaton::StartTrip(cISC4NetworkOccupant*, unsigned int, bool) @ 0x100119db2
    0x100119eecu, // [body] cSC4TrafficAutomaton::UpdateOrientation(cS3DVector3 const&, float*) @ 0x100119eec
    0x100119f70u, // [body] cSC4TrafficAutomaton::OffsetPathPoint(cS3DVector3&) @ 0x100119f70
    0x10011a24cu, // [body] cSC4TrafficAutomaton::SetDetectCollisions(bool) @ 0x10011a24c
    0x10011a2e8u, // [body] cSC4TrafficAutomaton::GetSafeDistance() @ 0x10011a2e8
    0x10011a4acu, // [body] cSC4TrafficAutomaton::GetStopPoint(cS3DVector3&) @ 0x10011a4ac
    0x10011a580u, // [body] cSC4TrafficAutomaton::SlowDownForStopLight() @ 0x10011a580
    0x10011a5d0u, // [body] cSC4TrafficAutomaton::ReturnToDefaultSpeed() @ 0x10011a5d0
    0x10011a640u, // [body] cSC4TrafficAutomaton::RemoveSurroundingAutomata(cISC4NetworkOccupant*, int) @ 0x10011a640
    0x10011a85eu, // [body] cSC4TrafficAutomaton::GetDebugExtraText() const @ 0x10011a85e
    0x10011aeb2u, // [body] cSC4TrafficAutomaton::CloseEnoughToDispatchDestination() const @ 0x10011aeb2
    0x10011af24u, // [body] cSC4TrafficAutomaton::FindBestNeighborConnection() @ 0x10011af24
    0x100507140u, // [body] cSC4TrafficAutomaton::GetDetectCollisions() const @ 0x100507140
};

// cSC4TrafficAutomataManager — 46 entry points (11 leaf, 35 body).
inline constexpr Uint64 k_cSC4TrafficAutomataManager[] = {
    0x100111c96u, // [leaf] cSC4TrafficAutomataManager::~cSC4TrafficAutomataManager() [GZCOM boilerplate] @ 0x100111c96
    0x100111d7cu, // [leaf] cSC4TrafficAutomataManager::~cSC4TrafficAutomataManager() @ 0x100111d7c
    0x100111d86u, // [leaf] cSC4TrafficAutomataManager::~cSC4TrafficAutomataManager() [GZCOM boilerplate] @ 0x100111d86
    0x10011206au, // [leaf] cSC4TrafficAutomataManager::HandleNewBuilding(cISC4BuildingOccupant*) @ 0x10011206a
    0x100112812u, // [leaf] cSC4TrafficAutomataManager::IsTrainGroup(unsigned int) const @ 0x100112812
    0x100113b98u, // [leaf] cSC4TrafficAutomataManager::CreateAccident(cISC4Occupant*, int, unsigned int) @ 0x100113b98
    0x100114f92u, // [leaf] cSC4TrafficAutomataManager::GetPathTypeForLayer(cISC4AutomataManager::PopulateLayer) const @ 0x100114f92
    0x100114fbeu, // [leaf] cSC4TrafficAutomataManager::GetAutomataGroupForLayer(cISC4AutomataManager::PopulateLayer) const @ 0x100114fbe
    0x100115462u, // [leaf] cSC4TrafficAutomataManager::GetEdgeDensityCurve(cISC4AutomataManager::PopulateLayer) @ 0x100115462
    0x10011548cu, // [leaf] cSC4TrafficAutomataManager::GetMaxAutosPerSecond(cISC4AutomataManager::PopulateLayer) @ 0x10011548c
    0x10011606eu, // [leaf] cSC4TrafficAutomataManager::ScanExemplars() @ 0x10011606e
    0x100111b96u, // [body] cSC4TrafficAutomataManager::cSC4TrafficAutomataManager() @ 0x100111b96
    0x100111db4u, // [body] cSC4TrafficAutomataManager::Init() @ 0x100111db4
    0x100111e24u, // [body] cSC4TrafficAutomataManager::Shutdown() @ 0x100111e24
    0x100111ee2u, // [body] cSC4TrafficAutomataManager::HandleRemoveNetworkOccupant(cISC4NetworkOccupant*) @ 0x100111ee2
    0x100112074u, // [body] cSC4TrafficAutomataManager::HandleRemoveOccupant(cISC4Occupant*) @ 0x100112074
    0x100112114u, // [body] cSC4TrafficAutomataManager::DispatchResponseAutomata(unsigned int, cISC4Occupant*, unsigned int, bool, unsigned char, unsigned int) @ 0x100112114
    0x10011220cu, // [body] cSC4TrafficAutomataManager::UpdateResponseList(float) @ 0x10011220c
    0x100112566u, // [body] cSC4TrafficAutomataManager::GetRandomCreationPoint(unsigned int, cS3DVector3 const&, float, float, cS3DVector3&, unsigned int, cS3DVector3*) @ 0x100112566
    0x100112842u, // [body] cSC4TrafficAutomataManager::Dispatch(unsigned int, tDispatchLocation const&, tDispatchLocation const&, unsigned int, unsigned char, cGZPersistResourceKey const&, unsigned char) @ 0x100112842
    0x10011298eu, // [body] cSC4TrafficAutomataManager::Dispatch(cISC4Automaton*, tDispatchLocation const&, tDispatchLocation const&, unsigned int, unsigned char) @ 0x10011298e
    0x100112d36u, // [body] cSC4TrafficAutomataManager::IsTrafficAutomaton(cISC4Automaton*) const @ 0x100112d36
    0x100112d88u, // [body] cSC4TrafficAutomataManager::DoAnimationTick(unsigned int, float) @ 0x100112d88
    0x100112f76u, // [body] cSC4TrafficAutomataManager::PopulateVisibleArea(float) @ 0x100112f76
    0x100113438u, // [body] cSC4TrafficAutomataManager::IsLayerVisible(cISC4AutomataManager::PopulateLayer) @ 0x100113438
    0x10011347eu, // [body] cSC4TrafficAutomataManager::PopulateNetworkOccupant(cISC4NetworkOccupant*, cISC4TrafficNetworkMap::TileInfo*, int, int, float) @ 0x10011347e
    0x100113eaeu, // [body] cSC4TrafficAutomataManager::PopulateNetworkOccupant(cISC4NetworkOccupant*, cISC4TrafficNetworkMap::TileInfo*, cISC4AutomataManager::PopulateLayer, int, int, float, double) @ 0x100113eae
    0x100114162u, // [body] cSC4TrafficAutomataManager::IsOffscreenEntryEdge(cISC4TrafficNetworkMap::TileInfo*, int, int, unsigned char) @ 0x100114162
    0x100114214u, // [body] cSC4TrafficAutomataManager::MaybeSpawnAutomaton(cISC4NetworkOccupant*, cISC4TrafficNetworkMap::TileInfo*, cISC4AutomataManager::PopulateLayer, float, unsigned char, unsigned char, double, float) @ 0x100114214
    0x10011471au, // [body] cSC4TrafficAutomataManager::ChooseRandomModel(unsigned int, cISC4NetworkOccupant*, cISC4AutomataManager::PopulateLayer, cGZPersistResourceKey&) @ 0x10011471a
    0x100114840u, // [body] cSC4TrafficAutomataManager::SpawnAutomaton(cISC4NetworkOccupant*, cISC4TrafficNetworkMap::TileInfo*, cISC4AutomataManager::PopulateLayer, unsigned char, unsigned char, float) @ 0x100114840
    0x100114c6eu, // [body] cSC4TrafficAutomataManager::RecallAutomatonFromNetwork(cISC4TrafficNetworkMap::TileInfo*, cISC4PathInfo::tPathType) @ 0x100114c6e
    0x100114de4u, // [body] cSC4TrafficAutomataManager::ChooseNetworkTypeForPathType(cISC4NetworkOccupant*, cISC4PathInfo::tPathType) @ 0x100114de4
    0x100114e88u, // [body] cSC4TrafficAutomataManager::HandlePostCityInit() @ 0x100114e88
    0x100114feau, // [body] cSC4TrafficAutomataManager::SpawnOccupantAutomaton(cISC4NetworkOccupant*, int, unsigned int, unsigned int, bool, cISC4PathInfo::tPathType) @ 0x100114fea
    0x100115196u, // [body] cSC4TrafficAutomataManager::GetEdgeDensity(cISC4NetworkOccupant*, cISC4TrafficNetworkMap::TileInfo*, cISC4AutomataManager::PopulateLayer, float, float (*) [4]) @ 0x100115196
    0x100115310u, // [body] cSC4TrafficAutomataManager::GetOccupantTripEdgeDensity(cISC4NetworkOccupant*, cISC4TrafficNetworkMap::TileInfo*, float, float (*) [4]) @ 0x100115310
    0x1001153c6u, // [body] cSC4TrafficAutomataManager::GetEdgeDensityMultiplier(cISC4AutomataManager::PopulateLayer) @ 0x1001153c6
    0x1001154bau, // [body] cSC4TrafficAutomataManager::PopulateCrimeLayer(float) @ 0x1001154ba
    0x10011574eu, // [body] cSC4TrafficAutomataManager::PopulateNetworkOccupantForCrimeLayer(cISC4NetworkOccupant*, cISC4TrafficNetworkMap::TileInfo*, float, unsigned int, unsigned int) @ 0x10011574e
    0x10011596cu, // [body] cSC4TrafficAutomataManager::GetCommotionMultiplier(cISC4NetworkOccupant*, cISC4AutomataManager::PopulateLayer) @ 0x10011596c
    0x1001159c4u, // [body] cSC4TrafficAutomataManager::RemoveAutomaton(cISC4Automaton*, bool) @ 0x1001159c4
    0x1001159fcu, // [body] cSC4TrafficAutomataManager::IsAutomatonOnscreen(cISC4Automaton*) @ 0x1001159fc
    0x100115ac0u, // [body] cSC4TrafficAutomataManager::RecalculateLeaders(cSC4TrafficAutomaton*) @ 0x100115ac0
    0x100115ea6u, // [body] cSC4TrafficAutomataManager::GetDispatchDestinationPoint(cSC4TrafficAutomaton*, cS3DVector3&) @ 0x100115ea6
    0x100115fa0u, // [body] cSC4TrafficAutomataManager::GetDefaultPathType(cSC4TrafficAutomaton*) const @ 0x100115fa0
};

// cSC4MySimDispatch — 64 entry points (16 leaf, 48 body).
inline constexpr Uint64 k_cSC4MySimDispatch[] = {
    0x1006ac7c4u, // [leaf] cSC4MySimDispatch::cLocation::SetByCommute() @ 0x1006ac7c4
    0x1006acb3cu, // [leaf] cSC4MySimDispatch::cSC4MySimDispatch(cSC4MySim*) @ 0x1006acb3c
    0x1006ace62u, // [leaf] cSC4MySimDispatch::~cSC4MySimDispatch() [GZCOM boilerplate] @ 0x1006ace62
    0x1006ad0e2u, // [leaf] cSC4MySimDispatch::~cSC4MySimDispatch() @ 0x1006ad0e2
    0x1006ad0ecu, // [leaf] cSC4MySimDispatch::~cSC4MySimDispatch() [GZCOM boilerplate] @ 0x1006ad0ec
    0x1006ad7f0u, // [leaf] cSC4MySimDispatch::QueryInterface(unsigned int, void**) [GZCOM boilerplate] @ 0x1006ad7f0
    0x1006ad85eu, // [leaf] cSC4MySimDispatch::AddRef() @ 0x1006ad85e
    0x1006ad884u, // [leaf] cSC4MySimDispatch::Release() @ 0x1006ad884
    0x1006afa4au, // [leaf] cSC4MySimDispatch::CheckFlags(int) @ 0x1006afa4a
    0x1006b0244u, // [leaf] cSC4MySimDispatch::GetState() @ 0x1006b0244
    0x1006b024eu, // [leaf] cSC4MySimDispatch::GetTravelType() @ 0x1006b024e
    0x1006b26cau, // [leaf] cSC4MySimDispatch::GetCurrentLocation() @ 0x1006b26ca
    0x1006b26d4u, // [leaf] cSC4MySimDispatch::GetCurrentDestination() @ 0x1006b26d4
    0x1006b26e2u, // [leaf] cSC4MySimDispatch::GetCurrentDefaultDestination() @ 0x1006b26e2
    0x1006b26f0u, // [leaf] cSC4MySimDispatch::GetCurrentCommuteDestination() @ 0x1006b26f0
    0x1006b2daeu, // [leaf] cSC4MySimDispatch::GetTransitSwitchWaitTimeMunutes() @ 0x1006b2dae
    0x1006ac5c6u, // [body] cSC4MySimDispatch::cLocation::SetCellPosition(int, int) @ 0x1006ac5c6
    0x1006ac5fcu, // [body] cSC4MySimDispatch::cLocation::cLocation() @ 0x1006ac5fc
    0x1006ac638u, // [body] cSC4MySimDispatch::cLocation::SetByOccupant(cISC4Occupant*) @ 0x1006ac638
    0x1006ac7dau, // [body] cSC4MySimDispatch::cLocation::IsValid() const @ 0x1006ac7da
    0x1006ac84eu, // [body] cSC4MySimDispatch::cLocation::AnyTravelConnectionExists(int, int) const @ 0x1006ac84e
    0x1006acaacu, // [body] cSC4MySimDispatch::cLocation::operator==(cSC4MySimDispatch::cLocation const&) const @ 0x1006acaac
    0x1006acb52u, // [body] cSC4MySimDispatch::cSC4MySimDispatch(cSC4MySim*) @ 0x1006acb52
    0x1006ad11au, // [body] cSC4MySimDispatch::Init() @ 0x1006ad11a
    0x1006ad3c8u, // [body] cSC4MySimDispatch::setState(cSC4MySimDispatch::eState) @ 0x1006ad3c8
    0x1006ad41cu, // [body] cSC4MySimDispatch::resetPath() @ 0x1006ad41c
    0x1006ad48au, // [body] cSC4MySimDispatch::Shutdown() @ 0x1006ad48a
    0x1006ad584u, // [body] cSC4MySimDispatch::Reset() @ 0x1006ad584
    0x1006ae2d6u, // [body] cSC4MySimDispatch::DoMessage(cIGZMessage2*) @ 0x1006ae2d6
    0x1006aea60u, // [body] cSC4MySimDispatch::notifyMySim(int, int, int, int) @ 0x1006aea60
    0x1006aeb5au, // [body] cSC4MySimDispatch::updateState() @ 0x1006aeb5a
    0x1006afa56u, // [body] cSC4MySimDispatch::setFlags(int, bool) @ 0x1006afa56
    0x1006afa98u, // [body] cSC4MySimDispatch::setNextPathLeg() @ 0x1006afa98
    0x1006afb8cu, // [body] cSC4MySimDispatch::GoTo(cSC4MySimDispatch::cLocation const&) @ 0x1006afb8c
    0x1006afdd8u, // [body] cSC4MySimDispatch::AddDestinationLocation(cSC4MySimDispatch::cLocation const&, int) @ 0x1006afdd8
    0x1006aff0eu, // [body] cSC4MySimDispatch::SetModelBaseResKey(cSC4MySimDispatch::eModelType, cGZPersistResourceKey const&, int) @ 0x1006aff0e
    0x1006b000au, // [body] cSC4MySimDispatch::SetModelOffset(cSC4MySimDispatch::eModelType, int, bool) @ 0x1006b000a
    0x1006b00deu, // [body] cSC4MySimDispatch::SetDefaultDestination(cSC4MySimDispatch::cLocation const&) @ 0x1006b00de
    0x1006b01d2u, // [body] cSC4MySimDispatch::StopTracking() @ 0x1006b01d2
    0x1006b0266u, // [body] cSC4MySimDispatch::StartTracking(int, cRZString const&) @ 0x1006b0266
    0x1006b03ecu, // [body] cSC4MySimDispatch::getOccupantToTrack() @ 0x1006b03ec
    0x1006b0486u, // [body] cSC4MySimDispatch::updateTrackingOccupant() @ 0x1006b0486
    0x1006b052cu, // [body] cSC4MySimDispatch::isBuilding(cSC4MySimDispatch::cLocation&) const @ 0x1006b052c
    0x1006b059cu, // [body] cSC4MySimDispatch::pickAutomataManager(int) @ 0x1006b059c
    0x1006b05e8u, // [body] cSC4MySimDispatch::setAutomataOccupant(cISC4Occupant*) @ 0x1006b05e8
    0x1006b06ecu, // [body] cSC4MySimDispatch::updateNeighborLocation(int, cSC4MySimDispatch::cLocation&) @ 0x1006b06ec
    0x1006b074cu, // [body] cSC4MySimDispatch::validateCommuteLocation(cSC4MySimDispatch::cLocation&) @ 0x1006b074c
    0x1006b0aacu, // [body] cSC4MySimDispatch::getLocationCellRect(cSC4MySimDispatch::cLocation const&) @ 0x1006b0aac
    0x1006b0b16u, // [body] cSC4MySimDispatch::switchTravel() @ 0x1006b0b16
    0x1006b1d76u, // [body] cSC4MySimDispatch::snapToNearestRoadTile(cSC4MySimDispatch::cLocation&) @ 0x1006b1d76
    0x1006b1fa2u, // [body] cSC4MySimDispatch::basicDispatch(std::__1::deque<cISC4PathFinder::RouteData, std::__1::allocator<cISC4PathFinder::RouteData>>&, cISC4Automaton*, bool) @ 0x1006b1fa2
    0x1006b2442u, // [body] cSC4MySimDispatch::distance_between_locations(cSC4MySimDispatch::cLocation const&, cSC4MySimDispatch::cLocation const&) @ 0x1006b2442
    0x1006b256au, // [body] cSC4MySimDispatch::basicDispatchOnNewPathLeg(cISC4Automaton*, bool) @ 0x1006b256a
    0x1006b25f4u, // [body] cSC4MySimDispatch::jumpToNextSwitch() @ 0x1006b25f4
    0x1006b2668u, // [body] cSC4MySimDispatch::setStuck() @ 0x1006b2668
    0x1006b26feu, // [body] cSC4MySimDispatch::SetCurrentCommuteDestination(cSC4MySimDispatch::cLocation const&, bool) @ 0x1006b26fe
    0x1006b2860u, // [body] cSC4MySimDispatch::TeleportToCommuteLocation() @ 0x1006b2860
    0x1006b29a6u, // [body] cSC4MySimDispatch::getNextSwitchPathSegment(std::__1::deque<cISC4PathFinder::RouteData, std::__1::allocator<cISC4PathFinder::RouteData>>&, bool) @ 0x1006b29a6
    0x1006b2a4au, // [body] cSC4MySimDispatch::snapCurrentPathSegmentToNearestNetworkTile() @ 0x1006b2a4a
    0x1006b2aeau, // [body] cSC4MySimDispatch::getAutoGroupAndModelType(cISC4TrafficSimulator::TravelType, int*, cSC4MySimDispatch::eModelType*) @ 0x1006b2aea
    0x1006b2c08u, // [body] cSC4MySimDispatch::createAutomaton(cISC4TrafficSimulator::TravelType, cISC4Automaton**, int*, cSC4MySimDispatch::eModelType*) @ 0x1006b2c08
    0x1006b2dd6u, // [body] cSC4MySimDispatch::CanUDriveIt() @ 0x1006b2dd6
    0x1006b2e9cu, // [body] cSC4MySimDispatch::EnterUDriveIt() @ 0x1006b2e9c
    0x1006b30a6u, // [body] cSC4MySimDispatch::ExitUDriveIt() @ 0x1006b30a6
};

// cSC4Vehicle — 57 entry points (15 leaf, 42 body).
inline constexpr Uint64 k_cSC4Vehicle[] = {
    0x1000dabf0u, // [leaf] cSC4Vehicle::operator new(unsigned long) @ 0x1000dabf0
    0x1000dac10u, // [leaf] cSC4Vehicle::cSC4Vehicle(cSC4VehicleManager*, cGZPersistResourceKey) @ 0x1000dac10
    0x1000dac1au, // [leaf] cSC4Vehicle::operator delete(void*, unsigned long) @ 0x1000dac1a
    0x1000dd314u, // [leaf] cSC4Vehicle::QueryInterface(unsigned int, void**) [GZCOM boilerplate] @ 0x1000dd314
    0x1000e18a8u, // [leaf] cSC4Vehicle::HandlePlayerDriveCommand(cISC4Automaton::ePlayerDriveCommand, bool) @ 0x1000e18a8
    0x1000e23eeu, // [leaf] cSC4Vehicle::DoBumpedTick(float) @ 0x1000e23ee
    0x1000e23f6u, // [leaf] cSC4Vehicle::DoCollideTick(float) @ 0x1000e23f6
    0x1000e254eu, // [leaf] cSC4Vehicle::~cSC4Vehicle() @ 0x1000e254e
    0x1000e2558u, // [leaf] cSC4Vehicle::~cSC4Vehicle() [GZCOM boilerplate] @ 0x1000e2558
    0x1000e2600u, // [leaf] cSC4Vehicle::SetPrevOrient(cS3DVector3 const&) @ 0x1000e2600
    0x1000e2620u, // [leaf] cSC4Vehicle::GetPathHistory() const @ 0x1000e2620
    0x100508e48u, // [leaf] cSC4Vehicle::GetPathPos() const @ 0x100508e48
    0x10050f570u, // [leaf] cSC4Vehicle::SetPathPos(cS3DVector3 const&) @ 0x10050f570
    0x10050f590u, // [leaf] cSC4Vehicle::GetPrevOrient() const @ 0x10050f590
    0x100510920u, // [leaf] cSC4Vehicle::~cSC4Vehicle() [GZCOM boilerplate] @ 0x100510920
    0x1000dacf2u, // [body] cSC4Vehicle::cSC4Vehicle(cSC4VehicleManager*, cGZPersistResourceKey) @ 0x1000dacf2
    0x1000daeeeu, // [body] cSC4Vehicle::Init() @ 0x1000daeee
    0x1000daf80u, // [body] cSC4Vehicle::Shutdown() @ 0x1000daf80
    0x1000db0bau, // [body] cSC4Vehicle::ClearMotionBlur() @ 0x1000db0ba
    0x1000db152u, // [body] cSC4Vehicle::DoAnimationTick(unsigned int, float) @ 0x1000db152
    0x1000db260u, // [body] cSC4Vehicle::DoStateTick(float) @ 0x1000db260
    0x1000db2c8u, // [body] cSC4Vehicle::DoPathTick(float) @ 0x1000db2c8
    0x1000db946u, // [body] cSC4Vehicle::LineIntersectsSphere(cS3DVector3 const&, cS3DVector3 const&, cS3DVector3 const&, float, cS3DVector3&, cS3DVector3&) const @ 0x1000db946
    0x1000dbad6u, // [body] cSC4Vehicle::FindPathIntersection(cS3DVector3 const&, cS3DVector3 const&, cS3DVector3 const&, float, cS3DVector3&, float) @ 0x1000dbad6
    0x1000dbd64u, // [body] cSC4Vehicle::FindBackPosition(cS3DVector3 const&, cS3DVector3&, float, std::__1::deque<cSC4TrafficAutomaton::tPathNode, std::__1::allocator<cSC4TrafficAutomaton::tPathNode>> const&) @ 0x1000dbd64
    0x1000dc0e4u, // [body] cSC4Vehicle::SavePathHistory(std::__1::deque<cSC4TrafficAutomaton::tPathNode, std::__1::allocator<cSC4TrafficAutomaton::tPathNode>>&, unsigned char) @ 0x1000dc0e4
    0x1000dc3d2u, // [body] cSC4Vehicle::StartTrip(cISC4NetworkOccupant*, unsigned int, bool) @ 0x1000dc3d2
    0x1000dc7a2u, // [body] cSC4Vehicle::SetPath(cISC4NetworkOccupant*, unsigned int, std::__1::vector<cS3DVector3, std::__1::allocator<cS3DVector3>> const*, bool) @ 0x1000dc7a2
    0x1000dccaeu, // [body] cSC4Vehicle::GetSafeDistance() @ 0x1000dccae
    0x1000dceb6u, // [body] cSC4Vehicle::GetDebugExtraText() const @ 0x1000dceb6
    0x1000dd3dcu, // [body] cSC4Vehicle::ShowHeadlights(bool, bool) @ 0x1000dd3dc
    0x1000dd55cu, // [body] cSC4Vehicle::ShowMotionBlur(bool) @ 0x1000dd55c
    0x1000dd5aau, // [body] cSC4Vehicle::UpdateMotionBlur(float) @ 0x1000dd5aa
    0x1000dd7a6u, // [body] cSC4Vehicle::RecalculatePathOffset() @ 0x1000dd7a6
    0x1000dd7e6u, // [body] cSC4Vehicle::GetCollidingAutomaton(float) @ 0x1000dd7e6
    0x1000dd81eu, // [body] cSC4Vehicle::RecalculateLeaders() @ 0x1000dd81e
    0x1000dd852u, // [body] cSC4Vehicle::GetSpeedModifier() const @ 0x1000dd852
    0x1000ddb08u, // [body] cSC4Vehicle::HandleNetworkOccupantRemoved() @ 0x1000ddb08
    0x1000ddc04u, // [body] cSC4Vehicle::DoPlummetTick(float) @ 0x1000ddc04
    0x1000ddf7cu, // [body] cSC4Vehicle::DoPullOverTick(float) @ 0x1000ddf7c
    0x1000de4b8u, // [body] cSC4Vehicle::GetTrafficAutomataManager() @ 0x1000de4b8
    0x1000de4d0u, // [body] cSC4Vehicle::PlayerDriveUpdateSpeed(tAutomataStatePlayerDrive*, tAutomataPlayerDriveParams*, bool, bool, float) @ 0x1000de4d0
    0x1000de6c0u, // [body] cSC4Vehicle::DoPlayerDriveTick(float) @ 0x1000de6c0
    0x1000dfbe4u, // [body] cSC4Vehicle::PlayerDriveGetNetwork(tAutomataStatePlayerDrive*, bool&, cS3DVector3 const&, cS3DVector3*) @ 0x1000dfbe4
    0x1000dffb6u, // [body] cSC4Vehicle::ResolveCollision(cS3DVector3, float, tAutomataStatePlayerDrive*, tAutomataPlayerDriveParams*) @ 0x1000dffb6
    0x1000e0138u, // [body] cSC4Vehicle::ChoosePathFromKeyArray(cISC4PathInfo*, std::__1::vector<unsigned int, std::__1::allocator<unsigned int>>&, cS3DVector3&, unsigned int&, int&) @ 0x1000e0138
    0x1000e0228u, // [body] cSC4Vehicle::ChooseNewPlayerDriveDestination(cS3DVector3&) @ 0x1000e0228
    0x1000e03f4u, // [body] cSC4Vehicle::UpdateAfterDamage(tAutomataStatePlayerDrive*, tAutomataPlayerDriveParams*) @ 0x1000e03f4
    0x1000e061au, // [body] cSC4Vehicle::DoPlayerDriveOffRoadTick(float, tAutomataStatePlayerDrive*, float, tAutomataPlayerDriveParams*, cISC4NetworkOccupant*) @ 0x1000e061a
    0x1000e0de0u, // [body] cSC4Vehicle::GetPlayerDriveLotIntersection(cS3DVector3 const&, cS3DVector3&, cS3DVector2*, cS3DVector2*) @ 0x1000e0de0
    0x1000e1592u, // [body] cSC4Vehicle::Check2DIntersection(float, float, float, float, cS3DVector3 const&, float, cS3DVector3&, float&) @ 0x1000e1592
    0x1000e16ceu, // [body] cSC4Vehicle::RestrictPlayerDrivePosition(cS3DVector3 const&, cS3DVector3&) @ 0x1000e16ce
    0x1000e18b6u, // [body] cSC4Vehicle::TranslatePlayerDriveCommands(tAutomataStatePlayerDrive*, float) @ 0x1000e18b6
    0x1000e1b8cu, // [body] cSC4Vehicle::HandleEnterState(tAutomataState*) @ 0x1000e1b8c
    0x1000e1bfau, // [body] cSC4Vehicle::DoCollisionCheck(cS3DVector3&, float&) @ 0x1000e1bfa
    0x1000e229eu, // [body] cSC4Vehicle::HandleCollision(cISC4Automaton*, cS3DVector3 const&) @ 0x1000e229e
    0x1000e2400u, // [body] cSC4Vehicle::SkipTown() @ 0x1000e2400
};

// cSC4Helicopter — 50 entry points (9 leaf, 41 body).
inline constexpr Uint64 k_cSC4Helicopter[] = {
    0x1006e86ccu, // [leaf] cSC4Helicopter::cSC4Helicopter() @ 0x1006e86cc
    0x1006eacaau, // [leaf] cSC4Helicopter::LoadModels() @ 0x1006eacaa
    0x1006eacb4u, // [leaf] cSC4Helicopter::GetModelResourceInstance() @ 0x1006eacb4
    0x1006f1eb2u, // [leaf] cSC4Helicopter::GetCallNumber() const @ 0x1006f1eb2
    0x1006f1ec0u, // [leaf] cSC4Helicopter::SetCallNumber(unsigned short) @ 0x1006f1ec0
    0x1006f1eceu, // [leaf] cSC4Helicopter::GetDispatchState() const @ 0x1006f1ece
    0x1006f1ef8u, // [leaf] cSC4Helicopter::~cSC4Helicopter() @ 0x1006f1ef8
    0x1006f1f02u, // [leaf] cSC4Helicopter::~cSC4Helicopter() [GZCOM boilerplate] @ 0x1006f1f02
    0x1006f20acu, // [leaf] cSC4Helicopter::~cSC4Helicopter() [GZCOM boilerplate] @ 0x1006f20ac
    0x1006e848au, // [body] cSC4Helicopter::cSC4Helicopter() @ 0x1006e848a
    0x1006e86d6u, // [body] cSC4Helicopter::Init() @ 0x1006e86d6
    0x1006e9054u, // [body] cSC4Helicopter::InitInstance(int) @ 0x1006e9054
    0x1006e9978u, // [body] cSC4Helicopter::PhysicsInit(HELI_DATA*, cS3DVector3 const&) @ 0x1006e9978
    0x1006ea340u, // [body] cSC4Helicopter::Shutdown() @ 0x1006ea340
    0x1006ea72eu, // [body] cSC4Helicopter::PhysicsShutdown() @ 0x1006ea72e
    0x1006ea9c2u, // [body] cSC4Helicopter::PlaceSelf() @ 0x1006ea9c2
    0x1006eab34u, // [body] cSC4Helicopter::GetNewDirection(cS3DVector3 const&, cS3DVector3&) @ 0x1006eab34
    0x1006eacc0u, // [body] cSC4Helicopter::DoAnimationTick(unsigned int, float) @ 0x1006eacc0
    0x1006eadf8u, // [body] cSC4Helicopter::AI_PursueGoal(float) @ 0x1006eadf8
    0x1006ebe98u, // [body] cSC4Helicopter::UpdatePhysics(float) @ 0x1006ebe98
    0x1006ec416u, // [body] cSC4Helicopter::UpdateSpotlight() @ 0x1006ec416
    0x1006ecb6au, // [body] cSC4Helicopter::LoadPlayerDriveParameters(HELI_TYPE_DATA*) @ 0x1006ecb6a
    0x1006ecc6au, // [body] cSC4Helicopter::LoadDriveParameters(HELI_TYPE_DATA*) @ 0x1006ecc6a
    0x1006ece66u, // [body] cSC4Helicopter::SetHelicopterState(unsigned int) @ 0x1006ece66
    0x1006ed0acu, // [body] cSC4Helicopter::NextFrame(HELI_DATA*, float) @ 0x1006ed0ac
    0x1006ed36au, // [body] cSC4Helicopter::CalculateEnginePower(HELI_DATA*, float) @ 0x1006ed36a
    0x1006ed524u, // [body] cSC4Helicopter::YawPitchRollSlideReal(HELI_DATA*, float) @ 0x1006ed524
    0x1006ee136u, // [body] cSC4Helicopter::MoveXZReal(HELI_DATA*, float) @ 0x1006ee136
    0x1006ee796u, // [body] cSC4Helicopter::MoveYReal(HELI_DATA*, float) @ 0x1006ee796
    0x1006ee9eeu, // [body] cSC4Helicopter::WeathervaneTransverseForce(HELI_DATA*, cS3DVector3*) @ 0x1006ee9ee
    0x1006eeb90u, // [body] cSC4Helicopter::Get3dDrag(float, HELI_DATA*, cS3DVector3*, float) @ 0x1006eeb90
    0x1006eed5au, // [body] cSC4Helicopter::LiftCoeff(HELI_DATA*) @ 0x1006eed5a
    0x1006eee70u, // [body] cSC4Helicopter::Apply_Force(cS3DVector3 const&, mv&, float, cS3DVector3 const&, float*) @ 0x1006eee70
    0x1006eef38u, // [body] cSC4Helicopter::AI_ControlReset(float) @ 0x1006eef38
    0x1006ef0b8u, // [body] cSC4Helicopter::DetectCollisions() @ 0x1006ef0b8
    0x1006ef960u, // [body] cSC4Helicopter::TranslatePlayerDriveCommands(tAutomataStatePlayerDrive*, float) @ 0x1006ef960
    0x1006f0174u, // [body] cSC4Helicopter::AI_ControlHover(float, float) @ 0x1006f0174
    0x1006f0222u, // [body] cSC4Helicopter::DoPlayerDriveTick(float) @ 0x1006f0222
    0x1006f0354u, // [body] cSC4Helicopter::AI_StartEngine() @ 0x1006f0354
    0x1006f03a4u, // [body] cSC4Helicopter::AI_AimSpotlight(cS3DVector3 const&, float) @ 0x1006f03a4
    0x1006f0af4u, // [body] cSC4Helicopter::AI_ControlMoveXZ(cS3DVector3 const&, float, float) @ 0x1006f0af4
    0x1006f130cu, // [body] cSC4Helicopter::AI_ControlLiftOverBuildings(float) @ 0x1006f130c
    0x1006f149cu, // [body] cSC4Helicopter::RespondToCrimeScene() @ 0x1006f149c
    0x1006f1582u, // [body] cSC4Helicopter::AI_ControlMaintainPositionXZ(float) @ 0x1006f1582
    0x1006f1756u, // [body] cSC4Helicopter::UpdateEffects() @ 0x1006f1756
    0x1006f19ccu, // [body] cSC4Helicopter::OnPlayerTakeOver() @ 0x1006f19cc
    0x1006f1a44u, // [body] cSC4Helicopter::HandleEnterState(tAutomataState*) @ 0x1006f1a44
    0x1006f1ac6u, // [body] cSC4Helicopter::HandleExitState(tAutomataState const*) @ 0x1006f1ac6
    0x1006f1b44u, // [body] cSC4Helicopter::SetOrientation(float const*) @ 0x1006f1b44
    0x1006f1d70u, // [body] cSC4Helicopter::SetDispatchState(cISC4DispatchManager::tDispatchState) @ 0x1006f1d70
};

// cSC4Airplane — 44 entry points (8 leaf, 36 body).
inline constexpr Uint64 k_cSC4Airplane[] = {
    0x1006ccf26u, // [leaf] cSC4Airplane::cSC4Airplane() @ 0x1006ccf26
    0x1006ce7e4u, // [leaf] cSC4Airplane::DoMessage(cIGZMessage2*) @ 0x1006ce7e4
    0x1006d54f8u, // [leaf] cSC4Airplane::GetCallNumber() const @ 0x1006d54f8
    0x1006d5506u, // [leaf] cSC4Airplane::SetCallNumber(unsigned short) @ 0x1006d5506
    0x1006d5514u, // [leaf] cSC4Airplane::GetDispatchState() const @ 0x1006d5514
    0x1006d552eu, // [leaf] cSC4Airplane::~cSC4Airplane() [GZCOM boilerplate] @ 0x1006d552e
    0x1006d5538u, // [leaf] cSC4Airplane::~cSC4Airplane() [GZCOM boilerplate] @ 0x1006d5538
    0x1006d5620u, // [leaf] cSC4Airplane::~cSC4Airplane() [GZCOM boilerplate] @ 0x1006d5620
    0x1006ccd2cu, // [body] cSC4Airplane::cSC4Airplane() @ 0x1006ccd2c
    0x1006ccf30u, // [body] cSC4Airplane::Init() @ 0x1006ccf30
    0x1006cd5dcu, // [body] cSC4Airplane::LoadPlayerDriveParameters() @ 0x1006cd5dc
    0x1006cd84au, // [body] cSC4Airplane::LoadDriveParameters() @ 0x1006cd84a
    0x1006cdbb2u, // [body] cSC4Airplane::CalculateClimbSlope() @ 0x1006cdbb2
    0x1006cde04u, // [body] cSC4Airplane::CalculateDescentSlope() @ 0x1006cde04
    0x1006ce05eu, // [body] cSC4Airplane::CalculateTurnRate() @ 0x1006ce05e
    0x1006ce36eu, // [body] cSC4Airplane::Shutdown() @ 0x1006ce36e
    0x1006ce5d6u, // [body] cSC4Airplane::SetOrientation(float const*) @ 0x1006ce5d6
    0x1006ce7f4u, // [body] cSC4Airplane::DoAnimationTick(unsigned int, float) @ 0x1006ce7f4
    0x1006ce990u, // [body] cSC4Airplane::AI_PursueGoal(float) @ 0x1006ce990
    0x1006cee14u, // [body] cSC4Airplane::UpdatePhysics(float) @ 0x1006cee14
    0x1006cf77cu, // [body] cSC4Airplane::DoPlayerDriveTick(float) @ 0x1006cf77c
    0x1006cf894u, // [body] cSC4Airplane::TranslatePlayerDriveCommands(tAutomataStatePlayerDrive*, float) @ 0x1006cf894
    0x1006cff10u, // [body] cSC4Airplane::CalculateForces(cS3DVector3 const&, cS3DVector3 const&, cSC4Airplane::Controls const&) const @ 0x1006cff10
    0x1006d0be2u, // [body] cSC4Airplane::SetAirplaneState(cSC4Airplane::State) @ 0x1006d0be2
    0x1006d0cd0u, // [body] cSC4Airplane::DetectCollisions() @ 0x1006d0cd0
    0x1006d14e8u, // [body] cSC4Airplane::UpdateOrientation(cS3DVector3 const&, cS3DVector3&, cSC4Airplane::Controls const&, cS3DTransform&, float) @ 0x1006d14e8
    0x1006d1e62u, // [body] cSC4Airplane::UpdateEffects() @ 0x1006d1e62
    0x1006d2160u, // [body] cSC4Airplane::HandleEnterState(tAutomataState*) @ 0x1006d2160
    0x1006d224au, // [body] cSC4Airplane::HandleExitState(tAutomataState const*) @ 0x1006d224a
    0x1006d22a6u, // [body] cSC4Airplane::HandlePlayerDriveCommand(cISC4Automaton::ePlayerDriveCommand, bool) @ 0x1006d22a6
    0x1006d2454u, // [body] cSC4Airplane::EvaluateClearance(cS3DVector3 const&, cS3DVector3 const&) const @ 0x1006d2454
    0x1006d258cu, // [body] cSC4Airplane::PickRunwayDirection(cS3DVector3 const&, cS3DVector3&, cS3DVector3&, bool) @ 0x1006d258c
    0x1006d2932u, // [body] cSC4Airplane::PickApproachBox(cS3DVector3 const&, cS3DVector3 const&) @ 0x1006d2932
    0x1006d2db0u, // [body] cSC4Airplane::FindFireNearLocation(cS3DVector3 const&, cS3DVector3&) const @ 0x1006d2db0
    0x1006d2f12u, // [body] cSC4Airplane::AI_GotoDestination(cS3DVector3 const&, float) @ 0x1006d2f12
    0x1006d33f8u, // [body] cSC4Airplane::AI_Land(float) @ 0x1006d33f8
    0x1006d3f70u, // [body] cSC4Airplane::AI_Stop(float) @ 0x1006d3f70
    0x1006d40e2u, // [body] cSC4Airplane::AI_TakeOff(float) @ 0x1006d40e2
    0x1006d44ccu, // [body] cSC4Airplane::AI_Taxi(cS3DVector3 const&, float) @ 0x1006d44cc
    0x1006d4852u, // [body] cSC4Airplane::AI_TurnToVector(cS3DVector3 const&, float) @ 0x1006d4852
    0x1006d4a0cu, // [body] cSC4Airplane::AI_ApproachAltitude(float, bool, float) @ 0x1006d4a0c
    0x1006d4df8u, // [body] cSC4Airplane::AI_FollowVectorAndTurn(cS3DVector3 const&, cS3DVector3 const&, cS3DVector3 const&, float) @ 0x1006d4df8
    0x1006d51a2u, // [body] cSC4Airplane::AI_FollowRay(cS3DVector3 const&, cS3DVector3 const&, float) @ 0x1006d51a2
    0x1006d54c6u, // [body] cSC4Airplane::SetDispatchState(cISC4DispatchManager::tDispatchState) @ 0x1006d54c6
};

// cSC4AutomataControllerManager — 39 entry points (6 leaf, 33 body).
inline constexpr Uint64 k_cSC4AutomataControllerManager[] = {
    0x1000f3fc8u, // [leaf] cSC4AutomataControllerManager::~cSC4AutomataControllerManager() [GZCOM boilerplate] @ 0x1000f3fc8
    0x1000f646eu, // [leaf] cSC4AutomataControllerManager::RemoveControllers(cISC4Occupant*) @ 0x1000f646e
    0x1000f8278u, // [leaf] cSC4AutomataControllerManager::QueryInterface(unsigned int, void**) [GZCOM boilerplate] @ 0x1000f8278
    0x1000f82e8u, // [leaf] cSC4AutomataControllerManager::AddRef() @ 0x1000f82e8
    0x1000f8320u, // [leaf] cSC4AutomataControllerManager::Release() [GZCOM boilerplate] @ 0x1000f8320
    0x1000fbc10u, // [leaf] cSC4AutomataControllerManager::GetControllerTime() const @ 0x1000fbc10
    0x1000f3efcu, // [body] cSC4AutomataControllerManager::cSC4AutomataControllerManager() @ 0x1000f3efc
    0x1000f4076u, // [body] cSC4AutomataControllerManager::Init() @ 0x1000f4076
    0x1000f43b8u, // [body] cSC4AutomataControllerManager::Shutdown() @ 0x1000f43b8
    0x1000f454cu, // [body] cSC4AutomataControllerManager::ClearControllerQInfo() @ 0x1000f454c
    0x1000f459au, // [body] cSC4AutomataControllerManager::ClearControllers() @ 0x1000f459a
    0x1000f4642u, // [body] cSC4AutomataControllerManager::HandlePostCityInit() @ 0x1000f4642
    0x1000f53f6u, // [body] cSC4AutomataControllerManager::DoMessage(cGZMessage&) @ 0x1000f53f6
    0x1000f5556u, // [body] cSC4AutomataControllerManager::DoMessage(cIGZMessage2*) @ 0x1000f5556
    0x1000f5a68u, // [body] cSC4AutomataControllerManager::HandleViewChanged() @ 0x1000f5a68
    0x1000f5b12u, // [body] cSC4AutomataControllerManager::DoAnimationTick(unsigned int, float) @ 0x1000f5b12
    0x1000f5e78u, // [body] cSC4AutomataControllerManager::UpdateControllers() @ 0x1000f5e78
    0x1000f5ef2u, // [body] cSC4AutomataControllerManager::SendControllerMessage(unsigned int, cISC4AutomataController*) @ 0x1000f5ef2
    0x1000f5feeu, // [body] cSC4AutomataControllerManager::AddControllers(cISC4Occupant*) @ 0x1000f5fee
    0x1000f6124u, // [body] cSC4AutomataControllerManager::AddControllersFromOccupantGroup(cRZString const&, cISC4Occupant*, float const*) @ 0x1000f6124
    0x1000f6198u, // [body] cSC4AutomataControllerManager::AddControllersFromOccupantGroup(unsigned int, cISC4Occupant*, float const*) @ 0x1000f6198
    0x1000f620cu, // [body] cSC4AutomataControllerManager::RemoveControllersFromOccupantGroup(cRZString const&, cISC4Occupant*) @ 0x1000f620c
    0x1000f6246u, // [body] cSC4AutomataControllerManager::RemoveControllersFromOccupantGroup(cSC4OccupantGroupTemplate const*, cISC4Occupant*) @ 0x1000f6246
    0x1000f63fcu, // [body] cSC4AutomataControllerManager::RemoveControllersFromOccupantGroup(unsigned int, cISC4Occupant*) @ 0x1000f63fc
    0x1000f6480u, // [body] cSC4AutomataControllerManager::MoveControllers(cISC4Occupant*) @ 0x1000f6480
    0x1000f6710u, // [body] cSC4AutomataControllerManager::MoveController(cISC4AutomataController*) @ 0x1000f6710
    0x1000f6740u, // [body] cSC4AutomataControllerManager::AddControllerFromTemplateName(cISC4Occupant*, cRZString const&, float const*) @ 0x1000f6740
    0x1000f6796u, // [body] cSC4AutomataControllerManager::AddControllerFromTemplate(cISC4Occupant*, tControllerTemplate*, float const*) @ 0x1000f6796
    0x1000f6fccu, // [body] cSC4AutomataControllerManager::RemoveController(cISC4Occupant*, cRZString const&) @ 0x1000f6fcc
    0x1000f71c8u, // [body] cSC4AutomataControllerManager::RemoveController(cISC4AutomataController*) @ 0x1000f71c8
    0x1000f727au, // [body] cSC4AutomataControllerManager::GetController(cISC4Occupant*, cRZString const&) @ 0x1000f727a
    0x1000f72bcu, // [body] cSC4AutomataControllerManager::GetController(cISC4Occupant*, tControllerTemplate const*) @ 0x1000f72bc
    0x1000f7332u, // [body] cSC4AutomataControllerManager::GetController(cS3DVector3, cRZString const&, float) @ 0x1000f7332
    0x1000f7550u, // [body] cSC4AutomataControllerManager::GetRelevantControllersByWorldPos(float, float, unsigned int, std::__1::list<cISC4AutomataController*, std::__1::allocator<cISC4AutomataController*>>&) @ 0x1000f7550
    0x1000f762cu, // [body] cSC4AutomataControllerManager::GetCompositeAttractionEffect(cISC4Occupant*, cS3DVector3&, float&, float&) @ 0x1000f762c
    0x1000f7a08u, // [body] cSC4AutomataControllerManager::GetCompositeAttractionCommotion(cISC4NetworkOccupant*) @ 0x1000f7a08
    0x1000f7b36u, // [body] cSC4AutomataControllerManager::ReleaseControllerList(std::__1::list<cISC4AutomataController*, std::__1::allocator<cISC4AutomataController*>>&) @ 0x1000f7b36
    0x1000f7b66u, // [body] cSC4AutomataControllerManager::GetControllerQInfo(cISC4Occupant*, bool) @ 0x1000f7b66
    0x1000f833cu, // [body] cSC4AutomataControllerManager::FinalRelease() @ 0x1000f833c
};

// cSC4AutomataController — 44 entry points (19 leaf, 25 body).
inline constexpr Uint64 k_cSC4AutomataController[] = {
    0x1000f85b0u, // [leaf] cSC4AutomataController::~cSC4AutomataController() [GZCOM boilerplate] @ 0x1000f85b0
    0x1000f8658u, // [leaf] cSC4AutomataController::~cSC4AutomataController() [GZCOM boilerplate] @ 0x1000f8658
    0x1000f865eu, // [leaf] cSC4AutomataController::~cSC4AutomataController() [GZCOM boilerplate] @ 0x1000f865e
    0x1000f8d6eu, // [leaf] cSC4AutomataController::CheckTimeOfDay(tControllerTime const&, tControllerTime const&) @ 0x1000f8d6e
    0x1000f9dd4u, // [leaf] cSC4AutomataController::QueryInterface(unsigned int, void**) [GZCOM boilerplate] @ 0x1000f9dd4
    0x1000fb8deu, // [leaf] cSC4AutomataController::GetOccupant() const @ 0x1000fb8de
    0x1000fb8e8u, // [leaf] cSC4AutomataController::GetTemplateName() const @ 0x1000fb8e8
    0x1000fb902u, // [leaf] cSC4AutomataController::GetTemplate() const @ 0x1000fb902
    0x1000fb90cu, // [leaf] cSC4AutomataController::GetRadius() const @ 0x1000fb90c
    0x1000fb97eu, // [leaf] cSC4AutomataController::CheckFlag(cISC4AutomataController::tControllerFlag) const @ 0x1000fb97e
    0x1000fb98au, // [leaf] cSC4AutomataController::SetFlag(cISC4AutomataController::tControllerFlag, bool) @ 0x1000fb98a
    0x1000fb9a4u, // [leaf] cSC4AutomataController::SetPosition(float const*) @ 0x1000fb9a4
    0x1000fb9bcu, // [leaf] cSC4AutomataController::SetLifetime(float) @ 0x1000fb9bc
    0x1000fb9cau, // [leaf] cSC4AutomataController::IsFull() const @ 0x1000fb9ca
    0x1000fb9f0u, // [leaf] cSC4AutomataController::TractRect() @ 0x1000fb9f0
    0x1000fb9fau, // [leaf] cSC4AutomataController::IsExpired() const @ 0x1000fb9fa
    0x1000fba22u, // [leaf] cSC4AutomataController::ReadyToExpire() const @ 0x1000fba22
    0x1000fbc32u, // [leaf] cSC4AutomataController::AddRef() @ 0x1000fbc32
    0x1000fbc40u, // [leaf] cSC4AutomataController::Release() [GZCOM boilerplate] @ 0x1000fbc40
    0x1000f8664u, // [body] cSC4AutomataController::Init() @ 0x1000f8664
    0x1000f8846u, // [body] cSC4AutomataController::PlaySFX(bool) @ 0x1000f8846
    0x1000f895cu, // [body] cSC4AutomataController::Shutdown() @ 0x1000f895c
    0x1000f8a1eu, // [body] cSC4AutomataController::Activate(bool) @ 0x1000f8a1e
    0x1000f8af8u, // [body] cSC4AutomataController::SetOccupant(cISC4Occupant*) @ 0x1000f8af8
    0x1000f8b46u, // [body] cSC4AutomataController::SetTemplate(tControllerTemplate const*) @ 0x1000f8b46
    0x1000f8b8au, // [body] cSC4AutomataController::DoAnimationTick(float, unsigned int, tControllerTime const&) @ 0x1000f8b8a
    0x1000f8c60u, // [body] cSC4AutomataController::CheckEffectTriggers() @ 0x1000f8c60
    0x1000f8d74u, // [body] cSC4AutomataController::NotifySourceDataChanged() @ 0x1000f8d74
    0x1000f8dceu, // [body] cSC4AutomataController::GetBehaviorTemplate(float, cS3DVector3 const&) const @ 0x1000f8dce
    0x1000f8ec2u, // [body] cSC4AutomataController::HandleStateTick(cISC4Automaton*, unsigned int, float) @ 0x1000f8ec2
    0x1000f9016u, // [body] cSC4AutomataController::HandleEnterState(cISC4Automaton*, unsigned int) @ 0x1000f9016
    0x1000f90fau, // [body] cSC4AutomataController::HandleExitState(cISC4Automaton*, unsigned int) @ 0x1000f90fa
    0x1000f91e2u, // [body] cSC4AutomataController::HandleRemoveAutomaton(cISC4Automaton*) @ 0x1000f91e2
    0x1000f9288u, // [body] cSC4AutomataController::HandleViewChanged() @ 0x1000f9288
    0x1000f946cu, // [body] cSC4AutomataController::GetPosition(float*) const @ 0x1000f946c
    0x1000f94c0u, // [body] cSC4AutomataController::GetLotFacingPoint(float*) const @ 0x1000f94c0
    0x1000f9670u, // [body] cSC4AutomataController::IsPointWithinRadius(float const*, float*) @ 0x1000f9670
    0x1000f97e6u, // [body] cSC4AutomataController::GetBoundingBoxIntersection(float const*, float const*, float*) const @ 0x1000f97e6
    0x1000f98eau, // [body] cSC4AutomataController::GetOccupantBoundingBox(float*, float*) const @ 0x1000f98ea
    0x1000f99feu, // [body] cSC4AutomataController::GetBoundingBox(float*, float*) const @ 0x1000f99fe
    0x1000f9e16u, // [body] cSC4AutomataController::CreateTemplateInstance(cRZString const&, unsigned int) @ 0x1000f9e16
    0x1000fb926u, // [body] cSC4AutomataController::IsActive() const @ 0x1000fb926
    0x1000fba2au, // [body] cSC4AutomataController::IsTickable() const @ 0x1000fba2a
    0x1000fbc6cu, // [body] cSC4AutomataController::FinalRelease() @ 0x1000fbc6c
};

// cSC4AutomataGenerator — 26 entry points (12 leaf, 14 body).
inline constexpr Uint64 k_cSC4AutomataGenerator[] = {
    0x1000fa4e6u, // [leaf] cSC4AutomataGenerator::cSC4AutomataGenerator() @ 0x1000fa4e6
    0x1000fa69cu, // [leaf] cSC4AutomataGenerator::ReadyToExpire() const @ 0x1000fa69c
    0x1000fb7d2u, // [leaf] cSC4AutomataGenerator::QueryInterface(unsigned int, void**) @ 0x1000fb7d2
    0x1000fbab2u, // [leaf] cSC4AutomataGenerator::AddRef() @ 0x1000fbab2
    0x1000fbac0u, // [leaf] cSC4AutomataGenerator::Release() [GZCOM boilerplate] @ 0x1000fbac0
    0x1000fbae0u, // [leaf] cSC4AutomataGenerator::~cSC4AutomataGenerator() @ 0x1000fbae0
    0x1000fbaeau, // [leaf] cSC4AutomataGenerator::~cSC4AutomataGenerator() [GZCOM boilerplate] @ 0x1000fbaea
    0x1000fbb18u, // [leaf] cSC4AutomataGenerator::GetControllerType() const @ 0x1000fbb18
    0x1000fbb24u, // [leaf] cSC4AutomataGenerator::GetRadius() const @ 0x1000fbb24
    0x1000fbbc6u, // [leaf] cSC4AutomataGenerator::SetLifetimeAutomataCount(unsigned int) @ 0x1000fbbc6
    0x1000fc22au, // [leaf] cSC4AutomataGenerator::tTemplate::~tTemplate() [GZCOM boilerplate] @ 0x1000fc22a
    0x1000fc270u, // [leaf] cSC4AutomataGenerator::tTemplate::~tTemplate() [GZCOM boilerplate] @ 0x1000fc270
    0x1000fa3e0u, // [body] cSC4AutomataGenerator::cSC4AutomataGenerator() @ 0x1000fa3e0
    0x1000fa4f0u, // [body] cSC4AutomataGenerator::Init() @ 0x1000fa4f0
    0x1000fa504u, // [body] cSC4AutomataGenerator::Shutdown() @ 0x1000fa504
    0x1000fa518u, // [body] cSC4AutomataGenerator::Activate(bool) @ 0x1000fa518
    0x1000fa5beu, // [body] cSC4AutomataGenerator::SetAutomataGroups(std::__1::vector<unsigned int, std::__1::allocator<unsigned int>> const&) @ 0x1000fa5be
    0x1000fa5f2u, // [body] cSC4AutomataGenerator::SetTemplate(tControllerTemplate const*) @ 0x1000fa5f2
    0x1000fa66eu, // [body] cSC4AutomataGenerator::CheckTimeOfDay(tControllerTime const&, tControllerTime const&) @ 0x1000fa66e
    0x1000fa6a4u, // [body] cSC4AutomataGenerator::DoAnimationTick(float, unsigned int, tControllerTime const&) @ 0x1000fa6a4
    0x1000fa896u, // [body] cSC4AutomataGenerator::TriggerGenerator(tControllerTime const&, float) @ 0x1000fa896
    0x1000fb1e6u, // [body] cSC4AutomataGenerator::GetNetworksInRadius(std::__1::vector<SC4Point<int>, std::__1::allocator<SC4Point<int>>>&, unsigned int) @ 0x1000fb1e6
    0x1000fb5e0u, // [body] cSC4AutomataGenerator::ChooseAutomataGroup() @ 0x1000fb5e0
    0x1000fbb40u, // [body] cSC4AutomataGenerator::IsExpired() const @ 0x1000fbb40
    0x1000fbb7cu, // [body] cSC4AutomataGenerator::IsTickable() const @ 0x1000fbb7c
    0x1000fbb9cu, // [body] cSC4AutomataGenerator::FinalRelease() @ 0x1000fbb9c
};

// cSC4AutomataCOMDirector — 34 entry points (3 leaf, 31 body).
inline constexpr Uint64 k_cSC4AutomataCOMDirector[] = {
    0x1004db628u, // [leaf] cSC4AutomataCOMDirector::~cSC4AutomataCOMDirector() @ 0x1004db628
    0x1004dc0b4u, // [leaf] cSC4AutomataCOMDirector::GetDirectorID() const @ 0x1004dc0b4
    0x1004dc0c0u, // [leaf] cSC4AutomataCOMDirector::~cSC4AutomataCOMDirector() [GZCOM boilerplate] @ 0x1004dc0c0
    0x1004db632u, // [body] cSC4AutomataCOMDirector::cSC4AutomataCOMDirector() @ 0x1004db632
    0x1004db8deu, // [body] cSC4AutomataCOMDirector::CreateAutomataScriptSystem() @ 0x1004db8de
    0x1004db916u, // [body] cSC4AutomataCOMDirector::CreateVehicleManager() @ 0x1004db916
    0x1004db952u, // [body] cSC4AutomataCOMDirector::CreatePedestrianManager() @ 0x1004db952
    0x1004db98eu, // [body] cSC4AutomataCOMDirector::CreateAircraftManager() @ 0x1004db98e
    0x1004db9cau, // [body] cSC4AutomataCOMDirector::CreateWatercraftManager() @ 0x1004db9ca
    0x1004dba06u, // [body] cSC4AutomataCOMDirector::CreateSC4Vehicle() @ 0x1004dba06
    0x1004dba66u, // [body] cSC4AutomataCOMDirector::CreateSC4FireTruck() @ 0x1004dba66
    0x1004dba9eu, // [body] cSC4AutomataCOMDirector::CreateSC4PatrolCar() @ 0x1004dba9e
    0x1004dbad6u, // [body] cSC4AutomataCOMDirector::CreateSC4TrainCar() @ 0x1004dbad6
    0x1004dbb36u, // [body] cSC4AutomataCOMDirector::CreateSC4Train() @ 0x1004dbb36
    0x1004dbb8au, // [body] cSC4AutomataCOMDirector::CreateSC4TrailerVehicle() @ 0x1004dbb8a
    0x1004dbbdeu, // [body] cSC4AutomataCOMDirector::CreateSC4GetawayVan() @ 0x1004dbbde
    0x1004dbc22u, // [body] cSC4AutomataCOMDirector::CreateSC4Bus() @ 0x1004dbc22
    0x1004dbc66u, // [body] cSC4AutomataCOMDirector::CreateSC4CarjackedVehicle() @ 0x1004dbc66
    0x1004dbcaau, // [body] cSC4AutomataCOMDirector::CreateSC4Tank() @ 0x1004dbcaa
    0x1004dbceeu, // [body] cSC4AutomataCOMDirector::CreateSC4MayorLimo() @ 0x1004dbcee
    0x1004dbd32u, // [body] cSC4AutomataCOMDirector::CreateSC4Ambulance() @ 0x1004dbd32
    0x1004dbd6au, // [body] cSC4AutomataCOMDirector::CreateSC4MassTransitTrain() @ 0x1004dbd6a
    0x1004dbda2u, // [body] cSC4AutomataCOMDirector::CreateSC4Pedestrian() @ 0x1004dbda2
    0x1004dbe02u, // [body] cSC4AutomataCOMDirector::CreateSC4PedestrianOneShot() @ 0x1004dbe02
    0x1004dbe62u, // [body] cSC4AutomataCOMDirector::CreateSC4SimFireCrew() @ 0x1004dbe62
    0x1004dbea8u, // [body] cSC4AutomataCOMDirector::CreateSC4RiotPolice() @ 0x1004dbea8
    0x1004dbeeeu, // [body] cSC4AutomataCOMDirector::CreateSC4Aircraft() @ 0x1004dbeee
    0x1004dbf26u, // [body] cSC4AutomataCOMDirector::CreateSC4Helicopter() @ 0x1004dbf26
    0x1004dbf5eu, // [body] cSC4AutomataCOMDirector::CreateSC4Airplane() @ 0x1004dbf5e
    0x1004dbf96u, // [body] cSC4AutomataCOMDirector::CreateSC4Watercraft() @ 0x1004dbf96
    0x1004dbfceu, // [body] cSC4AutomataCOMDirector::CreateAutomataControllerManager() @ 0x1004dbfce
    0x1004dc006u, // [body] cSC4AutomataCOMDirector::CreateAutomataAttractor() @ 0x1004dc006
    0x1004dc03eu, // [body] cSC4AutomataCOMDirector::CreateAutomataGenerator() @ 0x1004dc03e
    0x1004dc076u, // [body] cSC4AutomataCOMDirector::CreateSC4CitySituationManager() @ 0x1004dc076
};

// cSC4Pedestrian — 34 entry points (9 leaf, 25 body).
inline constexpr Uint64 k_cSC4Pedestrian[] = {
    0x1000f13eau, // [leaf] cSC4Pedestrian::operator new(unsigned long) @ 0x1000f13ea
    0x1000f140au, // [leaf] cSC4Pedestrian::cSC4Pedestrian(cSC4PedestrianManager*, cGZPersistResourceKey) @ 0x1000f140a
    0x1000f1414u, // [leaf] cSC4Pedestrian::operator delete(void*, unsigned long) @ 0x1000f1414
    0x1000f17f2u, // [leaf] cSC4Pedestrian::Shutdown() @ 0x1000f17f2
    0x1000f3562u, // [leaf] cSC4Pedestrian::SlowDownForStopLight() @ 0x1000f3562
    0x1000f3796u, // [leaf] cSC4Pedestrian::QueryInterface(unsigned int, void**) [GZCOM boilerplate] @ 0x1000f3796
    0x1000f3c00u, // [leaf] cSC4Pedestrian::GetRadius() const @ 0x1000f3c00
    0x1000f3c22u, // [leaf] cSC4Pedestrian::~cSC4Pedestrian() @ 0x1000f3c22
    0x1000f3c2cu, // [leaf] cSC4Pedestrian::~cSC4Pedestrian() [GZCOM boilerplate] @ 0x1000f3c2c
    0x1000f117cu, // [body] cSC4Pedestrian::HandleCrowdReactionEvent(cS3DVector3&, unsigned int) @ 0x1000f117c
    0x1000f1610u, // [body] cSC4Pedestrian::cSC4Pedestrian(cSC4PedestrianManager*, cGZPersistResourceKey) @ 0x1000f1610
    0x1000f1792u, // [body] cSC4Pedestrian::Init() @ 0x1000f1792
    0x1000f180au, // [body] cSC4Pedestrian::MoveTo(float const*, float const*) @ 0x1000f180a
    0x1000f187au, // [body] cSC4Pedestrian::DoAnimationTick(unsigned int, float) @ 0x1000f187a
    0x1000f190au, // [body] cSC4Pedestrian::ChooseNewDestination() @ 0x1000f190a
    0x1000f1a98u, // [body] cSC4Pedestrian::HandleEnterState(tAutomataState*) @ 0x1000f1a98
    0x1000f1ca8u, // [body] cSC4Pedestrian::DoRiotLeaderTick(float, bool) @ 0x1000f1ca8
    0x1000f1fe0u, // [body] cSC4Pedestrian::HandleExitState(tAutomataState const*) @ 0x1000f1fe0
    0x1000f201au, // [body] cSC4Pedestrian::DoStateTick(float) @ 0x1000f201a
    0x1000f2124u, // [body] cSC4Pedestrian::DoBeeLineTick(float) @ 0x1000f2124
    0x1000f21c2u, // [body] cSC4Pedestrian::DoCrowdTick(float) @ 0x1000f21c2
    0x1000f2470u, // [body] cSC4Pedestrian::DoQueueTick(float) @ 0x1000f2470
    0x1000f2602u, // [body] cSC4Pedestrian::DoStrikeTick(float) @ 0x1000f2602
    0x1000f293au, // [body] cSC4Pedestrian::DoRiotFollowerTick(float) @ 0x1000f293a
    0x1000f2cb2u, // [body] cSC4Pedestrian::DoIgniteTick(float) @ 0x1000f2cb2
    0x1000f2de8u, // [body] cSC4Pedestrian::MoveForwardWithCollision(cS3DVector3&, float) @ 0x1000f2de8
    0x1000f2ffau, // [body] cSC4Pedestrian::TestCollisionExpensive(cS3DVector3&, float) @ 0x1000f2ffa
    0x1000f328eu, // [body] cSC4Pedestrian::VerifyNetworkOccupant() @ 0x1000f328e
    0x1000f3334u, // [body] cSC4Pedestrian::HandleNetworkOccupantRemoved() @ 0x1000f3334
    0x1000f338cu, // [body] cSC4Pedestrian::UpdateAnimation(float) @ 0x1000f338c
    0x1000f34d8u, // [body] cSC4Pedestrian::UpdateOrientation(cS3DVector3 const&, float*) @ 0x1000f34d8
    0x1000f3568u, // [body] cSC4Pedestrian::RecalculatePathOffset() @ 0x1000f3568
    0x1000f35a8u, // [body] cSC4Pedestrian::HandleStopped() @ 0x1000f35a8
    0x1000f385eu, // [body] cSC4Pedestrian::GetTrafficAutomataManager() @ 0x1000f385e
};

// cSC4Train — 33 entry points (8 leaf, 25 body).
inline constexpr Uint64 k_cSC4Train[] = {
    0x100509ca6u, // [leaf] cSC4Train::cSC4Train(cSC4VehicleManager*, cGZPersistResourceKey) @ 0x100509ca6
    0x10050dc60u, // [leaf] cSC4Train::FindDrivingAutomaton() @ 0x10050dc60
    0x10050e116u, // [leaf] cSC4Train::QueryInterface(unsigned int, void**) [GZCOM boilerplate] @ 0x10050e116
    0x10050e298u, // [leaf] cSC4Train::~cSC4Train() [GZCOM boilerplate] @ 0x10050e298
    0x10050e362u, // [leaf] cSC4Train::~cSC4Train() [GZCOM boilerplate] @ 0x10050e362
    0x10050e3a8u, // [leaf] cSC4Train::IsEngine() const @ 0x10050e3a8
    0x10050e3c8u, // [leaf] cSC4Train::GetDesiredNumCars() const @ 0x10050e3c8
    0x1006f31d0u, // [leaf] cSC4Train::GetDesiredLookaheadCount() const @ 0x1006f31d0
    0x100509bd6u, // [body] cSC4Train::cSC4Train(cSC4VehicleManager*, cGZPersistResourceKey) @ 0x100509bd6
    0x100509cb0u, // [body] cSC4Train::Init() @ 0x100509cb0
    0x100509e6au, // [body] cSC4Train::Shutdown() @ 0x100509e6a
    0x100509ff8u, // [body] cSC4Train::MarkTrainInCell(cSC4TrafficAutomaton::tPathNode&, bool) @ 0x100509ff8
    0x10050a072u, // [body] cSC4Train::DoAnimationTick(unsigned int, float) @ 0x10050a072
    0x10050a756u, // [body] cSC4Train::TickFollowingCars(float) @ 0x10050a756
    0x10050b14au, // [body] cSC4Train::DoPathTick(float) @ 0x10050b14a
    0x10050b600u, // [body] cSC4Train::SnapToPath() @ 0x10050b600
    0x10050bb2au, // [body] cSC4Train::SetPath(cISC4NetworkOccupant*, unsigned int, std::__1::vector<cS3DVector3, std::__1::allocator<cS3DVector3>> const*, bool) @ 0x10050bb2a
    0x10050bb86u, // [body] cSC4Train::StartTrip(cISC4NetworkOccupant*, unsigned int, bool) @ 0x10050bb86
    0x10050c09cu, // [body] cSC4Train::ChooseNewDestination() @ 0x10050c09c
    0x10050c25cu, // [body] cSC4Train::HandleEnterState(tAutomataState*) @ 0x10050c25c
    0x10050c360u, // [body] cSC4Train::HandleExitState(tAutomataState const*) @ 0x10050c360
    0x10050c3ceu, // [body] cSC4Train::DoPlayerDriveTick(float) @ 0x10050c3ce
    0x10050d1aau, // [body] cSC4Train::PlayerDriveCheckForDerail() @ 0x10050d1aa
    0x10050d34au, // [body] cSC4Train::TranslatePlayerDriveCommands(tAutomataStatePlayerDrive*, float) @ 0x10050d34a
    0x10050d404u, // [body] cSC4Train::ChooseNewPlayerDriveDestination(cS3DVector3&) @ 0x10050d404
    0x10050d6d2u, // [body] cSC4Train::SetNumCars(unsigned char) @ 0x10050d6d2
    0x10050d770u, // [body] cSC4Train::SetDesiredNumCars(unsigned char) @ 0x10050d770
    0x10050d7b4u, // [body] cSC4Train::AddCarAtEnd() @ 0x10050d7b4
    0x10050da62u, // [body] cSC4Train::GetSpeedModifier() const @ 0x10050da62
    0x10050db5eu, // [body] cSC4Train::HandleExitNetwork(cISC4NetworkOccupant*, unsigned int, cISC4Lot*, SC4Rect<int> const&) @ 0x10050db5e
    0x10050dc6au, // [body] cSC4Train::ShouldCollide(cSC4Automaton*) @ 0x10050dc6a
    0x10050dca0u, // [body] cSC4Train::HandleCollision(cISC4Automaton*, cS3DVector3 const&) @ 0x10050dca0
    0x10050e3b0u, // [body] cSC4Train::GetNumCars() const @ 0x10050e3b0
};

// cSC4EmergencyVehicle — 33 entry points (9 leaf, 24 body).
inline constexpr Uint64 k_cSC4EmergencyVehicle[] = {
    0x1002f5d00u, // [leaf] cSC4EmergencyVehicle::DoHandlingEventTick(float) @ 0x1002f5d00
    0x1002f5f9eu, // [leaf] cSC4EmergencyVehicle::DoPlayerDriveTick(float) @ 0x1002f5f9e
    0x1002f7efau, // [leaf] cSC4EmergencyVehicle::ShowMotionBlur(bool) @ 0x1002f7efa
    0x1002f80a6u, // [leaf] cSC4EmergencyVehicle::GetCallNumber() const @ 0x1002f80a6
    0x1002f80c2u, // [leaf] cSC4EmergencyVehicle::GetDispatchState() const @ 0x1002f80c2
    0x1002f80d2u, // [leaf] cSC4EmergencyVehicle::~cSC4EmergencyVehicle() [GZCOM boilerplate] @ 0x1002f80d2
    0x1002f80d8u, // [leaf] cSC4EmergencyVehicle::~cSC4EmergencyVehicle() [GZCOM boilerplate] @ 0x1002f80d8
    0x10050f2d8u, // [leaf] cSC4EmergencyVehicle::SetCallNumber(unsigned short) @ 0x10050f2d8
    0x10050f550u, // [leaf] cSC4EmergencyVehicle::GetDispatchDestination() const @ 0x10050f550
    0x1002f5580u, // [body] cSC4EmergencyVehicle::cSC4EmergencyVehicle() @ 0x1002f5580
    0x1002f563cu, // [body] cSC4EmergencyVehicle::Init() @ 0x1002f563c
    0x1002f56dcu, // [body] cSC4EmergencyVehicle::Shutdown() @ 0x1002f56dc
    0x1002f576eu, // [body] cSC4EmergencyVehicle::DoAnimationTick(unsigned int, float) @ 0x1002f576e
    0x1002f57fcu, // [body] cSC4EmergencyVehicle::DoStateTick(float) @ 0x1002f57fc
    0x1002f58d4u, // [body] cSC4EmergencyVehicle::DoEnRouteTick(float) @ 0x1002f58d4
    0x1002f5a26u, // [body] cSC4EmergencyVehicle::ValidateNetworkOccupant() @ 0x1002f5a26
    0x1002f5ae2u, // [body] cSC4EmergencyVehicle::DoRepathingTick(float) @ 0x1002f5ae2
    0x1002f5c36u, // [body] cSC4EmergencyVehicle::SendPathBlockedMessage() @ 0x1002f5c36
    0x1002f5d08u, // [body] cSC4EmergencyVehicle::DoTeleportTick(float) @ 0x1002f5d08
    0x1002f5fa8u, // [body] cSC4EmergencyVehicle::UpdateAttraction() @ 0x1002f5fa8
    0x1002f600cu, // [body] cSC4EmergencyVehicle::HandlePlayerDriveCommand(cISC4Automaton::ePlayerDriveCommand, bool) @ 0x1002f600c
    0x1002f603cu, // [body] cSC4EmergencyVehicle::HandleEnterState(tAutomataState*) @ 0x1002f603c
    0x1002f611eu, // [body] cSC4EmergencyVehicle::HandleExitState(tAutomataState const*) @ 0x1002f611e
    0x1002f615au, // [body] cSC4EmergencyVehicle::HandleReachedRouteDestination() @ 0x1002f615a
    0x1002f62e6u, // [body] cSC4EmergencyVehicle::HandleNetworkOccupantRemoved() @ 0x1002f62e6
    0x1002f6322u, // [body] cSC4EmergencyVehicle::HandleDispatched(cS3DVector3 const&, cISC4Occupant*) @ 0x1002f6322
    0x1002f64bcu, // [body] cSC4EmergencyVehicle::AppendRouteToNearbyRoad(int, int, cS3DVector3 const&) @ 0x1002f64bc
    0x1002f7670u, // [body] cSC4EmergencyVehicle::ReturnToSourceOccupant() @ 0x1002f7670
    0x1002f76e4u, // [body] cSC4EmergencyVehicle::GetDebugExtraText() const @ 0x1002f76e4
    0x1002f78e6u, // [body] cSC4EmergencyVehicle::ShouldCollide(cSC4Automaton*) @ 0x1002f78e6
    0x1002f796cu, // [body] cSC4EmergencyVehicle::SetDispatchState(cISC4DispatchManager::tDispatchState) @ 0x1002f796c
    0x1002f7bf8u, // [body] cSC4EmergencyVehicle::CloseEnoughToDispatchDestination() const @ 0x1002f7bf8
    0x1002f7cdcu, // [body] cSC4EmergencyVehicle::ShowEmergencyLights(bool) @ 0x1002f7cdc
};

// cSC4DispatchVehicleView — 32 entry points (8 leaf, 24 body).
inline constexpr Uint64 k_cSC4DispatchVehicleView[] = {
    0x10023066cu, // [leaf] cSC4DispatchVehicleView::cSC4DispatchVehicleView() @ 0x10023066c
    0x100230676u, // [leaf] cSC4DispatchVehicleView::~cSC4DispatchVehicleView() [GZCOM boilerplate] @ 0x100230676
    0x1002306ecu, // [leaf] cSC4DispatchVehicleView::~cSC4DispatchVehicleView() @ 0x1002306ec
    0x100230704u, // [leaf] cSC4DispatchVehicleView::~cSC4DispatchVehicleView() [GZCOM boilerplate] @ 0x100230704
    0x100233ad2u, // [leaf] cSC4DispatchVehicleView::SetIndicatorMax(cSC4DispatchVehicleView::TrackerType, unsigned int) @ 0x100233ad2
    0x10023446cu, // [leaf] cSC4DispatchVehicleView::QueryInterface(unsigned int, void**) [GZCOM boilerplate] @ 0x10023446c
    0x1002344feu, // [leaf] cSC4DispatchVehicleView::AddRef() @ 0x1002344fe
    0x10023450cu, // [leaf] cSC4DispatchVehicleView::Release() @ 0x10023450c
    0x100230560u, // [body] cSC4DispatchVehicleView::cSC4DispatchVehicleView() @ 0x100230560
    0x100230740u, // [body] cSC4DispatchVehicleView::Init() @ 0x100230740
    0x100230a1cu, // [body] cSC4DispatchVehicleView::Shutdown() @ 0x100230a1c
    0x100230c1au, // [body] cSC4DispatchVehicleView::BuildWindowClipList() @ 0x100230c1a
    0x100230cb8u, // [body] cSC4DispatchVehicleView::sEnumChildWindows(cIGZWin*, unsigned int, void*, void*) @ 0x100230cb8
    0x100230e16u, // [body] cSC4DispatchVehicleView::testOverlap(cSC4DispatchVehicleView::tRectInfo&, std::__1::list<cSC4DispatchVehicleView::tRectInfo, std::__1::allocator<cSC4DispatchVehicleView::tRectInfo>>&, SC4Rect<int>&, cSC4DispatchVehicleView::tRectInfo&) @ 0x100230e16
    0x100230ea6u, // [body] cSC4DispatchVehicleView::Update(SC4DrawContext*) @ 0x100230ea6
    0x100232478u, // [body] cSC4DispatchVehicleView::RemoveTrackInfo(std::__1::__map_iterator<std::__1::__tree_iterator<std::__1::__value_type<unsigned char, cSC4DispatchVehicleView::TrackInfo>, std::__1::__tree_node<std::__1::__value_type<unsigned char, cSC4DispatchVehicleView::TrackInfo>, void*>*, long>>, cSC4DispatchVehicleView::TrackInfo&) @ 0x100232478
    0x1002325b2u, // [body] cSC4DispatchVehicleView::SortIndicatorPriority(cISC4Occupant*) @ 0x1002325b2
    0x100232890u, // [body] cSC4DispatchVehicleView::Update3DModelIndicator(SC4DrawContext*, cSC4DispatchVehicleView::TrackInfo&, float) @ 0x100232890
    0x100232cc8u, // [body] cSC4DispatchVehicleView::HandleViewChanged() @ 0x100232cc8
    0x100232d38u, // [body] cSC4DispatchVehicleView::BuildCallNumberDisplay(cSC4DispatchVehicleView::TrackInfo&) @ 0x100232d38
    0x1002336b2u, // [body] cSC4DispatchVehicleView::FindIndicatorHandle(int, int) @ 0x1002336b2
    0x10023377eu, // [body] cSC4DispatchVehicleView::IsSelectableIndicator(cSC4DispatchVehicleView::TrackInfo const&) @ 0x10023377e
    0x1002337d0u, // [body] cSC4DispatchVehicleView::SetActiveIndicator(unsigned int, cSC4DispatchVehicleView::tButtonState) @ 0x1002337d0
    0x10023389cu, // [body] cSC4DispatchVehicleView::FindTrackInfoByHandle(unsigned int) @ 0x10023389c
    0x1002338f8u, // [body] cSC4DispatchVehicleView::LockIndicator(unsigned int, bool) @ 0x1002338f8
    0x100233938u, // [body] cSC4DispatchVehicleView::FlashIndicator(unsigned int, bool, float) @ 0x100233938
    0x1002339b0u, // [body] cSC4DispatchVehicleView::GetActiveOccupant() @ 0x1002339b0
    0x1002339dcu, // [body] cSC4DispatchVehicleView::ShowAllIndicators(cSC4DispatchVehicleView::TrackerType, bool) @ 0x1002339dc
    0x100233ae2u, // [body] cSC4DispatchVehicleView::TrackOccupant(cISC4Occupant*, cSC4DispatchVehicleView::TrackerType, unsigned int, cRZString const*) @ 0x100233ae2
    0x1002342e0u, // [body] cSC4DispatchVehicleView::UntrackOccupant(unsigned int, bool) @ 0x1002342e0
    0x10023439cu, // [body] cSC4DispatchVehicleView::GetLastScreenIconRect(unsigned int) @ 0x10023439c
    0x1002343d8u, // [body] cSC4DispatchVehicleView::DoMessage(cIGZMessage2*) @ 0x1002343d8
};

// cSC4FirePlaneDispatchSource — 25 entry points (9 leaf, 16 body).
inline constexpr Uint64 k_cSC4FirePlaneDispatchSource[] = {
    0x100722edcu, // [leaf] cSC4FirePlaneDispatchSource::~cSC4FirePlaneDispatchSource() [GZCOM boilerplate] @ 0x100722edc
    0x100722f16u, // [leaf] cSC4FirePlaneDispatchSource::~cSC4FirePlaneDispatchSource() [GZCOM boilerplate] @ 0x100722f16
    0x100723136u, // [leaf] cSC4FirePlaneDispatchSource::QueryInterface(unsigned int, void**) [GZCOM boilerplate] @ 0x100723136
    0x10072318eu, // [leaf] cSC4FirePlaneDispatchSource::AddRef() @ 0x10072318e
    0x1007231b8u, // [leaf] cSC4FirePlaneDispatchSource::Release() [GZCOM boilerplate] @ 0x1007231b8
    0x1007234d8u, // [leaf] cSC4FirePlaneDispatchSource::CanHandleDispatchType(unsigned int) const @ 0x1007234d8
    0x10072392au, // [leaf] cSC4FirePlaneDispatchSource::RevokeDispatchFromStation(cISC4Occupant*, unsigned int) @ 0x10072392a
    0x100723934u, // [leaf] cSC4FirePlaneDispatchSource::DoDispatchBegin(unsigned int) @ 0x100723934
    0x1007239a2u, // [leaf] cSC4FirePlaneDispatchSource::GetDispatchAutomataProperties(cISC4Occupant const*, unsigned int&, cGZPersistResourceKey&, unsigned int&, cS3DVector3 const&) @ 0x1007239a2
    0x100722e6cu, // [body] cSC4FirePlaneDispatchSource::cSC4FirePlaneDispatchSource() @ 0x100722e6c
    0x100722f62u, // [body] cSC4FirePlaneDispatchSource::Init() @ 0x100722f62
    0x10072303eu, // [body] cSC4FirePlaneDispatchSource::Shutdown() @ 0x10072303e
    0x10072331cu, // [body] cSC4FirePlaneDispatchSource::DoMessage(cIGZMessage2*) @ 0x10072331c
    0x1007234e4u, // [body] cSC4FirePlaneDispatchSource::CanDispatch(float, float, bool*) @ 0x1007234e4
    0x100723532u, // [body] cSC4FirePlaneDispatchSource::GetMaxPossibleDispatchCount() @ 0x100723532
    0x100723592u, // [body] cSC4FirePlaneDispatchSource::GetStations(ilist<cISC4Occupant*>&) @ 0x100723592
    0x1007235a6u, // [body] cSC4FirePlaneDispatchSource::CopyOccupantList(ilist<cISC4Occupant*>&, ilist<cISC4Occupant*>&) @ 0x1007235a6
    0x100723624u, // [body] cSC4FirePlaneDispatchSource::GetDispatchesAtStation(cISC4Occupant*, unsigned int*, unsigned int*, unsigned int*) @ 0x100723624
    0x1007236ccu, // [body] cSC4FirePlaneDispatchSource::DispatchFromNearestStation(float, float, unsigned int*) @ 0x1007236cc
    0x1007237dcu, // [body] cSC4FirePlaneDispatchSource::DispatchFromStation(cISC4Occupant*, unsigned int*) @ 0x1007237dc
    0x100723894u, // [body] cSC4FirePlaneDispatchSource::ReturnDispatchToStation(cISC4Occupant*, unsigned int) @ 0x100723894
    0x10072393au, // [body] cSC4FirePlaneDispatchSource::DoDispatchActivated(unsigned int, bool) @ 0x10072393a
    0x100723962u, // [body] cSC4FirePlaneDispatchSource::DoDispatchEnd(unsigned int) @ 0x100723962
    0x1007239c6u, // [body] cSC4FirePlaneDispatchSource::DoOccupantInserted(cISC4Occupant*) @ 0x1007239c6
    0x100723beau, // [body] cSC4FirePlaneDispatchSource::DoOccupantRemoved(cISC4Occupant*) @ 0x100723bea
};

// cSC4PoliceHelicopterDispatchSource — 25 entry points (9 leaf, 16 body).
inline constexpr Uint64 k_cSC4PoliceHelicopterDispatchSource[] = {
    0x100724736u, // [leaf] cSC4PoliceHelicopterDispatchSource::~cSC4PoliceHelicopterDispatchSource() [GZCOM boilerplate] @ 0x100724736
    0x100724770u, // [leaf] cSC4PoliceHelicopterDispatchSource::~cSC4PoliceHelicopterDispatchSource() [GZCOM boilerplate] @ 0x100724770
    0x100724990u, // [leaf] cSC4PoliceHelicopterDispatchSource::QueryInterface(unsigned int, void**) [GZCOM boilerplate] @ 0x100724990
    0x1007249e8u, // [leaf] cSC4PoliceHelicopterDispatchSource::AddRef() @ 0x1007249e8
    0x100724a12u, // [leaf] cSC4PoliceHelicopterDispatchSource::Release() [GZCOM boilerplate] @ 0x100724a12
    0x100724d32u, // [leaf] cSC4PoliceHelicopterDispatchSource::CanHandleDispatchType(unsigned int) const @ 0x100724d32
    0x100724d8cu, // [leaf] cSC4PoliceHelicopterDispatchSource::DoDispatchBegin(unsigned int) @ 0x100724d8c
    0x1007251f2u, // [leaf] cSC4PoliceHelicopterDispatchSource::RevokeDispatchFromStation(cISC4Occupant*, unsigned int) @ 0x1007251f2
    0x1007251fcu, // [leaf] cSC4PoliceHelicopterDispatchSource::GetDispatchAutomataProperties(cISC4Occupant const*, unsigned int&, cGZPersistResourceKey&, unsigned int&, cS3DVector3 const&) @ 0x1007251fc
    0x1007246c6u, // [body] cSC4PoliceHelicopterDispatchSource::cSC4PoliceHelicopterDispatchSource() @ 0x1007246c6
    0x1007247bcu, // [body] cSC4PoliceHelicopterDispatchSource::Init() @ 0x1007247bc
    0x100724898u, // [body] cSC4PoliceHelicopterDispatchSource::Shutdown() @ 0x100724898
    0x100724b76u, // [body] cSC4PoliceHelicopterDispatchSource::DoMessage(cIGZMessage2*) @ 0x100724b76
    0x100724d3eu, // [body] cSC4PoliceHelicopterDispatchSource::CanDispatch(float, float, bool*) @ 0x100724d3e
    0x100724d92u, // [body] cSC4PoliceHelicopterDispatchSource::DoDispatchActivated(unsigned int, bool) @ 0x100724d92
    0x100724dbau, // [body] cSC4PoliceHelicopterDispatchSource::DoDispatchEnd(unsigned int) @ 0x100724dba
    0x100724dfau, // [body] cSC4PoliceHelicopterDispatchSource::ReturnDispatchToStation(cISC4Occupant*, unsigned int) @ 0x100724dfa
    0x100724e90u, // [body] cSC4PoliceHelicopterDispatchSource::GetMaxPossibleDispatchCount() @ 0x100724e90
    0x100724ef0u, // [body] cSC4PoliceHelicopterDispatchSource::GetStations(ilist<cISC4Occupant*>&) @ 0x100724ef0
    0x100724f04u, // [body] cSC4PoliceHelicopterDispatchSource::CopyOccupantList(ilist<cISC4Occupant*>&, ilist<cISC4Occupant*>&) @ 0x100724f04
    0x100724f82u, // [body] cSC4PoliceHelicopterDispatchSource::GetDispatchesAtStation(cISC4Occupant*, unsigned int*, unsigned int*, unsigned int*) @ 0x100724f82
    0x10072502au, // [body] cSC4PoliceHelicopterDispatchSource::DispatchFromNearestStation(float, float, unsigned int*) @ 0x10072502a
    0x10072513au, // [body] cSC4PoliceHelicopterDispatchSource::DispatchFromStation(cISC4Occupant*, unsigned int*) @ 0x10072513a
    0x100725220u, // [body] cSC4PoliceHelicopterDispatchSource::DoOccupantInserted(cISC4Occupant*) @ 0x100725220
    0x100725444u, // [body] cSC4PoliceHelicopterDispatchSource::DoOccupantRemoved(cISC4Occupant*) @ 0x100725444
};

// cSC4SimFireCrew — 23 entry points (8 leaf, 15 body).
inline constexpr Uint64 k_cSC4SimFireCrew[] = {
    0x100503e3cu, // [leaf] cSC4SimFireCrew::cSC4SimFireCrew(cSC4FireTruck*) @ 0x100503e3c
    0x100504ed0u, // [leaf] cSC4SimFireCrew::SetCompetence(SC4Percentage const&) @ 0x100504ed0
    0x100506490u, // [leaf] cSC4SimFireCrew::QueryInterface(unsigned int, void**) @ 0x100506490
    0x1005064e0u, // [leaf] cSC4SimFireCrew::AddRef() @ 0x1005064e0
    0x100506530u, // [leaf] cSC4SimFireCrew::Release() @ 0x100506530
    0x1005067cau, // [leaf] cSC4SimFireCrew::~cSC4SimFireCrew() @ 0x1005067ca
    0x1005067d4u, // [leaf] cSC4SimFireCrew::~cSC4SimFireCrew() [GZCOM boilerplate] @ 0x1005067d4
    0x100506964u, // [leaf] cSC4SimFireCrew::~cSC4SimFireCrew() [GZCOM boilerplate] @ 0x100506964
    0x100503b70u, // [body] cSC4SimFireCrew::cSC4SimFireCrew(cSC4FireTruck*) @ 0x100503b70
    0x100503e46u, // [body] cSC4SimFireCrew::Init() @ 0x100503e46
    0x100504030u, // [body] cSC4SimFireCrew::SendFireCrewMessage(unsigned int, cISC4Occupant*, unsigned int, unsigned int, unsigned int, unsigned int) @ 0x100504030
    0x100504120u, // [body] cSC4SimFireCrew::InitRegTranslationArray() @ 0x100504120
    0x100504266u, // [body] cSC4SimFireCrew::Shutdown() @ 0x100504266
    0x10050438eu, // [body] cSC4SimFireCrew::SetClosestFire(cISC4FireOccupant*, bool) @ 0x10050438e
    0x100504434u, // [body] cSC4SimFireCrew::SetClosestToxicSpill(cISC4ToxicOccupant*, bool) @ 0x100504434
    0x1005044e8u, // [body] cSC4SimFireCrew::ClampToCityBounds(cS3DVector3&) @ 0x1005044e8
    0x10050458au, // [body] cSC4SimFireCrew::SetupFireCrew() @ 0x10050458a
    0x100504edeu, // [body] cSC4SimFireCrew::DoAnimationTick(unsigned int, float) @ 0x100504ede
    0x10050583au, // [body] cSC4SimFireCrew::FindTargetsInRadius(cS3DVector3 const*) @ 0x10050583a
    0x1005060b6u, // [body] cSC4SimFireCrew::GetWaterHoseTransform(cS3DTransform&) @ 0x1005060b6
    0x10050629cu, // [body] cSC4SimFireCrew::SetZoomAndRotation(int, int) @ 0x10050629c
    0x100506326u, // [body] cSC4SimFireCrew::SetVisible(bool) @ 0x100506326
    0x10050639cu, // [body] cSC4SimFireCrew::DoMessage(cIGZMessage2*) @ 0x10050639c
};

// cSC4FireTruck — 21 entry points (6 leaf, 15 body).
inline constexpr Uint64 k_cSC4FireTruck[] = {
    0x10050f91cu, // [leaf] cSC4FireTruck::cSC4FireTruck() @ 0x10050f91c
    0x10050fba8u, // [leaf] cSC4FireTruck::HandleReachedRouteDestination() @ 0x10050fba8
    0x100510816u, // [leaf] cSC4FireTruck::~cSC4FireTruck() @ 0x100510816
    0x100510820u, // [leaf] cSC4FireTruck::~cSC4FireTruck() [GZCOM boilerplate] @ 0x100510820
    0x100510866u, // [leaf] cSC4FireTruck::GetDispatchType() const @ 0x100510866
    0x100510a4cu, // [leaf] cSC4FireTruck::~cSC4FireTruck() [GZCOM boilerplate] @ 0x100510a4c
    0x10050f670u, // [body] cSC4FireTruck::cSC4FireTruck() @ 0x10050f670
    0x10050f926u, // [body] cSC4FireTruck::Shutdown() @ 0x10050f926
    0x10050f97cu, // [body] cSC4FireTruck::ReleaseFireCrew() @ 0x10050f97c
    0x10050f9ecu, // [body] cSC4FireTruck::ChooseNewDestination() @ 0x10050f9ec
    0x10050fbb2u, // [body] cSC4FireTruck::HandleCrewFinished() @ 0x10050fbb2
    0x10050fbf2u, // [body] cSC4FireTruck::SetDispatchState(cISC4DispatchManager::tDispatchState) @ 0x10050fbf2
    0x10050fd50u, // [body] cSC4FireTruck::GetFireCount(int&, int&) @ 0x10050fd50
    0x10050ff3cu, // [body] cSC4FireTruck::GetToxicSpillCount(int&, int&) @ 0x10050ff3c
    0x100510060u, // [body] cSC4FireTruck::CreateFireCrew() @ 0x100510060
    0x10051019cu, // [body] cSC4FireTruck::GetEffectiveRadius() const @ 0x10051019c
    0x1005101fcu, // [body] cSC4FireTruck::DoHandlingEventTick(float) @ 0x1005101fc
    0x1005102c2u, // [body] cSC4FireTruck::HandlePlayerDriveCommand(cISC4Automaton::ePlayerDriveCommand, bool) @ 0x1005102c2
    0x100510444u, // [body] cSC4FireTruck::HandleExitState(tAutomataState const*) @ 0x100510444
    0x100510474u, // [body] cSC4FireTruck::TranslatePlayerDriveCommands(tAutomataStatePlayerDrive*, float) @ 0x100510474
    0x100510680u, // [body] cSC4FireTruck::GetStreamTargetPoint(cS3DVector3&) @ 0x100510680
};

// cSC4Tank — 22 entry points (7 leaf, 15 body).
inline constexpr Uint64 k_cSC4Tank[] = {
    0x1006f4062u, // [leaf] cSC4Tank::cSC4Tank() @ 0x1006f4062
    0x1006f4688u, // [leaf] cSC4Tank::ShowHeadlights(bool, bool) @ 0x1006f4688
    0x1006f468eu, // [leaf] cSC4Tank::InitGhostModel() @ 0x1006f468e
    0x1006f46c4u, // [leaf] cSC4Tank::UpdateGhostModel() @ 0x1006f46c4
    0x1006f6c82u, // [leaf] cSC4Tank::~cSC4Tank() @ 0x1006f6c82
    0x1006f6c8cu, // [leaf] cSC4Tank::~cSC4Tank() [GZCOM boilerplate] @ 0x1006f6c8c
    0x1006f6fb2u, // [leaf] cSC4Tank::~cSC4Tank() [GZCOM boilerplate] @ 0x1006f6fb2
    0x1006f3ea0u, // [body] cSC4Tank::cSC4Tank() @ 0x1006f3ea0
    0x1006f406cu, // [body] cSC4Tank::Init() @ 0x1006f406c
    0x1006f433au, // [body] cSC4Tank::Shutdown() @ 0x1006f433a
    0x1006f458eu, // [body] cSC4Tank::GetDebugExtraText() const @ 0x1006f458e
    0x1006f4698u, // [body] cSC4Tank::ShutdownGhostModel() @ 0x1006f4698
    0x1006f46ceu, // [body] cSC4Tank::HandleEnterState(tAutomataState*) @ 0x1006f46ce
    0x1006f47aau, // [body] cSC4Tank::DoPlayerDriveTick(float) @ 0x1006f47aa
    0x1006f4cfcu, // [body] cSC4Tank::TranslatePlayerDriveCommands(tAutomataStatePlayerDrive*, float) @ 0x1006f4cfc
    0x1006f5498u, // [body] cSC4Tank::DoAnimationTick(unsigned int, float) @ 0x1006f5498
    0x1006f6012u, // [body] cSC4Tank::InitTurretGhostModel() @ 0x1006f6012
    0x1006f6260u, // [body] cSC4Tank::ShutdownTurretGhostModel() @ 0x1006f6260
    0x1006f630cu, // [body] cSC4Tank::UpdateTurretGhostModel() @ 0x1006f630c
    0x1006f664eu, // [body] cSC4Tank::InitBarrelGhostModel() @ 0x1006f664e
    0x1006f6892u, // [body] cSC4Tank::ShutdownBarrelGhostModel() @ 0x1006f6892
    0x1006f693eu, // [body] cSC4Tank::UpdateBarrelGhostModel() @ 0x1006f693e
};

// cSC4TrailerVehicle — 21 entry points (7 leaf, 14 body).
inline constexpr Uint64 k_cSC4TrailerVehicle[] = {
    0x100507802u, // [leaf] cSC4TrailerVehicle::cSC4TrailerVehicle(cSC4VehicleManager*, cGZPersistResourceKey) @ 0x100507802
    0x1005079c0u, // [leaf] cSC4TrailerVehicle::ShowHeadlights(bool, bool) @ 0x1005079c0
    0x100508a24u, // [leaf] cSC4TrailerVehicle::GetCollidePosition(cS3DVector3&) @ 0x100508a24
    0x100508d30u, // [leaf] cSC4TrailerVehicle::QueryInterface(unsigned int, void**) [GZCOM boilerplate] @ 0x100508d30
    0x100508dfau, // [leaf] cSC4TrailerVehicle::~cSC4TrailerVehicle() @ 0x100508dfa
    0x100508e04u, // [leaf] cSC4TrailerVehicle::~cSC4TrailerVehicle() [GZCOM boilerplate] @ 0x100508e04
    0x100509192u, // [leaf] cSC4TrailerVehicle::~cSC4TrailerVehicle() [GZCOM boilerplate] @ 0x100509192
    0x100507720u, // [body] cSC4TrailerVehicle::cSC4TrailerVehicle(cSC4VehicleManager*, cGZPersistResourceKey) @ 0x100507720
    0x10050780cu, // [body] cSC4TrailerVehicle::Init() @ 0x10050780c
    0x100507868u, // [body] cSC4TrailerVehicle::Shutdown() @ 0x100507868
    0x100507904u, // [body] cSC4TrailerVehicle::SetAutomataGroup(cSC4OccupantGroupTemplate*) @ 0x100507904
    0x10050795cu, // [body] cSC4TrailerVehicle::SetPath(cISC4NetworkOccupant*, unsigned int, std::__1::vector<cS3DVector3, std::__1::allocator<cS3DVector3>> const*, bool) @ 0x10050795c
    0x1005079d2u, // [body] cSC4TrailerVehicle::DoAnimationTick(unsigned int, float) @ 0x1005079d2
    0x100507b1au, // [body] cSC4TrailerVehicle::TickFollowingCars(float) @ 0x100507b1a
    0x100508122u, // [body] cSC4TrailerVehicle::TickFollowingCarsOffRoad(float) @ 0x100508122
    0x100508558u, // [body] cSC4TrailerVehicle::FindPointOnPathSegment(cS3DVector3 const&, cS3DVector3 const&, cS3DVector3 const&, cS3DVector3&) @ 0x100508558
    0x10050876au, // [body] cSC4TrailerVehicle::AddTrailer(unsigned int) @ 0x10050876a
    0x1005087a8u, // [body] cSC4TrailerVehicle::AddCarAtEnd() @ 0x1005087a8
    0x1005089eeu, // [body] cSC4TrailerVehicle::ShouldCollide(cSC4Automaton*) @ 0x1005089ee
    0x100508e7au, // [body] cSC4TrailerVehicle::GetNumCars() const @ 0x100508e7a
    0x100508e92u, // [body] cSC4TrailerVehicle::GetDesiredNumCars() const @ 0x100508e92
};

inline constexpr bool AddressCited(Uint64 address) noexcept {
    return AnySite(address, k_cSC4Automaton, k_cSC4AutomataManager, k_cSC4Watercraft, k_cSC4WatercraftManager, k_cSC4TrafficAutomaton, k_cSC4TrafficAutomataManager, k_cSC4MySimDispatch, k_cSC4Vehicle, k_cSC4Helicopter, k_cSC4Airplane, k_cSC4AutomataControllerManager, k_cSC4AutomataController, k_cSC4AutomataGenerator, k_cSC4AutomataCOMDirector, k_cSC4Pedestrian, k_cSC4Train, k_cSC4EmergencyVehicle, k_cSC4DispatchVehicleView, k_cSC4FirePlaneDispatchSource, k_cSC4PoliceHelicopterDispatchSource, k_cSC4SimFireCrew, k_cSC4FireTruck, k_cSC4Tank, k_cSC4TrailerVehicle);
}

inline constexpr Uint32 CitedCount() noexcept {
    return SiteTotal(k_cSC4Automaton, k_cSC4AutomataManager, k_cSC4Watercraft, k_cSC4WatercraftManager, k_cSC4TrafficAutomaton, k_cSC4TrafficAutomataManager, k_cSC4MySimDispatch, k_cSC4Vehicle, k_cSC4Helicopter, k_cSC4Airplane, k_cSC4AutomataControllerManager, k_cSC4AutomataController, k_cSC4AutomataGenerator, k_cSC4AutomataCOMDirector, k_cSC4Pedestrian, k_cSC4Train, k_cSC4EmergencyVehicle, k_cSC4DispatchVehicleView, k_cSC4FirePlaneDispatchSource, k_cSC4PoliceHelicopterDispatchSource, k_cSC4SimFireCrew, k_cSC4FireTruck, k_cSC4Tank, k_cSC4TrailerVehicle);
}

inline constexpr bool IndexSelfTest() noexcept {
    if (ApplyAutomataFlagWord(0x7u, 0x4u, false) != 0x3u) {
        return false;
    }
    if (ApplyAutomataFlagWord(0x1u, 0x4u, true) != 0x5u) {
        return false;
    }
    if (!CheckAutomataFlagWord(0x5u, 0x1u) || CheckAutomataFlagWord(0x5u, 0x2u)) {
        return false;
    }
    if (ClassifyAutomataState(0x34u) != AutomataStateType::Chase) {
        return false;
    }
    if (ClassifyAutomataState(1u) != AutomataStateType::Plain) {
        return false;
    }
    if (!BaseDoStateTickCovers(0x13u) || BaseDoStateTickCovers(20u)) {
        return false;
    }
    const AutomataTransition cross = MakeAutomataTransition(AutomataStackOp::Push, 11u, 20u);
    if (!cross.crossesTickTable || !cross.specializedClassChanges) {
        return false;
    }
    const AutomataTransition same = MakeAutomataTransition(AutomataStackOp::Set, 11u, 11u);
    if (same.crossesTickTable || same.specializedClassChanges) {
        return false;
    }
    if (StackDepthAfterPush(0u) != 1u || StackDepthAfterPop(0u) != 0u || StackDepthAfterPop(2u) != 1u) {
        return false;
    }
    const AutomatonPriorityVisibility marked = WithFlag(AutomatonPriorityVisibility{2u, 0u, false}, 0x8u, true);
    if (PriorityOf(marked) != 2u || PriorityRank(marked) != 2 || VisibleOf(marked)) {
        return false;
    }
    if (!VisibleOf(WithVisibility(marked, true)) || marked.flags != 0x8u) {
        return false;
    }
    if (!WatercraftGroupTakesPriorityPath(0x4400u) || !WatercraftGroupTakesPriorityPath(0x4402u)) {
        return false;
    }
    if (WatercraftGroupTakesPriorityPath(0x4401u)) {
        return false;
    }
    const WatercraftStateView boat{4u, false, true};
    if (!WatercraftAcceptsChange(boat) || WatercraftChangeStateInRange(5u) || WatercraftOnLand(boat)) {
        return false;
    }
    if (!WatercraftOutOfBounds(boat)) {
        return false;
    }
    const VehicleMotionView vehicle{10.0f, 4.0f, true, false, true};
    if (VehicleDefaultSpeed(vehicle) != 10.0f || VehicleSpeedSample(vehicle) != 4.0f) {
        return false;
    }
    if (!VehicleShowsHeadlights(vehicle) || VehicleShowsMotionBlur(vehicle) || !VehicleShowsEmergencyLights(vehicle)) {
        return false;
    }
    const AirplaneStateStep takeoff = StepAirplaneState(0u, 1u, true);
    if (!takeoff.writePhaseThree || takeoff.aems != AirplaneAemsSlot::Slot28) {
        return false;
    }
    if (StepAirplaneState(3u, 1u, true).aems != AirplaneAemsSlot::Slot30Then28) {
        return false;
    }
    if (StepAirplaneState(1u, 0u, true).aems != AirplaneAemsSlot::Slot30) {
        return false;
    }
    if (StepAirplaneState(2u, 2u, true).aems != AirplaneAemsSlot::None) {
        return false;
    }
    if (!AirplaneStateAtLeastTwo(2u) || AirplaneStateAtLeastTwo(1u)) {
        return false;
    }
    const HelicopterPhysicsUpdate idle = HelicopterPhysicsForState(0u);
    const HelicopterPhysicsUpdate transit = HelicopterPhysicsForState(4u);
    const HelicopterPhysicsUpdate other = HelicopterPhysicsForState(2u);
    if (!idle.rewrite || idle.mode != 0u || !transit.rewrite || transit.mode != 1u || other.rewrite) {
        return false;
    }
    const AircraftStateView craft{0u, 0u, 1u, true};
    if (!AirplaneStep(craft, 1u).writePhaseThree || !AirplanePastTakeoff(AircraftStateView{2u, 3u, 0u, false})) {
        return false;
    }
    if (!HelicopterStep(craft).rewrite || HelicopterStep(craft).mode != 1u) {
        return false;
    }
    const MySimStateView stepped = CommitMySimState(MySimStateView{1u, 0u, 0x10u}, 3u);
    if (stepped.current != 3u || stepped.previous != 1u || !MySimStateChanged(stepped)) {
        return false;
    }
    if (!MySimStateRequestsFlagUpdate(3u) || MySimStateRequestsFlagUpdate(2u) || MySimFlags(stepped) != 0x10u) {
        return false;
    }
    if (CitedCount() != 1071u) {
        return false;
    }
    return AddressCited(0x1000e24f0u) && AddressCited(0x1000e2446u) && AddressCited(0x1004ef26au) && AddressCited(0x1004eddacu) && AddressCited(0x1000e25d0u) && AddressCited(0x100111c96u) && AddressCited(0x1006ac7c4u) && AddressCited(0x1000dabf0u) && AddressCited(0x1006e86ccu) && AddressCited(0x1006ccf26u) && AddressCited(0x1000f3fc8u) && AddressCited(0x1000f85b0u) && AddressCited(0x1000fa4e6u) && AddressCited(0x1004db628u) && AddressCited(0x1000f13eau) && AddressCited(0x100509ca6u) && AddressCited(0x1002f5d00u) && AddressCited(0x10023066cu) && AddressCited(0x100722edcu) && AddressCited(0x100724736u) && AddressCited(0x100503e3cu) && AddressCited(0x10050f91cu) && AddressCited(0x1006f4062u) && AddressCited(0x100507802u);
}

static_assert(CitedCount() == 1071u);
static_assert(IndexSelfTest());

} // namespace automata_index

// Wave 3 Part 3 entry points: 74 classes, 452 methods (0x100xxxxxx).
struct Wave3Part3Method {
    Uint64 address;
    Uint32 sizeBytes;
    char const* name;
};

inline constexpr Wave3Part3Method kWave3Part3Methods[] = {
    // cSTETerrainView3D (78)
    {0x1007460feu, 62u, "cSTETerrainView3D::AddSpansCorrespondingToTexture(int, unsigned int, std::__1::vector<cSTETerrainView3D::tRenderTexInfo, std::__1::allocator<cSTETerrainView3D::tRenderTexInfo>>&)"},
    {0x10074613cu, 352u, "cSTETerrainView3D::AddSpansThatNeedToBeRendered(int, cSTETerrainView3D::tTextureSpanInfo const&, std::__1::vector<cSTETerrainView3D::tRenderTexInfo, std::__1::allocator<cSTETerrainView3D::tRenderTexInfo>>&)"},
    {0x10074629cu, 242u, "cSTETerrainView3D::AddTextureSpan(std::__1::list<cSTETerrainView3D::tTextureSpanInfo, std::__1::allocator<cSTETerrainView3D::tTextureSpanInfo>>&, unsigned int, int, int)"},
    {0x100746402u, 182u, "cSTETerrainView3D::MergeTextureSpan(cSTETerrainView3D::tTextureSpanInfo&, cSTETerrainView3D::tSpan&)"},
    {0x1007464b8u, 620u, "cSTETerrainView3D::AssignTerrainAndWaterTextureCoordinates(SC4DrawContext*)"},
    {0x100746724u, 56u, "cSTETerrainView3D::ChangeRotation(SC4DrawContext*)"},
    {0x10074675cu, 110u, "cSTETerrainView3D::SetWaterColorUpdateRect(SC4Rect<int> const*)"},
    {0x1007467cau, 52u, "cSTETerrainView3D::ChangeTerrainType(int)"},
    {0x1007467feu, 142u, "cSTETerrainView3D::ReleaseLoadedTerrainTextures()"},
    {0x10074688cu, 228u, "cSTETerrainView3D::SetUpTexturesForCurrentTerrainTypeAndZoom(int)"},
    {0x100746970u, 176u, "cSTETerrainView3D::ChangeZoomLevel(SC4DrawContext*)"},
    {0x100746a50u, 132u, "cSTETerrainView3D::ComputeDrawFrustumOptimizationInfo(SC4DrawContext*)"},
    {0x100746ad4u, 824u, "cSTETerrainView3D::ComputeDrawRectsForDrawFrustum(SC4DrawContext*)"},
    {0x100746e0cu, 754u, "cSTETerrainView3D::ComputeHeightBasedCullingInfoForDrawFrustum(SC4DrawContext*)"},
    {0x1007470feu, 60u, "cSTETerrainView3D::ComputeViewFrustumOptimizationInfo(SC4DrawContext*)"},
    {0x10074713au, 824u, "cSTETerrainView3D::ComputeDrawRectsForViewFrustum(SC4DrawContext*)"},
    {0x100747472u, 754u, "cSTETerrainView3D::ComputeHeightBasedCullingInfoForViewFrustum(SC4DrawContext*)"},
    {0x100747764u, 226u, "cSTETerrainView3D::ComputeMinimumVisibleHtAtDrawRows(SC4DrawContext*, float*)"},
    {0x100747846u, 226u, "cSTETerrainView3D::ComputeMinimumVisibleHtAtViewRows(SC4DrawContext*, float*)"},
    {0x100747928u, 344u, "cSTETerrainView3D::ComputeTextureSpansForRow(int)"},
    {0x100747a80u, 1644u, "cSTETerrainView3D::CreateTerrainSizeSpecificDataStructures()"},
    {0x1007480ecu, 210u, "cSTETerrainView3D::InitTerrainSurfaceItems()"},
    {0x1007481beu, 198u, "cSTETerrainView3D::InitWaterSurfaceItems()"},
    {0x100748284u, 302u, "cSTETerrainView3D::cSTETerrainView3D()"},
    {0x1007484deu, 856u, "cSTETerrainView3D::Shutdown()"},
    {0x10074888au, 682u, "cSTETerrainView3D::DeleteTerrainSizeSpecificDataStructures()"},
    {0x100748b34u, 48u, "cSTETerrainView3D::DoLighting()"},
    {0x100748b64u, 552u, "cSTETerrainView3D::LightBorderCliffs()"},
    {0x100748d8cu, 318u, "cSTETerrainView3D::DoMessage(cIGZMessage2*)"},
    {0x100748ecau, 354u, "cSTETerrainView3D::ObtainHeightFieldInfo(SC4Rect<int> const&)"},
    {0x10074902cu, 502u, "cSTETerrainView3D::ObtainWaterSurfaceInfo(SC4Rect<int> const&)"},
    {0x100749234u, 1104u, "cSTETerrainView3D::DrawGrid(SC4DrawContext*, cSTEVertBufExtn*)"},
    {0x100749aceu, 1138u, "cSTETerrainView3D::DrawTerrainMeshSubsetWithVertBufExtensionInDrawFrustum(SC4DrawContext*, cSTEVertBufExtn*, cRZRect)"},
    {0x100749f40u, 442u, "cSTETerrainView3D::DrawTerrainMeshSubsetInDrawFrustum(cRZRect, SC4DrawContext*)"},
    {0x10074a0fau, 1140u, "cSTETerrainView3D::DrawSideCliffs(SC4DrawContext*)"},
    {0x10074a56eu, 446u, "cSTETerrainView3D::DrawTerrainMeshSubsetInViewFrustum(cRZRect, SC4DrawContext*)"},
    {0x10074a72cu, 1138u, "cSTETerrainView3D::DrawTerrainMeshSubsetWithVertBufExtensionInViewFrustum(SC4DrawContext*, cSTEVertBufExtn*, cRZRect)"},
    {0x10074ab9eu, 556u, "cSTETerrainView3D::DrawMixedMeshSubsetInDrawFrustum(cRZRect const&, SC4DrawContext*)"},
    {0x10074adcau, 556u, "cSTETerrainView3D::DrawMixedMeshSubsetInViewFrustum(cRZRect const&, SC4DrawContext*)"},
    {0x10074aff6u, 546u, "cSTETerrainView3D::DrawTerrainSurface(SC4DrawContext*, cSTEVertBufExtn*)"},
    {0x10074b218u, 138u, "cSTETerrainView3D::DrawTextureLessTerrainSurfaceWithVertBufExtension(cSTEVertBufExtn*)"},
    {0x10074b2a2u, 206u, "cSTETerrainView3D::DrawTextureLessTerrainSurface(SC4DrawContext*)"},
    {0x10074b370u, 334u, "cSTETerrainView3D::DrawTexturedTerrainSurface(SC4DrawContext*, cSTEVertBufExtn*)"},
    {0x10074b4beu, 922u, "cSTETerrainView3D::DrawTerrainSurfaceRows(SC4DrawContext*, std::__1::vector<int, std::__1::allocator<int>>&)"},
    {0x10074b858u, 406u, "cSTETerrainView3D::GetNextTextureToDraw(std::__1::vector<int, std::__1::allocator<int>>&, int&, std::__1::set<unsigned int, std::__1::less<unsigned int>, std::__1::allocator<unsigned int>>&, unsigned int&, std::__1::vector<cSTETerrainView3D::tRenderTexInfo, std::__1::allocator<cSTETerrainView3D::tRenderTexInfo>>&)"},
    {0x10074b9eeu, 800u, "cSTETerrainView3D::DrawTerrainSurfaceRowsWithVertBufExtension(SC4DrawContext*, cSTEVertBufExtn*, std::__1::vector<int, std::__1::allocator<int>>&)"},
    {0x10074bd0eu, 1026u, "cSTETerrainView3D::DrawWaterBox(SC4DrawContext*)"},
    {0x10074c110u, 496u, "cSTETerrainView3D::DrawWaterSurface(SC4DrawContext*, cSTEVertBufExtn*)"},
    {0x10074c300u, 326u, "cSTETerrainView3D::DrawWaterSurfaceSubsetInDrawFrustum(cRZRect, SC4DrawContext*)"},
    {0x10074c446u, 326u, "cSTETerrainView3D::DrawWaterSurfaceSubsetInViewFrustum(cRZRect, SC4DrawContext*)"},
    {0x10074c58cu, 1636u, "cSTETerrainView3D::DrawPlumbingInfo(SC4DrawContext*)"},
    {0x10074cbf0u, 428u, "cSTETerrainView3D::ReadPlumbingTuningData(cS3DColorARGB8*, bool&)"},
    {0x10074cd9cu, 92u, "cSTETerrainView3D::GetBuffer(unsigned int, cGZBufferColorType::eColorType)"},
    {0x10074ce30u, 1608u, "cSTETerrainView3D::Init(cSTETerrain*)"},
    {0x10074d478u, 670u, "cSTETerrainView3D::StaticInit()"},
    {0x10074d716u, 190u, "cSTETerrainView3D::SetTerrainSize(unsigned int, unsigned int)"},
    {0x10074d7d4u, 54u, "cSTETerrainView3D::MarkTextureSpanInfoDirty(int, int)"},
    {0x10074d80au, 104u, "cSTETerrainView3D::SetTriangleIndicesForAllRows()"},
    {0x10074d9aeu, 206u, "cSTETerrainView3D::UpdateBorderHeight(int, int, float, float)"},
    {0x10074da7cu, 18u, "cSTETerrainView3D::Pick(cS3DVector3 const&, cS3DVector3 const&, SC4DrawContext*, float*)"},
    {0x10074da8eu, 1458u, "cSTETerrainView3D::Pick(cS3DVector3 const&, cS3DVector3 const&, SC4DrawContext*, float*, bool)"},
    {0x10074e18cu, 456u, "cSTETerrainView3D::ReadTuningParameters()"},
    {0x10074e354u, 268u, "cSTETerrainView3D::InitializeCityEdgeLayerHeights()"},
    {0x10074e4beu, 218u, "cSTETerrainView3D::DrawTerrainQuad(SC4DrawContext*, unsigned int, unsigned int)"},
    {0x10074e5a6u, 538u, "cSTETerrainView3D::DrawTexturedTerrainQuad(SC4DrawContext*, unsigned int, unsigned int, cS3DVector2*, float, bool)"},
    {0x10074e7d2u, 478u, "cSTETerrainView3D::LevelCell(int, int, float)"},
    {0x10074f0a8u, 458u, "cSTETerrainView3D::UpdateLevelCellsFromTerrain(int, int, int)"},
    {0x10074f272u, 260u, "cSTETerrainView3D::UpdateLevelCellLighting(int, int, int)"},
    {0x10074f376u, 354u, "cSTETerrainView3D::SetTriangleIndicesForCell(int, int)"},
    {0x10074f4e6u, 444u, "cSTETerrainView3D::RemoveCellLevelling(int, int)"},
    {0x10074f6c8u, 86u, "cSTETerrainView3D::SetDisplayGridFlag(bool)"},
    {0x10074f730u, 648u, "cSTETerrainView3D::HandleCheatCommand(cRZCmdLine&)"},
    {0x10074f9c6u, 1134u, "cSTETerrainView3D::SetColorOfWaterSurface()"},
    {0x10074fe34u, 118u, "cSTETerrainView3D::SetLightingUpdateRect(SC4Rect<int> const*)"},
    {0x10074fef8u, 476u, "cSTETerrainView3D::StaticShutdown(void*)"},
    {0x10075014cu, 334u, "cSTETerrainView3D::Update(SC4DrawContext*)"},
    {0x10075029au, 216u, "cSTETerrainView3D::DrawTerrainLayers(SC4DrawContext*, cSTEVertBufExtn*)"},
    {0x100750372u, 108u, "cSTETerrainView3D::DrawWaterLayers(SC4DrawContext*, cSTEVertBufExtn*)"},
    // cSC4ViewInputControlPlaceLot (24)
    {0x1003bb6bau, 642u, "cSC4ViewInputControlPlaceLot::cSC4ViewInputControlPlaceLot(int, unsigned int, unsigned int, bool)"},
    {0x1003bbbbau, 1206u, "cSC4ViewInputControlPlaceLot::Init()"},
    {0x1003bc070u, 1170u, "cSC4ViewInputControlPlaceLot::DoUserLotConfigurationTypePick(unsigned int)"},
    {0x1003bc502u, 1194u, "cSC4ViewInputControlPlaceLot::DoUserBuildingTypePick(unsigned int)"},
    {0x1003bc9acu, 3822u, "cSC4ViewInputControlPlaceLot::GetBuildingPreviewModel()"},
    {0x1003bd89au, 398u, "cSC4ViewInputControlPlaceLot::Shutdown()"},
    {0x1003bda28u, 712u, "cSC4ViewInputControlPlaceLot::ReleaseLotConfigurationEffects()"},
    {0x1003bdd92u, 1630u, "cSC4ViewInputControlPlaceLot::SetPropOccupantDrawAttributes(cS3DVector3 const&, SC4DrawAttributes&, bool)"},
    {0x1003be3f0u, 284u, "cSC4ViewInputControlPlaceLot::OnKeyDown(unsigned int, unsigned int)"},
    {0x1003be50cu, 2072u, "cSC4ViewInputControlPlaceLot::SetVisualUserFeedback(bool, bool, cS3DVector3 const&)"},
    {0x1003bed24u, 468u, "cSC4ViewInputControlPlaceLot::OnMouseDownL(int, int, unsigned int)"},
    {0x1003beef8u, 3926u, "cSC4ViewInputControlPlaceLot::PlaceLot(cS3DVector3 const&, int, bool, SC4Rect<int>&)"},
    {0x1003bfe4eu, 594u, "cSC4ViewInputControlPlaceLot::CanDemolishLot(cISC4LotConfiguration*, cS3DVector3 const&, int, SC4Rect<int> const&, long long*)"},
    {0x1003c00a0u, 482u, "cSC4ViewInputControlPlaceLot::GetAutomaticFacingAndCellCoverage(cISC4LotConfiguration*, cS3DVector3&, int&, SC4Rect<int>&)"},
    {0x1003c0282u, 446u, "cSC4ViewInputControlPlaceLot::GetLotCenterAndCellCoverage(int, float, float, float&, float&, SC4Rect<int>&)"},
    {0x1003c0440u, 396u, "cSC4ViewInputControlPlaceLot::GetLotFacingStreetCount(SC4Rect<int> const&, cISC4LotConfiguration*, cS3DVector3 const&, int)"},
    {0x1003c05ccu, 640u, "cSC4ViewInputControlPlaceLot::CanPlaceLot(cS3DVector3 const&, int, SC4Rect<int>&, bool&, long long&, int&, cRZString&)"},
    {0x1003c084cu, 1064u, "cSC4ViewInputControlPlaceLot::PickTollBoothLotAndOrientation(unsigned int&, int&, cS3DVector3&, SC4Rect<int>&)"},
    {0x1003c0c74u, 48u, "cSC4ViewInputControlPlaceLot::ChangeLotConfiguration(unsigned int)"},
    {0x1003c0ca4u, 1304u, "cSC4ViewInputControlPlaceLot::OnMouseMove(int, int, unsigned int)"},
    {0x1003c11bcu, 100u, "cSC4ViewInputControlPlaceLot::OnMouseExit()"},
    {0x1003c1220u, 1840u, "cSC4ViewInputControlPlaceLot::SetConstraintFeedback(cS3DVector3 const&)"},
    {0x1003c1998u, 356u, "cSC4ViewInputControlPlaceLot::DoMessage(cIGZMessage2*)"},
    {0x1003c1b0eu, 606u, "cSC4ViewInputControlPlaceLot::SeekCompatibleNetworks(SC4Rect<int>&, int, int)"},
    // cSC4UndergroundNetworkTool (21)
    {0x1001a71e6u, 50u, "cSC4UndergroundNetworkTool::CreateNetworkOccupant(cSC4NetworkCellInfo*, cISC4NetworkOccupant**, float (*) [3], unsigned int)"},
    {0x1001a7218u, 330u, "cSC4UndergroundNetworkTool::InsertOccupantsForSolvedCells(std::__1::vector<cSC4NetworkTool::tSolvedCell, std::__1::allocator<cSC4NetworkTool::tSolvedCell>>&, std::__1::list<cISC4NetworkOccupant*, std::__1::allocator<cISC4NetworkOccupant*>>*, bool)"},
    {0x1001a7362u, 358u, "cSC4UndergroundNetworkTool::CreateNewUndergroundOccupant(cSC4NetworkCellInfo*, cSC4NetworkTool::tSolvedCell const&, cSC4UndergroundOccupant*&, bool&)"},
    {0x1001a74c8u, 132u, "cSC4UndergroundNetworkTool::cSC4UndergroundNetworkTool(cISC4NetworkManager::eNetworkType)"},
    {0x1001a754cu, 30u, "cSC4UndergroundNetworkTool::Init()"},
    {0x1001a756au, 172u, "cSC4UndergroundNetworkTool::Shutdown()"},
    {0x1001a7616u, 388u, "cSC4UndergroundNetworkTool::InsertRangeConstraintsForCell(unsigned int)"},
    {0x1001a779au, 334u, "cSC4UndergroundNetworkTool::InsertNetworkSpecificConstraintsForCell(unsigned int)"},
    {0x1001a78eeu, 834u, "cSC4UndergroundNetworkTool::CreateCellInfosForCurrentPlacementTiles(cISC4NetworkManager::eNetworkType)"},
    {0x1001a7c30u, 78u, "cSC4UndergroundNetworkTool::GetNetworkVertexHt(int) const"},
    {0x1001a7c7eu, 192u, "cSC4UndergroundNetworkTool::ReloadNetworkSpecificInfo(unsigned int)"},
    {0x1001a7d3eu, 280u, "cSC4UndergroundNetworkTool::FindUndergroundOccupant(int, int, cSC4UndergroundOccupant*&, unsigned int)"},
    {0x1001a7e56u, 662u, "cSC4UndergroundNetworkTool::AddCellPreview(cSC4NetworkCellInfo&, cSC4NetworkTool::tSolvedCell const&)"},
    {0x1001a80ecu, 234u, "cSC4UndergroundNetworkTool::ClearCellPreviews()"},
    {0x1001a81d6u, 314u, "cSC4UndergroundNetworkTool::FindUndergroundConnectionOccupant(int, int, unsigned int, cSC4UndergroundConnectionOccupant*&)"},
    {0x1001a8310u, 496u, "cSC4UndergroundNetworkTool::StartPlacing()"},
    {0x1001a8500u, 226u, "cSC4UndergroundNetworkTool::SetQuadTrianglesToDraw(cISC4NetworkOccupant*, cSC4TileNeighborhoodCompact const&)"},
    {0x1001a85e2u, 2906u, "cSC4UndergroundNetworkTool::UpdateUndergroundOccupantTerrainHeights(std::__1::vector<SC4Point<unsigned int>, std::__1::allocator<SC4Point<unsigned int>>>, std::__1::list<cSC4NetworkOccupant*, std::__1::allocator<cSC4NetworkOccupant*>>&, cISC4NetworkManager::eNetworkType)"},
    {0x1001a913cu, 419u, "cSC4UndergroundNetworkTool::PropagateFromCell(cSC4NetworkCellInfo*, int)"},
    {0x1001a932au, 124u, "cSC4UndergroundNetworkTool::PropagateSolvedCellsForSmoothing()"},
    {0x1001a93a6u, 135u, "cSC4UndergroundNetworkTool::TestForAutoConnectOccupant(int, int)"},
    // cSC4DBSegmentOStream (20)
    {0x1003ce804u, 444u, "cSC4DBSegmentOStream::Open(cISC4DBSegment*, cGZPersistResourceKey const&, bool)"},
    {0x1003ce9d2u, 154u, "cSC4DBSegmentOStream::Close()"},
    {0x1003cead6u, 58u, "cSC4DBSegmentOStream::WriteGZSerializable(cIGZSerializable const*)"},
    {0x1003ceb4au, 42u, "cSC4DBSegmentOStream::WriteResKey(cGZPersistResourceKey const&)"},
    {0x1003ceb9eu, 174u, "cSC4DBSegmentOStream::WriteVariant(cIGZVariant const&)"},
    {0x1003cecd4u, 46u, "cSC4DBSegmentOStream::SetSint8(signed char)"},
    {0x1003ced30u, 48u, "cSC4DBSegmentOStream::SetUint8(unsigned char)"},
    {0x1003ced90u, 46u, "cSC4DBSegmentOStream::SetSint16(short)"},
    {0x1003cedecu, 46u, "cSC4DBSegmentOStream::SetUint16(unsigned short)"},
    {0x1003cee48u, 42u, "cSC4DBSegmentOStream::SetSint32(int)"},
    {0x1003cee9cu, 42u, "cSC4DBSegmentOStream::SetUint32(unsigned int)"},
    {0x1003ceef0u, 42u, "cSC4DBSegmentOStream::SetSint64(long long)"},
    {0x1003cef44u, 42u, "cSC4DBSegmentOStream::SetUint64(unsigned long long)"},
    {0x1003cef98u, 42u, "cSC4DBSegmentOStream::SetFloat32(float)"},
    {0x1003cefecu, 42u, "cSC4DBSegmentOStream::SetFloat64(double)"},
    {0x1003cf040u, 250u, "cSC4DBSegmentOStream::SetRZCharStr(char const*)"},
    {0x1003cf148u, 42u, "cSC4DBSegmentOStream::SetGZStr(cIGZString const&)"},
    {0x1003cf19cu, 46u, "cSC4DBSegmentOStream::SetGZSerializable(cIGZSerializable const&)"},
    {0x1003cf1f8u, 42u, "cSC4DBSegmentOStream::SetVoid(void const*, unsigned int)"},
    {0x1003cf268u, 78u, "cSC4DBSegmentOStream::SetUserData(cIGZVariant*)"},
    // cSC4WinLotConfigurationChooser (14)
    {0x1004160e0u, 132u, "cSC4WinLotConfigurationChooser::cSC4WinLotConfigurationChooser()"},
    {0x10041627cu, 170u, "cSC4WinLotConfigurationChooser::Init()"},
    {0x100416326u, 480u, "cSC4WinLotConfigurationChooser::PostInit()"},
    {0x100416506u, 562u, "cSC4WinLotConfigurationChooser::SetupLotConfigurationList()"},
    {0x100416738u, 482u, "cSC4WinLotConfigurationChooser::Shutdown()"},
    {0x10041691au, 370u, "cSC4WinLotConfigurationChooser::DoMessage(cGZMessage&)"},
    {0x100416a8cu, 94u, "cSC4WinLotConfigurationChooser::DoRequestClose()"},
    {0x100416aeau, 982u, "cSC4WinLotConfigurationChooser::DoRequestEditLotConfiguration(bool)"},
    {0x100416ec0u, 1446u, "cSC4WinLotConfigurationChooser::DoRequestDeleteLotConfiguration()"},
    {0x100417466u, 180u, "cSC4WinLotConfigurationChooser::GetSelectedLotConfigurationType()"},
    {0x10041751au, 144u, "cSC4WinLotConfigurationChooser::IsSelectedLotBeingEdited()"},
    {0x1004175aau, 88u, "cSC4WinLotConfigurationChooser::GetSelectedLotConfigurationKey(cGZPersistResourceKey&)"},
    {0x100417710u, 250u, "cSC4WinLotConfigurationChooser::RemoveLotConfigurationTypeFromGrid(unsigned int)"},
    {0x10041780au, 118u, "cSC4WinLotConfigurationChooser::DoLotConfigurationEditorClosed(cSC4WinLotConfigurationEditor*)"},
    // cSC4ConfigRuleManager (13)
    {0x10076d2cau, 522u, "cSC4ConfigRuleManager::cSC4ConfigRuleManager()"},
    {0x10076d71au, 112u, "cSC4ConfigRuleManager::Init(cISC4RenderProperties*)"},
    {0x10076d78au, 1742u, "cSC4ConfigRuleManager::GatherInformation()"},
    {0x10076de58u, 1018u, "cSC4ConfigRuleManager::ParseConfigFile()"},
    {0x10076e252u, 1542u, "cSC4ConfigRuleManager::SetFallbackRules()"},
    {0x10076e858u, 630u, "cSC4ConfigRuleManager::SetOptionsFromConfiguration()"},
    {0x10076eaceu, 1666u, "cSC4ConfigRuleManager::LogConfigInfo()"},
    {0x10076f150u, 754u, "cSC4ConfigRuleManager::ApplyRules()"},
    {0x10076f442u, 2452u, "cSC4ConfigRuleManager::LogOptions()"},
    {0x10076fdd6u, 104u, "cSC4ConfigRuleManager::LogConfigurationToFile(char const*)"},
    {0x10076fe3eu, 94u, "cSC4ConfigRuleManager::ApplyOptionGroupSetting(tSC4VideoOptionGroup, tSC4VideoSetting)"},
    {0x10076ff2au, 372u, "cSC4ConfigRuleManager::RuleAppliesMatchAny(cSC4ConfigRule const&)"},
    {0x10077009eu, 364u, "cSC4ConfigRuleManager::RuleAppliesMatchAll(cSC4ConfigRule const&)"},
    // cSC4AdvisorUIImageWinProc (12)
    {0x1002dcbd0u, 316u, "cSC4AdvisorUIImageWinProc::cSC4AdvisorUIImageWinProc(int)"},
    {0x1002dcf1eu, 510u, "cSC4AdvisorUIImageWinProc::Init(cIGZWin*)"},
    {0x1002dd11cu, 218u, "cSC4AdvisorUIImageWinProc::Shutdown(cIGZWin*)"},
    {0x1002dd1f6u, 120u, "cSC4AdvisorUIImageWinProc::Enable(bool)"},
    {0x1002dd26eu, 908u, "cSC4AdvisorUIImageWinProc::LoadModels(bool)"},
    {0x1002dd5fau, 120u, "cSC4AdvisorUIImageWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x1002dd672u, 62u, "cSC4AdvisorUIImageWinProc::DoMessage(cIGZMessage2*)"},
    {0x1002dd6f2u, 222u, "cSC4AdvisorUIImageWinProc::DoAnimationTick(float)"},
    {0x1002dd7d0u, 200u, "cSC4AdvisorUIImageWinProc::PlaceHead(float, float, float, float)"},
    {0x1002dd8a4u, 14u, "cSC4AdvisorUIImageWinProc::SetMood(int)"},
    {0x1002dd8b2u, 58u, "cSC4AdvisorUIImageWinProc::ForceMood(int)"},
    {0x1002dd8ecu, 404u, "cSC4AdvisorUIImageWinProc::DoPaint()"},
    // cSC4EffectMaps (12)
    {0x1005fbef0u, 188u, "cSC4EffectMaps::cSC4EffectMaps()"},
    {0x1005fc0acu, 278u, "cSC4EffectMaps::Init()"},
    {0x1005fc1c2u, 96u, "cSC4EffectMaps::Shutdown()"},
    {0x1005fc26au, 174u, "cSC4EffectMaps::DrawTerrainMaps(SC4DrawContext*)"},
    {0x1005fc318u, 1066u, "cSC4EffectMaps::UpdateMapsMesh()"},
    {0x1005fc742u, 332u, "cSC4EffectMaps::MapLerpedHeight(unsigned int, float, float) const"},
    {0x1005fc88eu, 98u, "cSC4EffectMaps::MapVertexHeight(unsigned int, unsigned int, unsigned int) const"},
    {0x1005fc8f0u, 812u, "cSC4EffectMaps::MapLerpedNormal(unsigned int, float, float) const"},
    {0x1005fcc1cu, 262u, "cSC4EffectMaps::AddRectToMap(unsigned int, SC4Rect<int> const&, float, unsigned int)"},
    {0x1005fcd22u, 314u, "cSC4EffectMaps::SetValueAboveTerrain(unsigned int, SC4Rect<int> const&, float, unsigned int)"},
    {0x1005fce5cu, 168u, "cSC4EffectMaps::ResetValueToTerrain(unsigned int, SC4Rect<int> const&)"},
    {0x1005fcf04u, 70u, "cSC4EffectMaps::ClearMaps()"},
    // cSC4MapUtilities (11)
    {0x1000d7098u, 506u, "cSC4MapUtilities::AddRadiusEffect(std::__1::vector<std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>, std::__1::allocator<std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>>>&, int, int, int, int, float, float, float)"},
    {0x1000d7292u, 514u, "cSC4MapUtilities::AddRadiusEffect(std::__1::vector<std::__1::vector<signed char, std::__1::allocator<signed char>>, std::__1::allocator<std::__1::vector<signed char, std::__1::allocator<signed char>>>>&, int, int, int, int, float, float, float)"},
    {0x1000d7494u, 532u, "cSC4MapUtilities::AddRadiusEffect(std::__1::vector<std::__1::vector<short, std::__1::allocator<short>>, std::__1::allocator<std::__1::vector<short, std::__1::allocator<short>>>>&, int, int, int, int, float, float, float)"},
    {0x1000d76a8u, 1104u, "cSC4MapUtilities::AddRadiusEffectWithCurves(cISC4SimGrid<unsigned char>*, cISC4SimGrid<float>*, std::__1::list<cSC4ResponseCurve, std::__1::allocator<cSC4ResponseCurve>> const&, float, float, float, std::__1::list<cSC4ResponseCurve, std::__1::allocator<cSC4ResponseCurve>> const&, float, float, int, cISC4SimGrid<unsigned char>*, cISC4SimGrid<unsigned short>*, float&)"},
    {0x1000d7af8u, 532u, "cSC4MapUtilities::AddDistanceEffect(std::__1::vector<std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>, std::__1::allocator<std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>>>&, int, int, std::__1::vector<std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>, std::__1::allocator<std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>>> const&, unsigned char, float, float, float)"},
    {0x1000d7e5cu, 300u, "cSC4MapUtilities::ExpandFastOnePass(std::__1::vector<std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>, std::__1::allocator<std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>>>&, int, int)"},
    {0x1000d7f88u, 452u, "cSC4MapUtilities::FloodFillDistanceSetup(cISC4SimGrid<unsigned char>*, cISC4SimGrid<unsigned char> const*, unsigned char)"},
    {0x1000d814cu, 360u, "cSC4MapUtilities::FloodFillDistance(cISC4SimGrid<unsigned char>*, cISC4SimGrid<unsigned char> const*, unsigned char)"},
    {0x1000d82b4u, 120u, "cSC4MapUtilities::FloodFillDistance(std::__1::vector<std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>, std::__1::allocator<std::__1::vector<unsigned char, std::__1::allocator<unsigned char>>>>&, unsigned char, unsigned char)"},
    {0x1000d832cu, 810u, "cSC4MapUtilities::FloodFill(int, int, int, int, int, int, int, int (*)(int, int, void*), void*, void (*)(int, int, int, void*), void*, bool (*)(int, int, int, void*), void*)"},
    {0x1000d8670u, 402u, "cSC4MapUtilities::GetTractsInRadius(float, float, float, float, unsigned int, unsigned int, std::__1::list<SC4Point<unsigned int>, std::__1::allocator<SC4Point<unsigned int>>>&)"},
    // cSC4DisasterManagerFire (11)
    {0x10044f24au, 58u, "cSC4DisasterManagerFire::GetFireInstance()"},
    {0x10044f2beu, 108u, "cSC4DisasterManagerFire::FireInstance()"},
    {0x10044f32au, 60u, "cSC4DisasterManagerFire::SetCause(unsigned int)"},
    {0x10044f374u, 54u, "cSC4DisasterManagerFire::GetCause()"},
    {0x10044f3b8u, 70u, "cSC4DisasterManagerFire::IgniteOccupant(cISC4Occupant*)"},
    {0x10044f40cu, 100u, "cSC4DisasterManagerFire::IgniteCell(unsigned int, unsigned int, bool, bool, unsigned int)"},
    {0x10044f486u, 84u, "cSC4DisasterManagerFire::ExtinguishCell(unsigned int, unsigned int, int)"},
    {0x10044f532u, 70u, "cSC4DisasterManagerFire::QuickBurnCell(unsigned int, unsigned int)"},
    {0x10044f586u, 90u, "cSC4DisasterManagerFire::IgniteCellRadius(unsigned int, unsigned int, unsigned int, unsigned int)"},
    {0x10044f5feu, 26u, "cSC4DisasterManagerFire::AttemptDisasterStart(int, int)"},
    {0x10044f63eu, 58u, "cSC4DisasterManagerFire::Init()"},
    // cSC4GlyphTextureManager (11)
    {0x10070cb30u, 172u, "cSC4GlyphTextureManager::cSC4GlyphTextureManager()"},
    {0x10070cd68u, 134u, "cSC4GlyphTextureManager::Init()"},
    {0x10070ce00u, 202u, "cSC4GlyphTextureManager::Shutdown()"},
    {0x10070cedcu, 754u, "cSC4GlyphTextureManager::DoMessage(cIGZMessage2*)"},
    {0x10070d1e0u, 252u, "cSC4GlyphTextureManager::OnTick(unsigned int)"},
    {0x10070d372u, 658u, "cSC4GlyphTextureManager::StringToGlyphTextureArray(SC4GlyphAndFontInfo&, cIGZString&, std::__1::vector<cRZAutoRefCount<cISC4GlyphTextureInfo>, std::__1::allocator<cRZAutoRefCount<cISC4GlyphTextureInfo>>>&)"},
    {0x10070d604u, 578u, "cSC4GlyphTextureManager::GetGlyphTexture(SC4GlyphAndFontInfo const&, cISC4GlyphTextureInfo**)"},
    {0x10070d8eau, 118u, "cSC4GlyphTextureManager::SignalUnused(cSC4GlyphTextureInfo*)"},
    {0x10070d960u, 328u, "cSC4GlyphTextureManager::GetCachedStringTexture(unsigned int, cIGZString&, cISC4CachedStringTexture**)"},
    {0x10070daa8u, 156u, "cSC4GlyphTextureManager::GetTextureSize(cIGZString&, cIGZFont*, int&, int&)"},
    {0x10070db44u, 268u, "cSC4GlyphTextureManager::GetBoundIdForTexture(cSC4CachedStringTexture*)"},
    // cSC4COMSerializer (10)
    {0x1003cfb70u, 226u, "cSC4COMSerializer::cSC4COMSerializer()"},
    {0x1003cfc52u, 132u, "cSC4COMSerializer::reset()"},
    {0x1003cfdd4u, 144u, "cSC4COMSerializer::Open(cISC4DBSegment*, int)"},
    {0x1003cfe72u, 54u, "cSC4COMSerializer::Close()"},
    {0x1003cff72u, 130u, "cSC4COMSerializer::WriteGZSerializable(cIGZOStream&, cIGZSerializable const*)"},
    {0x1003cfff4u, 976u, "cSC4COMSerializer::onSetGZSerializable(cIGZSerializable const*, unsigned long, unsigned int)"},
    {0x1003d03d2u, 242u, "cSC4COMSerializer::ReadGZSerializable(cIGZIStream&, cIGZSerializable**, bool)"},
    {0x1003d04c4u, 536u, "cSC4COMSerializer::onGetGZSerializable(cIGZSerializable**, unsigned long, unsigned int)"},
    {0x1003d06dcu, 592u, "cSC4COMSerializer::loadSingleObject(cIGZSerializable*, unsigned long, unsigned int)"},
    {0x1003d1930u, 310u, "cSC4COMSerializer::cClassInfo::cClassInfo(cSC4COMSerializer::cClassInfo const&)"},
    // cSC4PolygonClipper<sVF_V3F_C4UB_T2F> (10)
    {0x100715368u, 196u, "cSC4PolygonClipper<sVF_V3F_C4UB_T2F>::Allocate()"},
    {0x10071542cu, 160u, "cSC4PolygonClipper<sVF_V3F_C4UB_T2F>::AddPrimitives(cS3DPrimitiveData*, unsigned short const*, int)"},
    {0x1007154ccu, 174u, "cSC4PolygonClipper<sVF_V3F_C4UB_T2F>::MoveTo(std::__1::vector<unsigned char, cRZFastSTLAllocatorThreadSafe<unsigned char>>&, std::__1::vector<unsigned short, cRZFastSTLAllocatorThreadSafe<unsigned short>>&)"},
    {0x10071557au, 196u, "cSC4PolygonClipper<sVF_V3F_C4UB_T2F>::Free(cSC4PolygonClipper<sVF_V3F_C4UB_T2F>*)"},
    {0x1007176f2u, 146u, "cSC4PolygonClipper<sVF_V3F_C4UB_T2F>::StaticShutdown(void*)"},
    {0x100717884u, 722u, "cSC4PolygonClipper<sVF_V3F_C4UB_T2F>::AddPrimitive(unsigned short const*, int, int)"},
    {0x100717b56u, 898u, "cSC4PolygonClipper<sVF_V3F_C4UB_T2F>::ClipPolygonY(unsigned short const*, int const*, int)"},
    {0x100717ed8u, 2262u, "cSC4PolygonClipper<sVF_V3F_C4UB_T2F>::ClipPolygonXZ(unsigned short const*, int const*, int)"},
    {0x1007187aeu, 678u, "cSC4PolygonClipper<sVF_V3F_C4UB_T2F>::TranslateOriginalVertIndex(unsigned short)"},
    {0x100718a54u, 340u, "cSC4PolygonClipper<sVF_V3F_C4UB_T2F>::ComputeLineToRectIntersection(unsigned int, sVF_V3F_C4UB_T2F const&, sVF_V3F_C4UB_T2F const&)"},
    // cSC4PropOccupantTerrainDecal (9)
    {0x10015f92au, 214u, "cSC4PropOccupantTerrainDecal::cSC4PropOccupantTerrainDecal()"},
    {0x10015fb54u, 146u, "cSC4PropOccupantTerrainDecal::Init()"},
    {0x10015fc0au, 188u, "cSC4PropOccupantTerrainDecal::Shutdown()"},
    {0x1001600ccu, 330u, "cSC4PropOccupantTerrainDecal::DoMessage(cIGZMessage2*)"},
    {0x100160232u, 138u, "cSC4PropOccupantTerrainDecal::SetPosition(float const*)"},
    {0x1001602d8u, 104u, "cSC4PropOccupantTerrainDecal::SetDecalTexture(cGZPersistResourceKey const&, float)"},
    {0x100160370u, 130u, "cSC4PropOccupantTerrainDecal::SetOpacityFade(float, cISC4PropOccupantTerrainDecal::tCompletionAction, float, float)"},
    {0x1001603f2u, 102u, "cSC4PropOccupantTerrainDecal::SetDecalSize(float)"},
    {0x100160458u, 366u, "cSC4PropOccupantTerrainDecal::SetAllFlags(unsigned int)"},
    // cSC4CheatCodeEntryDialog (9)
    {0x1003ac35cu, 114u, "cSC4CheatCodeEntryDialog::cSC4CheatCodeEntryDialog()"},
    {0x1003ac54cu, 546u, "cSC4CheatCodeEntryDialog::Init()"},
    {0x1003ac76eu, 150u, "cSC4CheatCodeEntryDialog::Shutdown()"},
    {0x1003ac804u, 364u, "cSC4CheatCodeEntryDialog::GZOnKeyDown(unsigned int, unsigned int)"},
    {0x1003ac970u, 922u, "cSC4CheatCodeEntryDialog::DoPopupStringListMenu()"},
    {0x1003acd0au, 140u, "cSC4CheatCodeEntryDialog::PlaySoundA(int)"},
    {0x1003acd96u, 14u, "cSC4CheatCodeEntryDialog::GZOnMouseDownR(int, int, unsigned int)"},
    {0x1003acda4u, 68u, "cSC4CheatCodeEntryDialog::DoMessage(cIGZWin*, cGZMessage&)"},
    {0x1003acdfau, 1194u, "cSC4CheatCodeEntryDialog::DoMessage(cGZMessage&)"},
    // cSC4TrafficEffectManager (8)
    {0x100111eaau, 56u, "cSC4TrafficEffectManager::Shutdown()"},
    {0x100112e0eu, 360u, "cSC4TrafficEffectManager::Update()"},
    {0x100113ab4u, 84u, "cSC4TrafficEffectManager::GetPopulateFlags(int, int)"},
    {0x100113baau, 406u, "cSC4TrafficEffectManager::CreateAccident(cISC4Occupant*, int, unsigned int)"},
    {0x100113d40u, 298u, "cSC4TrafficEffectManager::CreateEffect(int, int, int, cISC4Occupant*)"},
    {0x100113e6au, 68u, "cSC4TrafficEffectManager::StopEffect(cSC4TrafficEffect*)"},
    {0x10011b568u, 102u, "cSC4TrafficEffectManager::ShutdownEffect(cSC4TrafficEffect&)"},
    {0x10011b768u, 72u, "cSC4TrafficEffectManager::cSC4TrafficEffectManager()"},
    // cSC4ViewInputControlCityScroll (8)
    {0x1002c13a2u, 66u, "cSC4ViewInputControlCityScroll::cSC4ViewInputControlCityScroll()"},
    {0x1002c13e4u, 86u, "cSC4ViewInputControlCityScroll::Init()"},
    {0x1002c143au, 142u, "cSC4ViewInputControlCityScroll::ReadTunableValues()"},
    {0x1002c14c8u, 142u, "cSC4ViewInputControlCityScroll::Shutdown()"},
    {0x1002c1556u, 96u, "cSC4ViewInputControlCityScroll::EndInput()"},
    {0x1002c15b6u, 18u, "cSC4ViewInputControlCityScroll::OnMouseUpR(int, int, unsigned int)"},
    {0x1002c15c8u, 872u, "cSC4ViewInputControlCityScroll::OnMouseMove(int, int, unsigned int)"},
    {0x1002c1930u, 572u, "cSC4ViewInputControlCityScroll::SetMouseScroll(int, int)"},
    // cSC4FireOccupant (8)
    {0x10045ebe0u, 188u, "cSC4FireOccupant::cSC4FireOccupant()"},
    {0x10045edb8u, 112u, "cSC4FireOccupant::Init()"},
    {0x10045ee3au, 310u, "cSC4FireOccupant::Shutdown()"},
    {0x10045efa4u, 538u, "cSC4FireOccupant::SetTarget(cISC4Occupant*)"},
    {0x10045f1beu, 108u, "cSC4FireOccupant::SetStage(unsigned int)"},
    {0x10045f22au, 1246u, "cSC4FireOccupant::CreateFlames()"},
    {0x1004600ecu, 374u, "cSC4FireOccupant::ViewUpdate()"},
    {0x100460660u, 68u, "cSC4FireOccupant::SetVisible(bool)"},
    // cSC4MySimInputControlChangeHouse (8)
    {0x1006b4f8cu, 68u, "cSC4MySimInputControlChangeHouse::cSC4MySimInputControlChangeHouse()"},
    {0x1006b5102u, 34u, "cSC4MySimInputControlChangeHouse::Init()"},
    {0x1006b5124u, 1366u, "cSC4MySimInputControlChangeHouse::Init(cSC4MySim*)"},
    {0x1006b567au, 356u, "cSC4MySimInputControlChangeHouse::Shutdown()"},
    {0x1006b57e6u, 2166u, "cSC4MySimInputControlChangeHouse::OnMouseMove(int, int, unsigned int)"},
    {0x1006b605cu, 110u, "cSC4MySimInputControlChangeHouse::OnMouseDownL(int, int, unsigned int)"},
    {0x1006b60cau, 26u, "cSC4MySimInputControlChangeHouse::OnKeyDown(unsigned int, unsigned int)"},
    {0x1006b60e4u, 72u, "cSC4MySimInputControlChangeHouse::OnMouseExit()"},
    // cSC4NetworkTypeInfo (7)
    {0x1001c70d8u, 188u, "cSC4NetworkTypeInfo::cSC4NetworkTypeInfo()"},
    {0x1001c7308u, 126u, "cSC4NetworkTypeInfo::Clear()"},
    {0x1001c7390u, 132u, "cSC4NetworkTypeInfo::LoadEvaluator(cGZPersistResourceKey const&, bool, char const*)"},
    {0x1001c7414u, 2116u, "cSC4NetworkTypeInfo::LoadBridgeTypes(cRZIniFile&, char const*, unsigned int, int)"},
    {0x1001c7c58u, 184u, "cSC4NetworkTypeInfo::LoadSupports(unsigned int)"},
    {0x1001c8060u, 279u, "cSC4NetworkTypeInfo::LoadTilePatternVerifiers(cRZIniFile&, char const*)"},
    {0x1001c854au, 1462u, "cSC4NetworkTypeInfo::LoadPlacementParams(unsigned int)"},
    // cSC4AudioEventHandlerDisasterFire (7)
    {0x1003dedeau, 60u, "cSC4AudioEventHandlerDisasterFire::Create()"},
    {0x1003ec79cu, 158u, "cSC4AudioEventHandlerDisasterFire::cSC4AudioEventHandlerDisasterFire()"},
    {0x1003ec91eu, 600u, "cSC4AudioEventHandlerDisasterFire::Init()"},
    {0x1003ecb84u, 202u, "cSC4AudioEventHandlerDisasterFire::Shutdown()"},
    {0x1003ecc60u, 458u, "cSC4AudioEventHandlerDisasterFire::DoMessage(cIGZMessage2*)"},
    {0x1003ece84u, 98u, "cSC4AudioEventHandlerDisasterFire::IsOccupantIncluded(cISC4Occupant*)"},
    {0x1003ecef0u, 90u, "cSC4AudioEventHandlerDisasterFire::IterateFireOccupantCallback(cISC4Occupant*)"},
    // cSC4DisasterInstanceEffect (7)
    {0x10044c3c6u, 262u, "cSC4DisasterInstanceEffect::Init()"},
    {0x10044c668u, 164u, "cSC4DisasterInstanceEffect::cSC4DisasterInstanceEffect(cISC4VisualEffect*, unsigned int)"},
    {0x10044c7b8u, 112u, "cSC4DisasterInstanceEffect::DoMessage(cIGZMessage2*)"},
    {0x10044c948u, 222u, "cSC4DisasterInstanceEffect::Shutdown()"},
    {0x10044ca38u, 86u, "cSC4DisasterInstanceEffect::SetLocation(float*)"},
    {0x10044ca8eu, 12u, "cSC4DisasterInstanceEffect::Stop()"},
    {0x10044caaau, 44u, "cSC4DisasterInstanceEffect::CancelAttract()"},
    // cSC4MySimBalloonAdvice (7)
    {0x1006aa0b4u, 216u, "cSC4MySimBalloonAdvice::cSC4MySimBalloonAdvice()"},
    {0x1006aa272u, 1434u, "cSC4MySimBalloonAdvice::Init(int)"},
    {0x1006aa8acu, 90u, "cSC4MySimBalloonAdvice::GetTrigger() const"},
    {0x1006aa964u, 30u, "cSC4MySimBalloonAdvice::GetIconBalloonCount()"},
    {0x1006aa99au, 68u, "cSC4MySimBalloonAdvice::GetIconBalloon(int)"},
    {0x1006aaa1cu, 108u, "cSC4MySimBalloonAdvice::GetIconBalloonID(int)"},
    {0x1006aaad0u, 78u, "cSC4MySimBalloonAdvice::SetStatusFlags(int, bool)"},
    // cSC4DisasterInstanceToxic (7)
    {0x1006fc5aeu, 142u, "cSC4DisasterInstanceToxic::GetToxicCountInCellRect(SC4Rect<int>&)"},
    {0x1006fc730u, 292u, "cSC4DisasterInstanceToxic::SpillToxicWaste(int, int, int)"},
    {0x1006fca74u, 168u, "cSC4DisasterInstanceToxic::Init()"},
    {0x1006fcb4cu, 146u, "cSC4DisasterInstanceToxic::cSC4DisasterInstanceToxic()"},
    {0x1006fccaau, 42u, "cSC4DisasterInstanceToxic::DoMessage(cIGZMessage2*)"},
    {0x1006fccd4u, 304u, "cSC4DisasterInstanceToxic::Update()"},
    {0x1006fcf00u, 148u, "cSC4DisasterInstanceToxic::Shutdown()"},
    // cSC4PropOccupantAutomataController (6)
    {0x1001624c0u, 198u, "cSC4PropOccupantAutomataController::cSC4PropOccupantAutomataController()"},
    {0x100162712u, 510u, "cSC4PropOccupantAutomataController::Init()"},
    {0x100162934u, 124u, "cSC4PropOccupantAutomataController::Shutdown()"},
    {0x100162ceau, 458u, "cSC4PropOccupantAutomataController::RestartController()"},
    {0x100162ed6u, 340u, "cSC4PropOccupantAutomataController::SetControllerTemplate(char const*)"},
    {0x10016302au, 50u, "cSC4PropOccupantAutomataController::ActivateController(bool)"},
    // cSC4SequenceEffect (6)
    {0x100572cd0u, 340u, "cSC4SequenceEffect::cSC4SequenceEffect(cSC4SequenceDescription*)"},
    {0x100572f4au, 60u, "cSC4SequenceEffect::Start(cISC4VisualEffect::tTransitionType)"},
    {0x100572f86u, 224u, "cSC4SequenceEffect::Stop(cISC4VisualEffect::tTransitionType)"},
    {0x100573086u, 204u, "cSC4SequenceEffect::SetTransforms(cS3DTransform const&, cS3DTransform const&)"},
    {0x100573162u, 582u, "cSC4SequenceEffect::ApplyEffect(float)"},
    {0x1005733a8u, 212u, "cSC4SequenceEffect::CullSoftStoppedEffects()"},
    // cSC4NetworkOccupantModelInstance (6)
    {0x100714a7cu, 112u, "cSC4NetworkOccupantModelInstance::cSC4NetworkOccupantModelInstance()"},
    {0x100714bb0u, 114u, "cSC4NetworkOccupantModelInstance::Init(cIS3DResModel*, unsigned int)"},
    {0x100714c22u, 1150u, "cSC4NetworkOccupantModelInstance::Clip(cS3DVector3 const*, cS3DVector3 const*, bool)"},
    {0x10071563eu, 30u, "cSC4NetworkOccupantModelInstance::Prelight()"},
    {0x10071565cu, 900u, "cSC4NetworkOccupantModelInstance::FlipAndLight(bool)"},
    {0x1007159e0u, 584u, "cSC4NetworkOccupantModelInstance::Deform(int, bool, cS3DVector3 const&, cS3DVector3 const&, cS3DVector3 const&, bool)"},
    // cSC4LandfillOccupant (5)
    {0x1000b2ea8u, 35u, "cSC4LandfillOccupant::operator delete(void*)"},
    {0x1000b2ee2u, 216u, "cSC4LandfillOccupant::cSC4LandfillOccupant(unsigned int, int)"},
    {0x1000b33b0u, 88u, "cSC4LandfillOccupant::SetPosition(float const*)"},
    {0x1000b3460u, 50u, "cSC4LandfillOccupant::GetBoundingBox(float*, float*)"},
    {0x1000b34beu, 128u, "cSC4LandfillOccupant::SetTileID(unsigned int)"},
    // cSC4OccupantGroupTemplate (5)
    {0x1001de014u, 46u, "cSC4OccupantGroupTemplate::GetAnimOffset(cRZString const&, unsigned int&) const"},
    {0x1001de05au, 248u, "cSC4OccupantGroupTemplate::GetGroupIDVariant()"},
    {0x1001de152u, 208u, "cSC4OccupantGroupTemplate::IsOccupantIncluded(cISC4Occupant*)"},
    {0x1001de222u, 120u, "cSC4OccupantGroupTemplate::GetKeyEffect(cISC4Automaton::ePlayerDriveCommand, tKeyEffect&) const"},
    {0x1001de4b0u, 308u, "cSC4OccupantGroupTemplate::cSC4OccupantGroupTemplate()"},
    // cSC4AudioEmanationModel3D (5)
    {0x1003f33c4u, 84u, "cSC4AudioEmanationModel3D::Init()"},
    {0x1003f3426u, 34u, "cSC4AudioEmanationModel3D::Shutdown()"},
    {0x1003f346au, 496u, "cSC4AudioEmanationModel3D::UpdateVolPan(cIGZSndRegisterSet*, cIGZSndRegisterSet*) const"},
    {0x1003f365au, 526u, "cSC4AudioEmanationModel3D::ConfigureChannel(cIGZSndRegisterSet*, cIGZSndRegisterSet*) const"},
    {0x1003f481au, 28u, "cSC4AudioEmanationModel3D::GetName(cIGZString&) const"},
    // cSC4DisasterManagerVolcano (5)
    {0x100456e88u, 124u, "cSC4DisasterManagerVolcano::AttemptDisasterStart()"},
    {0x100456f12u, 152u, "cSC4DisasterManagerVolcano::AttemptDisasterStart(int, int)"},
    {0x100456fb8u, 172u, "cSC4DisasterManagerVolcano::CreateVolcanoInstance(cISC4DisasterInstanceVolcano**)"},
    {0x100457152u, 18u, "cSC4DisasterManagerVolcano::OnStandardMessage(cIGZMessage2Standard*)"},
    {0x100457244u, 168u, "cSC4DisasterManagerVolcano::GetLavaHeightByCityCell(int, int)"},
    // cSC4BrushEffect (5)
    {0x100575cdeu, 180u, "cSC4BrushEffect::SetDescription(cSC4BrushDescription*)"},
    {0x100575fb0u, 44u, "cSC4BrushEffect::Stop(cISC4VisualEffect::tTransitionType)"},
    {0x100575fe6u, 120u, "cSC4BrushEffect::SetTransforms(cS3DTransform const&, cS3DTransform const&)"},
    {0x10057606eu, 250u, "cSC4BrushEffect::ApplyEffect(float)"},
    {0x100576168u, 210u, "cSC4BrushEffect::ApplyBrush(int)"},
    // cSC4FloraOccupant (4)
    {0x1000cb758u, 35u, "cSC4FloraOccupant::operator delete(void*)"},
    {0x1000cb786u, 146u, "cSC4FloraOccupant::cSC4FloraOccupant()"},
    {0x1000cbb58u, 184u, "cSC4FloraOccupant::GetBoundingBox(float*, float*)"},
    {0x1000cbc1eu, 104u, "cSC4FloraOccupant::SetStaticFloraData(cSC4StaticFloraData*)"},
    // cSC4RuleParser (4)
    {0x10056ebccu, 206u, "cSC4RuleParser::cSC4RuleParser()"},
    {0x10056ee04u, 236u, "cSC4RuleParser::ParseBegin()"},
    {0x10056eefcu, 66u, "cSC4RuleParser::ParseEnd()"},
    {0x10056ef3eu, 242u, "cSC4RuleParser::RegisterCommands()"},
    // cSC4AutoLoadRecord (3)
    {0x100042634u, 218u, "cSC4AutoLoadRecord::openRecord(cIGZPersistDBSegment*, unsigned int, unsigned int, unsigned int)"},
    {0x100042732u, 194u, "cSC4AutoLoadRecord::cSC4AutoLoadRecord(cIGZIStream&, unsigned int, unsigned int, unsigned int)"},
    {0x10004287au, 112u, "cSC4AutoLoadRecord::Close()"},
    // cSC4BusinessUnit (3)
    {0x10018357au, 120u, "cSC4BusinessUnit::cSC4BusinessUnit()"},
    {0x100183c2cu, 196u, "cSC4BusinessUnit::Add(cISC4Lot*)"},
    {0x100183cf0u, 138u, "cSC4BusinessUnit::Remove(cISC4Lot*)"},
    // cSC4EdgeConnectionStore (3)
    {0x10019f2d8u, 324u, "cSC4EdgeConnectionStore::SetEdgeTypeConfig(cISC4NetworkManager::eNetworkType, unsigned int)"},
    {0x1001a02e6u, 116u, "cSC4EdgeConnectionStore::operator=(cSC4EdgeConnectionStore const&)"},
    {0x1001a035au, 78u, "cSC4EdgeConnectionStore::GetRaisedEdgeTypeConfig() const"},
    // cSC4ComplexViewOccupantFilter (3)
    {0x1003511bcu, 502u, "cSC4ComplexViewOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    {0x1003513b2u, 122u, "cSC4ComplexViewOccupantFilter::SetForceIncludePipeOccupants(bool)"},
    {0x10035142cu, 122u, "cSC4ComplexViewOccupantFilter::SetForceIncludeSubwayOccupants(bool)"},
    // cSC4DisasterManagerEffect (3)
    {0x10044c356u, 112u, "cSC4DisasterManagerEffect::StartDisasterEffect(cISC4VisualEffect*, cS3DVector3&, unsigned int)"},
    {0x10044c4d8u, 64u, "cSC4DisasterManagerEffect::AttractDisasterEffect(float*)"},
    {0x10044c560u, 58u, "cSC4DisasterManagerEffect::CancelAttract()"},
    // cSC4MayorLimo (3)
    {0x1006f3580u, 158u, "cSC4MayorLimo::cSC4MayorLimo()"},
    {0x1006f3628u, 180u, "cSC4MayorLimo::Init()"},
    {0x1006f371eu, 246u, "cSC4MayorLimo::TranslatePlayerDriveCommands(tAutomataStatePlayerDrive*, float)"},
    // cSC4DefaultOccupantFilter (2)
    {0x1000a5f54u, 206u, "cSC4DefaultOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    {0x1000a6022u, 96u, "cSC4DefaultOccupantFilter::IsPropertyHolderIncluded(cISCPropertyHolder*)"},
    // cPortLot (2)
    {0x1000cc3e4u, 106u, "cPortLot::GetLot()"},
    {0x1000ccf24u, 134u, "cPortLot::IsGrowthTriggered(cSC4PortDeveloper*)"},
    // cSC4FoundationOccupant (2)
    {0x10016d922u, 138u, "cSC4FoundationOccupant::cSC4FoundationOccupant()"},
    {0x10016de5au, 122u, "cSC4FoundationOccupant::GetBoundingBox(float*, float*)"},
    // cSC4RegionalTerrainView (2)
    {0x1002358c0u, 210u, "cSC4RegionalTerrainView::cSC4RegionalTerrainView()"},
    {0x100235a4cu, 300u, "cSC4RegionalTerrainView::Update(SC4DrawContext*)"},
    // cSC4CtabFilter (2)
    {0x1003762aau, 104u, "cSC4CtabFilter::cSC4CtabFilter(cIGZWin*)"},
    {0x1003763fcu, 52u, "cSC4CtabFilter::DoMessage(cIGZWin*, cGZMessage&)"},
    // cAmbienceOccupantFilterIsIndustrialDirty (2)
    {0x1003df1d2u, 50u, "cAmbienceOccupantFilterIsIndustrialDirty::Create()"},
    {0x1003e4ee8u, 338u, "cAmbienceOccupantFilterIsIndustrialDirty::IsTrueOccupant(cISC4Occupant*)"},
    // cAmbienceOccupantFilterIsLandfill (2)
    {0x1003df29au, 50u, "cAmbienceOccupantFilterIsLandfill::Create()"},
    {0x1003e5744u, 70u, "cAmbienceOccupantFilterIsLandfill::IsTrueOccupant(cISC4Occupant*)"},
    // cAmbienceOccupantFilterIsFarmVehicle (2)
    {0x1003df362u, 50u, "cAmbienceOccupantFilterIsFarmVehicle::Create()"},
    {0x1003e5336u, 76u, "cAmbienceOccupantFilterIsFarmVehicle::IsTrueOccupant(cISC4Occupant*)"},
    // cAmbienceOccupantFilterIsCheapVehicle (2)
    {0x1003df42au, 50u, "cAmbienceOccupantFilterIsCheapVehicle::Create()"},
    {0x1003e5466u, 76u, "cAmbienceOccupantFilterIsCheapVehicle::IsTrueOccupant(cISC4Occupant*)"},
    // cAmbienceOccupantFilterIsBus (2)
    {0x1003df4f2u, 50u, "cAmbienceOccupantFilterIsBus::Create()"},
    {0x1003e5596u, 102u, "cAmbienceOccupantFilterIsBus::IsTrueOccupant(cISC4Occupant*)"},
    // cAmbienceOccupantFilterIsResidential (2)
    {0x1003df5bau, 50u, "cAmbienceOccupantFilterIsResidential::Create()"},
    {0x1003e4c44u, 344u, "cAmbienceOccupantFilterIsResidential::IsTrueOccupant(cISC4Occupant*)"},
    // cAmbienceOccupantFilterIsAbandoned (2)
    {0x1003df682u, 50u, "cAmbienceOccupantFilterIsAbandoned::Create()"},
    {0x1003e4bc8u, 124u, "cAmbienceOccupantFilterIsAbandoned::IsTrueOccupant(cISC4Occupant*)"},
    // cAmbienceOccupantFilterIsHighWealth (2)
    {0x1003df74au, 50u, "cAmbienceOccupantFilterIsHighWealth::Create()"},
    {0x1003e5aeeu, 162u, "cAmbienceOccupantFilterIsHighWealth::IsTrueOccupant(cISC4Occupant*)"},
    // cSC4NetworkLot (2)
    {0x100409272u, 40u, "cSC4NetworkLot::GetLotConfigurationObjectCount()"},
    {0x10040929au, 134u, "cSC4NetworkLot::GetLotConfigurationObjectArray()"},
    // cSC4RewardAdvice (2)
    {0x10046ce4eu, 46u, "cSC4RewardAdvice::Init(int)"},
    {0x10046ce7cu, 1040u, "cSC4RewardAdvice::GetTrigger() const"},
    // cSC4GlyphTextureInfo (2)
    {0x10070dd5cu, 78u, "cSC4GlyphTextureInfo::SetManager(cSC4GlyphTextureManager*)"},
    {0x10070df66u, 24u, "cSC4GlyphTextureInfo::GetGlyph(cIGZString&) const"},
    // cSC4PropertySet (2)
    {0x10076cec8u, 118u, "cSC4PropertySet::cSC4PropertySet(cSC4PropertySet const&)"},
    {0x10076cf9eu, 130u, "cSC4PropertySet::Apply(cISC4RenderProperties*)"},
    // cSC4OccupantFilter (1)
    {0x1000a5edau, 40u, "cSC4OccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cGarbageHeap (1)
    {0x1000acb56u, 86u, "cGarbageHeap::DecayGarbage()"},
    // cSC4OccupantImmovabilityFilter (1)
    {0x1000eea44u, 120u, "cSC4OccupantImmovabilityFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // tAutomataStateCollide (1)
    {0x10010e822u, 98u, "tAutomataStateCollide::Shutdown(cISC4Automaton*)"},
    // cLotSizesFilter (1)
    {0x10013c4cau, 76u, "cLotSizesFilter::IsLotConfigurationIncluded(cISC4LotConfiguration*)"},
    // cSC4AutomataMarkedCellView (1)
    {0x1001ea082u, 190u, "cSC4AutomataMarkedCellView::ReleaseAutomaton(cSC4Automaton*)"},
    // tCrimePropOccupantFilter (1)
    {0x1001fff04u, 66u, "tCrimePropOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cSC4QueryMessageFilter (1)
    {0x1002db77eu, 240u, "cSC4QueryMessageFilter::DoMessage(cIGZWin*, cGZMessage&)"},
    // cSC4StoplightPropOccupantFilter (1)
    {0x1002dc206u, 82u, "cSC4StoplightPropOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cSC4AnimatableOccupantFilter (1)
    {0x100359d32u, 74u, "cSC4AnimatableOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cSC4PlaceLotOccupantFilter (1)
    {0x1003c1f5au, 40u, "cSC4PlaceLotOccupantFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cLCEBlockingLotOccupantFilter (1)
    {0x10042f056u, 52u, "cLCEBlockingLotOccupantFilter::IsOccupantTypeIncluded(unsigned int)"},
    // cSC4BuildingPropFilter (1)
    {0x100502548u, 48u, "cSC4BuildingPropFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cMySimPylonFilter (1)
    {0x1006b88f6u, 236u, "cMySimPylonFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cSC4SignOrLabelFilter (1)
    {0x1006ca0c4u, 152u, "cSC4SignOrLabelFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // cSC4OccupantFilterByNetworkFlags (1)
    {0x1006e6298u, 216u, "cSC4OccupantFilterByNetworkFlags::IsOccupantIncluded(cISC4Occupant*)"},
    // cSC4NetworkCellInfo (1)
    {0x10079d6acu, 54u, "cSC4NetworkCellInfo::cSC4NetworkCellInfo()"},
};

inline constexpr Uint32 kWave3Part3MethodCount =
    sizeof(kWave3Part3Methods) / sizeof(kWave3Part3Methods[0]);

static_assert(kWave3Part3MethodCount == 452u);
static_assert(kWave3Part3Methods[0].address == 0x1007460feu);
static_assert(kWave3Part3Methods[kWave3Part3MethodCount - 1u].address == 0x10079d6acu);

inline constexpr bool Wave3Part3MethodsWellFormed() noexcept {
    if (kWave3Part3MethodCount != 452u) {
        return false;
    }
    for (Uint32 i = 0; i < kWave3Part3MethodCount; ++i) {
        Wave3Part3Method const& method = kWave3Part3Methods[i];
        if (method.address < 0x100000000ull || method.address > 0x100ffffffull) {
            return false;
        }
        if (method.sizeBytes == 0u || method.name == nullptr || method.name[0] == '\0') {
            return false;
        }
    }
    return true;
}

static_assert(Wave3Part3MethodsWellFormed());


// Wave 4 Part 3 entry points: 86 classes, 808 methods (0x100xxxxxx).
struct Wave4Part3Method {
    Uint64 address;
    Uint32 sizeBytes;
    char const* name;
};

inline constexpr Wave4Part3Method kWave4Part3Methods[] = {
    // cSC4View3DWin (132)
    {0x10077efc2u, 1834u, "cSC4View3DWin::cSC4View3DWin()"},
    {0x10077fbe2u, 3374u, "cSC4View3DWin::Init()"},
    {0x100780910u, 116u, "cSC4View3DWin::CreateProgressScreen(bool)"},
    {0x100780992u, 998u, "cSC4View3DWin::Shutdown()"},
    {0x100780d78u, 306u, "cSC4View3DWin::AddWinAnimationRecorder(bool)"},
    {0x100780eaau, 1422u, "cSC4View3DWin::RemoveGUI()"},
    {0x100781556u, 58u, "cSC4View3DWin::GetViewManager()"},
    {0x100781590u, 506u, "cSC4View3DWin::GZPaint()"},
    {0x10078178au, 382u, "cSC4View3DWin::UpdateCostIndicator(float)"},
    {0x100781908u, 216u, "cSC4View3DWin::DoScrollEnd()"},
    {0x1007819e0u, 64u, "cSC4View3DWin::DoTemporarySimHiddenPause()"},
    {0x100781a20u, 64u, "cSC4View3DWin::DoTemporarySimHiddenResume()"},
    {0x100781ac0u, 70u, "cSC4View3DWin::SetKeyAccelerator(cIGZWinKeyAccelerator*)"},
    {0x100781b06u, 128u, "cSC4View3DWin::SetScrolling(bool, float, float)"},
    {0x100781b86u, 302u, "cSC4View3DWin::DoScrollBegin()"},
    {0x100781cb4u, 58u, "cSC4View3DWin::ScrollStop()"},
    {0x100781d04u, 278u, "cSC4View3DWin::RecalcScrollVelocity()"},
    {0x100781e1au, 74u, "cSC4View3DWin::SetZoom(int)"},
    {0x100781e64u, 142u, "cSC4View3DWin::SetZoomLimits(int, int)"},
    {0x100781ef2u, 38u, "cSC4View3DWin::CanZoomIn()"},
    {0x100781f18u, 38u, "cSC4View3DWin::CanZoomOut()"},
    {0x100781f3eu, 48u, "cSC4View3DWin::SetRotation(int)"},
    {0x100781f6eu, 48u, "cSC4View3DWin::RotateLeft()"},
    {0x100781f9eu, 48u, "cSC4View3DWin::RotateRight()"},
    {0x100781fceu, 70u, "cSC4View3DWin::ZoomIn()"},
    {0x100782014u, 70u, "cSC4View3DWin::ZoomOut()"},
    {0x10078205au, 74u, "cSC4View3DWin::SetZoomAndRotation(int, int)"},
    {0x1007820a4u, 22u, "cSC4View3DWin::GetCurrentViewFilterInfo(cSC4ViewFilterInformation&)"},
    {0x1007820bau, 374u, "cSC4View3DWin::SetCurrentViewFilterInfo(cSC4ViewFilterInformation&)"},
    {0x10078223eu, 286u, "cSC4View3DWin::SetCurrentViewLevelInfo(cSC4ViewLevelInformation&)"},
    {0x100782368u, 40u, "cSC4View3DWin::SetCurrentViewLevel(unsigned int)"},
    {0x100782390u, 30u, "cSC4View3DWin::GetCurrentViewInputControl()"},
    {0x1007823aeu, 424u, "cSC4View3DWin::SetCurrentViewInputControl(cISC4ViewInputControl*, cISC4View3DWin::ViewInputControlStackOperation)"},
    {0x100782556u, 266u, "cSC4View3DWin::SendViewInputControlChangeMessage(cISC4ViewInputControl*, cISC4ViewInputControl*)"},
    {0x100782660u, 304u, "cSC4View3DWin::RemoveCurrentViewInputControl(bool)"},
    {0x100782790u, 40u, "cSC4View3DWin::RemoveAllViewInputControls(bool)"},
    {0x1007827b8u, 134u, "cSC4View3DWin::AddPersistentViewInputControl(cISC4ViewInputControl*)"},
    {0x10078283eu, 136u, "cSC4View3DWin::RemovePersistentViewInputControl(cISC4ViewInputControl*)"},
    {0x1007828c6u, 102u, "cSC4View3DWin::RemoveAllPersistentViewInputControls()"},
    {0x10078292cu, 54u, "cSC4View3DWin::PickTerrain(int, int, float*, bool)"},
    {0x100782962u, 150u, "cSC4View3DWin::PickOccupant(int, int, cISC4Occupant*&)"},
    {0x1007829f8u, 380u, "cSC4View3DWin::GZOnCharacter(char)"},
    {0x100782ba2u, 382u, "cSC4View3DWin::ProcessKeyDown(unsigned int, unsigned int)"},
    {0x100782d20u, 382u, "cSC4View3DWin::ProcessKeyUp(unsigned int, unsigned int)"},
    {0x100782e9eu, 440u, "cSC4View3DWin::GZOnMouseDownL(int, int, unsigned int)"},
    {0x100783068u, 494u, "cSC4View3DWin::GZOnMouseDownR(int, int, unsigned int)"},
    {0x100783268u, 506u, "cSC4View3DWin::GZOnMouseUpL(int, int, unsigned int)"},
    {0x100783474u, 768u, "cSC4View3DWin::GZOnMouseUpR(int, int, unsigned int)"},
    {0x100783786u, 730u, "cSC4View3DWin::GZOnMouseMove(int, int, unsigned int)"},
    {0x100783a60u, 112u, "cSC4View3DWin::UpdateCursorTextPositions(int, int)"},
    {0x100783ae2u, 390u, "cSC4View3DWin::GZOnMouseWheel(int, int, unsigned int, int)"},
    {0x100783c7au, 232u, "cSC4View3DWin::GZOnMouseExit(cIGZWin*)"},
    {0x100783d86u, 164u, "cSC4View3DWin::InvokeZoningTool(unsigned int)"},
    {0x100783e2au, 612u, "cSC4View3DWin::InvokeDisaster(unsigned int)"},
    {0x10078408eu, 124u, "cSC4View3DWin::InvokeLandscapeTool(unsigned int, bool)"},
    {0x10078410au, 2226u, "cSC4View3DWin::InvokeCatalogItemCommand(unsigned int, unsigned int, bool)"},
    {0x1007849bcu, 744u, "cSC4View3DWin::RunGlobalTerrainTool(int)"},
    {0x100784ca4u, 972u, "cSC4View3DWin::DoMessage(cGZMessage&)"},
    {0x100785070u, 506u, "cSC4View3DWin::HandleAdviceLinkMessage(cGZMessage&)"},
    {0x10078526au, 1592u, "cSC4View3DWin::ProcessCheatCode(unsigned int, cRZCmdLine&)"},
    {0x1007858a2u, 430u, "cSC4View3DWin::Update24HourClockHover()"},
    {0x100785a5eu, 1806u, "cSC4View3DWin::DoMessage(cIGZMessage2*)"},
    {0x10078616cu, 1644u, "cSC4View3DWin::DoPreCityInit()"},
    {0x1007867d8u, 1894u, "cSC4View3DWin::DoPostCityInit()"},
    {0x100786f3eu, 128u, "cSC4View3DWin::DoCityEstablished()"},
    {0x100786fbeu, 76u, "cSC4View3DWin::DoCityObliterated()"},
    {0x10078700au, 136u, "cSC4View3DWin::UpdateCityName()"},
    {0x1007872e8u, 76u, "cSC4View3DWin::UpdateZoomButtons()"},
    {0x100787334u, 1378u, "cSC4View3DWin::AddBackgroundImageToRenderer()"},
    {0x100787896u, 390u, "cSC4View3DWin::UpdateSpeedButtons()"},
    {0x100787a1cu, 330u, "cSC4View3DWin::UpdateAlertBorder()"},
    {0x100787b66u, 684u, "cSC4View3DWin::UpdateMissionProgress(cIGZUnknown*, unsigned int)"},
    {0x100787e12u, 420u, "cSC4View3DWin::UpdateSimulationClock()"},
    {0x100787fb6u, 206u, "cSC4View3DWin::UpdatePopulation()"},
    {0x100788084u, 1848u, "cSC4View3DWin::UpdateMayorRatingBar()"},
    {0x1007887bcu, 342u, "cSC4View3DWin::UpdateBudget()"},
    {0x100788912u, 746u, "cSC4View3DWin::Update24HourClock(unsigned int, unsigned int)"},
    {0x100788bfcu, 176u, "cSC4View3DWin::UpdateAmbientLightLevel()"},
    {0x100788d0eu, 120u, "cSC4View3DWin::DoBudgetFailure()"},
    {0x100788d98u, 118u, "cSC4View3DWin::UpdateProgressMessage()"},
    {0x100788e0eu, 1262u, "cSC4View3DWin::ReadTunableValues()"},
    {0x1007892fcu, 5980u, "cSC4View3DWin::AddGUI()"},
    {0x10078aa58u, 134u, "cSC4View3DWin::UpdateDayNightButtons()"},
    {0x10078aadeu, 254u, "cSC4View3DWin::HidePanel(unsigned int)"},
    {0x10078ac02u, 358u, "cSC4View3DWin::ShowPanel(unsigned int)"},
    {0x10078ad68u, 424u, "cSC4View3DWin::ShowMayorSubPanel(unsigned int)"},
    {0x10078af10u, 206u, "cSC4View3DWin::DoMessage(cIGZWin*, cGZMessage&)"},
    {0x10078afdeu, 4862u, "cSC4View3DWin::HandleButtonActivated(cIGZWin*, cGZMessage&)"},
    {0x10078c2dcu, 934u, "cSC4View3DWin::DoPanelCommandGUI(unsigned int)"},
    {0x10078c690u, 720u, "cSC4View3DWin::DoLandscapeMenu(cIGZWin*, cGZMessage&)"},
    {0x10078c960u, 300u, "cSC4View3DWin::InvokeSecondaryMenu(unsigned int, unsigned int, cIGZWin*, unsigned int)"},
    {0x10078ca8cu, 152u, "cSC4View3DWin::DoDayNightMenu(unsigned int)"},
    {0x10078cb24u, 144u, "cSC4View3DWin::DoZoningMenu(cIGZWin*, cGZMessage&)"},
    {0x10078cbb4u, 1216u, "cSC4View3DWin::DoUtilitiesMenu(cIGZWin*, cGZMessage&)"},
    {0x10078d074u, 330u, "cSC4View3DWin::CreateBuildingMenu(unsigned int, std::__1::set<unsigned int, std::__1::less<unsigned int>, std::__1::allocator<unsigned int>> const&, std::__1::set<unsigned int, std::__1::less<unsigned int>, std::__1::allocator<unsigned int>> const&, unsigned int, int)"},
    {0x10078d1beu, 1444u, "cSC4View3DWin::InvokeTertiaryMenu(unsigned int, cIGZWin*, unsigned int, cISC4WinCatalogView*, unsigned int, cSC4View3DWin::CatalogState*)"},
    {0x10078d762u, 1102u, "cSC4View3DWin::InvokeSecondaryMenu(unsigned int, cIGZWin*, unsigned int, cISC4WinCatalogView*, unsigned int, cSC4View3DWin::CatalogState*)"},
    {0x10078dbb0u, 354u, "cSC4View3DWin::DoDisasterCopingMenu(cIGZWin*, cGZMessage&)"},
    {0x10078dd12u, 714u, "cSC4View3DWin::DoTransportMenu(cIGZWin*, cGZMessage&)"},
    {0x10078dfdcu, 300u, "cSC4View3DWin::InvokeTertiaryMenu(unsigned int, unsigned int, cIGZWin*, unsigned int, int)"},
    {0x10078e108u, 472u, "cSC4View3DWin::DoVehicleMenu(cIGZWin*, cGZMessage&)"},
    {0x10078e2e0u, 6944u, "cSC4View3DWin::ProcessCommand(unsigned int, cIGZCommandParameterSet&, cIGZCommandParameterSet&)"},
    {0x10078fe00u, 120u, "cSC4View3DWin::ToggleContourDisplay()"},
    {0x10078fe78u, 184u, "cSC4View3DWin::ObliterateCity()"},
    {0x10078ff30u, 1822u, "cSC4View3DWin::DoEstablishCityDialog()"},
    {0x10079064eu, 1106u, "cSC4View3DWin::SetRenderProperties(cIGZString const&, cIGZString const&)"},
    {0x100790aa0u, 170u, "cSC4View3DWin::DoLandscapeCatalogView(cIGZWin*, unsigned int, unsigned int, unsigned int, cSC4View3DWin::CatalogState*)"},
    {0x100790b4au, 78u, "cSC4View3DWin::ApplyDefaultTerrainBrush(int, bool)"},
    {0x100790b98u, 178u, "cSC4View3DWin::DoFloraCatalogView(cIGZWin*, unsigned int, unsigned int, unsigned int, cSC4View3DWin::CatalogState*)"},
    {0x100790c4au, 178u, "cSC4View3DWin::DoZoneCatalogView(cIGZWin*, unsigned int, unsigned int, unsigned int, cSC4View3DWin::CatalogState*)"},
    {0x100790cfcu, 342u, "cSC4View3DWin::DoEarnedVehicleCatalogView(cIGZWin*, unsigned int, cISC4AutomataManager*, cSC4View3DWin::CatalogState*)"},
    {0x100790e52u, 1242u, "cSC4View3DWin::AddAutomataToItemList(cISC4AutomataManager*, std::__1::multimap<int, cISC4CatalogItem*, std::__1::less<int>, std::__1::allocator<std::__1::pair<int const, cISC4CatalogItem*>>>&)"},
    {0x10079132cu, 562u, "cSC4View3DWin::SetCursorText(unsigned int, unsigned int, cIGZString const*, cIGZString const*, cISC4View3DWin::CursorTextIcon)"},
    {0x10079155eu, 284u, "cSC4View3DWin::CursorTextChanged()"},
    {0x100791772u, 62u, "cSC4View3DWin::ClearCursorText(unsigned int)"},
    {0x100791802u, 220u, "cSC4View3DWin::SetCostIndicator(unsigned int, unsigned int)"},
    {0x1007918deu, 806u, "cSC4View3DWin::RenderCostIndicator()"},
    {0x100791c04u, 86u, "cSC4View3DWin::SetCostIndicatorAlpha(unsigned char)"},
    {0x100791c5au, 106u, "cSC4View3DWin::ClearCostIndicator(bool)"},
    {0x100791cc4u, 84u, "cSC4View3DWin::SetCursorTextPosition3D(cS3DVector3 const&)"},
    {0x100791d18u, 110u, "cSC4View3DWin::CloseSecondaryMenu()"},
    {0x100791d86u, 2696u, "cSC4View3DWin::AddBuildingsToItemList(unsigned int, std::__1::set<unsigned int, std::__1::less<unsigned int>, std::__1::allocator<unsigned int>> const&, std::__1::set<unsigned int, std::__1::less<unsigned int>, std::__1::allocator<unsigned int>> const&, std::__1::multimap<int, cISC4CatalogItem*, std::__1::less<int>, std::__1::allocator<std::__1::pair<int const, cISC4CatalogItem*>>>&)"},
    {0x10079280eu, 204u, "cSC4View3DWin::CloseTertiaryMenu()"},
    {0x1007928dau, 154u, "cSC4View3DWin::ChangeTerrainType()"},
    {0x100792974u, 42u, "cSC4View3DWin::GetGlobalEffectTriggerState(unsigned int)"},
    {0x10079299eu, 206u, "cSC4View3DWin::AssertGlobalEffectTrigger(unsigned int)"},
    {0x100792a6cu, 206u, "cSC4View3DWin::RetractGlobalEffectTrigger(unsigned int)"},
    {0x100792b4eu, 308u, "cSC4View3DWin::MinimizeUI(bool)"},
    {0x100792c82u, 508u, "cSC4View3DWin::ShowMissionStatusPanel(unsigned int)"},
    {0x100792e7eu, 74u, "cSC4View3DWin::HideMissionStatusPanel()"},
    {0x100792ec8u, 498u, "cSC4View3DWin::UpdateMissionStatusPanel()"},
    {0x100793abeu, 20u, "cSC4View3DWin::SetCostIndicator(unsigned int)"},
    // cHitTrackPlayer (62)
    {0x100473b9au, 32u, "cHitTrackPlayer::SetSoundPriority(int)"},
    {0x100473c3eu, 32u, "cHitTrackPlayer::GetVolume() const"},
    {0x100473c5eu, 26u, "cHitTrackPlayer::SetPitch(int)"},
    {0x100473c78u, 22u, "cHitTrackPlayer::GetPan() const"},
    {0x100473c8eu, 26u, "cHitTrackPlayer::SetPan(int)"},
    {0x100473ca8u, 26u, "cHitTrackPlayer::SetEffectsType(int)"},
    {0x100473cc2u, 28u, "cHitTrackPlayer::SetEffectsLevel(int)"},
    {0x100473cdeu, 24u, "cHitTrackPlayer::GetFrequency() const"},
    {0x100473cf6u, 28u, "cHitTrackPlayer::GetName(cIGZString&) const"},
    {0x1004744eau, 832u, "cHitTrackPlayer::operator=(cHitTrackPlayer const&)"},
    {0x10047adf8u, 1150u, "cHitTrackPlayer::cHitTrackPlayer()"},
    {0x10047b982u, 94u, "cHitTrackPlayer::Shutdown()"},
    {0x10047b9f2u, 82u, "cHitTrackPlayer::IsPlaying() const"},
    {0x10047ba98u, 18u, "cHitTrackPlayer::Update(int)"},
    {0x10047bac0u, 672u, "cHitTrackPlayer::HandleTimerCallback(int)"},
    {0x10047bda2u, 10966u, "cHitTrackPlayer::DoCommand()"},
    {0x10047e878u, 24u, "cHitTrackPlayer::Play(int, bool, cIGZSnd::SndDup)"},
    {0x10047e8aeu, 646u, "cHitTrackPlayer::Play(int, int, int, int)"},
    {0x10047eb34u, 496u, "cHitTrackPlayer::StartSndDependent(unsigned int)"},
    {0x10047ed56u, 746u, "cHitTrackPlayer::SetTrackDef(cIGZPropertySetT<cKeyStandardTraitsT<unsigned int, 3954248925ul, 1806839612ul>>*)"},
    {0x10047f040u, 76u, "cHitTrackPlayer::SetHitList(cIHitList*)"},
    {0x10047f0aau, 258u, "cHitTrackPlayer::CompletionTime()"},
    {0x10047f1acu, 24u, "cHitTrackPlayer::Reset()"},
    {0x10047f1c4u, 926u, "cHitTrackPlayer::PlayPause()"},
    {0x10047f562u, 52u, "cHitTrackPlayer::UpdateVolPan()"},
    {0x10047f596u, 644u, "cHitTrackPlayer::UpdatePitch()"},
    {0x10047f81au, 172u, "cHitTrackPlayer::Pause(bool)"},
    {0x10047f8dcu, 190u, "cHitTrackPlayer::Unpause()"},
    {0x10047f9a8u, 340u, "cHitTrackPlayer::Stop()"},
    {0x10047fb0eu, 370u, "cHitTrackPlayer::Kill()"},
    {0x10047fc92u, 58u, "cHitTrackPlayer::CancelNote()"},
    {0x10047fcccu, 60u, "cHitTrackPlayer::Step()"},
    {0x10047fd08u, 158u, "cHitTrackPlayer::GetRegisterVal(int) const"},
    {0x10047fdb4u, 148u, "cHitTrackPlayer::GetRegisterFloat(unsigned int) const"},
    {0x10047fe56u, 84u, "cHitTrackPlayer::HasRegisterVal(int) const"},
    {0x10047feb8u, 538u, "cHitTrackPlayer::SetRegisterVal(int, int, bool)"},
    {0x1004800e4u, 524u, "cHitTrackPlayer::SetRegisterFloat(unsigned int, float, bool)"},
    {0x100480302u, 220u, "cHitTrackPlayer::ClearRegisterVal(int, bool)"},
    {0x1004803f0u, 1082u, "cHitTrackPlayer::HandleNotifySetPropertyValue(int)"},
    {0x10048082au, 530u, "cHitTrackPlayer::UpdatePatch()"},
    {0x100480a3cu, 138u, "cHitTrackPlayer::NoteOn(int)"},
    {0x100480ac6u, 18u, "cHitTrackPlayer::GetSourceFilenameAndLineNum(cRZString&, int&)"},
    {0x100480ad8u, 18u, "cHitTrackPlayer::GetLocalRegisterVal(int)"},
    {0x100480af2u, 212u, "cHitTrackPlayer::SetTrackDataEntryPoint(unsigned int)"},
    {0x100480bc6u, 74u, "cHitTrackPlayer::SetFrequency(int)"},
    {0x100480c60u, 44u, "cHitTrackPlayer::Load()"},
    {0x100480cb8u, 36u, "cHitTrackPlayer::Unload()"},
    {0x100480d00u, 56u, "cHitTrackPlayer::CurrentPlayPosition(unsigned char**)"},
    {0x100480d38u, 256u, "cHitTrackPlayer::AddSource(unsigned int)"},
    {0x100480e46u, 106u, "cHitTrackPlayer::RemoveSource(unsigned int)"},
    {0x100480ebeu, 76u, "cHitTrackPlayer::RemoveAllSources()"},
    {0x100480f58u, 48u, "cHitTrackPlayer::UpdateSources()"},
    {0x100480fbcu, 60u, "cHitTrackPlayer::SetCompletionNotification(cIGZSndCompletionFunctionObject*)"},
    {0x100481034u, 48u, "cHitTrackPlayer::StopCompletionNotification()"},
    {0x100481094u, 114u, "cHitTrackPlayer::GetPosition(float*, float*, float*) const"},
    {0x10048118cu, 118u, "cHitTrackPlayer::SetPosition(float, float, float)"},
    {0x100481248u, 32u, "cHitTrackPlayer::GetSoundPriority() const"},
    {0x100481268u, 32u, "cHitTrackPlayer::GetOriginalFrequency() const"},
    {0x100481298u, 40u, "cHitTrackPlayer::SetVolume(int)"},
    {0x1004812c8u, 32u, "cHitTrackPlayer::IsFading() const"},
    {0x1004812e8u, 32u, "cHitTrackPlayer::SourceList()"},
    {0x100481308u, 24u, "cHitTrackPlayer::SetName(cIGZString const&)"},
    // SC4DrawContext (51)
    {0x100771e8au, 122u, "SC4DrawContext::cStageInfo::cStageInfo()"},
    {0x100771f04u, 882u, "SC4DrawContext::SC4DrawContext()"},
    {0x100772360u, 546u, "SC4DrawContext::Init()"},
    {0x100772604u, 234u, "SC4DrawContext::SetCamera(cS3DCamera*)"},
    {0x1007726eeu, 538u, "SC4DrawContext::ResetModelViewTransform()"},
    {0x100772908u, 94u, "SC4DrawContext::GetModelViewMatrix(float*)"},
    {0x100772974u, 78u, "SC4DrawContext::EnableColorMaskFlag(bool)"},
    {0x1007729c2u, 102u, "SC4DrawContext::EnableAlphaTestFlag(bool)"},
    {0x100772a28u, 88u, "SC4DrawContext::EnableDepthTestFlag(bool)"},
    {0x100772a80u, 84u, "SC4DrawContext::EnableDepthMaskFlag(bool)"},
    {0x100772ad4u, 374u, "SC4DrawContext::EnableTextureStateFlag(bool, int)"},
    {0x100772c4au, 108u, "SC4DrawContext::EnableBlendStateFlag(bool)"},
    {0x100772cb6u, 88u, "SC4DrawContext::EnableCullFaceFlag(bool)"},
    {0x100772d0eu, 86u, "SC4DrawContext::SetTexture(int, int)"},
    {0x100772d64u, 68u, "SC4DrawContext::SetDepthFunc(int)"},
    {0x100772da8u, 120u, "SC4DrawContext::SetBlendFunc(int, int)"},
    {0x100772e20u, 170u, "SC4DrawContext::SetAlphaFunc(int, float)"},
    {0x100772ecau, 218u, "SC4DrawContext::SetTexEnvMode(int, int)"},
    {0x100772fa4u, 134u, "SC4DrawContext::SetTexCoord(eGDTexCoordSource, int)"},
    {0x10077302au, 222u, "SC4DrawContext::SetTexWrapModes(eGDTextureParam, eGDTextureParam, int)"},
    {0x100773108u, 266u, "SC4DrawContext::SetTexFiltering(eGDTextureParam, eGDTextureParam, int)"},
    {0x100773212u, 132u, "SC4DrawContext::SetTexCombiner(cGDCombiner const&, int)"},
    {0x100773296u, 40u, "SC4DrawContext::SetTexColor(cS3DVector4 const&)"},
    {0x1007732beu, 264u, "SC4DrawContext::SetTexTransform4(cS3DMatrix4 const&, int)"},
    {0x1007733c6u, 122u, "SC4DrawContext::ClearTexTransform(int)"},
    {0x100773440u, 20u, "SC4DrawContext::SetVertexBuffer(eGDVertexFormat, unsigned int, void const*)"},
    {0x100773462u, 70u, "SC4DrawContext::DrawPrims(eGDPrimType, eGDVertexFormat, unsigned int, void const*)"},
    {0x1007734a8u, 18u, "SC4DrawContext::DrawPrims(eGDPrimType, int, int)"},
    {0x1007734bau, 46u, "SC4DrawContext::DrawPrimsIndexed(eGDPrimType, int, int)"},
    {0x1007734e8u, 26u, "SC4DrawContext::DrawPrimsIndexed(eGDPrimType, int, unsigned short const*)"},
    {0x100773502u, 128u, "SC4DrawContext::SetLighting(bool)"},
    {0x100773582u, 12u, "SC4DrawContext::GetLighting()"},
    {0x10077358eu, 74u, "SC4DrawContext::SetShade(cS3DVector3 const&)"},
    {0x1007735d8u, 50u, "SC4DrawContext::SetSelfLitShade(cS3DVector3 const&)"},
    {0x10077360au, 180u, "SC4DrawContext::SetModelShade(cIS3DModelInstance*, cS3DVector3*)"},
    {0x1007736beu, 70u, "SC4DrawContext::ResetShade()"},
    {0x100773704u, 22u, "SC4DrawContext::SetTransparency(float)"},
    {0x10077371au, 30u, "SC4DrawContext::ResetTransparency()"},
    {0x100773738u, 418u, "SC4DrawContext::SetFog(bool, float const*, float, float)"},
    {0x100773906u, 770u, "SC4DrawContext::SetRenderState(cSC4PackedRenderState const&)"},
    {0x100773c08u, 328u, "SC4DrawContext::SetDefaultRenderStateUnilaterally()"},
    {0x100773d50u, 852u, "SC4DrawContext::SetRenderState(cS3DMaterial const&, SC4DrawAttributes*)"},
    {0x1007740a4u, 140u, "SC4DrawContext::SetRenderStateHighlight(cS3DMaterial const&, SC4DrawAttributes*)"},
    {0x100774130u, 136u, "SC4DrawContext::SetRenderStateHighlight(SC4HighlightType)"},
    {0x1007741b8u, 210u, "SC4DrawContext::SetDepthOffset(int)"},
    {0x10077428au, 400u, "SC4DrawContext::SetEmulatedSecondStageRenderState()"},
    {0x10077441au, 248u, "SC4DrawContext::RenderMesh(cS3DMesh const&)"},
    {0x100774512u, 744u, "SC4DrawContext::SetModelTransform(cS3DTransform const&)"},
    {0x1007747fau, 454u, "SC4DrawContext::SetModelTransform(cS3DMatrix4 const&)"},
    {0x100774ad0u, 908u, "SC4DrawContext::RenderModelInstance(cIS3DModelInstance*, cIS3DMeshAndMaterial*, SC4DrawAttributes*, bool)"},
    {0x100774e5cu, 515u, "SC4DrawContext::DrawBoundingBox(cS3DBoundingBox const&, cS3DVector4 const&)"},
    // cSC4WinSnapshotAlbum (40)
    {0x100398e48u, 2010u, "cSC4WinSnapshotAlbum::cSC4WinSnapshotAlbum()"},
    {0x1003997eau, 424u, "cSC4WinSnapshotAlbum::CreateSnapshotAlbumWindow(cIGZWin*&)"},
    {0x100399992u, 1614u, "cSC4WinSnapshotAlbum::on_init(cIGZWin*)"},
    {0x100399fe0u, 378u, "cSC4WinSnapshotAlbum::DoModalWin()"},
    {0x10039a15au, 140u, "cSC4WinSnapshotAlbum::end_dialog(cIGZWin*, unsigned int)"},
    {0x10039a1e6u, 1586u, "cSC4WinSnapshotAlbum::update_list_of_album_directories(bool&, bool)"},
    {0x10039a818u, 598u, "cSC4WinSnapshotAlbum::SetInitialFile()"},
    {0x10039aa6eu, 596u, "cSC4WinSnapshotAlbum::on_shutdown(cIGZWin*)"},
    {0x10039acc2u, 460u, "cSC4WinSnapshotAlbum::DoWinMsg(cIGZWin*, unsigned int, unsigned long, unsigned long, unsigned long)"},
    {0x10039ae8eu, 1018u, "cSC4WinSnapshotAlbum::SetCurrentFile(cRZString const&, bool)"},
    {0x10039b288u, 732u, "cSC4WinSnapshotAlbum::on_button_activated(cIGZWin*, unsigned int)"},
    {0x10039b564u, 208u, "cSC4WinSnapshotAlbum::GoToAlbum(unsigned int)"},
    {0x10039b634u, 546u, "cSC4WinSnapshotAlbum::SaveDescriptionOfFile()"},
    {0x10039b856u, 876u, "cSC4WinSnapshotAlbum::DeleteCurrentGraphic()"},
    {0x10039bbc2u, 516u, "cSC4WinSnapshotAlbum::GoToPreviousGraphic()"},
    {0x10039bdc6u, 542u, "cSC4WinSnapshotAlbum::GoToNextGraphic()"},
    {0x10039bfe4u, 1400u, "cSC4WinSnapshotAlbum::RefreshGraphicList(bool*)"},
    {0x10039c55cu, 290u, "cSC4WinSnapshotAlbum::convert_album_directory_name_to_album_name(cRZString const&, cRZString&)"},
    {0x10039c67eu, 310u, "cSC4WinSnapshotAlbum::album_directory_has_album_entries(cRZString const&, bool)"},
    {0x10039c7b4u, 432u, "cSC4WinSnapshotAlbum::set_to_no_image_file(bool)"},
    {0x10039c964u, 162u, "cSC4WinSnapshotAlbum::set_image_count_text(unsigned int, unsigned int)"},
    {0x10039ca06u, 306u, "cSC4WinSnapshotAlbum::convert_album_directory_name_to_album_index(cRZString const&, unsigned int&)"},
    {0x10039cb38u, 224u, "cSC4WinSnapshotAlbum::GoToAlbumForCurrentCity(bool)"},
    {0x10039cc18u, 392u, "cSC4WinSnapshotAlbum::GoToAlbum(cRZString const&, unsigned int, bool)"},
    {0x10039cda0u, 192u, "cSC4WinSnapshotAlbum::AlbumExists(cRZString const&, unsigned int)"},
    {0x10039ce60u, 580u, "cSC4WinSnapshotAlbum::CreateAlbum(cRZString const&, unsigned int)"},
    {0x10039d0a4u, 2232u, "cSC4WinSnapshotAlbum::DeleteAlbum(unsigned int)"},
    {0x10039d95cu, 1522u, "cSC4WinSnapshotAlbum::AddNewGraphic(cRZString const&, cRZString const&, cRZString const&, cRZString const&, unsigned int, bool)"},
    {0x10039df4eu, 1220u, "cSC4WinSnapshotAlbum::GetNewTempFileName(cRZString const&, cRZString&)"},
    {0x10039e412u, 344u, "cSC4WinSnapshotAlbum::SaveInformationOfStandardFile(cRZString const&, cRZString const&)"},
    {0x10039e56au, 334u, "cSC4WinSnapshotAlbum::SaveDescriptionOfFile(cRZString const&, cRZString const&)"},
    {0x10039e6b8u, 612u, "cSC4WinSnapshotAlbum::GoToGraphic(cRZString const&)"},
    {0x10039e91cu, 402u, "cSC4WinSnapshotAlbum::GetDescriptionOfFile(cRZString const&, cRZString&)"},
    {0x10039eaaeu, 128u, "cSC4WinSnapshotAlbum::set_image_count_text()"},
    {0x10039eb2eu, 422u, "cSC4WinSnapshotAlbum::get_path_for_info_file(cRZString const&, cRZString&)"},
    {0x10039ecd4u, 422u, "cSC4WinSnapshotAlbum::get_path_for_description_file(cRZString const&, cRZString&)"},
    {0x10039f058u, 3678u, "cSC4WinSnapshotAlbum::GetInformationOfStandardFile(cRZString const&, cRZString&)"},
    {0x10039feb6u, 220u, "cSC4WinSnapshotAlbum::UnObfuscateData(cRZString const&, cRZString&)"},
    {0x10039ff92u, 220u, "cSC4WinSnapshotAlbum::ObfuscateData(cRZString const&, cRZString&)"},
    {0x1003a006eu, 294u, "cSC4WinSnapshotAlbum::GetNewTempFileNameForCurrentAlbum(cRZString&)"},
    // SC4ViewUtilities (37)
    {0x100173480u, 580u, "SC4ViewUtilities::SC4ViewUtilities()"},
    {0x1001736e2u, 106u, "SC4ViewUtilities::ClipWorldPositionToCityPosition(float*)"},
    {0x10017376au, 68u, "SC4ViewUtilities::GetVisibleEdges(int&, int&)"},
    {0x1001737cau, 114u, "SC4ViewUtilities::GetCameraViewAnglesForRotation(int, int, float&, float&, float&)"},
    {0x10017383cu, 164u, "SC4ViewUtilities::ProjectPointToViewCoordinates(float*)"},
    {0x1001738e0u, 96u, "SC4ViewUtilities::ProjectPointFromViewCoordinatesToViewportCoordinates(float*)"},
    {0x100173940u, 120u, "SC4ViewUtilities::ProjectPointToViewportCoordinates(float*)"},
    {0x1001739b8u, 98u, "SC4ViewUtilities::ProjectPointToViewportZ(float const*)"},
    {0x100173a1au, 86u, "SC4ViewUtilities::ProjectPointToViewportCoordinates2D(float*)"},
    {0x100173a70u, 136u, "SC4ViewUtilities::ProjectPointToFixedViewCoordinates(float*)"},
    {0x100173af8u, 34u, "SC4ViewUtilities::ProjectPointFromViewportCoordinatesToFixedViewCoordinates(float*)"},
    {0x100173b1au, 114u, "SC4ViewUtilities::UnProjectPointFromViewportCoordinatesToWorldCoordinates(float*)"},
    {0x100173b8cu, 176u, "SC4ViewUtilities::UnProjectPointFromViewCoordinatesToWorldCoordinates(float*)"},
    {0x100173c3cu, 58u, "SC4ViewUtilities::GetViewFrustumBoundingBox(float*, float*)"},
    {0x100173c76u, 392u, "SC4ViewUtilities::IsBoundingBoxInViewFrustum(float const*, float const*)"},
    {0x100173dfeu, 396u, "SC4ViewUtilities::IsBoundingBoxInDrawFrustum(float const*, float const*)"},
    {0x100173f8au, 212u, "SC4ViewUtilities::IsPointInViewFrustum(float const*)"},
    {0x10017405eu, 1404u, "SC4ViewUtilities::ProjectBoundingBoxToViewportRectangle(float const*, float const*, int*, int*)"},
    {0x1001745dau, 3098u, "SC4ViewUtilities::UpdateCameraPosition(cSC4CameraControl*)"},
    {0x1001751f4u, 566u, "SC4ViewUtilities::GetFrustumAABBCityClipped(bool, cS3DVector3&, cS3DVector3&)"},
    {0x100175444u, 3288u, "SC4ViewUtilities::BuildDrawFrustumDataFromViewportRect(int const*, int const*)"},
    {0x10017611cu, 240u, "SC4ViewUtilities::GetViewportLimitsGroundPlanePositions(float*, float*, float*, float*)"},
    {0x10017620cu, 228u, "SC4ViewUtilities::CalculateViewportGroundPlanePositions(float*, float*, float*, float*, int const*, int const*)"},
    {0x1001762f0u, 242u, "SC4ViewUtilities::ViewportPositionToRay(int, int, float*, float*)"},
    {0x1001763e2u, 158u, "SC4ViewUtilities::GetViewportPointIntersectionWithGroundPlane(int, int, float*)"},
    {0x100176480u, 88u, "SC4ViewUtilities::GetRayIntersectionWithGroundPlane(float const*, float const*, float*)"},
    {0x1001764d8u, 116u, "SC4ViewUtilities::TrianglePick(cSC4PickContext*, float const*, float const*, float const*)"},
    {0x10017654cu, 448u, "SC4ViewUtilities::MeshPick(cSC4PickContext*, cS3DMesh const*, cS3DTransform const*)"},
    {0x100176a3cu, 170u, "SC4ViewUtilities::ModelPick(cSC4PickContext*, cIS3DModelInstance const*, cIS3DMeshAndMaterial const*)"},
    {0x100176ae6u, 198u, "SC4ViewUtilities::TransformBoundingBox(cS3DTransform const*, cS3DBoundingBox&)"},
    {0x100176bacu, 1082u, "SC4ViewUtilities::GetGroundPlaneVisibleAreaRectangle(float*, float*, float*, float*)"},
    {0x10017703eu, 76u, "SC4ViewUtilities::GetDrawFrustumGroundPlanePositions(float*, float*, float*, float*)"},
    {0x10017708au, 278u, "SC4ViewUtilities::GetCityCornerPosition(int, float*)"},
    {0x1001771a0u, 18u, "SC4ViewUtilities::GetObjectYRotationAngle(int)"},
    {0x1001771cau, 114u, "SC4ViewUtilities::RotateVector(float*, int)"},
    {0x10017723cu, 60u, "SC4ViewUtilities::GetVectorForEdge(float*, int)"},
    {0x100177278u, 72u, "SC4ViewUtilities::GetCityWidthFromOrthoView(float, int)"},
    // cSC4WinRegionScreen (33)
    {0x100235b78u, 858u, "cSC4WinRegionScreen::cSC4WinRegionScreen()"},
    {0x100236398u, 1998u, "cSC4WinRegionScreen::PutCitiesInView()"},
    {0x100236b66u, 208u, "cSC4WinRegionScreen::SetCitySnapshotsToDefault(cSC4RegionViewCity&)"},
    {0x100236c36u, 1232u, "cSC4WinRegionScreen::RecalculateImagesFromCitySnapshots(cSC4RegionViewCity&)"},
    {0x100237106u, 636u, "cSC4WinRegionScreen::ComputeBaseLimits()"},
    {0x100237382u, 688u, "cSC4WinRegionScreen::GZPaint()"},
    {0x100237632u, 278u, "cSC4WinRegionScreen::UpdateRendererPosition()"},
    {0x100237748u, 76u, "cSC4WinRegionScreen::GZOnMouseEnter(cIGZWin*)"},
    {0x100237794u, 30u, "cSC4WinRegionScreen::GZOnMouseMove(int, int, unsigned int)"},
    {0x1002377b2u, 508u, "cSC4WinRegionScreen::GZOnMouseDownL(int, int, unsigned int)"},
    {0x1002379aeu, 170u, "cSC4WinRegionScreen::RestoreSignposts()"},
    {0x100237a58u, 386u, "cSC4WinRegionScreen::ShowPanel(int)"},
    {0x100237bdau, 24u, "cSC4WinRegionScreen::GZOnMouseDownR(int, int, unsigned int)"},
    {0x100237bf2u, 3296u, "cSC4WinRegionScreen::Init()"},
    {0x1002388d2u, 160u, "cSC4WinRegionScreen::PostRegionScreenMessage(unsigned int)"},
    {0x100238972u, 1756u, "cSC4WinRegionScreen::InitApplicationServices()"},
    {0x10023904eu, 556u, "cSC4WinRegionScreen::ReadTunableValues()"},
    {0x10023927au, 2086u, "cSC4WinRegionScreen::LoadGUI()"},
    {0x100239aa0u, 780u, "cSC4WinRegionScreen::Shutdown()"},
    {0x100239dacu, 362u, "cSC4WinRegionScreen::ShutdownApplicationServices()"},
    {0x100239fccu, 272u, "cSC4WinRegionScreen::DoMessage(cGZMessage&)"},
    {0x10023a0dcu, 288u, "cSC4WinRegionScreen::DoMessage(cIGZMessage2*)"},
    {0x10023a1fcu, 1100u, "cSC4WinRegionScreen::ProcessCommand(unsigned int, cIGZCommandParameterSet&, cIGZCommandParameterSet&)"},
    {0x10023a66cu, 2096u, "cSC4WinRegionScreen::MakeWholeRegion()"},
    {0x10023ae9cu, 660u, "cSC4WinRegionScreen::GetOrCreateRegionalCity(cSC4RegionViewCity const&, bool)"},
    {0x10023b130u, 566u, "cSC4WinRegionScreen::LoadRegion(cRZString const&)"},
    {0x10023b366u, 726u, "cSC4WinRegionScreen::LaunchCity(cRZString const&, unsigned int, cRZString&, bool)"},
    {0x10023b63cu, 338u, "cSC4WinRegionScreen::LaunchRegionalCity(cRZAutoRefCount<cISC4RegionalCity>*)"},
    {0x10023b78eu, 30u, "cSC4WinRegionScreen::GZOnMouseExit(cIGZWin*)"},
    {0x10023b7bcu, 138u, "cSC4WinRegionScreen::SetUserPreferencesToCurrentRegion()"},
    {0x10023b846u, 228u, "cSC4WinRegionScreen::SetUserPreferencesToCurrentPosition()"},
    {0x10023b92au, 3106u, "cSC4WinRegionScreen::GZOnCommand(unsigned int, unsigned long)"},
    {0x10023c54cu, 246u, "cSC4WinRegionScreen::UpdatePopulation()"},
    // cS3DModelInstanceAnimatedSprite (26)
    {0x1001a455cu, 640u, "cS3DModelInstanceAnimatedSprite::cS3DModelInstanceAnimatedSprite()"},
    {0x1001a4a40u, 3480u, "cS3DModelInstanceAnimatedSprite::SetOrientation(cS3DVector3 const&)"},
    {0x1001a57d8u, 334u, "cS3DModelInstanceAnimatedSprite::SetOrientation(cS3DVector3 const&, cS3DVector3 const&)"},
    {0x1001a5926u, 188u, "cS3DModelInstanceAnimatedSprite::ModelPointToWorldPoint(cS3DVector3&)"},
    {0x1001a59e2u, 160u, "cS3DModelInstanceAnimatedSprite::ModelPointToWorldPoint(float*)"},
    {0x1001a5a82u, 128u, "cS3DModelInstanceAnimatedSprite::ModelVectorToWorldVector(cS3DVector3&)"},
    {0x1001a5b02u, 128u, "cS3DModelInstanceAnimatedSprite::ModelVectorToWorldVector(float*)"},
    {0x1001a5b82u, 56u, "cS3DModelInstanceAnimatedSprite::SetZoomAndRotation(int, int)"},
    {0x1001a5bbau, 90u, "cS3DModelInstanceAnimatedSprite::SetFrameset(int)"},
    {0x1001a5c14u, 108u, "cS3DModelInstanceAnimatedSprite::SetFrame(int)"},
    {0x1001a5c9au, 78u, "cS3DModelInstanceAnimatedSprite::IncrementFrame()"},
    {0x1001a5ce8u, 200u, "cS3DModelInstanceAnimatedSprite::IncrementFrameSimple()"},
    {0x1001a5dc4u, 302u, "cS3DModelInstanceAnimatedSprite::DoAutoFrameUpdate(float)"},
    {0x1001a5f08u, 82u, "cS3DModelInstanceAnimatedSprite::SetOpacity(bool, float)"},
    {0x1001a5f5au, 60u, "cS3DModelInstanceAnimatedSprite::SetHighlight(int)"},
    {0x1001a5fc6u, 98u, "cS3DModelInstanceAnimatedSprite::LoadSpriteResource(cGZPersistResourceKey const&)"},
    {0x1001a6028u, 536u, "cS3DModelInstanceAnimatedSprite::LoadSpriteResource(cIS3DResSpriteAnim*)"},
    {0x1001a6384u, 106u, "cS3DModelInstanceAnimatedSprite::GetTextureData(float, float, float, int, int, int, int, cAnimatedSpriteTextureData*)"},
    {0x1001a63eeu, 220u, "cS3DModelInstanceAnimatedSprite::GetTextureResourceData(float, float, float, int, int, cAnimatedSpriteTextureResourceData*&)"},
    {0x1001a64cau, 560u, "cS3DModelInstanceAnimatedSprite::GetTextureData(int, int, int, cAnimatedSpriteTextureResourceData const*, cAnimatedSpriteTextureData*)"},
    {0x1001a66fau, 52u, "cS3DModelInstanceAnimatedSprite::GetSortValue(unsigned int&) const"},
    {0x1001a675au, 144u, "cS3DModelInstanceAnimatedSprite::GetWorldBounds(cS3DBoundingBox&)"},
    {0x1001a67f8u, 936u, "cS3DModelInstanceAnimatedSprite::UpdateView(SC4DrawContext*)"},
    {0x1001a6ba0u, 620u, "cS3DModelInstanceAnimatedSprite::Draw(SC4DrawContext*, bool)"},
    {0x1001a6e24u, 162u, "cS3DModelInstanceAnimatedSprite::Pick(cSC4PickContext*)"},
    {0x1001a701cu, 60u, "cS3DModelInstanceAnimatedSprite::GetBounds(cS3DBoundingBox&)"},
    // cSC4WinMiniMap (24)
    {0x1005c4664u, 232u, "cSC4WinMiniMap::cSC4WinMiniMap()"},
    {0x1005c4946u, 790u, "cSC4WinMiniMap::Init()"},
    {0x1005c4c5cu, 352u, "cSC4WinMiniMap::LoadTunableValues()"},
    {0x1005c4dbcu, 170u, "cSC4WinMiniMap::OnCityInit()"},
    {0x1005c4e66u, 272u, "cSC4WinMiniMap::Shutdown()"},
    {0x1005c4f76u, 100u, "cSC4WinMiniMap::UnhookOccupantMessages()"},
    {0x1005c4fdau, 110u, "cSC4WinMiniMap::SetArea(int, int, int, int)"},
    {0x1005c5048u, 38u, "cSC4WinMiniMap::GZOnMouseDownL(int, int, unsigned int)"},
    {0x1005c506eu, 416u, "cSC4WinMiniMap::CenterOnMapPoint(int, int)"},
    {0x1005c520eu, 20u, "cSC4WinMiniMap::GZOnMouseUpL(int, int, unsigned int)"},
    {0x1005c5222u, 22u, "cSC4WinMiniMap::GZOnMouseMove(int, int, unsigned int)"},
    {0x1005c5260u, 1066u, "cSC4WinMiniMap::DoMessage(cIGZMessage2*)"},
    {0x1005c568au, 96u, "cSC4WinMiniMap::OnCityShutdown()"},
    {0x1005c56eau, 146u, "cSC4WinMiniMap::InvalidateTZMapRect(cRZRect const&)"},
    {0x1005c57a6u, 82u, "cSC4WinMiniMap::SetOverlay(cIGZBuffer*)"},
    {0x1005c5842u, 76u, "cSC4WinMiniMap::SetDefaultZoneColorToTrans()"},
    {0x1005c58d4u, 70u, "cSC4WinMiniMap::SetDefaultZoneColor(unsigned int)"},
    {0x1005c599au, 92u, "cSC4WinMiniMap::SetFlags(unsigned int, unsigned int)"},
    {0x1005c5a08u, 196u, "cSC4WinMiniMap::InitTZMap()"},
    {0x1005c5accu, 110u, "cSC4WinMiniMap::HookOccupantMessages()"},
    {0x1005c5b3au, 106u, "cSC4WinMiniMap::cTZMap::Init(int, int)"},
    {0x1005c5ba4u, 1730u, "cSC4WinMiniMap::Update()"},
    {0x1005c637au, 1708u, "cSC4WinMiniMap::UpdateTerrainAndZoneMap()"},
    {0x1005c6a38u, 2036u, "cSC4WinMiniMap::GZPaint()"},
    // cSC4WinCatalogView (22)
    {0x1002fcf76u, 284u, "cSC4WinCatalogView::cSC4WinCatalogView()"},
    {0x1002fd29eu, 128u, "cSC4WinCatalogView::Init()"},
    {0x1002fd330u, 220u, "cSC4WinCatalogView::Shutdown()"},
    {0x1002fd41au, 168u, "cSC4WinCatalogView::SetCalloutBox(cISC4WinCalloutBox*)"},
    {0x1002fd4c2u, 86u, "cSC4WinCatalogView::Clear()"},
    {0x1002fd518u, 92u, "cSC4WinCatalogView::AddItem(cISC4CatalogItem*)"},
    {0x1002fd57cu, 76u, "cSC4WinCatalogView::GetItemByID(int)"},
    {0x1002fd5c8u, 110u, "cSC4WinCatalogView::SetEnabledAllItemsExceptItemWithID(unsigned int, bool)"},
    {0x1002fd636u, 194u, "cSC4WinCatalogView::GetItemPositionByID(unsigned int, int&, int&)"},
    {0x1002fd6f8u, 232u, "cSC4WinCatalogView::FlashItemByID(unsigned int, unsigned int, unsigned int)"},
    {0x1002fd8a0u, 200u, "cSC4WinCatalogView::SetScrollPosition(int)"},
    {0x1002fd968u, 82u, "cSC4WinCatalogView::ChangeScrollPos(int)"},
    {0x1002fd9bau, 86u, "cSC4WinCatalogView::DoMessage(cGZMessage&)"},
    {0x1002fda1eu, 102u, "cSC4WinCatalogView::SetTextFontStyle(unsigned int)"},
    {0x1002fda84u, 832u, "cSC4WinCatalogView::SetHighlightedItem(int)"},
    {0x1002fddf2u, 98u, "cSC4WinCatalogView::SetSelectedItemByID(unsigned int)"},
    {0x1002fde54u, 194u, "cSC4WinCatalogView::MakeItemVisible(unsigned int)"},
    {0x1002fdf16u, 70u, "cSC4WinCatalogView::GZOnMouseMove(int, int, unsigned int)"},
    {0x1002fdf6eu, 18u, "cSC4WinCatalogView::GZOnMouseExit(cIGZWin*)"},
    {0x1002fdf96u, 252u, "cSC4WinCatalogView::GZOnMouseUpL(int, int, unsigned int)"},
    {0x1002fe0a4u, 84u, "cSC4WinCatalogView::GZOnMouseWheel(int, int, unsigned int, int)"},
    {0x1002fe15cu, 418u, "cSC4WinCatalogView::GZPaint()"},
    // cSC4AudioPreferencesWinProc (20)
    {0x1003a89e2u, 124u, "cSC4AudioPreferencesWinProc::cSC4AudioPreferencesWinProc()"},
    {0x1003a8ad0u, 330u, "cSC4AudioPreferencesWinProc::DoModalWin()"},
    {0x1003a8c1au, 194u, "cSC4AudioPreferencesWinProc::EndDialog()"},
    {0x1003a8cdcu, 166u, "cSC4AudioPreferencesWinProc::ShowDialog()"},
    {0x1003a8d82u, 570u, "cSC4AudioPreferencesWinProc::StaticInit()"},
    {0x1003a8fbcu, 178u, "cSC4AudioPreferencesWinProc::ReadTunerVariables()"},
    {0x1003a906eu, 2578u, "cSC4AudioPreferencesWinProc::InitDialog()"},
    {0x1003a9a80u, 42u, "cSC4AudioPreferencesWinProc::Shutdown()"},
    {0x1003a9b02u, 66u, "cSC4AudioPreferencesWinProc::FinalRelease()"},
    {0x1003a9b44u, 548u, "cSC4AudioPreferencesWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x1003a9d68u, 3338u, "cSC4AudioPreferencesWinProc::HandleControlMessage(unsigned int, cIGZWin*, unsigned int)"},
    {0x1003aaa72u, 198u, "cSC4AudioPreferencesWinProc::UpdateSoundSys()"},
    {0x1003aabceu, 1046u, "cSC4AudioPreferencesWinProc::InsertMusicList(cSC4AudioPreferencesWinProc::eAudioMode, int)"},
    {0x1003ab0c2u, 440u, "cSC4AudioPreferencesWinProc::ReadPlaylistInfoFromExemplar(cSC4AudioPreferencesWinProc::eAudioMode)"},
    {0x1003ab27au, 120u, "cSC4AudioPreferencesWinProc::SetDefaultMusicListPreferences()"},
    {0x1003ab2f2u, 248u, "cSC4AudioPreferencesWinProc::AudioFileExists(cSC4AudioPreferencesWinProc::eAudioMode, unsigned int)"},
    {0x1003ab3eau, 102u, "cSC4AudioPreferencesWinProc::GetMusicSelectionIndex(int&)"},
    {0x1003ab450u, 756u, "cSC4AudioPreferencesWinProc::UpdateExclusionList(cSC4AudioPreferencesWinProc::eAudioMode)"},
    {0x1003ab744u, 1200u, "cSC4AudioPreferencesWinProc::IsSongEnabled(cRZString const&)"},
    {0x1003abbf4u, 112u, "cSC4AudioPreferencesWinProc::GetVolumeFromSliderValue(unsigned int, cSC4AudioPreferencesWinProc::eAudioType)"},
    // cSC4GeometrySourcedParticlesEffect (19)
    {0x1005e88a8u, 190u, "cSC4GeometrySourcedParticlesEffect::cSC4GeometrySourcedParticlesEffect()"},
    {0x1005e8992u, 66u, "cSC4GeometrySourcedParticlesEffect::Shutdown()"},
    {0x1005e89e2u, 228u, "cSC4GeometrySourcedParticlesEffect::SetGeometrySource(unsigned int)"},
    {0x1005e8ac6u, 268u, "cSC4GeometrySourcedParticlesEffect::SetGeometrySource(cIS3DMeshAndMaterial*)"},
    {0x1005e8bd2u, 326u, "cSC4GeometrySourcedParticlesEffect::SetupArrays()"},
    {0x1005e8d18u, 520u, "cSC4GeometrySourcedParticlesEffect::CreateParticlesFromModelQuads()"},
    {0x1005e8f20u, 1796u, "cSC4GeometrySourcedParticlesEffect::DiceAndAddQuad(cS3DVector3*, cS3DVector2*, unsigned int, unsigned int)"},
    {0x1005e9624u, 618u, "cSC4GeometrySourcedParticlesEffect::CreateParticlesFromModelTriangles()"},
    {0x1005e988eu, 758u, "cSC4GeometrySourcedParticlesEffect::DiceAndAddTriangle(std::__1::vector<cS3DVector3, std::__1::allocator<cS3DVector3>>&, std::__1::vector<cS3DVector2, std::__1::allocator<cS3DVector2>>&, int*, float, unsigned int)"},
    {0x1005e9b84u, 778u, "cSC4GeometrySourcedParticlesEffect::CreateParticle(cS3DVector3 const&)"},
    {0x1005e9e8eu, 1242u, "cSC4GeometrySourcedParticlesEffect::AddTriangle(cS3DVector3*, cS3DVector2*, int*, unsigned int)"},
    {0x1005ea3e2u, 300u, "cSC4GeometrySourcedParticlesEffect::SetDescription(cSC4ParticlesDescription*)"},
    {0x1005ea50eu, 244u, "cSC4GeometrySourcedParticlesEffect::SetModel(cIS3DModelInstance*)"},
    {0x1005ea602u, 84u, "cSC4GeometrySourcedParticlesEffect::SetFloatParams(cISC4VisualEffect::tParamType, float const*, int)"},
    {0x1005ea656u, 866u, "cSC4GeometrySourcedParticlesEffect::SetExplosionPointFromViewDir(cS3DVector3 const&, cS3DVector3 const&)"},
    {0x1005ea9e0u, 310u, "cSC4GeometrySourcedParticlesEffect::GenerateNewParticles()"},
    {0x1005eab16u, 1158u, "cSC4GeometrySourcedParticlesEffect::UpdateParticlePhysics(cSC4EffectsParticle*, float)"},
    {0x1005eaf9cu, 2492u, "cSC4GeometrySourcedParticlesEffect::RenderActiveParticles(SC4DrawContext*)"},
    {0x1005eb958u, 4424u, "cSC4GeometrySourcedParticlesEffect::RenderActiveParticlesVB(SC4DrawContext*)"},
    // cRZSysServPtr (17)
    {0x100007394u, 74u, "cRZSysServPtr<cIGZGraphicSystem2, 3296425084u, 3289776732u>::operator=(cIGZGraphicSystem2*)"},
    {0x1003aafe4u, 67u, "cRZSysServPtr<cIGZSndSys, 274757586u, 600755104u>::operator=(cIGZSndSys*)"},
    {0x1003edb30u, 68u, "cRZSysServPtr<cIGZRadio, 3763405408u, 487043999u>::operator=(cIGZRadio*)"},
    {0x1003f5950u, 32u, "cRZSysServPtr<cIGZMessageServer2, 1696765127u, 83526747u>::cRZSysServPtr()"},
    {0x1003f5970u, 32u, "cRZSysServPtr<cIGZMessageServer2, 1696765127u, 83526747u>::operator cIGZMessageServer2*()"},
    {0x1003f82a0u, 128u, "cRZSysServPtr<cIGZMessageServer2, 1696765127u, 83526747u>::cRZSysServPtr()"},
    {0x100487980u, 32u, "cRZSysServPtr<cIGZFileSystem, 3760207456u, 544416093u>::cRZSysServPtr()"},
    {0x1004879a0u, 32u, "cRZSysServPtr<cIGZFileSystem, 3760207456u, 544416093u>::operator cIGZFileSystem*()"},
    {0x1004879c0u, 32u, "cRZSysServPtr<cIGZFileSystem, 3760207456u, 544416093u>::operator->() const"},
    {0x10049bc80u, 128u, "cRZSysServPtr<cIGZFileSystem, 3760207456u, 544416093u>::cRZSysServPtr()"},
    {0x1004a1b70u, 32u, "cRZSysServPtr<cIGZPersistResourceManager, 1701547772u, 90935406u>::cRZSysServPtr()"},
    {0x1004a1b90u, 32u, "cRZSysServPtr<cIGZPersistResourceManager, 1701547772u, 90935406u>::operator cIGZPersistResourceManager*()"},
    {0x1004b0430u, 128u, "cRZSysServPtr<cIGZPersistResourceManager, 1701547772u, 90935406u>::cRZSysServPtr()"},
    {0x1006256f0u, 32u, "cRZSysServPtr<cIGZSndSys, 274757586u, 600755104u>::cRZSysServPtr()"},
    {0x100625710u, 32u, "cRZSysServPtr<cIGZSndSys, 274757586u, 600755104u>::operator cIGZSndSys*()"},
    {0x100627290u, 128u, "cRZSysServPtr<cIGZSndSys, 274757586u, 600755104u>::cRZSysServPtr()"},
    {0x100636300u, 32u, "cRZSysServPtr<cIGZSndSys, 274757586u, 600755104u>::operator->() const"},
    // cSC4WinSnapshotTaker (17)
    {0x1003a0194u, 256u, "cSC4WinSnapshotTaker::cSC4WinSnapshotTaker()"},
    {0x1003a0366u, 1694u, "cSC4WinSnapshotTaker::Init()"},
    {0x1003a0a04u, 304u, "cSC4WinSnapshotTaker::AdjustPosition()"},
    {0x1003a0b34u, 230u, "cSC4WinSnapshotTaker::GoToSizeIndex(unsigned int)"},
    {0x1003a0c1au, 106u, "cSC4WinSnapshotTaker::Shutdown()"},
    {0x1003a0c84u, 214u, "cSC4WinSnapshotTaker::CreateWinSnapshotTaker(cIGZWin*, unsigned int)"},
    {0x1003a0d5au, 108u, "cSC4WinSnapshotTaker::RemoveSelf()"},
    {0x1003a0dceu, 318u, "cSC4WinSnapshotTaker::GZOnKeyDown(unsigned int, unsigned int)"},
    {0x1003a0f0cu, 24u, "cSC4WinSnapshotTaker::GZOnMouseDownL(int, int, unsigned int)"},
    {0x1003a0f24u, 14u, "cSC4WinSnapshotTaker::GZOnMouseDownR(int, int, unsigned int)"},
    {0x1003a0f32u, 156u, "cSC4WinSnapshotTaker::GZOnMouseMove(int, int, unsigned int)"},
    {0x1003a0fceu, 478u, "cSC4WinSnapshotTaker::GZPaint()"},
    {0x1003a11acu, 138u, "cSC4WinSnapshotTaker::DoMessage(cGZMessage&)"},
    {0x1003a1236u, 956u, "cSC4WinSnapshotTaker::TakeSnapshot(bool)"},
    {0x1003a15f2u, 510u, "cSC4WinSnapshotTaker::ShowSnapshotAlbum(bool, bool, cIGZWin*, cIGZString*)"},
    {0x1003a17f0u, 396u, "cSC4WinSnapshotTaker::AddNewGraphicForCurrentCity(cSC4WinSnapshotAlbum*, cRZString const&, bool)"},
    {0x1003a197cu, 2488u, "cSC4WinSnapshotTaker::GetGraphicInfoForCurrentCity(cRZString&)"},
    // tSound (16)
    {0x1003b9952u, 74u, "tSound::Pause(bool)"},
    {0x1003b999cu, 216u, "tSound::Resume()"},
    {0x1003b9b08u, 290u, "tSound::EnableCompletionNotification(bool)"},
    {0x1003b9c2au, 98u, "tSound::AddSlave(tSound*)"},
    {0x1003b9cf0u, 146u, "tSound::StartStopSoundForSimulationPause()"},
    {0x1003b9dc8u, 276u, "tSound::TransferSlaves()"},
    {0x1003b9edcu, 86u, "tSound::RemoveSlave(tSound*)"},
    {0x1003b9f74u, 62u, "tSound::SetPosition(cS3DVector3 const&)"},
    {0x1003b9fb2u, 80u, "tSound::SetVolume(int)"},
    {0x1003ba002u, 102u, "tSound::Update(unsigned int)"},
    {0x1003ba068u, 168u, "tSound::StartStopSoundForDistance()"},
    {0x1003ba226u, 182u, "tSound::UpdatePosition()"},
    {0x1003ba2dcu, 154u, "tSound::UpdateVolPan3D()"},
    {0x1003ba376u, 126u, "tSound::UpdateVolPanEffects()"},
    {0x1003ba3f4u, 190u, "tSound::ShouldPauseForDistance()"},
    {0x1003ba4fcu, 1146u, "tSound::UpdateAveragePosition(cS3DVector3&, cS3DVector3&)"},
    // cSC4WinTertiaryMenuFrame (14)
    {0x1002fe310u, 198u, "cSC4WinTertiaryMenuFrame::cSC4WinTertiaryMenuFrame()"},
    {0x1002fe57eu, 84u, "cSC4WinTertiaryMenuFrame::Init()"},
    {0x1002fe5e4u, 118u, "cSC4WinTertiaryMenuFrame::Shutdown()"},
    {0x1002fe668u, 1048u, "cSC4WinTertiaryMenuFrame::GZPaint()"},
    {0x1002fea92u, 192u, "cSC4WinTertiaryMenuFrame::SetBackgroundImage(cIGZBuffer*, int, int, int, int, int, int, int)"},
    {0x1002feb52u, 304u, "cSC4WinTertiaryMenuFrame::PositionFrame(int, int, int, int, int, int)"},
    {0x1002fec82u, 60u, "cSC4WinTertiaryMenuFrame::IsPointInWindowWindowCoordinates(int, int) const"},
    {0x1002fecfau, 74u, "cSC4WinTertiaryMenuFrame::IsPointInWindowScreenCoordinates(int, int) const"},
    {0x1002fed94u, 136u, "cSC4WinTertiaryMenuFrame::GZOnMouseMove(int, int, unsigned int)"},
    {0x1002fee2eu, 48u, "cSC4WinTertiaryMenuFrame::GZOnMouseExit(cIGZWin*)"},
    {0x1002fee92u, 192u, "cSC4WinTertiaryMenuFrame::GZOnMouseDownL(int, int, unsigned int)"},
    {0x1002fef52u, 96u, "cSC4WinTertiaryMenuFrame::ScrollContentCallback(void*)"},
    {0x1002fefc4u, 84u, "cSC4WinTertiaryMenuFrame::GZOnMouseUpL(int, int, unsigned int)"},
    {0x1002ff0c2u, 70u, "cSC4WinTertiaryMenuFrame::EnableScrollArrow(bool, bool)"},
    // cSC4ViewInputControlDemolish (14)
    {0x1003cb99cu, 220u, "cSC4ViewInputControlDemolish::cSC4ViewInputControlDemolish()"},
    {0x1003cba78u, 732u, "cSC4ViewInputControlDemolish::Init()"},
    {0x1003cbd54u, 356u, "cSC4ViewInputControlDemolish::Shutdown()"},
    {0x1003cbeb8u, 222u, "cSC4ViewInputControlDemolish::ClearSelectedRegion()"},
    {0x1003cbf96u, 166u, "cSC4ViewInputControlDemolish::OnMouseDownL(int, int, unsigned int)"},
    {0x1003cc03cu, 200u, "cSC4ViewInputControlDemolish::DoCellPick(int, int, SC4Point<int>&)"},
    {0x1003cc104u, 560u, "cSC4ViewInputControlDemolish::DoLotPick(int, int, SC4Rect<int>&)"},
    {0x1003cc334u, 2416u, "cSC4ViewInputControlDemolish::UpdateSelectedRegion()"},
    {0x1003ccca4u, 286u, "cSC4ViewInputControlDemolish::OnMouseMove(int, int, unsigned int)"},
    {0x1003ccdc2u, 438u, "cSC4ViewInputControlDemolish::OnMouseUpL(int, int, unsigned int)"},
    {0x1003ccf78u, 62u, "cSC4ViewInputControlDemolish::OnKeyDown(unsigned int, unsigned int)"},
    {0x1003ccfb6u, 42u, "cSC4ViewInputControlDemolish::OnMouseExit()"},
    {0x1003ccfe0u, 60u, "cSC4ViewInputControlDemolish::EndInput()"},
    {0x1003cd01cu, 220u, "cSC4ViewInputControlDemolish::PickDemolishableOccupant(int, int, cISC4Occupant**)"},
    // cS3DCamera (13)
    {0x100163b14u, 252u, "cS3DCamera::cS3DCamera()"},
    {0x100163c10u, 102u, "cS3DCamera::MakeIdentity()"},
    {0x100163da6u, 394u, "cS3DCamera::LookAt(cS3DVector3 const&, cS3DVector3 const&)"},
    {0x100163f30u, 50u, "cS3DCamera::GetLookAt(cS3DVector3&)"},
    {0x100163f6cu, 44u, "cS3DCamera::ViewXform()"},
    {0x100163f98u, 412u, "cS3DCamera::_UpdateXform()"},
    {0x100164166u, 72u, "cS3DCamera::SetOrtho(float, float, float, float, float, float)"},
    {0x1001641bau, 142u, "cS3DCamera::GetViewVolume(float&, float&, float&, float&, float&, float&) const"},
    {0x100164248u, 48u, "cS3DCamera::ProjectionMatrix()"},
    {0x100164278u, 1002u, "cS3DCamera::_UpdateProjection()"},
    {0x100164678u, 602u, "cS3DCamera::GetEyeRay(cS3DVector2 const&, cS3DVector3&, cS3DVector3&)"},
    {0x1001648d2u, 654u, "cS3DCamera::UnProject(cS3DVector3 const&, cS3DVector3&)"},
    {0x100164b60u, 590u, "cS3DCamera::Project(cS3DVector3 const&, cS3DVector3&)"},
    // nsGZLua5CompatWithLua4 (13)
    {0x100696ee0u, 54u, "nsGZLua5CompatWithLua4::NewTag(lua_State*)"},
    {0x100696f16u, 85u, "nsGZLua5CompatWithLua4::SetTag(lua_State*)"},
    {0x100696f6bu, 202u, "nsGZLua5CompatWithLua4::PushTagMetaTable(lua_State*, int)"},
    {0x100697035u, 74u, "nsGZLua5CompatWithLua4::lua_SetTag(lua_State*, int)"},
    {0x10069707fu, 219u, "nsGZLua5CompatWithLua4::SetTagMethod(lua_State*)"},
    {0x10069715au, 439u, "nsGZLua5CompatWithLua4::Lua4EventToMetaMehod(char const*)"},
    {0x100697311u, 193u, "nsGZLua5CompatWithLua4::GetTagMethod(lua_State*)"},
    {0x1006973d2u, 129u, "nsGZLua5CompatWithLua4::PushTagMetaTableMap(lua_State*)"},
    {0x100697453u, 133u, "nsGZLua5CompatWithLua4::Tag(lua_State*)"},
    {0x1006974d8u, 266u, "nsGZLua5CompatWithLua4::CopyTagMethods(lua_State*)"},
    {0x1006975e2u, 21u, "nsGZLua5CompatWithLua4::Globals(lua_State*)"},
    {0x1006975f7u, 66u, "nsGZLua5CompatWithLua4::lua_PushUserTag(lua_State*, void*, int)"},
    {0x100697639u, 295u, "nsGZLua5CompatWithLua4::OpenLib(cIGZLua5*)"},
    // cHitList (12)
    {0x100481d20u, 92u, "cHitList::cHitList()"},
    {0x100481d7cu, 78u, "cHitList::cHitList()"},
    {0x100481ec6u, 80u, "cHitList::Remove(int)"},
    {0x100481fccu, 94u, "cHitList::Add(int)"},
    {0x10048202au, 40u, "cHitList::RemoveAll()"},
    {0x100482052u, 320u, "cHitList::Choose()"},
    {0x100482192u, 120u, "cHitList::Random(int)"},
    {0x10048220au, 100u, "cHitList::GetValueByIndex(unsigned int, int*) const"},
    {0x10048226eu, 98u, "cHitList::SetValueByIndex(unsigned int, int)"},
    {0x1004822d0u, 150u, "cHitList::RemoveByIndex(unsigned int)"},
    {0x100482366u, 1196u, "cHitList::ReadResource(cIGZIStream&, unsigned int, cIGZUnknown*)"},
    {0x100482812u, 282u, "cHitList::WriteResource(cIGZOStream&, unsigned int, cIGZUnknown*)"},
    // cSC4BuildingSelectWinProc (12)
    {0x10075913cu, 70u, "cSC4BuildingSelectWinProc::cSC4BuildingSelectWinProc(cIGZWin*)"},
    {0x100759232u, 118u, "cSC4BuildingSelectWinProc::Shutdown()"},
    {0x100759328u, 584u, "cSC4BuildingSelectWinProc::Init()"},
    {0x100759570u, 602u, "cSC4BuildingSelectWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x1007597cau, 82u, "cSC4BuildingSelectWinProc::SetExpanded(bool)"},
    {0x10075981cu, 190u, "cSC4BuildingSelectWinProc::AddActiveStyle(unsigned int)"},
    {0x1007598dau, 192u, "cSC4BuildingSelectWinProc::RemoveActiveStyle(unsigned int)"},
    {0x10075999au, 136u, "cSC4BuildingSelectWinProc::EnableStyleButtons()"},
    {0x100759a22u, 316u, "cSC4BuildingSelectWinProc::ShowWindow()"},
    {0x100759b5eu, 114u, "cSC4BuildingSelectWinProc::IsStyleActive(unsigned int)"},
    {0x100759bd0u, 18u, "cSC4BuildingSelectWinProc::HideWindow()"},
    {0x100759be2u, 18u, "cSC4BuildingSelectWinProc::IsVisible()"},
    // cSC4ViewInputControlSignTool (11)
    {0x1006ca19cu, 160u, "cSC4ViewInputControlSignTool::cSC4ViewInputControlSignTool()"},
    {0x1006ca23cu, 320u, "cSC4ViewInputControlSignTool::Init()"},
    {0x1006ca37cu, 234u, "cSC4ViewInputControlSignTool::Shutdown()"},
    {0x1006ca466u, 280u, "cSC4ViewInputControlSignTool::SetSelection(cISC4SignpostOccupant*)"},
    {0x1006ca5f6u, 938u, "cSC4ViewInputControlSignTool::OnMouseDownL(int, int, unsigned int)"},
    {0x1006ca9a0u, 278u, "cSC4ViewInputControlSignTool::OnMouseUpL(int, int, unsigned int)"},
    {0x1006caab6u, 510u, "cSC4ViewInputControlSignTool::OnMouseMove(int, int, unsigned int)"},
    {0x1006cacb4u, 294u, "cSC4ViewInputControlSignTool::DoMessage(cGZMessage&)"},
    {0x1006cadecu, 640u, "cSC4ViewInputControlSignTool::OnCharacter(char)"},
    {0x1006cb06cu, 258u, "cSC4ViewInputControlSignTool::AppendText(cRZString&)"},
    {0x1006cb16eu, 360u, "cSC4ViewInputControlSignTool::OnKeyDown(unsigned int, unsigned int)"},
    // cSC4ViewInputControlGameOfLife (10)
    {0x1002fc20au, 194u, "cSC4ViewInputControlGameOfLife::cSC4ViewInputControlGameOfLife()"},
    {0x1002fc2f6u, 458u, "cSC4ViewInputControlGameOfLife::Init()"},
    {0x1002fc574u, 208u, "cSC4ViewInputControlGameOfLife::RefreshMarkedCellView()"},
    {0x1002fc644u, 200u, "cSC4ViewInputControlGameOfLife::Shutdown()"},
    {0x1002fc748u, 42u, "cSC4ViewInputControlGameOfLife::DoMessage(cIGZMessage2*)"},
    {0x1002fc79au, 128u, "cSC4ViewInputControlGameOfLife::OnKeyDown(unsigned int, unsigned int)"},
    {0x1002fc8aau, 14u, "cSC4ViewInputControlGameOfLife::OnKeyUp(unsigned int, unsigned int)"},
    {0x1002fc8b8u, 172u, "cSC4ViewInputControlGameOfLife::OnMouseDownL(int, int, unsigned int)"},
    {0x1002fc9acu, 186u, "cSC4ViewInputControlGameOfLife::OnMouseMove(int, int, unsigned int)"},
    {0x1002fca66u, 40u, "cSC4ViewInputControlGameOfLife::OnMouseUpL(int, int, unsigned int)"},
    // SL (9)
    {0x10004058cu, 960u, "SL::OnAppInit1()"},
    {0x10004094cu, 82u, "SL::OnAppInit2()"},
    {0x1000409aeu, 1188u, "SL::OnCityInit()"},
    {0x100040e52u, 382u, "SL::OnCityShutdown()"},
    {0x100040ffcu, 128u, "SL::OnView3DInit()"},
    {0x10004107cu, 52u, "SL::OnView3DShutdown()"},
    {0x1000410ceu, 18u, "SL::ViewManager()"},
    {0x1000410e0u, 84u, "SL::ATM()"},
    {0x100041134u, 84u, "SL::AEMS()"},
    // cSC4WinSplashScreen (9)
    {0x1002701d0u, 236u, "cSC4WinSplashScreen::cSC4WinSplashScreen()"},
    {0x1002703e4u, 252u, "cSC4WinSplashScreen::SetupScriptWindow()"},
    {0x1002704e0u, 260u, "cSC4WinSplashScreen::Init()"},
    {0x1002705e4u, 156u, "cSC4WinSplashScreen::TransitionTimerCallback(void*)"},
    {0x100270680u, 170u, "cSC4WinSplashScreen::Shutdown()"},
    {0x1002707acu, 58u, "cSC4WinSplashScreen::DoMessage(cIGZMessage2*)"},
    {0x1002707f0u, 38u, "cSC4WinSplashScreen::GZOnMouseDownL(int, int, unsigned int)"},
    {0x100270816u, 46u, "cSC4WinSplashScreen::GZOnKeyDown(unsigned int, unsigned int)"},
    {0x100270844u, 268u, "cSC4WinSplashScreen::GZPaint()"},
    // cS3DShapeFile (9)
    {0x10046d342u, 56u, "cS3DShapeFile::Close()"},
    {0x10046d3c0u, 78u, "cS3DShapeFile::SwapDirectoryAndFrames()"},
    {0x10046d40eu, 238u, "cS3DShapeFile::Open(cIGZPersistDBSerialRecord*)"},
    {0x10046d4fcu, 122u, "cS3DShapeFile::SetCurrentFrameByIndex(unsigned int)"},
    {0x10046d576u, 232u, "cS3DShapeFile::OpenInfoOnly(cIGZPersistDBSerialRecord*)"},
    {0x10046d65eu, 200u, "cS3DShapeFile::Open(unsigned char const*, int)"},
    {0x10046d72cu, 106u, "cS3DShapeFile::GetCurrentFrameInfo(cS3DShapeFile::cFrameInfo&)"},
    {0x10046d796u, 84u, "cS3DShapeFile::GetCurrentFramePaletteInfo(cS3DShapeFile::cPaletteInfo&)"},
    {0x10046d7eau, 182u, "cS3DShapeFile::PixelWidthMul2(unsigned int)"},
    // cS3DModelProcessor (8)
    {0x100108e06u, 218u, "cS3DModelProcessor::operator=(cS3DModelProcessor const&)"},
    {0x100165910u, 92u, "cS3DModelProcessor::cS3DModelProcessor()"},
    {0x10016596cu, 74u, "cS3DModelProcessor::cS3DModelProcessor()"},
    {0x100165b52u, 1252u, "cS3DModelProcessor::SetModel(cIS3DModelInstance*)"},
    {0x100166036u, 52u, "cS3DModelProcessor::ReleaseModel()"},
    {0x10016606au, 338u, "cS3DModelProcessor::LightModel(cISC4LightingManager*, bool)"},
    {0x1001661bcu, 1218u, "cS3DModelProcessor::CreateVertexNormals()"},
    {0x10016667eu, 346u, "cS3DModelProcessor::SortMeshes(cS3DVector3 const&)"},
    // cSCHappinessCurveEntryView (8)
    {0x100528bd0u, 88u, "cSCHappinessCurveEntryView::GZPaint()"},
    {0x100528c28u, 2014u, "cSCHappinessCurveEntryView::Init()"},
    {0x1005294fcu, 482u, "cSCHappinessCurveEntryView::doButtonPress(unsigned int)"},
    {0x1005296deu, 536u, "cSCHappinessCurveEntryView::updateCurveDisplay()"},
    {0x1005298f6u, 90u, "cSCHappinessCurveEntryView::addPoint(short, float)"},
    {0x100529950u, 64u, "cSCHappinessCurveEntryView::deletePoint(short)"},
    {0x1005299acu, 220u, "cSCHappinessCurveEntryView::GetValue(cIGZVariant&)"},
    {0x100529a9au, 334u, "cSCHappinessCurveEntryView::GetPropertyValueString(unsigned int, cIGZVariant const&, cIGZString&)"},
    // cSC4CameraEffect (7)
    {0x100571deeu, 190u, "cSC4CameraEffect::cSC4CameraEffect(cSC4CameraDescription*)"},
    {0x100571fc0u, 48u, "cSC4CameraEffect::DoMessage(cIGZMessage2*)"},
    {0x100572020u, 300u, "cSC4CameraEffect::Start(cISC4VisualEffect::tTransitionType)"},
    {0x10057214cu, 62u, "cSC4CameraEffect::Stop(cISC4VisualEffect::tTransitionType)"},
    {0x100572194u, 96u, "cSC4CameraEffect::SetTransforms(cS3DTransform const&, cS3DTransform const&)"},
    {0x1005721f4u, 190u, "cSC4CameraEffect::SetFloatParams(cISC4VisualEffect::tParamType, float const*, int)"},
    {0x1005722bau, 68u, "cSC4CameraEffect::ApplyEffect(float)"},
    // cSC4GamePreferencesWinProc (6)
    {0x1003a2c64u, 346u, "cSC4GamePreferencesWinProc::DoModalWin()"},
    {0x1003a2dbeu, 206u, "cSC4GamePreferencesWinProc::EndDialog()"},
    {0x1003a2e8cu, 1450u, "cSC4GamePreferencesWinProc::ShowDialog(cSC4GamePreferencesWinProc::eGameWindowType)"},
    {0x1003a3492u, 82u, "cSC4GamePreferencesWinProc::FinalRelease()"},
    {0x1003a34e4u, 154u, "cSC4GamePreferencesWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
    {0x1003a357eu, 1266u, "cSC4GamePreferencesWinProc::HandleControlMessage(unsigned int, cIGZWin*)"},
    // cSCColorSequenceEntryView (6)
    {0x10052d134u, 108u, "cSCColorSequenceEntryView::Create()"},
    {0x10052d1a0u, 1942u, "cSCColorSequenceEntryView::Init()"},
    {0x10052d936u, 528u, "cSCColorSequenceEntryView::RefreshGridRow(int)"},
    {0x10052db46u, 1248u, "cSCColorSequenceEntryView::DoMessage(cGZMessage&)"},
    {0x10052e532u, 166u, "cSCColorSequenceEntryView::SetValue(cIGZVariant const&)"},
    {0x10052e5eau, 268u, "cSCColorSequenceEntryView::GetValue(cIGZVariant&)"},
    // cSLString (5)
    {0x100121286u, 112u, "cSLString::cSLString(cGZPersistResourceKey const&, bool)"},
    {0x1001212f6u, 736u, "cSLString::Load(bool, bool)"},
    {0x1001215e4u, 108u, "cSLString::cSLString(unsigned int, unsigned int, bool)"},
    {0x100121664u, 60u, "cSLString::operator=(cSLString const&)"},
    {0x1001216d0u, 104u, "cSLString::ConvertTokens(cISCStringDetokenizer*, bool)"},
    // cTablePropertyReader (5)
    {0x100298108u, 256u, "cTablePropertyReader::cTablePropertyReader(cISCLua*, char const*, int)"},
    {0x10043daeau, 134u, "cTablePropertyReader::GetString(char const*)"},
    {0x100441360u, 98u, "cTablePropertyReader::GetNumber(char const*)"},
    {0x10044310au, 102u, "cTablePropertyReader::GetNumber(int)"},
    {0x1006aa80cu, 122u, "cTablePropertyReader::HasField(char const*)"},
    // cHitResourceFactory (5)
    {0x10046f73cu, 208u, "cHitResourceFactory::Write(cIGZPersistResource const&, cIGZPersistDBRecord&)"},
    {0x10046f80cu, 852u, "cHitResourceFactory::CreateInstance(unsigned int, unsigned int, void**, unsigned int, cIGZUnknown*)"},
    {0x10046fb86u, 306u, "cHitResourceFactory::CreateInstance(cIGZPersistDBRecord&, unsigned int, void**, unsigned int, cIGZUnknown*)"},
    {0x1004731b0u, 20u, "cHitResourceFactory::CreateInstance(unsigned int, unsigned int, void**)"},
    {0x1004731c4u, 28u, "cHitResourceFactory::CreateInstance(cIGZPersistDBRecord&, unsigned int, void**)"},
    // cSCUint32ComboBoxEntryView (5)
    {0x100525bf2u, 108u, "cSCUint32ComboBoxEntryView::Create()"},
    {0x100525c5eu, 468u, "cSCUint32ComboBoxEntryView::Init()"},
    {0x100525e32u, 230u, "cSCUint32ComboBoxEntryView::populateIndexValue()"},
    {0x100525f18u, 616u, "cSCUint32ComboBoxEntryView::createComboBox(cIGZWinCombo*&, cRZVariant const&)"},
    {0x1005261a0u, 342u, "cSCUint32ComboBoxEntryView::GetValue(cIGZVariant&)"},
    // cSC4ViewInputControlTollBooth (5)
    {0x1006cb4bcu, 48u, "cSC4ViewInputControlTollBooth::cSC4ViewInputControlTollBooth()"},
    {0x1006cb4ecu, 54u, "cSC4ViewInputControlTollBooth::Init()"},
    {0x1006cb522u, 78u, "cSC4ViewInputControlTollBooth::Shutdown()"},
    {0x1006cb570u, 1048u, "cSC4ViewInputControlTollBooth::OnMouseMove(int, int, unsigned int)"},
    {0x1006cb988u, 684u, "cSC4ViewInputControlTollBooth::PickLotAndState(int, int, cSC4ViewInputControlTollBooth::TollCursorState&, unsigned int&, unsigned int&, cS3DVector3&)"},
    // MacTools (5)
    {0x100796c1au, 83u, "MacTools::Sleep(unsigned int)"},
    {0x100796c6du, 57u, "MacTools::PostWindowsEvent(unsigned long, unsigned long, unsigned long)"},
    {0x100796ca6u, 147u, "MacTools::PostPrivateEvent(unsigned long, unsigned long, unsigned long)"},
    {0x100796d39u, 149u, "MacTools::MessageBoxA(__CFString const*, unsigned char)"},
    {0x100796dceu, 66u, "MacTools::MessageBoxA(char const*, unsigned char)"},
    // SCLuaTableRef (4)
    {0x1001e763au, 120u, "SCLuaTableRef::SCLuaTableRef(cISCLua*, int)"},
    {0x1001e76bcu, 424u, "SCLuaTableRef::SCLuaTableRef(cISCLua*, char const*)"},
    {0x1001e786eu, 68u, "SCLuaTableRef::begin()"},
    {0x1001e78b2u, 70u, "SCLuaTableRef::next()"},
    // cRZSlabAllocator (4)
    {0x100410dc2u, 766u, "cRZSlabAllocator<14, 16>::Alloc(unsigned int)"},
    {0x1004110dau, 594u, "cRZSlabAllocator<14, 16>::Free(void*, unsigned int)"},
    {0x1004113f0u, 202u, "cRZSlabAllocator<14, 16>::Clear()"},
    {0x10041156au, 202u, "cRZSlabAllocator<9, 14>::Clear()"},
    // cHitSegmentDataSourceChannelCreator (4)
    {0x10043ba86u, 426u, "cHitSegmentDataSourceChannelCreator::cHitSegmentDataSourceChannelCreator()"},
    {0x10043bcaau, 322u, "cHitSegmentDataSourceChannelCreator::Create(unsigned int, unsigned int, cIGZVariant*, unsigned int, void**)"},
    {0x10043be24u, 70u, "cHitSegmentDataSourceChannelCreator::RegisterSoundDef(unsigned int, cIGZString const&, unsigned int, cIGZVariant const*)"},
    {0x10043be6au, 110u, "cHitSegmentDataSourceChannelCreator::RegisterTrackDefParser(cIGZSndDefParser*)"},
    // cGXCOMDirector (4)
    {0x1004dc172u, 140u, "cGXCOMDirector::cGXCOMDirector()"},
    {0x1004dc1feu, 56u, "cGXCOMDirector::CreateGimexFactory()"},
    {0x1004dc236u, 56u, "cGXCOMDirector::CreateGimexReader()"},
    {0x1004dc26eu, 70u, "cGXCOMDirector::CreateGimexWriter()"},
    // cSC4WinGameDelay (3)
    {0x10003aa12u, 110u, "cSC4WinGameDelay::cSC4WinGameDelay(unsigned int, cSC4WinGameDelay::DelayType)"},
    {0x10003aa80u, 64u, "cSC4WinGameDelay::Init()"},
    {0x10003aac0u, 88u, "cSC4WinGameDelay::GZPaint()"},
    // cSLCursor (3)
    {0x100043ee0u, 126u, "cSLCursor::cSLCursor(cGZPersistResourceKey const&)"},
    {0x100043f5eu, 510u, "cSLCursor::Load()"},
    {0x100044166u, 122u, "cSLCursor::cSLCursor(unsigned int, unsigned int)"},
    // cSC4WinRegion3DView (3)
    {0x10023d4d2u, 88u, "cSC4WinRegion3DView::Init()"},
    {0x10023d52au, 52u, "cSC4WinRegion3DView::Shutdown()"},
    {0x10023d55eu, 66u, "cSC4WinRegion3DView::GZPaint()"},
    // SFX (3)
    {0x1003ba4b2u, 74u, "SFX::Listener()"},
    {0x1003efc28u, 69u, "SFX::SoundManager()"},
    {0x10040d1bau, 60u, "SFX::AmbienceManager()"},
    // cSC4WinIMECandidateList (3)
    {0x10046421eu, 86u, "cSC4WinIMECandidateList::cSC4WinIMECandidateList()"},
    {0x100464354u, 632u, "cSC4WinIMECandidateList::Init()"},
    {0x1004645deu, 1568u, "cSC4WinIMECandidateList::SetupWindow(cGZIMECandidateListData*)"},
    // cHitSem2dDefault (3)
    {0x1004c25bcu, 204u, "cHitSem2dDefault::cHitSem2dDefault()"},
    {0x1004d1510u, 226u, "cHitSem2dDefault::UpdateVolPan(cIGZSndRegisterSet*, cIGZSndRegisterSet*) const"},
    {0x1004d15f2u, 380u, "cHitSem2dDefault::ConfigureChannel(cIGZSndRegisterSet*, cIGZSndRegisterSet*) const"},
    // cSC24BitRGBColorCustomizer (3)
    {0x10052ea2eu, 102u, "cSC24BitRGBColorCustomizer::Create()"},
    {0x10052eb5eu, 118u, "cSC24BitRGBColorCustomizer::GetPropertyValueString(unsigned int, cIGZVariant const&, cIGZString&)"},
    {0x10052ec4au, 52u, "cSC24BitRGBColorCustomizer::ValueIsValid(unsigned int, cIGZVariant const&)"},
    // HTTPMessage (3)
    {0x100634b20u, 248u, "HTTPMessage::GetMessageHeaderEntryCopy(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>*, unsigned int) const"},
    {0x100634c18u, 264u, "HTTPMessage::GetMessageHeaderEntryCopy(int, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>*, unsigned int) const"},
    {0x100634d40u, 1162u, "HTTPMessage::GetSeparatedStringList(std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>> const&, std::__1::basic_string<char, std::__1::char_traits<char>, std::__1::allocator<char>>*, unsigned int, bool, char, bool)"},
    // cSCStringDetokenizerDefault (2)
    {0x10005e5c2u, 558u, "cSCStringDetokenizerDefault::Init()"},
    {0x10005e812u, 438u, "cSCStringDetokenizerDefault::DefaultReplacementFunction(cIGZString const&, cIGZString&)"},
    // tControllerTime (2)
    {0x1000f5aaeu, 100u, "tControllerTime::operator+=(float)"},
    {0x1000f5d7cu, 252u, "tControllerTime::Normalize()"},
    // cAmbienceFilterIsDay (2)
    {0x1003def40u, 38u, "cAmbienceFilterIsDay::Create()"},
    {0x1003e48b6u, 24u, "cAmbienceFilterIsDay::IsTrue()"},
    // cAmbienceFilterIsZoom3 (2)
    {0x1003defd8u, 38u, "cAmbienceFilterIsZoom3::Create()"},
    {0x1003e492au, 46u, "cAmbienceFilterIsZoom3::IsTrue()"},
    // cAmbienceFilterIsSubwayView (2)
    {0x1003df096u, 38u, "cAmbienceFilterIsSubwayView::Create()"},
    {0x1003e49dcu, 32u, "cAmbienceFilterIsSubwayView::IsTrue()"},
    // cAmbienceFilterIsSimSpeedMedium (2)
    {0x1003df12eu, 38u, "cAmbienceFilterIsSimSpeedMedium::Create()"},
    {0x1003e4a70u, 32u, "cAmbienceFilterIsSimSpeedMedium::IsTrue()"},
    // cAmbienceCellFilterIsRoad (2)
    {0x1003df7e0u, 50u, "cAmbienceCellFilterIsRoad::Create()"},
    {0x1003e5cfeu, 116u, "cAmbienceCellFilterIsRoad::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsWater (2)
    {0x1003df8a8u, 50u, "cAmbienceCellFilterIsWater::Create()"},
    {0x1003e5fb6u, 106u, "cAmbienceCellFilterIsWater::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsAirport (2)
    {0x1003df970u, 50u, "cAmbienceCellFilterIsAirport::Create()"},
    {0x1003e613au, 92u, "cAmbienceCellFilterIsAirport::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsLowGrowthStage (2)
    {0x1003dfa6au, 50u, "cAmbienceCellFilterIsLowGrowthStage::Create()"},
    {0x1003e6472u, 256u, "cAmbienceCellFilterIsLowGrowthStage::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsMediumCrime (2)
    {0x1003dfb32u, 50u, "cAmbienceCellFilterIsMediumCrime::Create()"},
    {0x1003e67f2u, 112u, "cAmbienceCellFilterIsMediumCrime::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsHighDesirability (2)
    {0x1003dfbfau, 50u, "cAmbienceCellFilterIsHighDesirability::Create()"},
    {0x1003e69aeu, 110u, "cAmbienceCellFilterIsHighDesirability::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsAgriculture (2)
    {0x1003dfcc2u, 50u, "cAmbienceCellFilterIsAgriculture::Create()"},
    {0x1003e6a1cu, 302u, "cAmbienceCellFilterIsAgriculture::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsBuilding (2)
    {0x1003dfd8au, 50u, "cAmbienceCellFilterIsBuilding::Create()"},
    {0x1003e6eacu, 116u, "cAmbienceCellFilterIsBuilding::IsTrueCell(int, int)"},
    // cAmbienceCellFilterIsMediumWealth (2)
    {0x1003dfe52u, 50u, "cAmbienceCellFilterIsMediumWealth::Create()"},
    {0x1003e638eu, 114u, "cAmbienceCellFilterIsMediumWealth::IsTrueCell(int, int)"},
    // cADVString (2)
    {0x1003faf80u, 70u, "cADVString::cADVString(char const*)"},
    {0x1003fafc6u, 182u, "cADVString::init(char const*, int)"},
    // cSourceFileDebugInfo (2)
    {0x100485a74u, 142u, "cSourceFileDebugInfo::cSourceFileDebugInfo(cIGZString&, unsigned char*, int)"},
    {0x100485bccu, 84u, "cSourceFileDebugInfo::LineNumFromOffset(int)"},
    // cSCSint64TextEntryView (2)
    {0x100523ff4u, 416u, "cSCSint64TextEntryView::Init()"},
    {0x1005241b4u, 450u, "cSCSint64TextEntryView::GetValue(cIGZVariant&)"},
    // cSCUint8TextEntryView (2)
    {0x10052525au, 420u, "cSCUint8TextEntryView::Init()"},
    {0x10052541eu, 446u, "cSCUint8TextEntryView::GetValue(cIGZVariant&)"},
    // cS3DMaterialManager (2)
    {0x100569fa2u, 234u, "cS3DMaterialManager::CreateMaterial(cS3DResMaterial const&, cS3DMaterial**)"},
    {0x10056a0a4u, 44u, "cS3DMaterialManager::GetStatsString(cIGZString&)"},
    // cSC4SequenceDescription (2)
    {0x100572a5cu, 86u, "cSC4SequenceDescription::cSC4SequenceDescription()"},
    {0x100572b62u, 72u, "cSC4SequenceDescription::Instantiate()"},
    // cSC4ParticlesDescriptionBase (2)
    {0x1005b4eb0u, 1366u, "cSC4ParticlesDescriptionBase::cSC4ParticlesDescriptionBase()"},
    {0x1005b544au, 1218u, "cSC4ParticlesDescriptionBase::operator=(cSC4ParticlesDescriptionBase const&)"},
    // cSTEChangeTerrainUI (2)
    {0x10072690au, 408u, "cSTEChangeTerrainUI::DoDialog(int&)"},
    {0x100726aa2u, 112u, "cSTEChangeTerrainUI::DoMessage(cIGZWin*, cGZMessage&)"},
    // SC4AudioPreferences (1)
    {0x10001384au, 126u, "SC4AudioPreferences::SC4AudioPreferences()"},
    // cPortTypeConfig (1)
    {0x1000cd79eu, 932u, "cPortTypeConfig::LoadConfiguration(int, cISCPropertyHolder*)"},
    // cAnimatedSpriteTableData (1)
    {0x1001a6240u, 324u, "cAnimatedSpriteTableData::Init(cIS3DResSpriteAnimTable*)"},
    // RandomIterator (1)
    {0x100213f58u, 438u, "RandomIterator::Reset(bool)"},
    // cStoplightPropFilter (1)
    {0x1002a07d2u, 80u, "cStoplightPropFilter::IsOccupantIncluded(cISC4Occupant*)"},
    // MiscGraphLine (1)
    {0x10037bd06u, 118u, "MiscGraphLine::DrawSelf(cIGZGraph*, cIGZBuffer*)"},
    // cGZAllocatorServiceSTLAllocator (1)
    {0x1003ddbeau, 110u, "cGZAllocatorServiceSTLAllocator<unsigned int>::AS()"},
    // cS3DFSHTexture (1)
    {0x10046611cu, 138u, "cS3DFSHTexture::cS3DFSHTexture()"},
    // TickerMessage (1)
    {0x100513de0u, 256u, "TickerMessage::TickerMessage(unsigned int, cIGZString*, cIGZFont*, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, cGZMessage const*, void (*)(unsigned int, void*), void*)"},
    // cSCLuaCOMDirector (1)
    {0x100566730u, 160u, "cSCLuaCOMDirector::cSCLuaCOMDirector()"},
    // cSC4WinText (1)
    {0x1005c333cu, 1876u, "cSC4WinText::GZPaint()"},
    // cSC4MadCowPlayerWin (1)
    {0x1006371f6u, 46u, "cSC4MadCowPlayerWin::SuspendMovie()"},
    // CColorArray (1)
    {0x10063add8u, 56u, "CColorArray::SetPointer(int, unsigned int, int, void const*)"},
    // cRemoveSubTreeVisitor (1)
    {0x1006c16d2u, 62u, "cRemoveSubTreeVisitor::sVisit(unsigned long, void*)"},
    // cSC4RegionSignpostAnimatorWinProc (1)
    {0x10075a442u, 352u, "cSC4RegionSignpostAnimatorWinProc::DoWinProcMessage(cIGZWin*, cGZMessage&)"},
};

inline constexpr Uint32 kWave4Part3MethodCount =
    sizeof(kWave4Part3Methods) / sizeof(kWave4Part3Methods[0]);

static_assert(kWave4Part3MethodCount == 808u);
static_assert(kWave4Part3Methods[0].address == 0x10077efc2u);
static_assert(kWave4Part3Methods[kWave4Part3MethodCount - 1u].address == 0x10075a442u);

inline constexpr bool Wave4Part3MethodsWellFormed() noexcept {
    if (kWave4Part3MethodCount != 808u) {
        return false;
    }
    for (Uint32 i = 0; i < kWave4Part3MethodCount; ++i) {
        Wave4Part3Method const& method = kWave4Part3Methods[i];
        if (method.address < 0x100000000ull || method.address > 0x100ffffffull) {
            return false;
        }
        if (method.sizeBytes == 0u || method.name == nullptr || method.name[0] == '\0') {
            return false;
        }
    }
    return true;
}

static_assert(Wave4Part3MethodsWellFormed());
} // namespace opensc4
