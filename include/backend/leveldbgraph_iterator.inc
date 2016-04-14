#ifndef GRAPH_BACKEND_LEVELDBGRAPH_ITERATOR
#define GRAPH_BACKEND_LEVELDBGRAPH_ITERATOR


#include "debug.hpp"
#include "graphdsl.hpp"
#include "leveldbgraph_db_utility.inc"
#include "leveldbgraph_deduction.inc"
#include <type_traits>
#include <string>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <map>
#include <iostream>

namespace netalgo
{
    template<typename NodeType, typename EdgeType>
        struct LevelDbGraphResult
		{
			std::unordered_map<std::string, NodeType> nodes;
			std::unordered_map<std::string, EdgeType> edges;
			NodeType& getNode(const std::string& key) 
            {
                auto it = nodes.find(key);
                assert(it != nodes.end());
                return it->second;
            }
			EdgeType& getEdge(const std::string& key) 
            {
                auto it = edges.find(key);
                assert(it != edges.end());
                return it->second;
            }
		};

    template<typename NodeType, typename EdgeType>
        class LevelDbGraphIteratorBase
        {
            public:
                typedef typename std::decay<decltype(std::declval<NodeType>().id())>::type NodeIdType;
                typedef typename std::decay<decltype(std::declval<EdgeType>().id())>::type EdgeIdType;
                typedef std::set< EdgeIdType > inoutEdgesType;
                typedef LevelDbGraphResult<NodeType, EdgeType> value_type;
                typedef value_type& reference;
                typedef value_type* pointer;
            protected:
                GraphSqlSentence sql;
                DeductionStepsType deductionSteps;
                LevelDbGraphResult<NodeType, EdgeType> result;
                bool isEnd;
                std::vector< NodeIdType > nodesId, nextNodesId;
                std::vector< EdgeIdType > edgesId, nextEdgesId;
                leveldb::DB *db;
                explicit LevelDbGraphIteratorBase(leveldb::DB *dbP,
                            const GraphSqlSentence& gs):
                    db(dbP), sql(gs), deductionSteps(generateDeductionSteps(gs)),
                    isEnd(false) {}
                LevelDbGraphIteratorBase() : db(nullptr) {}
                virtual ~LevelDbGraphIteratorBase()
                {
                }
            protected:
                bool hasNodeIdFoundBefore(const NodeIdType &nodeId,
                            const std::size_t dedIdx);
                std::size_t findDedIdxById(const std::size_t queryid);
                template<typename T,
                    typename = typename std::enable_if< std::is_same<T, NodeType>::value ||
                        std::is_same<T, EdgeType>::value >::type >
                        bool compareProperties(const Properties &props,
                                    T& val);

                bool found = false;
                bool cached = false;
        };

    template<typename NodeType, typename EdgeType, bool isDirected>
        class LevelDbGraphIterator;

    template<typename NodeType, typename EdgeType>
        class LevelDbGraphIterator<NodeType, EdgeType, true> :
        protected LevelDbGraphIteratorBase<NodeType, EdgeType>
        {
            private:
                typedef LevelDbGraphIteratorBase<NodeType, EdgeType> BaseType;
            public:
                typedef typename BaseType::NodeIdType NodeIdType;
                typedef typename BaseType::EdgeIdType EdgeIdType;
                typedef typename BaseType::inoutEdgesType   inoutEdgesType;
                typedef typename BaseType::value_type value_type;
                typedef typename BaseType::reference reference;
                typedef typename BaseType::pointer pointer;
            protected:
                LevelDbGraph<NodeType, EdgeType, true>& graph;
			private:

                bool checkLeftConstrained(const std::size_t id);
				bool checkRightConstrained(const std::size_t id);

                bool isSelfConstrained(const std::size_t id);
                NodeIdType getNodeIdFromLeftEdge(const std::size_t nodeId);
                NodeIdType getNodeIdFromRightEdge(const std::size_t nodeId);

                bool findNextPossible(const int deductionIdx);
                void searchPossible(std::size_t dedId);

			public:
                LevelDbGraphIterator(LevelDbGraph<NodeType, EdgeType, true> &graphP, const GraphSqlSentence& gs);
                explicit LevelDbGraphIterator(LevelDbGraph<NodeType, EdgeType, true> &graphP);
                LevelDbGraphIterator(const LevelDbGraphIterator& other):
                    BaseType(other), graph(other.graph) {}

                LevelDbGraphIterator& operator++();
                reference operator*();
                pointer operator->();

                bool operator==(const LevelDbGraphIterator& other);
                bool operator!=(const LevelDbGraphIterator& other);

				friend LevelDbGraph<NodeType, EdgeType, true>;
                virtual ~LevelDbGraphIterator() {}
		};

        template<typename NodeType, typename EdgeType>
        class LevelDbGraphIterator<NodeType, EdgeType, false> :
        protected LevelDbGraphIteratorBase<NodeType, EdgeType>
        {
            private:
            typedef LevelDbGraphIteratorBase<NodeType, EdgeType> BaseType;
            public:
            typedef typename BaseType::NodeIdType NodeIdType;
            typedef typename BaseType::EdgeIdType EdgeIdType;
            typedef typename BaseType::inoutEdgesType   inoutEdgesType;
            typedef typename BaseType::value_type value_type;
            typedef typename BaseType::reference reference;
            typedef typename BaseType::pointer pointer;
            protected:
            LevelDbGraph<NodeType, EdgeType, false>& graph;
            private:

            bool checkLeftConstrained(const std::size_t id);
            bool checkRightConstrained(const std::size_t id);

            bool isSelfConstrained(const std::size_t id);
            NodeIdType getNodeIdFromLeftEdge(const std::size_t nodeId);
            NodeIdType getNodeIdFromRightEdge(const std::size_t nodeId);

            bool findNextPossible(const int deductionIdx);
            void searchPossible(std::size_t dedId);

            public:
            LevelDbGraphIterator(LevelDbGraph<NodeType, EdgeType, false> &graphP, const GraphSqlSentence& gs);
            explicit LevelDbGraphIterator(LevelDbGraph<NodeType, EdgeType, false> &graphP);
            LevelDbGraphIterator(const LevelDbGraphIterator& other):
            BaseType(other), graph(other.graph) {}

            LevelDbGraphIterator& operator++();
            reference operator*();
            pointer operator->();

            bool operator==(const LevelDbGraphIterator& other);
            bool operator!=(const LevelDbGraphIterator& other);

            friend LevelDbGraph<NodeType, EdgeType, false>;
            virtual ~LevelDbGraphIterator() {}
        };

    template<typename NodeType, typename EdgeType>
    LevelDbGraphIterator<NodeType, EdgeType, true>::
    LevelDbGraphIterator(LevelDbGraph<NodeType, EdgeType, true> &graphP, const GraphSqlSentence& gs) :
        graph(graphP),
        BaseType(graphP.db, gs)
    {
        LOGGER(trace, "DeductionStepsSize: {}", this->deductionSteps.size());
        this->nodesId.resize(gs.first.nodes.size());
        this->edgesId.resize(gs.first.edges.size());
        this->nextNodesId.resize(gs.first.nodes.size());
        this->nextEdgesId.resize(gs.first.edges.size());
        searchPossible(0);
        if (!this->found)
            this->isEnd = true;
        else
            this->isEnd = false;
    }

