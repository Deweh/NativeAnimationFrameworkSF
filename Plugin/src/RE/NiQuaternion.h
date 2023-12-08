#pragma once

namespace RE
{
	class NiQuaternion
	{
	public:
		float w{ 1.0f };
		float x{ 0.0f };
		float y{ 0.0f };
		float z{ 0.0f };

		NiQuaternion() {}

		NiQuaternion(float w, float x, float y, float z) : w(w), x(x), y(y), z(z) {}

		NiQuaternion(const NiMatrix3& mat)
		{
			FromMatrix(mat);
		}

		static NiQuaternion EulerZXY(const NiPoint3& euler)
		{
			NiQuaternion result;
			result.FromEulerAnglesZXY(euler);
			return result;
		}

		static NiQuaternion AngleAxis(float angle, const NiPoint3& axis)
		{
			NiQuaternion result;
			result.FromAngleAxis(angle, axis);
			return result;
		}

		NiPoint3 ToEulerAnglesZXY() const
		{
			NiPoint3 euler;
			euler.x = std::atan2(2.0f * (w * x + y * z), 1.0f - 2.0f * (x * x + y * y));
			euler.y = std::asin(2.0f * (w * y - z * x));
			euler.z = std::atan2(2.0f * (w * z + x * y), 1.0f - 2.0f * (y * y + z * z));

			return euler;
		}

		void FromEulerAnglesZXY(const NiPoint3& euler)
		{
			float cy = std::cos(euler.z * 0.5f);
			float sy = std::sin(euler.z * 0.5f);
			float cp = std::cos(euler.y * 0.5f);
			float sp = std::sin(euler.y * 0.5f);
			float cr = std::cos(euler.x * 0.5f);
			float sr = std::sin(euler.x * 0.5f);

			w = cr * cp * cy + sr * sp * sy;
			x = sr * cp * cy - cr * sp * sy;
			y = cr * sp * cy + sr * cp * sy;
			z = cr * cp * sy - sr * sp * cy;
		}

		void FromAngleAxis(float angle, const NiPoint3& axis)
		{
			float halfAngle = angle / 2.0f;
			w = std::cos(halfAngle);
			x = std::sin(halfAngle) * axis.x;
			y = std::sin(halfAngle) * axis.y;
			z = std::sin(halfAngle) * axis.z;
		}

		void ToMatrix(NiMatrix3& a_mat) const
		{
			using func_t = decltype(&NiQuaternion::ToMatrix);
			REL::Relocation<func_t> func{ REL::ID(77539) };
			return func(this, a_mat);
		}

		void FromMatrix(const NiMatrix3& a_mat)
		{
			using func_t = decltype(&NiQuaternion::FromMatrix);
			REL::Relocation<func_t> func{ REL::ID(210482) };
			func(this, a_mat);
			*this = SlowNormalize();
		}

		float Dot() const
		{
			return w * w + x * x + y * y + z * z;
		}

		float Dot(const NiQuaternion& q) const
		{
			return w * q.w + x * q.x + y * q.y + z * q.z;
		}

		float Magnitude() const
		{
			return std::sqrt(Dot());
		}

		NiQuaternion Normalize() const
		{
			__m128 temp = _mm_set_ss(Dot());
			temp = _mm_rsqrt_ss(temp);
			float rsqrt = _mm_cvtss_f32(temp);
			return { w * rsqrt, x * rsqrt, y * rsqrt, z * rsqrt };
		}

		NiQuaternion SlowNormalize() const
		{
			float rsqrt = 1.0f / Magnitude();
			return { w * rsqrt, x * rsqrt, y * rsqrt, z * rsqrt };
		}

		NiQuaternion InvertVector() const
		{
			return { w, -x, -y, -z };
		}

		NiQuaternion InvertScalar() const
		{
			return { -w, x, y, z };
		}

		float& operator[](size_t i)
		{
			assert(i < 4);
			return (&w)[i];
		}

		const float& operator[](size_t i) const
		{
			assert(i < 4);
			return (&w)[i];
		}

		NiQuaternion operator*(const NiQuaternion& a_rhs) const
		{
			return {
				a_rhs.w * w - a_rhs.x * x - a_rhs.y * y - a_rhs.z * z,
				a_rhs.w * x + a_rhs.x * w - a_rhs.y * z + a_rhs.z * y,
				a_rhs.w * y + a_rhs.x * z + a_rhs.y * w - a_rhs.z * x,
				a_rhs.w * z - a_rhs.x * y + a_rhs.y * x + a_rhs.z * w
			};
		}

		void operator*=(const NiQuaternion& a_rhs)
		{
			(*this) = operator*(a_rhs);
		}

		NiQuaternion operator*(float s) const
		{
			return { w * s, x * s, y * s, z * s };
		}

		NiQuaternion operator+(const NiQuaternion& a_rhs) const
		{
			return { w + a_rhs.w, x + a_rhs.x, y + a_rhs.y, z + a_rhs.z };
		}

		NiQuaternion operator-(const NiQuaternion& a_rhs) const
		{
			return { w - a_rhs.w, x - a_rhs.x, y - a_rhs.y, z - a_rhs.z };
		}

		NiQuaternion operator/(float s) const
		{
			return { w / s, x / s, y / s, z / s };
		}

		NiQuaternion operator-() const
		{
			return { -w, -x, -y, -z };
		}
	};
	static_assert(sizeof(NiQuaternion) == 0x10);
}