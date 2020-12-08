#pragma once

#include <neo/buffer_sink.hpp>
#include <neo/buffer_source.hpp>

#include <neo/fwd.hpp>
#include <neo/ref.hpp>

namespace neo {

struct buffers_count {
    std::size_t bytes_committed = 0;
    std::size_t bytes_consumed  = 0;
};

template <typename Bufs, typename Handler>
requires(buffer_sink<Bufs> || buffer_source<Bufs>)  //
    class counting_buffers {
public:
    using stream_type  = std::remove_cvref_t<Bufs>;
    using handler_type = std::remove_cvref_t<Handler>;

private:
    [[no_unique_address]] wrap_refs_t<Bufs>    _bufs;
    [[no_unique_address]] wrap_refs_t<Handler> _handler;

public:
    constexpr counting_buffers() = default;

    constexpr explicit counting_buffers(Bufs&& s, Handler&& h) noexcept(
        std::is_nothrow_constructible_v<wrap_refs_t<Bufs>, Bufs>&&
            std::is_nothrow_constructible_v<wrap_refs_t<Handler>, Handler>)
        : _bufs(NEO_FWD(s))
        , _handler(NEO_FWD(h)) {}

    NEO_DECL_UNREF_GETTER(buffers, _bufs);
    NEO_DECL_REF_REBINDER(rebind_buffers, Bufs, _bufs);

    NEO_DECL_UNREF_GETTER(handler, _handler);
    NEO_DECL_REF_REBINDER(rebind_handler, Handler, _handler);

    constexpr decltype(auto)
    prepare(std::size_t s) noexcept(noexcept_buffer_output_v<Bufs>) requires buffer_sink<Bufs> {
        return buffers().prepare(s);
    }

    constexpr decltype(auto)
    next(std::size_t s) noexcept(noexcept_buffer_input_v<Bufs>) requires buffer_source<Bufs> {
        return buffers().next(s);
    }

    void commit(std::size_t s) noexcept(noexcept_buffer_output_v<Bufs>) requires buffer_sink<Bufs> {
        buffers().commit(s);
        std::invoke(handler(), buffers_count{.bytes_committed = s});
    }

    void
    consume(std::size_t s) noexcept(noexcept_buffer_input_v<Bufs>) requires buffer_source<Bufs> {
        buffers().consume(s);
        std::invoke(handler(), buffers_count{.bytes_consumed = s});
    }
};

template <typename S, typename H>
explicit counting_buffers(S&&, H &&) -> counting_buffers<S, H>;

}  // namespace neo
