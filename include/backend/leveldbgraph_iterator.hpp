#ifndef GRAPH_BACKEND_LEVELDBGRAPH_ITERATOR
#define GRAPH_BACKEND_LEVELDBGRAPH_ITERATOR


#include "graphdsl.hpp"
#include "leveldbgraph_db_utility.hpp"
#include "leveldbgraph_deduction.hpp"
#include <type_traits>
#include <string>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <map>

namespace netalgo
{
	
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
			public:
				typedef typename std::decay<decltype(std::declval<NodeType>().id())>::type NodeIdType;
				typedef typename std::decay<decltype(std::declval<EdgeType>().id())>::type EdgeIdType;
				typedef std::set< EdgeIdType > inoutEdgesType;
			private:
				GraphSqlSentence sql;
				DeductionStepsType deductionSteps;
				LevelDbGraphResult<NodeType, EdgeType> result;
				LevelDbGraph<NodeType, EdgeType> &graph;
				bool isEnd;

				bool checkLeftConstrained(const std::size_t id)
				{
					if (isNode(id - 1)) // node - edge(*)
					{
						NodeIdType nodeId = this->nodesId.at(getNodeIndex(id - 1));
						// the edge dir in query
						EdgeDirection edgeDir = this->sql.first.edges.at(getEdgeIndex(id)).direction;

						inoutEdgesType outEdges = graph.getOutEdge(nodeId),
									   inEdges = graph.getInEdge(nodeId);

						bool result = true;
						if (edgeDir == EdgeDirection::prev)
							result &= (inEdges.find(this->edgesId.at(getEdgeIndex(id))) !=
										inEdges.end());
						if (edgeDir == EdgeDirection::next ||
									edgeDir == EdgeDirection::bidirection)
							result &= (outEdges.find(this->edgesId.at(getEdgeIndex(id))) !=
										outEdges.end());
						return result;
					} else // edge - node(*)
					{
						EdgeIdType edgeId = this->edgesId.at(getEdgeIndex(id - 1));
						NodeIdType nodeId = this->nodesId.at(getNodeIndex(id));
						std::string s;
						db->Get(leveldb::ReadOptions(), addSuffix(edgeId, edgeDataIdSuffix), &s);
						EdgeType e = strToDataByProtobuf< EdgeType >(s);
						//TODO: How to handle bidir edge?
						EdgeDirection edgeDir = e.direction; //edge Dir of actual edge
						bool result = false;
						if (edgeDir == EdgeDirection::next ||
									edgeDir == EdgeDirection::bidirection)
							result |= e.to == nodeId;
						if (edgeDir == EdgeDirection::prev ||
									edgeDir == EdgeDirection::bidirection)
							result |= e.from == nodeId;
						return result;
					}
				}

				bool checkRightConstrained(const std::size_t id)
				{
					if (isNode(id + 1)) // edge(*) - node
					{
						NodeIdType nodeId = this->nodesId.at(getNodeIndex(id + 1));
						// the edge dir in query
						EdgeDirection edgeDir = this->sql.first.edges.at(getEdgeIndex(id)).direction;

						inoutEdgesType outEdges = graph.getOutEdge(nodeId),
									   inEdges = graph.getInEdge(nodeId);

						bool result = true;
						if (edgeDir == EdgeDirection::next)
							result &= (inEdges.find(this->edgesId.at(getEdgeIndex(id))) !=
										inEdges.end());
						if (edgeDir == EdgeDirection::prev ||
									edgeDir == EdgeDirection::bidirection)
							result &= (outEdges.find(this->edgesId.at(getEdgeIndex(id))) !=
										outEdges.end());
						return result;
					} else // node(*) - edge
					{
						EdgeIdType edgeId = this->edgesId.at(getEdgeIndex(id + 1));
						NodeIdType nodeId = this->nodesId.at(getNodeIndex(id));
						std::string s;
						db->Get(leveldb::ReadOptions(), addSuffix(edgeId, edgeDataIdSuffix), &s);
						EdgeType e = strToDataByProtobuf< EdgeType >(s);
						//TODO: How to handle bidir edge?
						EdgeDirection edgeDir = e.direction; //edge Dir of actual edge
						bool result = false;
						if (edgeDir == EdgeDirection::prev ||
									edgeDir == EdgeDirection::bidirection)
							result |= e.to == nodeId;
						if (edgeDir == EdgeDirection::next ||
									edgeDir == EdgeDirection::bidirection)
							result |= e.from == nodeId;
						return result;
					}
				}

