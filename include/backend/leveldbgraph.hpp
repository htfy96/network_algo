#ifndef BACKEND_LEVELDBGRAPH_HPP
#define BACKEND_LEVELDBGRAPH_HPP

#include "graph_interface.hpp"

#include <leveldb/db.h>
#include <leveldb/write_batch.h>
#include <leveldb/cache.h>

#include <cereal/archives/binary.hpp>
#include <cereal/types/utility.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/set.hpp>
#include <cereal/types/string.hpp>

#include <google/protobuf/generated_message_util.h>
#include <google/protobuf/message.h>
#include <google/protobuf/repeated_field.h>
#include <google/protobuf/extension_set.h>
#include <google/protobuf/unknown_field_set.h>
#include <google/protobuf/stubs/common.h>

#include <string>
#include <cassert>
#include <exception>
#include <iostream>
#include <type_traits>
#include <sstream>
#include <memory>
#include <cstring>
#include <iterator>
#include <set>

namespace
{
    class stringStreamSlice
    {
        private:
            std::string str_;
        public:
            stringStreamSlice(const std::ostringstream& os):str_(os.str()) {}
            leveldb::Slice getSlice()
            {
                return leveldb::Slice(str_);
            }
    };

    class stringSlice
    {
        private:
            std::string str_;
        public:
            stringSlice(std::string s): str_(std::move(s)) {}
            leveldb::Slice getSlice()
            {
                return leveldb::Slice(str_);
            }
    };

        
    template<typename T>
        stringSlice dataToSliceByProtobuf(const T& data)
        {
            std::string result;
            data.SerializeToString(&result);
            return result;
        }

    template<typename T>
        stringStreamSlice dataToSliceByCereal(const T& data)
        {
            std::ostringstream os(std::ios::binary | std::ios::out);
            cereal::BinaryOutputArchive oa(os);
            oa << data;
            return os;
        }

    template<typename T>
        T sliceToDataByProtobuf(const leveldb::Slice& slice)
        {
            std::string str(slice.ToString());
            T result;
            result.ParseFromString(str);
            return result;
        }

    template<typename T>
        T strToDataByCereal(std::string s)
        {
            std::istringstream is(std::move(s), std::ios::binary | std::ios::in);
            cereal::BinaryInputArchive archive(is);
            T data;
            archive(data);
            return data;
        }

    template<typename T>
        T strToDataByProtobuf(const std::string s)
        {
            T result;
            result.ParseFromString(s);
            return result;
        }

    const char nodeDataIdSuffix[] = ":node:@data";
    const char edgeDataIdSuffix[] = ":edge:@data";
    const char outEdgeSuffix[] = ":@outedge";
    const char inEdgeSuffix[] = ":@inedge";

    template<typename T>
        std::string addSuffix(const T& originalId, const char* suffix)
        {
            leveldb::Slice idSlice(originalId);
            std::string dataId(idSlice.data(), idSlice.size());
            dataId += suffix;
            return dataId;
        }

}

namespace netalgo
{
    template<typename NodeType, typename EdgeType>
        class LevelDbGraph;
    template<typename NodeType, typename EdgeType>
        struct LevelDbGraphResult
        {
            std::unordered_map<std::string, NodeType> nodes;
            std::unordered_map<std::string, EdgeType> edges;
            NodeType& getNode(const std::string& key) { return nodes[key]; }
            EdgeType& getEdge(const std::string& key) { return edges[key]; }
        };

    template<typename NodeType, typename EdgeType>
        class LevelDbGraphIterator
        {
            private:
                GraphSqlSentence sql;
            public:
                typedef decltype(std::declval<NodeType>().id) NodeIdType;
                typedef decltype(std::declval<EdgeType>().id) EdgeIdType;
            protected:
                std::vector< NodeIdType > nodesId;
                std::vector< EdgeIdType > edgesId;
            public:
                typedef LevelDbGraphResult<NodeType, EdgeType> value_type;
                typedef value_type& reference;
                typedef value_type* pointer;
                LevelDbGraphIterator& operator++();
                value_type operator*();
                pointer operator->();
                bool operator==(const LevelDbGraphIterator& other);
                bool operator!=(const LevelDbGraphIterator& other);
                friend LevelDbGraph<NodeType, EdgeType>;
        };
                
