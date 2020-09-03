#include <neo/buffer_algorithm/concat.hpp>

#include <catch2/catch.hpp>

TEST_CASE("Concat some strings") {
    std::string out;
    neo::dynbuf_concat(out, "Hello, ", "world!");
    CHECK(out == "Hello, world!");
}
