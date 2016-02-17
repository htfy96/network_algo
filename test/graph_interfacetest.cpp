#include "gtest/gtest.h"
#include "graph_interface.hpp"
#include <string>
using namespace std;

TEST(GraphInterfaceTest, GraphInterfaceTest1)
{
    using namespace netalgo;
    struct NodeType
    {
        string id;
        int otherinfo;
    };

    struct EdgeType
    {
        const char* id;
        double x;
    };
    typedef GraphInterface<NodeType, EdgeType> GI;

    testing::StaticAssertTypeEq<string, GI::NodeIdType>();
    testing::StaticAssertTypeEq<const char*, GI::EdgeIdType>();
}
    
