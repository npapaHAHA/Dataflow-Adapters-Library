#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <processing.h>


TEST(DirTest, EnumerateNonRecursive) {
    Dir dir{"/etc", false};


    auto result = dir();


    EXPECT_FALSE(result.empty());
}

