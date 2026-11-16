// clean-room fallback (not authentic): one bounded diagnostic, no general UI API.
#ifndef OPENSC4_TUTORIAL_OPEN_FRAGMENT_INC
#define OPENSC4_TUTORIAL_OPEN_FRAGMENT_INC
#ifndef OPENSC4_TUTORIAL_INSPECT_FRAGMENT_INC
#error "include actual fixture base, bindings and tutorial inspection first"
#endif
namespace tutorial_open {
using namespace fixture_placement;
namespace fb=fixture_placement_bindings;
namespace ti=tutorial_inspect;
static std::atomic<bool> attempted{false};
// Same-process successful original OPEN observation; immutable after publication.
struct SuccessfulReceipt {
    bool valid=false;
    uint64_t city=0,ui=0,w=0,mgr=0,task=0,dialog=0;
    uint32_t date=0;
};
static SuccessfulReceipt successful_receipt{};
// Expected address-point spans and entry prefixes generated from the SHA-bound
// original image; runtime checks cover EVERY byte/slot in each used table span.
static constexpr uint64_t window_table[] = {
    0x10024ddc0,0x10024de10,0x10024de2c,0x10024f01c,0x10024de48,0x10024e74a,
    0x10016d194,0x10016d19e,0x10016d1ae,0x10016d1b8,0x10016ccfc,0x100169784,
    0x10016978e,0x10016a044,0x100169806,0x10016990c,0x1001699c6,0x100169ae6,
    0x1001699dc,0x10016a062,0x10016a0d0,0x10016a444,0x10016a476,0x100169f0a,
    0x100169f2e,0x100169b64,0x100169d2a,0x100169d54,0x100169df2,0x100169ea0,
    0x100169f52,0x100169faa,0x10016c582,0x10016c6a6,0x10016a138,0x10016a19c,
    0x10016a38e,0x10016a3c4,0x10016cd0c,0x10016a222,0x10016a2ce,0x10016c7e2,
    0x10016c7f4,0x10016c806,0x10016c812,0x10016c81e,0x10016c82a,0x10016c966,
    0x10016c97e,0x10016c974,0x10016c99a,0x10016c8e6,0x10016c910,0x10016c93a,
    0x10024e902,0x10016c8c6,0x10016c9c8,0x10016c9f8,0x10016a4e8,0x10016ca3e,
    0x10016ca2e,0x10016a3fa,0x10024ec42,0x10016d0b4,0x10016d0aa,0x10016d0cc,
    0x10016d0be,0x10016cbde,0x100168f7e,0x10016cbee,0x10016cc52,0x10016ccb2,
    0x10016ccc8,0x10016d092,0x10024ec0c,0x10016a8a6,0x10016a8b4,0x10016a8f6,
    0x10016d0d8,0x10016d0e4,0x10016d0f0,0x10016d112,0x10016bdd8,0x10016a9a0,
    0x10016a914,0x1002505c0,0x10016cd22,0x10016cd2c,0x10024ed30,0x10016b86c,
    0x100169556,0x10016d120,0x10016d12e,0x10016d16a,0x10016d178,0x10016b8b2,
    0x10016ba20,0x10016ba70,0x10016bd86,0x1002748a0,0x10016be44,0x10016d186,
    0x10016d2b2,0x10016d29c,0x10016d24a,0x10016d1c8,0x10016d280,0x10016d230,
    0x10016c382,0x10016d3d4,0x10016d3e6,0x10016d3ba,0x10016d3c6,0x10016ce30,
    0x10016cd36,0x10016ce82,0x10016cf14,0x10016cfde,0x10016c9b0,0x100169672,
    0x1001696dc,0x10024ec8a,0x10016cb10,0x10016abd0,0x10016b49a,0x10016d23e,
    0x10016d290,0x10016c3fe,0x10016c482,0x10016a82a,0x10024f14e,0x10024f260,
    0x10016a84a,0x10016a896,0x10024f2be,0x10016d2e8,0x10024f47c,0x10016d2f0,
    0x10024f4a0,0x10016d300,0x10016a89e,0x10016d308,0x10016d310,0x10024f6c8,
    0x10016d318,0x10016d350,0x10016d37c,0x10016d38e,
};
static constexpr uint64_t iface_table[] = {
    0x10024dde4,0x10024de1a,0x10024de36,0x10024fb50,
};
static constexpr uint64_t ui_table[] = {
    0x1000b7b16,0x1000b7b6e,0x1000b7b94,0x1000b6c36,0x1000b6c40,0x10003c93c,
    0x10003c952,0x1000b7fdc,
};
static constexpr uint64_t proc_table[] = {
    0x1000b7b52,0x1000b7b78,0x1000b7b9e,0x1000b8804,
};
static constexpr uint64_t manager_table[] = {
    0x100621f00,0x100621eb4,0x100621eda,0x1006222f4,0x1006215dc,0x100622302,
    0x100622352,0x1006223a2,0x100622406,0x1006213ac,0x100622320,0x100622340,
    0x100622478,0x1006224b4,0x1006228ba,0x100622944,0x1006229d2,0x100622510,
    0x10062275c,0x100621528,0x100623834,0x100623ad4,0x100623b14,0x100623dc6,
    0x100623c4e,
};
static constexpr Entry native_entries[] = {
    {0x10024de10,{0x55,0x48,0x89,0xe5,0x5d,0xe9,0x76,0xaf,0xf1,0xff,0x55,0x48,0x89,0xe5,0x48,0x81}},
    {0x10024de2c,{0x55,0x48,0x89,0xe5,0x5d,0xe9,0x68,0xaf,0xf1,0xff,0x55,0x48,0x89,0xe5,0x48,0x81}},
    {0x100168d90,{0x55,0x48,0x89,0xe5,0x8b,0x47,0x18,0xff,0xc0,0x89,0x47,0x18,0x5d,0xc3,0x8b,0x47}},
    {0x100168d9e,{0x8b,0x47,0x18,0x83,0xf8,0x2,0x72,0x6,0xff,0xc8,0x89,0x47,0x18,0xc3,0x55,0x48}},
    {0x1000b7b6e,{0x55,0x48,0x89,0xe5,0x5d,0xe9,0x9a,0x4d,0xf8,0xff,0x55,0x48,0x89,0xe5,0x48,0x83}},
    {0x1000b7b94,{0x55,0x48,0x89,0xe5,0x5d,0xe9,0x82,0x4d,0xf8,0xff,0x55,0x48,0x89,0xe5,0x48,0x83}},
    {0x10003c912,{0x55,0x48,0x89,0xe5,0x8b,0x47,0x8,0xff,0xc0,0x89,0x47,0x8,0x5d,0xc3,0x8b,0x47}},
    {0x10003c920,{0x8b,0x47,0x8,0x83,0xf8,0x2,0x72,0x6,0xff,0xc8,0x89,0x47,0x8,0xc3,0x55,0x48}},
    {0x100621eb4,{0x55,0x48,0x89,0xe5,0x48,0x83,0xc7,0x8,0x5d,0xe9,0xc,0xb4,0xa1,0xff,0x55,0x48}},
    {0x100621eda,{0x55,0x48,0x89,0xe5,0x48,0x83,0xc7,0x8,0x5d,0xe9,0xfe,0xb3,0xa1,0xff,0x55,0x48}},
    {0x10003d2ce,{0x55,0x48,0x89,0xe5,0x48,0x83,0xc7,0x8,0x5d,0xe9,0x36,0xf6,0xff,0xff,0x55,0x48}},
    {0x10003d2e6,{0x55,0x48,0x89,0xe5,0x48,0x83,0xc7,0x8,0x5d,0xe9,0x2c,0xf6,0xff,0xff,0x55,0x48}},
    {0x100623c4e,{0x55,0x48,0x89,0xe5,0x48,0x83,0xec,0x10,0x48,0x89,0x75,0xf8,0x48,0x85,0xf6,0x74}},
    {0x10024fb50,{0x55,0x48,0x89,0xe5,0x48,0x8d,0x87,0xb0,0xfe,0xff,0xff,0x5d,0xc3,0x90,0x55,0x48}},
    {0x1000b8804,{0x55,0x48,0x89,0xe5,0x48,0x83,0xc7,0xf0,0x5d,0xe9,0xca,0xf7,0xff,0xff,0x55,0x48}},
    {0x10016d38e,{0x55,0x48,0x89,0xe5,0x48,0x83,0xec,0x20,0x48,0x8d,0x45,0xe0,0x89,0x10,0x48,0x89}},
    {0x10016d37c,{0x55,0x48,0x89,0xe5,0x48,0x8b,0x7f,0x8,0x48,0x8b,0x7,0xff,0x50,0x70,0xb0,0x1}},
    {0x1006228ba,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec}},
};
template<size_t N> static bool table(uint64_t p,uint64_t expected,const uint64_t (&slots)[N]) {
    if(!object(p,expected,8) || !span(relocated(expected),N*8)) return false;
    for(size_t i=0;i<N;++i) if(!slot(p,unsigned(i*8),slots[i])) return false;
    return true;
}
static bool guards() {
    if(!image_guard() || !fb::entries_guard()) return false;
    for(const Entry& e:native_entries) {
        unsigned char b[16];
        if(!read(relocated(e.address),b,16) || std::memcmp(b,e.bytes,16)) return false;
    }
    return true;
}
struct Capture {
    uint64_t tutorial=0,ui=0,iface=0,w=0,mgr=0,task=0,dialog=0;
    ti::Snapshot stable{};
};
static bool capture(const State& s,Capture& c,const char* phase) {
    auto& v=c.stable;
    if(!ti::field(v,phase,"city_tutorial",s.city,0x340,c.tutorial) ||
       !ti::known(v,phase,"tutorial",c.tutorial,0x100a6b7c0,0x68) ||
       !ti::field(v,phase,"tutorial_ui",c.tutorial,0x60,c.ui) ||
       !ti::known(v,phase,"ui",c.ui,0x100a6ee40,0xc0)) return false;
    uint8_t raw=0;
    for(unsigned off:{0x2fu,0x3cu}) if(!ti::field(v,phase,"tutorial_flag",c.tutorial,off,raw)) return false;
    uint32_t word=0;
    for(unsigned off:{0x30u,0x34u}) if(!ti::field(v,phase,"tutorial_word",c.tutorial,off,word)) return false;
    if(!ti::field(v,phase,"ui_finish",c.ui,0x73,raw) ||
       !ti::field(v,phase,"ui_interface",c.ui,0x20,c.iface) || c.iface<0x150 ||
       !table(c.iface,0x100a89f80,iface_table)) return false;
    // AsIGZWin secondary thunk @0x10024fb50: lea rax,[rdi-150].
    c.w=c.iface-0x150;
    uint64_t back=0,proc=0;
    if(!object(c.w,0x100a899c0,0x1a0) || !table(c.w,0x100a899c0,window_table) ||
       !read(c.w+0x150,back) || back!=relocated(0x100a89f80) ||
       !ti::field(v,phase,"window_manager",c.w,8,c.mgr) ||
       !object(c.mgr,0x100ad8970,0x59) || !table(c.mgr,0x100ad8970,manager_table) ||
       !table(c.ui,0x100a6ee40,ui_table) ||
       !ti::field(v,phase,"window_proc",c.w,0x198,proc) || c.ui>UINT64_MAX-0x10 || proc!=c.ui+0x10 ||
       !table(proc,0x100a6eec8,proc_table) ||
       !ti::field(v,phase,"manager_initialized",c.mgr,0x58,raw) || raw!=1) return false;
    // Fresh manager+C0 @0x100623c4e: bool(this, nonnull W). Never drain queue.
    if(!original<bool(*)(uint64_t,uint64_t)>(0x100623c4e)(c.mgr,c.w)) return false;
    // Raw queue observations ONLY; not traversed or interpreted as delivery proof.
    for(unsigned off:{0x30u,0x38u,0x40u}) {
        uint64_t q=0;
        if(!read(c.mgr+off,q)) return false;
        logf_("TUTORIAL_OPEN QUEUE phase=%s offset=%x raw=%llx",phase,off,U(q));
    }
    uint64_t begin=0,end=0,cap=0; uint32_t index=0;
    if(!ti::field(v,phase,"task_index",c.ui,0x60,index) || index!=166 ||
       !ti::field(v,phase,"tasks_begin",c.ui,0x88,begin) ||
       !ti::field(v,phase,"tasks_end",c.ui,0x90,end) ||
       !ti::field(v,phase,"tasks_cap",c.ui,0x98,cap) ||
       !begin || (begin&7) || end<begin || end-begin!=224*8 || cap<end ||
       (cap-begin)%8 || cap-begin>4096*8 || !span(begin,size_t(cap-begin)) ||
       !ti::field(v,phase,"task",begin,index*8,c.task) ||
       !ti::known(v,phase,"task",c.task,0x100a75330,0x148)) return false;
    // GetCurrentTask @0x1000b8812 and EndTask @0x10011edfe; observations only.
    if(!ti::field(v,phase,"task_function",c.task,0x104,word) || word!=11) return false;
    for(unsigned off:{0x100u,0x128u,0x129u}) if(!ti::field(v,phase,"task_byte",c.task,off,raw)) return false;
    uint64_t dep=0;
    if(!ti::field(v,phase,"task_dependency",c.task,0x140,dep)) return false;
    // Dialog deliberately excluded from stable inventory: it must change NULL -> nonnull.
    if(!read(c.ui+0x48,c.dialog)) return false;
    logf_("TUTORIAL_OPEN BINDINGS phase=%s W=%llx UI=%llx manager=%llx dialog=%llx new_runtime_validation=1",phase,U(c.w),U(c.ui),U(c.mgr),U(c.dialog));
    return v.add(c.w) && v.add(c.iface);
}
struct Lease { uint64_t p=0,vt=0,add=0,release=0; unsigned offset=0; bool owned=false; };
struct Job {
    fb::Owner owner{}; State before{}; Capture first{};
    Lease leases[3]{};
    bool pending=false,ended=false;
};
// A raw static pointer intentionally has NO destructor/native shutdown actions.
// Exactly one process-wide attempt => at most one quarantined heap context.
static Job* quarantine=nullptr;
static bool lease_guard(const Lease& l) {
    if(l.vt==0x100a899c0) return table(l.p,l.vt,window_table);
    if(l.vt==0x100a6ee40) return table(l.p,l.vt,ui_table);
    if(l.vt==0x100ad8970) return table(l.p,l.vt,manager_table);
    return false;
}
static bool retain(Lease& l,bool& uncertain) {
    uncertain=false;
    uint32_t before=0,after=0;
    if(!lease_guard(l) || !read(l.p+l.offset,before) || !before || before>=UINT32_MAX-1) return false;
    // Mark before native entry: exceptions leave ownership uncertain => quarantine.
    l.owned=true; uncertain=true;
    const uint32_t result=original<uint32_t(*)(uint64_t)>(l.add)(l.p);
    const bool ok=read(l.p+l.offset,after) && after==before+1 && result==after;
    logf_("TUTORIAL_OPEN RETAIN p=%llx before=%u after=%u result=%u proven=%u",U(l.p),before,after,result,unsigned(ok));
    if(ok) uncertain=false;
    return ok;
}
static bool release_all(Job& j) {
    // Preflight ALL tables/counts before any native release. Recheck each too.
    for(const Lease& l:j.leases) if(l.owned) {
        uint32_t count=0;
        if(!lease_guard(l) || !read(l.p+l.offset,count) || count<2) return false;
    }
    for(int i=2;i>=0;--i) { Lease& l=j.leases[i]; if(!l.owned) continue;
        uint32_t before=0,after=0;
        if(!lease_guard(l) || !read(l.p+l.offset,before) || before<2) return false;
        // Never enter destruction branch; never release an unknown address point.
        const uint32_t result=original<uint32_t(*)(uint64_t)>(l.release)(l.p);
        l.owned=false;
        const bool ok=read(l.p+l.offset,after) && after==before-1 && result==after;
        logf_("TUTORIAL_OPEN RELEASE p=%llx before=%u after=%u result=%u proven=%u",U(l.p),before,after,result,unsigned(ok));
        if(!ok) return false;
    }
    return true;
}
static void end(Job* j,bool complete,const char* reason,bool hold) {
    if(j->ended) return;
    j->ended=true;
    if(hold) {
        quarantine=j;
        logf_("TUTORIAL_OPEN INCOMPLETE reason=%s lease_retained_pending_process_shutdown=1 immediate_game_window_cleanup_required=1 no_retry=1 busy_remains_held=1",reason);
    } else fb::busy.store(false,std::memory_order_release);
    logf_("TUTORIAL_OPEN END complete=%u reason=%s pending=%u dialog_class=UNVERIFIED response_approval=0",unsigned(complete),reason,unsigned(j->pending));
    if(!hold) delete j;
}
static bool stable(const State& a,const State& b) {
    // Explicit pause must remain equal/positive; modal/hidden counters are logged
    // separately and may legitimately change when the original dialog opens.
    State copy=b;
    copy.pauses[1]=a.pauses[1]; copy.pauses[2]=a.pauses[2];
    return same(a,copy) && b.pauses[0]>0;
}
static void observe(void* opaque) {
    Job* j=static_cast<Job*>(opaque);
    logf_("TUTORIAL_OPEN OBSERVE main_queue=%u delay_is_not_delivery_proof=1",unsigned(pthread_main_np()!=0));
    bool ok=false;
    try {
        State after{},last{}; Capture c{};
        if(pthread_main_np() && guards() && state(after)) {
            logf_("TUTORIAL_OPEN PAUSE_DELTA explicit=%lld modal=%lld hidden=%lld before=%d,%d,%d after=%d,%d,%d",
                static_cast<long long>(after.pauses[0])-j->before.pauses[0],static_cast<long long>(after.pauses[1])-j->before.pauses[1],static_cast<long long>(after.pauses[2])-j->before.pauses[2],
                j->before.pauses[0],j->before.pauses[1],j->before.pauses[2],after.pauses[0],after.pauses[1],after.pauses[2]);
            ti::Snapshot dialog{};
            ok=fb::identity(&j->owner,after) && stable(j->before,after) && capture(after,c,"OBSERVE") &&
                ti::equal(j->first.stable,c.stable) && c.dialog &&
                ti::unresolved(dialog,"OBSERVE","original_created_dialog_UNVERIFIED",c.dialog) &&
                state(last) && fb::identity(&j->owner,last) && stable(j->before,last);
            // unresolved() accepts null vptr for general inspect; OPEN requires one.
            uint64_t vt=0;
            ok=ok && read(c.dialog,vt) && vt && span(vt,64);
        }
        if(ok) {
            logf_("TUTORIAL_OPEN DELIVERY_OBSERVED creation_only=1 known_relations_stable=1");
            if(guards() && release_all(*j)) {
                if(!successful_receipt.valid)
                    successful_receipt={true,j->before.city,c.ui,c.w,c.mgr,c.task,c.dialog,j->before.date};
                j->pending=false; end(j,true,"dialog_creation_observed",false); return;
            }
        }
    } catch(...) { logf_("TUTORIAL_OPEN INCOMPLETE reason=cpp_exception_native_faults_not_caught"); }
    end(j,false,"delivery_or_lifecycle_or_release_unproven",true);
}
static void begin(void* opaque) {
    Job* j=static_cast<Job*>(opaque);
    logf_("TUTORIAL_OPEN BEGIN complete=0 main_queue=%u process_attempt_consumed=1",unsigned(pthread_main_np()!=0));
    const char* reason="prepost_guard";
    bool safe_release=true;
    try {
        auto run=[&]() -> bool {
            if(!pthread_main_np() || !guards() || !state(j->before)) return false;
            auto& o=j->owner;
            if(!fb::loaded_path(j->before,o.path,o.regional)) return false;
            // Restrict to the dedicated original Rush Hour prepared-city path.
            const char* name=std::strrchr(o.path,'/');
            if(!name || std::strcmp(name+1,"City - Rush Hour Tutorial.sc4")) { reason="dedicated_RushHour_path_required"; return false; }
            o.city=j->before.city; o.receipt=j->before; o.inspected=true;
            if(!fb::identity(&o,j->before) || !capture(j->before,j->first,"BEFORE") || j->first.dialog) return false;
            const Capture& c=j->first;
            // Primary AddRef/Release uint32(this) ABI: W+18 @168d90/9e;
            // UI+8 @03c912/920; manager+18 via two +8 thunks @621eb4/eda,
            // @03d2ce/e6. Fresh bytes are logged, no unknown Release calls.
            j->leases[0]={c.w,0x100a899c0,0x10024de10,0x10024de2c,0x18,false};
            j->leases[1]={c.ui,0x100a6ee40,0x1000b7b6e,0x1000b7b94,8,false};
            j->leases[2]={c.mgr,0x100ad8970,0x100621eb4,0x100621eda,0x18,false};
            for(Lease& l:j->leases) {
                bool uncertain=false;
                if(!retain(l,uncertain)) { safe_release=!uncertain; reason="retain_unproven"; return false; }
            }
            State now{}; Capture again{};
            if(!guards() || !state(now) || !fb::identity(&o,now) || !same(j->before,now) ||
               !capture(now,again,"PREPOST") || again.dialog || !ti::equal(c.stable,again.stable)) { reason="prepost_lifecycle_changed"; return false; }
            // Native scalar PostMsg @0x10016d38e; source=W, target=W.
            // Original wrapper constructs 32-byte message; manager @1006228ba
            // copies semantic fields, retains NO target. p3=0 is OPEN-handler-
            // equivalent, NOT a capture of the actual button flag.
            j->pending=true; // before entry: exception may leave command queued.
            const bool returned=original<bool(*)(uint64_t,uint64_t,uint32_t,uint64_t,uint64_t,uint64_t)>(0x10016d38e)
                (c.w,c.w,3,0x287259f6,0x4a383da6,0);
            logf_("TUTORIAL_OPEN POSTED native_return=%u calls=1 delivery_proven=0 p3_zero_handler_equivalent_only=1",unsigned(returned));
            // One bounded later callback, normal event loop owns queue progress.
            dispatch_after_f(dispatch_time(DISPATCH_TIME_NOW,500*NSEC_PER_MSEC),dispatch_get_main_queue(),j,observe);
            return true;
        };
        if(run()) return;
    } catch(...) { reason="cpp_exception_native_faults_not_caught"; safe_release=false; }
    if(j->pending) {
        logf_("TUTORIAL_OPEN POSTED native_return=UNKNOWN outcome=UNKNOWN no_retry=1");
        dispatch_after_f(dispatch_time(DISPATCH_TIME_NOW,500*NSEC_PER_MSEC),dispatch_get_main_queue(),j,observe);
        return;
    }
    logf_("TUTORIAL_OPEN FAIL reason=%s prepost=1",reason);
    bool has_refs=false;
    for(const Lease& l:j->leases) has_refs=has_refs || l.owned;
    bool released=!has_refs;
    try { if(has_refs) released=safe_release && pthread_main_np() && guards() && release_all(*j); } catch(...) {}
    end(j,false,reason,!released);
}
// Queue acceptance only. Latch is consumed before ownership or native actions,
// never reset (including prepost errors); shared busy excludes every other command.
static bool opentutorialexit() {
    bool expected=false;
    if(!fb::busy.compare_exchange_strong(expected,true)) return false;
    expected=false;
    if(!attempted.compare_exchange_strong(expected,true)) { fb::busy.store(false,std::memory_order_release); return false; }
    try {
        std::unique_ptr<Job> j(new Job);
        dispatch_async_f(dispatch_get_main_queue(),j.get(),begin);
        j.release(); return true;
    } catch(...) {
        logf_("TUTORIAL_OPEN BEGIN complete=0 process_attempt_consumed=1");
        logf_("TUTORIAL_OPEN FAIL reason=context_allocation_or_queue_preparation");
        logf_("TUTORIAL_OPEN END complete=0 pending=0");
        fb::busy.store(false,std::memory_order_release); return false;
    }
}
} // namespace tutorial_open
#endif
