#pragma once

#include <neo/detail/single_buffer_iter.hpp>
#include <neo/trivial_range.hpp>

#include <neo/assert.hpp>

#include <utility>

namespace neo::detail {

template <typename P, typename ThisType>
class buffer_base {
public:
    /**
     * The pointer type of the buffer.
     */
    using pointer = P;

    /**
     * The size type. Just std::size_t
     */
    using size_type = std::size_t;

private:
    /// The data contained in the buffer
    pointer   _data = nullptr;
    size_type _size = 0;

public:
    /**
     * Default-construct an empty buffer
     */
    constexpr buffer_base() noexcept = default;

    /**
     * Construct a new buffer from a given pointer and size
     */
    constexpr buffer_base(pointer p, size_type size) noexcept
        : _data(p)
        , _size(size) {}

    /// Obtain a pointer to the data
    [[nodiscard]] constexpr pointer data() const noexcept { return _data; }
    /// Obtain the past-the-end pointer for the data
    [[nodiscard]] constexpr pointer data_end() const noexcept { return _data + size(); }
    /// Obtain the number of bytes in the buffer
    [[nodiscard]] constexpr size_type size() const noexcept { return _size; }
    /// Determine whether the buffer is empty (size == 0)
    [[nodiscard]] constexpr bool empty() const noexcept { return size() == 0; }

    /**
     * Remove the first `n` bytes from the buffer
     */
    constexpr void remove_prefix(size_type n) noexcept {
        neo_assert(expects,
                   n <= size(),
                   "Cannot remove more bytes from a buffer than it has available",
                   n,
                   size());
        _data += n;
        _size -= n;
    }

    /**
     * Remove the trailing `n` bytes from the buffer
     */
    constexpr void remove_suffix(size_type n) noexcept {
        neo_assert(expects,
                   n <= size(),
                   "Cannot remove more bytes from a buffer than it has available",
                   n,
                   size());
        _size -= n;
    }

    /**
     * Remove the leading `n` bytes from the buffer. (Same as remove_prefix)
     */
    constexpr ThisType& operator+=(size_type s) noexcept {
        remove_prefix(s);
        return static_cast<ThisType&>(*this);
    }

    /**
     * Get a reference to the Nth byte in the buffer
     */
    [[nodiscard]] constexpr std::remove_pointer_t<pointer>&
    operator[](size_type offset) const noexcept {
        neo_assert(expects, offset < size(), "Buffer index is out-of-range", offset, size());
        return data()[offset];
    }

    /**
     * Create a new buffer that only views the first `s` bytes of the buffer.
     * The given size must be less than or equal-to size()
     */
    [[nodiscard]] constexpr ThisType first(size_type s) const noexcept {
        neo_assert(expects,
                   s <= size(),
                   "Cannot take slice of buffer with size greater than the buffer's size",
                   s,
                   size());
        return ThisType(_data, s);
    }

    /**
     * Create a new buffer that views the last `s` bytes of the buffer.
     * The given size must be less than or equal-to size()
     */
    [[nodiscard]] constexpr ThisType last(size_type s) const noexcept {
        neo_assert(expects,
                   s <= size(),
                   "Cannot take slice of buffer with size greater than the buffer's size",
                   s,
                   size());
        auto off = _size - s;
        return *this + off;
    }

    /**
     * Split the buffer in two, partitioning an `part`.
     */
    [[nodiscard]] constexpr std::pair<ThisType, ThisType> split(size_type part) const noexcept {
        neo_assert(expects,
                   part <= size(),
                   "Cannot partition a buffer past-the-end of the buffer",
                   part,
                   size());
        auto left  = first(part);
        auto right = last(size() - part);
        return {left, right};
    }

    /**
     * Create a new buffer that drops the first `s` bytes from the left-hand
     * buffer.
     */
    [[nodiscard]] friend constexpr ThisType operator+(buffer_base buf, size_type s) noexcept {
        auto copy = buf;
        copy += s;
        return ThisType(copy);
    }

    // clang-format off
    /**
     * Compare the contents of the buffer to the contents of the given data
     * container.
     */
    template <typename String>
    requires constructible_from<String, ThisType> &&
             equality_comparable<String>
    constexpr bool equals_string(const String& s) const noexcept {
        return String(static_cast<const ThisType&>(*this)) == s;
    }
    // clang-format on

    constexpr auto begin() const noexcept {
        return detail::single_buffer_iter(static_cast<const ThisType&>(*this));
    }
    constexpr auto end() const noexcept { return detail::single_buffer_iter_sentinel(); }

    constexpr explicit operator bool() const noexcept { return !empty(); }
};

}  // namespace neo::detail
