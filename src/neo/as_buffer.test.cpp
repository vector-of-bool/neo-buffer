#include <neo/as_buffer.hpp>

#include <catch2/catch.hpp>

#include <neo/buffer_algorithm.hpp>

#include <array>
#include <string>

struct my_simple_struct {
    int a;
    int b;
};

struct mine_with_as_buffer_member {
    int a;
    int b;

    neo::mutable_buffer as_buffer() noexcept {
        return neo::mutable_buffer(neo::byte_pointer(this), sizeof *this);
    }
};

TEST_CASE("From mutable_buffer") {
    int  i    = 0;
    auto buf  = neo::mutable_buffer(neo::byte_pointer(&i), sizeof i);
    auto buf2 = neo::as_buffer(buf);
    static_assert(std::is_same_v<decltype(buf2), neo::mutable_buffer>);
    CHECK(buf2.data() == buf.data());
    CHECK(buf2.size() == buf.size());

    // Attempt to make a larger size
    buf2 = neo::as_buffer(buf, sizeof(i) + 663);
    CHECK(buf2.data() == buf.data());
    CHECK(buf2.size() == buf.size());

    // Make a smaller size
    buf2 = neo::as_buffer(buf, sizeof(i) / 2);
    CHECK(buf2.data() == buf.data());
    CHECK(buf2.size() != buf.size());  // Will be smaller
}

TEST_CASE("From const_buffer") {
    int  i    = 0;
    auto buf  = neo::const_buffer(neo::byte_pointer(&i), sizeof i);
    auto buf2 = neo::as_buffer(buf);
    static_assert(std::is_same_v<decltype(buf2), neo::const_buffer>);
    CHECK(buf2.data() == buf.data());
    CHECK(buf2.size() == buf.size());

    // Attempt to make a larger size
    buf2 = neo::as_buffer(buf, sizeof(i) + 663);
    CHECK(buf2.data() == buf.data());
    CHECK(buf2.size() == buf.size());

    // Make a smaller size
    buf2 = neo::as_buffer(buf, sizeof(i) / 2);
    CHECK(buf2.data() == buf.data());
    CHECK(buf2.size() != buf.size());  // Will be smaller
}

template <typename BufferType, typename ArrayType>
void array_checks(ArrayType& arr) {
    using value_type     = typename std::iterator_traits<decltype(std::begin(arr))>::value_type;
    const auto count     = std::size(arr);
    const auto byte_size = count * sizeof(value_type);

    auto buf = neo::as_buffer(arr);
    static_assert(std::is_same_v<decltype(buf), BufferType>);

    CHECK(buf.data() == neo::byte_pointer(std::data(arr)));
    CHECK(buf.size() == byte_size);

    // Clips
    buf = neo::as_buffer(arr, byte_size + 2);
    CHECK(buf.data() == neo::byte_pointer(std::data(arr)));
    CHECK(buf.size() == byte_size);

    // Smaller
    buf = neo::as_buffer(arr, byte_size - 3);
    CHECK(buf.data() == neo::byte_pointer(std::data(arr)));
    CHECK(buf.size() == byte_size - 3);
}

TEST_CASE("mutable_buffer from trivial array") {
    int arr[4] = {1, 2, 3, 4};
    array_checks<neo::mutable_buffer>(arr);
}

TEST_CASE("const_buffer from trivial array") {
    const int arr[4] = {1, 2, 3, 4};
    array_checks<neo::const_buffer>(arr);
}

TEST_CASE("mutable_buffer from std::array") {
    std::array<int, 4> arr = {1, 2, 3, 4};
    array_checks<neo::mutable_buffer>(arr);
}

TEST_CASE("const_buffer from std::array of const") {
    std::array<const int, 4> arr = {1, 2, 3, 4};
    array_checks<neo::const_buffer>(arr);
}

TEST_CASE("const_buffer from const std::array") {
    const std::array<int, 4> arr = {1, 2, 3, 4};
    array_checks<neo::const_buffer>(arr);
}

TEST_CASE("mutable_buffer from std::string") {
    std::string s = "I am a string";
    array_checks<neo::mutable_buffer>(s);
}

TEST_CASE("mutable_buffer from std::wstring") {
    std::wstring s = L"I am a wide string";
    array_checks<neo::mutable_buffer>(s);
}

TEST_CASE("const_buffer from std::string") {
    const std::string s = "I am a string";
    array_checks<neo::const_buffer>(s);
}

TEST_CASE("const_buffer from std::wstring") {
    const std::wstring s = L"I am a wide string";
    array_checks<neo::const_buffer>(s);
}

TEST_CASE("const_buffer from std::string_view") {
    std::string_view s = "I am a string view";
    array_checks<neo::const_buffer>(s);
}

TEST_CASE("const_buffer from std::wstring_view") {
    std::wstring_view s = L"I am a wide string view";
    array_checks<neo::const_buffer>(s);
}

TEST_CASE("mutable_buffer from std::vector") {
    std::vector<int> vec = {5, 3, 5, 2};
    array_checks<neo::mutable_buffer>(vec);
}

TEST_CASE("const_buffer from std::vector") {
    const std::vector<int> vec = {32, 44, 11};
    array_checks<neo::const_buffer>(vec);
}

TEST_CASE("Member as_buffer") {
    mine_with_as_buffer_member m;
    m.a = 32;
    m.b = 44;
    auto mb = neo::as_buffer(m);
    CHECK(mb.size() == sizeof m);
}