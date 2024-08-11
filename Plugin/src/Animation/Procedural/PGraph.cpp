#include "PGraph.h"

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

	std::span<ozz::math::SoaTransform> PGraph::Evaluate(InstanceData& a_graphInst, PoseCache& a_poseCache)
	{
		auto instIter = a_graphInst.nodeInstances.begin();
		for (auto& n : sortedNodes) {
			a_graphInst.resultMap[n] = std::move(n->Evaluate(instIter->get(), a_poseCache, a_graphInst.resultMap));
			instIter++;
		}

		// TODO: Save the index of the actor output node, in case it's not the last one.
		return std::get<PoseCache::Handle>(a_graphInst.resultMap[sortedNodes.back()]).get();
	}

	void PGraph::AdvanceTime(InstanceData& a_graphInst, float a_deltaTime)
	{
		auto instIter = a_graphInst.nodeInstances.begin();
		for (auto& n : sortedNodes) {
			n->AdvanceTime(instIter->get(), a_deltaTime);
			instIter++;
		}
	}

	void PGraph::InitInstanceData(InstanceData& a_graphInst, const OzzSkeleton* a_skeleton)
	{
		for (auto& n : sortedNodes) {
			a_graphInst.nodeInstances.emplace_back(n->CreateInstanceData(a_skeleton));
		}
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