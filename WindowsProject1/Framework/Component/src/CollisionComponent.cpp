#include "../include/CollisionComponent.h"
#include "../../Core/include/RenderObject.h"

CollisionComponent::CollisionComponent(ECollisionType type, ECollisionChannel channel)
	: collisionType(type)
{
	AddCollisionChannel(channel);
}

std::shared_ptr<RenderObject> CollisionComponent::GetOwner() const
{
	return std::static_pointer_cast<RenderObject>(Component::GetOwner());
}

bool CollisionComponent::HaveSameCollisionChannel(const CollisionComponent& other) const
{
	return collisionChannel & other.collisionChannel;
}

bool CollisionComponent::IsColliding(std::shared_ptr<CollisionComponent> other) const
{
	for (const std::weak_ptr<CollisionComponent>& iter : collidingObjects)
	{
		if (std::shared_ptr<CollisionComponent> lockIter = iter.lock())
		{
			if (lockIter == other) return true;
		}
	}
	return false;
}

void CollisionComponent::AddCollidingObject(std::shared_ptr<CollisionComponent> other)
{
	collidingObjects.push_back(other);
}

void CollisionComponent::RemoveCollidingObject(std::shared_ptr<CollisionComponent> other)
{
	std::erase_if(collidingObjects, [&other](const std::weak_ptr<CollisionComponent>& object) {
		std::shared_ptr<CollisionComponent> lockObject = object.lock();
		return !lockObject || lockObject == other;
		});
}

void CollisionComponent::EnterCollision(std::shared_ptr<CollisionComponent> other)
{
	if (onEnterCollision) onEnterCollision(other);
}

void CollisionComponent::StayCollision(std::shared_ptr<CollisionComponent> other)
{
	if (onStayCollision) onStayCollision(other);
}

void CollisionComponent::ExitCollision(std::shared_ptr<CollisionComponent> other)
{
	if (onExitCollision) onExitCollision(other);
}

void CollisionComponent::PostUpdate()
{
	std::erase_if(collidingObjects, [](const std::weak_ptr<CollisionComponent>& object) {
		return object.expired();
		});
}
