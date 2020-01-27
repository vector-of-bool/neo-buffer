#pragma once

#include <neo/as_buffer.hpp>
#include <neo/dynamic_buffer.hpp>

#include <neo/fwd.hpp>

#include <string>
#include <vector>

namespace neo {

template <typename String>
class dynamic_string_buffer {
public:
    using string_type = String;

    using const_buffers_type   = const_buffer;
    using mutable_buffers_type = mutable_buffer;

private:
    std::reference_wrapper<string_type> _string;

public:
    explicit dynamic_string_buffer(string_type& str)
        : _string(str) {}

    string_type&       string() noexcept { return _string; }
    const string_type& string() const noexcept { return _string; }

    auto size() const noexcept { return string().size(); }
    auto max_size() const noexcept { return string().max_size(); }
    auto capacity() const noexcept { return string().capacity(); }

    mutable_buffers_type data(std::size_t position, std::size_t size) noexcept {
        return as_buffer(string(), size) + position;
    }

    const_buffers_type data(std::size_t position, std::size_t size) const noexcept {
        return as_buffer(string(), size) + position;
    }

    mutable_buffers_type grow(std::size_t n) {
        auto init_size = size();
        string().resize(init_size + n);
        return data(init_size, n);
    }

    void shrink(std::size_t n) noexcept { string().resize(size() - n); }
    void consume(std::size_t n) noexcept { string().erase(0, n); }
};

template <typename T>
dynamic_string_buffer(T) -> dynamic_string_buffer<T>;

namespace detail {

// clang-format off
template <typename T>
concept has_member_as_dynbuf = requires(T t) {
    { NEO_FWD(t).as_dynamic_buffer() } -> dynamic_buffer;
};

template <typename T>
concept has_nonmember_as_dynbuf = requires(T t) {
    { as_dynamic_buffer(NEO_FWD(t)) } -> dynamic_buffer;
};

template <typename T>
concept has_both_as_dynbuf =
    has_member_as_dynbuf<T> &&
    has_nonmember_as_dynbuf<T>;
// clang-format on

}  // namespace detail

inline namespace cpo {

inline constexpr struct as_dynamic_buffer_fn {
    template <detail::has_member_as_dynbuf T>
    decltype(auto) operator()(T&& what) const
        noexcept(noexcept(NEO_FWD(what).as_dynamic_buffer())) {
        return NEO_FWD(what).as_dynamic_buffer();
    }

    template <detail::has_nonmember_as_dynbuf T>
    decltype(auto) operator()(T&& what) const noexcept(noexcept(as_dynamic_buffer(NEO_FWD(what)))) {
        return as_dynamic_buffer(NEO_FWD(what));
    }

    template <detail::has_both_as_dynbuf T>
    decltype(auto) operator()(T&& what) const
        noexcept(noexcept(NEO_FWD(what).as_dynamic_buffer())) {
        return NEO_FWD(what).as_dynamic_buffer();
    }

    template <typename Char, typename Traits, typename Alloc>
    decltype(auto) operator()(std::basic_string<Char, Traits, Alloc>& string) const noexcept {
        return dynamic_string_buffer(string);
    }

} as_dynamic_buffer;
}  // namespace cpo

}  // namespace neo
