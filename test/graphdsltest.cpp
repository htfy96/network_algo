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
        EXPECT_EQ(e.getNearbyChars(), "a^b\xff");
    }
    
}
