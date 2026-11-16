// Host-only tests of ACTUAL helpers; no native game call or launch.
#include <dispatch/dispatch.h>
#include <stdexcept>
#include <cassert>
#include <string>
static unsigned queued=0;
static bool queue_throws=false;
static dispatch_time_t fake_time(dispatch_time_t,int64_t delta) { assert(delta==500*NSEC_PER_MSEC); return 1; }
static dispatch_queue_t fake_queue() { return nullptr; }
static void fake_after(dispatch_time_t,dispatch_queue_t,void*,dispatch_function_t) {
    ++queued; if(queue_throws) throw std::runtime_error("queue");
}
#define dispatch_time fake_time
#define dispatch_get_main_queue fake_queue
#define dispatch_after_f fake_after
#include "placement_checkpoint_syntax_stub.cpp"
#undef dispatch_time
#undef dispatch_get_main_queue
#undef dispatch_after_f
static std::string logs;
static void logf_(const char* fmt,...) {
    char text[8192]; va_list ap; va_start(ap,fmt); int n=vsnprintf(text,sizeof text,fmt,ap); va_end(ap);
    assert(n>=0 && size_t(n)<sizeof text); logs+=text; logs+='\n';
}
static uint64_t get_app() { std::abort(); }
ScopedRZString::ScopedRZString() { std::memset(buf,0,sizeof buf); }
ScopedRZString::~ScopedRZString() {}
namespace fb=fixture_placement_bindings;
namespace pc=placement_checkpoint;
static unsigned checks=0;
static void check(bool v) { assert(v); ++checks; }
static fb::InventoryObservation fixed() {
    using namespace fb;
    InventoryObservation o;
    o.identity_complete=true; o.owner_inspected=true; o.owner_spent=true; o.pid=15730;
    o.kind=FixtureKind::SavedInactive;
    o.initial.app=0xf123456700000001; o.initial.city=0xf123456700000002;
    o.initial.sim=0xf123456700000003; o.initial.view=0xf123456700000004;
    o.initial.renderer=0xf123456700000005; o.initial.date=2455319; o.initial.pauses[0]=2;
    o.owner_receipt=o.initial; o.city=o.initial.city; o.regional=0xf123456700000006;
    o.guarded_descriptor=0xf123456700000007;
    o.region_path=region; o.path=saved_path; o.manifest_path=saved_manifest; o.manifest_sha256=saved_manifest_hash;
    o.list_counts[0]=1;
    o.occurrences.emplace_back(); auto& v=o.occurrences.back();
    v.offset=0xc0; v.node=0xf123456700000008;
    auto& p=v.value;
    p.complete=true; p.occupant=0xf123456700000108; p.base=p.occupant-8;
    p.holder=p.occupant+0x10; p.exemplar=0xf123456700000009;
    p.tgi[0]=0x6534284a; p.tgi[1]=0xc8dbccba; p.tgi[2]=0x1f4b0000;
    p.lot.complete=true; p.lot.lot=0xf12345670000000a; p.lot.descriptor=o.guarded_descriptor;
    p.lot.rect={58,66,63,72}; p.lot.state=3;
    const uint32_t ids[]={0xc8ebd131,0xa8ebcf0c,0x88ed1059,0x27812852,0xc910b4cf,
        0x8a1e07ee,0x28ecf510,0xea1cf220,0xea1cf221,0xea1cf222};
    const uint32_t types[]={6,0,0,0,0xa,0x800a,7,0xa,0xa,0xa};
    const uint32_t scalars[]={0,0,0,0,0,0,0,0x3a919ef9,0x3f800000,0x3f8ccccd};
    const uint32_t curve[]={0x00000000,0x3f800000,0x3f000000,0x3f733333,0x3f266666,
        0x3f666666,0x3f400000,0x3f4ccccd,0x3f666666,0x3f0ccccd,0x3f800000,0x3e99999a,0x3f8ccccd,0x00000000};
    for(unsigned i=0;i<10;++i) {
        auto& r=p.properties[i]; r.id=ids[i]; r.type=types[i]; r.complete=true;
        r.status=i>=1 && i<=3?PropertyStatus::Missing:PropertyStatus::Supported;
        if(r.status==PropertyStatus::Supported) { r.word_count=1; r.words[0]=scalars[i]; }
    }
    p.properties[5].count=14; p.properties[5].word_count=14;
    std::memcpy(p.properties[5].words,curve,sizeof curve);
    finish_inventory_observation(o,o.initial,true);
    return o;
}

