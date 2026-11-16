// Pure host lifecycle checks against actual adapter helpers. No game/native calls.
#include <dispatch/dispatch.h>
#include <cassert>
#include <cstdio>
#include <string>
#include <stdexcept>
static unsigned queued=0;
static bool queue_throws=false;
static void* queued_context=nullptr;
static dispatch_function_t queued_callback=nullptr;
static dispatch_time_t fake_time(dispatch_time_t when,int64_t delta) {
    assert(when==DISPATCH_TIME_NOW && delta==500*NSEC_PER_MSEC); return 123;
}
static dispatch_queue_t fake_main_queue() { return nullptr; }
static void fake_after(dispatch_time_t when,dispatch_queue_t q,void* context,dispatch_function_t callback) {
    assert(when==123 && !q); ++queued;
    if(queue_throws) throw std::runtime_error("pure queue failure");
    queued_context=context; queued_callback=callback;
}
#define dispatch_time fake_time
#define dispatch_get_main_queue fake_main_queue
#define dispatch_after_f fake_after
#include "saved_fixture_binding_syntax_stub.cpp"
#undef dispatch_time
#undef dispatch_get_main_queue
#undef dispatch_after_f
static std::string logs;
static unsigned checks=0;
static void logf_(const char* fmt,...) {
    char text[8192]; va_list ap; va_start(ap,fmt);
    const int n=vsnprintf(text,sizeof text,fmt,ap); va_end(ap);
    assert(n>=0 && size_t(n)<sizeof text); logs+=text; logs+='\n';
}
static uint64_t get_app() { std::abort(); }
ScopedRZString::ScopedRZString() { std::abort(); }
ScopedRZString::~ScopedRZString() { std::abort(); }
namespace fb=fixture_placement_bindings;
static void check(bool b) { assert(b); ++checks; }
static unsigned occurrences(const char* needle) {
    unsigned n=0; for(size_t p=0;(p=logs.find(needle,p))!=std::string::npos;p+=std::strlen(needle)) ++n;
    return n;
}
int main() {
    using namespace fb;
    for(bool attempt:{false,true}) for(auto kind:{FixtureKind::Unbound,FixtureKind::OriginalPrepared,FixtureKind::SavedInactive})
        for(unsigned clicks:{0u,1u,2u,UINT_MAX}) for(bool complete:{false,true}) {
            Job j; j.attempt=attempt; j.context.click_calls=clicks; j.context.complete=complete;
            const LateScope wanted=(!attempt || kind!=FixtureKind::SavedInactive || clicks==0)?LateScope::None:
                clicks==1?LateScope::One:LateScope::Invalid;
            check(late_scope(j.attempt,j.context.click_calls,kind)==wanted);
        }
    check(aging_fields(FixtureKind::SavedInactive));
    check(!aging_fields(FixtureKind::OriginalPrepared) && !aging_fields(FixtureKind::Unbound));
    check(sizeof aging_ids/sizeof *aging_ids==4 && aging_ids[0]==0x28ecf510u && aging_ids[1]==0xea1cf220u &&
          aging_ids[2]==0xea1cf221u && aging_ids[3]==0xea1cf222u);
    for(uint32_t type:{0u,1u,6u,7u,0xau,0xbu,0x800au,0x8006u,UINT_MAX}) {
        logs.clear(); const bool expected=type==6 || type==7 || type==0xa || type==0x800a;
        check(log_property_domain(0x11,0x28ecf510,0x22,0x2a,type,257)==expected);
        check(occurrences("FIXTURE PROPERTY_DOMAIN ")==1 && logs.find("count=257")!=std::string::npos);
        check(logs.find(expected?"supported=1":"supported=0")!=std::string::npos && occurrences("FIXTURE VALUE")==0);
    }
    State expected{}; expected.app=1; expected.city=2; expected.sim=3;
    State fresh=expected; fresh.date=UINT_MAX; fresh.pauses[0]=INT_MAX;
    check(late_ids(expected,fresh)); // dates/pauses observed, never repaired
    fresh.app++; check(!late_ids(expected,fresh)); fresh=expected;
    fresh.city++; check(!late_ids(expected,fresh)); fresh=expected;
    fresh.sim++; check(!late_ids(expected,fresh));
    receipt.reset(new Owner); receipt->kind=FixtureKind::SavedInactive; receipt->inspected=true; receipt->spent=true;
    Job owner_job; owner_job.context.owner=receipt.get(); check(late_owner(owner_job));
    receipt->spent=false; check(!late_owner(owner_job)); receipt->spent=true;
    receipt->inspected=false; check(!late_owner(owner_job)); receipt->inspected=true;
    receipt->kind=FixtureKind::OriginalPrepared; check(!late_owner(owner_job)); receipt->kind=FixtureKind::SavedInactive;
    owner_job.context.owner=&owner_job.candidate; check(!late_owner(owner_job));
    // None retains unique ownership and normal Unlock semantics, with no queue.
    busy.store(true); { Unlock unlock; std::unique_ptr<Job> j(new Job);
        check(!late_transfer(j,unlock,LateScope::None)); check(j && unlock.release && queued==0); }
    check(!busy.load());
    // Test synthetic jobs only. Production never resets/deletes the quarantine.
    for(bool base_complete:{false,true}) for(bool throws:{false,true}) {
        logs.clear(); queued=0; queued_callback=nullptr; queue_throws=throws; busy.store(true);
        Job* held=nullptr;
        { Unlock unlock; std::unique_ptr<Job> j(new Job); held=j.get();
          j->attempt=true; j->context.click_calls=1; j->context.complete=base_complete;
          // Invalid owner makes actual callback fail before any game access.
          check(late_transfer(j,unlock,LateScope::One));
          check(!j && !unlock.release && late_context==held && queued==1); }
        check(busy.load()); check(occurrences("FIXTURE COMMAND_END ")==1);
        check(logs.find("late_pending=1")!=std::string::npos);
        if(!throws) { check(queued_callback && queued_context==held); queued_callback(queued_context); queued_callback(queued_context); }
        check(occurrences("FIXTURE LATE_END ")==1);
        check(occurrences("FIXTURE LATE_BEGIN ")==unsigned(!throws));
        check(logs.find("later_inventory_complete=0")!=std::string::npos && logs.find("fixture_success=not_inferred")!=std::string::npos);
        check(busy.load() && queued==1 && held->late_ended);
        late_end(*held,true,"duplicate_must_not_emit"); check(occurrences("FIXTURE LATE_END ")==1);
        delete held; late_context=nullptr;
    }
    logs.clear(); queued=0; busy.store(true);
    { Unlock unlock; std::unique_ptr<Job> j(new Job); j->attempt=true; j->context.click_calls=2;
      check(late_transfer(j,unlock,LateScope::Invalid)); check(!j && !unlock.release && !queued); }
    check(busy.load() && occurrences("FIXTURE LATE_END ")==1 && logs.find("invalid_click_scope_no_retry")!=std::string::npos);
    delete late_context; late_context=nullptr;
    std::printf("placement late pure checks: %u passed; no game calls or real dispatch\n",checks);
}
