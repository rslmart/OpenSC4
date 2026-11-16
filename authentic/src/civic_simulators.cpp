// Exact decompilations of the civic & utility simulator save records.
// See include/opensc4/civic_simulators.h for the provenance of every field.

#include "opensc4/civic_simulators.h"
#include "opensc4/tract_developer.h"
#include "opensc4/pollution_capacity_kernel.h"
#include <new>

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <random>
#include <type_traits>

namespace opensc4 {
namespace {

// SC4ReadGZSerializable: Uint32 memAddr; Uint32 clsid only if memAddr != 0.
bool ReadRef(cIGZIStream& stream, SimSerializableRef& ref) noexcept {
    stream >> ref.memAddr;
    if (!stream.Good()) {
        return false;
    }
    ref.clsid = 0;
    if (ref.memAddr != 0) {
        stream >> ref.clsid;
    }
    return stream.Good();
}

void WriteRef(cIGZOStream& stream, const SimSerializableRef& ref) {
    stream << ref.memAddr;
    if (ref.memAddr != 0) {
        stream << ref.clsid;
    }
}

// ilist<cISC4Occupant*>: the binary rejects counts > 0x98967f, then reads
// that many serializable refs (IID 0x26bb767e).
bool ReadRefList(cIGZIStream& stream, OccupantRefList& list) noexcept {
    Uint32 count = 0;
    stream >> count;
    if (!stream.Good() || count > 0x98967fu) {
        return false;
    }
    list.clear();
    list.reserve(count);
    for (Uint32 i = 0; i < count; ++i) {
        SimSerializableRef ref;
        if (!ReadRef(stream, ref)) {
            return false;
        }
        list.push_back(ref);
    }
    return true;
}

void WriteRefList(cIGZOStream& stream, const OccupantRefList& list) {
    stream << static_cast<Uint32>(list.size());
    for (const SimSerializableRef& ref : list) {
        WriteRef(stream, ref);
    }
}

// RandomIterator operator>> @ 0x1000cea88.
bool ReadRandomIterator(cIGZIStream& stream, RandomIteratorState& it) noexcept {
    stream >> it.field0 >> it.field1 >> it.field2;
    for (Sint32& v : it.values) {
        stream >> v;
    }
    Uint32 count = 0;
    stream >> count;
    if (!stream.Good()) {
        return false;
    }
    it.indices.assign(count, 0);
    for (Uint16& v : it.indices) {
        stream >> v;
    }
    for (Sint32& v : it.tailValues) {
        stream >> v;
    }
    return stream.Good();
}

void WriteRandomIterator(cIGZOStream& stream, const RandomIteratorState& it) {
    stream << it.field0 << it.field1 << it.field2;
    for (Sint32 v : it.values) {
        stream << v;
    }
    stream << static_cast<Uint32>(it.indices.size());
    for (Uint16 v : it.indices) {
        stream << v;
    }
    for (Sint32 v : it.tailValues) {
        stream << v;
    }
}

// ScanningIterator operator>> @ 0x1000cec88 (29 serialized bytes).
bool ReadScanningIterator(cIGZIStream& stream, ScanningIteratorState& it) noexcept {
    stream >> it.field0 >> it.field4 >> it.field8 >> it.fieldC >> it.field10 >> it.field14 >>
        it.field18 >> it.field1C;
    return stream.Good();
}

void WriteScanningIterator(cIGZOStream& stream, const ScanningIteratorState& it) {
    stream << it.field0 << it.field4 << it.field8 << it.fieldC << it.field10 << it.field14
           << it.field18 << it.field1C;
}

// Uint32 count, then count x (Sint32, Sint32, Sint32).
bool ReadTripleList(cIGZIStream& stream, std::vector<Sint32Triple>& list) noexcept {
    Uint32 count = 0;
    stream >> count;
    if (!stream.Good()) {
        return false;
    }
    list.clear();
    list.reserve(count);
    for (Uint32 i = 0; i < count; ++i) {
        Sint32Triple t;
        stream >> t.a >> t.b >> t.c;
        if (!stream.Good()) {
            return false;
        }
        list.push_back(t);
    }
    return true;
}

void WriteTripleList(cIGZOStream& stream, const std::vector<Sint32Triple>& list) {
    stream << static_cast<Uint32>(list.size());
    for (const Sint32Triple& t : list) {
        stream << t.a << t.b << t.c;
    }
}

// DBPF subfile wrapper: [recordSize:Uint32][flags:Uint32][memAddr:Uint32][chunk].
template <typename Record>
bool ReadRecordFromSC4(Record& rec, std::span<const Uint8> recordBytes) noexcept {
    if (recordBytes.size() < 14u) {
        return false;
    }
    Uint32 recSize = 0;
    std::memcpy(&recSize, recordBytes.data(), sizeof(Uint32));
    if (recSize < 14u || recSize > recordBytes.size()) {
        return false;
    }
    std::memcpy(&rec.memAddr, recordBytes.data() + 8u, sizeof(Uint32));
    cIGZIStream stream(recordBytes.data() + 12u, recSize - 12u);
    return rec.Read(stream);
}

bool LoadProps(const cGZExemplar& exemplar,
               std::unordered_map<Uint32, ExemplarProperty>& properties) {
    properties = exemplar.properties;
    return true;
}

// Exact table kPrimeNumbers @ 0x100a1b190, used by RandomIterator::Reset
// @ 0x100213f58 and LinearRandomIterator::Reset @ 0x10008daf0. Index 70 is
// the binary's trailing zero at 0x100a1b21c, followed by another zero at
// 0x100a1b21e and string data at 0x100a1b220. Keeping that padding in bounds
// preserves the read made after the divisible final entry at the clamp.
constexpr std::array<Sint16, 71> kPrimeNumbers{
    1, 3, 5, 7, 11, 13, 17, 19, 23, 29,
    31, 37, 41, 43, 47, 53, 59, 61, 67, 71,
    73, 79, 83, 89, 97, 101, 103, 107, 109, 113,
    179, 211, 257, 307, 373, 479, 577, 641, 719, 857,
    977, 1063, 1237, 1471, 1663, 1993, 2221, 2539, 2909, 3259,
    3733, 4507, 5281, 5861, 6481, 6947, 7573, 9013, 10103, 12227,
    15683, 17431, 19801, 21569, 23633, 26003, 27529, 29683, 32051, 32611,
    0,
};

Sint32 SelectDirectPrime(Sint32& count) {
    Uint32 primeIndex = 0;
    if (count >= 0x1d) {
        if (count < 0x47) {
            primeIndex = 10;
        } else if (count < 0x71) {
            primeIndex = 20;
        } else if (count < 0x359) {
            primeIndex = 30;
        } else if (count < 0xcbb) {
            primeIndex = 40;
        } else if (count < 0x2fc3) {
            primeIndex = 50;
        } else {
            if (count >= 0x1fd8c) {
                count = 0x1fd8c;
            }
            primeIndex = 60;
        }
    }

    while (kPrimeNumbers[primeIndex] < count / 4) {
        ++primeIndex;
    }

    Sint32 prime = 0;
    for (;;) {
        prime = kPrimeNumbers[primeIndex];
        const Sint32 remainder = count % prime;
        // Preserve the binary's unusual bound-check order: an entry at index
        // 0x45 which divides count advances to 0x46 before the next load.
        if (primeIndex > 0x45u) {
            break;
        }
        ++primeIndex;
        if (remainder != 0) {
            break;
        }
    }
    return prime;
}

// libc++ random_shuffle<Uint16*> @ 0x10009b36d and
// random_shuffle<Uint32*> @ 0x1000c4afc use a shared default-seeded
// __rs_default engine and a forward Fisher-Yates traversal.
std::mt19937& SharedRandomShuffleEngine() {
    static std::mt19937 engine{};
    return engine;
}

template <typename T>
void RandomShuffle(std::vector<T>& values) {
    if (values.size() < 2u) {
        return;
    }
    std::mt19937& engine = SharedRandomShuffleEngine();
    auto current = values.begin();
    for (std::ptrdiff_t distance = static_cast<std::ptrdiff_t>(values.size()) - 1;
         distance > 0; --distance, ++current) {
        std::uniform_int_distribution<std::ptrdiff_t> distribution(0, distance);
        const std::ptrdiff_t offset = distribution(engine);
        if (offset != 0) {
            std::swap(*current, *(current + offset));
        }
    }
}

} // namespace

// Exact RandomIterator::Reset(bool) @ 0x100213f58.
void RandomIteratorState::Reset(bool randomize) {
    values[4] = (values[2] + 1 - values[0]) * (values[3] + 1 - values[1]);
    field0 = 1;
    tailValues[0] = 0;

    if (field1 != 0) {
        tailValues[2] = SelectDirectPrime(values[4]);
        if (randomize) {
            tailValues[1] = std::rand() % values[4];
        }
        return;
    }

    if (indices.size() != static_cast<Uint32>(values[4])) {
        indices.resize(static_cast<std::size_t>(values[4]));
        field2 = true;
    }
    if (field2) {
        std::size_t index = 0;
        for (Sint32 x = values[0]; x <= values[2]; ++x) {
            const Uint32 shiftedX = static_cast<Uint32>(x) << 8u;
            for (Sint32 z = values[1]; z <= values[3]; ++z) {
                indices[index++] = static_cast<Uint16>(
                    shiftedX | (static_cast<Uint32>(z) & 0xffu));
            }
        }
    }
    if (randomize) {
        RandomShuffle(indices);
    }
}

// Exact LinearRandomIterator::Reset(bool) @ 0x10008daf0.
void LinearRandomIteratorState::Reset(bool randomize) {
    field0 = 1;
    tailValues[0] = 0;
    if (count == 0) {
        field0 = 2;
        tailValues[1] = 0;
        tailValues[2] = 0;
        field2 = true;
        return;
    }

    if (field1 != 0) {
        tailValues[2] = SelectDirectPrime(count);
        if (randomize) {
            tailValues[1] = std::rand() % count;
        }
        return;
    }

    if (indices.size() != static_cast<Uint32>(count)) {
        indices.resize(static_cast<std::size_t>(count));
        field2 = true;
    }
    if (field2) {
        for (Sint32 i = 0; i < count; ++i) {
            indices[static_cast<std::size_t>(i)] = static_cast<Uint32>(i);
        }
    }
    if (randomize) {
        RandomShuffle(indices);
    }
}

// ===========================================================================
// cSC4ProtectionSimulator base (Read 0x1000aa362 / Write 0x1000aa25a)
// ===========================================================================
bool ProtectionSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version > 3) {
        return false;
    }
    if (!ReadRef(stream, simManagerRef)) {
        return false;
    }
    if (!ReadRandomIterator(stream, randomIterator)) {
        return false;
    }
    if (!ReadRef(stream, gridRefA) || !ReadRef(stream, gridRefB)) {
        return false;
    }
    if (!ReadRefList(stream, occupants)) {
        return false;
    }
    stream >> fieldD0 >> fieldD4 >> fieldD8 >> fieldDC;
    if (version >= 3) {
        stream >> fieldE0;
    } else {
        Float32 raw = 0.0f;
        stream >> raw;
        fieldE0 = raw * 0.01f;
    }
    stream >> fieldE8 >> fieldEC >> fieldF0;
    return stream.Good();
}

bool ProtectionSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{3};
    WriteRef(stream, simManagerRef);
    WriteRandomIterator(stream, randomIterator);
    WriteRef(stream, gridRefA);
    WriteRef(stream, gridRefB);
    WriteRefList(stream, occupants);
    stream << fieldD0 << fieldD4 << fieldD8 << fieldDC << fieldE0 << fieldE8 << fieldEC
           << fieldF0;
    return true;
}

// ===========================================================================
// cSC4LandValueSimulator (Read 0x1000c5c1a / Write 0x1000c5b7a)
// ===========================================================================
bool LandValueSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version != 1) {
        return false;
    }
    if (!ReadRef(stream, gridRef40) || !ReadRef(stream, gridRef48) ||
        !ReadRef(stream, gridRef58)) {
        return false;
    }
    stream >> field60 >> fieldC8 >> fieldCC;
    return stream.Good();
}

bool LandValueSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{1};
    WriteRef(stream, gridRef40);
    WriteRef(stream, gridRef48);
    WriteRef(stream, gridRef58);
    stream << field60 << fieldC8 << fieldCC;
    return true;
}

bool LandValueSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

bool LandValueTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    fieldD0 = static_cast<Uint8>(exemplar.GetInt(0x47e2c200u, fieldD0));
    return LoadProps(exemplar, properties);
}

// ===========================================================================
// cSC4CommercialSimulator (Read 0x1001fc566 / Write 0x1001fc4f4)
// ===========================================================================
bool CommercialSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version != 1) {
        return false;
    }
    stream >> field38;
    if (!ReadRef(stream, gridRef58) || !ReadRef(stream, gridRef60)) {
        return false;
    }
    return stream.Good();
}

bool CommercialSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{1} << field38;
    WriteRef(stream, gridRef58);
    WriteRef(stream, gridRef60);
    return true;
}

bool CommercialSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

// ===========================================================================
// cSC4IndustrialSimulator (Read 0x100215646 / Write 0x1002155ee)
// ===========================================================================
bool IndustrialSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version != 1) {
        return false;
    }
    return stream.Good();
}

bool IndustrialSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{1};
    return true;
}

bool IndustrialSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

// ===========================================================================
// cSC4FireProtectionSimulator (Read 0x1002fae42 / Write 0x1002fadd2)
// ===========================================================================
bool FireProtectionSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version > 3) {
        return false;
    }
    if (!protection.Read(stream)) {
        return false;
    }
    if (version >= 3) {
        if (!ReadRef(stream, field120)) {
            return false;
        }
    }
    return stream.Good();
}

bool FireProtectionSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{3};
    protection.Write(stream);
    WriteRef(stream, field120);
    return true;
}

bool FireProtectionSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

bool FireProtectionTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    field14C = exemplar.GetFloat(0x29233e94u, field14C);
    return LoadProps(exemplar, properties);
}

// ===========================================================================
// cSC4PoliceSimulator (Read 0x1000a3204 / Write 0x1000a30de)
// ===========================================================================
bool PoliceSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version > 8) {
        return false;
    }
    if (!protection.Read(stream)) {
        return false;
    }
    if (!ReadRef(stream, field128)) {
        return false;
    }
    if (!ReadRefList(stream, list168) || !ReadRefList(stream, list130)) {
        return false;
    }
    stream >> field148 >> field14C >> field150 >> field154 >> field15C >> field164 >>
        field158 >> field160;
    if (!stream.Good()) {
        return false;
    }
    if (version >= 8) {
        if (!ReadRef(stream, field118)) {
            return false;
        }
    }
    return stream.Good();
}

bool PoliceSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{8};
    protection.Write(stream);
    WriteRef(stream, field128);
    WriteRefList(stream, list168);
    WriteRefList(stream, list130);
    stream << field148 << field14C << field150 << field154 << field15C << field164 <<
        field158 << field160;
    WriteRef(stream, field118);
    return true;
}

bool PoliceSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

bool PoliceTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    field1D4 = static_cast<Sint32>(exemplar.GetInt(0x68ddae9bu, field1D4));
    field200 = static_cast<Sint32>(exemplar.GetInt(0x68ddae9fu, field200));
    return LoadProps(exemplar, properties);
}

// ===========================================================================
// cSC4FlammabilitySimulator (Read 0x1000c36b8 / Write 0x1000c35e4)
// ===========================================================================
bool FlammabilitySimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version != 2) {
        return false;
    }
    if (!ReadRef(stream, gridRef80) || !ReadRef(stream, gridRef88)) {
        return false;
    }
    stream >> field94 >> field90;
    if (!stream.Good()) {
        return false;
    }
    return ReadTripleList(stream, entries);
}

bool FlammabilitySimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{2};
    WriteRef(stream, gridRef80);
    WriteRef(stream, gridRef88);
    stream << field94 << field90;
    WriteTripleList(stream, entries);
    return true;
}

bool FlammabilitySimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

bool FlammabilityTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    fieldD8 = exemplar.GetFloat(0x29b40134u, exemplar.GetFloat(0xc9b40128u, fieldD8));
    fieldF4 = exemplar.GetFloat(0x29b4013eu, fieldF4);
    return LoadProps(exemplar, properties);
}

// ===========================================================================
// cSC4CrimeSimulator (Read 0x1001fde02 / Write 0x1001fdc82)
// ===========================================================================
bool CrimeSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version != 6) {
        return false;
    }
    if (!ReadRef(stream, gridRef90) || !ReadRef(stream, gridRef98)) {
        return false;
    }
    if (!ReadRandomIterator(stream, randomIterator)) {
        return false;
    }
    stream >> fieldA0 >> fieldA4 >> fieldA8 >> fieldD8 >> fieldC8 >> fieldCC >> fieldD0 >>
        fieldD4;
    Sint32 count = 0;
    stream >> count;
    if (!stream.Good() || count < 0) {
        return false;
    }
    events.clear();
    events.reserve(static_cast<std::size_t>(count));
    for (Sint32 i = 0; i < count; ++i) {
        CrimeEventRecord ev;
        if (!ReadRef(stream, ev.occupantRef)) {
            return false;
        }
        stream >> ev.field08;
        if (!stream.ReadGZString(ev.name)) {
            return false;
        }
        stream >> ev.resourceKey.type >> ev.resourceKey.group >> ev.resourceKey.instance;
        if (!stream.Good()) {
            return false;
        }
        events.push_back(std::move(ev));
    }
    return stream.Good();
}

bool CrimeSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{6};
    WriteRef(stream, gridRef90);
    WriteRef(stream, gridRef98);
    WriteRandomIterator(stream, randomIterator);
    stream << fieldA0 << fieldA4 << fieldA8 << fieldD8 << fieldC8 << fieldCC << fieldD0
           << fieldD4;
    stream << static_cast<Sint32>(events.size());
    for (const CrimeEventRecord& ev : events) {
        WriteRef(stream, ev.occupantRef);
        stream << ev.field08;
        stream.WriteGZString(ev.name);
        stream << ev.resourceKey.type << ev.resourceKey.group << ev.resourceKey.instance;
    }
    return true;
}

bool CrimeSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

bool CrimeTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    field24C = static_cast<Sint32>(exemplar.GetInt(0x47e2c200u, field24C));
    return LoadProps(exemplar, properties);
}

// ===========================================================================
// cSC4AuraSimulator (Read 0x100219b70 / Write 0x10021997e)
// ===========================================================================
bool AuraSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version < 8) {
        return false;
    }
    if (!ReadRef(stream, gridRef58) || !ReadRef(stream, gridRef88) ||
        !ReadRef(stream, gridRef80) || !ReadRef(stream, gridRef68) ||
        !ReadRef(stream, gridRef70) || !ReadRef(stream, gridRef78) ||
        !ReadRef(stream, gridRefA8)) {
        return false;
    }
    hasLegacyRef = false;
    gridMap.clear();
    if (version <= 8) {
        hasLegacyRef = true;
        if (!ReadRef(stream, legacyRef)) {
            return false;
        }
    } else {
        Uint32 count = 0;
        stream >> count;
        if (!stream.Good()) {
            return false;
        }
        gridMap.reserve(count);
        for (Uint32 i = 0; i < count; ++i) {
            Uint32 key = 0;
            SimSerializableRef ref;
            stream >> key;
            if (!ReadRef(stream, ref)) {
                return false;
            }
            gridMap.emplace_back(key, ref);
        }
    }
    stream >> fieldD0 >> fieldD4 >> field1F0 >> field1F4 >> field1F8 >> field1FC >>
        field200 >> field204;
    if (!stream.Good()) {
        return false;
    }
    return ReadTripleList(stream, entries);
}

bool AuraSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{9};
    WriteRef(stream, gridRef58);
    WriteRef(stream, gridRef88);
    WriteRef(stream, gridRef80);
    WriteRef(stream, gridRef68);
    WriteRef(stream, gridRef70);
    WriteRef(stream, gridRef78);
    WriteRef(stream, gridRefA8);
    stream << static_cast<Uint32>(gridMap.size());
    for (const auto& [key, ref] : gridMap) {
        stream << key;
        WriteRef(stream, ref);
    }
    stream << fieldD0 << fieldD4 << field1F0 << field1F4 << field1F8 << field1FC << field200
           << field204;
    WriteTripleList(stream, entries);
    return true;
}

bool AuraSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

bool AuraTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    field1A8 = exemplar.GetFloat(0xea02327au, field1A8);
    field1AC = exemplar.GetFloat(0xea02327bu, field1AC);
    field1B0 = static_cast<Sint32>(exemplar.GetInt(0xea02327cu, field1B0));
    field1B4 = static_cast<Sint32>(exemplar.GetInt(0xea02327du, field1B4));
    field1B8 = static_cast<Sint32>(exemplar.GetInt(0xea02327eu, field1B8));
    return LoadProps(exemplar, properties);
}

// ===========================================================================
// cSC4PowerSimulator (Read 0x10020fada / Write 0x10020f6da)
// ===========================================================================
bool PowerSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version > 6) {
        return false;
    }
    if (!ReadRef(stream, simManagerRef)) {
        return false;
    }
    stream >> field68 >> field6C >> field70;
    if (version >= 6) {
        stream >> field94;
    } else {
        Sint32 raw = 0;
        stream >> raw;
        field94 = static_cast<Float32>(raw) * 0.01f;
    }
    stream >> field98 >> field9C >> fieldA0 >> fieldA4 >> fieldA8 >> field80 >> field84 >>
        field88 >> field8C >> field90;
    if (!stream.Good() || !ReadRef(stream, gridRefB0)) {
        return false;
    }
    Uint32 rows = 0, cols = 0;
    stream >> rows >> cols;
    if (!stream.Good()) {
        return false;
    }
    sint16Grid.assign(rows, std::vector<Sint16>(cols, 0));
    for (auto& row : sint16Grid) {
        for (Sint16& v : row) {
            stream >> v;
        }
    }
    for (auto& year : monthlyHistory) {
        for (Sint32& v : year) {
            stream >> v;
        }
    }
    for (Sint32& v : field2B0) {
        stream >> v;
    }
    Sint32 entryCount = 0;
    stream >> entryCount;
    for (Sint32& v : field358) {
        stream >> v;
    }
    if (!stream.Good() || entryCount < 0) {
        return false;
    }
    occupantEntries.clear();
    occupantEntries.reserve(static_cast<std::size_t>(entryCount));
    for (Sint32 i = 0; i < entryCount; ++i) {
        Uint32 key = 0;
        SimSerializableRef ref;
        stream >> key;
        if (!ReadRef(stream, ref)) {
            return false;
        }
        occupantEntries.emplace_back(key, ref);
    }
    stream >> field31C >> field320;
    if (!stream.Good()) {
        return false;
    }
    poleOccupantRefs.clear();
    poleOccupantRefs.reserve(field31C);
    for (Uint32 i = 0; i < field31C; ++i) {
        SimSerializableRef ref;
        if (!ReadRef(stream, ref)) {
            return false;
        }
        poleOccupantRefs.push_back(ref);
    }
    Uint32 arrayCount = 0;
    stream >> arrayCount;
    if (!stream.Good()) {
        return false;
    }
    auto readArray = [&stream](std::vector<Uint32>& arr, Uint32 n) noexcept {
        arr.assign(n, 0);
        for (Uint32& v : arr) {
            stream >> v;
        }
        return stream.Good();
    };
    if (!readArray(array328, arrayCount) || !readArray(array330, arrayCount) ||
        !readArray(array348, arrayCount) || !readArray(array340, arrayCount)) {
        return false;
    }
    stream >> field350 >> field3A8 >> field3AC >> field3B0 >> field3B1;
    field3A8 = false; // the binary forces +0x3a8 to 0 after reading
    stream >> field3C4;
    return stream.Good();
}

bool PowerSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{6};
    WriteRef(stream, simManagerRef);
    stream << field68 << field6C << field70 << field94 << field98 << field9C << fieldA0
           << fieldA4 << fieldA8 << field80 << field84 << field88 << field8C << field90;
    WriteRef(stream, gridRefB0);
    const Uint32 rows = static_cast<Uint32>(sint16Grid.size());
    const Uint32 cols = rows > 0 ? static_cast<Uint32>(sint16Grid[0].size()) : 0;
    stream << rows << cols;
    for (const auto& row : sint16Grid) {
        for (Sint16 v : row) {
            stream << v;
        }
    }
    for (const auto& year : monthlyHistory) {
        for (Sint32 v : year) {
            stream << v;
        }
    }
    for (Sint32 v : field2B0) {
        stream << v;
    }
    stream << static_cast<Sint32>(occupantEntries.size());
    for (Sint32 v : field358) {
        stream << v;
    }
    for (const auto& [key, ref] : occupantEntries) {
        stream << key;
        WriteRef(stream, ref);
    }
    stream << field31C << field320;
    for (const SimSerializableRef& ref : poleOccupantRefs) {
        WriteRef(stream, ref);
    }
    const Uint32 arrayCount = static_cast<Uint32>(array328.size());
    stream << arrayCount;
    for (const std::vector<Uint32>* arr : {&array328, &array330, &array348, &array340}) {
        for (Uint32 v : *arr) {
            stream << v;
        }
    }
    stream << field350 << field3A8 << field3AC << field3B0 << field3B1 << field3C4;
    return true;
}

bool PowerSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

bool PowerTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    field314 = static_cast<Uint32>(exemplar.GetInt(0xa911e2f1u, field314));
    field318 = exemplar.GetFloat(0xe911e2fcu, field318);
    field3FC = static_cast<Sint32>(exemplar.GetInt(0x2912202eu, field3FC));
    return LoadProps(exemplar, properties);
}

// ===========================================================================
// cSC4PlumbingSimulator (Read 0x10009d0a6 / Write 0x10009cdd8)
// ===========================================================================
namespace {

bool ReadWaterProducer(cIGZIStream& stream, WaterProducerRecord& p) noexcept {
    stream >> p.field00 >> p.field04 >> p.field08 >> p.field0C >> p.field10 >> p.field14 >>
        p.field1C >> p.field20;
    if (!stream.Good()) {
        return false;
    }
    return ReadRef(stream, p.occupantRef);
}

void WriteWaterProducer(cIGZOStream& stream, const WaterProducerRecord& p) {
    stream << p.field00 << p.field04 << p.field08 << p.field0C << p.field10 << p.field14
           << p.field1C << p.field20;
    WriteRef(stream, p.occupantRef);
}

} // namespace

bool PlumbingSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version != 4) {
        return false;
    }
    if (!ReadRef(stream, gridRef58) || !ReadRef(stream, gridRef60)) {
        return false;
    }
    stream >> gridWidth >> gridHeight;
    if (!stream.Good()) {
        return false;
    }
    const std::size_t cellCount =
        static_cast<std::size_t>(gridWidth) * static_cast<std::size_t>(gridHeight);
    // The binary reads one byte per cell through the +0x28 slot into a raw
    // byte buffer (shipped saves hold values such as 0x40), so the bytes are
    // preserved verbatim rather than normalized to 0/1.
    boolGrid.assign(cellCount, 0);
    for (Uint8& cell : boolGrid) {
        stream >> cell;
    }
    stream >> fieldE4 >> fieldE8 >> field1B8;
    Uint32 producerCount = 0;
    stream >> producerCount >> field23C;
    if (!stream.Good()) {
        return false;
    }
    producers.clear();
    producers.reserve(producerCount);
    for (Uint32 i = 0; i < producerCount; ++i) {
        WaterProducerRecord p;
        if (!ReadWaterProducer(stream, p)) {
            return false;
        }
        producers.push_back(p);
    }
    if (!ReadWaterProducer(stream, currentProducer)) {
        return false;
    }
    if (!ReadScanningIterator(stream, scanningIterator)) {
        return false;
    }
    stream >> field198;
    Uint32 refCount = 0;
    stream >> refCount >> field21C >> field220;
    if (!stream.Good()) {
        return false;
    }
    gridRefList200.clear();
    gridRefList200.reserve(refCount);
    for (Uint32 i = 0; i < refCount; ++i) {
        SimSerializableRef ref;
        if (!ReadRef(stream, ref)) {
            return false;
        }
        gridRefList200.push_back(ref);
    }
    if (!ReadRef(stream, simManagerRef)) {
        return false;
    }
    stream >> field178 >> field180 >> field188 >> field190;
    return stream.Good();
}

bool PlumbingSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{4};
    WriteRef(stream, gridRef58);
    WriteRef(stream, gridRef60);
    stream << gridWidth << gridHeight;
    for (Uint8 cell : boolGrid) {
        stream << cell;
    }
    stream << fieldE4 << fieldE8 << field1B8;
    stream << static_cast<Uint32>(producers.size()) << field23C;
    for (const WaterProducerRecord& p : producers) {
        WriteWaterProducer(stream, p);
    }
    WriteWaterProducer(stream, currentProducer);
    WriteScanningIterator(stream, scanningIterator);
    stream << field198;
    stream << static_cast<Uint32>(gridRefList200.size()) << field21C << field220;
    for (const SimSerializableRef& ref : gridRefList200) {
        WriteRef(stream, ref);
    }
    WriteRef(stream, simManagerRef);
    stream << field178 << field180 << field188 << field190;
    return true;
}

bool PlumbingSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

bool PlumbingTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    fieldF8 = static_cast<Uint32>(exemplar.GetInt(0x68f5436au, fieldF8));
    fieldFC = static_cast<Uint32>(exemplar.GetInt(0x68f5436bu, fieldFC));
    field100 = static_cast<Uint32>(exemplar.GetInt(0x68f5436cu, field100));
    field104 = static_cast<Uint32>(exemplar.GetInt(0x68f5436du, field104));
    field108 = static_cast<Uint32>(exemplar.GetInt(0xaa161f81u, field108));
    field228 = exemplar.GetFloat(0x8a235c56u, field228);
    field288 = exemplar.GetFloat(0x8a84778fu, field288);
    return LoadProps(exemplar, properties);
}

// ===========================================================================
// cSC4PollutionSimulator (Read 0x100096588 / Write 0x1000961d0)
// ===========================================================================
namespace {
void BindPollutionGridRuntime(PollutionSimulatorSaveRecord& record) noexcept {
    static_assert(sizeof(bool) == 1);
    static_assert(std::is_same_v<std::uint8_t, unsigned char>);
    auto& state = record.pollutionGridRuntime;
    state.radioactiveDirty = reinterpret_cast<unsigned char*>(&record.field328);
    state.airDirty = reinterpret_cast<unsigned char*>(&record.field329);
    state.waterDirty = reinterpret_cast<unsigned char*>(&record.field32A);
    state.garbageDirty = reinterpret_cast<unsigned char*>(&record.field32B);
    state.activeLandfillCount = &record.field2CC;
}
}

