#include <cassert>
#include <Windows.h>
#include <cmath>
#include <d2d1effects.h>

#include "../include/SpriteSheet.h"
#include "../include/Graphics.h"

SpriteSheet::SpriteSheet(std::wstring filename, std::shared_ptr<Graphics> gfx)
	:graphics(gfx), bmp(nullptr)
{
	HRESULT hr;

	ComPtr<IWICImagingFactory> wicFactory = nullptr;
	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IWICImagingFactory,
		(LPVOID*)wicFactory.GetAddressOf());
	assert(!FAILED(hr) && "Fail to Create IWICImagingFactory");
	if (FAILED(hr)) return;

	ComPtr<IWICBitmapDecoder> wicDecoder = nullptr;
	hr = wicFactory->CreateDecoderFromFilename(
		filename.c_str(),
		NULL,
		GENERIC_READ,
		WICDecodeMetadataCacheOnLoad,
		wicDecoder.GetAddressOf());
	assert(!FAILED(hr) && "Fail to Create IWICBitmapDecoder");
	if (FAILED(hr)) return;

	ComPtr<IWICBitmapFrameDecode> wicFrame = nullptr;
	hr = wicDecoder->GetFrame(0, wicFrame.GetAddressOf());
	assert(!FAILED(hr) && "Fail to Create IWICBitmapFrameDecode");
	if (FAILED(hr)) return;

	ComPtr<IWICFormatConverter> wicConverter = nullptr;
	hr = wicFactory->CreateFormatConverter(wicConverter.GetAddressOf());
	assert(!FAILED(hr) && "Fail to Create IWICFormatConverter");
	if (FAILED(hr)) return;

	hr = wicConverter->Initialize(
		wicFrame.Get(),
		GUID_WICPixelFormat32bppPBGRA,
		WICBitmapDitherTypeNone,
		NULL,
		.0f,
		WICBitmapPaletteTypeCustom);
	assert(!FAILED(hr) && "Fail to Initialize IWICFormatConverter");
	if (FAILED(hr)) return;

	D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_NONE,
		D2D1::PixelFormat(
			DXGI_FORMAT_B8G8R8A8_UNORM,
			D2D1_ALPHA_MODE_PREMULTIPLIED)
	);

	hr = graphics->GetDeviceContext()->CreateBitmapFromWicBitmap(
		wicConverter.Get(),
		&props,
		bmp.GetAddressOf());
	assert(!FAILED(hr) && "Fail to Create ID2D1Bitmap1 from WicBitmap");
}

SpriteSheet::SpriteSheet(std::shared_ptr<Graphics> gfx, ComPtr<ID2D1Bitmap1> bitmap)
	: graphics(gfx), bmp(bitmap)
{
}

SpriteSheet::~SpriteSheet() {}

void SpriteSheet::DrawSprite(float startX, float startY, float endX, float endY)
{
	DrawSpriteByRegion(
		startX, startY, endX, endY,
		.0f, 0.f, bmp->GetSize().width, bmp->GetSize().height);
}

void SpriteSheet::DrawSpriteByRegion(
	float destStartX, float destStartY, float destEndX, float destEndY,
	float srcStartX, float srcStartY, float srcEndX, float srcEndY)
{
	graphics->GetDeviceContext()->DrawBitmap(
		bmp.Get(),
		D2D1::RectF(destStartX, destStartY, destEndX, destEndY),
		1.f,
		D2D1_BITMAP_INTERPOLATION_MODE::D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR,
		D2D1::RectF(srcStartX, srcStartY, srcEndX, srcEndY));
}

void SpriteSheet::DrawSpriteWarped(const Matrix4x4& localToScreen)
{
	// D2D1_MATRIX_4X4_F is interpreted with D2D's native row-vector convention (v' = v * M),
	// while our Matrix4x4 uses column-vector convention (v' = M * v) -- transpose on conversion.
	D2D1_MATRIX_4X4_F d2dMatrix{};
	for (int row = 0; row < 4; ++row)
	{
		for (int col = 0; col < 4; ++col)
		{
			d2dMatrix.m[row][col] = localToScreen.m[col][row];
		}
	}

	ComPtr<ID2D1Effect> transformEffect;
	graphics->GetDeviceContext()->CreateEffect(CLSID_D2D13DTransform, &transformEffect);
	transformEffect->SetInput(0, bmp.Get());
	transformEffect->SetValue(D2D1_3DTRANSFORM_PROP_INTERPOLATION_MODE, D2D1_3DTRANSFORM_INTERPOLATION_MODE_LINEAR);
	transformEffect->SetValue(D2D1_3DTRANSFORM_PROP_TRANSFORM_MATRIX, d2dMatrix);

	graphics->GetDeviceContext()->DrawImage(transformEffect.Get());
}

