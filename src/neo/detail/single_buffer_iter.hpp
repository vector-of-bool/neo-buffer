#pragma once

#include <neo/iterator_facade.hpp>

#include <neo/assert.hpp>

#include <cstddef>
#include <iterator>

namespace neo::detail {

class single_buffer_iter_sentinel {};

template <typename Buffer>
class single_buffer_iter : public iterator_facade<single_buffer_iter<Buffer>> {
    Buffer _buf;
    bool   _dead = false;

public:
    using sentinel_type = single_buffer_iter_sentinel;

    constexpr single_buffer_iter() = default;

    constexpr explicit single_buffer_iter(Buffer b) noexcept
        : _buf(b) {}

    constexpr Buffer dereference() const noexcept { return _buf; }
    constexpr bool   operator==(single_buffer_iter o) const noexcept { return _dead == o._dead; }
    constexpr bool   operator==(sentinel_type) const noexcept { return _dead; }

    constexpr void increment() noexcept {
        neo_assert(expects, !_dead, "Advanced a single-buffer iterator that was already advanced");
        _dead = true;
    }

    constexpr void decrement() noexcept {
        neo_assert(expects, _dead, "Rewind a single-buffer iterator that hasn't been incremented.");
        _dead = false;
    }
};

}  // namespace neo::detail
