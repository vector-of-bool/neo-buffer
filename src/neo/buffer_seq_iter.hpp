#pragma once

#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <neo/concepts.hpp>
#include <neo/fwd.hpp>

#include <iterator>
#include <type_traits>
#include <utility>

namespace neo {

// clang-format off
template <typename T>
concept const_buffer_sequence_iterator = requires(T iter) {
    ++iter;
    { *iter } -> convertible_to<const_buffer>;
};

template <typename T>
concept mutable_buffer_sequence_iterator =
    const_buffer_sequence_iterator<T> &&
    requires(T iter) {
        { *iter } -> convertible_to<mutable_buffer>;
    };
// clang-format on

namespace detail {

// clang-format off
template <typename T>
concept has_nonmember_bufseq_begin = requires(T t) {
    { buffer_sequence_begin(NEO_FWD(t)) } -> const_buffer_sequence_iterator;
};

template <typename T>
concept has_member_bufseq_begin = requires(T t) {
    { NEO_FWD(t).buffer_sequence_begin() } -> const_buffer_sequence_iterator;
};

template <typename T>
concept has_member_begin_bufseq = requires(T t) {
    { NEO_FWD(t).begin() } -> const_buffer_sequence_iterator;
};

template <typename T>
concept has_nonmember_begin_bufseq = requires(T t) {
    { begin(NEO_FWD(t)) } -> const_buffer_sequence_iterator;
};
// clang-format on

}  // namespace detail

inline namespace cpo {

constexpr inline struct _buffer_sequence_begin_fn {
    template <detail::has_nonmember_bufseq_begin T>
    decltype(auto) operator()(T&& t) const noexcept(noexcept(buffer_sequence_begin(NEO_FWD(t)))) {
        return buffer_sequence_begin(NEO_FWD(t));
    }

    template <detail::has_member_bufseq_begin T>
    decltype(auto) operator()(T&& t) const noexcept(noexcept(NEO_FWD(t).buffer_sequence_begin())) {
        return NEO_FWD(t).buffer_sequence_begin();
    }

    template <detail::has_member_begin_bufseq T>
        requires                                                                      //
        (detail::has_member_begin_bufseq<T>&& detail::has_nonmember_begin_bufseq<T>)  //
        ||                                                                            //
        detail::has_member_begin_bufseq<T>                                            //
        decltype(auto) operator()(T&& t) const noexcept(noexcept(NEO_FWD(t).begin())) {
        return NEO_FWD(t).begin();
    }

    // template <detail::has_nonmember_begin_bufseq T>
    // decltype(auto) operator()(T&& t) const noexcept(noexcept(begin(NEO_FWD(t)))) {
    //     return begin(NEO_FWD(t));
    // }
} buffer_sequence_begin;

}  // namespace cpo

namespace detail {

template <typename T>
concept has_nonmember_bufseq_end = requires(T t) {
    buffer_sequence_end(t);
    { neo::buffer_sequence_begin(t) != buffer_sequence_end(t) }
    ->neo::simple_boolean;
};

template <typename T>
concept has_member_bufseq_end = requires(T t) {
    t.buffer_sequence_end();
    { neo::buffer_sequence_begin(t) != t.buffer_sequence_end() }
    ->neo::simple_boolean;
};

template <typename T>
concept has_member_end_bufseq = requires(T t) {
    t.end();
    { neo::buffer_sequence_begin(t) != t.end() }
    ->neo::simple_boolean;
};

}  // namespace detail

inline namespace cpo {

constexpr inline struct _buffer_sequence_end_fn {
    template <detail::has_nonmember_bufseq_end T>
    decltype(auto) operator()(T&& t) const {
        return buffer_sequence_end(t);
    }
    template <detail::has_member_bufseq_end T>
    decltype(auto) operator()(T&& t) const {
        return t.buffer_sequence_end();
    }

    template <detail::has_member_end_bufseq T>
    decltype(auto) operator()(T&& t) const {
        return t.end();
    }
} buffer_sequence_end;

}  // namespace cpo

}  // namespace neo