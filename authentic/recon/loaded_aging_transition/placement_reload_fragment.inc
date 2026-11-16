// clean-room fallback (not authentic): fixed diagnostic reader policy, no RPC here.
#ifndef OPENSC4_PLACEMENT_RELOAD_FRAGMENT_INC
#define OPENSC4_PLACEMENT_RELOAD_FRAGMENT_INC
#if !defined(OPENSC4_PLACEMENT_RELOAD_INTEGRATION) || !defined(OPENSC4_PLACEMENT_CHECKPOINT_FRAGMENT_INC)
#error "define reload integration before bindings; include actual placement checkpoint first"
#endif
namespace placement_reload {
using namespace fixture_placement;
namespace fb=fixture_placement_bindings;
namespace pc=placement_checkpoint;
namespace tc=tutorial_checkpoint;
struct Sample {
#ifdef OPENSC4_AGING_INPUT_CAPTURE_FRAGMENT_INC
    aging_input_capture::Snapshot inputs{};
#endif
    State core{};
    fb::InventoryObservation inventory{};
    uint64_t regional=0,tutorial=0,ui=0;
    uint32_t task=0,guid=0;
    uint8_t active=0xff,cleanup=0,startup=0;
    bool core_ok=false,paths_ok=false,files_ok=false,tutorial_ok=false,full=false;
    char path[1024]{},pref[1024]{};
    fb::FixtureKind profile=fb::FixtureKind::Unbound;
};
// Each Context owns its sole private capability; never copies it to a receipt.
struct Context {
    const bool input_mode;
    fb::Owner owner{};
    Sample initial{},later{};
    explicit Context(bool inputs=false):input_mode(inputs) { owner.diagnostic_reload=true; }
};
static Context* context=nullptr; // published before queue; quarantined until process death
static std::atomic<bool> attempted{false},ended{false};
#ifdef OPENSC4_AGING_INPUT_CAPTURE_FRAGMENT_INC
// Selected once after the attempt/busy gates, before allocation can fail.
// Diagnostic terminal routing only; never grants an Owner capability.
static bool terminal_input_mode=false;
#endif
static bool target(const fb::OccupantObservation& o,uint64_t descriptor) {
    if(!o.complete || o.tgi[0]!=0x6534284a || o.tgi[1]!=0xc8dbccba || o.tgi[2]!=0x1f4b0000 ||
       !o.lot.complete || !o.lot.lot || !descriptor || o.lot.descriptor!=descriptor ||
       o.lot.rect.l!=58 || o.lot.rect.t!=66 || o.lot.rect.r!=63 || o.lot.rect.b!=72) return false;
    for(const auto& p:o.properties)
        if(!p.complete || (p.status!=fb::PropertyStatus::Missing && p.status!=fb::PropertyStatus::Supported) ||
           (p.status==fb::PropertyStatus::Supported && (!fb::property_domain(p.type) || p.word_count>256 || (p.type==0x800a && p.count>256)))) return false;
    return true; // actual state/condition/words retained, no pre-save goldens
}
static fb::PlacementSelection selection(const fb::InventoryObservation& o) {
    fb::PlacementSelection r{};
    for(size_t i=0;i<o.occurrences.size();++i) {
        const auto& v=o.occurrences[i];
        if(v.offset==0xc0 && target(v.value,o.guarded_descriptor)) {
            if(!r.count) { r.occurrence=i; r.index=v.index; r.node=v.node; r.occupant=v.value.occupant; }
            ++r.count;
        }
    }
    return r;
}
static bool readable(const Sample& s) {
    return s.full && s.core_ok && s.paths_ok && s.files_ok && s.tutorial_ok &&
        s.profile==fb::FixtureKind::DiagnosticPlaced && s.inventory.complete && s.inventory.identity_complete;
}
static bool stable(const Sample& a,const Sample& b) {
    return a.core_ok && b.core_ok && same(a.core,b.core) && a.regional==b.regional &&
        a.tutorial==b.tutorial && a.profile==b.profile && !std::strcmp(a.path,b.path) && !std::strcmp(a.pref,b.pref);
}
static bool complete(const Sample& a,const Sample& b) {
    return readable(a) && readable(b) && stable(a,b) && selection(a.inventory).count==1 && selection(b.inventory).count==1;
}
#ifdef OPENSC4_AGING_INPUT_CAPTURE_FRAGMENT_INC
static bool inputs_complete(const Sample& a,const Sample& b) {
    return complete(a,b) && a.inputs.complete && b.inputs.complete &&
        aging_input_capture::same_target(a.inputs,b.inputs) && aging_input_capture::same_inputs(a.inputs,b.inputs);
}
#endif
static bool collect(Context& c,Sample& s,const char* phase) {
    if(!image_guard() || !fb::entries_guard() || !tc::guards()) return false;
    s.files_ok=tc::canonical(fb::reload_manifest) && fb::saved_file(fb::reload_manifest) &&
        fb::file_hash(fb::reload_manifest,fb::reload_manifest_hash) && tc::canonical(fb::saved_path) &&
        fb::saved_file(fb::saved_path) && fb::file_hash(fb::saved_path,fb::reload_sha);
    if(!s.files_ok) return false;
    // State enforces signed explicit pause >0; failure can precede date acquisition.
    s.core_ok=state(s.core);
    if(!s.core_ok) return false;
    if(!pc::strict_paths(s.core,s.regional)) return false;
    // Fresh exact native strings, strict length policy even on failed captures.
    ScopedRZString loaded,preferences;
    original<void(*)(uint64_t,void*)>(0x1000480ee)(s.core.city,loaded.buf);
    original<void(*)(uint64_t,void*)>(0x100013038)(s.core.app,preferences.buf);
    s.paths_ok=pc::native_string(loaded,s.path) && pc::native_string(preferences,s.pref) &&
        !std::strcmp(s.path,fb::saved_path) && !std::strcmp(s.pref,pc::pref_path);
    if(!s.paths_ok) return false;
    // Raw system fields only, original class @100a6b7c0 / Read @10005fc1a.
    s.tutorial_ok=read(s.core.city+0x340,s.tutorial) && object(s.tutorial,0x100a6b7c0,0x68) &&
        read(s.tutorial+0x2f,s.active) && read(s.tutorial+0x60,s.ui) && read(s.tutorial+0x3c,s.cleanup) &&
        read(s.tutorial+0x30,s.task) && read(s.tutorial+0x34,s.guid) && read(s.tutorial+0x3d,s.startup);
    if(!s.tutorial_ok || s.active || s.ui) return false;
    char path[1024]{}; uint64_t regional=0;
    if(!fb::loaded_path(s.core,path,regional,&s.profile,c.owner.reload_allowed()) || s.profile!=fb::FixtureKind::DiagnosticPlaced ||
       std::strcmp(path,s.path) || regional!=s.regional) return false;
    auto& o=c.owner;
    if(o.kind!=fb::FixtureKind::Unbound && o.kind!=s.profile) return false;
    if(o.inspected && (o.city!=s.core.city || o.regional!=s.regional || std::strcmp(o.path,s.path))) return false;
    // Bind only after every initial identity comparison; no saved pointer binding.
    if(!o.inspected) {
        o.city=s.core.city; o.regional=s.regional; std::strcpy(o.path,s.path);
        o.kind=s.profile; o.receipt=s.core; o.inspected=true;
    }
    return fb::services(&o,s.core,0) && fb::inventory_observation(&o,s.core,phase,s.inventory);
}
static void sample(Context& c,Sample& s,const char* phase) {
    try { s.full=collect(c,s,phase); } catch(...) { s.full=false; } // NOT Mach fault containment
#ifdef OPENSC4_AGING_INPUT_CAPTURE_FRAGMENT_INC
    if(c.input_mode && s.full) {
        try {
            const auto selected=selection(s.inventory);
            if(selected.count==1) aging_input_capture::capture(s.inputs,s.core,s.inventory.occurrences[selected.occurrence]);
            if(s.inputs.complete)
                aging_input_capture::finish_capture(s.inputs,s.core,
                    [](State& after) { return state(after); },
                    [&c](const State& after) { return fb::identity(&c.owner,after); },phase);
        } catch(...) { s.inputs.complete=false; s.inputs.stage="cpp_exception"; }
        aging_input_capture::log(s.inputs,phase);
    }
#endif
    s.path[1023]=0; s.pref[1023]=0;
    logf_("PLACEMENT_RELOAD %s profile=DiagnosticPlaced PID=%d manifest=%s citySHA=%s source_runtime_END_complete=0 full=%u readable=%u core_ok=%u date_and_pauses_valid_only_if_core_ok=1 app=%llx city=%llx sim=%llx view=%llx regional=%llx tutorial=%llx date=%u pauses=%d,%d,%d raw2f=%u raw60=%llx raw3c=%u raw30=%x raw34=%x raw3d=%u tutorial_readable=%u paths_ok=%u files_ok=%u path=%s pref=%s",
        phase,int(getpid()),fb::reload_manifest_hash,fb::reload_sha,unsigned(s.full),unsigned(readable(s)),unsigned(s.core_ok),U(s.core.app),U(s.core.city),U(s.core.sim),U(s.core.view),U(s.regional),U(s.tutorial),s.core.date,s.core.pauses[0],s.core.pauses[1],s.core.pauses[2],unsigned(s.active),U(s.ui),unsigned(s.cleanup),s.task,s.guid,unsigned(s.startup),unsigned(s.tutorial_ok),unsigned(s.paths_ok),unsigned(s.files_ok),s.path,s.pref);
    // Original collector logs each raw row/word with phase; owned copies survive END.
    const auto r=selection(s.inventory);
    logf_("PLACEMENT_RELOAD TARGET phase=%s matching=%zu policy=exactly_one_matching_C0 other_occupants_allowed=1 index=%llu node=%llx occupant=%llx pointer_scope=this_PID_only full=%u",phase,r.count,U(r.index),U(r.node),U(r.occupant),unsigned(s.full));
    for(const auto& v:s.inventory.occurrences)
        logf_("PLACEMENT_RELOAD TARGET_ROW phase=%s list=%x index=%llu node=%llx occupant=%llx key=%08x:%08x:%08x lot=%llx descriptor=%llx state=%u condition=%u readable=%u",phase,v.offset,U(v.index),U(v.node),U(v.value.occupant),v.value.tgi[0],v.value.tgi[1],v.value.tgi[2],U(v.value.lot.lot),U(v.value.lot.descriptor),v.value.lot.state,v.value.lot.condition,unsigned(v.value.complete));
}
static void end(bool ok,const char* reason) {
    if(ended.exchange(true)) return;
#ifdef OPENSC4_AGING_INPUT_CAPTURE_FRAGMENT_INC
    if(terminal_input_mode) logf_("AGING_INPUT_CAPTURE END complete=%u reason=%s source_runtime_END_complete=0 source_commit=17ce264 source_hidden_pause_mismatch=1 non_atomic=1 aging_proof=0 saved_body_mapping=0 allocations_quarantined=1",unsigned(ok),reason);
#endif
    logf_("PLACEMENT_RELOAD END complete=%u reason=%s profile=DiagnosticPlaced PID=%d manifest=%s citySHA=%s source_runtime_END_complete=0 immediate_stop_required=1 noRetry=1 busyheld=1 context_present=%u allocations_quarantined=1 no_deliberate_mutation=1 queue_drained_claim=0 lifecycle_complete_claim=0",unsigned(ok),reason,int(getpid()),fb::reload_manifest_hash,fb::reload_sha,unsigned(context!=nullptr));
}
static void later(void*) {
    try {
        if(!pthread_main_np()) { end(false,"later_not_main"); return; }
        sample(*context,context->later,"LATER");
        const auto& a=context->initial; const auto& b=context->later;
        logf_("PLACEMENT_RELOAD DELTA initial_full=%u later_full=%u initial_target=%zu later_target=%zu stable=%u core_delta_valid=%u date=%lld pauses=%lld,%lld,%lld roots_equal=%u regional_equal=%u profile_equal=%u path_equal=%u pref_equal=%u delay_ms=500 ordinary_event_loop_only=1 no_repair=1",unsigned(readable(a)),unsigned(readable(b)),selection(a.inventory).count,selection(b.inventory).count,unsigned(stable(a,b)),unsigned(a.core_ok&&b.core_ok),static_cast<long long>(b.core.date)-a.core.date,static_cast<long long>(b.core.pauses[0])-a.core.pauses[0],static_cast<long long>(b.core.pauses[1])-a.core.pauses[1],static_cast<long long>(b.core.pauses[2])-a.core.pauses[2],unsigned(a.core.app==b.core.app&&a.core.city==b.core.city&&a.core.sim==b.core.sim),unsigned(a.regional==b.regional),unsigned(a.profile==b.profile),unsigned(!std::strcmp(a.path,b.path)),unsigned(!std::strcmp(a.pref,b.pref)));
#ifdef OPENSC4_AGING_INPUT_CAPTURE_FRAGMENT_INC
        if(context->input_mode) {
            logf_("AGING_INPUT_CAPTURE DELTA same_target_identity=%u same_captured_inputs_ownership_curves_funding_typed_outputs=%u",unsigned(aging_input_capture::same_target(a.inputs,b.inputs)),unsigned(aging_input_capture::same_inputs(a.inputs,b.inputs)));
            end(inputs_complete(a,b),"two_input_observations"); return;
        }
#endif
        end(complete(a,b),"two_observations");
    } catch(...) { end(false,"later_cpp_exception"); }
}
static void begin(void*) {
    try {
        logf_("PLACEMENT_RELOAD BEGIN profile=DiagnosticPlaced PID=%d manifest=%s citySHA=%s source_runtime_END_complete=0 no_deliberate_mutation=1 native_query_cache_allocator_effects_possible=1",int(getpid()),fb::reload_manifest_hash,fb::reload_sha);
#ifdef OPENSC4_AGING_INPUT_CAPTURE_FRAGMENT_INC
        if(context->input_mode) logf_("AGING_INPUT_CAPTURE BEGIN source_runtime_END_complete=0 source_commit=17ce264 source_hidden_pause_mismatch=1 non_atomic=1 aging_proof=0 saved_body_mapping=0");
#endif
        if(!pthread_main_np()) { end(false,"initial_not_main"); return; }
        sample(*context,context->initial,"INITIAL");
        if(!readable(context->initial)) { end(false,"initial_incomplete"); return; }
#ifdef OPENSC4_AGING_INPUT_CAPTURE_FRAGMENT_INC
        if(context->input_mode && !context->initial.inputs.complete) { end(false,"initial_inputs_incomplete"); return; }
#endif
        dispatch_after_f(dispatch_time(DISPATCH_TIME_NOW,500LL*NSEC_PER_MSEC),dispatch_get_main_queue(),nullptr,later);
    } catch(...) { end(false,"initial_or_queue_cpp_exception"); }
}
static bool queue_inspection(bool input_mode) {
    if(attempted.exchange(true)) return false;
    bool expected=false;
    if(!fb::busy.compare_exchange_strong(expected,true)) {
        logf_("PLACEMENT_RELOAD QUEUE accepted=0 reason=busy attempt_consumed=1"); return false;
    }
#ifdef OPENSC4_AGING_INPUT_CAPTURE_FRAGMENT_INC
    terminal_input_mode=input_mode;
#endif
    try {
        context=new Context(input_mode);
        dispatch_async_f(dispatch_get_main_queue(),nullptr,begin);
        logf_("PLACEMENT_RELOAD QUEUE accepted=1 completion=unknown busyheld=1");
        return true;
    } catch(...) { end(false,"allocation_or_queue_cpp_exception"); return false; }
    // Never release busy, delete context, acquire game references, or retry.
}
static bool inspectplacedcheckpointreload() { return queue_inspection(false); }
#ifdef OPENSC4_AGING_INPUT_CAPTURE_FRAGMENT_INC
static bool inspectloadedaginginputs() { return queue_inspection(true); }
#endif
}
#endif
