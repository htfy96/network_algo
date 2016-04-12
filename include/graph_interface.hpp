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
            static std::false_type sfinae_checkid(bool, U NodeType::* = &NodeType::id()) {}
        static std::true_type sfinae_checkid(int) {}
        static_assert(decltype(sfinae_checkid(0))::value, "NodeType must contain member function id");

        template<typename U>
            static std::false_type sfinae_checkedge(bool, U EdgeType::* = &EdgeType::id()) {}
        static std::true_type sfinae_checkedge(int) {}
        static_assert(decltype(sfinae_checkedge(0))::value, "EdgeType must contain member function id");

        template<typename U>
            static std::false_type sfinae_checkedgefrom(bool, U EdgeType::* = &EdgeType::from()) {}
        static std::true_type sfinae_checkedgefrom(int) {}
        static_assert(decltype(sfinae_checkedgefrom(0))::value, "EdgeType must contain member function from");

        template<typename U>
            static std::false_type sfinae_checkedgeto(bool, U EdgeType::* = &EdgeType::to) {}
        static std::true_type sfinae_checkedgeto(int) {}
        static_assert(decltype(sfinae_checkedgeto(0))::value, "EdgeType must contain member function to");

        static_assert(std::is_convertible<
                    decltype(std::declval<EdgeType>().from()),
                    decltype(std::declval<NodeType>().id())
                    >::value,
                    "EdgeType.from() must be convertible to NodeType.id()");

        static_assert(std::is_convertible<
                    decltype(std::declval<EdgeType>().to()),
                    decltype(std::declval<NodeType>().id())
                    >::value,
                    "EdgeType.to() must be convertible to NodeType.id()");

    public:
        GraphInterface() = default;
        GraphInterface(const GraphInterface&) = delete;
        GraphInterface& operator=(const GraphInterface&) = delete;
        GraphInterface(GraphInterface&&) = default;
        virtual ~GraphInterface() {}

        typedef std::vector<NodeType> NodesBundle;
        typedef std::vector<EdgeType> EdgesBundle;
        typedef std::pair<NodesBundle, EdgesBundle> ResultType;

        typedef typename std::remove_const<
            typename std::remove_reference<decltype(std::declval<NodeType>().id())>::type
            >::type
            NodeIdType;
        typedef typename std::remove_const<
            typename std::remove_reference<decltype(std::declval<EdgeType>().id())>::type
            >::type
            EdgeIdType;

        virtual void setNode(const NodeType&) = 0;
        virtual void setEdge(const EdgeType&) = 0;
        virtual void setNodesBundle(const NodesBundle&) = 0;
        virtual void setEdgesBundle(const EdgesBundle&) = 0;

        virtual void removeNode(const NodeIdType&) = 0;
        virtual void removeEdge(const EdgeIdType&) = 0;

        virtual void destroy() = 0;

};

}
#endif
