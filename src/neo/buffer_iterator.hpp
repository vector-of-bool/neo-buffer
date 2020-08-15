#pragma once

#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <neo/concepts.hpp>
#include <neo/iterator_concepts.hpp>

#include <iterator>

namespace neo {

// clang-format off
/**
 * Any input_iterator whose value type is convertible to const_buffer
 */
template <typename T>
concept buffer_iterator =
    input_iterator<T> &&
    convertible_to<iter_value_t<T>, const_buffer>;

/**
 * Any input_iterator whose value type is exactly mutable_buffer
 */
template <typename T>
concept mutable_buffer_iterator =
    buffer_iterator<T> &&
    same_as<iter_value_t<T>, mutable_buffer>;
// clang-format on

/**
 * Prototype buffer_iterator
 */
struct proto_buffer_iterator {
    using iterator_concept  = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using difference_type   = int;
    using value_type        = const_buffer;
    using reference         = const_buffer&;
    using pointer           = const_buffer*;
    proto_buffer_iterator& operator++();
    proto_buffer_iterator  operator++(int);
    const_buffer           operator*() const;

    bool operator!=(proto_buffer_iterator) const noexcept;
    bool operator==(proto_buffer_iterator) const noexcept;
};

/**
 * Prototype mutable_buffer_iterator
 */
struct proto_mutable_buffer_iterator {
    using iterator_concept  = std::input_iterator_tag;
    using iterator_category = std::input_iterator_tag;
    using difference_type   = int;
    using value_type        = mutable_buffer;
    using reference         = mutable_buffer&;
    using pointer           = mutable_buffer*;
    proto_mutable_buffer_iterator& operator++();
    proto_mutable_buffer_iterator  operator++(int);
    mutable_buffer                 operator*() const;

    bool operator!=(proto_mutable_buffer_iterator) const noexcept;
    bool operator==(proto_mutable_buffer_iterator) const noexcept;
};

}  // namespace neo
