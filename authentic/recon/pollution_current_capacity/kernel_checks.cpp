// Standalone discriminators for GetCurrentCapacity @ 0x1000992cc.
#include "opensc4/pollution_capacity_kernel.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <string>
using namespace opensc4::pollution_capacity;
static unsigned checks = 0;
static void check(bool ok, const char* what) {
    ++checks;
    if (!ok) { std::fprintf(stderr, "FAIL: %s\n", what); std::exit(1); }
}
static float raw(std::uint32_t bits) { float f; std::memcpy(&f,&bits,4); return f; }
struct Fixture {
    std::uint32_t output=77, capacity=10;
    bool present=true, powered=true, curve=true, input_present=true;
    float scale=1, response=1, input=9, seen_input=-1;
    int holders=0, destroyed=0, storage=0, holder1=0, holder2=0, property=0;
    char mutate=0, throws=0;
    std::uint32_t mutation=0, seen_scale=0, seen_destroy=0;
    std::string trace;
    void event(char c) {
        trace += c;
        if (mutate==c) output=mutation;
        if (throws==c) throw c;
    }
};
static Fixture& ctx(void* p) { return *static_cast<Fixture*>(p); }
static Callbacks callbacks {
    [](void* p,void* o)->void* { auto& f=ctx(p); check(o==p,"occupant forwarded");
        f.event('H'); return ++f.holders==1 ? &f.holder1 : &f.holder2; },
    [](void* p,void* h,std::uint32_t id,std::uint32_t& out) {
        auto& f=ctx(p); check(h==&f.holder1 && id==123,"capacity holder/id");
        check(&out==&f.output,"direct output alias"); out=f.capacity; f.event('C'); return f.present; },
    [](void* p,void* o) { auto& f=ctx(p); check(o==p,"power occupant"); f.event('P'); return f.powered; },
    [](void* p) { auto& f=ctx(p); f.seen_scale=f.output; f.event('S'); return f.scale; },
    [](void* p,void* h,std::uint32_t id)->void* { auto& f=ctx(p);
        check(h==&f.holder2 && id==0x8a1e07eeu,"refetched holder/curve id");
        f.event('L'); return f.curve ? &f.property : nullptr; },
    [](void* p,void* s) { auto& f=ctx(p); check(s==&f.storage,"construct storage"); f.event('N'); },
    [](void* p,void* s,void* prop) { auto& f=ctx(p);
        check(s==&f.storage && prop==&f.property,"raw curve load"); f.event('D'); },
    [](void* p,void* h,std::uint32_t id,float& input) { auto& f=ctx(p);
        check(h==&f.holder2 && id==0xc910b4cfu,"input holder/id");
        check(input==0,"input initialized before read");
        if(f.input_present) input=f.input;
        f.event('I'); return f.input_present; },
    [](void* p,void* s,float input) { auto& f=ctx(p); check(s==&f.storage,"evaluate storage");
        f.seen_input=input; f.event('E'); return f.response; },
    [](void* p,void* s) noexcept { auto& f=ctx(p); check(s==&f.storage,"destroy storage");
        f.seen_destroy=f.output; ++f.destroyed; f.event('X'); }
};
static bool run(Fixture& f) { return get_current_capacity({&f,&f.storage},callbacks,123,&f,f.output); }
int main() {
    { Fixture f; check(run(f),"normal true"); check(f.output==10,"normal output");
      check(f.trace=="HCPSHLNDIEX","complete invocation order"); check(f.destroyed==1,"normal cleanup"); }
    { Fixture f; f.present=false; check(!run(f),"missing false");
      check(f.output==0 && f.trace=="HC","missing capacity skips gate and scale"); }
    { Fixture f; f.curve=false; check(run(f),"absent curve true");
      check(f.output==10 && f.trace=="HCPSHL" && !f.destroyed,"absent curve no construction"); }
    for(float scale: {1.0f, std::numeric_limits<float>::infinity()}) {
      Fixture f; f.powered=false; f.scale=scale; f.curve=false;
      check(run(f) && f.output==0 && f.seen_scale==0,"power false zeros before scale, including inf");
      check(f.trace=="HCPSHL","power false still scales and refetches");
    }
    { Fixture f; f.input_present=false; check(run(f) && f.seen_input==0 && f.output==10,
      "missing float input ignored, zero used"); }
    // Callback writes discriminate every output snapshot boundary from the asm.
    for(char stage: {'H','C','P','S','L','N','D','I','E','X'}) {
      Fixture f; f.mutate=stage; f.mutation=6; f.response=2;
      check(run(f),"alias path true");
      const unsigned expected = stage=='X' ? 6 : (stage=='S'||stage=='E' ? 20 : 12);
      check(f.output==expected,"callback output mutation timing");
    }
    { Fixture f; f.powered=false; f.mutate='S'; f.mutation=999; f.curve=false;
      check(run(f) && f.output==0,"scale callback mutation overwritten by zero product"); }
    // CVTTSS2SI64 results calculated as trunc(value) mod 2^32; invalid -> low32(0x8000000000000000)=0.
    struct Numeric { float value; std::uint32_t expected; };
    const Numeric truncation[] = {
      {0,0},{-0.0f,0},{0.999f,0},{-0.999f,0},{1.5f,1},{-1.5f,0xffffffffu},
      {0x1.fffffep30f,0x7fffff80u},{0x1p31f,0x80000000u},{0x1.000002p31f,0x80000100u},
      {-0x1.fffffep30f,0x80000080u},{-0x1p31f,0x80000000u},{-0x1.000002p31f,0x7fffff00u},
      {0x1.fffffep31f,0xffffff00u},{0x1p32f,0},{0x1.000002p32f,512},
      {-0x1.fffffep31f,256},{-0x1p32f,0},{-0x1.000002p32f,0xfffffe00u},
      {0x1.fffffep62f,0},{0x1p63f,0},{0x1.000002p63f,0},
      {-0x1.fffffep62f,0},{-0x1p63f,0},{-0x1.000002p63f,0},
      {raw(0x7f800000),0},{raw(0xff800000),0},{raw(0x7fc12345),0},
      {std::numeric_limits<float>::max(),0},{-std::numeric_limits<float>::max(),0}
    };
    for(auto n: truncation) { Fixture f; f.capacity=1; f.scale=n.value; f.curve=false;
      check(run(f) && f.output==n.expected,"truncation bit fixture"); }
    for(auto n: {std::uint32_t(0x80000000),std::uint32_t(0xffffffff)}) {
      Fixture f; f.capacity=n; f.curve=false;
      check(run(f) && f.output==(n==0xffffffffu ? 0u : 0x80000000u),"unsigned conversion high bit");
    }
    // Binary32 addition at 12582912 has unit spacing: exact halves round to even.
    // Infinities/quiet NaNs retain raw bits before unsigned correction 0xb4c00000.
    const Numeric magic[] = {
      {2.5f,2},{3.5f,4},{raw(0x401fffff),2},{raw(0x40200001),3},
      {raw(0x405fffff),3},{raw(0x40600001),4},
      {-2.5f,0xfffffffeu},{-3.5f,0xfffffffcu},{0,0},{-0.0f,0},
      {raw(0x7f800000),0x34400000u},{raw(0xff800000),0xb4400000u},
      {raw(0x7fc12345),0x34812345u},{raw(0xffc12345),0xb4812345u}
    };
    for(auto n: magic) { Fixture f; f.capacity=1; f.response=n.value;
      check(run(f) && f.output==n.expected,"magic-add raw bit fixture"); }
    // MULSS infinity * zero generates x86 indefinite 0xffc00000, unlike
    // ARM64's default positive NaN; the integer correction exposes its sign.
    for (auto responseBits : {0x7f800000u, 0xff800000u}) {
      Fixture f; f.capacity=0; f.response=raw(responseBits);
      check(run(f) && f.output==0xb4800000u,"SSE generated NaN for zero capacity times infinity");
    }
    { Fixture f; f.mutate='I'; f.mutation=0x80000000u; f.response=0x1p-30f;
      check(run(f) && f.output==2,"unsigned highbit saved capacity conversion"); }
    { Fixture f; f.mutate='I'; f.mutation=0xffffffffu; f.response=0x1p-30f;
      check(run(f) && f.output==4,"UINT_MAX rounds to 2^32 before response multiply"); }
    for(char stage: {'H','C','P','S','L','N','D','I','E'}) {
      Fixture f; f.throws=stage; bool caught=false;
      try { (void)run(f); } catch(char c) { caught=(c==stage); }
      check(caught,"callback exception propagates");
      check(f.destroyed==((stage=='D'||stage=='I'||stage=='E')?1:0),"cleanup iff constructed");
      if(f.destroyed) check(f.trace.back()=='X',"cleanup last during unwind");
    }
    std::printf("GetCurrentCapacity: %u assertions passed, 0 failed\n",checks);
}
