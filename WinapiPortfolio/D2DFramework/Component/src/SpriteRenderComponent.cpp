#include "../include/SpriteRenderComponent.h"
#include "../../Core/include/RenderObject.h"
#include "../../Graphics/include/SpriteSheet.h"

SpriteRenderComponent::SpriteRenderComponent(std::shared_ptr<SpriteSheet> spriteSheet, ERenderLayer renderLayer)
	: spriteSheet(std::move(spriteSheet)), renderLayer(renderLayer)
{
}

void SpriteRenderComponent::Render()
{
	std::shared_ptr<RenderObject> owner = std::static_pointer_cast<RenderObject>(GetOwner());
	if (!owner || !spriteSheet || !spriteSheet->IsValid()) return;

	float halfW = spriteSheet->GetImageWidth() / 2.f;
	float halfH = spriteSheet->GetImageHeight() / 2.f;
	float x = owner->GetPositionX();
	float y = owner->GetPositionY();

	spriteSheet->DrawSprite(x - halfW, y - halfH, x + halfW, y + halfH);
}

void SpriteRenderComponent::SetSpriteSheet(std::shared_ptr<SpriteSheet> newSpriteSheet)
{
	spriteSheet = std::move(newSpriteSheet);
}
