// Exact decompilation of the Batch 2 .sc4 save subfiles. See
// include/opensc4/sc4_subfiles_batch2.h for the binary address of every routine.

#include "opensc4/sc4_subfiles_batch2.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace opensc4 {
namespace {

using Ref = SimSerializableRef;

bool RRef(cIGZIStream& s, Ref& r) noexcept { return ReadSimSerializableRef(s, r); }
void WRef(cIGZOStream& s, const Ref& r) { WriteSimSerializableRef(s, r); }

bool CountOk(const cIGZIStream& s, Uint32 n, std::size_t minBytes) noexcept {
    return s.Good() && (minBytes == 0 || n <= s.Remaining() / minBytes);
}

// Uint32 count followed by count x SC4WriteGZSerializable (also the layout of
// operator<<(cIGZOStream&, ilist<cISC4Occupant*> const&) @ 0x1000cfbd2).
bool RRefVec(cIGZIStream& s, std::vector<Ref>& v) noexcept {
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 4)) {
        return false;
    }
    v.assign(n, Ref{});
    for (Ref& r : v) {
        if (!RRef(s, r)) {
            return false;
        }
    }
    return s.Good();
}

void WRefVec(cIGZOStream& s, const std::vector<Ref>& v) {
    s << static_cast<Uint32>(v.size());
    for (const Ref& r : v) {
        WRef(s, r);
    }
}

bool RRect(cIGZIStream& s, SC4Rect<Sint32>& r) noexcept {
    s >> r.x0 >> r.y0 >> r.x1 >> r.y1;
    return s.Good();
}

// operator<< <int>(cIGZOStream&, SC4Rect<int> const&) @ 0x10030d31a.
void WRect(cIGZOStream& s, const SC4Rect<Sint32>& r) { s << r.x0 << r.y0 << r.x1 << r.y1; }

// ScanningIterator operator>> / operator<< (29 bytes).
bool RScan(cIGZIStream& s, ScanningIteratorState& it) noexcept {
    s >> it.field0 >> it.field4 >> it.field8 >> it.fieldC >> it.field10 >> it.field14 >>
        it.field18 >> it.field1C;
    return s.Good();
}

void WScan(cIGZOStream& s, const ScanningIteratorState& it) {
    s << it.field0 << it.field4 << it.field8 << it.fieldC << it.field10 << it.field14
      << it.field18 << it.field1C;
}

} // namespace

// ===========================================================================
// cSC4OrdinanceSimple
// ===========================================================================
bool OrdinanceSimpleSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 4) {
        return false;
    }
    s >> field4C >> clsid5C;
    if (!s.ReadGZString(name60) || !s.ReadGZString(description88)) {
        return false;
    }
    s >> fieldB0 >> fieldB4 >> fieldB8 >> fieldC0 >> fieldC8 >> fieldD0 >> fieldD4 >> fieldD9 >>
        fieldDA >> fieldDB >> fieldE0;
    if (version >= 2) {
        s >> fieldD8;
    }
    return s.Good() && exemplarHolder.Read(s);
}

bool OrdinanceSimpleSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    s << ver << field4C << clsid5C;
    s.WriteGZString(name60);
    s.WriteGZString(description88);
    s << fieldB0 << fieldB4 << fieldB8 << fieldC0 << fieldC8 << fieldD0 << fieldD4 << fieldD9
      << fieldDA << fieldDB << fieldE0;
    if (ver >= 2) {
        s << fieldD8;
    }
    return exemplarHolder.Write(s);
}

void OrdinanceSimpleSaveRecord::SetAvailable(bool available) noexcept {
    // Exact cSC4OrdinanceSimple::SetAvailable @ 0x100798566.
    fieldD9 = available ? 1u : 0u;
    fieldE0 = 0;
}

void OrdinanceSimpleSaveRecord::SetOn(bool on) noexcept {
    // Exact cSC4OrdinanceSimple::SetOn @ 0x100798590:
    //   if ((fieldDA != 0) != on) {
    //     if (on) fieldE0 = Sint64(float(fieldE0) + fabsf(float(fieldB8)));
    //     else    fieldE0 = Sint64(float(fieldE0) - fabsf(float(fieldC0)));
    //     fieldDA = on;
    //   }
    const bool current = (fieldDA != 0u);
    if (current == on) {
        return;
    }
    const Float32 curF = static_cast<Float32>(fieldE0);
    if (on) {
        const Float32 enact = std::fabs(static_cast<Float32>(fieldB8));
        fieldE0 = static_cast<Sint64>(curF + enact);
    } else {
        const Float32 retract = std::fabs(static_cast<Float32>(fieldC0));
        fieldE0 = static_cast<Sint64>(curF - retract);
    }
    fieldDA = on ? 1u : 0u;
}

