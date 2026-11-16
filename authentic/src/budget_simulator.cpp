#include "opensc4/budget_simulator.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace opensc4 {

namespace {

bool ReadRef(cIGZIStream& stream, GrowthDeveloperSaveRecord::GridRef& ref) noexcept {
    stream >> ref.memAddr;
    if (!stream.Good()) {
        return false;
    }
    if (ref.memAddr != 0u) {
        stream >> ref.clsid;
    } else {
        ref.clsid = 0u;
    }
    return stream.Good();
}

void WriteRef(cIGZOStream& stream, const GrowthDeveloperSaveRecord::GridRef& ref) {
    stream << ref.memAddr;
    if (ref.memAddr != 0u) {
        stream << ref.clsid;
    }
}

const ExemplarProperty* FindProp(const cGZExemplar& exemplar, Uint32 propId) {
    const auto it = exemplar.properties.find(propId);
    return it != exemplar.properties.end() ? &it->second : nullptr;
}

} // namespace

Sint16 GetTaxTypeForPurposeAndWealth(Sint32 purpose, Sint32 wealth) noexcept {
    // Exact kTaxTypeByPurposeAndWealth @ 0x100a1b320 (rows 0..8; rows beyond
    // 8 in the binary are unrelated data, and ComputeTax never indexes them
    // because purpose comes from the building holder's purpose enum).
    static constexpr Sint16 kTable[9][4] = {
        {-1, -1, -1, -1},
        {-1,  0,  1,  2},
        {-1,  3,  4,  5},
        {-1, -1,  6,  7},
        {-1,  3,  4,  5},
        {-1,  8,  8,  8},
        {-1,  9,  9,  9},
        {-1, 10, 10, 10},
        {-1, 11, 11, 11},
    };
    if (purpose < 0 || purpose > 8 || wealth < 0 || wealth > 3) {
        return -1;
    }
    return kTable[purpose][wealth];
}

bool BudgetSimulatorTuning::LoadFromExemplar(const cGZExemplar& exemplar) {
    // Exact ReadSimulatorTunableValues @ 0x10021ea9e. The tuning exemplar is
    // {0x6534284a, 0xe7e2c2db, 0x0bff6824}; defaults are set by the caller/ctor
    // before this runs, mirroring the binary's pre-initialized fields.
    if (const auto* prop = FindProp(exemplar, 0x6a9c7718u)) {
        sint64List7E0 = prop->intValues;
    }
    if (FindProp(exemplar, 0x890e0a99u) != nullptr) {
        field7F8 = exemplar.GetFloat(0x890e0a99u, field7F8);
    }
    if (FindProp(exemplar, 0x090e0a9fu) != nullptr) {
        minAllowableFunds = exemplar.GetInt(0x090e0a9fu, minAllowableFunds);
    }
    if (FindProp(exemplar, 0x490e0aa6u) != nullptr) {
        field808 = exemplar.GetInt(0x490e0aa6u, field808);
    }
    if (FindProp(exemplar, 0x090e0aacu) != nullptr) {
        field810 = static_cast<Sint32>(exemplar.GetInt(0x090e0aacu, field810));
    }
    if (FindProp(exemplar, 0x890e0ab4u) != nullptr) {
        field818 = exemplar.GetInt(0x890e0ab4u, field818);
    }
    if (FindProp(exemplar, 0x8a3fb98du) != nullptr) {
        // Binary multiplies the loaded float by 0.01f (@ 0x100a15928).
        bondInterestRate = exemplar.GetFloat(0x8a3fb98du, bondInterestRate) * 0.01f;
    }
    if (FindProp(exemplar, 0x0a3fb9a8u) != nullptr) {
        bondTermMonths = static_cast<Uint32>(exemplar.GetInt(0x0a3fb9a8u, bondTermMonths));
    }
    if (FindProp(exemplar, 0x89170c6cu) != nullptr) {
        lvToResTaxTransmogrifier = exemplar.GetFloat(0x89170c6cu, lvToResTaxTransmogrifier);
    }
    if (FindProp(exemplar, 0x09170c74u) != nullptr) {
        lvToComTaxTransmogrifier = exemplar.GetFloat(0x09170c74u, lvToComTaxTransmogrifier);
    }
    if (FindProp(exemplar, 0xc9170c78u) != nullptr) {
        lvToIndTaxTransmogrifier = exemplar.GetFloat(0xc9170c78u, lvToIndTaxTransmogrifier);
    }
    if (FindProp(exemplar, 0x49170c7cu) != nullptr) {
        minAverageLandValue = static_cast<Uint8>(exemplar.GetInt(0x49170c7cu, minAverageLandValue));
    }
    // Per-type multipliers in binary write order (see header note on +0x844).
    struct PropToSlot { Uint32 propId; std::size_t slot; };
    static constexpr PropToSlot kMultiplierProps[] = {
        {0xa9188ef5u, 0}, {0xe918c920u, 1}, {0x4918c925u, 2}, {0xa918c929u, 3},
        {0x4918c92cu, 4}, {0x4918c930u, 7}, {0x8918c937u, 6}, {0x4918c93au, 7},
        {0x69a36e40u, 8}, {0xa918c942u, 9}, {0x8918c93eu, 10}, {0xa9a36e47u, 11},
    };
    for (const auto& entry : kMultiplierProps) {
        if (FindProp(exemplar, entry.propId) != nullptr) {
            taxTypeMultipliers[entry.slot] = exemplar.GetFloat(entry.propId, taxTypeMultipliers[entry.slot]);
        }
    }
    if (const auto* prop = FindProp(exemplar, 0x8a480df7u)) {
        wealthTaxFactors = prop->floatValues;
    }
    if (const auto* prop = FindProp(exemplar, 0xebff69e7u)) {
        taxCapFactors = prop->floatValues;
    }
    if (const auto* prop = FindProp(exemplar, 0x0c032488u)) {
        floatList8A8 = prop->floatValues;
    }
    if (const auto* prop = FindProp(exemplar, 0x4ac519b8u)) {
        landValueResponseCurve.SetResponseCurveData(*prop);
    }
    return true;
}

