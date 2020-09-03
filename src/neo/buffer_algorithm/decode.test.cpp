#include "./decode.hpp"

#include <neo/bit_cast.hpp>
#include <neo/buffer_algorithm/copy.hpp>

#include <catch2/catch.hpp>

#include <array>
#include <vector>

using namespace neo::literals;

namespace {

struct unbounded_sentinel_t {
    template <neo::input_or_output_iterator O>
    constexpr friend bool operator==(O, unbounded_sentinel_t) noexcept {
        return false;
    }

    template <neo::input_or_output_iterator O>
    constexpr friend bool operator!=(O, unbounded_sentinel_t) noexcept {
        return true;
    }

    template <neo::input_or_output_iterator O>
    constexpr bool operator==(O) const noexcept {
        return false;
    }

    template <neo::input_or_output_iterator O>
    constexpr bool operator!=(O) const noexcept {
        return true;
    }
} unbounded_sentinel;

template <typename C>
struct my_back_inserter {
    C* container = nullptr;

    using value_type      = void;
    using difference_type = std::ptrdiff_t;

    my_back_inserter() = default;
    explicit my_back_inserter(C& c)
        : container(&c) {}

    auto& operator++() { return *this; }
    auto  operator++(int) { return *this; }

    auto& operator*() const { return *this; }

    template <typename T>
    void operator=(T&& val) const requires requires {
        container->push_back(NEO_FWD(val));
    }
    { container->push_back(NEO_FWD(val)); }
};

template <typename T>
my_back_inserter(T &&) -> my_back_inserter<T>;

struct be_int32_decoder {
    std::array<std::byte, 4> buf;
    std::size_t              mb_off = 0;

    struct result {
        std::size_t  bytes_read = 0;
        bool         _has_val   = false;
        std::int32_t val        = -42;

        bool has_value() { return _has_val; }
        bool has_error() { return false; }
        auto value() const noexcept { return val; }
    };

    result operator()(neo::const_buffer cb) {
        auto bytes_read = neo::buffer_copy(neo::mutable_buffer(buf) + mb_off, cb);
        mb_off += bytes_read;

        if (mb_off == 4) {
            std::int32_t val = 0;
            val |= std::int32_t(buf[0]) << 24;
            val |= std::int32_t(buf[1]) << 16;
            val |= std::int32_t(buf[2]) << 8;
            val |= std::int32_t(buf[3]) << 0;
            *this = {};
            return {.bytes_read = bytes_read, ._has_val = true, .val = val};
        } else {
            return {.bytes_read = bytes_read};
        }
    }
};

}  // namespace

TEST_CASE("Decode an integer") {
    auto bytes = neo::const_buffer("\x00\x00\x00\x05");

    be_int32_decoder dec;
    auto             res = neo::buffer_decode(dec, bytes);
    CHECK(res.has_value());
    CHECK(res.value() == 5);

    res = neo::buffer_decode(dec, bytes + 2);
    CHECK_FALSE(res.has_value());
    CHECK(res.bytes_read == 2);
    res = neo::buffer_decode(dec, bytes);
    CHECK(res.bytes_read == 2);
    CHECK(res.has_value());
    CHECK(res.value() == 0x00'05'00'00);
}

TEST_CASE("Decode into an output param") {
    std::int32_t ival  = 0;
    auto         bytes = neo::const_buffer("\x00\x00\x00\x05");

    auto result = neo::buffer_decode(be_int32_decoder(), bytes, neo::into(ival));
    CHECK(result.has_value());
    CHECK(ival == 0x05);
}

TEST_CASE("Partial decode") {
    be_int32_decoder decode;
    const auto       bytes1 = "\x00\x00"_buf;
    auto             result = neo::buffer_decode(decode, bytes1);
    CHECK_FALSE(result.has_value());
    result = neo::buffer_decode(decode, "\x00"_buf);
    CHECK_FALSE(result.has_value());
    result = neo::buffer_decode(decode, "\x07"_buf);
    CHECK(result.has_value());
    CHECK(result.value() == 0x07);

    result = neo::buffer_decode(decode, "\x05\x12\x05\x00"_buf);
    CHECK(result.has_value());
    CHECK(result.value() == 0x05'12'05'00);
}

TEST_CASE("Decode into a vector") {
    std::vector<std::int32_t> values;

    auto bufs = {
        "\x00\x00\x00\x05"_buf,
        "\x01\x00\x00\x99"_buf,
        "\x00\x11"_buf,
        "\x01"_buf,
        "\x03"_buf,
        "\x04\x15"_buf,
    };
    CHECK(buffer_size(bufs) == 14);

    auto result = neo::buffer_decode(be_int32_decoder(),
                                     bufs,
                                     my_back_inserter(values),
                                     unbounded_sentinel);
    CHECK(values.size() == 3);
    CHECK(values[0] == 5);
    CHECK(values[1] == 0x01'00'00'99);
    CHECK(values[2] == 0x00'11'01'03);
    CHECK(result.bytes_read == 14);
}