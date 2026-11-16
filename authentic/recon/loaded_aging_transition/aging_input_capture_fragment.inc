// clean-room fallback (not authentic): bounded diagnostic acquisition policy.
// Include AFTER fixture_placement_bindings and BEFORE placement_reload_fragment.
#ifndef OPENSC4_AGING_INPUT_CAPTURE_FRAGMENT_INC
#define OPENSC4_AGING_INPUT_CAPTURE_FRAGMENT_INC
namespace aging_input_capture {
namespace f=fixture_placement_bindings;
using namespace fixture_placement;
enum class Fault { None, Unreadable, NullMismatch, NegativeCount, Order, Alignment, CountSpan, Limit, Overflow, Guard, Unsupported };
struct Snapshot {
    bool complete=false;
    Fault fault=Fault::None;
    const char* stage="not_started";
    std::vector<uint64_t> identity,data;
};
static bool alias(uint64_t a,uint64_t b) { return a && b && a==b; }
static bool add(uint64_t a,uint64_t b,uint64_t& out) { if(a>UINT64_MAX-b) return false; out=a+b; return true; }
struct NativeReader { bool operator()(uint64_t p,void* b,size_t n) const { return read(p,b,n); } };
struct Curve {
    uint64_t begin=0,end=0,capacity=0; int32_t count=0;
    uint32_t tail=0; // all 32 inline object bytes, including last raw word
    std::vector<uint32_t> words;
};
// Layout: SetResponseCurveData (input_curve_setdata_fresh_asm.txt), GetOutputValue @ 0x1000ac4fe.
// No evaluator call. Exact count/span is ADAPTER policy, not game behavior.
template<class R> static Fault curve(R rd,uint64_t p,Curve& c) {
    unsigned char raw[32]{}; uint64_t last=0;
    if(!add(p,32,last)) return Fault::Overflow;
    if(!rd(p,raw,32)) return Fault::Unreadable;
    std::memcpy(&c.begin,raw,8); std::memcpy(&c.end,raw+8,8); std::memcpy(&c.capacity,raw+16,8);
    std::memcpy(&c.count,raw+24,4); std::memcpy(&c.tail,raw+28,4);
    if(c.count<0) return Fault::NegativeCount;
    if(!c.begin || !c.end || !c.capacity) {
        if(c.begin || c.end || c.capacity || c.count) return Fault::NullMismatch;
        return Fault::None;
    }
    if(c.begin>c.end || c.end>c.capacity) return Fault::Order;
    if((c.begin&3) || (c.end-c.begin)%20 || (c.capacity-c.begin)%20) return Fault::Alignment;
    if((c.end-c.begin)/20!=uint64_t(c.count)) return Fault::CountSpan;
    if(c.count>4096) return Fault::Limit;
    c.words.resize(size_t(c.count)*5);
    if(!c.words.empty() && !rd(c.begin,c.words.data(),c.words.size()*4)) return Fault::Unreadable;
    return Fault::None;
}
struct Entries { uint64_t begin=0,end=0; std::vector<uint64_t> raw; };
// Local @100049fc4 reads +10/+18; default primary @10030113c +58/+60.
// Both layouts contain 16-byte entries. The default thunk @100301196 subtracts28.
template<class R> static Fault entries(R rd,uint64_t p,unsigned offset,Entries& e) {
    uint64_t at=0,limit=0;
    if(!add(p,offset,at) || !add(at,16,limit)) return Fault::Overflow;
    uint64_t pair[2]{}; if(!rd(at,pair,16)) return Fault::Unreadable;
    e.begin=pair[0]; e.end=pair[1];
    if(!e.begin || !e.end) return e.begin==e.end ? Fault::None:Fault::NullMismatch;
    if(e.end<e.begin) return Fault::Order;
    if((e.begin&7) || (e.end-e.begin)%16) return Fault::Alignment;
    if((e.end-e.begin)/16>4096) return Fault::Limit;
    e.raw.resize(size_t((e.end-e.begin)/8));
    if(!e.raw.empty() && !rd(e.begin,e.raw.data(),e.raw.size()*8)) return Fault::Unreadable;
    for(size_t i=2;i<e.raw.size();i+=2)
        if(uint32_t(e.raw[i-2])>=uint32_t(e.raw[i])) return Fault::Order;
    return Fault::None;
}
static void push(Snapshot& s,uint64_t n) { s.data.push_back(n); }
static bool need(Snapshot& s,bool ok,const char* stage,Fault fault=Fault::Guard) {
    s.stage=stage; if(!ok) s.fault=fault; return ok;
}
static void pack(Snapshot& s,const Curve& c) {
    for(uint64_t n:{c.begin,c.end,c.capacity,uint64_t(uint32_t(c.count)),uint64_t(c.tail)}) push(s,n);
    for(auto n:c.words) push(s,n);
}
static void pack(Snapshot& s,const Entries& e) { push(s,e.begin); push(s,e.end); for(auto n:e.raw) push(s,n); }
struct Raw { uint64_t property=0,variant=0,payload=0; uint32_t type=0,count=0; std::vector<uint32_t> words; };
static bool raw(Snapshot& s,uint64_t p,uint32_t id,Raw& r,bool array=false) {
    r.property=p; push(s,p);
    if(!p) return true; // missing is a valid observation
    if(!need(s,object(p,0x100a69a40,0x24) && slot(p,0x28,0x10004c650) && slot(p,0x30,0x10004c65a),"property")) return false;
    uint32_t embedded=0;
    if(!need(s,read(p+0x20,embedded) && embedded==id,"property_id")) return false;
    r.variant=p+8;
    if(!need(s,object(r.variant,0x100a69ab0,0x18) && slot(r.variant,0xc0,0x10004f4d2) && slot(r.variant,0xd8,0x10004f5c8) && slot(r.variant,0x138,0x10004f998),"variant")) return false;
    if(!need(s,slot(r.variant,0x18,0x100051c00) && slot(r.variant,0x20,0x10004c682),"type_count_slots")) return false;
    r.type=original<uint32_t(*)(uint64_t)>(0x100051c00)(r.variant);
    r.count=original<uint32_t(*)(uint64_t)>(0x10004c682)(r.variant); r.payload=r.variant+8;
    push(s,r.variant); push(s,r.type); push(s,r.count);
    if(r.type==0x800a && array) {
        if(!need(s,r.count>=2 && r.count<=256 && !(r.count&1) && slot(r.variant,0x3a8,0x10004c804),"capacity_curve_domain",Fault::Unsupported)) return false;
        r.payload=original<uint64_t(*)(uint64_t)>(0x10004c804)(r.variant);
        r.words.resize(r.count);
    } else {
        if(!need(s,!array && (r.type==6 || r.type==7 || r.type==0xa),"scalar_domain",Fault::Unsupported)) return false;
        r.words.resize(1);
    }
    push(s,r.payload);
    if(!need(s,read(r.payload,r.words.data(),r.words.size()*4),"payload",Fault::Unreadable)) return false;
    for(auto n:r.words) push(s,n);
    return true;
}
static bool typed(uint64_t holder,uint32_t id,unsigned kind,uint32_t& bits) {
    // GetPropertyValue @10004b56e/5ac/636: RDI holder, ESI id, RDX host ref.
    if(kind==0) { int32_t v=0; std::memcpy(&v,&bits,4); bool ok=original<bool(*)(uint64_t,uint32_t,int32_t&)>(0x10004b56e)(holder,id,v); std::memcpy(&bits,&v,4); return ok; }
    if(kind==1) return original<bool(*)(uint64_t,uint32_t,uint32_t&)>(0x10004b5ac)(holder,id,bits);
    float v=0; std::memcpy(&v,&bits,4); bool ok=original<bool(*)(uint64_t,uint32_t,float&)>(0x10004b636)(holder,id,v); std::memcpy(&bits,&v,4); return ok;
}
// Verbatim input_capture_prefixes_fresh.txt: guarded entry prefixes, not fault containment.
static bool entry_guards() {
    struct Entry { uint64_t address; unsigned char bytes[16]; };
    static const Entry entries[]={
{0x100305da8,{0x55,0x48,0x89,0xe5,0x48,0x8d,0x47,0x28,0x5d,0xc3,0x55,0x48,0x89,0xe5,0x48,0x8d}},
{0x100301196,{0x55,0x48,0x89,0xe5,0x48,0x83,0xc7,0xd8,0x5d,0xe9,0x98,0xff,0xff,0xff,0x55,0x48}},
{0x10030113c,{0x55,0x48,0x89,0xe5,0x48,0x8b,0x47,0x58,0x4c,0x8b,0x47,0x60,0x4c,0x89,0xc2,0x48}},
{0x100049fc4,{0x55,0x48,0x89,0xe5,0x48,0x8b,0x47,0x10,0x4c,0x8b,0x47,0x18,0x4c,0x89,0xc2,0x48}},
{0x10004b56e,{0x55,0x48,0x89,0xe5,0x53,0x50,0x48,0x89,0xd3,0x48,0x8b,0x07,0xff,0x50,0x28,0x48}},
{0x10004b5ac,{0x55,0x48,0x89,0xe5,0x53,0x50,0x48,0x89,0xd3,0x48,0x8b,0x07,0xff,0x50,0x28,0x48}},
{0x10004b636,{0x55,0x48,0x89,0xe5,0x53,0x50,0x48,0x89,0xd3,0x48,0x8b,0x07,0xff,0x50,0x28,0x48}},
{0x10004f998,{0x55,0x48,0x89,0xe5,0x41,0x56,0x53,0x48,0x83,0xec,0x20,0x48,0x89,0xf3,0x0f,0xb7}},
{0x1000992cc,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec}},
{0x100243d94,{0x55,0x48,0x89,0xe5,0x48,0x8d,0x47,0x54,0x5d,0xc3,0x55,0x48,0x89,0xe5,0x41,0x57}},
    };
    for(const auto& e:entries) { unsigned char b[16]{};
        if(!read(e.address+g_slide,b,sizeof b) || std::memcmp(b,e.bytes,sizeof b)) return false;
    }
    return true;
}
static bool capture(Snapshot& s,const State& core,const f::ListOccurrenceObservation& target) {
#define AI_NEED(x,why) do { if(!need(s,(x),why)) return false; } while(0)
    AI_NEED(entry_guards(),"entry_prefixes");
    const auto& o=target.value;
    s.identity={target.index,target.node,o.occupant,o.base,o.holder,o.exemplar,o.lot.lot,o.lot.descriptor};
    for(auto n:o.tgi) s.identity.push_back(n);
    for(auto n:{o.lot.rect.l,o.lot.rect.t,o.lot.rect.r,o.lot.rect.b}) s.identity.push_back(uint32_t(n));
    // Retain newly acquired geometry/lot observations as input context too.
    for(auto n:o.position_bounds) push(s,n);
    for(auto n:o.stored_position) push(s,n);
    for(auto n:o.lot.raw_bounds) push(s,n);
    push(s,o.lot.state); push(s,o.lot.condition);
    uint64_t pollution=0,vptr=0,department=0,funding=0;
    AI_NEED(read(core.city+0x2d8,pollution) && object(pollution,0x100a6ce20,0x34c) && read(pollution,vptr),"pollution_primary");
    push(s,pollution); push(s,vptr); push(s,0); // guarded primary adjustment
    AI_NEED(read(pollution+0x1c0,department) && object(department,0x100a88dd0,0x58) && slot(department,0x50,0x100243d94),"department");
    funding=original<uint64_t(*)(uint64_t)>(0x100243d94)(department);
    uint32_t bits=0; AI_NEED(funding==department+0x54 && read(funding,bits),"funding_return");
    push(s,department); push(s,0x100a88dd0+g_slide); push(s,0x100243d94+g_slide); push(s,funding); push(s,funding==department+0x54); push(s,bits);
    for(unsigned off:{0x1c8u,0x1e8u}) {
        Curve c; s.fault=curve(NativeReader{},pollution+off,c); pack(s,c);
        s.stage="inline_curve"; if(s.fault!=Fault::None) return false;
    }
    AI_NEED(read(0x100b78a18+g_slide,bits),"runtime_zero"); push(s,bits);
    AI_NEED(o.occupant>=8 && object(o.occupant-8,0x100a7b900,0x84) && object(o.occupant,0x100a7ba08,0x7c) && slot(o.occupant,0x30,0x10019bb80),"occupant");
    const uint64_t holder=original<uint64_t(*)(uint64_t)>(0x10019bb80)(o.occupant);
    uint64_t exemplar=0;
    AI_NEED(holder==o.holder && holder==o.occupant+0x10 && object(holder,0x100a7bb90,0x38) && slot(holder,0x28,0x10004ae40) && read(holder+0x30,exemplar) && exemplar==o.exemplar,"holder");
    AI_NEED(object(exemplar,0x100a95bc0,0x68) && slot(exemplar,0x30,0x100305da8),"exemplar");
    const uint64_t def=original<uint64_t(*)(uint64_t)>(0x100305da8)(exemplar);
    AI_NEED(def==exemplar+0x28 && object(def,0x100a95d10,0x40) && slot(def,0x28,0x100301196),"default_holder");
    push(s,holder); push(s,exemplar); push(s,def);
    for(uint64_t p:{o.base,o.occupant,holder,exemplar,def,o.lot.lot}) {
        uint64_t vt=0; AI_NEED(read(p,vt),"identity_vptr_read"); push(s,p); push(s,vt);
    }
    logf_("AGING_INPUT_CAPTURE ROOTS pollution=%llx raw_primary_vptr=%llx primary_adjustment=0 node=%llx occupant=%llx base=%llx holder=%llx exemplar=%llx default_holder=%llx lot=%llx funding_receiver=%llx funding_entry=%llx funding_return=%llx return_equals_plus54=1",U(pollution),U(vptr),U(target.node),U(o.occupant),U(o.base),U(holder),U(exemplar),U(def),U(o.lot.lot),U(department),U(0x100243d94+g_slide),U(funding));
    Entries local,defaults;
    s.fault=entries(NativeReader{},holder,0x10,local); s.stage="local_vector"; pack(s,local); if(s.fault!=Fault::None) return false;
    s.fault=entries(NativeReader{},exemplar,0x58,defaults); s.stage="default_vector"; pack(s,defaults); if(s.fault!=Fault::None) return false;
    const uint32_t ids[]={0x28ecf510,0xc8ebd131,0xc910b4cf,0xea1cf220,0xea1cf221,0xea1cf222};
    for(unsigned i=0;i<6;++i) {
        const uint32_t id=ids[i]; push(s,id);
        uint64_t lp=original<uint64_t(*)(uint64_t,uint32_t)>(0x100049fc4)(holder,id);
        uint64_t dp=original<uint64_t(*)(uint64_t,uint32_t)>(0x100301196)(def,id);
        uint64_t ep=original<uint64_t(*)(uint64_t,uint32_t)>(0x10004ae40)(holder,id);
        for(uint64_t p:{lp,dp,ep}) { Raw r; if(!raw(s,p,id,r)) return false; }
        push(s,alias(lp,dp)); push(s,alias(lp,ep)); push(s,alias(dp,ep));
        bits=i>=4 ? 0x42c60000:0;
        bool result=typed(holder,id,i<2?i:2,bits); push(s,result); push(s,bits);
        logf_("AGING_INPUT_CAPTURE PROPERTY id=%08x local=%llx default=%llx effective=%llx nonnull_local_default_alias=%u nonnull_local_effective_alias=%u nonnull_default_effective_alias=%u helper_return=%u helper_initial=%08x helper_output=%08x",id,U(lp),U(dp),U(ep),unsigned(alias(lp,dp)),unsigned(alias(lp,ep)),unsigned(alias(dp,ep)),unsigned(result),i>=4?0x42c60000u:0u,bits);
    }
    // Capacity's transitive curve @1000992cc; absent permitted, present bounded.
    Raw response; uint64_t rp=original<uint64_t(*)(uint64_t,uint32_t)>(0x10004ae40)(holder,0x8a1e07ee);
    if(!raw(s,rp,0x8a1e07ee,response,true)) return false;
    uint64_t manager=0,power=0;
    AI_NEED(read(0x100b21340+g_slide,manager) && manager>=8 && object(manager-8,0x100a758e0,0xa0) && object(manager,0x100a75a28,8) && slot(manager,0x70,0x100131184) && slot(manager-8,0x80,0x100131192),"lot_manager");
    const uint64_t lot=original<uint64_t(*)(uint64_t,uint64_t)>(0x100131184)(manager,o.occupant);
    AI_NEED(lot==o.lot.lot && object(lot,0x100a79fb0,0xce) && slot(lot,0x208,0x100183b3e) && slot(lot,0x210,0x100183b50),"lot");
    AI_NEED(read(pollution+0x60,power) && object(power,0x100a86a30,8) && slot(power,0xf8,0x10021485c),"power");
    uint8_t flags=0; AI_NEED(read(lot+0xcd,flags) && read(pollution+0x348,bits) && slot(pollution,0x1a8,0x1000992cc),"capacity_guards");
    push(s,manager); push(s,lot); push(s,power); push(s,flags); push(s,bits);
    uint32_t capacity=0;
    const bool result=original<bool(*)(uint64_t,uint32_t,uint64_t,uint32_t&)>(0x1000992cc)(pollution,0xc8ebd131,o.occupant,capacity);
    push(s,result); push(s,capacity);
    logf_("AGING_INPUT_CAPTURE CAPACITY original=1000992cc initial=00000000 returned=%u output=%08x scale_raw=%08x power=%llx manager=%llx lot=%llx direct_curve_evaluation=0 internal_local_curve_allocate_evaluate_free_possible=1",unsigned(result),capacity,bits,U(power),U(manager),U(lot));
    s.stage="captured"; s.complete=true;
#undef AI_NEED
    return true;
}
// Diagnostic post-query guard: the reused inventory's final check precedes these
// extra native queries. Reacquire afterward; this still is not an atomic freeze.
template<class StateReader,class IdentityCheck>
static bool finish_capture(Snapshot& snapshot,const State& before,StateReader rd,IdentityCheck identity,const char* phase) {
    State after{};
    const bool readable=rd(after);
    const bool stable=readable && same(before,after);
    const bool bound=stable && identity(after);
    logf_("AGING_INPUT_CAPTURE POST phase=%s readable=%u stable=%u identity=%u values_valid_only_if_readable=1 app=%llx city=%llx sim=%llx date=%u pauses=%d,%d,%d non_atomic=1",phase,unsigned(readable),unsigned(stable),unsigned(bound),U(after.app),U(after.city),U(after.sim),after.date,after.pauses[0],after.pauses[1],after.pauses[2]);
    if(!bound) { snapshot.complete=false; snapshot.fault=Fault::Guard; snapshot.stage="post_query_state_or_identity"; }
    return bound;
}
static bool same_target(const Snapshot& a,const Snapshot& b) { return a.identity==b.identity; }
static bool same_inputs(const Snapshot& a,const Snapshot& b) { return a.data==b.data; }
static void log(const Snapshot& s,const char* phase) {
    logf_("AGING_INPUT_CAPTURE SAMPLE phase=%s complete=%u fault=%u stage=%s non_atomic=1 native_transitive_allocator_query_fault_scope=1 no_direct_curve_evaluation=1",phase,unsigned(s.complete),unsigned(s.fault),s.stage);
    for(size_t i=0;i<s.identity.size();++i) logf_("AGING_INPUT_CAPTURE ID phase=%s index=%zu raw=%llx",phase,i,U(s.identity[i]));
    for(size_t i=0;i<s.data.size();++i) logf_("AGING_INPUT_CAPTURE DATA phase=%s index=%zu raw=%llx",phase,i,U(s.data[i]));
}
}
#endif
