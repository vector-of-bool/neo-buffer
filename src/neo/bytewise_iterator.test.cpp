#include "./bytewise_iterator.hpp"

#include <neo/buffer_algorithm.hpp>

#include <catch2/catch.hpp>

template <neo::input_iterator It>
void check_iter() {}

TEST_CASE("Iterate over some bytes") {
    check_iter<neo::bytewise_iterator<neo::proto_const_buffer_sequence>>();

    neo::bytewise_iterator byte_iter(neo::const_buffer("hello"));
    auto                   it   = byte_iter.begin();
    const auto             stop = byte_iter.end();
    CHECK(it != stop);
    CHECK((char)*it++ == 'h');
    CHECK(it != stop);
    CHECK((char)*it++ == 'e');
    CHECK((char)*it++ == 'l');
    CHECK((char)*it++ == 'l');
    CHECK((char)*it++ == 'o');
    CHECK((it == stop));
    // Rewind
    CHECK((char)*--it == 'o');
    CHECK((char)*--it == 'l');
    CHECK((char)*--it == 'l');
    CHECK((char)*--it == 'e');
    CHECK((char)*--it == 'h');
}

TEST_CASE("Empty buffer sequence") {
    std::vector<neo::const_buffer> empty;
    neo::bytewise_iterator         it(empty);
    CHECK(it == it.end());
}

TEST_CASE("Simple buffer sequence") {
    auto                   two_bufs = {neo::const_buffer("first"), neo::const_buffer("second")};
    neo::bytewise_iterator it(two_bufs);
    const auto             stop = it.end();
    CHECK(it != stop);
    CHECK((char)*it++ == 'f');
    CHECK((char)*it++ == 'i');
    CHECK((char)*it++ == 'r');
    CHECK((char)*it++ == 's');
    CHECK((char)*it++ == 't');
    CHECK((char)*it++ == 's');
    CHECK((char)*it++ == 'e');
    CHECK((char)*it++ == 'c');
    CHECK((char)*it++ == 'o');
    CHECK((char)*it++ == 'n');
    CHECK((char)*it++ == 'd');
    it == stop;
    CHECK(it == stop);
    // Null advance
    it += 0;
    // Walking backwards now
    CHECK((char)*--it == 'd');
    CHECK((char)*--it == 'n');
    CHECK((char)*--it == 'o');
    CHECK((char)*--it == 'c');
    CHECK((char)*--it == 'e');
    CHECK((char)*--it == 's');
    CHECK((char)*--it == 't');
    CHECK((char)*--it == 's');
    CHECK((char)*--it == 'r');
    CHECK((char)*--it == 'i');
    CHECK((char)*--it == 'f');
}

TEST_CASE("Buffer skipping") {
    auto bufs = {
        neo::const_buffer("first"),
        neo::const_buffer("second"),
        neo::const_buffer("third"),
    };
    neo::bytewise_iterator it(bufs);
    auto                   stop = it.end();
    it += 2;
    CHECK((char)*it == 'r');
    it += 10;
    CHECK((char)*it++ == 'h');
    CHECK((char)*it++ == 'i');
    CHECK((char)*it++ == 'r');
    CHECK((char)*it++ == 'd');
    CHECK(it == stop);
    // Rewind again
    it -= 13;
    CHECK((char)*--it == 'r');
    CHECK((char)*--it == 'i');
    CHECK((char)*--it == 'f');

    CHECK((char)*it == 'f');
    it += 14;
    it -= 14;
    CHECK((char)*it == 'f');

    // Step back-and-forth repeatedly
    for (auto i = 0u; i < neo::buffer_size(bufs); ++i) {
        CHECK((char)*it == 'f');
        it += i;
        it -= i;
        CHECK((char)*it == 'f');
    }
}
