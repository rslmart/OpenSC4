
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
static constexpr uint64_t window_table[] = {
    0x10024ddc0,0x10024de10,0x10024de2c,0x10024f01c,0x10024de48,0x10024e74a,
    0x10016d194,0x10016d19e,0x10016d1ae,0x10016d1b8,0x10016ccfc,0x100169784,
    0x10016978e,0x10016a044,0x100169806,0x10016990c,0x1001699c6,0x100169ae6,
    0x1001699dc,0x10016a062,0x10016a0d0,0x10016a444,0x10016a476,0x100169f0a,
    0x100169f2e,0x100169b64,0x100169d2a,0x100169d54,0x100169df2,0x100169ea0,
    0x100169f52,0x100169faa,0x10016c582,0x10016c6a6,0x10016a138,0x10016a19c,
    0x10016a38e,0x10016a3c4,0x10016cd0c,0x10016a222,0x10016a2ce,0x10016c7e2,
    0x10016c7f4,0x10016c806,0x10016c812,0x10016c81e,0x10016c82a,0x10016c966,
    0x10016c97e,0x10016c974,0x10016c99a,0x10016c8e6,0x10016c910,0x10016c93a,
    0x10024e902,0x10016c8c6,0x10016c9c8,0x10016c9f8,0x10016a4e8,0x10016ca3e,
    0x10016ca2e,0x10016a3fa,0x10024ec42,0x10016d0b4,0x10016d0aa,0x10016d0cc,
    0x10016d0be,0x10016cbde,0x100168f7e,0x10016cbee,0x10016cc52,0x10016ccb2,
    0x10016ccc8,0x10016d092,0x10024ec0c,0x10016a8a6,0x10016a8b4,0x10016a8f6,
    0x10016d0d8,0x10016d0e4,0x10016d0f0,0x10016d112,0x10016bdd8,0x10016a9a0,
    0x10016a914,0x1002505c0,0x10016cd22,0x10016cd2c,0x10024ed30,0x10016b86c,
    0x100169556,0x10016d120,0x10016d12e,0x10016d16a,0x10016d178,0x10016b8b2,
    0x10016ba20,0x10016ba70,0x10016bd86,0x1002748a0,0x10016be44,0x10016d186,
    0x10016d2b2,0x10016d29c,0x10016d24a,0x10016d1c8,0x10016d280,0x10016d230,
    0x10016c382,0x10016d3d4,0x10016d3e6,0x10016d3ba,0x10016d3c6,0x10016ce30,
    0x10016cd36,0x10016ce82,0x10016cf14,0x10016cfde,0x10016c9b0,0x100169672,
    0x1001696dc,0x10024ec8a,0x10016cb10,0x10016abd0,0x10016b49a,0x10016d23e,
    0x10016d290,0x10016c3fe,0x10016c482,0x10016a82a,0x10024f14e,0x10024f260,
    0x10016a84a,0x10016a896,0x10024f2be,0x10016d2e8,0x10024f47c,0x10016d2f0,
    0x10024f4a0,0x10016d300,0x10016a89e,0x10016d308,0x10016d310,0x10024f6c8,
    0x10016d318,0x10016d350,0x10016d37c,0x10016d38e,
};
}
static constexpr uint64_t button_table[] = {
    0x10027fe28,0x10027fe5c,0x10027fe74,0x100281d98,0x10027ffb0,0x100280096,
    0x10016d194,0x10016d19e,0x10016d1ae,0x10016d1b8,0x10016ccfc,0x100169784,
    0x10016978e,0x10016a044,0x100169806,0x10016990c,0x1001699c6,0x100169ae6,
    0x1001699dc,0x10016a062,0x10016a0d0,0x10016a444,0x10016a476,0x100169f0a,
    0x100169f2e,0x100169b64,0x100169d2a,0x100169d54,0x100169df2,0x100169ea0,
    0x100169f52,0x100169faa,0x10016c582,0x10016c6a6,0x10016a138,0x10016a19c,
    0x10016a38e,0x10016a3c4,0x10016cd0c,0x10016a222,0x10016a2ce,0x10016c7e2,
    0x10016c7f4,0x10016c806,0x10016c812,0x10016c81e,0x10016c82a,0x10016c966,
    0x10016c97e,0x10016c974,0x10016c99a,0x10016c8e6,0x10016c910,0x10016c93a,
    0x100280d4a,0x10016c8c6,0x10016c9c8,0x10016c9f8,0x10016a4e8,0x10016ca3e,
    0x10016ca2e,0x10016a3fa,0x10016cb3c,0x1002800ce,0x1002800b6,0x10016d0cc,
    0x10016d0be,0x10016cbde,0x10028066c,0x10016cbee,0x10016cc52,0x10016ccb2,
    0x10016ccc8,0x1002810ea,0x1002810cc,0x10016a8a6,0x10016a8b4,0x10016a8f6,
    0x10016d0d8,0x10028010e,0x10016d0f0,0x10016d112,0x10016bdd8,0x10016a9a0,
    0x10016a914,0x10016a984,0x10016cd22,0x10016cd2c,0x1002814cc,0x10016b86c,
    0x100169556,0x10016d120,0x10016d12e,0x10016d16a,0x10016d178,0x10016b8b2,
    0x10016ba20,0x10016ba70,0x10016bd86,0x1002748a0,0x10016be44,0x10016d186,
    0x10016d2b2,0x10016d29c,0x10016d24a,0x10016d1c8,0x10016d280,0x10016d230,
    0x10016c382,0x10016d3d4,0x10016d3e6,0x10016d3ba,0x10016d3c6,0x10016ce30,
    0x10016cd36,0x10016ce82,0x10016cf14,0x10016cfde,0x10016c9b0,0x100169672,
    0x1001696dc,0x10016ca8c,0x10016cb10,0x10016abd0,0x10016b49a,0x10016d23e,
    0x10016d290,0x10016c3fe,0x10016c482,0x10016a82a,0x100281566,0x1002815e2,
    0x10016a84a,0x10016a896,0x100281736,0x1002817f2,0x1002818dc,0x1002819de,
    0x1002819f8,0x10016d300,0x100281c30,0x100281b88,0x100281bc0,0x10016a842,
    0x10016d318,0x10016d350,0x10016d37c,0x10016d38e,
};
static constexpr uint64_t button_primary[] = {
    0x10027fe06,0x10027fe4e,0x10027fe66,
};
static constexpr uint64_t flatrect_table[] = {
    0x100294404,0x100294454,0x100294470,0x10029448c,0x100168dbe,0x100168e56,
    0x10016d194,0x10016d19e,0x10016d1ae,0x10016d1b8,0x10016ccfc,0x100169784,
    0x10016978e,0x10016a044,0x100169806,0x10016990c,0x1001699c6,0x100169ae6,
    0x1001699dc,0x10016a062,0x10016a0d0,0x10016a444,0x10016a476,0x100169f0a,
    0x100169f2e,0x100169b64,0x100169d2a,0x100169d54,0x100169df2,0x100169ea0,
    0x100169f52,0x100169faa,0x10016c582,0x10016c6a6,0x10016a138,0x10016a19c,
    0x10016a38e,0x10016a3c4,0x10016cd0c,0x10016a222,0x10016a2ce,0x10016c7e2,
    0x10016c7f4,0x10016c806,0x10016c812,0x10016c81e,0x10016c82a,0x10016c966,
    0x10016c97e,0x10016c974,0x10016c99a,0x10016c8e6,0x10016c910,0x10016c93a,
    0x10016c836,0x10016c8c6,0x10016c9c8,0x10016c9f8,0x10016a4e8,0x10016ca3e,
    0x10016ca2e,0x10016a3fa,0x10016cb3c,0x10016d0b4,0x10016d0aa,0x10016d0cc,
    0x10016d0be,0x10016cbde,0x100168f7e,0x10016cbee,0x10016cc52,0x10016ccb2,
    0x10016ccc8,0x10016d092,0x10016d09c,0x10016a8a6,0x10016a8b4,0x10016a8f6,
    0x10016d0d8,0x10016d0e4,0x10016d0f0,0x10016d112,0x10016bdd8,0x10016a9a0,
    0x10016a914,0x10016a984,0x10016cd22,0x10016cd2c,0x10029450a,0x10016b86c,
    0x100169556,0x10016d120,0x10016d12e,0x10016d16a,0x10016d178,0x10016b8b2,
    0x10016ba20,0x10016ba70,0x10016bd86,0x1002748a0,0x10016be44,0x10016d186,
    0x100294926,0x1002948ee,0x10029490a,0x1002948d2,0x10016d280,0x10016d230,
    0x10016c382,0x10016d3d4,0x10016d3e6,0x10016d3ba,0x10016d3c6,0x10016ce30,
    0x10016cd36,0x10016ce82,0x10016cf14,0x10016cfde,0x10016c9b0,0x100169672,
    0x1001696dc,0x10016ca8c,0x10016cb10,0x10016abd0,0x10016b49a,0x10016d23e,
    0x10016d290,0x10016c3fe,0x10016c482,0x10016a82a,0x10029467e,0x1002946b4,
    0x10016a84a,0x10016a896,0x10016d2d8,0x10016d2e8,0x10016d2e0,0x10016d2f0,
    0x10016d2f8,0x10016d300,0x10016a89e,0x10016d308,0x10016d310,0x10016a842,
    0x10016d318,0x10016d350,0x10016d37c,0x10016d38e,
};
struct Snapshot { std::vector<uint64_t> values; bool add(uint64_t v) { values.push_back(v); return values.size()<=128; } };
struct Control { uint64_t child=0,parent=0,dialog_parent=0,dialog_proc=0; uint32_t command=0,flags=0; Snapshot ancestry; };
static bool control(const op::Capture& c,uint64_t child,Control& out) {
    if(!child || child<8 || !object(child,0x100a8f660,0x154) ||
       !op::table(child,0x100a8f660,button_table) ||
       !op::table(child-8,0x100a8f410,button_primary) ||
       !object(c.dialog,0x100a899c0,0x1a0) || !op::table(c.dialog,0x100a899c0,op::window_table)) return false;
    uint32_t id=0; uint64_t manager=0,target=0,dm=0;
    out.child=child;
    if(!read(child+0x1c,id) || id!=0x6aaeec42 || !read(child+8,manager) || manager!=c.mgr ||
       !read(child+0x60,target) || target!=c.w || !read(c.dialog+8,dm) || dm!=c.mgr ||
       !read(child+0x58,out.parent) || !read(child+0x150,out.command) ||
       !read(child+0x140,out.flags) || !read(c.dialog+0x58,out.dialog_parent) ||
       !read(c.dialog+0x198,out.dialog_proc)) return false;
    uint64_t node=out.parent;
    for(unsigned i=0;i<32;++i) {
        uint64_t vt=0,m=0,next=0;
        if(!node || !read(node,vt)) return false;
        const bool known=vt==relocated(0x100a899c0) ?
            object(node,0x100a899c0,0x1a0) && op::table(node,0x100a899c0,op::window_table) :
            // Exact inherited cGZWin layout: FlatRect ctor @0x1002942de;
            // manager @0x10016d194, parent @0x100169784. Extent is adapter policy.
            vt==relocated(0x100a903d0) ?
            object(node,0x100a903d0,0x60) && op::table(node,0x100a903d0,flatrect_table) :
            vt==relocated(0x100a8f660) && node>=8 && object(node,0x100a8f660,0x154) &&
            op::table(node,0x100a8f660,button_table) && op::table(node-8,0x100a8f410,button_primary);
        if(!known || !read(node+8,m) || m!=c.mgr || !read(node+0x58,next)) return false;
        logf_("TUTORIAL_FINISH PARENT index=%u node=%llx vptr=%llx manager=%llx parent=%llx root_dialog=%u",i,U(node),U(vt),U(m),U(next),unsigned(node==c.dialog));
        if(!out.ancestry.add(node) || !out.ancestry.add(vt) || !out.ancestry.add(next)) return false;
        if(node==c.dialog) return true;
        node=next;
    }
    return false;
}
static bool control_before_flatrect(const op::Capture& c,uint64_t child,Control& out) {
    if(!child || child<8 || !object(child,0x100a8f660,0x154) ||
       !op::table(child,0x100a8f660,button_table) ||
       !op::table(child-8,0x100a8f410,button_primary) ||
       !object(c.dialog,0x100a899c0,0x1a0) || !op::table(c.dialog,0x100a899c0,op::window_table)) return false;
    uint32_t id=0; uint64_t manager=0,target=0,dm=0;
    out.child=child;
    if(!read(child+0x1c,id) || id!=0x6aaeec42 || !read(child+8,manager) || manager!=c.mgr ||
       !read(child+0x60,target) || target!=c.w || !read(c.dialog+8,dm) || dm!=c.mgr ||
       !read(child+0x58,out.parent) || !read(child+0x150,out.command) ||
       !read(child+0x140,out.flags) || !read(c.dialog+0x58,out.dialog_parent) ||
       !read(c.dialog+0x198,out.dialog_proc)) return false;
    uint64_t node=out.parent;
    for(unsigned i=0;i<32;++i) {
        uint64_t vt=0,m=0,next=0;
        if(!node || !read(node,vt)) return false;
        const bool known=vt==relocated(0x100a899c0) ?
            object(node,0x100a899c0,0x1a0) && op::table(node,0x100a899c0,op::window_table) :
            vt==relocated(0x100a8f660) && node>=8 && object(node,0x100a8f660,0x154) &&
            op::table(node,0x100a8f660,button_table) && op::table(node-8,0x100a8f410,button_primary);
        if(!known || !read(node+8,m) || m!=c.mgr || !read(node+0x58,next)) return false;
        logf_("TUTORIAL_FINISH PARENT index=%u node=%llx vptr=%llx manager=%llx parent=%llx root_dialog=%u",i,U(node),U(vt),U(m),U(next),unsigned(node==c.dialog));
        if(!out.ancestry.add(node) || !out.ancestry.add(vt) || !out.ancestry.add(next)) return false;
        if(node==c.dialog) return true;
        node=next;
    }
    return false;
}

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
