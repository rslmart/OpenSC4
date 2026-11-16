// Prints a deterministic trace of simulation state. Built once against
// opensc4_authentic_core and once against opensc4_enhanced_core (with the
// Authentic edition selected); the two outputs must be byte-identical.

#include "opensc4/city.h"
#include "opensc4/dbpf.h"

#include <cstdio>
#include <filesystem>
#include <string>

using namespace opensc4;

namespace {

Uint64 Fnv(Uint64 h, const void* data, std::size_t n) {
    const auto* p = static_cast<const unsigned char*>(data);
    for (std::size_t i = 0; i < n; ++i) {
        h ^= p[i];
        h *= 0x100000001b3ull;
    }
    return h;
}

template <typename T>
Uint64 Mix(Uint64 h, const T& v) { return Fnv(h, &v, sizeof(v)); }

template <typename T>
Uint64 HashGrid(const cSC4SimGrid<T>& g) {
    Uint64 h = 0xcbf29ce484222325ull;
    for (int z = 0; z < g.GetTractCountZ(); ++z) {
        for (int x = 0; x < g.GetTractCountX(); ++x) {
            h = Mix(h, g.GetTractValue(x, z));
        }
    }
    return h;
}

Uint64 HashRoutes(const cSC4City& city) {
    Uint64 h = 0xcbf29ce484222325ull;
    for (const auto& r : city.ActiveCommuteRoutes()) {
        h = Mix(h, r.startLotIndex);
        h = Mix(h, r.destLotIndex);
        h = Mix(h, r.commuters);
        h = Mix(h, r.tripLength);
        h = Fnv(h, r.compressedRoute.data(), r.compressedRoute.size());
    }
    return h;
}

void Dump(const char* scenario, const cSC4City& city) {
    const auto& d = city.Demand();
    std::printf(
        "%s day=%u total=%u funds=%lld pop=%u/%u/%u lots=%zu bld=%zu net=%zu "
        "demand=%d,%d,%d,%d,%d,%d,%d,%d,%d routes=%zu:%016llx "
        "lv=%016llx pol=%016llx crm=%016llx trf=%016llx pwr=%016llx wtr=%016llx may=%016llx\n",
        scenario, city.SimDay(), city.TotalSimDays(),
        static_cast<long long>(city.TreasuryFunds()),
        city.ResidentialPop(), city.CommercialPop(), city.IndustrialPop(),
        city.Lots().size(), city.Buildings().size(), city.Networks().size(),
        d.resLow, d.resMed, d.resHigh, d.comLow, d.comMed, d.comHigh, d.indAg, d.indMed, d.indHigh,
        city.ActiveCommuteRoutes().size(), static_cast<unsigned long long>(HashRoutes(city)),
        static_cast<unsigned long long>(HashGrid(city.LandValueGrid())),
        static_cast<unsigned long long>(HashGrid(city.PollutionGrid())),
        static_cast<unsigned long long>(HashGrid(city.CrimeGrid())),
        static_cast<unsigned long long>(HashGrid(city.TrafficGrid())),
        static_cast<unsigned long long>(HashGrid(city.PowerGrid())),
        static_cast<unsigned long long>(HashGrid(city.WaterGrid())),
        static_cast<unsigned long long>(HashGrid(city.MayorRatingGrid())));
}

void SelectAuthentic(cSC4City& city) {
#ifdef OPENSC4_PARITY_ENHANCED
    city.SetEdition(EngineEdition::Authentic);
#else
    (void)city;
#endif
}

void Run(const char* scenario, cSC4City& city) {
    // Not dumped before the first step: the default constructor builds the city
    // under DefaultEnhanced, and demand is only recomputed by the next step.
    SelectAuthentic(city);
    for (int step = 1; step <= 24; ++step) {
        if (step == 3) city.SetTaxRate(0, 1, 10.0f);
        if (step == 6) city.IssueBond(25000);
        city.StepSimulation(15);
        Dump(scenario, city);
    }
}

struct SubfileRecord { std::vector<Uint8> data; };
struct TgiLess {
    bool operator()(const cSC4TGI& l, const cSC4TGI& r) const noexcept {
        if (l.type != r.type) return l.type < r.type;
        if (l.group != r.group) return l.group < r.group;
        return l.instance < r.instance;
    }
};

// Pair-diff mode: deterministic subfile inventory + diff between two .sc4 archives.
// Used by the M4 evolution-parity oracle: pristine bundle save (D) vs a
// fresh original-game save (D+N) — the moved-Type-ID inventory is the
// concrete re-simulation target list.
//   DBPF open path: cGZDBSegmentPackedFile::Open @ 0x10036a4ac (header magic
//   check @ 0x1003701b5, index bound checks @ 0x1003701dc-20d, per-record
//   bounds @ 0x100370291-2a1 — docs/LOADER_CONSTRAINTS.md).
int RunPairDiff(const std::string& pristinePath, const std::string& freshPath) {
    cDBPFArchive pristine;
    if (!pristine.Open(pristinePath)) {
        std::printf("pair-diff: pristine open failed: %s\n", pristinePath.c_str());
        return 1;
    }
    cDBPFArchive fresh;
    if (!fresh.Open(freshPath)) {
        std::printf("pair-diff: fresh open failed: %s\n", freshPath.c_str());
        return 1;
    }
    auto load = [](cDBPFArchive& arc) {
        std::map<cSC4TGI, SubfileRecord, TgiLess> records;
        for (const auto& e : arc.Entries()) {
            std::vector<Uint8> data;
            if (!arc.ReadEntry(e, data)) {
                std::printf("pair-diff: ReadEntry failed for type=%08x group=%08x instance=%08x\n",
                    e.tgi.type, e.tgi.group, e.tgi.instance);
                continue;
            }
            records[e.tgi] = {std::move(data)};
        }
        return records;
    };
    const auto a = load(pristine);
    const auto b = load(fresh);
    std::map<Uint32, std::vector<cSC4TGI>, std::less<Uint32>> byType;
    for (const auto& [tgi, rec] : b) byType[tgi.type].push_back(tgi);
    for (const auto& [tgi, rec] : a) byType[tgi.type].push_back(tgi);
    Uint32 typesAdded = 0, typesRemoved = 0, typesChanged = 0, typesIdentical = 0;
    for (const auto& [type, tgis] : byType) {
        bool anyInA = false, anyInB = false, anyDiff = false;
        std::size_t bytesA = 0, bytesB = 0;
        for (const auto& tgi : tgis) {
            const auto ia = a.find(tgi);
            const auto ib = b.find(tgi);
            anyInA = anyInA || (ia != a.end());
            anyInB = anyInB || (ib != b.end());
            if (ia != a.end()) bytesA += ia->second.data.size();
            if (ib != b.end()) bytesB += ib->second.data.size();
            if (ia != a.end() && ib != b.end() && ia->second.data != ib->second.data) anyDiff = true;
        }
        if (!anyInA) { ++typesAdded; } else if (!anyInB) { ++typesRemoved; } else if (anyDiff) { ++typesChanged; } else { ++typesIdentical; }
        if (!anyInA || !anyInB || anyDiff) {
            std::string name = GetGZClassName(type);
            std::printf("pair-diff: %-28s type=%08x records=%zu A=%zuB=%zu changed=%s\n",
                name.c_str(), type, tgis.size(), bytesA, bytesB, anyDiff ? "yes" : "no");
        }
    }
    std::printf("pair-diff summary: typesAdded=%u typesRemoved=%u typesChanged=%u typesIdentical=%u\n",
        typesAdded, typesRemoved, typesChanged, typesIdentical);
    return 0;
}

} // namespace

int main(int argc, char* argv[]) {
    if (argc >= 4 && std::string(argv[1]) == "--pair-diff") {
        return RunPairDiff(argv[2], argv[3]);
    }
    {
        cSC4City city;
        Run("procedural", city);
    }

    const std::string app = cDBPFSystem::DefaultSteamSC4AppPath();
    const std::string save =
        app + "/Contents/GameData/Regions/Timbuktu/City - Big City Tutorial.sc4";
    if (std::filesystem::exists(save)) {
        cDBPFSystem dbpfSys;
        dbpfSys.MountInstall(app);
        cSC4City city;
        if (city.LoadFromSC4File(save, &dbpfSys)) {
            Run("big_city_tutorial", city);
        } else {
            std::printf("big_city_tutorial load-failed\n");
        }
    }
    return 0;
}
