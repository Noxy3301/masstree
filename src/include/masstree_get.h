#pragma once

#include "masstree_node.h"


Value *masstree_get(Node *root, Key &key) {
    if (root == nullptr) return nullptr;    // Layer0がemptyの状態でgetが来た場合
RETRY:
    std::pair<BorderNode*, Version> node_version = findBorder(root, key);
    BorderNode *node = node_version.first;
    Version version = node_version.second;
FORWARD:
    if (version.deleted) {
        if (version.is_root) {
            return nullptr; // Layer0がemptyにされた or 下位レイヤに移った場合
        } else {
            goto RETRY;
        }
    }
    
}