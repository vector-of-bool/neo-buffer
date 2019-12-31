#pragma once

#include <neo/buffer_concepts.hpp>
#include <neo/byte_pointer.hpp>
#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <neo/concepts.hpp>

#include <array>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

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

    /**
     * Copy an existing mutable_buffer
     */
    constexpr mutable_buffer operator()(const mutable_buffer& b) const noexcept { return b; }
    constexpr mutable_buffer operator()(const mutable_buffer&     b,
                                        mutable_buffer::size_type s) const noexcept {
        return mutable_buffer(b.data(), std::min(s, b.size()));
    }

    /**
     * Copy an existing const_buffer
     */
    constexpr const_buffer operator()(const const_buffer& b) const noexcept { return b; }
    constexpr const_buffer operator()(const const_buffer&     b,
                                      const_buffer::size_type s) const noexcept {
        return const_buffer(b.data(), std::min(s, b.size()));
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
     * Create a mutable buffer referring to the elements of a std::array
     */
    template <buffer_safe Elem, std::size_t N>
    constexpr mutable_buffer operator()(std::array<Elem, N>& arr,
                                        std::size_t          max_size
                                        = sizeof(std::array<Elem, N>)) const noexcept {
        return mutable_buffer(byte_pointer(arr.data()),
                              std::min(max_size, arr.size() * sizeof(Elem)));
    }

    /**
     * Create an immutable buffer referring to the elements of a std::array
     */
    template <buffer_safe Elem, std::size_t N>
    constexpr const_buffer operator()(const std::array<Elem, N>& arr,
                                      std::size_t                max_size
                                      = sizeof(std::array<Elem, N>)) const noexcept {
        return const_buffer(byte_pointer(arr.data()),
                            std::min(max_size, arr.size() * sizeof(Elem)));
    }

    template <buffer_safe Elem, std::size_t N>
    constexpr const_buffer operator()(const std::array<const Elem, N>& arr,
                                      std::size_t                      max_size
                                      = sizeof(std::array<const Elem, N>)) const noexcept {
        return const_buffer(byte_pointer(arr.data()),
                            std::min(max_size, arr.size() * sizeof(Elem)));
    }

    // #############################################################################
    /**
     * Create a mutable buffer referring to the characters of a basic_string object
     */
    template <typename Char, typename Traits, typename Alloc>
    constexpr mutable_buffer operator()(std::basic_string<Char, Traits, Alloc>& str,
                                        std::size_t max_size) const noexcept {
        return mutable_buffer(byte_pointer(str.data()),
                              std::min(max_size, str.size() * sizeof(Char)));
    }

    template <typename Char, typename Traits, typename Alloc>
    constexpr mutable_buffer
    operator()(std::basic_string<Char, Traits, Alloc>& str) const noexcept {
        return (*this)(str, str.size() * sizeof(Char));
    }

    /**
     * Create an immutable buffer refering to the characters of a basic_string object
     */
    template <typename Char, typename Traits, typename Alloc>
    constexpr const_buffer operator()(const std::basic_string<Char, Traits, Alloc>& str,
                                      std::size_t max_size) const noexcept {
        return const_buffer(byte_pointer(str.data()),
                            std::min(max_size, str.size() * sizeof(Char)));
    }

    template <typename Char, typename Traits, typename Alloc>
    constexpr const_buffer
    operator()(const std::basic_string<Char, Traits, Alloc>& str) const noexcept {
        return (*this)(str, str.size() * sizeof(Char));
    }

    // #############################################################################
    /**
     * Create an immutable buffer referring to the characters of a basic_string_view
     */
    template <typename Char, typename Traits>
    constexpr const_buffer operator()(std::basic_string_view<Char, Traits> sv,
                                      std::size_t max_size) const noexcept {
        return const_buffer(byte_pointer(sv.data()), std::min(max_size, sv.size() * sizeof(Char)));
    }

    template <typename Char, typename Traits>
    constexpr const_buffer operator()(std::basic_string_view<Char, Traits> sv) const noexcept {
        return (*this)(sv, sv.size() * sizeof(Char));
    }

    // #############################################################################
    /**
     * Create a mutable buffer to the contents of a vector of trivial objects.
     */
    template <buffer_safe Elem, typename Alloc>
    constexpr mutable_buffer operator()(std::vector<Elem, Alloc>& vec,
                                        std::size_t               max_size) const noexcept {
        return mutable_buffer(byte_pointer(vec.data()),
                              std::min(max_size, vec.size() * sizeof(Elem)));
    }

    template <buffer_safe Elem, typename Alloc>
    constexpr mutable_buffer operator()(std::vector<Elem, Alloc>& vec) const noexcept {
        return (*this)(vec, vec.size() * sizeof(Elem));
    }

    /**
     * Create an immutable buffer to the contents of a vector of trivial objects.
     */
    template <buffer_safe Elem, typename Alloc>
    constexpr const_buffer operator()(const std::vector<Elem, Alloc>& vec,
                                      std::size_t                     max_size) const noexcept {
        return const_buffer(byte_pointer(vec.data()),
                            std::min(max_size, vec.size() * sizeof(Elem)));
    }

    template <buffer_safe Elem, typename Alloc>
    constexpr const_buffer operator()(const std::vector<Elem, Alloc>& vec) const noexcept {
        return (*this)(vec, vec.size() * sizeof(Elem));
    }
} as_buffer;

}  // namespace cpo

}  // namespace neo
