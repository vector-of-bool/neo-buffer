#pragma once

#include <neo/buffer_concepts.hpp>

#include <cstddef>

namespace neo {

/**
 * Return the number of individual buffers in the given buffer sequence
 */
template <const_buffer_sequence Seq>
constexpr std::size_t buffer_count(const Seq& seq) noexcept {
    auto it   = buffer_sequence_begin(seq);
    auto stop = buffer_sequence_end(seq);
    if constexpr (random_access_iterator<decltype(it)>) {
        return stop - it;
    } else {
        std::size_t count = 0;
        for (; it != stop; ++it) {
            ++count;
        }
        return count;
    }
}

}  // namespace neo
