// Read-only saved-byte diagnostic. All file access is input-only.
#include "placement_record_snapshot.h"
#include "opensc4/dbpf.h"
#include <CommonCrypto/CommonDigest.h>
#include <cstdio>
#include <cinttypes>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <limits>
using namespace placement_record;
static std::string sha(std::span<const U8> bytes) {
    CC_SHA256_CTX ctx; CC_SHA256_Init(&ctx);
    while(!bytes.empty()) {
        const auto n=std::min<std::size_t>(bytes.size(),65536);
        CC_SHA256_Update(&ctx,bytes.data(),static_cast<CC_LONG>(n));bytes=bytes.subspan(n);
    }
    unsigned char d[CC_SHA256_DIGEST_LENGTH];CC_SHA256_Final(d,&ctx);
    std::ostringstream s;for(auto v:d)s<<std::hex<<std::setfill('0')<<std::setw(2)<<unsigned(v);return s.str();
}
static std::vector<U8> readComplete(const char* path) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);
    if(!f)throw std::runtime_error("file open failed");
    const auto end=f.tellg();
    if(end<0 || static_cast<uintmax_t>(end)>static_cast<uintmax_t>(std::numeric_limits<std::streamsize>::max()))
        throw std::runtime_error("invalid file length");
    std::vector<U8> bytes(static_cast<std::size_t>(end)); f.seekg(0);
    if(!f)throw std::runtime_error("file seek failed");
    if(!bytes.empty())f.read(reinterpret_cast<char*>(bytes.data()),static_cast<std::streamsize>(bytes.size()));
    if(!f || static_cast<std::size_t>(f.gcount())!=bytes.size())throw std::runtime_error("incomplete file read");
    char extra;f.read(&extra,1);
    if(f.gcount()!=0 || !f.eof() || f.bad())throw std::runtime_error("file changed or final IO failed");
    return bytes;
}
int main(int argc,char** argv) {
    if(argc!=3) {std::fprintf(stderr,"usage: probe PATH EXPECTED_SHA256\n");return 2;}
    try {
        const std::string expected=argv[2];
        if(expected.size()!=64 || expected.find_first_not_of("0123456789abcdef")!=std::string::npos)
            throw std::runtime_error("expected SHA256 must be 64 lowercase hex digits");
        auto bytes=readComplete(argv[1]); const auto digest=sha(bytes);
        if(digest!=expected)throw std::runtime_error("complete-file SHA256 mismatch before archive parse");
        std::printf("FILE path=%s bytes=%zu sha256=%s\n",argv[1],bytes.size(),digest.c_str());
        opensc4::cDBPFArchive archive;
        if(!archive.OpenFromMemory(std::move(bytes),argv[1]))throw std::runtime_error("DBPF parse failed");
        constexpr Key pollutionKey{0x8990c065,0x299b2d1b,0};
        std::vector<Key> keys;for(const auto& e:archive.Entries())keys.push_back({e.tgi.type,e.tgi.group,e.tgi.instance});
        for(auto k:{metadataKey,pollutionKey,tutorialKey,buildingKey})
            if(!uniqueExact(keys,k))throw std::runtime_error("requires unique exact metadata/pollution/tutorial/building TGIs");
        auto read=[&](Key k){std::vector<U8> b;if(!archive.ReadByTGI({k.type,k.group,k.instance},b))throw std::runtime_error("subfile decode failed");return b;};
        auto m=read(metadataKey);
        Metadata md{true,true,metadataKey.type,metadataKey.group,metadataKey.instance,m};
        auto p=read(pollutionKey),t=read(tutorialKey);
        auto pollution=native_pollution::decode(md,p);
        auto tutorial=native_tutorial::decode(md,tutorialKey,t);
        if(pollution.status!=Status::Ok || tutorial.status!=Status::Ok)
            throw std::runtime_error("pollution/tutorial helper decode failed");
        if(pollution.records.size()!=1 || pollution.records[0].listC0.size()!=1)
            throw std::runtime_error("probe domain requires one pollution record and exactly one C0 ref");
        const auto ref=pollution.records[0].listC0[0];
        // Class stream key construction @ 0x1003d0cb2..0x1003d0cc2.
        const Key key{ref.classID,0x299b2d1b,0};
        if(!same(key,buildingKey))throw std::runtime_error("C0 class outside a9bd882d scope");
        auto stream=read(key);
        const auto result=scan(md,ref,key,stream);
        std::printf("FRAMING appRaw=");for(auto b:m)std::printf("%02x",b);
        std::printf(" pollutionRecords=%zu pollutionBytes=%zu tutorialRecords=%zu tutorialBytes=%zu C0=%016" PRIx64 " class=%08x\n",
                    pollution.records.size(),pollution.offset,tutorial.records.size(),tutorial.offset,ref.identity,ref.classID);
        for(const auto& r:tutorial.records)std::printf("TUTORIAL rawActive=%u crc=%08x/%08x\n",unsigned(r.active),r.storedCRC,r.computedCRC);
        std::printf("CLASS TGI=%08x:%08x:%08x bytes=%zu sha256=%s records=%zu consumed=%zu status=%d reason=%s\n",
                    key.type,key.group,key.instance,stream.size(),sha(stream).c_str(),result.recordCount,result.consumed,int(result.status),result.reason.c_str());
        if(result.status!=Status::Ok)throw std::runtime_error(result.reason);
        const auto& s=*result.selected;
        std::printf("SELECTED identity=%016" PRIx64 " offset=%zu size=%u headerSize=%zu bodyOffset=%zu bodySize=%zu storedCRC=%08x computedCRC=%08x rawBodyVersion=%u bodySHA256=%s\n",
                    s.identity,s.offset,s.size,s.headerSize,s.bodyOffset,s.bodySize,s.storedCRC,s.computedCRC,s.rawBodyVersion,sha(s.body).c_str());
        std::puts("HEADER_RESOLUTION_ONLY exemplarTGI/lot=UNPROVEN saved/runtime_pointer_equality=UNPROVEN reload=UNPROVEN aging_acceptance=NO export=DIAGNOSTIC runtime_END_complete=0");
        return 0;
    } catch(const std::exception& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}
}
