#pragma once

// cSTETerrain runtime foundation — M3 foundational slice + M3 brush pipeline.
//
// Clean-room C++20 reimplementation of the terrain object lifecycle, the
// water-surface / altitude queries, and the brush dispatch pipeline
// (ApplyBrush -> ApplyBrushComponents -> ApplyBrushAtLocation -> BrushFn*),
// all reconstructed from the shipped 64-bit macOS binary (Sim City 4 Deluxe
// Editionsub). Every behavioral method cites the machine-code address it was
// reconstructed from, per AGENTS.md / the rule of authenticity. Float op
// order, branch structure, thresholds, and clamping reproduce the binary
// exactly; nothing is "improved" here.
//
// Intrinsic state layout (binary `this+offset` -> member):
//   +0x18 bool   mInitialized          (IsInitialized @ 0x100740e24)
//   +0x1c Uint32 mRefCount
//   +0x38        mSurfaceWater         (cSTESurfaceWater* ; water depth grid)
//   +0x40 Sint32 mCellCountX           (CellCountX @ 0x100740e3c)
//   +0x44 Sint32 mCellCountZ           (CellCountZ @ 0x100740e46)
//   +0x48 Sint32 mMaxVertexIndexX      (GetAltitude clamp, cmova @ 0x1007389d3)
//   +0x4c Sint32 mMaxVertexIndexZ      (GetAltitude clamp, cmova @ 0x1007389da)
//   +0x50 Sint32 mVertexCountX         (row stride; GetVertIndex @ 0x100740e50)
//   +0x54 Sint32 mVertexCountZ
//   +0x68/+0x6c Float32 world width/height (cellCount * 16.0f)
//   +0x74 Sint32 mVertexCount          (vertexCountX * vertexCountZ)
//   +0x88        mAltitude grid        (36-byte/vertex records, altitude @ +0)
//   +0x90 Float32 mMinHeight           (MinHeight @ 0x100740ea4)
//   +0x94 Float32 mMaxHeight           (MaxHeight @ 0x100740eb2)
//   +0x9c Float32 mSeaLevel            (GetSeaLevel @ 0x1007394ce)
//   +0xdc Sint32 mBrushStrengthIndex   (BrushRadius @ 0x100734db5; ctx strength
//                                       index selection @ 0x100731d97)
//   +0x150       mFloraBrushInfo       (Flora gate: != 0 required @ 0x100732017)
//   +0x230/+0x240 volcanic-ash region list (AddVolcanicAshRegion @ 0x100731628;
//                                       0x28-byte nodes, size at +0x240)
//   +0x268/+0x278 decal map            (BeginBrushScrub clear @ 0x100734a8e)

#include "opensc4/gzcom.h"
#include "opensc4/rz_random.h"

#include <array>
#include <cstddef>
#include <deque>
#include <list>
#include <map>
#include <unordered_map>
#include <utility>
#include <vector>

namespace opensc4 {

// ---------------------------------------------------------------------------
// Brush dispatch pipeline types (M3 terrain wave). Layouts reconstructed from
// ApplyBrush @ 0x100731c50, ApplyBrushComponents @ 0x100731d10,
// ApplyBrushAtLocation @ 0x1007321b8/0x1007327de, GetTextureValue @ 0x100733436
// and the BrushFn* bodies @ 0x10073083e..0x10073340c.
// ---------------------------------------------------------------------------

// SC4Rect<int> as used by the brush pipeline: {left, top, right, bottom}.
// SC4Rect<int>::UnionRect @ 0x100212e68: this = out; left/top = min of the two
// operands, right/bottom = max (cmovl/cmovg pairs).
struct SC4RectI {
    Sint32 left = 0;
    Sint32 top = 0;
    Sint32 right = 0;
    Sint32 bottom = 0;
};

// Exact union of two rects (SC4Rect<int>::UnionRect @ 0x100212e68 semantics).
inline SC4RectI UnionRectValues(const SC4RectI& a, const SC4RectI& b) noexcept {
    SC4RectI r;
    r.left = (a.left < b.left) ? a.left : b.left;
    r.top = (a.top < b.top) ? a.top : b.top;
    r.right = (a.right > b.right) ? a.right : b.right;
    r.bottom = (a.bottom > b.bottom) ? a.bottom : b.bottom;
    return r;
}

// Exact SC4Rect<int>::IntersectRect(out, a, b) @ 0x100314ce8: full
// overlap gate (fail if a.right < b.left / a.left > b.right /
// a.bottom < b.top / a.top > b.bottom, signed jl/jg/jl/jle @
// 0x100314cec..0x100314d0a); on failure the out rect is zeroed
// ({0,0,0,0}, xorl + two movq @ 0x100314d0c..0x100314d15) and false is
// returned; on success out = {max(lefts), max(tops), min(rights),
// min(bottoms)} (cmovg/cmovl pairs @ 0x100314d17..0x100314d55).
inline bool IntersectRectValues(SC4RectI& out, const SC4RectI& a,
                                const SC4RectI& b) noexcept {
    if (a.right < b.left || a.left > b.right || a.bottom < b.top ||
        a.top > b.bottom) {
        out = SC4RectI{0, 0, 0, 0};
        return false;
    }
    out.left = (a.left > b.left) ? a.left : b.left;
    out.top = (a.top > b.top) ? a.top : b.top;
    out.right = (a.right < b.right) ? a.right : b.right;
    out.bottom = (a.bottom < b.bottom) ? a.bottom : b.bottom;
    return true;
}

// SC4Point<float>: {x, z} pair (ComputeRectAroundPt writes five of these,
// the first four forming the rotated footprint polygon @ 0x10073255c).
struct SC4PointF {
    Float32 x = 0.0f;
    Float32 z = 0.0f;
};

// kEmptyRect @ 0x100b904c0 — {INT_MAX, INT_MAX, INT_MIN, INT_MIN}, the
// identity element for UnionRect (value from the constant-pool copy
// @ 0x100a1c0c0). Applied to tBrushParameters::rect by ApplyBrush
// @ 0x100731c61..0x100731c73 and to the per-sweep rect by ApplyBrushAtLocation
// @ 0x1007322e1/0x10073287b.
inline constexpr SC4RectI kEmptyRect{0x7fffffff, 0x7fffffff,
                                     static_cast<Sint32>(0x80000000u),
                                     static_cast<Sint32>(0x80000000u)};

// Minimal cS3DTextureBufferInfo model — the fields the brush path reads:
//   +0x08 Sint32 width, +0x0c Sint32 height (ApplyBrushAtLocation @
//   0x100732203/0x100732207, GetTextureValue @ 0x100733498/0x10073349c)
//   +0x20 pointer whose [+0x00] is the texture byte data (ApplyBrushAtLocation
//   @ 0x100732247, GetTextureValue @ 0x10073356e).
struct S3DTextureBufferInfo {
    Sint32 width = 0;    // +0x08
    Sint32 height = 0;   // +0x0c
    const Uint8* data = nullptr; // +0x20 -> [+0x00] (modeled)
};

// Texture-set holder referenced by tBrushComponent +0x08. The binary calls
// vtable+0x18 on it to obtain the cS3DTextureBufferInfo* (ApplyBrushComponents
// @ 0x100731f3c..0x100731f48, GetTextureValue @ 0x100733454/0x10073345b);
// modeled here as a stored pointer.
struct TerrainTextureSet {
    const S3DTextureBufferInfo* bufferInfo = nullptr; // vtable+0x18 result (modeled)
};

// cSTETerrain::tBrushComponent — 0x78 = 120 bytes in the binary (the
// ApplyBrushComponents component-loop stride @ 0x10073215f; array bounded by
// desc+0x28..+0x30). Field offsets derived from ApplyBrushComponents
// @ 0x100731d10, GetTextureValue @ 0x100733436, and the BrushFn* target
// compare at +0x48 (BrushFnRaiseLower @ 0x1007329cc, BrushFnRain @ 0x100730849):
struct TerrainBrushComponent {
    Uint32 resourceId = 0;                   // +0x00 (GUID low word for the spResMan load gate @ 0x100731ee1)
    Sint32 effect = 0;                       // +0x04 (effect type 0..10, jump table @ 0x10073218c)
    TerrainTextureSet* textureSet = nullptr; // +0x08 (vtable+0x18 -> cS3DTextureBufferInfo*)
    Uint8 useDirectionAngle = 0;             // +0x10 (selects desc+0x88 over desc+0x84 as the angle)
    std::vector<Float32> radii;              // +0x18 (per-strength radii, scaled by params.radiusScale)
    std::vector<Float32> strengths;          // +0x30 (per-strength strengths, scaled by params.strengthScale)
    Uint8 flag48 = 0;                        // +0x48 (raise/lower mode byte compared against ctx.flag)
    Uint32 maxDecalCount = 0;                // +0x4c (BrushFnPaint @ 0x100732e5a / BrushFnPlaceEffect
                                             //      @ 0x1007330a8 unsigned decal-map count gate)
    Uint32 decalResourceId = 0;              // +0x50 (BrushFnPaint GUID low word @ 0x100732e6d/0x100732e7b)
    Float32 jitterAmplitude = 0.0f;          // +0x54 (BrushFnPaint jitter-draw gate @ 0x100732e8e)
    Sint32 effectParam58 = 0;                // +0x58 (BrushFnPlaceEffect +0x138 target vt+0x38 arg @ 0x1007330bb)
    Sint32 effectParam5c = 0;                // +0x5c (BrushFnPlaceEffect +0x138 target vt+0x38 arg @ 0x1007330be)
    std::vector<Uint32> field60;             // +0x60 VESTIGIAL: a vector<Uint32>
                                             //      the property loader zeroes at the component-loop
                                             //      head @ 0x10073ab38 and copies empty into every
                                             //      pushed component @ 0x10073ae30 (ReadBrushComponent
                                             //      Properties @ 0x10073aafe); required only to fill
                                             //      the 0x78 stride. Always empty after loading.
};

// ---------------------------------------------------------------------------
// Modeled boundaries for unreconstructed callees of the wave-2 brush effects
// (BrushFnFlora/Paint/PlaceEffect, InitFloraBrushInfo). Each stand-in models
// ONLY what the reconstructed callers observe; the real callees are not
// decompiled. clean-room fallback (not authentic).
// ---------------------------------------------------------------------------

// Flora brush-info type entry as consumed by InitFloraBrushInfo
// @ 0x1007385e2: the +0x148 provider's per-type object exposes an attribute
// byte at +0x180 (masked & 3, must equal 2 to survive the filter
// @ 0x100738678..0x100738682).
struct TerrainFloraBrushType {
    Uint32 id = 0;
    Uint8 attr180 = 0;
};

// One BrushFnFlora @ 0x100732a0e placement attempt: the jittered cell point
// {rndX, GetAltitude(rndX, rndZ), rndZ} plus the RandomUint32Uniform(+0x150)
// flora-index roll. Modeled observation record for the +0x148 flora-manager
// create call (vt+0x90 @ 0x100732b73, not reconstructed).
struct TerrainFloraPlacement {
    Float32 x = 0.0f;
    Float32 y = 0.0f;
    Float32 z = 0.0f;
    Sint32 floraIndex = 0;
};

// Clean-room stand-in for the decal/paint target at cSTETerrain +0x130
// (BrushFnPaint null gate @ 0x100732d39; vtable+0x20 place-decal callee
// @ 0x100732f6a with {GUID 0x891B0E1A7AB50E44, comp+0x50}, the jittered
// point, ctx.amount and the jitter draw — not reconstructed). Modeled as a
// recorded placement list.
struct TerrainDecalPlacementTarget {
    struct Placement {
        Uint32 resourceId = 0;   // component +0x50 (GUID low word)
        Float32 x = 0.0f, z = 0.0f; // jittered point in the vertex's cell
        Float32 amount = 0.0f;   // ctx.amount (+0x18)
        Float32 jitter = 0.0f;   // comp+0x54-gated RandomDoubleRangeUniform draw
    };
    std::vector<Placement> placements;
};

// Clean-room stand-in for the effect target at cSTETerrain +0x138
// (BrushFnPlaceEffect vtable+0x38 eligibility query @ 0x1007330c5 with
// (comp+0x58, comp+0x5c, &out) returning bool, then the out-object calls
// vt+0x40(&transform)/vt+0x18(0)/vt+0x10 @ 0x1007331e8..0x100733208 — not
// reconstructed). The query is modeled as a settable gate.
struct TerrainEffectPlacementTarget {
    bool queryResult = true;
    struct Placement {
        Sint32 effectParam58 = 0;     // component +0x58
        Sint32 effectParam5c = 0;     // component +0x5c
        Float32 x = 0.0f, y = 0.0f, z = 0.0f; // (rndX, max(seaLevel, GetAltitude), rndZ)
    };
    std::vector<Placement> placements;
};

// ---------------------------------------------------------------------------
// Clean-room boundary models for the brush description property-loading wave
// (ReadBrushDescriptions @ 0x10073b49c .. ReadBrushDescription @ 0x10073aefe
// .. GetEffectIDProperty @ 0x100738dcc). Only the surface the reconstructed
// cSTETerrain methods observe is modeled: cGZPersistResourceKey (dword id at
// key+0x08), cISCPropertyHolder + the cSCPropertyHelp::GetPropertyValue
// overloads (float&, vector<float>&, uint&, vector<uint>&, byte&, bool& — on
// failure the out arg is left UNTOUCHED and false returned), the
// cSCResFilterFixedGroupAndTypeId(0x6534284a, 0x6a01fc2a) exemplar filter,
// spResMan (vt+0x18 exemplar load, vt+0x88 filtered query) and the resource
// iterator (vt+0x38 apply-callback, vt+0x10 release). clean-room fallback
// (not authentic).
// ---------------------------------------------------------------------------

// cGZPersistResourceKey as the brush loader observes it: the exemplar id
// dword at key+0x08 (the map key — ReadBrushDescription @ 0x10073af25).
struct SC4PersistResourceKey {
    Uint32 id = 0; // binary key+0x08
};

// One exemplar property value, type-tagged. A typed GetPropertyValue read
// succeeds iff the stored kind matches; a missing id or kind mismatch leaves
// the out arg untouched and returns false.
struct SCExemplarPropertyValue {
    enum class Kind : Uint8 { Float, FloatArray, Uint, UintArray, Byte, Bool };
    Kind kind = Kind::Float;
    Float32 floatValue = 0.0f;
    std::vector<Float32> floatArray;
    Uint32 uintValue = 0;
    std::vector<Uint32> uintArray;
    Uint8 byteValue = 0;
    bool boolValue = false;
};

// cISCPropertyHolder + cSCPropertyHelp::GetPropertyValue contract fixture.
// clean-room fallback (not authentic).
class BrushPropertyHolderFixture {
public:
    std::map<Uint32, SCExemplarPropertyValue> properties;

