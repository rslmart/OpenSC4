// clean-room fallback (not authentic): diagnostic only; every bound is a tool guard.
// Include at namespace scope AFTER logf_, g_slide and get_app declarations.
#include <mach/mach.h>
#include <mach/mach_vm.h>
#include <pthread.h>
#include <cstdint>
#include <cstring>

namespace loaded_capacity_capture_detail {
using U = unsigned long long;
static bool read(uint64_t p, void* out, size_t n) {
    if (p < 0x1000 || p > UINT64_MAX - n) return false;
    mach_vm_size_t got = 0;
    return mach_vm_read_overwrite(mach_task_self(), p, n,
        reinterpret_cast<mach_vm_address_t>(out), &got) == KERN_SUCCESS && got == n;
}
template<class T> static bool read(uint64_t p, T& out) { return read(p, &out, sizeof out); }
static bool span(uint64_t p, size_t n) {
    unsigned char scratch[256];
    while (n) {
        const size_t k = n < sizeof scratch ? n : sizeof scratch;
        if (!read(p, scratch, k)) return false;
        p += k; n -= k;
    }
    return true;
}
static bool object(uint64_t p, uint64_t vt, size_t size) {
    uint64_t actual = 0;
    return read(p, actual) && actual == vt + g_slide && span(p, size);
}
static bool property_object(uint64_t p) {
    // GetPropertyID @ 0x10004c640 reads the dword at +0x20.
    return object(p, 0x100a69a40, 0x24);
}
static bool slot(uint64_t p, unsigned offset, uint64_t expected) {
    uint64_t vt = 0, fn = 0; unsigned char first = 0;
    return read(p, vt) && read(vt + offset, fn) && fn == expected + g_slide && read(fn, first);
}
template<class F> static F original(uint64_t a) { return reinterpret_cast<F>(a + g_slide); }
static const void* ptr(uint64_t p) { return reinterpret_cast<const void*>(p); }
struct Capture {
    unsigned properties = 0, capacities = 0, curve_words = 0;
    const char* failure = nullptr;
    bool need(bool ok, const char* why) { if (!ok) failure = why; return ok; }
    bool run() {
#define LC_REQUIRE(test, why) do { if (!need((test), why)) return false; } while (0)
        LC_REQUIRE(pthread_main_np() != 0, "not_main_thread");
        // get_app is the established oracle helper; validate its result before fields.
        const uint64_t app = get_app();
        uint32_t state = 0; uint64_t city = 0, pollution = 0;
        LC_REQUIRE(object(app, 0x100a67010, 0xa8), "app_pointer_or_vtable");
        LC_REQUIRE(read(app + 0x78, state) && state == 2, "app_not_city_state_2");
        LC_REQUIRE(read(app + 0xa0, city) && object(city, 0x100a692e0, 0x2e0), "city_pointer_or_vtable");
        // GetPollutionSimulator @ 0x100048842: concrete city +0x2d8.
        LC_REQUIRE(read(city + 0x2d8, pollution) && object(pollution, 0x100a6ce20, 0x34c), "pollution_pointer_or_vtable");
        logf_("LOADED_CAPACITY STATE app=%llx state=%u city=%llx pollution=%llx simdate=unavailable pause=unavailable", U(app), state, U(city), U(pollution));
        // F0 list traversal from 0x100099f00; never call that mutating method.
        const uint64_t sentinel = pollution + 0xf0;
        uint64_t node = 0, tail = 0, prev = 0, next = 0, occupant = 0;
        LC_REQUIRE(read(sentinel + 8, node) && node != sentinel && (node & 7) == 0 && span(node, 24), "F0_empty_or_bad_node");
        LC_REQUIRE(read(sentinel, tail) && read(node, prev) && read(node + 8, next) &&
            tail == node && prev == sentinel && next == sentinel, "F0_not_exactly_one_sane_node");
        LC_REQUIRE(read(node + 0x10, occupant), "F0_occupant_read");
        // Building primary vptr 0x100a7b900; occupant interface +8 uses +0x108 vtable.
        // Fresh cSC4BuildingOccupant vtable, position thunk @ 0x10019b46c.
        LC_REQUIRE(occupant >= 8 && object(occupant - 8, 0x100a7b900, 0x84) &&
            object(occupant, 0x100a7ba08, 0x7c), "occupant_not_building_interface");
        LC_REQUIRE(slot(occupant, 0x30, 0x10019bb80) && slot(occupant, 0x40, 0x10019b46c) &&
            slot(occupant, 0x50, 0x10019b558), "building_slots");
        const uint64_t holder = original<uint64_t(*)(const void*)>(0x10019bb80)(ptr(occupant));
        LC_REQUIRE(holder == occupant + 0x10 && object(holder, 0x100a7bb90, 0x38) &&
            slot(holder, 0x28, 0x10004ae40), "holder_alignment_or_slot");
        uint64_t exemplar = 0;
        LC_REQUIRE(read(holder + 0x30, exemplar) && object(exemplar, 0x100a95bc0, 0x24) &&
            slot(exemplar, 0x58, 0x100305dc6), "default_exemplar_or_GetKey_slot");
        // GetKey @ 0x100305dc6 writes three uint32s; 0x1003260bc and
        // GetType/GetGroup/GetInstance @ 0x1003260f0/fa/104 establish T,G,I order.
        uint32_t key[3] = {};
        LC_REQUIRE(original<bool(*)(const void*, void*)>(0x100305dc6)(ptr(exemplar), key), "GetKey_false");
        logf_("LOADED_CAPACITY FIXTURE node=%llx occupant=%llx holder=%llx exemplar=%llx tgi=%08x:%08x:%08x pointer_identity=runtime_only", U(node), U(occupant), U(holder), U(exemplar), key[0], key[1], key[2]);
        LC_REQUIRE(key[0] == 0x6534284a && key[1] == 0xc8dbccba && key[2] == 0x1f470000, "fixture_TGI_mismatch");
        float position[3] = {}, lo[3] = {}, hi[3] = {};
        original<void(*)(const void*, float*)>(0x10019b46c)(ptr(occupant), position);
        original<void(*)(const void*, float*, float*)>(0x10019b558)(ptr(occupant), lo, hi);
        uint32_t posbits[3], bounds[6];
        std::memcpy(posbits, position, sizeof posbits);
        std::memcpy(bounds, lo, sizeof lo); std::memcpy(bounds + 3, hi, sizeof hi);
        logf_("LOADED_CAPACITY GEOMETRY position=%08x,%08x,%08x min=%08x,%08x,%08x max=%08x,%08x,%08x", posbits[0], posbits[1], posbits[2], bounds[0], bounds[1], bounds[2], bounds[3], bounds[4], bounds[5]);
        // Fixture raw words from fixture_grid_output.txt; bounds getter @ 0x10019b52c.
        const uint32_t expected_bounds[] = {0x44100333,0x439706ab,0x44e2019a,0x441ffccd,0x43bf86ab,0x44e9fe66};
        LC_REQUIRE(std::memcmp(bounds, expected_bounds, sizeof bounds) == 0, "fixture_raw_bounds_mismatch");
        logf_("LOADED_CAPACITY IDENTITY status=matched basis=F0_single_node_TGI_raw_bounds saved_runtime_pointer_identity=not_claimed");

        const uint32_t ids[] = {0x08fd1613,0x48ebd0b1,0x27812852,0xc910b4cf,0x8a1e07ee,0x00000001};
        for (uint32_t id : ids) {
            // Effective original lookup @ 0x10004ae40; no parser or setters.
            const uint64_t property = original<uint64_t(*)(const void*, uint32_t)>(0x10004ae40)(ptr(holder), id);
            if (!property) {
                logf_("LOADED_CAPACITY PROPERTY id=%08x lookup=missing", id);
                LC_REQUIRE(id == 1, "required_effective_property_missing");
                ++properties; continue;
            }
            LC_REQUIRE(property_object(property) && slot(property, 0x18, 0x10004c63c) &&
                slot(property, 0x28, 0x10004c650), "property_pointer_or_slots");
            const uint32_t embedded = original<uint32_t(*)(const void*)>(0x10004c63c)(ptr(property));
            const uint64_t variant = original<uint64_t(*)(const void*)>(0x10004c650)(ptr(property));
            // cSCProperty embeds cIGZVariant at +8; secondary vtable at primary +0x70.
            LC_REQUIRE(variant == property + 8 && object(variant, 0x100a69ab0, 0x18) &&
                slot(variant, 0x18, 0x100051c00) && slot(variant, 0x20, 0x10004c682), "variant_alignment_or_slots");
            const uint32_t type = original<uint32_t(*)(const void*)>(0x100051c00)(ptr(variant));
            const uint32_t count = original<uint32_t(*)(const void*)>(0x10004c682)(ptr(variant));
            logf_("LOADED_CAPACITY PROPERTY id=%08x lookup=present embedded=%08x property=%llx variant=%llx type=%04x count=%u", id, embedded, U(property), U(variant), type, count);
            LC_REQUIRE(id != 1 && embedded == id, "missing_probe_present_or_embedded_ID_mismatch");
            if (type == 6 || type == 7 || type == 0xa) {
                uint32_t raw = 0;
                LC_REQUIRE(id != 0x8a1e07ee && read(variant + 8, raw), "scalar_payload_or_curve_type");
                logf_("LOADED_CAPACITY SCALAR id=%08x raw=%08x", id, raw);
            } else if (type == 0x800a) {
                LC_REQUIRE(id == 0x8a1e07ee && count <= 256 && slot(variant, 0x3a8, 0x10004c804), "array_type_count_or_slot_guard");
                const uint64_t data = original<uint64_t(*)(const void*)>(0x10004c804)(ptr(variant));
                uint32_t raw[256] = {};
                LC_REQUIRE(!count || read(data, raw, size_t(count) * 4), "array_payload_unreadable");
                logf_("LOADED_CAPACITY ARRAY id=%08x data=%llx count=%u tool_max=256", id, U(data), count);
                for (uint32_t i = 0; i < count; ++i) {
                    logf_("LOADED_CAPACITY CURVE index=%u raw=%08x", i, raw[i]); ++curve_words;
                }
                LC_REQUIRE(count == 16, "fixture_curve_count_not_16");
            } else { LC_REQUIRE(false, "unsupported_runtime_variant_type"); }
            ++properties;
        }
        // Original IsOccupantPowered @ 0x10021485c selects SL::spLotManager.
        // Manager is cSC4ZoneDeveloper's +8 interface, NOT its primary vptr.
        uint64_t manager = 0, power = 0;
        LC_REQUIRE(read(0x100b21340 + g_slide, manager) && manager >= 8 &&
            object(manager - 8, 0x100a758e0, 0xa0) && object(manager, 0x100a75a28, 8) &&
            slot(manager, 0x70, 0x100131184) && slot(manager - 8, 0x80, 0x100131192), "lot_manager_interface_or_slots");
        const uint64_t lot = original<uint64_t(*)(const void*, const void*)>(0x100131184)(ptr(manager), ptr(occupant));
        LC_REQUIRE(object(lot, 0x100a79fb0, 0xce) && slot(lot, 0x208, 0x100183b3e) &&
            slot(lot, 0x210, 0x100183b50), "selected_lot_concrete_base_or_slots");
        const bool has = original<bool(*)(const void*)>(0x100183b50)(ptr(lot));
        const bool needs = original<bool(*)(const void*)>(0x100183b3e)(ptr(lot));
        uint8_t flags = 0;
        LC_REQUIRE(read(lot + 0xcd, flags), "lot_flags_read"); // getters @ 0x100183b3e/50.
        LC_REQUIRE(read(pollution + 0x60, power) && object(power, 0x100a86a30, 8) &&
            slot(power, 0xf8, 0x10021485c), "power_pointer_or_slot");
        const bool powered = original<bool(*)(const void*, const void*)>(0x10021485c)(ptr(power), ptr(occupant));
        uint32_t scale = 0;
        LC_REQUIRE(read(pollution + 0x348, scale), "scale_read");
        logf_("LOADED_CAPACITY POWER manager=%llx lot=%llx flagsCD=%02x hasPower=%u needsPower=%u power=%llx isPowered=%u scale_raw=%08x lot_config=unavailable lot_bounds=unavailable", U(manager), U(lot), unsigned(flags), unsigned(has), unsigned(needs), U(power), unsigned(powered), scale);
        // GetCurrentCapacity @ 0x1000992cc: only original read paths and a LOCAL
        // response curve. Deliberate calls, not natural F0 dispatch or aging.
        unsigned char entry = 0;
        LC_REQUIRE(read(0x1000992cc + g_slide, entry), "capacity_entry_unreadable");
        const uint32_t capacity_ids[] = {0x08fd1613,0x48ebd0b1,0x27812852,1};
        for (uint32_t id : capacity_ids) {
            uint32_t out = 0xa55a5aa5;
            const bool result = original<bool(*)(const void*, uint32_t, const void*, uint32_t&)>(0x1000992cc)(ptr(pollution), id, ptr(occupant), out);
            logf_("LOADED_CAPACITY CAPACITY id=%08x invocation=deliberate_original sentinel=a55a5aa5 returned=%u output=%08x decimal=%u", id, unsigned(result), out, out);
            ++capacities; // false is captured evidence, not a harness failure.
        }
        uint64_t final_node = 0, final_city = 0; uint32_t final_state = 0, final_scale = 0;
        LC_REQUIRE(read(app + 0x78, final_state) && final_state == state && read(app + 0xa0, final_city) && final_city == city &&
            read(sentinel + 8, final_node) && final_node == node && read(pollution + 0x348, final_scale) && final_scale == scale,
            "state_changed_during_capture");
        LC_REQUIRE(properties == 6 && capacities == 4 && curve_words == 16, "incomplete_casecounts");
#undef LC_REQUIRE
        return true;
    }
};
} // namespace loaded_capacity_capture_detail
static void loaded_capacity_capture_exec(void*) {
    logf_("LOADED_CAPACITY BEGIN version=1 expected_properties=6 expected_capacities=4 expected_curve_words=16 thread=main_required");
    loaded_capacity_capture_detail::Capture capture;
    bool complete = false;
    try { complete = capture.run(); }
    catch (...) { capture.failure = "original_call_exception"; }
    if (!complete) logf_("LOADED_CAPACITY FAIL reason=%s", capture.failure ? capture.failure : "unknown");
    logf_("LOADED_CAPACITY END status=%s complete=%u properties=%u/6 capacities=%u/4 curve_words=%u/16 parity=not_tested natural_dispatch=not_tested aging=not_tested",
        complete ? "CAPTURE_COMPLETE" : "FAIL", unsigned(complete), capture.properties, capture.capacities, capture.curve_words);
}
