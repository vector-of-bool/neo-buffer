#pragma once

#include <neo/buffer_range.hpp>
#include <neo/buffer_sink.hpp>
#include <neo/buffer_source.hpp>
#include <neo/buffers_consumer.hpp>

#include <neo/concepts.hpp>

#include "./size.hpp"

#include <algorithm>
#include <cstddef>

namespace neo {

/**
 * Low-level buffer copier that copies from the beginning to the end.
 * `dest` and `src` must have the same size `s`!
 */
constexpr void
ll_buffer_copy_forward(std::byte* dest, const std::byte* src, std::size_t s) noexcept {
    for (; s; --s) {
        *dest++ = *src++;
    }
}

/**
 * Low-level buffer copier that copies from the end to the beginning.
 * `dest` and `src` must have the same size `s`!
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
 * `src` are disjoint. Behavior is undefined otherwise.
 * `dest` and `src` must have the same size!
 */
constexpr void ll_buffer_copy_fast(std::byte* dest, const std::byte* src, std::size_t s) noexcept {
    /// XXX: Improve with memcpy() when we have is_constant_evaluated()
    ll_buffer_copy_forward(dest, src, s);
}

/**
 * Low-level buffer copier that copies buffers in a way that provides intuitive
 * results in the case of overlap.
 * `dest` and `src` must have the same size!
 */
constexpr void ll_buffer_copy_safe(std::byte* dest, const std::byte* src, std::size_t s) noexcept {
    if (std::less<>{}(dest, src)) {
        ll_buffer_copy_forward(dest, src, s);
    } else {
        ll_buffer_copy_backward(dest, src, s);
    }
}

/**
 * Models a function that takes two pointers to std::byte arrays, the first being
 * mutable, and a size, and copies the contents from the second array into the first
 * array, assuming both arrays have the same size.
 */
template <typename T>
concept ll_buffer_copy_fn = neo::invocable<T, std::byte*, const std::byte*, std::size_t>;

/**
 * Copy data from the source buffer into the destination buffer, with a maximum of `max_copy`. The
 * actual number of bytes that are copied is the minimum of the buffer sizes and `max_copy`. The
 * number of bytes copied is returned. The `copy` parameter is the low-level buffer copying
 * function.
 */
template <ll_buffer_copy_fn Copy>
constexpr std::size_t
buffer_copy(mutable_buffer dest, const_buffer src, std::size_t max_copy, Copy&& copy) noexcept {
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
buffer_copy(mutable_buffer dest, mutable_buffer src, std::size_t max_copy, Copy&& copy) noexcept {
    return buffer_copy(dest, const_buffer(src), max_copy, copy);
}

// clang-format off
/**
 * Copy data from `src` into `dest`. `src` may be a buffer-range or a buffer-source,
 * and `dest` may be a buffer-range or buffer-sink. At most `max_copy` bytes will
 * be copied. The operation is bounds-checked, and the number of bytes copied is
 * returned.
 */
template <buffer_output Dest, buffer_input Source, ll_buffer_copy_fn Copy>
constexpr std::size_t
buffer_copy(Dest&& dest, Source&& src, std::size_t max_copy, Copy&& copy)
    noexcept(noexcept_buffer_output_v<Dest> && noexcept_buffer_input_v<Source>)
{
    // clang-format on
    auto remaining = max_copy;

    auto&& out = ensure_buffer_sink(dest);
    auto&& in  = ensure_buffer_source(src);

    while (remaining != 0) {
        auto in_part  = in.next(remaining);
        auto out_part = out.prepare(buffer_size(in_part));
        auto n_copied = buffer_copy(out_part, in_part, remaining, copy);
        if (n_copied == 0) {
            break;
        }
        in.consume(n_copied);
        out.commit(n_copied);
        remaining -= n_copied;
    }

    return max_copy - remaining;
}

// clang-format off
template <typename Dest, typename Source>
constexpr std::size_t buffer_copy(Dest&& dest, Source&& source, std::size_t max_copy)
    noexcept(noexcept(buffer_copy(dest, source, max_copy, ll_buffer_copy_safe)))
    requires requires { buffer_copy(dest, source, max_copy, ll_buffer_copy_safe); }
{
    return buffer_copy(dest, source, max_copy, ll_buffer_copy_safe);
}

template <typename Dest, typename Source, ll_buffer_copy_fn Copy>
constexpr std::size_t buffer_copy(Dest&& dest, Source&& source, Copy&& copy)
    noexcept(noexcept(buffer_copy(dest, source, std::size_t(), copy)))
    requires requires(std::size_t s) { buffer_copy(dest, source, s, copy); }
{
    return buffer_copy(dest, source, std::numeric_limits<std::size_t>::max(), copy);
}

template <typename Dest, typename Source>
constexpr std::size_t buffer_copy(Dest&& dest, Source&& source)
    noexcept(noexcept(buffer_copy(dest, source, std::size_t(), ll_buffer_copy_safe)))
    requires requires(std::size_t s) { buffer_copy(dest, source, s, ll_buffer_copy_safe); }
{
    return buffer_copy(dest, source, ll_buffer_copy_safe);
}
// clang-format on

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

namespace detail {

struct default_ll_copy {
    constexpr void operator()(std::byte* dest, const std::byte* src, std::size_t s) const noexcept {
        ll_buffer_copy_safe(dest, src, s);
    }
};

}  // namespace detail

template <ll_buffer_copy_fn Copy = detail::default_ll_copy>
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
