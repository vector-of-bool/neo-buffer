#include <neo/bit_cast.hpp>

#include <catch2/catch.hpp>

TEST_CASE("Bit cast an integer") {
    int i  = 12;
    int i2 = neo::bit_cast<int>(i);
    CHECK(i2 == 12);
}

TEST_CASE("Buffer-style bit-cast an array") {
    std::array<int, 5> arr  = {1, 2, 3, 4, 5};
    auto               buf  = neo::as_buffer(arr);
    auto               arr2 = neo::buffer_bit_cast<std::array<int, 3>>(buf + sizeof(int));
    CHECK(arr2 == std::array{2, 3, 4});
}
