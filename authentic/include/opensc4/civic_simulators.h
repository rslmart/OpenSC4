#pragma once

// Exact decompilations of the civic & utility simulator save records from the
// shipped 64-bit macOS binary (Sim City 4 Deluxe Editionsub), plus their
// ReadSimulatorTunableValues exemplar layouts.
//
// Every Read/Write below mirrors the binary's cIGZIStream/cIGZOStream call
// sequence (vtable +0x20=Uint8, +0x28=Bool, +0x30=Sint16, +0x38=Uint16,
// +0x40=Sint32, +0x48=Uint32, +0x50=Sint64, +0x58=Uint64, +0x60=Float32,
// +0x68=Float64, +0x78=GZString). Version constants are the values the
// binary's Write stores. Binary member offsets are given in comments.
//
// Shared serialized forms:
//  * SC4ReadGZSerializable (0x100043267): Uint32 memAddr, then Uint32 clsid
//    only when memAddr != 0. Modeled by SimSerializableRef (same layout as
//    the serializable-ref convention used by the budget/tract records).
//  * ilist<cISC4Occupant*> (operator>> 0x1000cfb37): Uint32 count (binary
//    rejects > 0x98967f), then count serializable refs (IID 0x26bb767e).
//  * RandomIterator (operator>> 0x1000cea88): Uint8, Uint8, Bool,
//    5 x Sint32, Uint32 count, count x Uint16, 3 x Sint32 (at +0x30/+0x34/
//    +0x38; the first four Sint32s form an embedded cRZRect on the Write
//    side, operator<< 0x1000ceb93).
//  * ScanningIterator (operator>> 0x1000cec88): Uint8, Sint32, Sint32,
//    Uint32, 4 x Sint32 (29 serialized bytes).
//  * 3 x Sint32 list entries (Flammability/Aura): Uint32 count, then
//    count x (Sint32, Sint32, Sint32).

#include "opensc4/formats.h"
#include "opensc4/gzcom.h"
#include "opensc4/simgrid.h"
#include "opensc4/rz_random.h"
#include "opensc4/pollution_grid_kernel.h"

#include <array>
#include <cstddef>
#include <span>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace opensc4 {

class cSC4ResponseCurve;

// Serialized form of a GZCOM serializable reference (see header comment).
struct SimSerializableRef {
    Uint32 memAddr = 0;
    Uint32 clsid   = 0;

    bool operator==(const SimSerializableRef&) const noexcept = default;
};

using OccupantRefList = std::vector<SimSerializableRef>;

// RandomIterator @ 0x1000cea88 (cSC4CrimeSimulator +0x28,
// cSC4PollutionSimulator +0x2e8, cSC4ProtectionSimulator +0x40).
// 20-byte runtime iterator state for the intrinsic-grid walk (binary layout:
// status +0x00, x +0x04, z +0x08, step +0x0c, minX +0x10, minZ +0x14,
// maxX +0x18, maxZ +0x1c). Reset @ 0x1000c5eb3.
struct IntrinsicGridIteratorState {
    Uint8 status = 0;
    Sint32 x = 0;
    Sint32 z = 0;
    Sint32 step = 1;
    Sint32 minX = 0;
    Sint32 minZ = 0;
    Sint32 maxX = 0;
    Sint32 maxZ = 0;
};

// Exact RandomIterator::Reset(bool) @ 0x100213f58.
struct RandomIteratorState {
    Uint8 field0 = 0;              // +0x00
    Uint8 field1 = 0;              // +0x01
    bool field2  = false;          // +0x02
    std::array<Sint32, 5> values{}; // +0x04, +0x08, +0x0c, +0x10, +0x14
    std::vector<Uint16> indices;   // Uint32 count @ +0x18, then Uint16 items
    std::array<Sint32, 3> tailValues{}; // +0x30, +0x34, +0x38

    // Exact RandomIterator::Reset(bool) @ 0x100213f58.
    void Reset(bool randomize);
};

// LinearRandomIterator is a transient 0x30-byte stack object, not an
// independently serialized type. For example, cSC4TrafficSimulator::
// CheckForAccident @ 0x1000840d8 constructs it at rbp-0x58, calls Reset, and
// destroys its vector before returning. The binary has no LinearRandomIterator
// stream operators or CLSID, so there is deliberately no save-registry entry.
struct LinearRandomIteratorState {
    Uint8 field0 = 0;                 // +0x00
    Uint8 field1 = 0;                 // +0x01
    bool field2  = false;             // +0x02
    std::vector<Uint32> indices;      // +0x08..+0x1f
    Sint32 count = 0;                 // +0x20
    std::array<Sint32, 3> tailValues{}; // +0x24, +0x28, +0x2c

    // Exact LinearRandomIterator::Reset(bool) @ 0x10008daf0.
    void Reset(bool randomize);
};

// ScanningIterator @ 0x1000cec88 (cSC4PlumbingSimulator +0x1b0,
// cSC4ResidentialSimulator +0x56c, cSC4TrafficSimulator *+0xa38).
struct ScanningIteratorState {
    Uint8 field0 = 0;   // +0x00
    Sint32 field4 = 0;  // +0x04
    Sint32 field8 = 0;  // +0x08
    Uint32 fieldC = 0;  // +0x0c
    Sint32 field10 = 0; // +0x10
    Sint32 field14 = 0; // +0x14
    Sint32 field18 = 0; // +0x18
    Sint32 field1C = 0; // +0x1c
};

// BoundaryIterator @ 0x1000ce99f / 0x1000cea25 (cSC4TrafficSimulator *+0xb60,
// 30 serialized bytes: 2 x Uint8, 3 x Sint32, then cRZRect = 4 x Sint32).
struct BoundaryIteratorState {
    Uint8 field0 = 0;                 // +0x00
    Uint8 field1 = 0;                 // +0x01
    Sint32 field4 = 0;                // +0x04
    Sint32 field8 = 0;                // +0x08
    Sint32 fieldC = 0;                // +0x0c
    std::array<Sint32, 4> rect10{};   // +0x10..+0x1c
};

// Serialized cSCProperty / cSCPropertyHolder entry (cSCPropertyHolder::Read
// @ 0x100049d3c, Write @ 0x100049cc4; cSCProperty::Read @ 0x100049648, Write
// @ 0x100049610; cSC4DBSegmentIStream::ReadVariant @ 0x1003cdf3e,
// cGZPersistDBSerialRecord::GetFieldVariant @ 0x10038beea).
struct SerializedPropertyEntry {
    Uint32 holderPropId = 0;          // written by cSCPropertyHolder
    Uint32 propertyId = 0;            // written by cSCProperty (+0x20)
    Uint32 variantTag = 0;            // 0xa9bee9ac = serializable ref, 0 = DB variant
    SimSerializableRef unknownRef{};  // valid when variantTag == 0xa9bee9ac
    Uint32 dbType = 0;                // 0..0x0c scalar or 0x8000..0x8011 array
    Uint32 arrayCount = 0;            // valid when (dbType & 0x8000) && dbType != 0x800c
    std::string stringValue;          // valid when dbType == 0x800c
    std::vector<Uint8> rawValueBytes; // scalar or array element payload bytes
};

struct SerializedPropertyHolder {
    Uint16 version = 2;               // must == 2 in cSCPropertyHolder::Read
    std::vector<SerializedPropertyEntry> properties;
};

// SC4ReadGZSerializable / SC4WriteGZSerializable (Uint32 memAddr, then Uint32
// clsid only when memAddr != 0).
bool ReadSimSerializableRef(cIGZIStream& stream, SimSerializableRef& ref) noexcept;
void WriteSimSerializableRef(cIGZOStream& stream, const SimSerializableRef& ref);
// cSCPropertyHolder::Read @ 0x100049d3c / Write @ 0x100049cc4.
bool ReadSCPropertyHolder(cIGZIStream& stream, SerializedPropertyHolder& holder) noexcept;
void WriteSCPropertyHolder(cIGZOStream& stream, const SerializedPropertyHolder& holder);

// (Sint32, Sint32, Sint32) list node payload (binary list nodes are 0x20
// bytes storing the triple at +0x10).
struct Sint32Triple {
    Sint32 a = 0;
    Sint32 b = 0;
    Sint32 c = 0;
};

// ---------------------------------------------------------------------------
// cSC4ProtectionSimulator (base of Police/FireProtection). Read 0x1000aa362,
// Write 0x1000aa25a. Reads/writes its OWN Uint16 version (must be <= 3;
// Write stores 3). Offsets are relative to the base sub-object (which sits
// at +8 inside both derived simulators).
// ---------------------------------------------------------------------------
struct ProtectionSimulatorSaveRecord {
    Uint16 version = 3;
    SimSerializableRef simManagerRef;      // +0xa0, IID 0x223e2b67
    RandomIteratorState randomIterator;    // +0x40
    SimSerializableRef gridRefA;           // +0xa8, IID 0xc9bd939a
    SimSerializableRef gridRefB;           // +0xb0, IID 0xc9bd939a
    OccupantRefList occupants;             // +0xb8
    Sint32 fieldD0 = 0;                    // +0xd0
    Sint32 fieldD4 = 0;                    // +0xd4
    Sint32 fieldD8 = 0;                    // +0xd8
    Float32 fieldDC = 0.0f;                // +0xdc
    Float32 fieldE0 = 0.0f;                // +0xe0 (x0.01 in stream if version < 3)
    bool fieldE8 = false;                  // +0xe8
    Uint32 fieldEC = 0;                    // +0xec
    Uint32 fieldF0 = 0;                    // +0xf0

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;

    // Exact cSC4ProtectionSimulator::ProtectionEffect layout (+0x00 x, +0x04 z,
    // +0x08 radius, +0x0c centerStrength, +0x10 exteriorStrength, +0x14 rawCenterStrength).
    struct ProtectionEffect {
        Float32 worldX = 0.0f;         // +0x00
        Float32 worldZ = 0.0f;         // +0x04
        Float32 radius = 0.0f;         // +0x08
        Sint32 centerStrength = 0;     // +0x0c
        Sint32 exteriorStrength = 0;   // +0x10
        Sint32 rawCenterStrength = 0;  // +0x14
    };

    // Exact cSC4ProtectionSimulator methods:
    //   cSC4ProtectionSimulator() @ 0x1000a9b10, ~cSC4ProtectionSimulator() @ 0x1000a9c2c /
    //   0x1000a9c8c / 0x1000a9c92, ProtectionInit @ 0x1000a9c98,
    //   ProtectionShutdown @ 0x1000aa060, DoMessage @ 0x1000aa4d6,
    //   DoDispatchActivated @ 0x1000aa760, DoDispatchEnd @ 0x1000aa7c4,
    //   DoMonthlySimulation @ 0x1000aa804, DoDailySimulation @ 0x1000aa80a,
    //   IsProtectionGridCellZoned @ 0x1000aaa58, DispatchFromStation @ 0x1000aaab8,
    //   ReturnDispatchToStation @ 0x1000aab78, AdjustProtectionEffect @ 0x1000aac14,
    //   UpdateBothGridsByPosition @ 0x1000aac4e, CanDispatch @ 0x1000aacf6,
    //   GetStations @ 0x1000aadb6, CopyOccupantList @ 0x1000aadce,
    //   GetMaxPossibleDispatchCount @ 0x1000aae64,
    //   DispatchFromNearestStation @ 0x1000aaee0,
    //   RevokeDispatchFromStation @ 0x1000ab03c,
    //   DoOccupantInserted @ 0x1000ab054, DoOccupantRemoved @ 0x1000ab31c,
    //   DoOccupantChanged @ 0x1000ab444, UpdateUnadjustedProtectionGrid @ 0x1000ab4ca,
    //   CalculateAndWriteAdjustedGridDataByCell @ 0x1000ab812.
    static void AdjustProtectionEffect(ProtectionEffect& effect,
                                       Float32 efficiency) noexcept;
    void FinalizeDailyCoverageCycle() noexcept;
    void AccumulateCellCoverage(Uint8 adjustedCellValue, bool isZoned) noexcept;

    // Exact cSC4ProtectionSimulator::CalculateAndWriteAdjustedGridDataByCell @ 0x1000ab812.
    static Uint8 CalculateAdjustedProtectionCell(Sint16 unadjustedValue,
                                                 Float32 adjustmentMultiplier) noexcept;
    static void CalculateAdjustedProtectionGrid(std::span<const Sint16> unadjustedGrid,
                                                Float32 adjustmentMultiplier,
                                                std::span<Uint8> outAdjustedGrid) noexcept;
};

// ---------------------------------------------------------------------------
// cSC4LandValueSimulator, CLSID 0xe990c18e. Read 0x1000c5c1a (version must
// == 1), Write 0x1000c5b7a. Tuning instance 0xe7e2c8d8.
// ---------------------------------------------------------------------------
struct LandValueTuning;
struct LandValueSimulatorSaveRecord {
    Uint16 version = 1;
    Uint32 memAddr = 0;
    SimSerializableRef gridRef40; // +0x40, IID 0xc9bd939a (stream order 1st)
    SimSerializableRef gridRef48; // +0x48, IID 0xc9bd939a (2nd)
    SimSerializableRef gridRef58; // +0x58, IID 0xc9bd939a (3rd)
    SimSerializableRef gridRef50; // +0x50, working intrinsic land-value grid (binary: created in Init @ 0x1000c50fa, tract-size 2, 64x64)
    Uint8 idleState = 0;           // +0x68 (runtime state machine byte; jump table @ 0x1000c6d10: states 0-8, state 9 returns)
    IntrinsicGridIteratorState intrinsicIterator{}; // +0x70 (intrinsic-grid iterator; reset @ 0x1000c5eb3)
    const LandValueTuning* tuning = nullptr;  // runtime tuning pointer (fieldD0 tract size @ 0x47e2c200)
    Uint8 fieldD1 = 0;                 // +0xd1 (tract fill value; binary reads [this+0xd1] @ 0x1000c5fee; tuning byte-pair 0x47e2c300/301/310 loaded by Init/integration wiring)
    Uint8 fieldD5 = 0;                 // +0xd5 (near strength for State 2; binary converts to Float32 @ 0x1000c606f)
    Uint8 fieldD6 = 0;                 // +0xd6 (maximum distance for State 2; binary converts to Float32 @ 0x1000c6062)
    cSC4SimGrid<Uint8>* intrinsicGrid = nullptr; // +0x40 grid object pointer (State 2/3 read tract counts; State 4 writes the altitude-applied result @ 0x1000c64d6-0x1000c64dd)
    cSC4SimGrid<Uint8>* workingGrid = nullptr;   // +0x50 grid object pointer (binary: created in Init @ 0x1000c50fa, tract-size 2, 64x64)
    const cSC4SimGrid<Uint8>* waterGrid = nullptr;  // runtime terrain water grid (TractHasLand gate @ 0x1000c6dd8 samples it via CellHasWater @ 0x10073562c)
    const cSC4SimGrid<Sint8>* zoneGrid = nullptr; // runtime zone grid read by cSC4ZoneManager::GetZoneType @ 0x100199e64
    cSC4SimGrid<Uint8>* landValueGrid = nullptr; // +0x48 grid object pointer (State 8 averages it @ 0x1000c6893-0x1000c68ab; State 7 empty-list fallback copies +0x50 into it @ 0x1000c6909)
    cSC4SimGrid<Float32>* slopeGrid = nullptr; // +0x58 float slope grid object (State 5 fills it @ 0x1000c65a6-0x1000c667a; GetTractShift via vtable +0x58 @ 0x1000c65c0)
    const cSC4ResponseCurve* altitudeCurve = nullptr; // +0xd8 altitude response curve (State 4 null-guard @ 0x1000c6423-0x1000c642c; consumed by LookupAltitudeEffect @ 0x1000c6f32)
    using TerrainAltitudeSampler = Float32 (*)(const void*, Float32, Float32) noexcept;
    using TerrainAltitudeLimit = Float32 (*)(const void*) noexcept;
    const void* terrainContext = nullptr;
    TerrainAltitudeSampler terrainGetAltitude = nullptr; // cSTETerrain::GetAltitude @ 0x100738928 (virtual +0x60)
    TerrainAltitudeLimit terrainGetMinAltitude = nullptr; // terrain virtual +0xb8, called @ 0x1000c60b2
    TerrainAltitudeLimit terrainGetMaxAltitude = nullptr; // terrain virtual +0xc0, called @ 0x1000c609a / 0x1000c6ee5
    IntrinsicGridIteratorState slopeIterator{}; // +0x78 (reset when State 3 completes @ 0x1000c6b63)
    Sint32 altitudeSum80 = 0;          // +0x80 (float-add/truncate accumulation @ 0x1000c62d5-0x1000c62ea)
    Sint32 altitudeSampleCount84 = 0;  // +0x84
    bool altitudeFallback88 = false;   // +0x88 (all-land second pass @ 0x1000c6ab7-0x1000c6c1b)
    std::vector<Sint32> altitudeHistogram90; // +0x90/+0x98/+0xa0 (vector begin/end/capacity)
    Sint32 fieldA8 = 5;                // +0xa8 (histogram shift; Init store @ 0x1000c5477)
    bool field60 = false;         // +0x60
    void* growthDeveloperCursor = nullptr; // +0xc0 (State 7 traversal cursor; State 6 arms it from +0xe8 @ 0x1000c66e0-0x1000c66e8, State 7 advances it)
    Uint8 fieldC8 = 0;            // +0xc8 (single byte via the +0x28 slot;
                                  // shipped saves hold values like 0x1d)
    Float32 fieldCC = 0.0f;       // +0xcc
    void* growthDeveloperHead = nullptr; // +0xe8 (map begin iterator; State 6 copies it to +0xc0)
    // State 7 growth-developer map (std::map at [this+0xf0], iterated in key
    // (pointer) order). Node layout: +0x00 left, +0x08 right, +0x10 parent,
    // +0x20 developer (cSC4GrowthDeveloper*), +0x28 Float32 weight. These hooks
    // are injection points pending the real GZCOM wiring; semantics are
    // binary-exact (extracted from DoIdle @ 0x1000c6706-0x1000c6972).
    using LandValueGrowthDevRead = void (*)(void* cursor, void** developer, Float32* weight) noexcept; // node +0x20 / +0x28
    using LandValueGrowthDevAdvance = void* (*)(void* cursor) noexcept;                                 // ++iterator @ 0x1000c6753-0x1000c6972
    using LandValueGrowthDevDesirability = const cSC4SimGrid<Sint16>* (*)(void* developer) noexcept;    // GetDesirabilityMap @ 0x10015b188 (developer +0x1f8)
    void* growthDeveloperEnd = nullptr;               // &[this+0xf0] end sentinel
    const cSC4ResponseCurve* growthCurve = nullptr;   // [this+0xe0] desirability curve
    LandValueGrowthDevRead readGrowthDeveloper = nullptr;
    LandValueGrowthDevAdvance advanceGrowthDeveloper = nullptr;
    LandValueGrowthDevDesirability getGrowthDeveloperDesirabilityMap = nullptr;
    // State 8 non-grid side effects (bodies live in other classes; DoIdle only
    // sequences them, so they are injected for determinism).
    using LandValueHistoryEmit = void (*)(void* context, Uint32 historyId, Uint8 value) noexcept;
    using LandValueCycleEnd = void (*)(void* owner, Uint32 messageId) noexcept;
    void* historyContext = nullptr;
    LandValueHistoryEmit emitHistory = nullptr;       // SL::spHistoryWarehouse vtable +0x60 @ 0x1000c6a2d (id 0x6a663ac8)
    void* cycleOwner = nullptr;                       // [this+0x20] DoEndOfSimulationCycle dispatch target
    LandValueCycleEnd endOfSimulationCycle = nullptr; // DoEndOfSimulationCycle @ 0x1000c738e (sends msg 0x87cc75b3)
    // State 5 runtime: lot-developer discovery and the terrain vertex-altitude
    // query used by GetAreaSlope.
    using LandValueLotDeveloperAccessor = void* (*)(void* context) noexcept; // SL::spBuildingDevelopmentSim vtable +0x120 GetLotDeveloper @ 0x1000c65d2
    using LandValueLotVertexAltitude = Float32 (*)(void* lotDeveloper, Sint32 x, Sint32 z, bool flag) noexcept; // cSC4LotDeveloper vtable +0x78 @ 0x1000c647e/0x1000c648b
    void* buildingDevContext = nullptr;
    LandValueLotDeveloperAccessor getLotDeveloper = nullptr;
    LandValueLotVertexAltitude getLotVertexAltitude = nullptr;
    // SC4Rect<int> (rect field order +0x00 minX, +0x04 minZ, +0x08 maxX, +0x0c maxZ),
    // as built by State 5 @ 0x1000c662b-0x1000c6650.
    struct LandValueIntRect {
        Sint32 minX = 0;
        Sint32 minZ = 0;
        Sint32 maxX = 0;
        Sint32 maxZ = 0;
    };

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;

