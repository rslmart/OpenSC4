// clean-room fallback (not authentic): prepare-only bounded diagnostic adapter.
// No RPC registration. Independent prerun review required; no game approval.
#ifndef OPENSC4_TUTORIAL_FINISH_FRAGMENT_INC
#define OPENSC4_TUTORIAL_FINISH_FRAGMENT_INC
#ifndef OPENSC4_TUTORIAL_OPEN_FRAGMENT_INC
#error "include actual base/bindings/inspect/open first"
#endif
#if !defined(__x86_64__) || __cplusplus < 201703L
#error "C++17 x86_64 required"
#endif
namespace tutorial_checkpoint {
static bool preflight(void*);
static void save_after_exit(void*);
static void finish_end(void*,bool);
}
namespace tutorial_finish {
using namespace fixture_placement;
namespace fb=fixture_placement_bindings;
namespace op=tutorial_open;
namespace ti=tutorial_inspect;
static std::atomic<bool> attempted{false};
// Machine-generated from SHA-bound source bytes, fresh --no-verify dumps in log.
static constexpr uint64_t button_table[] = {
    0x10027fe28,0x10027fe5c,0x10027fe74,0x100281d98,0x10027ffb0,0x100280096,
    0x10016d194,0x10016d19e,0x10016d1ae,0x10016d1b8,0x10016ccfc,0x100169784,
    0x10016978e,0x10016a044,0x100169806,0x10016990c,0x1001699c6,0x100169ae6,
    0x1001699dc,0x10016a062,0x10016a0d0,0x10016a444,0x10016a476,0x100169f0a,
    0x100169f2e,0x100169b64,0x100169d2a,0x100169d54,0x100169df2,0x100169ea0,
    0x100169f52,0x100169faa,0x10016c582,0x10016c6a6,0x10016a138,0x10016a19c,
    0x10016a38e,0x10016a3c4,0x10016cd0c,0x10016a222,0x10016a2ce,0x10016c7e2,
    0x10016c7f4,0x10016c806,0x10016c812,0x10016c81e,0x10016c82a,0x10016c966,
    0x10016c97e,0x10016c974,0x10016c99a,0x10016c8e6,0x10016c910,0x10016c93a,
    0x100280d4a,0x10016c8c6,0x10016c9c8,0x10016c9f8,0x10016a4e8,0x10016ca3e,
    0x10016ca2e,0x10016a3fa,0x10016cb3c,0x1002800ce,0x1002800b6,0x10016d0cc,
    0x10016d0be,0x10016cbde,0x10028066c,0x10016cbee,0x10016cc52,0x10016ccb2,
    0x10016ccc8,0x1002810ea,0x1002810cc,0x10016a8a6,0x10016a8b4,0x10016a8f6,
    0x10016d0d8,0x10028010e,0x10016d0f0,0x10016d112,0x10016bdd8,0x10016a9a0,
    0x10016a914,0x10016a984,0x10016cd22,0x10016cd2c,0x1002814cc,0x10016b86c,
    0x100169556,0x10016d120,0x10016d12e,0x10016d16a,0x10016d178,0x10016b8b2,
    0x10016ba20,0x10016ba70,0x10016bd86,0x1002748a0,0x10016be44,0x10016d186,
    0x10016d2b2,0x10016d29c,0x10016d24a,0x10016d1c8,0x10016d280,0x10016d230,
    0x10016c382,0x10016d3d4,0x10016d3e6,0x10016d3ba,0x10016d3c6,0x10016ce30,
    0x10016cd36,0x10016ce82,0x10016cf14,0x10016cfde,0x10016c9b0,0x100169672,
    0x1001696dc,0x10016ca8c,0x10016cb10,0x10016abd0,0x10016b49a,0x10016d23e,
    0x10016d290,0x10016c3fe,0x10016c482,0x10016a82a,0x100281566,0x1002815e2,
    0x10016a84a,0x10016a896,0x100281736,0x1002817f2,0x1002818dc,0x1002819de,
    0x1002819f8,0x10016d300,0x100281c30,0x100281b88,0x100281bc0,0x10016a842,
    0x10016d318,0x10016d350,0x10016d37c,0x10016d38e,
};
static constexpr uint64_t button_primary[] = {
    0x10027fe06,0x10027fe4e,0x10027fe66,
};
// Exact cGZWinFlatRect primary table @0x100a903d0, byte-generated.
static constexpr uint64_t flatrect_table[] = {
    0x100294404,0x100294454,0x100294470,0x10029448c,0x100168dbe,0x100168e56,
    0x10016d194,0x10016d19e,0x10016d1ae,0x10016d1b8,0x10016ccfc,0x100169784,
    0x10016978e,0x10016a044,0x100169806,0x10016990c,0x1001699c6,0x100169ae6,
    0x1001699dc,0x10016a062,0x10016a0d0,0x10016a444,0x10016a476,0x100169f0a,
    0x100169f2e,0x100169b64,0x100169d2a,0x100169d54,0x100169df2,0x100169ea0,
    0x100169f52,0x100169faa,0x10016c582,0x10016c6a6,0x10016a138,0x10016a19c,
    0x10016a38e,0x10016a3c4,0x10016cd0c,0x10016a222,0x10016a2ce,0x10016c7e2,
    0x10016c7f4,0x10016c806,0x10016c812,0x10016c81e,0x10016c82a,0x10016c966,
    0x10016c97e,0x10016c974,0x10016c99a,0x10016c8e6,0x10016c910,0x10016c93a,
    0x10016c836,0x10016c8c6,0x10016c9c8,0x10016c9f8,0x10016a4e8,0x10016ca3e,
    0x10016ca2e,0x10016a3fa,0x10016cb3c,0x10016d0b4,0x10016d0aa,0x10016d0cc,
    0x10016d0be,0x10016cbde,0x100168f7e,0x10016cbee,0x10016cc52,0x10016ccb2,
    0x10016ccc8,0x10016d092,0x10016d09c,0x10016a8a6,0x10016a8b4,0x10016a8f6,
    0x10016d0d8,0x10016d0e4,0x10016d0f0,0x10016d112,0x10016bdd8,0x10016a9a0,
    0x10016a914,0x10016a984,0x10016cd22,0x10016cd2c,0x10029450a,0x10016b86c,
    0x100169556,0x10016d120,0x10016d12e,0x10016d16a,0x10016d178,0x10016b8b2,
    0x10016ba20,0x10016ba70,0x10016bd86,0x1002748a0,0x10016be44,0x10016d186,
    0x100294926,0x1002948ee,0x10029490a,0x1002948d2,0x10016d280,0x10016d230,
    0x10016c382,0x10016d3d4,0x10016d3e6,0x10016d3ba,0x10016d3c6,0x10016ce30,
    0x10016cd36,0x10016ce82,0x10016cf14,0x10016cfde,0x10016c9b0,0x100169672,
    0x1001696dc,0x10016ca8c,0x10016cb10,0x10016abd0,0x10016b49a,0x10016d23e,
    0x10016d290,0x10016c3fe,0x10016c482,0x10016a82a,0x10029467e,0x1002946b4,
    0x10016a84a,0x10016a896,0x10016d2d8,0x10016d2e8,0x10016d2e0,0x10016d2f0,
    0x10016d2f8,0x10016d300,0x10016a89e,0x10016d308,0x10016d310,0x10016a842,
    0x10016d318,0x10016d350,0x10016d37c,0x10016d38e,
};
static constexpr uint64_t message_table[] = {
    0x100053ea0,0x100053f0e,0x100053f34,0x100053f6a,0x100054c8e,
};
static constexpr uint64_t finish_ui_table[] = {
    0x1000b7b16,0x1000b7b6e,0x1000b7b94,0x1000b6c36,0x1000b6c40,0x10003c93c,
    0x10003c952,0x1000b7fdc,0x1000b89dc,0x1000bb0a0,0x1000bb28e,0x1000bb114,
    0x1000b8812,
};
static constexpr uint64_t task_table[] = {
    0x10011e52e,0x10011e576,0x10011e58e,0x10011d7b6,0x10011e702,0x10011e7b2,
    0x10011d762,0x10011d77a,0x10011e716,0x10011e72e,0x10011e79e,0x10011e7a8,
    0x10011e794,0x10011e70a,0x10011e7c0,0x10011e526,0x10011eb7e,0x10011f1a4,
    0x10011edfe,
};
static constexpr Entry native_entries[] = {
    {0x10003c912,{0x55,0x48,0x89,0xe5,0x8b,0x47,0x08,0xff,0xc0,0x89,0x47,0x08,0x5d,0xc3,0x8b,0x47}},
    {0x10003d2ce,{0x55,0x48,0x89,0xe5,0x48,0x83,0xc7,0x08,0x5d,0xe9,0x36,0xf6,0xff,0xff,0x55,0x48}},
    {0x10004871c,{0x55,0x48,0x89,0xe5,0x48,0x8b,0x87,0x80,0x03,0x00,0x00,0x5d,0xc3,0x90,0x55,0x48}},
    {0x100053f6a,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec}},
    {0x100054c8e,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x41,0x89,0xd4,0x49,0x89}},
    {0x10005f3ae,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x53,0x50,0x41,0x89,0xf6,0x49,0x89,0xff}},
    {0x1000b6c40,{0x55,0x48,0x89,0xe5,0x41,0x56,0x53,0x48,0x89,0xfb,0xe8,0xd1,0xfd,0xff,0xff,0x48}},
    {0x1000b7470,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x50,0x49,0x89}},
    {0x1000b7bba,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x53,0x50,0x41,0x89,0xf7,0x48,0x8b,0x07}},
    {0x1000b7fdc,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x48,0x83,0xec,0x40,0x49}},
    {0x1000b8812,{0x55,0x48,0x89,0xe5,0x8b,0x47,0x60,0x48,0x8b,0x97,0x90,0x00,0x00,0x00,0x48,0x8b}},
    {0x10011edfe,{0x55,0x48,0x89,0xe5,0x41,0x56,0x53,0x48,0x89,0xfb,0x41,0xb6,0x01,0x80,0xbb,0x00}},
    {0x100168d90,{0x55,0x48,0x89,0xe5,0x8b,0x47,0x18,0xff,0xc0,0x89,0x47,0x18,0x5d,0xc3,0x8b,0x47}},
    {0x100169622,{0x55,0x48,0x89,0xe5,0x41,0x56,0x53,0x49,0x89,0xf6,0x41,0xff,0x4e,0x18,0x74,0x05}},
    {0x10016a19c,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x50,0x41,0x89}},
    {0x10016cbde,{0x55,0x48,0x89,0xe5,0x85,0xb7,0x40,0x01,0x00,0x00,0x0f,0x95,0xc0,0x5d,0xc3,0x90}},
    {0x10016cc52,{0x55,0x48,0x89,0xe5,0x53,0x50,0x48,0x89,0xfb,0x48,0x8b,0x03,0xbe,0x01,0x00,0x00}},
    {0x10016d194,{0x55,0x48,0x89,0xe5,0x48,0x8b,0x47,0x08,0x5d,0xc3,0x55,0x48,0x89,0xe5,0x48,0x89}},
    {0x10016d37c,{0x55,0x48,0x89,0xe5,0x48,0x8b,0x7f,0x08,0x48,0x8b,0x07,0xff,0x50,0x70,0xb0,0x01}},
    {0x10016d38e,{0x55,0x48,0x89,0xe5,0x48,0x83,0xec,0x20,0x48,0x8d,0x45,0xe0,0x89,0x10,0x48,0x89}},
    {0x1001f19d8,{0x55,0x48,0x89,0xe5,0x48,0x81,0xc7,0x00,0x01,0x00,0x00,0x5d,0xe9,0x91,0xd3,0xe4}},
    {0x10027fe5c,{0x55,0x48,0x89,0xe5,0x5d,0xe9,0x2a,0x8f,0xee,0xff,0x55,0x48,0x89,0xe5,0x48,0x83}},
    {0x1006213ac,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x54,0x53,0x48,0x83,0xec,0x40,0x49}},
    {0x1006228ba,{0x55,0x48,0x89,0xe5,0x41,0x57,0x41,0x56,0x41,0x55,0x41,0x54,0x53,0x48,0x83,0xec}},
};
static bool guards() {
    if(!op::guards()) return false;
    for(const Entry& e:native_entries) {
        unsigned char b[16];
        if(!read(relocated(e.address),b,16) || std::memcmp(b,e.bytes,16)) return false;
    }
    return true;
}
static bool response_slots(const op::Capture& c,uint64_t city) {
    return slot(c.dialog,0x30,0x10016d194) &&
        slot(c.dialog,0x230,0x10016cc52) &&
        slot(c.mgr,0x48,0x1006213ac) &&
        slot(city,0x188,0x10004871c) &&
        slot(c.ui,0x60,0x1000b8812) &&
        slot(c.ui,0x20,0x1000b6c40) &&
        slot(c.task,0x90,0x10011edfe);
}
struct Lease {
    uint64_t p=0,vt=0,add=0; unsigned counter=0; bool entered=false,proven=false;
};
struct Control {
    uint64_t child=0,parent=0,dialog_parent=0,dialog_proc=0;
    uint32_t command=0,flags=0;
    ti::Snapshot ancestry{};
};
struct Job {
    explicit Job(bool inspect,bool checkpoint=false):checkpoint_mode(checkpoint),inspect_only(inspect) {}
    const bool checkpoint_mode;
    const bool inspect_only; // fixed by noarg entry, never parsed from user text
    bool inventory_complete=false;
    fb::Owner owner{}; State before{}; op::Capture first{}; Control control{};
    op::SuccessfulReceipt receipt{};
    Lease leases[5]{};
    unsigned retain_entries=0,lookup_entries=0,flag_entries=0,post_entries=0;
    bool pending=false,ended=false,post_return_known=false,post_return=false;
};
// Raw pointer: intentionally no destructor, Release, shutdown hook, or retry.
static Job* quarantine=nullptr;
static bool lease_guard(const Lease& l) {
    if(l.vt==0x100a899c0) return op::table(l.p,l.vt,op::window_table);
    if(l.vt==0x100a6ee40) return op::table(l.p,l.vt,finish_ui_table);
    if(l.vt==0x100ad8970) return op::table(l.p,l.vt,op::manager_table);
    if(l.vt==0x100a8f660) return l.p>=8 && op::table(l.p,l.vt,button_table) &&
        op::table(l.p-8,0x100a8f410,button_primary);
    return false;
}
static bool retain(Job& j,Lease& l) {
    uint32_t before=0,after=0;
    if(!lease_guard(l) || !read(l.p+l.counter,before) || !before || before>=UINT32_MAX-1) return false;
    l.entered=true; ++j.retain_entries; // Unknown native outcome quarantines too.
    logf_("TUTORIAL_FINISH RETAIN_ENTRY p=%llx vt=%llx counter=%x before=%u operation=%u",U(l.p),U(l.vt),l.counter,before,j.retain_entries);
    const uint32_t result=original<uint32_t(*)(uint64_t)>(l.add)(l.p);
    const bool readable=read(l.p+l.counter,after);
    l.proven=readable && after==before+1 && result==after;
    logf_("TUTORIAL_FINISH RETAIN_RESULT p=%llx readable=%u before=%u after=%u result=%u proven=%u",U(l.p),unsigned(readable),before,after,result,unsigned(l.proven));
    return l.proven;
}
static bool receipt_matches(const State& s,const op::Capture& c,const op::SuccessfulReceipt& r) {
    return r.valid && r.dialog && r.city==s.city && r.date==s.date &&
        r.ui==c.ui && r.w==c.w && r.mgr==c.mgr && r.task==c.task && r.dialog==c.dialog;
}
static bool domain(const op::Capture& c) {
    uint8_t active=0,finish=0,task=0,a=0,b=0; uint32_t fn=0; uint64_t dep=0;
    // ADAPTER DOMAIN, not additional original handler guards. EndTask @0x10011edfe.
    return read(c.tutorial+0x2f,active) && active==1 && read(c.ui+0x73,finish) && !finish &&
        read(c.task+0x100,task) && task==1 && read(c.task+0x104,fn) && fn==11 &&
        read(c.task+0x128,a) && !a && read(c.task+0x129,b) && !b &&
        read(c.task+0x140,dep) && !dep && op::table(c.ui,0x100a6ee40,finish_ui_table) &&
        op::table(c.task,0x100a75330,task_table);
}
// Mach-only validation: conservative known-class ancestry, maximum 32 nodes.
// Bounds are diagnostic policy. No native getter/repair of an unfamiliar parent.
static bool control(const op::Capture& c,uint64_t child,Control& out) {
    if(!child || child<8 || !object(child,0x100a8f660,0x154) ||
       !op::table(child,0x100a8f660,button_table) ||
       !op::table(child-8,0x100a8f410,button_primary) ||
       !object(c.dialog,0x100a899c0,0x1a0) || !op::table(c.dialog,0x100a899c0,op::window_table)) return false;
    uint32_t id=0; uint64_t manager=0,target=0,dm=0;
    out.child=child;
    if(!read(child+0x1c,id) || id!=0x6aaeec42 || !read(child+8,manager) || manager!=c.mgr ||
       !read(child+0x60,target) || target!=c.w || !read(c.dialog+8,dm) || dm!=c.mgr ||
       !read(child+0x58,out.parent) || !read(child+0x150,out.command) ||
       !read(child+0x140,out.flags) || !read(c.dialog+0x58,out.dialog_parent) ||
       !read(c.dialog+0x198,out.dialog_proc)) return false;
    uint64_t node=out.parent;
    for(unsigned i=0;i<32;++i) {
        uint64_t vt=0,m=0,next=0;
        if(!node || !read(node,vt)) return false;
        const bool known=vt==relocated(0x100a899c0) ?
            object(node,0x100a899c0,0x1a0) && op::table(node,0x100a899c0,op::window_table) :
            // Exact inherited cGZWin layout: FlatRect ctor @0x1002942de;
            // manager @0x10016d194, parent @0x100169784. Extent is adapter policy.
            vt==relocated(0x100a903d0) ?
            object(node,0x100a903d0,0x60) && op::table(node,0x100a903d0,flatrect_table) :
            vt==relocated(0x100a8f660) && node>=8 && object(node,0x100a8f660,0x154) &&
            op::table(node,0x100a8f660,button_table) && op::table(node-8,0x100a8f410,button_primary);
        if(!known || !read(node+8,m) || m!=c.mgr || !read(node+0x58,next)) return false;
        logf_("TUTORIAL_FINISH PARENT index=%u node=%llx vptr=%llx manager=%llx parent=%llx root_dialog=%u",i,U(node),U(vt),U(m),U(next),unsigned(node==c.dialog));
        if(!out.ancestry.add(node) || !out.ancestry.add(vt) || !out.ancestry.add(next)) return false;
        if(node==c.dialog) return true;
        node=next;
    }
    return false;
}
// clean-room fallback (not authentic): raw diagnostic inventory, NOT class proof.
// Offsets mirror control() above; unknown storage has NO inferred semantics.
// Mach readability is neither an allocation nor a lifetime guarantee.
template<class T> struct RawField {
    T value{};
    const char* status="not_read";
    bool readable=false;
};
template<class T> static RawField<T> raw_field(const char* group,unsigned index,
                                             uint64_t base,uint64_t offset) {
    RawField<T> r;
    uint64_t address=0;
    if(!base) r.status="absent_base";
    else if(offset>UINT64_MAX-base || sizeof(T)-1>UINT64_MAX-(base+offset)) r.status="overflow";
    else {
        address=base+offset;
        r.readable=read(address,r.value);
        r.status=r.readable?"read":"unreadable";
    }
    // Value is meaningful ONLY when status=read; failed reads never mean zero.
    logf_("TUTORIAL_RESPONSE_INSPECT RAW group=%s index=%u base=%llx offset=%llx address=%llx status=%s readable=%u raw=%llx raw_only=1",
        group,index,U(base),U(offset),U(address),r.status,unsigned(r.readable),U(r.value));
    return r;
}
static bool raw_inventory(const op::Capture& c,uint64_t child) {
    logf_("TUTORIAL_RESPONSE_INSPECT INVENTORY child=%llx expected_W=%llx expected_M=%llx expected_D=%llx class_validation=0 unknown_native_calls=0",
        U(child),U(c.w),U(c.mgr),U(c.dialog));
    bool complete=true;
    auto u64=[&](const char* group,unsigned i,uint64_t base,uint64_t offset) {
        auto r=raw_field<uint64_t>(group,i,base,offset);
        if(base && !r.readable) complete=false;
        return r;
    };
    auto u32=[&](const char* group,unsigned i,uint64_t base,uint64_t offset) {
        auto r=raw_field<uint32_t>(group,i,base,offset);
        if(base && !r.readable) complete=false;
        return r;
    };
    u64("child_vptr",0,child,0);
    if(child>=8) u64("child_minus8_possible_vptr",0,child-8,0);
    else {
        logf_("TUTORIAL_RESPONSE_INSPECT RAW group=child_minus8_possible_vptr status=%s readable=0 raw_only=1",child?"subtraction_invalid":"absent_base");
        if(child) complete=false;
    }
    u32("child_ID",0,child,0x1c);
    u64("child_manager",0,child,8);
    u64("child_target",0,child,0x60);
    const auto parent=u64("child_parent",0,child,0x58);
    u32("child_flags",0,child,0x140);
    u32("child_command",0,child,0x150);
    u64("dialog_vptr",0,c.dialog,0);
    u64("dialog_manager",0,c.dialog,8);
    u64("dialog_parent",0,c.dialog,0x58);
    u64("dialog_proc",0,c.dialog,0x198);
    uint64_t seen[32]{};
    uint64_t node=parent.value;
    const char* termination=child?"parent_unreadable":"null_child";
    if(parent.readable) {
        for(unsigned i=0;;++i) {
            if(!node) { termination="null_parent"; break; }
            bool cycle=false;
            for(unsigned k=0;k<i;++k) if(seen[k]==node) cycle=true;
            if(cycle) { complete=false; termination="cycle"; break; }
            if(i==32) { complete=false; termination="bound32"; break; }
            seen[i]=node;
            const auto vt=u64("parent_vptr",i,node,0);
            const auto mgr=u64("parent_manager",i,node,8);
            const auto next=u64("parent_next",i,node,0x58);
            if(!vt.readable || !mgr.readable || !next.readable) {
                complete=false; termination="node_unreadable"; break;
            }
            // Numerical equality only: no inferred class or object identity.
            if(node==c.dialog) { termination="raw_address_equals_D"; break; }
            node=next.value;
        }
    }
    logf_("TUTORIAL_RESPONSE_INSPECT INVENTORY_RESULT inventoryComplete=%u termination=%s binding=unsupported_or_unvalidated raw_only=1 unknown_native_calls=0",
        unsigned(complete),termination);
    return complete;
}
static bool equal_control(const Control& a,const Control& b) {
    return a.child==b.child && a.parent==b.parent && a.dialog_parent==b.dialog_parent &&
        a.dialog_proc==b.dialog_proc && a.command==b.command && a.flags==b.flags &&
        ti::equal(a.ancestry,b.ancestry);
}
static void end(Job& j,bool complete,const char* reason) {
    if(j.ended) return;
    j.ended=true;
    if(j.checkpoint_mode) tutorial_checkpoint::finish_end(&j,complete);
    if(j.inspect_only) {
        logf_("TUTORIAL_RESPONSE_INSPECT RETAINED_LEASE quarantine=1 max_contexts=1 acquired_or_uncertain=%u,%u,%u,%u,%u fbBusy_held=1 immediate_process_stop_required=1 noRetry=1 no_native_cleanup=1",
            unsigned(j.leases[0].entered),unsigned(j.leases[1].entered),unsigned(j.leases[2].entered),unsigned(j.leases[3].entered),unsigned(j.leases[4].entered));
        logf_("TUTORIAL_RESPONSE_INSPECT END inventoryComplete=%u reason=%s retain_entries=%u lookup_entries=%u flag_entries=%u post_entries=%u unknown_native_calls=0 delayed_observation=0 tutorial_exit_claim=0 immediate_process_stop_required=1",
            unsigned(j.inventory_complete),reason,j.retain_entries,j.lookup_entries,j.flag_entries,j.post_entries);
        return;
    }
    logf_("TUTORIAL_FINISH RETAINED_LEASE quarantine=1 max_contexts=1 max_refs=5 acquired_or_uncertain=%u,%u,%u,%u,%u fbBusy_held=1 immediate_process_stop_required=1 noRetry=1 no_native_cleanup=1 pins_allocation_not_Shutdown=1",
        unsigned(j.leases[0].entered),unsigned(j.leases[1].entered),unsigned(j.leases[2].entered),unsigned(j.leases[3].entered),unsigned(j.leases[4].entered));
    logf_("TUTORIAL_FINISH END complete=%u reason=%s pending_or_uncertain=%u retain_entries=%u lookup_entries=%u flag_entries=%u post_entries=%u queue_return_known=%u queue_return=%u",
        unsigned(complete),reason,unsigned(j.pending),j.retain_entries,j.lookup_entries,j.flag_entries,j.post_entries,unsigned(j.post_return_known),unsigned(j.post_return));
}
// Fresh core-only post-exit reader. NEVER state(): its explicit-pause rejection
// precedes date. NEVER copy stale view/renderer/rect fields into a fresh State.
// get_app/GetDate/loaded_path native Mach-fault boundaries remain; catch is not containment.
static bool exit_state(State& s) {
    uint32_t app_state=0; uint64_t city=0,sim=0;
    s=State{}; s.app=get_app();
    if(!object(s.app,0x100a67010,0xa8) || !read(s.app+0x78,app_state) || app_state!=2 ||
       !read(s.app+0xa0,s.city) || !object(s.city,0x100a692e0,0x348) ||
       !read(relocated(0x100b21308),city) || city!=s.city ||
       !read(s.city+0x258,s.sim) || !object(s.sim,0x100a841e0,0x118) ||
       !read(relocated(0x100b213e8),sim) || sim!=s.sim ||
       !slot(s.sim,0x40,0x1001f19d8) || !read(s.sim+0x48,s.pauses)) return false;
    // GetDate @0x1001f19d8 runs regardless of ALL three signed counter values.
    s.date=original<uint32_t(*)(uint64_t)>(0x1001f19d8)(s.sim);
    logf_("TUTORIAL_FINISH EXIT_CORE city=%llx sim=%llx date=%u pauses=%d,%d,%d before_acceptance=1",U(s.city),U(s.sim),s.date,s.pauses[0],s.pauses[1],s.pauses[2]);
    return true;
}
static void observe(void* opaque) {
    Job& j=*static_cast<Job*>(opaque);
    logf_("TUTORIAL_FINISH OBSERVE main_queue=%u callbacks=1 delay_is_not_proof=1",unsigned(pthread_main_np()!=0));
    const char* reason="incomplete_core_or_identity"; bool complete=false;
    try {
        State now{};
        if(pthread_main_np() && guards() && exit_state(now)) {
            logf_("TUTORIAL_FINISH DELTA date=%lld pause=%lld,%lld,%lld",
                static_cast<long long>(now.date)-j.before.date,
                static_cast<long long>(now.pauses[0])-j.before.pauses[0],
                static_cast<long long>(now.pauses[1])-j.before.pauses[1],
                static_cast<long long>(now.pauses[2])-j.before.pauses[2]);
            // identity consumes only newly acquired app/city; no view requirement.
            if(now.app==j.before.app && now.city==j.before.city && now.sim==j.before.sim && fb::identity(&j.owner,now)) {
                uint64_t system=0,ui=0; uint8_t active=0,ended=0;
                if(read(now.city+0x340,system)) {
                    if(!system) reason="incomplete_null_system";
                    else if(system!=j.first.tutorial || !object(system,0x100a6b7c0,0x68)) reason="incomplete_changed_or_unknown_system";
                    else if(read(system+0x2f,active) && read(system+0x60,ui) && read(system+0x3c,ended)) {
                        logf_("TUTORIAL_FINISH EXIT_SYSTEM system=%llx raw2f=%u system60=%llx raw3c=%u old_UI_task_window_dereferences=0",U(system),unsigned(active),U(ui),unsigned(ended));
                        complete=!active && !ui && ended==1;
                        reason=complete?"EXITOBSERVED":"incomplete_nonterminal_or_partial";
                        if(complete && j.checkpoint_mode) tutorial_checkpoint::save_after_exit(&j);
                    } else reason="incomplete_system_read";
                }
            }
        }
    } catch(...) { reason="cpp_exception_native_faults_not_caught"; }
    // No old UI/task/window/dialog/child dereference or pause repair after posting.
    end(j,complete,reason);
}
static void begin(void* opaque) {
    Job& j=*static_cast<Job*>(opaque);
    logf_(j.inspect_only?"TUTORIAL_RESPONSE_INSPECT BEGIN main_queue=%u process_attempt_consumed=1 observation_only=1 post_entries=0 unknown_native_calls=0 tutorial_exit_claim=0":"TUTORIAL_FINISH BEGIN main_queue=%u process_attempt_consumed=1 prepare_only_contract=1",unsigned(pthread_main_np()!=0));
    const char* reason="prepost_guard_or_missing_receipt";
    try {
        auto run=[&]() -> bool {
            if(!pthread_main_np() || !guards() || !op::successful_receipt.valid || !op::attempted.load() || !state(j.before)) return false;
            j.receipt=op::successful_receipt; // main-queue immutable same-process receipt
            auto& o=j.owner;
            if(!fb::loaded_path(j.before,o.path,o.regional)) return false;
            const char* name=std::strrchr(o.path,'/');
            if(!name || std::strcmp(name+1,"City - Rush Hour Tutorial.sc4")) return false;
            o.city=j.before.city; o.receipt=j.before; o.inspected=true;
            if(!fb::identity(&o,j.before) || !op::capture(j.before,j.first,"FINISH_BEFORE") ||
               !receipt_matches(j.before,j.first,j.receipt) || !domain(j.first)) return false;
            if(j.checkpoint_mode && !tutorial_checkpoint::preflight(&j)) { reason="checkpoint_preflight"; return false; }
            auto& c=j.first;
            j.leases[0]={c.w,0x100a899c0,0x10024de10,0x18};
            j.leases[1]={c.ui,0x100a6ee40,0x1000b7b6e,8};
            j.leases[2]={c.mgr,0x100ad8970,0x100621eb4,0x18};
            // Discard borrowed D across these native calls; reacquire it afterwards.
            c.dialog=0;
            for(unsigned i=0;i<3;++i) if(!retain(j,j.leases[i])) { reason="retain_unknown_or_failed"; return false; }
            if(!read(c.ui+0x48,c.dialog) || c.dialog!=j.receipt.dialog ||
               !object(c.dialog,0x100a899c0,0x1a0) || !op::table(c.dialog,0x100a899c0,op::window_table)) return false;
            uint64_t dm=0;
            if(!read(c.dialog+8,dm) || dm!=c.mgr) return false;
            j.leases[3]={c.dialog,0x100a899c0,0x10024de10,0x18};
            if(!retain(j,j.leases[3])) { reason="dialog_retain_unknown_or_failed"; return false; }
            if(!response_slots(c,j.before.city)) return false;
            // GetChildWindowFromIDRecursive @0x10016a19c returns BORROWED.
            // Native traversal can fault; unlock @0x100169622 may release children.
            ++j.lookup_entries;
            logf_("TUTORIAL_FINISH LOOKUP_ENTRY dialog=%llx calls=%u borrowed_result=1 native_traversal_fault_and_unlock_side_effects=1",U(c.dialog),j.lookup_entries);
            const uint64_t child=original<uint64_t(*)(uint64_t,uint32_t)>(0x10016a19c)(c.dialog,0x6aaeec42);
            // No native call on any raw/unknown child or parent in this inventory.
            j.inventory_complete=raw_inventory(c,child);
            if(j.inspect_only) {
                reason=j.inventory_complete?"raw_inventory_complete_binding_unvalidated":"raw_inventory_incomplete";
                return false; // unconditional stop before child retain/flags/services/post
            }
            // Mach-only class/ancestry validation before ANY further native call.
            if(!control(c,child,j.control)) { reason="HPR_actual_child_class_or_parent_or_target_unsupported"; return false; }
            j.leases[4]={child,0x100a8f660,0x10027fe5c,0x18};
            // Window-secondary thunk uses C UNCHANGED, not C-8 @0x10027fe5c.
            if(!retain(j,j.leases[4])) { reason="child_retain_unknown_or_failed"; return false; }
            ++j.flag_entries;
            const bool visible=original<bool(*)(uint64_t,uint32_t)>(0x10016cbde)(child,1);
            ++j.flag_entries;
            const bool enabled=original<bool(*)(uint64_t,uint32_t)>(0x10016cbde)(child,2);
            logf_("TUTORIAL_FINISH CONTROL child=%llx primary=%llx ID=6aaeec42 raw_command=%x p3=%u local_flags=%x local_visible=%u local_enabled=%u full_GUI_eligibility=UNPROVEN calls=%u",U(child),U(child-8),j.control.command,j.control.command&1,j.control.flags,unsigned(visible),unsigned(enabled),j.flag_entries);
            // Conservative SUPPORTED AVAILABILITY, NOT original handler guards.
            if(!visible || !enabled || (j.control.flags&3)!=3) { reason="unsupported_local_control_availability"; return false; }
            uint64_t ms=0;
            if(!read(relocated(0x100b212c8),ms) || !op::table(ms,0x100a6afb8,message_table)) return false;
            logf_("TUTORIAL_FINISH MESSAGE_SERVICE object=%llx vptr=%llx send18=100053f6a post20=100054c8e calls=0 own_message_and_recipient_retention=original",U(ms),U(relocated(0x100a6afb8)));
            State now{}; op::Capture again{}; Control final{}; uint64_t global_sim=0;
            if(!guards() || !state(now) || !same(j.before,now) || !fb::identity(&o,now) ||
               !op::capture(now,again,"FINISH_PREPOST") || !receipt_matches(now,again,j.receipt) ||
               !domain(again) || !ti::equal(c.stable,again.stable) || !response_slots(again,now.city) ||
               !read(relocated(0x100b213e8),global_sim) || global_sim!=now.sim ||
               !read(relocated(0x100b212c8),ms) || !op::table(ms,0x100a6afb8,message_table) ||
               !control(again,child,final) || !equal_control(j.control,final)) { reason="prepost_current_chain_changed"; return false; }
            if(j.checkpoint_mode && !tutorial_checkpoint::preflight(&j)) { reason="checkpoint_prepost"; return false; }
            // Scalar PostMsg @0x10016d38e: bool(source,target,u32,u64,u64,u64).
            // W/W supported ONLY by current real child notification target/same mgr.
            j.pending=true; ++j.post_entries; // BEFORE entry; unknown outcome => no retry.
            j.post_return=original<bool(*)(uint64_t,uint64_t,uint32_t,uint64_t,uint64_t,uint64_t)>(0x10016d38e)
                (again.w,again.w,3,0x287259f6,0x6aaeec42,final.command&1);
            j.post_return_known=true;
            return true;
        };
        run();
    } catch(...) { reason="cpp_exception_native_faults_not_caught"; }
    if(j.inspect_only) { end(j,false,reason); return; }
    logf_("TUTORIAL_FINISH POSTED entered=%u return_known=%u native_bool=%u delivery_proven=0",j.post_entries,unsigned(j.post_return_known),unsigned(j.post_return));
    if(j.pending) {
        // ONE callback. Normal window queue, then RZ aa3840e6 teardown own progress.
        // 500ms is a bounded sample, NOT completion proof. Never drain/repost.
        dispatch_after_f(dispatch_time(DISPATCH_TIME_NOW,500*NSEC_PER_MSEC),dispatch_get_main_queue(),&j,observe);
    } else {
        logf_("TUTORIAL_FINISH OBSERVE scheduled=0 incomplete_prepost=1");
        end(j,false,reason);
    }
}
// This bool is immutable QUEUE ACCEPTANCE, distinct from later EXITOBSERVED.
// Conductor owns RPC, independent review, process-stop and exit verification.
template<bool InspectOnly,bool Checkpoint=false> static bool queue_tutorial_response() {
    bool expected=false;
    if(!fb::busy.compare_exchange_strong(expected,true)) return false;
    expected=false;
    if(!attempted.compare_exchange_strong(expected,true)) { fb::busy.store(false,std::memory_order_release); return false; }
    try {
        static_assert(!(InspectOnly && Checkpoint),"inspection cannot checkpoint");
        quarantine=new Job(InspectOnly,Checkpoint); // process-lifetime context, never delete, even prepost errors
        dispatch_async_f(dispatch_get_main_queue(),quarantine,begin);
        return true;
    } catch(...) {
        if(InspectOnly) {
            logf_("TUTORIAL_RESPONSE_INSPECT BEGIN process_attempt_consumed=1 observation_only=1");
            if(quarantine) end(*quarantine,false,"allocation_or_queue_exception");
            else logf_("TUTORIAL_RESPONSE_INSPECT END inventoryComplete=0 reason=allocation_exception post_entries=0 unknown_native_calls=0 tutorial_exit_claim=0 fbBusy_held=1 quarantine=1 immediate_process_stop_required=1 noRetry=1");
            return false;
        }
        if(Checkpoint && !quarantine) logf_("TUTORIAL_CHECKPOINT END complete=0 reason=allocation_exception saveAttempts=0 return_known=0 busy_held=1 immediate_process_stop_required=1 noRetry=1");
        logf_("TUTORIAL_FINISH BEGIN process_attempt_consumed=1");
        logf_("TUTORIAL_FINISH POSTED entered=0");
        logf_("TUTORIAL_FINISH OBSERVE scheduled=0");
        if(quarantine) end(*quarantine,false,"allocation_or_queue_exception");
        else {
            logf_("TUTORIAL_FINISH RETAINED_LEASE max_contexts=1 refs=0 fbBusy_held=1 quarantine=1 immediate_process_stop_required=1 noRetry=1");
            logf_("TUTORIAL_FINISH END complete=0 reason=allocation_exception");
        }
        return false;
    }
}
static bool finishtutorial() { return queue_tutorial_response<false>(); }
static bool inspecttutorialresponse() { return queue_tutorial_response<true>(); }
} // namespace tutorial_finish
#endif
