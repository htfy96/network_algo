#include "debug.hpp"
#include "gtest/gtest.h"
#include "backend/leveldbgraph.hpp"
#include <string>
#include <vector>
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

TEST(LevelDbGraphTest, LevelDbGraphQuery)
{
    using namespace netalgo;
    using namespace netalgo::impl;
    LevelDbGraph<Node, Edge> g("yyy.db", 4);
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

    Node c;
    c.set_id("C");
    c.set_imp(65.56);

    Edge e2;
    e2.set_id("E2");
    e2.set_from("A");
    e2.set_to("C");

    g.setNode(n);
    g.setNode(m);
    g.setNode(c);
    g.setEdge(e);
    g.setEdge(e2);

    auto state = "select (id=\"A\")-[e]->(b) return e,b"_graphsql;

    auto t = chrono::high_resolution_clock::now();
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
                LOGGER(debug, "Last minute {} tests were done", count);
                count = 0;
            }
            if (chrono::duration_cast<chrono::seconds>(tt-start).count() > 10)
                break;
        }
        EXPECT_EQ(string("\"A\""), state.first.nodes[0].properties[0].value);
        EXPECT_EQ("A", getId(state.first.nodes[0].properties));
        DeductionStepsType deductionSteps = generateDeductionSteps(state);
        std::size_t cnt = 0;
        for (auto it = g.query("select (id=\"A\")-[e]->(b) return e,b"_graphsql);
                    it !=g.end();
                    ++it)
        {
            if (0==cnt)
            {
                EXPECT_EQ(string("B"), it->getNode("b").id());
                EXPECT_EQ(string("E"), it->getEdge("e").id());
            }
            else
            if (1==cnt)
            {
                EXPECT_EQ(string("C"), it->getNode("b").id());
                EXPECT_EQ(string("E2"), it->getNode("e").id());
            }
            ++cnt;
        }
    }
    g.removeEdge("E");
    g.removeNode("A");
    g.removeNode("B");
    g.destroy();
}

TEST(LevelDbGraphTest, LevelDbDeductionStepsTest2)
{
    using namespace netalgo;
    using namespace netalgo::impl;
    auto sql = "select (a)-->(b) return a,b"_graphsql;
    auto ded = generateDeductionSteps(sql);
    EXPECT_EQ(0u, ded[0].id);
    EXPECT_FALSE(ded[0].direct);
    EXPECT_EQ(DeductionTrait::notConstrainted, ded[0].constraint);
    EXPECT_EQ(2u, ded[1].id);
    EXPECT_FALSE(ded[1].direct);
    EXPECT_EQ(DeductionTrait::notConstrainted, ded[1].constraint);
    EXPECT_EQ(1u, ded[2].id);
    EXPECT_FALSE(ded[2].direct);
    EXPECT_EQ(DeductionTrait::bothConstrained, ded[2].constraint);
}


TEST(LevelDbGraphTest, LevelDbGraphInsertTest)
{
    using namespace netalgo;
    LevelDbGraph<Node, Edge> g("kkk.db");
    g.destroy();

    auto start = chrono::high_resolution_clock::now();
    std::size_t cnt = 0;
    for (cnt=0; cnt<1000000; ++cnt)
    {
        int i = cnt;
        if (!(cnt % 1000))
            if (chrono::duration_cast< chrono::seconds >(
                            chrono::high_resolution_clock::now() - start
                            ).count() > 10)
                break;
        Node n;
        if (!(cnt % 10000)) LOGGER(debug, "Inserting {}", cnt);
        n.set_id(std::to_string(cnt));
        n.set_imp(cnt);
        g.setNode(n);
        Edge e;
        if (i > 0)
        {
            e.set_from(std::to_string(i-1));
            e.set_to(std::to_string(i));
            e.set_id(std::to_string(i));
            g.setEdge(e);
        }
    }
    auto ms = chrono::duration_cast<chrono::milliseconds>(
                chrono::high_resolution_clock::now() - start
                ).count();

    LOGGER(info, "{} inserts takes {}ms, {} OP per second",cnt, ms, cnt * 1000.0 / ms);

    start = chrono::high_resolution_clock::now();
    cnt = 0;
    LOGGER(debug, g.getNode("2").DebugString());
    for(auto it = g.query("select (a)-->(b) return a,b"_graphsql);
                it != g.end();
                ++it)
    {
        if (!(cnt % 10))
            if (chrono::duration_cast< chrono::seconds >(
                            chrono::high_resolution_clock::now() - start
                            ).count() > 10)
                break;
        LOGGER(debug, it->getNode("a").DebugString());
        LOGGER(debug, it->getNode("b").DebugString());
        EXPECT_EQ((int)cnt, atoi(it->getNode("a").id().c_str()));
        EXPECT_EQ((int)(cnt+1), atoi(it->getNode("b").id().c_str()));
        ++cnt;
    }
    ms = chrono::duration_cast<chrono::milliseconds>(
                chrono::high_resolution_clock::now() - start
                ).count();

    LOGGER(info, "{} query takes {}ms, {} OP per second",cnt, ms, cnt * 1000.0 / ms);
    g.destroy();
}

TEST(LevelDbGraphTest, LevelDbUndirectedGraphTest)
{
    using namespace netalgo;
    LevelDbGraph<Node, Edge, false> g("uuu.db");
    Node a,b,c;
    a.set_id("a");
    b.set_id("b");
    c.set_id("c");

    a.set_imp(1);
    b.set_imp(2);
    c.set_imp(3);

    Edge e1, e2;
    e1.set_id("a->b");
    e1.set_from("a");
    e1.set_to("b");

    e2.set_id("c->a");
    e2.set_from("c");
    e2.set_to("a");

    vector<Node> vn;
    vn.push_back(a); vn.push_back(b); vn.push_back(c);
    g.setNodesBundle(vn);

    g.setEdge(e1); g.setEdge(e2);

}



TEST(LevelDbGraphTest, LevelDbGraphSpeedTest)
{
    using namespace netalgo;
    auto t = chrono::high_resolution_clock::now();
    LevelDbGraph<Node, Edge> g("zzz.db", 128);
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
                LOGGER(debug, "Last second {} tests were done", count);
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

}

TEST(LevelDbGraphTest, LevelDbDeductionStepsTest)
{
    try
    {
        auto s = "select ()-[id=\"dsfsdf\"]-(c id=\"\")<--()-->(id=\"\") return c"_graphsql;
        using namespace netalgo;
        using namespace netalgo::impl;
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
        LOGGER(critical, e.getNearbyChars());
        throw;
    }
}

