// SYNTHETIC diagnostic framing checks; not game-produced evidence or a native writer.
#include "native_pollution_snapshot.h"
#include <cstdio>
#include <cstdlib>
using namespace native_pollution;
namespace {
int checks=0;
void require(bool v,const char* what){++checks;if(!v){std::fprintf(stderr,"FAIL %s\n",what);std::exit(1);}}
void put(std::vector<U8>& b,U64 v,unsigned n){for(unsigned i=0;i<n;++i)b.push_back(static_cast<U8>(v>>(8*i)));}
void patch(std::vector<U8>& b,std::size_t p,U64 v,unsigned n){for(unsigned i=0;i<n;++i)b[p+i]=static_cast<U8>(v>>(8*i));}
void seal(std::vector<U8>& b){patch(b,0,b.size(),4);patch(b,4,recordCRC(b),4);}
Metadata meta(std::vector<U8>& b){return {true,true,0xea3ff4f5,0xea3ff506,0xea3ff4f5,b};}
std::vector<U8> fixture(unsigned width){
    std::vector<U8> b(8);
    put(b,width==8?0xfedcba9800000000ULL:0x87654321, width); put(b,4,2);
    for(unsigned i=0;i<8;++i){
        U64 id=i==0?(width==8?0x1234567800000000ULL:7):(i==1?0:0x12345670+i);
        put(b,id,width);if(id)put(b,0xaabb0000+i,4);
    }
    put(b,0x10203040,4);put(b,0x50607080,4);
    for(unsigned i=0;i<4;++i){put(b,2,4);put(b,0,width);put(b,width==8?0xabcdef0100000000ULL+i:0x12340000+i,width);put(b,0xcafe0000+i,4);}
    for(unsigned i=0;i<10;++i)put(b,0x200+i,4);
    put(b,0x1234,2);put(b,0x238,4);put(b,0x23c,4);put(b,0x7fc01234,4);
    put(b,0xa5,1);put(b,0x248,4);put(b,0x8877665544332211ULL,8);put(b,0xffeeddccbbaa9988ULL,8);put(b,0x2c8,4);
    for(unsigned i=0;i<14;++i)put(b,0x8000000000000000ULL+i,8);
    put(b,0x2c4,4);put(b,0x2c0,4);put(b,0x2cc,4);put(b,0x2e4,4);
    put(b,0x11,1);put(b,0x22,1);put(b,0x80,1);
    for(unsigned i=0;i<5;++i)put(b,0x900+i,4);
    put(b,3,4);put(b,0,2);put(b,0xffff,2);put(b,0x1234,2);
    for(unsigned i=0;i<3;++i)put(b,0xa00+i,4);
    put(b,0x81,1);put(b,0x82,1);put(b,0x83,1);put(b,0x84,1);
    put(b,0xff800000,4);put(b,0x80000000,4);put(b,0xffffffff,4);
    put(b,width==8?0xfacefeed00000000ULL:0x76543210,width);put(b,0xabcdefab,4);
    seal(b);return b;
}
}
int main(){
    for(unsigned width:{4u,8u}){
        std::vector<U8> m;put(m,width==4?0x41:0x42,4);auto md=meta(m);
        auto b=fixture(width);auto r=decode(md,b);
        require(r.status==Status::Ok,"synthetic exact consumption");require(r.offset==b.size(),"header/body offsets");
        const auto& s=r.records.at(0);
        require(s.identity==(width==8?0xfedcba9800000000ULL:0x87654321),"record upper bits");
        require(s.grids78[0].identity==(width==8?0x1234567800000000ULL:7) && s.grids78[0].classID==0xaabb0000,"full null test and class presence");
        require(s.grids78[1].identity==0 && s.grids78[1].classID==0,"null consumes no classID");
        const std::vector<Ref>* lists[]={&s.listC0,&s.listD8,&s.listF0,&s.list108};
        for(unsigned i=0;i<4;++i)require(lists[i]->size()==2 && (*lists[i])[1].identity==(width==8?0xabcdef0100000000ULL+i:0x12340000+i) && (*lists[i])[1].classID==0xcafe0000+i,"every list identities/classes");
        require(s.simManager.identity==(width==8?0xfacefeed00000000ULL:0x76543210) && s.simManager.classID==0xabcdefab,"final ref preserves upper bits");
        require(s.random.prefix[2]==0x80 && s.random.indices==std::vector<unsigned short>({0,0xffff,0x1234}) && s.random.tail[2]==0xa02,"iterator raw layout");
        require(s.scalars[0].bits==0x10203040 && s.scalars[15].bits==0x7fc01234 && s.scalars[16].bits==0xa5,"scalar raw bits");
        for(std::size_t n=0;n<b.size();++n)require(decode(md,std::span<const U8>(b.data(),n)).status!=Status::Ok,"every short prefix rejected");
        // Resealed truncated records reach body bounds rather than only outer-size/CRC gates.
        for(std::size_t n=8+width;n<b.size();++n){
            std::vector<U8> shortBody(b.begin(),b.begin()+n);seal(shortBody);
            require(decode(md,shortBody).status!=Status::Ok,"every resealed body prefix rejected");
        }
        require(s.scalars.size()==46,"all scalar slots retained");
        require(s.scalars[34].bits==0x800000000000000dULL && s.scalars[35].bits==0x2c4 && s.scalars[36].bits==0x2c0,"wide scalar tail and 2C4-before-2C0 order");
        auto wrong=m;wrong[0]=width==4?0x42:0x41;require(decode(meta(wrong),b).status!=Status::Ok,"wrong metadata width rejected");
        auto bad=b;bad[8]^=1;require(decode(md,bad).reason=="record CRC mismatch","identity participates in CRC");
        bad=b;patch(bad,8+width,3,2);seal(bad);require(decode(md,bad).status==Status::Unsupported,"body v3 explicit unsupported");
        bad=b;bad.push_back(0);seal(bad);require(decode(md,bad).reason=="unconsumed pollution body bytes","complete body required");
        // First list count offset: eight refs, seven nonnull, two scalars.
        const std::size_t list=8+width+2+8*width+7*4+8;
        bad=b;patch(bad,list,0x989680,4);seal(bad);require(decode(md,bad).status==Status::Malformed,"list binary limit");
        bad=b;patch(bad,list,10000,4);seal(bad);require(decode(md,bad).status==Status::Malformed,"list bounded by bytes");
        // Iterator count: tail is 12+4+12+width+4 bytes after indices.
        const auto countOffset=b.size()-(12+4+12+width+4)-6-4;
        bad=b;patch(bad,countOffset,0xffffffff,4);seal(bad);require(decode(md,bad).reason=="RandomIterator count outside diagnostic bounds","iterator bounded before allocation");
        bad=b;bad.insert(bad.end(),b.begin(),b.end());require(decode(md,bad).records.size()==2,"multiple records complete");
        md.present=false;require(decode(md,b).status==Status::Unsupported,"missing metadata adapter domain");
        md=meta(m);md.decoded=false;require(decode(md,b).status==Status::Unsupported,"failed metadata decode");
        md=meta(m);md.bytes=std::span<const U8>(m.data(),3);require(decode(md,b).status==Status::Unsupported,"short metadata");
        md=meta(m);md.group^=1;require(decode(md,b).status==Status::Unsupported,"exact metadata key");
    }
    for(U32 app:{0x3du,0x43u}){std::vector<U8> m;put(m,app,4);require(decode(meta(m),fixture(8)).status==Status::Unsupported,"app range");}
    for(U32 app:{0x3eu,0x3fu,0x40u}){std::vector<U8> m;put(m,app,4);require(decode(meta(m),fixture(4)).status==Status::Ok,"supported legacy app range");}
    std::vector<U8> crc(8+0x3d090+1,0x42);auto a=recordCRC(crc);crc[8+0x3d090]^=1;
    require(a==recordCRC(crc),"CRC excludes first byte beyond cap");crc[8+0x3d090-1]^=1;require(a!=recordCRC(crc),"CRC includes cap final byte");
    // Published CRC-32/MPEG-2 known vector: non-reflected poly, all-ones init, no xorout.
    const U8 known[]={'1','2','3','4','5','6','7','8','9'};
    require(opensc4::ComputeMaxisCRC32(known,9)==0x0376e6e7,"original CRC algorithm independent known vector");
    std::printf("SYNTHETIC checks=%d fail=0\n",checks);
}
