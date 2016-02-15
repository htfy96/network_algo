#include "gtest/gtest.h"
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
        EXPECT_EQ("a^b\xff", e.getNearbyChars());
    }
    
}
