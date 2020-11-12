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
    [[no_unique_address]] wrap_refs_t<Sink>      _sink;
    [[no_unique_address]] wrap_refs_t<Transform> _transformer;
    [[no_unique_address]] wrap_refs_t<DynBuffer> _buffer;

public:
    constexpr buffer_transform_sink() = default;
    constexpr explicit buffer_transform_sink(Sink&& s) noexcept
        : _sink(NEO_FWD(s)) {}

    constexpr explicit buffer_transform_sink(Sink&& s, Transform&& tr) noexcept
        : _sink(NEO_FWD(s))
        , _transformer(NEO_FWD(tr)) {}

    constexpr explicit buffer_transform_sink(Sink&& s, Transform&& tr, DynBuffer&& db) noexcept
        : _sink(NEO_FWD(s))
        , _transformer(NEO_FWD(tr))
        , _buffer(NEO_FWD(db)) {}

    NEO_DECL_UNREF_GETTER(sink, _sink);
    NEO_DECL_UNREF_GETTER(transformer, _transformer);
    NEO_DECL_UNREF_GETTER(buffer, _buffer);

    auto prepare(std::size_t prep_size) noexcept(noexcept(buffer().grow(prep_size))) {
        auto& buf   = buffer();
        auto  avail = buf.size();
        if (avail >= prep_size) {
            return buf.data(0, prep_size);
        }
        auto more_size = prep_size - avail;
        dynbuf_safe_grow(buf, more_size);
        return buf.data(0, buf.size());
    }

    void commit(std::size_t n) noexcept(
        noexcept(buffer_transform(transformer(), sink(), buffer().data(1, 1))))  //
    {
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

template <buffer_source      Source,
          buffer_transformer Transform,
          dynamic_buffer     DynBuffer = shifting_string_buffer>
class buffer_transform_source {
    [[no_unique_address]] wrap_refs_t<Source>    _source;
    [[no_unique_address]] wrap_refs_t<Transform> _transformer;
    [[no_unique_address]] wrap_refs_t<DynBuffer> _buffer;

    std::size_t _avail = unref(_buffer).size();

public:
    constexpr buffer_transform_source() = default;
    constexpr explicit buffer_transform_source(Source&& s) noexcept
        : _source(NEO_FWD(s)) {}

    constexpr explicit buffer_transform_source(Source&& s, Transform&& tr) noexcept
        : _source(NEO_FWD(s))
        , _transformer(NEO_FWD(tr)) {}

    constexpr explicit buffer_transform_source(Source&& s, Transform&& tr, DynBuffer&& db) noexcept
        : _source(NEO_FWD(s))
        , _transformer(NEO_FWD(tr))
        , _buffer(NEO_FWD(db)) {}

    NEO_DECL_UNREF_GETTER(source, _source);
    NEO_DECL_UNREF_GETTER(transformer, _transformer);
    NEO_DECL_UNREF_GETTER(buffer, _buffer);

    auto next(std::size_t want_size)                                                        //
        noexcept(noexcept(buffer().grow(want_size)) &&                                      //
                 noexcept(buffer_transform(transformer(), buffer().data(1, 1), source())))  //
    {
        auto& buf = buffer();
        if (_avail >= want_size) {
            return buf.data(0, want_size);
        }

        if (buf.size() < want_size) {
            auto grow_size = want_size - buf.size();
            auto cap_size  = (std::min)(grow_size, std::size_t(1024 * 1024 * 10));
            dynbuf_safe_grow(buf, cap_size);
        }

        auto tail_size      = buf.size() - _avail;
        auto want_read_size = want_size - _avail;
        auto read_size      = (std::min)(want_read_size, tail_size);
        auto read_buf       = buf.data(_avail, read_size);

        auto res = buffer_transform(transformer(), read_buf, source());
        _avail += res.bytes_written;
        const auto give = (std::min)(_avail, want_size);
        return buf.data(0, give);
    }

    void consume(std::size_t n) noexcept {
        neo_assert(
            expects,
            n <= _avail,
            "Attempted to consume more bytes from a buffer_transform_source than are available.",
            _avail,
            n);
        buffer().consume(n);
        _avail -= n;
    }
};

template <typename S, typename Tr>
explicit buffer_transform_source(S&&, Tr &&) -> buffer_transform_source<S, Tr>;

template <typename S, typename Tr, typename B>
explicit buffer_transform_source(S&&, Tr&&, B &&) -> buffer_transform_source<S, Tr, B>;

}  // namespace neo
