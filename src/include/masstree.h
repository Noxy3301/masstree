#pragma once

#include "masstree_put.h"
#include "masstree_get.h"
#include "masstree_remove.h"
#include "masstree_scan.h"
#include "status.h"

class Masstree {
    public:
        Value *get(Key &key) {
            Node *root_ = root.load(std::memory_order_acquire);
            Value *v = masstree_get(root_, key);
            key.reset();
            return v;
        }

        void put(Key &key, Value *value, GarbageCollector &gc) {
        RETRY:
            Node *old_root = root.load(std::memory_order_acquire);
            std::pair<PutResult, Node*> resultPair = masstree_put(old_root, key, value, gc);
            if (resultPair.first == RetryFromUpperLayer) goto RETRY;
            Node *new_root = resultPair.second;

            key.reset();
            // old_rootがぬるぽならrootを作るけど他スレッドと争奪戦が起きるのでCASを使う
            if (old_root == nullptr) {
                bool CAS_success = root.compare_exchange_weak(old_root, new_root);
                if (CAS_success) {
                    return;
                } else {
                    // ハァ...ハァ...敗北者...?(new_rootを消す)
                    assert(new_root != nullptr);
                    assert(new_root->getIsBorder());
                    new_root->setDeleted(true);
                    gc.add(reinterpret_cast<BorderNode*>(new_root));
                    goto RETRY;
                }
            }
            // masstree_putでrootが更新された場合Masstree自体のrootを更新する
            if (old_root != new_root) {
                assert(old_root != nullptr);
                root.store(new_root, std::memory_order_release);
            }
        }


        // Scan results will be stored in a vector of <Key, Value> pairs, provided as an argument.
        void scan(Key &left_key,
                  bool l_exclusive,
                  Key &right_key,
                  bool r_exclusive,
                  std::vector<std::pair<Key, Value*>> &result) {
            Node *root_ = root.load(std::memory_order_acquire);
            Key current_key = left_key;
            Status scan_status = Status::OK;
            masstree_scan(root_, scan_status, current_key, left_key, l_exclusive, right_key, r_exclusive, result);
            left_key.reset();
            right_key.reset();
        }

    private:
        std::atomic<Node *> root{nullptr};
};