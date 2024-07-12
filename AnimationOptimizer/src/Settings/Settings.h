#pragma once
#include "Animation/Ozz.h"

namespace Settings
{
	void SetFaceMorphs(const std::vector<std::string>& a_morphs);
	const std::map<std::string, size_t>& GetFaceMorphIndexMap();
	const std::vector<std::string>& GetFaceMorphs();
}