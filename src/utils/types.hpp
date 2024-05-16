/**
 * @file types.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v2.1 or later (LGPL-2.1-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>

#include <cstdint>

// NOLINTBEGIN
/** @cond INTERNAL */
inline namespace labrat {
/** @endcond */
namespace lbot {
/** @cond INTERNAL */
inline namespace utils {
/** @endcond */

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

/** @cond INTERNAL */
}  // namespace utils
/** @endcond */
}  // namespace lbot
/** @cond INTERNAL */
}  // namespace labrat
/** @endcond */
// NOLINTEND
