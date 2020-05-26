#include <neo/buffer_source.hpp>

#include <neo/buffer_range_consumer.hpp>

#include <neo/test_concept.hpp>

#include <catch2/catch.hpp>

NEO_TEST_CONCEPT(neo::buffer_source<neo::proto_buffer_source>);

NEO_TEST_CONCEPT(neo::buffer_source<neo::buffer_range_consumer<neo::const_buffer>>);
NEO_TEST_CONCEPT(neo::buffer_source<neo::buffer_range_consumer<neo::proto_buffer_range>>);
