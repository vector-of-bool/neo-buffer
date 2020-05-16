#pragma once

#include <neo/data_container_concepts.hpp>
#include <neo/detail/buffer_base.hpp>
#include <neo/detail/single_buffer_iter.hpp>

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
    template <mutable_data_container C>
    explicit constexpr mutable_buffer(C&& c) noexcept
        : buffer_base(byte_pointer(std::data(c)), data_container_byte_size(c)) {}
};

}  // namespace neo
