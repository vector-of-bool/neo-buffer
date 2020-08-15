#pragma once

#include <neo/buffer_range.hpp>

#include <cstddef>

namespace neo {

/**
 * Return the number of individual buffers in the given buffer sequence
 */
template <buffer_range Seq>
constexpr std::size_t buffer_count(const Seq& seq) noexcept {
    auto it   = std::begin(seq);
    auto stop = std::end(seq);
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
