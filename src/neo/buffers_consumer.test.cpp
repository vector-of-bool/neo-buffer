#include <neo/buffers_consumer.hpp>

#include <neo/buffer_algorithm/copy.hpp>

#include <catch2/catch.hpp>

using namespace std::literals;

TEST_CASE("Consume some buffers") {
    auto bufs = {
        neo::const_buffer("meow"),
        neo::const_buffer("bark"),
        neo::const_buffer("sing"),
    };
    neo::buffers_consumer cbs{bufs};

    std::string str;
    str.resize(6);
    auto n_copied = buffer_copy(neo::mutable_buffer(str), cbs.next(8));
    CHECK(n_copied == 6);
    CHECK(str == "meowba");
    cbs.consume(n_copied);
    n_copied = buffer_copy(neo::mutable_buffer(str), cbs.next(200));
    CHECK(n_copied == 6);
    CHECK(str == "rksing");

    // Consume on buffer boundaries
    cbs = neo::buffers_consumer{bufs};
    cbs.consume(4);
    CHECK(cbs.next_contiguous().equals_string("bark"sv));

    // Clamp the buffers
    neo::buffers_consumer partial{bufs, 6};
    str.resize(10, 'Z');
    n_copied = buffer_copy(neo::mutable_buffer(str), partial);
    CHECK(n_copied == 6);
    CHECK(str == "meowbaZZZZ");

    cbs      = neo::buffers_consumer{bufs};
    str      = "1234567890";
    n_copied = buffer_copy(neo::as_buffer(str), cbs.next(6));
    CHECK(n_copied == 6);
    CHECK(str == "meowba7890");
}

TEST_CASE("Buffer consumer for singular buffers") {
    neo::buffers_consumer bufs{neo::const_buffer("Just one buffer")};

    std::string str;
    str.resize(9);
    auto n_copied = buffer_copy(neo::mutable_buffer(str), bufs.next(16));
    CHECK(n_copied == 9);
    CHECK(str == "Just one ");
    // Copying again will just copy the same bytes
    auto n_copied_2 = buffer_copy(neo::mutable_buffer(str), bufs.next(6));
    CHECK(n_copied_2 == 6);
    CHECK(str == "Just one ");
    // Copy the rest
    bufs.consume(n_copied);
    n_copied = buffer_copy(neo::mutable_buffer(str), bufs.next(str.size()));
    CHECK(n_copied == 6);
    CHECK(str == "bufferne ");  // Copied over the top of the prior string
}

TEST_CASE("Consume mutable buffers too") {
    std::string a, b;
    a.resize(10);
    b.resize(3);
    auto bufs_il = {
        neo::mutable_buffer(a),
        neo::mutable_buffer(b),
    };

    neo::buffers_consumer bufs{bufs_il};

    buffer_copy(bufs.prepare(15), neo::const_buffer("I am a string"));
    CHECK(a == "I am a str");
    CHECK(b == "ing");

    neo::buffers_consumer c{neo::mutable_buffer(a)};
    buffer_copy(c.prepare(200), neo::const_buffer("short string"));
    CHECK(a == "short stri");
}
