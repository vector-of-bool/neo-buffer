#include <neo/buffer_range_adaptor.hpp>

#include <neo/as_dynamic_buffer.hpp>
#include <neo/buffer_algorithm/copy.hpp>
#include <neo/buffer_algorithm/transform.hpp>
#include <neo/dynbuf_io.hpp>

#include <catch2/catch.hpp>

NEO_TEST_CONCEPT(
    neo::buffer_iterator<neo::buffer_range_adaptor<std::array<std::string, 2>>::iterator>);
NEO_TEST_CONCEPT(neo::buffer_range<neo::buffer_range_adaptor<std::vector<std::string>>>);

TEST_CASE("Create a simple adapter") {
    std::vector<std::string> strings = {"Hello, ", "world!"};
    std::string              full_str;

    neo::buffer_range_adaptor adapted_strings{strings};

    auto size = neo::buffer_transform(neo::buffer_copy_transformer{},
                                      neo::dynbuf_io(full_str),
                                      adapted_strings)
                    .bytes_written;
    full_str.resize(size);
    CHECK(full_str == "Hello, world!");
}
