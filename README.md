#Network Algorithm ![test status](https://travis-ci.org/htfy96/network_algo.svg?branch=master)

A fast, embedded network framework.

*Still under development. The following building steps won't work now*

 > **This project has been migrated to Gitlab for better buildbot and privacy protection.**: https://gitlab.com/htfy96/network_algo

##Build
```
git clone --recursive https://github.com/htfy96/network_algo.git
sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y
sudo apt-get -qq update
sudo apt-get install -y libmysqlclient-dev cmake libsqlite3-dev libleveldb-dev libprotobuf-dev protobuf-compiler libsnappy-dev
sudo apt-get install gcc-5 g++-5 -y //GCC 5+ is required
export CXX="g++-5" CC="gcc-5"

cd network_algo
mkdir build && cd build
cmake ..
make -j2
sudo make install
```

##Usage
###Define node and edge type
Save
```proto
message Node
{
    required string id = 1; //must be named id and has type string
    required double dummy = 2; //custom data
    //...
}
message Edge
{
    required string id = 1; //required
    required string from = 2; //required
    required string to = 3; //required
    required int32 dummy = 4; //custom data
    //...
}
```
to `graphproto.proto`. Then run `protoc graphproto.proto --cpp-out=/your-cpp-source-directory`,
   and it will generate `graphproto.pb.h` and `graphproto.pb.cc`.

###Use it in your program
```cpp
#include "graphproto.pb.h" // which defines class Node and Edge
#include <netalgo/include/leveldbgraph.hpp>
#include <netalgo/include/graphdsl.hpp>
// N --> M <-- K
using namespace netalgo;
using namespace std;
int main()
{
    LeveldbGraph<Node, Edge, true> graph("mygraph.db");
    //the third argument means "directed". true is the default value
    Node n; n.set_id("N"); n.set_dummy(2.33);
    Node m; m.set_id("M"); m.set_dummy(6.66);
    Node k; k.set_id("K"); k.set_dummy(-2.5);
    Edge nm; nm.set_id("NtoM"); nm.set_from("N"); nm.set_to("M"); nm.set_dummy(1);
    Edge km; km.set_id("KtoM"); km.set_from("K"); km.set_to("M"); km.set_dummy(2);
    graph.setNode(n); graph.setNode(m); graph.setNode(k); //setEdge can be used in a similar way
    vector<Edge> edgeVec; edgeVec.push_back(nm); edgeVec.push_back(km);
    graph.setEdgesBundle(edgeVec); //setNodesBundle

    auto start = graph.parse("match (a)-[e1]->(mid id="M")<-[e2]-(c) return a,e1,c"_graphsql); // return an iterator
    for (auto it=start; it!=graph.end(); ++it) //iterates over result set
    {
        cout << it->getNode("a").id() << " " << it->getNode("c").id() << endl; //N K
        cout << it->getEdge("e1").dummy() << endl; //1
        Node a = it->getNode("a");
        std::set<std::string> outEdges = graph.getOutEdges(a.id());
        for (auto &e: outEdges)
        {
            graph.removeEdge(e.id());
        }
    }

    cout << calcDensity(graph) << endl; //use existing algorithm
}
```

##LICENSE
GPLv3, except those with special notes in header.

 > This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 > This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 > You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
