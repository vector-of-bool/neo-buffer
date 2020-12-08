#pragma once

#include <neo/buffer_range.hpp>
#include <neo/buffers_consumer.hpp>

#include <neo/fwd.hpp>
#include <neo/ref.hpp>
#include <neo/returns.hpp>

namespace neo {

/**
 * A "buffer_source" is an object that allows the user to read buffers from an
 * abstract source, and notify that source when it has consumed some input.
 */
// clang-format off
template <typename T>
concept buffer_source =
    requires (T source, std::size_t size) {
        { source.next(size) } -> buffer_range;
        { source.consume(size) } noexcept;
    };
// clang-format on

struct proto_buffer_source {
    proto_buffer_source() = delete;

    proto_buffer_range next(std::size_t);
    void               consume(std::size_t) noexcept;
};

template <buffer_source T>
constexpr std::size_t buffer_source_next_size_hint_v = 1024 * 4;

template <typename T>
concept buffer_input = buffer_range<T> || buffer_source<T>;

template <buffer_input B>
constexpr decltype(auto) ensure_buffer_source(B&& b) noexcept {
    if constexpr (buffer_source<B>) {
        return B(NEO_FWD(b));
    } else {
        return buffers_consumer(NEO_FWD(b));
    }
}

template <typename T>
constexpr bool noexcept_buffer_input_v;

template <buffer_input T>
constexpr bool
    noexcept_buffer_input_v<T> = noexcept(ensure_buffer_source(ref_v<T>).next(std::size_t(1)));

}  // namespace neo