// Exact cSC4PollutionSimulator::SetIsRadioactive @ 0x10009a738.
void PollutionSimulatorSaveRecord::SetIsRadioactive(Uint32 x, Uint32 z, bool value) noexcept {
    BindPollutionGridRuntime(*this);
    pollution_grid_kernel::SetIsRadioactive(pollutionGridRuntime, x, z, value);
}
// Exact cSC4PollutionSimulator::SetAirValue @ 0x10009a790.
void PollutionSimulatorSaveRecord::SetAirValue(Uint32 x, Uint32 z, Sint32 value) noexcept {
    BindPollutionGridRuntime(*this);
    pollution_grid_kernel::SetAirValue(pollutionGridRuntime, x, z, value);
}
// Exact cSC4PollutionSimulator::SetWaterValue @ 0x10009a7e8.
void PollutionSimulatorSaveRecord::SetWaterValue(Uint32 x, Uint32 z, Sint32 value) noexcept {
    BindPollutionGridRuntime(*this);
    pollution_grid_kernel::SetWaterValue(pollutionGridRuntime, x, z, value);
}
// Exact cSC4PollutionSimulator::SetGarbageValue @ 0x10009a840.
void PollutionSimulatorSaveRecord::SetGarbageValue(Uint32 x, Uint32 z, Sint32 value) noexcept {
    BindPollutionGridRuntime(*this);
    pollution_grid_kernel::SetGarbageValue(pollutionGridRuntime, x, z, value);
}
// Exact cSC4PollutionSimulator::IsActiveLandfill @ 0x10009a924.
bool PollutionSimulatorSaveRecord::IsActiveLandfill(Uint32 x, Uint32 z) const noexcept {
    return pollution_grid_kernel::IsActiveLandfill(pollutionGridRuntime, x, z);
}
// Exact cSC4PollutionSimulator::SetIsActiveLandfill @ 0x10009a940.
void PollutionSimulatorSaveRecord::SetIsActiveLandfill(Uint32 x, Uint32 z, bool value) noexcept {
    BindPollutionGridRuntime(*this);
    pollution_grid_kernel::SetIsActiveLandfill(pollutionGridRuntime, x, z, value);
}

// Required integration includes: civic_simulators.h, tract_developer.h,
// pollution_capacity_kernel.h, <new>. Fragment belongs in namespace opensc4.
namespace {
// Exact cSC4PollutionSimulator::GetCurrentCapacity @ 0x1000992cc.
struct PollutionCapacityRecordContext {
    const PollutionSimulatorSaveRecord& record;
};
const pollution_capacity::Callbacks kPollutionCapacityRecordCallbacks{
    [](void*, void* occupant) -> void* {
        return const_cast<OccupantPropertyHolderSeam*>(
            static_cast<const OccupantSeam*>(occupant)->GetPropertyHolder());
    },
    [](void* context, void* holder, Uint32 id, Uint32& out) {
        const auto& record = static_cast<PollutionCapacityRecordContext*>(context)->record;
        return record.capacityAccess->ReadCapacity(
            *static_cast<const OccupantPropertyHolderSeam*>(holder), id, out);
    },
    [](void* context, void* occupant) {
        const auto& record = static_cast<PollutionCapacityRecordContext*>(context)->record;
        return record.capacityAccess->IsOccupantPowered(
            *static_cast<const OccupantSeam*>(occupant));
    },
    [](void* context) {
        // Live field348 read AFTER power dispatch @ 0x10009933b.
        return static_cast<PollutionCapacityRecordContext*>(context)->record.field348;
    },
    [](void* context, void* holder, Uint32 id) -> void* {
        const auto& record = static_cast<PollutionCapacityRecordContext*>(context)->record;
        return const_cast<RawCurvePropertySeam*>(record.capacityAccess->LookupCurve(
            *static_cast<const OccupantPropertyHolderSeam*>(holder), id));
    },
    [](void*, void* storage) { ::new (storage) cSC4ResponseCurve; },
    [](void*, void* storage, void* property) {
        // SetResponseCurveData @ 0x1000ac37a: value, data, count in that order.
        const auto* value = static_cast<const RawCurvePropertySeam*>(property)->GetValue();
        const Float32* data = value->GetScalarData();
        const Uint32 count = value->GetScalarCount();
        (void)static_cast<cSC4ResponseCurve*>(storage)->SetResponseCurveData(
            std::span<const Float32>(data, count));
    },
    [](void*, void* holder, Uint32 id, Float32& out) {
        return static_cast<const OccupantPropertyHolderSeam*>(holder)->GetPropertyValue(id, out);
    },
    [](void*, void* storage, Float32 input) {
        Float32 out;
        if (!static_cast<const cSC4ResponseCurve*>(storage)->TryGetOutputValue(input, out)) {
            // Portable boundary for null backing read @ 0x1000ac542.
            throw ResponseCurveStorageFault{};
        }
        return out;
    },
    [](void*, void* storage) noexcept {
        static_cast<cSC4ResponseCurve*>(storage)->~cSC4ResponseCurve();
    }
};
} // namespace

// Exact cSC4PollutionSimulator::GetCurrentCapacity @ 0x1000992cc.
bool PollutionSimulatorSaveRecord::GetCurrentCapacity(
    Uint32 propertyId, const OccupantSeam& occupant, Uint32& out) const {
    PollutionCapacityRecordContext context{*this};
    alignas(cSC4ResponseCurve) unsigned char storage[sizeof(cSC4ResponseCurve)];
    return pollution_capacity::get_current_capacity(
        {&context, storage}, kPollutionCapacityRecordCallbacks, propertyId,
        const_cast<OccupantSeam*>(&occupant), out);
}

bool PollutionSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version > 4) {
        return false;
    }
    for (SimSerializableRef& ref : gridRefs78) {
        if (!ReadRef(stream, ref)) {
            return false;
        }
    }
    for (SimSerializableRef& ref : gridRefs98) {
        if (!ReadRef(stream, ref)) {
            return false;
        }
    }
    stream >> fieldB8 >> fieldBC;
    if (!ReadRefList(stream, listC0) || !ReadRefList(stream, listD8) ||
        !ReadRefList(stream, listF0) || !ReadRefList(stream, list108)) {
        return false;
    }
    stream >> field208 >> field20C >> field210 >> field214 >> field218 >> field21C >>
        field220 >> field224 >> field228 >> field22C;
    hasLegacyName = false;
    legacyName.clear();
    if (version <= 2) {
        // The binary reads this GZString into a temp and discards it.
        hasLegacyName = true;
        if (!stream.ReadGZString(legacyName)) {
            return false;
        }
    }
    stream >> field234 >> field238 >> field23C >> field240 >> field244 >> field248 >>
        field2D0 >> field2D8;
    if (version >= 4) {
        stream >> field2C8;
    }
    stream >> field288 >> field290 >> field298 >> field2A0 >> field2A8 >> field2B0 >>
        field2B8 >> field250 >> field258 >> field260 >> field268 >> field270 >> field278 >>
        field280 >> field2C4 >> field2C0 >> field2CC >> field2E4;
    if (!stream.Good()) {
        return false;
    }
    if (!ReadRandomIterator(stream, randomIterator)) {
        return false;
    }
    stream >> field328 >> field329 >> field32A >> field32B;
    hasLegacyUint32 = false;
    legacyUint32 = 0;
    if (version <= 2) {
        hasLegacyUint32 = true;
        stream >> legacyUint32;
    }
    stream >> field348 >> field34C >> field350;
    if (!stream.Good()) {
        return false;
    }
    return ReadRef(stream, simManagerRef);
}

bool PollutionSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{4};
    for (const SimSerializableRef& ref : gridRefs78) {
        WriteRef(stream, ref);
    }
    for (const SimSerializableRef& ref : gridRefs98) {
        WriteRef(stream, ref);
    }
    stream << fieldB8 << fieldBC;
    WriteRefList(stream, listC0);
    WriteRefList(stream, listD8);
    WriteRefList(stream, listF0);
    WriteRefList(stream, list108);
    stream << field208 << field20C << field210 << field214 << field218 << field21C <<
        field220 << field224 << field228 << field22C;
    stream << field234 << field238 << field23C << field240 << field244 << field248 <<
        field2D0 << field2D8 << field2C8;
    stream << field288 << field290 << field298 << field2A0 << field2A8 << field2B0 <<
        field2B8 << field250 << field258 << field260 << field268 << field270 << field278
           << field280 << field2C4 << field2C0 << field2CC << field2E4;
    WriteRandomIterator(stream, randomIterator);
    stream << field328 << field329 << field32A << field32B;
    stream << field348 << field34C << field350;
    WriteRef(stream, simManagerRef);
    return true;
}

bool PollutionSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

bool PollutionTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    // fieldStatic0 read dropped (swarm wave 1): prop 0x69501944 is superseded
    // by the namespace-scope mTrafficAirFactor, written by
    // ReadSimulatorTunableValues @ 0x100095b29.
    return LoadProps(exemplar, properties);
}

// ---------------------------------------------------------------------------
// Swarm wave 1: the six class-level curve objects (binary BSS
// 0x100b21770..0x100b21798) are .cpp-owned statics; the header holds them as
// extern pointers (civic_simulators.h sits below tract_developer.h in the
// include graph, so the object type cannot appear there). The loader assigns.
// ---------------------------------------------------------------------------
cSC4ResponseCurve gPropRequestCurve;          // 0x100b21770
cSC4ResponseCurve gWpEffectAlphaCurve;        // 0x100b21778
cSC4ResponseCurve gApEffectAlphaCurve;        // 0x100b21780
cSC4ResponseCurve gRpEffectAlphaCurve;        // 0x100b21788
cSC4ResponseCurve gGpEffectAlphaCurve;        // 0x100b21790
cSC4ResponseCurve gGarbageTransportCapacity; // 0x100b21798
cSC4ResponseCurve* mpPropRequestCurve = &gPropRequestCurve;
cSC4ResponseCurve* mWpEffectAlphaCurvePtr = &gWpEffectAlphaCurve;
cSC4ResponseCurve* mApEffectAlphaCurvePtr = &gApEffectAlphaCurve;
cSC4ResponseCurve* mRpEffectAlphaCurvePtr = &gRpEffectAlphaCurve;
cSC4ResponseCurve* mGpEffectAlphaCurvePtr = &gGpEffectAlphaCurve;
cSC4ResponseCurve* mpGarbageTransportCapacity = &gGarbageTransportCapacity;

// ===========================================================================
// Exact cSC4PollutionSimulator::ReadSimulatorTunableValues @ 0x100095912.
// `record` models the simulator instance (binary `this`); `exemplar` is the
// parsed tuning exemplar {0x6534284a, 0xe7e2c2db, 0xc911e35b} (key built
// @ 0x10009592e-0x10009593c; holder via the SL singleton @ 0x100b21280,
// vtable+0x18 call @ 0x100095b05). Pass nullptr to model the lookup-failure
// early-out @ 0x100095b0a: defaults and curve fallbacks below are set
// unconditionally BEFORE the lookup, so they survive the failure path.
// The binary ignores every cSCPropertyHelp::GetPropertyValue bool return
// (no branch follows any call @ 0x100095b29-0x100095c5b): a missing property
// leaves the target at its default. No version gating exists in this method.
// ===========================================================================
void PollutionTuning::ReadSimulatorTunableValues(PollutionSimulatorSaveRecord& record,
                                                const cGZExemplar* exemplar) {
    // Unconditional static defaults @ 0x100095943-0x100095bf (re-set every call).
    mTrafficAirFactor = 8.0f;              // 0x41000000 -> 0x100b2172c @ 0x100095943
    kMaxGarbagePolluteForUI = 27000;      // 0x6978 -> 0x100b21730 @ 0x10009594d
    kMaxAirPolluteForUI = 27000;          // 0x6978 -> 0x100b21734 @ 0x100095958
    kMaxWaterPolluteForUI = 32000;        // 0x7d00 -> 0x100b21738 @ 0x10009595e
    kMaxRadiationPolluteForUI = 32000;    // 0x7d00 -> 0x100b2173c @ 0x100095969
    kMaxAirPollutionDelta = 50;           // 0x32 -> 0x100b21740 @ 0x10009596f
    kMaxWaterPollutionDelta = 50;         // 0x32 -> 0x100b21744 @ 0x10009597a
    kAirPollutedThreshold = 16000;        // 0x3e80 -> 0x100b2174c @ 0x100095980
    kWaterPollutedThreshold = 16000;      // 0x3e80 -> 0x100b21750 @ 0x10009598b
    kGarbagePollutedThreshold = 16000;    // 0x3e80 -> 0x100b21748 @ 0x100095991
    kRadiationPollutedThreshold = 1;      // -> 0x100b21754 @ 0x100095997
    kGarbageAbductionAmount = 5000;       // 0x1388 -> 0x100b21758 @ 0x1000959a1
    mGarbageScalingFactor = 0.125f;        // 0x3e000000 -> 0x100b2175c @ 0x1000959ab
    kLandfillTileCapacity = 32767;        // 0x7fff -> 0x100b21760 @ 0x1000959b5
    kMaxGarbageOnValve = 655340;          // 0x9ffec -> 0x100b21764 @ 0x1000959bf

    // Curve fallbacks @ 0x1000959c9-0x100095ae7, unconditional. The six
    // class-level curves get the single point (0.0f, 0.0f) (push @ 0x1000959e0,
    // SetResponseCurveData @ 0x1000959e5-0x100095a40).
    static constexpr cSC4ResponseCurve::Point kFlatZero[] = {{0.0f, 0.0f}};
    const std::span<const cSC4ResponseCurve::Point> flatZero{kFlatZero};
    mpPropRequestCurve->SetResponseCurveData(flatZero);              // @ 0x1000959e5
    mWpEffectAlphaCurvePtr->SetResponseCurveData(flatZero);            // @ 0x1000959f5
    mApEffectAlphaCurvePtr->SetResponseCurveData(flatZero);            // @ 0x100095a05
    mRpEffectAlphaCurvePtr->SetResponseCurveData(flatZero);            // @ 0x100095a15
    mGpEffectAlphaCurvePtr->SetResponseCurveData(flatZero);            // @ 0x100095a25
    mpGarbageTransportCapacity->SetResponseCurveData(flatZero);       // @ 0x100095a35
    // Instance curves (this+0x1c8 / this+0x1e8) get (0,0), (0,1), (1,1)
    // (pushes @ 0x100095a4d, 0x100095a8c; SetResponseCurveData @ 0x100095ac4,
    // 0x100095ae8). The duplicate x=0 points are in the binary; reproduced
    // verbatim, not "improved".
    static constexpr cSC4ResponseCurve::Point kIdentity[] = {
        {0.0f, 0.0f}, {0.0f, 1.0f}, {1.0f, 1.0f}};
    const std::span<const cSC4ResponseCurve::Point> identity{kIdentity};
    // Runtime-only allocation, flagged for reviewer: the binary's this+0x1c8 /
    // this+0x1e8 are inline curve members; the save-record model stores
    // loader-allocated pointers, so the loader allocates on first use.
    if (!record.field1C8) {
        record.field1C8 = new cSC4ResponseCurve;
    }
    if (!record.field1E8) {
        record.field1E8 = new cSC4ResponseCurve;
    }
    record.field1C8->SetResponseCurveData(identity);                // @ 0x100095ac4
    record.field1E8->SetResponseCurveData(identity);                // @ 0x100095ae8

    if (exemplar == nullptr) {
        return; // holder lookup failed @ 0x100095b0a
    }

    const auto& props = exemplar->properties;
    // Scalar reads in exact binary order @ 0x100095b29-0x100095c5b.
    if (props.find(0x69501944u) != props.end()) {            // f32 @ 0x100095b29
        mTrafficAirFactor = exemplar->GetFloat(0x69501944u, mTrafficAirFactor);
    }
    if (props.find(0x49134bd4u) != props.end()) {            // u32 @ 0x100095b3d
        kMaxGarbagePolluteForUI =
            static_cast<Sint32>(exemplar->GetInt(0x49134bd4u, kMaxGarbagePolluteForUI));
    }
    if (props.find(0x49134bddu) != props.end()) {            // u32 @ 0x100095b51
        kMaxAirPolluteForUI =
            static_cast<Sint32>(exemplar->GetInt(0x49134bddu, kMaxAirPolluteForUI));
    }
    if (props.find(0x09134be2u) != props.end()) {            // u32 @ 0x100095b65
        kMaxWaterPolluteForUI =
            static_cast<Sint32>(exemplar->GetInt(0x09134be2u, kMaxWaterPolluteForUI));
    }
    if (props.find(0x89135044u) != props.end()) {            // u32 @ 0x100095b79
        kMaxRadiationPolluteForUI =
            static_cast<Sint32>(exemplar->GetInt(0x89135044u, kMaxRadiationPolluteForUI));
    }
    if (props.find(0x6a406201u) != props.end()) {            // u32 @ 0x100095b8d
        kMaxAirPollutionDelta =
            static_cast<Sint32>(exemplar->GetInt(0x6a406201u, kMaxAirPollutionDelta));
    }
    if (props.find(0x8a40620bu) != props.end()) {            // u32 @ 0x100095ba1
        kMaxWaterPollutionDelta =
            static_cast<Sint32>(exemplar->GetInt(0x8a40620bu, kMaxWaterPollutionDelta));
    }
    if (props.find(0x0913519bu) != props.end()) {            // u32 @ 0x100095bb5
        kGarbagePollutedThreshold =
            static_cast<Sint32>(exemplar->GetInt(0x0913519bu, kGarbagePollutedThreshold));
    }
    if (props.find(0xa91351a6u) != props.end()) {            // u32 @ 0x100095bc9
        kAirPollutedThreshold =
            static_cast<Sint32>(exemplar->GetInt(0xa91351a6u, kAirPollutedThreshold));
    }
    if (props.find(0x091351aeu) != props.end()) {            // u32 @ 0x100095bdd
        kWaterPollutedThreshold =
            static_cast<Sint32>(exemplar->GetInt(0x091351aeu, kWaterPollutedThreshold));
    }
    if (props.find(0x291351b5u) != props.end()) {            // u32 @ 0x100095bf1
        kRadiationPollutedThreshold =
            static_cast<Sint32>(exemplar->GetInt(0x291351b5u, kRadiationPollutedThreshold));
    }
    if (props.find(0x09135479u) != props.end()) {            // u32 @ 0x100095c05
        kGarbageAbductionAmount =
            static_cast<Sint32>(exemplar->GetInt(0x09135479u, kGarbageAbductionAmount));
    }
    if (props.find(0x09135482u) != props.end()) {            // f32 @ 0x100095c19
        mGarbageScalingFactor = exemplar->GetFloat(0x09135482u, mGarbageScalingFactor);
    }
    if (props.find(0xaa1f8ab7u) != props.end()) {            // f32 -> this+0x2e0 @ 0x100095c33
        record.field2E0 = exemplar->GetFloat(0xaa1f8ab7u, record.field2E0);
    }
    if (props.find(0x4a652679u) != props.end()) {            // u32 @ 0x100095c44
        kMaxGarbageOnValve =
            static_cast<Sint32>(exemplar->GetInt(0x4a652679u, kMaxGarbageOnValve));
    }
    if (props.find(0x4a65267au) != props.end()) {            // u32 @ 0x100095c5b
        kLandfillTileCapacity =
            static_cast<Sint32>(exemplar->GetInt(0x4a65267au, kLandfillTileCapacity));
    }

    // Curve loads @ 0x100095c60-0x100095cfb (SetResponseCurveData(key, propId)
    // in the binary; the key is the same tuning exemplar). FindProp guard
    // models "missing curve property keeps the fallback set above" — see HPR
    // risk note on the undecoded 0x1000ac47e miss path.
    if (auto it = props.find(0x69a437f6u); it != props.end()) {    // @ 0x100095c60
        mpPropRequestCurve->SetResponseCurveData(it->second);
    }
    if (auto it = props.find(0x0a10c772u); it != props.end()) {     // @ 0x100095c75
        mWpEffectAlphaCurvePtr->SetResponseCurveData(it->second);
    }
    if (auto it = props.find(0x0a10c773u); it != props.end()) {     // @ 0x100095c8a
        mApEffectAlphaCurvePtr->SetResponseCurveData(it->second);
    }
    if (auto it = props.find(0x0a10c774u); it != props.end()) {     // @ 0x100095c9f
        mRpEffectAlphaCurvePtr->SetResponseCurveData(it->second);
    }
    if (auto it = props.find(0x0a10c775u); it != props.end()) {     // @ 0x100095cb4
        mGpEffectAlphaCurvePtr->SetResponseCurveData(it->second);
    }
    if (auto it = props.find(0x8c19c3e9u); it != props.end()) {     // @ 0x100095cc9
        mpGarbageTransportCapacity->SetResponseCurveData(it->second);
    }
    if (auto it = props.find(0x0911e2e8u); it != props.end()) {      // -> this+0x1c8 @ 0x100095cea
        record.field1C8->SetResponseCurveData(it->second);
    }
    if (auto it = props.find(0x0911e2e9u); it != props.end()) {      // -> this+0x1e8 @ 0x100095cf8
        record.field1E8->SetResponseCurveData(it->second);
    }
}

// Wiring note (W1 handoff): LoadFromExemplar's fieldStatic0 read drop is
// applied above; city.cpp's loadCivicTuning lambda still needs a follow-up
// call mPollutionTuning.ReadSimulatorTunableValues(mPollutionSaveRecord,
// parsed ? &exemplar : nullptr).

// Exact cSC4PollutionSimulator::AddToGarbageSupply @ 0x10009abde.
// vtable slot +0xe8, dispatched from eatGarbage block D @ 0x10009a51c.
void PollutionSimulatorSaveRecord::AddToGarbageSupply(Sint32 amount) noexcept {
    Sint32 supply = field34C;  // mov eax, [rdi+0x34c] @ 0x10009abe4
    if (amount < 0 && supply + amount < 0) { // test/jns + cmovs @ 0x10009abe4/0x10009abf4
        amount = -supply;      // neg ecx @ 0x10009abee
    }
    supply = supply + amount;  // add eax, esi @ 0x10009abf9
    if (supply < 0) supply = 0; // xor ecx,ecx; cmovs @ 0x10009abf7/0x10009abfb
    field34C = supply;         // mov [rdi+0x34c] @ 0x10009abfe
}

