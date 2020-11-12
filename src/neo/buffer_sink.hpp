#pragma once

#include <neo/buffer_range.hpp>
#include <neo/buffers_consumer.hpp>

namespace neo {

// clang-format off
template <typename T>
concept buffer_sink =
    requires (T sink, std::size_t size) {
        { sink.prepare(size) } -> mutable_buffer_range;
        sink.commit(size);
    };
// clang-format on

struct proto_buffer_sink {
    proto_buffer_sink()  = delete;
    ~proto_buffer_sink() = delete;
    void operator=(proto_buffer_sink) = delete;

    proto_mutable_buffer_range prepare(std::size_t);
    void                       commit(std::size_t);
};

template <typename T>
concept buffer_output = mutable_buffer_range<T> || buffer_sink<T>;

template <buffer_output Out>
constexpr decltype(auto) ensure_buffer_sink(Out&& out) noexcept {
    if constexpr (buffer_sink<Out>) {
        return Out(NEO_FWD(out));
    } else {
        return buffers_consumer(NEO_FWD(out));
    }
}

template <typename T>
constexpr bool noexcept_buffer_output_v;

template <buffer_output T>
constexpr bool noexcept_buffer_output_v<T> =                          //
    noexcept(ensure_buffer_sink(ref_v<T>).commit(std::size_t(1))) &&  //
    noexcept(ensure_buffer_sink(ref_v<T>).prepare(std::size_t(1)));

}  // namespace neo