bool BudgetSimulatorSaveRecord::Read(cIGZIStream& stream) noexcept {
    // Exact cSC4BudgetSimulator::Read @ 0x10021f918.
    departments.clear();
    bonds.clear();
    groups.clear();
    if (!stream.Good()) {
        return false;
    }
    stream >> version;
    if (version < 4u) {
        return false;
    }
    stream >> totalFunds;
    stream >> fundsMirror;
    stream >> field58;
    stream >> lastTaxIncome;
    Uint32 departmentCount = 0;
    stream >> departmentCount;
    for (Uint32 i = 0; i < departmentCount; ++i) {
        BudgetDepartmentRef dept{};
        stream >> dept.departmentId;
        if (!ReadRef(stream, dept.budget)) {
            return false;
        }
        departments.push_back(dept);
    }
    for (auto& category : taxCategories) {
        stream >> category.taxRate;
        stream >> category.secondaryRate;
        for (auto& value : category.monthlyHistory) {
            stream >> value;
        }
        stream >> category.field68;
        stream >> category.weightedAssessedValue;
        stream >> category.totalTaxIncome;
    }
    stream >> totalMonthlyExpenseCity;
    stream >> totalOtherIncome;
    for (auto& value : monthlyExpenseByMonth) {
        stream >> value;
    }
    for (auto& value : monthlyOtherIncomeByMonth) {
        stream >> value;
    }
    for (auto& value : yearlyTotals) {
        stream >> value;
    }
    Uint32 bondCount = 0;
    stream >> bondCount;
    for (Uint32 i = 0; i < bondCount; ++i) {
        BudgetBond bond{};
        stream >> bond.maturityMonth;
        stream >> bond.maturityYear;
        stream >> bond.principal;
        stream >> bond.field20;
        stream >> bond.remainingPrincipal;
        stream >> bond.totalPaid;
        stream >> bond.monthlyPayment;
        stream >> bond.balance;
        if (!stream.Good()) {
            return false;
        }
        bonds.push_back(bond);
    }
    stream >> totalBondPayments;
    stream >> lastMonthlyIncome;
    stream >> field8C8;
    stream >> lastMonthlyExpense;
    stream >> field8D8;
    Uint32 groupCount = 0;
    stream >> groupCount;
    for (Uint32 i = 0; i < groupCount; ++i) {
        BudgetGroup group{};
        stream >> group.groupId;
        stream >> group.field28;
        if (!stream.ReadGZString(group.name)) {
            return false;
        }
        stream >> group.field58;
        stream >> group.field60;
        Uint32 deptCount = 0;
        stream >> deptCount;
        for (Uint32 d = 0; d < deptCount; ++d) {
            Uint32 deptId = 0;
            stream >> deptId;
            group.departmentIds.push_back(deptId);
        }
        if (!stream.Good()) {
            return false;
        }
        groups.push_back(std::move(group));
    }
    return stream.Good();
}

