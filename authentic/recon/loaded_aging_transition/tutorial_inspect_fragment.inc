// clean-room fallback (not authentic): read-only diagnostic policies and limits.
// Include after fixture base + bindings; no query fragment dependency.
#ifndef OPENSC4_TUTORIAL_INSPECT_FRAGMENT_INC
#define OPENSC4_TUTORIAL_INSPECT_FRAGMENT_INC
#if !defined(OPENSC4_FIXTURE_PLACEMENT_FRAGMENT_INC) || !defined(OPENSC4_FIXTURE_BINDINGS_INC)
#error "tutorial inspection requires fixture base and bindings"
#endif
#if !defined(__x86_64__) || __cplusplus < 201703L
#error "tutorial inspection requires C++17 x86_64"
#endif
namespace tutorial_inspect {
using namespace fixture_placement;
namespace fb = fixture_placement_bindings;
// No game references owned: local storage survives exactly this queued callback.
struct Job { fb::Owner owner{}; };
struct Snapshot {
    uint64_t values[128] = {}; unsigned count=0;
    bool add(uint64_t v) {
        if (count>=128) return false;
        values[count++]=v; return true;
    }
};
// All new object inspections use Mach reads; no pointer in these logs is callable.
// Observations include width and read outcome. Failed reads never become zero facts.
template<class T> static bool field(Snapshot& s,const char* phase,const char* name,
                                    uint64_t base,unsigned offset,T& v) {
    const bool ok=base && base<=UINT64_MAX-offset && read(base+offset,v);
    logf_("TUTORIAL FIELD phase=%s name=%s base=%llx offset=%x width=%u readable=%u raw=%llx",
          phase,name,U(base),offset,unsigned(sizeof v),unsigned(ok),U(ok?v:0));
    return ok && s.add(uint64_t(v));
}
static bool known(Snapshot& s,const char* phase,const char* name,uint64_t p,uint64_t vt,size_t extent) {
    uint64_t actual=0;
    const bool vr=read(p,actual), extent_readable=vr && span(p,extent);
    const bool ok=vr && actual==relocated(vt) && extent_readable;
    logf_("TUTORIAL OBJECT phase=%s name=%s p=%llx vptr_read=%u vptr=%llx expected=%llx read_attempt_extent=%u extent_readable=%u allocation_extent=UNVERIFIED known=%u",
          phase,name,U(p),unsigned(vr),U(actual),U(relocated(vt)),unsigned(extent),unsigned(extent_readable),unsigned(ok));
    return s.add(p) && s.add(actual) && ok;
}
static bool unresolved(Snapshot& s,const char* phase,const char* name,uint64_t p) {
    logf_("TUTORIAL UNVERIFIED phase=%s name=%s p=%llx null=%u class=UNRESOLVED address_point=UNVERIFIED slots=observations_only calls=0",
          phase,name,U(p),unsigned(!p));
    if(!s.add(p)) return false;
    if(!p) return true; // absent dialog/service/dependency is a valid observation
    uint64_t vt=0;
    if(!field(s,phase,name,p,0,vt)) return false;
    if(!vt) return true;
    // Bounded first eight entries, not a claim of primary vtable or slot semantics.
    for(unsigned off=0;off<64;off+=8) {
        uint64_t target=0;
        if(!field(s,phase,"UNVERIFIED_slot_target_address_only",vt,off,target)) return false;
    }
    return true;
}
static bool capture(const State& state,Snapshot& s,const char* phase) {
    logf_("TUTORIAL INVENTORY_BEGIN phase=%s max_tasks=4096 max_slots_per_unknown=8",phase);
    bool complete=false;
    auto collect=[&]() -> bool {
        uint64_t tutorial=0,ui=0,task=0,iface=0,dialog=0;
        // GetTutorialSystem @0x100048922 reads city+340. No original getter call.
        if(!field(s,phase,"city_tutorial",state.city,0x340,tutorial)) return false;
        if(!tutorial) {
            logf_("TUTORIAL UNSUPPORTED phase=%s reason=null_tutorial",phase); return false;
        }
        // Tutorial system layout/flags @0x1000600e0,0x1000600ee,0x10005fba6,0x10005f3ae.
        if(!known(s,phase,"tutorial",tutorial,0x100a6b7c0,0x68)) return false;
        uint8_t flag=0,ended=0; uint32_t number=0,id=0;
        if(!field(s,phase,"tutorial_raw2f",tutorial,0x2f,flag) ||
           !field(s,phase,"tutorial_raw30",tutorial,0x30,number) ||
           !field(s,phase,"tutorial_raw34",tutorial,0x34,id) ||
           !field(s,phase,"tutorial_raw3c",tutorial,0x3c,ended) ||
           !field(s,phase,"tutorial_ui",tutorial,0x60,ui)) return false;
        if(!ui) { logf_("TUTORIAL UNSUPPORTED phase=%s reason=null_ui",phase); return false; }
        // Constructor/Init @0x1000b6d8c; through b8+8 is a read attempt, not sizeof.
        if(!known(s,phase,"ui_primary",ui,0x100a6ee40,0xc0)) return false;
        uint8_t finish=0; uint32_t index=0; uint64_t begin=0,end=0,cap=0;
        if(!field(s,phase,"ui_interface_raw20",ui,0x20,iface) ||
           !field(s,phase,"ui_dialog_raw48",ui,0x48,dialog) ||
           !field(s,phase,"ui_raw73",ui,0x73,finish) ||
           !field(s,phase,"ui_current_index",ui,0x60,index) ||
           !field(s,phase,"ui_tasks_begin",ui,0x88,begin) ||
           !field(s,phase,"ui_tasks_end",ui,0x90,end) ||
           !field(s,phase,"ui_tasks_capacity",ui,0x98,cap)) return false;
        // DoWinProcMessage @0x1000b7ffc would call interface vt+18. DO NOT CALL.
        if(!unresolved(s,phase,"ui20_interface",iface) || !unresolved(s,phase,"ui48_dialog",dialog)) return false;
        logf_("TUTORIAL WINDOW phase=%s actual_window=UNRESOLVED interface18_return=NOT_CALLED slot_targets_only=1",phase);
        // GetCurrentTask @0x1000b8812: u32 index, (end-begin)>>3, direct begin[index].
        // Stronger storage limits/alignment are diagnostic policy, not game behavior.
        if(end<begin || cap<end || (begin&7) || (end-begin)%8 || (cap-begin)%8 ||
           cap-begin>4096*8 || (cap!=begin && !span(begin,size_t(cap-begin)))) return false;
        const uint64_t count=(end-begin)/8;
        logf_("TUTORIAL TASK_VECTOR phase=%s count=%llu index=%u index_in_bounds=%u capacity_readable=1",phase,U(count),index,unsigned(index<count));
        if(index<count && !field(s,phase,"current_task",begin,index*8,task)) return false;
        if(!s.add(task)) return false;
        logf_("TUTORIAL TASK phase=%s p=%llx null=%u",phase,U(task),unsigned(!task));
        if(task) {
            // EndTask @0x10011edfe fresh --no-verify disasm: byte100, dword104,
            // bytes128/129, QWORD140 (0x10011ee4d/64). Extent is 148, not 144.
            if(!known(s,phase,"task",task,0x100a75330,0x148)) return false;
            uint8_t active=0,a=0,b=0; uint32_t function=0; uint64_t dependency=0;
            if(!field(s,phase,"task_raw100",task,0x100,active) ||
               !field(s,phase,"task_function_raw104",task,0x104,function) ||
               !field(s,phase,"task_raw128",task,0x128,a) ||
               !field(s,phase,"task_raw129",task,0x129,b) ||
               !field(s,phase,"task_pointer_raw140",task,0x140,dependency) ||
               !unresolved(s,phase,"task140_dependency",dependency)) return false;
        }
        uint64_t sim=0,ms=0;
        // EndTask @0x10011ee90 loads SL::spSimulator; no simulator virtual calls.
        if(!field(s,phase,"EndTask_global_simulator",relocated(0x100b213e8),0,sim) ||
           !unresolved(s,phase,"EndTask_simulator",sim)) return false;
        // finish_tutorial @0x1000b7bba uses SL::spMS2. Live address point unresolved.
        if(!field(s,phase,"message_global",relocated(0x100b212c8),0,ms) ||
           !unresolved(s,phase,"message_service_slots18_20_in_first8",ms)) return false;
        return true;
    };
    complete=collect();
    logf_("TUTORIAL INVENTORY_END phase=%s complete=%u observations=%u",phase,unsigned(complete),s.count);
    return complete;
}
static bool equal(const Snapshot& a,const Snapshot& b) {
    return a.count==b.count && !std::memcmp(a.values,b.values,a.count*sizeof(uint64_t));
}
static void command_exec(void* opaque) {
    std::unique_ptr<Job> job(static_cast<Job*>(opaque));
    struct Unlock { ~Unlock(){ fb::busy.store(false,std::memory_order_release); } } unlock;
    bool complete=false,before_ok=false,after_ok=false,stable=false;
    const char* failure=nullptr;
    logf_("TUTORIAL BEGIN complete=0 main_queue=1 readonly=1 nontransactional=1");
    try {
        State before{},after{},end{}; Snapshot first{},last{};
        auto run=[&]() -> bool {
            if(!pthread_main_np()) { failure="not_main_thread"; return false; }
            if(!image_guard() || !fb::entries_guard()) { failure="original_image_or_prefix_guard"; return false; }
            // Reuse existing state() native boundary (get_app/date getter) and
            // proven loaded_path originals only. All new object reads above Mach-only.
            if(!state(before)) { failure="loaded_paused_snapshot"; return false; }
            fb::Owner& o=job->owner;
            if(!fb::loaded_path(before,o.path,o.regional)) { failure="canonical_prepared_city_manifest"; return false; }
            o.city=before.city; o.receipt=before; o.inspected=true;
            if(!fb::identity(&o,before)) { failure="identity_before"; return false; }
            before_ok=capture(before,first,"BEFORE");
            // Reacquire from city. Never use cached tutorial/UI/task pointers here.
            // No queue dispatch or other work is inserted between the two captures.
            if(!state(after) || !fb::identity(&o,after)) { failure="identity_or_pause_after"; return false; }
            after_ok=capture(after,last,"AFTER");
            if(!state(end) || !fb::identity(&o,end)) { failure="identity_or_pause_final"; return false; }
            stable=same(before,after) && same(before,end) && equal(first,last);
            if(!before_ok || !after_ok) { failure="unsupported_or_incomplete_inventory"; return false; }
            if(!stable) { failure="city_tutorial_ui_task_sim_date_pause_changed"; return false; }
            return true;
        };
        complete=run();
    } catch(...) { failure="cpp_exception_native_faults_not_caught"; }
    if(!complete) logf_("TUTORIAL FAIL reason=%s complete=0",failure?failure:"incomplete");
    logf_("TUTORIAL END complete=%u before_complete=%u after_complete=%u stable=%u actual_window=UNRESOLVED ui_actions=0 new_original_calls=0",
          unsigned(complete),unsigned(before_ok),unsigned(after_ok),unsigned(stable));
}
// Queue acceptance only. Shared busy latch; no receipt creation or game ownership.
static bool inspecttutorial() {
    bool was=false;
    if(!fb::busy.compare_exchange_strong(was,true)) return false;
    try {
        std::unique_ptr<Job> job(new Job);
        dispatch_async_f(dispatch_get_main_queue(),job.get(),command_exec);
        job.release(); return true;
    } catch(...) { fb::busy.store(false,std::memory_order_release); return false; }
}
} // namespace tutorial_inspect
#endif