    template<typename NodeType, typename EdgeType>
        class LevelDbGraph : public GraphInterface<NodeType, EdgeType>
    {
        private:
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
            const std::string filename_;
        public:
            LevelDbGraph(const std::string& filename);
            LevelDbGraph(const std::string& filename, leveldb::Options options);
            LevelDbGraph(const LevelDbGraph&) = delete;
            LevelDbGraph& operator=(const LevelDbGraph&)= delete;
            LevelDbGraph(LevelDbGraph&&) = default;
            virtual ~LevelDbGraph();

            typedef typename InterfaceType::NodesBundle NodesBundle;
            typedef typename InterfaceType::EdgesBundle EdgesBundle;
            typedef typename InterfaceType::ResultType ResultType;
            typedef typename InterfaceType::NodeIdType NodeIdType;
            typedef typename InterfaceType::EdgeIdType EdgeIdType;

            virtual typename InterfaceType::ResultType
                query(const GraphSqlSentence&) override;
            virtual void setNode(const NodeType&) override;
            virtual void setEdge(const EdgeType&) override;
            virtual void setNodesBundle(const NodesBundle&) override;
            virtual void setEdgesBundle(const EdgesBundle&) override;

            virtual void removeNode(const NodeIdType&) override;
            virtual void removeEdge(const EdgeIdType&) override;

            virtual void destroy() override;
        protected:
            void removeEdgeImpl(const EdgeIdType& edgeId,
                        bool updateFromNode,
                        bool updateToNode, leveldb::WriteBatch* batch = nullptr);

    };

    template<typename NodeType, typename EdgeType>
        LevelDbGraph<NodeType, EdgeType>::LevelDbGraph(const std::string& filename):
            LevelDbGraph(filename, leveldb::Options())
    {}

