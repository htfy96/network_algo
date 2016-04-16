#ifndef BACKEND_LEVELDBGRAPH_HPP
#define BACKEND_LEVELDBGRAPH_HPP

#include "debug.hpp"
#include "graph_interface.hpp"
#include "utility.hpp"
#include "typedmrumap.hpp"
#include "reflection.hpp"

#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <leveldb/cache.h>

#include <string>
#include <cassert>
#include <exception>
#include <iostream>
#include <type_traits>
#include <sstream>
#include <memory>
#include <cstring>
#include <iterator>
#include <vector>
#include <algorithm>

#include "leveldbgraph_db_utility.inc"

namespace netalgo
{
    template<typename NodeType, typename EdgeType, bool isDirected = true>
        class LevelDbGraphIterator;

    namespace
    {
        template<typename NodeType, typename EdgeType>
            class LevelDbGraphBase : public GraphInterface<NodeType, EdgeType>
            {
                typedef GraphInterface<NodeType, EdgeType> InterfaceType;
                static_assert( std::is_base_of<google::protobuf::Message, NodeType>::value &&
                            std::is_base_of<google::protobuf::Message, EdgeType>::value,
                            "NodeType and EdgeType must derive from protobuf::Message");
                static_assert(std::is_convertible< decltype(std::declval<NodeType>().id()),
                            const std::string >::value, "NodeType.id() must be string");
                static_assert(std::is_convertible< decltype(std::declval<EdgeType>().id()),
                            const std::string >::value, "EdgeType.id() must be string");
                static_assert(std::is_convertible<decltype(std::declval<EdgeType>().from()),
                            const std::string>::value, "EdgeType.from() must be string");
                static_assert(std::is_convertible<decltype(std::declval<EdgeType>().to()),
                            const std::string>::value, "EdgeType.to() must be string");
                protected:
                    leveldb::DB* db;
                    leveldb::Options options;
                    const std::string filename_;
                    const std::size_t cacheSize_;
                public:
                    explicit LevelDbGraphBase(const std::string& filename);
                    explicit LevelDbGraphBase(const std::string& filename, std::size_t cacheSizeInMB);
                    LevelDbGraphBase(const LevelDbGraphBase&) = delete;
                    LevelDbGraphBase& operator=(const LevelDbGraphBase&)= delete;
                    LevelDbGraphBase(LevelDbGraphBase&&) = default;
                    virtual ~LevelDbGraphBase();

                    typedef typename InterfaceType::NodesBundle NodesBundle;
                    typedef typename InterfaceType::EdgesBundle EdgesBundle;
                    typedef typename InterfaceType::ResultType ResultType;
                    typedef typename InterfaceType::NodeIdType NodeIdType;
                    typedef typename InterfaceType::EdgeIdType EdgeIdType;

                    virtual void destroy() override;
            };

        template<typename NodeType, typename EdgeType>
            LevelDbGraphBase<NodeType, EdgeType>::LevelDbGraphBase(const std::string& filename):
                LevelDbGraphBase(filename, 100)
        {}

        template<typename NodeType, typename EdgeType>
            LevelDbGraphBase<NodeType, EdgeType>::LevelDbGraphBase(const std::string& filename,
                        std::size_t cacheSizeInMB): filename_(filename), cacheSize_(cacheSizeInMB)
        {
            options.create_if_missing = true;
            options.block_cache = leveldb::NewLRUCache(cacheSizeInMB * 1024 * 1024);
            leveldb::Status status = leveldb::DB::Open(options,
                        filename,
                        &db);
            if (!status.ok())
            {
                std::cerr << status.ToString() << std::endl;
                std::terminate();
            }
        }

        template<typename NodeType, typename EdgeType>
            LevelDbGraphBase<NodeType, EdgeType>::~LevelDbGraphBase()
            {
                delete db;
                delete options.block_cache;
            }

