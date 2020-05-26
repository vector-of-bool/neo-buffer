#include <neo/io_buffer.hpp>

#include <neo/as_dynamic_buffer.hpp>
#include <neo/buffer_algorithm/copy.hpp>

#include <catch2/catch.hpp>

TEST_CASE("Create an IO adapter around std::string") {
    std::string str;

    neo::dynamic_io_buffer_adaptor io(neo::as_dynamic_buffer(str));

    auto buf       = io.prepare(6);
    auto n_written = neo::buffer_copy(buf, neo::const_buffer("Hello!"));
    CHECK(n_written == str.size());
    CHECK(str == "Hello!");

    // There is no read area yet
    CHECK(neo::buffer_size(io.data()) == 0);
    // Commit data from the write-area into the read-area
    io.commit(n_written);
    // Now we have data:
    CHECK(neo::buffer_size(io.data()) == 6);
    CHECK(std::string_view(io.data()) == "Hello!");
}