namespace {

// Reinterpret a Float32's bits without value conversion (movd idiom).
inline Uint32 PSBitsOfF32(Float32 value) noexcept {
    Uint32 bits = 0;
    static_assert(sizeof(bits) == sizeof(value), "float/u32 size mismatch");
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

// Exact float -> Sint32 conversion used by the waste-to-energy and incinerator
// blocks (B @ 0x10009a38e..0x10009a3c2, C @ 0x10009a415..0x10009a449).
// Two SEPARATELY-rounded adds first (no fusion):
//   t = (value + (-0.5f)) + 12582912.0f   @ 0x10009a396 / 0x10009a39a
//   (constants 0x100a158b4 = -0.5f bits 0xbf000000, 0x100a158b8 = 12582912.0f bits 0x4b400000)
// then the integer is recovered from the low mantissa bits:
//   candidate = (i32)(bits - 0x4b400000)  @ 0x10009a3a6 (lea edi, [rcx - 0x4b400000])
//   alt       = (i32)(bits + 0xb4c00001)  @ 0x10009a3ac (== candidate + 1, wrapped)
// and an ordered-equality cmov pair picks alt only when value == (float)alt:
//   cmovne/cmovp -> candidate otherwise   @ 0x10009a3bc / 0x10009a3bf
// Net effect on every input (verified bit-level, do NOT "simplify" to a library
// call): floor(value), with exact integers round-tripping through the alt path.
inline Sint32 PSFloatFloorSint32(Float32 value) noexcept {
    const Uint32 bits = PSBitsOfF32((value + (-0.5f)) + 12582912.0f);
    const Sint32 candidate = static_cast<Sint32>(bits - 0x4b400000u);
    const Sint32 alt = static_cast<Sint32>(bits + 0xb4c00001u);
    return (value == static_cast<Float32>(alt)) ? alt : candidate;
}

} // namespace

// Exact cSC4PollutionSimulator::eatGarbage @ 0x10009a2ee.
// Parameter mapping from the binary: rsi = 1st ref (incinerator pool, block C),
// rdx = 2nd ref (waste-to-energy pool, block B). Both are unsigned in/out.
// Consumes a garbage amount through five consumers, in this exact order:
//   A. neighbor export    (+0x260 accumulator, +0x2c0 capacity)        @ 0x10009a314
//   B. waste-to-energy    (+0x278 accumulator; pool = 2nd ref, rdx)    @ 0x10009a374
//   C. incinerator        (+0x270 accumulator; pool = 1st ref, rsi)    @ 0x10009a3f8
//   D. import/transport   (spLandfillDeveloper +0x70, curve, +0x348,
//                          AddToGarbageSupply @ 0x10009abde, vt+0xe8)  @ 0x10009a47f
//   E. abduction          (+0x2c4 accumulator, kGarbageAbductionAmount) @ 0x10009a57b
// Returns the float total eaten; the addss accumulation order across blocks is
// preserved exactly. NaN garbage reaches no consumer (every skip guard is a
// jbe, i.e. <= OR unordered, expressed below as `garbage > 0.0f`).
Float32 PollutionSimulatorSaveRecord::EatGarbage(Float32 garbage,
                                                 Uint32& incineratorCapacityRemaining,
                                                 Uint32& wasteToEnergyCapacityRemaining) noexcept {
    Float32 totalEaten = 0.0f; // xorps xmm5, xmm5 @ 0x10009a300

    // @ 0x10009a303: ucomiss xmm5(0), xmm0(garbage); jae (ordered, CF=0) ->
    // 0 >= garbage returns immediately. NaN falls through here (CF=1) but is
    // caught by every jbe guard below, so NaN still returns 0 unchanged.
    if (0.0f >= garbage) {
        return totalEaten;
    }

    // ---- Block A: neighbor export (+0x260 exported, +0x2c0 capacity) ----
    // @ 0x10009a30f: ucomiss xmm0(garbage), xmm4(0); jbe (<= OR unordered) skips.
    if (garbage > 0.0f) {
        // 32-bit wrapping subtraction @ 0x10009a314..0x10009a326:
        //   ecx = (i32)field2C0; rax = (i64)field260; ecx -= eax (low dword only).
        const Sint32 room = static_cast<Sint32>(
            static_cast<Uint32>(field2C0) - static_cast<Uint32>(static_cast<Sint32>(field260)));
        if (room > 0) { // test ecx, ecx; jle skips @ 0x10009a326/0x10009a328
            const Float32 roomf = static_cast<Float32>(room); // cvtsi2ss ecx @ 0x10009a32a
            // cmpnless blend @ 0x10009a334: picks roomf unless garbage < roomf
            // (i.e. min; the unordered path would pick roomf, but garbage is
            // ordered > 0 here, so it cannot fire).
            const Float32 eaten = !(garbage < roomf) ? roomf : garbage;
            // cvtsi2ss rax (full 64-bit field260) + addss + cvttss2si + qword
            // store @ 0x10009a348..0x10009a356.
            field260 = static_cast<Sint64>(static_cast<Float32>(field260) + eaten);
            totalEaten = 0.0f + eaten; // xorps/addss xmm5 @ 0x10009a35e/0x10009a361
            // jae @ 0x10009a365 consumes the flags of ucomiss xmm1(roomf),
            // xmm0(garbage) @ 0x10009a32e (the blend instructions in between do
            // not touch EFLAGS): ordered roomf >= garbage -> the export deal
            // absorbed everything; return now.
            if (roomf >= garbage) {
                return totalEaten;
            }
            garbage = garbage - roomf; // subss @ 0x10009a36b
        } else {
            totalEaten = 0.0f; // xorps xmm5 @ 0x10009a371 (mirror; already 0)
        }
    } else {
        totalEaten = 0.0f; // jbe @ 0x10009a312 -> 0x10009a371 (mirror; already 0)
    }

    // ---- Block B: waste-to-energy (+0x278; pool = 2nd ref, rdx) ----
    // @ 0x10009a374: ucomiss xmm0(garbage), xmm4(0); jbe skips.
    if (garbage > 0.0f) {
        const Uint32 pool = wasteToEnergyCapacityRemaining; // mov eax, [rdx] @ 0x10009a379
        if (pool != 0) { // test eax, eax; je skips @ 0x10009a37b/0x10009a37d
            const Float32 poolf = static_cast<Float32>(static_cast<Uint64>(pool));
            // ^ mov ecx, eax (zero-extends) + cvtsi2ss rcx @ 0x10009a37f/0x10009a384:
            //   the pool converts UNSIGNED.
            Float32 amountToAccumulate; // xmm0 at the merge @ 0x10009a3d1
            Float32 garbageAfter;       // xmm2 at the merge @ 0x10009a3d1
            // ucomiss xmm1(poolf), xmm0(garbage); jbe @ 0x10009a389/0x10009a38c
            // (<= OR unordered polarity; poolf is never NaN but keep the shape).
            if (!(poolf > garbage)) {
                // QUIRK (reproduced exactly, do not "fix"): pool <= garbage zeroes
                // the pool but eats NOTHING: no +0x278 add, total unchanged,
                // garbage unchanged (it flows on to blocks C/D/E).
                wasteToEnergyCapacityRemaining = 0; // xor eax,eax; mov [rdx] @ 0x10009a3c9/0x10009a3d1
                amountToAccumulate = 0.0f;          // xorps xmm0 @ 0x10009a3ce
                garbageAfter = garbage;             // movaps xmm2, xmm0 @ 0x10009a3cb
            } else {
                const Sint32 consumed = PSFloatFloorSint32(garbage);
                // ^ magic floor @ 0x10009a38e..0x10009a3c2
                wasteToEnergyCapacityRemaining = pool - static_cast<Uint32>(consumed);
                // ^ sub eax, ecx; mov [rdx] @ 0x10009a3c2/0x10009a3d1
                // QUIRK (reproduced exactly): the pool loses floor(garbage) but
                // the accumulator and total take the FULL float garbage, not
                // the floored integer.
                amountToAccumulate = garbage; // xmm0 is untouched on this path
                garbageAfter = 0.0f;          // xorps xmm2 @ 0x10009a3c4
            }
            // cvtsi2ss qword [r12+0x278] + addss + cvttss2si + qword store
            // @ 0x10009a3d6..0x10009a3e9.
            field278 = static_cast<Sint64>(static_cast<Float32>(field278) + amountToAccumulate);
            totalEaten = totalEaten + amountToAccumulate; // addss xmm5, xmm0 @ 0x10009a3f1
            garbage = garbageAfter;                        // movaps xmm0, xmm2 @ 0x10009a3f5
        }
    }

    // ---- Block C: incinerator (+0x270; pool = 1st ref, rsi) ----
    // @ 0x10009a3f8: ucomiss xmm0(garbage), xmm3(0); jbe skips.
    if (garbage > 0.0f) {
        const Uint32 pool = incineratorCapacityRemaining; // mov eax, [rsi] @ 0x10009a400
        if (pool != 0) { // test/je @ 0x10009a402/0x10009a404
            const Float32 poolf = static_cast<Float32>(static_cast<Uint64>(pool));
            // ^ mov ecx, eax + cvtsi2ss rcx @ 0x10009a406/0x10009a40b (unsigned)
            Float32 amountToAccumulate; // xmm0 at the merge @ 0x10009a458
            Float32 garbageAfter;       // xmm1 at the merge @ 0x10009a458
            // ucomiss xmm1(poolf), xmm0(garbage); jbe @ 0x10009a410/0x10009a413.
            if (!(poolf > garbage)) {
                // Same QUIRK as block B: pool zeroed, nothing eaten.
                incineratorCapacityRemaining = 0; // xor eax; mov [rsi] @ 0x10009a450/0x10009a458
                amountToAccumulate = 0.0f;        // xorps xmm0 @ 0x10009a455
                garbageAfter = garbage;            // movaps xmm1, xmm0 @ 0x10009a452
            } else {
                const Sint32 consumed = PSFloatFloorSint32(garbage);
                // ^ magic floor @ 0x10009a415..0x10009a449 (identical idiom to
                //   block B; the alt register here is edx @ 0x10009a42d)
                incineratorCapacityRemaining = pool - static_cast<Uint32>(consumed);
                // ^ sub eax, ecx; mov [rsi] @ 0x10009a449/0x10009a458
                amountToAccumulate = garbage; // xmm0 untouched on this path
                garbageAfter = 0.0f;          // xorps xmm1 @ 0x10009a44b
            }
            // cvtsi2ss qword [r12+0x270] + addss + cvttss2si + qword store
            // @ 0x10009a45d..0x10009a470.
            field270 = static_cast<Sint64>(static_cast<Float32>(field270) + amountToAccumulate);
            totalEaten = totalEaten + amountToAccumulate; // addss xmm5, xmm0 @ 0x10009a478
            garbage = garbageAfter;                        // movaps xmm0, xmm1 @ 0x10009a47c
        }
    }

    // ---- Block D: import via transport (spLandfillDeveloper at +0x70) ----
    // @ 0x10009a47f: ucomiss xmm0(garbage), xmm3(0); jbe skips.
    // NOTE: the two singleton reads and the curve evaluation always run, even
    // when the resulting capacity is <= 0 (the singleton advances regardless).
    if (garbage > 0.0f) {
        const Float32 totalSaved = totalEaten;  // movss [rbp-0x28], xmm5 @ 0x10009a488
        const Float32 garbageSaved = garbage;   // movaps [rbp-0x40], xmm0 @ 0x10009a48d

        Sint32 rangeOutFirst = 0;  // lea r14,[rbp-0x48]; mov dword [r14], 0 @ 0x10009a493/0x10009a497
        Sint32 rangeOutSecond = 0; // lea r15,[rbp-0x44]; mov dword [r15], 0 @ 0x10009a49a/0x10009a49e
        // this+0x70 is the SL::spLandfillDeveloper singleton (non-owning; stored
        // by Init @ 0x1000958c8..0x1000958d2 from the global at 0x100b21380).
        // vtable+0x50 call with the two out-refs @ 0x10009a4a1..0x10009a4af.
        field70->ReadRangeAsInts(rangeOutFirst, rangeOutSecond);
        const Sint32 range = rangeOutSecond - rangeOutFirst;
        // ^ mov ebx, [r15]; sub ebx, [r14] @ 0x10009a4b2/0x10009a4b5 (i32 wrap)

        const Float32 capacityScale = field348;
        // ^ movss xmm0, dword [r12+0x348]; movss [rbp-0x24], xmm0 @ 0x10009a4b8/0x10009a4c2
        // vtable+0x58 call @ 0x10009a4d6: result in eax; only the low 32 bits
        // are used and they convert UNSIGNED (cvtsi2ss xmm0, eax @ 0x10009a4dc).
        const Float32 curveInput = static_cast<Float32>(field70->ReadValueLow32());
        // cSC4ResponseCurve::GetOutputValue(float) const @ 0x1000ac4f6, invoked
        // on the global mpGarbageTransportCapacity (0x100b21798) @ 0x10009a4c7/0x10009a4e3.
        const Float32 curveOutput = mpGarbageTransportCapacity->GetOutputValue(curveInput);
        const Float32 product = curveOutput * capacityScale; // mulss @ 0x10009a4e8

        // @ 0x10009a4ed..0x10009a506: round-to-nearest-even bits trick on
        // product + 12582912.0f (0x100a158b8), with field34C folded in at the
        // BIT level: esi = 0xb4c00000 - field34C + bits(product + magic)
        // (esi = bits - 0x4b400000 - field34C, mod 2^32).
        Sint32 importCapacity = static_cast<Sint32>(
            0xb4c00000u - static_cast<Uint32>(field34C) +
            PSBitsOfF32(product + 12582912.0f));
        if (importCapacity > range) { // cmp esi, ebx; cmovg @ 0x10009a508/0x10009a50a
            importCapacity = range;
        }
        if (importCapacity > 0) { // test esi, esi; jle skips @ 0x10009a50d/0x10009a50f
            const Float32 importCapacityf = static_cast<Float32>(importCapacity);
            // ^ cvtsi2ss xmm0, esi @ 0x10009a514
            // vtable slot +0xe8 -> cSC4PollutionSimulator::AddToGarbageSupply(int)
            // @ 0x10009abde (fresh --vtable resolution). The argument is the
            // INTEGER in esi; xmm0 is stale on both paths and is NOT the argument.
            // @ 0x10009a51c: ucomiss xmm0(importCapacityf), xmm1(garbageSaved);
            //    jae @ 0x10009a52a (ordered only).
            if (importCapacityf >= garbageSaved) {
                // Full import: the integer argument is round-half-even of
                // garbageSaved via bits(garbageSaved + 12582912.0f) + 0xb4c00000
                // @ 0x10009a550..0x10009a560.
                const Sint32 imported = static_cast<Sint32>(
                    PSBitsOfF32(garbageSaved + 12582912.0f) + 0xb4c00000u);
                AddToGarbageSupply(imported); // call @ 0x10009a569 (esi = imported)
                garbage = 0.0f;                // xorps xmm0 @ 0x10009a56f
                totalEaten = totalSaved + garbageSaved;
                // ^ movss xmm5, [rbp-0x28]; addss xmm5, xmm1 @ 0x10009a572/0x10009a577
            } else {
                AddToGarbageSupply(importCapacity); // call @ 0x10009a534 (esi = cap)
                totalEaten = totalSaved + importCapacityf;
                // ^ movss xmm5, [rbp-0x28]; addss xmm5, xmm1 @ 0x10009a572/0x10009a577
                garbage = garbageSaved - importCapacityf; // subss @ 0x10009a53f
            }
        } else {
            garbage = garbageSaved;  // movaps xmm0, [rbp-0x40] @ 0x10009a545
            totalEaten = totalSaved; // movss xmm5, [rbp-0x28] @ 0x10009a549
        }
    }

    // ---- Block E: abduction (+0x2c4; kGarbageAbductionAmount) ----
    // @ 0x10009a57b: ucomiss xmm0(garbage), xmm1(0); jbe returns @ 0x10009a57e/0x10009a581.
    if (garbage > 0.0f) {
        const Sint32 budget = static_cast<Sint32>(
            static_cast<Uint32>(kGarbageAbductionAmount) - field2C4);
        // ^ mov ecx, [kGarbageAbductionAmount]; mov eax, [r12+0x2c4];
        //   sub ecx, eax @ 0x10009a583..0x10009a591 (i32 wrap)
        if (budget > 0) { // test ecx, ecx; jle returns @ 0x10009a593/0x10009a595
            const Float32 budgetf = static_cast<Float32>(budget); // cvtsi2ss @ 0x10009a59a
            // QUIRK (reproduced exactly, do NOT "fix" to min): the cmpnless blend
            // @ 0x10009a5a1..0x10009a5ac picks the MAXIMUM of garbage and
            // budgetf. Block A's blend @ 0x10009a334 uses the same predicate
            // but wires the operands the other way (min); here the mask selects
            // garbage. Budget 50 / garbage 10 -> 50 eaten; garbage 10 /
            // budget 5 -> 10 eaten (over budget either way).
            const Float32 eaten = !(garbage < budgetf) ? garbage : budgetf;
            // (float)(u64)field2C4 + eaten, truncated to i64, stored as a DWORD
            // (mov eax, eax zero-extends @ 0x10009a5af; dword store @ 0x10009a5c2
            // — unlike the qword stores in blocks A/B/C).
            field2C4 = static_cast<Uint32>(static_cast<Sint64>(
                static_cast<Float32>(static_cast<Uint64>(field2C4)) + eaten));
            totalEaten = totalEaten + eaten; // addss xmm5 @ 0x10009a5ca
        }
    }

    return totalEaten; // movaps xmm0, xmm5 @ 0x10009a5ce
}

// ---------------------------------------------------------------------------
// Swarm wave 3 (Nov 2026): the maintenance-cost pair. Seam types
// (OccupantSeam, OccupantPropertyHolderSeam, CurvePropertyData,
// BudgetCostLineData, BudgetCostLineEntrySeam, BudgetSimulatorSeam) are
// declared in the wave-1 header block. Neither method reads any this+0xNNN
// instance field (CalcMaintenanceCosts uses `this` only for the vtable+0x208
// dispatch), so this block adds no fieldNNN members.
// ---------------------------------------------------------------------------

// Exemplar property ids consumed by this pair. The exemplar-name mapping for
// these ids was not present in the recon inputs, so they are cited by raw
// value only — no guessed semantics attached.
inline constexpr Uint32 kPropId2A58D350 =
    0x2a58d350u; // i64 base maintenance cost property @ 0x10009a146
inline constexpr Uint32 kPropIdC910B4CF =
    0xc910b4cfu; // f32 curve-input property @ 0x10009a15e
inline constexpr Uint32 kPropId911E2E7 =
    0x0911e2e7u; // curve-points property (holder vt+0x28 arg) @ 0x10009a172
inline constexpr Uint32 kPropIdCA58EDC7 =
    0xca58edc7u; // budget cost-line property (budget vt+0x198 arg) @ 0x10009a26d

// cvttss2si rax, xmm0 @ 0x10009a1b3 (64-bit destination): truncates toward
// zero; NaN or any value outside [-2^63, 2^63) yields the x86-64 conversion
// error result 0x8000000000000000. A plain static_cast<Sint64> is UB on the
// NaN/overflow paths, so the conversion is spelled out. -2^63 itself is
// representable and converts exactly (note the `<` bounds check); 2^63 is the
// first invalid magnitude (note the `>=`).
Sint64 Cvttss2siI64(Float32 v) noexcept
{
    const Float32 kMinBound = -9223372036854775808.0f; // exactly -2^63
    const Float32 kMaxBound  = 9223372036854775808.0f; // exactly +2^63
    if (std::isnan(v) || v >= kMaxBound || v < kMinBound)
        return static_cast<Sint64>(0x8000000000000000ull);
    return static_cast<Sint64>(v);
}

namespace {

// stub (not authentic): wave 1 has no concrete budget simulator; this
// do-nothing seam exists only so CalcMaintenanceCosts compiles against the
// declared BudgetSimulatorSeam surface. Nothing dispatches it yet.
class StubBudgetSimulatorSeam final : public BudgetSimulatorSeam {
public:
    bool GetCostLine(const OccupantPropertyHolderSeam*, Uint32,
                     BudgetCostLineData& out) override {
        out = BudgetCostLineData{};
        return false;
    }
    BudgetCostLineEntrySeam* GetCostLineEntry(BudgetCostLineData&) override {
        return nullptr;
    }
    void NotifyCostChanged(const OccupantPropertyHolderSeam*, Uint32,
                           Sint64) override {}
};

// SL::spBudgetSim @ 0x100b213f8, loaded @ 0x10009a25f and dereferenced
// @ 0x10009a269 — the bytes show NO null guard on the global, so the seam
// accessor hands back a reference. stub (not authentic): the budget
// simulator is not modeled in wave 1, so the reference targets the no-op
// seam above until a real implementation lands.
BudgetSimulatorSeam& GetBudgetSimulatorSeam() noexcept {
    static StubBudgetSimulatorSeam budgetSim;
    return budgetSim;
}

} // namespace

// Exact cSC4PollutionSimulator::CalcOccupantMaintenanceCost @ 0x10009a11c.
// Binary signature: bool (cISC4Occupant*, long long&) const; it is virtual
// (dispatched through this vtable+0x208 by CalcMaintenanceCosts @
// 0x10009a24e) — the refactor calls it directly, which drops the binary's
// override hook (documented modeling deviation, not a semantic change here).
// The recon annotations ">> Sint16" (occupant vt+0x30) and ">> Uint8"
// (holder vt+0x28) were REJECTED: the bytes show the vt+0x30 result feeding
// cSCPropertyHelp::GetPropertyValue(cISCPropertyHolder const*, ...) as its
// arg1 @ 0x10009a151, so the slot returns the occupant's property holder.
bool PollutionSimulatorSaveRecord::CalcOccupantMaintenanceCost(
    const OccupantSeam& occupant, Sint64& cost) const
{
    // Occupant vtable+0x30 @ 0x10009a132..0x10009a135: fetch the property
    // holder (rdi = occupant, result -> rbx).
    const OccupantPropertyHolderSeam* holder = occupant.GetPropertyHolder();

    // Both out slots are zeroed BEFORE any property read:
    // dword [rbp-0x1c] = 0 @ 0x10009a138 (f32 slot), qword [r14] = 0 @
    // 0x10009a13f (cost slot).
    Float32 curveInput = 0.0f;
    cost = 0;

    // cSCPropertyHelp::GetPropertyValue(holder, 0x2a58d350, long long&) @
    // 0x10009a151 (i64 overload). On failure the out arg is left untouched
    // (the cISCPropertyHolder/cSCPropertyHelp contract), so this early
    // return (je @ 0x10009a158 -> xor ecx,ecx @ 0x10009a1d1) observes
    // cost == 0 — the only false path where cost is 0.
    if (!holder->GetPropertyValue(kPropId2A58D350, cost))
        return false;

    // cSCPropertyHelp::GetPropertyValue(holder, 0xc910b4cf, float&) @
    // 0x10009a166 (f32 overload, out at [rbp-0x1c]). On this false path
    // (je @ 0x10009a16d) cost RETAINS the first property's value.
    if (!holder->GetPropertyValue(kPropIdC910B4CF, curveInput))
        return false;

    // Holder vtable+0x28 GetProperty(0x911e2e7) @ 0x10009a17a..0x10009a17d:
    // non-null iff a property is stored under the id. Null -> false (je @
    // 0x10009a183) with cost still retaining the first property's value.
    const CurvePropertyData* curveProp = holder->GetProperty(kPropId911E2E7);
    if (curveProp == nullptr)
        return false;

    // Stack-constructed cSC4ResponseCurve @ 0x10009a189 (binary ctor @
    // 0x1000ac10e), loaded from the raw cISCProperty via
    // cSC4ResponseCurve::SetResponseCurveData(cISCProperty const*) @
    // 0x10009a197 (binary 0x1000ac37a; modeled through the flat-x/y-pairs
    // seam overload of the reconstructed curve class).
    std::vector<Float32> curveXYPairs;
    curveProp->GetPoints(curveXYPairs);
    cSC4ResponseCurve curve;
    curve.SetResponseCurveData(std::span<const Float32>(curveXYPairs));

    // GetOutputValue(curveInput) @ 0x10009a1a5 (binary 0x1000ac4f6) returns
    // xmm0; then cvtsi2ss xmm1, qword [r14] @ 0x10009a1aa converts cost
    // (i64 -> f32, round-to-nearest-even), and mulss xmm0, xmm1 @ 0x10009a1af
    // forms the product in exactly this operand order:
    // curveOutput * (Float32)cost.
    const Float32 product =
        curve.GetOutputValue(curveInput) * static_cast<Float32>(cost);

    // cvttss2si rax, xmm0 @ 0x10009a1b3 truncates the product toward zero and
    // the result is stored back into the cost slot @ 0x10009a1b8.
    cost = Cvttss2siI64(product);

    // The stack curve is destroyed @ 0x10009a1bb..0x10009a1c8 (its heap
    // buffer released through the null-checked delete at 0x10092aeb0), and
    // the unwind cleanup path @ 0x10009a1e0..0x10009a1f8 mirrors that before
    // rethrowing. C++ destruction of the local covers both here.
    return true; // mov cl, 1 @ 0x10009a1cd
}

// Exact cSC4PollutionSimulator::CalcMaintenanceCosts @ 0x10009a1fe.
// Binary signature: void (ilist<cISC4Occupant*>&) — NON-const (no K in the
// mangled name); the ilist is modeled as a vector of occupant seams. Neither
// this method nor its callee reads any instance field; the record receiver
// exists in the binary only as the vtable+0x208 dispatch target.
void PollutionSimulatorSaveRecord::CalcMaintenanceCosts(
    const std::vector<OccupantSeam*>& occupants)
{
    // Empty-list fast exit @ 0x10009a21a..0x10009a21d (list head == first
    // node); otherwise iterate nodes (value at node+0x10, advance node+0x8).
    for (OccupantSeam* occupant : occupants) {
        // Occupant vtable+0x30 @ 0x10009a232: fetch the property holder once
        // per iteration; this same holder is what the budget calls below
        // receive (rsi = r14 @ 0x10009a275). The per-occupant callee below
        // performs its own vt+0x30 fetch (a genuine double-fetch in the
        // binary @ 0x10009a132), which this model reproduces.
        const OccupantPropertyHolderSeam* holder = occupant->GetPropertyHolder();

        // Loop-local cost slot zeroed every iteration @ 0x10009a238
        // (qword [rbp-0x30] = 0).
        Sint64 cost = 0;

        // This vtable+0x208 @ 0x10009a24e dispatches CalcOccupantMaintenance-
        // Cost (rdi = this, rsi = occupant, rdx = &cost); failure skips the
        // occupant entirely — the budget is not consulted (je @ 0x10009a256
        // jumps past all budget calls).
        if (!CalcOccupantMaintenanceCost(*occupant, cost))
            continue;

        // dword [rbp-0x3c] = 0 @ 0x10009a258: zeroed every iteration but never
        // read inside this window; possibly a flag consumed by a budget
        // callee or the +0x1c tail of the out struct (see hpr_w3.md
        // ambiguity note). Reproduced verbatim, no invented use.
        Uint32 unreadFlag = 0;
        (void)unreadFlag;

        // SL::spBudgetSim @ 0x100b213f8, loaded @ 0x10009a25f and
        // dereferenced @ 0x10009a269 — the bytes show NO null guard on the
        // global, so the seam accessor hands back a reference.
        BudgetSimulatorSeam& budgetSim = GetBudgetSimulatorSeam();

        // Budget vtable+0x198 @ 0x10009a27c: (holder, 0xca58edc7, &out) with
        // the ~0x1c-byte out struct at [rbp-0x58]. Failure skips the
        // occupant (je @ 0x10009a284).
        BudgetCostLineData line{};
        if (!budgetSim.GetCostLine(holder, kPropIdCA58EDC7, line))
            continue;

        // cost == line.currentCost (out +0x10) @ 0x10009a286..0x10009a28e:
        // equal cost skips all three budget calls.
        if (cost == line.currentCost)
            continue;

        // Budget vtable+0x1c8 @ 0x10009a29b: (&out) -> cost-line entry object,
        // test rax, rax @ 0x10009a2a1; null skips BOTH trailing calls (je @
        // 0x10009a2a4 -> loop advance @ 0x10009a2ce).
        BudgetCostLineEntrySeam* entry = budgetSim.GetCostLineEntry(line);
        if (entry == nullptr)
            continue;

        // Entry vtable+0x68 @ 0x10009a2b4 with args (rdi = entry,
        // rsi = cost - out[+0x10] @ 0x10009a2a9..0x10009a2ad, edx =
        // out[+0x18] dword @ 0x10009a2bb, rcx = cost @ 0x10009a2c2).
        entry->ApplyCostDelta(cost - line.currentCost, line.field18, cost);

        // Budget vtable+0x1a8 @ 0x10009a2c8 with args (rdi = budget,
        // rsi = holder, edx = out[+0x18] dword, rcx = cost).
        budgetSim.NotifyCostChanged(holder, line.field18, cost);

        // Loop advance @ 0x10009a2d2..0x10009a2d9 (r15 = node+0x8; while
        // node != list head @ 0x10009a2d9).
    }
}

// ---------------------------------------------------------------------------
// Swarm wave 4 (Nov 2026): leaf accessors + GetGarbageBuildings +
// GetRequestedShare + GZCOM boilerplate. The file-scope tuning statics
// (mGarbageScalingFactor, kLandfillTileCapacity, kMax*ForUI) are the header's
// inline wave-1 globals — referenced, not redefined. AddToGarbageSupply
// @ 0x10009abde is NOT restated here: it was spliced with the eatGarbage wave
// above. NOTE: W4's fresh decode of the jns @ 0x10009abde/0x10009abf7 reads
// "non-negative amount = complete no-op", while the spliced body adds the
// amount — flag raised for the reviewer gate; eatGarbage block D only ever
// passes POSITIVE amounts, which supports the spliced reading.
// ---------------------------------------------------------------------------

// Exact cSC4PollutionSimulator::IsGarbageUncollected @ 0x10009a6f6.
bool PollutionSimulatorSaveRecord::IsGarbageUncollected() const noexcept {
    // cmp byte ptr [rdi + 0x244], 0 ; setne al
    return field244 != 0;  // +0x244
}

// Exact cSC4PollutionSimulator::GetTotalGarbageUncollected @ 0x10009a706.
Uint32 PollutionSimulatorSaveRecord::GetTotalGarbageUncollected() const noexcept {
    return field248;  // +0x248 (mov eax, [rdi+0x248])
}

// Exact cSC4PollutionSimulator::GetAverageAirValue @ 0x10009a712.
Uint32 PollutionSimulatorSaveRecord::GetAverageAirValue() const noexcept {
    return field20C;  // +0x20c (mov eax, [rdi+0x20c])
}

// Exact cSC4PollutionSimulator::GetAverageWaterValue @ 0x10009a71e.
Uint32 PollutionSimulatorSaveRecord::GetAverageWaterValue() const noexcept {
    return field210;  // +0x210 (mov eax, [rdi+0x210])
}

// Exact cSC4PollutionSimulator::GetAverageGarbageValue @ 0x10009a72a.
Uint32 PollutionSimulatorSaveRecord::GetAverageGarbageValue() const noexcept {
    return field234;  // +0x234 (movzx eax, word ptr [rdi+0x234]) — Uint16 load
}

// Exact cSC4PollutionSimulator::GetMonthlyIncineratorCapacity @ 0x10009a8bc.
Uint32 PollutionSimulatorSaveRecord::GetMonthlyIncineratorCapacity() const noexcept {
    return field238;  // +0x238 (mov eax, [rdi+0x238])
}

// Exact cSC4PollutionSimulator::GetMonthlyWasteToEnergyCapacity @ 0x10009a8c8.
Uint32 PollutionSimulatorSaveRecord::GetMonthlyWasteToEnergyCapacity() const noexcept {
    return field23C;  // +0x23c (mov eax, [rdi+0x23c])
}

// Exact cSC4PollutionSimulator::GetRecyclingCenterEffect @ 0x10009a8d4.
const Float32* PollutionSimulatorSaveRecord::GetRecyclingCenterEffect() const noexcept {
    // lea rax, [rdi + 0x240] — returns the address of the field, not its value.
    return &field240;  // +0x240
}

// Exact cSC4PollutionSimulator::GetGarbageScalingFactor @ 0x10009a916.
Float32 PollutionSimulatorSaveRecord::GetGarbageScalingFactor() const noexcept {
    // movss xmm0, dword ptr [rip+...] ## mGarbageScalingFactor (0x100b2175c) —
    // reads the class static, not an instance field. Runtime-mutable BSS
    // global filled by ReadSimulatorTunableValues @ 0x100095c0d.
    return mGarbageScalingFactor;
}

// Exact cSC4PollutionSimulator::GetLandfillTileCapacity @ 0x10009aea6.
Sint32 PollutionSimulatorSaveRecord::GetLandfillTileCapacity() const noexcept {
    // mov eax, dword ptr [rip+...] ## kLandfillTileCapacity (0x100b21760) —
    // class static, runtime-mutable BSS global filled by
    // ReadSimulatorTunableValues @ 0x100095c4c.
    return kLandfillTileCapacity;
}

// Exact cSC4PollutionSimulator::GetGarbageCapacity @ 0x10009aeb2.
Sint32 PollutionSimulatorSaveRecord::GetGarbageCapacity() const noexcept {
    return field350;  // +0x350 (mov eax, [rdi+0x350])
}

// Exact cSC4PollutionSimulator::SetGarbageCapacity @ 0x10009aebe.
void PollutionSimulatorSaveRecord::SetGarbageCapacity(Sint32 capacity) noexcept {
    field350 = capacity;  // +0x350 (mov [rdi+0x350], esi)
}

// Exact cSC4PollutionSimulator::GetGarbageAbductedPrevMonth @ 0x10009aeca.
Uint32 PollutionSimulatorSaveRecord::GetGarbageAbductedPrevMonth() const noexcept {
    return field2C8;  // +0x2c8 (mov eax, [rdi+0x2c8]) — Uint32, unlike the Sint64 siblings
}

// Exact cSC4PollutionSimulator::GetGarbageProducedPrevMonth @ 0x10009aed6.
Sint64 PollutionSimulatorSaveRecord::GetGarbageProducedPrevMonth() const noexcept {
    return field288;  // +0x288 (mov rax, [rdi+0x288])
}

// Exact cSC4PollutionSimulator::GetGarbageImportedPrevMonth @ 0x10009aee4.
Sint64 PollutionSimulatorSaveRecord::GetGarbageImportedPrevMonth() const noexcept {
    return field290;  // +0x290 (mov rax, [rdi+0x290])
}

// Exact cSC4PollutionSimulator::GetGarbageExportedPrevMonth @ 0x10009aef2.
Sint64 PollutionSimulatorSaveRecord::GetGarbageExportedPrevMonth() const noexcept {
    return field298;  // +0x298 (mov rax, [rdi+0x298])
}

// Exact cSC4PollutionSimulator::GetGarbageRecycledPrevMonth @ 0x10009af00.
Sint64 PollutionSimulatorSaveRecord::GetGarbageRecycledPrevMonth() const noexcept {
    // cvttsd2si rax, qword ptr [rdi + 0x2a0] — Float64 slot, truncating
    // conversion to Sint64 (NaN/out-of-range yields INT64_MIN on x86).
    return static_cast<Sint64>(field2A0);  // +0x2a0
}

// Exact cSC4PollutionSimulator::GetGarbageIncineratedPrevMonth @ 0x10009af10.
Sint64 PollutionSimulatorSaveRecord::GetGarbageIncineratedPrevMonth() const noexcept {
    return field2A8;  // +0x2a8 (mov rax, [rdi+0x2a8])
}

// Exact cSC4PollutionSimulator::GetGarbageConvertedToEnergyPrevMonth @ 0x10009af1e.
Sint64 PollutionSimulatorSaveRecord::GetGarbageConvertedToEnergyPrevMonth() const noexcept {
    return field2B0;  // +0x2b0 (mov rax, [rdi+0x2b0])
}

// Exact cSC4PollutionSimulator::GetGarbageLandfilledPrevMonth @ 0x10009af2c.
Sint64 PollutionSimulatorSaveRecord::GetGarbageLandfilledPrevMonth() const noexcept {
    return field2B8;  // +0x2b8 (mov rax, [rdi+0x2b8])
}

// Exact cSC4PollutionSimulator::GetWasteToEnergyBuildingCount @ 0x10009af46.
Uint32 PollutionSimulatorSaveRecord::GetWasteToEnergyBuildingCount() const noexcept {
    // mov eax, dword ptr [rdi + 0x100] — the size slot (+0x10) of the ilist
    // based at +0xf0 (listF0), i.e. the list's element count.
    return static_cast<Uint32>(listF0.size());
}

// Exact cSC4PollutionSimulator::GetRecyclingCenterCount @ 0x10009af52.
Uint32 PollutionSimulatorSaveRecord::GetRecyclingCenterCount() const noexcept {
    // mov eax, dword ptr [rdi + 0x118] — the size slot of the ilist based at
    // +0x108 (list108).
    return static_cast<Uint32>(list108.size());
}

// Exact cSC4PollutionSimulator::GetWaterTreatmentPlantCount @ 0x10009af5e.
Uint32 PollutionSimulatorSaveRecord::GetWaterTreatmentPlantCount() const noexcept {
    // mov eax, dword ptr [rdi + 0xd0] — the size slot of the ilist based at
    // +0xc0 (listC0).
    return static_cast<Uint32>(listC0.size());
}

// Exact cSC4PollutionSimulator::GetMaxAirPollutionLevelForUI @ 0x10009af6a.
Sint32 PollutionSimulatorSaveRecord::GetMaxAirPollutionLevelForUI() const noexcept {
    // mov eax, dword ptr [rip+...] ## kMaxAirPolluteForUI (0x100b21734) —
    // runtime-mutable BSS global, ReadSimulatorTunableValues @ 0x100095b45.
    return kMaxAirPolluteForUI;
}

// Exact cSC4PollutionSimulator::GetMaxWaterPollutionLevelForUI @ 0x10009af76.
Sint32 PollutionSimulatorSaveRecord::GetMaxWaterPollutionLevelForUI() const noexcept {
    // kMaxWaterPolluteForUI @ 0x100b21738, loaded @ 0x100095b59.
    return kMaxWaterPolluteForUI;
}

// Exact cSC4PollutionSimulator::GetMaxGarbagePollutionLevelForUI @ 0x10009af82.
Sint32 PollutionSimulatorSaveRecord::GetMaxGarbagePollutionLevelForUI() const noexcept {
    // kMaxGarbagePolluteForUI @ 0x100b21730, loaded @ 0x100095b31.
    return kMaxGarbagePolluteForUI;
}

// Exact cSC4PollutionSimulator::GetMaxRadiationPollutionLevelForUI @ 0x10009af8e.
Sint32 PollutionSimulatorSaveRecord::GetMaxRadiationPollutionLevelForUI() const noexcept {
    // kMaxRadiationPolluteForUI @ 0x100b2173c, loaded @ 0x100095b6d.
    return kMaxRadiationPolluteForUI;
}

// Exact cSC4PollutionSimulator::GetRequesterSatisfactionId @ 0x10009afa6.
Uint32 PollutionSimulatorSaveRecord::GetRequesterSatisfactionId() const noexcept {
    return 0xe9a4300bu;  // mov eax, 0xe9a4300b
}

// Exact cSC4PollutionSimulator::UsesCustomPropSelector @ 0x10009afb2.
bool PollutionSimulatorSaveRecord::UsesCustomPropSelector() const noexcept {
    return false;  // xor eax, eax
}

// Exact cSC4PollutionSimulator::SelectProp @ 0x10009afba.
bool PollutionSimulatorSaveRecord::SelectProp(const std::vector<Uint32>& /*props*/,
                                              Sint32 /*a*/, Sint32 /*b*/) const noexcept {
    return false;  // xor eax, eax
}

// ---------------------------------------------------------------------------
// Supply / landfill mutators.
// ---------------------------------------------------------------------------

// Exact cSC4PollutionSimulator::GarbagePutInLandfill @ 0x10009aa1a.
void PollutionSimulatorSaveRecord::GarbagePutInLandfill(Sint32 amount) noexcept {
    // movsxd rax, esi ; add qword ptr [rdi + 0x280], rax — the Sint32
    // argument is sign-extended before the 64-bit add (no clamping).
    field280 += static_cast<Sint64>(amount);  // +0x280
}

// Exact cSC4PollutionSimulator::GetGarbageSupply @ 0x10009ac06.
//
// ODDITY (reproduced exactly): this is a CONST method in the binary
// (mangled ...K...) yet it performs a DIRTY WRITE — a negative stored supply
// is clamped to 0 in this+0x34c on read.
Sint32 PollutionSimulatorSaveRecord::GetGarbageSupply() const noexcept {
    Sint32 supply = field34C;                    // mov eax, [rdi+0x34c]
    if (supply < 0) {                            // test eax, eax ; jns @ 0x10009ac12
        const_cast<PollutionSimulatorSaveRecord*>(this)->field34C = 0;
                                                 // mov dword ptr [rdi+0x34c], 0 @ 0x10009ac17
        return 0;                                // xor eax, eax
    }
    return supply;
}

// Exact cSC4PollutionSimulator::GetPollutionGrid @ 0x10009ac22.
//
// cmp esi, 3 ; mov eax, esi — the ONLY gate is == 3 (cmove to the +0x78
// array); every other type, including out-of-range values, indexes the +0x98
// array at 8*type with a ZERO-EXTENDED index and NO bounds check. Type 4
// reads this+0xb8 (past gridRefs98); negative types read wild (index wraps
// through 32-bit zero-extension). Reproduced unchecked — do not add a guard.
SimSerializableRef PollutionSimulatorSaveRecord::GetPollutionGrid(
    PollutionType type) const noexcept {
    const Uint32 index = static_cast<Uint32>(type);  // mov eax, esi (zero-extend)
    if (type == PollutionType::Radiation) {          // cmp esi, 3 ; cmove @ 0x10009ac36
        // lea rcx, [rdi + 8*rax + 0x78] — radiation lives in the +0x78 array.
        return gridRefs78[index];
    }
    // lea rax, [rdi + 8*rax + 0x98] — unchecked for every other type value.
    return gridRefs98[index];
}

// ---------------------------------------------------------------------------
// Worst-tract getters.
// ---------------------------------------------------------------------------

// Exact cSC4PollutionSimulator::GetWorstAirTract @ 0x10009ae46.
void PollutionSimulatorSaveRecord::GetWorstAirTract(Sint32& outX, Sint32& outZ,
                                                    Sint32& outValue) const noexcept {
    outX = field330;        // movzx eax, word ptr [rdi + 0x330] (Uint16)
    outZ = field332;        // movzx eax, word ptr [rdi + 0x332] (Uint16)
    outValue = field334;    // mov eax, dword ptr [rdi + 0x334]
}

// Exact cSC4PollutionSimulator::GetWorstWaterTract @ 0x10009ae66.
void PollutionSimulatorSaveRecord::GetWorstWaterTract(Sint32& outX, Sint32& outZ,
                                                      Sint32& outValue) const noexcept {
    outX = field338;        // movzx eax, word ptr [rdi + 0x338] (Uint16)
    outZ = field33A;        // movzx eax, word ptr [rdi + 0x33a] (Uint16)
    outValue = field33C;    // mov eax, dword ptr [rdi + 0x33c]
}

// Exact cSC4PollutionSimulator::GetWorstGarbageTract @ 0x10009ae86.
void PollutionSimulatorSaveRecord::GetWorstGarbageTract(Sint32& outX, Sint32& outZ,
                                                        Sint32& outValue) const noexcept {
    outX = field340;        // movzx eax, word ptr [rdi + 0x340] (Uint16)
    outZ = field342;        // movzx eax, word ptr [rdi + 0x342] (Uint16)
    outValue = field344;    // mov eax, dword ptr [rdi + 0x344]
}

// ---------------------------------------------------------------------------
// GZCOM boilerplate (runtime lifecycle, not part of the save serialization).
// ---------------------------------------------------------------------------

// Exact cSC4PollutionSimulator::AddRef @ 0x100094d10.
Uint32 PollutionSimulatorSaveRecord::AddRef() noexcept {
    // mov eax, [rdi+0x24] ; inc eax ; mov [rdi+0x24], eax — refcount at +0x24.
    return ++refCount24;
}

// Exact cSC4PollutionSimulator::Release @ 0x100094d48.
// ODDITY (reproduced exactly): refcount < 2 goes to FinalRelease and returns
// 0 WITHOUT deleting this; the >= 2 path stores and returns the DECREMENTED
// count. There is no delete-expression anywhere in this body.
Uint32 PollutionSimulatorSaveRecord::Release() noexcept {
    Uint32 count = refCount24;             // mov eax, [rdi+0x24]
    if (count < 2) {                       // cmp eax, 2 ; jl @ 0x100094d56
        FinalRelease();                    // call FinalRelease @ 0x100094d64
        return 0;                          // xor eax, eax
    }
    return refCount24 = count - 1;         // dec eax ; mov [rdi+0x24], eax
}

// stub: real body @ 0x100094d64 not in wave 1 (see the header note).
void PollutionSimulatorSaveRecord::FinalRelease() noexcept {
    /* stub: real body @ 0x100094d64 not in wave 1 */
}

// Exact cSC4PollutionSimulator::QueryInterface @ 0x100094c90.
// rid chain verbatim: rids <= 0xe4fda3d3 test 0x877181ec (self) and
// 0x89c498be (this+0x10); rids > it test 0xe4fda3d4 (this+0x18), 0x1 (self),
// and 0x452294aa (this+0x8). Success stores the pointer, AddRefs, returns 1.
bool PollutionSimulatorSaveRecord::QueryInterface(Uint32 rid, void** out) noexcept {
    void* self = this;
    if (rid <= 0xe4fda3d3u) {              // cmp esi, 0xe4fda3d3 ; jle @ 0x100094c9c
        if (rid == 0x877181ecu) { *out = self; AddRef(); return true; }   // @ 0x100094cb9
        if (rid == 0x89c498beu) { *out = static_cast<Uint8*>(self) + 0x10; AddRef(); return true; } // @ 0x100094cc1
        return false;                      // jne @ 0x100094cc7
    }
    if (rid == 0xe4fda3d4u) { *out = static_cast<Uint8*>(self) + 0x18; AddRef(); return true; } // @ 0x100094cd4
    if (rid == 0x1u) { *out = self; AddRef(); return true; }              // @ 0x100094ca6
    if (rid == 0x452294aau) { *out = static_cast<Uint8*>(self) + 0x8; AddRef(); return true; }  // @ 0x100094cab
    return false;                         // jne @ 0x100094cb1
}

// cSC4PollutionSimulator::~cSC4PollutionSimulator @ 0x100094a7c (532 B):
// NOT reconstructed — pure GZCOM member teardown (the record model's RAII
// members cover destruction); flagged for a follow-up ticket if COM
// lifecycle is ever modeled. No save-record counterpart.

// ---------------------------------------------------------------------------
// Medium query methods.
// ---------------------------------------------------------------------------

// Exact cSC4PollutionSimulator::GetGarbageBuildings @ 0x10009ac48.
//
// Binary walks the three ilists based at +0xd8 (listD8), +0xf0 (listF0) and
// +0x108 (list108) FORWARD (start at the sentinel's next — [this+0xe0] /
// [this+0xf8] / [this+0x110] — advance via node+0x8, terminate at the list
// base) and TAIL-INSERTS each occupant into the destination list (the link
// code at 0x10009ac86-0x10009ac9f splices the new node as the sentinel's new
// prev, i.e. push_back; node layout +0 prev / +8 next / +0x10 value, size at
// list+0x10). Net destination order: listD8 ++ listF0 ++ list108, each in
// forward order.
//
// Per node the binary first calls the occupant's vtable slot +0x8 (GZCOM
// AddRef, result discarded) @ 0x10009ac70 — a live-object refcount side
// effect with no counterpart in the save-record model (SimSerializableRef is
// a value copy); reproduced as a comment, not an AddRef call.
void PollutionSimulatorSaveRecord::GetGarbageBuildings(OccupantRefList& out) const {
    for (const SimSerializableRef& ref : listD8) {
        // occupant->AddRef() @ 0x10009ac70 (vtable +0x8, result discarded).
        out.push_back(ref);   // allocate node 0x18, copy value, tail-splice, ++size
    }
    for (const SimSerializableRef& ref : listF0) {
        // occupant->AddRef() @ 0x10009acc3 (vtable +0x8, result discarded).
        out.push_back(ref);   // same insert sequence @ 0x10009acd9-0x10009acf2
    }
    for (const SimSerializableRef& ref : list108) {
        // occupant->AddRef() @ 0x10009ad16 (vtable +0x8, result discarded).
        out.push_back(ref);   // same insert sequence @ 0x10009ad2c-0x10009ad45
    }
}

// Exact cSC4PollutionSimulator::GetRequestedShare @ 0x10009ad5c.
//
// rect field order (SC4Rect<int>): +0x00 minX, +0x04 minZ, +0x08 maxX,
// +0x0c maxZ. The double loop is INCLUSIVE on both axes: entry gate is
// `minX > maxX` / `minZ > maxZ` skip (jle @ 0x10009ad7a, jg @ 0x10009ada3)
// and the back-edge is `old < max` (jl @ 0x10009add1 / 0x10009ade6), so the
// body runs for every tile in [minX..maxX] x [minZ..maxZ] — a rect is empty
// only when an axis is inverted.
//
// Per tile the binary dispatches the simulator's OWN vtable slot +0x48
// (x, y, Uint32& out) @ 0x10009adb7 — the GetGarbageValue slot, the same
// dispatch IsGarbageAccumulated @ 0x10009a6e0 makes — and adds `out` to the
// accumulator (the virtual's return value is discarded). The save-record
// model has no vtable, so the tile query is injected as a callback and the
// class-static mpPropRequestCurve (BSS pointer @ 0x100b21770, set by
// ReadSimulatorTunableValues @ 0x1000959e5) is injected as a curve reference.
//
// ODDITY (reproduced, not guarded): area = (maxX+1-minX) * (maxZ+1-minZ);
// an EXACTLY-empty rect (minX == maxX+1 or minZ == maxZ+1) makes area 0 and
// the binary executes `div r9d` against zero — a #DE fault. There is no
// guard in the bytes. (An inverted axis by 2+ wraps to a huge unsigned
// divisor and silently yields quotient 0.)
Sint32 PollutionSimulatorSaveRecord::GetRequestedShare(const PollutionIntRect& rect,
                                                        const cSC4ResponseCurve& propRequestCurve,
                                                        PollutionTileGarbageValue tileGarbageValue,
                                                        void* tileContext) const noexcept {
    Uint32 sum = 0;  // xor eax, eax @ 0x10009ad84/0x10009ad93

    if (rect.minX <= rect.maxX) {                     // cmp r12d, ebx ; jle @ 0x10009ad77
        for (Sint32 x = rect.minX; x <= rect.maxX; ++x) {   // back-edge jl @ 0x10009ade6
            if (rect.minZ > rect.maxZ) {               // cmp r13d, r9d ; jg @ 0x10009ada3
                continue;                              //   (binary falls to the outer back-edge)
            }
            for (Sint32 z = rect.minZ; z <= rect.maxZ; ++z) { // back-edge jl @ 0x10009add1
                Uint32 value = 0;
                // call qword ptr [rax + 0x48] @ 0x10009adb7 — return discarded.
                (void)tileGarbageValue(tileContext, static_cast<Uint32>(x),
                                       static_cast<Uint32>(z), value);
                sum += value;                          // add eax, [rbp-0x30] @ 0x10009adc0
            }
        }
    }

    // inc ebx ; sub ebx, r12d — (maxX + 1) - minX, wrapping 32-bit.
    const Uint32 width = static_cast<Uint32>(rect.maxX + 1) -
                         static_cast<Uint32>(rect.minX);          // @ 0x10009adeb
    // inc r9d ; sub r9d, [rdx] — (maxZ + 1) - minZ.
    const Uint32 height = static_cast<Uint32>(rect.maxZ + 1) -
                          static_cast<Uint32>(rect.minZ);         // @ 0x10009adf0
    const Uint32 area = width * height;                            // imul r9d, ebx @ 0x10009adf6

    // div r9d @ 0x10009adfc — UNSIGNED division of the zero-extended sum
    // (accumulator was built from Uint32 adds). area == 0 -> #DE (see above).
    const Uint32 average = sum / area;

    // cvttsi2ss xmm0, rax @ 0x10009adff (i64 quotient, value < 2^32 here).
    Float32 normalized = static_cast<Float32>(static_cast<Sint64>(average));
    // divss xmm0, [0x100a165ac] @ 0x10009ae04 — fresh-dumped 32767.0f
    // (bits 0x46fffe00). NOT 32768, NOT an int division constant.
    normalized = normalized / 32767.0f;

    // cSC4ResponseCurve::GetOutputValue(float) @ 0x1000ac4f6 via the
    // mpPropRequestCurve static @ 0x100b21770.
    const Float32 curveOutput = propRequestCurve.GetOutputValue(normalized);

    // addss xmm0, [0x100a158b8] @ 0x10009ae18 — fresh-dumped 12582912.0f
    // (0x4b400000 = 1.5*2^23), then movd eax ; add eax, 0xb4c00000 @
    // 0x10009ae20/0x10009ae24: the classic float->int bit trick. Adding the
    // magic forces round-to-nearest with ULP 1.0, so the low mantissa bits
    // hold round(curveOutput) and + 0xb4c00000 (== -bits(12582912.0f) mod
    // 2^32) cancels the offset, yielding the integer in eax. Reproduced
    // byte-for-byte — do NOT replace with a plain cast.
    const Float32 biased = curveOutput + 12582912.0f;
    Sint32 result;
    std::memcpy(&result, &biased, sizeof(result));   // movd eax, xmm0
    result += static_cast<Sint32>(0xb4c00000u);      // add eax, 0xb4c00000
    return result;
}

// ===========================================================================
// cSC4ResidentialSimulator (Read 0x100204c4e / Write 0x1002047d4)
// ===========================================================================
namespace {

bool ReadDepartmentData(cIGZIStream& stream, Uint16 version,
                        ResidentialDepartmentData& d) noexcept {
    Float32 raw = 0.0f;
    auto readScaledFloat = [&](Float32& out, bool scaleBelow6) noexcept {
        stream >> raw;
        const bool scale = scaleBelow6 ? (version < 6) : (version < 5);
        out = scale ? raw * 0.01f : raw;
    };
    readScaledFloat(d.field00, false);
    readScaledFloat(d.field04, false);
    readScaledFloat(d.field08, false);
    stream >> d.field0C;
    readScaledFloat(d.field10, true);
    stream >> d.field78 >> d.field7C;
    if (!ReadRef(stream, d.ref80) || !ReadRef(stream, d.ref88)) {
        return false;
    }
    readScaledFloat(d.field90, false);
    stream >> d.field94 >> d.field98;
    return stream.Good();
}

void WriteDepartmentData(cIGZOStream& stream, const ResidentialDepartmentData& d) {
    // Write always emits version 7, so the legacy x0.01 forms never apply.
    stream << d.field00 << d.field04 << d.field08 << d.field0C << d.field10 << d.field78
           << d.field7C;
    WriteRef(stream, d.ref80);
    WriteRef(stream, d.ref88);
    stream << d.field90 << d.field94 << d.field98;
}

bool ReadRefVector(cIGZIStream& stream, std::vector<SimSerializableRef>& vec) noexcept {
    Uint32 count = 0;
    stream >> count;
    if (!stream.Good()) {
        return false;
    }
    vec.clear();
    vec.reserve(count);
    for (Uint32 i = 0; i < count; ++i) {
        SimSerializableRef ref;
        if (!ReadRef(stream, ref)) {
            return false;
        }
        vec.push_back(ref);
    }
    return true;
}

void WriteRefVector(cIGZOStream& stream, const std::vector<SimSerializableRef>& vec) {
    stream << static_cast<Uint32>(vec.size());
    for (const SimSerializableRef& ref : vec) {
        WriteRef(stream, ref);
    }
}

bool ReadListVector(cIGZIStream& stream, std::vector<OccupantRefList>& vec) noexcept {
    Uint32 count = 0;
    stream >> count;
    if (!stream.Good()) {
        return false;
    }
    vec.assign(count, OccupantRefList{});
    for (OccupantRefList& list : vec) {
        if (!ReadRefList(stream, list)) {
            return false;
        }
    }
    return true;
}

void WriteListVector(cIGZOStream& stream, const std::vector<OccupantRefList>& vec) {
    stream << static_cast<Uint32>(vec.size());
    for (const OccupantRefList& list : vec) {
        WriteRefList(stream, list);
    }
}

} // namespace

bool ResidentialSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good()) {
        return false;
    }
    stream >> field60;
    if (!ReadRef(stream, gridRef68) || !ReadRef(stream, gridRef70) ||
        !ReadRef(stream, gridRef78)) {
        return false;
    }
    stream >> field80;
    for (SimSerializableRef& ref : gridRefs88) {
        if (!ReadRef(stream, ref)) {
            return false;
        }
    }
    if (!ReadRef(stream, gridRefA0)) {
        return false;
    }
    if (!ReadRefVector(stream, gridRefsA8) || !ReadRefVector(stream, gridRefsC0)) {
        return false;
    }
    if (!ReadRef(stream, gridRefD8) || !ReadRef(stream, gridRefE0)) {
        return false;
    }
    stream >> fieldE8;
    for (SimSerializableRef& ref : gridRefsF0) {
        if (!ReadRef(stream, ref)) {
            return false;
        }
    }
    for (SimSerializableRef& ref : gridRefs108) {
        if (!ReadRef(stream, ref)) {
            return false;
        }
    }
    stream >> field124 >> field128 >> field120 >> field12C >> field130;
    if (!stream.Good()) {
        return false;
    }
    if (!ReadDepartmentData(stream, version, department160) ||
        !ReadDepartmentData(stream, version, department200)) {
        return false;
    }
    if (!ReadRefList(stream, list2D0) || !ReadRefList(stream, list2E8) ||
        !ReadRefList(stream, list300)) {
        return false;
    }
    if (!ReadListVector(stream, lists318) || !ReadListVector(stream, lists330)) {
        return false;
    }
    if (!ReadRefList(stream, list348)) {
        return false;
    }
    if (version >= 3) {
        if (!ReadScanningIterator(stream, scanningIterator)) {
            return false;
        }
        stream >> field590;
    }
    if (version > 3) {
        stream >> field134;
    }
    if (version >= 7) {
        // operator>>(stream, bool) @ 0x1000ce510
        stream >> field648 >> field649;
    }
    // The binary then applies runtime fixups (legacy scanning-iterator state,
    // and +0x120 += 12 when it holds 1..3); those mutate live state only and
    // are not part of the serialized form, so they are not applied here.
    return stream.Good();
}

