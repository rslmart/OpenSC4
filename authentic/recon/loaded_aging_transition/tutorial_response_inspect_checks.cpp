
#include <cstdint>
#include <cstdio>
#include <cstdarg>
#include <cstring>
#include <cassert>
#include <map>
#include <string>
static std::map<uint64_t,unsigned char> memory;
static std::string logs;
static unsigned reads=0;
static unsigned long long U(uint64_t p) { return p; }
static void logf_(const char* f,...) {
    char b[2048]; va_list a; va_start(a,f); vsnprintf(b,sizeof(b),f,a); va_end(a);
    logs+=b; logs+='\n';
}
template<class T> static bool read(uint64_t p,T& out) {
    ++reads;
    for(unsigned i=0;i<sizeof(T);++i) if(!memory.count(p+i)) return false;
    auto* bytes=reinterpret_cast<unsigned char*>(&out);
    for(unsigned i=0;i<sizeof(T);++i) bytes[i]=memory[p+i];
    return true;
}
static void put(uint64_t p,uint64_t v,unsigned n=8) {
    for(unsigned i=0;i<n;++i) memory[p+i]=static_cast<unsigned char>(v>>(i*8));
}
namespace op { struct Capture { uint64_t w=0x8000,mgr=0x9000,dialog=0x10000; }; }
template<class T> struct RawField {
    T value{};
    const char* status="not_read";
    bool readable=false;
};
template<class T> static RawField<T> raw_field(const char* group,unsigned index,
                                             uint64_t base,uint64_t offset) {
    RawField<T> r;
    uint64_t address=0;
    if(!base) r.status="absent_base";
    else if(offset>UINT64_MAX-base || sizeof(T)-1>UINT64_MAX-(base+offset)) r.status="overflow";
    else {
        address=base+offset;
        r.readable=read(address,r.value);
        r.status=r.readable?"read":"unreadable";
    }
    // Value is meaningful ONLY when status=read; failed reads never mean zero.
    logf_("TUTORIAL_RESPONSE_INSPECT RAW group=%s index=%u base=%llx offset=%llx address=%llx status=%s readable=%u raw=%llx raw_only=1",
        group,index,U(base),U(offset),U(address),r.status,unsigned(r.readable),U(r.value));
    return r;
}
static bool raw_inventory(const op::Capture& c,uint64_t child) {
    logf_("TUTORIAL_RESPONSE_INSPECT INVENTORY child=%llx expected_W=%llx expected_M=%llx expected_D=%llx class_validation=0 unknown_native_calls=0",
        U(child),U(c.w),U(c.mgr),U(c.dialog));
    bool complete=true;
    auto u64=[&](const char* group,unsigned i,uint64_t base,uint64_t offset) {
        auto r=raw_field<uint64_t>(group,i,base,offset);
        if(base && !r.readable) complete=false;
        return r;
    };
    auto u32=[&](const char* group,unsigned i,uint64_t base,uint64_t offset) {
        auto r=raw_field<uint32_t>(group,i,base,offset);
        if(base && !r.readable) complete=false;
        return r;
    };
    u64("child_vptr",0,child,0);
    if(child>=8) u64("child_minus8_possible_vptr",0,child-8,0);
    else {
        logf_("TUTORIAL_RESPONSE_INSPECT RAW group=child_minus8_possible_vptr status=%s readable=0 raw_only=1",child?"subtraction_invalid":"absent_base");
        if(child) complete=false;
    }
    u32("child_ID",0,child,0x1c);
    u64("child_manager",0,child,8);
    u64("child_target",0,child,0x60);
    const auto parent=u64("child_parent",0,child,0x58);
    u32("child_flags",0,child,0x140);
    u32("child_command",0,child,0x150);
    u64("dialog_vptr",0,c.dialog,0);
    u64("dialog_manager",0,c.dialog,8);
    u64("dialog_parent",0,c.dialog,0x58);
    u64("dialog_proc",0,c.dialog,0x198);
    uint64_t seen[32]{};
    uint64_t node=parent.value;
    const char* termination=child?"parent_unreadable":"null_child";
    if(parent.readable) {
        for(unsigned i=0;;++i) {
            if(!node) { termination="null_parent"; break; }
            bool cycle=false;
            for(unsigned k=0;k<i;++k) if(seen[k]==node) cycle=true;
            if(cycle) { complete=false; termination="cycle"; break; }
            if(i==32) { complete=false; termination="bound32"; break; }
            seen[i]=node;
            const auto vt=u64("parent_vptr",i,node,0);
            const auto mgr=u64("parent_manager",i,node,8);
            const auto next=u64("parent_next",i,node,0x58);
            if(!vt.readable || !mgr.readable || !next.readable) {
                complete=false; termination="node_unreadable"; break;
            }
            // Numerical equality only: no inferred class or object identity.
            if(node==c.dialog) { termination="raw_address_equals_D"; break; }
            node=next.value;
        }
    }
    logf_("TUTORIAL_RESPONSE_INSPECT INVENTORY_RESULT inventoryComplete=%u termination=%s binding=unsupported_or_unvalidated raw_only=1 unknown_native_calls=0",
        unsigned(complete),termination);
    return complete;
}

