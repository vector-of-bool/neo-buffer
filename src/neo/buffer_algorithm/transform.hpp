#pragma once

#include <neo/buffer_range_consumer.hpp>
#include <neo/dynamic_buffer.hpp>
#include <neo/io_buffer.hpp>

#include <neo/assert.hpp>
#include <neo/fwd.hpp>
#include <neo/ref.hpp>
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

static_assert(buffer_transform_result<proto_buffer_transform_result>);

struct proto_buffer_transformer {
    proto_buffer_transformer()  = delete;
    ~proto_buffer_transformer() = delete;

    proto_buffer_transform_result operator()(mutable_buffer mb, const_buffer cb);
};

static_assert(buffer_transformer<proto_buffer_transformer>);

/**
 * The base case transformation function. This transforms fixed-length buffer
 * sequences, no dynamic resizing required. Other buffer_transform overloads
 * are implemented in terms of multiple calls to this base case.
 */
template <mutable_buffer_range Out,
          buffer_range         In,
          typename... Args,
          buffer_transformer<Args...> Tr>
constexpr auto buffer_transform(Tr&& tr, Out&& out_, In&& in_, Args&&... args) {
    using result_type = buffer_transform_result_t<Tr, Args...>;
    buffer_range_consumer out{out_};
    buffer_range_consumer in{in_};
    result_type           acc_res;

    while (true) {
        auto part_out = out.next_contiguous();
        auto part_in  = in.next_contiguous();
        auto part_res = tr(part_out, part_in, args...);
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
                   (part_res.bytes_written == part_out.size()
                    || part_res.bytes_read == part_in.size()),
                   "neo::buffer_transform() encoundered a malformed data transformer. "
                   "The transformer was unable to completely consume either of the buffers "
                   "provided to it. This indicates a bug in the data transformer and is not "
                   "the result of user error.",
                   part_res.bytes_written,
                   part_out.size(),
                   part_res.bytes_read,
                   part_in.size());

        if (part_res.bytes_written == 0 && part_res.bytes_read == 0) {
            // No data was written or read. The transformer cannot make any further progress
            break;
        }
    }

    return acc_res;
}

/**
 * Apply a buffer transformation on the given input `in_`, writing output into the output buffer
 */
template <typename... Args,
          dynamic_output_buffer       Out,
          buffer_range                In,
          buffer_transformer<Args...> Tr>
constexpr auto buffer_transform(Tr&& tr, Out&& out, In&& in_, Args&&... args) {
    // The actual final result type:
    using result_type = buffer_transform_result_t<Tr, Args...>;
    // The growth size can vary based on the algorithm
    constexpr std::size_t growth_size = buffer_transform_dynamic_growth_hint_v<std::decay_t<Tr>>;
    static_assert(growth_size > 0);

    // The input consumer
    buffer_range_consumer in{in_};
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
        buffer_range_consumer next_out{next_out_area};
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
 * Apply a buffer transformation on the given input `in_`, writing output to the
 * `dyn_out` dynamic buffer. The `dyn_out` buffer will be extended. Prior
 * content in `dyn_out` will be untouched.
 */
template <typename... Args, dynamic_buffer Out, buffer_range In, buffer_transformer<Args...> Tr>
constexpr auto buffer_transform(Tr&& tr, Out&& dyn_out, In&& in, Args&&... args) {
    dynamic_io_buffer_adaptor io{dyn_out};
    auto                      ret = buffer_transform(tr, io, in, std::forward<Args>(args)...);
    io.shrink_uncommitted();
    return ret;
}

}  // namespace neo
