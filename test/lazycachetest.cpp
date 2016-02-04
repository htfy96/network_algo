#include "gtest/gtest.h"
#include <iostream>
#include <utility>
#include "lazycache.hpp"
using namespace std;

TEST(LazyCacheTest, LazyCacheTest1)
{
    auto lam = [](const int i) { std::cout << i << std::endl; return std::make_pair(i,i); };
    netalgo::Lazymap<int, int> lm( "test.db", lam, netalgo::singleToMulti<int, int>(lam));

    lm.insertToMem(make_pair(2,3),1);
    lm.insertToMem(make_pair(3,4), 42);
    EXPECT_EQ(42, lm.getHitCount(3));
}