bool OrdinanceSimpleSaveRecord::CheckConditions(Uint32 currentYear) const noexcept {
    // Exact cSC4OrdinanceSimple::CheckConditions @ 0x1007985fc.
    return fieldDB != 0u && currentYear >= fieldB0;
}

Sint64 OrdinanceSimpleSaveRecord::GetCurrentMonthlyIncome(Sint64 scalingBaseValue) const noexcept {
    // Exact cSC4OrdinanceSimple::GetCurrentMonthlyIncome @ 0x10079865a
    // (and derived FreeClinics @ 0x100795f12, ProReadingCampaign @ 0x1007979f8,
    // JuniorSportsLeague @ 0x100796308, TouristPromotion @ 0x10079ac02,
    // TrashPresort @ 0x10079afe4, LandfillGasRecovery @ 0x1007968cc):
    //   Sint64(float(fieldC8) + float(scalingBaseValue) * fieldD0).
    const Float32 base = static_cast<Float32>(fieldC8);
    const Float32 scaled = static_cast<Float32>(scalingBaseValue) * fieldD0;
    return static_cast<Sint64>(base + scaled);
}

Sint64 OrdinanceSimpleSaveRecord::Simulate(Sint64 scalingBaseValue) noexcept {
    // Exact cSC4OrdinanceSimple::Simulate @ 0x1007986b0.
    fieldE0 = GetCurrentMonthlyIncome(scalingBaseValue);
    return fieldE0;
}

// ===========================================================================
// Version-only / empty / trivial records
// ===========================================================================
bool VersionOnlySaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    return s.Good() && version <= 1;
}
bool VersionOnlySaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1};
    return true;
}

bool CityDetailManagerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version >> field10;
    return s.Good();
}
bool CityDetailManagerSaveRecord::Write(cIGZOStream& s) const {
    s << Uint32{0} << field10;
    return true;
}

bool TerrainView3DSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version >> displayGrid;
    return s.Good() && version <= 1;
}
bool TerrainView3DSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1} << displayGrid;
    return true;
}

bool TerrainSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    return s.Good() && version <= 2 && RRef(s, terrainView);
}
bool TerrainSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    WRef(s, terrainView);
    return true;
}

// ===========================================================================
// cSC4PortDeveloper / cSC4AirportDeveloper / cSC4SeaportDeveloper
// ===========================================================================
bool PortDeveloperSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 1) {
        return false;
    }
    Uint16 count = 0;
    s >> field20 >> field58 >> count;
    if (!CountOk(s, count, 10)) {
        return false;
    }
    entries.assign(count, PortDeveloperEntry{});
    for (auto& e : entries) {
        s >> e.field0 >> e.field4 >> e.field6 >> e.field8;
    }
    return s.Good();
}
bool PortDeveloperSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1} << field20 << field58 << static_cast<Uint16>(entries.size());
    for (const auto& e : entries) {
        s << e.field0 << e.field4 << e.field6 << e.field8;
    }
    return true;
}

bool PortDeveloperSubclassSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    return s.Good() && version == 2 && port.Read(s);
}
bool PortDeveloperSubclassSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    return port.Write(s);
}

// ===========================================================================
// cGarbageHeap / cSC4LandfillZone / cSC4LandfillDeveloper
// ===========================================================================
bool GarbageHeapSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 2) {
        return false;
    }
    s >> field8 >> fieldC >> field10 >> field11 >> field1C >> field18;
    return s.Good();
}
bool GarbageHeapSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2} << field8 << fieldC << field10 << field11 << field1C << field18;
    return true;
}

