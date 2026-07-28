#include <cassert>
#include "../include/RenderObject.h"
#include "../../Graphics/include/SpriteSheet.h"

RenderObject::RenderObject(float _positionX, float positionY)
	:positionX(_positionX), positionY(positionY){}

void RenderObject::Render()
{
	RenderComponents();
}

void RenderObject::Update(double deltaTime)
{
	Super::Update(deltaTime);
}

void RenderObject::AddSpriteSheet(std::shared_ptr<SpriteSheet> newSpriteSheet, ERenderLayer renderLayer)
{
	assert(newSpriteSheet && newSpriteSheet->IsValid() && "RenderObject::AddSpriteSheet: invalid sprite sheet");
	if (!newSpriteSheet || !newSpriteSheet->IsValid()) return;

	assert(!spriteRenderComponent && "RenderObject::AddSpriteSheet: sprite render component already exists, use SetSpriteSheet instead");
	if (spriteRenderComponent) return;

	spriteRenderComponent = AddComponent<SpriteRenderComponent>(newSpriteSheet, renderLayer);
}

void RenderObject::SetSpriteSheet(std::shared_ptr<SpriteSheet> newSpriteSheet)
{
	assert(newSpriteSheet && newSpriteSheet->IsValid() && "RenderObject::SetSpriteSheet: invalid sprite sheet");
	if (!newSpriteSheet || !newSpriteSheet->IsValid()) return;

	assert(spriteRenderComponent && "RenderObject::SetSpriteSheet: no sprite render component yet, use AddSpriteSheet instead");
	if (!spriteRenderComponent) return;

	spriteRenderComponent->SetSpriteSheet(newSpriteSheet);
}

std::shared_ptr<SpriteSheet> RenderObject::GetSpriteSheet() const
{
	return spriteRenderComponent ? spriteRenderComponent->GetSpriteSheet() : nullptr;
}

ERenderLayer RenderObject::GetRenderLayer() const
{
	return spriteRenderComponent ? spriteRenderComponent->GetRenderLayer() : ERenderLayer::layer1;
}

float RenderObject::GetSpriteWidth() const
{
	std::shared_ptr<SpriteSheet> spriteSheet = GetSpriteSheet();
	return spriteSheet ? spriteSheet->GetImageWidth() : 0.f;
}

float RenderObject::GetSpriteHeight() const
{
	std::shared_ptr<SpriteSheet> spriteSheet = GetSpriteSheet();
	return spriteSheet ? spriteSheet->GetImageHeight() : 0.f;
}

float RenderObject::GetSpriteWidthHalf() const
{
	return GetSpriteWidth() / 2;
}

float RenderObject::GetSpriteHeightHalf() const
{
	return GetSpriteHeight() / 2;
}
