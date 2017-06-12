#include <boost/asio/buffer.hpp>
#include <vector>

#include "bredis/MarkerHelpers.hpp"
#include "bredis/Protocol.hpp"
#include "catch.hpp"

namespace r = bredis;
namespace asio = boost::asio;

using Buffer = asio::const_buffers_1;
using Iterator = boost::asio::buffers_iterator<Buffer, char>;

TEST_CASE("simple string", "[protocol]") {
    std::string ok = "+OK\r\n";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    auto positive_parse_result =
        boost::get<r::positive_parse_result_t<Iterator>>(parsed_result);
    REQUIRE(positive_parse_result.consumed == ok.size());
    REQUIRE(boost::apply_visitor(r::marker_helpers::equality<Iterator>("OK"),
                                 positive_parse_result.result));
};

TEST_CASE("empty string", "[protocol]") {
    std::string ok = "";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    r::not_enough_data_t *r = boost::get<r::not_enough_data_t>(&parsed_result);
    REQUIRE(r != nullptr);
};

TEST_CASE("non-finished ", "[protocol]") {
    std::string ok = "+OK";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    r::not_enough_data_t *r = boost::get<r::not_enough_data_t>(&parsed_result);
    REQUIRE(r != nullptr);
};

TEST_CASE("wrong start marker", "[protocol]") {
    std::string ok = "!OK";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    r::protocol_error_t *r = boost::get<r::protocol_error_t>(&parsed_result);
    REQUIRE(r->what == "wrong introduction");
};

TEST_CASE("number-like", "[protocol]") {
    std::string ok = ":-55abc\r\n";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    auto positive_parse_result =
        boost::get<r::positive_parse_result_t<Iterator>>(parsed_result);
    REQUIRE(positive_parse_result.consumed);
    REQUIRE(positive_parse_result.consumed == ok.size());
    REQUIRE(boost::get<r::markers::int_t<Iterator>>(
                &positive_parse_result.result) != nullptr);
    REQUIRE(
        boost::apply_visitor(r::marker_helpers::equality<Iterator>("-55abc"),
                             positive_parse_result.result));
};

TEST_CASE("simple error", "[protocol]") {
    std::string ok = "-Ooops\r\n";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    auto positive_parse_result =
        boost::get<r::positive_parse_result_t<Iterator>>(parsed_result);
    REQUIRE(positive_parse_result.consumed);
    REQUIRE(positive_parse_result.consumed == ok.size());
    REQUIRE(boost::get<r::markers::error_t<Iterator>>(
                &positive_parse_result.result) != nullptr);
    REQUIRE(boost::apply_visitor(r::marker_helpers::equality<Iterator>("Ooops"),
                                 positive_parse_result.result));
};

TEST_CASE("nil", "[protocol]") {
    std::string ok = "$-1\r\n";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    auto positive_parse_result =
        boost::get<r::positive_parse_result_t<Iterator>>(parsed_result);
    REQUIRE(positive_parse_result.consumed);
    REQUIRE(positive_parse_result.consumed == ok.size());

    auto *nil =
        boost::get<r::markers::nil_t<Iterator>>(&positive_parse_result.result);
    REQUIRE(nil != nullptr);
    REQUIRE(boost::apply_visitor(r::marker_helpers::equality<Iterator>("-1"),
                                 positive_parse_result.result));
};

TEST_CASE("malformed bulk string", "[protocol]") {
    std::string ok = "$-5\r\nsome\r\n";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    r::protocol_error_t *r = boost::get<r::protocol_error_t>(&parsed_result);
    REQUIRE(r->what == "Value -5 in unacceptable for bulk strings");
};

TEST_CASE("some bulk string", "[protocol]") {
    std::string ok = "$4\r\nsome\r\n";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    auto positive_parse_result =
        boost::get<r::positive_parse_result_t<Iterator>>(parsed_result);
    REQUIRE(positive_parse_result.consumed);
    REQUIRE(positive_parse_result.consumed == ok.size());
    REQUIRE(boost::get<r::markers::string_t<Iterator>>(
                &positive_parse_result.result) != nullptr);
    REQUIRE(boost::apply_visitor(r::marker_helpers::equality<Iterator>("some"),
                                 positive_parse_result.result));
};

