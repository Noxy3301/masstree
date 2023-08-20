#pragma once

#include "masstree_node.h"

class GarbageCollector {
    public:
        GarbageCollector() = default;
        GarbageCollector(GarbageCollector &&other) = delete;
        GarbageCollector(const GarbageCollector &other) = delete;
        GarbageCollector &operator=(GarbageCollector &&other) = delete;
        GarbageCollector &operator=(const GarbageCollector &other) = delete;

        void add(BorderNode *borderNode) {
            assert(!contain(borderNode));
            assert(borderNode->getDeleted());
            borders.push_back(borderNode);
        }

        void add(InteriorNode *interiorNode) {
            assert(!contain(interiorNode));
            assert(interiorNode->getDeleted());
            interiors.push_back(interiorNode);
        }

        void add(Value *value) {
            assert(!contain(value));
            values.push_back(value);
        }

        void add(BigSuffix *suffix) {
            assert(!contain(suffix));
            suffixes.push_back(suffix);
        }

        bool contain(BorderNode const *borderNode) {
            return std::find(borders.begin(), borders.end(), borderNode) != borders.end();
        }

        bool contain(InteriorNode const *interiorNode) {
            return std::find(interiors.begin(), interiors.end(), interiorNode) != interiors.end();
        }

        bool contain(Value const *value) {
            return std::find(values.begin(), values.end(), value) != values.end();
        }

        bool contain(BigSuffix const *suffix) const {
            return std::find(suffixes.begin(), suffixes.end(), suffix) != suffixes.end();
        }

        void run() {
            for (auto &borderNode : borders) delete borderNode;
            borders.clear();

            for (auto &interiorNode: interiors) delete interiorNode;
            interiors.clear();

            for (auto &value : values) delete value;
            values.clear();

            for (auto &suffix : suffixes) delete suffix;
            suffixes.clear();
        }

    private:
        std::vector<BorderNode *> borders{};
        std::vector<InteriorNode *> interiors{};
        std::vector<Value *> values{};
        std::vector<BigSuffix *> suffixes{};
};