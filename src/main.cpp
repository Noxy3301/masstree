#include <iostream>
#include <vector>
#include <string>
#include "include/masstree.h"

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

int main() {
    Masstree masstree;
    GarbageCollector gc;

    std::vector<std::string> keys {
        "another super long string but even longer than the previous one 1234567890 abcdefghijklmnopqrstuvwxyz", // slices: {7020671414875550240, 8319679467645594735, 7955362947020253550, 7431047641760294262, 7308814895366956901, 8223700911238553716, 7522454419919959657, 8031452093630801184, 3544952156018063160, 4120829261678601317, 7378981316436126829, 7957702699140740213, 8536424081837260800}, lastSliceSize = 5
        "apple",                                                                                                 // slices: {7021235429923880960}, lastSliceSize = 5
        "banana",                                                                                                // slices: {7089068653200605184}, lastSliceSize = 6
        "cherry",                                                                                                // slices: {7163086749553983488}, lastSliceSize = 6
        "date",                                                                                                  // slices: {7233190453674246144}, lastSliceSize = 4
        "elderberry",                                                                                            // slices: {7308326682188998002, 8248624192505774080}, lastSliceSize = 2
        "fig",                                                                                                   // slices: {7379542714120929280}, lastSliceSize = 3
        "grape",                                                                                                 // slices: {7454127468610322432}, lastSliceSize = 5
        "honeydew",                                                                                              // slices: {7525354884466763127}, lastSliceSize = 8
        "kiwi",                                                                                                  // slices: {7739848727468179456}, lastSliceSize = 4
        "lemon",                                                                                                 // slices: {7810769454098284544}, lastSliceSize = 5
        "mango",                                                                                                 // slices: {7881702213398036480}, lastSliceSize = 5
        "nectarine",                                                                                             // slices: {7954873668322093422, 7277816997830721536}, lastSliceSize = 1
        "orange",                                                                                                // slices: {8030588212363984896}, lastSliceSize = 6
        "papaya",                                                                                                // slices: {8097877168939401216}, lastSliceSize = 6
        "quince",                                                                                                // slices: {8175556621395886080}, lastSliceSize = 6
        "raspberry",                                                                                             // slices: {8241995719589065330, 8718968878589280256}, lastSliceSize = 1
        "strawberry",                                                                                            // slices: {8319400174600480114, 8248624192505774080}, lastSliceSize = 2
        "super long string with multiple slices and layers 1234567890",                                          // slices: {8319679467645594735, 7955362947020253550, 7431070679969570925, 8461265796128859424, 8317138487471186017, 7954518491707041138, 8295684605293638966, 3978993149103046656}, lastSliceSize = 4
        "tangerine",                                                                                             // slices: {8386105371503257966, 7277816997830721536}, lastSliceSize = 1
        "watermelon",                                                                                            // slices: {8602284742314648940, 8029355185648173056}, lastSliceSize = 2
        "xigua",                                                                                                 // slices: {8676579910942195712}, lastSliceSize = 5
        "yellow fruit",                                                                                          // slices: {8747517064219402342, 8247614239536054272}, lastSliceSize = 4
        "zucchini"                                                                                               // slices: {8824068323507007081}, lastSliceSize = 8
    };

    // 通常のスライスとレイヤーをまたがるスライスを使用する
    for (size_t i = 0; i < keys.size(); i++) {
        auto slices_lastSliceSize = stringToUint64t(keys[i]);
        Key key(slices_lastSliceSize.first, slices_lastSliceSize.second);
        masstree.put(key, new Value{static_cast<int>(i)}, gc);
    }

    // 特定の範囲のキーに対してスキャンを実行
    auto left_slices_lastSliceSize = stringToUint64t("date");
    Key left_key(left_slices_lastSliceSize.first, left_slices_lastSliceSize.second);
    auto right_slices_lastSliceSize = stringToUint64t("yellow fruit");
    Key right_key(right_slices_lastSliceSize.first, right_slices_lastSliceSize.second);
    std::vector<std::pair<Key, Value*>> results;
    masstree.scan(left_key, true, right_key, false, results);

    // スキャンの結果を出力
    std::cout << "Scan results:" << std::endl;
    for (const auto& pair : results) {
        std::string key_str = uint64tToString(pair.first.slices, pair.first.lastSliceSize);
        std::cout << "Key: " << key_str << ", Value: " << pair.second->getBody() << std::endl;
    }

    return 0;
}