        template<typename NodeType, typename EdgeType>
            void LevelDbGraphBase<NodeType, EdgeType>::destroy()
            {
                delete db;
                delete options.block_cache;
                leveldb::DestroyDB(filename_, leveldb::Options());

                options.block_cache = leveldb::NewLRUCache(cacheSize_ * 1024 * 1024);
                leveldb::Status status = leveldb::DB::Open(options,
                            filename_,
                            &db);
                if (!status.ok())
                {
                    std::cerr << status.ToString() << std::endl;
                    std::terminate();
                }

            }
    }

    template<typename NodeType, typename EdgeType,
        bool isDirected = true>
            class LevelDbGraph;
                
    template<typename NodeType, typename EdgeType>
        class LevelDbGraph<NodeType, EdgeType, true>
        : public LevelDbGraphBase<NodeType, EdgeType>
    {
        private:
            typedef GraphInterface<NodeType, EdgeType> InterfaceType;
        public:
            explicit LevelDbGraph(const std::string& filename);
            explicit LevelDbGraph(const std::string& filename, std::size_t cacheSizeInMB);
            LevelDbGraph(const LevelDbGraph&) = delete;
            LevelDbGraph& operator=(const LevelDbGraph&)= delete;
            LevelDbGraph(LevelDbGraph&&) = default;
            virtual ~LevelDbGraph();

            typedef typename InterfaceType::NodesBundle NodesBundle;
            typedef typename InterfaceType::EdgesBundle EdgesBundle;
            typedef LevelDbGraphIterator<NodeType, EdgeType, true> ResultType;
            typedef typename InterfaceType::NodeIdType NodeIdType;
            typedef typename InterfaceType::EdgeIdType EdgeIdType;

            friend class LevelDbGraphIterator<NodeType, EdgeType, true>;

            virtual ResultType
                query(const GraphSqlSentence&);
            virtual ResultType
                end();
            virtual void setNode(const NodeType&) override;
            virtual void setEdge(const EdgeType&) override;
            virtual void setNodesBundle(const NodesBundle&) override;
            virtual void setEdgesBundle(const EdgesBundle&) override;

            virtual void removeNode(const NodeIdType&) override;
            virtual void removeEdge(const EdgeIdType&) override;

            typedef std::set<EdgeIdType> inoutEdgesType;
        protected:
            void removeEdgeImpl(const EdgeIdType& edgeId,
                        bool updateFromNode,
                        bool updateToNode, leveldb::WriteBatch* batch = nullptr);
            static const std::size_t edgeCacheSize = 1024 * 1024;
            void setInEdge(const NodeIdType& nodeId, inoutEdgesType inEdges,
                        leveldb::WriteBatch *batch = nullptr);
            void setOutEdge(const NodeIdType& nodeId, inoutEdgesType outEdges,
                        leveldb::WriteBatch *batch = nullptr);

        public:
            inoutEdgesType getInEdge(const NodeIdType& nodeId);
            inoutEdgesType getOutEdge(const NodeIdType& nodeId);
            NodeType getNode(const NodeIdType &nodeId);
            EdgeType getEdge(const EdgeIdType &edgeId);
        private:
            impl::TypedMRUMap<NodeIdType, inoutEdgesType > outEdgeCache, inEdgeCache;

    };

