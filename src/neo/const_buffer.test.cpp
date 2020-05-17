#include <neo/const_buffer.hpp>

#include <catch2/catch.hpp>

#include <iostream>
#include <string_view>

using namespace std::string_view_literals;

TEST_CASE("Default construct") {
    neo::const_buffer buf;
    CHECK(buf.size() == 0);
    CHECK(buf.data() == nullptr);
}

TEST_CASE("From pointer and size") {
    const char*       string = "I am a string";
    neo::const_buffer buf{neo::byte_pointer(string), 4};
    CHECK(buf.data() == neo::byte_pointer(string));
    CHECK(buf.size() == 4);
    CHECK(buf[0] == std::byte{'I'});
    CHECK(buf[1] == std::byte{' '});
    CHECK(buf[2] == std::byte{'a'});
    CHECK(buf[3] == std::byte{'m'});
}

TEST_CASE("From string literal") {
    auto buf = neo::const_buffer("meow");
    CHECK(buf.size() == 4);
    CHECK(buf[0] == std::byte{'m'});
    CHECK(buf[1] == std::byte{'e'});
    CHECK(buf[2] == std::byte{'o'});
    CHECK(buf[3] == std::byte{'w'});
}

TEST_CASE("From mutable buffer") {
    std::string         str = "Hello!";
    neo::mutable_buffer mbuf{neo::byte_pointer(str.data()), str.size()};
    neo::const_buffer   cbuf = mbuf;
    CHECK(mbuf.data() == cbuf.data());
    CHECK(mbuf[0] == cbuf[0]);
    CHECK(cbuf[0] == std::byte{'H'});
    mbuf[0] = std::byte{'C'};
    REQUIRE(str == "Cello!");
    CHECK(cbuf[0] == std::byte{'C'});
}

TEST_CASE("From string_view") {
    std::string       str = "Hello!";
    neo::const_buffer buf{std::string_view{str}};
    CHECK(buf.data() == neo::byte_pointer(str.data()));
    CHECK(buf.size() == str.size());
    str[0] = 'C';
    CHECK(buf[0] == std::byte{'C'});
}

TEST_CASE("From non-char-string_view") {
    std::wstring      str = L"Hello!";
    neo::const_buffer buf{std::wstring_view{str}};
    CHECK(buf.size() == str.size() * sizeof(wchar_t));
    CHECK(buf.data() == neo::byte_pointer(buf.data()));
    CHECK(buf.equals_string(std::wstring_view{L"Hello!"}));
}

TEST_CASE("Convert to string_view") {
    std::string       my_string = "Hello, test!";
    neo::const_buffer buf{std::string_view{my_string}};
    // Go back to a string_view
    auto view = std::string_view(buf);
    CHECK(view == my_string);
    my_string[0] = 'C';
    CHECK(view[0] == 'C');
}

TEST_CASE("Advance a buffer") {
    std::string       my_string = "Hello!";
    neo::const_buffer buf{std::string_view{my_string}};
    CHECK(buf.size() == my_string.size());
    buf += 2;
    CHECK(buf.size() == (my_string.size() - 2));
    CHECK(buf[0] == std::byte{'l'});
    buf += buf.size();
    CHECK(buf.size() == 0);

    // Use remove_prefix
    buf = neo::const_buffer{my_string};
    CHECK(buf.size() == my_string.size());
    buf.remove_prefix(3);
    CHECK(buf.size() == (my_string.size() - 3));
    CHECK(buf[0] == std::byte{'l'});
}

TEST_CASE("Remove a suffix") {
    std::string my_string = "Hello!";

    auto buf = neo::const_buffer(my_string);
    CHECK(buf[0] == std::byte{'H'});
    buf.remove_suffix(1);
    CHECK(buf.size() == my_string.size() - 1);
    CHECK(buf[0] == std::byte{'H'});
}

TEST_CASE("Get a head/tail") {
    std::string       str = "I am a string";
    neo::const_buffer buf{str};
    CHECK(buf.size() == str.size());

    CHECK(std::string_view(buf) == "I am a string");
    neo::const_buffer part = buf.first(4);
    CHECK(std::string_view(part) == "I am");
    CHECK(part.equals_string("I am"sv));

    part = buf.last(6);
    CHECK(std::string_view(part) == "string");
    CHECK(part.equals_string("string"sv));

    auto [head, tail] = buf.split(4);
    CHECK(head.equals_string("I am"sv));
    CHECK(tail.equals_string(" a string"sv));
}

TEST_CASE("Single-buffer sequence") {
    std::string       str = "I am a string";
    neo::const_buffer buf{str};

    auto it   = std::begin(buf);
    auto stop = std::end(buf);
    CHECK(it != stop);
    CHECK(it->size() == str.size());
    CHECK(std::string_view(*it) == str);
    ++it;
    CHECK(it == stop);
}
