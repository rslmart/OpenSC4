#include "opensc4/loaded_aging_transition.h"
#include "opensc4/city.h"
#include "opensc4/pollution_aging_math.h"
#include "../recon/loaded_aging_transition/local_age_adapter_fixture.h"
#include <algorithm>
#include <cstdio>
#include <cfenv>
#if defined(__x86_64__)
#include <xmmintrin.h>
#endif
namespace opensc4::loaded_aging_transition::checks {
struct CheckContext {
    unsigned passed=0,failed=0;
    void (*report)(bool,const char*)=nullptr;
    void Check(bool value,const char* name) {
        if(value) ++passed;else ++failed;
        if(report) report(value,name);else if(!value) std::fprintf(stderr,"FAIL %s\n",name);
    }
};
void RunChecks(CheckContext& c) {
    fenv_t env; feholdexcept(&env); fesetround(FE_TONEAREST);
#if defined(__x86_64__)
    const auto csr=_mm_getcsr(); _mm_setcsr(0x1f80);
#elif defined(__aarch64__)
    uint64_t fpcr; asm volatile("mrs %0, fpcr":"=r"(fpcr)); uint64_t zero=0;asm volatile("msr fpcr, %0"::"r"(zero));
#endif
    auto fixture=LocalAgeFixture();
    cSC4City city,second;
    c.Check(Import(city,fixture)==Status::Ok,"actual import coefficients all five fields");
    c.Check(Import(second,fixture)==Status::Ok,"independent import");
    auto state=city.LoadedAgingState(); auto age=state->Lookup(0xc910b4cf); auto variant=age->variant;
    std::vector<std::pair<std::shared_ptr<const Property>,Variant>> unchanged;
    for(auto p:fixture.properties) if(p.identity!=age->identity) {
        for(auto h:{fixture.local,fixture.defaults}) for(auto [key,id]:h) if(id==p.identity) {
            auto object=state->Lookup(key);unchanged.push_back({object,*object->variant});
        }
    }
    auto r=Execute(city);
    c.Check(r.status==Status::Ok && r.beforeBits==0 && r.afterBits==0x38e8fe5b,"offline predicted age, not live observation");
    c.Check(r.capacity==0 && r.factorBits==0 && r.fundingCurveBits==0x3f800000 && r.factorCurveBits==0x3dcccccd,"zero capacity still ages through actual kernels");
    c.Check(state->Lookup(0xc910b4cf)==age && age->variant==variant && variant->scalar.bits==r.afterBits,"stable property and variant");
    c.Check(city.LoadedAgingState()==state && second.LoadedAgingState()->Lookup(0xc910b4cf)->variant->scalar.bits==0,"retained and city independent");
    bool same=true;for(auto& [p,v]:unchanged) same &= p->variant->scalar.bits==v.scalar.bits && p->variant->scalar.runtimeTag==v.scalar.runtimeTag && p->variant->array==v.array;
    c.Check(same && state->ImportedSnapshot().lotFlags==fixture.lotFlags && state->ImportedSnapshot().lotRectangle==fixture.lotRectangle && state->ImportedSnapshot().lotState==fixture.lotState && state->ImportedSnapshot().lotCondition==fixture.lotCondition,"unrelated/default/lot unchanged");
    const std::vector<Event> expected={Event::FundingReference,Event::Used,Event::Capacity,Event::Power,Event::Scale,Event::Construct,Event::Load,Event::CapacityInput,Event::Evaluate,Event::Destroy,Event::Age,Event::Multiplier,Event::Lower,Event::Upper,Event::FundingRead,Event::FundingCurve,Event::FactorCurve,Event::Write,Event::Threshold};
    c.Check(r.trace==expected && !r.agingLotLookups && !r.conditionCalls && !r.posts,"ordered trace and skipped notification boundary");
    r=Execute(city); c.Check(r.beforeBits==0x38e8fe5b && r.afterBits==0x3968fe5b,"retained second offline execution");
    uint32_t out=0; c.Check(state->ReadUnsigned(0xc8ebd131,out)==Status::Ok && out==0,"valid zero");
    out=123;c.Check(state->ReadFloat(0xdeadbeef,out)==Status::Missing && out==123,"missing preserves initialized output");
    c.Check(state->ReadFloat(0x08d9763d,out)==Status::Unsupported && out==123,"unmodeled is unsupported");
    c.Check(state->WriteFloat(0xdeadbeef,0)==Status::Missing && state->WriteFloat(0x8a1e07ee,0)==Status::Unsupported && age->variant->scalar.bits==r.afterBits,"excluded writes no mutation");
    auto bad=fixture; bad.fundingCurve[1][4]^=1;
    c.Check(Import(city,bad)==Status::CurveMismatch && city.LoadedAgingState()==state,"coefficient mismatch no publication");
    // Default-hit writes preserve shared aliases; local shadow must win.
    auto aliases=fixture; uint32_t mult=aliases.defaults.at(0xea1cf220);
    aliases.defaults[0x1234]=mult; aliases.local[0x1235]=aliases.local.at(0xc910b4cf);aliases.defaults[0x1235]=mult;
    c.Check(Import(city,aliases)==Status::Ok,"alias fixture import");state=city.LoadedAgingState();auto def=state->Lookup(0xea1cf220);
    c.Check(state->WriteFloat(0xea1cf220,0x3f000000)==Status::Ok && state->Lookup(0x1234)==def && def->variant->scalar.bits==0x3f000000,"default shared alias mutation");
    c.Check(state->WriteFloat(0x1235,0x3e800000)==Status::Ok && state->Lookup(0xc910b4cf)->variant->scalar.bits==0x3e800000 && def->variant->scalar.bits==0x3f000000,"local shadow wins");
    auto unsupported=fixture;
    for(auto& p:unsupported.properties) if(p.identity==unsupported.local.at(0xc910b4cf))p.scalar.runtimeTag=0x123;
    Import(city,unsupported); state=city.LoadedAgingState();out=789;
    c.Check(state->ReadFloat(0xc910b4cf,out)==Status::Unsupported && out==789 && Execute(city).status==Status::Unsupported && state->WriteFloat(0xc910b4cf,1)==Status::Unsupported,"unsupported tag rejected without mutation");
    auto missing=fixture;missing.local.erase(0x28ecf510);Import(city,missing);
    c.Check(Execute(city).status==Status::Missing && city.LoadedAgingState()->Lookup(0xc910b4cf)->variant->scalar.bits==0,"execution does not relabel missing as valid zero");
    auto shared=fixture;shared.properties.push_back({99,shared.properties[2].variantIdentity,shared.properties[2].scalar,{}});shared.defaults[0x112233]=99;
    Import(city,shared);state=city.LoadedAgingState();
    c.Check(state->WriteFloat(0xc910b4cf,0x3f000000)==Status::Ok && state->Lookup(0x112233)->variant==state->Lookup(0xc910b4cf)->variant && state->Lookup(0x112233)->variant->scalar.bits==0x3f000000,"distinct properties sharing variant retain alias");
    // Force exact lower/upper, NaN upper, and unordered updated through production execution.
    for(auto threshold: {0x38e8fe5bu,0u,0x7fc00001u}) {
        auto x=fixture;
        for(auto& p:x.properties) if(p.identity==x.defaults.at(0xea1cf221)) p.scalar.bits=threshold;
        Import(city,x);auto report=Execute(city);
        const bool excluded=threshold!=0x7fc00001u;
        c.Check(report.status==(excluded?Status::NotificationExcluded:Status::Ok),"lower exact/over/unordered selection");
        c.Check(city.LoadedAgingState()->Lookup(0xc910b4cf)->variant->scalar.bits==(excluded?0u:0x38e8fe5bu),"preflight mutation boundary");
    }
    Import(city,fixture);state=city.LoadedAgingState();state->WriteFloat(0xea1cf222,0x38e8fe5b);
    c.Check(Execute(city).status==Status::NotificationExcluded,"exact upper rejects before write");
    state->WriteFloat(0xea1cf222,0x7fc00001);state->WriteFloat(0xea1cf221,0);
    c.Check(Execute(city).status==Status::NotificationExcluded,"NaN upper does not skip satisfied lower");
    state->WriteFloat(0xc910b4cf,0x7fc01234);
    c.Check(Execute(city).status==Status::Ok && state->Lookup(0xc910b4cf)->variant->scalar.bits==0x7fc01234,"unordered age skips thresholds preserving payload");
    // Nonzero dependency perturbation defeats observed-zero shortcut; needs-power bit toggles capacity.
    auto powered=fixture;for(auto& p:powered.properties) if(p.identity==powered.defaults.at(0xc8ebd131))p.scalar.bits=100;
    Import(city,powered);c.Check(Execute(city).capacity==100,"actual capacity input consumed");
    auto scaled=powered;scaled.scaleBits=0x40000000;Import(city,scaled);c.Check(Execute(city).capacity==200,"captured scale is consumed");
    auto response=powered;for(auto& p:response.properties)if(p.identity==response.defaults.at(0x8a1e07ee))p.array[1]=0x3f000000;
    Import(city,response);c.Check(Execute(city).capacity==50,"owned capacity response curve evaluated");
    auto utilized=powered;for(auto& p:utilized.properties)if(p.identity==utilized.local.at(0x28ecf510))p.scalar.bits=50;
    Import(city,utilized);c.Check(Execute(city).factorBits==0x3f000000,"used getter and nonzero factor consumed");
    powered.lotFlags=8;Import(city,powered);c.Check(Execute(city).capacity==0,"needs power without power");
    powered.lotFlags=0;Import(city,powered);c.Check(Execute(city).capacity==100,"no power needed");
    powered.lotExists=false;Import(city,powered);c.Check(Execute(city).capacity==0,"missing lot is unpowered");
    // Operand order is observable with NaNs in factor curve versus multiplier.
    const float a=std::bit_cast<float>(0x7fc01234u),z=std::bit_cast<float>(0x7fc05678u);
    auto result=aging_math::update_age(1,z,0,0,[](float){return 1.f;},[&](float){return a;});
    c.Check(std::bit_cast<uint32_t>(result)==0x7fc01234,"factor result is MUL destination");
    // A captured runtime adapter belongs to one city lifecycle, even on failed load.
    Import(city,fixture);
    c.Check(!city.LoadFromSC4File("/opensc4-missing-local-age-regression.sc4") && !city.LoadedAgingState(),"failed file load discards old imported aging state");
    Import(city,fixture);cDBPFArchive unopened;
    c.Check(!city.LoadFromSC4Archive(unopened) && !city.LoadedAgingState(),"archive load attempt discards old imported aging state");
    Import(city,fixture);city.GenerateProceduralCity(32,32,123);
    c.Check(!city.LoadedAgingState(),"new procedural city discards old imported aging state");
#if defined(__x86_64__)
    _mm_setcsr(csr);
#elif defined(__aarch64__)
    asm volatile("msr fpcr, %0"::"r"(fpcr));
#endif
    fesetenv(&env);
}
}
#ifdef OPENSC4_LOCAL_AGE_STANDALONE
int main(){opensc4::loaded_aging_transition::checks::CheckContext c;RunChecks(c);std::printf("loaded aging: %u passed, %u failed\n",c.passed,c.failed);return c.failed?1:0;}
#endif

void RunLoadedAgingTransitionChecks(void (*check)(bool,const char*)) {
    opensc4::loaded_aging_transition::checks::CheckContext context;
    context.report=check;
    opensc4::loaded_aging_transition::checks::RunChecks(context);
}
