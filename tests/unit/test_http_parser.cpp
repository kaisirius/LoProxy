#include <catch2/catch_test_macros.hpp>

#include <http/HttpParser.hpp>
#include <http/ParseResult.hpp>

TEST_CASE("Full HTTP request in one feed becomes COMPLETE",
          "[http_parser][full_request]") {

    HttpParser parser;

    const std::string request =
        "GET /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "\r\n";

    ParseResult result = parser.parse(request, 0);

    REQUIRE(result.status == COMPLETE);

    ParsedRequest parsedRequest = parser.getParsedReqObj();

    REQUIRE(parsedRequest.method == "GET");
    REQUIRE(parsedRequest.uri == "/hello");
    REQUIRE(parsedRequest.headers.at("Host") == "localhost");
}


TEST_CASE("HTTP request split across 3 feeds becomes COMPLETE after 3rd feed",
          "[http_parser][streaming]") {

    HttpParser parser;

    const std::string feed1 =
        "GET /hello HTTP/1.1\r\n"
        "Hos";

    const std::string feed2 =
        "t: localhost\r\n";

    const std::string feed3 =
        "\r\n";

    ParseResult result1 = parser.parse(feed1, 0);

    REQUIRE(result1.status == INCOMPLETE);

    ParseResult result2 = parser.parse(feed2, 0);

    REQUIRE(result2.status == INCOMPLETE);

    ParseResult result3 = parser.parse(feed3, 0);

    REQUIRE(result3.status == COMPLETE);

    ParsedRequest parsedRequest = parser.getParsedReqObj();

    REQUIRE(parsedRequest.method == "GET");
    REQUIRE(parsedRequest.uri == "/hello");
    REQUIRE(parsedRequest.headers.at("Host") == "localhost");
}


TEST_CASE("Malformed HTTP request line returns ERROR",
          "[http_parser][malformed]") {

    HttpParser parser;

    const std::string request =
        "INVALID /hello HTTP/1.1\r\n"
        "\r\n";

    ParseResult result = parser.parse(request, 0);

    REQUIRE(result.status == ERROR);
}


TEST_CASE("Content-Length body is correctly accumulated",
          "[http_parser][body]") {

    HttpParser parser;

    const std::string request =
        "POST /hello HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "hello world";

    ParseResult result = parser.parse(request, 0);

    REQUIRE(result.status == COMPLETE);

    ParsedRequest parsedRequest = parser.getParsedReqObj();

    REQUIRE(parsedRequest.method == "POST");
    REQUIRE(parsedRequest.uri == "/hello");
    REQUIRE(parsedRequest.headers.at("Content-Length") == "11");
    REQUIRE(parsedRequest.body == "hello world");
}