    template<typename NodeType, typename EdgeType>
    LevelDbGraphIterator<NodeType, EdgeType, false>::
    LevelDbGraphIterator(LevelDbGraph<NodeType, EdgeType, false> &graphP, const GraphSqlSentence& gs) :
    graph(graphP),
    BaseType(graphP.db, gs)
    {
        LOGGER(trace, "DeductionStepsSize: {}", this->deductionSteps.size());
        this->nodesId.resize(gs.first.nodes.size());
        this->edgesId.resize(gs.first.edges.size());
        this->nextNodesId.resize(gs.first.nodes.size());
        this->nextEdgesId.resize(gs.first.edges.size());
        searchPossible(0);
        if (!this->found)
        this->isEnd = true;
        else
        this->isEnd = false;
    }

    template<typename NodeType, typename EdgeType>
        LevelDbGraphIterator<NodeType, EdgeType, true>::
        LevelDbGraphIterator(LevelDbGraph<NodeType, EdgeType, true> &graphP):
            BaseType(),
            graph(graphP)
    {
        this->isEnd = true;
    }

    template<typename NodeType, typename EdgeType>
    LevelDbGraphIterator<NodeType, EdgeType, false>::
    LevelDbGraphIterator(LevelDbGraph<NodeType, EdgeType, false> &graphP):
    BaseType(),
    graph(graphP)
    {
        this->isEnd = true;
    }

    template<typename NodeType, typename EdgeType>
    auto LevelDbGraphIterator<NodeType, EdgeType, true>::
    operator++() -> LevelDbGraphIterator&
    {
        if (this->isEnd)
            throw std::runtime_error("++ on a past-end leveldbGraph iterator is invalid");
        if (!findNextPossible(this->nodesId.size() * 2 - 2))
            this->isEnd = true;
        return *this;
        //if (this->cached)
        //{
            //if (!findNextPossible(this->nodesId.size() * 2 - 2))
                //this->isEnd = true;
            //this->nodesId.swap(this->nextNodesId);
            //this->edgesId.swap(this->nextEdgesId);
            //return *this;
        //} else
        //{
            //this->cached = true;
            //if (!findNextPossible(this->nodesId.size() * 2 - 2))
            //{
                //this->isEnd = true;
                //std::cout << "next failed1" << std::endl;
                //return *this;
            //}
            //this->nodesId.swap(this->nextNodesId);
            //this->edgesId.swap(this->nextEdgesId);
            //if (!findNextPossible(this->nodesId.size() * 2 - 2))
                //this->isEnd = true;
            //this->nodesId.swap(this->nextNodesId);
            //this->edgesId.swap(this->nextEdgesId);
            //return *this;
        //}
    }

    template<typename NodeType, typename EdgeType>
    auto LevelDbGraphIterator<NodeType, EdgeType, false>::
    operator++() -> LevelDbGraphIterator&
    {
        if (this->isEnd)
        throw std::runtime_error("++ on a past-end leveldbGraph iterator is invalid");
        if (!findNextPossible(this->nodesId.size() * 2 - 2))
        this->isEnd = true;
        return *this;
        //if (!this->nextNodesId.empty())
        //{
            //if (!findNextPossible(this->nodesId.size() * 2 - 2))
            //this->isEnd = true;
            //this->nodesId.swap(this->nextNodesId);
            //this->edgesId.swap(this->nextEdgesId);
        //} else
        //{
            //assert(findNextPossible(this->nodesId.size() * 2 - 2));
            //this->nodesId.swap(this->nextNodesId);
            //this->edgesId.swap(this->nextEdgesId);
            //if (!findNextPossible(this->nodesId.size() * 2 - 2))
            //this->isEnd = true;
            //this->nodesId.swap(this->nextNodesId);
            //this->edgesId.swap(this->nextEdgesId);
        //}
    }

    template<typename NodeType, typename EdgeType>
    auto LevelDbGraphIterator<NodeType, EdgeType, true>::
    operator*() -> reference
    {
        if (this->isEnd)
            throw std::runtime_error("* on a past-end leveldbGraph iterator is invalid");
        std::unordered_set< std::string > returnName;
        for (const auto& item : this->sql.second.returnName)
        {
            returnName.insert(item);
        }
        for (std::size_t i=0; i < this->nodesId.size() * 2 - 1; ++i)
            if (isNode(i))
            {
                std::string nodeName = this->sql.first.nodes.at(getNodeIndex(i)).id;
                if (nodeName != "" && 
                            returnName.find(nodeName) !=
                            returnName.end())
                {
                    this->result.nodes[nodeName] = 
                                    graph.getNode(this->nodesId.at(getNodeIndex(i)));
                }
            } else
            {
                std::string edgeName = this->sql.first.edges.at(getEdgeIndex(i)).id;
                if (edgeName != "" &&
                            returnName.find(edgeName) !=
                            returnName.end())
                    this->result.edges[edgeName] = 
                                    graph.getEdge(this->edgesId.at(getEdgeIndex(i)));
            }
        return this->result;
    }

    template<typename NodeType, typename EdgeType>
    auto LevelDbGraphIterator<NodeType, EdgeType, false>::
    operator*() -> reference
    {
        if (this->isEnd)
            throw std::runtime_error("* on a past-end leveldbGraph iterator is invalid");
        std::unordered_set< std::string > returnName;
        for (const auto& item : this->sql.second.returnName)
        {
            returnName.insert(item);
        }
        for (std::size_t i=0; i < this->nodesId.size() * 2 - 1; ++i)
            if (isNode(i))
            {
                std::string nodeName = this->sql.first.nodes.at(getNodeIndex(i)).id;
                if (nodeName != "" && 
                            returnName.find(nodeName) !=
                            returnName.end())
                {
                    this->result.nodes[nodeName] = 
                                    graph.getNode(this->nodesId.at(getNodeIndex(i)));
                }
            } else
            {
                std::string edgeName = this->sql.first.edges.at(getEdgeIndex(i)).id;
                if (edgeName != "" &&
                            returnName.find(edgeName) !=
                            returnName.end())
                    this->result.edges[edgeName] = 
                                    graph.getEdge(this->edgesId.at(getEdgeIndex(i)));
            }
        return this->result;
    }

    template<typename NodeType, typename EdgeType>
    auto LevelDbGraphIterator<NodeType, EdgeType, true>::
    operator->() -> pointer
    {
        if (this->isEnd)
            throw std::runtime_error("-> on a past-end leveldbGraph iterator is invalid");
        return & (operator*());
    }

    template<typename NodeType, typename EdgeType>
    auto LevelDbGraphIterator<NodeType, EdgeType, false>::
    operator->() -> pointer
    {
        if (this->isEnd)
        throw std::runtime_error("-> on a past-end leveldbGraph iterator is invalid");
        return & (operator*());
    }

