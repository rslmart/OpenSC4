#pragma once

// Direct clean-room C++20 implementation of Maxis's SC4SimGrid.cpp.inl
// Preserves the exact binary serialization layout (kSC4SimGridLoadSaveVersion = 1)
// and tract math from Contents/Resources/SC4SimGrid.cpp.inl.

#include "opensc4/gzcom.h"
#include <cassert>
#include <cmath>
#include <vector>

namespace opensc4 {

// SimGrid Class IDs verified from Sim City 4 Deluxe Editionsub & .sc4 subfiles
constexpr Uint16 kSC4SimGridLoadSaveVersion = 1;

// Exact SimGrid Instance IDs disassembled from cSC4*Simulator::Init() in Sim City 4 Deluxe Editionsub
constexpr Uint32 kSimGridInst_AuraMayorRating   = 0x69d5b600u; // cSC4AuraSimulator::Init()+0xfc (Uint8, tractSize=4)
constexpr Uint32 kSimGridInst_Power             = 0x49d5bc86u; // cSC4PowerSimulator::Init()+0x4dd (Sint8, tractSize=1)
constexpr Uint32 kSimGridInst_Water             = 0x49d5bba1u; // cSC4PlumbingSimulator::Init()+0x432 (Sint8, tractSize=1)
constexpr Uint32 kSimGridInst_WaterPollution    = 0x49d5bb8cu; // cSC4PlumbingSimulator::Init()+0x3a3 (Sint8, tractSize=1)
constexpr Uint32 kSimGridInst_FireCoverage      = 0x49d5b8f9u; // cSC4FireProtectionSimulator::Init()+0x115 (Sint8, tractSize=2)
constexpr Uint32 kSimGridInst_Flammability      = 0x49d5b953u; // cSC4FlammabilitySimulator::Init() (Sint8, tractSize=1)
constexpr Uint32 kSimGridInst_PoliceCoverage    = 0x49d5bbb9u; // cSC4PoliceSimulator::Init()+0x1d6 (Sint8, tractSize=2)
constexpr Uint32 kSimGridInst_Crime             = 0x89d5b895u; // cSC4CrimeSimulator::Init()+0x243 (Sint8, tractSize=2)
constexpr Uint32 kSimGridInst_LandValue         = 0x89d5bb39u; // cSC4LandValueSimulator::Init() (Sint8, tractSize=2)
constexpr Uint32 kSimGridInst_LandValueDesire   = 0x49d5bb6fu; // cSC4LandValueSimulator::Init() (Float32, tractSize=8)
constexpr Uint32 kSimGridInst_AirPollution      = 0x89d5bbf0u; // cSC4PollutionSimulator::Init() (Uint16, tractSize=2)
constexpr Uint32 kSimGridInst_TrafficCongestion = 0x89d5c3a2u; // cSC4TrafficSimulator::Init() (Sint8, tractSize=1)

template <typename T>
constexpr GZCLSID DefaultSimGridClassID() noexcept { return 0; }
template <> constexpr GZCLSID DefaultSimGridClassID<Uint8>() noexcept   { return kGZCLSID_cSC4SimGridUint8; }
template <> constexpr GZCLSID DefaultSimGridClassID<Sint8>() noexcept   { return kGZCLSID_cSC4SimGridSint8; }
template <> constexpr GZCLSID DefaultSimGridClassID<Uint16>() noexcept  { return kGZCLSID_cSC4SimGridUint16; }
template <> constexpr GZCLSID DefaultSimGridClassID<Sint16>() noexcept  { return kGZCLSID_cSC4SimGridSint16; }
template <> constexpr GZCLSID DefaultSimGridClassID<Uint32>() noexcept  { return kGZCLSID_cSC4SimGridUint32; }
template <> constexpr GZCLSID DefaultSimGridClassID<Sint32>() noexcept  { return kGZCLSID_cSC4SimGridSint32; }
template <> constexpr GZCLSID DefaultSimGridClassID<Uint64>() noexcept  { return kGZCLSID_cSC4SimGridUint64; }
template <> constexpr GZCLSID DefaultSimGridClassID<Sint64>() noexcept  { return kGZCLSID_cSC4SimGridSint64; }
template <> constexpr GZCLSID DefaultSimGridClassID<Float32>() noexcept { return kGZCLSID_cSC4SimGridFloat32; }
template <> constexpr GZCLSID DefaultSimGridClassID<Float64>() noexcept { return kGZCLSID_cSC4SimGridFloat64; }

template <class T>
class cISC4SimGrid : public cIGZSerializable {
public:
    using GridData = std::vector<std::vector<T>>;

