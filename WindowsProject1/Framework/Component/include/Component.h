#pragma once

#include <memory>

class NonRenderObject;

class Component
{
public:
	virtual void Update(double deltaTime) = 0;
	virtual void Render() = 0;
	virtual void PostUpdate() = 0;

	inline std::shared_ptr<NonRenderObject> GetOwner() const { return owner.lock(); }

private:
	friend class NonRenderObject;
	void BindOwner(std::weak_ptr<NonRenderObject> newOwner);

	std::weak_ptr<NonRenderObject> owner;
};
