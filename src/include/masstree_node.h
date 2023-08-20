#pragma once

#include <atomic>
#include <tuple>
#include <cassert>
#include <utility>
#include <algorithm>
#include <mutex>

#include "atomic_wrapper.h"
#include "masstree_version.h"
#include "masstree_value.h"
#include "masstree_key.h"
#include "permutation.h"

class InteriorNode;
class BorderNode;

class Node {
    public:
        static constexpr size_t ORDER = 16;

        // Node() = default;
        Node() : parent(nullptr), upperLayer(nullptr) {};

        // CHECK: このコンストラクタがどういう意味で書かれているのかわからん、ミス防止ってこと？
        Node(const Node& other) = delete;
        Node &operator=(const Node& other) = delete;
        Node(Node&& other) = delete;
        Node &operator=(Node&& other) = delete;

        // Stableな状態((version.inserting || version.splitting) == 0)のversionを取得する
        Version stableVersion() const {
            Version v = getVersion();
            while (v.inserting || v.splitting) v = getVersion();
            return v;
        }

        void lock() {
            Version expected, desired;

            for (;;) {
                expected = getVersion();
                if (!expected.locked) { // lock取れる場合
                    desired = expected;
                    desired.locked = true;
                    if (version.compare_exchange_weak(expected, desired)) return;
                }
            }
        }

        void unlock() {
            Version v = getVersion();
            assert(v.locked && !(v.inserting && v.splitting));
            if (v.inserting) {  // insertとsplittingは同時に起きないことを明示的に書いてる
                v.v_insert++;
            } else if (v.splitting) {
                v.v_split++;
            }
            v.locked    = false;
            v.inserting = false;
            v.splitting = false;

            setVersion(v);
        }

        inline void setInserting(bool inserting) {
            Version v = getVersion();
            assert(v.locked);
            v.inserting = inserting;
            setVersion(v);
        }

        inline void setSplitting(bool splitting) {
            Version v = getVersion();
            assert(v.locked);
            v.splitting = splitting;
            setVersion(v);
        }

        inline void setDeleted(bool deleted) {
            Version v = getVersion();
            assert(v.locked);
            v.deleted = deleted;
            setVersion(v);
        }

        inline void setIsRoot(bool is_root) {
            Version v = getVersion();
            v.is_root = is_root;
            setVersion(v);
        }

        inline void setIsBorder(bool is_border) {
            Version v = getVersion();
            v.is_border = is_border;
            setVersion(v);
        }

        inline void setVersion(Version const &v) {
            // storeRelease(version, v);    
            //TODO: wrapperに&v(ポインタ)を渡すと関数内部で二重ポインタになっているっぽい？wrapper作りたい
            version.store(v, std::memory_order_release);
        }


        inline void setUpperLayer(BorderNode *p) {
            if (p != nullptr) assert(reinterpret_cast<Node *>(p)->isLocked()); // setParentをするときはその親をロックしているのが前提, 自分のノードはロックする必要ない
            // storeRelease(upperLayer, p);
            upperLayer.store(p, std::memory_order_release);
        }

        inline void setParent(InteriorNode *p) {
            if (p != nullptr) assert(reinterpret_cast<Node *>(p)->isLocked());
            // storeRelease(parent, p);
            parent.store(p, std::memory_order_release);
        }
        
        inline bool getInserting() const {
            Version v = getVersion();
            return v.inserting;
        }
        
        inline bool getSplitting() const {
            Version v = getVersion();
            return v.splitting;
        }
        
        inline bool getDeleted() const {
            Version v = getVersion();
            return v.deleted;
        }
        
        inline bool getIsRoot() const {
            Version v = getVersion();
            return v.is_root;
        }
        
        inline bool getIsBorder() const {
            Version v = getVersion();
            return v.is_border;
        }
        
        inline Version getVersion() const {
            // return loadAcquire(version); // TODO: wrapperの作成
            return version.load(std::memory_order_acquire);
        }

