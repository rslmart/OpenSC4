#include "opensc4/gl_city_renderer.h"
#include "opensc4/dbpf.h"
#include "opensc4/formats.h"
#include <cmath>
#include <algorithm>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <cctype>
#include <string>

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#elif defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#endif

namespace opensc4 {

static void PushTriangle(std::vector<RenderVertex>& out,
                         float x0, float y0, float z0,
                         float x1, float y1, float z1,
                         float x2, float y2, float z2,
                         float r, float g, float b, float a = 1.0f) {
    float ux = x1 - x0, uy = y1 - y0, uz = z1 - z0;
    float vx = x2 - x0, vy = y2 - y0, vz = z2 - z0;
    float nx = uy * vz - uz * vy;
    float ny = uz * vx - ux * vz;
    float nz = ux * vy - uy * vx;
    float len = std::sqrt(nx * nx + ny * ny + nz * nz);
    if (len > 1e-5f) { nx /= len; ny /= len; nz /= len; }
    else { nx = 0.0f; ny = 1.0f; nz = 0.0f; }

    out.push_back({x0, y0, z0, nx, ny, nz, r, g, b, a});
    out.push_back({x1, y1, z1, nx, ny, nz, r, g, b, a});
    out.push_back({x2, y2, z2, nx, ny, nz, r, g, b, a});
}

static void PushQuad(std::vector<RenderVertex>& out,
                     float x0, float y0, float z0,
                     float x1, float y1, float z1,
                     float x2, float y2, float z2,
                     float x3, float y3, float z3,
                     float r, float g, float b, float a = 1.0f) {
    PushTriangle(out, x0, y0, z0, x1, y1, z1, x2, y2, z2, r, g, b, a);
    PushTriangle(out, x0, y0, z0, x2, y2, z2, x3, y3, z3, r, g, b, a);
}

static void PushBox(std::vector<RenderVertex>& out,
                    float minX, float minY, float minZ,
                    float maxX, float maxY, float maxZ,
                    float r, float g, float b,
                    float roofR, float roofG, float roofB) {
    // Top roof
    PushQuad(out,
             minX, maxY, minZ,
             minX, maxY, maxZ,
             maxX, maxY, maxZ,
             maxX, maxY, minZ,
             roofR, roofG, roofB, 1.0f);
    // South (+Z)
    PushQuad(out,
             minX, minY, maxZ,
             maxX, minY, maxZ,
             maxX, maxY, maxZ,
             minX, maxY, maxZ,
             r, g, b, 1.0f);
    // North (-Z)
    PushQuad(out,
             maxX, minY, minZ,
             minX, minY, minZ,
             minX, maxY, minZ,
             maxX, maxY, minZ,
             r * 0.85f, g * 0.85f, b * 0.85f, 1.0f);
    // East (+X)
    PushQuad(out,
             maxX, minY, maxZ,
             maxX, minY, minZ,
             maxX, maxY, minZ,
             maxX, maxY, maxZ,
             r * 0.92f, g * 0.92f, b * 0.92f, 1.0f);
    // West (-X)
    PushQuad(out,
             minX, minY, minZ,
             minX, minY, maxZ,
             minX, maxY, maxZ,
             minX, maxY, minZ,
             r * 0.78f, g * 0.78f, b * 0.78f, 1.0f);
}

static_assert(sizeof(TexturedRenderVertex) == 9 * sizeof(float),
              "TexturedRenderVertex must stay tightly packed for the UV shader");

static void ReleaseTexturedBatchGL(TexturedMeshBatch& batch) {
    if (batch.vbo != 0) {
        glDeleteBuffers(1, &batch.vbo);
        batch.vbo = 0;
    }
    if (batch.vao != 0) {
        glDeleteVertexArrays(1, &batch.vao);
        batch.vao = 0;
    }
    if (batch.glTex != 0) {
        glDeleteTextures(1, &batch.glTex);
        batch.glTex = 0;
    }
}

static void UploadTexturedBatch(TexturedMeshBatch& batch) {
    if (batch.vertices.empty() || batch.texWidth == 0 || batch.texHeight == 0) return;
    const std::size_t pixelBytes =
        static_cast<std::size_t>(batch.texWidth) * static_cast<std::size_t>(batch.texHeight) * 4u;
    if (batch.rgbaPixels.size() < pixelBytes) return;

    // FSH rows are top-down and SC4's Direct3D-style UVs put v = 0 on the top
    // row, so upload rows unflipped. Flipping mirrors every texture, which is
    // invisible on symmetric straights but turns diagonal pieces 90 degrees.
    glGenTextures(1, &batch.glTex);
    glBindTexture(GL_TEXTURE_2D, batch.glTex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    const GLint wrap = batch.isTerrain ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrap);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
                 static_cast<GLsizei>(batch.texWidth),
                 static_cast<GLsizei>(batch.texHeight),
                 0, GL_RGBA, GL_UNSIGNED_BYTE, batch.rgbaPixels.data());
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, 0);

    const auto stride = static_cast<GLsizei>(sizeof(TexturedRenderVertex));
    glGenVertexArrays(1, &batch.vao);
    glGenBuffers(1, &batch.vbo);
    glBindVertexArray(batch.vao);
    glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(batch.vertices.size() * sizeof(TexturedRenderVertex)),
                 batch.vertices.data(),
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride,
                          reinterpret_cast<const void*>(5 * sizeof(float)));
    glBindVertexArray(0);
}

// SC4 S3D vertex colors are Direct3D A8R8G8B8 (B,G,R,A in little-endian memory).
static void UnpackS3DColor(Uint32 color, float& r, float& g, float& b, float& a) noexcept {
    r = static_cast<float>((color >> 16) & 0xFFu) / 255.0f;
    g = static_cast<float>((color >>  8) & 0xFFu) / 255.0f;
    b = static_cast<float>( color        & 0xFFu) / 255.0f;
    a = static_cast<float>((color >> 24) & 0xFFu) / 255.0f;
}

// Quarter-turns about Y. Orientation 1 is +90 degrees (x, z) -> (-z, x).
static void PlaceModelVertex(const S3DVertex& v,
                             float modelCenterX, float modelGroundY, float modelCenterZ,
                             float anchorX, float anchorY, float anchorZ,
                             unsigned orientation,
                             float& outX, float& outY, float& outZ) noexcept {
    const float lx = v.x - modelCenterX;
    const float ly = v.y - modelGroundY;
    const float lz = v.z - modelCenterZ;
    float rx = lx;
    float rz = lz;
    switch (orientation & 3u) {
        case 1: rx = -lz; rz =  lx; break;
        case 2: rx = -lx; rz = -lz; break;
        case 3: rx =  lz; rz = -lx; break;
        default: break;
    }
    outX = anchorX + rx;
    outY = anchorY + ly;
    outZ = anchorZ + rz;
}

static void AppendTexturedVertex(std::vector<TexturedRenderVertex>& out,
                                 float x, float y, float z, float u, float v,
                                 float r, float g, float b, float a) {
    out.push_back(TexturedRenderVertex{x, y, z, u, v, r, g, b, a});
}

cSC4GLCityRenderer::cSC4GLCityRenderer() = default;

cSC4GLCityRenderer::~cSC4GLCityRenderer() {
    ShutdownGL();
}

static Uint32 CompileShader(Uint32 type, const char* src) {
    Uint32 sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);
    return sh;
}

static Uint32 CompileTexturedProgram() {
#if defined(__EMSCRIPTEN__)
    const char* vsSrc = R"(#version 300 es
        precision highp float;
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aUV;
        layout(location = 2) in vec4 aColor;
        uniform mat4 uViewProj;
        out vec2 vUV;
        out vec4 vColor;
        void main() {
            vUV = aUV;
            vColor = aColor;
            gl_Position = uViewProj * vec4(aPos, 1.0);
        }
    )";
    const char* fsSrc = R"(#version 300 es
        precision mediump float;
        uniform sampler2D uTex;
        in vec2 vUV;
        in vec4 vColor;
        out vec4 fragColor;
        void main() {
            vec4 color = texture(uTex, vUV) * vColor;
            if (color.a < 0.05) discard;
            fragColor = color;
        }
    )";
#else
    const char* vsSrc = R"(#version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec2 aUV;
        layout(location = 2) in vec4 aColor;
        uniform mat4 uViewProj;
        out vec2 vUV;
        out vec4 vColor;
        void main() {
            vUV = aUV;
            vColor = aColor;
            gl_Position = uViewProj * vec4(aPos, 1.0);
        }
    )";
    const char* fsSrc = R"(#version 330 core
        uniform sampler2D uTex;
        in vec2 vUV;
        in vec4 vColor;
        out vec4 fragColor;
        void main() {
            vec4 color = texture(uTex, vUV) * vColor;
            if (color.a < 0.05) discard;
            fragColor = color;
        }
    )";
#endif
    Uint32 vs = CompileShader(GL_VERTEX_SHADER, vsSrc);
    Uint32 fs = CompileShader(GL_FRAGMENT_SHADER, fsSrc);
    Uint32 prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return prog;
}

bool cSC4GLCityRenderer::InitGL() {
    if (mGLReady) return true;

#if defined(__EMSCRIPTEN__)
    const char* vsSrc = R"(#version 300 es
        precision highp float;
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec4 aColor;
        uniform mat4 uViewProj;
        out vec3 vNormal;
        out vec4 vColor;
        void main() {
            vNormal = aNormal;
            vColor = aColor;
            gl_Position = uViewProj * vec4(aPos, 1.0);
        }
    )";
    const char* fsSrc = R"(#version 300 es
        precision mediump float;
        in vec3 vNormal;
        in vec4 vColor;
        out vec4 fragColor;
        void main() {
            vec3 sunDir = normalize(vec3(0.72, 0.48, 0.22));
            float ndl = max(dot(normalize(vNormal), sunDir), 0.0);
            float light = 0.36 + 0.72 * ndl;
            vec3 warm = vec3(1.08, 0.98, 0.90);
            fragColor = vec4(vColor.rgb * light * warm, vColor.a);
        }
    )";
#else
    const char* vsSrc = R"(#version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec4 aColor;
        uniform mat4 uViewProj;
        out vec3 vNormal;
        out vec4 vColor;
        void main() {
            vNormal = aNormal;
            vColor = aColor;
            gl_Position = uViewProj * vec4(aPos, 1.0);
        }
    )";
    const char* fsSrc = R"(#version 330 core
        in vec3 vNormal;
        in vec4 vColor;
        out vec4 fragColor;
        void main() {
            vec3 sunDir = normalize(vec3(0.72, 0.48, 0.22));
            float ndl = max(dot(normalize(vNormal), sunDir), 0.0);
            float light = 0.36 + 0.72 * ndl;
            vec3 warm = vec3(1.08, 0.98, 0.90);
            fragColor = vec4(vColor.rgb * light * warm, vColor.a);
        }
    )";
#endif

    Uint32 vs = CompileShader(GL_VERTEX_SHADER, vsSrc);
    Uint32 fs = CompileShader(GL_FRAGMENT_SHADER, fsSrc);
    mShaderProg = glCreateProgram();
    glAttachShader(mShaderProg, vs);
    glAttachShader(mShaderProg, fs);
    glLinkProgram(mShaderProg);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glGenVertexArrays(1, &mVao);
    glGenBuffers(1, &mVbo);

    glBindVertexArray(mVao);
    glBindBuffer(GL_ARRAY_BUFFER, mVbo);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), reinterpret_cast<const void*>(0));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), reinterpret_cast<const void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(RenderVertex), reinterpret_cast<const void*>(6 * sizeof(float)));
    glBindVertexArray(0);

    mTexShaderProg = CompileTexturedProgram();

    mGLReady = true;
    return true;
}

void cSC4GLCityRenderer::RetireTexturedBatches() {
    for (auto& batch : mTexturedBatches) {
        if (batch.glTex != 0 || batch.vao != 0 || batch.vbo != 0) {
            mRetiredBatches.push_back(std::move(batch));
        }
    }
    mTexturedBatches.clear();
    mTerrainBatchIndex.clear();
    mTexturedTerrain = false;
}

void cSC4GLCityRenderer::SyncGpuResources() {
    for (auto& batch : mRetiredBatches) ReleaseTexturedBatchGL(batch);
    mRetiredBatches.clear();
    for (auto& batch : mTexturedBatches) {
        if (batch.glTex == 0 && !batch.vertices.empty()) UploadTexturedBatch(batch);
    }
    if (mCityVboStale && mVbo != 0) {
        glBindBuffer(GL_ARRAY_BUFFER, mVbo);
        glBufferData(GL_ARRAY_BUFFER,
                     static_cast<GLsizeiptr>(mVertices.size() * sizeof(RenderVertex)),
                     mVertices.empty() ? nullptr : mVertices.data(),
                     GL_DYNAMIC_DRAW);
        mCityVboStale = false;
    }
}

