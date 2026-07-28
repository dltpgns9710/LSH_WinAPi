#include <algorithm>
#include "../include/CollisionCalc.h"
#include "../../Component/include/CollisionComponent.h"
#include "../../Core/include/RenderObject.h"

bool CollisionCalc::IsEnterCollision(std::shared_ptr<CollisionComponent> a, std::shared_ptr<CollisionComponent> b)
{
	if (!a || !b) return false;
	if (a->GetCollisionType() == ECollisionType::none || b->GetCollisionType() == ECollisionType::none) return false;
	if (!a->HaveSameCollisionChannel(*b)) return false;

	switch (a->GetCollisionType())
	{
	case ECollisionType::aabb:
		switch (b->GetCollisionType())
		{
		case ECollisionType::aabb:
			return CheckCollisionAABBvsAABB(a, b);
		case ECollisionType::circle:
			return CheckCollisionAABBvsCircle(a, b);
		default:
			return false;
		}
		break;
	case ECollisionType::circle:
		switch (b->GetCollisionType())
		{
		case ECollisionType::aabb:
			return CheckCollisionAABBvsCircle(b, a);
		case ECollisionType::circle:
			return CheckCollisionCirclevsCircle(a, b);
		default:
			return false;
		}
		break;
	default:
		return false;
	}

	return false;
}

bool CollisionCalc::CheckCollisionAABBvsAABB(std::shared_ptr<CollisionComponent> a, std::shared_ptr<CollisionComponent> b)
{
	std::shared_ptr<RenderObject> ownerA = a->GetOwner();
	std::shared_ptr<RenderObject> ownerB = b->GetOwner();

	float aMinX = ownerA->GetPositionX() - ownerA->GetSpriteWidthHalf();
	float aMaxX = ownerA->GetPositionX() + ownerA->GetSpriteWidthHalf();
	float aMinY = ownerA->GetPositionY() - ownerA->GetSpriteHeightHalf();
	float aMaxY = ownerA->GetPositionY() + ownerA->GetSpriteHeightHalf();

	float bMinX = ownerB->GetPositionX() - ownerB->GetSpriteWidthHalf();
	float bMaxX = ownerB->GetPositionX() + ownerB->GetSpriteWidthHalf();
	float bMinY = ownerB->GetPositionY() - ownerB->GetSpriteHeightHalf();
	float bMaxY = ownerB->GetPositionY() + ownerB->GetSpriteHeightHalf();

	return aMinX <= bMaxX && aMaxX >= bMinX && aMinY <= bMaxY && aMaxY >= bMinY;
}

bool CollisionCalc::CheckCollisionAABBvsCircle(std::shared_ptr<CollisionComponent> a, std::shared_ptr<CollisionComponent> b)
{
	std::shared_ptr<RenderObject> ownerA = a->GetOwner();
	std::shared_ptr<RenderObject> ownerB = b->GetOwner();

	float halfW = ownerA->GetSpriteWidthHalf();
	float halfH = ownerA->GetSpriteHeightHalf();

	float aabbMinX = ownerA->GetPositionX() - halfW;
	float aabbMaxX = ownerA->GetPositionX() + halfW;
	float aabbMinY = ownerA->GetPositionY() - halfH;
	float aabbMaxY = ownerA->GetPositionY() + halfH;

	float closestX = std::clamp(ownerB->GetPositionX(), aabbMinX, aabbMaxX);
	float closestY = std::clamp(ownerB->GetPositionY(), aabbMinY, aabbMaxY);

	float dx = ownerB->GetPositionX() - closestX;
	float dy = ownerB->GetPositionY() - closestY;
	float distSq = dx * dx + dy * dy;

	float radius = ownerB->GetSpriteHeightHalf() > ownerB->GetSpriteWidthHalf() ? ownerB->GetSpriteHeightHalf() : ownerB->GetSpriteWidthHalf();
	return distSq <= radius * radius;
}

bool CollisionCalc::CheckCollisionCirclevsCircle(std::shared_ptr<CollisionComponent> a, std::shared_ptr<CollisionComponent> b)
{
	std::shared_ptr<RenderObject> ownerA = a->GetOwner();
	std::shared_ptr<RenderObject> ownerB = b->GetOwner();

	float dx = ownerA->GetPositionX() - ownerB->GetPositionX();
	float dy = ownerA->GetPositionY() - ownerB->GetPositionY();
	float distSq = dx * dx + dy * dy;

	float aRadius = ownerA->GetSpriteHeightHalf() > ownerA->GetSpriteWidthHalf() ? ownerA->GetSpriteHeightHalf() : ownerA->GetSpriteWidthHalf();
	float bRadius = ownerB->GetSpriteHeightHalf() > ownerB->GetSpriteWidthHalf() ? ownerB->GetSpriteHeightHalf() : ownerB->GetSpriteWidthHalf();
	float radiusSum = aRadius + bRadius;
	return distSq <= radiusSum * radiusSum;

}
