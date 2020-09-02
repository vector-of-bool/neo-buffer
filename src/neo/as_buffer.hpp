#pragma once

#include <neo/buffer_range.hpp>
#include <neo/byte_pointer.hpp>
#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>
#include <neo/trivial_range.hpp>

#include <neo/concepts.hpp>
#include <neo/fwd.hpp>
#include <neo/ref.hpp>

#include <algorithm>
#include <type_traits>
#include <utility>

namespace neo {

namespace detail {

// clang-format off
template <typename T>
concept has_adl_as_buffer = requires(T t) {
    { as_buffer(NEO_FWD(t)) } noexcept -> convertible_to<const_buffer>;
};

template <typename T>
concept has_member_as_buffer = requires(T t) {
    { NEO_FWD(t).as_buffer() } noexcept -> convertible_to<const_buffer>;
};

template <typename T>
concept as_buffer_convertible_check =
       has_member_as_buffer<T>
    || has_adl_as_buffer<T>
    || constructible_from<const_buffer, T>
    || constructible_from<mutable_buffer, T>
    ;
// clang-format on

}  // namespace detail

namespace cpo {

inline constexpr struct as_buffer_fn {
    /**
     * Convert an object into a singular buffer that views the contents of that
     * object.
     */
    template <detail::as_buffer_convertible_check T>
    [[nodiscard]] constexpr decltype(auto) operator()(T&& what) const noexcept {
        // First, prefer member .as_buffer()
        if constexpr (detail::has_member_as_buffer<T>) {
            return NEO_FWD(what).as_buffer();
        }
        // Second, check ADL-found
        else if constexpr (detail::has_adl_as_buffer<T>) {
            return as_buffer(NEO_FWD(what));
        }
        // Third, just check if it can convert to mutable_buffer
        else if constexpr (std::is_constructible_v<mutable_buffer, T>) {
            return static_cast<mutable_buffer>(NEO_FWD(what));
        }
        // Finally, convert to const_buffer
        else {
            static_assert(std::is_constructible_v<const_buffer, T>,
                          "You should never see this. This is a bug in neo-buffer.");
            return static_cast<const_buffer>(NEO_FWD(what));
        }
    }

    /**
     * Convert to a singular buffer, but ensure that it is no larger than `max_size`
     */
    template <detail::as_buffer_convertible_check T>
    [[nodiscard]] constexpr decltype(auto) operator()(T&&         what,
                                                      std::size_t max_size) const noexcept {
        auto buf = (*this)(NEO_FWD(what));
        if (max_size < buf.size()) {
            return buf.first(max_size);
        }
        return buf;
    }

    /**
     * Convert a std::byte pointer into a const_buffer
     */
    [[nodiscard]] constexpr const_buffer operator()(const std::byte* ptr,
                                                    std::size_t      size) const noexcept {
        return const_buffer(ptr, size);
    }

    /**
     * Convert a std::byte pointer to a mutable_buffer
     */
    [[nodiscard]] constexpr mutable_buffer operator()(std::byte*  ptr,
                                                      std::size_t size) const noexcept {
        return mutable_buffer(ptr, size);
    }
} as_buffer;
}  // namespace cpo

using namespace cpo;

/**
 * Check that a type can be converted to a single buffer using as_buffer
 */
template <typename T>
concept as_buffer_convertible = requires(T&& val) {
    as_buffer(NEO_FWD(val));
};

/**
 * Determine the type of buffer that will be returned by `as_buffer` if given an
 * argument of the given type.
 */
template <as_buffer_convertible T>
using as_buffer_t = decltype(as_buffer(std::declval<T>()));

/**
 * Create a buffer that views the representation of a buffer_safe object.
 */
template <buffer_safe_cvr T>
constexpr auto trivial_buffer(T&& what) noexcept {
    return as_buffer(byte_pointer(std::addressof(what)), sizeof what);
}

template <typename T>
concept buffer_range_convertible = buffer_range<T> || as_buffer_convertible<T>;

template <buffer_range_convertible T>
constexpr decltype(auto) ensure_buffer_range(T&& t) noexcept {
    if constexpr (buffer_range<T>) {
        return T(NEO_FWD(t));
    } else {
        return as_buffer(NEO_FWD(t));
    }
}

}  // namespace neo
