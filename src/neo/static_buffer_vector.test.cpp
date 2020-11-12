#include <neo/static_buffer_vector.hpp>

#include <neo/buffer_algorithm.hpp>
#include <neo/const_buffer.hpp>

#include <catch2/catch.hpp>

using namespace neo;

static_assert(buffer_range<static_buffer_vector<const_buffer, 4>>);
static_assert(mutable_buffer_range<static_buffer_vector<mutable_buffer, 4>>);

TEST_CASE("Create a zero-length buffer vector") {
    static_buffer_vector<const_buffer, 0> b;
    CHECK(buffer_count(b) == 0);
    CHECK(b.size() == 0);
    CHECK(b.max_size() == 0);
}

TEST_CASE("Simple buffer vector") {
    static_buffer_vector<const_buffer, 2> b;
    CHECK(b.size() == 0);
    CHECK(b.max_size() == 2);
    // Push one
    b.push_back(const_buffer("Simple string"));
    CHECK(b.size() == 1);
    CHECK(b[0].size() == 13);
    CHECK(std::distance(b.begin(), b.end()) == 1);
    CHECK(buffer_size(b) == 13);
    // Push another
    b.push_back(const_buffer("Another one"));
    CHECK(b[1].size() == 11);
    CHECK(buffer_count(b) == 2);
    CHECK(buffer_size(b) == 24);
}
