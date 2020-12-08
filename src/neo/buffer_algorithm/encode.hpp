#pragma once

#include <neo/buffer_range.hpp>
#include <neo/buffer_sink.hpp>

#include <neo/iterator_concepts.hpp>
#include <neo/out.hpp>

#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif

#if __cpp_lib_ranges
#include <ranges>
#endif

namespace neo {

// clang-format off
template <typename T>
concept buffer_encode_result =
    semiregular<T> &&
    requires (T res) {
        { res.bytes_written } -> alike<std::size_t>;
        { res.done() } -> simple_boolean;
    };

template <typename Enc, typename T>
concept buffer_encoder =
    neo::invocable<Enc, mutable_buffer, T&> &&
    buffer_encode_result<std::invoke_result_t<Enc, mutable_buffer, T&>>;

template <typename Enc, typename T>
    requires buffer_encoder<Enc, T>
using buffer_encode_result_t = std::invoke_result_t<Enc, mutable_buffer, T&>;
// clang-format on

/**
 * Base overload: Encode a single item into a buffer/range/sink
 */
template <buffer_output Out, typename T, buffer_encoder<T> Enc>
constexpr decltype(auto) buffer_encode(Enc&& enc, Out&& out_, T&& val)               //
    noexcept(noexcept(enc(mutable_buffer(), val)) && noexcept_buffer_output_v<Out>)  //
{
    if constexpr (single_buffer<Out>) {
        return enc(out_, val);
    } else {
        buffer_encode_result_t<Enc, T> result{};
        std::size_t                    total_written = 0;

        auto&& out = ensure_buffer_sink(out_);

        while (!result.done()) {
            auto next = out.prepare(1024);
            if (buffer_is_empty(next)) {
                break;
            }
            auto partial = buffer_encode(enc, next, val);
            total_written += partial.bytes_written;
            out.commit(partial.bytes_written);
            result = partial;
        }

        result.bytes_written = total_written;
        return result;
    }
}

/**
 * Range: Encode a range of objects (denoted by two iterators) into a buffer/range/sink
 */
template <buffer_output                          Out,
          input_iterator                         Iter,
          sentinel_for<Iter>                     Sentinel,
          buffer_encoder<iter_reference_t<Iter>> Enc>
constexpr decltype(auto) buffer_encode(Enc&&                      enc,
                                       Out&&                      out_,
                                       Iter                       it,
                                       Sentinel                   stop,
                                       neo::optional_output<Iter> it_dest = std::nullopt) {
    buffer_encode_result_t<Enc, iter_reference_t<Iter>> result{};
    std::size_t                                         total_written = 0;

    auto&& out = ensure_buffer_sink(out_);

    for (; it != stop; ++it) {
        auto next = out.prepare(1024);
        if (buffer_is_empty(next)) {
            break;
        }
        auto partial = buffer_encode(enc, out, *it);
        total_written += partial.bytes_written;
        result = partial;
    }

    it_dest.put(it);
    result.bytes_written = total_written;
    return result;
}

#if __cpp_lib_ranges
/**
 * Range: Encode a range of objects into a buffer/range
 */
template <buffer_output                                         Out,
          std::ranges::input_range                              Items,
          buffer_encoder<std::ranges::range_reference_t<Items>> Enc>
constexpr decltype(auto) buffer_encode(Enc&& enc, Out&& out, Items&& items) {
    return buffer_encode(enc, out, std::ranges::begin(items), std::ranges::end(items));
}
#endif

}  // namespace neo
