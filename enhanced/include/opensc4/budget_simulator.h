#pragma once

#include "opensc4/gzcom.h"
#include "opensc4/tract_developer.h"

#include <array>
#include <span>
#include <string>
#include <vector>

namespace opensc4 {

// ============================================================================
// cSC4BudgetSimulator (CLSID 0xe990be01, object size 0x8e8):
// Exact decompilation of GZCOM_CreatecSC4BudgetSimulator @ 0x10021dd20,
// cSC4BudgetSimulator ctor @ 0x10021dd6e, ReadSimulatorTunableValues
// @ 0x10021ea9e, Write @ 0x10021f4a6, Read @ 0x10021f918,
// ComputeTax @ 0x100220c22, DoMonthlySimulation @ 0x10022061e,
// GetMonthlyPaymentForLoan @ 0x1002223d2, GetTaxRate @ 0x100221f94, and
// GetWeightedAssessedTaxValue @ 0x1002222c4.
// ============================================================================

inline constexpr cSC4TGI kBudgetSimulatorTuningExemplarTGI{
    kTypeExemplar, 0xe7e2c2dbu, 0x0bff6824u
};

// Exact kTaxTypeByPurposeAndWealth @ 0x100a1b320: [purpose 0..8][wealth 0..3]
// as Sint16, row stride 4. -1 = not taxable.
Sint16 GetTaxTypeForPurposeAndWealth(Sint32 purpose, Sint32 wealth) noexcept;

// Exact ReadSimulatorTunableValues @ 0x10021ea9e defaults (ctor @ 0x10021dd6e
// zeroes the fields; the constants below are written before the exemplar is
// consulted, and any property present in the exemplar overwrites them).
struct BudgetSimulatorTuning {
    std::vector<Sint64> sint64List7E0;              // 0x6a9c7718 -> +0x7e0
    Float32 field7F8 = 25.0f;                       // 0x890e0a99 -> +0x7f8
    Sint64 minAllowableFunds = -100000;             // 0x090e0a9f -> +0x800
    Sint64 field808 = 1000;                         // 0x490e0aa6 -> +0x808
    Sint32 field810 = 5;                            // 0x090e0aac -> +0x810
    Sint64 field818 = 10000;                        // 0x890e0ab4 -> +0x818
    Float32 bondInterestRate = 0.085f;              // 0x8a3fb98d -> +0x820 (x0.01 after load)
    Uint32 bondTermMonths = 120u;                   // 0x0a3fb9a8 -> +0x824
    // Statics (mfLVTo*TaxTransmogrifier, mnMinAverageLV):
    Float32 lvToResTaxTransmogrifier = 0.0f;        // 0x89170c6c
    Float32 lvToComTaxTransmogrifier = 0.0f;        // 0x09170c74
    Float32 lvToIndTaxTransmogrifier = 0.0f;        // 0xc9170c78
    Uint8 minAverageLandValue = 0u;                 // 0x49170c7c
    // Per-tax-type multipliers @ +0x828..+0x854. NOTE: the binary writes
    // property 0x4918c930 to +0x844 and then overwrites it with 0x4918c93a;
    // offset +0x83c (index 5) is never written and stays 0. Replicated exactly.
    std::array<Float32, 12> taxTypeMultipliers{};   // 0xa9188ef5, 0xe918c920, 0x4918c925,
                                                    // 0xa918c929, 0x4918c92c, (0x83c unset),
                                                    // 0x8918c937, 0x4918c93a, 0x69a36e40,
                                                    // 0xa918c942, 0x8918c93e, 0xa9a36e47
    std::vector<Float32> wealthTaxFactors{1.0f, 1.0f, 1.0f};     // 0x8a480df7 -> +0x858
    std::vector<Float32> taxCapFactors{100000.0f, 100000.0f, 100000.0f}; // 0xebff69e7 -> +0x890
    std::vector<Float32> floatList8A8;              // 0x0c032488 -> +0x8a8
    cSC4ResponseCurve landValueResponseCurve;       // +0x870 (prop 0x4ac519b8)

