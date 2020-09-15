#include "./pathological_buffer_range.hpp"

#include <neo/test_concept.hpp>

#include <catch2/catch.hpp>

NEO_TEST_CONCEPT(neo::buffer_range<neo::pathological_buffer_range<neo::const_buffer>>);
NEO_TEST_CONCEPT(neo::mutable_buffer_range<neo::pathological_buffer_range<neo::mutable_buffer>>);

TEST_CASE("Iterate over a pathological buffer case") {
    neo::pathological_buffer_range rng(neo::const_buffer("Hello, world!"));

    auto it = rng.begin();
    CHECK(it->size() == 1);
    CHECK((*it)[0] == std::byte('H'));
    ++it;
    CHECK((*it)[0] == std::byte('e'));
}
