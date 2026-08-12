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

	// 매 프레임/매 파츠마다 CreateEffect로 새 이펙트 인스턴스를 만들면 이펙트 그래프 생성/파괴가
	// 반복되는데, 이 과정에서 드로우 콜 사이에 GPU 파이프라인 상태가 얽혀 다른 파츠가 엉뚱한 색으로
	// 그려지는 문제가 있었다. 이펙트를 이 SpriteSheet 인스턴스에 캐싱해 재사용하면 이 문제가 사라진다.
	if (!warpEffect)
	{
		graphics->GetDeviceContext()->CreateEffect(CLSID_D2D13DTransform, &warpEffect);
		warpEffect->SetValue(D2D1_3DTRANSFORM_PROP_INTERPOLATION_MODE, D2D1_3DTRANSFORM_INTERPOLATION_MODE_LINEAR);
	}
	warpEffect->SetInput(0, bmp.Get());
	warpEffect->SetValue(D2D1_3DTRANSFORM_PROP_TRANSFORM_MATRIX, d2dMatrix);

	graphics->GetDeviceContext()->DrawImage(warpEffect.Get());
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

std::shared_ptr<SpriteSheet> SpriteSheet::CreateTiledRegion(float startX, float startY, float endX, float endY, int repeatX, int repeatY) const
{
	if (repeatX < 1) repeatX = 1;
	if (repeatY < 1) repeatY = 1;

	UINT32 tileWidth = static_cast<UINT32>(endX - startX);
	UINT32 tileHeight = static_cast<UINT32>(endY - startY);
	UINT32 destWidth = tileWidth * static_cast<UINT32>(repeatX);
	UINT32 destHeight = tileHeight * static_cast<UINT32>(repeatY);

	// 반복 채우기를 위해 SetTarget/BeginDraw/EndDraw로 타깃을 전환해야 하는데, 메인 프레임 렌더링에
	// 쓰는 공용 디바이스 컨텍스트(graphics->GetDeviceContext())를 그대로 쓰면 그 컨텍스트에 남아있는
	// 상태(바인딩된 타깃/이펙트 등)와 얽혀 다른 스프라이트 파츠의 비트맵이 깨지는 문제가 있었다.
	// (예: 이 함수가 SetTarget으로 타깃을 바꿨다가 되돌리는 사이, 같은 컨텍스트에서 만든 다른
	// CreateSubRegion 결과물이 이후 렌더링에서 엉뚱한 색으로 나타남.) 같은 ID2D1Device에서 만든
	// 별도의 디바이스 컨텍스트는 비트맵 등 디바이스 리소스를 공유하면서도 타깃/그리기 상태는
	// 독립적이므로, 이 함수 전용 컨텍스트를 하나 만들어 완전히 격리시킨다.
	ComPtr<ID2D1DeviceContext> bakeContext;
	graphics->GetD2DDevice()->CreateDeviceContext(D2D1_DEVICE_CONTEXT_OPTIONS_NONE, &bakeContext);

	// 1. atlasRect 한 조각만 우선 크롭한다 (CreateSubRegion과 동일한 방식).
	D2D1_BITMAP_PROPERTIES1 tileProps = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_NONE,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

	ComPtr<ID2D1Bitmap1> tileBitmap;
	bakeContext->CreateBitmap(D2D1::SizeU(tileWidth, tileHeight), nullptr, 0, tileProps, &tileBitmap);

	D2D1_RECT_U srcRect = D2D1::RectU(
		static_cast<UINT32>(startX), static_cast<UINT32>(startY),
		static_cast<UINT32>(endX), static_cast<UINT32>(endY));
	tileBitmap->CopyFromBitmap(nullptr, bmp.Get(), &srcRect);

	// 2. repeatX x repeatY 크기의 렌더타깃 비트맵을 만들고, WRAP 익스텐드 모드의 비트맵 브러시로
	// 한 번에 채운다 (반복 그리기를 GPU가 대신 해줌).
	D2D1_BITMAP_PROPERTIES1 destProps = D2D1::BitmapProperties1(
		D2D1_BITMAP_OPTIONS_TARGET,
		D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

	ComPtr<ID2D1Bitmap1> destBitmap;
	bakeContext->CreateBitmap(D2D1::SizeU(destWidth, destHeight), nullptr, 0, destProps, &destBitmap);

	ComPtr<ID2D1BitmapBrush1> tileBrush;
	D2D1_BITMAP_BRUSH_PROPERTIES1 brushProps = D2D1::BitmapBrushProperties1(
		D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_WRAP, D2D1_INTERPOLATION_MODE_NEAREST_NEIGHBOR);
	bakeContext->CreateBitmapBrush(tileBitmap.Get(), brushProps, D2D1::BrushProperties(), &tileBrush);

	bakeContext->SetTarget(destBitmap.Get());
	bakeContext->BeginDraw();
	bakeContext->FillRectangle(
		D2D1::RectF(0.f, 0.f, static_cast<float>(destWidth), static_cast<float>(destHeight)), tileBrush.Get());
	bakeContext->EndDraw();

	// 서로 다른 ID2D1DeviceContext가 같은 ID3D11 디바이스(즉시 컨텍스트)를 공유할 때는, 한 컨텍스트에서
	// 그린 커맨드가 실제로 GPU에 제출되기 전에 다른 컨텍스트로 그리기를 넘기면 안 된다(MSDN: "call Flush
	// on a device context before you use a different device context to draw to the same device"). 이걸
	// 안 하면 메인 프레임 렌더링(다음 프레임의 graphics->BeginDraw)이 이 임시 컨텍스트가 마지막으로
	// 바인딩한 타깃에 잘못 그려져, 이 함수가 만든 결과물과 무관한 다른 파츠가 깨져 보일 수 있다.
	bakeContext->Flush();

	return std::shared_ptr<SpriteSheet>(new SpriteSheet(graphics, destBitmap));
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
