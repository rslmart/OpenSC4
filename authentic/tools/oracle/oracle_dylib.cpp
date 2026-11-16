// OpenSC4 in-process game oracle (Phase 2: read-only).
//
// Injected into the shipped SimCity 4 Deluxe (macOS x86_64) via
// DYLD_INSERT_LIBRARIES. Clean-room tooling - NOT part of the authentic
// recreation; no authenticity rules apply to this file. See
// authentic/recon/dossier_game_plugin_abi.md for the forensics that
// established this path (built-in plugin loader is a stub; codesign
// flags 0x0; x86_64 PIE).
//
// Safety: strictly read-only w.r.t. game state in Phase 2. It reads
// plain data globals (benign from a background thread) and never calls
// game code. Phase 3 (call/hook commands) comes after this is proven.
//
// Channels (all under ~/.opensc4-oracle/):
//   oracle.log   - append-only: startup banner, battery dumps, RPC replies
//   command      - one line per command, consumed + truncated on read:
//                    ping
//                    redump                 (re-run the tunables battery)
//                    read f32|f64|i32|u32 <static-addr-hex>
//                    read mem <static-addr-hex> <n-bytes>
//   Static addresses are the disassembly-time 0x100... addresses; the
//   PIE slide is added by the oracle. The agent's ground-truth commands
//   (--dump) use the same addresses on the on-disk binary.

#include "../../include/opensc4/terrain_altitude_kernel.h"
#include "../../include/opensc4/pollution_grid_kernel.h"
#include "../../include/opensc4/pollution_capacity_kernel.h"
#include <xmmintrin.h>
#include <cstddef>

#include <mach-o/dyld.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <cerrno>
#include <dispatch/dispatch.h>

#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <cstdarg>
#include <string>
#include <vector>

