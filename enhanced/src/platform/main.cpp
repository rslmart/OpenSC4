#include "opensc4/platform.h"
#include "opensc4/formats.h"
#include <cstdio>
#include <cstring>
#include <string>

using namespace opensc4;

int main(int argc, char** argv) {
    std::string appPath = cDBPFSystem::DefaultSteamSC4AppPath();
    std::string sc4FileToLoad;
    bool serveMode = false;
    bool infoOnly = false;
    int port = 8080;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--serve") {
            serveMode = true;
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                port = std::atoi(argv[++i]);
            }
        } else if (arg == "--app-path" && i + 1 < argc) {
            appPath = argv[++i];
        } else if (arg == "--city" && i + 1 < argc) {
            sc4FileToLoad = argv[++i];
        } else if (arg == "--info") {
            infoOnly = true;
        } else if (arg == "--edition" && i + 1 < argc) {
            std::string ed = argv[++i];
            if (ed == "authentic" || ed == "vanilla") {
                sc4FileToLoad += ""; // handled after city init
            }
        }
    }

    cDBPFSystem dbpfSys;
    bool mounted = dbpfSys.MountInstall(appPath);

    cSC4City city;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--edition" && i + 1 < argc) {
            std::string ed = argv[++i];
            city.SetEdition((ed == "authentic" || ed == "vanilla")
                                ? EngineEdition::Authentic
                                : EngineEdition::Enhanced);
        } else if (arg == "--disable-mod" && i + 1 < argc) {
            if (const auto* desc = FindEnhancedModBySlug(argv[++i])) {
                city.SetModEnabled(desc->id, false);
            }
        } else if (arg == "--enable-mod" && i + 1 < argc) {
            if (const auto* desc = FindEnhancedModBySlug(argv[++i])) {
                city.SetModEnabled(desc->id, true);
            }
        }
    }
    if (!sc4FileToLoad.empty()) {
        city.LoadFromSC4File(sc4FileToLoad, &dbpfSys);
    } else if (mounted && !dbpfSys.RegionCities().empty()) {
        // Default to "City - Big City Tutorial.sc4" in Timbuktu if available
        std::string chosen = dbpfSys.RegionCities().front().fullPath;
        for (const auto& rc : dbpfSys.RegionCities()) {
            if (rc.fileName.find("Big City Tutorial") != std::string::npos) {
                chosen = rc.fullPath;
                break;
            }
        }
        city.LoadFromSC4File(chosen, &dbpfSys);
    }

    if (infoOnly) {
        MachOBinarySummary machO = InspectSC4MachOBinary(dbpfSys.MachOBinaryPath());
        std::printf("OpenSC4 Engine Summary:\n");
        std::printf("  Mounted Steam App: %s\n", mounted ? "YES" : "NO (OpenGFX Procedural Mode)");
        std::printf("  DBPF Archives:     %zu (%zu total TGI entries)\n",
                    dbpfSys.Archives().size(), dbpfSys.TotalIndexedEntries());
        std::printf("  Region .sc4 Saves: %zu\n", dbpfSys.RegionCities().size());
        std::printf("  Mach-O Symbols:    %u (%u GZCOM factories, %zu CLSIDs extracted)\n",
                    machO.totalSymbols, machO.gzcomFactoryCount, machO.extractedCLSIDs.size());
        std::printf("  Active City:       %s (Mayor: %s, %dx%d, Pop: %u, Funds: %lld)\n",
                    city.CityName().c_str(), city.MayorName().c_str(),
                    city.CellCountX(), city.CellCountZ(),
                    city.TotalPop(), static_cast<long long>(city.TreasuryFunds()));
        std::printf("  Simulation Date:   %04u-%02u-%02u (Julian day %u)\n",
                    city.SimYear(), city.SimMonth(), city.SimDay(), city.SimJulianDay());
        std::printf("  Occupants:         %zu Lots, %zu Buildings, %zu Networks, %zu Flora, %zu Props\n",
                    city.Lots().size(), city.Buildings().size(),
                    city.Networks().size(), city.Flora().size(), city.Props().size());
        return 0;
    }

    if (serveMode) {
        return RunHTTPBrowserServer(dbpfSys, city, port);
    }

#if defined(__APPLE__)
    return RunMacOSDesktopApp(dbpfSys, city);
#else
    return RunHTTPBrowserServer(dbpfSys, city, port);
#endif
}
