#pragma once
#include "Animation/Procedural/PGraph.h"

namespace Serialization
{
	class BlendGraphImport
	{
	public:
		static std::unique_ptr<Animation::Procedural::PGraph> LoadGraph(const std::filesystem::path& a_filePath);
	};
}