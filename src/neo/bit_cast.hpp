#pragma once

#include <neo/as_buffer.hpp>
#include <neo/buffer_algorithm/copy.hpp>

#include <neo/assert.hpp>
#include <neo/concepts.hpp>

namespace neo {

/**
 * Convert an object of type `Arg` to an object of type `T` by copying the bytes
 * from `arg` into the bytes of a default-constructed `T`
 */
template <neo::trivial_type T, neo::trivially_copyable Arg>
requires(sizeof(T) == sizeof(Arg))  //
    constexpr T bit_cast(const Arg& arg) noexcept {
    T dest;
    neo::buffer_copy(neo::trivial_buffer(dest), neo::trivial_buffer(arg), ll_buffer_copy_forward);
    return dest;
}

/**
 * Create an object of type `T` from the bytes of an object representation of a
 * `T` that is referenced by `buf`. Asserts that `buffer_size(buf) >= sizeof(T)`.
 * Excess bytes in `buf` are ignored.
 */
template <neo::trivial_type T, buffer_range Buf>
constexpr T buffer_bit_cast(Buf&& buf) noexcept {
    neo_assert(expects,
               buffer_size(buf) >= sizeof(T),
               "Attempted buffer_bit_cast from a too-small buffer to a too-large type",
               sizeof(T),
               buffer_size(buf));
    auto dest = T();
    buffer_copy(trivial_buffer(dest), buf, ll_buffer_copy_forward);
    return dest;
}

}  // namespace neo
