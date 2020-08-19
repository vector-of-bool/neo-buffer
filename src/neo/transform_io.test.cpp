#include <neo/transform_io.hpp>

#include <neo/string_io.hpp>
#include <neo/test_concept.hpp>

#include <catch2/catch.hpp>

namespace {

class bitnot_transformer {
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
    out.shrink_uncommitted();
    CHECK(out.storage() == original);
}

TEST_CASE("Sink with a fixed dynbuf") {
    neo::shifting_string_io_buffer out;
    out.string().clear();

    neo::string_io_buffer strio;
    strio.string().clear();
}