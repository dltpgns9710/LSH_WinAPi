#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include "../../Graphics/include/Graphics.h"
#include "../../Core/include/RenderObject.h"

class NonRenderObject;

class GameLevel
{
public:
	inline static void Init(std::shared_ptr<Graphics> gfx) { graphics = gfx; }

	virtual void Load() = 0;
	virtual void UnLoad() = 0;
	virtual void Render();
	virtual void Update(double deltaTime);
	virtual void PostUpdate();

protected:
	static std::shared_ptr<Graphics> graphics;
	std::vector<std::shared_ptr<RenderObject>> renderObjects;
	std::vector<std::shared_ptr<NonRenderObject>> nonRenderObjects;

	template<typename T>
	bool TryAddObject(std::shared_ptr<T> object);

private:
	void CollisionCheck();
	void DeleteGarbageObjects();
};

template<typename T>
bool GameLevel::TryAddObject(std::shared_ptr<T> object)
{
	if (std::shared_ptr<RenderObject> renderObject = std::dynamic_pointer_cast<RenderObject>(object))
	{
		auto insertPos = std::upper_bound(renderObjects.begin(), renderObjects.end(), renderObject,
			[](const std::shared_ptr<RenderObject>& a, const std::shared_ptr<RenderObject>& b) {
				return a->GetRenderLayer() < b->GetRenderLayer();
			});
		renderObjects.insert(insertPos, renderObject);
		return true;
	}
	if (std::shared_ptr<NonRenderObject> nonRenderObject = std::dynamic_pointer_cast<NonRenderObject>(object))
	{
		nonRenderObjects.push_back(nonRenderObject);
		return true;
	}
	return false;
}
