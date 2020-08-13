#pragma once

#include <neo/detail/buffer_base.hpp>
#include <neo/detail/single_buffer_iter.hpp>
#include <neo/trivial_range.hpp>

#include <cstddef>
#include <iterator>
#include <type_traits>

namespace neo {

class mutable_buffer : public detail::buffer_base<std::byte*, mutable_buffer> {
public:
    using buffer_base::buffer_base;

    /**
     * Create an instance of the buffer from the given container of mutable data
     */
    template <mutable_trivial_range C>
    explicit constexpr mutable_buffer(C&& c) noexcept
        : buffer_base(byte_pointer(std::data(c)), trivial_range_byte_size(c)) {}

    template <mutable_buffer_constructible T>
    explicit constexpr operator T() const noexcept {
        return T(static_cast<data_pointer_t<T>>(static_cast<void*>(data())),
                 size() / data_type_size_v<T>);
    }
};

}  // namespace neo