void cSC4GLCityRenderer::ShutdownGL() {
    if (!mGLReady) return;
    for (auto& batch : mRetiredBatches) ReleaseTexturedBatchGL(batch);
    mRetiredBatches.clear();
    for (auto& batch : mTexturedBatches) ReleaseTexturedBatchGL(batch);
    if (mVbo) glDeleteBuffers(1, &mVbo);
    if (mVao) glDeleteVertexArrays(1, &mVao);
    if (mShaderProg) glDeleteProgram(mShaderProg);
    if (mTexShaderProg) glDeleteProgram(mTexShaderProg);
    mVbo = 0;
    mVao = 0;
    mShaderProg = 0;
    mTexShaderProg = 0;
    mGLReady = false;
}

void cSC4GLCityRenderer::ResetCamera(const cSC4City& city) noexcept {
    if (!city.Buildings().empty()) {
        double sx = 0.0, sy = 0.0, sz = 0.0;
        for (const auto& b : city.Buildings()) {
            sx += 0.5 * (b.minX + b.maxX);
            sy += b.minY;
            sz += 0.5 * (b.minZ + b.maxZ);
        }
        double n = static_cast<double>(city.Buildings().size());
        mCamTargetX = static_cast<float>(sx / n);
        mCamTargetY = static_cast<float>(sy / n);
        mCamTargetZ = static_cast<float>(sz / n);
    } else {
        mCamTargetX = city.CellCountX() * 16.0f * 0.5f;
        mCamTargetZ = city.CellCountZ() * 16.0f * 0.5f;
        mCamTargetY = city.GetCellCenterHeight(city.CellCountX() / 2, city.CellCountZ() / 2);
    }
    mZoom = (city.CellCountX() >= 256) ? 0.65f : (city.CellCountX() >= 128 ? 1.15f : 1.50f);
    mRotationIndex = 0;
}

void cSC4GLCityRenderer::Pan(float dxWorld, float dzWorld) noexcept {
    float angle = (mRotationIndex * 0.5f + 0.25f) * 3.14159265f;
    float c = std::cos(angle);
    float s = std::sin(angle);
    mCamTargetX += (dxWorld * s + dzWorld * c) / std::max(0.25f, mZoom);
    mCamTargetZ += (-dxWorld * c + dzWorld * s) / std::max(0.25f, mZoom);
}

void cSC4GLCityRenderer::ZoomBy(float factor) noexcept {
    mZoom = std::clamp(mZoom * factor, 0.28f, 5.0f);
}

void cSC4GLCityRenderer::Rotate90(int dir) noexcept {
    mRotationIndex = (mRotationIndex + dir + 4) & 3;
    mGeometryDirty = true;
}

void cSC4GLCityRenderer::SetAnimPhase(float phase) noexcept {
    if (phase == mAnimPhase) return;
    mAnimPhase = phase;
    mGeometryDirty = true;
}

void cSC4GLCityRenderer::SetOverlay(DataOverlay ov, const cSC4City& city) {
    mOverlay = ov;
    RebuildCityGeometry(city, ov);
}

static void ZoneColor(ZoneType zt, float& r, float& g, float& b) noexcept {
    switch (zt) {
        case ZoneType::ResidentialLow:  r = 0.28f; g = 0.76f; b = 0.32f; break;
        case ZoneType::ResidentialMed:  r = 0.18f; g = 0.65f; b = 0.24f; break;
        case ZoneType::ResidentialHigh: r = 0.10f; g = 0.52f; b = 0.16f; break;
        case ZoneType::CommercialLow:   r = 0.30f; g = 0.52f; b = 0.88f; break;
        case ZoneType::CommercialMed:   r = 0.20f; g = 0.40f; b = 0.82f; break;
        case ZoneType::CommercialHigh:  r = 0.14f; g = 0.28f; b = 0.74f; break;
        case ZoneType::IndustrialAg:    r = 0.82f; g = 0.76f; b = 0.28f; break;
        case ZoneType::IndustrialMed:   r = 0.86f; g = 0.64f; b = 0.18f; break;
        case ZoneType::IndustrialHigh:  r = 0.76f; g = 0.50f; b = 0.14f; break;
        case ZoneType::Landfill:        r = 0.52f; g = 0.38f; b = 0.24f; break;
        case ZoneType::PloppedCivic:    r = 0.62f; g = 0.66f; b = 0.72f; break;
        default:                        r = 0.45f; g = 0.62f; b = 0.36f; break;
    }
}

// Mayor rating in [-100, +100]: red through neutral yellow to emerald green.
static void MayorRatingColor(float rating, float& r, float& g, float& b) noexcept {
    const float t = std::clamp((rating + 100.0f) / 200.0f, 0.0f, 1.0f);
    const float redR = 0.86f, redG = 0.15f, redB = 0.12f;
    const float yelR = 0.93f, yelG = 0.82f, yelB = 0.16f;
    const float grnR = 0.08f, grnG = 0.72f, grnB = 0.38f;
    if (t < 0.5f) {
        const float u = t * 2.0f;
        r = redR + (yelR - redR) * u;
        g = redG + (yelG - redG) * u;
        b = redB + (yelB - redB) * u;
    } else {
        const float u = (t - 0.5f) * 2.0f;
        r = yelR + (grnR - yelR) * u;
        g = yelG + (grnG - yelG) * u;
        b = yelB + (grnB - yelB) * u;
    }
}

struct NetPoint {
    float x, y, z;
};

static NetPoint SampleNet(const SC4NetworkRecord& net, float u, float v, float lift) noexcept {
    const float iu = 1.0f - u;
    const float iv = 1.0f - v;
    const SC4NetworkVertex& a = net.verts[0];
    const SC4NetworkVertex& b = net.verts[1];
    const SC4NetworkVertex& c = net.verts[2];
    const SC4NetworkVertex& d = net.verts[3];
    NetPoint p;
    p.x = iu * iv * a.x + u * iv * b.x + u * v * c.x + iu * v * d.x;
    p.y = iu * iv * a.y + u * iv * b.y + u * v * c.y + iu * v * d.y + lift;
    p.z = iu * iv * a.z + u * iv * b.z + u * v * c.z + iu * v * d.z;
    return p;
}

static void PushNetPatch(std::vector<RenderVertex>& out, const SC4NetworkRecord& net,
                         float u0, float v0, float u1, float v1,
                         float lift, float r, float g, float b) {
    const NetPoint p00 = SampleNet(net, u0, v0, lift);
    const NetPoint p10 = SampleNet(net, u1, v0, lift);
    const NetPoint p11 = SampleNet(net, u1, v1, lift);
    const NetPoint p01 = SampleNet(net, u0, v1, lift);
    PushQuad(out,
             p00.x, p00.y, p00.z,
             p01.x, p01.y, p01.z,
             p11.x, p11.y, p11.z,
             p10.x, p10.y, p10.z,
             r, g, b, 1.0f);
}

static void PushLaneSolid(std::vector<RenderVertex>& out, const SC4NetworkRecord& net,
                          bool alongV, float across, float halfWidth, float margin, float lift,
                          float r, float g, float b) {
    if (alongV) {
        PushNetPatch(out, net, across - halfWidth, margin, across + halfWidth, 1.0f - margin, lift, r, g, b);
    } else {
        PushNetPatch(out, net, margin, across - halfWidth, 1.0f - margin, across + halfWidth, lift, r, g, b);
    }
}

static void PushLaneDashes(std::vector<RenderVertex>& out, const SC4NetworkRecord& net,
                           bool alongV, float across, float halfWidth,
                           int count, float duty, float margin, float lift,
                           float r, float g, float b) {
    if (count < 1) return;
    const float span = 1.0f - 2.0f * margin;
    const float step = span / static_cast<float>(count);
    const float dash = step * duty;
    for (int i = 0; i < count; ++i) {
        const float a0 = margin + step * static_cast<float>(i);
        const float a1 = a0 + dash;
        if (alongV) {
            PushNetPatch(out, net, across - halfWidth, a0, across + halfWidth, a1, lift, r, g, b);
        } else {
            PushNetPatch(out, net, a0, across - halfWidth, a1, across + halfWidth, lift, r, g, b);
        }
    }
}

static bool CellLinked(const cSC4City& city, int x, int z, NetworkType type) noexcept {
    NetworkType other = NetworkType::Road;
    if (!city.HasCellNetwork(x, z, &other)) return false;
    return other == type;
}

static void AxisMask(const SC4NetworkRecord& net, bool ns, bool ew, bool& drawV, bool& drawU) noexcept {
    drawV = ns;
    drawU = ew;
    if (!drawV && !drawU) {
        drawV = (net.orientation & 1u) == 0u;
        drawU = !drawV;
    }
}

static constexpr float kMarkLift = 0.62f;
static constexpr float kVehicleLift = 0.82f;

static void AppendLaneMarkings(std::vector<RenderVertex>& out, const SC4NetworkRecord& net,
                               bool ns, bool ew) {
    bool drawV = false;
    bool drawU = false;
    AxisMask(net, ns, ew, drawV, drawU);
    const float yelR = 0.93f, yelG = 0.78f, yelB = 0.16f;
    const float whtR = 0.92f, whtG = 0.92f, whtB = 0.90f;

    auto paint = [&](bool alongV) {
        if (net.networkType == NetworkType::Road) {
            PushLaneDashes(out, net, alongV, 0.50f, 0.016f, 4, 0.55f, 0.06f, kMarkLift, yelR, yelG, yelB);
            PushLaneSolid(out, net, alongV, 0.10f, 0.010f, 0.02f, kMarkLift, whtR, whtG, whtB);
            PushLaneSolid(out, net, alongV, 0.90f, 0.010f, 0.02f, kMarkLift, whtR, whtG, whtB);
        } else if (net.networkType == NetworkType::Avenue) {
            PushLaneSolid(out, net, alongV, 0.465f, 0.012f, 0.03f, kMarkLift, yelR, yelG, yelB);
            PushLaneSolid(out, net, alongV, 0.535f, 0.012f, 0.03f, kMarkLift, yelR, yelG, yelB);
            PushLaneDashes(out, net, alongV, 0.25f, 0.010f, 3, 0.46f, 0.08f, kMarkLift, whtR, whtG, whtB);
            PushLaneDashes(out, net, alongV, 0.75f, 0.010f, 3, 0.46f, 0.08f, kMarkLift, whtR, whtG, whtB);
        } else if (net.networkType == NetworkType::Highway) {
            PushLaneDashes(out, net, alongV, 0.33f, 0.012f, 4, 0.42f, 0.05f, kMarkLift, whtR, whtG, whtB);
            PushLaneDashes(out, net, alongV, 0.67f, 0.012f, 4, 0.42f, 0.05f, kMarkLift, whtR, whtG, whtB);
            PushLaneSolid(out, net, alongV, 0.06f, 0.012f, 0.02f, kMarkLift, yelR, yelG, yelB);
            PushLaneSolid(out, net, alongV, 0.94f, 0.012f, 0.02f, kMarkLift, whtR, whtG, whtB);
        }
    };
    if (drawV) paint(true);
    if (drawU) paint(false);
}

static void AppendRailTies(std::vector<RenderVertex>& out, const SC4NetworkRecord& net,
                           bool ns, bool ew) {
    bool drawV = false;
    bool drawU = false;
    AxisMask(net, ns, ew, drawV, drawU);
    const float tieR = 0.40f, tieG = 0.27f, tieB = 0.16f;
    const float railR = 0.68f, railG = 0.70f, railB = 0.72f;

    auto paint = [&](bool alongV) {
        for (int i = 0; i < 7; ++i) {
            const float a0 = 0.05f + 0.128f * static_cast<float>(i);
            const float a1 = a0 + 0.038f;
            if (alongV) {
                PushNetPatch(out, net, 0.22f, a0, 0.78f, a1, kMarkLift, tieR, tieG, tieB);
            } else {
                PushNetPatch(out, net, a0, 0.22f, a1, 0.78f, kMarkLift, tieR, tieG, tieB);
            }
        }
        PushLaneSolid(out, net, alongV, 0.36f, 0.012f, 0.02f, kMarkLift + 0.08f, railR, railG, railB);
        PushLaneSolid(out, net, alongV, 0.64f, 0.012f, 0.02f, kMarkLift + 0.08f, railR, railG, railB);
    };
    if (drawV) paint(true);
    if (drawU) paint(false);
}

static float WrapUnit(float v) noexcept {
    float w = std::fmod(v, 1.0f);
    if (w < 0.0f) w += 1.0f;
    return w;
}

static Uint32 TileHash(int x, int z) noexcept {
    Uint32 h = static_cast<Uint32>(x) * 0x9E3779B1u ^ static_cast<Uint32>(z) * 0x85EBCA77u;
    h ^= h >> 16;
    h *= 0x7FEB352Du;
    h ^= h >> 15;
    return h;
}

