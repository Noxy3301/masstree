#include <gtest/gtest.h>
#include <cstdint>

#include "../src/include/masstree.h"
#include "gtest_util.h"

TEST(InsertTest, aaa) {
    // 既にデータある場合、WARN_ALREADY_EXISTSを返すテスト
    Masstree masstree;
    GarbageCollector gc;

    Key key({1}, 1);
    Value *value = new Value(1);
    Status status;

    status = masstree.insert_value(key, value, gc);
    EXPECT_EQ(status, Status::OK);

    status = masstree.insert_value(key, value, gc);
    EXPECT_EQ(status, Status::WARN_ALREADY_EXISTS);

    status = masstree.remove_value(key, gc);
    EXPECT_EQ(status, Status::OK);

    status = masstree.remove_value(key, gc);
    EXPECT_EQ(status, Status::WARN_NOT_FOUND);
}




// TEST(PutTest, update_and_gc) {
//     // 既にデータある場合、古いデータをgcに渡すテスト
//     GarbageCollector gc;
//     Key key({1}, 1);
//     Value *value = new Value(1);
//     Node *node = masstree_put(nullptr, key, value, gc).second;
//     node = masstree_put(node, key, new Value(2), gc).second;
//     // valueが上書きされるのでこのアイテムはgcに渡されているはず
//     EXPECT_TRUE(gc.contain(value));
// }