    bool GetPropertyValue(Uint32 propertyId, Float32& out) const;
    bool GetPropertyValue(Uint32 propertyId, std::vector<Float32>& out) const;
    bool GetPropertyValue(Uint32 propertyId, Uint32& out) const;
    bool GetPropertyValue(Uint32 propertyId, std::vector<Uint32>& out) const;
    bool GetPropertyValue(Uint32 propertyId, Uint8& out) const;
    bool GetPropertyValue(Uint32 propertyId, bool& out) const;

    // holder vt+0x28 GetProperty(id) surface (ReadTuningParameters @
    // 0x10073ddb1: non-null iff a value is stored under the id; the null
    // gate is je @ 0x10073ddb7). clean-room fallback (not authentic).
    bool HasProperty(Uint32 propertyId) const;

    // Verify-suite setters.
    void SetFloat(Uint32 propertyId, Float32 v);
    void SetFloatArray(Uint32 propertyId, std::vector<Float32> v);
    void SetUint(Uint32 propertyId, Uint32 v);
    void SetUintArray(Uint32 propertyId, std::vector<Uint32> v);
    void SetByte(Uint32 propertyId, Uint8 v);
    void SetBool(Uint32 propertyId, bool v);
};

// Resource-iterator surface (vt+0x38 apply-callback over the query matches,
// vt+0x10 release @ 0x10073b505..0x10073b51f). clean-room fallback.
class BrushResourceIteratorModel {
public:
    virtual ~BrushResourceIteratorModel() = default;
    virtual void ForEach(void (*callback)(const SC4PersistResourceKey&, void*),
                         void* userData) = 0;
    virtual void Release() = 0;
};

// spResMan surface (null gate @ 0x10073b4a7 / 0x10073af5b, vt+0x18 exemplar
// load {key, 0xa52160f5} @ 0x10073af77, vt+0x88 filtered query @
// 0x10073b4fb). clean-room fallback (not authentic).
class BrushResourceSourceModel {
public:
    virtual ~BrushResourceSourceModel() = default;
    virtual bool LoadExemplar(const SC4PersistResourceKey& key,
                              const BrushPropertyHolderFixture*& out) = 0;
    virtual Sint32 Query(BrushResourceIteratorModel*& outIter) = 0;
};

// Concrete fixture source + iterator for the verify suite. clean-room
// fallback (not authentic).
class BrushFixtureResourceIterator : public BrushResourceIteratorModel {
public:
    explicit BrushFixtureResourceIterator(std::vector<SC4PersistResourceKey> keys)
        : mKeys(std::move(keys)) {}
    void ForEach(void (*callback)(const SC4PersistResourceKey&, void*),
                 void* userData) override {
        for (const SC4PersistResourceKey& k : mKeys) callback(k, userData);
    }
    void Release() override { delete this; }
private:
    std::vector<SC4PersistResourceKey> mKeys;
};

class BrushFixtureResourceSource : public BrushResourceSourceModel {
public:
    // Brush id -> exemplar property holder, in ascending key order (the
    // binary's iterator order is the resource manager's; ascending is the
    // deterministic fixture choice).
    std::map<Uint32, BrushPropertyHolderFixture> exemplars;
    // Forced vt+0x88 query status: 0 exercises the query-failure gate
    // (no callback runs, je @ 0x10073b503).
    Sint32 queryStatus = 1;
    // Observation counters (not in the binary).
    Uint32 loadCalls = 0;
    Uint32 loadFailures = 0;

    bool LoadExemplar(const SC4PersistResourceKey& key,
                      const BrushPropertyHolderFixture*& out) override {
        ++loadCalls;
        const auto it = exemplars.find(key.id);
        if (it == exemplars.end()) {
            ++loadFailures;
            out = nullptr;
            return false;
        }
        out = &it->second;
        return true;
    }
    Sint32 Query(BrushResourceIteratorModel*& outIter) override {
        outIter = nullptr;
        if (queryStatus == 0) return 0;
        std::vector<SC4PersistResourceKey> keys;
        for (const auto& kv : exemplars) keys.push_back(SC4PersistResourceKey{kv.first});
        outIter = new BrushFixtureResourceIterator(std::move(keys));
        return queryStatus;
    }
};

// cSTETerrain::tBrushDescription — value type of sBrushDescriptionMap
// @ 0x100b71e20 (map node value at node+0x28, key at node+0x20). Offsets:
//   +0x04 jitter byte (ApplyBrushComponents @ 0x100731dad)
//   +0x08 rotation step (added to/subtracted from +0x84 per application)
//   +0x10 float* radii by strength index (BrushRadius @ 0x100734db5)
//   +0x28/+0x30 component array begin/end (0x78-byte components)
//   +0x40 Float32 costs[5] (bit-copied from property 0x8a020101 by
//       ReadBrushWholeProperties @ 0x10073b316..0x10073b32a; consumer
//       GetCostPerBrushApplication @ 0x100738d76 reads
//       cvttss2si [node+0x68+4*strengthIdx], default 32 when the brush is
//       missing)
//   +0x54/+0x5c/+0x64/+0x6c Uint32 pairs (2 uints each, UI props
//       0xaa2ed800..0xaa2ed803 via GetEffectIDProperty @ 0x10073b3d1..)
//   +0x74 / +0x78 Uint32 UI props 0xaa2ed804 / 0xaa2ed805 (loader defaults
//       0x071e5700 / 0x071e5705 @ 0x10073b469 / 0x10073b488)
//   +0x84 rotation accumulator (wrapped into [0, 2pi)), +0x88 direction angle
//   +0x7c/+0x80 scrub sound ids (BeginBrushScrub @ 0x100734b1e, EndBrushScrub
//   @ 0x1007387d3); +0x00 sound/scrub property id.
struct TerrainBrushDescription {
    Sint32 id = 0;                       // +0x00
    Uint8 useJitter = 0;                 // +0x04
    Float32 rotationStep = 0.0f;         // +0x08
    std::vector<Float32> radii;          // +0x10
    std::vector<TerrainBrushComponent> components; // +0x28..+0x30 (0x78 stride)
    Float32 costs[5] = {};                // +0x40..+0x54
    Uint32 uiEffectPairs[8] = {};         // +0x54/+0x5c/+0x64/+0x6c (2 uints each)
    Uint32 uiSoundIdA = 0;                // +0x74 (prop 0xaa2ed804; loader default
                                          //      0x071e5700, NOT a struct default)
    Uint32 uiSoundIdB = 0;                // +0x78 (prop 0xaa2ed805; loader default
                                          //      0x071e5705, NOT a struct default)
    Sint32 scrubSoundId = 0;             // +0x7c
    Sint32 endScrubSoundId = 0;          // +0x80
    Float32 rotationAccum = 0.0f;        // +0x84
    Float32 directionAngle = 0.0f;       // +0x88 (atan2f(dz, dx) + pi/2 @ 0x100731e65)
};

// cISTETerrain::tBrushParameters — offsets derived from ApplyBrush @ 0x100731c50
// and ApplyBrushComponents @ 0x100731d10:
//   +0x00/+0x04 previous point, +0x08/+0x0c current point (world coords)
//   +0x10 strength index override (>= 0 wins over the terrain's +0xdc)
//   +0x14 re-display bool (gates the vtable+0x228 call @ 0x100731cda)
//   +0x18 strength scale, +0x1c radius scale
//   +0x20 int passed through to the vtable+0x228 call @ 0x100731cf3
//   +0x24..+0x30 SC4Rect<int> out-rect (kEmptyRect-init @ 0x100731c61..0x100731c73)
//   +0x34 target height (raw-copied into ctx+0x1c @ 0x100731d8a; read as a
//   float by BrushFnLevel @ 0x100733262 / BrushFnLakeBed @ 0x10073338e)
struct TerrainBrushParameters {
    Float32 prevX = 0.0f, prevZ = 0.0f; // +0x00/+0x04
    Float32 x = 0.0f, z = 0.0f;        // +0x08/+0x0c
    Sint32 strengthIndex = -1;          // +0x10
    bool reDisplay = false;             // +0x14
    Float32 strengthScale = 1.0f;       // +0x18
    Float32 radiusScale = 1.0f;         // +0x1c
    Sint32 reDisplayArg = 0;            // +0x20
    SC4RectI rect = kEmptyRect;         // +0x24..+0x30
    Float32 targetHeight = 0.0f;        // +0x34
};

// cSTETerrain::tBrushContext — 0x38 bytes. Built as a stack local by
// ApplyBrushComponents @ 0x100731d10 (base at [rbp-0x80]) and passed by
// reference to ApplyBrushAtLocation and the BrushFn* member functions.
struct TerrainBrushContext {
    Float32 x = 0.0f;             // +0x00 <- params.x       (8-byte copy @ 0x100731d79)
    Float32 z = 0.0f;            // +0x04 <- params.z
    Float32 prevX = 0.0f;        // +0x08 <- params.prevX   (8-byte copy @ 0x100731e82)
    Float32 prevZ = 0.0f;        // +0x0c <- params.prevZ
    Sint32 strengthIndex = 0;     // +0x10 (params.strengthIndex if >= 0, else terrain +0xdc @ 0x100731d92)
    Uint8 flag = 0;              // +0x14 — per-pixel texture alpha byte. ODDITY:
                                  // ApplyBrushComponents never initializes this
                                  // slot; only the texture overload of
                                  // ApplyBrushAtLocation stores it (@ 0x10073248c),
                                  // so the plain (no-texture) sweep reads whatever
                                  // the stack held in the binary. Deterministically
                                  // modeled as 0 (value-initialization).
    Float32 amount = 0.0f;       // +0x18 <- strengths[idx] * params.strengthScale @ 0x100731f83
    Float32 targetHeight = 0.0f; // +0x1c <- params.targetHeight (raw copy @ 0x100731d8a)
    const TerrainBrushComponent* target = nullptr;   // +0x20 (current component @ 0x100731edd)
    const TerrainBrushParameters* params = nullptr;  // +0x28 @ 0x100731f9f
    const TerrainBrushDescription* desc = nullptr;   // +0x30 @ 0x100731fa3 (read by GetTextureValue)
};

class cSTETerrain; // forward (TerrainBrushMemberFn holds a pointer-to-member)

// Itanium x86_64 member-function pointer as consumed by the brush sweep: the
// binary's 16-byte {fn word, this-adjustment} pair. Bit 0 of the fn word set
// means a virtual call: vtable[fnWord - 1] invoked on (this + adj)
// (ApplyBrushAtLocation @ 0x100732473..0x1007324b4 texture / 0x100732910..0x100732932
// plain); otherwise the direct call (this + adj)->fn(vertex, ctx). Every
// jump-table case sets fn = &cSTETerrain::BrushFnX, adj = 0
// (`xor eax, eax; lea BrushFnX, rbx` @ 0x10073200e..0x100732099).
struct TerrainBrushMemberFn {
    using Fn = bool (cSTETerrain::*)(Sint32, const TerrainBrushContext&);
    Fn     fn = nullptr;       // fn word (direct member fn when !isVirtual)
    bool   isVirtual = false;  // fn word bit 0
    Sint64 adj = 0;            // second word (this-adjustment)
};

// Clean-room stand-in for cSTESurfaceWater (the object referenced via
// cSTETerrain +0x38; cSTESurfaceWater itself is not yet decompiled). Binary
// field access seen by BrushFnRain / GetWaterLevel / IsWaterVertex:
//   +0x28 Float32* water depth per vertex (36-byte records in the binary,
//          depth at record +0; modeled as a flat vector)
//   +0x70 Uint8 gate flag (cSTESurfaceWater::Rain @ 0x100730820)
//   +0x74 Sint32 current water layer/vertex index (BrushFnRain @ 0x100730862)
// The spring accumulator (record +4 per CreateSpring @ 0x100730b16) is
// modeled as its own flat vector.
struct TerrainSurfaceWaterState {
    std::vector<Float32> depth;          // +0x28 (modeled)
    std::vector<Float32> spring;         // record +4 accumulator (modeled)
    // Sediment carried by the water at each vertex (record +0x1c, written by
    // DoWaterErosion @ 0x1007338e7/0x10073399f/0x100733a3e; modeled flat).
    std::vector<Float32> sediment;
    Uint8 flag70 = 0;                     // +0x70 (Rain gate @ 0x100730820)
    Sint32 currentVertex = 0;            // +0x74
    // cSTESurfaceWater::ResetWaterToSeaLevel(SC4Rect<int> const&) (called
    // from cSTETerrain::SetSeaLevel @ 0x10073e58b) — body not reconstructed.
    // clean-room fallback (not authentic): recorded no-op; the depth
    // recompute is not modeled.
    Sint32 resetToSeaLevelCalls = 0;
    Sint32 resetToSeaLevelLastRect[4] = {0, 0, 0, 0};

