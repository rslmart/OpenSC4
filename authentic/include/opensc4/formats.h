#pragma once

#include "opensc4/gzcom.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace opensc4 {

// ============================================================================
// 1. Exemplar & Cohort (0x6534284a / 0x05342861: EQZB1###, EQZT1###, CQZB1###)
// ============================================================================
enum class ExemplarValueType : Uint16 {
    Uint8   = 0x0100,
    Uint16  = 0x0200,
    Uint32  = 0x0300,
    Sint8   = 0x0400,
    Sint16  = 0x0500,
    Sint32  = 0x0700,
    Sint64  = 0x0800,
    Float32 = 0x0900,
    Bool    = 0x0B00,
    String  = 0x0C00
};

// Well-known Maxis Exemplar Property IDs
constexpr Uint32 kPropExemplarType       = 0x00000010u;
constexpr Uint32 kPropExemplarName       = 0x00000020u;
constexpr Uint32 kPropOccupantSize       = 0x27812810u; // Float32[3]: width, height, depth (m)
constexpr Uint32 kPropResourceKeyType0   = 0x27812820u;
constexpr Uint32 kPropResourceKeyType1   = 0x27812821u;
constexpr Uint32 kPropWealth             = 0x27812832u;
constexpr Uint32 kPropCapacitySatisfied  = 0x27812834u;
constexpr Uint32 kPropPowerConsumed      = 0x27812854u;
constexpr Uint32 kPropWaterConsumed      = 0x27812851u;
constexpr Uint32 kPropPlopCost           = 0x49cac341u;
constexpr Uint32 kPropBulldozeCost       = 0xea54d286u;

struct ExemplarProperty {
    Uint32 id               = 0;
    Uint16 valueType        = 0;
    Uint16 keyType          = 0;
    std::string stringValue;
    std::vector<Sint64> intValues;
    std::vector<Float32> floatValues;
};

struct cGZExemplar {
    bool isCohort     = false;
    bool isTextFormat = false;
    cSC4TGI parentCohort{};
    std::string name;
    std::unordered_map<Uint32, ExemplarProperty> properties;

    bool Parse(const Uint8* data, std::size_t len);
    std::string GetString(Uint32 propId, const std::string& fallback = "") const;
    Sint64 GetInt(Uint32 propId, Sint64 fallback = 0) const;
    Float32 GetFloat(Uint32 propId, Float32 fallback = 0.0f) const;
};

// ============================================================================
// 2. S3D 3D Model (0x5ad0e817: 3DMD -> HEAD, VERT, INDX, PRIM, MATS, ANIM)
// ============================================================================
struct S3DVertex {
    Float32 x = 0.0f;
    Float32 y = 0.0f;
    Float32 z = 0.0f;
    Float32 u = 0.0f;
    Float32 v = 0.0f;
    Uint32 color = 0xFFFFFFFFu;
};

struct S3DMaterial {
    Uint32 flags     = 0;
    Uint32 textureId = 0; // FSH Instance ID
};

struct cS3DModel {
    Uint16 majorVersion = 1;
    Uint16 minorVersion = 5;
    std::vector<S3DVertex> vertices;
    std::vector<Uint16> indices;
    std::vector<S3DMaterial> materials;

    bool Parse(const Uint8* data, std::size_t len);
};

// ============================================================================
// 3. FSH Texture (0x7ab50e44: SHPI -> DXT1, DXT3, ARGB8888, RGB888, Indexed8)
// ============================================================================
struct cFSHImage {
    Uint16 width      = 0;
    Uint16 height     = 0;
    Uint8 recordCode  = 0; // 0x60=DXT1, 0x61=DXT3, 0x7D=ARGB8888, 0x7F=RGB888, 0x7B=Indexed8
    std::string tag;
    std::vector<Uint8> rgbaPixels; // width * height * 4 (RGBA8)

    bool Parse(const Uint8* data, std::size_t len);
};

// ============================================================================
// 4. LTEXT Localized String (0x2026960b) & Uncompressed PNG Encoder
// ============================================================================
std::string DecodeLTEXT(const Uint8* data, std::size_t len);
std::vector<Uint8> EncodeRGBA8ToPNG(Uint16 width, Uint16 height, const Uint8* rgbaPixels);

// ============================================================================
// 5. Mach-O Binary Inspector (Sim City 4 Deluxe Editionsub)
// Extracts all 62,434 symbols and 233 GZCOM_*CLSID() / GetGZCLSID() constants
// ============================================================================
struct MachOSymbolInfo {
    Uint64 address = 0;
    std::string name;
};

struct MachOBinarySummary {
    bool valid                    = false;
    Uint32 totalSymbols           = 0;
    Uint32 gzcomFactoryCount      = 0;
    std::unordered_map<Uint32, std::string> extractedCLSIDs;
    std::vector<MachOSymbolInfo> sc4ClassSymbols;
};

MachOBinarySummary InspectSC4MachOBinary(const std::string& machOPath);

} // namespace opensc4
