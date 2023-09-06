#include <gtest/gtest.h>

#include "../src/include/masstree.h"

//                                                                 ┌───── splitting
//                                                                 │
//                                                                 │   ┌───── inserting
//                                                                 │   │
//                   ┌───── v_split                                │   │   ┌───── locked
//                   │                                             │   │   │
//  version : [ 00 │ 0000'0000 │ 0000'0000'0000'0000 │ 0 │ 0 │ 0 │ 0 │ 0 │ 0 ]
//              │                │                     │   │   │
//              └───── unused    └───── v_insert       │   │   └───── deleted
//                                                     │   │
//                                                     │   └───── is_root
//                                                     │
//                                                     └───── is_border

TEST(VersionTest, bit) {
    // uint64_tのVersion.bodyが機能するかの確認
    uint64_t num = 0b0000'0000'0000'0000'0000'0000'0000'0001;
    Version version;
    version.body = num;
    EXPECT_TRUE(version.locked);
}

TEST(VersionTest, has_locked) {
    Version version{};
    Version before = version;
    // before  : [ 00 │ 0000'0000 │ 0000'0000'0000'0000 │ 0 │ 0 │ 0 │ 0 │ 0 │ 0 ]
    version.locked = true;
    version.inserting = true;
    // version : [ 00 │ 0000'0000 │ 0000'0000'0000'0000 │ 0 │ 0 │ 0 │ 0 │ 1 │ 1 ]
    EXPECT_TRUE((before ^ version) > 0);
    version.locked = false;
    version.inserting = false;
    version.v_insert++;
    // version : [ 00 │ 0000'0000 │ 0000'0000'0000'0001 │ 0 │ 0 │ 0 │ 0 │ 0 │ 0 ]
    EXPECT_TRUE((before ^ version) > 0);
}