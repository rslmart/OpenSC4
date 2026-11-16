#pragma once

// Symbol-indexed decompilation harness for the macOS x86_64 SimCity 4 Deluxe
// Mach-O binary ("Sim City 4 Deluxe Editionsub").
//
// Parses the 64-bit Mach-O LC_SYMTAB symbol table and __TEXT,__text machine
// code directly (no external tools), classifies all 43,333 __TEXT,__text
// symbols by subsystem, C++ class, and 4-Stage Decompilation Tier:
//   - Tier 1: Core SC4 Simulation, World, Networks, Automata, Advisors,
//             Disasters & Terrain (algorithmic methods)
//   - Tier 2: UI Windows, 3D View/Render, Hit/Fresh Audio & Lua/Visual Effects
//   - Tier 3: Auto-Verified GZCOM Boilerplate & <=48B Call-Free x86_64 Leaf
//             Accessors/Stubs (verified directly from __TEXT,__text bytes)
//   - Tier 4: Superseded Gonzo/Rizzo OS Framework, Aspyr DirectX7->OpenGL 1.x
//             Compatibility Layer, Legacy 2D GZWin Widgets, ABI Thunks & STL
//             (superseded by OpenSC4's C++20 / OpenGL 3.3 / WebGL2 runtime)

#include "opensc4/gzcom.h"

#include <array>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace opensc4 {

enum class SC4Subsystem : Uint8 {
    SimulatorsAndDevelopers = 0, // cSC4*Simulator, cSC4*Developer, cSC4Demand*, Budget, Ordinances
    CityLotZoneOccupants,        // cSC4City*, cSC4Lot*, cSC4Zone*, cSC4*Occupant*, Park/Prop/Landfill
    NetworkTrafficPathfinder,    // cSC4Network*, cSC4Traffic*, cSC4PathFinder*, cSC4EdgeConnection*, cSC4Tile*
    AutomataAndVehicles,         // cSC4Automat*, tAutomataState*, Vehicles/Pedestrians/Aircraft/Watercraft/Dispatch
    AdvisorsMySimTutorial,       // cSC4Advisor*, cSC4Advice*, cSC4MySim*, cSC4Tutorial*
    Disasters,                   // cSC4Disaster*, Volcano/Tornado/Earthquake/Fire/Riot/UFO/Autosaurus
    UIViewsAndTools,             // cSC4View*, cSC4Win*, cSC4*Tool*, 3DRender/Camera/Clock/ModelMaker/UI
    TerrainAndS3D,               // cSTE* terrain & cS3D* / nS3D* 3D scene graph
    GonzoRizzoFramework,         // cGZ*, cRZ*, cIGZ*, nGZ*, nRZ*, GZCOM_*, cSCRes*
    AudioHitFresh,               // cHit*, cFresh*, cGZSnd*, IDirectSound*
    ScriptingAndEffects,         // cSCLua*, lua*, cSC4Effects*, cSC4*Description*, cSLToken*
    MacDirectXPortLayer,         // nSGLDX7*, CDirect3D*, CDirectDraw*, nsTexelOp*
    OtherGameAndFreeFunctions,   // remaining cSC4* classes and engine C/C++ free functions
    // Compiler-generated / standard library symbols (tracked separately from game targets):
    ABIThunks,                   // non-virtual / virtual thunks (__ZThn*, __ZTv*)
    CppSTLRuntime,               // std::__1::*, operator new/delete, __clang_call_terminate
    StaticInitializers,          // ___cxx_global_var_init*, __GLOBAL__*, guard variables
    Count
};

enum class SC4DecompTier : Uint8 {
    Tier1_CoreSimulationAndWorld = 0,          // Essential 1:1 SimCity 4 simulation & world algorithms
    Tier2_ToolsViewAudioLua = 1,               // Interactive tools, 3D scene/view, audio & Lua/effects
    Tier3_AutoVerifiedCOMAndLeafAccessors = 2, // GZCOM boilerplate & <=48B call-free x86_64 leaf accessors
    Tier4_SupersededFrameworkAndRuntime = 3,   // Superseded cGZ/cRZ OS/UI, DX7 shim, C/STL runtime & thunks
    Count
};

constexpr std::size_t kSC4SubsystemCount = static_cast<std::size_t>(SC4Subsystem::Count);
constexpr std::size_t kSC4TargetSubsystemCount =
    static_cast<std::size_t>(SC4Subsystem::ABIThunks); // first 13 subsystems are target game/engine code