    // Exact cSTESurfaceWater::CreateSpring(int) @ 0x100730b16: adds 1.0f
    // (const @ 0x100a15090) to the spring accumulator of the vertex record
    // (depth-array base +0x28, record offset +4). No bounds check in the
    // binary; the vector model guards.
    bool CreateSpringAtVertex(Sint32 vertex) noexcept {
        if (vertex < 0 || static_cast<std::size_t>(vertex) >= spring.size()) return false;
        spring[static_cast<std::size_t>(vertex)] =
            spring[static_cast<std::size_t>(vertex)] + 1.0f;
        return true;
    }
};

// Brush-function effect-type dispatch table, reconstructed from the
// ApplyBrushComponents @ 0x100731d10 jump table @ 0x10073218c (effect types
// 0..10; `cmp rax, 0xa; ja` bound @ 0x100731ff1/0x100731ff5). Jump-table
// targets (offsets relative to the table base 0x10073218c):
//   0 -> 0x1007320e1 (BrushFnRaiseLower), 1 -> 0x100732017 (Flora gate),
//   2 -> 0x100732033 (BrushFnSmoothen),   3 -> 0x100732041 (BrushFnPaint),
//   4 -> 0x10073204f (BrushFnPlaceEffect),5 -> 0x10073205d (BrushFnLevel),
//   6 -> 0x100732068 (ApplyWaterErosionBrush), 7 -> 0x100732085 (BrushFnVolcanoAsh),
//   8 -> 0x100732090 (BrushFnLakeBed),   9 -> 0x10073209b (cSTESurfaceWater::Rain),
//   10 -> 0x100731ebe (cSTESurfaceWater::CreateSpring)
enum class TerrainBrushEffect : Sint32 {
    RaiseLower   = 0, // BrushFnRaiseLower      @ 0x1007329cc
    Flora        = 1, // BrushFnFlora           @ 0x100732a0e
    Smoothen     = 2, // BrushFnSmoothen        @ 0x100732bd2
    Paint        = 3, // BrushFnPaint           @ 0x100732d22
    PlaceEffect  = 4, // BrushFnPlaceEffect     @ 0x100732f92
    Level        = 5, // BrushFnLevel           @ 0x100733230
    WaterErosion = 6, // ApplyWaterErosionBrush @ 0x1007332ae
    VolcanoAsh   = 7, // BrushFnVolcanoAsh       @ 0x100733338
    LakeBed      = 8, // BrushFnLakeBed          @ 0x100733360
    Rain         = 9, // cSTESurfaceWater::Rain  @ 0x100730820 (-> BrushFnRain @ 0x10073083e)
    Spring       = 10 // cSTESurfaceWater::CreateSpring @ 0x100730b16
};

// ---------------------------------------------------------------------------
// Surface-cover cluster types (M3 terrain wave). Layouts reconstructed from
// AddLandTypeAtVertex @ 0x100731338, AdjustLandTypeRatios @ 0x10073173a,
// AddTemperatureAndMoistureToSurfaceCoverEntry @ 0x100731418,
// AddToTerrainTypeTextureList @ 0x10073159e, DecideSurfaceCover @ 0x100733e48
// and SprayVolcanicAsh @ 0x100734724.
// ---------------------------------------------------------------------------

// One land-type slot of the 36-byte per-vertex record: {u16 type, u16 value}
// at record +0x1a + 4*i / +0x1c + 4*i for slot i = 0..2 (i.e. slots at record
// +0x16, +0x1a, +0x1e — AddLandTypeAtVertex @ 0x10073136f..0x1007313d4,
// ClearSurfaceTextures @ 0x100733e14..0x100733e21 clears the three type
// words). Slots are compacted: filled slots precede empty ones
// (AdjustLandTypeRatios stops at the first empty slot @ 0x10073178b/0x1007317b4).
struct TerrainLandTypeSlot {
    uint16_t type = 0;   // record +0x1a + 4*i
    uint16_t value = 0;  // record +0x1c + 4*i (quantized int(value*255+1))
};

// cSC4ResponseCurve @ 0x1000ac4f6 — NOT reconstructed. clean-room fallback
// (not authentic): a settable control-point list evaluated by piecewise-linear
// interpolation, clamped to the first/last output; an empty curve returns 0.
// The binary stores 32-byte curve objects at tSurfaceCoverInfo+0x10+32*s.
struct SurfaceCoverResponseCurve {
    std::vector<std::pair<Float32, Float32>> points;
    Float32 Evaluate(Float32 x) const noexcept;
};

// cSTETerrain::tSurfaceCoverInfo — value type of sSurfaceCoverTextures
// @ 0x100b71cd8 (std::unordered_map<unsigned int, tSurfaceCoverInfo>), as far
// as DecideSurfaceCover/SprayVolcanicAsh observe it:
//   +0x18 Float32 multiplier (mulss 0x18(%rax) @ 0x100734643 / @ 0x100734895)
//   +0x10 + 32*strength: per-strength response curves (GetOutputValue this
//       @ 0x100734623..0x10073462f, 0x1007349b4..0x1007349bd)
//   +0xc0 + 4*strength: Sint32 spread (movl 0xc0(%rax,%rsi,4) @ 0x100734340,
//       0xb0(%rcx,%rax,4) with rcx = node+0x10 @ 0x1007347b3)
//   +0xd4 + 4*strength: Float32 curve parameter (squared, scaled by
//       0.00390625f @ 0x100a1b65c into the spread-loop max distance squared
//       @ 0x1007343fa..0x10073440f)
struct TerrainSurfaceCoverInfo {
    Float32 multiplier = 0.0f;                       // +0x18
    std::vector<SurfaceCoverResponseCurve> curves;   // +0x10+32*s (modeled)
    std::vector<Sint32> spread;                       // +0xc0+4*s (modeled)
    std::vector<Float32> curveParam;                  // +0xd4+4*s (modeled)
};

// cSTETerrain::sTexturesUsedInTerrainType @ 0x100b71cb0 entry: 24 bytes per
// terrain type {list header, size at +0x10} (lea r14,[rax+2*rax] and
// incq [r15+8*r14+0x10] @ 0x1007315b2..0x100731614). The list holds the
// texture ids used by the type, sorted ascending and unique (insert walk
// @ 0x1007315c1..0x1007315e8).
struct TerrainTypeTextureList {
    std::list<Uint32> textures; // ascending, unique
    Sint32 count = 0;            // +0x10 (incremented only on insert)
};

// Clean-room stand-in for the moisture/weather holder at cSTETerrain +0x140
// (class not reconstructed). vtable+0x80 receives an int[4] world rect
// (UpdateMoistureValues @ 0x100733dc4..0x100733dc7, rect * {16,16,16,16}
// @ 0x100a29bd0); vtable+0x98 receives (worldX, worldZ) and returns the
// moisture code combined into the surface-cover index @ 0x100734267..0x100734289.
// clean-room fallback (not authentic): settable constant code + recorded
// update rects.
struct TerrainMoistureSource {
    Sint32 moistureCode = 0;
    Sint32 lastUpdateRect[4] = {0, 0, 0, 0};
    Sint32 updateCallCount = 0;
    void UpdateRect(const Sint32 r[4]) noexcept {
        for (Sint32 i = 0; i < 4; ++i) lastUpdateRect[i] = r[i];
        ++updateCallCount;
    }
    Sint32 ValueAt(Float32, Float32) const noexcept { return moistureCode; }
};

// Clean-room observation record for the cSTETerrainView3D static / +0x110
// calls made by the surface-cover cluster (cSTETerrainView3D is not
// reconstructed). clean-room fallback (not authentic).
struct TerrainView3DObservation {
    Sint32 changeTerrainTypeCalls = 0;      // ChangeTerrainType(int) @ 0x1007467ca
    Sint32 changeTerrainTypeLastArg = 0;
    Sint32 setUpTexturesCalls = 0;          // SetUpTexturesForCurrentTerrainTypeAndZoom(int) @ 0x10074688c
    Sint32 setUpTexturesLastArg = 0;
    Sint32 markTextureSpanInfoDirtyCalls = 0; // MarkTextureSpanInfoDirty(int,int) @ 0x10074d7d4
    Sint32 markTextureSpanInfoDirtyLastLo = 0;
    Sint32 markTextureSpanInfoDirtyLastHi = 0;
    // cSTETerrainView3D::SetTriangleIndicesForCell(int, int) @ 0x10074f376
    // (tail-jmp from FlipInternalTriangulation @ 0x1007388b1 with the cell z
    // in esi and the cell x in edx — arg order swapped relative to the
    // flip call).
    Sint32 setTriangleIndicesCalls = 0;
    Sint32 setTriangleIndicesLastArg0 = 0; // first int (cell z)
    Sint32 setTriangleIndicesLastArg1 = 0; // second int (cell x)
    void Reset() noexcept { *this = TerrainView3DObservation{}; }
};

// ---------------------------------------------------------------------------
// Shore-cliff cluster types (M3 terrain wave). Layouts reconstructed from
// AnalyzeShoreAndCreateCliffs @ 0x1007317fe (28-byte push_back stride
// add rax, 0x1c @ 0x10073192f; element count via imul 0xb6db6db7
// @ 0x100731b06) and CreateShoreCliff @ 0x100731ab0.
// ---------------------------------------------------------------------------

// cSTETerrain::tCliffVertInfo — 28 bytes: the sea neighbor that seeded the
// shore walk, the shore vertex, and the five inland probe vertex indices
// along the ray seaNeighbor -> vertex (probe[k] = 2*vertex - seaNeighbor +
// k*(vertex - seaNeighbor), written by the probe loop @
// 0x1007318d0..0x1007318fd into stack slots rbp-0x58..rbp-0x48).
struct TerrainCliffVertInfo {
    Sint32 seaNeighbor = 0;  // +0x00 (stack slot rbp-0x60)
    Sint32 vertex = 0;       // +0x04 (stack slot rbp-0x5c)
    Sint32 probe[5] = {};    // +0x08..+0x1c
};

// Clean-room stand-in for the height fields of the cSTETerrainView3D object
// at cSTETerrain +0x110 (class not reconstructed): the candidate max/min
// floats read by ComputeMinAndMaxAltitudeForTerrainRedisplay
// @ 0x100735b52/0x100735b5a. clean-room fallback (not authentic).
struct TerrainView3DHeightRange {
    Float32 maxField94 = 0.0f;  // view +0x94 (candidate max)
    Float32 minField98 = 0.0f;  // view +0x98 (candidate min)
};

// Clean-room stand-in for SL::spViewUtilities @ 0x100b214d8 and
// SC4ViewUtilities::GetVisibleEdges(int&, int&) @ 0x10017376a (not
// reconstructed). `present` models the null global (the whole visible-edge
// block is skipped when the global is null @ 0x100735add..0x100735ae0);
// edge0/edge1 model the two out-ints GetVisibleEdges fills.
// clean-room fallback (not authentic).
struct TerrainViewUtilitiesModel {
    bool present = false;
    Sint32 edge0 = 0;
    Sint32 edge1 = 0;
};

// ---------------------------------------------------------------------------
// cSTETerrain — runtime foundation.
// ---------------------------------------------------------------------------
class cSTETerrain {
public:
    cSTETerrain() = default; // @ 0x100737020 (zero-init of state, modeled)
    virtual ~cSTETerrain() = default;

    // Foundation slice of Init @ 0x100739cb0: sets up the intrinsic state
    // layout (dimensions, vertex counts, altitude grid, water depth grid).
    // The binary additionally wires up the city manager, surface water, flora
    // brush info, texture lists, and brush parameters — deferred to follow-up
    // tickets (dependencies not yet decompiled).
    bool Init(Sint32 cellCountX, Sint32 cellCountZ);
    void Shutdown();

    // ---- Water surface + altitude queries (the runtime methods the DoIdle
    //      work already depends on) ----

    // Exact cSTETerrain::CellHasWater @ 0x10073562c. Out-of-range (x >=
    // cellCountX or z >= cellCountZ) returns false. In range, tests the water
    // depth at the four cell corners (x,z), (x+1,z), (x,z+1), (x+1,z+1) and
    // returns true iff ANY corner depth is strictly > 0.
    bool CellHasWater(Uint32 x, Uint32 z) const noexcept;

    // Exact cSTETerrain::CellHasLand @ 0x1007356a2. Out-of-range (x >=
    // cellCountX or z >= cellCountZ) returns false. In range, tests the
    // altitude at the four cell corners (x,z), (x+1,z), (x,z+1), (x+1,z+1)
    // via the altitude grid (+0x88) and returns true iff ANY corner altitude
    // is strictly > sea level (+0x9c). Counterpart to CellHasWater's depth
    // grid (+0x38) test; the DoIdle TractHasLand consumer depends on the pair's
    // exact corner semantics.
    bool CellHasLand(Uint32 x, Uint32 z) const noexcept;

    // Exact cSTETerrain::GetAltitude(float,float) @ 0x100738928. Triangular
    // interpolation over the vertex-altitude grid: world->index via the
    // -0.5/12582912.0f magic (round), unsigned cmova clamp against
    // +0x48/+0x4c, then constant v00->v11 diagonal barycentric interpolation
    // with a 16.0f cell width (subss/mulss/addss op order).
    Float32 GetAltitude(Float32 x, Float32 z) const noexcept;

    // Exact cSTETerrain::GetAltitude(int) @ 0x100738a84 (vertex index; 0 when
    // index >= vertexCount).
    Float32 GetAltitude(Sint32 vertexIndex) const noexcept;

