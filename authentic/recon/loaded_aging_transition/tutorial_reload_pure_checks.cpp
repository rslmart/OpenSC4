// Helper-only executable. Native bindings are never entered; host stubs abort.
#include "tutorial_reload_syntax_stub.cpp"
#include <cassert>
#include <fstream>
#include <sys/mman.h>
static unsigned end_logs=0;
static void logf_(const char* fmt,...) { if(std::strstr(fmt,"TUTORIAL_RELOAD END ")) ++end_logs; }
static uint64_t get_app() { std::abort(); }
ScopedRZString::ScopedRZString() { std::abort(); }
ScopedRZString::~ScopedRZString() { std::abort(); }
int main() {
    using namespace tutorial_reload;
    Sample unterminated{};
    std::memset(unterminated.path,'P',sizeof(unterminated.path));
    std::memset(unterminated.pref,'F',sizeof(unterminated.pref));
    sample(unterminated,"UNTERMINATED_FAILURE"); // guards reject this non-game image
    assert(!unterminated.full);
    assert(unterminated.path[1023]==0 && unterminated.pref[1023]==0);
    assert(std::strlen(unterminated.path)==1023 && std::strlen(unterminated.pref)==1023);
    assert(stable(unterminated,unterminated)); // both strcmp inputs now bounded
    Sample a{},b{};
    a.full=a.paths_ok=a.fixture_ok=a.city_ok=true;
    a.core.app=1; a.core.city=2; a.core.sim=3;
    a.tutorial=4; a.regional=5; a.registry=6; a.core.pauses[0]=1;
    std::strcpy(a.path,cp::expected_path); std::strcpy(a.pref,"/fixed/prefs");
    b=a;
    assert(completed(a,b));
    // Registry absent is valid evidence; never interpreted as active false.
    assert(item_fields(0,0x2c12d0e9,b) && !b.item_known && !b.item);
    assert(completed(a,b));
    b.core.date=UINT32_MAX; b.core.pauses[0]=-9; b.core.pauses[1]=INT32_MIN; b.core.pauses[2]=INT32_MAX;
    assert(completed(a,b)); // drift is captured, not repaired or rejected
    b=a;
    a.core.pauses[0]=0; assert(!completed(a,b));
    a.core.pauses[0]=-1; assert(!completed(a,b)); a.core.pauses[0]=1;
    for(int i=0;i<2;++i) {
        Sample& s=i?b:a;
        s.ui=0xffff; assert(!completed(a,b)); s.ui=0;
        s.active=255; assert(!completed(a,b)); s.active=0;
        bool* fields[]={&s.full,&s.paths_ok,&s.fixture_ok,&s.city_ok};
        for(bool* f:fields) { *f=false; assert(!completed(a,b)); *f=true; }
    }
    uint64_t* ids[]={&b.core.app,&b.core.city,&b.core.sim,&b.tutorial,&b.regional,&b.registry};
    for(auto p:ids) { ++*p; assert(!completed(a,b)); --*p; }
    b.path[0]='?'; assert(!completed(a,b)); b=a;
    b.pref[0]='?'; assert(!completed(a,b)); b=a;
    for(unsigned mask=0;mask<16;++mask)
        assert(files(mask&1,mask&2,mask&4,mask&8)==(mask==15));
    alignas(8) unsigned char raw[0x18]{};
    const uint32_t guid=0x2c12d0e9;
    std::memcpy(raw,&guid,4); raw[0x10]=1;
    assert(item_fields(reinterpret_cast<uint64_t>(raw),guid,b) && b.item_known && b.item_active==1);
    assert(!item_fields(reinterpret_cast<uint64_t>(raw),guid+1,b));
    assert(!item_fields(UINT64_MAX,guid,b));
    assert(!object(reinterpret_cast<uint64_t>(raw),0x100a6b7c0,0x18)); // unsupported object type
    const size_t page=size_t(getpagesize());
    auto mem=static_cast<unsigned char*>(mmap(nullptr,2*page,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0));
    assert(mem!=MAP_FAILED); assert(!munmap(mem+page,page));
    // Item's GUID fits, but +10 byte is unmapped: reject complete 0x11 span.
    std::memcpy(mem+page-0x10,&guid,4);
    assert(!item_fields(reinterpret_cast<uint64_t>(mem+page-0x10),guid,b));
    assert(!munmap(mem,page));
    static constexpr char path[]="/Users/russellmartin/Personal/OpenSC4/authentic/recon/loaded_aging_transition/tutorial_reload_hash_check.tmp";
    { std::ofstream out(path); out << "abc"; }
    static constexpr char abc[]="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    assert(cp::canonical(path) && fb::file_hash(path,abc));
    assert(!fb::file_hash(path,fixture_sha));
    { std::ofstream out(path); out << "abd"; }
    assert(!fb::file_hash(path,abc)); assert(!unlink(path));
    assert(!fb::file_hash(path,abc));
    assert(!fb::file_hash("/Users/russellmartin/Personal/OpenSC4/authentic/recon/loaded_aging_transition",abc));
    assert(!ended.load()); end(true,"helper_test"); assert(ended.load()); end(false,"duplicate_suppressed"); assert(end_logs==1);
    std::puts("PASS helper-only: both samples required; initial pause; later drift; all identities/paths; hash/read failures; NULL/nonNULL UI; raw active; absent registry; item GUID; unsupported object; full 0x11 span; END once guard");
}
