#pragma once

#include <neo/as_buffer.hpp>
#include <neo/buffer_algorithm/size.hpp>
#include <neo/static_buffer_vector.hpp>

#include <neo/assert.hpp>

namespace neo {

/**
 * A `buffer_sequence_consumer` is a utility type that can be used to consume
 * buffer sequences of unknown size with arbitrary step sizes.
 */
template <typename BaseSequence>
class buffer_sequence_consumer {
private:
    using inner_buffer_iterator = buffer_sequence_iterator_t<BaseSequence>;
    using inner_buffer_sentinel = buffer_sequence_sentinel_t<BaseSequence>;

public:
    /// The type of buffer that will be yielded by this consumer.
    using buffer_type = iter_value_t<inner_buffer_iterator>;

private:
    inner_buffer_iterator _seq_it;
    inner_buffer_sentinel _seq_stop;

    std::size_t _size_remaining = 0;
    std::size_t _elem_offset    = 0;

    constexpr static std::size_t _small_size = 16;

public:
    template <decays_to<BaseSequence> Seq>
    constexpr explicit buffer_sequence_consumer(Seq&& seq)
        : _seq_it(buffer_sequence_begin(seq))
        , _seq_stop(buffer_sequence_end(seq))
        , _size_remaining(buffer_size(seq)) {}

    [[nodiscard]] constexpr std::size_t bytes_remaining() const noexcept { return _size_remaining; }
    [[nodiscard]] constexpr bool        empty() const noexcept { return bytes_remaining() == 0; }

    [[nodiscard]] constexpr auto prepare(std::size_t n_to_prepare) const noexcept {
        // Build a small vector of buffers from the whole sequence
        static_buffer_vector<buffer_type, _small_size> bufs;
        // Keep track of how far into the first buffer we are skippings
        auto elem_offset = _elem_offset;
        for (auto it = _seq_it;
             // Stop if we reach the end of the base sequence
             it != _seq_stop &&
             // Or we have prepared every byte
             n_to_prepare != 0 &&
             // Or there is no more room in the temp buffer array
             bufs.size() < bufs.max_size();
             ++it) {
            // Store a buffer element. The first element may need to be offset into
            auto& buf = bufs.push_back(*it + elem_offset);
            // Decrement the max size by the number byte in that buffer
            n_to_prepare -= buf.size();
            // After the first, never advance the buffer.
            elem_offset = 0;
        }
        return bufs;
    }

    [[nodiscard]] constexpr buffer_type next_contiguous() const noexcept {
        if (_seq_it == _seq_stop) {
            return buffer_type();
        }
        return *_seq_it + _elem_offset;
    }

    constexpr void consume(std::size_t size) noexcept {
        neo_assert(
            expects,
            size <= bytes_remaining(),
            "Attempted to consume more bytes than are available in a buffer_sequence_consumer",
            size,
            bytes_remaining());
        _size_remaining -= size;
        while (_seq_it != _seq_stop && size != 0) {
            auto buf = next_contiguous();
            if (size < buf.size()) {
                // We're partially-consuming a buffer. Next time we iterate, we
                // need to skip into that buffer.
                _elem_offset += size;
                size = 0;
            } else {
                // Discard the entire buffer
                size -= buf.size();
                _elem_offset = 0;
                ++_seq_it;
            }
        }
    }
};

template <as_buffer_convertible T>
class buffer_sequence_consumer<T> {
public:
    using buffer_type = as_buffer_t<T>;

private:
    buffer_type _buf;

public:
    constexpr buffer_sequence_consumer(buffer_type b)
        : _buf(b) {}

    [[nodiscard]] constexpr auto prepare(std::size_t n) const noexcept {
        return as_buffer(_buf, n);
    }
    constexpr void consume(std::size_t n) noexcept { _buf += n; }

    [[nodiscard]] constexpr buffer_type next_contiguous() const noexcept { return _buf; }

    [[nodiscard]] constexpr std::size_t bytes_remaining() const noexcept { return _buf.size(); }
    [[nodiscard]] constexpr bool        empty() const noexcept { return bytes_remaining() == 0; }
};

template <typename T>
buffer_sequence_consumer(T &&) -> buffer_sequence_consumer<std::remove_reference_t<T>>;

}  // namespace neo
