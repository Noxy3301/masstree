#include "include/masstree.h"

Status Masstree::insert_value(Key &key, Value *value, GarbageCollector &gc) {
RETRY:
    Node *old_root = root.load(std::memory_order_acquire);
    std::pair<Status, Node*> resultPair = masstree_insert(old_root, key, value, gc);
    if (resultPair.first == Status::RETRY_FROM_UPPER_LAYER) goto RETRY;
    if (resultPair.first == Status::WARN_ALREADY_EXISTS) return resultPair.first;
    Node *new_root = resultPair.second;
    key.reset();
    // old_rootがぬるぽならrootを作るけど他スレッドと争奪戦が起きるのでCASを使う
    if (old_root == nullptr) {
        bool CAS_success = root.compare_exchange_weak(old_root, new_root);
        if (CAS_success) {
            return Status::OK;
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
    
    return Status::OK;
}


Status Masstree::remove_value(Key &key, GarbageCollector &gc) {
RETRY:
    Node *old_root = root.load(std::memory_order_acquire);
    if (old_root == nullptr) return Status::WARN_NOT_FOUND;

    std::pair<RootChange, Node*> resultPair = remove_at_layer0(old_root, key, gc);
    Node *new_root = resultPair.second;
    key.reset();

    if (resultPair.first == RootChange::DataNotFound) return Status::WARN_NOT_FOUND;

    // remove_at_layer0でrootが更新された場合Masstree自体のrootを更新する
    if (old_root != new_root) {
        bool CAS_success = root.compare_exchange_weak(old_root, new_root);
        if (CAS_success) {
            return Status::OK;
        } else {
            // CHECK: CASが失敗するケースをテストで確認したい
            goto RETRY;
        }
    }
    return Status::OK;
}

Value *Masstree::get_value(Key &key) {
    Node *root_ = root.load(std::memory_order_acquire);
    Value *v = masstree_get(root_, key);
    key.reset();
    return v;
}