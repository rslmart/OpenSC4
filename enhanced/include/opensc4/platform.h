#pragma once

#include "opensc4/city.h"
#include "opensc4/dbpf.h"
#include <string>

namespace opensc4 {

// Launch native macOS Cocoa + OpenGL 3.3 Core Profile desktop application
int RunMacOSDesktopApp(cDBPFSystem& dbpfSys, cSC4City& initialCity);

// Launch C++ HTTP + WebGL2 Browser Server for OpenSC4
int RunHTTPBrowserServer(cDBPFSystem& dbpfSys, cSC4City& city, int port = 8080);

} // namespace opensc4
