#pragma once

#include <neo/buffer_seq_iter.hpp>
#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <neo/concepts.hpp>
#include <neo/iterator_concepts.hpp>

#include <iterator>

namespace neo {

struct proto_const_buffer_sequence {
    proto_const_buffer_sequence_iterator buffer_sequence_end() const;
    proto_const_buffer_sequence_iterator buffer_sequence_begin() const;
};

struct proto_mutable_buffer_sequence {
    proto_mutable_buffer_sequence_iterator buffer_sequence_end() const;
    proto_mutable_buffer_sequence_iterator buffer_sequence_begin() const;
};

// clang-format off

template <typename T>
    requires requires (T&& t) {
        buffer_sequence_begin(NEO_FWD(t));
    }
using buffer_sequence_iterator_t = decltype(buffer_sequence_begin(std::declval<T&>()));

template <typename T>
    requires requires (T&& t) {
        buffer_sequence_end(NEO_FWD(t));
    }
using buffer_sequence_sentinel_t = decltype(buffer_sequence_end(std::declval<T&>()));

template <typename T>
concept const_buffer_sequence =
    requires {
        typename buffer_sequence_iterator_t<T>;
        typename buffer_sequence_sentinel_t<T>;
    } &&
    neo::sentinel_for<
        buffer_sequence_sentinel_t<T>,
        buffer_sequence_iterator_t<T>> &&
    // The iterator type must be a valid const_buffer iterator
    const_buffer_sequence_iterator<buffer_sequence_iterator_t<T>>;

static_assert(const_buffer_sequence<proto_const_buffer_sequence>);
static_assert(same_as<buffer_sequence_iterator_t<proto_const_buffer_sequence>, proto_const_buffer_sequence_iterator>);

template <typename T>
concept mutable_buffer_sequence =
    // Any mutable buffer sequence is also a const sequence:
    const_buffer_sequence<T> &&
    // The iterator must also be a mutable buffer sequence iterator, though:
    mutable_buffer_sequence_iterator<buffer_sequence_iterator_t<T>>;

static_assert(mutable_buffer_sequence<proto_mutable_buffer_sequence>);

// clang-format on

}  // namespace neo
