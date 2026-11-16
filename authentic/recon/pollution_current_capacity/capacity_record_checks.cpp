// CAPACITY_RECORD_CHECKS_BEGIN
// Record-boundary checks for GetCurrentCapacity @ 0x1000992cc.
struct CapacityRecordFixture final : OccupantSeam, OccupantPropertyHolderSeam,
    PollutionCapacityAccessSeam, RawCurvePropertySeam, RawCurveValueSeam {
    mutable PollutionSimulatorSaveRecord record{};
    mutable Uint32 out = 77;
    mutable std::string events;
    mutable bool identities = true;
    mutable unsigned holderCalls = 0;
    Uint32 capacity = 9;
    bool hasCapacity = true, powered = true, hasCurve = false, hasInput = true;
    Float32 input = 0;
    std::vector<Float32> scalars{0, 1};
    bool powerChangesScale = false;
    bool inputMutatesOut = false, dataThrows = false;
    CapacityRecordFixture() { record.capacityAccess = this; record.field348 = 1; }
    const OccupantPropertyHolderSeam* GetPropertyHolder() const override {
        events += 'H'; ++holderCalls; return this;
    }
    bool GetPropertyValue(Uint32, Sint64&) const override {
        events += 'X'; identities = false; return false;
    }
    bool GetPropertyValue(Uint32 id, Float32& value) const override {
        events += 'I'; identities &= id == 0xc910b4cfu && value == 0;
        if (inputMutatesOut) out = 7;
        if (hasInput) value = input;
        return hasInput;
    }
    const CurvePropertyData* GetProperty(Uint32) const override {
        events += 'X'; identities = false; return nullptr;
    }
    bool ReadCapacity(const OccupantPropertyHolderSeam& holder, Uint32 id,
                      Uint32& value) const override {
        events += 'R'; identities &= &holder == static_cast<const OccupantPropertyHolderSeam*>(this)
            && id == 0x1234u && &value == &out;
        value = capacity; return hasCapacity;
    }
    bool IsOccupantPowered(const OccupantSeam& occupant) const override {
        events += 'P'; identities &= &occupant == static_cast<const OccupantSeam*>(this)
            && out == capacity;
        if (powerChangesScale) record.field348 = 0.5f;
        return powered;
    }
    const RawCurvePropertySeam* LookupCurve(const OccupantPropertyHolderSeam& holder,
                                          Uint32 id) const override {
        events += 'L'; identities &= &holder == static_cast<const OccupantPropertyHolderSeam*>(this)
            && id == 0x8a1e07eeu;
        return hasCurve ? this : nullptr;
    }
    const RawCurveValueSeam* GetValue() const override { events += 'V'; return this; }
    const Float32* GetScalarData() const override {
        events += 'D'; if (dataThrows) throw 42;
        return scalars.data();
    }
    Uint32 GetScalarCount() const override {
        events += 'N'; return static_cast<Uint32>(scalars.size());
    }
    bool Run() const { return record.GetCurrentCapacity(0x1234u, *this, out); }
};

