#pragma once

#include "../src/include/masstree_node.h"

/*
 * The following functions are used in test/tree.cpp
 */

// @param Key key({0x0001'0203'0405'0607, 0x0A0B'0000'0000'0000}, 2), value = 1
static Node *createRootWithSuffix() {
    BorderNode *root = new BorderNode;
    root->setKeyLen(0, BorderNode::key_len_has_suffix);
    root->setKeySlice(0, 0x0001'0203'0405'0607);
    Key key({0x0001'0203'0405'0607, 0x0A0B'0000'0000'0000}, 2);
    root->getKeySuffixes().set(0, key, 1);
    root->setLV(0, LinkOrValue(new Value(1)));
    root->setIsRoot(true);
    root->setPermutation(Permutation::fromSorted(1));
    return root;
    // root
    // ├─── Key: 0x0001'0203'0405'0607 (With Suffix: 0x0A0B'0000'0000'0000)
    // └─── Value: 1
}

// @param Key1 key({0x0001'0203'0405'0607, 0x0A0B'0000'0000'0000}, 2), value = 1
// @param key2 key({0x0001'0203'0405'0607, 0x0C0D'0000'0000'0000}, 2), value = 2
static Node *createTwoLayerWithTwoKeys() {
    BorderNode *layer0_root = new BorderNode;
    BorderNode *layer1_root = new BorderNode;

    layer0_root->setKeyLen(0, BorderNode::key_len_layer);
    layer0_root->setKeySlice(0, 0x0001'0203'0405'0607);
    layer0_root->setLV(0, LinkOrValue(layer1_root));
    layer0_root->setIsRoot(true);
    layer0_root->setPermutation(Permutation::fromSorted(1));

    layer1_root->setKeyLen(0, 2);
    layer1_root->setKeySlice(0, 0x0A0B'0000'0000'0000);
    layer1_root->setLV(0, LinkOrValue(new Value(1)));
    layer1_root->setKeyLen(1, 2);
    layer1_root->setKeySlice(1, 0x0C0D'0000'0000'0000);
    layer1_root->setLV(1, LinkOrValue(new Value(2)));
    layer1_root->setIsRoot(true);
    layer1_root->setPermutation(Permutation::fromSorted(2));
    // ├─── Key: 0x0001'0203'0405'0607
    // │    └─── layer1_root (Is also a root)
    // │         ├─── Key: 0x0A0B'0000'0000'0000
    // │         │    └─── Value: 1
    // │         └─── Key: 0x0C0D'0000'0000'0000
    // │              └─── Value: 2
    return layer0_root;
}

static Node *createTreeWithOneInteriorAndThreeBorders() {
    InteriorNode *root = new InteriorNode;
    BorderNode *borderNode_9 = new BorderNode;
    BorderNode *borderNode_11 = new BorderNode;
    BorderNode *borderNode_160 = new BorderNode;

    root->setIsRoot(true);
    root->setNumKeys(15);
    root->setKeySlice(0,  0x0100);
    root->setKeySlice(1,  0x0200);
    root->setKeySlice(2,  0x0300);
    root->setKeySlice(3,  0x0400);
    root->setKeySlice(4,  0x0500);
    root->setKeySlice(5,  0x0600);
    root->setKeySlice(6,  0x0700);
    root->setKeySlice(7,  0x0800);
    root->setKeySlice(8,  0x0900);
    root->setKeySlice(9,  0x0100'00);
    root->setKeySlice(10, 0x0101'00);
    root->setKeySlice(11, 0x0102'00);
    root->setKeySlice(12, 0x0103'00);
    root->setKeySlice(13, 0x0104'00);
    root->setKeySlice(14, 0x0105'00);
    root->setChild(0,  borderNode_9);
    root->setChild(1,  borderNode_11);
    root->setChild(15, borderNode_160);

    root->lock();
    borderNode_9->setKeyLen(0, 1);
    borderNode_9->setKeySlice(0, 0x09);
    borderNode_9->setLV(0, LinkOrValue(new Value(18)));
    borderNode_9->setParent(root);

    borderNode_11->setKeyLen(0, 2);
    borderNode_11->setKeySlice(0, 0x0101);
    borderNode_11->setLV(0, LinkOrValue(new Value(22)));
    borderNode_11->setParent(root);

    borderNode_160->setKeyLen(0, 3);
    borderNode_160->setKeySlice(0, 0x0106'00);
    borderNode_160->setLV(0, LinkOrValue(new Value(320)));
    borderNode_160->setParent(root);
    root->unlock();
    //      ┌─── Key: 0x09 -> Value: 18
    // ├─── Key: 0x0100
    // │    ├─── Key: 0x0101 -> Value: 22
    // ├─── Key: 0x0200
    // ├─── Key: 0x0300
    // ├─── Key: 0x0400
    // ├─── Key: 0x0500
    // ├─── Key: 0x0600
    // ├─── Key: 0x0700
    // ├─── Key: 0x0800
    // ├─── Key: 0x0900
    // ├─── Key: 0x0100'00
    // ├─── Key: 0x0101'00
    // ├─── Key: 0x0102'00
    // ├─── Key: 0x0103'00
    // ├─── Key: 0x0104'00
    // ├─── Key: 0x0105'00
    //      └─── Key: 0x0106'00 -> Value: 320
    return root;
}



/*
 * The following functions and constants are used in test/permutation.cpp
 */

static constexpr uint64_t ONE   = 0x1111111111111111;
static constexpr uint64_t TWO   = 0x2222222222222222;
static constexpr uint64_t THREE = 0x3333333333333333;
static constexpr uint64_t FOUR  = 0x4444444444444444;
static constexpr uint64_t FIVE  = 0x5555555555555555;
static constexpr uint64_t SIX   = 0x6666666666666666;
static constexpr uint64_t SEVEN = 0x7777777777777777;
static constexpr uint64_t EIGHT = 0x8888888888888888;
static constexpr uint64_t NINE  = 0x9999999999999999;

static void makeSampleBorderNode(BorderNode *node) {
    node->setKeySlice(0, 2);
    node->setKeyLen(0, 5);

    node->setKeySlice(1, 2);
    node->setKeyLen(1, 7);

    node->setKeySlice(2, 0);
    node->setKeyLen(2, 0);

    node->setKeySlice(3, 1);
    node->setKeyLen(3, 1);

    node->setKeySlice(4, 1);
    node->setKeyLen(4, 2);

    node->setKeySlice(5, 1);
    node->setKeyLen(5, 8);
    // key_slice : [ 2| 2| 0| 1| 1| 1|__|__|__|__|__|__|__|__|__]
    // key_len   : [ 5| 7| 0| 1| 2| 8|__|__|__|__|__|__|__|__|__]
    node->setPermutation(Permutation::from({3, 4, 5, 0, 1}));
}