    virtual bool Init() = 0;
    virtual bool Shutdown() = 0;
    virtual Uint32 GetInstanceID() const = 0;
    virtual void SetInstanceID(Uint32 nInstanceID) = 0;
    virtual void SetTractSize(int nTractSize, int cityCellCountX = 128, int cityCellCountZ = 128, float cellWidthX = 16.0f, float cellWidthZ = 16.0f) = 0;
    virtual T GetCellValue(int nCityX, int nCityZ) const = 0;
    virtual void SetCellValue(int nCityX, int nCityZ, T val) = 0;
    virtual T GetAverageValueInCellRect(int nCityX, int nCityZ, int nSizeX, int nSizeZ) const = 0;
    virtual T GetAverageValueInTractRect(int nTractX, int nTractZ, int nSizeX, int nSizeZ) const = 0;
    virtual int GetTractShift() const = 0;
    virtual int GetTractSize() const = 0;
    virtual int GetTractCountX() const = 0;
    virtual int GetTractCountZ() const = 0;
    virtual GridData& GetGridData() = 0;
    virtual const GridData& GetGridData() const = 0;
};

template <class T>
class cSC4SimGrid : public cISC4SimGrid<T> {
public:
    using GridData = typename cISC4SimGrid<T>::GridData;

    explicit cSC4SimGrid(Uint32 nClassID = DefaultSimGridClassID<T>())
        : mbInitialized(false),
          mnRefCount(0),
          mnClassID(nClassID),
          mnInstanceID(0),
          mnTractSize(0),
          mnTractShift(0),
          mnTractCountX(0),
          mnTractCountZ(0),
          mfTractWidthX(16.0f),
          mfTractWidthZ(16.0f),
          mfOneOverTractWidthX(1.0f / 16.0f),
          mfOneOverTractWidthZ(1.0f / 16.0f) {}

    ~cSC4SimGrid() override = default;

    bool QueryInterface(GZREFIID riid, void** ppvObj) override {
        switch (riid) {
            case GZIID_cIGZUnknown:
                *ppvObj = static_cast<cIGZUnknown*>(static_cast<cISC4SimGrid<T>*>(this));
                AddRef();
                return true;
            case kGZIID_cISC4SimGrid:
                *ppvObj = static_cast<cISC4SimGrid<T>*>(this);
                AddRef();
                return true;
            case kGZIID_cIGZSerializable:
                *ppvObj = static_cast<cIGZSerializable*>(this);
                AddRef();
                return true;
        }
        return false;
    }

    Uint32 AddRef() override { return ++mnRefCount; }

    Uint32 Release() override {
        if (mnRefCount > 1) return --mnRefCount;
        Shutdown();
        delete this;
        return 0;
    }

    bool Init() override {
        if (!mbInitialized) mbInitialized = true;
        return true;
    }

    bool Shutdown() override {
        if (mbInitialized) mbInitialized = false;
        return true;
    }

    Uint32 GetClassID() const noexcept { return mnClassID; }
    Uint32 GetInstanceID() const override { return mnInstanceID; }
    void SetInstanceID(Uint32 nInstanceID) override { mnInstanceID = nInstanceID; }

