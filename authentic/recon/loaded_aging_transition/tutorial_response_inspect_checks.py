#!/usr/bin/env python3
"""Isolated diagnostic-helper checks; never loads or calls game code."""
from pathlib import Path
import subprocess

root = Path(__file__).resolve().parent
source = (root / 'tutorial_finish_fragment.inc').read_text()
raw = source[source.index('template<class T> struct RawField'):source.index('static bool equal_control')]
# Check placement of the unconditional inspection escape before side effects.
lookup = source.index('const uint64_t child=original<')
branch = source.index('if(j.inspect_only)', lookup)
retain = source.index('j.leases[4]=', lookup)
assert lookup < source.index('raw_inventory(c,child)', lookup) < branch < retain
assert 'return false;' in source[branch:retain]
end_branch = source.index('if(j.inspect_only) { end(j,false,reason); return; }')
assert end_branch < source.index('if(j.pending)', end_branch)
assert 'const bool inspect_only;' in source
assert 'new Job(InspectOnly)' in source
assert 'finishtutorial() { return queue_tutorial_response<false>(); }' in source
assert 'inspecttutorialresponse() { return queue_tutorial_response<true>(); }' in source
assert source.count('attempted.compare_exchange_strong(expected,true)') == 1
assert 'original<' not in raw and 'dispatch_' not in raw and 'retain(' not in raw

prefix = r'''
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
'''
suffix = r'''
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
'''
cpp = root / 'tutorial_response_inspect_checks.cpp'
exe = root / 'tutorial_response_inspect_checks.bin'
cpp.write_text(prefix + raw + suffix)
command = ['xcrun','clang++','-std=c++17','-arch','x86_64','-isysroot',
           '/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk',
           '-ffp-model=strict','-ffp-contract=off','-Wall','-Wextra','-Werror',
           str(cpp),'-o',str(exe)]
subprocess.run(command, check=True)
subprocess.run([str(exe)], check=True)
exe.unlink()
