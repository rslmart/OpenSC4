// Headless authentic frame capture vs original SC4.
//   opensc4_authentic_render_snapshot <out_dir> [--city <file.sc4>] [--no-textures]

#define GL_SILENCE_DEPRECATION
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3.h>

#include "opensc4/city.h"
#include "opensc4/formats.h"
#include "opensc4/gl_city_renderer.h"

#include <cstdio>
#include <string>
#include <vector>

using namespace opensc4;

namespace {

bool WritePNG(const std::string& path, int w, int h) {
    std::vector<Uint8> px(static_cast<std::size_t>(w) * h * 4);
    glReadPixels(0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    std::vector<Uint8> flipped(px.size());
    const std::size_t row = static_cast<std::size_t>(w) * 4;
    for (int y = 0; y < h; ++y) {
        std::copy_n(px.data() + static_cast<std::size_t>(h - 1 - y) * row, row,
                    flipped.data() + static_cast<std::size_t>(y) * row);
    }
    for (std::size_t i = 3; i < flipped.size(); i += 4) flipped[i] = 255;
    const auto png = EncodeRGBA8ToPNG(static_cast<Uint16>(w), static_cast<Uint16>(h), flipped.data());
    FILE* f = std::fopen(path.c_str(), "wb");
    if (f == nullptr) return false;
    std::fwrite(png.data(), 1, png.size(), f);
    std::fclose(f);
    std::printf("wrote %s\n", path.c_str());
    return true;
}

} // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::fprintf(stderr, "usage: %s <out_dir> [--city file.sc4] [--no-textures]\n", argv[0]);
        return 2;
    }
    const std::string outDir = argv[1];
    std::string cityPath;
    bool textures = true;
    for (int i = 2; i < argc; ++i) {
        const std::string a = argv[i];
        if (a == "--city" && i + 1 < argc) cityPath = argv[++i];
        else if (a == "--no-textures") textures = false;
    }

    CGLPixelFormatAttribute attrs[] = {
        kCGLPFAOpenGLProfile, static_cast<CGLPixelFormatAttribute>(kCGLOGLPVersion_3_2_Core),
        kCGLPFAColorSize, static_cast<CGLPixelFormatAttribute>(24),
        kCGLPFADepthSize, static_cast<CGLPixelFormatAttribute>(24),
        kCGLPFAAccelerated,
        static_cast<CGLPixelFormatAttribute>(0)};
    CGLPixelFormatObj pix = nullptr;
    GLint npix = 0;
    CGLContextObj ctx = nullptr;
    if (CGLChoosePixelFormat(attrs, &pix, &npix) != kCGLNoError || pix == nullptr ||
        CGLCreateContext(pix, nullptr, &ctx) != kCGLNoError) {
        std::fprintf(stderr, "CGL context creation failed\n");
        return 1;
    }
    CGLSetCurrentContext(ctx);

    constexpr int kW = 1600, kH = 1000;
    GLuint fbo = 0, color = 0, depth = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenRenderbuffers(1, &color);
    glBindRenderbuffer(GL_RENDERBUFFER, color);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, kW, kH);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, color);
    glGenRenderbuffers(1, &depth);
    glBindRenderbuffer(GL_RENDERBUFFER, depth);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, kW, kH);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr, "FBO incomplete\n");
        return 1;
    }

    cDBPFSystem dbpfSys;
    dbpfSys.MountInstall(cDBPFSystem::DefaultSteamSC4AppPath());
    if (cityPath.empty()) {
        for (const auto& rc : dbpfSys.RegionCities()) {
            // Prefer Konradshohe: Big City Tutorial crashes the original Mac build.
            if (rc.fileName.find("Konradshohe") != std::string::npos) {
                cityPath = rc.fullPath;
                break;
            }
        }
        if (cityPath.empty()) {
            for (const auto& rc : dbpfSys.RegionCities()) {
                if (rc.fileName.find("Big City Tutorial") != std::string::npos) {
                    cityPath = rc.fullPath;
                    break;
                }
            }
        }
    }
    cSC4City city;
    if (!city.LoadFromSC4File(cityPath, &dbpfSys)) {
        std::fprintf(stderr, "failed to load %s\n", cityPath.c_str());
        return 1;
    }
    std::size_t floraWithS3D = 0, propsWithS3D = 0, vehicles = 0, peds = 0, pedSprite = 0, pedS3D = 0;
    std::size_t bldWithS3D = 0, bldWithFsh = 0;
    for (const auto& fl : city.Flora()) if (fl.s3dInstanceId != 0) ++floraWithS3D;
    for (const auto& pr : city.Props()) if (pr.s3dInstanceId != 0) ++propsWithS3D;
    for (const auto& b : city.Buildings()) {
        if (b.s3dInstanceId != 0) ++bldWithS3D;
        if (b.fshTextureId != 0) ++bldWithFsh;
    }
    for (const auto& a : city.Automata()) {
        if (a.kind == SC4AutomatonRecord::Kind::Vehicle) ++vehicles;
        else {
            ++peds;
            if (a.model.type == 0x29a5d1ecu) ++pedSprite;
            else if (a.model.type == kTypeS3D) ++pedS3D;
        }
    }
    std::printf("city: path=%s lots=%zu networks=%zu buildings=%zu (s3d=%zu fsh=%zu) flora=%zu (s3d=%zu) props=%zu (s3d=%zu) lotTex=%zu automata=%zu (veh=%zu ped=%zu sprite=%zu s3d=%zu)\n",
                cityPath.c_str(), city.Lots().size(), city.Networks().size(),
                city.Buildings().size(), bldWithS3D, bldWithFsh,
                city.Flora().size(), floraWithS3D,
                city.Props().size(), propsWithS3D, city.LotTextures().size(),
                city.Automata().size(), vehicles, peds, pedSprite, pedS3D);

    cSC4GLCityRenderer renderer;
    if (textures) renderer.AttachDBPFSystem(&dbpfSys);
    renderer.InitGL();
    renderer.ResetCamera(city);
    renderer.RebuildCityGeometry(city, DataOverlay::None);
    std::size_t texVerts = 0;
    for (const auto& b : renderer.TexturedBatches()) texVerts += b.vertices.size();
    std::printf("geometry: untextured=%zu textured=%zu batches=%zu buildingsDrawn=%zu/%zu floraDrawn=%zu pedDrawn=%zu/%zu\n",
                renderer.TotalVertexCount(), texVerts, renderer.TexturedBatches().size(),
                renderer.TexturedBuildingCount(), city.Buildings().size(),
                renderer.TexturedFloraCount(),
                renderer.TexturedPedestrianCount(), city.Automata().size());
    std::size_t undrawnBld = 0, undrawnPed = 0;
    for (std::size_t bi = 0; bi < city.Buildings().size(); ++bi) {
        if (renderer.IsBuildingTextured(bi)) continue;
        const auto& b = city.Buildings()[bi];
        std::printf("undrawn building[%zu] name=%s s3d=0x%08x group=0x%08x fsh=0x%08x ex=0x%08x orient=%u bbox=(%.1f,%.1f)-(%.1f,%.1f)\n",
                    bi, b.name.c_str(), b.s3dInstanceId, b.s3dGroupId, b.fshTextureId, b.exemplarTGI.instance,
                    static_cast<unsigned>(b.orientation), b.minX, b.minZ, b.maxX, b.maxZ);
        std::vector<Uint8> probe;
        const bool s3dOk = (b.s3dGroupId != 0 &&
            dbpfSys.LoadAssetByTGI(cSC4TGI{kTypeS3D, b.s3dGroupId, b.s3dInstanceId}, probe)) ||
            dbpfSys.LoadAssetByTypeAndInstance(kTypeS3D, b.s3dInstanceId, probe);
        std::printf("  s3dLoad=%d bytes=%zu", (int)s3dOk, probe.size());
        if (s3dOk) {
            cS3DModel m;
            if (m.Parse(probe.data(), probe.size())) {
                std::printf(" verts=%zu mats=%zu", m.vertices.size(), m.materials.size());
                for (const auto& mat : m.materials) std::printf(" tex=0x%08x", mat.textureId);
            } else std::printf(" parseFAIL");
        }
        std::vector<Uint8> exb;
        if (dbpfSys.LoadAssetByTGI(b.exemplarTGI, exb) ||
            dbpfSys.LoadAssetByTypeAndInstance(kTypeExemplar, b.exemplarTGI.instance, exb)) {
            cGZExemplar ex;
            if (ex.Parse(exb.data(), exb.size())) {
                for (Uint32 key : {kPropResourceKeyType1, kPropResourceKeyType0,
                                   kPropResourceKeyType0 + 4u, kPropResourceKeyType0 + 5u}) {
                    auto it = ex.properties.find(key);
                    if (it == ex.properties.end()) continue;
                    std::printf("\n  RKT 0x%x:", key);
                    for (auto v : it->second.intValues) std::printf(" %08x", static_cast<Uint32>(v));
                }
            }
        }
        std::printf("\n");
        ++undrawnBld;
    }
    for (std::size_t ai = 0; ai < city.Automata().size(); ++ai) {
        const auto& a = city.Automata()[ai];
        if (a.kind != SC4AutomatonRecord::Kind::Pedestrian) continue;
        if (renderer.IsAutomatonTextured(ai)) continue;
        std::printf("undrawn ped[%zu] model=%08x:%08x:%08x pos=(%.1f,%.1f,%.1f)\n",
                    ai, a.model.type, a.model.group, a.model.instance, a.x, a.y, a.z);
        std::vector<Uint8> desc;
        const bool ok = dbpfSys.LoadAssetByTGI(a.model, desc);
        std::printf("  spriteLoad=%d bytes=%zu\n", (int)ok, desc.size());
        ++undrawnPed;
    }
    std::printf("undrawn: buildings=%zu pedestrians=%zu\n", undrawnBld, undrawnPed);

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    renderer.RenderFrame(kW, kH, city);
    glFinish();
    WritePNG(outDir + "/snap_default.png", kW, kH);

    renderer.ZoomBy(4.0f);
    renderer.SetAnimPhase(0.35f);
    renderer.RebuildCityGeometry(city, DataOverlay::None);
    renderer.RenderFrame(kW, kH, city);
    glFinish();
    WritePNG(outDir + "/snap_close.png", kW, kH);

    for (int q = 1; q <= 3; ++q) {
        renderer.Rotate90(1);
        renderer.RebuildCityGeometry(city, DataOverlay::None);
        renderer.RenderFrame(kW, kH, city);
        glFinish();
        WritePNG(outDir + "/snap_rot" + std::to_string(q * 90) + ".png", kW, kH);
    }

    std::printf("glError=0x%x\n", glGetError());
    renderer.ShutdownGL();
    CGLSetCurrentContext(nullptr);
    CGLDestroyContext(ctx);
    CGLDestroyPixelFormat(pix);
    return 0;
}