    bool LoadFromExemplar(const cGZExemplar& exemplar);
};

// One tax category: 0x80 bytes at +0xa8 + index*0x80.
struct BudgetTaxCategory {
    Float32 taxRate = 0.0f;                         // +0x00 (GetTaxRate/SetTaxRate)
    Float32 secondaryRate = 0.0f;                   // +0x04
    std::array<Sint64, 12> monthlyHistory{};        // +0x08..+0x67
    Sint64 field68 = 0;                             // +0x68
    Sint64 weightedAssessedValue = 0;               // +0x70 (GetWeightedAssessedTaxValue)
    Sint64 totalTaxIncome = 0;                      // +0x78 (truncated double total)
};

// One bond: serialized after the bond count at +0x7c8 (list at +0x7b8).
struct BudgetBond {
    Uint32 maturityMonth = 0;                       // +0x10
    Uint32 maturityYear = 0;                        // +0x14
    Sint64 principal = 0;                           // +0x18
    Sint64 field20 = 0;                             // +0x20
    Sint64 remainingPrincipal = 0;                  // +0x28
    Sint64 totalPaid = 0;                           // +0x30
    Sint64 monthlyPayment = 0;                      // +0x38
    Sint64 balance = 0;                             // +0x40
};

struct BudgetGroup {
    Uint32 groupId = 0;
    Uint32 field28 = 0;
    std::string name;
    Sint64 field58 = 0;
    Sint64 field60 = 0;
    std::vector<Uint32> departmentIds;              // count serialized at +0x78
};

struct BudgetDepartmentRef {
    Uint32 departmentId = 0;
    GrowthDeveloperSaveRecord::GridRef budget{};    // cSC4DepartmentBudget (IID 0xe990bffc)
};

// Exact serialized record of cSC4BudgetSimulator (CLSID 0xe990be01):
// Write @ 0x10021f4a6 (version 5), Read @ 0x10021f918 (requires version >= 4).
// Complete method index:
//   ~cSC4BudgetSimulator() @ 0x10021e0cc / 0x10021e1b0 / 0x10021e1ba,
//   QI @ 0x10021e1e8, AddRef @ 0x10021e24e, Release @ 0x10021e278,
//   Init @ 0x10021e330, ReadSimulatorTunableValues @ 0x10021ea9e,
//   sExchangeFunds @ 0x10021f010, sDepositFunds @ 0x10021f186,
//   Shutdown @ 0x10021f1fc, GetSimulatorType @ 0x1002201c4,
//   DoMessage @ 0x1002201fc, DoMonthlySimulation @ 0x10022061e,
//   PostFundsChangedMessage @ 0x100220b8e, ComputeTax @ 0x100220c22,
//   PostBondMessage @ 0x100220f4c, GetTotalMonthlyExpense @ 0x100220ff0,
//   GetTotalMonthlyIncome @ 0x1002210a8, DoBudgetGroupChanged @ 0x100221164,
//   SetTotalFunds @ 0x10022132a, GetTotalFunds @ 0x100221344,
//   DepositFunds @ 0x10022134e, WithdrawFunds @ 0x10022139e,
//   GetMinAllowableFunds @ 0x1002213d6, GetYTDIncome @ 0x1002213e4,
//   GetEstIncome @ 0x1002213ee, GetYTDExpenses @ 0x100221452,
//   GetEstExpenses @ 0x10022145c, GetTaxIncome() @ 0x1002214bc,
//   GetTaxIncome(int) @ 0x1002214ca, CreateDepartmentBudget @ 0x100221522,
//   GetBudgetGroup @ 0x1002216a2, RemoveDepartmentBudget @ 0x10022176a,
//   GetDepartmentBudget(unsigned int) @ 0x10022184e,
//   GetDepartmentBudget(cIGZString const&) @ 0x100221890,
//   GetAllGroups @ 0x100221b7c, SetGroupName @ 0x100221d0e,
//   GetDepartmentBudgetsInGroup @ 0x100221d30,
//   NeededFundingChanged @ 0x100221f28,
//   FundingPercentageChanged @ 0x100221f56,
//   GetFunding(cISC4DepartmentBudget*) @ 0x100221f84,
//   GetFunding(unsigned int) @ 0x100221f8c,
//   GetTaxRate @ 0x100221f94, GetBondIncrement @ 0x100221fce,
//   IssueBond @ 0x100221fdc, GetTotalBorrowed @ 0x100222288,
//   GetCurrentBorrowingLimit @ 0x100222292, GetCurrentBondLimit @ 0x1002222a8,
//   GetCurrentMaxOutstandingBondsLimit @ 0x1002222b6,
//   GetWeightedAssessedTaxValue @ 0x1002222c4, GetAllLoans @ 0x1002222e2,
//   GetMonthlyPaymentForLoan @ 0x1002223d2, GetFullCostOfLoan @ 0x10022243a,
//   GetTotalMonthlyBondPayments @ 0x10022245e,
//   GetBudgetItemInfo @ 0x100222484,
//   GetBudgetItemForPurpose @ 0x100222758,
//   ChangeBudgetItemLine @ 0x100222800, ChangeBudgetItemCost @ 0x1002228da,
//   ChangeBudgetItemLocalFunding @ 0x1002229b4,
//   AddBudgetItemToBudget @ 0x100222ac0,
//   RemoveBudgetItemFromBudget @ 0x100222b84,
//   GetLineItemFromBudgetItem @ 0x100222c4a,
//   CopyBudgetItemProperties @ 0x100222c84,
//   ShowBudgetWindow @ 0x100222fa4, GetTotalYearlyExpense @ 0x10022300c,
//   GetTotalYearlyIncome @ 0x10022301a,
//   SetTotalMonthlyExpense @ 0x100223028,
//   SetTotalYearlyExpense @ 0x100223036,
//   SetTotalMonthlyIncome @ 0x100223044,
//   SetTotalYearlyIncome @ 0x100223052, GetLoanTimeInMonths @ 0x100223060.
struct BudgetSimulatorSaveRecord {
    Uint32 memAddr = 0;
    Uint16 version = 5u;
    Sint64 totalFunds = 0;                          // +0x48
    Sint64 fundsMirror = 0;                         // +0x50
    Sint64 field58 = 0;                             // +0x58
    Sint64 lastTaxIncome = 0;                       // +0x6a8
    std::vector<BudgetDepartmentRef> departments;   // count at +0x88, map at +0x78
    std::array<BudgetTaxCategory, 12> taxCategories{}; // +0xa8, stride 0x80
    Sint64 totalMonthlyExpenseCity = 0;             // +0x6b8
    Sint64 totalOtherIncome = 0;                    // +0x6c0
    std::array<Sint64, 12> monthlyExpenseByMonth{}; // +0x6c8
    std::array<Sint64, 12> monthlyOtherIncomeByMonth{}; // +0x728
    std::array<Sint64, 6> yearlyTotals{};           // +0x788..+0x7b0
    std::vector<BudgetBond> bonds;                  // count at +0x7d0/+0x7c8
    Sint64 totalBondPayments = 0;                   // +0x7d8
    Sint64 lastMonthlyIncome = 0;                   // +0x8c0
    Sint64 field8C8 = 0;                            // +0x8c8
    Sint64 lastMonthlyExpense = 0;                  // +0x8d0
    Sint64 field8D8 = 0;                            // +0x8d8
    std::vector<BudgetGroup> groups;                // count at +0xa0, map at +0x90
    // Runtime (not serialized): YTD accumulators at +0x60/+0x68, reset each December.
    Sint64 yearToDateIncome = 0;                    // +0x60
    Sint64 yearToDateExpense = 0;                   // +0x68

