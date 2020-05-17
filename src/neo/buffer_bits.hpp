#pragma once

#include <neo/buffer_range.hpp>
#include <neo/bytewise_iterator.hpp>

#include <neo/assert.hpp>

#include <algorithm>
#include <cstdint>
#include <iterator>

namespace neo {

template <buffer_range Bufs>
class buffer_bits {
    bytewise_iterator<Bufs> _it;

    /**
     * With the three bytewise_iterators, we can treat the buffer sequence as a
     * byte sequence, which is much easier to deal with.
     *
     * We track the number of bits remaining in the current byte with
     * `_bits_left`, and this number is decreased as we advance
     * through the bits. When it reaches zero, we advance `_it` and reset it
     * back to eight.
     */

    /// The offset in the current byte (0 == high bit, 7 == low bit)
    std::uint8_t _bit_pos = 0;

    static constexpr std::byte _mask_n(std::size_t n) noexcept {
        // 0b0'0000'0001  <- 1
        // 0b0'0010'0000  <- 1 << 5 [for example]
        // 0b0'0001'1111  <- (1 << 5) - 1
        return static_cast<std::byte>((1 << n) - 1);
    }

public:
    constexpr buffer_bits() = default;
    constexpr buffer_bits(const Bufs& b)
        : _it(b) {}

    constexpr buffer_bits(bytewise_iterator<Bufs> it)
        : _it(it) {}

    /**
     * Read the next n_bits from the buffer, but don't change the read position
     */
    constexpr std::uint64_t peek(std::size_t count) const noexcept {
        neo_assert(expects,
                   count <= 64,
                   "`count` must be less than 65 (The maximum size of portable integers)",
                   count);

        // Calc how many bits remain in the current byte
        const auto bits_left = 8u - _bit_pos;

        if (count <= bits_left) {
            // If `count` is less than the number of bits left, we can do
            // simple bit swizzling
            auto b = *_it & _mask_n(bits_left);
            b >>= (bits_left - count);

            return static_cast<std::uint64_t>(b);
        }

        // Keep a separate iterator for iterating bytes
        auto it = _it;

        // Accumulate bits:
        std::uint64_t acc = 0;

        /// 1: count > bits_left means we can start by taking `bits_left` bits
        /// from the current byte
        acc |= static_cast<std::uint64_t>(*it & _mask_n(bits_left));
        count -= bits_left;
        // Advance by one byte
        ++it;

        /// 2: Consume whole bytes until count < bits_left
        while (count >= 8) {
            acc <<= 8;
            acc |= static_cast<std::uint64_t>(*it++);
            count -= 8;
        }

        /// 3: Take the final remaining bits that we want
        auto fin = count ? *it : std::byte();
        fin >>= (8 - count);
        acc <<= count;
        acc |= static_cast<std::uint64_t>(fin);
        return acc;
    }

    /**
     * Put the low `count` bits from `bits` into the output
     */
    constexpr void set(std::uint64_t bits, std::size_t count) noexcept {
        neo_assert(expects,
                   count <= 64,
                   "`count` must be less than 65 (The maximum size of portable integers)",
                   count);

        auto bits_left = 8u - _bit_pos;

        if (count < bits_left) {
            // Current byte:
            auto cur = *_it;
            // The offset from the low bit to the start of `count` low bits
            auto bit_off = 8u - count;
            // A mask that matches `count` bits at position `bit_off` from the high bit
            auto mask = _mask_n(count) << bit_off;
            // The bits from the original that we want to keep:
            auto keep_bits = cur & ~mask;
            // The shifted bits from `bits` that we will assign over the original
            auto shifted = std::byte(bits << bit_off) & mask;
            // Do it:
            *_it = keep_bits | shifted;
            return;
        }

        // Separated byte iterator
        auto it = _it;

        /// 1: count > bits_left means we can start by overwriting `bits_left`
        /// bits from the current position
        // Mask the low bits to re-assign
        auto mask      = _mask_n(bits_left);
        auto cur       = *it;
        auto keep_bits = cur & ~mask;
        // Get the bits from the source that we want to reassign,
        auto new_bits = std::byte(bits >> (count - bits_left)) & mask;
        *it           = keep_bits | new_bits;
        // Advance by one byte
        count -= bits_left;
        ++it;

        // Set entire bytes
        while (count >= 8u) {
            *it = std::byte((bits >> (count - bits_left)) & 0xff);
            ++it;
            count -= 8;
        }

        // Set the final bits
        cur       = *it;
        mask      = _mask_n(count);
        keep_bits = cur & ~(mask << (8u - count));
        new_bits  = (std::byte(bits) & mask) << (8u - count);
        *it       = keep_bits | new_bits;

        return;
    }

    /**
     * Advance through the next `count` bits
     */
    constexpr void skip(std::size_t count) noexcept {
        // For every time `count` goes into 8, advance one byte
        const auto n_bytes_adv = count / 8;
        _it += n_bytes_adv;
        // Advance through bits in the byte
        const auto remain = count % 8;
        // If we are advancing more than the remaining number of bits, advance one more
        _it += int(remain >= (8u - _bit_pos));
        _bit_pos = (_bit_pos + remain) % 8;
    }

    /**
     * Get the number of bits remaining until the byte boundary. (May return zero)
     */
    constexpr int bit_offset() const noexcept { return ((8 - _bit_pos) % 8); }

    /**
     * Skip trailing bits until the next byte boundary.
     *
     * Won't advance over a whole byte, so calling multiple times in sequence will have no effect.
     */
    constexpr void skip_to_byte_boundary() noexcept { skip(bit_offset()); }

    /**
     * Read and advance by `count` bits
     */
    constexpr std::uint64_t read(std::size_t count) noexcept {
        neo_assert(expects,
                   count <= 64,
                   "`count` must be less than 65 (The maximum size of portable integers)",
                   count);
        auto r = peek(count);
        skip(count);
        return r;
    }

    /**
     * Set the low `count` bits from `bits` into the output and advance by `count` bits.
     */
    constexpr void write(std::uint64_t bits, std::size_t count) noexcept {
        neo_assert(expects,
                   count <= 64,
                   "`count` must be less than 65 (The maximum size of portable integers)",
                   count);
        set(bits, count);
        skip(count);
    }

    /**
     * Get the byte iterator underneath this object.
     */
    constexpr auto inner_iterator() const noexcept { return _it; }
};

template <typename Bufs>
buffer_bits(const Bufs&) -> buffer_bits<Bufs>;

template <typename Bufs>
buffer_bits(bytewise_iterator<Bufs>) -> buffer_bits<Bufs>;

}  // namespace neo