    template<typename NodeType, typename EdgeType>
    bool LevelDbGraphIterator<NodeType, EdgeType, true>::
    operator==(const LevelDbGraphIterator& other)
    {
        if (this->isEnd ^ other.isEnd) return false;
        if (this->isEnd && other.isEnd) return true;
        if (this->nodesId.size() != other.nodesId.size()) return false;
        if (this->edgesId.size() != other.edgesId.size()) return false;
        for (std::size_t i=0; i<this->nodesId.size(); ++i)
            if (other.nodesId[i] != this->nodesId[i])
                return false;
        for (std::size_t i=0; i<this->edgesId.size(); ++i)
            if (other.edgesId[i] != this->edgesId[i])
                return false;
        return true;
    }

    template<typename NodeType, typename EdgeType>
    bool LevelDbGraphIterator<NodeType, EdgeType, false>::
    operator==(const LevelDbGraphIterator& other)
    {
        if (this->isEnd ^ other.isEnd) return false;
        if (this->isEnd && other.isEnd) return true;
        if (this->nodesId.size() != other.nodesId.size()) return false;
        if (this->edgesId.size() != other.edgesId.size()) return false;
        for (std::size_t i=0; i<this->nodesId.size(); ++i)
        if (other.nodesId[i] != this->nodesId[i])
        return false;
        for (std::size_t i=0; i<this->edgesId.size(); ++i)
        if (other.edgesId[i] != this->edgesId[i])
        return false;
        return true;
    }

    template<typename NodeType, typename EdgeType>
    bool LevelDbGraphIterator<NodeType, EdgeType, true>::
    operator!=(const LevelDbGraphIterator& other)
    {
        return !(operator==(other));
    }

    template<typename NodeType, typename EdgeType>
    bool LevelDbGraphIterator<NodeType, EdgeType, false>::
    operator!=(const LevelDbGraphIterator& other)
    {
        return !(operator==(other));
    }

    template<typename NodeType, typename EdgeType>
    bool LevelDbGraphIterator<NodeType, EdgeType, true>::checkLeftConstrained(const std::size_t id)
    {
        if (isNode(id - 1)) // node - edge(*)
        {
            NodeIdType nodeId = this->nodesId.at(getNodeIndex(id - 1));
            // the edge dir in query
            EdgeDirection edgeDir = this->sql.first.edges.at(getEdgeIndex(id)).direction;

            inoutEdgesType outEdges = graph.getOutEdge(nodeId),
                           inEdges = graph.getInEdge(nodeId);

            bool result = true;
            if (edgeDir == EdgeDirection::bidirection)
                throw std::runtime_error("Cannot use -- in directed graph");
            if (edgeDir == EdgeDirection::prev)
                result &= (inEdges.find(this->edgesId.at(getEdgeIndex(id))) !=
                            inEdges.end());
            if (edgeDir == EdgeDirection::next)
                result &= (outEdges.find(this->edgesId.at(getEdgeIndex(id))) !=
                            outEdges.end());
            return result;
        } else // edge - node(*)
        {
            EdgeIdType edgeId = this->edgesId.at(getEdgeIndex(id - 1));
            NodeIdType nodeId = this->nodesId.at(getNodeIndex(id));
            std::string s;
            this->db->Get(leveldb::ReadOptions(), addSuffix(edgeId, edgeDataIdSuffix), &s);
            EdgeType e = strToDataByProtobuf< EdgeType >(s);
            //TODO: How to handle bidir edge?
            EdgeDirection edgeDir = this->sql.first.edges.at(getEdgeIndex(id)).direction; //edge Dir of actual edge
            bool result = false;
            if (edgeDir == EdgeDirection::bidirection)
                throw std::runtime_error("Cannot use -- in directed graph");
            if (edgeDir == EdgeDirection::next)
                result |= e.to() == nodeId;
            if (edgeDir == EdgeDirection::prev)
                result |= e.from() == nodeId;
            return result;
        }
    }

    template<typename NodeType, typename EdgeType>
    bool LevelDbGraphIterator<NodeType, EdgeType, false>::checkLeftConstrained(const std::size_t id)
    {
        if (isNode(id - 1)) // node - edge(*)
        {
            NodeIdType nodeId = this->nodesId.at(getNodeIndex(id - 1));
            // the edge dir in query
            EdgeDirection edgeDir = this->sql.first.edges.at(getEdgeIndex(id)).direction;

            inoutEdgesType outEdges = graph.getOutEdge(nodeId);

            if (edgeDir == EdgeDirection::bidirection)
                return outEdges.find(this->edgesId.at(getEdgeIndex(id))) != outEdges.end();
            else
                throw std::runtime_error("Cannot apply directed edge(<--/-->) in undirected graph");
            } else // edge - node(*)
            {
                EdgeIdType edgeId = this->edgesId.at(getEdgeIndex(id - 1));
                NodeIdType nodeId = this->nodesId.at(getNodeIndex(id));
                std::string s;
                this->db->Get(leveldb::ReadOptions(), addSuffix(edgeId, edgeDataIdSuffix), &s);
                EdgeType e = strToDataByProtobuf< EdgeType >(s);
                //TODO: How to handle bidir edge?
                EdgeDirection edgeDir = this->sql.first.edges.at(getEdgeIndex(id)).direction; //edge Dir of actual edge
                if (edgeDir == EdgeDirection::bidirection)
                    return e.from() == nodeId ||
                        e.to() == nodeId;
                else
                    throw std::runtime_error("Cannot apply directed edge(<--/-->) in undirected graph");
            }
        }

    template<typename NodeType, typename EdgeType>
    bool LevelDbGraphIterator<NodeType, EdgeType, true>::checkRightConstrained(const std::size_t id)
    {
        if (isNode(id + 1)) // edge(*) - node
        {
            NodeIdType nodeId = this->nodesId.at(getNodeIndex(id + 1));
            // the edge dir in query
            EdgeDirection edgeDir = this->sql.first.edges.at(getEdgeIndex(id)).direction;

            inoutEdgesType outEdges = graph.getOutEdge(nodeId),
                           inEdges = graph.getInEdge(nodeId);

            bool result = true;
            if (edgeDir == EdgeDirection::bidirection)
                throw std::runtime_error("Cannot apply -- in directed graph");
            if (edgeDir == EdgeDirection::next)
                result &= (inEdges.find(this->edgesId.at(getEdgeIndex(id))) !=
                            inEdges.end());
            if (edgeDir == EdgeDirection::prev)
                result &= (outEdges.find(this->edgesId.at(getEdgeIndex(id))) !=
                            outEdges.end());
            return result;
        } else // node(*) - edge
        {
            EdgeIdType edgeId = this->edgesId.at(getEdgeIndex(id + 1));
            NodeIdType nodeId = this->nodesId.at(getNodeIndex(id));
            std::string s;
            this->db->Get(leveldb::ReadOptions(), addSuffix(edgeId, edgeDataIdSuffix), &s);
            EdgeType e = strToDataByProtobuf< EdgeType >(s);
            //TODO: How to handle bidir edge?
            EdgeDirection edgeDir = this->sql.first.edges.at(getEdgeIndex(id + 1)).direction; //edge Dir of actual edge
            bool result = false;
            if (edgeDir == EdgeDirection::bidirection)
                throw std::runtime_error("Cannot apply -- in directed graph");
            if (edgeDir == EdgeDirection::prev)
                result |= e.to() == nodeId;
            if (edgeDir == EdgeDirection::next)
                result |= e.from() == nodeId;
            return result;
        }
    }
    
