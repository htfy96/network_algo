#include "gtest/gtest.h"
#include "typedmrumap.hpp"
#include <vector>
#include <iostream>
#include <set>
#include <cstdlib>
#include <chrono>
#include <utility>

TEST(TypedMRUMapTest, getSizeTest)
{
    using namespace netalgo::impl;
    std::vector<int> v;
    v.reserve(42);

    EXPECT_EQ(v.capacity(), getSize(v));

    std::set<int> s;
    s.insert(1); s.insert(2); s.insert(3); s.insert(4);
    EXPECT_EQ(s.size(), getSize(s));

    struct SimpleStructure
    {
        int a;
        bool b;
    };

    EXPECT_EQ(1u, getSize(SimpleStructure()));
}

namespace
{

    template<typename FT>
        void speedTest(const FT& f)
        {
            using namespace std;
            auto startTime = chrono::high_resolution_clock::now();
            auto lastTime = startTime;

            std::size_t opCount = 0;
            for (int cnt = 0;; ++cnt)
            {
                ++opCount;
                if (0== (cnt % 100))
                {
                    auto currentTime = chrono::high_resolution_clock::now();
                    if (chrono::duration_cast< chrono::seconds >(
                                    currentTime - startTime).count() > 10)
                        break;
                    if (chrono::duration_cast< chrono::microseconds >(
                                    currentTime - lastTime).count() > 1000000)
                    {
                        cout << "In the last second "<<opCount <<" operations were done" << endl;
                        opCount = 0;
                        lastTime = currentTime;
                    }
                }
                f();
            }
        }

}

TEST(TypedMRUMapTest, InsertSpeedTest)
{
    using namespace std;
    netalgo::impl::TypedMRUMap<int, int> m(1024);

    cout << "Insert Speed Test" << endl;
    speedTest([&]() { m.insert(make_pair(rand(), rand())); });
}

TEST(TypedMRUMapTest, MixedOpSpeedTest)
{
    using namespace std;
    netalgo::impl::TypedMRUMap<int, int> m(1024);

    cout << "MixedOp Speed Test" << endl;
    speedTest([&] () {
                m.insert(make_pair( rand() % 4096, rand() % 4096));
                for(int i=0; i<7; ++i)
                {
                m.find(rand() % 4096);
                }
            });
}

    