    template<typename NodeType, typename EdgeType>
        class LevelDbGraph<NodeType, EdgeType, false>
        : public LevelDbGraphBase<NodeType, EdgeType>
        {
            private:
                typedef GraphInterface<NodeType, EdgeType> InterfaceType;
            public:
                explicit LevelDbGraph(const std::string& filename);
                explicit LevelDbGraph(const std::string& filename, std::size_t cacheSizeInMB);
                LevelDbGraph(const LevelDbGraph&) = delete;
                LevelDbGraph& operator=(const LevelDbGraph&)= delete;
                LevelDbGraph(LevelDbGraph&&) = default;
                virtual ~LevelDbGraph();

                typedef typename InterfaceType::NodesBundle NodesBundle;
                typedef typename InterfaceType::EdgesBundle EdgesBundle;
                typedef typename InterfaceType::ResultType ResultType;
                typedef typename InterfaceType::NodeIdType NodeIdType;
                typedef typename InterfaceType::EdgeIdType EdgeIdType;

                friend class LevelDbGraphIterator<NodeType, EdgeType>;

                virtual typename InterfaceType::ResultType
                    query(const GraphSqlSentence&);
                virtual void setNode(const NodeType&) override;
                virtual void setEdge(const EdgeType&) override;
                virtual void setNodesBundle(const NodesBundle&) override;
                virtual void setEdgesBundle(const EdgesBundle&) override;

                virtual void removeNode(const NodeIdType&) override;
                virtual void removeEdge(const EdgeIdType&) override;

                typedef std::set<EdgeIdType> inoutEdgesType;
            protected:
                void removeEdgeImpl(const EdgeIdType& edgeId,
                            bool updateFromNode,
                            bool updateToNode, leveldb::WriteBatch* batch = nullptr);
                void setOutEdge(const NodeIdType& nodeId, inoutEdgesType outEdges,
                            leveldb::WriteBatch *batch = nullptr);
                static const std::size_t edgeCacheSize = 1024 * 1024;

            public:
                inoutEdgesType getOutEdge(const NodeIdType& nodeId);
                NodeType getNode(const NodeIdType &nodeId);
                EdgeType getEdge(const EdgeIdType &edgeId);
            private:
                impl::TypedMRUMap<NodeIdType, inoutEdgesType > outEdgeCache;
        };

    template<typename NodeType, typename EdgeType>
        LevelDbGraph<NodeType, EdgeType, true>::LevelDbGraph(const std::string& filename):
            LevelDbGraphBase<NodeType, EdgeType>(filename),
            inEdgeCache(edgeCacheSize),
            outEdgeCache(edgeCacheSize)
    {}

    template<typename NodeType, typename EdgeType>
        LevelDbGraph<NodeType, EdgeType, false>::LevelDbGraph(const std::string& filename):
            LevelDbGraphBase<NodeType, EdgeType>(filename),
            outEdgeCache(edgeCacheSize)
    {}

    template<typename NodeType, typename EdgeType>
        LevelDbGraph<NodeType, EdgeType, true>::LevelDbGraph(const std::string& filename,
                    std::size_t cacheSizeInMB): LevelDbGraphBase<NodeType, EdgeType>(
                            filename, cacheSizeInMB), inEdgeCache(edgeCacheSize),
                    outEdgeCache(edgeCacheSize)
    {}

    template<typename NodeType, typename EdgeType>
        LevelDbGraph<NodeType, EdgeType, false>::LevelDbGraph(const std::string& filename,
                    std::size_t cacheSizeInMB): LevelDbGraphBase<NodeType, EdgeType>(
                            filename, cacheSizeInMB), outEdgeCache(edgeCacheSize)
    {}

    template<typename NodeType, typename EdgeType>
        LevelDbGraph<NodeType, EdgeType, true>::~LevelDbGraph()
        {}

    template<typename NodeType, typename EdgeType>
        LevelDbGraph<NodeType, EdgeType, false>::~LevelDbGraph()
        {}


    template<typename NodeType, typename EdgeType>
        typename LevelDbGraph<NodeType, EdgeType, true>::inoutEdgesType
        LevelDbGraph<NodeType, EdgeType, true>::getInEdge(const NodeIdType& nodeId)
        {
            auto result = inEdgeCache.find(nodeId);
            if (result != inEdgeCache.end())
                return result->second;
            else
            {
                leveldb::Status status;
                static std::string raw;
                status = this->db->Get(leveldb::ReadOptions(), addSuffix(nodeId, inEdgeSuffix), &raw);
                assert(status.ok() || status.IsNotFound());
                if (status.ok())
                    return strToDataByCereal< inoutEdgesType >(raw);
                else
                    return inoutEdgesType();
            }
        }

    //no getInEdge for undirected graph because every edges are outEdge

