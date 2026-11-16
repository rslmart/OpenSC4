#pragma once
// Scoped read-only diagnostic; no resolution, lifecycle, or native writer.
#include "native_pollution_snapshot.h"
namespace native_tutorial {
using native_pollution::U8;
using native_pollution::U32;
using native_pollution::U64;
using native_pollution::Metadata;
using native_pollution::Status;
struct Key { U32 type, group, instance; };
inline constexpr Key tutorialKey{0xaa3836fb,0x299b2d1b,0};
inline constexpr Key metadataKey{0xea3ff4f5,0xea3ff506,0xea3ff4f5};
inline bool same(Key a, Key b) {return a.type==b.type && a.group==b.group && a.instance==b.instance;}
// Exact class TGI construction @ 0x1003d0cb2. Counts are adapter policy.
inline bool uniqueExact(std::span<const Key> keys, Key wanted) {
    return std::count_if(keys.begin(),keys.end(),[&](Key k){return same(k,wanted);})==1;
}
struct Record {
    U32 size=0, storedCRC=0, computedCRC=0;
    U64 identity=0;
    unsigned version=0;
    U8 active=0;
    U32 task=0, guid=0; // preserve signed int32 wire bits
};
struct Result {
    Status status=Status::Malformed;
    std::string reason;
    std::size_t offset=0;
    U32 appFormat=0;
    unsigned identityBytes=0;
    std::vector<Record> records; // atomic publication after full validation
};
inline Result decode(const Metadata& metadata,Key key,std::span<const U8> bytes) {
    Result r;
    native_pollution::Cursor file{bytes};
    using native_pollution::Failure;
    try {
        // Metadata selector AppLoadCity @ 0x100045689; key @ 0x1001e9168.
        // Missing/failed/wrong metadata and unsupported formats are adapter domain.
        if(!same(key,tutorialKey)) throw Failure{Status::Unsupported,"requires exact tutorial TGI aa3836fb:299b2d1b:00000000"};
        if(!metadata.present || !metadata.decoded ||
           !same({metadata.type,metadata.group,metadata.instance},metadataKey) || metadata.bytes.size()!=4)
            throw Failure{Status::Unsupported,"requires decoded exact four-byte application metadata TGI"};
        native_pollution::Cursor mc{metadata.bytes};
        r.appFormat=static_cast<U32>(mc.get(4));
        if(r.appFormat<0x3e || r.appFormat>0x42)
            throw Failure{Status::Unsupported,"application format outside adapter domain 0x3e..0x42"};
        r.identityBytes=r.appFormat<=0x41?4:8; // validated widths only
        std::vector<Record> pending;
        while(file.remaining()) {
            // Resource bound is explicit adapter policy, not original behavior.
            if(pending.size()==4096) throw Failure{Status::Unsupported,"record count exceeds adapter limit 4096"};
            const auto base=file.pos;
            Record s;
            s.size=static_cast<U32>(file.get(4));
            if(s.size<8+r.identityBytes+2 || s.size>bytes.size()-base)
                throw Failure{Status::Malformed,"record size outside available framing bounds"};
            native_pollution::Cursor c{bytes.subspan(base,s.size)};
            c.get(4); s.storedCRC=static_cast<U32>(c.get(4));
            // GetCRC @ 0x1003cfeb6: full serialized identity and body, cap 0x3d090.
            s.computedCRC=native_pollution::recordCRC(c.bytes);
            if(s.storedCRC!=s.computedCRC) throw Failure{Status::Malformed,"record CRC mismatch"};
            s.identity=c.get(r.identityBytes);
            // Tutorial Read @ 0x10005fc1a / Write @ 0x10005fba6.
            // Class ID belongs to the TGI, never an extra body word.
            s.version=static_cast<unsigned>(c.get(2));
            if(s.version!=2) throw Failure{Status::Unsupported,"tutorial body version outside adapter v2 domain"};
            if(c.remaining()!=9) throw Failure{Status::Malformed,"tutorial v2 body must be exactly 11 bytes"};
            s.active=static_cast<U8>(c.get(1));
            s.task=static_cast<U32>(c.get(4));
            s.guid=static_cast<U32>(c.get(4));
            pending.push_back(s);
            file.pos=base+s.size;
        }
        if(pending.empty()) throw Failure{Status::Unsupported,"empty record sequence outside adapter domain"};
        r.records=std::move(pending); r.status=Status::Ok; r.reason="complete";
    } catch(const Failure& f) {r.status=f.status;r.reason=f.reason;}
    r.offset=file.pos; // failed record: position after its size word; success: complete size
    return r;
}
}
