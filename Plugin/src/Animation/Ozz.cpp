#include "Ozz.h"
#include "FileManager.h"
#include "Transform.h"

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

std::unique_ptr<std::vector<ozz::math::Transform>> Animation::OzzSkeleton::GetRestPose() const
{
	auto result = std::make_unique<std::vector<ozz::math::Transform>>();
	auto restPoses = data->joint_rest_poses();
	result->reserve(restPoses.size());

	ozz::math::Transform ot = ozz::math::Transform::identity();
	Transform::ExtractSoaTransforms(restPoses, [&result, &ot](size_t i, const Animation::Transform& t) {
		ot.rotation = { t.rotate.x, t.rotate.y, t.rotate.z, t.rotate.w };
		ot.translation = { t.translate.x, t.translate.y, t.rotate.z };
		result->push_back(ot);
	});
	return result;
}
