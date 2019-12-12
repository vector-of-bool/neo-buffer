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

namespace cpo {

constexpr inline struct _buffer_sequence_begin_fn {
    template <typename T>
    decltype(auto) operator()(T&& t) const requires requires {
        buffer_sequence_begin(t);
    }
    { return buffer_sequence_begin(t); }

    template <typename T>
    decltype(auto) operator()(T&& t) const requires requires {
        t.buffer_sequence_begin();
    }
    { return t.buffer_sequence_begin(); }

    template <typename T>
    decltype(auto) operator()(T&& t) const requires requires(T seq) {
        seq.begin();
        const_buffer_sequence_iterator<decltype(seq.begin())>;
    }
    { return t.begin(); }
} buffer_sequence_begin;

constexpr inline struct _buffer_sequence_end_fn {
    template <typename T>
    decltype(auto) operator()(T&& t) const requires requires {
        buffer_sequence_end(t);
    }
    { return buffer_sequence_end(t); }

    template <typename T>
    decltype(auto) operator()(T&& t) const requires requires {
        t.buffer_sequence_end();
    }
    { return t.buffer_sequence_end(); }

    template <typename T>
    decltype(auto) operator()(T&& t) const requires requires(T seq) {
        seq.end();
        bool(buffer_sequence_begin(seq) == seq.end());
    }
    { return t.end(); }
} buffer_sequence_end;

}  // namespace cpo

using namespace cpo;

}  // namespace neo