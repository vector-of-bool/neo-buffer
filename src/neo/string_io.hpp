#pragma once

#include "./io_buffer.hpp"
#include "./shifting_dynamic_buffer.hpp"

#include <string>

namespace neo {

struct string_io_buffer : dynamic_io_buffer<std::string> {
    decltype(auto) string() & noexcept { return storage(); }
    decltype(auto) string() const& noexcept { return storage(); }
    decltype(auto) string() && noexcept { return std::move(*this).storage(); }

    std::string_view read_area_view() const noexcept {
        return std::string_view(string()).substr(0, available());
    }
};

struct shifting_string_buffer : shifting_dynamic_buffer<as_dynamic_buffer_t<std::string>> {};

struct shifting_string_io_buffer : dynamic_io_buffer<shifting_string_buffer> {
    decltype(auto) string() & noexcept { return storage().storage().container(); }
    decltype(auto) string() const& noexcept { return storage().storage().container(); }
    decltype(auto) string() && noexcept { return std::move(*this).storage().storage().container(); }

    std::string_view read_area_view() const noexcept {
        return std::string_view(storage().data(0, available()));
    }
};

}  // namespace neo
