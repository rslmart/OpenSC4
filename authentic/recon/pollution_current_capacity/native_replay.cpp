// Replay original-game evidence: capacity @ 0x1000992cc, curve setter
// @ 0x1000ac37a, evaluator @ 0x1000ac4f6. Link actual production library.
#include "opensc4/civic_simulators.h"
#include "opensc4/tract_developer.h"
#include <array>
#include <bit>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <set>
#include <string>
using namespace opensc4;
static float F(Uint32 v) { return std::bit_cast<float>(v); }
static Uint32 B(float v) { return std::bit_cast<Uint32>(v); }
struct Fixture final : OccupantSeam, OccupantPropertyHolderSeam,
    PollutionCapacityAccessSeam, RawCurvePropertySeam, RawCurveValueSeam {
    Uint32 cap{}, scale{}, input{}, power{}, capok{}, ageok{}, count{};
    bool cp{}, ip{}, rp{};
    std::array<float,8> raw{};
    const OccupantPropertyHolderSeam* GetPropertyHolder() const override { return this; }
    bool GetPropertyValue(Uint32, Sint64&) const override { return false; }
    bool GetPropertyValue(Uint32 id, Float32& out) const override {
        if(id != 0xc910b4cfu || !ip) return false;
        out=F(input); return (ageok & 255u)!=0;
    }
    const CurvePropertyData* GetProperty(Uint32) const override { return nullptr; }
    bool ReadCapacity(const OccupantPropertyHolderSeam&,Uint32 id,Uint32& out) const override {
        if(id != 0x12345678u || !cp) return false;
        out=cap; return (capok & 255u)!=0;
    }
    bool IsOccupantPowered(const OccupantSeam&) const override { return (power&255u)!=0; }
    const RawCurvePropertySeam* LookupCurve(const OccupantPropertyHolderSeam&,Uint32 id) const override {
        return id==0x8a1e07eeu && rp ? this : nullptr;
    }
    const RawCurveValueSeam* GetValue() const override { return this; }
    const Float32* GetScalarData() const override { return raw.data(); }
    Uint32 GetScalarCount() const override { return count; }
};
int main(int argc,char** argv) {
    if(argc!=2) return 2;
    std::ifstream file(argv[1]); if(!file) return 2;
    unsigned capacity=0,setters=0,records=0,evaluations=0,failures=0;
    std::set<unsigned> capacityIDs, recordIDs, inputIDs;
    unsigned currentID=0, curveCount=0, nextID=0;
    bool active=false, cleared=false, backed=false;
    const auto complete=[&] { return !active || (cleared && inputIDs.size()==unsigned(backed)); };
    cSC4ResponseCurve curve;
    for(std::string line;std::getline(file,line);) {
        std::istringstream in(line);char kind{};unsigned id{};in>>kind>>id;
        if(!in) return 2;
        bool ok=false;
        if(kind=='C') {
            if(active || id>=2598 || !capacityIDs.insert(id).second) return 2;
            Fixture f;std::string present;Uint32 ret{},value{};
            in>>std::hex>>f.cap>>f.scale>>f.input>>f.power>>f.capok>>f.ageok;
            in>>present>>ret>>value>>std::dec>>f.count;
            for(auto& v:f.raw) {Uint32 b{};in>>std::hex>>b;v=F(b);}
            if(!in || f.count>8 || present.size()!=3 || present.find_first_not_of("01")!=std::string::npos) return 2;
            f.cp=present.at(0)=='1';f.ip=present.at(1)=='1';f.rp=present.at(2)=='1';
            PollutionSimulatorSaveRecord record;record.capacityAccess=&f;record.field348=F(f.scale);
            Uint32 out=0xdeadbeefu;
            try {bool got=record.GetCurrentCapacity(0x12345678u,f,out);ok=unsigned(got)==ret && out==value;}
            catch(...) {ok=false;}
            ++capacity;
        } else if(kind=='S' || kind=='Z') {
            unsigned ret{},count{},hasStorage{};in>>std::hex>>ret>>std::dec>>count>>hasStorage;
            if(!in || hasStorage>1 || count>4) return 2;
            bool got;
            if(kind=='S') {
                if(!complete() || id!=nextID++) return 2;
                currentID=id;active=true;cleared=false;backed=hasStorage!=0;curveCount=count;
                recordIDs.clear();inputIDs.clear();
                curve=cSC4ResponseCurve{};unsigned n{};in>>n;std::array<float,8> raw{};
                for(auto& v:raw){Uint32 b{};in>>std::hex>>b;v=F(b);}
                if(!in || n>8) return 2;
                got=curve.SetResponseCurveData(std::span<const float>(raw.data(),n));
            } else {
                if(!active || cleared || id!=currentID || recordIDs.size()!=curveCount ||
                   inputIDs.size()!=(backed?13u:0u) || hasStorage!=unsigned(backed)) return 2;
                cleared=true;inputIDs.clear();
                got=curve.SetResponseCurveData(std::span<const float>{});
            }
            float out=123;bool hasBacking=curve.TryGetOutputValue(0,out);
            ok=unsigned(got)==ret && curve.Segments().size()==count && unsigned(hasBacking)==hasStorage;
            ++setters;
        } else if(kind=='R') {
            unsigned index{};in>>index;std::array<Uint32,5> want{};
            for(auto& b:want)in>>std::hex>>b;
            if(!in || !active || cleared || id!=currentID || !recordIDs.insert(index).second) return 2;
            if(index<curve.Segments().size()) {
                auto s=curve.Segments()[index];std::array<Uint32,5> got{B(s.x),B(s.y),B(s.prevX),B(s.prevY),B(s.slope)};
                ok=got==want;
            }
            ++records;
        } else if(kind=='E') {
            Uint32 input{},want{};in>>std::hex>>input>>want;float out;
            if(!in || !active || id!=currentID || !inputIDs.insert(input).second ||
               (cleared && input!=0x7fc12345u)) return 2;
            ok=curve.TryGetOutputValue(F(input),out) && B(out)==want;
            ++evaluations;
        } else return 2;
        if(!in || !ok) {++failures;std::printf("FAIL %c id=%u\n",kind,id);}
    }
    std::printf("capacity=%u setters=%u records=%u evaluations=%u mismatches=%u\n",capacity,setters,records,evaluations,failures);
    return failures || capacity!=2285 || setters!=46 || records!=35 || evaluations!=280 || nextID!=23 || !complete();
}