    // Exact cSC4LandValueSimulator::GetLandValueType @ 0x1000c73fc:
    // returns 1 (Low), 2 (Medium), or 3 (High) for a cell's Uint8 land value.
    // Also: Init @ 0x1000c50fa, Shutdown @ 0x1000c5968, DoMessage @ 0x1000c5dac,
    // DoIdle @ 0x1000c5e64, TractIsZoned @ 0x1000c6d54, TractHasLand @ 0x1000c6dd8,
    // SetupAltitudeEffect @ 0x1000c6e66, LookupAltitudeEffect @ 0x1000c6f32,
    // DoEndOfSimulationCycle @ 0x1000c738e, GetLandValue @ 0x1000c7434.
    Float32 midAltAC = 0.0f;       // +0xac
    Float32 lowerSpanB0 = 1.0f;    // +0xb0
    Float32 upperSpanB4 = 1.0f;    // +0xb4
    Sint32 maxEffectIndexB8 = 100; // +0xb8
    Float32 minAlt100 = 0.0f;      // +0x100
    Float32 maxAlt104 = 0.0f;      // +0x104
    Float32 modeAlt108 = 0.0f;     // +0x108

    void DoIdle(unsigned int idleStep) noexcept; // @ 0x1000c5e64 (jump table @ 0x1000c6d10, states 0-8; state 9 returns)
    static bool TractIsZoned(const cSC4SimGrid<Uint8>& intrinsicGrid,
                             const cSC4SimGrid<Sint8>& zoneGrid,
                             Sint32 tractX, Sint32 tractZ) noexcept; // @ 0x1000c6d54 (any-cell, half-open tract bounds)
    static bool TractHasLand(const cSC4SimGrid<Uint8>& waterGrid,
                             Uint32 tractX, Uint32 tractZ, Uint32 tractSize) noexcept; // @ 0x1000c6dd8 (any-cell: inclusive tractSize+1 bounds; out-of-range CellHasWater @ 0x10073562c returns false = land)
    static Uint32 GetLandValueTypeForValue(Uint8 landValue, Uint8 lowMaxThreshold,
                                            Uint8 medMaxThreshold) noexcept;
    void SetupAltitudeEffect(Float32 terrainMaxAltitude) noexcept;
    Uint8 LookupAltitudeEffect(Float32 altitude,
                               const cSC4ResponseCurve& altitudeCurve) const noexcept;
    // Exact cSC4LotDeveloper::GetAreaSlope @ 0x1001478e8 (LengthOfLine2D @
    // 0x1000d6b98): samples vertex altitudes over X in [minX..maxX+1] and Z in
    // [minZ..maxZ+1], tracks the min/max altitude vertices, and returns
    // (maxAlt - minAlt) / compressedDistance.
    static Float32 ComputeLotAreaSlope(const LandValueIntRect& rect, bool flag,
                                       LandValueLotVertexAltitude vertexAltitude,
                                       void* lotDeveloper) noexcept;
};

struct LandValueTuning {
    static constexpr cSC4TGI kExemplarTGI{kTypeExemplar, 0xe7e2c2dbu, 0xe7e2c8d8u};
    Uint8 fieldD0 = 0; // prop 0x47e2c200 -> +0xd0 (Uint8 overload)
    std::unordered_map<Uint32, ExemplarProperty> properties;
    bool LoadFromExemplar(const cGZExemplar& exemplar);
};

// ---------------------------------------------------------------------------
// cSC4CommercialSimulator, CLSID 0x8990c080. Read 0x1001fc566 (version == 1).
// No ReadSimulatorTunableValues in the binary.
// ---------------------------------------------------------------------------
struct CommercialSimulatorSaveRecord {
    Uint16 version = 1;
    Uint32 memAddr = 0;
    bool field38 = false;         // +0x38
    SimSerializableRef gridRef58; // +0x58, IID 0xc9bd939a
    SimSerializableRef gridRef60; // +0x60, IID 0xc9bd939a

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;

    // Exact cSC4CommercialSimulator::GetProximityMap @ 0x1001fc80c,
    // DoMessage @ 0x1001fc608, DoIdle @ 0x1001fc6ba, Init @ 0x1001fc09e,
    // Shutdown @ 0x1001fc3cc (GZCOM_CreatecSC4CommercialSimulator @ 0x1001fbe90,
    // GZCOM_cSC4CommercialSimulatorCLSID @ 0x1001fbf0d, dtor @ 0x1001fbf24 /
    // 0x1001fbf56, QI @ 0x1001fbf60, AddRef @ 0x1001fbfc6, Release @ 0x1001fbff0).
    Uint32 idleStep3C = 0; // +0x3c (0..3 state machine stepped by DoIdle @ 0x1001fc6ba)
    SimSerializableRef GetProximityMapRef(Uint32 developerType) const noexcept;
    void StepIdleStateMachine() noexcept;
};

// ---------------------------------------------------------------------------
// cSC4IndustrialSimulator, CLSID 0xa990c022. Read @ 0x100215646, Write @ 0x1002155ee,
// Init @ 0x1002155ba, Shutdown @ 0x1002155dc, DoMessage @ 0x1002156ca,
// GetGZCLSID @ 0x1002156ee (GZCOM_CreatecSC4IndustrialSimulator @ 0x1002153f8,
// GZCOM_cSC4IndustrialSimulatorCLSID @ 0x10021543e, dtor @ 0x10021544a /
// 0x10021547c, QI @ 0x100215486, AddRef @ 0x1002154ec, Release @ 0x100215516).
// ---------------------------------------------------------------------------
struct IndustrialSimulatorSaveRecord {
    Uint16 version = 1;
    Uint32 memAddr = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;
};

// ---------------------------------------------------------------------------
// cSC4FireProtectionSimulator, CLSID 0x2990c05a. Read 0x1002fae42 (version
// <= 3), Write 0x1002fadd2. Stream: own version, then the
// cSC4ProtectionSimulator base (its own version word), then, if the outer
// version >= 3, one ref. Tuning instance 0x68f5524a.
// ---------------------------------------------------------------------------
struct FireProtectionSimulatorSaveRecord {
    Uint16 version = 3;
    Uint32 memAddr = 0;
    ProtectionSimulatorSaveRecord protection; // base at +8
    SimSerializableRef field120;              // +0x120, IID 0xc9c00435 (version >= 3)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;

    // Exact cSC4FireProtectionSimulator methods:
    //   cSC4FireProtectionSimulator() @ 0x1002fa4ce, ~cSC4FireProtectionSimulator() @ 0x1002fa5ec /
    //   0x1002fa6a4 / 0x1002fa6bc, QI @ 0x1002fa6f8, AddRef @ 0x1002fa778, Release @ 0x1002fa7c8,
    //   Init @ 0x1002fa8d2, ReadSimulatorTunableValues @ 0x1002faa86, Shutdown @ 0x1002fad3e,
    //   DoMessage @ 0x1002faef4, DoMonthlySimulation @ 0x1002faf6e,
    //   UpdateStrikingState @ 0x1002fb046, DoDailySimulation @ 0x1002fb3dc,
    //   ProtectionProcessCell @ 0x1002fb408, ProtectionEndOfCycle @ 0x1002fb414,
    //   GetStationEfficiency @ 0x1002fb58e, GetStationDispatchRadius @ 0x1002fb6be,
    //   GetDepartmentBudget @ 0x1002fb6e8, GetStationFunding @ 0x1002fb6f6,
    //   SetStationFunding @ 0x1002fb77e, IsFireStation @ 0x1002fb82e,
    //   CanHandleDispatchType @ 0x1002fb84e, GetDispatchAutomataProperties @ 0x1002fb866,
    //   GetTotalDispatchCount @ 0x1002fb8ae, GetDispatchesAtStation @ 0x1002fb928,
    //   IsOnStrike @ 0x1002fb9da, IsStationOnStrike @ 0x1002fb9e6,
    //   GetChanceOfStrike @ 0x1002fba26, BeginStrike @ 0x1002fbb1e,
    //   EndStrike @ 0x1002fbbfe, GetStrikeStartDate @ 0x1002fbc92,
    //   GetStrikeLength @ 0x1002fbc9e, GetStrikingStations @ 0x1002fbcea,
    //   GetAverageCoverage @ 0x1002fbd86, GetPercentCoverage @ 0x1002fbd94,
    //   GetOccupantCoverage @ 0x1002fbda2, GetFireStations @ 0x1002fbe3c,
    //   GetFireStationCount @ 0x1002fbe54,
    //   ProtectionSetStationBaseCallNumber @ 0x1002fbe60,
    //   ProtectionGetDispatchType @ 0x1002fbeac,
    //   ProtectionGetStationFunding @ 0x1002fbec4,
    //   ProtectionGetDispatchEffect @ 0x1002fbf5e,
    //   ProtectionGetStationEffect @ 0x1002fc00c,
    //   ProtectionSetStationAvailabilityFlags @ 0x1002fc198,
    //   GetFireProtectionGrid @ 0x1002fc1e4.
    static Float32 ComputeStationEfficiency(
        Float32 fundingFraction, const cSC4ResponseCurve& fundingCurve,
        bool hasRoadAccess, Float32 noRoadAccessMult,
        bool isOnStrike, Float32 strikeEfficiencyMult) noexcept;
};

struct FireProtectionTuning {
    static constexpr cSC4TGI kExemplarTGI{kTypeExemplar, 0xe7e2c2dbu, 0x68f5524au};
    Float32 field14C = 0.0f; // prop 0x29233e94 -> +0x14c
    std::unordered_map<Uint32, ExemplarProperty> properties;
    bool LoadFromExemplar(const cGZExemplar& exemplar);
};

// ---------------------------------------------------------------------------
// cSC4PoliceSimulator, CLSID 0x6990c04f. Read 0x1000a3204 (version <= 8,
// Write stores 8), Write 0x1000a30de. Tuning instance 0x28dfcf89.
// ---------------------------------------------------------------------------
struct PoliceSimulatorSaveRecord {
    Uint16 version = 8;
    Uint32 memAddr = 0;
    ProtectionSimulatorSaveRecord protection; // base at +8
    SimSerializableRef field128;              // +0x128, IID 0x223e2b67
    OccupantRefList list168;                  // +0x168
    OccupantRefList list130;                  // +0x130
    Sint32 field148 = 0;                      // read order: 148,14c,150,154,
    Sint32 field14C = 0;                      // 15c,164,158,160
    Sint32 field150 = 0;
    Sint32 field154 = 0;
    Sint32 field15C = 0;
    Sint32 field164 = 0;
    Sint32 field158 = 0;
    Sint32 field160 = 0;
    SimSerializableRef field118;              // +0x118, IID 0xc9c00435 (version >= 8)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;

    // Exact cSC4PoliceSimulator::DoMonthlyCriminalRehabilitation @ 0x1000a38fe
    // and ProcessMonthlyCrimeStats @ 0x1000a3b04.
    // Also: cSC4PoliceSimulator() @ 0x1000a237e, ~cSC4PoliceSimulator() @ 0x1000a25ac /
    //   0x1000a26b8 / 0x1000a26d0, QI @ 0x1000a270c, AddRef @ 0x1000a2788, Release @ 0x1000a27d8,
    //   StaticShutdown @ 0x1000a28e6, Init @ 0x1000a28fe,
    //   ReadSimulatorTunableValues @ 0x1000a2b74, Shutdown @ 0x1000a2fb4,
    //   DoMessage @ 0x1000a3380, GetNearestStation @ 0x1000a350c,
    //   UpdatePoliceStationCrimeStats @ 0x1000a35de,
    //   RecalculateCriminalStatistics @ 0x1000a36de,
    //   DoMonthlySimulation @ 0x1000a3834,
    //   CheckForStrikeStartOrEnd @ 0x1000a3944,
    //   CheckForJailbreakEnd @ 0x1000a3a12,
    //   CheckForJailbreakStart @ 0x1000a3a74,
    //   DoDailySimulation @ 0x1000a3bd8, ProtectionProcessCell @ 0x1000a3c04,
    //   ProtectionEndOfCycle @ 0x1000a3c10, GetStationEfficiency @ 0x1000a3d48,
    //   CalculateJailEfficiency @ 0x1000a3e3e, GetStationDispatchRadius @ 0x1000a3ef6,
    //   GetDepartmentBudget @ 0x1000a3f20, GetJailDepartmentBudget @ 0x1000a3f2e,
    //   UpdateStrikingState @ 0x1000a3f3c, IsOnStrike @ 0x1000a42c2,
    //   IsStationOnStrike @ 0x1000a42ce, GetChanceOfStrike @ 0x1000a430e,
    //   BeginStrike @ 0x1000a4406, EndStrike @ 0x1000a44f8,
    //   GetStrikeStartDate @ 0x1000a45a0, GetStrikeLength @ 0x1000a45ac,
    //   GetStrikingStations @ 0x1000a45f8, GetPoliceStations @ 0x1000a46a2,
    //   GetPoliceStationCount @ 0x1000a46ba, GetJails @ 0x1000a46c6,
    //   GetJailCount @ 0x1000a46de, BeginJailbreak @ 0x1000a46ea,
    //   EndJailbreak @ 0x1000a49b8, IsInJailbreak @ 0x1000a4bd2,
    //   GetJailbreakBuildings @ 0x1000a4c22, GetChanceOfJailbreak @ 0x1000a4c46,
    //   GetStationFunding @ 0x1000a4cda, SetStationFunding @ 0x1000a4d62,
    //   IsPoliceStation @ 0x1000a4e12, IsJail @ 0x1000a4e32,
    //   GetOccupantCoverage @ 0x1000a4e52, GetAverageCoverage @ 0x1000a4ee0,
    //   GetPercentCoverage @ 0x1000a4eee, ResetUnjailedCriminals @ 0x1000a4efc,
    //   GetJailCapacity() @ 0x1000a4f48, GetJailPopulation @ 0x1000a4f54,
    //   GetCriminalCount @ 0x1000a4f60, GetArrestCount @ 0x1000a4f6c,
    //   GetJailCurrentUpkeepCost @ 0x1000a4f78,
    //   GetJailCapacity(cISC4Occupant*) @ 0x1000a4fe8,
    //   GetJailedPercentage @ 0x1000a5054, DoOccupantInserted @ 0x1000a5080,
    //   DoOccupantRemoved @ 0x1000a52fe, DoOccupantChanged @ 0x1000a54fa,
    //   CanHandleDispatchType @ 0x1000a55be,
    //   GetDispatchAutomataProperties @ 0x1000a55d6,
    //   GetTotalDispatchCount @ 0x1000a569e,
    //   GetDispatchesAtStation @ 0x1000a5718,
    //   ProtectionSetStationBaseCallNumber @ 0x1000a57ca,
    //   ProtectionGetDispatchType @ 0x1000a5816,
    //   ProtectionGetStationFunding @ 0x1000a5826,
    //   ProtectionGetDispatchEffect @ 0x1000a589a,
    //   ProtectionGetStationEffect @ 0x1000a594c,
    //   ProtectionSetStationAvailabilityFlags @ 0x1000a5abc,
    //   GetPoliceStationArrestCount @ 0x1000a5b08,
    //   GetPoliceStationCrimeCount @ 0x1000a5b7c,
    //   GetPolicePowerGrid @ 0x1000a5bfa.
    void DoMonthlyCriminalRehabilitation(Float32 rehabRate) noexcept;
    void ProcessMonthlyCrimeStats() noexcept;
    void ResetUnjailedCriminals(Float32 jailCapacityMultiplier) noexcept;
    Float32 GetJailedPercentage() const noexcept;
};

struct PoliceTuning {
    static constexpr cSC4TGI kExemplarTGI{kTypeExemplar, 0xe7e2c2dbu, 0x28dfcf89u};
    Sint32 field1D4 = 0; // prop 0x68ddae9b -> +0x1d4
    Sint32 field200 = 0; // prop 0x68ddae9f -> +0x200
    std::unordered_map<Uint32, ExemplarProperty> properties;
    bool LoadFromExemplar(const cGZExemplar& exemplar);
};

// ---------------------------------------------------------------------------
// cSC4FlammabilitySimulator, CLSID 0xe990c045. Read 0x1000c36b8 (version
// must == 2), Write 0x1000c35e4. Tuning instance 0xa8f41ee3.
// ---------------------------------------------------------------------------
struct FlammabilitySimulatorSaveRecord {
    Uint16 version = 2;
    Uint32 memAddr = 0;
    SimSerializableRef gridRef80; // +0x80, IID 0xc9bd939a
    SimSerializableRef gridRef88; // +0x88, IID 0xc9bd939a
    Float32 field94 = 0.0f;       // +0x94
    // +0x90 bool, streamed as its raw storage byte (shipped saves contain
    // values such as 0x2e / 0x3c here, so it must not be normalised to 0/1).
    Uint8 field90 = 0;
    std::vector<Sint32Triple> entries; // list at +0xb8

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;

    // Exact cSC4FlammabilitySimulator methods:
    //   cSC4FlammabilitySimulator() @ 0x1000c2a6e, ~cSC4FlammabilitySimulator() @ 0x1000c2bec /
    //   0x1000c2c72 / 0x1000c2c7c, QI @ 0x1000c2caa, AddRef @ 0x1000c2d10, Release @ 0x1000c2d3a,
    //   Init @ 0x1000c2de8, ReadSimulatorTunableValues @ 0x1000c31ee, Shutdown @ 0x1000c3404,
    //   DoMessage @ 0x1000c380a, DoDailySimulation @ 0x1000c39ba, DoOccupantUpdate @ 0x1000c3bfe,
    //   EndOfCycle @ 0x1000c3e06, ComputeHotspotList @ 0x1000c3efa, TryRandomFirestart @ 0x1000c4092,
    //   GetOccupantRawFlammability @ 0x1000c43a6, ComputeTractRawFlammability @ 0x1000c448c,
    //   GetAverageFlammability @ 0x1000c472c, IsCellFlammable @ 0x1000c473a,
    //   GetFlammabilityHotspot @ 0x1000c4766, GetFlammabilityGrid @ 0x1000c4848.
    Uint8 GetAverageFlammability() const noexcept { return field90; }
    static Uint8 ComputeScaledFlammabilityByte(Sint32 rawPropValue, Float32 mult) noexcept;
    void ComputeHotspotList(std::span<const Sint32Triple> candidates,
                            Sint32 mergeDistanceThreshold,
                            Sint32 minClusterCount) noexcept;
};

