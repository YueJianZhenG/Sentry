//
// Created by 64658 on 2025/9/11.
//

#include <cmath>
#include "Vector3.h"


namespace math
{
	Vector3::Vector3()
	{
		this->x = 0;
		this->y = 0;
		this->z = 0;
	}

	Vector3::Vector3(float x, float y)
	{
		this->x = x;
		this->y = y;
		this->z = 0;
	}

	Vector3::Vector3(float x, float y, float z)
	{
		this->x = x;
		this->y = y;
		this->z = z;
	}

	float Vector3::Distance(const math::Vector3& a, const math::Vector3& b)
	{
		float dx = b.x - a.x;
		float dy = b.y - a.y;
		float dz = b.z - a.z;
		return dx * dx + dy * dy + dz * dz;
	}

	bool Vector3::IsRange(const math::Vector3& vec1, const math::Vector3& vec2, float range)
	{
		float dx = vec1.x - vec2.x;
		float dy = vec1.y - vec2.y;
		float dz = vec1.z - vec2.z;
		float distanceSquared = dx * dx + dy * dy + dz * dz;

		float rangeSquared = range * range;
		return distanceSquared <= rangeSquared;
	}
}
