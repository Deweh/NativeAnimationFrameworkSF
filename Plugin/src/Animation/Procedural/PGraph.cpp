#include "PGraph.h"
#include "PInternalCacheReleaseNode.h"
#include "PActorNode.h"
#include "Animation/Generator.h"

namespace Animation::Procedural
{
	bool PGraph::SortNodes()
	{
		sortedNodes.clear();
		std::unordered_set<PNode*> visited;
		std::unordered_set<PNode*> recursionStack;

		for (auto& node : nodes) {
			if (visited.find(node.get()) == visited.end()) {
				if (!DepthFirstNodeSort(node.get(), 0, visited, recursionStack)) {
					return false;
				}
			}
		}

		return true;
	}

	void PGraph::InsertCacheReleaseNodes()
	{
		std::map<PNode*, size_t> lastUsageMap;
		for (auto iter = sortedNodes.begin(); iter != sortedNodes.end(); iter++) {
			auto& n = *iter;
			size_t idx = std::distance(sortedNodes.begin(), iter);
			lastUsageMap[n] = idx;
			for (auto& i : n->inputs) {
				lastUsageMap[i] = idx;
			}
		}

		std::vector<std::pair<PNode*, size_t>> sortedLastUsages(lastUsageMap.begin(), lastUsageMap.end());
		std::sort(sortedLastUsages.begin(), sortedLastUsages.end(),
			[](const auto& a, const auto& b) { return a.second > b.second; });

		for (const auto& [n, idx] : sortedLastUsages) {
			if (auto typeInfo = n->GetTypeInfo(); !typeInfo || typeInfo->output != PEvaluationType<PoseCache::Handle>) {
				continue;
			}

			//If the last user of this pose is the actor output node, don't free it.
			if (IsNodeOfType<PActorNode>(sortedNodes[idx])) {
				continue;
			}

			auto& releaseNode = nodes.emplace_back(std::make_unique<PInternalCacheReleaseNode>());
			releaseNode->inputs.emplace_back(n);

			sortedNodes.insert(sortedNodes.begin() + (idx + 1), releaseNode.get());
		}
	}

	std::span<ozz::math::SoaTransform> PGraph::Evaluate(InstanceData& a_graphInst, PoseCache& a_poseCache)
	{
		auto instIter = a_graphInst.nodeInstances.begin();
		for (auto& n : sortedNodes) {
			a_graphInst.resultMap[n] = std::move(n->Evaluate(instIter->get(), a_poseCache, a_graphInst.resultMap));
			instIter++;
		}

		auto resultNode = std::get<PNode*>(a_graphInst.resultMap[actorNode]);
		return std::get<PoseCache::Handle>(a_graphInst.resultMap[resultNode]).get();
	}

	void PGraph::AdvanceTime(InstanceData& a_graphInst, float a_deltaTime)
	{
		auto instIter = a_graphInst.nodeInstances.begin();
		for (auto& n : sortedNodes) {
			n->AdvanceTime(instIter->get(), a_deltaTime);
			instIter++;
		}
	}

	void PGraph::InitInstanceData(InstanceData& a_graphInst)
	{
		for (auto& n : sortedNodes) {
			a_graphInst.nodeInstances.emplace_back(n->CreateInstanceData());
		}
	}

	std::unique_ptr<Generator> PGraph::CreateGenerator()
	{
		return std::make_unique<ProceduralGenerator>(std::static_pointer_cast<PGraph>(shared_from_this()));
	}

	bool PGraph::DepthFirstNodeSort(PNode* a_node, size_t a_depth, std::unordered_set<PNode*>& a_visited, std::unordered_set<PNode*>& a_recursionStack)
	{
		if (a_depth > MAX_DEPTH) {
			// Maximum recursion depth exceeded.
			return false;
		}

		a_visited.insert(a_node);
		a_recursionStack.insert(a_node);

		for (PNode* dependency : a_node->inputs) {
			if (a_recursionStack.find(dependency) != a_recursionStack.end()) {
				// Cycle detected in graph.
				return false;
			}

			if (a_visited.find(dependency) == a_visited.end()) {
				if (!DepthFirstNodeSort(dependency, a_depth + 1, a_visited, a_recursionStack)) {
					return false;
				}
			}
		}

		a_recursionStack.erase(a_node);
		sortedNodes.push_back(a_node);
		return true;
	}
}