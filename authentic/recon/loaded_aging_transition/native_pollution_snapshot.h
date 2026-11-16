#pragma once
// Read-only G3.3 diagnostic domain; no object resolution and no writer.
// Reviewed framing: LoadClassObjects @ 0x1003d0e06; references @ 0x1003d03d2.
#include "opensc4/gzcom.h"
#include <array>
#include <span>
#include <string>
#include <vector>
#include <algorithm>
#include <stdexcept>
namespace native_pollution {
using U8 = opensc4::Uint8;
using U32 = opensc4::Uint32;
using U64 = opensc4::Uint64;
struct Ref { U64 identity = 0; U32 classID = 0; };
struct Scalar { const char* name; unsigned bytes; U64 bits; };
struct Iterator {
    std::array<U8,3> prefix{}; // third byte retained raw, not normalized to bool
    std::array<U32,5> values{};
    std::vector<unsigned short> indices;
    std::array<U32,3> tail{};
};
struct Snapshot {
    U32 size=0, storedCRC=0, computedCRC=0;
    U64 identity=0;
    unsigned version=0;
    std::array<Ref,4> grids78{}, grids98{};
    std::vector<Ref> listC0, listD8, listF0, list108;
    std::vector<Scalar> scalars; // explicit serialized order, raw IEEE/integer/bool bits
    Iterator random;
    Ref simManager;
};
enum class Status { Ok, Unsupported, Malformed };
struct Result {
    Status status=Status::Malformed;
    std::string reason;
    std::size_t offset=0;
    unsigned identityBytes=0;
    std::vector<Snapshot> records; // only published after complete successful decode
};
struct Metadata {
    bool present=false, decoded=false;
    U32 type=0, group=0, instance=0;
    std::span<const U8> bytes;
};
struct Failure { Status status; const char* reason; };
struct Cursor {
    std::span<const U8> bytes;
    std::size_t pos=0;
    std::size_t remaining() const { return bytes.size()-pos; }
    U64 get(unsigned n) {
        if (n>remaining()) throw Failure{Status::Malformed,"short field"};
        U64 v=0;
        for(unsigned i=0;i<n;++i) v |= U64(bytes[pos++]) << (8*i);
        return v;
    }
};
// GetCRC @ 0x1003cfeb6: original Maxis CRC, offset 8, capped at 0x3d090.
inline U32 recordCRC(std::span<const U8> record) {
    if(record.size()<8) throw Failure{Status::Malformed,"short CRC header"};
    return opensc4::ComputeMaxisCRC32(record.data()+8,
                                     std::min<std::size_t>(record.size()-8,0x3d090));
}
// SC4ReadGZSerializable @ 0x100043267 -> 0x1003d03d2; full nonzero test.
// Bool loadSingleObject argument is irrelevant here: this adapter resolves nothing.
inline Ref readRef(Cursor& c, unsigned width) {
    Ref r; r.identity=c.get(width);
    if(r.identity!=0) r.classID=static_cast<U32>(c.get(4));
    return r;
}
inline void readList(Cursor& c,unsigned width,std::vector<Ref>& out) {
    const U64 count=c.get(4);
    // Binary list limit; remaining-byte bound is an adapter-domain restriction.
    if(count>0x98967f || count>c.remaining()/width)
        throw Failure{Status::Malformed,"reference list count outside diagnostic bounds"};
    out.reserve(static_cast<std::size_t>(count));
    for(U64 i=0;i<count;++i) out.push_back(readRef(c,width));
}
// RandomIterator operator>> @ 0x1000cea88, exact wire layout.
inline void readIterator(Cursor& c,Iterator& it) {
    for(auto& v:it.prefix) v=static_cast<U8>(c.get(1));
    for(auto& v:it.values) v=static_cast<U32>(c.get(4));
    const U64 count=c.get(4);
    // Diagnostic bounds only; do not claim original malformed-input behavior.
    if(c.remaining()<12 || count>(c.remaining()-12)/2)
        throw Failure{Status::Malformed,"RandomIterator count outside diagnostic bounds"};
    it.indices.reserve(static_cast<std::size_t>(count));
    for(U64 i=0;i<count;++i) it.indices.push_back(static_cast<unsigned short>(c.get(2)));
    for(auto& v:it.tail) v=static_cast<U32>(c.get(4));
}
// Pollution Read @ 0x100096588. Adapter domain deliberately supports body v4 only.
inline void readBody(Cursor& c,unsigned width,Snapshot& s) {
    s.version=static_cast<unsigned>(c.get(2));
    if(s.version!=4) throw Failure{Status::Unsupported,"pollution body version outside diagnostic v4 domain"};
    for(auto& r:s.grids78) r=readRef(c,width);
    for(auto& r:s.grids98) r=readRef(c,width);
    auto scalar=[&](const char* name,unsigned n){s.scalars.push_back({name,n,c.get(n)});};
    scalar("B8",4); scalar("BC",4);
    readList(c,width,s.listC0);
    readList(c,width,s.listD8);
    readList(c,width,s.listF0);
    readList(c,width,s.list108);
    scalar("208",4); scalar("20C",4); scalar("210",4);
    scalar("214",4); scalar("218",4); scalar("21C",4);
    scalar("220",4); scalar("224",4); scalar("228",4); scalar("22C",4);
    // v4 has no legacy GZString here.
    scalar("234",2); scalar("238",4); scalar("23C",4); scalar("240",4);
    scalar("244",1); scalar("248",4); scalar("2D0",8); scalar("2D8",8);
    scalar("2C8",4);
    scalar("288",8); scalar("290",8); scalar("298",8); scalar("2A0",8);
    scalar("2A8",8); scalar("2B0",8); scalar("2B8",8);
    scalar("250",8); scalar("258",8); scalar("260",8); scalar("268",8);
    scalar("270",8); scalar("278",8); scalar("280",8);
    scalar("2C4",4); scalar("2C0",4); scalar("2CC",4); scalar("2E4",4);
    readIterator(c,s.random);
    scalar("328",1); scalar("329",1); scalar("32A",1); scalar("32B",1);
    // v4 has no legacy Uint32 here.
    scalar("348",4); scalar("34C",4); scalar("350",4);
    s.simManager=readRef(c,width);
    if(c.remaining()!=0) throw Failure{Status::Malformed,"unconsumed pollution body bytes"};
}
inline Result decode(const Metadata& metadata,std::span<const U8> bytes) {
    Result result;
    Cursor file{bytes};
    std::size_t base=0;
    try {
        // AppLoadCity @ 0x100045689..694; exact metadata key @ 0x1001e9168.
        // Present, successful exact-four-byte metadata is THIS ADAPTER's domain.
        // Original missing-metadata fallback is intentionally not reproduced.
        if(!metadata.present || !metadata.decoded || metadata.type!=0xea3ff4f5 ||
           metadata.group!=0xea3ff506 || metadata.instance!=0xea3ff4f5 || metadata.bytes.size()!=4)
            throw Failure{Status::Unsupported,"requires decoded exact metadata TGI ea3ff4f5:ea3ff506:ea3ff4f5"};
        Cursor mc{metadata.bytes}; const U64 app=mc.get(4);
        if(app<0x3e || app>0x42) throw Failure{Status::Unsupported,"application format outside 0x3e..0x42"};
        result.identityBytes=app<=0x41?4:8;
        std::vector<Snapshot> records;
        while(file.remaining()!=0) {
            base=file.pos;
            Snapshot s;
            s.size=static_cast<U32>(file.get(4));
            if(s.size<8+result.identityBytes || s.size>bytes.size()-base)
                throw Failure{Status::Malformed,"record size outside diagnostic bounds"};
            Cursor record{bytes.subspan(base,s.size)};
            record.get(4); s.storedCRC=static_cast<U32>(record.get(4));
            s.computedCRC=recordCRC(record.bytes);
            if(s.computedCRC!=s.storedCRC) throw Failure{Status::Malformed,"record CRC mismatch"};
            s.identity=record.get(result.identityBytes);
            // Keep error offset exact within the body, including failures.
            try { readBody(record,result.identityBytes,s); }
            catch(...) {file.pos=base+record.pos; throw;}
            records.push_back(std::move(s));
            file.pos=base+record.bytes.size();
        }
        if(records.empty()) throw Failure{Status::Malformed,"empty pollution subfile"};
        result.records=std::move(records); result.status=Status::Ok; result.reason="complete";
    } catch(const Failure& f) {result.status=f.status; result.reason=f.reason;}
    result.offset=file.pos;
    return result;
}
}
