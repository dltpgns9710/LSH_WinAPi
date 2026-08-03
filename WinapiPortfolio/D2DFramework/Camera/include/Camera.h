#pragma once

#include "../../Math/include/Vector3.h"
#include "../../Math/include/Matrix4x4.h"

class Camera
{
public:
	Camera();
	Camera(const Vector3& position, float fovDegrees, float aspectRatio, float nearZ, float farZ);

	void SetPosition(const Vector3& position);
	inline const Vector3& GetPosition() const { return position; }

	void SetAspectRatio(float aspectRatio);
	void SetFov(float fovDegrees);
	void SetNearFar(float nearZ, float farZ);

	void MoveX(float distance);
	void MoveZ(float distance);

	void Render();

	Matrix4x4 GetViewMatrix() const;
	Matrix4x4 GetProjectionMatrix() const;

private:
	Vector3 position = Vector3(0.f, 0.f, 0.f);

	float fov = 1.2f;
	float aspectRatio = 16.f / 9.f;
	float nearZ = 20.f;
	float farZ = 500.f;
};
