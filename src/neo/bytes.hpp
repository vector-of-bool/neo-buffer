#pragma once

#include <neo/buffer_algorithm/copy.hpp>
#include <neo/buffer_algorithm/size.hpp>
#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <iterator>
#include <memory>

namespace neo {

/**
 * Represents a contiguous mutable array of bytes with no implied encoding.
 */
template <typename Allocator>
class basic_bytes {
public:
    /**
     * The allocator used by this bytes object
     */
    using allocator_type = Allocator;

    /**
     * A tag type that causes resize operations to leave new data in an
     * uninitialized state. Pass the tag value ``uninit``.
     */
    struct uninit_t {};
    constexpr static uninit_t uninit = {};

private:
    using alloc_traits = std::allocator_traits<allocator_type>;

public:
    using value_type      = std::byte;
    using size_type       = typename alloc_traits::size_type;
    using difference_type = typename alloc_traits::difference_type;
    using pointer         = typename alloc_traits::pointer;
    using const_pointer   = typename alloc_traits::const_pointer;

private:
    pointer   _bytes_ptr = nullptr;
    size_type _size      = 0;

private:
    [[no_unique_address]] allocator_type _alloc;

    /**
     * Resize the underlying array of bytes. This method will copy over the old
     * content, but it will not modify any new trailing bytes
     */
    constexpr pointer _resize_uninit(size_type new_size) noexcept {
        // Record the old buffer
        const auto old_ptr  = data();
        const auto old_size = size();
        const auto old_stop = old_ptr + old_size;

        // Allocate the new buffer
        _bytes_ptr = alloc_traits::allocate(_alloc, new_size);
        _size      = new_size;

        // Get the new stopping position
        const auto new_stop = data_end();

        // XXX: Replace with std::fill when we have the constexpr version available
        for (
            // Two iterators: One in the old buffer, one in the new buffer:
            auto old_it = old_ptr, new_it = _bytes_ptr;
            // Stop when either iterator hits its respective sentinel
            old_it != old_stop && new_it != new_stop;
            // Advance them in lock-step
            ++old_it,
                 ++new_it) {
            // Overwrite:
            *new_it = *old_it;
        }

        // Free the old buffer
        alloc_traits::deallocate(_alloc, old_ptr, old_size);

        // Return a pointer to the beginning of the new tail of the buffer if it
        // has grown, otherwise just the pointer to the end.
        const auto minsize = (new_size > old_size) ? old_size : new_size;
        return data() + minsize;
    }

    constexpr void _clear() noexcept {
        alloc_traits::deallocate(_alloc, _bytes_ptr, _size);
        _bytes_ptr = nullptr;
        _size      = 0;
    }

public:
    ~basic_bytes() { _clear(); }

    /**
     * Simple constructors
     */
    constexpr basic_bytes() noexcept = default;
    constexpr explicit basic_bytes(const allocator_type& alloc) noexcept
        : _alloc(alloc) {}

    /**
     * Construct a byte array of the given size filled with zero-bytes
     */
    constexpr explicit basic_bytes(size_type size) noexcept
        : basic_bytes(size, std::byte{0}) {}

    /**
     * Construct a byte array of the given size using the given allocator, and
     * fill it with zero-bytes
     */
    constexpr basic_bytes(size_type size, const allocator_type& alloc) noexcept
        : basic_bytes(size, std::byte{0}, alloc) {}

    /**
     * Construct a byte array and fill it with the given pattern bytes
     */
    constexpr basic_bytes(size_type size, std::byte pattern) noexcept
        : basic_bytes(size, pattern, allocator_type()) {}

    /**
     * Construct a byte array of the given size, and leave the contents
     * uninitialized.
     */
    constexpr basic_bytes(size_type size, uninit_t) noexcept
        : basic_bytes(size, uninit, allocator_type()) {}

    /**
     * Construct a byte array of the given size and fill it with the given
     * pattern. Use the provided allocator
     */
    constexpr basic_bytes(size_type size, std::byte pattern, const allocator_type& alloc) noexcept
        : _alloc(alloc) {
        resize(size, pattern);
    }

    /**
     * Construct a byte array of the given size using the given allocator, and
     * leave the contents uninitialized.
     */
    constexpr basic_bytes(size_type size, uninit_t, const allocator_type& alloc) noexcept
        : _alloc(alloc) {
        resize(size, uninit);
    }

    /**
     * Construct a byte array by copying the contents of the given buffer sequence
     */
    template <buffer_range Bufs>
    constexpr static basic_bytes copy(Bufs buf) noexcept {
        basic_bytes ret;
        ret.resize(neo::buffer_size(buf), uninit);
        neo::buffer_copy(neo::mutable_buffer(ret), buf);
        return ret;
    }

    /**
     * Regular copy constructor
     */
    constexpr basic_bytes(const basic_bytes& other) noexcept
        : basic_bytes(other, other.get_allocator()) {}

