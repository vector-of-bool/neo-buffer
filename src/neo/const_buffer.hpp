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
    template <data_container C>
    explicit constexpr const_buffer(const C& c) noexcept
        : buffer_base(byte_pointer(std::data(c)), data_container_byte_size(c)) {}
};

}  // namespace neo