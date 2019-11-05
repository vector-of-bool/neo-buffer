#pragma once

#include <cstddef>
#include <iterator>

namespace neo::detail {

class single_buffer_iter_sentinel {};

template <typename Buffer>
class single_buffer_iter {
    Buffer _buf;
    bool   _dead = false;

public:
    using difference_type   = std::ptrdiff_t;
    using value_type        = Buffer;
    using pointer           = const value_type*;
    using reference         = const value_type&;
    using iterator_category = std::forward_iterator_tag;

    constexpr single_buffer_iter(Buffer b) noexcept
        : _buf(b) {}

    constexpr reference operator*() const noexcept { return _buf; }
    constexpr pointer   operator->() const noexcept { return &_buf; }

    constexpr bool operator==(single_buffer_iter_sentinel) const noexcept { return _dead; }
    constexpr bool operator!=(single_buffer_iter_sentinel) const noexcept { return !_dead; }
    constexpr bool operator==(single_buffer_iter o) const noexcept { return _dead == o._dead; }
    constexpr bool operator!=(single_buffer_iter o) const noexcept { return !(*this == o); }

    constexpr single_buffer_iter operator++(int) noexcept {
        auto me = *this;
        _dead   = true;
        return me;
    }

    constexpr single_buffer_iter& operator++() noexcept {
        _dead = true;
        return *this;
    }
};

}  // namespace neo::detail
