//
// Created by 64658 on 2025/9/11.
//

#ifndef APP_VECTOR3_H
#define APP_VECTOR3_H

namespace math
{
	class Vector3
	{
	public:
		Vector3();

		Vector3(float x, float y);

		Vector3(float x, float y, float z);

	public:
		Vector3 operator+(const Vector3& other) const
		{
			return { x + other.x, y + other.y, z + other.z };
		}

		inline Vector3 operator-(const Vector3& other) const
		{
			return { x - other.x, y - other.y, z - other.z };
		}

		inline Vector3 operator*(float scalar) const
		{
			return { x * scalar, y * scalar, z * scalar };
		}

		inline Vector3 operator/(float scalar) const
		{
			if (scalar != 0)
			{
				return { x / scalar, y / scalar, z / scalar };
			}
			return { 0, 0, 0 }; // 避免除零
		}

		inline Vector3& operator+=(const Vector3& other)
		{
			this->x += other.x;
			this->y += other.y;
			this->z += other.z;
			return *this;
		}

		inline Vector3& operator-=(const Vector3& other)
		{
			this->x -= other.x;
			this->y -= other.y;
			this->z -= other.z;
			return *this;
		}

		inline float SqrMagnitude() const { return x * x + y * y + z * z; }
		inline float Magnitude() const { return std::sqrt(this->SqrMagnitude()); }
	public:
		static Vector3 Back() { return Vector3{ 0.0f, 00.f, -1.0f }; }
		static Vector3 Left() { return Vector3{ -1.0f, 00.f, 0.0f }; }
		static Vector3 Right() { return Vector3{ 1.0f, 00.f, 0.0f }; }
		static Vector3 Forward() { return Vector3{ 0.0f, 00.f, 1.0f }; }
		static float Distance(const Vector3& vec1, const Vector3& vec2);
		static bool IsRange(const Vector3& vec1, const Vector3& vec2, float range);
	public:
		float x;
		float y;
		float z;
	};
}


#endif //APP_VECTOR3_H
