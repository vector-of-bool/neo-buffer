#include <neo/iostream_io.hpp>

#include <neo/buffer_sink.hpp>

#include <neo/test_concept.hpp>

#include <catch2/catch.hpp>

#include <sstream>

NEO_TEST_CONCEPT(neo::buffer_sink<neo::iostream_io<std::stringstream>>);
NEO_TEST_CONCEPT(neo::buffer_sink<neo::iostream_io<std::ostream>>);
NEO_TEST_CONCEPT(neo::buffer_source<neo::iostream_io<std::istream>>);

TEST_CASE("iostream IO") {
    std::stringstream strm;
    neo::iostream_io  sink{strm};
    buffer_copy(sink, neo::const_buffer("Hello!"));
    CHECK(strm.str() == "Hello!");

    neo::iostream_io source{strm};

    auto buf = source.next(512);
    CHECK(buffer_size(buf) == strm.str().size());
    CHECK(std::string_view(buf) == "Hello!");

    buf = source.next(512);
    CHECK(std::string_view(buf) == "Hello!");

    // Advance by three chars:
    source.consume(3);
    buf = source.next(512);
    CHECK(std::string_view(buf) == "lo!");

    buf = source.next(2);
    CHECK(std::string_view(buf) == "lo");
    source.consume(3);

    strm.clear();
    strm.str("Hello, world!");

    buf = source.next(5);
    CHECK(std::string_view(buf) == "Hello");
}