namespace {

constexpr uint64_t kStaticLoadBase = 0x100000000ull;

struct Dump {
    const char* name;
    uint64_t static_addr;
    const char* type;  // f32 | f64 | i32 | u32 | vec32
};

// First experiment battery: the terrain tunables our reconstruction models
// as "settable, shipped-default, TODO exemplar wiring" (dossier addresses).
const Dump kBattery[] = {
    {"kMinSeaLevel",                  0x100b175d4, "f32"},
    {"kMaxSeaLevel",                  0x100b175d8, "f32"},
    {"kSeaRaiseLowerAmount",          0x100b175dc, "f32"},
    {"kMaxTerrainHeight",            0x100b1763c, "f32"},
    {"kMinTerrainHeight",            0x100b17640, "f32"},
    {"kMinShoreCliffSize",            0x100b1766c, "i32"},
    {"kMaxShoreCliffSize",            0x100b17670, "i32"},
    {"kMinShoreCliffCandidateHtSum", 0x100b17674, "f32"},
    {"kMinShoreCliffHt",              0x100b17678, "f32"},
    {"kMaxShoreCliffHt",              0x100b1767c, "f32"},
    {"kMaxNormalYForCliff",          0x100b17680, "f32"},
    {"kTemperatureRangeFactor",       0x100b71db0, "f32"},
    {"kWindErosionFactor",            0x100b71d28, "f32"},
    {"kNumFlowStepsPerWaterErosionBrushApplication", 0x100b71e00, "vec32"},
};

uint64_t g_slide = 0;
int g_log_fd = -1;
char g_log_path[512] = {};
char g_cmd_path[512] = {};
char g_dir_path[512] = {};

void logf_(const char* fmt, ...) __attribute__((format(printf, 1, 2)));
void logfd_reopen();

void logf_(const char* fmt, ...) {
    if (g_log_fd < 0) return;
    std::time_t t = std::time(nullptr);
    std::tm tmv{};
    localtime_r(&t, &tmv);
    char stamp[32];
    strftime(stamp, sizeof(stamp), "%H:%M:%S", &tmv);
    char body[1024];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(body, sizeof(body) - 1, fmt, ap);
    va_end(ap);
    if (n < 0) return;
    if (n > (int)sizeof(body) - 2) n = (int)sizeof(body) - 2;
    char line[1100];
    int m = snprintf(line, sizeof(line), "[%s] %.*s\n", stamp, n, body);
    if (m <= 0) return;
    ssize_t wr = write(g_log_fd, line, (size_t)m);
    if (wr < 0 && (errno == EBADF || errno == EIO)) {
        // The engine may close or exhaust our descriptors; self-heal once.
        logfd_reopen();
        if (g_log_fd >= 0) wr = write(g_log_fd, line, (size_t)m);
    }
    (void)wr;
}

void logfd_reopen() {
    if (g_log_fd >= 0) close(g_log_fd);
    g_log_fd = open(g_log_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
}

bool addr_valid(uint64_t addr) {
    return addr >= kStaticLoadBase && addr - kStaticLoadBase < (1ull << 32);
}

void dump_one(const Dump& d) {
    const uint64_t runtime = d.static_addr + g_slide;
    if (strcmp(d.type, "vec32") == 0) {
        // libc++ std::vector<uint32_t>: {begin, end, cap} pointers.
        const uint64_t* words = reinterpret_cast<const uint64_t*>(runtime);
        uint64_t begin = words[0], end = words[1];
        logf_("BATTERY %s @ 0x%llx begin=0x%llx end=0x%llx count=%lld",
              d.name, (unsigned long long)d.static_addr,
              (unsigned long long)begin, (unsigned long long)end,
              begin || end ? (long long)((end - begin) / 4) : 0);
        if (begin && end >= begin && (end - begin) / 4 <= 64) {
            const uint32_t* els = reinterpret_cast<const uint32_t*>(begin + g_slide);
            std::string els_str;
            for (uint64_t i = 0; i < (end - begin) / 4; ++i) {
                char buf[16];
                snprintf(buf, sizeof(buf), "%s%u", i ? "," : "", els[i]);
                els_str += buf;
            }
            logf_("BATTERY %s elements[%lld]={%s}",
                  d.name, (long long)((end - begin) / 4), els_str.c_str());
        }
        return;
    }
    char line[160];
    if (strcmp(d.type, "f32") == 0) {
        const float v = *reinterpret_cast<const float*>(runtime);
        const uint32_t bits = *reinterpret_cast<const uint32_t*>(runtime);
        snprintf(line, sizeof(line), "value=%g bits=0x%08x", (double)v, bits);
    } else if (strcmp(d.type, "f64") == 0) {
        const double v = *reinterpret_cast<const double*>(runtime);
        snprintf(line, sizeof(line), "value=%g", v);
    } else {
        const uint32_t v = *reinterpret_cast<const uint32_t*>(runtime);
        snprintf(line, sizeof(line), "value=%u (0x%x)", v, v);
    }
    logf_("BATTERY %s @ 0x%llx %s", d.name,
          (unsigned long long)d.static_addr, line);
}

void run_battery(const char* when) {
    logf_("BATTERY-START (%s)", when);
    for (const Dump& d : kBattery) {
        if (addr_valid(d.static_addr)) dump_one(d);
    }
    logf_("BATTERY-END");
}

// --- call command: invoke a game function on the MAIN thread -----------------
// x86_64 SysV integer ABI: up to 6 u64 args, u64 return. Dispatched via
// libdispatch's main queue (no Objective-C blocks; C function + context).
// The main queue is serviced by the app's event loop; if the game stops
// pumping it, calls stall - test with a known function before relying on it.
typedef uint64_t (*fn6_t)(uint64_t, uint64_t, uint64_t, uint64_t, uint64_t, uint64_t);

struct CallCtx {
    uint64_t fn;
    uint64_t args[6];
    int argc;
};

void call_exec(void* p) {
    CallCtx* c = reinterpret_cast<CallCtx*>(p);
    const fn6_t f = reinterpret_cast<fn6_t>(c->fn);
    const uint64_t r = f(c->args[0], c->args[1], c->args[2], c->args[3],
                         c->args[4], c->args[5]);
    logf_("RPC call 0x%llx(%d args) = 0x%llx (%lld)",
          (unsigned long long)(c->fn - g_slide), c->argc,
          (unsigned long long)r, (long long)r);
    free(c);
}

// --- Phase B: reconstructed implementation substitution ----------------------
// cSTETerrain::BrushRadius @ 0x100734d6e — our reconstructed algorithm
// (authentic/src/terrain.cpp, cited "Exact @ 0x100734d6e") transplanted to
// the LIVE layout: the game's sBrushDescriptionMap root at 0x100b71e20
// (map+0x8), node keys at +0x20, radii-vector begin at node+0x38, the
// strength index at this+0xdc, default 32.0f @ 0x100a18764. Read-only.
// Algorithm is verbatim from the disassembly (see commit history): libc++
// lower-bound tree walk with unsigned compares (cmovae/ja), ungated radii
// read — the model in terrain.cpp adds a bounds guard; THIS shim follows
// the binary exactly so the prove-comparison is apples-to-apples.
extern "C" float brush_radius_shim(const void* terrain, uint32_t brush_id) {
    const uint64_t root_ptr_addr = 0x100b71e20 + g_slide;  // map+0x8
    float result = 32.0f;                                   // @ 0x100a18764
    uint64_t node = *reinterpret_cast<const uint64_t*>(root_ptr_addr);
    if (node == 0) return result;                           // je @ 0x100734d84
    uint64_t candidate = root_ptr_addr;                     // end-node identity
    while (true) {                                          // loop @ 0x100734d8d
        const uint32_t key = *reinterpret_cast<const uint32_t*>(node + 0x20);
        uint64_t next = node + 8;                           // default: right
        if (key >= brush_id) {                              // cmovae @ d94
            next = node;                                    // go left
            candidate = node;                               // cmovae rax @ d98
        }
        node = *reinterpret_cast<const uint64_t*>(next);
        if (node == 0) break;                               // jne @ da2
    }
    if (candidate == root_ptr_addr) return result;          // je @ dae
    if (*reinterpret_cast<const uint32_t*>(candidate + 0x20) > brush_id) {
        return result;                                     // ja @ db3
    }
    const int64_t idx = static_cast<int64_t>(
        *reinterpret_cast<const int32_t*>(
            reinterpret_cast<uintptr_t>(terrain) + 0xdc));  // movsxd @ db5
    const uint64_t radii_begin =
        *reinterpret_cast<const uint64_t*>(candidate + 0x38);  // mov rax,[rax+0x38] @ dbc
    return *reinterpret_cast<const float*>(radii_begin + 4ull * idx);
}

static void collect_map_keys(uint64_t node, uint32_t* out, int* count, int max) {
    // In-order walk of the game's sBrushDescriptionMap (left @ +0, right @ +8).
    if (node == 0 || *count >= max) return;
    collect_map_keys(*reinterpret_cast<const uint64_t*>(node), out, count, max);
    if (*count < max) out[(*count)++] =
        *reinterpret_cast<const uint32_t*>(node + 0x20);
    collect_map_keys(*reinterpret_cast<const uint64_t*>(node + 8), out, count, max);
}

// --- Phase B wave 2: four more reconstructed implementations --------------
// All transplanted from authentic/src/terrain.cpp onto the LIVE layout,
// following the binary byte-exactly (same float op order, same NaN paths)
// so the prove-comparisons against the originals are apples-to-apples.
// cSTETerrain vtable @ 0x100aea6b0 (+0x10 slot base):
//   vt+0x060 GetAltitude(float,float)      @ 0x100738928
//   vt+0x110 CellHasWater(uint,uint)       @ 0x10073562c
//   vt+0x118 CellHasLand(uint,uint)        @ 0x1007356a2
//   vt+0x1f0 GetCostPerBrushApplication    @ 0x100738d76
//   vt+0x1e8 BrushRadius                  @ 0x100734d6e (wave 1)

// GetCostPerBrushApplication @ 0x100738d76 — BrushRadius's sibling: same
// sBrushDescriptionMap lower-bound walk, but the per-strength cost floats
// are an INLINE array at node+0x68 (radii were a vector pointer at +0x38),
// read with cvttss2si (truncate toward zero), default 32 (mov eax, 0x20).
extern "C" int64_t get_cost_shim(const void* terrain, uint32_t brush_id) {
    (void)terrain;
    const uint64_t root_ptr_addr = 0x100b71e20 + g_slide;  // map+0x8
    int64_t result = 32;                                    // @ 0x100738db5
    uint64_t node = *reinterpret_cast<const uint64_t*>(root_ptr_addr);
    if (node == 0) return result;
    uint64_t candidate = root_ptr_addr;
    while (true) {
        const uint32_t key = *reinterpret_cast<const uint32_t*>(node + 0x20);
        uint64_t next = node + 8;
        if (key >= brush_id) { next = node; candidate = node; }
        node = *reinterpret_cast<const uint64_t*>(next);
        if (node == 0) break;
    }
    if (candidate == root_ptr_addr) return result;
    if (*reinterpret_cast<const uint32_t*>(candidate + 0x20) > brush_id) {
        return result;  // jbe gate @ 0x100738db3 (inverse of ja)
    }
    const int64_t idx = static_cast<int64_t>(*reinterpret_cast<const int32_t*>(
        reinterpret_cast<uintptr_t>(terrain) + 0xdc));  // movsxd @ 0x100738dbc
    const float cost = *reinterpret_cast<const float*>(
        candidate + 0x68 + 4ull * static_cast<uint64_t>(idx));
    return static_cast<int64_t>(cost);  // cvttss2si rax @ 0x100738dc3
}

// CellHasWater @ 0x10073562c / CellHasLand @ 0x1007356a2 share one helper:
// bounds [t+0x40]/[t+0x44] unsigned jbe; water-depth grid via
// *(*(t+0x38)+0x28); stride W=[t+0x50]; vertex index z*W+x, 36-byte records,
// depth float at +0; four corners (x,z),(x+1,z),(x,z+1),(x+1,z+1).
// Water: any corner depth > 0 (`ja` — ordered only, NaN is NOT water).
// Land:  any corner depth <= 0 (`jae` — unordered path via ZF, NaN IS land).
static bool cell_corner_test(const void* terrain, uint32_t x, uint32_t z,
                             bool for_water) {
    const uint32_t count_x = *reinterpret_cast<const uint32_t*>(
        reinterpret_cast<uintptr_t>(terrain) + 0x40);
    if (count_x <= x) return false;  // jbe @ 0x100735630/0x1007356a6
    const uint32_t count_z = *reinterpret_cast<const uint32_t*>(
        reinterpret_cast<uintptr_t>(terrain) + 0x44);
    if (count_z <= z) return false;  // jbe @ 0x100735635/0x1007356ab
    const uint32_t w = *reinterpret_cast<const uint32_t*>(
        reinterpret_cast<uintptr_t>(terrain) + 0x50);
    const uint64_t water_obj = *reinterpret_cast<const uint64_t*>(
        reinterpret_cast<uintptr_t>(terrain) + 0x38);
    const uint64_t depths =
        *reinterpret_cast<const uint64_t*>(water_obj + 0x28);
    const uint32_t base = z * w + x;  // imul/add @ 0x10073563e/0x1007356b4
    const uint32_t idxs[4] = {base, base + 1, base + w, base + w + 1};
    for (int i = 0; i < 4; ++i) {
        const float d = *reinterpret_cast<const float*>(
            depths + 36ull * idxs[i]);
        if (for_water) {
            if (d > 0.0f) return true;    // ucomiss/ja @ 0x10073565d
        } else {
            if (!(d > 0.0f)) return true;  // ucomiss 0,depth/jae @ 0x1007356ce
        }
    }
    return false;
}

extern "C" uint64_t cell_has_water_shim(const void* terrain, uint32_t x,
                                        uint32_t z) {
    return cell_corner_test(terrain, x, z, true) ? 1 : 0;
}

extern "C" uint64_t cell_has_land_shim(const void* terrain, uint32_t x,
                                       uint32_t z) {
    return cell_corner_test(terrain, x, z, false) ? 1 : 0;
}

// GetAltitude(float,float) @ 0x100738928 — the hottest terrain query.
// Entry gate: self-virtual call vt+0x58 = LocationIsInBounds @ 0x100740e7a
// (pure float compares on bounds [t+0x68]/[t+0x6c], no recursion risk);
// je -> 0.0f. Then the magic-constant truncate transform, unsigned cmova
// clamp, 36-byte vertex records from the altitude array [t+0x88], triangle
// diagonal pick by fracZ vs fracX, min/max blend, and the exact
// (dLo*maxF + dHi*minF)*kInv + v00 op order. NaN fractions: binary picks
// maxF=fracX (cmpnless mask), minF=fracZ (jb taken), diagonal=(sx+1,sz)
// (cmovae not taken) — reproduced below including the unordered paths.
extern "C" float get_altitude_shim(const void* terrain, float x, float z) {
    const uint64_t vt = *reinterpret_cast<const uint64_t*>(
        reinterpret_cast<uintptr_t>(terrain));
    const bool in_bounds =
        reinterpret_cast<bool (*)(const void*, float, float)>(
            *reinterpret_cast<const uint64_t*>(vt + 0x58))(terrain, x, z);
    if (!in_bounds) return 0.0f;  // xorps/xorps @ 0x100738942

    const uintptr_t t = reinterpret_cast<uintptr_t>(terrain);
    const uint32_t max_x = *reinterpret_cast<const uint32_t*>(t + 0x48);
    const uint32_t max_z = *reinterpret_cast<const uint32_t*>(t + 0x4c);
    const uint32_t stride = *reinterpret_cast<const uint32_t*>(t + 0x50);
    const uint64_t alts = *reinterpret_cast<const uint64_t*>(t + 0x88);
    return opensc4::terrain_altitude::Interpolate(
        x, z, max_x, max_z, stride, [alts](uint32_t index) {
            return *reinterpret_cast<const float*>(alts + 36ull * index);
        });
}

// Counting wrappers: swapped slots point HERE, so the counters prove the
// game itself executed our code (Phase A's thunks only forwarded).
constexpr int kPhaseBTargets = 5;  // 0=brushradius 1=cost 2=water 3=land 4=altitude
static uint64_t g_pb_counts[kPhaseBTargets];
const char* const kPBNames[kPhaseBTargets] = {
    "BrushRadius", "GetCostPerBrushApplication", "CellHasWater", "CellHasLand",
    "GetAltitude"};

extern "C" float pb_brushradius_counted(const void* t, uint32_t id) {
    g_pb_counts[0]++;
    return brush_radius_shim(t, id);
}
extern "C" int64_t pb_cost_counted(const void* t, uint32_t id) {
    g_pb_counts[1]++;
    return get_cost_shim(t, id);
}
extern "C" uint64_t pb_water_counted(const void* t, uint32_t x, uint32_t z) {
    g_pb_counts[2]++;
    return cell_has_water_shim(t, x, z);
}
extern "C" uint64_t pb_land_counted(const void* t, uint32_t x, uint32_t z) {
    g_pb_counts[3]++;
    return cell_has_land_shim(t, x, z);
}
extern "C" float pb_altitude_counted(const void* t, float x, float z) {
    g_pb_counts[4]++;
    return get_altitude_shim(t, x, z);
}

struct ProveCtx {
    char kind[16];
    uint64_t terrain;
};

// Equivalence proofs on live data: our shim vs the original function, same
// live inputs, bit-exact comparison. Dispatched on the MAIN queue (the
// originals are game code; brushradius-prove's RPC-thread precedent is kept
// for wave 1, but the heavier proves go through the safe pump point).
// W6 isolated ABI fixtures. Layout verified from fresh --no-verify disassembly:
// SetIsRadioactive @ 0x10009a738, SetAirValue @ 0x10009a790,
// SetWaterValue @ 0x10009a7e8, SetGarbageValue @ 0x10009a840,
// IsActiveLandfill @ 0x10009a924, SetIsActiveLandfill @ 0x10009a940.
// Only synthetic storage/vtables are writable; no city or global vtable access.
#include "../../recon/pollution_current_capacity/capacity_oracle_fragment.cpp"

namespace pg = opensc4::pollution_grid_kernel;
struct PGFixture;
struct PGEndpoint { uintptr_t* vt; PGFixture* owner; uint32_t id; };
struct PGObject {
    uintptr_t* vt;
    uint8_t pad08[0x38];
    PGEndpoint* mode;
    uint8_t pad48[0x48];
    PGEndpoint* grid[4];
    uint8_t padb0[0x21c];
    uint32_t count;
    uint8_t pad2d0[0x58];
    uint8_t dirty[4];
};
static_assert(offsetof(PGObject, mode) == 0x40);
static_assert(offsetof(PGObject, grid) == 0x90);
static_assert(offsetof(PGObject, count) == 0x2cc);
static_assert(offsetof(PGObject, dirty) == 0x328);
struct PGEvent {
    uint32_t kind, grid, x, z, raw, count, dirty;
};
struct PGFixture {
    PGObject object; // first: own-query callback recovers this standard-layout owner
    uintptr_t ownVT[50], gridVT[26], modeVT[13];
    PGEndpoint grids[4], mode;
    pg::State state;
    PGEvent events[8];
    uint32_t n, overflow, old, modeResult, queryResult;
    int32_t shift;
    uint16_t stored[4];
};
static_assert(offsetof(PGFixture, object) == 0);
static uint32_t pg_dirty(const PGFixture& f) {
    uint32_t result; memcpy(&result, f.object.dirty, sizeof(result)); return result;
}
static void pg_event(PGFixture& f, uint32_t kind, uint32_t grid,
                     uint32_t x, uint32_t z, uint32_t raw) {
    if (f.n == 8) { ++f.overflow; return; }
    f.events[f.n++] = {kind, grid, x, z, raw, f.object.count, pg_dirty(f)};
}
// Raw uint32 return is intentional: the original consumes TEST/CMP AL, not bool.
static uint32_t pg_query(void* p, uint32_t x, uint32_t z) {
    auto& f = *static_cast<PGFixture*>(p);
    pg_event(f, 4, 0, x, z, f.queryResult); return f.queryResult;
}
static uint32_t pg_mode(void* p) {
    auto& f = *static_cast<PGEndpoint*>(p)->owner;
    pg_event(f, 3, 0, 0, 0, f.modeResult); return f.modeResult;
}
static uint32_t pg_get(void* p, uint32_t x, uint32_t z) {
    auto& e = *static_cast<PGEndpoint*>(p);
    pg_event(*e.owner, 0, e.id, x, z, e.owner->old); return e.owner->old;
}
static int32_t pg_shift(void* p) {
    auto& e = *static_cast<PGEndpoint*>(p);
    pg_event(*e.owner, 1, e.id, 0, 0, uint32_t(e.owner->shift)); return e.owner->shift;
}
static void pg_set(void* p, uint32_t x, uint32_t z, int32_t raw) {
    auto& e = *static_cast<PGEndpoint*>(p);
    pg_event(*e.owner, 2, e.id, x, z, uint32_t(raw));
    // Ushort storage boundary only, distinct from full rawECX trace above.
    // Original ushort-grid store @ 0x10015b516 uses CX. No bounds model implied.
    e.owner->stored[e.id] = static_cast<uint16_t>(raw);
}
// Kernel contexts are owner slots, resolved afresh on EVERY dispatch.
static uint32_t pg_slot_get(void* p, uint32_t x, uint32_t z) {
    auto* e = *static_cast<PGEndpoint**>(p);
    return reinterpret_cast<uint32_t (*)(void*,uint32_t,uint32_t)>(e->vt[7])(e,x,z);
}
static int32_t pg_slot_shift(void* p) {
    auto* e = *static_cast<PGEndpoint**>(p);
    return reinterpret_cast<int32_t (*)(void*)>(e->vt[11])(e);
}
static void pg_slot_set(void* p, uint32_t x, uint32_t z, int32_t raw) {
    auto* e = *static_cast<PGEndpoint**>(p);
    reinterpret_cast<void (*)(void*,uint32_t,uint32_t,int32_t)>(e->vt[25])(e,x,z,raw);
}
static uint32_t pg_slot_mode(void* p) {
    auto* e = *static_cast<PGEndpoint**>(p);
    return reinterpret_cast<uint32_t (*)(void*)>(e->vt[12])(e);
}
static uint32_t pg_slot_query(void* p, uint32_t x, uint32_t z) {
    auto* o = static_cast<PGObject*>(p);
    return reinterpret_cast<uint32_t (*)(void*,uint32_t,uint32_t)>(o->vt[49])(o,x,z);
}
static void pg_init(PGFixture& f, int32_t shift, uint32_t old, uint32_t mode,
                    uint32_t query, uint32_t count, uint32_t dirty) {
    memset(&f, 0, sizeof(f));
    f.ownVT[49] = reinterpret_cast<uintptr_t>(&pg_query);
    f.gridVT[7] = reinterpret_cast<uintptr_t>(&pg_get);
    f.gridVT[11] = reinterpret_cast<uintptr_t>(&pg_shift);
    f.gridVT[25] = reinterpret_cast<uintptr_t>(&pg_set);
    f.modeVT[12] = reinterpret_cast<uintptr_t>(&pg_mode);
    f.object.vt = f.ownVT;
    f.mode = {f.modeVT, &f, 0}; f.object.mode = &f.mode;
    pg::Grid descriptors[4];
    for (uint32_t i=0; i<4; ++i) {
        f.grids[i] = {f.gridVT, &f, i}; f.object.grid[i] = &f.grids[i];
        f.stored[i] = static_cast<uint16_t>(old);
        descriptors[i] = {&f.object.grid[i], pg_slot_get, pg_slot_shift, pg_slot_set};
    }
    f.state = {descriptors[0], descriptors[1], descriptors[2], descriptors[3],
        &f.object.dirty[0], &f.object.dirty[1], &f.object.dirty[2], &f.object.dirty[3],
        &f.object.count, &f.object.mode, pg_slot_mode, &f.object, pg_slot_query};
    f.old=old; f.shift=shift; f.modeResult=mode; f.queryResult=query;
    f.object.count=count; memcpy(f.object.dirty, &dirty, 4);
}
static const uint64_t pg_addresses[] = {
    0x10009a738,0x10009a790,0x10009a7e8,0x10009a840,0x10009a924,0x10009a940};
static const char* const pg_names[] = {"SetIsRadioactive","SetAirValue","SetWaterValue",
    "SetGarbageValue","IsActiveLandfill","SetIsActiveLandfill"};
static void pg_log_result(const PGFixture& f, uint32_t method, uint32_t id,
                          const char* side, uint32_t ret) {
    logf_("PG case=%u method=%s side=%s return=%s:%u dirty=%08x count=%08x "
          "storage16=%04x,%04x,%04x,%04x events=%u overflow=%u", id, pg_names[method], side,
          method==4 ? "u32" : "void", ret, pg_dirty(f), f.object.count,
          unsigned(f.stored[0]),unsigned(f.stored[1]),unsigned(f.stored[2]),unsigned(f.stored[3]), f.n,f.overflow);
    for (uint32_t i=0;i<f.n;++i) {
        const auto& e=f.events[i];
        logf_("PG case=%u side=%s event=%u kind=%u grid=%u x=%08x z=%08x "
              "raw=%08x count=%08x dirty=%08x",id,side,i,e.kind,e.grid,e.x,e.z,e.raw,e.count,e.dirty);
    }
}
static bool pg_case(uint32_t method, uint32_t id, uint32_t x, uint32_t z,
                    int32_t value, int32_t shift, uint32_t old, uint32_t mode,
                    uint32_t query, uint32_t count, uint32_t dirty) {
    PGFixture a{}, b{};
    pg_init(a,shift,old,mode,query,count,dirty);
    pg_init(b,shift,old,mode,query,count,dirty);
    const uint64_t addr=pg_addresses[method]+g_slide;
    uint32_t ar=0, br=0; // void methods have no meaningful return register
    if (method==4) ar=reinterpret_cast<uint32_t (*)(void*,uint32_t,uint32_t)>(addr)(&a.object,x,z);
    else if (method==0 || method==5)
        reinterpret_cast<void (*)(void*,uint32_t,uint32_t,bool)>(addr)(&a.object,x,z,value!=0);
    else reinterpret_cast<void (*)(void*,uint32_t,uint32_t,int32_t)>(addr)(&a.object,x,z,value);
    switch(method) {
        case 0: pg::SetIsRadioactive(b.state,x,z,value!=0); break;
        case 1: pg::SetAirValue(b.state,x,z,value); break;
        case 2: pg::SetWaterValue(b.state,x,z,value); break;
        case 3: pg::SetGarbageValue(b.state,x,z,value); break;
        case 4: br=pg::IsActiveLandfill(b.state,x,z); break;
        case 5: pg::SetIsActiveLandfill(b.state,x,z,value!=0); break;
    }
    const bool ok=ar==br && a.n==b.n && !a.overflow && !b.overflow &&
        !memcmp(a.events,b.events,sizeof(a.events)) &&
        !memcmp(a.stored,b.stored,sizeof(a.stored)) &&
        a.object.count==b.object.count && pg_dirty(a)==pg_dirty(b);
    logf_("PG INPUT case=%u method=%s x=%08x z=%08x value=%08x shift=%d "
          "old=%08x mode=%08x query=%08x count=%08x dirty=%08x result=%s",
          id,pg_names[method],x,z,uint32_t(value),shift,old,mode,query,count,dirty,ok?"MATCH":"MISMATCH");
    pg_log_result(a,method,id,"original",ar); pg_log_result(b,method,id,"kernel",br);
    return ok;
}
static void pollution_grid_prove_exec(void*) {
    if (!pthread_main_np()) { logf_("PG ERROR: not main thread"); return; }
    logf_("PG BEGIN original-vs-shared synthetic fixtures; no city/tick/substitution evidence");
    uint32_t totals[6]={}, failures[6]={}, id=0;
    const uint32_t coords[][2]={{0,0},{31,65},{0x80000001u,0xffffffffu}};
    const int32_t shifts[]={0,1,31,32,33,-1,256};
    const int32_t values[]={0,1,5,32767,32768,65535,65536,-1,INT32_MIN,INT32_MAX};
    const uint32_t olds[]={0,0x30,0x8000,0xffff,0xffff0030,0x12348030};
    const uint32_t modes[]={14,0,0x1000000e};
    const uint32_t queries[]={0,1,2,0x100,0x101,0x102,0xffffffff};
    const uint32_t counts[]={0,1,0xffffffff};
    const uint32_t dirties[]={0,0xa5b6c7d8};
    auto run=[&](uint32_t m,const uint32_t* xy,int32_t v,int32_t sh,uint32_t old,
                 uint32_t mode,uint32_t query,uint32_t count,uint32_t dirty) {
        ++totals[m]; if (!pg_case(m,++id,xy[0],xy[1],v,sh,old,mode,query,count,dirty)) ++failures[m];
    };
    for (const auto& xy:coords) for (auto dirty:dirties) {
        for (auto sh:shifts) {
            for (int v=0;v<2;++v) run(0,xy,v,sh,0x30,14,0,1,dirty);
            for (auto v:values) {
                run(1,xy,v,sh,0x30,14,0,1,dirty);
                run(2,xy,v,sh,0x30,14,0,1,dirty);
                for (auto old:olds) run(3,xy,v,sh,old,14,0,1,dirty);
            }
        }
        for (auto old:olds) {
            run(4,xy,0,0,old,14,0,1,dirty);
            for (auto mode:modes) for (auto query:queries) for (auto count:counts)
                for (int v=0;v<2;++v) run(5,xy,v,0,old,mode,query,count,dirty);
        }
    }
    uint32_t mismatches=0;
    for (uint32_t m=0;m<6;++m) {
        mismatches+=failures[m];
        logf_("PG METHOD %s @0x%llx cases=%u mismatches=%u",pg_names[m],
              (unsigned long long)pg_addresses[m],totals[m],failures[m]);
    }
    logf_("PG TOTAL cases=%u mismatches=%u END",id,mismatches);
}

void phaseb_prove_exec(void* p) {
    ProveCtx* c = reinterpret_cast<ProveCtx*>(p);
    const uint64_t t = c->terrain;
    if (t < 0x1000) {
        logf_("RPC prove %s error: bad terrain address", c->kind);
        free(c);
        return;
    }
    if (strcmp(c->kind, "cost") == 0) {
        const auto original = reinterpret_cast<int64_t (*)(const void*, uint32_t)>(
            0x100738d76 + g_slide);
        uint32_t keys[128];
        int count = 0;
        const uint64_t root =
            *reinterpret_cast<const uint64_t*>(0x100b71e20 + g_slide);
        collect_map_keys(root, keys, &count, 128);
        int mismatches = 0;
        for (int i = 0; i < count; ++i) {
            const int64_t orig = original((const void*)t, keys[i]);
            const int64_t ours = get_cost_shim((const void*)t, keys[i]);
            if (orig != ours) {
                mismatches++;
                logf_("PHASE-B PROVE MISMATCH cost key=0x%x orig=%lld ours=%lld",
                      keys[i], (long long)orig, (long long)ours);
            }
        }
        const uint32_t missing[] = {0x1u, 0x9999u, 0xffffffffu};
        for (uint32_t id : missing) {
            const int64_t orig = original((const void*)t, id);
            const int64_t ours = get_cost_shim((const void*)t, id);
            if (orig != ours) mismatches++;
        }
        logf_("PHASE-B PROVE GetCostPerBrushApplication: %d live keys + 3 "
              "missing ids, %d mismatches", count, mismatches);
    } else if (strcmp(c->kind, "cell") == 0) {
        const auto orig_water =
            reinterpret_cast<bool (*)(const void*, uint32_t, uint32_t)>(
                0x10073562c + g_slide);
        const auto orig_land =
            reinterpret_cast<bool (*)(const void*, uint32_t, uint32_t)>(
                0x1007356a2 + g_slide);
        const uint32_t count_x =
            *reinterpret_cast<const uint32_t*>(t + 0x40);
        const uint32_t count_z =
            *reinterpret_cast<const uint32_t*>(t + 0x44);
        const uint32_t step_x = count_x / 64 + 1;
        const uint32_t step_z = count_z / 64 + 1;
        int mismatches = 0;
        int tested = 0;
        for (uint32_t x = 0; x < count_x; x += step_x) {
            for (uint32_t z = 0; z < count_z; z += step_z) {
                ++tested;
                if (orig_water((const void*)t, x, z) !=
                    cell_has_water_shim((const void*)t, x, z)) {
                    mismatches++;
                    logf_("PHASE-B PROVE MISMATCH water cell=(%u,%u)", x, z);
                }
                if (orig_land((const void*)t, x, z) !=
                    cell_has_land_shim((const void*)t, x, z)) {
                    mismatches++;
                    logf_("PHASE-B PROVE MISMATCH land cell=(%u,%u)", x, z);
                }
            }
        }
        // Out-of-range coordinates (jbe bounds gate).
        const uint32_t oob[][2] = {{count_x, 0}, {0, count_z}, {0xffffffffu, 2}};
        for (auto& coords : oob) {
            ++tested;
            if (orig_water((const void*)t, coords[0], coords[1]) !=
                cell_has_water_shim((const void*)t, coords[0], coords[1])) {
                mismatches++;
            }
            if (orig_land((const void*)t, coords[0], coords[1]) !=
                cell_has_land_shim((const void*)t, coords[0], coords[1])) {
                mismatches++;
            }
        }
        logf_("PHASE-B PROVE CellHasWater+CellHasLand: %d cells (both "
              "directions) + 3 oob pairs, %d mismatches",
              tested * 2, mismatches);
    } else if (strcmp(c->kind, "altitude") == 0) {
        const auto original =
            reinterpret_cast<float (*)(const void*, float, float)>(
                0x100738928 + g_slide);
        const uint32_t max_x = *reinterpret_cast<const uint32_t*>(t + 0x48);
        const uint32_t max_z = *reinterpret_cast<const uint32_t*>(t + 0x4c);
        const float extent_x = static_cast<float>(max_x + 2) * 16.0f;
        const float extent_z = static_cast<float>(max_z + 2) * 16.0f;
        int mismatches = 0;
        long tested = 0;
        // Coarse grid across the whole city (~3700 points).
        for (int i = 0; i <= 60; ++i) {
            for (int j = 0; j <= 60; ++j) {
                const float x = extent_x * i / 60.0f;
                const float z = extent_z * j / 60.0f;
                const float o = original((const void*)t, x, z);
                const float s = get_altitude_shim((const void*)t, x, z);
                ++tested;
                if (__builtin_memcmp(&o, &s, 4) != 0) {
                    if (++mismatches <= 5) {
                        logf_("PHASE-B PROVE MISMATCH altitude x=%g z=%g "
                              "orig=%g ours=%g (bits %08x vs %08x)",
                              (double)x, (double)z, (double)o, (double)s,
                              __builtin_bit_cast(uint32_t, o),
                              __builtin_bit_cast(uint32_t, s));
                    }
                }
            }
        }
        // Fine sweep across a few cells near the center — exercises both
        // triangle diagonals and the fractional blend at 0.5-world-unit steps.
        const float cx = extent_x / 2.0f;
        const float cz = extent_z / 2.0f;
        for (float fx = 0.0f; fx < 32.0f; fx += 0.5f) {
            for (float fz = 0.0f; fz < 32.0f; fz += 0.5f) {
                const float o = original((const void*)t, cx + fx, cz + fz);
                const float s = get_altitude_shim((const void*)t, cx + fx, cz + fz);
                ++tested;
                if (__builtin_memcmp(&o, &s, 4) != 0) {
                    if (++mismatches <= 5) {
                        logf_("PHASE-B PROVE MISMATCH altitude-fine x=%g z=%g "
                              "orig=%g ours=%g",
                              (double)(cx + fx), (double)(cz + fz), (double)o,
                              (double)s);
                    }
                }
            }
        }
        // Out-of-bounds gate points (LocationIsInBounds -> 0.0f on both).
        const float oob[][2] = {{-1.0f, 0.0f}, {0.0f, -1.0f},
                                {extent_x + 64.0f, extent_z + 64.0f},
                                {1.0e9f, 1.0e9f}};
        for (auto& pt : oob) {
            const float o = original((const void*)t, pt[0], pt[1]);
            const float s = get_altitude_shim((const void*)t, pt[0], pt[1]);
            ++tested;
            if (__builtin_memcmp(&o, &s, 4) != 0) mismatches++;
        }
        // Explicit gate geometry absent from the historical 7,821-point
        // sweep: inclusive bounds, nextafter crossings, NaNs and infinities.
        const float width = *reinterpret_cast<const float*>(t + 0x68);
        const float height = *reinterpret_cast<const float*>(t + 0x6c);
        const float nan = __builtin_nanf("");
        const float inf = __builtin_inff();
        const float edges[][2] = {{-0.0f, +0.0f}, {+0.0f, -0.0f},
            {width, height}, {width, 0.0f}, {0.0f, height},
            {__builtin_nextafterf(width, inf), 0.0f},
            {0.0f, __builtin_nextafterf(height, inf)},
            {__builtin_nextafterf(0.0f, -1.0f), 0.0f},
            {0.0f, __builtin_nextafterf(0.0f, -1.0f)},
            {nan, 0.0f}, {0.0f, nan}, {nan, nan},
            {inf, 0.0f}, {0.0f, inf}, {-inf, 0.0f}, {0.0f, -inf}};
        for (const auto& pt : edges) {
            const float o = original((const void*)t, pt[0], pt[1]);
            const float s = get_altitude_shim((const void*)t, pt[0], pt[1]);
            ++tested;
            if (__builtin_memcmp(&o, &s, 4) != 0) ++mismatches;
        }
        // A POD fixture supplies only fields read by original GetAltitude
        // and LocationIsInBounds. Real terrain and saves remain untouched.
        // Offsets are from fresh asm @ 0x100738928 / 0x100740e7a.
        alignas(8) unsigned char synthetic[0x90] = {};
        float vertices[4][9] = {}; // original altitude records have stride 36.
        auto put = [&](size_t offset, const auto& value) {
            __builtin_memcpy(synthetic + offset, &value, sizeof(value));
        };
        const uint64_t vtable = *reinterpret_cast<const uint64_t*>(t);
        const uint32_t max_index = 0;
        const uint32_t stride = 2;
        const float fixture_extent = 16.0f;
        const uint64_t alt_ptr = reinterpret_cast<uint64_t>(vertices);
        put(0, vtable); put(0x48, max_index); put(0x4c, max_index);
        put(0x50, stride); put(0x68, fixture_extent); put(0x6c, fixture_extent);
        put(0x88, alt_ptr);
        for (float slope : {-1.0f, +1.0f}) {
            vertices[0][0] = -0.0f;
            vertices[2][0] = slope;
            vertices[3][0] = +0.0f;
            const float o = original(synthetic, -0.0f, +0.0f);
            const float s = get_altitude_shim(synthetic, -0.0f, +0.0f);
            ++tested;
            if (__builtin_memcmp(&o, &s, 4) != 0) ++mismatches;
            logf_("PHASE-B PROVE signed-zero slope=%g orig=%08x shared=%08x",
                  (double)slope, __builtin_bit_cast(uint32_t, o),
                  __builtin_bit_cast(uint32_t, s));
        }
        logf_("PHASE-B PROVE GetAltitude: %ld points (coarse+fine+oob+edges+signed-zero), "
              "%d mismatches (bit-exact float compare)", tested, mismatches);
    }
    free(c);
}

// --- vtable swap: strangler-fig mechanics -------------------------------------
// Patches a vtable slot (in the game's __DATA_CONST, mprotect dance) to a
// counting thunk that forwards to the original target. Phase A proves the
// mechanics (interception + call counting + clean restore); Phase B will
// point slots at reconstructed implementations. The Mac port's built-in
// plugin loader is a stub (dossier_game_plugin_abi.md), so vtable
// substitution via the injected oracle IS the strangler fig on this binary.
//
// Limitation (documented): the generic thunks forward 6 integer-ABI args
// (x86_64 SysV). Float-arg virtuals and stack-passed args beyond 6 are not
// covered by the counting path.
constexpr int kMaxSwaps = 8;
struct SwapEntry {
    uint64_t slot_addr;   // runtime address of the vtable slot
    uint64_t original;     // original target
    uint64_t counter;
    bool active;
};
static SwapEntry g_swaps[kMaxSwaps];

static uint64_t swap_forward(int idx, uint64_t a0, uint64_t a1, uint64_t a2,
                              uint64_t a3, uint64_t a4, uint64_t a5) {
    SwapEntry& e = g_swaps[idx];
    e.counter++;
    const fn6_t f = reinterpret_cast<fn6_t>(e.original);
    return f(a0, a1, a2, a3, a4, a5);
}

#define SWAP_THUNK(N)                                                          \
    extern "C" uint64_t swap_thunk_##N(                                        \
        uint64_t a0, uint64_t a1, uint64_t a2, uint64_t a3, uint64_t a4,      \
        uint64_t a5) {                                                         \
        return swap_forward(N, a0, a1, a2, a3, a4, a5);                        \
    }
SWAP_THUNK(0) SWAP_THUNK(1) SWAP_THUNK(2) SWAP_THUNK(3)
SWAP_THUNK(4) SWAP_THUNK(5) SWAP_THUNK(6) SWAP_THUNK(7)
static const void* const k_swap_thunks[kMaxSwaps] = {
    (const void*)&swap_thunk_0, (const void*)&swap_thunk_1,
    (const void*)&swap_thunk_2, (const void*)&swap_thunk_3,
    (const void*)&swap_thunk_4, (const void*)&swap_thunk_5,
    (const void*)&swap_thunk_6, (const void*)&swap_thunk_7,
};

static SwapEntry* find_swap(uint64_t slot_addr) {
    for (auto& e : g_swaps) {
        if (e.active && e.slot_addr == slot_addr) return &e;
    }
    return nullptr;
}

static bool patch_u64(uint64_t addr, uint64_t value) {
    const mach_vm_address_t page = addr & ~0xFFFULL;
    mach_vm_size_t size = 0x1000;
    vm_region_basic_info_data_64_t info;
    mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
    mach_port_t obj_name = MACH_PORT_NULL;
    mach_vm_address_t probe = page;
    if (mach_vm_region(mach_task_self(), &probe, &size, VM_REGION_BASIC_INFO_64,
                       reinterpret_cast<vm_region_info_t>(&info), &count,
                       &obj_name) != KERN_SUCCESS) {
        return false;
    }
    if (mach_vm_protect(mach_task_self(), page, 0x1000, 0,
                        VM_PROT_READ | VM_PROT_WRITE | VM_PROT_COPY) != KERN_SUCCESS) {
        logf_("SWAP mprotect RW failed at 0x%llx", (unsigned long long)page);
        return false;
    }
    *reinterpret_cast<uint64_t*>(addr) = value;
    mach_vm_protect(mach_task_self(), page, 0x1000, 0, info.protection);
    return true;
}
// --- find command: scan all writable regions for a u64 value ------------------
// Locates live objects by vptr (e.g. find 0x100a67010 = &__ZTV7cSC4App+0x10
// yields the cSC4App instance). Read-only; runs on the RPC thread.
void scan_for(uint64_t needle, int max_hits) {
    int hits = 0;
    int regions = 0;
    uint64_t total_scanned = 0;
    mach_vm_address_t addr = 0;
    mach_vm_size_t size = 0;
    char out[1024];
    size_t o = 0;
    while (hits < max_hits && regions < 100000) {
        vm_region_basic_info_data_64_t info;
        mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
        mach_port_t obj_name = MACH_PORT_NULL;
        const kern_return_t kr = mach_vm_region(
            mach_task_self(), &addr, &size, VM_REGION_BASIC_INFO_64,
            reinterpret_cast<vm_region_info_t>(&info), &count, &obj_name);
        if (kr != KERN_SUCCESS) {
            if (regions == 0) {
                logf_("SCAN first mach_vm_region failed: kr=%d", (int)kr);
            }
            break;
        }
        regions++;
        if (regions <= 3) {
            logf_("SCAN region %d: base=0x%llx size=%llu prot=%d maxprot=%d",
                  regions, (unsigned long long)addr, (unsigned long long)size,
                  (int)info.protection, (int)info.max_protection);
        }
        if ((info.protection & VM_PROT_WRITE) && size <= 512ull * 1024 * 1024) {
            // Read in 1 MB chunks with 8-byte overlap handling.
            mach_vm_address_t cur = addr;
            mach_vm_address_t end = addr + size;
            while (cur < end && hits < max_hits) {
                mach_vm_size_t chunk = 1024 * 1024;
                if (end - cur < chunk) chunk = end - cur;
                static uint8_t buf[1024 * 1024];
                mach_vm_size_t got_bytes = 0;
                if (mach_vm_read_overwrite(mach_task_self(), cur, chunk,
                                           (mach_vm_address_t)buf, &got_bytes) == KERN_SUCCESS) {
                    total_scanned += got_bytes;
                    // 4-byte-aligned scan: match 32-bit needles exactly, and
                    // 64-bit needles as two consecutive halves.
                    for (mach_vm_size_t i = 0; i + 4 <= got_bytes && hits < max_hits; i += 4) {
                        uint32_t lo;
                        memcpy(&lo, buf + i, 4);
                        if (lo != (uint32_t)needle) continue;
                        if (needle > 0xffffffffull) {
                            if (i + 8 > got_bytes) continue;
                            uint32_t hi;
                            memcpy(&hi, buf + i + 4, 4);
                            if (hi != (uint32_t)(needle >> 32)) continue;
                        }
                        if (o == 0) {
                            o = (size_t)snprintf(out, sizeof(out), "RPC find 0x%llx:", (unsigned long long)needle);
                        }
                        if (hits < 16) {
                            o += (size_t)snprintf(out + o, sizeof(out) - o, " 0x%llx",
                                                  (unsigned long long)(cur + i));
                        }
                        hits++;
                    }
                }
                cur += chunk;
            }
        }
        addr += size;
        size = 0;
    }
    if (hits > 0) {
        if (hits > 16) snprintf(out + o, sizeof(out) - o, " ... (%d hits total)", hits);
        logf_("%s", out);
    } else {
        logf_("RPC find 0x%llx: no hits (%d regions, %llu bytes scanned)",
              (unsigned long long)needle, regions, (unsigned long long)total_scanned);
    }
}

// --- game-playing commands ----------------------------------------------------
// Load chain (recon dossier_game_plugin_abi + load-chain recon, Nov 2026):
//   cSC4App::LoadCity(cRZString const&, cRZAutoRefCount<cISC4RegionalCity>*)
//     @ 0x1000455d8 - SYNCHRONOUS, directly callable with (app, path, 0).
//   cSC4App::GetCity @ 0x10001231a = *(this+0xa0); GetAppState @ 0x100013634
//     = *(this+0x78). cRZString ctor from char* @ 0x1004879e0 (construct in a
//   game-runtime-owned way to avoid the two-libc++ hazard).
//   cSC4App::SaveCityForObliteration(cRZString const&) @ 0x1000475cc.
constexpr uint64_t kLoadCity = 0x1000455d8;
constexpr uint64_t kCRZStringFromChar = 0x1004879e0;
constexpr uint64_t kCRZStringDtor = 0x10005f070;
constexpr uint64_t kCRZStringVptr = 0x100a67890;  // __ZTV9cRZString + 0x10
constexpr uint64_t kSaveCityForObliteration = 0x1000475cc;
// Full checkpoint save: cSC4App::SaveCity(cRZString const&, bool) @
// 0x100046036 (vt+0x148) - the real save path: cSC4App::Write @ 0x100044510
// serializes app+city+regionalCity records, via temp file + backup rename.
// (The obliteration variant @ 0x1000475cc writes only terrain+flora - verified
// empirically: a Spandau checkpoint contained 0x8a91e7e3/0x8a91e7e0/0xa9dd6ff4
// + 3038 flora records and no development subfiles.)
constexpr uint64_t kSaveCityPath = 0x100046036;
constexpr uint64_t kRequestGoToRegionView = 0x1000451e6;
constexpr uint64_t kEnablePauseOnFocusLoss = 0x10000f2ba;
constexpr uint64_t kFullGamePause = 0x10000f322;
constexpr uint64_t kRegionScreenLoadRegion = 0x10023b130;
constexpr uint64_t kRegionScreenGetOrCreateCity = 0x10023ae9c;
constexpr uint64_t kRegionScreenLaunchRegionalCity = 0x10023b63c;
constexpr uint64_t kRegionScreenVptr = 0x100a88520;  // __ZTV19cSC4WinRegionScreen + 0x10
constexpr uint64_t kSimGetDateNumber = 0x1001f19d8;
constexpr uint64_t kSimPause = 0x1001f1100;
constexpr uint64_t kSimResume = 0x1001f11a0;
constexpr uint64_t kSimHiddenResume = 0x1001f11c4;
constexpr uint64_t kSimSetSpeed = 0x1001f093e;
constexpr uint64_t kFrameworkMpApp = 0x100b21238;    // cGZFramework::mpApp (cSC4App+0x48)
constexpr uint64_t kSLSpSC4 = 0x100b21258;           // SL::spSC4 (cISC4App* = cSC4App+0x18)
constexpr uint64_t kSkipMovieFlag = 0x100b713b1;     // CQTPlayer::mbsNCTSkipMovie
constexpr uint64_t kAppVptr = 0x100a67010;           // __ZTV7cSC4App + 0x10

uint64_t g_app = 0;  // cached cSC4App instance (static addr space is slide==0,
                     // but heap addrs are absolute runtime values)

uint64_t find_app_instance();

uint64_t get_app() {
    // O(1) lookup via cGZFramework::mpApp @ 0x100b21238 (set in SC4AppStart @
    // 0x100003983 to cSC4App + 0x48). Falls back to heap scan if needed.
    const uint64_t mp_app =
        *reinterpret_cast<const uint64_t*>(kFrameworkMpApp + g_slide);
    if (mp_app > 0x1000) {
        const uint64_t cand = mp_app - 0x48;
        if (*reinterpret_cast<const uint64_t*>(cand) == kAppVptr + g_slide) {
            g_app = cand;
            return g_app;
        }
    }
    if (g_app == 0) g_app = find_app_instance();
    return g_app;
}

#include "../../recon/loaded_building_capacity/live_capture_fragment.inc"
#include "../../recon/loaded_building_capacity/sint32_live_capture.inc"

struct ScopedRZString {
    alignas(8) uint8_t buf[64];
    ScopedRZString() {
        memset(buf, 0, sizeof(buf));
        *reinterpret_cast<uint64_t*>(buf) = kCRZStringVptr + g_slide;
    }
    explicit ScopedRZString(const char* s) {
        memset(buf, 0, sizeof(buf));
        reinterpret_cast<void (*)(void*, const char*)>(
            kCRZStringFromChar + g_slide)(buf, s ? s : "");
    }
    ~ScopedRZString() {
        reinterpret_cast<void (*)(void*)>(kCRZStringDtor + g_slide)(buf);
    }
    const char* c_str() const {
        const uint8_t tag = buf[0x8];
        return (tag & 1)
                   ? *reinterpret_cast<const char* const*>(buf + 0x18)
                   : reinterpret_cast<const char*>(buf + 0x9);
    }
};

#include "../../recon/loaded_aging_transition/fixture_placement_fragment.inc"
#define OPENSC4_PLACEMENT_CHECKPOINT_INTEGRATION 1
#define OPENSC4_PLACEMENT_RELOAD_INTEGRATION 1
#include "../../recon/loaded_aging_transition/fixture_placement_bindings.inc"
#include "../../recon/loaded_aging_transition/fixture_query_fragment.inc"
#include "../../recon/loaded_aging_transition/tutorial_inspect_fragment.inc"
#include "../../recon/loaded_aging_transition/tutorial_open_fragment.inc"
#include "../../recon/loaded_aging_transition/tutorial_finish_fragment.inc"
#include "../../recon/loaded_aging_transition/tutorial_checkpoint_fragment.inc"
#include "../../recon/loaded_aging_transition/tutorial_reload_fragment.inc"
#include "../../recon/loaded_aging_transition/placement_checkpoint_fragment.inc"
#include "../../recon/loaded_aging_transition/aging_input_capture_fragment.inc"
#include "../../recon/loaded_aging_transition/placement_reload_fragment.inc"

// Ticket-local RPC parser: complete signed integer pairs, at most32 points.
static bool parse_fixture_points(const char* text, fixture_placement::Point (&points)[32], unsigned& count) {
    count = 0;
    while (*text == ' ' || *text == '\t') ++text;
    while (*text) {
        if (count == 32) return false;
        int32_t pair[2];
        for (unsigned i = 0; i < 2; ++i) {
            if (!*text) return false;
            char* end = nullptr; errno = 0;
            const long value = std::strtol(text, &end, 10);
            if (end == text || errno == ERANGE || value < INT32_MIN || value > INT32_MAX ||
                (*end && *end != ' ' && *end != '\t')) return false;
            pair[i] = static_cast<int32_t>(value); text = end;
            while (*text == ' ' || *text == '\t') ++text;
        }
        points[count++] = {pair[0],pair[1]};
    }
    return count != 0;
}

uint64_t get_region_screen(uint64_t app) {
    if (!app) return 0;
    const uint64_t win_app = *reinterpret_cast<const uint64_t*>(app + 0x88);
    if (win_app < 0x1000) return 0;
    const uint64_t screen = *reinterpret_cast<const uint64_t*>(win_app + 0x178);
    if (screen < 0x1000) return 0;
    const uint64_t vptr = *reinterpret_cast<const uint64_t*>(screen);
    if (vptr != kRegionScreenVptr + g_slide) return 0;
    return screen;
}

void disable_background_pause(uint64_t app) {
    if (!app) return;
    reinterpret_cast<void (*)(void*, bool)>(kEnablePauseOnFocusLoss + g_slide)(
        (void*)app, false);
    reinterpret_cast<void (*)(void*, bool)>(kFullGamePause + g_slide)(
        (void*)app, false);
}

void regions_exec(void*) {
    const uint64_t sp_sc4 = *reinterpret_cast<const uint64_t*>(kSLSpSC4 + g_slide);
    if (sp_sc4 < 0x1000) {
        logf_("RPC regions error: SL::spSC4 is null");
        return;
    }
    const uint64_t sp_vt = *reinterpret_cast<const uint64_t*>(sp_sc4);
    const uint64_t nation =
        reinterpret_cast<uint64_t (*)(uint64_t)>(
            *reinterpret_cast<const uint64_t*>(sp_vt + 0x110))(sp_sc4);
    if (nation < 0x1000) {
        logf_("RPC regions error: GetNation() returned null");
        return;
    }
    const uint64_t nat_vt = *reinterpret_cast<const uint64_t*>(nation);
    const uint32_t count =
        reinterpret_cast<uint32_t (*)(uint64_t)>(
            *reinterpret_cast<const uint64_t*>(nat_vt + 0x30))(nation);
    const uint64_t screen = get_region_screen(get_app());
    const int32_t active_idx =
        screen ? *reinterpret_cast<const int32_t*>(screen + 0x2bc) : -1;
    logf_("RPC regions count=%u (active=%d):", count, active_idx);
    for (uint32_t i = 0; i < count; ++i) {
        const uint64_t reg =
            reinterpret_cast<uint64_t (*)(uint64_t, uint32_t)>(
                *reinterpret_cast<const uint64_t*>(nat_vt + 0x58))(nation, i);
        if (reg < 0x1000) continue;
        const uint64_t reg_vt = *reinterpret_cast<const uint64_t*>(reg);
        const uint64_t igz_str =
            reinterpret_cast<uint64_t (*)(uint64_t)>(
                *reinterpret_cast<const uint64_t*>(reg_vt + 0x18))(reg);
        const char* name = "";
        if (igz_str > 0x1000) {
            const uint64_t str_vt = *reinterpret_cast<const uint64_t*>(igz_str);
            name = reinterpret_cast<const char* (*)(uint64_t)>(
                *reinterpret_cast<const uint64_t*>(str_vt + 0x40))(igz_str);
        }
        logf_("RPC region[%u]%s = \"%s\"", i,
              ((int32_t)i == active_idx) ? "*" : "", name ? name : "");
    }
}

struct StrArgCtx {
    char arg[512];
};

void region_load_exec(void* p) {
    StrArgCtx* c = reinterpret_cast<StrArgCtx*>(p);
    const uint64_t app = get_app();
    if (!app) {
        logf_("RPC region error: app not ready");
        free(c);
        return;
    }
    disable_background_pause(app);
    if (*reinterpret_cast<const uint64_t*>(app + 0xa0) != 0) {
        reinterpret_cast<bool (*)(void*, bool)>(
            kRequestGoToRegionView + g_slide)((void*)app, false);
    }
    const uint64_t screen = get_region_screen(app);
    if (!screen) {
        logf_("RPC region error: cSC4WinRegionScreen not active");
        free(c);
        return;
    }
    ScopedRZString rname(c->arg);
    const bool ok = reinterpret_cast<bool (*)(void*, const void*)>(
        kRegionScreenLoadRegion + g_slide)((void*)screen, rname.buf);
    const uint64_t new_screen = get_region_screen(app);
    const int32_t active_idx =
        new_screen ? *reinterpret_cast<const int32_t*>(new_screen + 0x2bc) : -1;
    logf_("RPC region \"%s\" -> %s (screen=0x%llx regionIdx=%d)",
          c->arg, ok ? "OK" : "FAILED", (unsigned long long)new_screen,
          active_idx);
    free(c);
}

void cities_exec(void*) {
    const uint64_t app = get_app();
    const uint64_t screen = get_region_screen(app);
    if (!screen) {
        logf_("RPC cities error: not on region screen (use 'region-view' first)");
        return;
    }
    const uint64_t sp_sc4 = *reinterpret_cast<const uint64_t*>(kSLSpSC4 + g_slide);
    const uint64_t sp_vt = *reinterpret_cast<const uint64_t*>(sp_sc4);
    const uint64_t nation =
        reinterpret_cast<uint64_t (*)(uint64_t)>(
            *reinterpret_cast<const uint64_t*>(sp_vt + 0x110))(sp_sc4);
    const uint64_t nat_vt = *reinterpret_cast<const uint64_t*>(nation);
    const uint32_t reg_idx = *reinterpret_cast<const uint32_t*>(screen + 0x2bc);
    const uint64_t region =
        reinterpret_cast<uint64_t (*)(uint64_t, uint32_t)>(
            *reinterpret_cast<const uint64_t*>(nat_vt + 0x58))(nation, reg_idx);
    if (region < 0x1000) {
        logf_("RPC cities error: current region is null");
        return;
    }
    const uint64_t reg_vt = *reinterpret_cast<const uint64_t*>(region);
    const uint64_t begin = *reinterpret_cast<const uint64_t*>(screen + 0x1d0);
    const uint64_t end = *reinterpret_cast<const uint64_t*>(screen + 0x1d8);
    const uint64_t stride = 0xf0;
    const uint32_t total = (end >= begin) ? (uint32_t)((end - begin) / stride) : 0;
    logf_("RPC cities in regionIdx=%u: %u tiles", reg_idx, total);
    uint32_t idx = 0;
    for (uint64_t cur = begin; cur + stride <= end; cur += stride, ++idx) {
        const uint32_t tx = *reinterpret_cast<const uint32_t*>(cur + 0x8);
        const uint32_t tz = *reinterpret_cast<const uint32_t*>(cur + 0xc);
        const uint8_t sz = *reinterpret_cast<const uint8_t*>(cur + 0x18);
        const uint64_t ref_ptr =
            reinterpret_cast<uint64_t (*)(uint64_t, uint32_t, uint32_t)>(
                *reinterpret_cast<const uint64_t*>(reg_vt + 0x58))(region, tx, tz);
        const uint64_t rcity =
            (ref_ptr > 0x1000) ? *reinterpret_cast<const uint64_t*>(ref_ptr) : 0;
        bool est = false;
        ScopedRZString cname;
        ScopedRZString cpath;
        if (rcity > 0x1000) {
            const uint64_t rvt = *reinterpret_cast<const uint64_t*>(rcity);
            est = reinterpret_cast<bool (*)(uint64_t)>(
                *reinterpret_cast<const uint64_t*>(rvt + 0x158))(rcity);
            reinterpret_cast<void (*)(uint64_t, void*)>(
                *reinterpret_cast<const uint64_t*>(rvt + 0x108))(rcity, cname.buf);
            reinterpret_cast<void (*)(uint64_t, void*)>(
                *reinterpret_cast<const uint64_t*>(rvt + 0xf8))(rcity, cpath.buf);
        }
        logf_("RPC city[#%u] (%u,%u) size=%u est=%d name=\"%s\" path=\"%s\"",
              idx, tx, tz, (unsigned)sz, est ? 1 : 0, cname.c_str(), cpath.c_str());
    }
}

static bool ci_contains(const char* hay, const char* needle) {
    if (!hay || !needle || !*needle) return false;
    const size_t nlen = strlen(needle);
    for (const char* p = hay; *p; ++p) {
        size_t i = 0;
        while (i < nlen && p[i]) {
            char a = p[i], b = needle[i];
            if (a >= 'A' && a <= 'Z') a = (char)(a - 'A' + 'a');
            if (b >= 'A' && b <= 'Z') b = (char)(b - 'A' + 'a');
            if (a != b) break;
            ++i;
        }
        if (i == nlen) return true;
    }
    return false;
}

void launch_city_exec(void* p) {
    StrArgCtx* c = reinterpret_cast<StrArgCtx*>(p);
    const uint64_t app = get_app();
    if (!app) {
        logf_("RPC launch-city error: app not ready");
        free(c);
        return;
    }
    disable_background_pause(app);
    if (*reinterpret_cast<const uint64_t*>(app + 0xa0) != 0) {
        reinterpret_cast<bool (*)(void*, bool)>(
            kRequestGoToRegionView + g_slide)((void*)app, false);
    }
    const uint64_t screen = get_region_screen(app);
    if (!screen) {
        logf_("RPC launch-city error: cSC4WinRegionScreen not active");
        free(c);
        return;
    }
    const uint64_t sp_sc4 = *reinterpret_cast<const uint64_t*>(kSLSpSC4 + g_slide);
    const uint64_t sp_vt = *reinterpret_cast<const uint64_t*>(sp_sc4);
    const uint64_t nation =
        reinterpret_cast<uint64_t (*)(uint64_t)>(
            *reinterpret_cast<const uint64_t*>(sp_vt + 0x110))(sp_sc4);
    const uint64_t nat_vt = *reinterpret_cast<const uint64_t*>(nation);
    const uint32_t reg_idx = *reinterpret_cast<const uint32_t*>(screen + 0x2bc);
    const uint64_t region =
        reinterpret_cast<uint64_t (*)(uint64_t, uint32_t)>(
            *reinterpret_cast<const uint64_t*>(nat_vt + 0x58))(nation, reg_idx);
    const uint64_t reg_vt = *reinterpret_cast<const uint64_t*>(region);
    const uint64_t begin = *reinterpret_cast<const uint64_t*>(screen + 0x1d0);
    const uint64_t end = *reinterpret_cast<const uint64_t*>(screen + 0x1d8);
    const uint64_t stride = 0xf0;

    int want_idx = -1;
    uint32_t want_x = 0, want_z = 0;
    bool by_coords = false;
    if (c->arg[0] == '#') {
        want_idx = (int)strtol(c->arg + 1, nullptr, 10);
    } else if (sscanf(c->arg, "%u,%u", &want_x, &want_z) == 2 &&
               strchr(c->arg, ',') != nullptr) {
        by_coords = true;
    }

    uint64_t match_tile = 0;
    uint32_t idx = 0;
    for (uint64_t cur = begin; cur + stride <= end; cur += stride, ++idx) {
        const uint32_t tx = *reinterpret_cast<const uint32_t*>(cur + 0x8);
        const uint32_t tz = *reinterpret_cast<const uint32_t*>(cur + 0xc);
        if (want_idx >= 0) {
            if ((int)idx == want_idx) { match_tile = cur; break; }
            continue;
        }
        if (by_coords) {
            if (tx == want_x && tz == want_z) { match_tile = cur; break; }
            continue;
        }
        const uint64_t ref_ptr =
            reinterpret_cast<uint64_t (*)(uint64_t, uint32_t, uint32_t)>(
                *reinterpret_cast<const uint64_t*>(reg_vt + 0x58))(region, tx, tz);
        const uint64_t rcity =
            (ref_ptr > 0x1000) ? *reinterpret_cast<const uint64_t*>(ref_ptr) : 0;
        if (rcity <= 0x1000) continue;
        const uint64_t rvt = *reinterpret_cast<const uint64_t*>(rcity);
        ScopedRZString cname;
        ScopedRZString cpath;
        reinterpret_cast<void (*)(uint64_t, void*)>(
            *reinterpret_cast<const uint64_t*>(rvt + 0x108))(rcity, cname.buf);
        reinterpret_cast<void (*)(uint64_t, void*)>(
            *reinterpret_cast<const uint64_t*>(rvt + 0xf8))(rcity, cpath.buf);
        if (strcmp(cname.c_str(), c->arg) == 0) {
            match_tile = cur;
            break;
        }
        if (!match_tile &&
            (ci_contains(cname.c_str(), c->arg) ||
             ci_contains(cpath.c_str(), c->arg))) {
            match_tile = cur;
        }
    }

    if (!match_tile) {
        logf_("RPC launch-city \"%s\" FAILED: no matching tile in regionIdx=%u",
              c->arg, reg_idx);
        free(c);
        return;
    }
    const uint32_t mtx = *reinterpret_cast<const uint32_t*>(match_tile + 0x8);
    const uint32_t mtz = *reinterpret_cast<const uint32_t*>(match_tile + 0xc);
    const uint64_t ref_ptr =
        reinterpret_cast<uint64_t (*)(void*, const void*, bool)>(
            kRegionScreenGetOrCreateCity + g_slide)((void*)screen,
                                                    (const void*)match_tile, true);
    if (ref_ptr < 0x1000) {
        logf_("RPC launch-city \"%s\" FAILED: GetOrCreateRegionalCity null",
              c->arg);
        free(c);
        return;
    }
    const bool ok = reinterpret_cast<bool (*)(void*, uint64_t)>(
        kRegionScreenLaunchRegionalCity + g_slide)((void*)screen, ref_ptr);
    disable_background_pause(app);
    const uint64_t city = *reinterpret_cast<const uint64_t*>(app + 0xa0);
    const uint32_t state = *reinterpret_cast<const uint32_t*>(app + 0x78);
    const uint64_t terrain =
        city ? *reinterpret_cast<const uint64_t*>(city + 0x390) : 0;
    const uint64_t sim =
        city ? *reinterpret_cast<const uint64_t*>(city + 0x258) : 0;
    logf_("RPC launch-city \"%s\" tile=(%u,%u) -> %s "
          "(appState=0x%x city=0x%llx terrain=0x%llx sim=0x%llx)",
          c->arg, mtx, mtz, ok ? "OK" : "FAILED", state,
          (unsigned long long)city, (unsigned long long)terrain,
          (unsigned long long)sim);
    free(c);
}

void region_view_exec(void*) {
    const uint64_t app = get_app();
    if (!app) {
        logf_("RPC region-view error: app not ready");
        return;
    }
    const bool ok = reinterpret_cast<bool (*)(void*, bool)>(
        kRequestGoToRegionView + g_slide)((void*)app, false);
    const uint32_t state = *reinterpret_cast<const uint32_t*>(app + 0x78);
    const uint64_t city = *reinterpret_cast<const uint64_t*>(app + 0xa0);
    const uint64_t screen = get_region_screen(app);
    logf_("RPC region-view -> %s (appState=0x%x city=0x%llx screen=0x%llx)",
          ok ? "OK" : "FAILED", state, (unsigned long long)city,
          (unsigned long long)screen);
}

void sim_cmd_exec(void* p) {
    StrArgCtx* c = reinterpret_cast<StrArgCtx*>(p);
    const uint64_t app = get_app();
    const uint64_t city =
        app ? *reinterpret_cast<const uint64_t*>(app + 0xa0) : 0;
    const uint64_t sim =
        city ? *reinterpret_cast<const uint64_t*>(city + 0x258) : 0;
    if (!sim) {
        logf_("RPC sim error: no active city/simulator");
        free(c);
        return;
    }
    disable_background_pause(app);
    if (strcmp(c->arg, "resume") == 0) {
        int guard = 16;
        while (*reinterpret_cast<const int32_t*>(sim + 0x4c) > 0 && guard-- > 0) {
            reinterpret_cast<void (*)(uint64_t)>(kSimHiddenResume + g_slide)(sim);
        }
        guard = 16;
        while (*reinterpret_cast<const int32_t*>(sim + 0x48) > 0 && guard-- > 0) {
            reinterpret_cast<void (*)(uint64_t)>(kSimResume + g_slide)(sim);
        }
        if (*reinterpret_cast<const int32_t*>(sim + 0x54) <= 0) {
            reinterpret_cast<bool (*)(uint64_t, int)>(kSimSetSpeed + g_slide)(sim, 1);
        }
    } else if (strcmp(c->arg, "pause") == 0) {
        reinterpret_cast<void (*)(uint64_t)>(kSimPause + g_slide)(sim);
    } else if (strncmp(c->arg, "speed ", 6) == 0) {
        const int spd = (int)strtol(c->arg + 6, nullptr, 10);
        reinterpret_cast<bool (*)(uint64_t, int)>(kSimSetSpeed + g_slide)(sim, spd);
    }
    const uint32_t jdn =
        reinterpret_cast<uint32_t (*)(uint64_t)>(kSimGetDateNumber + g_slide)(sim);
    const int32_t pause_ct = *reinterpret_cast<const int32_t*>(sim + 0x48);
    const int32_t hpause_ct = *reinterpret_cast<const int32_t*>(sim + 0x4c);
    const int32_t epause_ct = *reinterpret_cast<const int32_t*>(sim + 0x50);
    const int32_t speed = *reinterpret_cast<const int32_t*>(sim + 0x54);
    const uint8_t enabled = *reinterpret_cast<const uint8_t*>(sim + 0x5c);
    logf_("RPC sim (%s) sim=0x%llx jdn=%u pause=%d hiddenPause=%d emergPause=%d speed=%d enabled=%u",
          c->arg, (unsigned long long)sim, jdn, pause_ct, hpause_ct, epause_ct,
          speed, (unsigned)enabled);
    free(c);
}

struct LoadCtx {
    char path[512];
};

void load_exec(void* p) {
    LoadCtx* c = reinterpret_cast<LoadCtx*>(p);
    // LoadCity calls CloseCity via vt+0x138 when a city is open; at the main
    // menu the placeholder city at +0xa0 makes that path crash (verified by
    // crash forensics). Refuse rather than crash; close via the UI first.
    const uint64_t city_before = *reinterpret_cast<const uint64_t*>(g_app + 0xa0);
    if (city_before != 0) {
        logf_("RPC load REFUSED: city already open (0x%llx) - CloseCity from "
              "this state crashes; close it via the UI first, then load",
              (unsigned long long)city_before);
        free(c);
        return;
    }
    // Construct the cRZString with the GAME's own ctor (its libc++ layout).
    static uint8_t str_buf[64];  // cRZString: vtable + std::string, 0x30 bytes
    memset(str_buf, 0, sizeof(str_buf));
    reinterpret_cast<void (*)(void*, const char*)>(kCRZStringFromChar + g_slide)(
        str_buf, c->path);
    const bool ok = reinterpret_cast<bool (*)(void*, const void*, void*)>(
        kLoadCity + g_slide)((void*)g_app, str_buf, nullptr);
    const uint64_t city = *reinterpret_cast<const uint64_t*>(g_app + 0xa0);
    const uint32_t state = *reinterpret_cast<const uint32_t*>(g_app + 0x78);
    logf_("RPC load \"%s\" -> %s (city=0x%llx appState=0x%x)",
          c->path, ok ? "OK" : "FAILED", (unsigned long long)city, state);
    free(c);
}

struct SaveCtx {
    char path[512];
};

void saveas_exec(void* p) {
    SaveCtx* c = reinterpret_cast<SaveCtx*>(p);
    static uint8_t str_buf[64];
    memset(str_buf, 0, sizeof(str_buf));
    reinterpret_cast<void (*)(void*, const char*)>(kCRZStringFromChar + g_slide)(
        str_buf, c->path);
    const bool ok = reinterpret_cast<bool (*)(void*, const void*, bool)>(
        kSaveCityPath + g_slide)((void*)g_app, str_buf, false);
    logf_("RPC saveas \"%s\" -> %s (full SaveCity @ 0x100046036)",
          c->path, ok ? "OK" : "FAILED");
    free(c);
}

bool is_game_vtable(uint64_t vptr) {
    // Real vtables live in the game's read-only __DATA_CONST region:
    // static range 0x100a60000..0x100b22000 (slide==0 observed; add slide
    // for generality).
    const uint64_t lo = 0x100a60000 + g_slide;
    const uint64_t hi = 0x100b22000 + g_slide;
    return vptr >= lo && vptr < hi;
}

// Validate a cSC4App-vptr candidate: GetMainWindow (+0x80) must deref to a
// real game vtable, and the cheat/debug managers (+0x3c0/+0x678) must be
// non-null heap pointers. False positives (scan-buffer copies, partial
// cSC4App-family objects) fail these derefs.
bool validate_app_candidate(uint64_t app) {
    if (app < 0x1000) return false;
    const uint64_t window = *reinterpret_cast<const uint64_t*>(app + 0x80);
    if (window < 0x1000) return false;
    const uint64_t window_vptr = *reinterpret_cast<const uint64_t*>(window);
    if (!is_game_vtable(window_vptr)) return false;
    const uint64_t cheat = *reinterpret_cast<const uint64_t*>(app + 0x3c0);
    if (cheat < 0x1000) return false;
    return true;
}

uint64_t find_app_instance() {
    // Scan all writable regions for the cSC4App vptr, validating candidates.
    int hits = 0;
    uint64_t result = 0;
    mach_vm_address_t addr = 0;
    mach_vm_size_t size = 0;
    while (hits < 8) {
        vm_region_basic_info_data_64_t info;
        mach_msg_type_number_t count = VM_REGION_BASIC_INFO_COUNT_64;
        mach_port_t obj_name = MACH_PORT_NULL;
        if (mach_vm_region(mach_task_self(), &addr, &size, VM_REGION_BASIC_INFO_64,
                           reinterpret_cast<vm_region_info_t>(&info), &count,
                           &obj_name) != KERN_SUCCESS) {
            break;
        }
        if ((info.protection & VM_PROT_WRITE) && size <= 512ull * 1024 * 1024) {
            static uint8_t buf[1024 * 1024];
            mach_vm_address_t cur = addr;
            const mach_vm_address_t end = addr + size;
            while (cur < end && hits < 8) {
                mach_vm_size_t chunk = 1024 * 1024;
                if (end - cur < chunk) chunk = end - cur;
                mach_vm_size_t got_bytes = 0;
                if (mach_vm_read_overwrite(mach_task_self(), cur, chunk,
                                           (mach_vm_address_t)buf, &got_bytes) == KERN_SUCCESS) {
                    for (mach_vm_size_t i = 0; i + 8 <= got_bytes && hits < 8; i += 8) {
                        uint64_t v;
                        memcpy(&v, buf + i, 8);
                        if (v == kAppVptr) {
                            hits++;
                            const uint64_t cand = cur + i;
                            // Skip our own scan buffer's stale copies: only
                            // accept hits outside our dylib's data (validated
                            // below anyway).
                            if (validate_app_candidate(cand) && result == 0) {
                                result = cand;
                            }
                        }
                    }
                }
                cur += chunk;
            }
        }
        addr += size;
        size = 0;
    }
    if (result == 0) {
        logf_("APP-SCAN: %d vptr hits, none passed validation "
              "(window-vtable + cheat-manager checks)", hits);
    } else {
        logf_("APP-SCAN: %d vptr hits, validated app = 0x%llx", hits,
              (unsigned long long)result);
    }
    return result;
}

// --- per-line command processing ----------------------------------------------
void process_command_line(const char* content) {
    char type[8] = {};
    char addr_hex[32] = {};
    int n = 0;
    if (strcmp(content, "ping") == 0) {
        logf_("RPC pong");
        return;
    }
    if (strcmp(content, "redump") == 0) {
        run_battery("rpc-redump");
        return;
    }
    if (strncmp(content, "readh ", 6) == 0) {
        // Heap/runtime address variant: no slide, no static-range guard.
        char rtype[8] = {};
        char raddr[32] = {};
        int rn = 0;
        if (sscanf(content, "readh %7s %31s %d", rtype, raddr, &rn) >= 2) {
            const uint64_t addr = strtoull(raddr, nullptr, 16);
            if (addr < 0x1000) {
                logf_("RPC readh error: bad address %s", raddr);
                return;
            }
            if (strcmp(rtype, "u64") == 0) {
                const uint64_t v = *reinterpret_cast<const uint64_t*>(addr);
                logf_("RPC readh u64 0x%llx = %llu (0x%llx)",
                      (unsigned long long)addr, (unsigned long long)v,
                      (unsigned long long)v);
            } else if (strcmp(rtype, "u32") == 0) {
                const uint32_t v = *reinterpret_cast<const uint32_t*>(addr);
                logf_("RPC readh u32 0x%llx = %u (0x%x)", (unsigned long long)addr, v, v);
            } else if (strcmp(rtype, "mem") == 0 && rn > 0 && rn <= 256) {
                const uint8_t* p = reinterpret_cast<const uint8_t*>(addr);
                char hex[256 * 2 + 16];
                size_t o = 0;
                for (int i = 0; i < rn; ++i) {
                    o += (size_t)snprintf(hex + o, sizeof(hex) - o, "%02x", p[i]);
                    if (i % 32 == 31 && i + 1 < rn) o += (size_t)snprintf(hex + o, sizeof(hex) - o, " | ");
                }
                logf_("RPC readh mem 0x%llx x%d: %s", (unsigned long long)addr, rn, hex);
            } else {
                logf_("RPC readh error: bad type '%s' (u64/u32/mem)", rtype);
            }
            return;
        }
        logf_("RPC readh error: usage 'readh u64|u32|mem <hex-addr> [n]'");
        return;
    }
    if (strncmp(content, "brushradius-prove ", 18) == 0) {
        char thex[32] = {};
        if (sscanf(content, "brushradius-prove %31s", thex) == 1) {
            const uint64_t terrain = strtoull(thex, nullptr, 16);
            const auto original =
                reinterpret_cast<float (*)(const void*, uint32_t)>(0x100734d6e + g_slide);
            uint32_t keys[128];
            int count = 0;
            const uint64_t root = *reinterpret_cast<const uint64_t*>(0x100b71e20 + g_slide);
            collect_map_keys(root, keys, &count, 128);
            int mismatches = 0;
            for (int i = 0; i < count; ++i) {
                const float orig = original((const void*)terrain, keys[i]);
                const float ours = brush_radius_shim((const void*)terrain, keys[i]);
                if (__builtin_memcmp(&orig, &ours, 4) != 0) {
                    mismatches++;
                    logf_("PHASE-B PROVE MISMATCH key=0x%x orig=%g ours=%g",
                          keys[i], (double)orig, (double)ours);
                }
            }
            // Missing-ID cases + a full report line.
            const uint32_t missing[] = {0x1u, 0x9999u, 0xffffffffu};
            for (uint32_t id : missing) {
                const float orig = original((const void*)terrain, id);
                const float ours = brush_radius_shim((const void*)terrain, id);
                if (__builtin_memcmp(&orig, &ours, 4) != 0) mismatches++;
            }
            logf_("PHASE-B PROVE BrushRadius: %d live keys + 3 missing ids, "
                  "%d mismatches (bit-exact float compare)",
                  count, mismatches);
        } else {
            logf_("RPC brushradius-prove error: usage 'brushradius-prove <terrain-hex>'");
        }
        return;
    }
    if (strcmp(content, "swap-brushradius") == 0) {
        // cSTETerrain vtable @ 0x100aea6b0, BrushRadius @ vt+0x1e8.
        const uint64_t slot_addr = 0x100aea6b0 + g_slide + 0x10 + 0x1e8;
        if (find_swap(slot_addr)) {
            logf_("RPC swap-brushradius error: slot already swapped");
            return;
        }
        int idx = -1;
        for (int i = 0; i < kMaxSwaps; ++i) {
            if (!g_swaps[i].active) { idx = i; break; }
        }
        if (idx < 0) {
            logf_("RPC swap-brushradius error: no free swap slots");
            return;
        }
        const uint64_t original = *reinterpret_cast<const uint64_t*>(slot_addr);
        g_swaps[idx] = SwapEntry{slot_addr, original, 0, true};
        if (patch_u64(slot_addr, reinterpret_cast<uint64_t>(&brush_radius_shim))) {
            logf_("PHASE-B SWAP: cSTETerrain::BrushRadius (slot 0x%llx) -> OUR shim "
                  "(orig 0x%llx). The game now executes our reconstruction.",
                  (unsigned long long)slot_addr, (unsigned long long)original);
        } else {
            g_swaps[idx].active = false;
            logf_("RPC swap-brushradius FAILED (mprotect)");
        }
        return;
    }
    if (strcmp(content, "prove-pollution-capacity") == 0) {
        dispatch_async_f(dispatch_get_main_queue(), nullptr, &pollution_capacity_prove_exec);
        return;
    }
    if (strcmp(content, "capture-loaded-capacity") == 0) {
        dispatch_async_f(dispatch_get_main_queue(), nullptr, &loaded_capacity_capture_exec);
        return;
    }
    if (strcmp(content, "capture-sint32") == 0) {
        dispatch_async_f(dispatch_get_main_queue(), nullptr, &sint32_capture_exec);
        return;
    }
    if (strcmp(content, "inspectloadedaginginputs") == 0) {
        logf_("RPC inspectloadedaginginputs queued=%u",
            unsigned(placement_reload::inspectloadedaginginputs()));
        return;
    }
    if (strcmp(content, "inspectplacedcheckpointreload") == 0) {
        logf_("RPC inspectplacedcheckpointreload queued=%u",
            unsigned(placement_reload::inspectplacedcheckpointreload()));
        return;
    }
    if (strcmp(content, "inspectplacementcheckpoint") == 0) {
        logf_("RPC inspectplacementcheckpoint queued=%u",
            unsigned(placement_checkpoint::inspectplacementcheckpoint()));
        return;
    }
    if (strcmp(content, "attemptplacementcheckpoint") == 0) {
        logf_("RPC attemptplacementcheckpoint queued=%u",
            unsigned(placement_checkpoint::attemptplacementcheckpoint()));
        return;
    }
    if (strcmp(content, "inspectcheckpointreload") == 0) {
        logf_("RPC inspectcheckpointreload queued=%u",
            unsigned(tutorial_reload::inspectcheckpointreload()));
        return;
    }
    if (strcmp(content, "inspectsavebindings") == 0) {
        logf_("RPC inspectsavebindings queued=%u",
            unsigned(tutorial_checkpoint::inspectsavebindings()));
        return;
    }
    if (strcmp(content, "finishtutorialcheckpoint") == 0) {
        logf_("RPC finishtutorialcheckpoint queued=%u",
            unsigned(tutorial_checkpoint::finishtutorialcheckpoint()));
        return;
    }
    if (strcmp(content, "inspecttutorialresponse") == 0) {
        logf_("RPC inspecttutorialresponse queued=%u",
            unsigned(tutorial_finish::inspecttutorialresponse()));
        return;
    }
    if (strcmp(content, "finishtutorial") == 0) {
        logf_("RPC finishtutorial queued=%u",
            unsigned(tutorial_finish::finishtutorial()));
        return;
    }
    if (strcmp(content, "opentutorialexit") == 0) {
        logf_("RPC opentutorialexit queued=%u",
            unsigned(tutorial_open::opentutorialexit()));
        return;
    }
    if (strcmp(content, "inspecttutorial") == 0) {
        logf_("RPC inspecttutorial queued=%u",
            unsigned(tutorial_inspect::inspecttutorial()));
        return;
    }
    if (strncmp(content, "inspectfixture", 14) == 0 &&
        (content[14] == 0 || content[14] == ' ' || content[14] == '\t')) {
        fixture_placement::Point points[32]; unsigned count = 0;
        if (!parse_fixture_points(content + 14, points, count)) {
            logf_("RPC inspectfixture error: require 1..32 complete int32 x y pairs"); return;
        }
        logf_("RPC inspectfixture queued=%u points=%u",
            unsigned(fixture_placement_bindings::inspectfixture(points,count)),count);
        return;
    }
    if (strncmp(content, "queryfixture", 12) == 0 &&
        (content[12] == 0 || content[12] == ' ' || content[12] == '\t')) {
        fixture_placement::Point points[32]; unsigned count = 0;
        if (!parse_fixture_points(content + 12, points, count) || count != 1) {
            logf_("RPC queryfixture error: require exactly one int32 x y pair"); return;
        }
        logf_("RPC queryfixture queued=%u",
            unsigned(fixture_query::queryfixture(points[0])));
        return;
    }
    if (strncmp(content, "attemptfixture", 14) == 0 &&
        (content[14] == 0 || content[14] == ' ' || content[14] == '\t')) {
        fixture_placement::Point points[32]; unsigned count = 0;
        if (!parse_fixture_points(content + 14, points, count) || count != 1) {
            logf_("RPC attemptfixture error: require exactly one int32 x y pair"); return;
        }
        logf_("RPC attemptfixture queued=%u",
            unsigned(fixture_placement_bindings::attemptfixture(points[0])));
        return;
    }
    if (strcmp(content, "prove-pollution-grid") == 0) {
        dispatch_async_f(dispatch_get_main_queue(), nullptr, &pollution_grid_prove_exec);
        return;
    }
    if (strncmp(content, "prove-cost ", 10) == 0 ||
        strncmp(content, "prove-cell ", 10) == 0 ||
        strncmp(content, "prove-altitude ", 14) == 0) {
        char thex[32] = {};
        ProveCtx* c = reinterpret_cast<ProveCtx*>(malloc(sizeof(ProveCtx)));
        memset(c, 0, sizeof(*c));
        bool ok = false;
        if (sscanf(content, "prove-cost %31s", thex) == 1) {
            snprintf(c->kind, sizeof(c->kind), "cost");
            ok = true;
        } else if (sscanf(content, "prove-cell %31s", thex) == 1) {
            snprintf(c->kind, sizeof(c->kind), "cell");
            ok = true;
        } else if (sscanf(content, "prove-altitude %31s", thex) == 1) {
            snprintf(c->kind, sizeof(c->kind), "altitude");
            ok = true;
        }
        if (!ok) {
            logf_("RPC prove error: usage 'prove-cost|prove-cell|prove-altitude "
                  "<terrain-hex>'");
            free(c);
            return;
        }
        c->terrain = strtoull(thex, nullptr, 16);
        dispatch_async_f(dispatch_get_main_queue(), c, &phaseb_prove_exec);
        return;
    }
    if (strncmp(content, "phaseb-swap ", 12) == 0) {
        char name[32] = {};
        if (sscanf(content, "phaseb-swap %31s", name) == 1) {
            struct Target {
                const char* name;
                uint64_t off;
                const void* fn;
            };
            static const Target targets[] = {
                {"brushradius", 0x1e8, (const void*)&pb_brushradius_counted},
                {"cost", 0x1f0, (const void*)&pb_cost_counted},
                {"water", 0x110, (const void*)&pb_water_counted},
                {"land", 0x118, (const void*)&pb_land_counted},
                {"altitude", 0x060, (const void*)&pb_altitude_counted},
            };
            int which = -1;
            for (int i = 0; i < kPhaseBTargets; ++i) {
                if (strcmp(name, targets[i].name) == 0) which = i;
            }
            if (which < 0) {
                logf_("RPC phaseb-swap error: unknown target '%s' "
                      "(brushradius|cost|water|land|altitude)", name);
                return;
            }
            const uint64_t slot_addr =
                0x100aea6b0 + g_slide + 0x10 + targets[which].off;
            if (find_swap(slot_addr)) {
                logf_("RPC phaseb-swap error: slot 0x%llx already swapped",
                      (unsigned long long)slot_addr);
                return;
            }
            int idx = -1;
            for (int i = 0; i < kMaxSwaps; ++i) {
                if (!g_swaps[i].active) { idx = i; break; }
            }
            if (idx < 0) {
                logf_("RPC phaseb-swap error: no free swap slots");
                return;
            }
            const uint64_t original =
                *reinterpret_cast<const uint64_t*>(slot_addr);
            g_swaps[idx] = SwapEntry{slot_addr, original, 0, true};
            if (patch_u64(slot_addr,
                          reinterpret_cast<uint64_t>(targets[which].fn))) {
                logf_("PHASE-B SWAP: cSTETerrain::%s (slot 0x%llx, "
                      "vt+%llx) -> OUR counting shim (orig 0x%llx). The game "
                      "now executes our reconstruction.",
                      kPBNames[which], (unsigned long long)slot_addr,
                      (unsigned long long)targets[which].off,
                      (unsigned long long)original);
            } else {
                g_swaps[idx].active = false;
                logf_("RPC phaseb-swap FAILED (mprotect)");
            }
        }
        return;
    }
    if (strcmp(content, "phaseb-stats") == 0) {
        for (int i = 0; i < kPhaseBTargets; ++i) {
            logf_("PHASE-B COUNT %s = %llu",
                  kPBNames[i], (unsigned long long)g_pb_counts[i]);
        }
        return;
    }
    if (strncmp(content, "vtread ", 7) == 0) {
        char vaddr[32] = {};
        char voff[16] = {};
        if (sscanf(content, "vtread %31s %15s", vaddr, voff) == 2) {
            const uint64_t vt = strtoull(vaddr, nullptr, 16);
            const uint64_t off = strtoull(voff, nullptr, 0);
            if (!addr_valid(vt)) {
                logf_("RPC vtread error: bad static address %s", vaddr);
                return;
            }
            const uint64_t slot = *reinterpret_cast<const uint64_t*>(vt + g_slide + 0x10 + off);
            logf_("RPC vtread 0x%llx+0x%llx -> 0x%llx", (unsigned long long)vt,
                  (unsigned long long)off, (unsigned long long)slot);
        } else {
            logf_("RPC vtread error: usage 'vtread <vtable-static-hex> <slot-off-hex>'");
        }
        return;
    }
    if (strncmp(content, "swap ", 5) == 0) {
        char vaddr[32] = {};
        char voff[16] = {};
        if (sscanf(content, "swap %31s %15s", vaddr, voff) == 2) {
            const uint64_t vt = strtoull(vaddr, nullptr, 16);
            const uint64_t off = strtoull(voff, nullptr, 0);
            if (!addr_valid(vt)) {
                logf_("RPC swap error: bad static address %s", vaddr);
                return;
            }
            const uint64_t slot_addr = vt + g_slide + 0x10 + off;
            if (find_swap(slot_addr)) {
                logf_("RPC swap error: slot 0x%llx already swapped", (unsigned long long)slot_addr);
                return;
            }
            int idx = -1;
            for (int i = 0; i < kMaxSwaps; ++i) {
                if (!g_swaps[i].active) { idx = i; break; }
            }
            if (idx < 0) {
                logf_("RPC swap error: all %d swap slots in use (unswap first)", kMaxSwaps);
                return;
            }
            const uint64_t original = *reinterpret_cast<const uint64_t*>(slot_addr);
            g_swaps[idx] = SwapEntry{slot_addr, original, 0, true};
            if (patch_u64(slot_addr, reinterpret_cast<uint64_t>(k_swap_thunks[idx]))) {
                logf_("RPC swap slot=0x%llx (vt 0x%llx+0x%llx) idx=%d orig=0x%llx -> thunk",
                      (unsigned long long)slot_addr, (unsigned long long)vt,
                      (unsigned long long)off, idx, (unsigned long long)original);
            } else {
                g_swaps[idx].active = false;
                logf_("RPC swap FAILED (mprotect) slot=0x%llx", (unsigned long long)slot_addr);
            }
        } else {
            logf_("RPC swap error: usage 'swap <vtable-static-hex> <slot-off-hex>'");
        }
        return;
    }
    if (strncmp(content, "unswap ", 7) == 0) {
        char vaddr[32] = {};
        char voff[16] = {};
        if (sscanf(content, "unswap %31s %15s", vaddr, voff) == 2) {
            const uint64_t vt = strtoull(vaddr, nullptr, 16);
            const uint64_t off = strtoull(voff, nullptr, 0);
            const uint64_t slot_addr = vt + g_slide + 0x10 + off;
            SwapEntry* e = find_swap(slot_addr);
            if (!e) {
                logf_("RPC unswap error: slot 0x%llx not swapped", (unsigned long long)slot_addr);
                return;
            }
            const int idx = static_cast<int>(e - g_swaps);
            if (patch_u64(slot_addr, e->original)) {
                logf_("RPC unswap slot=0x%llx idx=%d restored 0x%llx (count=%llu)",
                      (unsigned long long)slot_addr, idx, (unsigned long long)e->original,
                      (unsigned long long)e->counter);
                e->active = false;
            } else {
                logf_("RPC unswap FAILED (mprotect) slot=0x%llx", (unsigned long long)slot_addr);
            }
        } else {
            logf_("RPC unswap error: usage 'unswap <vtable-static-hex> <slot-off-hex>'");
        }
        return;
    }
    if (strcmp(content, "swapstats") == 0) {
        for (int i = 0; i < kMaxSwaps; ++i) {
            if (g_swaps[i].active) {
                logf_("RPC swapstat idx=%d slot=0x%llx orig=0x%llx count=%llu", i,
                      (unsigned long long)g_swaps[i].slot_addr,
                      (unsigned long long)g_swaps[i].original,
                      (unsigned long long)g_swaps[i].counter);
            }
        }
        return;
    }
    if (strncmp(content, "app", 3) == 0 &&
        (content[3] == '\0' || content[3] == ' ')) {
        g_app = get_app();
        logf_("RPC app = 0x%llx", (unsigned long long)g_app);
        return;
    }
    if (strncmp(content, "state", 5) == 0 &&
        (content[5] == '\0' || content[5] == ' ')) {
        if (g_app == 0) g_app = get_app();
        if (g_app == 0) {
            logf_("RPC state error: cSC4App not found");
            return;
        }
        const uint32_t state = *reinterpret_cast<const uint32_t*>(g_app + 0x78);
        const uint64_t city = *reinterpret_cast<const uint64_t*>(g_app + 0xa0);
        logf_("RPC state appState=0x%x city=0x%llx", state, (unsigned long long)city);
        return;
    }
    if (strncmp(content, "load ", 5) == 0) {
        if (g_app == 0) g_app = get_app();
        if (g_app == 0) {
            logf_("RPC load error: cSC4App not found");
            return;
        }
        LoadCtx* c = reinterpret_cast<LoadCtx*>(malloc(sizeof(LoadCtx)));
        memset(c, 0, sizeof(*c));
        snprintf(c->path, sizeof(c->path), "%s", content + 5);
        dispatch_async_f(dispatch_get_main_queue(), c, &load_exec);
        return;
    }
    if (strncmp(content, "saveas ", 7) == 0) {
        if (g_app == 0) g_app = get_app();
        if (g_app == 0) {
            logf_("RPC saveas error: cSC4App not found");
            return;
        }
        SaveCtx* c = reinterpret_cast<SaveCtx*>(malloc(sizeof(SaveCtx)));
        memset(c, 0, sizeof(*c));
        snprintf(c->path, sizeof(c->path), "%s", content + 7);
        dispatch_async_f(dispatch_get_main_queue(), c, &saveas_exec);
        return;
    }
    if (strcmp(content, "regions") == 0) {
        dispatch_async_f(dispatch_get_main_queue(), nullptr, &regions_exec);
        return;
    }
    if (strcmp(content, "region-view") == 0) {
        dispatch_async_f(dispatch_get_main_queue(), nullptr, &region_view_exec);
        return;
    }
    if (strcmp(content, "cities") == 0) {
        dispatch_async_f(dispatch_get_main_queue(), nullptr, &cities_exec);
        return;
    }
    if (strncmp(content, "region ", 7) == 0) {
        StrArgCtx* c = reinterpret_cast<StrArgCtx*>(malloc(sizeof(StrArgCtx)));
        memset(c, 0, sizeof(*c));
        snprintf(c->arg, sizeof(c->arg), "%s", content + 7);
        dispatch_async_f(dispatch_get_main_queue(), c, &region_load_exec);
        return;
    }
    if (strncmp(content, "launch-city ", 12) == 0) {
        StrArgCtx* c = reinterpret_cast<StrArgCtx*>(malloc(sizeof(StrArgCtx)));
        memset(c, 0, sizeof(*c));
        snprintf(c->arg, sizeof(c->arg), "%s", content + 12);
        dispatch_async_f(dispatch_get_main_queue(), c, &launch_city_exec);
        return;
    }
    if (strncmp(content, "sim ", 4) == 0) {
        StrArgCtx* c = reinterpret_cast<StrArgCtx*>(malloc(sizeof(StrArgCtx)));
        memset(c, 0, sizeof(*c));
        snprintf(c->arg, sizeof(c->arg), "%s", content + 4);
        dispatch_async_f(dispatch_get_main_queue(), c, &sim_cmd_exec);
        return;
    }
    if (strncmp(content, "write ", 6) == 0) {
        char wtype[8] = {};
        char waddr[32] = {};
        char wval[64] = {};
        if (sscanf(content, "write %7s %31s %63s", wtype, waddr, wval) != 3) {
            logf_("RPC write error: usage 'write u32|u64|f32 <addr> <val>'");
            return;
        }
        // 'write' takes static addresses (slide applied); 'writeh' takes raw
        // runtime/heap addresses. Type carries the variant.
        bool heap = false;
        char t[8];
        snprintf(t, sizeof(t), "%s", wtype);
        if (strlen(t) > 1 && t[strlen(t)-1] == 'h') { heap = true; t[strlen(t)-1] = '\0'; }
        const uint64_t addr_num = strtoull(waddr, nullptr, 16);
        uint64_t runtime;
        if (heap) {
            if (addr_num < 0x1000) { logf_("RPC write error: bad address"); return; }
            runtime = addr_num;
        } else {
            if (!addr_valid(addr_num)) {
                logf_("RPC write error: bad static address %s", waddr);
                return;
            }
            runtime = addr_num + g_slide;
        }
        if (strcmp(t, "u32") == 0) {
            *reinterpret_cast<uint32_t*>(runtime) = (uint32_t)strtoull(wval, nullptr, 0);
            logf_("RPC write%s u32 0x%llx = %s", heap ? "h" : "",
                  (unsigned long long)addr_num, wval);
        } else if (strcmp(t, "u64") == 0) {
            *reinterpret_cast<uint64_t*>(runtime) = strtoull(wval, nullptr, 0);
            logf_("RPC write%s u64 0x%llx = %s", heap ? "h" : "",
                  (unsigned long long)addr_num, wval);
        } else if (strcmp(t, "f32") == 0) {
            *reinterpret_cast<float*>(runtime) = strtof(wval, nullptr);
            logf_("RPC write%s f32 0x%llx = %s", heap ? "h" : "",
                  (unsigned long long)addr_num, wval);
        } else {
            logf_("RPC write error: bad type '%s'", wtype);
        }
        return;
    }
    if (strncmp(content, "find ", 5) == 0) {
        char rest[64] = {};
        sscanf(content, "find %63[^\n]", rest);
        const uint64_t needle = strtoull(rest, nullptr, 0);
        int max_hits = 64;
        const char* sp = strchr(rest, ' ');
        if (sp) max_hits = (int)strtoul(sp, nullptr, 0);
        if (max_hits < 1) max_hits = 1;
        if (needle == 0) {
            logf_("RPC find error: needle must be nonzero");
        } else {
            scan_for(needle, max_hits);
        }
        return;
    }
    if (strncmp(content, "call ", 5) == 0) {
        // call <static-addr-hex> [arg-hex ...]  (up to 6 integer args)
        char args_str[256];
        const int got = sscanf(content, "call %31s %255[^\n]", addr_hex, args_str);
        const uint64_t static_addr = strtoull(addr_hex, nullptr, 16);
        if (!addr_valid(static_addr)) {
            logf_("RPC call error: bad static address %s", addr_hex);
            return;
        }
        CallCtx* c = reinterpret_cast<CallCtx*>(malloc(sizeof(CallCtx)));
        memset(c, 0, sizeof(*c));  // unfilled arg slots MUST be 0, not malloc garbage
        c->fn = static_addr + g_slide;
        c->argc = 0;
        if (got >= 2) {
            char* save = nullptr;
            for (char* tok = strtok_r(args_str, " \t", &save);
                 tok != nullptr && c->argc < 6; tok = strtok_r(nullptr, " \t", &save)) {
                c->args[c->argc++] = strtoull(tok, nullptr, 0);
            }
        }
        dispatch_async_f(dispatch_get_main_queue(), c, &call_exec);
        return;
    }
    if (sscanf(content, "read %7s %31s %d", type, addr_hex, &n) >= 2) {
        const uint64_t static_addr = strtoull(addr_hex, nullptr, 16);
        if (!addr_valid(static_addr)) {
            logf_("RPC read error: bad static address %s", addr_hex);
            return;
        }
        const uint64_t runtime = static_addr + g_slide;
        if (strcmp(type, "f32") == 0) {
            const float v = *reinterpret_cast<const float*>(runtime);
            const uint32_t bits = *reinterpret_cast<const uint32_t*>(runtime);
            logf_("RPC read f32 0x%llx = %g (bits 0x%08x)",
                  (unsigned long long)static_addr, (double)v, bits);
        } else if (strcmp(type, "f64") == 0) {
            const double v = *reinterpret_cast<const double*>(runtime);
            logf_("RPC read f64 0x%llx = %g", (unsigned long long)static_addr, v);
        } else if (strcmp(type, "u32") == 0 || strcmp(type, "i32") == 0) {
            const uint32_t v = *reinterpret_cast<const uint32_t*>(runtime);
            logf_("RPC read %s 0x%llx = %u (0x%x)", type,
                  (unsigned long long)static_addr, v, v);
        } else if (strcmp(type, "u64") == 0) {
            const uint64_t v = *reinterpret_cast<const uint64_t*>(runtime);
            logf_("RPC read u64 0x%llx = %llu (0x%llx)",
                  (unsigned long long)static_addr,
                  (unsigned long long)v, (unsigned long long)v);
        } else if (strcmp(type, "mem") == 0 && n > 0 && n <= 256) {
            const uint8_t* p = reinterpret_cast<const uint8_t*>(runtime);
            char hex[256 * 2 + 16];
            size_t o = 0;
            for (int i = 0; i < n; ++i) {
                o += (size_t)snprintf(hex + o, sizeof(hex) - o, "%02x", p[i]);
                if (i % 32 == 31 && i + 1 < n) o += (size_t)snprintf(hex + o, sizeof(hex) - o, " | ");
            }
            logf_("RPC read mem 0x%llx x%d: %s",
                  (unsigned long long)static_addr, n, hex);
        } else {
            logf_("RPC read error: bad type '%s'", type);
        }
        return;
    }
    logf_("RPC error: unrecognized command: %s", content);
}

bool read_command_file() {
    const int fd = open(g_cmd_path, O_RDONLY);
    if (fd < 0) {
        // Rate-limited diagnostics: pinpoint whether it's this path, the
        // directory, or ALL file opens failing in this process.
        static int err_ticks = 0;
        if ((err_ticks++ % 200) == 0) {  // ~once per 20 s
            const int e_cmd = errno;
            const int probe_root = open("/", O_RDONLY);
            const int e_root = probe_root >= 0 ? 0 : errno;
            if (probe_root >= 0) close(probe_root);
            const int probe_dir = open("/Users/russellmartin/.opensc4-oracle", O_RDONLY);
            const int e_dir = probe_dir >= 0 ? 0 : errno;
            if (probe_dir >= 0) close(probe_dir);
            const int probe_hard = open("/Users/russellmartin/.opensc4-oracle/command", O_RDONLY);
            const int e_hard = probe_hard >= 0 ? 0 : errno;
            if (probe_hard >= 0) close(probe_hard);
            logf_("RPC open(%s) failed: errno=%d | probes: / ->%d, dir ->%d, hardcoded ->%d",
                  g_cmd_path, e_cmd, e_root, e_dir, e_hard);
        }
        return false;
    }
    char buf[4096];
    char pending[4096];
    size_t pending_len = 0;
    ssize_t got;
    while ((got = read(fd, buf, sizeof(buf))) > 0) {
        for (ssize_t i = 0; i < got; ++i) {
            if (buf[i] == '\n') {
                if (pending_len > 0) {
                    pending[pending_len] = '\0';
                    process_command_line(pending);
                    pending_len = 0;
                }
            } else if (pending_len + 1 < sizeof(pending)) {
                pending[pending_len++] = buf[i];
            }
        }
    }
    close(fd);
    if (truncate(g_cmd_path, 0) != 0) {
        logf_("RPC truncate failed: errno=%d", errno);
        return false;
    }
    return pending_len > 0;  // incomplete trailing line: caller rewrites it
}

bool am_command_owner() {
    // The newest instance writes its pid to <dir>/owner at startup. A stale
    // instance (old dylib, old app state, pre-restart heap) must not answer
    // commands on the shared file - it raced twice during testing and replied
    // with stale addresses and old handlers.
    if (g_dir_path[0] == '\0') return false;
    char owner_path[512];
    snprintf(owner_path, sizeof(owner_path), "%s/owner", g_dir_path);
    const int fd = open(owner_path, O_RDONLY);
    if (fd < 0) return true;  // no claim file: assume we own it
    char buf[32] = {};
    const ssize_t n = read(fd, buf, sizeof(buf) - 1);
    close(fd);
    if (n <= 0) return true;
    const long owner = strtol(buf, nullptr, 10);
    return owner == 0 || owner == (long)getpid();
}

void* rpc_thread(void*) {
    int battery_countdown = 300;  // ~30s auto-redump after game init
    while (true) {
        struct timespec ts{0, 100 * 1000 * 1000};  // 100ms
        nanosleep(&ts, nullptr);
        if (am_command_owner()) {
            read_command_file();
        }
        if (battery_countdown > 0 && --battery_countdown == 0) {
            run_battery("auto+30s");
        }
    }
    return nullptr;
}

const char* kEnvHome = "HOME";

// --- DYLD interposition: skip the native intro movies ------------------------
// The Mac port plays GameData/Video/{EA_logo,Aspyr,SC4Intro}.mov via
// NCT_NewIntroMovieSequence @ 0x100003147 (called unconditionally from _main
// @ 0x100002c01, before the game core's command line exists - the -intro:off
// switch and the "intro-movie" preference only gate the CORE's intro).
// Interposing open/fopen to fail those paths makes CQTPlayer::Open fail and
// the sequence falls through to the menu. No app-bundle modification.
static bool g_allow_movies = false;  // set from env; A/B kill-switch for the
                                     // interposition (black-window diagnosis)

static bool is_intro_movie(const char* path) {
    if (g_allow_movies) return false;
    if (path == nullptr) return false;
    const char* v = strstr(path, "GameData/Video/");
    return v != nullptr && strstr(v, ".mov") != nullptr;
}

extern "C" int opensc4_interposed_open(const char* path, int oflag, ...) {
    if (is_intro_movie(path)) {
        errno = ENOENT;
        return -1;
    }
    va_list ap;
    va_start(ap, oflag);
    const int mode = va_arg(ap, int);
    va_end(ap);
    return open(path, oflag, mode);  // self-calls are NOT interposed
}

extern "C" FILE* opensc4_interposed_fopen(const char* path, const char* mode) {
    if (is_intro_movie(path)) {
        errno = ENOENT;
        return nullptr;
    }
    return fopen(path, mode);
}

struct interpose_pair {
    const void* replacement;
    const void* original;
};
static const interpose_pair k_interposers[]
    __attribute__((used, section("__DATA,__interpose"))) = {
        {(const void*)opensc4_interposed_open, (const void*)open},
        {(const void*)opensc4_interposed_fopen, (const void*)fopen},
};

}  // namespace

