#pragma once

#include <neo/byte_pointer.hpp>

#include <neo/concepts.hpp>
#include <neo/iterator_concepts.hpp>

#include <iterator>
#include <type_traits>

namespace neo {

// clang-format off

/**
 * Get the pointer type returned by the .data() method of the given container.
 */
template <typename Container>
using data_pointer_t = decltype(std::data(std::declval<Container&>()));

/**
 * Get the pointer type returned by the .data() method of the given container
 * if it is a `const` instance of said container.
 */
template <typename Container>
using const_data_pointer_t = decltype(std::data(std::as_const(std::declval<Container&>())));

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
concept data_container =
    buffer_safe<data_type_t<C>> &&
    requires (const C& c, data_pointer_t<C> ptr) {
        { neo::byte_pointer(ptr) } -> convertible_to<const std::byte*>;
        { std::size(c) } -> convertible_to<std::size_t>;
    };

/**
 * Prototype model of `data_container`
 */
struct proto_data_container {
    const int*  data() const;
    std::size_t size() const;
};
static_assert(data_container<proto_data_container>);

/**
 * Model a data_container that can be used to write-thru to the underlying data.
 */
template <typename C>
concept mutable_data_container =
    data_container<C> &&
    requires (data_pointer_t<C> ptr) {
        { neo::byte_pointer(ptr) } -> same_as<std::byte*>;
    };

/**
 * Prototype model of `mutable_data_container`
 */
struct proto_mutable_data_container {
    int*        data();
    const int*  data() const;
    std::size_t size() const;
};
static_assert(mutable_data_container<proto_mutable_data_container>);

/**
 * Given a data container, return the size of the data referenced by that
 * container as a number of bytes.
 */
template <data_container C>
constexpr std::size_t data_container_byte_size(const C& c) noexcept {
    return std::size(c) * data_type_size_v<C>;
}

/**
 * Models a type that is a data container that can be constructed from a
 * (possibly-const) pointer to data and a size of that data.
 */
template <typename C>
concept const_buffer_constructible =
    data_container<C> &&
    neo::constructible_from<C, const_data_pointer_t<C>, std::size_t>;

/**
 * Models a type that is a data container and can be constructed from a pointer
 * to that data and the size of the data.
 */
template <typename C>
concept mutable_buffer_constructible =
    data_container<C> &&
    neo::constructible_from<C, data_pointer_t<C>, std::size_t>;
// clang-format on

}  // namespace neo