    template<typename NodeType, typename EdgeType>
    bool LevelDbGraphIterator<NodeType, EdgeType, false>::checkRightConstrained(const std::size_t id)
    {
        if (isNode(id + 1)) // edge(*) - node
        {
            NodeIdType nodeId = this->nodesId.at(getNodeIndex(id + 1));
            // the edge dir in query
            EdgeDirection edgeDir = this->sql.first.edges.at(getEdgeIndex(id)).direction;

            inoutEdgesType outEdges = graph.getOutEdge(nodeId);

            if (edgeDir == EdgeDirection::bidirection)
                return outEdges.find(this->edgesId.at(getEdgeIndex(id))) !=
                    outEdges.end();
            else
                throw std::runtime_error("Cannot apply -- in directed graph");

            } else // node(*) - edge
            {
                EdgeIdType edgeId = this->edgesId.at(getEdgeIndex(id + 1));
                NodeIdType nodeId = this->nodesId.at(getNodeIndex(id));
                std::string s;
                this->db->Get(leveldb::ReadOptions(), addSuffix(edgeId, edgeDataIdSuffix), &s);
                EdgeType e = strToDataByProtobuf< EdgeType >(s);
                //TODO: How to handle bidir edge?
                EdgeDirection edgeDir = this->sql.first.edges.at(getEdgeIndex(id + 1)).direction; //edge Dir of actual edge
                if (edgeDir == EdgeDirection::bidirection)
                    return e.from() == nodeId ||
                        e.to() == nodeId;
                else
                    throw std::runtime_error("Cannot apply -- in directed graph");
            }
        }

    template<typename NodeType, typename EdgeType>
    template<typename T,
        typename>
            bool LevelDbGraphIteratorBase<NodeType, EdgeType>::compareProperties(const Properties &props,
                        T& val)
            {
                for (const Property& prop : props)
                {
                    if (!reflectedCompare(&val, prop.name,
                                    prop.relationship, prop.value))
                        return false;
                }
                return true;
            }

    template<typename NodeType, typename EdgeType>
    bool LevelDbGraphIterator<NodeType, EdgeType, true>::isSelfConstrained(const std::size_t id)
    {
        if (isNode(id))
        {
            NodeIdType nodeId = this->nodesId.at(getNodeIndex(id));
            NodeType node = graph.getNode(nodeId);
            Properties queryProp = this->sql.first.nodes.at(getNodeIndex(id)).properties;
            return this->compareProperties(queryProp, node);
        } else
        {
            EdgeIdType edgeId = this->edgesId.at(getEdgeIndex(id));
            EdgeType edge = graph.getEdge(edgeId);
            Properties queryProp = this->sql.first.edges.at(getEdgeIndex(id)).properties;
            return this->compareProperties(queryProp, edge);
        }
    }

    template<typename NodeType, typename EdgeType>
    bool LevelDbGraphIterator<NodeType, EdgeType, false>::isSelfConstrained(const std::size_t id)
    {
        if (isNode(id))
        {
            NodeIdType nodeId = this->nodesId.at(getNodeIndex(id));
            NodeType node = graph.getNode(nodeId);
            Properties queryProp = this->sql.first.nodes.at(getNodeIndex(id)).properties;
            return this->compareProperties(queryProp, node);
        } else
        {
            EdgeIdType edgeId = this->edgesId.at(getEdgeIndex(id));
            EdgeType edge = graph.getEdge(edgeId);
            Properties queryProp = this->sql.first.edges.at(getEdgeIndex(id)).properties;
            return this->compareProperties(queryProp, edge);
        }
    }

    template<typename NodeType, typename EdgeType>
    bool LevelDbGraphIteratorBase<NodeType, EdgeType>::
    hasNodeIdFoundBefore(const NodeIdType &nodeId,
                const std::size_t dedIdx)
    {
        return std::find_if(deductionSteps.begin(),
                    deductionSteps.begin() + dedIdx,
                    [&](const netalgo::DeductionTrait &dt)
                    {
                    return isNode(dt.id) &&
                    this->nodesId.at(getNodeIndex(dt.id)) == nodeId;
                    }
                    ) != deductionSteps.end();
    }

    template<typename NodeType, typename EdgeType>
    std::size_t LevelDbGraphIteratorBase<NodeType, EdgeType>::
    findDedIdxById(const std::size_t queryid)
    {
        return std::find_if(deductionSteps.begin(),
                    deductionSteps.end(),
                    [&](const netalgo::DeductionTrait &dt)->bool
                    {
                    return dt.id == queryid;
                    }) - deductionSteps.begin();
    }

    template<typename NodeType, typename EdgeType>
    auto LevelDbGraphIterator<NodeType, EdgeType, true>::
    getNodeIdFromLeftEdge(const std::size_t nodeId) -> NodeIdType
    {
        EdgeType prevEdge = graph.getEdge(this->edgesId.at(getEdgeIndex(nodeId - 1)));
        netalgo::EdgeType queryEdge = this->sql.first.edges.at(getEdgeIndex(nodeId - 1));
        switch(queryEdge.direction)
        {
            case netalgo::EdgeDirection::next:
                return prevEdge.to();
                break;
            case netalgo::EdgeDirection::prev:
                return prevEdge.from();
                break;
            case netalgo::EdgeDirection::bidirection:
                throw std::runtime_error("Invalid -- in directed graph");
                break;
        }
    }

    template<typename NodeType, typename EdgeType>
    auto LevelDbGraphIterator<NodeType, EdgeType, false>::
    getNodeIdFromLeftEdge(const std::size_t nodeId) -> NodeIdType
    {
        EdgeType prevEdge = graph.getEdge(this->edgesId.at(getEdgeIndex(nodeId - 1)));
        netalgo::EdgeType queryEdge = this->sql.first.edges.at(getEdgeIndex(nodeId - 1));
        switch(queryEdge.direction)
        {
            case netalgo::EdgeDirection::next:
            case netalgo::EdgeDirection::prev:
                throw std::runtime_error("Invalid <--/--> in undirected graph");
                break;
            case netalgo::EdgeDirection::bidirection:
                break;
        }
        NodeIdType thisNodeId;
        if (this->hasNodeIdFoundBefore(prevEdge.from(), this->findDedIdxById(nodeId + 1)))
        thisNodeId = prevEdge.to();
        else
        thisNodeId = prevEdge.from();
        return thisNodeId;
    }

    template<typename NodeType, typename EdgeType>
    auto LevelDbGraphIterator<NodeType, EdgeType, true>::
    getNodeIdFromRightEdge(const std::size_t nodeId) -> NodeIdType
    {
        EdgeType nextEdge = graph.getEdge(this->edgesId.at(getEdgeIndex(nodeId + 1)));
        netalgo::EdgeType queryEdge = this->sql.first.edges.at(getEdgeIndex(nodeId + 1));
        switch(queryEdge.direction)
        {
            case netalgo::EdgeDirection::next:
                return nextEdge.to();
                break;
            case netalgo::EdgeDirection::prev:
                return nextEdge.from();
                break;
            case netalgo::EdgeDirection::bidirection:
                throw std::runtime_error("Invalid -- in directed graph");
                break;
        }
    }

