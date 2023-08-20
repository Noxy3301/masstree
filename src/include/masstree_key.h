#pragma once

#include <cstdint>
#include <vector>
#include <cassert>

// CHECK: KeyWithSliceって何に使うんだ？

struct SliceWithSize {
    uint64_t slice;
    uint8_t size;

    SliceWithSize(uint64_t slice_, uint8_t size_) : slice(slice_), size(size_) {
        assert(1 <= size && size <= 8);
    }

    bool operator==(const SliceWithSize &right) const {
        return slice == right.slice && size == right.size;
    }

    bool operator!=(const SliceWithSize &right) const {
        return !(*this == right);
    }
};

class Key {
    public:
        std::vector<uint64_t> slices;
        size_t lastSliceSize = 0;
        size_t cursor = 0;

        Key(std::vector<uint64_t> slices_, size_t lastSliceSize_) : slices(std::move(slices_)), lastSliceSize(lastSliceSize_) {
            assert(1 <= lastSliceSize && lastSliceSize <= 8);
        }

        bool hasNext() const {
            if (slices.size() == cursor + 1) return false;
            return true;
        }

        size_t remainLength(size_t from) const {
            assert(from <= slices.size() - 1);
            return (slices.size() - from - 1)*8 + lastSliceSize;
        }

        size_t getCurrentSliceSize() const {
            if (hasNext()) {
                return 8;
            } else {
                return lastSliceSize;
            }
        }

        // 現在のスライスとカーソル(スライスの位置/インデックス)を取得する
        SliceWithSize getCurrentSlice() const {
            return SliceWithSize(slices[cursor], getCurrentSliceSize());
        }

        void next() {
            assert(hasNext());
            cursor++;
        }

        void back() {
            assert(cursor != 0);
            cursor--;
        }

        void reset() {
            cursor = 0;
        }

        bool operator==(const Key &right) const {
            return lastSliceSize == right.lastSliceSize && cursor == right.cursor && slices == right.slices;
        }

        bool operator!=(const Key &right) const {
            return !(*this == right);
        }
};