bool BudgetSimulatorSaveRecord::Write(cIGZOStream& stream) const {
    // Exact cSC4BudgetSimulator::Write @ 0x10021f4a6 (writes version 5).
    stream << static_cast<Uint16>(5u);
    stream << totalFunds;
    stream << fundsMirror;
    stream << field58;
    stream << lastTaxIncome;
    stream << static_cast<Uint32>(departments.size());
    for (const auto& dept : departments) {
        stream << dept.departmentId;
        WriteRef(stream, dept.budget);
    }
    for (const auto& category : taxCategories) {
        stream << category.taxRate;
        stream << category.secondaryRate;
        for (const auto value : category.monthlyHistory) {
            stream << value;
        }
        stream << category.field68;
        stream << category.weightedAssessedValue;
        stream << category.totalTaxIncome;
    }
    stream << totalMonthlyExpenseCity;
    stream << totalOtherIncome;
    for (const auto value : monthlyExpenseByMonth) {
        stream << value;
    }
    for (const auto value : monthlyOtherIncomeByMonth) {
        stream << value;
    }
    for (const auto value : yearlyTotals) {
        stream << value;
    }
    stream << static_cast<Uint32>(bonds.size());
    for (const auto& bond : bonds) {
        stream << bond.maturityMonth;
        stream << bond.maturityYear;
        stream << bond.principal;
        stream << bond.field20;
        stream << bond.remainingPrincipal;
        stream << bond.totalPaid;
        stream << bond.monthlyPayment;
        stream << bond.balance;
    }
    stream << totalBondPayments;
    stream << lastMonthlyIncome;
    stream << field8C8;
    stream << lastMonthlyExpense;
    stream << field8D8;
    stream << static_cast<Uint32>(groups.size());
    for (const auto& group : groups) {
        stream << group.groupId;
        stream << group.field28;
        stream.WriteGZString(group.name);
        stream << group.field58;
        stream << group.field60;
        stream << static_cast<Uint32>(group.departmentIds.size());
        for (const auto deptId : group.departmentIds) {
            stream << deptId;
        }
    }
    return true;
}

bool BudgetSimulatorSaveRecord::ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept {
    // DBPF subfile wrapper: [recordSize:Uint32][flags:Uint32][memAddr:Uint32][chunk].
    if (recordBytes.size() < 14u) {
        return false;
    }
    Uint32 recSize = 0;
    std::memcpy(&recSize, recordBytes.data(), sizeof(Uint32));
    if (recSize < 14u || recSize > recordBytes.size()) {
        return false;
    }
    std::memcpy(&memAddr, recordBytes.data() + 8u, sizeof(Uint32));
    cIGZIStream stream(recordBytes.data() + 12u, recSize - 12u);
    return Read(stream);
}

Float32 BudgetSimulatorSaveRecord::GetTaxRate(Sint32 taxType) const noexcept {
    // Exact GetTaxRate @ 0x100221f94: out-of-range types return 0.
    if (taxType < 0 || taxType > 11) {
        return 0.0f;
    }
    return taxCategories[static_cast<std::size_t>(taxType)].taxRate;
}

void BudgetSimulatorSaveRecord::SetTaxRate(Sint32 taxType, Float32 rate) noexcept {
    // Exact SetTaxRate @ 0x100221fb2: out-of-range types are ignored.
    if (taxType < 0 || taxType > 11) {
        return;
    }
    taxCategories[static_cast<std::size_t>(taxType)].taxRate = rate;
}

Sint64 BudgetSimulatorSaveRecord::GetWeightedAssessedTaxValue(Sint32 taxType) const noexcept {
    // Exact GetWeightedAssessedTaxValue @ 0x1002222c4.
    if (taxType < 0 || taxType > 11) {
        return 0;
    }
    return taxCategories[static_cast<std::size_t>(taxType)].weightedAssessedValue;
}

