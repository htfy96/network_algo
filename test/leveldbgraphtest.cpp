#include "gtest/gtest.h"
#include "backend/leveldbgraph.hpp"
#include <string>
#include <chrono>
#include "leveldbgraphtest.pb.h"
#include "graphdsl.hpp"

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

TEST(LevelDbGraphTest, LevelDbDeductionStepsTest)
{
    try
    {
        auto s = "select ()-[id=\"dsfsdf\"]-(c id=\"\")<--()-->(id=\"\") return c"_graphsql;
        using namespace netalgo;
        auto result = generateDeductionSteps(s);
        EXPECT_EQ(7u, result.size());

        EXPECT_EQ(1u, result[0].id);
        EXPECT_EQ(DeductionTrait::notConstrainted, result[0].constraint);
        EXPECT_TRUE(result[0].direct);

        EXPECT_EQ(2u, result[1].id);
        EXPECT_EQ(DeductionTrait::leftConstrained, result[1].constraint);
        EXPECT_TRUE(result[1].direct);

        EXPECT_EQ(6u, result[2].id);
        EXPECT_EQ(DeductionTrait::notConstrainted, result[2].constraint);
        EXPECT_TRUE(result[2].direct);

        EXPECT_EQ(0u, result[3].id);
        EXPECT_EQ(DeductionTrait::rightConstrained, result[3].constraint);
        EXPECT_FALSE(result[3].direct);

        EXPECT_EQ(3u, result[4].id);
        EXPECT_EQ(DeductionTrait::leftConstrained, result[4].constraint);
        EXPECT_FALSE(result[4].direct);

        EXPECT_EQ(5u, result[5].id);
        EXPECT_EQ(DeductionTrait::rightConstrained, result[5].constraint);
        EXPECT_FALSE(result[5].direct);

        EXPECT_EQ(4u, result[6].id);
        EXPECT_EQ(DeductionTrait::bothConstrained, result[6].constraint);
        EXPECT_FALSE(result[6].direct);
    } catch(netalgo::GraphSqlParseStateException& e)
    {
        cout << e.getNearbyChars() << endl;
        throw;
    }
}

