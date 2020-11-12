#pragma once

#include <neo/buffer_range.hpp>

#include <cstddef>

#include <neo/assert.hpp>

namespace neo {

/**
 * A `static_buffer_vector` represents a dynamically-sized array of buffers with
 * a fixed maximum size. Buffers can be pushed onto the array up to a limit.
 * This models `buffer_range` and conditionally `mutable_buffer_range` (if `BufferType` is
 * mutable_buffer).
 */
template <typename BufferType, std::size_t MaxBuffers>
struct static_buffer_vector {
    using value_type  = BufferType;
    using buffer_type = value_type;

    using pointer         = buffer_type*;
    using const_pointer   = const buffer_type*;
    using reference       = buffer_type&;
    using const_reference = const buffer_type&;
    using iterator        = pointer;
    using const_iterator  = const_pointer;

    std::size_t active_count = 0;
    buffer_type buffers[MaxBuffers == 0 ? 1 : MaxBuffers];

    constexpr std::size_t size() const noexcept { return active_count; }
    constexpr std::size_t max_size() const noexcept { return MaxBuffers; }

    constexpr const_iterator cbegin() const noexcept { return buffers; }
    constexpr const_iterator begin() const noexcept { return cbegin(); }
    constexpr iterator       begin() noexcept { return buffers; }
    constexpr const_iterator cend() const noexcept { return cbegin() + active_count; }
    constexpr const_iterator end() const noexcept { return cend(); }
    constexpr iterator       end() noexcept { return begin() + active_count; }

    constexpr reference push_back(buffer_type b) noexcept {
        neo_assert(expects,
                   size() < max_size(),
                   "Pushed too many elements into a statically-sized vector",
                   max_size());
        auto& ret = buffers[size()] = b;
        ++active_count;
        return ret;
    }

    constexpr reference operator[](std::size_t idx) noexcept {
        neo_assert(expects, idx < size(), "Index out-of-range", idx, size());
        return buffers[idx];
    }

    constexpr const_reference operator[](std::size_t idx) const noexcept {
        neo_assert(expects, idx < size(), "Index out-of-range", idx, size());
        return buffers[idx];
    }
};

}  // namespace neo