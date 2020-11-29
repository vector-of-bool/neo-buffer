#include <neo/counting_buffers.hpp>
#include <neo/string_io.hpp>

#include <catch2/catch.hpp>

using namespace std::literals;

TEST_CASE("Create a counting buffer") {
    neo::string_dynbuf_io out;
    neo::string_dynbuf_io in{"I am a string"};

    std::size_t count = 0;
    neo::buffer_copy(neo::counting_buffers{out, [&](auto ev) { count += ev.bytes_committed; }}, in);
    CHECK(out.read_area_view() == "I am a string");
    CHECK(count == out.read_area_view().size());
}
