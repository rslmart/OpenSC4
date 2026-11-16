#include "opensc4/gl_city_renderer.h"
#include "opensc4/dbpf.h"
#include "opensc4/formats.h"
#include <cmath>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#elif defined(__APPLE__)
#include <OpenGL/gl3.h>
#else
#define GL_GLEXT_PROTOTYPES
#include <GL/gl.h>
#endif

namespace opensc4 {

template <typename Out>
static void PushTriangle(Out& out,
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

    out.push_back(RenderVertex{x0, y0, z0, nx, ny, nz, r, g, b, a});
    out.push_back(RenderVertex{x1, y1, z1, nx, ny, nz, r, g, b, a});
    out.push_back(RenderVertex{x2, y2, z2, nx, ny, nz, r, g, b, a});
}

template <typename Out>
static void PushQuad(Out& out,
                     float x0, float y0, float z0,
                     float x1, float y1, float z1,
                     float x2, float y2, float z2,
                     float x3, float y3, float z3,
                     float r, float g, float b, float a = 1.0f) {
    PushTriangle(out, x0, y0, z0, x1, y1, z1, x2, y2, z2, r, g, b, a);
    PushTriangle(out, x0, y0, z0, x2, y2, z2, x3, y3, z3, r, g, b, a);
}

static void PushBoxWithTag(std::vector<RenderVertex>& out,
                           float minX, float minY, float minZ,
                           float maxX, float maxY, float maxZ,
                           float r, float g, float b,
                           float roofR, float roofG, float roofB,
                           float wallTag = 1.0f) {
    // Top roof (always opaque tag 1.0 so roof doesn't get wall windows)
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
             r, g, b, wallTag);
    // North (-Z)
    PushQuad(out,
             maxX, minY, minZ,
             minX, minY, minZ,
             minX, maxY, minZ,
             maxX, maxY, minZ,
             r * 0.86f, g * 0.86f, b * 0.86f, wallTag);
    // East (+X)
    PushQuad(out,
             maxX, minY, maxZ,
             maxX, minY, minZ,
             maxX, maxY, minZ,
             maxX, maxY, maxZ,
             r * 0.93f, g * 0.93f, b * 0.93f, wallTag);
    // West (-X)
    PushQuad(out,
             minX, minY, minZ,
             minX, minY, maxZ,
             minX, maxY, maxZ,
             minX, maxY, minZ,
             r * 0.80f, g * 0.80f, b * 0.80f, wallTag);
}

static void PushBox(std::vector<RenderVertex>& out,
                    float minX, float minY, float minZ,
                    float maxX, float maxY, float maxZ,
                    float r, float g, float b,
                    float roofR, float roofG, float roofB) {
    PushBoxWithTag(out, minX, minY, minZ, maxX, maxY, maxZ, r, g, b, roofR, roofG, roofB, 1.0f);
}

static void PushPitchedRoof(std::vector<RenderVertex>& out,
                            float minX, float baseY, float minZ,
                            float maxX, float peakY, float maxZ,
                            float r, float g, float b, bool ridgeAlongX) {
    if (ridgeAlongX) {
        const float midZ = 0.5f * (minZ + maxZ);
        // South slope (+Z)
        PushQuad(out, minX, baseY, maxZ, maxX, baseY, maxZ, maxX, peakY, midZ, minX, peakY, midZ,
                 r, g, b, 1.0f);
        // North slope (-Z)
        PushQuad(out, maxX, baseY, minZ, minX, baseY, minZ, minX, peakY, midZ, maxX, peakY, midZ,
                 r * 0.86f, g * 0.86f, b * 0.86f, 1.0f);
        // West & East gables
        PushTriangle(out, minX, baseY, minZ, minX, baseY, maxZ, minX, peakY, midZ,
                     r * 0.82f, g * 0.82f, b * 0.82f, 1.0f);
        PushTriangle(out, maxX, baseY, maxZ, maxX, baseY, minZ, maxX, peakY, midZ,
                     r * 0.92f, g * 0.92f, b * 0.92f, 1.0f);
    } else {
        const float midX = 0.5f * (minX + maxX);
        // East slope (+X)
        PushQuad(out, maxX, baseY, maxZ, maxX, baseY, minZ, midX, peakY, minZ, midX, peakY, maxZ,
                 r * 0.92f, g * 0.92f, b * 0.92f, 1.0f);
        // West slope (-X)
        PushQuad(out, minX, baseY, minZ, minX, baseY, maxZ, midX, peakY, maxZ, midX, peakY, minZ,
                 r * 0.82f, g * 0.82f, b * 0.82f, 1.0f);
        // South & North gables
        PushTriangle(out, minX, baseY, maxZ, maxX, baseY, maxZ, midX, peakY, maxZ,
                     r, g, b, 1.0f);
        PushTriangle(out, maxX, baseY, minZ, minX, baseY, minZ, midX, peakY, minZ,
                     r * 0.86f, g * 0.86f, b * 0.86f, 1.0f);
    }
}

static void PushPyramidSpire(std::vector<RenderVertex>& out,
                             float minX, float baseY, float minZ,
                             float maxX, float peakY, float maxZ,
                             float r, float g, float b) {
    const float midX = 0.5f * (minX + maxX);
    const float midZ = 0.5f * (minZ + maxZ);
    PushTriangle(out, minX, baseY, maxZ, maxX, baseY, maxZ, midX, peakY, midZ, r, g, b, 1.0f);
    PushTriangle(out, maxX, baseY, maxZ, maxX, baseY, minZ, midX, peakY, midZ, r * 0.92f, g * 0.92f, b * 0.92f, 1.0f);
    PushTriangle(out, maxX, baseY, minZ, minX, baseY, minZ, midX, peakY, midZ, r * 0.84f, g * 0.84f, b * 0.84f, 1.0f);
    PushTriangle(out, minX, baseY, minZ, minX, baseY, maxZ, midX, peakY, midZ, r * 0.78f, g * 0.78f, b * 0.78f, 1.0f);
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
    std::vector<Uint8>().swap(batch.rgbaPixels);

    const auto stride = static_cast<GLsizei>(sizeof(TexturedRenderVertex));
    glGenVertexArrays(1, &batch.vao);
    glGenBuffers(1, &batch.vbo);
    glBindVertexArray(batch.vao);
    glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(batch.vertices.size() * sizeof(TexturedRenderVertex)),
                 batch.vertices.data(),
                 GL_DYNAMIC_DRAW);
    batch.gpuVertexCount = batch.vertices.size();
    batch.verticesDirty = false;
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
        out vec3 vWorldPos;
        out vec3 vNormal;
        out vec4 vColor;
        void main() {
            vWorldPos = aPos;
            vNormal = aNormal;
            vColor = aColor;
            gl_Position = uViewProj * vec4(aPos, 1.0);
        }
    )";
    const char* fsSrc = R"(#version 300 es
        precision highp float;
        in vec3 vWorldPos;
        in vec3 vNormal;
        in vec4 vColor;
        out vec4 fragColor;

        float hash21(vec2 p) {
            p = fract(p * vec2(123.34, 456.21));
            p += dot(p, p + 45.32);
            return fract(p.x * p.y);
        }

        void main() {
            vec3 n = normalize(vNormal);
            vec3 sunDir = normalize(vec3(0.52, 0.78, 0.34));
            float ndl = max(dot(n, sunDir), 0.0);
            float skyHemi = 0.5 + 0.5 * n.y;
            float light = 0.38 + 0.16 * skyHemi + 0.52 * ndl;

            vec3 baseRgb = vColor.rgb;
            float matTag = vColor.a;
            float outAlpha = 1.0;

            if (matTag < 0.99) {
                // Translucent water or cursor
                outAlpha = matTag;
                if (matTag > 0.70 && matTag < 0.85) {
                    float wave = sin(vWorldPos.x * 0.35 + vWorldPos.z * 0.25) * 0.04;
                    baseRgb += vec3(wave * 0.5, wave * 0.8, wave * 1.2);
                }
            } else if (matTag >= 1.8 && matTag <= 4.5 && abs(n.y) < 0.35) {
                // Architectural building wall with procedural window grid
                float isOffice = (matTag > 2.5 && matTag < 3.5) ? 1.0 : 0.0;
                float isInd = (matTag > 3.5) ? 1.0 : 0.0;
                float floorH = isOffice > 0.5 ? 3.5 : (isInd > 0.5 ? 4.8 : 3.1);
                float bayW   = isOffice > 0.5 ? 2.4 : (isInd > 0.5 ? 4.2 : 2.9);
                float hCoord = (abs(n.x) > abs(n.z)) ? vWorldPos.z : vWorldPos.x;

                float fy = fract(vWorldPos.y / floorH);
                float fx = fract(hCoord / bayW);
                float floorIdx = floor(vWorldPos.y / floorH);
                float bayIdx   = floor(hCoord / bayW);

                bool isWindowX = (fx > 0.16 && fx < 0.84);
                bool isWindowY = (fy > 0.22 && fy < 0.84);
                if (isInd > 0.5) {
                    isWindowY = (fy > 0.45 && fy < 0.82);
                }

                if (isWindowX && isWindowY) {
                    float roomSeed = hash21(vec2(bayIdx * 1.37 + floorIdx * 0.73, floorIdx));
                    vec3 glassSky = mix(vec3(0.18, 0.28, 0.42), vec3(0.42, 0.58, 0.76), clamp(fy, 0.0, 1.0));
                    vec3 warmInterior = vec3(0.92, 0.82, 0.58);
                    float litMask = step(0.62, roomSeed);
                    vec3 paneCol = mix(glassSky, warmInterior, litMask * 0.55);
                    baseRgb = mix(baseRgb * 0.55, paneCol, 0.82);
                } else if (fy < 0.08 || fy > 0.93) {
                    // Floor slab / cornice band
                    baseRgb *= 0.82;
                } else if (!isWindowX) {
                    // Vertical pilaster / mullion
                    baseRgb *= 0.94;
                }
            } else if (matTag > 4.5) {
                // Terrain surface: subtle cell grid + contour shading
                vec2 cellUV = fract(vWorldPos.xz / 16.0);
                float edgeDist = min(min(cellUV.x, 1.0 - cellUV.x), min(cellUV.y, 1.0 - cellUV.y));
                float gridLine = 1.0 - smoothstep(0.0, 0.035, edgeDist);
                baseRgb = mix(baseRgb, baseRgb * 0.86, gridLine * 0.35);
                // Steep slope rock blending
                float slope = 1.0 - clamp(n.y, 0.0, 1.0);
                if (slope > 0.22) {
                    vec3 rockCol = vec3(0.46, 0.44, 0.41);
                    baseRgb = mix(baseRgb, rockCol, clamp((slope - 0.22) * 2.8, 0.0, 0.85));
                }
            }

            fragColor = vec4(baseRgb * light, outAlpha);
        }
    )";
