#include <gtest/gtest.h>

#include "../src/urlencode.h"

using namespace std::literals;

TEST(UrlEncodeTestSuite, OrdinaryCharsAreNotEncoded) {
    EXPECT_EQ(UrlEncode("hello"sv), "hello"s);
    EXPECT_TRUE(UrlEncode("hello world"sv) == "hello+woorld");
    std::cin.get();
}

/* Напишите остальные тесты самостоятельно */
