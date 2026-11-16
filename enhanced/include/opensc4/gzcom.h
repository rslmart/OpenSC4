#pragma once

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <string>
#include <vector>
#include <unordered_map>

namespace opensc4 {

// Maxis Gonzo/Rizzo (GZCOM) fundamental types matching SC4SimGrid.cpp.inl
using Bool8   = int8_t;
using Uint8   = uint8_t;
using Sint8   = int8_t;
using Uint16  = uint16_t;
using Sint16  = int16_t;
using Uint32  = uint32_t;
using Sint32  = int32_t;
using Uint64  = uint64_t;
using Sint64  = int64_t;
using Float32 = float;
using Float64 = double;

using GZCLSID  = Uint32;
using GZIID    = Uint32;
using GZREFIID = Uint32;

// Standard Gonzo Interface IDs from SC4SimGrid.cpp.inl
constexpr GZIID GZIID_cIGZUnknown       = 0x00000001u;
constexpr GZIID kGZIID_cIGZSerializable = 0x05342912u;
constexpr GZIID kGZIID_cISC4SimGrid     = 0x49b9e600u;

// SimGrid Class IDs verified from Sim City 4 Deluxe Editionsub & .sc4 subfiles
constexpr GZCLSID kGZCLSID_cSC4SimGridBool8   = 0x49b9e601u;
constexpr GZCLSID kGZCLSID_cSC4SimGridSint8   = 0x49b9e602u;
constexpr GZCLSID kGZCLSID_cSC4SimGridUint8   = 0x49b9e603u;
constexpr GZCLSID kGZCLSID_cSC4SimGridSint16  = 0x49b9e604u;
constexpr GZCLSID kGZCLSID_cSC4SimGridUint16  = 0x49b9e605u;
constexpr GZCLSID kGZCLSID_cSC4SimGridSint32  = 0x49b9e606u;
constexpr GZCLSID kGZCLSID_cSC4SimGridUint32  = 0x49b9e607u;
constexpr GZCLSID kGZCLSID_cSC4SimGridSint64  = 0x49b9e608u;
constexpr GZCLSID kGZCLSID_cSC4SimGridUint64  = 0x49b9e609u;
constexpr GZCLSID kGZCLSID_cSC4SimGridFloat32 = 0x49b9e60au;
constexpr GZCLSID kGZCLSID_cSC4SimGridFloat64 = 0x49b9e60bu;

// Core SC4 Class IDs extracted directly from Sim City 4 Deluxe Editionsub
constexpr GZCLSID kCLSID_cSC4ReadRegionalCity     = 0xca027edbu;
constexpr GZCLSID kCLSID_cSC4RegionalCity         = 0x6a037c24u;
constexpr GZCLSID kCLSID_cSC4City                 = 0x8990c372u;
constexpr GZCLSID kCLSID_cSTETerrain              = 0xe98f9525u;
constexpr GZCLSID kCLSID_cSTETerrainView3D        = 0xc9b84e10u;
constexpr GZCLSID kCLSID_cSTETerrainHeightmap     = 0xa9dd6ff4u;
constexpr GZCLSID kCLSID_cSC4OccupantManager      = 0x098f964du;
constexpr GZCLSID kCLSID_cSC4Occupant             = 0xa9bc9ab6u;
constexpr GZCLSID kCLSID_cSC4Lot                  = 0xc9bd5d4au;
constexpr GZCLSID kCLSID_cSC4BuildingOccupant     = 0xa9bd882du;
constexpr GZCLSID kCLSID_cSC4NetworkOccupant      = 0xc9c05c6eu;
constexpr GZCLSID kCLSID_cSC4NetworkPreBuilt      = 0x49c1a034u;
constexpr GZCLSID kCLSID_cSC4NetworkBridge        = 0x49cc1bcdu;
constexpr GZCLSID kCLSID_cSC4NetworkTunnel        = 0x8a4bd52bu;
constexpr GZCLSID kCLSID_cSC4PropOccupant         = 0x2977aa47u;
constexpr GZCLSID kCLSID_cSC4FloraOccupant        = 0xa9c05c85u;
constexpr GZCLSID kCLSID_cSC4PipeOccupant         = 0x49c05b9fu;
constexpr GZCLSID kCLSID_cSC4SubwayOccupant       = 0xca16374fu;
constexpr GZCLSID kCLSID_cSC4PowerLineOccupant    = 0xc9c05c5du;
constexpr GZCLSID kCLSID_cSC4PowerPoleOccupant    = 0x09c05c6au;
constexpr GZCLSID kCLSID_cSC4PylonOccupant        = 0x49b6d69bu;
constexpr GZCLSID kCLSID_cSC4FoundationOccupant   = 0x49c05c8fu;
constexpr GZCLSID kCLSID_cSC4RetainingWall        = 0x49c05c9fu;
constexpr GZCLSID kCLSID_cSC4LotBaseTexture       = 0xc97f987cu;
constexpr GZCLSID kCLSID_cSC4LandfillOccupant     = 0x8a22bebeu;
constexpr GZCLSID kCLSID_cSC4ZoneManager          = 0x298f9b2du;
constexpr GZCLSID kCLSID_cSC4NetworkManager       = 0xc990bd46u;
constexpr GZCLSID kCLSID_cSC4NetworkLotManager    = 0xe9a6cd3bu;
constexpr GZCLSID kCLSID_cSC4Simulator            = 0x2990c1e5u;
constexpr GZCLSID kCLSID_cSC424HourClock          = 0xa7e7f929u;
constexpr GZCLSID kCLSID_cSC4DemandSimulator      = 0x2990bff2u;
constexpr GZCLSID kCLSID_cSC4Demand               = 0x498f9a1au;
constexpr GZCLSID kGZCLSID_cSC4DemandSerializable = 0x8990c12du;
constexpr GZCLSID kCLSID_cSC4ResidentialSimulator = 0x4990c013u;
constexpr GZCLSID kCLSID_cSC4CommercialSimulator  = 0x8990c080u;
constexpr GZCLSID kCLSID_cSC4IndustrialSimulator  = 0xa990c022u;
constexpr GZCLSID kCLSID_cSC4TrafficSimulator     = 0x6990c1aau;
constexpr GZCLSID kCLSID_cSC4PollutionSimulator   = 0x8990c065u;
constexpr GZCLSID kCLSID_cSC4PlumbingSimulator    = 0x0990c075u;
constexpr GZCLSID kCLSID_cSC4PowerSimulator       = 0x0990c005u;
constexpr GZCLSID kCLSID_cSC4PoliceSimulator      = 0x6990c04fu;
constexpr GZCLSID kCLSID_cSC4FireProtectionSim    = 0x2990c05au;
constexpr GZCLSID kCLSID_cSC4CrimeSimulator       = 0x8990c09au;
constexpr GZCLSID kCLSID_cSC4AuraSimulator        = 0x0990bdd8u;
constexpr GZCLSID kCLSID_cSC4BudgetSimulator      = 0xe990be01u;
constexpr GZCLSID kCLSID_cSC4LandValueSimulator   = 0xe990c18eu;
constexpr GZCLSID kCLSID_cSC4FlammabilitySim      = 0xe990c045u;
constexpr GZCLSID kCLSID_cSC4FloraSimulator       = 0xc990c034u;
constexpr GZCLSID kCLSID_cSC4WeatherSimulator     = 0x2990c1bcu;
constexpr GZCLSID kCLSID_cSC4CivicBuildingSim     = 0x0a413481u;
constexpr GZCLSID kCLSID_cSC4NeighborsSimulator   = 0xc990c0a4u;
constexpr GZCLSID kCLSID_cSC4NeighborConnection   = 0x0a134f24u;
constexpr GZCLSID kCLSID_cSC4NeighborDeal         = 0x2a134f42u;
constexpr GZCLSID kCLSID_cSC4OrdinanceSimulator   = 0xa990c08eu;
constexpr GZCLSID kCLSID_cSC4MySimAgentSimulator  = 0x29e37decu;
constexpr GZCLSID kCLSID_cSC4BuildingDevSim       = 0x8990be7au;
constexpr GZCLSID kCLSID_cSC4LotConfigurationManager = 0x299062c0u;
constexpr GZCLSID kCLSID_cSC4LotDeveloper         = 0xa990bfe0u;
constexpr GZCLSID kCLSID_cSC4ZoneDeveloper        = 0x498f9b01u;
constexpr GZCLSID kCLSID_cSC4TractDeveloper       = 0x2990c142u;
constexpr GZCLSID kCLSID_cSC4GrowthDeveloper      = 0x0990c138u;
constexpr GZCLSID kCLSID_cSC4PropManager          = 0xa990bd10u;
constexpr GZCLSID kCLSID_cSC4ParkManager          = 0xaa3fe6bcu;
constexpr GZCLSID kCLSID_cSC4AirportDeveloper     = 0xaa2049a1u;
constexpr GZCLSID kCLSID_cSC4SeaportDeveloper     = 0xea230a28u;
constexpr GZCLSID kCLSID_cSC4LandfillDeveloper    = 0xea09d4aeu;
constexpr GZCLSID kCLSID_cSC4TutorialSystem       = 0xaa3836fbu;
constexpr GZCLSID kCLSID_cSC4Demolition           = 0x0990c38au;
constexpr GZCLSID kCLSID_cSC4CityDetailManager    = 0xead4ba36u;
constexpr GZCLSID kCLSID_cSC43DRender             = 0x89c6271bu;

// Well-known DBPF Asset Type IDs
constexpr Uint32 kTypeUI       = 0x00000000u;
constexpr Uint32 kTypeCohort   = 0x05342861u;
constexpr Uint32 kTypeLTEXT    = 0x2026960bu;
constexpr Uint32 kTypeS3D      = 0x5ad0e817u;
constexpr Uint32 kTypeExemplar = 0x6534284au;
constexpr Uint32 kTypeFSH      = 0x7ab50e44u;
constexpr Uint32 kTypePNG      = 0x8a2482b9u;
constexpr Uint32 kTypeLUA      = 0xca63e2a3u;
constexpr Uint32 kTypeWAV      = 0x2026960eu;
constexpr Uint32 kTypeDIR      = 0xe86b1eefu;
constexpr Uint32 kTypeRUL      = 0x0a5bcf4bu;
constexpr Uint32 kGroupRUL     = 0xaa5bcf57u;
constexpr Uint32 kTypeINI      = 0x00000000u;
constexpr Uint32 kGroupNetworkIni = 0x8a5971c5u;
constexpr Uint32 kInstNetworkIni  = 0x8a5993b9u;

// 96-bit Type / Group / Instance key used across all Maxis DBPF files
struct cSC4TGI {
    Uint32 type     = 0;
    Uint32 group    = 0;
    Uint32 instance = 0;

