#include "../include/Camera.h"


#include <cmath>

#include "../../../Level/Dungeon.h"
#include "../../Math/include/MathUtil.h"

namespace
{
	constexpr float kTransitionDuration = 0.5f; // moveRequest/rotateRequest 애니메이션 지속시간(초)
}

Camera::Camera()
	: Camera(Vector3(0.f, 0.f, 0.f), 70.f, 1920.f / 1080.f, 20.f, 2000.f){}

Camera::Camera(const Vector3& position, float fovDegrees, float aspectRatio, float nearZ, float farZ)
	: position(position), fov(MathUtil::DegToRad(fovDegrees)), aspectRatio(aspectRatio), nearZ(nearZ), farZ(farZ)
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
	fov = MathUtil::DegToRad(fovDegrees);
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

void Camera::Update(double deltaTime)
{
	if (cameraState == ECameraState::move)
	{
		elapsedTime += deltaTime;
		if (elapsedTime >= kTransitionDuration)
		{
			position = targetPosition;
			cameraState = ECameraState::idle;
			elapsedTime = 0.0f;
		}
		else position = MathUtil::Lerp(position, targetPosition, static_cast<float>(elapsedTime / kTransitionDuration));
	}
	else if (cameraState == ECameraState::rotate)
	{
		elapsedTime += deltaTime;
		if (elapsedTime >= kTransitionDuration)
		{
			theta = targetTheta;
			cameraState = ECameraState::idle;
			elapsedTime = 0.0f;
		}
		else theta = MathUtil::Lerp(theta, targetTheta, static_cast<float>(elapsedTime / kTransitionDuration));
		RefreshThetaCache();
		for (auto& plane : planes)
		{
			plane.RotateFromBaseWithRadian(-theta);
		}
	}
}

Matrix4x4 Camera::GetViewMatrix() const
{
	return Matrix4x4::RotationY(theta) * Matrix4x4::Translation(position * -1.f);
}

Matrix4x4 Camera::GetProjectionMatrix() const
{
	return Matrix4x4::PerspectiveFovLH(fov, aspectRatio, nearZ, farZ);
}

Matrix4x4 Camera::GetViewProjectionMatrix() const
{
	return GetProjectionMatrix() * GetViewMatrix();
}

bool Camera::isRenderPosition(const Vector3 targetPosition)
{
	Vector3 localPosition = targetPosition - position;

	Vector3 viewLocalPosition(cachedCosTheta * localPosition.x + cachedSinTheta * localPosition.z,
								localPosition.y,
								-cachedSinTheta * localPosition.x + cachedCosTheta * localPosition.z);

	//1차
	float offset = farZ * 0.2f;
	if (viewLocalPosition.z  >= farZ+offset || viewLocalPosition.z <= nearZ-offset) return false;
	if (viewLocalPosition.x  >= farZ+offset || viewLocalPosition.x <= -farZ-offset) return false;

	return true;
}

bool Camera::isRenderTile(const FloorTileInstance& targetTile)
{
	if (!isRenderPosition(targetTile.position)) return false;

	Vector3 localPosition = targetTile.position - position;
	std::vector<Vector3> vertexs;
	vertexs.push_back(Vector3(localPosition.x, localPosition.y, localPosition.z));
	vertexs.push_back(Vector3(localPosition.x, localPosition.y, localPosition.z+targetTile.kTileSize));
	vertexs.push_back(Vector3(localPosition.x+targetTile.kTileSize, localPosition.y, localPosition.z));
	vertexs.push_back(Vector3(localPosition.x+targetTile.kTileSize, localPosition.y, localPosition.z+targetTile.kTileSize));

	// 네 꼭짓점 중 하나라도 모든 평면 안쪽이면 보이는 것으로 취급한다(타일이 프러스텀 경계에 걸쳐
	// 있는데 꼭짓점이 전부 바깥인 경우는 놓치는 근사치지만, 기존 동작 그대로 유지).
	for (const auto& vertex : vertexs)
	{
		if (isRenderPoint(vertex + position)) return true;
	}

	return false;
}

bool Camera::isRenderPoint(const Vector3 targetPosition)
{
	Vector3 localPosition = targetPosition - position;
	for (const auto& plane : planes)
	{
		if (plane.CheckSide(localPosition) == EPlaneSide::Outside) return false;
	}
	return true;
}

void Camera::RotateCameraRadian(float radian)
{
	theta += radian;
	RefreshThetaCache();

	if (planes.empty()) return;
	for (auto& plane : planes)
	{
		plane.RotatePlane(-radian);
	}
}

void Camera::RotateCameraDegree(float degree)
{
	RotateCameraRadian(MathUtil::DegToRad(degree));
}

void Camera::moveRequest(EMoveDirection moveDir, float moveDistance)
{
	if (cameraState != ECameraState::idle) return;
	//todo : 전방벡터에 대한 연산 필요
	float c = std::cos(-theta);
	float s = std::sin(-theta);
	Vector3 forward = Vector3(s, 0, c);
	Vector3 right = Vector3(forward.z, 0, -forward.x);
	float distance = 400.0f;
	switch (moveDir)
	{
	case EMoveDirection::Forward:
		targetPosition = position + (forward * distance);
		break;
	case EMoveDirection::Backward:
		targetPosition = position - (forward * distance);
		break;
	case EMoveDirection::Left:
		targetPosition = position - (right * distance);
		break;
	case EMoveDirection::Right:
		targetPosition = position + (right * distance);
		break;
	}
	cameraState = ECameraState::move;
	elapsedTime = 0;
}

void Camera::rotateRequest(ERotateDirection rotateDir, float rotateDegree)
{
	if (cameraState != ECameraState::idle) return;

	float rad = MathUtil::DegToRad(rotateDegree);
	
	switch (rotateDir)
	{
	case ERotateDirection::Left:
		break;
	case ERotateDirection::Right:
		rad = -rad;
		break;
	}
	
	targetTheta += rad;
	cameraState = ECameraState::rotate;
	elapsedTime = 0;
}

void Camera::RefreshThetaCache()
{
	cachedCosTheta = std::cos(theta);
	cachedSinTheta = std::sin(theta);
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
	/*
	Plane rightPlane = Plane(Vector3(c,0,-s), 0);
	planes.push_back(rightPlane);
	Plane leftPlane = Plane(Vector3(-c,0,-s), 0);
	planes.push_back(leftPlane);
	*/
	
	float hFov = std::atan(std::tan(fov / 2) * aspectRatio);
	float sh = std::sin(hFov);
	float ch = std::cos(hFov);
	Plane rightPlane = Plane(Vector3(ch,0,-sh), 0);
	planes.push_back(rightPlane);
	Plane leftPlane = Plane(Vector3(-ch,0,-sh), 0);
	planes.push_back(leftPlane);
	
}
