#pragma once

#include <neo/buffer_concepts.hpp>
#include <neo/buffer_seq_iter.hpp>

#include <neo/iterator_concepts.hpp>

#include <iterator>
#include <type_traits>

namespace neo {

template <const_buffer_sequence Buffers>
class bytewise_iterator {
    using inner_iter_type     = buffer_sequence_iterator_t<Buffers>;
    using inner_sentinel_type = buffer_sequence_sentinel_t<Buffers>;

public:
    using buffer_type      = iter_value_t<inner_iter_type>;
    using iterator_concept = typename std::iterator_traits<inner_iter_type>::iterator_category;
    using difference_type  = std::ptrdiff_t;
    using value_type       = std::remove_cv_t<std::remove_pointer_t<typename buffer_type::pointer>>;
    using reference
        = std::add_lvalue_reference_t<std::remove_pointer_t<typename buffer_type::pointer>>;

private:
    inner_iter_type     _cur;
    inner_sentinel_type _stop;
    std::size_t         _cur_buf_pos = 0;
    std::size_t         _abs_pos     = 0;

    auto _buf() const noexcept { return *_cur; }

    void _advance(std::size_t off) noexcept {
        // Check for iterating past-the end
        assert(_cur != _stop);
        // Calc the number of bytes remaining in the current buffer
        auto remaining_in_buf = _buf().size() - _cur_buf_pos;
        // Check if we are going to run off-the-end of the current buffer:
        if (off < remaining_in_buf) {
            // We've still got space remaining. Okay:
            _cur_buf_pos += off;
            _abs_pos += off;
            return;
        }
        // We've hit the end of our current buffer.
        _abs_pos += remaining_in_buf;
        _cur_buf_pos = 0;
        // Skip to the next non-empty buffer in the sequence.
        do {
            _cur++;
        } while (_cur != _stop && (*_cur).empty());
        // We might have more to do:
        if (off != remaining_in_buf) {
            // Advance again, with the offset adjusted by how many bytes we skipped
            return _advance(off - remaining_in_buf);
        }
    }

    void _rewind(std::size_t off) noexcept {
        assert(_abs_pos != 0);
        // Check if we are about to step behind the bottom of the current buffer
        const auto buf_pos = _cur_buf_pos;
        if (off <= buf_pos) {
            // We're okay. We still have room.
            _cur_buf_pos -= off;
            _abs_pos -= off;
            return;
        }
        // We're stepping behind the current buffer
        _abs_pos -= buf_pos;
        --_cur;
        _cur_buf_pos = _buf().size();
        return _rewind(off - buf_pos);
    }

public:
    constexpr bytewise_iterator() = default;

    constexpr explicit bytewise_iterator(const Buffers& bufs)
        : _cur(buffer_sequence_begin(bufs))
        , _stop(buffer_sequence_end(bufs)) {
        // Find the first non-zero buffer
        while (_cur != _stop && _buf().size() == 0) {
            ++_cur;  // Skip this empty buffer
        }
    }

    constexpr auto begin() const noexcept { return *this; }
    constexpr auto end() const noexcept {
        auto cp = *this;
        while (cp._cur != cp._stop) {
            cp._abs_pos += cp._cur->size();
            ++cp._cur;
        }
        return cp;
    }

    constexpr auto& operator++() noexcept {
        _advance(1);
        return *this;
    }
    constexpr auto operator++(int) noexcept {
        auto cp = *this;
        ++*this;
        return cp;
    }

    constexpr auto& operator--() noexcept {
        _rewind(1);
        return *this;
    }
    constexpr auto operator--(int) noexcept {
        auto cp = *this;
        --*this;
        return cp;
    }

    constexpr auto& operator*() const noexcept { return (*_cur)[_cur_buf_pos]; }
    constexpr auto& operator[](difference_type pos) const noexcept {
        auto tmp = *this + pos;
        return *tmp;
    }

    constexpr bool operator==(const bytewise_iterator& right) const noexcept {
        return _abs_pos == right._abs_pos;
    }
    constexpr bool operator!=(const bytewise_iterator& right) const noexcept {
        return !(*this == right);
    }
    constexpr bool operator<(const bytewise_iterator& other) const noexcept {
        return _abs_pos < other._abs_pos;
    }
    constexpr bool operator>(const bytewise_iterator& other) const noexcept {
        return _abs_pos > other._abs_pos;
    }
    constexpr bool operator>=(const bytewise_iterator& other) const noexcept {
        return !(_abs_pos < other._abs_pos);
    }
    constexpr bool operator<=(const bytewise_iterator& other) const noexcept {
        return !(_abs_pos > other._abs_pos);
    }

    difference_type operator-(const bytewise_iterator& other) const noexcept {
        return _abs_pos - other._abs_pos;
    }

    constexpr bytewise_iterator& operator+=(difference_type off) noexcept {
        if (off < 0) {
            _rewind(static_cast<std::size_t>(-off));
        } else if (off > 0) {
            _advance(static_cast<std::size_t>(off));
        }
        return *this;
    }

    constexpr bytewise_iterator& operator-=(difference_type off) noexcept { return *this += -off; }

    constexpr friend bytewise_iterator operator+(bytewise_iterator it,
                                                 difference_type   off) noexcept {
        return it += off;
    }
    constexpr friend bytewise_iterator operator+(difference_type   off,
                                                 bytewise_iterator it) noexcept {
        return it += off;
    }
    constexpr friend bytewise_iterator operator-(bytewise_iterator it,
                                                 difference_type   off) noexcept {
        return it -= off;
    }
};

template <typename T>
bytewise_iterator(T) -> bytewise_iterator<T>;

}  // namespace neo