#pragma once

#include <neo/buffer_concepts.hpp>
#include <neo/byte_pointer.hpp>
#include <neo/const_buffer.hpp>
#include <neo/data_container_concepts.hpp>
#include <neo/mutable_buffer.hpp>

#include <neo/concepts.hpp>

#include <algorithm>
#include <type_traits>
#include <utility>

namespace neo {

namespace detail {

// clang-format off
template <typename T>
concept has_nonmember_as_buffer = requires(T&& t) {
    { as_buffer(static_cast<T&&>(t)) } -> neo::convertible_to<const_buffer>;
};

template <typename T>
concept has_member_as_buffer = requires(T&& t) {
    { static_cast<T&&>(t).as_buffer() } -> neo::convertible_to<const_buffer>;
};

template <typename T>
concept has_both_as_buffer =
    has_nonmember_as_buffer<T> &&
    has_member_as_buffer<T>;
// clang-format on

}  // namespace detail

inline namespace cpo {

inline constexpr struct as_buffer_fn {
    /**
     * Find ADL as_buffer
     */
    template <detail::has_nonmember_as_buffer T>
    [[nodiscard]] constexpr decltype(auto) operator()(T&& what) const
        noexcept(noexcept(as_buffer(what))) {
        return as_buffer(what);
    }

    /**
     * Find member as_buffer
     */
    template <detail::has_member_as_buffer T>
    [[nodiscard]] constexpr decltype(auto) operator()(T&& what) const
        noexcept(noexcept(what.as_buffer())) {
        return what.as_buffer();
    }

    /**
     * When both are available, prefer member as_buffer
     */
    template <detail::has_both_as_buffer T>
    [[nodiscard]] constexpr decltype(auto) operator()(T&& what) const
        noexcept(noexcept(what.as_buffer())) {
        return what.as_buffer();
    }

    // #############################################################################
    /**
     * Create a mutable buffer referring to the elements of a data container
     */
    template <data_container Container>
    [[nodiscard]] constexpr auto operator()(Container&& c, std::size_t max_size) const noexcept {
        return (*this)(byte_pointer(std::data(c)), std::min(max_size, data_container_byte_size(c)));
    }

    template <data_container Container>
    [[nodiscard]] constexpr auto operator()(Container&& c) const noexcept {
        return (*this)(c, data_container_byte_size(c));
    }

    [[nodiscard]] constexpr const_buffer operator()(const std::byte* ptr,
                                                    std::size_t      size) const noexcept {
        return const_buffer(ptr, size);
    }

    [[nodiscard]] constexpr mutable_buffer operator()(std::byte*  ptr,
                                                      std::size_t size) const noexcept {
        return mutable_buffer(ptr, size);
    }
} as_buffer;

}  // namespace cpo

template <typename T>
concept as_buffer_convertible = requires(T val) {
    as_buffer(val);
};

template <typename T>
using as_buffer_t = decltype(as_buffer(std::declval<T&>()));

}  // namespace neo
