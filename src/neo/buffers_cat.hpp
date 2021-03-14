#pragma once

#include <neo/buffer_algorithm/size.hpp>
#include <neo/buffer_range.hpp>

#include <neo/fwd.hpp>
#include <neo/iterator_facade.hpp>

#include <array>
#include <tuple>
#include <variant>

namespace neo {

namespace detail {

/// Forward-decl only. See the detail {} section below for more information.
template <buffer_range... Ts>
struct bufcat_impl;

}  // namespace detail

/**
 * buffers_seq_concat is a tuple-like type that creates a buffer sequences representing
 * the concatenation of other buffer sequences.
 */
template <buffer_range... Bufs>
class buffers_seq_concat {
    /// The actual tuple type
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
    constexpr buffers_seq_concat() = default;

    /// Construct the concatenation of buffers from the given buffers.
    /// XXX: A bug on MSVC causes the default constructor to collide with the N-ary constructor for
    /// N=0, as the attached `requires` clause is not evaluated at the right time.
    explicit constexpr buffers_seq_concat(Bufs&&... bufs) requires(sizeof...(Bufs) != 0)
        : _bufs(NEO_FWD(bufs)...) {}

    class iterator : public iterator_facade<iterator> {
    private:
        const buffer_tuple* _bufs;

        struct total_end {
            constexpr bool operator==(total_end) const noexcept { return true; }
        };

        using variant_type
            = std::variant<buffer_range_iterator_t<make_cref_t<Bufs&>>..., total_end>;

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
                           "Advanced past-the-end iterator in a buffers_seq_concat type",
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
                neo_assert_always(
                    expects,
                    false,
                    "Dereference the past-the-end iterator in a buffers_seq_concat type",
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
        constexpr bool        operator==(const iterator& other) const noexcept {
            return _iter_var == other._iter_var;
        }
    };

    constexpr iterator begin() const noexcept { return iterator(_bufs); }
    constexpr iterator end() const noexcept { return iterator(); }

