#pragma once
#include "Animation/Procedural/PGraph.h"

namespace Serialization
{
	class BlendGraphImport
	{
	public:
		inline static constexpr const char* FILE_EXTENSION{ ".bt" };

		static std::unique_ptr<Animation::Procedural::PGraph> LoadGraph(const std::filesystem::path& a_filePath, const std::string_view a_skeleton);
	};
}