#pragma once

#include <memory>

class CollisionComponent;

class CollisionCalc
{
public:
	static bool IsEnterCollision(std::shared_ptr<CollisionComponent> a, std::shared_ptr<CollisionComponent> b);
	static bool CheckCollisionAABBvsAABB(std::shared_ptr<CollisionComponent> a, std::shared_ptr<CollisionComponent> b);
	static bool CheckCollisionAABBvsCircle(std::shared_ptr<CollisionComponent> a, std::shared_ptr<CollisionComponent> b);
	static bool CheckCollisionCirclevsCircle(std::shared_ptr<CollisionComponent> a, std::shared_ptr<CollisionComponent> b);
};