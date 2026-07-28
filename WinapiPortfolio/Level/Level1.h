#pragma once

#include "../Framework/Scene/include/GameLevel.h"

class Level1 : public GameLevel
{
	float y;
	float ySpeed;
	std::shared_ptr<SpriteSheet> sprites;

public:
	virtual void Load() override;
	virtual void UnLoad() override;
	virtual void Render() override;
	virtual void Update(double deltaTime) override;
};

