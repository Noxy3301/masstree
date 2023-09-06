#pragma once

#include "../src/include/masstree_node.h"

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