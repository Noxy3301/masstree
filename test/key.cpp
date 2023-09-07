#include <gtest/gtest.h>
#include <cstdint>

#include "../src/include/masstree.h"
#include "gtest_util.h"
#include "sample.h"
#include <iostream>

// TODO: この関数はtestじゃなくてsrcのどっかで定義する、というかkeyのコンストラクタに仕込んだ方がやりやすいか
// TODO: ASCII文字に対応しているけど、NULとかBSとか使わないやつは無効化処理しないと
std::pair<std::vector<uint64_t>, size_t> stringToUint64t(const std::string &key) {
    std::vector<uint64_t> slices;
    size_t lastSliceSize = 0;
    size_t index = 0;

    while (index < key.size()) {
        uint64_t slice = 0;
        size_t i = 0;
        for (; i < 8 && index < key.size(); i++, index++) {
            slice = (slice << 8) | static_cast<uint8_t>(key[index]);    // 8bit左シフトしてORを取ることでsliceにkey[index]を挿入する
        }
        slice <<= (8 - i) * 8;    // 残りの部分を0埋め
        slices.push_back(slice);
        lastSliceSize = i;
    }
    
    return std::make_pair(slices, lastSliceSize);
}

std::string uint64tToString(const std::vector<uint64_t> &slices, size_t lastSliceSize) {
    std::string result;
    // 最後のスライス以外の処理
    for (size_t i = 0; i < slices.size() - 1; i++) {
        for (int j = 7; j >= 0; j--) {  // jをsize_tにするとj=-1の時にオーバーフローが発生するよ！
            result.push_back(static_cast<char>((slices[i] >> (j * 8)) & 0xFF)); // AND 0xFFで下位8bitを取得する
        }
    }
    // 最後のスライスの処理(lastSliceSizeより後ろは0埋めされているので)
    for (size_t i = 0; i < lastSliceSize; i++) {
        result.push_back(static_cast<char>((slices.back() >> ((7 - i) * 8)) & 0xFF));
    }
    return result;
}

TEST(KeyTest, stringAndUint64tConversion) {
    // std::stringからkeyの形({slice, ...}, lastSliceSize)に変換する
    std::string str = "higaisya dura no tiwawa";
    std::pair<std::vector<uint64_t>, size_t> slices_lastSliceSize = stringToUint64t(str);
    std::cout << GTEST_COUT_INFO << " {";
    for (size_t i = 0; i < slices_lastSliceSize.first.size(); i++) {
        std::cout << slices_lastSliceSize.first[i] << (i != slices_lastSliceSize.first.size() - 1 ? ", " : "");
    } std::cout << "}, " << "lastSliceSize = " << slices_lastSliceSize.second << std::endl;
    // keyの形({slice, ...}, lastSliceSize)からstd::stringに変換する
    std::string str2 = uint64tToString(slices_lastSliceSize.first, slices_lastSliceSize.second);
    std::cout << GTEST_COUT_INFO << str2 << std::endl;
    EXPECT_EQ(str, str2); // str -> uint64t -> strなのでEQUALになるはず
}

TEST(KeyTest, remainLength) {
    // higaisya dura no tiwawa <-> {7523658320577788257, 2334119641000996463, 2338610067969368320}, lastSliceSize = 7
    std::pair<std::vector<uint64_t>, size_t> slices_lastSliceSize = stringToUint64t("higaisya dura no tiwawa");
    Key key(slices_lastSliceSize.first, slices_lastSliceSize.second);
    // 指定されたスライスの位置からの残りの長さをバイト数で返す、from=0なら8+8+7=23
    EXPECT_EQ(key.remainLength(0), 23);
    EXPECT_EQ(key.remainLength(1), 15);
}

TEST(KeyTest, equal) {
    // Keyの演算子オーバーロードの確認
    Key key1({ONE, TWO}, 7);
    Key key2({ONE, TWO}, 8);
    EXPECT_FALSE(key1 == key2); // lastSliceSizeが違うのでKey1とKey2はスライスが同じであっても別のキーとして認識される
}

TEST(KeyTest, lastSliceSize) {
    // KeyのコンストラクタでlastSliceSizeをセットする
    Key key({ONE, TWO}, 7);
    EXPECT_EQ(key.lastSliceSize, 7);
}

TEST(KeyTest, getCurrentSlice) {
    // key.next()とkey.back()でカーソル(現在のスライスを指す)を移動させる
    Key key({ONE, TWO, THREE, FOUR, FIVE, SIX}, 7);
    key.next();
    key.next();
    key.next();
    EXPECT_EQ(key.getCurrentSlice().slice, FOUR);
    key.back();
    key.back();
    EXPECT_EQ(key.getCurrentSlice().slice, TWO);
}