bool ResidentialSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{7} << field60;
    WriteRef(stream, gridRef68);
    WriteRef(stream, gridRef70);
    WriteRef(stream, gridRef78);
    stream << field80;
    for (const SimSerializableRef& ref : gridRefs88) {
        WriteRef(stream, ref);
    }
    WriteRef(stream, gridRefA0);
    WriteRefVector(stream, gridRefsA8);
    WriteRefVector(stream, gridRefsC0);
    WriteRef(stream, gridRefD8);
    WriteRef(stream, gridRefE0);
    stream << fieldE8;
    for (const SimSerializableRef& ref : gridRefsF0) {
        WriteRef(stream, ref);
    }
    for (const SimSerializableRef& ref : gridRefs108) {
        WriteRef(stream, ref);
    }
    stream << field124 << field128 << field120 << field12C << field130;
    WriteDepartmentData(stream, department160);
    WriteDepartmentData(stream, department200);
    WriteRefList(stream, list2D0);
    WriteRefList(stream, list2E8);
    WriteRefList(stream, list300);
    WriteListVector(stream, lists318);
    WriteListVector(stream, lists330);
    WriteRefList(stream, list348);
    WriteScanningIterator(stream, scanningIterator);
    stream << field590 << field134 << field648 << field649;
    return true;
}

bool ResidentialSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

bool ResidentialTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    field520 = static_cast<Uint32>(exemplar.GetInt(0x8a891421u, field520));
    field524 = static_cast<Uint32>(exemplar.GetInt(0x8a89142au, field524));
    field560 = exemplar.GetFloat(0x4aa84161u, field560);
    field564 = exemplar.GetFloat(0x2aa84164u, field564);
    field568 = exemplar.GetFloat(0x6a9f6eddu, field568);
    return LoadProps(exemplar, properties);
}

// ===========================================================================
// cSC4TrafficSimulator (Read 0x10007b550 / Write 0x10007a5f8)
// Complete decompilation of the v19..v41 transit-graph serialization format
// (zero opaque bytes).
// ===========================================================================
namespace {

// BoundaryIterator operator>> @ 0x1000ce99f, operator<< @ 0x1000cea25.
bool ReadBoundaryIterator(cIGZIStream& stream, BoundaryIteratorState& it) noexcept {
    stream >> it.field0 >> it.field1 >> it.field4 >> it.field8 >> it.fieldC >>
        it.rect10[0] >> it.rect10[1] >> it.rect10[2] >> it.rect10[3];
    return stream.Good();
}

void WriteBoundaryIterator(cIGZOStream& stream, const BoundaryIteratorState& it) {
    stream << it.field0 << it.field1 << it.field4 << it.field8 << it.fieldC <<
        it.rect10[0] << it.rect10[1] << it.rect10[2] << it.rect10[3];
}

// operator>>(cIGZIStream&, vector<uint8>&) @ 0x1000cf09e (cap 0xf423f)
// and operator<< @ 0x1000cf12e.
bool ReadUint8Vector(cIGZIStream& stream, std::vector<Uint8>& vec) noexcept {
    Uint32 count = 0;
    stream >> count;
    if (!stream.Good() || count > 0xf423fu) {
        return false;
    }
    vec.assign(count, 0);
    for (Uint8& b : vec) {
        stream >> b;
    }
    return stream.Good();
}

void WriteUint8Vector(cIGZOStream& stream, const std::vector<Uint8>& vec) {
    stream << static_cast<Uint32>(vec.size());
    for (Uint8 b : vec) {
        stream << b;
    }
}

int VariantDbElementSize(Uint32 dbBaseType) noexcept {
    // Exact element byte widths from cGZPersistDBSerialRecord::GetFieldVariant
    // jump tables @ 0x10038c958 / 0x10038c98c.
    switch (dbBaseType) {
        case 0x00u: return 0; // Void
        case 0x01u: // Uint8
        case 0x05u: // Sint8
        case 0x0bu: // Bool
        case 0x0cu: // Char (scalar)
        case 0x11u: // Char (array)
            return 1;
        case 0x02u: // Uint16
        case 0x06u: // Sint16
        case 0x10u: // RZChar (2-byte array)
            return 2;
        case 0x03u: // Uint32
        case 0x07u: // Sint32
        case 0x09u: // Float32
            return 4;
        case 0x04u: // Uint64
        case 0x08u: // Sint64
        case 0x0au: // Float64
            return 8;
        default:
            return -1;
    }
}

// cSCPropertyHolder::Read @ 0x100049d3c / Write @ 0x100049cc4.
bool ReadSerializedPropertyHolder(cIGZIStream& stream,
                                  SerializedPropertyHolder& holder) noexcept {
    stream >> holder.version;
    if (!stream.Good() || holder.version != 2) {
        return false;
    }
    Uint32 propCount = 0;
    stream >> propCount;
    if (!stream.Good()) {
        return false;
    }
    holder.properties.clear();
    holder.properties.reserve(propCount);
    for (Uint32 i = 0; i < propCount; ++i) {
        SerializedPropertyEntry entry;
        stream >> entry.holderPropId >> entry.propertyId >> entry.variantTag;
        if (!stream.Good()) {
            return false;
        }
        if (entry.variantTag == 0xa9bee9acu) {
            if (!ReadRef(stream, entry.unknownRef)) {
                return false;
            }
        } else {
            stream >> entry.dbType;
            if (!stream.Good()) {
                return false;
            }
            if ((entry.dbType & 0x8000u) != 0) {
                const Uint32 baseType = entry.dbType - 0x8000u;
                if (baseType == 0x0cu) {
                    if (!stream.ReadGZString(entry.stringValue)) {
                        return false;
                    }
                } else {
                    const int elemSize = VariantDbElementSize(baseType);
                    if (elemSize <= 0) {
                        return false;
                    }
                    stream >> entry.arrayCount;
                    if (!stream.Good()) {
                        return false;
                    }
                    const std::size_t totalBytes =
                        static_cast<std::size_t>(entry.arrayCount) *
                        static_cast<std::size_t>(elemSize);
                    entry.rawValueBytes.resize(totalBytes);
                    if (!stream.ReadBytes(entry.rawValueBytes.data(), totalBytes)) {
                        return false;
                    }
                }
            } else {
                const int elemSize = VariantDbElementSize(entry.dbType);
                if (elemSize < 0) {
                    return false;
                }
                entry.rawValueBytes.resize(static_cast<std::size_t>(elemSize));
                if (elemSize > 0 &&
                    !stream.ReadBytes(entry.rawValueBytes.data(),
                                      static_cast<std::size_t>(elemSize))) {
                    return false;
                }
            }
        }
        holder.properties.push_back(std::move(entry));
    }
    return stream.Good();
}

void WriteSerializedPropertyHolder(cIGZOStream& stream,
                                   const SerializedPropertyHolder& holder) {
    stream << Uint16{2} << static_cast<Uint32>(holder.properties.size());
    for (const SerializedPropertyEntry& entry : holder.properties) {
        stream << entry.holderPropId << entry.propertyId << entry.variantTag;
        if (entry.variantTag == 0xa9bee9acu) {
            WriteRef(stream, entry.unknownRef);
        } else {
            stream << entry.dbType;
            if ((entry.dbType & 0x8000u) != 0) {
                const Uint32 baseType = entry.dbType - 0x8000u;
                if (baseType == 0x0cu) {
                    stream.WriteGZString(entry.stringValue);
                } else {
                    stream << entry.arrayCount;
                    stream.WriteBytes(entry.rawValueBytes.data(),
                                      entry.rawValueBytes.size());
                }
            } else {
                stream.WriteBytes(entry.rawValueBytes.data(), entry.rawValueBytes.size());
            }
        }
    }
}

Uint16 LegacyFloatToTrafficUint16(Float32 v) noexcept {
    // Exact float-magic rounding at 0x10007b870: add 12582912.0f (0x4b400000),
    // add 0xb4c00000 to bit pattern, clamp to 0xffff.
    const Float32 magic = v + 12582912.0f;
    Uint32 bits = 0;
    std::memcpy(&bits, &magic, sizeof(Uint32));
    bits += 0xb4c00000u;
    return bits >= 0xffffu ? Uint16{0xffffu} : static_cast<Uint16>(bits);
}

bool ReadUint16Set(cIGZIStream& stream, std::vector<Uint16>& out) noexcept {
    Uint32 count = 0;
    stream >> count;
    if (!stream.Good()) {
        return false;
    }
    out.assign(count, 0);
    for (Uint16& v : out) {
        stream >> v;
    }
    return stream.Good();
}

void WriteUint16Set(cIGZOStream& stream, const std::vector<Uint16>& vec) {
    stream << static_cast<Uint32>(vec.size());
    for (Uint16 v : vec) {
        stream << v;
    }
}

bool WriteTrafficAtVersion(const TrafficSimulatorSaveRecord& rec, cIGZOStream& stream,
                           Uint16 writeVersion) {
    stream << writeVersion;
    if (writeVersion > 0x17u) {
        stream << rec.countCC;
        if (writeVersion >= 0x25u) {
            stream << rec.countC8;
        }
        if (writeVersion > 0x1bu) {
            stream << rec.countE4 << rec.count60;
        }
    }
    WriteRef(stream, rec.gridRef48);
    WriteRef(stream, rec.gridRef50);
    WriteRef(stream, rec.gridRef58);
    WriteRef(stream, rec.gridRef60);
    WriteRef(stream, rec.gridRef70);
    WriteRef(stream, rec.gridRef78);

    stream << rec.byteGridRows << rec.byteGridCols;
    for (Uint8 b : rec.byteGrid) {
        stream << b;
    }

    WriteRefVector(stream, rec.floatGridRefs298);
    WriteRef(stream, rec.gridRef2B0);
    WriteRef(stream, rec.gridRef2B8);

    stream << static_cast<Uint32>(rec.edgeDensities.size());
    for (const TrafficEdgeDensityRecord& ed : rec.edgeDensities) {
        stream << ed.cellKey;
        for (Uint16 v : ed.matrixA) {
            stream << v;
        }
        stream << ed.field20;
        for (Uint16 v : ed.matrixB) {
            stream << v;
        }
    }

    for (std::size_t i = 0; i < 3; ++i) {
        stream << rec.field32C[i] << rec.field338[i] << rec.field344[i];
    }

    WriteRef(stream, rec.gridRef888);
    WriteRef(stream, rec.gridRef890);
    stream << rec.field898;

    stream << static_cast<Uint32>(rec.transitSwitches.size());
    for (const TransitSwitchRecord& ts : rec.transitSwitches) {
        stream << ts.key;
        WriteRef(stream, ts.occupantRef);
        stream << ts.field20 << ts.field24 << ts.field28;
        if (writeVersion <= 0x1cu) {
            WriteUint8Vector(stream, ts.legacyBytes);
        }
        stream << ts.field2D << ts.field2E << ts.field2F << ts.field30 << ts.field34;
        if (writeVersion >= 0x1du) {
            stream << static_cast<Uint32>(ts.rules.size());
            for (const TransitSwitchRuleRecord& rule : ts.rules) {
                stream << rule.flag0;
                for (bool f : rule.flags1to4) {
                    stream << f;
                }
                stream << rule.travelTypeIn << rule.travelTypeOut;
            }
            if (writeVersion >= 0x21u) {
                stream << ts.field54 << ts.field58;
            }
        }
    }

    WriteRef(stream, rec.ref8D0);
    WriteRef(stream, rec.ref8D8);
    stream << rec.field940 << rec.field948;
    if (writeVersion >= 0x29u) {
        stream << rec.field94C;
    }
    stream << rec.field950 << rec.field958;
    if (writeVersion >= 0x29u) {
        stream << rec.field960;
    }
    stream << rec.field968 << rec.field96C << rec.field970 << rec.field974 << rec.field978
           << rec.field97C;

    WriteUint16Set(stream, rec.set980);
    WriteUint16Set(stream, rec.set9A8);
    if (writeVersion >= 0x1au) {
        stream << rec.field9D0 << rec.field9D4 << rec.field9D8;
        WriteUint16Set(stream, rec.set9E0);
        WriteUint16Set(stream, rec.setA08);
    }

    stream << rec.fieldA30;
    WriteScanningIterator(stream, rec.scanningIterator);
    stream << rec.fieldA40 << rec.fieldA44 << rec.fieldA48 << rec.fieldA4C << rec.fieldA50
           << rec.fieldB78 << rec.fieldB7C << rec.fieldB80 << rec.fieldB58 << rec.fieldB5A;
    WriteBoundaryIterator(stream, rec.boundaryIterator);
    stream << rec.fieldAC0 << rec.fieldAA9 << rec.fieldAA4 << rec.fieldA70 << rec.fieldA74
           << rec.fieldABC;
    for (Uint32 v : rec.fieldA94) {
        stream << v;
    }
    stream << rec.fieldAA8 << rec.fieldA90;
    for (Sint32 v : rec.rectAAC) {
        stream << v;
    }
    for (const auto& row : rec.matrixAD4) {
        for (Uint32 v : row) {
            stream << v;
        }
    }
    stream << rec.fieldB40 << rec.fieldB41 << rec.fieldB68 << rec.fieldB6C;
    if (writeVersion >= 0x19u) {
        stream << rec.fieldB70;
    }
    stream << rec.fieldB84 << rec.fieldB88 << rec.fieldB8C << rec.fieldB90;
    if (writeVersion > 0x18u) {
        stream << rec.fieldB94 << rec.fieldB98 << rec.fieldB9C << rec.fieldBA0
               << rec.fieldBA4 << rec.fieldBA8 << rec.fieldBAC;
        if (writeVersion >= 0x28u) {
            stream << rec.fieldBB0 << rec.fieldBB4 << rec.fieldBB8 << rec.fieldBBC
                   << rec.fieldBC0 << rec.fieldBC4;
        }
    }

    for (const TrafficCommuteBlock& blk : rec.commuteBlocks) {
        for (const TrafficStatEntry20& st : blk.primaryStats) {
            stream << st.field00 << st.field04 << st.field08 << st.field0C << st.field10;
        }
        if (writeVersion >= 0x25u) {
            for (const TrafficStatEntry20& st : blk.extraStats) {
                stream << st.field00 << st.field04 << st.field08 << st.field0C << st.field10;
            }
        }
        for (Uint16 w : blk.primaryWords) {
            stream << w;
        }
        if (writeVersion >= 0x25u) {
            for (Uint16 w : blk.extraWords) {
                stream << w;
            }
        }
        stream << blk.field134;
        if (writeVersion >= 0x22u) {
            stream << static_cast<Sint32>(blk.subStats.size());
        }
        for (const TrafficSubStatEntry20& ss : blk.subStats) {
            stream << ss.field00 << ss.field04 << ss.field08 << ss.field0C << ss.field10;
        }
        stream << blk.field228 << blk.field22C << blk.field230 << blk.field234;
        if (writeVersion > 0x14u) {
            stream << blk.field238;
        }
    }

    stream << static_cast<Uint32>(rec.tunnels.size());
    for (const TrafficTunnelInfoRecord& tun : rec.tunnels) {
        stream << tun.key << tun.field14 << tun.field15 << tun.field16 << tun.field3C;
        for (Float32 f : tun.travelTimeFloats) {
            stream << f;
        }
    }

    stream << rec.field58C << rec.field870 << rec.field874;
    if (writeVersion >= 0x15u) {
        stream << rec.field878 << rec.field87C;
    }
    for (Uint32 v : rec.array908) {
        stream << v;
    }
    stream << static_cast<Uint32>(rec.array31C.size());
    for (Uint32 v : rec.array31C) {
        stream << v;
    }

    stream << static_cast<Uint32>(rec.propertyHolders.size());
    for (const auto& [key, holder] : rec.propertyHolders) {
        stream << key;
        WriteSerializedPropertyHolder(stream, holder);
    }

    if (writeVersion >= 0x1bu) {
        for (const auto& [first, second] : rec.pairs590_5A8) {
            stream << first << second;
        }
        if (writeVersion >= 0x27u) {
            stream << rec.field5C0;
        }
        if (writeVersion >= 0x1eu) {
            stream << static_cast<Uint32>(rec.ferryTerminals.size());
            for (const FerryTerminalRecord& ft : rec.ferryTerminals) {
                if (writeVersion <= 0x1fu) {
                    stream << ft.legacyWord;
                }
                for (Uint8 b : ft.bytes10to15) {
                    stream << b;
                }
                stream << ft.field18Byte << ft.field1C << ft.field50 << ft.field52;
                if (writeVersion >= 0x26u) {
                    stream << ft.field54 << ft.field55;
                }
                stream << static_cast<Uint32>(ft.terminalLinks.size());
                for (const FerryTerminalLinkRecord& link : ft.terminalLinks) {
                    stream << link.field18 << link.field12 << link.field14 << link.field10;
                    WriteUint8Vector(stream, link.pathBytes);
                    if (writeVersion >= 0x24u) {
                        stream << link.field38 << link.field3C << link.field3A
                               << link.field3E;
                    }
                }
                stream << static_cast<Uint32>(ft.neighborLinks.size());
                for (const FerryNeighborConnectionLinkRecord& link : ft.neighborLinks) {
                    stream << link.field18 << link.field10 << link.field12 << link.field14
                           << link.field16;
                    WriteUint8Vector(stream, link.pathBytesA);
                    if (writeVersion >= 0x1fu) {
                        WriteUint8Vector(stream, link.pathBytesB);
                        if (writeVersion >= 0x24u) {
                            stream << link.field50 << link.field54 << link.field52
                                   << link.field56;
                        }
                    }
                }
            }
        }
    }
    return true;
}

} // namespace