TEST_CASE("empty bulk string", "[protocol]") {
    std::string ok = "$0\r\n\r\n";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    auto positive_parse_result =
        boost::get<r::positive_parse_result_t<Iterator>>(parsed_result);
    REQUIRE(positive_parse_result.consumed);
    REQUIRE(positive_parse_result.consumed == ok.size());
    REQUIRE(boost::get<r::markers::string_t<Iterator>>(
                &positive_parse_result.result) != nullptr);
    REQUIRE(boost::apply_visitor(r::marker_helpers::equality<Iterator>(""),
                                 positive_parse_result.result));
};

TEST_CASE("patrial bulk string(1)", "[protocol]") {
    std::string ok = "$10\r\nsome\r\n";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    REQUIRE(boost::get<r::not_enough_data_t>(&parsed_result) != nullptr);
};

TEST_CASE("patrial bulk string(2)", "[protocol]") {
    std::string ok = "$4\r\nsome\r";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    REQUIRE(boost::get<r::not_enough_data_t>(&parsed_result) != nullptr);
};

TEST_CASE("malformed bulk string(2)", "[protocol]") {
    std::string ok = "$1\r\nsome\r\n";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    r::protocol_error_t *r = boost::get<r::protocol_error_t>(&parsed_result);
    REQUIRE(r->what == "Terminator not found for bulk string");
};

TEST_CASE("empty array", "[protocol]") {
    std::string ok = "*0\r\n";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    auto positive_parse_result =
        boost::get<r::positive_parse_result_t<Iterator>>(parsed_result);
    REQUIRE(positive_parse_result.consumed);
    REQUIRE(positive_parse_result.consumed == ok.size());
    auto *array_holder = boost::get<r::markers::array_holder_t<Iterator>>(
        &positive_parse_result.result);
    REQUIRE(array_holder != nullptr);
    REQUIRE(array_holder->elements.size() == 0);
};

TEST_CASE("null array", "[protocol]") {
    std::string ok = "*-1\r\n";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    auto positive_parse_result =
        boost::get<r::positive_parse_result_t<Iterator>>(parsed_result);
    REQUIRE(positive_parse_result.consumed);
    REQUIRE(positive_parse_result.consumed == ok.size());
    auto *nil =
        boost::get<r::markers::nil_t<Iterator>>(&positive_parse_result.result);
    REQUIRE(nil != nullptr);
    REQUIRE(boost::apply_visitor(r::marker_helpers::equality<Iterator>("-1"),
                                 positive_parse_result.result));
};

TEST_CASE("malformed array", "[protocol]") {
    std::string ok = "*-4\r\nsome\r\n";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    r::protocol_error_t *r = boost::get<r::protocol_error_t>(&parsed_result);
    REQUIRE(r->what == "Value -4 in unacceptable for arrays");
};

TEST_CASE("patrial array", "[protocol]") {
    std::string ok = "*1\r\n";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    REQUIRE(boost::get<r::not_enough_data_t>(&parsed_result) != nullptr);
};

TEST_CASE("array: string, int, nil", "[protocol]") {
    std::string ok = "*3\r\n$4\r\nsome\r\n:5\r\n$-1\r\n";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    auto positive_parse_result =
        boost::get<r::positive_parse_result_t<Iterator>>(parsed_result);
    REQUIRE(positive_parse_result.consumed);
    REQUIRE(positive_parse_result.consumed == ok.size());

    auto *array = boost::get<r::markers::array_holder_t<Iterator>>(
        &positive_parse_result.result);
    REQUIRE(array != nullptr);
    REQUIRE(array->elements.size() == 3);

    REQUIRE(boost::apply_visitor(r::marker_helpers::equality<Iterator>("some"),
                                 array->elements[0]));
    REQUIRE(boost::apply_visitor(r::marker_helpers::equality<Iterator>("5"),
                                 array->elements[1]));
    REQUIRE(boost::get<r::markers::int_t<Iterator>>(&array->elements[1]) !=
            nullptr);
    REQUIRE(boost::get<r::markers::nil_t<Iterator>>(&array->elements[2]) !=
            nullptr);
};

