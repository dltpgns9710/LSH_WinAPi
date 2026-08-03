#include <cmath>
#include "../include/Matrix4x4.h"

Matrix4x4 Matrix4x4::Identity()
{
	return Matrix4x4
	{ {
		{ 1.f, 0.f, 0.f, 0.f },
		{ 0.f, 1.f, 0.f, 0.f },
		{ 0.f, 0.f, 1.f, 0.f },
		{ 0.f, 0.f, 0.f, 1.f },
	} };
}

Matrix4x4 Matrix4x4::Translation(const Vector3& t)
{
	return Matrix4x4
	{ {
		{ 1.f, 0.f, 0.f, t.x },
		{ 0.f, 1.f, 0.f, t.y },
		{ 0.f, 0.f, 1.f, t.z },
		{ 0.f, 0.f, 0.f, 1.f },
	} };
}

Matrix4x4 Matrix4x4::RotationY(float radians)
{
	float s = std::sin(radians);
	float c = std::cos(radians);

	return Matrix4x4
	{ {
		{  c,  0.f, s,   0.f },
		{ 0.f, 1.f, 0.f, 0.f },
		{ -s,  0.f, c,   0.f },
		{ 0.f, 0.f, 0.f, 1.f },
	} };
}

Matrix4x4 Matrix4x4::PerspectiveFovLH(float fov, float aspectRatio, float nearZ, float farZ)
{
	float d = 1.f / std::tan(fov * 0.5f);
	float zScale = -(nearZ + farZ) / (nearZ - farZ);
	float zTranslate = (2.f * nearZ * farZ) / (nearZ - farZ);

	return Matrix4x4
	{ {
		{ d / aspectRatio, 0.f, 0.f,    0.f },
		{ 0.f,             d,   0.f,    0.f },
		{ 0.f,             0.f, zScale, zTranslate },
		{ 0.f,             0.f, 1.f,    0.f },
	} };
}

Matrix4x4 Matrix4x4::operator*(const Matrix4x4& rhs) const
{
	Matrix4x4 result{};
	for (int row = 0; row < 4; ++row)
	{
		for (int col = 0; col < 4; ++col)
		{
			float sum = 0.f;
			for (int k = 0; k < 4; ++k)
			{
				sum += m[row][k] * rhs.m[k][col];
			}
			result.m[row][col] = sum;
		}
	}
	return result;
}