struct FlammabilityTuning {
    // cSC4FlammabilitySimulator::ReadSimulatorTunableValues @ 0x1000c31ee
    static constexpr cSC4TGI kExemplarTGI{kTypeExemplar, 0xe7e2c2dbu, 0xa8f41ee3u};
    Float32 fieldD8 = 0.0f; // props 0xc9b40128 / 0x29b40134 -> +0xd8
    Float32 fieldF4 = 0.0f; // prop 0x29b4013e -> +0xf4
    std::unordered_map<Uint32, ExemplarProperty> properties;
    bool LoadFromExemplar(const cGZExemplar& exemplar);
};

// ---------------------------------------------------------------------------
// cSC4CrimeSimulator, CLSID 0x8990c09a. Read 0x1001fde02 (version must == 6),
// Write 0x1001fdc82. Tuning instance 0xc8e8f6c5.
// ---------------------------------------------------------------------------
struct CrimeEventRecord {
    SimSerializableRef occupantRef; // node +0x00, IID 0xe818fced
    Uint32 field08 = 0;             // node +0x08
    std::string name;               // node +0x10 (GZString)
    cSC4TGI resourceKey;            // node +0x38 (cGZPersistResourceKey)
    // node +0x44 is a byte the binary zeroes after reading; not serialized.
};

struct CrimeSimulatorSaveRecord {
    Uint16 version = 6;
    Uint32 memAddr = 0;
    SimSerializableRef gridRef90; // +0x90, IID 0xc9bd939a
    SimSerializableRef gridRef98; // +0x98, IID 0xc9bd939a
    RandomIteratorState randomIterator; // +0x28
    Uint32 fieldA0 = 0;           // +0xa0
    Uint32 fieldA4 = 0;           // +0xa4
    Float32 fieldA8 = 0.0f;       // +0xa8
    Float32 fieldD8 = 0.0f;       // +0xd8
    Uint32 fieldC8 = 0;           // +0xc8
    Sint32 fieldCC = 0;           // +0xcc
    Sint32 fieldD0 = 0;           // +0xd0
    Sint32 fieldD4 = 0;           // +0xd4
    std::vector<CrimeEventRecord> events; // Sint32 count, then events

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;

    // Exact cSC4CrimeSimulator::DecayCrimeGrid @ 0x1001fead8 and
    // CalculateEconomicFactors @ 0x1001fe8f2.
    // Also: cSC4CrimeSimulator() @ 0x1001fcae8, ~cSC4CrimeSimulator() @ 0x1001fcdbc /
    //   0x1001fced0 / 0x1001fceda, QI @ 0x1001fcf08, AddRef @ 0x1001fcf84, Release @ 0x1001fcfbc,
    //   StaticInit @ 0x1001fd0a2, StaticShutdown @ 0x1001fd0dc,
    //   LoadAllCrimeTypesFromExemplars @ 0x1001fd15c, Init @ 0x1001fd26e,
    //   ReadSimulatorTunableValues @ 0x1001fd622, Shutdown @ 0x1001fda74,
    //   ClearCrimeEvents @ 0x1001fdbe6, GetSimulatorType @ 0x1001fe008,
    //   DoMessage @ 0x1001fe014, DoIdle @ 0x1001fe164,
    //   DoOccupantChanged @ 0x1001fe382, GetAverageCrime @ 0x1001fe4ec,
    //   GetMonthlyCrimeCount @ 0x1001fe4fe, GetRequestedShare @ 0x1001fe50a,
    //   SelectProp @ 0x1001fe578, DoCensus @ 0x1001fe778,
    //   ComputeCriminalBase @ 0x1001fe9e2, AddCrimes @ 0x1001feb94,
    //   ComputeAverageCrime @ 0x1001fed98, AddCrimeEvent @ 0x1001feec6,
    //   LoadCrimeTypeFromExemplar @ 0x1001ff2b8, GetLotBits @ 0x1001ff5a8,
    //   GetCrimeHotspot @ 0x1001ff6ac, GetCrimeIncidentArray @ 0x1001ff726,
    //   SetVisualsCreated @ 0x1001ff894, GetVisualsCreated @ 0x1001ff8c4,
    //   GetNearestCrimeProp @ 0x1001ff8f6, GetNearestCrimeSim @ 0x1001ffbd4,
    //   GetCrimeGrid @ 0x1001ffea8, GetRequesterSatisfactionId @ 0x1001ffec2,
    //   UsesCustomPropSelector @ 0x1001ffece.
    void DecayCrimeGrid(std::span<Uint8> crimeGrid, Uint32 width, Uint32 height,
                        Float32 decayFactor) noexcept;
    static Float32 ComputeEconomicCrimeFactor(Float32 supply, Float32 demand,
                                              Float32 cap) noexcept;
    Uint8 GetAverageCrime() const noexcept {
        return static_cast<Uint8>(static_cast<Sint32>(fieldA8));
    }
    bool GetCrimeHotspot(Sint32 tractSize, Uint8* outValue,
                         Sint32* outWorldX, Sint32* outWorldZ) const noexcept;
};

struct CrimeTuning {
    static constexpr cSC4TGI kExemplarTGI{kTypeExemplar, 0xe7e2c2dbu, 0xc8e8f6c5u};
    Sint32 field24C = 0; // prop 0x47e2c200 -> +0x24c
    std::unordered_map<Uint32, ExemplarProperty> properties;
    bool LoadFromExemplar(const cGZExemplar& exemplar);
};

// ---------------------------------------------------------------------------
// cSC4AuraSimulator, CLSID 0x0990bdd8. Read 0x100219b70 (version >= 8
// required), Write 0x10021997e stores version 9, so the map branch is the
// live one. Tuning instance 0xaa023079.
// ---------------------------------------------------------------------------
struct AuraSimulatorSaveRecord {
    Uint16 version = 9;
    Uint32 memAddr = 0;
    // Seven SimGrid refs (IID 0xc9bd939a) in exact stream order:
    SimSerializableRef gridRef58; // +0x58 (1st)
    SimSerializableRef gridRef88; // +0x88 (2nd)
    SimSerializableRef gridRef80; // +0x80 (3rd)
    SimSerializableRef gridRef68; // +0x68 (4th)
    SimSerializableRef gridRef70; // +0x70 (5th)
    SimSerializableRef gridRef78; // +0x78 (6th)
    SimSerializableRef gridRefA8; // +0xa8 (7th)
    // Version <= 8 only: one extra ref the binary reads into a temp and
    // discards. Preserved here so legacy records round-trip.
    bool hasLegacyRef = false;
    SimSerializableRef legacyRef;
    // Version > 8: Uint32 count, then count x (Uint32 key + ref), map @ +0x90.
    std::vector<std::pair<Uint32, SimSerializableRef>> gridMap;
    Sint32 fieldD0 = 0;   // +0xd0
    Sint32 fieldD4 = 0;   // +0xd4
    Sint32 field1F0 = 0;  // +0x1f0
    Sint32 field1F4 = 0;  // +0x1f4
    Sint16 field1F8 = 0;  // +0x1f8
    Sint32 field1FC = 0;  // +0x1fc
    Sint32 field200 = 0;  // +0x200
    Sint16 field204 = 0;  // +0x204
    std::vector<Sint32Triple> entries; // list at +0x130

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;

    // Exact cSC4AuraSimulator::UpdateAuraGrid @ 0x10021cc5c.
    // Also: cSC4AuraSimulator() @ 0x10021801e, Init @ 0x100218422,
    // sAddTransientEffect @ 0x100219220, sEndStrike @ 0x100219316,
    // Shutdown @ 0x100219384, DoMessage @ 0x100219f62, DoIdle @ 0x10021a1c4,
    // DoOccupantChanged @ 0x10021acd2, DoEndOfSimulationCycle @ 0x10021b2d0,
    // GetAuraValue @ 0x10021b364, AddTransientEffect @ 0x10021b384 / 0x10021b3ba,
    // ComputeTaxRateChangeEffect @ 0x10021b476, GetLotBaseAura @ 0x10021b564,
    // ComputeHotspotList @ 0x10021cfdc, CheckForStartingRiot @ 0x10021d174,
    // BustStrike @ 0x10021d272.
    void UpdateAuraGrid(std::span<const Sint8> baseAuraGrid,
                        std::span<const Sint8> transientAuraGrid,
                        std::span<const Uint16> resPopGrid,
                        Uint32 width, Uint32 height, Sint32 tractSize,
                        std::span<Sint8> outTotalAuraGrid) noexcept;
    void ComputeHotspotList(std::span<const Sint32Triple> candidates,
                            Sint32 mergeDistanceThreshold,
                            Sint32 minClusterCount) noexcept;
};

struct AuraTuning {
    static constexpr cSC4TGI kExemplarTGI{kTypeExemplar, 0xe7e2c2dbu, 0xaa023079u};
    Float32 field1A8 = 0.0f; // prop 0xea02327a -> +0x1a8
    Float32 field1AC = 0.0f; // prop 0xea02327b -> +0x1ac
    Sint32 field1B0 = 0;     // prop 0xea02327c -> +0x1b0
    Sint32 field1B4 = 0;     // prop 0xea02327d -> +0x1b4
    Sint32 field1B8 = 0;     // prop 0xea02327e -> +0x1b8
    std::unordered_map<Uint32, ExemplarProperty> properties;
    bool LoadFromExemplar(const cGZExemplar& exemplar);
};

// ---------------------------------------------------------------------------
// cSC4PowerSimulator, CLSID 0x0990c005. Read 0x10020fada (version <= 6),
// Write 0x10020f6da stores 6. Tuning instance 0xc911e35b (shared simulator
// tuning exemplar, also used by Plumbing and Pollution).
// Complete method index:
//   cSC4PowerSimulator() @ 0x10020e21e, ~cSC4PowerSimulator() @ 0x10020e53e /
//   0x10020e67e / 0x10020e688, QI @ 0x10020e6b6, AddRef @ 0x10020e71c,
//   Release @ 0x10020e746, Init @ 0x10020e752,
//   ReadSimulatorTunableValues @ 0x10020ed2c,
//   InitMarkAndWireBits @ 0x10020f2ba, Shutdown @ 0x10020f3b6,
//   ShutdownMarkAndWireBits @ 0x10020f65a,
//   PositionToCell @ 0x100210144, CellToPosition @ 0x1002101b6,
//   DoMessage @ 0x100210248, DoMonthlySimulation @ 0x1002108c0,
//   DoOccupantInserted @ 0x1002109d6, DoOccupantRemoved @ 0x10021102c,
//   CellChanged @ 0x100211630, DoIdle @ 0x1002116ec,
//   DoTerrainHeightsChanged @ 0x100212228,
//   RecomputeInfrastructureCosts @ 0x1002125ae,
//   ExpandRectToIncludeNeighborhood @ 0x100212644,
//   ApplyPoleFunding @ 0x100212708,
//   GetCurrentPowerProduced @ 0x100212880, GetSupplyValue @ 0x100212992,
//   FindPowerPlants @ 0x1002129cc, GetNeighborhoodOfPoint @ 0x100212c4c,
//   MarkPowerPlantsAsVisited @ 0x100212d12, ResetWireBits @ 0x100212e28,
//   UpdateWireBits @ 0x100212eb4, SetsWireBits @ 0x10021338c,
//   IsPowered @ 0x1002133ee, IsWired @ 0x10021342c,
//   UpdatePowerGrid @ 0x100213478,
//   IsNeighborConnectedToCurrentGrid @ 0x100213e2a,
//   UpdatePoweredLots @ 0x10021410e, get_connection_cell @ 0x10021437e,
//   GetMonthlyPowerGenerationCost @ 0x10021441e,
//   AgePowerPlant @ 0x100214476, IsOccupantPowered @ 0x10021485c,
//   GetOccupantPowerConsumed @ 0x1002148a8,
//   GetOccupantPowerProduced @ 0x1002148f2,
//   GetOccupantMaxPowerProduced @ 0x100214984,
//   GetPowerPlants @ 0x1002149ac, GetStrikeLength @ 0x100214a48,
//   GetPowerPlantFundingPercentage @ 0x100214a88,
//   SetPowerPlantFundingPercentage @ 0x100214af2,
//   GetCurrentPlantEfficiencyPercent @ 0x100214b80,
//   GetCurrentPlantConditionPercent @ 0x100214c2c,
//   GetFundingPercent @ 0x100214e16.
// ---------------------------------------------------------------------------
struct PowerSimulatorSaveRecord {
    Uint16 version = 6;
    Uint32 memAddr = 0;
    SimSerializableRef simManagerRef; // +0x38, IID 0x223e2b67
    Sint32 field68 = 0;               // +0x68
    Sint32 field6C = 0;               // +0x6c
    Sint32 field70 = 0;               // +0x70
    Float32 field94 = 0.0f;           // +0x94 (Sint32 x 0.01 in stream if version < 6)
    Sint32 field98 = 0;               // +0x98
    Sint32 field9C = 0;               // +0x9c
    Sint32 fieldA0 = 0;               // +0xa0
    Sint32 fieldA4 = 0;               // +0xa4
    Sint64 fieldA8 = 0;               // +0xa8
    Sint32 field80 = 0;               // +0x80
    Sint32 field84 = 0;               // +0x84
    Sint32 field88 = 0;               // +0x88
    Sint32 field8C = 0;               // +0x8c
    Sint32 field90 = 0;               // +0x90
    SimSerializableRef gridRefB0;     // +0xb0, IID 0xc9bd939a
    // vector<vector<Sint16>> at +0xb8: Uint32 rows, Uint32 cols, then
    // rows x cols Sint16 values.
    std::vector<std::vector<Sint16>> sint16Grid;
    std::array<std::array<Sint32, 10>, 12> monthlyHistory{}; // +0xd0, stride 0x28
    std::array<Sint32, 10> field2B0{};                       // +0x2b0
    // List at +0x2d8: Sint32 count at +0x354, 10 fixed Sint32 at +0x358,
    // then count x (Uint32 key + ref IID 0x26bb767e).
    std::array<Sint32, 10> field358{};
    std::vector<std::pair<Uint32, SimSerializableRef>> occupantEntries;
    Uint32 field31C = 0; // +0x31c; also the count of poleOccupantRefs
    Uint32 field320 = 0; // +0x320
    std::vector<SimSerializableRef> poleOccupantRefs; // +0x2f0, IID 0xa8c86142
    // Four parallel Uint32 arrays (binary mallocs each at arrayCount):
    std::vector<Uint32> array328; // +0x328
    std::vector<Uint32> array330; // +0x330
    std::vector<Uint32> array348; // +0x348
    std::vector<Uint32> array340; // +0x340
    bool field350 = false;   // +0x350
    bool field3A8 = false;   // +0x3a8 (binary forces it to 0 after reading)
    Sint32 field3AC = 0;     // +0x3ac
    bool field3B0 = false;   // +0x3b0
    bool field3B1 = false;   // +0x3b1
    Uint32 field3C4 = 0;     // +0x3c4

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;

    // Exact cSC4PowerSimulator::DoMonthlySimulation @ 0x1002108c0.
    void StepMonthly() noexcept;
};

struct PowerTuning {
    static constexpr cSC4TGI kExemplarTGI{kTypeExemplar, 0xe7e2c2dbu, 0xc911e35bu};
    Uint32 field314 = 2;       // prop 0xa911e2f1 -> +0x314 (binary default 2)
    Float32 field318 = 0.1f;   // prop 0xe911e2fc -> +0x318 (binary default 0.1f)
    Float32 field3F4 = 1.2f;   // +0x3f4 (binary default 1.2f, 0x3f99999a)
    Sint32 field3FC = 0;       // prop 0x2912202e -> +0x3fc
    std::unordered_map<Uint32, ExemplarProperty> properties;
    bool LoadFromExemplar(const cGZExemplar& exemplar);
};

// ---------------------------------------------------------------------------
// cSC4PlumbingSimulator, CLSID 0x0990c075. Read 0x10009d0a6 (version must
// == 4), Write 0x10009cdd8. Tuning instance 0xc911e35b (shared).
// WaterProducer (0x48 bytes in memory) via ReadWaterProducer 0x10009d488 /
// WriteWaterProducer 0x10009d00c.
// ---------------------------------------------------------------------------
struct WaterProducerRecord {
    Sint32 field00 = 0; // +0x00
    Sint32 field04 = 0; // +0x04
    Sint32 field08 = 0; // +0x08
    Sint32 field0C = 0; // +0x0c
    Sint32 field10 = 0; // +0x10
    Sint32 field14 = 0; // +0x14
    Uint32 field1C = 0; // +0x1c
    Uint32 field20 = 0; // +0x20
    SimSerializableRef occupantRef; // +0x30, IID 0x26bb767e
};

struct PlumbingSimulatorSaveRecord {
    Uint16 version = 4;
    Uint32 memAddr = 0;
    SimSerializableRef gridRef58; // +0x58, IID 0xc9bd939a
    SimSerializableRef gridRef60; // +0x60, IID 0xc9bd939a
    // Bool grid at +0xc8: Uint32 width (+0xd0), Uint32 height (+0xd4),
    // then width*height bool bytes (binary reads the first cell before the
    // loop; total is still W*H bytes). +0xe0 = min(W,H) is computed.
    std::vector<Uint8> boolGrid;
    Uint32 gridWidth = 0;
    Uint32 gridHeight = 0;
    Sint32 fieldE4 = 0;  // +0xe4
    Uint32 fieldE8 = 0;  // +0xe8
    Sint64 field1B8 = 0; // +0x1b8
    Uint32 field23C = 0; // +0x23c (read after the producer count)
    std::vector<WaterProducerRecord> producers; // list at +0x110
    // The binary then reads one more producer (the "current" one) and links
    // it against the list by +0x40 identity; serialized as a full record.
    WaterProducerRecord currentProducer;
    ScanningIteratorState scanningIterator; // +0x1b0
    Sint64 field198 = 0;                    // +0x198
    Uint32 field21C = 0;                    // +0x21c
    Uint32 field220 = 0;                    // +0x220
    std::vector<SimSerializableRef> gridRefList200; // +0x200, IID 0x49c05b9f, count at +0x218
    SimSerializableRef simManagerRef;       // +0xf0, IID 0x223e2b67
    Sint64 field178 = 0;                    // +0x178
    Sint64 field180 = 0;                    // +0x180
    Sint64 field188 = 0;                    // +0x188
    Sint64 field190 = 0;                    // +0x190

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;

