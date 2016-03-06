#ifndef GRAPH_BACKEND_LEVELDBGRAPH_DEDUCTION
#define GRAPH_BACKEND_LEVELDBGRAPH_DEDUCTION

#include "graphdsl.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
namespace netalgo
{
	namespace
	{
		class DeductionSteps;
		// 0: first node
		// 1: first edge
		// (0)-[1]-(2)-[3]-(4)
		inline bool isNode(const int idx) { return idx % 2 == 0; }
		inline bool isEdge(const int idx) { return !isNode(idx); }
		inline int getNodeIndex(const int idx) //start from 0
		{
			assert(isNode(idx));
			return idx / 2;
		}
		inline int getEdgeIndex(const int idx)
		{
			assert(isEdge(idx));
			return idx / 2;
		}

		inline int nodeIndexToGlobalIndex(const int nodeidx)
		{
			return nodeidx * 2;
		}

		inline int edgeIndexToGlobalIndex(const int edgeidx)
		{
			return edgeidx * 2 + 1;
		}

		struct DeductionTrait
		{
			std::size_t id;
			enum ConstraintType
			{
				leftConstrained,
				rightConstrained,
				bothConstrained,
				notConstrainted
			} constraint;
			bool direct;

			DeductionTrait() = default;

			DeductionTrait(std::size_t idP,
						ConstraintType constraintP,
						bool directP):
				id(idP),
				constraint(constraintP),
				direct(directP)
			{}

		};

		bool acquiredDirectly(const netalgo::Properties& prop)
		{
			for (const auto& item : prop)
				if (item.name == "id" &&
							item.relationship == netalgo::Relationship::equal
				   )
					return true;
			return false;
		}

		typedef std::vector< DeductionTrait > DeductionStepsType;

		void addToDeductionSteps(DeductionStepsType& deductionSteps,
					int leftBound,
					int rightBound, int maxId) // add Points in (left, right)
		{
			std::size_t middleCnt = rightBound - leftBound - 1;
			// (2)-[3]->(4)-[5]-(6)
			for(std::size_t j=0; j<middleCnt; ++j)
			{
				std::size_t currentId;
				if (j % 2 == 0)
					currentId = leftBound + j / 2 + 1;
				else
					currentId = rightBound - j/2 - 1;
				DeductionTrait dt;
				dt.id = currentId;
				dt.direct = false;
				if (j == middleCnt - 1)
					dt.constraint = DeductionTrait::bothConstrained;
				else
					if (j%2 == 0)
						dt.constraint = DeductionTrait::leftConstrained;
					else
						dt.constraint = DeductionTrait::rightConstrained;

				if (leftBound == -1)
				{
					if (dt.constraint == DeductionTrait::leftConstrained)
						dt.constraint = DeductionTrait::notConstrainted;
					else if (dt.constraint == DeductionTrait::bothConstrained)
						dt.constraint = DeductionTrait::rightConstrained;
				}

				if (rightBound > maxId)
				{
					if (dt.constraint == DeductionTrait::rightConstrained)
						dt.constraint =DeductionTrait::notConstrainted;
					else if (dt.constraint == DeductionTrait::bothConstrained)
						dt.constraint = DeductionTrait::leftConstrained;
				}

				deductionSteps.push_back(dt);
			}
		}

		DeductionStepsType
			generateDeductionSteps(const netalgo::GraphSqlSentence& q)
			{
				const netalgo::SelectSentence& selectSentece = q.first;
				std::size_t nodesCnt = selectSentece.nodes.size();
				std::vector< DeductionTrait > result;

				for (std::size_t i = 0; i<nodesCnt*2 - 1; ++i)
				{
					bool couldAcquiredDirectly;
					if (isNode(i))
					{
						const auto& node = selectSentece.nodes.at(getNodeIndex(i));
						couldAcquiredDirectly = acquiredDirectly(node.properties);
					} else
					{
						const auto& edge = selectSentece.edges.at(getEdgeIndex(i));
						couldAcquiredDirectly = acquiredDirectly(edge.properties);
					}
					if (couldAcquiredDirectly)
						result.push_back(DeductionTrait(
										i,
										(!result.empty() && (*result.rbegin()).id == i-1) ?
										DeductionTrait::leftConstrained :
										DeductionTrait::notConstrainted,
										true
										));
				} //first loop for points that could be directly acquired

				std::size_t directAcquiredSize = result.size();
				for(std::size_t i = 0; i<directAcquiredSize; ++i)
				{
					std::size_t now = result[i].id;
					std::size_t prev = i > 0 ? result[i-1].id : -1;
					addToDeductionSteps(result, prev, now, nodesCnt *2 -2);
				}
				addToDeductionSteps(result, result[directAcquiredSize - 1].id, nodesCnt * 2 - 1, nodesCnt * 2 - 2);
				return result;
			}

	} //anonymous namespace
}
#endif
