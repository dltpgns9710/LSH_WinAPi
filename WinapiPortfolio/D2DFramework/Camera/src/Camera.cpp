#include "../include/Camera.h"

namespace
{
	constexpr float kPi = 3.14159265358979323846f;

	constexpr float DegreesToRadians(float degrees)
	{
		return degrees * (kPi / 180.f);
	}
}

Camera::Camera() = default;

Camera::Camera(const Vector3& position, float fovDegrees, float aspectRatio, float nearZ, float farZ)
	: position(position), fov(DegreesToRadians(fovDegrees)), aspectRatio(aspectRatio), nearZ(nearZ), farZ(farZ)
{
}

void Camera::SetPosition(const Vector3& newPosition)
{
	position = newPosition;
}

void Camera::SetAspectRatio(float newAspectRatio)
{
	aspectRatio = newAspectRatio;
}

void Camera::SetFov(float fovDegrees)
{
	fov = DegreesToRadians(fovDegrees);
}

void Camera::SetNearFar(float newNearZ, float newFarZ)
{
	nearZ = newNearZ;
	farZ = newFarZ;
}

void Camera::MoveX(float distance)
{
	position.x += distance;
}

void Camera::MoveZ(float distance)
{
	position.z += distance;
}

void Camera::Render()
{
}

Matrix4x4 Camera::GetViewMatrix() const
{
	return Matrix4x4::Translation(position * -1.f);
}

Matrix4x4 Camera::GetProjectionMatrix() const
{
	return Matrix4x4::PerspectiveFovLH(fov, aspectRatio, nearZ, farZ);
}
