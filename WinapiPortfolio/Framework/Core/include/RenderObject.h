#pragma once

#include <memory>
#include <type_traits>
#include "NonRenderObject.h"
#include "../../Component/include/SpriteRenderComponent.h"

class SpriteSheet;

class RenderObject : public NonRenderObject
{
	using Super = NonRenderObject;
public:
	template<typename T, typename... Args>
	static std::shared_ptr<T> Create(Args&&... args)
	{
		static_assert(std::is_base_of_v<RenderObject, T>, "RenderObject::Create: T must derive from RenderObject");

		std::shared_ptr<T> object(new T(std::forward<Args>(args)...));
		std::static_pointer_cast<RenderObject>(object)->OnCreated();
		return object;
	}

	virtual void Render();
	virtual void Update(double deltaTime) override;
	inline virtual void PostUpdate() override { Super::PostUpdate(); }
	inline virtual void DeleteObject() override { shouldDelete = true; }
	inline virtual bool ShouldDelete() override { return shouldDelete; }
	inline virtual void OnHit() {}

	void AddSpriteSheet(std::shared_ptr<SpriteSheet> newSpriteSheet, ERenderLayer renderLayer = ERenderLayer::layer1);
	void SetSpriteSheet(std::shared_ptr<SpriteSheet> newSpriteSheet);
	std::shared_ptr<SpriteSheet> GetSpriteSheet() const;

	ERenderLayer GetRenderLayer() const;

	inline float GetPositionX() const { return positionX; }
	inline float GetPositionY() const { return positionY; }
	virtual float GetSpriteWidth() const;
	virtual float GetSpriteHeight() const;
	virtual float GetSpriteWidthHalf() const;
	virtual float GetSpriteHeightHalf() const;

protected:
	RenderObject(float _positionX, float positionY);
	inline virtual void OnCreated() {}

	std::shared_ptr<SpriteRenderComponent> spriteRenderComponent;

	float positionX;
	float positionY;
};