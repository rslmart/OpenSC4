#pragma once

#include "opensc4/gzcom.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace opensc4 {

// Maxis QFS / RefPack (0x10FB) decompression
// Handles both DBPF subfiles with a 4-byte compressed-size prefix + 0x10FB
// and raw 0x10FB streams. Returns raw input unchanged if not QFS-compressed.
bool QFSDecompress(const Uint8* src, std::size_t srcLen, std::vector<Uint8>& out, bool* wasCompressed = nullptr);

struct DBPFHeader {
    char magic[4]          = {'D', 'B', 'P', 'F'};
    Uint32 majorVersion    = 1;
    Uint32 minorVersion    = 0;
    Uint32 dateCreated     = 0;
    Uint32 dateModified    = 0;
    Uint32 indexMajorVer   = 7;
    Uint32 indexEntryCount = 0;
    Uint32 indexOffset     = 0;
    Uint32 indexSize       = 0;
    Uint32 holeEntryCount  = 0;
    Uint32 holeOffset      = 0;
    Uint32 holeSize        = 0;
};

struct DBPFIndexEntry {
    cSC4TGI tgi;
    Uint32 offset           = 0;
    Uint32 fileSize         = 0;
    Uint32 uncompressedSize = 0; // Populated from DIR (0xE86B1EEF) if compressed
    bool isCompressed       = false;
};

class cDBPFArchive {
public:
    cDBPFArchive() = default;

    bool Open(const std::string& path);
    bool OpenFromMemory(std::vector<Uint8> data, const std::string& virtualName = "memory.sc4");

    bool IsOpen() const noexcept { return mIsOpen; }
    const std::string& Path() const noexcept { return mPath; }
    const DBPFHeader& Header() const noexcept { return mHeader; }
    const std::vector<DBPFIndexEntry>& Entries() const noexcept { return mEntries; }

    // Find first matching entry by exact TGI
    const DBPFIndexEntry* FindEntry(const cSC4TGI& tgi) const noexcept;
    // Find all entries matching a given Type ID
    std::vector<const DBPFIndexEntry*> FindEntriesByType(Uint32 typeId) const;
    // Find first entry matching (Type, Instance) regardless of Group
    const DBPFIndexEntry* FindByTypeAndInstance(Uint32 typeId, Uint32 instanceId) const noexcept;

    // Read & automatically QFS-decompress an entry
    bool ReadEntry(const DBPFIndexEntry& entry, std::vector<Uint8>& outData) const;
    bool ReadByTGI(const cSC4TGI& tgi, std::vector<Uint8>& outData) const;
    bool ReadFirstByType(Uint32 typeId, std::vector<Uint8>& outData) const;

    // Count entries grouped by Type ID
    std::unordered_map<Uint32, Uint32> GetTypeHistogram() const;

private:
    bool ParseArchive();
    bool ReadRawSlice(Uint32 offset, Uint32 size, std::vector<Uint8>& out) const;

    std::string mPath;
    bool mIsOpen = false;
    DBPFHeader mHeader{};
    std::vector<DBPFIndexEntry> mEntries;
    std::unordered_map<cSC4TGI, std::size_t, cSC4TGIHash> mTgiLookup;
    std::unordered_map<Uint64, std::size_t> mTypeInstanceLookup;
    mutable std::vector<Uint8> mMemoryBuffer;
};

struct RegionDescriptor {
    std::string name;
    Uint32 width       = 16; // in small-city (64x64) tracts from config.bmp
    Uint32 height      = 16;
    Uint32 terrainType = 0;
    Uint32 waterMin    = 60;
    Uint32 waterMax    = 100;
};

struct RegionCityInfo {
    std::string regionName;
    std::string fileName;
    std::string fullPath;
    std::string cityName;
    std::string mayorName;
    Uint32 tileX          = 0;
    Uint32 tileZ          = 0;
    Uint32 sizeX          = 2; // 1=Small(64), 2=Medium(128), 4=Large(256)
    Uint32 sizeZ          = 2;
    Uint32 residentialPop = 0;
    Uint32 commercialPop  = 0;
    Uint32 industrialPop  = 0;
    Sint64 funds          = 0;
    bool hasPngThumbnail  = false;
};

// Multi-archive virtual filesystem mounting SimCity_1..5.dat, EP1.dat, Sound.dat,
// SimCityLocale.dat, and Regions/*/*.sc4 from the user's SimCity 4 Deluxe install
class cDBPFSystem {
public:
    static std::string DefaultSteamSC4AppPath();

    bool MountInstall(const std::string& appOrGameDataPath);
    bool IsMounted() const noexcept { return mMounted; }
    const std::string& GameDataPath() const noexcept { return mGameDataPath; }
    const std::string& MachOBinaryPath() const noexcept { return mMachOBinaryPath; }

    const std::vector<std::unique_ptr<cDBPFArchive>>& Archives() const noexcept { return mArchives; }
    const std::vector<RegionDescriptor>& Regions() const noexcept { return mRegions; }
    const std::vector<RegionCityInfo>& RegionCities() const noexcept { return mRegionCities; }

    std::size_t TotalIndexedEntries() const noexcept;
    bool LoadAssetByTGI(const cSC4TGI& tgi, std::vector<Uint8>& outData) const;
    bool LoadAssetByTypeAndInstance(Uint32 typeId, Uint32 instanceId, std::vector<Uint8>& outData) const;
    bool ReadCityThumbnailPNG(std::size_t cityIndex, std::vector<Uint8>& outPng) const;

private:
    bool mMounted = false;
    std::string mAppBundlePath;
    std::string mGameDataPath;
    std::string mMachOBinaryPath;
    std::vector<std::unique_ptr<cDBPFArchive>> mArchives;
    std::vector<RegionDescriptor> mRegions;
    std::vector<RegionCityInfo> mRegionCities;
};

} // namespace opensc4
