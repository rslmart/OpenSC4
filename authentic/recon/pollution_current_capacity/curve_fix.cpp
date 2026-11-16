// Extracted verbatim production response-curve definitions for isolated validation.
#include "opensc4/tract_developer.h"
#include <bit>
#include <limits>
namespace opensc4 {
// Exact cSC4ResponseCurve::SetResponseCurveData @ 0x1000ac220.
bool cSC4ResponseCurve::SetResponseCurveData(std::span<const Point> points) {
    mSegments.clear();
    if (points.empty()) {
        return false;
    }

    constexpr Float64 kEpsilon = 1e-10;
    const Float32 negMaxFloat = -std::numeric_limits<Float32>::max();

    for (std::size_t i = 0; i < points.size(); ++i) {
        const Float32 x = points[i].x;
        const Float32 y = points[i].y;
        const Float32 prevX = (i == 0u) ? negMaxFloat : points[i - 1u].x;
        const Float32 prevY = (i == 0u) ? y : points[i - 1u].y;
        const Float32 dy = y - prevY;
        // andps @ 0x1000ac2bf clears the binary32 sign BEFORE cvtss2sd.
        const Float32 absDyFloat = std::bit_cast<Float32>(
            std::bit_cast<Uint32>(dy) & 0x7fffffffu);
        const Float64 absDy = static_cast<Float64>(absDyFloat);

        Float32 slope = 0.0f;
        // Both jbe branches include unordered operands.
        if (!(absDy < kEpsilon)) {
            const Float32 dx = x - prevX;
            if (!(static_cast<Float64>(dx) > static_cast<Float64>(dy) * kEpsilon)) {
                continue;
            }
            slope = dy / dx;
        }

        mSegments.push_back(Segment{x, y, prevX, prevY, slope});
        // First-slot writes @ 0x1000ac308..0x1000ac32a replace retained Y.
        // Clearing logical contents does not erase original backing storage.
        if (mSegments.size() == 1u) {
            mRetainedFirstY = y;
            mHasRetainedFirstY = true;
        }
    }

    return true;
}

bool cSC4ResponseCurve::SetResponseCurveData(
    std::span<const Float32> flatXYPairs) {
    const std::size_t pointCount = flatXYPairs.size() / 2u;
    std::vector<Point> points;
    points.reserve(pointCount);
    for (std::size_t i = 0; i < pointCount; ++i) {
        points.push_back(Point{flatXYPairs[i * 2u], flatXYPairs[i * 2u + 1u]});
    }
    return SetResponseCurveData(points);
}

// Exact present-property path @ 0x1000ac37a; shr @ 0x1000ac3cb
// floors the scalar count, and even zero pairs call the clearing setter.
bool cSC4ResponseCurve::SetResponseCurveData(const ExemplarProperty& prop) {
    return SetResponseCurveData(std::span<const Float32>(prop.floatValues));
}

Float32 cSC4ResponseCurve::GetOutputValue(Float32 input) const noexcept {
    Float32 out;
    if (TryGetOutputValue(input, out)) return out;
    // clean-room fallback (not authentic): legacy callers get zero only for
    // missing backing storage. Authentic adapters must propagate Try's fault.
    return 0.0f;
}

// Exact evaluation @ 0x1000ac4f6, with an explicit portable fault boundary
// for the null backing-slot read @ 0x1000ac542 (out remains untouched).
bool cSC4ResponseCurve::TryGetOutputValue(Float32 input, Float32& out) const noexcept {
    if (mSegments.empty()) {
        if (!mHasRetainedFirstY) return false;
        out = mRetainedFirstY;
        return true;
    }
    if (mSegments.size() < 2u) {
        out = mSegments.front().y;
        return true;
    }

    const Sint32 last = static_cast<Sint32>(mSegments.size() - 1u);
    Sint32 low = 0;
    Sint32 high = last;
    while (high > low) {
        const Sint32 mid = (low + high) >> 1;
        const Float32 midX = mSegments[static_cast<std::size_t>(mid)].x;
        // Complementary cmova/cmovbe @ 0x1000ac527: unordered advances low.
        if (midX > input) {
            high = mid;
        } else {
            low = mid + 1;
        }
    }

    if (low <= 0) {
        out = mSegments.front().y;
        return true;
    }
    if (low >= last) {
        const Segment& endSeg = mSegments[static_cast<std::size_t>(last)];
        // jae @ 0x1000ac554 excludes unordered comparisons.
        if (input >= endSeg.x) {
            out = endSeg.y;
            return true;
        }
    }

    const Segment& seg = mSegments[static_cast<std::size_t>(low)];
    // subss, mulss, addss @ 0x1000ac55c..0x1000ac568; build with no FMA.
    const Float32 dx = input - seg.prevX;
    const Float32 dy = dx * seg.slope;
    out = dy + seg.prevY;
    return true;
}

bool cSC4ResponseCurve::GetMaxInputValue(Float32& outMaxX) const noexcept {
    if (mSegments.empty()) {
        return false;
    }
    outMaxX = mSegments.back().x;
    return true;
}

}
