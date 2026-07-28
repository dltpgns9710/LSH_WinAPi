#pragma once

#include <memory>
#include "Component.h"

class SpriteSheet;

enum class ERenderLayer
{
	layer1,
	layer2,
	layer3,
	layer4,
	layer5,
	max
};

class SpriteRenderComponent : public Component
{
public:
	explicit SpriteRenderComponent(std::shared_ptr<SpriteSheet> spriteSheet, ERenderLayer renderLayer = ERenderLayer::layer1);

	virtual void Update(double deltaTime) override {}
	virtual void Render() override;
	virtual void PostUpdate() override {}

	void SetSpriteSheet(std::shared_ptr<SpriteSheet> newSpriteSheet);
	inline std::shared_ptr<SpriteSheet> GetSpriteSheet() const { return spriteSheet; }

	inline ERenderLayer GetRenderLayer() const { return renderLayer; }

private:
	std::shared_ptr<SpriteSheet> spriteSheet;
	const ERenderLayer renderLayer;
};