    /**
     * Copy, but use the provided allocator
     */
    constexpr basic_bytes(const basic_bytes& other, const allocator_type& alloc) noexcept
        : _alloc(alloc) {
        // Resize to match the other
        resize(other.size(), uninit);
        // Copy the data
        const auto my_stop = data_end();
        auto       my_it   = data();
        auto       ot_it   = other.data();
        for (; my_it != my_stop; ++my_it, ++ot_it) {
            *my_it = *ot_it;
        }
    }

    /**
     * Move-construct from another byte array.
     */
    constexpr basic_bytes(basic_bytes&& other) noexcept
        : _bytes_ptr(other.data())
        , _size(other.size())
        , _alloc(other.get_allocator()) {
        // Clear the other
        other._bytes_ptr = nullptr;
        other._size      = 0;
    }

    /**
     * Move-construct from a byte array, but take a different allocator.
     */
    constexpr basic_bytes(basic_bytes&& other, const allocator_type& alloc) noexcept
        : _alloc(alloc)
        , _bytes_ptr(other.data())
        , _size(other.size()) {
        // Clear the other
        other._bytes_ptr = nullptr;
        other._size      = 0;
    }

    /**
     * Copy-assign from another byte array.
     */
    constexpr basic_bytes& operator=(const basic_bytes& other) noexcept {
        _clear();  // Clear before re-assigning our allocator
        // Take the allocator from the other
        _alloc = other.get_allocator();
        // Resize using the new allocator
        resize(other.size(), uninit);
        const auto stop = data_end();
        for (auto it = data(), other_it = other.data(); it != stop; ++it, ++other_it) {
            *it = *other_it;
        }
        return *this;
    }

    /**
     * Move-assign from another byte array.
     */
    constexpr basic_bytes& operator=(basic_bytes&& other) noexcept {
        _clear();  // Clear before re-assigning our allocator;
        // Take the data from the other
        _alloc     = other.get_allocator();
        _bytes_ptr = other.data();
        _size      = other._size();
        // Clear the other.
        other._bytes_ptr = nullptr;
        other._size      = 0;
        return *this;
    }

    /**
     * Get the allocator for this bytes object
     */
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept { return _alloc; }

    /**
     * Get the current size of the bytes object
     */
    [[nodiscard]] constexpr size_type size() const noexcept { return _size; }

    /**
     * Obtain a pointer to the beginning of the data.
     */
    [[nodiscard]] constexpr pointer       data() noexcept { return _bytes_ptr; }
    [[nodiscard]] constexpr const_pointer data() const noexcept { return _bytes_ptr; }
    /**
     * Obtain the past-the-end pointer to the data.
     */
    [[nodiscard]] constexpr pointer       data_end() noexcept { return data() + size(); }
    [[nodiscard]] constexpr const_pointer data_end() const noexcept { return data() + size(); }

    /**
     * Deallocate the bytes and set the size to zero.
     */
    constexpr void clear() noexcept { _clear(); }

    /**
     * Set every byte in the object to the given pattern byte.
     */
    constexpr void fill(value_type pat) noexcept {
        for (auto it = data(), stop = data_end(); it != stop; ++it) {
            *it = pat;
        }
    }

    /**
     * Resize the buffer. If grown, the new data will be filled with zero-bytes.
     */
    constexpr pointer resize(size_type size) noexcept { return resize(size, std::byte{0}); }

    /**
     * Resize the buffer. If grown, the new bytes will be filled with the given
     * pattern byte.
     */
    constexpr pointer resize(size_type size, value_type pattern) noexcept {
        const auto tail_ptr = resize(size, uninit);

        const auto stop = data_end();
        for (auto it = tail_ptr; it != stop; ++it) {
            *it = pattern;
        }

        return tail_ptr;
    }

    /**
     * Resize the buffer. If grown, the new bytes will be left uninitialized.
     */
    constexpr pointer resize(size_type size, uninit_t) noexcept { return _resize_uninit(size); }

    [[nodiscard]] constexpr friend bool operator==(const basic_bytes& lhs,
                                                   const_buffer       rhs) noexcept {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        for (auto it = lhs.data(), stop = lhs.data_end(), rhs_it = rhs.data(); it != stop;
             ++it, ++rhs_it) {
            if (*it != *rhs_it) {
                return false;
            }
        }
        return true;
    }
    [[nodiscard]] constexpr friend bool operator==(const basic_bytes& lhs,
                                                   const basic_bytes& rhs) noexcept {
        return lhs == const_buffer(rhs);
    }
    [[nodiscard]] constexpr friend bool operator==(const_buffer       lhs,
                                                   const basic_bytes& rhs) noexcept {
        return rhs == lhs;
    }
    [[nodiscard]] constexpr friend bool operator!=(const basic_bytes& lhs,
                                                   const basic_bytes& rhs) noexcept {
        return !(lhs == rhs);
    }
    [[nodiscard]] constexpr friend bool operator!=(const basic_bytes& lhs,
                                                   const_buffer       rhs) noexcept {
        return !(lhs == rhs);
    }
    [[nodiscard]] constexpr friend bool operator!=(const_buffer       lhs,
                                                   const basic_bytes& rhs) noexcept {
        return !(rhs == lhs);
    }
};

using bytes = basic_bytes<std::allocator<std::byte>>;

}  // namespace neo
