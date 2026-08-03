#include "Dungeon.h"

#include <cstdlib>
#include <ctime>

#include "../D2DFramework/Graphics/include/SpriteAtlas.h"
#include "../D2DFramework/Manager/include/SpriteSheetManager.h"
#include "../D2DFramework/Camera/include/Camera.h"

namespace
{
    constexpr float kHalfPi = 1.5707963f;
    constexpr float kTileSize = 300.f;
    constexpr int kGridSizeX = 10;
    constexpr int kGridSizeZ = 3;
    constexpr float kGridStartX = -400.f;
    constexpr float kGridStartZ = 100.f;
    // 카메라가 X축 기준 타일 격자 중앙에 오도록.
    constexpr float kCameraX = kGridStartX + (kGridSizeX * kTileSize) / 2.f;
}

void Dungeon::Load()
{
    SpriteSheetManager::GetInstance().LoadTexture(L"Background", L"Background.png");
    background = make_shared<SpriteAtlas>(SpriteSheetManager::GetInstance().GetTexture(L"Background"));
    background->SetDrawRegion(0,250,background->GetClientWidthSize(),background->GetClientHeightSize());

    SpriteSheetManager::GetInstance().LoadTexture(L"FloorTest", L"d01_f01_floor_01.png");
    std::shared_ptr<SpriteSheet> floorAtlas = SpriteSheetManager::GetInstance().GetTexture(L"FloorTest");

    // 아틀라스는 같은 크기의 타일 4개(2x2)로 구성되어 있다. 그중 상단 2종류만 사용한다.
    float tileImgW = floorAtlas->GetImageWidth() / 2.f;
    float tileImgH = floorAtlas->GetImageHeight() / 2.f;
    floorTiles[0] = floorAtlas->CreateSubRegion(0.f, 0.f, tileImgW, tileImgH);
    floorTiles[1] = floorAtlas->CreateSubRegion(tileImgW, 0.f, tileImgW * 2.f, tileImgH);

    std::srand(static_cast<unsigned>(std::time(nullptr)));
    floorGrid.reserve(kGridSizeX * kGridSizeZ);
    for (int i = 0; i < kGridSizeX; ++i)
    {
        for (int j = 0; j < kGridSizeZ; ++j)
        {
            FloorTileInstance tile;
            tile.position = Vector3(kGridStartX + i * kTileSize, 0.f, kGridStartZ + j * kTileSize);
            tile.tileIndex = std::rand() % 2;
            floorGrid.push_back(tile);
        }
    }

    GetCamera()->SetPosition(Vector3(kCameraX, 150.f, 50.f));
}

void Dungeon::UnLoad()
{
}

void Dungeon::Render()
{
    background->DrawSpriteAtlas(0,0,background->GetClientWidthSize(),background->GetClientHeightSize());
    super::Render();

    Matrix4x4 viewProj = GetCamera()->GetViewProjectionMatrix();
    Matrix4x4 viewport = Matrix4x4::Viewport(background->GetClientWidthSize(), background->GetClientHeightSize());

    for (const FloorTileInstance& tile : floorGrid)
    {
        std::shared_ptr<SpriteSheet>& sprite = floorTiles[tile.tileIndex];

        // 평평한 비트맵 평면(로컬 z=0)을 90도 눕혀서 XZ 바닥 평면에 놓는다.
        // (Y 출력을 그냥 0으로 고정하면 행렬이 특이(singular)해져 3D 워프 이펙트가 아무것도 못 그린다.)
        Matrix4x4 model = Matrix4x4::Translation(tile.position)
            * Matrix4x4::RotationX(kHalfPi)
            * Matrix4x4::Scale(Vector3(kTileSize / sprite->GetImageWidth(), kTileSize / sprite->GetImageHeight(), 1.f));
        Matrix4x4 final = viewport * viewProj * model;

        sprite->DrawSpriteWarped(final);
    }
}

void Dungeon::Update(double deltaTime)
{
    super::Update(deltaTime);
}