static void AppendAutomata(std::vector<RenderVertex>& out, const cSC4City& city,
                           const SC4NetworkRecord& net, int tx, int tz,
                           bool ns, bool ew, float animPhase) {
    bool drawV = false;
    bool drawU = false;
    AxisMask(net, ns, ew, drawV, drawU);
    const Uint32 h = TileHash(tx, tz);
    bool alongV = drawV;
    if (drawV && drawU) alongV = (h & 1u) == 0u;
    else if (!drawV && drawU) alongV = false;

    const float dayPhase = static_cast<float>(city.TotalSimDays() % 8u) / 8.0f;
    const float along = 0.20f + 0.60f * WrapUnit(
        static_cast<float>(h & 1023u) / 1023.0f + dayPhase + animPhase);

    float lateral = 0.50f;
    if (net.networkType == NetworkType::Road) {
        lateral = ((h >> 10) & 1u) != 0u ? 0.68f : 0.32f;
    } else if (net.networkType == NetworkType::Avenue) {
        lateral = ((h >> 10) & 1u) != 0u ? 0.78f : 0.22f;
    } else if (net.networkType == NetworkType::Highway) {
        const float lanes[4] = {0.20f, 0.40f, 0.60f, 0.80f};
        lateral = lanes[(h >> 12) & 3u];
    }

    const bool truck = net.networkType == NetworkType::Highway && ((h >> 16) & 3u) == 0u;
    float hu = alongV ? 0.055f : (truck ? 0.22f : 0.14f);
    float hv = alongV ? (truck ? 0.22f : 0.14f) : 0.055f;
    const float cu = std::clamp(alongV ? lateral : along, hu, 1.0f - hu);
    const float cv = std::clamp(alongV ? along : lateral, hv, 1.0f - hv);

    const NetPoint c00 = SampleNet(net, cu - hu, cv - hv, kVehicleLift);
    const NetPoint c10 = SampleNet(net, cu + hu, cv - hv, kVehicleLift);
    const NetPoint c11 = SampleNet(net, cu + hu, cv + hv, kVehicleLift);
    const NetPoint c01 = SampleNet(net, cu - hu, cv + hv, kVehicleLift);
    const float minX = std::min(std::min(c00.x, c10.x), std::min(c11.x, c01.x));
    const float maxX = std::max(std::max(c00.x, c10.x), std::max(c11.x, c01.x));
    const float minZ = std::min(std::min(c00.z, c10.z), std::min(c11.z, c01.z));
    const float maxZ = std::max(std::max(c00.z, c10.z), std::max(c11.z, c01.z));
    const float baseY = 0.25f * (c00.y + c10.y + c11.y + c01.y);
    const float bodyH = truck ? 2.15f : 1.32f;

    struct Paint { float r, g, b; };
    const Paint paints[6] = {
        {0.78f, 0.16f, 0.13f},
        {0.16f, 0.34f, 0.70f},
        {0.90f, 0.90f, 0.92f},
        {0.16f, 0.17f, 0.19f},
        {0.18f, 0.48f, 0.28f},
        {0.86f, 0.70f, 0.16f}
    };
    const Paint paint = paints[(h >> 18) % 6u];
    const float roofR = std::clamp(paint.r * 0.82f, 0.05f, 1.0f);
    const float roofG = std::clamp(paint.g * 0.82f, 0.05f, 1.0f);
    const float roofB = std::clamp(paint.b * 0.82f, 0.05f, 1.0f);
    PushBox(out, minX, baseY, minZ, maxX, baseY + bodyH, maxZ,
            paint.r, paint.g, paint.b, roofR, roofG, roofB);
}

static void AppendPowerPole(std::vector<RenderVertex>& out, const SC4NetworkRecord& net, bool crossAlongX) {
    const NetPoint c = SampleNet(net, 0.5f, 0.5f, 0.42f);
    const float height = 12.0f;
    PushBox(out,
            c.x - 0.30f, c.y, c.z - 0.30f,
            c.x + 0.30f, c.y + height, c.z + 0.30f,
            0.46f, 0.36f, 0.24f,
            0.36f, 0.28f, 0.18f);
    const float y0 = c.y + height * 0.80f;
    const float y1 = y0 + 0.42f;
    if (crossAlongX) {
        PushBox(out,
                c.x - 3.5f, y0, c.z - 0.20f,
                c.x + 3.5f, y1, c.z + 0.20f,
                0.32f, 0.26f, 0.18f,
                0.32f, 0.26f, 0.18f);
    } else {
        PushBox(out,
                c.x - 0.20f, y0, c.z - 3.5f,
                c.x + 0.20f, y1, c.z + 3.5f,
                0.32f, 0.26f, 0.18f,
                0.32f, 0.26f, 0.18f);
    }
}

static void AppendTransmissionPylon(std::vector<RenderVertex>& out, float x, float y, float z) {
    const float r = 0.62f, g = 0.66f, b = 0.70f;
    const float h = 24.0f;
    const float s = 1.55f;
    const float t = 0.28f;
    PushBox(out, x - s, y, z - s, x - s + t, y + h, z - s + t, r, g, b, r, g, b);
    PushBox(out, x + s - t, y, z - s, x + s, y + h, z - s + t, r, g, b, r, g, b);
    PushBox(out, x - s, y, z + s - t, x - s + t, y + h, z + s, r, g, b, r, g, b);
    PushBox(out, x + s - t, y, z + s - t, x + s, y + h, z + s, r, g, b, r, g, b);
    const float brace = 0.22f;
    const float yA0 = y + h * 0.42f;
    const float yA1 = yA0 + 0.30f;
    const float yB0 = y + h * 0.74f;
    const float yB1 = yB0 + 0.30f;
    PushBox(out, x - s, yA0, z - brace, x + s, yA1, z + brace, r * 0.9f, g * 0.9f, b * 0.9f, r, g, b);
    PushBox(out, x - brace, yA0, z - s, x + brace, yA1, z + s, r * 0.9f, g * 0.9f, b * 0.9f, r, g, b);
    PushBox(out, x - s * 0.72f, yB0, z - brace, x + s * 0.72f, yB1, z + brace, r * 0.9f, g * 0.9f, b * 0.9f, r, g, b);
    PushBox(out, x - brace, yB0, z - s * 0.72f, x + brace, yB1, z + s * 0.72f, r * 0.9f, g * 0.9f, b * 0.9f, r, g, b);
    PushBox(out, x - 0.34f, y + h, z - 0.34f, x + 0.34f, y + h + 1.5f, z + 0.34f,
            0.48f, 0.50f, 0.54f, 0.40f, 0.42f, 0.46f);
}

