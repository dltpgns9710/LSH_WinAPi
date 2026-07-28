#pragma once

#include <memory>
#include <type_traits>
#include <vector>
#include "../../Component/include/Component.h"

class NonRenderObject : public std::enable_shared_from_this<NonRenderObject>
{
public:
	virtual void Update(double deltaTime) {UpdateComponents(deltaTime);}
	virtual void PostUpdate() {PostUpdateComponents();}

	inline virtual void DeleteObject() { shouldDelete = true; }
	inline virtual bool ShouldDelete() { return shouldDelete; }

	template<typename T, typename... Args>
	std::shared_ptr<T> AddComponent(Args&&... args)
	{
		static_assert(std::is_base_of_v<Component, T>, "NonRenderObject::AddComponent: T must derive from Component");

		std::shared_ptr<T> component = std::make_shared<T>(std::forward<Args>(args)...);
		component->BindOwner(weak_from_this());
		components.push_back(component);
		return component;
	}

	template<typename T>
	std::shared_ptr<T> GetComponent() const
	{
		static_assert(std::is_base_of_v<Component, T>, "NonRenderObject::GetComponent: T must derive from Component");

		for (const std::shared_ptr<Component>& component : components)
		{
			if (std::shared_ptr<T> casted = std::dynamic_pointer_cast<T>(component)) return casted;
		}
		return nullptr;
	}

	template<typename T>
	void RemoveComponent()
	{
		static_assert(std::is_base_of_v<Component, T>, "NonRenderObject::RemoveComponent: T must derive from Component");

		std::erase_if(components, [](const std::shared_ptr<Component>& component) {
			return std::dynamic_pointer_cast<T>(component) != nullptr;
			});
	}

protected:
	bool shouldDelete = false;

	void UpdateComponents(double deltaTime) { for (const std::shared_ptr<Component>& component : components) component->Update(deltaTime); }
	void RenderComponents() { for (const std::shared_ptr<Component>& component : components) component->Render(); }
	void PostUpdateComponents() { for (const std::shared_ptr<Component>& component : components) component->PostUpdate(); }

private:
	std::vector<std::shared_ptr<Component>> components;
};
