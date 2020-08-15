#include <neo/byte_array.hpp>

#include <neo/as_buffer.hpp>

#include <neo/test_concept.hpp>

#include <catch2/catch.hpp>

NEO_TEST_CONCEPT(neo::as_buffer_convertible<neo::byte_array<12>>);

TEST_CASE("Create a byte array") {
    neo::byte_array arr = {std::byte(3), std::byte(2)};
    CHECK(arr.size() == 2);

    neo::byte_array<0> empty;
    CHECK(empty.empty());
}
