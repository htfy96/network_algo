#include "gtest/gtest.h"
#include "backend/leveldbgraph.hpp"
#include <string>
#include <chrono>
#include "leveldbgraphtest.pb.h"

using namespace std;

TEST(LevelDbGraphTest, LevelDbGraphBuild)
{
    using namespace netalgo;
    LevelDbGraph<Node, Edge> g("xxx.db");
}

TEST(LevelDbGraphTest, LevelDbGraphAdd)
{
    using namespace netalgo;
    LevelDbGraph<Node, Edge> g("yyy.db");
    Node n;
    n.set_id("A");
    n.set_imp(2.333);

    Node m;
    m.set_id("B");
    m.set_imp(6.666);

    Edge e;
    e.set_id("E");
    e.set_from("A");
    e.set_to("B");

    g.setNode(n);
    g.setNode(m);
    g.setEdge(e);
    g.removeEdge("E");
    g.removeNode("A");
    g.removeNode("B");

    g.destroy();
}

TEST(LevelDbGraphTest, LevelDbGraphSpeedTest)
{
    using namespace netalgo;
    leveldb::Options options;
    options.block_cache= leveldb::NewLRUCache(1024*1024*32);

    auto t = chrono::high_resolution_clock::now();
    LevelDbGraph<Node, Edge> g("zzz.db");

    int count = 0;
    for (int i=0; i<100000; ++i)
    {
        ++count;
        if (!(count % 100))
        {
            auto tt = chrono::high_resolution_clock::now();
            if (chrono::duration_cast<chrono::milliseconds>(tt-t).count()>1000)
            {
                t = tt;
                cout << count << endl;
                count = 0;
            }
        }

        Node n;
        n.set_id("A");
        n.set_imp(2.333);

        Node m;
        m.set_id("B");
        m.set_imp(6.666);

        Edge e;
        e.set_id("E");
        e.set_from("A");
        e.set_to("B");

        g.setNode(n);
        g.setNode(m);
        g.setEdge(e);
        g.removeEdge("E");
        g.removeNode("A");
        g.removeNode("B");
    }
    g.destroy();

}
