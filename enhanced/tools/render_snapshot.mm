// Headless frame capture: renders a city through cSC4GLCityRenderer into an
// offscreen FBO and writes PNGs, so rendering can be checked without a window.
//
//   opensc4_render_snapshot <out_dir> [--city <file.sc4>] [--no-textures]

#define GL_SILENCE_DEPRECATION
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl3.h>

#include "opensc4/city.h"
#include "opensc4/formats.h"
#include "opensc4/gl_city_renderer.h"

#include <cmath>
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
            if (rc.fileName.find("Big City Tutorial") != std::string::npos) {
                cityPath = rc.fullPath;
                break;
            }
        }
    }
    cSC4City city;
    if (!city.LoadFromSC4File(cityPath, &dbpfSys)) {
        std::fprintf(stderr, "failed to load %s\n", cityPath.c_str());
        return 1;
    }
    std::printf("city: lots=%zu networks=%zu buildings=%zu\n", city.Lots().size(),
                city.Networks().size(), city.Buildings().size());

    cSC4GLCityRenderer renderer;
    if (textures) renderer.AttachDBPFSystem(&dbpfSys);
    renderer.InitGL();
    renderer.ResetCamera(city);
    renderer.RebuildCityGeometry(city, DataOverlay::None);
    std::size_t texVerts = 0;
    for (const auto& b : renderer.TexturedBatches()) texVerts += b.vertices.size();
    std::printf("geometry: untextured=%zu textured=%zu batches=%zu\n", renderer.TotalVertexCount(),
                texVerts, renderer.TexturedBatches().size());

    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    renderer.RenderFrame(kW, kH, city);
    glFinish();
    WritePNG(outDir + "/snap_default.png", kW, kH);

    for (int i = 2; i + 2 < argc; ++i) {
        if (std::string(argv[i]) != "--probe") continue;
        const int px = std::atoi(argv[i + 1]), py = std::atoi(argv[i + 2]);
        int tx = -1, tz = -1;
        renderer.ScreenToTile(px, py, kW, kH, city, tx, tz);
        std::printf("probe (%d,%d) -> tile (%d,%d)\n", px, py, tx, tz);
        for (const auto& n : city.Networks()) {
            if (std::abs(n.tileX - tx) <= 2 && std::abs(n.tileZ - tz) <= 2)
                std::printf("   net type=%d tile=(%d,%d) piece=%08x mask=%x\n", int(n.networkType), n.tileX, n.tileZ, n.textureId, n.networkTypeMask);
        }
        for (const auto& b : city.Buildings()) {
            const int bx = static_cast<int>((b.minX + b.maxX) * 0.5f / 16.0f);
            const int bz = static_cast<int>((b.minZ + b.maxZ) * 0.5f / 16.0f);
            if (std::abs(bx - tx) <= 2 && std::abs(bz - tz) <= 2)
                std::printf("   bld %s tile=(%d,%d) h=%.1f\n", b.name.c_str(), bx, bz, b.maxY - b.minY);
        }
        int flora = 0;
        for (const auto& f : city.Flora()) {
            if (std::abs(static_cast<int>(f.x / 16.0f) - tx) <= 2 && std::abs(static_cast<int>(f.z / 16.0f) - tz) <= 2) ++flora;
        }
        std::printf("   flora within 2 tiles: %d\n", flora);
    }

    renderer.ZoomBy(4.0f);
    renderer.RenderFrame(kW, kH, city);
    glFinish();
    WritePNG(outDir + "/snap_close.png", kW, kH);

    const char* kRotatedNames[3] = {"snap_rot90.png", "snap_rot180.png", "snap_rot270.png"};
    for (int q = 1; q <= 3; ++q) {
        renderer.SetFreeCameraAngles(cSC4GLCityRenderer::kSC4ViewYaw + q * 1.5707963f, cSC4GLCityRenderer::kSC4ViewPitch);
        renderer.RenderFrame(kW, kH, city);
        glFinish();
        WritePNG(outDir + "/" + kRotatedNames[q - 1], kW, kH);
    }

    renderer.SetFreeCameraAngles(cSC4GLCityRenderer::kSC4ViewYaw, 1.45f);
    renderer.RenderFrame(kW, kH, city);
    glFinish();
    WritePNG(outDir + "/snap_topdown.png", kW, kH);

    for (const auto& net : city.Networks()) {
        if (net.networkType != NetworkType::Rail) continue;
        renderer.SetCameraTarget(net.centerX, net.centerY, net.centerZ, 6.0f);
        renderer.SetFreeCameraAngles(cSC4GLCityRenderer::kSC4ViewYaw, cSC4GLCityRenderer::kSC4ViewPitch);
        renderer.RenderFrame(kW, kH, city);
        glFinish();
        WritePNG(outDir + "/snap_rail.png", kW, kH);
        break;
    }

    for (const auto& net : city.Networks()) {
        if (net.textureId != 0x00000a00u || net.tileX != 63 || net.tileZ != 75) continue;
        renderer.SetCameraTarget(net.centerX, net.centerY, net.centerZ, 7.0f);
        renderer.SetFreeCameraAngles(cSC4GLCityRenderer::kSC4ViewYaw, cSC4GLCityRenderer::kSC4ViewPitch);
        renderer.RenderFrame(kW, kH, city);
        glFinish();
        WritePNG(outDir + "/snap_diagonal.png", kW, kH);
        break;
    }

    // Saved cars and sims at SC4's zoom-5 scale (7 px per metre), then 3 s later.
    const float zoom5 = 7.0f * 1040.0f / static_cast<float>(kH);
    renderer.SetFreeCameraAngles(cSC4GLCityRenderer::kSC4ViewYaw, cSC4GLCityRenderer::kSC4ViewPitch);
    for (SC4AutomatonRecord::Kind kind : {SC4AutomatonRecord::Kind::Vehicle, SC4AutomatonRecord::Kind::Pedestrian}) {
        const bool sims = kind == SC4AutomatonRecord::Kind::Pedestrian;
        for (const auto& a : city.Automata()) {
            if (a.kind != kind) continue;
            renderer.SetCameraTarget(a.x, a.y, a.z, zoom5);
            renderer.StepAutomata(city, 0.0f);
            renderer.RenderFrame(kW, kH, city);
            glFinish();
            WritePNG(outDir + (sims ? "/snap_sims.png" : "/snap_traffic.png"), kW, kH);
            for (int i = 0; i < 30; ++i) renderer.StepAutomata(city, 0.1f);
            renderer.RenderFrame(kW, kH, city);
            glFinish();
            WritePNG(outDir + (sims ? "/snap_sims_3s.png" : "/snap_traffic_3s.png"), kW, kH);
            break;
        }
    }
    if (!city.Automata().empty()) {
        const auto& a = city.Automata().front();
        renderer.SetCameraTarget(a.x + 60.0f, a.y, a.z - 40.0f, zoom5 * 0.5f);
        renderer.RenderFrame(kW, kH, city);
        glFinish();
        WritePNG(outDir + "/snap_zoom4.png", kW, kH);
    }
    std::printf("automata: saved=%zu vehicles=%zu sims=%zu active\n", city.Automata().size(),
                renderer.ActiveVehicleCount(), renderer.ActivePedestrianCount());

    // Mirror the desktop app's sim timer: StepSimulation then a full rebuild.
    renderer.ResetCamera(city);
    for (int day = 1; day <= 60; ++day) {
        city.StepSimulation(1);
        renderer.RebuildCityGeometry(city, renderer.CurrentOverlay());
        if (day == 1 || day == 5 || day == 30 || day == 60) {
            std::size_t tv = 0;
            for (const auto& b : renderer.TexturedBatches()) tv += b.vertices.size();
            std::size_t onTerrain = 0;
            for (const auto& n : city.Networks()) {
                const int vx = static_cast<int>(std::lround(n.verts[0].x / 16.0f));
                const int vz = static_cast<int>(std::lround(n.verts[0].z / 16.0f));
                if (std::fabs(n.verts[0].y - city.GetVertexHeight(vx, vz)) < 0.5f) ++onTerrain;
            }
            std::printf("day %d: lots=%zu networks=%zu onTerrain=%zu buildings=%zu texturedVerts=%zu untextured=%zu\n",
                        day, city.Lots().size(), city.Networks().size(), onTerrain,
                        city.Buildings().size(), tv, renderer.TotalVertexCount());
            renderer.RenderFrame(kW, kH, city);
            glFinish();
            WritePNG(outDir + "/snap_day" + std::to_string(day) + ".png", kW, kH);
        }
    }

    // Desktop timer path: rebuild while no GL context is current.
    CGLSetCurrentContext(nullptr);
    city.StepSimulation(1);
    renderer.RebuildCityGeometry(city, renderer.CurrentOverlay());
    CGLSetCurrentContext(ctx);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    renderer.RenderFrame(kW, kH, city);
    glFinish();
    WritePNG(outDir + "/snap_rebuild_no_context.png", kW, kH);

    std::printf("glError=0x%x\n", glGetError());
    renderer.ShutdownGL();
    CGLSetCurrentContext(nullptr);
    CGLDestroyContext(ctx);
    CGLDestroyPixelFormat(pix);
    return 0;
}
