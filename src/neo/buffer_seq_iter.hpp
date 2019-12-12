#pragma once

#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <neo/concepts.hpp>

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

inline namespace cpo {

template <typename T>
concept has_nonmember_bufseq_begin = requires(T t) {
    { buffer_sequence_begin(t) } -> const_buffer_sequence_iterator;
};

template <typename T>
concept has_member_bufseq_begin = requires(T t) {
    { t.buffer_sequence_begin() } -> const_buffer_sequence_iterator;
};

template <typename T>
concept has_member_begin_bufseq = requires(T t) {
    { t.begin() } -> const_buffer_sequence_iterator;
};

constexpr inline struct _buffer_sequence_begin_fn {
    template <has_nonmember_bufseq_begin T> decltype(auto) operator()(T&& t) const { return buffer_sequence_begin(t); }
    template <has_member_bufseq_begin T> decltype(auto) operator()(T&& t) const { return t.buffer_sequence_begin(); }

    template <has_member_begin_bufseq T> decltype(auto) operator()(T&& t) const { return t.begin(); }
} buffer_sequence_begin;

template <typename T>
concept has_nonmember_bufseq_end = requires(T t) {
    buffer_sequence_end(t);
    { buffer_sequence_begin(t) != buffer_sequence_end(t) } -> neo::simple_boolean;
};

template <typename T>
concept has_member_bufseq_end = requires(T t) {
    t.buffer_sequence_end();
    { buffer_sequence_begin(t) != t.buffer_sequence_end() } -> neo::simple_boolean;
};

template <typename T>
concept has_member_end_bufseq = requires(T t) {
    t.end();
    { buffer_sequence_begin(t) != t.end() } -> neo::simple_boolean;
};

constexpr inline struct _buffer_sequence_end_fn {
    template <has_nonmember_bufseq_end T> decltype(auto) operator()(T&& t) const { return buffer_sequence_end(t); }
    template <has_member_bufseq_end T> decltype(auto) operator()(T&& t) const { return t.buffer_sequence_end(); }

    template <has_member_end_bufseq T> decltype(auto) operator()(T&& t) const { return t.end(); }
} buffer_sequence_end;

}  // namespace cpo

}  // namespace neo