static bool ContainsIgnoreCase(const std::string& text, const char* needle) noexcept {
    if (needle == nullptr || needle[0] == '\0') return false;
    const std::size_t n = std::strlen(needle);
    if (text.size() < n) return false;
    for (std::size_t i = 0; i + n <= text.size(); ++i) {
        bool match = true;
        for (std::size_t j = 0; j < n; ++j) {
            unsigned char a = static_cast<unsigned char>(text[i + j]);
            unsigned char b = static_cast<unsigned char>(needle[j]);
            if (a >= 'A' && a <= 'Z') a = static_cast<unsigned char>(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = static_cast<unsigned char>(b - 'A' + 'a');
            if (a != b) {
                match = false;
                break;
            }
        }
        if (match) return true;
    }
    return false;
}

static bool IsCivicPowerBuilding(const SC4BuildingRecord& bld) noexcept {
    if (ContainsIgnoreCase(bld.name, "power")) return true;
    if (ContainsIgnoreCase(bld.name, "nuclear")) return true;
    if (ContainsIgnoreCase(bld.name, "solar")) return true;
    return bld.zoneHint == ZoneType::PloppedCivic && ContainsIgnoreCase(bld.name, "plant");
}

static bool IsPowerLineTile(const SC4NetworkRecord& net) noexcept {
    const unsigned bit = 1u << static_cast<unsigned>(NetworkType::PowerLine);
    return net.networkType == NetworkType::PowerLine || (net.networkTypeMask & bit) != 0u;
}

static void AppendNetworkDetail(std::vector<RenderVertex>& out, const cSC4City& city,
                                const SC4NetworkRecord& net, float animPhase) {
    if (!std::isfinite(net.centerX) || !std::isfinite(net.centerZ)) return;
    const int tx = static_cast<int>(std::floor(net.centerX / 16.0f));
    const int tz = static_cast<int>(std::floor(net.centerZ / 16.0f));
    const bool ns = CellLinked(city, tx, tz - 1, net.networkType) ||
                    CellLinked(city, tx, tz + 1, net.networkType);
    const bool ew = CellLinked(city, tx + 1, tz, net.networkType) ||
                    CellLinked(city, tx - 1, tz, net.networkType);

    if (IsPowerLineTile(net)) {
        const bool alongV = ns || !ew;
        AppendPowerPole(out, net, alongV);
    }
    if (net.networkType == NetworkType::Rail) {
        AppendRailTies(out, net, ns, ew);
        return;
    }
    if (net.networkType == NetworkType::Road ||
        net.networkType == NetworkType::Avenue ||
        net.networkType == NetworkType::Highway) {
        AppendLaneMarkings(out, net, ns, ew);
        // Prefer saved cSC4Vehicle S3D over procedural colored boxes.
        if (city.Automata().empty()) {
            AppendAutomata(out, city, net, tx, tz, ns, ew, animPhase);
        }
    }
}

static void AppendHoverCursor(std::vector<RenderVertex>& out, const cSC4City& city, int hx, int hz) {
    if (hx < 0 || hz < 0 || hx >= city.CellCountX() || hz >= city.CellCountZ()) return;
    const float inset = 0.55f;
    const float x0 = static_cast<float>(hx) * 16.0f + inset;
    const float z0 = static_cast<float>(hz) * 16.0f + inset;
    const float x1 = static_cast<float>(hx + 1) * 16.0f - inset;
    const float z1 = static_cast<float>(hz + 1) * 16.0f - inset;
    const float lift = 1.15f;
    const float y00 = city.GetVertexHeight(hx, hz) + lift;
    const float y10 = city.GetVertexHeight(hx + 1, hz) + lift;
    const float y11 = city.GetVertexHeight(hx + 1, hz + 1) + lift;
    const float y01 = city.GetVertexHeight(hx, hz + 1) + lift;
    PushQuad(out,
             x0, y00, z0,
             x0, y01, z1,
             x1, y11, z1,
             x1, y10, z0,
             1.00f, 0.86f, 0.20f, 0.62f);

    const float edge = 0.42f;
    const float rim = lift + 0.16f;
    const float ry00 = city.GetVertexHeight(hx, hz) + rim;
    const float ry10 = city.GetVertexHeight(hx + 1, hz) + rim;
    const float ry11 = city.GetVertexHeight(hx + 1, hz + 1) + rim;
    const float ry01 = city.GetVertexHeight(hx, hz + 1) + rim;
    PushQuad(out,
             x0, ry00, z0,
             x0, ry00, z0 + edge,
             x1, ry10, z0 + edge,
             x1, ry10, z0,
             1.0f, 0.96f, 0.55f, 0.95f);
    PushQuad(out,
             x0, ry01, z1 - edge,
             x0, ry01, z1,
             x1, ry11, z1,
             x1, ry11, z1 - edge,
             1.0f, 0.96f, 0.55f, 0.95f);
    PushQuad(out,
             x0, ry00, z0,
             x0, ry01, z1,
             x0 + edge, ry01, z1,
             x0 + edge, ry00, z0,
             1.0f, 0.96f, 0.55f, 0.95f);
    PushQuad(out,
             x1 - edge, ry10, z0,
             x1 - edge, ry11, z1,
             x1, ry11, z1,
             x1, ry10, z0,
             1.0f, 0.96f, 0.55f, 0.95f);
}

// Matches cSTETerrainView3D::SetUpTexturesForCurrentTerrainTypeAndZoom @ 0x10074688c
// and AssignTerrainAndWaterTextureCoordinates @ 0x1007464b8. Terrain.ini ships as
// TGI (0, 0x8a5971c5, 0xaa597172); FSH tiles live in group 0x891b0e1a.
constexpr Uint32 kTerrainIniGroup    = 0x8a5971c5u;
constexpr Uint32 kTerrainIniInstance = 0xaa597172u;
constexpr Uint32 kTerrainFshGroup    = 0x891b0e1au;
constexpr float kTerrainTextureWorldSize = 32.0f;
constexpr float kTerrainTint[3] = {0.85f, 0.88f, 1.12f};

static void ParseTerrainIni(const std::vector<Uint8>& bytes, std::vector<Uint8>& table,
                            Uint8& highCliff, Uint8& lowCliff, Uint8& beach) {
    table.clear();
    const std::string text(bytes.begin(), bytes.end());
    std::string section;
    std::string climate;
    std::vector<Uint8> rows(32 * 16, 0);
    std::vector<bool> rowSeen(32, false);
    std::unordered_map<std::string, std::unordered_map<std::string, Uint8>> misc;
    auto finishTable = [&]() {
        if (!table.empty() || climate.empty()) return;
        if (std::all_of(rowSeen.begin(), rowSeen.end(), [](bool b) { return b; })) table = rows;
        else climate.clear();
    };
    std::size_t start = 0;
    while (start < text.size()) {
        std::size_t end = text.find('\n', start);
        if (end == std::string::npos) end = text.size();
        std::string line = text.substr(start, end - start);
        start = end + 1;
        if (const std::size_t semi = line.find(';'); semi != std::string::npos) line.resize(semi);
        while (!line.empty() && std::isspace(static_cast<unsigned char>(line.back()))) line.pop_back();
        if (line.empty()) continue;
        if (line.front() == '[') {
            finishTable();
            section = line.substr(1, line.find(']') - 1);
            const std::string suffix = "TextureMapTable";
            if (table.empty() && section.size() > suffix.size() &&
                section.compare(section.size() - suffix.size(), suffix.size(), suffix) == 0) {
                climate = section.substr(0, section.size() - suffix.size());
                std::fill(rows.begin(), rows.end(), 0);
                std::fill(rowSeen.begin(), rowSeen.end(), false);
            }
            continue;
        }
        const std::size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        while (!key.empty() && std::isspace(static_cast<unsigned char>(key.back()))) key.pop_back();
        const std::string value = line.substr(eq + 1);
        if (!climate.empty() && table.empty() && section == climate + "TextureMapTable") {
            const int row = std::atoi(key.c_str());
            if (row < 0 || row >= 32) continue;
            std::size_t pos = 0;
            int col = 0;
            while (col < 16 && pos < value.size()) {
                const std::size_t comma = value.find(',', pos);
                const std::string item = value.substr(pos, comma == std::string::npos ? std::string::npos : comma - pos);
                if (item.find_first_not_of(" \t") != std::string::npos) {
                    rows[static_cast<std::size_t>(row) * 16 + col++] =
                        static_cast<Uint8>(std::strtoul(item.c_str(), nullptr, 16));
                }
                if (comma == std::string::npos) break;
                pos = comma + 1;
            }
            if (col == 16) rowSeen[row] = true;
        } else {
            misc[section][key] = static_cast<Uint8>(std::strtoul(value.c_str(), nullptr, 16));
        }
    }
    finishTable();
    if (table.empty()) return;
    const auto found = misc.find(climate + "MiscTextures");
    if (found == misc.end()) return;
    auto get = [&](const char* key) -> Uint8 {
        const auto it = found->second.find(key);
        return it == found->second.end() ? 0u : it->second;
    };
    highCliff = get("HighCliff");
    lowCliff = get("LowCliff");
    beach = get("Beach");
}

std::size_t cSC4GLCityRenderer::BuildTexturedBatchesFromDBPF(const cSC4City& city,
                                                                  const cDBPFSystem& dbpfSys) {
    RetireTexturedBatches();
    mTexturedBuildingMask.assign(city.Buildings().size(), 0);
    mTexturedFloraMask.assign(city.Flora().size(), 0);
    mTexturedPedestrianMask.assign(city.Automata().size(), 0);

    std::unordered_map<Uint32, TexturedMeshBatch> byTexture;
    auto getOrLoadBatch = [&](Uint32 fshId) -> TexturedMeshBatch* {
        auto found = byTexture.find(fshId);
        if (found != byTexture.end()) {
            if (found->second.texWidth == 0 || found->second.rgbaPixels.empty()) return nullptr;
            return &found->second;
        }

        TexturedMeshBatch created;
        created.fshInstanceId = fshId;
        std::vector<Uint8> bytes;
        cFSHImage image;
        bool loaded = false;
        // Prefer exact instance; then Zoom-5 (+4) / Zoom-1 (-4) siblings used by lot/net FSH.
        for (Uint32 tryId : {fshId, fshId + 4u, (fshId >= 4u ? fshId - 4u : fshId)}) {
            bytes.clear();
            if (dbpfSys.LoadAssetByTypeAndInstance(kTypeFSH, tryId, bytes) &&
                image.Parse(bytes.data(), bytes.size()) && image.width > 0 && image.height > 0) {
                const std::size_t need = static_cast<std::size_t>(image.width) *
                                         static_cast<std::size_t>(image.height) * 4u;
                if (image.rgbaPixels.size() >= need) {
                    created.texWidth = image.width;
                    created.texHeight = image.height;
                    created.rgbaPixels = std::move(image.rgbaPixels);
                    loaded = true;
                    break;
                }
            }
        }
        (void)loaded;
        auto inserted = byTexture.emplace(fshId, std::move(created));
        TexturedMeshBatch& stored = inserted.first->second;
        if (stored.texWidth == 0 || stored.rgbaPixels.empty()) return nullptr;
        return &stored;
    };

    for (const auto& net : city.Networks()) {
        if (net.textureId == 0) continue;
        // Network FSH instances are piece + zoom (0..4); +4 is the 128x128 Zoom 5 image.
        TexturedMeshBatch* batch = getOrLoadBatch(net.textureId + 4u);
        if (batch == nullptr) batch = getOrLoadBatch(net.textureId);
        if (batch == nullptr) continue;
        const float lift = 0.36f;
        auto& verts = batch->vertices;
        // Saved corners run (x0,z0),(x0,z1),(x1,z1),(x1,z0) like the terrain
        // cells, so 0,1,2 / 0,2,3 shares the terrain's split diagonal.
        auto push = [&](const SC4NetworkVertex& v) {
            const float r = static_cast<float>(v.rgba & 0xFFu) / 255.0f;
            const float g = static_cast<float>((v.rgba >> 8) & 0xFFu) / 255.0f;
            const float b = static_cast<float>((v.rgba >> 16) & 0xFFu) / 255.0f;
            AppendTexturedVertex(verts, v.x, v.y + lift, v.z, v.u, v.v, r, g, b, 1.0f);
        };
        for (int idx : {0, 1, 2, 0, 2, 3}) push(net.verts[idx]);
    }

    // Models only contain faces visible from one fixed camera. Like SC4, pick the
    // model for the camera quarter (instance rotation nibble 0x30) and place it
    // with camQuarter turns so it faces the view without double-rotating.
    constexpr std::size_t kMaxCachedModels = 8192;
    // Key = (group << 32) | instance so park models under 0xbadb57f1 don't collide.
    std::unordered_map<Uint64, cS3DModel> modelCache;
    modelCache.reserve(kMaxCachedModels);
    const unsigned camQuarter = static_cast<unsigned>(mRotationIndex) & 3u;

    // SC4 S3D instance: bits 8..11 = zoom (0x0000..0x0400), bits 4..5 = camera quarter.
    // Bias one LOD coarser so mid-zoom forests read as canopy rather than pinpricks.
    const Uint32 floraZoomNibble = [&]() -> Uint32 {
        if (mZoom < 0.70f) return 0x0000u;
        if (mZoom < 1.20f) return 0x0100u;
        if (mZoom < 2.00f) return 0x0200u;
        if (mZoom < 3.20f) return 0x0300u;
        return 0x0400u;
    }();

    auto loadCachedModel = [&](Uint32 instanceId, Uint32 groupId = 0) -> const cS3DModel* {
        const Uint64 cacheKey = (static_cast<Uint64>(groupId) << 32) | instanceId;
        auto found = modelCache.find(cacheKey);
        if (found != modelCache.end()) {
            return (found->second.vertices.empty() || found->second.indices.size() < 3)
                ? nullptr : &found->second;
        }
        if (modelCache.size() >= kMaxCachedModels) return nullptr;
        cS3DModel loaded;
        std::vector<Uint8> s3dBytes;
        const bool loadedOk = (groupId != 0 &&
            dbpfSys.LoadAssetByTGI(cSC4TGI{kTypeS3D, groupId, instanceId}, s3dBytes)) ||
            dbpfSys.LoadAssetByTypeAndInstance(kTypeS3D, instanceId, s3dBytes);
        if (loadedOk) loaded.Parse(s3dBytes.data(), s3dBytes.size());
        auto inserted = modelCache.emplace(cacheKey, std::move(loaded));
        return (inserted.first->second.vertices.empty() || inserted.first->second.indices.size() < 3)
            ? nullptr : &inserted.first->second;
    };

    auto appendOccupantModel = [&](Uint32 s3dInstanceId, unsigned objectOrientation,
                                   float anchorX, float anchorY, float anchorZ,
                                   Uint32 fallbackFshId = 0u,
                                   bool varyZoom = true,
                                   float modelScale = 1.0f,
                                   Uint32 s3dGroupId = 0u) -> bool {
        if (s3dInstanceId == 0) return false;
        if (!std::isfinite(anchorX) || !std::isfinite(anchorY) || !std::isfinite(anchorZ)) return false;
        const unsigned viewRotation = (objectOrientation - camQuarter) & 3u;
        // Prefer the camera-quarter model; fall back through other quarters when a
        // flora/prop/building exemplar only ships one rotation. Flora also tries
        // higher zooms if the overview LOD is missing.
        const cS3DModel* modelPtr = nullptr;
        unsigned usedCamQuarter = camQuarter;
        const unsigned startZoom = varyZoom ? ((floraZoomNibble >> 8) & 0xFu)
                                            : ((s3dInstanceId >> 8) & 0xFu);
        const unsigned endZoom = varyZoom ? 4u : startZoom;
        for (unsigned zLevel = startZoom; zLevel <= endZoom && modelPtr == nullptr; ++zLevel) {
            const Uint32 base = (s3dInstanceId & ~0x0F00u) | (zLevel << 8);
            for (unsigned tryRot = 0; tryRot < 4u && modelPtr == nullptr; ++tryRot) {
                const unsigned rot = (viewRotation + tryRot) & 3u;
                const Uint32 instanceId = (base & ~0x30u) | (rot << 4);
                modelPtr = loadCachedModel(instanceId, s3dGroupId);
                if (modelPtr != nullptr) {
                    usedCamQuarter = (camQuarter - tryRot) & 3u;
                }
            }
        }
        // Park/plop stubs (e.g. 0x0e110000) ship as a single un-nibbled instance —
        // no zoom/rotation variants exist under group 0xbadb57f1.
        if (modelPtr == nullptr) {
            const Uint32 exactBase = s3dInstanceId & 0xFFFFF000u;
            modelPtr = loadCachedModel(exactBase, s3dGroupId);
            if (modelPtr == nullptr && s3dGroupId != 0) {
                modelPtr = loadCachedModel(exactBase, 0);
            }
        }
        if (modelPtr == nullptr) return false;
        const cS3DModel& model = *modelPtr;
        Uint32 fshId = 0;
        for (const auto& mat : model.materials) {
            if (mat.textureId != 0) { fshId = mat.textureId; break; }
        }
        if (fshId == 0) fshId = fallbackFshId;

        float minX = model.vertices.front().x;
        float minY = model.vertices.front().y;
        float minZ = model.vertices.front().z;
        float maxX = minX;
        float maxZ = minZ;
        for (const S3DVertex& vert : model.vertices) {
            minX = std::min(minX, vert.x);
            minY = std::min(minY, vert.y);
            minZ = std::min(minZ, vert.z);
            maxX = std::max(maxX, vert.x);
            maxZ = std::max(maxZ, vert.z);
        }
        if (!std::isfinite(minX) || !std::isfinite(minY) || !std::isfinite(minZ) ||
            !std::isfinite(maxX) || !std::isfinite(maxZ)) {
            return false;
        }

        TexturedMeshBatch* batch = (fshId != 0) ? getOrLoadBatch(fshId) : nullptr;
        if (batch == nullptr) {
            // Vertex-colored S3D with no MATS FSH: draw through a 1x1 white atlas.
            constexpr Uint32 kWhiteFshKey = 0xFFFFFFFFu;
            auto found = byTexture.find(kWhiteFshKey);
            if (found == byTexture.end()) {
                TexturedMeshBatch created;
                created.fshInstanceId = kWhiteFshKey;
                created.texWidth = 1;
                created.texHeight = 1;
                created.rgbaPixels = {255, 255, 255, 255};
                found = byTexture.emplace(kWhiteFshKey, std::move(created)).first;
            }
            batch = &found->second;
        }

        const float modelCenterX = 0.5f * (minX + maxX);
        const float modelCenterZ = 0.5f * (minZ + maxZ);
        const unsigned orientation = usedCamQuarter;
        const std::size_t before = batch->vertices.size();
        for (std::size_t i = 0; i + 2 < model.indices.size(); i += 3) {
            const Uint16 i0 = model.indices[i];
            const Uint16 i1 = model.indices[i + 1];
            const Uint16 i2 = model.indices[i + 2];
            const std::size_t vertCount = model.vertices.size();
            if (i0 >= vertCount || i1 >= vertCount || i2 >= vertCount) continue;
            const S3DVertex& a = model.vertices[i0];
            const S3DVertex& b = model.vertices[i1];
            const S3DVertex& c = model.vertices[i2];
            float ax = 0.0f, ay = 0.0f, az = 0.0f;
            float bx = 0.0f, by = 0.0f, bz = 0.0f;
            float cx = 0.0f, cy = 0.0f, cz = 0.0f;
            PlaceModelVertex(a, modelCenterX, minY, modelCenterZ, anchorX, anchorY, anchorZ, orientation, ax, ay, az);
            PlaceModelVertex(b, modelCenterX, minY, modelCenterZ, anchorX, anchorY, anchorZ, orientation, bx, by, bz);
            PlaceModelVertex(c, modelCenterX, minY, modelCenterZ, anchorX, anchorY, anchorZ, orientation, cx, cy, cz);
            if (modelScale != 1.0f) {
                ax = anchorX + (ax - anchorX) * modelScale;
                ay = anchorY + (ay - anchorY) * modelScale;
                az = anchorZ + (az - anchorZ) * modelScale;
                bx = anchorX + (bx - anchorX) * modelScale;
                by = anchorY + (by - anchorY) * modelScale;
                bz = anchorZ + (bz - anchorZ) * modelScale;
                cx = anchorX + (cx - anchorX) * modelScale;
                cy = anchorY + (cy - anchorY) * modelScale;
                cz = anchorZ + (cz - anchorZ) * modelScale;
            }
            float ar = 1.0f, ag = 1.0f, ab = 1.0f, aa = 1.0f;
            float br = 1.0f, bg = 1.0f, bb = 1.0f, ba = 1.0f;
            float cr = 1.0f, cg = 1.0f, cb = 1.0f, ca = 1.0f;
            UnpackS3DColor(a.color, ar, ag, ab, aa);
            UnpackS3DColor(b.color, br, bg, bb, ba);
            UnpackS3DColor(c.color, cr, cg, cb, ca);
            AppendTexturedVertex(batch->vertices, ax, ay, az, a.u, a.v, ar, ag, ab, aa);
            AppendTexturedVertex(batch->vertices, bx, by, bz, b.u, b.v, br, bg, bb, ba);
            AppendTexturedVertex(batch->vertices, cx, cy, cz, c.u, c.v, cr, cg, cb, ca);
        }
        return batch->vertices.size() > before;
    };

    for (std::size_t bi = 0; bi < city.Buildings().size(); ++bi) {
        const SC4BuildingRecord& bld = city.Buildings()[bi];
        const float ax = 0.5f * (bld.minX + bld.maxX);
        const float az = 0.5f * (bld.minZ + bld.maxZ);
        if (appendOccupantModel(bld.s3dInstanceId, bld.orientation, ax, bld.minY, az,
                                bld.fshTextureId, /*varyZoom=*/false, 1.0f, bld.s3dGroupId) ||
            appendOccupantModel(bld.s3dInstanceId, bld.orientation, ax, bld.minY, az,
                                bld.fshTextureId, /*varyZoom=*/true, 1.0f, bld.s3dGroupId)) {
            mTexturedBuildingMask[bi] = 1;
        }
    }
    for (const SC4PropRecord& prop : city.Props()) {
        appendOccupantModel(prop.s3dInstanceId, prop.orientation,
                            0.5f * (prop.minX + prop.maxX), prop.minY, 0.5f * (prop.minZ + prop.maxZ));
    }
    // Mid-zoom canopy: SC4's far LODs read larger on screen than Zoom-5 metres.
    const float floraScale = (mZoom < 1.8f) ? 1.55f : ((mZoom < 2.8f) ? 1.25f : 1.0f);
    for (std::size_t fi = 0; fi < city.Flora().size(); ++fi) {
        const SC4FloraRecord& fl = city.Flora()[fi];
        if (appendOccupantModel(fl.s3dInstanceId, fl.orientation, fl.x, fl.y, fl.z,
                                0u, /*varyZoom=*/true, floraScale)) {
            mTexturedFloraMask[fi] = 1;
        }
    }

    // Saved cSC4Vehicle / S3D pedestrians (dogs etc.): models face +Z; rotate by heading.
    auto appendHeadingS3D = [&](std::size_t autoIndex, const SC4AutomatonRecord& autoRec) -> bool {
        if (autoRec.model.type != kTypeS3D || autoRec.model.instance == 0) return false;
        if (!std::isfinite(autoRec.x) || !std::isfinite(autoRec.y) || !std::isfinite(autoRec.z)) return false;

        const cS3DModel* modelPtr = nullptr;
        const Uint64 autoKey = (static_cast<Uint64>(autoRec.model.group) << 32) | autoRec.model.instance;
        auto cached = modelCache.find(autoKey);
        if (cached != modelCache.end()) {
            modelPtr = (cached->second.vertices.empty() || cached->second.indices.size() < 3)
                ? nullptr : &cached->second;
        } else if (modelCache.size() < kMaxCachedModels) {
            cS3DModel loaded;
            std::vector<Uint8> s3dBytes;
            if (dbpfSys.LoadAssetByTGI(cSC4TGI{kTypeS3D, autoRec.model.group, autoRec.model.instance}, s3dBytes) ||
                dbpfSys.LoadAssetByTypeAndInstance(kTypeS3D, autoRec.model.instance, s3dBytes)) {
                loaded.Parse(s3dBytes.data(), s3dBytes.size());
            }
            auto inserted = modelCache.emplace(autoKey, std::move(loaded));
            modelPtr = (inserted.first->second.vertices.empty() || inserted.first->second.indices.size() < 3)
                ? nullptr : &inserted.first->second;
        }
        if (modelPtr == nullptr) return false;
        const cS3DModel& model = *modelPtr;
        const Uint32 fshId = !model.materials.empty() ? model.materials.front().textureId : 0u;
        if (fshId == 0) return false;
        TexturedMeshBatch* batch = getOrLoadBatch(fshId);
        if (batch == nullptr) return false;

        float minX = model.vertices.front().x;
        float minY = model.vertices.front().y;
        float minZ = model.vertices.front().z;
        float maxX = minX;
        float maxZ = minZ;
        for (const S3DVertex& vert : model.vertices) {
            minX = std::min(minX, vert.x);
            minY = std::min(minY, vert.y);
            minZ = std::min(minZ, vert.z);
            maxX = std::max(maxX, vert.x);
            maxZ = std::max(maxZ, vert.z);
        }
        const float midX = 0.5f * (minX + maxX);
        const float midZ = 0.5f * (minZ + maxZ);
        const float hx = autoRec.headingX;
        const float hz = autoRec.headingZ;
        const std::size_t before = batch->vertices.size();
        for (std::size_t i = 0; i + 2 < model.indices.size(); i += 3) {
            const Uint16 i0 = model.indices[i];
            const Uint16 i1 = model.indices[i + 1];
            const Uint16 i2 = model.indices[i + 2];
            const std::size_t vertCount = model.vertices.size();
            if (i0 >= vertCount || i1 >= vertCount || i2 >= vertCount) continue;
            auto place = [&](const S3DVertex& v, float& ox, float& oy, float& oz) {
                const float lx = v.x - midX;
                const float lz = v.z - midZ;
                ox = autoRec.x + lx * hz + lz * hx;
                oy = autoRec.y + (v.y - minY) + 0.22f;
                oz = autoRec.z - lx * hx + lz * hz;
            };
            float ax = 0, ay = 0, az = 0, bx = 0, by = 0, bz = 0, cx = 0, cy = 0, cz = 0;
            place(model.vertices[i0], ax, ay, az);
            place(model.vertices[i1], bx, by, bz);
            place(model.vertices[i2], cx, cy, cz);
            float ar = 1, ag = 1, ab = 1, aa = 1, br = 1, bg = 1, bb = 1, ba = 1, cr = 1, cg = 1, cb = 1, ca = 1;
            UnpackS3DColor(model.vertices[i0].color, ar, ag, ab, aa);
            UnpackS3DColor(model.vertices[i1].color, br, bg, bb, ba);
            UnpackS3DColor(model.vertices[i2].color, cr, cg, cb, ca);
            AppendTexturedVertex(batch->vertices, ax, ay, az, model.vertices[i0].u, model.vertices[i0].v, ar, ag, ab, aa);
            AppendTexturedVertex(batch->vertices, bx, by, bz, model.vertices[i1].u, model.vertices[i1].v, br, bg, bb, ba);
            AppendTexturedVertex(batch->vertices, cx, cy, cz, model.vertices[i2].u, model.vertices[i2].v, cr, cg, cb, ca);
        }
        if (batch->vertices.size() > before && autoIndex < mTexturedPedestrianMask.size() &&
            autoRec.kind == SC4AutomatonRecord::Kind::Pedestrian) {
            mTexturedPedestrianMask[autoIndex] = 1;
        }
        return batch->vertices.size() > before;
    };
    for (std::size_t ai = 0; ai < city.Automata().size(); ++ai) {
        const SC4AutomatonRecord& autoRec = city.Automata()[ai];
        if (autoRec.kind == SC4AutomatonRecord::Kind::Vehicle ||
            (autoRec.kind == SC4AutomatonRecord::Kind::Pedestrian && autoRec.model.type == kTypeS3D)) {
            appendHeadingS3D(ai, autoRec);
        }
    }

    // Saved cSC4Pedestrian automata: sprite animation type 0x29a5d1ec names an FSH
    // atlas plus per-zoom frame tables (0x09adcd75). Billboards face the camera.
    constexpr Uint32 kTypePedestrianSprite = 0x29a5d1ecu;
    constexpr float kZoom5PixelsPerMetre = 7.0f;
    auto readLE32 = [](const std::vector<Uint8>& bytes, std::size_t offset) -> Uint32 {
        if (offset + 4 > bytes.size()) return 0;
        return static_cast<Uint32>(bytes[offset]) |
               (static_cast<Uint32>(bytes[offset + 1]) << 8) |
               (static_cast<Uint32>(bytes[offset + 2]) << 16) |
               (static_cast<Uint32>(bytes[offset + 3]) << 24);
    };
    struct SpriteFrame { Uint8 x, y, w, h, anchorX, anchorY; };
    struct PedestrianSprite {
        std::size_t batch = 0;
        int framesPerDirection = 1;
        float metresPerPixel = 1.0f / kZoom5PixelsPerMetre;
        std::vector<SpriteFrame> frames;
    };
    std::unordered_map<Uint32, PedestrianSprite> pedSprites;
    std::unordered_set<Uint32> missingPedSprites;
    auto pedestrianSpriteFor = [&](const cSC4TGI& key) -> const PedestrianSprite* {
        if (auto found = pedSprites.find(key.instance); found != pedSprites.end()) return &found->second;
        if (missingPedSprites.count(key.instance) != 0) return nullptr;
        auto fail = [&]() -> const PedestrianSprite* {
            missingPedSprites.insert(key.instance);
            return nullptr;
        };
        std::vector<Uint8> desc;
        if (key.type != kTypePedestrianSprite || !dbpfSys.LoadAssetByTGI(key, desc) || desc.size() < 48 ||
            readLE32(desc, 4) != kTypeFSH) {
            return fail();
        }
        const cSC4TGI atlas{kTypeFSH, readLE32(desc, 8), readLE32(desc, 12)};
        const Uint32 tableType = readLE32(desc, 16);
        const Uint32 tableGroup = readLE32(desc, 20);
        const int framesPerDirection = static_cast<int>(readLE32(desc, 44));
        if (framesPerDirection <= 0 || framesPerDirection > 64) return fail();

        PedestrianSprite sprite;
        sprite.framesPerDirection = framesPerDirection;
        std::vector<Uint8> table;
        const Uint32 zoom5 = readLE32(desc, 40);
        const Uint32 zoom4 = readLE32(desc, 36);
        if (zoom5 != 0 && dbpfSys.LoadAssetByTGI(cSC4TGI{tableType, tableGroup, zoom5}, table)) {
            sprite.metresPerPixel = 1.0f / kZoom5PixelsPerMetre;
        } else if (zoom4 != 0 && dbpfSys.LoadAssetByTGI(cSC4TGI{tableType, tableGroup, zoom4}, table)) {
            sprite.metresPerPixel = 2.0f / kZoom5PixelsPerMetre;
        } else {
            return fail();
        }
        const Uint32 directions = readLE32(table, 32);
        if (directions == 0 || directions > 16 || table.size() < 36 + 8u * directions) return fail();

        // Atlas may not be in the instance-only FSH index used by getOrLoadBatch.
        TexturedMeshBatch* batch = nullptr;
        {
            auto found = byTexture.find(atlas.instance);
            if (found != byTexture.end()) {
                if (found->second.texWidth != 0 && !found->second.rgbaPixels.empty()) batch = &found->second;
            } else {
                std::vector<Uint8> bytes;
                cFSHImage image;
                const bool loaded =
                    (dbpfSys.LoadAssetByTGI(atlas, bytes) ||
                     dbpfSys.LoadAssetByTypeAndInstance(kTypeFSH, atlas.instance, bytes)) &&
                    image.Parse(bytes.data(), bytes.size());
                const std::size_t need = static_cast<std::size_t>(image.width) * image.height * 4u;
                if (loaded && image.width != 0 && image.height != 0 && image.rgbaPixels.size() >= need) {
                    TexturedMeshBatch created;
                    created.fshInstanceId = atlas.instance;
                    created.texWidth = image.width;
                    created.texHeight = image.height;
                    created.rgbaPixels = std::move(image.rgbaPixels);
                    batch = &byTexture.emplace(atlas.instance, std::move(created)).first->second;
                }
            }
        }
        if (batch == nullptr) return fail();
        sprite.batch = atlas.instance; // resolved later via byTexture key
        const int atlasW = batch->texWidth;
        const int atlasH = batch->texHeight;
        for (Uint32 d = 0; d < directions; ++d) {
            const Uint8* e = &table[36 + 8u * d];
            int x = e[2];
            int y = e[3];
            const int w = e[4];
            const int h = e[5];
            if (w == 0 || h == 0) return fail();
            for (int f = 0; f < framesPerDirection; ++f) {
                if (x + w > atlasW) {
                    x = 0;
                    y += h;
                }
                if (y + h > atlasH) return fail();
                sprite.frames.push_back(SpriteFrame{static_cast<Uint8>(x), static_cast<Uint8>(y),
                                                    static_cast<Uint8>(w), static_cast<Uint8>(h), e[6], e[7]});
                x += w;
            }
        }
        return &pedSprites.emplace(key.instance, std::move(sprite)).first->second;
    };

    float yaw = (mRotationIndex * 0.5f + 0.25f) * 3.14159265f;
    float pitch = 0.6154797f;
    float dirX = std::cos(pitch) * std::cos(yaw);
    float dirY = std::sin(pitch);
    float dirZ = std::cos(pitch) * std::sin(yaw);
    float horiz = std::max(1e-5f, std::sqrt(dirX * dirX + dirZ * dirZ));
    float sx = dirZ / horiz, sz = -dirX / horiz;
    float ux = sz * dirY, uy = sx * dirZ - sz * dirX, uz = -sx * dirY;
    float awayX = -dirX / horiz, awayZ = -dirZ / horiz;
    struct SpriteDir { Uint8 dir; bool mirror; };
    static constexpr SpriteDir kSectors[8] = {
        {0, true}, {1, true}, {5, false}, {1, false}, {0, false}, {4, false}, {2, false}, {4, true}};

    for (std::size_t ai = 0; ai < city.Automata().size(); ++ai) {
        const SC4AutomatonRecord& a = city.Automata()[ai];
        if (a.kind != SC4AutomatonRecord::Kind::Pedestrian) continue;
        if (!std::isfinite(a.x) || !std::isfinite(a.y) || !std::isfinite(a.z)) continue;
        const PedestrianSprite* sprite = pedestrianSpriteFor(a.model);
        if (sprite == nullptr) continue;
        auto batchIt = byTexture.find(static_cast<Uint32>(sprite->batch));
        if (batchIt == byTexture.end()) continue;
        TexturedMeshBatch& batch = batchIt->second;
        const int fpd = sprite->framesPerDirection;
        const int directions = static_cast<int>(sprite->frames.size()) / fpd;
        if (directions <= 0) continue;
        const float angle = std::atan2(a.headingX * awayX + a.headingZ * awayZ,
                                       a.headingX * sx + a.headingZ * sz);
        const int sector = static_cast<int>(std::lround(angle / 0.78539816f)) & 7;
        const SpriteDir pick = kSectors[sector];
        if (pick.dir >= directions) continue;
        const SpriteFrame& f = sprite->frames[static_cast<std::size_t>(pick.dir) * fpd];
        const float mpp = sprite->metresPerPixel;
        const float side = pick.mirror ? -1.0f : 1.0f;
        const float x0 = -side * static_cast<float>(f.anchorX) * mpp;
        const float x1 = side * static_cast<float>(f.w - f.anchorX) * mpp;
        const float yTop = static_cast<float>(f.anchorY) * mpp;
        const float yBot = -static_cast<float>(f.h - f.anchorY) * mpp;
        const float tw = static_cast<float>(batch.texWidth);
        const float th = static_cast<float>(batch.texHeight);
        const float u0 = (static_cast<float>(f.x) + 0.25f) / tw;
        const float u1 = (static_cast<float>(f.x + f.w) - 0.25f) / tw;
        const float v0 = (static_cast<float>(f.y) + 0.25f) / th;
        const float v1 = (static_cast<float>(f.y + f.h) - 0.25f) / th;
        const float baseX = a.x + dirX * 2.0f;
        const float baseY = a.y + 0.22f + dirY * 2.0f;
        const float baseZ = a.z + dirZ * 2.0f;
        auto corner = [&](float px, float py, float u, float v) {
            AppendTexturedVertex(batch.vertices,
                                 baseX + sx * px + ux * py, baseY + uy * py, baseZ + sz * px + uz * py,
                                 u, v, 1.0f, 1.0f, 1.0f, 1.0f);
        };
        corner(x0, yTop, u0, v0);
        corner(x0, yBot, u0, v1);
        corner(x1, yBot, u1, v1);
        corner(x0, yTop, u0, v0);
        corner(x1, yBot, u1, v1);
        corner(x1, yTop, u1, v0);
        mTexturedPedestrianMask[ai] = 1;
    }

    // Terrain spans: cSTETerrainView3D::ComputeTextureSpansForRow / DrawTerrainSurface.
    // Temperature (grid98) picks the TextureMapTable row; moisture picks the column.
    // DoLighting @ 0x100748b34 is approximated as a camera-relative hemisphere light.
    const int cellsX = city.CellCountX();
    const int cellsZ = city.CellCountZ();
    mTexturedTerrain = false;
    if (!mTerrainIniLoaded) {
        mTerrainIniLoaded = true;
        std::vector<Uint8> ini;
        if (dbpfSys.LoadAssetByTGI(cSC4TGI{0u, kTerrainIniGroup, kTerrainIniInstance}, ini)) {
            ParseTerrainIni(ini, mTerrainTextureTable, mTerrainHighCliff, mTerrainLowCliff, mTerrainBeach);
        }
    }
    if (mTerrainTextureTable.size() == 32u * 16u && cellsX > 0 && cellsZ > 0) {
        constexpr std::size_t kMissing = static_cast<std::size_t>(-1);
        auto getTerrainBatch = [&](Uint8 tex, bool blend) -> TexturedMeshBatch* {
            const Uint32 key = static_cast<Uint32>(tex) | (blend ? 0x100u : 0u);
            if (auto found = mTerrainBatchIndex.find(key); found != mTerrainBatchIndex.end()) {
                return found->second == kMissing ? nullptr : &mTexturedBatches[found->second];
            }
            cFSHImage image;
            std::vector<Uint8> bytes;
            bool loaded = false;
            for (Uint32 instance : {0x400u | tex, static_cast<Uint32>(tex)}) {
                bytes.clear();
                if (dbpfSys.LoadAssetByTGI(cSC4TGI{kTypeFSH, kTerrainFshGroup, instance}, bytes) &&
                    image.Parse(bytes.data(), bytes.size()) && image.width != 0 && image.height != 0) {
                    loaded = true;
                    break;
                }
            }
            if (!loaded) {
                mTerrainBatchIndex.emplace(key, kMissing);
                return nullptr;
            }
            TexturedMeshBatch created;
            created.fshInstanceId = tex;
            created.texWidth = image.width;
            created.texHeight = image.height;
            created.rgbaPixels = std::move(image.rgbaPixels);
            created.isTerrain = true;
            created.isTerrainBlend = blend;
            mTerrainBatchIndex.emplace(key, mTexturedBatches.size());
            mTexturedBatches.push_back(std::move(created));
            return &mTexturedBatches.back();
        };

        // DoLighting @ 0x100748b34: low afternoon sun from camera upper-left
        // (matches live Konradshohe warm directional light + long shadows).
        const float yaw = (mRotationIndex * 0.5f + 0.25f) * 3.14159265f;
        const float toCamX = std::cos(yaw), toCamZ = std::sin(yaw);
        const float rightX = toCamZ, rightZ = -toCamX;
        float lx = -0.72f * rightX + 0.22f * toCamX;
        float ly = 0.48f;
        float lz = -0.72f * rightZ + 0.22f * toCamZ;
        const float llen = std::sqrt(lx * lx + ly * ly + lz * lz);
        lx /= llen; ly /= llen; lz /= llen;
        constexpr float kAmbient = 0.34f, kDiffuse = 0.92f;
        const float flatLight = kAmbient + kDiffuse * ly;

        const int vx = cellsX + 1, vz = cellsZ + 1;
        std::vector<float> shade(static_cast<std::size_t>(vx) * static_cast<std::size_t>(vz), 1.0f);
        for (int z = 0; z < vz; ++z) {
            for (int x = 0; x < vx; ++x) {
                const float hx0 = city.GetVertexHeight(std::max(0, x - 1), z);
                const float hx1 = city.GetVertexHeight(std::min(cellsX, x + 1), z);
                const float hz0 = city.GetVertexHeight(x, std::max(0, z - 1));
                const float hz1 = city.GetVertexHeight(x, std::min(cellsZ, z + 1));
                float nx = -(hx1 - hx0) / 32.0f, ny = 1.0f, nz = -(hz1 - hz0) / 32.0f;
                const float nlen = std::sqrt(nx * nx + ny * ny + nz * nz);
                nx /= nlen; ny /= nlen; nz /= nlen;
                const float ndl = std::max(0.0f, nx * lx + ny * ly + nz * lz);
                shade[static_cast<std::size_t>(z) * vx + x] =
                    std::clamp((kAmbient + kDiffuse * ndl) / flatLight, 0.35f, 1.25f);
            }
        }

        const WeatherSimulatorSaveRecord& weather = city.WeatherSaveRecord();
        const std::size_t cellCount = static_cast<std::size_t>(cellsX) * static_cast<std::size_t>(cellsZ);
        const bool haveClimate =
            weather.grid98.rows == static_cast<Uint32>(cellsX) && weather.grid98.cols == static_cast<Uint32>(cellsZ) &&
            weather.grid98.cells.size() == cellCount &&
            weather.moistureGrid138.rows == static_cast<Uint32>(cellsX) &&
            weather.moistureGrid138.cols == static_cast<Uint32>(cellsZ) &&
            weather.moistureGrid138.cells.size() == cellCount;
        const float sea = city.SeaLevel();
        constexpr float kHighCliffRise = 12.0f;
        constexpr float kLowCliffRise = 7.0f;
        constexpr float kBeachHeight = 1.5f;
        std::vector<Uint8> cellTex(cellCount, 0);
        for (int cz = 0; cz < cellsZ; ++cz) {
            for (int cx = 0; cx < cellsX; ++cx) {
                const float c00 = city.GetVertexHeight(cx, cz), c01 = city.GetVertexHeight(cx, cz + 1);
                const float c11 = city.GetVertexHeight(cx + 1, cz + 1), c10 = city.GetVertexHeight(cx + 1, cz);
                const float minY = std::min({c00, c01, c11, c10});
                const float rise = std::max({c00, c01, c11, c10}) - minY;
                Uint8 tex = 0;
                if (rise > kHighCliffRise && mTerrainHighCliff != 0) tex = mTerrainHighCliff;
                else if (rise > kLowCliffRise && mTerrainLowCliff != 0) tex = mTerrainLowCliff;
                else if (minY < sea + kBeachHeight && mTerrainBeach != 0) tex = mTerrainBeach;
                if (tex == 0) {
                    int row = 22, col = 7;
                    if (haveClimate) {
                        const std::size_t gi = static_cast<std::size_t>(cx) * static_cast<std::size_t>(cellsZ) +
                                               static_cast<std::size_t>(cz);
                        row = weather.grid98.cells[gi] >> 3;
                        col = weather.moistureGrid138.cells[gi] >> 4;
                    }
                    tex = mTerrainTextureTable[static_cast<std::size_t>(row) * 16u + static_cast<std::size_t>(col)];
                }
                cellTex[static_cast<std::size_t>(cz) * cellsX + cx] = tex;
            }
        }

        auto vertexWeight = [&](int x, int z, Uint8 tex) -> float {
            int hits = 0, total = 0;
            for (int dz = -1; dz <= 0; ++dz) {
                for (int dx = -1; dx <= 0; ++dx) {
                    const int cx = x + dx, cz = z + dz;
                    if (cx < 0 || cz < 0 || cx >= cellsX || cz >= cellsZ) continue;
                    ++total;
                    if (cellTex[static_cast<std::size_t>(cz) * cellsX + cx] == tex) ++hits;
                }
            }
            return total > 0 ? static_cast<float>(hits) / static_cast<float>(total) : 0.0f;
        };

        std::size_t terrainCells = 0;
        for (int cz = 0; cz < cellsZ; ++cz) {
            for (int cx = 0; cx < cellsX; ++cx) {
                const int ix[4] = {cx, cx, cx + 1, cx + 1};
                const int iz[4] = {cz, cz + 1, cz + 1, cz};
                float ys[4];
                for (int k = 0; k < 4; ++k) ys[k] = city.GetVertexHeight(ix[k], iz[k]);
                auto emit = [&](TexturedMeshBatch& batch, const float* weights) {
                    for (int k : {0, 1, 2, 0, 2, 3}) {
                        const float wx = ix[k] * 16.0f, wz = iz[k] * 16.0f;
                        const float sh = shade[static_cast<std::size_t>(iz[k]) * vx + ix[k]];
                        AppendTexturedVertex(batch.vertices, wx, ys[k], wz,
                                             wx / kTerrainTextureWorldSize, wz / kTerrainTextureWorldSize,
                                             sh * kTerrainTint[0], sh * kTerrainTint[1], sh * kTerrainTint[2],
                                             weights[k]);
                    }
                };
                const Uint8 own = cellTex[static_cast<std::size_t>(cz) * cellsX + cx];
                TexturedMeshBatch* base = getTerrainBatch(own, false);
                if (base == nullptr) continue;
                static constexpr float kOpaque[4] = {1.0f, 1.0f, 1.0f, 1.0f};
                emit(*base, kOpaque);
                ++terrainCells;

                Uint8 seen[9];
                int seenCount = 0;
                for (int dz = -1; dz <= 1; ++dz) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int nx2 = cx + dx, nz2 = cz + dz;
                        if (nx2 < 0 || nz2 < 0 || nx2 >= cellsX || nz2 >= cellsZ) continue;
                        const Uint8 other = cellTex[static_cast<std::size_t>(nz2) * cellsX + nx2];
                        if (other == own || std::find(seen, seen + seenCount, other) != seen + seenCount) continue;
                        seen[seenCount++] = other;
                        float weights[4];
                        bool any = false;
                        for (int k = 0; k < 4; ++k) {
                            weights[k] = vertexWeight(ix[k], iz[k], other);
                            any = any || weights[k] > 0.0f;
                        }
                        if (!any) continue;
                        if (TexturedMeshBatch* overlay = getTerrainBatch(other, true)) emit(*overlay, weights);
                    }
                }
            }
        }
        mTexturedTerrain = terrainCells == cellCount;
    }

    // Lot base textures (cSC4LotBaseTextureOccupant) sit on the terrain under props/buildings.
    for (const SC4LotTextureRecord& tex : city.LotTextures()) {
        TexturedMeshBatch* batch = getOrLoadBatch(tex.textureId + 4u);
        if (batch == nullptr) batch = getOrLoadBatch(tex.textureId);
        if (batch == nullptr) continue;
        const int cx = tex.cellX;
        const int cz = tex.cellZ;
        if (cx < 0 || cz < 0 || cx >= cellsX || cz >= cellsZ) continue;
        const float lift = 0.10f;
        const float xs[4] = {cx * 16.0f, (cx + 1) * 16.0f, (cx + 1) * 16.0f, cx * 16.0f};
        const float zs[4] = {cz * 16.0f, cz * 16.0f, (cz + 1) * 16.0f, (cz + 1) * 16.0f};
        const float ys[4] = {city.GetVertexHeight(cx, cz), city.GetVertexHeight(cx + 1, cz),
                             city.GetVertexHeight(cx + 1, cz + 1), city.GetVertexHeight(cx, cz + 1)};
        static constexpr float kU[4] = {0.0f, 1.0f, 1.0f, 0.0f};
        static constexpr float kV[4] = {0.0f, 0.0f, 1.0f, 1.0f};
        for (int corner : {0, 1, 2, 0, 2, 3}) {
            const int uvIdx = (corner + tex.orientation) & 3;
            AppendTexturedVertex(batch->vertices, xs[corner], ys[corner] + lift, zs[corner],
                                 kU[uvIdx], kV[uvIdx], 1.0f, 1.0f, 1.0f, 1.0f);
        }
    }

    std::size_t total = 0;
    mTexturedBatches.reserve(mTexturedBatches.size() + byTexture.size());
    for (auto& entry : byTexture) {
        TexturedMeshBatch& batch = entry.second;
        if (batch.vertices.empty()) continue;
        total += batch.vertices.size();
        mTexturedBatches.push_back(std::move(batch));
    }
    for (const auto& batch : mTexturedBatches) {
        if (batch.isTerrain) total += batch.vertices.size();
    }
    return total;
}

