#pragma once

#include <neo/as_buffer.hpp>

#include <neo/assert.hpp>
#include <neo/iterator_concepts.hpp>
#include <neo/iterator_facade.hpp>
#include <neo/ref.hpp>

namespace neo {

template <typename Range>
requires(as_buffer_convertible<iter_value_t<decltype(std::begin(ref_v<Range>))>>)  //
    class buffer_range_adaptor {
public:
    using range_type     = std::remove_cvref_t<Range>;
    using inner_iterator = decltype(std::begin(ref_v<Range>));
    using inner_sentinel = decltype(std::end(ref_v<Range>));
    using value_type     = as_buffer_t<iter_value_t<inner_iterator>>;

private:
    [[no_unique_address]] wrap_refs_t<Range> _range;

    enum { uses_sentinel = !same_as<inner_iterator, inner_sentinel> };

    struct nothing {};
    struct sentinel_base {
        struct sentinel_type {};
    };

public:
    constexpr explicit buffer_range_adaptor() = default;
    constexpr explicit buffer_range_adaptor(Range&& rng)
        : _range(NEO_FWD(rng)) {}

    NEO_DECL_UNREF_GETTER(range, _range);

    class iterator : public iterator_facade<iterator> {
        [[no_unique_address]] inner_iterator _it;

    public:
        constexpr iterator() = default;
        constexpr explicit iterator(inner_iterator it)
            : _it(it) {}

        struct sentinel_type {};

        constexpr decltype(auto) dereference() const noexcept {
            if constexpr (uses_sentinel) {
                neo_assert(expects,
                           *this != sentinel_base::sentinel_type(),
                           "Dereferenced a past-the-end buffer_range_adaptor::iterator");
            }
            return neo::as_buffer(*_it);
        }

        constexpr void advance(std::ptrdiff_t off) noexcept(noexcept(_it += off))  //
            requires(random_access_iterator<inner_iterator>) {
            _it += off;
        }

        constexpr void increment() noexcept(noexcept(++_it)) {
            if constexpr (uses_sentinel) {
                neo_assert(expects,
                           !at_end(sentinel_type{}),
                           "Advanced a past-the-end buffer_range_adaptor::iterator");
            }
            ++_it;
        }

        constexpr void decrement() noexcept requires(bidirectional_iterator<inner_iterator>) {
            --_it;
        }

        constexpr bool operator==(iterator other) const noexcept { return _it == other._it; }
        constexpr bool operator==(sentinel_type) const noexcept requires(bool(uses_sentinel)) {
            return _it == inner_sentinel();
        }
    };

    constexpr iterator begin() noexcept(noexcept(std::begin(range()))) {
        return iterator(std::begin(range()));
    }
    constexpr auto end() noexcept {
        if constexpr (uses_sentinel) {
            return typename iterator::sentinel_type();
        } else {
            return iterator(std::end(range()));
        }
    }
};

template <typename Range>
requires as_buffer_convertible<iter_value_t<decltype(std::begin(ref_v<Range>))>>
buffer_range_adaptor(Range&& rng) -> buffer_range_adaptor<Range>;

}  // namespace neo
