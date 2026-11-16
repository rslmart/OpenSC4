#include "opensc4/pollution_grid_kernel.h"
#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
using namespace opensc4::pollution_grid_kernel;
namespace {
unsigned checks = 0;
void check(bool ok, const char* label) {
    ++checks;
    if (!ok) { std::cerr << "FAIL " << label << '\n'; std::exit(1); }
}
struct Fixture;
struct Mock {
    Fixture* owner;
    int id;
    std::uint32_t raw = 0;
    std::int32_t shift = 0;
    bool drop = false;
    void (*afterShift)(Mock&) = nullptr;
    void (*afterGet)(Mock&) = nullptr;
    void (*afterSet)(Mock&) = nullptr;
};
struct Event {
    char op; int id; std::uint32_t x, z; std::int32_t value;
    std::uint32_t count; std::array<std::uint8_t,4> dirty;
};
struct Fixture {
    std::array<std::uint8_t,4> dirty{};
    std::uint32_t count = 0, mode = 14, active = 0;
    std::vector<Event> trace;
    Mock r{this,0}, a{this,1}, w{this,2}, g{this,3}, replacement{this,4};
    State state;
    static void log(Mock& m, char op, std::uint32_t x=0, std::uint32_t z=0, std::int32_t v=0) {
        auto& f=*m.owner;
        f.trace.push_back({op,m.id,x,z,v,f.count,f.dirty});
    }
    static std::uint32_t get(void* p, std::uint32_t x, std::uint32_t z) {
        auto& m=*static_cast<Mock*>(p); log(m,'G',x,z);
        const auto raw=m.raw;
        if(m.afterGet) m.afterGet(m);
        return raw;
    }
    static std::int32_t shift(void* p) {
        auto& m=*static_cast<Mock*>(p); log(m,'H');
        const auto n=m.shift;
        if(m.afterShift) m.afterShift(m);
        return n;
    }
    static void set(void* p, std::uint32_t x, std::uint32_t z, std::int32_t v) {
        auto& m=*static_cast<Mock*>(p); log(m,'S',x,z,v);
        // Caller-provided low16 store. Drop is a mock policy, NOT original bounds behavior.
        if(!m.drop) m.raw=static_cast<std::uint32_t>(v)&0xffffu;
        if(m.afterSet) m.afterSet(m);
    }
    static Grid grid(Mock& m) { return {&m,get,shift,set}; }
    static std::uint32_t getMode(void* p) {
        auto& f=*static_cast<Fixture*>(p); log(f.g,'M'); return f.mode;
    }
    static std::uint32_t query(void* p, std::uint32_t x, std::uint32_t z) {
        auto& f=*static_cast<Fixture*>(p); log(f.g,'Q',x,z); return f.active;
    }
    Fixture(): state{grid(r),grid(a),grid(w),grid(g),&dirty[0],&dirty[1],&dirty[2],&dirty[3],
                    &count,this,getMode,this,query} {}
    std::string ops() const { std::string s; for(auto& e:trace) s+=e.op; return s; }
};
void dirtyOnly(const Fixture& f, int id) {
    for(int i=0;i<4;++i) check(f.dirty[i]==(i==id ? 1:0),"dirty selection");
}
void scalarChecks() {
    struct Shift { std::int32_t n; std::uint32_t x,z; };
    const Shift shifts[]={{0,0xffffffffu,0x80000000u},{1,0x7fffffffu,0x40000000u},
        {31,1,1},{32,0xffffffffu,0x80000000u},{33,0x7fffffffu,0x40000000u},
        {-1,1,1},{-32,0xffffffffu,0x80000000u},{256,0xffffffffu,0x80000000u}};
    struct Value { std::int32_t in,out; };
    const Value values[]={{0,0},{32767,32767},{32768,-32768},{65535,-1},{65536,0},
        {-1,-1},{-32769,32767},{2147483647,-1},{(-2147483647-1),0}};
    for(int id=0;id<3;++id) for(auto sh:shifts) for(auto v:values) {
        Fixture f; Mock* m[]={&f.r,&f.a,&f.w}; m[id]->shift=sh.n;
        if(id==0) SetIsRadioactive(f.state,0xffffffffu,0x80000000u,v.in!=0);
        if(id==1) SetAirValue(f.state,0xffffffffu,0x80000000u,v.in);
        if(id==2) SetWaterValue(f.state,0xffffffffu,0x80000000u,v.in);
        check(f.ops()=="HS","scalar order"); const auto& e=f.trace.back();
        check(e.id==id && e.x==sh.x && e.z==sh.z,"scalar grid/unsigned masked shift");
        check(e.value==(id==0 ? (v.in!=0):v.out),"scalar raw ECX");
        check(e.dirty==std::array<std::uint8_t,4>{},"dirty follows set");
        check(m[id]->raw==(static_cast<std::uint32_t>(e.value)&0xffffu),"low16 store");
        dirtyOnly(f,id);
    }
}
void garbageChecks() {
    struct Case { std::uint32_t old; std::int32_t in,out; };
    const Case cases[]={{0x30,5,5},{0x8030,5,-32763},{0x30,0x8005,-32763},
        {0xffffffffu,0,-32768},{0x7fff,65536,0},{0,-1,-1},{0xffff0000u,5,5}};
    for(auto c:cases) {
        Fixture f; f.g.raw=c.old; f.g.shift=33;
        SetGarbageValue(f.state,0xffffffffu,0x80000000u,c.in);
        check(f.ops()=="HGS","garbage order");
        check(f.trace[1].x==0xffffffffu && f.trace[1].z==0x80000000u,"garbage raw get coords");
        const auto& e=f.trace.back();
        check(e.x==0x7fffffffu && e.z==0x40000000u,"garbage shifted set coords");
        check(e.value==c.out,"garbage lowbit clearing/highbit input");
        check(e.dirty[3]==0,"garbage dirty after set"); dirtyOnly(f,3);
    }
    Fixture f; f.g.drop=true; SetGarbageValue(f.state,0,0,5);
    check(f.g.raw==0 && f.dirty[3]==1,"caller drop still dirty, not original bounds parity");
}
void queryChecks() {
    for(auto raw:{0u,0x7fffu,0x8000u,0xffffu,0x10000u,0xffff0000u,0xffffffffu}) {
        Fixture f; f.g.raw=raw;
        check(IsActiveLandfill(f.state,0xffffffffu,0x80000000u)==((raw&0x8000u)!=0),"query bit15");
        check(f.ops()=="G" && f.trace[0].x==0xffffffffu && f.trace[0].z==0x80000000u,"query raw coords");
        dirtyOnly(f,-1);
    }
}
void landfillChecks() {
    for(auto mode:{14u,0u,0x1000000eu})
    for(auto active:{false,true})
    for(auto prior:{0u,1u,2u,0x100u,0x101u,0x102u,0xffffffffu})
    for(auto count:{0u,1u,0xffffffffu}) {
        Fixture f; f.mode=mode; f.active=prior; f.count=count; f.g.raw=0x1234abcd;
        SetIsActiveLandfill(f.state,0xffffffffu,0x80000000u,active);
        const bool allowed=mode==14 || !active;
        const bool add=allowed && active && (prior%256==0);
        const bool remove=allowed && !active && (prior%256==1);
        check(f.ops()==(!allowed ? "M":add||remove ? "MQGS":"MQ"),"landfill dispatch matrix");
        const auto expected=add ? count+1u : remove && count!=0 ? count-1u : count;
        check(f.count==expected,"counter wrap/floor"); dirtyOnly(f,add||remove ? 3:-1);
        if(allowed) check(f.trace[1].x==0xffffffffu && f.trace[1].z==0x80000000u,"own query raw coords");
        if(add||remove) {
            check(f.trace[2].count==count,"counter after get");
            const auto& e=f.trace[3];
            check(e.x==0xffffffffu && e.z==0x80000000u,"landfill unshifted set coords");
            check(e.value==(add ? -21555:11213),"landfill raw ECX full getter transform");
            check(e.count==expected && e.dirty[3]==0,"counter before set and dirty after");
        }
    }
}
void reloadChecks() {
    for(int id=0;id<4;++id) {
        Fixture f; Mock* m[]={&f.r,&f.a,&f.w,&f.g}; m[id]->shift=1;
        m[id]->afterShift=[](Mock& m) {
            auto& f=*m.owner; Grid* grids[]={&f.state.radioactive,&f.state.air,&f.state.water,&f.state.garbage};
            *grids[m.id]=Fixture::grid(f.replacement);
        };
        f.replacement.raw=0x8000;
        if(id==0) SetIsRadioactive(f.state,8,10,true);
        if(id==1) SetAirValue(f.state,8,10,5);
        if(id==2) SetWaterValue(f.state,8,10,5);
        if(id==3) SetGarbageValue(f.state,8,10,5);
        check(f.trace.back().id==4 && f.trace.back().x==4,"grid reload after shift");
        if(id==3) check(f.trace[1].id==4 && f.trace.back().value==-32763,"garbage get reload");
    }
    for(int method=0;method<3;++method) {
        Fixture f; f.g.raw=0x1234; f.g.shift=1; f.active=method==2 ? 1:0;
        f.g.afterGet=[](Mock& m) { auto& f=*m.owner; f.state.garbage=Fixture::grid(f.replacement); };
        if(method==0) SetGarbageValue(f.state,8,10,5);
        else SetIsActiveLandfill(f.state,8,10,method==1);
        check(f.trace.back().id==4,"grid reload after get");
        check(f.trace.back().x==(method==0 ? 4u:8u),"reload keeps saved shift/raw coords");
    }
}
// Additional callback cases requested by W6_REVIEW.md; counted separately.
void reviewCallbackChecks() {
    {
        Fixture f;
        f.state.getMode=[](void* p)->std::uint32_t {
            auto& f=*static_cast<Fixture*>(p); Fixture::log(f.g,'M');
            f.state.queryActiveLandfill=[](void* p,std::uint32_t x,std::uint32_t z)->std::uint32_t {
                auto& f=*static_cast<Fixture*>(p); Fixture::log(f.g,'Q',x,z);
                f.state.garbage=Fixture::grid(f.replacement); return 0;
            };
            return 14;
        };
        f.active=2; f.replacement.raw=5;
        SetIsActiveLandfill(f.state,8,10,true);
        check(f.ops()=="MQGS","mode replacement query invoked");
        check(f.trace[2].id==4 && f.trace[3].id==4 && f.replacement.raw==0x8005,
              "query replacement garbage descriptor reread");
    }
    for(bool add:{false,true}) {
        Fixture f; f.active=add ? 0:1; f.count=42;
        f.g.afterGet=add ? +[](Mock& m) { m.owner->count=0xffffffffu; }
                        : +[](Mock& m) { m.owner->count=0; };
        SetIsActiveLandfill(f.state,8,10,add);
        check(f.count==0 && f.trace.back().count==0,"getter counter change used for wrap/floor");
    }
    for(bool add:{false,true}) {
        Fixture f; f.active=add ? 0:1;
        f.g.afterSet=[](Mock& m) { m.owner->count=77; m.owner->dirty[3]=9; };
        SetIsActiveLandfill(f.state,8,10,add);
        check(f.count==77,"setter counter mutation survives");
        check(f.dirty[3]==1,"dirty becomes exactly one after setter mutation");
    }
    for(auto prior:{0u,1u,2u}) for(bool add:{false,true}) {
        if((prior==0 && add) || (prior==1 && !add)) continue;
        Fixture f; f.active=prior; f.count=42; f.dirty={3,4,5,6};
        SetIsActiveLandfill(f.state,8,10,add);
        check(f.ops()=="MQ" && f.count==42 && f.dirty==std::array<std::uint8_t,4>{3,4,5,6},
              "no transition preserves nonzero dirty bytes and count");
    }
    {
        Fixture f; f.mode=0; f.dirty={3,4,5,6};
        SetIsActiveLandfill(f.state,8,10,true);
        check(f.ops()=="M" && f.dirty==std::array<std::uint8_t,4>{3,4,5,6},
              "mode rejection preserves nonzero dirty bytes");
    }
}

}
int main() {
    scalarChecks(); garbageChecks(); queryChecks(); landfillChecks(); reloadChecks();
    const auto originalChecks=checks;
    reviewCallbackChecks();
    std::cout << originalChecks << " original assertions + " << checks-originalChecks << " reviewer assertions; " << checks << " checks passed, 0 failed (standalone callbacks; no live parity claim)\n";
}
