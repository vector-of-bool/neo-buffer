#include <neo/as_dynamic_buffer.hpp>

#include <neo/buffer_algorithm.hpp>

#include <catch2/catch.hpp>

template <neo::dynamic_buffer T>
void check_dynbuf(T) {}

TEST_CASE("dynamic_string_buffer") {
    std::string str;
    auto        dynbuf = neo::as_dynamic_buffer(str);
    static_assert(std::is_same_v<decltype(dynbuf), neo::dynamic_string_buffer<std::string>>);
    check_dynbuf(dynbuf);

    CHECK(dynbuf.size() == 0);
    CHECK(dynbuf.max_size() == str.max_size());
    CHECK(dynbuf.capacity() == str.capacity());

    neo::mutable_buffer buf = dynbuf.grow(50);
    CHECK(str.size() == buf.size());
    CHECK(neo::byte_pointer(str.data()) == buf.data());

    // Copy one string into another
    std::string in_str = "I am a string";
    neo::buffer_copy(buf, neo::as_buffer(in_str));
    // The new string is now at the beginning:
    CHECK(str.find(in_str) == 0);

    // Consume some content from the front:
    dynbuf.consume(5);
    // We've moved the front:
    CHECK(str.find("a string") == 0);

    // Content is preserved if we grow larger:
    dynbuf.grow(200);
    CHECK(str.find("a string") == 0);

    // We can eat the entire buffer:
    dynbuf.consume(dynbuf.size());
    CHECK(str == "");

    // Consuming more data is a no-op
    dynbuf.consume(1);
    CHECK(str == "");
}