#pragma once

#include <neo/byte_pointer.hpp>

#include <neo/concepts.hpp>
#include <neo/iterator_concepts.hpp>
#include <neo/ref.hpp>

#include <iterator>
#include <type_traits>

namespace neo {

// clang-format off

/**
 * Get the pointer type returned by the std::data of the given container.
 */
template <typename C>
using data_pointer_t = decltype(std::data(ref_v<C>));

/**
 * Get the pointer type returned by the std::data of the given container
 * if it is a `const` instance of said container.
 */
template <typename C>
using const_data_pointer_t = decltype(std::data(cref_v<C>));

/**
 * Get the pointer-to-const that corresponds to the data_pointer_t of the given container
 */
template <typename Container>
using data_pointer_to_const_t = std::add_pointer_t<std::add_const_t<std::remove_pointer_t<data_pointer_t<Container>>>>;

/**
 * Get the value type of the container.
 */
template <typename C>
using data_type_t = iter_value_t<data_pointer_t<C>>;

/**
 * Get the size of the value type of the contaienr.
 */
template <typename C>
constexpr auto data_type_size_v = sizeof(data_type_t<C>);

/**
 * Model a container/view which affords a pointer to the underlying data along
 * with a size of that data.
 */
template <typename C>
concept trivial_range =
    buffer_safe<data_type_t<C>> &&
    requires (const C& c, data_pointer_t<C> ptr) {
        { neo::byte_pointer(ptr) } -> convertible_to<const std::byte*>;
        { std::size(c) } -> convertible_to<std::size_t>;
    };

/**
 * Prototype model of `trivial_range`
 */
struct proto_trivial_range {
    const int*  data() const;
    std::size_t size() const;
};

/**
 * Model a trivial_range that can be used to write-thru to the underlying data.
 */
template <typename C>
concept mutable_trivial_range =
    trivial_range<C> &&
    requires (data_pointer_t<C> ptr) {
        { neo::byte_pointer(ptr) } -> same_as<std::byte*>;
    };

/**
 * Prototype model of `mutable_trivial_range`
 */
struct proto_mutable_trivial_range {
    int*        data();
    const int*  data() const;
    std::size_t size() const;
};

/**
 * Given a data container, return the size of the data referenced by that
 * container as a number of bytes.
 */
template <trivial_range C>
constexpr std::size_t trivial_range_byte_size(const C& c) noexcept {
    return std::size(c) * data_type_size_v<C>;
}

/**
 * Models a type that is a data container and can be constructed from a pointer
 * to that data and the size of the data.
 */
template <typename C>
concept mutable_buffer_constructible =
    trivial_range<C> &&
    neo::constructible_from<C, data_pointer_t<C>, std::size_t>;

/**
 * Models a type that is a data container that can be constructed from a
 * pointer-to-const to data and a size of that data.
 */
template <typename C>
concept const_buffer_constructible =
    mutable_buffer_constructible<C> &&
    neo::constructible_from<C, data_pointer_to_const_t<C>, std::size_t>;
// clang-format on

}  // namespace neo
