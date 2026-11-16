#include "opensc4/tract_developer.h"
#include "opensc4/pollution_capacity_kernel.h"
#include <bit>
#include <cstdio>
#include <limits>
#ifdef CURVE_ARITHMETIC_MATRIX
#include "opensc4/sse_float32.h"
#endif
using namespace opensc4;
static int checks=0, failures=0;
static void Check(bool ok, const char* label) { ++checks; if(!ok) {++failures; printf("FAIL %s\n",label);} }
[[maybe_unused]] static float f(unsigned u){return std::bit_cast<float>(u);}
[[maybe_unused]] static unsigned b(float v){return std::bit_cast<unsigned>(v);}
int main() {
// CURVE_NAN_REGRESSIONS_BEGIN (requires bit, pollution_capacity_kernel includes)
{
 using Curve=cSC4ResponseCurve;
 const auto f=[](unsigned u){return std::bit_cast<float>(u);};
 const auto b=[](float v){return std::bit_cast<unsigned>(v);};
 const float inf=f(0x7f800000u);
 const auto run=[&](const char* label,std::initializer_list<Curve::Point> pts,
                    float input,unsigned expected,unsigned expectedCapacity,bool slope) {
   Curve curve; curve.SetResponseCurveData(std::span(pts.begin(),pts.size()));
   float out=42; bool ok=curve.TryGetOutputValue(input,out);
   struct C { Curve* curve; float input; } ctx{&curve,input};
   using namespace pollution_capacity;
   Callbacks cb{
    [](void* p,void*)->void*{return p;},
    [](void*,void*,unsigned,unsigned& o){o=100;return true;},
    [](void*,void*){return true;},[](void*){return 1.0f;},
    [](void* p,void*,unsigned)->void*{return p;},
    [](void*,void*){},[](void*,void*,void*){},
    [](void* p,void*,unsigned,float& o){o=static_cast<C*>(p)->input;return true;},
    [](void* p,void*,float i){float o=0;static_cast<C*>(p)->curve->TryGetOutputValue(i,o);return o;},
    [](void*,void*) noexcept{}};
#if defined(__x86_64__) && defined(CURVE_NAN_VERBOSE)
   // Independent instruction oracle pins fixture constants; no compiler arithmetic.
   const auto& seg=curve.Segments().back(); float ref=input;
   asm volatile("subss %1,%0; mulss %2,%0; addss %3,%0"
       : "+x"(ref): "m"(seg.prevX),"m"(seg.slope),"m"(seg.prevY));
   Check(b(ref)==expected,"fixture expected bits pinned by explicit SSE");
   if(slope) {
     float numerator=inf,denominator=-inf;
     asm volatile("divss %1,%0":"+x"(numerator):"x"(denominator));
     Check(b(numerator)==expected,"slope expected bits pinned by explicit DIVSS");
   }
   const float capacity=100.0f,bias=12582912.0f;
   asm volatile("mulss %1,%0; addss %2,%0":"+x"(ref):"m"(capacity),"m"(bias));
   Check(b(ref)+0xb4c00000u==expectedCapacity,"capacity expected pinned by explicit SSE");
#endif
   unsigned cap=0; bool capOK=get_current_capacity({&ctx,&curve},cb,123,&ctx,cap);
   Check(ok && b(out)==expected && capOK && cap==expectedCapacity &&
         (!slope || (curve.Segments().size()>1 && b(curve.Segments()[1].slope)==expected)),label);
#ifdef CURVE_NAN_VERBOSE
   printf("%s out=%08x expected=%08x capacity=%08x expected=%08x\n",label,b(out),expected,cap,expectedCapacity);
#endif
 };
 run("invalid-sub",{{inf,1},{f(0x7fc12345),1}},inf,0xffc00000,0xb4800000,false);
 run("invalid-mul",{{-inf,1},{inf,1}},0,0xffc00000,0xb4800000,false);
 run("invalid-add",{{0,1},{1,inf},{2,1}},1.5f,0xffc00000,0xb4800000,false);
 run("invalid-div",{{-2,1},{-inf,inf},{2,1}},1,0xffc00000,0xb4800000,true);
 run("mixed-positive-sNaN",{{f(0x7f812345),1},{2,1}},f(0xffc54321),0xffc54321,0xb4854321,false);
 run("mixed-negative-sNaN",{{f(0xff812345),1},{2,1}},f(0xffc54321),0xffc54321,0xb4854321,false);
}
// CURVE_NAN_REGRESSIONS_END
#ifdef CURVE_ARITHMETIC_MATRIX
 const unsigned operands[]={0,0x80000000,0x3f800000,0xbf800000,0x7f800000,0xff800000,
 0x7fc12345,0xffc54321,0x7f812345,0xff854321,1,0x00800000,0x7f7fffff,0x33800000};
 unsigned long long digest=1469598103934665603ull;
 for(int op=0;op<4;++op) for(unsigned a:operands) for(unsigned c:operands) {
  float x=f(a),y=f(c),actual;
  using namespace sse_float32;
  switch(op){case 0:actual=sub(x,y);break;case 1:actual=mul(x,y);break;case 2:actual=add(x,y);break;default:actual=div(x,y);}
  digest=(digest^b(actual))*1099511628211ull;
#ifdef __x86_64__
  float expected=x;
  switch(op){case 0:asm volatile("subss %1,%0":"+x"(expected):"x"(y));break;
  case 1:asm volatile("mulss %1,%0":"+x"(expected):"x"(y));break;
  case 2:asm volatile("addss %1,%0":"+x"(expected):"x"(y));break;
  default:asm volatile("divss %1,%0":"+x"(expected):"x"(y));}
  if(b(actual)!=b(expected)) printf("matrix op=%d a=%08x b=%08x actual=%08x sse=%08x\n",op,a,c,b(actual),b(expected));
  Check(b(actual)==b(expected),"explicit SSE operand matrix");
#endif
 }
 printf("matrix 784 cases digest=%016llx\n",digest);
#endif
 printf("%d checks %d failures\n",checks,failures);return failures?1:0;
}
