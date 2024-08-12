#pragma once
#include "PNode.h"
#include "Animation/PoseCache.h"

namespace Animation::Procedural
{
	class PGraph
	{
	public:
		inline static constexpr size_t MAX_DEPTH{ 100 };

		struct InstanceData
		{
			std::vector<std::unique_ptr<PNodeInstanceData>> nodeInstances;
			std::unordered_map<PNode*, PEvaluationResult> resultMap;
		};

		std::vector<std::unique_ptr<PNode>> nodes;
		std::vector<PNode*> sortedNodes;
		PNode* actorNode;

		bool SortNodes();
		void InsertCacheReleaseNodes();
		std::span<ozz::math::SoaTransform> Evaluate(InstanceData& a_graphInst, PoseCache& a_poseCache);
		void AdvanceTime(InstanceData& a_graphInst, float a_deltaTime);
		void InitInstanceData(InstanceData& a_graphInst, const OzzSkeleton* a_skeleton);

	private:
		bool DepthFirstNodeSort(PNode* a_node, size_t a_depth, std::unordered_set<PNode*>& a_visited, std::unordered_set<PNode*>& a_recursionStack);
	};
}