#include "opensc4/tract_developer.h"
#include <array>
#include <cmath>
#include <cstdio>
#include <limits>
#include <span>
using namespace opensc4;
static int checks = 0, failures = 0;
static void Check(bool ok, const char* label) {
    ++checks;
    if (!ok) { ++failures; std::printf("FAIL: %s\n", label); }
}
int main() {
// CURVE_REGRESSIONS_BEGIN
        // Binary boundary fixtures: setter @ 0x1000ac220, property @ 0x1000ac37a,
        // getter @ 0x1000ac4f6. Historical old-code hangs recorded by independent
        // review; execute these only against the fixed terminating search.
        {
            using Curve = cSC4ResponseCurve;
            using Point = Curve::Point;
            const Float32 nan = std::numeric_limits<Float32>::quiet_NaN();
            const auto set = [](Curve& c, std::initializer_list<Point> pts) {
                return c.SetResponseCurveData(std::span<const Point>(pts.begin(), pts.size()));
            };
            Curve c;
            Float32 out = 123.0f;
            Check(!c.TryGetOutputValue(nan, out) && out == 123.0f,
                  "curve fresh missing storage reports fault without changing output");
            set(c, {{0, 1}, {1, 2}});
            Check(c.TryGetOutputValue(nan, out) && std::isnan(out),
                  "curve unordered input terminates and interpolates NaN");
            set(c, {{0, 1}, {nan, 1}, {2, 1}});
            Check(c.Segments().size() == 3 && c.TryGetOutputValue(0.5f, out) && std::isnan(out),
                  "curve unordered midX terminates and retains input predecessor NaN");
            set(c, {{0, 1}, {1, nan}});
            const bool nanDYSkipped = c.Segments().size() == 1;
            set(c, {{0, 1}, {nan, 2}});
            Check(nanDYSkipped && c.Segments().size() == 1,
                  "curve unordered dy and dx skip segments");
            Curve freshSkipped;
            out = 321.0f;
            const bool allSkipped = set(freshSkipped, {{0, nan}, {1, nan}});
            Check(allSkipped && freshSkipped.Empty() &&
                  !freshSkipped.TryGetOutputValue(0, out) && out == 321.0f,
                  "curve nonempty all-skipped input succeeds without backing slot");
            set(c, {{0, 7}});
            ExemplarProperty emptyProperty;
            const bool presentEmpty = c.SetResponseCurveData(emptyProperty);
            Check(!presentEmpty && c.Empty() && c.TryGetOutputValue(0, out) && out == 7,
                  "curve present empty property clears logical segments and retains Y7");
            const bool retainedSkipped = set(c, {{0, nan}});
            Check(retainedSkipped && c.Empty() && c.TryGetOutputValue(nan, out) && out == 7,
                  "curve all-skipped input preserves earlier backing-slot Y");
            // First two input points skip (NaN dy); first accepted point is index 2.
            set(c, {{0, nan}, {1, 9}, {2, 9}});
            const bool acceptedLate = c.Segments().size() == 1;
            const bool cleared = !c.SetResponseCurveData(std::span<const Point>{});
            Curve copied(c);
            Curve assigned; assigned = c;
            Check(acceptedLate && cleared && c.Empty() &&
                  c.TryGetOutputValue(0, out) && out == 9 &&
                  copied.TryGetOutputValue(0, out) && out == 9 &&
                  assigned.TryGetOutputValue(0, out) && out == 9,
                  "curve next first accepted segment updates retained Y and copies preserve it");
            ExemplarProperty oddProperty;
            oddProperty.floatValues = {0, 3, 99};
            const bool oddSet = c.SetResponseCurveData(oddProperty);
            Check(oddSet && c.Segments().size() == 1 && c.GetOutputValue(99) == 3,
                  "curve present raw property floors odd scalar count");
            const Float32 above = 1e-10f;
            const Float32 below = std::nextafter(above, 0.0f);
            set(c, {{0, 0}, {1, below}});
            const bool belowFlat = c.Segments().size() == 2 && c.Segments()[1].slope == 0;
            set(c, {{0, 0}, {1, above}});
            Check(static_cast<double>(below) < 1e-10 && static_cast<double>(above) > 1e-10 &&
                  belowFlat && c.Segments().size() == 2 && c.Segments()[1].slope == above,
                  "curve epsilon binary32 neighbors distinguish flat from division");
            set(c, {{-0.0f, -0.0f}, {0.0f, 0.0f}});
            const bool zeroSlope = c.Segments().size() == 2 &&
                !std::signbit(c.Segments()[0].slope) && !std::signbit(c.Segments()[1].slope);
            const bool negativeY = c.TryGetOutputValue(-1, out) && std::signbit(out);
            const bool positiveY = c.TryGetOutputValue(0, out) && !std::signbit(out);
            Check(zeroSlope && negativeY && positiveY,
                  "curve signed zeros preserve Y signs and positive zero slopes");
            set(c, {{0, 2}, {0, 1}});
            Check(c.Segments().size() == 2 && std::isinf(c.Segments()[1].slope) &&
                  std::signbit(c.Segments()[1].slope),
                  "curve duplicate x negative dy admits division by positive zero");
            set(c, {{0, 0}, {0, 10}, {2, 14}});
            Check(c.Segments().size() == 2 && c.Segments()[1].prevX == 0 &&
                  c.Segments()[1].prevY == 10 && c.Segments()[1].slope == 2 &&
                  c.GetOutputValue(1) == 12,
                  "curve skipped input remains predecessor for following accepted segment");
        }
// CURVE_REGRESSIONS_END
    std::printf("%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
