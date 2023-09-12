#include <gtest/gtest.h>

#include "../src/include/masstree.h"
#include "sample.h"
#include "gtest_util.h"

class DummyNode : public Node {
public:
    explicit DummyNode(int v) : value(v) {}
    int getValue() const { return value; }
    void setValue(int v) { value = v; }
private:
    int value;
};

TEST(TreeTest, findChild_lessThanKeySlice) {
    // InteriorNodeのfindChild関数がちゃんと動くかの確認(When less than the key_slice)
    // InteriorNodeのKey_slice[0] = 0x01
    // findChildの条件式に渡すとslice < key_slice[i](0x00 < 0x01)で0番目のKeyが返ってくる
    InteriorNode node;
    DummyNode dummyNode{0};
    node.setChild(0, &dummyNode);
    node.setNumKeys(1);
    node.setKeySlice(0, 0x01);

    DummyNode* foundChild = reinterpret_cast<DummyNode *>(node.findChild(0x00));
    EXPECT_EQ(foundChild->getValue(), 0);
}

TEST(TreeTest, findChild_EqualToKeySlice) {
    // InteriorNodeのfindChild関数がちゃんと動くかの確認(When equal to the key_slice)
    // InteriorNodeのKey_slice[0] = 0x01
    //  | 1 | 
    //  ┌┘ └┐
    // (0) (2)
    InteriorNode node;
    DummyNode dummyNode_left{0};
    DummyNode dummyNode_right{2};
    node.setChild(0, &dummyNode_left);
    node.setChild(1, &dummyNode_right);
    node.setNumKeys(1);
    node.setKeySlice(0, 0x01);

    DummyNode* foundChild = reinterpret_cast<DummyNode *>(node.findChild(0x01));
    EXPECT_EQ(foundChild->getValue(), 2);
}

TEST(TreeTest, RootWithSuffix) {
    // Suffixを持つBorderNodeを作成して動作を確認する
    Node *root = createRootWithSuffix();
    Key key({0x0001'0203'0405'0607, 0x0A0B'0000'0000'0000}, 2);
    std::pair<BorderNode*, Version> borderNode_version = findBorder(root, key);
    // createRootWithSuffixでsuffixを登録してあるからそれの確認
    EXPECT_EQ(borderNode_version.first->getKeySuffixes().get(0)->getCurrentSlice().slice, 0x0A0B'0000'0000'0000);
    // std::cout << GTEST_COUT_INFO << borderNode_version.first->getKeySuffixes().get(0)->getCurrentSlice().slice << std::endl;
    EXPECT_EQ(*borderNode_version.first->getLV(0).value, 1);
}

TEST(TreeTest, TwoLayerWithTwoKeys) {
    // Layer0, Layer1を跨ぐ2つのKeyを作成して動作を確認する
    Node *root = createTwoLayerWithTwoKeys();
    Key key({0x0001020304050607, 0x0A0B'0000'0000'0000}, 2);
    std::pair<BorderNode*, Version> borderNode_version = findBorder(root, key);
    // レイヤの中で指定したSliceを持つBorderNodeを探す
    // 本来だったらfindBorderの結果をsearchLinkOrValueに投げて、その結果に応じて下位レイヤに行くかValueを返す
    EXPECT_EQ(borderNode_version.first->getKeyLen(0), BorderNode::key_len_layer);
}

TEST(TreeTest, getValueFromRootWithSuffix) {
    // createRootWithSuffixで作成したBorderNodeから値を取得する
    Node *root = createRootWithSuffix();
    Key key({0x0001'0203'0405'0607, 0x0A0B'0000'0000'0000}, 2);
    Value *value = masstree_get(root, key);
    assert(value != nullptr);
    EXPECT_EQ(*value, 1);
}

TEST(TreeTest, getValueFromTwoLayerWithTwoKeys) {
    // createTwoLayerWithTwoKeysで作成したBorderNodeから値を取得する
    Node *root = createTwoLayerWithTwoKeys();
    Key key({0x0001'0203'0405'0607, 0x0C0D'0000'0000'0000}, 2);
    Value *value = masstree_get(root, key);
    assert(value != nullptr);
    EXPECT_EQ(*value, 2);
}

TEST(TreeTest, getValueFromTreeWithOneInteriorAndThreeBorders) {
    // createTreeWithOneInteriorAndThreeBordersで作成したツリーから値を取得する
    Node *root = createTreeWithOneInteriorAndThreeBorders();
    Key key({0x0101}, 2);
    Value *value = masstree_get(root, key);
    assert(value != nullptr);
    EXPECT_EQ(*value, 22);

    Key key2({0x0106'00}, 3);
    Value *value2 = masstree_get(root, key2);
    assert(value2 != nullptr);
    EXPECT_EQ(*value2, 320);
}

TEST(TreeTest, putAndGetValueFromTreeWithOneInteriorAndThreeBorders) {
    // createTreeWithOneInteriorAndThreeBordersで作成したツリーにキーを挿入して、正常にgetできるかの確認
    Node *root = createTreeWithOneInteriorAndThreeBorders();
    Key key({0x0101}, 2);
    GarbageCollector gc;

    root = masstree_put(root, key, new Value(23), gc).second;
    Value *value = masstree_get(root, key);
    assert(value != nullptr);
    EXPECT_EQ(*value, 23);

    Key key2({0x0800'1020}, 4);
    root = masstree_put(root, key2, new Value(2442), gc).second;
    Value *value2 = masstree_get(root, key2);
    assert(value2 != nullptr);
    EXPECT_EQ(*value2, 2442);
}

TEST(TreeTest, startNewTree) {
    // start_new_treeで新しいツリーを作成する奴の確認
    Key key({0x0102'0304'0506'0708, 0x0102'0304'0506'0708, 0x0102'0304'0506'0708, 0x1718'1900'0000'0000}, 3);
    Node *root = start_new_tree(key, new Value(100));
    Value *value = masstree_get(root, key);
    assert(value != nullptr);
    EXPECT_EQ(*value, 100);
}

TEST(TreeTest, insert) {
    // 空のMasstreeにkey1とkey2を正常にinsertできるかの確認
    Key key1({0x0102'0304'0506'0708, 0x0A0B'0000'0000'0000}, 2);
    Key key2({0x1112'1314'1516'1718}, 8);
    GarbageCollector gc;
    Node *root = masstree_put(nullptr, key1, new Value(1), gc).second;
    root = masstree_put(root, key2, new Value(2), gc).second;
    Value *value = masstree_get(root, key1);
    assert(value != nullptr);
    EXPECT_EQ(*value, 1);
}


static void print_sub_tree(Node *root){
  if(root->getIsBorder()){
    auto border = reinterpret_cast<BorderNode *>(root);
    border->printNode();
  }else{
    auto interior = reinterpret_cast<InteriorNode *>(root);
    interior->printNode();
  }
}

TEST(TreeTest, split) {
    // BorderNodeがちゃんとsplitできるかの確認
    Node *root = nullptr;
    GarbageCollector gc;
    for (uint64_t i = 0; i < 10000; i++) {
        Key key({i}, 1);
        root = masstree_put(root, key, new Value(i), gc).second;
    }
    // print_sub_tree(root);
    // ちゃんとMasstreeに格納できている(splitできている)ならinsertしたキーを取得できるはず
    Key key2442({2442}, 1);
    Value *value = masstree_get(root, key2442);
    assert(value != nullptr);
    EXPECT_EQ(*value, 2442);
}