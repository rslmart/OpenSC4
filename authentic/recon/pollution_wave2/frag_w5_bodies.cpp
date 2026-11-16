// W5 completed bodies (transcription of the W5 handoff, verbatim).
// Concatenate with /tmp/swarm2/frag_w5.cpp's skeleton at merge time.
// HPR dependencies: IGZMessage2Seam, ISimManagerSeam/resolvedSimManager,
// SetResponseCurveDataByResource, PollutionHistoryEmit/emitHistory/
// historyWarehouse, PollutionPostMessage/PostMessageSeam (hpr_w5.md).

// ==================== DoMessage ====================
// Exact cSC4PollutionSimulator::DoMessage(cIGZMessage2*) @ 0x100096a58.
bool PollutionSimulatorSaveRecord::DoMessage(IGZMessage2Seam& message) noexcept {
    const Sint32 id = message.GetMessageId(); // vt+0x20 @ 0x100096a6e (cmp ebx, ... signed)
    if (id > 0x26d31ec0) {                    // jg @ 0x100096a79
        if (id > 0x66956813) {                // jg @ 0x100096ab1
            if (id == 0x66956816) {           // je @ 0x100096adb
                StepMonthly();                // DoMonthlySimulation @ 0x100097cf2, call @ 0x100096b13
            } else if (id == 0x66956814) {    // je @ 0x100096ae3
                DoDailySimulation();          // call @ 0x100096ae8
            }
        } else if (id == 0x26d31ec1) {        // je @ 0x100096ab9
            UpdatePollutionVisualEffect();    // jmp target 0x100096b44, call @ 0x1000986d8
        } else if (id == 0x66956810) {        // je @ 0x100096ac5
            SimulationBegin();                // call @ 0x100096ace
        }
        return true;                          // mov al,1 @ 0x100096b4c
    }
    // id <= 0x26d31ec0: unsigned band (id + 0x6610eebe) < 2 encodes
    // {0x99ef1142, 0x99ef1143} — lea/cmp/jb @ 0x100096a7b-0x100096a84.
    if (static_cast<Uint32>(static_cast<Uint32>(id) + 0x6610eebeu) < 2u) {
        OccupantSeam* occupant = message.GetOccupant(); // vt+0x50 @ 0x100096af5
        const bool added = (id == 0x99ef1142);          // sete dl @ 0x100096b00 (band member 1 only)
        DoOccupantChanged(occupant, added);              // @ 0x1000980b2, call @ 0x100096b09
        return true;
    }
    if (id == 0xc6cdbf1a) {                   // je @ 0x100096a8c
        const Uint32 value = message.GetGZStringFirstDword(); // vt+0x78 @ 0x100096b20, dword [rax] @ 0x100096b23
        const Uint32 incoming = (value == 1u) ? 1u : 0u;      // sete cl @ 0x100096b2a
        if (static_cast<Uint8>(field32C) == incoming) return true; // cmp/je @ 0x100096b35
        field32C = (value == 1u);             // sete byte @ 0x100096b3c
        UpdatePollutionVisualEffect();        // fall-through @ 0x100096b44
        return true;
    }
    if (id == 0xca1b151c) {                   // jne @ 0x100096a98
        ReadSimulatorTunableValues();         // @ 0x100095912, call @ 0x100096aa1
    }
    return true;                              // unknown ids: no-op, still true
}

// ==================== SimulationBegin ====================
// Exact cSC4PollutionSimulator::SimulationBegin() @ 0x100098006.
void PollutionSimulatorSaveRecord::SimulationBegin() noexcept {
    cSC4ResponseCurve curve; // ctor @ 0x10009801b; RAII dtor = cleanup @ 0x100098077-0x100098089
    // Resource key @ 0x100098020-0x100098040: cGZPersistResourceKey
    // {0x6534284A (= kTypeExemplar), 0xE7E2DB, 0xC911E35B} == PollutionTuning::kExemplarTGI,
    // second arg 0x6911e2df. SetResponseCurveData @ 0x1000ac47e.
    SetResponseCurveDataByResource(curve, PollutionTuning::kExemplarTGI, 0x6911e2dfu);
    // this+0x1c0 = resolved sim manager (serialized simManagerRef, IID 0x22e2b67);
    // vt+0x50 @ 0x10009804f returns Float32*. NO null guard in the binary.
    const Float32 population = *resolvedSimManager->GetPopulationFloatPtr(); // movss [rax] @ 0x100098052
    // Exact op order: (pop * 100.0f) -> GetOutputValue -> * 0.01f @ 0x100098056-0x100098067.
    const Float32 scaled = population * 100.0f;            // mulss 0x42c80000 (0x100a161f4)
    const Float32 curved = curve.GetOutputValue(scaled);  // @ 0x1000ac4f6, call @ 0x100098062
    field348 = curved * 0.009999999776482582f;             // mulss 0x3c23d70a (0x100a15928), store @ 0x10009806f
}