void cSC4GLCityRenderer::RebuildCityGeometry(const cSC4City& city, DataOverlay overlay) {
    mOverlay = overlay;
    mVertices.clear();
    if (mDbpfSys != nullptr && overlay == DataOverlay::None) {
        BuildTexturedBatchesFromDBPF(city, *mDbpfSys);
    } else {
        RetireTexturedBatches();
        mTexturedBuildingMask.clear();
        mTexturedFloraMask.clear();
        mTexturedPedestrianMask.clear();
        mTexturedTerrain = false;
    }
    const int nx = city.CellCountX();
    const int nz = city.CellCountZ();
    mVertices.reserve(static_cast<std::size_t>(nx) * static_cast<std::size_t>(nz) * 12u
                      + city.Buildings().size() * 80u
                      + city.Networks().size() * 120u
                      + 64u);

    const float sea = city.SeaLevel();
    std::vector<RenderVertex> waterVertices;
    waterVertices.reserve(static_cast<std::size_t>(nx) * static_cast<std::size_t>(nz) * 3u);

    // 1. Terrain Mesh & Data Overlay / Zone Tinting
    for (int cz = 0; cz < nz; ++cz) {
        for (int cx = 0; cx < nx; ++cx) {
            float x0 = cx * 16.0f;
            float z0 = cz * 16.0f;
            float x1 = (cx + 1) * 16.0f;
            float z1 = (cz + 1) * 16.0f;

            float y00 = city.GetVertexHeight(cx, cz);
            float y10 = city.GetVertexHeight(cx + 1, cz);
            float y11 = city.GetVertexHeight(cx + 1, cz + 1);
            float y01 = city.GetVertexHeight(cx, cz + 1);
            float avgY = 0.25f * (y00 + y10 + y11 + y01);

            float r = 0.44f, g = 0.60f, b = 0.34f;
            if (avgY < sea - 1.5f) {
                // Underwater seabed sand
                r = 0.62f; g = 0.58f; b = 0.44f;
            } else if (avgY < sea + 3.5f) {
                // Shoreline beach
                r = 0.76f; g = 0.72f; b = 0.52f;
            } else {
                float elevFactor = std::clamp((avgY - sea) / 180.0f, 0.0f, 1.0f);
                r = 0.40f + 0.22f * elevFactor;
                g = 0.58f - 0.08f * elevFactor;
                b = 0.32f + 0.12f * elevFactor;
            }

            // Apply active DataOverlay (SimGrid / Zone visualization)
            ZoneType zt = city.GetCellZone(cx, cz);
            if (overlay == DataOverlay::Zones && zt != ZoneType::None) {
                ZoneColor(zt, r, g, b);
            } else if (overlay == DataOverlay::LandValue) {
                float v = city.LandValueGrid().GetCellValue(cx, cz) / 255.0f;
                r = 0.15f + 0.20f * (1.0f - v);
                g = 0.25f + 0.70f * v;
                b = 0.20f + 0.55f * v;
            } else if (overlay == DataOverlay::Pollution) {
                float v = city.PollutionGrid().GetCellValue(cx, cz) / 255.0f;
                r = 0.35f + 0.65f * v;
                g = 0.55f * (1.0f - 0.6f * v);
                b = 0.25f * (1.0f - v);
            } else if (overlay == DataOverlay::Crime) {
                float v = city.CrimeGrid().GetCellValue(cx, cz) / 255.0f;
                r = 0.30f + 0.70f * v;
                g = 0.50f * (1.0f - v);
                b = 0.30f * (1.0f - v);
            } else if (overlay == DataOverlay::Traffic) {
                float v = city.TrafficGrid().GetCellValue(cx, cz) / 255.0f;
                r = 0.25f + 0.75f * v;
                g = 0.60f - 0.40f * v;
                b = 0.35f - 0.25f * v;
            } else if (overlay == DataOverlay::Power) {
                float v = city.PowerGrid().GetCellValue(cx, cz) / 255.0f;
                r = v > 0.1f ? 0.25f : 0.65f;
                g = v > 0.1f ? 0.78f : 0.22f;
                b = 0.28f;
            } else if (overlay == DataOverlay::Water) {
                float v = city.WaterGrid().GetCellValue(cx, cz) / 255.0f;
                r = 0.20f;
                g = v > 0.1f ? 0.58f : 0.32f;
                b = v > 0.1f ? 0.92f : 0.35f;
            } else if (overlay == DataOverlay::MayorRating) {
                const float rating = static_cast<float>(city.MayorRatingGrid().GetCellValue(cx, cz));
                MayorRatingColor(rating, r, g, b);
            } else if (zt != ZoneType::None) {
                float zr, zg, zb;
                ZoneColor(zt, zr, zg, zb);
                r = r * 0.55f + zr * 0.45f;
                g = g * 0.55f + zg * 0.45f;
                b = b * 0.55f + zb * 0.45f;
            }

            // Subtle SC4 grid line shading on tile borders
            if (((cx + cz) & 1) == 0) {
                r *= 0.97f; g *= 0.97f; b *= 0.97f;
            }

            // Flat fallback only when Terrain.ini FSH spans are unavailable (overlays keep it).
            if (!mTexturedTerrain || overlay != DataOverlay::None) {
                PushQuad(mVertices,
                         x0, y00, z0,
                         x0, y01, z1,
                         x1, y11, z1,
                         x1, y10, z0,
                         r, g, b, 1.0f);
            }

            // InitWaterSurfaceItems @ 0x1007481be — defer translucent plane so it
            // blends over opaque terrain. Depth darkens shoreline grey → deep slate.
            if (y00 < sea || y10 < sea || y11 < sea || y01 < sea) {
                const float wx[4] = {x0, x0, x1, x1};
                const float wz[4] = {z0, z1, z1, z0};
                const float wy[4] = {y00, y01, y11, y10};
                for (int k : {0, 1, 2, 0, 2, 3}) {
                    const float t = std::clamp((sea - wy[k]) / 18.0f, 0.0f, 1.0f);
                    waterVertices.push_back(RenderVertex{
                        wx[k], sea + 0.15f, wz[k], 0.0f, 1.0f, 0.0f,
                        0.36f + (0.21f - 0.36f) * t,
                        0.37f + (0.23f - 0.37f) * t,
                        0.40f + (0.29f - 0.40f) * t,
                        0.68f + (0.94f - 0.68f) * t});
                }
            }
        }
    }

    // 2. Network Occupant Quads (Roads, Streets, Avenues, Highways, Rails)
    for (const auto& net : city.Networks()) {
        float nr = 0.26f, ng = 0.27f, nb = 0.29f;
        switch (net.networkType) {
            case NetworkType::Road:       nr = 0.25f; ng = 0.26f; nb = 0.28f; break;
            case NetworkType::Street:     nr = 0.38f; ng = 0.39f; nb = 0.41f; break;
            case NetworkType::Avenue:     nr = 0.20f; ng = 0.22f; nb = 0.25f; break;
            case NetworkType::Highway:    nr = 0.32f; ng = 0.30f; nb = 0.28f; break;
            case NetworkType::Rail:       nr = 0.42f; ng = 0.32f; nb = 0.24f; break;
            case NetworkType::OneWayRoad: nr = 0.27f; ng = 0.28f; nb = 0.32f; break;
            default:                      nr = 0.28f; ng = 0.29f; nb = 0.31f; break;
        }
        const float lift = 0.35f;
        PushQuad(mVertices,
                 net.verts[0].x, net.verts[0].y + lift, net.verts[0].z,
                 net.verts[1].x, net.verts[1].y + lift, net.verts[1].z,
                 net.verts[2].x, net.verts[2].y + lift, net.verts[2].z,
                 net.verts[3].x, net.verts[3].y + lift, net.verts[3].z,
                 nr, ng, nb, 1.0f);
        AppendNetworkDetail(mVertices, city, net, mAnimPhase);
    }

    // 3. Building Occupants (3D architectural massing colored by wealth, exemplar hash & zone).
    // Buildings that produced an S3D mesh skip the stand-in box so the textured model is visible.
    for (std::size_t bi = 0; bi < city.Buildings().size(); ++bi) {
        if (bi < mTexturedBuildingMask.size() && mTexturedBuildingMask[bi] != 0) continue;
        const auto& bld = city.Buildings()[bi];
        float br = 0.76f, bg = 0.72f, bb = 0.66f;
        float rr = 0.42f, rg = 0.38f, rb = 0.35f;

        Uint32 h = bld.exemplarTGI.instance;
        float tint = ((h & 0xFFu) / 255.0f - 0.5f) * 0.14f;

        if (bld.zoneHint >= ZoneType::ResidentialLow && bld.zoneHint <= ZoneType::ResidentialHigh) {
            if (bld.wealthHint == 1)      { br = 0.78f; bg = 0.68f; bb = 0.58f; rr = 0.52f; rg = 0.34f; rb = 0.26f; }
            else if (bld.wealthHint == 2) { br = 0.82f; bg = 0.76f; bb = 0.68f; rr = 0.44f; rg = 0.36f; rb = 0.32f; }
            else                          { br = 0.88f; bg = 0.85f; bb = 0.80f; rr = 0.32f; rg = 0.42f; rb = 0.48f; }
        } else if (bld.zoneHint >= ZoneType::CommercialLow && bld.zoneHint <= ZoneType::CommercialHigh) {
            br = 0.58f; bg = 0.68f; bb = 0.78f;
            rr = 0.34f; rg = 0.40f; rb = 0.46f;
        } else if (bld.zoneHint >= ZoneType::IndustrialAg && bld.zoneHint <= ZoneType::IndustrialHigh) {
            br = 0.70f; bg = 0.64f; bb = 0.52f;
            rr = 0.46f; rg = 0.42f; rb = 0.36f;
        } else {
            // Civic / Landmark / Utility buildings
            br = 0.78f; bg = 0.75f; bb = 0.70f;
            rr = 0.45f; rg = 0.48f; rb = 0.52f;
        }

        br = std::clamp(br + tint, 0.2f, 0.95f);
        bg = std::clamp(bg + tint, 0.2f, 0.95f);
        bb = std::clamp(bb + tint, 0.2f, 0.95f);

        float minY = bld.minY;
        float maxY = std::max(bld.maxY, minY + 3.5f);
        PushBox(mVertices, bld.minX, minY, bld.minZ, bld.maxX, maxY, bld.maxZ, br, bg, bb, rr, rg, rb);
    }

    // 4. Flora Occupants — soft ground shadow (AddShadowDecal-style) + stand-in mesh
    for (std::size_t fi = 0; fi < city.Flora().size(); ++fi) {
        const auto& fl = city.Flora()[fi];
        if (fl.y < sea) continue;
        const float sr = (mZoom < 1.8f) ? 3.2f : 2.2f;
        PushQuad(mVertices,
                 fl.x - sr, fl.y + 0.08f, fl.z - sr * 0.65f,
                 fl.x - sr, fl.y + 0.08f, fl.z + sr * 0.65f,
                 fl.x + sr, fl.y + 0.08f, fl.z + sr * 0.65f,
                 fl.x + sr, fl.y + 0.08f, fl.z - sr * 0.65f,
                 0.05f, 0.07f, 0.04f, 0.35f);
        if (fi < mTexturedFloraMask.size() && mTexturedFloraMask[fi] != 0) continue;
        PushBox(mVertices,
                fl.x - 1.8f, fl.y, fl.z - 1.8f,
                fl.x + 1.8f, fl.y + 7.5f, fl.z + 1.8f,
                0.20f, 0.48f, 0.22f,
                0.26f, 0.58f, 0.28f);
    }

    // 4b. Pedestrian stand-ins when sprite atlas 0x29a5d1ec did not resolve.
    for (std::size_t ai = 0; ai < city.Automata().size(); ++ai) {
        if (ai < mTexturedPedestrianMask.size() && mTexturedPedestrianMask[ai] != 0) continue;
        const SC4AutomatonRecord& a = city.Automata()[ai];
        if (a.kind != SC4AutomatonRecord::Kind::Pedestrian) continue;
        if (a.y < sea) continue;
        PushBox(mVertices,
                a.x - 0.35f, a.y, a.z - 0.35f,
                a.x + 0.35f, a.y + 1.7f, a.z + 0.35f,
                0.72f, 0.58f, 0.42f,
                0.55f, 0.42f, 0.30f);
    }

    // 5. Transmission pylons on civic power lots / power-plant buildings
    for (const auto& bld : city.Buildings()) {
        if (!IsCivicPowerBuilding(bld)) continue;
        if (!std::isfinite(bld.minX) || !std::isfinite(bld.maxX) ||
            !std::isfinite(bld.minY) || !std::isfinite(bld.minZ) || !std::isfinite(bld.maxZ)) {
            continue;
        }
        const float zc = 0.5f * (bld.minZ + bld.maxZ);
        AppendTransmissionPylon(mVertices, bld.minX - 2.0f, bld.minY, zc);
        AppendTransmissionPylon(mVertices, bld.maxX + 2.0f, bld.minY, zc);
    }

    // 6. Highlighted tile cursor
    if (mHoverTileX >= 0 && mHoverTileZ >= 0) {
        AppendHoverCursor(mVertices, city, mHoverTileX, mHoverTileZ);
    }

    // 7. Translucent water last (InitWaterSurfaceItems) for correct alpha over terrain.
    mVertices.insert(mVertices.end(), waterVertices.begin(), waterVertices.end());

    mVertexCount = mVertices.size();
    mCityVboStale = true;
    mGeometryDirty = false;
}

