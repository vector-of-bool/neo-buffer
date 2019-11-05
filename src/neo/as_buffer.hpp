#pragma once

#include <neo/buffer_concepts.hpp>
#include <neo/byte_pointer.hpp>
#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <array>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace neo {

inline constexpr struct as_buffer_fn {
    /**
     * Copy an existing mutable_buffer
     */
    constexpr mutable_buffer operator()(const mutable_buffer& b) const noexcept { return b; }
    constexpr mutable_buffer operator()(const mutable_buffer& b, mutable_buffer::size_type s) const
        noexcept {
        return mutable_buffer(b.data(), std::min(s, b.size()));
    }

    /**
     * Copy an existing const_buffer
     */
    constexpr const_buffer operator()(const const_buffer& b) const noexcept { return b; }
    constexpr const_buffer operator()(const const_buffer& b, const_buffer::size_type s) const
        noexcept {
        return const_buffer(b.data(), std::min(s, b.size()));
    }

    // #############################################################################
    /**
     * Create a mutable buffer that refers to the bytes of an array of trivial objects.
     */
    template <typename Trivial, std::size_t N>
    requires std::is_trivial_v<Trivial>  //
        constexpr mutable_buffer operator()(Trivial (&item)[N],
                                            std::size_t max_size = sizeof(Trivial[N])) const
        noexcept {
        auto min_size = std::min(sizeof(item), max_size);
        return mutable_buffer(byte_pointer(std::addressof(item)), min_size);
    }

    /**
     * Create an immutable buffer that refers to the bytes of an array of trivial objects.
     */
    template <typename Trivial,
              std::size_t N>
    requires std::is_trivial_v<Trivial>  //
        constexpr const_buffer operator()(const Trivial (&item)[N],
                                          std::size_t max_size = sizeof(Trivial[N])) const
        noexcept {
        auto min_size = std::min(sizeof(item), max_size);
        return const_buffer(byte_pointer(std::addressof(item)), min_size);
    }

    // #############################################################################
    /**
     * Create a mutable buffer referring to the elements of a std::array
     */
    template <typename Elem, std::size_t N>
requires std::is_trivial_v<Elem> && !std::is_const_v<Elem> //
    constexpr mutable_buffer operator()(std::array<Elem, N>& arr,
                                    std::size_t max_size = sizeof(std::array<Elem, N>)) const noexcept {
        return mutable_buffer(byte_pointer(arr.data()),
                              std::min(max_size, arr.size() * sizeof(Elem)));
    }

    /**
     * Create an immutable buffer referring to the elements of a std::array
     */
    template <typename Elem, std::size_t N>
    requires std::is_trivial_v<Elem>  //
        constexpr const_buffer operator()(const std::array<Elem, N>& arr,
                                          std::size_t max_size = sizeof(std::array<Elem, N>)) const
        noexcept {
        return const_buffer(byte_pointer(arr.data()),
                            std::min(max_size, arr.size() * sizeof(Elem)));
    }

    template <typename Elem, std::size_t N>
    requires std::is_trivial_v<Elem>  //
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
    constexpr mutable_buffer operator()(std::basic_string<Char, Traits, Alloc>& str) const
        noexcept {
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
    constexpr const_buffer operator()(const std::basic_string<Char, Traits, Alloc>& str) const
        noexcept {
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
    template <typename Elem, typename Alloc>
    requires std::is_trivial_v<Elem>  //
        constexpr mutable_buffer operator()(std::vector<Elem, Alloc>& vec,
                                            std::size_t               max_size) const noexcept {
        return mutable_buffer(byte_pointer(vec.data()),
                              std::min(max_size, vec.size() * sizeof(Elem)));
    }

    template <typename Elem, typename Alloc>
    requires std::is_trivial_v<Elem>  //
        constexpr mutable_buffer operator()(std::vector<Elem, Alloc>& vec) const noexcept {
        return (*this)(vec, vec.size() * sizeof(Elem));
    }

    /**
     * Create an immutable buffer to the contents of a vector of trivial objects.
     */
    template <typename Elem, typename Alloc>
    requires std::is_trivial_v<Elem>  //
        constexpr const_buffer operator()(const std::vector<Elem, Alloc>& vec,
                                          std::size_t                     max_size) const noexcept {
        return const_buffer(byte_pointer(vec.data()),
                            std::min(max_size, vec.size() * sizeof(Elem)));
    }

    template <typename Elem, typename Alloc>
    requires std::is_trivial_v<Elem>  //
        constexpr const_buffer operator()(const std::vector<Elem, Alloc>& vec) const noexcept {
        return (*this)(vec, vec.size() * sizeof(Elem));
    }
} as_buffer;

template <typename T>
NEO_CONCEPT can_view_as_const_buffer = requires(T buf, const T cbuf) {
    { as_buffer(buf) }
    ->neo::const_buffer;
    { as_buffer(cbuf) }
    ->neo::const_buffer;
};

template <typename T>
NEO_CONCEPT can_view_as_mutable_buffer = requires(T buf) {
    can_view_as_const_buffer<T>;
    { as_buffer(buf) }
    ->neo::const_buffer;
};

}  // namespace neo