static op::Capture c;
static constexpr uint64_t child=0x20000;
static void reset() {
    memory.clear(); logs.clear(); reads=0;
    put(c.dialog,0xabcdef); put(c.dialog+8,c.mgr);
    put(c.dialog+0x58,0); put(c.dialog+0x198,0);
}
static void fixture(uint64_t parent=0) {
    reset();
    put(child,0x12345678); // deliberately unknown raw vptr
    put(child-8,0x87654321); put(child+0x1c,0,4);
    put(child+8,0); put(child+0x60,0); put(child+0x58,parent);
    put(child+0x140,0,4); put(child+0x150,0,4);
}
int main() {
    reset(); assert(raw_inventory(c,0));
    assert(logs.find("termination=null_child")!=std::string::npos);
    fixture(); assert(raw_inventory(c,child)); // unknown/null binding still observable
    assert(logs.find("termination=null_parent")!=std::string::npos);
    auto z=raw_field<uint32_t>("zero",0,child,0x1c);
    assert(z.readable && z.value==0 && std::string(z.status)=="read");
    memory.erase(child+0x1c);
    auto fail=raw_field<uint32_t>("failed",0,child,0x1c);
    assert(!fail.readable && std::string(fail.status)=="unreadable");
    assert(!raw_inventory(c,child));
    unsigned before=reads;
    auto ov=raw_field<uint64_t>("add_overflow",0,UINT64_MAX-2,8);
    auto span=raw_field<uint64_t>("span_overflow",0,UINT64_MAX-2,0);
    auto absent=raw_field<uint64_t>("absent",0,0,0);
    assert(!ov.readable && !span.readable && !absent.readable && reads==before);
    assert(std::string(ov.status)=="overflow" && std::string(span.status)=="overflow");
    reset(); assert(!raw_inventory(c,1));
    assert(logs.find("status=subtraction_invalid")!=std::string::npos);
    fixture(child); assert(!raw_inventory(c,child));
    assert(logs.find("termination=cycle")!=std::string::npos);
    fixture(0x30000);
    for(unsigned i=0;i<33;++i) {
        uint64_t n=0x30000+i*0x1000;
        put(n,0x5555); put(n+8,0); put(n+0x58,n+0x1000);
    }
    assert(!raw_inventory(c,child));
    assert(logs.find("termination=bound32")!=std::string::npos);
    logs.clear(); put(0x30000+31*0x1000+0x58,0);
    assert(raw_inventory(c,child)); // precisely 32 observed nodes ending null
    fixture(c.dialog); assert(raw_inventory(c,child));
    assert(logs.find("termination=raw_address_equals_D")!=std::string::npos);
    fixture(c.dialog); memory.erase(c.dialog+0x58);
    assert(!raw_inventory(c,child));
    assert(logs.find("termination=node_unreadable")!=std::string::npos);
    std::puts("PASS isolated raw helpers: null/unknown/zero/read failure/overflow/subtraction/cycle/bound/dialog; structural inspection side-effect escape");
}
