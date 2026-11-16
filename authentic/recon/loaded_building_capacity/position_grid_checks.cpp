#include "position_grid_fragment.h"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cfenv>
#if defined(__x86_64__)
#include <xmmintrin.h>
#endif
using namespace opensc4::lot_position;
#if defined(__x86_64__)
// Independent oracle: literal scalar SSE instruction sequence from the packet.
PositionBits original_position(PositionBits a, PositionBits b) {
    PositionBits out;
    const uint32_t half = 0x3f000000;
    asm volatile(
        "movss 0(%1), %%xmm0\n\taddss 0(%2), %%xmm0\n\t"
        "movss (%3), %%xmm1\n\tmulss %%xmm1, %%xmm0\n\tmovss %%xmm0, 0(%0)\n\t"
        "movl 4(%1), %%eax\n\tmovl %%eax, 4(%0)\n\t"
        "movss 8(%1), %%xmm0\n\taddss 8(%2), %%xmm0\n\t"
        "mulss %%xmm1, %%xmm0\n\tmovss %%xmm0, 8(%0)"
        : : "r"(&out), "r"(&a), "r"(&b), "r"(&half)
        : "eax", "xmm0", "xmm1", "memory");
    return out;
}
Cell original_cell(PositionBits p) {
    Cell out;
    const uint32_t constants[] = {0x3d800000, 0xbf000000, 0x4b400000};
    asm volatile(
        "movss 0(%2), %%xmm0\n\tmovss 0(%1), %%xmm1\n\tmulss %%xmm0, %%xmm1\n\t"
        "movss 4(%2), %%xmm2\n\tmovaps %%xmm1, %%xmm3\n\taddss %%xmm2, %%xmm3\n\t"
        "movss 8(%2), %%xmm4\n\taddss %%xmm4, %%xmm3\n\tmovd %%xmm3, %%esi\n\t"
        "leal -0x4b400000(%%rsi), %%eax\n\taddl $0xb4c00001, %%esi\n\t"
        "xorps %%xmm3, %%xmm3\n\tcvtsi2ss %%esi, %%xmm3\n\tucomiss %%xmm3, %%xmm1\n\t"
        "cmovne %%eax, %%esi\n\tcmovp %%eax, %%esi\n\tmovl %%esi, 0(%0)\n\t"
        "mulss 8(%1), %%xmm0\n\taddss %%xmm0, %%xmm2\n\taddss %%xmm4, %%xmm2\n\t"
        "movd %%xmm2, %%edx\n\tleal -0x4b400000(%%rdx), %%eax\n\taddl $0xb4c00001, %%edx\n\t"
        "xorps %%xmm1, %%xmm1\n\tcvtsi2ss %%edx, %%xmm1\n\tucomiss %%xmm1, %%xmm0\n\t"
        "cmovne %%eax, %%edx\n\tcmovp %%eax, %%edx\n\tmovl %%edx, 4(%0)"
        : : "r"(&out), "r"(&p), "r"(constants)
        : "eax", "esi", "edx", "xmm0", "xmm1", "xmm2", "xmm3", "xmm4", "cc", "memory");
    return out;
}
#endif
// Adjacent encodings around +/-0, +/-half-cell, +/-integer-cell, 127/128/129,
// bias precision transitions, extrema, both NaN signs/payloads/signaling states.
constexpr uint32_t corpus[] = {
    0,0x80000000,1,0x80000001,0x007fffff,0x00800000,0x80800000,
    0x3f000000,0xbf000000,0x3f800000,0xbf800000,
    0x40ffffff,0x41000000,0x41000001,0xc0ffffff,0xc1000000,0xc1000001,
    0x417fffff,0x41800000,0x41800001,0xc17fffff,0xc1800000,0xc1800001,
    0x44fdffff,0x44fe0000,0x44fe0001,0x44ffffff,0x45000000,0x45000001,
    0x4500ffff,0x45010000,0x45010001,0x4bffffff,0x4c000000,0x4c000001,
    0xcbffffff,0xcc000000,0xcc000001,0x7f7fffff,0xff7fffff,
    0x7f800000,0xff800000,0x7fc12345,0xffc54321,0x7f800001,0xff800123,
    0x44100333,0x439706ab,0x44e2019a,0x441ffccd,0x43bf86ab,0x44e9fe66
};
uint64_t hp=14695981039346656037ull, hc=14695981039346656037ull;
void hash(uint64_t& h,uint32_t v) { for(int i=0;i<4;++i) { h ^= (v >> (i*8)) & 255; h *= 1099511628211ull; } }
size_t checked=0;
void check(PositionBits a,PositionBits b,bool digest) {
    const auto p=BuildingPosition(a,b); const auto c=OccupantCell(a);
#if defined(__x86_64__)
    const auto op=original_position(a,b); const auto oc=original_cell(a);
    if(p.x!=op.x || p.y!=op.y || p.z!=op.z || c.x!=oc.x || c.z!=oc.z) {
        std::fprintf(stderr,"Mismatch #%zu a=%08x,%08x,%08x b=%08x,%08x,%08x\n",checked,a.x,a.y,a.z,b.x,b.y,b.z);
        std::exit(1);
    }
#endif
    if(digest) {hash(hp,p.x);hash(hp,p.y);hash(hp,p.z);hash(hc,std::bit_cast<uint32_t>(c.x));hash(hc,std::bit_cast<uint32_t>(c.z));}
    ++checked;
}
int main() {
    std::fesetround(FE_TONEAREST);
#if defined(__x86_64__)
    _mm_setcsr(0x1f80); // masked, RNE, FTZ/DAZ off
#endif
    for(auto a:corpus) for(auto b:corpus) check({a,b,b},{b,a,a},true);
    const PositionBits lo{0x44100333,0x439706ab,0x44e2019a}, hi{0x441ffccd,0x43bf86ab,0x44e9fe66};
    check(lo,hi,true);
    const auto p=BuildingPosition(lo,hi); const auto c=OccupantCell(p);
    std::printf("goldens midpoint=%016llx cell=%016llx fixture=%08x,%08x,%08x -> %d,%d\n",
        (unsigned long long)hp,(unsigned long long)hc,p.x,p.y,p.z,c.x,c.z);
    // Fixed corpus digests from literal SSE oracle, x86_64 Rosetta, 2026-11-14.
    // FNV-1a over explicit little-endian output bytes, not host object layout.
    // Every contributing output was compared against the oracle before capture.
    if(hp != 0x1dda63f1f0ba62d8ull || hc != 0xfedad0cf800084acull ||
       p.x != 0x44180000u || p.y != 0x439706abu || p.z != 0x44e60000u ||
       c.x != 38 || c.z != 115) {
        std::fprintf(stderr,"Fixed SSE golden mismatch\n"); return 1;
    }
#if defined(__x86_64__)
    uint32_t state=0x91abcdef;
    auto next=[&] {state^=state<<13;state^=state>>17;state^=state<<5;return state;};
    for(int i=0;i<200000;++i) {PositionBits a{next(),next(),next()}, b{next(),next(),next()};check(a,b,false);}
#endif
    std::printf("PASS %zu cases\n",checked);
}