bool TrafficSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version < 0x13u) {
        return false;
    }
    countCC = 8;
    countC8 = 1;
    countE4 = 4;
    count60 = 7;
    if (version > 0x17u) {
        stream >> countCC;
        if (version >= 0x25u) {
            stream >> countC8;
        }
        if (version > 0x1bu) {
            stream >> countE4 >> count60;
        }
    }
    if (!stream.Good() || countCC < 0 || countC8 < 0 || countE4 < 0 || count60 < 0) {
        return false;
    }

    if (!ReadRef(stream, gridRef48) || !ReadRef(stream, gridRef50) ||
        !ReadRef(stream, gridRef58) || !ReadRef(stream, gridRef60) ||
        !ReadRef(stream, gridRef70) || !ReadRef(stream, gridRef78)) {
        return false;
    }

    stream >> byteGridRows >> byteGridCols;
    if (!stream.Good()) {
        return false;
    }
    const std::size_t gridCells =
        static_cast<std::size_t>(byteGridRows) * static_cast<std::size_t>(byteGridCols);
    if (gridCells > stream.Remaining()) {
        return false;
    }
    byteGrid.assign(gridCells, 0);
    for (Uint8& b : byteGrid) {
        stream >> b;
    }

    if (!ReadRefVector(stream, floatGridRefs298) ||
        !ReadRef(stream, gridRef2B0) || !ReadRef(stream, gridRef2B8)) {
        return false;
    }

    Uint32 edgeDensityCount = 0;
    stream >> edgeDensityCount;
    if (!stream.Good()) {
        return false;
    }
    edgeDensities.clear();
    edgeDensities.reserve(edgeDensityCount);
    for (Uint32 i = 0; i < edgeDensityCount; ++i) {
        TrafficEdgeDensityRecord ed;
        stream >> ed.cellKey;
        if (version <= 0x13u) {
            for (Uint16& w : ed.matrixA) {
                Float32 f = 0.0f;
                stream >> f;
                w = LegacyFloatToTrafficUint16(f);
            }
            {
                Float32 f = 0.0f;
                stream >> f;
                ed.field20 = LegacyFloatToTrafficUint16(f);
            }
            for (Uint16& w : ed.matrixB) {
                Float32 f = 0.0f;
                stream >> f;
                w = LegacyFloatToTrafficUint16(f);
            }
            stream >> ed.legacyExtraFloat;
        } else {
            for (Uint16& w : ed.matrixA) {
                stream >> w;
            }
            stream >> ed.field20;
            for (Uint16& w : ed.matrixB) {
                stream >> w;
            }
        }
        if (!stream.Good()) {
            return false;
        }
        edgeDensities.push_back(ed);
    }

    for (std::size_t i = 0; i < 3; ++i) {
        stream >> field32C[i] >> field338[i] >> field344[i];
    }

    if (!ReadRef(stream, gridRef888) || !ReadRef(stream, gridRef890)) {
        return false;
    }
    Uint32 transitSwitchCount = 0;
    stream >> field898 >> transitSwitchCount;
    if (!stream.Good()) {
        return false;
    }
    transitSwitches.clear();
    transitSwitches.reserve(transitSwitchCount);
    for (Uint32 i = 0; i < transitSwitchCount; ++i) {
        TransitSwitchRecord ts;
        stream >> ts.key;
        if (!ReadRef(stream, ts.occupantRef)) {
            return false;
        }
        stream >> ts.field20 >> ts.field24 >> ts.field28;
        if (version <= 0x1cu) {
            if (!ReadUint8Vector(stream, ts.legacyBytes)) {
                return false;
            }
        }
        stream >> ts.field2D >> ts.field2E >> ts.field2F >> ts.field30 >> ts.field34;
        if (version < 0x17u) {
            ts.field34 *= 0.01f;
        }
        if (version >= 0x1du) {
            Uint32 ruleCount = 0;
            stream >> ruleCount;
            if (!stream.Good()) {
                return false;
            }
            ts.rules.clear();
            ts.rules.reserve(ruleCount);
            for (Uint32 r = 0; r < ruleCount; ++r) {
                TransitSwitchRuleRecord rule;
                stream >> rule.flag0;
                for (bool& f : rule.flags1to4) {
                    stream >> f;
                }
                stream >> rule.travelTypeIn >> rule.travelTypeOut;
                if (!stream.Good()) {
                    return false;
                }
                ts.rules.push_back(rule);
            }
            if (version >= 0x21u) {
                stream >> ts.field54 >> ts.field58;
            }
        }
        if (!stream.Good()) {
            return false;
        }
        transitSwitches.push_back(std::move(ts));
    }

    if (!ReadRef(stream, ref8D0) || !ReadRef(stream, ref8D8)) {
        return false;
    }
    stream >> field940 >> field948;
    field94C = 0.0f;
    if (version >= 0x29u) {
        stream >> field94C;
    }
    stream >> field950;
    if (version >= 0x16u) {
        stream >> field958;
    } else {
        Float32 f = 0.0f;
        stream >> f;
        field958 = static_cast<Float64>(f);
    }
    field960 = 0.0;
    if (version >= 0x29u) {
        stream >> field960;
    }
    stream >> field968 >> field96C >> field970 >> field974 >> field978 >> field97C;
    if (!stream.Good()) {
        return false;
    }

    if (!ReadUint16Set(stream, set980) || !ReadUint16Set(stream, set9A8)) {
        return false;
    }
    set9E0.clear();
    setA08.clear();
    if (version >= 0x1au) {
        stream >> field9D0 >> field9D4 >> field9D8;
        if (!ReadUint16Set(stream, set9E0) || !ReadUint16Set(stream, setA08)) {
            return false;
        }
    }

    stream >> fieldA30;
    if (!ReadScanningIterator(stream, scanningIterator)) {
        return false;
    }
    stream >> fieldA40 >> fieldA44 >> fieldA48 >> fieldA4C >> fieldA50 >> fieldB78 >>
        fieldB7C >> fieldB80;
    if (version <= 0x18u) {
        Uint8 b0 = 0, b1 = 0;
        stream >> b0 >> b1;
        fieldB58 = static_cast<Sint16>(b0);
        fieldB5A = static_cast<Sint16>(b1);
    } else {
        stream >> fieldB58 >> fieldB5A;
    }
    if (!ReadBoundaryIterator(stream, boundaryIterator)) {
        return false;
    }
    stream >> fieldAC0 >> fieldAA9 >> fieldAA4 >> fieldA70 >> fieldA74 >> fieldABC;
    for (Uint32& v : fieldA94) {
        stream >> v;
    }
    stream >> fieldAA8 >> fieldA90;
    for (Sint32& v : rectAAC) {
        stream >> v;
    }
    for (auto& row : matrixAD4) {
        row.assign(static_cast<std::size_t>(count60), 0);
        for (Uint32& v : row) {
            stream >> v;
        }
    }
    stream >> fieldB40 >> fieldB41 >> fieldB68 >> fieldB6C;
    fieldB70 = 0;
    if (version >= 0x19u) {
        stream >> fieldB70;
    }
    stream >> fieldB84 >> fieldB88 >> fieldB8C >> fieldB90;
    fieldB94 = fieldB98 = fieldB9C = 0;
    fieldBA0 = fieldBA4 = fieldBA8 = fieldBAC = 0;
    fieldBB0 = fieldBB4 = fieldBB8 = fieldBBC = fieldBC0 = fieldBC4 = 0;
    if (version > 0x18u) {
        stream >> fieldB94 >> fieldB98 >> fieldB9C >> fieldBA0 >> fieldBA4 >> fieldBA8 >>
            fieldBAC;
        if (version >= 0x28u) {
            stream >> fieldBB0 >> fieldBB4 >> fieldBB8 >> fieldBBC >> fieldBC0 >> fieldBC4;
        }
    }
    if (!stream.Good()) {
        return false;
    }

    for (TrafficCommuteBlock& blk : commuteBlocks) {
        const Sint32 nPrimary = (version >= 0x23u) ? countCC : countE4;
        blk.primaryStats.assign(static_cast<std::size_t>(nPrimary), TrafficStatEntry20{});
        for (TrafficStatEntry20& st : blk.primaryStats) {
            stream >> st.field00 >> st.field04 >> st.field08 >> st.field0C >> st.field10;
        }
        blk.extraStats.clear();
        if (version >= 0x25u && countC8 > 0) {
            blk.extraStats.assign(static_cast<std::size_t>(countC8), TrafficStatEntry20{});
            for (TrafficStatEntry20& st : blk.extraStats) {
                stream >> st.field00 >> st.field04 >> st.field08 >> st.field0C >> st.field10;
            }
        }
        blk.primaryWords.assign(static_cast<std::size_t>(countCC), 0);
        for (Uint16& w : blk.primaryWords) {
            stream >> w;
        }
        blk.extraWords.clear();
        if (version >= 0x25u && countC8 > 0) {
            blk.extraWords.assign(static_cast<std::size_t>(countC8), 0);
            for (Uint16& w : blk.extraWords) {
                stream >> w;
            }
        }
        stream >> blk.field134;
        blk.subStatCount = 3;
        if (version >= 0x22u) {
            stream >> blk.subStatCount;
        }
        if (!stream.Good() || blk.subStatCount < 0) {
            return false;
        }
        blk.subStats.assign(static_cast<std::size_t>(blk.subStatCount),
                            TrafficSubStatEntry20{});
        for (TrafficSubStatEntry20& ss : blk.subStats) {
            stream >> ss.field00 >> ss.field04;
            if (version < 0x17u) {
                ss.field04 *= 0.01f;
            }
            stream >> ss.field08 >> ss.field0C >> ss.field10;
            if (version < 0x17u) {
                ss.field10 *= 0.01f;
            }
        }
        stream >> blk.field228 >> blk.field22C >> blk.field230 >> blk.field234;
        blk.field238 = 0x80u;
        if (version > 0x14u) {
            stream >> blk.field238;
        }
        if (!stream.Good()) {
            return false;
        }
    }

    Uint32 tunnelCount = 0;
    stream >> tunnelCount;
    if (!stream.Good()) {
        return false;
    }
    tunnels.clear();
    tunnels.reserve(tunnelCount);
    for (Uint32 i = 0; i < tunnelCount; ++i) {
        TrafficTunnelInfoRecord tun;
        stream >> tun.key >> tun.field14 >> tun.field15 >> tun.field16 >> tun.field3C;
        tun.travelTimeFloats.assign(static_cast<std::size_t>(count60), 0.0f);
        for (Float32& f : tun.travelTimeFloats) {
            stream >> f;
        }
        if (!stream.Good()) {
            return false;
        }
        tunnels.push_back(std::move(tun));
    }

    stream >> field58C >> field870 >> field874;
    field878 = 0.0f;
    field87C = 0;
    if (version >= 0x15u) {
        stream >> field878 >> field87C;
    }
    array908.assign(static_cast<std::size_t>(countCC), 0);
    for (Uint32& v : array908) {
        stream >> v;
    }
    Uint32 count31C = 0;
    stream >> count31C;
    if (!stream.Good()) {
        return false;
    }
    array31C.assign(count31C, 0);
    for (Uint32& v : array31C) {
        stream >> v;
    }

    Uint32 propHolderCount = 0;
    stream >> propHolderCount;
    if (!stream.Good()) {
        return false;
    }
    propertyHolders.clear();
    propertyHolders.reserve(propHolderCount);
    for (Uint32 i = 0; i < propHolderCount; ++i) {
        Uint16 key = 0;
        stream >> key;
        SerializedPropertyHolder holder;
        if (!ReadSerializedPropertyHolder(stream, holder)) {
            return false;
        }
        propertyHolders.emplace_back(key, std::move(holder));
    }

    pairs590_5A8.clear();
    field5C0 = 0;
    ferryTerminals.clear();
    if (version >= 0x1bu) {
        pairs590_5A8.assign(static_cast<std::size_t>(count60), {0u, 0u});
        for (auto& [first, second] : pairs590_5A8) {
            stream >> first >> second;
        }
        if (version >= 0x27u) {
            stream >> field5C0;
        }
        if (version >= 0x1eu) {
            Uint32 ftCount = 0;
            stream >> ftCount;
            if (!stream.Good()) {
                return false;
            }
            ferryTerminals.reserve(ftCount);
            for (Uint32 i = 0; i < ftCount; ++i) {
                FerryTerminalRecord ft;
                if (version <= 0x1fu) {
                    stream >> ft.legacyWord;
                }
                for (Uint8& b : ft.bytes10to15) {
                    stream >> b;
                }
                stream >> ft.field18Byte >> ft.field1C >> ft.field50 >> ft.field52;
                if (version >= 0x26u) {
                    stream >> ft.field54 >> ft.field55;
                } else {
                    ft.field54 = ft.bytes10to15[0];
                    ft.field55 = ft.bytes10to15[1];
                }
                Uint32 linkCount1 = 0;
                stream >> linkCount1;
                if (!stream.Good()) {
                    return false;
                }
                ft.terminalLinks.reserve(linkCount1);
                for (Uint32 j = 0; j < linkCount1; ++j) {
                    FerryTerminalLinkRecord link;
                    stream >> link.field18 >> link.field12 >> link.field14 >> link.field10;
                    if (!ReadUint8Vector(stream, link.pathBytes)) {
                        return false;
                    }
                    if (version >= 0x24u) {
                        stream >> link.field38 >> link.field3C >> link.field3A >> link.field3E;
                    }
                    if (!stream.Good()) {
                        return false;
                    }
                    ft.terminalLinks.push_back(std::move(link));
                }
                Uint32 linkCount2 = 0;
                stream >> linkCount2;
                if (!stream.Good()) {
                    return false;
                }
                ft.neighborLinks.reserve(linkCount2);
                for (Uint32 j = 0; j < linkCount2; ++j) {
                    FerryNeighborConnectionLinkRecord link;
                    stream >> link.field18 >> link.field10 >> link.field12 >> link.field14 >>
                        link.field16;
                    if (!ReadUint8Vector(stream, link.pathBytesA)) {
                        return false;
                    }
                    if (version >= 0x1fu) {
                        if (!ReadUint8Vector(stream, link.pathBytesB)) {
                            return false;
                        }
                        if (version >= 0x24u) {
                            stream >> link.field50 >> link.field54 >> link.field52 >>
                                link.field56;
                        }
                    }
                    if (!stream.Good()) {
                        return false;
                    }
                    ft.neighborLinks.push_back(std::move(link));
                }
                ferryTerminals.push_back(std::move(ft));
            }
        }
    }
    return stream.Good();
}

bool TrafficSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    return WriteTrafficAtVersion(*this, stream, version);
}

bool TrafficSimulatorSaveRecord::WriteV41(cIGZOStream& stream) const {
    // Exact cSC4TrafficSimulator::Write @ 0x10007a5f8 (always writes version 0x29 = 41).
    return WriteTrafficAtVersion(*this, stream, Uint16{41});
}

bool TrafficSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

bool TrafficTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    // Exact cSC4TrafficSimulator::ReadSimulatorTunableValues @ 0x100078e16.
    fieldBCC = exemplar.GetFloat(0x491332e6u, fieldBCC);
    for (Uint32 i = 0; i < 9u; ++i) {
        const auto it = exemplar.properties.find(0x491332e7u + i);
        if (it != exemplar.properties.end() && !it->second.floatValues.empty()) {
            travelTypeNetworkSpeeds[i] = it->second.floatValues;
        }
    }
    fieldBD4 = exemplar.GetFloat(0x29136788u, fieldBD4);
    fieldBD0 = exemplar.GetFloat(0x29136789u, fieldBD0);
    {
        const auto it = exemplar.properties.find(0xa92356b3u);
        if (it != exemplar.properties.end() && !it->second.floatValues.empty()) {
            networkCapacities = it->second.floatValues;
        }
    }
    pctDE0 = exemplar.GetFloat(0xa92356aeu, pctDE0 * 100.0f) * 0.01f;
    pctDE4 = exemplar.GetFloat(0xa92356afu, pctDE4 * 100.0f) * 0.01f;
    fieldDE8 = exemplar.GetFloat(0xa92356b0u, fieldDE8);
    fieldEA0 = static_cast<Uint32>(exemplar.GetInt(0xa92356b6u, fieldEA0));
    fieldEA4 = static_cast<Uint32>(exemplar.GetInt(0xa92356b7u, fieldEA4));
    fieldEA8 = static_cast<Uint32>(exemplar.GetInt(0xa92356b8u, fieldEA8));
    fieldEAC = exemplar.GetFloat(0xa92356bau, fieldEAC);
    fieldF4C = exemplar.GetFloat(0xa92356bdu, fieldF4C);
    fieldF5C = exemplar.GetFloat(0x49a2e8beu, fieldF5C);
    fieldF60 = exemplar.GetFloat(0x4a678060u, fieldF60);
    fieldF64 = exemplar.GetFloat(0xca76013bu, fieldF64);
    fieldF68 = exemplar.GetFloat(0xea7b5f06u, fieldF68);
    fieldFF8 = exemplar.GetFloat(0x4be09ecbu, fieldFF8);
    fieldFFC = exemplar.GetFloat(0x4be09eccu, fieldFFC);
    fieldF6C = exemplar.GetFloat(0x0bd86fd5u, fieldF6C);
    fieldF70 = exemplar.GetFloat(0x8c3caef5u, fieldF70);
    return LoadProps(exemplar, properties);
}

// ===========================================================================
// Shared cRZRandomSubclass helper (operator>> @ 0x1000ce8b3 /
// cRZRandomSubclass::Read @ 0x1000ce8d2, Write @ 0x1000ce948).
// ===========================================================================
namespace {

bool ReadRZRandom(cIGZIStream& stream, RZRandomState& rng) noexcept {
    stream >> rng.seedA >> rng.seedB >> rng.field08 >> rng.field10;
    return stream.Good();
}

void WriteRZRandom(cIGZOStream& stream, const RZRandomState& rng) {
    stream << rng.seedA << rng.seedB << rng.field08 << rng.field10;
}

bool ReadWeatherByteGrid2D(cIGZIStream& stream, WeatherByteGrid2D& grid) noexcept {
    stream >> grid.rows >> grid.cols;
    if (!stream.Good() || grid.rows > 0xf423fu || grid.cols > 0xf423fu) {
        return false;
    }
    const std::size_t total =
        static_cast<std::size_t>(grid.rows) * static_cast<std::size_t>(grid.cols);
    if (total > stream.Remaining()) {
        return false;
    }
    grid.cells.assign(total, 0);
    for (Uint8& b : grid.cells) {
        stream >> b;
    }
    return stream.Good();
}

void WriteWeatherByteGrid2D(cIGZOStream& stream, const WeatherByteGrid2D& grid,
                            bool normalizeBool) {
    stream << grid.rows << grid.cols;
    for (Uint8 b : grid.cells) {
        stream << static_cast<Uint8>(normalizeBool ? (b != 0 ? 1u : 0u) : b);
    }
}

} // namespace

// ===========================================================================
// cSC4CivicBuildingSimulator (CLSID 0x0a413481, Read 0x1000e97f6,
// Write 0x1000e9712)
// ===========================================================================
bool CivicBuildingSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version != 6) {
        return false;
    }
    Uint32 setCount = 0;
    stream >> setCount;
    if (!stream.Good()) {
        return false;
    }
    unlockedSet.assign(setCount, 0);
    for (Uint32& id : unlockedSet) {
        stream >> id;
    }
    Uint32 statusCount = 0;
    stream >> statusCount;
    if (!stream.Good()) {
        return false;
    }
    statuses.clear();
    statuses.reserve(statusCount);
    for (Uint32 i = 0; i < statusCount; ++i) {
        ConditionalBuildingStatusRecord st;
        stream >> st.buildingTypeId >> st.statusByte >> st.statusWord;
        if (!stream.ReadGZString(st.statusText)) {
            return false;
        }
        statuses.push_back(std::move(st));
    }
    if (!ReadRef(stream, deptBudgetRef) ||
        !ReadRefList(stream, zooOccupants) ||
        !ReadRefList(stream, civicOccupants)) {
        return false;
    }
    return stream.Good();
}

bool CivicBuildingSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{6} << static_cast<Uint32>(unlockedSet.size());
    for (Uint32 id : unlockedSet) {
        stream << id;
    }
    stream << static_cast<Uint32>(statuses.size());
    for (const ConditionalBuildingStatusRecord& st : statuses) {
        stream << st.buildingTypeId << st.statusByte << st.statusWord;
        stream.WriteGZString(st.statusText);
    }
    WriteRef(stream, deptBudgetRef);
    WriteRefList(stream, zooOccupants);
    WriteRefList(stream, civicOccupants);
    return true;
}

bool CivicBuildingSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

// ===========================================================================
// cSC4FloraSimulator (CLSID 0xc990c034, Read 0x1000be6d8, Write 0x1000be5a0,
// ReadSimulatorTunableValues 0x1000be15a)
// ===========================================================================
bool FloraSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version != 2) {
        return false;
    }
    stream >> flag50 >> flag51 >> flag52 >> flag53 >> field54 >> field58 >> field5C >>
        field60 >> field64 >> field68 >> field6C >> field70 >> field74;
    if (!stream.Good()) {
        return false;
    }
    if (!ReadRandomIterator(stream, randomIterator) ||
        !ReadRZRandom(stream, rngState) ||
        !ReadRefList(stream, floraOccupants)) {
        return false;
    }
    stream >> fieldF0 >> fieldF4;
    return stream.Good();
}

bool FloraSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{2} << flag50 << flag51 << flag52 << flag53 << field54 << field58
           << field5C << field60 << field64 << field68 << field6C << field70 << field74;
    WriteRandomIterator(stream, randomIterator);
    WriteRZRandom(stream, rngState);
    WriteRefList(stream, floraOccupants);
    stream << fieldF0 << fieldF4;
    return true;
}

bool FloraSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

bool FloraTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    // Exact cSC4FloraSimulator::ReadSimulatorTunableValues @ 0x1000be15a.
    const Float32 prop00 = exemplar.GetFloat(0xc9dbdd00u, 0.0f);
    if (prop00 != 0.0f) {
        inverseProp00 = 1.0f / prop00;
    }
    fieldFC = exemplar.GetFloat(0xc9dbdd01u, fieldFC);
    field68 = static_cast<Sint32>(exemplar.GetInt(0x8761abd0u, field68));
    field70 = static_cast<Sint32>(exemplar.GetInt(0x085489c7u, field70));
    rawProp02 = exemplar.GetFloat(0xc9dbdd02u, rawProp02);
    return LoadProps(exemplar, properties);
}

// ===========================================================================
// cSC4MySimAgentSimulator (CLSID 0x29e37dec, Read 0x1007ac44e, Write 0x1007ac3f6)
// ===========================================================================
bool MySimAgentSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    return stream.Good() && version == 1;
}

bool MySimAgentSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{1};
    return true;
}

bool MySimAgentSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

// ===========================================================================
// cSC4NeighborsSimulator (CLSID 0xc990c0a4, Read 0x10006f528, Write 0x10006f4b6)
// ===========================================================================
bool NeighborsSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version != 2) {
        return false;
    }
    return ReadRefVector(stream, connectionRefs);
}

bool NeighborsSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{2};
    WriteRefVector(stream, connectionRefs);
    return true;
}

