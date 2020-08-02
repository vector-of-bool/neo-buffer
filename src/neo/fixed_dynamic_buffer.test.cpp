#include <neo/fixed_dynamic_buffer.hpp>

#include <neo/dynamic_buffer.hpp>
#include <neo/shifting_dynamic_buffer.hpp>

#include <neo/test_concept.hpp>

#include <catch2/catch.hpp>

#include <array>
#include <string>

NEO_TEST_CONCEPT(neo::dynamic_buffer<neo::fixed_dynamic_buffer<std::string>>);

TEST_CASE("Fixed dynbuf from a string") {
    std::string str;
    str.resize(400);
    neo::fixed_dynamic_buffer fixed{str};
    CHECK(fixed.size() == str.size());
    CHECK(fixed.max_size() == str.size());
    CHECK(fixed.grow(0).size() == 0);
}

TEST_CASE("Fixed dynbuf from an array") {
    neo::fixed_dynamic_buffer<std::array<std::byte, 64>> dbuf;

    CHECK(dbuf.size() == 64);
    CHECK(dbuf.max_size() == 64);
}

TEST_CASE("Shifting buffer around a fixed buffer") {
    std::array<std::byte, 1024>  arr;
    neo::fixed_dynamic_buffer    fixed{arr};
    neo::shifting_dynamic_buffer dbuf{fixed};
    CHECK(dbuf.size() == arr.size());
    dbuf.consume(64);
    dbuf.grow(0);
    CHECK(dbuf.size() == (arr.size() - 64));
    // Reduced capacity since the base has been shifted:
    CHECK(dbuf.capacity() == (arr.size() - 64));
    // We can grow *up to* 64 bytes more since there is room in the underlying array
    CHECK(dbuf.max_size() == arr.size());
    dbuf.grow(60);
    // We down-shifted, so we are back to full capacity
    CHECK(dbuf.capacity() == arr.size());
}

TEST_CASE("Shifting buffer around a fixed buffer - start empty") {
    std::array<std::byte, 1024>  arr;
    neo::fixed_dynamic_buffer    fixed{arr};
    neo::shifting_dynamic_buffer dbuf{fixed, 0};
    CHECK(dbuf.size() == 0);
    CHECK(dbuf.capacity() == arr.size());
    CHECK(dbuf.max_size() == arr.size());
}