    // Exact cSTETerrain::GetAltitudeAtVertex @ 0x100738aa6.
    Float32 GetAltitudeAtVertex(Sint32 x, Sint32 z) const noexcept;

    // Exact cSTETerrain::GetWaterDepthAtVertex(int) @ 0x100739b36.
    Float32 GetWaterDepthAtVertex(Sint32 vertexIndex) const noexcept;

    // Exact cSTETerrain::GetWaterLevel(float,float) @ 0x100739a68.
    // Single-vertex altitude + water-depth sum at the magic-round clamped
    // index (see terrain.cpp for the reconstruction notes).
    Float32 GetWaterLevel(Float32 x, Float32 z) const noexcept;

    // Exact cSTETerrain::GetSeaLevel @ 0x1007394ce.
    Float32 GetSeaLevel() const noexcept { return mSeaLevel; }
    // Exact cSTETerrain::IsInitialized @ 0x100740e24.
    bool IsInitialized() const noexcept { return mInitialized; }
    // Exact cSTETerrain::CellCountX @ 0x100740e3c / CellCountZ @ 0x100740e46.
    Sint32 CellCountX() const noexcept { return mCellCountX; }
    Sint32 CellCountZ() const noexcept { return mCellCountZ; }
    // Exact cSTETerrain::MinHeight @ 0x100740ea4 / MaxHeight @ 0x100740eb2.
    Float32 MinHeight() const noexcept { return mMinHeight; }
    Float32 MaxHeight() const noexcept { return mMaxHeight; }

    // ---- Brush dispatch pipeline (M3 terrain wave) ----

    // cSTETerrain::sRand @ 0x100b713d0 (cRZRandom instance used by the
    // ApplyBrushComponents rotation jitter @ 0x100731dbd).
    static RZRandomState sRand;

    // cSTETerrain::sBrushDescriptionMap @ 0x100b71e20:
    // std::map<unsigned int, tBrushDescription>, keyed by brush exemplar id.
    static std::map<Uint32, TerrainBrushDescription>& BrushDescriptionMap();

    // Exact cSTETerrain::ApplyBrush @ 0x100731c50: kEmptyRect-init of
    // params.rect, sBrushDescriptionMap lookup, delegate to
    // ApplyBrushComponents, then the vtable+0x228 re-display call gated on
    // params.reDisplay and rect bounds. Returns whether the brush was found.
    bool ApplyBrush(Uint32 brushID, TerrainBrushParameters& params);

    // Exact cSTETerrain::ApplyBrushComponents @ 0x100731d10: the core loop
    // over the brush description's components (0x78-byte entries).
    void ApplyBrushComponents(TerrainBrushDescription& desc, TerrainBrushParameters& params);

    // Exact cSTETerrain::ApplyBrushAtLocation (texture-aware) @ 0x1007321b8:
    // rotated-footprint sweep whose per-pixel texture byte feeds ctx.flag.
    void ApplyBrushAtLocation(const S3DTextureBufferInfo* texInfo, Sint32 gridX, Sint32 gridY,
                              Float32 size, Float32 angle, const TerrainBrushMemberFn& fn,
                              TerrainBrushContext& ctx, SC4RectI& rect);

    // Exact cSTETerrain::ApplyBrushAtLocation (plain) @ 0x1007327de. The uint
    // arg is a repeat count: the whole row/col sweep repeats `repeat` times
    // (@ 0x100732986..0x100732994), and a zero count skips the sweep entirely.
    void ApplyBrushAtLocation(Sint32 gridX, Sint32 gridY, Float32 size, Float32 angle,
                              const TerrainBrushMemberFn& fn, Uint32 repeat,
                              TerrainBrushContext& ctx, SC4RectI& rect);

    // Exact cSTETerrain::ComputeRectAroundPt @ 0x10073255c: the rotated brush
    // footprint corners (five SC4PointF outs; pts[0..3] are the four corners
    // of the proper rotated rectangle, ordered (+h,+v), (+h,-v), (-h,-v),
    // (-h,+v) relative to the center).
    void ComputeRectAroundPt(SC4PointF* pts, Float32 x, Float32 z, Float32 size,
                             Float32 aspect, Float32 angle);

    // Exact cSTETerrain::ComputeSpansForRectangle @ 0x100732656: per-row min
    // and max column spans of the footprint polygon; spanMax is stored as
    // (max crossing + 1), clamped to cellCountX, and the sweep treats it as
    // an inclusive bound.
    void ComputeSpansForRectangle(const SC4PointF* pts, Sint32* spanMin, Sint32* spanMax,
                                  Sint32& minRow, Sint32& maxRow) const;

    // Exact cSTETerrain::PadRectangle @ 0x100733596.
    void PadRectangle(SC4RectI& rect, Sint32 pad);

    // Exact cSTETerrain::BrushRadius @ 0x100734d6e:
    // desc->radii[terrain strength index (+0xdc)], default 32.0f @ 0x100a18764.
    Float32 BrushRadius(Uint32 brushID) const;

    // ---- Brush description property loading (M3 terrain wave 1) ----
    // Runtime fill of sBrushDescriptionMap from brush exemplars. The property
    // holder + resource-manager surfaces are clean-room fixture boundaries
    // (see BrushPropertyHolderFixture / BrushResourceSourceModel above).

    // Exact cSTETerrain::GetCostPerBrushApplication @ 0x100738d76 (const):
    // map lookup by brush id, default 32 @ 0x100738db5 when missing; else
    // cvttss2si desc.costs[mBrushStrengthIndex] @ 0x100738dbc..0x100738dc3
    // (value+0x40, truncate float -> int; the binary has no bounds check on
    // the strength index — the vector model guards).
    Sint32 GetCostPerBrushApplication(Uint32 brushID) const;

    // Exact cSTETerrain::GetEffectIDProperty @ 0x100738dcc (STATIC): reads the
    // vector<unsigned> property @ 0x100738dea and requires it to span EXACTLY
    // 8 bytes (end - begin == 8, cmp/jne @ 0x100738e06); on success copies
    // both dwords into out[2] @ 0x100738e08..0x100738e12 and returns true.
    // On ANY failure out is left untouched (callers pre-fill defaults).
    static bool GetEffectIDProperty(const BrushPropertyHolderFixture& holder,
                                    Uint32 propertyId, Uint32* out);

    // Exact cSTETerrain::ReadBrushWholeProperties @ 0x10073b26a (STATIC).
    // Always returns true (mov al, 1 @ 0x10073b38e).
    static bool ReadBrushWholeProperties(const BrushPropertyHolderFixture& holder,
                                         TerrainBrushDescription& desc);

    // Exact cSTETerrain::ReadBrushUIProperties @ 0x10073b3b8 (STATIC).
    // Always returns true (mov al, 1 @ 0x10073b48e).
    static bool ReadBrushUIProperties(const BrushPropertyHolderFixture& holder,
                                      TerrainBrushDescription& desc);

    // Exact cSTETerrain::ReadBrushComponentProperties @ 0x10073aafe (STATIC):
    // the 4-component loop. Returns the count>0 gate @ 0x10073ae63 — true iff
    // at least one component's effect property read succeeded.
    static bool ReadBrushComponentProperties(const BrushPropertyHolderFixture& holder,
                                             TerrainBrushDescription& desc);

    // Exact cSTETerrain::ReadBrushDescription @ 0x10073aefe (STATIC; the
    // resource-iterator apply-callback target, vt+0x38 @ 0x10073b515). The
    // userData argument is never read.
    static void ReadBrushDescription(const SC4PersistResourceKey& key, void* userData);

    // Exact cSTETerrain::ReadBrushDescriptions @ 0x10073b49c (STATIC): the
    // spResMan gate, ReleaseBrushDescriptions, the
    // cSCResFilterFixedGroupAndTypeId(0x6534284a, 0x6a01fc2a) query and the
    // apply-callback over matches.
    static void ReadBrushDescriptions();

    // Exact cSTETerrain::ReleaseBrushDescriptions @ 0x10073b548 (STATIC):
    // destroys every map node's component vectors then resets the map
    // @ 0x10073b5ce..0x10073b5f1.
    static void ReleaseBrushDescriptions();

    // cSTETerrain::spResMan @ 0x100b713c8 — not reconstructed. clean-room
    // fallback (not authentic): injectable fixture source modeling the
    // resource-manager surface the brush loader observes (null gate @
    // 0x10073b4a7 / 0x10073af5b, vt+0x18 exemplar load with class id
    // 0xa52160f5, vt+0x88 filtered query, iterator vt+0x38 apply-callback).
    static BrushResourceSourceModel*& BrushResourceSource();

    // ---- Terrain tuning parameter loading (M3 terrain data-loading wave 2) ----
    // Runtime fill of the tuning globals below from the terrain tuning
    // exemplar. The holder + SL::spRM surfaces are clean-room fixture
    // boundaries (BrushPropertyHolderFixture / TuningResourceSource).

    // Exact cSTETerrain::ReadTuningParameters @ 0x10073d95e (STATIC,
    // RNG-free): loads the terrain tuning exemplar — resource key
    // {group qword 0x88CD66E96534284A, instance dword 1} @
    // 0x10073d96b..0x10073d97c — via SL::spRM (vt+0x18 with class id
    // 0xa52160f5 @ 0x10073d99f; holder = loaded vt+0x30 @ 0x10073d9b1;
    // release vt+0x10 @ 0x10073ddf7), then overwrites the tuning globals in
    // the binary's fixed read order via cSCPropertyHelp::GetPropertyValue
    // (a missing/mistyped property leaves the global untouched), with the
    // four post-processing quirks (see terrain.cpp for the per-line
    // citations).
    static void ReadTuningParameters();

    // SL::spRM @ 0x100b21280 — not reconstructed. clean-room fallback (not
    // authentic): injectable fixture source modeling the exemplar-load
    // surface ReadTuningParameters observes (load with class id 0xa52160f5
    // @ 0x10073d99f, holder query @ 0x10073d9b1, release @ 0x10073ddf7).
    // The model resource key is the {group, dword} pair's dword (1).
    static BrushResourceSourceModel*& TuningResourceSource();

    // cSC4ResponseCurve::SetResponseCurveData @ 0x10073ddc3 — NOT
    // reconstructed (the callee for property 0x48cd7c00 after the non-null
    // gate @ 0x10073ddb7; target object
    // kEdgeReconcilliationSmoothingRadiusVsHeightDiff @ 0x100b71e30).
    // clean-room fallback (not authentic): TODO no-op boundary; this
    // observation counter pins the null gate.
    static Uint32& EdgeReconcilliationCurveLoadCount();

    // cSTETerrain::DetermineTextureFamilyMembersForGridCells @ 0x100737430
    // — called by ReadTuningParameters @ 0x10073d9cb right after the
    // texture-randomization grid-size read; NOT reconstructed (TODO).
    // clean-room fallback (not authentic): observation counter only; the
    // settable TextureFamilyMemberForGrid() table is left as-is.
    static Uint32& DetermineTextureFamilyMembersCallCount();

    // Exact cSTETerrain::BeginBrushScrub @ 0x100734a7e: clears the decal map,
    // then (brush found) runs the spSSE scrub sound session and tails into the
    // vtable+0x2b8 delegate.
    void BeginBrushScrub(Uint32 brushID, Uint32 scrubID);

    // Exact cSTETerrain::DoPostBrushApplicationOperation @ 0x10073340e.
    // Branch targets from the disassembly: effect 7 -> AddVolcanicAshRegion,
    // effect 2 -> PadRectangle(rect, 1); everything else is a no-op. The ctx
    // argument is unused (the caller does not even set its register
    // @ 0x100732148).
    void DoPostBrushApplicationOperation(Sint32 effect, TerrainBrushContext& ctx, SC4RectI& rect);

    // Exact cSTETerrain::GetTextureValue @ 0x100733436: rotated texture lookup
    // for a grid point; stores the texture byte into ctx.flag and returns
    // false when the texel is out of bounds.
    bool GetTextureValue(Sint32 x, Sint32 y, TerrainBrushContext& ctx);

    // Exact cSTETerrain::AddVolcanicAshRegion @ 0x100731628: pushes a region
    // onto the ash list (this+0x230, size at this+0x240), merging it with any
    // overlapping existing regions first.
    void AddVolcanicAshRegion(SC4RectI& rect);

    // ---- BrushFn* per-vertex effects (dispatched through the pipeline via
    //      TerrainBrushMemberFn, or directly for testing) ----

    // Exact cSTETerrain::BrushFnRaiseLower @ 0x1007329cc. Raise/lower terrain
    // brush: compares ctx.flag (+0x14) against component flag48 (+0x48); when
    // unequal, applies (ctx.flag * amount) via RaiseVertex (ctx.flag greater)
    // or (flag48 * amount) via LowerVertex.
    bool BrushFnRaiseLower(Sint32 vertex, const TerrainBrushContext& ctx) noexcept;

    // Exact cSTETerrain::BrushFnSmoothen @ 0x100732bd2. Smoothen brush:
    // iterates the 8 neighbors of the target vertex (div/mod by vertexCountX
    // +0x50), computes height differences against neighbor altitudes, compares
    // against kCrossHtDiffThreshold and kDiagonalHtDiffThreshold, and calls
    // RaiseVertex/LowerVertex with scaled amounts.
    bool BrushFnSmoothen(Sint32 vertex, const TerrainBrushContext& ctx) noexcept;

    // Exact cSTETerrain::BrushFnRain @ 0x10073083e. Adds amount/10.0f to the
    // water depth at the surface-water current layer when ctx.flag differs
    // from the component's flag48. ALWAYS returns false — the shared exit
    // @ 0x10073087e is `xor eax, eax`.
    bool BrushFnRain(Sint32 vertex, const TerrainBrushContext& ctx) noexcept;

