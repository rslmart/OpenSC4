// Read-only saved-byte diagnostic. All file access is input-only.
#include "native_tutorial_snapshot.h"
#include "opensc4/dbpf.h"
#include <CommonCrypto/CommonDigest.h>
#include <cstdio>
#include <cinttypes>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <limits>
using namespace native_tutorial;
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
    if(argc!=4 || (std::string(argv[3])!="baseline" && std::string(argv[3])!="checkpoint")) {
        std::fprintf(stderr,"usage: probe PATH EXPECTED_SHA256 baseline|checkpoint\n");return 2;
    }
    try {
        const std::string expected=argv[2];
        if(expected.size()!=64 || expected.find_first_not_of("0123456789abcdef")!=std::string::npos)
            throw std::runtime_error("expected SHA256 must be 64 lowercase hex digits");
        auto bytes=readComplete(argv[1]);const auto digest=sha(bytes);
        std::printf("FILE path=%s bytes=%zu sha256=%s\n",argv[1],bytes.size(),digest.c_str());
        if(digest!=expected)throw std::runtime_error("complete-file SHA256 mismatch");
        opensc4::cDBPFArchive archive;
        if(!archive.OpenFromMemory(std::move(bytes),argv[1]))throw std::runtime_error("DBPF parse failed");
        std::vector<Key> keys;for(const auto& e:archive.Entries())keys.push_back({e.tgi.type,e.tgi.group,e.tgi.instance});
        if(!uniqueExact(keys,metadataKey))throw std::runtime_error("adapter requires one exact metadata TGI; missing or duplicate");
        if(!uniqueExact(keys,tutorialKey))throw std::runtime_error("adapter requires one exact tutorial TGI; missing or duplicate");
        std::vector<U8> m,body;
        Metadata md{true,false,metadataKey.type,metadataKey.group,metadataKey.instance,{}};
        md.decoded=archive.ReadByTGI({md.type,md.group,md.instance},m);md.bytes=m;
        std::printf("METADATA TGI=%08x:%08x:%08x decoded=%d bytes=%zu raw=",md.type,md.group,md.instance,md.decoded,m.size());
        for(auto v:m)std::printf("%02x",v);std::puts("");
        if(!archive.ReadByTGI({tutorialKey.type,tutorialKey.group,tutorialKey.instance},body))throw std::runtime_error("tutorial decompression/read failed");
        std::printf("TUTORIAL TGI=%08x:%08x:%08x decompressedBytes=%zu sha256=%s\n",tutorialKey.type,tutorialKey.group,tutorialKey.instance,body.size(),sha(body).c_str());
        const auto r=decode(md,tutorialKey,body);
        std::printf("DECODE status=%d reason=%s appFormat=%08x width=%u consumed=%zu/%zu records=%zu\n",int(r.status),r.reason.c_str(),r.appFormat,r.identityBytes,r.offset,body.size(),r.records.size());
        if(r.status!=Status::Ok)throw std::runtime_error("tutorial decoder rejected saved bytes");
        bool inactive=true;
        for(std::size_t i=0;i<r.records.size();++i) {
            const auto& s=r.records[i];
            std::printf("RECORD index=%zu identity=%016" PRIx64 " size=%u CRCstored=%08x CRCcomputed=%08x bodyVersion=%u rawActive=%02x rawTask=%08x rawGUID=%08x\n",i,static_cast<uint64_t>(s.identity),s.size,s.storedCRC,s.computedCRC,s.version,unsigned(s.active),s.task,s.guid);
            inactive=inactive && s.active==0;
        }
        if(std::string(argv[3])=="checkpoint" && !inactive)throw std::runtime_error("checkpoint capture criterion failed: raw active byte is nonzero");
        std::printf("CRITERION mode=%s result=PASS scope=saved-bytes-only\n",argv[3]);
        return 0;
    }catch(const std::exception& e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}
}
