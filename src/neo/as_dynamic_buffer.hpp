#pragma once

#include <neo/as_buffer.hpp>
#include <neo/buffer_algorithm/copy.hpp>
#include <neo/dynamic_buffer.hpp>

#include <neo/fwd.hpp>

#include <limits>

namespace neo {

namespace detail {

// clang-format off

template <typename T>
concept has_member_as_dynbuf = requires(T t) {
    { NEO_FWD(t).as_dynamic_buffer() } -> dynamic_buffer;
};

template <typename T>
concept has_adl_as_dynbuf = requires(T t) {
    { as_dynamic_buffer(NEO_FWD(t)) } -> dynamic_buffer;
};

template <typename T>
concept simple_resizable_byte_container =
    as_buffer_convertible<T> &&
    (sizeof(data_type_t<T>) == 1) &&
    requires(const std::remove_cvref_t<T> c_cont, std::remove_cvref_t<T> cont, std::size_t size) {
        { c_cont.size() } noexcept -> same_as<std::size_t>;
        { cont.resize(size) };
    };

template <typename T>
concept as_dynamic_buffer_convertible_check =
       has_member_as_dynbuf<T>
    || has_adl_as_dynbuf<T>
    || simple_resizable_byte_container<T>
    || dynamic_buffer<std::remove_cvref_t<T>>;

template <typename C>
concept container_has_capacity = requires(const C c) { c.capacity(); };
template <typename C>
concept container_has_max_size = requires(const C c) { c.max_size(); };

// clang-format on

}  // namespace detail

template <detail::simple_resizable_byte_container Container>
class dynamic_buffer_byte_container_adaptor {
public:
    using container_type = std::remove_cvref_t<Container>;

private:
    wrap_refs_t<Container> _container;

public:
    constexpr dynamic_buffer_byte_container_adaptor() = default;
    constexpr explicit dynamic_buffer_byte_container_adaptor(Container&& c)
        : _container(NEO_FWD(c)) {}

    NEO_DECL_UNREF_GETTER(container, _container);

    constexpr std::size_t size() const noexcept { return as_buffer(container()).size(); }
    constexpr std::size_t max_size() const noexcept {
        if constexpr (detail::container_has_max_size<Container>) {
            return container().max_size();
        } else {
            return std::numeric_limits<std::size_t>::max();
        }
    }
    constexpr std::size_t capacity() const noexcept {
        if constexpr (detail::container_has_capacity<Container>) {
            return container().capacity();
        } else {
            return size();
        }
    }

    constexpr auto data(std::size_t position, std::size_t size) noexcept {
        return (as_buffer(container()) + position).first(size);
    }

    constexpr auto data(std::size_t position, std::size_t size) const noexcept {
        return (as_buffer(container()) + position).first(size);
    }

    constexpr mutable_buffer grow(std::size_t n) noexcept(noexcept(container().resize(n))) {
        const auto init_size           = size();
        const auto remaining_grow_size = max_size() - init_size;
        neo_assert(expects,
                   n <= remaining_grow_size,
                   "grow() would put dynamic_buffer beyond its maximum size",
                   n,
                   this->max_size(),
                   this->size());
        container().resize(init_size + n);
        return data(init_size, n);
    }

    constexpr void shrink(std::size_t n) noexcept {
        neo_assert(expects,
                   n <= size(),
                   "Cannot shrink() a dynamic buffer more than its size",
                   n,
                   size());
        container().resize(size() - n);
    }

    constexpr void consume(std::size_t n_bytes) noexcept {
        neo_assert(expects,
                   n_bytes <= size(),
                   "Should never remove more bytes than are available in a dynamic buffer.",
                   n_bytes,
                   size());

        const auto dest     = data(0, size());
        const auto src      = dest + n_bytes;
        const auto n_copied = buffer_copy(dest, src, ll_buffer_copy_forward);
        neo_assert(invariant,
                   n_copied == src.size(),
                   "Didn't copy as expected from byte container",
                   n_copied,
                   src.size(),
                   dest.size(),
                   n_bytes);

        const auto new_size = size() - n_bytes;
        container().resize(new_size);
    }
};

template <typename T>
explicit dynamic_buffer_byte_container_adaptor(T &&) -> dynamic_buffer_byte_container_adaptor<T>;

namespace cpo {
inline constexpr struct as_dynamic_buffer_fn {
    template <detail::as_dynamic_buffer_convertible_check T>
    constexpr decltype(auto) operator()(T&& what) const noexcept {
        if constexpr (detail::has_member_as_dynbuf<T>) {
            return NEO_FWD(what).as_dynamic_buffer();
        } else if constexpr (detail::has_adl_as_dynbuf<T>) {
            return as_dynamic_buffer(NEO_FWD(what));
        } else if constexpr (detail::simple_resizable_byte_container<T>) {
            return dynamic_buffer_byte_container_adaptor(NEO_FWD(what));
        } else {
            static_assert(dynamic_buffer<std::remove_cvref_t<T>>);
            return NEO_FWD(what);
        }
    }
} as_dynamic_buffer;
}  // namespace cpo

using namespace cpo;

/**
 * Requires that the given type support being passed to `as_dynamic_buffer`
 */
template <typename T>
concept as_dynamic_buffer_convertible = requires(T t) {
    as_dynamic_buffer(NEO_FWD(t));
};

/**
 * Obtain the type of the dynamic buffer returned by as_dynamic_buffer if given
 * an object of type T.
 */
template <as_dynamic_buffer_convertible T>
using as_dynamic_buffer_t = decltype(as_dynamic_buffer(std::declval<T>()));

}  // namespace neo
