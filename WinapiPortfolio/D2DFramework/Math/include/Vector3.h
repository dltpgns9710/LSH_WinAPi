#pragma once

struct Vector3
{
	float x = 0.f;
	float y = 0.f;
	float z = 0.f;

	Vector3() = default;
	Vector3(float x, float y, float z);

	Vector3 operator+(const Vector3& rhs) const;
	Vector3 operator-(const Vector3& rhs) const;
	Vector3 operator*(float scalar) const;

	float Dot(const Vector3& rhs) const;
	Vector3 Cross(const Vector3& rhs) const;
	float Length() const;
	Vector3 Normalized() const;
};
