#include "opensc4/loaded_aging_transition.h"
#include "opensc4/city.h"
#include "opensc4/pollution_aging_math.h"
#include "opensc4/pollution_capacity_kernel.h"
#include <optional>
namespace opensc4::loaded_aging_transition {
namespace {
float f(uint32_t b) { return std::bit_cast<float>(b); }
uint32_t b(float f) { return std::bit_cast<uint32_t>(f); }
bool supported(const Variant& v) { return v.array.empty() && (v.scalar.runtimeTag==6 || v.scalar.runtimeTag==7 || v.scalar.runtimeTag==10); }
std::vector<float> flat(const std::vector<uint32_t>& raw) {
    std::vector<float> result; for(auto v:raw) result.push_back(f(v)); return result;
}
bool validate(const Records& raw, cSC4ResponseCurve& curve) {
    std::vector<cSC4ResponseCurve::Point> points;
    for(const auto& r:raw) points.push_back({f(r[0]),f(r[1])});
    if(!curve.SetResponseCurveData(points) || curve.Segments().size()!=raw.size()) return false;
    for(size_t i=0;i<raw.size();++i) {
        const auto& s=curve.Segments()[i];
        if(std::array<uint32_t,5>{b(s.x),b(s.y),b(s.prevX),b(s.prevY),b(s.slope)}!=raw[i]) return false;
    }
    return true;
}
}
std::shared_ptr<const Property> State::Lookup(uint32_t key) const {
    // Local-first/default-second lookup; explicit alias references, no effective flattening.
    auto it=source.local.find(key);
    if(it!=source.local.end()) { auto o=objects.find(it->second); return o==objects.end()?nullptr:o->second; }
    it=source.defaults.find(key);
    if(it==source.defaults.end()) return {};
    auto o=objects.find(it->second); return o==objects.end()?nullptr:o->second;
}
#define READ(NAME,TYPE,READER) \
Status State::NAME(uint32_t key, TYPE& out) const { \
    auto p=Lookup(key); \
    if(!p) return (source.local.contains(key)||source.defaults.contains(key))?Status::Unsupported:Status::Missing; \
    if(!supported(*p->variant)) return Status::Unsupported; \
    return property_scalar::READER(p->variant->scalar,out)==property_scalar::ReadStatus::Value?Status::Ok:Status::Unsupported; \
}
READ(ReadFloat,uint32_t,ReadFloat32Bits)
READ(ReadSigned,int32_t,ReadSint32)
READ(ReadUnsigned,uint32_t,ReadUint32)
#undef READ
Status State::WriteFloat(uint32_t key,uint32_t bits) {
    auto p=Lookup(key);
    if(!p) return (source.local.contains(key)||source.defaults.contains(key))?Status::Unsupported:Status::Missing;
    if(!supported(*p->variant)) return Status::Unsupported;
    // Lookup-success SetPropertyValue @ 0x10004c256..0x10004c27b:
    // mutate the existing variant (including default hits); never clone/insert.
    p->variant->scalar={10,bits}; return Status::Ok;
}
Status Import(cSC4City& city,const Snapshot& input) {
    auto state=std::make_shared<State>(); state->source=input;
    std::map<uint32_t,std::shared_ptr<Variant>> variants;
    for(const auto& p:input.properties) {
        if(!p.identity || !p.variantIdentity || state->objects.contains(p.identity)) return Status::InvalidSnapshot;
        auto& v=variants[p.variantIdentity];
        if(v && (v->scalar.runtimeTag!=p.scalar.runtimeTag || v->scalar.bits!=p.scalar.bits || v->array!=p.array)) return Status::InvalidSnapshot;
        if(!v) v=std::make_shared<Variant>(Variant{p.scalar,p.array});
        state->objects[p.identity]=std::make_shared<Property>(Property{p.identity,v});
    }
    for(const auto* h:{&input.local,&input.defaults}) for(auto [key,id]:*h)
        if(id && !state->objects.contains(id)) return Status::InvalidSnapshot;
    // All five captured fields must survive existing SetResponseCurveData @ 0x1000ac220.
    if(!validate(input.fundingCurve,state->funding)||!validate(input.factorCurve,state->factor)) return Status::CurveMismatch;
    auto response=state->Lookup(0x8a1e07ee);
    if(!response || response->variant->scalar.runtimeTag!=0x800a || response->variant->array.size()!=14) return Status::Unsupported;
    cSC4ResponseCurve probe;
    if(!probe.SetResponseCurveData(flat(response->variant->array)) || probe.Empty()) return Status::CurveMismatch;
    city.LoadedAgingState()=std::move(state); return Status::Ok;
}
Report Execute(cSC4City& city) {
    Report r; auto s=city.LoadedAgingState();
    if(!s) { r.status=Status::NoState; return r; }
    auto fail=[&](Status status){r.status=status;return r;};
    // Adapter-only prevalidation: reject unsupported mutation/notification domains BEFORE writes.
    auto ageLocal=s->source.local.find(0xc910b4cf);
    if(ageLocal==s->source.local.end() || !ageLocal->second) return fail(Status::Unsupported);
    uint32_t scratch=0;
    for(auto id:{0x28ecf510u,0xc8ebd131u,0xc910b4cfu,0xea1cf220u,0xea1cf221u,0xea1cf222u}) {
        auto status=s->ReadFloat(id,scratch); if(status!=Status::Ok) return fail(status);
    }
    struct Context { State& s; Report& r; std::optional<cSC4ResponseCurve> curve; bool ok=true; } ctx{*s,r,{},true};
    using C=Context;
    pollution_capacity::Callbacks cb{
        [](void* c,void*)->void*{return &static_cast<C*>(c)->s;},
        [](void* c,void*,uint32_t id,uint32_t& out){return static_cast<C*>(c)->s.ReadUnsigned(id,out)==Status::Ok;},
        [](void* c,void*){auto& x=*static_cast<C*>(c);x.r.trace.push_back(Event::Power);
            const auto& lot=x.s.ImportedSnapshot();
            // IsOccupantPowered @ 0x10021485c; flags @ 0x100183b50/0x100183b3e.
            return lot.lotExists && ((lot.lotFlags&0x10)!=0 || (lot.lotFlags&8)==0);},
        [](void* c){auto& x=*static_cast<C*>(c);x.r.trace.push_back(Event::Scale);return f(x.s.ImportedSnapshot().scaleBits);},
        [](void* c,void*,uint32_t id)->void* {auto p=static_cast<C*>(c)->s.Lookup(id);return p?p->variant.get():nullptr;},
        [](void* c,void*){auto& x=*static_cast<C*>(c);x.r.trace.push_back(Event::Construct);x.curve.emplace();},
        [](void* c,void*,void* p){auto& x=*static_cast<C*>(c);x.r.trace.push_back(Event::Load);x.ok=x.curve->SetResponseCurveData(flat(static_cast<Variant*>(p)->array));},
        [](void* c,void*,uint32_t id,float& out){auto& x=*static_cast<C*>(c);x.r.trace.push_back(Event::CapacityInput);uint32_t bits=b(out);auto st=x.s.ReadFloat(id,bits);out=f(bits);return st==Status::Ok;},
        [](void* c,void*,float in){auto& x=*static_cast<C*>(c);x.r.trace.push_back(Event::Evaluate);float out=0;x.ok=x.curve->TryGetOutputValue(in,out)&&x.ok;return out;},
        [](void* c,void*) noexcept {auto& x=*static_cast<C*>(c);x.r.trace.push_back(Event::Destroy);x.curve.reset();}
    };
    // Success order from AgeOccupantList @ 0x100099d12..0x100099e4f.
    r.trace.reserve(32);
    r.trace.push_back(Event::FundingReference); const uint32_t* funding=&s->source.fundingBits;
    int32_t used=0; r.trace.push_back(Event::Used); s->ReadSigned(0x28ecf510,used);
    r.trace.push_back(Event::Capacity);
    if(!pollution_capacity::get_current_capacity({&ctx,nullptr},cb,0xc8ebd131,s.get(),r.capacity)||!ctx.ok) return fail(Status::Unsupported);
    const float factor=aging_math::factor(0x28ecf510,used,r.capacity,0,0,s->source.percentageZeroBits);
    r.factorBits=b(factor);
    uint32_t multiplier=0,lower=0x42c60000,upper=0x42c60000;
    r.trace.push_back(Event::Age);s->ReadFloat(0xc910b4cf,r.beforeBits);
    r.trace.push_back(Event::Multiplier);s->ReadFloat(0xea1cf220,multiplier);
    r.trace.push_back(Event::Lower);s->ReadFloat(0xea1cf221,lower);
    r.trace.push_back(Event::Upper);s->ReadFloat(0xea1cf222,upper);
    r.trace.push_back(Event::FundingRead);
    const float updated=aging_math::update_age(f(*funding),f(multiplier),factor,f(r.beforeBits),
        [&](float in){r.trace.push_back(Event::FundingCurve);float out=0;ctx.ok=s->funding.TryGetOutputValue(in,out)&&ctx.ok;r.fundingCurveBits=b(out);return out;},
        [&](float in){r.trace.push_back(Event::FactorCurve);float out=0;ctx.ok=s->factor.TryGetOutputValue(in,out)&&ctx.ok;r.factorCurveBits=b(out);return out;});
    r.afterBits=b(updated);
    if(!ctx.ok) return fail(Status::Unsupported);
    // Deliberate adapter preflight, not an original guard or original false return.
    if(aging_math::select_request(updated,f(lower),f(upper))!=aging_math::Request::none) return fail(Status::NotificationExcluded);
    r.status=s->WriteFloat(0xc910b4cf,r.afterBits);
    if(r.status!=Status::Ok) return r;
    r.trace.push_back(Event::Write);
    r.trace.push_back(Event::Threshold);
    (void)aging_math::select_request(updated,f(lower),f(upper));
    // Below-lower/unordered branch @ 0x100099e4f skips the entire notification boundary.
    // No callback is exposed: agingLotLookups/conditionCalls/posts remain zero.
    return r;
}
} // namespace opensc4::loaded_aging_transition
