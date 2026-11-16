#include "placement_record_snapshot.h"
#include <cstdio>
#include <cstdlib>
using namespace placement_record;
static int checks=0;
static void require(bool ok,const char* name) {++checks;if(!ok){std::fprintf(stderr,"FAIL %s\n",name);std::exit(1);}}
static void put(std::vector<U8>& b,std::size_t p,U64 v,unsigned n) {for(unsigned i=0;i<n;++i)b[p+i]=U8(v>>(i*8));}
// Synthetic fixture encoder only. CRC oracle below is independent bitwise math.
static U32 crc(std::span<const U8> b) {
    U32 v=0xffffffff;
    for(auto byte:b){v^=U32(byte)<<24;for(int k=0;k<8;++k)v=(v&0x80000000)?(v<<1)^0x04c11db7:(v<<1);}
    return v;
}
static void seal(std::vector<U8>& b) {put(b,4,crc(std::span<const U8>(b).subspan(8,std::min<std::size_t>(b.size()-8,0x3d090))),4);}
static std::vector<U8> record(U64 id,std::size_t size=23,unsigned version=2) {
    std::vector<U8> b(size,0x5a);put(b,0,size,4);put(b,8,id,8);put(b,16,version,2);seal(b);return b;
}
static std::vector<U8> join(std::vector<U8> a,const std::vector<U8>& b) {a.insert(a.end(),b.begin(),b.end());return a;}
int main() {
    std::vector<U8> meta{0x42,0,0,0};
    Metadata md{true,true,metadataKey.type,metadataKey.group,metadataKey.instance,meta};
    constexpr U64 target=0x1234567800000042ULL;
    const Ref ref{target,buildingKey.type};
    auto run=[&](std::span<const U8> b){return scan(md,ref,buildingKey,b);};
    auto fail=[&](std::span<const U8> b,Status expected,const char* name){auto r=run(b);require(r.status==expected && !r.selected,name);};
    auto good=record(target);auto r=run(good);
    require(r.status==Status::Ok && r.selected && r.consumed==good.size(),"full consumption");
    require(r.selected->identity==target && r.selected->body==std::vector<U8>(good.begin()+16,good.end()) && r.selected->bodyOffset==16 && r.selected->headerSize==16 && r.selected->bodySize==7,"full64 and 16-byte header/body ownership");
    good[18]^=1;require(r.selected->body[2]==0x5a,"owned body survives input mutation");good[18]^=1;
    auto high=record(0xabcdef0100000000ULL);
    auto hr=scan(md,{0xabcdef0100000000ULL,buildingKey.type},buildingKey,high);
    require(hr.status==Status::Ok && hr.selected->identity==0xabcdef0100000000ULL,"highword-only target is nonnull");
    require(scan(md,{0,buildingKey.type},buildingKey,high).status==Status::Unsupported,"null full64 target");
    auto other=record(0x8765432100000042ULL,23,77);
    r=run(join(other,good));require(r.status==Status::Ok && r.recordCount==2 && r.selected->offset==other.size(),"same low32 different high32 and nonselected version ignored");
    fail(other,Status::Malformed,"low32 collision cannot satisfy full64 target");
    fail({},Status::Malformed,"empty missing target");
    fail(join(good,good),Status::Malformed,"duplicate full64 target atomic");
    auto nullrec=record(0);require(run(join(nullrec,good)).status==Status::Ok,"null nonselected record retains framing");
    for(std::size_t n=0;n<good.size();++n)fail(std::span<const U8>(good).first(n),Status::Malformed,"all record truncations");
    for(auto size:{0U,12U,16U,17U,24U,0xffffffffU}){auto b=good;put(b,0,size,4);fail(b,Status::Malformed,"invalid total size");}
    auto bad=good;bad[8]^=1;fail(bad,Status::Malformed,"CRC includes identity offset8");
    bad=good;bad[12]^=1;fail(bad,Status::Malformed,"CRC includes high identity word");
    bad=good;bad[18]^=1;fail(bad,Status::Malformed,"CRC includes body");
    bad=good;put(bad,4,crc(std::span<const U8>(bad).subspan(16)),4);fail(bad,Status::Malformed,"body-only CRC rejected");
    bad=good;put(bad,16,3,2);seal(bad);fail(bad,Status::Unsupported,"resealed selected version outside policy");
    bad=other;bad[18]^=1;fail(join(good,bad),Status::Malformed,"selected first later CRC fails atomically");
    bad=join(good,other);bad.pop_back();fail(bad,Status::Malformed,"selected first later truncation atomic");
    bad=good;bad.push_back(0);fail(bad,Status::Malformed,"trailing byte rejected");
    auto legacy=good;legacy.erase(legacy.begin()+12,legacy.begin()+16);put(legacy,0,legacy.size(),4);seal(legacy);fail(legacy,Status::Malformed,"legacy12 header cannot resolve full64");
    auto capped=record(target,8+0x3d090+1);
    require(run(capped).status==Status::Ok,"CRC cap plus one");
    capped.back()^=1;require(run(capped).status==Status::Ok,"byte beyond cap excluded");
    capped[capped.size()-2]^=1;fail(capped,Status::Malformed,"last byte inside cap included");
    auto boundary=record(target,8+0x3d090);boundary.back()^=1;fail(boundary,Status::Malformed,"exact cap last byte included");
    auto pre=record(target,8+0x3d090-1);pre.back()^=1;fail(pre,Status::Malformed,"cap minus one last byte included");
    for(int field=0;field<6;++field){auto m=md;if(field==0)m.present=false;if(field==1)m.decoded=false;if(field==2)m.type^=1;if(field==3)m.group^=1;if(field==4)m.instance^=1;if(field==5)m.bytes=m.bytes.first(3);require(scan(m,ref,buildingKey,good).status==Status::Unsupported,"metadata validation");}
    auto extra=meta;extra.push_back(0);auto m=md;m.bytes=extra;require(scan(m,ref,buildingKey,good).status==Status::Unsupported,"metadata overwidth");
    for(U8 v:{U8(0x41),U8(0x43),U8(0)}){auto b=meta;b[0]=v;m=md;m.bytes=b;require(scan(m,ref,buildingKey,good).status==Status::Unsupported,"metadata unsupported format");}
    require(scan(md,{target,1},buildingKey,good).status==Status::Unsupported,"wrong ref class");
    for(auto k:{Key{1,buildingKey.group,0},Key{buildingKey.type,1,0},Key{buildingKey.type,buildingKey.group,1}})require(scan(md,ref,k,good).status==Status::Unsupported,"wrong exact TGI");
    std::vector<U8> sequence=good;auto tiny=record(1,18,99);
    for(std::size_t i=1;i<maxRecords;++i)sequence.insert(sequence.end(),tiny.begin(),tiny.end());
    r=run(sequence);require(r.status==Status::Ok && r.recordCount==maxRecords && r.consumed==sequence.size(),"record count at policy limit");
    sequence.insert(sequence.end(),tiny.begin(),tiny.end());fail(sequence,Status::Unsupported,"record count over policy limit atomic");
    std::vector<U8> huge(maxStreamBytes+1);fail(huge,Status::Unsupported,"stream allocation bound");
    std::puts("synthetic-only header-resolution checks");std::printf("PASS %d checks\n",checks);
}
