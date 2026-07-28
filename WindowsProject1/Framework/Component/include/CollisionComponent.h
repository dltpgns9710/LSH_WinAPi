#pragma once

#include <functional>
#include <memory>
#include <vector>
#include "Component.h"

class RenderObject;

enum class ECollisionType
{
	none,
	circle,
	aabb
};

enum class ECollisionChannel
{
	layer1 = 1 << 1,
	layer2 = 1 << 2,
	layer3 = 1 << 3,
	layer4 = 1 << 4,
	max = 1 << 5
};

class CollisionComponent : public Component, public std::enable_shared_from_this<CollisionComponent>
{
public:
	using CollisionCallback = std::function<void(std::shared_ptr<CollisionComponent>)>;

	CollisionComponent(ECollisionType type, ECollisionChannel channel);

	std::shared_ptr<RenderObject> GetOwner() const;

	inline void SetCollisionType(ECollisionType newCollisionType) { collisionType = newCollisionType; }
	inline void SetCollisionChannel(ECollisionChannel newCollisionChannel) { collisionChannel = static_cast<int>(newCollisionChannel); }
	inline void AddCollisionChannel(ECollisionChannel newCollisionChannel) { collisionChannel |= static_cast<int>(newCollisionChannel); }
	inline void RemoveCollisionChannel(ECollisionChannel newCollisionChannel) { collisionChannel &= ~static_cast<int>(newCollisionChannel); }

	inline ECollisionType GetCollisionType() const { return collisionType; }
	bool HaveSameCollisionChannel(const CollisionComponent& other) const;

	bool IsColliding(std::shared_ptr<CollisionComponent> other) const;
	void AddCollidingObject(std::shared_ptr<CollisionComponent> other);
	void RemoveCollidingObject(std::shared_ptr<CollisionComponent> other);

	inline void SetOnEnterCollision(CollisionCallback callback) { onEnterCollision = std::move(callback); }
	inline void SetOnStayCollision(CollisionCallback callback) { onStayCollision = std::move(callback); }
	inline void SetOnExitCollision(CollisionCallback callback) { onExitCollision = std::move(callback); }

	void EnterCollision(std::shared_ptr<CollisionComponent> other);
	void StayCollision(std::shared_ptr<CollisionComponent> other);
	void ExitCollision(std::shared_ptr<CollisionComponent> other);

	virtual void Update(double deltaTime) override {}
	virtual void Render() override {}
	virtual void PostUpdate() override;

private:
	ECollisionType collisionType;
	int collisionChannel = 0;
	std::vector<std::weak_ptr<CollisionComponent>> collidingObjects;
	CollisionCallback onEnterCollision;
	CollisionCallback onStayCollision;
	CollisionCallback onExitCollision;
};
