#pragma once

#include "opensc4/city.h"
#include "opensc4/formats.h"
#include <array>
#include <unordered_map>
#include <unordered_set>
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
    std::size_t gpuVertexCount = 0;
    bool verticesDirty = false;
    bool isTerrain = false;      // Tiled (GL_REPEAT) and drawn before the translucent water
    bool isTerrainBlend = false; // Neighbour-weighted overlay drawn after every opaque terrain layer
    bool isAutomata = false;     // Cars and sims; refilled every frame
};

class cSC4GLCityRenderer {
public:
    cSC4GLCityRenderer();
    ~cSC4GLCityRenderer();

    bool InitGL();
    void ShutdownGL();

    void RebuildCityGeometry(const cSC4City& city, DataOverlay overlay = DataOverlay::None);
    void RenderFrame(int viewportWidth, int viewportHeight, const cSC4City& city);

    void AttachDBPFSystem(const cDBPFSystem* dbpfSys) noexcept {
        if (dbpfSys != mDbpfSys) RetireTexturedBatches();
        mDbpfSys = dbpfSys;
    }
    const cDBPFSystem* AttachedDBPFSystem() const noexcept { return mDbpfSys; }
    std::size_t BuildTexturedBatchesFromDBPF(const cSC4City& city, const cDBPFSystem& dbpfSys);
    const std::vector<TexturedMeshBatch>& TexturedBatches() const noexcept { return mTexturedBatches; }
    std::size_t TotalVertexCount() const noexcept { return mVertices.size(); }

    // SC4's zoom-5 camera, recovered by fitting the zoom-5 model UVs (which are
    // an orthographic projection of their vertices): 45° pitch, 67.5° yaw in
    // model space, plus 90° per rotation.
    static constexpr float kSC4ViewYaw   = 1.17809725f;
    static constexpr float kSC4ViewPitch = 0.78539816f;

    // Camera controls: supports both classic SC4 4-rotation isometric snap
    // AND full 360° free orbit (yaw + pitch + perspective/orthographic toggle).
    void Pan(float dxWorld, float dzWorld) noexcept;
    void ZoomBy(float factor) noexcept;
    void Rotate90(int dir) noexcept; // +1 or -1 (snaps/steps yaw by 90°)
    void OrbitFree(float deltaYawRadians, float deltaPitchRadians) noexcept;
    void SetFreeCameraAngles(float yawRadians, float pitchRadians) noexcept;
    void SetPerspectiveMode(bool perspective) noexcept { mPerspectiveMode = perspective; }
    bool IsPerspectiveMode() const noexcept { return mPerspectiveMode; }
    void ResetCamera(const cSC4City& city) noexcept;
    void SetCameraTarget(float x, float y, float z, float zoom) noexcept {
        mCamTargetX = x;
        mCamTargetY = y;
        mCamTargetZ = z;
        mZoom = zoom;
    }

    // Raycast screen pixel to city tile (cx, cz) on the terrain surface
    bool ScreenToTile(int screenX, int screenY, int viewportW, int viewportH,
                      const cSC4City& city, int& outTileX, int& outTileZ) const noexcept;

    void SetHoveredTile(int tx, int tz) noexcept {
        if (tx == mHoverTileX && tz == mHoverTileZ) return;
        mHoverTileX = tx;
        mHoverTileZ = tz;
        mHoverDirty = true;
    }
    int HoveredTileX() const noexcept { return mHoverTileX; }
    int HoveredTileZ() const noexcept { return mHoverTileZ; }
    bool IsGeometryDirty() const noexcept { return mGeometryDirty; }
    Uint32 RebuildCount() const noexcept { return mRebuildCount; }

    // Slides automata along their lane. The fractional part is used.
    void SetAnimPhase(float phase) noexcept;
    float AnimPhase() const noexcept { return mAnimPhase; }

    // Moves the saved cars and sims along the street grid. RenderFrame calls
    // this with wall-clock time; tools may call it directly.
    void StepAutomata(const cSC4City& city, float dtSeconds);
    std::size_t ActiveVehicleCount() const noexcept;
    std::size_t ActivePedestrianCount() const noexcept;

    DataOverlay CurrentOverlay() const noexcept { return mOverlay; }
    void SetOverlay(DataOverlay ov, const cSC4City& city);

    float CameraZoom() const noexcept { return mZoom; }
    int CameraRotationIndex() const noexcept { return mRotationIndex; }
    float CameraYawRadians() const noexcept { return mYawRadians; }
    float CameraPitchRadians() const noexcept { return mPitchRadians; }

private:
    void BuildViewProjMatrix(int viewportW, int viewportH, float outVP[16]) const noexcept;
    void UploadHoverCursor(const cSC4City& city);
    void RetireTexturedBatches();
    void SyncGpuResources();

