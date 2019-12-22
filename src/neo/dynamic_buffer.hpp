#pragma once

#include <neo/buffer_concepts.hpp>
#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <neo/concepts.hpp>

#include <cstdlib>

namespace neo {

struct proto_dynamic_buffer {
    proto_dynamic_buffer() = delete;

    using const_buffers_type   = proto_const_buffer_sequence;
    using mutable_buffers_type = proto_mutable_buffer_sequence;

    std::size_t size() const;
    std::size_t max_size() const;
    std::size_t capacity() const;

    const_buffers_type data(std::size_t, std::size_t) const;
    mutable_buffers_type data(std::size_t, std::size_t);

    mutable_buffers_type grow(std::size_t);

    void shrink(std::size_t);
    void consume(std::size_t);
};

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

static_assert(dynamic_buffer<proto_dynamic_buffer>);

// clang-format on

}  // namespace neo