    template<typename NodeType, typename EdgeType>
        typename LevelDbGraph<NodeType, EdgeType, true>::inoutEdgesType
        LevelDbGraph<NodeType, EdgeType, true>::getOutEdge(const NodeIdType& nodeId)
        {
            auto result = outEdgeCache.find(nodeId);
            if (result != outEdgeCache.end())
                return result->second;
            else
            {
                leveldb::Status status;
                static std::string raw;
                status = this->db->Get(leveldb::ReadOptions(), addSuffix(nodeId, outEdgeSuffix), &raw);
                if (status.ok())
                    return strToDataByCereal< inoutEdgesType >(raw);
                else
                    return inoutEdgesType();
            }
        }

    template<typename NodeType, typename EdgeType>
        typename LevelDbGraph<NodeType, EdgeType, false>::inoutEdgesType
        LevelDbGraph<NodeType, EdgeType, false>::getOutEdge(const NodeIdType& nodeId)
        {
            auto result = outEdgeCache.find(nodeId);
            if (result != outEdgeCache.end())
                return result->second;
            else
            {
                leveldb::Status status;
                static std::string raw;
                status = this->db->Get(leveldb::ReadOptions(), addSuffix(nodeId, outEdgeSuffix), &raw);
                if (status.ok())
                    return strToDataByCereal< inoutEdgesType >(raw);
                else
                    return inoutEdgesType();
            }
        }

    template<typename NodeType, typename EdgeType>
        NodeType
        LevelDbGraph<NodeType, EdgeType, true>::getNode(const NodeIdType& nodeId)
        {
            std::string s;
            assert(this->db->Get(leveldb::ReadOptions(), addSuffix(nodeId, nodeDataIdSuffix), &s)
                        .ok());
            return strToDataByProtobuf< NodeType >
                (s);
        }

    template<typename NodeType, typename EdgeType>
        NodeType
        LevelDbGraph<NodeType, EdgeType, false>::getNode(const NodeIdType& nodeId)
        {
            std::string s;
            assert(this->db->Get(leveldb::ReadOptions(), addSuffix(nodeId, nodeDataIdSuffix), &s)
                        .ok());
            return strToDataByProtobuf< NodeType >
                (s);
        }

    template<typename NodeType, typename EdgeType>
        EdgeType
        LevelDbGraph<NodeType, EdgeType, true>::getEdge(const EdgeIdType& edgeId)
        {
            std::string s;
            this->db->Get(leveldb::ReadOptions(), addSuffix(edgeId, edgeDataIdSuffix), &s);
            return strToDataByProtobuf< EdgeType >
                (s);
        }