#else
    const char* vsSrc = R"(#version 330 core
        layout(location = 0) in vec3 aPos;
        layout(location = 1) in vec3 aNormal;
        layout(location = 2) in vec4 aColor;
        uniform mat4 uViewProj;
        out vec3 vWorldPos;
        out vec3 vNormal;
        out vec4 vColor;
        void main() {
            vWorldPos = aPos;
            vNormal = aNormal;
            vColor = aColor;
            gl_Position = uViewProj * vec4(aPos, 1.0);
        }
    )";
    const char* fsSrc = R"(#version 330 core
        in vec3 vWorldPos;
        in vec3 vNormal;
        in vec4 vColor;
        out vec4 fragColor;

        float hash21(vec2 p) {
            p = fract(p * vec2(123.34, 456.21));
            p += dot(p, p + 45.32);
            return fract(p.x * p.y);
        }

        void main() {
            vec3 n = normalize(vNormal);
            vec3 sunDir = normalize(vec3(0.52, 0.78, 0.34));
            float ndl = max(dot(n, sunDir), 0.0);
            float skyHemi = 0.5 + 0.5 * n.y;
            float light = 0.38 + 0.16 * skyHemi + 0.52 * ndl;

            vec3 baseRgb = vColor.rgb;
            float matTag = vColor.a;
            float outAlpha = 1.0;

            if (matTag < 0.99) {
                outAlpha = matTag;
                if (matTag > 0.70 && matTag < 0.85) {
                    float wave = sin(vWorldPos.x * 0.35 + vWorldPos.z * 0.25) * 0.04;
                    baseRgb += vec3(wave * 0.5, wave * 0.8, wave * 1.2);
                }
            } else if (matTag >= 1.8 && matTag <= 4.5 && abs(n.y) < 0.35) {
                float isOffice = (matTag > 2.5 && matTag < 3.5) ? 1.0 : 0.0;
                float isInd = (matTag > 3.5) ? 1.0 : 0.0;
                float floorH = isOffice > 0.5 ? 3.5 : (isInd > 0.5 ? 4.8 : 3.1);
                float bayW   = isOffice > 0.5 ? 2.4 : (isInd > 0.5 ? 4.2 : 2.9);
                float hCoord = (abs(n.x) > abs(n.z)) ? vWorldPos.z : vWorldPos.x;

                float fy = fract(vWorldPos.y / floorH);
                float fx = fract(hCoord / bayW);
                float floorIdx = floor(vWorldPos.y / floorH);
                float bayIdx   = floor(hCoord / bayW);

                bool isWindowX = (fx > 0.16 && fx < 0.84);
                bool isWindowY = (fy > 0.22 && fy < 0.84);
                if (isInd > 0.5) {
                    isWindowY = (fy > 0.45 && fy < 0.82);
                }

                if (isWindowX && isWindowY) {
                    float roomSeed = hash21(vec2(bayIdx * 1.37 + floorIdx * 0.73, floorIdx));
                    vec3 glassSky = mix(vec3(0.18, 0.28, 0.42), vec3(0.42, 0.58, 0.76), clamp(fy, 0.0, 1.0));
                    vec3 warmInterior = vec3(0.92, 0.82, 0.58);
                    float litMask = step(0.62, roomSeed);
                    vec3 paneCol = mix(glassSky, warmInterior, litMask * 0.55);
                    baseRgb = mix(baseRgb * 0.55, paneCol, 0.82);
                } else if (fy < 0.08 || fy > 0.93) {
                    baseRgb *= 0.82;
                } else if (!isWindowX) {
                    baseRgb *= 0.94;
                }
            } else if (matTag > 4.5) {
                vec2 cellUV = fract(vWorldPos.xz / 16.0);
                float edgeDist = min(min(cellUV.x, 1.0 - cellUV.x), min(cellUV.y, 1.0 - cellUV.y));
                float gridLine = 1.0 - smoothstep(0.0, 0.035, edgeDist);
                baseRgb = mix(baseRgb, baseRgb * 0.86, gridLine * 0.35);
                float slope = 1.0 - clamp(n.y, 0.0, 1.0);
                if (slope > 0.22) {
                    vec3 rockCol = vec3(0.46, 0.44, 0.41);
                    baseRgb = mix(baseRgb, rockCol, clamp((slope - 0.22) * 2.8, 0.0, 0.85));
                }
            }

            fragColor = vec4(baseRgb * light, outAlpha);
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

    glGenVertexArrays(1, &mCursorVao);
    glGenBuffers(1, &mCursorVbo);
    glBindVertexArray(mCursorVao);
    glBindBuffer(GL_ARRAY_BUFFER, mCursorVbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(mCursorVertices.size() * sizeof(RenderVertex)),
                 nullptr,
                 GL_DYNAMIC_DRAW);
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
    mBatchIndexByFsh.clear();
    mTerrainBatchIndex.clear();
    mMissingFsh.clear();
    mModelCache.clear();
    mTerrainIniLoaded = false;
    mAutomataBatchIndex.clear();
    mAutomataModels.clear();
    mPedestrianSprites.clear();
    mMissingPedestrianSprites.clear();
}

void cSC4GLCityRenderer::SyncGpuResources() {
    for (auto& batch : mRetiredBatches) ReleaseTexturedBatchGL(batch);
    mRetiredBatches.clear();
    for (auto& batch : mTexturedBatches) {
        if (batch.glTex == 0) {
            if (!batch.vertices.empty()) UploadTexturedBatch(batch);
        } else if (batch.verticesDirty && batch.vbo != 0) {
            glBindBuffer(GL_ARRAY_BUFFER, batch.vbo);
            glBufferData(GL_ARRAY_BUFFER,
                         static_cast<GLsizeiptr>(batch.vertices.size() * sizeof(TexturedRenderVertex)),
                         batch.vertices.empty() ? nullptr : batch.vertices.data(),
                         GL_DYNAMIC_DRAW);
            batch.gpuVertexCount = batch.vertices.size();
            batch.verticesDirty = false;
        }
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
    mTexturedBatches.clear();
    mBatchIndexByFsh.clear();
    mMissingFsh.clear();
    if (mCursorVbo) glDeleteBuffers(1, &mCursorVbo);
    if (mCursorVao) glDeleteVertexArrays(1, &mCursorVao);
    if (mVbo) glDeleteBuffers(1, &mVbo);
    if (mVao) glDeleteVertexArrays(1, &mVao);
    if (mShaderProg) glDeleteProgram(mShaderProg);
    if (mTexShaderProg) glDeleteProgram(mTexShaderProg);
    mCursorVbo = 0;
    mCursorVao = 0;
    mCursorVertexCount = 0;
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
    mYawRadians = kSC4ViewYaw;
    mPitchRadians = kSC4ViewPitch;
    if (mRotationIndex != mModelRotationIndex) mGeometryDirty = true;
}

void cSC4GLCityRenderer::Pan(float dxWorld, float dzWorld) noexcept {
    float c = std::cos(mYawRadians);
    float s = std::sin(mYawRadians);
    float zoomScale = std::max(0.20f, mZoom);
    mCamTargetX += (dxWorld * s + dzWorld * c) / zoomScale;
    mCamTargetZ += (-dxWorld * c + dzWorld * s) / zoomScale;
}

void cSC4GLCityRenderer::ZoomBy(float factor) noexcept {
    mZoom = std::clamp(mZoom * factor, 0.18f, 8.0f);
}

void cSC4GLCityRenderer::Rotate90(int dir) noexcept {
    mRotationIndex = (mRotationIndex + dir + 4) & 3;
    mYawRadians = kSC4ViewYaw + mRotationIndex * 1.57079633f;
    mPitchRadians = kSC4ViewPitch;
    if (mRotationIndex != mModelRotationIndex) mGeometryDirty = true;
}

void cSC4GLCityRenderer::OrbitFree(float deltaYawRadians, float deltaPitchRadians) noexcept {
    constexpr float kTwoPi = 6.283185307f;
    constexpr float kMinPitch = 0.12f;  // ~6.9° above horizon
    constexpr float kMaxPitch = 1.48f;  // ~84.8° top-down
    mYawRadians = std::fmod(mYawRadians + deltaYawRadians, kTwoPi);
    if (mYawRadians < 0.0f) mYawRadians += kTwoPi;
    mPitchRadians = std::clamp(mPitchRadians + deltaPitchRadians, kMinPitch, kMaxPitch);
    // Keep mRotationIndex synced to nearest 90° quadrant for any legacy callers
    int quad = static_cast<int>(std::round((mYawRadians - kSC4ViewYaw) / 1.57079633f));
    mRotationIndex = ((quad % 4) + 4) % 4;
    if (mRotationIndex != mModelRotationIndex) mGeometryDirty = true;
}

void cSC4GLCityRenderer::SetFreeCameraAngles(float yawRadians, float pitchRadians) noexcept {
    mYawRadians = yawRadians;
    mPitchRadians = std::clamp(pitchRadians, 0.12f, 1.48f);
    int quad = static_cast<int>(std::round((mYawRadians - kSC4ViewYaw) / 1.57079633f));
    mRotationIndex = ((quad % 4) + 4) % 4;
    if (mRotationIndex != mModelRotationIndex) mGeometryDirty = true;
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

static constexpr float kMarkLift = 0.21f;

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
                                const SC4NetworkRecord& net) {
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
    }
}

