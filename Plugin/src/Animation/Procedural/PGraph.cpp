#include "PGraph.h"
#include "PInternalCacheReleaseNode.h"
#include "PFullAnimationNode.h"
#include "PVariableNode.h"
#include "PActorNode.h"
#include "Animation/Generator.h"

namespace Animation::Procedural
{
	std::span<ozz::math::SoaTransform> PGraph::Evaluate(InstanceData& a_graphInst, PoseCache& a_poseCache)
	{
		auto instIter = a_graphInst.nodeInstances.begin();
		for (auto iter = nodes.begin(); iter != nodes.end(); iter++) {
			a_graphInst.results[std::distance(nodes.begin(), iter)] = std::move((*iter)->Evaluate(instIter->get(), a_poseCache, a_graphInst));
			instIter++;
		}

		auto resultIdx = std::get<uint64_t>(a_graphInst.results[actorNode]);
		return std::get<PoseCache::Handle>(a_graphInst.results[resultIdx]).get();
	}

	bool PGraph::AdvanceTime(InstanceData& a_graphInst, float a_deltaTime)
	{
		auto instIter = a_graphInst.nodeInstances.begin();
		for (auto& n : nodes) {
			n->AdvanceTime(instIter->get(), a_deltaTime);
			instIter++;
		}
		if (loopTrackingNode != UINT64_MAX) {
			return static_cast<PFullAnimationNode::InstanceData*>(a_graphInst.nodeInstances[loopTrackingNode].get())->looped;
		}
		return false;
	}

	void PGraph::Synchronize(InstanceData& a_graphInst, InstanceData& a_ownerInst, PGraph* a_ownerGraph, float a_correctionDelta)
	{
		if (a_graphInst.lastSyncOwner != std::addressof(a_ownerInst)) {
			a_graphInst.syncMap.clear();

			for (auto selfIter = nodes.begin(); selfIter != nodes.end(); selfIter++) {
				auto& n = *selfIter;
				if (n->syncId == UINT64_MAX)
					continue;

				for (auto ownerIter = a_ownerGraph->nodes.begin(); ownerIter != a_ownerGraph->nodes.end(); ownerIter++) {
					auto& oN = *ownerIter;
					if (oN->syncId == n->syncId && oN->GetTypeInfo() == n->GetTypeInfo()) {
						auto& d = a_graphInst.syncMap.emplace_back();
						d.node = n.get();
						d.selfInstData = a_graphInst.nodeInstances[std::distance(nodes.begin(), selfIter)].get();
						d.ownerInstData = a_ownerInst.nodeInstances[std::distance(a_ownerGraph->nodes.begin(), ownerIter)].get();
						break;
					}
				}
			}

			a_graphInst.lastSyncOwner = std::addressof(a_ownerInst);
		}

		for (auto& i : a_graphInst.syncMap) {
			i.node->Synchronize(i.selfInstData, i.ownerInstData, a_correctionDelta);
		}
	}

	void PGraph::InitInstanceData(InstanceData& a_graphInst)
	{
		a_graphInst.nodeInstances.reserve(nodes.size());
		for (auto& n : nodes) {
			a_graphInst.nodeInstances.emplace_back(n->CreateInstanceData());
			if (IsNodeOfType<PVariableNode>(n.get())) {
				a_graphInst.variableMap.emplace(static_cast<PVariableNode*>(n.get())->name, static_cast<PVariableInstance*>(a_graphInst.nodeInstances.back().get()));
			}
		}
		a_graphInst.results.resize(nodes.size());
	}

	std::unique_ptr<Generator> PGraph::CreateGenerator()
	{
		return std::make_unique<ProceduralGenerator>(std::static_pointer_cast<PGraph>(shared_from_this()));
	}

	bool PGraph::SortNodes(std::vector<PNode*>& a_sortedNodes)
	{
		std::unordered_set<PNode*> visited;
		std::unordered_set<PNode*> recursionStack;

		for (auto& node : nodes) {
			if (visited.find(node.get()) == visited.end()) {
				if (!DepthFirstNodeSort(node.get(), 0, visited, recursionStack, a_sortedNodes)) {
					return false;
				}
			}
		}

		return true;
	}

