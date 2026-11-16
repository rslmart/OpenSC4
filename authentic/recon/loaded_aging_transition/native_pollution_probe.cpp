// Read-only original/checkpoint diagnostic. No writers or object loading.
#include "native_pollution_snapshot.h"
#include "opensc4/dbpf.h"
#include <CommonCrypto/CommonDigest.h>
#include <cstdio>
#include <cinttypes>
#include <fstream>
#include <iomanip>
#include <sstream>
using namespace native_pollution;
std::string shaFile(const char* path){
    std::ifstream f(path,std::ios::binary);if(!f)return "UNREADABLE";
    CC_SHA256_CTX ctx;CC_SHA256_Init(&ctx);char buf[65536];
    while(f.read(buf,sizeof buf) || f.gcount())CC_SHA256_Update(&ctx,buf,static_cast<CC_LONG>(f.gcount()));
    unsigned char digest[CC_SHA256_DIGEST_LENGTH];CC_SHA256_Final(digest,&ctx);
    std::ostringstream out;for(auto v:digest)out<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(v);return out.str();
}
std::string shaBytes(std::span<const U8> b){unsigned char d[CC_SHA256_DIGEST_LENGTH];CC_SHA256(b.data(),static_cast<CC_LONG>(b.size()),d);std::ostringstream s;for(auto v:d)s<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(v);return s.str();}
void printRef(const char* name,std::size_t i,const Ref& r){std::printf("REF %s[%zu] identity=%016" PRIx64 " class=%08x\n",name,i,static_cast<uint64_t>(r.identity),r.classID);}
int main(int argc,char** argv){
    if(argc<2)return 2;
    for(int i=1;i<argc;++i){
        opensc4::cDBPFArchive archive;
        std::printf("FILE path=%s sha256=%s\n",argv[i],shaFile(argv[i]).c_str());
        if(!archive.Open(argv[i]))return 3;
        std::vector<U8> m;Metadata md;md.type=0xea3ff4f5;md.group=0xea3ff506;md.instance=0xea3ff4f5;
        const auto* entry=archive.FindEntry({md.type,md.group,md.instance});md.present=entry!=nullptr;
        md.decoded=entry && archive.ReadEntry(*entry,m);md.bytes=m;
        std::printf("METADATA exactTGI=%08x:%08x:%08x present=%d decoded=%d bytes=%zu raw=",md.type,md.group,md.instance,md.present,md.decoded,m.size());for(auto v:m)std::printf("%02x",v);std::puts("");
        auto entries=archive.FindEntriesByType(opensc4::kCLSID_cSC4PollutionSimulator);
        if(entries.size()!=1){std::printf("UNSUPPORTED pollution entry count=%zu\n",entries.size());return 4;}
        std::vector<U8> bytes;if(!archive.ReadEntry(*entries[0],bytes))return 5;
        std::printf("POLLUTION TGI=%08x:%08x:%08x bytes=%zu sha256=%s\n",entries[0]->tgi.type,entries[0]->tgi.group,entries[0]->tgi.instance,bytes.size(),shaBytes(bytes).c_str());
        const auto r=decode(md,bytes);
        std::printf("DECODE status=%d reason=%s offset=%zu width=%u records=%zu\n",int(r.status),r.reason.c_str(),r.offset,r.identityBytes,r.records.size());
        if(r.status!=Status::Ok)return 6;
        for(const auto& s:r.records){
            std::printf("RECORD identity=%016" PRIx64 " size=%u body=%u CRCstored=%08x CRCcomputed=%08x C0=%zu D8=%zu F0=%zu L108=%zu\n",static_cast<uint64_t>(s.identity),s.size,s.version,s.storedCRC,s.computedCRC,s.listC0.size(),s.listD8.size(),s.listF0.size(),s.list108.size());
            for(std::size_t j=0;j<4;++j){printRef("78",j,s.grids78[j]);printRef("98",j,s.grids98[j]);}
            for(auto pair:{std::pair{"C0",&s.listC0},{"D8",&s.listD8},{"F0",&s.listF0},{"108",&s.list108}})for(std::size_t j=0;j<pair.second->size();++j)printRef(pair.first,j,(*pair.second)[j]);
            printRef("simManager",0,s.simManager);
            for(auto v:s.scalars)std::printf("SCALAR %s bytes=%u raw=%016" PRIx64 "\n",v.name,v.bytes,static_cast<uint64_t>(v.bits));
            std::printf("ITERATOR prefix=%02x%02x%02x count=%zu\n",s.random.prefix[0],s.random.prefix[1],s.random.prefix[2],s.random.indices.size());
            for(std::size_t j=0;j<5;++j)std::printf("ITERATOR value[%zu]=%08x\n",j,s.random.values[j]);
            for(std::size_t j=0;j<s.random.indices.size();++j)std::printf("ITERATOR index[%zu]=%04x\n",j,s.random.indices[j]);
            for(std::size_t j=0;j<3;++j)std::printf("ITERATOR tail[%zu]=%08x\n",j,s.random.tail[j]);
        }
    }
}