template <typename Out>
static void AppendHoverCursor(Out& out, const cSC4City& city, int hx, int hz) {
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

struct HoverCursorSink {
    RenderVertex* data = nullptr;
    std::size_t size = 0;
    std::size_t capacity = 0;
    void push_back(const RenderVertex& vertex) {
        if (data != nullptr && size < capacity) data[size++] = vertex;
    }
};

void cSC4GLCityRenderer::UploadHoverCursor(const cSC4City& city) {
    static_assert(sizeof(mCursorVertices) / sizeof(RenderVertex) == 30,
                  "hover cursor is the fill quad plus four rim quads");
    mCursorVertexCount = 0;
    if (mHoverTileX >= 0 && mHoverTileZ >= 0) {
        HoverCursorSink sink{mCursorVertices.data(), 0, mCursorVertices.size()};
        AppendHoverCursor(sink, city, mHoverTileX, mHoverTileZ);
        mCursorVertexCount = sink.size;
    }
    if (!mGLReady || mCursorVbo == 0 || mCursorVertexCount == 0) return;
    glBindBuffer(GL_ARRAY_BUFFER, mCursorVbo);
    glBufferSubData(GL_ARRAY_BUFFER,
                    0,
                    static_cast<GLsizeiptr>(mCursorVertexCount * sizeof(RenderVertex)),
                    mCursorVertices.data());
}

constexpr Uint32 kTerrainIniGroup    = 0x8a5971c5u;
constexpr Uint32 kTerrainIniInstance = 0xaa597172u;
constexpr Uint32 kTerrainFshGroup    = 0x891b0e1au;
constexpr float kTerrainTextureWorldSize = 32.0f; // Metres covered by one repeat
// Matched to SC4's lit terrain, which renders darker and bluer than the raw texture.
constexpr float kTerrainTint[3] = {0.85f, 0.88f, 1.12f};

// Reads the first complete [<Climate>TextureMapTable] (rows "NN=0x..,0x..,")
// and the cliff/beach textures from the matching [<Climate>MiscTextures].
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
    for (auto& batch : mTexturedBatches) {
        batch.vertices.clear();
        batch.verticesDirty = true;
    }
    mTexturedBuildingMask.assign(city.Buildings().size(), 0);
    mTexturedNetworkMask.assign(city.Networks().size(), 0);

    // The returned pointer is only valid until the next call (push_back may reallocate).
    auto getOrLoadBatch = [&](Uint32 fshId) -> TexturedMeshBatch* {
        if (auto found = mBatchIndexByFsh.find(fshId); found != mBatchIndexByFsh.end()) {
            return &mTexturedBatches[found->second];
        }
        if (mMissingFsh.count(fshId) != 0) return nullptr;

        std::vector<Uint8> bytes;
        cFSHImage image;
        const bool loaded = dbpfSys.LoadAssetByTypeAndInstance(kTypeFSH, fshId, bytes) &&
                            image.Parse(bytes.data(), bytes.size());
        const std::size_t need = static_cast<std::size_t>(image.width) *
                                 static_cast<std::size_t>(image.height) * 4u;
        if (!loaded || image.width == 0 || image.height == 0 || image.rgbaPixels.size() < need) {
            mMissingFsh.insert(fshId);
            return nullptr;
        }
        TexturedMeshBatch created;
        created.fshInstanceId = fshId;
        created.texWidth = image.width;
        created.texHeight = image.height;
        created.rgbaPixels = std::move(image.rgbaPixels);
        created.verticesDirty = true;
        mBatchIndexByFsh.emplace(fshId, mTexturedBatches.size());
        mTexturedBatches.push_back(std::move(created));
        return &mTexturedBatches.back();
    };

    for (std::size_t ni = 0; ni < city.Networks().size(); ++ni) {
        const auto& net = city.Networks()[ni];
        if (net.textureId == 0) continue;
        // Network FSH instances are piece + zoom (0..4); +4 is the 128x128 Zoom 5 image.
        TexturedMeshBatch* batch = getOrLoadBatch(net.textureId + 4u);
        if (batch == nullptr) batch = getOrLoadBatch(net.textureId);
        if (batch == nullptr) continue;
        const float lift = 0.20f;
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
        mTexturedNetworkMask[ni] = 1u;
    }

    // Models only contain the faces visible from one fixed camera, so like SC4
    // we pick the model for the camera's quarter turn (rotation nibble 0x30 of
    // the instance) and turn it back into world space.
    constexpr std::size_t kMaxCachedModels = 8192;
    const unsigned camQuarter = static_cast<unsigned>(mRotationIndex) & 3u;
    mModelRotationIndex = mRotationIndex;
    auto appendModel = [&](Uint32 s3dInstanceId, unsigned objectOrientation,
                           float anchorX, float anchorY, float anchorZ,
                           Uint32 fallbackFshId) -> bool {
        if (s3dInstanceId == 0) return false;
        if (!std::isfinite(anchorX) || !std::isfinite(anchorY) || !std::isfinite(anchorZ)) return false;
        const unsigned viewRotation = (objectOrientation - camQuarter) & 3u;
        const Uint32 instanceId = (s3dInstanceId & ~0x30u) | (viewRotation << 4);

        auto cached = mModelCache.find(instanceId);
        if (cached == mModelCache.end()) {
            if (mModelCache.size() >= kMaxCachedModels) return false;
            cS3DModel loaded;
            std::vector<Uint8> s3dBytes;
            if (dbpfSys.LoadAssetByTypeAndInstance(kTypeS3D, instanceId, s3dBytes)) {
                loaded.Parse(s3dBytes.data(), s3dBytes.size());
            }
            cached = mModelCache.emplace(instanceId, std::move(loaded)).first;
        }
        const cS3DModel& model = cached->second;
        if (model.vertices.empty() || model.meshes.empty()) return false;

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

        const float modelCenterX = 0.5f * (minX + maxX);
        const float modelCenterZ = 0.5f * (minZ + maxZ);
        // Model n+1 is model n turned (x, z) -> (-z, x), which is PlaceModelVertex's
        // quarter turn, so undoing the camera offset is camQuarter of those.
        const unsigned orientation = camQuarter;
        const std::size_t vertCount = model.vertices.size();
        bool appended = false;
        for (const S3DMesh& mesh : model.meshes) {
            // Each rotation's model has its own textures; the fallback is the rotation-0 one.
            Uint32 fshId = mesh.materialIndex < model.materials.size()
                ? model.materials[mesh.materialIndex].textureId
                : 0u;
            if (fshId == 0) fshId = fallbackFshId;
            if (fshId == 0) continue;
            TexturedMeshBatch* batch = getOrLoadBatch(fshId);
            if (batch == nullptr) continue;
            const std::size_t end = std::min<std::size_t>(model.indices.size(),
                                                          static_cast<std::size_t>(mesh.firstIndex) + mesh.indexCount);
            for (std::size_t i = mesh.firstIndex; i < end; ++i) {
                const Uint32 vi = model.indices[i];
                if (vi >= vertCount) continue;
                const S3DVertex& v = model.vertices[vi];
                float px = 0.0f, py = 0.0f, pz = 0.0f;
                PlaceModelVertex(v, modelCenterX, minY, modelCenterZ,
                                 anchorX, anchorY, anchorZ, orientation, px, py, pz);
                float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
                UnpackS3DColor(v.color, r, g, b, a);
                AppendTexturedVertex(batch->vertices, px, py, pz, v.u, v.v, r, g, b, a);
                appended = true;
            }
        }
        return appended;
    };

    const int cellsX = city.CellCountX();
    const int cellsZ = city.CellCountZ();

    // Terrain: Terrain.ini's map table picks each cell's texture from the
    // weather simulator's temperature (row) and moisture (column) grids.
    if (!mTerrainIniLoaded) {
        mTerrainIniLoaded = true;
        std::vector<Uint8> ini;
        if (dbpfSys.LoadAssetByTGI(cSC4TGI{0u, kTerrainIniGroup, kTerrainIniInstance}, ini)) {
            ParseTerrainIni(ini, mTerrainTextureTable, mTerrainHighCliff, mTerrainLowCliff, mTerrainBeach);
        }
    }
    mTexturedTerrain = false;
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
            created.verticesDirty = true;
            created.isTerrain = true;
            created.isTerrainBlend = blend;
            mTerrainBatchIndex.emplace(key, mTexturedBatches.size());
            mTexturedBatches.push_back(std::move(created));
            return &mTexturedBatches.back();
        };

        // SC4's sprites are lit from the camera's upper left in every rotation, so
        // the terrain light turns with the view like the models do.
        const float yaw = kSC4ViewYaw + static_cast<float>(camQuarter) * 1.57079633f;
        const float toCamX = std::cos(yaw), toCamZ = std::sin(yaw);
        const float rightX = toCamZ, rightZ = -toCamX;
        float lx = -0.55f * rightX + 0.30f * toCamX;
        float ly = 0.80f;
        float lz = -0.55f * rightZ + 0.30f * toCamZ;
        const float llen = std::sqrt(lx * lx + ly * ly + lz * lz);
        lx /= llen; ly /= llen; lz /= llen;
        constexpr float kAmbient = 0.42f, kDiffuse = 0.78f;
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
        // The weather grids are indexed [x][z].
        const bool haveClimate =
            weather.grid98.rows == static_cast<Uint32>(cellsX) && weather.grid98.cols == static_cast<Uint32>(cellsZ) &&
            weather.grid98.cells.size() == cellCount &&
            weather.moistureGrid138.rows == static_cast<Uint32>(cellsX) &&
            weather.moistureGrid138.cols == static_cast<Uint32>(cellsZ) &&
            weather.moistureGrid138.cells.size() == cellCount;
        const float sea = city.SeaLevel();
        constexpr float kHighCliffRise = 12.0f; // ~37 degrees across a cell
        constexpr float kLowCliffRise = 7.0f;   // ~24 degrees
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

        // Fraction of the (up to four) cells around vertex (x, z) that use `tex`.
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

        // Each cell lays its own texture opaque, then every neighbouring texture
        // over it weighted per vertex, so both sides of an edge meet at the same mix.
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
                        const float s = shade[static_cast<std::size_t>(iz[k]) * vx + ix[k]];
                        AppendTexturedVertex(batch.vertices, wx, ys[k], wz,
                                             wx / kTerrainTextureWorldSize, wz / kTerrainTextureWorldSize,
                                             s * kTerrainTint[0], s * kTerrainTint[1], s * kTerrainTint[2],
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

    // Lot base textures lie on the terrain below props and buildings.
    mLotTextureCellMask.assign(static_cast<std::size_t>(cellsX) * static_cast<std::size_t>(cellsZ), 0u);
    for (const SC4LotTextureRecord& tex : city.LotTextures()) {
        TexturedMeshBatch* batch = getOrLoadBatch(tex.textureId + 4u);
        if (batch == nullptr) batch = getOrLoadBatch(tex.textureId);
        if (batch == nullptr) continue;
        const int cx = tex.cellX;
        const int cz = tex.cellZ;
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
        mLotTextureCellMask[static_cast<std::size_t>(cz) * static_cast<std::size_t>(cellsX) +
                            static_cast<std::size_t>(cx)] = 1u;
    }

    for (std::size_t bi = 0; bi < city.Buildings().size(); ++bi) {
        const SC4BuildingRecord& bld = city.Buildings()[bi];
        if (appendModel(bld.s3dInstanceId, bld.orientation,
                        0.5f * (bld.minX + bld.maxX), bld.minY, 0.5f * (bld.minZ + bld.maxZ),
                        bld.fshTextureId)) {
            mTexturedBuildingMask[bi] = 1u;
        }
    }
    for (const SC4PropRecord& prop : city.Props()) {
        appendModel(prop.s3dInstanceId, prop.orientation,
                    0.5f * (prop.minX + prop.maxX), prop.minY, 0.5f * (prop.minZ + prop.maxZ), 0u);
    }
    mTexturedFloraMask.assign(city.Flora().size(), 0u);
    for (std::size_t fi = 0; fi < city.Flora().size(); ++fi) {
        const SC4FloraRecord& fl = city.Flora()[fi];
        if (appendModel(fl.s3dInstanceId, fl.orientation, fl.x, fl.y, fl.z, 0u)) {
            mTexturedFloraMask[fi] = 1u;
        }
    }

    std::size_t total = 0;
    for (const auto& batch : mTexturedBatches) total += batch.vertices.size();
    return total;
}

