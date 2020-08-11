#include <neo/buffer_source.hpp>

#include <neo/buffers_consumer.hpp>

#include <neo/test_concept.hpp>

#include <catch2/catch.hpp>

NEO_TEST_CONCEPT(neo::buffer_source<neo::proto_buffer_source>);

NEO_TEST_CONCEPT(neo::buffer_source<neo::buffers_consumer<neo::const_buffer>>);
NEO_TEST_CONCEPT(neo::buffer_source<neo::buffers_consumer<neo::proto_buffer_range>>);
