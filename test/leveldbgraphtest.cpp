#include "gtest/gtest.h"
#include "backend/leveldbgraph.hpp"
#include <string>
#include "leveldbgraphtest.pb.h"

using namespace std;

TEST(LevelDbGraphTest, LevelDbGraphBuild)
{
    using namespace netalgo;
    
    LevelDbGraph<Node, Edge> g("xxx.db");
}
    