constexpr std::size_t kSC4DecompTierCount = static_cast<std::size_t>(SC4DecompTier::Count);

std::string_view SC4SubsystemName(SC4Subsystem sub) noexcept;
std::string_view SC4DecompTierName(SC4DecompTier tier) noexcept;
bool IsTargetGameSubsystem(SC4Subsystem sub) noexcept;

// Inspects raw x86_64 machine code bytes of a __TEXT,__text symbol and returns
// true if it is a deterministic <=48-byte call-free leaf accessor/stub ending
// in ret (0xc3) or tail-call jmp (0xeb / 0xe9) with no internal call instructions.
bool IsCallFreeLeafX86(std::span<const Uint8> codeBytes) noexcept;

struct MachOTextSymbol {
    Uint64 address = 0;          // VM address in __TEXT,__text (0x1000018a0..0x10092ad40)
    Uint32 byteSize = 0;         // nextSymbol.address - address
    SC4Subsystem subsystem = SC4Subsystem::OtherGameAndFreeFunctions;
    SC4DecompTier tier = SC4DecompTier::Tier4_SupersededFrameworkAndRuntime;
    bool isCallFreeLeaf = false; // verified directly from __TEXT,__text machine code
    bool decompiled = false;
    std::string mangledName;     // raw LC_SYMTAB string (e.g. "__ZN8cSC4City4InitEv")
    std::string classOrPrefix;   // parsed outer class/namespace (or "<free>")
    std::string memberName;      // parsed method/function identifier
};

struct SubsystemProgress {
    SC4Subsystem subsystem = SC4Subsystem::SimulatorsAndDevelopers;
    Uint32 totalSymbols = 0;
    Uint32 decompiledSymbols = 0;
    Uint64 totalBytes = 0;
    Uint64 decompiledBytes = 0;
};

struct TierProgress {
    SC4DecompTier tier = SC4DecompTier::Tier1_CoreSimulationAndWorld;
    Uint32 totalSymbols = 0;
    Uint32 decompiledSymbols = 0;
    Uint64 totalBytes = 0;
    Uint64 decompiledBytes = 0;
};

struct DecompilationCatalogReport {
    Uint32 totalTextSymbols = 0;           // all __TEXT,__text symbols (43,333)
    Uint32 targetEngineSymbols = 0;        // non-thunk, non-STL, non-static-init (30,825)
    Uint32 decompiledTargetSymbols = 0;    // target symbols decompiled / auto-verified in OpenSC4
    Uint32 autoVerifiedLeafSymbols = 0;    // <=48B call-free x86_64 leaf accessors verified from binary
    Uint64 totalTextBytes = 0;
    Uint64 targetEngineBytes = 0;
    Uint64 decompiledTargetBytes = 0;
    Uint32 registeredAddressesChecked = 0; // size of RegisteredDecompiledAddresses()
    Uint32 registeredAddressesValid = 0;   // how many resolved to exact __TEXT,__text entry points
    std::array<SubsystemProgress, kSC4SubsystemCount> bySubsystem{};
    std::array<TierProgress, kSC4DecompTierCount> byTier{};
};

// Sorted list of explicit __TEXT,__text function entry addresses decompiled in OpenSC4.
std::span<const Uint64> RegisteredDecompiledAddresses() noexcept;

// Classify an Itanium C++ ABI mangled symbol name and extract (classOrPrefix, memberName).
SC4Subsystem ClassifyMachOSymbol(std::string_view mangled,
                                 std::string& outClassOrPrefix,
                                 std::string& outMemberName);

// Returns true if the symbol is covered by RegisteredDecompiledAddresses() or by
// OpenSC4's decompiled class/subsystem implementations.
bool IsSymbolDecompiled(Uint64 address, std::string_view mangled) noexcept;

// Parse all __TEXT,__text symbols from the 64-bit Mach-O binary at machOPath.
bool LoadMachOTextSymbols(const std::string& machOPath,
                          std::vector<MachOTextSymbol>& outSymbols);

// Build the full subsystem-by-subsystem and tier-by-tier decompilation progress report.
bool BuildDecompilationCatalogReport(const std::string& machOPath,
                                     DecompilationCatalogReport& outReport);

} // namespace opensc4
