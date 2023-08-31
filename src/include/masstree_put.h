#pragma once

#include "masstree_node.h"
#include "masstree_gc.h"

enum PutResult : uint8_t {
    DONE,
    RetryFromUpperLayer
};

BorderNode *start_new_tree(const Key &key, Value *value);

static ssize_t check_break_invariant(BorderNode *const borderNode, const Key &key);

static void handle_break_invariant(BorderNode *node, Key &key, size_t old_index, GarbageCollector &gc);

static void insert_to_border(BorderNode *border, const Key &key, Value *value, GarbageCollector &gc);

// static void insert_into_border(BorderNode *border, const Key &key, Value *value, GarbageCollector &gc) {}

// static size_t cut(size_t len) {}

static void split_keys_among(InteriorNode *parent, InteriorNode *parent1, uint64_t slice, Node *node1, size_t node_index, std::optional<uint64_t> &k_prime);

static void create_slice_table(BorderNode *const node, std::vector<std::pair<uint64_t, size_t>> &table, std::vector<uint64_t> &found);

static size_t split_point(uint64_t new_slice, const std::vector<std::pair<uint64_t, size_t>> &table, const std::vector<uint64_t> &found);

static void split_keys_among(BorderNode *node, BorderNode *node1, const Key &key, Value *value);

static InteriorNode *create_root_with_children(Node *left, uint64_t slice, Node *right);

static void insert_into_parent(InteriorNode *parent, Node *node1, uint64_t slice, size_t node_index);

static Node *split(Node *node, const Key &key, Value *value);

std::pair<PutResult, Node*> masstree_put(Node *root, Key &key, Value *value, GarbageCollector &gc);