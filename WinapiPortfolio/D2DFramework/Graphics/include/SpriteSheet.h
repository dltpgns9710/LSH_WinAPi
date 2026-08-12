#pragma once

#include <memory>
#include <string>
#include <wrl/client.h>
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

#include "../../Math/include/Matrix4x4.h"

class Graphics;
struct ID2D1Bitmap1;
struct ID2D1Effect;

using Microsoft::WRL::ComPtr;

class SpriteSheet
{
public:
	SpriteSheet(std::wstring filename, std::shared_ptr<Graphics> gfx);
	~SpriteSheet();

	bool IsValid() const { return bmp != nullptr; }

	virtual void DrawSprite(float startX, float startY, float endX, float endY);
	void DrawSpriteWarped(const Matrix4x4& localToScreen);
	std::shared_ptr<SpriteSheet> CreateSubRegion(float startX, float startY, float endX, float endY) const;

	// atlasRect(startX..endY) 한 조각을 repeatX x repeatY번 반복해서 채운 새 비트맵을 만든다.
	// tiled:true인 텍스처를 CreateSubRegion처럼 한 조각만 늘려 그리지 않고 실제로 반복시키기 위한 용도.
	std::shared_ptr<SpriteSheet> CreateTiledRegion(float startX, float startY, float endX, float endY, int repeatX, int repeatY) const;

	virtual float GetImageWidth();
	virtual float GetImageHeight();
	float GetClientWidthSize();
	float GetClientHeightSize();

	inline std::shared_ptr<Graphics> GetGraphics() const { return graphics; }
	ComPtr<ID2D1Bitmap1> GetBitmap() const;

protected:
	SpriteSheet(std::shared_ptr<Graphics> gfx, ComPtr<ID2D1Bitmap1> bitmap);

	void DrawSpriteByRegion(
		float destStartX, float destStartY, float destEndX, float destEndY,
		float srcStartX, float srcStartY, float srcEndX, float srcEndY);

	std::shared_ptr<Graphics> graphics;
	ComPtr<ID2D1Bitmap1> bmp;
	ComPtr<ID2D1Effect> warpEffect; // DrawSpriteWarped 전용, 매 호출 재사용
};
