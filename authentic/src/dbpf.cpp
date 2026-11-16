#include "opensc4/dbpf.h"
#include <fstream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

namespace opensc4 {

bool QFSDecompress(const Uint8* src, std::size_t srcLen, std::vector<Uint8>& out, bool* wasCompressed) {
    if (wasCompressed) *wasCompressed = false;
    if (!src || srcLen == 0) {
        out.clear();
        return true;
    }

    std::size_t pos = 0;
    Uint32 uncompressedLen = 0;
    bool hasMagic = false;

    // DBPF QFS stream usually starts with [uint32_t compressedSize][0x10][0xFB][24-bit big-endian uncompressedSize]
    if (srcLen >= 9 && src[4] == 0x10 && src[5] == 0xFB) {
        hasMagic = true;
        uncompressedLen = (static_cast<Uint32>(src[6]) << 16) |
                          (static_cast<Uint32>(src[7]) << 8) |
                          static_cast<Uint32>(src[8]);
        pos = 9;
    } else if (srcLen >= 5 && src[0] == 0x10 && src[1] == 0xFB) {
        hasMagic = true;
        uncompressedLen = (static_cast<Uint32>(src[2]) << 16) |
                          (static_cast<Uint32>(src[3]) << 8) |
                          static_cast<Uint32>(src[4]);
        pos = 5;
    }

    if (!hasMagic) {
        out.assign(src, src + srcLen);
        return true;
    }

    if (wasCompressed) *wasCompressed = true;
    out.clear();
    out.reserve(uncompressedLen > 0 && uncompressedLen < (128u * 1024u * 1024u) ? uncompressedLen : srcLen * 3);

    while (pos < srcLen) {
        Uint8 b0 = src[pos++];
        if ((b0 & 0x80u) == 0) {
            // 2-byte command: 0cccpppp pppppppp
            if (pos >= srcLen) break;
            Uint8 b1 = src[pos++];
            Uint32 literalCount = b0 & 0x03u;
            for (Uint32 i = 0; i < literalCount && pos < srcLen; ++i) {
                out.push_back(src[pos++]);
            }
            Uint32 copyLen = ((b0 & 0x1Cu) >> 2) + 3u;
            Uint32 copyOffset = ((static_cast<Uint32>(b0 & 0x60u)) << 3) + b1 + 1u;
            if (copyOffset == 0 || copyOffset > out.size()) return false;
            for (Uint32 i = 0; i < copyLen; ++i) {
                out.push_back(out[out.size() - copyOffset]);
            }
        } else if ((b0 & 0x40u) == 0) {
            // 3-byte command: 10cccccc pppppppp pppppppp
            if (pos + 1 >= srcLen) break;
            Uint8 b1 = src[pos++];
            Uint8 b2 = src[pos++];
            Uint32 literalCount = (b1 >> 6) & 0x03u;
            for (Uint32 i = 0; i < literalCount && pos < srcLen; ++i) {
                out.push_back(src[pos++]);
            }
            Uint32 copyLen = (b0 & 0x3Fu) + 4u;
            Uint32 copyOffset = ((static_cast<Uint32>(b1 & 0x3Fu)) << 8) + b2 + 1u;
            if (copyOffset == 0 || copyOffset > out.size()) return false;
            for (Uint32 i = 0; i < copyLen; ++i) {
                out.push_back(out[out.size() - copyOffset]);
            }
        } else if ((b0 & 0x20u) == 0) {
            // 4-byte command: 110cccpp pppppppp pppppppp cccccccc
            if (pos + 2 >= srcLen) break;
            Uint8 b1 = src[pos++];
            Uint8 b2 = src[pos++];
            Uint8 b3 = src[pos++];
            Uint32 literalCount = b0 & 0x03u;
            for (Uint32 i = 0; i < literalCount && pos < srcLen; ++i) {
                out.push_back(src[pos++]);
            }
            Uint32 copyLen = ((static_cast<Uint32>(b0 & 0x0Cu)) << 6) + b3 + 5u;
            Uint32 copyOffset = ((static_cast<Uint32>(b0 & 0x10u)) << 12) +
                                (static_cast<Uint32>(b1) << 8) + b2 + 1u;
            if (copyOffset == 0 || copyOffset > out.size()) return false;
            for (Uint32 i = 0; i < copyLen; ++i) {
                out.push_back(out[out.size() - copyOffset]);
            }
        } else {
            // 1-byte literal block or stop code: 111ppppp
            Uint32 literalCount = ((b0 & 0x1Fu) + 1u) << 2;
            if (literalCount <= 0x70u) {
                for (Uint32 i = 0; i < literalCount && pos < srcLen; ++i) {
                    out.push_back(src[pos++]);
                }
            } else {
                // Stop command: 111111pp (0..3 trailing literal bytes)
                Uint32 tail = b0 & 0x03u;
                for (Uint32 i = 0; i < tail && pos < srcLen; ++i) {
                    out.push_back(src[pos++]);
                }
                break;
            }
        }
    }
    return true;
}

bool cDBPFArchive::Open(const std::string& path) {
    mPath = path;
    mMemoryBuffer.clear();
    mIsOpen = false;
    return ParseArchive();
}

bool cDBPFArchive::OpenFromMemory(std::vector<Uint8> data, const std::string& virtualName) {
    mPath = virtualName;
    mMemoryBuffer = std::move(data);
    mIsOpen = false;
    return ParseArchive();
}

bool cDBPFArchive::ReadRawSlice(Uint32 offset, Uint32 size, std::vector<Uint8>& out) const {
    if (!mMemoryBuffer.empty()) {
        if (static_cast<std::size_t>(offset) + size > mMemoryBuffer.size()) return false;
        out.assign(mMemoryBuffer.begin() + offset, mMemoryBuffer.begin() + offset + size);
        return true;
    }
    std::ifstream f(mPath, std::ios::binary);
    if (!f) return false;
    f.seekg(offset, std::ios::beg);
    if (!f) return false;
    out.resize(size);
    if (size > 0) {
        f.read(reinterpret_cast<char*>(out.data()), size);
        if (!f) return false;
    }
    return true;
}

bool cDBPFArchive::ParseArchive() {
    std::vector<Uint8> hdrBytes;
    if (!ReadRawSlice(0, 96, hdrBytes)) return false;

    if (std::memcmp(hdrBytes.data(), "DBPF", 4) != 0) return false;

    cIGZIStream hs(hdrBytes);
    hs.ReadBytes(mHeader.magic, 4);
    hs >> mHeader.majorVersion >> mHeader.minorVersion;
    hs.Skip(12); // reserved
    hs >> mHeader.dateCreated >> mHeader.dateModified;
    hs >> mHeader.indexMajorVer >> mHeader.indexEntryCount >> mHeader.indexOffset >> mHeader.indexSize;
    hs >> mHeader.holeEntryCount >> mHeader.holeOffset >> mHeader.holeSize;

    if (mHeader.indexEntryCount == 0 || mHeader.indexSize < mHeader.indexEntryCount * 20u) {
        return false;
    }

    std::vector<Uint8> idxBytes;
    if (!ReadRawSlice(mHeader.indexOffset, mHeader.indexSize, idxBytes)) return false;

    mEntries.clear();
    mEntries.reserve(mHeader.indexEntryCount);
    mTgiLookup.clear();
    mTgiLookup.reserve(mHeader.indexEntryCount);
    mTypeInstanceLookup.clear();
    mTypeInstanceLookup.reserve(mHeader.indexEntryCount);

    cIGZIStream is(idxBytes);
    const DBPFIndexEntry* dirEntry = nullptr;
    for (Uint32 i = 0; i < mHeader.indexEntryCount; ++i) {
        DBPFIndexEntry e{};
        is >> e.tgi.type >> e.tgi.group >> e.tgi.instance >> e.offset >> e.fileSize;
        e.uncompressedSize = e.fileSize;
        e.isCompressed = false;
        std::size_t idx = mEntries.size();
        mEntries.push_back(e);
        mTgiLookup[e.tgi] = idx;
        Uint64 tiKey = (static_cast<Uint64>(e.tgi.type) << 32) | e.tgi.instance;
        mTypeInstanceLookup[tiKey] = idx;
        if (e.tgi.type == kTypeDIR) {
            dirEntry = &mEntries.back();
        }
    }

    mIsOpen = true;

    // DIR key {0xe86b1eef, 0xe86b1eef, 0x286b1f03} is constructed @
    // 0x10036ab2d-43. ReadCompressedSetRecord @ 0x10036aae4 reads each
    // 16-byte record as type@+0, group@+4, instance@+8, uncompressedSize@+12.
    if (dirEntry && dirEntry->fileSize >= 16) {
        std::vector<Uint8> dirData;
        if (ReadEntry(*dirEntry, dirData)) {
            std::size_t count = dirData.size() / 16;
            cIGZIStream ds(dirData);
            for (std::size_t i = 0; i < count; ++i) {
                cSC4TGI tgi{};
                Uint32 uncompSz = 0;
                ds >> tgi.type >> tgi.group >> tgi.instance >> uncompSz;
                auto it = mTgiLookup.find(tgi);
                if (it != mTgiLookup.end()) {
                    mEntries[it->second].uncompressedSize = uncompSz;
                    mEntries[it->second].isCompressed = true;
                }
            }
        }
    }

    return true;
}

const DBPFIndexEntry* cDBPFArchive::FindEntry(const cSC4TGI& tgi) const noexcept {
    auto it = mTgiLookup.find(tgi);
    if (it == mTgiLookup.end()) return nullptr;
    return &mEntries[it->second];
}

std::vector<const DBPFIndexEntry*> cDBPFArchive::FindEntriesByType(Uint32 typeId) const {
    std::vector<const DBPFIndexEntry*> result;
    for (const auto& e : mEntries) {
        if (e.tgi.type == typeId) result.push_back(&e);
    }
    return result;
}

const DBPFIndexEntry* cDBPFArchive::FindByTypeAndInstance(Uint32 typeId, Uint32 instanceId) const noexcept {
    Uint64 tiKey = (static_cast<Uint64>(typeId) << 32) | instanceId;
    auto it = mTypeInstanceLookup.find(tiKey);
    if (it != mTypeInstanceLookup.end()) return &mEntries[it->second];
    return nullptr;
}

bool cDBPFArchive::ReadRawEntry(const DBPFIndexEntry& entry, std::vector<Uint8>& outData) const {
    if (!mIsOpen) return false;
    return ReadRawSlice(entry.offset, entry.fileSize, outData);
}

bool cDBPFArchive::ReadRawFile(std::vector<Uint8>& outData) const {
    if (!mIsOpen) return false;
    if (!mMemoryBuffer.empty()) {
        outData = mMemoryBuffer;
        return true;
    }

    std::ifstream f(mPath, std::ios::binary | std::ios::ate);
    if (!f) return false;
    const std::streamoff length = f.tellg();
    if (length < 0) return false;
    outData.resize(static_cast<std::size_t>(length));
    f.seekg(0, std::ios::beg);
    if (!outData.empty()) {
        f.read(reinterpret_cast<char*>(outData.data()), length);
        if (!f) return false;
    }
    return true;
}

bool cDBPFArchive::ReadEntry(const DBPFIndexEntry& entry, std::vector<Uint8>& outData) const {
    std::vector<Uint8> raw;
    if (!ReadRawEntry(entry, raw)) return false;
    return QFSDecompress(raw.data(), raw.size(), outData);
}

bool cDBPFArchive::ReadByTGI(const cSC4TGI& tgi, std::vector<Uint8>& outData) const {
    const auto* e = FindEntry(tgi);
    if (!e) return false;
    return ReadEntry(*e, outData);
}

bool cDBPFArchive::ReadFirstByType(Uint32 typeId, std::vector<Uint8>& outData) const {
    for (const auto& e : mEntries) {
        if (e.tgi.type == typeId) return ReadEntry(e, outData);
    }
    return false;
}

std::unordered_map<Uint32, Uint32> cDBPFArchive::GetTypeHistogram() const {
    std::unordered_map<Uint32, Uint32> hist;
    for (const auto& e : mEntries) {
        hist[e.tgi.type]++;
    }
    return hist;
}

std::string cDBPFSystem::DefaultSteamSC4AppPath() {
    return "/Users/russellmartin/Library/Application Support/Steam/steamapps/common/SimCity 4 Deluxe/Sim City 4 Deluxe Edition.app";
}

bool cDBPFSystem::MountInstall(const std::string& appOrGameDataPath) {
    mArchives.clear();
    mRegions.clear();
    mRegionCities.clear();
    mMounted = false;

    fs::path p(appOrGameDataPath);
    if (!fs::exists(p)) return false;

    fs::path gameData = p;
    if (fs::exists(p / "Contents" / "GameData")) {
        mAppBundlePath = p.string();
        gameData = p / "Contents" / "GameData";
        fs::path binSub = p / "Contents" / "MacOS" / "Sim City 4 Deluxe Editionsub";
        if (fs::exists(binSub)) mMachOBinaryPath = binSub.string();
    } else if (fs::exists(p / "SimCity_1.dat")) {
        gameData = p;
        mAppBundlePath = p.parent_path().parent_path().string();
        fs::path binSub = p.parent_path() / "MacOS" / "Sim City 4 Deluxe Editionsub";
        if (fs::exists(binSub)) mMachOBinaryPath = binSub.string();
    } else {
        return false;
    }

    mGameDataPath = gameData.string();

    // Standard SC4 Deluxe load order: SimCity_1..5.dat, EP1.dat, Intro.dat, Sound.dat, English/SimCityLocale.dat
    const std::vector<std::string> archiveNames = {
        "SimCity_1.dat",
        "SimCity_2.dat",
        "SimCity_3.dat",
        "SimCity_4.dat",
        "SimCity_5.dat",
        "EP1.dat",
        "Intro.dat",
        "Sound.dat",
        "English/SimCityLocale.dat"
    };

    for (const auto& rel : archiveNames) {
        fs::path datPath = gameData / rel;
        if (fs::exists(datPath)) {
            auto arc = std::make_unique<cDBPFArchive>();
            if (arc->Open(datPath.string())) {
                mArchives.push_back(std::move(arc));
            }
        }
    }

    // Scan shipped Regions/*/*.sc4 and parse cSC4ReadRegionalCity (0xca027edb) headers
    fs::path regionsDir = gameData / "Regions";
    if (fs::exists(regionsDir) && fs::is_directory(regionsDir)) {
        std::vector<fs::path> regionFolders;
        for (const auto& entry : fs::directory_iterator(regionsDir)) {
            if (entry.is_directory()) regionFolders.push_back(entry.path());
        }
        std::sort(regionFolders.begin(), regionFolders.end());

        for (const auto& regFolder : regionFolders) {
            std::string regName = regFolder.filename().string();
            RegionDescriptor regDesc{};
            regDesc.name = regName;

            // Parse config.bmp dimensions (18..26 in BMP header)
            fs::path bmpPath = regFolder / "config.bmp";
            if (fs::exists(bmpPath)) {
                std::ifstream bf(bmpPath, std::ios::binary);
                Uint8 bhdr[32] = {0};
                if (bf.read(reinterpret_cast<char*>(bhdr), 26) && bhdr[0] == 'B' && bhdr[1] == 'M') {
                    Sint32 bw = 0, bh = 0;
                    std::memcpy(&bw, bhdr + 18, 4);
                    std::memcpy(&bh, bhdr + 22, 4);
                    if (bw > 0 && bw <= 64 && std::abs(bh) > 0 && std::abs(bh) <= 64) {
                        regDesc.width = static_cast<Uint32>(bw);
                        regDesc.height = static_cast<Uint32>(std::abs(bh));
                    }
                }
            }
            mRegions.push_back(regDesc);

            std::vector<fs::path> sc4Files;
            for (const auto& f : fs::directory_iterator(regFolder)) {
                if (f.is_regular_file() && f.path().extension() == ".sc4") {
                    sc4Files.push_back(f.path());
                }
            }
            std::sort(sc4Files.begin(), sc4Files.end());

            for (const auto& sc4Path : sc4Files) {
                cDBPFArchive cityArc;
                if (!cityArc.Open(sc4Path.string())) continue;

                RegionCityInfo info{};
                info.regionName = regName;
                info.fileName = sc4Path.filename().string();
                info.fullPath = sc4Path.string();
                info.cityName = sc4Path.stem().string();
                info.hasPngThumbnail = !cityArc.FindEntriesByType(kTypePNG).empty();

                std::vector<Uint8> rcData;
                if (cityArc.ReadFirstByType(kCLSID_cSC4ReadRegionalCity, rcData) && rcData.size() >= 32) {
                    cIGZIStream rs(rcData);
                    Uint16 maj = 0, minv = 0;
                    rs >> maj >> minv;
                    rs >> info.tileX >> info.tileZ >> info.sizeX >> info.sizeZ;
                    rs >> info.residentialPop >> info.commercialPop >> info.industrialPop;
                    // Extract city & mayor strings if present at offset 0x40
                    if (rcData.size() > 0x44) {
                        for (std::size_t off = 0x38; off + 8 < std::min<std::size_t>(rcData.size(), 256); ++off) {
                            Uint32 slen = 0;
                            std::memcpy(&slen, rcData.data() + off, 4);
                            if (slen >= 3 && slen <= 48 && off + 4 + slen <= rcData.size()) {
                                bool printable = true;
                                for (Uint32 k = 0; k < slen; ++k) {
                                    Uint8 ch = rcData[off + 4 + k];
                                    if (ch < 32 || ch >= 127) { printable = false; break; }
                                }
                                if (printable) {
                                    info.cityName.assign(reinterpret_cast<const char*>(rcData.data() + off + 4), slen);
                                    // Look for mayor name right after
                                    for (std::size_t moff = off + 4 + slen; moff + 8 < std::min<std::size_t>(rcData.size(), off + 128); ++moff) {
                                        Uint32 mlen = 0;
                                        std::memcpy(&mlen, rcData.data() + moff, 4);
                                        if (mlen >= 2 && mlen <= 48 && moff + 4 + mlen <= rcData.size()) {
                                            bool mprint = true;
                                            for (Uint32 k = 0; k < mlen; ++k) {
                                                Uint8 ch = rcData[moff + 4 + k];
                                                if (ch < 32 || ch >= 127) { mprint = false; break; }
                                            }
                                            if (mprint) {
                                                std::string s(reinterpret_cast<const char*>(rcData.data() + moff + 4), mlen);
                                                if (s != "  Defacto") {
                                                    info.mayorName = s;
                                                    break;
                                                }
                                            }
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }

                std::vector<Uint8> budgetData;
                if (cityArc.ReadFirstByType(kCLSID_cSC4BudgetSimulator, budgetData) && budgetData.size() >= 22) {
                    std::memcpy(&info.funds, budgetData.data() + 14, sizeof(Sint64));
                }

                mRegionCities.push_back(std::move(info));
            }
        }
    }

    mMounted = !mArchives.empty();
    return mMounted;
}

std::size_t cDBPFSystem::TotalIndexedEntries() const noexcept {
    std::size_t total = 0;
    for (const auto& a : mArchives) total += a->Entries().size();
    return total;
}

bool cDBPFSystem::LoadAssetByTGI(const cSC4TGI& tgi, std::vector<Uint8>& outData) const {
    // Search in reverse load order so EP1.dat overrides SimCity_1.dat (authentic SC4 behavior)
    for (auto it = mArchives.rbegin(); it != mArchives.rend(); ++it) {
        if ((*it)->ReadByTGI(tgi, outData)) return true;
    }
    return false;
}

bool cDBPFSystem::LoadAssetByTypeAndInstance(Uint32 typeId, Uint32 instanceId, std::vector<Uint8>& outData) const {
    for (auto it = mArchives.rbegin(); it != mArchives.rend(); ++it) {
        const auto* e = (*it)->FindByTypeAndInstance(typeId, instanceId);
        if (e && (*it)->ReadEntry(*e, outData)) return true;
    }
    return false;
}

bool cDBPFSystem::ReadCityThumbnailPNG(std::size_t cityIndex, std::vector<Uint8>& outPng) const {
    if (cityIndex >= mRegionCities.size()) return false;
    cDBPFArchive cityArc;
    if (!cityArc.Open(mRegionCities[cityIndex].fullPath)) return false;
    return cityArc.ReadFirstByType(kTypePNG, outPng);
}

} // namespace opensc4
