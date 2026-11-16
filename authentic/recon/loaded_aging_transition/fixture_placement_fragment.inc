// clean-room fallback (not authentic): bounded diagnostic policies, not game semantics.
// Include at namespace scope after logf_, g_slide, get_app. C++17 x86_64 only.
#ifndef OPENSC4_FIXTURE_PLACEMENT_FRAGMENT_INC
#define OPENSC4_FIXTURE_PLACEMENT_FRAGMENT_INC
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <mach-o/dyld.h>
#include <CommonCrypto/CommonDigest.h>
#include <pthread.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <climits>
namespace fixture_placement {
using U = unsigned long long;
constexpr char image_sha[] = "0d7a18ffee1d2abbdc5f58d364a7ef515b093fb7f85fa09840f8f022a8874224";
static bool read(uint64_t p, void* out, size_t n) {
    if (!out || !n || n > 65536 || p < 0x1000 || p > UINT64_MAX - n) return false;
    mach_vm_size_t got = 0;
    return mach_vm_read_overwrite(mach_task_self(), p, n,
        reinterpret_cast<mach_vm_address_t>(out), &got) == KERN_SUCCESS && got == n;
}
template<class T> static bool read(uint64_t p, T& out) { return read(p, &out, sizeof out); }
static bool span(uint64_t p, size_t n) {
    unsigned char b[256];
    if (!n || n > 65536 || p > UINT64_MAX - n) return false;
    while (n) { size_t k = n < sizeof b ? n : sizeof b;
        if (!read(p, b, k)) return false;
        p += k; n -= k;
    }
    return true;
}
static uint64_t relocated(uint64_t a) {
    // dyld slide is signed; avoid overflow rather than guessing an address.
    const int64_t s = int64_t(g_slide);
    if (s >= 0) return a <= UINT64_MAX - uint64_t(s) ? a + uint64_t(s) : 0;
    const uint64_t magnitude = uint64_t(-(s + 1)) + 1;
    return a >= magnitude ? a - magnitude : 0;
}
static bool object(uint64_t p, uint64_t vt, size_t n) {
    uint64_t actual = 0;
    return read(p, actual) && actual == relocated(vt) && span(p, n);
}
static bool slot(uint64_t p, unsigned off, uint64_t target) {
    uint64_t vt = 0, fn = 0;
    return read(p, vt) && vt <= UINT64_MAX - off && read(vt + off, fn) && fn == relocated(target);
}
template<class F> static F original(uint64_t a) { return reinterpret_cast<F>(relocated(a)); }
struct Entry { uint64_t address; unsigned char bytes[16]; };
// Fresh original-image byte reads (2026-11-14); whole-file hash below is additional.
static constexpr Entry entries[] = {
    {0x10078410a,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec}},
    {0x100782390,{0x55,0x48,0x89,0xe5,0x48,0x8b,0x87,0xf0,0x01,0,0,0x48,0x39,0x87,0xe8,0x01}},
    {0x10078292c,{0x55,0x48,0x89,0xe5,0x48,0x8b,0xbf,0x88,0x01,0,0,0x48,0x85,0xff,0x74,0x14}},
    {0x1003c0ca4,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x81,0xec}},
    {0x1003bed24,{0x55,0x48,0x89,0xe5,0x41,0x56,0x53,0x48,0x83,0xec,0x20,0x49,0x89,0xfe,0x49,0x8b}},
    {0x1003bdcf0,{0x55,0x48,0x89,0xe5,0x8b,0x47,0x0c,0xff,0xc0,0x89,0x47,0x0c,0x5d,0xc3,0x55,0x48}},
    {0x1003bdd0c,{0x8b,0x47,0x0c,0x83,0xf8,0x02,0x72,0x06,0xff,0xc8,0x89,0x47,0x0c,0xc3,0x55,0x48}},
    {0x1001f19d8,{0x55,0x48,0x89,0xe5,0x48,0x81,0xc7,0,0x01,0,0,0x5d,0xe9,0x91,0xd3,0xe4}}
};
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
static bool image_guard() {
    if (int64_t(g_slide) != _dyld_get_image_vmaddr_slide(0)) return false;
    const char* name = _dyld_get_image_name(0);
    if (!name) return false;
    FILE* f = std::fopen(name, "rb");
    if (!f) return false;
    CC_SHA256_CTX ctx; CC_SHA256_Init(&ctx);
    unsigned char b[16384], digest[CC_SHA256_DIGEST_LENGTH]; size_t total = 0, n = 0;
    bool ok = true;
    while ((n = std::fread(b, 1, sizeof b, f)) != 0) {
        total += n;
        if (total > 128u * 1024u * 1024u) { ok = false; break; }
        CC_SHA256_Update(&ctx, b, CC_LONG(n));
    }
    ok = ok && !std::ferror(f); std::fclose(f); CC_SHA256_Final(digest, &ctx);
    char hex[65] = {}; static constexpr char digits[] = "0123456789abcdef";
    for (unsigned i = 0; i < sizeof digest; ++i) {
        hex[2*i] = digits[digest[i] >> 4]; hex[2*i+1] = digits[digest[i] & 15];
    }
    if (!ok || std::strcmp(hex, image_sha)) return false;
    for (const Entry& e : entries) {
        unsigned char live[16];
        if (!read(relocated(e.address), live, sizeof live) || std::memcmp(live, e.bytes, sizeof live)) return false;
    }
    return true;
}
#pragma clang diagnostic pop
struct Point { int32_t x, y; };
struct Rect { int32_t l, t, r, b; };
struct State {
    uint64_t app = 0, city = 0, sim = 0, view = 0, renderer = 0;
    uint32_t date = 0; int32_t pauses[3] = {}; Rect area{}, absolute{};
};
// REQUIRED conductor adapters. They are read-only, bounded, synchronous, main-thread
// callbacks. No function/context/game pointer is retained after exec returns.
// identity must revalidate the verified loaded city's path/manifest binding each call.
// services must validate/log ALL catalog/move/click dependencies, window hierarchy,
// renderer PickTerrain target/prologue, established city; control!=0 additionally
// requires actual descriptor resolution and its complete guarded chain.
// inventory must log complete building/lot/TGI/position/bounds and monthly C0/D8/108
// reference count/order/identities, including absence; false means incomplete.
struct Dependencies {
    bool (*identity)(void*, const State&) = nullptr;
    bool (*services)(void*, const State&, uint64_t control) = nullptr;
    bool (*inventory)(void*, const State&, const char* phase) = nullptr;
};
struct Context {
    Dependencies deps{}; void* owner = nullptr;
    uint64_t verified_city = 0; // conductor-owned binding to independently verified load
    char verified_path[1024] = {}; // canonical prepared-manifest path, never a config ID
    char manifest_sha256[65] = {};
    bool attempt = false;
    unsigned point_count = 0; Point points[32] = {}; // inspect only, conductor supplied
    Point target{}; // attempt only: exactly the same x/y passed to move and click
    State inspection_state{}; // copied receipt, revalidated; owns no game references
    bool inspection_complete = false, complete = false;
    bool inspection_valid[32] = {}; // each original terrain pick's result
    unsigned catalog_calls = 0, move_calls = 0, click_calls = 0;
    bool consumed = false; // a Context is single use, including ordinary failures
};
static bool state(State& s) {
    uint32_t app_state = 0; uint64_t global_city = 0;
    s.app = get_app();
    if (!object(s.app, 0x100a67010, 0xa8) || !read(s.app + 0x78, app_state) || app_state != 2 ||
        !read(s.app + 0xa0, s.city) || !object(s.city, 0x100a692e0, 0x2e0) ||
        !read(relocated(0x100b21308), global_city) || global_city != s.city ||
        !read(s.city + 0x258, s.sim) || !object(s.sim, 0x100a841e0, 0x118) ||
        !slot(s.sim, 0x40, 0x1001f19d8) || !read(s.sim + 0x48, s.pauses) || s.pauses[0] <= 0 ||
        !read(relocated(0x100b214b8), s.view) || !object(s.view, 0x100aec7b0, 0x220) ||
        !object(s.view + 0x18, 0x100aecaa0, 0x130) ||
        !slot(s.view, 0xc0, 0x100782390) || !slot(s.view, 0x120, 0x10078292c) ||
        !slot(s.view + 0x18, 0x148, 0x10016c7e2) || !slot(s.view + 0x18, 0x178, 0x10016c966) ||
        !slot(s.view + 0x18, 0x188, 0x10016c974) ||
        !read(s.view + 0x188, s.renderer) || !span(s.renderer, 8) ||
        !read(s.view + 0x18 + 0x120, s.area) || !read(s.view + 0x18 + 0x20, s.absolute)) return false;
    // GetArea / GetW @ 0x10016c966/0x10016c7e2; IGZWin is primary+0x18.
    // Mouse dispatch @ 0x100783786 forwards client x/y; PickTerrain @ 0x10078292c
    // forwards ESI/EDX unchanged to renderer, RCX=float[3], R8b=false.
    const int64_t w = int64_t(s.area.r) - s.area.l, h = int64_t(s.area.b) - s.area.t;
    if (w <= 0 || h <= 0 || w > INT_MAX || h > INT_MAX) return false;
    // IsPaused @ 0x1001f19ea is signed count48>0; stronger policy requires explicit pause.
    s.date = original<uint32_t(*)(uint64_t)>(0x1001f19d8)(s.sim);
    uint64_t renderer_vptr=0;
    if (!read(s.renderer,renderer_vptr)) return false;
    logf_("FIXTURE VPTR city=%llx sim=%llx view=%llx win=%llx renderer=%llx",
        U(relocated(0x100a692e0)),U(relocated(0x100a841e0)),U(relocated(0x100aec7b0)),U(relocated(0x100aecaa0)),U(renderer_vptr));
    logf_("FIXTURE STATE city=%llx sim=%llx date=%u pause=%d,%d,%d view=%llx renderer=%llx area=%d,%d,%d,%d absolute=%d,%d,%d,%d client=0,0,%lld,%lld",
        U(s.city), U(s.sim), s.date, s.pauses[0], s.pauses[1], s.pauses[2], U(s.view), U(s.renderer),
        s.area.l,s.area.t,s.area.r,s.area.b,s.absolute.l,s.absolute.t,s.absolute.r,s.absolute.b,
        static_cast<long long>(w),static_cast<long long>(h));
    return true;
}
static bool same(const State& a, const State& b) {
    return a.app == b.app && a.city == b.city && a.sim == b.sim && a.view == b.view && a.renderer == b.renderer &&
        a.date == b.date && !std::memcmp(a.pauses,b.pauses,sizeof a.pauses) &&
        !std::memcmp(&a.area,&b.area,sizeof a.area) && !std::memcmp(&a.absolute,&b.absolute,sizeof a.absolute);
}
static bool pick(const State& s, Point p) {
    if (p.x < 0 || p.y < 0 || int64_t(p.x) >= int64_t(s.area.r)-s.area.l ||
        int64_t(p.y) >= int64_t(s.area.b)-s.area.t) return false;
    float xyz[3] = {}; uint32_t raw[3] = {};
    const bool ok = original<bool(*)(uint64_t,int32_t,int32_t,float*,bool)>(0x10078292c)(s.view,p.x,p.y,xyz,false);
    std::memcpy(raw,xyz,sizeof raw);
    logf_("FIXTURE PICK x=%d y=%d returned=%u world_bits=%08x,%08x,%08x",p.x,p.y,unsigned(ok),raw[0],raw[1],raw[2]);
    return ok && (raw[0]&0x7f800000)!=0x7f800000 && (raw[1]&0x7f800000)!=0x7f800000 && (raw[2]&0x7f800000)!=0x7f800000;
}
static uint64_t active(uint64_t view) {
    // Getter @ 0x100782390 dereferences stack end-8. Bound the entire stack first.
    uint64_t begin=0,end=0;
    if (!read(view+0x1e8,begin) || !read(view+0x1f0,end) || end < begin ||
        (end-begin)%8 || end-begin > 4096 || (end!=begin && !span(begin,size_t(end-begin)))) return 0;
    return original<uint64_t(*)(uint64_t)>(0x100782390)(view);
}
struct Retained {
    uint64_t p = 0;
    ~Retained() { if (p) original<uint32_t(*)(uint64_t)>(0x1003bdd0c)(p); }
};
static bool control(uint64_t p, uint64_t view) {
    uint32_t config=0; uint64_t attached=0;
    return object(p,0x100aa40d0,0x220) && slot(p,8,0x1003bdcf0) && slot(p,16,0x1003bdd0c) &&
        slot(p,0xa0,0x1003c0ca4) && slot(p,0x80,0x1003bed24) &&
        read(p+0x4c,config) && config==0x04000004 && read(p+0x28,attached) && attached==view;
}
static bool binding(Context& c, const State& s) {
    return c.verified_city && s.city==c.verified_city &&
        std::memchr(c.verified_path,0,sizeof c.verified_path) && c.verified_path[0]=='/' &&
        std::strstr(c.verified_path,"/_opensc4_aging_20261114/") &&
        std::memchr(c.manifest_sha256,0,sizeof c.manifest_sha256) && std::strlen(c.manifest_sha256)==64 &&
        c.deps.identity && c.deps.identity(c.owner,s);
}
static bool attempt_started = false; // one catalog/placement sequence per process
static void exec(void* arg) {
    Context* c = static_cast<Context*>(arg);
    logf_("FIXTURE BEGIN version=1 main_thread_required=1 nontransactional=1");
    const char* failure = nullptr; bool capture_after = false; State before{};
    bool after_ok = false;
    try {
        auto run = [&]() -> bool {
#define FP_REQUIRE(test,why) do { if (!(test)) { failure=why; return false; } } while (0)
            FP_REQUIRE(c,"null_context");
            c->complete=false;
            FP_REQUIRE(!c->consumed,"context_already_consumed_no_retry"); c->consumed=true;
            FP_REQUIRE(!c->catalog_calls && !c->move_calls && !c->click_calls,"nonzero_initial_counters");
            FP_REQUIRE(pthread_main_np(),"not_main_thread");
            FP_REQUIRE(image_guard(),"image_hash_slide_or_entry_bytes");
            FP_REQUIRE(state(before),"city_pause_view_bounds_or_core_service_guard");
            FP_REQUIRE(binding(*c,before),"missing_or_mismatched_verified_loaded_city_manifest_binding");
            FP_REQUIRE(c->deps.services,"missing_complete_service_guard_adapter");
            FP_REQUIRE(c->deps.services(c->owner,before,0),"service_guard_failed");
            if (!c->attempt) {
                FP_REQUIRE(c->point_count && c->point_count<=32,"inspection_requires_1_to_32_client_points");
                unsigned valid=0;
                for (unsigned i=0;i<c->point_count;++i) {
                    c->inspection_valid[i] = pick(before,c->points[i]);
                    if (c->inspection_valid[i]) ++valid;
                }
                State end{};
                FP_REQUIRE(state(end) && same(before,end) && binding(*c,end),"inspection_state_changed");
                FP_REQUIRE(valid,"no_valid_terrain_pick");
                c->inspection_state=end; c->inspection_complete=true; return true;
            }
            FP_REQUIRE(!attempt_started,"process_attempt_already_started_no_retry");
            FP_REQUIRE(c->inspection_complete && same(before,c->inspection_state),"missing_or_stale_inspection_receipt");
            FP_REQUIRE(c->deps.inventory,"missing_actual_building_lot_monthly_inventory_adapter");
            FP_REQUIRE(c->deps.inventory(c->owner,before,"BEFORE"),"incomplete_before_inventory");
            const Point target=c->target; // freeze exact coordinates for move/click ABI
            FP_REQUIRE(pick(before,target),"target_outside_client_or_invalid_terrain_pick");
            State pre{};
            FP_REQUIRE(state(pre) && same(before,pre) && binding(*c,pre),"state_changed_before_catalog");
            // Original normal catalog @ 0x10078410a; no Init repeat / manual patch.
            attempt_started=true; capture_after=true; ++c->catalog_calls;
            logf_("FIXTURE CATALOG command=6a2b245f config=04000004 close_menu=0");
            original<void(*)(uint64_t,uint32_t,uint32_t,bool)>(0x10078410a)(before.view,0x6a2b245f,0x04000004,false);
            const uint64_t p=active(before.view);
            FP_REQUIRE(control(p,before.view),"active_control_config_attached_view_or_slots");
            Retained held;
            original<uint32_t(*)(uint64_t)>(0x1003bdcf0)(p); held.p=p;
            FP_REQUIRE(c->deps.services(c->owner,before,p),"control_descriptor_or_services_failed");
            State selected{};
            FP_REQUIRE(state(selected) && same(before,selected) && binding(*c,selected) && active(before.view)==p,
                "state_changed_after_catalog");
            logf_("FIXTURE MOVE control=%llx x=%d y=%d flags=0",U(p),target.x,target.y);
            ++c->move_calls;
            original<bool(*)(uint64_t,int32_t,int32_t,uint32_t)>(0x1003c0ca4)(p,target.x,target.y,0);
            uint8_t eligible=0;
            State moved{};
            // Read-only checks only between final original move and single click.
            FP_REQUIRE(state(moved) && same(before,moved) && active(before.view)==p && control(p,before.view) &&
                read(p+0xdc,eligible) && eligible,"move_not_eligible_or_state_changed");
            ++c->click_calls; // increment BEFORE call: faults cannot masquerade as no attempt
            logf_("FIXTURE CLICK attempt=1 control=%llx x=%d y=%d flags=0",U(p),target.x,target.y);
            const bool result=original<bool(*)(uint64_t,int32_t,int32_t,uint32_t)>(0x1003bed24)(p,target.x,target.y,0);
            logf_("FIXTURE CLICK_RETURN value=%u fixture_success=not_inferred",unsigned(result));
            return true;
#undef FP_REQUIRE
        };
        const bool ran=run();
        if (c) c->complete=ran;
    } catch (...) { failure="cpp_exception_original_or_adapter"; if(c) c->complete=false; }
    // Always try actual observables after catalog/move/click, even on false/exception.
    // A Mach fault/process death cannot be recovered by catch and invalidates the log.
    if (c && capture_after) {
        try {
            State after{};
            if (state(after) && binding(*c,after)) {
                after_ok=c->deps.inventory && c->deps.inventory(c->owner,after,"AFTER");
                if (!same(before,after)) { c->complete=false; failure="state_changed_after_operation"; }
            }
        } catch (...) { after_ok=false; }
        if (!after_ok) { c->complete=false; failure="incomplete_after_inventory_nontransactional"; }
    }
    if (failure) logf_("FIXTURE FAIL reason=%s",failure);
    logf_("FIXTURE END complete=%u inspect=%u catalog_calls=%u move_calls=%u click_attempts=%u after_complete=%u aging=not_invoked save=conductor_owned",
        unsigned(c && c->complete),unsigned(c && c->inspection_complete),c?c->catalog_calls:0,c?c->move_calls:0,c?c->click_calls:0,unsigned(after_ok));
}
} // namespace fixture_placement
#endif
