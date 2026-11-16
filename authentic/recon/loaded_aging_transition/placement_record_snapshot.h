#pragma once
// Offline diagnostic only; no factory, inheritance, body mapping, or writer.
#include "native_tutorial_snapshot.h"
#include <optional>
#include <new>
namespace placement_record {
using namespace native_tutorial;
using native_pollution::Ref;
inline constexpr Key buildingKey{0xa9bd882d,0x299b2d1b,0};
inline constexpr std::size_t maxRecords=100000, maxStreamBytes=64*1024*1024;
struct Record {
    std::size_t offset=0, headerSize=16, bodyOffset=0, bodySize=0;
    U64 identity=0;
    U32 size=0, storedCRC=0, computedCRC=0;
    unsigned rawBodyVersion=0;
    std::vector<U8> body;
};
struct Result {
    Status status=Status::Malformed;
    std::string reason;
    std::size_t consumed=0, recordCount=0;
    std::optional<Record> selected; // published only after complete validation
};
inline Result scan(const Metadata& md,Ref target,Key key,std::span<const U8> bytes) {
    Result r;
    using native_pollution::Failure;
    using native_pollution::Cursor;
    try {
        // Metadata selector @ 0x100045689; EXACT 0x42 is this adapter's policy.
        if(!md.present || !md.decoded || md.bytes.size()!=4 ||
           !same({md.type,md.group,md.instance},metadataKey))
            throw Failure{Status::Unsupported,"requires exact decoded four-byte application metadata"};
        Cursor mc{md.bytes};
        if(mc.get(4)!=0x42) throw Failure{Status::Unsupported,"requires application format 0x42 (66)"};
        if(target.identity==0 || target.classID!=buildingKey.type || !same(key,buildingKey))
            throw Failure{Status::Unsupported,"requires nonnull full64 reference of class a9bd882d and exact class-stream TGI"};
        // Resource limits are diagnostic adapter policy, not original rejection rules.
        if(bytes.size()>maxStreamBytes) throw Failure{Status::Unsupported,"class stream exceeds adapter 64 MiB limit"};
        std::optional<Record> pending;
        Cursor file{bytes};
        while(file.remaining()) {
            if(r.recordCount==maxRecords) throw Failure{Status::Unsupported,"record count exceeds adapter 100000 limit"};
            const auto base=file.pos;
            // LoadClassObjects @ 0x1003d0e06: size32, CRC32, identity64 in format42.
            const auto size=static_cast<U32>(file.get(4));
            if(size<18 || size>bytes.size()-base)
                throw Failure{Status::Malformed,"record size outside available framing bounds"};
            Cursor c{bytes.subspan(base,size)};
            c.get(4); const auto stored=static_cast<U32>(c.get(4));
            const auto computed=native_pollution::recordCRC(c.bytes); // @ 0x1003cfeb6
            if(stored!=computed) throw Failure{Status::Malformed,"record CRC mismatch"};
            const auto identity=c.get(8);
            if(identity==target.identity) {
                if(pending) throw Failure{Status::Malformed,"duplicate full64 selected identity"};
                const auto version=static_cast<unsigned>(c.get(2));
                // Building Read @ 0x10019b350 supports v2. Only the raw first word
                // is examined here; requiring v2 is diagnostic policy, not an
                // assertion about original rejection of other versions/bodies.
                if(version!=2) throw Failure{Status::Unsupported,"selected raw body version outside diagnostic v2 policy"};
                Record s;
                s.offset=base; s.bodyOffset=base+16; s.bodySize=size-16;
                s.identity=identity; s.size=size; s.storedCRC=stored; s.computedCRC=computed;
                s.rawBodyVersion=version;
                const auto body=c.bytes.subspan(16);
                s.body.assign(body.begin(),body.end()); pending=std::move(s);
            }
            file.pos=base+size; r.consumed=file.pos; ++r.recordCount;
        }
        if(!pending) throw Failure{Status::Malformed,"missing full64 selected identity"};
        r.selected=std::move(pending);r.status=Status::Ok;r.reason="complete";
    } catch(const Failure& f) {r.status=f.status;r.reason=f.reason;}
      catch(const std::bad_alloc&) {r.status=Status::Unsupported;r.reason="diagnostic allocation failed";}
    return r;
}
}