    // Exact cSTETerrain::BrushFnLevel @ 0x100733230. Rate-limited move toward
    // ctx.targetHeight: rate = flag * amount * 0.00390625f; when |diff| <=
    // rate the vertex snaps to the target via vtable+0x188, otherwise it moves
    // by rate (LowerVertex above the target, RaiseVertex below). diff == 0
    // returns false.
    bool BrushFnLevel(Sint32 vertex, const TerrainBrushContext& ctx) noexcept;

    // Exact cSTETerrain::BrushFnLakeBed @ 0x100733360. Moves the vertex toward
    // (targetHeight + delta) where delta = (flag - flag48) * raw component
    // strength, rate = |delta|/16; raising a submerged vertex (depth > 0) is
    // refused; snap via vtable+0x188 when within rate.
    bool BrushFnLakeBed(Sint32 vertex, const TerrainBrushContext& ctx) noexcept;

    // Exact cSTETerrain::BrushFnVolcanoAsh @ 0x100733338: flag gate, then
    // `orw 0x400` into the vertex-record flags word (+0x22).
    bool BrushFnVolcanoAsh(Sint32 vertex, const TerrainBrushContext& ctx) noexcept;

    // Exact cSTETerrain::BrushFnFlora @ 0x100732a0e. Flora planting attempt:
    // flag >= 10 gate, a 31/100 placement roll, submerged-vertex bail, cell
    // bounds, a jittered point in the 16m cell, a flora-index roll from the
    // +0x150 count, GetAltitude for the point's y, then the flora-manager
    // create / city gate / AuraSim notify. ALWAYS returns false.
    bool BrushFnFlora(Sint32 vertex, const TerrainBrushContext& ctx) noexcept;

    // Exact cSTETerrain::BrushFnPaint @ 0x100732d22. Decal paint: gates on
    // the +0x130 target, ctx.target, ctx.flag and the cell bounds; a
    // decal-map entry keyed by vertex with its count gated against the
    // component's maxDecalCount (+0x4c) before incrementing; jittered
    // placement point clamped to the world extents; place-decal call on the
    // +0x130 target. ALWAYS returns false.
    bool BrushFnPaint(Sint32 vertex, const TerrainBrushContext& ctx) noexcept;

    // Exact cSTETerrain::BrushFnPlaceEffect @ 0x100732f92. Effect/decal
    // placement: ctx.flag must differ from the component's flag48; decal-map
    // count gate; the +0x138 target's eligibility query must succeed before
    // the count increments; jittered position with y = max(seaLevel,
    // GetAltitude(rndX, rndZ)); out-object transform calls. ALWAYS returns
    // false.
    bool BrushFnPlaceEffect(Sint32 vertex, const TerrainBrushContext& ctx) noexcept;

    // Exact cSTETerrain::GetNeighborWithLowestWaterLevel @ 0x100737b50.
    // Returns the direction index (0=W, 1=NW, 2=N, 3=NE, 4=E, 5=SE, 6=S,
    // 7=SW, per the +0xa4 offset table and the edge masks
    // @ 0x100737b64..0x100737b88) of the 8-neighbor whose water level
    // (altitude + depth) is lowest, with cross-neighbor differences
    // compared unscaled, the best scaled by sqrt(2), then the diagonals —
    // or -1 when no neighbor is strictly lower.
    Sint32 GetNeighborWithLowestWaterLevel(Sint32 vertex) noexcept;

    // Exact cSTETerrain::DoWaterErosion @ 0x1007335e6 (three phases: initial
    // depth/sediment reset over the rect; per-flow-step rainfall + flow/
    // erode/deposit pass; the padded-rect water-to-sea-level reset tail).
    void DoWaterErosion(const SC4RectI& rect, Sint32 numFlowSteps);

    // Exact cSTETerrain::InitFloraBrushInfo @ 0x1007385e2: one-shot (gated on
    // +0x150 != 0) initialization of the flora brush id list (+0x158) from
    // the +0x148 provider, filtered in place to the ids whose type attribute
    // byte (+0x180 & 3) == 2, decrementing the live count on each removal.
    // (Private in the model would block the verify suite; the binary calls
    // it from ApplyBrushComponents @ 0x100731fc1.)
    void InitFloraBrushInfo();

    // Per-vertex effect dispatch — the ApplyBrushComponents jump table
    // @ 0x10073218c. Kept as the direct per-vertex entry for the verify suite;
    // the pipeline itself resolves the same mapping into TerrainBrushMemberFn
    // (MemberFnForEffect) and calls it through the sweep.
    bool ApplyBrushEffect(TerrainBrushEffect effect, Sint32 vertex,
                          const TerrainBrushContext& ctx) noexcept;

    // vtable+0x228 re-display entry — cSTETerrain::RedisplayTerrain(bool,
    // bool, SC4Rect<int> const&, unsigned int) @ 0x10073defa (slot verified:
    // __ZTV11cSTETerrain 0x100aea6b0 + 0x10 + 0x228 holds 0x10073defa; the
    // 448-byte body is not reconstructed). Called by ApplyBrush @ 0x100731d07
    // with (1, 1, &rect, params+0x20). clean-room fallback (not authentic):
    // the call is recorded so the gate is observable.
    virtual void ReDisplayTerrain(Sint32 a, Sint32 b, const SC4RectI& rect, Sint32 c);
    Sint32 ReDisplayCallCount() const noexcept { return mReDisplayCount; }

    // vtable+0x2b8 scrub tail (BeginBrushScrub @ 0x100734b58; callee not
    // resolved). clean-room fallback (not authentic): recorded no-op.
    virtual void BeginBrushScrubDelegate(Uint32 brushID, Uint32 scrubID);
    Sint32 BeginScrubDelegateCount() const noexcept { return mBeginScrubDelegateCount; }

    // ---- Direct grid access (for the verify suite and loader wiring) ----
    void    SetAltitude(Sint32 vertexIndex, Float32 altitude);
    void    SetWaterDepth(Sint32 vertexIndex, Float32 depth);
    // Vertex-record flags word (+0x22) accessor for the verify suite.
    uint16_t VertexFlags(Sint32 vertexIndex) const noexcept {
        if (vertexIndex < 0 || vertexIndex >= static_cast<Sint32>(mVertexFlags.size())) return 0;
        return mVertexFlags[static_cast<std::size_t>(vertexIndex)];
    }
    // Raw field setter for the verify suite. The binary's sea level is set
    // through the vtable+0x168 SetSeaLevel(float) @ 0x10073e556 (below),
    // whose range gate (kMinSeaLevel/kMaxSeaLevel) and side effects
    // (water reset + redisplay) are inappropriate for fixture setup.
    void    SetSeaLevelForTest(Float32 seaLevel) { mSeaLevel = seaLevel; }
    void    SetMinMaxHeight(Float32 minH, Float32 maxH) {
        mMinHeight = minH; mMaxHeight = maxH;
    }
    void    SetMaxVertexIndexBounds(Sint32 maxX, Sint32 maxZ);
    Float32 AltitudeAt(Sint32 vertexIndex) const;
    TerrainSurfaceWaterState&       SurfaceWater()       { return mSurfaceWater; }
    const TerrainSurfaceWaterState& SurfaceWater() const { return mSurfaceWater; }

    // Modeled observation hooks for the verify suite (not in the binary).
    std::size_t VolcanicAshRegionCount() const noexcept { return mVolcanicAshRegions.size(); }
    const SC4RectI& VolcanicAshFront() const noexcept { return mVolcanicAshRegions.front().rect; }
    std::size_t DecalMapSize() const noexcept { return mDecalMap.size(); }
    void SeedDecalMapForTest(Uint32 n) {
        for (Uint32 i = 0; i < n; ++i) mDecalMap[i] = 0;
    }
    // tDecalMapEntry count (node+0x20) for one vertex, 0 when absent
    // (BrushFnPaint @ 0x100732e52 / BrushFnPlaceEffect @ 0x1007330a1).
    Sint32 DecalMapEntryCountAt(Uint32 vertex) const noexcept {
        const auto it = mDecalMap.find(vertex);
        return it == mDecalMap.end() ? 0 : it->second;
    }
    // Vertex-record flags word (+0x22) setter for the verify suite (the
    // erosion/deposit suppression bit 0x4000, DoWaterErosion
    // @ 0x100733975/0x100733a1e).
    void SetVertexFlagsForTest(Sint32 vertexIndex, uint16_t flags) noexcept {
        if (vertexIndex >= 0 && static_cast<std::size_t>(vertexIndex) < mVertexFlags.size()) {
            mVertexFlags[static_cast<std::size_t>(vertexIndex)] = flags;
        }
    }
    // ---- Shore-cliff cluster test accessors (modeled, not in the binary) ----
    // 36-byte vertex-record normal floats (+0x4/+0x8/+0xc), written by the
    // phase-2 pass of CalculateNormalsAndAssignCliffTextures
    // @ 0x100735241..0x100735259.
    const Float32* VertexNormalAt(Sint32 vertex) const noexcept {
        if (vertex < 0 || static_cast<std::size_t>(vertex) >= mVertexNormals.size()) {
            return nullptr;
        }
        return mVertexNormals[static_cast<std::size_t>(vertex)].data();
    }
    // Quad-normal ping-pong buffers (+0x170/+0x188): 6 floats per column
    // (two {x, y, z} quad normals), sized 6*vertexCountX by Init (the
    // binary's allocation site is not reconstructed — modeled).
    const std::vector<Float32>& QuadNormalsA() const noexcept { return mQuadNormalsA; }
    const std::vector<Float32>& QuadNormalsB() const noexcept { return mQuadNormalsB; }
    // Cached whole-world rect at this+0x58 (fill site not reconstructed;
    // Init models it as {0, 0, cellCountX, cellCountZ}).
    void SetWorldRectForTest(const SC4RectI& r) noexcept { mWorldRect = r; }
    void SetBrushStrengthIndexForTest(Sint32 idx) noexcept { mBrushStrengthIndex = idx; }
    const SC4RectI& WorldRect() const noexcept { return mWorldRect; }
    // +0x110 view height-range model (clean-room boundary).
    void SetView3DHeightRangeForTest(TerrainView3DHeightRange* r) noexcept {
        mView3D = r;
    }
    // cSTETerrain::PostMessageA @ 0x100734b92 (not reconstructed) —
    // recorded no-op for the message posts of this cluster.
    Sint32 PostMessageACalls() const noexcept { return mPostMessageACalls; }
    Uint32 PostMessageALastId() const noexcept { return mPostMessageALastId; }

    // ---- Modeled boundary accessors (clean-room fallback, not authentic;
    //      see the stand-in structs and the BrushFn* citations above) ----
    void SetDecalPlacementTarget(TerrainDecalPlacementTarget* t) noexcept {
        mDecalTarget130 = t;
    }
    TerrainEffectPlacementTarget& EffectPlacementTarget() noexcept { return mEffectTarget138; }
    std::vector<TerrainFloraBrushType>& FloraBrushInfoSource() noexcept {
        return mFloraBrushInfoSource;
    }
    const std::vector<Uint32>& FloraBrushIds() const noexcept { return mFloraBrushIds; }
    Sint32 FloraBrushInfoCount() const noexcept { return mFloraBrushInfo; }
    const std::vector<TerrainFloraPlacement>& FloraPlacements() const noexcept {
        return mFloraPlacements;
    }

    // cSTETerrain::kNumFlowStepsPerWaterErosionBrushApplication @
    // 0x100b71e00 — registered as an EMPTY vector by the static initializer
    // (@ 0x100743bd1: movq %rbx empty-rep stores + __cxa_atexit) and filled
    // at runtime from loaded tuning data; the fill site is not reconstructed
    // (TODO). Modeled as a settable per-strength table; reads outside the
    // table return 0 (the binary has no bounds check @ 0x10073331c).
    static std::vector<Sint32>& FlowStepsPerErosionApplication() {
        static std::vector<Sint32> sTable;
        return sTable;
    }

    // ---- Surface-cover cluster (M3 terrain wave) ----
    //
    // Every table below is a runtime-filled static in the binary (filled from
    // loaded exemplar/tuning data; the fill sites are not reconstructed,
    // TODO) — modeled as settable containers, clean-room fallback (not
    // authentic). Reads outside a table return 0 / "not found"; the binary
    // has no bounds checks (e.g. 0x10073403c, 0x100734293).