    // Exact cSC4PlumbingSimulator methods:
    //   cSC4PlumbingSimulator() @ 0x10009bc9e, ~cSC4PlumbingSimulator() @ 0x10009bf2c /
    //   0x10009c038 / 0x10009c042, QI @ 0x10009c070, AddRef @ 0x10009c0d6,
    //   Release @ 0x10009c100, Init @ 0x10009c1b8, ReadSimulatorTunableValues @ 0x10009c782,
    //   Shutdown @ 0x10009cb48, Write @ 0x10009cdd8, WriteWaterProducer @ 0x10009d00c,
    //   Read @ 0x10009d0a6, ReadWaterProducer @ 0x10009d488, DoMessage @ 0x10009d558,
    //   DoIdle @ 0x10009d7a6, DoOccupantInserted @ 0x10009f510,
    //   DoOccupantRemoved @ 0x10009f5ca, ApplyPipeFunding @ 0x10009f68c,
    //   UpdateWaterProduction @ 0x10009fa6c, get_connection_cell @ 0x10009fd6e,
    //   IsNeighborConnectedToCurrentNetwork @ 0x10009fed2,
    //   BuildingConsumption @ 0x10009ffca, FillGridAtOccupant @ 0x1000a00ca,
    //   AgeWaterProducer @ 0x1000a0204, GetOccupantMonthlyUpkeepCost @ 0x1000a040e,
    //   FindWaterBuildings @ 0x1000a0466, GetWaterProducers @ 0x1000a06fc,
    //   GetSupplyValue @ 0x1000a076a, GetSupplyValueMapPtr @ 0x1000a0780,
    //   AreWateredBuildingsInCell @ 0x1000a078e, IsBuildingWatered @ 0x1000a07ca,
    //   GetWaterProduction @ 0x1000a0836, IsCellWatered @ 0x1000a0862,
    //   DoPipeChanged @ 0x1000a0874, DoBuildingChanged @ 0x1000a0af4,
    //   GetWaterProducerInfo @ 0x1000a0f2c,
    //   GetWaterProducerConditionPercent @ 0x1000a0fae,
    //   NearestWater @ 0x1000a106a, GetWaterDemand @ 0x1000a11e4,
    //   GetWateredGrid @ 0x1000a11f0, GetNumWaterSources @ 0x1000a11fa,
    //   GetNumPollutedWaterSources @ 0x1000a1206,
    //   GetWaterProductionCapacity @ 0x1000a1212, GetWaterConsumed @ 0x1000a121e,
    //   GetWaterImported @ 0x1000a122a, GetWaterExported @ 0x1000a1236,
    //   GetNumPipes @ 0x1000a1242, GetNumDistressedPipes @ 0x1000a124e,
    //   GetNumBurstPipes @ 0x1000a125a, GetNumWateredBuildings @ 0x1000a1266,
    //   GetNumUnwateredBuildings @ 0x1000a1272.
    Sint64 GetWaterDemand() const noexcept { return field198; }
    Uint32 GetNumWaterSources() const noexcept { return static_cast<Uint32>(producers.size()); }
    Uint32 GetNumPollutedWaterSources() const noexcept { return field23C; }
    Sint64 GetWaterProductionCapacity() const noexcept { return field178; }
    Sint64 GetWaterConsumed() const noexcept { return field180; }
    Sint64 GetWaterImported() const noexcept { return field188; }
    Sint64 GetWaterExported() const noexcept { return field190; }
    Uint32 GetNumPipes() const noexcept { return static_cast<Uint32>(gridRefList200.size()); }
    Uint32 GetNumDistressedPipes() const noexcept { return field21C; }
    Uint32 GetNumBurstPipes() const noexcept { return field220; }
};

struct PlumbingTuning {
    static constexpr cSC4TGI kExemplarTGI{kTypeExemplar, 0xe7e2c2dbu, 0xc911e35bu};
    Uint32 fieldF8 = 0;   // prop 0x68f5436a -> +0xf8
    Uint32 fieldFC = 0;   // prop 0x68f5436b -> +0xfc
    Uint32 field100 = 0;  // prop 0x68f5436c -> +0x100
    Uint32 field104 = 0;  // prop 0x68f5436d -> +0x104
    Uint32 field108 = 0;  // prop 0xaa161f81 -> +0x108
    Float32 field228 = 0.0f; // prop 0x8a235c56 -> +0x228
    Float32 field288 = 0.0f; // prop 0x8a84778f -> +0x288
    std::unordered_map<Uint32, ExemplarProperty> properties;
    bool LoadFromExemplar(const cGZExemplar& exemplar);
};

// ---------------------------------------------------------------------------
// cSC4PollutionSimulator, CLSID 0x8990c065. Read 0x100096588 (version <= 4),
// Write 0x1000961d0 stores 4. Tuning instance 0xc911e35b (shared); the
// binary's pollution tunables are file-scope statics, so they are modeled
// as plain values here. Legacy note: for version <= 2 the binary reads (and
// discards) a GZString after the +0x22c field and a Uint32 after the four
// +0x328 bools; both are preserved below so legacy records round-trip.
// Complete method index:
//   cSC4PollutionSimulator() @ 0x10009469e, ~cSC4PollutionSimulator() @ 0x100094a7c /
//   0x100094c2c / 0x100094c36, QI @ 0x100094c64, AddRef @ 0x100094cca,
//   Release @ 0x100094cf4, FinalRelease @ 0x100094d64,
//   StaticInit @ 0x100094e06, StaticShutdown @ 0x100094f4e,
//   Init @ 0x100095092, ReadSimulatorTunableValues @ 0x100095912,
//   Shutdown @ 0x100095d7a, DoMessage @ 0x100096a58,
//   DoDailySimulation @ 0x100096b5a, DoMonthlySimulation @ 0x100097cf2,
//   SimulationBegin @ 0x100098006, DoOccupantChanged @ 0x1000980b2,
//   UpdatePollutionVisualEffect @ 0x1000986d8,
//   GetRecyclingCenterMaxCapacity @ 0x10009921a,
//   GetRecyclingCenterCurrentCapacity @ 0x10009924a,
//   GetMaxCapacity @ 0x10009929e, GetCurrentCapacity @ 0x1000992cc,
//   GetIncineratorCurrentCapacity @ 0x100099414,
//   GetWasteToEnergyCurrentCapacity @ 0x10009943e,
//   GetWaterTreatmentMaxCapacity @ 0x100099468,
//   GetWaterTreatmentCurrentCapacity @ 0x100099492,
//   GetWaterTreatmentUsedCapacity @ 0x1000994bc,
//   GetCurrentOccupantCondition @ 0x1000994e6,
//   GetCurrentOccupantEfficiency @ 0x1000995b2,
//   GetUnusedCapacity @ 0x10009962c,
//   GetIncineratorUsedCapacity @ 0x100099698,
//   GetWasteToEnergyUsedCapacity @ 0x1000996c6,
//   EndOfCycle @ 0x1000996f4, CalculateRecyclingCenterEffect @ 0x100099954,
//   ResetOccupantListCapacityUsedProperty @ 0x100099bc0,
//   AgeOccupantList @ 0x100099c88,
//   ComputeWasteToEnergyListCurrentCapacity @ 0x100099f00,
//   SetOccupantListUsedCapacity @ 0x10009a004,
//   CalcOccupantMaintenanceCost @ 0x10009a11c,
//   CalcMaintenanceCosts @ 0x10009a1fe, eatGarbage @ 0x10009a2ee,
//   GetPollutionValue @ 0x10009a5e0, IsRadioactive @ 0x10009a604,
//   GetAirValue @ 0x10009a61e, GetWaterValue @ 0x10009a63e,
//   GetGarbageValue @ 0x10009a65e, IsAirPolluted @ 0x10009a684,
//   IsWaterPolluted @ 0x10009a6aa, IsGarbageAccumulated @ 0x10009a6d0,
//   SetIsRadioactive @ 0x10009a738, SetAirValue @ 0x10009a790,
//   SetWaterValue @ 0x10009a7e8, SetGarbageValue @ 0x10009a840,
//   GetBuildingGarbageCapacity @ 0x10009a8e2,
//   IsActiveLandfill @ 0x10009a924, SetIsActiveLandfill @ 0x10009a940,
//   AddOrRemoveOccupant @ 0x10009aa2a, AddRadiationToCity @ 0x10009aaf4,
//   GetGarbageBuildings @ 0x10009ac48, GetRequestedShare @ 0x10009ad5c,
//   GetIncineratorCount @ 0x10009af3a.
// ---------------------------------------------------------------------------
// --- Swarm wave 1 (Nov 2026): seams + file-scope tuning globals ------------
// The binary's cSC4PollutionSimulator interacts with neighbor subsystems
// through virtual dispatch; the save-record model has no vtable, so those
// surfaces become minimal abstract seams. Only the cited slots are modeled —
// no invented semantics beyond the observations.

// @ 0x10009a4af/0x10009a4d6: eatGarbage's two calls into the
// SL::spLandfillDeveloper singleton (stored at this+0x70 by Init
// @ 0x1000958c8-0x1000958d2). The auto-dossier labeled the slots
// ">> Sint64">> Uint64", but the caller allocates two adjacent 4-byte
// stack slots and reads dwords only — 32-bit outs (W2 fresh decode).
class ISimLandfillDeveloper {
public:
    virtual ~ISimLandfillDeveloper() = default;
    virtual void ReadRangeAsInts(Sint32& outFirst, Sint32& outSecond) = 0; // vt+0x50
    virtual Uint32 ReadValueLow32() = 0;                                    // vt+0x58
};

// W3 seams: occupant property access (vt+0x30 holder; holder vt+0x28
// property; GetPropertyValue overloads leave the out arg UNTOUCHED on
// failure) and the budget-simulator cost-line interaction.
struct CurvePropertyData {
    virtual ~CurvePropertyData() = default;
    virtual bool GetPoints(std::vector<Float32>& outXY) const = 0;
};
class OccupantPropertyHolderSeam {
public:
    virtual ~OccupantPropertyHolderSeam() = default;
    virtual bool GetPropertyValue(Uint32 propertyId, Sint64& out) const = 0;
    virtual bool GetPropertyValue(Uint32 propertyId, Float32& out) const = 0;
    virtual const CurvePropertyData* GetProperty(Uint32 propertyId) const = 0;
};
class OccupantSeam {
public:
    virtual ~OccupantSeam() = default;
    virtual const OccupantPropertyHolderSeam* GetPropertyHolder() const = 0; // vt+0x30
};
// Raw curve property dispatch observed in SetResponseCurveData @ 0x1000ac37a.
// Data retrieval precedes count retrieval; these are adapter names, not claimed
// original interface method names. No bool extraction shortcut is equivalent.
class RawCurveValueSeam {
public:
    virtual ~RawCurveValueSeam() = default;
    virtual const Float32* GetScalarData() const = 0; // vt+0x3a8
    virtual Uint32 GetScalarCount() const = 0;       // vt+0x20, unsigned /2
};
class RawCurvePropertySeam {
public:
    virtual ~RawCurvePropertySeam() = default;
    virtual const RawCurveValueSeam* GetValue() const = 0; // property vt+0x28
};
class PollutionCapacityAccessSeam {
public:
    virtual ~PollutionCapacityAccessSeam() = default;
    // uint32 helper @ 0x10004b5ac; caller passes its output directly.
    virtual bool ReadCapacity(const OccupantPropertyHolderSeam&, Uint32,
                              Uint32&) const = 0;
    // holder vt+0x28 @ 0x100099364; null denotes absent property.
    virtual const RawCurvePropertySeam* LookupCurve(
        const OccupantPropertyHolderSeam&, Uint32) const = 0;
    // pollution+0x60 -> power vt+0xf8 @ 0x10009930e.
    virtual bool IsOccupantPowered(const OccupantSeam&) const = 0;
};
// Portable adapter outcome for original null backing read @ 0x1000ac542.
// This is distinct from GetCurrentCapacity returning false or a valid zero.
struct ResponseCurveStorageFault {};
// Stack out-struct at [rbp-0x58] in CalcMaintenanceCosts (0x1c bytes).
struct BudgetCostLineData {
    Sint64 field00 = 0;      // +0x00 (opaque)
    Sint64 field08 = 0;      // +0x08 (opaque)
    Sint64 currentCost = 0;  // +0x10 — compared against cost @ 0x10009a28a
    Uint32 field18 = 0;      // +0x18 — passed through to both trailing calls
};
class BudgetCostLineEntrySeam {
public:
    virtual ~BudgetCostLineEntrySeam() = default;
    virtual void ApplyCostDelta(Sint64 delta, Uint32 field18, Sint64 cost) = 0; // vt+0x68 @ 0x10009a2b4
};
class BudgetSimulatorSeam {
public:
    virtual ~BudgetSimulatorSeam() = default;
    virtual bool GetCostLine(const OccupantPropertyHolderSeam* holder,
                             Uint32 propertyId, BudgetCostLineData& out) = 0;   // vt+0x198 @ 0x10009a27c
    virtual BudgetCostLineEntrySeam* GetCostLineEntry(
        BudgetCostLineData& line) = 0;                                          // vt+0x1c8 @ 0x10009a29b
    virtual void NotifyCostChanged(const OccupantPropertyHolderSeam* holder,
                                   Uint32 field18, Sint64 cost) = 0;           // vt+0x1a8 @ 0x10009a2c8
};
Sint64 Cvttss2siI64(Float32 v) noexcept; // helper reproducing cvttss2si NaN/overflow -> INT64_MIN @ 0x10009a1b3

// GetRequestedShare's per-tile dispatch: the binary calls this's vtable
// slot +0x48 (GetGarbageValue slot, same dispatch as IsGarbageAccumulated
// @ 0x10009a6e0) per tile @ 0x10009adb7, discarding the return.
using PollutionTileGarbageValue = Uint32 (*)(void* context, Uint32 x,
                                              Uint32 z, Uint32& outValue) noexcept;

// SC4Rect<int> as used by GetRequestedShare @ 0x10009ad5c (+0x00 minX,
// +0x04 minZ, +0x08 maxX, +0x0c maxZ — matches the loop bounds).
struct PollutionIntRect {
    Sint32 minX = 0;
    Sint32 minZ = 0;
    Sint32 maxX = 0;
    Sint32 maxZ = 0;
};

// File-scope tuning statics — binary __DATA,__common (BSS), fresh-dumped
// 0 in the image, runtime-loaded by ReadSimulatorTunableValues @
// 0x100095943-0x100095bf (W1) and read by the leaf getters (W4).
// REVIEWER FLAG: W1 typed the int scalars Uint32, W4's leaf getters return
// Sint32 for the kMax*/kLandfill globals — model as the binary's i32 loads;
// settle with fresh dumps at the reviewer gate.
inline Float32 mTrafficAirFactor = 8.0f;        // 0x100b2172c / prop 0x69501944 @ 0x100095b29
inline Sint32 kMaxGarbagePolluteForUI = 27000;   // 0x100b21730 / prop 0x49134bd4 @ 0x100095b3d
inline Sint32 kMaxAirPolluteForUI = 27000;       // 0x100b21734 / prop 0x49134bdd @ 0x100095b51
inline Sint32 kMaxWaterPolluteForUI = 32000;     // 0x100b21738 / prop 0x09134be2 @ 0x100095b65
inline Sint32 kMaxRadiationPolluteForUI = 32000; // 0x100b2173c / prop 0x89135044 @ 0x100095b79
inline Sint32 kMaxAirPollutionDelta = 50;        // 0x100b21740 / prop 0x6a406201 @ 0x100095b8d
inline Sint32 kMaxWaterPollutionDelta = 50;      // 0x100b21744 / prop 0x8a40620b @ 0x100095ba1
inline Sint32 kGarbagePollutedThreshold = 16000; // 0x100b21748 / prop 0x0913519b @ 0x100095bb5
inline Sint32 kAirPollutedThreshold = 16000;     // 0x100b2174c / prop 0xa91351a6 @ 0x100095bc9
inline Sint32 kWaterPollutedThreshold = 16000;    // 0x100b21750 / prop 0x091351ae @ 0x100095bdd
inline Sint32 kRadiationPollutedThreshold = 1;    // 0x100b21754 / prop 0x291351b5 @ 0x100095bf1
inline Sint32 kGarbageAbductionAmount = 5000;    // 0x100b21758 / prop 0x09135479 @ 0x100095c05
inline Float32 mGarbageScalingFactor = 0.125f;   // 0x100b2175c / prop 0x09135482 @ 0x100095c19
inline Sint32 kLandfillTileCapacity = 32767;     // 0x100b21760 / prop 0x4a65267a @ 0x100095c5b
inline Sint32 kMaxGarbageOnValve = 655340;       // 0x100b21764 / prop 0x4a652679 @ 0x100095c44
// The six class-level curves (BSS objects @ 0x100b21770..0x100b21798), held
// as pointers because civic_simulators.h sits BELOW tract_developer.h in the
// include graph (sc4_subfiles.h -> civic_simulators.h -> tract_developer.h
// would cycle). The objects are .cpp-owned statics; the loader assigns them.
// REVIEWER FLAG: W2's decode shows 0x100b21798 is genuinely a POINTER
// (mpGarbageTransportCapacity, deref @ 0x10009a4c7) — pointer model matches.
extern cSC4ResponseCurve* mpPropRequestCurve;             // 0x100b21770 / prop 0x69a437f6 @ 0x100095c60
extern cSC4ResponseCurve* mWpEffectAlphaCurvePtr;         // 0x100b21778 / prop 0x0a10c772 @ 0x100095c75
extern cSC4ResponseCurve* mApEffectAlphaCurvePtr;         // 0x100b21780 / prop 0x0a10c773 @ 0x100095c8a
extern cSC4ResponseCurve* mRpEffectAlphaCurvePtr;         // 0x100b21788 / prop 0x0a10c774 @ 0x100095c9f
extern cSC4ResponseCurve* mGpEffectAlphaCurvePtr;         // 0x100b21790 / prop 0x0a10c775 @ 0x100095cb4
extern cSC4ResponseCurve* mpGarbageTransportCapacity;    // 0x100b21798 / prop 0x8c19c3e9 @ 0x100095cc9