        inline BorderNode *getUpperLayer() const {
            // return loadAcquire(upperLayer); // TODO: wrapperの作成
            return upperLayer.load(std::memory_order_acquire);
        }

        inline InteriorNode *getParent() const {
            // return loadAcquire(parent); // TODO: wrapperの作成
            return parent.load(std::memory_order_acquire);
        }

        // ParentのLockを取得する
        InteriorNode *lockedParent() const {
        RETRY:
            Node *p = reinterpret_cast<Node *>(getParent());
            if (p != nullptr) p->lock();
            // pのロック中にpの親が変わってないかの確認((T1)getParent -> (T2)setParent -> (T1)locked)
            if (p != reinterpret_cast<Node *>(getParent())) {
                assert(p != nullptr);
                p->unlock();
                goto RETRY;
            }
            return reinterpret_cast<InteriorNode *>(p);
        }
        // UpperLayerのLockを取得する
        BorderNode *lockedUpperNode() const {
        RETRY:
            Node *p = reinterpret_cast<Node *>(getUpperLayer());
            if (p != nullptr) p->lock();
            // pのロック中にpのUpperLayerが変わってないかの確認((T1)getUpperLayer -> (T2)setUpperLayer -> (T1)locked)
            if (p != reinterpret_cast<Node *>(getUpperLayer())) {
                assert(p != nullptr);
                p->unlock();
                goto RETRY;
            }
            return reinterpret_cast<BorderNode *>(p);
        }

        inline bool isLocked() const {
            Version version = getVersion();
            return version.locked;
        }
        // CHECK: これいつ使うんだ？
        inline bool isUnlocked() const {
            return !isLocked();
        }
    
    private:
        std::atomic<Version> version = {};
        std::atomic<InteriorNode *> parent;
        std::atomic<BorderNode *> upperLayer;
};

class InteriorNode : public Node {
    public:
        InteriorNode() : n_keys(0) {}
        
        Node *findChild(uint64_t slice) {
            uint8_t num_keys = getNumKeys();
            for (size_t i = 0; i < num_keys; i++) {
                if (slice < key_slice[i]) return getChild(i);
            }
            return getChild(num_keys);
        }

        inline bool isNotFull() const {
            return (getNumKeys() != ORDER - 1);
        }

        inline bool isFull() const {
            return !isNotFull();
        }
        // bool debug_has_skip
        // bool printNode

        size_t findChildIndex(Node *arg_child) const {
            size_t index = 0;
            while (index <= getNumKeys() && (getChild(index) != arg_child)) index++;
            assert(getChild(index) == arg_child);
            return index;
        }

        inline uint8_t getNumKeys() const {
            // return loadAcquire(n_keys);  // TODO: wrapperの作成
            return n_keys.load(std::memory_order_acquire);
        }

        inline void setNumKeys(uint8_t nKeys) {
            // storeRelease(n_keys, nKeys);    // TODO: wrapperの作成
            n_keys.store(nKeys, std::memory_order_release);
        }

        inline void incNumKeys() {
            n_keys.fetch_add(1);
        }

        inline void decNumKeys() {
            n_keys.fetch_sub(1);
        }

        inline uint64_t getKeySlice(size_t index) const {
            // return loadAcquire(key_slice[index]);   // TODO: wrapperの作成
            return key_slice[index].load(std::memory_order_acquire);
        }

        void resetKeySlices() {
            for (size_t i = 0; i < ORDER - 1; i++) setKeySlice(i, 0);
        }

        inline void setKeySlice(size_t index, const uint64_t &slice) {
            // storeRelease(key_slice[index], slice);  // TODO: wrapperの作成
            key_slice[index].store(slice, std::memory_order_release);
        }

        inline Node *getChild(size_t index) const {
            // return loadAcquire(child[index]);   // TODO: wrapperの作成
            return child[index].load(std::memory_order_acquire);
        }

        inline void setChild(size_t index, Node *c) {
            assert(0 <= index && index <= 15);
            // storeRelease(child[index], child);  // TODO: wrapperの作成
            child[index].store(c, std::memory_order_release);
        }
        // bool debug_contain_child

