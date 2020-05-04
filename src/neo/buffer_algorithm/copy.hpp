#pragma once

#include <neo/buffer_concepts.hpp>
#include <neo/buffer_sequence_consumer.hpp>

#include <neo/concepts.hpp>

#include "./size.hpp"

#include <algorithm>
#include <cassert>
#include <cstddef>

namespace neo {

/**
 * Low-level buffer copier that copies from the beginning to the end.
 * `dest` and `src` must have the same size()!
 */
constexpr void ll_buffer_copy_forward(mutable_buffer dest, const_buffer src) noexcept {
    assert(dest.size() == src.size());
    auto in  = src.data();
    auto out = dest.data();
    for (; in != src.data_end(); ++in, ++out) {
        *out = *in;
    }
}

/**
 * Low-level buffer copier that copies from the end to the beginning
 * `dest` and `src` must have the same size()!
 */
constexpr void ll_buffer_copy_backward(mutable_buffer dest, const_buffer src) noexcept {
    assert(dest.size() == src.size());
    auto r_in  = src.data_end();
    auto r_out = dest.data_end();
    while (r_in != src.data()) {
        *--r_out = *--r_in;
    }
}

/**
 * Low-level buffer copier that copies buffers in a way that assumes `dest` and
 * `src` are disjoin. Behavior is undefined otherwise.
 * `dest` and `src` must have the same size()!
 */
constexpr void ll_buffer_copy_fast(mutable_buffer dest, const_buffer src) noexcept {
    /// XXX: Improve with memcpy() when we have is_constant_evaluated()
    ll_buffer_copy_forward(dest, src);
}

/**
 * Low-level buffer copier that copies buffers in a way that provides intuitive
 * results in the case of overlap.
 * `dest` and `src` must have the same size()!
 */
constexpr void buffer_copy_safe(mutable_buffer dest, const_buffer src) noexcept {
    if (std::less<>{}(dest.data(), src.data())) {
        ll_buffer_copy_forward(dest, src);
    } else {
        ll_buffer_copy_backward(dest, src);
    }
}

template <typename T>
concept ll_buffer_copy_fn = neo::invocable<T, mutable_buffer, const_buffer>&& trivially_copyable<T>;

/**
 * Copy data from the source buffer into the destination buffer, with a maximum of `max_copy`. The
 * actual number of bytes that are copied is the minimum of the buffer sizes and `max_copy`. The
 * number of bytes copied is returned. The `copy` parameter is the low-level buffer copying
 * function.
 */
template <ll_buffer_copy_fn Copy>
constexpr std::size_t
buffer_copy(mutable_buffer dest, const_buffer src, std::size_t max_copy, Copy copy) noexcept {
    // Calculate how much we should copy in this operation. It will be the minimum of the buffer
    // sizes and the maximum bytes we want to copy
    const auto n_to_copy = (std::min)(src.size(), (std::min)(dest.size(), max_copy));
    // Do the copy!
    copy(dest.first(n_to_copy), src.first(n_to_copy));
    return n_to_copy;
}

// Catch copying from mutable->mutable and call the overload of const->mutable
template <ll_buffer_copy_fn Copy>
constexpr std::size_t
buffer_copy(mutable_buffer dest, mutable_buffer src, std::size_t max_copy, Copy copy) noexcept {
    return buffer_copy(dest, const_buffer(src), max_copy, copy);
}

/**
 * Copy data from the `src` buffer into the `dest` buffer, up to `max_copy`
 * bytes. The operation is bounds-checked, and the number of bytes copied is
 * returned.
 */
template <mutable_buffer_sequence Dest, const_buffer_sequence Source, ll_buffer_copy_fn Copy>
constexpr std::size_t
buffer_copy(Dest&& dest, Source&& src, std::size_t max_copy, Copy copy) noexcept {
    buffer_sequence_consumer in{src};
    buffer_sequence_consumer out{dest};
    // Keep count of how many bytes remain
    auto        remaining_to_copy = max_copy;
    std::size_t total_copied      = 0;
    while (remaining_to_copy && !in.empty() && !out.empty()) {
        const std::size_t n_copied
            = buffer_copy(out.next_contiguous(), in.next_contiguous(), remaining_to_copy, copy);
        in.consume(n_copied);
        out.consume(n_copied);
        remaining_to_copy -= n_copied;
        total_copied += n_copied;
    }
    return total_copied;
}

/**
 * Copy the data from the `src` buffer sequence into the `dest` buffer sequence.
 * The operation is bounds-checked, and the number of bytes successfully copied
 * is returned.
 *
 * This overload is guaranteed to exhaust at least one of the source or
 * destination buffers.
 */
template <mutable_buffer_sequence Dest, const_buffer_sequence Source>
constexpr std::size_t buffer_copy(Dest&& dest, Source&& src) noexcept {
    return buffer_copy(dest, src, buffer_copy_safe);
}

template <mutable_buffer_sequence Dest, const_buffer_sequence Source>
constexpr std::size_t buffer_copy(Dest&& dest, Source&& src, std::size_t max_copy) noexcept {
    return buffer_copy(dest, src, max_copy, buffer_copy_safe);
}

template <mutable_buffer_sequence Dest, const_buffer_sequence Source, ll_buffer_copy_fn Copy>
constexpr auto buffer_copy(Dest&& dest, Source&& src, Copy copy) noexcept {
    return buffer_copy(dest, src, std::numeric_limits<std::size_t>::max(), copy);
}

}  // namespace neo