bool LandfillZoneSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 4 || !RRect(s, rect)) {
        return false;
    }
    const Sint64 w = std::max<Sint64>(0, Sint64{rect.x1} - rect.x0);
    const Sint64 h = std::max<Sint64>(0, Sint64{rect.y1} - rect.y0);
    if (static_cast<std::size_t>(w * h) > s.Remaining()) {
        return false;
    }
    cells.resize(static_cast<std::size_t>(w * h));
    if (!cells.empty() && !s.ReadBytes(cells.data(), cells.size())) {
        return false;
    }
    s >> field20 >> field1C;
    // +0x18 = +0x1c * +0x20 heaps (one per heap-grid cell).
    const Sint64 heapCount = Sint64{field1C} * field20;
    if (!s.Good() || heapCount < 0 || static_cast<std::size_t>(heapCount) > s.Remaining() / 22u) {
        return false;
    }
    heaps.assign(static_cast<std::size_t>(heapCount), GarbageHeapSaveRecord{});
    for (auto& heap : heaps) {
        if (!heap.Read(s)) {
            return false;
        }
    }
    if (version == 3) {
        s >> field41;
    }
    if (version >= 4) {
        s >> field41;
        if (!RRef(s, ref60)) {
            return false;
        }
    }
    return s.Good();
}
bool LandfillZoneSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    s << ver;
    WRect(s, rect);
    s.WriteBytes(cells.data(), cells.size());
    s << field20 << field1C;
    for (const auto& heap : heaps) {
        heap.Write(s);
    }
    if (ver >= 3) {
        s << field41;
    }
    if (ver >= 4) {
        WRef(s, ref60);
    }
    return true;
}

bool LandfillDeveloperSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 3) {
        return false;
    }
    Sint32 count = 0;
    s >> count;
    if (!s.Good() || count < 0 || !CountOk(s, static_cast<Uint32>(count), 18)) {
        return false;
    }
    zones.assign(static_cast<std::size_t>(count), LandfillZoneSaveRecord{});
    for (auto& zone : zones) {
        if (!zone.Read(s)) {
            return false;
        }
    }
    return s.Good();
}
bool LandfillDeveloperSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    s << ver << static_cast<Sint32>(zones.size());
    for (const auto& zone : zones) {
        zone.WriteWithVersion(s, zone.version);
    }
    return true;
}
bool LandfillDeveloperSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{3} << static_cast<Sint32>(zones.size());
    for (const auto& zone : zones) {
        zone.Write(s);
    }
    return true;
}

// ===========================================================================
// Managers with ref lists
// ===========================================================================
bool ParkManagerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    return s.Good() && version <= 2 && RRefVec(s, list28) && RRefVec(s, list40);
}
bool ParkManagerSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    WRefVec(s, list28);
    WRefVec(s, list40);
    return true;
}

bool PropManagerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    return s.Good() && version <= 1 && RRef(s, refA) && RRef(s, refB);
}
bool PropManagerSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1};
    WRef(s, refA);
    WRef(s, refB);
    return true;
}

bool BusinessUnitSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    return s.Good() && version <= 1 && RRefVec(s, refs);
}
bool BusinessUnitSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1};
    WRefVec(s, refs);
    return true;
}

bool TutorialSystemSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 2) {
        return false;
    }
    s >> field2F >> field30 >> field34;
    return s.Good();
}
bool TutorialSystemSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2} << field2F << field30 << field34;
    return true;
}

bool CitySituationManagerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 2) {
        return false;
    }
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 6)) {
        return false;
    }
    situations.assign(n, {});
    for (auto& [id, value] : situations) {
        s >> id >> value;
    }
    s >> fieldA18;
    return s.Good();
}
bool CitySituationManagerSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2} << static_cast<Uint32>(situations.size());
    for (const auto& [id, value] : situations) {
        s << id << value;
    }
    s << fieldA18;
    return true;
}

// ===========================================================================
// cSC424HourClock
// ===========================================================================
bool TwentyFourHourClockSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 2) {
        return false;
    }
    s >> field40 >> field48 >> field50 >> field54;
    if (version <= 1) {
        s >> legacyTimerUnits >> legacyTimerElapsed >> legacyTimerRunning;
    }
    s >> field78 >> field7C;
    return s.Good();
}
bool TwentyFourHourClockSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    s << ver << field40 << field48 << field50 << field54;
    if (ver <= 1) {
        s << legacyTimerUnits << legacyTimerElapsed << legacyTimerRunning;
    }
    s << field78 << field7C;
    return true;
}