        void resetChildren() {
            for (size_t i = 0; i < ORDER; i++) setChild(i, nullptr);
        }

    private:
        std::atomic<uint8_t> n_keys = 0;
        std::array<std::atomic<uint64_t>, ORDER - 1> key_slice = {};
        std::array<std::atomic<Node *>, ORDER> child = {};

};

union LinkOrValue {
    LinkOrValue() = default;

    explicit LinkOrValue(Node *next) : next_layer(next) {}
    explicit LinkOrValue(Value *value_) : value(value_) {}

    Node *next_layer = nullptr;
    Value *value;
};

// vectorの先頭要素を消す
template<typename T>
static void pop_front(std::vector<T> &v) {
    if (!v.empty()) v.erase(v.begin());
}

/*
 * BigSuffixは複数のスライス(8byte)として保存される
 * Suffixの長さ、現在のスライスの取得、Suffixが次のSuffixを持っているかをチェックする
 * Suffixの長さが8byteよりも大きいときに使う
 */
class BigSuffix {
public:
    BigSuffix() = default;
    BigSuffix(const BigSuffix &other) : slices(other.slices), lastSliceSize(other.lastSliceSize) {}
    BigSuffix &operator=(const BigSuffix &other) = delete;
    BigSuffix(BigSuffix &&other) = delete;
    BigSuffix &operator=(const BigSuffix &&other) = delete;

    BigSuffix(std::vector<uint64_t> &&slices_, size_t lastSliceSize_) : slices(std::move(slices_)), lastSliceSize(lastSliceSize_) {}

    // 現在のスライスを取得する(最後のスライスならそのスライスだけ、違うなら8byte)
    SliceWithSize getCurrentSlice() {
        if (hasNext()) {
            std::lock_guard<std::mutex> lock(suffixMutex);
            return SliceWithSize(slices[0], 8);
        } else {
            std::lock_guard<std::mutex> lock(suffixMutex);
            return SliceWithSize(slices[0], lastSliceSize);
        }
    }

    size_t remainLength() {
        std::lock_guard<std::mutex> lock(suffixMutex);
        return (slices.size() - 1) * 8 + lastSliceSize;
    }

    // slicesが2より大きいなら次のスライスがある(slicesをpop_frontしていくのでここで気づける)
    bool hasNext() {
        std::lock_guard<std::mutex> lock(suffixMutex);
        return slices.size() >= 2;
    }

    void next() {
        assert(hasNext());
        pop_front(slices);
    }

    // CHECK: これ何に使うんだ？
    void insertTop(uint64_t slice) {
        std::lock_guard<std::mutex> lock(suffixMutex);
        slices.insert(slices.begin(), slice);
    }

    // keyとsuffixが一致するか
    bool isSame(const Key &key, size_t from) {
        if (key.remainLength(from) != this->remainLength()) return false;
        std::lock_guard<std::mutex> lock(suffixMutex);
        for (size_t i = 0; i < slices.size(); i++) {
            if (key.slices[i + from] != slices[i]) return false;
        }
        return true;
    }

    // 指定したkeyと開始位置から新しいBigSuffixを作成する
    static BigSuffix *from(const Key &key, size_t from) {
        std::vector<uint64_t> temp{};
        for(size_t i = from; i < key.slices.size(); i++) {
            temp.push_back(key.slices[i]);
        }
        return new BigSuffix(std::move(temp), key.lastSliceSize);
    }

private:
    std::mutex suffixMutex{};
    std::vector<uint64_t> slices = {};
    const size_t lastSliceSize = 0;
};

/*
 * KeySuffixはBorderNode内全てのkeyのSuffixへの参照を一元管理する
 */
class KeySuffix {
public:
    KeySuffix() = default;
    
    // fromからのsuffixを取得してBigSuffixにコピーする
    inline void set(size_t i, const Key &key, size_t from) {
        // storeRelease(suffixes[i], BigSuffix::from(key, from));   // TODO: wrapperの作成
        suffixes[i].store(BigSuffix::from(key, from), std::memory_order_release);
    }

