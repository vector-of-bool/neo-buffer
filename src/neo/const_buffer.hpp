#pragma once

#include <neo/byte_pointer.hpp>
#include <neo/detail/single_buffer_iter.hpp>
#include <neo/mutable_buffer.hpp>

#include <cassert>
#include <cstddef>
#include <string_view>

namespace neo {

/**
 * A type that represents a view to a readonly segment of contiguous memory.
 */
class const_buffer {
public:
    using pointer   = const std::byte*;
    using size_type = std::size_t;

private:
    pointer   _data = nullptr;
    size_type _size = 0;

    friend constexpr auto buffer_sequence_begin(const_buffer cb) noexcept {
        return detail::single_buffer_iter(cb);
    }

    friend constexpr auto buffer_sequence_end(const_buffer) noexcept {
        return detail::single_buffer_iter_sentinel();
    }

public:
    constexpr const_buffer() noexcept = default;
    constexpr const_buffer(pointer ptr, size_type size) noexcept
        : _data(ptr)
        , _size(size) {}

    constexpr const_buffer(mutable_buffer buf) noexcept
        : _data(buf.data())
        , _size(buf.size()) {}

    explicit constexpr const_buffer(std::string_view sv) noexcept
        : _data(byte_pointer(sv.data()))
        , _size(sv.size()) {}

    template <typename Char, typename Traits>
    explicit constexpr const_buffer(std::basic_string_view<Char, Traits> sv) noexcept
        : _data(byte_pointer(sv.data()))
        , _size(sv.size() * sizeof(Char)) {}

    template <typename Char, typename Traits>
    explicit constexpr operator std::basic_string_view<Char, Traits>() const noexcept {
        return std::basic_string_view<Char, Traits>(reinterpret_cast<const Char*>(data()),
                                                    size() / sizeof(Char));
    }

    constexpr pointer   data() const noexcept { return _data; }
    constexpr pointer   data_end() const noexcept { return _data + size(); }
    constexpr size_type size() const noexcept { return _size; }
    constexpr bool      empty() const noexcept { return size() == 0; }

    constexpr const_buffer& operator+=(size_type s) noexcept {
        assert(s <= size() && "Advanced neo::const_buffer past-the-end");
        _data += s;
        _size -= s;
        return *this;
    }

    constexpr void remove_prefix(size_type n) noexcept { *this += n; }
    constexpr void remove_suffix(size_type n) noexcept {
        assert(n <= size() && "neo::const_buffer::remove_suffix(n) : `n` is greater than size()");
        _size -= n;
    }

    constexpr const_buffer first(size_type s) const noexcept {
        assert(s <= size() && "neo::const_buffer::first() requested past-the-end of the buffer");
        return const_buffer(_data, s);
    }

    constexpr const_buffer last(size_type s) const noexcept {
        assert(s <= size() && "neo::const_buffer::last(n) : Given `n` is greater than size()");
        auto off = _size - s;
        return *this + off;
    }

    std::byte operator[](size_type offset) const noexcept {
        assert(offset < size() && "neo::const_buffer[n] : Given `n` is past-the-end");
        return data()[offset];
    }

    friend constexpr const_buffer operator+(const_buffer buf, const_buffer::size_type s) noexcept {
        auto copy = buf;
        copy += s;
        return copy;
    }
};

}  // namespace neo