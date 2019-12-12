#pragma once

#include <neo/buffer_seq_iter.hpp>
#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <neo/concepts.hpp>

#include <iterator>

namespace neo {

// clang-format off

template <typename T>
concept const_buffer_sequence = requires(T seq) {
    // Take an end iterator:
    neo::buffer_sequence_end(seq);
    // Get a begin iterator:
    { neo::buffer_sequence_begin(seq) } -> const_buffer_sequence_iterator;
    // Compare with end:
    { neo::buffer_sequence_begin(seq) != neo::buffer_sequence_end(seq) } -> neo::simple_boolean;
};

template <typename T>
concept mutable_buffer_sequence =
    // Any mutable buffer sequence is also a const sequence:
    const_buffer_sequence<T>
    // The iterator must also be a mutable buffer sequence iterator, though:
    && requires(T seq) {
        { neo::buffer_sequence_begin(seq) } -> mutable_buffer_sequence_iterator;
    };

// clang-format on

}  // namespace neo