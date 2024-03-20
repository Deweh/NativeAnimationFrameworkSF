#pragma once

namespace Animation
{
	struct XYZTransform
	{
		RE::NiPoint3 rotate;
		RE::NiPoint3 translate;

		void ConstrainRotationAngles();
	};

	struct Transform
	{
		RE::NiQuaternion rotate;
		RE::NiPoint3 translate;

		Transform();
		Transform(const RE::NiQuaternion& rotate, const RE::NiPoint3& translate);
		Transform(const RE::NiTransform& t);
		Transform(const ozz::math::Transform& t);

		static void ExtractSoaTransform(const ozz::math::SoaTransform& in, std::array<Transform, 4>& out);
		static void StoreSoaTransform(const std::span<Transform, 4>& in, ozz::math::SoaTransform& out);
		static void ExtractSoaTransforms(const std::vector<ozz::math::SoaTransform>& in, const std::function<void(size_t, const Transform&)> func);
		static void StoreSoaTransforms(std::vector<ozz::math::SoaTransform>& out, const std::function<Transform(size_t)> func);
		static void StoreSoaTransforms(std::span<ozz::math::SoaTransform>& out, const std::function<Transform(size_t)> func);

		static void ExtractSoaMatrixPoint(const ozz::math::SoaFloat4x4& mat_in, const ozz::math::SoaFloat3& pt_in, std::array<RE::NiMatrix3, 4>& mat_out, std::array<RE::NiPoint3, 4>& pt_out);
		static void ExtractSoaTransformsReal(const std::vector<ozz::math::SoaTransform>& in, const std::function<void(size_t, const RE::NiMatrix3&, const RE::NiPoint3&)> func);

		void FromOzz(const ozz::math::Transform& t);
		void ToReal(RE::NiTransform& t) const;
		void FromReal(const RE::NiTransform& t);
		bool IsIdentity() const;
		void MakeIdentity();
		Transform operator-(const Transform& rhs) const;
		Transform operator*(const Transform& rhs) const;
	};
}