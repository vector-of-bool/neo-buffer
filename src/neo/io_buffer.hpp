#pragma once

#include <neo/buffer_algorithm/size.hpp>
#include <neo/buffer_range.hpp>
#include <neo/buffer_range_consumer.hpp>
#include <neo/buffer_sink.hpp>
#include <neo/buffer_source.hpp>
#include <neo/dynamic_buffer.hpp>

#include <neo/assert.hpp>
#include <neo/ref.hpp>

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
    wrap_if_reference_t<DynBuf> _dyn_buf;

    std::size_t _read_area_size  = unref(_dyn_buf).size();
    std::size_t _write_area_size = unref(_dyn_buf).size() - _read_area_size;

public:
    constexpr explicit dynamic_io_buffer_adaptor() = default;

    constexpr explicit dynamic_io_buffer_adaptor(DynBuf&& db)
        : _dyn_buf(NEO_FWD(db)) {}

    constexpr dynamic_io_buffer_adaptor(DynBuf&& db, std::size_t read_area_size)
        : _dyn_buf(NEO_FWD(db))
        , _read_area_size(read_area_size) {}

    constexpr auto& storage() noexcept { return unref(_dyn_buf); }
    constexpr auto& storage() const noexcept { return unref(_dyn_buf); }

    constexpr decltype(auto) data(std::size_t size) {
        auto read_size = (std::min)(size, _read_area_size);
        return storage().data(0, read_size);
    }

    constexpr void consume(std::size_t s) { storage().consume(s); }

    constexpr decltype(auto) prepare(std::size_t size) {
        if (size <= _write_area_size) {
            // There's enough room in the output area to just yield it
            return storage().data(_read_area_size, size);
        } else {
            // We need to expand the output area
            auto prepare_grow_size = size - _write_area_size;
            auto grow_size         = dynbuf_safe_grow_size(storage(), prepare_grow_size);
            storage().grow(grow_size);
            _write_area_size += grow_size;
            return storage().data(_read_area_size, _write_area_size);
        }
    }

    constexpr void commit(std::size_t size) noexcept {
        neo_assert(expects,
                   size <= _write_area_size,
                   "Cannot commit more bytes than are available in the write-area",
                   size,
                   _write_area_size);
        _read_area_size += size;
        _write_area_size -= size;
    }

    constexpr void shrink_uncommitted() noexcept {
        storage().shrink(_write_area_size);
        _write_area_size = 0;
    }
};

template <typename T>
dynamic_io_buffer_adaptor(T &&) -> dynamic_io_buffer_adaptor<T>;

template <typename T>
dynamic_io_buffer_adaptor(T&&, std::size_t) -> dynamic_io_buffer_adaptor<T>;

}  // namespace neo