    template<typename NodeType, typename EdgeType>
        LevelDbGraph<NodeType, EdgeType>::LevelDbGraph(const std::string& filename,
                    leveldb::Options options): filename_(filename)
        {
            options.create_if_missing = true;
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
        LevelDbGraph<NodeType, EdgeType>::~LevelDbGraph()
        {
            delete db;
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType>::destroy()
        {
            leveldb::DestroyDB(filename_, leveldb::Options());
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType>::setNode(const NodeType& node)
        {
            stringSlice ssslice = dataToSliceByProtobuf(node);
            leveldb::Status status = db->Put(leveldb::WriteOptions(), addSuffix(node.id(), nodeDataIdSuffix), ssslice.getSlice());
            assert(status.ok());
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType>::setNodesBundle(const NodesBundle& nb)
        {
            leveldb::WriteBatch batch;
            for (auto& node : nb)
            {
                stringSlice ssslice = dataToSliceByProtobuf(node);
                batch.Put(addSuffix(node.id(), nodeDataIdSuffix), ssslice.getSlice());
            }
            leveldb::Status status = db->Write(leveldb::WriteOptions(), &batch);
            assert(status.ok());
        }


    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType>::setEdge(const EdgeType& edge)
        {
            const NodeIdType& from = edge.from();
            const NodeIdType& to = edge.to();
            const EdgeIdType& id = edge.id();

            std::string raw;
            leveldb::Status status;

            // deal with outedge
            status = db->Get(leveldb::ReadOptions(), addSuffix(from, outEdgeSuffix), &raw);
            std::set<EdgeIdType> outSet;
            if (status.ok())
                outSet = strToDataByCereal< std::set<EdgeIdType> >(raw);
            outSet.insert(id);
            stringStreamSlice result = dataToSliceByCereal(outSet);
            db->Put(leveldb::WriteOptions(), addSuffix(from, outEdgeSuffix), result.getSlice());

            //deal with inedge
            status = db->Get(leveldb::ReadOptions(), addSuffix(to, inEdgeSuffix), &raw);
            std::set<EdgeIdType> inSet;
            if (status.ok())
                inSet = strToDataByCereal< std::set<EdgeIdType> >(raw);
            inSet.insert(id);
            result = dataToSliceByCereal(inSet);
            db->Put(leveldb::WriteOptions(), addSuffix(to, inEdgeSuffix), result.getSlice());

            //save the edge
            stringSlice resultproto = dataToSliceByProtobuf(edge);
            status = db->Put(leveldb::WriteOptions(), addSuffix(id, edgeDataIdSuffix), resultproto.getSlice());

            assert(status.ok());
                
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType>::setEdgesBundle(const EdgesBundle& eb)
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
                status = db->Get(leveldb::ReadOptions(), addSuffix(e.from(), outEdgeSuffix), &raw);
                std::set<EdgeIdType> outSet;
                if (status.ok())
                    outSet = strToDataByCereal< std::set<EdgeIdType> >(raw);
                outSet.insert(e.id());
                auto newSlice = dataToSliceByCereal(outSet);
                batch.Put(addSuffix(e.from(), outEdgeSuffix), newSlice.getSlice());
            }

            //deal with inEdge
            for(auto& e : eb)
            {
                status = db->Get(leveldb::ReadOptions(), addSuffix(e.to(), inEdgeSuffix), &raw);
                std::set<EdgeIdType> inSet;
                if (status.ok())
                    inSet = strToDataByCereal< std::set<EdgeIdType> >(raw);
                inSet.insert(e.id());
                auto newSlice = dataToSliceByCereal(inSet);
                batch.Put(addSuffix(e.to(), inEdgeSuffix), newSlice.getSlice());
            }

            status = db->Write(leveldb::WriteOptions(), &batch);
            assert(status.ok());
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType>::removeNode(const NodeIdType& nodeId)
        {
            static std::string raw;
            static leveldb::Status status;
            leveldb::WriteBatch batch;

            status = db->Delete(leveldb::WriteOptions(), addSuffix(nodeId, nodeDataIdSuffix));
            assert(status.ok());
    
            std::set<EdgeIdType> inSet;
            status = db->Get(leveldb::ReadOptions(), addSuffix(nodeId, inEdgeSuffix), &raw);
            if (!status.IsNotFound())
                inSet = strToDataByCereal< std::set<EdgeIdType> >(raw);
            for (auto& edgeId : inSet)
                removeEdgeImpl(edgeId, false, true, &batch);

            std::set<EdgeIdType> outSet;
            status = db->Get(leveldb::ReadOptions(), addSuffix(nodeId, outEdgeSuffix), &raw);
            if (!status.IsNotFound())
                outSet = strToDataByCereal< std::set<EdgeIdType> >(raw);
            for (auto& edgeId : outSet)
                removeEdgeImpl(edgeId, true, false, &batch);

            status = db->Write(leveldb::WriteOptions(), &batch);
            assert(status.ok());
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType>::removeEdge(const EdgeIdType& edgeId)
        {
            removeEdgeImpl(edgeId, true, true);
        }

    template<typename NodeType, typename EdgeType>
        void LevelDbGraph<NodeType, EdgeType>::removeEdgeImpl(
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
                status = db->Delete(leveldb::WriteOptions(), addSuffix(edgeId, edgeDataIdSuffix));
                assert(status.ok() || status.IsNotFound());
            }
            else
            {
                status = db->Get(leveldb::ReadOptions(), addSuffix(edgeId, edgeDataIdSuffix), &raw);
                assert(status.ok() || status.IsNotFound());
                if (status.IsNotFound())
                    return;
                EdgeType edge = strToDataByProtobuf<EdgeType>(raw);
                
                if (updateFromNode)
                {
                    status = db->Get(leveldb::ReadOptions(), addSuffix(edge.from(), outEdgeSuffix), &raw);
                    assert(status.ok());

                    std::set<EdgeIdType> outEdge = strToDataByCereal< std::set<EdgeIdType> >(raw);
                    assert(outEdge.find(edgeId) != outEdge.end());
                    outEdge.erase(edgeId);
                    auto slice = dataToSliceByCereal(outEdge);
                    batch->Put(addSuffix(edge.from(), outEdgeSuffix), slice.getSlice());
                }
                if (updateToNode)
                {
                    status = db->Get(leveldb::ReadOptions(), addSuffix(edge.to(), inEdgeSuffix), &raw);
                    assert(status.ok());
                    std::set<EdgeIdType> inEdge = strToDataByCereal< std::set<EdgeIdType> >(raw);
                    assert(inEdge.find(edgeId) != inEdge.end());
                    inEdge.erase(edgeId);
                    auto slice = dataToSliceByCereal(inEdge);
                    batch->Put(addSuffix(edge.to(), inEdgeSuffix), slice.getSlice());
                }

                if (autoCommit)
                {
                    status = db->Write(leveldb::WriteOptions(), batch);
                    assert(status.ok());
                    delete batch;
                }
            }
        }

    template<typename NodeType, typename EdgeType>
        typename LevelDbGraph<NodeType, EdgeType>::ResultType 
        LevelDbGraph<NodeType, EdgeType>::query(const GraphSqlSentence& q)
        {
        }


            
}

#endif
