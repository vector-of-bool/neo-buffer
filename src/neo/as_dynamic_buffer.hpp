#pragma once

#include <neo/as_buffer.hpp>
#include <neo/dynamic_buffer.hpp>

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

inline namespace cpo {

inline constexpr struct as_dynamic_buffer_fn {
    template <typename T>
    auto operator()(T&& what) const requires requires {
        what.as_dynamic_buffer()->dynamic_buffer;
    }
    { return what.as_dynamic_buffer(); }

    template <typename T>
    auto operator()(T&& what) const requires requires {
        as_dynamic_buffer(what)->dynamic_buffer;
    }
    { return as_dynamic_buffer(what); }

    template <typename Char, typename Traits, typename Alloc>
    auto operator()(std::basic_string<Char, Traits, Alloc>& string) const noexcept {
        return dynamic_string_buffer(string);
    }

} as_dynamic_buffer;
}  // namespace cpo

}  // namespace neo
