#include "gtest/gtest.h"
#include "debug.hpp"
#include <iostream>
#include <utility>
#include <sstream>
#include "graphdsl.cpp"
using namespace std;

TEST(GraphDSLTest, TokenTest1)
{
    istringstream is("(abc :abc)");
    using namespace HIDDEN;
    EXPECT_EQ(token( token::leftparen, "(" ), getToken(is));
    EXPECT_EQ(token(token::identifier, "abc"), getToken(is));
    EXPECT_EQ(token(token::identifier, ":abc"), getToken(is));
    EXPECT_EQ(token(token::rightparen, ")"), getToken(is));
}

TEST(GraphDSLTest, TokenTest2)
{
    istringstream is("(A_1 :person data>100)-[k len<-10]->(B)");
    using namespace HIDDEN;
    EXPECT_EQ(token( token::leftparen, "(" ), getToken(is));
    EXPECT_EQ(token(token::identifier, "A_1"), getToken(is));
    EXPECT_EQ(token(token::identifier, ":person"), getToken(is));
    EXPECT_EQ(token(token::identifier, "data"), getToken(is));
    EXPECT_EQ(token(token::greater, ">"), getToken(is));
    EXPECT_EQ(token(token::number, "100"), getToken(is));
    EXPECT_EQ(token(token::rightparen, ")"), getToken(is));
    EXPECT_EQ(token(token::dash, "-"), getToken(is));
    EXPECT_EQ(token(token::leftbracket, "["), getToken(is));
    EXPECT_EQ(token(token::identifier, "k"), getToken(is));
    EXPECT_EQ(token(token::identifier, "len"), getToken(is));
    EXPECT_EQ(token(token::smaller, "<"), getToken(is));
    EXPECT_EQ(token(token::number, "-10"), getToken(is));
    EXPECT_EQ(token(token::rightbracket, "]"), getToken(is));
    EXPECT_EQ(token(token::dash, "-"), getToken(is));
    EXPECT_EQ(token(token::greater, ">"), getToken(is));
    EXPECT_EQ(token(token::leftparen, "("), getToken(is));
    EXPECT_EQ(token(token::identifier, "B"), getToken(is));
    EXPECT_EQ(token(token::rightparen, ")"), getToken(is));
}

TEST(GraphDSLTest, InvalidTokenTest)
{
    istringstream is("a^b");
    using namespace HIDDEN;
    try
    {
        getToken(is);
        token t = getToken(is);
    } catch(netalgo::GraphSqlParseStateException& e)
    {
        EXPECT_STREQ(e.what(), "The first character of identifier should be a letter or : or _");
        EXPECT_EQ("a^b", e.getNearbyChars());
    }
    
}

TEST(GraphDSLTest, SentenceTest1)
{
    using namespace netalgo;
    GraphSqlSentence s = "select (a len>3)--(len=2) return a"_graphsql;
    auto & select = s.first;
    EXPECT_EQ(2ul, select.nodes.size());
    EXPECT_EQ(1ul, select.edges.size());
    auto &n1 = *select.nodes.begin();
    EXPECT_EQ("a", n1.id);
    EXPECT_EQ(1ul, n1.properties.size());
    EXPECT_EQ("len", n1.properties.begin()->name);
    EXPECT_EQ(Relationship::greater, n1.properties.begin()->relationship);
    EXPECT_EQ("3", n1.properties.begin()->value);

    auto& e1 = *select.edges.begin();
    EXPECT_EQ(0ul, e1.properties.size());
    EXPECT_EQ(EdgeDirection::bidirection, e1.direction);

    auto& n2 = *(select.nodes.begin() + 1);
    EXPECT_EQ("", n2.id);
    EXPECT_EQ("len", n2.properties.begin()->name);

    auto& ret = s.second;
    EXPECT_EQ(1ul, ret.returnName.size());
    EXPECT_EQ("a", *ret.returnName.begin());

}
