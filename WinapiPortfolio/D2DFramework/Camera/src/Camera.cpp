#include "../include/Camera.h"


#include <cmath>

namespace
{
	constexpr float kPi = 3.14159265358979323846f;

	constexpr float DegreesToRadians(float degrees)
	{
		return degrees * (kPi / 180.f);
	}
}

Camera::Camera()
	: Camera(Vector3(0.f, 0.f, 0.f), 70.f, 16.f / 9.f, 20.f, 2000.f){}

Camera::Camera(const Vector3& position, float fovDegrees, float aspectRatio, float nearZ, float farZ)
	: position(position), fov(DegreesToRadians(fovDegrees)), aspectRatio(aspectRatio), nearZ(nearZ), farZ(farZ)
{
	InitPlanes();
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

Matrix4x4 Camera::GetViewProjectionMatrix() const
{
	return GetProjectionMatrix() * GetViewMatrix();
}

Matrix4x4 Camera::GetViewProjectionMatrix(float b) const
{
	//return GetProjectionMatrix() * GetViewMatrix();
	return GetProjectionMatrix() * Matrix4x4::RotationY(b) * GetViewMatrix();
}

bool Camera::isRenderPosition(const Vector3 targetPosition)
{
	Vector3 localPosition = targetPosition - position;
	
	float offset = farZ * 0.2f;
	if (localPosition.z  >= farZ+offset || localPosition.z <= nearZ-offset) return false;
	if (localPosition.x  >= farZ+offset || localPosition.x <= nearZ-offset) return false;
	
	for (auto plane : planes)
	{
		if (plane.CheckSide(localPosition) == PlaneSide::Outside) return false;
	}
	return true;
}

void Camera::InitPlanes()
{
	float s = std::sin(fov/2);
	float c = std::cos(fov/2);
	
	Plane nearPlane = Plane(Vector3(0,0,-1), nearZ);
	planes.push_back(nearPlane);
	
	Plane farPlane = Plane(Vector3(0,0,1), -farZ);
	planes.push_back(farPlane);
	
	Plane upPlane = Plane(Vector3(0,c,-s), 0);
	planes.push_back(upPlane);
	Plane downPlane = Plane(Vector3(0,-c,-s), 0);
	planes.push_back(downPlane);
	/*Plane rightPlane = Plane(Vector3(c,0,-s), 0);
	planes.push_back(rightPlane);
	Plane leftPlane = Plane(Vector3(-c,0,-s), 0);
	planes.push_back(leftPlane);*/
	
	float hFov = std::atan(std::tan(fov / 2) * aspectRatio);
	float sh = std::sin(hFov);
	float ch = std::cos(hFov);
	Plane rightPlane = Plane(Vector3(ch,0,-sh), 0);
	planes.push_back(rightPlane);
	Plane leftPlane = Plane(Vector3(-ch,0,-sh), 0);
	planes.push_back(leftPlane);
}
