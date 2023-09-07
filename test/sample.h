#pragma once

#include "../src/include/masstree_node.h"

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