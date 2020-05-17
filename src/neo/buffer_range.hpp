#pragma once

#include <neo/buffer_iterator.hpp>
#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <neo/concepts.hpp>
#include <neo/fwd.hpp>
#include <neo/iterator_concepts.hpp>
#include <neo/platform.hpp>
#include <neo/ref.hpp>

#include <iterator>
#include <type_traits>
#include <utility>

namespace neo {

// clang-format off
template <typename T>
concept buffer_range =
    requires(T&& t) {
        { std::begin(NEO_FWD(t)) } -> buffer_iterator;
        { std::end(NEO_FWD(t)) } -> sentinel_for<decltype(std::begin(NEO_FWD(t)))>;
    };

template <buffer_range T>
using buffer_range_iterator_t = decltype(std::begin(ref_v<T>));

template <buffer_range T>
using buffer_range_sentinel_t = decltype(std::end(ref_v<T>));

template <buffer_range T>
using buffer_range_value_t = iter_value_t<buffer_range_iterator_t<T>>;

template <typename T>
concept mutable_buffer_range =
    buffer_range<T> &&
    mutable_buffer_iterator<buffer_range_iterator_t<T>>;

template <typename T>
concept single_const_buffer = buffer_range<T> && alike<T, const_buffer>;

template <typename T>
concept single_mutable_buffer = mutable_buffer_range<T> && alike<T, mutable_buffer>;

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

NEO_TEST_CONCEPT(buffer_range, proto_buffer_range);
NEO_TEST_CONCEPT(mutable_buffer_range, proto_mutable_buffer_range);

static_assert(same_as<buffer_range_value_t<proto_buffer_range>, const_buffer>);
static_assert(same_as<buffer_range_value_t<proto_mutable_buffer_range>, mutable_buffer>);

}  // namespace neo
