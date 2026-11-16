// Pure host checks against the actual include stack. Never calls native getters.
#include "saved_fixture_binding_syntax_stub.cpp"
#include <cassert>
static void logf_(const char*,...) {}
static uint64_t get_app() { std::abort(); }
ScopedRZString::ScopedRZString() { std::abort(); }
ScopedRZString::~ScopedRZString() { std::abort(); }
namespace fb=fixture_placement_bindings;
static unsigned checks=0;
static void check(bool v) { assert(v); ++checks; }
static fb::InventoryObservation fixed() {
    using namespace fb;
    InventoryObservation o;
    o.identity_complete=true; o.owner_inspected=true; o.owner_spent=true; o.pid=15730;
    o.kind=FixtureKind::SavedInactive;
    o.initial.app=0xf123456700000001; o.initial.city=0xf123456700000002;
    o.initial.sim=0xf123456700000003; o.initial.view=0xf123456700000004;
    o.initial.renderer=0xf123456700000005; o.initial.date=2455319; o.initial.pauses[0]=2;
    o.owner_receipt=o.initial; o.city=o.initial.city; o.regional=0xf123456700000006;
    o.guarded_descriptor=0xf123456700000007;
    o.region_path=region; o.path=saved_path; o.manifest_path=saved_manifest; o.manifest_sha256=saved_manifest_hash;
    o.list_counts[0]=1;
    o.occurrences.emplace_back(); auto& v=o.occurrences.back();
    v.offset=0xc0; v.node=0xf123456700000008;
    auto& p=v.value;
    p.complete=true; p.occupant=0xf123456700000108; p.base=p.occupant-8;
    p.holder=p.occupant+0x10; p.exemplar=0xf123456700000009;
    p.tgi[0]=0x6534284a; p.tgi[1]=0xc8dbccba; p.tgi[2]=0x1f4b0000;
    p.lot.complete=true; p.lot.lot=0xf12345670000000a; p.lot.descriptor=o.guarded_descriptor;
    p.lot.rect={58,66,63,72}; p.lot.state=3;
    const uint32_t ids[]={0xc8ebd131,0xa8ebcf0c,0x88ed1059,0x27812852,0xc910b4cf,
        0x8a1e07ee,0x28ecf510,0xea1cf220,0xea1cf221,0xea1cf222};
    const uint32_t types[]={6,0,0,0,0xa,0x800a,7,0xa,0xa,0xa};
    const uint32_t scalars[]={0,0,0,0,0,0,0,0x3a919ef9,0x3f800000,0x3f8ccccd};
    const uint32_t curve[]={0x00000000,0x3f800000,0x3f000000,0x3f733333,0x3f266666,
        0x3f666666,0x3f400000,0x3f4ccccd,0x3f666666,0x3f0ccccd,0x3f800000,0x3e99999a,0x3f8ccccd,0x00000000};
    for(unsigned i=0;i<10;++i) {
        auto& r=p.properties[i]; r.id=ids[i]; r.type=types[i]; r.complete=true;
        r.status=i>=1 && i<=3?PropertyStatus::Missing:PropertyStatus::Supported;
        if(r.status==PropertyStatus::Supported) { r.word_count=1; r.words[0]=scalars[i]; }
    }
    p.properties[5].count=14; p.properties[5].word_count=14;
    std::memcpy(p.properties[5].words,curve,sizeof curve);
    finish_inventory_observation(o,o.initial,true);
    return o;
}
template<class F> static void rejects(F f) {
    auto o=fixed(); f(o); fb::finish_inventory_observation(o,o.final,o.complete);
    check(!fb::placement_candidate_matches(o));
}
int main() {
    using namespace fb;
    auto o=fixed(); check(placement_candidate_matches(o));
    check(o.matching_c0_count==1 && o.selected_index==0 && o.selected_node==0xf123456700000008 &&
          o.selected_occupant==0xf123456700000108 && o.occurrences[0].value.holder==0xf123456700000118 &&
          o.occurrences[0].value.base==0xf123456700000100 && o.guarded_descriptor==0xf123456700000007);
    rejects([](auto& x){x.occurrences.clear();x.list_counts[0]=0;});
    rejects([](auto& x){x.occurrences[0].value.lot.lot=0;});
    rejects([](auto& x){x.occurrences[0].value.lot.complete=false;});
    rejects([](auto& x){x.occurrences[0].value.complete=false;});
    rejects([](auto& x){x.occurrences[0].value.occupant=0;});
    rejects([](auto& x){x.occurrences[0].value.base=0;});
    rejects([](auto& x){x.occurrences[0].value.holder=0;});
    rejects([](auto& x){x.occurrences[0].value.exemplar=0;});
    rejects([](auto& x){x.guarded_descriptor^=0x100000000ull;});
    rejects([](auto& x){x.identity_complete=false;});
    rejects([](auto& x){x.owner_inspected=false;});
    rejects([](auto& x){x.pid=0;});
    rejects([](auto& x){x.city++;}); rejects([](auto& x){x.regional=0;});
    rejects([](auto& x){x.owner_receipt.sim++;});
    for(auto kind:{FixtureKind::Unbound,FixtureKind::OriginalPrepared}) rejects([&](auto& x){x.kind=kind;});
    rejects([](auto& x){x.region_path+="/wrong";}); rejects([](auto& x){x.path+="wrong";});
    rejects([](auto& x){x.manifest_path+="wrong";}); rejects([](auto& x){x.manifest_sha256[0]^=1;});
    rejects([](auto& x){x.guarded_descriptor=0;}); rejects([](auto& x){x.guarded_descriptor++;});
    rejects([](auto& x){x.occurrences[0].value.lot.descriptor++;});
    for(unsigned i=0;i<3;++i) rejects([&](auto& x){x.occurrences[0].value.tgi[i]^=1;});
    for(unsigned i=0;i<4;++i) rejects([&](auto& x){
        auto& r=x.occurrences[0].value.lot.rect;
        if(i==0) ++r.l; if(i==1) ++r.t; if(i==2) ++r.r; if(i==3) ++r.b;
    });
    rejects([](auto& x){x.occurrences[0].value.lot.state++;});
    rejects([](auto& x){x.occurrences[0].value.lot.condition++;});
    for(unsigned i=0;i<10;++i) {
        rejects([&](auto& x){x.occurrences[0].value.properties[i].complete=false;});
        rejects([&](auto& x){x.occurrences[0].value.properties[i].id^=1;});
        for(auto status:{PropertyStatus::Unreadable,PropertyStatus::Unsupported})
            rejects([&](auto& x){x.occurrences[0].value.properties[i].status=status;});
        if(i>=1 && i<=3) {
            rejects([&](auto& x){x.occurrences[0].value.properties[i].status=PropertyStatus::Supported;});
        } else {
            rejects([&](auto& x){x.occurrences[0].value.properties[i].status=PropertyStatus::Missing;});
            rejects([&](auto& x){x.occurrences[0].value.properties[i].count++;});
            rejects([&](auto& x){x.occurrences[0].value.properties[i].type^=1;});
            rejects([&](auto& x){x.occurrences[0].value.properties[i].word_count--;});
            rejects([&](auto& x){x.occurrences[0].value.properties[i].words[0]^=1;});
        }
    }
    for(unsigned i=0;i<14;++i) rejects([&](auto& x){x.occurrences[0].value.properties[5].words[i]^=1;});
    for(uint32_t bits:{0x80000000u,0x7fc12345u,0xffffffffu})
        rejects([&](auto& x){x.occurrences[0].value.properties[0].words[0]=bits;});
    rejects([](auto& x){x.occurrences.push_back(x.occurrences[0]); x.occurrences.back().index=1;
        x.occurrences.back().node++; x.list_counts[0]=2;});
    o=fixed(); o.occurrences.push_back(o.occurrences[0]); o.occurrences.back().index=1;
    o.occurrences.back().node++; o.occurrences.back().value.tgi[2]++; o.list_counts[0]=2;
    finish_inventory_observation(o,o.final,true);
    check(placement_candidate_matches(o) && o.matching_c0_count==1 && o.occurrences.size()==2);
    // A matching occurrence need not be the first C0 element.
    std::swap(o.occurrences[0].value,o.occurrences[1].value);
    finish_inventory_observation(o,o.final,true);
    check(placement_candidate_matches(o) && o.selected_index==1 && o.selected_occurrence==1);
    // Matching non-C0 data is preserved and does not add a C0 match.
    o.occurrences.push_back(o.occurrences[0]); o.occurrences.back().offset=0xd8; o.list_counts[1]=1;
    finish_inventory_observation(o,o.final,true); check(placement_candidate_matches(o));
    rejects([](auto& x){x.final.date++;}); rejects([](auto& x){x.final.pauses[2]++;});
    rejects([](auto& x){x.final.renderer++;}); rejects([](auto& x){x.complete=false;});
    rejects([](auto& x){x.list_counts[0]++;});
    // Actual finalization helper cannot validate a failed final sample.
    o=fixed(); finish_inventory_observation(o,o.final,false);
    check(!o.complete && !placement_candidate_matches(o));
    // Host ownership: copies retain full raw/sign/NaN words after source changes.
    o=fixed(); o.occurrences[0].value.position_bounds[0]=0x7fc12345;
    o.occurrences[0].value.stored_position[1]=0x80000000;
    auto copy=o; o={};
    check(copy.occurrences[0].value.position_bounds[0]==0x7fc12345 &&
          copy.occurrences[0].value.stored_position[1]==0x80000000 && placement_candidate_matches(copy));
    // Actual acquisition copy helper: full bounded payload, no float conversion.
    uint32_t raw[256]; for(unsigned i=0;i<256;++i) raw[i]=0x80000000u+i;
    raw[17]=0x7fc12345; RawPropertyObservation payload;
    payload.id=0x8a1e07ee; payload.type=0x800a; payload.count=256;
    check(copy_property_words(payload,raw,256));
    check(payload.complete && payload.word_count==256 && payload.type==0x800a && payload.count==256);
    for(unsigned i=0;i<256;++i) check(payload.words[i]==raw[i]);
    raw[17]=0; check(payload.words[17]==0x7fc12345);
    check(!copy_property_words(payload,raw,257) && !payload.complete && payload.word_count==0 &&
          payload.status==PropertyStatus::Unreadable);
    check(!copy_property_words(payload,nullptr,1) && !payload.complete);
    check(copy_property_words(payload,nullptr,0) && payload.complete && payload.word_count==0);
    Owner owner; owner.kind=FixtureKind::SavedInactive; owner.city=0xf123456700000002;
    owner.regional=0xf123456700000006; owner.inspected=true; owner.spent=true;
    std::strcpy(owner.path,saved_path); owner.receipt=fixed().initial;
    o={}; observe_owner(o,owner);
    check(o.identity_complete && o.path==saved_path && o.manifest_sha256==saved_manifest_hash &&
          o.city==0xf123456700000002 && o.regional==0xf123456700000006 && o.owner_spent);
    owner.path[0]='X'; check(o.path==saved_path); // no borrowed string storage
    std::memset(owner.path,'X',sizeof owner.path); observe_owner(o,owner);
    check(!o.identity_complete);
    // Explicit observation wrapper resets a stale candidate before host preflight failure.
    o=fixed(); check(!inventory_observation(&owner,o.initial,"PURE",o) && !o.complete);
    o=fixed(); check(!inventory_observation(nullptr,o.initial,"PURE",o) && !o.complete);
    static_assert(std::is_same<decltype(&inventory),bool(*)(void*,const State&,const char*)>::value,"dependency ABI");
    std::printf("placement receipt pure checks: %u passed; host-only, no native acquisition proof\n",checks);
}
