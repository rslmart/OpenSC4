// Compile-only host declarations. Never link or execute this stub.
#include <cstdint>
#include <cstdarg>
#include <initializer_list>
static intptr_t g_slide=0;
static void logf_(const char*,...) __attribute__((format(printf,1,2)));
static uint64_t get_app();
struct ScopedRZString {
    alignas(8) uint8_t buf[64];
    ScopedRZString();
    ~ScopedRZString();
};
#include "fixture_placement_fragment.inc"
#include "fixture_placement_bindings.inc"
#include "tutorial_inspect_fragment.inc"
#include "tutorial_open_fragment.inc"
bool tutorial_open_parser_syntax_stub() { return tutorial_open::opentutorialexit(); }
#include "tutorial_finish_fragment.inc"
bool tutorial_finish_parser_syntax_stub() { return tutorial_finish::finishtutorial(); }

#include "tutorial_checkpoint_fragment.inc"
bool checkpoint_parser_syntax_stub() { return tutorial_checkpoint::inspectsavebindings() && tutorial_checkpoint::finishtutorialcheckpoint(); }

#include "tutorial_reload_fragment.inc"
bool reload_parser_syntax_stub() { return tutorial_reload::inspectcheckpointreload(); }
