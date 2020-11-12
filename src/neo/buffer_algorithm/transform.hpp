#pragma once

#include <neo/buffer_sink.hpp>
#include <neo/buffer_source.hpp>
#include <neo/buffers_consumer.hpp>

#include <neo/assert.hpp>
#include <neo/fwd.hpp>
#include <neo/ref.hpp>

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
constexpr auto buffer_transform(Tr&& tr, mutable_buffer out, const_buffer in, Args&&... args)  //
    noexcept(noexcept(tr(out, in, args...)))                                                   //
{
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
constexpr auto buffer_transform(Tr&&           tr,
                                mutable_buffer out,
                                mutable_buffer in,
                                Args&&... args) noexcept(noexcept(tr(out, in, args...))) {
    return buffer_transform(tr, out, const_buffer(in), args...);
}

template <buffer_output Out, buffer_input In, typename... Args, buffer_transformer<Args...> Tr>
constexpr auto buffer_transform(Tr&& tr, Out&& out_, In&& in_, Args&&... args)  //
    noexcept(noexcept(tr(mutable_buffer(), const_buffer(), args...))
             && noexcept_buffer_input_v<In> && noexcept_buffer_output_v<Out>)  //
{
    using result_type = buffer_transform_result_t<Tr>;
    // The growth size can vary based on the algorithm
    constexpr std::size_t growth_size
        = buffer_transform_dynamic_growth_hint_v<std::remove_cvref_t<Tr>>;
    static_assert(growth_size > 0);

    auto&& in  = ensure_buffer_source(in_);
    auto&& out = ensure_buffer_sink(out_);

    result_type result_acc;

    while (true) {
        auto in_part        = in.next(growth_size);
        auto out_part       = out.prepare(growth_size);
        auto partial_result = buffer_transform(tr, out_part, in_part, args...);
        in.consume(partial_result.bytes_read);
        out.commit(partial_result.bytes_written);
        result_acc += partial_result;
        if (result_acc.done) {
            break;
        }
        neo_assert(invariant,
                   partial_result.bytes_read == buffer_size(in_part)
                       || partial_result.bytes_written == buffer_size(out_part),
                   "The lower-level buffer_transform() should hae exhausted at least one of the "
                   "input or output buffers",
                   buffer_size(in_part),
                   buffer_size(out_part),
                   partial_result.bytes_read,
                   partial_result.bytes_written);
        if (partial_result.bytes_read == 0 && partial_result.bytes_written == 0) {
            break;
        }
    }

    return result_acc;
}

}  // namespace neo