    template<typename NodeType, typename EdgeType>
    auto LevelDbGraphIterator<NodeType, EdgeType, false>::
    getNodeIdFromRightEdge(const std::size_t nodeId) -> NodeIdType
    {
        EdgeType prevEdge = graph.getEdge(this->edgesId.at(getEdgeIndex(nodeId + 1)));
        netalgo::EdgeType queryEdge = this->sql.first.edges.at(getEdgeIndex(nodeId + 1));
        switch(queryEdge.direction)
        {
            case netalgo::EdgeDirection::next:
            case netalgo::EdgeDirection::prev:
            throw std::runtime_error("Invalid <--/--> in undirected graph");
            break;
            case netalgo::EdgeDirection::bidirection:
            break;
        }
        NodeIdType thisNodeId;
        if (this->hasNodeIdFoundBefore(prevEdge.from(), this->findDedIdxById(nodeId + 1)))
        thisNodeId = prevEdge.to();
        else
        thisNodeId = prevEdge.from();
        return thisNodeId;
    }

    template<typename NodeType, typename EdgeType>
    bool LevelDbGraphIterator<NodeType, EdgeType, true>::findNextPossible(const int deductionIdx)
    {
        if (deductionIdx < 0) return false;
        DeductionTrait d = this->deductionSteps[deductionIdx];
        if (d.direct) return false;

        std::size_t id = d.id;
        if (isNode(id))
        {
            switch(d.constraint)
            {
                case DeductionTrait::ConstraintType::leftConstrained:
                    do
                    {
                        if (!findNextPossible(deductionIdx - 1)) return false;
                        this->nodesId.at(getNodeIndex(id)) = getNodeIdFromLeftEdge(id);
                    } while (!isSelfConstrained(id));
                    break;
                    //leftConstrained
                case DeductionTrait::ConstraintType::rightConstrained:
                    do
                    {
                        if (!findNextPossible(deductionIdx - 1)) return false;
                        this->nodesId.at(getNodeIndex(id)) = getNodeIdFromRightEdge(id);
                    } while (!isSelfConstrained(id));
                    break;
                    //rightConstrained
                case DeductionTrait::ConstraintType::bothConstrained:
                    do
                    {
                        if (!findNextPossible(deductionIdx - 1)) return false;
                        this->nodesId.at(getNodeIndex(id)) = getNodeIdFromLeftEdge(id);
                    } while (!isSelfConstrained(id) ||
                                !checkRightConstrained(id)); //bothConstrained
                    break;
                case DeductionTrait::ConstraintType::notConstrainted:
                    {
                        std::unique_ptr<leveldb::Iterator> it ( this->db->NewIterator(leveldb::ReadOptions()) );
                        it->Seek(addSuffix(this->nodesId.at(getNodeIndex(id)), nodeDataIdSuffix));
                        LOGGER(trace, "In notConstrained Node condition, current node is {}, its validity is {}",
                            this->nodesId.at(getNodeIndex(id)), it->Valid());
                        LOGGER(trace, "Its key is {}", it->key().ToString());
                        it->Next();
                        if (it->Valid()) LOGGER(trace, "After Next() the key becomes {}", it->key().ToString());
                        for(;
                                    it->Valid();
                                    it->Next())
                        {
                            LOGGER(trace, "Iterating {}", it->key().ToString());
                            std::string key = it->key().ToString();
                            std::size_t pos = key.find(nodeDataIdSuffix);
                            if (pos != key.npos)
                            {
                                this->nodesId.at(getNodeIndex(id)) = key.substr(0, pos);
                                LOGGER(trace, "Find node {}", key);
                                if (isSelfConstrained(id)) break;
                            }
                        }
                        bool valid = it->Valid();
                        LOGGER(trace, "Validity = {}", valid);
                        return valid;
                        break;
                    } //notConstrainted
            } // switch(d.constraint)
        } else //!isNode
        {
            EdgeType e = graph.getEdge(this->edgesId.at(getEdgeIndex(id)));
            switch(d.constraint)
            {
                case netalgo::DeductionTrait::leftConstrained:
                    {
                        netalgo::EdgeType edgeQuery =
                            this->sql.first.edges.at(getEdgeIndex(id));
                        bool firsttime = true;
                        for(;;)
                        {
                            inoutEdgesType edgesSet;
                            if (edgeQuery.direction == netalgo::EdgeDirection::next ||
                                        edgeQuery.direction == netalgo::EdgeDirection::bidirection)
                                edgesSet = this->graph.getOutEdge(this->nodesId.at(getNodeIndex(id - 1)));
                            else
                                edgesSet = this->graph.getInEdge(this->nodesId.at(getNodeIndex(id - 1)));
                            if (firsttime)
                                for (auto it = edgesSet.find(e.id());
                                            it!=edgesSet.end();)
                                {
                                    ++it; if (it == edgesSet.end()) break;
                                    this->edgesId.at(getEdgeIndex(id)) = *it;
                                    if (isSelfConstrained(id)) return true;
                                }
                            else
                                for (auto it = edgesSet.begin();
                                            it != edgesSet.end(); ++it)
                                {
                                    this->edgesId.at(getEdgeIndex(id)) = *it;
                                    if (isSelfConstrained(id)) return true;
                                }
                            firsttime = false;
                            if (!findNextPossible(deductionIdx - 1)) return false;
                        }
                        break;
                    }

                case netalgo::DeductionTrait::rightConstrained:
                    {
                        netalgo::EdgeType edgeQuery =
                            this->sql.first.edges.at(getEdgeIndex(id));
                        bool firsttime = true;
                        for(;;)
                        {
                            inoutEdgesType edgesSet;
                            if (edgeQuery.direction == netalgo::EdgeDirection::prev ||
                                        edgeQuery.direction == netalgo::EdgeDirection::bidirection)
                                edgesSet = this->graph.getOutEdge(this->nodesId.at(getNodeIndex(id + 1)));
                            else
                                edgesSet = this->graph.getInEdge(this->nodesId.at(getNodeIndex(id + 1)));
                            if (firsttime)
                                for (auto it = edgesSet.find(e.id());
                                            it!=edgesSet.end();)
                                {
                                    ++it; if (it == edgesSet.end()) break;
                                    this->edgesId.at(getEdgeIndex(id)) = *it;
                                    if (isSelfConstrained(id)) return true;
                                }
                            else
                                for (auto it = edgesSet.begin();
                                            it != edgesSet.end(); ++it)
                                {
                                    this->edgesId.at(getEdgeIndex(id)) = *it;
                                    if (isSelfConstrained(id)) return true;
                                }
                            firsttime = false;
                            if (!findNextPossible(deductionIdx - 1)) return false;
                        }
                        break;
                    } //right constrained
                case netalgo::DeductionTrait::bothConstrained:
                    {
                        netalgo::EdgeType edgeQuery =
                            this->sql.first.edges.at(getEdgeIndex(id));
                        bool firsttime = true;
                        for (;;)
                        {
                            inoutEdgesType edgesSet1, edgesSet2;
                            if (edgeQuery.direction == netalgo::EdgeDirection::next ||
                                        edgeQuery.direction == netalgo::EdgeDirection::bidirection)
                                edgesSet1 = this->graph.getOutEdge(this->nodesId.at(getNodeIndex(id - 1)));
                            else
                                edgesSet1 = this->graph.getInEdge(this->nodesId.at(getNodeIndex(id - 1)));
                            if (edgeQuery.direction == netalgo::EdgeDirection::prev ||
                                        edgeQuery.direction == netalgo::EdgeDirection::bidirection)
                                edgesSet2 = this->graph.getOutEdge(this->nodesId.at(getNodeIndex(id + 1)));
                            else
                                edgesSet2 = this->graph.getInEdge(this->nodesId.at(getNodeIndex(id + 1)));
                            std::vector<EdgeIdType> intersectEdgesSet;
                            std::set_intersection(edgesSet1.begin(),
                                        edgesSet1.end(),
                                        edgesSet2.begin(),
                                        edgesSet2.end(),
                                        std::back_inserter(intersectEdgesSet)
                                        );
                            if (firsttime)
                                for (auto it = std::find(intersectEdgesSet.begin(),
                                                intersectEdgesSet.end(), e.id());
                                            it!=intersectEdgesSet.end();)
                                {
                                    ++it; if (it == intersectEdgesSet.end()) break;
                                    this->edgesId.at(getEdgeIndex(id)) = *it;
                                    if (isSelfConstrained(id)) return true;
                                }
                            else
                                for (auto it = intersectEdgesSet.begin();
                                            it != intersectEdgesSet.end(); ++it)
                                {
                                    this->edgesId.at(getEdgeIndex(id)) = *it;
                                    if (isSelfConstrained(id)) return true;
                                }
                            firsttime = false;
                            if (!findNextPossible(deductionIdx - 1)) return false;
                        }
                        break;
                    } //bothConstrained
                case netalgo::DeductionTrait::notConstrainted:
                    {
                        leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
                        for(it->Seek(addSuffix(e.id(),
                                            edgeDataIdSuffix));
                                    it->Valid();
                                    it->Next())
                        {
                            std::string key = it->key().ToString();
                            std::size_t pos = key.find(edgeDataIdSuffix);
                            if (pos != key.npos)
                            {
                                this->edgesId.at(getEdgeIndex(id)) = key.substr(0, pos);
                                if (isSelfConstrained(id)) break;
                            }
                        }
                        bool valid = it->Valid();
                        delete it;
                        return valid;
                        break;
                    }
            } // switch(d.constriant)
        } //!isNode
    } //findNextPossible

