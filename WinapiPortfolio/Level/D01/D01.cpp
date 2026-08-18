#include "D01.h"

#include <cmath>

#include "../../D2DFramework/Manager/include/DataManager.h"
#include "../../D2DFramework/Manager/include/SpriteSheetManager.h"
#include "../../D2DFramework/Manager/include/InputManager.h"
#include "../../D2DFramework/Graphics/include/SpriteAtlas.h"
#include "../../D2DFramework/Camera/include/Camera.h"
#include "../../Data/SpriteTextureMapData.h"

#include <dwrite.h>
#pragma comment(lib, "dwrite.lib")

namespace
{
    constexpr float kCameraHeight = 150.f; // 오브젝트 피벗과 카메라 사이 눈높이(cm), ObjectEditor와 동일값

    // 던전용 카메라 시야 거리(cm). Camera의 기본값(20, 2000)은 2000/400 = 5칸이라 복도 끝이 잘렸다.
    // Camera::isRenderPosition은 farZ * 1.2 를 컷 기준으로 쓰므로 실제로 보이는 거리는 이 값의 1.2배다.
    constexpr float kCameraNearZ = 20.f;
    constexpr float kCameraFarZ = 2000.f; // 5칸 (컬링 한계는 2400cm = 6칸)

    // facingQuarter(0=+Z, 90도 단위로 반시계) -> 정면 방향 단위 벡터.
    // Camera::moveRequest가 쓰는 forward=(sin(-theta),0,cos(-theta))와 같은 규칙(A=Left가 theta를 90도씩 증가)이다.
    Vector3 QuarterToForward(int quarter)
    {
        switch (((quarter % 4) + 4) % 4)
        {
        case 0: return Vector3(0.f, 0.f, 1.f);
        case 1: return Vector3(-1.f, 0.f, 0.f);
        case 2: return Vector3(0.f, 0.f, -1.f);
        default: return Vector3(1.f, 0.f, 0.f);
        }
    }
}

void D01::Load()
{
    SpriteSheetManager::GetInstance().LoadTexture(L"Background", L"Background.png");
    background = std::make_shared<SpriteAtlas>(SpriteSheetManager::GetInstance().GetTexture(L"Background"));
    background->SetDrawRegion(0, 250, background->GetClientWidthSize(), background->GetClientHeightSize());

    DataManager::GetInstance().Load(L"Dungeon01", L"dungeon_01_01.json");
    DataManager::GetInstance().GetDataAs(L"Dungeon01", dungeonData);

    SpriteTextureMapData spriteMapData;
    DataManager::GetInstance().Load(L"SpriteTextureMap", L"sprite_texture_map.json");
    DataManager::GetInstance().GetDataAs(L"SpriteTextureMap", spriteMapData);

    float cellSize = dungeonData.grid.cellSize;
    templateLibrary.Init(std::move(spriteMapData), cellSize);
    cellPlacer.Init(&dungeonData, &templateLibrary);
    cellPlacer.PlaceAllCells();

    // 시작 위치 기준으로 얇은 벽들의 경사 방향을 첫 프레임부터 맞춰둔다.
    cellPlacer.RefreshFacing(dungeonData.startPosition.x, dungeonData.startPosition.y);

    // Camera 기본 farZ(2000cm)는 셀 5칸 거리라, isRenderPosition의 컷 기준(farZ * 1.2 = 6칸)을 넘는
    // 복도 안쪽이 통째로 그려지지 않아 화면 중앙이 뻥 뚫린 것처럼 보였다. 던전은 시야가 길게 뻗는
    // 복도가 많으므로 D01에서만 늘려 잡는다(전역 기본값을 바꾸면 Dungeon/ObjectEditor까지 영향).
    GetCamera()->SetNearFar(kCameraNearZ, kCameraFarZ);

    Vector3 startWorldPos = GridToWorld(dungeonData.startPosition.x, dungeonData.startPosition.y, cellSize);
    startWorldPos.y = kCameraHeight;
    GetCamera()->SetPosition(startWorldPos);

    // 화면 우측 상단에 던전 이름을 표시하기 위한 DirectWrite 리소스 준비
    /*
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(nameDWriteFactory.GetAddressOf()));
    nameDWriteFactory->CreateTextFormat(L"Consolas", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        20.0f, L"en-us", nameTextFormat.GetAddressOf());
    nameTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    graphics->GetDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), nameBrush.GetAddressOf());
    */
}

void D01::UnLoad()
{
}

