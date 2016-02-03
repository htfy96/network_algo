#include <string>
#include <memory>
#include <vector>
#include <map>

namespace netalgo
{
    struct Null {};

    template<typename NodeProperty = Null, typename NodeIndex=std::string, typename NodeLabel=std::string>
        struct Node
        {
            public:
                typedef NodeProperty nodePropertyType;
                typedef NodeIndex nodeIndexType;
                typedef NodeLabel nodeLabelType;

                NodeIndex index;
                std::vector<NodeLabel> labels;
                NodeProperty property;

                Node(const NodeIndex& pindex) : index(pindex) {}
        };

    template< typename EdgeProperty = Null, typename NodeIndex=std::string>
        struct Edge
        {
            public:
                typedef EdgeProperty edgePropertyType;
                typedef NodeIndex nodeIndexType;
                NodeIndex from, to;
                EdgeProperty property;
        };

    template<bool directed = true, typename NodeProperty=Null, typename EdgeProperty=Null, 
        typename NodeIndex=std::string, typename NodeLabel=std::string>
    class Graph
    {
        public:
            typedef NodeProperty nodePropertyType;
            typedef EdgeProperty edgePropertyType;
            typedef Node<NodeProperty, NodeIndex, NodeLabel> nodeType;
            typedef Edge<EdgeProperty, NodeIndex> edgeType;

            typedef NodeIndex nodeIndexType;

            typedef std::multimap<NodeIndex, edgeType> edgeMapType;
            typedef std::map<NodeIndex, nodeType> nodeMapType;

            typedef typename edgeMapType::iterator edgeMapIterator;
            typedef typename nodeMapType::iterator nodeMapIterator;

            Graph() = default;
            
            Graph(const Graph&) = default;

            Graph(Graph&&) = default;

            ~Graph() = default;

        private:
            edgeMapType edgeMap_;
            nodeMapType nodeMap_;

        public:
            //Node operations
            std::pair<nodeMapIterator,bool> addNode(nodeType&& node)
            {
                return nodeMap_.insert(make_pair(node.index, node));
            }
            std::pair<nodeMapIterator,bool> addNode(const nodeType& node)
            {
                return nodeMap_.insert(make_pair(node.index, node));
            }

            nodeMapIterator nodeMapEnd()
            {
                return nodeMap_.end();
            }

            typename nodeMapType::size_type nodeSize()
            {
                return nodeMap_.size();
            }

            nodeMapIterator getNode(const nodeIndexType& index)
            {
                return nodeMap_.find(index);
            }

    
    };

            
}
