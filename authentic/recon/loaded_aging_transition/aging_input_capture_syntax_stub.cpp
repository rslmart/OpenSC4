#define OPENSC4_PLACEMENT_RELOAD_INTEGRATION 1
#include "placement_checkpoint_syntax_stub.cpp"
#include "aging_input_capture_fragment.inc"
#include "placement_reload_fragment.inc"
bool aging_input_rpc_syntax() { return placement_reload::inspectloadedaginginputs(); }
