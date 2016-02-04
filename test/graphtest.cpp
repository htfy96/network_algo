#include "gtest/gtest.h"
#include "graph.hpp"

TEST(GraphTest, nodeAddFind)
{
    netalgo::Graph<> g;
    g.addNode(netalgo::Node<>("1"));

    EXPECT_EQ(1, g.nodeSize());

    int size = g.nodeSize();
    for(int i=0; i<11; ++i)
    {
      auto it = g.addNode(netalgo::Node<>("3"));
      if (it.second)
        ++size;
      EXPECT_EQ(size, g.nodeSize());
    }

}
    
