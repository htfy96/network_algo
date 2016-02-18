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
    auto start = t;

    int count = 0;
    for (;;)
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
            if (chrono::duration_cast<chrono::seconds>(tt-start).count() > 10)
                break;
        }

        Node n;
        n.set_id("A");
        n.set_imp(2.333);

        Node m;
        m.set_id("B");
        m.set_imp(6.666);

        Node c;
        c.set_id("C");
        c.set_imp(6.666);

        Node d;
        d.set_id("D");
        d.set_imp(6.666);

        Edge e;
        e.set_id("E");
        e.set_from("A");
        e.set_to("B");

        Edge e2;
        e2.set_id("E2");
        e2.set_from("C");
        e2.set_to("D");

        Edge e3;
        e3.set_id("E3");
        e3.set_from("D");
        e3.set_to("A");

        Edge e4;
        e4.set_id("E4");
        e4.set_from("C");
        e4.set_to("A");

        for (int j=0; j<3; ++j)
        {
            g.setNode(n);
            g.setNode(m);
            g.setNode(c);
            g.setNode(d);
            g.setEdge(e);
            g.setEdge(e2);
            g.setEdge(e3);
            g.setEdge(e4);
        }

        g.removeEdge("E2");
        g.removeNode("A");
        g.removeNode("B");
        g.removeNode("C");
        g.removeNode("D");
    }
    g.destroy();
    delete options.block_cache;

}
