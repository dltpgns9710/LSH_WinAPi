#pragma once

#include <vector>

#include "../../Math/include/Vector3.h"
#include "../../Math/include/Matrix4x4.h"
#include "../../Math/include/Plane.h"

enum class ECameraState
{
	idle,
	move,
	rotate
};

enum class EMoveDirection
{
	Forward,
	Backward,
	Left,
	Right
};

enum class ERotateDirection
{
	Left,
	Right
};

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
	void Update(double deltaTime);
	
	Matrix4x4 GetViewMatrix() const;
	Matrix4x4 GetProjectionMatrix() const;
	Matrix4x4 GetViewProjectionMatrix() const;

	bool isRenderPosition(const Vector3 targetPosition);
	bool isRenderTile(const struct FloorTileInstance& targetTile);
	bool isRenderPoint(const Vector3 targetPosition); // 프러스텀 평면(planes) 전부의 안쪽인지 점 하나만 검사

	// moveRequest/rotateRequest가 idle이 아니면 요청을 조용히 무시하므로, 호출 전에 확인해 씹히지 않게 한다.
	inline bool IsIdle() const { return cameraState == ECameraState::idle; }

	void RotateCameraRadian(float radian);
	void RotateCameraDegree(float degree);
	
	void moveRequest(EMoveDirection moveDir, float moveDistance = 400.0f); // todo : 400하드코딩 수정필요, tile 사이즈
	void rotateRequest(ERotateDirection rotateDir, float rotateDegree = 90.0f);
private:
	void InitPlanes();
	
	ECameraState cameraState = ECameraState::idle;
	
	std::vector<Plane> planes;
	Vector3 position;
	Vector3 targetPosition;
	
	float fov;
	float aspectRatio;
	float nearZ;
	float farZ;
	
	float theta = 0; //radian
	float targetTheta = 0; //radian
	
	float elapsedTime = 0;
};
