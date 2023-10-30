#include "Interpolation.h"

namespace Animation
{
	void QuaternionLinearInterpolator::operator()(const RE::NiQuaternion& prev, RE::NiQuaternion next, float t, RE::NiQuaternion& outVal)
	{
		float dot = prev.Dot(next);
		if (dot < 0.0f) {
			next = -next;
			dot = -dot;
		}

		const float epsilon = 1e-6f;
		if (1.0f - dot > epsilon) {
			float theta = std::acosf(dot);
			float sinTheta = std::sinf(theta);
			float weight1 = std::sinf((1.0f - t) * theta) / sinTheta;
			float weight2 = std::sinf(t * theta) / sinTheta;

			outVal = (prev * weight1) + (next * weight2); 
		} else {
			float tInverse = 1.0f - t;
			outVal.w = tInverse * prev.w + t * next.w;
			outVal.x = tInverse * prev.x + t * next.x;
			outVal.y = tInverse * prev.y + t * next.y;
			outVal.z = tInverse * prev.z + t * next.z;
		}
	}

	void Point3LinearInterpolator::operator()(const RE::NiPoint3& prev, const RE::NiPoint3& next, float t, RE::NiPoint3& outVal) {
		outVal = {
			std::lerp(prev.x, next.x, t),
			std::lerp(prev.y, next.y, t),
			std::lerp(prev.z, next.z, t),
		};
	}
}