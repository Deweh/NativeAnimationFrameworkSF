#include "BlendGraphImport.h"
#include "simdjson.h"

namespace Serialization
{
	std::unique_ptr<Animation::Procedural::PGraph> BlendGraphImport::LoadGraph(const std::filesystem::path& a_filePath)
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

			int version = doc["id"];
			if (version > 1) {
				throw std::exception{ "Blend graph uses an unsupported format version." };
			}

			result = std::make_unique<PGraph>();
			auto& nodeTypes = GetRegisteredNodeTypes();

			std::unordered_map<size_t, PNode*> nodeIdMap;
			auto nodes = doc["nodes"].get_array();

			//Pass 1: Parse all node data.
			std::vector<PEvaluationResult> values;
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

				if (typeName == "actor") {
					result->actorNode = currentNode.get();
				}

				if (!typeInfo->inputs.empty()) {
					auto inputs = n["inputs"];
					for (auto& i : typeInfo->inputs) {
						uint64_t targetNode = inputs[i.first][0];
						//We don't have pointers to all the nodes yet, so just store the uint64 ID as a pointer until the next step.
						currentNode->inputs.push_back(reinterpret_cast<PNode*>(targetNode));
					}
				}
				if (!typeInfo->customValues.empty()) {
					auto customVals = n["values"];
					values.clear();
					std::string_view stringVal;
					double numVal;
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
						}
					}
					currentNode->SetCustomValues(values);
				}

				result->nodes.emplace_back(std::move(currentNode));
			}

			//Pass 2: Connect inputs together.
			for (auto& n : result->nodes) {
				for (auto& i : n->inputs) {
					if (auto iter = nodeIdMap.find(reinterpret_cast<uint64_t>(i)); iter != nodeIdMap.end()) {
						i = iter->second;
					} else {
						throw std::exception{ "Blend graph refers to non-existant node ID." };
					}
				}
			}

			if (!result->SortNodes()) {
				throw std::exception{ "Failed to sort nodes (either due to a dependency loop or too many nodes.)" };
			}
		}
		catch (const std::exception& ex) {
			ERROR("{}", ex.what());
			return nullptr;
		}

		return result;
	}
}