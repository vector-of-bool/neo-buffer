#pragma once

#include "./dynbuf_io.hpp"
#include "./shifting_dynamic_buffer.hpp"

#include <string>

namespace neo {

struct string_dynbuf_io : dynbuf_io<std::string> {
    using dynbuf_io::dynbuf_io;

    decltype(auto) string() & noexcept { return storage(); }
    decltype(auto) string() const& noexcept { return storage(); }
    decltype(auto) string() && noexcept { return std::move(*this).storage(); }

    std::string_view read_area_view() const noexcept {
        return std::string_view(string()).substr(0, available());
    }
};

struct shifting_string_buffer : shifting_dynamic_buffer<std::string> {};

struct shifting_string_dynbuf_io : dynbuf_io<shifting_string_buffer> {
    using dynbuf_io::dynbuf_io;

    decltype(auto) string() & noexcept { return storage().storage(); }
    decltype(auto) string() const& noexcept { return storage().storage(); }
    decltype(auto) string() && noexcept { return std::move(*this).storage().storage(); }

    std::string_view read_area_view() const noexcept {
        return std::string_view(storage().data(0, available()));
    }
};

}  // namespace neo