struct PollutionSimulatorSaveRecord {
    Uint16 version = 4;
    Uint32 memAddr = 0;
    std::array<SimSerializableRef, 4> gridRefs78; // +0x78.. (IID 0xc9bd939a)
    std::array<SimSerializableRef, 4> gridRefs98; // +0x98.. (IID 0xc9bd939a)
    Uint32 fieldB8 = 0;  // +0xb8
    Uint32 fieldBC = 0;  // +0xbc
    OccupantRefList listC0;  // +0xc0
    OccupantRefList listD8;  // +0xd8
    OccupantRefList listF0;  // +0xf0
    OccupantRefList list108; // +0x108
    Uint32 field208 = 0; // +0x208
    Uint32 field20C = 0; // +0x20c
    Uint32 field210 = 0; // +0x210
    Sint32 field214 = 0; // +0x214
    Sint32 field218 = 0; // +0x218
    Sint32 field21C = 0; // +0x21c
    Sint32 field220 = 0; // +0x220
    Sint32 field224 = 0; // +0x224
    Sint32 field228 = 0; // +0x228
    Uint32 field22C = 0; // +0x22c
    bool hasLegacyName = false; // version <= 2
    std::string legacyName;
    Uint16 field234 = 0;    // +0x234
    Uint32 field238 = 0;    // +0x238
    Uint32 field23C = 0;    // +0x23c
    Float32 field240 = 0.0f; // +0x240
    bool field244 = false;  // +0x244
    Uint32 field248 = 0;    // +0x248
    Sint64 field2D0 = 0;    // +0x2d0
    Sint64 field2D8 = 0;    // +0x2d8
    Uint32 field2C8 = 0;    // +0x2c8 (version >= 4 only)
    Sint64 field288 = 0;    // +0x288
    Sint64 field290 = 0;    // +0x290
    Sint64 field298 = 0;    // +0x298
    Float64 field2A0 = 0.0; // +0x2a0 (Float64 slot)
    Sint64 field2A8 = 0;    // +0x2a8
    Sint64 field2B0 = 0;    // +0x2b0
    Sint64 field2B8 = 0;    // +0x2b8
    Sint64 field250 = 0;    // +0x250
    Sint64 field258 = 0;    // +0x258
    Sint64 field260 = 0;    // +0x260
    Float64 field268 = 0.0; // +0x268 (Float64 slot)
    Sint64 field270 = 0;    // +0x270
    Sint64 field278 = 0;    // +0x278
    Sint64 field280 = 0;    // +0x280
    Sint32 field284 = 0;    // +0x284 (runtime monthly garbage cost, not serialized)
    Uint32 field2C4 = 0;    // +0x2c4 (read before +0x2c0)
    Sint32 field2C0 = 0;    // +0x2c0
    Uint32 field2CC = 0;    // +0x2cc
    Uint32 field2E4 = 0;    // +0x2e4
    RandomIteratorState randomIterator; // +0x2e8
    bool field328 = false;  // +0x328
    bool field329 = false;  // +0x329
    bool field32A = false;  // +0x32a
    bool field32B = false;  // +0x32b
    bool hasLegacyUint32 = false; // version <= 2
    Uint32 legacyUint32 = 0;
    Float32 field348 = 0.0f; // +0x348
    Sint32 field34C = 0;     // +0x34c
    Sint32 field350 = 0;     // +0x350
    SimSerializableRef simManagerRef; // +0x1c0, IID 0x223e2b67

    // Runtime callback/layout view for the six grid methods @ 0x10009a738..
    // 0x10009a940. Never serialized. Adapters resolve current runtime grids;
    // dirty bytes and the landfill counter are bound to this record on use.
    pollution_grid_kernel::State pollutionGridRuntime{};
    PollutionCapacityAccessSeam* capacityAccess = nullptr; // runtime only
    bool GetCurrentCapacity(Uint32 propertyId, const OccupantSeam& occupant,
                            Uint32& out) const; // @ 0x1000992cc
    void SetIsRadioactive(Uint32 x, Uint32 z, bool radioactive) noexcept;
    void SetAirValue(Uint32 x, Uint32 z, Sint32 value) noexcept;
    void SetWaterValue(Uint32 x, Uint32 z, Sint32 value) noexcept;
    void SetGarbageValue(Uint32 x, Uint32 z, Sint32 value) noexcept;
    bool IsActiveLandfill(Uint32 x, Uint32 z) const noexcept;
    void SetIsActiveLandfill(Uint32 x, Uint32 z, bool active) noexcept;

    // --- Swarm wave 1 additions (runtime, NOT save-serialized) ---
    Uint32 refCount24 = 0;   // +0x24 @ 0x100094d10 (AddRef), Release @ 0x100094d48
    ISimLandfillDeveloper* field70 = nullptr; // +0x70 non-owning singleton ptr @ 0x10009a4a1
    cSC4ResponseCurve* field1C8 = nullptr; // +0x1c8 runtime curve (loader-allocated) @ 0x100095ac4
    cSC4ResponseCurve* field1E8 = nullptr; // +0x1e8 runtime curve (loader-allocated) @ 0x100095ae8
    Float32 field2E0 = 0.0f;    // +0x2e0 runtime, prop 0xaa1f8ab7 @ 0x100095c33
    Uint16 field330 = 0;  // +0x330 @ 0x10009ae46 (GetWorstAirTract)
    Uint16 field332 = 0;  // +0x332 @ 0x10009ae46
    Sint32 field334 = 0;  // +0x334 @ 0x10009ae46
    Uint16 field338 = 0;  // +0x338 @ 0x10009ae66 (GetWorstWaterTract)
    Uint16 field33A = 0;  // +0x33a @ 0x10009ae66
    Sint32 field33C = 0;  // +0x33c @ 0x10009ae66
    Uint16 field340 = 0;  // +0x340 @ 0x10009ae86 (GetWorstGarbageTract)
    Uint16 field342 = 0;  // +0x342 @ 0x10009ae86
    Sint32 field344 = 0;  // +0x344 @ 0x10009ae86

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;

    // Exact cSC4PollutionSimulator::DoMonthlySimulation @ 0x100097cf2.
    void StepMonthly(Float32 garbageCostMultiplier = 0.0f) noexcept;

    // --- Swarm wave 1: leaf accessors + queries (all @-cited in the .cpp) ---
    Uint32 AddRef() noexcept;
    Uint32 Release() noexcept; // calls FinalRelease stub when refcount < 2
    bool QueryInterface(Uint32 rid, void** out) noexcept;
    bool IsGarbageUncollected() const noexcept;
    Uint32 GetTotalGarbageUncollected() const noexcept;
    Uint32 GetAverageAirValue() const noexcept;
    Uint32 GetAverageWaterValue() const noexcept;
    Uint32 GetAverageGarbageValue() const noexcept;
    Uint32 GetMonthlyIncineratorCapacity() const noexcept;
    Uint32 GetMonthlyWasteToEnergyCapacity() const noexcept;
    const Float32* GetRecyclingCenterEffect() const noexcept;
    Float32 GetGarbageScalingFactor() const noexcept;
    Sint32 GetLandfillTileCapacity() const noexcept;
    Sint32 GetGarbageCapacity() const noexcept;
    void SetGarbageCapacity(Sint32 capacity) noexcept;
    Uint32 GetGarbageAbductedPrevMonth() const noexcept;
    Sint64 GetGarbageProducedPrevMonth() const noexcept;
    Sint64 GetGarbageImportedPrevMonth() const noexcept;
    Sint64 GetGarbageExportedPrevMonth() const noexcept;
    Sint64 GetGarbageRecycledPrevMonth() const noexcept;
    Sint64 GetGarbageIncineratedPrevMonth() const noexcept;
    Sint64 GetGarbageConvertedToEnergyPrevMonth() const noexcept;
    Sint64 GetGarbageLandfilledPrevMonth() const noexcept;
    Uint32 GetWasteToEnergyBuildingCount() const noexcept;
    Uint32 GetRecyclingCenterCount() const noexcept;
    Uint32 GetWaterTreatmentPlantCount() const noexcept;
    Sint32 GetMaxAirPollutionLevelForUI() const noexcept;
    Sint32 GetMaxWaterPollutionLevelForUI() const noexcept;
    Sint32 GetMaxGarbagePollutionLevelForUI() const noexcept;
    Sint32 GetMaxRadiationPollutionLevelForUI() const noexcept;
    Uint32 GetRequesterSatisfactionId() const noexcept;
    bool UsesCustomPropSelector() const noexcept;
    bool SelectProp(const std::vector<Uint32>&, Sint32, Sint32) const noexcept;
    void GarbagePutInLandfill(Sint32 amount) noexcept;
    void AddToGarbageSupply(Sint32 amount) noexcept;
    Sint32 GetGarbageSupply() const noexcept; // dirty-writes field34C when negative
    // PollutionType: binary gates only type==3 (radiation, gridRefs78 slot)
    // @ 0x10009ac29; 0..2 = air/water/garbage via gridRefs98; NO bounds check.
    enum class PollutionType : Sint32 { Air = 0, Water = 1, Garbage = 2, Radiation = 3 };
    SimSerializableRef GetPollutionGrid(PollutionType type) const noexcept;
    void GetWorstAirTract(Sint32& outX, Sint32& outZ, Sint32& outValue) const noexcept;
    void GetWorstWaterTract(Sint32& outX, Sint32& outZ, Sint32& outValue) const noexcept;
    void GetWorstGarbageTract(Sint32& outX, Sint32& outZ, Sint32& outValue) const noexcept;
    void GetGarbageBuildings(OccupantRefList& out) const;
    Sint32 GetRequestedShare(const PollutionIntRect& rect,
                             const cSC4ResponseCurve& propRequestCurve,
                             PollutionTileGarbageValue tileGarbageValue,
                             void* tileContext) const noexcept;
    Float32 EatGarbage(Float32 garbage, Uint32& incineratorCapacityRemaining,
                       Uint32& wasteToEnergyCapacityRemaining) noexcept;
    bool CalcOccupantMaintenanceCost(const OccupantSeam& occupant,
                                     Sint64& cost) const;
    void CalcMaintenanceCosts(const std::vector<OccupantSeam*>& occupants);
    void FinalRelease() noexcept; // stub — real body @ 0x100094d64 not in wave 1
};

struct PollutionTuning {
    static constexpr cSC4TGI kExemplarTGI{kTypeExemplar, 0xe7e2c2dbu, 0xc911e35bu};
    std::unordered_map<Uint32, ExemplarProperty> properties;
    bool LoadFromExemplar(const cGZExemplar& exemplar);
    // Exact cSC4PollutionSimulator::ReadSimulatorTunableValues @ 0x100095912.
    // Writes the namespace-scope BSS tuning globals (mTrafficAirFactor etc.)
    // + the record's runtime curves/fields. exemplar == nullptr models the
    // holder-lookup failure @ 0x100095b0a: defaults + curve fallbacks stand.
    void ReadSimulatorTunableValues(PollutionSimulatorSaveRecord& record,
                                    const cGZExemplar* exemplar);
};

// ---------------------------------------------------------------------------
// cSC4ResidentialSimulator, CLSID 0x4990c013. Read 0x100204c4e (no version
// validation; branches on version >= 3, > 3, >= 7), Write 0x1002047d4
// stores 7. Tuning instance 0x69189c84.
// tDepartmentData via ReadDepartmentData 0x100205126, WriteDepartmentData 0x100204b72,
// tDepartmentData() 0x10020d51a.
// Complete method index:
//   cSC4ResidentialSimulator() @ 0x10020158e, ~cSC4ResidentialSimulator() @ 0x100201ca2 /
//   0x1002020ee / 0x1002020f8, QI @ 0x100202126, AddRef @ 0x10020218c, Release @ 0x1002021b6,
//   Init @ 0x1002021ea, ReadSimulatorTunableValues @ 0x100202f56,
//   GetOrdinanceEffects @ 0x100203648, ReadTunablePopulationValues @ 0x100203d3e,
//   Shutdown @ 0x10020417e, DoMessage @ 0x100205322,
//   DoDailySimulation @ 0x100205410, DoBuildingChanged @ 0x100206e8c,
//   GetCoveredTractList @ 0x10020793c, MarkBuildingCoverage @ 0x1002079f4,
//   GetPopulationServed @ 0x100207c6e, GetSchoolQueryData @ 0x100207f86,
//   GetHospitalQueryData @ 0x1002080ec, DoesOrdinanceAffectOccupant @ 0x100208248,
//   GetOrdinanceEffectListForOccupant @ 0x100208308,
//   GetOrdinanceCurveListsForOccupant @ 0x100208448,
//   GetPollutionTractValue @ 0x10020850e, DoPopulationChange @ 0x1002085be,
//   PostPopulationChange @ 0x100208f92, RecordHistoryData @ 0x100209134,
//   GetPropertyHolderCurveData @ 0x1002091d8, UpdateCoverageMap @ 0x10020938a,
//   GetOccupantEfficiencyRatings @ 0x100209534, GetOccupantCoverage @ 0x100209702,
//   EstimateCurrentOccupantCapacity @ 0x100209a90,
//   EstimateIdealFunding @ 0x100209c0e, ComputeOccupantGrade @ 0x100209ebc,
//   GetBaseStaffEfficiency @ 0x10020a69a, GetSchoolBuildingCount @ 0x10020a76e,
//   GetBuildings @ 0x10020a7ba, GetSchoolBuildings @ 0x10020a8a0,
//   GetHospitalBuildingCount @ 0x10020a930, GetHospitalBuildings @ 0x10020a97c,
//   GetHealthAffectingBuildings @ 0x10020aa0c,
//   ChanceOfSchoolStrike @ 0x10020aaf6, GetChanceOfStrike @ 0x10020abbe,
//   ChanceOfHealthStrike @ 0x10020acac, GetHealthSystemRating @ 0x10020ad74,
//   GetSchoolSystemRating @ 0x10020aedc, GetSystemTotals @ 0x10020b044,
//   SetOccupantFundingPercentages @ 0x10020b298,
//   GetRawOccupantFundingPercentages @ 0x10020b39a,
//   BeginStrike @ 0x10020b4fe, EndSchoolStrike @ 0x10020b5ea,
//   EndStrike @ 0x10020b646, EndHealthStrike @ 0x10020b702,
//   DoMonthlyStrikeCheck @ 0x10020b75e, UpdateStrikingState @ 0x10020b8e2,
//   GetProximityMap @ 0x10020bdde, GetTractQuotient @ 0x10020be12,
//   AverageGrids @ 0x10020bf5e, GetAverageEQGrid @ 0x10020c1c6,
//   GetAverageHQGrid @ 0x10020c274, GetEQGrids @ 0x10020c322,
//   GetHQGrids @ 0x10020c36c, GetPopulationGrids @ 0x10020c3b6,
//   GetCellLifeExpectancy @ 0x10020c400,
//   GetCellWorkforcePercent @ 0x10020c586,
//   GetGlobalWorkforcePercent @ 0x10020c70a,
//   GetEQMinAndMaxCellCoords @ 0x10020c87a,
//   GetHQMinAndMaxCellCoords @ 0x10020caa0,
//   GetGlobalEQ @ 0x10020ccc6, GetGlobalHQ @ 0x10020ce80,
//   GetGlobalLE @ 0x10020d03a, GetCellEQ @ 0x10020d19a,
//   GetCellHQ @ 0x10020d204, GetCellEQByWealth @ 0x10020d26e,
//   GetCellHQByWealth @ 0x10020d2e0,
//   GetAverageNewAgeByWealth @ 0x10020d352,
//   GetAutoBudget @ 0x10020d408, SetAutoBudget @ 0x10020d438.
// ---------------------------------------------------------------------------
struct ResidentialDepartmentData {
    Float32 field00 = 0.0f; // +0x00 (x0.01 in stream if version < 5)
    Float32 field04 = 0.0f; // +0x04 (x0.01 if version < 5)
    Float32 field08 = 0.0f; // +0x08 (x0.01 if version < 5)
    Sint32 field0C = 0;     // +0x0c
    Float32 field10 = 0.0f; // +0x10 (x0.01 in stream if version < 6)
    bool field78 = false;   // +0x78
    Sint32 field7C = 0;     // +0x7c
    SimSerializableRef ref80; // +0x80, IID 0x223e2b67
    SimSerializableRef ref88; // +0x88, IID 0x223e2b67
    Float32 field90 = 0.0f; // +0x90 (x0.01 in stream if version < 5)
    Float32 field94 = 0.0f; // +0x94
    Uint32 field98 = 0;     // +0x98
};

struct ResidentialSimulatorSaveRecord {
    Uint16 version = 7;
    Uint32 memAddr = 0;
    Sint32 field60 = 0;                 // +0x60
    SimSerializableRef gridRef68;       // +0x68, IID 0xc9bd939a
    SimSerializableRef gridRef70;       // +0x70
    SimSerializableRef gridRef78;       // +0x78
    Sint32 field80 = 0;                 // +0x80
    std::array<SimSerializableRef, 3> gridRefs88;  // +0x88..
    SimSerializableRef gridRefA0;       // +0xa0
    std::vector<SimSerializableRef> gridRefsA8;    // +0xa8: Uint32 count + refs
    std::vector<SimSerializableRef> gridRefsC0;    // +0xc0: Uint32 count + refs
    SimSerializableRef gridRefD8;       // +0xd8
    SimSerializableRef gridRefE0;       // +0xe0
    Sint32 fieldE8 = 0;                 // +0xe8
    std::array<SimSerializableRef, 3> gridRefsF0;  // +0xf0..
    std::array<SimSerializableRef, 3> gridRefs108; // +0x108..
    Float32 field124 = 0.0f;            // +0x124
    Float32 field128 = 0.0f;            // +0x128
    Uint32 field120 = 0;                // +0x120
    bool field12C = false;              // +0x12c
    Sint32 field130 = 0;                // +0x130
    ResidentialDepartmentData department160; // +0x160
    ResidentialDepartmentData department200; // +0x200
    OccupantRefList list2D0;            // +0x2d0
    OccupantRefList list2E8;            // +0x2e8
    OccupantRefList list300;            // +0x300
    std::vector<OccupantRefList> lists318; // +0x318: Uint32 count + ilists
    std::vector<OccupantRefList> lists330; // +0x330: Uint32 count + ilists
    OccupantRefList list348;            // +0x348
    ScanningIteratorState scanningIterator; // +0x56c (version >= 3)
    Uint64 field590 = 0;                // +0x590 (version >= 3)
    Sint32 field134 = 0;                // +0x134 (version > 3)
    bool field648 = false;              // +0x648 (version >= 7)
    bool field649 = false;              // +0x649 (version >= 7)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;
};

struct ResidentialTuning {
    static constexpr cSC4TGI kExemplarTGI{kTypeExemplar, 0xe7e2c2dbu, 0x69189c84u};
    Uint32 field520 = 0;     // prop 0x8a891421 -> +0x520
    Uint32 field524 = 0;     // prop 0x8a89142a -> +0x524
    Float32 field560 = 0.0f; // prop 0x4aa84161 -> +0x560
    Float32 field564 = 0.0f; // prop 0x2aa84164 -> +0x564
    Float32 field568 = 0.0f; // prop 0x6a9f6edd -> +0x568
    std::unordered_map<Uint32, ExemplarProperty> properties;
    bool LoadFromExemplar(const cGZExemplar& exemplar);
};

