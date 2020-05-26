#pragma once

#include <neo/buffer_algorithm/size.hpp>
#include <neo/buffer_range.hpp>
#include <neo/buffer_range_consumer.hpp>
#include <neo/buffer_sink.hpp>
#include <neo/buffer_source.hpp>
#include <neo/dynamic_buffer.hpp>

#include <neo/assert.hpp>
#include <neo/test_concept.hpp>

namespace neo {

// clang-format off
template <typename T>
concept dynamic_io_buffer = buffer_source<T> && buffer_sink<T>;
// clang-format on

struct proto_dynamic_io_buffer {
    proto_buffer_range         data(std::size_t);
    void                       consume(std::size_t);
    proto_mutable_buffer_range prepare(std::size_t);
    void                       commit(std::size_t);
};

template <dynamic_buffer DynBuf>
class dynamic_io_buffer_adaptor {
    DynBuf _dyn_buf;

    std::size_t _read_area_size  = _dyn_buf.size();
    std::size_t _write_area_size = _dyn_buf.size() - _read_area_size;

public:
    constexpr explicit dynamic_io_buffer_adaptor(DynBuf&& db_)
        : _dyn_buf(db_) {}

    constexpr dynamic_io_buffer_adaptor(DynBuf&& db_, std::size_t read_area_size)
        : _dyn_buf(db_)
        , _read_area_size(read_area_size) {}

    constexpr decltype(auto) data(std::size_t size) {
        auto read_size = (std::min)(size, _read_area_size);
        return _dyn_buf.data(0, read_size);
    }

    constexpr decltype(auto) data() { return data(_read_area_size); }

    constexpr void consume(std::size_t s) { _dyn_buf.consume(s); }

    constexpr decltype(auto) prepare(std::size_t size) {
        if (size <= _write_area_size) {
            // There's enough room in the output area to just yield it
            return _dyn_buf.data(_read_area_size, size);
        } else {
            // Grow the output area
            auto grow_size = size - _write_area_size;
            auto tail      = _dyn_buf.grow(grow_size);
            _write_area_size += buffer_size(tail);
            return _dyn_buf.data(_read_area_size, _write_area_size);
        }
    }

    constexpr void commit(std::size_t size) {
        neo_assert(expects,
                   size <= _write_area_size,
                   "Cannot commit more bytes than are available in the write-area",
                   size,
                   _write_area_size);
        _read_area_size += size;
        _write_area_size -= size;
    }

    constexpr void shrink_uncommitted() {
        _dyn_buf.shrink(_write_area_size);
        _write_area_size = 0;
    }
};

template <typename T>
dynamic_io_buffer_adaptor(T &&) -> dynamic_io_buffer_adaptor<T>;

template <typename T>
dynamic_io_buffer_adaptor(T&&, std::size_t) -> dynamic_io_buffer_adaptor<T>;

NEO_TEST_CONCEPT(dynamic_io_buffer<dynamic_io_buffer_adaptor<proto_dynamic_buffer>>);

}  // namespace neo
