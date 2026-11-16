// clean-room fallback (not authentic): bounded read-only diagnostic policy.
// Include AFTER actual base/bindings/inspect/open/finish/checkpoint. No RPC here.
#ifndef OPENSC4_TUTORIAL_RELOAD_FRAGMENT_INC
#define OPENSC4_TUTORIAL_RELOAD_FRAGMENT_INC
#ifndef OPENSC4_TUTORIAL_CHECKPOINT_FRAGMENT_INC
#error "include actual checkpoint after finish, then reload"
#endif
namespace tutorial_reload {
using namespace fixture_placement;
namespace fb=fixture_placement_bindings;
namespace tf=tutorial_finish;
namespace cp=tutorial_checkpoint;
static constexpr char fixture_path[]="/Users/russellmartin/Personal/OpenSC4/authentic/recon/loaded_aging_transition/TUTORIAL_RELOAD_FIXTURE_IDENTITY.json";
static constexpr char fixture_sha[]="25dbaa34ee02113ac32520c0f28482fa0502e6985d715b9c51cb94bdb5de64a6";
static constexpr char city_sha[]="01f91c3f091ddbb8bdeed9b0c48e60e10a8832f077355bce2a61386c2d8ae3a0";
// Direct reads of SHA-verified source image; tutorial_reload_prefixes.log.
struct Prefix { uint64_t address; unsigned char bytes[16]; size_t size; };
static constexpr Prefix prefixes[]={
{0x1003b2e54,{0x55,0x48,0x89,0xe5,0x8b,0x87,0x64,0x02,0x00,0x00,0x5d,0xc3},12},
{0x10064307a,{0x55,0x48,0x89,0xe5,0x48,0x8b,0x0f,0x48,0x83,0xc7,0x08,0x48,0x39,0xf9,0x74,0x53},16},
{0x100013824,{0x55,0x48,0x89,0xe5,0x48,0x8d,0x87,0x68,0x03,0x00,0x00,0x5d,0xc3,0x90,0x55,0x48},16},
};
static bool guards() {
    if(!cp::guards()) return false;
    for(const auto& e:prefixes) { unsigned char b[16]{};
        if(!read(relocated(e.address),b,e.size) || std::memcmp(b,e.bytes,e.size)) return false;
    }
    return true;
}
// Only numeric observations retained. Never dereference a previous sample value.
struct Sample {
    State core{};
    uint64_t tutorial=0,regional=0,registry=0,item=0,ui=0,header[3]{};
    uint32_t task=0,guid=0,regional_guid=0,item_guid=0;
    uint8_t active=0,cleanup=0,startup=0,item_active=0;
    bool paths_ok=false,fixture_ok=false,city_ok=false,full=false,item_known=false;
    char path[1024]{},pref[1024]{};
};
static bool files(bool canonical_fixture,bool fixture_hash,bool canonical_city,bool city_hash) {
    return canonical_fixture && fixture_hash && canonical_city && city_hash;
}
static bool criterion(const Sample& s) {
    return s.full && s.paths_ok && s.fixture_ok && s.city_ok && !s.active && !s.ui;
}
static bool stable(const Sample& a,const Sample& b) {
    return a.core.app==b.core.app && a.core.city==b.core.city && a.core.sim==b.core.sim &&
        a.tutorial==b.tutorial && a.regional==b.regional && a.registry==b.registry &&
        !std::strcmp(a.path,b.path) && !std::strcmp(a.pref,b.pref);
}
static bool completed(const Sample& a,const Sample& b) {
    // Initial explicit pause is an adapter precondition; subsequent drift is evidence.
    return criterion(a) && criterion(b) && a.core.pauses[0]>0 && stable(a,b);
}
static bool item_fields(uint64_t p,uint32_t query,Sample& s) {
    // GetItem @0x10064307a returns node+10 (NOT a vtable-bearing object).
    // DoMessage @0x10005f4b6 reads returned Item+10 as active.
    s.item=p; s.item_known=false;
    if(!p) return true; // absence is known, active is unavailable, never false-by-default
    if(!span(p,0x11) || !read(p,s.item_guid) || !read(p+0x10,s.item_active)) return false;
    s.item_known=true;
    return s.item_guid==query;
}
static bool collect(Sample& s) {
    // Fixed paths/hashes, freshly checked at each sample. No caller parameters.
    const bool cf=cp::canonical(fixture_path),cc=cp::canonical(cp::expected_path);
    const bool hf=fb::file_hash(fixture_path,fixture_sha);
    const bool hc=fb::file_hash(cp::expected_path,city_sha);
    s.fixture_ok=cf && hf; s.city_ok=cc && hc;
    if(!files(cf,hf,cc,hc) || !guards() || !tf::exit_state(s.core)) return false;
    s.paths_ok=cp::paths(s.core,s.path,s.regional,s.pref,false);
    if(!s.paths_ok || !cp::app_interface(s.core)) return false;
    // Read raw system fields; no UI/task/window dereference even if UI is nonnull.
    // System primary table @0x100a6b7c0; Read @0x10005fc1a / DoMessage @0x10005f4b6.
    if(!read(s.core.city+0x340,s.tutorial) || !object(s.tutorial,0x100a6b7c0,0x68) ||
       !read(s.tutorial+0x2f,s.active) || !read(s.tutorial+0x3c,s.cleanup) ||
       !read(s.tutorial+0x3d,s.startup) || !read(s.tutorial+0x30,s.task) ||
       !read(s.tutorial+0x34,s.guid) || !read(s.tutorial+0x60,s.ui)) return false;
    // GetTutorialGUID @0x1003b2e54 reads DWORD+264. Path getter's 128 is insufficient.
    if(!object(s.regional,0x100aa3230,0x268) || !slot(s.regional,0x208,0x1003b2e54)) return false;
    s.regional_guid=original<uint32_t(*)(uint64_t)>(0x1003b2e54)(s.regional);
    // cISC4App is primary+18, getter @0x100013824 returns interface+368=primary+380.
    // Header is raw data, NOT a vtable. Its readability does not prove tree lifetime.
    if(s.core.app>UINT64_MAX-0x398 || !object(s.core.app,0x100a67010,0x398)) return false;
    s.registry=original<uint64_t(*)(uint64_t)>(0x100013824)(s.core.app+0x18);
    if(s.registry!=s.core.app+0x380 || !read(s.registry,s.header)) return false;
    // Native traversal is an unchecked transitive Mach-fault boundary, NOT contained
    // by the bounded header read. GUID bit pattern passes unchanged in ESI.
    const uint64_t item=original<uint64_t(*)(uint64_t,uint32_t)>(0x10064307a)(s.registry,s.guid);
    return item_fields(item,s.guid,s);
}
static void sample(Sample& s,const char* phase) {
    try { s.full=collect(s); }
    catch(...) { s.full=false; } // C++ only; Mach faults are NOT caught
    // Failed string_copy may fill its entire buffer. Bound diagnostic consumers
    // on every path; termination does not upgrade the incomplete observation.
    s.path[sizeof(s.path)-1]=0;
    s.pref[sizeof(s.pref)-1]=0;
    logf_("TUTORIAL_RELOAD SAMPLE phase=%s full=%u criterion=%u app=%llx city=%llx sim=%llx tutorial=%llx regional=%llx registry=%llx registryHeader=%llx,%llx,%llx registryItem=%llx registryItemPresent=%u registryActiveKnown=%u registryActive=%d registryGUID=%08x regionalGUID=%08x task=%08x GUID=%08x active=%u UI=%llx cleanup=%u startupguard=%u date=%u pauses=%d,%d,%d path_ok=%u fixture_hash_ok=%u city_hash_ok=%u path=%s prefpath=%s saveDisabled_restriction=requires_false raw_fields_valid_only_if_full=1",
        phase,unsigned(s.full),unsigned(criterion(s)),U(s.core.app),U(s.core.city),U(s.core.sim),U(s.tutorial),U(s.regional),U(s.registry),
        U(s.header[0]),U(s.header[1]),U(s.header[2]),U(s.item),unsigned(s.item!=0),unsigned(s.item_known),s.item_known?int(s.item_active):-1,s.item_guid,s.regional_guid,
        s.task,s.guid,unsigned(s.active),U(s.ui),unsigned(s.cleanup),unsigned(s.startup),s.core.date,s.core.pauses[0],s.core.pauses[1],s.core.pauses[2],
        unsigned(s.paths_ok),unsigned(s.fixture_ok),unsigned(s.city_ok),s.path,s.pref);
}
struct Context { Sample initial{},later{}; };
static Context* context=nullptr; // heap-owned until process death; no game references acquired
static std::atomic<bool> attempted{false},ended{false};
static void end(bool complete,const char* reason) {
    if(ended.exchange(true)) return;
    logf_("TUTORIAL_RELOAD END complete=%u reason=%s busy_held=1 context_retained=1 noRetry=1 immediate_process_stop_required=1 two_observations_only=1 queue_drained_claim=0",
        unsigned(complete),reason);
}
static void later(void*) {
    if(!pthread_main_np()) { end(false,"later_not_main"); return; }
    sample(context->later,"LATER");
    logf_("TUTORIAL_RELOAD DELTA date=%lld pauses=%lld,%lld,%lld identity_path_consistent=%u initial_pause_precondition=%u no_repair=1 delay_ms=500 delay_is_not_proof=1",
        static_cast<long long>(context->later.core.date)-context->initial.core.date,
        static_cast<long long>(context->later.core.pauses[0])-context->initial.core.pauses[0],
        static_cast<long long>(context->later.core.pauses[1])-context->initial.core.pauses[1],
        static_cast<long long>(context->later.core.pauses[2])-context->initial.core.pauses[2],
        unsigned(stable(context->initial,context->later)),unsigned(context->initial.core.pauses[0]>0));
    end(completed(context->initial,context->later),"two_samples_observed_check_complete");
}
static void begin(void*) {
    logf_("TUTORIAL_RELOAD BEGIN main_queue=%u normal_load_and_explicit_pause_operator_preconditions=1",unsigned(pthread_main_np()!=0));
    if(!pthread_main_np()) { end(false,"initial_not_main"); return; }
    sample(context->initial,"INITIAL");
    // One ordinary delayed main-queue callback, no polling or native message draining.
    dispatch_after_f(dispatch_time(DISPATCH_TIME_NOW,500LL*NSEC_PER_MSEC),dispatch_get_main_queue(),nullptr,later);
}
static bool inspectcheckpointreload() {
    // Exact noarg entry. True is queue acceptance only; END is completion evidence.
    if(attempted.exchange(true)) return false;
    bool expected=false;
    if(!fb::busy.compare_exchange_strong(expected,true)) {
        logf_("TUTORIAL_RELOAD QUEUE accepted=0 reason=busy attempt_consumed=1"); return false;
    }
    try {
        context=new Context;
        logf_("TUTORIAL_RELOAD QUEUE accepted=1 completion=unknown busy_held=1");
        dispatch_async_f(dispatch_get_main_queue(),nullptr,begin);
        return true;
    } catch(...) { end(false,"allocation_or_queue_cpp_exception"); return false; }
    // Busy is intentionally NEVER cleared, including normal completion.
}
} // namespace tutorial_reload
#endif
