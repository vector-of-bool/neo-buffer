#include <neo/buffer_algorithm/transform.hpp>

#include <neo/buffer_algorithm/copy.hpp>
#include <neo/string_io.hpp>

#include <neo/test_concept.hpp>

#include <catch2/catch.hpp>

NEO_TEST_CONCEPT(neo::buffer_transform_result<neo::proto_buffer_transform_result>);
NEO_TEST_CONCEPT(neo::buffer_transformer<neo::proto_buffer_transformer>);

TEST_CASE("Copy some data") {
    neo::buffer_copy_transformer c;

    std::string text = "Hello, people!";
    std::string comp;
    comp.resize(50);
    auto res = neo::buffer_transform(c, neo::mutable_buffer(comp), neo::const_buffer(text));
    CHECK(res.bytes_read == text.size());
    CHECK(text == comp.substr(0, text.size()));
}

TEST_CASE("Copy with not enough output room") {
    neo::buffer_copy_transformer c;

    std::string text = "Hello, people!";
    std::string out;
    out.resize(5);
    auto res = neo::buffer_transform(c, neo::mutable_buffer(out), neo::const_buffer(text));
    CHECK(res.bytes_read == out.size());
    CHECK(text.substr(0, 5) == out);
}

std::string pasta
    = "Did you ever hear the tragedy of Darth Plagueis The Wise? I thought not. It’s not a "
      "story the Jedi would tell you. It’s a Sith legend. Darth Plagueis was a Dark Lord of "
      "the Sith, so powerful and so wise he could use the Force to influence the midichlorians "
      "to create life… He had such a knowledge of the dark side that he could even keep the "
      "ones he cared about from dying. The dark side of the Force is a pathway to many "
      "abilities some consider to be unnatural. He became so powerful… the only thing he was "
      "afraid of was losing his power, which eventually, of course, he did. Unfortunately, he "
      "taught his apprentice everything he knew, then his apprentice killed him in his sleep. "
      "Ironic. He could save others from death, but not himself.";

TEST_CASE("Copy into a dynamic buffer") {

    neo::string_dynbuf_io out;

    auto res = neo::buffer_transform(neo::buffer_copy_transformer(), out, neo::const_buffer(pasta));
    CHECK(res.bytes_read == out.available());
    CHECK(out.read_area_view() == pasta);
}

TEST_CASE("Copy multi-part input") {
    auto bufs = {
        neo::const_buffer(
            "Did you ever hear the tragedy of Darth Plagueis The Wise? I thought not. It’s not a "),
        neo::const_buffer("story the Jedi would tell you. It’s a Sith legend. Darth Plagueis was a "
                          "Dark Lord of "),
        neo::const_buffer("the Sith, so powerful and so wise he could use the Force to influence "
                          "the midichlorians "),
        neo::const_buffer("to create life… He had such a knowledge of the dark side that he could "
                          "even keep the "),
        neo::const_buffer(
            "ones he cared about from dying. The dark side of the Force is a pathway to many "),
        neo::const_buffer("abilities some consider to be unnatural. He became so powerful… the "
                          "only thing he was "),
        neo::const_buffer("afraid of was losing his power, which eventually, of course, he did. "
                          "Unfortunately, he "),
        neo::const_buffer("taught his apprentice everything he knew, then his apprentice killed "
                          "him in his sleep. "),
        neo::const_buffer("Ironic. He could save others from death, but not himself."),
    };
    neo::string_dynbuf_io out;
    auto                  res = neo::buffer_transform(neo::buffer_copy_transformer(), out, bufs);
    CHECK(res.bytes_written == out.available());
    CHECK(out.available() == neo::buffer_size(bufs));
    CHECK(out.read_area_view() == pasta);
}
