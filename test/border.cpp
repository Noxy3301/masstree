#include <gtest/gtest.h>
#include <cstdint>

#include "../src/include/masstree.h"
#include "gtest_util.h"

TEST(BorderNodeTest, searchLinkOrValueWithIndex) {
    // searchLinkOrValueWithIndexがちゃんと動作するかのテスト
    BorderNode *borderNode = new BorderNode;
    // (1)
    borderNode->setKeyLen(0, 2);
    borderNode->setKeySlice(0, 0x1111'1111'1111'1111);
    Value *value = new Value(100);
    borderNode->setLV(0, LinkOrValue(value));
    // (2)
    borderNode->setKeyLen(1, BorderNode::key_len_has_suffix);
    borderNode->setKeySlice(1, 0x1111'1111'1111'1111);
    BigSuffix suffix({0x2222'2222'2222'2222}, 2);
    borderNode->getKeySuffixes().set(1, &suffix);
    borderNode->setLV(1, LinkOrValue(value));
    // (3)
    BorderNode next_layer;
    borderNode->setKeyLen(2, BorderNode::key_len_layer);
    borderNode->setKeySlice(2, 0x2222'2222'2222'2222);
    borderNode->setLV(2, LinkOrValue(&next_layer));
    Permutation permutation = Permutation::fromSorted(3);
    borderNode->setPermutation(permutation);
    //        ┌─ key_len     : 2 (即ち対応するキーは0x1111)
    //      ┌─┴─ linkOrValue : Value(100)
    // ┌─── Key: 0x1111'1111'1111'1111
    // │    │ ┌─ key_len     : has_suffix
    // │    │ ├─ suffix      : {0x2222'2222'2222'2222}, 2
    // │    ├─┴─ linkOrValue : Value(100)
    // ├─── Key: 0x1111'1111'1111'1111
    // │    │ ┌─ key_len     : key_len_layer
    // │    ├─┴─ linkOrValue : Link(next_layer)
    // ├─── Key: 0x2222'2222'2222'2222

    // (1)で挿入したデータを取得する
    Key key1({0x1111'1111'1111'1111}, 2);
    std::tuple<SearchResult, LinkOrValue, size_t> tuple1 = borderNode->searchLinkOrValueWithIndex(key1);
    EXPECT_EQ(std::get<0>(tuple1), SearchResult::VALUE);
    EXPECT_EQ(std::get<1>(tuple1).value, value);
    EXPECT_EQ(std::get<2>(tuple1), 0);
    // (2)で挿入したデータを取得する
    Key key2({0x1111'1111'1111'1111, 0x2222'2222'2222'2222}, 2);
    std::tuple<SearchResult, LinkOrValue, size_t> tuple2 = borderNode->searchLinkOrValueWithIndex(key2);
    EXPECT_EQ(std::get<0>(tuple2), SearchResult::VALUE);
    EXPECT_EQ(std::get<1>(tuple2).value, value);
    EXPECT_EQ(std::get<2>(tuple2), 1);
    // (3)で挿入したデータを取得する
    // このレイヤ(BorderNode)は0x2222'2222'2222'2222までの情報が格納されているので、本来であればLAYERを検知して次のレイヤに移動してRootから探す(その時のスライスは0x3333'3333'3333'3333)
    Key key3({0x2222'2222'2222'2222, 0x3333'3333'3333'3333}, 8);
    std::tuple<SearchResult, LinkOrValue, size_t> tuple3 = borderNode->searchLinkOrValueWithIndex(key3);
    EXPECT_EQ(std::get<0>(tuple3), SearchResult::LAYER);
    EXPECT_EQ(std::get<1>(tuple3).next_layer, &next_layer);
    EXPECT_EQ(std::get<2>(tuple3), 2);
    // 存在しないものは存在しない
    // 0x1111'1111'1111'1111のスライスは存在するけど、対応するキーは0x1111なのでNOTFOUNDとなる
    Key key4({0x1111'1111'1111'1111}, 8);
    std::tuple<SearchResult, LinkOrValue, size_t> tuple4 = borderNode->searchLinkOrValueWithIndex(key4);
    EXPECT_EQ(std::get<0>(tuple4), SearchResult::NOTFOUND);
}

TEST(BorderNodeTest, numberOfKeys) {
    // numberOfKeysが正常にセット/取得できるのかのテスト
    BorderNode *root = new BorderNode;
    root->setKeyLen(0, 4);
    root->setKeySlice(0, 0x0102'0304);
    root->setLV(0, LinkOrValue(new Value(1)));

    root->setKeyLen(1, 8);
    root->setKeySlice(1, 0x0102'0304'0506'0708);
    root->setLV(1, LinkOrValue(new Value(2)));

    root->setIsRoot(true);
    root->setPermutation(Permutation::fromSorted(2));
    // ぶっちゃけsetPermutationだけでいいんだけどね
    EXPECT_EQ(root->getPermutation().getNumKeys(), 2);
}

TEST(BorderNodeTest, sort) {
    // BorderNodeの中身をソートする(要Splitting)テスト
    // Masstreeは高速化の目的から、BorderNodeの中身はソートされておらず、代わりにPermutationがインデックスを保持する
    // splitするときに、昇順に並んでいる必要があるためsortが呼び出される
    // ソートされていない一杯のBorderNodeを作成する
    BorderNode *node = new BorderNode;
    for (size_t i = 0; i <= 7; i++) {
        node->setKeyLen(i, i+1);
        node->setKeySlice(i, 0x2222'2222'2222'2222);
    }
    for (size_t i = 8; i <= 14; i++) {
        node->setKeyLen(i, i-7);
        node->setKeySlice(i, 0x1111'1111'1111'1111);
    }
    node->setPermutation(Permutation::from({8, 9, 10, 11, 12, 13, 14, 0, 1, 2, 3, 4, 5, 6, 7}));
    // ┌─────────────┬───────────────────────┬───────────────────────┐
    // │ Permutation │ Key (sliced)          │ Slice                 │
    // ├─────────────┼───────────────────────┼───────────────────────┤
    // │ 8           │ 0x22                  │ 0x2222'2222'2222'2222 │
    // │ 9           │ 0x2222                │ 0x2222'2222'2222'2222 │
    // │ 10          │ 0x2222'22             │ 0x2222'2222'2222'2222 │
    // │ 11          │ 0x2222'2222           │ 0x2222'2222'2222'2222 │
    // │ 12          │ 0x2222'2222'22        │ 0x2222'2222'2222'2222 │
    // │ 13          │ 0x2222'2222'2222      │ 0x2222'2222'2222'2222 │
    // │ 14          │ 0x2222'2222'2222'22   │ 0x2222'2222'2222'2222 │
    // │ 0           │ 0x11                  │ 0x1111'1111'1111'1111 │
    // │ 1           │ 0x1111                │ 0x1111'1111'1111'1111 │
    // │ 2           │ 0x1111'11             │ 0x1111'1111'1111'1111 │
    // │ 3           │ 0x1111'1111           │ 0x1111'1111'1111'1111 │
    // │ 4           │ 0x1111'1111'11        │ 0x1111'1111'1111'1111 │
    // │ 5           │ 0x1111'1111'1111      │ 0x1111'1111'1111'1111 │
    // │ 6           │ 0x1111'1111'1111'11   │ 0x1111'1111'1111'1111 │
    // │ 7           │ 0x1111'1111'1111'1111 │ 0x1111'1111'1111'1111 │
    // └─────────────┴───────────────────────┴───────────────────────┘
    node->lock();
    node->setSplitting(true);
    node->sort();
    node->unlock();
    EXPECT_EQ(node->getKeyLen(0), 1);
    EXPECT_EQ(node->getKeySlice(0), 0x1111'1111'1111'1111);
    EXPECT_EQ(node->getKeyLen(7), 1);
    EXPECT_EQ(node->getKeySlice(7), 0x2222'2222'2222'2222);
}

TEST(BorderNodeTest, insertPoint) {
    // キーを挿入する際に、insertPointを適切な位置に設定できているかのテスト
    BorderNode node;
    // BorderNodeの作成
    node.setKeyLen(0, 5);
    node.setKeySlice(0, 2);
    node.setKeyLen(1, 7);
    node.setKeySlice(1, 2);
    node.setKeyLen(2, 10);
    node.setKeySlice(2, 3);
    node.setLV(2, LinkOrValue(new Value(1)));

    node.setKeyLen(3, 1);
    node.setKeySlice(3, 1);
    node.setKeyLen(4, 2);
    node.setKeySlice(4, 1);
    node.setKeyLen(5, 8);
    node.setKeySlice(5, 1);
    node.setKeyLen(6, 18);
    node.setKeySlice(6, 1);
    node.getKeySuffixes().set(6, new BigSuffix({2, 3}, 4));
    node.setLV(6, LinkOrValue(new Value(1)));
    node.setPermutation(Permutation::from({3, 4, 5, 0, 1}));
    node.setIsRoot(true);
    // ┌──────────────────┬───────────┬─────────┬───────────┐
    // │ permutationIndex │ trueIndex │ key_len │ key_slice │
    // ├──────────────────┼───────────┼─────────┼───────────┤
    // │ 0                │ 3         │ 5       │ 2         │
    // │ 1                │ 4         │ 7       │ 2         │
    // │ 2                │ 5         │ 10      │ 3         │ Value(1)
    // │ 3                │ 0         │ 1       │ 1         │
    // │ 4                │ 1         │ 2       │ 1         │
    // │ 5                │           │ 8       │ 1         │
    // │ 6                │           │ 18      │ 1         │ BigSuffix({2, 3}, 4)
    // │ 7                │           │         │           │
    GarbageCollector gc;
    Key key1({3}, 1);
    std::pair<size_t, bool> pair = node.insertPoint();
    // permutationIndex=2のpermutation slotはremovedされている(10 <= len && len <= 18)から、ここを使うことになる
    EXPECT_EQ(pair.first, 2);
    EXPECT_EQ(pair.second, true);
    masstree_put(&node, key1, new Value(1), gc);
    Key key2({1, 2, 3}, 4);
    pair = node.insertPoint();
    // permutationIndex=6のpermutation slotはremovedされている(10 <= len && len <= 18)から、ここを使うことになる
    EXPECT_EQ(pair.first, 6);
    EXPECT_EQ(pair.second, true);
    masstree_put(&node, key2, new Value(1), gc);
    Key key3({1}, 3);
    pair = node.insertPoint();
    // permutationIndex=7のpermutation slotが空なのでここを使う
    EXPECT_EQ(pair.first, 7);
    EXPECT_EQ(pair.second, false);
}