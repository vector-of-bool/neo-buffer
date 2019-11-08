#pragma once

#include <type_traits>
#include <cstddef>

namespace neo {

template <typename T>
constexpr std::byte* byte_pointer(T* ptr) noexcept requires std::is_trivial_v<T> {
    auto void_ = static_cast<void*>(ptr);
    return static_cast<std::byte*>(void_);
}

template <typename T>
constexpr const std::byte* byte_pointer(const T* ptr) noexcept requires std::is_trivial_v<T> {
    auto void_ = static_cast<const void*>(ptr);
    return static_cast<const std::byte*>(void_);
}

}  // namespace neo