#include <neo/io_buffer.hpp>

#include <neo/as_dynamic_buffer.hpp>
#include <neo/buffer_algorithm/copy.hpp>

#include <catch2/catch.hpp>

TEST_CASE("Create an input buffer adaptor around a string") {
    std::string str = "Hello, user!";

    neo::input_buffer_adaptor in{neo::as_buffer(str)};

    CHECK(neo::buffer_size(in.read(1024)) == str.size());
    CHECK(std::string_view(in.read(1024)) == "Hello, user!");
    in.consume(7);
    CHECK(std::string_view(in.read(1024)) == "user!");
    in.consume(5);
    CHECK(std::string_view(in.read(1024)) == "");
}

TEST_CASE("Create an output buffer adaptor around a string") {
    std::string str = "Hello, user!";

    neo::output_buffer_adaptor out{neo::as_buffer(str)};
    // We already have a pending area due to the string's contents
    out.commit(7);
    neo::buffer_copy(out.prepare(4), neo::const_buffer("Mike"));
    CHECK(str == "Hello, Mike!");  // Hello, Joe!
}

TEST_CASE("Create an IO adapter around std::string") {
    std::string str;

    neo::dynamic_io_buffer_adaptor io(neo::as_dynamic_buffer(str));

    auto buf       = io.prepare(6);
    auto n_written = neo::buffer_copy(buf, neo::const_buffer("Hello!"));
    CHECK(n_written == str.size());
    CHECK(str == "Hello!");

    // There is no read area yet
    CHECK(neo::buffer_size(io.read()) == 0);
    // Commit data from the write-area into the read-area
    io.commit(n_written);
    // Now we have data:
    CHECK(neo::buffer_size(io.read()) == 6);
    CHECK(std::string_view(io.read()) == "Hello!");
}
