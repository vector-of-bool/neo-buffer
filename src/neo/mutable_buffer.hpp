#pragma once

#include <neo/detail/single_buffer_iter.hpp>

#include <cassert>
#include <cstddef>
#include <iterator>
#include <string>
#include <string_view>
#include <type_traits>

namespace neo {

class mutable_buffer {
public:
    using pointer   = std::byte*;
    using size_type = std::size_t;

private:
    pointer   _data = nullptr;
    size_type _size = 0;

    friend constexpr auto buffer_sequence_begin(mutable_buffer mb) noexcept {
        return detail::single_buffer_iter(mb);
    }

    friend constexpr auto buffer_sequence_end(mutable_buffer) noexcept {
        return detail::single_buffer_iter_sentinel();
    }

public:
    constexpr mutable_buffer() noexcept = default;
    constexpr mutable_buffer(pointer p, size_type size) noexcept
        : _data(p)
        , _size(size) {}

    template <typename Char, typename Traits, typename Alloc>
    explicit constexpr mutable_buffer(std::basic_string<Char, Traits, Alloc>& str) noexcept
        : _data(neo::byte_pointer(str.data()))
        , _size(str.size() * sizeof(Char)) {}

    template <typename Char, typename Traits>
    explicit constexpr operator std::basic_string_view<Char, Traits>() const noexcept {
        return std::basic_string_view<Char, Traits>(reinterpret_cast<const char*>(_data),
                                                    size() / sizeof(Char));
    }

    constexpr pointer   data() const noexcept { return _data; }
    constexpr pointer   data_end() const noexcept { return _data + size(); }
    constexpr size_type size() const noexcept { return _size; }

    constexpr mutable_buffer& operator+=(size_type s) noexcept {
        assert(s <= size() && "Advanced neo::mutable_buffer past-the-end");
        _data += s;
        _size -= s;
        return *this;
    }

    constexpr void remove_prefix(size_type n) noexcept { *this += n; }
    constexpr void remove_suffix(size_type n) noexcept {
        assert(n <= size() && "neo::mutable_buffer::remove_suffix(n) : `n` is greater than size()");
        _size -= n;
    }

    constexpr mutable_buffer first(size_type s) const noexcept {
        assert(s <= size() && "neo::mutable_buffer::first() requested past-the-end of the buffer");
        return mutable_buffer(_data, s);
    }

    constexpr mutable_buffer last(size_type s) const noexcept {
        assert(s <= size() && "neo::mutable_buffer::last(n) : Given `n` is greater than size()");
        auto off = _size - s;
        return *this + off;
    }

    constexpr std::byte& operator[](size_type offset) const noexcept {
        assert(offset < size()
               && "neo::mutable_buffer::mutable_buffer[n] : Given `n` is past-the-end");
        return data()[offset];
    }

    friend constexpr mutable_buffer operator+(mutable_buffer            buf,
                                              mutable_buffer::size_type s) noexcept {
        auto copy = buf;
        copy += s;
        return copy;
    }
};

}  // namespace neo