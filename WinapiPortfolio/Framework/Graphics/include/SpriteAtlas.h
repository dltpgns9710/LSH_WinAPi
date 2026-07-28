#pragma once

#include "../include/SpriteSheet.h"

class SpriteAtlas : public SpriteSheet
{
public:
	SpriteAtlas(std::shared_ptr<SpriteSheet> sourceSheet);
	~SpriteAtlas();

	void SetColumns(int newColumns);
	void SetRows(int newRows);

	virtual float GetImageWidth() override;
	virtual float GetImageHeight() override;
	virtual void DrawSprite(float startX, float startY, float endX, float endY) override;

	float GetSheetWidth();
	float GetSheetHeight();

	void SetDrawRegion(int index);
	void SetDrawRegion(float startX, float startY, float endX, float endY);
	void DrawSpriteAtlas(float startX, float startY, float endX, float endY);

protected:
	int columns = 1;
	int rows = 1;

private:
	int drawIndex = 0;
	float drawRegionStartX = 0.f;
	float drawRegionStartY = 0.f;
	float drawRegionEndX = 0.f;
	float drawRegionEndY = 0.f;
};
