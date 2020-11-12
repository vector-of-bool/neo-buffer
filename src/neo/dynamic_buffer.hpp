#pragma once

#include <neo/buffer_range.hpp>

#include <neo/concepts.hpp>

#include <cstddef>
#include <limits>

namespace neo {

struct proto_dynamic_buffer {
    proto_dynamic_buffer() = delete;

    std::size_t size() const noexcept;
    std::size_t max_size() const noexcept;
    std::size_t capacity() const noexcept;

    proto_buffer_range         data(std::size_t, std::size_t) const noexcept;
    proto_mutable_buffer_range data(std::size_t, std::size_t) noexcept;

    proto_mutable_buffer_range grow(std::size_t);

    void shrink(std::size_t) noexcept;
    void consume(std::size_t) noexcept;
};

// clang-format off

template <typename DynBuf>
concept dynamic_buffer = requires(DynBuf buf,
                                  const std::remove_reference_t<DynBuf> cbuf,
                                  std::size_t position,
                                  std::size_t size) {
    { cbuf.size() } noexcept -> same_as<std::size_t>;
    { cbuf.max_size() }noexcept -> same_as<std::size_t>;
    { cbuf.capacity() } noexcept-> same_as<std::size_t>;
    { cbuf.data(position, size) }noexcept -> buffer_range;
    { buf.data(position, size) } noexcept-> mutable_buffer_range;
    { buf.grow(size) } -> mutable_buffer_range;
    { buf.shrink(size) } noexcept;
    { buf.consume(size) } noexcept;
};

// clang-format on

template <dynamic_buffer Buf>
constexpr std::size_t dynbuf_safe_grow_size(Buf&&       b,
                                            std::size_t want
                                            = std::numeric_limits<std::size_t>::max()) noexcept {
    auto max = b.max_size() - b.size();
    return max < want ? max : want;
}

/**
 * Grow the given dynamic buffer by up-to `want_grow` bytes. If `want_grow` would
 * resize `b` beyond its max_size(), then `b` wiill be grown to its max_size().
 * Returns the grown buffer area.
 */
template <dynamic_buffer Buf>
constexpr decltype(auto)
dynbuf_safe_grow(Buf&& b, std::size_t want_grow) noexcept(noexcept(b.grow(want_grow))) {
    const auto grow_avail = dynbuf_safe_grow_size(b);
    const auto min_grow   = grow_avail < want_grow ? grow_avail : want_grow;
    return b.grow(min_grow);
}

template <dynamic_buffer Buf>
constexpr void dynbuf_clear(Buf&& b) noexcept {
    b.shrink(b.size());
}

template <dynamic_buffer Buf>
constexpr void dynbuf_resize(Buf&& b, std::size_t new_size) noexcept(noexcept(b.grow(new_size))) {
    const auto cur_size = b.size();
    if (cur_size < new_size) {
        b.grow(new_size - cur_size);
    } else {
        b.shrink(cur_size - new_size);
    }
}

}  // namespace neo
