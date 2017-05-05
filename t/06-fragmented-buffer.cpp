#include <boost/asio/buffer.hpp>
#include <boost/asio/streambuf.hpp>
#include <sstream>
#include <vector>

#include "bredis/MarkerHelpers.hpp"
#include "bredis/Protocol.hpp"

#include "catch.hpp"

namespace r = bredis;
namespace asio = boost::asio;

TEST_CASE("right consumption", "[protocol]") {
    using Buffer = std::vector<asio::const_buffers_1>;
    using Iterator = boost::asio::buffers_iterator<Buffer, char>;
    using parsed_result_t = r::parse_result_t<Iterator>;

    std::string full_message =
        "*3\r\n$7\r\nmessage\r\n$13\r\nsome-channel1\r\n$10\r\nmessage-a1\r\n";
    auto from = full_message.cbegin();
    auto to = full_message.cbegin() - 1;
    Buffer buff;
    for (auto i = 0; i < full_message.size(); i++) {
        asio::const_buffers_1 v(full_message.c_str() + i, 1);
        buff.push_back(v);
    }
    auto b_from = Iterator::begin(buff), b_to = Iterator::end(buff);
    auto parsed_result = r::Protocol::parse(b_from, b_to);
    auto positive_parse_result =
        boost::get<r::positive_parse_result_t<Iterator>>(parsed_result);
    REQUIRE(positive_parse_result.consumed);
    REQUIRE(positive_parse_result.consumed == full_message.size());

    auto *array = boost::get<r::markers::array_holder_t<Iterator>>(
        &positive_parse_result.result);
    REQUIRE(array != nullptr);
    REQUIRE(array->elements.size() == 3);

    REQUIRE(boost::apply_visitor(
        r::marker_helpers::equality<Iterator>("message"), array->elements[0]));
    REQUIRE(boost::apply_visitor(
        r::marker_helpers::equality<Iterator>("some-channel1"),
        array->elements[1]));
    REQUIRE(boost::apply_visitor(
        r::marker_helpers::equality<Iterator>("message-a1"),
        array->elements[2]));
    REQUIRE(boost::get<r::markers::string_t<Iterator>>(&array->elements[0]) !=
            nullptr);
    REQUIRE(boost::get<r::markers::string_t<Iterator>>(&array->elements[1]) !=
            nullptr);
    REQUIRE(boost::get<r::markers::string_t<Iterator>>(&array->elements[2]) !=
            nullptr);
}

TEST_CASE("using strembuff", "[protocol]") {
    using Buffer = boost::asio::streambuf;
    using Iterator = typename r::to_iterator<Buffer>::iterator_t;
    using parsed_result_t = r::parse_result_t<Iterator>;

    std::string ok = "+OK\r\n";
    Buffer buff;
    std::ostream os(&buff);
    os.write(ok.c_str(), ok.size());

    auto const_buff = buff.data();
    auto from = Iterator::begin(const_buff), to = Iterator::end(const_buff);
    auto parsed_result = r::Protocol::parse(from, to);

    auto positive_parse_result =
        boost::get<r::positive_parse_result_t<Iterator>>(parsed_result);
    REQUIRE(positive_parse_result.consumed == ok.size());
    REQUIRE(boost::apply_visitor(r::marker_helpers::equality<Iterator>("OK"),
                                 positive_parse_result.result));
}
