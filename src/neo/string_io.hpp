#pragma once

#include "./io_buffer.hpp"
#include "./shifting_dynamic_buffer.hpp"

#include <string>

namespace neo {

struct string_io_buffer : dynamic_io_buffer<std::string> {
    decltype(auto) string() & noexcept { return storage(); }
    decltype(auto) string() const& noexcept { return storage(); }
    decltype(auto) string() && noexcept { return std::move(*this).storage(); }
};

struct shifting_string_buffer : shifting_dynamic_buffer<as_dynamic_buffer_t<std::string>> {};

struct shifting_string_io_buffer : dynamic_io_buffer<shifting_string_buffer> {
    decltype(auto) string() & noexcept { return storage().storage().container(); }
    decltype(auto) string() const& noexcept { return storage().storage().container(); }
    decltype(auto) string() && noexcept { return std::move(*this).storage().storage().container(); }
};

}  // namespace neo