    template<typename NodeType, typename EdgeType>
        EdgeType
        LevelDbGraph<NodeType, EdgeType, false>::getEdge(const EdgeIdType& edgeId)
        {
            std::string s;
            this->db->Get(leveldb::ReadOptions(), addSuffix(edgeId, edgeDataIdSuffix), &s);
            return strToDataByProtobuf< EdgeType >
                (s);
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, true>::setInEdge(const NodeIdType& nodeId,
                    inoutEdgesType inEdges, leveldb::WriteBatch *batch)
        {
            stringStreamSlice slice = dataToSliceByCereal(inEdges);
            if (batch == nullptr)
            {
                leveldb::Status status = this->db->Put(leveldb::WriteOptions(), addSuffix(nodeId, inEdgeSuffix),
                            slice.getSlice());
                assert(status.ok());
            }
            else
            {
                batch->Put(addSuffix(nodeId, inEdgeSuffix), slice.getSlice());
            }

            inEdgeCache[nodeId] = std::move(inEdges);
        }

    // no getInEdge/setInEdge for undirected graph

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, true>::setOutEdge(const NodeIdType& nodeId,
                    inoutEdgesType outEdges, leveldb::WriteBatch *batch)
        {
            stringStreamSlice slice = dataToSliceByCereal(outEdges);
            if (batch == nullptr)
            {
                leveldb::Status status = this->db->Put(leveldb::WriteOptions(), addSuffix(nodeId, outEdgeSuffix),
                            slice.getSlice());
                assert(status.ok());
            }
            else
            {
                batch->Put(addSuffix(nodeId, outEdgeSuffix), slice.getSlice());
            }

            outEdgeCache[nodeId] = std::move(outEdges);
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, false>::setOutEdge(const NodeIdType& nodeId,
                    inoutEdgesType outEdges, leveldb::WriteBatch *batch)
        {
            stringStreamSlice slice = dataToSliceByCereal(outEdges);
            if (batch == nullptr)
            {
                leveldb::Status status = this->db->Put(leveldb::WriteOptions(), addSuffix(nodeId, outEdgeSuffix),
                            slice.getSlice());
                assert(status.ok());
            }
            else
            {
                batch->Put(addSuffix(nodeId, outEdgeSuffix), slice.getSlice());
            }

            outEdgeCache[nodeId] = std::move(outEdges);
        }


    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, true>::setNode(const NodeType& node)
        {
            stringSlice ssslice = dataToSliceByProtobuf(node);
            LOGGER(trace, "Actual added id: {}", addSuffix(node.id(), nodeDataIdSuffix));
            //std::cout << "Actual added id:"<< addSuffix(node.id(), nodeDataIdSuffix) << std::endl;
            leveldb::Status status = this->db->Put(leveldb::WriteOptions(), addSuffix(node.id(), nodeDataIdSuffix), ssslice.getSlice());
            assert(status.ok());
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, false>::setNode(const NodeType& node)
        {
            stringSlice ssslice = dataToSliceByProtobuf(node);
            leveldb::Status status = this->db->Put(leveldb::WriteOptions(), addSuffix(node.id(), nodeDataIdSuffix), ssslice.getSlice());
            assert(status.ok());
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, true>::setNodesBundle(const NodesBundle& nb)
        {
            leveldb::WriteBatch batch;
            for (auto& node : nb)
            {
                stringSlice ssslice = dataToSliceByProtobuf(node);
                batch.Put(addSuffix(node.id(), nodeDataIdSuffix), ssslice.getSlice());
                LOGGER(trace, "Actual added id: {}", addSuffix(node.id(), nodeDataIdSuffix));
                //std::cout << "Put " << addSuffix(node.id(), nodeDataIdSuffix) << std::endl;
            }
            leveldb::Status status = this->db->Write(leveldb::WriteOptions(), &batch);
            assert(status.ok());
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, false>::setNodesBundle(const NodesBundle& nb)
        {
            leveldb::WriteBatch batch;
            for (auto& node : nb)
            {
                stringSlice ssslice = dataToSliceByProtobuf(node);
                batch.Put(addSuffix(node.id(), nodeDataIdSuffix), ssslice.getSlice());
            }
            leveldb::Status status = this->db->Write(leveldb::WriteOptions(), &batch);
            assert(status.ok());
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, true>::setEdge(const EdgeType& edge)
        {
            const NodeIdType& from = edge.from();
            const NodeIdType& to = edge.to();
            const EdgeIdType& id = edge.id();

            std::string raw;
            leveldb::Status status;
            leveldb::WriteBatch batch;

            // deal with outedge
            inoutEdgesType outSet = getOutEdge(from);
            outSet.insert(id);
            setOutEdge(from, std::move(outSet), &batch);


            //deal with inedge
            inoutEdgesType inSet = getInEdge(to);
            inSet.insert(id);
            setInEdge(to, std::move(inSet), &batch);

            //save the edge
            stringSlice resultproto = dataToSliceByProtobuf(edge);
            batch.Put(addSuffix(id, edgeDataIdSuffix), resultproto.getSlice());

            status = this->db->Write(leveldb::WriteOptions(), &batch);
            assert(status.ok());
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, false>::setEdge(const EdgeType& edge)
        {
            const NodeIdType& from = edge.from();
            const NodeIdType& to = edge.to();
            const EdgeIdType& id = edge.id();

            std::string raw;
            leveldb::Status status;
            leveldb::WriteBatch batch;

            // deal with outedge
            inoutEdgesType outSet = getOutEdge(from);
            outSet.insert(id);
            setOutEdge(from, std::move(outSet), &batch);


            //deal with inedge
            inoutEdgesType outSet2 = getOutEdge(to);
            outSet2.insert(id);
            setOutEdge(to, std::move(outSet2), &batch);

            //save the edge
            stringSlice resultproto = dataToSliceByProtobuf(edge);
            batch.Put(addSuffix(id, edgeDataIdSuffix), resultproto.getSlice());

            status = this->db->Write(leveldb::WriteOptions(), &batch);
            assert(status.ok());
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, true>::setEdgesBundle(const EdgesBundle& eb)
        {
            std::string raw;
            leveldb::Status status;

            //save the edge
            leveldb::WriteBatch batch;
            for(auto& e : eb)
            {
                auto result = dataToSliceByProtobuf(e);
                batch.Put(addSuffix(e.id(), edgeDataIdSuffix), result.getSlice());
            }

            //deal with outEdge
            for(auto& e : eb)
            {
                inoutEdgesType outSet = getOutEdge(e.from());
                outSet.insert(e.id());
                setOutEdge(e.from(), std::move(outSet), &batch);

                inoutEdgesType inSet = getInEdge(e.to());
                inSet.insert(e.id());
                setInEdge(e.to(), std::move(inSet), &batch);
            }


            status = this->db->Write(leveldb::WriteOptions(), &batch);
            assert(status.ok());
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, false>::setEdgesBundle(const EdgesBundle& eb)
        {
            std::string raw;
            leveldb::Status status;

            //save the edge
            leveldb::WriteBatch batch;
            for(auto& e : eb)
            {
                auto result = dataToSliceByProtobuf(e);
                batch.Put(addSuffix(e.id(), edgeDataIdSuffix), result.getSlice());
            }

            //deal with outEdge
            for(auto& e : eb)
            {
                inoutEdgesType outSet = getOutEdge(e.from());
                outSet.insert(e.id());
                setOutEdge(e.from(), std::move(outSet), &batch);

                inoutEdgesType outSet2 = getOutEdge(e.to());
                outSet2.insert(e.id());
                setOutEdge(e.to(), std::move(outSet2), &batch);
            }


            status = this->db->Write(leveldb::WriteOptions(), &batch);
            assert(status.ok());
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, true>::removeNode(const NodeIdType& nodeId)
        {
            static std::string raw;
            static leveldb::Status status;
            leveldb::WriteBatch batch;

            status = this->db->Delete(leveldb::WriteOptions(), addSuffix(nodeId, nodeDataIdSuffix));
            assert(status.ok());

            inoutEdgesType inSet = getInEdge(nodeId);
            for (auto& edgeId : inSet)
                removeEdgeImpl(edgeId, false, true, &batch);

            inoutEdgesType outSet = getOutEdge(nodeId);
            for (auto& edgeId : outSet)
                removeEdgeImpl(edgeId, true, false, &batch);

            status = this->db->Write(leveldb::WriteOptions(), &batch);
            assert(status.ok());
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, false>::removeNode(const NodeIdType& nodeId)
        {
            static std::string raw;
            static leveldb::Status status;
            leveldb::WriteBatch batch;

            status = this->db->Delete(leveldb::WriteOptions(), addSuffix(nodeId, nodeDataIdSuffix));
            assert(status.ok());

            inoutEdgesType outSet = getOutEdge(nodeId);
            for (auto& edgeId : outSet)
            {
                if (nodeId == getEdge(edgeId).from())
                    removeEdgeImpl(edgeId, false, true, &batch);
                else
                    removeEdgeImpl(edgeId, true, false, &batch);
            }

            status = this->db->Write(leveldb::WriteOptions(), &batch);
            assert(status.ok());
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, true>::removeEdge(const EdgeIdType& edgeId)
        {
            removeEdgeImpl(edgeId, true, true);
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, false>::removeEdge(const EdgeIdType& edgeId)
        {
            removeEdgeImpl(edgeId, true, true);
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, true>::removeEdgeImpl(
                    const EdgeIdType& edgeId,
                    bool updateFromNode,
                    bool updateToNode,
                    leveldb::WriteBatch* batch)
        {
            std::string raw;
            leveldb::Status status;
            bool autoCommit = false;
            if (batch == nullptr)
            {
                batch = new leveldb::WriteBatch();
                autoCommit = true;
            }
            if (!updateFromNode && !updateToNode)
            {
                status = this->db->Delete(leveldb::WriteOptions(), addSuffix(edgeId, edgeDataIdSuffix));
                assert(status.ok() || status.IsNotFound());
            }
            else
            {
                status = this->db->Get(leveldb::ReadOptions(), addSuffix(edgeId, edgeDataIdSuffix), &raw);
                assert(status.ok() || status.IsNotFound());
                if (status.IsNotFound())
                    return;
                EdgeType edge = strToDataByProtobuf<EdgeType>(raw);

                if (updateFromNode)
                {
                    inoutEdgesType outEdges = getOutEdge(edge.from());
                    outEdges.erase(edgeId);
                    setOutEdge(edge.from(), std::move(outEdges), batch);
                }
                if (updateToNode)
                {
                    inoutEdgesType inEdges = getInEdge(edge.to());
                    inEdges.erase(edgeId);
                    setInEdge(edge.to(), std::move(inEdges), batch);
                }

                if (autoCommit)
                {
                    status = this->db->Write(leveldb::WriteOptions(), batch);
                    assert(status.ok());
                    delete batch;
                }
            }
        }


    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType, false>::removeEdgeImpl(
                    const EdgeIdType& edgeId,
                    bool updateFromNode,
                    bool updateToNode,
                    leveldb::WriteBatch* batch)
        {
            std::string raw;
            leveldb::Status status;
            bool autoCommit = false;
            if (batch == nullptr)
            {
                batch = new leveldb::WriteBatch();
                autoCommit = true;
            }
            if (!updateFromNode && !updateToNode)
            {
                status = this->db->Delete(leveldb::WriteOptions(), addSuffix(edgeId, edgeDataIdSuffix));
                assert(status.ok() || status.IsNotFound());
            }
            else
            {
                status = this->db->Get(leveldb::ReadOptions(), addSuffix(edgeId, edgeDataIdSuffix), &raw);
                assert(status.ok() || status.IsNotFound());
                if (status.IsNotFound())
                    return;
                EdgeType edge = strToDataByProtobuf<EdgeType>(raw);

                if (updateFromNode)
                {
                    inoutEdgesType outEdges = getOutEdge(edge.from());
                    outEdges.erase(edgeId);
                    setOutEdge(edge.from(), std::move(outEdges), batch);
                }
                if (updateToNode)
                {
                    inoutEdgesType inEdges = getOutEdge(edge.to());
                    inEdges.erase(edgeId);
                    setOutEdge(edge.to(), std::move(inEdges), batch);
                }

                if (autoCommit)
                {
                    status = this->db->Write(leveldb::WriteOptions(), batch);
                    assert(status.ok());
                    delete batch;
                }
            }
        }
}

#include "leveldbgraph_deduction.inc"
#include "leveldbgraph_iterator.inc"

namespace netalgo
{
    template<typename NodeType, typename EdgeType>
        typename LevelDbGraph<NodeType, EdgeType, true>::ResultType
        LevelDbGraph<NodeType, EdgeType, true>::query(const GraphSqlSentence& q)
        {
            return LevelDbGraphIterator<NodeType, EdgeType, true>(*this, q);
        }

    template<typename NodeType, typename EdgeType>
        typename LevelDbGraph<NodeType, EdgeType, false>::ResultType
        LevelDbGraph<NodeType, EdgeType, false>::query(const GraphSqlSentence& q)
        {
            using namespace impl;
            throw std::runtime_error("Query not implemented yet");
            LevelDbGraphIterator<NodeType, EdgeType, false> it(*this);
            DeductionStepsType deductionSteps = generateDeductionSteps(q);
        }

    template<typename NodeType, typename EdgeType>
        typename LevelDbGraph<NodeType, EdgeType, true>::ResultType
        LevelDbGraph<NodeType, EdgeType, true>::end()
        {
            return LevelDbGraphIterator<NodeType, EdgeType, true>(*this);
        }

}

#endif