				template<typename T,
					typename = typename std::enable_if< std::is_same<T, NodeType>::value ||
						std::is_same<T, EdgeType>::value >::type >
						bool compareProperties(const Properties &props,
									T& val)
						{
							for (const Property& prop : props)
								if (!reflectedCompare(&val, prop.name,
												prop.relationship, prop.value))
									return false;
							return true;
						}

				bool isSelfConstrained(const std::size_t id)
				{
					if (isNode(id))
					{
						NodeIdType nodeId = nodesId.at(getNodeIndex(id));
						if (nodeId != sql.first.nodes.at(getNodeIndex(id)))
							return false;
						NodeType node = graph.getNode(nodeId);
						Properties queryProp = sql.first.nodes.at(getNodeIndex(id)).properties;
						return compareProperties(queryProp, node);
					} else
					{
						EdgeIdType edgeId = edgesId.at(getEdgeIndex(id));
						if (edgeId != sql.first.edges.at(getEdgeIndex(id)))
							return false;
						EdgeType edge = graph.getEdge(edgeId);
						Properties queryProp = sql.first.edges.at(getEdgeIndex(id)).properties;
						return compareProperties(queryProp, edge);
					}
				}

				bool hasNodeIdFoundBefore(const NodeIdType &nodeId,
							const std::size_t dedIdx)
				{
					return std::find(deductionSteps.begin(),
								deductionSteps.begin() + dedIdx,
								[&](const netalgo::DeductionTrait &dt)
								{
								return isNode(dt.id) &&
								nodesId.at(getNodeIndex(dt.id)) == nodeId;
								}
								);
				}

				std::size_t findDedIdxById(const std::size_t id)
				{
					return std::find(deductionSteps.begin(),
								deductionSteps.end(),
								[&](const netalgo::DeductionTrait &dt)
								{
								return dt.id == id;
								});
					deductionSteps.begin();
				}

				NodeIdType getNodeIdFromLeftEdge(const std::size_t nodeId)
				{
					EdgeType prevEdge = graph.getEdge(edgesId.at(getEdgeIndex(nodeId - 1)));
					netalgo::EdgeType queryEdge = sql.first.edges.at(getEdgeIndex(nodeId - 1));
					switch(queryEdge.direction)
					{
						case netalgo::EdgeDirection::next:
							return prevEdge.to();
							break;
						case netalgo::EdgeDirection::prev:
							return prevEdge.from();
							break;
						case netalgo::EdgeDirection::bidirection:
							NodeIdType thisNodeId;
							if (hasNodeIdFoundBefore(findDedIdxById(nodesId - 1), prevEdge.from()))
								thisNodeId = prevEdge.to();
							else
								thisNodeId = prevEdge.from();
							return thisNodeId;
							break;
					}
				}

				NodeIdType getNodeIdFromRightEdge(const std::size_t nodeId)
				{
					EdgeType nextEdge = graph.getEdge(edgesId.at(getEdgeIndex(nodeId + 1)));
					netalgo::EdgeType queryEdge = sql.first.edges.at(getEdgeIndex(nodeId + 1));
					switch(queryEdge.direction)
					{
						case netalgo::EdgeDirection::next:
							return nextEdge.to();
							break;
						case netalgo::EdgeDirection::prev:
							return nextEdge.from();
							break;
						case netalgo::EdgeDirection::bidirection:
							NodeIdType thisNodeId;
							if (hasNodeIdFoundBefore(findDedIdxById(nodesId + 1), nextEdge.from()))
								thisNodeId = nextEdge.to();
							else
								thisNodeId = nextEdge.from();
							return thisNodeId;
							break;
					}
				}

