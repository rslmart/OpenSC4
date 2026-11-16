#pragma once
#include "opensc4/civic_simulators.h"
#include <cstdint>

namespace opensc4::weather_coordinates {
// Bounded portable record adapter, not the original native object layout.
// Output references must denote live, distinct host objects disjoint from record.
// Floating operations require masked exceptions, round-to-nearest-even and
// gradual underflow. FP status flags and nondefault controls are outside scope.
// Invalid native conversion/division domains return unsupported without writes;
// this is caller policy, not an original-game branch or native return value.
enum class Status { applied, unsupported };
struct Cell { std::int32_t x = 0, z = 0; };
struct Position { float x = 0, z = 0; };
// Supported: finite x/z, finite positive widths, separately rounded float32
// quotients in [-2147483648,2147483648). Truncate each quotient toward zero.
Status PositionToCell(const WeatherSimulatorSaveRecord&, float x, float z, Cell&) noexcept;
// Supported: finite positive widths and finite separately rounded float32
// products. Convert signed int32 to float32 BEFORE multiplying each axis.
Status CellToPosition(const WeatherSimulatorSaveRecord&, std::int32_t x,
                      std::int32_t z, Position&) noexcept;
// All int32 inputs/scales: low-32-bit wrap, no C++ signed-overflow UB.
// Native oddity @ 0x1000e48c6: Z output also uses input X, ignoring input Z.
Status CellToStandardCityCell(const WeatherSimulatorSaveRecord&, std::int32_t x,
                              std::int32_t z, Cell&) noexcept;
// Reject either zero divisor or either INT32_MIN/-1 pair before any output.
// Otherwise signed division toward zero; rejection is caller policy.
Status StandardCityCellToCell(const WeatherSimulatorSaveRecord&, std::int32_t x,
                              std::int32_t z, Cell&) noexcept;
} // namespace opensc4::weather_coordinates