// ===========================================================================
// cSC4CameraControl / cSC4NeighborConnection / cSC4NeighborDeal
// ===========================================================================
bool CameraControlSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 1 || !field4C.Read(s)) {
        return false;
    }
    for (Sint32& v : field108) {
        s >> v;
    }
    return s.Good();
}
bool CameraControlSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1};
    field4C.Write(s);
    for (Sint32 v : field108) {
        s << v;
    }
    return true;
}

bool NeighborConnectionSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 1) {
        return false;
    }
    s >> field44 >> field48 >> field4C >> field50 >> field54 >> field58;
    return s.Good() && ReadSCPropertyHolder(s, properties);
}
bool NeighborConnectionSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1} << field44 << field48 << field4C << field50 << field54 << field58;
    WriteSCPropertyHolder(s, properties);
    return true;
}

bool NeighborDealSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 5) {
        return false;
    }
    s >> field3C >> field40 >> field44 >> field50 >> field64 >> field68 >> fieldB0 >> field48 >>
        field4C >> field54 >> field5C >> field64Again >> field98 >> field78 >> field88;
    if (!s.Good() || !ReadSCPropertyHolder(s, properties)) {
        return false;
    }
    if (version >= 5) {
        s >> fieldA8;
    }
    return s.Good();
}
bool NeighborDealSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    s << ver << field3C << field40 << field44 << field50 << field64 << field68 << fieldB0
      << field48 << field4C << field54 << field5C << field64Again << field98 << field78
      << field88;
    WriteSCPropertyHolder(s, properties);
    if (ver >= 5) {
        s << fieldA8;
    }
    return true;
}

// ===========================================================================
// cSC4ZoneDeveloper
// ===========================================================================
bool ZoneDeveloperSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 1) {
        return false;
    }
    Uint32 outer = 0;
    Uint32 inner = 0;
    s >> outer >> inner;
    if (!s.Good() ||
        (inner != 0 && !CountOk(s, outer, std::size_t{4} * inner)) ||
        (inner == 0 && outer > (1u << 20))) {
        return false;
    }
    rows.assign(outer, std::vector<Ref>(inner));
    for (auto& row : rows) {
        for (Ref& r : row) {
            if (!RRef(s, r)) {
                return false;
            }
        }
    }
    return s.Good();
}
bool ZoneDeveloperSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1} << static_cast<Uint32>(rows.size())
      << static_cast<Uint32>(rows.empty() ? 0u : rows.front().size());
    for (const auto& row : rows) {
        for (const Ref& r : row) {
            WRef(s, r);
        }
    }
    return true;
}

// ===========================================================================
// cSC43DRender
// ===========================================================================
bool ThreeDRenderSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 2) {
        return false;
    }
    s >> fieldB4 >> fieldB8;
    for (Sint32& v : fieldBC) {
        s >> v;
    }
    if (!RRef(s, refE0)) {
        return false;
    }
    s >> fieldE8 >> fieldEC >> field138;
    return s.Good();
}
bool ThreeDRenderSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2} << fieldB4 << fieldB8;
    for (Sint32 v : fieldBC) {
        s << v;
    }
    WRef(s, refE0);
    s << fieldE8 << fieldEC << field138;
    return true;
}

// ===========================================================================
// cSC4DispatchManager
// ===========================================================================
bool DispatchManagerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 2) {
        return false;
    }
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 37)) {
        return false;
    }
    dispatches.assign(n, DispatchInfoRecord{});
    for (auto& d : dispatches) {
        s >> d.field0 >> d.field4 >> d.field8 >> d.field10 >> d.field14 >> d.field18 >> d.field1C;
        if (!RRef(s, d.ref20) || !RRef(s, d.ref28) || !RRef(s, d.ref30)) {
            return false;
        }
    }
    s >> field50;
    if (!RRef(s, ref60) || !RRefVec(s, refs)) {
        return false;
    }
    s >> field80;
    return s.Good();
}
bool DispatchManagerSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2} << static_cast<Uint32>(dispatches.size());
    for (const auto& d : dispatches) {
        s << d.field0 << d.field4 << d.field8 << d.field10 << d.field14 << d.field18 << d.field1C;
        WRef(s, d.ref20);
        WRef(s, d.ref28);
        WRef(s, d.ref30);
    }
    s << field50;
    WRef(s, ref60);
    WRefVec(s, refs);
    s << field80;
    return true;
}