    template<typename NodeType, typename EdgeType>
    bool LevelDbGraphIterator<NodeType, EdgeType, false>::findNextPossible(const int deductionIdx)
    {
        if (deductionIdx < 0) return false;
        DeductionTrait d = this->deductionSteps[deductionIdx];
        if (d.direct) return false;

        std::size_t id = d.id;
        if (isNode(id))
        {
            switch(d.constraint)
            {
                case DeductionTrait::ConstraintType::leftConstrained:
                    do
                    {
                        if (!findNextPossible(deductionIdx - 1)) return false;
                        this->nodesId.at(getNodeIndex(id)) = getNodeIdFromLeftEdge(id);
                    } while (!isSelfConstrained(id));
                    break;
                    //leftConstrained
                case DeductionTrait::ConstraintType::rightConstrained:
                    do
                    {
                        if (!findNextPossible(deductionIdx - 1)) return false;
                        this->nodesId.at(getNodeIndex(id)) = getNodeIdFromRightEdge(id);
                    } while (!isSelfConstrained(id));
                    break;
                    //rightConstrained
                case DeductionTrait::ConstraintType::bothConstrained:
                    do
                    {
                        if (!findNextPossible(deductionIdx - 1)) return false;
                        this->nodesId.at(getNodeIndex(id)) = getNodeIdFromLeftEdge(id);
                    } while (!isSelfConstrained(id) ||
                                !checkRightConstrained(id)); //bothConstrained
                    break;
                case DeductionTrait::ConstraintType::notConstrainted:
                    {
                        leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
                        for(it->Seek(addSuffix(this->nodesId.at(getNodeIndex(id)),
                                            nodeDataIdSuffix));
                                    it->Valid();
                                    it->Next())
                        {
                            std::string key = it->key().ToString();
                            std::size_t pos = key.find(nodeDataIdSuffix);
                            if (pos != key.npos)
                            {
                                this->nodesId.at(getNodeIndex(id)) = key.substr(0, pos);
                                if (isSelfConstrained(id)) break;
                            }
                        }
                        bool valid = it->Valid();
                        delete it;
                        return valid;
                        break;
                    } //notConstrainted
            } // switch(d.constraint)
        } else //!isNode
        {
            EdgeType e = graph.getEdge(this->edgesId.at(getEdgeIndex(id)));
            switch(d.constraint)
            {
                case netalgo::DeductionTrait::leftConstrained:
                    {
                        netalgo::EdgeType edgeQuery =
                            this->sql.first.edges.at(getEdgeIndex(id));
                        bool firsttime = true;
                        for(;;)
                        {
                            inoutEdgesType edgesSet;
                            edgesSet = this->graph.getOutEdge(this->nodesId.at(getNodeIndex(id - 1)));
                            if (firsttime)
                                for (auto it = edgesSet.find(e.id());
                                            it!=edgesSet.end();)
                                {
                                    ++it; if (it == edgesSet.end()) break;
                                    this->edgesId.at(getEdgeIndex(id)) = *it;
                                    if (isSelfConstrained(id)) return true;
                                }
                            else
                                for (auto it = edgesSet.begin();
                                            it != edgesSet.end(); ++it)
                                {
                                    this->edgesId.at(getEdgeIndex(id)) = *it;
                                    if (isSelfConstrained(id)) return true;
                                }
                            firsttime = false;
                            if (!findNextPossible(deductionIdx - 1)) return false;
                        }
                        break;
                    }

                case netalgo::DeductionTrait::rightConstrained:
                    {
                        netalgo::EdgeType edgeQuery =
                            this->sql.first.edges.at(getEdgeIndex(id));
                        bool firsttime = true;
                        for(;;)
                        {
                            inoutEdgesType edgesSet;
                            edgesSet = this->graph.getOutEdge(this->nodesId.at(getNodeIndex(id + 1)));
                            if (firsttime)
                                for (auto it = edgesSet.find(e.id());
                                            it!=edgesSet.end();)
                                {
                                    ++it; if (it == edgesSet.end()) break;
                                    this->edgesId.at(getEdgeIndex(id)) = *it;
                                    if (isSelfConstrained(id)) return true;
                                }
                            else
                                for (auto it = edgesSet.begin();
                                            it != edgesSet.end(); ++it)
                                {
                                    this->edgesId.at(getEdgeIndex(id)) = *it;
                                    if (isSelfConstrained(id)) return true;
                                }
                            firsttime = false;
                            if (!findNextPossible(deductionIdx - 1)) return false;
                        }
                        break;
                    } //right constrained
                case netalgo::DeductionTrait::bothConstrained:
                    {
                        netalgo::EdgeType edgeQuery =
                            this->sql.first.edges.at(getEdgeIndex(id));
                        bool firsttime = true;
                        for (;;)
                        {
                            inoutEdgesType edgesSet1, edgesSet2;
                            edgesSet1 = this->graph.getOutEdge(this->nodesId.at(getNodeIndex(id - 1)));
                            edgesSet2 = this->graph.getOutEdge(this->nodesId.at(getNodeIndex(id + 1)));
                            std::vector<EdgeIdType> intersectEdgesSet;
                            std::set_intersection(edgesSet1.begin(),
                                        edgesSet1.end(),
                                        edgesSet2.begin(),
                                        edgesSet2.end(),
                                        std::back_inserter(intersectEdgesSet)
                                        );
                            if (firsttime)
                                for (auto it = std::find(intersectEdgesSet.begin(),
                                                intersectEdgesSet.end(), e.id());
                                            it!=intersectEdgesSet.end();)
                                {
                                    ++it; if (it == intersectEdgesSet.end()) break;
                                    this->edgesId.at(getEdgeIndex(id)) = *it;
                                    if (isSelfConstrained(id)) return true;
                                }
                            else
                                for (auto it = intersectEdgesSet.begin();
                                            it != intersectEdgesSet.end(); ++it)
                                {
                                    this->edgesId.at(getEdgeIndex(id)) = *it;
                                    if (isSelfConstrained(id)) return true;
                                }
                            firsttime = false;
                            if (!findNextPossible(deductionIdx - 1)) return false;
                        }
                        break;
                    } //bothConstrained
                case netalgo::DeductionTrait::notConstrainted:
                    {
                        leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
                        for(it->Seek(addSuffix(e.id(),
                                            edgeDataIdSuffix));
                                    it->Valid();
                                    it->Next())
                        {
                            std::string key = it->key().ToString();
                            std::size_t pos = key.find(edgeDataIdSuffix);
                            if (pos != key.npos)
                            {
                                this->edgesId.at(getEdgeIndex(id)) = key.substr(0, pos);
                                if (isSelfConstrained(id)) break;
                            }
                        }
                        bool valid = it->Valid();
                        delete it;
                        return valid;
                        break;
                    }
            } // switch(d.constriant)
        } //!isNode
    } //findNextPossible

