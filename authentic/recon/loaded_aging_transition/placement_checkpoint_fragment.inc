// clean-room fallback (not authentic): fixed, prepare-only diagnostic capture policy.
// Include after ALL actual tutorial checkpoint/reload and query fragments.
#ifndef OPENSC4_PLACEMENT_CHECKPOINT_FRAGMENT_INC
#define OPENSC4_PLACEMENT_CHECKPOINT_FRAGMENT_INC
#ifndef OPENSC4_PLACEMENT_CHECKPOINT_INTEGRATION
#error "define integration before bindings"
#endif
#ifndef OPENSC4_TUTORIAL_RELOAD_FRAGMENT_INC
#error "include actual tutorial reload first"
#endif
namespace placement_checkpoint {
using namespace fixture_placement;
namespace fb=fixture_placement_bindings;
namespace tc=tutorial_checkpoint;
#include "placement_checkpoint_saved_files.inc"
static constexpr char pref_path[]="/Users/russellmartin/Documents/SimCity 4/Sim City 4 Deluxe Edition.cfg";
struct Bridge { uint32_t pid=0,nonce=0; char original[65]{},receipt[65]{}; };
// Exact ASCII wire format, canonical decimals (no signs/leading zero), final LF.
static bool decimal(const char*& p,const char* end,uint32_t& n) {
    n=0; const char* start=p;
    while(p!=end && *p>='0' && *p<='9') {
        unsigned v=unsigned(*p++-'0'); if(n>(uint32_t(INT_MAX)-v)/10) return false; n=n*10+v;
    }
    return p!=start && *start!='0' && n && p!=end && *p++=='\n';
}
static bool digest(const char*& p,const char* end,char (&out)[65]) {
    if(end-p<65) return false;
    for(unsigned i=0;i<64;++i) { char c=*p++; if(!((c>='0'&&c<='9')||(c>='a'&&c<='f'))) return false; out[i]=c; }
    out[64]=0; return *p++=='\n';
}
static bool parse_bridge(const char* data,size_t size,Bridge& out) {
    out={}; static constexpr char magic[]="OPENSC4_PLACEMENT_BRIDGE_V1\n";
    if(!data || size>256 || size<sizeof(magic)-1 || std::memcmp(data,magic,sizeof(magic)-1)) return false;
    Bridge b{}; const char* p=data+sizeof(magic)-1; const char* end=data+size;
    if(!decimal(p,end,b.pid)||!decimal(p,end,b.nonce)||!digest(p,end,b.original)||!digest(p,end,b.receipt)||p!=end) return false;
    out=b; return true;
}
static const char* saved_hash(const char* p) {
    size_t n=sizeof(fb::region)-1;
    if(!p||std::strlen(p)<=n||std::strncmp(p,fb::region,n)||p[n]!='/'||!p[n+1]||std::strchr(p+n+1,'/')) return nullptr;
    for(const auto& f:saved_files) if(!std::strcmp(f.name,p+n+1)) return f.sha;
    return nullptr;
}
static bool saved_file(const char* p) {
    const char* hash=saved_hash(p); tc::File s{};
    return hash&&fb::file_hash(fb::saved_manifest,fb::saved_manifest_hash)&&tc::snapshot(p,s,false)&&!std::strcmp(s.sha,hash);
}
// ToChar @100017124 layout already used by query::feedback_copy. Policy requires
// stored length, full termination and no embedded NUL; zero length alone skips.
static bool native_string(const ScopedRZString& s,char (&out)[1024]) {
    unsigned tag=s.buf[8]; uint64_t n=tag>>1,p=reinterpret_cast<uint64_t>(s.buf)+9;
    if(tag&1) { std::memcpy(&n,s.buf+0x10,8); std::memcpy(&p,s.buf+0x18,8); }
    const bool ok=n<=1023 && ((tag&1)||n<=22) && read(p,out,size_t(n)+1) &&
        !out[n] && !std::memchr(out,0,size_t(n));
    // Log callers use %s even after failure. Preserve rejection before making
    // the bounded diagnostic buffer safe on every partial/unterminated read.
    out[1023]=0;
    return ok;
}
struct Protection { Bridge wire{}; tc::File bridge{},backup{},receipt{},runtime{}; };
static bool owned(const tc::File& f) { return f.present && f.st.st_uid==geteuid() && f.st.st_nlink==1; }
static bool protection(Protection& out) {
    char path[1024]{},buf[257]{};
    std::snprintf(path,sizeof path,"%s/placement_checkpoint_%d/protection.bridge",tc::backup_root,int(getpid()));
    if(!tc::snapshot(path,out.bridge,false)||!owned(out.bridge)||out.bridge.st.st_size>256) return false;
    int fd=open(path,O_RDONLY|O_NOFOLLOW); if(fd<0) return false;
    const ssize_t n=::read(fd,buf,sizeof buf); bool ok=close(fd)==0;
    tc::File again{};
    if(!ok||n!=out.bridge.st.st_size||!parse_bridge(buf,size_t(n),out.wire)||out.wire.pid!=uint32_t(getpid())||
       !tc::snapshot(path,again,false)||!tc::same_file(out.bridge,again)) return false;
    std::snprintf(path,sizeof path,"%s/tutorial_checkpoint_%u/preferences.original",tc::backup_root,out.wire.nonce);
    if(!tc::snapshot(path,out.backup,false)||!owned(out.backup)||std::strcmp(out.backup.sha,out.wire.original)) return false;
    std::snprintf(path,sizeof path,"%s/tutorial_checkpoint_%u/preferences_receipt.json",tc::backup_root,out.wire.nonce);
    if(!tc::snapshot(path,out.receipt,false)||!owned(out.receipt)||out.receipt.st.st_size>65536||std::strcmp(out.receipt.sha,out.wire.receipt)||
       !tc::snapshot(pref_path,out.runtime,false)||!owned(out.runtime)) return false;
    // External operator verifies full JSON and restoration capability. Native code
    // binds its entire hash; this is not ACL/xattr restoration proof.
    return out.backup.st.st_dev!=out.runtime.st.st_dev || out.backup.st.st_ino!=out.runtime.st.st_ino;
}
static bool equal_protection(const Protection& a,const Protection& b) {
    return a.wire.pid==b.wire.pid&&a.wire.nonce==b.wire.nonce&&tc::same_file(a.bridge,b.bridge)&&
        tc::same_file(a.backup,b.backup)&&tc::same_file(a.receipt,b.receipt)&&tc::same_file(a.runtime,b.runtime);
}
// Own separate <=4 output lists, published before native entry and never destroyed.
static tc::ListContext* lists[4]{}; static unsigned list_count=0;
static bool saved_neighbors(const State& s,tc::Neighbors& out) {
    if(!tc::app_interface(s)||list_count>=4) return false;
    uint64_t ns=0;
    if(!read(relocated(0x100b21448),ns)||!object(ns,0x100a6c410,8)||!slot(ns,0xa0,0x100072144)) return false;
    auto* ctx=new tc::ListContext; lists[list_count++]=ctx;
    // GetNeighborRegionalCities @100072144, original sentinel/node layout in tc.
    original<void(*)(uint64_t,void*,bool)>(0x100072144)(ns,&ctx->list,true);
    uint64_t values[128]{}; unsigned count=0;
    if(!tc::list_values(ctx->list,values,count)) return false;
    for(unsigned i=0;i<count;++i) if(!object(values[i],0x100aa3230,0x128)||!slot(values[i],0xf8,0x1003b2cd0)) return false;
    out.count=count;
    for(unsigned i=0;i<count;++i) {
        auto& item=out.items[i]; item.regional=values[i]; ScopedRZString p;
        original<void(*)(uint64_t,void*)>(0x1003b2cd0)(item.regional,p.buf);
        if(!native_string(p,item.path)||(item.path[0]&&!saved_file(item.path))) return false;
        logf_("PLACEMENT_CHECKPOINT NEIGHBOR index=%u regional=%llx path=%s native_skip_empty=%u",i,U(item.regional),item.path,unsigned(!item.path[0]));
    }
    return true;
}
struct Ready { pid_t pid=0; State state{}; uint64_t regional=0,tutorial=0; Protection protected_files{}; tc::Neighbors adjacent{}; };
static Ready* inspection=nullptr;
static const Ready* ready=nullptr;
static fb::Job* job=nullptr;
static fb::InventoryObservation* observation=nullptr;
struct Positive { const fb::Job* job=nullptr; pid_t pid=0; State state{}; size_t occurrence=0; uint64_t node=0,occupant=0; const fb::InventoryObservation* observed=nullptr; };
static Positive* positive_storage=nullptr;
static const Positive* positive=nullptr;
static std::atomic<bool> inspected{false},attempted{false};
static bool ended=false,spent=false,return_known=false,native_return=false;
static unsigned save_attempts=0;
static void end(bool complete,const char* reason) {
    if(ended) return; ended=true;
    logf_("PLACEMENT_CHECKPOINT END complete=%u reason=%s saveAttempts=%u return_known=%u native_bool=%u busy_held=1 contexts_retained=1 immediate_stop_required=1 noRetry=1",unsigned(complete),reason,save_attempts,unsigned(return_known),unsigned(native_return));
}
static bool tutorial(const State& s,uint64_t& system) {
    uint64_t ui=0; uint32_t task=0,guid=0; uint8_t active=0,cleanup=0,startup=0;
    bool ok=read(s.city+0x340,system)&&object(system,0x100a6b7c0,0x68)&&read(system+0x2f,active)&&read(system+0x60,ui)&&
        read(system+0x3c,cleanup)&&read(system+0x30,task)&&read(system+0x34,guid)&&read(system+0x3d,startup);
    logf_("PLACEMENT_CHECKPOINT RAW readable=%u system=%llx raw2f=%u raw60=%llx raw3c=%u raw30=%x raw34=%x raw3d=%u date=%u pause=%d,%d,%d",unsigned(ok),U(system),unsigned(active),U(ui),unsigned(cleanup),task,guid,unsigned(startup),s.date,s.pauses[0],s.pauses[1],s.pauses[2]);
    return ok&&!active&&!ui&&!cleanup;
}
// Reuse tc's guarded path acquisition, then require stored native lengths on
// every path used by this mode (tc's historical C-string policy is unchanged).
static bool strict_paths(const State& s,uint64_t& regional) {
    char path[1024]{},pref[1024]{};
    if(!tc::paths(s,path,regional,pref,false)) return false;
    ScopedRZString loaded,name,generated,rp,preferences;
    original<void(*)(uint64_t,void*)>(0x1000480ee)(s.city,loaded.buf);
    original<void(*)(uint64_t,void*)>(0x100048114)(s.city,name.buf);
    uint32_t serial=original<uint32_t(*)(uint64_t)>(0x1000480a8)(s.city);
    original<void(*)(const void*,uint32_t,void*)>(0x1001e98a4)(name.buf,serial,generated.buf);
    original<void(*)(uint64_t,void*)>(0x1003b2cd0)(regional,rp.buf);
    original<void(*)(uint64_t,void*)>(0x100013038)(s.app,preferences.buf);
    char a[1024]{},b[1024]{},c[1024]{},d[1024]{},n[1024]{};
    bool ok=native_string(loaded,a)&&native_string(name,n)&&native_string(generated,b)&&native_string(rp,c)&&native_string(preferences,d);
    logf_("PLACEMENT_CHECKPOINT PATH readable=%u loaded=%s generated=%s regional=%s preferences=%s",unsigned(ok),a,b,c,d);
    return ok&&tc::exact_path(a,b,c)&&!std::strcmp(d,pref_path);
}
static bool fresh(State& s,uint64_t& regional,uint64_t& system,bool hashes) {
    return tc::guards()&&(!hashes||(image_guard()&&fb::entries_guard()))&&state(s)&&strict_paths(s,regional)&&tutorial(s,system)&&
        object(s.view,0x100aec7b0,0x368)&&slot(s.view,0x1d8,0x100791d18)&&slot(s.view,0x1e0,0x10079280e)&&
        (!hashes||saved_file(fb::saved_path));
}
static bool validate(State& s,bool enumerate) {
    uint64_t r=0,t=0; Protection p{};
    if(!ready||ready->pid!=getpid()||!fresh(s,r,t,true)||!same(s,ready->state)||r!=ready->regional||t!=ready->tutorial||
       !protection(p)||!equal_protection(p,ready->protected_files)) return false;
    if(enumerate) { tc::Neighbors n{}; if(!saved_neighbors(s,n)||!tc::equal_neighbors(n,ready->adjacent)) return false; }
    return true;
}
static void inspect_exec(void*) {
    bool ok=false;
    logf_("PLACEMENT_CHECKPOINT BEGIN mode=inspect main_queue=%u",unsigned(pthread_main_np()!=0));
    try {
        auto& r=*inspection; r.pid=getpid();
        ok=fresh(r.state,r.regional,r.tutorial,true)&&protection(r.protected_files)&&saved_neighbors(r.state,r.adjacent);
        State last{}; uint64_t reg=0,t=0;
        ok=ok&&fresh(last,reg,t,true)&&same(last,r.state)&&reg==r.regional&&t==r.tutorial;
        if(ok) { ready=&r; logf_("PLACEMENT_CHECKPOINT READY pid=%d path=%s prefpath=%s originalSHA=%s runtimeSHA=%s date=%u pause=%d,%d,%d",int(r.pid),fb::saved_path,pref_path,r.protected_files.backup.sha,r.protected_files.runtime.sha,r.state.date,r.state.pauses[0],r.state.pauses[1],r.state.pauses[2]); fb::busy.store(false); }
    } catch(...) { ok=false; }
    if(!ok) end(false,"ready_failed");
}
static bool inspectplacementcheckpoint() {
    bool was=false; if(!fb::busy.compare_exchange_strong(was,true)) return false;
    was=false; if(!inspected.compare_exchange_strong(was,true)) { end(false,"inspect_spent"); return false; }
    try { inspection=new Ready; dispatch_async_f(dispatch_get_main_queue(),nullptr,inspect_exec); return true; }
    catch(...) { end(false,"inspect_allocation_or_queue_exception"); return false; }
}
static bool command_preflight(fb::Job& j) {
    logf_("PLACEMENT_CHECKPOINT BEGIN mode=capture pid=%d fixed_point=900,600",int(getpid()));
    State now{};
    return j.checkpoint_mode&&job==&j&&attempted.load()&&j.attempt&&j.context.target.x==900&&j.context.target.y==600&&
        validate(now,true)&&fb::receipt&&fb::receipt->inspected&&!fb::receipt->spent&&fb::receipt->kind==fb::FixtureKind::SavedInactive&&
        same(fb::receipt->receipt,now)&&fb::receipt->regional==ready->regional&&!std::strcmp(fb::receipt->path,fb::saved_path)&&fb::identity(fb::receipt.get(),now);
}
static bool eligible(const fb::Job& j,const fb::InventoryObservation& o,pid_t pid) {
    const auto& c=j.context;
    return ready&&ready->pid==pid&&job==&j&&j.checkpoint_mode&&j.attempt&&c.target.x==900&&c.target.y==600&&
        c.complete&&c.consumed&&c.catalog_calls==1&&c.move_calls==1&&c.click_calls==1&&fb::late_owner(j)&&
        j.late_first_ok&&j.late_second_ok&&same(j.late_expected,ready->state)&&same(j.late_first,j.late_expected)&&same(j.late_second,j.late_expected)&&
        o.pid==pid&&o.owner_spent&&o.regional==ready->regional&&same(o.owner_receipt,j.late_expected)&&
        c.verified_city==ready->state.city&&!std::strcmp(c.verified_path,fb::saved_path)&&!std::strcmp(c.manifest_sha256,fb::saved_manifest_hash)&&
        same(o.initial,j.late_second)&&same(o.final,j.late_second)&&fb::placement_candidate_matches(o);
}
static bool consume_save() {
    if(spent) return false;
    spent=true; ++save_attempts; return true;
}
static void save() {
    bool complete=false; const char* reason="presave_guard";
    try {
        auto run=[&]() -> bool {
            State now{};
            if(!positive||positive->job!=job||positive->pid!=getpid()||positive->observed!=observation||spent||!validate(now,true)||!same(now,positive->state)||
               !fb::identity(fb::receipt.get(),now)||!eligible(*job,*observation,getpid())) return false;
            // Scoped getter-produced native cRZString. Its destructor completes
            // before run returns and before terminal END, including C++ unwinding.
            ScopedRZString supplied; char path[1024]{};
            original<void(*)(uint64_t,void*)>(0x1000480ee)(now.city,supplied.buf);
            State last{};
            if(!native_string(supplied,path)||std::strcmp(path,fb::saved_path)||!validate(last,false)||!same(last,positive->state)) return false;
            logf_("PLACEMENT_CHECKPOINT PRESAVE app=%llx city=%llx sim=%llx regional=%llx view=%llx path=%s SaveDisabled=0 date=%u pause=%d,%d,%d",U(last.app),U(last.city),U(last.sim),U(ready->regional),U(last.view),path,last.date,last.pauses[0],last.pauses[1],last.pauses[2]);
            if(!consume_save()) return false;
            logf_("PLACEMENT_CHECKPOINT SAVE_ENTRY address=100046036 app=%llx bool_argument=0 saveAttempts=%u",U(last.app),save_attempts);
            // Exact original SaveCity @100046036, established tc ABI. False does
            // not confine writes or certify neighbor/subscriber/OS behavior.
            native_return=original<bool(*)(uint64_t,const void*,bool)>(0x100046036)(last.app,supplied.buf,false);
            return_known=true;
            logf_("PLACEMENT_CHECKPOINT SAVE_RETURN known=1 native_bool=%u neighbor_returns=unknown",unsigned(native_return));
            State after{}; uint64_t reg=0,t=0;
            // Fresh core/path/tutorial ONLY; never assert old prepared city hash.
            bool post=fresh(after,reg,t,false);
            logf_("PLACEMENT_CHECKPOINT POST readable=%u app=%llx city=%llx sim=%llx regional=%llx path=%s date=%u pause=%d,%d,%d neighbor_returns=unknown no_repair=1",unsigned(post),U(after.app),U(after.city),U(after.sim),U(reg),post?fb::saved_path:"unavailable",after.date,after.pauses[0],after.pauses[1],after.pauses[2]);
            reason=post?"save_return_observed":"poststate_incomplete";
            return post&&same(after,positive->state)&&reg==ready->regional&&t==ready->tutorial&&native_return;
        };
        complete=run();
    } catch(...) { reason="cpp_exception_native_faults_not_caught"; if(spent&&!return_known) logf_("PLACEMENT_CHECKPOINT SAVE_RETURN known=0"); }
    end(complete,reason);
}
static void late_exec(void* opaque) {
    auto* j=static_cast<fb::Job*>(opaque);
    if(!j||j!=job||j->late_entered||ended) return;
    j->late_entered=true; bool ok=false;
    try {
        if(pthread_main_np()&&fb::late_owner(*j)&&image_guard()&&fb::entries_guard()) {
            j->late_first_ok=state(j->late_first);
            if(j->late_first_ok&&fb::identity(fb::receipt.get(),j->late_first)) {
                j->late_second_ok=state(j->late_second);
                if(j->late_second_ok&&fb::identity(fb::receipt.get(),j->late_second))
                    ok=fb::inventory_observation(fb::receipt.get(),j->late_second,"PLACEMENT_CHECKPOINT_LATER",*observation);
            }
        }
        ok=ok&&eligible(*j,*observation,getpid());
        if(ok) {
            *positive_storage={j,getpid(),j->late_second,observation->selected_occurrence,observation->selected_node,observation->selected_occupant,observation};
            positive=positive_storage;
            logf_("PLACEMENT_CHECKPOINT RECEIPT pid=%d job=%llx occurrence=%zu node=%llx occupant=%llx",int(getpid()),U(reinterpret_cast<uint64_t>(j)),positive->occurrence,U(positive->node),U(positive->occupant));
        }
    } catch(...) { ok=false; }
    fb::late_state_log("first",j->late_expected,j->late_first,j->late_first_ok);
    fb::late_state_log("second",j->late_expected,j->late_second,j->late_second_ok);
    j->late_ended=true;
    logf_("FIXTURE LATE_END checkpoint_pending=%u immediate_stop_required=%u later_inventory_complete=%u queue_drained_claim=0",unsigned(ok),unsigned(!ok),unsigned(observation->complete));
    if(ok) save(); else end(false,"no_positive_receipt");
}
static bool transfer(std::unique_ptr<fb::Job>& j) {
    fb::Job* held=j.release(); // ALL outcomes quarantine heap Job, receipt and busy.
    if(held!=job||!held->context.complete||held->context.click_calls!=1) { end(false,"base_incomplete_no_callback"); return false; }
    try { dispatch_after_f(dispatch_time(DISPATCH_TIME_NOW,500*NSEC_PER_MSEC),dispatch_get_main_queue(),held,late_exec); return true; }
    catch(...) { end(false,"late_queue_exception"); return false; }
}
static bool attemptplacementcheckpoint() {
    bool was=false; if(!fb::busy.compare_exchange_strong(was,true)) return false;
    was=false; if(!attempted.compare_exchange_strong(was,true)) { end(false,"capture_spent"); return false; }
    // Consume before catalog. Publish all host allocations before mutation; never
    // static-RAII delete native-bearing contexts, including allocation failures.
    try {
        job=new fb::Job(true); job->attempt=true; job->context.target={900,600};
        observation=new fb::InventoryObservation; positive_storage=new Positive;
        dispatch_async_f(dispatch_get_main_queue(),job,fb::command_exec); return true;
    } catch(...) { end(false,"allocation_or_queue_exception"); return false; }
}
} // namespace placement_checkpoint
#endif
