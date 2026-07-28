#include <cassert>
#include <d2d1_1.h>
#include "../include/SpriteAtlas.h"

SpriteAtlas::SpriteAtlas(std::shared_ptr<SpriteSheet> sourceSheet)
	: SpriteSheet(
		sourceSheet ? sourceSheet->GetGraphics() : nullptr,
		sourceSheet ? sourceSheet->GetBitmap() : nullptr)
{
	assert(sourceSheet && sourceSheet->IsValid() && "SpriteAtlas: sourceSheet is null or invalid");
	SetDrawRegion(0);
}

SpriteAtlas::~SpriteAtlas() {}

void SpriteAtlas::SetColumns(int newColumns)
{
	columns = newColumns > 0 ? newColumns : 1;
	SetDrawRegion(drawIndex);
}

void SpriteAtlas::SetRows(int newRows)
{
	rows = newRows > 0 ? newRows : 1;
	SetDrawRegion(drawIndex);
}

float SpriteAtlas::GetImageWidth()
{
	return SpriteSheet::GetImageWidth() / columns;
}

float SpriteAtlas::GetImageHeight()
{
	return SpriteSheet::GetImageHeight() / rows;
}

void SpriteAtlas::DrawSprite(float startX, float startY, float endX, float endY)
{
	DrawSpriteAtlas(startX, startY, endX, endY);
}

float SpriteAtlas::GetSheetWidth()
{
	return SpriteSheet::GetImageWidth();
}

float SpriteAtlas::GetSheetHeight()
{
	return SpriteSheet::GetImageHeight();
}

void SpriteAtlas::SetDrawRegion(int index)
{
	if (index < 0 || index >= columns * rows) return;

	drawIndex = index;

	int column = index % columns;
	int row = index / columns;

	float cellWidth = GetImageWidth();
	float cellHeight = GetImageHeight();

	SetDrawRegion(
		static_cast<float>(column) * cellWidth,
		static_cast<float>(row) * cellHeight,
		static_cast<float>(column + 1) * cellWidth,
		static_cast<float>(row + 1) * cellHeight);
}

void SpriteAtlas::SetDrawRegion(float startX, float startY, float endX, float endY)
{
	drawRegionStartX = startX;
	drawRegionStartY = startY;
	drawRegionEndX = endX;
	drawRegionEndY = endY;
}

void SpriteAtlas::DrawSpriteAtlas(float startX, float startY, float endX, float endY)
{
	DrawSpriteByRegion(
		startX, startY, endX, endY,
		drawRegionStartX, drawRegionStartY, drawRegionEndX, drawRegionEndY);
}