    bool operator==(const cSC4TGI& o) const noexcept {
        return type == o.type && group == o.group && instance == o.instance;
    }
};

struct cSC4TGIHash {
    std::size_t operator()(const cSC4TGI& k) const noexcept {
        std::size_t h1 = std::hash<Uint32>{}(k.type);
        std::size_t h2 = std::hash<Uint32>{}(k.group);
        std::size_t h3 = std::hash<Uint32>{}(k.instance);
        return h1 ^ (h2 * 0x9e3779b9u) ^ (h3 * 0x85ebca6bu);
    }
};

// Base GZCOM interfaces matching SC4SimGrid.cpp.inl
class cIGZUnknown {
public:
    virtual ~cIGZUnknown() = default;
    virtual bool QueryInterface(GZREFIID riid, void** ppvObj) = 0;
    virtual Uint32 AddRef() = 0;
    virtual Uint32 Release() = 0;
};

class cIGZIStream;
class cIGZOStream;

class cIGZSerializable : public cIGZUnknown {
public:
    virtual bool Write(cIGZOStream& oStream) const = 0;
    virtual bool Read(cIGZIStream& iStream) = 0;
};

// Binary Input Stream (little-endian, matching Maxis cIGZIStream)
class cIGZIStream {
public:
    cIGZIStream(const Uint8* data, std::size_t size)
        : mData(data), mSize(size), mPos(0), mOk(true) {}
    explicit cIGZIStream(const std::vector<Uint8>& buf)
        : mData(buf.data()), mSize(buf.size()), mPos(0), mOk(true) {}

