#include <neo/trivial_range.hpp>

#include <catch2/catch.hpp>

#include <array>

TEST_CASE("Simple array") {
    using Array = std::array<int, 4>;
    static_assert(neo::same_as<neo::data_type_t<Array>, int>);
    static_assert(neo::same_as<neo::data_type_t<Array&>, int>);
    static_assert(neo::same_as<neo::data_type_t<Array&&>, int>);
    static_assert(neo::same_as<neo::data_type_t<const Array>, int>);
    static_assert(neo::same_as<neo::data_type_t<const Array&>, int>);
    static_assert(neo::same_as<neo::data_type_t<const Array&&>, int>);

    static_assert(neo::same_as<neo::data_pointer_t<Array>, int*>);
    static_assert(neo::same_as<neo::data_pointer_t<Array&>, int*>);
    static_assert(neo::same_as<neo::data_pointer_t<Array&&>, int*>);
    static_assert(neo::same_as<neo::data_pointer_t<const Array>, const int*>);
    static_assert(neo::same_as<neo::data_pointer_t<const Array&>, const int*>);
    static_assert(neo::same_as<neo::data_pointer_t<const Array&&>, const int*>);

    static_assert(neo::same_as<neo::const_data_pointer_t<Array>, const int*>);
    static_assert(neo::same_as<neo::const_data_pointer_t<Array&>, const int*>);
    static_assert(neo::same_as<neo::const_data_pointer_t<Array&&>, const int*>);
    static_assert(neo::same_as<neo::const_data_pointer_t<const Array>, const int*>);
    static_assert(neo::same_as<neo::const_data_pointer_t<const Array&>, const int*>);
    static_assert(neo::same_as<neo::const_data_pointer_t<const Array&&>, const int*>);

    static_assert(neo::trivial_range<neo::proto_trivial_range>);
    static_assert(neo::mutable_trivial_range<neo::proto_mutable_trivial_range>);
}
