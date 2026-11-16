#include "opensc4/dbpf.h"
#include "opensc4/sc4_subfiles.h"
#include <cstdio>
#include <cstring>
#include <set>
#include <tuple>
using namespace opensc4;
// Read-only evidence probe, not a runtime property accessor or inheritance model.
int main() {
    cDBPFSystem sys;
    const auto app = cDBPFSystem::DefaultSteamSC4AppPath();
    if (!sys.MountInstall(app)) return 2;
    cDBPFArchive city;
    if (!city.Open(app + "/Contents/GameData/Regions/Timbuktu/City - Rush Hour Tutorial.sc4")) return 3;
    std::vector<Uint8> bytes;
    std::vector<BuildingOccupantSaveRecord> buildings;
    if (!city.ReadFirstByType(kCLSID_cSC4BuildingOccupant, bytes) ||
        !DecodeMultiRecordSubfile<BuildingOccupantSaveRecord>(bytes, buildings)) return 4;
    const BuildingOccupantSaveRecord* saved = nullptr;
    for (const auto& b : buildings) if (b.memAddr == 0x138a8240u) {
        if (saved) return 5;
        saved = &b;
    }
    if (!saved) return 6;
    std::set<Uint32> wanted{0xc910b4cf,0xea1cf220,0xea1cf221,0xea1cf222,
        0x28ecf510,0xc8ebd131,0xe8ecf641,0xa8ebcf0c,0x8a1e07ee,0x48ebd0b1};
    for (const auto& p : saved->base.exemplarHolder.properties.properties) {
        wanted.insert(p.holderPropId);
        std::printf("saved holder=%08x property=%08x tag=%08x dbtype=%08x count=%u bytes=",
            p.holderPropId,p.propertyId,p.variantTag,p.dbType,p.arrayCount);
        for (auto b : p.rawValueBytes) std::printf("%02x",b);
        std::puts("");
    }
    cSC4TGI key = saved->base.exemplarHolder.exemplarKey;
    std::set<std::tuple<Uint32,Uint32,Uint32>> visited;
    for (unsigned depth=0; depth<16; ++depth) {
        if (!visited.emplace(key.type,key.group,key.instance).second) return 7;
        if (!sys.LoadAssetByTGI(key,bytes)) return 8;
        std::printf("resource depth=%u tgi=%08x:%08x:%08x decoded_size=%zu\n",
            depth,key.type,key.group,key.instance,bytes.size());
        for (const auto& a : sys.Archives()) if (auto e=a->FindEntry(key))
            std::printf("candidate archive=%s offset=%u size=%u\n",a->Path().c_str(),e->offset,e->fileSize);
        if (bytes.size()<24 || (std::memcmp(bytes.data(),"EQZB1###",8) &&
            std::memcmp(bytes.data(),"CQZB1###",8))) return 9;
        std::size_t pos=8;
        auto get=[&](unsigned n)->Uint32 {
            if(pos+n>bytes.size()) throw "truncated";
            Uint32 v=0; for(unsigned i=0;i<n;++i) v|=Uint32(bytes[pos++])<<(8*i);
            return v;
        };
        cSC4TGI parent{get(4),get(4),get(4)};
        const auto count=get(4);
        std::printf("header bytes="); for(unsigned i=0;i<24;++i) std::printf("%02x",bytes[i]); std::puts("");
        std::printf("parent=%08x:%08x:%08x count=%u\n",parent.type,parent.group,parent.instance,count);
        std::set<Uint32> seen;
        for(Uint32 i=0;i<count;++i) {
            const auto start=pos; const auto id=get(4),type=get(2),tag=get(2),unused=get(1);
            const auto n=tag==0x80 ? get(4):1;
            unsigned width=0;
            switch(type) {
                case 0x100: case 0x400: case 0xb00: case 0xc00: width=1;break;
                case 0x200: case 0x500: width=2;break;
                case 0x300: case 0x700: case 0x900: width=4;break;
                case 0x800: width=8;break;
                default:return 10;
            }
            if(n>(bytes.size()-pos)/width) return 11;
            pos+=std::size_t(n)*width;
            if(!seen.insert(id).second) std::printf("duplicate id=%08x ordinal=%u\n",id,i);
            if(wanted.contains(id)) {
                std::printf("property id=%08x type=%04x tag=%04x unused=%02x count=%u offset=%zu record_bytes=",id,type,tag,unused,n,start);
                for(auto j=start;j<pos;++j) std::printf("%02x",bytes[j]); std::puts("");
            }
        }
        for (auto id : wanted) if (!seen.contains(id)) std::printf("absent local property=%08x\n",id);
        std::printf("consumed=%zu total=%zu unique=%zu\n",pos,bytes.size(),seen.size());
        if(pos!=bytes.size()) return 12;
        if(parent.type==0 && parent.group==0 && parent.instance==0) { std::puts("chain terminates at null TGI");return 0; }
        key=parent;
    }
    return 13;
}
