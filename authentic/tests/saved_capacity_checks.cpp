// Synthetic adapter fixtures below are boundary checks, not live parity evidence.
#include "opensc4/saved_building_capacity.h"
#include <bit>
#include <cstdio>
#include <cfenv>
#include <stdexcept>
#include <string>
#include <utility>
using namespace opensc4;
namespace {
using Bytes = std::vector<Uint8>;
void U32(Bytes& b, Uint32 v) { for (unsigned i=0;i<4;++i) b.push_back(Uint8(v>>(8*i))); }
Bytes Bits(Uint32 v) { Bytes b; U32(b,v); return b; }
SerializedPropertyEntry Scalar(Uint32 key, Uint32 db, Uint32 bits) {
    SerializedPropertyEntry p; p.holderPropId=key; p.propertyId=key;
    p.dbType=db; p.rawValueBytes=Bits(bits); return p;
}
SerializedPropertyEntry Curve(std::vector<Uint32> bits) {
    SerializedPropertyEntry p; p.holderPropId=p.propertyId=0x8a1e07ee;
    p.dbType=0x8009; p.arrayCount=Uint32(bits.size());
    for(auto v:bits) U32(p.rawValueBytes,v); return p;
}
Bytes Exemplar(const std::vector<SerializedPropertyEntry>& props={}) {
    Bytes b{'E','Q','Z','B','1','#','#','#'}; b.resize(20); U32(b,Uint32(props.size()));
    for (const auto& p:props) {
        U32(b,p.holderPropId); b.push_back(0); U32(b,p.dbType);
        if(p.dbType&0x8000) U32(b,p.dbType==0x800c ? Uint32(p.rawValueBytes.size()):p.arrayCount);
        b.insert(b.end(),p.rawValueBytes.begin(),p.rawValueBytes.end());
    }
    return b;
}
struct Power { SavedBuildingLotStatus status=SavedBuildingLotStatus::Lot; bool powered=true; mutable unsigned calls=0; };
SavedBuildingLotResult QueryPower(const void* v) {
    const auto& p=*static_cast<const Power*>(v); ++p.calls;
    SavedBuildingLotResult r; r.status=p.status; r.savedPowered=p.powered; return r;
}
// Direct existing-method oracle: bypasses parser, lookup and numeric adapters.
// Supplies already decoded raw floats without implementing capacity or curves.
struct DirectHolder final : OccupantPropertyHolderSeam {
    bool hasInput=false; Uint32 input=0;
    bool GetPropertyValue(Uint32, Sint64&) const override { throw std::logic_error("unused Sint64"); }
    const CurvePropertyData* GetProperty(Uint32) const override { throw std::logic_error("unused legacy curve"); }
    bool GetPropertyValue(Uint32, Float32& out) const override {
        if (!hasInput) return false;
        out=std::bit_cast<Float32>(input); return true;
    }
};
struct DirectOccupant final : OccupantSeam {
    DirectHolder holder;
    const OccupantPropertyHolderSeam* GetPropertyHolder() const override { return &holder; }
};
struct DirectAccess final : PollutionCapacityAccessSeam, RawCurvePropertySeam, RawCurveValueSeam {
    std::vector<Float32> raw;
    explicit DirectAccess(const std::vector<Uint32>& bits) {
        for(auto b:bits) raw.push_back(std::bit_cast<Float32>(b));
    }
    bool ReadCapacity(const OccupantPropertyHolderSeam&,Uint32,Uint32& out) const override { out=20; return true; }
    bool IsOccupantPowered(const OccupantSeam&) const override { return true; }
    const RawCurvePropertySeam* LookupCurve(const OccupantPropertyHolderSeam&,Uint32) const override { return this; }
    const RawCurveValueSeam* GetValue() const override { return this; }
    const Float32* GetScalarData() const override { return raw.data(); }
    Uint32 GetScalarCount() const override { return Uint32(raw.size()); }
};
SavedCapacityResult Direct(const std::vector<Uint32>& bits, bool hasInput, Uint32 input) {
    PollutionSimulatorSaveRecord record; record.field348=1;
    DirectAccess access(bits); record.capacityAccess=&access;
    DirectOccupant occupant; occupant.holder.hasInput=hasInput; occupant.holder.input=input;
    Uint32 out=0;
    try { const bool ok=record.GetCurrentCapacity(0x27812852,occupant,out);
        return {ok?SavedCapacityStatus::Value:SavedCapacityStatus::MissingCapacity,out};
    } catch(const ResponseCurveStorageFault&) { return {SavedCapacityStatus::CurveStorageFault,0}; }
}
}
// Reusable with the conductor's Check shim: no main required unless opted in.
// check(bool, const char*) must consume the label synchronously.
void RunSavedCapacityChecks(void (*check)(bool,const char*), bool mountedFixture) {
    using S=SavedCapacityStatus;
    BuildingOccupantSaveRecord b;
    PollutionSimulatorSaveRecord pollution; pollution.field348=1;
    Power power;
    auto& props=b.base.exemplarHolder.properties.properties;
    const Uint32 cap=0x27812852;
    auto eval=[&](Bytes bytes=Exemplar(), Uint32 id=0x27812852) {
        return EvaluateSavedBuildingCapacitySnapshot(b,pollution,bytes,id,QueryPower,&power);
    };
    auto expect=[&](SavedCapacityResult r,S status,Uint32 value,const char* label) {
        check(r.status==status && ((status!=S::Value && status!=S::MissingCapacity) || r.value==value),label);
    };
    expect(eval(),S::MissingCapacity,0,"missing capacity");
    check(power.calls==0,"missing capacity skips power");
    props={Scalar(cap,3,123)};
    props[0].propertyId=0x12345678;
    expect(eval(Exemplar({Scalar(cap,3,456)})),S::Value,123,"local key wins despite distinct embedded ID");
    expect(eval(Exemplar(),0x12345678),S::MissingCapacity,0,"embedded ID is not holder key");
    props.clear();
    expect(eval(Exemplar({Scalar(cap,3,456)})),S::Value,456,"default lookup");
    for(Uint32 key:{0u,0x7fffffffu,0x80000000u,0xffffffffu}) {
        props={Scalar(0xffffffffu,3,41),Scalar(0,3,42),Scalar(0x80000000,3,43),Scalar(0x7fffffff,3,44)};
        expect(eval(Exemplar(),key),S::Value,key==0?42:key==0x7fffffff?44:key==0x80000000?43:41,"unsigned saved lower bound");
        const auto defaults=Exemplar(props); props.clear();
        expect(eval(defaults,key),S::Value,key==0?42:key==0x7fffffff?44:key==0x80000000?43:41,"unsigned default lower bound");
    }
    // All three scalar source conversions through capacity's Uint32 adapter.
    for(auto pair:std::vector<std::pair<Uint32,Uint32>>{{3,17},{7,17},{9,0x418c0000}}) {
        props={Scalar(cap,pair.first,pair.second)};
        expect(eval(),S::Value,17,"Uint32 adapter three source types");
    }
    for(Uint32 bits:{0x7fc12345u,0xff800000u,0x80000000u}) {
        props={Scalar(cap,9,bits)};
        expect(eval(),S::Value,0,"float invalid/signed zero to Uint32 low word");
    }
    props={Scalar(cap,3,20)};
    power.powered=false; expect(eval(),S::Value,0,"saved power false");
    power.status=SavedBuildingLotStatus::NoLot; power.powered=true;
    expect(eval(),S::Value,0,"NoLot is false");
    power.status=SavedBuildingLotStatus::Unavailable; expect(eval(),S::Unavailable,0,"power unavailable distinct");
    power.status=SavedBuildingLotStatus::Unsupported; expect(eval(),S::Unsupported,0,"power unsupported distinct");
    power.status=SavedBuildingLotStatus::Lot;
    expect(EvaluateSavedBuildingCapacitySnapshot(b,pollution,Exemplar(),cap,nullptr,nullptr),S::Unavailable,0,"missing power provider");
    props={Scalar(cap,8,20)}; props[0].rawValueBytes.resize(8);
    expect(eval(),S::Unsupported,0,"DB08 not missing");
    expect(eval(Exemplar(),99),S::MissingCapacity,0,"unqueried DB08 retained");
    props={Scalar(cap,0x8003,20)}; props[0].arrayCount=1;
    expect(eval(),S::Unsupported,0,"one-element scalar array not flattened");
    props={Scalar(cap,3,20)}; props[0].variantTag=0xa9bee9ac;
    expect(eval(),S::Unsupported,0,"reference not missing");
    props={Scalar(cap,3,20),Scalar(cap,3,30)};
    expect(eval(),S::Unsupported,0,"duplicate saved keys");
    props.clear();
    expect(eval(Exemplar({Scalar(cap,3,20),Scalar(cap,3,30)})),S::Unsupported,0,"duplicate default keys");
    Bytes invalid=Exemplar(); invalid[8]=1;
    expect(eval(invalid),S::Unsupported,0,"non-null parent");
    invalid=Exemplar({Scalar(cap,3,20)}); invalid[28]=1;
    expect(eval(invalid),S::Unsupported,0,"default reference flag");
    invalid=Exemplar(); invalid.push_back(0);
    expect(eval(invalid),S::Unsupported,0,"trailing data");
    invalid=Exemplar(); invalid[0]='C';
    expect(eval(invalid),S::Unsupported,0,"cohort unsupported");
    invalid=Exemplar(); invalid[3]='T';
    expect(eval(invalid),S::Unsupported,0,"text unsupported");
    const auto valid=Exemplar({Scalar(cap,3,20)});
    for(std::size_t n=0;n<valid.size();++n)
        expect(eval(Bytes(valid.begin(),valid.begin()+n)),S::Unsupported,0,"every truncation prefix");
    invalid=Exemplar(); for(unsigned i=20;i<24;++i) invalid[i]=255;
    expect(eval(invalid),S::Unsupported,0,"count bounded before allocation");
    auto huge=Curve({}); huge.arrayCount=0xffffffff;
    expect(eval(Exemplar({huge})),S::Unsupported,0,"array count bounded before allocation");
    props={Scalar(cap,3,20)};
    auto unknown=Scalar(1,0x1234,0);
    expect(eval(Exemplar({unknown})),S::Unsupported,0,"unskippable DB type");
    auto string=Scalar(1,0x800c,0); string.rawValueBytes={'x',0,'y'};
    expect(eval(Exemplar({string})),S::Value,20,"unqueried string retained");
    expect(eval(Exemplar({string}),1),S::Unsupported,0,"queried string unsupported");
    for(Uint32 count:{0u,1u,3u,16u,128u,129u}) {
        std::vector<Uint32> raw(count);
        for(Uint32 i=0;i<count;++i) raw[i]=(i%2)?0x3f800000:std::bit_cast<Uint32>(float(i/2));
        if(count%2) raw.back()=0x7f812345; // discarded only by original pair loader
        expect(eval(Exemplar({Curve(raw)})),count<2?S::CurveStorageFault:S::Value,20,"default raw curve count boundary");
        props={Scalar(cap,3,20),Curve(raw)};
        expect(eval(),count<2?S::CurveStorageFault:S::Value,20,"saved raw curve count boundary");
        props={Scalar(cap,3,20)};
    }
    // Three Float32 source types reach holder getter via curve input. An input
    // of one selects Y=.5 in this fixture; missing input remains initial zero.
    const auto curve=Curve({0,0x3f800000,0x3f800000,0x3f000000,0x40000000,0});
    for(auto pair:std::vector<std::pair<Uint32,Uint32>>{{3,1},{7,1},{9,0x3f800000}}) {
        props={Scalar(cap,3,20),Scalar(0xc910b4cf,pair.first,pair.second)};
        expect(eval(Exemplar({curve})),S::Value,10,"Float32 adapter three source types");
    }
    props={Scalar(cap,3,20)};
    expect(eval(Exemplar({curve})),S::Value,20,"missing float leaves initial zero input");
    for(Uint32 bits:{0u,0x80000000u,0x7fc12345u,0x7f812345u}) {
        props={Scalar(cap,3,20),Scalar(0xc910b4cf,9,bits)};
        const auto expected=Direct({0,0x3f800000,0x3f800000,0x3f000000,0x40000000,0},true,bits);
        expect(eval(Exemplar({curve})),expected.status,expected.value,"raw Float32 input matches direct method");
    }
    for(Uint32 bits:{0u,0x80000000u,0x7fc12345u,0xffc54321u,0x7f812345u}) {
        props={Scalar(cap,3,20)};
        const auto expected=Direct({0,bits},false,0);
        expect(eval(Exemplar({Curve({0,bits})})),expected.status,expected.value,
            "raw curve signed zero and NaN payload matches direct method");
    }
    props={Scalar(cap,3,20),Scalar(0xc910b4cf,8,1)};
    expect(eval(Exemplar({curve})),S::Unsupported,0,"unsupported curve input preserved");
    expect(eval(),S::Value,20,"absent curve skips unsupported input");
    check(pollution.capacityAccess==nullptr,"persistent capacityAccess unchanged");
    DirectAccess persistent({0,0x3f800000});
    pollution.capacityAccess=&persistent;
    expect(eval(),S::Value,20,"local access overrides persistent seam");
    check(pollution.capacityAccess==&persistent,"nonnull persistent seam unchanged");
    pollution.capacityAccess=nullptr;
    cSC4City emptyCity; cDBPFSystem emptyAssets;
    expect(QuerySavedBuildingCapacity(emptyCity,{0x138a8240,kCLSID_cSC4BuildingOccupant},cap,emptyAssets),S::Unavailable,0,"undecoded city unavailable");
    if(mountedFixture) {
        cDBPFSystem assets; const auto app=cDBPFSystem::DefaultSteamSC4AppPath();
        check(assets.MountInstall(app),"mount original read-only data");
        cSC4City city;
        check(city.LoadFromSC4File(app+"/Contents/GameData/Regions/Timbuktu/City - Rush Hour Tutorial.sc4",&assets),"load actual Rush Hour fixture");
        const SimSerializableRef ref{0x138a8240,kCLSID_cSC4BuildingOccupant};
        const auto actual=QuerySavedBuildingCapacity(city,ref,cap,assets);
        std::printf("Rush Hour saved query status=%d value=%u\n",int(actual.status),actual.value);
        check(actual.status==S::Value,"actual public query accepts saved DB08 and raw default curve");
        const auto* building=city.FindSavedBuildingOccupant(ref); Bytes bytes;
        check(building && assets.LoadAssetByTGI(building->base.exemplarHolder.exemplarKey,bytes),"exact TGI fixture");
        struct C { const cSC4City* city; SimSerializableRef ref; } ctx{&city,ref};
        const auto direct=EvaluateSavedBuildingCapacitySnapshot(*building,city.PollutionSaveRecord(),bytes,cap,
            [](const void* p){ const auto& c=*static_cast<const C*>(p); return c.city->QuerySavedBuildingLot(c.ref); },&ctx);
        check(actual.status==direct.status && actual.value==direct.value,"wrapper matches direct saved snapshot");
        expect(QuerySavedBuildingCapacity(city,ref,cap,emptyAssets),S::Unavailable,0,"exact TGI absent unavailable");
        check(city.PollutionSaveRecord().capacityAccess==nullptr,"city persistent capacityAccess unchanged");
    }
}
#ifdef OPENSC4_SAVED_CAPACITY_CHECKS_MAIN
namespace { unsigned checks=0, failures=0;
void Check(bool ok,const char* label) { ++checks; if(!ok){ ++failures; std::fprintf(stderr,"FAIL: %s\n",label); } }
}
int main() { std::fesetround(FE_TONEAREST); RunSavedCapacityChecks(Check,true); std::printf("saved capacity: %u checks, %u failures\n",checks,failures); return failures?1:0; }
#endif
