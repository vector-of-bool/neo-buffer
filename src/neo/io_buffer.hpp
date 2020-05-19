#pragma once

#include <neo/buffer_algorithm/size.hpp>
#include <neo/buffer_range.hpp>
#include <neo/buffer_range_consumer.hpp>
#include <neo/dynamic_buffer.hpp>

#include <neo/assert.hpp>
#include <neo/test_concept.hpp>

namespace neo {

// clang-format off
template <typename T>
concept dynamic_input_buffer =
    requires(T in, std::size_t size) {
        { in.read(size) } -> buffer_range;
        { in.consume(size) };
    };

template <typename T>
concept dynamic_output_buffer =
    requires(T out, std::size_t size) {
        { out.prepare(size) } -> mutable_buffer_range;
        { out.commit(size) };
    };

template <typename T>
concept dynamic_io_buffer = dynamic_input_buffer<T> && dynamic_output_buffer<T>;
// clang-format on

struct proto_dynamic_input_buffer {
    proto_buffer_range read(std::size_t);
    void               consume(std::size_t);
};

struct proto_dynamic_output_buffer {
    proto_mutable_buffer_range prepare(std::size_t);
    void                       commit(std::size_t);
};

struct proto_dynamic_io_buffer {
    proto_buffer_range         read(std::size_t);
    void                       consume(std::size_t);
    proto_mutable_buffer_range prepare(std::size_t);
    void                       commit(std::size_t);
};

NEO_TEST_CONCEPT(dynamic_input_buffer<proto_dynamic_input_buffer>);
NEO_TEST_CONCEPT(dynamic_output_buffer<proto_dynamic_output_buffer>);

template <dynamic_buffer DynBuf>
class dynamic_io_buffer_adaptor {
    DynBuf _dyn_buf;

    std::size_t _read_area_size  = _dyn_buf.size();
    std::size_t _write_area_size = _dyn_buf.size() - _read_area_size;

public:
    explicit dynamic_io_buffer_adaptor(DynBuf&& db_)
        : _dyn_buf(db_) {}

    dynamic_io_buffer_adaptor(DynBuf&& db_, std::size_t read_area_size)
        : _dyn_buf(db_)
        , _read_area_size(read_area_size) {}

    decltype(auto) read(std::size_t size) {
        auto read_size = (std::min)(size, _read_area_size);
        return _dyn_buf.data(0, read_size);
    }

    decltype(auto) read() { return read(_read_area_size); }

    void consume(std::size_t s) { _dyn_buf.consume(s); }

    decltype(auto) prepare(std::size_t size) {
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

    void commit(std::size_t size) {
        neo_assert(expects,
                   size <= _write_area_size,
                   "Cannot commit more bytes than are available in the write-area",
                   size,
                   _write_area_size);
        _read_area_size += size;
        _write_area_size -= size;
    }

    void shrink_uncommitted() {
        _dyn_buf.shrink(_write_area_size);
        _write_area_size = 0;
    }
};

template <typename T>
dynamic_io_buffer_adaptor(T &&) -> dynamic_io_buffer_adaptor<T>;

template <typename T>
dynamic_io_buffer_adaptor(T&&, std::size_t) -> dynamic_io_buffer_adaptor<T>;

NEO_TEST_CONCEPT(dynamic_io_buffer<dynamic_io_buffer_adaptor<proto_dynamic_buffer>>);

template <buffer_range T>
class input_buffer_adaptor {
    T                              _in_bufseq;
    neo::buffer_range_consumer<T&> _in{_in_bufseq};

public:
    explicit input_buffer_adaptor(T&& in)
        : _in_bufseq(in) {}

    decltype(auto) read(std::size_t s) { return _in.prepare(s); }
    void           consume(std::size_t s) { _in.consume(s); }
};

template <typename T>
input_buffer_adaptor(T &&) -> input_buffer_adaptor<T>;

NEO_TEST_CONCEPT(dynamic_input_buffer<input_buffer_adaptor<proto_buffer_range>>);

template <mutable_buffer_range T>
class output_buffer_adaptor {
    T                              _out_bufseq;
    neo::buffer_range_consumer<T&> _out{_out_bufseq};

public:
    explicit output_buffer_adaptor(T&& out)
        : _out_bufseq(out) {}

    decltype(auto) prepare(std::size_t s) { return _out.prepare(s); }
    void           commit(std::size_t s) { _out.consume(s); }
};

template <typename T>
output_buffer_adaptor(T &&) -> output_buffer_adaptor<T>;

NEO_TEST_CONCEPT(dynamic_output_buffer<output_buffer_adaptor<proto_mutable_buffer_range>>);

}  // namespace neo
