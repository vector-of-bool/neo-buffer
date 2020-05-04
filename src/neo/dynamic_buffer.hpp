#pragma once

#include <neo/buffer_concepts.hpp>
#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <neo/concepts.hpp>

#include <cstdlib>

namespace neo {

struct proto_dynamic_buffer {
    proto_dynamic_buffer() = delete;

    std::size_t size() const;
    std::size_t max_size() const;
    std::size_t capacity() const;

    proto_const_buffer_sequence   data(std::size_t, std::size_t) const;
    proto_mutable_buffer_sequence data(std::size_t, std::size_t);

    proto_mutable_buffer_sequence grow(std::size_t);

    void shrink(std::size_t);
    void consume(std::size_t);
};

// clang-format off

template <typename DynBuf>
concept dynamic_buffer = requires(DynBuf buf,
                                  const DynBuf cbuf,
                                  std::size_t position,
                                  std::size_t size) {
    { cbuf.size() } -> same_as<std::size_t>;
    { cbuf.max_size() } -> same_as<std::size_t>;
    { cbuf.capacity() } -> same_as<std::size_t>;
    { cbuf.data(position, size) } -> const_buffer_sequence;
    { buf.data(position, size) } -> mutable_buffer_sequence;
    { buf.grow(size) } -> mutable_buffer_sequence;
    buf.shrink(size);
    buf.consume(size);
};

static_assert(dynamic_buffer<proto_dynamic_buffer>);

// clang-format on

}  // namespace neo
