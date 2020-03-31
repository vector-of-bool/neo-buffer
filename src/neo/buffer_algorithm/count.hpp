#pragma once

#include <neo/buffer_concepts.hpp>

#include <algorithm>
#include <cstddef>

namespace neo {

/**
 * Return the number of individual buffers in the given buffer sequence
 */
template <const_buffer_sequence Seq>
constexpr std::size_t buffer_count(const Seq& seq) noexcept {
    return std::distance(buffer_sequence_begin(seq), buffer_sequence_end(seq));
}

}  // namespace neo