    constexpr const buffer_tuple& tuple() const& noexcept { return _bufs; }
    constexpr buffer_tuple&&      tuple() && noexcept { return std::move(_bufs); }
};

template <typename... Buffers>
buffers_seq_concat(Buffers&&...) -> buffers_seq_concat<Buffers...>;

template <buffer_range... Bufs>
constexpr decltype(auto) buffers_cat(Bufs&&... bufs) {
    return detail::bufcat_impl<Bufs...>::concat(NEO_FWD(bufs)...);
}

template <buffer_range... Bs>
using buffers_cat_t = decltype(buffers_cat(std::declval<Bs>()...));

namespace detail {

/// Detect if `T` is a std::array of a buffer type
template <typename T>
constexpr inline bool is_buffer_array_v = false;

template <single_buffer Buf, std::size_t N>
constexpr inline bool is_buffer_array_v<std::array<Buf, N>> = true;

/// Utility concept to match std::array of single buffers
template <typename T>
concept buffer_std_array
    = buffer_range<T> && (std::is_array_v<T> || is_buffer_array_v<std::remove_cvref_t<T>>);

/// Detect of `T` is a buffer_seq_concat specialization
template <typename T>
constexpr inline bool is_buffer_concat_v = false;

template <typename... Ts>
constexpr inline bool is_buffer_concat_v<buffers_seq_concat<Ts...>> = true;

/// Utility concept to match specializations of buffers_seq_concat
template <typename T>
concept match_buffer_seq_concat = buffer_range<T>&& is_buffer_concat_v<std::remove_cvref_t<T>>;

/// Utility concept to match buffer sequence types that can be concatenated "simply" without the
/// need for buffers_seq_concat.
template <typename T>
concept match_simple_buffer_concat = buffer_std_array<T> || single_buffer<T>;

/// Constexpr variable template that determines the size of a std::array, or 1 for single buffers
template <match_simple_buffer_concat T>
constexpr std::size_t cexpr_buf_count = 99999;  // Never used

template <single_buffer B>
constexpr std::size_t cexpr_buf_count<B> = 1;

template <buffer_std_array B>
constexpr std::size_t cexpr_buf_count<B> = std::tuple_size<std::remove_cvref_t<B>>::value;

/**
 * The actual implementation of buffer concatenation is in specializations of bufcat_impl.
 *
 * The following [partial] specializations are defined:
 *
 * 1. bufcat_impl<> - returns a std::array<mutable_buffer, 0>
 * 2. bufcat_impl<buffer_range T> - return `T` unchanged.
 * 3. bufcat_impl<simple_buffer A, simple_buffer B, Tail...> - Concats A and B into another
 *      simple_buffer, and then concats that result with `Tail...`
 * 4. bufcat_impl<buffer_range A, Tail...> - Runs concat on `Tail...` and then inserts `A`
 *      at the beginning. This is *less-specialized* than [3], allows us to concat simple_buffer
 *      sequences throughout the original template arguments when there may be non-simple buffer
 *      sequences within the template arguments.
 * 5. bufcat_impl<buffers_cat_seq A, buffers_cat_seq B, Tail...> - Joins A and B into a single
 *      buffers_cat_seq, then concats that result with `Tail...`
 *
 * For examples of what each specialization does, see the comments nearby their definition.
 */
template <buffer_range... Ts>
struct bufcat_impl;

/**
 * [1]: Empty concat case. We return a simple empty buffer range in the form of a
 *      zero-length `std::array`. The buffer type is always mutable_buffer, to not restrict
 *      those that concatenate it with another mutable_buffer range.
 */
template <>
struct bufcat_impl<> {
    static constexpr std::array<mutable_buffer, 0> concat() noexcept { return {}; }
};

/**
 * [2]: The single-range case. This is the fixed-point case for most processes, rather
 *      than bufcat_impl<>, since we want to do smarts depending on the types of the
 *      first two parameters.
 *
 *      This specialization simply returns the same B that it was given.
 */
template <buffer_range B>
struct bufcat_impl<B> {
    static constexpr B concat(B&& b) noexcept { return NEO_FWD(b); }
};

/**
 * [3]: Case: The first two ranges are "simple", and can be concatenated into another
 *      simple range. This prevents us from needing to pull out the heavy
 *      buffers_seq_concat.
 *
 * This specialization is defined in terms of recursion. on buffer concatenation.
 * Read the body comments to understand what it is doing
 */
template <match_simple_buffer_concat A, match_simple_buffer_concat B, buffer_range... Tail>
struct bufcat_impl<A, B, Tail...> {
    /**
     * We're building a `std::array` of buffers to represent the concatenation
     * of A and B. We're not dealing with `Tail...` until the very end.
     *
     * First, let's get the range size of the first two ranges:
     */
    static constexpr std::size_t arr_size = cexpr_buf_count<A> + cexpr_buf_count<B>;

    /**
     * The buffer type we store in the array is the common type of the buffer type of the two
     * ranges.
     */
    using arr_buf_type = std::common_type_t<buffer_range_value_t<A>, buffer_range_value_t<B>>;

    /**
     * We can now declare the array type as an array of the common buffer type,
     * with the size that is the sum of the two arrays.
     */
    using array_type = std::array<arr_buf_type, arr_size>;

    /**
     * Now query information about the recursive concatenation operation. The
     * next concat is to concat the `array_type` that we will generate from A
     * and B with the `Tail...`.
     *
     * At this point, We've folded A and B together into a single concatenation
     * operand. If the first of `Tail...` happens to be another simple_buffer,
     * we'll go on to recurse on our own specialization and further reduce the
     * number of operands. If not, we'll pick another specialization that knows
     * what to do with its arguments.
     *
     * In essence, as long as the leading operands of the concatenation are simple,
     * we'll recursively fold them together until we have one std::array that
     * contains the results of those operations.
     */
    using result_type = buffers_cat_t<array_type, Tail...>;

