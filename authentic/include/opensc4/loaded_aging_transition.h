#pragma once
#include "opensc4/property_scalar_sint32.h"
#include "opensc4/tract_developer.h"
#include <array>
#include <memory>
#include <map>
namespace opensc4 { class cSC4City; }
namespace opensc4::loaded_aging_transition {
// Explicit snapshot boundary, NOT original save hydration or a native ABI.
enum class Status { Ok, Missing, Unsupported, InvalidSnapshot, CurveMismatch, NotificationExcluded, NoState };
struct Variant { property_scalar::Scalar32 scalar{}; std::vector<uint32_t> array; };
struct Property { uint32_t identity; std::shared_ptr<Variant> variant; };
struct PropertyInput { uint32_t identity, variantIdentity; property_scalar::Scalar32 scalar; std::vector<uint32_t> array; };
using Holder = std::map<uint32_t, uint32_t>; // property key -> logical identity; 0 = unmodeled
using Records = std::vector<std::array<uint32_t,5>>;
struct Snapshot {
    std::vector<PropertyInput> properties;
    Holder local, defaults;
    Records fundingCurve, factorCurve;
    uint32_t fundingBits{}, percentageZeroBits{}, scaleBits{};
    bool lotExists{};
    uint8_t lotFlags{}, lotState{}, lotCondition{};
    std::array<uint32_t,4> lotRectangle{};
    std::array<uint32_t,3> occupantTGI{};
    uint32_t lotConfig{};
};
enum class Event { FundingReference, Used, Capacity, Power, Scale, Construct, Load, CapacityInput, Evaluate, Destroy, Age, Multiplier, Lower, Upper, FundingRead, FundingCurve, FactorCurve, Write, Threshold };
struct Report {
    Status status = Status::Ok;
    uint32_t capacity{}, factorBits{}, fundingCurveBits{}, factorCurveBits{}, beforeBits{}, afterBits{};
    unsigned agingLotLookups{}, conditionCalls{}, posts{};
    std::vector<Event> trace;
};
class State {
public:
    std::shared_ptr<const Property> Lookup(uint32_t key) const;
    Status ReadFloat(uint32_t key, uint32_t& initializedOutput) const;
    Status ReadSigned(uint32_t key, int32_t& initializedOutput) const;
    Status ReadUnsigned(uint32_t key, uint32_t& initializedOutput) const;
    Status WriteFloat(uint32_t key, uint32_t bits);
    const Snapshot& ImportedSnapshot() const { return source; }
private:
    Snapshot source;
    std::map<uint32_t,std::shared_ptr<Property>> objects;
    cSC4ResponseCurve funding, factor;
    friend Status Import(cSC4City&, const Snapshot&);
    friend // Masked RNE / gradual underflow only; flags/traps and alternate FP excluded.
Report Execute(cSC4City&);
};
Status Import(cSC4City&, const Snapshot&); // publishes only after complete validation
// Masked RNE / gradual underflow only; flags/traps and alternate FP excluded.
Report Execute(cSC4City&); // one local-age transition; notification branches excluded
} // namespace opensc4::loaded_aging_transition
