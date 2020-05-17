#include <neo/buffer_algorithm/copy.hpp>
#include <neo/buffer_algorithm/size.hpp>

#include <catch2/catch.hpp>

#include <neo/as_buffer.hpp>
#include <neo/const_buffer.hpp>
#include <neo/mutable_buffer.hpp>

#include <array>
#include <cstring>
#include <string_view>

struct dummy_buffer_range {
    neo::const_buffer* begin() const { return nullptr; }
    neo::const_buffer* end() const { return nullptr; }
};

TEST_CASE("buffer_size of empty") {
    dummy_buffer_range dbuf;
    CHECK(neo::buffer_size(dbuf) == 0);
}

TEST_CASE("Array of buffers") {
    int  arr1[]     = {1, 2, 3, 4, 5};
    int  arr2[]     = {1, 4};
    int  arr_dest[] = {0, 0, 0, 0, 0, 0, 0};
    auto buf_seq    = std::array{neo::as_buffer(arr1), neo::as_buffer(arr2)};
    CHECK(neo::buffer_size(buf_seq) == sizeof arr1 + sizeof arr2);
    CHECK(neo::buffer_size(buf_seq) == sizeof arr_dest);
    neo::buffer_copy(neo::as_buffer(arr_dest), buf_seq);
    CHECK(std::memcmp(arr1, arr_dest, sizeof arr1) == 0);
    CHECK(std::memcmp(arr2, arr_dest + 5, sizeof arr2) == 0);
}

TEST_CASE("buffer_size with real buffers") {
    auto cbuf = neo::const_buffer("I am a string");
    CHECK(neo::buffer_size(cbuf) == cbuf.size());

    std::string str  = "I am another string";
    auto        mbuf = neo::mutable_buffer(neo::byte_pointer(str.data()), str.size());
    CHECK(neo::buffer_size(mbuf) == mbuf.size());
    CHECK(neo::buffer_size(mbuf) == str.size());
}

TEST_CASE("Single-buf iterator") {
    auto buf      = neo::const_buffer("A string");
    auto buf_iter = std::begin(buf);
    CHECK(buf_iter->data() == buf.data());
    CHECK(buf_iter != std::end(buf));
    CHECK(neo::buffer_size(*buf_iter) == neo::buffer_size(buf));
    ++buf_iter;
    CHECK(buf_iter == std::end(buf));
}