    // cSTETerrain::sLowCliffTexResIds @ 0x100b71ccc / sBeachTexResIds @
    // 0x100b71cd0 — int arrays by terrain type (DecideSurfaceCover
    // @ 0x100733e63 / @ 0x1007340f5).
    static std::vector<Uint32>& LowCliffTexResIds();
    static std::vector<Uint32>& BeachTexResIds();
    // cSTETerrain::snMaxSurfaceTextureSpread @ 0x100b71d10 — int array by
    // strength index (DecideSurfaceCover @ 0x100733eeb,
    // AssignSurfaceTexturesAndColors @ 0x100733bda).
    static std::vector<Sint32>& MaxSurfaceTextureSpread();
    // cSTETerrain::sTextureRandomizationGridSize @ 0x100b713f0 /
    // sTextureGridCellCountX @ 0x100b71490 — int arrays by strength
    // (DecideSurfaceCover @ 0x100734032/0x100734050).
    static std::vector<Sint32>& TextureRandomizationGridSize();
    static std::vector<Sint32>& TextureGridCellCountX();
    // cSTETerrain::sTextureFamilyMemberForGrid @ 0x100b71410 — uint16 arrays
    // by strength (DecideSurfaceCover @ 0x10073405d..0x10073406c).
    static std::vector<std::vector<uint16_t>>& TextureFamilyMemberForGrid();
    // cSTETerrainView3D::sTerrainTextureBinding @ 0x100b720e0 — per-strength
    // tables indexed by texture id whose entries expose a count at +0x8
    // (div @ 0x10073407d). Modeled as [strength][texId] -> variant count;
    // a missing entry reads 1 (model guard — the binary would dereference
    // whatever the table held).
    static std::vector<std::map<Uint32, Sint32>>& TerrainTextureBindingCounts();
    // cSTETerrain::sSurfaceCoverMap @ 0x100b714b0 — int array with 2048-byte
    // (512-int) entries per terrain type. DecideSurfaceCover reads the flat
    // index terrainType*512 + tempCode*16 + moisture (@ 0x100733f3f..0x100733f51
    // base terrainType<<11 bytes, movsxd read @ 0x100734293);
    // AddTemperatureAndMoistureToSurfaceCoverEntry writes sub-entry ints via
    // (hash & 0xfffffff) << 6 byte offsets (@ 0x100731496..0x1007314b2).
    static std::vector<Sint32>& SurfaceCoverMap();
    // cSTETerrain::sSurfaceCoverTextures @ 0x100b71cd8 —
    // unordered_map<Uint32, tSurfaceCoverInfo> (find @ 0x100734311 /
    // @ 0x10073474a).
    static std::unordered_map<Uint32, TerrainSurfaceCoverInfo>&
    SurfaceCoverTextures();
    // cSTETerrain::sTexturesUsedInTerrainType @ 0x100b71cb0 — 24-byte entries
    // per terrain type (AddToTerrainTypeTextureList @ 0x10073159e).
    static std::map<Sint32, TerrainTypeTextureList>& TexturesUsedInTerrainType();

    // RZConvertToUintPtrA @ 0x10001a307 — not reconstructed. clean-room
    // fallback (not authentic): a settable char* -> Uint32 hash used by the
    // surface-cover exemplar loading path (AddTemperatureAndMoisture... @
    // 0x100731486/0x100731507). When unset, a clean-room FNV-1a 32-bit hash
    // is used so the entry math stays observable.
    static Uint32 (*&SurfaceCoverStringHashHook())(const char*);

    // cSTETerrain::kTemperatureRangeFactor @ 0x100b71db0 — read as a FLOAT by
    // DecideSurfaceCover (@ 0x100733eb9..0x100733ec1) but the shipped __DATA
    // bytes are the bit pattern 0x0000000a (int 10 -> denormal 1.4e-44 as a
    // float; neighbor kNumInitialErosionPasses @ 0x100b71db8 = int 1). The
    // loader presumably overwrites it at runtime (TODO — fill site not
    // reconstructed). Modeled as a settable float whose default reproduces
    // the shipped bit pattern exactly.
    static Float32& TemperatureRangeFactor();

    // cSTETerrain::kMaxTerrainHeight @ 0x100b1763c / kMinTerrainHeight @
    // 0x100b17640 — the RaiseVertex/LowerVertex clamp limits (movss loads
    // @ 0x100734d13 / 0x100734cb5) and the DecideSurfaceCover temp-range
    // source (@ 0x100733eac). The SHIPPED __DATA bytes are 200.0f / 5.0f
    // (fresh byte dumps; nm-confirmed symbols) — the previous ±2048 model
    // constants were a transcription error, fixed in this ticket. Both are
    // RUNTIME-OVERRIDDEN from the terrain tuning exemplar:
    // cSCPropertyHelp::GetPropertyValue(holder, 0x48cd7b26, kMaxTerrainHeight&)
    // @ 0x10073d9e4 and (holder, 0x48cd7b27, kMinTerrainHeight&) @
    // 0x10073da07 (the same tuning family as kSeaLevel property 0x48cd7b21);
    // the loader additionally overwrites kSeaLevel with kMinTerrainHeight
    // when the min exceeds it @ 0x10073da31. The exemplar override wiring is
    // future work (TODO); modeled as settable tunables with the shipped
    // byte-exact defaults.
    static Float32& MaxTerrainHeightTunable();
    static Float32& MinTerrainHeightTunable();

    // cSTETerrainView3D static/instance calls recorded for observation:
    // ChangeTerrainType(terrainType) @ 0x1007467ca and
    // SetUpTexturesForCurrentTerrainTypeAndZoom(strengthIndex) @ 0x10074688c
    // (both called directly with int args @ 0x100733eca/0x100733ecf..0x100733ed5),
    // plus MarkTextureSpanInfoDirty(lo, hi) on the +0x110 view
    // @ 0x10074d7d4 (gated on the +0x118 byte @ 0x100733c0c).
    // clean-room fallback (not authentic).
    static TerrainView3DObservation& View3DObservation();

    // Exact cSTETerrain::AddLandTypeAtVertex @ 0x100731338: quantizes the
    // float ratio to int(ratio * 255.0f + 1.0f) (@ 0x10073333c..0x10073334c,
    // cvttss2si) and records {type, value} in the vertex's 3-slot table —
    // matching type keeps the max value (@ 0x1007313d6..0x1007313e5), the
    // first empty slot is filled (@ 0x1007313c5..0x1007313d4), a full table
    // replaces its min-value slot when the new value is larger
    // (@ 0x100731386..0x1007313c3).
    void AddLandTypeAtVertex(Uint32 vertex, uint16_t type, Float32 ratio) noexcept;

    // Exact cSTETerrain::AdjustLandTypeRatios @ 0x10073173a: over the rect,
    // sums the values of the filled slot prefix (the scan exits at the first
    // empty slot, je @ 0x10073178b) and, when the sum >= 251 (0xfb @
    // 0x10073179e), rescales each filled slot's value to value*250/sum
    // (imul 0xfa / div @ 0x1007317bc..0x1007317c6, also stopping at the
    // first empty slot @ 0x1007317b4).
    void AdjustLandTypeRatios(const SC4RectI& rect) noexcept;

    // Exact cSTETerrain::AreSeaVertices @ 0x100733b16: unsigned bounds gates
    // (vertexCountX/Z <= left/top/right/bottom -> false, jbe @ 0x100733b26/
    // 0x100733b32/0x100733b3b/0x100733b44), then true iff every vertex in the
    // rect has altitude strictly below sea level (jbe @ 0x100733b80).
    // BINARY ODDITY: an empty rect (top > bottom, or left > right with rows)
    // returns TRUE (al set to 1 before the emptiness checks @ 0x100733b46).
    bool AreSeaVertices(const SC4RectI& rect) const noexcept;

    // Exact cSTETerrain::AssignSurfaceTexturesAndColors @ 0x100733bc8:
    // ComputeWaterProximityValues -> PadRectangle(rect,
    // snMaxSurfaceTextureSpread[strength]) -> UpdateMoistureValues ->
    // ClearSurfaceTextures -> (+0x118-gated) MarkTextureSpanInfoDirty ->
    // DecideSurfaceCover -> SprayVolcanicAsh -> tail-jmp AdjustLandTypeRatios.
    // The rect is passed BY REFERENCE and is padded in place.
    void AssignSurfaceTexturesAndColors(SC4RectI& rect);

    // Exact cSTETerrain::ComputeWaterProximityValues @ 0x100733c62: unions
    // the rect into the +0xcc dirty rect (UnionRect @ 0x100733c7f) and
    // recomputes the +0x200 proximity map only when the union spans >= 100
    // cells (imul/cmp 0x64 @ 0x100733ca4..0x100733caa); water vertices get
    // proximity 0, land vertices kMaxBeachWidth+2 (@ 0x100733ccd..0x100733d3a),
    // the water-vertex count is stored at +0x108 and the map is flood-filled
    // from the water when any exists (FloodFillDistance @ 0x100733d8e —
    // clean-room BFS model, see terrain.cpp).
    void ComputeWaterProximityValues(const SC4RectI& rect);

    // Exact cSTETerrain::UpdateMoistureValues @ 0x100733d98: null gate on
    // the +0x140 object (@ 0x100733d9f), then vtable+0x80 with the rect
    // converted to world ints (cvtdq2ps, mulps {16,16,16,16} @ 0x100a29bd0,
    // cvttps2dq @ 0x100733d98..0x100733dc7).
    void UpdateMoistureValues(const SC4RectI& rect) noexcept;

    // Exact cSTETerrain::ClearSurfaceTextures @ 0x100733dd4: over the rect,
    // zeroes the three land-type slot type words of each vertex record
    // (record +0x16/+0x1a/+0x1e, three word stores @ 0x100733e0a..0x100733e21).
    void ClearSurfaceTextures(const SC4RectI& rect) noexcept;

    // Exact cSTETerrain::DecideSurfaceCover @ 0x100733e48 — the core decision
    // pass. See terrain.cpp for the per-block reconstruction notes.
    void DecideSurfaceCover(const SC4RectI& rect) noexcept;

    // Exact cSTETerrain::SprayVolcanicAsh @ 0x100734724 — applies the
    // 0xe0 volcanic-ash land type around ash-flagged (word & 0x400) vertices
    // within the queued ash regions. See terrain.cpp.
    void SprayVolcanicAsh(const SC4RectI& rect) noexcept;

    // Exact cSTETerrain::AddToTerrainTypeTextureList @ 0x10073159e (STATIC):
    // sorted-unique ascending insert of textureId into
    // sTexturesUsedInTerrainType[typeIndex]; the +0x10 count increments only
    // on insert (@ 0x100731614).
    static void AddToTerrainTypeTextureList(Sint32 typeIndex, Uint32 textureId);

    // Exact cSTETerrain::AddTemperatureAndMoistureToSurfaceCoverEntry @
    // 0x100731418 (STATIC; rdi=key string, rsi=value string, rdx=int entry
    // index — the asm uses rdx both as the map entry index (& 0x7fffff) and
    // as the AddToTerrainTypeTextureList int arg @ 0x100731511, so the third
    // parameter is an int, not a cRZString). Splits the value string on ','
    // (SplitToken loop, <= 15 tokens @ 0x1007314c4..0x100731522) and for each
    // token: id = RZConvertToUintPtrA(token); entry->ids[i] = id
    // (@ 0x10073150c); AddToTerrainTypeTextureList(entryIndex, id)
    // (@ 0x100731516). The cRZString token plumbing is modeled with const
    // char* args; RZConvertToUintPtrA is a clean-room boundary (see
    // SurfaceCoverStringHashHook).
    static void AddTemperatureAndMoistureToSurfaceCoverEntry(const char* key,
                                                              const char* value,
                                                              Sint32 entryIndex);

    // ---- Shore-cliff & normal-assignment cluster (M3 terrain wave) ----

    // Runtime-filled tuning constants of the cluster. All sit in the
    // binary's tuning-data block (0x100b175d4..0x100b17680 and 0x100b71d28)
    // alongside kMaxTerrainHeight, which the loader overwrites from the
    // terrain tuning exemplar (@ 0x10073d9e4); the fill sites for these are
    // not reconstructed (TODO). Modeled as settable statics with the shipped
    // byte-exact defaults, like MaxTerrainHeightTunable.
    static Float32& MinSeaLevelTunable();                  // 0x100b175d4 = -10.1f
    static Float32& MaxSeaLevelTunable();                  // 0x100b175d8 = 200.1f
    static Float32& SeaRaiseLowerAmountTunable();          // 0x100b175dc = 20.0f
    static Sint32& MinShoreCliffSizeTunable();             // 0x100b1766c = 4
    static Sint32& MaxShoreCliffSizeTunable();             // 0x100b17670 = 70
    static Float32& MinShoreCliffCandidateHtSumTunable(); // 0x100b17674 = 15.0f
    static Float32& MinShoreCliffHtTunable();              // 0x100b17678 = 40.1f
    static Float32& MaxShoreCliffHtTunable();              // 0x100b1767c = 50.1f
    static Float32& MaxNormalYForCliffTunable();           // 0x100b17680 = 0.3f
    // cSTETerrain::kWindErosionFactor @ 0x100b71d28 — read as a FLOAT by
    // SmoothenMarkedRegion (mulss @ 0x100736a99/0x100736af5) but the shipped
    // __DATA bytes are the bit pattern 0x00000001 (denormal 1.4e-44f), like
    // kTemperatureRangeFactor @ 0x100b71db0 (bit pattern 0x0000000a). The
    // loader presumably overwrites it at runtime (TODO). Modeled as a
    // settable float whose default reproduces the shipped bit pattern.
    static Float32& WindErosionFactorTunable();

    // ---- Tuning globals wired by ReadTuningParameters @ 0x10073d95e
    //      (M3 terrain data-loading wave 2) ----
    // Each accessor is a binary tuning global with its shipped __DATA
    // default (fresh byte dumps; nm-confirmed addresses); the loader
    // overwrites them from the terrain tuning exemplar, leaving a global
    // untouched when its property is missing (GetPropertyValue failure
    // contract). Scalar/uint/byte defaults are bit-exact; the vector
    // globals ship EMPTY (static-initializer empty-rep stores).

