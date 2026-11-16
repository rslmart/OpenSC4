#include "aging_input_capture_syntax_stub.cpp"
#include <cassert>
#include <stdexcept>
static unsigned checks=0,ends=0,aging_ends=0;
static unsigned reload_complete=99,aging_complete=99;
static void check(bool b) { assert(b); ++checks; }
static void logf_(const char* f,...) {
 const bool reload=std::strstr(f,"PLACEMENT_RELOAD END ")!=nullptr;
 const bool aging=std::strstr(f,"AGING_INPUT_CAPTURE END ")!=nullptr;
 if(reload || aging) {
  va_list args;va_start(args,f);const unsigned complete=va_arg(args,unsigned);va_end(args);
  if(reload){++ends;reload_complete=complete;}
  if(aging){++aging_ends;aging_complete=complete;}
 }
}
static uint64_t get_app() { std::abort(); }
ScopedRZString::ScopedRZString() { std::memset(buf,0,sizeof buf); }
ScopedRZString::~ScopedRZString() {}
namespace mock {
using namespace placement_reload;
static placement_reload::Context* context=nullptr;
static std::atomic<bool> attempted{false},ended{false};
static bool terminal_input_mode=false;
static bool allocation_throws=false;
static placement_reload::Context* allocate(bool mode) { if(allocation_throws) throw std::bad_alloc(); return new placement_reload::Context(mode); }
static unsigned queued=0,delayed=0,captures=0; static bool capture=true,throws=false;
static dispatch_function_t callback=nullptr,late_callback=nullptr;
static bool pthread_main_np() { return true; }
static void sample(placement_reload::Context&,Sample& s,const char*) { ++captures; s.full=s.core_ok=s.paths_ok=s.files_ok=s.tutorial_ok=s.inventory.complete=s.inventory.identity_complete=capture; s.profile=fb::FixtureKind::DiagnosticPlaced; s.inputs.complete=capture; }
static dispatch_queue_t dispatch_get_main_queue() { return nullptr; }
static dispatch_time_t dispatch_time(dispatch_time_t,int64_t d) { check(d==500LL*NSEC_PER_MSEC); return 0; }
static void dispatch_async_f(dispatch_queue_t,void*,dispatch_function_t cb) { ++queued; callback=cb; if(throws) throw std::runtime_error("queue"); }
static void dispatch_after_f(dispatch_time_t,dispatch_queue_t,void*,dispatch_function_t cb) { ++delayed; late_callback=cb; if(throws) throw std::runtime_error("queue"); }
static void end(bool ok,const char* reason) {
    if(ended.exchange(true)) return;
#ifdef OPENSC4_AGING_INPUT_CAPTURE_FRAGMENT_INC
    if(terminal_input_mode) logf_("AGING_INPUT_CAPTURE END complete=%u reason=%s source_runtime_END_complete=0 source_commit=17ce264 source_hidden_pause_mismatch=1 non_atomic=1 aging_proof=0 saved_body_mapping=0 allocations_quarantined=1",unsigned(ok),reason);
#endif
    logf_("PLACEMENT_RELOAD END complete=%u reason=%s profile=DiagnosticPlaced PID=%d manifest=%s citySHA=%s source_runtime_END_complete=0 immediate_stop_required=1 noRetry=1 busyheld=1 context_present=%u allocations_quarantined=1 no_deliberate_mutation=1 queue_drained_claim=0 lifecycle_complete_claim=0",unsigned(ok),reason,int(getpid()),fb::reload_manifest_hash,fb::reload_sha,unsigned(context!=nullptr));
}
static void later(void*) {
    try {
        if(!pthread_main_np()) { mock::end(false,"later_not_main"); return; }
        mock::sample(*context,context->later,"LATER");
        const auto& a=context->initial; const auto& b=context->later;
        logf_("PLACEMENT_RELOAD DELTA initial_full=%u later_full=%u initial_target=%zu later_target=%zu stable=%u core_delta_valid=%u date=%lld pauses=%lld,%lld,%lld roots_equal=%u regional_equal=%u profile_equal=%u path_equal=%u pref_equal=%u delay_ms=500 ordinary_event_loop_only=1 no_repair=1",unsigned(readable(a)),unsigned(readable(b)),selection(a.inventory).count,selection(b.inventory).count,unsigned(stable(a,b)),unsigned(a.core_ok&&b.core_ok),static_cast<long long>(b.core.date)-a.core.date,static_cast<long long>(b.core.pauses[0])-a.core.pauses[0],static_cast<long long>(b.core.pauses[1])-a.core.pauses[1],static_cast<long long>(b.core.pauses[2])-a.core.pauses[2],unsigned(a.core.app==b.core.app&&a.core.city==b.core.city&&a.core.sim==b.core.sim),unsigned(a.regional==b.regional),unsigned(a.profile==b.profile),unsigned(!std::strcmp(a.path,b.path)),unsigned(!std::strcmp(a.pref,b.pref)));
#ifdef OPENSC4_AGING_INPUT_CAPTURE_FRAGMENT_INC
        if(context->input_mode) {
            logf_("AGING_INPUT_CAPTURE DELTA same_target_identity=%u same_captured_inputs_ownership_curves_funding_typed_outputs=%u",unsigned(aging_input_capture::same_target(a.inputs,b.inputs)),unsigned(aging_input_capture::same_inputs(a.inputs,b.inputs)));
            mock::end(inputs_complete(a,b),"two_input_observations"); return;
        }
#endif
        mock::end(complete(a,b),"two_observations");
    } catch(...) { mock::end(false,"later_cpp_exception"); }
}
static void begin(void*) {
    try {
        logf_("PLACEMENT_RELOAD BEGIN profile=DiagnosticPlaced PID=%d manifest=%s citySHA=%s source_runtime_END_complete=0 no_deliberate_mutation=1 native_query_cache_allocator_effects_possible=1",int(getpid()),fb::reload_manifest_hash,fb::reload_sha);
#ifdef OPENSC4_AGING_INPUT_CAPTURE_FRAGMENT_INC
        if(context->input_mode) logf_("AGING_INPUT_CAPTURE BEGIN source_runtime_END_complete=0 source_commit=17ce264 source_hidden_pause_mismatch=1 non_atomic=1 aging_proof=0 saved_body_mapping=0");
#endif
        if(!pthread_main_np()) { mock::end(false,"initial_not_main"); return; }
        mock::sample(*context,context->initial,"INITIAL");
        if(!readable(context->initial)) { mock::end(false,"initial_incomplete"); return; }
#ifdef OPENSC4_AGING_INPUT_CAPTURE_FRAGMENT_INC
        if(context->input_mode && !context->initial.inputs.complete) { mock::end(false,"initial_inputs_incomplete"); return; }
#endif
        mock::dispatch_after_f(mock::dispatch_time(DISPATCH_TIME_NOW,500LL*NSEC_PER_MSEC),mock::dispatch_get_main_queue(),nullptr,later);
    } catch(...) { mock::end(false,"initial_or_queue_cpp_exception"); }
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
        context=allocate(input_mode);
        mock::dispatch_async_f(mock::dispatch_get_main_queue(),nullptr,begin);
        logf_("PLACEMENT_RELOAD QUEUE accepted=1 completion=unknown busyheld=1");
        return true;
    } catch(...) { mock::end(false,"allocation_or_queue_cpp_exception"); return false; }
    // Never release busy, delete context, acquire game references, or retry.
}
static bool inspectplacedcheckpointreload() { return mock::queue_inspection(false); }
static bool inspectloadedaginginputs() { return mock::queue_inspection(true); }
}
namespace typed_mock {
static bool returned=false,write=false;
static uint32_t output=0;
template<class T> static bool native(uint64_t h,uint32_t id,T& v) {
 check(h==123 && id==456);
 if(write) std::memcpy(&v,&output,4);
 return returned;
}
static bool typed(uint64_t holder,uint32_t id,unsigned kind,uint32_t& bits) {
    // GetPropertyValue @10004b56e/5ac/636: RDI holder, ESI id, RDX host ref.
    if(kind==0) { int32_t v=0; std::memcpy(&v,&bits,4); bool ok=native(holder,id,v); std::memcpy(&bits,&v,4); return ok; }
    if(kind==1) return native(holder,id,bits);
    float v=0; std::memcpy(&v,&bits,4); bool ok=native(holder,id,v); std::memcpy(&bits,&v,4); return ok;
}
}
int main() {
 namespace m=mock; namespace f=fixture_placement_bindings;
 f::busy=false; check(m::inspectloadedaginginputs()); check(m::context->input_mode && m::context->owner.reload_allowed() && f::busy);
 check(!m::inspectplacedcheckpointreload() && m::queued==1);
 m::capture=false; m::callback(nullptr);check(m::ended && m::delayed==0 && m::captures==1);
 unsigned old=ends;m::end(true,"again");check(ends==old);
 m::ended=false;m::capture=true;m::begin(nullptr);check(m::delayed==1 && !m::ended);
 m::late_callback(nullptr);check(m::ended && m::captures==3);
 m::ended=false;m::throws=true;m::begin(nullptr);check(m::ended && f::busy);
 m::attempted=false;check(!m::inspectloadedaginginputs() && m::attempted);
 f::busy=false;m::attempted=false;m::ended=false;
 check(!m::inspectloadedaginginputs() && m::context && m::ended && f::busy);
 f::busy=false;m::attempted=false;m::ended=false;m::context=nullptr;m::allocation_throws=true;
 const unsigned before_reload=ends,before_aging=aging_ends,before_queue=m::queued,before_delayed=m::delayed;
 check(!m::inspectloadedaginginputs() && m::ended && !m::context && f::busy);
 check(m::attempted && m::terminal_input_mode && m::queued==before_queue && m::delayed==before_delayed);
 check(ends==before_reload+1 && aging_ends==before_aging+1 && reload_complete==0 && aging_complete==0);
 m::end(true,"repeat_after_allocation_failure");
 check(ends==before_reload+1 && aging_ends==before_aging+1);
 m::allocation_throws=false;
 // Old API selects old mode through the SAME actual queue helper.
 f::busy=false;m::attempted=false;m::ended=false;m::throws=false;
 check(m::inspectplacedcheckpointreload() && !m::context->input_mode && !m::terminal_input_mode);
 const unsigned old_mode_aging=aging_ends,old_mode_reload=ends;
 m::capture=false;m::callback(nullptr);
 check(ends==old_mode_reload+1 && aging_ends==old_mode_aging && reload_complete==0);
 m::end(true,"repeat_old_mode");check(ends==old_mode_reload+1 && aging_ends==old_mode_aging);
 // Old-mode allocation failure also emits only its reload terminal.
 f::busy=false;m::attempted=false;m::ended=false;m::context=nullptr;m::allocation_throws=true;
 const unsigned old_alloc_aging=aging_ends,old_alloc_reload=ends;
 check(!m::inspectplacedcheckpointreload() && m::ended && m::attempted && f::busy && !m::terminal_input_mode);
 check(ends==old_alloc_reload+1 && aging_ends==old_alloc_aging && reload_complete==0);
 for(unsigned kind=0;kind<3;++kind) {
  uint32_t bits=0x42c60000;typed_mock::returned=false;typed_mock::write=false;
  check(!typed_mock::typed(123,456,kind,bits) && bits==0x42c60000);
  typed_mock::returned=true;typed_mock::write=true;typed_mock::output=0;
  check(typed_mock::typed(123,456,kind,bits) && bits==0);
  typed_mock::returned=false;bits=0;
  check(!typed_mock::typed(123,456,kind,bits) && bits==0);
 }
 std::printf("aging_input_capture extracted lifecycle/typed checks=%u native_acquisition=mocked live_proof=0 allocation_failure_injection=1\n",checks);
}
