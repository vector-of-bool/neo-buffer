#include <neo/io_buffer.hpp>

#include <neo/as_dynamic_buffer.hpp>
#include <neo/buffer_algorithm/copy.hpp>
#include <neo/fixed_dynamic_buffer.hpp>

#include <neo/test_concept.hpp>

#include <catch2/catch.hpp>

TEST_CASE("Create an IO adapter around std::string") {
    std::string str;

    neo::dynamic_io_buffer io(neo::as_dynamic_buffer(str));

    auto buf       = io.prepare(6);
    auto n_written = neo::buffer_copy(buf, neo::const_buffer("Hello!"));
    CHECK(n_written == str.size());
    CHECK(str == "Hello!");

    // There is no read area yet
    CHECK(neo::buffer_size(io.next(1024)) == 0);
    // Commit data from the write-area into the read-area
    io.commit(n_written);
    // Now we have data:
    CHECK(std::string_view(io.next(1024)) == "Hello!");
}

TEST_CASE("Create an I/O buffer adaptor that respects a restricted max_size()") {
    std::string str;
    str.resize(16);
    neo::fixed_dynamic_buffer dbuf{str};

    neo::dynamic_io_buffer io{dbuf, 0};
}

TEST_CASE("Grow a string with a WAY too large request") {
    std::string            str;
    neo::dynamic_io_buffer dbuf{str};
    CHECK_NOTHROW(dbuf.prepare(std::numeric_limits<std::size_t>::max() - 92));
}