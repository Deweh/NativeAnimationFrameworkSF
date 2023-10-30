#pragma once

namespace Animation
{
	struct Transform
	{
		RE::NiQuaternion rotate;
		RE::NiPoint3 translate;

		Transform();
		Transform(const RE::NiQuaternion& rotate, const RE::NiPoint3& translate);
		Transform(const RE::NiTransform& t);

		void ToReal(RE::NiTransform& t) const;
		void FromReal(const RE::NiTransform& t);
		bool IsIdentity() const;
	};
}