    bool Good() const noexcept { return mOk; }
    std::size_t Position() const noexcept { return mPos; }
    std::size_t Size() const noexcept { return mSize; }
    std::size_t Remaining() const noexcept { return mPos <= mSize ? (mSize - mPos) : 0; }
    const Uint8* CurrentPtr() const noexcept { return mData + mPos; }

    bool Seek(std::size_t pos) noexcept {
        if (pos > mSize) { mOk = false; return false; }
        mPos = pos;
        return true;
    }

    bool Skip(std::size_t n) noexcept {
        return Seek(mPos + n);
    }

    bool ReadBytes(void* dst, std::size_t n) noexcept {
        if (mPos + n > mSize) {
            mOk = false;
            std::memset(dst, 0, n);
            return false;
        }
        if (n > 0) {
            std::memcpy(dst, mData + mPos, n);
            mPos += n;
        }
        return true;
    }

    template <typename T>
    cIGZIStream& ReadPrimitive(T& v) noexcept {
        ReadBytes(&v, sizeof(T));
        return *this;
    }

    cIGZIStream& operator>>(bool& v) noexcept {
        Uint8 b = 0;
        ReadBytes(&b, 1);
        v = (b != 0);
        return *this;
    }
    cIGZIStream& operator>>(Uint8& v) noexcept   { return ReadPrimitive(v); }
    cIGZIStream& operator>>(Sint8& v) noexcept   { return ReadPrimitive(v); }
    cIGZIStream& operator>>(Uint16& v) noexcept  { return ReadPrimitive(v); }
    cIGZIStream& operator>>(Sint16& v) noexcept  { return ReadPrimitive(v); }
    cIGZIStream& operator>>(Uint32& v) noexcept  { return ReadPrimitive(v); }
    cIGZIStream& operator>>(Sint32& v) noexcept  { return ReadPrimitive(v); }
    cIGZIStream& operator>>(Uint64& v) noexcept  { return ReadPrimitive(v); }
    cIGZIStream& operator>>(Sint64& v) noexcept  { return ReadPrimitive(v); }
    cIGZIStream& operator>>(Float32& v) noexcept { return ReadPrimitive(v); }
    cIGZIStream& operator>>(Float64& v) noexcept { return ReadPrimitive(v); }

