#pragma once

#include <neo/buffer_range.hpp>

#include <neo/iterator_facade.hpp>

namespace neo {

/**
 * @brief Convert a regular buffer range into a buffer range of individual bytes
 *
 * This type is intented to be used for testing purposes as it exhibits worst-case
 * behavior of buffer_range wherein the buffer emits *single bytes* at a time. The
 * range iterator type is only a forward_iterator.
 *
 * @tparam Buf The underlying buffer type.
 */
template <single_buffer Buf>
class pathological_buffer_range {
    Buf _buf;

public:
    using buffer_type = Buf;
    using pointer     = typename buffer_type::pointer;

    constexpr pathological_buffer_range() = default;
    constexpr pathological_buffer_range(buffer_type b)
        : _buf(b) {}

    struct iterator : neo::iterator_facade<iterator> {
        pointer _ptr = nullptr;
        iterator()   = default;
        iterator(pointer p)
            : _ptr(p) {}

        constexpr buffer_type dereference() const noexcept { return buffer_type(_ptr, 1); }
        constexpr void        increment() noexcept { ++_ptr; }
        constexpr bool        operator==(iterator o) const noexcept { return _ptr == o._ptr; }
    };

    constexpr auto begin() const noexcept { return iterator(_buf.data()); }
    constexpr auto end() const noexcept { return iterator(_buf.data_end()); }
};

}  // namespace neo