bool BudgetSimulatorSaveRecord::DepositFunds(Sint64 amount) noexcept {
    // Exact cSC4BudgetSimulator::DepositFunds @ 0x10022134e:
    // if (amount < 0) return WithdrawFunds(-amount);
    // clamps against Sint64 max overflow.
    if (amount < 0) {
        return WithdrawFunds(-amount);
    }
    if (amount == 0) {
        return true;
    }
    constexpr Sint64 kMaxFunds = std::numeric_limits<Sint64>::max();
    if (totalFunds > 0 && kMaxFunds - totalFunds < amount) {
        totalFunds = kMaxFunds;
    } else {
        totalFunds += amount;
    }
    fundsMirror = totalFunds;
    return true;
}

bool BudgetSimulatorSaveRecord::WithdrawFunds(
    Sint64 amount, Sint64 minAllowableFunds) noexcept {
    // Exact cSC4BudgetSimulator::WithdrawFunds @ 0x10022139e:
    // if (amount < 0) return DepositFunds(-amount);
    // refuses withdrawal if totalFunds - amount < minAllowableFunds.
    if (amount < 0) {
        return DepositFunds(-amount);
    }
    if (amount == 0) {
        return true;
    }
    const Sint64 nextFunds = totalFunds - amount;
    if (nextFunds < minAllowableFunds) {
        return false;
    }
    totalFunds = nextFunds;
    fundsMirror = nextFunds;
    return true;
}

Sint64 BudgetSimulatorSaveRecord::GetTotalMonthlyBondPayments() const noexcept {
    // Exact cSC4BudgetSimulator::GetTotalMonthlyBondPayments @ 0x10022245e.
    Sint64 total = 0;
    for (const auto& bond : bonds) {
        total += bond.monthlyPayment;
    }
    return total;
}

bool BudgetSimulatorSaveRecord::IssueBond(
    Uint32 requestedAmount, const BudgetSimulatorTuning& tuning,
    Uint32 currentMonth, Uint32 currentYear) {
    // Exact cSC4BudgetSimulator::IssueBond @ 0x100221fdc (corrected Nov 2026
    // after the blind-agent benchmark flagged divergences; all settled with
    // fresh bytes):
    //   - The denomination [this+0x808] is used RAW (idiv rcx) — the earlier
    //     max<Sint64>(1, ...) clamp was invented and is removed.
    //   - cmove zero-round-up @ 0x100222018: when the requested amount rounds
    //     down to a zero bond ((amt/denom)*denom == 0, tested via the low 32
    //     bits), the bond value becomes THE DENOMINATION itself — the game
    //     issues a minimum bond; it never rejects a small amount. The earlier
    //     `principal <= 0 -> false` reject was invented and is removed.
    //   - Bond value gate @ 0x10022201f: only `principal > [0x818] -> false`
    //     (signed jg).
    //   - Borrowing limit @ 0x100222028..0x10022203e: the check is
    //     `debt + bond <= GetCurrentBorrowingLimit()` — i.e. evaluated AFTER
    //     adding the new bond (cmp r15, rax; jle continue). The limit itself
    //     is byte-confirmed: cSC4BudgetSimulator::GetCurrentBorrowingLimit
    //     @ 0x100222292 = movsxd [0x810] * [0x818]. The earlier pre-check
    //     `debt >= limit` accepted oversize bonds at debt near the limit.
    if (requestedAmount == 0u) {
        return false;
    }
    if (static_cast<Sint32>(bonds.size()) >= tuning.field810) {
        return false;
    }
    const Sint64 denomination = tuning.field808;  // raw idiv operand
    Sint64 principal = (static_cast<Sint64>(requestedAmount) / denomination) *
                       denomination;
    if (principal == 0) {
        principal = denomination;  // cmove rax, rcx @ 0x100222018
    }
    if (principal > tuning.field818) {
        return false;
    }
    const Sint64 borrowingLimit =
        static_cast<Sint64>(static_cast<Sint32>(tuning.field810)) *
        tuning.field818;  // GetCurrentBorrowingLimit @ 0x100222292
    if (field58 + principal > borrowingLimit) {
        return false;  // post-add compare @ 0x10022203b
    }
    const Sint64 monthlyPayment = GetMonthlyPaymentForLoan(tuning, principal);
    const Sint64 fullCost =
        monthlyPayment * static_cast<Sint64>(tuning.bondTermMonths);
    Uint32 matMonth = currentMonth + (tuning.bondTermMonths % 12u);
    Uint32 matYear = currentYear + (tuning.bondTermMonths / 12u);
    if (matMonth > 12u) {
        matMonth -= 12u;
        ++matYear;
    }
    BudgetBond bond{};
    bond.maturityMonth = matMonth;
    bond.maturityYear = matYear;
    bond.principal = principal;
    bond.field20 = fullCost;
    bond.remainingPrincipal = fullCost;
    bond.totalPaid = 0;
    bond.monthlyPayment = monthlyPayment;
    bond.balance = principal;
    bonds.push_back(bond);
    field58 += principal;
    DepositFunds(principal);
    return true;
}