    // BigSuffixへのポインタを保存する
    inline void set(size_t i, BigSuffix *const &ptr) {
        // storeRelease(suffixes[i], ptr); // TODO: wrapperの作成
        suffixes[i].store(ptr, std::memory_order_release);
    }

    inline BigSuffix *get(size_t i) const {
        // loadAcquire(suffixes[i]);    // TODO: wrapperの作成
        return suffixes[i].load(std::memory_order_acquire);
    }

    // suffixes[i]のreferenceを外す
    void unreferenced(size_t i) {
        assert(get(i) != nullptr);
        set(i, nullptr);
    }

    void delete_ptr(size_t i) {
        BigSuffix *ptr = get(i);
        assert(ptr != nullptr);
        delete ptr;
        set(i, nullptr);
    }

    void delete_all() {
        for (size_t i = 0; i < Node::ORDER - 1; i++) {
            BigSuffix *ptr = get(i);
            if (ptr != nullptr) delete_ptr(i);
        }
    }

    // suffixesをリセットする、splitの時とかに使う
    void reset() {
        std::fill(suffixes.begin(), suffixes.end(), nullptr);
    }

private:
    std::array<std::atomic<BigSuffix*>, Node::ORDER - 1> suffixes = {};
};

enum SearchResult: uint8_t {
    NOTFOUND,
    VALUE,
    LAYER,
    UNSTABLE
};

class BorderNode : public Node {
    public:
        // CHECK: これ何に使ってるんだっけ
        static constexpr uint8_t key_len_layer = 255;
        static constexpr uint8_t key_len_unstable = 254;
        static constexpr uint8_t key_len_has_suffix = 9;

        BorderNode() : permutation(Permutation::sizeOne()) {
            setIsBorder(true);
        }

        ~BorderNode() {
            for (size_t i = 0; i < ORDER - 1; i++) {
                // assert()
            }
        }

        // BorderNodeの中からkeyに対応するLink or Valueを取得する
        inline std::pair<SearchResult, LinkOrValue> searchLinkOrValue(const Key &key) {
            std::tuple<SearchResult, LinkOrValue, size_t> tuple = searchLinkOrValueWithIndex(key);
            return std::make_pair(std::get<0>(tuple), std::get<1>(tuple));
        }

        // BorderNodeの中からkeyに対応するLink or Valueを取得する、ついでにkeyのindexも返す
        std::tuple<SearchResult, LinkOrValue, size_t> searchLinkOrValueWithIndex(const Key &key) {
            /*
             * 次のスライスがない場合(現在のスライスが最後)はkey sliceの長さは1~8
             * 次のスライスがある場合はkey sliceの長さは8
             * key_lenが8の場合、残りのkey sliceは1つのみであり次のkey sliceはkey suffixに入っているのでそこを探す
             * 
             * key_lenがLAYERを表す場合は次のLayerを探す
             * key_lenがUNSTABLEを表す場合はUNSTABLEを返す
             * それ以外はNOTFOUNDを返す
             */
            SliceWithSize current = key.getCurrentSlice();
            Permutation permutation = getPermutation();

            if (!key.hasNext()) {   // 現在のkeyのスライスが最後の場合(current layerにValueがあるはず)
                for (size_t i = 0; i < permutation.getNumKeys(); i++) {
                    uint8_t keysIndex = permutation(i);
                    if (getKeySlice(keysIndex) == current.slice && getKeyLen(keysIndex) == current.size) {
                        return std::make_tuple(VALUE, getLV(keysIndex), keysIndex);
                    }
                }
            } else {    // 次のスライスがある場合(current layerにはvalueがないので下位ノードを辿るためのLinkを探す)
                for (size_t i = 0; i < permutation.getNumKeys(); i++) {
                    uint8_t keysIndex = permutation(i);
                    if (getKeySlice(keysIndex) == current.slice) {
                        if (getKeyLen(keysIndex) == BorderNode::key_len_has_suffix) {
                            // suffixの中を見る
                            BigSuffix *suffix = getKeySuffixes().get(keysIndex);
                            if (suffix != nullptr && suffix->isSame(key, key.cursor + 1)) {
                                return std::make_tuple(VALUE, getLV(keysIndex), keysIndex);
                            }
                        }

                        if (getKeyLen(keysIndex) == BorderNode::key_len_layer) {
                            return std::make_tuple(LAYER, getLV(keysIndex), keysIndex);
                        }
                        if (getKeyLen(keysIndex) == BorderNode::key_len_unstable) {
                            return std::make_tuple(UNSTABLE, LinkOrValue{}, 0);
                        }
                    }
                }
            }
            return std::make_tuple(NOTFOUND, LinkOrValue{}, 0);
        }

