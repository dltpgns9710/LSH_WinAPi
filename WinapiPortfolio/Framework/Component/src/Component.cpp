#include <cassert>
#include "../include/Component.h"

void Component::BindOwner(std::weak_ptr<NonRenderObject> newOwner)
{
	assert(owner.expired() && "Component::BindOwner: owner is already bound and cannot be changed");
	if (!owner.expired()) return;
	owner = std::move(newOwner);
}
