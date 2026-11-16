#include "opensc4/platform.h"
#include "opensc4/formats.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <cctype>
#include <sstream>
#include <iomanip>
#include <algorithm>

namespace opensc4 {

static std::string JsonEscape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (unsigned char c : s) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c >= 32 && c < 127) out.push_back(static_cast<char>(c));
    }
    return out;
}

static float SafeJsonFloat(float v, float fallback = 0.0f) noexcept {
    return std::isfinite(v) ? v : fallback;
}

static std::string BuildCityStateJson(const cSC4City& city) {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1);
    oss << "{";
    oss << "\"cityName\":\"" << JsonEscape(city.CityName()) << "\",";
    oss << "\"mayorName\":\"" << JsonEscape(city.MayorName()) << "\",";
    oss << "\"sourcePath\":\"" << JsonEscape(city.SourcePath()) << "\",";
    oss << "\"isOriginalSave\":" << (city.IsFromOriginalSave() ? "true" : "false") << ",";
    oss << "\"cellCountX\":" << city.CellCountX() << ",";
    oss << "\"cellCountZ\":" << city.CellCountZ() << ",";
    oss << "\"seaLevel\":" << city.SeaLevel() << ",";
    oss << "\"resPop\":" << city.ResidentialPop() << ",";
    oss << "\"comPop\":" << city.CommercialPop() << ",";
    oss << "\"indPop\":" << city.IndustrialPop() << ",";
    oss << "\"funds\":" << city.TreasuryFunds() << ",";
    oss << "\"year\":" << city.SimYear() << ",";
    oss << "\"month\":" << city.SimMonth() << ",";
    oss << "\"day\":" << city.SimDay() << ",";
    oss << "\"hasThumbnail\":" << (!city.PngThumbnail().empty() ? "true" : "false") << ",";
    oss << "\"pipeCount\":" << city.PipeOccupantCount() << ",";
    oss << "\"subwayCount\":" << city.SubwayOccupantCount() << ",";
    oss << "\"powerLineCount\":" << city.PowerLineOccupantCount() << ",";
    oss << "\"simGridCounts\":{"
        << "\"sint8\":" << city.LoadedSint8Grids().size() << ","
        << "\"uint8\":" << city.LoadedUint8Grids().size() << ","
        << "\"sint16\":" << city.LoadedSint16Grids().size() << ","
        << "\"uint16\":" << city.LoadedUint16Grids().size() << ","
        << "\"float32\":" << city.LoadedFloat32Grids().size()
        << "},";
    oss << "\"buildingDevelopment\":{"
        << "\"constructionResources\":" << city.BuildingDevelopment().constructionResources << ","
        << "\"minimumResources\":" << city.BuildingDevelopment().minimumConstructionResources << ","
        << "\"maximumResources\":" << city.BuildingDevelopment().maximumConstructionResources << ","
        << "\"dailyCycles\":" << city.BuildingDevelopment().dailyCycles << ","
        << "\"monthlyCycles\":" << city.BuildingDevelopment().monthlyCycles << ","
        << "\"growthAttempts\":" << city.BuildingDevelopment().growthAttempts << ","
        << "\"growthExecutions\":" << city.BuildingDevelopment().growthExecutions << ","
        << "\"declinePlanCycles\":" << city.BuildingDevelopment().declinePlanCycles << ","
        << "\"declinePlansCreated\":" << city.BuildingDevelopment().declinePlansCreated << ","
        << "\"pendingDeclinePlans\":" << city.BuildingDevelopment().pendingDeclinePlans << ","
        << "\"declineExecutions\":" << city.BuildingDevelopment().declineExecutions << ","
        << "\"declinePlansDropped\":" << city.BuildingDevelopment().declinePlansDropped
        << "},";
    oss << "\"demand\":{"
        << "\"r1\":" << city.Demand().resLow << ","
        << "\"r2\":" << city.Demand().resMed << ","
        << "\"r3\":" << city.Demand().resHigh << ","
        << "\"c1\":" << city.Demand().comLow << ","
        << "\"c2\":" << city.Demand().comMed << ","
        << "\"c3\":" << city.Demand().comHigh << ","
        << "\"i1\":" << city.Demand().indAg << ","
        << "\"i2\":" << city.Demand().indMed << ","
        << "\"i3\":" << city.Demand().indHigh
        << "},";

    // Heightmap array
    oss << "\"heightmap\":[";
    const auto& hm = city.Heightmap();
    for (std::size_t i = 0; i < hm.size(); ++i) {
        if (i > 0) oss << ",";
        oss << SafeJsonFloat(hm[i], 250.0f);
    }
    oss << "],";

    // Lots array
    oss << "\"lots\":[";
    const auto& lots = city.Lots();
    for (std::size_t i = 0; i < lots.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& l = lots[i];
        oss << "[" << int(l.minX) << "," << int(l.minZ) << ","
            << int(l.maxX) << "," << int(l.maxZ) << ","
            << int(l.zoneType) << "," << int(l.wealth) << "," << SafeJsonFloat(l.altitudeY, 250.0f) << "]";
    }
    oss << "],";

    // Buildings array: [bounds..., zoneHint, wealthHint, exemplarIID, name, s3dIID, fshIID]
    oss << "\"buildings\":[";
    const auto& blds = city.Buildings();
    for (std::size_t i = 0; i < blds.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& b = blds[i];
        oss << "[" << SafeJsonFloat(b.minX) << "," << SafeJsonFloat(b.minY, 250.0f) << "," << SafeJsonFloat(b.minZ) << ","
            << SafeJsonFloat(b.maxX) << "," << SafeJsonFloat(b.maxY, 260.0f) << "," << SafeJsonFloat(b.maxZ) << ","
            << int(b.zoneHint) << "," << int(b.wealthHint) << ","
            << b.exemplarTGI.instance << ",\"" << JsonEscape(b.name) << "\","
            << b.s3dInstanceId << "," << b.fshTextureId << "]";
    }
    oss << "],";

    // Networks array: [four xyz vertices, netType, tileX, tileZ, fshIID, four uv pairs]
    oss << "\"networks\":[";
    const auto& nets = city.Networks();
    for (std::size_t i = 0; i < nets.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& n = nets[i];
        oss << "["
            << SafeJsonFloat(n.verts[0].x) << "," << SafeJsonFloat(n.verts[0].y, 250.0f) << "," << SafeJsonFloat(n.verts[0].z) << ","
            << SafeJsonFloat(n.verts[1].x) << "," << SafeJsonFloat(n.verts[1].y, 250.0f) << "," << SafeJsonFloat(n.verts[1].z) << ","
            << SafeJsonFloat(n.verts[2].x) << "," << SafeJsonFloat(n.verts[2].y, 250.0f) << "," << SafeJsonFloat(n.verts[2].z) << ","
            << SafeJsonFloat(n.verts[3].x) << "," << SafeJsonFloat(n.verts[3].y, 250.0f) << "," << SafeJsonFloat(n.verts[3].z) << ","
            << int(n.networkType) << "," << int(n.tileX) << "," << int(n.tileZ) << "," << (n.textureId ? n.textureId + 4u : 0u) << ","
            << SafeJsonFloat(n.verts[0].u) << "," << SafeJsonFloat(n.verts[0].v) << ","
            << SafeJsonFloat(n.verts[1].u) << "," << SafeJsonFloat(n.verts[1].v) << ","
            << SafeJsonFloat(n.verts[2].u) << "," << SafeJsonFloat(n.verts[2].v) << ","
            << SafeJsonFloat(n.verts[3].u) << "," << SafeJsonFloat(n.verts[3].v) << "]";
    }
    oss << "],";

    // Flora array (cap at 1500 for fast JSON payload)
    oss << "\"flora\":[";
    const auto& fl = city.Flora();
    std::size_t flLimit = std::min<std::size_t>(fl.size(), 1500);
    for (std::size_t i = 0; i < flLimit; ++i) {
        if (i > 0) oss << ",";
        oss << "[" << SafeJsonFloat(fl[i].x) << "," << SafeJsonFloat(fl[i].y, 250.0f) << "," << SafeJsonFloat(fl[i].z) << "]";
    }
    oss << "],";

    // Tract SimGrids (LandValue, Pollution, Crime, Traffic)
    auto dumpGrid = [&](const char* key, const cSC4SimGrid<Uint8>& g, bool comma) {
        oss << "\"" << key << "\":[";
        int tx = g.GetTractCountX();
        int tz = g.GetTractCountZ();
        bool first = true;
        for (int z = 0; z < tz; ++z) {
            for (int x = 0; x < tx; ++x) {
                if (!first) oss << ",";
                first = false;
                oss << int(g.GetTractValue(x, z));
            }
        }
        oss << "]" << (comma ? "," : "");
    };
    oss << "\"tractCountX\":" << city.LandValueGrid().GetTractCountX() << ",";
    oss << "\"tractCountZ\":" << city.LandValueGrid().GetTractCountZ() << ",";
    dumpGrid("landValue", city.LandValueGrid(), true);
    dumpGrid("pollution", city.PollutionGrid(), true);
    dumpGrid("crime",     city.CrimeGrid(),     true);
    dumpGrid("traffic",   city.TrafficGrid(),   true);

    // Mayor rating is signed (Sint8, -100..100), so it cannot share dumpGrid.
    oss << "\"mayorRating\":[";
    {
        const auto& rating = city.MayorRatingGrid();
        const int tx = rating.GetTractCountX();
        const int tz = rating.GetTractCountZ();
        bool first = true;
        for (int z = 0; z < tz; ++z) {
            for (int x = 0; x < tx; ++x) {
                if (!first) oss << ",";
                first = false;
                oss << int(rating.GetTractValue(x, z));
            }
        }
    }
    oss << "]";

    oss << "}";
    return oss.str();
}

