#pragma once

#include <neo/concepts.hpp>

#include <cstddef>
#include <type_traits>

namespace neo {

struct proto_buffer_safe {
    proto_buffer_safe() = delete;
};

/**
 * A type is `buffer_safe` iff it is trivially copyable, or is an array
 * thereof that is: It is well-defined to make a copy of the object by copying
 * the representation of that object into another location.
 */
// clang-format off
template <typename T>
concept buffer_safe =
    trivially_copyable<T> ||
    (std::is_array_v<T> && trivially_copyable<std::remove_all_extents_t<T>>);

template <typename T>
concept buffer_safe_cvr = buffer_safe<std::remove_cvref_t<T>>;
// clang-format on

/**
 * Convert the given pointer to a pointer to `std::byte` referring to the
 * storage of the object that would live at `ptr`.
 */
template <buffer_safe T>
constexpr std::byte* byte_pointer(T* ptr) noexcept {
    auto void_ = static_cast<void*>(ptr);
    return static_cast<std::byte*>(void_);
}

/**
 * Convert the given pointer to a pointer to `const std::byte` referring to the
 * storage of the object that would live at `ptr`.
 */
template <buffer_safe T>
constexpr const std::byte* byte_pointer(const T* ptr) noexcept {
    auto void_ = static_cast<const void*>(ptr);
    return static_cast<const std::byte*>(void_);
}

}  // namespace neo
