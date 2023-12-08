#include "Transform.h"

namespace Animation
{
	void XYZTransform::ConstrainRotationAngles()
	{
		rotate.x = std::fmodf(rotate.x, 2.0f * PI_F);
		rotate.y = std::fmodf(rotate.y, 2.0f * PI_F);
		rotate.z = std::fmodf(rotate.z, 2.0f * PI_F);
	}

	Transform::Transform() {}

	Transform::Transform(const RE::NiQuaternion& rotate, const RE::NiPoint3& translate) :
		rotate(rotate), translate(translate) {}

	Transform::Transform(const RE::NiTransform& t)
	{
		FromReal(t);
	}

	void Transform::ToReal(RE::NiTransform& t) const
	{
		rotate.ToMatrix(t.rotate);
		t.translate = translate;
	}

	void Transform::FromReal(const RE::NiTransform& t)
	{
		rotate.FromMatrix(t.rotate);
		translate = t.translate;
	}

	bool Transform::IsIdentity() const
	{
		return (
			translate.x == 0.0f &&
			translate.y == 0.0f &&
			translate.z == 0.0f &&
			rotate.w == 1.0f &&
			rotate.x == 0.0f &&
			rotate.y == 0.0f &&
			rotate.z == 0.0f);
	}
}