    int GetTractShift() const override { return mnTractShift; }
    int GetTractSize() const override { return mnTractSize; }
    int GetTractCountX() const override { return mnTractCountX; }
    int GetTractCountZ() const override { return mnTractCountZ; }

    GridData& GetGridData() override { return mGrid; }
    const GridData& GetGridData() const override { return mGrid; }

    void SetTractSize(int nTractSize, int cityCellCountX = 128, int cityCellCountZ = 128, float cellWidthX = 16.0f, float cellWidthZ = 16.0f) override {
        mbInitialized = true;
        if (nTractSize != mnTractSize || mnTractCountX == 0) {
            mnTractSize = nTractSize;
            mnTractShift = 0;
            int tmp = nTractSize;
            while (tmp > 1) {
                tmp /= 2;
                ++mnTractShift;
            }
            mnTractCountX        = cityCellCountX >> mnTractShift;
            mnTractCountZ        = cityCellCountZ >> mnTractShift;
            mfTractWidthX        = cellWidthX * static_cast<float>(mnTractSize);
            mfTractWidthZ        = cellWidthZ * static_cast<float>(mnTractSize);
            mfOneOverTractWidthX = 1.0f / mfTractWidthX;
            mfOneOverTractWidthZ = 1.0f / mfTractWidthZ;

            mGrid.assign(mnTractCountX, std::vector<T>(mnTractCountZ, static_cast<T>(0)));
        }
    }

    T GetCellValue(int nCityX, int nCityZ) const override {
        const int nTractX = nCityX >> mnTractShift;
        const int nTractZ = nCityZ >> mnTractShift;
        if (nTractX < 0 || nTractX >= mnTractCountX || nTractZ < 0 || nTractZ >= mnTractCountZ) {
            return static_cast<T>(0);
        }
        return mGrid[nTractX][nTractZ];
    }

    void SetCellValue(int nCityX, int nCityZ, T val) override {
        const int nTractX = nCityX >> mnTractShift;
        const int nTractZ = nCityZ >> mnTractShift;
        if (nTractX >= 0 && nTractX < mnTractCountX && nTractZ >= 0 && nTractZ < mnTractCountZ) {
            mGrid[nTractX][nTractZ] = val;
        }
    }

    T GetTractValue(int nTractX, int nTractZ) const noexcept {
        if (nTractX < 0 || nTractX >= mnTractCountX || nTractZ < 0 || nTractZ >= mnTractCountZ) {
            return static_cast<T>(0);
        }
        return mGrid[nTractX][nTractZ];
    }

    void SetTractValue(int nTractX, int nTractZ, T val) noexcept {
        if (nTractX >= 0 && nTractX < mnTractCountX && nTractZ >= 0 && nTractZ < mnTractCountZ) {
            mGrid[nTractX][nTractZ] = val;
        }
    }

    T GetAverageValueInCellRect(int nCityX, int nCityZ, int nSizeX, int nSizeZ) const override {
        if (nSizeX <= 0 || nSizeZ <= 0) return static_cast<T>(0);
        const int endX = nCityX + nSizeX;
        const int endZ = nCityZ + nSizeZ;

        if ((nCityX >> mnTractShift) == ((endX - 1) >> mnTractShift) &&
            (nCityZ >> mnTractShift) == ((endZ - 1) >> mnTractShift)) {
            return GetCellValue(nCityX, nCityZ);
        }

        double sum = 0.0;
        for (int i = nCityX; i < endX; ++i) {
            for (int j = nCityZ; j < endZ; ++j) {
                sum += static_cast<double>(GetCellValue(i, j));
            }
        }
        return static_cast<T>(sum / (nSizeX * nSizeZ));
    }

