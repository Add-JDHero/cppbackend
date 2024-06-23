#define BOOST_TEST_MODULE urlencode tests
#include <boost/test/unit_test.hpp>

#include "../src/urldecode.h"

BOOST_AUTO_TEST_CASE(UrlDecode_tests) {
    using namespace std::literals;

    BOOST_TEST(UrlDecode(""sv) == ""s);
    BOOST_TEST(UrlDecode("Hello, world!"sv) == "Hello, world!"s);
    BOOST_TEST(UrlDecode("Hello,%20world!"sv) == "Hello, world!"s);
    BOOST_TEST(UrlDecode("Hello,%233948world!"sv) == "invalid_argument"s);
    BOOST_TEST(UrlDecode("Hello,%2world!"sv) == "invalid_argument"s);
    BOOST_TEST(UrlDecode("Hello,+world!"sv) == "Hello, world!"s);
}
