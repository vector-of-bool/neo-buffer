#pragma once

#include <neo/detail/single_buffer_iter.hpp>

#include <cassert>
#include <cstddef>
#include <iterator>
#include <type_traits>

namespace neo {

struct proto_const_data_container {
    using value_type    = int;
    using const_pointer = const value_type*;
    const value_type* data() noexcept;
    const value_type* data() const noexcept;
    std::size_t       size() const noexcept;
};

struct proto_mutable_data_container {
    using value_type = int;
};

template <typename Container>
using data_pointer_t = decltype(std::declval<Container>().data());

template <typename Container>
using data_type_t = std::remove_pointer_t<data_pointer_t<Container>>;

template <typename Container>
constexpr std::size_t data_type_size_v = sizeof(data_type_t<Container>);

// clang-format off
template <typename T>
concept const_data_container =
    requires (const T& cb) {
        { cb.data() } noexcept -> buffer_safe;
        { neo::byte_pointer(cb.data()) } noexcept;
        { cb.size() } noexcept -> convertible_to<std::size_t>;
    };

template <typename T>
concept mutable_data_container =
    const_data_container<T> &&
    requires (T& cont) {
        { neo::byte_pointer(cont.data()) } noexcept -> same_as<std::byte*>;
    };

static_assert(const_data_container<proto_const_data_container>);

template <typename T>
concept const_buffer_constructible =
    neo::trivially_copyable<typename T::value_type> &&
    neo::constructible_from<T, typename T::const_pointer, std::size_t>;

template <typename T>
concept mutable_buffer_constructible =
    const_buffer_constructible<T> &&
    neo::constructible_from<T, typename T::pointer, std::size_t>;
// clang-format on

template <const_data_container C>
constexpr std::size_t data_container_byte_size(const C& c) noexcept {
    return c.size() * data_type_size_v<C>;
}

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

    template <mutable_data_container C>
    explicit constexpr mutable_buffer(C&& c) noexcept
        : _data(neo::byte_pointer(c.data()))
        , _size(c.size() * sizeof(data_type_t<C>)) {}

    template <mutable_buffer_constructible T>
    explicit constexpr operator T() const noexcept {
        return T(reinterpret_cast<data_pointer_t<T>>(data()), size() / sizeof(data_type_t<T>));
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

    // clang-format off
    template <typename String>
    requires const_data_container<String>
             && equality_comparable<String>
    constexpr bool equals_string(const String& s) const noexcept {
        return String(*this) == s;
    }
    // clang-format on

    friend constexpr mutable_buffer operator+(mutable_buffer            buf,
                                              mutable_buffer::size_type s) noexcept {
        auto copy = buf;
        copy += s;
        return copy;
    }
};

}  // namespace neo