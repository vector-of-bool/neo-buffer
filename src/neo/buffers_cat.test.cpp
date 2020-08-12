#include <neo/buffers_cat.hpp>

#include <neo/as_dynamic_buffer.hpp>
#include <neo/buffer_algorithm/copy.hpp>
#include <neo/buffer_algorithm/count.hpp>
#include <neo/buffer_algorithm/size.hpp>
#include <neo/buffer_algorithm/transform.hpp>
#include <neo/platform.hpp>

#include <neo/test_concept.hpp>

#include <catch2/catch.hpp>

using namespace std::literals;

using neo::const_buffer;
using neo::mutable_buffer;

namespace {

template <neo::buffer_range R>
void check_buffer_str(R&& r, std::string_view expect) {
    std::string result;
    neo::buffer_transform(neo::buffer_copy_transformer(), neo::as_dynamic_buffer(result), r);
    CHECK(result == expect);
}

#define CHECK_BUFFER_STR(Buf, String)                                                              \
    do {                                                                                           \
        std::string result;                                                                        \
        neo::buffer_transform(neo::buffer_copy_transformer(),                                      \
                              neo::as_dynamic_buffer(result),                                      \
                              Buf);                                                                \
        CHECK(result == String);                                                                   \
    } while (0)

}  // namespace

TEST_CASE("Concat buffer arrays") {
    // Concat just two buffers:
    std::array<neo::const_buffer, 2> pair
        = neo::buffers_cat(neo::const_buffer("foo"), neo::const_buffer("bar"));
    CHECK_BUFFER_STR(pair, "foobar");

    // Three buffers
    auto                             str  = "baz"s;
    std::array<neo::const_buffer, 3> trio = neo::buffers_cat(neo::const_buffer("foo"),
                                                             neo::const_buffer("bar"),
                                                             neo::mutable_buffer(str));
    CHECK_BUFFER_STR(trio, "foobarbaz");

    // All mutable buffers?
    std::array<neo::mutable_buffer, 2> pair2
        = neo::buffers_cat(neo::mutable_buffer(str), neo::mutable_buffer(str));
    CHECK_BUFFER_STR(pair2, "bazbaz");

    // Append two arrays
    std::array<neo::const_buffer, 4> quad = neo::buffers_cat(pair, pair2);
    CHECK_BUFFER_STR(quad, "foobarbazbaz");

    // Append a single buffer to an array
    auto five_bufs = neo::buffers_cat(quad, neo::const_buffer("quux"));
    CHECK_BUFFER_STR(five_bufs, "foobarbazbazquux");

    auto five_bufs2 = neo::buffers_cat(neo::const_buffer("quux"), quad);
    CHECK_BUFFER_STR(five_bufs2, "quuxfoobarbazbaz");

    // Append multiple simple buffer sequences
    std::array<neo::const_buffer, 7> seven_bufs
        = neo::buffers_cat(pair, pair, neo::const_buffer("boo"), pair);
    CHECK_BUFFER_STR(seven_bufs, "foobarfoobarboofoobar");

    using complex_buffer = neo::static_buffer_vector<const_buffer, 42>;
    complex_buffer bvec;

    neo::buffers_seq_concat<complex_buffer&, std::array<const_buffer, 2>&> tail_check
        = neo::buffers_cat(bvec, pair);
    CHECK_BUFFER_STR(tail_check, "foobar");

    neo::buffers_seq_concat<std::array<const_buffer, 4>,
                            complex_buffer&,
                            std::array<const_buffer, 2>&>
        test = neo::buffers_cat(pair, pair, bvec, pair);
    CHECK_BUFFER_STR(test, "foobarfoobarfoobar");

    neo::buffers_seq_concat<std::array<const_buffer, 4>, complex_buffer&> test2
        = neo::buffers_cat(pair, pair, bvec);
    CHECK_BUFFER_STR(test2, "foobarfoobar");

    auto cat1 = neo::buffers_cat(bvec, bvec);
    auto cat2 = neo::buffers_cat(bvec, bvec);
    neo::buffers_seq_concat<complex_buffer&, complex_buffer&, complex_buffer&, complex_buffer&>
        cat1_cat2 = neo::buffers_cat(cat1, cat2);
    CHECK_BUFFER_STR(cat1_cat2, "");
    bvec.push_back(const_buffer("123"));
    CHECK_BUFFER_STR(cat1_cat2, "123123123123");
}

// Check the result types for each buffer concatenation operation
NEO_TEST_CONCEPT(neo::same_as<neo::buffers_cat_t<>, std::array<mutable_buffer, 0>>);
NEO_TEST_CONCEPT(neo::same_as<neo::buffers_cat_t<const_buffer>, const_buffer>);
NEO_TEST_CONCEPT(
    neo::same_as<neo::buffers_cat_t<const_buffer, mutable_buffer>, std::array<const_buffer, 2>>);
NEO_TEST_CONCEPT(neo::same_as<neo::buffers_cat_t<mutable_buffer, mutable_buffer>,
                              std::array<mutable_buffer, 2>>);
NEO_TEST_CONCEPT(neo::same_as<neo::buffers_cat_t<std::array<const_buffer, 2>, const_buffer>,
                              std::array<const_buffer, 3>>);
NEO_TEST_CONCEPT(neo::same_as<neo::buffers_cat_t<>, std::array<mutable_buffer, 0>>);
NEO_TEST_CONCEPT(
    neo::same_as<neo::buffers_cat_t<std::array<mutable_buffer, 5>, std::array<mutable_buffer, 6>>,
                 std::array<mutable_buffer, 11>>);
NEO_TEST_CONCEPT(neo::same_as<neo::buffers_cat_t<std::array<const_buffer, 5>,
                                                 std::array<mutable_buffer, 3>,
                                                 std::array<const_buffer, 3>>,
                              std::array<const_buffer, 11>>);
NEO_TEST_CONCEPT(neo::same_as<neo::buffers_cat_t<std::array<const_buffer, 5>,
                                                 std::array<mutable_buffer, 3>,
                                                 const_buffer,
                                                 std::array<const_buffer, 3>>,
                              std::array<const_buffer, 12>>);
NEO_TEST_CONCEPT(neo::same_as<neo::buffers_cat_t<std::array<const_buffer, 5>,
                                                 std::array<mutable_buffer, 3>,
                                                 neo::proto_buffer_range,
                                                 std::array<const_buffer, 3>>,
                              neo::buffers_seq_concat<std::array<const_buffer, 8>,
                                                      neo::proto_buffer_range,
                                                      std::array<const_buffer, 3>>>);
NEO_TEST_CONCEPT(
    neo::same_as<
        neo::buffers_cat_t<neo::buffers_seq_concat<neo::static_buffer_vector<const_buffer, 4>,
                                                   neo::static_buffer_vector<const_buffer, 6>>,
                           neo::buffers_seq_concat<neo::static_buffer_vector<const_buffer, 9>,
                                                   neo::static_buffer_vector<const_buffer, 1>>>,
        neo::buffers_seq_concat<neo::static_buffer_vector<const_buffer, 4>,
                                neo::static_buffer_vector<const_buffer, 6>,
                                neo::static_buffer_vector<const_buffer, 9>,
                                neo::static_buffer_vector<const_buffer, 1>>>);
