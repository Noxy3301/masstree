#include <gtest/gtest.h>

#include "../src/include/masstree.h"
#include "sample.h"
#include "gtest_util.h"

TEST(PermutationTest, getNumKeys){
	// Permutationの下位4bitを取得する
	Permutation p{};
	// [ 0| 1| 2|__|__|__|__|__|__|__|__|__|__|__|__| 3]
	p.body = 0b0000'0001'0010'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0011;
	auto num = p.getNumKeys();
	EXPECT_EQ(num, 3);
}

TEST(PermutationTest, setNumKeys){
	// Permutationの下位4bitを更新する
	Permutation p{};
	// [ 3| 1| 2|__|__|__|__|__|__|__|__|__|__|__|__| 3]
	p.body = 0b0011'0001'0010'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0011;
	p.setNumKeys(4);
	// [ 3| 1| 2|__|__|__|__|__|__|__|__|__|__|__|__| 4]
	EXPECT_EQ(p.getNumKeys(), 4);
	EXPECT_EQ(p.getKeyIndex(0), 3);
	EXPECT_EQ(p.getKeyIndex(1), 1);
	EXPECT_EQ(p.getKeyIndex(2), 2);
}

TEST(PermutationTest, getKeyIndex) {
	Permutation p{};
	// Permutationの特定のindexに対応するkeysIndexを取得する
	// index 	 : Permutation自体のindex
	// keysIndex : keyのindex
	// [ 3| 1| 2|__|__|__|__|__|__|__|__|__|__|__|__| 3]
	p.body = 0b0011'0001'0010'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0000'0011;
	EXPECT_EQ(p.getNumKeys(), 3);
	EXPECT_EQ(p.getKeyIndex(0), 3);
	EXPECT_EQ(p.getKeyIndex(1), 1);
	EXPECT_EQ(p.getKeyIndex(2), 2);
	// setKeyIndexで所定の位置にkeysIndexをセットする
	p.setKeyIndex(1, 4);
	p.setKeyIndex(2, 1);
	// [ 3| 4| 1|__|__|__|__|__|__|__|__|__|__|__|__| 3]
	EXPECT_EQ(p.getNumKeys(), 3);
	EXPECT_EQ(p.getKeyIndex(0), 3);
	EXPECT_EQ(p.getKeyIndex(1), 4);
	EXPECT_EQ(p.getKeyIndex(2), 1);
}

TEST(PermutationTest, util) {
	// PermutationのSyntax sugarのテスト(構文の省略した書き方)
	Permutation p{};
	p.setNumKeys(1);
	p.setKeyIndex(0, 3);
	// [ 3|__|__|__|__|__|__|__|__|__|__|__|__|__|__| 1]
	EXPECT_EQ(p(0), 3);
}

TEST(PermutationTest, searchLinkOrValueWithIndex) {
	// BorderNodeの中からkeyに対応するLink or Valueとkeyのindexを取得する
	BorderNode *node = new BorderNode;
	makeSampleBorderNode(node);
	Key key({1}, 2);	// key({slice, ...}, lastSliceSize)
	std::tuple<SearchResult, LinkOrValue, size_t> tuple = node->searchLinkOrValueWithIndex(key);
	EXPECT_EQ(std::get<2>(tuple), 4);

	Key key2({2}, 5);	// key({slice, ...}, lastSliceSize)
	std::tuple<SearchResult, LinkOrValue, size_t> tuple2 = node->searchLinkOrValueWithIndex(key2);
	EXPECT_EQ(std::get<2>(tuple2), 0);

	Key key3({1}, 1);	// key({slice, ...}, lastSliceSize)
	std::tuple<SearchResult, LinkOrValue, size_t> tuple3 = node->searchLinkOrValueWithIndex(key3);
	EXPECT_EQ(std::get<2>(tuple3), 3);
}

TEST(PermutationTest, insert) {
	// Permutationの(index:3)に(keysIndex:2)をinsertする
	Permutation permutation = Permutation::from({3, 4, 5, 0, 1});
	// [ 3| 4| 5| 0| 1|__|__|__|__|__|__|__|__|__|__| 5]
	permutation.insert(3, 2);
	// [ 3| 4| 5| 2| 0| 1|__|__|__|__|__|__|__|__|__| 6]
	EXPECT_EQ(permutation.getKeyIndex(3), 2);
	EXPECT_EQ(permutation.getKeyIndex(4), 0);
	EXPECT_EQ(permutation.getKeyIndex(5), 1);
}

// TEST(PermutationTest, removeIndex) {}