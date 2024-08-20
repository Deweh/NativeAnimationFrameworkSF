#pragma once
#include "PNode.h"
#include "Animation/PoseCache.h"
#include "Animation/IAnimationFile.h"

namespace Serialization
{
	class BlendGraphImport;
}

namespace Animation::Procedural
{
	class PGraph : public IAnimationFile
	{
	public:
		inline static constexpr size_t MAX_DEPTH{ 100 };
		using InstanceData = PEvaluationContext;

		std::vector<std::unique_ptr<PNode>> nodes;
		uint64_t actorNode = 0;
		uint64_t loopTrackingNode = 0;
		
		std::span<ozz::math::SoaTransform> Evaluate(InstanceData& a_graphInst, PoseCache& a_poseCache);
		bool AdvanceTime(InstanceData& a_graphInst, float a_deltaTime);
		void Synchronize(InstanceData& a_graphInst, InstanceData& a_ownerInst, PGraph* a_ownerGraph, float a_correctionDelta);
		void InitInstanceData(InstanceData& a_graphInst);
		virtual std::unique_ptr<Generator> CreateGenerator() override;

	protected:
		friend class Serialization::BlendGraphImport;

		bool SortNodes(std::vector<PNode*>& a_sortedNodes);
		void InsertCacheReleaseNodes(std::vector<PNode*>& a_sortedNodes);
		void PointersToIndexes(std::vector<PNode*>& a_sortedNodes);
		void EmplaceNodeOrder(std::vector<PNode*>& a_sortedNodes);

	private:
		bool DepthFirstNodeSort(PNode* a_node, size_t a_depth, std::unordered_set<PNode*>& a_visited, std::unordered_set<PNode*>& a_recursionStack, std::vector<PNode*>& a_sortedNodes);
	};
}