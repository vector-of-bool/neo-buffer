#pragma once

#include <neo/buffer_concepts.hpp>
#include <neo/buffer_sequence_consumer.hpp>

#include <neo/concepts.hpp>

#include "./size.hpp"

#include <algorithm>
#include <cstddef>

namespace neo {

/**
 * Low-level buffer copier that copies from the beginning to the end.
 * `dest` and `src` must have the same size()!
 */
constexpr void
ll_buffer_copy_forward(std::byte* dest, const std::byte* src, std::size_t s) noexcept {
    for (; s; --s) {
        *dest++ = *src++;
    }
}

/**
 * Low-level buffer copier that copies from the end to the beginning
 * `dest` and `src` must have the same size()!
 */
constexpr void
ll_buffer_copy_backward(std::byte* dest, const std::byte* src, std::size_t s) noexcept {
    dest = dest + s;
    src  = src + s;
    for (; s; --s) {
        *--dest = *--src;
    }
}

/**
 * Low-level buffer copier that copies buffers in a way that assumes `dest` and
 * `src` are disjoin. Behavior is undefined otherwise.
 * `dest` and `src` must have the same size()!
 */
constexpr void ll_buffer_copy_fast(std::byte* dest, const std::byte* src, std::size_t s) noexcept {
    /// XXX: Improve with memcpy() when we have is_constant_evaluated()
    ll_buffer_copy_forward(dest, src, s);
}

/**
 * Low-level buffer copier that copies buffers in a way that provides intuitive
 * results in the case of overlap.
 * `dest` and `src` must have the same size()!
 */
constexpr void ll_buffer_copy_safe(std::byte* dest, const std::byte* src, std::size_t s) noexcept {
    if (std::less<>{}(dest, src)) {
        ll_buffer_copy_forward(dest, src, s);
    } else {
        ll_buffer_copy_backward(dest, src, s);
    }
}

// clang-format off
template <typename T>
concept ll_buffer_copy_fn =
    neo::invocable<T, std::byte*, const std::byte*, std::size_t>
    && trivially_copyable<T>;
// clang-format on

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
    copy(dest.data(), src.data(), n_to_copy);
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
    return buffer_copy(dest, src, ll_buffer_copy_safe);
}

template <mutable_buffer_sequence Dest, const_buffer_sequence Source>
constexpr std::size_t buffer_copy(Dest&& dest, Source&& src, std::size_t max_copy) noexcept {
    return buffer_copy(dest, src, max_copy, ll_buffer_copy_safe);
}

template <mutable_buffer_sequence Dest, const_buffer_sequence Source, ll_buffer_copy_fn Copy>
constexpr auto buffer_copy(Dest&& dest, Source&& src, Copy copy) noexcept {
    return buffer_copy(dest, src, std::numeric_limits<std::size_t>::max(), copy);
}

struct buffer_copy_transform_result {
    std::size_t bytes_written = 0;
    std::size_t bytes_read    = 0;
    bool        done          = false;

    buffer_copy_transform_result& operator+=(const buffer_copy_transform_result& o) noexcept {
        bytes_written += o.bytes_written;
        bytes_read += o.bytes_read;
        done = done || o.done;
        return *this;
    }
};

template <ll_buffer_copy_fn Copy
          = decltype([](std::byte* dest, const std::byte* src, std::size_t s) {
                ll_buffer_copy_safe(dest, src, s);
            })>
class buffer_copy_transformer {
    [[no_unique_address]] Copy _ll_copy;

public:
    explicit buffer_copy_transformer() = default;
    explicit buffer_copy_transformer(Copy c)
        : _ll_copy(c) {}

    constexpr buffer_copy_transform_result operator()(mutable_buffer dest,
                                                      const_buffer   src) const noexcept {
        auto n_copied = buffer_copy(dest, src, _ll_copy);
        return {n_copied, n_copied, false};
    }
};

}  // namespace neo
