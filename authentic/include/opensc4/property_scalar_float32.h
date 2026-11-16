#pragma once

#include <bit>
#include <cstdint>

#if defined(__FAST_MATH__)
#error "Float32 getter requires strict floating-point compilation (no fast-math)."
#endif

namespace opensc4::property_scalar {

enum class ReadStatus { Value, Unsupported };

// Adapter representation: runtime variant tags, not DBPF property type codes.
struct Scalar32 {
    std::uint16_t runtimeTag;
    std::uint32_t bits;
};

// Scoped to default masked exceptions and round-to-nearest-even. Exception
// flags/traps and alternate floating-point environments are not reproduced.
// Compile with -fno-fast-math -ffp-contract=off.
inline ReadStatus ReadFloat32Bits(const Scalar32& source,
                                  std::uint32_t& outBits) noexcept {
    // cRZVariant::GetValFloat32 @ 0x10004f998; matching tag bypass at
    // 0x10004f9aa..0x10004f9b4 copies the payload without FP operations.
    if (source.runtimeTag == 0x000a) {
        outBits = source.bits;
        return ReadStatus::Value;
    }

    // cRZVariant::ConvertToFloat64 @ 0x100050286; source runtime tag 6
    // selects 0x1000502e6 (zero-extend Uint32, then cvtsi2sd rax), tag 7
    // selects 0x1000502f0 (signed cvtsi2sd dword). Volatile materializes
    // the Float64 intermediate before the Float32 conversion.
    double widened;
    if (source.runtimeTag == 0x0006) {
        widened = static_cast<double>(source.bits);
    } else if (source.runtimeTag == 0x0007) {
        widened = static_cast<double>(std::bit_cast<std::int32_t>(source.bits));
    } else {
        // Adapter-domain exclusion, not the original ConvertTypes false path.
        // Do not modify caller output for unsupported inputs.
        return ReadStatus::Unsupported;
    }
    volatile double intermediate = widened;
    // cRZVariant::ConvertTypes @ 0x10004efd5 calls ConvertToFloat64;
    // 0x10004efda cvtsd2ss, 0x10004efe0 movss stores the Float32 result.
    const float narrowed = static_cast<float>(intermediate);
    outBits = std::bit_cast<std::uint32_t>(narrowed);
    return ReadStatus::Value;
}

} // namespace opensc4::property_scalar
