#pragma once

#include "../include/SpriteAtlas.h"

class SpriteSheetAnimation : public SpriteAtlas
{
public:
	SpriteSheetAnimation(std::shared_ptr<SpriteSheet> sourceSheet);
	~SpriteSheetAnimation();

	inline void SetIntervalTime(double newIntervalTime) { intervalTime = newIntervalTime > 0.0 ? newIntervalTime : 0.001; }
	inline void SetLoop(bool newLoop) { loop = newLoop; }

	void AddElapsedTime(double deltaTime);
	void DrawSpriteAnimation(float startX, float startY, float endX, float endY);
	virtual void DrawSprite(float startX, float startY, float endX, float endY) override;
	bool IsDrawEnd();

private:
	double intervalTime = 1.0 / 60.0;
	double elapsedTime = 0.0;
	bool loop = false;
};