TEST_CASE("array of arrays: [int, int, int,], [str,err] ", "[protocol]") {
    std::string ok = "*2\r\n*3\r\n:1\r\n:2\r\n:3\r\n*2\r\n+Foo\r\n-Bar\r\n";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    auto positive_parse_result =
        boost::get<r::positive_parse_result_t<Iterator>>(parsed_result);
    REQUIRE(positive_parse_result.consumed);
    REQUIRE(positive_parse_result.consumed == ok.size());

    auto *array = boost::get<r::markers::array_holder_t<Iterator>>(
        &positive_parse_result.result);
    REQUIRE(array != nullptr);
    REQUIRE(array->elements.size() == 2);

    auto *a1 =
        boost::get<r::markers::array_holder_t<Iterator>>(&array->elements[0]);
    REQUIRE(a1->elements.size() == 3);
    REQUIRE(boost::apply_visitor(r::marker_helpers::equality<Iterator>("1"),
                                 a1->elements[0]));
    REQUIRE(boost::apply_visitor(r::marker_helpers::equality<Iterator>("2"),
                                 a1->elements[1]));
    REQUIRE(boost::apply_visitor(r::marker_helpers::equality<Iterator>("3"),
                                 a1->elements[2]));
    REQUIRE(boost::get<r::markers::int_t<Iterator>>(&a1->elements[0]) !=
            nullptr);
    REQUIRE(boost::get<r::markers::int_t<Iterator>>(&a1->elements[1]) !=
            nullptr);
    REQUIRE(boost::get<r::markers::int_t<Iterator>>(&a1->elements[2]) !=
            nullptr);

    auto *a2 =
        boost::get<r::markers::array_holder_t<Iterator>>(&array->elements[1]);
    REQUIRE(a2->elements.size() == 2);
    REQUIRE(boost::apply_visitor(r::marker_helpers::equality<Iterator>("Foo"),
                                 a2->elements[0]));
    REQUIRE(boost::apply_visitor(r::marker_helpers::equality<Iterator>("Bar"),
                                 a2->elements[1]));
    REQUIRE(boost::get<r::markers::string_t<Iterator>>(&a2->elements[0]) !=
            nullptr);
    REQUIRE(boost::get<r::markers::error_t<Iterator>>(&a2->elements[1]) !=
            nullptr);
};

TEST_CASE("right consumption", "[protocol]") {
    std::string ok =
        "*3\r\n$7\r\nmessage\r\n$13\r\nsome-channel1\r\n$10\r\nmessage-a1\r\n";
    ok = ok + ok;
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    auto positive_parse_result =
        boost::get<r::positive_parse_result_t<Iterator>>(parsed_result);
    REQUIRE(positive_parse_result.consumed);
    REQUIRE(positive_parse_result.consumed == ok.size() / 2);
}

