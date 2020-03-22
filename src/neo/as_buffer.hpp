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
    constexpr decltype(auto) operator()(T&& what) const noexcept(noexcept(as_buffer(what))) {
        return as_buffer(what);
    }

    /**
     * Find member as_buffer
     */
    template <detail::has_member_as_buffer T>
    constexpr decltype(auto) operator()(T&& what) const noexcept(noexcept(what.as_buffer())) {
        return what.as_buffer();
    }

    /**
     * When both are available, prefer member as_buffer
     */
    template <detail::has_both_as_buffer T>
    constexpr decltype(auto) operator()(T&& what) const noexcept(noexcept(what.as_buffer())) {
        return what.as_buffer();
    }

    // #############################################################################
    /**
     * Create a mutable buffer that refers to the bytes of an array of trivial objects.
     */
    template <buffer_safe Trivial, std::size_t N>
    constexpr mutable_buffer operator()(Trivial (&item)[N],
                                        std::size_t max_size = sizeof(Trivial[N])) const noexcept {
        auto min_size = std::min(sizeof(item), max_size);
        return mutable_buffer(byte_pointer(std::addressof(item)), min_size);
    }

    /**
     * Create an immutable buffer that refers to the bytes of an array of trivial objects.
     */
    template <buffer_safe Trivial, std::size_t N>
    constexpr const_buffer operator()(const Trivial (&item)[N],
                                      std::size_t max_size = sizeof(Trivial[N])) const noexcept {
        auto min_size = std::min(sizeof(item), max_size);
        return const_buffer(byte_pointer(std::addressof(item)), min_size);
    }

    // #############################################################################
    /**
     * Create a mutable buffer referring to the elements of a data container
     */
    template <mutable_data_container Container>
    constexpr mutable_buffer operator()(Container& c, std::size_t max_size) const noexcept {
        return mutable_buffer(byte_pointer(c.data()),
                              std::min(max_size, data_container_byte_size(c)));
    }

    template <mutable_data_container Container>
    constexpr mutable_buffer operator()(Container& c) const noexcept {
        return (*this)(c, data_container_byte_size(c));
    }

    /**
     * Create an immutable buffer referring to the elements of a data container
     */
    template <data_container C>
    constexpr const_buffer operator()(const C& c, std::size_t max_size) const noexcept {
        return const_buffer(byte_pointer(c.data()),
                            std::min(max_size, data_container_byte_size(c)));
    }

    template <data_container C>
    constexpr const_buffer operator()(const C& c) const noexcept {
        return (*this)(c, data_container_byte_size(c));
    }
} as_buffer;

}  // namespace cpo

}  // namespace neo
