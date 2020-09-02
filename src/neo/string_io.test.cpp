#include <neo/string_io.hpp>

#include <catch2/catch.hpp>

TEST_CASE("String IO") {
    neo::string_io_buffer strbuf;
    CHECK(strbuf.string().empty());
    neo::buffer_copy(strbuf, neo::const_buffer("Hello, string!"));
    CHECK(strbuf.available() == 14);
    CHECK(strbuf.read_area_view() == "Hello, string!");
}

TEST_CASE("Shifting string IO") {
    neo::shifting_string_io_buffer strbuf;
    CHECK(strbuf.string().empty());
    neo::buffer_copy(strbuf, neo::const_buffer("Hello, string!"));
    CHECK(strbuf.available() == 14);
    CHECK(strbuf.read_area_view() == "Hello, string!");
    strbuf.consume(3);
    CHECK(strbuf.available() == 11);
    CHECK(strbuf.read_area_view() == "lo, string!");
}
