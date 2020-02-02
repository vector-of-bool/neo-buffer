#include <neo/bytes.hpp>

#include <neo/as_dynamic_buffer.hpp>

#include <catch2/catch.hpp>

#include <string_view>

template <neo::dynamic_buffer T>
void take_dynamic_buffer(T) {}

template <neo::const_buffer_sequence T>
void take_cb(T) {}

template <neo::mutable_buffer_sequence T>
void take_mb(T) {}

TEST_CASE("Create bytes") {
    neo::bytes bs;
    bs.resize(12, std::byte(23));

    take_dynamic_buffer(neo::as_dynamic_buffer(bs));

    take_cb(neo::as_buffer(bs));
    take_mb(neo::as_buffer(bs));

    auto b2 = neo::bytes::copy(neo::as_buffer(bs));
    CHECK(bs == b2);
}

TEST_CASE("Copy a string literal") {
    using namespace std::literals;
    auto b1 = neo::bytes::copy(neo::as_buffer("I am a string"sv));
    CHECK(b1.size() == 13);
    auto b2 = b1;
    CHECK(b1 == b2);
    b2.resize(4);
    CHECK(b1 != b2);
    b1.resize(4);
    CHECK(b1 == b2);
}