// ==================== EndOfCycle ====================
// Exact cSC4PollutionSimulator::EndOfCycle() @ 0x1000996f4.
void PollutionSimulatorSaveRecord::EndOfCycle() noexcept {
    CalculateRecyclingCenterEffect(); // call @ 0x100099704
    const Uint32 tractCount = field22C;      // ebx @ 0x100099709
    const Uint32 totalUncollected = field248; // edi @ 0x100099710
    field244 = (totalUncollected != 0);      // setne byte @ 0x100099719
    Uint32 avgAir, avgWater, garbagePerTract, garbageScaled;
    if (tractCount != 0) {                   // test ebx,ebx; je @ 0x100099723
        avgAir = field214 / tractCount;      // u32 div @ 0x100099735
        avgWater = field218 / tractCount;    // u32 div @ 0x10009973d
        garbagePerTract = totalUncollected / tractCount; // u32 div @ 0x100099745
        garbageScaled = (field230 * 100u) / tractCount;  // imul 0x64 @ 0x100099749, div @ 0x100099753
    } else {                                 // @ 0x10009975a
        avgAir = 0u; avgWater = 0u; garbagePerTract = 0u; garbageScaled = 0u;
    }
    field20C = avgAir;                        // @ 0x100099763
    field210 = avgWater;                       // @ 0x10009976a
    field234 = static_cast<Uint16>(garbagePerTract); // mov word @ 0x100099771 (truncating store)
    // 8-byte copy @ 0x100099779: [+0x224..0x22b] = [+0x21c..0x223].
    field224 = field21C;
    field228 = field220;
    // SL::spHistoryWarehouse @ 0x100b214b0 (ptr-to-ptr; deref once @ 0x10009978e),
    // vtable+0x60 Set(id, value) — auto-labeled Float32 arg; bits pass through edx.
    if (emitHistory != nullptr) {
        emitHistory(historyWarehouse, 0x0a1100e8u, avgAir);        // @ 0x10009979e
        emitHistory(historyWarehouse, 0xea110130u, avgWater);      // @ 0x1000997b3
        emitHistory(historyWarehouse, 0xea110134u, field234);       // movzx word @ 0x1000997b9
        emitHistory(historyWarehouse, 0xea110137u, field230);      // @ 0x1000997de
        emitHistory(historyWarehouse, 0xea110138u, avgAir + garbageScaled); // add r14d @ 0x1000997e4
    }
    // Zeroing: three qword stores @ 0x1000997f9-0x100099809.
    field22C = 0; field230 = 0;   // qword @ +0x22c
    field21C = 0; field220 = 0;   // qword @ +0x21c
    field214 = 0; field218 = 0;   // qword @ +0x214
    // Conditional posts @ 0x100099810-0x10009988b: SC4PostMessage2(id, 0, 0, 0, 0, null).
    if (field328) PostMessageSeam(0xda6dd35du, 0u, 0u, 0u, 0u, nullptr);
    if (field329) PostMessageSeam(0xfa6dd0bbu, 0u, 0u, 0u, 0u, nullptr);
    if (field32A) PostMessageSeam(0x5a6dd1aau, 0u, 0u, 0u, 0u, nullptr);
    if (field32B) PostMessageSeam(0xda6dd2cbu, 0u, 0u, 0u, 0u, nullptr);
    // DWORD store @ 0x100099890 clears all four flag bytes +0x328..+0x32b.
    field328 = false; field329 = false; field32A = false; field32B = false;
    // Curve rebuild, identical to SimulationBegin @ 0x10009989b-0x1000998f6.
    cSC4ResponseCurve curve; // ctor @ 0x1000998a2
    SetResponseCurveDataByResource(curve, PollutionTuning::kExemplarTGI, 0x6911e2dfu);
    const Float32 population = *resolvedSimManager->GetPopulationFloatPtr(); // vt+0x50 @ 0x1000998d6
    const Float32 scaled = population * 100.0f;          // 0x42c80000 @ 0x1000998dd
    const Float32 curved = curve.GetOutputValue(scaled); // call @ 0x1000998e9
    field348 = curved * 0.009999999776482582f;            // 0x3c23d70a @ 0x1000998ee, store @ 0x1000998f6
    PostMessageSeam(0xea418d8fu, 8u, 0u, 0u, 0u, nullptr); // @ 0x1000998ff-0x100099913
}

// W5 QUIRKS (reviewer gate): (1) field32C: any value != 1 sets false;
// visual update fires only on state change. (2) sete on band member 1 only:
// 0x99ef1142=true, 0x99ef1143=false. (3) EndOfCycle dword store at +0x328
// clears all four flags at once. (4) field230 zeroed by the +0x22c qword
// store AFTER history emission. (5) No null guard on the vt+0x50 population
// pointer (both sites). (6) emitHistory null-guard is a clean-room deviation
// (the binary always derefs spHistoryWarehouse) — flagged. (7) Second arg
// 0x6911e2df left unnamed. (8) field32C written as 0/1 byte.