    // cSTETerrain::kSeaLevel @ 0x100b71d48 — the STATIC tuning global the
    // loader overwrites (and quirks against kMinTerrainHeight @
    // 0x10073da31); distinct from the per-terrain instance sea level.
    // Shipped bytes are the bit pattern 0x00000001 (denormal 1.4e-44f) —
    // an uninitialized slot like kTemperatureRangeFactor.
    static Float32& SeaLevelTunable();
    // kDefaultTerrainHeight @ 0x100b17644 = 50.0f (0x42480000); quirked
    // against kMinTerrainHeight @ 0x10073da5e.
    static Float32& DefaultTerrainHeightTunable();
    // kMaxNormalTerrainHeight @ 0x100b71d30 (vector<float>, ships empty).
    static std::vector<Float32>& MaxNormalTerrainHeightTunable();
    // kMinRegularHtFactor @ 0x100b71d50 / kMaxRegularHtFactor @ 0x100b71d68
    // / kMinMountainHtFactor @ 0x100b71d80 / kMaxMountainHtFactor @
    // 0x100b71d98 (vector<float>, ships empty).
    static std::vector<Float32>& MinRegularHtFactorTunable();
    static std::vector<Float32>& MaxRegularHtFactorTunable();
    static std::vector<Float32>& MinMountainHtFactorTunable();
    static std::vector<Float32>& MaxMountainHtFactorTunable();
    // kImageImportScaleFactor @ 0x100b17648 = 1.5f (0x3fc00000).
    static Float32& ImageImportScaleFactorTunable();
    // kWaterProximityRangeFactor @ 0x100b71db4 = 0.0f.
    static Float32& WaterProximityRangeFactorTunable();
    // kSeaLevelChangeDueToWaterSlider @ 0x100b71dc8 (vector<float>, ships
    // empty).
    static std::vector<Float32>& SeaLevelChangeDueToWaterSliderTunable();
    // kNumInitialErosionPasses @ 0x100b71db8 = 1 /
    // kNumHydraulicIterationsPerInitialErosionPass @ 0x100b71dbc = 334 /
    // kNumWindIterationsPerInitialErosionPass @ 0x100b71dc0 = 10.
    static Uint32& NumInitialErosionPassesTunable();
    static Uint32& NumHydraulicIterationsPerInitialErosionPassTunable();
    static Uint32& NumWindIterationsPerInitialErosionPassTunable();
    // kMinCellsInUsefulFlatArea @ 0x100b17650 = 401 /
    // kAutoFlatAreaEdgeSmootheningRadius @ 0x100b17664 = 4 /
    // kNumSmoothingIterationsAfterWaterErosion @ 0x100b1764c = 1 /
    // kEdgeReconcilliationSmoothingIterations @ 0x100b17694 = 5.
    static Uint32& MinCellsInUsefulFlatAreaTunable();
    static Uint32& AutoFlatAreaEdgeSmootheningRadiusTunable();
    static Uint32& NumSmoothingIterationsAfterWaterErosionTunable();
    static Uint32& EdgeReconcilliationSmoothingIterationsTunable();
    // kMaxHtChangeToFlattenFactor @ 0x100b17654 = 0.1f (0x3dcccccd) /
    // kMinTerraceAltitudeFactor @ 0x100b17658 = 0.1f (0x3dcccccd) /
    // kMaxTerraceAltitudeFactor @ 0x100b1765c = 0.6f (0x3f19999a).
    static Float32& MaxHtChangeToFlattenFactorTunable();
    static Float32& MinTerraceAltitudeFactorTunable();
    static Float32& MaxTerraceAltitudeFactorTunable();
    // kTerrainRaiseLowerAmount @ 0x100b175e0 = 20.0f.
    static Float32& TerrainRaiseLowerAmountTunable();
    // kMaxBeachWidth @ 0x100b175e4 (byte) = 4 — consumed by
    // ComputeWaterProximityValues (proximity init value kMaxBeachWidth+2 @
    // 0x100733cd4, FloodFillDistance arg @ 0x100733d7e, beach gate byte
    // compare @ 0x1007340d6).
    static Uint8& MaxBeachWidthTunable();
    // kMaxBeachAltitude @ 0x100b175e8 = 7.0f (0x40e00000) — consumed by
    // DecideSurfaceCover (beachAlt = seaLevel + kMaxBeachAltitude @
    // 0x100733e73).
    static Float32& MaxBeachAltitudeTunable();
    // kMinWaterDepthForDeepSeaBedTexture @ 0x100b17634 = 15.0f /
    // kMaxWaterDepthForDeepSeaBedTexture @ 0x100b17638 = 30.0f — consumed
    // by DecideSurfaceCover (depth gate @ 0x100733fe9 / texture pick @
    // 0x10073409d).
    static Float32& MinWaterDepthForDeepSeaBedTextureTunable();
    static Float32& MaxWaterDepthForDeepSeaBedTextureTunable();
    // kbAutoFlattenUponCreation @ 0x100b71de0 (byte, ships 0x8a) /
    // kbShoreCliffsUponCreation @ 0x100b17668 (byte, ships 1) — the loader
    // ALWAYS rewrites both through setne stores (@ 0x10073dcef /
    // 0x10073dd09); consumers are not reconstructed (TODO).
    static Uint8& AutoFlattenUponCreationTunable();
    static Uint8& ShoreCliffsUponCreationTunable();
    // kTypicalBrushAngleIncrement @ 0x100b71de8 (vector<float>, ships
    // empty) — the loader post-scales the first FIVE entries by f64 pi/180
    // @ 0x10073dd24..0x10073dd4f.
    static std::vector<Float32>& TypicalBrushAngleIncrementTunable();
    // kVolcanicAshLifeTime @ 0x100b17684 = 10000000 (uint) — consumed by
    // SprayVolcanicAsh (spray factor @ 0x100734864..0x10073489a).
    static Uint32& VolcanicAshLifeTimeTunable();
    // kVolcanicAshUpdateInterval @ 0x100b17688 = 10000000 pre-derivation —
    // the loader recomputes it as kVolcanicAshLifeTime / 20 @
    // 0x10073dd65..0x10073dd78; consumers are not reconstructed (TODO).
    static Uint32& VolcanicAshUpdateIntervalTunable();
    // kEdgeReconcilliationHtDifferenceThreshold @ 0x100b17698 = 1.2f
    // (0x3f99999a); consumers are not reconstructed (TODO).
    static Float32& EdgeReconcilliationHtDifferenceThresholdTunable();

    // SL::spViewUtilities @ 0x100b214d8 + SC4ViewUtilities::GetVisibleEdges
    // @ 0x10017376a — not reconstructed. clean-room fallback (not
    // authentic): settable presence flag + visible-edge pair consumed by
    // ComputeMinAndMaxAltitudeForTerrainRedisplay.
    static TerrainViewUtilitiesModel& ViewUtilitiesModel();

    // Exact cSTETerrain::FindMinAndMaxHeights(SC4Rect<int> const&, float&,
    // float&) @ 0x100735c54: FLT_MAX/-FLT_MAX seeds, rows [top, bottom] and
    // per-row scan [left, right] INCLUSIVE (do-while column shape — column
    // `left` is processed even when left == right).
    void FindMinAndMaxHeights(const SC4RectI& rect, Float32& outMin, Float32& outMax);

    // Exact cSTETerrain::FindMinAndMaxHeights() @ 0x1007362b4: full-grid
    // scan writing mMinHeight (+0x90) / mMaxHeight (+0x94).
    void FindMinAndMaxHeights();

    // Exact cSTETerrain::ClearVertexPropertyFlags(SC4Rect<int> const&,
    // unsigned short) @ 0x100735288: rows [top, bottom] x columns
    // [left, right] inclusive, flags &= ~mask.
    void ClearVertexPropertyFlags(const SC4RectI& rect, uint16_t mask);

    // Exact cSTETerrain::ClearVertexPropertyFlags(unsigned short) @
    // 0x100735810: flags &= ~mask over all vertices.
    void ClearVertexPropertyFlags(uint16_t mask);

    // Exact cSTETerrain::ClearVertexCliffFlipTriangulationFlags @
    // 0x1007352f0: over the rect, vertices with flag bit 0x800 call
    // FlipInternalTriangulation(x, z, false) (vtable+0x2f0 @ 0x100735353)
    // then clear the bit.
    void ClearVertexCliffFlipTriangulationFlags(const SC4RectI& rect);

    // Exact cSTETerrain::FlipInternalTriangulation(unsigned int, unsigned
    // int, bool) @ 0x100738848 (vtable+0x2f0, slot verified). Sets/clears
    // flag bit 0x1000 of the cell's bottom-left vertex; no-op when the bit
    // already matches; the +0x118-gated tail notifies the +0x110 view's
    // SetTriangleIndicesForCell(z, x) @ 0x10074f376.
    void FlipInternalTriangulation(Uint32 x, Uint32 z, bool flip);

    // Exact cSTETerrain::MarkCliff(int, bool, bool) @ 0x100735398 — see
    // terrain.cpp for the three mark paths and the recomputed quad
    // steepness.
    void MarkCliff(Sint32 vertex, bool markFirst, bool markSecond);

    // Exact cSTETerrain::CalculateNormalsAndAssignCliffTextures @
    // 0x100734dc8 — the normal/cliff pass. See terrain.cpp.
    void CalculateNormalsAndAssignCliffTextures(const SC4RectI& rect);

    // Exact cSTETerrain::GetNeighboringSeaVertex(int) const @ 0x100731a1c:
    // first sea cross-neighbor (W, N, E, S) of the vertex, or -1.
    // BINARY ODDITY: the row coordinate divides by vertexCountZ (+0x54)
    // while the column uses vertexCountX (+0x50) @ 0x100731a32/0x100731a40
    // — identical only on square grids; reproduced as-is.
    Sint32 GetNeighboringSeaVertex(Sint32 vertex) const;

    // Exact cSTETerrain::AnalyzeShoreAndCreateCliffs(std::deque<int>&) @
    // 0x1007317fe — see terrain.cpp.
    bool AnalyzeShoreAndCreateCliffs(std::deque<Sint32>& shore);

    // Exact cSTETerrain::CreateShoreCliff(std::vector<tCliffVertInfo>&) @
    // 0x100731ab0 — see terrain.cpp.
    void CreateShoreCliff(std::vector<TerrainCliffVertInfo>& verts);

    // Exact cSTETerrain::CreateShoreCliffs(bool) @ 0x100736d3e — see
    // terrain.cpp.
    bool CreateShoreCliffs(bool redisplay);

    // Exact cSTETerrain::DetectShoreVertices() @ 0x100736ef0: clears flag
    // bit 0x10 over {0, 0, cellCountX, cellCountZ} then sets it on every
    // IsShoreVertex.
    void DetectShoreVertices();

    // Exact cSTETerrain::GetShoreStartingAtVert(int, std::deque<int>&,
    // unsigned short) @ 0x100736f5e.
    void GetShoreStartingAtVert(Sint32 vertex, std::deque<Sint32>& shore,
                                uint16_t mark);

    // Exact cSTETerrain::TrackShoreWithLandToTheRight @ 0x100739610 /
    // TrackShoreWithLandToTheLeft @ 0x100739798 — see terrain.cpp.
    void TrackShoreWithLandToTheRight(Sint32 vertex, std::deque<Sint32>& shore,
                                      uint16_t mark);
    void TrackShoreWithLandToTheLeft(Sint32 vertex, std::deque<Sint32>& shore,
                                     uint16_t mark);

    // Exact cSTETerrain::IsShoreVertex(int) const @ 0x10073751a: land vertex
    // with at least one sea cross-neighbor (W, N, E, S; direction mask from
    // the GetNeighbors @ 0x100734d42 edge logic).
    bool IsShoreVertex(Sint32 vertex) const;

    // Exact cSTETerrain::MarkRegion(int, bool (cSTETerrain::*)(int) const,
    // bool (cSTETerrain::*)(int) const, unsigned short) @ 0x100736728 — see
    // terrain.cpp.
    void MarkRegion(Sint32 radius, bool (cSTETerrain::*qualify)(Sint32) const,
                    bool (cSTETerrain::*markIf)(Sint32) const, uint16_t mark);

    // Exact cSTETerrain::SmoothenMarkedRegion(SC4Rect<int> const&, int,
    // unsigned short) @ 0x10073697a — see terrain.cpp.
    void SmoothenMarkedRegion(const SC4RectI& rect, Sint32 passes, uint16_t mark);

    // Exact cSTETerrain::ComputeBoundsOfMarkedRegion(SC4Rect<int>&,
    // unsigned short) const @ 0x1007359a2: bounding box of vertices with
    // the mark bits, kEmptyRect when none.
    void ComputeBoundsOfMarkedRegion(SC4RectI& out, uint16_t mark) const;

    // Exact cSTETerrain::IsVertexWithMark3(int) const @ 0x100736fd4 (flag
    // bit 0x4) / IsLandAndNotMark3(int) const @ 0x100736ff2 (bit 0x4 clear
    // AND altitude > sea level).
    bool IsVertexWithMark3(Sint32 vertex) const;
    bool IsLandAndNotMark3(Sint32 vertex) const;

    // Exact cSTETerrain::ChangeSeaLevel(bool) @ 0x100735714: tail-jmps
    // vtable+0x168 (SetSeaLevel @ 0x10073e556) with mSeaLevel ±
    // kSeaRaiseLowerAmount (negated through the sign-mask xorps
    // @ 0x100a16ac0 when the bool is false), so the callee's bool return
    // propagates (jmp rax @ 0x100735743).
    bool ChangeSeaLevel(bool raiseLevel);

    // Exact cSTETerrain::SetSeaLevel(float) @ 0x10073e556 (vtable+0x168,
    // slot verified). Range gate against kMinSeaLevel (-10.1f)/kMaxSeaLevel
    // (200.1f) @ 0x10073e562..0x10073e576, stores the level, resets the
    // surface water to sea level over the cached world rect (+0x58), posts
    // message 0x89171496, and tail-redisplayS via vtable+0x228.
    bool SetSeaLevel(Float32 newLevel);

    // Exact cSTETerrain::ChangeTerrainLevel(bool, float) @ 0x100735746:
    // RaiseVertex/LowerVertex over every vertex, then the vtable+0x228
    // redisplay with (1, 1, this+0x58, 1).
    void ChangeTerrainLevel(bool raise, Float32 amount);

    // Exact cSTETerrain::ComputeMinAndMaxAltitudeForTerrainRedisplay @
    // 0x100735a58 — see terrain.cpp.
    void ComputeMinAndMaxAltitudeForTerrainRedisplay(const SC4RectI& rect,
                                                     Float32& outMin,
                                                     Float32& outMax);