// ===========================================================================
// cSC4LineItem / cSC4DepartmentBudget
// ===========================================================================
bool LineItemSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 4 || !s.ReadGZString(name18)) {
        return false;
    }
    s >> field50 >> field58 >> field60 >> field68 >> field74 >> field78 >> field7C >> field88;
    if (!RRef(s, ref80)) {
        return false;
    }
    s >> field40 >> field44;
    return s.Good();
}
bool LineItemSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{4};
    s.WriteGZString(name18);
    s << field50 << field58 << field60 << field68 << field74 << field78 << field7C << field88;
    WRef(s, ref80);
    s << field40 << field44;
    return true;
}

bool DepartmentBudgetSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 10) {
        return false;
    }
    s >> field10 >> field18;
    if (!s.ReadGZString(name20)) {
        return false;
    }
    s >> field50 >> field54 >> field58 >> field60 >> field68;
    if (!RRefVec(s, lineItems)) {
        return false;
    }
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 8)) {
        return false;
    }
    holders.assign(n, {});
    for (auto& [ref, value] : holders) {
        if (!RRef(s, ref)) {
            return false;
        }
        s >> value;
    }
    s >> field48 >> field4C;
    return s.Good();
}
bool DepartmentBudgetSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    s << ver << field10 << field18;
    s.WriteGZString(name20);
    s << field50 << field54 << field58 << field60 << field68;
    WRefVec(s, lineItems);
    s << static_cast<Uint32>(holders.size());
    for (const auto& [ref, value] : holders) {
        WRef(s, ref);
        s << value;
    }
    s << field48 << field4C;
    return true;
}

// ===========================================================================
// cSC4HistoryWarehouse
// ===========================================================================
bool HistoryWarehouseSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 2) {
        return false;
    }
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 12)) {
        return false;
    }
    histories.assign(n, HistoryWarehouseEntry{});
    for (auto& h : histories) {
        Uint32 count = 0;
        s >> h.key >> h.field40 >> count;
        if (!CountOk(s, count, 8)) {
            return false;
        }
        h.nodes.assign(count, {});
        for (auto& [date, value] : h.nodes) {
            s >> date >> value;
        }
    }
    return s.Good();
}
bool HistoryWarehouseSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    s << ver << static_cast<Uint32>(histories.size());
    for (const auto& h : histories) {
        s << h.key << h.field40 << static_cast<Uint32>(h.nodes.size());
        for (const auto& [date, value] : h.nodes) {
            s << date << value;
        }
    }
    return true;
}

// ===========================================================================
// cSC4Demand
// ===========================================================================
bool DemandSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 2) {
        return false;
    }
    s >> field18 >> field1C >> field20 >> field24 >> field28 >> field2C >> field30 >> field38 >>
        field34 >> field3C >> field40 >> field44 >> field48;
    if (version == 1) {
        s >> legacyField4C;
        field4C = static_cast<Float32>(legacyField4C);
    } else {
        s >> field4C;
    }
    return s.Good();
}
bool DemandSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    s << ver << field18 << field1C << field20 << field24 << field28 << field2C << field30
      << field38 << field34 << field3C << field40 << field44 << field48;
    if (ver == 1) {
        s << legacyField4C;
    } else {
        s << field4C;
    }
    return true;
}

// ===========================================================================
// cSC4ModelMaker
// ===========================================================================
bool ModelMakerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version > 2 || !RRef(s, ref20)) {
        return false;
    }
    for (Sint32& v : field38) {
        s >> v;
    }
    if (!RRect(s, rect48)) {
        return false;
    }
    s >> field58;
    if (!RRef(s, ref70) || !RRef(s, ref98) || !RRef(s, refA0) || !RRef(s, refA8)) {
        return false;
    }
    for (Float32& f : field15C) {
        s >> f;
    }
    for (Sint32& v : field16C) {
        s >> v;
    }
    return s.Good();
}
bool ModelMakerSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{2};
    WRef(s, ref20);
    for (Sint32 v : field38) {
        s << v;
    }
    WRect(s, rect48);
    s << field58;
    WRef(s, ref70);
    WRef(s, ref98);
    WRef(s, refA0);
    WRef(s, refA8);
    for (Float32 f : field15C) {
        s << f;
    }
    for (Sint32 v : field16C) {
        s << v;
    }
    return true;
}

