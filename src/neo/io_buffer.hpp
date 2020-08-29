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
    [[no_unique_address]] wrap_refs_t<DynBuf> _dyn_buf;

    std::size_t _read_area_size = as_dynamic_buffer(unref(_dyn_buf)).size();

    constexpr std::size_t _get_write_area_size() const noexcept {
        return as_dynamic_buffer(unref(const_cast<wrap_refs_t<DynBuf>&>(_dyn_buf))).size()
            - _read_area_size;
    }

public:
    constexpr dynamic_io_buffer() = default;

    constexpr explicit dynamic_io_buffer(DynBuf&& db)
        : _dyn_buf(NEO_FWD(db)) {}

    constexpr dynamic_io_buffer(DynBuf&& db, std::size_t read_area_size)
        : _dyn_buf(NEO_FWD(db))
        , _read_area_size(read_area_size) {}

    NEO_DECL_UNREF_GETTER(storage, _dyn_buf);

    constexpr decltype(auto) buffer() noexcept { return as_dynamic_buffer(storage()); }
    constexpr decltype(auto) buffer() const noexcept { return as_dynamic_buffer(storage()); }

    constexpr std::size_t available() const noexcept { return _read_area_size; }

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
                   _get_write_area_size());
        buffer().consume(s);
        _read_area_size -= s;
    }

    constexpr decltype(auto) prepare(std::size_t size) {
        if (size <= _get_write_area_size()) {
            // There's enough room in the output area to just yield it
            return buffer().data(_read_area_size, size);
        } else {
            // We need to expand the output area
            // Calc how much we can grow within the bounds of max_size():
            auto prepare_grow_size = size - _get_write_area_size();
            auto grow_size         = dynbuf_safe_grow_size(buffer(), prepare_grow_size);
            // Prevent a single grow that would allocate an unreasonably large area:
            // XXX: Allow user to tweak this as a library config option
            constexpr std::size_t max_alloc_size   = 1024 * 1024 * 16;
            const auto            capped_grow_size = (std::min)(max_alloc_size, grow_size);
            buffer().grow(capped_grow_size);
            return buffer().data(_read_area_size, _get_write_area_size());
        }
    }

    constexpr void commit(std::size_t size) noexcept {
        neo_assert(expects,
                   size <= _get_write_area_size(),
                   "Cannot commit more bytes than are available in the write-area",
                   size,
                   _get_write_area_size());
        _read_area_size += size;
    }

    constexpr void shrink_uncommitted() noexcept { dynbuf_resize(buffer(), available()); }
};

template <typename T>
dynamic_io_buffer(T &&) -> dynamic_io_buffer<T>;

template <typename T>
dynamic_io_buffer(T&&, std::size_t) -> dynamic_io_buffer<T>;

}  // namespace neo