TEST_CASE("overfilled buffer", "[protocol]") {
    std::string ok = "*3\r\n$7\r\nmessage\r\n$13\r\nsome-channel1\r\n$"
                     "10\r\nmessage-a1\r\n*3\r\n$7\r\nmessage\r\n$13\r\nsome-"
                     "channel1\r\n$10\r\nmessage-a2\r\n*3\r\n$7\r\nmessage\r\n$"
                     "13\r\nsome-channel2\r\n$4\r\nlast\r\n";
    Buffer buff(ok.c_str(), ok.size());
    auto from = Iterator::begin(buff), to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(from, to);
    auto positive_parse_result =
        boost::get<r::positive_parse_result_t<Iterator>>(parsed_result);
    REQUIRE(positive_parse_result.consumed);
    REQUIRE(positive_parse_result.consumed == 54);

    auto &a1 = boost::get<r::markers::array_holder_t<Iterator>>(
        positive_parse_result.result);
    REQUIRE(a1.elements.size() == 3);
    REQUIRE(boost::apply_visitor(
        r::marker_helpers::equality<Iterator>("message"), a1.elements[0]));
    REQUIRE(boost::apply_visitor(
        r::marker_helpers::equality<Iterator>("some-channel1"),
        a1.elements[1]));
    REQUIRE(boost::apply_visitor(
        r::marker_helpers::equality<Iterator>("message-a1"), a1.elements[2]));
    REQUIRE(boost::get<r::markers::string_t<Iterator>>(&a1.elements[0]) !=
            nullptr);
    REQUIRE(boost::get<r::markers::string_t<Iterator>>(&a1.elements[1]) !=
            nullptr);
    REQUIRE(boost::get<r::markers::string_t<Iterator>>(&a1.elements[2]) !=
            nullptr);

    buff = Buffer(ok.c_str() + 54, ok.size() - 54);
    from = Iterator::begin(buff), to = Iterator::end(buff);
    parsed_result = r::Protocol::parse(from, to);
    positive_parse_result =
        boost::get<r::positive_parse_result_t<Iterator>>(parsed_result);
    REQUIRE(positive_parse_result.consumed);
    REQUIRE(positive_parse_result.consumed == 54);

    auto &a2 = boost::get<r::markers::array_holder_t<Iterator>>(
        positive_parse_result.result);
    REQUIRE(a2.elements.size() == 3);
    REQUIRE(boost::apply_visitor(
        r::marker_helpers::equality<Iterator>("message"), a2.elements[0]));
    REQUIRE(boost::apply_visitor(
        r::marker_helpers::equality<Iterator>("some-channel1"),
        a2.elements[1]));
    REQUIRE(boost::apply_visitor(
        r::marker_helpers::equality<Iterator>("message-a2"), a2.elements[2]));
    REQUIRE(boost::get<r::markers::string_t<Iterator>>(&a2.elements[0]) !=
            nullptr);
    REQUIRE(boost::get<r::markers::string_t<Iterator>>(&a2.elements[1]) !=
            nullptr);
    REQUIRE(boost::get<r::markers::string_t<Iterator>>(&a2.elements[2]) !=
            nullptr);

    buff = Buffer(ok.c_str() + 54 * 2, ok.size() - 54 * 2);
    from = Iterator::begin(buff), to = Iterator::end(buff);
    parsed_result = r::Protocol::parse(from, to);
    positive_parse_result =
        boost::get<r::positive_parse_result_t<Iterator>>(parsed_result);
    REQUIRE(positive_parse_result.consumed);
    REQUIRE(positive_parse_result.consumed == 47);

    auto &a3 = boost::get<r::markers::array_holder_t<Iterator>>(
        positive_parse_result.result);
    REQUIRE(a3.elements.size() == 3);
    REQUIRE(boost::apply_visitor(
        r::marker_helpers::equality<Iterator>("message"), a3.elements[0]));
    REQUIRE(boost::apply_visitor(
        r::marker_helpers::equality<Iterator>("some-channel2"),
        a3.elements[1]));
    REQUIRE(boost::apply_visitor(r::marker_helpers::equality<Iterator>("last"),
                                 a3.elements[2]));
    REQUIRE(boost::get<r::markers::string_t<Iterator>>(&a3.elements[0]) !=
            nullptr);
    REQUIRE(boost::get<r::markers::string_t<Iterator>>(&a3.elements[1]) !=
            nullptr);
    REQUIRE(boost::get<r::markers::string_t<Iterator>>(&a3.elements[2]) !=
            nullptr);
}

TEST_CASE("serialize", "[protocol]") {
    std::stringstream buff;
    r::single_command_t cmd("LLEN", "fmm.cheap-travles2");
    r::Protocol::serialize(buff, cmd);
    std::string expected("*2\r\n$4\r\nLLEN\r\n$18\r\nfmm.cheap-travles2\r\n");
    REQUIRE(buff.str() == expected);
};
