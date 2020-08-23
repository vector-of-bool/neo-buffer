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

template <buffer_range R>
constexpr bool buffer_size_at_least(R&& r, std::size_t min) noexcept {
    std::size_t size = 0;
    for (const_buffer b : r) {
        size += b.size();
        if (size >= min) {
            return true;
        }
    }
    return false;
}

template <buffer_range R>
constexpr bool buffer_is_empty(R&& r) noexcept {
    for (const_buffer b : r) {
        if (b) {
            return false;
        }
    }
    return true;
}

}  // namespace neo
