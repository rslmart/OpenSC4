#!/usr/bin/env python3
"""Exercise the actual control() body with a bounded byte-map reader, no game."""
from pathlib import Path
import re
import subprocess
import tempfile

packet = Path(__file__).resolve().parent
finish = (packet / 'tutorial_finish_fragment.inc').read_text()
opened = (packet / 'tutorial_open_fragment.inc').read_text()

def array(text, name):
    match = re.search(r'static constexpr uint64_t ' + name + r'\[\] = \{.*?\n\};', text, re.S)
    assert match, name
    return match[0]

start = finish.index('static bool control(')
stop = finish.index('// clean-room fallback (not authentic): raw diagnostic inventory', start)
body = finish[start:stop]
source = r'''
#include <cstdint>
#include <cstring>
#include <map>
#include <vector>
#include <cstdio>
static std::map<uint64_t, unsigned char> mem;
static uint64_t relocated(uint64_t a) { return a; }
static unsigned long long U(uint64_t a) { return a; }
static void logf_(const char*, ...) {}
static bool read(uint64_t p, void* dst, size_t n) {
    auto out=static_cast<unsigned char*>(dst);
    for(size_t i=0;i<n;++i) { auto it=mem.find(p+i); if(it==mem.end()) return false; out[i]=it->second; }
    return true;
}
template<class T> static bool read(uint64_t p,T& out) { return read(p,&out,sizeof out); }
template<class T> static void put(uint64_t p,T value) {
    const auto bytes=reinterpret_cast<const unsigned char*>(&value);
    for(size_t i=0;i<sizeof value;++i) mem[p+i]=bytes[i];
}
static void alloc(uint64_t p,size_t n,uint64_t vt) { for(size_t i=0;i<n;++i) mem[p+i]=0; put(p,vt); }
static bool object(uint64_t p,uint64_t vt,size_t n) {
    uint64_t actual=0; std::vector<unsigned char> bytes(n);
    return read(p,actual) && actual==vt && read(p,bytes.data(),n);
}
namespace op {
struct Capture { uint64_t dialog=0,mgr=0,w=0; };
template<size_t N> static bool table(uint64_t p,uint64_t vt,const uint64_t(&expected)[N]) {
    uint64_t actual=0; if(!read(p,actual) || actual!=vt) return false;
    for(size_t i=0;i<N;++i) if(!read(vt+8*i,actual) || actual!=expected[i]) return false;
    return true;
}
'''
source += array(opened, 'window_table') + '\n}\n'
source += '\n'.join(array(finish, n) for n in ['button_table','button_primary','flatrect_table'])
source += r'''
struct Snapshot { std::vector<uint64_t> values; bool add(uint64_t v) { values.push_back(v); return values.size()<=128; } };
struct Control { uint64_t child=0,parent=0,dialog_parent=0,dialog_proc=0; uint32_t command=0,flags=0; Snapshot ancestry; };
'''
source += body
# The old implementation differs only by absence of the explicit FlatRect arm.
old_arm = re.compile(r'            // Exact inherited cGZWin layout:.*?op::table\(node,0x100a903d0,flatrect_table\) :\n', re.S)
old_body, removed = old_arm.subn('', body)
assert removed == 1
source += old_body.replace('static bool control(', 'static bool control_before_flatrect(', 1)
source += r'''
static int checks=0,failed=0;
static void check(bool value,const char* name) { ++checks; if(!value) { ++failed; std::printf("FAIL %s\n",name); } }
template<size_t N> static void install(uint64_t vt,const uint64_t(&data)[N]) { for(size_t i=0;i<N;++i) put(vt+8*i,data[i]); }
static op::Capture base() {
    mem.clear();
    install(0x100a899c0,op::window_table); install(0x100a8f660,button_table);
    install(0x100a8f410,button_primary); install(0x100a903d0,flatrect_table);
    alloc(0x20000,0x180,0x100a8f410); put<uint64_t>(0x20008,0x100a8f660);
    alloc(0x30000,0x60,0x100a903d0); alloc(0x40000,0x1a0,0x100a899c0);
    op::Capture c{0x40000,0x60000,0x50000};
    put<uint32_t>(0x20008+0x1c,0x6aaeec42); put<uint64_t>(0x20008+8,c.mgr);
    put<uint64_t>(0x20008+0x60,c.w); put<uint64_t>(0x20008+0x58,0x30000);
    put<uint64_t>(0x30000+8,c.mgr); put<uint64_t>(0x30000+0x58,c.dialog); put<uint64_t>(c.dialog+8,c.mgr);
    return c;
}
static bool accepts(const op::Capture& c) { Control out; return control(c,0x20008,out); }
int main() {
    auto c=base(); Control old;
    check(accepts(c),"observed FlatRect chain accepted");
    check(!control_before_flatrect(c,0x20008,old),"observed chain discriminates previous guard");
    for(size_t i=0;i<148;++i) { c=base(); put(0x100a903d0+8*i,flatrect_table[i]^uint64_t{1}); check(!accepts(c),"each altered FlatRect slot rejects"); }
    c=base(); put<uint64_t>(0x30000,0x100a903d8); check(!accepts(c),"unknown parent class rejects");
    c=base(); put<uint64_t>(0x30008,0x60008); check(!accepts(c),"wrong parent manager rejects");
    c=base(); mem.erase(0x3005f); check(!accepts(c),"truncated parent extent rejects");
    c=base(); put<uint64_t>(0x30058,0); check(!accepts(c),"null before dialog rejects");
    c=base(); put<uint64_t>(0x30058,0x30000); check(!accepts(c),"cycle rejects by bound");
    c=base(); put<uint64_t>(0x20008+0x60,0); check(!accepts(c),"wrong target rejects");
    c=base(); put<uint32_t>(0x20008+0x1c,0x6aaeec41); check(!accepts(c),"wrong command ID rejects");
    c=base(); put(0x100a8f660,button_table[0]^uint64_t{1}); check(!accepts(c),"button table remains checked");
    c=base(); put(0x100a899c0,op::window_table[0]^uint64_t{1}); check(!accepts(c),"dialog table remains checked");
    c=base(); alloc(0x30000,0x1a0,0x100a899c0); put<uint64_t>(0x30008,c.mgr); put<uint64_t>(0x30058,c.dialog); check(accepts(c),"Gen parent remains accepted");
    c=base(); put<uint64_t>(0x20008+0x58,c.dialog); check(accepts(c),"direct dialog parent remains accepted");
    for(unsigned count: {31u,32u}) {
        c=base();
        for(unsigned i=0;i<count;++i) { uint64_t n=0x80000+0x100*i; alloc(n,0x60,0x100a903d0); put<uint64_t>(n+8,c.mgr); put<uint64_t>(n+0x58,i+1==count?c.dialog:n+0x100); }
        put<uint64_t>(0x20008+0x58,0x80000);
        check(accepts(c)==(count==31),"32 total ancestors accepted, 33 rejected");
    }
    std::printf("checks=%d failed=%d; actual control body, byte-map adapter only\n",checks,failed);
    return failed?1:0;
}
'''
out = packet / 'tutorial_flatrect_guard_checks.cpp'
out.write_text(source)
with tempfile.TemporaryDirectory(prefix='opensc4-flatrect-') as temp:
    exe = str(Path(temp) / 'checks')
    subprocess.run(['xcrun','clang++','-std=c++17','-arch','x86_64','-isysroot',
                    '/Library/Developer/CommandLineTools/SDKs/MacOSX26.5.sdk',
                    '-ffp-model=strict','-ffp-contract=off','-O2',str(out),'-o',exe],check=True)
    subprocess.run([exe],check=True)
