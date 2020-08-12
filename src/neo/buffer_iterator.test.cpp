#include <neo/buffer_iterator.hpp>

#include <neo/test_concept.hpp>

NEO_TEST_CONCEPT(neo::buffer_iterator<neo::proto_buffer_iterator>);
NEO_TEST_CONCEPT(neo::mutable_buffer_iterator<neo::proto_mutable_buffer_iterator>);
