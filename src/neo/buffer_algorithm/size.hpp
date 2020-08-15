#pragma once

#include <neo/buffer_range.hpp>

#include <cstdlib>

namespace neo {

/**
 * Obtain the length of a buffer sequence, in bytes.
 */
template <buffer_range Seq>
constexpr std::size_t buffer_size(const Seq& seq) noexcept {
    std::size_t size = 0;
    for (const_buffer b : seq) {
        size += b.size();
    }
    return size;
}

constexpr std::size_t buffer_size(const_buffer b) noexcept { return b.size(); }
constexpr std::size_t buffer_size(mutable_buffer b) noexcept { return b.size(); }

}  // namespace neo
