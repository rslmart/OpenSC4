#include "opensc4/building_lot_position.h"
#include "opensc4/city.h"
#include "opensc4/sc4_subfiles.h"
#include "opensc4/sc4_subfiles_automata.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <unordered_map>

namespace opensc4 {

namespace {

constexpr Uint32 kJdn2000Jan01 = 2451545u;
constexpr cSC4TGI kBuildingDevelopmentExemplar{
    kTypeExemplar, 0xe7e2c2dbu, 0xe8da7677u
};
constexpr Uint32 kPropConstructionResourceRange = 0x47e2c380u;
constexpr Uint32 kPropDemandToResourceMultiplier = 0x47e2c381u;
constexpr Uint32 kPropIdlePlanningSteps = 0x47e2c382u;
constexpr Uint32 kPropDeclineThresholds = 0x47e2c383u;

// Decodes each [size][crc][memAddr]+body record of a save subfile with the
// exact stream reader; undecodable records are skipped.
template <typename RecordT, typename Fn>
void ForEachSC4Record(const std::vector<Uint8>& bytes, Fn&& fn) {
    std::size_t pos = 0;
    while (pos + 12u <= bytes.size()) {
        Uint32 recSize = 0;
        Uint32 memAddr = 0;
        std::memcpy(&recSize, bytes.data() + pos, 4);
        std::memcpy(&memAddr, bytes.data() + pos + 8, 4);
        if (recSize < 12u || pos + recSize > bytes.size()) {
            return;
        }
        cIGZIStream s(bytes.data() + pos + 12, recSize - 12u);
        RecordT rec{};
        rec.memAddr = memAddr;
        if (rec.Read(s) && s.Good()) {
            fn(rec);
        }
        pos += recSize;
    }
}

// Resource Key Types 0, 1, 4 and 5 (0x27812820..0x27812825) all embed the
// model as a {type, group, instance} triple; RKT4/RKT5 list one per state.
// cSC4FloraOccupant::SetStaticFloraData / prop resource-key lookup uses these.
struct S3DResourceKey {
    Uint32 group = 0;
    Uint32 instance = 0;
};
S3DResourceKey FindS3DKeyInExemplar(const cGZExemplar& exemplar) noexcept {
    for (Uint32 key : {kPropResourceKeyType1, kPropResourceKeyType0,
                       kPropResourceKeyType0 + 4u, kPropResourceKeyType0 + 5u}) {
        const auto it = exemplar.properties.find(key);
        if (it == exemplar.properties.end()) continue;
        const auto& values = it->second.intValues;
        for (std::size_t i = 0; i + 2 < values.size(); ++i) {
            if (static_cast<Uint32>(values[i]) == kTypeS3D && values[i + 2] != 0) {
                return S3DResourceKey{static_cast<Uint32>(values[i + 1]),
                                      static_cast<Uint32>(values[i + 2])};
            }
        }
    }
    return {};
}
Uint32 FindS3DInstanceInExemplar(const cGZExemplar& exemplar) noexcept {
    return FindS3DKeyInExemplar(exemplar).instance;
}

// cSC4NetworkOccupant::GetEdgeTypesFlag @ 0x10019eb50: the store holds one
// primary eNetworkType plus secondary {type, edgeFlags} entries.
Uint32 NetworkTypeMaskFromEdgeStore(const EdgeConnectionStoreSaveRecord& store) noexcept {
    constexpr Uint8 kMaxType = static_cast<Uint8>(NetworkType::DirtRoad);
    Uint32 mask = 0;
    if (store.primaryNetworkType <= kMaxType) {
        mask |= 1u << store.primaryNetworkType;
    }
    for (const auto& e : store.entries) {
        if (e.networkType <= kMaxType) {
            mask |= 1u << e.networkType;
        }
    }
    return mask;
}

Uint32 JulianDayFromCivil(Uint32 year, Uint32 month, Uint32 day) noexcept {
    const Sint64 a = (14 - static_cast<Sint64>(month)) / 12;
    const Sint64 y = static_cast<Sint64>(year) + 4800 - a;
    const Sint64 m = static_cast<Sint64>(month) + 12 * a - 3;
    return static_cast<Uint32>(static_cast<Sint64>(day) + (153 * m + 2) / 5 +
                               365 * y + y / 4 - y / 100 + y / 400 - 32045);
}

bool CivilFromJulianDay(Uint32 julianDay, Uint32& year, Uint32& month, Uint32& day) noexcept {
    if (julianDay < 1721426u) return false; // 0001-01-01 Gregorian
    const Sint64 a = static_cast<Sint64>(julianDay) + 32044;
    const Sint64 b = (4 * a + 3) / 146097;
    const Sint64 c = a - (146097 * b) / 4;
    const Sint64 d = (4 * c + 3) / 1461;
    const Sint64 e = c - (1461 * d) / 4;
    const Sint64 m = (5 * e + 2) / 153;
    day = static_cast<Uint32>(e - (153 * m + 2) / 5 + 1);
    month = static_cast<Uint32>(m + 3 - 12 * (m / 10));
    year = static_cast<Uint32>(100 * b + d - 4800 + m / 10);
    return year >= 1 && month >= 1 && month <= 12 && day >= 1 && day <= 31;
}

Uint32 DayOfYear(Uint32 year, Uint32 month, Uint32 day) noexcept {
    return JulianDayFromCivil(year, month, day) - JulianDayFromCivil(year, 1, 1) + 1;
}

struct ZoneDevelopmentProfile {
    const char* name;
    Float32 heightMeters;
};

ZoneDevelopmentProfile GetZoneDevelopmentProfile(ZoneType zone) noexcept {
    switch (zone) {
        case ZoneType::ResidentialLow:  return {"R$ Cottage", 8.0f};
        case ZoneType::ResidentialMed:  return {"R$$ Apartments", 24.0f};
        case ZoneType::ResidentialHigh: return {"R$$$ Condominiums", 62.0f};
        case ZoneType::CommercialLow:   return {"Cs$ Retail Shop", 10.0f};
        case ZoneType::CommercialMed:   return {"Co$$ Office Plaza", 32.0f};
        case ZoneType::CommercialHigh:  return {"Co$$$ Skyscraper", 84.0f};
        case ZoneType::IndustrialAg:    return {"I-R Farm Silo", 11.0f};
        case ZoneType::IndustrialMed:   return {"I-M Factory", 18.0f};
        case ZoneType::IndustrialHigh:  return {"I-HT Tech Lab", 26.0f};
        default:                        return {"RCI Building", 12.0f};
    }
}

} // namespace

bool SC4DemandRecord::EndCycleAndCalculate() noexcept {
    const bool changed = newSupply != 0.0f || newDemand != 0.0f;
    supplyValue += newSupply;
    demandValue += newDemand;
    newSupply = 0.0f;
    newDemand = 0.0f;

    const Float32 rawDemand = std::clamp(demandValue - supplyValue,
                                         activeDemandMin, activeDemandMax);
    const Float32 modifier = economyModifier + taxModifier;
    const Float32 modifiedDemand =
        rawDemand + std::min(demandValue * modifier, activeDemandMax * modifier);
    const Float32 demandAfterModifiers = std::clamp(modifiedDemand,
                                                    activeDemandMin, activeDemandMax);
    const Float32 capMaximum = activeDemandMax - demandCap * activeDemandMax;
    activeDemand = std::max(activeDemandMin,
                            std::min(demandAfterModifiers, capMaximum));
    return changed;
}

Sint32 BuildingDevelopmentState::CalculateMonthlyResources(
    Float32 residentialDemand,
    Float32 commercialDemand,
    Float32 industrialDemand) const noexcept {
    const Float32 aggregateDemand =
        residentialDemand + commercialDemand + industrialDemand;
    const Sint32 candidate =
        static_cast<Sint32>(aggregateDemand * demandToResourceMultiplier);
    return std::clamp(candidate,
                      minimumConstructionResources,
                      maximumConstructionResources);
}

cSC4City::cSC4City() {
    GenerateProceduralCity(128, 128, 0x53433400u);
}

float cSC4City::GetVertexHeight(int vx, int vz) const noexcept {
    int nx = mCellCountX + 1;
    int nz = mCellCountZ + 1;
    vx = std::clamp(vx, 0, mCellCountX);
    vz = std::clamp(vz, 0, mCellCountZ);
    std::size_t idx = static_cast<std::size_t>(vz) * nx + vx;
    if (idx < mHeightmap.size()) return mHeightmap[idx];
    (void)nz;
    return mSeaLevel;
}

void cSC4City::SetVertexHeight(int vx, int vz, float h) noexcept {
    if (vx < 0 || vx > mCellCountX || vz < 0 || vz > mCellCountZ) return;
    int nx = mCellCountX + 1;
    std::size_t idx = static_cast<std::size_t>(vz) * nx + vx;
    if (idx < mHeightmap.size()) mHeightmap[idx] = h;
}

float cSC4City::GetCellCenterHeight(int cx, int cz) const noexcept {
    float h00 = GetVertexHeight(cx, cz);
    float h10 = GetVertexHeight(cx + 1, cz);
    float h01 = GetVertexHeight(cx, cz + 1);
    float h11 = GetVertexHeight(cx + 1, cz + 1);
    return 0.25f * (h00 + h10 + h01 + h11);
}

bool cSC4City::IsCellUnderwater(int cx, int cz) const noexcept {
    return GetCellCenterHeight(cx, cz) < mSeaLevel - 0.5f;
}

ZoneType cSC4City::GetCellZone(int cx, int cz) const noexcept {
    if (cx < 0 || cx >= mCellCountX || cz < 0 || cz >= mCellCountZ) return ZoneType::None;
    std::size_t idx = static_cast<std::size_t>(cz) * mCellCountX + cx;
    return idx < mCellZones.size() ? mCellZones[idx] : ZoneType::None;
}

bool cSC4City::HasCellNetwork(int cx, int cz, NetworkType* outType) const noexcept {
    if (cx < 0 || cx >= mCellCountX || cz < 0 || cz >= mCellCountZ) return false;
    std::size_t idx = static_cast<std::size_t>(cz) * mCellCountX + cx;
    if (idx >= mCellNetworkIdx.size() || mCellNetworkIdx[idx] < 0) return false;
    Sint16 netIdx = mCellNetworkIdx[idx];
    if (static_cast<std::size_t>(netIdx) < mNetworks.size()) {
        if (outType) *outType = mNetworks[netIdx].networkType;
        return true;
    }
    return false;
}

static Uint32 CountSizePrefixedRecords(const std::vector<Uint8>& buf) noexcept {
    Uint32 count = 0;
    std::size_t pos = 0;
    while (pos + 8 <= buf.size()) {
        Uint32 recSz = 0;
        std::memcpy(&recSz, buf.data() + pos, 4);
        if (recSz < 12 || pos + recSz > buf.size()) break;
        ++count;
        pos += recSz;
    }
    return count;
}

bool cSC4City::HasCivicSaveRecord(Uint32 clsid) const noexcept {
    const auto it = mCivicSaveRecordPresent.find(clsid);
    return it != mCivicSaveRecordPresent.end() && it->second;
}

// Integration-only identity binding over canonical saved records. This is not
// a reconstructed game method; the canonical readers provide the provenance.
const BuildingOccupantSaveRecord* cSC4City::FindSavedBuildingOccupant(
    const SimSerializableRef& ref) const noexcept {
    if (!mSavedBuildingOccupantsAvailable || ref.memAddr == 0 ||
        ref.clsid != kCLSID_cSC4BuildingOccupant) return nullptr;
    const BuildingOccupantSaveRecord* match = nullptr;
    for (const auto& record : mSavedBuildingOccupants) {
        if (record.memAddr != ref.memAddr) continue;
        if (match) return nullptr; // Do not choose between duplicate identities.
        match = &record;
    }
    return match;
}

// Saved-snapshot integration policy: availability, unique identities and grid
// shape validation are adapter domain checks, not original runtime guards.
SavedBuildingLotResult cSC4City::QuerySavedBuildingLot(
    const SimSerializableRef& ref) const noexcept {
    SavedBuildingLotResult result;
    if (!mSavedBuildingOccupantsAvailable || !mSavedLotsAvailable ||
        !mSavedZoneDevelopersAvailable) return result;
    result.status = SavedBuildingLotStatus::Unsupported;
    const auto* building = FindSavedBuildingOccupant(ref);
    if (!building || mSavedZoneDevelopers.size() != 1) return result;
    const auto& rows = mSavedZoneDevelopers.front().rows;
    if (rows.size() != static_cast<std::size_t>(mCellCountX)) return result;
    for (const auto& row : rows)
        if (row.size() != static_cast<std::size_t>(mCellCountZ)) return result;
    const auto bits = [](const S3DVector3& v) {
        return lot_position::PositionBits{std::bit_cast<Uint32>(v.x),
            std::bit_cast<Uint32>(v.y), std::bit_cast<Uint32>(v.z)};
    };
    // GetPosition @ 0x10019b430; GetOccupantLot arithmetic @ 0x1001310b4.
    // Use raw bounds floats, never the unrelated serialized base.cellBounds.
    const auto cell = lot_position::OccupantCell(lot_position::BuildingPosition(
        bits(building->minBounds), bits(building->maxBounds)));
    result.cellX = cell.x;
    result.cellZ = cell.z;
    result.status = SavedBuildingLotStatus::NoLot;
    // GetLot @ 0x100131192, unrestricted-cell path (bool=false).
    if (cell.x < 0 || cell.z < 0 || cell.x >= mCellCountX || cell.z >= mCellCountZ)
        return result;
    result.lotRef = rows[cell.x][cell.z];
    if (result.lotRef.memAddr == 0) return result;
    result.status = SavedBuildingLotStatus::Unsupported;
    if (result.lotRef.clsid != kCLSID_cSC4Lot) return result;
    const LotFullSaveRecord* lot = nullptr;
    for (const auto& candidate : mSavedLots) {
        if (candidate.memAddr != result.lotRef.memAddr) continue;
        if (lot) return result;
        lot = &candidate;
    }
    if (!lot) return result;
    result.status = SavedBuildingLotStatus::Lot;
    result.savedFlagsCD = lot->flagsCD;
    // IsOccupantPowered @ 0x10021485c; GetHasPower @ 0x100183b50;
    // GetNeedsPower @ 0x100183b3e. Saved flags only, no post-load refresh.
    result.savedPowered = (lot->flagsCD & 0x10u) != 0 || (lot->flagsCD & 0x08u) == 0;
    return result;
}

bool cSC4City::LoadFromSC4File(const std::string& sc4Path, const cDBPFSystem* dbpfSys) {
    // Adapter-owned observations are scoped to the previous city lifecycle.
    mLoadedAgingState.reset();
    mSavedBuildingOccupants.clear();
    mSavedBuildingOccupantsAvailable = false;
    mSavedLots.clear();
    mSavedZoneDevelopers.clear();
    mSavedLotsAvailable = false;
    mSavedZoneDevelopersAvailable = false;
    cDBPFArchive arc;
    if (!arc.Open(sc4Path)) return false;
    mSourcePath = sc4Path;
    return LoadFromSC4Archive(arc, dbpfSys);
}

bool cSC4City::LoadFromSC4Archive(const cDBPFArchive& arc, const cDBPFSystem* dbpfSys) {
    // Adapter-owned observations are scoped to the previous city lifecycle.
    mLoadedAgingState.reset();
    mSavedBuildingOccupants.clear();
    mSavedBuildingOccupantsAvailable = false;
    mSavedLots.clear();
    mSavedZoneDevelopers.clear();
    mSavedLotsAvailable = false;
    mSavedZoneDevelopersAvailable = false;
    if (!arc.IsOpen()) return false;

    mOriginalSaveBytes.clear();
    mDirtySaveEntries.clear();
    mBudgetSaveTGI = {};
    if (!arc.ReadRawFile(mOriginalSaveBytes)) return false;

    mSourcePath = arc.Path();
    mIsFromOriginalSave = true;
    mLots.clear();
    mBuildings.clear();
    mNetworks.clear();
    mProps.clear();
    mFlora.clear();
    mLotTextures.clear();
    mAutomata.clear();
    mPngThumbnail.clear();
    mPipeCount = 0;
    mSubwayCount = 0;
    mPowerLineCount = 0;
    mLoadedDemandValues.clear();
    mLoadedDemandRecords.clear();
    mYear = 2000;
    mMonth = 1;
    mDay = 1;
    mJulianDay = kJdn2000Jan01;
    mTotalDays = 0;
    mCadence = {};
    mBuildingDevelopment = {};
    mProceduralDeclinePlans.clear();
    mTractDeveloperTuning = {};
    mTractDeveloperState = {};
    mBuildingDevSimRecord = {};
    mGrowthDeveloperRecords.clear();
    mGrowthDeveloperTunings.clear();
    mGrowthDeveloperGrowthLists.clear();
    mGrowthDeveloperDeclineLists.clear();
    mDemandSimRecord = {};
    mDemandSimTuning = {};
    mLotDeveloperTuning = {};
    mLotDeveloperRecord = {};
    mLotConfigManagerRecord = {};

    if (dbpfSys) {
        std::vector<Uint8> exemplarBytes;
        if (dbpfSys->LoadAssetByTGI(kBuildingDevelopmentExemplar, exemplarBytes)) {
            cGZExemplar exemplar;
            if (exemplar.Parse(exemplarBytes.data(), exemplarBytes.size())) {
                mBuildingDevelopment.loadedTunablesFromExemplar = true;
                mTractDeveloperTuning.LoadFromExemplar(exemplar);
                mLotDeveloperTuning.LoadFromExemplar(exemplar);
                auto range = exemplar.properties.find(kPropConstructionResourceRange);
                if (range != exemplar.properties.end() && range->second.intValues.size() >= 2) {
                    mBuildingDevelopment.minimumConstructionResources =
                        static_cast<Sint32>(range->second.intValues[0]);
                    mBuildingDevelopment.maximumConstructionResources =
                        static_cast<Sint32>(range->second.intValues[1]);
                }
                mBuildingDevelopment.demandToResourceMultiplier =
                    exemplar.GetFloat(kPropDemandToResourceMultiplier, 0.5f);
                mBuildingDevelopment.idlePlanningSteps =
                    static_cast<Uint32>(exemplar.GetInt(kPropIdlePlanningSteps, 10));
                auto thresholds = exemplar.properties.find(kPropDeclineThresholds);
                if (thresholds != exemplar.properties.end()) {
                    mBuildingDevelopment.declineThresholds = thresholds->second.floatValues;
                }
            }
        }
        std::vector<Uint8> demandSimExBytes;
        if (dbpfSys->LoadAssetByTGI(kDemandSimulatorTuningExemplarTGI, demandSimExBytes)) {
            cGZExemplar demandSimEx;
            if (demandSimEx.Parse(demandSimExBytes.data(), demandSimExBytes.size())) {
                mDemandSimTuning.LoadFromExemplar(demandSimEx);
            }
        }
        for (const Uint32 subType : kGrowthDeveloperSubTypes) {
            std::vector<Uint8> devExBytes;
            cGZExemplar devEx;
            GrowthDeveloperTuning devTuning{};
            if (dbpfSys->LoadAssetByTGI(
                    {kTypeExemplar, kExemplarGroup_GrowthDeveloper, subType},
                    devExBytes) &&
                devEx.Parse(devExBytes.data(), devExBytes.size()) &&
                devTuning.LoadFromExemplar(devEx)) {
                mGrowthDeveloperTunings[subType] = std::move(devTuning);
            }
        }
    }

    std::vector<Uint8> tractDevBytes;
    if (arc.ReadFirstByType(kCLSID_cSC4TractDeveloper, tractDevBytes)) {
        mTractDeveloperState.ReadFromSC4Record(tractDevBytes);
    }
    mTractDeveloperState.ApplyTuningDefaults(mTractDeveloperTuning);

    std::vector<Uint8> bldgDevSimBytes;
    if (arc.ReadFirstByType(kCLSID_cSC4BuildingDevSim, bldgDevSimBytes)) {
        mBuildingDevSimRecord.ReadFromSC4Record(bldgDevSimBytes);
    }

    std::vector<Uint8> growthDevBytes;
    if (arc.ReadFirstByType(kCLSID_cSC4GrowthDeveloper, growthDevBytes)) {
        ParseGrowthDeveloperSubfile(growthDevBytes, mGrowthDeveloperRecords);
    }

    std::vector<Uint8> demandSimBytes;
    if (arc.ReadFirstByType(kCLSID_cSC4DemandSimulator, demandSimBytes)) {
        mDemandSimRecord.ReadFromSC4Record(demandSimBytes);
    }

    std::vector<Uint8> lotDevBytes;
    if (arc.ReadFirstByType(kCLSID_cSC4LotDeveloper, lotDevBytes)) {
        mLotDeveloperRecord.ReadFromSC4Record(lotDevBytes);
    }

    std::vector<Uint8> lotConfigMgrBytes;
    if (arc.ReadFirstByType(kCLSID_cSC4LotConfigurationManager, lotConfigMgrBytes)) {
        mLotConfigManagerRecord.ReadFromSC4Record(lotConfigMgrBytes);
    }
    // 1. Parse RegionalCity (0xca027edb) for dimensions, population, tile coords, and city/mayor names
    Uint32 sizeTractsX = 2; // default Medium (128x128)
    Uint32 sizeTractsZ = 2;
    std::vector<Uint8> rcData;
    if (arc.ReadFirstByType(kCLSID_cSC4ReadRegionalCity, rcData) && rcData.size() >= 32) {
        cIGZIStream rs(rcData);
        Uint16 maj = 0, minv = 0;
        rs >> maj >> minv;
        rs >> mRegionTileX >> mRegionTileZ >> sizeTractsX >> sizeTractsZ;
        rs >> mResidentialPop >> mCommercialPop >> mIndustrialPop;

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
                    mCityName.assign(reinterpret_cast<const char*>(rcData.data() + off + 4), slen);
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
                                    mMayorName = s;
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

    // 2. Parse Terrain Heightmap (0xa9dd6ff4, Instance 0x00000001)
    // Header is 6 bytes (uint16_t version = 2, plus 4 bytes or directly (N+1)*(N+1) float32s)
    const auto* hmEntry = arc.FindByTypeAndInstance(kCLSID_cSTETerrainHeightmap, 1);
    std::vector<Uint8> hmData;
    if (hmEntry && arc.ReadEntry(*hmEntry, hmData) && hmData.size() >= 65 * 65 * 4) {
        std::size_t floatBytes = hmData.size() - 2; // after uint16 version = 2
        std::size_t numVerts = floatBytes / 4;
        int side = static_cast<int>(std::round(std::sqrt(static_cast<double>(numVerts))));
        if (side == 65 || side == 129 || side == 257) {
            mCellCountX = side - 1;
            mCellCountZ = side - 1;
            mHeightmap.resize(static_cast<std::size_t>(side) * side);
            std::size_t byteOffset = hmData.size() - (static_cast<std::size_t>(side) * side * 4);
            std::memcpy(mHeightmap.data(), hmData.data() + byteOffset, mHeightmap.size() * sizeof(float));
        }
    }

    if (mHeightmap.empty()) {
        mCellCountX = static_cast<int>(std::clamp(sizeTractsX, 1u, 4u) * 64u);
        mCellCountZ = static_cast<int>(std::clamp(sizeTractsZ, 1u, 4u) * 64u);
        mHeightmap.assign(static_cast<std::size_t>(mCellCountX + 1) * (mCellCountZ + 1), 270.0f);
    }

    // Estimate sea level (standard Maxis sea level is 250.0m)
    mSeaLevel = 250.0f;

    // 3. Parse cSC4BudgetSimulator (0xe990be01) save record for Treasury funds
    std::vector<Uint8> budgetData;
    mHasBudgetSaveRecord =
        arc.ReadFirstByType(kCLSID_cSC4BudgetSimulator, budgetData) &&
        mBudgetSaveRecord.ReadFromSC4Record(budgetData);
    if (mHasBudgetSaveRecord) {
        mFunds = mBudgetSaveRecord.totalFunds;
        for (const auto& entry : arc.Entries()) {
            if (entry.tgi.type == kCLSID_cSC4BudgetSimulator) {
                mBudgetSaveTGI = entry.tgi;
                break;
            }
        }
    }
    {
        std::vector<Uint8> budgetTuningBytes;
        cGZExemplar budgetTuningExemplar;
        if (dbpfSys != nullptr &&
            dbpfSys->LoadAssetByTGI(kBudgetSimulatorTuningExemplarTGI, budgetTuningBytes) &&
            budgetTuningExemplar.Parse(budgetTuningBytes.data(), budgetTuningBytes.size())) {
            mBudgetTuning.LoadFromExemplar(budgetTuningExemplar);
        }
    }

    // 3c. Parse the civic & utility simulator save records. Every Read is
    // the exact binary format (byte-exact roundtrip verified against this
    // save by opensc4_verify); failures simply mark the record absent.
    {
        auto loadCivicRecord = [&arc, this](Uint32 clsid, auto& record) {
            std::vector<Uint8> data;
            const bool ok = arc.ReadFirstByType(clsid, data) && record.ReadFromSC4Record(data);
            mCivicSaveRecordPresent[clsid] = ok;
        };
        loadCivicRecord(kCLSID_cSC4LandValueSimulator, mLandValueSaveRecord);
        loadCivicRecord(kCLSID_cSC4CommercialSimulator, mCommercialSaveRecord);
        loadCivicRecord(kCLSID_cSC4IndustrialSimulator, mIndustrialSaveRecord);
        loadCivicRecord(kCLSID_cSC4FireProtectionSim, mFireProtectionSaveRecord);
        loadCivicRecord(kCLSID_cSC4PoliceSimulator, mPoliceSaveRecord);
        loadCivicRecord(kCLSID_cSC4FlammabilitySim, mFlammabilitySaveRecord);
        loadCivicRecord(kCLSID_cSC4CrimeSimulator, mCrimeSaveRecord);
        loadCivicRecord(kCLSID_cSC4AuraSimulator, mAuraSaveRecord);
        loadCivicRecord(kCLSID_cSC4PowerSimulator, mPowerSaveRecord);
        loadCivicRecord(kCLSID_cSC4PlumbingSimulator, mPlumbingSaveRecord);
        loadCivicRecord(kCLSID_cSC4PollutionSimulator, mPollutionSaveRecord);
        loadCivicRecord(kCLSID_cSC4ResidentialSimulator, mResidentialSaveRecord);
        loadCivicRecord(kCLSID_cSC4TrafficSimulator, mTrafficSaveRecord);
        loadCivicRecord(kCLSID_cSC4CivicBuildingSim, mCivicBuildingSaveRecord);
        loadCivicRecord(kCLSID_cSC4FloraSimulator, mFloraSaveRecord);
        loadCivicRecord(kCLSID_cSC4MySimAgentSimulator, mMySimAgentSaveRecord);
        loadCivicRecord(kCLSID_cSC4NeighborsSimulator, mNeighborsSaveRecord);
        loadCivicRecord(kCLSID_cSC4OrdinanceSimulator, mOrdinanceSaveRecord);
        loadCivicRecord(kCLSID_cSC4WeatherSimulator, mWeatherSaveRecord);
        loadCivicRecord(kCLSID_cSC4Simulator, mSimulatorSaveRecord);
        if (dbpfSys != nullptr) {
            auto loadCivicTuning = [dbpfSys](const cSC4TGI& tgi, auto& tuning) {
                std::vector<Uint8> tuningBytes;
                cGZExemplar tuningExemplar;
                if (dbpfSys->LoadAssetByTGI(tgi, tuningBytes) &&
                    tuningExemplar.Parse(tuningBytes.data(), tuningBytes.size())) {
                    tuning.LoadFromExemplar(tuningExemplar);
                }
            };
            loadCivicTuning(LandValueTuning::kExemplarTGI, mLandValueTuning);
            loadCivicTuning(FireProtectionTuning::kExemplarTGI, mFireProtectionTuning);
            loadCivicTuning(PoliceTuning::kExemplarTGI, mPoliceTuning);
            loadCivicTuning(FlammabilityTuning::kExemplarTGI, mFlammabilityTuning);
            loadCivicTuning(CrimeTuning::kExemplarTGI, mCrimeTuning);
            loadCivicTuning(AuraTuning::kExemplarTGI, mAuraTuning);
            loadCivicTuning(PowerTuning::kExemplarTGI, mPowerTuning);
            loadCivicTuning(PlumbingTuning::kExemplarTGI, mPlumbingTuning);
            loadCivicTuning(PollutionTuning::kExemplarTGI, mPollutionTuning);
            loadCivicTuning(ResidentialTuning::kExemplarTGI, mResidentialTuning);
            loadCivicTuning(TrafficTuning::kExemplarTGI, mTrafficTuning);
            loadCivicTuning(FloraTuning::kExemplarTGI, mFloraTuning);
            loadCivicTuning(WeatherTuning::kExemplarTGI, mWeatherTuning);
            loadCivicTuning(SimulatorTuning::kExemplarTGI, mSimulatorTuning);
        }
    }

    // 4. Sync city calendar from the decoded cSC4Simulator (0x2990c1e5) record.
    if (HasCivicSaveRecord(kCLSID_cSC4Simulator) && mSimulatorSaveRecord.version <= 4u) {
        Uint32 savedYear = 0, savedMonth = 0, savedDay = 0;
        if (CivilFromJulianDay(mSimulatorSaveRecord.julianDayNumber,
                               savedYear, savedMonth, savedDay) &&
            savedYear >= 1900 && savedYear <= 9999) {
            mJulianDay = mSimulatorSaveRecord.julianDayNumber;
            mYear = savedYear;
            mMonth = savedMonth;
            mDay = savedDay;
        }
    }

    // 5. Parse the separately persisted cSC4Demand objects. Each 67-byte
    // record contains the standard 12-byte SC4 persist header followed by the
    // exact version-2 field order from cSC4Demand::Read.
    std::vector<Uint8> demandBytes;
    if (arc.ReadFirstByType(kGZCLSID_cSC4DemandSerializable, demandBytes)) {
        std::size_t pos = 0;
        while (pos + 67 <= demandBytes.size()) {
            Uint32 recordSize = 0;
            Uint16 version = 0;
            std::memcpy(&recordSize, demandBytes.data() + pos, sizeof(recordSize));
            if (recordSize < 67 || pos + recordSize > demandBytes.size()) break;
            std::memcpy(&version, demandBytes.data() + pos + 12, sizeof(version));
            if (version <= 2) {
                SC4DemandRecord record{};
                std::memcpy(&record.memAddr, demandBytes.data() + pos + 8, 4);
                std::memcpy(&record.id, demandBytes.data() + pos + 14, 4);
                std::memcpy(&record.supplyValue, demandBytes.data() + pos + 18, 4);
                std::memcpy(&record.demandValue, demandBytes.data() + pos + 22, 4);
                std::memcpy(&record.newSupply, demandBytes.data() + pos + 26, 4);
                std::memcpy(&record.newDemand, demandBytes.data() + pos + 30, 4);
                std::memcpy(&record.serializedValue2C, demandBytes.data() + pos + 34, 4);
                std::memcpy(&record.serializedValue30, demandBytes.data() + pos + 38, 4);
                std::memcpy(&record.activeDemandMin, demandBytes.data() + pos + 42, 4);
                std::memcpy(&record.activeDemandMax, demandBytes.data() + pos + 46, 4);
                std::memcpy(&record.activeDemand, demandBytes.data() + pos + 50, 4);
                record.enabled = demandBytes[pos + 54] != 0;
                std::memcpy(&record.economyModifier, demandBytes.data() + pos + 55, 4);
                std::memcpy(&record.taxModifier, demandBytes.data() + pos + 59, 4);
                std::memcpy(&record.demandCap, demandBytes.data() + pos + 63, 4);

                if (mBuildingDevSimRecord.energyDemand.memAddr != 0u &&
                    record.memAddr == mBuildingDevSimRecord.energyDemand.memAddr &&
                    std::isfinite(record.supplyValue) &&
                    std::isfinite(record.demandValue)) {
                    mBuildingDevelopment.constructionResources =
                        std::max(0.0f, record.supplyValue - record.demandValue);
                }

                if (std::isfinite(record.activeDemand)) {
                    mLoadedDemandValues[record.id] = record.activeDemand;
                    mLoadedDemandRecords[record.id] = record;
                }
            }
            pos += recordSize;
        }

        auto demandInt = [this](Uint32 id) {
            return static_cast<Sint32>(std::lround(LoadedDemandValue(id)));
        };
        if (!mLoadedDemandValues.empty()) {
            // Exact saved Maxis demand classes; the three-column compatibility
            // view averages categories SC4 models separately.
            mDemand.resLow = demandInt(0x21010u);
            mDemand.resMed = demandInt(0x21020u);
            mDemand.resHigh = demandInt(0x21030u);
            mDemand.comLow = demandInt(0x23110u);
            mDemand.comMed = (demandInt(0x23120u) + demandInt(0x23320u)) / 2;
            mDemand.comHigh = (demandInt(0x23130u) + demandInt(0x23330u)) / 2;
            mDemand.indAg = demandInt(0x24100u);
            mDemand.indMed = (demandInt(0x24200u) + demandInt(0x24300u)) / 2;
            mDemand.indHigh = demandInt(0x24400u);
        }
    }

    // 6. Parse PNG Snapshot (0x8a2482b9)
    arc.ReadFirstByType(kTypePNG, mPngThumbnail);

    // Integration policy: publish canonical snapshots only after complete decode.
    // Zone Read @ 0x100132ffc preserves serialized rows; no runtime grid rebuild.
    std::vector<Uint8> zoneBytes;
    if (arc.ReadFirstByType(kCLSID_cSC4ZoneDeveloper, zoneBytes)) {
        std::vector<ZoneDeveloperSaveRecord> decoded;
        if (DecodeMultiRecordSubfile<ZoneDeveloperSaveRecord>(zoneBytes, decoded)) {
            mSavedZoneDevelopers.swap(decoded);
            mSavedZoneDevelopersAvailable = true;
        }
    }

    // 7. Parse cSC4Lot records (0xc9bd5d4a)
    std::vector<Uint8> lotBytes;
    if (arc.ReadFirstByType(kCLSID_cSC4Lot, lotBytes)) {
        std::vector<LotFullSaveRecord> decoded;
        if (DecodeMultiRecordSubfile<LotFullSaveRecord>(lotBytes, decoded)) {
            mSavedLots.swap(decoded);
            mSavedLotsAvailable = true;
        }
        ForEachSC4Record<LotFullSaveRecord>(lotBytes, [&](const LotFullSaveRecord& rec) {
            SC4LotRecord lot{};
            lot.memAddr = rec.memAddr;
            lot.configInstanceId = rec.configId;
            lot.buildingExemplarId = rec.buildingType64;
            lot.flags = rec.flagsCD;
            lot.minX = std::min(rec.minX, rec.maxX);
            lot.minZ = std::min(rec.minZ, rec.maxZ);
            lot.maxX = std::max(rec.minX, rec.maxX);
            lot.maxZ = std::max(rec.minZ, rec.maxZ);
            if (lot.minX >= mCellCountX || lot.minZ >= mCellCountZ) {
                return;
            }
            lot.maxX = static_cast<Uint8>(std::min<int>(lot.maxX, mCellCountX - 1));
            lot.maxZ = static_cast<Uint8>(std::min<int>(lot.maxZ, mCellCountZ - 1));
            lot.sizeX = static_cast<Uint8>(lot.maxX - lot.minX + 1);
            lot.sizeZ = static_cast<Uint8>(lot.maxZ - lot.minZ + 1);
            lot.altitudeY = std::isfinite(rec.altitude44)
                                ? rec.altitude44
                                : GetCellCenterHeight(lot.minX, lot.minZ);
            lot.slope1 = rec.slope4C;
            lot.slope2 = rec.slope48;
            lot.facing = rec.facingD0 & 0x03u;
            lot.zoneType = rec.zoneTypeD1 <= 15 ? static_cast<ZoneType>(rec.zoneTypeD1)
                                                : ZoneType::PloppedCivic;
            lot.wealth = std::clamp<Uint8>(rec.wealthD2 ? rec.wealthD2 : 1, 1, 3);
            mLots.push_back(lot);
        });
    }

    // 6. Parse cSC4BuildingOccupant records (0xa9bd882d, 82 bytes base + optional name string)
    struct CachedExemplarInfo {
        std::string name;
        Uint32 baseS3DInstance = 0;
        Uint32 s3dGroupId      = 0;
        Uint32 fshTextureId    = 0;
    };
    std::unordered_map<Uint32, CachedExemplarInfo> exemplarCache;

    std::vector<Uint8> bldBytes;
    if (arc.ReadFirstByType(kCLSID_cSC4BuildingOccupant, bldBytes)) {
        // Retain canonical records separately from the existing display projection.
        // Source provenance: Building Read @ 0x10019b350, base Read @ 0x1000bc86c,
        // exemplar holder Read @ 0x10004b34a. No new serialization semantics.
        std::vector<BuildingOccupantSaveRecord> decoded;
        if (DecodeMultiRecordSubfile<BuildingOccupantSaveRecord>(bldBytes, decoded)) {
            mSavedBuildingOccupants.swap(decoded);
            mSavedBuildingOccupantsAvailable = true;
        }

        std::size_t pos = 0;
        while (pos + 78 <= bldBytes.size()) {
            Uint32 recSz = 0;
            std::memcpy(&recSz, bldBytes.data() + pos, 4);
            if (recSz < 78 || pos + recSz > bldBytes.size()) break;

            const Uint8* r = bldBytes.data() + pos;
            SC4BuildingRecord bld{};
            std::memcpy(&bld.memAddr, r + 8, 4);

            // Standard 82-byte record has TGI at +36 and bbox at +53; named records shift by (recSz - 82)
            std::size_t shift = (recSz > 82 && recSz <= 260) ? (recSz - 82) : 0;
            std::memcpy(&bld.exemplarTGI.group, r + 36 + shift, 4);
            std::memcpy(&bld.exemplarTGI.type, r + 40 + shift, 4);
            std::memcpy(&bld.exemplarTGI.instance, r + 44 + shift, 4);
            std::memcpy(&bld.minX, r + 53 + shift, 4);
            std::memcpy(&bld.minY, r + 57 + shift, 4);
            std::memcpy(&bld.minZ, r + 61 + shift, 4);
            std::memcpy(&bld.maxX, r + 65 + shift, 4);
            std::memcpy(&bld.maxY, r + 69 + shift, 4);
            std::memcpy(&bld.maxZ, r + 73 + shift, 4);
            bld.orientation = r[77 + shift] & 0x03u;

            if (!std::isfinite(bld.minX) || !std::isfinite(bld.maxX) ||
                !std::isfinite(bld.minY) || !std::isfinite(bld.maxY) ||
                !std::isfinite(bld.minZ) || !std::isfinite(bld.maxZ)) {
                bld.minX = 0.0f; bld.maxX = 16.0f;
                bld.minY = mSeaLevel; bld.maxY = mSeaLevel + 10.0f;
                bld.minZ = 0.0f; bld.maxZ = 16.0f;
            }
            if (bld.minX > bld.maxX) std::swap(bld.minX, bld.maxX);
            if (bld.minY > bld.maxY) std::swap(bld.minY, bld.maxY);
            if (bld.minZ > bld.maxZ) std::swap(bld.minZ, bld.maxZ);
            bld.minX = std::clamp(bld.minX, 0.0f, mCellCountX * 16.0f);
            bld.maxX = std::clamp(std::max(bld.maxX, bld.minX + 4.0f), 0.0f, mCellCountX * 16.0f);
            bld.minZ = std::clamp(bld.minZ, 0.0f, mCellCountZ * 16.0f);
            bld.maxZ = std::clamp(std::max(bld.maxZ, bld.minZ + 4.0f), 0.0f, mCellCountZ * 16.0f);
            bld.minY = std::clamp(bld.minY, 150.0f, 900.0f);
            bld.maxY = std::clamp(std::max(bld.maxY, bld.minY + 4.0f), 154.0f, 950.0f);

            // Resolve Exemplar -> S3D -> FSH from mounted GameData .dat archives (O(1) cached)
            if (dbpfSys && dbpfSys->IsMounted() && bld.exemplarTGI.instance != 0) {
                auto it = exemplarCache.find(bld.exemplarTGI.instance);
                if (it == exemplarCache.end()) {
                    CachedExemplarInfo info{};
                    std::vector<Uint8> exBytes;
                    if (dbpfSys->LoadAssetByTGI(bld.exemplarTGI, exBytes) ||
                        dbpfSys->LoadAssetByTypeAndInstance(kTypeExemplar, bld.exemplarTGI.instance, exBytes)) {
                        cGZExemplar ex;
                        if (ex.Parse(exBytes.data(), exBytes.size())) {
                            info.name = ex.name;
                            // Prefer an embedded S3D resource key (RKT0/1/4/5), not
                            // blindly intValues[2] which may be a non-model TGI.
                            const S3DResourceKey key = FindS3DKeyInExemplar(ex);
                            info.baseS3DInstance = key.instance;
                            info.s3dGroupId = key.group;
                            if (info.baseS3DInstance == 0) {
                                auto rkt1 = ex.properties.find(kPropResourceKeyType1);
                                auto rkt0 = ex.properties.find(kPropResourceKeyType0);
                                if (rkt1 != ex.properties.end() && rkt1->second.intValues.size() >= 3) {
                                    info.baseS3DInstance = static_cast<Uint32>(rkt1->second.intValues[2]);
                                    if (rkt1->second.intValues.size() >= 2)
                                        info.s3dGroupId = static_cast<Uint32>(rkt1->second.intValues[1]);
                                } else if (rkt0 != ex.properties.end() && rkt0->second.intValues.size() >= 3) {
                                    info.baseS3DInstance = static_cast<Uint32>(rkt0->second.intValues[2]);
                                    if (rkt0->second.intValues.size() >= 2)
                                        info.s3dGroupId = static_cast<Uint32>(rkt0->second.intValues[1]);
                                }
                            }
                        }
                    }
                    if (info.baseS3DInstance == 0) {
                        info.baseS3DInstance = bld.exemplarTGI.instance & 0xFFFFF000u;
                    }
                    // Look up Zoom 5 S3D (try rotations) to extract MATS FSH texture ID.
                    // Park stubs only exist at the exact base instance (no zoom/rot nibbles).
                    auto tryExtractFsh = [&](Uint32 s3dInst) {
                        if (info.fshTextureId != 0) return;
                        std::vector<Uint8> s3dBytes;
                        const bool loaded = (info.s3dGroupId != 0 &&
                            dbpfSys->LoadAssetByTGI(cSC4TGI{kTypeS3D, info.s3dGroupId, s3dInst}, s3dBytes)) ||
                            dbpfSys->LoadAssetByTypeAndInstance(kTypeS3D, s3dInst, s3dBytes);
                        if (!loaded) return;
                        cS3DModel s3d;
                        if (!s3d.Parse(s3dBytes.data(), s3dBytes.size())) return;
                        for (const auto& mat : s3d.materials) {
                            if (mat.textureId != 0) {
                                info.fshTextureId = mat.textureId;
                                break;
                            }
                        }
                    };
                    for (unsigned rot = 0; rot < 4u && info.fshTextureId == 0; ++rot) {
                        tryExtractFsh((info.baseS3DInstance & 0xFFFFF000u) | 0x0400u | (rot << 4));
                    }
                    if (info.fshTextureId == 0) {
                        tryExtractFsh(info.baseS3DInstance & 0xFFFFF000u);
                    }
                    it = exemplarCache.emplace(bld.exemplarTGI.instance, std::move(info)).first;
                }
                bld.name = it->second.name;
                bld.s3dInstanceId = (it->second.baseS3DInstance & 0xFFFFF000u) | 0x0400u | ((bld.orientation & 0x03u) << 4);
                bld.s3dGroupId = it->second.s3dGroupId;
                bld.fshTextureId = it->second.fshTextureId;
            }

            // Match building to containing lot for zone/wealth color hints
            int cx = static_cast<int>((bld.minX + bld.maxX) * 0.5f / 16.0f);
            int cz = static_cast<int>((bld.minZ + bld.maxZ) * 0.5f / 16.0f);
            for (const auto& l : mLots) {
                if (cx >= l.minX && cx <= l.maxX && cz >= l.minZ && cz <= l.maxZ) {
                    bld.zoneHint = l.zoneType;
                    bld.wealthHint = l.wealth;
                    break;
                }
            }

            mBuildings.push_back(std::move(bld));
            pos += recSz;
        }
    }

    // 7. Parse cSC4NetworkOccupant records (0xc9c05c6e, 231..256 bytes each)
    std::vector<Uint8> netBytes;
    if (arc.ReadFirstByType(kCLSID_cSC4NetworkOccupant, netBytes)) {
        ForEachSC4Record<NetworkOccupantSaveRecord>(netBytes, [&](const NetworkOccupantSaveRecord& rec) {
            SC4NetworkRecord net{};
            net.memAddr = rec.memAddr;
            net.centerX = rec.transform.translation.x;
            net.centerY = rec.transform.translation.y;
            net.centerZ = rec.transform.translation.z;
            for (int v = 0; v < 4; ++v) {
                const auto& cv = rec.cornerVerts[static_cast<std::size_t>(v)];
                net.verts[v].x = cv.x;
                net.verts[v].y = cv.y;
                net.verts[v].z = cv.z;
                net.verts[v].u = cv.u;
                net.verts[v].v = cv.v;
                net.verts[v].rgba = static_cast<Uint32>(cv.r) | (static_cast<Uint32>(cv.g) << 8) |
                                    (static_cast<Uint32>(cv.b) << 16) | (static_cast<Uint32>(cv.a) << 24);
            }
            net.textureId = rec.pieceId134;
            net.orientation = rec.orientFlip13D & 0x03u;

            net.networkTypeMask = NetworkTypeMaskFromEdgeStore(rec.edgeStore);
            if (net.networkTypeMask == 0) {
                net.networkTypeMask = 1u;
            }
            for (Uint8 nt = 0; nt <= static_cast<Uint8>(NetworkType::DirtRoad); ++nt) {
                if (net.networkTypeMask & (1u << nt)) {
                    net.networkType = static_cast<NetworkType>(nt);
                    break;
                }
            }

            net.tileX = static_cast<Uint8>(std::clamp<int>(
                static_cast<int>(std::floor(net.centerX / 16.0f)), 0, mCellCountX - 1));
            net.tileZ = static_cast<Uint8>(std::clamp<int>(
                static_cast<int>(std::floor(net.centerZ / 16.0f)), 0, mCellCountZ - 1));

            bool validVerts = true;
            for (int v = 0; v < 4; ++v) {
                if (!std::isfinite(net.verts[v].x) || !std::isfinite(net.verts[v].y) || !std::isfinite(net.verts[v].z) ||
                    net.verts[v].x < -64.0f || net.verts[v].x > mCellCountX * 16.0f + 64.0f ||
                    net.verts[v].z < -64.0f || net.verts[v].z > mCellCountZ * 16.0f + 64.0f ||
                    net.verts[v].y < 50.0f  || net.verts[v].y > 1200.0f) {
                    validVerts = false;
                    break;
                }
            }
            if (validVerts) {
                net.tileX = static_cast<Uint8>(std::clamp<int>(static_cast<int>(0.25f * (net.verts[0].x + net.verts[1].x + net.verts[2].x + net.verts[3].x) / 16.0f), 0, mCellCountX - 1));
                net.tileZ = static_cast<Uint8>(std::clamp<int>(static_cast<int>(0.25f * (net.verts[0].z + net.verts[1].z + net.verts[2].z + net.verts[3].z) / 16.0f), 0, mCellCountZ - 1));
            } else {
                int tx = std::clamp<int>(net.tileX, 0, mCellCountX - 1);
                int tz = std::clamp<int>(net.tileZ, 0, mCellCountZ - 1);
                float wx0 = tx * 16.0f, wz0 = tz * 16.0f;
                float wx1 = (tx + 1) * 16.0f, wz1 = (tz + 1) * 16.0f;
                float y = GetCellCenterHeight(tx, tz) + 0.15f;
                net.verts[0] = {wx0, y, wz0, 0, 0, 0xFFD8D4D8u};
                net.verts[1] = {wx1, y, wz0, 1, 0, 0xFFD8D4D8u};
                net.verts[2] = {wx1, y, wz1, 1, 1, 0xFFD8D4D8u};
                net.verts[3] = {wx0, y, wz1, 0, 1, 0xFFD8D4D8u};
            }

            mNetworks.push_back(net);
        });
    }

    // Props and flora name their model through their exemplar's resource key
    // (RKT0/1/4/5). Zoom nibble 0x0400 matches building Zoom-5 S3D selection.
    std::unordered_map<Uint32, Uint32> modelByExemplar;
    auto resolveModel = [&](const cSC4TGI& exemplarTGI) -> Uint32 {
        if (dbpfSys == nullptr || !dbpfSys->IsMounted() || exemplarTGI.instance == 0) return 0;
        if (auto it = modelByExemplar.find(exemplarTGI.instance); it != modelByExemplar.end()) {
            return it->second;
        }
        Uint32 s3d = 0;
        std::vector<Uint8> exBytes;
        if (dbpfSys->LoadAssetByTGI(exemplarTGI, exBytes) ||
            dbpfSys->LoadAssetByTypeAndInstance(kTypeExemplar, exemplarTGI.instance, exBytes)) {
            cGZExemplar ex;
            if (ex.Parse(exBytes.data(), exBytes.size())) {
                s3d = FindS3DInstanceInExemplar(ex);
                if (s3d != 0) s3d = (s3d & 0xFFFFF000u) | 0x0400u;
            }
        }
        modelByExemplar.emplace(exemplarTGI.instance, s3d);
        return s3d;
    };

    // 8. Parse cSC4PropOccupant records (0x2977aa47, Read @ 0x10015e82e)
    std::vector<Uint8> propBytes;
    if (arc.ReadFirstByType(kCLSID_cSC4PropOccupant, propBytes)) {
        ForEachSC4Record<PropOccupantSaveRecord>(propBytes, [&](const PropOccupantSaveRecord& rec) {
            SC4PropRecord prop{};
            prop.exemplarTGI = rec.base.exemplarHolder.exemplarKey;
            prop.minX = std::min(rec.minBounds.x, rec.maxBounds.x);
            prop.minY = std::min(rec.minBounds.y, rec.maxBounds.y);
            prop.minZ = std::min(rec.minBounds.z, rec.maxBounds.z);
            prop.maxX = std::max(rec.minBounds.x, rec.maxBounds.x);
            prop.maxY = std::max(rec.minBounds.y, rec.maxBounds.y);
            prop.maxZ = std::max(rec.minBounds.z, rec.maxBounds.z);
            if (!std::isfinite(prop.minX) || !std::isfinite(prop.maxX) ||
                !std::isfinite(prop.minY) || !std::isfinite(prop.maxY) ||
                !std::isfinite(prop.minZ) || !std::isfinite(prop.maxZ)) {
                return;
            }
            prop.orientation = static_cast<Uint8>(rec.orientation & 0x03);
            prop.s3dInstanceId = resolveModel(prop.exemplarTGI);
            mProps.push_back(prop);
        });
    }

    // 9. Parse cSC4FloraOccupant records (0xa9c05c85, Read @ 0x1000cba0e)
    std::vector<Uint8> floraBytes;
    if (arc.ReadFirstByType(kCLSID_cSC4FloraOccupant, floraBytes)) {
        ForEachSC4Record<FloraOccupantSaveRecord>(floraBytes, [&](const FloraOccupantSaveRecord& rec) {
            SC4FloraRecord fl{};
            fl.exemplarTGI = rec.base.exemplarHolder.exemplarKey;
            fl.x = rec.position.x;
            fl.y = rec.position.y;
            fl.z = rec.position.z;
            if (!std::isfinite(fl.x) || !std::isfinite(fl.y) || !std::isfinite(fl.z) ||
                fl.x < 0.0f || fl.x > mCellCountX * 16.0f ||
                fl.z < 0.0f || fl.z > mCellCountZ * 16.0f) {
                return;
            }
            fl.orientation = static_cast<Uint8>(rec.orientation & 0x03);
            fl.s3dInstanceId = resolveModel(fl.exemplarTGI);
            mFlora.push_back(fl);
        });
    }

    // 9b. Parse cSC4LotBaseTextureOccupant records (0xc97f987c): spec bytes
    // 0/1 are the cell, byte 2 the quarter-turn of the texture.
    std::vector<Uint8> lotTexBytes;
    if (arc.ReadFirstByType(kCLSID_cSC4LotBaseTexture, lotTexBytes)) {
        ForEachSC4Record<LotBaseTextureOccupantSaveRecord>(lotTexBytes, [&](const LotBaseTextureOccupantSaveRecord& rec) {
            for (const auto& spec : rec.specs) {
                if (spec.textureId == 0 || spec.bytes[0] >= mCellCountX || spec.bytes[1] >= mCellCountZ) continue;
                SC4LotTextureRecord tex{};
                tex.textureId = spec.textureId;
                tex.cellX = spec.bytes[0];
                tex.cellZ = spec.bytes[1];
                tex.orientation = static_cast<Uint8>(spec.bytes[2] & 0x03u);
                mLotTextures.push_back(tex);
            }
        });
    }

    // 9c. Parse the cars and sims on the streets at save time
    // (cSC4Vehicle 0x896e714a / cSC4Pedestrian 0x896e75af).
    auto addAutomaton = [&](SC4AutomatonRecord::Kind kind, const AutomatonSaveRecord& a) {
        SC4AutomatonRecord rec{};
        rec.kind = kind;
        rec.x = a.vec74.x;
        rec.y = a.vec74.y;
        rec.z = a.vec74.z;
        const float len = std::sqrt(a.vec8C.x * a.vec8C.x + a.vec8C.z * a.vec8C.z);
        if (!std::isfinite(rec.x) || !std::isfinite(rec.y) || !std::isfinite(rec.z) || !(len > 1e-3f) ||
            rec.x < 0.0f || rec.x > mCellCountX * 16.0f || rec.z < 0.0f || rec.z > mCellCountZ * 16.0f) {
            return;
        }
        rec.headingX = a.vec8C.x / len;
        rec.headingZ = a.vec8C.z / len;
        rec.speed = std::isfinite(a.fieldB0) ? a.fieldB0 : 0.0f;
        rec.model = cSC4TGI{a.key1C8[0], a.key1C8[1], a.key1C8[2]};
        mAutomata.push_back(rec);
    };
    std::vector<Uint8> automatonBytes;
    if (arc.ReadFirstByType(kCLSID_cSC4AutomatonVehicle, automatonBytes)) {
        ForEachSC4Record<VehicleSaveRecord>(automatonBytes, [&](const VehicleSaveRecord& rec) {
            addAutomaton(SC4AutomatonRecord::Kind::Vehicle, rec.traffic.automaton);
        });
    }
    if (arc.ReadFirstByType(kCLSID_cSC4AutomatonPedestrian, automatonBytes)) {
        ForEachSC4Record<PedestrianSaveRecord>(automatonBytes, [&](const PedestrianSaveRecord& rec) {
            addAutomaton(SC4AutomatonRecord::Kind::Pedestrian, rec.traffic.automaton);
        });
    }

    // 10. Count Underground & Utility Occupants (Pipes, Subways, PowerLines)
    std::vector<Uint8> tmpBytes;
    if (arc.ReadFirstByType(kCLSID_cSC4PipeOccupant, tmpBytes)) mPipeCount = CountSizePrefixedRecords(tmpBytes);
    if (arc.ReadFirstByType(kCLSID_cSC4SubwayOccupant, tmpBytes)) mSubwayCount = CountSizePrefixedRecords(tmpBytes);
    if (arc.ReadFirstByType(kCLSID_cSC4PowerLineOccupant, tmpBytes)) mPowerLineCount = CountSizePrefixedRecords(tmpBytes);

    // 11. Parse all embedded cSC4SimGrid<T> subfiles (0x49b9e602..0x49b9e60a)
    std::vector<Uint8> sgBytes;
    if (arc.ReadFirstByType(kGZCLSID_cSC4SimGridSint8, sgBytes))   mLoadedSint8Grids   = ParseSimGridSubfile<Sint8>(sgBytes);
    if (arc.ReadFirstByType(kGZCLSID_cSC4SimGridUint8, sgBytes))   mLoadedUint8Grids   = ParseSimGridSubfile<Uint8>(sgBytes);
    if (arc.ReadFirstByType(kGZCLSID_cSC4SimGridSint16, sgBytes))  mLoadedSint16Grids  = ParseSimGridSubfile<Sint16>(sgBytes);
    if (arc.ReadFirstByType(kGZCLSID_cSC4SimGridUint16, sgBytes))  mLoadedUint16Grids  = ParseSimGridSubfile<Uint16>(sgBytes);
    if (arc.ReadFirstByType(kGZCLSID_cSC4SimGridFloat32, sgBytes)) mLoadedFloat32Grids = ParseSimGridSubfile<Float32>(sgBytes);

    RebuildSpatialLookups();
    UpdateSimulationGrids();
    RebuildOriginalSaveDeveloperLists();
    return true;
}

void cSC4City::GenerateProceduralCity(Uint32 cellCountX, Uint32 cellCountZ, Uint32 seed) {
    // Adapter-owned observations are scoped to the previous city lifecycle.
    mLoadedAgingState.reset();
    mSavedBuildingOccupants.clear();
    mSavedBuildingOccupantsAvailable = false;
    mSavedLots.clear();
    mSavedZoneDevelopers.clear();
    mSavedLotsAvailable = false;
    mSavedZoneDevelopersAvailable = false;
    mCityName = "OpenSC4 Procedural City";
    mMayorName = "Mayor";
    mSourcePath = "procedural://opengfx";
    mIsFromOriginalSave = false;
    mOriginalSaveBytes.clear();
    mDirtySaveEntries.clear();
    mBudgetSaveTGI = {};
    mCellCountX = static_cast<int>(cellCountX);
    mCellCountZ = static_cast<int>(cellCountZ);
    mSeaLevel = 250.0f;
    mFunds = 150000;
    mLoadedDemandValues.clear();
    mLoadedDemandRecords.clear();
    mYear = 2000;
    mMonth = 1;
    mDay = 1;
    mJulianDay = kJdn2000Jan01;
    mTotalDays = 0;
    mCadence = {};
    mBuildingDevelopment = {};
    mProceduralDeclinePlans.clear();
    mTractDeveloperTuning = {};
    mTractDeveloperState = {};
    mBuildingDevSimRecord = {};
    mGrowthDeveloperRecords.clear();
    mGrowthDeveloperTunings.clear();
    mGrowthDeveloperGrowthLists.clear();
    mGrowthDeveloperDeclineLists.clear();
    mDemandSimRecord = {};
    mDemandSimTuning = {};
    mLotDeveloperTuning = {};
    mLotDeveloperRecord = {};
    mLotConfigManagerRecord = {};
    mBuildingDevelopment.constructionResources =
        static_cast<Float32>(mBuildingDevelopment.minimumConstructionResources);

    mLots.clear();
    mBuildings.clear();
    mNetworks.clear();
    mProps.clear();
    mFlora.clear();
    mLotTextures.clear();
    mAutomata.clear();

    const int nx = mCellCountX + 1;
    const int nz = mCellCountZ + 1;
    mHeightmap.resize(static_cast<std::size_t>(nx) * nz);

    // Coastal river valley terrain with hills in the northeast
    for (int vz = 0; vz < nz; ++vz) {
        for (int vx = 0; vx < nx; ++vx) {
            float fx = static_cast<float>(vx) / mCellCountX;
            float fz = static_cast<float>(vz) / mCellCountZ;
            float riverDist = std::abs((fx - 0.22f) + 0.12f * std::sin(fz * 6.28318f));
            float h = 266.0f;
            if (riverDist < 0.08f) {
                h = 236.0f + (riverDist / 0.08f) * 18.0f;
            } else {
                h = 256.0f + 14.0f * (fx + fz) +
                    18.0f * std::sin(fx * 5.0f + 0.4f) * std::cos(fz * 4.5f);
            }
            SetVertexHeight(vx, vz, h);
        }
    }

    // Build starter road grid and R/C/I zones on the east bank
    int xStart = mCellCountX / 3;
    int xEnd = mCellCountX - 16;
    int zStart = 20;
    int zEnd = mCellCountZ - 20;

    for (int x = xStart; x <= xEnd; ++x) {
        for (int z = zStart; z <= zEnd; ++z) {
            if (IsCellUnderwater(x, z)) continue;
            bool onAvenue = (x == xStart + 16 || z == (zStart + zEnd) / 2);
            bool onRoad = ((x - xStart) % 8 == 0) || ((z - zStart) % 8 == 0);
            if (onAvenue || onRoad) {
                ApplyTool(onAvenue ? ToolType::BuildAvenue : ToolType::BuildRoad, x, z, x, z);
            }
        }
    }

    // Zone starter blocks; the first industrial block hosts the power plant
    // and the first residential block the water pump, each on its corner cell.
    bool placedPowerPlant = false;
    bool placedWaterPump = false;
    for (int bx = xStart + 1; bx + 6 < xEnd; bx += 8) {
        for (int bz = zStart + 1; bz + 6 < zEnd; bz += 8) {
            if (IsCellUnderwater(bx + 3, bz + 3)) continue;
            ToolType zTool = ToolType::ZoneResMed;
            if (bx < xStart + 16) zTool = ToolType::ZoneComMed;
            else if (bz > zEnd - 18) zTool = ToolType::ZoneIndMed;
            else if ((bx + bz) % 16 == 0) zTool = ToolType::ZoneResLow;

            ToolType utility = ToolType::Inspect;
            if (!placedPowerPlant && zTool == ToolType::ZoneIndMed) {
                utility = ToolType::PlopPowerPlant;
                placedPowerPlant = true;
            } else if (!placedWaterPump && zTool == ToolType::ZoneResMed) {
                utility = ToolType::PlopWaterPump;
                placedWaterPump = true;
            }
            if (utility == ToolType::Inspect) {
                ApplyTool(zTool, bx, bz, bx + 5, bz + 5);
            } else {
                ApplyTool(utility, bx, bz, bx, bz);
                ApplyTool(zTool, bx + 1, bz, bx + 5, bz + 5);
                ApplyTool(zTool, bx, bz + 1, bx, bz + 5);
            }
        }
    }

    // Scatter procedural trees on higher ground
    Uint32 rng = seed;
    for (int i = 0; i < 450; ++i) {
        rng = rng * 1664525u + 1013904223u;
        int tx = static_cast<int>(rng % mCellCountX);
        rng = rng * 1664525u + 1013904223u;
        int tz = static_cast<int>(rng % mCellCountZ);
        if (!IsCellUnderwater(tx, tz) && !HasCellNetwork(tx, tz) && GetCellZone(tx, tz) == ZoneType::None) {
            SC4FloraRecord fl{};
            fl.x = (tx + 0.5f) * 16.0f;
            fl.z = (tz + 0.5f) * 16.0f;
            fl.y = GetCellCenterHeight(tx, tz);
            mFlora.push_back(fl);
        }
    }

    StepSimulation(5);
    // A newly generated city starts on the canonical SC4 epoch rather than
    // inheriting the setup ticks used to seed its first developments.
    mYear = 2000;
    mMonth = 1;
    mDay = 1;
    mJulianDay = kJdn2000Jan01;
    mTotalDays = 0;
    mCadence = {};
    mBuildingDevelopment.dailyCycles = 0;
    mBuildingDevelopment.monthlyCycles = 0;
    mBuildingDevelopment.growthAttempts = 0;
    mBuildingDevelopment.growthExecutions = 0;
    mBuildingDevelopment.declinePlanCycles = 0;
    mBuildingDevelopment.constructionResources =
        static_cast<Float32>(mBuildingDevelopment.minimumConstructionResources);
}

void cSC4City::RebuildSpatialLookups() {
    std::size_t numCells = static_cast<std::size_t>(mCellCountX) * mCellCountZ;
    mCellZones.assign(numCells, ZoneType::None);
    mCellNetworkIdx.assign(numCells, -1);

    for (const auto& lot : mLots) {
        for (int z = lot.minZ; z <= lot.maxZ && z < mCellCountZ; ++z) {
            for (int x = lot.minX; x <= lot.maxX && x < mCellCountX; ++x) {
                mCellZones[static_cast<std::size_t>(z) * mCellCountX + x] = lot.zoneType;
            }
        }
    }

    for (std::size_t i = 0; i < mNetworks.size(); ++i) {
        const auto& net = mNetworks[i];
        int tx = net.tileX;
        int tz = net.tileZ;
        if (tx >= 0 && tx < mCellCountX && tz >= 0 && tz < mCellCountZ) {
            mCellNetworkIdx[static_cast<std::size_t>(tz) * mCellCountX + tx] = static_cast<Sint16>(i);
        }
    }
}

void cSC4City::UpdateSimulationGrids() {
    // Standard SC4 tract size is 4x4 city cells (64m x 64m per tract)
    const int tractSize = 4;
    mLandValueGrid.SetTractSize(tractSize, mCellCountX, mCellCountZ);
    mPollutionGrid.SetTractSize(tractSize, mCellCountX, mCellCountZ);
    mCrimeGrid.SetTractSize(tractSize, mCellCountX, mCellCountZ);
    mTrafficGrid.SetTractSize(tractSize, mCellCountX, mCellCountZ);
    mPowerGrid.SetTractSize(tractSize, mCellCountX, mCellCountZ);
    mWaterGrid.SetTractSize(tractSize, mCellCountX, mCellCountZ);
    mMayorRatingGrid.SetTractSize(tractSize, mCellCountX, mCellCountZ);

    // Locate authentic Mach-O SimGrids from the .sc4 save if present
    const cSC4SimGrid<Sint8>* savedLandVal = nullptr;
    const cSC4SimGrid<Sint8>* savedCrime = nullptr;
    const cSC4SimGrid<Sint8>* savedTraffic = nullptr;
    for (const auto& g : mLoadedSint8Grids) {
        switch (g.GetInstanceID()) {
            case kSimGridInst_LandValue:         savedLandVal = &g; break;
            case kSimGridInst_Crime:             savedCrime = &g; break;
            case kSimGridInst_TrafficCongestion: savedTraffic = &g; break;
            default: break;
        }
    }
    const cSC4SimGrid<Uint8>* savedAura = nullptr;
    for (const auto& g : mLoadedUint8Grids) {
        if (g.GetInstanceID() == kSimGridInst_AuraMayorRating) savedAura = &g;
    }
    const cSC4SimGrid<Uint16>* savedAirPoll = nullptr;
    for (const auto& g : mLoadedUint16Grids) {
        if (g.GetInstanceID() == kSimGridInst_AirPollution) savedAirPoll = &g;
    }

    int tCountX = mLandValueGrid.GetTractCountX();
    int tCountZ = mLandValueGrid.GetTractCountZ();

    for (int tx = 0; tx < tCountX; ++tx) {
        for (int tz = 0; tz < tCountZ; ++tz) {
            int cx = tx * tractSize + 2;
            int cz = tz * tractSize + 2;
            float elev = GetCellCenterHeight(cx, cz) - mSeaLevel;
            bool nearWater = (elev >= 0.0f && elev < 18.0f);

            int roadCount = 0;
            int indCount = 0;
            int resCount = 0;
            int comCount = 0;

            for (int dx = 0; dx < tractSize; ++dx) {
                for (int dz = 0; dz < tractSize; ++dz) {
                    int x = tx * tractSize + dx;
                    int z = tz * tractSize + dz;
                    if (HasCellNetwork(x, z)) ++roadCount;
                    ZoneType zt = GetCellZone(x, z);
                    if (zt >= ZoneType::ResidentialLow && zt <= ZoneType::ResidentialHigh) ++resCount;
                    else if (zt >= ZoneType::CommercialLow && zt <= ZoneType::CommercialHigh) ++comCount;
                    else if (zt >= ZoneType::IndustrialAg && zt <= ZoneType::IndustrialHigh) ++indCount;
                }
            }

            Uint8 poll = static_cast<Uint8>(std::clamp(indCount * 14 + roadCount * 3, 0, 255));
            if (savedAirPoll) {
                Uint16 rawP = savedAirPoll->GetAverageValueInCellRect(tx * tractSize, tz * tractSize, tractSize, tractSize);
                poll = std::max<Uint8>(poll, static_cast<Uint8>(std::clamp<int>(rawP / 4, 0, 255)));
            }

            Uint8 traffic = static_cast<Uint8>(std::clamp(roadCount * 16 + comCount * 10 + resCount * 5, 0, 255));
            if (savedTraffic) {
                int rawT = static_cast<Uint8>(savedTraffic->GetAverageValueInCellRect(tx * tractSize, tz * tractSize, tractSize, tractSize));
                traffic = std::max<Uint8>(traffic, static_cast<Uint8>(std::clamp(rawT, 0, 255)));
            }

            int lv = 85 + (nearWater ? 45 : 0) + static_cast<int>( std::clamp(elev * 0.6f, 0.0f, 50.0f) )
                     + comCount * 4 + resCount * 3 - poll / 2;
            Uint8 landVal = static_cast<Uint8>(std::clamp(lv, 10, 255));
            if (savedLandVal) {
                Uint8 rawLv = static_cast<Uint8>(savedLandVal->GetAverageValueInCellRect(tx * tractSize, tz * tractSize, tractSize, tractSize));
                if (rawLv > 0) landVal = static_cast<Uint8>((landVal + rawLv) / 2);
            }

            Uint8 crime = static_cast<Uint8>(std::clamp((resCount + comCount) * 4 + (255 - landVal) / 8, 0, 255));
            if (savedCrime) {
                Uint8 rawC = static_cast<Uint8>(savedCrime->GetAverageValueInCellRect(tx * tractSize, tz * tractSize, tractSize, tractSize));
                crime = std::max(crime, rawC);
            }


            Sint8 rating = static_cast<Sint8>(std::clamp((landVal / 3) - (poll / 4) - (crime / 4), -100, 100));
            if (savedAura) {
                Uint8 rawA = savedAura->GetAverageValueInCellRect(tx * tractSize, tz * tractSize, tractSize, tractSize);
                if (rawA > 0) rating = static_cast<Sint8>(std::clamp<int>(static_cast<int>(rawA) - 128, -100, 100));
            }

            mPollutionGrid.SetTractValue(tx, tz, poll);
            mTrafficGrid.SetTractValue(tx, tz, traffic);
            mLandValueGrid.SetTractValue(tx, tz, landVal);
            mCrimeGrid.SetTractValue(tx, tz, crime);
            mMayorRatingGrid.SetTractValue(tx, tz, rating);
        }
    }

    StepLivePowerAndWaterBFS();
    RecomputeCommuteRoutes(24);
}

void cSC4City::AutoTileNetworkAt(int x, int z) {
    if (x < 0 || x >= mCellCountX || z < 0 || z >= mCellCountZ) return;
    std::size_t cellIdx = static_cast<std::size_t>(z) * mCellCountX + x;
    if (cellIdx >= mCellNetworkIdx.size() || mCellNetworkIdx[cellIdx] < 0) return;

    auto& net = mNetworks[mCellNetworkIdx[cellIdx]];
    auto connects = [&](int nx, int nz) {
        if (nx < 0 || nx >= mCellCountX || nz < 0 || nz >= mCellCountZ) return false;
        std::size_t neighborCell = static_cast<std::size_t>(nz) * mCellCountX + nx;
        if (neighborCell >= mCellNetworkIdx.size() || mCellNetworkIdx[neighborCell] < 0) return false;
        const auto& neighbor = mNetworks[mCellNetworkIdx[neighborCell]];
        return (neighbor.networkTypeMask & net.networkTypeMask) != 0;
    };
    bool n = connects(x, z - 1);
    bool e = connects(x + 1, z);
    bool s = connects(x, z + 1);
    bool w = connects(x - 1, z);
    int mask = (n ? 1 : 0) | (e ? 2 : 0) | (s ? 4 : 0) | (w ? 8 : 0);

    // Maxis Road RUL/FSH pieces verified against shipped city network records.
    // Non-road networks retain procedural rendering until their individual RUL tables are decoded.
    if (net.networkType != NetworkType::Road) {
        net.textureId = 0;
        return;
    }

    Uint32 basePiece = 0x05004b00u;
    Uint8 orient = 0;
    switch (mask) {
        case 0:
        case 1:
        case 4:
        case 5:  basePiece = 0x05004b00u; orient = 0; break; // N-S straight/end
        case 2:
        case 8:
        case 10: basePiece = 0x05004b00u; orient = 1; break; // E-W straight/end
        case 3:  basePiece = 0x00000a00u; orient = 0; break; // N-E curve
        case 6:  basePiece = 0x00000a00u; orient = 1; break; // E-S curve
        case 12: basePiece = 0x00000a00u; orient = 2; break; // S-W curve
        case 9:  basePiece = 0x00000a00u; orient = 3; break; // W-N curve
        case 7:  basePiece = 0x00005700u; orient = 0; break; // N-E-S T-intersection
        case 14: basePiece = 0x00005700u; orient = 1; break; // E-S-W T-intersection
        case 13: basePiece = 0x00005700u; orient = 2; break; // S-W-N T-intersection
        case 11: basePiece = 0x00005700u; orient = 3; break; // W-N-E T-intersection
        case 15: basePiece = 0x05020700u; orient = 0; break; // 4-way intersection
    }
    net.textureId = basePiece;
    net.orientation = orient;

    const Float32 baseUV[4][2] = {{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
    for (int i = 0; i < 4; ++i) {
        Float32 u = baseUV[i][0];
        Float32 v = baseUV[i][1];
        switch (orient & 3u) {
            case 1: net.verts[i].u = v;        net.verts[i].v = 1.0f - u; break;
            case 2: net.verts[i].u = 1.0f - u; net.verts[i].v = 1.0f - v; break;
            case 3: net.verts[i].u = 1.0f - v; net.verts[i].v = u;        break;
            default: net.verts[i].u = u;       net.verts[i].v = v;        break;
        }
    }
}

Sint32 cSC4City::DemandForZone(ZoneType zone) const noexcept {
    switch (zone) {
        case ZoneType::ResidentialLow:  return mDemand.resLow;
        case ZoneType::ResidentialMed:  return mDemand.resMed;
        case ZoneType::ResidentialHigh: return mDemand.resHigh;
        case ZoneType::CommercialLow:   return mDemand.comLow;
        case ZoneType::CommercialMed:   return mDemand.comMed;
        case ZoneType::CommercialHigh:  return mDemand.comHigh;
        case ZoneType::IndustrialAg:    return mDemand.indAg;
        case ZoneType::IndustrialMed:   return mDemand.indMed;
        case ZoneType::IndustrialHigh:  return mDemand.indHigh;
        default:                        return 0;
    }
}

bool cSC4City::LotHasBuilding(const SC4LotRecord& lot) const noexcept {
    for (const auto& building : mBuildings) {
        const Float32 centerX = (building.minX + building.maxX) * 0.5f / 16.0f;
        const Float32 centerZ = (building.minZ + building.maxZ) * 0.5f / 16.0f;
        if (centerX >= lot.minX && centerX < lot.maxX + 1.0f &&
            centerZ >= lot.minZ && centerZ < lot.maxZ + 1.0f) {
            return true;
        }
    }
    return false;
}

bool cSC4City::DevelopOneProceduralLot() {
    if (mIsFromOriginalSave) return false;

    struct Candidate {
        std::size_t lotIndex;
        Uint32 weight;
        Uint32 resourceCost;
    };
    std::vector<Candidate> candidates;
    Uint32 totalWeight = 0;

    Float32 availableResources = mBuildingDevelopment.constructionResources;
    for (std::size_t i = 0; i < mLots.size(); ++i) {
        const auto& lot = mLots[i];
        const Sint32 activeDemand = DemandForZone(lot.zoneType);
        // cSC4BuildingDevelopmentSimulator::Grow excludes developers below 6,
        // then adds the planning-weight exemplar property to active demand.
        if (!BuildingDevelopmentState::IsGrowthDemandEligible(
                static_cast<Float32>(activeDemand))) continue;
        if (LotHasBuilding(lot)) continue;

        bool roadAccess = false;
        for (int x = static_cast<int>(lot.minX) - 1;
             x <= static_cast<int>(lot.maxX) + 1 && !roadAccess; ++x) {
            roadAccess = HasCellNetwork(x, static_cast<int>(lot.minZ) - 1) ||
                         HasCellNetwork(x, static_cast<int>(lot.maxZ) + 1);
        }
        for (int z = static_cast<int>(lot.minZ);
             z <= static_cast<int>(lot.maxZ) && !roadAccess; ++z) {
            roadAccess = HasCellNetwork(static_cast<int>(lot.minX) - 1, z) ||
                         HasCellNetwork(static_cast<int>(lot.maxX) + 1, z);
        }
        if (!roadAccess) continue;

        const Uint32 resourceCost = std::max<Uint32>(
            1u, static_cast<Uint32>(lot.sizeX) * lot.sizeZ * (1u + lot.wealth));
        if (availableResources < static_cast<Float32>(resourceCost)) continue;

        const Uint32 weight = static_cast<Uint32>(activeDemand) +
                              mBuildingDevelopment.idlePlanningSteps;
        candidates.push_back({i, weight, resourceCost});
        totalWeight += weight;
    }

    ++mBuildingDevelopment.growthAttempts;
    if (candidates.empty() || totalWeight == 0) return false;

    // Deterministic clean-room stand-in for SC4's cGZRandom choice. This keeps
    // native and WASM regressions identical while preserving weighted selection.
    Uint32 random = mJulianDay ^ (mTotalDays * 0x9e3779b9u) ^
                    static_cast<Uint32>(mLots.size() * 0x85ebca6bu);
    random ^= random << 13;
    random ^= random >> 17;
    random ^= random << 5;
    Uint32 selectedWeight = random % totalWeight;
    const Candidate* selected = &candidates.back();
    for (const auto& candidate : candidates) {
        if (selectedWeight < candidate.weight) {
            selected = &candidate;
            break;
        }
        selectedWeight -= candidate.weight;
    }

    const auto& lot = mLots[selected->lotIndex];
    const auto profile = GetZoneDevelopmentProfile(lot.zoneType);
    SC4BuildingRecord building{};
    building.name = profile.name;
    building.exemplarTGI = {
        kTypeExemplar, 0x4a283465u,
        static_cast<Uint32>(0x00010000u + mBuildings.size())
    };
    building.minX = lot.minX * 16.0f + 2.0f;
    building.maxX = (lot.maxX + 1.0f) * 16.0f - 2.0f;
    building.minZ = lot.minZ * 16.0f + 2.0f;
    building.maxZ = (lot.maxZ + 1.0f) * 16.0f - 2.0f;
    building.minY = lot.altitudeY;
    const Uint32 jitterSeed = static_cast<Uint32>(lot.minX) * 31u +
                              static_cast<Uint32>(lot.minZ) * 17u;
    const Float32 jitter = 0.75f + 0.05f * static_cast<Float32>(jitterSeed % 10u);
    building.maxY = lot.altitudeY + profile.heightMeters * jitter;
    building.zoneHint = lot.zoneType;
    building.wealthHint = lot.wealth;
    mBuildings.push_back(std::move(building));

    mBuildingDevelopment.constructionResources -=
        static_cast<Float32>(selected->resourceCost);
    ++mBuildingDevelopment.growthExecutions;
    return true;
}

void cSC4City::PlanProceduralDecline() {
    mProceduralDeclinePlans.clear();
    mBuildingDevelopment.pendingDeclinePlans = 0;
    if (mIsFromOriginalSave) return;

    struct Candidate {
        ZoneType zone;
        Uint32 weight;
    };
    std::vector<Candidate> candidates;
    Uint32 totalWeight = 0;
    const ZoneType zones[] = {
        ZoneType::ResidentialLow, ZoneType::ResidentialMed, ZoneType::ResidentialHigh,
        ZoneType::CommercialLow, ZoneType::CommercialMed, ZoneType::CommercialHigh,
        ZoneType::IndustrialMed, ZoneType::IndustrialHigh
    };

    for (ZoneType zone : zones) {
        const Sint32 activeDemand = DemandForZone(zone);
        if (!BuildingDevelopmentState::IsDeclinePlanningEligible(
                static_cast<Float32>(activeDemand))) continue;

        bool hasDevelopedLot = false;
        for (const auto& lot : mLots) {
            if (lot.zoneType == zone && LotHasBuilding(lot)) {
                hasDevelopedLot = true;
                break;
            }
        }
        if (!hasDevelopedLot) continue;

        const Uint32 weight = static_cast<Uint32>(-activeDemand) +
                              mBuildingDevelopment.idlePlanningSteps;
        candidates.push_back({zone, weight});
        totalWeight += weight;
    }

    Uint32 ordinal = 0;
    Uint32 random = mJulianDay ^ 0x27d4eb2du;
    while (!candidates.empty() && totalWeight > 0) {
        random = random * 1664525u + 1013904223u;
        Uint32 selectedWeight = random % totalWeight;
        std::size_t selectedIndex = candidates.size() - 1;
        for (std::size_t i = 0; i < candidates.size(); ++i) {
            if (selectedWeight < candidates[i].weight) {
                selectedIndex = i;
                break;
            }
            selectedWeight -= candidates[i].weight;
        }

        mProceduralDeclinePlans.push_back({
            mTotalDays + ordinal, candidates[selectedIndex].zone
        });
        ++mBuildingDevelopment.declinePlansCreated;
        ++ordinal;
        totalWeight -= candidates[selectedIndex].weight;
        candidates.erase(candidates.begin() + static_cast<std::ptrdiff_t>(selectedIndex));
    }
    mBuildingDevelopment.pendingDeclinePlans =
        static_cast<Uint32>(mProceduralDeclinePlans.size());
}

void cSC4City::ExecuteProceduralDeclinePlan() {
    if (mIsFromOriginalSave) return;

    for (auto plan = mProceduralDeclinePlans.begin();
         plan != mProceduralDeclinePlans.end();) {
        if (mTotalDays < plan->executionDay) {
            ++plan;
            continue;
        }

        const Sint32 activeDemand = DemandForZone(plan->zone);
        if (!BuildingDevelopmentState::IsDeclineExecutionEligible(
                static_cast<Float32>(activeDemand))) {
            plan = mProceduralDeclinePlans.erase(plan);
            ++mBuildingDevelopment.declinePlansDropped;
            mBuildingDevelopment.pendingDeclinePlans =
                static_cast<Uint32>(mProceduralDeclinePlans.size());
            continue;
        }

        auto building = std::find_if(
            mBuildings.begin(), mBuildings.end(),
            [zone = plan->zone](const SC4BuildingRecord& candidate) {
                return candidate.zoneHint == zone;
            });
        if (building == mBuildings.end()) {
            plan = mProceduralDeclinePlans.erase(plan);
            ++mBuildingDevelopment.declinePlansDropped;
            mBuildingDevelopment.pendingDeclinePlans =
                static_cast<Uint32>(mProceduralDeclinePlans.size());
            continue;
        }

        mBuildings.erase(building);
        mProceduralDeclinePlans.erase(plan);
        ++mBuildingDevelopment.declineExecutions;
        mBuildingDevelopment.pendingDeclinePlans =
            static_cast<Uint32>(mProceduralDeclinePlans.size());
        // ExecuteDeclinePlan stops after the tract developer reports success.
        break;
    }
}

void cSC4City::RebuildOriginalSaveDeveloperLists() {
    mGrowthDeveloperGrowthLists.clear();
    mGrowthDeveloperDeclineLists.clear();
    if (!mIsFromOriginalSave || mGrowthDeveloperTunings.empty()) {
        return;
    }

    const auto getSupply = [this](Uint32 demandId) noexcept -> Float32 {
        auto it = mLoadedDemandRecords.find(demandId);
        return it != mLoadedDemandRecords.end() ? it->second.supplyValue : 0.0f;
    };
    const Float32 resSupply1001 = getSupply(0x1001u);
    const Float32 comSupply3001 = getSupply(0x3001u);
    const Float32 comRegSupply3301 = getSupply(0x3301u);
    const Float32 indSupply4001 = getSupply(0x4001u);

    for (const Uint32 subType : kGrowthDeveloperSubTypes) {
        auto tuningIt = mGrowthDeveloperTunings.find(subType);
        if (tuningIt == mGrowthDeveloperTunings.end()) {
            continue;
        }
        const GrowthDeveloperTuning& tuning = tuningIt->second;

        const Uint32 zonableGridId =
            GrowthDeveloperSimGridIDs::ZonableCellsGridId(subType);
        const Uint32 desGridId =
            GrowthDeveloperSimGridIDs::DesirabilityGridId(subType);

        const cSC4SimGrid<Sint8>* zonableGrid = nullptr;
        for (const auto& g : mLoadedSint8Grids) {
            if (g.GetInstanceID() == zonableGridId) {
                zonableGrid = &g;
                break;
            }
        }

        const cSC4SimGrid<Uint16>* desGrid = nullptr;
        for (const auto& g : mLoadedUint16Grids) {
            if (g.GetInstanceID() == desGridId) {
                desGrid = &g;
                break;
            }
        }

        if (!zonableGrid || !desGrid) {
            continue;
        }

        const Sint32 tractsX = zonableGrid->GetTractCountX();
        const Sint32 tractsZ = zonableGrid->GetTractCountZ();
        if (tractsX <= 0 || tractsZ <= 0 ||
            desGrid->GetTractCountX() != tractsX ||
            desGrid->GetTractCountZ() != tractsZ) {
            continue;
        }

        const auto& zonableData = zonableGrid->GetGridData();
        const auto& desData = desGrid->GetGridData();
        std::vector<Sint8> flatZonable(
            static_cast<std::size_t>(tractsX) * static_cast<std::size_t>(tractsZ),
            0);
        std::vector<Sint16> flatDesirability(
            static_cast<std::size_t>(tractsX) * static_cast<std::size_t>(tractsZ),
            0);

        for (Sint32 x = 0; x < tractsX; ++x) {
            for (Sint32 z = 0; z < tractsZ; ++z) {
                const std::size_t idx =
                    static_cast<std::size_t>(x) *
                        static_cast<std::size_t>(tractsZ) +
                    static_cast<std::size_t>(z);
                flatZonable[idx] = zonableData[x][z];
                flatDesirability[idx] = static_cast<Sint16>(desData[x][z]);
            }
        }

        const Float32 scaledStagePop = ComputeStageLookupPopulation(
            tuning.developerClass,
            resSupply1001,
            comSupply3001,
            comRegSupply3301,
            indSupply4001,
            0,
            0,
            1.0f);

        std::vector<GrowthDeveloperItem> growthList;
        std::vector<GrowthDeveloperItem> declineList;
        BuildGrowthAndDeclineLists(
            tuning,
            flatZonable,
            flatDesirability,
            tractsX,
            tractsZ,
            scaledStagePop,
            growthList,
            declineList);

        mGrowthDeveloperGrowthLists[subType] = std::move(growthList);
        mGrowthDeveloperDeclineLists[subType] = std::move(declineList);
    }
}

void cSC4City::StepOriginalSaveDailyGrowth() {
    if (!mIsFromOriginalSave || mGrowthDeveloperGrowthLists.empty()) {
        return;
    }

    // Exact cSC4BuildingDevelopmentSimulator::Grow @ 0x10014fb24:
    // Each developer with int(activeDemand) >= 6 and non-empty mGrowthList
    // selects its next unmarked tract from mGrowthList (wrapping around and
    // clearing mGrowthMarks[subType] when all tracts have been visited).
    for (const Uint32 subType : kGrowthDeveloperSubTypes) {
        auto tuningIt = mGrowthDeveloperTunings.find(subType);
        auto listIt = mGrowthDeveloperGrowthLists.find(subType);
        if (tuningIt == mGrowthDeveloperTunings.end() ||
            listIt == mGrowthDeveloperGrowthLists.end() ||
            listIt->second.empty()) {
            continue;
        }

        const Uint32 rawSourceId = tuningIt->second.sourceDemandId;
        const Uint32 activeDemandId =
            (rawSourceId != 0u && rawSourceId < 0x20000u)
                ? (rawSourceId + 0x20000u)
                : rawSourceId;
        const Float32 activeDemand = LoadedDemandValue(activeDemandId);
        if (!BuildingDevelopmentState::IsGrowthDemandEligible(activeDemand)) {
            continue;
        }

        std::vector<std::pair<Sint32, Sint32>>* marksPtr = nullptr;
        for (auto& markEntry : mBuildingDevSimRecord.growthMarks) {
            if (markEntry.markKey == subType) {
                marksPtr = &markEntry.tracts;
                break;
            }
        }
        if (!marksPtr) {
            mBuildingDevSimRecord.growthMarks.push_back(
                BuildingDevelopmentSimSaveRecord::GrowthMarkEntry{subType, {}});
            marksPtr = &mBuildingDevSimRecord.growthMarks.back().tracts;
        }

        GrowthDeveloperItem selectedTract{};
        if (SelectNextUnmarkedGrowthTract(
                listIt->second, *marksPtr, selectedTract)) {
            ++mBuildingDevelopment.growthAttempts;
        }
    }
}

void cSC4City::StepSimulation(int days) {
    for (int d = 0; d < days; ++d) {
        bool monthlyEvent = false;
        ++mTotalDays;
        ++mJulianDay;
        CivilFromJulianDay(mJulianDay, mYear, mMonth, mDay);

        // cSC4Simulator::SimulateTimePassage posts these events after
        // cGZDate::operator++(). cGZDate numbers Monday as weekday 1.
        ++mCadence.dailyEvents;
        ++mBuildingDevelopment.dailyCycles;
        if (HasCivicSaveRecord(kCLSID_cSC4FireProtectionSim)) {
            mFireProtectionSaveRecord.protection.FinalizeDailyCoverageCycle();
        }
        if (HasCivicSaveRecord(kCLSID_cSC4PoliceSimulator)) {
            mPoliceSaveRecord.protection.FinalizeDailyCoverageCycle();
        }
        if (HasCivicSaveRecord(kCLSID_cSC4FloraSimulator)) {
            const Float32 dailyStep =
                mFloraTuning.inverseProp00 > 0.0f ? mFloraTuning.inverseProp00 : (1.0f / 30.0f);
            mFloraSaveRecord.StepDailyBurntFloraRemoval(dailyStep, mFloraTuning.fieldFC);
        }
        if (HasCivicSaveRecord(kCLSID_cSC4CommercialSimulator)) {
            mCommercialSaveRecord.StepIdleStateMachine();
        }
        if (mIsFromOriginalSave) {
            StepOriginalSaveDailyGrowth();
        } else {
            DevelopOneProceduralLot();
            ExecuteProceduralDeclinePlan();
        }
        // ExecuteDeclinePlan is always called after Grow/ExecuteGrowthPlan.
        ++mBuildingDevelopment.declinePlanCycles;
        const Uint32 weekDay = (mJulianDay % 7u) + 1u;
        const Uint32 dayOfYear = DayOfYear(mYear, mMonth, mDay);
        if (HasCivicSaveRecord(kCLSID_cSC4Simulator)) {
            Uint32 resetTimers = 0;
            mSimulatorSaveRecord.StepSpeedGatedTimePassage(
                mSimulatorSaveRecord.intervalPeriodMs98, resetTimers);
            mSimulatorSaveRecord.julianDayNumber = mJulianDay;
            mSimulatorSaveRecord.intervalCounters[4] = dayOfYear;
        }
        if (weekDay == 1u) {
            ++mCadence.weeklyEvents;
            mCadence.lastWeekOfYear = ((dayOfYear - 1u) / 7u) + 1u;
        }
        if (mDay == 1u) {
            monthlyEvent = true;
            ++mCadence.monthlyEvents;
            ++mBuildingDevelopment.monthlyCycles;
            if (dayOfYear == 1u) {
                ++mCadence.yearlyEvents;
                mTractDeveloperState.OnNewYear();
            }

            if (mIsFromOriginalSave) {
                RebuildOriginalSaveDeveloperLists();
            }

            if (HasCivicSaveRecord(kCLSID_cSC4PowerSimulator)) {
                mPowerSaveRecord.StepMonthly();
            }
            if (HasCivicSaveRecord(kCLSID_cSC4PollutionSimulator)) {
                mPollutionSaveRecord.StepMonthly();
            }
            if (HasCivicSaveRecord(kCLSID_cSC4PoliceSimulator)) {
                mPoliceSaveRecord.DoMonthlyCriminalRehabilitation(0.05f);
                mPoliceSaveRecord.ProcessMonthlyCrimeStats();
            }
            if (HasCivicSaveRecord(kCLSID_cSC4OrdinanceSimulator)) {
                const Sint64 prevExp = mOrdinanceSaveRecord.GetOrdinanceMonthlyExpense();
                const Sint64 prevInc = mOrdinanceSaveRecord.GetOrdinanceMonthlyIncome();
                if (prevExp == 0 && prevInc == 0) {
                    mOrdinanceSaveRecord.StepMonthly({});
                }
            }

            Float32 residentialDemand = 0.0f;
            Float32 commercialDemand = 0.0f;
            Float32 industrialDemand = 0.0f;
            if (!mLoadedDemandRecords.empty()) {
                residentialDemand = LoadedDemandValue(0x1001u);
                commercialDemand = LoadedDemandValue(0x3001u);
                industrialDemand = LoadedDemandValue(0x4001u);
            } else {
                residentialDemand = static_cast<Float32>(
                    mDemand.resLow + mDemand.resMed + mDemand.resHigh);
                commercialDemand = static_cast<Float32>(
                    mDemand.comLow + mDemand.comMed + mDemand.comHigh);
                industrialDemand = static_cast<Float32>(
                    mDemand.indAg + mDemand.indMed + mDemand.indHigh);
            }
            mBuildingDevelopment.constructionResources = static_cast<Float32>(
                mBuildingDevelopment.CalculateMonthlyResources(
                    residentialDemand, commercialDemand, industrialDemand));

            // Monthly tax revenue minus civic & network maintenance.
            // When the city came from an original .sc4 save, advance the exact
            // cSC4BudgetSimulator monthly step (DoMonthlySimulation
            // @ 0x10022061e) on the decoded save record; otherwise keep the
            // procedural approximation for generated cities.
            Sint64 taxIncome = (static_cast<Sint64>(mResidentialPop) * 9 +
                                static_cast<Sint64>(mCommercialPop)  * 12 +
                                static_cast<Sint64>(mIndustrialPop)  * 10) / 15;
            Sint64 roadUpkeep = static_cast<Sint64>(mNetworks.size()) / 3;
            if (mHasBudgetSaveRecord) {
                std::vector<BudgetTaxLotInput> taxLots;
                taxLots.reserve(mLots.size());
                for (const auto& lot : mLots) {
                    if (!LotHasBuilding(lot)) continue;
                    Sint32 purpose = 0;
                    if (lot.zoneType >= ZoneType::ResidentialLow &&
                        lot.zoneType <= ZoneType::ResidentialHigh) {
                        purpose = 1;
                    } else if (lot.zoneType == ZoneType::CommercialHigh) {
                        purpose = 3;
                    } else if (lot.zoneType >= ZoneType::CommercialLow &&
                               lot.zoneType <= ZoneType::CommercialMed) {
                        purpose = 2;
                    } else if (lot.zoneType == ZoneType::IndustrialAg) {
                        purpose = 4;
                    } else if (lot.zoneType >= ZoneType::IndustrialMed &&
                               lot.zoneType <= ZoneType::IndustrialHigh) {
                        const Uint8 w = std::clamp<Uint8>(lot.wealth, 1u, 3u);
                        purpose = (w == 1u) ? 5 : ((w == 2u) ? 6 : 7);
                    }
                    if (purpose != 0) {
                        BudgetTaxLotInput item{};
                        item.purpose = purpose;
                        item.wealth = static_cast<Sint32>(std::clamp<Uint8>(lot.wealth, 1u, 3u));
                        item.assessedValueProperty = static_cast<Sint64>(
                            std::max<Uint32>(lot.capacity,
                                             static_cast<Uint32>(lot.sizeX) * lot.sizeZ * 10u));
                        item.sizeX = lot.sizeX;
                        item.sizeZ = lot.sizeZ;
                        const Uint8 lv = mLandValueGrid.GetCellValue(lot.minX, lot.minZ);
                        item.landValueClass = static_cast<Uint8>(
                            LandValueSimulatorSaveRecord::GetLandValueTypeForValue(lv, 32u, 96u));
                        item.holderCategory = item.wealth;
                        item.simulatorTypeIndex = 0;
                        taxLots.push_back(item);
                    }
                }
                if (!taxLots.empty()) {
                    const Sint64 computedTax =
                        ComputeBudgetTax(mBudgetSaveRecord, mBudgetTuning, taxLots);
                    if (computedTax > 0) {
                        taxIncome = computedTax;
                    }
                }
                BudgetMonthlyInputs budgetInputs{};
                budgetInputs.taxIncome = taxIncome;
                budgetInputs.cityExpense = roadUpkeep +
                    (HasCivicSaveRecord(kCLSID_cSC4OrdinanceSimulator)
                         ? (mOrdinanceSaveRecord.GetOrdinanceMonthlyExpense() -
                            mOrdinanceSaveRecord.GetOrdinanceMonthlyIncome())
                         : 0);
                budgetInputs.month = static_cast<Sint32>(mMonth);
                budgetInputs.year = static_cast<Sint32>(mYear);
                ApplyMonthlyBudgetStep(mBudgetSaveRecord, mBudgetTuning, budgetInputs);
                mFunds = mBudgetSaveRecord.totalFunds;
                if (mIsFromOriginalSave) {
                    MarkSaveEntryDirty(mBudgetSaveTGI);
                }
            } else {
                mFunds += (taxIncome - roadUpkeep);
            }
        }

        // Grow undeveloped lots that have road access and positive RCI demand
        Uint32 rPopFromLots = 0;
        Uint32 cPopFromLots = 0;
        Uint32 iPopFromLots = 0;

        for (auto& lot : mLots) {
            if (lot.capacity == 0) {
                lot.capacity = static_cast<Uint32>(lot.sizeX) * lot.sizeZ * 14u;
            }
            if (!LotHasBuilding(lot)) continue;
            if (lot.zoneType >= ZoneType::ResidentialLow && lot.zoneType <= ZoneType::ResidentialHigh) {
                rPopFromLots += lot.capacity;
            } else if (lot.zoneType >= ZoneType::CommercialLow && lot.zoneType <= ZoneType::CommercialHigh) {
                cPopFromLots += lot.capacity;
            } else if (lot.zoneType >= ZoneType::IndustrialAg && lot.zoneType <= ZoneType::IndustrialHigh) {
                iPopFromLots += lot.capacity;
            }
        }

        if (!mIsFromOriginalSave) {
            mResidentialPop = rPopFromLots;
            mCommercialPop  = cPopFromLots;
            mIndustrialPop  = iPopFromLots;

            // Clean-room fallback demand model. Original saves retain their
            // decoded cSC4Demand values until the census/economy-link phases
            // are fully reproduced.
            Sint32 targetC = static_cast<Sint32>(mResidentialPop / 2) - static_cast<Sint32>(mCommercialPop);
            Sint32 targetI = static_cast<Sint32>(mResidentialPop / 2) - static_cast<Sint32>(mIndustrialPop);
            Sint32 targetR = static_cast<Sint32>(mCommercialPop + mIndustrialPop) - static_cast<Sint32>(mResidentialPop);

            mDemand.resLow  = std::clamp(1200 + targetR / 4, -4000, 4000);
            mDemand.resMed  = std::clamp(900  + targetR / 5, -4000, 4000);
            mDemand.resHigh = std::clamp(450  + targetR / 8, -4000, 4000);
            mDemand.comLow  = std::clamp(1000 + targetC / 4, -4000, 4000);
            mDemand.comMed  = std::clamp(800  + targetC / 5, -4000, 4000);
            mDemand.comHigh = std::clamp(400  + targetC / 8, -4000, 4000);
            mDemand.indAg   = std::clamp(500  + targetI / 6, -4000, 4000);
            mDemand.indMed  = std::clamp(1100 + targetI / 4, -4000, 4000);
            mDemand.indHigh = std::clamp(600  + targetI / 6, -4000, 4000);
        }

        // PlanDecline runs in the monthly idle phase. Maxis clears the old
        // list, uses <= -21 demand to choose profiles, and schedules entries
        // on consecutive simulation days.
        if (monthlyEvent) PlanProceduralDecline();
    }

    UpdateSimulationGrids();
}

bool cSC4City::ApplyTool(ToolType tool, int x0, int z0, int x1, int z1) {
    if (x0 > x1) std::swap(x0, x1);
    if (z0 > z1) std::swap(z0, z1);
    x0 = std::clamp(x0, 0, mCellCountX - 1);
    x1 = std::clamp(x1, 0, mCellCountX - 1);
    z0 = std::clamp(z0, 0, mCellCountZ - 1);
    z1 = std::clamp(z1, 0, mCellCountZ - 1);

    if (tool == ToolType::Inspect) return false;

    if (tool == ToolType::TerraformRaise || tool == ToolType::TerraformLower) {
        float delta = (tool == ToolType::TerraformRaise) ? 6.0f : -6.0f;
        for (int vz = z0; vz <= z1 + 1; ++vz) {
            for (int vx = x0; vx <= x1 + 1; ++vx) {
                SetVertexHeight(vx, vz, GetVertexHeight(vx, vz) + delta);
            }
        }
        mFunds -= 50;
        UpdateSimulationGrids();
        return true;
    }

    if (tool == ToolType::Bulldoze) {
        mLots.erase(std::remove_if(mLots.begin(), mLots.end(), [&](const SC4LotRecord& l) {
            return !(l.maxX < x0 || l.minX > x1 || l.maxZ < z0 || l.minZ > z1);
        }), mLots.end());

        mBuildings.erase(std::remove_if(mBuildings.begin(), mBuildings.end(), [&](const SC4BuildingRecord& b) {
            int bx = static_cast<int>((b.minX + b.maxX) * 0.5f / 16.0f);
            int bz = static_cast<int>((b.minZ + b.maxZ) * 0.5f / 16.0f);
            return bx >= x0 && bx <= x1 && bz >= z0 && bz <= z1;
        }), mBuildings.end());

        mNetworks.erase(std::remove_if(mNetworks.begin(), mNetworks.end(), [&](const SC4NetworkRecord& n) {
            return n.tileX >= x0 && n.tileX <= x1 && n.tileZ >= z0 && n.tileZ <= z1;
        }), mNetworks.end());

        mFunds -= 25;
        RebuildSpatialLookups();
        for (int z = z0 - 1; z <= z1 + 1; ++z) {
            for (int x = x0 - 1; x <= x1 + 1; ++x) {
                AutoTileNetworkAt(x, z);
            }
        }
        UpdateSimulationGrids();
        return true;
    }

    // Network building tools
    if (tool >= ToolType::BuildRoad && tool <= ToolType::BuildRail) {
        NetworkType nt = NetworkType::Road;
        if (tool == ToolType::BuildStreet)  nt = NetworkType::Street;
        if (tool == ToolType::BuildAvenue)  nt = NetworkType::Avenue;
        if (tool == ToolType::BuildHighway) nt = NetworkType::Highway;
        if (tool == ToolType::BuildRail)    nt = NetworkType::Rail;

        for (int z = z0; z <= z1; ++z) {
            for (int x = x0; x <= x1; ++x) {
                if (HasCellNetwork(x, z)) continue;
                SC4NetworkRecord net{};
                net.tileX = static_cast<Uint8>(x);
                net.tileZ = static_cast<Uint8>(z);
                net.networkType = nt;
                float wx0 = x * 16.0f;
                float wz0 = z * 16.0f;
                float wx1 = (x + 1) * 16.0f;
                float wz1 = (z + 1) * 16.0f;
                float y00 = GetVertexHeight(x, z) + 0.15f;
                float y10 = GetVertexHeight(x + 1, z) + 0.15f;
                float y11 = GetVertexHeight(x + 1, z + 1) + 0.15f;
                float y01 = GetVertexHeight(x, z + 1) + 0.15f;
                net.centerX = 0.5f * (wx0 + wx1);
                net.centerZ = 0.5f * (wz0 + wz1);
                net.centerY = 0.25f * (y00 + y10 + y11 + y01);
                net.verts[0] = {wx0, y00, wz0, 0.0f, 0.0f, 0xFFD8D4D8u};
                net.verts[1] = {wx1, y10, wz0, 1.0f, 0.0f, 0xFFD8D4D8u};
                net.verts[2] = {wx1, y11, wz1, 1.0f, 1.0f, 0xFFD8D4D8u};
                net.verts[3] = {wx0, y01, wz1, 0.0f, 1.0f, 0xFFD8D4D8u};
                mNetworks.push_back(net);
                mFunds -= 10;
            }
        }
        RebuildSpatialLookups();
        for (int z = z0 - 1; z <= z1 + 1; ++z) {
            for (int x = x0 - 1; x <= x1 + 1; ++x) {
                AutoTileNetworkAt(x, z);
            }
        }
        UpdateSimulationGrids();
        return true;
    }

    // Zoning & Civic Plopping tools
    ZoneType zType = ZoneType::ResidentialLow;
    std::string bldName = "Residential Home";
    float heightMeters = 9.0f;
    Uint8 wealth = 1;

    switch (tool) {
        case ToolType::ZoneResLow:     zType = ZoneType::ResidentialLow;  bldName = "R$ Cottage";        heightMeters = 8.0f;  wealth = 1; break;
        case ToolType::ZoneResMed:     zType = ZoneType::ResidentialMed;  bldName = "R$$ Apartments";    heightMeters = 24.0f; wealth = 2; break;
        case ToolType::ZoneResHigh:    zType = ZoneType::ResidentialHigh; bldName = "R$$$ Condominiums"; heightMeters = 62.0f; wealth = 3; break;
        case ToolType::ZoneComLow:     zType = ZoneType::CommercialLow;   bldName = "Cs$ Retail Shop";   heightMeters = 10.0f; wealth = 1; break;
        case ToolType::ZoneComMed:     zType = ZoneType::CommercialMed;   bldName = "Co$$ Office Plaza"; heightMeters = 32.0f; wealth = 2; break;
        case ToolType::ZoneComHigh:    zType = ZoneType::CommercialHigh;  bldName = "Co$$$ Skyscraper";  heightMeters = 84.0f; wealth = 3; break;
        case ToolType::ZoneIndAg:      zType = ZoneType::IndustrialAg;    bldName = "I-R Farm Silo";     heightMeters = 11.0f; wealth = 1; break;
        case ToolType::ZoneIndMed:     zType = ZoneType::IndustrialMed;   bldName = "I-M Factory";       heightMeters = 18.0f; wealth = 2; break;
        case ToolType::ZoneIndHigh:    zType = ZoneType::IndustrialHigh;  bldName = "I-HT Tech Lab";     heightMeters = 26.0f; wealth = 3; break;
        case ToolType::PlopPowerPlant: zType = ZoneType::PloppedCivic;    bldName = "Coal Power Plant";  heightMeters = 38.0f; wealth = 2; break;
        case ToolType::PlopWaterPump:  zType = ZoneType::PloppedCivic;    bldName = "Water Pumping Stn"; heightMeters = 14.0f; wealth = 2; break;
        case ToolType::PlopPolice:     zType = ZoneType::PloppedCivic;    bldName = "Police Precinct";   heightMeters = 18.0f; wealth = 2; break;
        case ToolType::PlopFire:       zType = ZoneType::PloppedCivic;    bldName = "Fire Station";      heightMeters = 16.0f; wealth = 2; break;
        case ToolType::PlopPark:       zType = ZoneType::PloppedCivic;    bldName = "City Plaza Park";   heightMeters = 4.0f;  wealth = 3; break;
        default: break;
    }

    const bool isCivicPlop =
        tool == ToolType::PlopPowerPlant || tool == ToolType::PlopWaterPump ||
        tool == ToolType::PlopPolice || tool == ToolType::PlopFire ||
        tool == ToolType::PlopPark;

    // Subdivide zoning immediately, but leave RCI lots empty for the daily
    // development simulator. Civic plops still instantiate immediately.
    int step = (x1 - x0 >= 3 && z1 - z0 >= 3) ? 2 : 1;
    for (int z = z0; z <= z1; z += step) {
        for (int x = x0; x <= x1; x += step) {
            int lx1 = std::min(x + step - 1, x1);
            int lz1 = std::min(z + step - 1, z1);
            if (HasCellNetwork(x, z) || IsCellUnderwater(x, z)) continue;

            SC4LotRecord lot{};
            lot.minX = static_cast<Uint8>(x);
            lot.minZ = static_cast<Uint8>(z);
            lot.maxX = static_cast<Uint8>(lx1);
            lot.maxZ = static_cast<Uint8>(lz1);
            lot.sizeX = static_cast<Uint8>(lx1 - x + 1);
            lot.sizeZ = static_cast<Uint8>(lz1 - z + 1);
            lot.altitudeY = GetCellCenterHeight(x, z);
            lot.zoneType = zType;
            lot.wealth = wealth;
            lot.flags = 0x50u; // powered + watered
            lot.capacity = static_cast<Uint32>(lot.sizeX) * lot.sizeZ * (wealth * 18u);
            mLots.push_back(lot);

            if (isCivicPlop) {
                SC4BuildingRecord bld{};
                bld.name = bldName;
                bld.exemplarTGI = {kTypeExemplar, 0x4a283465u, static_cast<Uint32>(0x00010000u + mBuildings.size())};
                bld.minX = x * 16.0f + 2.0f;
                bld.maxX = (lx1 + 1) * 16.0f - 2.0f;
                bld.minZ = z * 16.0f + 2.0f;
                bld.maxZ = (lz1 + 1) * 16.0f - 2.0f;
                bld.minY = lot.altitudeY;
                float jitter = 0.75f + 0.5f * ((static_cast<unsigned>(x * 31 + z * 17) % 10) / 10.0f);
                bld.maxY = lot.altitudeY + heightMeters * jitter;
                bld.zoneHint = zType;
                bld.wealthHint = wealth;
                mBuildings.push_back(std::move(bld));
            }

            mFunds -= 50;
        }
    }

    RebuildSpatialLookups();
    UpdateSimulationGrids();
    return true;
}

bool cSC4City::SaveToSC4Buffer(std::vector<Uint8>& outBuffer) const {
    // Write a valid DBPF 1.0 archive containing cSC4ReadRegionalCity + Heightmap + LandValue SimGrid
    cIGZOStream rcStream;
    rcStream << Uint16(1) << Uint16(13);
    rcStream << mRegionTileX << mRegionTileZ;
    rcStream << Uint32(mCellCountX / 64) << Uint32(mCellCountZ / 64);
    rcStream << mResidentialPop << mCommercialPop << mIndustrialPop;

    cIGZOStream hmStream;
    hmStream << Uint16(2);
    for (float h : mHeightmap) hmStream << h;

    cIGZOStream sgBody;
    mLandValueGrid.Write(sgBody);
    cIGZOStream sgStream;
    Uint32 sgRecSize = static_cast<Uint32>(sgBody.Size() + 12);
    Uint32 sgCrc = ComputeSC4RecordCRC(0x14A25B18u, sgBody.Buffer().data(), sgBody.Size());
    sgStream << sgRecSize << sgCrc << Uint32(0x14A25B18u);
    sgStream.WriteBytes(sgBody.Buffer().data(), sgBody.Size());

    struct Subfile {
        cSC4TGI tgi;
        std::vector<Uint8> data;
    };
    std::vector<Subfile> files = {
        {{kCLSID_cSC4ReadRegionalCity, 0xca027ee1u, 0u}, rcStream.Buffer()},
        {{kCLSID_cSTETerrainHeightmap, 0x299b2d1bu, 1u}, hmStream.Buffer()},
        {{kGZCLSID_cSC4SimGridUint8,   0x299b2d1bu, 0u}, sgStream.Buffer()}
    };

    auto appendPersistRecord = [&files](Uint32 clsid, Uint32 memAddr,
                                        const auto& record) {
        cIGZOStream body;
        if (!record.Write(body)) {
            return;
        }
        cIGZOStream rec;
        const Uint32 recSize = static_cast<Uint32>(body.Size() + 12u);
        const Uint32 crc = ComputeSC4RecordCRC(memAddr, body.Buffer().data(), body.Size());
        rec << recSize << crc << memAddr;
        rec.WriteBytes(body.Buffer().data(), body.Size());
        files.push_back(Subfile{{clsid, 0x299b2d1bu, 0u}, std::move(rec.Buffer())});
    };

    if (mHasBudgetSaveRecord) {
        appendPersistRecord(kCLSID_cSC4BudgetSimulator,
                            mBudgetSaveRecord.memAddr, mBudgetSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4Simulator)) {
        appendPersistRecord(kCLSID_cSC4Simulator,
                            mSimulatorSaveRecord.memAddr, mSimulatorSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4LandValueSimulator)) {
        appendPersistRecord(kCLSID_cSC4LandValueSimulator,
                            mLandValueSaveRecord.memAddr, mLandValueSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4CommercialSimulator)) {
        appendPersistRecord(kCLSID_cSC4CommercialSimulator,
                            mCommercialSaveRecord.memAddr, mCommercialSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4IndustrialSimulator)) {
        appendPersistRecord(kCLSID_cSC4IndustrialSimulator,
                            mIndustrialSaveRecord.memAddr, mIndustrialSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4FireProtectionSim)) {
        appendPersistRecord(kCLSID_cSC4FireProtectionSim,
                            mFireProtectionSaveRecord.memAddr, mFireProtectionSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4PoliceSimulator)) {
        appendPersistRecord(kCLSID_cSC4PoliceSimulator,
                            mPoliceSaveRecord.memAddr, mPoliceSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4FlammabilitySim)) {
        appendPersistRecord(kCLSID_cSC4FlammabilitySim,
                            mFlammabilitySaveRecord.memAddr, mFlammabilitySaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4CrimeSimulator)) {
        appendPersistRecord(kCLSID_cSC4CrimeSimulator,
                            mCrimeSaveRecord.memAddr, mCrimeSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4AuraSimulator)) {
        appendPersistRecord(kCLSID_cSC4AuraSimulator,
                            mAuraSaveRecord.memAddr, mAuraSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4PowerSimulator)) {
        appendPersistRecord(kCLSID_cSC4PowerSimulator,
                            mPowerSaveRecord.memAddr, mPowerSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4PlumbingSimulator)) {
        appendPersistRecord(kCLSID_cSC4PlumbingSimulator,
                            mPlumbingSaveRecord.memAddr, mPlumbingSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4PollutionSimulator)) {
        appendPersistRecord(kCLSID_cSC4PollutionSimulator,
                            mPollutionSaveRecord.memAddr, mPollutionSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4ResidentialSimulator)) {
        appendPersistRecord(kCLSID_cSC4ResidentialSimulator,
                            mResidentialSaveRecord.memAddr, mResidentialSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4TrafficSimulator)) {
        appendPersistRecord(kCLSID_cSC4TrafficSimulator,
                            mTrafficSaveRecord.memAddr, mTrafficSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4CivicBuildingSim)) {
        appendPersistRecord(kCLSID_cSC4CivicBuildingSim,
                            mCivicBuildingSaveRecord.memAddr, mCivicBuildingSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4FloraSimulator)) {
        appendPersistRecord(kCLSID_cSC4FloraSimulator,
                            mFloraSaveRecord.memAddr, mFloraSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4MySimAgentSimulator)) {
        appendPersistRecord(kCLSID_cSC4MySimAgentSimulator,
                            mMySimAgentSaveRecord.memAddr, mMySimAgentSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4NeighborsSimulator)) {
        appendPersistRecord(kCLSID_cSC4NeighborsSimulator,
                            mNeighborsSaveRecord.memAddr, mNeighborsSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4OrdinanceSimulator)) {
        appendPersistRecord(kCLSID_cSC4OrdinanceSimulator,
                            mOrdinanceSaveRecord.memAddr, mOrdinanceSaveRecord);
    }
    if (HasCivicSaveRecord(kCLSID_cSC4WeatherSimulator)) {
        appendPersistRecord(kCLSID_cSC4WeatherSimulator,
                            mWeatherSaveRecord.memAddr, mWeatherSaveRecord);
    }

    cIGZOStream dbpf;
    Uint32 dataOffset = 96;
    std::vector<DBPFIndexEntry> idx;
    for (const auto& f : files) {
        DBPFIndexEntry e{};
        e.tgi = f.tgi;
        e.offset = dataOffset;
        e.fileSize = static_cast<Uint32>(f.data.size());
        idx.push_back(e);
        dataOffset += e.fileSize;
    }

    Uint32 indexOffset = dataOffset;
    Uint32 indexSize = static_cast<Uint32>(idx.size() * 20);

    // 96-byte DBPF 1.0 header
    dbpf.WriteBytes("DBPF", 4);
    dbpf << Uint32(1) << Uint32(0); // v1.0
    for (int i = 0; i < 5; ++i) dbpf << Uint32(0);
    dbpf << Uint32(7) << static_cast<Uint32>(idx.size()) << indexOffset << indexSize;
    for (int i = 0; i < 12; ++i) dbpf << Uint32(0);

    for (const auto& f : files) {
        dbpf.WriteBytes(f.data.data(), f.data.size());
    }
    for (const auto& e : idx) {
        dbpf << e.tgi.type << e.tgi.group << e.tgi.instance << e.offset << e.fileSize;
    }

    outBuffer = std::move(dbpf.Buffer());
    return true;
}

void cSC4City::MarkSaveEntryDirty(const cSC4TGI& tgi) {
    const auto it = std::find(mDirtySaveEntries.begin(), mDirtySaveEntries.end(), tgi);
    if (it == mDirtySaveEntries.end()) {
        mDirtySaveEntries.push_back(tgi);
    }
}

bool cSC4City::SerializeDirtySaveEntry(const cSC4TGI& tgi,
                                       std::vector<Uint8>& outData) const {
    if (tgi.type != kCLSID_cSC4BudgetSimulator || !mHasBudgetSaveRecord) {
        return false;
    }

    cIGZOStream body;
    if (!mBudgetSaveRecord.Write(body) ||
        body.Size() > std::numeric_limits<Uint32>::max() - 12u) {
        return false;
    }

    // cSC4COMSerializer verifies [size][crc][memAddr] on load @ 0x1003d0df1.
    // ComputeSC4RecordCRC covers memAddr + body with the original byte cap.
    cIGZOStream record;
    const Uint32 recordSize = static_cast<Uint32>(body.Size() + 12u);
    const Uint32 crc = ComputeSC4RecordCRC(
        mBudgetSaveRecord.memAddr, body.Buffer().data(), body.Size());
    record << recordSize << crc << mBudgetSaveRecord.memAddr;
    record.WriteBytes(body.Buffer().data(), body.Size());
    outData = record.Buffer();
    return true;
}

bool cSC4City::SaveOriginalToSC4Buffer(std::vector<Uint8>& outBuffer) const {
    if (mOriginalSaveBytes.size() < 96u) return false;

    // The unmodified oracle is a verbatim clone: shipped saves can contain
    // holes and physical entry layouts that a canonical rebuild would alter.
    if (mDirtySaveEntries.empty()) {
        outBuffer = mOriginalSaveBytes;
        return true;
    }

    cDBPFArchive source;
    if (!source.OpenFromMemory(mOriginalSaveBytes, mSourcePath)) return false;

    const auto isDirty = [this](const cSC4TGI& tgi) {
        return std::find(mDirtySaveEntries.begin(), mDirtySaveEntries.end(), tgi) !=
               mDirtySaveEntries.end();
    };
    constexpr cSC4TGI kDirTGI{kTypeDIR, kTypeDIR, 0x286b1f03u};

    std::vector<Uint8> regeneratedDir;
    bool hasDir = false;
    for (const auto& entry : source.Entries()) {
        if (entry.tgi == kDirTGI) {
            std::vector<Uint8> originalDir;
            if (!source.ReadEntry(entry, originalDir)) return false;
            hasDir = true;

            // DIR key construction is @ 0x10036ab2d-43; its 16-byte records
            // are type@+0, group@+4, instance@+8, uncompressedSize@+12 per
            // ReadCompressedSetRecord @ 0x10036aae4. DoOpenRecord chooses
            // decompression by DIR membership @ 0x10036b62f. Dirty records
            // are raw, so remove only their records and preserve all others.
            const std::size_t wholeRecords = originalDir.size() - originalDir.size() % 16u;
            regeneratedDir.reserve(originalDir.size());
            for (std::size_t pos = 0; pos < wholeRecords; pos += 16u) {
                cSC4TGI recordTGI{};
                std::memcpy(&recordTGI.type, originalDir.data() + pos, 4);
                std::memcpy(&recordTGI.group, originalDir.data() + pos + 4, 4);
                std::memcpy(&recordTGI.instance, originalDir.data() + pos + 8, 4);
                if (!isDirty(recordTGI)) {
                    regeneratedDir.insert(regeneratedDir.end(),
                                          originalDir.begin() + pos,
                                          originalDir.begin() + pos + 16);
                }
            }
            regeneratedDir.insert(regeneratedDir.end(),
                                  originalDir.begin() + wholeRecords, originalDir.end());
            break;
        }
    }

    struct OutputFile {
        DBPFIndexEntry index{};
        std::vector<Uint8> data;
    };
    std::vector<OutputFile> files;
    files.reserve(source.Entries().size());
    std::size_t patchedRecords = 0;

    for (const auto& sourceEntry : source.Entries()) {
        OutputFile file;
        file.index.tgi = sourceEntry.tgi;
        if (hasDir && sourceEntry.tgi == kDirTGI) {
            file.data = regeneratedDir;
        } else if (isDirty(sourceEntry.tgi)) {
            if (!SerializeDirtySaveEntry(sourceEntry.tgi, file.data)) return false;
            ++patchedRecords;
        } else if (!source.ReadRawEntry(sourceEntry, file.data)) {
            return false;
        }
        files.push_back(std::move(file));
    }
    if (patchedRecords != mDirtySaveEntries.size()) return false;

    Uint64 nextOffset = 96u;
    for (auto& file : files) {
        if (file.data.size() > std::numeric_limits<Uint32>::max() ||
            nextOffset > std::numeric_limits<Uint32>::max()) {
            return false;
        }
        file.index.offset = static_cast<Uint32>(nextOffset);
        file.index.fileSize = static_cast<Uint32>(file.data.size());
        nextOffset += file.data.size();
    }
    const Uint64 indexOffset = nextOffset;
    const Uint64 indexSize = static_cast<Uint64>(files.size()) * 20u;
    if (indexOffset > std::numeric_limits<Uint32>::max() ||
        indexSize > std::numeric_limits<Uint32>::max() ||
        indexOffset + indexSize > std::numeric_limits<Uint32>::max()) {
        return false;
    }

    cIGZOStream dbpf;
    dbpf.WriteBytes(mOriginalSaveBytes.data(), 96u);
    for (const auto& file : files) {
        dbpf.WriteBytes(file.data.data(), file.data.size());
    }
    for (const auto& file : files) {
        dbpf << file.index.tgi.type << file.index.tgi.group << file.index.tgi.instance
             << file.index.offset << file.index.fileSize;
    }
    outBuffer = dbpf.Buffer();

    auto patchUint32 = [&outBuffer](std::size_t offset, Uint32 value) {
        for (unsigned int i = 0; i < 4; ++i) {
            outBuffer[offset + i] = static_cast<Uint8>(value >> (i * 8u));
        }
    };
    patchUint32(36, static_cast<Uint32>(files.size()));
    patchUint32(40, static_cast<Uint32>(indexOffset));
    patchUint32(44, static_cast<Uint32>(indexSize));
    // Zero holes are structurally valid per VerifyHeaderRecordIntegrity @
    // 0x10037020f. All other header fields, including both dates, are cloned.
    patchUint32(48, 0);
    patchUint32(52, 0);
    patchUint32(56, 0);
    return true;
}

bool cSC4City::SaveToSC4File(const std::string& path) {
    const bool hasOriginal = mIsFromOriginalSave && !mOriginalSaveBytes.empty();
    std::vector<Uint8> buffer;
    const bool serialized = hasOriginal ? SaveOriginalToSC4Buffer(buffer)
                                        : SaveToSC4Buffer(buffer);
    if (!serialized) return false;

    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(buffer.data()),
              static_cast<std::streamsize>(buffer.size()));
    out.close();
    if (!out) return false;

    if (hasOriginal) {
        mOriginalSaveBytes = std::move(buffer);
        mDirtySaveEntries.clear();
        mSourcePath = path;
    }
    return true;
}

bool cSC4City::SetTaxRate(Sint32 purpose, Sint32 wealth, Float32 rate) noexcept {
    // An original-save patch needs the authoritative source TGI; never enqueue
    // a synthetic {0,0,0} record that cannot be patched by SaveToSC4File.
    if (mIsFromOriginalSave && !mHasBudgetSaveRecord) return false;
    if (!mHasBudgetSaveRecord) {
        mHasBudgetSaveRecord = true;
        mBudgetSaveRecord.SetTotalFunds(mFunds);
    }
    const Sint16 taxType = GetTaxTypeForPurposeAndWealth(purpose, wealth);
    if (taxType < 0 || taxType >= 12) {
        return false;
    }
    // cSC4BudgetSimulator::SetTaxRate @ 0x100221fb2 changes only this
    // simulator's serialized taxCategories[taxType].taxRate field.
    mBudgetSaveRecord.SetTaxRate(taxType, rate);
    if (mIsFromOriginalSave) {
        MarkSaveEntryDirty(mBudgetSaveTGI);
    }
    return true;
}

Float32 cSC4City::GetTaxRate(Sint32 purpose, Sint32 wealth) const noexcept {
    if (!mHasBudgetSaveRecord) {
        return 9.0f;
    }
    const Sint16 taxType = GetTaxTypeForPurposeAndWealth(purpose, wealth);
    if (taxType < 0 || taxType >= 12) {
        return 0.0f;
    }
    return mBudgetSaveRecord.GetTaxRate(taxType);
}

bool cSC4City::IssueBond(Sint32 amount) noexcept {
    if (!mHasBudgetSaveRecord) {
        mHasBudgetSaveRecord = true;
        mBudgetSaveRecord.SetTotalFunds(mFunds);
    }
    const bool ok = mBudgetSaveRecord.IssueBond(
        amount, mBudgetTuning, static_cast<Sint32>(mMonth), static_cast<Sint32>(mYear));
    if (ok) {
        mFunds = mBudgetSaveRecord.totalFunds;
        if (mIsFromOriginalSave) {
            MarkSaveEntryDirty(mBudgetSaveTGI);
        }
    }
    return ok;
}

Sint64 cSC4City::GetTotalMonthlyBondPayments() const noexcept {
    if (!mHasBudgetSaveRecord) {
        return 0;
    }
    return mBudgetSaveRecord.GetTotalMonthlyBondPayments();
}

TileQueryReport cSC4City::QueryTile(int cx, int cz) const noexcept {
    TileQueryReport r{};
    r.cellX = std::clamp(cx, 0, std::max(0, mCellCountX - 1));
    r.cellZ = std::clamp(cz, 0, std::max(0, mCellCountZ - 1));
    r.elevationY = GetCellCenterHeight(r.cellX, r.cellZ);
    r.underwater = IsCellUnderwater(r.cellX, r.cellZ);
    r.zoneType = GetCellZone(r.cellX, r.cellZ);
    r.hasNetwork = HasCellNetwork(r.cellX, r.cellZ, &r.networkType);
    r.landValue = mLandValueGrid.GetCellValue(r.cellX, r.cellZ);
    r.pollution = mPollutionGrid.GetCellValue(r.cellX, r.cellZ);
    r.crime = mCrimeGrid.GetCellValue(r.cellX, r.cellZ);
    r.traffic = mTrafficGrid.GetCellValue(r.cellX, r.cellZ);
    r.power = mPowerGrid.GetCellValue(r.cellX, r.cellZ);
    r.water = mWaterGrid.GetCellValue(r.cellX, r.cellZ);
    r.mayorRating = mMayorRatingGrid.GetCellValue(r.cellX, r.cellZ);

    for (const auto& lot : mLots) {
        if (r.cellX >= lot.minX && r.cellX <= lot.maxX &&
            r.cellZ >= lot.minZ && r.cellZ <= lot.maxZ) {
            r.lotCapacity = lot.capacity;
            break;
        }
    }
    for (const auto& bld : mBuildings) {
        const int bx0 = static_cast<int>(std::floor(bld.minX / 16.0f));
        const int bz0 = static_cast<int>(std::floor(bld.minZ / 16.0f));
        const int bx1 = static_cast<int>(std::floor((bld.maxX - 0.1f) / 16.0f));
        const int bz1 = static_cast<int>(std::floor((bld.maxZ - 0.1f) / 16.0f));
        if (r.cellX >= bx0 && r.cellX <= bx1 && r.cellZ >= bz0 && r.cellZ <= bz1) {
            r.hasBuilding = true;
            r.buildingName = bld.name;
            r.buildingWealth = bld.wealthHint;
            break;
        }
    }
    return r;
}

void cSC4City::StepLivePowerAndWaterBFS() {
    // cSC4PowerSimulator::UpdatePowerGrid @ 0x100213478 &
    // cSC4PlumbingSimulator::FillGridAtOccupant @ 0x1000a00ca:
    // power flood-fills from power plants across zoned/building/powerline
    // cells within the neighborhood relay radius; water covers the area
    // around pumps and pipes.
    if (mCellCountX <= 0 || mCellCountZ <= 0) {
        mPoweredCellCount = 0;
        mWateredCellCount = 0;
        return;
    }
    const std::size_t totalCells =
        static_cast<std::size_t>(mCellCountX) * static_cast<std::size_t>(mCellCountZ);
    std::vector<Uint8> conductive(totalCells, 0u);
    std::vector<Uint8> powered(totalCells, 0u);
    std::vector<Uint8> watered(totalCells, 0u);
    std::vector<std::pair<int, int>> powerQueue;

    const cSC4SimGrid<Sint8>* savedPower = nullptr;
    const cSC4SimGrid<Sint8>* savedWater = nullptr;
    for (const auto& g : mLoadedSint8Grids) {
        if (g.GetInstanceID() == kSimGridInst_Power) savedPower = &g;
        if (g.GetInstanceID() == kSimGridInst_Water) savedWater = &g;
    }

    for (int z = 0; z < mCellCountZ; ++z) {
        for (int x = 0; x < mCellCountX; ++x) {
            const std::size_t idx = static_cast<std::size_t>(z) * mCellCountX + x;
            NetworkType nt = NetworkType::Road;
            const bool hasNet = HasCellNetwork(x, z, &nt);
            // Shipped saves power network tiles too (the Big City Tutorial's
            // saved power grid covers 99% of its streets).
            if (GetCellZone(x, z) != ZoneType::None || hasNet) {
                conductive[idx] = 1u;
            }
            // Pipe networks are not decoded from saves yet, so the saved water
            // grid stands in for them.
            if ((hasNet && nt == NetworkType::Pipe) ||
                (savedWater != nullptr && savedWater->GetCellValue(x, z) > 0)) {
                watered[idx] = 1u;
            }
        }
    }

    bool hasPowerPlant = false;
    const int wRad = 6;
    for (const auto& bld : mBuildings) {
        const int bx0 = std::clamp(static_cast<int>(std::floor(bld.minX / 16.0f)), 0, mCellCountX - 1);
        const int bz0 = std::clamp(static_cast<int>(std::floor(bld.minZ / 16.0f)), 0, mCellCountZ - 1);
        const int bx1 = std::clamp(static_cast<int>(std::floor((bld.maxX - 0.1f) / 16.0f)), bx0, mCellCountX - 1);
        const int bz1 = std::clamp(static_cast<int>(std::floor((bld.maxZ - 0.1f) / 16.0f)), bz0, mCellCountZ - 1);
        const bool isPlant = bld.name.find("Power") != std::string::npos;
        hasPowerPlant = hasPowerPlant || isPlant;
        for (int z = bz0; z <= bz1; ++z) {
            for (int x = bx0; x <= bx1; ++x) {
                const std::size_t idx = static_cast<std::size_t>(z) * mCellCountX + x;
                conductive[idx] = 1u;
                if (isPlant && !powered[idx]) {
                    powered[idx] = 1u;
                    powerQueue.emplace_back(x, z);
                }
            }
        }
        if (bld.name.find("Water") != std::string::npos) {
            const int cx = (bx0 + bx1) / 2;
            const int cz = (bz0 + bz1) / 2;
            for (int dz = -wRad; dz <= wRad; ++dz) {
                for (int dx = -wRad; dx <= wRad; ++dx) {
                    const int nx = cx + dx;
                    const int nz = cz + dz;
                    if (nx >= 0 && nx < mCellCountX && nz >= 0 && nz < mCellCountZ) {
                        watered[static_cast<std::size_t>(nz) * mCellCountX + nx] = 1u;
                    }
                }
            }
        }
    }

    // Saves powered by a neighbor deal have no plant of their own; keep the
    // power the save recorded.
    if (!hasPowerPlant && savedPower != nullptr) {
        for (int z = 0; z < mCellCountZ; ++z) {
            for (int x = 0; x < mCellCountX; ++x) {
                const std::size_t idx = static_cast<std::size_t>(z) * mCellCountX + x;
                if (savedPower->GetCellValue(x, z) > 0 && !powered[idx]) {
                    powered[idx] = 1u;
                    powerQueue.emplace_back(x, z);
                }
            }
        }
    }

    const int pRad = 2;
    std::size_t head = 0;
    while (head < powerQueue.size()) {
        const auto [cx, cz] = powerQueue[head++];
        // cSC4PowerSimulator::GetNeighborhoodOfPoint @ 0x100212c4c uses a 2-cell relay radius
        for (int dz = -pRad; dz <= pRad; ++dz) {
            for (int dx = -pRad; dx <= pRad; ++dx) {
                const int nx = cx + dx;
                const int nz = cz + dz;
                if (nx < 0 || nx >= mCellCountX || nz < 0 || nz >= mCellCountZ) continue;
                const std::size_t nIdx = static_cast<std::size_t>(nz) * mCellCountX + nx;
                if (!powered[nIdx] && conductive[nIdx]) {
                    powered[nIdx] = 1u;
                    powerQueue.emplace_back(nx, nz);
                }
            }
        }
    }

    Uint32 powCount = 0;
    Uint32 watCount = 0;
    for (std::size_t i = 0; i < totalCells; ++i) {
        if (powered[i]) ++powCount;
        if (watered[i]) ++watCount;
    }
    mPoweredCellCount = powCount;
    mWateredCellCount = watCount;

    const int tractSize = 4;
    for (int tz = 0; tz < mPowerGrid.GetTractCountZ(); ++tz) {
        for (int tx = 0; tx < mPowerGrid.GetTractCountX(); ++tx) {
            bool tractPowered = false;
            bool tractWatered = false;
            for (int z = tz * tractSize; z < (tz + 1) * tractSize && z < mCellCountZ; ++z) {
                for (int x = tx * tractSize; x < (tx + 1) * tractSize && x < mCellCountX; ++x) {
                    const std::size_t idx = static_cast<std::size_t>(z) * mCellCountX + x;
                    tractPowered = tractPowered || powered[idx] != 0u;
                    tractWatered = tractWatered || watered[idx] != 0u;
                }
            }
            mPowerGrid.SetTractValue(tx, tz, tractPowered ? 220 : 0);
            mWaterGrid.SetTractValue(tx, tz, tractWatered ? 220 : 0);
        }
    }

    for (auto& lot : mLots) {
        const int lx = std::clamp<int>(lot.minX, 0, mCellCountX - 1);
        const int lz = std::clamp<int>(lot.minZ, 0, mCellCountZ - 1);
        const std::size_t idx = static_cast<std::size_t>(lz) * mCellCountX + lx;
        if (powered[idx]) lot.flags |= 0x10u;
        if (watered[idx]) lot.flags |= 0x40u;
    }
}

std::size_t cSC4City::RecomputeCommuteRoutes(std::size_t maxRoutes) {
    // Uses cSC4PathFinderEngine::FindPath (@ 0x1000d2644) to route commute trips
    // from developed Residential lots to Commercial/Industrial lots over the road network.
    mActiveCommuteRoutes.clear();
    if (mCellCountX <= 0 || mCellCountZ <= 0 || mNetworks.empty() || maxRoutes == 0u) {
        return 0u;
    }

    const std::size_t totalCells =
        static_cast<std::size_t>(mCellCountX) * static_cast<std::size_t>(mCellCountZ);
    std::vector<Float32> travelCost(totalCells, 0.0f);
    std::vector<Uint8> travelType(totalCells, 0u);

    for (const auto& net : mNetworks) {
        if (net.tileX >= mCellCountX || net.tileZ >= mCellCountZ) continue;
        const std::size_t idx =
            static_cast<std::size_t>(net.tileZ) * mCellCountX + net.tileX;
        Float32 cost = 1.0f;
        Uint8 tType = 1u; // Car / Road
        switch (net.networkType) {
            case NetworkType::Highway: cost = 0.45f; tType = 1u; break;
            case NetworkType::Avenue:  cost = 0.65f; tType = 1u; break;
            case NetworkType::Road:    cost = 1.00f; tType = 1u; break;
            case NetworkType::Street:  cost = 1.35f; tType = 1u; break;
            case NetworkType::Rail:    cost = 0.55f; tType = 3u; break;
            default:                   cost = 1.00f; tType = 1u; break;
        }
        travelCost[idx] = cost;
        travelType[idx] = tType;
    }

    std::vector<std::size_t> resLots;
    std::vector<std::size_t> jobLots;
    for (std::size_t i = 0; i < mLots.size(); ++i) {
        const auto& lot = mLots[i];
        if (!LotHasBuilding(lot) && !mIsFromOriginalSave) continue;
        if (lot.zoneType >= ZoneType::ResidentialLow &&
            lot.zoneType <= ZoneType::ResidentialHigh) {
            resLots.push_back(i);
        } else if (lot.zoneType >= ZoneType::CommercialLow &&
                   lot.zoneType <= ZoneType::IndustrialHigh) {
            jobLots.push_back(i);
        }
    }

    auto findAdjacentNetworkCell = [&](const SC4LotRecord& lot, Sint32& outX, Sint32& outZ) -> bool {
        const int x0 = std::max(0, static_cast<int>(lot.minX) - 1);
        const int x1 = std::min(mCellCountX - 1, static_cast<int>(lot.maxX) + 1);
        const int z0 = std::max(0, static_cast<int>(lot.minZ) - 1);
        const int z1 = std::min(mCellCountZ - 1, static_cast<int>(lot.maxZ) + 1);
        for (int z = z0; z <= z1; ++z) {
            for (int x = x0; x <= x1; ++x) {
                if (travelCost[static_cast<std::size_t>(z) * mCellCountX + x] > 0.0f) {
                    outX = x;
                    outZ = z;
                    return true;
                }
            }
        }
        return false;
    };

    const std::size_t resStep = std::max<std::size_t>(1u, resLots.size() / maxRoutes);
    for (std::size_t r = 0; r < resLots.size() && mActiveCommuteRoutes.size() < maxRoutes; r += resStep) {
        const std::size_t resIdx = resLots[r];
        const auto& resLot = mLots[resIdx];
        Sint32 sx = 0, sz = 0;
        if (!findAdjacentNetworkCell(resLot, sx, sz)) continue;

        // Pick closest job lot that also touches the road network
        Sint32 bestJobIdx = -1;
        Sint32 bestEx = 0, bestEz = 0;
        Sint32 bestDist = 0x7fffffff;
        const std::size_t jobStep = std::max<std::size_t>(1u, jobLots.size() / 32u);
        for (std::size_t j = 0; j < jobLots.size(); j += jobStep) {
            const auto& jLot = mLots[jobLots[j]];
            Sint32 ex = 0, ez = 0;
            if (!findAdjacentNetworkCell(jLot, ex, ez)) continue;
            const Sint32 dist = std::abs(ex - sx) + std::abs(ez - sz);
            if (dist > 0 && dist < bestDist) {
                bestDist = dist;
                bestJobIdx = static_cast<Sint32>(jobLots[j]);
                bestEx = ex;
                bestEz = ez;
            }
        }
        if (bestJobIdx < 0) continue;

        SC4PathFindResult pf{};
        const SC4Rect<Sint32> startRect{sx, sz, sx, sz};
        const SC4Rect<Sint32> endRect{bestEx, bestEz, bestEx, bestEz};
        if (cSC4PathFinderEngine::FindPath(
                startRect, endRect, mCellCountX, mCellCountZ,
                travelCost, travelType, 180.0f, pf) &&
            pf.found) {
            SC4CommuteRoute route{};
            route.startLotIndex = static_cast<Sint32>(resIdx);
            route.destLotIndex = bestJobIdx;
            route.commuters = std::max<Uint32>(10u, resLot.capacity);
            route.tripLength = pf.tripLength;
            route.cellX = std::move(pf.cellX);
            route.cellZ = std::move(pf.cellZ);
            route.compressedRoute = std::move(pf.compressedRoute);

            for (std::size_t k = 0; k < route.cellX.size(); ++k) {
                const int tx = route.cellX[k] / 4;
                const int tz = route.cellZ[k] / 4;
                const Uint8 cur = mTrafficGrid.GetTractValue(tx, tz);
                mTrafficGrid.SetTractValue(
                    tx, tz, static_cast<Uint8>(std::min<int>(255, cur + 8)));
            }
            mActiveCommuteRoutes.push_back(std::move(route));
        }
    }
    return mActiveCommuteRoutes.size();
}

void cSC4City::SetOrdinanceOn(Uint32 ordinanceId, bool enabled) {
    mActiveOrdinances[ordinanceId] = enabled;
}

bool cSC4City::IsOrdinanceOn(Uint32 ordinanceId) const noexcept {
    auto it = mActiveOrdinances.find(ordinanceId);
    return it != mActiveOrdinances.end() && it->second;
}

bool cSC4City::TriggerDisaster(Uint32 disasterType, int cellX, int cellZ) {
    if (cellX < 0 || cellX >= mCellCountX || cellZ < 0 || cellZ >= mCellCountZ) {
        return false;
    }
    ++mActiveDisasterCount;
    // DisasterType: 1=Fire, 2=Tornado, 3=Volcano/Meteor (raises terrain & pollution)
    const int tx = cellX / 4;
    const int tz = cellZ / 4;
    mPollutionGrid.SetTractValue(
        tx, tz, static_cast<Uint8>(std::min<int>(255, mPollutionGrid.GetTractValue(tx, tz) + 80)));
    if (disasterType == 3u) {
        SetVertexHeight(cellX, cellZ, GetVertexHeight(cellX, cellZ) + 18.0f);
    }
    return true;
}

} // namespace opensc4