    template<typename NodeType, typename EdgeType>
    void LevelDbGraphIterator<NodeType, EdgeType, true>::
    searchPossible(std::size_t dedId)
    {
        typedef netalgo::DeductionTrait::ConstraintType ConstraintType;
        if (this->found) return;
        if (dedId >= this->deductionSteps.size())
        {
            this->found = true;
            return;
        }
        DeductionTrait dt = this->deductionSteps.at(dedId);
        std::size_t id = dt.id;
        if (isNode(id))
        {
            netalgo::NodeType querynode = this->sql.first.nodes.at(getNodeIndex(id));
            if (dt.direct)
            {
                this->nodesId.at(getNodeIndex(id)) = getId(querynode.properties);
                if (!isSelfConstrained(id) ||
                            (isLeftContrained(dt.constraint) && !checkLeftConstrained(id)) ||
                            (isRightContrained(dt.constraint) && !checkRightConstrained(id))) return;
                else
                    searchPossible(dedId + 1);
                return;
            } // dt.direct
            else
            {
                switch(dt.constraint)
                {
                    case ConstraintType::leftConstrained:
                        this->nodesId.at(getNodeIndex(id)) = getNodeIdFromLeftEdge(id);
                        if (isSelfConstrained(id))
                            searchPossible(dedId + 1);
                        return;
                        break;
                    case ConstraintType::rightConstrained:
                        this->nodesId.at(getNodeIndex(id)) = getNodeIdFromRightEdge(id);
                        if (isSelfConstrained(id))
                            searchPossible(dedId + 1);
                        return;
                        break;
                    case ConstraintType::bothConstrained:
                        this->nodesId.at(getNodeIndex(id)) = getNodeIdFromLeftEdge(id);
                        if (this->nodesId[getNodeIndex(id)] == getNodeIdFromRightEdge(id) &&
                                    isSelfConstrained(id))
                            searchPossible(dedId + 1);
                        return;
                        break;
                    case ConstraintType::notConstrainted:
                        {
                            leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
                            for(it->SeekToFirst();
                                        it->Valid();
                                        it->Next())
                            {
                                std::string key = it->key().ToString();
                                LOGGER(trace, "Checking key {}", key);
                                std::size_t pos = key.find(nodeDataIdSuffix);
                                if (pos != key.npos)
                                {
                                    this->nodesId.at(getNodeIndex(id)) = key.substr(0, pos);
                                    if (isSelfConstrained(id))
                                    {
                                        searchPossible(dedId + 1);
                                        if (this->found) { delete it; return; }
                                    }
                                }
                            }
                            delete it;
                            return;
                            break;
                        }
                } //switch
            } // not Direct

        } // isNode
        else //!isNode
        {
            netalgo::EdgeType queryEdge = this->sql.first.edges.at(getEdgeIndex(id));
            if (dt.direct)
            {
                this->edgesId.at(getEdgeIndex(id)) = getId(queryEdge.properties);
                if (!isSelfConstrained(id) ||
                            (isLeftContrained(dt.constraint) && !checkLeftConstrained(id)) ||
                            (isRightContrained(dt.constraint) && !checkRightConstrained(id)))
                    return;
                else
                    searchPossible(dedId + 1);
                return;
            } else
            { // not direct
                switch(dt.constraint)
                {
                    case DeductionTrait::leftConstrained:
                        {
                            inoutEdgesType leftSet;
                            if (queryEdge.direction == EdgeDirection::next ||
                                        queryEdge.direction == EdgeDirection::bidirection)
                                leftSet = this->graph.getOutEdge(this->nodesId.at(getNodeIndex(id - 1)));
                            else
                                leftSet = this->graph.getInEdge(this->nodesId.at(getNodeIndex(id - 1)));
                            for (const auto& item : leftSet)
                            {
                                this->edgesId.at(getEdgeIndex(id)) = item;
                                if (isSelfConstrained(id))
                                {
                                    searchPossible(dedId + 1);
                                    if (this->found) return;
                                }
                            }
                            return;
                        } //case leftConstrained
                    case DeductionTrait::rightConstrained:
                        {
                            inoutEdgesType rightSet;
                            if (queryEdge.direction == EdgeDirection::prev ||
                                        queryEdge.direction == EdgeDirection::bidirection)
                                rightSet = this->graph.getOutEdge(this->nodesId.at(getNodeIndex(id + 1)));
                            else
                                rightSet = this->graph.getInEdge(this->nodesId.at(getNodeIndex(id + 1)));
                            for (const auto& item : rightSet)
                            {
                                this->edgesId.at(getEdgeIndex(id)) = item;
                                if (isSelfConstrained(id))
                                {
                                    searchPossible(dedId + 1);
                                    if (this->found) return;
                                }
                            }
                            return;
                        } //case rightConstrained
                    case DeductionTrait::bothConstrained:
                        {
                            inoutEdgesType leftSet, rightSet;
                            if (queryEdge.direction == EdgeDirection::next ||
                                        queryEdge.direction == EdgeDirection::bidirection)
                                leftSet = this->graph.getOutEdge(this->nodesId.at(getNodeIndex(id - 1)));
                            else
                                leftSet = this->graph.getInEdge(this->nodesId.at(getNodeIndex(id - 1)));
                            if (queryEdge.direction == EdgeDirection::prev ||
                                        queryEdge.direction == EdgeDirection::bidirection)
                                rightSet = this->graph.getOutEdge(this->nodesId.at(getNodeIndex(id + 1)));
                            else
                                rightSet = this->graph.getInEdge(this->nodesId.at(getNodeIndex(id + 1)));
                            std::vector<EdgeIdType> result;
                            std::set_intersection(leftSet.begin(), leftSet.end(),
                                        rightSet.begin(), rightSet.end(),
                                        std::back_inserter(result));
                            for (const auto &item : result)
                            {
                                this->edgesId[getEdgeIndex(id)] = item;
                                if (isSelfConstrained(id))
                                {
                                    searchPossible(dedId + 1);
                                    if (this->found) return;
                                }
                            }
                            return;
                        } //case bothConstrained
                    case DeductionTrait::notConstrainted:
                        {
                            leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
                            for(it->SeekToFirst();
                                        it->Valid();
                                        it->Next())
                            {
                                std::string key = it->key().ToString();
                                std::size_t pos = key.find(edgeDataIdSuffix);
                                if (pos != key.npos)
                                {
                                    this->edgesId.at(getEdgeIndex(id)) = key.substr(0, pos);
                                    if (isSelfConstrained(id))
                                    {
                                        searchPossible(dedId + 1);
                                        if (this->found) return;
                                    }
                                }
                            }
                            delete it;
                            return;
                            break;
                        }//case notConstrained

                } //switch(dt.constaint)
            } //not direct
        } // !isNode

    }

