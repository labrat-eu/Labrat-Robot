/**
 * @file concepts.hpp
 * @author Max Yvon Zimmermann
 *
 * @copyright GNU Lesser General Public License v3.0 (LGPL-3.0-or-later)
 *
 */

#pragma once

#include <labrat/lbot/base.hpp>

#include <concepts>

/** @cond INTERNAL */
inline namespace labrat {
/** @endcond */
namespace lbot {
/** @cond INTERNAL */
inline namespace utils {
/** @endcond */

/** @cond INTERNAL */
template <typename T, template <typename...> class Z>
struct is_specialization_of : std::false_type {};

template <typename... Args, template <typename...> class Z>
struct is_specialization_of<Z<Args...>, Z> : std::true_type {};

template <typename T, template <typename...> class Z>
inline constexpr bool is_specialization_of_v = is_specialization_of<T, Z>::value;
/** @endcond  */

/** @cond INTERNAL */
}  // namespace utils
/** @endcond */
}  // namespace lbot
/** @cond INTERNAL */
}  // namespace labrat
/** @endcond */
