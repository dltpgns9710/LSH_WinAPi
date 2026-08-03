#pragma once

#include "Vector3.h"
#include "Vector4.h"

// Column-vector convention: v' = M * v.
// To apply A first, then B, compose as (B * A).
struct Matrix4x4
{
	float m[4][4];

	static Matrix4x4 Identity();
	static Matrix4x4 Translation(const Vector3& t);
	static Matrix4x4 RotationY(float radians);
	static Matrix4x4 PerspectiveFovLH(float fov, float aspectRatio, float nearZ, float farZ);

	Matrix4x4 operator*(const Matrix4x4& rhs) const;
};
