#pragma once

#include <neo/buffer_concepts.hpp>
#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <algorithm>
#include <cstddef>
#include <cstring>

namespace neo {

/**
 * Return the number of individual buffers in the given buffer sequence
 */
template <const_buffer_sequence Seq>
constexpr std::size_t buffer_count(const Seq& seq) {
    return std::distance(buffer_sequence_begin(seq), buffer_sequence_end(seq));
}

/**
 * Obtain the length of a buffer sequence, in bytes.
 */
template <const_buffer_sequence Seq>
constexpr std::size_t buffer_size(const Seq& seq) {
    auto        iter = buffer_sequence_begin(seq);
    const auto  stop = buffer_sequence_end(seq);
    std::size_t size = 0;
    while (iter != stop) {
        size += static_cast<std::size_t>(iter->size());
        ++iter;
    }
    return size;
}

/**
 * Copy data from the source buffer into the destination buffer, with a maximum of `max_copy`. The
 * actual number of bytes that are copied is the minimum of the buffer sizes and `max_copy`. The
 * number of bytes copied is returned.
 */
constexpr std::size_t buffer_copy(mutable_buffer dest, const_buffer src, std::size_t max_copy) {
    // Calculate how much we should copy in this operation. It will be the minimum of the buffer
    // sizes and the maximum bytes we want to copy
    const auto n_to_copy = std::min(src.size(), std::min(dest.size(), max_copy));
    // Do the copy!
    std::memcpy(dest.data(), src.data(), n_to_copy);
    return n_to_copy;
}

/**
 * Copy data from the `src` buffer into the `dest` buffer, up to `max_copy`
 * bytes. The operation is bounds-checked, and the number of bytes copied is
 * returned.
 */
template <mutable_buffer_sequence MutableSeq, const_buffer_sequence ConstSeq>
constexpr std::size_t
buffer_copy(const MutableSeq& dest, const ConstSeq& src, std::size_t max_copy) {
    // Keep count of how many bytes remain
    auto remaining_to_copy = max_copy;
    // And how many bytes we have copied so far (to return later)
    std::size_t total_copied = 0;
    // Iterators into the destination
    auto       dest_iter = buffer_sequence_begin(dest);
    const auto dest_stop = buffer_sequence_end(dest);
    // Iterators from the source
    auto       src_iter = buffer_sequence_begin(src);
    const auto src_stop = buffer_sequence_end(src);

    // Because buffers may be unaligned, we need to keep track of if we are
    // part of the way into either buffer.
    std::size_t src_offset  = 0;
    std::size_t dest_offset = 0;

    // We copied buffers in pairs and advance the iterators as we consume them.
    while (dest_iter != dest_stop && src_iter != src_stop && remaining_to_copy) {
        // The source buffer, with our offset prefix removed:
        const_buffer src_buf = *src_iter + src_offset;
        // The dest buffer, with the offset prefix removed:
        mutable_buffer dest_buf = *dest_iter + dest_offset;

        // Do the copY
        const std::size_t n_copied = buffer_copy(dest_buf, src_buf, remaining_to_copy);
        // Accumulate
        total_copied += n_copied;
        // Decrement our remaining
        remaining_to_copy -= n_copied;
        // Advance the offsets
        src_offset += n_copied;
        dest_offset += n_copied;
        // If we've exhausted either buffer, advance the corresponding iterator and set the its
        // buffer offset back to zero.
        if (src_buf.size() == n_copied) {
            ++src_iter;
            src_offset = 0;
        }
        if (dest_buf.size() == n_copied) {
            ++dest_iter;
            dest_offset = 0;
        }
    }

    return total_copied;
}

/**
 * Copy the data from the `src` buffer sequence into the `dest` buffer sequence.
 * The operation is bounds-checked, and the number of bytes successfully copied
 * is returned.
 *
 * This overload is guaranteed to exhaust at least one of the source or
 * destination buffers.
 */
template <mutable_buffer_sequence MutableSeq, const_buffer_sequence ConstSeq>
constexpr std::size_t buffer_copy(const MutableSeq& dest, const ConstSeq& src) {
    auto src_size  = buffer_size(src);
    auto dest_size = buffer_size(dest);
    auto min_size  = (src_size > dest_size) ? dest_size : src_size;
    return buffer_copy(dest, src, min_size);
}

}  // namespace neo