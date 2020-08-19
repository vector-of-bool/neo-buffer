#pragma once

#include <neo/buffer_sink.hpp>
#include <neo/buffers_consumer.hpp>
#include <neo/io_buffer.hpp>

#include <neo/assert.hpp>
#include <neo/fwd.hpp>
#include <neo/ref.hpp>
#include <neo/returns.hpp>
#include <neo/test_concept.hpp>

#include <functional>

namespace neo {

// clang-format off
template <typename T>
concept buffer_transform_result =
    semiregular<T> &&
    requires(const T c_res, T res) {
        { res += c_res } noexcept;
        { c_res.bytes_written } -> alike<std::size_t>;
        { c_res.bytes_read } -> alike<std::size_t>;
        { c_res.done } -> alike<bool>;
    };

template <typename T, typename... MoreArgs>
concept buffer_transformer =
    invocable<T, mutable_buffer, const_buffer, MoreArgs...> &&
    buffer_transform_result<std::invoke_result_t<T, mutable_buffer, const_buffer, MoreArgs...>>;
// clang-format on

template <typename T>
constexpr std::size_t buffer_transform_dynamic_growth_hint_v = 1024;

template <typename T, typename... Args>
using buffer_transform_result_t
    = decltype(ref_v<T>(mutable_buffer(), const_buffer(), ref_v<Args>...));

struct proto_buffer_transform_result {
    proto_buffer_transform_result& operator+=(const proto_buffer_transform_result&) noexcept;

    std::size_t bytes_written;
    std::size_t bytes_read;
    bool        done;
};

struct simple_transform_result {
    std::size_t bytes_written = 0;
    std::size_t bytes_read    = 0;
    bool        done          = false;

    constexpr simple_transform_result& operator+=(simple_transform_result o) noexcept {
        bytes_written += o.bytes_written;
        bytes_read += o.bytes_read;
        done = done || o.done;
        return *this;
    }
};

struct proto_buffer_transformer {
    proto_buffer_transformer()  = delete;
    ~proto_buffer_transformer() = delete;

