#include <catch2/catch.hpp>

#include <neo/byte_pointer.hpp>

TEST_CASE("Byte pointer") {
    int  i     = 0;
    auto b_ptr = neo::byte_pointer(&i);
    static_assert(std::is_same_v<decltype(b_ptr), std::byte*>);
    CHECK(b_ptr == reinterpret_cast<std::byte*>(&i));

    const int ci     = 0;
    auto      cb_ptr = neo::byte_pointer(&ci);
    static_assert(std::is_same_v<decltype(cb_ptr), const std::byte*>);
    CHECK(cb_ptr == reinterpret_cast<const std::byte*>(&ci));
}