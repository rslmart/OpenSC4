
#include "tutorial_reload_syntax_stub.cpp"
#include <cassert>
#include <string>
#include <sys/mman.h>
#include <unistd.h>
static void logf_(const char*,...) {}
static uint64_t get_app() { std::abort(); }
ScopedRZString::ScopedRZString() { std::abort(); }
ScopedRZString::~ScopedRZString() { std::abort(); }
namespace fb=fixture_placement_bindings;
namespace profile_test {
using namespace fb;
static bool original_manifest=true,saved_manifest_ok=true,city_ok=true,regular=true;
static const char* disk_sha=saved_sha;
static bool file_hash(const char* path,const char* sha) {
    if(!std::strcmp(path,manifest)) return original_manifest && !std::strcmp(sha,manifest_hash);
    if(!std::strcmp(path,saved_manifest)) return saved_manifest_ok && !std::strcmp(sha,saved_manifest_hash);
    return city_ok && !std::strcmp(sha,disk_sha);
}
static bool saved_file(const char*) { return regular; }
static FixtureKind fixture_profile(const char* path,uint64_t city) {
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
    return FixtureKind::Unbound;
}
}
namespace identity_test {
using namespace fb;
static bool captured=true;
static FixtureKind selected=FixtureKind::SavedInactive;
static uint64_t reg=22;
static const char* selected_path=saved_path;
static bool capture_stub(const State&,char (&path)[1024],uint64_t& regional,FixtureKind* kind) {
    if(!captured) return false;
    std::strcpy(path,selected_path); regional=reg; *kind=selected; return true;
}
static bool test_identity(void* opaque,const State& s) {
    Owner& o=*static_cast<Owner*>(opaque); char path[1024]={}; uint64_t regional=0;
    FixtureKind kind=FixtureKind::Unbound;
    if(!capture_stub(s,path,regional,&kind) || kind==FixtureKind::Unbound || (o.inspected &&
       (o.city!=s.city || o.regional!=regional || std::strcmp(o.path,path)))) return false;
    if(o.kind!=FixtureKind::Unbound && o.kind!=kind) return false;
    o.kind=kind; // legacy inspected Owners bind only after all prior comparisons pass
    return true;
}
}
// Actual bounded string-copy logic with a trivial byte-container host stub;
// avoids constructing any native RZ string. Same read helper, no native calls.
namespace string_test {
using fixture_placement::read;
struct ScopedRZString { unsigned char buf[64]{}; };
static bool string_copy(const ScopedRZString& s, char (&out)[1024]) {
    // Host c_str ABI: small string tag+8, inline+9, heap+18. Read boundedly.
    uint64_t p=reinterpret_cast<uint64_t>(s.buf)+9;
    if(s.buf[8]&1) std::memcpy(&p,s.buf+0x18,8);
    for(unsigned i=0;i<sizeof out;++i) { if(!read(p+i,out[i])) return false; if(!out[i]) return i>0; }
    return false;
}
}
int main() {
    using namespace fb;
    alignas(8) unsigned char city[0x348]{}, tutorial[0x68]{};
    uint64_t tp=reinterpret_cast<uint64_t>(tutorial), cp=reinterpret_cast<uint64_t>(city), vt=0x100a6b7c0;
    std::memcpy(city+0x340,&tp,8); std::memcpy(tutorial,&vt,8);
    assert(saved_inactive(cp));
    for(unsigned i=1;i<256;++i) { tutorial[0x2f]=i; assert(!saved_inactive(cp)); }
    tutorial[0x2f]=0; tutorial[0x60]=1; assert(!saved_inactive(cp)); tutorial[0x60]=0;
    tutorial[0]^=1; assert(!saved_inactive(cp)); tutorial[0]^=1;
    assert(!saved_inactive(0) && !saved_inactive(UINT64_MAX-0x340));
    const size_t page=getpagesize();
    auto mem=static_cast<unsigned char*>(mmap(nullptr,page*2,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANON,-1,0));
    assert(mem!=MAP_FAILED && !munmap(mem+page,page));
    uint64_t truncated=reinterpret_cast<uint64_t>(mem+page-0x60);
    std::memcpy(mem+page-0x60,&vt,8); std::memcpy(city+0x340,&truncated,8);
    assert(!saved_inactive(cp)); // correct class/active, but UI tail unreadable
    std::memcpy(city+0x340,&tp,8);
    using K=FixtureKind;
    { using namespace profile_test;
      assert(profile_test::fixture_profile(saved_path,cp)==K::SavedInactive);
      disk_sha="81ee61200118484b05f1e17ba00545e99c9db7780e147b4e049ce5d5fb2b40c1";
      tutorial[0x2f]=1; assert(profile_test::fixture_profile(saved_path,cp)==K::OriginalPrepared);
      original_manifest=false; assert(profile_test::fixture_profile(saved_path,cp)==K::Unbound);
      original_manifest=true; tutorial[0x2f]=0; disk_sha=saved_sha;
      for(bool* flag:{&saved_manifest_ok,&city_ok,&regular}) {
          *flag=false; assert(profile_test::fixture_profile(saved_path,cp)==K::Unbound); *flag=true;
      }
      for(const Prepared& original:prepared) {
          disk_sha=original.sha;
          std::string original_path=std::string(region)+"/"+original.name;
          assert(profile_test::fixture_profile(original_path.c_str(),0)==K::OriginalPrepared);
      }
      disk_sha="bad hash"; assert(profile_test::fixture_profile(saved_path,cp)==K::Unbound); disk_sha=saved_sha;
      for(const char* wrong:{"/outside/City - Rush Hour Tutorial.sc4",
          "/Users/russellmartin/Documents/SimCity 4/Regions/_opensc4_aging_20261114x/City - Rush Hour Tutorial.sc4",
          "/Users/russellmartin/Documents/SimCity 4/Regions/_opensc4_aging_20261114/Wrong.sc4",
          "/Users/russellmartin/Documents/SimCity 4/Regions/_opensc4_aging_20261114/sub/City - Rush Hour Tutorial.sc4"})
          assert(profile_test::fixture_profile(wrong,cp)==K::Unbound);
      tutorial[0x2f]=1; assert(profile_test::fixture_profile(saved_path,cp)==K::Unbound); tutorial[0x2f]=0;
      tutorial[0x60]=1; assert(profile_test::fixture_profile(saved_path,cp)==K::Unbound); tutorial[0x60]=0;
      tutorial[0]^=1; assert(profile_test::fixture_profile(saved_path,cp)==K::Unbound); tutorial[0]^=1;
      assert(profile_test::fixture_profile(saved_path,0)==K::Unbound);
    }
    { using namespace identity_test;
      State s{}; s.city=11; Owner o{}; o.inspected=true; o.city=11; o.regional=22; std::strcpy(o.path,saved_path);
      selected=K::Unbound; assert(!identity_test::test_identity(&o,s) && o.kind==K::Unbound); selected=K::SavedInactive;
      captured=false; assert(!identity_test::test_identity(&o,s) && o.kind==K::Unbound); captured=true;
      ++o.city; assert(!identity_test::test_identity(&o,s) && o.kind==K::Unbound); --o.city;
      ++reg; assert(!identity_test::test_identity(&o,s) && o.kind==K::Unbound); --reg;
      selected_path="/wrong"; assert(!identity_test::test_identity(&o,s) && o.kind==K::Unbound); selected_path=saved_path;
      assert(identity_test::test_identity(&o,s) && o.kind==K::SavedInactive); assert(identity_test::test_identity(&o,s));
      selected=K::OriginalPrepared; assert(!identity_test::test_identity(&o,s) && o.kind==K::SavedInactive);
      Owner fresh{}; assert(identity_test::test_identity(&fresh,s) && fresh.kind==K::OriginalPrepared);
      selected=K::SavedInactive; assert(!identity_test::test_identity(&fresh,s) && fresh.kind==K::OriginalPrepared);
      Owner copied=o; assert(copied.kind==K::SavedInactive && identity_test::test_identity(&copied,s));
    }
    // Files created only here, in scoped check artifacts; never read game assets.
    char dir[]= "/Users/russellmartin/Personal/OpenSC4/authentic/recon/loaded_aging_transition/saved_fixture_binding_tmpXXXXXX";
    assert(mkdtemp(dir)); std::string path=std::string(dir)+"/file", linkpath=std::string(dir)+"/link";
    FILE* f=std::fopen(path.c_str(),"wb"); assert(f); assert(std::fwrite("abc",1,3,f)==3); assert(!std::fclose(f));
    assert(fb::saved_file(path.c_str())); assert(!fb::saved_file(dir));
    static constexpr char abc[]="ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad";
    assert(fb::file_hash(path.c_str(),abc)); assert(!fb::file_hash(path.c_str(),saved_sha));
    assert(!fb::saved_file((std::string(dir)+"/./file").c_str()));
    assert(!symlink(path.c_str(),linkpath.c_str())); assert(!fb::saved_file(linkpath.c_str())); assert(!unlink(linkpath.c_str()));
    assert(!link(path.c_str(),linkpath.c_str())); assert(!fb::saved_file(path.c_str())); assert(!fb::saved_file(linkpath.c_str()));
    assert(!unlink(linkpath.c_str())); assert(fb::saved_file(path.c_str()));
    std::string alias=std::string(dir)+"/alias"; assert(!symlink(dir,alias.c_str()));
    assert(!fb::saved_file((alias+"/file").c_str())); assert(!unlink(alias.c_str()));
    assert(!fb::saved_file((std::string(dir)+"/../"+std::strrchr(dir,'/')+"/file").c_str()));
    assert(!unlink(path.c_str())); assert(!fb::saved_file(path.c_str())); assert(!rmdir(dir));
    string_test::ScopedRZString rz{}; char full[1024]; std::memset(full,'X',sizeof full);
    rz.buf[8]=1; uint64_t p=reinterpret_cast<uint64_t>(full); std::memcpy(rz.buf+0x18,&p,8);
    char result[1024]{}; assert(!string_test::string_copy(rz,result));
    full[1023]=0; assert(string_test::string_copy(rz,result));
    p=reinterpret_cast<uint64_t>(mem+page-1); mem[page-1]='X'; std::memcpy(rz.buf+0x18,&p,8);
    assert(!string_test::string_copy(rz,result)); assert(!munmap(mem,page));
    std::puts("PASS saved fixture helper checks: profiles, hash/path failures, raw active/UI/class/readability, canonical links, immutable/lazy Owner, bounded strings; native stubs abort");
}