Sint64 ComputeBudgetTax(
    BudgetSimulatorSaveRecord& record,
    const BudgetSimulatorTuning& tuning,
    std::span<const BudgetTaxLotInput> lots) {
    // Exact ComputeTax @ 0x100220c22.
    std::array<Float64, 12> categoryIncome{};
    for (auto& category : record.taxCategories) {
        category.weightedAssessedValue = 0;
        category.totalTaxIncome = 0;
    }
    Float64 totalIncome = 0.0;
    for (const auto& lot : lots) {
        const Sint16 taxType = GetTaxTypeForPurposeAndWealth(lot.purpose, lot.wealth);
        if (taxType < 0) {
            continue;
        }
        const auto typeIdx = static_cast<std::size_t>(taxType);
        const Float32 curveValue = tuning.landValueResponseCurve.GetOutputValue(
            static_cast<Float32>(lot.landValueClass));
        const Float32 scaled = curveValue * static_cast<Float32>(lot.sizeX) *
            static_cast<Float32>(lot.sizeZ) + 12582912.0f;
        Uint32 bits = 0;
        std::memcpy(&bits, &scaled, sizeof(bits));
        const Sint64 landComponent = static_cast<Sint64>(bits + 0xb4c00000u);
        const Sint64 assessed = landComponent + lot.assessedValueProperty;
        // Binary order: weighted = (Float32)assessed * multiplier, then
        // weighted *= wealthTaxFactors[holderCategory - wealth].
        Float32 weighted = static_cast<Float32>(assessed) * tuning.taxTypeMultipliers[typeIdx];
        const Sint32 factorIdx = lot.holderCategory - lot.wealth;
        if (factorIdx >= 0 && static_cast<std::size_t>(factorIdx) < tuning.wealthTaxFactors.size()) {
            weighted *= tuning.wealthTaxFactors[static_cast<std::size_t>(factorIdx)];
        }
        const Sint64 taxInt = static_cast<Sint64>(weighted);
        Float32 capFactor = 0.0f;
        if (lot.simulatorTypeIndex >= 0 &&
            static_cast<std::size_t>(lot.simulatorTypeIndex) < tuning.taxCapFactors.size()) {
            capFactor = tuning.taxCapFactors[static_cast<std::size_t>(lot.simulatorTypeIndex)];
        }
        const Float32 income = static_cast<Float32>(taxInt) *
            (record.GetTaxRate(taxType) * capFactor) / 1200.0f;
        record.taxCategories[typeIdx].weightedAssessedValue += taxInt;
        const Float64 income64 = static_cast<Float64>(income);
        categoryIncome[typeIdx] += income64;
        totalIncome += income64;
    }
    for (std::size_t i = 0; i < record.taxCategories.size(); ++i) {
        record.taxCategories[i].totalTaxIncome = static_cast<Sint64>(categoryIncome[i]);
    }
    return static_cast<Sint64>(totalIncome);
}

Sint64 GetMonthlyPaymentForLoan(const BudgetSimulatorTuning& tuning, Sint64 principal) noexcept {
    // Exact GetMonthlyPaymentForLoan @ 0x1002223d2.
    const Float64 rate = static_cast<Float64>(tuning.bondInterestRate) / 12.0;
    if (rate == 0.0) {
        return tuning.bondTermMonths != 0u
            ? principal / static_cast<Sint64>(tuning.bondTermMonths) : 0;
    }
    const Float64 discounted = std::pow(1.0 + rate, -static_cast<Float64>(tuning.bondTermMonths));
    return static_cast<Sint64>(static_cast<Float64>(principal) * rate / (1.0 - discounted));
}

