#pragma once

#include <neo/as_buffer.hpp>

#include <neo/assert.hpp>
#include <neo/ref.hpp>

namespace neo {

template <as_buffer_convertible Storage>
class fixed_dynamic_buffer {
public:
    using storage_type = std::remove_reference_t<Storage>;

private:
    wrap_refs_t<Storage> _storage;

public:
    constexpr fixed_dynamic_buffer() = default;
    constexpr explicit fixed_dynamic_buffer(Storage&& c) noexcept
        : _storage(NEO_FWD(c)) {}

    NEO_DECL_UNREF_GETTER(storage, _storage);

    constexpr std::size_t size() const noexcept { return as_buffer(storage()).size(); }
    constexpr std::size_t max_size() const noexcept { return size(); }
    constexpr std::size_t capacity() const noexcept { return size(); }

    constexpr auto data(std::size_t pos, std::size_t n) noexcept {
        return (as_buffer(storage()) + pos).first(n);
    }
    constexpr auto data(std::size_t pos, std::size_t n) const noexcept {
        return (as_buffer(storage()) + pos).first(n);
    }

    constexpr mutable_buffer grow(std::size_t grow_n) noexcept {
        neo_assert_always(expects,
                          grow_n == 0,
                          "fixed_dynamic_buffer cannot be grown",
                          size(),
                          grow_n);
        return data(size(), 0);
    }

    constexpr void shrink(std::size_t shrink_n) noexcept {
        neo_assert_always(expects,
                          shrink_n == 0,
                          "fixed_dynamic_buffer cannot be shrunk",
                          size(),
                          shrink_n);
    }

    constexpr void consume(std::size_t consume_n) noexcept {
        neo_assert_always(expects,
                          consume_n == 0,
                          "fixed_dynamic_buffer cannot be shrunk",
                          size(),
                          consume_n);
    }
};

template <typename T>
fixed_dynamic_buffer(T &&) -> fixed_dynamic_buffer<T>;

}  // namespace neo