    bool Read(cIGZIStream& stream) noexcept;
    bool Write(cIGZOStream& stream) const;
    bool ReadFromSC4Record(std::span<const Uint8> recordBytes) noexcept;

    Float32 GetTaxRate(Sint32 taxType) const noexcept;
    void SetTaxRate(Sint32 taxType, Float32 rate) noexcept;
    Sint64 GetWeightedAssessedTaxValue(Sint32 taxType) const noexcept;
    void SetTotalFunds(Sint64 funds) noexcept { totalFunds = funds; fundsMirror = funds; }
    Sint64 GetTotalFunds() const noexcept { return totalFunds; }
    bool DepositFunds(Sint64 amount) noexcept;
    bool WithdrawFunds(Sint64 amount, Sint64 minAllowableFunds = -100000) noexcept;
    Sint64 GetTotalMonthlyBondPayments() const noexcept;
    bool IssueBond(Uint32 requestedAmount, const BudgetSimulatorTuning& tuning,
                   Uint32 currentMonth, Uint32 currentYear);
};

// Per-lot input to the exact ComputeTax @ 0x100220c22 inner loop.
struct BudgetTaxLotInput {
    Sint32 purpose = 0;                             // building holder dword +0
    Sint32 wealth = 0;                              // lot wealth (table column)
    Sint64 assessedValueProperty = 0;               // building prop 0xe91a0b5f
    Uint8 sizeX = 0;                                // lot footprint
    Uint8 sizeZ = 0;
    Uint8 landValueClass = 0;                       // LandValueSim class at lot
    Sint32 holderCategory = 0;                      // building holder dword +4
    Sint32 simulatorTypeIndex = 0;                  // index into taxCapFactors (+0x890)
};

// Exact ComputeTax @ 0x100220c22: zeroes each category's +0x70/+0x78, then per
// lot: taxType = kTaxTypeByPurposeAndWealth[purpose][wealth] (skip if -1);
//   f = curve(lvClass) * sizeX * sizeZ + 12582912.0f
//   landComponent = (Sint64)(Uint32)(floatBits(f) + 0xb4c00000)  [float->int magic]
//   assessed = landComponent + assessedValueProperty
//   weighted = (float)assessed * taxTypeMultipliers[taxType] * wealthTaxFactors[holderCategory - wealth]
//   taxInt = (Sint64)weighted
//   income = (float)taxInt * (taxRate[taxType] * taxCapFactors[simulatorTypeIndex]) / 1200.0f
//   category.weightedAssessedValue += taxInt; categoryIncome[taxType] += (double)income
// Finally category.totalTaxIncome = (Sint64)categoryIncome[taxType] and the
// Sint64 total (truncated grand double sum) is returned.
Sint64 ComputeBudgetTax(
    BudgetSimulatorSaveRecord& record,
    const BudgetSimulatorTuning& tuning,
    std::span<const BudgetTaxLotInput> lots);

// Exact GetMonthlyPaymentForLoan @ 0x1002223d2:
//   r = (double)bondInterestRate / 12.0; p = pow(1.0 + r, -(double)bondTermMonths)
//   payment = (Sint64)((double)principal * r / (1.0 - p))
Sint64 GetMonthlyPaymentForLoan(const BudgetSimulatorTuning& tuning, Sint64 principal) noexcept;

// Advances one bond by one month (DoMonthlySimulation bond loop
// @ 0x10022088d): applies the monthly payment and interest
//   interest = (Sint64)((Float32)balance * (bondInterestRate / 12.0f))
//   balance = interest - payment + balance
// Returns true when the bond matures this month (remaining months == 0), in
// which case the caller subtracts principal from field58 and removes it.
bool AdvanceBudgetBondOneMonth(
    BudgetBond& bond,
    const BudgetSimulatorTuning& tuning,
    Sint32 currentMonth,
    Sint32 currentYear,
    Sint64& inOutTotalBondPayments) noexcept;

struct BudgetMonthlyInputs {
    Sint64 taxIncome = 0;                           // ComputeTax() result
    Sint64 otherIncome = 0;                         // city vtable +0xb8
    Sint64 neighborIncome = 0;                      // spNeighborsSim +0x100
    Sint64 cityExpense = 0;                         // city vtable +0xb0
    Sint64 neighborExpense = 0;                     // spNeighborsSim +0x108
    Sint64 departmentExpenses = 0;                  // sum of department monthly expenses
    Sint32 month = 1;                               // 1..12
    Sint32 year = 2000;
};

// Exact DoMonthlySimulation @ 0x10022061e (message/history side effects omitted):
// monthIdx = (month == 1) ? 11 : month - 2; funds sync, tax/other/expense
// bookkeeping, bond payments, treasury update, YTD accumulation, December
// year-end reset, and the low-funds warning predicate (funds <= minAllowableFunds).
// Returns true if the low-funds message (0xaa5cf1e3) would be posted.
bool ApplyMonthlyBudgetStep(
    BudgetSimulatorSaveRecord& record,
    const BudgetSimulatorTuning& tuning,
    const BudgetMonthlyInputs& inputs) noexcept;

} // namespace opensc4
