#include <neo/as_buffer.hpp>
#include <neo/buffer_algorithm/copy.hpp>

#include <neo/as_dynamic_buffer.hpp>
#include <neo/fixed_dynamic_buffer.hpp>
#include <neo/io_buffer.hpp>
#include <neo/shifting_dynamic_buffer.hpp>

#include <catch2/catch.hpp>

using neo::as_buffer;
using neo::buffer_copy;
using namespace neo::literals;

TEST_CASE("Copy simple bytes") {
    std::string s1 = "Hello!";
    std::string s2;
    s2.resize(s1.size());
    auto n = neo::buffer_copy(as_buffer(s2), as_buffer(s1));
    CHECK(n == s1.size());
    CHECK(s1 == s2);
}

TEST_CASE("Copy with mismatched size") {
    std::string s1 = "Hello, world!";
    std::string s2 = "Hello!";
    auto        n  = buffer_copy(as_buffer(s2), as_buffer(s1));
    CHECK(n == s2.size());
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

TEST_CASE("Copy dynamic sources and sinks") {
    neo::buffers_consumer in{neo::const_buffer("I am a string")};

    std::string str;

    auto n_copied = neo::buffer_copy(neo::dynamic_io_buffer(str), in);
    CHECK(n_copied == str.size());
    CHECK(str == "I am a string");
}

TEST_CASE("Copy buffer_range -> buffer") {
    const auto  bufs = {"foo"_buf, "bar"_buf};
    std::string str;
    str.resize(20);
    auto n = buffer_copy(as_buffer(str), bufs);
    CHECK(n == 6);
    CHECK(str.substr(0, 6) == "foobar");
}

TEST_CASE("Copy buffer -> buffer_range") {
    std::string         s1     = "foo";
    std::string         s2     = "bar";
    std::string         s3     = "baz";
    neo::mutable_buffer bufs[] = {as_buffer(s1), as_buffer(s2), as_buffer(s3)};
    neo::const_buffer   src{"howdy!"};
    std::size_t         n = buffer_copy(bufs, src);
    CHECK(n == 6);
    CHECK(s1 == "how");
    CHECK(s2 == "dy!");
    CHECK(s3 == "baz");
}

TEST_CASE("Copy buffer_range -> buffer_range") {
    const std::string s1 = "Hello ";
    const std::string s2 = "world";
    const std::string s3 = "!";
    std::string       d1 = "one";
    std::string       d2 = "two";
    std::string       d3 = "three";

    neo::const_buffer   src[]  = {as_buffer(s1), as_buffer(s2), as_buffer(s3)};
    neo::mutable_buffer dest[] = {as_buffer(d1), as_buffer(d2), as_buffer(d3)};
    auto                n      = neo::buffer_copy(dest, src);
    CHECK(n == (d1.size() + d2.size() + d3.size()));
    CHECK(d1 == "Hel");
    CHECK(d2 == "lo ");
    CHECK(d3 == "world");
}

TEST_CASE("Copy buffer -> buffer_sink") {
    std::string str;
    auto        buf = "Hello, world!"_buf;
    auto        n   = buffer_copy(neo::dynamic_io_buffer(str), buf);
    CHECK(n == buf.size());
    CHECK(str.size() == buf.size());
    CHECK(str == "Hello, world!");
}

TEST_CASE("Copy buffer_source -> buffer") {
    std::string str = "Some string";
    std::string dest;
    dest.resize(5);
    auto n = buffer_copy(neo::as_buffer(dest), neo::dynamic_io_buffer(str));
    CHECK(n == 5);
    CHECK(dest == "Some ");
}

TEST_CASE("Copy buffer_source -> buffer_sink") {
    std::string str = "I am a string";
    std::string dest;

    auto n = buffer_copy(neo::dynamic_io_buffer(dest),
                         neo::dynamic_io_buffer(
                             neo::shifting_dynamic_buffer(neo::fixed_dynamic_buffer(str))));
    CHECK(n == 13);
    CHECK(dest.substr(0, 13) == str);
}
