#pragma once

#include <neo/byte_pointer.hpp>
#include <neo/detail/buffer_base.hpp>
#include <neo/detail/single_buffer_iter.hpp>
#include <neo/mutable_buffer.hpp>

#include <neo/concepts.hpp>

#include <cstddef>

namespace neo {

/**
 * A type that represents a view to a readonly segment of contiguous memory.
 */
class const_buffer : public detail::buffer_base<const std::byte*, const_buffer> {
public:
    using buffer_base::buffer_base;

    /**
     * Implict conversion of a mutable buffer to a read-only buffer.
     */
    constexpr const_buffer(mutable_buffer buf) noexcept
        : buffer_base(buf.data(), buf.size()) {}

    /**
     * Special constructor that will catch character arrays. This overload will
     * subtract one from the array size to ignore the null-terminator.
     */
    template <std::size_t N>
    explicit constexpr const_buffer(const char (&arr)[N]) noexcept
        : buffer_base(byte_pointer(arr), N - 1) {}

    /**
     * Construct a buffer view from a data container.
     */
    template <trivial_range C>
    explicit constexpr const_buffer(const C& c) noexcept
        : buffer_base(byte_pointer(std::data(c)), trivial_range_byte_size(c)) {}

    template <const_buffer_constructible T>
    explicit constexpr operator T() const noexcept {
        return T(static_cast<data_pointer_to_const_t<T>>((const void*)data()),
                 size() / data_type_size_v<T>);
    }
};

inline namespace literals {
inline namespace buffer_literals {
constexpr const_buffer operator""_buf(const char* first, std::size_t count) noexcept {
    return const_buffer(byte_pointer(first), count);
}
}  // namespace buffer_literals
}  // namespace literals

}  // namespace neo