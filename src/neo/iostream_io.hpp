#pragma once

#include "./dynamic_buffer.hpp"
#include "./dynbuf_io.hpp"
#include "./string_io.hpp"

#include <neo/assert.hpp>
#include <neo/concepts.hpp>
#include <neo/ref.hpp>

#include <ios>

namespace neo {

namespace detail {

template <typename S>
concept can_buf_ios_read = requires(S s, mutable_buffer mb) {
    buffer_ios_read(s, mb);
};

template <typename S>
concept can_buf_ios_write = requires(S s, const_buffer cb) {
    buffer_ios_write(s, cb);
};

}  // namespace detail

/**
 * Adapt a stdlib istream/ostream to be used as a buffer_source/buffer_sink.
 *
 * Although this class provides both sink functions AND source functions, only
 * one of the two interfaces should be used at a time on a single object. This
 * class internally only has a single buffer to be used for both reading and
 * writing.
 */
template <typename Stream, dynamic_buffer DynBuffer = shifting_string_buffer>
class iostream_io {
    wrap_refs_t<Stream>  _stream;
    dynbuf_io<DynBuffer> _buffer;

    static constexpr bool is_istream = detail::can_buf_ios_read<Stream>;
    static constexpr bool is_ostream = detail::can_buf_ios_write<Stream>;

public:
    iostream_io() = default;

    explicit iostream_io(Stream&& in) noexcept
        : _stream(NEO_FWD(in)) {}

    explicit iostream_io(Stream&& in, DynBuffer&& db) noexcept
        : _stream(NEO_FWD(in))
        , _buffer(NEO_FWD(db)) {}

    NEO_DECL_UNREF_GETTER(buffer, _buffer);
    NEO_DECL_UNREF_GETTER(stream, _stream);

    void clear_buffer() noexcept { buffer().clear(); }

    decltype(auto) prepare(std::size_t prep_size) noexcept requires is_ostream {
        return buffer().prepare(prep_size);
    }

    void commit(std::size_t n) requires is_ostream {
        auto& buf = buffer();
        buf.commit(n);
        auto dat  = buf.next(buf.available());
        auto obuf = stream().rdbuf();
        neo_assert(expects, obuf != nullptr, "No buffer for the output stream");
        std::size_t n_written = buffer_ios_write(stream(), dat);
        buf.consume(n_written);
    }

    decltype(auto) next(std::size_t want_size) requires is_istream {
        // Easy case: We already have enough bytes that we can just return that
        // to the caller:
        auto& buf = buffer();
        if (buf.available() >= want_size) {
            // We already have enough bytes that we can just return that
            return buf.next(want_size);
        } else if (buf.available()) {
            return buf.next(buf.available());
        } else {
            // Buffer is empty. Read some more.
        }

        // Get a buffer to read into
        const auto  read_buf     = buf.prepare(want_size);
        std::size_t n_total_read = buffer_ios_read(stream(), read_buf);
        // Commit the bytes we read into the read-area of the dynbuffer, and return that area:
        buf.commit(n_total_read);
        neo_assert(ensures,
                   buf.available() <= want_size,
                   "We read too many bytes from the underlying stream??",
                   buf.available(),
                   want_size,
                   n_total_read);
        return buf.next(buf.available());
    }

    void consume(std::size_t s) noexcept requires is_istream {
        neo_assert(expects,
                   s <= buffer().available(),
                   "Attempted to consume more bytes from an istream_source than have been read",
                   s,
                   buffer().available());
        buffer().consume(s);
    }
};

template <typename S, typename B>
explicit iostream_io(S&&, B &&) -> iostream_io<S, B>;

template <typename S>
explicit iostream_io(S &&) -> iostream_io<S>;

/**
 * Read data from an istream into the given buffer range. Expects that the given
 * stream is good for reading. Returns the number of bytes successfully read.
 */
template <typename Char, typename Traits, mutable_buffer_range Bufs>
std::size_t buffer_ios_read(std::basic_istream<Char, Traits>& in, Bufs&& bufs) {
    neo_assert(expects,
               in.good() || in.eof(),
               "Attempted to read bytes from a basic_istream that isn't open",
               in.good(),
               in.eof(),
               in.fail(),
               in.bad(),
               in.rdstate());
    std::size_t n_read = 0;
    for (mutable_buffer part : bufs) {
        while (part) {
            auto n_chars = part.size() / sizeof(Char);
            in.read(reinterpret_cast<Char*>(part.data()), n_chars);
            // Figure out how much we *actually* read:
            auto n_read_part = static_cast<std::size_t>(in.gcount());
            n_read += n_read_part * sizeof(Char);
            part += n_read_part * sizeof(Char);
            if (n_read_part == 0 || part.size() < sizeof(Char)) {
                // We didn't read any data (error) or the remaining buffer is
                // less than a single char.
                break;
            }
        }
        if (part) {
            // We didn't read into the entire buffer. Probably an io error.
            break;
        }
    }
    return n_read;
}

/**
 * Write data into an ostream from the given buffers. Expects that the given
 * ostream is valid and ready for writing. Returns the number of bytes written.
 */
template <typename Char, typename Traits, buffer_range Bufs>
std::size_t buffer_ios_write(std::basic_ostream<Char, Traits>& out, Bufs&& bufs) {
    neo_assert(expects,
               out.good() || out.eof(),
               "Attempted to write bytes to a basic_ostream that isn't open",
               out.good(),
               out.eof(),
               out.fail(),
               out.bad(),
               out.rdstate());
    // We need to use the rdbuf to do the written, since it is the only way (?) to know how many
    // bytes we are actually writing into the stream.
    auto sbuf = out.rdbuf();
    neo_assert(expects,
               sbuf != nullptr,
               "Cannot write to a basic_ostream with no associated streambuf");
    std::size_t n_written = 0;
    for (const_buffer part : bufs) {
        while (part) {
            auto n_chars        = part.size() / sizeof(Char);
            auto n_written_part = sbuf->sputn(reinterpret_cast<const Char*>(part.data()), n_chars);
            n_written += n_written_part * sizeof(Char);
            part += n_written_part * sizeof(Char);
            if (n_written_part == 0 || part.size() < sizeof(Char)) {
                // We didn't write any data (error) or the remaining buffer is
                // less than a single char.
                break;
            }
        }
        if (part) {
            // We didn't write the whole buffer, which only happens on error.
            break;
        }
    }
    return n_written;
}

}  // namespace neo
