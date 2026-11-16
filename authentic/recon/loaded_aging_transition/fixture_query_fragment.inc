// clean-room fallback (not authentic): bounded query diagnostic policy.
// Include AFTER fixture_placement_fragment.inc AND fixture_placement_bindings.inc.
#ifndef OPENSC4_FIXTURE_QUERY_FRAGMENT_INCLUDED
#define OPENSC4_FIXTURE_QUERY_FRAGMENT_INCLUDED
#if !defined(OPENSC4_FIXTURE_PLACEMENT_FRAGMENT_INC) || !defined(OPENSC4_FIXTURE_BINDINGS_INC)
#error "fixture query requires base and concrete bindings first"
#endif
#if !defined(__x86_64__)
#error "fixture query ABIs require x86_64"
#endif
#include <cmath>
namespace fixture_query {
using namespace fixture_placement;
namespace fb = fixture_placement_bindings;
// Fresh read-only original image bytes, 2026-11-14; linear vmaddr mapping.
static constexpr Entry query_entries[] = {
{0x1000eb874,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x81,0xec}},
{0x1003c05cc,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec}},
{0x1003c0282,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec}},
{0x1003bfe4e,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec}},
{0x100221344,{0x55,0x48,0x89,0xe5,0x48,0x8b,0x47,0x48,0x5d,0xc3,0x48,0x85,0xf6,0x7e,0x3a,0x55}},
{0x100048922,{0x55,0x48,0x89,0xe5,0x48,0x8b,0x87,0x40,0x03,0x00,0x00,0x5d,0xc3,0x90,0x48,0x8b}},
{0x1000600e0,{0x55,0x48,0x89,0xe5,0x80,0x7f,0x2f,0x00,0x0f,0x95,0xc0,0x5d,0xc3,0x90,0x48,0x8b}},
{0x1000600ee,{0x48,0x8b,0x7f,0x60,0x48,0x85,0xff,0x74,0x0c,0x55,0x48,0x89,0xe5,0x48,0x8b,0x07}},
{0x1000b8812,{0x55,0x48,0x89,0xe5,0x8b,0x47,0x60,0x48,0x8b,0x97,0x90,0x00,0x00,0x00,0x48,0x8b}},
{0x10011e7b2,{0x55,0x48,0x89,0xe5,0x48,0x8d,0x87,0x08,0x01,0x00,0x00,0x5d,0xc3,0x90,0x55,0x48}},
{0x100017124,{0x55,0x48,0x89,0xe5,0xf6,0x47,0x08,0x01,0x75,0x09,0x48,0x83,0xc7,0x08,0x48,0xff}},
{0x10005f070,{0x55,0x48,0x89,0xe5,0xf6,0x47,0x08,0x01,0x75,0x02,0x5d,0xc3,0x48,0x8b,0x7f,0x18}},
{0x100403026,{0x55,0x48,0x89,0xe5,0x0f,0xb6,0x47,0x38,0x89,0x06,0x0f,0xb6,0x47,0x39,0x89,0x02}},
{0x100740e2e,{0x55,0x48,0x89,0xe5,0xf3,0x0f,0x10,0x05,0xca,0x5a,0x2d,0x00,0x5d,0xc3,0x55,0x48}},
{0x100740e3c,{0x55,0x48,0x89,0xe5,0x8b,0x47,0x40,0x5d,0xc3,0x90,0x55,0x48,0x89,0xe5,0x8b,0x47}},
{0x100740e46,{0x55,0x48,0x89,0xe5,0x8b,0x47,0x44,0x5d,0xc3,0x90,0x55,0x48,0x89,0xe5,0x0f,0xaf}},
{0x100132b24,{0x55,0x48,0x89,0xe5,0x48,0x83,0xc7,0xf8,0x5d,0xe9,0x9a,0xf3,0xff,0xff,0x55,0x48}},
{0x100148e18,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x53,0x50,0x48,0x89,0xfb,0x44,0x8b,0x76}},
};
static bool query_bytes() {
    for (const Entry& e:query_entries) {
        unsigned char b[16];
        if (!read(relocated(e.address),b,sizeof b) || std::memcmp(b,e.bytes,sizeof b)) return false;
    }
    return true;
}
static bool camera(const State& s,const fb::Owner& o) {
    uint64_t p=0; unsigned char bytes[fb::camera_extent]={};
    return read(s.renderer+0xd8,p) && p==o.camera && read(p,bytes,sizeof bytes) &&
        fb::camera_equal(bytes,o.camera_bytes);
}
static bool stable(const State& before,fb::Owner& o,uint64_t control_ptr=0) {
    State now{};
    return state(now) && same(before,now) && fb::identity(&o,now) && camera(now,o) &&
        (!control_ptr || (active(now.view)==control_ptr && control(control_ptr,now.view)));
}
static bool raw_control(uint64_t p,const char* phase) {
    uint64_t vt=0,view=0,manager=0; uint32_t config=0,xyz[3]={};
    int32_t orient[3]={},screen[2]={}; Rect exclusion{};
    uint8_t permission[2]={},special=0; int64_t cost=0,base=0;
    if (!read(p,vt) || !read(p+0x28,view) || !read(p+0x68,manager) ||
        !read(p+0x4c,config) || !read(p+0x54,orient) || !read(p+0x60,screen) ||
        !read(p+0xd0,xyz) || !read(p+0xdc,permission) || !read(p+0xc0,exclusion) ||
        !read(p+0x1f0,cost) || !read(p+0x1e0,base) || !read(p+0x218,special)) return false;
    logf_("QUERY CONTROL phase=%s p=%llx vptr=%llx view28=%llx manager68=%llx config4c=%08x orient54_58_5c=%d,%d,%d screen60_64=%d,%d xyz_d0=%08x,%08x,%08x permission_dc_dd=%u,%u cost1f0=%lld base1e0=%lld special218=%u exclusion_c0=%d,%d,%d,%d",
        phase,U(p),U(vt),U(view),U(manager),config,orient[0],orient[1],orient[2],screen[0],screen[1],xyz[0],xyz[1],xyz[2],unsigned(permission[0]),unsigned(permission[1]),
        static_cast<long long>(cost),static_cast<long long>(base),unsigned(special),exclusion.l,exclusion.t,exclusion.r,exclusion.b);
    return true;
}
// Policy clips by rejecting points; never clamp/change coordinates or game outputs.
// Original center @0x1003c0282 uses descriptor size and terrain cell geometry.
static bool domain(uint64_t manager,const float (&xyz)[3]) {
    uint64_t terrain=0;
    if (!object(manager,0x100ae1f90,8) || !slot(manager,0x40,0x1006c4166) ||
        !read(relocated(0x100b21318),terrain) || !object(terrain,0x100aea6c0,8) ||
        !slot(terrain,0x30,0x100740e2e) || !slot(terrain,0x38,0x100740e3c) || !slot(terrain,0x40,0x100740e46)) return false;
    uint64_t d=original<uint64_t(*)(uint64_t,uint32_t)>(0x1006c4166)(manager,0x04000004);
    if (!object(d,0x100aacc30,0x3a) || !slot(d,0x18,0x100402fba) || !slot(d,0x38,0x100403026) ||
        original<uint32_t(*)(uint64_t)>(0x100402fba)(d)!=0x04000004) return false;
    uint32_t w=0,h=0;
    original<void(*)(uint64_t,uint32_t*,uint32_t*)>(0x100403026)(d,&w,&h);
    const float cell=original<float(*)(uint64_t)>(0x100740e2e)(terrain);
    const uint32_t nx=original<uint32_t(*)(uint64_t)>(0x100740e3c)(terrain);
    const uint32_t nz=original<uint32_t(*)(uint64_t)>(0x100740e46)(terrain);
    logf_("QUERY DESCRIPTOR manager=%llx descriptor=%llx size=%u,%u terrain=%llx cell=%a dimensions=%u,%u",U(manager),U(d),w,h,U(terrain),double(cell),nx,nz);
    if (!w || !h || w>64 || h>64 || !nx || !nz || nx>1024 || nz>1024 || !std::isfinite(cell) || cell<=0 || cell>1024) return false;
    const double margin=double(w>h?w:h)+2;
    return std::isfinite(xyz[0]) && std::isfinite(xyz[1]) && std::isfinite(xyz[2]) &&
        double(xyz[0])/cell>=margin && double(xyz[2])/cell>=margin &&
        double(xyz[0])/cell<double(nx)-margin && double(xyz[2])/cell<double(nz)-margin;
}
static bool fresh_pick(const State& s,Point p,float (&xyz)[3]) {
    if (p.x<0 || p.y<0 || int64_t(p.x)>=int64_t(s.area.r)-s.area.l || int64_t(p.y)>=int64_t(s.area.b)-s.area.t) return false;
    bool ok=original<bool(*)(uint64_t,int32_t,int32_t,float*,bool)>(0x10078292c)(s.view,p.x,p.y,xyz,false);
    uint32_t bits[3]; std::memcpy(bits,xyz,sizeof bits);
    logf_("QUERY PICK x=%d y=%d returned=%u bits=%08x,%08x,%08x",p.x,p.y,unsigned(ok),bits[0],bits[1],bits[2]);
    return ok;
}
static bool feedback_copy(const ScopedRZString& s) {
    // ToChar @0x100017124: empty inline feedback is valid. Bounded hex log
    // preserves bytes without newline injection; never Release caller storage.
    uint8_t tag=s.buf[8]; uint64_t len=tag>>1,p=reinterpret_cast<uint64_t>(s.buf)+9;
    if (tag&1) { std::memcpy(&len,s.buf+0x10,8); std::memcpy(&p,s.buf+0x18,8); }
    if (len>1023 || (!(tag&1) && len>22)) return false;
    unsigned char text[1024]={}; if (len && !read(p,text,size_t(len))) return false;
    char hex[2047]={}; const char* digits="0123456789abcdef";
    for (size_t i=0;i<len;++i) { hex[2*i]=digits[text[i]>>4]; hex[2*i+1]=digits[text[i]&15]; }
    logf_("QUERY FEEDBACK length=%llu bytes_hex=%s",U(len),hex); return true;
}
static bool tutorial(uint64_t city,const Rect& r,bool coverage) {
    uint64_t t=0,raw=0;
    if (!slot(city,0x290,0x100048922) || !read(city+0x340,raw)) return false;
    t=original<uint64_t(*)(uint64_t)>(0x100048922)(city);
    if (t!=raw) return false;
    if (!t) { logf_("QUERY TUTORIAL system=0 restriction=0"); return true; }
    uint8_t flag=0;
    if (!object(t,0x100a6b7c0,0x68) || !slot(t,0x40,0x1000600e0) || !slot(t,0x50,0x1000600ee) || !read(t+0x2f,flag)) return false;
    const bool is=original<bool(*)(uint64_t)>(0x1000600e0)(t);
    logf_("QUERY TUTORIAL system=%llx flag2f=%u is_tutorial=%u",U(t),unsigned(flag),unsigned(is));
    if (!is) return true; // same gate as PlaceLot @0x1003bf175
    uint64_t ui=0;
    if (!read(t+0x60,ui)) return false;
    if (ui) {
        uint64_t begin=0,end=0,cap=0; uint32_t index=0;
        if (!object(ui,0x100a6ee40,0xa0) || !slot(ui,0x60,0x1000b8812) ||
            !read(ui+0x60,index) || !read(ui+0x88,begin) || !read(ui+0x90,end) || !read(ui+0x98,cap) ||
            end<begin || cap<end || (end-begin)%8 || cap-begin>32768 ||
            (end!=begin && !span(begin,size_t(end-begin)))) return false;
        logf_("QUERY TASK_VECTOR ui=%llx index=%u begin=%llx end=%llx capacity=%llx",U(ui),index,U(begin),U(end),U(cap));
    }
    const uint64_t task=original<uint64_t(*)(uint64_t)>(0x1000600ee)(t);
    if (!task) { logf_("QUERY TUTORIAL task=0 restriction=0"); return true; }
    if (!object(task,0x100a75330,0x118) || !slot(task,0x28,0x10011e7b2)) return false;
    const uint64_t ptr=original<uint64_t(*)(uint64_t)>(0x10011e7b2)(task);
    Rect region{};
    if (ptr!=task+0x108 || !read(ptr,region)) return false;
    logf_("QUERY TUTORIAL task=%llx region_ptr=%llx rect=%d,%d,%d,%d coverage_valid=%u exact_equal=%u",U(task),U(ptr),region.l,region.t,region.r,region.b,unsigned(coverage),unsigned(coverage && !std::memcmp(&r,&region,sizeof r)));
    return true;
}
struct Job { Point point; };
static void command_exec(void* opaque) {
    std::unique_ptr<Job> j(static_cast<Job*>(opaque));
    struct Unlock { ~Unlock(){ fb::busy.store(false,std::memory_order_release); } } unlock;
    unsigned catalog=0,move=0,query_count=0; bool complete=false,after=false,capture=false;
    const char* fail=nullptr; State before{}; fb::Owner* owner=nullptr;
    logf_("QUERY BEGIN x=%d y=%d main_queue=1 nontransactional=1",j->point.x,j->point.y);
    try {
        auto run=[&]() -> bool {
#define FQ_REQUIRE(test,why) do { if (!(test)) { fail=why; return false; } } while (0)
            FQ_REQUIRE(pthread_main_np(),"not_main_thread");
            FQ_REQUIRE(image_guard() && fb::entries_guard() && query_bytes(),"image_slide_hash_or_fresh_entry_bytes");
            owner=fb::receipt.get();
            FQ_REQUIRE(owner && !attempt_started && fb::receipt_allows_point(*owner,j->point),"missing_spent_receipt_or_process_attempt_or_unsuccessful_point");
            FQ_REQUIRE(state(before) && same(before,owner->receipt) && fb::identity(owner,before) && camera(before,*owner),"prepared_city_pause_state_or_camera_receipt");
            FQ_REQUIRE(fb::services(owner,before,0),"catalog_query_services_unavailable");
            FQ_REQUIRE(fb::inventory(owner,before,"QUERY_BEFORE"),"incomplete_focused_before_inventory");
            float xyz[3]={}; uint64_t manager=0;
            FQ_REQUIRE(fresh_pick(before,j->point,xyz) && read(relocated(0x100b21360),manager) && domain(manager,xyz),"pre_catalog_pick_or_clipped_diagnostic_domain");
            FQ_REQUIRE(stable(before,*owner),"state_or_camera_changed_before_catalog");
            // Consume BOTH routes before original catalog, including failure paths.
            owner->spent=true; attempt_started=true; capture=true; ++catalog;
            original<void(*)(uint64_t,uint32_t,uint32_t,bool)>(0x10078410a)(before.view,0x6a2b245f,0x04000004,false);
            const uint64_t p=active(before.view);
            FQ_REQUIRE(control(p,before.view),"control_init_or_installation_failed");
            Retained held; original<uint32_t(*)(uint64_t)>(0x1003bdcf0)(p); held.p=p;
            FQ_REQUIRE(fb::services(owner,before,p) && stable(before,*owner,p),"control_descriptor_services_or_camera_changed");
            FQ_REQUIRE(raw_control(p,"BEFORE_MOVE"),"raw_control_before_move_unreadable");
            uint64_t lm=0,ds=0,dev=0,budget=0;
            FQ_REQUIRE(read(relocated(0x100b21340),lm) && slot(lm,0xa8,0x100132b24) &&
                read(relocated(0x100b21400),ds) && slot(ds,0x120,0x10015387a),"semantic_query_lotmanager_or_developer_unavailable");
            dev=original<uint64_t(*)(uint64_t)>(0x10015387a)(ds);
            FQ_REQUIRE(object(dev,0x100a75d70,8) && slot(dev,0x100,0x100148e18) &&
                read(relocated(0x100b213f8),budget) && object(budget,0x100a87140,0x50) && slot(budget,0x30,0x100221344),"semantic_query_cost_or_budget_unavailable");
            ++move;
            const bool moved=original<bool(*)(uint64_t,int32_t,int32_t,uint32_t)>(0x1003c0ca4)(p,j->point.x,j->point.y,0);
            logf_("QUERY MOVE returned=%u x=%d y=%d placement=not_invoked",unsigned(moved),j->point.x,j->point.y);
            FQ_REQUIRE(stable(before,*owner,p) && raw_control(p,"POST_HOVER"),"post_hover_state_camera_or_raw_control");
            int32_t orient=0,screen[2]={}; uint8_t special=0;
            FQ_REQUIRE(read(p+0x58,orient) && orient>=0 && orient<=3 && read(p+0x60,screen) && screen[0]==j->point.x && screen[1]==j->point.y && read(p+0x218,special) && !special,"post_hover_orientation_coords_or_special_path_outside_domain");
            FQ_REQUIRE(fresh_pick(before,j->point,xyz) && domain(manager,xyz) && stable(before,*owner,p),"post_hover_fresh_pick_domain_or_camera");
            Rect center_rect{-1,-1,-1,-1}; float cx=0,cz=0;
            // Mixed SSE/integer ABI: RDI p, ESI orient, XMM0/1 x/z,
            // RDX &cx, RCX &cz, R8 &rect. No alternate implementation.
            const bool center=original<bool(*)(uint64_t,int32_t,float,float,float*,float*,Rect*)>(0x1003c0282)(p,orient,xyz[0],xyz[2],&cx,&cz,&center_rect);
            uint32_t center_bits[2]; std::memcpy(center_bits,&cx,4); std::memcpy(center_bits+1,&cz,4);
            logf_("QUERY CENTER returned=%u center_bits=%08x,%08x rect=%d,%d,%d,%d",unsigned(center),center_bits[0],center_bits[1],center_rect.l,center_rect.t,center_rect.r,center_rect.b);
            FQ_REQUIRE(center && center_rect.l>=0 && center_rect.t>=0 && center_rect.r>=center_rect.l && center_rect.b>=center_rect.t && int64_t(center_rect.r)-center_rect.l<64 && int64_t(center_rect.b)-center_rect.t<64,"center_coverage_invalid_or_outside_bounded_query_domain");
            FQ_REQUIRE(tutorial(before.city,center_rect,center),"tutorial_task_region_unavailable");
            int64_t funds_raw=0;
            FQ_REQUIRE(read(budget+0x48,funds_raw),"funds_raw_unreadable");
            const int64_t funds=original<int64_t(*)(uint64_t)>(0x100221344)(budget);
            logf_("QUERY FUNDS budget=%llx raw48=%lld original=%lld",U(budget),static_cast<long long>(funds_raw),static_cast<long long>(funds));
            Rect rect{-1,-1,-1,-1}; bool coverage=false; int64_t cost=0; int32_t error=0;
            ScopedRZString feedback;
            FQ_REQUIRE(object(reinterpret_cast<uint64_t>(feedback.buf),0x100a67890,0x24) && slot(reinterpret_cast<uint64_t>(feedback.buf),0x38,0x100017124) && slot(reinterpret_cast<uint64_t>(feedback.buf),0x18,0x10005f070),"caller_owned_string_vptr_or_slot");
            FQ_REQUIRE(stable(before,*owner,p) && query_bytes(),"state_camera_or_bytes_changed_before_query");
            // Eight total args: error and string are entry stack+8/+16.
            // Local outputs only. Original may call CanDemolishLot QUERY @0x1003bfe4e.
            ++query_count;
            const bool permission=original<bool(*)(uint64_t,const float*,int32_t,Rect*,bool*,int64_t*,int32_t*,void*)>(0x1003c05cc)(p,xyz,orient,&rect,&coverage,&cost,&error,feedback.buf);
            int64_t cached_cost=0; uint32_t cached_xyz[3]={},fresh_xyz[3]={};
            FQ_REQUIRE(read(p+0x1f0,cached_cost) && read(p+0xd0,cached_xyz),"post_query_cache_read_failed");
            std::memcpy(fresh_xyz,xyz,sizeof fresh_xyz);
            logf_("QUERY CACHE cached_cost=%lld sentinel=%u funds_ge_cached_cost=%u cached_vs_query_cost_equal=%u cached_vs_fresh_xyz_equal=%u",static_cast<long long>(cached_cost),unsigned(cached_cost==-100000),unsigned(funds>=cached_cost),unsigned(cached_cost==cost),unsigned(!std::memcmp(cached_xyz,fresh_xyz,sizeof fresh_xyz)));
            logf_("QUERY RESULT permission=%u coverage=%u error=%d cost=%lld rect=%d,%d,%d,%d independent_rect_equal=%u funds_ge_query_cost=%u fixture_success=not_inferred",unsigned(permission),unsigned(coverage),error,static_cast<long long>(cost),rect.l,rect.t,rect.r,rect.b,unsigned(!std::memcmp(&rect,&center_rect,sizeof rect)),unsigned(funds>=cost));
            FQ_REQUIRE(feedback_copy(feedback),"feedback_length_or_bounded_copy_failed");
            FQ_REQUIRE(raw_control(p,"AFTER_QUERY") && stable(before,*owner,p),"after_query_raw_control_state_or_camera");
            FQ_REQUIRE(query_count==1 && catalog==1 && move==1,"unexpected_operation_counts");
            return true;
#undef FQ_REQUIRE
        };
        complete=run();
    } catch (...) { fail="cpp_exception_nontransactional"; complete=false; }
    if (capture && owner) {
        try {
            State end{};
            after=state(end) && fb::identity(owner,end) && fb::inventory(owner,end,"QUERY_AFTER");
            if (!after || !same(before,end) || !camera(end,*owner)) { complete=false; if (!fail) fail="after_inventory_state_or_camera_incomplete"; }
        } catch (...) { complete=false; fail="after_inventory_cpp_exception"; }
    }
    if (fail) logf_("QUERY FAIL reason=%s",fail);
    logf_("QUERY END complete=%u catalog_count=%u move_count=%u query_count=%u after_complete=%u click_count=0 placement=not_invoked preview_query_sideeffects=possible fixture_success=not_inferred",unsigned(complete),catalog,move,query_count,unsigned(after));
}
// Queue acceptance only; no caller pointers retained. Shared busy/receipt/latch
// are the concrete bindings' existing objects, not another adapter layer.
static bool queryfixture(Point point) {
    bool was=false; if (!fb::busy.compare_exchange_strong(was,true)) return false;
    try {
        std::unique_ptr<Job> j(new Job{point});
        dispatch_async_f(dispatch_get_main_queue(),j.get(),command_exec); j.release(); return true;
    } catch (...) { fb::busy.store(false,std::memory_order_release); return false; }
}
} // namespace fixture_query
#endif