    T GetAverageValueInTractRect(int nTractX, int nTractZ, int nSizeX, int nSizeZ) const override {
        if (nSizeX <= 0 || nSizeZ <= 0) return static_cast<T>(0);
        const int endX = nTractX + nSizeX;
        const int endZ = nTractZ + nSizeZ;
        double sum = 0.0;
        for (int i = nTractX; i < endX; ++i) {
            for (int j = nTractZ; j < endZ; ++j) {
                sum += static_cast<double>(GetTractValue(i, j));
            }
        }
        return static_cast<T>(sum / (nSizeX * nSizeZ));
    }

    void PositionToTract(float fPositionX, float fPositionZ, int& nTractX, int& nTractZ) const noexcept {
        nTractX = static_cast<int>(fPositionX * mfOneOverTractWidthX);
        nTractZ = static_cast<int>(fPositionZ * mfOneOverTractWidthZ);
    }

    void TractCornerToPosition(int nTractX, int nTractZ, float& fPositionX, float& fPositionZ) const noexcept {
        fPositionX = nTractX * mfTractWidthX;
        fPositionZ = nTractZ * mfTractWidthZ;
    }

    void TractCenterToPosition(int nTractX, int nTractZ, float& fPositionX, float& fPositionZ) const noexcept {
        fPositionX = (nTractX + 0.5f) * mfTractWidthX;
        fPositionZ = (nTractZ + 0.5f) * mfTractWidthZ;
    }

    void SetTractValues(T newValue) {
        for (int i = 0; i < mnTractCountX; ++i) {
            for (int j = 0; j < mnTractCountZ; ++j) {
                mGrid[i][j] = newValue;
            }
        }
    }

    // Exact binary layout from SC4SimGrid.cpp.inl lines 415-445
    bool Write(cIGZOStream& oStream) const override {
        oStream << kSC4SimGridLoadSaveVersion;
        oStream << mbInitialized;
        oStream << mnClassID;
        oStream << mnInstanceID;
        oStream << mnTractSize;
        oStream << mnTractShift;
        oStream << mnTractCountX;
        oStream << mnTractCountZ;
        oStream << mfTractWidthX;
        oStream << mfTractWidthZ;
        oStream << mfOneOverTractWidthX;
        oStream << mfOneOverTractWidthZ;

        for (int x = 0; x < mnTractCountX; ++x) {
            for (int z = 0; z < mnTractCountZ; ++z) {
                oStream << mGrid[x][z];
            }
        }
        return true;
    }

