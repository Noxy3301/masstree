#include <gtest/gtest.h>
#include <cstdint>

#include "../src/include/masstree.h"
#include "gtest_util.h"

TEST(BigSuffixTest, BigSuffix_from) {
    // suffixのコピー(BigSuffix::from)のテスト
    BigSuffix *suffix;
    Key key({0x1111'1111'1111'1111, 0x2222'2222'2222'2222, 0x3333'3333'3333'3333, 0x0A0B'0000'0000'0000}, 2);
    suffix = BigSuffix::from(key, 1);
    // {0x2222'2222'2222'2222, 0x3333'3333'3333'3333, 0x0A0B'0000'0000'0000}
    EXPECT_EQ(suffix->getCurrentSlice().slice, 0x2222'2222'2222'2222);
    suffix->next(); // next()はslicesの先頭をpopする
    // {0x3333'3333'3333'3333, 0x0A0B'0000'0000'0000}
    EXPECT_EQ(suffix->getCurrentSlice().slice, 0x3333'3333'3333'3333);
    suffix->next();
    // {0x0A0B'0000'0000'0000}
    EXPECT_EQ(suffix->getCurrentSlice().slice, 0x0A0B'0000'0000'0000);
    EXPECT_EQ(suffix->getCurrentSlice().size, 2);
}

TEST(BigSuffixTest, BigSuffix_isSame) {
    // 特定のSuffixが同じかどうかのテスト
    BigSuffix suffix1 = BigSuffix({0x2222'2222'2222'2222, 0x1111'1111'1111'1111, 0x0A0B'0000'0000'0000}, 2);
    Key key1({0x8888'8888'8888'8888, 0x9999'9999'9999'9999, 0x2222'2222'2222'2222, 0x1111'1111'1111'1111, 0x0A0B'0000'0000'0000}, 2);
    EXPECT_TRUE(suffix1.isSame(key1, 2)); // Keyの2以降のスライス(0x2222'~)が一致する
    BigSuffix suffix2 = BigSuffix({0x2222'2222'2222'2222, 0x1111'1111'1111'1111}, 2);
    Key key2({0x2222'2222'2222'2222, 0x1111'1111'1111'1111}, 4);
    // sliceの内容は同じだけど、lastSliceSizeが違うからこれらのスライスは別物として認識される
    EXPECT_FALSE(suffix2.isSame(key2, 0));
}

TEST(BigSuffixTest, BigSuffix_insertTop) {
    // BigSuffixの先頭に新しいスライスを追加するテスト
    BigSuffix suffix = BigSuffix({0x1111'1111'1111'1111, 0x2222'2222'2222'2222}, 8);
    suffix.insertTop(0x3333'3333'3333'3333);
    // {0x3333'3333'3333'3333, 0x1111'1111'1111'1111, 0x2222'2222'2222'2222}
    EXPECT_EQ(suffix.getCurrentSlice().slice, 0x3333'3333'3333'3333);
    suffix.next();
    // {0x1111'1111'1111'1111, 0x2222'2222'2222'2222}
    EXPECT_EQ(suffix.getCurrentSlice().slice, 0x1111'1111'1111'1111);
    suffix.next();
    // {0x2222'2222'2222'2222}
    EXPECT_EQ(suffix.getCurrentSlice().slice, 0x2222'2222'2222'2222);
}