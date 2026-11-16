#define OPENSC4_PLACEMENT_CHECKPOINT_INTEGRATION 1
#include "saved_fixture_binding_syntax_stub.cpp"
#include "placement_checkpoint_fragment.inc"
bool placement_checkpoint_rpc_syntax() { return placement_checkpoint::inspectplacementcheckpoint() && placement_checkpoint::attemptplacementcheckpoint(); }
