#include "Dungeon.h"

#include <cstdlib>
#include <ctime>
#include <limits>
#include <cmath>
#include <algorithm>

#include "../D2DFramework/Graphics/include/SpriteAtlas.h"
#include "../D2DFramework/Manager/include/SpriteSheetManager.h"
#include "../D2DFramework/Manager/include/DataManager.h"
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
    constexpr float kTestInstanceSpacing = 400.f; // 테스트용 d01_w01_0101을 좌우로 나열할 때 간격(cm)
    constexpr int kTestInstanceCountEachSide = 4;  // 원본 좌/우로 각각 몇 개씩 더 그릴지
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
    Matrix4x4 viewProj = cameraProj * cameraView; // z-버퍼 정렬용 (원근분할까지 적용해서 실제 depth 값을 구함)

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

    // ---- 테스트용: d01_w01_0101 스프라이트를 카메라 앞 Z축 2칸 위치에 렌더링 ----
    // sprite_texture_map.json의 textures[]를 그대로 읽어와 평면 하나씩 그린다.
    // (임시 검증 코드. 다른 구조는 건드리지 않고 Render() 안에서만 처리)
    struct TestQuad
    {
        std::shared_ptr<SpriteSheet> sheet;
        Vector3 localPosition; // 피벗(0,0,0) 기준 상대 좌표, cm
        Vector3 size;          // cm, 한 축이 0에 가까운 쪽이 이 평면의 법선
        bool isFloor = false;      // 법선=Y(바닥류) 여부 - chipName:"FLOOR"에 대응, z-버퍼 무관하게 항상 먼저 그림
        float extraYRotation = 0.f; // 축 추정만으로는 방향이 어색한 특정 파츠에 대한 보정용 추가 회전
    };
    static std::vector<TestQuad> testQuads;
    static Vector3 testPivot;
    static float testGroupAngle = 0.f;
    static bool testQuadsLoaded = false;
    if (!testQuadsLoaded)
    {
        testQuadsLoaded = true;

        // "지금 카메라 위치 기준"이므로 최초 1회만 스냅샷을 떠서 고정 배치한다
        // (매 프레임 갱신하면 카메라를 따라다니게 됨).
        testPivot = GetCamera()->GetPosition();
        testPivot.y = 0.f; // sprite_texture_map의 피벗은 항상 바닥면 중점
        testPivot.z += FloorTileInstance::kTileSize * 2.f; // 카메라 기준 Z축으로 2칸 앞

        // 처음 그릴 때 딱 한 번, 오브젝트의 기본 정면(+Z)이 카메라를 향하도록 90도 단위로 스냅 회전각을
        // 구해서 고정한다 (매 프레임 다시 계산하지 않음 - 이후 카메라가 움직여도 방향은 안 바뀜).
        Vector3 toCam = GetCamera()->GetPosition() - testPivot;
        float rawAngle = std::atan2(toCam.x, toCam.z);
        testGroupAngle = std::round(rawAngle / kHalfPi) * kHalfPi;

        // 지금 계산대로면 오브젝트가 X축을 바라봐서, Z축을 바라보도록 90도 고정 보정한다.
        // 이후 매 프레임 다시 도는 게 아니라 이 값 그대로 고정.
        testGroupAngle += kHalfPi;

        DataManager::GetInstance().Load(L"SpriteTextureMap", L"sprite_texture_map.json");
        nlohmann::json* mapJson = DataManager::GetInstance().GetData(L"SpriteTextureMap");
        if (mapJson)
        {
            for (auto& spriteJson : (*mapJson)["sprites"])
            {
                if (spriteJson["name"].get<std::string>() != "d01_w01_0101") continue;

                for (auto& texJson : spriteJson["textures"])
                {
                    if (texJson["texture"].is_null()) continue; // 안 보이는 헬퍼(BoundingBox 등) 스킵

                    std::string texKey = texJson["texture"].get<std::string>();

                    // 참고: d01_f01_plants_01의 atlasRectPx는 잎 하나가 아니라 서로 다른 식물 여러 개가
                    // 뭉친 아틀라스 영역 전체다. 원래는 tiled+repeatX/Y로 그중 작은 조각 하나만 반복해서
                    // 쓰는 텍스처인데, 반복(tiling) 로직을 구현하지 않아서 그 전체가 하나로 늘어나 보인다.

                    std::string texPath = texJson["texturePath"].get<std::string>();
                    std::string fileName = texPath.substr(texPath.find_last_of('/') + 1);

                    SpriteSheetManager::GetInstance().LoadTexture(
                        std::wstring(texKey.begin(), texKey.end()),
                        std::wstring(fileName.begin(), fileName.end()));
                    std::shared_ptr<SpriteSheet> fullSheet =
                        SpriteSheetManager::GetInstance().GetTexture(std::wstring(texKey.begin(), texKey.end()));
                    if (!fullSheet) continue;

                    auto& rect = texJson["atlasRectPx"];
                    float rx = rect["x"].get<float>();
                    float ry = rect["y"].get<float>();
                    float rw = rect["width"].get<float>();
                    float rh = rect["height"].get<float>();

                    TestQuad quad;
                    quad.sheet = fullSheet->CreateSubRegion(rx, ry, rx + rw, ry + rh);

                    auto& pos = texJson["position"];
                    auto& size = texJson["size"];
                    quad.localPosition = Vector3(pos["x"].get<float>(), pos["y"].get<float>(), pos["z"].get<float>());
                    quad.size = Vector3(size["x"].get<float>(), size["y"].get<float>(), size["z"].get<float>());
                    {
                        float qax = std::abs(quad.size.x), qay = std::abs(quad.size.y), qaz = std::abs(quad.size.z);
                        quad.isFloor = (qay <= qax && qay <= qaz); // 법선=Y축(바닥에 눕는 케이스)
                    }

                    // 나무 기둥(wood)은 축 크기만으로 법선을 추정하면 카메라 쪽으로 옆면(90도 돌아간 상태)이
                    // 보여서 실제 몸통 폭이 아니라 얇은 단면처럼 보인다. 90도 더 돌려서 넓은 면이 보이게 한다.
                    if (texKey == "d01_f01_plants_01_wood") quad.extraYRotation = kHalfPi;

                    testQuads.push_back(quad);
                }
                break;
            }
        }
    }

    // 테스트용으로 원본 1개 + 좌우로 kTestInstanceCountEachSide개씩 더, X축으로 나열해서 그린다.
    std::vector<Vector3> instancePivots;
    instancePivots.reserve(kTestInstanceCountEachSide * 2 + 1);
    for (int i = -kTestInstanceCountEachSide; i <= kTestInstanceCountEachSide; ++i)
    {
        instancePivots.push_back(testPivot + Vector3(i * kTestInstanceSpacing, 0.f, 0.f));
    }

    for (const Vector3& pivot : instancePivots)
    {
        // 매 프레임 카메라를 따라 도는 것이 아니라, 처음 그릴 때 계산해 둔 고정 각도(testGroupAngle)만큼만 돌린다.
        Matrix4x4 groupRotation = Matrix4x4::Translation(pivot) * Matrix4x4::RotationY(testGroupAngle);

        // D2D는 깊이 테스트 없이 그린 순서대로 위에 덮어 그리므로, 카메라로부터 먼 텍스처부터 그려야
        // 가까운 장식(나무/잎 등)이 먼 배경(bg_01 등) 위에 자연스럽게 겹쳐 보인다.
        // 정렬 기준은 실제 z-버퍼 값(NDC z, 원근분할 z/w까지 적용)을 쓴다 - viewProj로 클립 공간까지
        // 보내고 w로 나눠서 [-1,1] 깊이값을 구한다. 단순 3D 유클리드 거리로 하면 안 된다 - bg_01은
        // y=800(벽 높은 곳)에 있고 다른 장식들은 y=20~650(카메라 높이 100 근처)에 있어서, 높이 차이 때문에
        // 실제로는 더 가까운 bg_01이 "먼 것"으로 잘못 판정된다.
        // 단, FLOOR(법선=Y, 바닥에 눕는 케이스)는 z-버퍼 값과 무관하게 항상 맨 먼저(=맨 뒤) 그린다.
        std::vector<const TestQuad*> drawOrder;
        drawOrder.reserve(testQuads.size());
        for (const TestQuad& quad : testQuads) drawOrder.push_back(&quad);
        std::sort(drawOrder.begin(), drawOrder.end(), [&](const TestQuad* a, const TestQuad* b)
            {
                if (a->isFloor != b->isFloor) return a->isFloor; // FLOOR가 항상 먼저(맨 뒤)

                auto ndcDepth = [&](const TestQuad* q)
                    {
                        Vector3 w = pivot + q->localPosition;
                        float clipZ = viewProj.m[2][0] * w.x + viewProj.m[2][1] * w.y
                            + viewProj.m[2][2] * w.z + viewProj.m[2][3];
                        float clipW = viewProj.m[3][0] * w.x + viewProj.m[3][1] * w.y
                            + viewProj.m[3][2] * w.z + viewProj.m[3][3];
                        return clipW != 0.f ? clipZ / clipW : clipZ; // z-버퍼 값(클수록 카메라에서 멀다)
                    };
                return ndcDepth(a) > ndcDepth(b); // 먼 것부터(내림차순) 그린다
            });

        for (const TestQuad* quadPtr : drawOrder)
        {
            const TestQuad& quad = *quadPtr;
            if (!quad.sheet) continue;

            float pixelW = quad.sheet->GetImageWidth();
            float pixelH = quad.sheet->GetImageHeight();
            if (pixelW <= 0.f || pixelH <= 0.f) continue;

            float ax = std::abs(quad.size.x);
            float ay = std::abs(quad.size.y);
            float az = std::abs(quad.size.z);

            Matrix4x4 extraRotation = Matrix4x4::RotationY(quad.extraYRotation);

            Matrix4x4 localModel; // 회전군 적용 전, pivot 로컬 공간 기준
            if (ax <= ay && ax <= az)
            {
                // 법선 = X축 (좌우를 바라보는 수직 평면)
                localModel = Matrix4x4::Translation(quad.localPosition)
                    * extraRotation
                    * Matrix4x4::RotationY(kHalfPi)
                    * Matrix4x4::Scale(Vector3(quad.size.z / pixelW, -quad.size.y / pixelH, 1.f))
                    * Matrix4x4::Translation(Vector3(-pixelW / 2.f, -pixelH / 2.f, 0.f));
            }
            else if (ay <= ax && ay <= az)
            {
                // 법선 = Y축 (바닥/천장류, 기존 floorGrid와 동일한 눕히기 방식)
                localModel = Matrix4x4::Translation(quad.localPosition)
                    * extraRotation
                    * Matrix4x4::RotationX(kHalfPi)
                    * Matrix4x4::Scale(Vector3(quad.size.x / pixelW, quad.size.z / pixelH, 1.f))
                    * Matrix4x4::Translation(Vector3(-pixelW / 2.f, -pixelH / 2.f, 0.f));
            }
            else
            {
                // 법선 = Z축 (정면을 바라보는 수직 평면)
                localModel = Matrix4x4::Translation(quad.localPosition)
                    * extraRotation
                    * Matrix4x4::Scale(Vector3(quad.size.x / pixelW, -quad.size.y / pixelH, 1.f))
                    * Matrix4x4::Translation(Vector3(-pixelW / 2.f, -pixelH / 2.f, 0.f));
            }

            Matrix4x4 model = groupRotation * localModel;
            Matrix4x4 testFinal = viewport * cameraProj * cameraView * model;
            quad.sheet->DrawSpriteWarped(testFinal);
        }
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
