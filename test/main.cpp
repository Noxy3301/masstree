#include <gtest/gtest.h>

#include "../src/include/masstree.h"

TEST(sampleTest, test) {
    InteriorNode n{};
    n.setNumKeys(3);
    n.decNumKeys();
    EXPECT_TRUE(n.getNumKeys() == 2);
}


int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}