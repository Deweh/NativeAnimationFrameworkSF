#include "Ozz.h"
#include "FileManager.h"

Animation::OzzAnimation::~OzzAnimation() noexcept
{
	FileManager::GetSingleton()->OnAnimationDestroyed(this);
}
