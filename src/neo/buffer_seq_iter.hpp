#pragma once

#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <type_traits>
#include <utility>
#include <iterator>

#ifndef NEO_CONCEPT
#if defined(__GNUC__) && __GNUC__ < 9
#define NEO_CONCEPT concept bool
#else
#define NEO_CONCEPT concept
#endif
#endif

namespace neo {

template <typename T>
NEO_CONCEPT const_buffer_sequence_iterator = requires(T iter) {
    typename std::iterator_traits<T>::value_type;
    std::is_convertible_v<typename std::iterator_traits<T>::value_type, const_buffer>;
};

template <typename T>
NEO_CONCEPT mutable_buffer_sequence_iterator
    = const_buffer_sequence_iterator<T>&& requires(T iter) {
    std::is_convertible_v<typename std::iterator_traits<T>::value_type, mutable_buffer>;
};

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
    decltype(auto) operator()(T&& t) const requires requires {
        _impl_buffer_sequence_begin(t);
    }
    { return _impl_buffer_sequence_begin(t); }

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
    decltype(auto) operator()(T&& t) const requires requires {
        _impl_buffer_sequence_end(t);
    }
    { return _impl_buffer_sequence_end(t); }

    template <typename T>
    decltype(auto) operator()(T&& t) const requires requires(T seq) {
        seq.end();
        bool(buffer_sequence_begin(seq) == seq.end());
    }
    { return t.end(); }
} buffer_sequence_end;

}  // namespace neo