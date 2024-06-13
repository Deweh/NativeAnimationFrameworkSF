#include "IKTwoBoneData.h"

namespace Animation
{
	IKTwoBoneData::IKTwoBoneData()
	{
		job.weight = 1.0f;
	}

	void IKTwoBoneData::TransitionIn(float a_duration)
	{
		transition.mode = IKTwoBoneData::TransitionMode::kIn;
		transition.duration = a_duration;
		transition.currentTime = 0.0f;
		job.weight = 0.0f;
	}

	void IKTwoBoneData::TransitionOut(float a_duration, bool a_delete)
	{
		transition.mode = IKTwoBoneData::TransitionMode::kOut;
		transition.duration = a_duration;
		transition.currentTime = 0.0f;
		job.weight = 1.0f;

		if (a_delete) {
			flags.set(FLAG::kDeleteOnTransitionFinish);
		}
	}

	bool IKTwoBoneData::Validate()
	{
		for (auto& n : nodes) {
			if (n == nullptr) {
				return false;
			}
		}
		return true;
	}

	bool IKTwoBoneData::Update(float a_deltaTime, RE::NiAVObject* a_rootNode)
	{
		if (transition.mode != TransitionMode::kNone) {
			transition.currentTime += a_deltaTime;
			if (transition.currentTime >= transition.duration) {
				job.weight = transition.mode == TransitionMode::kIn ? 1.0f : 0.0f;
				transition.mode = TransitionMode::kNone;
				if (flags.any(FLAG::kDeleteOnTransitionFinish)) {
					flags.reset(FLAG::kDeleteOnTransitionFinish);
					flags.set(FLAG::kPendingDelete);
				}
			} else {
				float normalTime = transition.currentTime / transition.duration;
				job.weight = (transition.mode == TransitionMode::kIn ? normalTime : (1.0f - normalTime));
			}
		}

		//TODO: Add code for setting joint info & running job.
	}
}