    // ---- Test/verify accessors (modeled, not in the binary) ----
    const TerrainLandTypeSlot& LandTypeSlotAt(Sint32 vertex, Sint32 slot) const noexcept {
        static const TerrainLandTypeSlot kEmpty{};
        if (vertex < 0 || slot < 0 || slot > 2 ||
            static_cast<std::size_t>(vertex) >= mLandTypes.size()) return kEmpty;
        return mLandTypes[static_cast<std::size_t>(vertex)][static_cast<std::size_t>(slot)];
    }
    void SetLandTypeSlotForTest(Sint32 vertex, Sint32 slot, uint16_t type,
                                uint16_t value) noexcept {
        if (vertex >= 0 && slot >= 0 && slot <= 2 &&
            static_cast<std::size_t>(vertex) < mLandTypes.size()) {
            mLandTypes[static_cast<std::size_t>(vertex)][static_cast<std::size_t>(slot)] =
                TerrainLandTypeSlot{type, value};
        }
    }
    Uint8 WaterProximityAt(Sint32 x, Sint32 z) const noexcept {
        if (z < 0 || static_cast<std::size_t>(z) >= mWaterProximity.size()) return 0;
        const auto& row = mWaterProximity[static_cast<std::size_t>(z)];
        if (x < 0 || static_cast<std::size_t>(x) >= row.size()) return 0;
        return row[static_cast<std::size_t>(x)];
    }
    const SC4RectI& WaterProximityRect() const noexcept { return mWaterProximityRect; }
    Sint32 NumberOfWaterVerticesField() const noexcept { return mNumberOfWaterVertices; }
    void SetTerrainType(Sint32 t) noexcept { mTerrainType = t; }
    Sint32 TerrainTypeField() const noexcept { return mTerrainType; }
    void SetMoistureSource(TerrainMoistureSource* m) noexcept { mMoistureSource = m; }
    void SetTextureSpanInfoDirtyGate(bool gate) noexcept {
        mTextureSpanInfoDirtyGate = gate;
    }
    // Seeds one ash region with an explicit age (node +0x20 dword; the +0x24
    // dword is cleared by SprayVolcanicAsh when a region is fully covered,
    // @ 0x10073484f..0x100734853).
    void SeedVolcanicAshRegionForTest(const SC4RectI& rect, Sint32 age) {
        VolcanicAshRegion r;
        r.rect = rect;
        r.field20 = age;
        mVolcanicAshRegions.push_back(r);
    }
    Sint32 VolcanicAshRegionCoverFlag(std::size_t index) const noexcept {
        if (index >= mVolcanicAshRegions.size()) return -1;
        auto it = mVolcanicAshRegions.begin();
        std::advance(it, static_cast<std::ptrdiff_t>(index));
        return static_cast<Sint32>((*it).field20 >> 32);
    }

private:
    // Jump-table resolution @ 0x10073218c: effect -> {&cSTETerrain::BrushFnX, 0}.
    static TerrainBrushMemberFn MemberFnForEffect(Sint32 effect);

    // Itanium member-fn pointer dispatch @ 0x100732910..0x100732932 (plain) /
    // 0x100732473..0x1007324b4 (texture): fn word bit 0 -> virtual
    // vtable[fnWord-1] on (this + adj); else direct call on (this + adj).
    bool CallBrushFn(const TerrainBrushMemberFn& fn, Sint32 vertex,
                     const TerrainBrushContext& ctx);

    // The 0x1007320e1 block of ApplyBrushComponents: texture overload when a
    // cS3DTextureBufferInfo* is available, plain overload (repeat count 1)
    // otherwise.
    void ApplyBrushSweep(const S3DTextureBufferInfo* texInfo, Sint32 gridX, Sint32 gridY,
                         Float32 size, Float32 angle, const TerrainBrushMemberFn& fn,
                         TerrainBrushContext& ctx, SC4RectI& rect);

    // vtable+0x48 grid -> vertex index (GetVertIndex @ 0x100740e50 semantics:
    // y * stride + x; dispatched from the Rain/Spring cases @ 0x1007320b1/0x100731eca).
    Sint32 GetVertIndex(Sint32 x, Sint32 y) const noexcept {
        return y * mVertexCountX + x;
    }

    // vtable+0x188 altitude-set entry — cSTETerrain::SetAltitude(int, float)
    // @ 0x10073e192 (slot verified: __ZTV11cSTETerrain 0x100aea6b0 + 0x10 +
    // 0x188 holds 0x10073e192; exact body in terrain.cpp). Kept under the
    // model name SetVertexAltitude because the public raw-grid test
    // accessor SetAltitude already occupies the binary name. Callers:
    // BrushFnLevel @ 0x1007332a4, BrushFnLakeBed @ 0x1007333f6,
    // CreateShoreCliff @ 0x100731bda/0x100731c21.
    void SetVertexAltitude(Sint32 vertex, Float32 altitude) noexcept;

    // Exact cSTETerrain::ApplyWaterErosionBrush @ 0x1007332ae: builds the
    // {gridX, gridY} rect from ctx.x/z, pads it by trunc(strengthCells/2),
    // then runs DoWaterErosion (int)ctx.amount times with the per-strength
    // flow-step count.
    void ApplyWaterErosionBrush(TerrainBrushContext& ctx, Sint32 strengthCells, SC4RectI& rect);

    // Exact cSTESurfaceWater::Rain @ 0x100730820: returns
    // {&cSTETerrain::BrushFnRain, 0} when the surface-water flag at +0x70 is
    // set (so the generic sweep runs); otherwise calls Rain_Static and returns
    // a null member-fn (sweep skipped).
    TerrainBrushMemberFn SurfaceWaterRain(Sint32 vertex, TerrainBrushContext& ctx,
                                           SC4RectI& rect, Float32 strength);

    // Exact cSTETerrain::RaiseVertex @ 0x100734cde / LowerVertex @ 0x100734c7c.
    bool RaiseVertex(Uint32 vertexIndex, Float32 amount) noexcept;
    bool LowerVertex(Uint32 vertexIndex, Float32 amount) noexcept;

    // Float -> Sint32 index via the binary magic (add -0.5, add 12582912.0f,
    // fixed-point round with cmovne/cmovp) used by GetAltitude @ 0x100738928
    // (x is already multiplied by 1/16.0f). See terrain.cpp.
    static Sint32 FloatToVertexIndex(Float32 x) noexcept;

    // Volcanic-ash region list node model (binary node is 0x28 bytes: list
    // links, SC4Rect<int> at +0x10, zero-initialized qword at +0x20 per
    // AddVolcanicAshRegion @ 0x1007316e6..0x100731717).
    struct VolcanicAshRegion {
        SC4RectI rect;
        Sint64   field20 = 0; // node+0x20 (initialized to 0 @ 0x1007316f4)
    };

    // Intrinsic state (binary offsets in the header comment above).
    bool    mInitialized    = false; // +0x18
    Uint32  mRefCount       = 0;     // +0x1c
    Sint32  mCellCountX     = 0;     // +0x40
    Sint32  mCellCountZ     = 0;     // +0x44
    Sint32  mMaxVertexIndexX = 0;    // +0x48
    Sint32  mMaxVertexIndexZ = 0;    // +0x4c
    Sint32  mVertexCountX   = 0;     // +0x50
    Sint32  mVertexCountZ   = 0;     // +0x54
    Sint32  mVertexCount    = 0;     // +0x74
    Float32 mMinHeight      = 0.0f;  // +0x90
    Float32 mMaxHeight      = 0.0f;  // +0x94
    Float32 mSeaLevel       = 0.0f;  // +0x9c
    Sint32  mBrushStrengthIndex = 0; // +0xdc (BrushRadius @ 0x100734db5)
    Sint32  mFloraBrushInfo  = 0;   // +0x150 (Flora gate @ 0x100732017: != 0 required)
    // +0x158/+0x160 flora brush id vector (InitFloraBrushInfo @ 0x1007385e2;
    // modeled as a plain vector).
    std::vector<Uint32> mFloraBrushIds;
    // Modeled boundaries for the unreconstructed +0x130/+0x138/+0x148 callees
    // (see the stand-in structs above; clean-room fallback, not authentic):
    std::vector<TerrainFloraBrushType> mFloraBrushInfoSource; // +0x148 (modeled)
    std::vector<TerrainFloraPlacement> mFloraPlacements;      // modeled observation (not in the binary)
    TerrainDecalPlacementTarget* mDecalTarget130 = nullptr;   // +0x130 (modeled pointer)
    TerrainEffectPlacementTarget mEffectTarget138;             // +0x138 (modeled)
    // +0x88 altitude grid: the binary stores 36-byte per-vertex records with
    // the altitude float at record offset 0; modeled as a flat float vector
    // indexed by vertex index = z*mVertexCountX + x.
    std::vector<Float32> mAltitude;
    TerrainSurfaceWaterState mSurfaceWater; // +0x38
    // 36-byte vertex record flags word at +0x22 (VolcanoAsh @ 0x100733350
    // `or word [altitude+4*idx*9+0x22], 0x400`); sized by Init.
    std::vector<uint16_t> mVertexFlags;
    // Volcanic-ash regions: binary std::list sentinel at this+0x230, node count
    // at this+0x240 (AddVolcanicAshRegion @ 0x100731628).
    std::list<VolcanicAshRegion> mVolcanicAshRegions;
    // Decal map: binary std::map<Uint32, tDecalMapEntry> tree at this+0x268
    // (size at +0x278; cleared by BeginBrushScrub @ 0x100734a8e..0x100734ac2).
    // tDecalMapEntry is not reconstructed; modeled as a count-only map.
    std::map<Uint32, Sint32> mDecalMap;
    // ---- Surface-cover cluster state (binary offsets cited per member) ----
    Sint32 mTerrainType = 0;      // +0x24 (TerrainType @ 0x100740f24; low-cliff
                                  // / beach tex id index @ 0x100733e5f/0x1007340f5)
    // Cached whole-world rect at this+0x58 (compared field-by-field by
    // ComputeMinAndMaxAltitudeForTerrainRedisplay @ 0x100735a81..0x100735aa6;
    // passed by SetSeaLevel @ 0x10073e584 to the water reset and by
    // ChangeTerrainLevel @ 0x1007357ea / SetSeaLevel @ 0x10073e5b6 /
    // CreateShoreCliffs @ 0x100736e7f to the vtable+0x228 redisplay). The
    // fill site is not reconstructed — Init models it as
    // {0, 0, cellCountX, cellCountZ}.
    SC4RectI mWorldRect;             // +0x58
    // Quad-normal scratch buffers at this+0x170 / this+0x188: 24 bytes per
    // column x (two 3-float quad normals), ping-ponged per row by
    // CalculateNormalsAndAssignCliffTextures (@ 0x100734ddf/+0x170,
    // @ 0x100734ded/+0x188; allocation site not reconstructed — modeled in
    // Init as 6 floats per column).
    std::vector<Float32> mQuadNormalsA;  // +0x170
    std::vector<Float32> mQuadNormalsB;  // +0x188
    // 36-byte vertex-record normal floats at +0x4/+0x8/+0xc (phase-2 stores
    // @ 0x100735241..0x100735259).
    std::vector<std::array<Float32, 3>> mVertexNormals;
    // +0x110 cSTETerrainView3D height fields (+0x94/+0x98) — clean-room
    // boundary (see TerrainView3DHeightRange).
    TerrainView3DHeightRange* mView3D = nullptr;
    // cSTETerrain::PostMessageA @ 0x100734b92 (not reconstructed) —
    // recorded no-op (see PostMessageACalls).
    Sint32 mPostMessageACalls = 0;
    Uint32 mPostMessageALastId = 0;
    Sint32 mWaterProximityFieldC8 = 0; // +0xc8, zeroed by ComputeWaterProximityValues
                                  // @ 0x100733cb2 (purpose unresolved)
    SC4RectI mWaterProximityRect = kEmptyRect; // +0xcc dirty rect (union
                                  // @ 0x100733c7f, reset to kEmptyRect
                                  // @ 0x100733cb8..0x100733cca)
    Sint32 mNumberOfWaterVertices = 0; // +0x108 (NumberOfWaterVertices @
                                  // 0x100740f18; recomputed and stored by
                                  // ComputeWaterProximityValues @ 0x100733cdb)
    bool mTextureSpanInfoDirtyGate = false; // +0x118 byte gating the
                                  // MarkTextureSpanInfoDirty call @ 0x100733c0c
    TerrainMoistureSource* mMoistureSource = nullptr; // +0x140 (UpdateMoistureValues
                                  // null gate @ 0x100733d9f; vtable+0x98
                                  // @ 0x100734267)
    // +0x200 water-proximity map: std::vector<std::vector<uint8>> (rows at
    // 24-byte stride, begin ptr at +24*z — ComputeWaterProximityValues
    // @ 0x100733d18/0x100733d28, DecideSurfaceCover @ 0x1007340bf..0x1007340d2).
    std::vector<std::vector<Uint8>> mWaterProximity;
    // 36-byte per-vertex record land-type slots at +0x16/+0x1a/+0x1e
    // (AddLandTypeAtVertex @ 0x10073136f..0x1007313d4, ClearSurfaceTextures
    // @ 0x100733e0a..0x100733e21, AdjustLandTypeRatios @ 0x100731784..0x1007317c6).
    std::vector<std::array<TerrainLandTypeSlot, 3>> mLandTypes;
    // Modeled observation counters (not in the binary).
    Sint32 mReDisplayCount = 0;
    Sint32 mBeginScrubDelegateCount = 0;
};

} // namespace opensc4