__attribute__((constructor)) static void opensc4_oracle_init() {
    // "1" (or any non-empty value other than "0") plays the native intro
    // movies; the default (unset/0) skips them via the binary's own flag.
    const char* allow_env = getenv("OPENSC4_ORACLE_ALLOW_MOVIES");
    g_allow_movies =
        allow_env != nullptr && allow_env[0] != '\0' && strcmp(allow_env, "0") != 0;
    const char* home = getenv(kEnvHome);
    if (!home) return;
    char dir[512];
    snprintf(dir, sizeof(dir), "%s/.opensc4-oracle", home);
    mkdir(dir, 0755);
    snprintf(g_dir_path, sizeof(g_dir_path), "%s", dir);
    snprintf(g_log_path, sizeof(g_log_path), "%s/oracle.log", dir);
    logfd_reopen();
    if (g_log_fd < 0) return;
    snprintf(g_cmd_path, sizeof(g_cmd_path), "%s/command", dir);
    // Ownership claim: only the NEWEST game instance processes commands.
    // Without this, a stale instance (old dylib, old app state) races on the
    // shared command file and answers with stale addresses - bit us twice.
    char owner_path[512];
    snprintf(owner_path, sizeof(owner_path), "%s/owner", dir);
    const int ofd = open(owner_path, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (ofd >= 0) {
        char pidbuf[32];
        const int plen = snprintf(pidbuf, sizeof(pidbuf), "%d\n", (int)getpid());
        ssize_t wr = write(ofd, pidbuf, (size_t)plen);
        (void)wr;
        close(ofd);
    }
    // Truncate any stale command file at startup (hold no fd on it: the
    // engine may exhaust or close our descriptors mid-init; we reopen on
    // demand and the held log fd is verified before each write batch).
    const int cfd = open(g_cmd_path, O_WRONLY | O_TRUNC | O_CREAT, 0644);
    if (cfd >= 0) {
        close(cfd);
        logf_("RPC command file created ok (fd=%d)", cfd);
    } else {
        logf_("RPC command file create FAILED: errno=%d", errno);
    }

    // Find the main executable image and its PIE slide. Static addresses
    // in our tooling are file/disassembly addresses (vmaddr from 0x100000000);
    // runtime = static + slide.
    const struct mach_header* mh = _dyld_get_image_header(0);  // main executable
    g_slide = static_cast<uint64_t>(
        reinterpret_cast<uintptr_t>(mh) - kStaticLoadBase);
    logf_("ORACLE-UP pid=%d image_header=%p slide=0x%llx static_base=0x%llx",
          static_cast<int>(getpid()), (const void*)mh,
          (unsigned long long)g_slide, (unsigned long long)kStaticLoadBase);
    if (mh->magic != MH_MAGIC_64) {
        logf_("ORACLE-ERROR header at image[0] is not MH_MAGIC_64 (got 0x%x) - "
              "slide/base assumptions broken; refusing battery",
              mh->magic);
        return;
    }

    // Skip the native intro movies with the binary's OWN switch:
    // CQTPlayer::mbsNCTSkipMovie @ 0x100b713b1. Honored by CQTPlayer::Open
    // (@ 0x100724247: skips the AVPlayerLayer attach), CQTPlayer::Play
    // (@ 0x100724468: marks started, returns immediately), and
    // CQTPlayer::IsPlaying (@ 0x10072461e: returns 0) - so
    // NCT_NewIntroMovieSequence @ 0x100003147 tears through all three
    // GameData/Video/*.mov files in milliseconds and runs its normal window
    // teardown, and SC4AppStart @ 0x100003930 populates SL::spSC4 right away.
    // Constructor time is early enough: _main (which calls the sequence) runs
    // after all image constructors. Read-modify-write the full qword so the
    // adjacent statics (0x100b713b2..) keep their on-disk values.
    if (!g_allow_movies) {
        const uint64_t skip_addr = kSkipMovieFlag + g_slide;
        uint64_t qword;
        memcpy(&qword, reinterpret_cast<const void*>(skip_addr), 8);
        reinterpret_cast<uint8_t*>(&qword)[0] = 1;
        if (!patch_u64(skip_addr, qword)) {
            // Plain __DATA may already be RW; try the direct write.
            *reinterpret_cast<uint8_t*>(skip_addr) = 1;
        }
        logf_("ORACLE skip-movie flag @ 0x%llx -> %d (readback)",
              (unsigned long long)kSkipMovieFlag,
              (int)*reinterpret_cast<const uint8_t*>(skip_addr));
    }
    run_battery("constructor");
    pthread_t t;
    if (pthread_create(&t, nullptr, rpc_thread, nullptr) == 0) {
        pthread_detach(t);
        logf_("RPC-READY command file: %s", g_cmd_path);
    } else {
        logf_("ORACLE-ERROR rpc thread failed to start");
    }
}
