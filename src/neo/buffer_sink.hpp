#pragma once

#include <neo/buffer_range.hpp>

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

}  // namespace neo