// ---------------------------------------------------------------------------
// cSC4TrafficSimulator, CLSID 0x6990c1aa. Exact decompilation of Read
// @ 0x10007b550, Write @ 0x10007a5f8, and ReadSimulatorTunableValues
// @ 0x100078e16. Decodes the entire transit graph (SimGrid refs, 2D byte
// grid, TrafficEdgeDensity table, TransitSwitch/TransitSwitchRule table,
// ScanningIterator, BoundaryIterator, both commute-stat blocks, TunnelInfo
// table, cSCPropertyHolder map, and FerryTerminal/Link lists) with zero
// opaque bytes across all 417 shipped .sc4 saves (1,858,642 chunk bytes in
// Big City Tutorial.sc4).
// Complete method index:
//   cSC4TrafficSimulator() @ 0x1000767e2, ~cSC4TrafficSimulator() @ 0x100077250 /
//   0x1000779c8 / 0x1000779d2, QI @ 0x100077a00, AddRef @ 0x100077a66, Release @ 0x100077a90,
//   Init @ 0x100077ac8, TrafficSourceIterator::Reset @ 0x100079f22,
//   Shutdown @ 0x100079f84, DoMessage @ 0x10007e06e,
//   DoMonthlySimulation @ 0x10007e666, DoDailySimulation @ 0x10007e778,
//   DoRoadChanged @ 0x10007eb6c, DoRailChanged @ 0x10007ef76,
//   DoTunnelChanged @ 0x10007f350, DoConnectionsChanged @ 0x10007f5f4,
//   DoLotDeactivated @ 0x100080810, DoLotActivated @ 0x100080ce4,
//   DoPostCityInit @ 0x1000817c0, AddNeighborConnection @ 0x1000820ac,
//   RemoveNeighborConnection @ 0x100082358, UpdateTravelStrategies @ 0x10008242c,
//   ReadTransitSwitchRules @ 0x100082660, RemoveUnusedPools @ 0x1000828a2,
//   HasMapExit @ 0x100082ae2, ClearSubnetworks @ 0x100082bf4,
//   UpdateSubnetworks @ 0x100082c6a, UpdateSubnetworkInfo @ 0x100082e2c,
//   WorkOnClearTripDestinations @ 0x100083508,
//   WorkOnRefreshCommuteMap @ 0x1000836ba,
//   TrafficSourceIterator::GetNext @ 0x1000838f4,
//   UpdateMassTransitBudget @ 0x1000839f0, UpdateRoadsBudget @ 0x100083b62,
//   CreatePathFinder @ 0x100083d00, ApplyRoadFunding @ 0x100083d5e,
//   ApplyMassTransitFunding @ 0x100083e40, UpdateStrikes @ 0x100083f0a,
//   CheckForAccident @ 0x1000840d8, WorkOnSimCycle @ 0x100084492,
//   UpdateIntersectionData @ 0x100084c00, WorkOnStartRecalc @ 0x100084fac,
//   WorkOnCreateMaps @ 0x100085052, WorkOnCreateFinalMap @ 0x100085a34,
//   WorkOnReduceEdgeDensities @ 0x100085cea,
//   WorkOnDumpEdgeDensities @ 0x100085f5c,
//   WorkOnFinalCalculations @ 0x100085fe4,
//   WorkOnUpdateNeighborConnections @ 0x100086086,
//   WorkOnUpdateSubnetworkInfo @ 0x100086252,
//   WorkOnFinishTripMap @ 0x10008629a, WorkOnConfirmTrips @ 0x1000866ba,
//   WorkOnReRouteTrips @ 0x100086846, WorkOnFullTrips @ 0x100086c64,
//   WorkOnFerryRoutes @ 0x1000870ee,
//   RecalculateNeighborConnectionCapacities @ 0x100087336,
//   ConfirmTrips @ 0x1000878f8, CapacityStillAvailable @ 0x100087c06,
//   SetupTrip @ 0x100087cc4, PerformPathSearch @ 0x100087f7c,
//   GetNeighborPoolPropertyHolder @ 0x1000884e8,
//   CompleteTripSourceCalculations @ 0x100088684,
//   ConfirmRoute @ 0x100088990, ApplyRoute @ 0x100088b4e,
//   GetNetworkInfo @ 0x10008954c, ApplyTrafficToDestination @ 0x1000895c6,
//   GetNeighborConnection @ 0x100089830,
//   RouteAndLinkTerminals @ 0x10008993a,
//   RouteTerminalToNeighbor @ 0x100089bf0,
//   GetTrafficEdgeDensity @ 0x100089e8c, GetCapacity @ 0x100089eda,
//   GetConnectionCount @ 0x100089efc, GetTravelTimeRatio @ 0x100089f14,
//   GetTravelStrategyPercentages @ 0x10008a000,
//   GetTrafficNeighborConnection @ 0x10008a090,
//   GetBackgroundTraffic @ 0x10008a0d4,
//   InvalidateIntersectionData @ 0x10008a3b0,
//   RemoveNetworkInfo @ 0x10008a42c,
//   RemoveBadElevatedPaths @ 0x10008a48e,
//   FullSetPathNodeConnection @ 0x10008a878,
//   AddNetworkInfo @ 0x10008a938, DoFerryTerminalActivated @ 0x10008aad8,
//   GetNeighborConnectionInnerCell @ 0x10008b05a,
//   GetFerryRouteBetweenTiles @ 0x10008b132,
//   GetAllFerryRoutes @ 0x10008b30c, GetFerryRoutesInUse @ 0x10008b40c,
//   GetWaterRoute @ 0x10008bb26, ApplyPotholeChanges @ 0x10008bb90,
//   CheckRailAccident @ 0x10008bf24, SetupPathFinderForLot @ 0x10008bf74,
//   FindHighwayReturnTile @ 0x10008c1d2,
//   TransitSwitchEntryCost @ 0x10008c282, FloodSubnetwork @ 0x10008c4e4,
//   AreLotsConnected @ 0x10008cc88, GetConnectedOccupantCount @ 0x10008cd9a,
//   GetConnectedDestinationCount @ 0x10008ceb4,
//   GetSubnetworksForLot @ 0x10008cfa2,
//   GetSubnetworksInRectangle @ 0x10008cfd2,
//   GetSubnetworksInRegion @ 0x10008d394,
//   GetOccupantCountForAllSubnetworks @ 0x10008d794,
//   GetTransitSwitches @ 0x10008d84e,
//   GetTransitSwitchQueryData @ 0x10008d8bc,
//   IsRoadDamaged @ 0x10008de5a,
//   SubnetworkInfo::SubnetworkInfo @ 0x1000917cc,
//   SubnetworkInfo::operator= @ 0x100091d3a.
// ---------------------------------------------------------------------------
struct TrafficEdgeDensityRecord {
    Uint32 cellKey = 0;                      // node +0x10
    std::array<Uint16, 16> matrixA{};        // node +0x14 (4x4 Uint16)
    Uint16 field20 = 0;                      // node +0x34
    std::array<Uint16, 16> matrixB{};        // node +0x36 (4x4 Uint16)
    Float32 legacyExtraFloat = 0.0f;         // only in version <= 19
};

struct TransitSwitchRuleRecord {
    bool flag0 = false;                      // +0x00
    std::array<bool, 4> flags1to4{};         // +0x01..+0x04
    Uint16 travelTypeIn = 0;                 // +0x06
    Uint16 travelTypeOut = 0;                // +0x08
};

struct TransitSwitchRecord {
    Uint32 key = 0;                          // node +0x10
    SimSerializableRef occupantRef{};        // node +0x18, IID 0x26bb767e
    Float32 field20 = 0.0f;                  // node +0x20
    Float32 field24 = 0.0f;                  // node +0x24
    Float32 field28 = 0.0f;                  // node +0x28
    std::vector<Uint8> legacyBytes;          // only in version <= 0x1c
    bool field2D = false;                    // node +0x2d
    Uint8 field2E = 0;                       // node +0x2e
    Uint8 field2F = 0;                       // node +0x2f
    Float32 field30 = 0.0f;                  // node +0x30
    Float32 field34 = 0.0f;                  // node +0x34
    std::vector<TransitSwitchRuleRecord> rules; // node +0x38 (version >= 0x1d)
    Float32 field54 = 0.0f;                  // node +0x54 (version >= 0x21)
    bool field58 = false;                    // node +0x58 (version >= 0x21)
};

struct TrafficStatEntry20 {
    Float32 field00 = 0.0f;                  // +0x00
    Uint32 field04 = 0;                      // +0x04
    Sint32 field08 = 0;                      // +0x08
    Sint32 field0C = 0;                      // +0x0c
    Float32 field10 = 0.0f;                  // +0x10
};

struct TrafficSubStatEntry20 {
    Uint16 field00 = 0;                      // +0x00
    Float32 field04 = 0.0f;                  // +0x04
    Sint32 field08 = 0;                      // +0x08
    Sint32 field0C = 0;                      // +0x0c
    Float32 field10 = 0.0f;                  // +0x10
};

struct TrafficCommuteBlock {
    std::vector<TrafficStatEntry20> primaryStats; // countCC (or countE4 if v < 0x23)
    std::vector<TrafficStatEntry20> extraStats;   // countC8 (version >= 0x25)
    std::vector<Uint16> primaryWords;             // countCC (+0x118)
    std::vector<Uint16> extraWords;               // countC8 (+0x132, version >= 0x25)
    Uint16 field134 = 0;                          // +0x134
    Sint32 subStatCount = 12;                     // written as 12 (3 if version < 0x22)
    std::vector<TrafficSubStatEntry20> subStats;  // +0x138
    Float32 field228 = 0.0f;                      // +0x228
    Float32 field22C = 0.0f;                      // +0x22c
    bool field230 = false;                        // +0x230
    Float32 field234 = 0.0f;                      // +0x234
    Uint8 field238 = 0x80;                        // +0x238 (version > 0x14)
};

struct TrafficTunnelInfoRecord {
    Uint16 key = 0;                          // node +0x10
    Uint8 field14 = 0;                       // node +0x14
    Uint8 field15 = 0;                       // node +0x15
    Uint16 field16 = 0;                      // node +0x16
    Uint16 field3C = 0;                      // node +0x3c
    std::vector<Float32> travelTimeFloats;   // node +0x18 (count60 floats, 9 in v40/v41)
};

struct FerryTerminalLinkRecord {
    Float32 field18 = 0.0f;                  // +0x18
    Sint16 field12 = 0;                      // +0x12
    Sint16 field14 = 0;                      // +0x14
    Uint16 field10 = 0;                      // +0x10
    std::vector<Uint8> pathBytes;            // +0x20
    Uint16 field38 = 0;                      // +0x38 (version >= 0x24)
    Uint16 field3C = 0;                      // +0x3c (version >= 0x24)
    Uint16 field3A = 0;                      // +0x3a (version >= 0x24)
    Uint16 field3E = 0;                      // +0x3e (version >= 0x24)
};

struct FerryNeighborConnectionLinkRecord {
    Float32 field18 = 0.0f;                  // +0x18
    Sint16 field10 = 0;                      // +0x10
    Sint16 field12 = 0;                      // +0x12
    Sint16 field14 = 0;                      // +0x14
    Sint16 field16 = 0;                      // +0x16
    std::vector<Uint8> pathBytesA;           // +0x20
    std::vector<Uint8> pathBytesB;           // +0x38 (version >= 0x1f)
    Uint16 field50 = 0;                      // +0x50 (version >= 0x24)
    Uint16 field54 = 0;                      // +0x54 (version >= 0x24)
    Uint16 field52 = 0;                      // +0x52 (version >= 0x24)
    Uint16 field56 = 0;                      // +0x56 (version >= 0x24)
};

struct FerryTerminalRecord {
    Uint16 legacyWord = 0;                   // only in version <= 0x1f
    std::array<Uint8, 6> bytes10to15{};      // +0x10..+0x15
    Uint8 field18Byte = 0;                   // stored as Uint32 at +0x18
    Uint16 field1C = 0;                      // +0x1c
    Uint16 field50 = 0;                      // +0x50
    Uint16 field52 = 0;                      // +0x52
    Uint8 field54 = 0;                       // +0x54 (version >= 0x26)
    Uint8 field55 = 0;                       // +0x55 (version >= 0x26)
    std::vector<FerryTerminalLinkRecord> terminalLinks;              // +0x20
    std::vector<FerryNeighborConnectionLinkRecord> neighborLinks;    // +0x38
};

struct TrafficSimulatorSaveRecord {
    Uint16 version = 41;                     // 40 in Big City Tutorial, 41 in Write
    Uint32 memAddr = 0;
    Sint32 countCC = 13;                     // network type count (13 in v40/v41)
    Sint32 countC8 = 1;                      // extra network count (1 in v40/v41)
    Sint32 countE4 = 13;                     // legacy network count (13 in v40/v41)
    Sint32 count60 = 9;                      // travel type count (9 in v40/v41)
    SimSerializableRef gridRef48{};          // +0x48, IID 0xc9bd939a
    SimSerializableRef gridRef50{};          // +0x50
    SimSerializableRef gridRef58{};          // +0x58
    SimSerializableRef gridRef60{};          // +0x60
    SimSerializableRef gridRef70{};          // +0x70
    SimSerializableRef gridRef78{};          // +0x78
    Uint32 byteGridRows = 0;                 // +0x80 outer size
    Uint32 byteGridCols = 0;                 // +0x80 inner size
    std::vector<Uint8> byteGrid;             // byteGridRows * byteGridCols bytes
    std::vector<SimSerializableRef> floatGridRefs298; // +0x298, IID 0xc9bd939a
    SimSerializableRef gridRef2B0{};         // +0x2b0, IID 0xc9bd939a
    SimSerializableRef gridRef2B8{};         // +0x2b8, IID 0xc9bd939a
    std::vector<TrafficEdgeDensityRecord> edgeDensities; // +0x2c0
    std::array<Uint32, 3> field32C{};        // +0x32c..+0x334
    std::array<Uint32, 3> field338{};        // +0x338..+0x340
    std::array<Uint32, 3> field344{};        // +0x344..+0x34c
    SimSerializableRef gridRef888{};         // +0x888, IID 0xc9bd939a
    SimSerializableRef gridRef890{};         // +0x890, IID 0xc9bd939a
    Uint32 field898 = 0;                     // +0x898
    std::vector<TransitSwitchRecord> transitSwitches; // +0x8a0
    SimSerializableRef ref8D0{};             // +0x8d0, IID 0x223e2b67
    SimSerializableRef ref8D8{};             // +0x8d8, IID 0x223e2b67
    Sint64 field940 = 0;                     // +0x940
    Float32 field948 = 0.0f;                 // +0x948
    Float32 field94C = 0.0f;                 // +0x94c (version >= 41)
    Float32 field950 = 0.0f;                 // +0x950
    Float64 field958 = 0.0;                  // +0x958
    Float64 field960 = 0.0;                  // +0x960 (version >= 41)
    Uint32 field968 = 0;                     // +0x968
    Uint8 field96C = 0;                      // +0x96c
    Sint32 field970 = 0;                     // +0x970
    Uint32 field974 = 0;                     // +0x974
    Uint32 field978 = 0;                     // +0x978
    Float32 field97C = 0.0f;                 // +0x97c
    std::vector<Uint16> set980;              // +0x980
    std::vector<Uint16> set9A8;              // +0x9a8
    Uint32 field9D0 = 0;                     // +0x9d0 (version >= 0x1a)
    Uint32 field9D4 = 0;                     // +0x9d4 (version >= 0x1a)
    Float32 field9D8 = 0.0f;                 // +0x9d8 (version >= 0x1a)
    std::vector<Uint16> set9E0;              // +0x9e0 (version >= 0x1a)
    std::vector<Uint16> setA08;              // +0xa08 (version >= 0x1a)
    Uint8 fieldA30 = 0;                      // +0xa30
    ScanningIteratorState scanningIterator;  // *+0xa38
    bool fieldA40 = false;                   // +0xa40
    Sint32 fieldA44 = 0;                     // +0xa44
    Sint32 fieldA48 = 0;                     // +0xa48
    Sint32 fieldA4C = 0;                     // +0xa4c
    Sint32 fieldA50 = 0;                     // +0xa50
    bool fieldB78 = false;                   // +0xb78
    Uint32 fieldB7C = 0;                     // +0xb7c
    Uint32 fieldB80 = 0;                     // +0xb80
    Sint16 fieldB58 = 0;                     // +0xb58
    Sint16 fieldB5A = 0;                     // +0xb5a
    BoundaryIteratorState boundaryIterator;  // *+0xb60
    bool fieldAC0 = false;                   // +0xac0
    bool fieldAA9 = false;                   // +0xaa9
    Float32 fieldAA4 = 0.0f;                 // +0xaa4
    Sint32 fieldA70 = 0;                     // +0xa70
    Sint32 fieldA74 = 0;                     // +0xa74
    Uint32 fieldABC = 0;                     // +0xabc
    std::array<Uint32, 4> fieldA94{};        // +0xa94..+0xaa0
    Uint8 fieldAA8 = 0;                      // +0xaa8
    Uint32 fieldA90 = 0;                     // +0xa90
    std::array<Sint32, 4> rectAAC{};         // +0xaac..+0xab8 (SC4Rect<int>)
    std::array<std::vector<Uint32>, 3> matrixAD4{}; // +0xad4 (3 x count60)
    Uint8 fieldB40 = 0;                      // +0xb40
    Uint8 fieldB41 = 0;                      // +0xb41
    Sint32 fieldB68 = 0;                     // +0xb68
    Sint32 fieldB6C = 0;                     // +0xb6c
    Sint32 fieldB70 = 0;                     // +0xb70 (version >= 0x19)
    Uint32 fieldB84 = 0;                     // +0xb84
    Uint32 fieldB88 = 0;                     // +0xb88
    Uint32 fieldB8C = 0;                     // +0xb8c
    Uint32 fieldB90 = 0;                     // +0xb90
    Sint32 fieldB94 = 0;                     // +0xb94 (version > 0x18)
    Sint32 fieldB98 = 0;                     // +0xb98
    Sint32 fieldB9C = 0;                     // +0xb9c
    Uint32 fieldBA0 = 0;                     // +0xba0
    Uint32 fieldBA4 = 0;                     // +0xba4
    Uint32 fieldBA8 = 0;                     // +0xba8
    Uint32 fieldBAC = 0;                     // +0xbac
    Uint32 fieldBB0 = 0;                     // +0xbb0 (version >= 0x28)
    Uint32 fieldBB4 = 0;                     // +0xbb4
    Uint32 fieldBB8 = 0;                     // +0xbb8
    Uint32 fieldBBC = 0;                     // +0xbbc
    Uint32 fieldBC0 = 0;                     // +0xbc0
    Uint32 fieldBC4 = 0;                     // +0xbc4
    std::array<TrafficCommuteBlock, 2> commuteBlocks{}; // +0x350 and +0x5c4
    std::vector<TrafficTunnelInfoRecord> tunnels;       // +0x170
    Float32 field58C = 0.0f;                 // +0x58c
    Float32 field870 = 0.0f;                 // +0x870
    Uint32 field874 = 0;                     // +0x874
    Float32 field878 = 0.0f;                 // +0x878 (version >= 0x15)
    Uint32 field87C = 0;                     // +0x87c (version >= 0x15)
    std::vector<Uint32> array908;            // +0x908 (countCC Uint32s)
    std::vector<Uint32> array31C;            // +0x31c (4 Uint32s)
    std::vector<std::pair<Uint16, SerializedPropertyHolder>> propertyHolders; // +0x150
    std::vector<std::pair<Uint32, Uint32>> pairs590_5A8; // +0x590/+0x5a8 (count60 pairs)
    Uint32 field5C0 = 0;                     // +0x5c0 (version >= 0x27)
    std::vector<FerryTerminalRecord> ferryTerminals;    // +0x1e8 (version >= 0x1e)

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool WriteV41(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;
};

struct TrafficTuning {
    static constexpr cSC4TGI kExemplarTGI{kTypeExemplar, 0xe7e2c2dbu, 0xc9133286u};
    Float32 fieldBCC = 0.0f;                 // prop 0x491332e6 -> +0xbcc
    Float32 fieldBD4 = 0.0f;                 // prop 0x29136788 -> +0xbd4
    Float32 fieldBD0 = 0.0f;                 // prop 0x29136789 -> +0xbd0
    std::array<std::vector<Float32>, 9> travelTypeNetworkSpeeds{}; // props 0x491332e7..ef
    std::vector<Float32> networkCapacities;  // prop 0xa92356b3 -> +0xe24 (13 floats)
    Float32 pctDE0 = 0.0f;                   // prop 0xa92356ae * 0.01f -> +0xde0
    Float32 pctDE4 = 0.0f;                   // prop 0xa92356af * 0.01f -> +0xde4
    Float32 fieldDE8 = 0.0f;                 // prop 0xa92356b0 -> +0xde8
    Uint32 fieldEA0 = 0;                     // prop 0xa92356b6 -> +0xea0
    Uint32 fieldEA4 = 0;                     // prop 0xa92356b7 -> +0xea4
    Uint32 fieldEA8 = 0;                     // prop 0xa92356b8 -> +0xea8
    Float32 fieldEAC = 0.0f;                 // prop 0xa92356ba -> +0xeac
    Float32 fieldF4C = 0.0f;                 // prop 0xa92356bd -> +0xf4c
    Float32 fieldF5C = 0.0f;                 // prop 0x49a2e8be -> +0xf5c
    Float32 fieldF60 = 0.0f;                 // prop 0x4a678060 -> +0xf60
    Float32 fieldF64 = 0.0f;                 // prop 0xca76013b -> +0xf64
    Float32 fieldF68 = 0.0f;                 // prop 0xea7b5f06 -> +0xf68
    Float32 fieldF6C = 1.0f;                 // prop 0x0bd86fd5 -> +0xf6c (default 1.0f)
    Float32 fieldF70 = 0.0f;                 // prop 0x8c3caef5 -> +0xf70
    Float32 fieldFF8 = 10.0f;                // prop 0x4be09ecb -> +0xff8 (default 10.0f)
    Float32 fieldFFC = 60.0f;                // prop 0x4be09ecc -> +0xffc (default 60.0f)
    std::unordered_map<Uint32, ExemplarProperty> properties;
    bool LoadFromExemplar(const cGZExemplar& exemplar);
};

// ---------------------------------------------------------------------------
// cSC4CivicBuildingSimulator, CLSID 0x0a413481.
//   Init @ 0x1000e8fb0, sGetLandmarkCount @ 0x1000e94cc, Shutdown @ 0x1000e9564,
//   Write @ 0x1000e9712, Read @ 0x1000e97f6, DoMessage @ 0x1000e9c7a,
//   DoLotActivated @ 0x1000ea076, DoMonthlySimulation @ 0x1000ea2f0,
//   DoBuildingRemoved @ 0x1000ea536, GetConditionalBuildingStatus @ 0x1000ea6fc,
//   UpdateConditionalBuildingStatus @ 0x1000ea728, IsBuildingExcluded @ 0x1000ea75c,
//   GetLandmarkCount @ 0x1000ea782.
// ---------------------------------------------------------------------------
struct ConditionalBuildingStatusRecord {
    Uint32 buildingTypeId = 0; // node +0x10
    Uint8 statusByte = 0;      // node +0x18
    Uint16 statusWord = 0;     // node +0x1a
    std::string statusText;    // node +0x20 (GZString)
};

struct CivicBuildingSimulatorSaveRecord {
    Uint16 version = 6;
    Uint32 memAddr = 0;
    std::vector<Uint32> unlockedSet;                         // +0x28
    std::vector<ConditionalBuildingStatusRecord> statuses;   // +0x50
    SimSerializableRef deptBudgetRef{};                      // +0x20, IID 0x223e2b67
    OccupantRefList zooOccupants;                            // +0x78
    OccupantRefList civicOccupants;                          // +0x90

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;