    template<typename NodeType, typename EdgeType>
    void LevelDbGraphIterator<NodeType, EdgeType, false>::
    searchPossible(std::size_t dedId)
    {
        typedef netalgo::DeductionTrait::ConstraintType ConstraintType;
        if (this->found) return;
        if (dedId >= this->deductionSteps.size())
        {
            this->found = true;
            return;
        }
        DeductionTrait dt = this->deductionSteps.at(dedId);
        std::size_t id = dt.id;
        if (isNode(id))
        {
            netalgo::NodeType querynode = this->sql.first.nodes.at(getNodeIndex(id));
            if (dt.direct)
            {
                this->nodesId.at(getNodeIndex(id)) = getId(querynode.properties);
                if (!isSelfConstrained(id) ||
                            (isLeftContrained(dt.constraint) && !checkLeftConstrained(id)) ||
                            (isRightContrained(dt.constraint) && !checkRightConstrained(id))) return;
                else
                    searchPossible(dedId + 1);
                return;
            } // dt.direct
            else
            {
                switch(dt.constraint)
                {
                    case ConstraintType::leftConstrained:
                        this->nodesId.at(getNodeIndex(id)) = getNodeIdFromLeftEdge(id);
                        if (isSelfConstrained(id))
                            searchPossible(dedId + 1);
                        return;
                        break;
                    case ConstraintType::rightConstrained:
                        this->nodesId.at(getNodeIndex(id)) = getNodeIdFromRightEdge(id);
                        if (isSelfConstrained(id))
                            searchPossible(dedId + 1);
                        return;
                        break;
                    case ConstraintType::bothConstrained:
                        this->nodesId.at(getNodeIndex(id)) = getNodeIdFromLeftEdge(id);
                        if (this->nodesId[getNodeIndex(id)] == getNodeIdFromRightEdge(id) &&
                                    isSelfConstrained(id))
                            searchPossible(dedId + 1);
                        return;
                        break;
                    case ConstraintType::notConstrainted:
                        {
                            leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
                            for(it->SeekToFirst();
                                        it->Valid();
                                        it->Next())
                            {
                                std::string key = it->key().ToString();
                                std::size_t pos = key.find(nodeDataIdSuffix);
                                if (pos != key.npos)
                                {
                                    this->nodesId.at(getNodeIndex(id)) = key.substr(0, pos);
                                    if (isSelfConstrained(id))
                                    {
                                        searchPossible(dedId + 1);
                                        if (this->found) return;
                                    }
                                }
                            }
                            delete it;
                            return;
                            break;
                        }
                } //switch
            } // not Direct

        } // isNode
        else //!isNode
        {
            netalgo::EdgeType queryEdge = this->sql.first.edges.at(getEdgeIndex(id));
            if (dt.direct)
            {
                this->edgesId.at(getEdgeIndex(id)) = getId(queryEdge.properties);
                if (!isSelfConstrained(id) ||
                            (isLeftContrained(dt.constraint) && !checkLeftConstrained(id)) ||
                            (isRightContrained(dt.constraint) && !checkRightConstrained(id)))
                    return;
                else
                    searchPossible(dedId + 1);
                return;
            } else
            { // not direct
                switch(dt.constraint)
                {
                    case DeductionTrait::leftConstrained:
                        {
                            inoutEdgesType leftSet;
                            leftSet = this->graph.getOutEdge(this->nodesId.at(getNodeIndex(id - 1)));
                            for (const auto& item : leftSet)
                            {
                                this->edgesId.at(getEdgeIndex(id)) = item;
                                if (isSelfConstrained(id))
                                {
                                    searchPossible(dedId + 1);
                                    if (this->found) return;
                                }
                            }
                            return;
                        } //case leftConstrained
                    case DeductionTrait::rightConstrained:
                        {
                            inoutEdgesType rightSet;
                            rightSet = this->graph.getOutEdge(this->nodesId.at(getNodeIndex(id + 1)));
                            for (const auto& item : rightSet)
                            {
                                this->edgesId.at(getEdgeIndex(id)) = item;
                                if (isSelfConstrained(id))
                                {
                                    searchPossible(dedId + 1);
                                    if (this->found) return;
                                }
                            }
                            return;
                        } //case rightConstrained
                    case DeductionTrait::bothConstrained:
                        {
                            inoutEdgesType leftSet, rightSet;
                            leftSet = this->graph.getOutEdge(this->nodesId.at(getNodeIndex(id - 1)));
                            rightSet = this->graph.getOutEdge(this->nodesId.at(getNodeIndex(id + 1)));
                            std::vector<EdgeIdType> result;
                            std::set_intersection(leftSet.begin(), leftSet.end(),
                                        rightSet.begin(), rightSet.end(),
                                        std::back_inserter(result));
                            for (const auto &item : result)
                            {
                                this->edgesId[getEdgeIndex(id)] = item;
                                if (isSelfConstrained(id))
                                {
                                    searchPossible(dedId + 1);
                                    if (this->found) return;
                                }
                            }
                            return;
                        } //case bothConstrained
                    case DeductionTrait::notConstrainted:
                        {
                            leveldb::Iterator *it = this->db->NewIterator(leveldb::ReadOptions());
                            for(it->SeekToFirst();
                                        it->Valid();
                                        it->Next())
                            {
                                std::string key = it->key().ToString();
                                std::size_t pos = key.find(edgeDataIdSuffix);
                                if (pos != key.npos)
                                {
                                    this->edgesId.at(getEdgeIndex(id)) = key.substr(0, pos);
                                    if (isSelfConstrained(id))
                                    {
                                        searchPossible(dedId + 1);
                                        if (this->found) return;
                                    }
                                }
                            }
                            delete it;
                            return;
                            break;
                        }//case notConstrained

                } //switch(dt.constaint)
            } //not direct
        } // !isNode

    }
}
#endif