// ===========================================================================
// cSC4PropDeveloper
// ===========================================================================
bool PropDeveloperSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version < 4 || version > 5) {
        return false;
    }
    s >> field20;
    if (!RScan(s, scanningIterator)) {
        return false;
    }
    s >> fieldA18;
    if (!RRefVec(s, requestersA20) || !RRef(s, refA38) || !RRef(s, refA40) ||
        !RRef(s, refA50)) {
        return false;
    }
    s >> fieldA70 >> fieldA74 >> fieldA78 >> fieldA7C >> fieldA80;
    if (!RRefVec(s, propsA58)) {
        return false;
    }
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 8)) {
        return false;
    }
    hashA90.assign(n, PropDeveloperHashEntry{});
    for (auto& e : hashA90) {
        s >> e.key;
        if (!RRefVec(s, e.props)) {
            return false;
        }
    }
    if (!RRefVec(s, listAD8) || !RRefVec(s, listAC0) || !RRefVec(s, listAF0)) {
        return false;
    }
    if (version >= 5) {
        s >> fieldB00;
        return RRefVec(s, listB10);
    }
    return s.Good();
}
bool PropDeveloperSaveRecord::WriteWithVersion(cIGZOStream& s, Uint16 ver) const {
    s << ver << field20;
    WScan(s, scanningIterator);
    s << fieldA18;
    WRefVec(s, requestersA20);
    WRef(s, refA38);
    WRef(s, refA40);
    WRef(s, refA50);
    s << fieldA70 << fieldA74 << fieldA78 << fieldA7C << fieldA80;
    WRefVec(s, propsA58);
    s << static_cast<Uint32>(hashA90.size());
    for (const auto& e : hashA90) {
        s << e.key;
        WRefVec(s, e.props);
    }
    WRefVec(s, listAD8);
    WRefVec(s, listAC0);
    WRefVec(s, listAF0);
    if (ver >= 5) {
        s << fieldB00;
        WRefVec(s, listB10);
    }
    return true;
}

// ===========================================================================
// cSC4ZoneManager
// ===========================================================================
namespace {
bool RZoneMap(cIGZIStream& s, std::vector<ZoneManagerMapEntry>& m) noexcept {
    Uint32 n = 0;
    s >> n;
    if (!CountOk(s, n, 6)) {
        return false;
    }
    m.assign(n, ZoneManagerMapEntry{});
    for (auto& e : m) {
        s >> e.field1C >> e.field1D >> e.field20;
    }
    return s.Good();
}
void WZoneMap(cIGZOStream& s, const std::vector<ZoneManagerMapEntry>& m) {
    s << static_cast<Uint32>(m.size());
    for (const auto& e : m) {
        s << e.field1C << e.field1D << e.field20;
    }
}
} // namespace

bool ZoneManagerSaveRecord::Read(cIGZIStream& s) noexcept {
    s >> version;
    if (!s.Good() || version != 1 || !RRef(s, ref70)) {
        return false;
    }
    for (auto* arr : {&field78, &fieldB8, &fieldF8}) {
        for (Sint32& v : *arr) {
            s >> v;
        }
    }
    s >> field138 >> field13C >> field140 >> field144;
    for (auto* arr : {&field148, &field188}) {
        for (Sint32& v : *arr) {
            s >> v;
        }
    }
    for (auto* arr : {&field208, &field288}) {
        for (Sint64& v : *arr) {
            s >> v;
        }
    }
    for (auto* arr : {&field308, &field348}) {
        for (Uint32& v : *arr) {
            s >> v;
        }
    }
    s >> field388;
    if (!RZoneMap(s, map390) || !RZoneMap(s, map3A8)) {
        return false;
    }
    for (Ref& r : refs30) {
        if (!RRef(s, r)) {
            return false;
        }
    }
    return s.Good();
}
bool ZoneManagerSaveRecord::Write(cIGZOStream& s) const {
    s << Uint16{1};
    WRef(s, ref70);
    for (const auto* arr : {&field78, &fieldB8, &fieldF8}) {
        for (Sint32 v : *arr) {
            s << v;
        }
    }
    s << field138 << field13C << field140 << field144;
    for (const auto* arr : {&field148, &field188}) {
        for (Sint32 v : *arr) {
            s << v;
        }
    }
    for (const auto* arr : {&field208, &field288}) {
        for (Sint64 v : *arr) {
            s << v;
        }
    }
    for (const auto* arr : {&field308, &field348}) {
        for (Uint32 v : *arr) {
            s << v;
        }
    }
    s << field388;
    WZoneMap(s, map390);
    WZoneMap(s, map3A8);
    for (const Ref& r : refs30) {
        WRef(s, r);
    }
    return true;
}

