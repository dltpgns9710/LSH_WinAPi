#include "../include/SpriteSheetAnimation.h"
#include <cmath>

SpriteSheetAnimation::SpriteSheetAnimation(std::shared_ptr<SpriteSheet> sourceSheet)
	: SpriteAtlas(sourceSheet) {}

SpriteSheetAnimation::~SpriteSheetAnimation() {}

void SpriteSheetAnimation::AddElapsedTime(double deltaTime)
{
	elapsedTime += deltaTime;

	if (loop)
	{
		double totalDuration = intervalTime * (columns * rows);
		if (elapsedTime >= totalDuration) elapsedTime = std::fmod(elapsedTime, totalDuration);
	}
}

void SpriteSheetAnimation::DrawSpriteAnimation(float startX, float startY, float endX, float endY)
{
	if (IsDrawEnd()) return;

	int frameIndex = static_cast<int>(elapsedTime / intervalTime);
	SetDrawRegion(frameIndex);
	DrawSpriteAtlas(startX, startY, endX, endY);
}

void SpriteSheetAnimation::DrawSprite(float startX, float startY, float endX, float endY)
{
	DrawSpriteAnimation(startX, startY, endX, endY);
}

bool SpriteSheetAnimation::IsDrawEnd()
{
	if (loop) return false;

	int frameIndex = static_cast<int>(elapsedTime / intervalTime);
	return frameIndex >= columns * rows;
}
