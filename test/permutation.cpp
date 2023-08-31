#include <gtest/gtest.h>

#include "../src/include/masstree.h"

TEST(PermutationTest, getNumKeys){
  Permutation p{};
  p.body = 0b0000'0001'0010'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0011;
  auto num = p.getNumKeys();
  EXPECT_EQ(num, 3);
}

TEST(PermutationTest, setNumKeys){
  Permutation p{};
  p.body = 0b1011'0001'0010'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0011;
  p.setNumKeys(4);
  EXPECT_EQ(p.getNumKeys(), 4);
  EXPECT_EQ(p.getKeyIndex(1), 1);
}
