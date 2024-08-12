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
			for (auto n : nodes) {
				std::string_view typeName = n["type"];
				PNode::Registration* typeInfo = nullptr;
				if (auto iter = nodeTypes.find(typeName); iter != nodeTypes.end()) {
					typeInfo = iter->second;
				} else {
					throw std::exception{ "Unknown node type." };
				}

				std::unique_ptr<PNode> currentNode = typeInfo->createFunctor();
				if (!typeInfo->inputs.empty()) {
					auto inputs = n["inputs"];
					for (auto& i : typeInfo->inputs) {
						size_t targetNode = inputs[i.first][0];
					}
				}
			}
		}
		catch (const std::exception&) {
			return nullptr;
		}

		return result;
	}
}