    /**
     * Do the concatenation!
     */
    static constexpr result_type concat(A&& a, B&& b, Tail&&... tail) noexcept {
        // Declare the array:
        array_type arr;
        // Iterate over a then b, inserting buffers into our array
        auto out = arr.begin();
        for (auto&& a_part : a) {
            *out++ = a_part;
        }
        for (auto&& b_part : b) {
            *out++ = b_part;
        }
        // Now, do the next concatenation:
        return buffers_cat(NEO_FWD(arr), NEO_FWD(tail)...);
    }
};

/**
 * [4]: Concatenate buffers, but we encounter a buffer that is not simple. We
 *      want to continue folding simple buffers, so we'll have to "defer" the
 *      concat of the head buffer into the sequence.
 *
 * To do this, we are defined as concatenating the elements of `Tail...`, and
 * then *inserting* `Head` into the generated concatenation sequence.
 */
template <buffer_range Head, buffer_range... Tail>
requires(sizeof...(Tail) != 0) struct bufcat_impl<Head, Tail...> {
    /**
     * The tail_result_type is the result_type of the concatenation of `Tail...`.
     * We'll use that type and we'll be inserting `Head` onto the front of it.
     */
    using tail_result_type = buffers_cat_t<Tail...>;

    /**
     * We need to switch the way we "prepend" `Head` based on the actual type of
     * `tail_result_type`...
     *
     * In the base case, create a buffers_seq_concat of `Head` and `tail_result_type`.
     */
    template <typename Next>
    struct impl {
        /// Result is just a buffers_seq_concat:
        using type = buffers_seq_concat<Head, Next>;

        /// Do it!
        constexpr static type prepend(Head&& h, tail_result_type&& tail) noexcept {
            return type(NEO_FWD(h), NEO_FWD(tail));
        }
    };

    /**
     * In the specialized case where `tail_result_type` is a `buffers_seq_concat`,
     * we want to insert `Head` onto the front of `buffers_seq_concat` rather
     * than make another nested `buffers_seq_concat`.
     */
    template <typename... Ts>
    struct impl<buffers_seq_concat<Ts...>> {
        /// Put `head` on the front:
        using type = buffers_seq_concat<Head, Ts...>;

        constexpr static type prepend(Head&& h, tail_result_type&& tail) {
            /// Insert the result at the front using std::apply to explode the tail elements.
            return std::apply([&](auto&&... args) { return type(NEO_FWD(h), NEO_FWD(args)...); },
                              std::move(tail).tuple());
        }
    };

    /// Pick which of the two methods we need based on the tail_result_type:
    using my_impl = impl<tail_result_type>;

    /// The actual final concatenation:
    static constexpr auto concat(Head&& a, Tail&&... tail) noexcept {
        /// First, concatenate the tail:
        decltype(auto) tail_result = buffers_cat(NEO_FWD(tail)...);
        /// No insert `Head` onto the front of that result:
        return my_impl::prepend(NEO_FWD(a), NEO_FWD(tail_result));
    }
};

/**
 * [5]: The next two elements in the sequence are adjacent buffers_seq_concat items
 *      elements. We don't want to create nested instances thereof: We instead want
 *      to create a new buffers_seq_concat that joins the two individually.
 *
 * In this step, we create a new buffers_seq_concat from the first two items, then we
 * pass that result down to the next concatenation operation against `Tail...`.
 */
template <match_buffer_seq_concat First, match_buffer_seq_concat Second, typename... Tail>
struct bufcat_impl<First, Second, Tail...> {
    /// glue_concat gives us a new buffers_seq_concat that joins the types from `First`
    /// and `Second` into
    template <typename A, typename B>
    struct glue_concat;
    template <typename... FirstTs, typename... SecondTs>
    struct glue_concat<buffers_seq_concat<FirstTs...>, buffers_seq_concat<SecondTs...>> {
        using type = buffers_seq_concat<FirstTs..., SecondTs...>;
    };

    using joined_result =
        typename glue_concat<std::remove_cvref_t<First>, std::remove_cvref_t<Second>>::type;

    static constexpr auto concat(First&& f, Second&& s, Tail&&... tail) noexcept {
        // Create a tuple of the buffers from `First` and `Second`
        auto all_tup = std::tuple_cat(NEO_FWD(f).tuple(), NEO_FWD(s).tuple());
        /// Join those buffers into a new buffer concatenation:
        auto joined = std::apply([](auto&&... elems) { return buffers_cat(NEO_FWD(elems)...); },
                                 std::move(all_tup));
        // Now concatenate `Tail` onto that:
        return buffers_cat(std::move(joined), NEO_FWD(tail)...);
    }
};

}  // namespace detail

}  // namespace neo