	void PGraph::InsertCacheReleaseNodes(std::vector<PNode*>& a_sortedNodes)
	{
		std::map<PNode*, size_t> lastUsageMap;
		for (auto iter = a_sortedNodes.begin(); iter != a_sortedNodes.end(); iter++) {
			auto& n = *iter;
			size_t idx = std::distance(a_sortedNodes.begin(), iter);
			lastUsageMap[n] = idx;
			for (auto& i : n->inputs) {
				if (i == 0)
					continue;

				lastUsageMap[reinterpret_cast<PNode*>(i)] = idx;
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
			if (IsNodeOfType<PActorNode>(a_sortedNodes[idx])) {
				continue;
			}

			auto& releaseNode = nodes.emplace_back(std::make_unique<PInternalCacheReleaseNode>());
			releaseNode->inputs.emplace_back(reinterpret_cast<uint64_t>(n));

			a_sortedNodes.insert(a_sortedNodes.begin() + (idx + 1), releaseNode.get());
		}
	}

	void PGraph::PointersToIndexes(std::vector<PNode*>& a_sortedNodes)
	{
		std::unordered_map<PNode*, size_t> idxMap;
		for (auto iter = a_sortedNodes.begin(); iter != a_sortedNodes.end(); iter++) {
			idxMap[*iter] = std::distance(a_sortedNodes.begin(), iter);
		}
		actorNode = idxMap[reinterpret_cast<PNode*>(actorNode)];

		if (loopTrackingNode != 0) {
			loopTrackingNode = idxMap[reinterpret_cast<PNode*>(loopTrackingNode)];
		} else {
			loopTrackingNode = UINT64_MAX;
		}

		for (auto& n : a_sortedNodes) {
			for (auto& i : n->inputs) {
				if (i != 0) {
					i = idxMap[reinterpret_cast<PNode*>(i)];
				} else {
					i = UINT64_MAX;
				}
			}
		}
	}

	void PGraph::EmplaceNodeOrder(std::vector<PNode*>& a_sortedNodes)
	{
		std::vector<std::unique_ptr<PNode>> result;
		result.resize(nodes.size());

		std::map<PNode*, size_t> nodeOrder;
		for (auto iter = a_sortedNodes.begin(); iter != a_sortedNodes.end(); iter++) {
			nodeOrder.emplace(*iter, std::distance(a_sortedNodes.begin(), iter));
		}

		for (auto& n : nodes) {
			if (auto iter = nodeOrder.find(n.get()); iter != nodeOrder.end() && iter->second < result.size()) {
				result[iter->second] = std::move(n);
			} else {
				throw std::exception{ "Internal error: sorted nodes do not match node set." };
			}
		}

		nodes = std::move(result);
	}

	bool PGraph::DepthFirstNodeSort(PNode* a_node, size_t a_depth, std::unordered_set<PNode*>& a_visited, std::unordered_set<PNode*>& a_recursionStack, std::vector<PNode*>& a_sortedNodes)
	{
		if (a_depth > MAX_DEPTH) {
			// Maximum recursion depth exceeded.
			return false;
		}

		a_visited.insert(a_node);
		a_recursionStack.insert(a_node);

		for (uint64_t ptr : a_node->inputs) {
			if (ptr == 0)
				continue;

			auto dependency = reinterpret_cast<PNode*>(ptr);
			if (a_recursionStack.find(dependency) != a_recursionStack.end()) {
				// Cycle detected in graph.
				return false;
			}

			if (a_visited.find(dependency) == a_visited.end()) {
				if (!DepthFirstNodeSort(dependency, a_depth + 1, a_visited, a_recursionStack, a_sortedNodes)) {
					return false;
				}
			}
		}

		a_recursionStack.erase(a_node);
		a_sortedNodes.push_back(a_node);
		return true;
	}
}