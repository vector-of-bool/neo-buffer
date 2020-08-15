#include <neo/buffer_bits.hpp>

#include <neo/as_buffer.hpp>

#include <catch2/catch.hpp>

TEST_CASE("View the bits in a buffer") {
    auto bytes = {
        std::byte(0b1101'1001),
        std::byte(0b1100'1011),
        std::byte(0b0011'1001),
        std::byte(0b1001'1111),
    };

    auto cb = neo::const_buffer(bytes);

    // Instantiate with no buffer:
    neo::buffer_bits<neo::const_buffer> bits_null;

    // Look at them bits
    neo::buffer_bits bits(cb);

    // Copy:
    auto bits2 = bits;

    // Read the entire thing at once
    CHECK(bits.read(32) == 0b1101'1001'1100'1011'0011'1001'1001'1111);

    bits = bits2;
    CHECK(bits.read(3) == 0b110);
    CHECK(bits.read(3) == 0b110);
    CHECK(bits.read(3) == 0b011);
    CHECK(bits.read(6) == 0b100101);
    bits.skip(1);
    CHECK(bits.read(8) == 0b0011'1001);

    // Reset to the front
    bits = bits2;
    // Read two whole bytes
    CHECK(bits.read(16) == 0b1101'1001'1100'1011);

    // Read two and a half bytes
    bits = bits2;
    CHECK(bits.read(20) == 0b1101'1001'1100'1011'0011);

    // Read two bytes, starting from the middle of a byte
    bits = bits2;
    bits.skip(4);
    CHECK(bits.read(16) == 0b1001'1100'1011'0011);

    // Read three bits, then skip to the boundary
    bits = bits2;
    CHECK(bits.read(3) == 0b110);
    bits.skip_to_byte_boundary();
    bits.skip_to_byte_boundary();  // Multiple calls do nothing
    bits.skip_to_byte_boundary();
    bits.skip_to_byte_boundary();
    CHECK(bits.read(8) == 0b1100'1011);
}

TEST_CASE("Set the bits in a buffer") {
    std::byte bytes[] = {
        std::byte(0b0000'0000),
        std::byte(0b0000'0000),
        std::byte(0b0000'0000),
        std::byte(0b0000'0000),
    };

    neo::buffer_bits bits(neo::as_buffer(bytes));

    bits.set(0b101, 3);

    CHECK(bytes[0] == std::byte(0b1010'0000));

    bits.set(0b1110'0111'1011, 12);
    CHECK(bytes[0] == std::byte(0b1110'0111));
    CHECK(bytes[1] == std::byte(0b1011'0000));

    bits.skip(8);
    bits.write(0b11, 2);
    CHECK(bytes[1] == std::byte(0b1111'0000));

    bits.skip(2);
    bits.write(0b1010, 4);
    CHECK(bytes[1] == std::byte(0b1111'1010));
}
