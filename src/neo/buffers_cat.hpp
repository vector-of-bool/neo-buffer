#pragma once

#include <neo/buffer_algorithm/size.hpp>
#include <neo/buffer_range.hpp>

#include <neo/fwd.hpp>
#include <neo/iterator_facade.hpp>

#include <tuple>
#include <variant>

namespace neo {

/**
 * Create a buffer range that is the concatenation of some number of other
 * buffer ranges
 */
template <buffer_range... Bufs>
class buffers_cat {
    using buffer_tuple = std::tuple<Bufs...>;

    buffer_tuple _bufs;

public:
    /**
     * The type of buffer yielded by the buffer concatenation. If all inner
     * buffer ranges mutable_buffer_ranges, then this type is `mutable_buffer`,
     * otherwise it is `const_buffer`.
     */
    using buffer_type
        = std::conditional_t<(mutable_buffer_range<Bufs> && ...), mutable_buffer, const_buffer>;

    /// Default-construct the range. Requires all inner buffer ranges to be default-constructible
    constexpr buffers_cat() = default;

    /// Construct the concatenation of buffers from the given buffers.
    explicit constexpr buffers_cat(Bufs&&... bufs) requires(sizeof...(Bufs) != 0)
        : _bufs(NEO_FWD(bufs)...) {}

    class iterator : public iterator_facade<iterator> {
    private:
        const buffer_tuple* _bufs;

        struct total_end {
            constexpr bool operator==(total_end) const noexcept { return true; }
        };

        using variant_type = std::variant<buffer_range_iterator_t<Bufs>..., total_end>;

        variant_type _iter_var = total_end{};

        /**
         * "Become" an iterator to the beginning of the Nth buffer range.
         */
        template <std::size_t N>
        constexpr void _become() noexcept {
            if constexpr (N == sizeof...(Bufs)) {
                // We've reached the end of the tuple.
                _iter_var = total_end{};
            } else {
                auto&& nth_buffer = std::get<N>(*_bufs);
                if (buffer_size(nth_buffer) == 0) {
                    // The buffer at position N is empty. Skip over it
                    return _become<N + 1>();
                }
                // We have a non-empty buffer. Begin iterating through it.
                _iter_var.template emplace<N>(std::begin(nth_buffer));
            }
        }

        template <std::size_t N>
        constexpr void _increment() noexcept {
            if constexpr (N == sizeof...(Bufs)) {
                // We've been incremented past-the-end. No!
                neo_assert(expects,
                           false,
                           "Advanced past-the-end iterator in a buffers_cat type",
                           sizeof...(Bufs));
            } else {
                if (_iter_var.index() != N) {
                    // We aren't in the Nth state. Try the next one
                    return _increment<N + 1>();
                }

                // Advance this inner iterator at position N
                auto&& _inner_iter = std::get<N>(_iter_var);
                ++_inner_iter;
                if (_inner_iter == std::end(std::get<N>(*_bufs))) {
                    // We have reached the end of the range at N. Advance to the next one
                    _become<N + 1>();
                }
            }
        }

        template <std::size_t N>
        constexpr buffer_type _deref() const noexcept {
            if constexpr (N == sizeof...(Bufs)) {
                neo_assert(expects,
                           false,
                           "Dereference the past-the-end iterator in a buffers_cat type",
                           sizeof...(Bufs));
            } else {
                if (_iter_var.index() != N) {
                    return _deref<N + 1>();
                }
                return *std::get<N>(_iter_var);
            }
        }

    public:
        constexpr iterator() = default;

        explicit constexpr iterator(const buffer_tuple& tup)
            : _bufs(&tup) {
            // Start out by referring to the first buffer in the tuple
            _become<0>();
        }

        constexpr buffer_type dereference() const noexcept { return _deref<0>(); }
        constexpr void        increment() noexcept { _increment<0>(); }
        constexpr bool        equal_to(const iterator& other) const noexcept {
            return _iter_var == other._iter_var;
        }
    };

    constexpr iterator begin() const noexcept { return iterator(_bufs); }
    constexpr iterator end() const noexcept { return iterator(); }
};

template <typename... Buffers>
buffers_cat(Buffers&&...) -> buffers_cat<Buffers...>;

}  // namespace neo