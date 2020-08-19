#pragma once

#include "./as_dynamic_buffer.hpp"
#include "./buffer_algorithm/transform.hpp"
#include "./buffer_sink.hpp"
#include "./string_io.hpp"

#include <neo/fwd.hpp>
#include <neo/ref.hpp>

#include <string>

namespace neo {

template <buffer_sink        Sink,
          buffer_transformer Transform,
          dynamic_buffer     DynBuffer = shifting_string_buffer>
class buffer_transform_sink {
    [[no_unique_address]] wrap_if_reference_t<Sink>      _sink;
    [[no_unique_address]] wrap_if_reference_t<Transform> _transformer;
    [[no_unique_address]] wrap_if_reference_t<DynBuffer> _buffer;

public:
    constexpr buffer_transform_sink() = default;
    constexpr explicit buffer_transform_sink(Sink&& s)
        : _sink(NEO_FWD(s)) {}

    constexpr explicit buffer_transform_sink(Sink&& s, Transform&& tr)
        : _sink(NEO_FWD(s))
        , _transformer(NEO_FWD(tr)) {}

    constexpr explicit buffer_transform_sink(Sink&& s, Transform&& tr, DynBuffer&& db)
        : _sink(NEO_FWD(s))
        , _transformer(NEO_FWD(tr))
        , _buffer(NEO_FWD(db)) {}

    constexpr decltype(auto) sink() & noexcept { return unref(_sink); }
    constexpr decltype(auto) sink() const& noexcept { return unref(_sink); }
    constexpr decltype(auto) sink() && noexcept { return unref(std::move(_sink)); }

    constexpr decltype(auto) transformer() & noexcept { return unref(_transformer); }
    constexpr decltype(auto) transformer() const& noexcept { return unref(_transformer); }
    constexpr decltype(auto) transformer() && noexcept { return unref(std::move(_transformer)); }

    constexpr decltype(auto) buffer() & noexcept { return unref(_buffer); }
    constexpr decltype(auto) buffer() const& noexcept { return unref(_buffer); }
    constexpr decltype(auto) buffer() && noexcept { return unref(std::move(_buffer)); }

    auto prepare(std::size_t prep_size) noexcept {
        auto& buf   = buffer();
        auto  avail = buf.size();
        if (avail >= prep_size) {
            return buf.data(0, prep_size);
        }
        auto more_size = prep_size - avail;
        dynbuf_safe_grow(buf, more_size);
        return buf.data(0, buf.size());
    }

    void commit(std::size_t n) noexcept {
        auto&  buf     = buffer();
        auto&& databuf = buf.data(0, n);
        buffer_transform(transformer(), sink(), databuf);
        buf.consume(n);
    }
};

template <typename S, typename Tr>
explicit buffer_transform_sink(S&&, Tr &&) -> buffer_transform_sink<S, Tr>;

template <typename S, typename Tr, typename B>
explicit buffer_transform_sink(S&&, Tr&&, B &&) -> buffer_transform_sink<S, Tr, B>;

}  // namespace neo
