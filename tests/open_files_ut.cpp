#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <processing.h>
#include <filesystem>
#include <string>

TEST(OpenFilesTest, ReadsContent) {
    std::vector<std::filesystem::path> paths = {
        "../../testdata/file1.txt",
    "../../testdata/file2.txt"
    };

    auto result = AsDataFlow(paths) | OpenFiles();

    ASSERT_EQ(result.size(), 2u);
    EXPECT_THAT(result[0], testing::HasSubstr("Hello"));
    EXPECT_THAT(result[1], testing::HasSubstr("12345"));
}

TEST(FilterTest, FilterEvenAndTransformSquare) {
  std::vector<int> input = {1, 2, 3, 4, 5};
  auto result = AsDataFlow(input) | Transform([](int x) { return x * x; }) | Filter([](int x) { return x % 2 == 0; }) | AsVector();
  for (auto& i: result) {
    std::cout << i << std::endl;
  }
  ASSERT_THAT(result, testing::ElementsAre(4, 16));
}