        uint64_t lowestKey() const {
            Permutation permutation = getPermutation();
            return getKeySlice(permutation(0));
        }

        // void connectPrevAndNext() const{}

        std::pair<size_t, bool> insertPoint() const {
            assert(getPermutation().isNotFull());
            for (size_t i = 0; i < ORDER - 1; i++) {
                uint8_t len = getKeyLen(i);
                if (len == 0) return std::make_pair(i, false);
                if (10 <= len && len <= 18) return std::make_pair(i, true); // CHECK: 10~18ってなんで？removed slotが再利用される場合はこっちに来るらしいんだけど(その場合insert側はv_insertを更新するらしい)
            }
            assert(false);
        }

        // void printNode() const{}

        // splitをするとき簡略化のためにsortを行う(BorderNode, permutationどっちも)
        void sort() {
            Permutation permutation = getPermutation();
            assert(permutation.isFull());
            assert(isLocked());
            assert(getSplitting());

            uint8_t temp_key_len[Node::ORDER - 1] = {};
            uint64_t temp_key_slice[Node::ORDER - 1] = {};
            LinkOrValue temp_lv[Node::ORDER - 1] = {};
            BigSuffix *temp_suffix[Node::ORDER - 1] = {};

            for (size_t i = 0; i < ORDER - 1; i++) {
                uint8_t trueIndex = permutation(i);
                temp_key_len[i] = getKeyLen(trueIndex);
                temp_key_slice[i] = getKeySlice(trueIndex);
                temp_lv[i] = getLV(trueIndex);
                temp_suffix[i] = getKeySuffixes().get(trueIndex);
            }
            // forが2回入っていて冗長に見えるけど全部そろえてからpermutationをいじった方が整合性保証的な意味で良い気がする
            for (size_t i = 0; i < ORDER - 1; i++) {
                setKeyLen(i, temp_key_len[i]);
                setKeySlice(i, temp_key_slice[i]);
                setLV(i, temp_lv[i]);
                getKeySuffixes().set(i, temp_suffix[i]);
            }
            setPermutation(Permutation::fromSorted(ORDER - 1));
        }

        // void markKeyRemoved(uint8_t i){}

        // bool isKeyRemoved(uint8_t i) const{}

        size_t findNextLayerIndex(Node *next_layer) const {
            assert(this->isLocked());
            for (size_t i = 0; i < ORDER - 1; i++) {
                if (getLV(i).next_layer == next_layer) return i;
            }
            assert(false);  // ここにはたどり着かないはず
        }
        
        inline uint8_t getKeyLen(size_t i) const {
            // return loadAcquire(key_len[i]); // TODO: wrapperの作成
            return key_len[i].load(std::memory_order_acquire);
        }
        
        // key_len[i]にkeyの長さをセットする
        inline void setKeyLen(size_t i, const uint8_t &len) {
            key_len[i].store(len, std::memory_order_release);
        }

        void resetKeyLen() {
            for (size_t i = 0; i < ORDER - 1; i++) setKeyLen(i, 0);
        }
        
        inline uint64_t getKeySlice(size_t i) const {
            // return loadAcquire(key_slice[i]); // TODO: wrapperの作成
            return key_slice[i].load(std::memory_order_acquire); 
        }
        
