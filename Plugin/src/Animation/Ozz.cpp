#include "Ozz.h"
#include "FileManager.h"

size_t Animation::OzzAnimation::GetSize()
{
	size_t result = 0;

	if (data) {
		result += data->size();
	}

	if (faceData) {
		for (auto& t : faceData->tracks) {
			result += t.size();
		}
	}

	return result;
}

Animation::OzzAnimation::~OzzAnimation() noexcept
{
	FileManager::GetSingleton()->OnAnimationDestroyed(this);
}