void cSC4GLCityRenderer::BuildViewProjMatrix(int viewportW, int viewportH, float outVP[16]) const noexcept {
    float aspect = viewportH > 0 ? static_cast<float>(viewportW) / static_cast<float>(viewportH) : 1.5f;
    float halfH = 520.0f / std::max(0.25f, mZoom);
    float halfW = halfH * aspect;

    // Isometric camera direction matching SimCity 4's trimetric/isometric projection
    float yaw = (mRotationIndex * 0.5f + 0.25f) * 3.14159265f; // 45°, 135°, 225°, 315°
    float pitch = 0.6154797f; // ~35.264° isometric elevation

    float dirX = std::cos(pitch) * std::cos(yaw);
    float dirY = std::sin(pitch);
    float dirZ = std::cos(pitch) * std::sin(yaw);

    float dist = 2200.0f;
    float eyeX = mCamTargetX + dirX * dist;
    float eyeY = mCamTargetY + dirY * dist;
    float eyeZ = mCamTargetZ + dirZ * dist;

    // LookAt basis vectors (f = normalize(target - eye))
    float fx = -dirX, fy = -dirY, fz = -dirZ;
    // Right vector s = normalize(f x up(0,1,0))
    float sx = -fz, sy = 0.0f, sz = fx;
    float slen = std::sqrt(sx * sx + sz * sz);
    if (slen > 1e-5f) { sx /= slen; sz /= slen; }
    // Up vector u = s x f
    float ux = sy * fz - sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy - sy * fx;

    // View matrix (column-major)
    float V[16] = {
         sx,  ux, -fx, 0.0f,
         sy,  uy, -fy, 0.0f,
         sz,  uz, -fz, 0.0f,
        -(sx * eyeX + sy * eyeY + sz * eyeZ),
        -(ux * eyeX + uy * eyeY + uz * eyeZ),
         (fx * eyeX + fy * eyeY + fz * eyeZ),
         1.0f
    };

    // Orthographic projection matrix (column-major, near = -4000, far = 6000)
    float l = -halfW, r = halfW, b = -halfH, t = halfH, n = 10.0f, f = 5500.0f;
    float P[16] = {
        2.0f / (r - l), 0.0f,           0.0f,            0.0f,
        0.0f,           2.0f / (t - b), 0.0f,            0.0f,
        0.0f,           0.0f,          -2.0f / (f - n),  0.0f,
       -(r + l)/(r - l), -(t + b)/(t - b), -(f + n)/(f - n), 1.0f
    };

    // outVP = P * V
    for (int col = 0; col < 4; ++col) {
        for (int row = 0; row < 4; ++row) {
            outVP[col * 4 + row] =
                P[0 * 4 + row] * V[col * 4 + 0] +
                P[1 * 4 + row] * V[col * 4 + 1] +
                P[2 * 4 + row] * V[col * 4 + 2] +
                P[3 * 4 + row] * V[col * 4 + 3];
        }
    }
}

