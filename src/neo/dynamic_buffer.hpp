#pragma once

#include <neo/buffer_concepts.hpp>
#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <neo/concepts.hpp>

namespace neo {

// clang-format off

template <typename DynBuf>
concept dynamic_buffer = requires(DynBuf buf,
                                  const DynBuf cbuf,
                                  std::size_t position,
                                  std::size_t size) {
    requires const_buffer_sequence<typename DynBuf::const_buffers_type>;
    requires mutable_buffer_sequence<typename DynBuf::mutable_buffers_type>;
    { cbuf.size() } -> same_as<std::size_t>;
    { cbuf.max_size() } -> same_as<std::size_t>;
    { cbuf.capacity() } -> same_as<std::size_t>;
    { cbuf.data(position, size) } -> same_as<typename DynBuf::const_buffers_type>;
    { buf.data(position, size) } -> same_as<typename DynBuf::mutable_buffers_type>;
    { buf.grow(size) } -> same_as<typename DynBuf::mutable_buffers_type>;
    buf.shrink(size);
    buf.consume(size);
};

// clang-format on

}  // namespace neo