        inline void setKeySlice(size_t i, const uint64_t &slice) {
            key_slice[i].store(slice, std::memory_order_release);   // TODO: wrapperの作成
        }

        void resetKeySlice() {
            for (size_t i = 0; i < ORDER - 1; i++) setKeySlice(i, 0);
        }
        
        inline LinkOrValue getLV(size_t i) const {
            // return loadAcquire(lv[i]);   // TODO: wrapperの作成
            return lv[i].load(std::memory_order_acquire); 
        }
        // lv[i]にlv_をセットする
        inline void setLV(size_t i, const LinkOrValue &lv_) {
            lv[i].store(lv_, std::memory_order_release);    // TODO: wrapperの作成
        }

        void resetLVs() {
            for (size_t i = 0; i < ORDER - 1; i++) setLV(i, LinkOrValue{});
        }

        inline BorderNode *getNext() const {
            return next.load(std::memory_order_acquire);    // TODO: wrapperの作成
        }

        inline void setNext(BorderNode *next_) {
            next.store(next_, std::memory_order_release);    // TODO: wrapperの作成
        }

        inline BorderNode *getPrev() const {
            return prev.load(std::memory_order_acquire);    // TODO: wrapperの作成
        }
        
        inline void setPrev(BorderNode *prev_) {
            prev.store(prev_, std::memory_order_release);    // TODO: wrapperの作成
        }

        // inline bool CASNext(BorderNode *expected, BorderNode *desired){}
        
        inline KeySuffix &getKeySuffixes() {
            return key_suffixes;
        }

        // inline const KeySuffix& getKeySuffixes() const{} // CHECK: こっち何に使うんだ？
        
        inline Permutation getPermutation() const {
            // return loadAcquire(permutation);    // TODO: wrapperの作成
            return permutation.load(std::memory_order_acquire); 
        }

        inline void setPermutation(const Permutation &p) {
            permutation.store(p, std::memory_order_release);
        }



    private:
        std::array<std::atomic<uint8_t>, ORDER - 1> key_len = {};   // keyの長さは255まで
        std::atomic<Permutation> permutation;   // CHECK: permutation::sizeOne()をここで呼ぶことはできない(コピー代入をサポートしてないから)からborderNodeのコンストラクタでpermutationのコンストラクタを呼び出す
        std::array<std::atomic<uint64_t>, ORDER - 1> key_slice = {};
        std::array<std::atomic<LinkOrValue>, ORDER - 1> lv = {};    // Link or Value
        std::atomic<BorderNode*> next{nullptr};
        std::atomic<BorderNode*> prev{nullptr};
        KeySuffix key_suffixes = {};
};

std::pair<BorderNode*, Version> findBorder(Node *root, const Key &key) {
RETRY:
    Node *node = root;
    Version version = node->stableVersion();

    if (!version.is_root) {
        root = root->getParent();
        goto RETRY;
    }
DESCEND:
    if (node->getIsBorder()) return std::pair<BorderNode*, Version>(reinterpret_cast<BorderNode *>(node), version);
    InteriorNode *interior_node = reinterpret_cast<InteriorNode*>(node);
    Node *next_node = interior_node->findChild(key.getCurrentSlice().slice);
    Version next_version;
    if (next_node != nullptr) {
        next_version = next_node->stableVersion();
    } else {
        next_version = Version();   // CHECK: ここなんでコンストラクタ呼んでるんだ？先の条件で未定義を回避するためか？
    }
    // nodeがロックされていないならそのまま下のノードに降下していく
    if ((next_node->getVersion() ^ next_version) <= Version::has_locked) {
        assert(next_node != nullptr);
        node = next_node;
        version = next_version;
        goto DESCEND;
    }
    // validationを挟んでversionが更新されていないか確認、されてたらRootからRETRY
    Version validation_version = node->stableVersion();
    if (validation_version.v_split != version.v_split) goto RETRY;
    version = validation_version;
    goto DESCEND;
}