bool cSC4GLCityRenderer::ScreenToTile(int screenX, int screenY, int viewportW, int viewportH,
                                      const cSC4City& city, int& outTileX, int& outTileZ) const noexcept {
    if (viewportW <= 0 || viewportH <= 0) return false;
    float ndcX = (2.0f * screenX) / viewportW - 1.0f;
    float ndcY = 1.0f - (2.0f * screenY) / viewportH;

    float aspect = static_cast<float>(viewportW) / static_cast<float>(viewportH);
    float halfH = 520.0f / std::max(0.25f, mZoom);
    float halfW = halfH * aspect;

    float camRightOffset = ndcX * halfW;
    float camUpOffset    = ndcY * halfH;

    float yaw = (mRotationIndex * 0.5f + 0.25f) * 3.14159265f;
    float pitch = 0.6154797f;

    float dirX = std::cos(pitch) * std::cos(yaw);
    float dirY = std::sin(pitch);
    float dirZ = std::cos(pitch) * std::sin(yaw);

    float fx = -dirX, fy = -dirY, fz = -dirZ;
    float sx = -fz, sz = fx;
    float slen = std::sqrt(sx * sx + sz * sz);
    if (slen > 1e-5f) { sx /= slen; sz /= slen; }
    float ux = -sz * fy;
    float uy = sz * fx - sx * fz;
    float uz = sx * fy;

    // Ray origin on camera plane through (camRightOffset, camUpOffset)
    float rx0 = mCamTargetX + sx * camRightOffset + ux * camUpOffset;
    float ry0 = mCamTargetY + uy * camUpOffset;
    float rz0 = mCamTargetZ + sz * camRightOffset + uz * camUpOffset;

    // Intersect ray (rx0 + t*fx, ry0 + t*fy, rz0 + t*fz) with plane y = mCamTargetY
    if (std::abs(fy) < 1e-4f) return false;
    float t = (mCamTargetY - ry0) / fy;
    float wx = rx0 + t * fx;
    float wz = rz0 + t * fz;

    int tx = static_cast<int>( std::floor(wx / 16.0f) );
    int tz = static_cast<int>( std::floor(wz / 16.0f) );
    if (tx < 0 || tx >= city.CellCountX() || tz < 0 || tz >= city.CellCountZ()) return false;
    outTileX = tx;
    outTileZ = tz;
    return true;
}

