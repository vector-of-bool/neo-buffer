#pragma once

#include <neo/as_buffer.hpp>
#include <neo/as_dynamic_buffer.hpp>
#include <neo/buffer_algorithm/copy.hpp>

#include <neo/assert.hpp>
#include <neo/fwd.hpp>
#include <neo/ref.hpp>

#include <limits>
#include <numeric>

namespace neo {

template <as_dynamic_buffer_convertible Storage>
class shifting_dynamic_buffer {
public:
    using storage_type = std::remove_cvref_t<Storage>;

private:
    wrap_refs_t<Storage> _storage;

    std::size_t _beg_idx = 0;
    std::size_t _size    = unref(_storage).size();

public:
    constexpr shifting_dynamic_buffer() = default;

    constexpr explicit shifting_dynamic_buffer(Storage&& s) noexcept
        : _storage(NEO_FWD(s)) {}

    constexpr shifting_dynamic_buffer(Storage&& s, std::size_t size) noexcept
        : _storage(NEO_FWD(s))
        , _size(size) {}

    NEO_DECL_UNREF_GETTER(storage, _storage);

    constexpr decltype(auto) inner_buffer() noexcept { return as_dynamic_buffer(storage()); }
    constexpr decltype(auto) inner_buffer() const noexcept { return as_dynamic_buffer(storage()); }

    constexpr auto max_size() const noexcept { return inner_buffer().max_size(); }
    constexpr auto size() const noexcept { return _size; }
    constexpr auto capacity() const noexcept { return inner_buffer().size() - _beg_idx; }

    constexpr auto data(std::size_t pos, std::size_t size_) const noexcept {
        neo_assert(expects,
                   size_ <= size(),
                   "Cannot read more bytes than are contained in a dynamic buffer",
                   pos,
                   size_,
                   this->size());
        return inner_buffer().data(pos + _beg_idx, size_);
    }

    constexpr auto data(std::size_t pos, std::size_t size_) noexcept {
        neo_assert(expects,
                   size_ <= size(),
                   "Cannot read more bytes than are contained in a dynamic buffer",
                   pos,
                   size_,
                   this->size());
        return inner_buffer().data(pos + _beg_idx, size_);
    }

    constexpr auto grow(std::size_t more) noexcept(noexcept(inner_buffer().grow(more))) {
        std::size_t prev_size  = size();
        std::size_t end_idx    = _beg_idx + _size;
        const auto  avail_room = inner_buffer().size() - end_idx;
        if (avail_room >= more) {
            // There is enough room following the partial buffer to just expand into that
            _size += more;
            return data(prev_size, more);
        } else if (_beg_idx != 0) {
            // We don't have enough room after the partial buffer to just expand it, but
            // we are offset from the beginning of the buffer and we might be able to make room
            // by just shifting everyone over.
            buffer_copy(inner_buffer().data(0, _size), inner_buffer().data(_beg_idx, _size));
            _beg_idx = 0;
            // Try again now that we have more room.
            return grow(more);
        } else {
            // We don't have enough room, and the buffer is at the bottom. We need to grow the
            // backing storage.
            std::size_t min_grow    = more - avail_room;
            auto        growth_size = (std::max)(std::size_t(1024), min_grow);
            inner_buffer().grow(growth_size);
            _size += more;
            return data(prev_size, more);
        }
    }

    constexpr void shrink(std::size_t size_) noexcept {
        neo_assert(expects,
                   size_ <= size(),
                   "Cannot shrink a dynamic buffer below its own size",
                   size_,
                   this->size());
        _size -= size_;
        if (_size == 0) {
            _beg_idx = 0;
        }
    }
    constexpr void consume(std::size_t size_) noexcept {
        neo_assert(expects,
                   size_ <= size(),
                   "Cannot consume more bytes than are contained in a dynamic buffer",
                   size_,
                   this->size());
        _beg_idx += size_;
        _size -= size_;
        if (_size == 0) {
            _beg_idx = 0;
        }
    }
};  // namespace neo

template <as_dynamic_buffer_convertible S>
explicit shifting_dynamic_buffer(S &&) -> shifting_dynamic_buffer<S>;

template <as_dynamic_buffer_convertible S>
shifting_dynamic_buffer(S&&, std::size_t) -> shifting_dynamic_buffer<S>;

}  // namespace neo
