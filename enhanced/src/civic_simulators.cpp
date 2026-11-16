// Exact decompilations of the civic & utility simulator save records.
// See include/opensc4/civic_simulators.h for the provenance of every field.

#include "opensc4/civic_simulators.h"
#include "opensc4/tract_developer.h"

#include <algorithm>
#include <cmath>
#include <cstring>

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

} // namespace

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
    fieldStatic0 = exemplar.GetFloat(0x69501944u, fieldStatic0);
    return LoadProps(exemplar, properties);
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
    if (upperSpanB4 > 100.0f && terrainMaxAltitude > 100.0f) {
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
