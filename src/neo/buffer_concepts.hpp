#pragma once

#include <neo/buffer_seq_iter.hpp>
#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <iterator>

namespace neo {

template <typename T>
NEO_CONCEPT const_buffer_sequence = requires(T seq) {
    { neo::buffer_sequence_begin(seq) }
    ->const_buffer_sequence_iterator;
    neo::buffer_sequence_end(seq);
    {bool(neo::buffer_sequence_begin(seq) != neo::buffer_sequence_end(seq))};
};

template <typename T>
NEO_CONCEPT mutable_buffer_sequence = const_buffer_sequence<T>&& requires(T seq) {
    { neo::buffer_sequence_begin(seq) }
    ->mutable_buffer_sequence_iterator;
};

}  // namespace neo