    bool IsBuildingExcluded(Uint32 buildingTypeId) const noexcept;
    const ConditionalBuildingStatusRecord* GetConditionalBuildingStatus(
        Uint32 buildingTypeId) const noexcept;
    void UpdateConditionalBuildingStatus(const ConditionalBuildingStatusRecord& rec);
};

// ---------------------------------------------------------------------------
// cSC4FloraSimulator, CLSID 0xc990c034. Read 0x1000be6d8 (version must == 2),
// Write 0x1000be5a0, ReadSimulatorTunableValues 0x1000be15a (exemplar TGI
// {0x6534284a, 0x7a4a8458, 0x1a2fdb6a}).
// Complete method index:
//   cSC4FloraSimulator() @ 0x1000bd5e6, ~cSC4FloraSimulator() @ 0x1000bd73c /
//   0x1000bd7a2 / 0x1000bd7ac, StaticInit @ 0x1000bd7da, StaticShutdown @ 0x1000bd820,
//   LoadAllStandardFloraTypes @ 0x1000bd8d2, Init @ 0x1000bd97e,
//   MakeDefaultStaticFloraData @ 0x1000bdef4, LoadObliterated @ 0x1000be292,
//   Shutdown @ 0x1000be3f4, DoMessage @ 0x1000be822,
//   DoDailySimulation @ 0x1000bf240, DoMonthlySimulation @ 0x1000bf2f0,
//   DoOccupantInserted @ 0x1000bf618, DoOccupantRemoved @ 0x1000bf72e,
//   DoTerrainAltitudeChange @ 0x1000bf7c0, EnableFlora @ 0x1000bf97c,
//   DropSeed @ 0x1000bfa36, AdjustFloraPositionFromTerrainChange @ 0x1000bfb9e,
//   GetElevationIndex @ 0x1000bfcda, GetPropagationScoreForFlora @ 0x1000bfdbe,
//   AddStaticFloraData @ 0x1000bfe74 / 0x1000c0208,
//   AddStaticFloraDataStatic @ 0x1000c0572,
//   PostFloraTypesUpdatedMessage @ 0x1000c05f8,
//   LocationIsOKForNewFlora @ 0x1000c06b6,
//   GetBestFloraTypeForLocation @ 0x1000c099c,
//   RemoveBurntFlora @ 0x1000c0ac2,
//   SimulateExistingFloraOccupant @ 0x1000c0b6e,
//   AddNewFloraOccupant(cISC4Occupant*) @ 0x1000c0f28,
//   AddNewFloraOccupant(unsigned int, float*, bool) @ 0x1000c0f48,
//   AdvanceClusterType @ 0x1000c1528,
//   GetAllFloraTypes(unsigned int*, unsigned int&) @ 0x1000c1628,
//   GetAllFloraTypes(vector&) @ 0x1000c1674,
//   GetAllFloraFamilyTypes @ 0x1000c16e4,
//   IsFloraTypeValid @ 0x1000c1788,
//   IsFloraFamilyTypeValid @ 0x1000c17de,
//   GetFloraTypesByFilter @ 0x1000c1870,
//   GetFloraTypesByFamily @ 0x1000c1b3a,
//   GetStaticFloraData @ 0x1000c1e00,
//   DemolishFloraOccupant @ 0x1000c1e4a,
//   DemolishFloraInCellArea @ 0x1000c1e66,
//   DemolishFloraInBoundingBox @ 0x1000c1f44,
//   GetCostOfOccupantDemolition @ 0x1000c2024,
//   KillFloraOccupant @ 0x1000c202c,
//   AddRef @ 0x1000c2170, Release @ 0x1000c219a,
//   QueryInterface @ 0x1000c2252, SaveObliterated @ 0x1000c22b8.
// ---------------------------------------------------------------------------
struct FloraSimulatorSaveRecord {
    Uint16 version = 2;
    Uint32 memAddr = 0;
    bool flag50 = false;                 // +0x50
    bool flag51 = false;                 // +0x51
    bool flag52 = false;                 // +0x52
    bool flag53 = false;                 // +0x53
    Float32 field54 = 0.0f;              // +0x54
    Float32 field58 = 0.0f;              // +0x58
    Sint32 field5C = 0;                  // +0x5c
    Sint32 field60 = 0;                  // +0x60
    Uint32 field64 = 0;                  // +0x64
    Uint32 field68 = 0;                  // +0x68
    Uint32 field6C = 0;                  // +0x6c
    Uint32 field70 = 0;                  // +0x70
    Uint8 field74 = 0;                   // +0x74
    RandomIteratorState randomIterator;  // +0x78
    RZRandomState rngState;              // +0xb8
    OccupantRefList floraOccupants;      // +0xd8
    Float32 fieldF0 = 0.0f;              // +0xf0
    Float32 fieldF4 = 0.0f;              // +0xf4

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;

    Sint32 StepDailyBurntFloraRemoval(Float32 dailyTickStepF8,
                                      Float32 burntRemovalRateFC) noexcept {
        // Exact cSC4FloraSimulator::DoDailySimulation @ 0x1000bf240.
        fieldF0 += dailyTickStepF8;
        if (fieldF0 >= 1.0f) {
            fieldF0 -= 1.0f;
            fieldF4 += burntRemovalRateFC;
        }
        if (fieldF4 >= 1.0f) {
            const Sint32 toRemove = static_cast<Sint32>(fieldF4);
            fieldF4 -= static_cast<Float32>(toRemove);
            return toRemove;
        }
        return 0;
    }
};

struct FloraTuning {
    static constexpr cSC4TGI kExemplarTGI{kTypeExemplar, 0x7a4a8458u, 0x1a2fdb6au};
    Float32 inverseProp00 = 0.0f;        // 1.0f / prop 0xc9dbdd00 -> +0xf8
    Float32 fieldFC = 0.0f;              // prop 0xc9dbdd01 -> +0xfc
    Sint32 field68 = 0;                  // prop 0x8761abd0 -> +0x68
    Sint32 field70 = 0;                  // prop 0x085489c7 -> +0x70
    Float32 rawProp02 = 0.0f;            // prop 0xc9dbdd02 -> +0xd0
    std::unordered_map<Uint32, ExemplarProperty> properties;
    bool LoadFromExemplar(const cGZExemplar& exemplar);
};

// ---------------------------------------------------------------------------
// cSC4MySimAgentSimulator, CLSID 0x29e37dec. Read 0x1007ac44e (version must
// == 1; version word only), Write 0x1007ac3f6.
// Complete method index:
//   cSC4MySimAgentSimulator() @ 0x1007ab82e, ~cSC4MySimAgentSimulator() @ 0x1007ab99c /
//   0x1007aba32 / 0x1007aba3c, QI @ 0x1007aba6a, AddRef @ 0x1007abad0,
//   Release @ 0x1007abafa, Init @ 0x1007abba8, s_get_sim @ 0x1007ac070,
//   s_get_sim_count @ 0x1007ac154, Shutdown @ 0x1007ac1f8,
//   DoMessage @ 0x1007ac4d2, DoBuildingAdded @ 0x1007acc0e,
//   DoFireAdded @ 0x1007acca2, DoBuildingChanged @ 0x1007acd18,
//   DoBuildingRemoved @ 0x1007acdac, DoHourChange @ 0x1007ace74,
//   DoDisasterStarted @ 0x1007acefa, DoPipeBurst @ 0x1007acf76,
//   DoTornado @ 0x1007acfce, GetActiveMySims @ 0x1007ad03e,
//   SetActiveUIPanelSim @ 0x1007ad144, GetActiveUIPanelSim @ 0x1007ad190,
//   MoveIn @ 0x1007ad1c2, TrackLocation @ 0x1007ad24c,
//   HideLocation @ 0x1007ad2ce, MySimLivesHere @ 0x1007ad324,
//   CancelMoveIn @ 0x1007ad37a.
// ---------------------------------------------------------------------------
struct MySimAgentSimulatorSaveRecord {
    Uint16 version = 1;
    Uint32 memAddr = 0;

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;
};

// ---------------------------------------------------------------------------
// cSC4NeighborsSimulator, CLSID 0xc990c0a4. Read 0x10006f528 (version must
// == 2), Write 0x10006f4b6.
// Complete method index:
//   cSC4NeighborsSimulator() @ 0x10006e61e, ~cSC4NeighborsSimulator() @ 0x10006e720 /
//   0x10006e7ee / 0x10006e7f8, QI @ 0x10006e826, AddRef @ 0x10006e88c, Release @ 0x10006e8b6,
//   Init @ 0x10006e944, update_deals_in_regional_cities @ 0x10006f258,
//   Shutdown @ 0x10006f388, DoMessage @ 0x10006f6b4,
//   DoTerrainChanged @ 0x10006f8fe, simulate_deals @ 0x10006fb6a,
//   IsValidConnectionCell @ 0x100070670,
//   ConvertConnectionCellToNeighboringRegionCell @ 0x1000706fa,
//   ConvertConnectionCellToNeighborCell @ 0x1000707f4,
//   ConvertNeighborCellToConnectionCell @ 0x100070a70,
//   FindConnection @ 0x100070ca0, FindUnvisitedConnection @ 0x100070d18,
//   HasConnection @ 0x100070d88,
//   FindConnectionOnOtherSide(ConnectionType, int, int) @ 0x100070d9a,
//   FindConnectionOnOtherSide(cISC4NeighborConnection*) @ 0x100070dfe,
//   RequestCreateConnection @ 0x100070e58, CreateConnection @ 0x100071026,
//   DeleteConnection(ConnectionType, int, int) @ 0x100071514,
//   DeleteConnection(cISC4NeighborConnection*, bool) @ 0x100071546,
//   DeleteOccupantConnection @ 0x100071842,
//   DeleteAllConnectionsOfType @ 0x100071b24,
//   FindCityOnOtherSideOfConnection @ 0x100071d38,
//   GetAllConnections @ 0x100071d94, GetConnectionsOfType @ 0x100071f3c,
//   GetConnectionsOfTypeToCity @ 0x100071fe0,
//   GetConnectionsToCity @ 0x100072096, GetAdjacentCities @ 0x100072144,
//   GetAllConnectedCities @ 0x1000726ce, GetConnectedCities @ 0x100072980,
//   FindConnectionSuitableForDeal @ 0x100072ade, ProposeDeal @ 0x100072d0e,
//   create_deal_proposition @ 0x100072e2e, ProposeDeals @ 0x100073090,
//   AddDeal @ 0x100073540,
//   FindDeals(DealType, ilist&) @ 0x100073606,
//   FindDeals(DealType, cISC4RegionalCity const*, ilist&) @ 0x1000736aa,
//   GetTotalMonthlyNeighborDealIncome @ 0x100073760,
//   GetTotalMonthlyNeighborDealExpense @ 0x1000737e6,
//   UpdateCommodityStats @ 0x10007386c, GetDealCost @ 0x100074040,
//   GetDealMaxAmount @ 0x100074434.
// ---------------------------------------------------------------------------
struct NeighborsSimulatorSaveRecord {
    Uint16 version = 2;
    Uint32 memAddr = 0;
    std::vector<SimSerializableRef> connectionRefs; // +0x30, IID 0x0773fef2

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;
};

// ---------------------------------------------------------------------------
// cSC4OrdinanceSimulator, CLSID 0xa990c08e.
//   GZCOM_CreatecSC4OrdinanceSimulator @ 0x100798b50, GZCOM_cSC4OrdinanceSimulatorCLSID @ 0x100798baa,
//   dtor @ 0x100798bc4 / 0x100798c02, QI @ 0x100798c52, AddRef @ 0x100798cb8, Release @ 0x100798ce2,
//   StaticShutdown @ 0x100798d9e, Init @ 0x100798db6, LoadAllStandardOrdinances @ 0x100798ecc,
//   Shutdown @ 0x100798f26, Write @ 0x100798f6e, Read @ 0x10079903a, DoMessage @ 0x1007992c6,
//   DoMonthlySimulation @ 0x100799302, SimulationBegin @ 0x1007994d0,
//   AddOrdinance @ 0x100799602, RemoveOrdinance @ 0x10079981c,
//   CreateStandardOrdinance @ 0x1007998e0, AddOrdinanceDataStatic @ 0x100799aa4,
//   IsOrdinanceEnabled @ 0x100799b24, SetOrdinanceEnabled @ 0x100799b48,
//   IsOrdinanceAvailable @ 0x100799b7a, IsOrdinanceOn @ 0x100799b9e,
//   IsOrdinanceAvailableButOff @ 0x100799bc2, SetOrdinanceAvailable @ 0x100799c02,
//   GetOrdinanceAvailableCount @ 0x100799d9a, GetOrdinanceAvailableArray @ 0x100799e14,
//   SetOrdinanceOn @ 0x100799edc, GetOrdinanceOnCount @ 0x10079a01e,
//   GetOrdinanceOnArray @ 0x10079a098, GetOrdinanceIDArray @ 0x10079a160,
//   GetOrdinanceIDByName @ 0x10079a206, GetOrdinanceByID @ 0x10079a226,
//   GetOrdinanceByName @ 0x10079a266, GetOrdinanceMonthlyExpense @ 0x10079a580,
//   GetOrdinanceMonthlyIncome @ 0x10079a58a, GetCumulativeOffset @ 0x10079a594,
//   GetCumulativeFactor @ 0x10079a712, GetGZCLSID @ 0x10079a87a.
// ---------------------------------------------------------------------------
struct OrdinanceSimulatorSaveRecord {
    Uint16 version = 4;
    Uint32 memAddr = 0;
    Sint64 monthlyExpense = 0;                      // +0x20
    Sint64 monthlyIncome = 0;                       // +0x28
    std::vector<SimSerializableRef> ordinanceRefs;  // +0x38, IID 0xe4fda3d4
    Sint64 pendingAdjustment = 0;                   // +0x30

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;
    void StepMonthly(std::span<const Sint64> activeOrdinanceMonthlyCosts) noexcept;
    Sint64 GetOrdinanceMonthlyExpense() const noexcept { return monthlyExpense; }
    Sint64 GetOrdinanceMonthlyIncome() const noexcept { return monthlyIncome; }
    static Sint32 ComputeCumulativeOffset(
        std::span<const SerializedPropertyHolder> activeOrdinanceHolders,
        Uint32 propertyId, Uint32 index) noexcept;
    static Float32 ComputeCumulativeFactor(
        std::span<const SerializedPropertyHolder> activeOrdinanceHolders,
        Uint32 propertyId, Uint32 index) noexcept;
};

// ---------------------------------------------------------------------------
// cSC4WeatherSimulator, CLSID 0x2990c1bc. Read 0x1000e43a4 (version <= 2),
// Write 0x1000e4016 (version 2), ReadSimulatorTunableValues 0x1000e3b30
// (exemplar TGI {0x6534284a, 0x7a4a8458, 0x1a2fdb6b}).
// Complete method index:
//   cSC4WeatherSimulator() @ 0x1000e320e, ~cSC4WeatherSimulator() @ 0x1000e34cc /
//   0x1000e35f2 / 0x1000e35fc, Init @ 0x1000e362a,
//   GenerateMoistureVariation @ 0x1000e3e1c, Shutdown @ 0x1000e3f3c,
//   DoPostCityInit @ 0x1000e3fcc, DoCompleteSimulation @ 0x1000e3ff0,
//   PositionToCell @ 0x1000e487a, CellToPosition @ 0x1000e4896,
//   CellToStandardCityCell @ 0x1000e48ba, StandardCityCellToCell @ 0x1000e48d0,
//   GetCurrentSeason @ 0x1000e48f8, GetCurrentSeasonMonth @ 0x1000e491c,
//   GetMoistureValueMapPtr @ 0x1000e4a52, GetMoistureType @ 0x1000e4a62,
//   SetManualMoistureAddition @ 0x1000e4a74, GetAmbientWind @ 0x1000e4b80,
//   GetWindAtCell @ 0x1000e4bc4, DoMessage(cGZMessage&) @ 0x1000e4bd8,
//   DoMessage(cIGZMessage2*) @ 0x1000e4be8,
//   DoWeeklySimulation @ 0x1000e4dd2, DoMonthlySimulation @ 0x1000e4f16,
//   SimulateCellTemperature @ 0x1000e4ffa,
//   PostMessageServerMessage @ 0x1000e5122,
//   SimulateMoisture @ 0x1000e51ee, UpdateMoisture @ 0x1000e623e,
//   RemoveWaterThatIsNotAtCityEdge @ 0x1000e6866,
//   CalculateSeaTemperatureModeration @ 0x1000e6994,
//   GetNewCurrentAmbientValues @ 0x1000e6b66,
//   WriteTemperatureValue @ 0x1000e6e82,
//   AddRef @ 0x1000e6f3e, Release @ 0x1000e6f76,
//   QueryInterface @ 0x1000e7068, CellWidthX @ 0x1000e70ec,
//   CellWidthZ @ 0x1000e70f8, CellCountX @ 0x1000e7104,
//   CellCountZ @ 0x1000e710e.
// ---------------------------------------------------------------------------
struct WeatherByteGrid2D {
    Uint32 rows = 0;
    Uint32 cols = 0;
    std::vector<Uint8> cells; // rows * cols bytes
};

struct WeatherSimulatorSaveRecord {
    Uint16 version = 2;
    Uint32 memAddr = 0;
    Sint32 gridCellsX = 0;                       // +0x50
    Sint32 gridCellsZ = 0;                       // +0x54
    Float32 cellWidthM = 0.0f;                   // +0x58
    Float32 cellHeightM = 0.0f;                  // +0x5c
    Float32 invCellWidth = 0.0f;                 // +0x60
    Float32 invCellHeight = 0.0f;                // +0x64
    Float32 worldWidthM = 0.0f;                  // +0x68
    Float32 worldHeightM = 0.0f;                 // +0x6c
    Sint32 cellsPerStandardCellX = 0;            // +0x70
    Sint32 cellsPerStandardCellZ = 0;            // +0x74
    Sint32 field78 = 0;                          // +0x78
    Sint32 field7C = 0;                          // +0x7c
    Sint32 field80 = 0;                          // +0x80
    Sint32 field84 = 0;                          // +0x84
    Float32 field88 = 0.0f;                      // +0x88
    Float32 field8C = 0.0f;                      // +0x8c
    Float32 field90 = 0.0f;                      // +0x90
    Float32 field94 = 0.0f;                      // +0x94
    WeatherByteGrid2D grid98;                    // +0x98 (vector<vector<uint8>>)
    WeatherByteGrid2D boolGridB0;                // +0xb0 (vector<vector<bool>>)
    std::vector<std::array<Sint32, 4>> rectsC8;  // +0xc8 (vector<cRZRect>)
    RandomIteratorState randomIterator;          // +0xe0
    Sint32 field120 = 0;                         // +0x120
    Sint32 field124 = 0;                         // +0x124
    Float32 field128 = 0.0f;                     // +0x128
    Float32 field12C = 0.0f;                     // +0x12c
    Float32 field130 = 0.0f;                     // +0x130
    WeatherByteGrid2D moistureGrid138;           // +0x138 (vector<vector<uint8>>)
    WeatherByteGrid2D moistureVarGrid150;        // +0x150 (vector<vector<uint8>>)
    Float32 field168 = 0.0f;                     // +0x168
    Float32 field16C = 0.0f;                     // +0x16c
    Float32 field170 = 0.0f;                     // +0x170
    Float32 field174 = 0.0f;                     // +0x174
    Float32 field178 = 0.0f;                     // +0x178
    Float32 field17C = 0.0f;                     // +0x17c
    Float32 field180 = 0.0f;                     // +0x180
    Uint32 field184 = 0;                         // +0x184
    Float32 field18C = 0.0f;                     // +0x18c
    std::array<Float32, 2> vec190{};             // +0x190 (cS3DVector2)
    Float32 ambientWindSpeed = 0.0f;             // +0x198
    std::array<Float32, 2> ambientWindDir{};     // +0x19c (cS3DVector2)
    Float32 field1A4 = 0.0f;                     // +0x1a4
    std::array<Float32, 2> vec1A8{};             // +0x1a8 (cS3DVector2)
    Float32 field1B0 = 0.0f;                     // +0x1b0
    std::array<Float32, 2> vec1B4{};             // +0x1b4 (cS3DVector2)
    RZRandomState rngState;                      // +0x1c0
    Sint32 field1D8 = 0;                         // +0x1d8

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;
    Uint8 GetMoistureValue(Float32 worldX, Float32 worldZ) const noexcept;
    Float32 GetHumidity() const noexcept;
};

struct WeatherTuning {
    static constexpr cSC4TGI kExemplarTGI{kTypeExemplar, 0x7a4a8458u, 0x1a2fdb6bu};
    Sint32 field78 = 1;                  // prop 0xa7607d70 -> +0x78 (clamped >= 1)
    Sint32 field7C = 1;                  // prop 0xa7607d71 -> +0x7c (clamped >= 1)
    Sint32 field80 = 0;                  // prop 0xa7607d72 -> +0x80
    Float32 field88 = 0.0f;              // prop 0xa7607d73 -> +0x88
    Float32 field8C = 0.0f;              // prop 0xa7607d74 -> +0x8c (clamped 0..1)
    Float32 field90 = 0.0f;              // prop 0xa7607d75 -> +0x90
    Sint32 field124 = 0;                 // prop 0xa7607d76 -> +0x124
    Float32 field128 = 0.0f;             // prop 0xa7607d77 -> +0x128 (clamped >= 0)
    Float32 field12C = 0.0f;             // prop 0xa7607d78 -> +0x12c
    Float32 field130 = 0.0f;             // prop 0xa7607d79 -> +0x130
    Float32 field16C = 0.0f;             // prop 0xa7607d7a -> +0x16c
    Float32 field170 = 0.0f;             // prop 0xa7607d7b -> +0x170 (clamped 0..1)
    Float32 field174 = 0.0f;             // prop 0xa7607d7c -> +0x174
    Float32 field178 = 0.0f;             // prop 0xa7607d7d -> +0x178
    Float32 field17C = 0.0f;             // prop 0xa7607d7e -> +0x17c
    Float32 field180 = 0.0f;             // prop 0xa7607d7f -> +0x180
    Uint32 field184 = 0;                 // prop 0xa7607d80 -> +0x184
    Uint32 field188 = 0;                 // prop 0xa7607d81 -> +0x188
    Float32 field18C = 0.0f;             // prop 0xa7607d86 -> +0x18c
    Float32 field190 = 0.0f;             // prop 0xa7607d87 -> +0x190
    Float32 field194 = 0.0f;             // prop 0xa7607d88 -> +0x194
    Float32 field1DC = 0.0f;             // prop 0xa7607d89 -> +0x1dc
    std::unordered_map<Uint32, ExemplarProperty> properties;
    bool LoadFromExemplar(const cGZExemplar& exemplar);
};

// ---------------------------------------------------------------------------
// cSC4GameOfLifeSimulator (non-GZCOM helper class:
//   Init @ 0x1002fc4c0, Shutdown @ 0x1002fc70c, RandomizeCells @ 0x1002fc81a,
//   DoEdit @ 0x1002fc964, DoNextIteration @ 0x1002fca8e).
//   Also cISC4TrafficSimulator::FerryRouteInfo::FerryRouteInfo @ 0x100092fec.
//   Exact Conway's Game of Life with toroidal wraparound and cell-age
//   increment up to 0xbf (191).
// ---------------------------------------------------------------------------
class cSC4GameOfLifeSimulator {
public:
    cSC4GameOfLifeSimulator(Uint32 width = 0, Uint32 height = 0);