static std::string BuildCatalogJson(const cDBPFSystem& dbpfSys, const MachOBinarySummary& machO) {
    std::ostringstream oss;
    oss << "{";
    oss << "\"mounted\":" << (dbpfSys.IsMounted() ? "true" : "false") << ",";
    oss << "\"gameDataPath\":\"" << JsonEscape(dbpfSys.GameDataPath()) << "\",";
    oss << "\"totalIndexedEntries\":" << dbpfSys.TotalIndexedEntries() << ",";
    oss << "\"machO\":{"
        << "\"valid\":" << (machO.valid ? "true" : "false") << ","
        << "\"totalSymbols\":" << machO.totalSymbols << ","
        << "\"gzcomFactories\":" << machO.gzcomFactoryCount << ","
        << "\"extractedCLSIDs\":" << machO.extractedCLSIDs.size()
        << "},";
    oss << "\"archives\":[";
    for (std::size_t i = 0; i < dbpfSys.Archives().size(); ++i) {
        if (i > 0) oss << ",";
        const auto& a = dbpfSys.Archives()[i];
        oss << "{\"path\":\"" << JsonEscape(a->Path()) << "\",\"entries\":" << a->Entries().size() << "}";
    }
    oss << "],";
    oss << "\"regions\":[";
    const auto& regions = dbpfSys.Regions();
    for (std::size_t i = 0; i < regions.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{\"index\":" << i
            << ",\"name\":\"" << JsonEscape(regions[i].name) << "\""
            << ",\"width\":" << regions[i].width
            << ",\"height\":" << regions[i].height
            << "}";
    }
    oss << "],";
    oss << "\"cities\":[";
    const auto& rc = dbpfSys.RegionCities();
    for (std::size_t i = 0; i < rc.size(); ++i) {
        if (i > 0) oss << ",";
        oss << "{\"index\":" << i
            << ",\"region\":\"" << JsonEscape(rc[i].regionName) << "\""
            << ",\"file\":\"" << JsonEscape(rc[i].fileName) << "\""
            << ",\"name\":\"" << JsonEscape(rc[i].cityName) << "\""
            << ",\"mayor\":\"" << JsonEscape(rc[i].mayorName) << "\""
            << ",\"resPop\":" << rc[i].residentialPop
            << ",\"comPop\":" << rc[i].commercialPop
            << ",\"indPop\":" << rc[i].industrialPop
            << ",\"funds\":" << rc[i].funds
            << ",\"tileX\":" << rc[i].tileX
            << ",\"tileZ\":" << rc[i].tileZ
            << ",\"tractSizeX\":" << rc[i].sizeX
            << ",\"tractSizeZ\":" << rc[i].sizeZ
            << ",\"sizeX\":" << (rc[i].sizeX * 64)
            << ",\"sizeZ\":" << (rc[i].sizeZ * 64)
            << ",\"hasThumbnail\":" << (rc[i].hasPngThumbnail ? "true" : "false")
            << "}";
    }
    oss << "]}";
    return oss.str();
}

static std::string BuildRegionJson(const cDBPFSystem& dbpfSys, std::size_t regionIndex) {
    const auto& regions = dbpfSys.Regions();
    if (regionIndex >= regions.size()) return "{\"error\":\"region not found\",\"cities\":[]}";

    const auto& region = regions[regionIndex];
    std::ostringstream oss;
    oss << "{\"index\":" << regionIndex
        << ",\"name\":\"" << JsonEscape(region.name) << "\""
        << ",\"width\":" << region.width
        << ",\"height\":" << region.height
        << ",\"cities\":[";

    bool first = true;
    const auto& cities = dbpfSys.RegionCities();
    for (std::size_t i = 0; i < cities.size(); ++i) {
        const auto& city = cities[i];
        if (city.regionName != region.name) continue;
        if (!first) oss << ",";
        first = false;
        oss << "{\"index\":" << i
            << ",\"name\":\"" << JsonEscape(city.cityName) << "\""
            << ",\"mayor\":\"" << JsonEscape(city.mayorName) << "\""
            << ",\"tileX\":" << city.tileX
            << ",\"tileZ\":" << city.tileZ
            << ",\"tractSizeX\":" << city.sizeX
            << ",\"tractSizeZ\":" << city.sizeZ
            << ",\"resPop\":" << city.residentialPop
            << ",\"comPop\":" << city.commercialPop
            << ",\"indPop\":" << city.industrialPop
            << ",\"hasThumbnail\":" << (city.hasPngThumbnail ? "true" : "false")
            << "}";
    }
    oss << "]}";
    return oss.str();
}

static std::string BuildS3DJson(const cDBPFSystem& dbpfSys, Uint32 instanceId) {
    std::vector<Uint8> bytes;
    cS3DModel model;
    if (!dbpfSys.LoadAssetByTypeAndInstance(kTypeS3D, instanceId, bytes) ||
        !model.Parse(bytes.data(), bytes.size())) {
        return "{\"error\":\"S3D not found\"}";
    }

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(5);
    oss << "{\"instanceId\":" << instanceId
        << ",\"textureId\":" << (model.materials.empty() ? 0 : model.materials.front().textureId)
        << ",\"vertices\":[";
    for (std::size_t i = 0; i < model.vertices.size(); ++i) {
        if (i > 0) oss << ",";
        const auto& v = model.vertices[i];
        oss << "[" << SafeJsonFloat(v.x) << "," << SafeJsonFloat(v.y) << ","
            << SafeJsonFloat(v.z) << "," << SafeJsonFloat(v.u) << "," << SafeJsonFloat(v.v) << "]";
    }
    oss << "],\"indices\":[";
    for (std::size_t i = 0; i < model.indices.size(); ++i) {
        if (i > 0) oss << ",";
        oss << model.indices[i];
    }
    oss << "]}";
    return oss.str();
}

static const char* kBrowserAppHTML = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8" />
<title>OpenSC4 — SimCity 4 Deluxe Browser & Native C++20 Engine</title>
<style>
  :root {
    --bg: #0f1722;
    --panel: rgba(19, 30, 46, 0.92);
    --border: #2f4b6e;
    --accent: #38bdf8;
    --gold: #facc15;
    --text: #e2e8f0;
    --muted: #94a3b8;
  }
  * { box-sizing: border-box; margin: 0; padding: 0; user-select: none; }
  body { background: var(--bg); color: var(--text); font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, monospace; overflow: hidden; height: 100vh; width: 100vw; }
  #glcanvas { width: 100vw; height: 100vh; display: block; cursor: crosshair; }
  .topbar {
    position: fixed; top: 10px; left: 10px; right: 10px;
    display: flex; align-items: center; gap: 10px; flex-wrap: wrap;
    background: var(--panel); border: 1px solid var(--border);
    padding: 8px 14px; border-radius: 8px; backdrop-filter: blur(8px);
    box-shadow: 0 8px 24px rgba(0,0,0,0.45); z-index: 10;
  }
  .brand { font-weight: 800; color: var(--accent); letter-spacing: 0.5px; font-size: 15px; margin-right: 6px; }
  select, button {
    background: #1e293b; color: var(--text); border: 1px solid var(--border);
    padding: 5px 10px; border-radius: 5px; font-size: 12px; cursor: pointer;
  }
  button:hover, select:hover { border-color: var(--accent); }
  button.active { background: #0284c7; border-color: #7dd3fc; font-weight: 600; }
  .stat-pill {
    background: rgba(15, 23, 42, 0.8); border: 1px solid #334155;
    padding: 4px 10px; border-radius: 5px; font-size: 12px;
  }
  .stat-pill b { color: var(--gold); }
  .sidebar {
    position: fixed; top: 64px; right: 10px; width: 320px;
    background: var(--panel); border: 1px solid var(--border);
    border-radius: 8px; padding: 12px; z-index: 10;
    max-height: calc(100vh - 120px); overflow-y: auto;
    box-shadow: 0 8px 24px rgba(0,0,0,0.45);
  }
  .sidebar h3 { font-size: 13px; color: var(--accent); margin-bottom: 8px; border-bottom: 1px solid #334155; padding-bottom: 4px; }
  .sidebar input[type="number"] {
    user-select: text; width: 72px; background: #1e293b; color: var(--text);
    border: 1px solid var(--border); border-radius: 4px; padding: 2px 6px; font-size: 12px;
  }
  #budgetBox button { width: 100%; margin-top: 8px; }
  #queryBox { font-size: 12px; }
  .kv { display: flex; justify-content: space-between; align-items: center; gap: 8px; font-size: 12px; padding: 3px 0; border-bottom: 1px dashed rgba(51,65,85,0.5); }
  .kv span:first-child { color: var(--muted); }
  .rci-bars { display: flex; gap: 6px; height: 70px; align-items: flex-end; margin: 10px 0; padding: 6px; background: #0b111a; border-radius: 6px; border: 1px solid #1e293b; }
  .bar-col { flex: 1; display: flex; flex-direction: column; align-items: center; height: 100%; justify-content: flex-end; }
  .bar-fill { width: 100%; border-radius: 3px 3px 0 0; min-height: 3px; transition: height 0.2s; }
  .bar-lbl { font-size: 10px; color: var(--muted); margin-top: 4px; }
  .thumb-box { margin-top: 8px; text-align: center; }
  .thumb-box img { max-width: 100%; border-radius: 4px; border: 1px solid var(--border); }
  #regionView {
    display: none; position: fixed; inset: 62px 10px 10px 10px; z-index: 7;
    border: 1px solid var(--border); border-radius: 8px; overflow: hidden;
    background: radial-gradient(circle at 50% 20%, #25415b 0%, #101a27 68%);
    box-shadow: 0 8px 24px rgba(0,0,0,0.55);
  }
  #regionMap { position: absolute; inset: 46px 12px 12px 12px; }
  #regionTitle {
    position: absolute; top: 10px; left: 14px; right: 14px; color: var(--accent);
    font-weight: 700; font-size: 15px; display: flex; justify-content: space-between;
  }
  .region-city {
    position: absolute; border: 2px solid #54789b; border-radius: 5px; overflow: hidden;
    background: #192a38; color: white; cursor: pointer; box-shadow: 0 3px 9px rgba(0,0,0,.55);
    transition: transform .12s, border-color .12s, filter .12s;
  }
  .region-city:hover { transform: translateY(-2px); border-color: #7dd3fc; filter: brightness(1.14); z-index: 3; }
  .region-city img { width: 100%; height: 100%; display: block; object-fit: cover; image-rendering: auto; pointer-events: none; }
  .region-city-label {
    position: absolute; left: 0; right: 0; bottom: 0; padding: 3px 5px;
    font-size: 10px; white-space: nowrap; overflow: hidden; text-overflow: ellipsis;
    background: linear-gradient(transparent, rgba(0,0,0,.88)); text-shadow: 0 1px 2px black;
    pointer-events: none;
  }
  .bottombar {
    position: fixed; bottom: 10px; left: 10px; right: 10px;
    display: flex; justify-content: space-between; align-items: center;
    background: var(--panel); border: 1px solid var(--border);
    padding: 7px 14px; border-radius: 8px; font-size: 12px; z-index: 10;
  }
</style>
</head>
<body>
<canvas id="glcanvas"></canvas>
<div id="regionView">
  <div id="regionTitle"><span>SC4 Region</span><span id="regionSummary"></span></div>
  <div id="regionMap"></div>
</div>

<div class="topbar">
  <span class="brand">OPENSC4 (C++20 / DBPF / WebGL2)</span>
  <button id="btnRegion">Region View</button>
  <select id="regionSelect" title="Select an installed SimCity 4 region"></select>
  <select id="citySelect" title="Select Shipped SC4 City Save or Procedural Map"></select>
  <select id="toolSelect">
    <option value="0">Tool: Inspect Tile</option>
    <option value="1">Zone: Low Residential (R$)</option>
    <option value="2">Zone: Med Residential (R$$)</option>
    <option value="3">Zone: High Residential (R$$$)</option>
    <option value="4">Zone: Low Commercial (Cs$)</option>
    <option value="5">Zone: Med Commercial (Co$$)</option>
    <option value="6">Zone: High Commercial (Co$$$)</option>
    <option value="7">Zone: Agriculture (I-R)</option>
    <option value="8">Zone: Med Industrial (I-M)</option>
    <option value="9">Zone: High-Tech (I-HT)</option>
    <option value="10">Network: Build Road</option>
    <option value="11">Network: Build Street</option>
    <option value="12">Network: Build Avenue</option>
    <option value="13">Network: Build Highway</option>
    <option value="14">Network: Build Rail</option>
    <option value="15">Civic: Coal Power Plant</option>
    <option value="16">Civic: Water Pump</option>
    <option value="17">Civic: Police Precinct</option>
    <option value="18">Civic: Fire Station</option>
    <option value="19">Civic: City Plaza Park</option>
    <option value="20">Bulldoze Tile</option>
    <option value="21">Terraform: Raise Terrain</option>
    <option value="22">Terraform: Lower Terrain</option>
  </select>
  <select id="overlaySelect">
    <option value="0">Overlay: Normal 3D View</option>
    <option value="1">Overlay: RCI Zones</option>
    <option value="2">Overlay: Land Value (cSC4SimGrid)</option>
    <option value="3">Overlay: Pollution (cSC4SimGrid)</option>
    <option value="4">Overlay: Crime (cSC4SimGrid)</option>
    <option value="5">Overlay: Traffic (cSC4SimGrid)</option>
    <option value="8">Overlay: Mayor Rating</option>
  </select>
  <button id="btnRotL" title="Rotate 90° CCW (Q)">⟲ Rotate</button>
  <button id="btnRotR" title="Rotate 90° CW (E)">⟳ Rotate</button>
  <button id="btnStep" title="Advance cSC4Simulator by 30 Days">+1 Month</button>
  <button id="btnAuto" class="active">Sim: Running</button>
  <span class="stat-pill" id="hudPop">Pop: <b>0</b></span>
  <span class="stat-pill" id="hudFunds">Funds: <b>§0</b></span>
  <span class="stat-pill" id="hudDate">Date: <b>2000-01-01</b></span>
</div>

<div class="sidebar">
  <h3>City & DBPF Inspector</h3>
  <div id="cityStats"></div>
  <h3 style="margin-top:12px;">RCI Demand (cSC4DemandSimulator)</h3>
  <div class="rci-bars" id="rciBars"></div>
  <h3>Steam SC4 Source of Truth</h3>
  <div id="sotStats"></div>
  <div class="thumb-box" id="thumbBox"></div>
  <h3 style="margin-top:12px;">Mayor Budget & Bonds</h3>
  <div id="budgetBox">
    <div class="kv"><span>R$ Tax</span><span><input id="taxR" type="number" min="0" max="20" step="0.1" value="9.0">%</span></div>
    <div class="kv"><span>C$ Tax</span><span><input id="taxC" type="number" min="0" max="20" step="0.1" value="9.0">%</span></div>
    <div class="kv"><span>I$ Tax</span><span><input id="taxI" type="number" min="0" max="20" step="0.1" value="9.0">%</span></div>
    <button id="btnBond" type="button">Issue §10,000 Bond</button>
  </div>
  <h3 style="margin-top:12px;">Tile Query Inspector</h3>
  <div id="queryBox">Left-click a tile with Inspect to query it.</div>
</div>

<div class="bottombar">
  <span id="hoverInfo">Controls: Right-Drag or WASD to Pan | Scroll to Zoom (Zooms 1-5) | Q/E to Rotate 90° | Left-Click on Map to Apply Tool</span>
  <span id="engineInfo">C++20 DBPF + QFS + GZCOM Engine</span>
</div>

<script>
const canvas = document.getElementById('glcanvas');
const gl = canvas.getContext('webgl2', { antialias: true });

let city = null;
let catalog = null;
let overlay = 0;
let regionMode = false;
let selectedRegion = 0;
let camTargetX = 1024, camTargetY = 255, camTargetZ = 1024;
let camZoom = 0.85;
let camRot = 0;
let autoSim = true;

// Compile WebGL2 shaders matching cSC4GLCityRenderer
const vsSrc = `#version 300 es
precision highp float;
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;
uniform mat4 uVP;
out vec3 vNormal;
out vec4 vColor;
void main() {
  vNormal = aNormal;
  vColor = aColor;
  gl_Position = uVP * vec4(aPos, 1.0);
}`;
const fsSrc = `#version 300 es
precision mediump float;
in vec3 vNormal;
in vec4 vColor;
out vec4 fragColor;
void main() {
  vec3 sun = normalize(vec3(0.55, 0.78, 0.30));
  float ndl = max(dot(normalize(vNormal), sun), 0.0);
  float light = 0.48 + 0.52 * ndl;
  fragColor = vec4(vColor.rgb * light, vColor.a);
}`;
const texVsSrc = `#version 300 es
precision highp float;
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aUV;
uniform mat4 uVP;
out vec3 vNormal;
out vec2 vUV;
void main() {
  vNormal = aNormal;
  vUV = aUV;
  gl_Position = uVP * vec4(aPos, 1.0);
}`;
const texFsSrc = `#version 300 es
precision mediump float;
in vec3 vNormal;
in vec2 vUV;
uniform sampler2D uTexture;
out vec4 fragColor;
void main() {
  vec4 texel = texture(uTexture, vUV);
  if (texel.a < 0.08) discard;
  vec3 sun = normalize(vec3(0.55, 0.78, 0.30));
  float light = 0.50 + 0.50 * max(dot(normalize(vNormal), sun), 0.0);
  fragColor = vec4(texel.rgb * light, texel.a);
}`;

function compileShader(type, src) {
  const s = gl.createShader(type);
  gl.shaderSource(s, src);
  gl.compileShader(s);
  return s;
}
const prog = gl.createProgram();
gl.attachShader(prog, compileShader(gl.VERTEX_SHADER, vsSrc));
gl.attachShader(prog, compileShader(gl.FRAGMENT_SHADER, fsSrc));
gl.linkProgram(prog);
const texProg = gl.createProgram();
gl.attachShader(texProg, compileShader(gl.VERTEX_SHADER, texVsSrc));
gl.attachShader(texProg, compileShader(gl.FRAGMENT_SHADER, texFsSrc));
gl.linkProgram(texProg);

const vao = gl.createVertexArray();
const vbo = gl.createBuffer();
gl.bindVertexArray(vao);
gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
gl.enableVertexAttribArray(0);
gl.vertexAttribPointer(0, 3, gl.FLOAT, false, 40, 0);
gl.enableVertexAttribArray(1);
gl.vertexAttribPointer(1, 3, gl.FLOAT, false, 40, 12);
gl.enableVertexAttribArray(2);
gl.vertexAttribPointer(2, 4, gl.FLOAT, false, 40, 24);

let vertexCount = 0;
let texturedBatches = [];
const modelAssets = new Map();
const textureAssets = new Map();
let assetGeneration = 0;

async function loadTexture(textureId) {
  if (!textureId) return null;
  if (textureAssets.has(textureId)) return await textureAssets.get(textureId);
  const pending = (async () => {
    const response = await fetch(`/api/fsh.png?id=${textureId}`);
    if (!response.ok || Number(response.headers.get('content-length') || 0) === 0) return null;
    const bitmap = await createImageBitmap(await response.blob());
    const texture = gl.createTexture();
    gl.bindTexture(gl.TEXTURE_2D, texture);
    gl.pixelStorei(gl.UNPACK_FLIP_Y_WEBGL, false); // SC4 UVs: v = 0 is the top FSH row
    gl.texImage2D(gl.TEXTURE_2D, 0, gl.RGBA, gl.RGBA, gl.UNSIGNED_BYTE, bitmap);
    gl.generateMipmap(gl.TEXTURE_2D);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MIN_FILTER, gl.LINEAR_MIPMAP_LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_MAG_FILTER, gl.LINEAR);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_S, gl.CLAMP_TO_EDGE);
    gl.texParameteri(gl.TEXTURE_2D, gl.TEXTURE_WRAP_T, gl.CLAMP_TO_EDGE);
    bitmap.close();
    textureAssets.set(textureId, texture);
    return texture;
  })().catch(() => null);
  textureAssets.set(textureId, pending);
  return pending;
}

async function loadAuthenticAssets() {
  if (!city) return;
  const generation = ++assetGeneration;
  const frequency = new Map();
  for (const b of city.buildings) {
    if (b[10]) frequency.set(b[10], (frequency.get(b[10]) || 0) + 1);
  }
  const modelIds = [...frequency.entries()].sort((a,b) => b[1] - a[1]).slice(0, 96).map(v => v[0]);
  await Promise.all(modelIds.map(async id => {
    if (modelAssets.has(id)) return;
    modelAssets.set(id, null);
    try {
      const model = await (await fetch(`/api/s3d?id=${id}`)).json();
      if (!model.error && model.vertices?.length && model.indices?.length) {
        let minX=Infinity,minY=Infinity,minZ=Infinity,maxX=-Infinity,maxY=-Infinity,maxZ=-Infinity;
        for (const v of model.vertices) {
          minX=Math.min(minX,v[0]); minY=Math.min(minY,v[1]); minZ=Math.min(minZ,v[2]);
          maxX=Math.max(maxX,v[0]); maxY=Math.max(maxY,v[1]); maxZ=Math.max(maxZ,v[2]);
        }
        model.bounds = [minX,minY,minZ,maxX,maxY,maxZ];
        modelAssets.set(id, model);
        await loadTexture(model.textureId);
      }
    } catch (_) {}
  }));
  const networkFrequency = new Map();
  for (const n of city.networks) {
    if (n[15]) networkFrequency.set(n[15], (networkFrequency.get(n[15]) || 0) + 1);
  }
  const networkTextures = [...networkFrequency.entries()].sort((a,b) => b[1]-a[1]).slice(0, 96).map(v => v[0]);
  await Promise.all(networkTextures.map(loadTexture));
  if (generation === assetGeneration) rebuildMesh();
}

function zoneColor(zt) {
  switch (zt) {
    case 1: return [0.28, 0.76, 0.32];
    case 2: return [0.18, 0.65, 0.24];
    case 3: return [0.10, 0.52, 0.16];
    case 4: return [0.30, 0.52, 0.88];
    case 5: return [0.20, 0.40, 0.82];
    case 6: return [0.14, 0.28, 0.74];
    case 7: return [0.82, 0.76, 0.28];
    case 8: return [0.86, 0.64, 0.18];
    case 9: return [0.76, 0.50, 0.14];
    case 14: return [0.52, 0.38, 0.24];
    case 15: return [0.62, 0.66, 0.72];
    default: return [0.45, 0.62, 0.36];
  }
}

function rebuildMesh() {
  if (!city) return;
  for (const batch of texturedBatches) {
    gl.deleteBuffer(batch.vbo);
    gl.deleteVertexArray(batch.vao);
  }
  texturedBatches = [];
  const nx = city.cellCountX;
  const nz = city.cellCountZ;
  const sea = city.seaLevel;
  const hm = city.heightmap;
  const verts = [];
  const texVerts = new Map();

  function pushTri(x0,y0,z0, x1,y1,z1, x2,y2,z2, r,g,b,a) {
    const ux = x1-x0, uy = y1-y0, uz = z1-z0;
    const vx = x2-x0, vy = y2-y0, vz = z2-z0;
    let nx = uy*vz - uz*vy, ny = uz*vx - ux*vz, nz = ux*vy - uy*vx;
    const l = Math.hypot(nx, ny, nz) || 1;
    nx /= l; ny /= l; nz /= l;
    verts.push(x0,y0,z0, nx,ny,nz, r,g,b,a,
               x1,y1,z1, nx,ny,nz, r,g,b,a,
               x2,y2,z2, nx,ny,nz, r,g,b,a);
  }
  function pushQuad(x0,y0,z0, x1,y1,z1, x2,y2,z2, x3,y3,z3, r,g,b,a=1) {
    pushTri(x0,y0,z0, x1,y1,z1, x2,y2,z2, r,g,b,a);
    pushTri(x0,y0,z0, x2,y2,z2, x3,y3,z3, r,g,b,a);
  }
  function pushBox(minX,minY,minZ, maxX,maxY,maxZ, r,g,b, rr,rg,rb) {
    pushQuad(minX,maxY,minZ, minX,maxY,maxZ, maxX,maxY,maxZ, maxX,maxY,minZ, rr,rg,rb,1);
    pushQuad(minX,minY,maxZ, maxX,minY,maxZ, maxX,maxY,maxZ, minX,maxY,maxZ, r,g,b,1);
    pushQuad(maxX,minY,minZ, minX,minY,minZ, minX,maxY,minZ, maxX,maxY,minZ, r*0.85,g*0.85,b*0.85,1);
    pushQuad(maxX,minY,maxZ, maxX,minY,minZ, maxX,maxY,minZ, maxX,maxY,maxZ, r*0.92,g*0.92,b*0.92,1);
    pushQuad(minX,minY,minZ, minX,minY,maxZ, minX,maxY,maxZ, minX,maxY,minZ, r*0.78,g*0.78,b*0.78,1);
  }
  function pushTexTri(textureId, a, b, c) {
    let out = texVerts.get(textureId);
    if (!out) { out = []; texVerts.set(textureId, out); }
    const ux=b[0]-a[0], uy=b[1]-a[1], uz=b[2]-a[2];
    const vx=c[0]-a[0], vy=c[1]-a[1], vz=c[2]-a[2];
    let nnx=uy*vz-uz*vy, nny=uz*vx-ux*vz, nnz=ux*vy-uy*vx;
    const len=Math.hypot(nnx,nny,nnz)||1; nnx/=len; nny/=len; nnz/=len;
    for (const p of [a,b,c]) out.push(p[0],p[1],p[2],nnx,nny,nnz,p[3],p[4]);
  }
  function pushTexQuad(textureId, a, b, c, d) {
    pushTexTri(textureId, a, b, c);
    pushTexTri(textureId, a, c, d);
  }

  const cellZones = new Uint8Array(nx * nz);
  for (const l of city.lots) {
    for (let z = l[1]; z <= l[3] && z < nz; z++) {
      for (let x = l[0]; x <= l[2] && x < nx; x++) {
        cellZones[z * nx + x] = l[4];
      }
    }
  }

  const vStride = nx + 1;
  for (let cz = 0; cz < nz; cz++) {
    for (let cx = 0; cx < nx; cx++) {
      const x0 = cx * 16, z0 = cz * 16, x1 = (cx+1)*16, z1 = (cz+1)*16;
      const y00 = hm[cz * vStride + cx];
      const y10 = hm[cz * vStride + cx + 1];
      const y11 = hm[(cz + 1) * vStride + cx + 1];
      const y01 = hm[(cz + 1) * vStride + cx];
      const avgY = 0.25 * (y00 + y10 + y11 + y01);

      let r = 0.44, g = 0.60, b = 0.34;
      if (avgY < sea - 1.5) { r = 0.62; g = 0.58; b = 0.44; }
      else if (avgY < sea + 3.5) { r = 0.76; g = 0.72; b = 0.52; }
      else {
        const ef = Math.min(Math.max((avgY - sea) / 180, 0), 1);
        r = 0.40 + 0.22 * ef; g = 0.58 - 0.08 * ef; b = 0.32 + 0.12 * ef;
      }

      const zt = cellZones[cz * nx + cx];
      const tIdx = (cz >> 2) * city.tractCountX + (cx >> 2);
      if (overlay === 1 && zt !== 0) {
        [r, g, b] = zoneColor(zt);
      } else if (overlay === 2) {
        const v = (city.landValue[tIdx] || 0) / 255;
        r = 0.15 + 0.20*(1-v); g = 0.25 + 0.70*v; b = 0.20 + 0.55*v;
      } else if (overlay === 3) {
        const v = (city.pollution[tIdx] || 0) / 255;
        r = 0.35 + 0.65*v; g = 0.55*(1 - 0.6*v); b = 0.25*(1-v);
      } else if (overlay === 4) {
        const v = (city.crime[tIdx] || 0) / 255;
        r = 0.30 + 0.70*v; g = 0.50*(1-v); b = 0.30*(1-v);
      } else if (overlay === 5) {
        const v = (city.traffic[tIdx] || 0) / 255;
        r = 0.25 + 0.75*v; g = 0.60 - 0.40*v; b = 0.35 - 0.25*v;
      } else if (overlay === 8) {
        const rating = city.mayorRating?.[tIdx] ?? 0;
        const t = Math.min(Math.max((rating + 100) / 200, 0), 1);
        const redR = 0.86, redG = 0.15, redB = 0.12;
        const yelR = 0.93, yelG = 0.82, yelB = 0.16;
        const grnR = 0.08, grnG = 0.72, grnB = 0.38;
        if (t < 0.5) {
          const u = t * 2;
          r = redR + (yelR - redR) * u;
          g = redG + (yelG - redG) * u;
          b = redB + (yelB - redB) * u;
        } else {
          const u = (t - 0.5) * 2;
          r = yelR + (grnR - yelR) * u;
          g = yelG + (grnG - yelG) * u;
          b = yelB + (grnB - yelB) * u;
        }
      } else if (zt !== 0) {
        const [zr, zg, zb] = zoneColor(zt);
        r = r*0.55 + zr*0.45; g = g*0.55 + zg*0.45; b = b*0.55 + zb*0.45;
      }

      if (((cx + cz) & 1) === 0) { r *= 0.97; g *= 0.97; b *= 0.97; }
      pushQuad(x0,y00,z0, x0,y01,z1, x1,y11,z1, x1,y10,z0, r,g,b,1);

      if (y00 < sea || y10 < sea || y11 < sea || y01 < sea) {
        pushQuad(x0,sea,z0, x0,sea,z1, x1,sea,z1, x1,sea,z0, 0.16,0.44,0.68,0.78);
      }
    }
  }

  for (const n of city.networks) {
    const networkTexture = textureAssets.get(n[15]);
    if (n[15] && networkTexture && typeof networkTexture.then !== 'function') {
      pushTexQuad(n[15],
        [n[0],n[1]+0.36,n[2],n[16],n[17]], [n[3],n[4]+0.36,n[5],n[18],n[19]],
        [n[6],n[7]+0.36,n[8],n[20],n[21]], [n[9],n[10]+0.36,n[11],n[22],n[23]]);
      continue;
    }
    let nr = 0.25, ng = 0.26, nb = 0.28;
    if (n[12] === 3) { nr = 0.38; ng = 0.39; nb = 0.41; }
    else if (n[12] === 6) { nr = 0.20; ng = 0.22; nb = 0.25; }
    else if (n[12] === 2) { nr = 0.32; ng = 0.30; nb = 0.28; }
    else if (n[12] === 1) { nr = 0.42; ng = 0.32; nb = 0.24; }
    pushQuad(n[0],n[1]+0.35,n[2], n[3],n[4]+0.35,n[5], n[6],n[7]+0.35,n[8], n[9],n[10]+0.35,n[11], nr,ng,nb,1);
  }

  for (const bld of city.buildings) {
    const model = modelAssets.get(bld[10]);
    const modelTexture = model && textureAssets.get(model.textureId);
    if (model && modelTexture && typeof modelTexture.then !== 'function') {
      const mb=model.bounds, sx=Math.max(mb[3]-mb[0],.001), sy=Math.max(mb[4]-mb[1],.001), sz=Math.max(mb[5]-mb[2],.001);
      const converted = model.vertices.map(v => [
        bld[0] + (v[0]-mb[0])/sx*(bld[3]-bld[0]),
        bld[1] + (v[1]-mb[1])/sy*(Math.max(bld[4],bld[1]+3.5)-bld[1]),
        bld[2] + (v[2]-mb[2])/sz*(bld[5]-bld[2]), v[3], v[4]
      ]);
      for (let i=0; i+2<model.indices.length; i+=3) {
        const a=converted[model.indices[i]], b=converted[model.indices[i+1]], c=converted[model.indices[i+2]];
        if (a && b && c) pushTexTri(model.textureId, a, b, c);
      }
      continue;
    }
    let br = 0.78, bg = 0.74, bb = 0.68, rr = 0.44, rg = 0.40, rb = 0.36;
    const zt = bld[6];
    if (zt >= 1 && zt <= 3) { br = 0.80; bg = 0.72; bb = 0.64; rr = 0.50; rg = 0.35; rb = 0.28; }
    else if (zt >= 4 && zt <= 6) { br = 0.58; bg = 0.68; bb = 0.78; rr = 0.34; rg = 0.40; rb = 0.46; }
    else if (zt >= 7 && zt <= 9) { br = 0.70; bg = 0.64; bb = 0.52; rr = 0.46; rg = 0.42; rb = 0.36; }
    const tint = (((bld[8] & 0xFF) / 255) - 0.5) * 0.14;
    pushBox(bld[0], bld[1], bld[2], bld[3], Math.max(bld[4], bld[1]+3.5), bld[5],
            br+tint, bg+tint, bb+tint, rr, rg, rb);
  }

  for (const fl of city.flora) {
    if (fl[1] >= sea) {
      pushBox(fl[0]-1.8, fl[1], fl[2]-1.8, fl[0]+1.8, fl[1]+7.5, fl[2]+1.8, 0.20,0.48,0.22, 0.26,0.58,0.28);
    }
  }

  const arr = new Float32Array(verts);
  vertexCount = arr.length / 10;
  gl.bindBuffer(gl.ARRAY_BUFFER, vbo);
  gl.bufferData(gl.ARRAY_BUFFER, arr, gl.StaticDraw || gl.STATIC_DRAW);

  for (const [textureId, values] of texVerts) {
    const texture = textureAssets.get(textureId);
    if (!texture || typeof texture.then === 'function' || values.length === 0) continue;
    const batchVao = gl.createVertexArray();
    const batchVbo = gl.createBuffer();
    gl.bindVertexArray(batchVao);
    gl.bindBuffer(gl.ARRAY_BUFFER, batchVbo);
    gl.bufferData(gl.ARRAY_BUFFER, new Float32Array(values), gl.STATIC_DRAW);
    gl.enableVertexAttribArray(0);
    gl.vertexAttribPointer(0,3,gl.FLOAT,false,32,0);
    gl.enableVertexAttribArray(1);
    gl.vertexAttribPointer(1,3,gl.FLOAT,false,32,12);
    gl.enableVertexAttribArray(2);
    gl.vertexAttribPointer(2,2,gl.FLOAT,false,32,24);
    texturedBatches.push({vao:batchVao,vbo:batchVbo,texture,count:values.length/8});
  }
}

function buildVP(w, h) {
  const aspect = w / Math.max(h, 1);
  const halfH = 520 / Math.max(camZoom, 0.25);
  const halfW = halfH * aspect;
  const yaw = (camRot * 0.5 + 0.25) * Math.PI;
  const pitch = 0.6154797;
  const dx = Math.cos(pitch) * Math.cos(yaw);
  const dy = Math.sin(pitch);
  const dz = Math.cos(pitch) * Math.sin(yaw);
  const dist = 2200;
  const ex = camTargetX + dx * dist, ey = camTargetY + dy * dist, ez = camTargetZ + dz * dist;
  const fx = -dx, fy = -dy, fz = -dz;
  let sx = -fz, sz = fx;
  const sl = Math.hypot(sx, sz) || 1;
  sx /= sl; sz /= sl;
  const ux = -sz * fy, uy = sz * fx - sx * fz, uz = sx * fy;

  const V = [
    sx, ux, -fx, 0,
    0,  uy, -fy, 0,
    sz, uz, -fz, 0,
    -(sx*ex + sz*ez), -(ux*ex + uy*ey + uz*ez), (fx*ex + fy*ey + fz*ez), 1
  ];
  const l = -halfW, r = halfW, b = -halfH, t = halfH, n = 10, f = 5500;
  const P = [
    2/(r-l), 0, 0, 0,
    0, 2/(t-b), 0, 0,
    0, 0, -2/(f-n), 0,
    -(r+l)/(r-l), -(t+b)/(t-b), -(f+n)/(f-n), 1
  ];
  const out = new Float32Array(16);
  for (let c = 0; c < 4; c++) {
    for (let r0 = 0; r0 < 4; r0++) {
      out[c*4 + r0] = P[r0]*V[c*4] + P[4+r0]*V[c*4+1] + P[8+r0]*V[c*4+2] + P[12+r0]*V[c*4+3];
    }
  }
  return out;
}

function screenToTile(sx, sy) {
  if (!city) return null;
  const w = canvas.clientWidth, h = canvas.clientHeight;
  const ndcX = (2 * sx) / w - 1;
  const ndcY = 1 - (2 * sy) / h;
  const aspect = w / Math.max(h, 1);
  const halfH = 520 / Math.max(camZoom, 0.25);
  const halfW = halfH * aspect;
  const rightOff = ndcX * halfW, upOff = ndcY * halfH;

  const yaw = (camRot * 0.5 + 0.25) * Math.PI;
  const pitch = 0.6154797;
  const dx = Math.cos(pitch) * Math.cos(yaw), dy = Math.sin(pitch), dz = Math.cos(pitch) * Math.sin(yaw);
  const fx = -dx, fy = -dy, fz = -dz;
  let sX = -fz, sZ = fx;
  const sl = Math.hypot(sX, sZ) || 1;
  sX /= sl; sZ /= sl;
  const uX = -sZ * fy, uY = sZ * fx - sX * fz, uZ = sX * fy;

  const rx0 = camTargetX + sX * rightOff + uX * upOff;
  const ry0 = camTargetY + uY * upOff;
  const rz0 = camTargetZ + sZ * rightOff + uZ * upOff;
  const t = (camTargetY - ry0) / fy;
  const wx = rx0 + t * fx, wz = rz0 + t * fz;
  const tx = Math.floor(wx / 16), tz = Math.floor(wz / 16);
  if (tx < 0 || tx >= city.cellCountX || tz < 0 || tz >= city.cellCountZ) return null;
  return [tx, tz];
}

function updateUI() {
  if (!city) return;
  document.getElementById('hudPop').innerHTML = `Pop: <b>${city.resPop.toLocaleString()}</b> (C:${city.comPop.toLocaleString()} I:${city.indPop.toLocaleString()})`;
  document.getElementById('hudFunds').innerHTML = `Treasury: <b>§${city.funds.toLocaleString()}</b>`;
  document.getElementById('hudDate').innerHTML = `Date: <b>${city.year}-${String(city.month).padStart(2,'0')}-${String(city.day).padStart(2,'0')}</b>`;

  document.getElementById('cityStats').innerHTML = `
    <div class="kv"><span>City Name</span><b>${city.cityName}</b></div>
    <div class="kv"><span>Mayor</span><b>${city.mayorName}</b></div>
    <div class="kv"><span>Grid Size</span><b>${city.cellCountX}x${city.cellCountZ} (${city.cellCountX*16}m)</b></div>
    <div class="kv"><span>cSC4Lot Records</span><b>${city.lots.length.toLocaleString()}</b></div>
    <div class="kv"><span>cSC4BuildingOccupants</span><b>${city.buildings.length.toLocaleString()}</b></div>
    <div class="kv"><span>cSC4NetworkOccupants</span><b>${city.networks.length.toLocaleString()}</b></div>
    <div class="kv"><span>cSC4FloraOccupants</span><b>${city.flora.length.toLocaleString()}</b></div>
    <div class="kv"><span>Pipes / Subways / Power</span><b>${city.pipeCount} / ${city.subwayCount} / ${city.powerLineCount}</b></div>
    <div class="kv"><span>Decoded SimGrids</span><b>${city.simGridCounts.sint8 + city.simGridCounts.uint8 + city.simGridCounts.sint16 + city.simGridCounts.uint16 + city.simGridCounts.float32}</b></div>
  `;

  const d = city.demand;
  const bars = [
    ['R$', d.r1, '#4ade80'], ['R$$', d.r2, '#22c55e'], ['R$$$', d.r3, '#16a34a'],
    ['Cs$', d.c1, '#60a5fa'], ['Co$$', d.c2, '#3b82f6'], ['Co$$$', d.c3, '#2563eb'],
    ['I-R', d.i1, '#facc15'], ['I-M', d.i2, '#eab308'], ['I-HT', d.i3, '#ca8a04']
  ];
  document.getElementById('rciBars').innerHTML = bars.map(([lbl, val, col]) => {
    const pct = Math.min(Math.max((val + 4000) / 8000 * 100, 8), 100);
    return `<div class="bar-col"><div class="bar-fill" style="height:${pct}%;background:${col}"></div><div class="bar-lbl">${lbl}</div></div>`;
  }).join('');

  if (city.hasThumbnail) {
    document.getElementById('thumbBox').innerHTML = `<img src="/api/thumbnail.png?t=${Date.now()}" alt="SC4 Region Thumbnail" />`;
  } else {
    document.getElementById('thumbBox').innerHTML = '';
  }
}

async function fetchCity(ResetCam = false) {
  const res = await fetch('/api/city');
  city = await res.json();
  if (ResetCam) {
    if (city.buildings.length > 0) {
      let sx = 0, sy = 0, sz = 0;
      for (const b of city.buildings) {
        sx += 0.5 * (b[0] + b[3]);
        sy += b[1];
        sz += 0.5 * (b[2] + b[5]);
      }
      camTargetX = sx / city.buildings.length;
      camTargetY = sy / city.buildings.length;
      camTargetZ = sz / city.buildings.length;
    } else {
      camTargetX = city.cellCountX * 8;
      camTargetZ = city.cellCountZ * 8;
      camTargetY = city.seaLevel + 12;
    }
    camZoom = city.cellCountX >= 256 ? 0.65 : (city.cellCountX >= 128 ? 1.15 : 1.50);
  }
  rebuildMesh();
  updateUI();
  if (ResetCam) {
    loadAuthenticAssets();
    syncBudgetInputs();
  }
}

function zoneLabel(zt) {
  const names = {
    0: 'None', 1: 'R$', 2: 'R$$', 3: 'R$$$',
    4: 'Cs$', 5: 'Co$$', 6: 'Co$$$',
    7: 'I-R', 8: 'I-M', 9: 'I-HT',
    10: 'Military', 11: 'Airport', 12: 'Seaport', 13: 'Spaceport',
    14: 'Landfill', 15: 'Civic'
  };
  return names[Number(zt)] || ('Zone ' + zt);
}

function wealthLabel(w) {
  const n = Number(w) || 0;
  if (n <= 0) return '—';
  return '$'.repeat(Math.min(n, 3));
}

function escHtml(s) {
  return String(s ?? '').replace(/[&<>"']/g, ch => (
    { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[ch]
  ));
}

function renderTileQuery(q) {
  const name = q.hasBuilding && q.buildingName ? q.buildingName : '—';
  document.getElementById('queryBox').innerHTML = `
    <div class="kv"><span>Tile</span><b>${q.cellX}, ${q.cellZ}</b></div>
    <div class="kv"><span>Elevation</span><b>${Number(q.elevationY).toFixed(1)} m</b></div>
    <div class="kv"><span>Zone</span><b>${zoneLabel(q.zoneType)}</b></div>
    <div class="kv"><span>Building</span><b>${escHtml(name)}</b></div>
    <div class="kv"><span>Wealth</span><b>${wealthLabel(q.buildingWealth)}</b></div>
    <div class="kv"><span>Capacity</span><b>${Number(q.lotCapacity).toLocaleString()}</b></div>
    <div class="kv"><span>Land Value</span><b>${q.landValue}</b></div>
    <div class="kv"><span>Pollution</span><b>${q.pollution}</b></div>
    <div class="kv"><span>Crime</span><b>${q.crime}</b></div>
    <div class="kv"><span>Traffic</span><b>${q.traffic}</b></div>
    <div class="kv"><span>Power</span><b>${q.power}</b></div>
    <div class="kv"><span>Water</span><b>${q.water}</b></div>
    <div class="kv"><span>Mayor Rating</span><b>${q.mayorRating}</b></div>`;
}

async function syncBudgetInputs() {
  try {
    const budget = await (await fetch('/api/budget')).json();
    const rates = budget.taxRates || {};
    const apply = (id, value) => {
      const el = document.getElementById(id);
      if (!el || document.activeElement === el || value == null) return;
      el.value = Number(value).toFixed(1);
    };
    apply('taxR', rates.r1);
    apply('taxC', rates.cs1);
    apply('taxI', rates.id);
  } catch (_) {}
}

async function postTaxRate(purpose, input) {
  let pct = Number(input.value);
  if (!Number.isFinite(pct)) pct = 0;
  pct = Math.min(20, Math.max(0, pct));
  input.value = pct.toFixed(1);
  const rateTenths = Math.round(pct * 10);
  await fetch(`/api/budget/tax?purpose=${purpose}&wealth=1&rateTenths=${rateTenths}`, { method: 'POST' });
  await fetchCity(false);
}

function setRegionMode(enabled) {
  regionMode = enabled;
  document.getElementById('regionView').style.display = enabled ? 'block' : 'none';
  canvas.style.visibility = enabled ? 'hidden' : 'visible';
  document.querySelector('.sidebar').style.display = enabled ? 'none' : 'block';
  document.querySelector('.bottombar').style.display = enabled ? 'none' : 'flex';
  document.getElementById('btnRegion').classList.toggle('active', enabled);
  document.getElementById('btnRegion').textContent = enabled ? 'Enter Selected City' : 'Region View';
  if (enabled) renderRegion(selectedRegion);
}

async function renderRegion(regionIndex) {
  selectedRegion = Number(regionIndex);
  const res = await fetch(`/api/region?index=${selectedRegion}`);
  const region = await res.json();
  const map = document.getElementById('regionMap');
  const pad = 6;
  const unit = Math.max(24, Math.floor(Math.min(
    (map.clientWidth - pad * 2) / region.width,
    (map.clientHeight - pad * 2) / region.height
  )));
  const mapW = region.width * unit;
  const mapH = region.height * unit;
  const left = Math.max(pad, Math.floor((map.clientWidth - mapW) / 2));
  const top = Math.max(pad, Math.floor((map.clientHeight - mapH) / 2));
  document.getElementById('regionTitle').firstElementChild.textContent = `SC4 Region — ${region.name}`;
  document.getElementById('regionSummary').textContent =
    `${region.cities.length} cities · ${region.width}×${region.height} small-city tracts`;
  map.innerHTML = region.cities.map(c => `
    <button class="region-city" data-city="${c.index}" title="${c.name} — Pop ${c.resPop.toLocaleString()}"
      style="left:${left + c.tileX * unit}px;top:${top + c.tileZ * unit}px;width:${Math.max(unit, c.tractSizeX * unit)}px;height:${Math.max(unit, c.tractSizeZ * unit)}px">
      ${c.hasThumbnail ? `<img src="/api/region-thumbnail.png?city=${c.index}" alt="" loading="lazy">` : ''}
      <span class="region-city-label">${c.name}<br>Pop ${c.resPop.toLocaleString()}</span>
    </button>`).join('');
  map.querySelectorAll('.region-city').forEach(el => el.addEventListener('click', async () => {
    const index = Number(el.dataset.city);
    await fetch(`/api/load?index=${index}`, { method: 'POST' });
    document.getElementById('citySelect').value = String(index);
    await fetchCity(true);
    setRegionMode(false);
  }));
}

async function init() {
  const catRes = await fetch('/api/catalog');
  catalog = await catRes.json();

  const sel = document.getElementById('citySelect');
  sel.innerHTML = `<option value="-1">[OpenGFX] Procedural Starter City (128x128)</option>` +
    catalog.cities.map(c => `<option value="${c.index}">[${c.region}] ${c.name} (${c.sizeX}x${c.sizeZ}, Pop ${c.resPop.toLocaleString()})</option>`).join('');
  const regionSel = document.getElementById('regionSelect');
  regionSel.innerHTML = catalog.regions.map(r =>
    `<option value="${r.index}">${r.name} (${r.width}×${r.height})</option>`).join('');

  // Select Big City Tutorial if loaded
  const bigIdx = catalog.cities.findIndex(c => c.file.includes('Big City Tutorial'));
  if (bigIdx >= 0) sel.value = String(bigIdx);

  document.getElementById('sotStats').innerHTML = `
    <div class="kv"><span>Steam DBPF Archives</span><b>${catalog.archives.length} .dat files</b></div>
    <div class="kv"><span>Indexed TGI Entries</span><b>${catalog.totalIndexedEntries.toLocaleString()}</b></div>
    <div class="kv"><span>Shipped .sc4 Cities</span><b>${catalog.cities.length}</b></div>
    <div class="kv"><span>Mach-O C++ Symbols</span><b>${catalog.machO.totalSymbols.toLocaleString()}</b></div>
    <div class="kv"><span>GZCOM Factories</span><b>${catalog.machO.gzcomFactories}</b></div>
    <div class="kv"><span>Extracted CLSIDs</span><b>${catalog.machO.extractedCLSIDs}</b></div>
  `;

  await fetchCity(true);
}

document.getElementById('citySelect').addEventListener('change', async (e) => {
  await fetch(`/api/load?index=${e.target.value}`, { method: 'POST' });
  await fetchCity(true);
});
document.getElementById('btnRegion').addEventListener('click', () => {
  if (regionMode) {
    setRegionMode(false);
  } else {
    const current = catalog.cities.find(c => String(c.index) === document.getElementById('citySelect').value);
    const regionIndex = current ? catalog.regions.findIndex(r => r.name === current.region) : selectedRegion;
    selectedRegion = Math.max(0, regionIndex);
    document.getElementById('regionSelect').value = String(selectedRegion);
    setRegionMode(true);
  }
});
document.getElementById('regionSelect').addEventListener('change', (e) => {
  selectedRegion = Number(e.target.value);
  if (regionMode) renderRegion(selectedRegion);
});
document.getElementById('overlaySelect').addEventListener('change', (e) => {
  overlay = Number(e.target.value);
  rebuildMesh();
});
document.getElementById('btnRotL').addEventListener('click', () => { camRot = (camRot + 3) & 3; });
document.getElementById('btnRotR').addEventListener('click', () => { camRot = (camRot + 1) & 3; });
document.getElementById('btnStep').addEventListener('click', async () => {
  await fetch('/api/step?days=30', { method: 'POST' });
  await fetchCity(false);
});
document.getElementById('btnAuto').addEventListener('click', (e) => {
  autoSim = !autoSim;
  e.target.classList.toggle('active', autoSim);
  e.target.textContent = autoSim ? 'Sim: Running' : 'Sim: Paused';
});
document.getElementById('taxR').addEventListener('change', (e) => postTaxRate(1, e.target));
document.getElementById('taxC').addEventListener('change', (e) => postTaxRate(2, e.target));
document.getElementById('taxI').addEventListener('change', (e) => postTaxRate(5, e.target));
document.getElementById('btnBond').addEventListener('click', async () => {
  await fetch('/api/budget/bond?amount=10000', { method: 'POST' });
  await fetchCity(false);
});

let isPanning = false, lastX = 0, lastY = 0;
canvas.addEventListener('contextmenu', e => e.preventDefault());
canvas.addEventListener('mousedown', async (e) => {
  if (e.button === 2 || e.shiftKey) {
    isPanning = true; lastX = e.clientX; lastY = e.clientY;
  } else if (e.button === 0) {
    const t = screenToTile(e.clientX, e.clientY);
    if (!t) return;
    const tool = Number(document.getElementById('toolSelect').value);
    if (tool === 0) {
      const q = await (await fetch(`/api/query?x=${t[0]}&z=${t[1]}`)).json();
      renderTileQuery(q);
    } else if (tool > 0) {
      await fetch(`/api/tool?tool=${tool}&x0=${t[0]}&z0=${t[1]}&x1=${Math.min(t[0]+2, city.cellCountX-1)}&z1=${Math.min(t[1]+2, city.cellCountZ-1)}`, { method: 'POST' });
      await fetchCity(false);
    }
  }
});
window.addEventListener('mouseup', () => { isPanning = false; });
window.addEventListener('mousemove', (e) => {
  if (isPanning) {
    const dx = e.clientX - lastX, dy = e.clientY - lastY;
    lastX = e.clientX; lastY = e.clientY;
    const angle = (camRot * 0.5 + 0.25) * Math.PI;
    const c = Math.cos(angle), s = Math.sin(angle);
    camTargetX += (-dx * s + dy * c) * 1.6 / Math.max(0.25, camZoom);
    camTargetZ += (dx * c + dy * s) * 1.6 / Math.max(0.25, camZoom);
  } else if (city) {
    const t = screenToTile(e.clientX, e.clientY);
    if (t) {
      const h = city.heightmap[t[1] * (city.cellCountX + 1) + t[0]];
      const tIdx = (t[1] >> 2) * city.tractCountX + (t[0] >> 2);
      document.getElementById('hoverInfo').textContent =
        `Tile (${t[0]}, ${t[1]}) | Elev: ${h.toFixed(1)}m | LandValue: ${city.landValue[tIdx]||0} | Pollution: ${city.pollution[tIdx]||0} | Traffic: ${city.traffic[tIdx]||0}`;
    }
  }
});
canvas.addEventListener('wheel', (e) => {
  e.preventDefault();
  camZoom = Math.min(Math.max(camZoom * (e.deltaY < 0 ? 1.12 : 0.89), 0.28), 5.0);
}, { passive: false });
window.addEventListener('keydown', (e) => {
  if (e.key === 'q' || e.key === 'Q') camRot = (camRot + 3) & 3;
  if (e.key === 'e' || e.key === 'E') camRot = (camRot + 1) & 3;
  if (e.key === 'w' || e.key === 'W') { camTargetX -= 48; camTargetZ -= 48; }
  if (e.key === 's' || e.key === 'S') { camTargetX += 48; camTargetZ += 48; }
  if (e.key === 'a' || e.key === 'A') { camTargetX -= 48; camTargetZ += 48; }
  if (e.key === 'd' || e.key === 'D') { camTargetX += 48; camTargetZ -= 48; }
});

setInterval(async () => {
  if (autoSim && city) {
    await fetch('/api/step?days=5', { method: 'POST' });
    await fetchCity(false);
  }
}, 2500);

function renderLoop() {
  const dpr = window.devicePixelRatio || 1;
  const w = Math.floor(canvas.clientWidth * dpr);
  const h = Math.floor(canvas.clientHeight * dpr);
  if (canvas.width !== w || canvas.height !== h) {
    canvas.width = w; canvas.height = h;
  }
  if (regionMode) {
    requestAnimationFrame(renderLoop);
    return;
  }
  gl.viewport(0, 0, w, h);
  gl.enable(gl.DEPTH_TEST);
  gl.enable(gl.BLEND);
  gl.blendFunc(gl.SRC_ALPHA, gl.ONE_MINUS_SRC_ALPHA);
  gl.clearColor(0.11, 0.16, 0.24, 1.0);
  gl.clear(gl.COLOR_BUFFER_BIT | gl.DEPTH_BUFFER_BIT);

  if (vertexCount > 0) {
    gl.useProgram(prog);
    const vp = buildVP(w, h);
    gl.uniformMatrix4fv(gl.getUniformLocation(prog, 'uVP'), false, vp);
    gl.bindVertexArray(vao);
    gl.drawArrays(gl.TRIANGLES, 0, vertexCount);

    if (texturedBatches.length > 0) {
      gl.useProgram(texProg);
      gl.uniformMatrix4fv(gl.getUniformLocation(texProg, 'uVP'), false, vp);
      gl.uniform1i(gl.getUniformLocation(texProg, 'uTexture'), 0);
      gl.activeTexture(gl.TEXTURE0);
      for (const batch of texturedBatches) {
        gl.bindTexture(gl.TEXTURE_2D, batch.texture);
        gl.bindVertexArray(batch.vao);
        gl.drawArrays(gl.TRIANGLES, 0, batch.count);
      }
    }
  }
  requestAnimationFrame(renderLoop);
}

init();
requestAnimationFrame(renderLoop);
</script>
</body>
</html>
)HTML";

static int GetQueryParamInt(const std::string& path, const std::string& key, int fallback = 0) {
    std::string needle = key + "=";
    auto pos = path.find(needle);
    if (pos == std::string::npos) return fallback;
    return std::atoi(path.c_str() + pos + needle.size());
}

static Uint32 GetQueryParamUint(const std::string& path, const std::string& key, Uint32 fallback = 0) {
    std::string needle = key + "=";
    auto pos = path.find(needle);
    if (pos == std::string::npos) return fallback;
    const char* value = path.c_str() + pos + needle.size();
    return static_cast<Uint32>(std::strtoul(value, nullptr, 0));
}


static bool IsLoopbackHostHeader(const char* request) {
    std::istringstream lines(request);
    std::string line;
    std::getline(lines, line);
    while (std::getline(lines, line) && line != "\r" && !line.empty()) {
        if (line.size() < 5) continue;
        std::string name = line.substr(0, 5);
        for (auto& c : name) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        if (name != "host:") continue;
        std::string host = line.substr(5);
        host.erase(0, host.find_first_not_of(" \t"));
        host.erase(host.find_last_not_of(" \t\r") + 1);
        host = host.substr(0, host.rfind(':') == std::string::npos ? host.size() : host.rfind(':'));
        for (auto& c : host) c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        return host == "localhost" || host == "127.0.0.1";
    }
    return false;
}

int RunHTTPBrowserServer(cDBPFSystem& dbpfSys, cSC4City& city, int port) {
    MachOBinarySummary machO{};
    if (!dbpfSys.MachOBinaryPath().empty()) {
        machO = InspectSC4MachOBinary(dbpfSys.MachOBinaryPath());
    }

    int serverFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        std::perror("socket");
        return 1;
    }

    int opt = 1;
    ::setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = htons(static_cast<uint16_t>(port));

    if (::bind(serverFd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::perror("bind");
        ::close(serverFd);
        return 1;
    }

    if (::listen(serverFd, 16) < 0) {
        std::perror("listen");
        ::close(serverFd);
        return 1;
    }

    std::printf("================================================================\n");
    std::printf(" OpenSC4 Browser Server listening at http://localhost:%d\n", port);
    std::printf(" Mounted Archives: %zu (%zu total TGI entries)\n",
                dbpfSys.Archives().size(), dbpfSys.TotalIndexedEntries());
    std::printf(" Loaded City: %s (%dx%d, %zu buildings, %zu networks)\n",
                city.CityName().c_str(), city.CellCountX(), city.CellCountZ(),
                city.Buildings().size(), city.Networks().size());
    std::printf("================================================================\n");
    std::fflush(stdout);

    std::string cachedCatalogJson = BuildCatalogJson(dbpfSys, machO);

    while (true) {
        sockaddr_in clientAddr{};
        socklen_t clientLen = sizeof(clientAddr);
        int clientFd = ::accept(serverFd, reinterpret_cast<sockaddr*>(&clientAddr), &clientLen);
        if (clientFd < 0) continue;

        char reqBuf[4096];
        ssize_t n = ::read(clientFd, reqBuf, sizeof(reqBuf) - 1);
        if (n <= 0) {
            ::close(clientFd);
            continue;
        }
        reqBuf[n] = '\0';

        std::string method, path;
        {
            std::istringstream iss(reqBuf);
            iss >> method >> path;
        }

        // Loopback binding alone still admits DNS-rebinding pages, so the
        // Host header must name this machine too.
        if (!IsLoopbackHostHeader(reqBuf)) {
            static const char kForbidden[] =
                "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            ::write(clientFd, kForbidden, sizeof(kForbidden) - 1);
            ::close(clientFd);
            continue;
        }

        auto sendResponse = [&](const char* contentType, const void* body, std::size_t len) {
            char hdr[512];
            int hlen = std::snprintf(hdr, sizeof(hdr),
                "HTTP/1.1 200 OK\r\n"
                "Content-Type: %s\r\n"
                "Content-Length: %zu\r\n"
                "Connection: close\r\n\r\n",
                contentType, len);
            ::write(clientFd, hdr, hlen);
            if (len > 0) ::write(clientFd, body, len);
        };

        if (path == "/" || path.rfind("/index.html", 0) == 0) {
            sendResponse("text/html; charset=utf-8", kBrowserAppHTML, std::strlen(kBrowserAppHTML));
        } else if (path.rfind("/api/catalog", 0) == 0) {
            sendResponse("application/json", cachedCatalogJson.data(), cachedCatalogJson.size());
        } else if (path.rfind("/api/region-thumbnail.png", 0) == 0) {
            int idx = GetQueryParamInt(path, "city", -1);
            std::vector<Uint8> png;
            if (idx >= 0) dbpfSys.ReadCityThumbnailPNG(static_cast<std::size_t>(idx), png);
            sendResponse("image/png", png.data(), png.size());
        } else if (path.rfind("/api/region", 0) == 0) {
            int idx = GetQueryParamInt(path, "index", 0);
            std::string body = BuildRegionJson(dbpfSys, idx < 0 ? 0 : static_cast<std::size_t>(idx));
            sendResponse("application/json", body.data(), body.size());
        } else if (path.rfind("/api/s3d", 0) == 0) {
            Uint32 instanceId = GetQueryParamUint(path, "id");
            std::string body = BuildS3DJson(dbpfSys, instanceId);
            sendResponse("application/json", body.data(), body.size());
        } else if (path.rfind("/api/fsh.png", 0) == 0) {
            Uint32 instanceId = GetQueryParamUint(path, "id");
            std::vector<Uint8> bytes;
            std::vector<Uint8> png;
            cFSHImage image;
            if (dbpfSys.LoadAssetByTypeAndInstance(kTypeFSH, instanceId, bytes) &&
                image.Parse(bytes.data(), bytes.size())) {
                png = EncodeRGBA8ToPNG(image.width, image.height, image.rgbaPixels.data());
            }
            sendResponse("image/png", png.data(), png.size());
        } else if (path.rfind("/api/city", 0) == 0) {
            std::string body = BuildCityStateJson(city);
            sendResponse("application/json", body.data(), body.size());
        } else if (path.rfind("/api/thumbnail.png", 0) == 0) {
            const auto& png = city.PngThumbnail();
            sendResponse("image/png", png.data(), png.size());
        } else if (path.rfind("/api/load", 0) == 0) {
            int idx = GetQueryParamInt(path, "index", -1);
            if (idx < 0) {
                city.GenerateProceduralCity(128, 128);
            } else if (static_cast<std::size_t>(idx) < dbpfSys.RegionCities().size()) {
                city.LoadFromSC4File(dbpfSys.RegionCities()[idx].fullPath, &dbpfSys);
            }
            const char* ok = "{\"ok\":true}";
            sendResponse("application/json", ok, std::strlen(ok));
        } else if (path.rfind("/api/step", 0) == 0) {
            int days = std::clamp(GetQueryParamInt(path, "days", 1), 1, 365);
            city.StepSimulation(days);
            const char* ok = "{\"ok\":true}";
            sendResponse("application/json", ok, std::strlen(ok));
        } else if (path.rfind("/api/tool", 0) == 0) {
            int tool = GetQueryParamInt(path, "tool", 0);
            int x0 = GetQueryParamInt(path, "x0", 0);
            int z0 = GetQueryParamInt(path, "z0", 0);
            int x1 = GetQueryParamInt(path, "x1", x0);
            int z1 = GetQueryParamInt(path, "z1", z0);
            city.ApplyTool(static_cast<ToolType>(tool), x0, z0, x1, z1);
            const char* ok = "{\"ok\":true}";
            sendResponse("application/json", ok, std::strlen(ok));
        } else if (path.rfind("/api/query", 0) == 0) {
            int x = GetQueryParamInt(path, "x", 0);
            int z = GetQueryParamInt(path, "z", 0);
            const TileQueryReport q = city.QueryTile(x, z);
            std::ostringstream qss;
            qss << std::fixed << std::setprecision(1)
                << "{\"cellX\":" << q.cellX
                << ",\"cellZ\":" << q.cellZ
                << ",\"elevationY\":" << SafeJsonFloat(q.elevationY, 250.0f)
                << ",\"underwater\":" << (q.underwater ? "true" : "false")
                << ",\"zoneType\":" << int(q.zoneType)
                << ",\"hasNetwork\":" << (q.hasNetwork ? "true" : "false")
                << ",\"networkType\":" << int(q.networkType)
                << ",\"hasBuilding\":" << (q.hasBuilding ? "true" : "false")
                << ",\"buildingName\":\"" << JsonEscape(q.buildingName) << "\""
                << ",\"buildingWealth\":" << int(q.buildingWealth)
                << ",\"lotCapacity\":" << q.lotCapacity
                << ",\"landValue\":" << int(q.landValue)
                << ",\"pollution\":" << int(q.pollution)
                << ",\"crime\":" << int(q.crime)
                << ",\"traffic\":" << int(q.traffic)
                << ",\"power\":" << int(q.power)
                << ",\"water\":" << int(q.water)
                << ",\"mayorRating\":" << int(q.mayorRating)
                << "}";
            std::string body = qss.str();
            sendResponse("application/json", body.data(), body.size());
        } else if (path.rfind("/api/budget/tax", 0) == 0) {
            int purpose = GetQueryParamInt(path, "purpose", 1);
            int wealth = GetQueryParamInt(path, "wealth", 1);
            int rateTenths = GetQueryParamInt(path, "rateTenths", 90);
            bool ok = city.SetTaxRate(purpose, wealth, static_cast<Float32>(rateTenths) * 0.1f);
            std::string body = std::string("{\"ok\":") + (ok ? "true" : "false") + "}";
            sendResponse("application/json", body.data(), body.size());
        } else if (path.rfind("/api/budget/bond", 0) == 0) {
            int amount = GetQueryParamInt(path, "amount", 10000);
            bool ok = city.IssueBond(amount);
            std::ostringstream bss;
            bss << "{\"ok\":" << (ok ? "true" : "false")
                << ",\"funds\":" << city.TreasuryFunds()
                << ",\"monthlyBondPayments\":" << city.GetTotalMonthlyBondPayments()
                << "}";
            std::string body = bss.str();
            sendResponse("application/json", body.data(), body.size());
        } else if (path.rfind("/api/budget", 0) == 0) {
            std::ostringstream bss;
            bss << std::fixed << std::setprecision(1)
                << "{\"funds\":" << city.TreasuryFunds()
                << ",\"monthlyBondPayments\":" << city.GetTotalMonthlyBondPayments()
                << ",\"taxRates\":{"
                << "\"r1\":" << city.GetTaxRate(1, 1) << ","
                << "\"r2\":" << city.GetTaxRate(1, 2) << ","
                << "\"r3\":" << city.GetTaxRate(1, 3) << ","
                << "\"cs1\":" << city.GetTaxRate(2, 1) << ","
                << "\"cs2\":" << city.GetTaxRate(2, 2) << ","
                << "\"cs3\":" << city.GetTaxRate(2, 3) << ","
                << "\"co2\":" << city.GetTaxRate(3, 2) << ","
                << "\"co3\":" << city.GetTaxRate(3, 3) << ","
                << "\"ir\":" << city.GetTaxRate(4, 1) << ","
                << "\"id\":" << city.GetTaxRate(5, 1) << ","
                << "\"im\":" << city.GetTaxRate(6, 2) << ","
                << "\"iht\":" << city.GetTaxRate(7, 3)
                << "}}";
            std::string body = bss.str();
            sendResponse("application/json", body.data(), body.size());
        } else {
            const char* notFound = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
            ::write(clientFd, notFound, std::strlen(notFound));
        }

        ::close(clientFd);
    }

    ::close(serverFd);
    return 0;
}

} // namespace opensc4