    // Exact binary layout from SC4SimGrid.cpp.inl lines 450-480
    bool Read(cIGZIStream& iStream) override {
        Uint16 nVersion = 0;
        iStream >> nVersion;
        if (!iStream.Good() || nVersion != kSC4SimGridLoadSaveVersion) {
            return false;
        }

        iStream >> mbInitialized;
        iStream >> mnClassID;
        iStream >> mnInstanceID;
        iStream >> mnTractSize;
        iStream >> mnTractShift;
        iStream >> mnTractCountX;
        iStream >> mnTractCountZ;
        iStream >> mfTractWidthX;
        iStream >> mfTractWidthZ;
        iStream >> mfOneOverTractWidthX;
        iStream >> mfOneOverTractWidthZ;

        if (!iStream.Good() || mnTractCountX <= 0 || mnTractCountZ <= 0 ||
            mnTractCountX > 1024 || mnTractCountZ > 1024) {
            return false;
        }

        mGrid.resize(mnTractCountX);
        for (int x = 0; x < mnTractCountX; ++x) {
            mGrid[x].resize(mnTractCountZ, static_cast<T>(0));
            for (int z = 0; z < mnTractCountZ; ++z) {
                iStream >> mGrid[x][z];
            }
        }
        return iStream.Good();
    }

private:
    bool mbInitialized;
    Uint32 mnRefCount;
    Uint32 mnClassID;
    Uint32 mnInstanceID;
    Sint32 mnTractSize;
    Sint32 mnTractShift;
    Sint32 mnTractCountX;
    Sint32 mnTractCountZ;
    Float32 mfTractWidthX;
    Float32 mfTractWidthZ;
    Float32 mfOneOverTractWidthX;
    Float32 mfOneOverTractWidthZ;
    GridData mGrid;
};

// Direct port of ProcessTwoGridsWithCallback from SC4SimGrid.cpp.inl lines 499-578
template <class T, class T2>
inline void ProcessTwoGridsWithCallback(
    cISC4SimGrid<T>* pMyGrid,
    const cISC4SimGrid<T2>* pOtherGrid,
    void (*function)(T& myValue, T2 otherValue, void* context),
    void* context = nullptr) {
    int nMyTractShift = pMyGrid->GetTractShift();
    int nMyTractCountX = pMyGrid->GetTractCountX();
    int nMyTractCountZ = pMyGrid->GetTractCountZ();
    typename cISC4SimGrid<T>::GridData& pMyGridData = pMyGrid->GetGridData();

    int nOtherTractShift = pOtherGrid->GetTractShift();
    int nOtherTractCountX = pOtherGrid->GetTractCountX();
    int nOtherTractCountZ = pOtherGrid->GetTractCountZ();
    const typename cISC4SimGrid<T2>::GridData& pOtherGridData = pOtherGrid->GetGridData();

    if (nMyTractShift == nOtherTractShift) {
        for (int i = 0; i < nMyTractCountX; ++i) {
            for (int j = 0; j < nMyTractCountZ; ++j) {
                function(pMyGridData[i][j], pOtherGridData[i][j], context);
            }
        }
    } else if (nMyTractShift > nOtherTractShift) {
        int nShift = nMyTractShift - nOtherTractShift;
        for (int i = 0; i < nMyTractCountX; ++i) {
            for (int j = 0; j < nMyTractCountZ; ++j) {
                int average = 0;
                int nStartX = i << nShift;
                int nEndX = (i + 1) << nShift;
                int nStartZ = j << nShift;
                int nEndZ = (j + 1) << nShift;
                for (int x = nStartX; x < nEndX; ++x) {
                    for (int z = nStartZ; z < nEndZ; ++z) {
                        average += static_cast<int>(pOtherGridData[x][z]);
                    }
                }
                average >>= nShift;
                average >>= nShift;
                function(pMyGridData[i][j], static_cast<T2>(average), context);
            }
        }
    } else {
        int nShift = nOtherTractShift - nMyTractShift;
        for (int x = 0; x < nOtherTractCountX; ++x) {
            for (int z = 0; z < nOtherTractCountZ; ++z) {
                T2 nOtherValue = pOtherGridData[x][z];
                int nStartI = x << nShift;
                int nEndI = (x + 1) << nShift;
                int nStartJ = z << nShift;
                int nEndJ = (z + 1) << nShift;
                for (int i = nStartI; i < nEndI; ++i) {
                    for (int j = nStartJ; j < nEndJ; ++j) {
                        function(pMyGridData[i][j], nOtherValue, context);
                    }
                }
            }
        }
    }
}

// Parse all concatenated cSC4SimGrid<T> records from a .sc4 subfile buffer
// Each record has a 12-byte header: [uint32_t recordSize, uint32_t crc32, uint32_t memAddr]
// followed by cSC4SimGrid<T>::Read(cIGZIStream&)
template <typename T>
inline std::vector<cSC4SimGrid<T>> ParseSimGridSubfile(const std::vector<Uint8>& bytes) {
    std::vector<cSC4SimGrid<T>> grids;
    std::size_t pos = 0;
    while (pos + 12 <= bytes.size()) {
        Uint32 recSize = 0;
        std::memcpy(&recSize, bytes.data() + pos, 4);
        if (recSize < 55 || pos + recSize > bytes.size()) break;
        cIGZIStream stream(bytes.data() + pos + 12, recSize - 12);
        cSC4SimGrid<T> grid;
        if (grid.Read(stream)) {
            grids.push_back(std::move(grid));
        }
        pos += recSize;
    }
    return grids;
}

} // namespace opensc4