    proto_buffer_transform_result operator()(mutable_buffer mb, const_buffer cb);
};

/**
 * The base case of buffer_transform, transforms individual single buffers. It
 * is guaranteed that either the entire input or the entire output is consumed,
 * or the transformer declares that it has no more work to do. (This is guarded
 * by an assertion)
 */
template <typename... Args, buffer_transformer<Args...> Tr>
constexpr auto buffer_transform(Tr&& tr, mutable_buffer out, const_buffer in, Args&&... args) {
    auto result = tr(out, in, args...);

    // If the transformer declares that it is done, then we shouldn't check that
    // it consumed entire buffers.
    if (result.done) {
        return result;
    }

    /**
     * The transformer has not declared that it is done, but it *must* have
     * consumed either the entire input or the entire output. If not, then
     * the transformer is ill-formed. The next iteration around, we will
     * call the transformer with the same tail of buffers that it left
     * behind. If it couldn't finish the buffers this time around, then
     * there is no reason to hope that it will make any progress on the next
     * iteration. Halt!
     *
     * Note that this _does_ consider the case of empty input or output
     * buffers, in which case we will break out.
     */
    neo_assert(expects,
               (result.bytes_written == out.size() || result.bytes_read == in.size()),
               "neo::buffer_transform() encoundered a malformed data transformer. "
               "The transformer was unable to completely consume either of the buffers "
               "provided to it. This indicates a bug in the data transformer and is not "
               "the result of user error.",
               result.bytes_written,
               out.size(),
               result.bytes_read,
               in.size());

    // We have a partial result, but that's okay.
    return result;
}

/**
 * Catch cast of mutable_buffer -> mutable_buffer, lower to
 * const_buffer -> mutable_buffer.
 */
template <typename... Args, buffer_transformer<Args...> Tr>
constexpr auto buffer_transform(Tr&& tr, mutable_buffer out, mutable_buffer in, Args&&... args)
    NEO_RETURNS(buffer_transform(tr, out, const_buffer(in), args...));

/**
 * This overload transforms fixed-length buffer sequences, no dynamic resizing
 * required. Other buffer_transform overloads may be implemented in terms of
 * multiple calls to this case.
 */
template <mutable_buffer_range Out,
          buffer_range         In,
          typename... Args,
          buffer_transformer<Args...> Tr>
constexpr auto buffer_transform(Tr&& tr, Out&& out_, In&& in_, Args&&... args) {
    using result_type = buffer_transform_result_t<Tr, Args...>;
    buffers_consumer out{out_};
    buffers_consumer in{in_};
    result_type      acc_res;

    while (true) {
        mutable_buffer part_out = out.next_contiguous();
        const_buffer   part_in  = in.next_contiguous();
        result_type    part_res = buffer_transform(tr, part_out, part_in, args...);
        out.consume(part_res.bytes_written);
        in.consume(part_res.bytes_read);
        acc_res += part_res;

        if (in.empty() && out.empty()) {
            // Input is empty and output is full. No progress can be made
            break;
        }

        if (acc_res.done) {
            // The transformer has declared that it has completed
            break;
        }

        if (part_res.bytes_written == 0 && part_res.bytes_read == 0) {
            // No data was written or read. The transformer cannot make any further progress
            break;
        }
    }

    return acc_res;
}

/**
 * Case: Given a buffer range as input, and a buffer_sink as the output. This case
 * will grow the sink as it needs to transform the entire input. Stops when:
 *  - The input is gone,
 *  - The sink fails to prepare room for more output,
 *  - or the transformer declares that it is done.
 */
template <typename... Args, buffer_sink Out, buffer_range In, buffer_transformer<Args...> Tr>
constexpr auto buffer_transform(Tr&& tr, Out&& out, In&& in_, Args&&... args) {
    // The actual final result type:
    using result_type = buffer_transform_result_t<Tr, Args...>;
    // The growth size can vary based on the algorithm
    constexpr std::size_t growth_size
        = buffer_transform_dynamic_growth_hint_v<std::remove_cvref_t<Tr>>;
    static_assert(growth_size > 0);

    // The input consumer
    buffers_consumer in{in_};
    // The result accumulator
    result_type acc_res;

    // We are a simple state machine with three states:
    enum tr_state_t {
        // Init: Grow the output buffer to fit more data. Becomes `more_input`
        grow_output,
        // Feed more data into the transformer. Becomes `stop` or `grow_output`,
        // depending on whether there is still work to be done.
        more_input,
        // Fully stop transforming data. Shrinks the output to fit the total
        // bytes written and returns the accumulated transform result.
        stop,
    } tr_state
        = grow_output;

    while (tr_state == grow_output) {
        // Prepare the next output area
        auto next_out_area = out.prepare(growth_size);
        // A consumer for that output area
        buffers_consumer next_out{next_out_area};
        // Keep track of how much we actually transform on this loop step
        std::size_t n_written_this_step = 0;

        // Inner logic: Sends data to the transformer
        auto do_transform_more = [&]() -> tr_state_t {
            // Get the next contiguous input or output buffers
            mutable_buffer out_buf = next_out.next_contiguous();
            const_buffer   in_buf  = in.next_contiguous();
            // We must have non-empty buffers:
            neo_assert(invariant,
                       !out_buf.empty() || !in_buf.empty(),
                       "Empty buffers appeared unexpectedly.");
            // Apply a partial transformation:
            auto part_res = buffer_transform(tr, out_buf, in_buf, args...);

            // Accumulate the partial result into the final result
            acc_res += part_res;

            if (part_res.bytes_read == 0 && part_res.bytes_written == 0) {
                // The transformer made no progress on this pass. It requires more input and has
                // nothing to write in output.
                return stop;
            }

            // We've written more data into the output:
            n_written_this_step += part_res.bytes_written;
            // Advance the buffers:
            out_buf += part_res.bytes_written;
            in_buf += part_res.bytes_read;
            // The transformer must have completely consumed one of the two buffers:
            neo_assert(invariant,
                       out_buf.empty() || in_buf.empty(),
                       "The lower-level buffer_transform() should have exhausted one of the two "
                       "input or output buffers",
                       out_buf.size(),
                       in_buf.size());

            if (acc_res.done) {
                // The transformer can make no further progress
                return stop;
            }

            in.consume(part_res.bytes_read);
            next_out.consume(part_res.bytes_written);
            if (next_out.empty()) {
                // There is no more room in the output buffer sequence. Break out to grow the buffer
                return grow_output;
            }

            return more_input;
        };

        for (tr_state = more_input; tr_state == more_input;) {
            n_written_this_step = 0;
            tr_state            = do_transform_more();
            out.commit(n_written_this_step);
        }
        neo_assert(invariant,
                   tr_state == grow_output || tr_state == stop,
                   "Invalid state transition in buffer_transform()",
                   tr_state);
    }

    return acc_res;
}

/**
 * Case: Given some valid output, and a buffer_source as the input. Repeatedly
 * pulls data from the source and transforms it into the output. Stops when:
 *  - The source fails to produce more data,
 *  - or the lower-level transform does not consume an entire input buffer range,
 *  - or the transformer declares that it is done.
 */
template <typename... Args, typename Out, buffer_source In, buffer_transformer<Args...> Tr>
constexpr auto buffer_transform(Tr&& tr, Out&& out, In&& in, Args&&... args)  //
    requires requires {
    buffer_transform(tr, out, in.next(1), args...);
}
{
    using result_type               = buffer_transform_result_t<Tr, Args...>;
    constexpr std::size_t read_size = 1024;

    result_type res_acc;

    while (true) {
        auto       more_input = in.next(read_size);
        const auto in_size    = buffer_size(more_input);
        if (in_size == 0) {
            // There is nothing more to read
            break;
        }
        // Transform this input segment:
        const auto part_res = buffer_transform(tr, out, more_input, args...);
        // Accumulate:
        res_acc += part_res;
        // Discard the bytes that we fed down:
        in.consume(part_res.bytes_read);
        if (part_res.bytes_read != in_size) {
            // The transformer did not consume the entire input buffer. This means
            // that it needs more room in the output and cannot take any more. Stop.
            break;
        }
        if (part_res.done) {
            // The transformer has signalled us to stop
            break;
        }
        // The entire input buffer was consumed. Go around again to read more data
    }
    return res_acc;
}

}  // namespace neo
