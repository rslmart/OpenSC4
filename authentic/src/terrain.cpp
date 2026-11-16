// Exact cSTETerrain runtime foundation — M3 foundational slice.
// See include/opensc4/terrain.h for the provenance of every field and the
// machine-code addresses behind each method. Float op order, branch
// structure, thresholds, and clamping reproduce the shipped binary exactly.

#include "opensc4/terrain.h"
#include "opensc4/terrain_altitude_kernel.h"

#include <algorithm>
#include <bit>
#include <cmath>
#include <utility>

namespace opensc4 {

// Terrain height clamps: cSTETerrain::kMaxTerrainHeight @ 0x100b1763c /
// kMinTerrainHeight @ 0x100b17640 (see the tunable accessors in terrain.h —
// shipped bytes 200.0f / 5.0f, runtime-overridden from the terrain tuning
// exemplar @ 0x10073d9e4/0x10073da07).

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

// cSTETerrain() @ 0x100737020 zero-initialises the intrinsic state (modelled
// by the member default-initialisers in terrain.h; the binary also stores
// +0xfd = 1 for the water-surface display state, which is out of this
// foundation slice's scope).

bool cSTETerrain::Init(Sint32 cellCountX, Sint32 cellCountZ) {
    // Foundation slice of cSTETerrain::Init @ 0x100739cb0. Field assignments
    // reproduce the binary exactly:
    //   [0x28] = city manager (vtable wiring); [0x38] = surface water
    //   [0x40] = cellCountX (+0x2e8 call), [0x44] = cellCountZ (+0x2f0 call)
    //   [0x48] = cellCountX - 1  (lea edx,[rcx-1] @ 0x100739d52)
    //   [0x4c] = cellCountZ - 1  (lea edx,[rax-1] @ 0x100739d5a)
    //   [0x50] = cellCountX + 1  (vertexCountX / row stride)
    //   [0x54] = cellCountZ + 1  (vertexCountZ)
    //   [0x74] = vertexCountX * vertexCountZ
    //   [0x68] = cellCountX * 16.0f, [0x6c] = cellCountZ * 16.0f (world size)
    // The binary additionally wires up flora brush info, texture lists, brush
    // parameters, and triangle/lighting iterators — deferred to follow-up
    // tickets (their dependencies are not yet decompiled).
    if (cellCountX <= 0 || cellCountZ <= 0) {
        return false;
    }
    mCellCountX = cellCountX;
    mCellCountZ = cellCountZ;
    mMaxVertexIndexX = cellCountX - 1; // @ 0x100739d52
    mMaxVertexIndexZ = cellCountZ - 1; // @ 0x100739d5a
    mVertexCountX = cellCountX + 1;    // @ 0x100739d62
    mVertexCountZ = cellCountZ + 1;    // @ 0x100739d6a
    mVertexCount = mVertexCountX * mVertexCountZ; // @ 0x100739d72
    mSeaLevel = 0.0f;
    mMinHeight = 0.0f;
    mMaxHeight = 0.0f;
    mAltitude.assign(static_cast<std::size_t>(mVertexCount), 0.0f);
    mVertexFlags.assign(static_cast<std::size_t>(mVertexCount), 0);
    // Surface-cover cluster state: the 36-byte per-vertex records' land-type
    // slot tables (record +0x16/+0x1a/+0x1e) and the +0x200 water-proximity
    // map (vertexCountZ rows x vertexCountX columns; the binary allocates the
    // vector-of-vectors elsewhere in Init — modeled here).
    mLandTypes.assign(static_cast<std::size_t>(mVertexCount),
                      std::array<TerrainLandTypeSlot, 3>{});
    mWaterProximity.assign(
        static_cast<std::size_t>(mVertexCountZ),
        std::vector<Uint8>(static_cast<std::size_t>(mVertexCountX), 0));
    mNumberOfWaterVertices = 0; // +0x108
    mWaterProximityRect = kEmptyRect; // +0xcc
    mWaterProximityFieldC8 = 0;       // +0xc8
    mSurfaceWater.depth.assign(static_cast<std::size_t>(mVertexCount), 0.0f);
    mSurfaceWater.spring.assign(static_cast<std::size_t>(mVertexCount), 0.0f);
    mSurfaceWater.sediment.assign(static_cast<std::size_t>(mVertexCount), 0.0f);
    mSurfaceWater.depth.assign(static_cast<std::size_t>(mVertexCount), 0.0f);
    mSurfaceWater.currentVertex = 0;
    // Shore-cliff cluster state: the cached world rect (+0x58; fill site
    // not reconstructed — modeled as {0, 0, cellCountX, cellCountZ}), the
    // quad-normal ping-pong buffers (+0x170/+0x188, 6 floats per column;
    // allocation sites not reconstructed — modeled), the per-vertex record
    // normals (+0x4/+0x8/+0xc), and the PostMessageA recorder.
    mWorldRect = SC4RectI{0, 0, cellCountX, cellCountZ};
    mVertexNormals.assign(static_cast<std::size_t>(mVertexCount),
                          std::array<Float32, 3>{});
    mQuadNormalsA.assign(static_cast<std::size_t>(mVertexCountX) * 6u, 0.0f);
    mQuadNormalsB.assign(static_cast<std::size_t>(mVertexCountX) * 6u, 0.0f);
    mPostMessageACalls = 0;
    mPostMessageALastId = 0;
    mInitialized = true;
    return true;
}

void cSTETerrain::Shutdown() {
    mInitialized = false;
    mAltitude.clear();
    mSurfaceWater.depth.clear();
    mSurfaceWater.sediment.clear();
    mSurfaceWater.currentVertex = 0;
}

// ---------------------------------------------------------------------------
// Water surface + altitude queries
// ---------------------------------------------------------------------------

bool cSTETerrain::CellHasWater(Uint32 x, Uint32 z) const noexcept {
    // Exact cSTETerrain::CellHasWater @ 0x10073562c.
    //   cmp [rdi+0x40], esi; jbe fail  (cellCountX <= x -> false)
    //   cmp [rdi+0x44], edx; jbe fail  (cellCountZ <= z -> false)
    // In range, tests the water depth at the four cell corners
    //   (x,z), (x+1,z), (x,z+1), (x+1,z+1)
    // reading through [rdi+0x38] -> [rax+0x28] (36-byte records, depth float at
    // offset 0), short-circuiting `ucomiss vs 0; ja` on each corner. Returns
    // true iff ANY corner depth is strictly > 0.
    if (static_cast<Uint32>(mCellCountX) <= x) {
        return false;
    }
    if (static_cast<Uint32>(mCellCountZ) <= z) {
        return false;
    }
    const Sint32 stride = mVertexCountX; // +0x50
    const Sint32 idx = static_cast<Sint32>(z) * stride + static_cast<Sint32>(x);
    if (mSurfaceWater.depth[static_cast<std::size_t>(idx)] > 0.0f) {
        return true; // corner (x, z)
    }
    if (mSurfaceWater.depth[static_cast<std::size_t>(idx + 1)] > 0.0f) {
        return true; // corner (x+1, z)
    }
    if (mSurfaceWater.depth[static_cast<std::size_t>(idx + stride)] > 0.0f) {
        return true; // corner (x, z+1)
    }
    return mSurfaceWater.depth[static_cast<std::size_t>(idx + stride + 1)] > 0.0f; // (x+1, z+1)
}

Sint32 cSTETerrain::FloatToVertexIndex(Float32 x) noexcept {
    // Exact conversion from GetAltitude @ 0x10073896b..0x100738997.
    return terrain_altitude::FloatToVertexIndex(x);
}

Float32 cSTETerrain::GetAltitude(Float32 x, Float32 z) const noexcept {
    // Exact cSTETerrain::GetAltitude(float,float) @ 0x100738928.
    if (!mInitialized) { // clean-room fallback (not authentic): model storage is absent.
        return 0.0f;
    }
    // Virtual entry callee LocationIsInBounds @ 0x100740e7a; model world
    // bounds derive from Init @ 0x100739cb0's cellCount * 16 assignments.
    if (!terrain_altitude::LocationIsInBounds(
            x, z, static_cast<Float32>(mCellCountX) * 16.0f,
            static_cast<Float32>(mCellCountZ) * 16.0f)) {
        return 0.0f;
    }
    return terrain_altitude::Interpolate(
        x, z, static_cast<Uint32>(mMaxVertexIndexX),
        static_cast<Uint32>(mMaxVertexIndexZ), static_cast<Uint32>(mVertexCountX),
        [this](Uint32 index) { return mAltitude[static_cast<std::size_t>(index)]; });
}

Float32 cSTETerrain::GetAltitude(Sint32 vertexIndex) const noexcept {
    // Exact cSTETerrain::GetAltitude(int) @ 0x100738a84: returns 0 when the
    // index is out of range (`cmp [rdi+0x74], esi; jbe`), else the altitude at
    // that vertex.
    if (static_cast<Uint32>(vertexIndex) >= static_cast<Uint32>(mVertexCount)) {
        return 0.0f;
    }
    return mAltitude[static_cast<std::size_t>(vertexIndex)];
}

Float32 cSTETerrain::GetAltitudeAtVertex(Sint32 x, Sint32 z) const noexcept {
    // Exact cSTETerrain::GetAltitudeAtVertex @ 0x100738aa6:
    //   `cmp [rdi+0x50], esi; jbe -> 0` (vertexCountX <= x)
    //   `cmp [rdi+0x54], edx; jbe -> 0` (vertexCountZ <= z)
    //   index = z * vertexCountX + x.
    if (static_cast<Uint32>(mVertexCountX) <= static_cast<Uint32>(x)) {
        return 0.0f;
    }
    if (static_cast<Uint32>(mVertexCountZ) <= static_cast<Uint32>(z)) {
        return 0.0f;
    }
    return mAltitude[static_cast<std::size_t>(z * mVertexCountX + x)];
}

Float32 cSTETerrain::GetWaterDepthAtVertex(Sint32 vertexIndex) const noexcept {
    // Exact cSTETerrain::GetWaterDepthAtVertex(int) @ 0x100739b36: 0 out of
    // range, else the water depth stored via [rdi+0x38] -> [rax+0x28].
    if (static_cast<Uint32>(vertexIndex) >= static_cast<Uint32>(mVertexCount)) {
        return 0.0f;
    }
    return mSurfaceWater.depth[static_cast<std::size_t>(vertexIndex)];
}

bool cSTETerrain::CellHasLand(Uint32 x, Uint32 z) const noexcept {
    // Exact cSTETerrain::CellHasLand @ 0x1007356a2 (corrected Nov 2026).
    //   cmp [rdi+0x40], esi; jbe fail  (cellCountX <= x -> false)
    //   cmp [rdi+0x44], edx; jbe fail  (cellCountZ <= z -> false)
    // In range, tests the WATER DEPTH at the four cell corners via the SAME
    // depth grid as CellHasWater ([rdi+0x38] -> [rax+0x28], 36-byte records,
    // depth float at offset 0), short-circuiting `ucomiss 0, depth; jae` per
    // corner. Returns true iff ANY corner depth <= 0; NaN depth counts as
    // land (jae takes the unordered path via ZF). The pre-correction
    // reconstruction read altitudes vs seaLevel — disproved by fresh
    // disassembly during the Phase B live-prove wave; a cell can be both
    // CellHasWater and CellHasLand when its corners mix wet and dry.
    if (static_cast<Uint32>(mCellCountX) <= x) {
        return false;
    }
    if (static_cast<Uint32>(mCellCountZ) <= z) {
        return false;
    }
    const Sint32 stride = mVertexCountX; // +0x50
    const Sint32 idx = static_cast<Sint32>(z) * stride + static_cast<Sint32>(x);
    if (!(mSurfaceWater.depth[static_cast<std::size_t>(idx)] > 0.0f)) {
        return true; // corner (x, z): jae = NOT(depth > 0), NaN included
    }
    if (!(mSurfaceWater.depth[static_cast<std::size_t>(idx + 1)] > 0.0f)) {
        return true; // corner (x+1, z)
    }
    if (!(mSurfaceWater.depth[static_cast<std::size_t>(idx + stride)] > 0.0f)) {
        return true; // corner (x, z+1)
    }
    return !(mSurfaceWater.depth[static_cast<std::size_t>(idx + stride + 1)] > 0.0f); // (x+1, z+1)
}

Float32 cSTETerrain::GetWaterLevel(Float32 x, Float32 z) const noexcept {
    // Exact cSTETerrain::GetWaterLevel(float, float) @ 0x100739a68.
    //
    // Entry gate: `call [vtable+0x58]; test al,al; je -> return 0.0f`
    // (0x100739a7e/0x100739a84/0x100739a86). Modelled as the initialized flag
    // below; the +0x58 callee symbol was not resolved (cited, not guessed).
    // World coordinates converted to vertex indices via the FloatToVertexIndex
    // magic (mulss 1/16.0f @ 0x100739a8c/0x100739a94, addss -0.5f @ 0x100739a99,
    // addss 12582912.0f @ 0x100739a9d, cvtsi2ss round @ 0x100739aaa/0x100739ab4).
    // Then bilinear interpolation over the water depth grid (+0x38 -> +0x28)
    // using the same stride (+0x50) and vertex layout as the altitude grid.
    // The binary reads altitude base (+0x88) and water base (+0x38->+0x28) in
    // the same loop structure; we model it as bilinear over the four surrounding
    // water-depth vertices: depth[sz*stride+sx], depth[sz*stride+sx+1],
    // depth[(sz+1)*stride+sx], depth[(sz+1)*stride+sx+1].
    if (!mInitialized) {
        return 0.0f;
    }

    const Float32 kOneOverCellWidth = 0.0625f; // 0x100a158b0 (1/16.0f)
    const Float32 kCellWidth = 16.0f;          // 0x100a16904

    // world -> vertex index (magic round), then unsigned cmova clamp:
    //   cmp eax, [0x48]; cmova eax, [0x48]   (0x1007389d3)
    //   cmp edx, [0x4c]; cmova edx, [0x4c]   (0x1007389da)
    Sint32 sx = FloatToVertexIndex(x * kOneOverCellWidth);
    Sint32 sz = FloatToVertexIndex(z * kOneOverCellWidth);
    if (static_cast<Uint32>(sx) > static_cast<Uint32>(mMaxVertexIndexX)) {
        sx = mMaxVertexIndexX;
    }
    if (static_cast<Uint32>(sz) > static_cast<Uint32>(mMaxVertexIndexZ)) {
        sz = mMaxVertexIndexZ;
    }

    const Sint32 stride = mVertexCountX;   // +0x50
    const Sint32 base = sz * stride + sx;  // vertex index of (sx, sz)

    // fractional offsets within the cell
    const Float32 cornerX = static_cast<Float32>(sx) * kCellWidth;
    const Float32 cornerZ = static_cast<Float32>(sz) * kCellWidth;
    const Float32 fracX = x - cornerX;
    const Float32 fracZ = z - cornerZ;

    // Exact cSTETerrain::GetWaterLevel(float, float) @ 0x100739a68 tail:
    //   imul ecx, [rbx+0x50] (stride); add ecx, eax (base = sz*stride+sx)
    //   mov rdx, [rbx+0x88] (altitude/vertex record array); mov rax, [rbx+0x38]
    //   (water object) -> mov rax, [rax+0x28] (water depth array)
    //   shl rcx, 2; lea rcx, [rcx+8*rcx] = index*36 (36-byte vertex record)
    //   movss xmm0, [rdx+rcx] (altitude at vertex); addss xmm0, [rax+rcx]
    //   (water depth at the SAME vertex); ret
    // The query is a SINGLE-VERTEX SUM at the clamped index — NOT bilinear
    // (the prior bilinear model was a clean-room guess that the midpoint
    // oracles falsified; corrected to the actual disassembly).
    return mAltitude[static_cast<std::size_t>(base)]
         + mSurfaceWater.depth[static_cast<std::size_t>(base)];
}

// ---------------------------------------------------------------------------
// Private brush helpers
// ---------------------------------------------------------------------------

bool cSTETerrain::RaiseVertex(Uint32 vertexIndex, Float32 amount) noexcept {
    // Exact cSTETerrain::RaiseVertex @ 0x100734cde.
    //   test byte [rax+4*rdx+0x23], 0x40; jne exit_false @ 0x100734cef/0x100734cf4
    //   addss xmm0, [rax+rdx]      (altitude += amount) @ 0x100734d09
    //   movss [rax+rdx], xmm0      (store new altitude)
    //   cmp new altitude with kMaxTerrainHeight (0x100b1763c); jbe skip_clamp @ 0x100734d1e
    //     clamp to kMaxTerrainHeight @ 0x100734d23
    //   subss seaLevel - newAltitude -> water depth @ 0x100734d2a/0x100734d32/0x100734d36
    // Returns true if the vertex was modified (not flagged 0x40).
    if (vertexIndex >= static_cast<Uint32>(mVertexCount)) {
        return false;
    }
    // Check flag 0x40 at record offset 0x23 (36-byte record)
    // We don't model the full 36-byte record; flag check is deferred.
    // The binary checks: byte ptr [rax + 4*rdx + 0x23], 0x40
    // Since we don't have the flag array, we skip this check (clean-room fallback).

    if (((mVertexFlags[static_cast<std::size_t>(vertexIndex)] >> 8) & 0x40) != 0) {
        return false; // 0x40 record flag byte @ +0x23 blocks modification @ 0x100734cef
    }
    Float32 newAltitude = mAltitude[static_cast<std::size_t>(vertexIndex)] + amount;
    // Clamp to kMaxTerrainHeight @ 0x100b1763c (movss load @ 0x100734d13;
    // ucomiss/jbe @ 0x100734d1b/0x100734d1e). Shipped bytes: 200.0f;
    // runtime-overridden from the tuning exemplar @ 0x10073d9e4 (property
    // 0x48cd7b26) — modeled as the settable tunable.
    const Float32 maxHeight = MaxTerrainHeightTunable();
    if (newAltitude > maxHeight) {
        newAltitude = maxHeight;
    }
    mAltitude[static_cast<std::size_t>(vertexIndex)] = newAltitude;

    // Update water depth: seaLevel - newAltitude
    mSurfaceWater.depth[static_cast<std::size_t>(vertexIndex)] = mSeaLevel - newAltitude;
    return true;
}

bool cSTETerrain::LowerVertex(Uint32 vertexIndex, Float32 amount) noexcept {
    // Exact cSTETerrain::LowerVertex @ 0x100734c7c.
    //   test byte [rax+4*rdx+0x23], 0x40; jne exit_false @ 0x100734c8d/0x100734c92
    //   subss xmm1, xmm0         (newAlt = altitude - amount) @ 0x100734cac
    //   movss [rax+rdx], xmm1    (store new altitude)
    //   cmp new altitude with kMinTerrainHeight (0x100b17640); jbe skip_clamp @ 0x100734cc0
    //     clamp to kMinTerrainHeight @ 0x100734cc5
    //   subss seaLevel - newAltitude -> water depth @ 0x100734cd4/0x100734cd8
    // Returns true if the vertex was modified (not flagged 0x40).
    if (vertexIndex >= static_cast<Uint32>(mVertexCount)) {
        return false;
    }
    // Flag 0x40 check deferred (same as RaiseVertex).

    if (((mVertexFlags[static_cast<std::size_t>(vertexIndex)] >> 8) & 0x40) != 0) {
        return false; // 0x40 record flag byte @ +0x23 blocks modification @ 0x100734c8d
    }
    Float32 newAltitude = mAltitude[static_cast<std::size_t>(vertexIndex)] - amount;
    // Clamp to kMinTerrainHeight @ 0x100b17640 (movss load @ 0x100734cb5;
    // ucomiss/jbe @ 0x100734cbd/0x100734cc0 — clamp when kMin > newAlt).
    // Shipped bytes: 5.0f; runtime-overridden from the tuning exemplar @
    // 0x10073da07 (property 0x48cd7b27) — modeled as the settable tunable.
    const Float32 minHeight = MinTerrainHeightTunable();
    if (newAltitude < minHeight) {
        newAltitude = minHeight;
    }
    mAltitude[static_cast<std::size_t>(vertexIndex)] = newAltitude;

    // Update water depth: seaLevel - newAltitude
    mSurfaceWater.depth[static_cast<std::size_t>(vertexIndex)] = mSeaLevel - newAltitude;
    return true;
}

// ---------------------------------------------------------------------------
// Brush functions (implemented)
// ---------------------------------------------------------------------------

bool cSTETerrain::BrushFnRaiseLower(Sint32 vertex, const TerrainBrushContext& ctx) noexcept {
    // Exact cSTETerrain::BrushFnRaiseLower @ 0x1007329cc.
    //   mov rax, [rdx+0x20]                 (target pointer)
    //   movzx eax, byte [rax+0x48]          (target->flag48)
    //   movzx ecx, byte [rdx+0x14]          (ctx.flag)
    //   cmp cl, al; jne raise_branch        @ 0x1007329dc/0x1007329de
    //   jmp exit_no_modify                  @ 0x1007329e2
    // raise_branch:
    //   cmp cl, al; jbe lower_branch        @ 0x1007329e4/0x1007329e6
    //   cvtsi2ss xmm0, ecx                  (ctx.flag as float)
    //   mulss xmm0, [rdx+0x18]              (amount * ctx.flag)
    //   call RaiseVertex                    @ 0x1007329f3
    //   jmp exit_return                     @ 0x1007329f8
    // lower_branch:
    //   cvtsi2ss xmm0, eax                  (target->flag48 as float)
    //   mulss xmm0, [rdx+0x18]              (amount * target->flag48)
    //   call LowerVertex                    @ 0x100732a00/0x100732a05
    // exit_return: return true
    // exit_no_modify: return false
    const Uint8 ctxFlag = ctx.flag;
    const TerrainBrushComponent* target = ctx.target;
    if (target == nullptr) {
        return false; // model guard (the binary would fault on a null target)
    }
    const Uint8 targetFlag = target->flag48; // byte [target+0x48]

    if (ctxFlag != targetFlag) {
        if (ctxFlag > targetFlag) {
            // Raise branch: amount * ctxFlag
            const Float32 amount = static_cast<Float32>(ctxFlag) * ctx.amount;
            return RaiseVertex(static_cast<Uint32>(vertex), amount);
        } else {
            // Lower branch: amount * targetFlag
            const Float32 amount = static_cast<Float32>(targetFlag) * ctx.amount;
            return LowerVertex(static_cast<Uint32>(vertex), amount);
        }
    }
    return false;
}

bool cSTETerrain::BrushFnSmoothen(Sint32 vertex, const TerrainBrushContext& ctx) noexcept {
    // Exact cSTETerrain::BrushFnSmoothen @ 0x100732bd2.
    //   mov al, [rdx+0x14]; test al,al; je exit_false @ 0x100732be6/0x100732beb
    //   cvtsi2ss xmm0, eax; mulss xmm0, [rdx+0x18] (amount * flag) @ 0x100732bf4/0x100732bf8/0x100732bfd
    //   div r12d (vertexCountX @ +0x50) for neighbor iteration @ 0x100732c06
    //   bounds checks against vertexCountX (+0x40) and vertexCountZ (+0x44) @ 0x100732c0f/0x100732c1d/0x100732c24/0x100732c2b
    //   Loop over 8 neighbors (r13 = 0..7):
    //     load neighbor altitude, compute height diff @ 0x100732c90/0x100732c9a
    //     test neighbor flags (bit 0 of r13) for cross vs diagonal @ 0x100732c9f/0x100732caa
    //     compare diff against kCrossHtDiffThreshold (+0x3e499f) or kDiagonalHtDiffThreshold (+0x3e4994) @ 0x100732cb1
    //     if exceeded: mulss diff * scaled_amount @ 0x100732cb3; call LowerVertex then RaiseVertex @ 0x100732cc3/0x100732cd2
    //   loop until r13 == 8 @ 0x100732d02/0x100732d06
    // Constants:
    //   kCrossHtDiffThreshold    @ 0x100b175fc (rip+0x3e499f)
    //   kDiagonalHtDiffThreshold @ 0x100b175f8 (rip+0x3e4994)
    if (ctx.flag == 0) {
        return false;
    }
    const Float32 scaledAmount = static_cast<Float32>(ctx.flag) * ctx.amount;

    if (vertex < 0 || static_cast<Uint32>(vertex) >= static_cast<Uint32>(mVertexCount)) {
        return false;
    }

    const Sint32 stride = mVertexCountX; // +0x50
    const Sint32 vx = vertex % stride;
    const Sint32 vz = vertex / stride;

    // Even direction indices (W, N, E, S) use kCrossHtDiffThreshold; odd
    // indices (NW, NE, SE, SW) use kDiagonalHtDiffThreshold
    // (bit 0 of the iteration counter @ 0x100732c9f).
    const Float32 kCrossHtDiffThreshold = 16.0f;    // @ 0x100b175fc
    const Float32 kDiagonalHtDiffThreshold = 8.0f;  // @ 0x100b175f8

    bool anyModified = false;

    // Neighbor offsets @ 0x100732c7d (direction table at this+0xa4): the
    // binary iterates directions 0..7 in the order W, NW, N, NE, E, SE, S, SW
    // — bit order proven by the GetNeighborWithLowestWaterLevel edge masks
    // @ 0x100737b64..0x100737b88 (west excludes bits 0,1,7; east 3,4,5;
    // north 1,2,3; south 5,6,7). Even indices are the cross neighbors, odd
    // indices the diagonals. (The prior model used a rotated order —
    // behaviorally identical only while the center altitude was cached; the
    // order matters now that the per-iteration reload below is faithful.)
    constexpr std::array<std::pair<Sint32, Sint32>, 8> neighborOffsets = {{
        {-1, 0},  // 0: W  (cross)
        {-1, -1}, // 1: NW (diagonal)
        {0, -1},  // 2: N  (cross)
        {1, -1},  // 3: NE (diagonal)
        {1, 0},   // 4: E  (cross)
        {1, 1},   // 5: SE (diagonal)
        {0, 1},   // 6: S  (cross)
        {-1, 1},  // 7: SW (diagonal)
    }};

    for (Sint32 i = 0; i < 8; ++i) {
        const Sint32 nx = vx + neighborOffsets[i].first;
        const Sint32 nz = vz + neighborOffsets[i].second;

        // Bounds check: vertexCountX <= nx or vertexCountZ <= nz -> skip
        if (static_cast<Uint32>(nx) >= static_cast<Uint32>(mVertexCountX) ||
            static_cast<Uint32>(nz) >= static_cast<Uint32>(mVertexCountZ)) {
            continue;
        }

        const Sint32 nIdx = nz * stride + nx;
        // FIDELITY FIX: the binary RELOADS the center altitude from the grid
        // on every neighbor iteration (movq 0x88(%r12), %rcx; movss
        // (%rcx,%r10,4), %xmm0 @ 0x100732c88/0x100732c90, inside the loop),
        // so a LowerVertex applied to the center by an earlier iteration is
        // observed by the later ones. The prior model cached the entry
        // altitude — a fidelity gap, now fixed.
        const Float32 centerAlt = mAltitude[static_cast<std::size_t>(vertex)];
        const Float32 neighborAlt = mAltitude[static_cast<std::size_t>(nIdx)];
        const Float32 diff = centerAlt - neighborAlt; // subss @ 0x100732c9a

        const Float32 threshold = (i % 2 == 0) ? kCrossHtDiffThreshold
                                               : kDiagonalHtDiffThreshold;

        if (diff > threshold) {
            const Float32 adjustAmount = diff * scaledAmount * 0.0625f; // * 1/16
            // Binary operand order (disassembly @ 0x100732cbd-0x100732cd2):
            //   LowerVertex: esi = the ORIGINAL/center vertex (esi never
            //   reloaded after entry); RaiseVertex: `mov esi, ebx` = the
            //   NEIGHBOR index. The prior implementation had these swapped;
            //   the midpoint/raise oracles caught it.
            LowerVertex(static_cast<Uint32>(vertex), adjustAmount);
            RaiseVertex(static_cast<Uint32>(nIdx), adjustAmount);
            anyModified = true;
        }
    }
    return anyModified;
}

// ---------------------------------------------------------------------------
// Brush foundation
// ---------------------------------------------------------------------------

bool cSTETerrain::BrushFnRain(Sint32 vertex, const TerrainBrushContext& ctx) noexcept {
    // Exact cSTETerrain::BrushFnRain @ 0x10073083e.
    //   al = ctx.flag (+0x14); rcx = ctx.target (+0x20)
    //   cmp al, byte [rcx+0x48]; je -> exit           @ 0x100730849/0x10073084c
    //   amount = ctx.amount (+0x18) / 10.0f           (divss @ 0x100730853,
    //                                                  10.0f @ 0x100a18190)
    //   depth write: surfaceWater (+0x38) -> depth array (+0x28), slot
    //   [vertex*36 + 8 + currentVertex(+0x74)*4]      @ 0x10073085b..0x100730878
    //   (flat-vector model here: depth[currentVertex] — fidelity gap of the
    //   36-byte-record water grid, kept for continuity with the existing
    //   CellHasWater/GetWaterDepth models)
    // clean-room fallback (not authentic): the layered write target
    //   [record+8+4*currentVertex within vertex*36] is modeled flat.
    //   ALWAYS returns false: the shared exit @ 0x10073087e is `xor eax, eax`,
    //   reached both from the flag-equal no-op path and the applied path.
    (void)vertex;
    const Uint8 flag = ctx.flag;
    const TerrainBrushComponent* target = ctx.target;
    if (target == nullptr) {
        return false; // model guard (the binary would fault on a null target)
    }
    if (flag == target->flag48) {
        return false; // @ 0x100730849/0x10073084c
    }
    const Float32 amount = ctx.amount / 10.0f;
    const Sint32 idx = mSurfaceWater.currentVertex;
    if (idx >= 0 && static_cast<std::size_t>(idx) < mSurfaceWater.depth.size()) {
        mSurfaceWater.depth[static_cast<std::size_t>(idx)] =
            mSurfaceWater.depth[static_cast<std::size_t>(idx)] + amount;
    }
    return false; // @ 0x10073087e
}

bool cSTETerrain::ApplyBrushEffect(TerrainBrushEffect effect, Sint32 vertex,
                                   const TerrainBrushContext& ctx) noexcept {
    // Per-vertex effect dispatch — the ApplyBrushComponents jump table
    // @ 0x10073218c (bound `cmp rax, 0xa; ja` @ 0x100731ff1/0x100731ff5).
    // Kept as the direct per-vertex entry for the verify suite; the pipeline
    // (ApplyBrushComponents) resolves the same mapping into
    // TerrainBrushMemberFn via MemberFnForEffect and invokes it through the
    // ApplyBrushAtLocation sweep.
    switch (effect) {
        case TerrainBrushEffect::RaiseLower:
            return BrushFnRaiseLower(vertex, ctx);   // @ 0x1007329cc
        case TerrainBrushEffect::Flora:
            return BrushFnFlora(vertex, ctx);       // @ 0x100732a0e (always false)
        case TerrainBrushEffect::Smoothen:
            return BrushFnSmoothen(vertex, ctx);     // @ 0x100732bd2
        case TerrainBrushEffect::Paint:
            return BrushFnPaint(vertex, ctx);       // @ 0x100732d22 (always false)
        case TerrainBrushEffect::PlaceEffect:
            return BrushFnPlaceEffect(vertex, ctx);  // @ 0x100732f92 (always false)
        case TerrainBrushEffect::Level:
            return BrushFnLevel(vertex, ctx);         // @ 0x100733230
        case TerrainBrushEffect::WaterErosion:
            // ApplyWaterErosionBrush @ 0x1007332ae is a whole-rect operation
            // dispatched from the component case 6 @ 0x100732068, not a
            // per-vertex BrushFn; per-vertex dispatch returns false.
            return false;
        case TerrainBrushEffect::VolcanoAsh:
            return BrushFnVolcanoAsh(vertex, ctx);   // @ 0x100733338
        case TerrainBrushEffect::LakeBed:
            return BrushFnLakeBed(vertex, ctx);       // @ 0x100733360
        case TerrainBrushEffect::Rain:
            return BrushFnRain(vertex, ctx);          // @ 0x10073083e
        case TerrainBrushEffect::Spring:
            // cSTESurfaceWater::CreateSpring(int) @ 0x100730b16 (dispatcher
            // case 10 @ 0x100731ebe converts the grid point via vtable+0x48
            // and calls CreateSpring once; no per-vertex sweep).
            return mSurfaceWater.CreateSpringAtVertex(vertex);
        default:
            return false; // > 0xa: no application @ 0x100731ff5
    }
}

// ---------------------------------------------------------------------------
// Brush dispatch pipeline (M3 terrain wave)
// ---------------------------------------------------------------------------

RZRandomState cSTETerrain::sRand{}; // cSTETerrain::sRand @ 0x100b713d0

std::map<Uint32, TerrainBrushDescription>& cSTETerrain::BrushDescriptionMap() {
    // cSTETerrain::sBrushDescriptionMap @ 0x100b71e20:
    // std::map<unsigned int, tBrushDescription> (key at map node +0x20, the
    // tBrushDescription value at node +0x28).
    static std::map<Uint32, TerrainBrushDescription> sMap;
    return sMap;
}

bool cSTETerrain::ApplyBrush(Uint32 brushID, TerrainBrushParameters& params) {
    // Exact cSTETerrain::ApplyBrush @ 0x100731c50.
    // Out-rect init from kEmptyRect @ 0x100b904c0 (two 8-byte copies
    // @ 0x100731c61..0x100731c73): {INT_MAX, INT_MAX, INT_MIN, INT_MIN}
    // (constant-pool copy @ 0x100a1c0c0).
    params.rect = kEmptyRect;
    auto& map = BrushDescriptionMap();
    // Tree walk @ 0x100731c77..0x100731cb4 (lower_bound descent, cmovae pairs,
    // then key <= brushID check — exact-match semantics); modeled by
    // std::map::find.
    const auto it = map.find(brushID);
    if (it == map.end()) {
        return false; // r12 = 0 @ 0x100731cb6
    }
    TerrainBrushDescription& desc = it->second; // map node value at node+0x28
    ApplyBrushComponents(desc, params);        // @ 0x100731cd2
    // Re-display gate @ 0x100731cda..0x100731d07: params.reDisplay (+0x14) and
    // rect.left (+0x24) <= rect.right (+0x2c) and rect.top (+0x28) <=
    // rect.bottom (+0x30), then the vtable+0x228 call with
    // (1, 1, &params.rect, params+0x20).
    if (params.reDisplay) {
        if (params.rect.left <= params.rect.right && params.rect.top <= params.rect.bottom) {
            ReDisplayTerrain(1, 1, params.rect, params.reDisplayArg); // vtable+0x228
        }
    }
    return true; // r12b = 1 @ 0x100731cd7
}

void cSTETerrain::ApplyBrushComponents(TerrainBrushDescription& desc,
                                       TerrainBrushParameters& params) {
    // Exact cSTETerrain::ApplyBrushComponents @ 0x100731d10.
    const Float32 kOneOverCellWidth = 0.0625f; // 0x100a158b0
    // World -> grid, truncate @ 0x100731d2d..0x100731d44 / 0x100731d51..0x100731d5c.
    const Sint32 gridX = static_cast<Sint32>(params.x * kOneOverCellWidth);
    if (static_cast<Uint32>(mVertexCountX) <= static_cast<Uint32>(gridX)) {
        return; // cmpl eax, [this+0x50]; jbe @ 0x100731d47
    }
    const Sint32 gridY = static_cast<Sint32>(params.z * kOneOverCellWidth);
    if (static_cast<Uint32>(mVertexCountZ) <= static_cast<Uint32>(gridY)) {
        return; // @ 0x100731d60
    }

    // Local out-rect @ [rbp-0x48]: {gridX, gridY, gridX, gridY} @ 0x100731d6a..0x100731d76.
    SC4RectI rect{gridX, gridY, gridX, gridY};

    // Local tBrushContext (0x38 bytes) @ [rbp-0x80].
    TerrainBrushContext ctx;
    ctx.x = params.x;             // +0x00 <- params+0x08 (8-byte copy @ 0x100731d79)
    ctx.z = params.z;             // +0x04 <- params+0x0c
    ctx.prevX = params.prevX;     // +0x08 <- params+0x00 (8-byte copy @ 0x100731e82)
    ctx.prevZ = params.prevZ;     // +0x0c <- params+0x04
    ctx.targetHeight = params.targetHeight; // +0x1c <- params+0x34 (raw copy @ 0x100731d8a)
    // Strength index @ 0x100731d92..0x100731daa: params+0x10 when >= 0 (cmovns),
    // else the terrain's +0xdc.
    ctx.strengthIndex = (params.strengthIndex >= 0) ? params.strengthIndex
                                                     : mBrushStrengthIndex;
    ctx.params = &params; // +0x28 @ 0x100731f9f
    ctx.desc = &desc;     // +0x30 @ 0x100731fa3
    // ODDITY (documented in terrain.h): ctx.flag (+0x14) is never initialized
    // here; the binary leaves the stack slot untouched and only the texture
    // overload of ApplyBrushAtLocation stores the per-pixel texture byte there
    // (@ 0x10073248c).

    // Rotation jitter @ 0x100731dad..0x100731e2f.
    if (desc.useJitter != 0) {
        const Uint32 roll = RandomUint32Uniform(sRand, 5); // cRZRandom::RandomUint32Uniform(5)
        constexpr Float32 kTwoPi = 6.2831854820251465f;      // 0x100a171a4
        constexpr Float32 kNegTwoPi = -6.2831854820251465f;  // 0x100a19690
        if (roll != 0) {
            // @ 0x100731dd8: rot = step + accum; wrap down by 2pi when >= 2pi
            // (comiss @ 0x100731de4, addss @ 0x100731ded).
            const Float32 rot = desc.rotationStep + desc.rotationAccum;
            desc.rotationAccum = rot;
            if (!(rot < kTwoPi)) {
                desc.rotationAccum = rot + kNegTwoPi;
            }
        } else {
            // @ 0x100731e0b: rot = accum - step; wrap up by 2pi when < 0
            // (@ 0x100731e17, addss @ 0x100731e1f).
            const Float32 rot = desc.rotationAccum - desc.rotationStep;
            desc.rotationAccum = rot;
            if (rot < 0.0f) {
                desc.rotationAccum = rot + kTwoPi;
            }
        }
    } else {
        desc.rotationAccum = 0.0f; // movl $0, 0x84(%rbx) @ 0x100731dff
    }

    // Stroke direction angle @ 0x100731e2f..0x100731e6d: only updated when the
    // point moved (ucomiss jne/jp pairs on x vs prevX, then z vs prevZ).
    if (params.x != params.prevX || params.z != params.prevZ) {
        desc.directionAngle =
            std::atan2(params.z - params.prevZ, params.x - params.prevX) +
            1.5707963705062866f; // pi/2 @ 0x100a29bb4
    }

    // Component loop @ 0x100731e75..0x100732171 (binary stride 0x78 between
    // desc+0x28 and desc+0x30).
    for (TerrainBrushComponent& comp : desc.components) {
        ctx.target = &comp; // +0x20 @ 0x100731edd

        // Resource-load gate @ 0x100731ee1..0x100731f32: resourceId != 0 and
        // textureSet == 0 triggers a spResMan GUID load
        // {0x891B0E1A7AB50E44, resourceId}; on failure the whole function
        // returns. clean-room fallback (not authentic): spResMan is not
        // reconstructed; the load is skipped and the component applies with a
        // null texture set (plain sweep).
        const S3DTextureBufferInfo* texInfo = nullptr;
        if (comp.textureSet != nullptr) {
            texInfo = comp.textureSet->bufferInfo; // textureSet vtable+0x18 @ 0x100731f45/0x100731f48
        }

        // Angle selection @ 0x100731f57..0x100731f72 (cmov on comp+0x10).
        const Float32 angle = (comp.useDirectionAngle != 0) ? desc.directionAngle
                                                            : desc.rotationAccum;

        // Amount (ctx+0x18) @ 0x100731f7e..0x100731f8a:
        // strengths[idx] * params.strengthScale (+0x18). The binary does no
        // bounds check; the vector model guards.
        const Sint32 idx = ctx.strengthIndex;
        const bool idxOk = idx >= 0 && idx < static_cast<Sint32>(comp.strengths.size());
        ctx.amount = idxOk ? comp.strengths[static_cast<std::size_t>(idx)] * params.strengthScale
                           : 0.0f;
        // Size @ 0x100731f8f..0x100731f9f: radii[idx] * params.radiusScale (+0x1c).
        const bool radOk = idx >= 0 && idx < static_cast<Sint32>(comp.radii.size());
        const Float32 size = radOk ? comp.radii[static_cast<std::size_t>(idx)] * params.radiusScale
                                   : 0.0f;

        if (comp.effect == 1) {
            InitFloraBrushInfo(); // @ 0x100731fc1
        }
        if (comp.effect == 2) {
            // @ 0x100731fde: comiss amount, 1.0f (const @ 0x100a15090); when
            // above, the amount saturates to 1.0f (0x3f800000 @ 0x100731fe7).
            if (ctx.amount > 1.0f) {
                ctx.amount = 1.0f;
            }
        }

        // Effect dispatch @ 0x100731fee..0x100732133 (jump table @ 0x10073218c,
        // bound 0xa @ 0x100731ff1/0x100731ff5: above -> application skipped,
        // the post-op and out-rect union still run).
        switch (comp.effect) {
            case 0: case 2: case 3: case 4: case 5: case 7: case 8:
                ApplyBrushSweep(texInfo, gridX, gridY, size, angle,
                                MemberFnForEffect(comp.effect), ctx, rect);
                break;
            case 1: // Flora @ 0x100732017: applied only when this+0x150 != 0.
                if (mFloraBrushInfo != 0) {
                    ApplyBrushSweep(texInfo, gridX, gridY, size, angle,
                                    MemberFnForEffect(1), ctx, rect);
                }
                break;
            case 6: // ApplyWaterErosionBrush @ 0x100732068: strength cells =
                // (int)(size * 1/16) @ 0x100732068/0x10073206c.
                ApplyWaterErosionBrush(ctx, static_cast<Sint32>(size * kOneOverCellWidth), rect);
                break;
            case 9: { // Rain @ 0x10073209b
                const Sint32 vertex = GetVertIndex(gridX, gridY); // vtable+0x48 @ 0x1007320b1
                const TerrainBrushMemberFn rainFn =
                    SurfaceWaterRain(vertex, ctx, rect, ctx.amount); // cSTESurfaceWater::Rain
                if (rainFn.fn != nullptr || rainFn.isVirtual) {
                    ApplyBrushSweep(texInfo, gridX, gridY, size, angle, rainFn, ctx, rect);
                }
                break;
            }
            case 10: { // CreateSpring @ 0x100731ebe
                const Sint32 vertex = GetVertIndex(gridX, gridY); // vtable+0x48 @ 0x100731eca
                mSurfaceWater.CreateSpringAtVertex(vertex); // cSTESurfaceWater::CreateSpring @ 0x100730b16
                break;
            }
            default:
                break; // > 0xa: application skipped
        }

        // Post-op for every component @ 0x100732137..0x100732148 (the ctx
        // argument register is not even set by the caller; the callee ignores it).
        DoPostBrushApplicationOperation(comp.effect, ctx, rect);
        // Out-rect union @ 0x10073214d..0x10073215a:
        // params.rect = UnionRect(params.rect, rect) (SC4Rect<int>::UnionRect @ 0x100212e68).
        params.rect = UnionRectValues(params.rect, rect);
    }
}

TerrainBrushMemberFn cSTETerrain::MemberFnForEffect(Sint32 effect) {
    // Jump table @ 0x10073218c: each case loads the direct BrushFnX address
    // with adj = 0 (`xor eax, eax; lea BrushFnX, rbx` @ 0x10073200e..0x100732099).
    switch (effect) {
        case 0: return TerrainBrushMemberFn{&cSTETerrain::BrushFnRaiseLower, false, 0};
        case 1: return TerrainBrushMemberFn{&cSTETerrain::BrushFnFlora, false, 0};
        case 2: return TerrainBrushMemberFn{&cSTETerrain::BrushFnSmoothen, false, 0};
        case 3: return TerrainBrushMemberFn{&cSTETerrain::BrushFnPaint, false, 0};
        case 4: return TerrainBrushMemberFn{&cSTETerrain::BrushFnPlaceEffect, false, 0};
        case 5: return TerrainBrushMemberFn{&cSTETerrain::BrushFnLevel, false, 0};
        case 7: return TerrainBrushMemberFn{&cSTETerrain::BrushFnVolcanoAsh, false, 0};
        case 8: return TerrainBrushMemberFn{&cSTETerrain::BrushFnLakeBed, false, 0};
        default: return TerrainBrushMemberFn{nullptr, false, 0};
    }
}

bool cSTETerrain::CallBrushFn(const TerrainBrushMemberFn& fn, Sint32 vertex,
                              const TerrainBrushContext& ctx) {
    // Itanium member-fn dispatch @ 0x100732473..0x1007324b4 (texture) /
    // 0x100732910..0x100732932 (plain): fn word bit 0 set -> virtual call
    // vtable[fnWord-1] on the adjusted this; otherwise the direct call.
    if (fn.isVirtual) {
        // clean-room fallback (not authentic): the cSTETerrain vtable layout
        // is not reconstructed, so virtual brush-fn slots cannot be resolved.
        return false;
    }
    if (fn.fn == nullptr) {
        return false; // model guard (the binary would call through a null fn word)
    }
    cSTETerrain* adjusted =
        reinterpret_cast<cSTETerrain*>(reinterpret_cast<char*>(this) + fn.adj);
    return (adjusted->*(fn.fn))(vertex, ctx);
}

void cSTETerrain::ApplyBrushSweep(const S3DTextureBufferInfo* texInfo, Sint32 gridX,
                                  Sint32 gridY, Float32 size, Float32 angle,
                                  const TerrainBrushMemberFn& fn, TerrainBrushContext& ctx,
                                  SC4RectI& rect) {
    // The 0x1007320e1 block of ApplyBrushComponents: texture overload when a
    // cS3DTextureBufferInfo* is available (@ 0x100732105), plain overload with
    // repeat count 1 otherwise (@ 0x10073212e).
    if (texInfo != nullptr) {
        ApplyBrushAtLocation(texInfo, gridX, gridY, size, angle, fn, ctx, rect);
    } else {
        ApplyBrushAtLocation(gridX, gridY, size, angle, fn, 1u, ctx, rect);
    }
}

namespace {

// The sincosf stub @ 0x10092af3a resolves to ___sincosf_stret, returning
// {sin, cos} in xmm0 lanes 0/1 (lane order: __sinval first, per the macOS SDK
// math.h; the brush code reads the cos lane via movshdup, e.g. @ 0x10073258c).
inline void BrushSincos(Float32 angle, Float32& sinOut, Float32& cosOut) noexcept {
#if defined(__APPLE__)
    const struct __float2 r = __sincosf_stret(angle);
    sinOut = r.__sinval;
    cosOut = r.__cosval;
#else
    sinOut = std::sin(angle); // clean-room fallback (not authentic)
    cosOut = std::cos(angle); // clean-room fallback (not authentic)
#endif
}

} // namespace

void cSTETerrain::ComputeRectAroundPt(SC4PointF* pts, Float32 x, Float32 z, Float32 size,
                                      Float32 aspect, Float32 angle) {
    // Exact cSTETerrain::ComputeRectAroundPt @ 0x10073255c.
    Float32 sinA, cosA;
    BrushSincos(angle, sinA, cosA); // @ 0x10073257d (stub 0x10092af3a)
    // h = size * -0.5f (const @ 0x100a158b4, mulss @ 0x100732598/0x1007325a4),
    // v = aspect * -0.5f.
    const Float32 h = size * -0.5f;
    const Float32 v = aspect * -0.5f;
    // Corners @ 0x1007325a8..0x1007325d1: pts[1] = (+h, -v), pts[2] = (-h, -v),
    // pts[3] = (-h, +v) — all four (with pts[0] below) form the proper rotated
    // rectangle for every aspect ratio. (0x1007325cc stores xmm2 = -h into
    // pts[3].x; an earlier workpack transcription read xmm3 there.)
    pts[1].x = h;   // [rbx+0x08] @ 0x1007325a8
    pts[1].z = -v;  // [rbx+0x0c] @ 0x1007325ba (sign mask @ 0x100a16ac0)
    pts[2].x = -h;  // [rbx+0x10] @ 0x1007325c2
    pts[2].z = -v;  // [rbx+0x14] @ 0x1007325c7
    pts[3].x = -h;  // [rbx+0x18] @ 0x1007325cc
    pts[3].z = v;   // [rbx+0x1c] @ 0x1007325d1
    // pts[0] = center + R(angle)*(h, v) @ 0x1007325d6..0x100732600:
    //   x' = cos*h - sin*v ; z' = sin*h + cos*v.
    pts[0].x = (cosA * h - sinA * v) + x;
    pts[0].z = (h * sinA + v * cosA) + z;
    // pts[1..3] = center + R(angle)*corner @ 0x100732605..0x10073264c:
    //   x' = cos*px - sin*pz ; z' = sin*px + cos*pz.
    for (int i = 1; i <= 3; ++i) {
        const Float32 px = pts[i].x;
        const Float32 pz = pts[i].z;
        pts[i].x = (cosA * px - sinA * pz) + x;
        pts[i].z = (sinA * px + cosA * pz) + z;
    }
}

void cSTETerrain::ComputeSpansForRectangle(const SC4PointF* pts, Sint32* spanMin,
                                           Sint32* spanMax, Sint32& minRow,
                                           Sint32& maxRow) const {
    // Exact cSTETerrain::ComputeSpansForRectangle @ 0x100732656.
    const Float32 kOneOverCellWidth = 0.0625f; // 0x100a158b0
    const Float32 kCellWidth = 16.0f;          // 0x100a16904
    minRow = 0x7fffffff;                       // [r8] init @ 0x10073265b
    maxRow = static_cast<Sint32>(0x80000000);  // [r9] init @ 0x100732662
    for (int i = 0; i < 4; ++i) {              // point loop @ 0x100732681..0x1007326a9
        const Sint32 row = static_cast<Sint32>(pts[i].z * kOneOverCellWidth);
        if (maxRow < row) maxRow = row; // [r9] update @ 0x10073268f..0x100732694
        if (minRow > row) minRow = row; // [r8] update @ 0x100732697..0x10073269f
    }
    if (minRow < 0) minRow = 0;                      // @ 0x1007326ab..0x1007326b7
    if (maxRow > mCellCountZ) maxRow = mCellCountZ;  // @ 0x1007326ba..0x1007326ca
    if (minRow > maxRow) return;                     // @ 0x1007326cc/0x1007326cf
    for (Sint32 row = minRow; row <= maxRow; ++row) { // row loop @ 0x1007326e3..0x1007327d4
        spanMin[row] = 0x7fffffff;                     // @ 0x1007326e3
        spanMax[row] = static_cast<Sint32>(0x80000000); // @ 0x1007326eb
        const Float32 rowZ = static_cast<Float32>(row) * kCellWidth; // @ 0x1007326f6/0x1007326fb
        for (int e = 1; e <= 4; ++e) {                 // edge loop @ 0x10073270a..0x10073279c
            const SC4PointF& p0 = pts[e - 1];
            const SC4PointF& p1 = pts[(e == 4) ? 0 : e]; // cmove rbx, 0 @ 0x100732717
            const Float32 z0 = p0.z, z1 = p1.z;
            // Crossing test @ 0x10073271b..0x100732733: crossing iff
            // (rowZ >= z0 && z1 > rowZ) || (rowZ < z0 && rowZ >= z1).
            bool crossing;
            if (rowZ >= z0) {
                crossing = (z1 > rowZ);
            } else {
                crossing = (rowZ >= z1);
            }
            if (!crossing) continue;
            Float32 crossX = p0.x;                     // equal-x fast path @ 0x100732773
            if (p0.x != p1.x) {                         // ucomiss jne @ 0x100732743
                // Interpolation @ 0x100732747..0x10073276f, quantized through
                // int (cvttss2si/cvtsi2ss pair @ 0x100732768..0x10073276f):
                // x = p0.x + (rowZ - z0) * (p1.x - p0.x)/(z1 - z0).
                const Float32 xi =
                    p0.x + (rowZ - z0) * ((p1.x - p0.x) / (z1 - z0));
                crossX = static_cast<Float32>(static_cast<Sint32>(xi));
            }
            const Sint32 col = static_cast<Sint32>(crossX * kOneOverCellWidth); // @ 0x100732773/0x100732777
            if (spanMin[row] > col) spanMin[row] = col; // @ 0x10073277b..0x100732781
            if (spanMax[row] < col) spanMax[row] = col; // @ 0x100732789..0x10073278e
        }
        if (spanMin[row] < 0) spanMin[row] = 0;                      // @ 0x1007327a2..0x1007327a9
        spanMax[row] = spanMax[row] + 1;                             // @ 0x1007327b5..0x1007327b8
        if (spanMax[row] > mCellCountX) spanMax[row] = mCellCountX;  // cmova @ 0x1007327bc..0x1007327c6
    }
}

void cSTETerrain::ApplyBrushAtLocation(const S3DTextureBufferInfo* texInfo, Sint32 gridX,
                                       Sint32 gridY, Float32 size, Float32 angle,
                                       const TerrainBrushMemberFn& fn,
                                       TerrainBrushContext& ctx, SC4RectI& rect) {
    // Exact cSTETerrain::ApplyBrushAtLocation @ 0x1007321b8 (texture-aware).
    if (texInfo == nullptr) {
        return; // model guard (the binary assumes a non-null rsi)
    }
    const Sint32 width = texInfo->width;   // +0x08 @ 0x100732203
    const Sint32 height = texInfo->height; // +0x0c @ 0x100732207
    const Float32 fwidth = static_cast<Float32>(width);   // @ 0x10073220b
    const Float32 fheight = static_cast<Float32>(height); // @ 0x100732217
    const Float32 worldPerTexel = size / fwidth;          // [rbp-0x878] @ 0x10073222c
    Float32 sinA, cosA;
    BrushSincos(angle, sinA, cosA);                      // @ 0x10073223b (stub 0x10092af3a)
    const Uint8* texData = texInfo->data;                // [texInfo+0x20]->[0] @ 0x100732247
    const Sint32 stride = mVertexCountX;                 // +0x50 reads @ 0x100732425
    // Rotated footprint @ [rbp-0x870] @ 0x10073228e..0x1007322a8; the aspect is
    // (size/width)*height (xmm3 = (size/width) * fheight).
    SC4PointF pts[5];
    ComputeRectAroundPt(pts, static_cast<Float32>(gridX) * 16.0f,
                        static_cast<Float32>(gridY) * 16.0f, size,
                        worldPerTexel * fheight, angle);
    // Span arrays ([rbp-0x440] / [rbp-0x850] in the binary) and row range.
    const std::size_t spanCount =
        static_cast<std::size_t>(mVertexCountZ > 0 ? mVertexCountZ : 1);
    std::vector<Sint32> spanMin(spanCount, 0);
    std::vector<Sint32> spanMax(spanCount, 0);
    Sint32 minRow = 0, maxRow = 0;
    ComputeSpansForRectangle(pts, spanMin.data(), spanMax.data(), minRow, maxRow);
    rect = kEmptyRect; // @ 0x1007322e1 (constant-pool copy @ 0x100a1c0c0)
    if (minRow > maxRow) {
        return; // @ 0x1007322fc
    }
    const Float32 halfW = fwidth * 0.5f;   // [rbp-0x87c] @ 0x100732317..0x10073232b (0.5f @ 0x100a158bc)
    const Float32 halfH = fheight * 0.5f;  // [rbp-0x874] @ 0x100732333..0x10073233f
    const Float32 texelsPerCell = 16.0f / worldPerTexel; // [rbp-0x894] @ 0x100732347
    const Float32 cosStep = cosA * texelsPerCell;        // [rbp-0x910] @ 0x10073235e..0x10073238b
    const Float32 sinStep = sinA * texelsPerCell;        // [rbp-0x900] @ 0x100732365..0x100732392
    for (Sint32 row = minRow; row <= maxRow; ++row) {    // row loop @ 0x100732399..0x10073252f
        const Sint32 colLo = spanMin[static_cast<std::size_t>(row)];
        const Sint32 colHi = spanMax[static_cast<std::size_t>(row)];
        if (colLo > colHi) {
            continue; // @ 0x1007323a9
        }
        // First-pixel texture coordinates @ 0x1007323b2..0x100732416:
        //   texRow = cos*dyT - sin*dxT + halfH ; texCol = cos*dxT + sin*dyT + halfW
        //   with dxT = (colLo - gridX)*texelsPerCell, dyT = (row - gridY)*texelsPerCell.
        const Float32 dxT0 = static_cast<Float32>(colLo - gridX) * texelsPerCell;
        const Float32 dyT0 = static_cast<Float32>(row - gridY) * texelsPerCell;
        Float32 texRow = (cosA * dyT0 - sinA * dxT0) + halfH;
        Float32 texCol = (cosA * dxT0 + sinA * dyT0) + halfW;
        // Column loop @ 0x100732433..0x10073251e: the body runs for every col
        // in [spanMin, spanMax] inclusive (spanMax already carries the +1
        // from ComputeSpansForRectangle), and the texture coordinates advance
        // incrementally per column @ 0x100732513/0x100732517
        // (texCol += cosStep ; texRow -= sinStep).
        for (Sint32 col = colLo; col <= colHi; ++col) {
            const Sint32 texR = static_cast<Sint32>(texRow); // cvttss2si rax, xmm7 @ 0x100732433
            const Sint32 texC = static_cast<Sint32>(texCol); // cvttss2si rcx, xmm6 @ 0x100732444
            if (static_cast<Uint32>(height) > static_cast<Uint32>(texR) && // @ 0x100732438
                static_cast<Uint32>(width) > static_cast<Uint32>(texC) && // @ 0x100732449
                texData != nullptr) {
                // ctx.flag (+0x14) = texture byte @ 0x100732489/0x10073248c
                // (pixel = texRow*width + texCol).
                ctx.flag = texData[static_cast<std::size_t>(texR) *
                                          static_cast<std::size_t>(width) +
                                  static_cast<std::size_t>(texC)];
                const Sint32 vertex = row * stride + col; // @ 0x100732462/0x100732469
                if (CallBrushFn(fn, vertex, ctx)) {       // @ 0x1007324aa..0x1007324b4
                    if (rect.left > col) rect.left = col;      // @ 0x1007324d8
                    if (rect.right < col) rect.right = col;    // @ 0x1007324e4
                    if (rect.top > row) rect.top = row;        // @ 0x1007324ee
                    if (rect.bottom < row) rect.bottom = row; // @ 0x1007324fb
                }
            }
            texCol = texCol + cosStep; // addss xmm6, xmm4 @ 0x100732513
            texRow = texRow - sinStep; // subss xmm7, xmm5 @ 0x100732517
        }
    }
}

void cSTETerrain::ApplyBrushAtLocation(Sint32 gridX, Sint32 gridY, Float32 size,
                                        Float32 angle, const TerrainBrushMemberFn& fn,
                                        Uint32 repeat, TerrainBrushContext& ctx,
                                        SC4RectI& rect) {
    // Exact cSTETerrain::ApplyBrushAtLocation @ 0x1007327de (plain overload).
    // Footprint: aspect = size (square) — the call passes xmm2 = xmm3 = size
    // @ 0x100732842.
    SC4PointF pts[5];
    ComputeRectAroundPt(pts, static_cast<Float32>(gridX) * 16.0f,
                        static_cast<Float32>(gridY) * 16.0f, size, size, angle);
    const std::size_t spanCount =
        static_cast<std::size_t>(mVertexCountZ > 0 ? mVertexCountZ : 1);
    std::vector<Sint32> spanMin(spanCount, 0);
    std::vector<Sint32> spanMax(spanCount, 0);
    Sint32 minRow = 0, maxRow = 0;
    ComputeSpansForRectangle(pts, spanMin.data(), spanMax.data(), minRow, maxRow);
    rect = kEmptyRect; // @ 0x10073287b (constant-pool copy @ 0x100a1c0c0)
    if (repeat == 0) {
        return; // test r14d, r14d @ 0x10073288d — the whole sweep is skipped
    }
    const Sint32 stride = mVertexCountX; // +0x50 @ 0x100732904
    // Repeat loop @ 0x1007328c2..0x100732994: the full row/col sweep runs
    // `repeat` times ([rbp-0x880] pass counter vs the uint arg).
    for (Uint32 pass = 0; pass < repeat; ++pass) {
        if (minRow > maxRow) {
            continue; // @ 0x1007328c8..0x1007328d5
        }
        for (Sint32 row = minRow; row <= maxRow; ++row) { // @ 0x1007328e2..0x100732980
            const Sint32 colLo = spanMin[static_cast<std::size_t>(row)];
            const Sint32 colHi = spanMax[static_cast<std::size_t>(row)];
            if (colLo > colHi) {
                continue; // @ 0x1007328f8
            }
            // Body runs for every col in [spanMin, spanMax] inclusive
            // (spanMax carries the +1 from ComputeSpansForRectangle).
            for (Sint32 col = colLo; col <= colHi; ++col) { // @ 0x10073290c..0x100732972
                const Sint32 vertex = row * stride + col;  // @ 0x10073290c
                if (CallBrushFn(fn, vertex, ctx)) {        // @ 0x100732910..0x100732934
                    if (rect.left > col) rect.left = col;      // @ 0x100732938
                    if (rect.right < col) rect.right = col;    // @ 0x100732940
                    if (rect.top > row) rect.top = row;        // @ 0x10073294a
                    if (rect.bottom < row) rect.bottom = row;  // @ 0x100732957
                }
            }
        }
    }
}

void cSTETerrain::PadRectangle(SC4RectI& rect, Sint32 pad) {
    // Exact cSTETerrain::PadRectangle @ 0x100733596.
    rect.top -= pad;    // [rsi+4] @ 0x1007335a0
    rect.left -= pad;   // [rsi]   @ 0x1007335a5
    rect.right += pad;  // [rsi+8] @ 0x1007335ab
    rect.bottom += pad; // [rsi+0xc] @ 0x1007335b3
    if (rect.top < 0) rect.top = 0;     // test eax @ 0x1007335b9
    if (rect.left < 0) rect.left = 0;   // test r8d @ 0x1007335c4
    if (rect.right > mCellCountX) rect.right = mCellCountX;   // @ 0x1007335cf
    if (rect.bottom > mCellCountZ) rect.bottom = mCellCountZ; // @ 0x1007335d9
}

Float32 cSTETerrain::BrushRadius(Uint32 brushID) const {
    // Exact cSTETerrain::BrushRadius @ 0x100734d6e. Default 32.0f
    // @ 0x100a18764.
    auto& map = BrushDescriptionMap();
    const auto it = map.find(brushID); // tree walk @ 0x100734d8d..0x100734db3
    if (it == map.end()) {
        return 32.0f;
    }
    const Sint32 idx = mBrushStrengthIndex; // this+0xdc @ 0x100734db5
    if (idx < 0 || idx >= static_cast<Sint32>(it->second.radii.size())) {
        return 32.0f; // bounds guard (not in the binary; the modeled vector would otherwise UB)
    }
    return it->second.radii[static_cast<std::size_t>(idx)]; // [desc+0x10 array][idx] @ 0x100734dbc
}

void cSTETerrain::BeginBrushScrub(Uint32 brushID, Uint32 scrubID) {
    // Exact cSTETerrain::BeginBrushScrub @ 0x100734a7e.
    // Decal map (std::map<Uint32, tDecalMapEntry> tree at this+0x268/+0x270,
    // size at +0x278): when non-empty the tree is destroyed and reset
    // @ 0x100734a8e..0x100734ac2.
    if (!mDecalMap.empty()) {
        mDecalMap.clear();
    }
    // Brush lookup @ 0x100734ac9..0x100734b04 (lower_bound descent, cmovae
    // pairs, key <= brushID check). Found-block @ 0x100734b06..0x100734b45:
    // the SL::spSSE session (@ 0x100b212a0) is begun (vt+0x8 @ 0x100734b1b),
    // and the scrub-loop sound Sint64(0xea451903, desc+0x7c, desc+0x0, 0, 0)
    // is posted only when desc+0x7c != 0 (movl 0xa4(%rbx) / testl / je
    // 0x100734b3f @ 0x100734b1e..0x100734b26); the session then ends (vt+0x10
    // @ 0x100734b45). clean-room fallback (not authentic): spSSE is not
    // reconstructed — the sound path is a skipped no-op; only the desc+0x7c
    // gate is reproduced structurally.
    const auto it = BrushDescriptionMap().find(brushID);
    if (it != BrushDescriptionMap().end() && it->second.scrubSoundId != 0) {
        // (scrub-loop sound post @ 0x100734b3c — clean-room fallback: skipped)
    }
    // Tail call vtable+0x2b8 @ 0x100734b58. Control-flow note, verified against
    // the shipped binary: EVERY exit path converges on the shared epilogue at
    // 0x100734b48 — the empty-map je @ 0x100734ad3, the not-found je
    // @ 0x100734afe, the key-mismatch ja @ 0x100734b04, the spSSE-null je
    // @ 0x100734b13, and the found-block fall-through — and that epilogue
    // loads the vtable (movq (%r14), %rax @ 0x100734b48) solely to execute
    // jmpq *0x2b8(%rax). There is no ret path: the delegate call is
    // UNCONDITIONAL in the binary, so the model fires it from every path.
    BeginBrushScrubDelegate(brushID, scrubID);
}

void cSTETerrain::DoPostBrushApplicationOperation(Sint32 effect, TerrainBrushContext& ctx,
                                                  SC4RectI& rect) {
    // Exact cSTETerrain::DoPostBrushApplicationOperation @ 0x10073340e.
    // Branch mapping: effect 7 -> AddVolcanicAshRegion (je 0x10073342a ->
    // tail jmp @ 0x10073342e); effect 2 -> PadRectangle(rect, 1)
    // @ 0x10073341c..0x100733425); everything else returns without side
    // effects. The ctx argument is unused by the callee, and the caller
    // @ 0x100732148 never sets its register (rdx).
    (void)ctx;
    if (effect == 7) {
        AddVolcanicAshRegion(rect); // tail jmp @ 0x10073342e
        return;
    }
    if (effect == 2) {
        PadRectangle(rect, 1); // @ 0x10073341c..0x100733425
    }
}

void cSTETerrain::AddVolcanicAshRegion(SC4RectI& rect) {
    // Exact cSTETerrain::AddVolcanicAshRegion @ 0x100731628.
    if (rect.left > rect.right) {
        return; // @ 0x10073163e
    }
    if (rect.top > rect.bottom) {
        return; // @ 0x10073164a
    }
    SC4RectI merged = rect; // local copy @ 0x100731653..0x100731662
    // Merge-overlap loop @ 0x1007316ba..0x1007316e1 (list at this+0x230, node
    // rect at +0x10; on merge the node is unlinked, deleted, and the size at
    // this+0x240 is decremented @ 0x100731695..0x1007316b5).
    for (auto it = mVolcanicAshRegions.begin(); it != mVolcanicAshRegions.end();) {
        const SC4RectI& r = it->rect;
        if (r.right >= merged.left &&  // node.right >= new.left @ 0x1007316ba (jl skip)
            r.left <= merged.right &&  // node.left <= new.right @ 0x1007316c6 (jg skip)
            r.bottom >= merged.top &&  // node.bottom >= new.top @ 0x1007316cd (jl skip)
            r.top <= merged.bottom) { // node.top <= new.bottom @ 0x1007316d5 (jle merge)
            merged = UnionRectValues(merged, r); // UnionRect @ 0x100731690 (this = merged local)
            it = mVolcanicAshRegions.erase(it);
        } else {
            ++it;
        }
    }
    // Push-front a new 0x28-byte node {rect at +0x10, zero qword at +0x20}
    // @ 0x1007316e6..0x10073171b; list size increments @ 0x10073171b.
    mVolcanicAshRegions.push_front(VolcanicAshRegion{merged, 0});
}

bool cSTETerrain::GetTextureValue(Sint32 x, Sint32 y, TerrainBrushContext& ctx) {
    // Exact cSTETerrain::GetTextureValue @ 0x100733436.
    const TerrainBrushComponent* comp = ctx.target;
    if (comp == nullptr || comp->textureSet == nullptr || ctx.params == nullptr ||
        ctx.desc == nullptr) {
        return false; // model guard (the binary would fault on a null texture set)
    }
    const S3DTextureBufferInfo* texInfo =
        comp->textureSet->bufferInfo; // textureSet vtable+0x18 @ 0x100733454/0x10073345b
    if (texInfo == nullptr) {
        return false; // model guard
    }
    const Sint32 idx = ctx.strengthIndex; // @ 0x100733461
    if (idx < 0 || idx >= static_cast<Sint32>(comp->radii.size())) {
        return false; // bounds guard (not in the binary)
    }
    const Float32 size =
        comp->radii[static_cast<std::size_t>(idx)] * ctx.params->radiusScale; // @ 0x100733471/0x100733476
    const Float32 angle = (comp->useDirectionAngle != 0) // cmov @ 0x10073348c/0x100733490
                              ? ctx.desc->directionAngle
                              : ctx.desc->rotationAccum;
    const Sint32 width = texInfo->width;   // @ 0x100733498
    const Sint32 height = texInfo->height; // @ 0x10073349c
    const Float32 fwidth = static_cast<Float32>(width);
    const Float32 worldPerTexel = size / fwidth;         // @ 0x1007334a9
    const Float32 halfW = fwidth * 0.5f;                 // 0.5f @ 0x100a158bc
    const Float32 texelsPerCell = 16.0f / worldPerTexel; // @ 0x1007334ba/0x1007334c2
    const Sint32 cx = static_cast<Sint32>(ctx.x * 0.0625f); // @ 0x1007334ce/0x1007334d6
    const Sint32 cz = static_cast<Sint32>(ctx.z * 0.0625f); // @ 0x1007334da/0x1007334df
    const Float32 dxT = static_cast<Float32>(x - cx) * texelsPerCell; // @ 0x1007334e3..0x1007334f2
    const Float32 dyT = static_cast<Float32>(y - cz) * texelsPerCell; // @ 0x1007334f7..0x100733506
    Float32 sinA, cosA;
    BrushSincos(angle, sinA, cosA); // @ 0x10073350b (stub 0x10092af3a)
    const Float32 texXf = cosA * dxT + sinA * dyT + halfW; // @ 0x10073351e..0x100733530
    const Sint32 texX = static_cast<Sint32>(texXf);        // @ 0x100733535
    if (static_cast<Uint32>(width) <= static_cast<Uint32>(texX)) { // @ 0x10073353c
        return false;
    }
    const Float32 halfH = static_cast<Float32>(height) * 0.5f;      // @ 0x10073354d
    const Float32 texYf = cosA * dyT - sinA * dxT + halfH;          // @ 0x100733555..0x100733561
    const Sint32 texY = static_cast<Sint32>(texYf);                // @ 0x100733565
    if (static_cast<Uint32>(height) <= static_cast<Uint32>(texY)) { // @ 0x10073356a
        return false;
    }
    const Uint8* data = texInfo->data; // [texInfo+0x20]->[0] @ 0x10073356e
    if (data == nullptr) {
        return false; // model guard
    }
    // pixel = texY*width + texX @ 0x100733575..0x10073357e; the byte lands
    // in ctx.flag (+0x14) @ 0x100733581.
    ctx.flag = data[static_cast<std::size_t>(texY) * static_cast<std::size_t>(width) +
                    static_cast<std::size_t>(texX)];
    return true; // @ 0x100733584
}

// ---------------------------------------------------------------------------
// BrushFn* effects (exact bodies)
// ---------------------------------------------------------------------------

bool cSTETerrain::BrushFnLevel(Sint32 vertex, const TerrainBrushContext& ctx) noexcept {
    // Exact cSTETerrain::BrushFnLevel @ 0x100733230.
    //   flag gate @ 0x100733234 (0 -> false);
    //   rate = flag * amount * 0.00390625f (const @ 0x100a1b65c) @ 0x10073323b..0x100733247;
    //   diff = altitude[vertex] - ctx.targetHeight (+0x1c) @ 0x100733262/0x100733267.
    //   diff > 0: rate >= diff -> snap via vtable+0x188 @ 0x10073329e, else
    //   LowerVertex @ 0x100733278. diff == 0 -> return false @ 0x10073327f..0x10073329a.
    //   diff < 0: diff >= -rate -> snap @ 0x100733284..0x100733291, else
    //   RaiseVertex @ 0x100733293. Returns true whenever the flag is set and
    //   diff != 0 (movb $1, al @ 0x1007332aa).
    const Uint8 flag = ctx.flag;
    if (flag == 0) {
        return false;
    }
    if (vertex < 0 || static_cast<Uint32>(vertex) >= static_cast<Uint32>(mVertexCount)) {
        return false; // model guard
    }
    const Float32 rate = static_cast<Float32>(flag) * ctx.amount * 0.00390625f;
    const Float32 diff = mAltitude[static_cast<std::size_t>(vertex)] - ctx.targetHeight;
    if (diff > 0.0f) {
        if (rate >= diff) {
            SetVertexAltitude(vertex, ctx.targetHeight); // vtable+0x188 @ 0x10073329e
        } else {
            LowerVertex(static_cast<Uint32>(vertex), rate); // @ 0x100733278
        }
    } else if (diff < 0.0f) {
        if (diff >= -rate) {
            SetVertexAltitude(vertex, ctx.targetHeight); // vtable+0x188 @ 0x10073329e
        } else {
            RaiseVertex(static_cast<Uint32>(vertex), rate); // @ 0x100733293
        }
    } else {
        return false; // diff == 0 @ 0x10073329a
    }
    return true; // @ 0x1007332aa
}

bool cSTETerrain::BrushFnLakeBed(Sint32 vertex, const TerrainBrushContext& ctx) noexcept {
    // Exact cSTETerrain::BrushFnLakeBed @ 0x100733360.
    //   delta = (float)(ctx.flag - component flag48) * raw strengths[ctx.strengthIndex]
    //   (no params scale) @ 0x100733364..0x10073337f; delta == 0 -> false
    //   @ 0x100733384..0x10073338c (ucomiss jne/jnp pair).
    //   target = ctx.targetHeight (+0x1c) + delta @ 0x10073338e..0x100733393;
    //   rate = |delta| * 0.0625f @ 0x100733397..0x10073339f (abs mask).
    //   diff = target - altitude[vertex] @ 0x100733b4..0x100733bc.
    //   diff > 0: raising only allowed when the vertex is NOT submerged —
    //   depth <= 0 (jae @ 0x100733cc); a submerged vertex returns false
    //   @ 0x1007333d2..0x1007333ed. rate <= diff -> RaiseVertex @ 0x100733404,
    //   else snap via vtable+0x188 @ 0x1007333f6.
    //   diff < 0: rate > -diff -> snap, else LowerVertex @ 0x1007333e6.
    //   diff == 0 -> false @ 0x1007333d5.
    const TerrainBrushComponent* comp = ctx.target;
    if (comp == nullptr) {
        return false; // model guard
    }
    const Sint32 idx = ctx.strengthIndex;
    if (idx < 0 || idx >= static_cast<Sint32>(comp->strengths.size())) {
        return false; // bounds guard (not in the binary)
    }
    if (vertex < 0 || static_cast<Uint32>(vertex) >= static_cast<Uint32>(mVertexCount)) {
        return false; // model guard
    }
    const Sint32 modeDelta = static_cast<Sint32>(ctx.flag) - static_cast<Sint32>(comp->flag48);
    const Float32 delta =
        static_cast<Float32>(modeDelta) * comp->strengths[static_cast<std::size_t>(idx)];
    if (delta == 0.0f) {
        return false; // @ 0x10073338c path
    }
    const Float32 target = ctx.targetHeight + delta;
    const Float32 rate = std::fabs(delta) * 0.0625f;
    const Float32 diff = target - mAltitude[static_cast<std::size_t>(vertex)];
    if (diff > 0.0f) {
        const Float32 depth = mSurfaceWater.depth[static_cast<std::size_t>(vertex)];
        if (depth > 0.0f) {
            return false; // submerged: raising refused @ 0x1007333d2..0x1007333ed
        }
        if (rate <= diff) {
            RaiseVertex(static_cast<Uint32>(vertex), rate); // @ 0x100733404
        } else {
            SetVertexAltitude(vertex, target); // vtable+0x188 @ 0x1007333f6
        }
    } else if (diff < 0.0f) {
        if (rate > -diff) {
            SetVertexAltitude(vertex, target); // vtable+0x188 @ 0x1007333f6
        } else {
            LowerVertex(static_cast<Uint32>(vertex), rate); // @ 0x1007333e6
        }
    } else {
        return false; // diff == 0 @ 0x1007333ed
    }
    return true; // movb $1, al @ 0x100733409
}

bool cSTETerrain::BrushFnVolcanoAsh(Sint32 vertex, const TerrainBrushContext& ctx) noexcept {
    // Exact cSTETerrain::BrushFnVolcanoAsh @ 0x100733338: flag gate
    // @ 0x10073333c, then `orw $0x400` into the vertex-record flags word
    // (+0x22 of the 36-byte record) @ 0x100733350.
    if (ctx.flag == 0) {
        return false;
    }
    if (vertex < 0 || static_cast<Uint32>(vertex) >= static_cast<Uint32>(mVertexCount)) {
        return false;
    }
    mVertexFlags[static_cast<std::size_t>(vertex)] |= 0x400;
    return true;
}

namespace {

// cRZRandom::RandomDoubleRangeUniform(double, double) @ 0x10003d3e4 — body
// not reconstructed in this ticket. clean-room fallback (not authentic):
// modeled as lo + (hi - lo) * RandomDoubleUniform(), preserving the
// one-draw-per-call RNG consumption order the brush effects depend on.
inline Float64 RandomDoubleRangeUniform(RZRandomState& rng, Float64 lo, Float64 hi) noexcept {
    return lo + (hi - lo) * RandomDoubleUniform(rng);
}

// Direction-index -> vertex-index offset for the cSTETerrain neighbor table
// at this+0xa4 (consumed by BrushFnSmoothen @ 0x100732c7d,
// GetNeighborWithLowestWaterLevel @ 0x100733c03/0x100733c66, DoWaterErosion
// @ 0x100733862). Bit order proven by the GetNeighborWithLowestWaterLevel
// edge masks @ 0x100737b64..0x100737b88: 0=W(-1,0), 1=NW(-1,-1), 2=N(0,-1),
// 3=NE(1,-1), 4=E(1,0), 5=SE(1,1), 6=S(0,1), 7=SW(-1,1).
inline Sint32 NeighborVertexOffset(Sint32 dir, Sint32 stride) noexcept {
    switch (dir) {
        case 0: return -1;
        case 1: return -stride - 1;
        case 2: return -stride;
        case 3: return -stride + 1;
        case 4: return 1;
        case 5: return stride + 1;
        case 6: return stride;
        case 7: return stride - 1;
        default: return 0;
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Brush description property loading (M3 terrain data-loading wave 1)
// ---------------------------------------------------------------------------

// cSCPropertyHelp::GetPropertyValue overloads over cISCPropertyHolder —
// reconstructed surface: read the property or leave the out arg untouched and
// return false (the binary's contract every caller below depends on).
// clean-room fallback (not authentic): fixture backing store is a
// type-tagged map, not the real exemplar property tree.

bool BrushPropertyHolderFixture::GetPropertyValue(Uint32 propertyId, Float32& out) const {
    // clean-room fallback (not authentic): kind must match exactly.
    const auto it = properties.find(propertyId);
    if (it == properties.end() || it->second.kind != SCExemplarPropertyValue::Kind::Float) {
        return false;
    }
    out = it->second.floatValue;
    return true;
}

bool BrushPropertyHolderFixture::GetPropertyValue(Uint32 propertyId, std::vector<Float32>& out) const {
    // clean-room fallback (not authentic): kind must match exactly.
    const auto it = properties.find(propertyId);
    if (it == properties.end() || it->second.kind != SCExemplarPropertyValue::Kind::FloatArray) {
        return false;
    }
    out = it->second.floatArray;
    return true;
}

bool BrushPropertyHolderFixture::GetPropertyValue(Uint32 propertyId, Uint32& out) const {
    // clean-room fallback (not authentic): kind must match exactly.
    const auto it = properties.find(propertyId);
    if (it == properties.end() || it->second.kind != SCExemplarPropertyValue::Kind::Uint) {
        return false;
    }
    out = it->second.uintValue;
    return true;
}

bool BrushPropertyHolderFixture::GetPropertyValue(Uint32 propertyId, std::vector<Uint32>& out) const {
    // clean-room fallback (not authentic): kind must match exactly.
    const auto it = properties.find(propertyId);
    if (it == properties.end() || it->second.kind != SCExemplarPropertyValue::Kind::UintArray) {
        return false;
    }
    out = it->second.uintArray;
    return true;
}

bool BrushPropertyHolderFixture::GetPropertyValue(Uint32 propertyId, Uint8& out) const {
    // clean-room fallback (not authentic): kind must match exactly.
    const auto it = properties.find(propertyId);
    if (it == properties.end() || it->second.kind != SCExemplarPropertyValue::Kind::Byte) {
        return false;
    }
    out = it->second.byteValue;
    return true;
}

bool BrushPropertyHolderFixture::GetPropertyValue(Uint32 propertyId, bool& out) const {
    // clean-room fallback (not authentic): kind must match exactly.
    const auto it = properties.find(propertyId);
    if (it == properties.end() || it->second.kind != SCExemplarPropertyValue::Kind::Bool) {
        return false;
    }
    out = it->second.boolValue;
    return true;
}

void BrushPropertyHolderFixture::SetFloat(Uint32 propertyId, Float32 v) {
    SCExemplarPropertyValue p;
    p.kind = SCExemplarPropertyValue::Kind::Float;
    p.floatValue = v;
    properties[propertyId] = p;
}

void BrushPropertyHolderFixture::SetFloatArray(Uint32 propertyId, std::vector<Float32> v) {
    SCExemplarPropertyValue p;
    p.kind = SCExemplarPropertyValue::Kind::FloatArray;
    p.floatArray = std::move(v);
    properties[propertyId] = p;
}

void BrushPropertyHolderFixture::SetUint(Uint32 propertyId, Uint32 v) {
    SCExemplarPropertyValue p;
    p.kind = SCExemplarPropertyValue::Kind::Uint;
    p.uintValue = v;
    properties[propertyId] = p;
}

void BrushPropertyHolderFixture::SetUintArray(Uint32 propertyId, std::vector<Uint32> v) {
    SCExemplarPropertyValue p;
    p.kind = SCExemplarPropertyValue::Kind::UintArray;
    p.uintArray = std::move(v);
    properties[propertyId] = p;
}

void BrushPropertyHolderFixture::SetByte(Uint32 propertyId, Uint8 v) {
    SCExemplarPropertyValue p;
    p.kind = SCExemplarPropertyValue::Kind::Byte;
    p.byteValue = v;
    properties[propertyId] = p;
}

void BrushPropertyHolderFixture::SetBool(Uint32 propertyId, bool v) {
    SCExemplarPropertyValue p;
    p.kind = SCExemplarPropertyValue::Kind::Bool;
    p.boolValue = v;
    properties[propertyId] = p;
}

bool BrushPropertyHolderFixture::HasProperty(Uint32 propertyId) const {
    // holder vt+0x28 GetProperty(id) surface (ReadTuningParameters @
    // 0x10073ddb1): non-null iff a value is stored under the id; the null
    // gate is je @ 0x10073ddb7. clean-room fallback (not authentic).
    return properties.find(propertyId) != properties.end();
}

Sint32 cSTETerrain::GetCostPerBrushApplication(Uint32 brushID) const {
    // Exact cSTETerrain::GetCostPerBrushApplication @ 0x100738d76.
    // Tree find by brushID @ 0x100738d8d..0x100738db3 (lower_bound descent,
    // cmovae pairs, key <= brushID check); missing -> eax = 0x20 (32)
    // @ 0x100738db5.
    auto& map = BrushDescriptionMap();
    const auto it = map.find(brushID);
    if (it == map.end()) {
        return 32; // default @ 0x100738db5
    }
    // cvttss2si [node+0x68+4*idx] @ 0x100738dbc..0x100738dc3: node+0x68 is
    // value+0x40 = costs[], idx = this+0xdc (movsxd @ 0x100738dbc),
    // truncate-toward-zero float -> int. NO bounds check on idx in the binary.
    const Sint32 idx = mBrushStrengthIndex; // this+0xdc
    if (idx < 0 || idx > 4) {
        return 32; // bounds guard (not in the binary; the modeled array would otherwise UB)
    }
    return static_cast<Sint32>(it->second.costs[static_cast<std::size_t>(idx)]);
}

bool cSTETerrain::GetEffectIDProperty(const BrushPropertyHolderFixture& holder,
                                      Uint32 propertyId, Uint32* out) {
    // Exact cSTETerrain::GetEffectIDProperty @ 0x100738dcc.
    // vector<unsigned> read into a fresh temp @ 0x100738dea
    // (cSCPropertyHelp::GetPropertyValue(vector<uint>&)); on read failure the
    // caller's out is left untouched and false returned (je @ 0x100738df1).
    std::vector<Uint32> values;
    if (!holder.GetPropertyValue(propertyId, values)) {
        return false;
    }
    // Exact-8-byte gate: end - begin == 8 (cmp rax, 8 / jne @ 0x100738e03..06)
    // — the vector must hold EXACTLY 2 uints; any other size fails with out
    // untouched (callers pre-fill defaults).
    if (values.size() != 2) {
        return false;
    }
    // out[0]/out[1] dword copies @ 0x100738e08..0x100738e12.
    out[0] = values[0];
    out[1] = values[1];
    return true;
}

bool cSTETerrain::ReadBrushWholeProperties(const BrushPropertyHolderFixture& holder,
                                           TerrainBrushDescription& desc) {
    // Exact cSTETerrain::ReadBrushWholeProperties @ 0x10073b26a.
    // Property 0x8a020102 (rotationStep, float) @ 0x10073b281.
    Float32 step = 0.0f;
    if (holder.GetPropertyValue(0x8a020102u, step)) {
        // jitter byte @ +0x04 = (step != 0 || isnan(step))
        // (setp/setne/or @ 0x10073b297..0x10073b2a2).
        desc.useJitter =
            (step != 0.0f || std::isnan(step)) ? 1u : 0u;
        // ucomiss on the PRE-scale value @ 0x10073b2a5 sets the flags the
        // jne/jp pair @ 0x10073b2b5..0x10073b2b7 consumes (the mulss in
        // between does not touch them).
        const bool preScaleNonZero = (step != 0.0f) || std::isnan(step);
        // deg -> rad: step *= 0.01745329238474369f @ 0x10073b2a8
        // (constant @ 0x100a18740), stored @ 0x10073b2b0.
        step *= 0.01745329238474369f;
        desc.rotationStep = step;
        if (preScaleNonZero) {
            // RNG path @ 0x10073b2c6..0x10073b2dd: rotationAccum = (Float32)
            // RandomDoubleRangeUniform(sRand, 0.0, 2*pi f64 @ 0x100a184e8).
            desc.rotationAccum = static_cast<Float32>(
                RandomDoubleRangeUniform(sRand, 0.0, 6.2831854820251465));
        } else {
            // Pre-scale step == 0.0f exactly: rotationAccum = 0.0f with NO
            // RNG call (fall-through to 0x10073b2e1; xmm0 still zeroed).
            desc.rotationAccum = 0.0f;
        }
    } else {
        // Missing property @ 0x10073b2bb: jitter = 1, step = bits 0x3c0efa35
        // (0.5 deg in rad) — then the RNG path ALWAYS runs (fall-through into
        // 0x10073b2c6).
        desc.useJitter = 1;                              // @ 0x10073b2bb
        desc.rotationStep = std::bit_cast<Float32>(0x3c0efa35u); // @ 0x10073b2bf
        desc.rotationAccum = static_cast<Float32>(
            RandomDoubleRangeUniform(sRand, 0.0, 6.2831854820251465));
    }
    desc.directionAngle = 0.0f; // +0x88 store @ 0x10073b2e9
    // Property 0x8a020101 (costs, vector<float>) @ 0x10073b30d.
    std::vector<Float32> costs;
    if (holder.GetPropertyValue(0x8a020101u, costs)) {
        // 5 raw dword copies [data + 4*j] -> desc+0x40+4*j
        // @ 0x10073b316..0x10073b32a — NO size gate on the read vector.
        for (Uint32 j = 0; j < 5; ++j) {
            // model guard (not in the binary): a short vector would read
            // past the buffer; the fixture surfaces 0.0f instead.
            desc.costs[j] = (j < costs.size()) ? costs[j] : 0.0f;
        }
    } else {
        // Failure -> all five costs zeroed (qword +0x48, qword +0x40,
        // dword +0x50 stores @ 0x10073b32e..0x10073b338).
        for (Uint32 j = 0; j < 5; ++j) {
            desc.costs[j] = 0.0f;
        }
    }
    // Property 0x8a020103 -> +0x7c scrub sound id @ 0x10073b34e; fail -> 0.
    Uint32 scrubSoundId = 0;
    if (!holder.GetPropertyValue(0x8a020103u, scrubSoundId)) {
        scrubSoundId = 0; // @ 0x10073b357
    }
    desc.scrubSoundId = static_cast<Sint32>(scrubSoundId);
    // Property 0x8a020104 -> +0x80 end-scrub sound id @ 0x10073b36d; fail -> 0.
    Uint32 endScrubSoundId = 0;
    if (!holder.GetPropertyValue(0x8a020104u, endScrubSoundId)) {
        endScrubSoundId = 0; // @ 0x10073b376
    }
    desc.endScrubSoundId = static_cast<Sint32>(endScrubSoundId);
    return true; // mov al, 1 @ 0x10073b38e
}

bool cSTETerrain::ReadBrushUIProperties(const BrushPropertyHolderFixture& holder,
                                        TerrainBrushDescription& desc) {
    // Exact cSTETerrain::ReadBrushUIProperties @ 0x10073b3b8.
    // Four GetEffectIDProperty pairs at +0x54/+0x5c/+0x64/+0x6c
    // @ 0x10073b3c8..0x10073b44d; on failure the 8-byte movabs default is
    // stored (low dword 0xea3169e6, high dword the pair index).
    if (!GetEffectIDProperty(holder, 0xaa2ed800u, &desc.uiEffectPairs[0])) {
        desc.uiEffectPairs[0] = 0xea3169e6u; // movabs 0x2ea3169e6 @ 0x10073b3da
        desc.uiEffectPairs[1] = 2u;
    }
    if (!GetEffectIDProperty(holder, 0xaa2ed801u, &desc.uiEffectPairs[2])) {
        desc.uiEffectPairs[2] = 0xea3169e6u; // movabs 0x1ea3169e6 @ 0x10073b3fd
        desc.uiEffectPairs[3] = 1u;
    }
    if (!GetEffectIDProperty(holder, 0xaa2ed802u, &desc.uiEffectPairs[4])) {
        desc.uiEffectPairs[4] = 0xea3169e6u; // movabs 0x4ea3169e6 @ 0x10073b420
        desc.uiEffectPairs[5] = 4u;
    }
    if (!GetEffectIDProperty(holder, 0xaa2ed803u, &desc.uiEffectPairs[6])) {
        desc.uiEffectPairs[6] = 0xea3169e6u; // movabs 0x3ea3169e6 @ 0x10073b443
        desc.uiEffectPairs[7] = 3u;
    }
    // 0xaa2ed804 -> +0x74 @ 0x10073b460; fail -> 0x071e5700 @ 0x10073b469.
    if (!holder.GetPropertyValue(0xaa2ed804u, desc.uiSoundIdA)) {
        desc.uiSoundIdA = 0x071e5700u;
    }
    // 0xaa2ed805 -> +0x78 @ 0x10073b47f; fail -> 0x071e5705 @ 0x10073b488.
    if (!holder.GetPropertyValue(0xaa2ed805u, desc.uiSoundIdB)) {
        desc.uiSoundIdB = 0x071e5705u;
    }
    return true; // mov al, 1 @ 0x10073b48e
}

bool cSTETerrain::ReadBrushComponentProperties(const BrushPropertyHolderFixture& holder,
                                               TerrainBrushDescription& desc) {
    // Exact cSTETerrain::ReadBrushComponentProperties @ 0x10073aafe.
    // 4-iteration loop (r14 = i, jb @ 0x10073adac); property family
    // 0x8a0202XX + i*0x100 (r13 = i << 8 @ 0x10073ab34).
    Uint32 successCount = 0; // [rbp-0x34] @ 0x10073ab2a
    for (Uint32 i = 0; i < 4; ++i) {
        const Uint32 family = 0x8a020200u + (i << 8);
        // Per-iteration component stack image is zeroed at the loop head:
        // textureSet @ 0x10073ab38, the radii/strengths/field60 temp vectors
        // @ 0x10073ab3f..0x10073ab69.
        // ---- 0x...05 effect (Uint32) @ 0x10073ab6c..0x10073ab8e ----
        Uint32 effect = 0;
        if (!holder.GetPropertyValue(family + 0x05u, effect)) {
            continue; // skip this component only (r15b = 4 path @ 0x10073ad88)
        }
        ++successCount; // inc [rbp-0x34] right after the effect read @ 0x10073ab8e
        if (effect > 0xbu) {
            return false; // ja @ 0x10073ab98 — abort ALL, no rollback
        }
        // ---- 0x...02 radii (vector<float>) @ 0x10073ab9e — required ----
        std::vector<Float32> radii;
        if (!holder.GetPropertyValue(family + 0x02u, radii)) {
            return false; // je @ 0x10073abb7 — abort ALL (radii is required)
        }
        // Elementwise MAX into desc.radii[0..4] @ 0x10073abbf..0x10073abe1:
        // ucomiss read vs desc; jbe skips the store — only read > desc wins
        // (NaN read entries lose too, unordered sets CF|ZF). NO size gate —
        // 5 entries are read unconditionally (model guard surfaces 0.0f for a
        // short vector; the binary reads past the buffer).
        for (Uint32 j = 0; j < 5; ++j) {
            const Float32 r = (j < radii.size()) ? radii[j] : 0.0f;
            // model guard (not in the binary): ReadBrushDescription resizes
            // desc.radii to 5 before calling in; the guard only protects
            // direct fixture calls against an unsized radii vector.
            if (j < desc.radii.size() && r > desc.radii[j]) {
                desc.radii[j] = r; // movss store @ 0x10073abd5
            }
        }
        // ---- 0x...03 strengths (vector<float>) @ 0x10073abe3 ----
        std::vector<Float32> strengths;
        if (!holder.GetPropertyValue(family + 0x03u, strengths)) {
            continue; // je @ 0x10073abfc — skip this component only
        }
        // ---- 0x...01 useDirectionAngle (bool) @ 0x10073ac02..0x10073ac20 ----
        // The byte slot is zeroed PER ITERATION @ 0x10073ac09, then read as
        // bool; the read result is IGNORED (no test al) — on failure the
        // freshly-zeroed 0 persists. The pushed component copies the raw
        // byte @ 0x10073adc9.
        bool useDirectionAngleBool = false;
        (void)holder.GetPropertyValue(family + 0x01u, useDirectionAngleBool);
        const Uint8 useDirectionAngle = useDirectionAngleBool ? 1u : 0u;

        // Per-effect fills. Effects 6 and 10 force resourceId=0 and
        // textureSet=0 @ 0x10073ac33..0x10073ac3d and skip BOTH the 0x...00
        // resourceId read and the 0x...04 flag48 read.
        Uint32 resourceId = 0;      // [rbp-0xb8] (+0x00)
        Uint8 flag48 = 0;           // [rbp-0x70] (+0x48)
        Uint32 decalResourceId = 0; // [rbp-0x68] (+0x50)
        Float32 jitterAmp = 0.0f;   // [rbp-0x64] (+0x54)
        Uint32 maxDecalCount = 0;   // [rbp-0x6c] (+0x4c)
        Uint32 effectParam58 = 0, effectParam5c = 0; // [rbp-0x60] (+0x58/+0x5c)
        constexpr Float32 kTwoPi = 6.2831854820251465f; // 0x100a171a4
        if (effect == 6u || effect == 10u) {
            resourceId = 0;      // @ 0x10073ac33
            flag48 = 0;          // @ 0x10073ac58
            decalResourceId = 0; // @ 0x10073ac5c
            jitterAmp = kTwoPi;  // @ 0x10073ac63..0x10073ac6b
        } else {
            // ---- 0x...00 resourceId (Uint32) @ 0x10073ac97 — required ----
            if (!holder.GetPropertyValue(family + 0x00u, resourceId)) {
                return false; // jmp @ 0x10073acb2 — abort ALL
            }
            // (textureSet = 0 @ 0x10073ac3d — the loader never loads one)
            // ---- 0x...04 flag48 (byte) @ 0x10073ace8 — required ----
            if (!holder.GetPropertyValue(family + 0x04u, flag48)) {
                return false; // je @ 0x10073acfe — abort ALL
            }
            if (effect == 3u) {
                // ---- effect 3 (Paint): 0x...06 decalResourceId (required)
                // @ 0x10073ad0a ----
                if (!holder.GetPropertyValue(family + 0x06u, decalResourceId)) {
                    return false; // je @ 0x10073ad20 — abort ALL
                }
                // ---- 0x...07 jitterAmp (float, required) @ 0x10073ad22 ----
                if (!holder.GetPropertyValue(family + 0x07u, jitterAmp)) {
                    return false; // je @ 0x10073ad38 — abort ALL
                }
                // jitterAmp /= 360.0f @ 0x10073ad3f (0x100a27260; NOT /16).
                jitterAmp /= 360.0f;
            } else {
                decalResourceId = 0; // @ 0x10073ac5c
                jitterAmp = kTwoPi;  // @ 0x10073ac63..0x10073ac6b
            }
        }
        // ---- effects 3 and 4: 0x...08 maxDecalCount (required) ----
        if (effect == 3u || effect == 4u) {
            if (!holder.GetPropertyValue(family + 0x08u, maxDecalCount)) {
                return false; // jmp @ 0x10073ac92 — abort ALL
            }
        } else {
            maxDecalCount = 0; // mov [rbp-0x6c], 0 @ 0x10073acb7
        }
        // ---- effect 4: 0x...09 effect pair via GetEffectIDProperty ----
        if (effect == 4u) {
            // @ 0x10073acc8..0x10073ace3: the pair slot is NOT pre-filled on
            // this path; GetEffectIDProperty failure aborts ALL (je @
            // 0x10073ace1), so the untouched slot is never observed.
            Uint32 pair[2] = {0u, 0u};
            if (!GetEffectIDProperty(holder, family + 0x09u, pair)) {
                return false;
            }
            effectParam58 = pair[0]; // +0x58
            effectParam5c = pair[1]; // +0x5c
        } else {
            effectParam58 = 0; // mov qword [rbp-0x60], 0 @ 0x10073adb7
            effectParam5c = 0;
        }
        // ---- push the component (stack-image copy @ 0x10073adbf..0x10073ae42) ----
        TerrainBrushComponent comp;
        comp.resourceId = resourceId;                // +0x00 @ 0x10073ade5
        comp.effect = static_cast<Sint32>(effect);   // +0x04 ([rbp-0xb4])
        comp.textureSet = nullptr;                   // +0x08 — always 0
        comp.useDirectionAngle = useDirectionAngle;  // +0x10 @ 0x10073adc9
        comp.radii = radii;                          // +0x18 (copy @ 0x10073adef)
        comp.strengths = strengths;                  // +0x30 (copy @ 0x10073ae06)
        comp.flag48 = flag48;                        // +0x48
        comp.maxDecalCount = maxDecalCount;          // +0x4c
        comp.decalResourceId = decalResourceId;      // +0x50
        comp.jitterAmplitude = jitterAmp;            // +0x54
        comp.effectParam58 = static_cast<Sint32>(effectParam58);   // +0x58
        comp.effectParam5c = static_cast<Sint32>(effectParam5c);   // +0x5c
        comp.field60.clear(); // +0x60 — vestigial empty vector (copy @ 0x10073ae38)
        desc.components.push_back(std::move(comp)); // end += 0x78 @ 0x10073ae3d
        // Loop CONTINUES after a successful push (r15 = 0 -> join @ 0x10073ae57).
    }
    // Return gate @ 0x10073ae63: setg over the success counter — true iff at
    // least one component's effect property read succeeded.
    return successCount > 0;
}

void cSTETerrain::ReadBrushDescription(const SC4PersistResourceKey& key, void* /*userData*/) {
    // Exact cSTETerrain::ReadBrushDescription @ 0x10073aefe (the
    // resource-iterator apply-callback target). The userData argument is
    // never read (xor edx, edx @ 0x10073b513).
    auto& map = BrushDescriptionMap();
    // Skip-if-present: inlined __tree walk @ 0x10073af25..0x10073af55, key =
    // dword at resourceKey+8.
    if (map.find(key.id) != map.end()) {
        return; // jae -> exit @ 0x10073af55
    }
    // spResMan null is gated by the caller (ReadBrushDescriptions); the
    // fixture source carries its own guard. Exemplar load via spResMan
    // vt+0x18 with class id 0xa52160f5 @ 0x10073af77; fail -> exit.
    BrushResourceSourceModel* source = BrushResourceSource();
    if (source == nullptr) {
        return; // model guard (binary: caller-gated deref @ 0x10073af5b)
    }
    const BrushPropertyHolderFixture* holder = nullptr;
    if (!source->LoadExemplar(key, holder)) {
        return; // je @ 0x10073af7c
    }
    // Zero-insert: a zeroed 0x8c value is built @ 0x10073af82..0x10073afff
    // and inserted @ 0x10073b06d (__construct_node + __insert_node_at).
    TerrainBrushDescription& desc = map[key.id];
    // radii resize(5) + the redundant zero loop @ 0x10073b0f0..0x10073b110.
    desc.radii.assign(5, 0.0f);
    desc.id = static_cast<Sint32>(key.id); // value+0x00 mirror @ 0x10073b112
    // holder = loaded exemplar vt+0x30 @ 0x10073b0e6.
    ReadBrushWholeProperties(*holder, desc); // @ 0x10073b123
    if (ReadBrushComponentProperties(*holder, desc)) { // @ 0x10073b12e
        ReadBrushUIProperties(*holder, desc);           // @ 0x10073b13d
    } else {
        // Component failure: the fresh map entry is removed
        // (__erase_unique @ 0x10073b14e).
        map.erase(key.id);
    }
    // Holder release @ 0x10073b15a (loaded exemplar vt+0x10) — the fixture
    // holder is not refcounted; nothing to release.
}

void cSTETerrain::ReadBrushDescriptions() {
    // Exact cSTETerrain::ReadBrushDescriptions @ 0x10073b49c.
    // spResMan null gate: cmp qword [0x100b713c8], 0; je exit @ 0x10073b4a7.
    BrushResourceSourceModel* source = BrushResourceSource();
    if (source == nullptr) {
        return;
    }
    ReleaseBrushDescriptions(); // call @ 0x10073b4b1
    // The binary allocates a cSCResFilterFixedGroupAndTypeId(0x6534284a =
    // kTypeExemplar group, type id 0x6a01fc2a) filter @ 0x10073b4cb..0x10073b4d8
    // and runs the spResMan vt+0x88 filtered query @ 0x10073b4fb; both are
    // modeled by BrushResourceSourceModel::Query. clean-room fallback (not
    // authentic): the filter object itself is fixture-internal.
    BrushResourceIteratorModel* iterator = nullptr;
    const Sint32 status = source->Query(iterator);
    if (status != 0) { // test eax, eax; je skip @ 0x10073b501..0x10073b503
        // Apply-callback over the query matches @ 0x10073b50c..0x10073b515:
        // iterator vt+0x38 with (&ReadBrushDescription, 0).
        iterator->ForEach(&cSTETerrain::ReadBrushDescription, nullptr);
        iterator->Release(); // iterator vt+0x10 @ 0x10073b51f
    }
    // Filter release @ 0x10073b522..0x10073b528 — fixture-internal.
}

void cSTETerrain::ReleaseBrushDescriptions() {
    // Exact cSTETerrain::ReleaseBrushDescriptions @ 0x10073b548.
    // The binary walks the map's nodes destroying each value's component
    // vector (vt+0x10 on every component's textureSet pointer + the
    // radii/strengths vector frees, stride 0x78 @ 0x10073b566..0x10073b591),
    // then __tree::destroy and the map reset @ 0x10073b5ce..0x10073b5f1 —
    // which is exactly std::map destruction semantics. Modeled by clear().
    BrushDescriptionMap().clear();
}

BrushResourceSourceModel*& cSTETerrain::BrushResourceSource() {
    // cSTETerrain::spResMan @ 0x100b713c8 — not reconstructed. clean-room
    // fallback (not authentic): injectable fixture source modeling the
    // resource-manager surface the brush loader observes (null gate @
    // 0x10073b4a7 / 0x10073af5b, vt+0x18 exemplar load with class id
    // 0xa52160f5, vt+0x88 filtered query, iterator vt+0x38 apply-callback).
    static BrushResourceSourceModel* sSource = nullptr;
    return sSource;
}

// ---------------------------------------------------------------------------
// Terrain tuning parameter loading (M3 terrain data-loading wave 2)
// ---------------------------------------------------------------------------

// SL::spRM @ 0x100b21280 — not reconstructed. clean-room fallback (not
// authentic): injectable fixture source modeling the exemplar-load surface
// ReadTuningParameters observes (load with class id 0xa52160f5 @
// 0x10073d99f, holder query @ 0x10073d9b1, release @ 0x10073ddf7). The
// binary does NOT null-gate SL::spRM before dereferencing it (movq (%rdi)
// @ 0x10073d98d would fault on null); the model treats a null source as a
// load failure (clean-room procedural mode — no tuning data mounted).
BrushResourceSourceModel*& cSTETerrain::TuningResourceSource() {
    static BrushResourceSourceModel* sSource = nullptr;
    return sSource;
}

// ---- Tuning globals (settable statics with the shipped byte-exact
//      defaults; the loader overwrites them from the terrain tuning
//      exemplar — see ReadTuningParameters). Shipped values verified by
//      fresh byte dumps (file offset = vmaddr - 0x100000000). ----

Float32& cSTETerrain::SeaLevelTunable() {                        // 0x100b71d48
    static Float32 v = std::bit_cast<Float32>(0x00000001u);     // shipped bits
    return v;
}
Float32& cSTETerrain::DefaultTerrainHeightTunable() {            // 0x100b17644
    static Float32 v = 50.0f;                                    // 0x42480000
    return v;
}
std::vector<Float32>& cSTETerrain::MaxNormalTerrainHeightTunable() { // 0x100b71d30
    static std::vector<Float32> t; // ships empty
    return t;
}
std::vector<Float32>& cSTETerrain::MinRegularHtFactorTunable() {  // 0x100b71d50
    static std::vector<Float32> t; // ships empty
    return t;
}
std::vector<Float32>& cSTETerrain::MaxRegularHtFactorTunable() {  // 0x100b71d68
    static std::vector<Float32> t; // ships empty
    return t;
}
std::vector<Float32>& cSTETerrain::MinMountainHtFactorTunable() { // 0x100b71d80
    static std::vector<Float32> t; // ships empty
    return t;
}
std::vector<Float32>& cSTETerrain::MaxMountainHtFactorTunable() { // 0x100b71d98
    static std::vector<Float32> t; // ships empty
    return t;
}
Float32& cSTETerrain::ImageImportScaleFactorTunable() {           // 0x100b17648
    static Float32 v = 1.5f;                                      // 0x3fc00000
    return v;
}
Float32& cSTETerrain::WaterProximityRangeFactorTunable() {        // 0x100b71db4
    static Float32 v = 0.0f;                                      // 0x00000000
    return v;
}
std::vector<Float32>& cSTETerrain::SeaLevelChangeDueToWaterSliderTunable() { // 0x100b71dc8
    static std::vector<Float32> t; // ships empty
    return t;
}
Uint32& cSTETerrain::NumInitialErosionPassesTunable() {           // 0x100b71db8
    static Uint32 v = 1;
    return v;
}
Uint32& cSTETerrain::NumHydraulicIterationsPerInitialErosionPassTunable() { // 0x100b71dbc
    static Uint32 v = 334;
    return v;
}
Uint32& cSTETerrain::NumWindIterationsPerInitialErosionPassTunable() { // 0x100b71dc0
    static Uint32 v = 10;
    return v;
}
Uint32& cSTETerrain::MinCellsInUsefulFlatAreaTunable() {          // 0x100b17650
    static Uint32 v = 401;
    return v;
}
Uint32& cSTETerrain::AutoFlatAreaEdgeSmootheningRadiusTunable() { // 0x100b17664
    static Uint32 v = 4;
    return v;
}
Uint32& cSTETerrain::NumSmoothingIterationsAfterWaterErosionTunable() { // 0x100b1764c
    static Uint32 v = 1;
    return v;
}
Uint32& cSTETerrain::EdgeReconcilliationSmoothingIterationsTunable() { // 0x100b17694
    static Uint32 v = 5;
    return v;
}
Float32& cSTETerrain::MaxHtChangeToFlattenFactorTunable() {       // 0x100b17654
    static Float32 v = 0.1f;                                      // 0x3dcccccd
    return v;
}
Float32& cSTETerrain::MinTerraceAltitudeFactorTunable() {         // 0x100b17658
    static Float32 v = 0.1f;                                      // 0x3dcccccd
    return v;
}
Float32& cSTETerrain::MaxTerraceAltitudeFactorTunable() {         // 0x100b1765c
    static Float32 v = 0.6f;                                      // 0x3f19999a
    return v;
}
Float32& cSTETerrain::TerrainRaiseLowerAmountTunable() {          // 0x100b175e0
    static Float32 v = 20.0f;                                     // 0x41a00000
    return v;
}
Uint8& cSTETerrain::MaxBeachWidthTunable() {                      // 0x100b175e4
    static Uint8 v = 4;
    return v;
}
Float32& cSTETerrain::MaxBeachAltitudeTunable() {                 // 0x100b175e8
    static Float32 v = 7.0f;                                      // 0x40e00000
    return v;
}
Float32& cSTETerrain::MinWaterDepthForDeepSeaBedTextureTunable() { // 0x100b17634
    static Float32 v = 15.0f;
    return v;
}
Float32& cSTETerrain::MaxWaterDepthForDeepSeaBedTextureTunable() { // 0x100b17638
    static Float32 v = 30.0f;
    return v;
}
Uint8& cSTETerrain::AutoFlattenUponCreationTunable() {            // 0x100b71de0
    static Uint8 v = 0x8au; // shipped byte (bit-cast; an int-looking slot)
    return v;
}
Uint8& cSTETerrain::ShoreCliffsUponCreationTunable() {            // 0x100b17668
    static Uint8 v = 1;
    return v;
}
std::vector<Float32>& cSTETerrain::TypicalBrushAngleIncrementTunable() { // 0x100b71de8
    static std::vector<Float32> t; // ships empty
    return t;
}
Uint32& cSTETerrain::VolcanicAshLifeTimeTunable() {               // 0x100b17684
    static Uint32 v = 10000000;
    return v;
}
Uint32& cSTETerrain::VolcanicAshUpdateIntervalTunable() {         // 0x100b17688
    // Shipped __DATA bytes are 10000000 (pre-derivation; the loader
    // recomputes the slot from kVolcanicAshLifeTime, see ReadTuningParameters).
    static Uint32 v = 10000000;
    return v;
}
Float32& cSTETerrain::EdgeReconcilliationHtDifferenceThresholdTunable() { // 0x100b17698
    static Float32 v = 1.2f;                                      // 0x3f99999a
    return v;
}

Uint32& cSTETerrain::EdgeReconcilliationCurveLoadCount() {
    // clean-room fallback (not authentic): observation counter for the
    // property-0x48cd7c00 non-null gate @ 0x10073ddb7 in
    // ReadTuningParameters (the SetResponseCurveData callee @ 0x10073ddc3
    // is a TODO no-op boundary).
    static Uint32 count = 0;
    return count;
}

Uint32& cSTETerrain::DetermineTextureFamilyMembersCallCount() {
    // clean-room fallback (not authentic): observation counter for the
    // DetermineTextureFamilyMembersForGridCells call @ 0x10073d9cb in
    // ReadTuningParameters (the callee @ 0x100737430 is a TODO no-op
    // boundary; the settable TextureFamilyMemberForGrid() table is left
    // as-is).
    static Uint32 count = 0;
    return count;
}

void cSTETerrain::ReadTuningParameters() {
    // Exact cSTETerrain::ReadTuningParameters @ 0x10073d95e (STATIC,
    // RNG-free). Loads the terrain tuning exemplar and overwrites the
    // tuning globals in one fixed pass; a missing/mistyped property leaves
    // its global untouched (the cSCPropertyHelp::GetPropertyValue failure
    // contract, see BrushPropertyHolderFixture).
    //
    // Prologue @ 0x10073d96b..0x10073d9b4: the fixed resource key
    // {group qword 0x88CD66E96534284A, instance dword 1} is built at
    // [rbp-0x38] @ 0x10073d96b..0x10073d97c; SL::spRM is loaded @
    // 0x10073d98a and vt+0x18 is called with class id 0xa52160f5
    // (edx), r8 = r9 = 0 @ 0x10073d99f; on failure the whole function
    // exits (je @ 0x10073d9a4). clean-room fallback (not authentic):
    // SL::spRM @ 0x100b21280 is modeled by TuningResourceSource(); the
    // model resource key is the {group, dword} pair's dword (1). The
    // binary does NOT null-gate spRM (it dereferences unconditionally @
    // 0x10073d98d); the model treats a null source as the load-failure
    // exit — clean-room procedural mode with no tuning data mounted.
    BrushResourceSourceModel* source = TuningResourceSource();
    if (source == nullptr) {
        return; // load-failure exit @ 0x10073d9a4
    }
    const BrushPropertyHolderFixture* holder = nullptr;
    if (!source->LoadExemplar(SC4PersistResourceKey{1u}, holder)) {
        return; // load-failure exit @ 0x10073d9a4 (je)
    }
    // loaded vt+0x30 -> r15 = cISCPropertyHolder* for every query below
    // @ 0x10073d9b1.
    const BrushPropertyHolderFixture& h = *holder;

    // ---- sTextureRandomizationGridSize (vector<unsigned>) @ 0x10073d9b7
    // ..0x10073d9c6 (property 0x89a312c2). The binary passes the global
    // vector @ 0x100b713f0 as the out arg directly; the model stores
    // vector<Sint32> (same 32-bit bits), so the read goes through a
    // vector<Uint32> temp. On failure the table is left untouched.
    {
        std::vector<Uint32> gridSize;
        if (h.GetPropertyValue(0x89a312c2u, gridSize)) {
            auto& t = TextureRandomizationGridSize();
            t.clear();
            t.reserve(gridSize.size());
            for (Uint32 v : gridSize) t.push_back(static_cast<Sint32>(v));
        }
    }
    // cSTETerrain::DetermineTextureFamilyMembersForGridCells @ 0x100737430
    // — called UNCONDITIONALLY right after the grid-size read @
    // 0x10073d9cb. NOT reconstructed (TODO; it derives the per-grid-cell
    // family members from the just-loaded grid sizes). clean-room fallback
    // (not authentic): observation counter only; the settable
    // TextureFamilyMemberForGrid() table is left as-is.
    ++DetermineTextureFamilyMembersCallCount();

    // ---- kMaxNormalTerrainHeight (vector<float>, property 0x48cd7b20)
    // @ 0x10073d9d0..0x10073d9df. On failure the vector is untouched.
    (void)h.GetPropertyValue(0x48cd7b20u, MaxNormalTerrainHeightTunable());
    // ---- kMaxTerrainHeight (float&, 0x48cd7b26) @ 0x10073d9e4.
    (void)h.GetPropertyValue(0x48cd7b26u, MaxTerrainHeightTunable());
    // ---- kMinTerrainHeight (float&, 0x48cd7b27) @ 0x10073da07.
    (void)h.GetPropertyValue(0x48cd7b27u, MinTerrainHeightTunable());
    // ---- kSeaLevel (float&, 0x48cd7b21) @ 0x10073da1b — the STATIC
    // tuning global (not the per-terrain instance sea level).
    (void)h.GetPropertyValue(0x48cd7b21u, SeaLevelTunable());
    // Quirk (a) @ 0x10073da20..0x10073da31: if kMinTerrainHeight >
    // kSeaLevel -> kSeaLevel := kMinTerrainHeight. ucomiss compares
    // minHt vs seaLevel @ 0x10073da28 and jbe @ 0x10073da2f skips when
    // minHt <= seaLevel OR unordered (NaN seaLevel sets CF -> jbe taken)
    // — exactly C++ `minHt > seaLevel`.
    if (MinTerrainHeightTunable() > SeaLevelTunable()) {
        SeaLevelTunable() = MinTerrainHeightTunable(); // movss @ 0x10073da31
    }
    // ---- kDefaultTerrainHeight (float&, 0x48cd7b2a) @ 0x10073da39..0x10073da48.
    (void)h.GetPropertyValue(0x48cd7b2au, DefaultTerrainHeightTunable());
    // Quirk (b) @ 0x10073da4d..0x10073da5e: same overwrite pattern vs
    // kDefaultTerrainHeight (ucomiss @ 0x10073da55, jbe @ 0x10073da5c,
    // movss store @ 0x10073da5e).
    if (MinTerrainHeightTunable() > DefaultTerrainHeightTunable()) {
        DefaultTerrainHeightTunable() = MinTerrainHeightTunable(); // @ 0x10073da5e
    }
    // ---- kMinRegularHtFactor (vector<float>, 0x48cd7b22) @ 0x10073da66.
    (void)h.GetPropertyValue(0x48cd7b22u, MinRegularHtFactorTunable());
    // ---- kMaxRegularHtFactor (vector<float>, 0x48cd7b23) @ 0x10073da7a.
    (void)h.GetPropertyValue(0x48cd7b23u, MaxRegularHtFactorTunable());
    // ---- kMinMountainHtFactor (vector<float>, 0x48cd7b24) @ 0x10073da8e.
    (void)h.GetPropertyValue(0x48cd7b24u, MinMountainHtFactorTunable());
    // ---- kMaxMountainHtFactor (vector<float>, 0x48cd7b25) @ 0x10073daa2.
    (void)h.GetPropertyValue(0x48cd7b25u, MaxMountainHtFactorTunable());
    // ---- kImageImportScaleFactor (float&, 0x48cd7b28) @ 0x10073dab6.
    (void)h.GetPropertyValue(0x48cd7b28u, ImageImportScaleFactorTunable());
    // ---- kTemperatureRangeFactor (float&, 0x48cd7b30) @ 0x10073daca.
    (void)h.GetPropertyValue(0x48cd7b30u, TemperatureRangeFactor());
    // ---- kWaterProximityRangeFactor (float&, 0x48cd7b31) @ 0x10073dade.
    (void)h.GetPropertyValue(0x48cd7b31u, WaterProximityRangeFactorTunable());
    // ---- kNumInitialErosionPasses (uint&, 0x48cd7b40) @ 0x10073daf2.
    (void)h.GetPropertyValue(0x48cd7b40u, NumInitialErosionPassesTunable());
    // ---- kNumHydraulicIterationsPerInitialErosionPass (uint&, 0x48cd7b41)
    // @ 0x10073db06.
    (void)h.GetPropertyValue(0x48cd7b41u,
                             NumHydraulicIterationsPerInitialErosionPassTunable());
    // ---- kNumWindIterationsPerInitialErosionPass (uint&, 0x48cd7b42)
    // @ 0x10073db1a.
    (void)h.GetPropertyValue(0x48cd7b42u,
                             NumWindIterationsPerInitialErosionPassTunable());
    // ---- kSeaLevelChangeDueToWaterSlider (vector<float>, 0x48cd7b43)
    // @ 0x10073db2e.
    (void)h.GetPropertyValue(0x48cd7b43u, SeaLevelChangeDueToWaterSliderTunable());
    // ---- kMinCellsInUsefulFlatArea (uint&, 0x48cd7b50) @ 0x10073db42.
    (void)h.GetPropertyValue(0x48cd7b50u, MinCellsInUsefulFlatAreaTunable());
    // ---- kMaxHtChangeToFlattenFactor (float&, 0x48cd7b51) @ 0x10073db56.
    (void)h.GetPropertyValue(0x48cd7b51u, MaxHtChangeToFlattenFactorTunable());
    // ---- kMaxTerraceAltitudeFactor (float&, 0x48cd7b52) @ 0x10073db6a.
    (void)h.GetPropertyValue(0x48cd7b52u, MaxTerraceAltitudeFactorTunable());
    // ---- kMinTerraceAltitudeFactor (float&, 0x48cd7b53) @ 0x10073db7e.
    (void)h.GetPropertyValue(0x48cd7b53u, MinTerraceAltitudeFactorTunable());
    // ---- kAutoFlatAreaEdgeSmootheningRadius (uint&, 0x48cd7b54)
    // @ 0x10073db92.
    (void)h.GetPropertyValue(0x48cd7b54u,
                             AutoFlatAreaEdgeSmootheningRadiusTunable());
    // ---- kMinWaterDepthForDeepSeaBedTexture (float&, 0x69ac5640)
    // @ 0x10073dba6.
    (void)h.GetPropertyValue(0x69ac5640u, MinWaterDepthForDeepSeaBedTextureTunable());
    // ---- kMaxWaterDepthForDeepSeaBedTexture (float&, 0x69ac5641)
    // @ 0x10073dbba.
    (void)h.GetPropertyValue(0x69ac5641u, MaxWaterDepthForDeepSeaBedTextureTunable());
    // ---- kMinShoreCliffSize (uint&, 0x48cd7b60) @ 0x10073dbce — the
    // binary passes the int global @ 0x100b1766c as an unsigned& out arg;
    // the model reads a Uint32 temp and bit-stores on success only.
    {
        Uint32 v = 0;
        if (h.GetPropertyValue(0x48cd7b60u, v)) {
            MinShoreCliffSizeTunable() = static_cast<Sint32>(v);
        }
    }
    // ---- kMaxShoreCliffSize (uint&, 0x48cd7b64) @ 0x10073dbe2.
    {
        Uint32 v = 0;
        if (h.GetPropertyValue(0x48cd7b64u, v)) {
            MaxShoreCliffSizeTunable() = static_cast<Sint32>(v);
        }
    }
    // ---- kMaxNormalYForCliff (float&, 0x48cd7b65) @ 0x10073dbf6.
    (void)h.GetPropertyValue(0x48cd7b65u, MaxNormalYForCliffTunable());
    // ---- kMinShoreCliffCandidateHtSum (float&, 0x48cd7b61) @ 0x10073dc0a.
    (void)h.GetPropertyValue(0x48cd7b61u, MinShoreCliffCandidateHtSumTunable());
    // Quirk (c) @ 0x10073dc1e..0x10073dc2e: kMinShoreCliffCandidateHtSum
    // *= 6.0f UNCONDITIONALLY (mulss against the constant with bits
    // 0x40c00000 @ 0x100a18ed0, load @ 0x10073dc1e, store @ 0x10073dc2e)
    // — it fires even when the property read failed (shipped 15.0f ->
    // 90.0f) and propagates NaNs.
    MinShoreCliffCandidateHtSumTunable() =
        MinShoreCliffCandidateHtSumTunable() * 6.0f;
    // ---- kMinShoreCliffHt (float&, 0x48cd7b62) @ 0x10073dc36.
    (void)h.GetPropertyValue(0x48cd7b62u, MinShoreCliffHtTunable());
    // ---- kMaxShoreCliffHt (float&, 0x48cd7b63) @ 0x10073dc4a.
    (void)h.GetPropertyValue(0x48cd7b63u, MaxShoreCliffHtTunable());
    // ---- kMinSeaLevel (float&, 0x48cd7b70) @ 0x10073dc5e.
    (void)h.GetPropertyValue(0x48cd7b70u, MinSeaLevelTunable());
    // ---- kMaxSeaLevel (float&, 0x48cd7b71) @ 0x10073dc72.
    (void)h.GetPropertyValue(0x48cd7b71u, MaxSeaLevelTunable());
    // ---- kSeaRaiseLowerAmount (float&, 0x48cd7b72) @ 0x10073dc86.
    (void)h.GetPropertyValue(0x48cd7b72u, SeaRaiseLowerAmountTunable());
    // ---- kTerrainRaiseLowerAmount (float&, 0x48cd7b73) @ 0x10073dc9a.
    (void)h.GetPropertyValue(0x48cd7b73u, TerrainRaiseLowerAmountTunable());
    // ---- kMaxBeachWidth (byte&, 0xa9c283d0) @ 0x10073dcae.
    (void)h.GetPropertyValue(0xa9c283d0u, MaxBeachWidthTunable());
    // ---- kMaxBeachAltitude (float&, 0xa9c283d1) @ 0x10073dcc2.
    (void)h.GetPropertyValue(0xa9c283d1u, MaxBeachAltitudeTunable());
    // ---- kbAutoFlattenUponCreation (byte&, 0x48cd7b80) @ 0x10073dcd6
    // ..0x10073dcef and kbShoreCliffsUponCreation (byte&, 0x48cd7b81)
    // @ 0x10073dcf6..0x10073dd09 ----
    // BINARY ODDITY: both reads target the SAME stack byte [rbp-0x19]
    // (r14 = rbp-0x19 @ 0x10073dcd6), which is NEVER initialized — on read
    // failure the setne store (@ 0x10073dcef / @ 0x10073dd09) folds
    // whatever the stack held, and the SECOND read inherits the FIRST
    // read's leftover byte when its own read fails. Deterministically
    // modeled as a pre-zeroed shared slot (clean-room fallback (not
    // authentic): failure -> 0), preserving the shared-slot carryover.
    // The setne stores ALWAYS fire: the globals are normalized to 0/1.
    {
        Uint8 flagSlot = 0; // shared [rbp-0x19] model, pre-zeroed
        (void)h.GetPropertyValue(0x48cd7b80u, flagSlot);
        AutoFlattenUponCreationTunable() = (flagSlot != 0) ? 1u : 0u; // setne @ 0x10073dcef
        // Same slot, NOT re-zeroed between the reads.
        (void)h.GetPropertyValue(0x48cd7b81u, flagSlot);
        ShoreCliffsUponCreationTunable() = (flagSlot != 0) ? 1u : 0u; // setne @ 0x10073dd09
    }
    // ---- kTypicalBrushAngleIncrement (vector<float>, 0x48cd7ba0)
    // @ 0x10073dd10..0x10073dd1f.
    (void)h.GetPropertyValue(0x48cd7ba0u, TypicalBrushAngleIncrementTunable());
    // Quirk (d) @ 0x10073dd24..0x10073dd4f: FIVE entries are scaled by
    // f64 pi/180 (bits 0x3f91df46aaaaaaab @ 0x100a211d0, movsd @
    // 0x10073dd24) via cvtss2sd / mulsd / cvtsd2ss / movss @
    // 0x10073dd36..0x10073dd43. The loop (rbx = 0..4, cmp $0x5 @
    // 0x10073dd4b) walks the vector's data pointer (movq @ 0x10073dd2c)
    // with NO size check — a short vector reads past its buffer in the
    // binary. model guard (not in the binary): entries beyond the vector
    // are skipped.
    {
        auto& angles = TypicalBrushAngleIncrementTunable();
        constexpr Float64 kPiOver180 =
            std::bit_cast<Float64>(0x3f91df46aaaaaaabULL); // f64 pi/180 @ 0x100a211d0
        for (std::size_t j = 0; j < 5; ++j) {
            if (j < angles.size()) {
                angles[j] = static_cast<Float32>(
                    static_cast<Float64>(angles[j]) * kPiOver180);
            }
        }
    }
    // ---- kVolcanicAshLifeTime (uint&, 0x89861864) @ 0x10073dd51..0x10073dd60.
    {
        Uint32 v = 0;
        if (h.GetPropertyValue(0x89861864u, v)) {
            VolcanicAshLifeTimeTunable() = v;
        }
    }
    // Quirk (e) @ 0x10073dd65..0x10073dd78: kVolcanicAshUpdateInterval =
    // kVolcanicAshLifeTime * 0xCCCCCCCD >> 36 (= lifeTime / 20, computed
    // with the magic-number reciprocal: movl 0xcccccccd @ 0x10073dd6b,
    // imulq @ 0x10073dd70, shrq $0x24 @ 0x10073dd74, dword store @
    // 0x10073dd78). Fires unconditionally off the (possibly untouched)
    // lifeTime value.
    VolcanicAshUpdateIntervalTunable() = static_cast<Uint32>(
        (static_cast<Uint64>(VolcanicAshLifeTimeTunable()) * 0xCCCCCCCDuLL) >> 36);
    // ---- kNumFlowStepsPerWaterErosionBrushApplication (vector<unsigned>,
    // 0x48cd7bf1) @ 0x10073dd7e..0x10073dd8d. The binary passes the global
    // vector @ 0x100b71e00 directly; the model table is vector<Sint32>
    // (FlowStepsPerErosionApplication), so the read goes through a
    // vector<Uint32> temp.
    {
        std::vector<Uint32> steps;
        if (h.GetPropertyValue(0x48cd7bf1u, steps)) {
            auto& t = FlowStepsPerErosionApplication();
            t.clear();
            t.reserve(steps.size());
            for (Uint32 v : steps) t.push_back(static_cast<Sint32>(v));
        }
    }
    // ---- kNumSmoothingIterationsAfterWaterErosion (uint&, 0x48cd7bf3)
    // @ 0x10073dd92.
    (void)h.GetPropertyValue(0x48cd7bf3u,
                             NumSmoothingIterationsAfterWaterErosionTunable());
    // ---- property 0x48cd7c00 -> holder vt+0x28 GetProperty @
    // 0x10073ddb1; a NON-NULL property feeds
    // cSC4ResponseCurve::SetResponseCurveData(
    // &kEdgeReconcilliationSmoothingRadiusVsHeightDiff, prop) @
    // 0x10073ddb9..0x10073ddc3 (null gate je @ 0x10073ddb7). The callee
    // is NOT reconstructed. clean-room fallback (not authentic): the
    // curve object @ 0x100b71e30 is a TODO no-op; the non-null gate is
    // observable via the counter.
    if (h.HasProperty(0x48cd7c00u)) { // test %rax,%rax; je @ 0x10073ddb7
        ++EdgeReconcilliationCurveLoadCount();
        // TODO(0x10073ddc3): cSC4ResponseCurve::SetResponseCurveData —
        // clean-room no-op boundary.
    }
    // ---- kEdgeReconcilliationSmoothingIterations (uint&, 0x48cd7c01)
    // @ 0x10073ddc8.
    (void)h.GetPropertyValue(0x48cd7c01u,
                             EdgeReconcilliationSmoothingIterationsTunable());
    // ---- kEdgeReconcilliationHtDifferenceThreshold (float&, 0x48cd7c02)
    // @ 0x10073dddc.
    (void)h.GetPropertyValue(0x48cd7c02u,
                             EdgeReconcilliationHtDifferenceThresholdTunable());
    // Holder release @ 0x10073ddf0..0x10073ddf7 (loaded vt+0x10) — the
    // fixture holder is not refcounted; nothing to release.
}

bool cSTETerrain::BrushFnFlora(Sint32 vertex, const TerrainBrushContext& ctx) noexcept {
    // Exact cSTETerrain::BrushFnFlora @ 0x100732a0e.
    // Gate 1 @ 0x100732a2e: ctx.flag >= 10 (unsigned byte compare, jb exit).
    if (ctx.flag < 10) {
        return false;
    }
    // Gate 2 @ 0x100732a38..0x100732a4c: RandomUint32Uniform(100) must roll
    // <= 30 (31/100 placement chance).
    if (RandomUint32Uniform(sRand, 100u) > 30u) {
        return false;
    }
    // Gate 3 @ 0x100732a52..0x100732a99: a submerged vertex (water depth > 0
    // at the +0x38 -> +0x28 record) bails after an spSSE "cannot place"
    // sound (vt+0x50 with (1, 0xAA62C9D5, 0, 0, 0) @ 0x100732a81..0x100732a96).
    if (vertex < 0 || static_cast<std::size_t>(vertex) >= mSurfaceWater.depth.size()) {
        return false; // model guard (the binary derefs the record directly)
    }
    const std::size_t v = static_cast<std::size_t>(vertex);
    if (mSurfaceWater.depth[v] > 0.0f) {
        // clean-room fallback (not authentic): spSSE is not reconstructed —
        // the sound post is a skipped no-op.
        return false;
    }
    // Gate 4 @ 0x100732a9e..0x100732ab4: cell bounds (unsigned) against
    // cellCountX (+0x40) and cellCountZ (+0x44) after the div by the row
    // stride (+0x50).
    const Sint32 stride = mVertexCountX;
    const Sint32 x =
        static_cast<Sint32>(static_cast<Uint32>(vertex) % static_cast<Uint32>(stride));
    const Sint32 z =
        static_cast<Sint32>(static_cast<Uint32>(vertex) / static_cast<Uint32>(stride));
    if (static_cast<Uint32>(x) >= static_cast<Uint32>(mCellCountX) ||
        static_cast<Uint32>(z) >= static_cast<Uint32>(mCellCountZ)) {
        return false;
    }
    // Jittered point in the 16m cell @ 0x100732aba..0x100732b34: two
    // RandomDoubleRangeUniform draws over [x*16, x*16+16] and [z*16, z*16+16]
    // (16.0f @ 0x100a16904).
    const Float32 loX = static_cast<Float32>(x) * 16.0f;
    const Float32 hiX = loX + 16.0f;
    const Float32 loZ = static_cast<Float32>(z) * 16.0f;
    const Float32 hiZ = loZ + 16.0f;
    const Float32 rndX = static_cast<Float32>(RandomDoubleRangeUniform(sRand, loX, hiX));
    const Float32 rndZ = static_cast<Float32>(RandomDoubleRangeUniform(sRand, loZ, hiZ));
    // Flora index roll @ 0x100732b3a..0x100732b44: RandomUint32Uniform on the
    // flora brush count (+0x150).
    const Sint32 floraIndex =
        static_cast<Sint32>(RandomUint32Uniform(sRand, static_cast<Uint32>(mFloraBrushInfo)));
    // y from vtable+0x60 = cSTETerrain::GetAltitude(float,float)
    // @ 0x100738928 (slot verified: __ZTV11cSTETerrain 0x100aea6b0 + 0x10 +
    // 0x60 holds 0x100738928) @ 0x100732b49..0x100732b5c.
    const Float32 y = GetAltitude(rndX, rndZ);
    // +0x148 flora-manager create (vt+0x90 with (0, &point, 0))
    // @ 0x100732b62..0x100732b73, then the spCity vt+0xb0 gate
    // @ 0x100732b79..0x100732b8e and the spAuraSim vt+0x38 notify
    // (1, point.x, point.z) @ 0x100732b90..0x100732bac.
    // clean-room fallback (not authentic): the create is recorded on the
    // modeled provider, the city gate is modeled as always-succeeding, and
    // the notify is a recorded no-op.
    mFloraPlacements.push_back(TerrainFloraPlacement{rndX, y, rndZ, floraIndex});
    return false; // always — xor eax, eax @ 0x100732bbf
}

bool cSTETerrain::BrushFnPaint(Sint32 vertex, const TerrainBrushContext& ctx) noexcept {
    // Exact cSTETerrain::BrushFnPaint @ 0x100732d22.
    // Gate @ 0x100732d39: the +0x130 decal/paint target must be non-null.
    if (mDecalTarget130 == nullptr) {
        return false;
    }
    const TerrainBrushComponent* comp = ctx.target; // 0x20(%r14) @ 0x100732d47
    if (comp == nullptr) {
        return false; // model guard (the binary would fault on a null target)
    }
    if (ctx.flag == 0) {
        return false; // cmpb $0, 0x14(%r14) @ 0x100732d54
    }
    // Cell bounds @ 0x100732d5f..0x100732d76 (unsigned, against +0x40/+0x44)
    // after the div by the row stride (+0x50).
    const Sint32 stride = mVertexCountX;
    const Sint32 x =
        static_cast<Sint32>(static_cast<Uint32>(vertex) % static_cast<Uint32>(stride));
    const Sint32 z =
        static_cast<Sint32>(static_cast<Uint32>(vertex) / static_cast<Uint32>(stride));
    if (static_cast<Uint32>(x) >= static_cast<Uint32>(mCellCountX) ||
        static_cast<Uint32>(z) >= static_cast<Uint32>(mCellCountZ)) {
        return false;
    }
    // Decal-map entry keyed by the recomputed vertex index (stride*z + x)
    // @ 0x100732d7c..0x100732e43 (find-or-insert over the std::map at
    // this+0x268; a fresh node's tDecalMapEntry count at node+0x20 is zeroed
    // by the 8-byte key store at node+0x1c @ 0x100732e2e/0x100732e31).
    const Uint32 key = static_cast<Uint32>(stride * z + x);
    const Uint32 count = static_cast<Uint32>(mDecalMap[key]);
    // Count gate (unsigned ja) against the component's maxDecalCount (+0x4c)
    // @ 0x100732e52..0x100732e5a, then the increment @ 0x100732e60/0x100732e62.
    if (count > comp->maxDecalCount) {
        return false;
    }
    mDecalMap[key] = static_cast<Sint32>(count + 1u);
    // Jitter draw @ 0x100732e86..0x100732ebc: only when the component's
    // jitterAmplitude (+0x54) is > 0; RandomDoubleRangeUniform(0.0, amp).
    Float32 jitter = 0.0f;
    if (comp->jitterAmplitude > 0.0f) {
        jitter = static_cast<Float32>(
            RandomDoubleRangeUniform(sRand, 0.0, static_cast<Float64>(comp->jitterAmplitude)));
    }
    // Jittered point, clamped to the world extents @ 0x100732ec1..0x100732f43:
    // lo = vertex*16, hi = min(lo + 16, cellCount*16) (minss @ 0x100732ef5/
    // 0x100732efe; +0x68/+0x6c = cellCount * 16.0f set by Init).
    const Float32 loX = static_cast<Float32>(x) * 16.0f;
    const Float32 loZ = static_cast<Float32>(z) * 16.0f;
    const Float32 worldW = static_cast<Float32>(mCellCountX) * 16.0f; // +0x68
    const Float32 worldH = static_cast<Float32>(mCellCountZ) * 16.0f; // +0x6c
    const Float32 hiX = std::min(worldW, loX + 16.0f);
    const Float32 hiZ = std::min(worldH, loZ + 16.0f);
    const Float32 rndX = static_cast<Float32>(
        RandomDoubleRangeUniform(sRand, static_cast<Float64>(loX), static_cast<Float64>(hiX)));
    const Float32 rndZ = static_cast<Float32>(
        RandomDoubleRangeUniform(sRand, static_cast<Float64>(loZ), static_cast<Float64>(hiZ)));
    // Place-decal call on the +0x130 target (vt+0x20) with
    // {{GUID 0x891B0E1A7AB50E44, comp+0x50}, point, ctx.amount, jitter}
    // @ 0x100732f4e..0x100732f6a. clean-room fallback (not authentic): the
    // callee is not reconstructed; the placement is recorded.
    mDecalTarget130->placements.push_back(TerrainDecalPlacementTarget::Placement{
        comp->decalResourceId, rndX, rndZ, ctx.amount, jitter});
    return false; // always — xor eax, eax @ 0x100732f6d
}

bool cSTETerrain::BrushFnPlaceEffect(Sint32 vertex, const TerrainBrushContext& ctx) noexcept {
    // Exact cSTETerrain::BrushFnPlaceEffect @ 0x100732f92.
    const TerrainBrushComponent* comp = ctx.target; // 0x20(%r15) @ 0x100732fa6
    if (comp == nullptr) {
        return false; // model guard (the binary would fault on a null target)
    }
    if (ctx.flag == comp->flag48) {
        return false; // cmpb 0x48(%rcx), al; je @ 0x100732fb7/0x100732fba
    }
    // NOTE (binary oddity): unlike BrushFnPaint there is NO cell bounds check
    // here — the div/mod results are used directly as the decal-map key.
    const Sint32 stride = mVertexCountX;
    const Sint32 x =
        static_cast<Sint32>(static_cast<Uint32>(vertex) % static_cast<Uint32>(stride));
    const Sint32 z =
        static_cast<Sint32>(static_cast<Uint32>(vertex) / static_cast<Uint32>(stride));
    const Uint32 key = static_cast<Uint32>(stride * z + x); // @ 0x100732fc4..0x100732fcf
    // Decal-map find-or-insert @ 0x100732fd2..0x100733094, then the count
    // gate (unsigned ja vs comp+0x4c) @ 0x1007330a1..0x1007330a8.
    const Uint32 count = static_cast<Uint32>(mDecalMap[key]);
    if (count > comp->maxDecalCount) {
        return false;
    }
    // +0x138 effect-target eligibility query (vt+0x38 with (comp+0x58,
    // comp+0x5c, &out)) @ 0x1007330b1..0x1007330c5 — the decal-map count
    // increments ONLY on success (incl @ 0x1007330d0). clean-room fallback
    // (not authentic): the query is modeled as the settable gate below.
    if (!mEffectTarget138.queryResult) {
        return false;
    }
    mDecalMap[key] = static_cast<Sint32>(count + 1u);
    // Transform/position @ 0x1007330d4..0x1007331da: a 0x38-byte struct
    // {word 0; 12x 1.0f matrix; 1.0f scale} (zero fill @ 0x1007330d8..0x1007330f7,
    // 1.0f const @ 0x100a15090) whose position slot +0x28..+0x30 receives
    // (rndX, max(seaLevel, GetAltitude(rndX, rndZ)), rndZ); the flags bytes
    // at +0/+0x1 take ++/|= 1 @ 0x1007331e0/0x1007331e5. The jittered point
    // mirrors BrushFnPaint's (lo = vertex*16, hi = min(lo + 16, world)).
    const Float32 loX = static_cast<Float32>(x) * 16.0f; // @ 0x100733122
    const Float32 loZ = static_cast<Float32>(z) * 16.0f;
    const Float32 worldW = static_cast<Float32>(mCellCountX) * 16.0f; // +0x68
    const Float32 worldH = static_cast<Float32>(mCellCountZ) * 16.0f; // +0x6c
    const Float32 hiX = std::min(worldW, loX + 16.0f); // minss @ 0x100733155
    const Float32 hiZ = std::min(worldH, loZ + 16.0f); // @ 0x10073315e
    const Float32 rndX = static_cast<Float32>(
        RandomDoubleRangeUniform(sRand, static_cast<Float64>(loX), static_cast<Float64>(hiX)));
    const Float32 rndZ = static_cast<Float32>(
        RandomDoubleRangeUniform(sRand, static_cast<Float64>(loZ), static_cast<Float64>(hiZ)));
    // vtable+0x60 = cSTETerrain::GetAltitude(float,float) @ 0x100738928
    // (slot verified via __ZTV11cSTETerrain 0x100aea6b0), maxed with the sea
    // level @ 0x1007331b8..0x1007331c4.
    const Float32 y = std::max(mSeaLevel, GetAltitude(rndX, rndZ));
    // Out-object calls vt+0x40(&transform)/vt+0x18(0)/vt+0x10
    // @ 0x1007331e8..0x100733208. clean-room fallback (not authentic): the
    // placement is recorded on the modeled +0x138 target.
    mEffectTarget138.placements.push_back(TerrainEffectPlacementTarget::Placement{
        comp->effectParam58, comp->effectParam5c, rndX, y, rndZ});
    return false; // always — xor eax, eax @ 0x10073320b
}

void cSTETerrain::InitFloraBrushInfo() {
    // Exact cSTETerrain::InitFloraBrushInfo @ 0x1007385e2.
    // One-shot gate @ 0x1007385f3: +0x150 != 0 means already initialized.
    if (mFloraBrushInfo != 0) {
        return;
    }
    // +0x148 provider queries @ 0x100738608..0x10073864d: vtable+0x40(0,
    // &count) for the type count (@ 0x10073861a, stored to +0x150
    // @ 0x10073861d), then the vector (+0x158) resize (@ 0x10073862d) and
    // vtable+0x40(ids, &count) to fill the ids (@ 0x10073864a).
    // clean-room fallback (not authentic): the provider is modeled as
    // FloraBrushInfoSource(); count = its size, the fill copies its ids in
    // order, and the fill call's return value (the initial loop bound
    // @ 0x10073864a/0x1007386d7) is modeled as that same count.
    const Sint32 count = static_cast<Sint32>(mFloraBrushInfoSource.size());
    mFloraBrushInfo = count;
    mFloraBrushIds.assign(mFloraBrushInfoSource.size(), 0u);
    for (std::size_t i = 0; i < mFloraBrushIds.size(); ++i) {
        mFloraBrushIds[i] = mFloraBrushInfoSource[i].id;
    }
    if (mFloraBrushInfo == 0) {
        return; // @ 0x10073864d/0x100738655
    }
    // Filter loop @ 0x10073865d..0x1007386da: id i survives only when its
    // type object's attribute byte (+0x180 & 3) == 2 (the vt+0x38 lookup
    // @ 0x100738675 returns the type object, byte read @ 0x100738678).
    // Removals shift the vector left and shrink it (@ 0x10073868f..0x1007386be)
    // and decrement the LIVE count at +0x150 (@ 0x1007386c5..0x1007386ce);
    // the loop index advances only on keep, and the bound is the live count
    // (kept fresh on both paths — reload @ 0x100738687, decrement
    // @ 0x1007386cc). clean-room fallback (not authentic): the lookup runs
    // by id over the modeled source; a miss (impossible via the fill path)
    // reads as a mismatch and removes the id.
    Uint8 keepAttr = 2; // bl @ 0x10073865b (re-set to 2 @ 0x1007386d5)
    for (Sint32 i = 0; i < mFloraBrushInfo; /* advance on keep only */) {
        const Uint32 id = mFloraBrushIds[static_cast<std::size_t>(i)];
        Uint8 attr = 0u;
        for (const TerrainFloraBrushType& t : mFloraBrushInfoSource) {
            if (t.id == id) {
                attr = static_cast<Uint8>(t.attr180 & 3u);
                break;
            }
        }
        if (attr == keepAttr) {
            ++i; // @ 0x100738684
        } else {
            mFloraBrushIds.erase(mFloraBrushIds.begin() + i);
            mFloraBrushInfo -= 1;
            keepAttr = 2;
        }
    }
}

void cSTETerrain::ApplyWaterErosionBrush(TerrainBrushContext& ctx, Sint32 strengthCells,
                                         SC4RectI& rect) {
    // Exact cSTETerrain::ApplyWaterErosionBrush @ 0x1007332ae.
    const Float32 kOneOverCellWidth = 0.0625f; // 0x100a158b0
    // Rect from the context point, truncated to the grid
    // @ 0x1007332c2..0x1007332ef: {left, top, right, bottom} =
    // {(int)(ctx.x/16), (int)(ctx.z/16)} on all four edges.
    const Sint32 gridX = static_cast<Sint32>(ctx.x * kOneOverCellWidth);
    const Sint32 gridY = static_cast<Sint32>(ctx.z * kOneOverCellWidth);
    rect.left = gridX;
    rect.top = gridY;
    rect.right = gridX;
    rect.bottom = gridY;
    // Pad by trunc-toward-zero(strengthCells / 2): (n + (n >>> 31)) >> 1
    // @ 0x1007332f6..0x1007332fb, then PadRectangle @ 0x100733302.
    const Sint32 pad =
        (strengthCells + static_cast<Sint32>(static_cast<Uint32>(strengthCells) >> 31)) >> 1;
    PadRectangle(rect, pad);
    // Application count = (int)ctx.amount @ 0x100733307; <= 0 -> nothing runs
    // @ 0x10073330d/0x10073330f.
    const Sint32 repeat = static_cast<Sint32>(ctx.amount);
    if (repeat <= 0) {
        return;
    }
    // Loop @ 0x100733311..0x10073332c (decl/jne do-while): each application
    // runs DoWaterErosion with the flow-step count
    // kNumFlowStepsPerWaterErosionBrushApplication[ctx.strengthIndex]
    // @ 0x100733315..0x10073331f (signed index; the binary has no bounds
    // check — the modeled table returns 0 outside its range).
    for (Sint32 i = 0; i < repeat; ++i) {
        const std::vector<Sint32>& table = FlowStepsPerErosionApplication();
        const Sint32 idx = ctx.strengthIndex;
        const Sint32 flowSteps =
            (idx >= 0 && static_cast<std::size_t>(idx) < table.size())
                ? table[static_cast<std::size_t>(idx)]
                : 0;
        DoWaterErosion(rect, flowSteps);
    }
}

Sint32 cSTETerrain::GetNeighborWithLowestWaterLevel(Sint32 vertex) noexcept {
    // Exact cSTETerrain::GetNeighborWithLowestWaterLevel @ 0x100737b50.
    if (vertex < 0 || static_cast<std::size_t>(vertex) >= mAltitude.size()) {
        return -1; // model guard (the binary's unsigned div assumes >= 0)
    }
    const Uint32 u = static_cast<Uint32>(vertex);
    const Uint32 stride = static_cast<Uint32>(mVertexCountX); // div @ 0x100737b61
    const Uint32 x = u % stride;
    const Uint32 z = u / stride;
    // Edge mask @ 0x100737b64..0x100737b88: default 0x7c (the west column
    // excludes directions 0,1,7), 0xff when x > 0; x >= cellCountX clears
    // bits 3,4,5 (east); z == 0 clears bits 1,2,3 (north); z >= cellCountZ
    // clears bits 5,6,7 (south).
    Uint32 mask = 0x7cu;
    if (x > 0) mask = 0xffu;
    if (static_cast<Uint32>(mCellCountX) <= x) mask &= 0xc7u;
    if (z == 0) mask &= 0xf1u;
    if (static_cast<Uint32>(mCellCountZ) <= z) mask &= 0x1fu;
    // My water level = altitude + depth @ 0x100737b8f..0x100737bab.
    const std::size_t v = static_cast<std::size_t>(vertex);
    const Float32 myLevel = mAltitude[v] + mSurfaceWater.depth[v];
    // Function-local static kRoot2 (0x3fb504f3 = sqrt(2), guard byte
    // @ 0x100b904f8 / value @ 0x100b904f0): the __cxa_guard_acquire/release
    // lazy-init pattern @ 0x100737bba..0x100737be5 is reproduced by the C++
    // function-local static initializer.
    static const Float32 kRoot2 = std::sqrt(2.0f);
    Float32 bestDiff = 0.0f;
    Sint32 bestDir = -1; // 0xffffffff @ 0x100737bf6
    // Cross loop @ 0x100737bfb..0x100733c48 (directions 0, 2, 4, 6): diffs
    // compared unscaled.
    for (Sint32 d = 0; d <= 6; d += 2) {
        if (((mask >> d) & 1u) == 0u) continue;
        const std::size_t n = static_cast<std::size_t>(
            vertex + NeighborVertexOffset(d, mVertexCountX));
        if (n >= mAltitude.size()) continue; // model guard
        const Float32 nLevel = mAltitude[n] + mSurfaceWater.depth[n];
        const Float32 diff = myLevel - nLevel; // subss @ 0x100733c32
        if (diff > bestDiff) {
            bestDiff = diff;
            bestDir = d;
        }
    }
    bestDiff = bestDiff * kRoot2; // @ 0x100733c4a
    // Diagonal loop @ 0x100733c59..0x100733ca3 (directions 1, 3, 5, 7).
    for (Sint32 d = 1; d <= 7; d += 2) {
        if (((mask >> d) & 1u) == 0u) continue;
        const std::size_t n = static_cast<std::size_t>(
            vertex + NeighborVertexOffset(d, mVertexCountX));
        if (n >= mAltitude.size()) continue; // model guard
        const Float32 nLevel = mAltitude[n] + mSurfaceWater.depth[n];
        const Float32 diff = myLevel - nLevel;
        if (diff > bestDiff) {
            bestDiff = diff;
            bestDir = d;
        }
    }
    return bestDir;
}

void cSTETerrain::DoWaterErosion(const SC4RectI& rect, Sint32 numFlowSteps) {
    // Exact cSTETerrain::DoWaterErosion @ 0x1007335e6. Phases:
    //   A (@ 0x100733680..0x1007336fa): depth/sediment reset over the rect.
    //   B (@ 0x100733707..0x100733a8f, only when numFlowSteps > 0): per flow
    //     step, a rainfall pass then a flow/erode/deposit pass.
    //   C (@ 0x100733a95..0x100733b02): padded-rect water reset.
    // Function-local static kOneByRoot2 (0x3f3504f3 = 1/sqrt(2), guard byte
    // @ 0x100b904e8 / value @ 0x100b904e0): the __cxa_guard_acquire/release
    // lazy-init pattern @ 0x100733616..0x10073364f is reproduced by the C++
    // function-local static initializer.
    static const Float32 kOneByRoot2 = 1.0f / std::sqrt(2.0f);

    // Tunables (read from the shipped __data bytes):
    const Float32 kRainFallFactor           = 0.01f;                 // @ 0x100b17608
    const Float32 kMaxSedimentPerUnitWater  = 5.0f;                   // @ 0x100b1760c
    const Float32 kHydrDepositRate         = 0.2f;                    // @ 0x100b17610
    const Float32 kMinHydrErosionRate       = 0.02f;                   // @ 0x100b17618
    const Float32 kMaxHydrErosionRate       = 0.05999999865889549f;   // @ 0x100b1761c
    const Float32 kHydrErosionRateRampBegin = 5.127999782562256f;     // @ 0x100b17620
    const Float32 kHydrErosionRateRampEnd   = 38.13999938964844f;     // @ 0x100b17624

    const Sint32 stride = mVertexCountX; // +0x50
    // Water-erosion threshold = sea level + (-15.0f) (addss @ 0x100733653,
    // const @ 0x100a1825c).
    const Float32 threshold = mSeaLevel + -15.0f;
    // Erosion-rate ramp slope = (max - min) / (rampEnd - rampBegin)
    // @ 0x10073365b..0x100733680 (both differences precomputed; the divide
    // happens once per flow-step pass @ 0x10073370c).
    const Float32 rateSlope = (kMaxHydrErosionRate - kMinHydrErosionRate) /
                              (kHydrErosionRateRampEnd - kHydrErosionRateRampBegin);

    // Model guard (not in the binary): clamp the rect to the vertex grid so
    // the flat-vector model cannot run out of bounds. PadRectangle
    // (@ 0x100733596) already clamps to cellCountX/Z on the pipeline path.
    const Sint32 rowMin = rect.top < 0 ? 0 : rect.top;
    const Sint32 rowMax = rect.bottom >= mVertexCountZ ? mVertexCountZ - 1 : rect.bottom;
    const Sint32 colMin = rect.left < 0 ? 0 : rect.left;
    const Sint32 colMax = rect.right >= mVertexCountX ? mVertexCountX - 1 : rect.right;

    // Phase A @ 0x100733680..0x1007336fa: depth = (threshold - altitude)
    // when altitude < threshold else 0 (cmpltss + andps
    // @ 0x1007336bf..0x1007336c8); sediment (record +0x1c) = 0
    // @ 0x1007336dc.
    for (Sint32 z = rowMin; z <= rowMax; ++z) {
        for (Sint32 x = colMin; x <= colMax; ++x) {
            const std::size_t v = static_cast<std::size_t>(z * stride + x);
            const Float32 alt = mAltitude[v];
            mSurfaceWater.depth[v] = (alt < threshold) ? (threshold - alt) : 0.0f;
            mSurfaceWater.sediment[v] = 0.0f;
        }
    }

    // Phase B @ 0x1007336fd..0x100733701: numFlowSteps <= 0 skips straight
    // to Phase C.
    for (Sint32 step = 0; step < numFlowSteps; ++step) {
        // B.1 rainfall pass @ 0x10073372c..0x100733794: depth += altitude *
        // kRainFallFactor for vertices strictly above the threshold
        // (mulss @ 0x10073375e, addss @ 0x10073376e, ucomiss/jbe skip
        // @ 0x100733759/0x10073375c).
        for (Sint32 z = rowMin; z <= rowMax; ++z) {
            for (Sint32 x = colMin; x <= colMax; ++x) {
                const std::size_t v = static_cast<std::size_t>(z * stride + x);
                const Float32 alt = mAltitude[v];
                if (alt > threshold) {
                    mSurfaceWater.depth[v] =
                        mSurfaceWater.depth[v] + alt * kRainFallFactor;
                }
            }
        }
        // B.2 flow/erode/deposit pass @ 0x10073379a..0x100733a8f.
        for (Sint32 z = rowMin; z <= rowMax; ++z) {
            for (Sint32 x = colMin; x <= colMax; ++x) {
                const Sint32 vI = z * stride + x;
                const std::size_t v = static_cast<std::size_t>(vI);
                Float32 myDepth = mSurfaceWater.depth[v];
                if (!(myDepth > 0.0f)) {
                    continue; // ucomiss 0, depth; jae skip @ 0x1007337e0/0x1007337e4
                }
                const Float32 myAlt = mAltitude[v];
                if (threshold > myAlt) {
                    // Submerged vertex: reset the depth to
                    // (threshold - altitude) @ 0x1007337f7..0x100733809 and
                    // skip the flow computation.
                    mSurfaceWater.depth[v] = threshold - myAlt;
                    continue;
                }
                // Neighbor selection @ 0x10073380e..0x1007338c9. BINARY
                // ODDITY: vertices on the RECT boundary dump their whole
                // depth into nothing (neighbor index -1, flow = full depth);
                // only interior vertices flow toward the lowest-water-level
                // neighbor (GetNeighborWithLowestWaterLevel @ 0x100733852;
                // no lower neighbor -> the vertex is skipped entirely
                // @ 0x100733857..0x100733859).
                Sint32 nbr = -1;
                Sint32 dir = -1;
                Float32 flow = myDepth;
                if (x == rect.right || z == rect.top || z == rect.bottom) {
                    // flow = full depth, neighbor = -1 @ 0x100733811..0x1007338cb
                } else if (x == rect.left) {
                    // flow = full depth, neighbor = -1 @ 0x10073383a
                } else {
                    const Sint32 dirRet = GetNeighborWithLowestWaterLevel(vI);
                    if (dirRet < 0) {
                        continue;
                    }
                    dir = dirRet;
                    // Direction-index -> vertex-offset table at this+0xa4
                    // @ 0x100733862.
                    nbr = vI + NeighborVertexOffset(dir, stride);
                    // flow = min(depth, (myLevel - nbrLevel) * 0.5f)
                    // @ 0x10073387a..0x1007338b8 (0.5f @ 0x100a158bc).
                    const std::size_t n = static_cast<std::size_t>(nbr);
                    const Float32 nbrLevel = mSurfaceWater.depth[n] + mAltitude[n];
                    flow = std::min(myDepth, (myDepth + myAlt - nbrLevel) * 0.5f);
                }
                if (!(flow > 0.0f)) {
                    continue; // ucomiss flow, 0; jbe @ 0x1007338d1/0x1007338d5
                }
                // Sediment/erosion @ 0x1007338db..0x1007339a9.
                const Float32 capacity = kMaxSedimentPerUnitWater * flow; // @ 0x1007338e3
                const Float32 mySediment = mSurfaceWater.sediment[v];
                Float32 newSediment;
                Float32 carry;
                if (mySediment > capacity) {
                    // BINARY ODDITY: already carrying more than the flow can
                    // move -> the excess STAYS on the vertex (no erosion)
                    // and the flow hands its full capacity to the neighbor
                    // (@ 0x1007338f3/0x1007338f7 jump past the rate block;
                    // the shared tail @ 0x10073399f/0x1007339a6 stores
                    // sediment - capacity and carry = capacity).
                    newSediment = mySediment - capacity;
                    carry = capacity;
                } else {
                    // Erosion rate from the (diagonal-scaled) height diff
                    // @ 0x1007338fc..0x10073396a: edges (neighbor index -1)
                    // erode at the min rate (height diff 0).
                    Float32 heightDiff = 0.0f;
                    if (nbr >= 0) {
                        heightDiff = myAlt - mAltitude[static_cast<std::size_t>(nbr)];
                        if ((dir & 1) != 0) {
                            // Diagonal neighbor: scale by 1/sqrt(2)
                            // @ 0x100733910..0x10073391c.
                            heightDiff = heightDiff * kOneByRoot2;
                        }
                    }
                    Float32 rate;
                    if (kHydrErosionRateRampBegin >= heightDiff) {
                        rate = kMinHydrErosionRate; // jae @ 0x100733960
                    } else if (heightDiff >= kHydrErosionRateRampEnd) {
                        rate = kMaxHydrErosionRate; // jae @ 0x10073396a
                    } else {
                        rate = (heightDiff - kHydrErosionRateRampBegin) * rateSlope +
                               kMinHydrErosionRate; // @ 0x10073394d..0x10073395e
                    }
                    if ((mVertexFlags[v] & 0x4000u) != 0u) {
                        // Vertex-record flags bit 0x4000 (byte at record
                        // +0x23, bit 0x40 @ 0x100733975) suppresses erosion:
                        // the sediment stays as-is and nothing is carried.
                        newSediment = mySediment;
                        carry = 0.0f; // xorps xmm6 @ 0x100733972
                    } else {
                        const Float32 erode = (capacity - mySediment) * rate; // @ 0x10073397d/0x100733981
                        mAltitude[v] = myAlt - erode;                          // @ 0x100733985/0x100733989
                        newSediment = 0.0f;                                    // @ 0x10073399c
                        carry = erode + mySediment;                            // @ 0x10073398f..0x1007339a6
                    }
                }
                mSurfaceWater.sediment[v] = newSediment; // @ 0x10073399f
                // Move the water @ 0x1007339a9..0x1007339fe.
                myDepth = myDepth - flow;
                mSurfaceWater.depth[v] = myDepth;
                if (nbr >= 0) {
                    const std::size_t n = static_cast<std::size_t>(nbr);
                    if (mAltitude[n] > threshold) { // ucomiss/jbe @ 0x1007339cb/0x1007339ce
                        mSurfaceWater.depth[n] = mSurfaceWater.depth[n] + flow;      // @ 0x1007339e0
                        mSurfaceWater.sediment[n] = mSurfaceWater.sediment[n] + carry; // @ 0x1007339f2
                        myDepth = mSurfaceWater.depth[v]; // reload @ 0x1007339fe
                    }
                }
                // Deposit @ 0x100733a04..0x100733a52.
                if (myDepth > 0.0f) {
                    const Float32 s = mSurfaceWater.sediment[v];
                    if (s <= 0.0f) {
                        continue; // jbe @ 0x100733a11/0x100733a15
                    }
                    if ((mVertexFlags[v] & 0x4000u) != 0u) {
                        continue; // test/jne @ 0x100733a1e/0x100733a24
                    }
                    const Float32 deposit = s * kHydrDepositRate; // @ 0x100733a26
                    mAltitude[v] = mAltitude[v] + deposit;        // @ 0x100733a2e..0x100733a38
                    mSurfaceWater.sediment[v] = s - deposit;      // @ 0x100733a3e..0x100733a49
                } else {
                    mSurfaceWater.sediment[v] = 0.0f; // @ 0x100733a52
                }
            }
        }
    }

    // Phase C @ 0x100733a95..0x100733b02: pad the rect by {-1,-1,+1,+1}
    // (SSE paddd const @ 0x100a15ce0), clamp against the ORIGINAL edges
    // (left/top <= 0 -> 0 @ 0x100733aba..0x100733ad3; right >= cellCountX /
    // bottom >= cellCountZ -> cellCount @ 0x100733ada..0x100733af7), then
    // cSTESurfaceWater::ResetWaterToSeaLevel @ 0x10072e648.
    SC4RectI padded{rect.left - 1, rect.top - 1, rect.right + 1, rect.bottom + 1};
    if (rect.top <= 0) padded.top = 0;
    if (rect.left <= 0) padded.left = 0;
    if (rect.right >= mCellCountX) padded.right = mCellCountX;
    if (rect.bottom >= mCellCountZ) padded.bottom = mCellCountZ;
    // clean-room fallback (not authentic): cSTESurfaceWater::ResetWaterToSeaLevel
    // @ 0x10072e648 is not decompiled. Modeled with Phase A's clamp pattern
    // at plain sea level (no -15 offset): depth = (altitude < seaLevel) ?
    // (seaLevel - altitude) : 0 over the padded rect.
    const Sint32 rMin = padded.top < 0 ? 0 : padded.top;
    const Sint32 rMax = padded.bottom >= mVertexCountZ ? mVertexCountZ - 1 : padded.bottom;
    const Sint32 cMin = padded.left < 0 ? 0 : padded.left;
    const Sint32 cMax = padded.right >= mVertexCountX ? mVertexCountX - 1 : padded.right;
    for (Sint32 z = rMin; z <= rMax; ++z) {
        for (Sint32 x = cMin; x <= cMax; ++x) {
            const std::size_t v = static_cast<std::size_t>(z * stride + x);
            mSurfaceWater.depth[v] =
                (mAltitude[v] < mSeaLevel) ? (mSeaLevel - mAltitude[v]) : 0.0f;
        }
    }
}

TerrainBrushMemberFn cSTETerrain::SurfaceWaterRain(Sint32 vertex, TerrainBrushContext& ctx,
                                                   SC4RectI& rect, Float32 strength) {
    // Exact cSTESurfaceWater::Rain @ 0x100730820: when the surface-water gate
    // at +0x70 is set, the member-fn pair {&cSTETerrain::BrushFnRain, 0} is
    // returned (lea BrushFnRain @ 0x100730826, xor edx @ 0x10073083b) so the
    // generic sweep runs; otherwise Rain_Static is invoked and a null pointer
    // is returned (sweep skipped).
    if (mSurfaceWater.flag70 != 0) {
        return TerrainBrushMemberFn{&cSTETerrain::BrushFnRain, false, 0};
    }
    // clean-room fallback (not authentic): cSTESurfaceWater::Rain_Static
    // @ 0x100730833 (body not reconstructed) — modeled as a no-op.
    (void)vertex;
    (void)ctx;
    (void)rect;
    (void)strength;
    return TerrainBrushMemberFn{nullptr, false, 0};
}

void cSTETerrain::ReDisplayTerrain(Sint32 a, Sint32 b, const SC4RectI& rect, Sint32 c) {
    // vtable+0x228 re-display entry (callee not resolved; dispatched by
    // ApplyBrush @ 0x100731d07 with (1, 1, &rect, params+0x20)). clean-room
    // fallback (not authentic): the call is recorded so the verify suite can
    // observe the gate.
    (void)a;
    (void)b;
    (void)rect;
    (void)c;
    ++mReDisplayCount;
}

void cSTETerrain::BeginBrushScrubDelegate(Uint32 brushID, Uint32 scrubID) {
    // vtable+0x2b8 scrub tail (BeginBrushScrub @ 0x100734b58; callee not
    // resolved). clean-room fallback (not authentic): recorded no-op.
    (void)brushID;
    (void)scrubID;
    ++mBeginScrubDelegateCount;
}

// ---------------------------------------------------------------------------
// Surface-cover cluster (M3 terrain wave)
//
// Section constants, all read from the shipped binary bytes (fresh dumps,
// cross-validated against the landed DoWaterErosion tunables at neighboring
// __DATA addresses):
namespace {

constexpr Float32 kLandTypeValueScale = 255.0f;  // 0x100a161f0
constexpr Float32 kOneF = 1.0f;                  // 0x100a15090
constexpr Float32 kCellWidthF = 16.0f;           // 0x100a16904 (vertex -> world)
constexpr Float32 kLowCliffAddValue = 50.0f;     // 0x100a16e84
constexpr Float32 kBeachAddValue = 1.0f;         // 0x100a15090
constexpr Float32 kKernelEps = 1e-5f;            // 0x100a18750 (9.9999997e-06)
constexpr Float32 kKernelC3 = 3.0f;              // 0x100a165b8
constexpr Float32 kKernelHalf = 0.5f;            // 0x100a158bc
constexpr Float32 kKernelScale = 0.00390625f;    // 0x100a1b65c (1/256)
constexpr Float32 kDistSqStep = 2.0f;            // 0x100a16690
constexpr Float32 kAshGateZero = 0.0f;           // 0x100a15cd8
constexpr Float32 kTempScale = 30.0f;            // 0x100a1969c
// 0x100b175e4 (kMaxBeachWidth), 0x100b175e8 (kMaxBeachAltitude), 0x100b17634
// (kMinWaterDepthForDeepSeaBedTexture), 0x100b17638
// (kMaxWaterDepthForDeepSeaBedTexture) and 0x100b17684 (kVolcanicAshLifeTime)
// are tuning globals wired by ReadTuningParameters @ 0x10073d95e — modeled by
// the MaxBeachWidthTunable()/MaxBeachAltitudeTunable()/
// Min/MaxWaterDepthForDeepSeaBedTextureTunable()/VolcanicAshLifeTimeTunable()
// accessors (see the tuning-loading section below).
constexpr Uint32 kVolcanicAshTextureId = 0xe0u;            // 0x100a29c90

// Runtime-filled-table read guards. The binary has NO bounds checks on any
// of these (e.g. 0x10073403c, 0x100734293); the flat-vector model returns 0
// / "not found" instead of faulting. clean-room fallback (not authentic).
Sint32 TableAt(const std::vector<Sint32>& t, Sint32 i) noexcept {
    return (i >= 0 && static_cast<std::size_t>(i) < t.size())
               ? t[static_cast<std::size_t>(i)] : 0;
}
Uint32 TableAtU(const std::vector<Uint32>& t, Sint32 i) noexcept {
    return (i >= 0 && static_cast<std::size_t>(i) < t.size())
               ? t[static_cast<std::size_t>(i)] : 0u;
}
Float32 TableAtF(const std::vector<Float32>& t, Sint32 i) noexcept {
    return (i >= 0 && static_cast<std::size_t>(i) < t.size())
               ? t[static_cast<std::size_t>(i)] : 0.0f;
}

// RZConvertToUintPtrA @ 0x10001a307 — not reconstructed. clean-room fallback
// (not authentic): the settable hook wins; otherwise a clean-room FNV-1a
// 32-bit hash keeps the entry math observable.
Uint32 ConvertSurfaceCoverString(const char* s) noexcept {
    if (auto hook = cSTETerrain::SurfaceCoverStringHashHook()) {
        return hook(s);
    }
    Uint32 h = 2166136261u;
    if (s != nullptr) {
        while (*s != '\0') {
            h ^= static_cast<Uint32>(static_cast<Uint8>(*s++));
            h *= 16777619u;
        }
    }
    return h;
}

// Texture-family variant for one vertex/texture: grid cell -> family member
// (word) -> member % binding count. Exact op sequence @ 0x100734021..0x100734086
// (water path; the beach block 0x100734108..0x10073416c and the temperature
// block 0x1007342a7..0x1007342ff are identical). Unsigned div throughout;
// gridSize <= 0 and missing binding entries are model guards.
Uint32 SurfaceCoverTextureVariant(Sint32 vertex, Uint32 texId, Sint32 strength,
                                  Sint32 stride) noexcept {
    const Uint32 u = static_cast<Uint32>(vertex);
    const Uint32 strideU = static_cast<Uint32>(stride);
    const Uint32 vz = u / strideU; // div [this+0x50]
    const Uint32 vx = u % strideU;
    Sint32 gridSize = TableAt(cSTETerrain::TextureRandomizationGridSize(), strength);
    if (gridSize <= 0) gridSize = 1; // model guard (the binary divides blindly)
    const Sint32 cellCountX =
        TableAt(cSTETerrain::TextureGridCellCountX(), strength);
    // gridIndex = (z / gridSize) * sTextureGridCellCountX + x / gridSize
    // (imul/add @ 0x100734050..0x100734054).
    const Uint32 gridIndex = (vz / static_cast<Uint32>(gridSize)) *
                                 static_cast<Uint32>(cellCountX) +
                             (vx / static_cast<Uint32>(gridSize));
    uint16_t member = 0;
    const auto& families = cSTETerrain::TextureFamilyMemberForGrid();
    if (strength >= 0 && static_cast<std::size_t>(strength) < families.size() &&
        gridIndex < families[static_cast<std::size_t>(strength)].size()) {
        member = families[static_cast<std::size_t>(strength)][gridIndex];
    }
    Sint32 count = 1; // model guard: missing/invalid entry -> 1
    const auto& bindings = cSTETerrain::TerrainTextureBindingCounts();
    if (strength >= 0 && static_cast<std::size_t>(strength) < bindings.size()) {
        const auto& m = bindings[static_cast<std::size_t>(strength)];
        const auto it = m.find(texId);
        if (it != m.end() && it->second > 0) count = it->second;
    }
    return static_cast<Uint32>(member) % static_cast<Uint32>(count); // div @ 0x10073407d
}

// cSC4ResponseCurve::GetOutputValue(float) @ 0x1000ac4f6 — NOT reconstructed.
// The 32-byte curve objects live at tSurfaceCoverInfo+0x10+32*strength
// (this @ 0x100734623..0x10073462f, 0x1007349b4..0x1007349bd). clean-room
// fallback (not authentic): piecewise-linear over the settable control
// points, clamped; a missing/empty curve returns 0.
Float32 EvaluateSurfaceCoverCurve(const TerrainSurfaceCoverInfo& info,
                                  Sint32 strength, Float32 x) noexcept {
    if (strength < 0 || static_cast<std::size_t>(strength) >= info.curves.size()) {
        return 0.0f;
    }
    return info.curves[static_cast<std::size_t>(strength)].Evaluate(x);
}

// cSC4MapUtilities::FloodFillDistance @ 0x1000d82b4 — NOT reconstructed
// (called with (map, kMaxBeachWidth, 0) @ 0x100733d7e..0x100733d8e, only
// when water vertices exist). clean-room fallback (not authentic):
// multi-source orthogonal BFS from every 0-valued (water) cell; a land cell
// reached within maxDistance steps receives its step distance, cells farther
// away keep the init value (kMaxBeachWidth + 2).
void FloodFillDistanceCleanRoom(std::vector<std::vector<Uint8>>& map,
                                 Uint8 maxDistance) noexcept {
    if (map.empty()) return;
    const std::size_t rows = map.size();
    const std::size_t cols = map[0].size();
    std::vector<std::vector<Uint8>> dist(
        rows, std::vector<Uint8>(cols, 0xffu));
    std::vector<std::pair<Sint32, Sint32>> queue;
    for (Sint32 z = 0; static_cast<std::size_t>(z) < rows; ++z) {
        for (Sint32 x = 0; static_cast<std::size_t>(x) < cols; ++x) {
            if (map[static_cast<std::size_t>(z)][static_cast<std::size_t>(x)] == 0u) {
                dist[static_cast<std::size_t>(z)][static_cast<std::size_t>(x)] = 0u;
                queue.emplace_back(z, x);
            }
        }
    }
    for (std::size_t head = 0; head < queue.size(); ++head) {
        const auto [cz, cx] = queue[head];
        const Uint8 d = dist[static_cast<std::size_t>(cz)][static_cast<std::size_t>(cx)];
        if (d >= maxDistance) continue; // cap @ kMaxBeachWidth
        const Sint32 nz[4] = {cz - 1, cz + 1, cz, cz};
        const Sint32 nx[4] = {cx, cx, cx - 1, cx + 1};
        for (int k = 0; k < 4; ++k) {
            if (nz[k] < 0 || static_cast<std::size_t>(nz[k]) >= rows ||
                nx[k] < 0 || static_cast<std::size_t>(nx[k]) >= cols) continue;
            const std::size_t rz = static_cast<std::size_t>(nz[k]);
            const std::size_t rx = static_cast<std::size_t>(nx[k]);
            if (dist[rz][rx] != 0xffu) continue;
            if (map[rz][rx] == 0u) continue; // water cells keep 0
            dist[rz][rx] = static_cast<Uint8>(d + 1u);
            queue.emplace_back(nz[k], nx[k]);
        }
    }
    for (std::size_t z = 0; z < rows; ++z) {
        for (std::size_t x = 0; x < cols; ++x) {
            if (dist[z][x] != 0xffu) map[z][x] = dist[z][x];
        }
    }
}

} // namespace

Float32 SurfaceCoverResponseCurve::Evaluate(Float32 x) const noexcept {
    // cSC4ResponseCurve::GetOutputValue @ 0x1000ac4f6 — see the
    // FloodFillDistanceCleanRoom note above. clean-room fallback (not
    // authentic).
    if (points.empty()) return 0.0f;
    if (x <= points.front().first) return points.front().second;
    for (std::size_t i = 1; i < points.size(); ++i) {
        if (x <= points[i].first) {
            const auto& a = points[i - 1];
            const auto& b = points[i];
            const Float32 t = (x - a.first) / (b.first - a.first);
            return a.second + (b.second - a.second) * t;
        }
    }
    return points.back().second;
}

// ---- Runtime-filled static table models (all clean-room fallback, not
//      authentic — fill sites not reconstructed, TODO) ----

std::vector<Uint32>& cSTETerrain::LowCliffTexResIds() {           // 0x100b71ccc
    static std::vector<Uint32> t;
    return t;
}
std::vector<Uint32>& cSTETerrain::BeachTexResIds() {             // 0x100b71cd0
    static std::vector<Uint32> t;
    return t;
}
std::vector<Sint32>& cSTETerrain::MaxSurfaceTextureSpread() {    // 0x100b71d10
    static std::vector<Sint32> t;
    return t;
}
std::vector<Sint32>& cSTETerrain::TextureRandomizationGridSize() { // 0x100b713f0
    static std::vector<Sint32> t;
    return t;
}
std::vector<Sint32>& cSTETerrain::TextureGridCellCountX() {      // 0x100b71490
    static std::vector<Sint32> t;
    return t;
}
std::vector<std::vector<uint16_t>>& cSTETerrain::TextureFamilyMemberForGrid() { // 0x100b71410
    static std::vector<std::vector<uint16_t>> t;
    return t;
}
std::vector<std::map<Uint32, Sint32>>& cSTETerrain::TerrainTextureBindingCounts() { // 0x100b720e0 (modeled)
    static std::vector<std::map<Uint32, Sint32>> t;
    return t;
}
std::vector<Sint32>& cSTETerrain::SurfaceCoverMap() {            // 0x100b714b0
    static std::vector<Sint32> t;
    return t;
}
std::unordered_map<Uint32, TerrainSurfaceCoverInfo>&
cSTETerrain::SurfaceCoverTextures() {                            // 0x100b71cd8
    static std::unordered_map<Uint32, TerrainSurfaceCoverInfo> t;
    return t;
}
std::map<Sint32, TerrainTypeTextureList>&
cSTETerrain::TexturesUsedInTerrainType() {                       // 0x100b71cb0
    static std::map<Sint32, TerrainTypeTextureList> t;
    return t;
}
Uint32 (*&cSTETerrain::SurfaceCoverStringHashHook())(const char*) {
    static Uint32 (*hook)(const char*) = nullptr;
    return hook;
}
Float32& cSTETerrain::TemperatureRangeFactor() {
    // Shipped __DATA bytes at 0x100b71db0 are the bit pattern 0x0000000a
    // (int 10) while DecideSurfaceCover reads the slot as a float
    // (movss @ 0x100733eb9..0x100733ec1) -> denormal 1.4e-44. The loader
    // presumably overwrites it (fill site not reconstructed, TODO); the
    // model default reproduces the shipped pattern exactly.
    static Float32 v = std::bit_cast<Float32>(0x0000000au);
    return v;
}
Float32& cSTETerrain::MaxTerrainHeightTunable() {
    // cSTETerrain::kMaxTerrainHeight @ 0x100b1763c — shipped bytes 200.0f
    // (fresh dump; nm-confirmed). Runtime-overridden from the terrain tuning
    // exemplar @ 0x10073d9e4 (cSCPropertyHelp::GetPropertyValue property
    // 0x48cd7b26); the wiring is TODO. The value itself is binary-exact;
    // only the override plumbing is future work.
    static Float32 v = 200.0f;
    return v;
}
Float32& cSTETerrain::MinTerrainHeightTunable() {
    // cSTETerrain::kMinTerrainHeight @ 0x100b17640 — shipped bytes 5.0f
    // (fresh dump). Runtime-overridden @ 0x10073da07 (property 0x48cd7b27);
    // the loader also overwrites kSeaLevel with this value when it exceeds
    // the sea level @ 0x10073da31 (TODO — exemplar wiring).
    static Float32 v = 5.0f;
    return v;
}
TerrainView3DObservation& cSTETerrain::View3DObservation() {
    static TerrainView3DObservation obs;
    return obs;
}

// ---- Land-type slots ----

void cSTETerrain::AddLandTypeAtVertex(Uint32 vertex, uint16_t type,
                                      Float32 ratio) noexcept {
    // Exact cSTETerrain::AddLandTypeAtVertex @ 0x100731338.
    // Quantize: int(ratio * 255.0f + 1.0f), truncate toward zero
    // (mulss 0x100a161f0, addss 0x100a15090, cvttss2si r8d
    // @ 0x10073333c..0x10073334c).
    const Sint32 quantized = static_cast<Sint32>(ratio * kLandTypeValueScale + kOneF);
    const uint16_t q = static_cast<uint16_t>(quantized);
    if (static_cast<std::size_t>(vertex) >= mLandTypes.size()) {
        return; // model guard (the binary has none)
    }
    auto& slots = mLandTypes[static_cast<std::size_t>(vertex)];
    // Slot search @ 0x10073136f..0x100731384 (slots 0..2 of the 36-byte
    // record, type at record+0x1a+4*i, value at record+0x1c+4*i):
    for (Sint32 i = 0; i < 3; ++i) {
        if (slots[static_cast<std::size_t>(i)].type == 0) {
            // First empty slot: fill it @ 0x1007313c5..0x1007313d4.
            slots[static_cast<std::size_t>(i)].type = type;
            slots[static_cast<std::size_t>(i)].value = q;
            return;
        }
        if (slots[static_cast<std::size_t>(i)].type == type) {
            // Existing type: keep the max value @ 0x1007313d6..0x1007313e5
            // (cmp word, r8w; jae keep-old).
            if (slots[static_cast<std::size_t>(i)].value < q) {
                slots[static_cast<std::size_t>(i)].value = q;
            }
            return;
        }
    }
    // Full table: replace the min-value slot when the new value beats it
    // @ 0x100731386..0x1007313c3 (min scan with cmovb pairs, then
    // cmp di, r8w; jae keep-old).
    Sint32 minIdx = 0;
    uint16_t minValue = slots[0].value;
    for (Sint32 i = 1; i < 3; ++i) {
        if (slots[static_cast<std::size_t>(i)].value < minValue) {
            minValue = slots[static_cast<std::size_t>(i)].value;
            minIdx = i;
        }
    }
    if (minValue >= q) return;
    slots[static_cast<std::size_t>(minIdx)].type = type;
    slots[static_cast<std::size_t>(minIdx)].value = q;
}

void cSTETerrain::AddToTerrainTypeTextureList(Sint32 typeIndex, Uint32 textureId) {
    // Exact cSTETerrain::AddToTerrainTypeTextureList @ 0x10073159e (static).
    // Sorted-unique ascending insert into the 24-byte
    // sTexturesUsedInTerrainType[typeIndex] entry (list at +0, size at +0x10).
    TerrainTypeTextureList& entry = TexturesUsedInTerrainType()[typeIndex];
    auto& list = entry.textures;
    // Walk @ 0x1007315c1..0x1007315e8: stop at end, at an equal value
    // (no insert) or at the first greater node (insertion point).
    auto it = list.begin();
    while (it != list.end()) {
        if (*it == textureId) return; // equal: al = 1 -> exit @ 0x1007315ea
        if (*it > textureId) break;    // greater: al = 2 -> insert here
        ++it;
    }
    list.insert(it, textureId);
    ++entry.count; // incq [entry+0x10] @ 0x100731614
}

void cSTETerrain::AddTemperatureAndMoistureToSurfaceCoverEntry(const char* key,
                                                               const char* value,
                                                               Sint32 entryIndex) {
    // Exact cSTETerrain::AddTemperatureAndMoistureToSurfaceCoverEntry @
    // 0x100731418 (static). Entry base = sSurfaceCoverMap +
    // ((entryIndex & 0x7fffff) << 11) + ((RZConvertToUintPtrA(key) &
    // 0xfffffff) << 6) bytes @ 0x10073148e..0x1007314b2, i.e. flat int index
    // (entryIndex & 0x7fffff)*512 + (keyHash & 0xfffffff)*16.
    const Uint32 typeIdx = static_cast<Uint32>(entryIndex) & 0x7fffffu;
    const Uint32 keyHash = ConvertSurfaceCoverString(key) & 0xfffffffu;
    // Token loop @ 0x1007314c4..0x100731522: the value string is split on ','
    // (cRZString::SplitToken — plumbing modeled with const char*), and the
    // loop runs r13 = 0..14 — ALWAYS 15 iterations (cmp r13, 0xf; jb),
    // regardless of the token count; the cRZString state SplitToken leaves
    // at exhaustion is not reconstructed, so the model hashes "" for the
    // remaining iterations (clean-room fallback, not authentic).
    const char* cursor = value;
    for (Sint32 i = 0; i < 15; ++i) {
        char token[256];
        std::size_t n = 0;
        if (cursor != nullptr && *cursor != '\0') {
            while (*cursor != '\0' && *cursor != ',' && n + 1 < sizeof(token)) {
                token[n++] = *cursor++;
            }
            if (*cursor == ',') ++cursor; // SplitToken separator
        }
        token[n] = '\0';
        const Uint32 id = ConvertSurfaceCoverString(token);
        // Store at entry + 4 + 4*i @ 0x10073150c (sub-entry ints [1..15];
        // int 0 is the key slot). Model guard: skip absurd indices.
        const std::size_t flat = static_cast<std::size_t>(typeIdx) * 512u +
                                 static_cast<std::size_t>(keyHash) * 16u +
                                 static_cast<std::size_t>(i) + 1u;
        if (flat < (1u << 22)) {
            auto& map = SurfaceCoverMap();
            if (map.size() <= flat) map.resize(flat + 1u, 0);
            map[flat] = static_cast<Sint32>(id);
        }
        // AddToTerrainTypeTextureList(entryIndex, id) @ 0x100731511..0x100731516.
        AddToTerrainTypeTextureList(entryIndex, id);
    }
}

void cSTETerrain::AdjustLandTypeRatios(const SC4RectI& rect) noexcept {
    // Exact cSTETerrain::AdjustLandTypeRatios @ 0x10073173a. Row loop over
    // [top..bottom] with the per-row left > right skip @ 0x100731755;
    // vertex = stride * z + left @ 0x10073175e..0x100731768.
    for (Sint32 z = rect.top; z <= rect.bottom; ++z) {
        if (rect.left > rect.right) continue;
        Sint32 vertex = mVertexCountX * z + rect.left;
        for (Sint32 x = rect.left; x <= rect.right; ++x, ++vertex) {
            if (vertex < 0 ||
                static_cast<std::size_t>(vertex) >= mLandTypes.size()) {
                continue; // model guard (the binary has none)
            }
            auto& slots = mLandTypes[static_cast<std::size_t>(vertex)];
            // Sum the filled slot prefix @ 0x10073177b..0x10073179c — the
            // scan exits at the FIRST empty slot (je 0x10073179e jumps past
            // the sum loop), so an empty slot-0 vertex sums 0.
            Uint32 sum = 0;
            Sint32 filled = 0;
            for (; filled < 3; ++filled) {
                if (slots[static_cast<std::size_t>(filled)].type == 0) break;
                sum += slots[static_cast<std::size_t>(filled)].value;
            }
            // Renormalize when sum >= 251 @ 0x10073179e..0x1007317d3:
            // value = value * 250 / sum (imul 0xfa, div ecx — unsigned),
            // also stopping at the first empty slot.
            if (sum >= 0xfbu) {
                for (Sint32 i = 0; i < filled; ++i) {
                    auto& slot = slots[static_cast<std::size_t>(i)];
                    slot.value = static_cast<uint16_t>(
                        (static_cast<Uint32>(slot.value) * 0xfau) / sum);
                }
            }
        }
    }
}

// ---- Water proximity / moisture / clear ----

bool cSTETerrain::AreSeaVertices(const SC4RectI& rect) const noexcept {
    // Exact cSTETerrain::AreSeaVertices @ 0x100733b16. Unsigned bounds gates
    // (jbe, fail when vertexCountX/Z <= the rect edge):
    const Uint32 left = static_cast<Uint32>(rect.left);
    const Uint32 top = static_cast<Uint32>(rect.top);
    const Uint32 right = static_cast<Uint32>(rect.right);
    const Uint32 bottom = static_cast<Uint32>(rect.bottom);
    if (static_cast<Uint32>(mVertexCountX) <= left) return false;   // 0x100733b26
    if (static_cast<Uint32>(mVertexCountZ) <= top) return false;    // 0x100733b32
    if (static_cast<Uint32>(mVertexCountX) <= right) return false;  // 0x100733b3b
    if (static_cast<Uint32>(mVertexCountZ) <= bottom) return false; // 0x100733b44
    // BINARY ODDITY @ 0x100733b46..0x100733b4b: al is set to 1 BEFORE the
    // top > bottom check, so an EMPTY rect (top > bottom) returns TRUE
    // ("all vertices are sea" holds vacuously). A left > right rect skips
    // every row the same way and also returns true.
    if (top > bottom) return true;
    for (Sint32 z = rect.top; z <= rect.bottom; ++z) {
        if (rect.left > rect.right) break; // row gate jg -> row tail @ 0x100733b5b
        for (Sint32 x = rect.left; x <= rect.right; ++x) {
            const Sint32 v = mVertexCountX * z + x;
            // Continue while seaLevel > altitude (jbe -> false @ 0x100733b80).
            if (v < 0 || static_cast<std::size_t>(v) >= mAltitude.size()) {
                return false; // model guard (unreachable behind the gates)
            }
            if (!(mSeaLevel > mAltitude[static_cast<std::size_t>(v)])) {
                return false;
            }
        }
    }
    return true;
}

void cSTETerrain::ComputeWaterProximityValues(const SC4RectI& rect) {
    // Exact cSTETerrain::ComputeWaterProximityValues @ 0x100733c62.
    // Union the rect into the +0xcc dirty rect @ 0x100733c6f..0x100733c84
    // (UnionRect with itself as the out).
    mWaterProximityRect = UnionRectValues(mWaterProximityRect, rect);
    // Area gate @ 0x100733c84..0x100733caa: (right-left)*(bottom-top) of the
    // UNION rect, both extents ZERO-extended (pmovzxdq) and multiplied
    // unsigned; < 100 (cmp 0x64, jl) -> no recompute. NOTE the zero
    // extension: a wrapped/negative extent becomes huge and passes.
    const Uint32 w = static_cast<Uint32>(
        mWaterProximityRect.right - mWaterProximityRect.left);
    const Uint32 h = static_cast<Uint32>(
        mWaterProximityRect.bottom - mWaterProximityRect.top);
    if (w * h < 100u) return;
    mWaterProximityFieldC8 = 0;               // +0xc8 @ 0x100733cb2
    mWaterProximityRect = kEmptyRect;         // +0xcc @ 0x100733cb8..0x100733cca
    mNumberOfWaterVertices = 0;               // +0x108 @ 0x100733cdb
    const Uint8 farProximity =
        static_cast<Uint8>(MaxBeachWidthTunable() + 2); // r10b @ 0x100733cd4..0x100733cd7
    if (mVertexCountZ <= 0) return;          // test r9d @ 0x100733ce5
    for (Sint32 z = 0; z < mVertexCountZ; ++z) {
        if (mVertexCountX <= 0) break;       // test eax per row @ 0x100733cf9
        for (Sint32 x = 0; x < mVertexCountX; ++x) {
            const Float32 depth =
                mSurfaceWater.depth[static_cast<std::size_t>(z * mVertexCountX + x)];
            if (depth > 0.0f) { // ucomiss xmm0(0), depth; jbe
                mWaterProximity[static_cast<std::size_t>(z)]
                              [static_cast<std::size_t>(x)] = 0u; // 0x100733d2e
                ++mNumberOfWaterVertices;                          // 0x100733d32
            } else {
                mWaterProximity[static_cast<std::size_t>(z)]
                              [static_cast<std::size_t>(x)] = farProximity; // 0x100733d3a
            }
        }
    }
    // FloodFillDistance gate @ 0x100733d62..0x100733d75: al = -(waterCount
    // <= 0) and the call only happens when water exists.
    if (mNumberOfWaterVertices <= 0) return;
    // cSC4MapUtilities::FloodFillDistance(map, kMaxBeachWidth, 0) @
    // 0x1000d82b4 — clean-room BFS model (see FloodFillDistanceCleanRoom).
    FloodFillDistanceCleanRoom(mWaterProximity, MaxBeachWidthTunable());
}

void cSTETerrain::UpdateMoistureValues(const SC4RectI& rect) noexcept {
    // Exact cSTETerrain::UpdateMoistureValues @ 0x100733d98.
    if (mMoistureSource == nullptr) return; // test rdi,rdi; je @ 0x100733d9f
    // The four rect ints converted as floats * {16,16,16,16} (@ 0x100a29bd0)
    // and truncated back (cvtdq2ps/mulps/cvttps2dq @ 0x100733dac..0x100733dc1),
    // then vtable+0x80 with the converted rect.
    const Float32 kMoistureRectCellWidth = 16.0f;
    Sint32 worldRect[4];
    const Sint32 src[4] = {rect.left, rect.top, rect.right, rect.bottom};
    for (int i = 0; i < 4; ++i) {
        worldRect[i] = static_cast<Sint32>(
            static_cast<Float32>(src[i]) * kMoistureRectCellWidth);
    }
    mMoistureSource->UpdateRect(worldRect);
}

void cSTETerrain::ClearSurfaceTextures(const SC4RectI& rect) noexcept {
    // Exact cSTETerrain::ClearSurfaceTextures @ 0x100733dd4. Row loop over
    // [top..bottom] (cmp/jg @ 0x100733de0) with the per-row left > right
    // skip (@ 0x100733deb); per vertex, the three slot TYPE words at record
    // +0x16/+0x1a/+0x1e are zeroed (three word stores, rdx = 0..2
    // @ 0x100733e0a..0x100733e21).
    for (Sint32 z = rect.top; z <= rect.bottom; ++z) {
        if (rect.left > rect.right) continue;
        Sint32 vertex = mVertexCountX * z + rect.left;
        for (Sint32 x = rect.left; x <= rect.right; ++x, ++vertex) {
            if (vertex < 0 ||
                static_cast<std::size_t>(vertex) >= mLandTypes.size()) {
                continue; // model guard (the binary has none)
            }
            auto& slots = mLandTypes[static_cast<std::size_t>(vertex)];
            for (Sint32 i = 0; i < 3; ++i) {
                slots[static_cast<std::size_t>(i)].type = 0;
            }
        }
    }
}

void cSTETerrain::AssignSurfaceTexturesAndColors(SC4RectI& rect) {
    // Exact cSTETerrain::AssignSurfaceTexturesAndColors @ 0x100733bc8 — the
    // post-modification refresh, in the binary's exact call order:
    ComputeWaterProximityValues(rect); // @ 0x100733bd5
    // PadRectangle(rect, snMaxSurfaceTextureSpread[strength]) @
    // 0x100733bda..0x100733bf1 — the strength index is +0xdc and the rect is
    // padded IN PLACE (passed by reference).
    PadRectangle(rect, TableAt(MaxSurfaceTextureSpread(), mBrushStrengthIndex));
    UpdateMoistureValues(rect);  // @ 0x100733bf6..0x100733c07
    ClearSurfaceTextures(rect);  // @ 0x100733c0c..0x100733c13 tail
    // MarkTextureSpanInfoDirty on the +0x110 view, gated on the +0x118 byte
    // (cmp byte ptr [rbx+0x118], 0; je @ 0x100733c0c/0x100733c13). Args:
    // lo = (top > 0) ? top - 1 : 0 (lea/cmovg @ 0x100733c15..0x100733c26),
    // hi = min(bottom + 1, maxVertexIndexZ) (inc edx, cmova vs +0x4c
    // @ 0x100733c29..0x100733c2e). cSTETerrainView3D is not reconstructed —
    // clean-room fallback (not authentic): recorded observation.
    if (mTextureSpanInfoDirtyGate) {
        const Sint32 lo = (rect.top > 0) ? rect.top - 1 : 0;
        Sint32 hi = rect.bottom + 1;
        if (static_cast<Uint32>(hi) > static_cast<Uint32>(mMaxVertexIndexZ)) {
            hi = mMaxVertexIndexZ;
        }
        TerrainView3DObservation& view = View3DObservation();
        ++view.markTextureSpanInfoDirtyCalls;
        view.markTextureSpanInfoDirtyLastLo = lo;
        view.markTextureSpanInfoDirtyLastHi = hi;
    }
    DecideSurfaceCover(rect); // @ 0x100733c3d..0x100733c48
    SprayVolcanicAsh(rect);  // @ 0x100733c48..0x100733c53
    AdjustLandTypeRatios(rect); // tail jmp @ 0x100733c59
}

void cSTETerrain::SprayVolcanicAsh(const SC4RectI& rect) noexcept {
    // Exact cSTETerrain::SprayVolcanicAsh @ 0x100734724.
    if (mVolcanicAshRegions.empty()) return; // list size +0x240 @ 0x10073473c
    auto& textures = SurfaceCoverTextures();
    const auto it = textures.find(kVolcanicAshTextureId); // kVolcanicAshTexture
    // = 0xe0 @ 0x100a29c90 (find @ 0x10073474a..0x100734764)
    if (it == textures.end()) return;
    const TerrainSurfaceCoverInfo& info = it->second;
    const Sint32 strength = mBrushStrengthIndex; // +0xdc
    for (VolcanicAshRegion& region : mVolcanicAshRegions) {
        // Spread from the surface-cover entry (node+0xc0+4*s @ 0x1007347a8..0x1007347b3).
        const Sint32 spread = TableAt(info.spread, strength);
        // Expand the PASSED rect by the spread, clamping
        // (@ 0x1007347ba..0x10073480a): left/top - spread (jns -> 0),
        // right/bottom + spread (jle vs cellCount).
        Sint32 left = rect.left - spread;
        if (left < 0) left = 0;
        Sint32 top = rect.top - spread;
        if (top < 0) top = 0;
        Sint32 right = rect.right + spread;
        if (right > mCellCountX) right = mCellCountX;
        Sint32 bottom = rect.bottom + spread;
        if (bottom > mCellCountZ) bottom = mCellCountZ;
        SC4RectI work;
        // Intersect with the region rect @ 0x10073480a..0x10073481b; no
        // intersection -> next region.
        if (!IntersectRectValues(work, SC4RectI{left, top, right, bottom},
                                 region.rect)) {
            continue;
        }
        // When the intersection covers the WHOLE region rect, the node's
        // +0x24 dword (the high half of the zero-initialized +0x20 qword —
        // the low dword is the region age) is cleared
        // (@ 0x100734828..0x100734853).
        if (work.left == region.rect.left && work.top == region.rect.top &&
            work.right == region.rect.right &&
            work.bottom == region.rect.bottom) {
            region.field20 &= 0xffffffffLL;
        }
        if (work.top > work.bottom) continue; // r8d vs [rbp-0x2c] @ 0x10073485a
        // Spray factor = (1 - age / kVolcanicAshLifeTime) * entry multiplier
        // (@ 0x100734864..0x10073489a): age is the node+0x20 DWORD
        // (cvtsi2ss), lifetime 10000000 int @ 0x100b17684, the multiplier is
        // the entry's +0x18 float (mulss 0x18(%rax) @ 0x100734895).
        const Float32 age = static_cast<Float32>(
            static_cast<Sint32>(region.field20 & 0xffffffffLL));
        const Float32 factor = (kOneF - age / static_cast<Float32>(VolcanicAshLifeTimeTunable())) *
                               info.multiplier;
        for (Sint32 z = work.top; z <= work.bottom; ++z) {
            for (Sint32 x = work.left; x <= work.right; ++x) {
                const Sint32 vertex = mVertexCountX * z + x;
                if (vertex < 0 ||
                    static_cast<std::size_t>(vertex) >= mVertexFlags.size()) {
                    continue; // model guard (the binary has none)
                }
                // Only ash-marked vertices: the record's +0x23 byte bit 0x4
                // (= flags-word bit 0x400, set by BrushFnVolcanoAsh
                // @ 0x100733350) @ 0x1007348c8..0x1007348d8.
                if ((mVertexFlags[static_cast<std::size_t>(vertex)] & 0x0400u) == 0u) {
                    continue;
                }
                // Influence rect around (x, z) clamped to the grid
                // (@ 0x1007348e5..0x100734931), intersected with the PASSED
                // rect (@ 0x100734934..0x100734946).
                Sint32 l2 = x - spread;
                if (l2 < 0) l2 = 0;
                Sint32 t2 = z - spread;
                if (t2 < 0) t2 = 0;
                Sint32 r2 = x + spread;
                if (r2 > mCellCountX) r2 = mCellCountX;
                Sint32 b2 = z + spread;
                if (b2 > mCellCountZ) b2 = mCellCountZ;
                SC4RectI influence;
                if (!IntersectRectValues(influence, SC4RectI{l2, t2, r2, b2}, rect)) {
                    continue;
                }
                if (influence.top > influence.bottom) continue; // @ 0x100734953
                for (Sint32 z2 = influence.top; z2 <= influence.bottom; ++z2) {
                    if (influence.left > influence.right) break; // @ 0x100734963
                    // dz^2 as float, computed once per z2 row
                    // (@ 0x100734974..0x100734984).
                    const Float32 dzSq = static_cast<Float32>((z2 - z) * (z2 - z));
                    for (Sint32 x2 = influence.left; x2 <= influence.right; ++x2) {
                        // distSq = dx^2 + dz^2, sqrt, * 16.0f
                        // (@ 0x100734989..0x1007349a1; the 16.0f is the cell
                        // width @ 0x100a16904 — vertex -> world meters).
                        const Float32 dx = static_cast<Float32>(x2 - x);
                        const Float32 dist =
                            std::sqrt(dx * dx + dzSq) * kCellWidthF;
                        // value = curve(dist) * factor
                        // (@ 0x1007349a9..0x1007349c2).
                        const Float32 value =
                            EvaluateSurfaceCoverCurve(info, strength, dist) * factor;
                        // Add only when value > 0 (ucomiss vs 0.0f @ 0x100a15cd8,
                        // jbe @ 0x1007349c7..0x1007349ce), with the volcanic
                        // ash surface-cover type 0xe0 (@ 0x1007349d4).
                        if (!(value > kAshGateZero)) continue;
                        AddLandTypeAtVertex(static_cast<Uint32>(mVertexCountX * z2 + x2),
                                            static_cast<uint16_t>(kVolcanicAshTextureId),
                                            value);
                    }
                }
            }
        }
    }
}

void cSTETerrain::DecideSurfaceCover(const SC4RectI& rect) noexcept {
    // Exact cSTETerrain::DecideSurfaceCover @ 0x100733e48 — the core
    // decision pass. All constants below were re-verified against fresh
    // otool dumps (the workpack has a known transcription-error history).
    const Sint32 terrainType = mTerrainType; // movsxd [rbx+0x24] @ 0x100733e5f
    const Uint32 lowCliffId = TableAtU(LowCliffTexResIds(), terrainType);
    // sLowCliffTexResIds @ 0x100b71ccc read @ 0x100733e63..0x100733e6d; the
    // land-type argument uses the LOW 16 BITS (movzx @ 0x100733f81).
    const uint16_t lowCliffId16 = static_cast<uint16_t>(lowCliffId);
    // cSTETerrainView3D::ChangeTerrainType(terrainType) @ 0x1007467ca
    // (called with rdi = terrainType @ 0x100733eca) and
    // SetUpTexturesForCurrentTerrainTypeAndZoom(strengthIndex) @ 0x10074688c
    // (@ 0x100733ecf..0x100733ed5). cSTETerrainView3D is not reconstructed —
    // clean-room fallback (not authentic): recorded observations.
    {
        TerrainView3DObservation& view = View3DObservation();
        ++view.changeTerrainTypeCalls;
        view.changeTerrainTypeLastArg = terrainType;
        ++view.setUpTexturesCalls;
        view.setUpTexturesLastArg = mBrushStrengthIndex;
    }
    const Sint32 strength = mBrushStrengthIndex; // +0xdc
    // Spread-expand the rect @ 0x100733eda..0x100733f22 by
    // snMaxSurfaceTextureSpread[strength] (@ 0x100b71d10 read
    // @ 0x100733eeb): left/top - spread clamped at 0 (cmovs), right/bottom +
    // spread clamped at cellCountX/Z (cmovg).
    const Sint32 spread = TableAt(MaxSurfaceTextureSpread(), strength);
    Sint32 left = rect.left - spread;
    if (left < 0) left = 0;
    Sint32 top = rect.top - spread;
    if (top < 0) top = 0;
    Sint32 right = rect.right + spread;
    if (right > mCellCountX) right = mCellCountX;
    Sint32 bottom = rect.bottom + spread;
    if (bottom > mCellCountZ) bottom = mCellCountZ;
    if (top > bottom) return; // @ 0x100733f25
    // beachAlt = seaLevel + kMaxBeachAltitude (7.0f @ 0x100b175e8,
    // addss @ 0x100733e97..0x100733ea7).
    const Float32 beachAlt = mSeaLevel + MaxBeachAltitudeTunable();
    // tempRange = (kMaxTerrainHeight - beachAlt) * kTemperatureRangeFactor
    // (@ 0x100733eac loads kMaxTerrainHeight @ 0x100b1763c — the shared
    // tunable; 0x100733f2b..0x100733f3a subtracts/multiplies; the factor @
    // 0x100b71db0 is the denormal-default tunable).
    const Float32 tempRange =
        (MaxTerrainHeightTunable() - beachAlt) * TemperatureRangeFactor();
    // Row loop [top..bottom] (post-checked @ 0x1007346f9..0x10073470c), with
    // the per-row left > right skip @ 0x100733f92..0x100733f98.
    for (Sint32 z = top; z <= bottom; ++z) {
        if (left > right) continue;
        const Float32 worldZ = static_cast<Float32>(z) * kCellWidthF; // @ 0x100733fa5..0x100733fb4
        Sint32 vertex = mVertexCountX * z + left;                    // @ 0x100733fbc..0x100733fc6
        for (Sint32 x = left; x <= right; ++x, ++vertex) {
            if (vertex < 0 ||
                static_cast<std::size_t>(vertex) >= mAltitude.size()) {
                continue; // model guard (the binary has none)
            }
            const std::size_t v = static_cast<std::size_t>(vertex);
            const Float32 depth = mSurfaceWater.depth[v]; // depth grid @ 0x100733fd0..0x100733fdd
            // Depth gate: skip when depth > kMaxWaterDepthForDeepSeaBedTexture
            // (30.0f @ 0x100b17638, ja @ 0x100733fe2..0x100733fe9).
            if (depth > MaxWaterDepthForDeepSeaBedTextureTunable()) continue;
            const Float32 altitude = mAltitude[v];
            // Cliff bit: record+0x22 byte & 0x8 (flags-word bit 0x0008,
            // test @ 0x100733ff6) -> the low-cliff texture is added and the
            // vertex skips the whole common tail (jmp @ 0x10073409a..0x1007340ba).
            if ((mVertexFlags[v] & 0x0008u) != 0u) {
                AddLandTypeAtVertex(static_cast<Uint32>(vertex), lowCliffId16,
                                    kLowCliffAddValue); // 50.0f @ 0x100a16e84
                continue;
            }
            Uint32 texId;
            Uint32 packed;
            bool allowWater; // the [+0x30] flag of the asm
            bool allowLand;  // the [+0x2c] flag of the asm
            if (depth > 0.0f) { // ucomiss/jbe @ 0x100734001/0x100734004
                // Water path @ 0x10073400a..0x100734095.
                // texId = 0x13 + (depth < 15.0f) via adc @ 0x10073400a..0x10073401b
                // (kMinWaterDepthForDeepSeaBedTexture @ 0x100b17634).
                texId = (depth < MinWaterDepthForDeepSeaBedTextureTunable()) ? 20u : 19u;
                // packed = (variant << 12) | texId — OR @ 0x100734080..0x100734086.
                packed = (SurfaceCoverTextureVariant(vertex, texId, strength,
                                                     mVertexCountX)
                          << 12u) |
                         texId;
                allowWater = true;  // [+0x30] = 1 @ 0x100734089
                allowLand = false; // [+0x2c] = 0 @ 0x10073408e
            } else {
                // Land path @ 0x1007340bf: read the proximity byte at (x, z)
                // from the +0x200 map (row vector begin at +24*z
                // @ 0x1007340bf..0x1007340d2).
                const Uint8 prox =
                    mWaterProximity[static_cast<std::size_t>(z)]
                                  [static_cast<std::size_t>(x)];
                // Beach gate: proximity <= kMaxBeachWidth (byte cmp, ja ->
                // inland @ 0x1007340d6..0x1007340e1; kMaxBeachWidth = 4
                // @ 0x100b175e4) AND beachAlt >= altitude (jb -> inland
                // @ 0x1007340e7..0x1007340ef).
                if (!(prox > MaxBeachWidthTunable()) && !(beachAlt < altitude)) {
                    // Beach path @ 0x1007340f5..0x10073425c.
                    texId = TableAtU(BeachTexResIds(), terrainType); // @ 0x1007340f5..0x100734104
                    // packed = (variant << 12) + texId — ADD @ 0x100734169..0x10073416f
                    // (ODDITY: the water path packs with OR, the beach and
                    // temperature paths with ADD).
                    packed = (SurfaceCoverTextureVariant(vertex, texId, strength,
                                                         mVertexCountX)
                              << 12u) +
                             texId;
                    allowWater = true; // [+0x30] = 1 @ 0x1007341a5 / 0x100734255
                    allowLand = true;  // [+0x2c] = 1 @ 0x10073425c
                    // The CENTER vertex gets a land type of 1.0f (quantized)
                    // only when it lies inside the ORIGINAL rect
                    // (@ 0x100734172..0x1007341d0; value 1.0f @ 0x100a15090).
                    if (x >= rect.left && z >= rect.top && x <= rect.right &&
                        z <= rect.bottom) {
                        AddLandTypeAtVertex(static_cast<Uint32>(vertex),
                                            static_cast<uint16_t>(packed),
                                            kBeachAddValue);
                    }
                } else {
                    // Inland temperature/moisture path @ 0x1007341d5..0x10073430a.
                    const Float32 worldX =
                        static_cast<Float32>(x) * kCellWidthF; // @ 0x1007341d5..0x1007341dd
                    Sint32 tempCode = 31; // r15d = 0x1f @ 0x1007341e5
                    if (beachAlt < altitude) { // ucomiss/jae @ 0x1007341eb..0x1007341f3
                        // normalized = ((altitude - seaLevel) / tempRange) *
                        // 30.0f (@ 0x1007341f5..0x100734203; scale
                        // @ 0x100a1969c), then rounded to the nearest even
                        // integer via the same -0.5/12582912.0f magic as
                        // FloatToVertexIndex (@ 0x10073420b..0x10073423a).
                        const Float32 normalized =
                            ((altitude - mSeaLevel) / tempRange) * kTempScale;
                        const Sint32 code = FloatToVertexIndex(normalized);
                        // Clamped inversion @ 0x10073423d..0x100734264:
                        // code > 31 -> 0 (js on 31-code); code < 0 -> 31
                        // (cmovs); else 31 - code.
                        if (code > 31) {
                            tempCode = 0;
                        } else if (code < 0) {
                            tempCode = 31;
                        } else {
                            tempCode = 31 - code;
                        }
                    }
                    // Moisture code from the +0x140 object's vtable+0x98 with
                    // (worldX, worldZ) @ 0x100734267..0x100734289 — the binary
                    // does NOT null-check here (unlike UpdateMoistureValues);
                    // the model guards. The code is UNCLAMPED in the binary
                    // (shl 4/add @ 0x100734282..0x100734286); the map read
                    // below is guarded by the model instead.
                    const Sint32 moisture =
                        mMoistureSource != nullptr
                            ? mMoistureSource->ValueAt(worldX, worldZ)
                            : 0;
                    const Sint32 combined = (tempCode << 4) + moisture;
                    // texId = sSurfaceCoverMap[terrainType*512 + combined]
                    // (base = map + terrainType<<11 bytes, movsxd read
                    // @ 0x10073428c..0x100734297).
                    texId = 0u;
                    {
                        auto& scMap = SurfaceCoverMap();
                        const Sint64 idx =
                            static_cast<Sint64>(terrainType) * 512 + combined;
                        if (idx >= 0 &&
                            static_cast<std::uint64_t>(idx) < scMap.size()) {
                            texId = static_cast<Uint32>(
                                scMap[static_cast<std::size_t>(idx)]);
                        }
                    }
                    // packed = (variant << 12) + texId — ADD @ 0x1007342f7..0x100734302.
                    packed = (SurfaceCoverTextureVariant(vertex, texId, strength,
                                                         mVertexCountX)
                              << 12u) +
                             texId;
                    allowWater = false; // [+0x30] = 0 @ 0x10073430a
                    allowLand = true;   // [+0x2c] = 1 @ 0x100734305
                }
            }
            // Common tail @ 0x100734311: look the texture id up in
            // sSurfaceCoverTextures (find @ 0x100734311..0x100734328); a
            // missing entry skips the vertex entirely.
            const auto it = SurfaceCoverTextures().find(texId);
            if (it == SurfaceCoverTextures().end()) continue;
            const TerrainSurfaceCoverInfo& info = it->second;
            // Spread (node+0xc0+4*s) @ 0x100734335..0x100734340.
            const Sint32 s2 = TableAt(info.spread, strength);
            // Influence rect around (x, z), clamped to the grid
            // (@ 0x100734347..0x10073438a), intersected with the ORIGINAL
            // rect (IntersectRect @ 0x100734390..0x1007343c3; the callers
            // re-check top > bottom @ 0x1007343c9..0x1007343d7).
            SC4RectI influence;
            influence.left = x - s2;
            if (influence.left < 0) influence.left = 0;
            influence.top = z - s2;
            if (influence.top < 0) influence.top = 0;
            influence.right = x + s2;
            if (influence.right > mCellCountX) influence.right = mCellCountX;
            influence.bottom = z + s2;
            if (influence.bottom > mCellCountZ) influence.bottom = mCellCountZ;
            SC4RectI out;
            if (!IntersectRectValues(out, influence, rect)) continue;
            if (out.top > out.bottom) continue;
            if (out.left > out.right) continue; // row gate @ 0x1007344d4
            // maxDistSq = curveParam[strength]^2 * 0.00390625 (1/256)
            // (@ 0x1007343e4..0x10073440f; the param lives at node+0xd4+4*s).
            const Float32 param = TableAtF(info.curveParam, strength);
            const Float32 maxDistSq = (param * param) * kKernelScale;
            // Incremental squared distance: row/col accumulators start at
            // 2*d0+1 and step by 2 (@ 0x100734414..0x100734478; the ROW
            // accumulator is computed through a double round-trip
            // @ 0x100734449..0x100734464, the column one in plain float
            // @ 0x100734431..0x100734441), the row base is the int square sum
            // converted once (@ 0x100734469..0x100734478).
            const Sint32 dx0 = out.left - x;
            const Sint32 dz0 = out.top - z;
            const Float32 colStep =
                (static_cast<Float32>(dx0) + static_cast<Float32>(dx0)) + kOneF;
            Float32 rowStep = static_cast<Float32>(
                static_cast<Float64>(static_cast<Float32>(dz0)) +
                static_cast<Float64>(static_cast<Float32>(dz0)) + 1.0);
            Float32 rowBase = static_cast<Float32>(dx0 * dx0 + dz0 * dz0);
            // Row base vertex [rbp-0x54] @ 0x10073447d..0x100734492: it is
            // incremented by the stride at each row head (@ 0x1007344b9..0x1007344c0)
            // and RELOADED into the column vertex counter at every row start
            // (@ 0x1007344dd) — the column loop's per-column inc never carries
            // across rows.
            Sint32 rowVertexBase = mVertexCountX * out.top + out.left;
            for (Sint32 z2 = out.top; z2 <= out.bottom; ++z2) {
                if (z2 != out.top) {
                    rowBase = rowBase + rowStep; // @ 0x10073449d..0x1007344a7
                    rowStep = rowStep + kDistSqStep; // @ 0x1007344b0..0x1007344b4
                    rowVertexBase += mVertexCountX;  // @ 0x1007344b9..0x1007344c0
                }
                if (out.left > out.right) continue;
                Float32 distSq = rowBase;   // xmm5 @ 0x10073451a
                Float32 colAccum = colStep; // xmm3 @ 0x10073451f
                Sint32 rowVertex = rowVertexBase; // reload @ 0x1007344dd
                for (Sint32 x2 = out.left; x2 <= out.right;
                     ++x2, ++rowVertex) {
                    // Every gate below breaks to the accumulator tail — in the
                    // binary all the skips jump to 0x100734697, so the
                    // incremental accumulators keep running for skipped
                    // columns too.
                    do {
                        if (rowVertex < 0 ||
                            static_cast<std::size_t>(rowVertex) >=
                                mAltitude.size() ||
                            static_cast<std::size_t>(z2) >=
                                mWaterProximity.size() ||
                            static_cast<std::size_t>(x2) >=
                                mWaterProximity[static_cast<std::size_t>(z2)]
                                               .size()) {
                            break; // model guard (the binary has none)
                        }
                        const std::size_t sv = static_cast<std::size_t>(rowVertex);
                        const Float32 d = mSurfaceWater.depth[sv];
                        // Gate 1 @ 0x100734527..0x100734537: pass iff
                        // (depth <= 0) OR allowWater (setae/orb/cmp al,1).
                        if (!((0.0f >= d) || allowWater)) break;
                        // Gate 2 @ 0x10073453d..0x10073454a: pass iff
                        // (depth > 0) OR allowLand (setb/orb/cmp cl,1).
                        if (!((0.0f < d) || allowLand)) break;
                        const Float32 alt2 = mAltitude[sv];
                        const Uint8 prox2 =
                            mWaterProximity[static_cast<std::size_t>(z2)]
                                          [static_cast<std::size_t>(x2)];
                        if (beachAlt >= alt2) {
                            // Low-vertex case @ 0x100734550..0x100734586:
                            // require maxDistSq > distSq (jbe skip), then the
                            // BINARY ODDITY gate (seta/orb/je
                            // @ 0x10073456b..0x100734586): a LOW LAND vertex
                            // NEAR water (prox <= 4, not water) is SKIPPED —
                            // only water or far-from-water low vertices
                            // receive this center's spread.
                            if (!(maxDistSq > distSq)) break;
                            const bool proxTooFar = prox2 > MaxBeachWidthTunable();
                            const bool isWater = 0.0f < d;
                            if (!(proxTooFar || isWater)) break;
                        } else {
                            // High-vertex case @ 0x10073455c..0x100734564:
                            // just the distance gate.
                            if (!(maxDistSq > distSq)) break;
                        }
                        // Distance kernel @ 0x10073458c..0x100734605, exact op
                        // order. With C3 = 3.0f the identity evaluates to the
                        // plain distance; |distSq| < 1e-5 short-circuits to 0
                        // (kernel input 0). The result scales by the cell
                        // width @ 0x100734609.
                        Float32 kernel = 0.0f;
                        if (!(kKernelEps > std::fabs(distSq))) {
                            const Float32 dist = std::sqrt(distSq); // sqrtss
                            const Float32 inv = kOneF / dist;       // divss 1/dist
                            const Float32 x0 = (inv * inv) * distSq;
                            Float32 t = kKernelC3 - x0;
                            t = t * inv;
                            t = t * kKernelHalf;
                            const Float32 t2d2 = (t * t) * distSq;
                            kernel = (kKernelC3 - t2d2) * distSq;
                            kernel = kernel * t;
                            kernel = kernel * kKernelHalf;
                        }
                        const Float32 curveInput = kernel * kCellWidthF;
                        // GetOutputValue on the per-strength response curve
                        // (@ 0x100734611..0x10073462f; this @ node+0x10+32*s)
                        // scaled by the entry multiplier (+0x18)
                        // (@ 0x100734643); value > 0 gates the add
                        // (ucomiss/jbe @ 0x100734648..0x100734663).
                        const Float32 value =
                            EvaluateSurfaceCoverCurve(info, strength, curveInput) *
                            info.multiplier;
                        if (!(value > kAshGateZero)) break;
                        AddLandTypeAtVertex(
                            static_cast<Uint32>(rowVertex),
                            static_cast<uint16_t>(packed), // movzx word @ 0x100734665
                            value);                       // @ 0x10073466f
                    } while (false);
                    // Column tail @ 0x100734697..0x1007346b9.
                    distSq = distSq + colAccum;
                    colAccum = colAccum + kDistSqStep;
                }
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Direct grid access (verify suite / loader wiring)
// ---------------------------------------------------------------------------

void cSTETerrain::SetAltitude(Sint32 vertexIndex, Float32 altitude) {
    if (vertexIndex >= 0 && static_cast<std::size_t>(vertexIndex) < mAltitude.size()) {
        mAltitude[static_cast<std::size_t>(vertexIndex)] = altitude;
    }
}

void cSTETerrain::SetWaterDepth(Sint32 vertexIndex, Float32 depth) {
    if (vertexIndex >= 0 && static_cast<std::size_t>(vertexIndex) < mSurfaceWater.depth.size()) {
        mSurfaceWater.depth[static_cast<std::size_t>(vertexIndex)] = depth;
    }
}

void cSTETerrain::SetMaxVertexIndexBounds(Sint32 maxX, Sint32 maxZ) {
    mMaxVertexIndexX = maxX;
    mMaxVertexIndexZ = maxZ;
}

Float32 cSTETerrain::AltitudeAt(Sint32 vertexIndex) const {
    if (vertexIndex >= 0 && static_cast<std::size_t>(vertexIndex) < mAltitude.size()) {
        return mAltitude[static_cast<std::size_t>(vertexIndex)];
    }
    return 0.0f;
}

// ---------------------------------------------------------------------------
// Shore-cliff & normal-assignment cluster (M3 terrain wave)
// ---------------------------------------------------------------------------

// Runtime-filled tuning constants (settable statics with the shipped
// byte-exact defaults; see the declarations in terrain.h). Shipped values
// verified by fresh byte dumps (file offset = vmaddr - 0x100000000).

Float32& cSTETerrain::MinSeaLevelTunable() {                 // 0x100b175d4
    static Float32 v = -10.1f;                                 // 0xc121999a
    return v;
}
Float32& cSTETerrain::MaxSeaLevelTunable() {                 // 0x100b175d8
    static Float32 v = 200.1f;                                 // 0x4348199a
    return v;
}
Float32& cSTETerrain::SeaRaiseLowerAmountTunable() {         // 0x100b175dc
    static Float32 v = 20.0f;                                  // 0x41a00000
    return v;
}
Sint32& cSTETerrain::MinShoreCliffSizeTunable() {            // 0x100b1766c
    static Sint32 v = 4;
    return v;
}
Sint32& cSTETerrain::MaxShoreCliffSizeTunable() {            // 0x100b17670
    static Sint32 v = 70;                                      // 0x46
    return v;
}
Float32& cSTETerrain::MinShoreCliffCandidateHtSumTunable() { // 0x100b17674
    static Float32 v = 15.0f;                                  // 0x41700000
    return v;
}
Float32& cSTETerrain::MinShoreCliffHtTunable() {             // 0x100b17678
    static Float32 v = 40.1f;                                  // 0x42206666
    return v;
}
Float32& cSTETerrain::MaxShoreCliffHtTunable() {             // 0x100b1767c
    static Float32 v = 50.1f;                                  // 0x42486666
    return v;
}
Float32& cSTETerrain::MaxNormalYForCliffTunable() {          // 0x100b17680
    static Float32 v = 0.3f;                                   // 0x3e99999a
    return v;
}
Float32& cSTETerrain::WindErosionFactorTunable() {           // 0x100b71d28
    static Float32 v = std::bit_cast<Float32>(0x00000001u);    // shipped bits
    return v;
}

TerrainViewUtilitiesModel& cSTETerrain::ViewUtilitiesModel() {
    // clean-room fallback (not authentic): SL::spViewUtilities @ 0x100b214d8
    // and SC4ViewUtilities::GetVisibleEdges(int&, int&) @ 0x10017376a are
    // not reconstructed; the pair is settable for the verify suite.
    static TerrainViewUtilitiesModel m;
    return m;
}

namespace {

// cSTETerrain::kXIncrement @ 0x100a29c10 / kZIncrement @ 0x100a29c40 —
// 8-entry int arrays, one entry per neighbor direction 0=W, 1=NW, 2=N,
// 3=NE, 4=E, 5=SE, 6=S, 7=SW (dword dump 0xa29c10/0xa29c40; the direction
// order matches the this+0xa4 table documented at NeighborVertexOffset).
constexpr Sint32 kXIncrementTable[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
constexpr Sint32 kZIncrementTable[8] = {0, -1, -1, -1, 0, 1, 1, 1};

// cRZRandom::RandomDoubleRangeGaussian(double, double) @ 0x10003d69e.
//   g = RandomDoubleGaussian()                                  @ 0x10003d6b0
//   t = (6.0 > g + 3.0) ? (g + 3.0) : 5.9998860...             @ 0x10003d6b5..0x10003d6da
//   t = max(0.0, t) * (1.0/6.0)                                 @ 0x10003d6de..0x10003d6e6
//   return lo + (hi - lo) * t                                   @ 0x10003d6ee..0x10003d700
// Constants: 3.0 @ 0x100a15620, 6.0 @ 0x100a15628,
// 0x4017FFE5C91D14E4 @ 0x100a15630, 1/6 @ 0x100a15638 (fresh byte dumps).
inline Float64 RandomDoubleRangeGaussian(RZRandomState& rng, Float64 lo,
                                         Float64 hi) noexcept {
    const Float64 g = RandomDoubleGaussian(rng);
    const Float64 shifted = g + 3.0;
    constexpr Float64 kClampHi = std::bit_cast<Float64>(0x4017FFE5C91D14E4ull);
    const Float64 t = (6.0 > shifted) ? shifted : kClampHi;
    const Float64 tClamped = (t > 0.0) ? t : 0.0; // maxsd with 0.0 @ 0x10003d6e2
    return lo + (hi - lo) * (tClamped * (1.0 / 6.0));
}

// model guard (not authentic): the binary ORs the 36-byte record flags word
// (+0x22) with no bounds check; the vector model guards the access.
inline void OrVertexFlagBits(std::vector<uint16_t>& flags, Sint32 vertex,
                             uint16_t bits) noexcept {
    if (vertex >= 0 && static_cast<std::size_t>(vertex) < flags.size()) {
        flags[static_cast<std::size_t>(vertex)] |= bits;
    }
}

} // namespace

// ---- (a) flag/clear helpers + FindMinAndMaxHeights pair ----

void cSTETerrain::ClearVertexPropertyFlags(const SC4RectI& rect, uint16_t mask) {
    // Exact cSTETerrain::ClearVertexPropertyFlags(SC4Rect<int> const&,
    // unsigned short) @ 0x100735288.
    //   cmp top, bottom; jg ret                         @ 0x100735294
    //   not edx (mask -> ~mask)                          @ 0x100735299
    //   rows [top, bottom] inclusive (jl back edge @ 0x1007352ec); per row,
    //   left > right skips (jg @ 0x1007352a5); columns [left, right]
    //   INCLUSIVE — the first clear at column `left` runs before the
    //   inc/cmp/jl tail (do-while shape @ 0x1007352c0..0x1007352dc), so
    //   column `left` is cleared even when left == right.
    if (rect.top > rect.bottom) {
        return;
    }
    const uint16_t inv = static_cast<uint16_t>(~mask);
    for (Sint32 z = rect.top; z <= rect.bottom; ++z) {
        if (rect.left > rect.right) {
            continue;
        }
        const Sint32 base = z * mVertexCountX;
        mVertexFlags[static_cast<std::size_t>(base + rect.left)] &= inv;
        for (Sint32 x = rect.left; x < rect.right; ++x) {
            mVertexFlags[static_cast<std::size_t>(base + x + 1)] &= inv;
        }
    }
}

void cSTETerrain::ClearVertexPropertyFlags(uint16_t mask) {
    // Exact cSTETerrain::ClearVertexPropertyFlags(unsigned short) @
    // 0x100735810: flags[i] &= ~mask for i in [0, vertexCount)
    // (word and with si = ~mask @ 0x10073582a, stride 0x24).
    if (mVertexCount == 0) {
        return;
    }
    const uint16_t inv = static_cast<uint16_t>(~mask);
    for (Sint32 i = 0; i < mVertexCount; ++i) {
        mVertexFlags[static_cast<std::size_t>(i)] &= inv;
    }
}

void cSTETerrain::ClearVertexCliffFlipTriangulationFlags(const SC4RectI& rect) {
    // Exact cSTETerrain::ClearVertexCliffFlipTriangulationFlags @
    // 0x1007352f0. Same inclusive row/column scan as
    // ClearVertexPropertyFlags; per vertex with flag bit 0x800 set
    // (test byte [record+0x23], 0x8 @ 0x10073533d): the vtable+0x2f0 call
    // FlipInternalTriangulation(x, z, false) @ 0x100735353, then the bit is
    // cleared (and 0xf7ff @ 0x100735361).
    if (rect.top > rect.bottom) {
        return;
    }
    for (Sint32 z = rect.top; z <= rect.bottom; ++z) {
        if (rect.left > rect.right) {
            continue;
        }
        for (Sint32 x = rect.left; x <= rect.right; ++x) {
            const Sint32 v = z * mVertexCountX + x;
            if (v < 0 || static_cast<std::size_t>(v) >= mVertexFlags.size()) {
                continue; // model guard (the binary derefs the record)
            }
            if ((mVertexFlags[static_cast<std::size_t>(v)] & 0x800) != 0) {
                FlipInternalTriangulation(static_cast<Uint32>(x),
                                           static_cast<Uint32>(z), false);
                mVertexFlags[static_cast<std::size_t>(v)] &=
                    static_cast<uint16_t>(~0x800);
            }
        }
    }
}

void cSTETerrain::FindMinAndMaxHeights(const SC4RectI& rect, Float32& outMin,
                                       Float32& outMax) {
    // Exact cSTETerrain::FindMinAndMaxHeights(SC4Rect<int> const&, float&,
    // float&) @ 0x100735c54.
    //   outMin = FLT_MAX (0x7f7fffff), outMax = -FLT_MAX (0xff7fffff)
    //                                                 @ 0x100735c58/0x100735c5e
    //   rows [top, bottom] inclusive; per row the scan runs [start, end]
    //   INCLUSIVE (do-while: index `start` is read before the inc/cmp/jl
    //   tail @ 0x100735ca7..0x100735ce8), where start = z*stride + left and
    //   end = z*stride + right (start > end skips the row, jg @ 0x100735c90).
    //   Per element: if (min > alt) min = alt (jbe skip @ 0x100735cbb);
    //   if (alt > max) max = alt (jbe skip @ 0x100735cd5). Both checks run
    //   for every element (the min-update path falls into the max check).
    outMin = 3.40282346638528859812e+38F;  // FLT_MAX bit pattern 0x7f7fffff
    outMax = -3.40282346638528859812e+38F;  // 0xff7fffff
    if (rect.top > rect.bottom) {
        return;
    }
    for (Sint32 z = rect.top; z <= rect.bottom; ++z) {
        const Sint32 start = z * mVertexCountX + rect.left;
        const Sint32 end = z * mVertexCountX + rect.right;
        if (start > end) {
            continue;
        }
        if (start < 0) {
            continue; // model guard (the binary would read below the grid)
        }
        for (Sint32 i = start; i <= end; ++i) {
            if (static_cast<std::size_t>(i) >= mAltitude.size()) {
                break; // model guard
            }
            const Float32 a = mAltitude[static_cast<std::size_t>(i)];
            if (outMin > a) {
                outMin = a;
            }
            if (a > outMax) {
                outMax = a;
            }
        }
    }
}

void cSTETerrain::FindMinAndMaxHeights() {
    // Exact cSTETerrain::FindMinAndMaxHeights() @ 0x1007362b4.
    //   null grid -> no writes (test rax, rax; je @ 0x1007362c2)
    //   min = max = altitude[0]                       @ 0x1007362c4..0x1007362cc
    //   vertexCount < 2 (unsigned) -> return           @ 0x1007362d2..0x1007362d9
    //   per i in [1, vertexCount): min > alt -> min = alt (jbe @ 0x1007362f3
    //   branches to the max check); else alt > max -> max = alt
    //   (jbe @ 0x100736302 skips) — the min-update path skips the max check.
    if (mAltitude.empty()) {
        return;
    }
    mMinHeight = mAltitude[0];
    mMaxHeight = mAltitude[0];
    if (static_cast<Uint32>(mVertexCount) < 2u) {
        return;
    }
    for (Sint32 i = 1; i < mVertexCount; ++i) {
        const Float32 a = mAltitude[static_cast<std::size_t>(i)];
        if (mMinHeight > a) {
            mMinHeight = a;
        } else if (a > mMaxHeight) {
            mMaxHeight = a;
        }
    }
}

// ---- (b) exact vtable bodies ----

void cSTETerrain::SetVertexAltitude(Sint32 vertex, Float32 altitude) noexcept {
    // Exact cSTETerrain::SetAltitude(int, float) @ 0x10073e192 — the
    // vtable+0x188 callee (slot verified: __ZTV11cSTETerrain 0x100aea6b0 +
    // 0x10 + 0x188 holds 0x10073e192). Kept under the model name because the
    // public raw-grid test accessor SetAltitude occupies the binary name.
    //   test byte [record+0x23], 0x40 -> no-op when set @ 0x10073e1a4
    //   (flag word bit 0x4000, the same suppress bit RaiseVertex/LowerVertex
    //   honor)
    //   oldSum = altitude[v] + depth[v]                  @ 0x10073e1bb/0x10073e1c0
    //   altitude[v] = new                                  @ 0x10073e1c5
    //   depth[v] = oldSum - new when (oldDepth > 0        @ 0x10073e1d2 (ja)
    //             || seaLevel > new)                       @ 0x10073e1df (jbe skips)
    //   — dry land above sea level keeps its old depth.
    if (vertex < 0 || static_cast<Uint32>(vertex) >= static_cast<Uint32>(mVertexCount)) {
        return; // model guard (the binary derefs the record directly)
    }
    const std::size_t v = static_cast<std::size_t>(vertex);
    if (((mVertexFlags[v] >> 8) & 0x40) != 0) {
        return;
    }
    const Float32 oldAltitude = mAltitude[v];
    const Float32 oldDepth = mSurfaceWater.depth[v];
    const Float32 oldSum = oldAltitude + oldDepth;
    mAltitude[v] = altitude;
    if (oldDepth > 0.0f || mSeaLevel > altitude) {
        mSurfaceWater.depth[v] = oldSum - altitude;
    }
}

bool cSTETerrain::SetSeaLevel(Float32 newLevel) {
    // Exact cSTETerrain::SetSeaLevel(float) @ 0x10073e556 — the vtable+0x168
    // callee (slot verified: __ZTV11cSTETerrain 0x100aea6b0 + 0x10 + 0x168
    // holds 0x10073e556).
    //   jb out when newLevel < kMinSeaLevel (-10.1f @ 0x100b175d4)
    //                                                 @ 0x10073e562..0x10073e569
    //   jb out when kMaxSeaLevel (200.1f @ 0x100b175d8) < newLevel
    //                                                 @ 0x10073e56b..0x10073e576
    //   (both unsigned-style jb: NaN also falls out with eax = 0)
    //   mSeaLevel (+0x9c) = newLevel                     @ 0x10073e578
    //   cSTESurfaceWater::ResetWaterToSeaLevel(this+0x58 rect)
    //                                                 @ 0x10073e580..0x10073e58b
    //   PostMessageA(0x89171496, 0, 0, 0, 0)             @ 0x10073e590..0x10073e59e
    //   vtable+0x228 redisplay with (1, 1, this+0x58, 0) @ 0x10073e5a3..0x10073e5b9
    //   return true                                       @ 0x10073e5bf
    if (!(newLevel >= MinSeaLevelTunable())) {
        return false;
    }
    if (!(MaxSeaLevelTunable() >= newLevel)) {
        return false;
    }
    mSeaLevel = newLevel;
    // clean-room fallback (not authentic): ResetWaterToSeaLevel is not
    // reconstructed — recorded no-op.
    ++mSurfaceWater.resetToSeaLevelCalls;
    mSurfaceWater.resetToSeaLevelLastRect[0] = mWorldRect.left;
    mSurfaceWater.resetToSeaLevelLastRect[1] = mWorldRect.top;
    mSurfaceWater.resetToSeaLevelLastRect[2] = mWorldRect.right;
    mSurfaceWater.resetToSeaLevelLastRect[3] = mWorldRect.bottom;
    // clean-room fallback (not authentic): PostMessageA @ 0x100734b92 is not
    // reconstructed — recorded no-op.
    ++mPostMessageACalls;
    mPostMessageALastId = 0x89171496u;
    ReDisplayTerrain(1, 1, mWorldRect, 0);
    return true;
}

void cSTETerrain::FlipInternalTriangulation(Uint32 x, Uint32 z, bool flip) {
    // Exact cSTETerrain::FlipInternalTriangulation(unsigned int, unsigned
    // int, bool) @ 0x100738848 — the vtable+0x2f0 callee (slot verified:
    // __ZTV11cSTETerrain 0x100aea6b0 + 0x10 + 0x2f0 holds 0x100738848).
    //   cell bounds (unsigned jbe): cellCountX <= x / cellCountZ <= z -> out
    //                                                 @ 0x10073884e..0x100738856
    //   vertex = z * vertexCountX + x                    @ 0x100738858..0x10073885e
    //   newFlags = flip ? (flags | 0x1000) : (flags & 0xefff)
    //                                                 @ 0x100738874..0x100738889
    //   unchanged -> out (je @ 0x100738892)
    //   store; the +0x118 byte gates the +0x110 view's
    //   SetTriangleIndicesForCell(z, x) tail-jmp @ 0x1007388b1 (the callee
    //   takes the cell z first, then the cell x — arg order swapped relative
    //   to this method).
    if (static_cast<Uint32>(mCellCountX) <= x || static_cast<Uint32>(mCellCountZ) <= z) {
        return;
    }
    const Sint32 v = static_cast<Sint32>(z) * mVertexCountX + static_cast<Sint32>(x);
    if (v < 0 || static_cast<std::size_t>(v) >= mVertexFlags.size()) {
        return; // model guard (the binary derefs the record)
    }
    const uint16_t flags = mVertexFlags[static_cast<std::size_t>(v)];
    const uint16_t newFlags = flip ? static_cast<uint16_t>(flags | 0x1000)
                                   : static_cast<uint16_t>(flags & 0xefff);
    if (newFlags == flags) {
        return;
    }
    mVertexFlags[static_cast<std::size_t>(v)] = newFlags;
    if (!mTextureSpanInfoDirtyGate) {
        return; // +0x118 byte gate @ 0x1007388a3
    }
    // clean-room fallback (not authentic):
    // cSTETerrainView3D::SetTriangleIndicesForCell(int, int) @ 0x10074f376
    // is not reconstructed — recorded for observation.
    TerrainView3DObservation& view = View3DObservation();
    ++view.setTriangleIndicesCalls;
    view.setTriangleIndicesLastArg0 = static_cast<Sint32>(z);
    view.setTriangleIndicesLastArg1 = static_cast<Sint32>(x);
}

// ---- (c) MarkCliff + CalculateNormalsAndAssignCliffTextures ----

void cSTETerrain::MarkCliff(Sint32 vertex, bool markFirst, bool markSecond) {
    // Exact cSTETerrain::MarkCliff(int, bool, bool) @ 0x100735398
    // (esi = vertex, dl = first bool, cl = second bool).
    //   model guard (not authentic): the binary reads/ORs the records of
    //   v, v+1, v+stride, v+stride+1 with no bounds check; the normals pass
    //   only ever calls it with the full quad in range (z <= maxVertexIndexZ,
    //   x <= maxVertexIndexX gives v+stride+1 == mVertexCount-1 at the
    //   extreme, so the guard never mis-fires there; it excludes exactly the
    //   last vertex row the binary would read past the grid). The model
    //   guards.
    if (vertex < 0 || vertex + mVertexCountX + 1 >= mVertexCount) {
        return;
    }
    const Sint32 stride = mVertexCountX;
    const std::size_t v = static_cast<std::size_t>(vertex);
    if (markFirst && markSecond) {
        // Both paths requested: recompute the two quad steepness values from
        // the altitudes, based at the bottom-left corner alt(v+stride):
        //   dv2 = alt(v) - alt(v+stride); dw2 = alt(v+1) - alt(v+stride);
        //   dh2 = alt(v+stride+1) - alt(v+stride)     @ 0x1007353c6..0x100735409
        //   A lanes = {dv2*16 - 0.0f, dv2*16 - dw2*16}
        //     (mulps by the {0.0f, 16.0f, 0.0f, 0.0f} constant @ 0x100a28610,
        //      subps @ 0x100735428); lenA = sqrt((A1^2 + 65536) + A0^2)
        //     @ 0x100735433..0x10073544f; nyA = (1/lenA) * 256 @ 0x100735453..0x100735468
        //   B lanes = {dw2*16 - dh2*16, (-16.0f)*dh2 - 0.0f}
        //     (constant 0xc1800000 @ 0x100a16da4, insertps/subps
        //      @ 0x10073547d..0x10073549d); lenB = sqrt((B1^2 + 65536) + B0^2)
        //     @ 0x1007354a0..0x1007354b4; nyB = (1/lenB) * 256 @ 0x1007354b4..0x1007354c0
        //   steep = kMaxNormalYForCliff (0.3f) > |ny|  (ucomiss/seta
        //     @ 0x10073547a/0x1007354c6)
        const Float32 belowLeft = mAltitude[v + static_cast<std::size_t>(stride)];
        const Float32 dv2 = mAltitude[v] - belowLeft;
        const Float32 dw2 = mAltitude[v + 1] - belowLeft;
        const Float32 dh2 = mAltitude[v + static_cast<std::size_t>(stride) + 1] - belowLeft;
        const Float32 a0 = dv2 * 16.0f;                     // lane0 minus +0.0f
        const Float32 a1 = dv2 * 16.0f - dw2 * 16.0f;
        const Float32 lenA = std::sqrt((a1 * a1 + 65536.0f) + a0 * a0);
        const Float32 nyA = (1.0f / lenA) * 256.0f;
        const Float32 b0 = dw2 * 16.0f - dh2 * 16.0f;
        const Float32 b1 = (-16.0f) * dh2;                  // lane1 minus +0.0f
        const Float32 lenB = std::sqrt((b1 * b1 + 65536.0f) + b0 * b0);
        const Float32 nyB = (1.0f / lenB) * 256.0f;
        const Float32 maxNY = MaxNormalYForCliffTunable();
        const bool steepA = maxNY > std::fabs(nyA);
        const bool steepB = maxNY > std::fabs(nyB);
        if (steepA && steepB) {
            // Fallthrough marks @ 0x1007354cf..0x100735518:
            //   first: {v, v+stride, v+stride+1}; second: {v, v+1, v+stride+1}.
            if (markFirst) {
                mVertexFlags[v] |= 0x8;
                mVertexFlags[v + static_cast<std::size_t>(stride)] |= 0x8;
                mVertexFlags[v + static_cast<std::size_t>(stride) + 1] |= 0x8;
            }
            if (markSecond) {
                mVertexFlags[v] |= 0x8;
                mVertexFlags[v + 1] |= 0x8;
                mVertexFlags[v + static_cast<std::size_t>(stride) + 1] |= 0x8;
            }
            return;
        }
        // Mixed path @ 0x100735566..0x100735618: the quad is not steep on
        // both triangulations — flip the triangulation via vtable+0x2f0
        // (call @ 0x100735589, BEFORE the 0x800 OR @ 0x10073559c; the flip
        // gates on the separate 0x1000 bit so the order is observable only
        // through the call), then mark whichever side is steep.
        //   steepA marks {v, v+stride, v+1} (r13 / r14d (=v) + stride / r12
        //     @ 0x1007355b5/0x1007355cc/0x1007355d8)
        //   steepB marks {v+1, v+stride, v+stride+1} — THREE distinct
        //     vertices: r12 (=9*(v+1)) @ 0x1007355ef, r14d += stride
        //     (r14d still holds v on the mixed path; the incl %r14d
        //     @ 0x10073553e belongs to the both-steep route which exits at
        //     0x100735561 and never falls through) @ 0x1007355fc/0x100735604,
        //     and r15d (=v+1) += stride @ 0x100735610/0x100735618. The
        //     earlier "redundant double-OR of v+stride+1" reading was a
        //     register mis-trace; there is no redundancy.
        const Uint32 cx = static_cast<Uint32>(vertex) % static_cast<Uint32>(stride);
        const Uint32 cz = static_cast<Uint32>(vertex) / static_cast<Uint32>(stride);
        FlipInternalTriangulation(cx, cz, true);
        mVertexFlags[v] |= 0x800;
        if (steepA) {
            mVertexFlags[v] |= 0x8;
            mVertexFlags[v + static_cast<std::size_t>(stride)] |= 0x8;
            mVertexFlags[v + 1] |= 0x8;
        }
        if (steepB) {
            mVertexFlags[v + 1] |= 0x8;
            mVertexFlags[v + static_cast<std::size_t>(stride)] |= 0x8;
            mVertexFlags[v + static_cast<std::size_t>(stride) + 1] |= 0x8;
        }
        return;
    }
    // !first || !second: straight to the standard marks @ 0x1007354cf.
    if (markFirst) {
        mVertexFlags[v] |= 0x8;
        mVertexFlags[v + static_cast<std::size_t>(stride)] |= 0x8;
        mVertexFlags[v + static_cast<std::size_t>(stride) + 1] |= 0x8;
    }
    if (markSecond) {
        mVertexFlags[v] |= 0x8;
        mVertexFlags[v + 1] |= 0x8;
        mVertexFlags[v + static_cast<std::size_t>(stride) + 1] |= 0x8;
    }
}

void cSTETerrain::CalculateNormalsAndAssignCliffTextures(const SC4RectI& rect) {
    // Exact cSTETerrain::CalculateNormalsAndAssignCliffTextures @ 0x100734dc8.
    //
    //   ClearVertexPropertyFlags(rect, 0x8)               @ 0x100734e00/e05
    //   padded local rect: left = (left <= 0) ? 0 : left-1 (cmovle
    //     @ 0x100734e13..0x100734e1b), top likewise; right clamped to
    //     maxVertexIndexX (unsigned jbe @ 0x100734e38), bottom to
    //     maxVertexIndexZ (@ 0x100734e4a)
    //   ClearVertexCliffFlipTriangulationFlags(local)       @ 0x100734e5d
    //   top > bottom -> out                                @ 0x100734e6a
    ClearVertexPropertyFlags(rect, 0x8);
    SC4RectI local = rect;
    local.left = (rect.left <= 0) ? 0 : rect.left - 1;
    local.top = (rect.top <= 0) ? 0 : rect.top - 1;
    if (static_cast<Uint32>(local.right) > static_cast<Uint32>(mMaxVertexIndexX)) {
        local.right = mMaxVertexIndexX;
    }
    if (static_cast<Uint32>(local.bottom) > static_cast<Uint32>(mMaxVertexIndexZ)) {
        local.bottom = mMaxVertexIndexZ;
    }
    ClearVertexCliffFlipTriangulationFlags(local);
    if (local.top > local.bottom) {
        return;
    }
    const Sint32 stride = mVertexCountX;
    const Float32 kScale = 16.0f;      // 0x100a16904
    const Float32 kLenAdd = 65536.0f;  // 0x100a1c420 (256^2)
    const Float32 kNYScale = 256.0f;   // 0x100a19694
    const Float32 kOne = 1.0f;          // nS3DL::k_one (BSS, runtime 1.0f)
    const Float32 maxNY = MaxNormalYForCliffTunable();
    // Quad-normal ping-pong: the first row (z = local.top) writes the
    // this+0x188 buffer (@ 0x100734ded), later rows alternate through the
    // phase-boundary block (@ 0x10073509a/0x1007350a1 re-load [rbp-0x48] /
    // [rbp-0x60]). Vertex-normal phase 2 only runs for rows z >= rect.top
    // (@ 0x100735096), so the padded top row gets quad normals but no
    // vertex normals (binary oddity, reproduced).
    Float32* cur = mQuadNormalsB.data();  // +0x188 for the first row
    Float32* prev = mQuadNormalsA.data(); // +0x170
    for (Sint32 z = local.top; z <= local.bottom; ++z) {
        // Quad pass: two normals per column x of the row (records of 6
        // floats at cur + 24*x, written @ 0x100734f5e..0x100735020).
        // BINARY ODDITY: both normals are constrained to the x + z = 0
        // plane (nA = {s*(dv-dh), ., s*(dh-dv)}, nB = {s*(dh-dw), .,
        // s*(dw-dh)}), which does not match the cross-product normal of
        // either triangulation — reproduced as-is.
        for (Sint32 x = local.left; x <= local.right; ++x) {
            const std::size_t idx = static_cast<std::size_t>(z * stride + x);
            const Float32 h00 = mAltitude[idx];                  // alt(x, z)
            const Float32 h10 = mAltitude[idx + 1];              // alt(x+1, z)
            const Float32 h01 = mAltitude[idx + static_cast<std::size_t>(stride)];
            const Float32 h11 = mAltitude[idx + static_cast<std::size_t>(stride) + 1];
            const Float32 dvS = (h01 - h00) * kScale;  // mulps @ 0x100734f3f
            const Float32 dhS = (h11 - h00) * kScale;
            const Float32 dwS = (h10 - h00) * kScale;  // mulss @ 0x100734f42
            // Normal A @ 0x100734f30..0x100734f52 (subps/insertps lanes).
            const Float32 ax = dvS - dhS;
            const Float32 az = dhS - dvS;
            // lenA = sqrt((ax^2 + 65536) + az^2) @ 0x100734f7d..0x100734f90.
            const Float32 lenA = std::sqrt((ax * ax + kLenAdd) + az * az);
            const Float32 invLenA = kOne / lenA;
            const Float32 nAx = ax * invLenA;   // mulss @ 0x100734f98
            const Float32 nAz = az * invLenA;   // mulss @ 0x100734fa3
            const Float32 nAy = invLenA * kNYScale; // mulss @ 0x100734fa7
            const Float32 bx = dhS - dwS;       // subss @ 0x100734f4a
            const Float32 bz = dwS - dhS;       // subss @ 0x100734f4e
            // lenB = sqrt((bz^2) + (bx^2 + 65536)) — addss order
            // @ 0x100734fe3..0x100734ff6 (commutes to the same value).
            const Float32 lenB = std::sqrt(bz * bz + (bx * bx + kLenAdd));
            const Float32 invLenB = kOne / lenB;
            const Float32 nBx = bx * invLenB;   // mulss @ 0x100735005
            const Float32 nBz = bz * invLenB;   // mulss @ 0x100735010
            const Float32 nBy = invLenB * kNYScale; // mulss @ 0x100735014
            // Record stores @ 0x100734f9c/0x100734fac/0x100734fb3 (nA) and
            // 0x100735009/0x100735019/0x100735020 (nB) — the transient
            // 256.0f placeholder stores @ 0x100734f65/0x100734fd0 are
            // overwritten by the normalized values.
            Float32* rec = cur + static_cast<std::size_t>(x) * 6u;
            rec[0] = nAx;
            rec[1] = nAy;
            rec[2] = nAz;
            rec[3] = nBx;
            rec[4] = nBy;
            rec[5] = nBz;
            // Cliff dispatch @ 0x100734fc6..0x100735050: MarkCliff runs iff
            // A or B is a cliff; steep = kMaxNormalYForCliff > |ny|
            // (seta on both @ 0x10073503f/0x100735047).
            const bool cliffA = maxNY > std::fabs(nAy);
            const bool cliffB = maxNY > std::fabs(nBy);
            if (cliffA || cliffB) {
                MarkCliff(static_cast<Sint32>(idx), cliffA, cliffB);
            }
        }
        // Vertex-normal phase 2, rows z >= rect.top only (@ 0x100735096
        // jl skips). Columns [rect.left, rect.right] of the ORIGINAL rect
        // (do-while shape — column `left` is processed even when left ==
        // right @ 0x1007350ab..0x100735266), vertex = z*stride + x.
        if (z >= rect.top) {
            if (rect.left <= rect.right) {
                for (Sint32 x = rect.left; x <= rect.right; ++x) {
                    // model guard (not authentic): the binary indexes the
                    // quad buffers and the vertex records unchecked.
                    if (x < 0 || x >= mVertexCountX) {
                        continue;
                    }
                    // Sum of the six adjacent quad normals
                    // @ 0x1007350dd..0x1007351bd (component-wise adds):
                    //   x > 0: cur[x-1].n1; z > 0: prev[x-1].n1 + prev[x-1].n0
                    //   rect.left < cellCountX gate @ 0x10073515b (BINARY
                    //   ODDITY: the x-column terms are gated on the rect's
                    //   LEFT edge, not on x):
                    //     z < cellCountZ: cur[x].n0 + cur[x].n1;
                    //     z > 0: prev[x].n0
                    Float32 sx = 0.0f, sy = 0.0f, sz = 0.0f;
                    if (x > 0) {
                        const Float32* pl = cur + static_cast<std::size_t>(x - 1) * 6u;
                        if (z < mCellCountZ) {
                            sx += pl[3]; // n1.x @ 0x1007350f8
                            sy += pl[4]; // n1.y @ 0x100735102
                            sz += pl[5]; // n1.z @ 0x10073510d
                        }
                        if (z > 0) {
                            const Float32* pp = prev + static_cast<std::size_t>(x - 1) * 6u;
                            sx += pp[3]; // prev n1.x @ 0x100735127
                            sy += pp[4];
                            sz += pp[5];
                            sx += pp[0]; // prev n0.x @ 0x100735142
                            sy += pp[1];
                            sz += pp[2];
                        }
                    }
                    if (static_cast<Uint32>(mCellCountX) >
                        static_cast<Uint32>(rect.left)) {
                        const Float32* pc = cur + static_cast<std::size_t>(x) * 6u;
                        if (z < mCellCountZ) {
                            sx += pc[0]; // n0.x @ 0x10073516e
                            sy += pc[1];
                            sz += pc[2];
                            sx += pc[3]; // n1.x @ 0x10073518c
                            sy += pc[4];
                            sz += pc[5];
                        }
                        if (z > 0) {
                            const Float32* pp = prev + static_cast<std::size_t>(x) * 6u;
                            sx += pp[0]; // prev n0.x @ 0x1007351ac
                            sy += pp[1];
                            sz += pp[2];
                        }
                    }
                    // Normalize with the zero-component guards
                    // @ 0x1007351bd..0x10073523d: len == 0 -> sx = 1;
                    // sx == 0 -> sy = 1; sy == 0 -> sz = 1 (each guard
                    // compares against +0.0f; NaN skips like jne/jp).
                    const Float32 len0 = std::sqrt((sx * sx + sy * sy) + sz * sz);
                    if (len0 == 0.0f) {
                        sx = 1.0f;
                    }
                    if (sx == 0.0f) {
                        sy = 1.0f;
                    }
                    if (sy == 0.0f) {
                        sz = 1.0f;
                    }
                    // len2 = sqrt(sz^2 + (sx^2 + sy^2)) @ 0x10073521f..0x10073522d.
                    const Float32 len2 = std::sqrt(sz * sz + (sx * sx + sy * sy));
                    const Float32 inv = kOne / len2;
                    // Record normal stores @ 0x100735241..0x100735259
                    // (record +0x4/+0x8/+0xc).
                    mVertexNormals[static_cast<std::size_t>(z * stride + x)] = {
                        sx * inv, sy * inv, inv * sz};
                }
            }
        }
        // Ping-pong swap (the phase-boundary block hands the current row's
        // buffer to prev and vice versa for the next row).
        std::swap(cur, prev);
    }
}

// ---- (d) shore trio ----

Sint32 cSTETerrain::GetNeighboringSeaVertex(Sint32 vertex) const {
    // Exact cSTETerrain::GetNeighboringSeaVertex(int) const @ 0x100731a1c.
    //   x = vertex % vertexCountX (div @ 0x100731a32)
    //   BINARY ODDITY: the row coordinate divides by vertexCountZ (+0x54)
    //   instead of vertexCountX (@ 0x100731a40) — identical only on square
    //   grids; reproduced as-is.
    //   The scan walks the static kXIncrement/kZIncrement tables at 8-byte
    //   strides (rsi += 8 @ 0x100731a98), i.e. the even entries 0, 2, 4, 6
    //   = W, N, E, S; per direction the neighbor is skipped when
    //   vertexCountX <= nx (jbe @ 0x10073164) or vertexCountZ <= nz
    //   (@ 0x100731a6c), and returned when seaLevel > neighborAltitude
    //   (ja @ 0x100731a92). No match -> -1 (@ 0x100731aa2).
    if (vertex < 0 || static_cast<std::size_t>(vertex) >= mAltitude.size()) {
        return -1; // model guard (the binary derefs the record)
    }
    const Uint32 x = static_cast<Uint32>(vertex) % static_cast<Uint32>(mVertexCountX);
    const Uint32 row = static_cast<Uint32>(vertex) / static_cast<Uint32>(mVertexCountZ);
    for (Sint32 k = 0; k < 4; ++k) {
        const Sint32 dir = 2 * k; // W, N, E, S
        const Sint32 nx = static_cast<Sint32>(x) + kXIncrementTable[dir];
        if (static_cast<Uint32>(mVertexCountX) <= static_cast<Uint32>(nx)) {
            continue;
        }
        const Sint32 nz = static_cast<Sint32>(row) + kZIncrementTable[dir];
        if (static_cast<Uint32>(mVertexCountZ) <= static_cast<Uint32>(nz)) {
            continue;
        }
        const Sint32 n = nz * mVertexCountX + nx;
        if (mSeaLevel > mAltitude[static_cast<std::size_t>(n)]) {
            return n;
        }
    }
    return -1;
}

bool cSTETerrain::AnalyzeShoreAndCreateCliffs(std::deque<Sint32>& shore) {
    // Exact cSTETerrain::AnalyzeShoreAndCreateCliffs(std::deque<int>&) @
    // 0x1007317fe.
    //   Early out: deque size < kMinShoreCliffSize -> false (cmp/jbe
    //   @ 0x100731819..0x100731821, unsigned).
    //   Per shore vertex i (@ 0x10073184e..0x10073187a): seaNeighbor =
    //   GetNeighboringSeaVertex(vertex); sum = alt(vertex) - seaLevel
    //   (@ 0x10073188f); probe walk: probe[0] = 2*vertex - seaNeighbor,
    //   probe[k+1] = probe[k] + (vertex - seaNeighbor) — five probes
    //   (@ 0x1007318c9..0x1007318fd), each written first, then gated by the
    //   unsigned row bound probe / vertexCountX < vertexCountZ (div/jbe
    //   @ 0x1007318d8) and alt >= seaLevel (ja breaks @ 0x1007318e7), each
    //   adding (alt - seaLevel) to the sum.
    //   Candidate iff all five probes survived AND sum >
    //   kMinShoreCliffCandidateHtSum (jbe skips @ 0x100731906).
    //   Abandoned walks flush: count > kMinShoreCliffSize ->
    //   CreateShoreCliff + createdAny = true, then the vector resets
    //   (end = begin @ 0x100731972); a final flush runs after the loop
    //   (@ 0x100731993..0x1007319d6).
    const Sint32 kMinSize = MinShoreCliffSizeTunable();
    if (static_cast<Uint32>(shore.size()) < static_cast<Uint32>(kMinSize)) {
        return false;
    }
    std::vector<TerrainCliffVertInfo> verts;
    bool createdAny = false;
    const auto flush = [&]() {
        if (verts.size() > static_cast<std::size_t>(
                               static_cast<Uint32>(kMinSize))) {
            CreateShoreCliff(verts);
            createdAny = true;
        }
        verts.clear();
    };
    if (!shore.empty()) {
        for (std::size_t i = 0; i < shore.size(); ++i) {
            const Sint32 vertex = shore[i];
            const Sint32 seaNeighbor = GetNeighboringSeaVertex(vertex);
            const Sint32 delta = vertex - seaNeighbor;
            Float32 sum = AltitudeAt(vertex) - mSeaLevel;
            TerrainCliffVertInfo info;
            info.seaNeighbor = seaNeighbor;
            info.vertex = vertex;
            Sint32 probe = 2 * vertex - seaNeighbor;
            bool allFive = true;
            for (Sint32 k = 0; k < 5; ++k) {
                info.probe[k] = probe;
                const Uint32 probeRow =
                    static_cast<Uint32>(probe) / static_cast<Uint32>(mVertexCountX);
                if (static_cast<Uint32>(mVertexCountZ) <= probeRow) {
                    allFive = false;
                    break;
                }
                const Float32 alt = AltitudeAt(probe);
                if (mSeaLevel > alt) {
                    allFive = false;
                    break;
                }
                sum = sum + (alt - mSeaLevel);
                probe += delta;
            }
            if (allFive && sum > MinShoreCliffCandidateHtSumTunable()) {
                verts.push_back(info); // push_back fast/slow path @ 0x100731908/0x100731984
            } else {
                flush(); // count check + CreateShoreCliff @ 0x100731935..0x10073196e
            }
        }
    }
    flush(); // final flush @ 0x1007319ac..0x1007319d4
    return createdAny;
}

void cSTETerrain::CreateShoreCliff(std::vector<TerrainCliffVertInfo>& verts) {
    // Exact cSTETerrain::CreateShoreCliff(std::vector<tCliffVertInfo>&) @
    // 0x100731ab0.
    //   base = (float)(RandomDoubleRangeUniform(kMinShoreCliffHt,
    //     kMaxShoreCliffHt) + seaLevel)   @ 0x100731ac7..0x100731b28
    //     (40.1f @ 0x100b17678 / 50.1f @ 0x100b1767c, doubles)
    //   count = min(vector size, kMaxShoreCliffSize) (imul 0xb6db6db7
    //     element count @ 0x100731b06, cmovb clamp @ 0x100731b14, signed
    //     jle empty-out @ 0x100731b19)
    //   per element i (first `count` elements):
    //     flags[seaNeighbor] |= 0x4 and flags[vertex] |= 0x4
    //                                                 @ 0x100731b67/0x100731b7c
    //     gauss = (float)RandomDoubleRangeGaussian(0.0, 2.0) — sigma double
    //       @ 0x100a18278, mean xorps @ 0x100731b81
    //     alt(vertex) > base -> newHeight = base + gauss else base - gauss
    //       (jbe @ 0x100731bb5); vtable+0x188 SetAltitude @ 0x100731bda
    //     five probes: SetAltitude(probe[k],
    //       (float)(RandomDoubleRangeUniform(-8.0, 8.0) + newHeight))
    //       — doubles @ 0x100a29bb8/0x100a29bc0, loop rbx 2..6
    //       @ 0x100731bf6..0x100731c2e
    //   BINARY ODDITY: when seaNeighbor is -1 (no sea neighbor), the flags
    //   OR writes 14 bytes below the grid; the model guard skips it.
    const Float64 seaLevelD = static_cast<Float64>(mSeaLevel); // cvtss2sd @ 0x100731ac7
    const Float64 draw =
        RandomDoubleRangeUniform(sRand, static_cast<Float64>(MinShoreCliffHtTunable()),
                                  static_cast<Float64>(MaxShoreCliffHtTunable()));
    Sint32 count = static_cast<Sint32>(verts.size());
    const Sint32 kMaxSize = MaxShoreCliffSizeTunable();
    if (static_cast<Uint32>(kMaxSize) < static_cast<Uint32>(count)) {
        count = kMaxSize; // cmovb @ 0x100731b14
    }
    if (count <= 0) {
        return; // signed jle @ 0x100731b19
    }
    const Float32 base = static_cast<Float32>(draw + seaLevelD); // addsd/cvtsd2ss
    for (Sint32 i = 0; i < count; ++i) {
        const TerrainCliffVertInfo& info =
            verts[static_cast<std::size_t>(i)]; // 28-byte stride (imul
                                                // 0xb6db6db7, 28*i @ 0x100731b54)
        OrVertexFlagBits(mVertexFlags, info.seaNeighbor, 0x4);
        OrVertexFlagBits(mVertexFlags, info.vertex, 0x4);
        const Float32 gauss =
            static_cast<Float32>(RandomDoubleRangeGaussian(sRand, 0.0, 2.0));
        Float32 newHeight;
        if (AltitudeAt(info.vertex) > base) {
            newHeight = base + gauss; // addss @ 0x100731bb7
        } else {
            newHeight = base - gauss; // subss @ 0x100731bc8
        }
        SetVertexAltitude(info.vertex, newHeight); // vtable+0x188 @ 0x100731bda
        const Float64 newHeightD = static_cast<Float64>(newHeight);
        for (Sint32 k = 0; k < 5; ++k) {
            // The uniform draw STAYS double: addsd jitter + newHeightD
            // @ 0x100731c0e, then ONE cvtsd2ss to float @ 0x100731c13 —
            // rounding the jitter to float first would double-round
            // (AGENTS.md rule 2).
            const Float64 jitter =
                RandomDoubleRangeUniform(sRand, -8.0, 8.0);
            SetVertexAltitude(info.probe[k],
                              static_cast<Float32>(jitter + newHeightD));
        }
    }
}

// ---- (e) CreateShoreCliffs + callees ----

bool cSTETerrain::IsShoreVertex(Sint32 vertex) const {
    // Exact cSTETerrain::IsShoreVertex(int) const @ 0x10073751a.
    //   seaLevel >= altitude -> false (jae @ 0x100737539)
    //   x = vertex % vertexCountX, z = vertex / vertexCountX (div @ 0x10073753f)
    //   neighbor-allowed mask (GetNeighbors @ 0x100734d42 shape):
    //     x <= 0 -> 0x7c else 0xff (@ 0x100737542..0x100737549)
    //     cellCountX <= x -> &= 0xc7 (ja skip @ 0x100737553)
    //     z <= 0 -> &= 0xf1 (@ 0x10073755b)
    //     cellCountZ <= z -> &= 0x1f (@ 0x100737561)
    //   scan bits 0, 2, 4, 6 (bt @ 0x100737570, rdx += 2 @ 0x10073758f) —
    //   the even directions W, N, E, S via the this+0xa4 offset table; a
    //   sea neighbor (seaLevel > alt) makes the vertex shore (ja
    //   @ 0x100737589).
    if (vertex < 0 || static_cast<std::size_t>(vertex) >= mAltitude.size()) {
        return false; // model guard (the binary derefs the record)
    }
    if (!(mAltitude[static_cast<std::size_t>(vertex)] > mSeaLevel)) {
        return false;
    }
    const Uint32 x = static_cast<Uint32>(vertex) % static_cast<Uint32>(mVertexCountX);
    const Uint32 z = static_cast<Uint32>(vertex) / static_cast<Uint32>(mVertexCountX);
    Uint32 mask = (static_cast<Sint32>(x) <= 0) ? 0x7cu : 0xffu;
    if (static_cast<Uint32>(mCellCountX) <= x) {
        mask &= 0xc7u;
    }
    if (static_cast<Sint32>(z) <= 0) {
        mask &= 0xf1u;
    }
    if (static_cast<Uint32>(mCellCountZ) <= z) {
        mask &= 0x1fu;
    }
    for (Sint32 d = 0; d < 8; d += 2) {
        if (((mask >> d) & 1u) == 0) {
            continue;
        }
        const Sint32 n = vertex + NeighborVertexOffset(d, mVertexCountX);
        if (n < 0 || static_cast<std::size_t>(n) >= mAltitude.size()) {
            continue; // model guard
        }
        if (mSeaLevel > mAltitude[static_cast<std::size_t>(n)]) {
            return true;
        }
    }
    return false;
}

void cSTETerrain::DetectShoreVertices() {
    // Exact cSTETerrain::DetectShoreVertices() @ 0x100736ef0.
    //   rect = {0, 0, cellCountX, cellCountZ} (qword loads @ 0x100736f08/
    //   0x100736f0f) -> ClearVertexPropertyFlags(rect, 0x10)
    //                                                 @ 0x100736f13/0x100736f18
    //   per vertex: IsShoreVertex -> flags |= 0x10  @ 0x100736f2c..0x100736f41
    ClearVertexPropertyFlags(SC4RectI{0, 0, mCellCountX, mCellCountZ}, 0x10);
    for (Sint32 v = 0; v < mVertexCount; ++v) {
        if (IsShoreVertex(v)) {
            mVertexFlags[static_cast<std::size_t>(v)] |= 0x10;
        }
    }
}

void cSTETerrain::GetShoreStartingAtVert(Sint32 vertex, std::deque<Sint32>& shore,
                                         uint16_t mark) {
    // Exact cSTETerrain::GetShoreStartingAtVert(int, std::deque<int>&,
    // unsigned short) @ 0x100736f5e.
    //   deque clear @ 0x100736f81, push_back(vertex) @ 0x100736f8c,
    //   flags[vertex] |= mark @ 0x100736fa0,
    //   TrackShoreWithLandToTheRight @ 0x100736faf then Left @ 0x100736fc1.
    shore.clear();
    shore.push_back(vertex);
    OrVertexFlagBits(mVertexFlags, vertex, mark);
    TrackShoreWithLandToTheRight(vertex, shore, mark);
    TrackShoreWithLandToTheLeft(vertex, shore, mark);
}

void cSTETerrain::TrackShoreWithLandToTheRight(Sint32 vertex, std::deque<Sint32>& shore,
                                               uint16_t mark) {
    // Exact cSTETerrain::TrackShoreWithLandToTheRight @ 0x100739610.
    //   x = vertex % stride, z = vertex / stride (div @ 0x10073962f)
    //   sea-direction scan: directions 0..7 (W..SW order, rsi += 4 @
    //   0x10073968a), in-bounds neighbor with seaLevel > alt ends the scan
    //   with seaDir = d (ja -> inc r10d @ 0x10073978e); no sea -> out
    //   (@ 0x100739694..0x10073969d).
    //   walk @ 0x1007396a3..0x100739779: first dir = (seaDir + 1) & 7; per
    //   direction, an in-bounds unmarked shore vertex (flags & 0x10, flags
    //   & mark == 0) is stepped onto — mark it, push_back, move, seaDir =
    //   (seaDir + 5) & 7, next dir = seaDir + 1; a land neighbor (alt >
    //   seaLevel) ends the walk; the full circle (dir wraps to seaDir)
    //   ends it too.
    if (vertex < 0) {
        return; // model guard
    }
    const Uint32 x = static_cast<Uint32>(vertex) % static_cast<Uint32>(mVertexCountX);
    const Uint32 z = static_cast<Uint32>(vertex) / static_cast<Uint32>(mVertexCountX);
    Sint32 cx = static_cast<Sint32>(x);
    Sint32 cz = static_cast<Sint32>(z);
    Sint32 seaDir = -1;
    for (Sint32 d = 0; d < 8; ++d) {
        const Sint32 nx = cx + kXIncrementTable[d];
        if (static_cast<Uint32>(mVertexCountX) <= static_cast<Uint32>(nx)) {
            continue; // jbe @ 0x10073964f
        }
        const Sint32 nz = cz + kZIncrementTable[d];
        if (static_cast<Uint32>(mVertexCountZ) <= static_cast<Uint32>(nz)) {
            continue; // jbe @ 0x10073965e
        }
        const Sint32 n = nz * mVertexCountX + nx;
        if (static_cast<std::size_t>(n) >= mAltitude.size()) {
            continue; // model guard
        }
        if (mSeaLevel > mAltitude[static_cast<std::size_t>(n)]) {
            seaDir = d;
            break;
        }
    }
    if (seaDir < 0) {
        return;
    }
    Sint32 dir = (seaDir + 1) & 7;
    for (;;) {
        const Sint32 nx = cx + kXIncrementTable[dir];
        const Sint32 nz = cz + kZIncrementTable[dir];
        bool stepped = false;
        if (static_cast<Uint32>(mVertexCountX) > static_cast<Uint32>(nx) &&
            static_cast<Uint32>(mVertexCountZ) > static_cast<Uint32>(nz)) {
            const Sint32 n = nz * mVertexCountX + nx;
            if (static_cast<std::size_t>(n) < mAltitude.size()) {
                const uint16_t flags = mVertexFlags[static_cast<std::size_t>(n)];
                if ((flags & 0x10) != 0 && (flags & mark) == 0) {
                    OrVertexFlagBits(mVertexFlags, n, mark); // or @ 0x100739736
                    shore.push_back(n);                      // @ 0x10073974a
                    cx = nx;
                    cz = nz;
                    // Both values derive from the PRE-update seaDir register
                    // (rax loaded once @ 0x100735755): newSeaDir = (old+5)&7
                    // @ 0x100735759/0x10073575d, and the next probe dir =
                    // (old+6)&7 @ 0x100735761/0x100739764 — i.e. (newSeaDir
                    // + 1) & 7, NOT (newSeaDir + 6) & 7.
                    const Sint32 oldSeaDir = seaDir;
                    seaDir = (oldSeaDir + 5) & 7;
                    dir = (oldSeaDir + 6) & 7;
                    stepped = true;
                } else if (mAltitude[static_cast<std::size_t>(n)] > mSeaLevel) {
                    return; // land ends the walk @ 0x100739722
                }
            }
        }
        if (stepped) {
            continue;
        }
        dir = (dir + 1) & 7;
        if (dir == seaDir) {
            return; // full circle @ 0x10073972a
        }
    }
}

void cSTETerrain::TrackShoreWithLandToTheLeft(Sint32 vertex, std::deque<Sint32>& shore,
                                              uint16_t mark) {
    // Exact cSTETerrain::TrackShoreWithLandToTheLeft @ 0x100739798.
    //   Mirror of the Right walker: the sea-direction scan runs directions
    //   7 down to 0 (dec r9 @ 0x10073980d), the walk advances dir downward
    //   ((dir == 0) ? 7 : dir - 1), steps push_FRONT (@ 0x1007398d6), and
    //   the post-step sea direction is (dir + 4) & 7 with the next dir
    //   mapped 0 -> 7 (dec/cmove @ 0x1007398fa..0x10073990b).
    if (vertex < 0) {
        return; // model guard
    }
    const Uint32 x = static_cast<Uint32>(vertex) % static_cast<Uint32>(mVertexCountX);
    const Uint32 z = static_cast<Uint32>(vertex) / static_cast<Uint32>(mVertexCountX);
    Sint32 cx = static_cast<Sint32>(x);
    Sint32 cz = static_cast<Sint32>(z);
    Sint32 seaDir = -1;
    for (Sint32 d = 7; d >= 0; --d) {
        const Sint32 nx = cx + kXIncrementTable[d];
        if (static_cast<Uint32>(mVertexCountX) <= static_cast<Uint32>(nx)) {
            continue; // jbe @ 0x1007397dd
        }
        const Sint32 nz = cz + kZIncrementTable[d];
        if (static_cast<Uint32>(mVertexCountZ) <= static_cast<Uint32>(nz)) {
            continue; // jbe @ 0x1007397e9
        }
        const Sint32 n = nz * mVertexCountX + nx;
        if (static_cast<std::size_t>(n) >= mAltitude.size()) {
            continue; // model guard
        }
        if (mSeaLevel > mAltitude[static_cast<std::size_t>(n)]) {
            seaDir = d;
            break;
        }
    }
    if (seaDir < 0) {
        return;
    }
    // First dir: (seaDir <= 0) ? 7 : seaDir - 1 (cmovle @ 0x100739825).
    Sint32 dir = (seaDir <= 0) ? 7 : seaDir - 1;
    for (;;) {
        if (dir == seaDir) {
            return; // full circle @ 0x100739838
        }
        const Sint32 nx = cx + kXIncrementTable[dir];
        const Sint32 nz = cz + kZIncrementTable[dir];
        bool stepped = false;
        if (static_cast<Uint32>(mVertexCountX) > static_cast<Uint32>(nx) &&
            static_cast<Uint32>(mVertexCountZ) > static_cast<Uint32>(nz)) {
            const Sint32 n = nz * mVertexCountX + nx;
            if (static_cast<std::size_t>(n) < mAltitude.size()) {
                const uint16_t flags = mVertexFlags[static_cast<std::size_t>(n)];
                if ((flags & 0x10) != 0 && (flags & mark) == 0) {
                    OrVertexFlagBits(mVertexFlags, n, mark); // or @ 0x1007398c4
                    shore.push_front(n);                      // @ 0x1007398d6
                    cx = nx;
                    cz = nz;
                    // addl $0x4 / andl $0x7 on r9d @ 0x1007398fa/0x100739908
                    // gives newSeaDir = (dir+4)&7 — and that andl also SETS
                    // the cmov flags: ZF fires iff newSeaDir == 0, so the
                    // remap @ 0x10073990b (cmovel ebx(7), edi after the decl
                    // @ 0x100739906) fires when seaDir == 0, giving dir = 7
                    // (a plain (seaDir-1)&7 would yield the same value; the
                    // cmov shape is reproduced for clarity).
                    seaDir = (dir + 4) & 7;
                    dir = (seaDir == 0) ? 7 : seaDir - 1;
                    stepped = true;
                } else if (mAltitude[static_cast<std::size_t>(n)] > mSeaLevel) {
                    return; // land ends the walk @ 0x10073989f
                }
            }
        }
        if (stepped) {
            continue;
        }
        dir = (dir <= 0) ? 7 : dir - 1; // cmovle @ 0x1007398a9
        if (dir == seaDir) {
            return;
        }
    }
}

void cSTETerrain::MarkRegion(Sint32 radius, bool (cSTETerrain::*qualify)(Sint32) const,
                             bool (cSTETerrain::*markIf)(Sint32) const, uint16_t mark) {
    // Exact cSTETerrain::MarkRegion(int, bool (cSTETerrain::*)(int) const,
    // bool (cSTETerrain::*)(int) const, unsigned short) @ 0x100736728.
    //   vertexCountZ == 0 -> out (@ 0x100736757)
    //   full-grid scan dz in [0, vertexCountZ), dx in [0, vertexCountX);
    //   fn1(vertex) gates the mark square (member-fn call through the
    //   Itanium pair, virtual dispatch for fn word bit 0 @ 0x1007367df —
    //   both call sites pass direct functions, so the model calls directly)
    //   square bounds @ 0x100736845..0x10073683c:
    //     zLo = max(0, dz - radius) (cmovs @ 0x100736813; the loop-head
    //            recompute @ 0x10073677e agrees)
    //     zHi = min(dz + radius, cellCountZ) (cmovle @ 0x100736836)
    //     xLo = max(0, dx - radius) (cmovs @ 0x10073681a)
    //     xHi = min(dx + radius, cellCountX) (cmovg @ 0x10073682d)
    //     zLo > zHi -> skip (jg @ 0x10073683c)
    //   the square rows AND columns are INCLUSIVE of both endpoints
    //   (loop tails @ 0x1007368b9..0x1007368c3 and 0x1007368c5..0x1007368d3).
    if (mVertexCountZ == 0) {
        return;
    }
    for (Sint32 dz = 0; dz < mVertexCountZ; ++dz) {
        if (mVertexCountX == 0) {
            continue; // test ecx, ecx; je row tail @ 0x10073676e
        }
        const Sint32 rowOff = mVertexCountX * dz;
        const Sint32 zLo = (dz - radius < 0) ? 0 : dz - radius;
        const Sint32 zHi = (dz + radius <= mCellCountZ) ? dz + radius : mCellCountZ;
        for (Sint32 dx = 0; dx < mVertexCountX; ++dx) {
            if (!((this->*qualify)(rowOff + dx))) {
                continue; // test al, al; je @ 0x1007367ff
            }
            const Sint32 xLo = (dx - radius < 0) ? 0 : dx - radius;
            const Sint32 xHi = (dx + radius > mCellCountX) ? mCellCountX : dx + radius;
            if (zLo > zHi) {
                continue;
            }
            for (Sint32 z = zLo; z <= zHi; ++z) {
                for (Sint32 x = xLo; x <= xHi; ++x) {
                    const Sint32 v = z * mVertexCountX + x;
                    if (v < 0 || static_cast<std::size_t>(v) >= mVertexFlags.size()) {
                        continue; // model guard
                    }
                    if ((this->*markIf)(v)) {
                        mVertexFlags[static_cast<std::size_t>(v)] |= mark;
                    }
                }
            }
        }
    }
}

void cSTETerrain::SmoothenMarkedRegion(const SC4RectI& rect, Sint32 passes,
                                       uint16_t mark) {
    // Exact cSTETerrain::SmoothenMarkedRegion(SC4Rect<int> const&, int,
    // unsigned short) @ 0x10073697a.
    //   passes <= 0 -> out (jle @ 0x100736990)
    //   per pass: rows [top, bottom] and columns [left, right] INCLUSIVE
    //   (jg row/col skips @ 0x1007369ae/0x1007369be; tails @ 0x100736b66/
    //   0x100736b7f)
    //   per vertex: the flags word is read once (r11w @ 0x1007369fc) and the
    //   neighbor-allowed mask is the GetNeighbors shape for the CURRENT
    //   (x, z) — even directions W/N/E/S compare against
    //   kCrossHtDiffThreshold (16.0f @ 0x100b175fc), odd diagonals against
    //   kDiagonalHtDiffThreshold (8.0f @ 0x100b175f8) (cmove @ 0x100736a80)
    //   gate: ((flags | neighborFlags) & mark) != 0 (@ 0x100736a4d..0x100736a57)
    //   diff > threshold: adjust = (diff - threshold) * kWindErosionFactor;
    //     flags & mark -> LowerVertex(vertex), neighborFlags & mark ->
    //     RaiseVertex(neighbor) (@ 0x100736a8b..0x100736ad1)
    //   diff < -threshold: adjust = (-diff - threshold) *
    //     kWindErosionFactor; Raise(vertex) / Lower(neighbor)
    //     (@ 0x100736ad3..0x100736b2d)
    if (passes <= 0) {
        return;
    }
    const Float32 kCrossHtDiffThreshold = 16.0f;    // @ 0x100b175fc
    const Float32 kDiagonalHtDiffThreshold = 8.0f;  // @ 0x100b175f8
    const Float32 windFactor = WindErosionFactorTunable(); // mulss
                                                         // @ 0x100736a99/0x100736af5
    for (Sint32 pass = 0; pass < passes; ++pass) {
        if (rect.top > rect.bottom) {
            continue;
        }
        for (Sint32 z = rect.top; z <= rect.bottom; ++z) {
            if (rect.left > rect.right) {
                continue;
            }
            for (Sint32 x = rect.left; x <= rect.right; ++x) {
                const Sint32 v = z * mVertexCountX + x;
                if (v < 0 || static_cast<std::size_t>(v) >= mVertexFlags.size()) {
                    continue; // model guard
                }
                const uint16_t flags = mVertexFlags[static_cast<std::size_t>(v)];
                Uint32 mask = (x <= 0) ? 0x7cu : 0xffu;
                if (static_cast<Uint32>(mCellCountX) <= static_cast<Uint32>(x)) {
                    mask &= 0xc7u;
                }
                if (z <= 0) {
                    mask &= 0xf1u;
                }
                if (static_cast<Uint32>(mCellCountZ) <= static_cast<Uint32>(z)) {
                    mask &= 0x1fu;
                }
                for (Sint32 d = 0; d < 8; ++d) {
                    if (((mask >> d) & 1u) == 0) {
                        continue; // test r13d, r12d; je @ 0x100736a29
                    }
                    const Sint32 n = v + NeighborVertexOffset(d, mVertexCountX);
                    if (n < 0 || static_cast<std::size_t>(n) >= mVertexFlags.size()) {
                        continue; // model guard
                    }
                    const uint16_t nflags = mVertexFlags[static_cast<std::size_t>(n)];
                    if (((flags | nflags) & mark) == 0) {
                        continue;
                    }
                    const Float32 diff =
                        mAltitude[static_cast<std::size_t>(v)] -
                        mAltitude[static_cast<std::size_t>(n)];
                    const Float32 threshold =
                        (d % 2 == 0) ? kCrossHtDiffThreshold : kDiagonalHtDiffThreshold;
                    if (diff > threshold) {
                        const Float32 adjust = (diff - threshold) * windFactor;
                        if ((flags & mark) != 0) {
                            LowerVertex(static_cast<Uint32>(v), adjust);
                        }
                        if ((nflags & mark) != 0) {
                            RaiseVertex(static_cast<Uint32>(n), adjust);
                        }
                    } else if (diff < -threshold) {
                        const Float32 adjust = (-diff - threshold) * windFactor;
                        if ((flags & mark) != 0) {
                            RaiseVertex(static_cast<Uint32>(v), adjust);
                        }
                        if ((nflags & mark) != 0) {
                            LowerVertex(static_cast<Uint32>(n), adjust);
                        }
                    }
                }
            }
        }
    }
}

void cSTETerrain::ComputeBoundsOfMarkedRegion(SC4RectI& out, uint16_t mark) const {
    // Exact cSTETerrain::ComputeBoundsOfMarkedRegion(SC4Rect<int>&,
    // unsigned short) const @ 0x1007359a2.
    //   out = kEmptyRect (movaps from 0x100a1c0c0 @ 0x1007359ab)
    //   full-grid scan; a vertex with (flags & mark) expands the bounds
    //   through signed min/max tracking (jge/jle skips
    //   @ 0x100736a06..0x100735a30): left/top take the running minimum,
    //   right/bottom the running maximum.
    out = kEmptyRect;
    if (mVertexCountZ == 0) {
        return; // @ 0x1007359b9
    }
    for (Sint32 z = 0; z < mVertexCountZ; ++z) {
        for (Sint32 x = 0; x < mVertexCountX; ++x) {
            const Sint32 v = z * mVertexCountX + x;
            if (v < 0 || static_cast<std::size_t>(v) >= mVertexFlags.size()) {
                continue; // model guard
            }
            if ((mVertexFlags[static_cast<std::size_t>(v)] & mark) == 0) {
                continue; // test word, dx; je @ 0x100735a04
            }
            if (x < out.left) {
                out.left = x;
            }
            if (x > out.right) {
                out.right = x;
            }
            if (z < out.top) {
                out.top = z;
            }
            if (z > out.bottom) {
                out.bottom = z;
            }
        }
    }
}

bool cSTETerrain::IsVertexWithMark3(Sint32 vertex) const {
    // Exact cSTETerrain::IsVertexWithMark3(int) const @ 0x100736fd4:
    // (record flags byte & 0x4) >> 2, i.e. flag word bit 0x4.
    return (VertexFlags(vertex) & 0x4) != 0;
}

bool cSTETerrain::IsLandAndNotMark3(Sint32 vertex) const {
    // Exact cSTETerrain::IsLandAndNotMark3(int) const @ 0x100736ff2:
    // flag word bit 0x4 set -> false; otherwise altitude > seaLevel (seta).
    if ((VertexFlags(vertex) & 0x4) != 0) {
        return false;
    }
    return AltitudeAt(vertex) > mSeaLevel;
}

bool cSTETerrain::CreateShoreCliffs(bool redisplay) {
    // Exact cSTETerrain::CreateShoreCliffs(bool) @ 0x100736d3e.
    //   DetectShoreVertices()                                  @ 0x100736d54
    //   vertexCountZ == 0 -> return false (je @ 0x100736d60)
    //   full-grid scan (fresh deque per candidate @ 0x100736db3..0x100736dc3):
    //     gate (flags & 0x11) == 0x10 — shore bit set, walk bit clear
    //     (@ 0x100736da3..0x100736dae)
    //     GetShoreStartingAtVert(v, deque, 0x1) @ 0x100736dd2, then
    //     AnalyzeShoreAndCreateCliffs ORs its result into createdAny
    //     (@ 0x100736de2..0x100736dec)
    //   createdAny -> MarkRegion(8, &IsVertexWithMark3, &IsLandAndNotMark3,
    //     0x2) @ 0x100736e26..0x100736e48, ComputeBoundsOfMarkedRegion(rect,
    //     0x2) @ 0x100736e5c, SmoothenMarkedRegion(rect, 20, 0x2)
    //     @ 0x100736e71, and the bool arg gates the vtable+0x228 redisplay
    //     (1, 1, this+0x58, 0) @ 0x100736e76..0x100736e93; return true
    //   the TAIL ALWAYS clears flag bit 0x8 over every vertex
    //   (word and -0x8 @ 0x100736ea9..0x100736ec3) — including the no-cliff
    //   and empty-grid paths.
    DetectShoreVertices();
    bool createdAny = false;
    if (mVertexCountZ != 0) {
        for (Sint32 z = 0; z < mVertexCountZ; ++z) {
            if (mVertexCountX == 0) {
                continue; // test ecx, ecx; je @ 0x100736d76
            }
            for (Sint32 x = 0; x < mVertexCountX; ++x) {
                const Sint32 v = z * mVertexCountX + x;
                if (v < 0 || static_cast<std::size_t>(v) >= mVertexFlags.size()) {
                    continue; // model guard
                }
                const uint16_t flags = mVertexFlags[static_cast<std::size_t>(v)];
                if ((flags & 0x11) != 0x10) {
                    continue;
                }
                std::deque<Sint32> shore;
                GetShoreStartingAtVert(v, shore, 0x1);
                if (AnalyzeShoreAndCreateCliffs(shore)) {
                    createdAny = true;
                }
            }
        }
        if (createdAny) {
            MarkRegion(8, &cSTETerrain::IsVertexWithMark3,
                       &cSTETerrain::IsLandAndNotMark3, 0x2);
            SC4RectI bounds;
            ComputeBoundsOfMarkedRegion(bounds, 0x2);
            SmoothenMarkedRegion(bounds, 20, 0x2);
            if (redisplay) {
                ReDisplayTerrain(1, 1, mWorldRect, 0);
            }
        }
    }
    // Tail: clear the cliff bit everywhere (always runs).
    if (mVertexCount != 0) {
        const uint16_t inv = static_cast<uint16_t>(~0x8);
        for (Sint32 i = 0; i < mVertexCount; ++i) {
            mVertexFlags[static_cast<std::size_t>(i)] &= inv;
        }
    }
    return createdAny;
}

// ---- (f) sea/terrain level changes + redisplay min/max ----

bool cSTETerrain::ChangeSeaLevel(bool raiseLevel) {
    // Exact cSTETerrain::ChangeSeaLevel(bool) @ 0x100735714: tail-jmps
    // vtable+0x168 (SetSeaLevel @ 0x10073e556) with mSeaLevel +
    // kSeaRaiseLowerAmount (20.0f @ 0x100b175dc; negated through the
    // sign-mask xorps @ 0x100a16ac0 when the bool is false, addss
    // @ 0x10073e3e) — the callee's bool return propagates (jmp rax
    // @ 0x10073e543).
    Float32 delta = SeaRaiseLowerAmountTunable();
    if (!raiseLevel) {
        delta = -delta;
    }
    return SetSeaLevel(mSeaLevel + delta);
}

void cSTETerrain::ChangeTerrainLevel(bool raise, Float32 amount) {
    // Exact cSTETerrain::ChangeTerrainLevel(bool, float) @ 0x100735746.
    //   vertexCountZ == 0 skips the loops (je @ 0x100735768/0x1007357a6)
    //   per row: countX <= 0 skips the inner loop (cmp start, start+countX;
    //     jge @ 0x10073577c/0x1007357ba)
    //   every vertex v = z*stride + x: RaiseVertex(v, amount) on the raise
    //     path (@ 0x10073578a) / LowerVertex(v, amount) on the lower path
    //     (@ 0x1007357c8)
    //   tail: vtable+0x228 with (1, 1, this+0x58, 1) @ 0x1007357e0..0x1007357fe
    //   (always, both paths).
    if (mVertexCountZ != 0) {
        for (Sint32 z = 0; z < mVertexCountZ; ++z) {
            if (mVertexCountX <= 0) {
                continue;
            }
            for (Sint32 x = 0; x < mVertexCountX; ++x) {
                const Sint32 v = z * mVertexCountX + x;
                if (raise) {
                    RaiseVertex(static_cast<Uint32>(v), amount);
                } else {
                    LowerVertex(static_cast<Uint32>(v), amount);
                }
            }
        }
    }
    ReDisplayTerrain(1, 1, mWorldRect, 1);
}

void cSTETerrain::ComputeMinAndMaxAltitudeForTerrainRedisplay(const SC4RectI& rect,
                                                              Float32& outMin,
                                                              Float32& outMax) {
    // Exact cSTETerrain::ComputeMinAndMaxAltitudeForTerrainRedisplay @
    // 0x100735a58.
    //   rect == cached world rect (+0x58, four dword compares
    //   @ 0x100735a81..0x100735aa6) -> cached mMinHeight/mMaxHeight
    //   (@ 0x100735aa8..0x100735abe); else FindMinAndMaxHeights(rect)
    //   (@ 0x100735acc)
    if (rect.left == mWorldRect.left && rect.top == mWorldRect.top &&
        rect.right == mWorldRect.right && rect.bottom == mWorldRect.bottom) {
        outMin = mMinHeight;
        outMax = mMaxHeight;
    } else {
        FindMinAndMaxHeights(rect, outMin, outMax);
    }
    bool minZeroed = false; // "al" — the visible-edge zeroing flag
    const TerrainViewUtilitiesModel& vu = ViewUtilitiesModel();
    if (vu.present) {
        // Edge-visibility bools @ 0x100735ae2..0x100735b17:
        //   [0] top <= 0, [1] right >= cellCountX (unsigned jb), [2] bottom
        //   >= cellCountZ, [3] left <= 0 — then SC4ViewUtilities::
        //   GetVisibleEdges(int&, int&) @ 0x10017376a fills the edge pair
        //   and either visible edge zeroes outMin (al = 1)
        //   @ 0x100735b23..0x100735b43.
        bool edges[4] = {false, false, false, false};
        if (rect.top <= 0) {
            edges[0] = true;
        }
        if (static_cast<Uint32>(mCellCountX) <= static_cast<Uint32>(rect.right)) {
            edges[1] = true;
        }
        if (static_cast<Uint32>(mCellCountZ) <= static_cast<Uint32>(rect.bottom)) {
            edges[2] = true;
        }
        if (rect.left <= 0) {
            edges[3] = true;
        }
        // clean-room fallback (not authentic): the edge pair is settable;
        // out-of-range indices read as not-visible (the binary indexes the
        // bool array unchecked).
        const auto visible = [&](Sint32 e) { return e >= 0 && e <= 3 && edges[e]; };
        if (visible(vu.edge0) || visible(vu.edge1)) {
            outMin = 0.0f;
            minZeroed = true;
        }
    }
    // Candidate range from the +0x110 view (+0x94 max, +0x98 min)
    // @ 0x100735b45..0x100735b62; without a view the candidates are
    // +FLT_MAX (0x100a161e0) and -FLT_MAX (0x100a169a0).
    Float32 candMax;
    Float32 candMin;
    if (mView3D != nullptr) {
        candMax = mView3D->maxField94;
        candMin = mView3D->minField98;
    } else {
        candMin = -3.40282346638528859812e+38F; // 0xff7fffff @ 0x100a169a0
        candMax = 3.40282346638528859812e+38F;   // 0x7f7fffff @ 0x100a161e0
    }
    bool maxReplaced = false; // "cl"
    if (candMax > mMaxHeight) {
        outMax = candMax; // @ 0x100735b7f
        maxReplaced = true;
    }
    if (!minZeroed) {
        if (mMinHeight > candMin) {
            outMin = candMin; // ja @ 0x100735b9b -> 0x100735bbe
        } else {
            outMin = outMin + (-20.0f); // eps @ 0x100a28e6c (0xc1a00000)
            candMin = (candMin < mMinHeight) ? candMin : mMinHeight; // minss
            if (candMin > outMin) {
                outMin = candMin; // @ 0x100735bbe
            }
        }
    }
    if (!maxReplaced) {
        outMax = outMax + 20.0f; // eps @ 0x100a18408 (0x41a00000)
        candMax = (candMax > mMaxHeight) ? candMax : mMaxHeight; // maxss
        if (outMax > candMax) {
            outMax = candMax; // ucomiss outMax', candMax'; jbe skips @ 0x100735be6
        }
    }
    if (0.0f > outMin) {
        outMin = 0.0f; // @ 0x100735bf6
    }
    const Float32 maxHeight = MaxTerrainHeightTunable(); // 0x100b1763c
    if (outMax > maxHeight) {
        outMax = maxHeight; // @ 0x100735c0f
    }
    if (mSeaLevel > outMax) {
        outMax = mSeaLevel; // @ 0x100735c26
    }
}

} // namespace opensc4
