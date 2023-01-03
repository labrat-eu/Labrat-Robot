#pragma once

#include <concepts>

inline namespace utils {

template <typename T, template <typename...> class Z>
struct is_specialization_of : std::false_type {};

template <typename... Args, template <typename...> class Z>
struct is_specialization_of<Z<Args...>, Z> : std::true_type {};

template <typename T, template <typename...> class Z>
inline constexpr bool is_specialization_of_v = is_specialization_of<T, Z>::value;

}  // namespace utils
