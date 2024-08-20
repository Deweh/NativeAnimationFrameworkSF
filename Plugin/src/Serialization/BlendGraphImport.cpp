#include "BlendGraphImport.h"
#include "simdjson.h"
#include "Animation/Procedural/PActorNode.h"
#include "Animation/Procedural/PFullAnimationNode.h"

namespace Serialization
{
	std::unique_ptr<Animation::Procedural::PGraph> BlendGraphImport::LoadGraph(const std::filesystem::path& a_filePath, const std::string_view a_skeleton)
	{
		using namespace Animation::Procedural;

		std::unique_ptr<PGraph> result;
		try {
			simdjson::ondemand::parser parser;
			auto jsonString = simdjson::padded_string::load(a_filePath.generic_string());
			simdjson::ondemand::document doc = parser.iterate(jsonString);
			if (!doc.is_alive()) {
				throw std::exception{ "Failed to parse JSON." };
			}

			uint64_t version = doc["version"];
			if (version > 1) {
				throw std::exception{ "Blend graph uses an unsupported format version." };
			}

			result = std::make_unique<PGraph>();
			auto& nodeTypes = GetRegisteredNodeTypes();

			std::unordered_map<size_t, PNode*> nodeIdMap;
			auto nodes = doc["nodes"].get_array();

			//Pass 1: Parse all node data.
			std::vector<PEvaluationResult> values;
			std::string_view stringVal;
			double numVal;
			uint64_t intVal;
			for (auto n : nodes) {
				std::string_view typeName = n["type"];
				PNode::Registration* typeInfo = nullptr;
				if (auto iter = nodeTypes.find(typeName); iter != nodeTypes.end()) {
					typeInfo = iter->second;
				} else {
					throw std::exception{ "Unknown node type." };
				}

				std::unique_ptr<PNode> currentNode = typeInfo->createFunctor();
				uint64_t id = n["id"];
				nodeIdMap[id] = currentNode.get();

				if (typeInfo == &PActorNode::_reg) {
					if (result->actorNode != 0) {
						throw std::exception{ "Blend graph contains multiple actor nodes." };
					} else {
						result->actorNode = reinterpret_cast<uint64_t>(currentNode.get());
					}
				}

				if (!typeInfo->inputs.empty()) {
					auto inputs = n["inputs"];
					for (auto& i : typeInfo->inputs) {
						uint64_t targetNode = inputs[i.name].get_array().at(0);
						//We don't have pointers to all the nodes yet, so just store the uint64 ID as a pointer until the next step.
						currentNode->inputs.push_back(targetNode);
					}
				}
				if (!typeInfo->customValues.empty()) {
					auto customVals = n["values"];
					values.clear();
					
					for (auto& v : typeInfo->customValues) {
						auto curVal = customVals[v.first];

						switch (v.second) {
						case PEvaluationType<float>:
							numVal = curVal;
							values.emplace_back(static_cast<float>(numVal));
							break;
						case PEvaluationType<std::string>:
							stringVal = curVal;
							values.emplace_back(std::string{ stringVal });
							break;
						case PEvaluationType<uint64_t>:
							intVal = curVal;
							values.emplace_back(intVal);
						}
					}
					if (!currentNode->SetCustomValues(values, a_skeleton)) {
						throw std::runtime_error{ std::format("Failed to process custom value(s) for '{}' node.", typeInfo->typeName) };
					}
				}

				result->nodes.emplace_back(std::move(currentNode));
			}

			if (result->actorNode == 0) {
				throw std::exception{ "Blend graph contains no actor node." };
			}

			//Pass 2: Connect inputs together.
			for (auto& n : result->nodes) {
				auto destTypeInfo = n->GetTypeInfo();

				// Find the animation node with the longest duration.
				if (destTypeInfo == &PFullAnimationNode::_reg) {
					if (auto ptr = reinterpret_cast<PFullAnimationNode*>(result->loopTrackingNode);
						ptr == nullptr || ptr->anim->data->duration() < static_cast<PFullAnimationNode*>(n.get())->anim->data->duration())
					{
						result->loopTrackingNode = reinterpret_cast<uint64_t>(n.get());
					}
				}

				auto destInputIter = destTypeInfo->inputs.begin();
				for (auto& i : n->inputs) {
					if (auto iter = nodeIdMap.find(i); iter != nodeIdMap.end()) {
						if (iter->second->GetTypeInfo()->output == destInputIter->evalType) {
							i = reinterpret_cast<uint64_t>(iter->second);
						} else {
							throw std::exception{ "Node connection has mismatched input/output types." };
						}
					} else if(destInputIter->optional == true) {
						i = 0;
					} else {
						throw std::exception{ "Blend graph refers to non-existant node ID." };
					}
					destInputIter++;
				}
			}

			//TODO: Add 3rd pass for discarding nodes that are disconnected from the graph flow.

			std::vector<PNode*> sortedNodes;
			sortedNodes.reserve(result->nodes.size());

			if (!result->SortNodes(sortedNodes)) {
				throw std::exception{ "Failed to sort nodes (either due to a dependency loop or too many nodes.)" };
			}

			result->InsertCacheReleaseNodes(sortedNodes);
			result->PointersToIndexes(sortedNodes);
			result->EmplaceNodeOrder(sortedNodes);
		}
		catch (const std::exception& ex) {
			WARN("{}", ex.what());
			return nullptr;
		}

		return result;
	}
}