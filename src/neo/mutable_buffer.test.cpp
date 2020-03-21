#include <neo/buffer_seq_iter.hpp>
#include <neo/mutable_buffer.hpp>

#include <catch2/catch.hpp>

#include <iostream>
#include <string_view>

using namespace std::string_view_literals;

TEST_CASE("Default construct") {
    neo::mutable_buffer buf;
    CHECK(buf.size() == 0);
    CHECK(buf.data() == nullptr);
}

TEST_CASE("From pointer and size") {
    std::string         string = "I am a string";
    neo::mutable_buffer buf{neo::byte_pointer(string.data()), 4};
    CHECK(buf.data() == neo::byte_pointer(string.data()));
    CHECK(buf.size() == 4);
    CHECK(buf[0] == std::byte{'I'});
    CHECK(buf[1] == std::byte{' '});
    CHECK(buf[2] == std::byte{'a'});
    CHECK(buf[3] == std::byte{'m'});
}

TEST_CASE("Convert to string_view") {
    std::string         my_string = "Hello, test!";
    neo::mutable_buffer buf{my_string};
    // Go back to a string_view
    auto view = std::string_view(buf);
    CHECK(view == my_string);
    my_string[0] = 'C';
    CHECK(view == "Cello, test!");
    buf[0] = std::byte{'H'};
    CHECK(view == "Hello, test!");
}

TEST_CASE("Advance a buffer") {
    std::string         my_string = "Hello!";
    neo::mutable_buffer buf{my_string};
    CHECK(buf.size() == my_string.size());
    buf += 2;
    CHECK(buf.size() == (my_string.size() - 2));
    CHECK(buf[0] == std::byte{'l'});
    buf += buf.size();
    CHECK(buf.size() == 0);

    // Use remove_prefix
    buf = neo::mutable_buffer{my_string};
    CHECK(buf.size() == my_string.size());
    buf.remove_prefix(3);
    CHECK(buf.size() == (my_string.size() - 3));
    CHECK(buf[0] == std::byte{'l'});
}

TEST_CASE("Remove a suffix") {
    std::string my_string = "Hello!";

    auto buf = neo::mutable_buffer(my_string);
    CHECK(buf[0] == std::byte{'H'});
    buf.remove_suffix(1);
    CHECK(buf.size() == my_string.size() - 1);
    CHECK(buf[0] == std::byte{'H'});
}

TEST_CASE("Get a head/tail") {
    std::string         str = "I am a string";
    neo::mutable_buffer buf{str};
    CHECK(buf.size() == str.size());

    CHECK(std::string_view(buf) == "I am a string");
    neo::mutable_buffer part = buf.first(4);
    CHECK(std::string_view(part) == "I am");
    CHECK(part.equals_string("I am"sv));

    part = buf.last(6);
    CHECK(std::string_view(part) == "string");
    CHECK(part.equals_string("string"sv));
}

TEST_CASE("Single-buffer sequence") {
    std::string         str = "I am a string";
    neo::mutable_buffer buf{neo::byte_pointer(str.data()), str.size()};

    auto it   = neo::buffer_sequence_begin(buf);
    auto stop = neo::buffer_sequence_end(buf);
    CHECK(it != stop);
    CHECK(it->size() == str.size());
    CHECK(std::string_view(*it) == str);
    ++it;
    CHECK(it == stop);
}
