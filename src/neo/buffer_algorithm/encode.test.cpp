#include "./encode.hpp"

#include <neo/as_buffer.hpp>
#include <neo/bit_cast.hpp>
#include <neo/buffer_algorithm/copy.hpp>
#include <neo/dynbuf_io.hpp>

#include <catch2/catch.hpp>

namespace {

struct be_int32_encoder {
    std::size_t off = 0;

    struct result {
        std::size_t bytes_written = 0;
        bool        done_         = false;
        auto        done() const noexcept { return done_; }
    };

    result operator()(neo::mutable_buffer mb, std::int32_t i) {
        std::array<std::byte, 4> bytes;
        bytes[0] = std::byte(i >> 24);
        bytes[1] = std::byte(i >> 16);
        bytes[2] = std::byte(i >> 8);
        bytes[3] = std::byte(i >> 0);

        auto cbuf      = neo::trivial_buffer(bytes) + off;
        auto n_written = neo::buffer_copy(mb, cbuf);
        off += n_written;
        off %= 4;
        return {.bytes_written = n_written, .done_ = n_written == cbuf.size()};
    }
};

}  // namespace

TEST_CASE("Encode a single integer into one buffer") {
    std::string str = "1234";

    int  value = 0x9a'7f'01'02;
    auto res   = neo::buffer_encode(be_int32_encoder(), neo::as_buffer(str), value);
    CHECK(res.done());
    CHECK(str == "\x9a\x7f\x01\x02");
}

TEST_CASE("Encode into a dynamic output") {
    std::string    str;
    neo::dynbuf_io str_io{str};
    const int      value = 0x9a'7f'01'02;

    auto res = neo::buffer_encode(be_int32_encoder(), str_io, value);
    CHECK(res.bytes_written == 4);
    CHECK(str_io.next(1024).size() == 4);
    CHECK(str.substr(0, 4) == "\x9a\x7f\x01\x02");
}

TEST_CASE("Encode a range of objects") {
    auto           arr = {1, 2, 3, 4};
    std::string    str;
    neo::dynbuf_io str_io{str};

    auto res = neo::buffer_encode(be_int32_encoder(), str_io, std::begin(arr), std::end(arr));
    CHECK(res.bytes_written == 16);
    CHECK(str_io.next(1034).size() == 16);
    CHECK(str.substr(0, 16)
          == std::string("\x00\x00\x00\x01"
                         "\x00\x00\x00\x02"
                         "\x00\x00\x00\x03"
                         "\x00\x00\x00\x04",
                         16));
}