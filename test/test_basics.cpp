#include <gtest/gtest.h>
#include <basetilewriter.h>
#include <fixtures.h>

using namespace std;

TEST(FirstTest, Basic) {
    EXPECT_EQ(1, 2 - 1);
}

TEST(BaseTiles, Write) {
    auto path = fixtures::testOutdir() / "basetiles";
    if (filesystem::exists(path))
        filesystem::remove_all(path);
    auto res = ensureBaseTilesOnSD(path.c_str());
    EXPECT_TRUE(res);
    EXPECT_TRUE(filesystem::exists(path));
    EXPECT_TRUE(filesystem::exists(path / "0/0/0.png"));
}