bool NeighborsSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

// ===========================================================================
// cSC4OrdinanceSimulator (CLSID 0xa990c08e, Read 0x10079903a, Write 0x100798f6e,
// DoMonthlySimulation 0x100799302)
// ===========================================================================
bool OrdinanceSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version != 4) {
        return false;
    }
    stream >> monthlyExpense >> monthlyIncome;
    if (!stream.Good() || !ReadRefVector(stream, ordinanceRefs)) {
        return false;
    }
    stream >> pendingAdjustment;
    return stream.Good();
}

bool OrdinanceSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{4} << monthlyExpense << monthlyIncome;
    WriteRefVector(stream, ordinanceRefs);
    stream << pendingAdjustment;
    return true;
}

bool OrdinanceSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

void OrdinanceSimulatorSaveRecord::StepMonthly(
    std::span<const Sint64> activeOrdinanceMonthlyCosts) noexcept {
    // Exact cSC4OrdinanceSimulator::DoMonthlySimulation @ 0x100799302:
    // 1. Reset monthlyExpense (+0x20) and monthlyIncome (+0x28) to 0.
    // 2. Fold pendingAdjustment (+0x30): if > 0, monthlyIncome = pendingAdjustment;
    //    else monthlyExpense = -pendingAdjustment; then pendingAdjustment = 0.
    // 3. For each active ordinance with a monthly cost: if cost > 0 add |cost| to
    //    monthlyIncome (+0x28), else add |cost| to monthlyExpense (+0x20).
    monthlyExpense = 0;
    monthlyIncome = 0;
    if (pendingAdjustment > 0) {
        monthlyIncome = pendingAdjustment;
    } else {
        monthlyExpense = -pendingAdjustment;
    }
    pendingAdjustment = 0;
    for (Sint64 cost : activeOrdinanceMonthlyCosts) {
        const Sint64 absCost = cost < 0 ? -cost : cost;
        if (cost > 0) {
            monthlyIncome += absCost;
        } else {
            monthlyExpense += absCost;
        }
    }
}

// ===========================================================================
// cSC4WeatherSimulator (CLSID 0x2990c1bc, Read 0x1000e43a4, Write 0x1000e4016,
// ReadSimulatorTunableValues 0x1000e3b30)
// ===========================================================================
bool WeatherSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version > 2) {
        return false;
    }
    stream >> gridCellsX >> gridCellsZ >> cellWidthM >> cellHeightM >> invCellWidth >>
        invCellHeight >> worldWidthM >> worldHeightM >> cellsPerStandardCellX >>
        cellsPerStandardCellZ >> field78 >> field7C >> field80 >> field84 >> field88 >>
        field8C >> field90 >> field94;
    if (!stream.Good()) {
        return false;
    }
    if (!ReadWeatherByteGrid2D(stream, grid98) ||
        !ReadWeatherByteGrid2D(stream, boolGridB0)) {
        return false;
    }
    Uint32 rectCount = 0;
    stream >> rectCount;
    if (!stream.Good()) {
        return false;
    }
    rectsC8.assign(rectCount, {0, 0, 0, 0});
    for (auto& r : rectsC8) {
        stream >> r[0] >> r[1] >> r[2] >> r[3];
    }
    if (!stream.Good() || !ReadRandomIterator(stream, randomIterator)) {
        return false;
    }
    stream >> field120 >> field124 >> field128 >> field12C >> field130;
    if (!stream.Good() ||
        !ReadWeatherByteGrid2D(stream, moistureGrid138) ||
        !ReadWeatherByteGrid2D(stream, moistureVarGrid150)) {
        return false;
    }
    stream >> field168 >> field16C >> field170 >> field174 >> field178 >> field17C >>
        field180 >> field184 >> field18C >> vec190[0] >> vec190[1] >> ambientWindSpeed >>
        ambientWindDir[0] >> ambientWindDir[1] >> field1A4 >> vec1A8[0] >> vec1A8[1] >>
        field1B0 >> vec1B4[0] >> vec1B4[1];
    if (!stream.Good()) {
        return false;
    }
    if (version < 2) {
        // Legacy version 1 wind vectors & two temporary byte grids (discarded by binary).
        Uint32 windCount = 0;
        stream >> windCount;
        if (!stream.Good() || !stream.Skip(static_cast<std::size_t>(windCount) * 16u)) {
            return false;
        }
        WeatherByteGrid2D tmp1, tmp2;
        if (!ReadWeatherByteGrid2D(stream, tmp1) || !ReadWeatherByteGrid2D(stream, tmp2)) {
            return false;
        }
    }
    if (!ReadRZRandom(stream, rngState)) {
        return false;
    }
    stream >> field1D8;
    return stream.Good();
}

bool WeatherSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{2} << gridCellsX << gridCellsZ << cellWidthM << cellHeightM
           << invCellWidth << invCellHeight << worldWidthM << worldHeightM
           << cellsPerStandardCellX << cellsPerStandardCellZ << field78 << field7C << field80
           << field84 << field88 << field8C << field90 << field94;
    WriteWeatherByteGrid2D(stream, grid98, false);
    WriteWeatherByteGrid2D(stream, boolGridB0, true);
    stream << static_cast<Uint32>(rectsC8.size());
    for (const auto& r : rectsC8) {
        stream << r[0] << r[1] << r[2] << r[3];
    }
    WriteRandomIterator(stream, randomIterator);
    stream << field120 << field124 << field128 << field12C << field130;
    WriteWeatherByteGrid2D(stream, moistureGrid138, false);
    WriteWeatherByteGrid2D(stream, moistureVarGrid150, false);
    stream << field168 << field16C << field170 << field174 << field178 << field17C
           << field180 << field184 << field18C << vec190[0] << vec190[1] << ambientWindSpeed
           << ambientWindDir[0] << ambientWindDir[1] << field1A4 << vec1A8[0] << vec1A8[1]
           << field1B0 << vec1B4[0] << vec1B4[1];
    WriteRZRandom(stream, rngState);
    stream << field1D8;
    return true;
}

bool WeatherSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

Uint8 WeatherSimulatorSaveRecord::GetMoistureValue(Float32 worldX,
                                                   Float32 worldZ) const noexcept {
    // Exact cSC4WeatherSimulator::GetMoistureValue @ 0x1000e497a.
    if (moistureGrid138.rows == 0 || moistureGrid138.cols == 0) {
        return 0;
    }
    const Float32 clampedX = std::clamp(worldX, 0.0f, std::max(0.0f, worldWidthM - 1.0f));
    const Float32 clampedZ = std::clamp(worldZ, 0.0f, std::max(0.0f, worldHeightM - 1.0f));
    const Sint32 cellX = std::clamp(static_cast<Sint32>(std::floor(clampedX * invCellWidth)),
                                    0, static_cast<Sint32>(moistureGrid138.rows) - 1);
    const Sint32 cellZ = std::clamp(static_cast<Sint32>(std::floor(clampedZ * invCellHeight)),
                                    0, static_cast<Sint32>(moistureGrid138.cols) - 1);
    return moistureGrid138.cells[static_cast<std::size_t>(cellX) * moistureGrid138.cols +
                                 static_cast<std::size_t>(cellZ)];
}

Float32 WeatherSimulatorSaveRecord::GetHumidity() const noexcept {
    // Exact cSC4WeatherSimulator::GetHumidity @ 0x1000e4b5a:
    // (1.0f - field16C) * 0.5f + field16C.
    return (1.0f - field16C) * 0.5f + field16C;
}

bool WeatherTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    // Exact cSC4WeatherSimulator::ReadSimulatorTunableValues @ 0x1000e3b30.
    field78 = std::max<Sint32>(1, static_cast<Sint32>(exemplar.GetInt(0xa7607d70u, field78)));
    field7C = std::max<Sint32>(1, static_cast<Sint32>(exemplar.GetInt(0xa7607d71u, field7C)));
    field80 = static_cast<Sint32>(exemplar.GetInt(0xa7607d72u, field80));
    field88 = exemplar.GetFloat(0xa7607d73u, field88);
    field8C = std::clamp(exemplar.GetFloat(0xa7607d74u, field8C), 0.0f, 1.0f);
    field90 = exemplar.GetFloat(0xa7607d75u, field90);
    field124 = static_cast<Sint32>(exemplar.GetInt(0xa7607d76u, field124));
    field128 = std::max(0.0f, exemplar.GetFloat(0xa7607d77u, field128));
    field12C = exemplar.GetFloat(0xa7607d78u, field12C);
    field130 = exemplar.GetFloat(0xa7607d79u, field130);
    field18C = exemplar.GetFloat(0xa7607d86u, field18C);
    field190 = exemplar.GetFloat(0xa7607d87u, field190);
    field194 = exemplar.GetFloat(0xa7607d88u, field194);
    field16C = exemplar.GetFloat(0xa7607d7au, field16C);
    field170 = std::clamp(exemplar.GetFloat(0xa7607d7bu, field170), 0.0f, 1.0f);
    field174 = exemplar.GetFloat(0xa7607d7cu, field174);
    field178 = exemplar.GetFloat(0xa7607d7du, field178);
    field17C = exemplar.GetFloat(0xa7607d7eu, field17C);
    field180 = exemplar.GetFloat(0xa7607d7fu, field180);
    field184 = static_cast<Uint32>(exemplar.GetInt(0xa7607d80u, field184));
    field188 = static_cast<Uint32>(exemplar.GetInt(0xa7607d81u, field188));
    field1DC = exemplar.GetFloat(0xa7607d89u, field1DC);
    return LoadProps(exemplar, properties);
}

// ===========================================================================
// cSC4GameOfLifeSimulator (DoEdit @ 0x1002fc964, DoNextIteration @ 0x1002fca8e)
// ===========================================================================
cSC4GameOfLifeSimulator::cSC4GameOfLifeSimulator(Uint32 width, Uint32 height) {
    Resize(width, height);
}

void cSC4GameOfLifeSimulator::Resize(Uint32 width, Uint32 height) {
    mWidth = width;
    mHeight = height;
    const std::size_t n = static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    mFrontBuffer.assign(n, 0);
    mBackBuffer.assign(n, 0);
}

bool cSC4GameOfLifeSimulator::DoEdit(Uint32 x, Uint32 y) noexcept {
    // Exact cSC4GameOfLifeSimulator::DoEdit @ 0x1002fc964.
    if (x >= mWidth || y >= mHeight) {
        return false;
    }
    const std::size_t idx =
        static_cast<std::size_t>(y) * static_cast<std::size_t>(mWidth) +
        static_cast<std::size_t>(x);
    if (mEditMode == 1) {
        mFrontBuffer[idx] = 0;
        if (mBackBuffer[idx] == 0) {
            return false;
        }
        mBackBuffer[idx] = 0;
        return true;
    }
    if (mEditMode == 0) {
        mFrontBuffer[idx] = 1;
        if (mBackBuffer[idx] == 0) {
            mBackBuffer[idx] = 2;
            return true;
        }
    }
    return false;
}

void cSC4GameOfLifeSimulator::DoNextIteration() noexcept {
    // Exact cSC4GameOfLifeSimulator::DoNextIteration @ 0x1002fca8e:
    // swaps front/back buffers, counts 8-neighbors with toroidal wraparound on
    // the boundary, applies Conway B3/S23 rules, and increments surviving cell
    // age up to 0xbf (191).
    if (mPaused || mWidth == 0 || mHeight == 0) {
        return;
    }
    std::swap(mFrontBuffer, mBackBuffer);
    for (Uint32 y = 0; y < mHeight; ++y) {
        const Uint32 ym1 = (y + mHeight - 1u) % mHeight;
        const Uint32 yp1 = (y + 1u) % mHeight;
        for (Uint32 x = 0; x < mWidth; ++x) {
            const Uint32 xm1 = (x + mWidth - 1u) % mWidth;
            const Uint32 xp1 = (x + 1u) % mWidth;
            auto at = [this](Uint32 cx, Uint32 cy) noexcept -> Uint32 {
                return mBackBuffer[static_cast<std::size_t>(cy) * mWidth + cx] != 0 ? 1u : 0u;
            };
            const Uint32 neighbors =
                at(xm1, ym1) + at(x, ym1) + at(xp1, ym1) +
                at(xm1, y)                + at(xp1, y) +
                at(xm1, yp1) + at(x, yp1) + at(xp1, yp1);
            const std::size_t idx = static_cast<std::size_t>(y) * mWidth + x;
            Uint8 prev = mBackBuffer[idx];
            if ((neighbors & ~1u) == 2u) { // neighbors == 2 or 3
                if (neighbors == 3u && prev == 0u) {
                    mFrontBuffer[idx] = 1u;
                } else {
                    if (prev != 0u && prev != 0xbfu) {
                        ++prev;
                        mBackBuffer[idx] = prev;
                    }
                    mFrontBuffer[idx] = prev;
                }
            } else {
                mFrontBuffer[idx] = 0u;
            }
        }
    }
}

Uint8 cSC4GameOfLifeSimulator::GetCell(Uint32 x, Uint32 y) const noexcept {
    if (x >= mWidth || y >= mHeight) {
        return 0;
    }
    return mFrontBuffer[static_cast<std::size_t>(y) * mWidth + x];
}

// ===========================================================================
// cSC4RigidBodySimulator2D (AdvanceTime @ 0x100700bfc, DoCollisions @ 0x100700ee2)
// ===========================================================================
void cSC4RigidBodySimulator2D::AddObject(const RigidBody2DObject& obj) {
    mObjects.push_back(obj);
}

void cSC4RigidBodySimulator2D::AdvanceTime(Float32 dt) noexcept {
    // Exact cSC4RigidBodySimulator2D::AdvanceTime @ 0x100700bfc / 0x100700c32.
    constexpr Float32 kTwoPi = 6.283185307179586f;
    for (RigidBody2DObject& obj : mObjects) {
        if (!obj.active) {
            continue;
        }
        obj.posX += obj.velX * dt;
        obj.posY += obj.velY * dt;
        obj.zHeight += obj.zVel * dt;
        obj.angle += obj.angularVel * dt;
        if (std::fabs(obj.angle) > kTwoPi) {
            obj.angle = static_cast<Float32>(std::fmod(static_cast<double>(obj.angle),
                                                       static_cast<double>(kTwoPi)));
        }
        const Float32 sinA = std::sin(obj.angle);
        const Float32 cosA = std::cos(obj.angle);
        obj.velX += obj.accelX * dt;
        obj.velY += obj.accelY * dt;
        if (obj.applyDrag) {
            const Float32 fwd = std::max(0.0f, (obj.velX * cosA + obj.velY * sinA) -
                                                   obj.linearDragForward * dt);
            const Float32 lat = std::max(0.0f, (obj.velY * cosA - obj.velX * sinA) -
                                                   obj.linearDragLateral * dt);
            obj.velX = fwd * cosA - lat * sinA;
            obj.velY = lat * cosA + fwd * sinA;
            obj.angularVel = std::max(0.0f, obj.angularVel - obj.angularDrag * dt);
        }
    }
    DoCollisions();
}

void cSC4RigidBodySimulator2D::DoCollisions() noexcept {
    // Exact broad-phase/narrow-phase AABB & z-span overlap in
    // cSC4RigidBodySimulator2D::DoCollisions @ 0x100700ee2.
    for (std::size_t i = 0; i < mObjects.size(); ++i) {
        for (std::size_t j = i + 1; j < mObjects.size(); ++j) {
            RigidBody2DObject& a = mObjects[i];
            RigidBody2DObject& b = mObjects[j];
            if (!a.active || !b.active) {
                continue;
            }
            if (std::fabs(b.posX - a.posX) <= (a.halfExtentX + b.halfExtentX) &&
                std::fabs(b.posY - a.posY) <= (a.halfExtentY + b.halfExtentY)) {
                ++a.collisionCount;
                ++b.collisionCount;
                std::swap(a.velX, b.velX);
                std::swap(a.velY, b.velY);
            }
        }
    }
}

// ===========================================================================
// cSC4Simulator (CLSID 0x2990c1e5, Read @ 0x1001f002c, Write @ 0x1001eff54,
// ReadSimulatorTunableValues @ 0x1001efbdc, DoSetSimSpeed @ 0x1001efd52,
// OnTick @ 0x1001f0b22, SimulateTimePassage @ 0x1001f05d6)
// ===========================================================================
namespace {

void JulianDayToYMD(Uint32 jdn, Uint32& year, Uint32& month, Uint32& day,
                    Uint32& dayOfYear, Uint32& weekDay) noexcept {
    // Fliegel-Van Flandern Gregorian JDN conversion matching cGZDate.
    const Sint64 l0 = static_cast<Sint64>(jdn) + 68569;
    const Sint64 n = (4 * l0) / 146097;
    const Sint64 l1 = l0 - (146097 * n + 3) / 4;
    const Sint64 i = (4000 * (l1 + 1)) / 1461001;
    const Sint64 l2 = l1 - (1461 * i) / 4 + 31;
    const Sint64 j = (80 * l2) / 2447;
    const Sint64 d = l2 - (2447 * j) / 80;
    const Sint64 l3 = j / 11;
    const Sint64 m = j + 2 - 12 * l3;
    const Sint64 y = 100 * (n - 49) + i + l3;
    year = static_cast<Uint32>(y);
    month = static_cast<Uint32>(m);
    day = static_cast<Uint32>(d);

    // Jan 1 of `year` JDN:
    const Sint64 a = (14 - 1) / 12;
    const Sint64 y1 = y + 4800 - a;
    const Sint64 m1 = 1 + 12 * a - 3;
    const Sint64 jan1Jdn = 1 + (153 * m1 + 2) / 5 + 365 * y1 + y1 / 4 - y1 / 100 +
                           y1 / 400 - 32045;
    dayOfYear = static_cast<Uint32>(static_cast<Sint64>(jdn) - jan1Jdn + 1);
    // cGZDate::WeekDay(): 0=Sunday, 1=Monday, ..., 6=Saturday
    weekDay = (jdn + 1u) % 7u;
}

} // namespace

bool SimulatorTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    // Exact cSC4Simulator::ReadSimulatorTunableValues @ 0x1001efbdc.
    speedPeriodMs = {5000u, 1000u, 100u};
    const auto itSpeed = exemplar.properties.find(0x89590e6du);
    if (itSpeed != exemplar.properties.end() && itSpeed->second.intValues.size() == 3u) {
        for (std::size_t i = 0; i < 3u; ++i) {
            speedPeriodMs[i] = static_cast<Uint32>(itSpeed->second.intValues[i]);
        }
    }
    animationTimeDilation = {0.0f, 1.0f, 1.5f, 2.0f, 1.0f};
    const auto itDil = exemplar.properties.find(0x89590e6eu);
    if (itDil != exemplar.properties.end() && !itDil->second.floatValues.empty()) {
        animationTimeDilation = itDil->second.floatValues;
    }
    return LoadProps(exemplar, properties);
}

bool SimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    stream >> version;
    if (!stream.Good() || version > 4) {
        return false;
    }
    stream >> isInOnTick45 >> isCityEstablished46 >> pauseCount48 >> simSpeed54 >>
        simTime58 >> maxMillisecsPerTick90;
    if (!stream.Good()) {
        return false;
    }
    for (std::size_t i = 0; i < 11u; ++i) {
        stream >> intervalCounters[i] >> intervalPending[i];
    }
    if (version <= 3) {
        // Legacy cRZTimer @ 0x1000ce7ca (Uint8 units, Uint64 elapsed, Uint8 running).
        Uint8 units = 0, running = 0;
        Uint64 elapsed = 0;
        stream >> units >> elapsed >> running;
    }
    stream >> julianDayNumber;
    return stream.Good();
}

bool SimulatorSaveRecord::Write(cIGZOStream& stream) const {
    stream << Uint16{4} << isInOnTick45 << isCityEstablished46 << pauseCount48
           << simSpeed54 << simTime58 << maxMillisecsPerTick90;
    for (std::size_t i = 0; i < 11u; ++i) {
        stream << intervalCounters[i] << intervalPending[i];
    }
    stream << julianDayNumber;
    return true;
}

bool SimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> bytes) noexcept {
    return ReadRecordFromSC4(*this, bytes);
}

bool SimulatorSaveRecord::DoSetSimSpeed(Sint32 speed,
                                        const SimulatorTuning& tuning) noexcept {
    // Exact cSC4Simulator::DoSetSimSpeed @ 0x1001efd52.
    if (static_cast<Uint32>(speed) > 2u) {
        return false;
    }
    targetTicksPerSec9C = kSimSpeedTicksTable[static_cast<std::size_t>(speed)];
    simSpeed54 = speed;
    intervalPeriodMs98 = tuning.speedPeriodMs[static_cast<std::size_t>(speed)];
    return true;
}

Uint32 SimulatorSaveRecord::ComputeTickTimeBudgetMs(
    Uint32 elapsedSinceLastTickMs) noexcept {
    // Exact budget calculation in cSC4Simulator::OnTick @ 0x1001f0b7c..0x1001f0baa:
    // rate = (pauseCount48 > 0 || hiddenPauseCount4C > 0) ? 30 : targetTicksPerSec9C;
    // maxMillisecsPerTick90 = max(15, (1000 / rate) - elapsedSinceLastTickMs).
    const Sint32 rate =
        (pauseCount48 > 0 || hiddenPauseCount4C > 0 || targetTicksPerSec9C <= 0)
            ? 30
            : targetTicksPerSec9C;
    const Sint32 budget = (1000 / rate) - static_cast<Sint32>(elapsedSinceLastTickMs);
    maxMillisecsPerTick90 = static_cast<Uint32>(budget > 14 ? budget : 15);
    return maxMillisecsPerTick90;
}

bool SimulatorSaveRecord::StepSpeedGatedTimePassage(
    Uint32 elapsedIntervalMs, Uint32& outResetTimerCount) noexcept {
    // Exact cSC4Simulator::SimulateTimePassage @ 0x1001f05d6 when agent buckets
    // are empty: gates minute/hour/day progression on (elapsedIntervalMs >=
    // intervalPeriodMs98 || rateMode94 != mode).
    outResetTimerCount = 0;
    intervalPending.fill(false);

    // 1. Minute step (+0xd0 = intervalCounters[2], +0xf6 = intervalPending[2]).
    if (elapsedIntervalMs >= intervalPeriodMs98 || rateMode94 != 2) {
        if (rateMode94 == 2) {
            ++outResetTimerCount;
        }
        intervalCounters[2] = 60u;
    }
    if (intervalCounters[2] < 60u) {
        return false;
    }

    // 2. Hour step (+0xd4 = intervalCounters[3], +0xf7 = intervalPending[3]).
    if (elapsedIntervalMs >= intervalPeriodMs98 || rateMode94 != 3) {
        intervalCounters[2] = 0u;
        if (rateMode94 == 3) {
            ++outResetTimerCount;
        }
        intervalCounters[3] = 24u;
    }
    if (intervalCounters[3] < 24u) {
        return false;
    }

    // 3. Day / Week / Month / Year step (+0xd8..+0xe4 = intervalCounters[4..7],
    //    +0xf8..+0xfb = intervalPending[4..7]).
    if (elapsedIntervalMs < intervalPeriodMs98 && rateMode94 == 4) {
        return false;
    }
    intervalCounters[3] = 0u;
    if (rateMode94 == 4) {
        ++outResetTimerCount;
    }
    intervalPending[4] = true;
    ++julianDayNumber;

    Uint32 year = 0, month = 0, day = 0, dayOfYear = 0, weekDay = 0;
    JulianDayToYMD(julianDayNumber, year, month, day, dayOfYear, weekDay);
    intervalCounters[4] = dayOfYear;
    if (weekDay == 1u) {
        intervalPending[5] = true;
        intervalCounters[5] = ((dayOfYear - 1u) / 7u) + 1u;
    }
    if (day == 1u) {
        intervalPending[6] = true;
        intervalCounters[6] = month;
    }
    if (dayOfYear == 1u) {
        intervalPending[7] = true;
        intervalCounters[7] = year;
    }
    return true;
}

// ===========================================================================
// Civic & Utility Simulator Step / Grid Update Implementations
// ===========================================================================

namespace {

inline Sint32 FloatMagicRound23(Float32 value) noexcept {
    // Exact x86_64 IEEE-754 2^23 magic constant (12582912.0f = 0x4b400000)
    // used in cSC4ProtectionSimulator::CalculateAndWriteAdjustedGridDataByCell
    // @ 0x1000ab812 and cSC4PoliceSimulator::DoMonthlyCriminalRehabilitation
    // @ 0x1000a38fe.
    const Float32 shifted = value + 12582912.0f;
    Uint32 bits = 0;
    std::memcpy(&bits, &shifted, sizeof(bits));
    return static_cast<Sint32>(bits - 0x4b400000u);
}

} // namespace

void ProtectionSimulatorSaveRecord::AdjustProtectionEffect(
    ProtectionEffect& effect, Float32 efficiency) noexcept {
    // Exact cSC4ProtectionSimulator::AdjustProtectionEffect @ 0x1000aac14:
    //   effect->radius *= efficiency;
    //   effect->centerStrength = (Sint32)((Float32)effect->centerStrength * efficiency);
    //   effect->exteriorStrength = (Sint32)((Float32)effect->exteriorStrength * efficiency);
    effect.radius *= efficiency;
    effect.centerStrength = static_cast<Sint32>(
        static_cast<Float32>(effect.centerStrength) * efficiency);
    effect.exteriorStrength = static_cast<Sint32>(
        static_cast<Float32>(effect.exteriorStrength) * efficiency);
}

void ProtectionSimulatorSaveRecord::FinalizeDailyCoverageCycle() noexcept {
    // Exact cSC4ProtectionSimulator::DoDailySimulation @ 0x1000aa80a (end-of-cycle branch):
    //   if (fieldD0 > 0) {
    //     fieldDC = (Float32)fieldD4 / (Float32)fieldD0;
    //     fieldE0 = (Float32)fieldD8 / (Float32)fieldD0;
    //   } else {
    //     fieldDC = 0.0f; fieldE0 = 0.0f;
    //   }
    //   fieldD0 = 0; fieldD4 = 0; fieldD8 = 0;
    if (fieldD0 > 0) {
        const Float32 denom = static_cast<Float32>(fieldD0);
        fieldDC = static_cast<Float32>(fieldD4) / denom;
        fieldE0 = static_cast<Float32>(fieldD8) / denom;
    } else {
        fieldDC = 0.0f;
        fieldE0 = 0.0f;
    }
    fieldD0 = 0;
    fieldD4 = 0;
    fieldD8 = 0;
}

void ProtectionSimulatorSaveRecord::AccumulateCellCoverage(
    Uint8 adjustedCellValue, bool isZoned) noexcept {
    // Exact cSC4ProtectionSimulator::DoDailySimulation @ 0x1000aa80a (per-cell branch).
    if (!isZoned) {
        return;
    }
    ++fieldD0;
    fieldD4 += static_cast<Sint32>(adjustedCellValue);
    if (adjustedCellValue != 0u) {
        ++fieldD8;
    }
}

Float32 FireProtectionSimulatorSaveRecord::ComputeStationEfficiency(
    Float32 fundingFraction, const cSC4ResponseCurve& fundingCurve,
    bool hasRoadAccess, Float32 noRoadAccessMult,
    bool isOnStrike, Float32 strikeEfficiencyMult) noexcept {
    // Exact cSC4FireProtectionSimulator::GetStationEfficiency @ 0x1002fb58e:
    //   eff = fundingCurve.GetOutputValue(fundingFraction * 100.0f) / 100.0f;
    //   if (!hasRoadAccess) eff *= noRoadAccessMult;
    //   if (isOnStrike) eff *= strikeEfficiencyMult;
    Float32 eff = fundingCurve.GetOutputValue(fundingFraction * 100.0f) * 0.01f;
    if (!hasRoadAccess) {
        eff *= noRoadAccessMult;
    }
    if (isOnStrike) {
        eff *= strikeEfficiencyMult;
    }
    return eff;
}

Uint8 ProtectionSimulatorSaveRecord::CalculateAdjustedProtectionCell(
    Sint16 unadjustedValue, Float32 adjustmentMultiplier) noexcept {
    // Exact cSC4ProtectionSimulator::CalculateAndWriteAdjustedGridDataByCell @ 0x1000ab812.
    const Sint32 rounded =
        FloatMagicRound23(static_cast<Float32>(unadjustedValue) * adjustmentMultiplier);
    return static_cast<Uint8>(std::clamp<Sint32>(rounded, 0, 255));
}

void ProtectionSimulatorSaveRecord::CalculateAdjustedProtectionGrid(
    std::span<const Sint16> unadjustedGrid, Float32 adjustmentMultiplier,
    std::span<Uint8> outAdjustedGrid) noexcept {
    const std::size_t n = std::min(unadjustedGrid.size(), outAdjustedGrid.size());
    for (std::size_t i = 0; i < n; ++i) {
        outAdjustedGrid[i] =
            CalculateAdjustedProtectionCell(unadjustedGrid[i], adjustmentMultiplier);
    }
}

Uint32 LandValueSimulatorSaveRecord::GetLandValueTypeForValue(
    Uint8 landValue, Uint8 lowMaxThreshold, Uint8 medMaxThreshold) noexcept {
    // Exact cSC4LandValueSimulator::GetLandValueType @ 0x1000c73fc:
    // returns 1 (Low), 2 (Medium), or 3 (High).
    if (landValue < lowMaxThreshold) {
        return 1u;
    }
    return (landValue < medMaxThreshold) ? 2u : 3u;
}

SimSerializableRef CommercialSimulatorSaveRecord::GetProximityMapRef(
    Uint32 developerType) const noexcept {
    // Exact cSC4CommercialSimulator::GetProximityMap @ 0x1001fc80c.
    if (developerType == 0x3320u) {
        return gridRef58;
    }
    if (developerType == 0x3330u) {
        return gridRef60;
    }
    return {};
}

void PoliceSimulatorSaveRecord::DoMonthlyCriminalRehabilitation(
    Float32 rehabRate) noexcept {
    // Exact cSC4PoliceSimulator::DoMonthlyCriminalRehabilitation @ 0x1000a38fe:
    // field150 -= FloatMagicRound23(Float32(field150) * rehabRate), clamped >= 0.
    const Sint32 rehabbed =
        FloatMagicRound23(static_cast<Float32>(field150) * rehabRate);
    field150 = std::max<Sint32>(0, field150 - rehabbed);
}

