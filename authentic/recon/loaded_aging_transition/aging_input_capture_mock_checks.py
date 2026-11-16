"""Extract actual lifecycle and typed helpers; mock external acquisition/native calls only."""
from pathlib import Path
r=Path(__file__).resolve().parent
s=(r/'placement_reload_fragment.inc').read_text()
a=(r/'aging_input_capture_fragment.inc').read_text()
def function(text,name):
 start=text.rfind('static ',0,text.index(name+'(')); brace=text.index('{',start); i=brace+1; depth=1
 while depth:
  depth+=(text[i]=='{')-(text[i]=='}');i+=1
 return text[start:i]
out='''#include "aging_input_capture_syntax_stub.cpp"
#include <cassert>
#include <stdexcept>
static unsigned checks=0,ends=0,aging_ends=0;
static unsigned reload_complete=99,aging_complete=99;
static void check(bool b) { assert(b); ++checks; }
static void logf_(const char* f,...) {
 const bool reload=std::strstr(f,"PLACEMENT_RELOAD END ")!=nullptr;
 const bool aging=std::strstr(f,"AGING_INPUT_CAPTURE END ")!=nullptr;
 if(reload || aging) {
  va_list args;va_start(args,f);const unsigned complete=va_arg(args,unsigned);va_end(args);
  if(reload){++ends;reload_complete=complete;}
  if(aging){++aging_ends;aging_complete=complete;}
 }
}
static uint64_t get_app() { std::abort(); }
ScopedRZString::ScopedRZString() { std::memset(buf,0,sizeof buf); }
ScopedRZString::~ScopedRZString() {}
namespace mock {
using namespace placement_reload;
static placement_reload::Context* context=nullptr;
static std::atomic<bool> attempted{false},ended{false};
static bool terminal_input_mode=false;
static bool allocation_throws=false;
static placement_reload::Context* allocate(bool mode) { if(allocation_throws) throw std::bad_alloc(); return new placement_reload::Context(mode); }
static unsigned queued=0,delayed=0,captures=0; static bool capture=true,throws=false;
static dispatch_function_t callback=nullptr,late_callback=nullptr;
static bool pthread_main_np() { return true; }
static void sample(placement_reload::Context&,Sample& s,const char*) { ++captures; s.full=s.core_ok=s.paths_ok=s.files_ok=s.tutorial_ok=s.inventory.complete=s.inventory.identity_complete=capture; s.profile=fb::FixtureKind::DiagnosticPlaced; s.inputs.complete=capture; }
static dispatch_queue_t dispatch_get_main_queue() { return nullptr; }
static dispatch_time_t dispatch_time(dispatch_time_t,int64_t d) { check(d==500LL*NSEC_PER_MSEC); return 0; }
static void dispatch_async_f(dispatch_queue_t,void*,dispatch_function_t cb) { ++queued; callback=cb; if(throws) throw std::runtime_error("queue"); }
static void dispatch_after_f(dispatch_time_t,dispatch_queue_t,void*,dispatch_function_t cb) { ++delayed; late_callback=cb; if(throws) throw std::runtime_error("queue"); }
'''
for name in ('end','later','begin','queue_inspection','inspectplacedcheckpointreload','inspectloadedaginginputs'):
 body=function(s,name)
 for call in ('sample','dispatch_async_f','dispatch_after_f','dispatch_get_main_queue','dispatch_time','end','queue_inspection'):
  body=body.replace(call+'(', 'mock::'+call+'(') if call!=name else body
 body=body.replace('new Context(input_mode)','allocate(input_mode)')
 out+=body+'\n'
out+='''}
namespace typed_mock {
static bool returned=false,write=false;
static uint32_t output=0;
template<class T> static bool native(uint64_t h,uint32_t id,T& v) {
 check(h==123 && id==456);
 if(write) std::memcpy(&v,&output,4);
 return returned;
}
'''
body=function(a,'typed')
import re
body=re.sub(r'original<bool\(\*\)\(uint64_t,uint32_t,(?:int32_t|uint32_t|float)&\)>\(0x[0-9a-f]+\)', 'native', body)
out+=body+'\n}\n'
out+='''int main() {
 namespace m=mock; namespace f=fixture_placement_bindings;
 f::busy=false; check(m::inspectloadedaginginputs()); check(m::context->input_mode && m::context->owner.reload_allowed() && f::busy);
 check(!m::inspectplacedcheckpointreload() && m::queued==1);
 m::capture=false; m::callback(nullptr);check(m::ended && m::delayed==0 && m::captures==1);
 unsigned old=ends;m::end(true,"again");check(ends==old);
 m::ended=false;m::capture=true;m::begin(nullptr);check(m::delayed==1 && !m::ended);
 m::late_callback(nullptr);check(m::ended && m::captures==3);
 m::ended=false;m::throws=true;m::begin(nullptr);check(m::ended && f::busy);
 m::attempted=false;check(!m::inspectloadedaginginputs() && m::attempted);
 f::busy=false;m::attempted=false;m::ended=false;
 check(!m::inspectloadedaginginputs() && m::context && m::ended && f::busy);
 f::busy=false;m::attempted=false;m::ended=false;m::context=nullptr;m::allocation_throws=true;
 const unsigned before_reload=ends,before_aging=aging_ends,before_queue=m::queued,before_delayed=m::delayed;
 check(!m::inspectloadedaginginputs() && m::ended && !m::context && f::busy);
 check(m::attempted && m::terminal_input_mode && m::queued==before_queue && m::delayed==before_delayed);
 check(ends==before_reload+1 && aging_ends==before_aging+1 && reload_complete==0 && aging_complete==0);
 m::end(true,"repeat_after_allocation_failure");
 check(ends==before_reload+1 && aging_ends==before_aging+1);
 m::allocation_throws=false;
 // Old API selects old mode through the SAME actual queue helper.
 f::busy=false;m::attempted=false;m::ended=false;m::throws=false;
 check(m::inspectplacedcheckpointreload() && !m::context->input_mode && !m::terminal_input_mode);
 const unsigned old_mode_aging=aging_ends,old_mode_reload=ends;
 m::capture=false;m::callback(nullptr);
 check(ends==old_mode_reload+1 && aging_ends==old_mode_aging && reload_complete==0);
 m::end(true,"repeat_old_mode");check(ends==old_mode_reload+1 && aging_ends==old_mode_aging);
 // Old-mode allocation failure also emits only its reload terminal.
 f::busy=false;m::attempted=false;m::ended=false;m::context=nullptr;m::allocation_throws=true;
 const unsigned old_alloc_aging=aging_ends,old_alloc_reload=ends;
 check(!m::inspectplacedcheckpointreload() && m::ended && m::attempted && f::busy && !m::terminal_input_mode);
 check(ends==old_alloc_reload+1 && aging_ends==old_alloc_aging && reload_complete==0);
 for(unsigned kind=0;kind<3;++kind) {
  uint32_t bits=0x42c60000;typed_mock::returned=false;typed_mock::write=false;
  check(!typed_mock::typed(123,456,kind,bits) && bits==0x42c60000);
  typed_mock::returned=true;typed_mock::write=true;typed_mock::output=0;
  check(typed_mock::typed(123,456,kind,bits) && bits==0);
  typed_mock::returned=false;bits=0;
  check(!typed_mock::typed(123,456,kind,bits) && bits==0);
 }
 std::printf("aging_input_capture extracted lifecycle/typed checks=%u native_acquisition=mocked live_proof=0 allocation_failure_injection=1\\n",checks);
}
'''
(r/'aging_input_capture_mock_checks.cpp').write_text(out)
