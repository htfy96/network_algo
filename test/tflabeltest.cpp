
#include "gtest/gtest.h"
#include "backend/leveldbgraph.hpp"
#include "algorithm/tflabel.hpp"
#include <cstddef>
#include <cstdint>
#include "leveldbgraphtest.pb.h"
TEST(TFLabelTest, Test1)
{
    using namespace netalgo;
    LevelDbGraph< Node, Edge > g("www2.db");
    g.destroy();
    TFLabel<LevelDbGraph < Node, Edge > > tf(g);
    tf.read();
    //std::exit(0);
    //std::cout << "fuck!!" << std::endl;
    tf.preprocess();
    tf.printDebugMessage();
    g.destroy();
}
    