void PoliceSimulatorSaveRecord::ProcessMonthlyCrimeStats() noexcept {
    // Exact cSC4PoliceSimulator::ProcessMonthlyCrimeStats @ 0x1000a3b04.
    if (field15C > 0) {
        field150 = std::max<Sint32>(0, field150 + field15C);
    }
    field154 = field15C;
    field15C = 0;
    field158 = field160;
    field160 = 0;
}

void PoliceSimulatorSaveRecord::ResetUnjailedCriminals(
    Float32 jailCapacityMultiplier) noexcept {
    // Exact cSC4PoliceSimulator::ResetUnjailedCriminals @ 0x1000a4efc:
    //   cap = (Sint32)((Float32)field148 * jailCapacityMultiplier);
    //   if (field150 > cap) {
    //     field164 += (field150 - cap);
    //     field150 = cap;
    //   }
    //   field164 = 0;
    const Sint32 cap = static_cast<Sint32>(
        static_cast<Float32>(field148) * jailCapacityMultiplier);
    if (field150 > cap) {
        field164 += (field150 - cap);
        field150 = cap;
    }
    field164 = 0;
}

Float32 PoliceSimulatorSaveRecord::GetJailedPercentage() const noexcept {
    // Exact cSC4PoliceSimulator::GetJailedPercentage @ 0x1000a5054:
    //   if (field150 > field148) return 1.0f;
    //   return (Float32)field150 / (Float32)field148;
    if (field148 <= 0) {
        return (field150 > 0) ? 1.0f : 0.0f;
    }
    if (field150 > field148) {
        return 1.0f;
    }
    return static_cast<Float32>(field150) / static_cast<Float32>(field148);
}

bool CrimeSimulatorSaveRecord::GetCrimeHotspot(
    Sint32 tractSize, Uint8* outValue,
    Sint32* outWorldX, Sint32* outWorldZ) const noexcept {
    // Exact cSC4CrimeSimulator::GetCrimeHotspot @ 0x1001ff6ac.
    if (fieldD4 <= 0) {
        return false;
    }
    if (outValue) {
        *outValue = static_cast<Uint8>(fieldD4);
    }
    if (outWorldX) {
        *outWorldX = fieldCC * tractSize;
    }
    if (outWorldZ) {
        *outWorldZ = fieldD0 * tractSize;
    }
    return true;
}

void CrimeSimulatorSaveRecord::DecayCrimeGrid(std::span<Uint8> crimeGrid,
                                              Uint32 width, Uint32 height,
                                              Float32 decayFactor) noexcept {
    // Exact cSC4CrimeSimulator::DecayCrimeGrid @ 0x1001fead8.
    const std::size_t total =
        std::min(crimeGrid.size(),
                 static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
    for (std::size_t i = 0; i < total; ++i) {
        const Uint8 v = crimeGrid[i];
        if (v != 0u) {
            const Sint32 scaled =
                static_cast<Sint32>(static_cast<Float32>(v) * decayFactor);
            crimeGrid[i] = static_cast<Uint8>(std::clamp<Sint32>(scaled, 0, 255));
        }
    }
    if (fieldCC >= 0 && fieldD0 >= 0 &&
        static_cast<Uint32>(fieldCC) < width &&
        static_cast<Uint32>(fieldD0) < height) {
        const std::size_t idx =
            static_cast<std::size_t>(fieldCC) * static_cast<std::size_t>(height) +
            static_cast<std::size_t>(fieldD0);
        if (idx < crimeGrid.size()) {
            fieldD4 = static_cast<Sint32>(crimeGrid[idx]);
        }
    }
}

Float32 CrimeSimulatorSaveRecord::ComputeEconomicCrimeFactor(
    Float32 supply, Float32 demand, Float32 cap) noexcept {
    // Exact cSC4CrimeSimulator::CalculateEconomicFactors @ 0x1001fe8f2:
    // ((supply - cap) / (demand - cap)) * 2.0f - 1.0f.
    const Float32 denom = demand - cap;
    if (denom <= 0.0f) {
        return 0.0f;
    }
    return ((supply - cap) / denom) * 2.0f - 1.0f;
}

void AuraSimulatorSaveRecord::UpdateAuraGrid(
    std::span<const Sint8> baseAuraGrid,
    std::span<const Sint8> transientAuraGrid,
    std::span<const Uint16> resPopGrid,
    Uint32 width, Uint32 height, Sint32 tractSize,
    std::span<Sint8> outTotalAuraGrid) noexcept {
    // Exact cSC4AuraSimulator::UpdateAuraGrid @ 0x10021cc5c.
    field1F8 = static_cast<Sint16>(-128);
    field204 = static_cast<Sint16>(127);
    const std::size_t total =
        static_cast<std::size_t>(width) * static_cast<std::size_t>(height);
    if (baseAuraGrid.size() < total || transientAuraGrid.size() < total ||
        outTotalAuraGrid.size() < total) {
        return;
    }

    Sint64 weightedAuraSum = 0;
    Sint64 totalPop = 0;
    for (Uint32 x = 0; x < width; ++x) {
        for (Uint32 z = 0; z < height; ++z) {
            const std::size_t idx =
                static_cast<std::size_t>(x) * static_cast<std::size_t>(height) +
                static_cast<std::size_t>(z);
            const Sint8 baseVal = baseAuraGrid[idx];
            if (baseVal == static_cast<Sint8>(-128)) {
                outTotalAuraGrid[idx] = 0;
                continue;
            }
            const Sint32 sum = std::clamp<Sint32>(
                static_cast<Sint32>(baseVal) +
                    static_cast<Sint32>(transientAuraGrid[idx]),
                -127, 127);
            outTotalAuraGrid[idx] = static_cast<Sint8>(sum);

            if (sum < static_cast<Sint32>(field204)) {
                field1FC = static_cast<Sint32>(x) * tractSize;
                field200 = static_cast<Sint32>(z) * tractSize;
                field204 = static_cast<Sint16>(sum);
            }
            if (sum > static_cast<Sint32>(field1F8)) {
                field1F0 = static_cast<Sint32>(x) * tractSize;
                field1F4 = static_cast<Sint32>(z) * tractSize;
                field1F8 = static_cast<Sint16>(sum);
            }
            if (idx < resPopGrid.size()) {
                const Sint64 pop = static_cast<Sint64>(resPopGrid[idx]);
                totalPop += pop;
                weightedAuraSum += pop * static_cast<Sint64>(sum);
            }
        }
    }

    if (totalPop > 0) {
        fieldD4 = static_cast<Sint32>(weightedAuraSum / totalPop);
    }
}

void PowerSimulatorSaveRecord::StepMonthly() noexcept {
    // Exact cSC4PowerSimulator::DoMonthlySimulation @ 0x1002108c0:
    // advances the 12-month ring buffer index (0..11), subtracts expiring
    // monthly buckets from the rolling 12-month sums at +0x2b0, zeroes the
    // new month's buckets, and updates shortage ratio field94 (+0x94).
    field68 = (field68 > 10) ? 0 : (field68 + 1);
    const std::size_t slot = static_cast<std::size_t>(std::clamp<Sint32>(field68, 0, 11));
    for (std::size_t i = 0; i < 10; ++i) {
        field2B0[i] -= monthlyHistory[slot][i];
        monthlyHistory[slot][i] = 0;
    }
    const Sint32 surplus = field6C - field70;
    if (surplus < 0 && field6C > 0) {
        field94 = static_cast<Float32>(surplus / field6C);
    } else {
        field94 = 0.0f;
    }
    field3B0 = true;
    field3B1 = true;
}

void PollutionSimulatorSaveRecord::StepMonthly(Float32 garbageCostMultiplier) noexcept {
    // Exact cSC4PollutionSimulator::DoMonthlySimulation @ 0x100097cf2:
    // snapshots current-month accumulators (+0x250..+0x280, +0x2c4) into
    // previous-month fields (+0x288..+0x2b8, +0x2c8) and zeroes current-month.
    field288 = field250;
    field290 = field258;
    field298 = field260;
    field2A0 = field268;
    field2A8 = field270;
    field2B0 = field278;
    field2B8 = field280;
    field2C8 = field2C4;

    if (garbageCostMultiplier > 0.0f) {
        field284 = static_cast<Sint64>(
            std::llround(static_cast<Float64>(field290) *
                         static_cast<Float64>(garbageCostMultiplier)));
    }

    field250 = 0;
    field258 = 0;
    field260 = 0;
    field268 = 0.0;
    field270 = 0;
    field278 = 0;
    field280 = 0;
    field2C0 = 0;
}

// Public entry points for SC4ReadGZSerializable / SC4WriteGZSerializable and
// cSCPropertyHolder::Read / Write, shared with the .sc4 subfile decoders.
bool ReadSimSerializableRef(cIGZIStream& stream, SimSerializableRef& ref) noexcept {
    return ReadRef(stream, ref);
}

void WriteSimSerializableRef(cIGZOStream& stream, const SimSerializableRef& ref) {
    WriteRef(stream, ref);
}

bool ReadSCPropertyHolder(cIGZIStream& stream, SerializedPropertyHolder& holder) noexcept {
    return ReadSerializedPropertyHolder(stream, holder);
}

void WriteSCPropertyHolder(cIGZOStream& stream, const SerializedPropertyHolder& holder) {
    WriteSerializedPropertyHolder(stream, holder);
}

void CommercialSimulatorSaveRecord::StepIdleStateMachine() noexcept {
    // Exact cSC4CommercialSimulator::DoIdle @ 0x1001fc6ba:
    //   case 0: idleStep3C = 1;
    //   case 1: FloodFillDistance(gridRef58, ..., 3, 2); idleStep3C = 2; break;
    //   case 2: FloodFillDistance(gridRef60, ..., 3, 3); idleStep3C = 3; break;
    //   case 3: if (field38) { field38 = false; idleStep3C = 0; } break;
    switch (idleStep3C) {
    case 0:
    case 1:
        idleStep3C = 2u;
        break;
    case 2:
        idleStep3C = 3u;
        break;
    case 3:
        if (field38) {
            field38 = false;
            idleStep3C = 0u;
        }
        break;
    default:
        break;
    }
}

Uint8 FlammabilitySimulatorSaveRecord::ComputeScaledFlammabilityByte(
    Sint32 rawPropValue, Float32 mult) noexcept {
    // Exact cSC4FlammabilitySimulator::GetOccupantRawFlammability @ 0x1000c43a6:
    //   float f = float(rawPropValue) * mult + 12582912.0f (0x4b400000);
    //   int v = bit_cast<int>(f) - 0x4b400000;
    //   clamped to [0, 255].
    const Float32 magic = 12582912.0f; // 0x4b400000
    const Float32 sum = static_cast<Float32>(rawPropValue) * mult + magic;
    Uint32 bits = 0;
    std::memcpy(&bits, &sum, sizeof(bits));
    if (bits <= 0x4b400000u) {
        return 0u;
    }
    const Uint32 delta = bits - 0x4b400000u;
    return static_cast<Uint8>(delta >= 0x100u ? 0xffu : delta);
}

void FlammabilitySimulatorSaveRecord::ComputeHotspotList(
    std::span<const Sint32Triple> candidates,
    Sint32 mergeDistanceThreshold,
    Sint32 minClusterCount) noexcept {
    // Exact cSC4FlammabilitySimulator::ComputeHotspotList @ 0x1000c3efa:
    // clusters candidate (x, z, count) triples by Manhattan distance
    // (|x - cx| + |z - cz| < mergeDistanceThreshold), updating the running
    // integer centroid:
    //   cx = (cx * n + x) / (n + 1);
    //   cz = (cz * n + z) / (n + 1);
    //   n  = n + 1;
    // then prunes clusters with n < minClusterCount (+0xf0).
    entries.clear();
    for (const Sint32Triple& cand : candidates) {
        const Sint32 x = cand.a;
        const Sint32 z = cand.b;
        bool merged = false;
        for (Sint32Triple& cluster : entries) {
            const Sint32 dx = (x < cluster.a) ? (cluster.a - x) : (x - cluster.a);
            const Sint32 dz = (z < cluster.b) ? (cluster.b - z) : (z - cluster.b);
            if (dx + dz < mergeDistanceThreshold) {
                const Sint32 n = cluster.c;
                const Sint32 nextN = n + 1;
                cluster.a = (cluster.a * n + x) / nextN;
                cluster.b = (cluster.b * n + z) / nextN;
                cluster.c = nextN;
                merged = true;
                break;
            }
        }
        if (!merged) {
            entries.push_back(cand);
        }
    }
    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
                       [minClusterCount](const Sint32Triple& c) {
                           return c.c < minClusterCount;
                       }),
        entries.end());
}

Sint32 OrdinanceSimulatorSaveRecord::ComputeCumulativeOffset(
    std::span<const SerializedPropertyHolder> activeOrdinanceHolders,
    Uint32 propertyId, Uint32 index) noexcept {
    // Exact cSC4OrdinanceSimulator::GetCumulativeOffset @ 0x10079a594:
    // sums integer property values across all active ordinances.
    Sint32 total = 0;
    for (const SerializedPropertyHolder& h : activeOrdinanceHolders) {
        for (const SerializedPropertyEntry& p : h.properties) {
            if (p.propertyId != propertyId) {
                continue;
            }
            if ((p.dbType == 0x6u || p.dbType == 0x7u) && p.rawValueBytes.size() >= 4u) {
                Sint32 v = 0;
                std::memcpy(&v, p.rawValueBytes.data(), 4);
                total += v;
            } else if ((p.dbType == 0x8006u || p.dbType == 0x8007u) &&
                       (static_cast<std::size_t>(index) + 1u) * 4u <= p.rawValueBytes.size()) {
                Sint32 v = 0;
                std::memcpy(&v, p.rawValueBytes.data() + static_cast<std::size_t>(index) * 4u, 4);
                total += v;
            }
        }
    }
    return total;
}

Float32 OrdinanceSimulatorSaveRecord::ComputeCumulativeFactor(
    std::span<const SerializedPropertyHolder> activeOrdinanceHolders,
    Uint32 propertyId, Uint32 index) noexcept {
    // Exact cSC4OrdinanceSimulator::GetCumulativeFactor @ 0x10079a712:
    // multiplies Float32 property values across all active ordinances (starting at 1.0f).
    Float32 factor = 1.0f;
    for (const SerializedPropertyHolder& h : activeOrdinanceHolders) {
        for (const SerializedPropertyEntry& p : h.properties) {
            if (p.propertyId != propertyId) {
                continue;
            }
            Float32 v = 1.0f;
            if (p.dbType == 0xau && p.rawValueBytes.size() >= 4u) {
                std::memcpy(&v, p.rawValueBytes.data(), 4);
            } else if (p.dbType == 0x800au &&
                       (static_cast<std::size_t>(index) + 1u) * 4u <= p.rawValueBytes.size()) {
                std::memcpy(&v, p.rawValueBytes.data() + static_cast<std::size_t>(index) * 4u, 4);
            }
            factor *= v;
        }
    }
    return factor;
}

bool SimulatorSaveRecord::SetSimSpeed(Sint32 speed, const SimulatorTuning& tuning) noexcept {
    // Exact cSC4Simulator::SetSimSpeed @ 0x1001f093e.
    if (!speedChangeAllowed5C) {
        return false;
    }
    if (simSpeed54 == speed) {
        return true;
    }
    return DoSetSimSpeed(speed, tuning);
}

bool SimulatorSaveRecord::AddAgent(
    Uint64 targetId, Sint32 interval, std::string_view name, Uint32 flags) {
    // Exact cSC4Simulator::AddAgent @ 0x1001f1224.
    if (interval < 0 || interval > 10) {
        return true;
    }
    auto& set = agentSets[static_cast<std::size_t>(interval)];
    auto it = std::find_if(set.begin(), set.end(),
                           [targetId](const SimulatorAgentInfo& a) { return a.targetId == targetId; });
    if (it == set.end()) {
        const Uint32 mask = (isCityEstablished46 == 1u) ? 1u : 2u;
        SimulatorAgentInfo info;
        info.targetId = targetId;
        info.enabled = (flags & mask) != 0u;
        info.name.assign(name);
        info.flags = flags;
        set.push_back(std::move(info));
        std::sort(set.begin(), set.end(),
                  [](const SimulatorAgentInfo& a, const SimulatorAgentInfo& b) {
                      return a.targetId < b.targetId;
                  });
    }
    return true;
}

bool SimulatorSaveRecord::RemoveAgent(Uint64 targetId, Sint32 interval) noexcept {
    // Exact cSC4Simulator::RemoveAgent(target, int) @ 0x1001f13e6.
    if (interval < 0 || interval > 10) {
        return false;
    }
    auto& set = agentSets[static_cast<std::size_t>(interval)];
    auto it = std::find_if(set.begin(), set.end(),
                           [targetId](const SimulatorAgentInfo& a) { return a.targetId == targetId; });
    if (it == set.end()) {
        return false;
    }
    set.erase(it);
    return true;
}

bool SimulatorSaveRecord::RemoveAgent(Uint64 targetId) noexcept {
    // Exact cSC4Simulator::RemoveAgent(target) @ 0x1001f14ac.
    bool removedAny = false;
    for (Sint32 i = 0; i <= 10; ++i) {
        if (RemoveAgent(targetId, i)) {
            removedAny = true;
        }
    }
    return removedAny;
}

bool SimulatorSaveRecord::RemoveAllAgents(Sint32 interval) noexcept {
    // Exact cSC4Simulator::RemoveAllAgents(int) @ 0x1001f14f2.
    if (interval >= 0 && interval <= 10) {
        agentSets[static_cast<std::size_t>(interval)].clear();
    }
    return true;
}

bool SimulatorSaveRecord::RemoveAllAgents() noexcept {
    // Exact cSC4Simulator::RemoveAllAgents() @ 0x1001f1546.
    for (auto& set : agentSets) {
        set.clear();
    }
    return true;
}

void SimulatorSaveRecord::EnumerateAgentsByName(std::vector<std::string>& outNames) const {
    // Exact cSC4Simulator::EnumerateAgentsByName @ 0x1001f15a4.
    outNames.clear();
    for (const auto& set : agentSets) {
        for (const SimulatorAgentInfo& a : set) {
            if (std::find(outNames.begin(), outNames.end(), a.name) == outNames.end()) {
                outNames.push_back(a.name);
            }
        }
    }
}

bool SimulatorSaveRecord::GetAgentEnabled(std::string_view name) const noexcept {
    // Exact cSC4Simulator::GetAgentEnabled @ 0x1001f178e.
    for (const auto& set : agentSets) {
        for (const SimulatorAgentInfo& a : set) {
            if (a.name == name && a.enabled) {
                return true;
            }
        }
    }
    return false;
}

void SimulatorSaveRecord::SetAgentEnabled(std::string_view name, bool enabled) noexcept {
    // Exact cSC4Simulator::SetAgentEnabled @ 0x1001f182c.
    for (auto& set : agentSets) {
        for (SimulatorAgentInfo& a : set) {
            if (a.name == name) {
                a.enabled = enabled;
            }
        }
    }
}

void SimulatorSaveRecord::SetCityEstablished(bool established) noexcept {
    // Exact cSC4Simulator::SetCityEstablished @ 0x1001f18ca.
    const bool current = (isCityEstablished46 != 0u);
    if (current == established) {
        return;
    }
    isCityEstablished46 = established ? 1u : 0u;
    const Uint32 mask = established ? 1u : 2u;
    for (auto& set : agentSets) {
        for (SimulatorAgentInfo& a : set) {
            a.enabled = (a.flags & mask) != 0u;
        }
    }
}

Float32 SimulatorSaveRecord::GetAnimationTimeDilation(
    const SimulatorTuning& tuning, bool vehicleManagerActive) const noexcept {
    // Exact cSC4Simulator::GetAnimationTimeDilation @ 0x1001f1952.
    std::size_t idx = 1u;
    if (IsHiddenPaused() || IsEmergencyPaused() || vehicleManagerActive) {
        idx = 4u;
    } else if (IsPaused()) {
        idx = 0u;
    } else if (simSpeed54 == 2) {
        idx = 3u;
    } else if (simSpeed54 == 1) {
        idx = 2u;
    } else {
        idx = 1u;
    }
    return idx < tuning.animationTimeDilation.size() ? tuning.animationTimeDilation[idx] : 1.0f;
}

void LandValueSimulatorSaveRecord::SetupAltitudeEffect(Float32 terrainMaxAltitude) noexcept {
    // Exact cSC4LandValueSimulator::SetupAltitudeEffect @ 0x1000c6e66.
    const Float32 mid = std::min(modeAlt108, fieldCC);
    midAltAC = mid;
    lowerSpanB0 = std::max(1.0f, mid - minAlt100);
    upperSpanB4 = std::max(1.0f, maxAlt104 - mid);
    maxEffectIndexB8 = 100;
    if (upperSpanB4 > 100.0f) {
        const Float32 scaled =
            ((upperSpanB4 - 100.0f) / (terrainMaxAltitude - 100.0f)) * 100.0f;
        const Sint32 candidate =
            static_cast<Sint32>(static_cast<Float32>(maxEffectIndexB8) + scaled);
        maxEffectIndexB8 = std::min<Sint32>(candidate, 150);
    }
}

Uint8 LandValueSimulatorSaveRecord::LookupAltitudeEffect(
    Float32 altitude, const cSC4ResponseCurve& altitudeCurve) const noexcept {
    // Exact cSC4LandValueSimulator::LookupAltitudeEffect @ 0x1000c6f32.
    if (altitude < minAlt100) {
        return 0u;
    }
    Float32 pct = 0.0f;
    if (altitude < midAltAC) {
        pct = ((altitude - minAlt100) * 50.0f) / lowerSpanB0;
    } else {
        pct = ((altitude - midAltAC) * static_cast<Float32>(maxEffectIndexB8 - 50)) /
                  upperSpanB4 +
              50.0f;
    }
    const Sint32 intPct = static_cast<Sint32>(pct);
    const Float32 out = altitudeCurve.GetOutputValue(static_cast<Float32>(intPct));
    return static_cast<Uint8>(static_cast<Sint32>(out));
}

Float32 LandValueSimulatorSaveRecord::ComputeLotAreaSlope(
    const LandValueIntRect& rect, bool flag, LandValueLotVertexAltitude vertexAltitude,
    void* lotDeveloper) noexcept {
    // Exact cSC4LotDeveloper::GetAreaSlope(const SC4Rect<int>&, bool) @ 0x1001478e8.
    // Rect field order is +0x00=minX, +0x04=minZ, +0x08=maxX, +0x0c=maxZ. The
    // scan is over the inclusive VERTEX bounds X in [minX..maxX+1] and Z in
    // [minZ..maxZ+1] (@ 0x100147960-0x1001479ea do/while loops). Each vertex
    // altitude comes from the lot developer virtual +0x78 (@ 0x100147984).
    // The empty-rect branch (minX > maxX+1 @ 0x100147905) reads uninitialized
    // locals in the binary and is unreachable for valid rects; return 0.
    if (rect.minX > rect.maxX + 1) {
        return 0.0f;
    }

    Float32 highValue = -3.4028234663852886e+38f; // -FLT_MAX @ 0x100a169a0
    Float32 lowValue = 3.4028234663852886e+38f;   // +FLT_MAX @ 0x100a161e0
    Float32 lowX = 0.0f, lowZ = 0.0f, highX = 0.0f, highZ = 0.0f;

    for (Sint32 x = rect.minX; x <= rect.maxX + 1; ++x) {
        for (Sint32 z = rect.minZ; z <= rect.maxZ + 1; ++z) {
            const Float32 altitude = vertexAltitude(lotDeveloper, x, z, flag);
            // Strict comparisons: the first minimum and the last strict maximum
            // win (@ 0x10014798c-0x1001479ca).
            if (altitude < lowValue) {
                lowX = static_cast<Float32>(x);
                lowZ = static_cast<Float32>(z);
                lowValue = altitude;
            }
            if (altitude > highValue) {
                highX = static_cast<Float32>(x);
                highZ = static_cast<Float32>(z);
                highValue = altitude;
            }
        }
    }

    // LengthOfLine2D @ 0x1000d6b98: sqrt((bx-ax)^2 + (by-ay)^2).
    const Float32 dx = highX - lowX;
    const Float32 dz = highZ - lowZ;
    Float32 distance = std::sqrt(dx * dx + dz * dz);
    if (distance == 0.0f) {
        return 0.0f; // @ 0x100147a00-0x100147a05
    }
    // Distance-compression cascade (@ 0x100147a07-0x100147a6d). The first two
    // are independent ifs (both can fire); the last pair is if/else-if.
    if (distance > 6.0f) { // 0x100a18ed0
        distance *= 0.25f;  // 0x100a169a8
    }
    if (distance > 5.0f) { // 0x100a16210
        distance *= 0.3f;  // 0x100a18a4c
    }
    if (distance > 4.0f) {         // 0x100a161fc
        distance *= 0.35f;         // 0x100a18ed8
    } else if (distance > 3.0f) {  // 0x100a165b8
        distance *= 0.4f;          // 0x100a18ed4
    }
    return (highValue - lowValue) / distance; // @ 0x100147a71-0x100147a7b
}

bool LandValueSimulatorSaveRecord::TractIsZoned(
    const cSC4SimGrid<Uint8>& intrinsicGrid, const cSC4SimGrid<Sint8>& zoneGrid,
    Sint32 tractX, Sint32 tractZ) noexcept {
    // Exact cSC4LandValueSimulator::TractIsZoned @ 0x1000c6d54. The tract
    // origin uses GetTractShift and the half-open bounds use GetTractSize
    // (@ 0x1000c6d6b-0x1000c6d99). X is the outer loop and Z the inner loop;
    // ANY nonzero cSC4ZoneManager::GetZoneType @ 0x100199e64 returns true
    // immediately (@ 0x1000c6d9c-0x1000c6dc6). Nonpositive tract sizes and
    // exhausted scans return false (@ 0x1000c6d8c-0x1000c6dc4).
    const Sint32 tractShift = intrinsicGrid.GetTractShift();
    const Sint32 tractSize = intrinsicGrid.GetTractSize();
    const Sint32 minX = tractX << tractShift;
    const Sint32 minZ = tractZ << tractShift;
    if (tractSize <= 0) {
        return false;
    }
    const Sint32 maxX = minX + tractSize;
    const Sint32 maxZ = minZ + tractSize;
    for (Sint32 x = minX; x < maxX; ++x) {
        for (Sint32 z = minZ; z < maxZ; ++z) {
            if (zoneGrid.GetCellValue(x, z) != 0) {
                return true;
            }
        }
    }
    return false;
}

bool LandValueSimulatorSaveRecord::TractHasLand(const cSC4SimGrid<Uint8>& waterGrid,
                                             Uint32 tractX, Uint32 tractZ, Uint32 tractSize) noexcept {
    // Exact cSC4LandValueSimulator::TractHasLand @ 0x1000c6dd8 (disassembly
    // 0x1000c6de0-0x1000c6e62): inclusive do/while sampling over (tractSize + 1)
    // coordinates per axis; ANY cell where cSTETerrain::CellHasWater @ 0x10073562c
    // returns false — including out-of-range coordinates — counts as land.
    const Sint32 minX = static_cast<Sint32>(tractX * tractSize);
    const Sint32 maxCityX = minX + static_cast<Sint32>(tractSize);
    const Sint32 minZ = static_cast<Sint32>(tractZ * tractSize);
    const Sint32 maxCityZ = minZ + static_cast<Sint32>(tractSize);
    Sint32 cz = minZ;
    do {
        Sint32 cx = minX;
        do {
            if (cx < 0 || cz < 0 || cx >= 128 || cz >= 128 ||
                waterGrid.GetCellValue(cx, cz) == 0) {
                return true;
            }
            ++cx;
        } while (cx <= maxCityX);
        ++cz;
    } while (cz <= maxCityZ);
    return false;
}

