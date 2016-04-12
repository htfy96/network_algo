#include "diskset.hpp"
#include <iostream>
#include <algorithm>
#include "gtest/gtest.h"

TEST(DISKSETTEST, DISKSETTEST1)
{
    netalgo::DiskSet<int> ds("d.db");
    for(int i=20; i>=0; --i)
        ds.insert(i);
    int i = 0;
    for (auto it = ds.begin(); it!=ds.end(); ++it)
    {
        EXPECT_EQ(i, *it);
        ++i;
    }
    EXPECT_EQ(ds.end(), ds.find(23));
    auto itx = ds.find(15), ity = ds.find(16);
    EXPECT_NE(itx, ity);
    EXPECT_EQ(itx, itx);
}
