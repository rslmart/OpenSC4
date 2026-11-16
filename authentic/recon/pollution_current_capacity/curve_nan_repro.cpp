#include "opensc4/tract_developer.h"
#include <bit>
#include "opensc4/pollution_capacity_kernel.h"
#include <cstdio>
#include <limits>
using namespace opensc4;
float f(unsigned u){return std::bit_cast<float>(u);} unsigned b(float v){return std::bit_cast<unsigned>(v);}
#ifdef __x86_64__
float ref(float input,const cSC4ResponseCurve::Segment& s){asm volatile("subss %1,%0; mulss %2,%0; addss %3,%0" : "+x"(input): "m"(s.prevX),"m"(s.slope),"m"(s.prevY));return input;}
#endif
unsigned capacity(cSC4ResponseCurve& curve,float input){
 struct C{cSC4ResponseCurve* c;float input;};C ctx{&curve,input};
 using namespace opensc4::pollution_capacity;
 Callbacks cb{
 [](void* p,void*)->void*{return p;},
 [](void*,void*,unsigned,unsigned& out){out=100;return true;},
 [](void*,void*){return true;}, [](void*){return 1.0f;},
 [](void* p,void*,unsigned)->void*{return p;},
 [](void*,void*){},[](void*,void*,void*){},
 [](void* p,void*,unsigned,float& out){out=static_cast<C*>(p)->input;return true;},
 [](void* p,void*,float in){float out;static_cast<C*>(p)->c->TryGetOutputValue(in,out);return out;},
 [](void*,void*) noexcept{}};
 unsigned out;get_current_capacity({&ctx,&curve},cb,123,&ctx,out);return out;
}
void run(const char* name,std::initializer_list<cSC4ResponseCurve::Point> p,float input){
 cSC4ResponseCurve c;c.SetResponseCurveData(std::span(p.begin(),p.size()));float out=42;c.TryGetOutputValue(input,out);
 printf("%-18s input=%08x n=%zu out=%08x",name,b(input),c.Segments().size(),b(out));
 if(c.Segments().size()>1){auto s=c.Segments().back();printf(" slope=%08x prevX=%08x prevY=%08x",b(s.slope),b(s.prevX),b(s.prevY));
#ifdef __x86_64__
 printf(" sse=%08x",b(ref(input,s)));
#endif
 }
 volatile float cap=100; float product=out*cap;volatile float bias=12582912.0f;printf(" capacity=%08x kernel=%08x\n",b(product+bias)+0xb4c00000u,capacity(c,input));
}
int main(){float inf=std::numeric_limits<float>::infinity();
 run("invalid-sub",{{inf,1},{f(0x7fc12345),1}},inf);
 run("invalid-mul",{{-inf,1},{inf,1}},0);
 run("invalid-add",{{0,1},{1,inf},{2,1}},1.5f);
 run("invalid-div",{{-2,1},{-inf,inf},{2,1}},1);
 for(unsigned n:{0x7fc12345u,0xffc12345u,0x7f812345u,0xff812345u}){
 run("nan-input",{{0,1},{2,2}},f(n));
 run("nan-prevX",{{f(n),1},{2,1}},1);
 run("two-nan",{{f(n),1},{2,1}},f(0xffc54321));
 run("nan-and-indef",{{-2,1},{-inf,inf},{2,1}},f(n));
 }
}
