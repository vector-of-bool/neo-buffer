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

TEST_CASE("Concatenate some buffers") {
    std::string         world = "world";
    neo::const_buffer   cb{"Hello, "};
    neo::mutable_buffer mb{world};

    neo::buffers_cat cat1{cb, mb};

    auto it = cat1.begin();
    CHECK(it->equals_string("Hello, "sv));
    ++it;
    CHECK(it->equals_string("world"sv));

    CHECK(neo::buffer_size(cat1) == 12);

    auto inner = {neo::const_buffer("brave "), neo::const_buffer{"new "}};

    std::string greeting;
    greeting.resize(12);
    neo::buffer_copy(neo::as_buffer(greeting), cat1);
    CHECK(greeting == "Hello, world");

    neo::buffers_cat cat2{cb, inner, mb};
    greeting.clear();
    neo::buffer_transform(neo::buffer_copy_transformer(), neo::as_dynamic_buffer(greeting), cat2);
    CHECK(greeting == "Hello, brave new world");

#if !NEO_COMPILER_IS_MSVC
    /// XXX: A bug on MSVC causes the default constructor to collide with the N-ary constructor for
    /// N=0, as the attached `requires` clause is not evaluated at the right time.
    // Empty buffers_cat is okay
    neo::buffers_cat<> empty_cat;
    static_cast<void>(empty_cat);
#endif

    // Copy-constructor will not deduce to a nested concatenation
    neo::buffers_cat cat3(cat2);
    static_assert(neo::same_as<decltype(cat3), decltype(cat2)>);

    // We can assign-over buffers_cat
    cat2 = cat3;

    // We can have a buffers_cat of one element

    neo::buffers_cat single_cat(neo::as_buffer(greeting));
    // Copy it:
    auto             s2 = single_cat;
    neo::buffers_cat single_cat2(s2);
    static_assert(neo::same_as<decltype(single_cat2), decltype(single_cat)>);

    std::string greeting2;
    neo::buffer_transform(neo::buffer_copy_transformer(),
                          neo::as_dynamic_buffer(greeting2),
                          single_cat2);
    CHECK(greeting2 == "Hello, brave new world");
}

NEO_TEST_CONCEPT(neo::buffer_range<neo::buffers_cat<neo::const_buffer, neo::const_buffer>>);
NEO_TEST_CONCEPT(
    neo::forward_iterator<neo::buffers_cat<neo::const_buffer&, neo::const_buffer&>::iterator>);