namespace {

using LandValueByteGrid = cISC4SimGrid<Uint8>::GridData;

// State 7 AddDesirability callback context (@ 0x1000c6717-0x1000c6729): the
// Float32 weight (map node +0x28, read as float @ 0x1000c7367) and the
// desirability response curve ([this+0xe0]).
struct LandValueDesirabilityContext {
    Float32 weight = 0.0f;                     // ctx+0x00
    const cSC4ResponseCurve* curve = nullptr;  // ctx+0x08
};

// Exact AddDesirability @ 0x1000c7340: dest = clamp_1_255(truncate(old +
// curve(desirability) * weight)); when curve is null the raw desirability is
// used in place of curve(desirability) (@ 0x1000c734d-0x1000c735a).
void LandValueAddDesirability(Uint8& dest, Sint16 desirability, void* context) {
    auto* ctx = static_cast<LandValueDesirabilityContext*>(context);
    Float32 value = static_cast<Float32>(desirability); // cvtsi2ss @ 0x1000c7354
    if (ctx->curve != nullptr) {
        value = ctx->curve->GetOutputValue(value); // @ 0x1000c735a
    }
    const Sint32 old = static_cast<Sint32>(dest); // movzx @ 0x1000c735f
    // (curve * weight) + old, preserving the mulss-then-addss order
    // (@ 0x1000c7367-0x1000c736b).
    const Float32 sum = value * ctx->weight + static_cast<Float32>(old);
    Sint32 clamped = static_cast<Sint32>(sum); // cvttss2si @ 0x1000c736f
    if (clamped <= 0) {
        clamped = 1; // cmovg: ecx > 0 ? ecx : 1 @ 0x1000c7373-0x1000c737a
    }
    if (clamped >= 255) {
        clamped = 255; // jl keeps < 0xff, else 0xff @ 0x1000c737d-0x1000c7386
    }
    dest = static_cast<Uint8>(clamped);
}

// Shared 0x20-byte scanning-iterator reset used at State 2 completion
// (@ 0x1000c60c2-0x1000c60e5 / 0x1000c6998), State 3 fallback
// (@ 0x1000c6bec-0x1000c6c0f / 0x1000c6c99), and State 3 completion
// (@ 0x1000c6b63-0x1000c6b86 / 0x1000c6c7b). Coordinates and status are
// cleared first; status 2 is stored only for the binary's empty-rectangle
// branch conditions.
void ResetLandValueIterator(IntrinsicGridIteratorState& iterator) noexcept {
    iterator.x = 0;
    iterator.z = 0;
    iterator.status = 0;
    if (iterator.minX > 0 || iterator.minZ > 0 || iterator.maxX < 0 || iterator.maxZ < 0) {
        iterator.status = 2;
    }
}

// Exact Expand @ 0x1000d7d0c. The traversal is x-outer/z-inner
// (@ 0x1000d7d4a-0x1000d7e34); each matching cell visits previous-x, next-x,
// previous-z, next-z in that order (@ 0x1000d7d98-0x1000d7e0f). Expansion is
// in place and writes only when the neighbour is strictly greater than the
// wrapped Uint8 distance + 1.
bool MapUtilitiesExpand(LandValueByteGrid& distances, Sint32 width, Sint32 height,
                        Uint8 distance) {
    if (width <= 0) {
        return false; // @ 0x1000d7d19-0x1000d7d1f / 0x1000d7e3c
    }
    const Uint8 nextDistance = static_cast<Uint8>(distance + 1u); // @ 0x1000d7d21-0x1000d7d24
    bool changed = false;
    for (Sint32 x = 0; x < width; ++x) {
        const Sint32 previousX = x <= 1 ? 0 : x - 1; // @ 0x1000d7d5b-0x1000d7d6d
        const Sint32 nextX = x + 1 < width ? x + 1 : width - 1; // @ 0x1000d7d71-0x1000d7d83
        for (Sint32 z = 0; z < height; ++z) {
            if (distances[x][z] != distance) {
                continue; // @ 0x1000d7d8f-0x1000d7d96
            }
            if (distances[previousX][z] > nextDistance) {
                distances[previousX][z] = nextDistance;
                changed = true; // @ 0x1000d7d98-0x1000d7dad
            }
            if (distances[nextX][z] > nextDistance) {
                distances[nextX][z] = nextDistance;
                changed = true; // @ 0x1000d7daf-0x1000d7dc4
            }
            const Sint32 previousZ = z <= 1 ? 0 : z - 1; // @ 0x1000d7dc6-0x1000d7dd8
            if (distances[x][previousZ] > nextDistance) {
                distances[x][previousZ] = nextDistance;
                changed = true; // @ 0x1000d7ddc-0x1000d7df1
            }
            const Sint32 nextZ = z + 1 < height ? z + 1 : height - 1; // @ 0x1000d7df3-0x1000d7e00
            if (distances[x][nextZ] > nextDistance) {
                distances[x][nextZ] = nextDistance;
                changed = true; // @ 0x1000d7e03-0x1000d7e0f
            }
        }
    }
    return changed; // @ 0x1000d7e3e
}

// Exact cSC4MapUtilities::AddDistanceEffect @ 0x1000d7af8. The temporary
// distance grid starts at 255 (@ 0x1000d7b24-0x1000d7bce); source cells equal
// to exclusion become zero-distance seeds (@ 0x1000d7bd9-0x1000d7c19).
void MapUtilitiesAddDistanceEffect(LandValueByteGrid& target, Sint32 width, Sint32 height,
                                   const LandValueByteGrid& source, Uint8 exclusion,
                                   Float32 maxDistance, Float32 nearStrength,
                                   Float32 farStrength) {
    LandValueByteGrid distances;
    distances.resize(width);
    for (auto& row : distances) {
        row.resize(height, Uint8{0xff});
    }
    for (Sint32 x = 0; x < width; ++x) {
        for (Sint32 z = 0; z < height; ++z) {
            if (source[x][z] == exclusion) {
                distances[x][z] = 0;
            }
        }
    }

    // cvttss2si bounds the Expand call count; Expand receives the low Uint8 of
    // the counter (@ 0x1000d7bd0-0x1000d7c3d).
    const Sint32 expansionCount = static_cast<Sint32>(maxDistance);
    if (expansionCount > 0) {
        for (Uint32 distance = 0; distance != static_cast<Uint32>(expansionCount); ++distance) {
            MapUtilitiesExpand(distances, width, height, static_cast<Uint8>(distance));
        }
    }
    if (width <= 0) {
        return; // @ 0x1000d7c3f-0x1000d7c4c
    }

    // Keep the binary's float operation order: (near - far) / maxDistance once,
    // then old + near first, distance * falloff second, and subtraction last
    // (@ 0x1000d7c52-0x1000d7c5a / 0x1000d7c8f-0x1000d7cae).
    const Float32 falloff = (nearStrength - farStrength) / maxDistance;
    for (Sint32 x = 0; x < width; ++x) {
        for (Sint32 z = 0; z < height; ++z) {
            const Uint8 distance = distances[x][z];
            if (distance == 0) {
                continue; // @ 0x1000d7c7b-0x1000d7c81
            }
            const Float32 distanceValue = static_cast<Float32>(distance);
            if (!(distanceValue < maxDistance)) {
                continue; // strict ordered 0 < distance < maxDistance @ 0x1000d7c83-0x1000d7c8d
            }
            const Float32 oldPlusNear = static_cast<Float32>(target[x][z]) + nearStrength;
            const Float32 distanceEffect = distanceValue * falloff;
            const Float32 result = oldPlusNear - distanceEffect;
            Sint32 cellValue = static_cast<Sint32>(result); // cvttss2si @ 0x1000d7cae
            if (cellValue < 0) {
                cellValue = 0;
            }
            if (cellValue >= 255) {
                cellValue = 255; // inclusive upper clamp @ 0x1000d7cb7-0x1000d7cbf
            }
            target[x][z] = static_cast<Uint8>(cellValue); // @ 0x1000d7cc1
        }
    }
}

} // namespace

void LandValueSimulatorSaveRecord::DoIdle(unsigned int) noexcept {
    // Exact cSC4LandValueSimulator::DoIdle @ 0x1000c5e64 (jump table @ 0x1000c6d10).
    // State machine byte [+0x68]: 0=reset/bounds, 1=intrinsic fill, 2=distance
    // effect, 3=altitude stats, 4=altitude apply (TODO), 5=slope
    // grid (TODO), 6=desirability copy (TODO), 7=growth-developer grids (TODO),
    // 8=average/history (TODO), 9=idle-return.
    switch (idleState) {
    case 0: {
        // Reset @ 0x1000c5eb3: zero the iterator coordinates/status (one 64-bit store),
        // then apply the binary's exact emptiness checks: minX ([+0x10]) > 0 or
        // minZ ([+0x14]) <= 0 jumps directly to state 2 (0x1000c697f/0x1000c5ed3).
        intrinsicIterator.x = 0;
        intrinsicIterator.z = 0;
        intrinsicIterator.status = 0;
        if (intrinsicIterator.minX > 0 || intrinsicIterator.minZ <= 0) {
            idleState = 2;
            break;
        }
        idleState = 1;
        [[fallthrough]];
    }
    case 1: {
        // State 1 @ 0x1000c5f90-0x1000c6008: fill the working intrinsic grid
        // [+0x50] with fieldD1 on tracts where TractHasLand is true, zero
        // elsewhere (byte-level fill, binary [this+0xd1] @ 0x1000c5fee).
        if (workingGrid != nullptr && waterGrid != nullptr && tuning != nullptr) {
            const Uint32 tractSize = static_cast<Uint32>(tuning->fieldD0);
            const Uint8 fillValue = fieldD1;
            while (intrinsicIterator.status != 2) {
                if (TractHasLand(*waterGrid, intrinsicIterator.x, intrinsicIterator.z, tractSize)) {
                    workingGrid->SetTractValue(static_cast<int>(intrinsicIterator.x),
                                               static_cast<int>(intrinsicIterator.z), fillValue);
                } else {
                    workingGrid->SetTractValue(static_cast<int>(intrinsicIterator.x),
                                               static_cast<int>(intrinsicIterator.z), 0);
                }
                intrinsicIterator.x += intrinsicIterator.step;
                if (intrinsicIterator.x > intrinsicIterator.maxX) {
                    intrinsicIterator.x = intrinsicIterator.minX;
                    intrinsicIterator.z += intrinsicIterator.step;
                    if (intrinsicIterator.z > intrinsicIterator.maxZ) {
                        intrinsicIterator.status = 2;
                        break;
                    }
                }
            }
        }
        idleState = 2; // @ 0x1000c69b4 (iterator completion -> state 2)
        break;
    }
    case 2: {
        // State 2 @ 0x1000c6038: target and source are both the working grid
        // [+0x50], dimensions come from the intrinsic grid [+0x40], exclusion
        // and far strength are zero, maximum distance is fieldD6, and near
        // strength is fieldD5 (@ 0x1000c603d-0x1000c608d).
        if (workingGrid != nullptr && intrinsicGrid != nullptr) {
            LandValueByteGrid& grid = workingGrid->GetGridData();
            MapUtilitiesAddDistanceEffect(
                grid, intrinsicGrid->GetTractCountX(), intrinsicGrid->GetTractCountZ(), grid, 0,
                static_cast<Float32>(fieldD6), static_cast<Float32>(fieldD5), 0.0f);
        }
        // State 3 entry initialization: terrain maximum initializes minAlt100
        // and terrain minimum initializes maxAlt104 (@ 0x1000c6092-0x1000c60b8).
        if (terrainGetMaxAltitude != nullptr && terrainGetMinAltitude != nullptr) {
            minAlt100 = terrainGetMaxAltitude(terrainContext);
            maxAlt104 = terrainGetMinAltitude(terrainContext);
        }
        ResetLandValueIterator(intrinsicIterator); // @ 0x1000c60c2-0x1000c60e5
        altitudeFallback88 = false; // @ 0x1000c60e5
        altitudeSum80 = 0;
        altitudeSampleCount84 = 0; // one 64-bit store @ 0x1000c60ee
        std::fill(altitudeHistogram90.begin(), altitudeHistogram90.end(), 0);
        // Histogram zeroing uses its full begin-to-end Sint32 range
        // (@ 0x1000c60fa-0x1000c613d).
        idleState = 3; // State 2 completion @ 0x1000c613f (not State 1's store @ 0x1000c69b4)
        break;
    }
    case 3: {
        // State 3 @ 0x1000c614d: sample terrain at every tract center, track
        // extrema over land, and accumulate altitude statistics over zoned land.
        // The all-land fallback is selected only after an empty first pass.
        if (intrinsicGrid == nullptr || waterGrid == nullptr || zoneGrid == nullptr ||
            terrainGetAltitude == nullptr || terrainGetMaxAltitude == nullptr) {
            break;
        }
        while (intrinsicIterator.status != 2) {
            // Status 0 starts at the current coordinates. Status 1 advances
            // before the next sample, resetting X to minX at row completion
            // and storing status 2 after the final row (@ 0x1000c61f1-0x1000c6231).
            if (intrinsicIterator.status == 0) {
                intrinsicIterator.status = 1;
            } else {
                intrinsicIterator.x += intrinsicIterator.step;
                if (intrinsicIterator.x > intrinsicIterator.maxX) {
                    intrinsicIterator.x = intrinsicIterator.minX;
                    intrinsicIterator.z += intrinsicIterator.step;
                    if (intrinsicIterator.z > intrinsicIterator.maxZ) {
                        intrinsicIterator.status = 2;
                        break;
                    }
                }
            }

            // TractHasLand is evaluated before the center conversion and terrain
            // sample; both of those calls occur even when the tract has no land
            // (@ 0x1000c6231-0x1000c6274). TractCenterToPosition is exact at
            // 0x1000c7520 and GetAltitude at 0x100738928.
            const bool hasLand = TractHasLand(
                *waterGrid, static_cast<Uint32>(intrinsicIterator.x),
                static_cast<Uint32>(intrinsicIterator.z),
                static_cast<Uint32>(intrinsicGrid->GetTractSize()));
            Float32 worldX = 0.0f;
            Float32 worldZ = 0.0f;
            intrinsicGrid->TractCenterToPosition(
                intrinsicIterator.x, intrinsicIterator.z, worldX, worldZ);
            const Float32 altitude = terrainGetAltitude(terrainContext, worldX, worldZ);

            // Land-only extrema updates use strict ordered comparisons and are
            // independent of zoning/fallback sampling (@ 0x1000c6274-0x1000c62a7).
            if (hasLand) {
                if (minAlt100 > altitude) {
                    minAlt100 = altitude;
                }
                if (altitude > maxAlt104) {
                    maxAlt104 = altitude;
                }
            }

            bool accumulate = false;
            if (altitudeFallback88) {
                accumulate = hasLand; // all-land pass @ 0x1000c62a7-0x1000c62b7
            } else {
                // The binary evaluates TractIsZoned even when hasLand is false,
                // then tests the two predicate results (@ 0x1000c62b9-0x1000c62d3).
                const bool isZoned = TractIsZoned(
                    *intrinsicGrid, *zoneGrid, intrinsicIterator.x, intrinsicIterator.z);
                accumulate = hasLand && isZoned;
            }
            if (!accumulate) {
                continue;
            }

            // Preserve the per-tract integer -> Float32 -> add -> truncate
            // sequence rather than using a floating-point accumulator
            // (@ 0x1000c62d5-0x1000c62ea).
            const Float32 sumAsFloat = static_cast<Float32>(altitudeSum80);
            const Float32 sumPlusAltitude = sumAsFloat + altitude;
            altitudeSum80 = static_cast<Sint32>(sumPlusAltitude);
            ++altitudeSampleCount84; // @ 0x1000c62f2

            // The truncated altitude is arithmetic-shifted by fieldA8 (5 from
            // Init @ 0x1000c5477), negative buckets clamp to zero, and buckets
            // beyond the vector clamp to its final element (@ 0x1000c62fa-0x1000c6330).
            Sint32 bucket = static_cast<Sint32>(altitude);
            bucket >>= static_cast<Uint8>(fieldA8);
            if (bucket < 0) {
                bucket = 0;
            }
            const Sint32 histogramSize = static_cast<Sint32>(altitudeHistogram90.size());
            if (histogramSize <= bucket) {
                bucket = histogramSize - 1;
            }
            ++altitudeHistogram90[static_cast<std::size_t>(bucket)];
        }

        // Iterator completion @ 0x1000c6a7b. Average calculation always clears
        // fieldCC first, then performs two integer-to-Float32 conversions and
        // one division when the signed sample count is positive
        // (@ 0x1000c6a7b-0x1000c6ab5).
        fieldCC = 0.0f;
        if (altitudeSampleCount84 > 0) {
            const Float32 sumAsFloat = static_cast<Float32>(altitudeSum80);
            const Float32 countAsFloat = static_cast<Float32>(altitudeSampleCount84);
            fieldCC = sumAsFloat / countAsFloat;
        } else if (!altitudeFallback88) {
            // An empty zoned pass rewinds for one all-land pass, preserving the
            // extrema but clearing the sum/count and every histogram element
            // (@ 0x1000c6ab7-0x1000c6ac6 / 0x1000c6bec-0x1000c6c76).
            ResetLandValueIterator(intrinsicIterator);
            altitudeSum80 = 0;
            altitudeSampleCount84 = 0;
            altitudeFallback88 = true;
            std::fill(altitudeHistogram90.begin(), altitudeHistogram90.end(), 0);
            idleState = 3; // @ 0x1000c6c3b-0x1000c6c40 / 0x1000c6b8b
            break;
        }

        // Select the earliest bucket whose count is strictly greater than the
        // best count seen so far. The separate >= store reproduces cmovge;
        // equal counts never replace the selected index (@ 0x1000c6ac6-0x1000c6b00).
        Sint32 modeIndex = 0;
        Sint32 bestCount = 0;
        const Sint32 histogramSize = static_cast<Sint32>(altitudeHistogram90.size());
        for (Sint32 index = 0; index < histogramSize; ++index) {
            const Sint32 count = altitudeHistogram90[static_cast<std::size_t>(index)];
            if (count > bestCount) {
                modeIndex = index;
            }
            if (count >= bestCount) {
                bestCount = count;
            }
        }

        // The modal altitude is the center of its 32-unit bucket, converted to
        // Float32 and clamped first to the minimum and then the maximum
        // (@ 0x1000c6b00-0x1000c6b5b).
        const Sint32 modeInteger =
            (modeIndex << fieldA8) + (1 << (fieldA8 - 1));
        modeAlt108 = static_cast<Float32>(modeInteger);
        if (minAlt100 > modeAlt108) {
            modeAlt108 = minAlt100;
        }
        if (modeAlt108 > maxAlt104) {
            modeAlt108 = maxAlt104;
        }
        SetupAltitudeEffect(terrainGetMaxAltitude(terrainContext)); // call @ 0x1000c6b5b
        ResetLandValueIterator(slopeIterator); // @ 0x1000c6b63-0x1000c6b86
        idleState = 5; // exact State 3 completion store @ 0x1000c6b86-0x1000c6b8b
        break;
    }
    case 4: {
        // Exact State 4 @ 0x1000c6363-0x1000c650d: the altitude-effect apply
        // pass. Reached from State 5's completion (5 -> 4 @ 0x1000c69e4); on
        // completion it resets the intrinsic iterator and stores state 6
        // (@ 0x1000c6bb4). Reader = working grid [+0x50] (@ 0x1000c64b7), writer
        // = intrinsic grid [+0x40] (@ 0x1000c64d6); a null response curve [+0xd8]
        // skips the pass entirely (@ 0x1000c6423-0x1000c642c).
        if (intrinsicGrid != nullptr && workingGrid != nullptr &&
            terrainGetAltitude != nullptr && altitudeCurve != nullptr) {
            while (intrinsicIterator.status != 2) {
                if (intrinsicIterator.status == 0) {
                    intrinsicIterator.status = 1;
                } else {
                    intrinsicIterator.x += intrinsicIterator.step;
                    if (intrinsicIterator.x > intrinsicIterator.maxX) {
                        intrinsicIterator.x = intrinsicIterator.minX;
                        intrinsicIterator.z += intrinsicIterator.step;
                        if (intrinsicIterator.z > intrinsicIterator.maxZ) {
                            intrinsicIterator.status = 2;
                            break;
                        }
                    }
                }
                // TractCenterToPosition via intrinsic grid vfunc +0xa8
                // (@ 0x1000c648b), then GetAltitude via terrain vfunc +0x60
                // (@ 0x1000c64a3), then LookupAltitudeEffect (@ 0x1000c64a9).
                Float32 worldX = 0.0f;
                Float32 worldZ = 0.0f;
                intrinsicGrid->TractCenterToPosition(
                    intrinsicIterator.x, intrinsicIterator.z, worldX, worldZ);
                const Float32 altitude = terrainGetAltitude(terrainContext, worldX, worldZ);
                const Uint8 effect = LookupAltitudeEffect(altitude, *altitudeCurve);
                const Sint32 source = static_cast<Sint32>(
                    workingGrid->GetTractValue(intrinsicIterator.x, intrinsicIterator.z));
                Sint32 applied = source + static_cast<Sint32>(effect);
                if (applied >= 255) {
                    applied = 255; // jb keeps < 0xff, else 0xff @ 0x1000c64cc-0x1000c64d4
                }
                intrinsicGrid->SetTractValue(intrinsicIterator.x, intrinsicIterator.z,
                                             static_cast<Uint8>(applied));
            }
        }
        ResetLandValueIterator(intrinsicIterator); // @ 0x1000c6b95-0x1000c6baf
        idleState = 6;                             // @ 0x1000c6bb4
        break;
    }
    case 5: {
        // Exact State 5 @ 0x1000c650d: fill the float slope grid [+0x58] with
        // GetAreaSlope(rect, true) per tract. lotDeveloper = GetLotDeveloper via
        // SL::spBuildingDevelopmentSim vtable +0x120 (@ 0x1000c65d2); the shift
        // comes from the slope grid's GetTractShift (vtable +0x58 @ 0x1000c65c0);
        // the rect is {x<<shift, z<<shift, ((x+1)<<shift)-1, ((z+1)<<shift)-1}
        // (@ 0x1000c662b-0x1000c6650) and the Float32 result is stored at
        // slope[x][z] (@ 0x1000c667a). Completion (slope-iterator status 2
        // @ 0x1000c65ee, or maxZ exceeded @ 0x1000c661b) resets the intrinsic
        // iterator and stores state 4 (@ 0x1000c69c2-0x1000c69e4).
        if (slopeGrid != nullptr && getLotVertexAltitude != nullptr) {
            void* lotDeveloper = getLotDeveloper != nullptr
                                     ? getLotDeveloper(buildingDevContext)
                                     : nullptr;
            const Sint32 shift = slopeGrid->GetTractShift(); // vtable +0x58 @ 0x1000c65c0
            while (slopeIterator.status != 2) {
                if (slopeIterator.status == 0) {
                    slopeIterator.status = 1;
                } else {
                    slopeIterator.x += slopeIterator.step;
                    if (slopeIterator.x > slopeIterator.maxX) {
                        slopeIterator.x = slopeIterator.minX;
                        slopeIterator.z += slopeIterator.step;
                        if (slopeIterator.z > slopeIterator.maxZ) {
                            slopeIterator.status = 2;
                            break;
                        }
                    }
                }
                const Sint32 tractX = slopeIterator.x;
                const Sint32 tractZ = slopeIterator.z;
                LandValueIntRect rect;
                rect.minX = tractX << shift;
                rect.minZ = tractZ << shift;
                rect.maxX = ((tractX + 1) << shift) - 1;
                rect.maxZ = ((tractZ + 1) << shift) - 1;
                const Float32 slope = ComputeLotAreaSlope(
                    rect, true, getLotVertexAltitude, lotDeveloper);
                slopeGrid->SetTractValue(tractX, tractZ, slope); // @ 0x1000c667a
            }
        }
        ResetLandValueIterator(intrinsicIterator); // @ 0x1000c69c5-0x1000c69df
        idleState = 4;                             // @ 0x1000c69e4
        break;
    }
    case 6: {
        // Exact State 6 @ 0x1000c66ad: copy the intrinsic grid [+0x40] Data into
        // the desirability working grid [+0x50] (vector assign @ 0x10010f188,
        // only when the two GridData objects differ, @ 0x1000c66cc-0x1000c66db),
        // then arm the growth-developer traversal cursor [+0xc0] from the head
        // pointer [+0xe8] (@ 0x1000c66e0-0x1000c66e8) and store state 7
        // (@ 0x1000c66f0).
        if (intrinsicGrid != nullptr && workingGrid != nullptr) {
            LandValueByteGrid& dst = workingGrid->GetGridData();
            const LandValueByteGrid& src = intrinsicGrid->GetGridData();
            if (&dst != &src) {
                dst = src; // std::vector<vector<Uint8>>::assign @ 0x10010f188
            }
        }
        growthDeveloperCursor = growthDeveloperHead; // @ 0x1000c66e0-0x1000c66e8
        idleState = 7;                               // @ 0x1000c66f0
        break;
    }
    case 7: {
        // Exact State 7 @ 0x1000c66fe: one growth-developer desirability grid
        // per invocation. If the cursor [+0xc0] equals the end sentinel
        // &[this+0xf0] (@ 0x1000c670e-0x1000c6711), run the fallback: copy the
        // desirability grid [+0x50] into the land-value grid [+0x48]
        // (@ 0x1000c68e5-0x1000c6913), reset the intrinsic iterator
        // (@ 0x1000c6918-0x1000c6932), zero [+0x80] (@ 0x1000c693b) and store
        // state 8 (@ 0x1000c6947). Otherwise read the node's developer [+0x20]
        // and Float32 weight [+0x28], fetch the developer's Sint16 desirability
        // grid via GetDesirabilityMap @ 0x10015b188 (developer +0x1f8), apply
        // AddDesirability @ 0x1000c7340 over the +0x50 grid through
        // ProcessTwoGridsWithCallback @ 0x1000c6fb2, then advance the cursor via
        // the pointer-ordered map walk (@ 0x1000c6753-0x1000c6972); stay in 7.
        if (growthDeveloperCursor == growthDeveloperEnd) {
            if (workingGrid != nullptr && landValueGrid != nullptr) {
                LandValueByteGrid& dst = landValueGrid->GetGridData();
                const LandValueByteGrid& src = workingGrid->GetGridData();
                if (&dst != &src) {
                    dst = src; // vector assign @ 0x10010f188 (copy +0x50 -> +0x48)
                }
            }
            ResetLandValueIterator(intrinsicIterator); // @ 0x1000c6918-0x1000c6932
            altitudeSum80 = 0;
            altitudeSampleCount84 = 0; // one qword store @ 0x1000c693b
            idleState = 8;             // @ 0x1000c6947
            break;
        }

        void* developer = nullptr;
        Float32 weight = 0.0f;
        if (readGrowthDeveloper != nullptr) {
            readGrowthDeveloper(growthDeveloperCursor, &developer, &weight); // @ 0x1000c6717-0x1000c671e
        }
        const cSC4SimGrid<Sint16>* desirabilityGrid =
            (developer != nullptr && getGrowthDeveloperDesirabilityMap != nullptr)
                ? getGrowthDeveloperDesirabilityMap(developer) // GetDesirabilityMap @ 0x1000c6730
                : nullptr;
        if (workingGrid != nullptr && desirabilityGrid != nullptr) {
            LandValueDesirabilityContext context;
            context.weight = weight;
            context.curve = growthCurve; // [this+0xe0] @ 0x1000c6721-0x1000c6729
            // ProcessTwoDesirabilityGrids @ 0x1000c6fb2: dest = +0x50 (Uint8),
            // source = developer's Sint16 grid, callback AddDesirability.
            ProcessTwoGridsWithCallback<Uint8, Sint16>(
                workingGrid, desirabilityGrid, LandValueAddDesirability, &context);
        }
        if (advanceGrowthDeveloper != nullptr) {
            growthDeveloperCursor = advanceGrowthDeveloper(growthDeveloperCursor); // ++iterator
        }
        break;
    }
    case 8: {
        // Exact State 8 @ 0x1000c6770: accumulate the +0x48 land-value grid
        // over zoned (or all-land fallback) tracts that have land, then write
        // the truncating integer average into fieldC8, emit history, run the
        // end-of-cycle message send, and advance to state 9. The +0x80/+0x84
        // accumulators are zeroed by State 7's completion (@ 0x1000c693b) in the
        // full chain; a caller entering at state 8 must supply them zeroed.
        if (intrinsicGrid != nullptr && waterGrid != nullptr && zoneGrid != nullptr &&
            landValueGrid != nullptr) {
            while (intrinsicIterator.status != 2) {
                if (intrinsicIterator.status == 0) {
                    intrinsicIterator.status = 1;
                } else {
                    intrinsicIterator.x += intrinsicIterator.step;
                    if (intrinsicIterator.x > intrinsicIterator.maxX) {
                        intrinsicIterator.x = intrinsicIterator.minX;
                        intrinsicIterator.z += intrinsicIterator.step;
                        if (intrinsicIterator.z > intrinsicIterator.maxZ) {
                            intrinsicIterator.status = 2;
                            break;
                        }
                    }
                }
                const Sint32 x = intrinsicIterator.x;
                const Sint32 z = intrinsicIterator.z;
                // Zoned gate first (skipped in the all-land fallback), then the
                // land gate (@ 0x1000c6863-0x1000c688e).
                if (!altitudeFallback88 &&
                    !TractIsZoned(*intrinsicGrid, *zoneGrid, x, z)) {
                    continue;
                }
                if (!TractHasLand(*waterGrid, static_cast<Uint32>(x),
                                  static_cast<Uint32>(z),
                                  static_cast<Uint32>(intrinsicGrid->GetTractSize()))) {
                    continue;
                }
                altitudeSum80 += landValueGrid->GetTractValue(x, z); // @ 0x1000c68a3
                ++altitudeSampleCount84;                             // @ 0x1000c68ab
            }
        }
        // Truncating average of sum/count into fieldC8 (@ 0x1000c69f5-0x1000c6a10;
        // signed `idiv` truncates toward zero, count <= 0 yields 0).
        Sint32 average = 0;
        if (altitudeSampleCount84 > 0) {
            average = altitudeSum80 / altitudeSampleCount84;
        }
        fieldC8 = static_cast<Uint8>(average); // @ 0x1000c6a10
        if (emitHistory != nullptr) {
            emitHistory(historyContext, 0x6a663ac8u, fieldC8); // @ 0x1000c6a22-0x1000c6a2d
        }
        idleState = 9; // @ 0x1000c6a30
        if (endOfSimulationCycle != nullptr) {
            endOfSimulationCycle(cycleOwner, 0x87cc75b3u); // DoEndOfSimulationCycle @ 0x1000c738e
        }
        break;
    }
    default:
        break;
    }
}



void AuraSimulatorSaveRecord::ComputeHotspotList(
    std::span<const Sint32Triple> candidates,
    Sint32 mergeDistanceThreshold,
    Sint32 minClusterCount) noexcept {
    // Exact cSC4AuraSimulator::ComputeHotspotList @ 0x10021cfdc.
    entries.clear();
    for (const Sint32Triple& cand : candidates) {
        const Sint32 x = cand.a;
        const Sint32 z = cand.b;
        bool merged = false;
        for (Sint32Triple& cluster : entries) {
            const Sint32 dx = (x < cluster.a) ? (cluster.a - x) : (x - cluster.a);
            const Sint32 dz = (z < cluster.b) ? (cluster.b - z) : (z - cluster.b);
            if (dx + dz < mergeDistanceThreshold) {
                const Sint32 n = cluster.c;
                const Sint32 nextN = n + 1;
                cluster.a = (cluster.a * n + x) / nextN;
                cluster.b = (cluster.b * n + z) / nextN;
                cluster.c = nextN;
                merged = true;
                break;
            }
        }
        if (!merged) {
            entries.push_back(cand);
        }
    }
    entries.erase(
        std::remove_if(entries.begin(), entries.end(),
                       [minClusterCount](const Sint32Triple& c) {
                           return c.c < minClusterCount;
                       }),
        entries.end());
}

bool CivicBuildingSimulatorSaveRecord::IsBuildingExcluded(Uint32 buildingTypeId) const noexcept {
    // Exact cSC4CivicBuildingSimulator::IsBuildingExcluded @ 0x1000ea75c.
    return std::find(unlockedSet.begin(), unlockedSet.end(), buildingTypeId) != unlockedSet.end();
}

const ConditionalBuildingStatusRecord*
CivicBuildingSimulatorSaveRecord::GetConditionalBuildingStatus(
    Uint32 buildingTypeId) const noexcept {
    // Exact cSC4CivicBuildingSimulator::GetConditionalBuildingStatus @ 0x1000ea6fc.
    for (const ConditionalBuildingStatusRecord& r : statuses) {
        if (r.buildingTypeId == buildingTypeId) {
            return &r;
        }
    }
    return nullptr;
}

void CivicBuildingSimulatorSaveRecord::UpdateConditionalBuildingStatus(
    const ConditionalBuildingStatusRecord& rec) {
    // Exact cSC4CivicBuildingSimulator::UpdateConditionalBuildingStatus @ 0x1000ea728.
    for (ConditionalBuildingStatusRecord& r : statuses) {
        if (r.buildingTypeId == rec.buildingTypeId) {
            r = rec;
            return;
        }
    }
    statuses.push_back(rec);
}

} // namespace opensc4
