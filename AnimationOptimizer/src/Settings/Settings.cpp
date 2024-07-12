#include "Settings.h"

namespace Settings
{
	std::map<std::string, size_t> idxMap;
	std::vector<std::string> morphs;

	void SetFaceMorphs(const std::vector<std::string>& a_morphs)
	{
		morphs = a_morphs;
		idxMap.clear();
		for (size_t i = 0; i < morphs.size(); i++) {
			idxMap[a_morphs[i]] = i;
		}
	}

	const std::map<std::string, size_t>& GetFaceMorphIndexMap()
	{
		return idxMap;
	}

	const std::vector<std::string>& GetFaceMorphs()
	{
		return morphs;
	}
}