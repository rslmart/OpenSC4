#include "opensc4/city.h"
#include "opensc4/gl_city_renderer.h"

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#include <emscripten/html5.h>
#define OPENSC4_WASM_EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define OPENSC4_WASM_EXPORT
#endif

namespace {

opensc4::cSC4City gCity;
opensc4::cSC4GLCityRenderer gRenderer;
opensc4::ToolType gCurrentTool = opensc4::ToolType::Inspect;

#if defined(__EMSCRIPTEN__)
int gViewportW = 1280;
int gViewportH = 800;

void MainTick() {
    int width = 0;
    int height = 0;
    emscripten_get_canvas_element_size("#canvas", &width, &height);
    if (width > 0 && height > 0) {
        gViewportW = width;
        gViewportH = height;
    }
    gRenderer.RenderFrame(gViewportW, gViewportH, gCity);
}
#endif

} // namespace

extern "C" {

OPENSC4_WASM_EXPORT bool opensc4_wasm_load_sc4(const uint8_t* data, int size) {
    if (!data || size <= 0) return false;
    std::vector<opensc4::Uint8> buf(data, data + size);
    opensc4::cDBPFArchive arc;
    if (!arc.OpenFromMemory(std::move(buf), "dropped.sc4")) return false;
    if (!gCity.LoadFromSC4Archive(arc)) return false;
    gRenderer.ResetCamera(gCity);
    gRenderer.RebuildCityGeometry(gCity, gRenderer.CurrentOverlay());
    return true;
}

OPENSC4_WASM_EXPORT void opensc4_wasm_step_sim(int days) {
    gCity.StepSimulation(days);
    gRenderer.RebuildCityGeometry(gCity, gRenderer.CurrentOverlay());
}

OPENSC4_WASM_EXPORT void opensc4_wasm_set_tool(int toolId) {
    gCurrentTool = static_cast<opensc4::ToolType>(toolId);
}

OPENSC4_WASM_EXPORT void opensc4_wasm_set_overlay(int overlayId) {
    gRenderer.SetOverlay(static_cast<opensc4::DataOverlay>(overlayId), gCity);
}

OPENSC4_WASM_EXPORT int opensc4_wasm_get_funds() {
    const opensc4::Sint64 funds = gCity.TreasuryFunds();
    constexpr opensc4::Sint64 kMax = 2147483647;
    constexpr opensc4::Sint64 kMin = -2147483647 - 1;
    if (funds > kMax) return static_cast<int>(kMax);
    if (funds < kMin) return static_cast<int>(kMin);
    return static_cast<int>(funds);
}

OPENSC4_WASM_EXPORT int opensc4_wasm_get_res_pop() {
    const opensc4::Uint32 pop = gCity.ResidentialPop();
    return pop > 2147483647u ? 2147483647 : static_cast<int>(pop);
}

OPENSC4_WASM_EXPORT int opensc4_wasm_get_com_pop() {
    const opensc4::Uint32 pop = gCity.CommercialPop();
    return pop > 2147483647u ? 2147483647 : static_cast<int>(pop);
}

OPENSC4_WASM_EXPORT int opensc4_wasm_get_ind_pop() {
    const opensc4::Uint32 pop = gCity.IndustrialPop();
    return pop > 2147483647u ? 2147483647 : static_cast<int>(pop);
}

OPENSC4_WASM_EXPORT int opensc4_wasm_get_year() {
    return static_cast<int>(gCity.SimYear());
}

OPENSC4_WASM_EXPORT int opensc4_wasm_get_month() {
    return static_cast<int>(gCity.SimMonth());
}

OPENSC4_WASM_EXPORT int opensc4_wasm_get_day() {
    return static_cast<int>(gCity.SimDay());
}

OPENSC4_WASM_EXPORT bool opensc4_wasm_apply_tool(int toolId, int x0, int z0, int x1, int z1) {
    const auto tool = static_cast<opensc4::ToolType>(static_cast<opensc4::Uint8>(toolId));
    const bool applied = gCity.ApplyTool(tool, x0, z0, x1, z1);
    if (applied) gRenderer.RebuildCityGeometry(gCity, gRenderer.CurrentOverlay());
    return applied;
}

OPENSC4_WASM_EXPORT void opensc4_wasm_pan(float dx, float dz) {
    gRenderer.Pan(dx, dz);
}

OPENSC4_WASM_EXPORT void opensc4_wasm_zoom(float factor) {
    gRenderer.ZoomBy(factor);
}

OPENSC4_WASM_EXPORT void opensc4_wasm_rotate(int dir) {
    gRenderer.Rotate90(dir);
}

OPENSC4_WASM_EXPORT bool opensc4_wasm_set_tax_rate(int purpose, int wealth, float rate) {
    return gCity.SetTaxRate(purpose, wealth, rate);
}

OPENSC4_WASM_EXPORT float opensc4_wasm_get_tax_rate(int purpose, int wealth) {
    return gCity.GetTaxRate(purpose, wealth);
}

OPENSC4_WASM_EXPORT bool opensc4_wasm_issue_bond(int amount) {
    return gCity.IssueBond(amount);
}

OPENSC4_WASM_EXPORT int opensc4_wasm_get_bond_payments() {
    return static_cast<int>(gCity.GetTotalMonthlyBondPayments());
}

OPENSC4_WASM_EXPORT int opensc4_wasm_query_tile_mayor_rating(int cx, int cz) {
    return gCity.QueryTile(cx, cz).mayorRating;
}

OPENSC4_WASM_EXPORT void opensc4_wasm_orbit_free(float deltaYawRad, float deltaPitchRad) {
    gRenderer.OrbitFree(deltaYawRad, deltaPitchRad);
}

OPENSC4_WASM_EXPORT void opensc4_wasm_set_free_angles(float yawRad, float pitchRad) {
    gRenderer.SetFreeCameraAngles(yawRad, pitchRad);
}

OPENSC4_WASM_EXPORT void opensc4_wasm_set_perspective(bool perspective) {
    gRenderer.SetPerspectiveMode(perspective);
}

} // extern "C"

int main() {
    gCity.GenerateProceduralCity(128, 128);
    gRenderer.ResetCamera(gCity);

#if defined(__EMSCRIPTEN__)
    EmscriptenWebGLContextAttributes attrs;
    emscripten_webgl_init_context_attributes(&attrs);
    attrs.majorVersion = 2;
    attrs.minorVersion = 0;
    attrs.depth = 1;
    EMSCRIPTEN_WEBGL_CONTEXT_HANDLE ctx = emscripten_webgl_create_context("#canvas", &attrs);
    if (ctx <= 0) return 1;
    emscripten_webgl_make_context_current(ctx);

    if (!gRenderer.InitGL()) return 1;
    gRenderer.RebuildCityGeometry(gCity);
    emscripten_set_main_loop(MainTick, 0, 1);
#endif
    return 0;
}
