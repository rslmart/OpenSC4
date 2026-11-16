#include "opensc4/city.h"
// Standalone tests only. Recompile constructor for new header layout; never link
// the stale archive city.o. Full application constructor/build is conductor-owned.
namespace opensc4 { cSC4City::cSC4City() = default; }
