#include "opensc4/pollution_aging_math.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <limits>
#include <cfenv>
#include <utility>
#if defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace {
using namespace opensc4::aging_math;
std::uint64_t checks = 0, hash = 14695981039346656037ull;
float f(std::uint32_t u) { return std::bit_cast<float>(u); }
std::uint32_t b(float v) { return std::bit_cast<std::uint32_t>(v); }
void eq(std::uint32_t actual, std::uint32_t expected) {
    ++checks;
    for (unsigned i=0;i<4;++i) { hash ^= (actual >> (i*8)) & 255; hash *= 1099511628211ull; }
    if (actual != expected) { std::fprintf(stderr,"check %llu: %08x != %08x\n",checks,actual,expected); std::abort(); }
}
#if defined(__x86_64__)
float oracle_narrow(std::uint64_t raw) {
    double d = std::bit_cast<double>(raw); float out;
    asm volatile("xorps %%xmm0, %%xmm0; cvtsd2ss %1, %%xmm0; movss %%xmm0, %0"
                 : "=m"(out) : "m"(d) : "xmm0"); return out;
}
float oracle_factor(std::uint32_t id, std::int32_t used, std::uint32_t cap,
                    std::uint32_t count, std::uint64_t raw, std::uint32_t initial) {
    if (id ? (cap==0 || used<=0) : count==0) return f(initial);
    float n;
    if (id) asm volatile("xorps %%xmm0, %%xmm0; cvtsi2ss %1, %%xmm0; movss %%xmm0, %0"
                         : "=m"(n) : "r"(used) : "xmm0");
    else n=oracle_narrow(raw);
    const std::int64_t den=id?cap:count;
    asm volatile("xorps %%xmm1, %%xmm1; cvtsi2ss %1, %%xmm1; divss %%xmm1, %0"
                 : "+x"(n) : "r"(den) : "xmm1"); return n;
}
float oracle_update(float funding_result,float multiplier,float factor_result,float previous) {
    asm volatile("mulss %1, %0" : "+x"(funding_result) : "x"(multiplier));
    // Destination changes before previous is consumed; prevent register aliasing.
    asm volatile("mulss %1, %0; addss %2, %0" : "+&x"(factor_result) : "x"(funding_result), "x"(previous));
    return factor_result;
}
unsigned oracle_request(float updated,float lower,float upper) {
    unsigned result;
    asm volatile("ucomiss %3, %1; jae 1f; ucomiss %2, %1; jb 2f; movl $2, %0; jmp 3f;"
                 "1: movl $4, %0; jmp 3f; 2: xorl %0, %0; 3:"
                 : "=&r"(result) : "x"(updated), "x"(lower), "x"(upper) : "cc"); return result;
}
#endif
std::uint64_t rng=0xa6719c88b35d0101ull;
std::uint64_t next() { rng ^= rng<<13; rng ^= rng>>7; rng ^= rng<<17; return rng; }
}
int aging_math_checks() {
    checks=0; hash=14695981039346656037ull; rng=0xa6719c88b35d0101ull;
#if defined(__x86_64__)
    const unsigned saved=_mm_getcsr();
#endif
    fenv_t old_env; std::fegetenv(&old_env); std::fesetenv(FE_DFL_ENV);
#if defined(__x86_64__)
    _mm_setcsr(0x1f80);
#endif
    // Fixed binary64 -> binary32 RNE goldens: ties, subnormals, overflow,
    // both zero signs, infinities, signed quiet/signaling NaN payloads.
    constexpr std::pair<std::uint64_t,std::uint32_t> narrowing[] = {
        {0,0},{0x8000000000000000ull,0x80000000},{1,0},{0x8000000000000001ull,0x80000000},
        {0x3ff0000010000000ull,0x3f800000},{0x3ff0000010000001ull,0x3f800001},
        {0x3ff0000030000000ull,0x3f800002},{0x3690000000000000ull,0},
        {0x3690000000000001ull,1},{0x36a0000000000000ull,1},
        {0x380fffffe0000000ull,0x00800000},{0x47efffffe0000000ull,0x7f7fffff},
        {0x47effffff0000000ull,0x7f800000},{0x7fefffffffffffffull,0x7f800000},
        {0x7ff0000000000000ull,0x7f800000},{0xfff0000000000000ull,0xff800000},
        {0x7ff0000000000001ull,0x7fc00000},{0xfff2468ac0000000ull,0xffd23456},
        {0x7ffabcdef0000000ull,0x7fd5e6f7}};
    for (auto [raw,want]:narrowing) {
        eq(b(narrow_double(raw)),want);
#if defined(__x86_64__)
        eq(b(narrow_double(raw)),b(oracle_narrow(raw)));
#endif
    }
    constexpr std::int32_t useds[]={INT32_MIN,-1,0,1,16777215,16777216,16777217,16777219,INT32_MAX};
    constexpr std::uint32_t counts[]={0,1,2,16777215,16777216,16777217,0x80000000,0xffffffff};
    for(auto initial:{0u,0x80000000u,0x7f812345u}) for(auto used:useds) for(auto cap:counts) {
        const auto got=b(factor(1,used,cap,17,0,initial));
        if(cap==0 || used<=0) eq(got,initial);
#if defined(__x86_64__)
        eq(got,b(oracle_factor(1,used,cap,17,0,initial)));
#endif
    }
    eq(b(factor(1,INT32_MAX,0xffffffff,0,0)),0x3f000000);
    eq(b(factor(1,16777217,1,0,0)),0x4b800000);
    eq(b(factor(1,16777219,1,0,0)),0x4b800002);
    eq(b(factor(0,999,123,2,0x4010000000000000ull)),0x40000000);
    eq(b(factor(0,999,123,0,0x7ff0000000000001ull,0x7f812345)),0x7f812345);
    for(auto count:counts) for(auto [raw,want]:narrowing) {
        (void)want; auto got=b(factor(0,INT32_MIN,123,count,raw,0x80000000));
        if(count==0) eq(got,0x80000000);
#if defined(__x86_64__)
        eq(got,b(oracle_factor(0,INT32_MIN,123,count,raw,0x80000000)));
#endif
    }
    constexpr std::uint32_t values[]={0,0x80000000,1,0x80000001,0x3f800000,0xbf800000,
      0x3f800001,0x7f7fffff,0xff7fffff,0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f812345};
    for(auto a:values) for(auto m:values) for(auto c:values) for(auto p:values) {
        int order=0;
        auto got=update_age(f(0x3eaaaaab),f(m),f(0x3f012345),f(p),
            [&](float x){eq(b(x),0x3eaaaaab); eq(++order,1);return f(a);},
            [&](float x){eq(b(x),0x3f012345); eq(++order,2);return f(c);});
#if defined(__x86_64__)
        eq(b(got),b(oracle_update(f(a),f(m),f(c),f(p))));
#else
        (void)got;
#endif
    }
    auto update=[](unsigned a,unsigned m,unsigned c,unsigned p){return b(update_age(0,f(m),0,f(p),[&](float){return f(a);},[&](float){return f(c);}));};
    eq(update(0x3f800001,0x3f7ffffe,0x3f800000,0xbf800000),0);
    eq(update(0x7fc11111,0x7fc22222,0x7fc33333,0x7fc44444),0x7fc33333);
    eq(update(0x7fc11111,0x7fc22222,0x3f800000,0x7fc44444),0x7fc11111);
    eq(update(0x3f800000,0x3f800000,0x3f800000,0x7f844444),0x7fc44444);
    eq(update(0x80000000,0x3f800000,0x3f800000,0x80000000),0x80000000);
    eq(update(0x7f800000,0,0x3f800000,0),0xffc00000);
    for(auto a:values) for(auto l:values) for(auto u:values) {
        auto got=static_cast<unsigned>(select_request(f(a),f(l),f(u)));
#if defined(__x86_64__)
        eq(got,oracle_request(f(a),f(l),f(u)));
#else
        (void)got;
#endif
    }
    eq(unsigned(select_request(1,1,f(0x7fc12345))),2);
    eq(unsigned(select_request(1,f(0x7fc12345),1)),4);
    eq(unsigned(select_request(f(0x7fc12345),0,0)),0);
    eq(unsigned(select_request(1,f(0x7fc12345),2)),0);
    eq(unsigned(select_request(0,1,2)),0); eq(unsigned(select_request(1,1,2)),2);
    eq(unsigned(select_request(2,1,2)),4);
    for(unsigned i=0;i<100000;++i) {
        const auto raw=next(); const std::uint32_t count=next(),a=next(),m=next(),c=next(),p=next();
        const auto used=std::bit_cast<std::int32_t>(a);
#if defined(__x86_64__)
        eq(b(narrow_double(raw)),b(oracle_narrow(raw)));
        eq(b(factor(i&1,used,m,count,raw,p)),b(oracle_factor(i&1,used,m,count,raw,p)));
        eq(update(a,m,c,p),b(oracle_update(f(a),f(m),f(c),f(p))));
        eq(unsigned(select_request(f(a),f(m),f(c))),oracle_request(f(a),f(m),f(c)));
#else
        // Exercise every operation under sanitizers; no independent oracle here.
        volatile auto sink=b(narrow_double(raw)) ^ b(factor(i&1,used,m,count,raw,p)) ^ update(a,m,c,p)
            ^ unsigned(select_request(f(a),f(m),f(c))); (void)sink;
#endif
    }
    std::fesetenv(&old_env);
#if defined(__x86_64__)
    _mm_setcsr(saved);
#endif
    std::printf("aging_math checks=%llu fnv1a64=%016llx random=100000\n",checks,hash);
    return 0;
}
#ifdef OPENSC4_AGING_MATH_STANDALONE
int main() { return aging_math_checks(); }
#endif

void RunPollutionAgingMathChecks(void (*check)(bool, const char*)) {
    check(aging_math_checks() == 0, "Pollution aging arithmetic @0x100099c88 boundary corpus");
}
