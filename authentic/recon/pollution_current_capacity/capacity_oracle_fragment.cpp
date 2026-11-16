// GetCurrentCapacity @ 0x1000992cc: initial oracle contract from inline asm.
// Synthetic simulator: power interface at +0x60; float scale at +0x348.
// Occupant vt+0x30 returns holder; holder vt+0x28 finds raw curve property.
// Power vt+0xf8 returns AL; normalize the low byte, not the full return word.
// Missing capacity writes zero and returns false; curve-absent returns true.
// Curve lifetime: ctor 0x1000ac10e, setter 0x1000ac37a, eval 0x1000ac4f6.
// Owned curve storage is disposed through original stub 0x10092aeb0.
// Final rounding: float multiply, add bits 0x4b400000, bitcast + 0xb4c00000.
// Never evaluate fresh/null-backed or all-skipped/null-backed curves live.
namespace capacity_oracle_packet {
constexpr unsigned long long method = 0x1000992ccULL;
constexpr unsigned curve_property = 0x8a1e07eeU;
constexpr unsigned age_property = 0xc910b4cfU;
constexpr unsigned rounding_bias_bits = 0x4b400000U;
constexpr unsigned rounding_adjust = 0xb4c00000U;
}

// Included inside oracle_dylib.cpp's anonymous namespace. Tool-only POD ABI.
namespace capacity_oracle {
namespace kernel = opensc4::pollution_capacity;
template<class F> F original(uint64_t address) { return reinterpret_cast<F>(address + g_slide); }
float fl(uint32_t x) { float f; memcpy(&f, &x, 4); return f; }
uint32_t bits(float f) { uint32_t x; memcpy(&x, &f, 4); return x; }
struct Fixture;
struct Object { void** vt; Fixture* f; unsigned kind; };
struct Curve { void* begin; void* end; void* limit; int32_t count; uint32_t pad; };
static_assert(offsetof(Curve, count) == 0x18, "original curve layout");
struct Fixture {
    void* ov[7]{}; void* hv[6]{}; void* pv[7]{}; void* vv[118]{}; void* pwv[32]{};
    Object occ{}, holder{}, cap{}, age{}, prop{}, valcap{}, valage{}, valcurve{}, power{};
    alignas(16) unsigned char sim[0x350]{};
    uint32_t capacity=1, scale=0x3f800000, input=0, powered=0xab01;
    uint32_t cap_ok=0xab01, age_ok=0xab01;
    bool cap_present=true, age_present=true, curve_present=false;
    uint32_t raw[8]{}; uint32_t scalar_count=4;
    char trace[128]{}; unsigned used=0;
    uint32_t response=0; bool evaluated=false;
    void mark(char c) { if (used+1 < sizeof(trace)) { trace[used++]=c; trace[used]=0; } }
    void reset() { used=0; trace[0]=0; evaluated=false; }
};
void* holder(Object* o) { o->f->mark('H'); return &o->f->holder; }
void* lookup(Object* o, uint32_t id) {
    Fixture& f=*o->f;
    if (id==0x12345678) { f.mark('C'); return f.cap_present ? &f.cap : nullptr; }
    if (id==0xc910b4cf) { f.mark('I'); return f.age_present ? &f.age : nullptr; }
    f.mark(id==0x8a1e07ee ? 'R' : '!'); return id==0x8a1e07ee && f.curve_present ? &f.prop : nullptr;
}
void* value(Object* o) {
    o->f->mark(o->kind==0 ? 'c' : 'i');
    return o->kind==0 ? &o->f->valcap : &o->f->valage;
}
void* rawvalue(Object* o) { o->f->mark('V'); return &o->f->valcurve; }
uint32_t getcap(Object* o, uint32_t* out) { o->f->mark('U'); *out=o->f->capacity; return o->f->cap_ok; }
uint32_t getage(Object* o, float* out) { o->f->mark('F'); *out=fl(o->f->input); return o->f->age_ok; }
void* data(Object* o) { o->f->mark('D'); return o->f->raw; }
uint32_t count(Object* o) { o->f->mark('N'); return o->f->scalar_count; }
uint32_t powered(Object* o, void*) { o->f->mark('P'); return o->f->powered; }
void init(Fixture& f) {
    f.ov[6]=(void*)&holder; f.hv[5]=(void*)&lookup;
    f.pv[5]=(void*)&rawvalue; f.pv[6]=(void*)&value;
    f.vv[0xc0/8]=(void*)&getcap; f.vv[0x138/8]=(void*)&getage;
    f.vv[0x3a8/8]=(void*)&data; f.vv[4]=(void*)&count; f.pwv[31]=(void*)&powered;
    f.occ={f.ov,&f,0}; f.holder={f.hv,&f,0}; f.cap={f.pv,&f,0};
    f.age={f.pv,&f,1}; f.prop={f.pv,&f,2}; f.valcap={f.vv,&f,0};
    f.valage={f.vv,&f,1}; f.valcurve={f.vv,&f,2}; f.power={f.pwv,&f,0};
    void* p=&f.power; memcpy(f.sim+0x60,&p,8); memcpy(f.sim+0x348,&f.scale,4);
}
void construct(void*, void* c) { original<void(*)(void*)>(0x1000ac10e)(c); }
uint8_t set(Curve& c, void* p) { return uint8_t(original<uint32_t(*)(void*,void*)>(0x1000ac37a)(&c,p)); }
void load(void*,void* c,void* p) { (void)set(*static_cast<Curve*>(c),p); }
float eval(void* ctx,void* c,float x) {
    float y=original<float(*)(void*,float)>(0x1000ac4f6)(c,x);
    if(ctx) { auto& f=*static_cast<Fixture*>(ctx); f.response=bits(y); f.evaluated=true; }
    return y;
}
// Same pointer/reset/free stub path as 0x1000993d0..0x1000993dd.
// Never pass this allocation to the oracle's SDK-era libc++ destructor.
void destroy(void*,void* p) noexcept {
    auto& c=*static_cast<Curve*>(p);
    if(c.begin) { c.end=c.begin; original<void(*)(void*)>(0x10092aeb0)(c.begin); }
    c={};
}
const kernel::Callbacks callbacks={
    [](void*,void* o)->void* { return holder(static_cast<Object*>(o)); },
    [](void*,void* h,uint32_t id,uint32_t& out)->bool {
        return uint8_t(original<uint32_t(*)(void*,uint32_t,uint32_t*)>(0x10004b5ac)(h,id,&out))!=0;
    },
    [](void* ctx,void* o)->bool { auto& f=*static_cast<Fixture*>(ctx); return uint8_t(powered(&f.power,o))!=0; },
    [](void* ctx)->float { return fl(static_cast<Fixture*>(ctx)->scale); },
    [](void*,void* h,uint32_t id)->void* { return lookup(static_cast<Object*>(h),id); },
    construct,load,
    [](void*,void* h,uint32_t id,float& out)->bool {
        return uint8_t(original<uint32_t(*)(void*,uint32_t,float*)>(0x10004b636)(h,id,&out))!=0;
    },eval,destroy
};
struct Geometry { const char* name; uint32_t count; uint32_t raw[8]; };
const Geometry geometries[]={
    {"linear",4,{0,0x3f800000,0x3f800000,0x40000000}},
    {"half",2,{0,0x3f000000}},
    {"negative",2,{0,0xbf800000}},
    {"odd",5,{0,0x3f800000,0x3f800000,0x40000000,0x7fc12345}},
    {"zero",4,{0x80000000,0x80000000,0,0}},
    {"nan-x",6,{0,0x3f800000,0x7fc12345,0x40000000,0x40000000,0x40400000}},
    {"nan-y",6,{0,0x3f800000,0x3f800000,0x7fc12345,0x40000000,0x40400000}},
    // Finite raw ordinates yield -infinity slope via overflowing subtraction;
    // unlike a raw infinite first Y, the first record remains allocated.
    {"overflow-slope",4,{0,0x7f7fffff,0x3f800000,0xff7fffff}},
    {"epsilon-below",4,{0,0,0x2edbe6fe,0x3f800000}},
    {"epsilon-above",4,{0,0,0x2edbe700,0x3f800000}},
    {"dy-epsilon-below",4,{0,0,0x3f800000,0x2edbe6fe}},
    {"dy-epsilon-above",4,{0,0,0x3f800000,0x2edbe700}},
    {"epsilon-nearest",4,{0,0,0x2edbe6ff,0x3f800000}},
    {"dy-epsilon-nearest",4,{0,0,0x3f800000,0x2edbe6ff}},
    {"all-skipped-nan",2,{0,0x7fc12345}},
    {"all-skipped-inf",2,{0,0x7f800000}},
    {"odd-empty",1,{0x7fc12345}},
    // Independent review's exact raw-bit discriminators, CURVE_FIX_REVIEW.md.
    {"invalid-sub",4,{0x7f800000,0x3f800000,0x7fc12345,0x3f800000}},
    {"invalid-mul",4,{0xff800000,0x3f800000,0x7f800000,0x3f800000}},
    {"invalid-add",6,{0,0x3f800000,0x3f800000,0x7f800000,0x40000000,0x3f800000}},
    {"invalid-div",6,{0xc0000000,0x3f800000,0xff800000,0x7f800000,0x40000000,0x3f800000}},
    {"snan-positive-x",4,{0x7f812345,0x3f800000,0x40000000,0x3f800000}},
    {"snan-negative-x",4,{0xff812345,0x3f800000,0x40000000,0x3f800000}}
};
void geometry(Fixture& f,unsigned g) { f.scalar_count=geometries[g].count; memcpy(f.raw,geometries[g].raw,sizeof(f.raw)); }
void rawlog(unsigned id,const Fixture& f) {
    logf_("CAP-RAW id=%u count=%u raw=%08x,%08x,%08x,%08x,%08x,%08x,%08x,%08x",id,f.scalar_count,
        f.raw[0],f.raw[1],f.raw[2],f.raw[3],f.raw[4],f.raw[5],f.raw[6],f.raw[7]);
}
void run() {
    // Restrict to default SSE masks, round-to-nearest, no DAZ/FTZ. Restore
    // caller flags as well: this fixture must not perturb ongoing simulation.
    const unsigned saved=_mm_getcsr(); _mm_setcsr(0x1f80);
    unsigned total=0,bad=0,skips=0,infzero=0;
    logf_("CAP-BEGIN method=1000992cc mxcsr_saved=%08x fixture_mxcsr=00001f80 original_curve_callbacks=1 production_curve_validated=0",saved);
    const uint32_t caps[]={0,1,3,0x00ffffff,0x01000001,0x7fffffff,0x80000000,0xffffffff};
    const uint32_t scales[]={0,0x80000000,0x3f000000,0x3f800000,0xbf800000,0x4f800000,0x5f000000,0xdf000000,0x7f800000,0xff800000,0x7fc12345};
    const uint32_t inputs[]={0,0x80000000,0x3f000000,0x3f7fffff,0x3f800000,0x40000000,0xbf800000,0x7fc12345,0xffc12345,0x7f800000,0xff800000,0xffc54321,0x3fc00000};
    auto compare=[&](Fixture& f,unsigned id) {
        init(f);
        if(f.curve_present) {
            // Preflight original setter only: it is safe even if all points
            // are skipped. Never let GetCurrentCapacity evaluate null backing.
            Curve probe{}; construct(nullptr,&probe); set(probe,&f.prop);
            const bool safe=probe.begin!=nullptr; destroy(nullptr,&probe); f.reset();
            if(!safe) { ++skips; logf_("CAP-SKIP id=%u null-backed evaluation excluded",id); rawlog(id,f); return; }
        }
        uint32_t a=0xdeadbeef,b=a;
        uint8_t ar=uint8_t(original<uint32_t(*)(void*,uint32_t,void*,uint32_t*)>(0x1000992cc)(f.sim,0x12345678,&f.occ,&a));
        char trace[128]; memcpy(trace,f.trace,sizeof(trace)); f.reset(); Curve c{};
        bool br=kernel::get_current_capacity({&f,&c},callbacks,0x12345678,&f.occ,b);
        bool mismatch=(ar!=uint8_t(br) || a!=b || strcmp(trace,f.trace)); ++total; bad+=mismatch;
        if(f.evaluated && (f.response&0x7fffffff)==0x7f800000 && f.capacity==0) ++infzero;
        logf_("CAP id=%u cap=%08x scale=%08x input=%08x power=%08x capok=%08x ageok=%08x present=%u%u%u orig=%02x/%08x kernel=%02x/%08x response=%08x evaluated=%u mismatch=%u",id,f.capacity,f.scale,f.input,f.powered,f.cap_ok,f.age_ok,f.cap_present,f.age_present,f.curve_present,ar,a,unsigned(br),b,f.response,f.evaluated,mismatch);
        logf_("CAP-TRACE id=%u original=%s kernel=%s",id,trace,f.trace); if(f.curve_present) rawlog(id,f);
    };
    unsigned id=0;
    for(auto cap:caps) for(auto scale:scales) for(unsigned p=0;p<2;++p) {
        Fixture f; f.capacity=cap; f.scale=scale; f.powered=p?0xab01:0xab00; compare(f,id++);
    }
    for(unsigned g=0;g<sizeof(geometries)/sizeof(*geometries);++g) for(auto x:inputs) for(auto cap:caps) {
        Fixture f; f.curve_present=true; geometry(f,g); f.input=x; f.capacity=cap; compare(f,id++);
    }
    for(auto scale:scales) for(unsigned p=0;p<2;++p) {
        Fixture f; f.curve_present=true; geometry(f,7); f.capacity=0;
        f.scale=scale; f.input=0x3f000000; f.powered=p?0xab01:0xab00; compare(f,id++);
    }
    for(unsigned mode=0;mode<8;++mode) {
        Fixture f; f.curve_present=true; geometry(f,0);
        if(mode==0) f.cap_present=false;
        if(mode==1) f.cap_ok=0xab00;
        if(mode==2) f.age_present=false;
        if(mode==3) f.age_ok=0xab00; // failed conversion still writes its output
        if(mode==4) f.powered=0xab00;
        if(mode==5) f.powered=0xab80;
        if(mode==6) f.cap_ok=0xab80;
        if(mode==7) f.scalar_count=0; // preflight skip, never original evaluate
        f.input=0x3f000000; compare(f,id++);
    }
    logf_("CAP-TOTAL method=1000992cc compared=%u mismatches=%u excluded=%u inf_response_zero_capacity=%u",total,bad,skips,infzero);
    // Independent original-only golden outputs for native production replay.
    // No parity score is claimed for these two dependency methods.
    unsigned golden=0;
    for(unsigned g=0;g<sizeof(geometries)/sizeof(*geometries);++g) {
        Fixture f; geometry(f,g); init(f); Curve c{}; construct(nullptr,&c);
        uint8_t ok=set(c,&f.prop); rawlog(g,f);
        logf_("CURVE-SET id=%u name=%s ret=%02x count=%d backed=%u trace=%s",g,geometries[g].name,ok,c.count,c.begin!=nullptr,f.trace);
        if(c.begin) for(auto x:inputs) { ++golden; logf_("CURVE-EVAL id=%u input=%08x output=%08x",g,x,bits(eval(nullptr,&c,fl(x)))); }
        // Raw derived records (5 binary32 words) aid independent setter replay.
        for(int r=0;r<c.count && r<4;++r) {
            uint32_t words[5]; memcpy(words,static_cast<unsigned char*>(c.begin)+20*r,20);
            logf_("CURVE-RECORD id=%u index=%d bits=%08x,%08x,%08x,%08x,%08x",g,r,words[0],words[1],words[2],words[3],words[4]);
        }
        // Retained-empty setter keeps the allocated first record reachable.
        f.scalar_count=0; f.reset(); ok=set(c,&f.prop);
        logf_("CURVE-EMPTY id=%u ret=%02x count=%d backed=%u trace=%s",g,ok,c.count,c.begin!=nullptr,f.trace);
        if(c.begin) { ++golden; logf_("CURVE-EMPTY-EVAL id=%u input=7fc12345 output=%08x",g,bits(eval(nullptr,&c,fl(0x7fc12345)))); }
        destroy(nullptr,&c);
    }
    logf_("CURVE-TOTAL setter=1000ac37a evaluator=1000ac4f6 golden_evaluations=%u mismatches=UNSCORED native_replay_required=1 fresh_null_and_all_skipped_null_faults=EXCLUDED",golden);
    _mm_setcsr(saved);
}
} // namespace capacity_oracle
static void pollution_capacity_prove_exec(void*) { capacity_oracle::run(); }
