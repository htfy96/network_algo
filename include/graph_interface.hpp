#ifndef GRAPH_INTERFACE_HPP
#define GRAPH_INTERFACE_HPP

#include "graphdsl.hpp"
#include <vector>
#include <utility>
#include <type_traits>
namespace netalgo
{

template<typename NodeType, typename EdgeType>
class GraphInterface
{
    private:
        static_assert(std::is_class<NodeType>::value, "NodeType must be a class type");
        static_assert(std::is_class<EdgeType>::value, "EdgeType must be a class type");

        template<typename U>
            static std::false_type sfinae_checkid(bool, U NodeType::* = &NodeType::id) {}
        static std::true_type sfinae_checkid(int) {}
        static_assert(decltype(sfinae_checkid(0))::type, "NodeType must contain member id");

        template<typename U>
            static std::false_type sfinae_checkedge(bool, U EdgeType::* = &EdgeType::id) {}
        static std::true_type sfinae_checkedge(int) {}
        static_assert(decltype(sfinae_checkedge(0))::type, "EdgeType must contain member id");

    public:
        GraphInterface();
        GraphInterface(const GraphInterface&) = delete;
        GraphInterface& operator=(const GraphInterface&) = delete;
        GraphInterface(GraphInterface&&) = default;
        virtual ~GraphInterface() {}

        typedef std::vector<NodeType> NodesBundle;
        typedef std::vector<EdgeType> EdgesBundle;
        typedef std::pair< NodesBundle, EdgesBundle> ResultType;

        typedef decltype(std::declval<NodeType>().id) NodeIdType;
        typedef decltype(std::declval<EdgeType>().id) EdgeIdType;

        virtual ResultType query(const GraphSqlSentence&) = 0;
        virtual void setNode(const NodeType&) = 0;
        virtual void setEdge(const EdgeType&) = 0;
        virtual void setNodesBundle(const NodesBundle&) = 0;
        virtual void setEdgesBundle(const EdgesBundle&) = 0;

        virtual void removeNode(const NodeIdType&) = 0;
        virtual void removeEdge(const EdgeIdType&) = 0;

};

}
#endif