    bool ReadGZString(std::string& out) {
        Uint32 len = 0;
        if (!(*this >> len).Good() || len > Remaining()) {
            mOk = false;
            return false;
        }
        out.resize(len);
        if (len > 0) {
            ReadBytes(out.data(), len);
        }
        return mOk;
    }

private:
    const Uint8* mData;
    std::size_t mSize;
    std::size_t mPos;
    bool mOk;
};

// Binary Output Stream (little-endian, matching Maxis cIGZOStream)
class cIGZOStream {
public:
    cIGZOStream() = default;

    const std::vector<Uint8>& Buffer() const noexcept { return mBuf; }
    std::vector<Uint8>& Buffer() noexcept { return mBuf; }
    std::size_t Size() const noexcept { return mBuf.size(); }

    void WriteBytes(const void* src, std::size_t n) {
        if (n == 0) return;
        const auto* p = static_cast<const Uint8*>(src);
        mBuf.insert(mBuf.end(), p, p + n);
    }

    template <typename T>
    cIGZOStream& WritePrimitive(const T& v) {
        WriteBytes(&v, sizeof(T));
        return *this;
    }

    cIGZOStream& operator<<(bool v) {
        Uint8 b = v ? 1 : 0;
        WriteBytes(&b, 1);
        return *this;
    }
    cIGZOStream& operator<<(Uint8 v)   { return WritePrimitive(v); }
    cIGZOStream& operator<<(Sint8 v)   { return WritePrimitive(v); }
    cIGZOStream& operator<<(Uint16 v)  { return WritePrimitive(v); }
    cIGZOStream& operator<<(Sint16 v)  { return WritePrimitive(v); }
    cIGZOStream& operator<<(Uint32 v)  { return WritePrimitive(v); }
    cIGZOStream& operator<<(Sint32 v)  { return WritePrimitive(v); }
    cIGZOStream& operator<<(Uint64 v)  { return WritePrimitive(v); }
    cIGZOStream& operator<<(Sint64 v)  { return WritePrimitive(v); }
    cIGZOStream& operator<<(Float32 v) { return WritePrimitive(v); }
    cIGZOStream& operator<<(Float64 v) { return WritePrimitive(v); }

    void WriteGZString(const std::string& s) {
        Uint32 len = static_cast<Uint32>(s.size());
        *this << len;
        WriteBytes(s.data(), s.size());
    }

private:
    std::vector<Uint8> mBuf;
};

// Maxis CRC32 (poly 0x04C11DB7 MSB-first used in SC4 record headers)
Uint32 ComputeMaxisCRC32(const Uint8* data, std::size_t length) noexcept;

// cSC4COMSerializer::GetCRC @ 0x1003cfeb6 as used by SaveClassObjects
// @ 0x1003d10c4: the per-record CRC covers the record from offset 8
// (Uint32 memAddr followed by the serialized body), capped at 0x3d090
// (250000) bytes. Verified against every record in the shipped .sc4 saves.
constexpr std::size_t kSC4RecordCRCMaxBytes = 0x3d090u;
Uint32 ComputeSC4RecordCRC(Uint32 memAddr, const Uint8* body, std::size_t bodyLength) noexcept;

// Lookup human-readable Maxis class or DBPF Type name from a 32-bit CLSID/TypeID
const char* GetGZClassName(Uint32 clsid) noexcept;

} // namespace opensc4