std::shared_ptr<SpriteSheet> SpriteSheet::CreateSubRegion(float startX, float startY, float endX, float endY) const
{
	UINT32 width = static_cast<UINT32>(endX - startX);
	UINT32 height = static_cast<UINT32>(endY - startY);

	D2D1_BITMAP_PROPERTIES1 props = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_NONE,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

	ComPtr<ID2D1Bitmap1> subBitmap;
	graphics->GetDeviceContext()->CreateBitmap(D2D1::SizeU(width, height), nullptr, 0, props, &subBitmap);

	D2D1_RECT_U srcRect = D2D1::RectU(
		static_cast<UINT32>(startX), static_cast<UINT32>(startY),
		static_cast<UINT32>(endX), static_cast<UINT32>(endY));
	subBitmap->CopyFromBitmap(nullptr, bmp.Get(), &srcRect);

	return std::shared_ptr<SpriteSheet>(new SpriteSheet(graphics, subBitmap));
}
/*
void SpriteSheet::DrawVer2()
{
	ComPtr<ID2D1Effect> m_perspectiveEffect;
	ComPtr<ID2D1DeviceContext> m_d2dContext;
	m_d2dContext = graphics->GetDeviceContext();

	HRESULT hr = m_d2dContext->CreateEffect(CLSID_D2D13DTransform, &m_perspectiveEffect);
	m_perspectiveEffect->SetInput(0, bmp.Get());
	m_perspectiveEffect->SetValue(D2D1_3DTRANSFORM_PROP_INTERPOLATION_MODE, D2D1_3DTRANSFORM_INTERPOLATION_MODE_LINEAR);

	D2D1_SIZE_F size = bmp->GetSize();
	float width = size.width;
	float height = size.height;
	double drgree = 65.0f;
	//double drgree = 65.0f;

	double radians = drgree * (std::acos(-1) / 180.0);
	double sinT = std::sin(radians);
	double cosT = std::cos(radians);
	double d = 400.0f;
	double halfH = height / 2;
	double halfW = width / 2;

	auto translationToOrigin = D2D1::Matrix4x4F::Translation(-halfW, -halfH, 0.0f);
	auto rotationX = D2D1::Matrix4x4F::RotationX(drgree);
	auto perspective = D2D1::Matrix4x4F::PerspectiveProjection(d);
	
	
	double radians = 65.0f * (std::acos(-1) / 180.0);
	
	double x = 512.0f * 400.f / (400.f - (512.f/2) * std::sin(radians));
	double y = (512.f/2 * std::cos(radians)) * 400.f / (400 + 512/2 * std::sin(radians));

	auto translationToScreen = D2D1::Matrix4x4F::Translation(x/2, y, 0.0f);
	

	// 위쪽 모서리 (원점기준, 회전 후)
	double zTop = halfH * sinT;          // = +232.015
	double topScale = d / (d + zTop);
	double topY = (-halfH * cosT) * topScale;   // 음수

	// 아래쪽 모서리
	double zBot = -halfH * sinT;         // = -232.015
	double botScale = d / (d + zBot);
	double botHalfW = halfW * botScale;  // 밑변 절반 길이

	// translation
	double tx = botHalfW;    // BottomLeft.x == 0 이 되게
	double ty = -topY;       // TopY == 0 이 되게

	auto translationToScreen = D2D1::Matrix4x4F::Translation(
		(float)tx, (float)ty, 0.0f
	);

	D2D1_MATRIX_4X4_F finalMatrix = translationToOrigin * rotationX * perspective * translationToScreen;

	m_perspectiveEffect->SetValue(D2D1_3DTRANSFORM_PROP_TRANSFORM_MATRIX, finalMatrix);
	m_d2dContext->DrawImage(m_perspectiveEffect.Get());
}*/

float SpriteSheet::GetImageWidth()
{
	return bmp->GetSize().width;
}

float SpriteSheet::GetImageHeight()
{
	return bmp->GetSize().height;
}

float SpriteSheet::GetClientWidthSize()
{
	return graphics->GetDeviceContext()->GetSize().width;
}

float SpriteSheet::GetClientHeightSize()
{
	return graphics->GetDeviceContext()->GetSize().height;
}

ComPtr<ID2D1Bitmap1> SpriteSheet::GetBitmap() const
{
	return bmp;
}
