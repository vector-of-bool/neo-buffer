#include <neo/as_buffer.hpp>
#include <neo/buffer_algorithm/copy.hpp>

#include <catch2/catch.hpp>

using neo::as_buffer;
using neo::buffer_copy;

TEST_CASE("Copy simple bytes") {
    std::string s1 = "Hello!";
    std::string s2;
    s2.resize(s1.size());
    neo::buffer_copy(as_buffer(s2), as_buffer(s1));
    CHECK(s1 == s2);
}

TEST_CASE("Copy with mismatched size") {
    std::string s1 = "Hello, world!";
    std::string s2 = "Hello!";
    buffer_copy(as_buffer(s2), as_buffer(s1));
    CHECK(s2 == "Hello,");
}

TEST_CASE("Copy with overlap") {
    const std::string orig = "Hello, buffer world!";

    // safe-copy will copy backwards:
    auto s1 = orig;
    buffer_copy(as_buffer(s1) + 7, as_buffer(s1));
    CHECK(s1 == "Hello, Hello, buffer");

    // Copying forward gives strange (but deterministic) results:
    s1 = orig;
    buffer_copy(as_buffer(s1) + 7, as_buffer(s1), neo::ll_buffer_copy_forward);
    CHECK(s1 == "Hello, Hello, Hello,");

    // Copy-forward is fine in this case
    s1 = "first, second, third";
    buffer_copy(as_buffer(s1), as_buffer(s1) + 7, neo::ll_buffer_copy_forward);
    CHECK(s1 == "second, third, third");

    // Copy-backward is wrong
    s1 = "first, second, third";
    buffer_copy(as_buffer(s1), as_buffer(s1) + 7, neo::ll_buffer_copy_backward);
    CHECK(s1 == " third, third, third");

    // Safe-copy will get it right:
    s1 = "first, second, third";
    buffer_copy(as_buffer(s1), as_buffer(s1) + 7);
    CHECK(s1 == "second, third, third");
}
