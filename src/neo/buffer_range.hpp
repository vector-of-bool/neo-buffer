#pragma once

#include <neo/buffer_iterator.hpp>
#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <neo/concepts.hpp>

#include <ranges>

namespace neo {

// clang-format off
/**
 * A range whose iterators model buffer_iterator. That is: The range's value
 * type is convertible to `const_buffer`.
 */
template <typename T>
concept buffer_range =
    std::ranges::input_range<T> &&
    buffer_iterator<std::ranges::iterator_t<T>>;

/**
 * Obtain the iterator type of the given buffer range
 */
template <buffer_range T>
using buffer_range_iterator_t = std::ranges::iterator_t<T>;

/**
 * Obtain the sentinel type of the given buffer range
 */
template <buffer_range T>
using buffer_range_sentinel_t = std::ranges::sentinel_t<T>;

/**
 * Obtain the buffer type (value type) of the given buffer range.
 */
template <buffer_range T>
using buffer_range_value_t = std::ranges::range_value_t<T>;

/**
 * A range whose iterators model mutable_buffer_iterator. That is: The range's
 * value type is exactly `mutable_buffer`.
 */
template <typename T>
concept mutable_buffer_range =
    buffer_range<T> &&
    mutable_buffer_iterator<buffer_range_iterator_t<T>>;

/**
 * A buffer range that is exactly a single const_buffer object, rather than a
 * range.
 */
template <typename T>
concept single_const_buffer = buffer_range<T> && alike<T, const_buffer>;

/**
 * A buffer range that is exactly a single mutable_buffer object, rather than a
 * real range.
 */
template <typename T>
concept single_mutable_buffer = mutable_buffer_range<T> && alike<T, mutable_buffer>;

/**
 * A buffer range that is either a single mutable_buffer or a const_buffer rather
 * than a real range.
 */
template <typename T>
concept single_buffer = single_const_buffer<T> || single_mutable_buffer<T>;
// clang-format on

struct proto_buffer_range {
    proto_buffer_range() = delete;

    proto_buffer_iterator begin() const;
    proto_buffer_iterator end() const;
};

struct proto_mutable_buffer_range {
    proto_mutable_buffer_range() = delete;

    proto_mutable_buffer_iterator begin() const;
    proto_mutable_buffer_iterator end() const;
};

}  // namespace neo
