#include <cmath>
#include "../include/Vector3.h"

Vector3::Vector3(float x, float y, float z)
	: x(x), y(y), z(z)
{
}

Vector3 Vector3::operator+(const Vector3& rhs) const
{
	return Vector3(x + rhs.x, y + rhs.y, z + rhs.z);
}

Vector3 Vector3::operator-(const Vector3& rhs) const
{
	return Vector3(x - rhs.x, y - rhs.y, z - rhs.z);
}

Vector3 Vector3::operator*(float scalar) const
{
	return Vector3(x * scalar, y * scalar, z * scalar);
}

float Vector3::Dot(const Vector3& rhs) const
{
	return x * rhs.x + y * rhs.y + z * rhs.z;
}

Vector3 Vector3::Cross(const Vector3& rhs) const
{
	return Vector3(
		y * rhs.z - z * rhs.y,
		z * rhs.x - x * rhs.z,
		x * rhs.y - y * rhs.x);
}

float Vector3::Length() const
{
	return std::sqrt(Dot(*this));
}

Vector3 Vector3::Normalized() const
{
	float length = Length();
	if (length < 1e-6f) return Vector3(0.f, 0.f, 0.f);
	return Vector3(x / length, y / length, z / length);
}