bool AdvanceBudgetBondOneMonth(
    BudgetBond& bond,
    const BudgetSimulatorTuning& tuning,
    Sint32 currentMonth,
    Sint32 currentYear,
    Sint64& inOutTotalBondPayments) noexcept {
    // DoMonthlySimulation bond loop @ 0x10022088d.
    const Sint32 remainingMonths =
        (static_cast<Sint32>(bond.maturityYear) - currentYear) * 12 +
        (static_cast<Sint32>(bond.maturityMonth) - currentMonth);
    if (remainingMonths <= 0) {
        return true; // matures: caller subtracts principal and removes the bond
    }
    const Sint64 payment = bond.monthlyPayment;
    inOutTotalBondPayments += payment;
    bond.remainingPrincipal -= payment;
    bond.totalPaid += payment;
    const Sint64 interest = static_cast<Sint64>(
        static_cast<Float32>(bond.balance) * (tuning.bondInterestRate / 12.0f));
    const Sint64 newBalance = interest - payment + bond.balance;
    bond.balance = newBalance;
    if (remainingMonths == 1) {
        bond.monthlyPayment = newBalance;
    }
    return false;
}

bool ApplyMonthlyBudgetStep(
    BudgetSimulatorSaveRecord& record,
    const BudgetSimulatorTuning& tuning,
    const BudgetMonthlyInputs& inputs) noexcept {
    // Exact DoMonthlySimulation @ 0x10022061e (message posting omitted; the
    // low-funds predicate is returned instead).
    if (record.totalFunds != record.fundsMirror) {
        record.totalFunds = record.fundsMirror;
    }
    const Sint32 monthIdx = (inputs.month == 1) ? 11 : (inputs.month - 2);
    const Sint32 bondYear = (inputs.month == 1) ? (inputs.year - 1) : inputs.year;
    const Sint32 bondMonth = (inputs.month == 1) ? 12 : (inputs.month - 1);
    const std::size_t mi = static_cast<std::size_t>(std::clamp(monthIdx, 0, 11));

    record.yearlyTotals[0] += inputs.taxIncome;
    record.lastTaxIncome = inputs.taxIncome;
    record.monthlyOtherIncomeByMonth[mi] = inputs.otherIncome;
    record.totalOtherIncome += inputs.otherIncome;
    // Income total includes the department-income sum: the binary's
    // department list walk adds each vt+0x80 return into the income
    // accumulator (add r15, rax @ 0x100220742), after taxIncome
    // (ComputeTax @ 0x100220698), otherIncome (vt+0xb8 @ 0x1002206b5), and
    // neighborIncome (spNeighborsSim vt+0x100 @ 0x1002206e5).
    const Sint64 incomeTotal = inputs.taxIncome + inputs.otherIncome +
                               inputs.neighborIncome + inputs.departmentIncomes;

    record.monthlyExpenseByMonth[mi] = inputs.cityExpense;
    record.totalMonthlyExpenseCity += inputs.cityExpense;
    const Sint64 expenseTotal = inputs.cityExpense + inputs.neighborExpense + inputs.departmentExpenses;

    // Bond payments / maturities.
    for (auto it = record.bonds.begin(); it != record.bonds.end();) {
        if (AdvanceBudgetBondOneMonth(*it, tuning, bondMonth, bondYear, record.totalBondPayments)) {
            record.field58 -= it->principal;
            it = record.bonds.erase(it);
        } else {
            ++it;
        }
    }

    record.totalFunds = record.totalFunds + incomeTotal - expenseTotal;
    record.yearToDateIncome += incomeTotal;
    record.yearToDateExpense += expenseTotal;
    record.lastMonthlyIncome = incomeTotal;
    record.lastMonthlyExpense = expenseTotal;
    record.fundsMirror = record.totalFunds;

    if (monthIdx == 11) {
        // December year-end reset @ 0x100220a3a.
        record.totalOtherIncome = 0;
        record.yearToDateIncome = 0;
        record.yearToDateExpense = 0;
        record.yearlyTotals.fill(0);
        record.totalMonthlyExpenseCity = 0;
        record.yearlyTotals[4] = 0;
        record.totalBondPayments = 0;
    }
    return record.totalFunds <= tuning.minAllowableFunds;
}

} // namespace opensc4
