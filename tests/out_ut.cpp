#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <processing.h>
#include <sstream>

TEST(OutTest, PrintsEachElement) {
    std::vector<std::string> lines = {"first", "second", "third"};
    std::stringstream ss;


    lines | Out(ss);


    std::string content = ss.str();
    ASSERT_THAT(content, testing::HasSubstr("first\n"));
    ASSERT_THAT(content, testing::HasSubstr("second\n"));
    ASSERT_THAT(content, testing::HasSubstr("third\n"));
}
