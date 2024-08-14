#include "IAnimationFile.h"
#include "FileManager.h"

namespace Animation
{
	IAnimationFile::~IAnimationFile()
	{
		FileManager::GetSingleton()->OnAnimationDestroyed(this);
	}
}