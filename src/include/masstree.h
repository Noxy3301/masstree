#pragma once

#include "masstree_insert.h"
#include "masstree_get.h"
#include "masstree_remove.h"
#include "status.h"

class Masstree {
    public:
        Status insert_value(Key &key, Value *value, GarbageCollector &gc);
        Status remove_value(Key &key, GarbageCollector &gc);
        Value *get_value(Key &key);

    private:
        std::atomic<Node *> root{nullptr};
};