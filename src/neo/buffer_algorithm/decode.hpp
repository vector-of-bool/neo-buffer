#pragma once

#include <neo/buffer_range.hpp>
#include <neo/buffer_source.hpp>

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
concept buffer_decode_result =
    semiregular<T> &&
    requires (T res) {
        { res.value() } noexcept;
        { res.has_value() } -> simple_boolean;
        { res.has_error() } -> simple_boolean;
        { res.bytes_read } -> alike<std::size_t>;
    };

template <typename T>
concept buffer_decoder =
    neo::invocable<T, const_buffer> &&
    buffer_decode_result<std::invoke_result_t<T, const_buffer>>;

template <buffer_decoder D>
using buffer_decode_result_t = std::invoke_result_t<D, const_buffer>;

template <buffer_decoder D>
using buffer_decode_type_t = decltype(std::declval<buffer_decode_result_t<D>>().value());
// clang-format on

/**
 * Base case: Decode a single item from a buffer input
 */
template <buffer_decoder Dec, buffer_input Source>
constexpr decltype(auto) buffer_decode(Dec&& decode, Source&& source)  //
    noexcept(noexcept(decode(const_buffer())) && noexcept_buffer_input_v<Source>) {
    if constexpr (single_buffer<Source>) {
        return decode(const_buffer(source));
    } else {
        buffer_decode_result_t<Dec> result;
        std::size_t                 total_read = 0;

        auto&& in = ensure_buffer_source(source);

        while (true) {
            auto next = in.next(1024);
            if (buffer_is_empty(next)) {
                break;
            }
            auto partial = buffer_decode(decode, next);
            total_read += partial.bytes_read;
            in.consume(partial.bytes_read);

            result = std::move(partial);
            if (result.has_value() || result.has_error()) {
                break;
            }
        }

        result.bytes_read = total_read;
        return result;
    }
}

// clang-format off
/**
 * Decode the result into an output parameter.
 */
template <buffer_input Source, typename T, buffer_decoder Decode>
constexpr decltype(auto) buffer_decode(Decode&& dec,
                                       Source&& source,
                                       neo::output<T> out)
    noexcept(noexcept(buffer_decode(dec, source)))
    requires requires(buffer_decode_type_t<Decode> val) {
        out.put(NEO_FWD(val));
    }
{
    // clang-format on
    auto result = buffer_decode(dec, source);
    if (result.has_value()) {
        out.put(NEO_FWD(result.value()));
    }
    return result;
}

/**
 * Decode the result into an output range denoted by two iterators.
 */
template <buffer_input                                  Source,
          buffer_decoder                                Decode,
          output_iterator<buffer_decode_type_t<Decode>> Iter,
          sentinel_for<Iter>                            Sentinel>
constexpr decltype(auto) buffer_decode(Decode&& dec,
                                       Source&& source,
                                       Iter     out,
                                       Sentinel stop)  //
    noexcept(noexcept(buffer_decode(dec, source))) {
    buffer_decode_result_t<Decode> result{};
    std::size_t                    total_read = 0;

    auto&& in = ensure_buffer_source(source);

    for (; out != stop; ++out) {
        auto next = in.next(1024);
        if (buffer_is_empty(next)) {
            break;
        }
        auto partial_result = buffer_decode(dec, next);
        total_read += partial_result.bytes_read;
        in.consume(partial_result.bytes_read);
        result = partial_result;
        if (result.has_value()) {
            *out = std::move(result.value());
        }
        if (result.has_error()) {
            break;
        }
    }
    result.bytes_read = total_read;
    return result;
}

#if __cpp_lib_ranges
/**
 * Decode results into an output range.
 */
template <buffer_input                                            Source,
          buffer_decoder                                          Decode,
          std::ranges::output_range<buffer_decode_type_t<Decode>> Out>
constexpr decltype(auto) buffer_decode(Decode&& dec, Source&& source, Out&& out) noexcept(
    noexcept(buffer_decode(dec, source, std::ranges::begin(out), std::ranges::end(out)))) {
    return buffer_decode(dec, source, std::ranges::begin(out), std::ranges::end(out));
}
#endif

}  // namespace neo
