#include "gtest/gtest.h"
#include "graph_interface.hpp"
#include <string>
using namespace std;

TEST(GraphInterfaceTest, GraphInterfaceTest1)
{
    using namespace netalgo;
    struct NodeType
    {
        string id_;
        int otherinfo;
        string id() { return id_; }
    };

    struct EdgeType
    {
        string id_;
        string from_;
        string to_;

        string id() { return id_; }
        string from() { return from_; }
        string to() { return to_; }
    };
    typedef GraphInterface<NodeType, EdgeType> GI;

    testing::StaticAssertTypeEq<string, GI::NodeIdType>();
    testing::StaticAssertTypeEq<string, GI::EdgeIdType>();
}