				bool findNextPossible(const int deductionIdx)
				{
					if (deductionIdx < 0) return false;
					DeductionTrait d = deductionSteps[deductionIdx];
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
									nodesId.at(getNodeIndex(id)) = getNodeIdFromLeftEdge(id);
								} while (!isSelfConstrained(id));
								break;
								//leftConstrained
							case DeductionTrait::ConstraintType::rightConstrained:
								do
								{
									if (!findNextPossible(deductionIdx - 1)) return false;
									nodesId.at(getNodeIndex(id)) = getNodeIdFromRightEdge(id);
								} while (!isSelfConstrained(id));
								break;
								//rightConstrained
							case DeductionTrait::ConstraintType::bothConstrained:
								do
								{
									if (!findNextPossible(deductionIdx - 1)) return false;
									nodesId.at(getNodeIndex(id)) = getNodeIdFromLeftEdge(id);
								} while (!isSelfConstrained(id) ||
											!checkRightConstrained(id)); //bothConstrained
								break;
							case DeductionTrait::ConstraintType::notConstrainted:
								{
									leveldb::Iterator *it = db->NewIterator(leveldb::ReadOptions());
									for(it->Seek(addSuffix(nodesId.at(getNodeIndex(id)),
														nodeDataIdSuffix));
												it->Valid();
												it->Next())
									{
										std::string key = it->key().ToString();
										if (key.find(nodeDataIdSuffix) != key.npos)
										{
											nodesId.at(getNodeIndex(id)) = key;
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
						EdgeType e = graph.getEdge(edgesId.at(getEdgeIndex(id)));
						switch(d.constraint)
						{
							case netalgo::DeductionTrait::leftConstrained:
								{
									netalgo::EdgeType edgeQuery =
										sql.first.edges.at(getEdgeIndex(id));
									bool firsttime = true;
									for(;;)
									{
										inoutEdgesType edgesSet;
										if (edgeQuery.direction == netalgo::EdgeDirection::next ||
													edgeQuery.direction == netalgo::EdgeDirection::bidirection)
											edgesSet = getOutEdge(nodesId(getNodeIndex(id - 1)));
										else
											edgesSet = getInEdge(nodesId(getNodeIndex(id - 1)));
										if (firsttime)
											for (auto it = edgesSet.find(e.id());
														it!=edgesSet.end();)
											{
												++it; if (it == edgesSet.end()) break;
												edgesId.at(getEdgeIndex(id)) = graph.getEdge(*it);
												if (isSelfConstrained(id)) return true;
											}
										else
											for (auto it = edgesSet.begin();
														it != edgesSet.end(); ++it)
											{
												edgesId.at(getEdgeIndex(id)) = graph.getEdge(*it);
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
										sql.first.edges.at(getEdgeIndex(id));
									bool firsttime = true;
									for(;;)
									{
										inoutEdgesType edgesSet;
										if (edgeQuery.direction == netalgo::EdgeDirection::prev ||
													edgeQuery.direction == netalgo::EdgeDirection::bidirection)
											edgesSet = getOutEdge(nodesId(getNodeIndex(id + 1)));
										else
											edgesSet = getInEdge(nodesId(getNodeIndex(id + 1)));
										if (firsttime)
											for (auto it = edgesSet.find(e.id());
														it!=edgesSet.end();)
											{
												++it; if (it == edgesSet.end()) break;
												edgesId.at(getEdgeIndex(id)) = graph.getEdge(*it);
												if (isSelfConstrained(id)) return true;
											}
										else
											for (auto it = edgesSet.begin();
														it != edgesSet.end(); ++it)
											{
												edgesId.at(getEdgeIndex(id)) = graph.getEdge(*it);
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
										sql.first.edges.at(getEdgeIndex(id));
									bool firsttime = true;
									for (;;)
									{
										inoutEdgesType edgesSet1, edgesSet2;
										if (edgeQuery.direction == netalgo::EdgeDirection::next ||
													edgeQuery.direction == netalgo::EdgeDirection::bidirection)
											edgesSet1 = getOutEdge(nodesId(getNodeIndex(id - 1)));
										else
											edgesSet1 = getInEdge(nodesId(getNodeIndex(id - 1)));
										if (edgeQuery.direction == netalgo::EdgeDirection::prev ||
													edgeQuery.direction == netalgo::EdgeDirection::bidirection)
											edgesSet2 = getOutEdge(nodesId(getNodeIndex(id + 1)));
										else
											edgesSet2 = getInEdge(nodesId(getNodeIndex(id + 1)));
										std::vector<EdgeIdType> intersectEdgesSet;
										std::set_intersection(edgesSet1.begin(),
													edgesSet1.end(),
													edgesSet2.begin(),
													edgesSet2.end(),
													std::back_inserter(intersectEdgesSet)
													);
										if (firsttime)
											for (auto it = intersectEdgesSet.find(e.id());
														it!=intersectEdgesSet.end();)
											{
												++it; if (it == intersectEdgesSet.end()) break;
												edgesId.at(getEdgeIndex(id)) = graph.getEdge(*it);
												if (isSelfConstrained(id)) return true;
											}
										else
											for (auto it = intersectEdgesSet.begin();
														it != intersectEdgesSet.end(); ++it)
											{
												edgesId.at(getEdgeIndex(id)) = graph.getEdge(*it);
												if (isSelfConstrained(id)) return true;
											}
										firsttime = false;
										if (!findNextPossible(deductionIdx - 1)) return false;
									}
									break;
								} //bothConstrained
							case netalgo::DeductionTrait::notConstrainted:
								{
									leveldb::Iterator *it = db->NewIterator(leveldb::ReadOptions());
									for(it->Seek(addSuffix(e.id(),
														edgeDataIdSuffix));
												it->Valid();
												it->Next())
									{
										std::string key = it->key().ToString();
										if (key.find(edgeDataIdSuffix) != key.npos)
										{
											nodesId.at(getEdgeIndex(id)) = key;
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


			protected:
				std::vector< NodeIdType > nodesId, nextNodesId;
				std::vector< EdgeIdType > edgesId, nextEdgesId;
				leveldb::DB *db;
			public:
				typedef LevelDbGraphResult<NodeType, EdgeType> value_type;
				typedef value_type& reference;
				typedef value_type* pointer;
				LevelDbGraphIterator(LevelDbGraph<NodeType, EdgeType>& graphP)
					:
						graph(graphP),
						db(graphP.db)
			{}
				LevelDbGraphIterator& operator++()
				{
					if (isEnd)
						throw std::runtime_error("++ on a past-end leveldbGraph iterator is invalid");
					if (!nextNodesId.empty())
					{
						if (!findNextPossible(nodesId.size() * 2 - 2))
							isEnd = true;
						nodesId.swap(nextNodesId);
						edgesId.swap(nextEdgesId);
					} else
					{
						assert(findNextPossible(nodesId.size() * 2 - 2));
						nodesId.swap(nextNodesId);
						edgesId.swap(nextEdgesId);
						if (!findNextPossible(nodesId.size() * 2 - 2))
							isEnd = true;
						nodesId.swap(nextNodesId);
						edgesId.swap(nextEdgesId);
					}
				}
				reference operator*()
				{
					if (isEnd)
						throw std::runtime_error("* on a past-end leveldbGraph iterator is invalid");
					std::unordered_set< std::string > returnName;
					for (const auto& item : sql.second.returnName)
						returnName.insert(item);
					for (std::size_t i=0; i < nodesId.size() * 2 - 1; ++i)
						if (isNode(i))
						{
							if (returnName.find(nodesId.at(getNodeIndex(i))) !=
										returnName.end())
								result.nodes.insert(std::make_pair(nodesId.at(getNodeIndex(i)),
												graph.getNode(nodesId.at(getNodeIndex(i)))));
						} else
							if (returnName.find(edgesId.at(getEdgeIndex(i))) !=
										returnName.end())
								result.edges.insert(std::make_pair(edgesId.at(getEdgeIndex(i)),
												graph.getEdge(edgesId.at(getEdgeIndex))));
					return result;
				}

				pointer operator->()
				{
					if (isEnd)
						throw std::runtime_error("-> on a past-end leveldbGraph iterator is invalid");
					return & (operator*());
				}

				bool operator==(const LevelDbGraphIterator& other)
				{
					if (isEnd ^ other.isEnd) return false;
					if (isEnd && other.isEnd) return true;
					if (nodesId.size() != other.size()) return false;
					if (edgesId.size() != other.size()) return false;
					for (std::size_t i=0; i<nodesId.size(); ++i)
						if (other.nodesId[i] != nodesId[i])
							return false;
					for (std::size_t i=0; i<edgesId.size(); ++i)
						if (other.edgesId[i] != edgesId[i])
							return false;
					return true;
				}

				bool operator!=(const LevelDbGraphIterator& other)
				{
					return !(operator==(other));
				}

				friend LevelDbGraph<NodeType, EdgeType>;
		};
}
#endif
