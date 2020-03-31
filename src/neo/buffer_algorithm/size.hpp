#pragma once

#include <neo/buffer_concepts.hpp>
#include <neo/buffer_seq_iter.hpp>

#include <cstdlib>

namespace neo {

/**
 * Obtain the length of a buffer sequence, in bytes.
 */
template <const_buffer_sequence Seq>
constexpr std::size_t buffer_size(const Seq& seq) noexcept {
    auto        iter = buffer_sequence_begin(seq);
    const auto  stop = buffer_sequence_end(seq);
    std::size_t size = 0;
    while (iter != stop) {
        size += static_cast<std::size_t>((*iter).size());
        ++iter;
    }
    return size;
}

constexpr std::size_t buffer_size(const_buffer b) noexcept { return b.size(); }
constexpr std::size_t buffer_size(mutable_buffer b) noexcept { return b.size(); }

}  // namespace neo
