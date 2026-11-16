// clean-room fallback (not authentic): ticket-local, fail-closed diagnostic policy.
// Include AFTER actual finish fragment; no RPC registration, no native execution approval.
#ifndef OPENSC4_TUTORIAL_CHECKPOINT_FRAGMENT_INC
#define OPENSC4_TUTORIAL_CHECKPOINT_FRAGMENT_INC
#ifndef OPENSC4_TUTORIAL_FINISH_FRAGMENT_INC
#error "include actual base/bindings/inspect/open/finish first"
#endif
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
namespace tutorial_checkpoint {
using namespace fixture_placement;
namespace fb=fixture_placement_bindings;
namespace tf=tutorial_finish;
static constexpr char expected_path[]="/Users/russellmartin/Documents/SimCity 4/Regions/_opensc4_aging_20261114/City - Rush Hour Tutorial.sc4";
static constexpr char backup_root[]="/Users/russellmartin/Documents/OpenSC4Fixtures/aging";
static constexpr char absent_marker[]="OpenSC4 tutorial checkpoint: preferences originally absent\n";
// Fresh SHA-verified original-image prefixes; checkpoint_binding_fresh.txt.
static constexpr Entry native_entries[]={
{0x100046036,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x81,0xec}},
{0x1001e98a4,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec}},
{0x1000480ee,{0x55,0x48,0x89,0xe5,0x48,0x8d,0x47,0x70,0x48,0x8b,0x0e,0x48,0x8b,0x49,0x78,0x48}},
{0x100048114,{0x55,0x48,0x89,0xe5,0x48,0x89,0xf8,0x48,0x8b,0x0e,0x48,0x8b,0x49,0x30,0x0f,0xb6}},
{0x1000480a8,{0x55,0x48,0x89,0xe5,0x8b,0x47,0x50,0x5d,0xc3,0x90,0x55,0x48,0x89,0xe5,0x89,0x77}},
{0x100013038,{0x55,0x48,0x89,0xe5,0x41,0x56,0x53,0x48,0x83,0xec,0x30,0x49,0x89,0xf6,0x48,0x89}},
{0x100048ef4,{0x55,0x48,0x89,0xe5,0x83,0xbf,0xbc,0x01,0x00,0x00,0x00,0x0f,0x95,0xc0,0x5d,0xc3}},
{0x100072144,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec}},
{0x10028b5e0,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x53,0x50,0x49,0x89,0xfe,0x49,0x83,0x7e}},
{0x10007264c,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x53,0x50,0x49,0x89,0xf7,0x49,0x89,0xfe}},
{0x1003b2cd0,{0x55,0x48,0x89,0xe5,0x48,0x8d,0x87,0x10,0x01,0x00,0x00,0x48,0x8b,0x0e,0x48,0x8b}},
{0x10000b034,{0x55,0x48,0x89,0xe5,0x48,0x83,0xc7,0xe8,0x5d,0xe9,0x76,0xff,0xff,0xff,0x55,0x48}},
{0x10001376e,{0x55,0x48,0x89,0xe5,0x48,0x8b,0x47,0x78,0x5d,0xc3,0x55,0x48,0x89,0xe5,0x48,0x8b}},
{0x100013778,{0x55,0x48,0x89,0xe5,0x48,0x8b,0x7f,0x78,0x48,0x8b,0x07,0x5d,0xff,0x60,0x40,0x90}},
{0x100013788,{0x55,0x48,0x89,0xe5,0x53,0x50,0x48,0x8b,0x87,0x80,0x00,0x00,0x00,0x48,0x85,0xc0}},
{0x100013824,{0x55,0x48,0x89,0xe5,0x48,0x8d,0x87,0x68,0x03,0x00,0x00,0x5d,0xc3,0x90,0x55,0x48}},
{0x100791d18,{0x55,0x48,0x89,0xe5,0x53,0x50,0x48,0x89,0xfb,0x48,0x83,0xbb,0x58,0x03,0x00,0x00}},
{0x10079280e,{0x55,0x48,0x89,0xe5,0x53,0x50,0x48,0x89,0xfb,0x48,0x8b,0xbb,0x60,0x03,0x00,0x00}},
};
static bool guards() {
    if(!pthread_main_np() || !tf::guards()) return false;
    for(const Entry& e:native_entries) { unsigned char b[16];
        if(!read(relocated(e.address),b,sizeof b) || std::memcmp(b,e.bytes,sizeof b)) return false;
    }
    return true;
}
// Every path component must already be canonical, with no symlink component.
// This deliberately rejects otherwise equivalent spellings. No filesystem writes.
static bool canonical(const char* p,bool absent=false) {
    if(!p || p[0]!='/' || std::strlen(p)>=1024) return false;
    char copy[1024]; std::strcpy(copy,p);
    for(char* q=copy+1;;++q) {
        if(*q && *q!='/') continue;
        const char saved=*q; *q=0; struct stat st{};
        const int rc=lstat(copy,&st); const int err=errno;
        if(rc && !(absent && !saved && err==ENOENT)) return false;
        if(!rc) {
            char resolved[PATH_MAX];
            if(S_ISLNK(st.st_mode) || !realpath(copy,resolved) || std::strcmp(copy,resolved)) return false;
            if(saved && !S_ISDIR(st.st_mode)) return false;
        }
        *q=saved; if(!saved) break;
    }
    return true;
}
struct File {
    bool present=false; char sha[65]{}; struct stat st{};
};
static bool metadata(const struct stat& a,const struct stat& b,bool identity=true) {
    return (!identity || (a.st_dev==b.st_dev && a.st_ino==b.st_ino)) &&
        a.st_size==b.st_size && a.st_mode==b.st_mode && a.st_uid==b.st_uid && a.st_gid==b.st_gid &&
        a.st_mtimespec.tv_sec==b.st_mtimespec.tv_sec && a.st_mtimespec.tv_nsec==b.st_mtimespec.tv_nsec &&
        (!identity || (a.st_ctimespec.tv_sec==b.st_ctimespec.tv_sec && a.st_ctimespec.tv_nsec==b.st_ctimespec.tv_nsec));
}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
// Unlike fb::file_hash, supports empty files and distinguishes ENOENT from error.
static bool snapshot(const char* p,File& out,bool allow_absent) {
    out=File{};
    if(!canonical(p,allow_absent)) return false;
    struct stat a{},z{},last{};
    if(lstat(p,&a)) {
        if(!allow_absent || errno!=ENOENT) return false;
        char parent[1024]; std::strcpy(parent,p); char* slash=std::strrchr(parent,'/');
        if(!slash || slash==parent) return false; *slash=0;
        return access(parent,R_OK|X_OK)==0;
    }
    if(!S_ISREG(a.st_mode) || a.st_nlink!=1 || a.st_size<0 || a.st_size>128*1024*1024) return false;
    int fd=open(p,O_RDONLY|O_NOFOLLOW); if(fd<0) return false;
    bool ok=!fstat(fd,&z) && metadata(a,z) && z.st_nlink==1;
    CC_SHA256_CTX ctx; CC_SHA256_Init(&ctx); unsigned char buf[16384],digest[32]; size_t total=0;
    while(ok) {
        ssize_t n=::read(fd,buf,sizeof buf);
        if(n<0) { if(errno==EINTR) continue; ok=false; break; }
        if(!n) break;
        total+=size_t(n); if(total>128*1024*1024) { ok=false; break; }
        CC_SHA256_Update(&ctx,buf,CC_LONG(n));
    }
    ok=ok && total==size_t(a.st_size) && !fstat(fd,&z) && metadata(a,z) && z.st_nlink==1;
    if(close(fd)) ok=false;
    ok=ok && !lstat(p,&last) && metadata(a,last) && last.st_nlink==1;
    if(!ok) return false;
    CC_SHA256_Final(digest,&ctx);
    for(unsigned i=0;i<32;++i) std::snprintf(out.sha+2*i,3,"%02x",digest[i]);
    out.present=true; out.st=a; return true;
}
#pragma clang diagnostic pop
static bool same_file(const File& a,const File& b,bool identity=true) {
    return a.present==b.present && (!a.present || (!std::strcmp(a.sha,b.sha) && metadata(a.st,b.st,identity)));
}
static bool protected_path(const char* p) {
    // Exact canonical app/source archive root excluded, plus disposable region.
    static constexpr char app[]="/Users/russellmartin/Library/Application Support/Steam/steamapps/common/SimCity 4 Deluxe";
    return !std::strncmp(p,app,sizeof(app)-1) ||
        !std::strncmp(p,fb::region,sizeof(fb::region)-1) || !std::strncmp(p,backup_root,sizeof(backup_root)-1);
}
static bool prepared_file(const char* p) {
    const size_t n=sizeof(fb::region)-1;
    if(std::strncmp(p,fb::region,n) || p[n]!='/' || !p[n+1] || std::strchr(p+n+1,'/') || !canonical(p)) return false;
    struct stat st{};
    if(lstat(p,&st) || !S_ISREG(st.st_mode) || st.st_nlink!=1 || !fb::file_hash(fb::manifest,fb::manifest_hash)) return false;
    for(const auto& entry:fb::prepared) if(!std::strcmp(p+n+1,entry.name)) return fb::file_hash(p,entry.sha);
    return false;
}
// Native SaveCity list representation @100046a92; clear @10028b5e0;
// push_back @10007264c: node prev+0,next+8,value+10, sentinel count+10.
struct alignas(8) NativeList { uint64_t prev,next,count; };
static_assert(sizeof(NativeList)==24,"original 24-byte representation");
struct Neighbor { uint64_t regional=0; char path[1024]{}; };
struct Neighbors { unsigned count=0; Neighbor items[128]{}; };
struct ListContext {
    NativeList list;
    ListContext() : list{reinterpret_cast<uint64_t>(&list),reinterpret_cast<uint64_t>(&list),0} {}
};
// No destructor, clear, or Release is ever called on a native output list.
static ListContext* lists[4]{}; static unsigned list_count=0;
static bool list_values(const NativeList& list,uint64_t (&values)[128],unsigned& count) {
    const uint64_t self=reinterpret_cast<uint64_t>(&list);
    if(list.count>128) return false;
    count=unsigned(list.count); uint64_t prev=self,node=list.next,seen[128]{};
    for(unsigned i=0;i<count;++i) {
        if(!node || node==self || (node&7)) return false;
        for(unsigned k=0;k<i;++k) if(seen[k]==node) return false;
        seen[i]=node; uint64_t raw[3]{};
        if(!read(node,raw,sizeof raw) || raw[0]!=prev || !raw[1] || !raw[2]) return false;
        uint64_t back=0; if(!read(raw[1],back) || back!=node) return false;
        values[i]=raw[2]; prev=node; node=raw[1];
    }
    return node==self && list.prev==prev && (count || list.next==self);
}
static bool optional_string(const ScopedRZString& s,char (&out)[1024]) {
    // fb::string_copy rejects empty; distinguish a confirmed empty native string.
    if(fb::string_copy(s,out)) return true;
    uint64_t p=reinterpret_cast<uint64_t>(s.buf)+9;
    if(s.buf[8]&1) std::memcpy(&p,s.buf+0x18,8);
    char first=1; return read(p,first) && !first && (out[0]=0,true);
}
static bool app_interface(const State& s) {
    uint64_t iface=0;
    // QI @10000afb8 +10000b034 proves cISC4App is primary+18, never primary.
    return s.app<=UINT64_MAX-0x18 && read(relocated(0x100b21258),iface) && iface==s.app+0x18 &&
        object(iface,0x100a67360,8) && slot(iface,0,0x10000b034) &&
        slot(iface,0x110,0x10001376e) && slot(iface,0x118,0x100013778) &&
        slot(iface,0x120,0x100013788) && slot(iface,0x178,0x100013824);
}
static bool neighbors(const State& s,Neighbors& out) {
    if(!app_interface(s) || list_count>=4) return false;
    uint64_t ns=0;
    if(!read(relocated(0x100b21448),ns) || !object(ns,0x100a6c410,8) || !slot(ns,0xa0,0x100072144)) return false;
    auto* ctx=new ListContext; lists[list_count++]=ctx; // publish before unknown native entry
    original<void(*)(uint64_t,void*,bool)>(0x100072144)(ns,&ctx->list,true);
    uint64_t values[128]{}; unsigned count=0;
    if(!list_values(ctx->list,values,count)) return false;
    out.count=count;
    // Validate ALL structural links before the first native pathname getter.
    for(unsigned i=0;i<count;++i) if(!object(values[i],0x100aa3230,0x128) || !slot(values[i],0xf8,0x1003b2cd0)) return false;
    for(unsigned i=0;i<count;++i) {
        auto& item=out.items[i]; item.regional=values[i]; ScopedRZString p;
        original<void(*)(uint64_t,void*)>(0x1003b2cd0)(item.regional,p.buf);
        if(!optional_string(p,item.path)) return false;
        logf_("TUTORIAL_CHECKPOINT NEIGHBOR index=%u regional=%llx path=%s native_skip_empty=%u",i,U(item.regional),item.path,unsigned(!item.path[0]));
        if(item.path[0] && !prepared_file(item.path)) return false;
    }
    return true;
}
static bool equal_neighbors(const Neighbors& a,const Neighbors& b) {
    if(a.count!=b.count) return false;
    for(unsigned i=0;i<a.count;++i) if(a.items[i].regional!=b.items[i].regional || std::strcmp(a.items[i].path,b.items[i].path)) return false;
    return true;
}
static bool exact_path(const char* loaded,const char* generated,const char* regional) {
    return !std::strcmp(loaded,expected_path) && !std::strcmp(loaded,generated) && !std::strcmp(loaded,regional);
}
static bool paths(const State& s,char (&path)[1024],uint64_t& regional,char (&pref)[1024],bool prepared=true) {
    uint64_t ref=0;
    if(!app_interface(s) || !object(s.app,0x100a67010,0x3a0) || !object(s.city,0x100a692e0,0x348) ||
       !slot(s.city,0x50,0x1000480ee) || !slot(s.city,0x60,0x100048114) || !slot(s.city,0x28,0x1000480a8) ||
       !slot(s.city,0x348,0x100048ef4) || !slot(s.app,0x2c0,0x100013038) || !slot(s.app,0x148,0x100046036) ||
       !read(s.app+0x98,ref) || !read(ref,regional) || !object(regional,0x100aa3230,0x128) || !slot(regional,0xf8,0x1003b2cd0)) return false;
    // ADAPTER DOMAIN: SaveCity itself does not check IsSaveDisabled @100048ef4.
    if(original<bool(*)(uint64_t)>(0x100048ef4)(s.city)) return false;
    ScopedRZString loaded,name,generated,rp,preferences;
    original<void(*)(uint64_t,void*)>(0x1000480ee)(s.city,loaded.buf);
    original<void(*)(uint64_t,void*)>(0x100048114)(s.city,name.buf);
    const uint32_t serial=original<uint32_t(*)(uint64_t)>(0x1000480a8)(s.city);
    original<void(*)(const void*,uint32_t,void*)>(0x1001e98a4)(name.buf,serial,generated.buf);
    original<void(*)(uint64_t,void*)>(0x1003b2cd0)(regional,rp.buf);
    original<void(*)(uint64_t,void*)>(0x100013038)(s.app,preferences.buf);
    char gen[1024]{},reg[1024]{};
    return fb::string_copy(loaded,path) && fb::string_copy(generated,gen) && fb::string_copy(rp,reg) &&
        exact_path(path,gen,reg) &&
        (!prepared || prepared_file(path)) && fb::string_copy(preferences,pref) && !protected_path(pref) && canonical(pref,true);
}
struct ReadyReceipt {
    pid_t pid=0; State state{}; uint64_t regional=0;
    char path[1024]{},pref[1024]{}; File preferences{}; Neighbors adjacent{};
};
// Published ONLY on successful main-queue inspection, immutable thereafter.
static const ReadyReceipt* ready=nullptr;
static ReadyReceipt* inspection_context=nullptr;
static std::atomic<bool> inspect_attempted{false};
static bool save_spent=false,checkpoint_ended=false,save_return_known=false,save_return=false;
static unsigned save_attempts=0;
static bool backup(const ReadyReceipt& r) {
    char dir[1024],file[1024];
    if(std::snprintf(dir,sizeof dir,"%s/tutorial_checkpoint_%d",backup_root,int(r.pid))>=int(sizeof dir) || !canonical(dir)) return false;
    struct stat st{}; if(lstat(dir,&st) || !S_ISDIR(st.st_mode)) return false;
    if(std::snprintf(file,sizeof file,"%s/%s",dir,r.preferences.present?"preferences.original":"preferences.absent")>=int(sizeof file)) return false;
    File saved{};
    if(!snapshot(file,saved,false) || !saved.present) return false;
    if(r.preferences.present) {
        return same_file(saved,r.preferences,false) &&
            !(saved.st.st_dev==r.preferences.st.st_dev && saved.st.st_ino==r.preferences.st.st_ino);
    }
    // Hash of the fixed marker bytes above; snapshot proves a complete bounded read.
    return saved.st.st_size==off_t(sizeof(absent_marker)-1) &&
        !std::strcmp(saved.sha,"4e29f417049ee870db30826c5ec73027fe6fcff44f94d60665fbca3f767f5cf7");
}
static void inspect_begin(void*) {
    bool ok=false;
    logf_("TUTORIAL_CHECKPOINT INSPECT_BEGIN main_queue=%u",unsigned(pthread_main_np()!=0));
    try {
        ReadyReceipt& r=*inspection_context; fb::Owner owner{};
        r.pid=getpid();
        ok=guards() && !tutorial_open::attempted.load() && !tf::attempted.load() && state(r.state) &&
            paths(r.state,r.path,r.regional,r.pref) && neighbors(r.state,r.adjacent) && snapshot(r.pref,r.preferences,true);
        if(ok) {
            owner.city=r.state.city; owner.regional=r.regional; std::strcpy(owner.path,r.path); owner.inspected=true;
            State last{};
            ok=fb::identity(&owner,r.state) && state(last) && same(r.state,last);
        }
        if(ok) {
            ready=&r;
            logf_("TUTORIAL_CHECKPOINT READY pid=%d app=%llx city=%llx sim=%llx regional=%llx path=%s prefpath=%s presence=%u sha256=%s size=%lld mode=%o uid=%u gid=%u mtime=%lld.%09ld date=%u pause=%d,%d,%d neighbors=%u",
                int(r.pid),U(r.state.app),U(r.state.city),U(r.state.sim),U(r.regional),r.path,r.pref,unsigned(r.preferences.present),r.preferences.sha,
                static_cast<long long>(r.preferences.st.st_size),unsigned(r.preferences.st.st_mode),unsigned(r.preferences.st.st_uid),unsigned(r.preferences.st.st_gid),
                static_cast<long long>(r.preferences.st.st_mtimespec.tv_sec),r.preferences.st.st_mtimespec.tv_nsec,r.state.date,r.state.pauses[0],r.state.pauses[1],r.state.pauses[2],r.adjacent.count);
            const auto& st=r.preferences.st;
            logf_("TUTORIAL_CHECKPOINT PREF_METADATA dev=%llu ino=%llu nlink=%u atime=%lld.%09ld ctime=%lld.%09ld birthtime=%lld.%09ld flags=%u",
                U(st.st_dev),U(st.st_ino),unsigned(st.st_nlink),static_cast<long long>(st.st_atimespec.tv_sec),st.st_atimespec.tv_nsec,
                static_cast<long long>(st.st_ctimespec.tv_sec),st.st_ctimespec.tv_nsec,static_cast<long long>(st.st_birthtimespec.tv_sec),st.st_birthtimespec.tv_nsec,unsigned(st.st_flags));
            fb::busy.store(false,std::memory_order_release);
        }
    } catch(...) { ok=false; }
    logf_("TUTORIAL_CHECKPOINT INSPECT_END complete=%u busy_held=%u context_retained=1 noRetry=1 immediate_stop_required=%u",unsigned(ok),unsigned(!ok),unsigned(!ok));
}
static bool inspectsavebindings() {
    bool expected=false;
    if(!fb::busy.compare_exchange_strong(expected,true)) return false;
    expected=false;
    if(!inspect_attempted.compare_exchange_strong(expected,true)) { fb::busy.store(false,std::memory_order_release); return false; }
    try {
        inspection_context=new ReadyReceipt;
        dispatch_async_f(dispatch_get_main_queue(),nullptr,inspect_begin); return true;
    } catch(...) { logf_("TUTORIAL_CHECKPOINT INSPECT_END complete=0 busy_held=1 noRetry=1 immediate_stop_required=1 allocation_or_queue_exception=1"); return false; }
}
static bool identities(const State& s) {
    return ready && ready->pid==getpid() && s.app==ready->state.app && s.city==ready->state.city && s.sim==ready->state.sim;
}
static bool attempt_available(bool receipt_present,bool spent) { return receipt_present && !spent; }
static bool validate(const State& s,bool enumerate) {
    if(!attempt_available(ready!=nullptr,save_spent) || !guards() || !identities(s)) return false;
    char path[1024]{},pref[1024]{}; uint64_t regional=0; File current{};
    if(!paths(s,path,regional,pref) || regional!=ready->regional || std::strcmp(path,ready->path) || std::strcmp(pref,ready->pref) ||
       !snapshot(pref,current,true) || !same_file(current,ready->preferences) || !backup(*ready)) return false;
    if(enumerate) { Neighbors n{}; if(!neighbors(s,n) || !equal_neighbors(n,ready->adjacent)) return false; }
    return true;
}
static bool preflight(void* opaque) {
    auto& j=*static_cast<tf::Job*>(opaque);
    if(!j.checkpoint_mode || j.inspect_only || !tutorial_open::successful_receipt.valid || !identities(j.before) ||
       j.before.date!=ready->state.date || j.before.pauses[0]!=ready->state.pauses[0]) return false;
    State now{};
    return state(now) && same(j.before,now) && validate(now,true);
}
static bool tutorial_fields(const State& s,uint64_t expected,uint8_t& active,uint8_t& cleanup) {
    uint64_t system=0,ui=0; uint8_t flag=0; uint32_t task=0,guid=0;
    if(!read(s.city+0x340,system) || system!=expected || !object(system,0x100a6b7c0,0x68) ||
       !read(system+0x2f,active) || !read(system+0x60,ui) || !read(system+0x3c,cleanup) ||
       !read(system+0x30,task) || !read(system+0x34,guid) || !read(system+0x3d,flag)) return false;
    logf_("TUTORIAL_CHECKPOINT RAW system=%llx raw2f=%u raw60=%llx raw3c=%u raw30=%x raw34=%x raw3d=%u date=%u pause=%d,%d,%d",
        U(system),unsigned(active),U(ui),unsigned(cleanup),task,guid,unsigned(flag),s.date,s.pauses[0],s.pauses[1],s.pauses[2]);
    return !active && !ui;
}
static void checkpoint_end(bool complete,const char* reason) {
    if(checkpoint_ended) return; checkpoint_ended=true;
    logf_("TUTORIAL_CHECKPOINT END complete=%u reason=%s saveAttempts=%u return_known=%u native_bool=%u busy_held=1 all_old_refs_retained=1 immediate_process_stop_required=1 noRetry=1",
        unsigned(complete),reason,save_attempts,unsigned(save_return_known),unsigned(save_return));
}
static void save_after_exit(void* opaque) {
    auto& j=*static_cast<tf::Job*>(opaque);
    logf_("TUTORIAL_CHECKPOINT BEGIN exit_accepted=1 main_queue=%u",unsigned(pthread_main_np()!=0));
    const char* reason="presave_guard"; bool complete=false;
    try {
        auto run=[&]() -> bool {
            State now{}; uint8_t active=0,cleanup=0;
            if(!j.checkpoint_mode || j.inspect_only || !guards() || !tf::exit_state(now) || !validate(now,true) ||
               !tutorial_fields(now,j.first.tutorial,active,cleanup) || cleanup!=1) return false;
            uint64_t view=0;
            // Fresh current primary view, NEVER any retained UI/window/task pointer.
            // Menu cleanup endTutorial @10005f428/434 -> primary slots1e0/1d8.
            if(!read(relocated(0x100b214b8),view) || !object(view,0x100aec7b0,0x368) ||
               !slot(view,0x1d8,0x100791d18) || !slot(view,0x1e0,0x10079280e)) return false;
            char path[1024]{},pref[1024]{}; uint64_t regional=0;
            if(!paths(now,path,regional,pref) || regional!=ready->regional || std::strcmp(pref,ready->pref)) return false;
            ScopedRZString supplied;
            // Obtain the exact same native string through the original pathname getter.
            original<void(*)(uint64_t,void*)>(0x1000480ee)(now.city,supplied.buf);
            char exact[1024]{};
            if(!fb::string_copy(supplied,exact) || std::strcmp(exact,expected_path) || !validate(now,false)) return false;
            logf_("TUTORIAL_CHECKPOINT PRESAVE path=%s city=%llx sim=%llx regional=%llx current_view=%llx saveDisabled=0",exact,U(now.city),U(now.sim),U(regional),U(view));
            // One-shot consumed BEFORE native entry, including unknown/exception result.
            save_spent=true; ++save_attempts;
            logf_("TUTORIAL_CHECKPOINT SAVE_ENTRY address=100046036 app=%llx bool_argument=0 saveAttempts=%u",U(now.app),save_attempts);
            save_return=original<bool(*)(uint64_t,const void*,bool)>(0x100046036)(now.app,supplied.buf,false);
            save_return_known=true;
            logf_("TUTORIAL_CHECKPOINT SAVE_RETURN known=1 native_bool=%u saveAttempts=%u",unsigned(save_return),save_attempts);
            reason="poststate_guard";
            State after{}; uint8_t a=0,c=0; char postpath[1024]{},postpref[1024]{}; uint64_t postregional=0;
            // Fresh core and pathname reads ONLY. The saved city hash has changed:
            // fb::identity is intentionally NOT used after native SaveCity.
            const bool post=tf::exit_state(after) && identities(after) && guards() &&
                paths(after,postpath,postregional,postpref,false) && postregional==ready->regional &&
                !std::strcmp(postpref,ready->pref) && tutorial_fields(after,j.first.tutorial,a,c);
            logf_("TUTORIAL_CHECKPOINT POSTSTATE readable=%u cleanup_expected_zero=%u cleanup_raw=%u native_save_bool=%u no_repair=1 neighbor_write_results=unknown",
                unsigned(post),unsigned(post && c==0),unsigned(c),unsigned(save_return));
            reason=post?(c==0?"save_return_observed":"cleanup_discrepancy_no_repair"):"poststate_incomplete";
            return post && c==0 && save_return;
        };
        complete=run();
    } catch(...) { reason="cpp_exception_native_faults_not_caught"; }
    checkpoint_end(complete,reason);
}
static void finish_end(void* opaque,bool exit_complete) {
    auto& j=*static_cast<tf::Job*>(opaque);
    if(j.checkpoint_mode && !checkpoint_ended) checkpoint_end(false,exit_complete?"missing_save_hook":"finish_exit_incomplete");
}
static bool finishtutorialcheckpoint() {
    // Exact noarg entry; shared FINISH attempt/busy/quarantine gate.
    return tf::queue_tutorial_response<false,true>();
}
} // namespace tutorial_checkpoint
#endif
