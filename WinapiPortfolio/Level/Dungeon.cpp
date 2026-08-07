#include "Dungeon.h"

#include <cstdlib>
#include <ctime>
#include <limits>

#include "../D2DFramework/Graphics/include/SpriteAtlas.h"
#include "../D2DFramework/Manager/include/SpriteSheetManager.h"
#include "../D2DFramework/Camera/include/Camera.h"
#include "../D2DFramework/Manager/include/InputManager.h"
#include "../D2DFramework/Math/include/MathUtil.h"

namespace
{
    constexpr float kHalfPi = static_cast<float>(MathUtil::pi / 2.0);
    constexpr int kGridSizeX = 500;
    constexpr int kGridSizeZ = 300;
    constexpr float kGridStartX = -400.f;
    constexpr float kGridStartZ = 100.f;
    // 카메라가 X축 기준 타일 격자 중앙에 오도록.
    constexpr float kCameraX = FloorTileInstance::kTileSize/2 + kGridStartX + (kGridSizeX * FloorTileInstance::kTileSize) / 2.f;
    constexpr float kCameraZ = FloorTileInstance::kTileSize/2 + kGridStartZ + (kGridSizeZ * FloorTileInstance::kTileSize) / 2.f;
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
            tile.position = Vector3(kGridStartX + i * FloorTileInstance::kTileSize, 0.f, kGridStartZ + j * FloorTileInstance::kTileSize);
            tile.tileIndex = std::rand() % 2;
            floorGrid.push_back(tile);
        }
    }

    GetCamera()->SetPosition(Vector3(kCameraX, 100.f, kCameraZ));
    //GetCamera()->SetPosition(Vector3(kCameraX, 100.f, 100.f));
}

void Dungeon::UnLoad()
{
}

void Dungeon::Render()
{
    background->DrawSpriteAtlas(0,0,background->GetClientWidthSize(),background->GetClientHeightSize());
    super::Render();

    Matrix4x4 cameraView = GetCamera()->GetViewMatrix();
    Matrix4x4 cameraProj = GetCamera()->GetProjectionMatrix();
    Matrix4x4 viewport = Matrix4x4::Viewport(background->GetClientWidthSize(), background->GetClientHeightSize());

    for (const FloorTileInstance& tile : floorGrid)
    {
        if (!GetCamera()->isRenderTile(tile)) continue;
            
        std::shared_ptr<SpriteSheet>& sprite = floorTiles[tile.tileIndex];

        Matrix4x4 model = Matrix4x4::Translation(tile.position)
            * Matrix4x4::RotationX(kHalfPi)
            * Matrix4x4::Scale(Vector3(FloorTileInstance::kTileSize / sprite->GetImageWidth(), FloorTileInstance::kTileSize / sprite->GetImageHeight(), 1.f));
        
        Matrix4x4 modelView = cameraView * model;                                                                                                                                              
        /*                                            
        const float EPSILON = 1e-5f;
        if (std::abs(modelView.m[2][3]) < EPSILON) modelView.m[2][3] = -EPSILON;  
        */
        Matrix4x4 final = viewport * cameraProj * modelView;
        
        sprite->DrawSpriteWarped(final);
    }
}

void Dungeon::Update(double deltaTime)
{
    super::Update(deltaTime);
    GetCamera()->Update(deltaTime);
    if (InputManager::GetInstance().GetButtonDown(KeyType::W)) GetCamera()->moveRequest(EMoveDirection::Forward);
    if (InputManager::GetInstance().GetButtonDown(KeyType::S)) GetCamera()->moveRequest(EMoveDirection::Backward);
    if (InputManager::GetInstance().GetButtonDown(KeyType::Q)) GetCamera()->moveRequest(EMoveDirection::Left);
    if (InputManager::GetInstance().GetButtonDown(KeyType::E)) GetCamera()->moveRequest(EMoveDirection::Right);
    if (InputManager::GetInstance().GetButtonDown(KeyType::A)) GetCamera()->rotateRequest(ERotateDirection::Left);
    if (InputManager::GetInstance().GetButtonDown(KeyType::D)) GetCamera()->rotateRequest(ERotateDirection::Right);
    
    //if (InputManager::GetInstance().GetButtonPressed(KeyType::W)) GetCamera()->MoveZ(100.f*deltaTime);
    //if (InputManager::GetInstance().GetButtonPressed(KeyType::S)) GetCamera()->MoveZ(-100.f*deltaTime);
    //if (InputManager::GetInstance().GetButtonPressed(KeyType::Q)) GetCamera()->MoveX(-100.f*deltaTime);
    //if (InputManager::GetInstance().GetButtonPressed(KeyType::E)) GetCamera()->MoveX(100.f*deltaTime);
    //if (InputManager::GetInstance().GetButtonPressed(KeyType::A)) GetCamera()->RotateCameraDegree(10.f*deltaTime);
    //if (InputManager::GetInstance().GetButtonPressed(KeyType::D)) GetCamera()->RotateCameraDegree(-10.f*deltaTime);
}
