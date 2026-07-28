#pragma once

#include <memory>
#include <string>
#include <wrl/client.h>
#include <wincodec.h>
#pragma comment(lib, "windowscodecs.lib")

class Graphics;
struct ID2D1Bitmap1;

using Microsoft::WRL::ComPtr;

class SpriteSheet
{
public:
	SpriteSheet(std::wstring filename, std::shared_ptr<Graphics> gfx);
	~SpriteSheet();

	bool IsValid() const { return bmp != nullptr; }

	virtual void DrawSprite(float startX, float startY, float endX, float endY);
	//void Draw();
	//void DrawVer2();

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
};