void cSC4GLCityRenderer::RenderFrame(int viewportWidth, int viewportHeight, const cSC4City& city) {
    if (!mGLReady) {
        InitGL();
        RebuildCityGeometry(city, mOverlay);
    } else if (mGeometryDirty) {
        RebuildCityGeometry(city, mOverlay);
    }
    if (mGLReady) SyncGpuResources();

    glViewport(0, 0, viewportWidth, viewportHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Classic SimCity 4 void; slight warm lift toward afternoon sky.
    glClearColor(0.14f, 0.15f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bool anyTextured = false;
    for (const auto& batch : mTexturedBatches) {
        if (batch.glTex != 0 && batch.vao != 0) {
            anyTextured = true;
            break;
        }
    }
    if (mVertexCount == 0 && !anyTextured) return;

    float vp[16];
    BuildViewProjMatrix(viewportWidth, viewportHeight, vp);

    auto drawTexturedBatches = [&](bool terrain, bool terrainBlend) {
        if (!anyTextured || mTexShaderProg == 0) return;
        glUseProgram(mTexShaderProg);
        GLint vpLoc = glGetUniformLocation(mTexShaderProg, "uViewProj");
        glUniformMatrix4fv(vpLoc, 1, GL_FALSE, vp);
        GLint texLoc = glGetUniformLocation(mTexShaderProg, "uTex");
        glUniform1i(texLoc, 0);
        glActiveTexture(GL_TEXTURE0);
        for (const auto& batch : mTexturedBatches) {
            if (batch.isTerrain != terrain || batch.isTerrainBlend != terrainBlend) continue;
            if (batch.glTex == 0 || batch.vao == 0 || batch.vertices.empty()) continue;
            glBindTexture(GL_TEXTURE_2D, batch.glTex);
            glBindVertexArray(batch.vao);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(batch.vertices.size()));
        }
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    };

    // Terrain before translucent water in the city mesh (DrawTerrainSurface then water).
    drawTexturedBatches(true, false);
    glDepthFunc(GL_LEQUAL);
    drawTexturedBatches(true, true);
    glDepthFunc(GL_LESS);

    if (mVertexCount > 0) {
        glUseProgram(mShaderProg);
        GLint loc = glGetUniformLocation(mShaderProg, "uViewProj");
        glUniformMatrix4fv(loc, 1, GL_FALSE, vp);

        glBindVertexArray(mVao);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mVertexCount));
        glBindVertexArray(0);
    }

    drawTexturedBatches(false, false);
}

} // namespace opensc4
