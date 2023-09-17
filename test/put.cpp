#include <gtest/gtest.h>
#include <cstdint>

#include "../src/include/masstree.h"
#include "gtest_util.h"

TEST(PutTest, start_new_tree) {
    // start_new_treeが正常に動作するかのテスト
    // start_new_treeは空のBorderNodeを作成してそこにキーを挿入する
    Key key1({0x1111'1111'1111'1111}, 8);
    Value value(1);
    BorderNode *root1 = start_new_tree(key1, &value);
    EXPECT_EQ(root1->getKeyLen(0), 8);
    EXPECT_EQ(root1->getKeySlice(0), 0x1111'1111'1111'1111);

    Key key2({0x1111'1111'1111'1111, 0x0A0B'0000'0000'0000}, 3);
    BorderNode *root2 = start_new_tree(key2, &value);
    EXPECT_EQ(root2->getKeyLen(0), BorderNode::key_len_has_suffix);
    EXPECT_EQ(root2->getKeySlice(0), 0x1111'1111'1111'1111);
    // root2は0番目の要素のsuffixとして{0x0A0B'00}を持っているからremainLength()は3になる
    EXPECT_EQ(root2->getKeySuffixes().get(0)->remainLength(), 3);
}

TEST(PutTest, check_break_invariant) {
    // check_break_invariantが正常に動作するかのテスト
    // keyを挿入したいけど、該当するスライスがあるところはkey_len_has_suffixのマークがついているので挿入できない(なので新しいレイヤを作成する)
    BorderNode borderNode;
    Permutation permutation;
    Key key({0x2222'2222'2222'2222, 0x0A0B'0000'0000'0000}, 2);
    borderNode.setKeyLen(1, BorderNode::key_len_has_suffix);
    borderNode.setKeySlice(1, 0x2222'2222'2222'2222);
    permutation.setKeyIndex(0, 1);
    permutation.setNumKeys(1);
    borderNode.setPermutation(permutation);
    borderNode.lock();
    EXPECT_EQ(check_break_invariant(&borderNode, key), 1);
}

TEST(PutTest, handle_break_invariant) {
    // handle_break_invariantが正常に動作するかのテスト
    // そのノードにデータをそれ以上入れることができないので、新しいレイヤを作成してそこに入れる
    BorderNode *borderNode = new BorderNode;
    Value *value1 = new Value(100);
    Value *value2 = new Value(200);
    borderNode->setKeyLen(0, 8);
    borderNode->setKeySlice(0, 0x8888'8888'8888'8888);
    borderNode->setLV(0, LinkOrValue(value1));
    borderNode->setKeyLen(1, BorderNode::key_len_has_suffix);
    borderNode->setKeySlice(1, 0x8888'8888'8888'8888);
    BigSuffix *suffix = new BigSuffix({0x1111'1111'1111'1111, 0x2222'2222'2222'2222, 0x3333'3333'3333'3333, 0x0A0B'0000'0000'0000}, 2);
    borderNode->getKeySuffixes().set(1, suffix);
    borderNode->setLV(1, LinkOrValue(value2));
    borderNode->setPermutation(Permutation::fromSorted(2));
    Key key({0x8888'8888'8888'8888, 0x1111'1111'1111'1111, 0x2222'2222'2222'2222, 0x0C0D'0000'0000'0000}, 2);
    GarbageCollector gc;
    borderNode->lock();

    // === before handle_break_invariant ===
    //        ┌─ key_len     : 8 (即ち対応するキーは0x8888'8888'8888'8888)
    //      ┌─┴─ linkOrValue : Value(100)
    // ┌─── Key: 0x8888'8888'8888'8888
    // │    │ ┌─ key_len     : has_suffix
    // │    │ ├─ suffix      : {0x1111'1111'1111'1111, 0x2222'2222'2222'2222, 0x3333'3333'3333'3333, 0x0A0B'0000'0000'0000}, 2
    // │    ├─┴─ linkOrValue : Value(200)
    // ├─── Key: 0x8888'8888'8888'8888
    
    handle_break_invariant(borderNode, key, 1, gc); // keyはborderNodeの1番目のスロットに挿入されるべきだけど、既にhas_suffixのデータが入っているので新しいレイヤを作成する
    
    // === after handle_break_invariant ===
    //        ┌─ key_len     : 8 (即ち対応するキーは0x8888'8888'8888'8888)
    //      ┌─┴─ linkOrValue : Value(100)
    // ┌─── Key: 0x8888'8888'8888'8888
    // │    │ ┌─ key_len     : layer
    // │    │ ├─ suffix      : nullptr
    // │    ├─┴─ linkOrValue : link(next)
    // │    │                  └─── Key: 0x1111'1111'1111'1111
    // │    │                       │ ┌─ key_len     : has_suffix          
    // │    │                       │ ├─ suffix      : {0x2222'2222'2222'2222, 0x3333'3333'3333'3333, 0x0A0B'0000'0000'0000}, 2
    // │    │                       ├─┴─ linkOrValue : Value(200)
    // │    │                       
    // ├─── Key: 0x8888'8888'8888'8888

    // masstree_putでは、handle_break_invariant後next_layerに行ってそこでRETRYする(ちなみにこのケースだとnext_layerでもう一度handle_break_invariantが呼ばれるはず)
    EXPECT_EQ(borderNode->getKeyLen(1), BorderNode::key_len_layer);
    EXPECT_EQ(borderNode->getKeySuffixes().get(1), nullptr);
    BorderNode *next = reinterpret_cast<BorderNode *>(borderNode->getLV(1).next_layer);
    EXPECT_EQ(next->getKeyLen(0), BorderNode::key_len_has_suffix);
    EXPECT_EQ(next->getKeySlice(0), 0x1111'1111'1111'1111);
    EXPECT_EQ(next->getKeySuffixes().get(0)->getCurrentSlice().slice, 0x2222'2222'2222'2222);
}

// TEST(PutTest, insert_into_border);

TEST(PutTest, split_keys_among_interior) {
    // InteriorNodeを正常にsplitできるかのテスト
    // interiorNode1が既に埋まっていて、それをinteriorNode2との間でsplitする
    InteriorNode interiorNode1, interiorNode2, childNode, dummyNode;
    // interiorNode1を埋める(子ノードとしてdummyNodeをセットする)
    interiorNode1.setNumKeys(Node::ORDER - 1);
    for (size_t i = 0; i < Node::ORDER; i++) {
        interiorNode1.setChild(i, &dummyNode);
    }
    interiorNode1.setKeySlice(0,  0);
    interiorNode1.setKeySlice(1,  1);
    interiorNode1.setKeySlice(2,  2);
    interiorNode1.setKeySlice(3,  3);
    interiorNode1.setKeySlice(4,  4);
    interiorNode1.setKeySlice(5,  5);
    interiorNode1.setKeySlice(6,  6);
    interiorNode1.setKeySlice(7,  7);
    interiorNode1.setKeySlice(8,  9);
    interiorNode1.setKeySlice(9,  10);
    interiorNode1.setKeySlice(10, 11);
    interiorNode1.setKeySlice(11, 12);
    interiorNode1.setKeySlice(12, 13);
    interiorNode1.setKeySlice(13, 14);
    interiorNode1.setKeySlice(14, 15);
    // MEMO: interiorNode1にはkey: 8が存在しないよ

    // === before ===
    // <interiorNode1>
    // │    index │  0 │  1 │  2 │  3 │  4 │  5 │  6 │  7 │  8 │  9 │ 10 │ 11 │ 12 │ 13 │ 14 │
    // ├──────────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼────┤
    // │ KeySlice │  0 │  1 │  2 │  3 │  4 │  5 │  6 │  7 │  9 │ 10 │ 11 │ 12 │ 13 │ 14 │ 15 │
    //            dm   dm   dm   dm   dm   dm   dm   dm   dm   dm   dm   dm   dm   dm   dm   dm

    // === after ===
    // <interiorNode1>
    // │    index │  0 │  1 │  2 │  3 │  4 │  5 │  6 │  7 │     │
    // ├──────────┼────┼────┼────┼────┼────┼────┼────┼────┼ ... ┤
    // │ KeySlice │  0 │  1 │  2 │  3 │  4 │  5 │  6 │  0 │     │
    //            dm   dm   dm   dm   dm   dm   dm   dm   
    // <interiorNode2>
    // │    index │  0 │  1 │  2 │  3 │  4 │  5 │  6 │  7 │  8 │     │
    // ├──────────┼────┼────┼────┼────┼────┼────┼────┼────┼────┼ ... ┤
    // │ KeySlice │  7 │  9 │ 10 │ 11 │ 12 │ 13 │ 14 │ 15 │  0 │     │
    //            │    dm   dm   dm   dm   dm   dm   dm   dm
    //            └─ childNode

    interiorNode1.lock();
    interiorNode1.setSplitting(true);
    interiorNode2.lock();
    interiorNode2.setSplitting(true);
    std::optional<uint64_t> k_prime;
    split_keys_among(&interiorNode1, &interiorNode2, 8, &childNode, 7, k_prime);
    EXPECT_EQ(interiorNode1.getNumKeys(), 7);
    EXPECT_EQ(interiorNode1.getKeySlice(7), 0);
    EXPECT_EQ(interiorNode1.getChild(8), nullptr);
    EXPECT_EQ(interiorNode2.getNumKeys(), 8);
    EXPECT_EQ(interiorNode2.getKeySlice(0), 7);
    EXPECT_EQ(interiorNode2.getKeySlice(8), 0);
    EXPECT_EQ(interiorNode2.getChild(0), &childNode);
    EXPECT_EQ(interiorNode2.getChild(1), &dummyNode);
    EXPECT_EQ(interiorNode2.getChild(9), nullptr);
}

TEST(PutTest, split_keys_among_border) {
    // BorderNodeを正常にsplitできるかのテスト
    // borderNode1が既に埋まっていて、それをborderNode2との間でsplitする
    BorderNode *borderNode1 = new BorderNode;
    BorderNode *borderNode2 = new BorderNode;

    // borderNode1を埋める
    Value value(100);
    borderNode1->setKeyLen(0, 1);
    borderNode1->setKeySlice(0, 110);
    borderNode1->setLV(0, LinkOrValue(&value));
    borderNode1->setKeyLen(1, 2);
    borderNode1->setKeySlice(1, 110);
    borderNode1->setLV(1, LinkOrValue(&value));
    borderNode1->setKeyLen(2, 3);
    borderNode1->setKeySlice(2, 110);
    borderNode1->setLV(2, LinkOrValue(&value));
    borderNode1->setKeyLen(3, BorderNode::key_len_has_suffix);
    borderNode1->setKeySlice(3, 110);
    borderNode1->getKeySuffixes().set(3, new BigSuffix({0x0A0B'0000'0000'0000}, 2));
    borderNode1->setLV(3, LinkOrValue(&value));

    borderNode1->setKeyLen(4, 1);
    borderNode1->setKeySlice(4, 111);
    borderNode1->setLV(4, LinkOrValue(&value));
    borderNode1->setKeyLen(5, 2);
    borderNode1->setKeySlice(5, 111);
    borderNode1->setLV(5, LinkOrValue(&value));
    borderNode1->setKeyLen(6, 3);
    borderNode1->setKeySlice(6, 111);
    borderNode1->setLV(6, LinkOrValue(&value));
    borderNode1->setKeyLen(7, BorderNode::key_len_has_suffix);
    borderNode1->setKeySlice(7, 111);
    borderNode1->getKeySuffixes().set(7, new BigSuffix({0x0C0D'0000'0000'0000}, 2));
    borderNode1->setLV(7, LinkOrValue(&value));

    borderNode1->setKeyLen(8 ,1);
    borderNode1->setKeySlice(8, 113);
    borderNode1->setLV(8, LinkOrValue(&value));
    borderNode1->setKeyLen(9 ,2);
    borderNode1->setKeySlice(9, 113);
    borderNode1->setLV(9, LinkOrValue(&value));
    borderNode1->setKeyLen(10, 3);
    borderNode1->setKeySlice(10, 113);
    borderNode1->setLV(10, LinkOrValue(&value));
    borderNode1->setKeyLen(11, BorderNode::key_len_layer);
    borderNode1->setKeySlice(11, 113);
    BorderNode next;
    borderNode1->setLV(11, LinkOrValue(&next));

    borderNode1->setKeyLen(12, 1);
    borderNode1->setKeySlice(12, 114);
    borderNode1->setLV(12, LinkOrValue(&value));
    borderNode1->setKeyLen(13, 2);
    borderNode1->setKeySlice(13 ,114);
    borderNode1->setLV(13, LinkOrValue(&value));
    borderNode1->setKeyLen(14, 3);
    borderNode1->setKeySlice(14, 114);
    borderNode1->setLV(14, LinkOrValue(&value));
    // === before ===
    // <borderNode1>
    // ┌─── Key: 110 ───┬─ key_len     : 1
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 110 ───┬─ key_len     : 2
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 110 ───┬─ key_len     : 3
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 110 ───┬─ key_len     : has_suffix
    // │                │  suffix      : {0x0A0B'0000'0000'0000}, 2
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 111 ───┬─ key_len     : 1
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 111 ───┬─ key_len     : 2
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 111 ───┬─ key_len     : 3
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 111 ───┬─ key_len     : has_suffix
    // │                │  suffix      : {0x0C0D'0000'0000'0000}, 2
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 113 ───┬─ key_len     : 1
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 113 ───┬─ key_len     : 2
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 113 ───┬─ key_len     : 3
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 113 ───┬─ key_len     : layer
    // │                └─ linkOrValue : Link(next)
    // ├─── Key: 114 ───┬─ key_len     : 1
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 114 ───┬─ key_len     : 2
    // │                └─ linkOrValue : Value(100)
    // └─── Key: 114 ───┬─ key_len     : 3
    //                  └─ linkOrValue : Value(100)

    // === after ===
    // <borderNode1>
    // ┌─── Key: 110 ───┬─ key_len     : 1
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 110 ───┬─ key_len     : 2
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 110 ───┬─ key_len     : 3
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 110 ───┬─ key_len     : has_suffix
    // │                │  suffix      : {0x0A0B'0000'0000'0000}, 2
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 111 ───┬─ key_len     : 1
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 111 ───┬─ key_len     : 2
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 111 ───┬─ key_len     : 3
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 111 ───┬─ key_len     : has_suffix
    // │                │  suffix      : {0x0C0D'0000'0000'0000}, 2
    // │                └─ linkOrValue : Value(100)
    // <borderNode2>
    // ┌─── Key: 113 ───┬─ key_len     : 1
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 113 ───┬─ key_len     : 2
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 113 ───┬─ key_len     : 3
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 113 ───┬─ key_len     : layer
    // │                └─ linkOrValue : Link(next)
    // ├─── Key: 114 ───┬─ key_len     : 1
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 114 ───┬─ key_len     : 2
    // │                └─ linkOrValue : Value(100)
    // ├─── Key: 114 ───┬─ key_len     : 3
    // │                └─ linkOrValue : Value(100)

    borderNode1->lock();
    borderNode1->setSplitting(true);
    borderNode1->setPermutation(Permutation::fromSorted(15));
    borderNode2->lock();
    borderNode2->setSplitting(true);

    Key key1({112, 0x0A0B'0000'0000'0000}, 2);
    split_keys_among(borderNode1, borderNode2, key1, &value);
    EXPECT_EQ(borderNode1->getKeyLen(8), BorderNode::key_len_has_suffix);
    EXPECT_EQ(borderNode1->getKeyLen(9), 0);
    EXPECT_EQ(borderNode2->getKeyLen(3), BorderNode::key_len_layer);

    // sortされていないBorderNodeを作成して正常にsplit pointを選定できるかのテスト
    BorderNode *unsorted = new BorderNode;
    for (size_t i = 0; i <= 7; i++) {
        unsorted->setKeyLen(i, i+1);
        unsorted->setKeySlice(i, 0x2222'2222'2222'2222);
    }
    for (size_t i = 8; i <= 14; i++) {
        unsorted->setKeyLen(i, i-7);
        unsorted->setKeySlice(i, 0x1111'1111'1111'1111);
    }
    unsorted->setPermutation(Permutation::from({8, 9, 10, 11, 12, 13, 14, 0, 1, 2, 3, 4, 5, 6, 7}));
    unsorted->lock();
    unsorted->setSplitting(true);
    BorderNode *borderNode3 = new BorderNode;
    borderNode3->lock();
    borderNode3->setSplitting(true);
    Key key2({0x1111'1111'1111'1111}, 8);
    split_keys_among(unsorted, borderNode3, key2, &value);
    EXPECT_EQ(unsorted->getKeyLen(7), 7);
    EXPECT_EQ(unsorted->getKeySlice(7), 0x1111'1111'1111'1111);
    EXPECT_EQ(borderNode3->getKeyLen(0), 1);
    EXPECT_EQ(borderNode3->getKeySlice(0), 0x2222'2222'2222'2222);
}

TEST(PutTest, split_point) {
    // splitする際の適切な場所を決定するsplit_point関数のテスト
    BorderNode *node = new BorderNode;
    node->setKeyLen(0, 1);
    node->setKeySlice(0, 0x1111'1111'1111'1111);
    node->setKeyLen(1, 2);
    node->setKeySlice(1, 0x1111'1111'1111'1111);
    node->setKeyLen(2, 3);
    node->setKeySlice(2, 0x1111'1111'1111'1111);
    node->setKeyLen(3, 4);
    node->setKeySlice(3, 0x1111'1111'1111'1111);
    node->setKeyLen(4, BorderNode::key_len_layer);
    node->setKeySlice(4, 0x1111'1111'1111'1111);
    node->setKeyLen(5, 1);
    node->setKeySlice(5, 0x2222'2222'2222'2222);
    node->setKeyLen(6, 2);
    node->setKeySlice(6, 0x2222'2222'2222'2222);
    node->setKeyLen(7, 3);
    node->setKeySlice(7, 0x2222'2222'2222'2222);
    node->setKeyLen(8, 4);
    node->setKeySlice(8, 0x2222'2222'2222'2222);
    node->setKeyLen(9, BorderNode::key_len_layer);
    node->setKeySlice(9, 0x2222'2222'2222'2222);
    node->setKeyLen(10, 1);
    node->setKeySlice(10, 0x4444'4444'4444'4444);
    node->setKeyLen(11, 2);
    node->setKeySlice(11, 0x4444'4444'4444'4444);
    node->setKeyLen(12, 3);
    node->setKeySlice(12, 0x4444'4444'4444'4444);
    node->setKeyLen(13, 4);
    node->setKeySlice(13, 0x4444'4444'4444'4444);
    node->setKeyLen(14, BorderNode::key_len_layer);
    node->setKeySlice(14, 0x4444'4444'4444'4444);
    node->setPermutation(Permutation::fromSorted(15));
    // ┌─── Key: 0x1111'1111'1111'1111 ─── key_len  : 1
    // ├─── Key: 0x1111'1111'1111'1111 ─── key_len  : 2
    // ├─── Key: 0x1111'1111'1111'1111 ─── key_len  : 3
    // ├─── Key: 0x1111'1111'1111'1111 ─── key_len  : 4
    // ├─── Key: 0x1111'1111'1111'1111 ─── key_len  : layer
    // ├─── Key: 0x2222'2222'2222'2222 ─── key_len  : 1
    // ├─── Key: 0x2222'2222'2222'2222 ─── key_len  : 2
    // ├─── Key: 0x2222'2222'2222'2222 ─── key_len  : 3
    // ├─── Key: 0x2222'2222'2222'2222 ─── key_len  : 4
    // ├─── Key: 0x2222'2222'2222'2222 ─── key_len  : layer
    // ├─── Key: 0x4444'4444'4444'4444 ─── key_len  : 1
    // ├─── Key: 0x4444'4444'4444'4444 ─── key_len  : 2
    // ├─── Key: 0x4444'4444'4444'4444 ─── key_len  : 3
    // ├─── Key: 0x4444'4444'4444'4444 ─── key_len  : 4
    // └─── Key: 0x4444'4444'4444'4444 ─── key_len  : layer
    std::vector<std::pair<uint64_t, size_t>> table;
    std::vector<uint64_t> found;

    create_slice_table(node, table, found);
    // <table>
    // ┌───────────────────────┬─────────────┐
    // │ key_slice             │ start_index │
    // ├───────────────────────┼─────────────┤
    // │ 0x1111'1111'1111'1111 │ 0           │
    // │ 0x2222'2222'2222'2222 │ 5           │
    // │ 0x4444'4444'4444'4444 │ 10          │
    // └───────────────────────┴─────────────┘
    // <found>
    // {0x1111'1111'1111'1111, 0x2222'2222'2222'2222, 0x4444'4444'4444'4444}
    EXPECT_EQ(table[0], std::make_pair(static_cast<uint64_t>(0x1111'1111'1111'1111), static_cast<size_t>(0)));
    EXPECT_EQ(found[0], 0x1111'1111'1111'1111);
    EXPECT_EQ(table[1], std::make_pair(static_cast<uint64_t>(0x2222'2222'2222'2222), static_cast<size_t>(5)));
    EXPECT_EQ(found[1], 0x2222'2222'2222'2222);
    EXPECT_EQ(table[2], std::make_pair(static_cast<uint64_t>(0x4444'4444'4444'4444), static_cast<size_t>(10)));
    EXPECT_EQ(found[2], 0x4444'4444'4444'4444);

    EXPECT_EQ(split_point(0x0A0B'0000'0000'0000, table, found), 1);
    EXPECT_EQ(split_point(0x1111'1111'1111'1111, table, found), 5 + 1);
    EXPECT_EQ(split_point(0x2222'2222'2222'2222, table, found), 5);
    EXPECT_EQ(split_point(0x3333'3333'3333'3333, table, found), 10 + 1);
    EXPECT_EQ(split_point(0x4444'4444'4444'4444, table, found), 10);
    EXPECT_EQ(split_point(0x5555'5555'5555'5555, table, found), 15);
}

TEST(PutTest, update_and_gc) {
    // 既にデータある場合、古いデータをgcに渡すテスト
    GarbageCollector gc;
    Key key({1}, 1);
    Value *value = new Value(1);
    Node *node = masstree_put(nullptr, key, value, gc).second;
    node = masstree_put(node, key, new Value(2), gc).second;
    // valueが上書きされるのでこのアイテムはgcに渡されているはず
    EXPECT_TRUE(gc.contain(value));
}