void cSC4GLCityRenderer::RebuildCityGeometry(const cSC4City& city, DataOverlay overlay) {
    ++mRebuildCount;
    mOverlay = overlay;
    mVertices.clear();
    if (mDbpfSys != nullptr && overlay == DataOverlay::None) {
        BuildTexturedBatchesFromDBPF(city, *mDbpfSys);
    } else {
        for (auto& batch : mTexturedBatches) {
            batch.vertices.clear();
            batch.verticesDirty = true;
        }
        mTexturedBuildingMask.clear();
        mTexturedNetworkMask.clear();
        mTexturedFloraMask.clear();
        mLotTextureCellMask.clear();
        mTexturedTerrain = false;
    }
    const int nx = city.CellCountX();
    const int nz = city.CellCountZ();
    mVertices.reserve(static_cast<std::size_t>(nx) * static_cast<std::size_t>(nz) * 12u
                      + city.Buildings().size() * 140u
                      + city.Networks().size() * 90u
                      + 128u);

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

            float r = 0.42f, g = 0.59f, b = 0.33f;
            if (avgY < sea - 1.5f) {
                r = 0.58f; g = 0.54f; b = 0.41f;
            } else if (avgY < sea + 3.0f) {
                r = 0.78f; g = 0.73f; b = 0.54f;
            } else {
                float elevFactor = std::clamp((avgY - sea) / 180.0f, 0.0f, 1.0f);
                r = 0.36f + 0.24f * elevFactor;
                g = 0.56f - 0.10f * elevFactor;
                b = 0.29f + 0.12f * elevFactor;
            }

            ZoneType zt = city.GetCellZone(cx, cz);
            float terrainTag = 5.0f; // Tag 5.0 enables shader grid & slope rock blending
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
                r = r * 0.62f + zr * 0.38f;
                g = g * 0.62f + zg * 0.38f;
                b = b * 0.62f + zb * 0.38f;
            }

            if (!mTexturedTerrain) {
                PushQuad(mVertices,
                         x0, y00, z0,
                         x0, y01, z1,
                         x1, y11, z1,
                         x1, y10, z0,
                         r, g, b, terrainTag);
            } else if (zt != ZoneType::None &&
                       (mLotTextureCellMask.empty() ||
                        mLotTextureCellMask[static_cast<std::size_t>(cz) * static_cast<std::size_t>(nx) + cx] == 0)) {
                // Zoned but undeveloped ground shows the zone colour over the terrain.
                float zr, zg, zb;
                ZoneColor(zt, zr, zg, zb);
                const float lift = 0.05f;
                PushQuad(mVertices,
                         x0, y00 + lift, z0,
                         x0, y01 + lift, z1,
                         x1, y11 + lift, z1,
                         x1, y10 + lift, z0,
                         zr, zg, zb, 0.35f);
            }

            // Defer translucent water plane so it renders after opaque terrain
            if (y00 < sea || y10 < sea || y11 < sea || y01 < sea) {
                // SC4's water darkens from a grey shoreline band to deep slate with depth;
                // the plane is cut against the terrain per pixel by the depth test.
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

    // 1b. Developed Lot Grounds (paved plazas, lawns, industrial concrete pads)
    if (overlay == DataOverlay::None) {
        for (const auto& lot : city.Lots()) {
            if (lot.zoneType == ZoneType::None) continue;
            if (!mLotTextureCellMask.empty() &&
                mLotTextureCellMask[static_cast<std::size_t>(lot.minZ) * static_cast<std::size_t>(nx) + lot.minX] != 0) {
                continue;
            }
            const float lx0 = lot.minX * 16.0f + 0.6f;
            const float lz0 = lot.minZ * 16.0f + 0.6f;
            const float lx1 = (lot.maxX + 1) * 16.0f - 0.6f;
            const float lz1 = (lot.maxZ + 1) * 16.0f - 0.6f;
            if (lx1 <= lx0 || lz1 <= lz0) continue;
            const float ly = lot.altitudeY + 0.08f;
            float pr = 0.56f, pg = 0.58f, pb = 0.55f;
            if (lot.zoneType == ZoneType::ResidentialLow) {
                pr = 0.34f; pg = 0.60f; pb = 0.30f; // manicured lawn
            } else if (lot.zoneType >= ZoneType::CommercialLow && lot.zoneType <= ZoneType::CommercialHigh) {
                pr = 0.68f; pg = 0.67f; pb = 0.64f; // stone plaza
            } else if (lot.zoneType >= ZoneType::IndustrialAg && lot.zoneType <= ZoneType::IndustrialHigh) {
                pr = 0.52f; pg = 0.50f; pb = 0.46f; // concrete yard
            }
            PushQuad(mVertices, lx0, ly, lz0, lx0, ly, lz1, lx1, ly, lz1, lx1, ly, lz0, pr, pg, pb, 1.0f);
        }
    }

    // 2. Network Occupant Quads (Roads, Streets, Avenues, Highways, Rails)
    for (std::size_t ni = 0; ni < city.Networks().size(); ++ni) {
        const auto& net = city.Networks()[ni];
        const bool hasFshTex = (ni < mTexturedNetworkMask.size() && mTexturedNetworkMask[ni] != 0u);
        if (!hasFshTex) {
            float nr = 0.24f, ng = 0.25f, nb = 0.27f;
            switch (net.networkType) {
                case NetworkType::Road:       nr = 0.23f; ng = 0.24f; nb = 0.26f; break;
                case NetworkType::Street:     nr = 0.36f; ng = 0.37f; nb = 0.39f; break;
                case NetworkType::Avenue:     nr = 0.19f; ng = 0.20f; nb = 0.23f; break;
                case NetworkType::Highway:    nr = 0.30f; ng = 0.29f; nb = 0.27f; break;
                case NetworkType::Rail:       nr = 0.40f; ng = 0.31f; nb = 0.23f; break;
                case NetworkType::OneWayRoad: nr = 0.25f; ng = 0.26f; nb = 0.30f; break;
                default:                      nr = 0.26f; ng = 0.27f; nb = 0.29f; break;
            }
            const float lift = 0.18f;
            PushQuad(mVertices,
                     net.verts[0].x, net.verts[0].y + lift, net.verts[0].z,
                     net.verts[1].x, net.verts[1].y + lift, net.verts[1].z,
                     net.verts[2].x, net.verts[2].y + lift, net.verts[2].z,
                     net.verts[3].x, net.verts[3].y + lift, net.verts[3].z,
                     nr, ng, nb, 1.0f);
            AppendNetworkDetail(mVertices, city, net);
        } else {
            if (!std::isfinite(net.centerX) || !std::isfinite(net.centerZ)) continue;
            const int tx = static_cast<int>(std::floor(net.centerX / 16.0f));
            const int tz = static_cast<int>(std::floor(net.centerZ / 16.0f));
            const bool ns = CellLinked(city, tx, tz - 1, net.networkType) ||
                            CellLinked(city, tx, tz + 1, net.networkType);
            const bool ew = CellLinked(city, tx + 1, tz, net.networkType) ||
                            CellLinked(city, tx - 1, tz, net.networkType);
            if (IsPowerLineTile(net)) {
                AppendPowerPole(mVertices, net, ns || !ew);
            }
        }
    }

    // 3. Building Occupants (Multi-Tiered 3D Architecture + Procedural Window Facades)
    // Buildings drawn from their S3D model skip the procedural stand-in.
    for (std::size_t bi = 0; bi < city.Buildings().size(); ++bi) {
        if (bi < mTexturedBuildingMask.size() && mTexturedBuildingMask[bi] != 0) continue;
        const auto& bld = city.Buildings()[bi];
        float br = 0.76f, bg = 0.72f, bb = 0.66f;
        float rr = 0.42f, rg = 0.38f, rb = 0.35f;
        float wallTag = 2.0f; // 2.0 = Residential, 3.0 = Commercial/Office, 4.0 = Industrial

        Uint32 h = bld.exemplarTGI.instance ^ (static_cast<Uint32>(bi) * 2654435761u);
        float tint = ((h & 0xFFu) / 255.0f - 0.5f) * 0.16f;
        const bool isRes = (bld.zoneHint >= ZoneType::ResidentialLow && bld.zoneHint <= ZoneType::ResidentialHigh);
        const bool isCom = (bld.zoneHint >= ZoneType::CommercialLow && bld.zoneHint <= ZoneType::CommercialHigh);
        const bool isInd = (bld.zoneHint >= ZoneType::IndustrialAg && bld.zoneHint <= ZoneType::IndustrialHigh);

        if (isRes) {
            wallTag = 2.0f;
            if (bld.wealthHint == 1)      { br = 0.78f; bg = 0.67f; bb = 0.56f; rr = 0.54f; rg = 0.32f; rb = 0.24f; }
            else if (bld.wealthHint == 2) { br = 0.83f; bg = 0.77f; bb = 0.69f; rr = 0.42f; rg = 0.35f; rb = 0.31f; }
            else                          { br = 0.90f; bg = 0.87f; bb = 0.82f; rr = 0.30f; rg = 0.42f; rb = 0.50f; }
        } else if (isCom) {
            wallTag = 3.0f;
            if (bld.wealthHint >= 3) { br = 0.52f; bg = 0.66f; bb = 0.80f; rr = 0.30f; rg = 0.38f; rb = 0.46f; }
            else                     { br = 0.64f; bg = 0.70f; bb = 0.76f; rr = 0.36f; rg = 0.40f; rb = 0.45f; }
        } else if (isInd) {
            wallTag = 4.0f;
            br = 0.70f; bg = 0.63f; bb = 0.52f;
            rr = 0.45f; rg = 0.41f; rb = 0.36f;
        } else {
            wallTag = 3.0f;
            br = 0.82f; bg = 0.79f; bb = 0.73f;
            rr = 0.44f; rg = 0.48f; rb = 0.52f;
        }

        br = std::clamp(br + tint, 0.22f, 0.95f);
        bg = std::clamp(bg + tint, 0.22f, 0.95f);
        bb = std::clamp(bb + tint, 0.22f, 0.95f);

        const float minX = bld.minX;
        const float maxX = bld.maxX;
        const float minZ = bld.minZ;
        const float maxZ = bld.maxZ;
        const float minY = bld.minY;
        const float maxY = std::max(bld.maxY, minY + 4.0f);
        const float height = maxY - minY;
        const float spanX = maxX - minX;
        const float spanZ = maxZ - minZ;

        if (isRes && height < 14.0f) {
            // Low-density residential cottage/house with 3D pitched roof
            const float wallTop = minY + height * 0.68f;
            PushBoxWithTag(mVertices, minX, minY, minZ, maxX, wallTop, maxZ,
                           br, bg, bb, rr, rg, rb, wallTag);
            PushPitchedRoof(mVertices, minX - 0.35f, wallTop, minZ - 0.35f,
                            maxX + 0.35f, maxY + 1.2f, maxZ + 0.35f,
                            rr, rg, rb, spanX >= spanZ);
        } else if (isInd) {
            // Industrial warehouse/factory + sawtooth roof bays + smokestack
            const float hallTop = minY + height * 0.82f;
            PushBoxWithTag(mVertices, minX, minY, minZ, maxX, hallTop, maxZ,
                           br, bg, bb, rr, rg, rb, wallTag);
            PushPitchedRoof(mVertices, minX, hallTop, minZ,
                            maxX, maxY, minZ + spanZ * 0.5f,
                            rr * 0.95f, rg * 0.95f, rb * 0.95f, true);
            PushPitchedRoof(mVertices, minX, hallTop, minZ + spanZ * 0.5f,
                            maxX, maxY, maxZ,
                            rr * 0.95f, rg * 0.95f, rb * 0.95f, true);
            if (spanX > 6.0f && spanZ > 6.0f) {
                const float sx0 = maxX - 2.4f;
                const float sz0 = minZ + 1.2f;
                PushBox(mVertices, sx0, hallTop, sz0, sx0 + 1.3f, maxY + 5.5f, sz0 + 1.3f,
                        0.48f, 0.42f, 0.38f, 0.25f, 0.22f, 0.20f);
            }
        } else if (height >= 36.0f && spanX > 8.0f && spanZ > 8.0f) {
            // High-rise skyscraper: lobby podium + main tower + upper setback + crown/spire
            const float podiumTop = minY + std::min(9.5f, height * 0.18f);
            PushBoxWithTag(mVertices, minX, minY, minZ, maxX, podiumTop, maxZ,
                           br * 0.92f, bg * 0.92f, bb * 0.92f, rr, rg, rb, wallTag);

            const float inset1X = spanX * 0.10f;
            const float inset1Z = spanZ * 0.10f;
            const float shaftTop = minY + height * 0.74f;
            PushBoxWithTag(mVertices,
                           minX + inset1X, podiumTop, minZ + inset1Z,
                           maxX - inset1X, shaftTop, maxZ - inset1Z,
                           br, bg, bb, rr, rg, rb, wallTag);

            const float inset2X = spanX * 0.22f;
            const float inset2Z = spanZ * 0.22f;
            PushBoxWithTag(mVertices,
                           minX + inset2X, shaftTop, minZ + inset2Z,
                           maxX - inset2X, maxY, maxZ - inset2Z,
                           br * 1.04f, bg * 1.04f, bb * 1.04f, rr, rg, rb, wallTag);

            if ((h & 3u) == 0u) {
                const float inset3X = spanX * 0.34f;
                const float inset3Z = spanZ * 0.34f;
                PushPyramidSpire(mVertices,
                                 minX + inset3X, maxY, minZ + inset3Z,
                                 maxX - inset3X, maxY + std::min(14.0f, height * 0.20f), maxZ - inset3Z,
                                 0.72f, 0.78f, 0.84f);
            } else {
                const float inset3X = spanX * 0.32f;
                const float inset3Z = spanZ * 0.32f;
                PushBox(mVertices,
                        minX + inset3X, maxY, minZ + inset3Z,
                        maxX - inset3X, maxY + 3.2f, maxZ - inset3Z,
                        rr * 0.85f, rg * 0.85f, rb * 0.85f, rr * 1.1f, rg * 1.1f, rb * 1.1f);
            }
        } else {
            // Mid-rise apartment / office / civic building with roof cornice & mechanical penthouse
            PushBoxWithTag(mVertices, minX, minY, minZ, maxX, maxY, maxZ,
                           br, bg, bb, rr, rg, rb, wallTag);
            // Roof cornice rim
            PushBox(mVertices, minX - 0.20f, maxY - 0.55f, minZ - 0.20f,
                    maxX + 0.20f, maxY + 0.35f, maxZ + 0.20f,
                    br * 0.84f, bg * 0.84f, bb * 0.84f, rr, rg, rb);
            if (spanX > 6.0f && spanZ > 6.0f) {
                const float px0 = minX + spanX * 0.30f;
                const float pz0 = minZ + spanZ * 0.30f;
                const float px1 = maxX - spanX * 0.30f;
                const float pz1 = maxZ - spanZ * 0.30f;
                PushBox(mVertices, px0, maxY + 0.35f, pz0, px1, maxY + 2.4f, pz1,
                        rr * 0.90f, rg * 0.90f, rb * 0.90f, rr * 1.08f, rg * 1.08f, rb * 1.08f);
            }
        }
    }

    // 4. Flora Occupants (3D Trees: trunk + layered tapered canopy)
    for (std::size_t fi = 0; fi < city.Flora().size(); ++fi) {
        if (fi < mTexturedFloraMask.size() && mTexturedFloraMask[fi] != 0) continue;
        const auto& fl = city.Flora()[fi];
        if (fl.y < sea) continue;
        const Uint32 th = static_cast<Uint32>(fi) * 747796405u;
        const float scale = 0.80f + 0.45f * static_cast<float>(th & 255u) / 255.0f;
        const float greenTint = (static_cast<float>((th >> 8) & 255u) / 255.0f - 0.5f) * 0.10f;
        const float cr = std::clamp(0.18f + greenTint * 0.5f, 0.10f, 0.35f);
        const float cg = std::clamp(0.48f + greenTint, 0.32f, 0.65f);
        const float cb = std::clamp(0.20f + greenTint * 0.4f, 0.12f, 0.35f);

        // Trunk
        const float trunkR = 0.45f * scale;
        const float trunkH = 2.6f * scale;
        PushBox(mVertices,
                fl.x - trunkR, fl.y, fl.z - trunkR,
                fl.x + trunkR, fl.y + trunkH, fl.z + trunkR,
                0.42f, 0.29f, 0.18f, 0.36f, 0.25f, 0.15f);
        // Lower canopy tier
        const float c1R = 2.4f * scale;
        const float c1Top = fl.y + 6.2f * scale;
        PushPyramidSpire(mVertices,
                         fl.x - c1R, fl.y + trunkH * 0.75f, fl.z - c1R,
                         fl.x + c1R, c1Top, fl.z + c1R,
                         cr, cg, cb);
        // Upper canopy crown
        const float c2R = 1.75f * scale;
        const float c2Top = fl.y + 8.8f * scale;
        PushPyramidSpire(mVertices,
                         fl.x - c2R, fl.y + 4.4f * scale, fl.z - c2R,
                         fl.x + c2R, c2Top, fl.z + c2R,
                         cr * 1.12f, cg * 1.12f, cb * 1.12f);
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

    // 6. Append translucent water plane last for proper alpha blending
    mVertices.insert(mVertices.end(), waterVertices.begin(), waterVertices.end());

    mVertexCount = mVertices.size();
    mCityVboStale = true;
    mGeometryDirty = false;
    mDriveCellsDirty = true;
    // Terrain heights live in the city mesh. Refresh the cursor against them
    // on the next frame without folding it back into that mesh.
    mHoverDirty = true;
}

void cSC4GLCityRenderer::BuildViewProjMatrix(int viewportW, int viewportH, float outVP[16]) const noexcept {
    float aspect = viewportH > 0 ? static_cast<float>(viewportW) / static_cast<float>(viewportH) : 1.5f;
    float zoomClamped = std::max(0.18f, mZoom);
    float halfH = 520.0f / zoomClamped;
    float halfW = halfH * aspect;

    // Continuous 360° yaw and pitch free camera
    float yaw = mYawRadians;
    float pitch = mPitchRadians;

    float dirX = std::cos(pitch) * std::cos(yaw);
    float dirY = std::sin(pitch);
    float dirZ = std::cos(pitch) * std::sin(yaw);

    float dist = mPerspectiveMode ? (1350.0f / zoomClamped) : 2200.0f;
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

    float P[16]{};
    if (mPerspectiveMode) {
        // Perspective projection matrix (45° vertical FOV, column-major)
        const float fovY = 0.78539816f; // 45 degrees
        const float fCot = 1.0f / std::tan(fovY * 0.5f);
        const float zNear = 8.0f;
        const float zFar = 12000.0f;
        P[0]  = fCot / aspect;
        P[5]  = fCot;
        P[10] = (zFar + zNear) / (zNear - zFar);
        P[11] = -1.0f;
        P[14] = (2.0f * zFar * zNear) / (zNear - zFar);
    } else {
        // Orthographic projection matrix
        float l = -halfW, r = halfW, b = -halfH, t = halfH, n = 10.0f, f = 6500.0f;
        P[0]  = 2.0f / (r - l);
        P[5]  = 2.0f / (t - b);
        P[10] = -2.0f / (f - n);
        P[12] = -(r + l) / (r - l);
        P[13] = -(t + b) / (t - b);
        P[14] = -(f + n) / (f - n);
        P[15] = 1.0f;
    }

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
    float zoomClamped = std::max(0.18f, mZoom);

    float yaw = mYawRadians;
    float pitch = mPitchRadians;

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

    float rx0 = 0.0f, ry0 = 0.0f, rz0 = 0.0f;
    float rdx = fx, rdy = fy, rdz = fz;

    if (mPerspectiveMode) {
        const float dist = 1350.0f / zoomClamped;
        rx0 = mCamTargetX + dirX * dist;
        ry0 = mCamTargetY + dirY * dist;
        rz0 = mCamTargetZ + dirZ * dist;
        const float tanHalfFov = std::tan(0.78539816f * 0.5f);
        const float vx = ndcX * aspect * tanHalfFov;
        const float vy = ndcY * tanHalfFov;
        rdx = fx + sx * vx + ux * vy;
        rdy = fy + uy * vy;
        rdz = fz + sz * vx + uz * vy;
    } else {
        float halfH = 520.0f / zoomClamped;
        float halfW = halfH * aspect;
        float camRightOffset = ndcX * halfW;
        float camUpOffset    = ndcY * halfH;
        rx0 = mCamTargetX + sx * camRightOffset + ux * camUpOffset;
        ry0 = mCamTargetY + uy * camUpOffset;
        rz0 = mCamTargetZ + sz * camRightOffset + uz * camUpOffset;
    }

    if (std::abs(rdy) < 1e-4f) return false;
    float t = (mCamTargetY - ry0) / rdy;
    if (t < 0.0f && mPerspectiveMode) return false;
    float wx = rx0 + t * rdx;
    float wz = rz0 + t * rdz;

    int tx = static_cast<int>( std::floor(wx / 16.0f) );
    int tz = static_cast<int>( std::floor(wz / 16.0f) );
    if (tx < 0 || tx >= city.CellCountX() || tz < 0 || tz >= city.CellCountZ()) return false;
    outTileX = tx;
    outTileZ = tz;
    return true;
}

namespace {

constexpr Uint32 kTypePedestrianSprite = 0x29a5d1ecu;
constexpr float kZoom5PixelsPerMetre = 7.0f; // Zoom-5 sprite UVs span 7 texels per metre
constexpr int kDirX[4] = {1, 0, -1, 0};
constexpr int kDirZ[4] = {0, 1, 0, -1};

bool IsDrivable(NetworkType type) noexcept {
    switch (type) {
        case NetworkType::Road:
        case NetworkType::Street:
        case NetworkType::Avenue:
        case NetworkType::Highway:
        case NetworkType::OneWayRoad:
        case NetworkType::DirtRoad:
            return true;
        default:
            return false;
    }
}

Uint32 NextRandom(Uint32& state) noexcept {
    state ^= state << 13;
    state ^= state >> 17;
    state ^= state << 5;
    return state;
}

Uint32 ReadLE32(const std::vector<Uint8>& bytes, std::size_t offset) noexcept {
    if (offset + 4 > bytes.size()) return 0;
    return static_cast<Uint32>(bytes[offset]) | (static_cast<Uint32>(bytes[offset + 1]) << 8) |
           (static_cast<Uint32>(bytes[offset + 2]) << 16) | (static_cast<Uint32>(bytes[offset + 3]) << 24);
}

} // namespace

void cSC4GLCityRenderer::RebuildDriveCells(const cSC4City& city) {
    mDriveCellsX = std::max(0, city.CellCountX());
    mDriveCellsZ = std::max(0, city.CellCountZ());
    const std::size_t cells = static_cast<std::size_t>(mDriveCellsX) * static_cast<std::size_t>(mDriveCellsZ);
    mDriveNet.assign(cells, -1);
    mDriveHeights.assign(cells, {});
    mDriveType.assign(cells, NetworkType::Road);
    const auto& nets = city.Networks();
    for (std::size_t ni = 0; ni < nets.size(); ++ni) {
        const SC4NetworkRecord& net = nets[ni];
        NetworkType type = net.networkType;
        if (!IsDrivable(type)) {
            bool found = false;
            for (unsigned bit : {0u, 3u, 6u, 2u, 10u, 11u}) {
                if ((net.networkTypeMask & (1u << bit)) != 0u) {
                    type = static_cast<NetworkType>(bit);
                    found = true;
                    break;
                }
            }
            if (!found) continue;
        }
        if (!std::isfinite(net.centerX) || !std::isfinite(net.centerZ)) continue;
        const int cx = static_cast<int>(std::floor(net.centerX / 16.0f));
        const int cz = static_cast<int>(std::floor(net.centerZ / 16.0f));
        if (cx < 0 || cz < 0 || cx >= mDriveCellsX || cz >= mDriveCellsZ) continue;
        const std::size_t idx = static_cast<std::size_t>(cz) * mDriveCellsX + cx;
        if (mDriveNet[idx] >= 0) continue;
        mDriveNet[idx] = static_cast<Sint32>(ni);
        mDriveType[idx] = type;
        std::array<float, 4> heights{};
        heights.fill(net.centerY);
        const float midX = static_cast<float>(cx) * 16.0f + 8.0f;
        const float midZ = static_cast<float>(cz) * 16.0f + 8.0f;
        for (const SC4NetworkVertex& v : net.verts) {
            if (!std::isfinite(v.y)) continue;
            heights[(v.x > midX ? 1u : 0u) + (v.z > midZ ? 2u : 0u)] = v.y;
        }
        mDriveHeights[idx] = heights;
    }
    mDriveCellsDirty = false;
}

bool cSC4GLCityRenderer::DriveCell(int cx, int cz) const noexcept {
    if (cx < 0 || cz < 0 || cx >= mDriveCellsX || cz >= mDriveCellsZ) return false;
    return mDriveNet[static_cast<std::size_t>(cz) * mDriveCellsX + cx] >= 0;
}

Uint8 cSC4GLCityRenderer::ChooseExit(AutomatonAgent& agent, int cx, int cz, Uint8 dirIn) const noexcept {
    const Uint8 back = static_cast<Uint8>((dirIn + 2u) & 3u);
    Uint8 options[3] = {};
    int count = 0;
    bool straight = false;
    for (Uint8 d = 0; d < 4; ++d) {
        if (d == back || !DriveCell(cx + kDirX[d], cz + kDirZ[d])) continue;
        if (d == dirIn) straight = true;
        options[count++] = d;
    }
    if (count == 0) return back;
    const Uint32 roll = NextRandom(agent.rng);
    if (straight && (count == 1 || roll % 100u < (agent.pedestrian ? 80u : 60u))) return dirIn;
    return options[(roll >> 8) % static_cast<Uint32>(count)];
}

// Lanes run parallel to the travel direction at a fixed offset to its right;
// turns follow a quadratic curve through the corner where the two lanes cross.
void cSC4GLCityRenderer::AgentPose(const AutomatonAgent& agent, float progress,
                                   float& x, float& y, float& z) const noexcept {
    const float midX = static_cast<float>(agent.cellX) * 16.0f + 8.0f;
    const float midZ = static_cast<float>(agent.cellZ) * 16.0f + 8.0f;
    const float lat = agent.lateral;
    auto lanePoint = [&](Uint8 d, float along, float& px, float& pz) {
        px = midX + kDirX[d] * along - kDirZ[d] * lat;
        pz = midZ + kDirZ[d] * along + kDirX[d] * lat;
    };
    const float r = agent.pedestrian ? 7.0f : 5.0f;
    const Uint8 dIn = agent.dirIn;
    const Uint8 dOut = agent.dirOut;
    if (dIn == dOut || progress <= 8.0f - r) {
        lanePoint(dIn, progress - 8.0f, x, z);
    } else if (progress >= 8.0f + r) {
        lanePoint(dOut, progress - 8.0f, x, z);
    } else {
        float p0x = 0.0f, p0z = 0.0f, p2x = 0.0f, p2z = 0.0f;
        lanePoint(dIn, -r, p0x, p0z);
        lanePoint(dOut, r, p2x, p2z);
        float cx = 0.0f, cz = 0.0f;
        if (dOut == ((dIn + 2u) & 3u)) {
            cx = midX + kDirX[dIn] * r;
            cz = midZ + kDirZ[dIn] * r;
        } else {
            cx = midX - kDirZ[dIn] * lat - kDirZ[dOut] * lat;
            cz = midZ + kDirX[dIn] * lat + kDirX[dOut] * lat;
        }
        const float t = (progress - (8.0f - r)) / (2.0f * r);
        const float it = 1.0f - t;
        x = it * it * p0x + 2.0f * it * t * cx + t * t * p2x;
        z = it * it * p0z + 2.0f * it * t * cz + t * t * p2z;
    }

    y = 0.0f;
    if (!DriveCell(agent.cellX, agent.cellZ)) return;
    const auto& h = mDriveHeights[static_cast<std::size_t>(agent.cellZ) * mDriveCellsX + agent.cellX];
    const float fx = std::clamp((x - (midX - 8.0f)) / 16.0f, 0.0f, 1.0f);
    const float fz = std::clamp((z - (midZ - 8.0f)) / 16.0f, 0.0f, 1.0f);
    y = (1.0f - fz) * ((1.0f - fx) * h[0] + fx * h[1]) + fz * ((1.0f - fx) * h[2] + fx * h[3]);
}

float cSC4GLCityRenderer::DriveSpeed(const AutomatonAgent& agent) const noexcept {
    if (agent.pedestrian) return 1.4f;
    if (!DriveCell(agent.cellX, agent.cellZ)) return 0.0f;
    switch (mDriveType[static_cast<std::size_t>(agent.cellZ) * mDriveCellsX + agent.cellX]) {
        case NetworkType::Street:   return 7.0f;
        case NetworkType::DirtRoad: return 6.0f;
        case NetworkType::Avenue:   return 12.0f;
        case NetworkType::Highway:  return 18.0f;
        default:                    return 10.0f;
    }
}

void cSC4GLCityRenderer::RebuildAgents(const cSC4City& city) {
    mAgents.clear();
    const auto& saved = city.Automata();
    mAgentSource = saved.data();
    mAgentSourceCount = saved.size();
    for (std::size_t i = 0; i < saved.size(); ++i) {
        const SC4AutomatonRecord& rec = saved[i];
        AutomatonAgent agent;
        agent.pedestrian = rec.kind == SC4AutomatonRecord::Kind::Pedestrian;
        agent.model = rec.model;
        agent.rng = static_cast<Uint32>(i) * 0x9E3779B1u + 0x6D2B79F5u;
        int cx = static_cast<int>(std::floor(rec.x / 16.0f));
        int cz = static_cast<int>(std::floor(rec.z / 16.0f));
        bool snapped = false;
        if (!DriveCell(cx, cz)) {
            // Sims cutting across a lot and cars in a driveway join the nearest street.
            int best = -1;
            float bestDist = 1e9f;
            for (int d = 0; d < 4; ++d) {
                const int nx = cx + kDirX[d];
                const int nz = cz + kDirZ[d];
                if (!DriveCell(nx, nz)) continue;
                const float dx = rec.x - (static_cast<float>(nx) * 16.0f + 8.0f);
                const float dz = rec.z - (static_cast<float>(nz) * 16.0f + 8.0f);
                const float dist = dx * dx + dz * dz;
                if (dist < bestDist) {
                    bestDist = dist;
                    best = d;
                }
            }
            if (best < 0) continue;
            cx += kDirX[best];
            cz += kDirZ[best];
            snapped = true;
        }
        Uint8 dir = std::fabs(rec.headingX) >= std::fabs(rec.headingZ)
            ? static_cast<Uint8>(rec.headingX > 0.0f ? 0u : 2u)
            : static_cast<Uint8>(rec.headingZ > 0.0f ? 1u : 3u);
        // A sim heading across the street (to or from a lot) walks along it instead.
        auto axisOpen = [&](Uint8 d) {
            return DriveCell(cx + kDirX[d], cz + kDirZ[d]) || DriveCell(cx - kDirX[d], cz - kDirZ[d]);
        };
        const Uint8 turned = static_cast<Uint8>((dir + 1u) & 3u);
        if (!axisOpen(dir) && axisOpen(turned)) {
            const float along = rec.headingX * kDirX[turned] + rec.headingZ * kDirZ[turned];
            dir = along >= 0.0f ? turned : static_cast<Uint8>((turned + 2u) & 3u);
        }
        const float relX = rec.x - (static_cast<float>(cx) * 16.0f + 8.0f);
        const float relZ = rec.z - (static_cast<float>(cz) * 16.0f + 8.0f);
        agent.cellX = cx;
        agent.cellZ = cz;
        agent.dirIn = dir;
        agent.progress = std::clamp(relX * kDirX[dir] + relZ * kDirZ[dir] + 8.0f, 0.0f, 15.9f);
        agent.lateral = std::clamp(-relX * kDirZ[dir] + relZ * kDirX[dir], -7.5f, 7.5f);
        if (agent.pedestrian && (snapped || std::fabs(agent.lateral) < 4.5f)) {
            agent.lateral = agent.lateral < 0.0f ? -6.0f : 6.0f;
        } else if (!agent.pedestrian && (snapped || std::fabs(agent.lateral) < 1.0f)) {
            agent.lateral = 2.5f;
        }
        agent.dirOut = DriveCell(cx + kDirX[dir], cz + kDirZ[dir]) ? dir : ChooseExit(agent, cx, cz, dir);
        if (agent.dirOut != agent.dirIn) {
            agent.progress = std::min(agent.progress, 8.0f - (agent.pedestrian ? 7.0f : 5.0f));
        }
        agent.walked = static_cast<float>(NextRandom(agent.rng) % 1000u) * 0.01f;
        mAgents.push_back(agent);
    }
}

void cSC4GLCityRenderer::StepAutomata(const cSC4City& city, float dtSeconds) {
    if (mDriveCellsDirty || mDriveCellsX != city.CellCountX() || mDriveCellsZ != city.CellCountZ()) {
        RebuildDriveCells(city);
    }
    if (city.Automata().data() != mAgentSource || city.Automata().size() != mAgentSourceCount) {
        RebuildAgents(city);
    }
    const float dt = std::clamp(dtSeconds, 0.0f, 0.25f);
    for (AutomatonAgent& agent : mAgents) {
        if (!agent.active) continue;
        if (!DriveCell(agent.cellX, agent.cellZ)) {
            agent.active = false;
            continue;
        }
        const float step = DriveSpeed(agent) * dt;
        agent.progress += step;
        agent.walked += step;
        for (int guard = 0; agent.progress >= 16.0f && guard < 4; ++guard) {
            agent.progress -= 16.0f;
            agent.cellX += kDirX[agent.dirOut];
            agent.cellZ += kDirZ[agent.dirOut];
            agent.dirIn = agent.dirOut;
            if (!DriveCell(agent.cellX, agent.cellZ)) {
                agent.active = false;
                break;
            }
            agent.dirOut = ChooseExit(agent, agent.cellX, agent.cellZ, agent.dirIn);
        }
    }
}

std::size_t cSC4GLCityRenderer::ActiveVehicleCount() const noexcept {
    return static_cast<std::size_t>(std::count_if(mAgents.begin(), mAgents.end(),
        [](const AutomatonAgent& a) { return a.active && !a.pedestrian; }));
}

std::size_t cSC4GLCityRenderer::ActivePedestrianCount() const noexcept {
    return static_cast<std::size_t>(std::count_if(mAgents.begin(), mAgents.end(),
        [](const AutomatonAgent& a) { return a.active && a.pedestrian; }));
}

TexturedMeshBatch* cSC4GLCityRenderer::AutomataBatch(const cSC4TGI& fsh, bool instanceOnly) {
    constexpr std::size_t kMissing = static_cast<std::size_t>(-1);
    const Uint64 key = (static_cast<Uint64>(instanceOnly ? 0u : fsh.group) << 32) | fsh.instance;
    if (auto found = mAutomataBatchIndex.find(key); found != mAutomataBatchIndex.end()) {
        return found->second == kMissing ? nullptr : &mTexturedBatches[found->second];
    }
    std::vector<Uint8> bytes;
    cFSHImage image;
    const bool loaded = mDbpfSys != nullptr &&
        (instanceOnly ? mDbpfSys->LoadAssetByTypeAndInstance(kTypeFSH, fsh.instance, bytes)
                      : mDbpfSys->LoadAssetByTGI(cSC4TGI{kTypeFSH, fsh.group, fsh.instance}, bytes)) &&
        image.Parse(bytes.data(), bytes.size());
    const std::size_t need = static_cast<std::size_t>(image.width) * static_cast<std::size_t>(image.height) * 4u;
    if (!loaded || image.width == 0 || image.height == 0 || image.rgbaPixels.size() < need) {
        mAutomataBatchIndex.emplace(key, kMissing);
        return nullptr;
    }
    TexturedMeshBatch created;
    created.fshInstanceId = fsh.instance;
    created.texWidth = image.width;
    created.texHeight = image.height;
    created.rgbaPixels = std::move(image.rgbaPixels);
    created.isAutomata = true;
    created.verticesDirty = true;
    mAutomataBatchIndex.emplace(key, mTexturedBatches.size());
    mTexturedBatches.push_back(std::move(created));
    return &mTexturedBatches.back();
}

const cS3DModel* cSC4GLCityRenderer::AutomataModel(const cSC4TGI& key) {
    auto cached = mAutomataModels.find(key.instance);
    if (cached == mAutomataModels.end()) {
        cS3DModel model;
        std::vector<Uint8> bytes;
        if (mDbpfSys != nullptr && mDbpfSys->LoadAssetByTGI(cSC4TGI{kTypeS3D, key.group, key.instance}, bytes)) {
            model.Parse(bytes.data(), bytes.size());
        }
        cached = mAutomataModels.emplace(key.instance, std::move(model)).first;
    }
    const cS3DModel& model = cached->second;
    return model.vertices.empty() || model.meshes.empty() ? nullptr : &model;
}

// Type 0x29a5d1ec names the sprite atlas (FSH) and, per zoom 1..5, a frame
// table (type 0x09adcd75). Each table entry is a direction's first frame; the
// direction's other frames follow it left to right, wrapping at the atlas edge.
const cSC4GLCityRenderer::PedestrianSprite* cSC4GLCityRenderer::PedestrianSpriteFor(const cSC4TGI& key) {
    if (auto found = mPedestrianSprites.find(key.instance); found != mPedestrianSprites.end()) return &found->second;
    if (mDbpfSys == nullptr || mMissingPedestrianSprites.count(key.instance) != 0) return nullptr;
    auto fail = [&]() -> const PedestrianSprite* {
        mMissingPedestrianSprites.insert(key.instance);
        return nullptr;
    };
    std::vector<Uint8> desc;
    if (key.type != kTypePedestrianSprite || !mDbpfSys->LoadAssetByTGI(key, desc) || desc.size() < 48 ||
        ReadLE32(desc, 4) != kTypeFSH) {
        return fail();
    }
    const cSC4TGI atlas{kTypeFSH, ReadLE32(desc, 8), ReadLE32(desc, 12)};
    const Uint32 tableType = ReadLE32(desc, 16);
    const Uint32 tableGroup = ReadLE32(desc, 20);
    const int framesPerDirection = static_cast<int>(ReadLE32(desc, 44));
    if (framesPerDirection <= 0 || framesPerDirection > 64) return fail();

    PedestrianSprite sprite;
    sprite.framesPerDirection = framesPerDirection;
    std::vector<Uint8> table;
    const Uint32 zoom5 = ReadLE32(desc, 40);
    const Uint32 zoom4 = ReadLE32(desc, 36);
    if (zoom5 != 0 && mDbpfSys->LoadAssetByTGI(cSC4TGI{tableType, tableGroup, zoom5}, table)) {
        sprite.metresPerPixel = 1.0f / kZoom5PixelsPerMetre;
    } else if (zoom4 != 0 && mDbpfSys->LoadAssetByTGI(cSC4TGI{tableType, tableGroup, zoom4}, table)) {
        sprite.metresPerPixel = 2.0f / kZoom5PixelsPerMetre;
    } else {
        return fail();
    }
    const Uint32 directions = ReadLE32(table, 32);
    if (directions == 0 || directions > 16 || table.size() < 36 + 8u * directions) return fail();

    TexturedMeshBatch* batch = AutomataBatch(atlas, false);
    if (batch == nullptr) return fail();
    const int atlasW = batch->texWidth;
    const int atlasH = batch->texHeight;
    sprite.batch = static_cast<std::size_t>(batch - mTexturedBatches.data());
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
    return &mPedestrianSprites.emplace(key.instance, std::move(sprite)).first->second;
}

void cSC4GLCityRenderer::AppendAutomataGeometry(int viewportH) {
    for (const auto& [key, idx] : mAutomataBatchIndex) {
        if (idx >= mTexturedBatches.size()) continue;
        mTexturedBatches[idx].vertices.clear();
        mTexturedBatches[idx].verticesDirty = true;
    }
    if (mDbpfSys == nullptr || mAgents.empty()) return;

    const float dirX = std::cos(mPitchRadians) * std::cos(mYawRadians);
    const float dirY = std::sin(mPitchRadians);
    const float dirZ = std::cos(mPitchRadians) * std::sin(mYawRadians);
    const float horiz = std::max(1e-5f, std::sqrt(dirX * dirX + dirZ * dirZ));
    const float sx = dirZ / horiz, sz = -dirX / horiz;    // Screen right
    const float ux = sz * dirY, uy = sx * dirZ - sz * dirX, uz = -sx * dirY; // Screen up
    const float awayX = -dirX / horiz, awayZ = -dirZ / horiz;
    const float pixelsPerMetre = static_cast<float>(viewportH) * std::max(0.18f, mZoom) / 1040.0f;
    const bool showSims = pixelsPerMetre >= 1.5f;

    // Screen-space heading sectors (right, up-right, ..., down-right) to the
    // sprite set's directions: 0 is a left-facing profile, 1 three-quarter
    // back, 5 back, 2 front and 4 three-quarter front.
    struct SpriteDir { Uint8 dir; bool mirror; };
    static constexpr SpriteDir kSectors[8] = {
        {0, true}, {1, true}, {5, false}, {1, false}, {0, false}, {4, false}, {2, false}, {4, true}};

    for (const AutomatonAgent& agent : mAgents) {
        if (!agent.active || (agent.pedestrian && !showSims)) continue;
        float x = 0.0f, y = 0.0f, z = 0.0f;
        AgentPose(agent, agent.progress, x, y, z);
        float ax = 0.0f, ay = 0.0f, az = 0.0f, bx = 0.0f, by = 0.0f, bz = 0.0f;
        AgentPose(agent, std::max(0.0f, agent.progress - 0.5f), ax, ay, az);
        AgentPose(agent, std::min(15.99f, agent.progress + 0.5f), bx, by, bz);
        float hx = bx - ax, hz = bz - az;
        const float hl = std::sqrt(hx * hx + hz * hz);
        if (hl > 1e-4f) {
            hx /= hl;
            hz /= hl;
        } else {
            hx = static_cast<float>(kDirX[agent.dirIn]);
            hz = static_cast<float>(kDirZ[agent.dirIn]);
        }

        if (agent.model.type == kTypeS3D) {
            const cS3DModel* model = AutomataModel(agent.model);
            if (model == nullptr) continue;
            float minX = model->vertices.front().x, maxX = minX;
            float minZ = model->vertices.front().z, maxZ = minZ;
            float minY = model->vertices.front().y;
            for (const S3DVertex& v : model->vertices) {
                minX = std::min(minX, v.x);
                maxX = std::max(maxX, v.x);
                minZ = std::min(minZ, v.z);
                maxZ = std::max(maxZ, v.z);
                minY = std::min(minY, v.y);
            }
            const float midX = 0.5f * (minX + maxX);
            const float midZ = 0.5f * (minZ + maxZ);
            // Automata models face +Z.
            for (const S3DMesh& mesh : model->meshes) {
                if (mesh.materialIndex >= model->materials.size()) continue;
                const Uint32 fshId = model->materials[mesh.materialIndex].textureId;
                if (fshId == 0) continue;
                TexturedMeshBatch* batch = AutomataBatch(cSC4TGI{kTypeFSH, 0u, fshId}, true);
                if (batch == nullptr) continue;
                const std::size_t end = std::min<std::size_t>(model->indices.size(),
                    static_cast<std::size_t>(mesh.firstIndex) + mesh.indexCount);
                for (std::size_t i = mesh.firstIndex; i < end; ++i) {
                    const Uint32 vi = model->indices[i];
                    if (vi >= model->vertices.size()) continue;
                    const S3DVertex& v = model->vertices[vi];
                    const float lx = v.x - midX;
                    const float lz = v.z - midZ;
                    float r = 1.0f, g = 1.0f, b = 1.0f, a = 1.0f;
                    UnpackS3DColor(v.color, r, g, b, a);
                    AppendTexturedVertex(batch->vertices,
                                         x + lx * hz + lz * hx, y + (v.y - minY) + 0.22f, z - lx * hx + lz * hz,
                                         v.u, v.v, r, g, b, a);
                }
            }
            continue;
        }

        const PedestrianSprite* sprite = PedestrianSpriteFor(agent.model);
        if (sprite == nullptr || sprite->batch >= mTexturedBatches.size()) continue;
        const int fpd = sprite->framesPerDirection;
        const int directions = static_cast<int>(sprite->frames.size()) / fpd;
        const float angle = std::atan2(hx * awayX + hz * awayZ, hx * sx + hz * sz);
        const int sector = static_cast<int>(std::lround(angle / 0.78539816f)) & 7;
        const SpriteDir pick = kSectors[sector];
        if (pick.dir >= directions) continue;
        const int frame = static_cast<int>(agent.walked / 1.6f * static_cast<float>(fpd)) % fpd;
        const SpriteFrame& f = sprite->frames[static_cast<std::size_t>(pick.dir) * fpd + frame];
        TexturedMeshBatch& batch = mTexturedBatches[sprite->batch];
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
        // Pulled toward the camera so the feet are not cut by the road surface.
        const float baseX = x + dirX * 2.0f, baseY = y + 0.22f + dirY * 2.0f, baseZ = z + dirZ * 2.0f;
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
    }
}

void cSC4GLCityRenderer::RenderFrame(int viewportWidth, int viewportHeight, const cSC4City& city) {
    bool rebuiltGeometry = false;
    if (!mGLReady) {
        InitGL();
        RebuildCityGeometry(city, mOverlay);
        rebuiltGeometry = true;
    } else if (mGeometryDirty) {
        RebuildCityGeometry(city, mOverlay);
        rebuiltGeometry = true;
    }
    const double now = std::chrono::duration<double>(std::chrono::steady_clock::now().time_since_epoch()).count();
    StepAutomata(city, mAutomataClock < 0.0 ? 0.0f : static_cast<float>(now - mAutomataClock));
    mAutomataClock = now;
    AppendAutomataGeometry(viewportHeight);
    if (mGLReady) SyncGpuResources();
    if (mGLReady && (mHoverDirty || rebuiltGeometry)) {
        UploadHoverCursor(city);
        mHoverDirty = false;
    }

    glViewport(0, 0, viewportWidth, viewportHeight);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Classic SimCity 4 deep slate-blue void background
    glClearColor(0.11f, 0.16f, 0.24f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    bool anyTextured = false;
    for (const auto& batch : mTexturedBatches) {
        if (batch.glTex != 0 && batch.vao != 0 && batch.gpuVertexCount != 0) {
            anyTextured = true;
            break;
        }
    }
    const bool drawCursor = mCursorVertexCount > 0 && mCursorVao != 0 &&
                            mHoverTileX >= 0 && mHoverTileZ >= 0;
    if (mVertexCount == 0 && !anyTextured && !drawCursor) return;

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
            if (batch.glTex == 0 || batch.vao == 0 || batch.gpuVertexCount == 0) continue;
            glBindTexture(GL_TEXTURE_2D, batch.glTex);
            glBindVertexArray(batch.vao);
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(batch.gpuVertexCount));
        }
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);
    };

    // Terrain goes first so the translucent water in the city mesh blends over it.
    // Blend layers repeat the opaque layer's vertices, so they need LEQUAL to land.
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

    if (drawCursor) {
        glUseProgram(mShaderProg);
        GLint loc = glGetUniformLocation(mShaderProg, "uViewProj");
        glUniformMatrix4fv(loc, 1, GL_FALSE, vp);
        glBindVertexArray(mCursorVao);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(mCursorVertexCount));
        glBindVertexArray(0);
    }

    drawTexturedBatches(false, false);
}

} // namespace opensc4