// ===========================================================================
// Registry
// ===========================================================================
bool RoundTripSC4SubfileBatch2(Uint32 typeId, std::span<const Uint8> bytes,
                               SC4SubfileRoundTripStats& st) {
    using sc4_detail::RoundTripMulti;
    if (std::find(kOrdinanceSimpleCLSIDs.begin(), kOrdinanceSimpleCLSIDs.end(), typeId) !=
        kOrdinanceSimpleCLSIDs.end()) {
        RoundTripMulti<OrdinanceSimpleSaveRecord>(bytes, 4, st);
        return true;
    }
    switch (typeId) {
    case kCLSID_cSC4Demolition:
    case kCLSID_cSC4AutomataScriptSystem:
        RoundTripMulti<VersionOnlySaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSC4WealthRequester:
        RoundTripMulti<EmptySaveRecord>(bytes, sc4_detail::kCompareAllVersions, st);
        return true;
    case kCLSID_cSC4CityDetailManager:
        RoundTripMulti<CityDetailManagerSaveRecord>(bytes, sc4_detail::kCompareAllVersions, st);
        return true;
    case kCLSID_cSTETerrainView3D:
        RoundTripMulti<TerrainView3DSaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSTETerrain:
        RoundTripMulti<TerrainSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4PortDeveloper:
        RoundTripMulti<PortDeveloperSaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSC4AirportDeveloper:
    case kCLSID_cSC4SeaportDeveloper:
        RoundTripMulti<PortDeveloperSubclassSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4LandfillDeveloper:
        RoundTripMulti<LandfillDeveloperSaveRecord>(bytes, 3, st);
        return true;
    case kCLSID_cSC4ParkManager:
        RoundTripMulti<ParkManagerSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4PropManager:
        RoundTripMulti<PropManagerSaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSC4BusinessUnit:
        RoundTripMulti<BusinessUnitSaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSC4TutorialSystem:
        RoundTripMulti<TutorialSystemSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4CitySituationManager:
        RoundTripMulti<CitySituationManagerSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC424HourClock:
        RoundTripMulti<TwentyFourHourClockSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4CameraControl:
        RoundTripMulti<CameraControlSaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSC4NeighborConnection:
        RoundTripMulti<NeighborConnectionSaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSC4NeighborDeal:
        RoundTripMulti<NeighborDealSaveRecord>(bytes, 5, st);
        return true;
    case kCLSID_cSC4ZoneDeveloper:
        RoundTripMulti<ZoneDeveloperSaveRecord>(bytes, 1, st);
        return true;
    case kCLSID_cSC4Occupant:
        RoundTripMulti<OccupantStandaloneSaveRecord>(bytes, 4, st);
        return true;
    case kCLSID_cSC43DRenderSave:
        RoundTripMulti<ThreeDRenderSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4DispatchManager:
        RoundTripMulti<DispatchManagerSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4LineItem:
        RoundTripMulti<LineItemSaveRecord>(bytes, 4, st);
        return true;
    case kCLSID_cSC4DepartmentBudget:
        RoundTripMulti<DepartmentBudgetSaveRecord>(bytes, 10, st);
        return true;
    case kCLSID_cSC4HistoryWarehouse:
        RoundTripMulti<HistoryWarehouseSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4DemandObject:
        RoundTripMulti<DemandSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4ModelMaker:
        RoundTripMulti<ModelMakerSaveRecord>(bytes, 2, st);
        return true;
    case kCLSID_cSC4PropDeveloper:
        RoundTripMulti<PropDeveloperSaveRecord>(bytes, 5, st);
        return true;
    case kCLSID_cSC4ZoneManager:
        RoundTripMulti<ZoneManagerSaveRecord>(bytes, 1, st);
        return true;
    default:
        return false;
    }
}

} // namespace opensc4
