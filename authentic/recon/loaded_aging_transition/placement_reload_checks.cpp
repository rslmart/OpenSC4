#include "placement_reload_syntax_stub.cpp"
#include <cassert>
#include <stdexcept>
static unsigned checks=0, ends=0;
static void check(bool b) { assert(b); ++checks; }
static void logf_(const char* f,...) { if(std::strstr(f,"PLACEMENT_RELOAD END ")) ++ends; }
static uint64_t get_app() { std::abort(); }
ScopedRZString::ScopedRZString() { std::memset(buf,0,sizeof buf); }
ScopedRZString::~ScopedRZString() {}
namespace profile_test {
using namespace fixture_placement_bindings;
static const char* disk=reload_sha;
static bool hash_ok=true,inactive=true,regular=true;
static bool file_hash(const char* p,const char* h) {
 if(!hash_ok) return false;
 if(!std::strcmp(p,manifest)) return !std::strcmp(h,manifest_hash);
 if(!std::strcmp(p,saved_manifest)) return !std::strcmp(h,saved_manifest_hash);
 if(!std::strcmp(p,reload_manifest)) return !std::strcmp(h,reload_manifest_hash);
 return !std::strcmp(h,disk);
}
static bool saved_file(const char*) { return regular; }
static bool saved_inactive(uint64_t) { return inactive; }
static FixtureKind fixture_profile(const char* path,uint64_t city,bool diagnostic=false) {
    const size_t n=sizeof(region)-1;
    if(std::strncmp(path,region,n) || path[n]!='/' || !path[n+1] || std::strchr(path+n+1,'/'))
        return FixtureKind::Unbound;
    const char* name=path+n+1;
    for(const Prepared& p:prepared)
        if(!std::strcmp(name,p.name) && file_hash(manifest,manifest_hash) && file_hash(path,p.sha))
            return FixtureKind::OriginalPrepared;
    if(!std::strcmp(path,saved_path) && saved_file(path) &&
       file_hash(saved_manifest,saved_manifest_hash) && file_hash(path,saved_sha) &&
       saved_file(path) && saved_inactive(city)) return FixtureKind::SavedInactive;
#ifdef OPENSC4_PLACEMENT_RELOAD_INTEGRATION
    if(diagnostic && !std::strcmp(path,saved_path) && saved_file(path) &&
       file_hash(reload_manifest,reload_manifest_hash) && file_hash(path,reload_sha) &&
       saved_file(path) && saved_inactive(city)) return FixtureKind::DiagnosticPlaced;
#else
    (void)diagnostic;
#endif
    return FixtureKind::Unbound;
}
}
namespace identity_test {
using namespace fixture_placement_bindings;
static FixtureKind chosen=FixtureKind::DiagnosticPlaced;
static bool loaded_path(const State&,char (&p)[1024],uint64_t& r,FixtureKind* k,bool allowed) { if(chosen==FixtureKind::DiagnosticPlaced&&!allowed) return false; std::strcpy(p,saved_path); r=22; *k=chosen; return true; }
static bool identity(void* opaque,const State& s) {
    Owner& o=*static_cast<Owner*>(opaque); char path[1024]={}; uint64_t regional=0;
    FixtureKind kind=FixtureKind::Unbound;
    bool diagnostic=false;
#ifdef OPENSC4_PLACEMENT_RELOAD_INTEGRATION
    diagnostic=o.reload_allowed();
#endif
    if(!identity_test::loaded_path(s,path,regional,&kind,diagnostic) || kind==FixtureKind::Unbound || (o.inspected &&
       (o.city!=s.city || o.regional!=regional || std::strcmp(o.path,path)))) return false;
    if(o.kind!=FixtureKind::Unbound && o.kind!=kind) return false;
    o.kind=kind; // legacy inspected Owners bind only after all prior comparisons pass
    return true;
}
}
namespace lifecycle_test {
using namespace placement_reload;
static placement_reload::Context* context=nullptr;
static std::atomic<bool> attempted{false},ended{false};
static unsigned queued=0,delayed=0; static bool capture=true,throws=false;
static dispatch_function_t callback=nullptr;
static bool pthread_main_np() { return true; }
static void sample(placement_reload::Context&,Sample& s,const char*) { s.full=s.core_ok=s.paths_ok=s.files_ok=s.tutorial_ok=s.inventory.complete=s.inventory.identity_complete=capture; s.profile=fb::FixtureKind::DiagnosticPlaced; }
static dispatch_queue_t dispatch_get_main_queue() { return nullptr; }
static dispatch_time_t dispatch_time(dispatch_time_t,int64_t d) { check(d==500LL*NSEC_PER_MSEC); return 0; }
static void dispatch_async_f(dispatch_queue_t,void*,dispatch_function_t cb) { ++queued; callback=cb; if(throws) throw std::runtime_error("queue"); }
static void dispatch_after_f(dispatch_time_t,dispatch_queue_t,void*,dispatch_function_t) { ++delayed; if(throws) throw std::runtime_error("queue"); }
static void later(void*) {}
static void end(bool ok,const char* reason) {
    if(ended.exchange(true)) return;
    logf_("PLACEMENT_RELOAD END complete=%u reason=%s profile=DiagnosticPlaced PID=%d manifest=%s citySHA=%s source_runtime_END_complete=0 immediate_stop_required=1 noRetry=1 busyheld=1 context_present=%u allocations_quarantined=1 no_deliberate_mutation=1 queue_drained_claim=0 lifecycle_complete_claim=0",unsigned(ok),reason,int(getpid()),fb::reload_manifest_hash,fb::reload_sha,unsigned(context!=nullptr));
}
static void begin(void*) {
    try {
        logf_("PLACEMENT_RELOAD BEGIN profile=DiagnosticPlaced PID=%d manifest=%s citySHA=%s source_runtime_END_complete=0 no_deliberate_mutation=1 native_query_cache_allocator_effects_possible=1",int(getpid()),fb::reload_manifest_hash,fb::reload_sha);
        if(!pthread_main_np()) { end(false,"initial_not_main"); return; }
        lifecycle_test::sample(*context,context->initial,"INITIAL");
        if(!readable(context->initial)) { end(false,"initial_incomplete"); return; }
        lifecycle_test::dispatch_after_f(lifecycle_test::dispatch_time(DISPATCH_TIME_NOW,500LL*NSEC_PER_MSEC),lifecycle_test::dispatch_get_main_queue(),nullptr,later);
    } catch(...) { end(false,"initial_or_queue_cpp_exception"); }
}
static bool inspectplacedcheckpointreload() {
    if(attempted.exchange(true)) return false;
    bool expected=false;
    if(!fb::busy.compare_exchange_strong(expected,true)) {
        logf_("PLACEMENT_RELOAD QUEUE accepted=0 reason=busy attempt_consumed=1"); return false;
    }
    try {
        context=new placement_reload::Context;
        lifecycle_test::dispatch_async_f(lifecycle_test::dispatch_get_main_queue(),nullptr,begin);
        logf_("PLACEMENT_RELOAD QUEUE accepted=1 completion=unknown busyheld=1");
        return true;
    } catch(...) { end(false,"allocation_or_queue_cpp_exception"); return false; }
    // Never release busy, delete context, acquire game references, or retry.
}
}
int main() {
using namespace placement_reload;
namespace f=fixture_placement_bindings;
namespace p=profile_test;
alignas(8) unsigned char city[0x348]{},system[0x68]{};
uint64_t sy=reinterpret_cast<uint64_t>(system),vt=0x100a6b7c0; std::memcpy(city+0x340,&sy,8); std::memcpy(system,&vt,8);
auto cityptr=reinterpret_cast<uint64_t>(city);
check(f::saved_inactive(cityptr));
for(unsigned x=1;x<256;++x) { system[0x2f]=x; check(!f::saved_inactive(cityptr)); } system[0x2f]=0;
uint64_t ui=0xf123456780000001; std::memcpy(system+0x60,&ui,8); check(!f::saved_inactive(cityptr)); ui=0; std::memcpy(system+0x60,&ui,8);
++vt; std::memcpy(system,&vt,8); check(!f::saved_inactive(cityptr));

check(p::fixture_profile(f::saved_path,1)==f::FixtureKind::Unbound);
check(p::fixture_profile(f::saved_path,1,true)==f::FixtureKind::DiagnosticPlaced);
for(bool* q:{&p::hash_ok,&p::regular,&p::inactive}) { *q=false; check(p::fixture_profile(f::saved_path,1,true)==f::FixtureKind::Unbound); *q=true; }
p::disk=f::saved_sha; check(p::fixture_profile(f::saved_path,1)==f::FixtureKind::SavedInactive);
for(const auto& v:f::prepared) { p::disk=v.sha; std::string path=std::string(f::region)+"/"+v.name; check(p::fixture_profile(path.c_str(),1)==f::FixtureKind::OriginalPrepared); }
placement_reload::Context c; f::Owner ordinary; check(!ordinary.reload_allowed() && c.owner.reload_allowed());
f::State st{}; st.city=1;
check(!identity_test::identity(&ordinary,st));
check(identity_test::identity(&c.owner,st));
identity_test::chosen=f::FixtureKind::SavedInactive; check(!identity_test::identity(&c.owner,st));
identity_test::chosen=f::FixtureKind::DiagnosticPlaced;
c.owner.inspected=true; c.owner.city=1; c.owner.regional=22; std::strcpy(c.owner.path,f::saved_path);
check(identity_test::identity(&c.owner,st)); st.city=2; check(!identity_test::identity(&c.owner,st));
f::InventoryObservation inv; f::observe_owner(inv,c.owner); check(inv.manifest_path==f::reload_manifest && inv.manifest_sha256==f::reload_manifest_hash);
check(!f::placement_candidate_matches(inv)); check(!f::receipt_allows_point(c.owner,{900,600}));
check(f::aging_fields(f::FixtureKind::DiagnosticPlaced)); check(!f::aging_fields(f::FixtureKind::OriginalPrepared));
Sample a; a.full=a.core_ok=a.paths_ok=a.files_ok=a.tutorial_ok=true;
a.inventory.complete=a.inventory.identity_complete=true; a.profile=f::FixtureKind::DiagnosticPlaced;
a.core.app=0xf123456700000001; a.core.city=0xf123456700000002; a.core.sim=3; a.core.view=4; a.core.renderer=5; a.core.pauses[0]=3;
a.regional=6; a.tutorial=7;
a.inventory.guarded_descriptor=0xf123456700000008;
a.inventory.occurrences.emplace_back(); auto& v=a.inventory.occurrences.back(); v.offset=0xc0; v.node=0xf123456700000009;
auto& o=v.value; o.complete=true; o.occupant=0xf123456700000010; o.lot.complete=true; o.lot.lot=0xf123456700000011; o.lot.descriptor=a.inventory.guarded_descriptor; o.tgi[0]=0x6534284a; o.tgi[1]=0xc8dbccba; o.tgi[2]=0x1f4b0000; o.lot.rect={58,66,63,72};
for(auto& q:o.properties) { q.complete=true; q.status=f::PropertyStatus::Supported; q.type=0xa; q.word_count=1; q.words[0]=0x7fc12345; }
o.lot.state=99; o.lot.condition=44;
Sample z=a;
for(auto& prop:o.properties) { prop.status=f::PropertyStatus::Unsupported; check(!target(o,a.inventory.guarded_descriptor)); prop.status=f::PropertyStatus::Supported; }
check(complete(a,z)); check(selection(a.inventory).occupant==0xf123456700000010);
for(auto q:{&z.core.app,&z.core.city,&z.core.sim,&z.core.view,&z.core.renderer,&z.regional,&z.tutorial}) { ++*q; check(!stable(a,z)); --*q; }
++z.core.date; check(!stable(a,z)); z=a;
for(unsigned i=0;i<3;++i) { for(int32_t n:{INT32_MIN,-1,0,1,2,INT32_MAX}) { z.core.pauses[i]=n; check(stable(a,z)==(n==a.core.pauses[i])); } z=a; }
z.path[0]='?'; check(!stable(a,z)); z=a; z.pref[0]='?'; check(!stable(a,z)); z=a; z.profile=f::FixtureKind::SavedInactive; check(!complete(a,z)); z=a;
for(bool* q:{&z.full,&z.core_ok,&z.paths_ok,&z.files_ok,&z.tutorial_ok,&z.inventory.complete,&z.inventory.identity_complete}) { *q=false; check(!complete(a,z)); *q=true; }
for(unsigned i=0;i<3;++i) { ++o.tgi[i]; check(!target(o,a.inventory.guarded_descriptor)); --o.tgi[i]; }
o.lot.lot=0; check(!target(o,a.inventory.guarded_descriptor)); o.lot.lot=1;
++o.lot.descriptor; check(!target(o,a.inventory.guarded_descriptor)); --o.lot.descriptor;
for(auto q:{&o.lot.rect.l,&o.lot.rect.t,&o.lot.rect.r,&o.lot.rect.b}) { ++*q; check(!target(o,a.inventory.guarded_descriptor)); --*q; }
a.inventory.occurrences.push_back(v); check(readable(a) && selection(a.inventory).count==2 && !complete(a,z));
a.inventory.occurrences.clear(); check(readable(a) && !complete(a,z));
ScopedRZString raw; char data[1024]; std::memset(data,'X',sizeof data); raw.buf[8]=1; uint64_t len=1023,ptr=reinterpret_cast<uint64_t>(data); std::memcpy(raw.buf+0x10,&len,8); std::memcpy(raw.buf+0x18,&ptr,8); char out[1024]{};
check(!pc::native_string(raw,out)); check(out[1023]==0 && std::strlen(out)==1023);
data[1023]=0; check(pc::native_string(raw,out)); data[100]=0; check(!pc::native_string(raw,out));
namespace l=lifecycle_test;
f::busy=false; check(l::inspectplacedcheckpointreload()); check(l::queued==1 && l::context && f::busy); check(!l::inspectplacedcheckpointreload());
l::capture=false; l::callback(nullptr); check(l::delayed==0 && l::ended && f::busy); unsigned n=ends; l::end(true,"again"); check(ends==n);
// Reset only host latches for independent scenarios, never production reset APIs.
l::ended=false; l::capture=true; l::begin(nullptr); check(l::delayed==1 && !l::ended);
l::throws=true; l::begin(nullptr); check(l::ended && f::busy);
l::attempted=false; check(!l::inspectplacedcheckpointreload()); check(l::attempted && f::busy);
f::busy=false; l::attempted=false; l::ended=false; check(!l::inspectplacedcheckpointreload()); check(l::context && l::ended && f::busy);
std::printf("placement_reload helper checks=%u native_acquisition=mocked live_proof=0\n",checks);
}
