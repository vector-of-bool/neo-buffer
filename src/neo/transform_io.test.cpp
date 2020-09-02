#include <neo/transform_io.hpp>

#include <neo/string_io.hpp>
#include <neo/test_concept.hpp>

#include <catch2/catch.hpp>

namespace {

struct bitnot_transformer {
    auto operator()(neo::mutable_buffer out, neo::const_buffer in) const noexcept {
        std::size_t count = 0;
        while (out && in) {
            out[0] = ~in[0];
            out += 1;
            in += 1;
            ++count;
        }
        return neo::simple_transform_result{count, count, false};
    }
};

}  // namespace

NEO_TEST_CONCEPT(neo::buffer_transform_result<neo::simple_transform_result>);
NEO_TEST_CONCEPT(neo::buffer_transformer<bitnot_transformer>);

NEO_TEST_CONCEPT(
    neo::buffer_sink<neo::buffer_transform_sink<neo::proto_buffer_sink, bitnot_transformer>>);

TEST_CASE("Create a simple sink") {
    // Create a destination
    neo::string_io_buffer out;
    // A sink that transforms by passing data through a bitnot_transformer
    neo::buffer_transform_sink bitnot_sink{out, bitnot_transformer()};
    // The string we are testing:
    const std::string original = "Hello, world!";

    // Send the string through the transforming sink:
    neo::buffer_copy(bitnot_sink, neo::as_buffer(original));
    out.shrink_uncommitted();

    // Check that we get a result
    std::string inverted_str = out.storage();
    CHECK(inverted_str != original);
    CHECK(inverted_str.size() == original.size());

    // Copy the string back
    out = {};
    neo::buffer_copy(bitnot_sink, neo::as_buffer(inverted_str));
    CHECK(out.read_area_view() == original);
}

TEST_CASE("Source with a fixed buffer") {
    std::string            str = "I am a string";
    neo::dynamic_io_buffer input{str};

    neo::buffer_transform_source bitnot_source{input, bitnot_transformer{}};

    neo::string_io_buffer out;
    neo::buffer_copy(out, bitnot_source);
    CHECK(out.string().size() == 13);
    CHECK(out.string()[0] == ~'I');
}