    void Resize(Uint32 width, Uint32 height);
    void Init(Uint32 width, Uint32 height, bool randomize = false) {
        // Exact cSC4GameOfLifeSimulator::Init @ 0x1002fc4c0.
        Shutdown();
        Resize(width, height);
        (void)randomize;
    }
    void Shutdown() noexcept {
        // Exact cSC4GameOfLifeSimulator::Shutdown @ 0x1002fc70c.
        mFrontBuffer.clear();
        mBackBuffer.clear();
    }
    void SetEditMode(Sint32 mode) noexcept { mEditMode = mode; }
    void SetPaused(bool paused) noexcept { mPaused = paused; }
    bool DoEdit(Uint32 x, Uint32 y) noexcept;
    void DoNextIteration() noexcept;
    Uint8 GetCell(Uint32 x, Uint32 y) const noexcept;

private:
    bool mPaused = false;            // +0x00
    Sint32 mEditMode = 0;            // +0x04 (0 = set alive, 1 = clear)
    Uint32 mWidth = 0;               // +0x08
    Uint32 mHeight = 0;              // +0x0c
    std::vector<Uint8> mFrontBuffer; // +0x10
    std::vector<Uint8> mBackBuffer;  // +0x18
};

// ---------------------------------------------------------------------------
// cSC4RigidBodySimulator2D (non-GZCOM helper class: AdvanceTime @ 0x100700bfc,
// DoCollisions @ 0x100700ee2).
// ---------------------------------------------------------------------------
struct RigidBody2DObject {
    Float32 posX = 0.0f;             // +0x10
    Float32 posY = 0.0f;             // +0x14
    Float32 velX = 0.0f;             // +0x18
    Float32 velY = 0.0f;             // +0x1c
    Float32 zHeight = 0.0f;          // +0x20
    Float32 zVel = 0.0f;             // +0x24
    Float32 angle = 0.0f;            // +0x28
    Float32 angularVel = 0.0f;       // +0x2c
    Float32 halfExtentX = 1.0f;      // +0x30
    Float32 halfExtentY = 1.0f;      // +0x34
    Float32 linearDragForward = 0.0f; // +0x40
    Float32 linearDragLateral = 0.0f; // +0x44
    Float32 angularDrag = 0.0f;      // +0x48
    bool applyDrag = false;          // +0x4c
    Float32 accelX = 0.0f;           // +0x50
    Float32 accelY = 0.0f;           // +0x54
    Uint32 collisionCount = 0;       // +0x5c
    bool active = true;              // +0x86
};

class cSC4RigidBodySimulator2D {
public:
    void AddObject(const RigidBody2DObject& obj);
    void AdvanceTime(Float32 dt) noexcept;
    void DoCollisions() noexcept;
    const std::vector<RigidBody2DObject>& Objects() const noexcept { return mObjects; }

private:
    std::vector<RigidBody2DObject> mObjects;
};

// ---------------------------------------------------------------------------
// cSC4Simulator, CLSID 0x2990c1e5. Exact decompilation of:
//   GZCOM_CreatecSC4Simulator @ 0x1001ef250, GZCOM_cSC4SimulatorCLSID @ 0x1001ef292,
//   cSC4Simulator() @ 0x1001ef29e, ~cSC4Simulator() @ 0x1001ef55c / 0x1001ef6c2 / 0x1001ef6e8,
//   QueryInterface @ 0x1001ef732, AddRef @ 0x1001ef79e, Release @ 0x1001ef7e4,
//   Init @ 0x1001ef894, ReadSimulatorTunableValues @ 0x1001efbdc,
//   DoSetSimSpeed @ 0x1001efd52, Shutdown @ 0x1001efdc0,
//   Write @ 0x1001eff54, Read @ 0x1001f002c,
//   DoMessage @ 0x1001f0170, DoCheatCode @ 0x1001f0438,
//   SimulateTimePassage @ 0x1001f05d6, SetSimSpeed @ 0x1001f093e,
//   PostMessageServerMessage @ 0x1001f0964, SetSimTime @ 0x1001f0a1e,
//   GetSimStartDate @ 0x1001f0a28, GetSimStartDateStatic @ 0x1001f0a4c,
//   SendAgentMessage @ 0x1001f0a6a, OnTick @ 0x1001f0b22,
//   SimulateAgents @ 0x1001f0c58, SimulateIdleTime @ 0x1001f0e18,
//   BeginNewInterval @ 0x1001f0f54, GetSimDate @ 0x1001f1062,
//   Pause @ 0x1001f1100, HiddenPause @ 0x1001f1128, EmergencyPause @ 0x1001f1150,
//   Resume @ 0x1001f11a0, HiddenResume @ 0x1001f11c4, EmergencyResume @ 0x1001f11e2,
//   AddAgent @ 0x1001f1224, RemoveAgent @ 0x1001f13e6 / 0x1001f14ac,
//   RemoveAllAgents @ 0x1001f14f2 / 0x1001f1546, EnumerateAgentsByName @ 0x1001f15a4,
//   GetAgentEnabled @ 0x1001f178e, SetAgentEnabled @ 0x1001f182c,
//   SetCityEstablished @ 0x1001f18ca, GetAnimationTimeDilation @ 0x1001f1952,
//   GetSimDate @ 0x1001f19ca, GetSimDateNumber @ 0x1001f19d8,
//   IsPaused @ 0x1001f19ea, IsHiddenPaused @ 0x1001f19f8,
//   IsEmergencyPaused @ 0x1001f1a06, IsAnyPaused @ 0x1001f1a14,
//   GetSimSpeed @ 0x1001f1a30, GetSimTime @ 0x1001f1a3a,
//   SetMaxMillisecondsPerTick @ 0x1001f1a44, GetGZCLSID @ 0x1001f1a50.
// ---------------------------------------------------------------------------
struct SimulatorTuning {
    static constexpr cSC4TGI kExemplarTGI{kTypeExemplar, 0xe7e2c2dbu, 0xe9590e52u};
    // +0x128: period in ms per simSpeed 0..2 (defaults: 5000, 1000, 100; prop 0x89590e6d)
    std::array<Uint32, 3> speedPeriodMs{5000u, 1000u, 100u};
    // +0x138: animation time dilation per speed (defaults: 0, 1, 1.5, 2, 1; prop 0x89590e6e)
    std::vector<Float32> animationTimeDilation{0.0f, 1.0f, 1.5f, 2.0f, 1.0f};
    std::unordered_map<Uint32, ExemplarProperty> properties;

    bool LoadFromExemplar(const cGZExemplar& exemplar);
};

// cSC424HourClock::ReadSimulatorTunableValues @ 0x100185130 (shares exemplar 0xe9590e52),
// cSC4ZoneManager::ReadSimulatorTunableValues @ 0x1001985b4 (exemplar 0xe9482490),
// cSC4Nation::ReadSimulatorTunableValues @ 0x1002891b2 (exemplar 0x4a1f6115),
// plus LandValue @ 0x1000c564e, Crime @ 0x1001fd622, Aura @ 0x100218c72,
// Police @ 0x1000a2b74, Fire @ 0x1002faa86, Power @ 0x10020ed2c,
// Plumbing @ 0x10009c782, Pollution @ 0x100095912, Residential @ 0x100202f56,
// BuildingDevelopment @ 0x10014d71e.

struct SimulatorAgentInfo {
    Uint64 targetId = 0;      // +0x20 (cIGZMessageTarget2*)
    bool enabled = true;      // +0x28
    std::string name;         // +0x30 (cRZString)
    Uint32 flags = 0;         // +0x58 (bit 0 = established city, bit 1 = unestablished city)
};

struct SimulatorSaveRecord {
    static constexpr std::array<Sint32, 4> kSimSpeedTicksTable{30, 20, 15, 0};

    Uint16 version = 4;                          // <= 4 in Read; Write stores 4
    Uint32 memAddr = 0;
    Uint8 isInOnTick45 = 0;                      // +0x45
    Uint8 isCityEstablished46 = 0;               // +0x46
    Sint32 pauseCount48 = 0;                     // +0x48
    Sint32 simSpeed54 = 0;                       // +0x54 (0=Turtle, 1=Rhino, 2=Cheetah, 3=Stopped)
    Sint32 simTime58 = 0;                        // +0x58
    Uint32 maxMillisecsPerTick90 = 15;           // +0x90
    std::array<Uint32, 11> intervalCounters{};   // +0xc8..+0xf0 (index 2=min, 3=hour, 4=day, 5=week, 6=month, 7=year)
    std::array<bool, 11> intervalPending{};      // +0xf4..+0xfe
    Uint32 julianDayNumber = 2451545u;           // +0x100 (cRZDate: 2000-01-01 = 2451545)

    // Runtime speed-gating & pause fields:
    Sint32 hiddenPauseCount4C = 0;               // +0x4c
    Sint32 emergencyPauseCount50 = 0;            // +0x50
    bool speedChangeAllowed5C = true;            // +0x5c
    Sint32 rateMode94 = 4;                       // +0x94 (2=minute, 3=hour, 4=day)
    Uint32 intervalPeriodMs98 = 5000;            // +0x98
    Sint32 targetTicksPerSec9C = 30;             // +0x9c
    std::array<std::vector<SimulatorAgentInfo>, 11> agentSets{}; // +0x150..+0x257

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;

    bool DoSetSimSpeed(Sint32 speed, const SimulatorTuning& tuning) noexcept;
    bool SetSimSpeed(Sint32 speed, const SimulatorTuning& tuning) noexcept;
    void SetSimTime(Sint32 t) noexcept { simTime58 = t; }
    Sint32 GetSimSpeed() const noexcept { return simSpeed54; }
    Sint32 GetSimTime() const noexcept { return simTime58; }
    Uint32 GetSimDateNumber() const noexcept { return julianDayNumber; }
    void SetMaxMillisecondsPerTick(Uint32 ms) noexcept { maxMillisecsPerTick90 = ms; }
    bool IsPaused() const noexcept { return pauseCount48 > 0; }
    bool IsHiddenPaused() const noexcept { return hiddenPauseCount4C > 0; }
    bool IsEmergencyPaused() const noexcept { return emergencyPauseCount50 > 0; }
    bool IsAnyPaused() const noexcept {
        return pauseCount48 > 0 || hiddenPauseCount4C > 0 || emergencyPauseCount50 > 0;
    }
    void Pause() noexcept { ++pauseCount48; }
    void HiddenPause() noexcept { ++hiddenPauseCount4C; }
    void EmergencyPause() noexcept {
        Pause();
        speedChangeAllowed5C = false;
        ++emergencyPauseCount50;
    }
    void Resume() noexcept {
        if (speedChangeAllowed5C) {
            --pauseCount48;
        }
    }
    void HiddenResume() noexcept { --hiddenPauseCount4C; }
    void EmergencyResume() noexcept {
        speedChangeAllowed5C = true;
        Resume();
        --emergencyPauseCount50;
    }
    bool AddAgent(Uint64 targetId, Sint32 interval, std::string_view name, Uint32 flags);
    bool RemoveAgent(Uint64 targetId, Sint32 interval) noexcept;
    bool RemoveAgent(Uint64 targetId) noexcept;
    bool RemoveAllAgents(Sint32 interval) noexcept;
    bool RemoveAllAgents() noexcept;
    void EnumerateAgentsByName(std::vector<std::string>& outNames) const;
    bool GetAgentEnabled(std::string_view name) const noexcept;
    void SetAgentEnabled(std::string_view name, bool enabled) noexcept;
    void SetCityEstablished(bool established) noexcept;
    Float32 GetAnimationTimeDilation(const SimulatorTuning& tuning,
                                     bool vehicleManagerActive = false) const noexcept;
    Uint32 ComputeTickTimeBudgetMs(Uint32 elapsedSinceLastTickMs) noexcept;
    bool StepSpeedGatedTimePassage(Uint32 elapsedIntervalMs,
                                   Uint32& outResetTimerCount) noexcept;
};

} // namespace opensc4
