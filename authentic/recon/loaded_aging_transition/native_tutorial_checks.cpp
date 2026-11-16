// SYNTHETIC fixture bytes only; no production native writer or game evidence.
#include "native_tutorial_snapshot.h"
#include <cstdio>
using namespace native_tutorial;
static int checks=0,fail=0;
static void check(bool ok,const char* name){++checks;if(!ok){++fail;std::printf("FAIL %s\n",name);}}
static void put(std::vector<U8>& b,std::size_t at,U64 v,unsigned n){for(unsigned i=0;i<n;++i)b.at(at+i)=static_cast<U8>(v>>(8*i));}
static void seal(std::vector<U8>& b){put(b,0,b.size(),4);put(b,4,native_pollution::recordCRC(b),4);}
static std::vector<U8> fixture(unsigned w,U64 identity=0x12345678,U8 active=0xfe){
    std::vector<U8> b(8+w+11);put(b,8,identity,w);put(b,8+w,2,2);b[10+w]=active;
    put(b,11+w,0x80000001,4);put(b,15+w,0xfedcba98,4);seal(b);return b;
}
int main(){
    std::vector<U8> m(4); Metadata md{true,true,metadataKey.type,metadataKey.group,metadataKey.instance,m};
    auto run=[&](const std::vector<U8>& b){return decode(md,tutorialKey,b);};
    auto reject=[&](const std::vector<U8>& b,const char* name){const auto r=run(b);check(r.status!=Status::Ok && r.records.empty(),name);};
    for(U32 app=0x3e;app<=0x42;++app){
        put(m,0,app,4);const unsigned w=app<=0x41?4:8;
        auto b=fixture(w,w==8?0x1234567800000000ULL:0x89abcdef);
        auto r=run(b);check(r.status==Status::Ok && r.offset==b.size() && r.records.size()==1,"complete framing");
        check(r.identityBytes==w && r.appFormat==app,"metadata width");
        if(r.records.size()==1){const auto& s=r.records[0];
            check(s.identity==(w==8?0x1234567800000000ULL:0x89abcdef),"high word identity preserved");
            check(s.size==8+w+11 && s.storedCRC==s.computedCRC,"full size CRC");
            check(s.version==2 && s.active==0xfe && s.task==0x80000001 && s.guid==0xfedcba98,"raw fields and ordering");}
        for(std::size_t n=0;n<b.size();++n){auto t=std::vector<U8>(b.begin(),b.begin()+n);reject(t,"every truncated prefix");if(n>=8){seal(t);reject(t,"CRC resealed truncated prefix");}}
        for(std::size_t i=0;i<b.size();++i){auto t=b;t[i]^=0x80;reject(t,"tamper each byte incl identity high word and body");}
        for(unsigned active:{0U,1U,2U,255U}){auto t=fixture(w,0,static_cast<U8>(active));auto z=run(t);check(z.status==Status::Ok && z.records[0].identity==0 && z.records[0].active==active,"zero identity and raw bool");}
        for(U32 v:{0U,1U,3U,65535U}){auto t=b;put(t,8+w,v,2);seal(t);auto z=run(t);check(z.status==Status::Unsupported && z.records.empty(),"unsupported version");}
        for(U32 size:{0U,7U,12U,0xffffffffU,static_cast<U32>(b.size()-1),static_cast<U32>(b.size()+1)}){auto t=b;put(t,0,size,4);reject(t,"wrong record size");}
        auto t=b;t.push_back(0);seal(t);reject(t,"extra body byte");
        t=b;t.push_back(0);reject(t,"trailing sequence byte");
        t=b;t.insert(t.end(),b.begin(),b.end());r=run(t);check(r.status==Status::Ok && r.records.size()==2 && r.offset==t.size(),"multiple valid records");
        t.back()^=1;reject(t,"later failure publishes no prefix");
        t.clear();for(int i=0;i<4096;++i)t.insert(t.end(),b.begin(),b.end());r=run(t);check(r.status==Status::Ok && r.records.size()==4096,"record count limit accepted");
        t.insert(t.end(),b.begin(),b.end());r=run(t);check(r.status==Status::Unsupported && r.records.empty(),"record count over-limit explicit");
    }
    auto b=fixture(8);put(m,0,0x42,4);
    for(U32 app:{0U,0x3dU,0x43U,0xffffffffU}){put(m,0,app,4);check(run(b).status==Status::Unsupported,"unsupported app metadata");}put(m,0,0x42,4);
    md.present=false;check(run(b).status==Status::Unsupported,"missing metadata");md.present=true;
    md.decoded=false;check(run(b).status==Status::Unsupported,"failed metadata");md.decoded=true;
    for(std::size_t n:{0U,1U,2U,3U,5U}){std::vector<U8> bad(n);md.bytes=bad;check(run(b).status==Status::Unsupported,"short or oversized metadata");}md.bytes=m;
    for(int i=0;i<3;++i){auto bad=md;if(i==0)bad.type^=1;if(i==1)bad.group^=1;if(i==2)bad.instance^=1;check(decode(bad,tutorialKey,b).status==Status::Unsupported,"wrong metadata TGI component");
        auto k=tutorialKey;if(i==0)k.type^=1;if(i==1)k.group^=1;if(i==2)k.instance^=1;check(decode(md,k,b).status==Status::Unsupported,"wrong tutorial TGI component");}
    std::vector<Key> keys;check(!uniqueExact(keys,tutorialKey),"missing exact entry");keys.push_back({tutorialKey.type,0,0});check(!uniqueExact(keys,tutorialKey),"wrong group entry");keys.push_back(tutorialKey);check(uniqueExact(keys,tutorialKey),"one exact entry");keys.push_back(tutorialKey);check(!uniqueExact(keys,tutorialKey),"duplicate exact entries rejected");
    std::printf("SYNTHETIC checks=%d fail=%d\n",checks,fail);return fail?1:0;
}
