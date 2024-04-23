/**
 * @file types.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>

#include <cstdint>

// NOLINTBEGIN
inline namespace labrat {
namespace lbot {
inline namespace utils {

// Signed integer types.
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;

// Unsigned integer types.
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;

// Float types.
using f32 = float;
using f64 = double;
using f128 = long double;

}  // namespace utils
}  // namespace lbot
}  // namespace labrat
// NOLINTEND
