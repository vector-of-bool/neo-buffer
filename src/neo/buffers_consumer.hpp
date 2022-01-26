#pragma once

#include <neo/as_buffer.hpp>
#include <neo/buffer_algorithm/size.hpp>
#include <neo/buffer_range.hpp>
#include <neo/static_buffer_vector.hpp>

#include <neo/assert.hpp>

#include <limits>

namespace neo {

/**
 * A `buffers_consumer` is a utility type that can be used to consume
 * buffer sequences of unknown size with arbitrary step sizes.
 */
template <buffer_range BaseRange>
class buffers_consumer {
private:
    using inner_buffer_iterator = buffer_range_iterator_t<BaseRange>;
    using inner_buffer_sentinel = buffer_range_sentinel_t<BaseRange>;

public:
    /// The type of buffer that will be yielded by this consumer.
    using buffer_type = buffer_range_value_t<BaseRange>;

protected:
    [[no_unique_address]] wrap_ref_member_t<BaseRange> _range;

    [[no_unique_address]] inner_buffer_iterator _seq_it   = std::begin(unref(_range));
    [[no_unique_address]] inner_buffer_sentinel _seq_stop = std::end(unref(_range));

    std::size_t _cur_elem_offset = 0;
    std::size_t _remaining;

public:
    constexpr buffers_consumer() = default;

    constexpr explicit buffers_consumer(BaseRange&& rng)
        : buffers_consumer(NEO_FWD(rng), std::numeric_limits<std::size_t>::max()) {}

    constexpr buffers_consumer(BaseRange&& rng, std::size_t clamp_size)
        : _range(NEO_FWD(rng))
        , _remaining(clamp_size) {}

    [[nodiscard]] constexpr bool empty() const noexcept {
        return _seq_it == _seq_stop || _remaining == 0;
    }

    [[nodiscard]] constexpr auto prepare(std::size_t n) const noexcept
        requires(mutable_buffer_range<BaseRange>) {
        n = (std::min)(n, _remaining);
        return next(n);
    }

    [[nodiscard]] constexpr auto next(std::size_t n) const noexcept {
        if (_seq_it == _seq_stop) {
            return buffer_type();
        }
        n = (std::min)(n, _remaining);
        return as_buffer(*_seq_it + _cur_elem_offset, n);
    }

    constexpr void consume(std::size_t size) noexcept {
        const auto consume_size = size;
        neo_assert(expects,
                   !empty() || size == 0,
                   "Attempted to consume more bytes than are available in a buffers_consumer",
                   size);
        while (_seq_it != _seq_stop && size != 0) {
            auto buf = *_seq_it + _cur_elem_offset;
            if (size < buf.size()) {
                // We're partially-consuming a buffer. Next time we iterate, we
                // need to skip into that buffer.
                _cur_elem_offset += size;
                _remaining -= size;
                size = 0;
            } else {
                // Discard the entire buffer
                size -= buf.size();
                _remaining -= buf.size();
                _cur_elem_offset = 0;
                ++_seq_it;
            }
        }
        neo_assert(expects,
                   size == 0,
                   "Attempted to consume more bytes than are available in a buffers_consumer",
                   consume_size);
    }

    constexpr void commit(std::size_t size) noexcept requires(mutable_buffer_range<BaseRange>) {
        consume(size);
    }
};

template <single_buffer T>
class buffers_consumer<T> {
public:
    using buffer_type = as_buffer_t<T>;

private:
    buffer_type _buf;

public:
    constexpr explicit buffers_consumer(buffer_type b)
        : _buf(b) {}

    constexpr buffers_consumer(buffer_type b, std::size_t clamp)
        : _buf(as_buffer(b, clamp)) {}

    [[nodiscard]] constexpr auto prepare(std::size_t n) const noexcept
        requires(mutable_buffer_range<T>) {
        return as_buffer(_buf, n);
    }
    [[nodiscard]] constexpr auto next(std::size_t n) const noexcept { return as_buffer(_buf, n); }

    constexpr void consume(std::size_t n) noexcept { _buf += n; }
    constexpr void commit(std::size_t n) noexcept requires(mutable_buffer_range<T>) { consume(n); }

    [[nodiscard]] constexpr bool empty() const noexcept { return _buf.empty(); }
};

template <typename T>
buffers_consumer(T&&) -> buffers_consumer<T>;

template <typename T>
buffers_consumer(T&&, std::size_t) -> buffers_consumer<T>;

template <buffer_range BaseRange>
class buffers_vec_consumer : public buffers_consumer<BaseRange> {
    constexpr static std::size_t _small_size = 16;

public:
    using buffer_type = typename buffers_vec_consumer::buffers_consumer::buffer_type;
    using buffers_vec_consumer::buffers_consumer::buffers_consumer;

    // Pull the base version of 'next' into scope so that it can be called in
    // the case that the BaseRange is actually a single_buffer, and next() will
    // just return a single contiguous buffer.
    using buffers_vec_consumer::buffers_consumer::next;

    [[nodiscard]] constexpr auto next(std::size_t n_to_prepare) noexcept
        requires(!single_buffer<BaseRange>) {
        // Build a small vector of buffers from the whole sequence
        static_buffer_vector<buffer_type, _small_size> bufs;
        // Keep track of how far into the first buffer we are skipping
        auto elem_offset = this->_cur_elem_offset;
        // Clamp to the max we are allowed to consume
        n_to_prepare = (std::min)(n_to_prepare, this->_remaining);
        for (auto it = this->_seq_it;
             // Stop if we reach the end of the base sequence
             it != this->_seq_stop &&
             // Or we have prepared every byte
             n_to_prepare != 0 &&
             // Or there is no more room in the temp buffer array
             bufs.size() < bufs.max_size();
             ++it) {
            // Store a buffer element. The first element may need to be offset into, and we may
            // need to clamp it
            auto buf = bufs.push_back(as_buffer(*it + elem_offset, n_to_prepare));
            // Decrement the max size by the number byte in that buffer
            n_to_prepare -= buf.size();
            // After the first, never advance the buffer.
            elem_offset = 0;
        }
        return bufs;
    }

    [[nodiscard]] constexpr auto prepare(std::size_t s) noexcept
        requires(mutable_buffer_range<BaseRange>) {
        return next(s);
    }
};

template <typename T>
buffers_vec_consumer(T&&) -> buffers_vec_consumer<T>;

template <typename T>
buffers_vec_consumer(T&&, std::size_t) -> buffers_vec_consumer<T>;

}  // namespace neo
