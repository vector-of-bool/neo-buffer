#pragma once

#include <neo/as_buffer.hpp>
#include <neo/buffer_algorithm/copy.hpp>
#include <neo/dynamic_buffer.hpp>

#include <neo/assert.hpp>
#include <neo/fwd.hpp>
#include <neo/ref.hpp>

#include <limits>
#include <numeric>

namespace neo {

template <dynamic_buffer Storage>
class shifting_dynamic_buffer {
public:
    using storage_type = std::remove_cvref_t<Storage>;

private:
    wrap_if_reference_t<Storage> _storage;

    std::size_t _beg_idx = 0;
    std::size_t _size    = unref(_storage).size();

public:
    constexpr shifting_dynamic_buffer() = default;

    constexpr explicit shifting_dynamic_buffer(Storage&& s)
        : _storage(NEO_FWD(s)) {}

    constexpr shifting_dynamic_buffer(Storage&& s, std::size_t size)
        : _storage(NEO_FWD(s))
        , _size(size) {}

    constexpr storage_type&       storage() noexcept { return _storage; }
    constexpr const storage_type& storage() const noexcept { return _storage; }

    constexpr auto max_size() const noexcept { return storage().max_size(); }
    constexpr auto size() const noexcept { return _size; }
    constexpr auto capacity() const noexcept { return storage().size() - _beg_idx; }

    constexpr auto data(std::size_t pos, std::size_t size_) const
        noexcept(noexcept(storage().data(0, 0))) {
        neo_assert(expects,
                   size_ <= size(),
                   "Cannot read more bytes than are contained in a dynamic buffer",
                   pos,
                   size_,
                   this->size());
        return storage().data(pos + _beg_idx, size_);
    }

    constexpr auto data(std::size_t pos,
                        std::size_t size_) noexcept(noexcept(storage().data(0, 0))) {
        neo_assert(expects,
                   size_ <= size(),
                   "Cannot read more bytes than are contained in a dynamic buffer",
                   pos,
                   size_,
                   this->size());
        return storage().data(pos + _beg_idx, size_);
    }

    constexpr auto grow(std::size_t more) noexcept(noexcept(storage().grow(0))) {
        std::size_t prev_size  = size();
        std::size_t end_idx    = _beg_idx + _size;
        const auto  avail_room = storage().size() - end_idx;
        if (avail_room >= more) {
            // There is enough room following the partial buffer to just expand into that
            _size += more;
            return data(prev_size, more);
        } else if (_beg_idx != 0) {
            // We don't have enough room after the partial buffer to just expand it, but
            // we are offset from the beginning of the buffer and we might be able to make room
            // by just shifting everyone over.
            buffer_copy(storage().data(0, _size), storage().data(_beg_idx, _size));
            _beg_idx = 0;
            // Try again now that we have more room.
            return grow(more);
        } else {
            // We don't have enough room, and the buffer is at the bottom. We need to grow the
            // backing storage.
            std::size_t min_grow    = more - avail_room;
            auto        growth_size = (std::max)(std::size_t(1024), min_grow);
            storage().grow(growth_size);
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
    }
    constexpr void consume(std::size_t size_) noexcept {
        neo_assert(expects,
                   size_ <= size(),
                   "Cannot consume more bytes than are contained in a dynamic buffer",
                   size_,
                   this->size());
        _beg_idx += size_;
        _size -= size_;
    }
};  // namespace neo

template <typename S>
explicit shifting_dynamic_buffer(S &&)->shifting_dynamic_buffer<S>;

template <typename S>
shifting_dynamic_buffer(S&&, std::size_t)->shifting_dynamic_buffer<S>;

}  // namespace neo
