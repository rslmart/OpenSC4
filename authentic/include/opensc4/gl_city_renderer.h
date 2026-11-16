#pragma once

#include "opensc4/city.h"
#include <unordered_map>
#include <vector>

namespace opensc4 {

struct RenderVertex {
    float x, y, z;
    float nx, ny, nz;
    float r, g, b, a;
};

struct TexturedRenderVertex {
    float x, y, z;
    float u, v;
    float r, g, b, a;
};

struct TexturedMeshBatch {
    Uint32 fshInstanceId = 0;
    Uint16 texWidth = 0;
    Uint16 texHeight = 0;
    std::vector<Uint8> rgbaPixels;
    std::vector<TexturedRenderVertex> vertices;
    Uint32 glTex = 0;
    Uint32 vao = 0;
    Uint32 vbo = 0;
    // cSTETerrainView3D draws terrain as repeating FSH tiles before water.
    bool isTerrain = false;
    bool isTerrainBlend = false;
};

class cSC4GLCityRenderer {
public:
    cSC4GLCityRenderer();
    ~cSC4GLCityRenderer();

    bool InitGL();
    void ShutdownGL();

    void RebuildCityGeometry(const cSC4City& city, DataOverlay overlay = DataOverlay::None);
    void RenderFrame(int viewportWidth, int viewportHeight, const cSC4City& city);

    void AttachDBPFSystem(const cDBPFSystem* dbpfSys) noexcept { mDbpfSys = dbpfSys; }
    const cDBPFSystem* AttachedDBPFSystem() const noexcept { return mDbpfSys; }
    std::size_t BuildTexturedBatchesFromDBPF(const cSC4City& city, const cDBPFSystem& dbpfSys);
    const std::vector<TexturedMeshBatch>& TexturedBatches() const noexcept { return mTexturedBatches; }
    std::size_t TotalVertexCount() const noexcept { return mVertices.size(); }
    std::size_t TexturedBuildingCount() const noexcept {
        std::size_t n = 0;
        for (Uint8 v : mTexturedBuildingMask) if (v != 0) ++n;
        return n;
    }
    bool IsBuildingTextured(std::size_t index) const noexcept {
        return index < mTexturedBuildingMask.size() && mTexturedBuildingMask[index] != 0;
    }
    bool IsAutomatonTextured(std::size_t index) const noexcept {
        return index < mTexturedPedestrianMask.size() && mTexturedPedestrianMask[index] != 0;
    }
    std::size_t TexturedFloraCount() const noexcept {
        std::size_t n = 0;
        for (Uint8 v : mTexturedFloraMask) if (v != 0) ++n;
        return n;
    }
    std::size_t TexturedPedestrianCount() const noexcept {
        std::size_t n = 0;
        for (Uint8 v : mTexturedPedestrianMask) if (v != 0) ++n;
        return n;
    }

    // Camera controls matching SC4's isometric 4-rotation + 5-zoom camera
    void Pan(float dxWorld, float dzWorld) noexcept;
    void ZoomBy(float factor) noexcept;
    void Rotate90(int dir) noexcept; // +1 or -1
    void ResetCamera(const cSC4City& city) noexcept;

    // Raycast screen pixel to city tile (cx, cz) on the terrain surface
    bool ScreenToTile(int screenX, int screenY, int viewportW, int viewportH,
                      const cSC4City& city, int& outTileX, int& outTileZ) const noexcept;

    void SetHoveredTile(int tx, int tz) noexcept {
        if (tx == mHoverTileX && tz == mHoverTileZ) return;
        mHoverTileX = tx;
        mHoverTileZ = tz;
        mGeometryDirty = true;
    }
    int HoveredTileX() const noexcept { return mHoverTileX; }
    int HoveredTileZ() const noexcept { return mHoverTileZ; }

    // Slides automata along their lane. The fractional part is used.
    void SetAnimPhase(float phase) noexcept;
    float AnimPhase() const noexcept { return mAnimPhase; }

    DataOverlay CurrentOverlay() const noexcept { return mOverlay; }
    void SetOverlay(DataOverlay ov, const cSC4City& city);

    float CameraZoom() const noexcept { return mZoom; }
    int CameraRotationIndex() const noexcept { return mRotationIndex; }

private:
    void BuildViewProjMatrix(int viewportW, int viewportH, float outVP[16]) const noexcept;
    void RetireTexturedBatches();
    void SyncGpuResources();

    bool mGLReady        = false;
    Uint32 mShaderProg   = 0;
    Uint32 mTexShaderProg = 0;
    Uint32 mVao          = 0;
    Uint32 mVbo          = 0;
    std::size_t mVertexCount = 0;
    const cDBPFSystem* mDbpfSys = nullptr;
    std::vector<TexturedMeshBatch> mTexturedBatches;
    // Rebuilds can run outside the GL context (e.g. from the sim timer), so
    // they only stage CPU data; RenderFrame frees and uploads GL objects.
    std::vector<TexturedMeshBatch> mRetiredBatches;
    bool mCityVboStale = false;
    std::vector<Uint8> mTexturedBuildingMask;
    std::vector<Uint8> mTexturedFloraMask;
    std::vector<Uint8> mTexturedPedestrianMask;

    // Terrain.ini [*TextureMapTable] via cSTETerrainView3D::SetUpTexturesForCurrentTerrainTypeAndZoom
    // @ 0x10074688c. FSH group 0x891b0e1a. INI TGI type 0 / group 0x8a5971c5 / instance 0xaa597172.
    std::vector<Uint8> mTerrainTextureTable;
    std::unordered_map<Uint32, std::size_t> mTerrainBatchIndex;
    Uint8 mTerrainHighCliff = 0;
    Uint8 mTerrainLowCliff = 0;
    Uint8 mTerrainBeach = 0;
    bool mTerrainIniLoaded = false;
    bool mTexturedTerrain = false;

    float mCamTargetX    = 1024.0f;
    float mCamTargetY    = 255.0f;
    float mCamTargetZ    = 1024.0f;
    float mZoom          = 1.0f; // 0.35 (Zoom 1) .. 4.5 (Zoom 5)
    int mRotationIndex   = 0;    // 0, 1, 2, 3 (45°, 135°, 225°, 315° azimuth)

    int mHoverTileX      = -1;
    int mHoverTileZ      = -1;
    float mAnimPhase     = 0.0f;
    bool mGeometryDirty  = false;
    DataOverlay mOverlay = DataOverlay::None;

    std::vector<RenderVertex> mVertices;
};

} // namespace opensc4
