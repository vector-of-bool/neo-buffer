#include <neo/data_container_concepts.hpp>

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
}
