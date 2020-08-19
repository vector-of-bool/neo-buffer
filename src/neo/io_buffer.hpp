#pragma once

#include <neo/as_dynamic_buffer.hpp>
#include <neo/buffer_algorithm/size.hpp>
#include <neo/buffer_range.hpp>
#include <neo/dynamic_buffer.hpp>

#include <neo/assert.hpp>
#include <neo/ref.hpp>

namespace neo {

template <as_dynamic_buffer_convertible DynBuf>
class dynamic_io_buffer {
    wrap_if_reference_t<DynBuf> _dyn_buf;

    std::size_t _read_area_size  = unref(_dyn_buf).size();
    std::size_t _write_area_size = unref(_dyn_buf).size() - _read_area_size;

public:
    constexpr dynamic_io_buffer() = default;

    constexpr explicit dynamic_io_buffer(DynBuf&& db)
        : _dyn_buf(NEO_FWD(db)) {}

    constexpr dynamic_io_buffer(DynBuf&& db, std::size_t read_area_size)
        : _dyn_buf(NEO_FWD(db))
        , _read_area_size(read_area_size) {}

    constexpr auto&          storage() & noexcept { return unref(_dyn_buf); }
    constexpr auto&          storage() const& noexcept { return unref(_dyn_buf); }
    constexpr decltype(auto) storage() && noexcept { return unref(NEO_FWD(_dyn_buf)); }

    constexpr decltype(auto) buffer() noexcept { return as_dynamic_buffer(storage()); }
    constexpr decltype(auto) buffer() const noexcept { return as_dynamic_buffer(storage()); }

    constexpr decltype(auto) next(std::size_t size) {
        auto read_size = (std::min)(size, _read_area_size);
        return buffer().data(0, read_size);
    }

    constexpr void consume(std::size_t s) {
        neo_assert(expects,
                   s <= _read_area_size,
                   "Cannot consume more bytes than are available in the read-area",
                   s,
                   _read_area_size,
                   _write_area_size);
        buffer().consume(s);
        _read_area_size -= s;
    }

    constexpr decltype(auto) prepare(std::size_t size) {
        if (size <= _write_area_size) {
            // There's enough room in the output area to just yield it
            return buffer().data(_read_area_size, size);
        } else {
            // We need to expand the output area
            auto prepare_grow_size = size - _write_area_size;
            auto grow_size         = dynbuf_safe_grow_size(buffer(), prepare_grow_size);
            buffer().grow(grow_size);
            _write_area_size += grow_size;
            return buffer().data(_read_area_size, _write_area_size);
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
        buffer().shrink(_write_area_size);
        _write_area_size = 0;
    }
};

template <typename T>
dynamic_io_buffer(T &&) -> dynamic_io_buffer<T>;

template <typename T>
dynamic_io_buffer(T&&, std::size_t) -> dynamic_io_buffer<T>;

}  // namespace neo