void D01::Render()
{
    // Dungeon::Render()와 동일하게, 화면 전체를 덮는 배경을 그려서 이전 프레임(다른 레벨)의
    // 잔상이 남지 않게 한다. 파츠가 그려지지 않는 픽셀(잎/잔디 텍스처의 투명한 부분, 캐노피 위쪽,
    // 시야 밖으로 컬링된 원거리)을 전부 이 배경이 메워준다 - 없으면 그 자리가 그대로 검게 보인다.
    background->DrawSpriteAtlas(0, 0, background->GetClientWidthSize(), background->GetClientHeightSize());

    super::Render();

    D2D1_SIZE_F clientSize = graphics->GetDeviceContext()->GetSize();

    if (cellPlacer.GetPlacedParts().empty()) return;

    partRenderer.Render(cellPlacer.GetPlacedParts(), GetCamera().get(), clientSize);

    // 던전 이름 아래에 카메라 위치/이동 방향 디버그 정보 표시. forward는 W, left/right는 Q/E(스트레이프)로
    // 이동했을 때 도착할 좌표이고, 벽이라도 그대로 계산해서 보여준다(IsWalkable 판정을 거치지 않음).
    // Vector3는 (x, y=높이, z)이고 이 던전의 이동 평면은 x/z이므로, 화면 표기는 x와 z를 각각 x, y로 매핑한다.
    // 월드 좌표(cm)를 그대로 보면 읽기 어려워, cellSize로 나눠 그리드 인덱스로 보여준다. z는 GridToWorld의
    // 부호 반전(z = -(gridY*cellSize))과 짝을 맞춰 -z/cellSize로 나눠야 실제 그리드 y와 일치한다.
    {
        /*
        Vector3 camPos = GetCamera()->GetPosition();
        Vector3 forwardDir = QuarterToForward(facingQuarter);
        Vector3 rightDir(forwardDir.z, 0.f, -forwardDir.x); // Update()의 이동 계산과 동일한 규칙
        float cellSize = dungeonData.grid.cellSize;
        Vector3 forwardPos = camPos + forwardDir * cellSize;
        Vector3 leftPos = camPos - rightDir * cellSize;
        Vector3 rightPos = camPos + rightDir * cellSize;

        auto toGridIndex = [cellSize](const Vector3& worldPos)
            {
                return D2D1::Point2F(worldPos.x / cellSize, -worldPos.z / cellSize);
            };
        D2D1_POINT_2F posIdx = toGridIndex(camPos);
        D2D1_POINT_2F forwardIdx = toGridIndex(forwardPos);
        D2D1_POINT_2F leftIdx = toGridIndex(leftPos);
        D2D1_POINT_2F rightIdx = toGridIndex(rightPos);

        wchar_t debugText[256];
        swprintf_s(debugText,
            L"pos : %.0f, %.0f\nforward : %.0f, %.0f\nleft : %.0f, %.0f\nright : %.0f, %.0f",
            posIdx.x, posIdx.y, forwardIdx.x, forwardIdx.y, leftIdx.x, leftIdx.y, rightIdx.x, rightIdx.y);

        D2D1_RECT_F debugRect = D2D1::RectF(clientSize.width - 400.f, 45.f, clientSize.width - 10.f, 165.f);
        graphics->GetDeviceContext()->DrawText(
            debugText, static_cast<UINT32>(wcslen(debugText)), nameTextFormat.Get(), debugRect, nameBrush.Get());
        */
    }
}

void D01::Update(double deltaTime)
{
    super::Update(deltaTime);
    GetCamera()->Update(deltaTime);

    if (!GetCamera()->IsIdle()) return; // 이전 이동/회전 애니메이션이 끝나기 전에는 새 요청을 보내지 않는다.

    float cellSize = dungeonData.grid.cellSize;
    Vector3 forward = QuarterToForward(facingQuarter);
    Vector3 right(forward.z, 0.f, -forward.x); // Camera::moveRequest와 동일한 규칙

    // 이동 요청 전에 목적지 칸이 FLOOR인지 먼저 확인한다. 카메라가 idle일 때만 여기 들어오므로,
    // walkable이면 moveRequest는 반드시 실제로 이동을 수행한다.
    auto tryMove = [&](EMoveDirection dir, const Vector3& delta)
        {
            Vector3 dest = GetCamera()->GetPosition() + delta;
            int gridX = static_cast<int>(std::lround(dest.x / cellSize));
            int gridY = static_cast<int>(std::lround(-dest.z / cellSize)); // GridToWorld의 z 반전과 짝을 맞춘 역변환
            if (!cellPlacer.IsWalkable(gridX, gridY)) return;
            GetCamera()->moveRequest(dir);
            cellPlacer.RefreshFacing(gridX, gridY); // 카메라가 도착할 칸 기준으로 얇은 벽의 경사 방향을 갱신
        };

    // Dungeon과 동일한 그리드 이동/회전 조작: W/S/Q/E로 한 칸씩 이동, A/D로 90도 회전.
    if (InputManager::GetInstance().GetButtonDown(KeyType::W)) tryMove(EMoveDirection::Forward, forward * cellSize);
    if (InputManager::GetInstance().GetButtonDown(KeyType::S)) tryMove(EMoveDirection::Backward, forward * -cellSize);
    if (InputManager::GetInstance().GetButtonDown(KeyType::Q)) tryMove(EMoveDirection::Left, right * -cellSize);
    if (InputManager::GetInstance().GetButtonDown(KeyType::E)) tryMove(EMoveDirection::Right, right * cellSize);

    if (InputManager::GetInstance().GetButtonDown(KeyType::A))
    {
        GetCamera()->rotateRequest(ERotateDirection::Left);
        facingQuarter = (facingQuarter + 1) % 4;
    }
    if (InputManager::GetInstance().GetButtonDown(KeyType::D))
    {
        GetCamera()->rotateRequest(ERotateDirection::Right);
        facingQuarter = (facingQuarter + 3) % 4;
    }
}
