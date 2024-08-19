#include "Ozz.h"
#include "FileManager.h"
#include "Transform.h"
#include "Generator.h"

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

std::unique_ptr<Animation::Generator> Animation::OzzAnimation::CreateGenerator()
{
	return std::make_unique<LinearClipGenerator>(std::static_pointer_cast<OzzAnimation>(shared_from_this()));
}

std::unique_ptr<std::vector<ozz::math::Transform>> Animation::OzzSkeleton::GetRestPose() const
{
	auto result = std::make_unique<std::vector<ozz::math::Transform>>();
	auto restPoses = data->joint_rest_poses();
	result->resize(data->num_joints());

	ozz::math::Transform ot = ozz::math::Transform::identity();
	Transform::ExtractSoaTransforms(restPoses, [&result, &ot](size_t i, const Animation::Transform& t) {
		if (i >= result->size())
			return;

		ot.rotation = { t.rotate.x, t.rotate.y, t.rotate.z, t.rotate.w };
		ot.translation = { t.translate.x, t.translate.y, t.rotate.z };
		result->at(i) = ot;
	});
	return result;
}