    struct AutomatonAgent {
        bool pedestrian = false;
        bool active = true;
        cSC4TGI model{};       // S3D model, or sprite animation for sims
        int cellX = 0;
        int cellZ = 0;
        Uint8 dirIn = 0;       // 0 +X, 1 +Z, 2 -X, 3 -Z
        Uint8 dirOut = 0;
        float progress = 0.0f; // Metres through the cell, 0..16
        float lateral = 0.0f;  // Metres right of the travel direction
        float walked = 0.0f;   // Drives the walk cycle
        Uint32 rng = 0;
    };
    struct SpriteFrame {
        Uint8 x, y, w, h, anchorX, anchorY;
    };
    struct PedestrianSprite {
        std::size_t batch = 0;
        int framesPerDirection = 0;
        float metresPerPixel = 1.0f / 7.0f;
        std::vector<SpriteFrame> frames; // Direction-major
    };
    void RebuildDriveCells(const cSC4City& city);
    void RebuildAgents(const cSC4City& city);
    bool DriveCell(int cx, int cz) const noexcept;
    Uint8 ChooseExit(AutomatonAgent& agent, int cx, int cz, Uint8 dirIn) const noexcept;
    void AgentPose(const AutomatonAgent& agent, float progress, float& x, float& y, float& z) const noexcept;
    float DriveSpeed(const AutomatonAgent& agent) const noexcept;
    TexturedMeshBatch* AutomataBatch(const cSC4TGI& fsh, bool instanceOnly);
    const cS3DModel* AutomataModel(const cSC4TGI& key);
    const PedestrianSprite* PedestrianSpriteFor(const cSC4TGI& key);
    void AppendAutomataGeometry(int viewportH);

    std::vector<AutomatonAgent> mAgents;
    const void* mAgentSource = nullptr;
    std::size_t mAgentSourceCount = 0;
    // Per cell: network index of a drivable piece, or -1, plus its corner heights.
    std::vector<Sint32> mDriveNet;
    std::vector<std::array<float, 4>> mDriveHeights;
    std::vector<NetworkType> mDriveType;
    int mDriveCellsX = 0;
    int mDriveCellsZ = 0;
    bool mDriveCellsDirty = true;
    std::unordered_map<Uint64, std::size_t> mAutomataBatchIndex;
    std::unordered_map<Uint32, cS3DModel> mAutomataModels;
    std::unordered_map<Uint32, PedestrianSprite> mPedestrianSprites;
    std::unordered_set<Uint32> mMissingPedestrianSprites;
    double mAutomataClock = -1.0;

    bool mGLReady        = false;
    Uint32 mShaderProg   = 0;
    Uint32 mTexShaderProg = 0;
    Uint32 mVao          = 0;
    Uint32 mVbo          = 0;
    std::size_t mVertexCount = 0;
    const cDBPFSystem* mDbpfSys = nullptr;
    // One batch per FSH instance, kept across rebuilds so textures are decoded
    // and uploaded once; rebuilds only refill vertices.
    std::vector<TexturedMeshBatch> mTexturedBatches;
    std::unordered_map<Uint32, std::size_t> mBatchIndexByFsh;
    std::unordered_map<Uint32, std::size_t> mTerrainBatchIndex; // Texture | 0x100 for the blend layer
    std::unordered_set<Uint32> mMissingFsh;
    // Terrain.ini [*TextureMapTable]: 32 temperature rows x 16 moisture columns.
    std::vector<Uint8> mTerrainTextureTable;
    Uint8 mTerrainHighCliff = 0;
    Uint8 mTerrainLowCliff = 0;
    Uint8 mTerrainBeach = 0;
    bool mTerrainIniLoaded = false;
    bool mTexturedTerrain = false;
    std::unordered_map<Uint32, cS3DModel> mModelCache;
    int mModelRotationIndex = 0;
    // Rebuilds can run outside the GL context (e.g. from the sim timer), so
    // they only stage CPU data; RenderFrame frees and uploads GL objects.
    std::vector<TexturedMeshBatch> mRetiredBatches;
    bool mCityVboStale = false;
    std::vector<Uint8> mTexturedBuildingMask;
    std::vector<Uint8> mTexturedNetworkMask;
    std::vector<Uint8> mTexturedFloraMask;
    std::vector<Uint8> mLotTextureCellMask; // Row-major cells covered by a lot base texture

    float mCamTargetX    = 1024.0f;
    float mCamTargetY    = 255.0f;
    float mCamTargetZ    = 1024.0f;
    float mZoom          = 1.0f; // 0.15 .. 8.0
    int mRotationIndex   = 0;    // Nearest 90° cardinal quadrant (0..3)
    float mYawRadians    = kSC4ViewYaw;
    float mPitchRadians  = kSC4ViewPitch; // Clamped 8°..85° when orbiting freely
    bool mPerspectiveMode = false;        // Model sprites are pre-rendered for SC4's orthographic view

    int mHoverTileX      = -1;
    int mHoverTileZ      = -1;
    float mAnimPhase     = 0.0f;
    bool mGeometryDirty  = false;
    bool mHoverDirty     = false;
    Uint32 mRebuildCount = 0;
    DataOverlay mOverlay = DataOverlay::None;

    // Tile highlight plus four rim quads. Kept off the city mesh so a hover
    // change does not rebuild the 235k-vertex city buffer.
    Uint32 mCursorVao = 0;
    Uint32 mCursorVbo = 0;
    std::array<RenderVertex, 30> mCursorVertices{};
    std::size_t mCursorVertexCount = 0;

    std::vector<RenderVertex> mVertices;
};

} // namespace opensc4
