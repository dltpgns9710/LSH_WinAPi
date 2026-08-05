#pragma once

#include <vector>

#include "../../Math/include/Vector3.h"
#include "../../Math/include/Matrix4x4.h"
#include "../../Math/include/Plane.h"

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
	Matrix4x4 GetViewProjectionMatrix() const;
	Matrix4x4 GetViewProjectionMatrix(float b) const;

	bool isRenderPosition(const Vector3 targetPosition);
	bool isRenderTile(const struct FloorTileInstance& targetTile);
private:
	void InitPlanes();
	
	std::vector<Plane> planes;
	Vector3 position;

	float fov;
	float aspectRatio;
	float nearZ;
	float farZ;
};
