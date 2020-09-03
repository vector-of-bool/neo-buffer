#include "./shifting_dynamic_buffer.hpp"

#include <neo/as_dynamic_buffer.hpp>
#include <neo/bytes.hpp>
#include <neo/dynamic_buffer.hpp>

#include <neo/test_concept.hpp>

#include <catch2/catch.hpp>

NEO_TEST_CONCEPT(
    neo::dynamic_buffer<neo::shifting_dynamic_buffer<neo::as_dynamic_buffer_t<neo::bytes>>>);

TEST_CASE("Create a shifting dynbuf") {
    std::string                  str;
    neo::shifting_dynamic_buffer shifty{neo::as_dynamic_buffer(str)};

    auto bufs = shifty.grow(12);
    CHECK(neo::buffer_size(bufs) == 12);
    CHECK(shifty.size() == 12);

    // Shifting buffer will round-up to avoid doing many tiny allocations:
    CHECK(str.size() == 1024);

    // We can grow some more, which won't change the storage.
    bufs = shifty.grow(33);
    CHECK(neo::buffer_size(bufs) == 33);
    CHECK(str.size() == 1024);

    CHECK(shifty.size() == (33 + 12));
    CHECK(shifty.capacity() == 1024);

    shifty.shrink(5);
    CHECK(shifty.size() == 40);
    CHECK(shifty.capacity() == 1024);

    // Copy a string through the buffer
    neo::buffer_copy(shifty.data(0, 6), neo::const_buffer("Hello!"));
    CHECK(str.substr(0, 6) == "Hello!");

    // "Consume" the first two bytes.
    shifty.consume(3);
    CHECK(shifty.size() == 37);
    CHECK(shifty.capacity() == 1021);  // Took two bytes off the front

    CHECK(std::string_view(shifty.data(0, 3)) == "lo!");

    neo::buffer_copy(shifty.data(0, 5), neo::const_buffer("pful!"));
    CHECK(std::string_view(shifty.data(0, 4)) == "pful");
    CHECK(str.substr(0, 8) == "Helpful!");

    shifty.grow((1024 - 40));
    CHECK(shifty.capacity() == 1021);
    CHECK(shifty.size() == 1021);
    // Dump most of the input
    shifty.consume(1018);
    CHECK(shifty.size() == 3);
    CHECK(shifty.capacity() == 3);
    neo::buffer_copy(shifty.data(0, 3), neo::const_buffer("Hi!"));
    CHECK(str.substr(1021, 3) == "Hi!");
    // Growing a tiny bit more will shift everyone down again
    shifty.grow(4);
    CHECK(shifty.size() == 7);
    // We're back at the bottom, so we have our capacity back
    CHECK(shifty.capacity() == 1024);
    // We've garbled the string, though
    CHECK(str.substr(0, 8) == "Hi!pful!");
    shifty.grow(1024 - 7);
    // We're back at-capacity
    CHECK(shifty.size() == 1024);
    // Consume a few
    shifty.consume(4);
    CHECK(shifty.size() == 1020);
    CHECK(shifty.capacity() == 1020);
    // Grow, but just barely too much to fit by shifting down
    shifty.grow(5);  // Overflows the 1024 capacity
    // We've shifted back down, and the string has been expanded
    CHECK(str.size() == 2048);
    CHECK(shifty.capacity() == 2048);
    // We've garbled the string further
}

TEST_CASE("Shrinking to zero resets the offset") {
    std::string                  str;
    neo::shifting_dynamic_buffer dbuf{neo::as_dynamic_buffer(str)};
    str.resize(256);
    CHECK(dbuf.capacity() == 256);
    dbuf.grow(250);
    CHECK(dbuf.capacity() == 256);
    dbuf.consume(245);
    CHECK(dbuf.capacity() == 11);
    CHECK(dbuf.size() == 5);
    dbuf.shrink(4);
    CHECK(dbuf.capacity() == 11);
    CHECK(dbuf.size() == 1);
    // When we clear a shifting buffer, we reset the offset since there's nothing to copy
    dbuf.shrink(1);
    CHECK(dbuf.capacity() == 256);
}

TEST_CASE("With bare dynamic_buffer_convertible") {
    std::string                  str;
    neo::shifting_dynamic_buffer dbuf{str};
    str.resize(256);
    CHECK(dbuf.capacity() == 256);
    dbuf.grow(250);
    CHECK(dbuf.capacity() == 256);
    dbuf.consume(245);
    CHECK(dbuf.capacity() == 11);
    CHECK(dbuf.size() == 5);
    dbuf.shrink(4);
    CHECK(dbuf.capacity() == 11);
    CHECK(dbuf.size() == 1);
    // When we clear a shifting buffer, we reset the offset since there's nothing to copy
    dbuf.shrink(1);
    CHECK(dbuf.capacity() == 256);
}
