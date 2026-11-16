#include "aging_input_capture_syntax_stub.cpp"
#include <cassert>
#include <cstdarg>
static unsigned checks=0;
static void check(bool b) { assert(b); ++checks; }
static void logf_(const char*,...) {}
static uint64_t get_app() { std::abort(); }
ScopedRZString::ScopedRZString() { std::memset(buf,0,sizeof buf); }
ScopedRZString::~ScopedRZString() {}
struct Memory {
 std::vector<unsigned char> bytes=std::vector<unsigned char>(100000);
 bool readable=true;
 bool operator()(uint64_t p,void* out,size_t n) const { if(!readable || p>bytes.size() || n>bytes.size()-p) return false; std::memcpy(out,bytes.data()+p,n); return true; }
 template<class T> void set(size_t p,T n) { std::memcpy(bytes.data()+p,&n,sizeof n); }
 void curve(uint64_t b,uint64_t e,uint64_t cap,int32_t count) { set(128,b);set(136,e);set(144,cap);set(152,count); }
};
int main() {
 namespace a=aging_input_capture; using a::Fault;
 Memory m;
 auto curve=[&](Fault want) { a::Curve c; check(a::curve(m,128,c)==want); return c; };
 m.curve(0,0,0,0); check(curve(Fault::None).words.empty());
 m.curve(512,512,512,0); check(curve(Fault::None).count==0);
 for(int count:{1,2,4096}) { m.curve(512,512+count*20,512+count*20,count);
  for(int i=0;i<count*5;++i) m.set(512+i*4,uint32_t(i%3==0?0x7fc12345:i%3==1?0x80000000:0));
  auto c=curve(Fault::None); check(c.words.size()==size_t(count*5)); check(c.words[0]==0x7fc12345 && c.words[1]==0x80000000 && c.words[2]==0);
 }
 m.curve(512,512+4097*20,512+4097*20,4097); curve(Fault::Limit);
 m.curve(512,532,532,-1); curve(Fault::NegativeCount);
 m.curve(0,532,532,1); curve(Fault::NullMismatch);
 m.curve(512,532,532,2); curve(Fault::CountSpan);
 m.curve(512,531,532,1); curve(Fault::Alignment);
 m.curve(512,532,531,1); curve(Fault::Order);
 m.curve(513,533,533,1); curve(Fault::Alignment);
 m.curve(99992,100012,100012,1); curve(Fault::Unreadable);
 m.readable=false; curve(Fault::Unreadable); m.readable=true;
 a::Curve c; check(a::curve(m,UINT64_MAX-8,c)==Fault::Overflow);
 uint64_t out=9; check(!a::add(UINT64_MAX,1,out) && out==9);
 check(!a::alias(0,0) && a::alias(4,4) && !a::alias(4,5));
 for(unsigned off:{0x10u,0x58u}) {
  m.set(128+off,uint64_t(512)); m.set(136+off,uint64_t(544));
  m.set(512,uint64_t(1)); m.set(520,uint64_t(777)); m.set(528,uint64_t(2)); m.set(536,uint64_t(888));
  a::Entries e; check(a::entries(m,128,off,e)==Fault::None && e.raw[1]==777 && e.raw[3]==888);
  m.set(528,uint64_t(0)); check(a::entries(m,128,off,e)==Fault::Order);
  m.set(136+off,uint64_t(543)); check(a::entries(m,128,off,e)==Fault::Alignment);
 }
 using namespace placement_reload;
 Sample x; x.full=x.core_ok=x.paths_ok=x.files_ok=x.tutorial_ok=true;
 x.profile=fb::FixtureKind::DiagnosticPlaced; x.inventory.complete=x.inventory.identity_complete=true;
 x.inventory.guarded_descriptor=1; x.inventory.occurrences.emplace_back(); auto& v=x.inventory.occurrences.back(); v.offset=0xc0;
 auto& o=v.value; o.complete=o.lot.complete=true; o.lot.lot=2; o.lot.descriptor=1; o.lot.rect={58,66,63,72};
 o.tgi[0]=0x6534284a;o.tgi[1]=0xc8dbccba;o.tgi[2]=0x1f4b0000;
 for(auto& p:o.properties) { p.complete=true;p.status=fb::PropertyStatus::Missing; }
 x.inputs.complete=true; x.inputs.identity={1,2,3,4,5}; x.inputs.data={0,0x7fc12345,0x80000000,42,9,1};
 Sample y=x; check(inputs_complete(x,y));
 for(size_t i=0;i<y.inputs.identity.size();++i) { ++y.inputs.identity[i]; check(!inputs_complete(x,y)); --y.inputs.identity[i]; }
 for(size_t i=0;i<y.inputs.data.size();++i) { ++y.inputs.data[i]; check(!inputs_complete(x,y)); --y.inputs.data[i]; }
 for(auto p:{&y.core.date}) { ++*p; check(!inputs_complete(x,y)); --*p; }
 for(unsigned i=0;i<3;++i) { ++y.core.pauses[i];check(!inputs_complete(x,y));--y.core.pauses[i]; }
 y.inputs.complete=false; check(!inputs_complete(x,y) && complete(x,y));
 // Regression: old sample path could accept state changed during native queries.
 a::Snapshot post; State before{},after{}; before.date=2455319;before.pauses[0]=3;after=before;
 bool can_read=true,identity=true;unsigned identity_calls=0;
 auto reader=[&](State& output){output=after;return can_read;};
 auto binding=[&](const State&){++identity_calls;return identity;};
 post.complete=true;check(a::finish_capture(post,before,reader,binding,"HOST") && post.complete);
 for(unsigned i=0;i<3;++i){after=before;++after.pauses[i];post.complete=true;unsigned calls=identity_calls;
  check(!a::finish_capture(post,before,reader,binding,"HOST") && !post.complete && identity_calls==calls);}
 after=before;++after.date;post.complete=true;check(!a::finish_capture(post,before,reader,binding,"HOST") && !post.complete);
 after=before;can_read=false;post.complete=true;check(!a::finish_capture(post,before,reader,binding,"HOST") && !post.complete);
 can_read=true;identity=false;post.complete=true;check(!a::finish_capture(post,before,reader,binding,"HOST") && !post.complete);
 identity=true;after=before;++after.city;post.complete=true;check(!a::finish_capture(post,before,reader,binding,"HOST") && !post.complete);
 std::printf("aging_input_capture actual helper checks=%u host_mock=1 live_proof=0\n",checks);
}