int main() {
    const std::string valid="OPENSC4_PLACEMENT_BRIDGE_V1\n123\n456\n"+std::string(64,'a')+"\n"+std::string(64,'b')+"\n";
    pc::Bridge bridge{};
    check(pc::parse_bridge(valid.data(),valid.size(),bridge)); check(bridge.pid==123&&bridge.nonce==456);
    for(size_t n=0;n<valid.size();++n) check(!pc::parse_bridge(valid.data(),n,bridge));
    for(const auto& number:{"0","-1","+1","01","2147483648","4294967296","99999999999999999999999","","1 2"}) {
        std::string bad="OPENSC4_PLACEMENT_BRIDGE_V1\n"+std::string(number)+"\n456\n"+std::string(64,'a')+"\n"+std::string(64,'b')+"\n";
        check(!pc::parse_bridge(bad.data(),bad.size(),bridge));
    }
    for(size_t i=0;i<valid.size();++i) { auto bad=valid; bad[i]='\0'; check(!pc::parse_bridge(bad.data(),bad.size(),bridge)); }
    auto extra=valid+"x"; check(!pc::parse_bridge(extra.data(),extra.size(),bridge));
    for(char c:{'A','g','/','\r'}) { auto bad=valid; bad[bad.size()-2]=c; check(!pc::parse_bridge(bad.data(),bad.size(),bridge)); }
    check(pc::saved_hash(fb::saved_path)&&!std::strcmp(pc::saved_hash(fb::saved_path),fb::saved_sha));
    for(const auto& f:pc::saved_files) { std::string p=std::string(fb::region)+"/"+f.name; check(pc::saved_hash(p.c_str())&&!std::strcmp(pc::saved_hash(p.c_str()),f.sha)); }
    for(const std::string& p:{std::string(fb::region),std::string(fb::region)+"/",std::string(fb::region)+"/../City - Rush Hour Tutorial.sc4",std::string(fb::region)+"x/City - Rush Hour Tutorial.sc4",std::string(fb::region)+"/City - Rush Hour Tutorial.sc4.bak",std::string("/tmp/City - Rush Hour Tutorial.sc4")}) check(!pc::saved_hash(p.c_str()));
    // Stored length versus first-byte NUL, small/heap, embedded NUL and bounds.
    ScopedRZString str; char out[1024]{};
    check(pc::native_string(str,out)&&!out[0]);
    str.buf[8]=4; str.buf[9]='a'; str.buf[10]='b'; str.buf[11]=0;
    check(pc::native_string(str,out)&&!std::strcmp(out,"ab"));
    str.buf[9]=0; check(!pc::native_string(str,out));
    str.buf[8]=46; check(!pc::native_string(str,out));
    char heap[1024]{}; uint64_t pointer=reinterpret_cast<uint64_t>(heap),len=1023;
    std::memset(heap,'a',1023); str.buf[8]=1; std::memcpy(str.buf+0x10,&len,8); std::memcpy(str.buf+0x18,&pointer,8);
    check(pc::native_string(str,out)); heap[1023]='a'; check(!pc::native_string(str,out));
    check(out[1023]==0); // failed full-buffer capture must remain safe for %s
    heap[1023]=0;
    len=1024; std::memcpy(str.buf+0x10,&len,8); check(!pc::native_string(str,out));
    len=1; heap[0]=0; std::memcpy(str.buf+0x10,&len,8); check(!pc::native_string(str,out));
    // Core authorization uses actual prior-worker candidate and actual Job binding.
    auto o=fixed(); pc::Ready r; r.pid=15730; r.state=o.initial; r.regional=o.regional; pc::ready=&r;
    fb::Job j(true); pc::job=&j; j.attempt=true; j.context.target={900,600}; j.context.complete=true;
    j.context.verified_city=r.state.city; std::strcpy(j.context.verified_path,fb::saved_path); std::strcpy(j.context.manifest_sha256,fb::saved_manifest_hash);
    j.context.consumed=true; j.context.catalog_calls=j.context.move_calls=j.context.click_calls=1;
    j.late_expected=j.late_first=j.late_second=o.initial; j.late_first_ok=j.late_second_ok=true;
    fb::receipt.reset(new fb::Owner); auto& owner=*fb::receipt; owner.inspected=owner.spent=true;
    owner.kind=fb::FixtureKind::SavedInactive; j.context.owner=&owner;
    auto valid_job=[&] { return pc::eligible(j,o,15730); }; check(valid_job());
    check(!pc::eligible(j,o,15731));
    for(unsigned k=0;k<3;++k) { ++j.late_first.pauses[k]; check(!valid_job()); --j.late_first.pauses[k]; ++j.late_second.pauses[k]; check(!valid_job()); --j.late_second.pauses[k]; }
    ++j.late_second.date; check(!valid_job()); --j.late_second.date;
    ++j.late_expected.date; check(!valid_job()); --j.late_expected.date;
    for(unsigned* count:{&j.context.catalog_calls,&j.context.move_calls,&j.context.click_calls}) {
        *count=0; check(!valid_job()); *count=2; check(!valid_job()); *count=1;
    }
    j.context.target.x=901; check(!valid_job()); j.context.target.x=900;
    j.context.target.y=601; check(!valid_job()); j.context.target.y=600;
    j.context.complete=false; check(!valid_job()); j.context.complete=true;
    owner.kind=fb::FixtureKind::OriginalPrepared; check(!valid_job()); owner.kind=fb::FixtureKind::SavedInactive;
    owner.spent=false; check(!valid_job()); owner.spent=true;
    for(bool* flag:{&j.attempt,&j.context.complete,&j.context.consumed,&j.late_first_ok,&j.late_second_ok,&owner.inspected,&owner.spent}) {
        *flag=false; check(!valid_job()); *flag=true;
    }
    for(uint64_t* value:{&j.late_first.app,&j.late_first.city,&j.late_first.sim,&j.late_second.view,&j.late_second.renderer,&o.regional}) {
        ++*value; check(!valid_job()); --*value;
    }
    ++o.owner_receipt.date; check(!valid_job()); --o.owner_receipt.date;
    o.owner_spent=false; check(!valid_job()); o.owner_spent=true;
    j.context.verified_path[0]='X'; check(!valid_job()); std::strcpy(j.context.verified_path,fb::saved_path);
    j.context.manifest_sha256[0]='X'; check(!valid_job()); std::strcpy(j.context.manifest_sha256,fb::saved_manifest_hash);
    auto keep=o; o.occurrences.clear(); o.list_counts[0]=0; fb::finish_inventory_observation(o,o.initial,true);
    check(o.complete&&!valid_job()); o=keep;
    o.occurrences.push_back(o.occurrences.front()); fb::finish_inventory_observation(o,o.initial,true); check(!valid_job()); o=keep;
    fb::Job ordinary; check(!ordinary.checkpoint_mode); check(!pc::eligible(ordinary,o,15730));
    // Original and READY runtime bytes may differ. Both independent snapshots
    // stay pinned; inode/content/metadata changes at either baseline reject.
    pc::Protection a; a.wire.pid=123; a.wire.nonce=456;
    for(auto* f:{&a.bridge,&a.backup,&a.receipt,&a.runtime}) { f->present=true; f->st.st_nlink=1; f->st.st_uid=geteuid(); std::strcpy(f->sha,"aaa"); }
    std::strcpy(a.runtime.sha,"bbb"); check(pc::equal_protection(a,a));
    for(unsigned k=0;k<4;++k) { auto b=a;  // indexes avoid aliasing real files
        pc::tc::File* fields[]={&b.bridge,&b.backup,&b.receipt,&b.runtime};
        fields[k]->sha[0]='c'; check(!pc::equal_protection(a,b));
        b=a; fields[k]->st.st_ino++; check(!pc::equal_protection(a,b));
    }
    unsigned mock_calls=0;
    for(unsigned outcome=0;outcome<3;++outcome) {
        pc::spent=false; pc::save_attempts=0;
        try { if(pc::consume_save()) { check(pc::spent&&pc::save_attempts==1); ++mock_calls; if(outcome==2) throw std::runtime_error("unknown"); } } catch(...) {}
        check(!pc::consume_save()&&pc::save_attempts==1);
    }
    check(mock_calls==3);
    pc::ended=false; logs.clear(); pc::end(false,"test"); pc::end(true,"wrong_retry");
    check(logs.find("wrong_retry")==std::string::npos&&logs.find("immediate_stop_required=1")!=std::string::npos);
    // All failed base outcomes quarantine, including no click and queue throw.
    for(unsigned outcome=0;outcome<3;++outcome) {
        std::unique_ptr<fb::Job> held(new fb::Job(true)); pc::job=held.get();
        held->context.complete=outcome!=0; held->context.click_calls=outcome?1:0;
        pc::ended=false; queue_throws=outcome==2; unsigned prior=queued;
        bool pending=pc::transfer(held); check(!held); check(pending==(outcome==1));
        check(queued==prior+(outcome!=0)); check(pc::ended==(outcome!=1));
    }
    std::printf("placement checkpoint actual helper checks: %u passed; mock entries are not live proof\n",checks);
}