void CheckPollutionCapacityRecordWrappers() {
    {
        CapacityRecordFixture f; f.hasCapacity = false;
        Check(!f.Run() && f.out == 0 && f.events == "HR" && f.identities,
              "capacity record missing uint32 property clears output and skips power @ 0x100099322");
    }
    {
        CapacityRecordFixture f; f.powerChangesScale = true;
        Check(f.Run() && f.out == 4 && f.events == "HRPHL" && f.identities && f.holderCalls == 2,
              "capacity record rereads holder and dynamic scale after power @ 0x10009933b");
    }
    {
        CapacityRecordFixture f; f.powered = false; f.record.field348 = 3;
        Check(f.Run() && f.out == 0 && f.events == "HRPHL" && f.identities,
              "capacity record unpowered zero still dispatches curve lookup @ 0x100099331");
    }
    {
        CapacityRecordFixture f; f.powered = false; f.record.field348 = std::numeric_limits<Float32>::infinity();
        Check(f.Run() && f.out == 0 && f.events == "HRPHL",
              "capacity record unpowered zero times infinity truncates low32 @ 0x100099344");
    }
    {
        CapacityRecordFixture f; f.capacity = 0x80000001u;
        Check(f.Run() && f.out == 0x80000000u && f.identities,
              "capacity record highbit capacity uses unsigned float conversion @ 0x10009931b");
    }
    {
        CapacityRecordFixture f; f.capacity = 0xffffffffu;
        Check(f.Run() && f.out == 0,
              "capacity record rounded uint32 max truncates through 64bit low32 @ 0x100099349");
    }
    {
        CapacityRecordFixture f; f.capacity = 0x80000001u; f.hasCurve = true;
        f.scalars = {0, 0x1p-30f};
        Check(f.Run() && f.out == 2 && f.identities,
              "capacity record curve multiplier uses unsigned saved highbit capacity @ 0x1000993b3");
    }
    {
        CapacityRecordFixture f; f.record.field348 = -0.5f;
        Check(f.Run() && f.out == 0xfffffffcu,
              "capacity record negative fractional scale truncates toward zero @ 0x100099344");
    }
    {
        CapacityRecordFixture f; f.hasCurve = true; f.scalars = {0, 0.5f, 999};
        Check(f.Run() && f.out == 4 && f.events == "HRPHLVDNI" && f.identities,
              "capacity record raw value data count dispatch floors odd tail and magic rounds tie @ 0x1000ac37a");
    }
    {
        CapacityRecordFixture f; f.hasCurve = true; f.hasInput = false;
        f.scalars = {0, 0.5f, 1, 1}; f.input = 1;
        Check(f.Run() && f.out == 4 && f.events == "HRPHLVDNI" && f.identities,
              "capacity record ignored missing float input retains initialized zero @ 0x10009938d");
    }
    {
        CapacityRecordFixture f; f.hasCurve = true; f.inputMutatesOut = true;
        f.scalars = {0, 1.0f};
        Check(f.Run() && f.out == 7 && f.identities,
              "capacity record input callback output mutation precedes capacity snapshot @ 0x1000993a0");
    }
    {
        CapacityRecordFixture f; f.hasCurve = true; f.scalars.clear();
        bool fault = false;
        try { (void)f.Run(); } catch (const ResponseCurveStorageFault&) { fault = true; }
        Check(fault && f.out == 9 && f.events == "HRPHLVDNI",
              "capacity record present empty curve faults distinctly without overwriting scaled out @ 0x1000ac542");
    }
    {
        CapacityRecordFixture f; f.hasCurve = true; f.scalars = {123};
        bool fault = false;
        try { (void)f.Run(); } catch (const ResponseCurveStorageFault&) { fault = true; }
        Check(fault && f.out == 9 && f.events == "HRPHLVDNI",
              "capacity record single odd scalar still clears fresh curve then faults @ 0x1000ac3cb");
    }
    {
        CapacityRecordFixture f; f.hasCurve = true; f.scalars = {0, 0};
        Check(f.Run() && f.out == 0 && f.events == "HRPHLVDNI",
              "capacity record valid zero response is success rather than storage fault @ 0x1000993cd");
    }
    {
        CapacityRecordFixture f; f.hasCurve = true; f.dataThrows = true;
        bool propagated = false;
        try { (void)f.Run(); } catch (int value) { propagated = value == 42; }
        f.dataThrows = false;
        const bool recovered = f.Run();
        Check(propagated && recovered && f.out == 9 && f.events == "HRPHLVDHRPHLVDNI",
              "capacity record raw dispatch exception unwinds curve and next invocation recovers @ 0x1000993f6");
    }
    {
        // Runtime seam must neither enter Write bytes nor be overwritten by Read.
        CapacityRecordFixture f;
        cIGZOStream withoutSeam, withSeam, reread;
        f.record.capacityAccess = nullptr;
        const bool wroteWithout = f.record.Write(withoutSeam);
        f.record.capacityAccess = &f;
        const bool wroteWith = f.record.Write(withSeam);
        cIGZIStream input(withSeam.Buffer().data(), withSeam.Buffer().size());
        const bool read = f.record.Read(input);
        Check(wroteWithout && wroteWith && read && f.record.Write(reread) &&
              f.record.capacityAccess == &f && withoutSeam.Buffer() == withSeam.Buffer() &&
              reread.Buffer() == withSeam.Buffer() && f.Run() && f.out == 9 && f.identities,
              "capacity record runtime source identity survives byte identical Read Write");
    }
}
// CAPACITY_RECORD_CHECKS_END
