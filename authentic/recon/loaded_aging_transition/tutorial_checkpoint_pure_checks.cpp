// Scoped helper checks only; never loads or calls the game image.
#include "tutorial_checkpoint_syntax_stub.cpp"
#include <cassert>
#include <string>
#include <fstream>
#include <cstdarg>
static void logf_(const char*,...) {}
static uint64_t get_app() { std::abort(); }
ScopedRZString::ScopedRZString() { std::abort(); }
ScopedRZString::~ScopedRZString() { std::abort(); }
int main() {
    using namespace tutorial_checkpoint;
    assert(exact_path(expected_path,expected_path,expected_path));
    assert(!exact_path("/tmp/other",expected_path,expected_path));
    assert(!exact_path(expected_path,"/tmp/other",expected_path));
    assert(!exact_path(expected_path,expected_path,"/tmp/other"));
    assert(attempt_available(true,false));
    assert(!attempt_available(true,true)); assert(!attempt_available(false,false));
    ListContext empty; uint64_t values[128]{}; unsigned count=999;
    assert(list_values(empty.list,values,count) && count==0);
    empty.list.count=129; assert(!list_values(empty.list,values,count));
    empty.list.count=1; assert(!list_values(empty.list,values,count));
    const auto self=reinterpret_cast<uint64_t>(&empty.list);
    alignas(8) uint64_t node[3]={self,self,0x1234};
    empty.list={reinterpret_cast<uint64_t>(node),reinterpret_cast<uint64_t>(node),1};
    assert(list_values(empty.list,values,count) && count==1 && values[0]==0x1234);
    node[1]=reinterpret_cast<uint64_t>(node); assert(!list_values(empty.list,values,count)); // cycle/reciprocity
    node[1]=self; node[0]=0; assert(!list_values(empty.list,values,count));
    const std::string dir="/Users/russellmartin/Personal/OpenSC4/authentic/recon/loaded_aging_transition/checkpoint_pure_artifacts";
    assert(!mkdir(dir.c_str(),0700) || errno==EEXIST);
    const auto emptyfile=dir+"/empty",missing=dir+"/absent",linkfile=dir+"/link",hard=dir+"/hard";
    unlink(emptyfile.c_str()); unlink(missing.c_str()); unlink(linkfile.c_str()); unlink(hard.c_str());
    { std::ofstream f(emptyfile); }
    File a{},b{}; assert(snapshot(emptyfile.c_str(),a,false) && a.present && a.st.st_size==0);
    assert(!std::strcmp(a.sha,"e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855"));
    assert(snapshot(missing.c_str(),b,true) && !b.present);
    assert(!snapshot(missing.c_str(),b,false)); // absence is not a readable original
    assert(!snapshot(dir.c_str(),b,true)); // error/nonregular is not absence
    assert(!same_file(a,b));
    b=a; b.sha[0]='0'; assert(!same_file(a,b));
    assert(!symlink(emptyfile.c_str(),linkfile.c_str()));
    assert(!canonical(linkfile.c_str()) && !snapshot(linkfile.c_str(),b,false));
    assert(!link(emptyfile.c_str(),hard.c_str()));
    assert(!snapshot(hard.c_str(),b,false) && !snapshot(emptyfile.c_str(),b,false));
    assert(!prepared_file(emptyfile.c_str())); // neighboring file outside dedicated region
    assert(!prepared_file(linkfile.c_str())); assert(!prepared_file(hard.c_str()));
    ReadyReceipt r{}; r.pid=-1; assert(!backup(r)); // trusted fixed path missing
    unlink(linkfile.c_str()); unlink(hard.c_str()); unlink(emptyfile.c_str());
    std::puts("PASS: exact paths; absent/error/empty/hash; outside/symlink/hardlink; backup missing; spent/missing receipt; list empty/count/cycle/reciprocity");
}
