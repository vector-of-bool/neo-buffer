#pragma once

#include <neo/assert.hpp>
#include <neo/concepts.hpp>

#include <cstddef>

namespace neo {

template <std::size_t Size>
struct byte_array {
    std::byte _bytes[Size ? Size : 1];

    using value_type      = std::byte;
    using size_type       = std::size_t;
    using difference_type = std::ptrdiff_t;
    using reference       = value_type&;
    using const_reference = const value_type&;
    using pointer         = value_type*;
    using const_pointer   = const value_type*;
    using iterator        = pointer;
    using const_iterator  = const_pointer;

    constexpr pointer       data() noexcept { return _bytes; }
    constexpr const_pointer data() const noexcept { return _bytes; }

    constexpr void fill(value_type v) noexcept {
        for (auto& b : *this) {
            b = v;
        }
    }

    constexpr size_type size() const noexcept { return Size; }
    constexpr bool      empty() const noexcept { return size() == 0; }
    constexpr size_type max_size() const noexcept { return size(); }

    constexpr iterator       begin() noexcept { return data(); }
    constexpr const_iterator begin() const noexcept { return data(); }
    constexpr const_iterator cbegin() const noexcept { return begin(); }
    constexpr iterator       end() noexcept { return data() + size(); }
    constexpr const_iterator end() const noexcept { return data() + size(); }
    constexpr const_iterator cend() const noexcept { return end(); }

    constexpr reference operator[](size_type idx) noexcept {
        neo_assert(expects, idx < size(), "Past-the-end access of byte_array", idx, size());
        return _bytes[idx];
    }

    constexpr const_reference operator[](size_type idx) const noexcept {
        neo_assert(expects, idx < size(), "Past-the-end access of byte_array", idx, size());
        return _bytes[idx];
    }
};

template <same_as<std::byte> T, same_as<std::byte>... Bs>
byte_array(T, Bs...) -> byte_array<sizeof...(Bs) + 1>;

}  // namespace neo
