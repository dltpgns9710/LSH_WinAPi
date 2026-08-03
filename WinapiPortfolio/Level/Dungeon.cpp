#include "Dungeon.h"

#include "../D2DFramework/Graphics/include/SpriteAtlas.h"
#include "../D2DFramework/Manager/include/SpriteSheetManager.h"

void Dungeon::Load()
{
    SpriteSheetManager::GetInstance().LoadTexture(L"Background", L"Background.png");
    background = make_shared<SpriteAtlas>(SpriteSheetManager::GetInstance().GetTexture(L"Background"));
    background->SetDrawRegion(0,250,background->GetClientWidthSize(),background->GetClientHeightSize());
}

void Dungeon::UnLoad()
{
}

void Dungeon::Render()
{
    background->DrawSpriteAtlas(0,0,background->GetClientWidthSize(),background->GetClientHeightSize());
    super::Render();
    
    
    /*
    D2D1_BITMAP_PROPERTIES1 targetProps = D2D1::BitmapProperties1(
        D2D1_BITMAP_OPTIONS_TARGET,
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    ComPtr<ID2D1Bitmap1> target;
    context->CreateBitmap(D2D1::SizeU(screenW, screenH), nullptr, 0, targetProps, &target);

    ComPtr<ID2D1Image> prevTarget;
    context->GetTarget(&prevTarget);
    context->SetTarget(target.Get());

    context->BeginDraw();
    context->Clear(D2D1::ColorF(0, 0));

    ComPtr<ID2D1BitmapBrush> brush;
    context->CreateBitmapBrush(
        skyboxBitmap.Get(),
        D2D1::BitmapBrushProperties(D2D1_EXTEND_MODE_WRAP, D2D1_EXTEND_MODE_CLAMP),
        D2D1::BrushProperties(),
        &brush);

    // 화면 세로 크기에 맞게 브러시 스케일 (원본 세로 608px 기준)
    UINT32 srcPixelH;
    skyboxBitmap->GetSize(nullptr, &srcPixelH); // 실제로는 GetPixelSize(&w,&h) 사용 권장
    float scale = (float)screenH / 608.0f;
    brush->SetTransform(D2D1::Matrix3x2F::Scale(scale, scale));

    context->FillRectangle(D2D1::RectF(0, 0, (float)screenW, (float)screenH), brush.Get());

    context->EndDraw();
    context->SetTarget(prevTarget.Get());

    cachedBackground = target;
    */
}

void Dungeon::Update(double deltaTime)
{
    super::Update(deltaTime);
}
