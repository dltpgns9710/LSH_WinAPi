#include "ObjectEditor.h"

#include <cmath>
#include <algorithm>

#include "../D2DFramework/Manager/include/DataManager.h"
#include "../D2DFramework/Manager/include/SpriteSheetManager.h"
#include "../D2DFramework/Manager/include/InputManager.h"
#include "../D2DFramework/Graphics/include/SpriteSheet.h"
#include "../D2DFramework/Camera/include/Camera.h"
#include "../D2DFramework/Math/include/MathUtil.h"

#include <dwrite.h>
#pragma comment(lib, "dwrite.lib")

namespace
{
    constexpr float kHalfPi = static_cast<float>(MathUtil::pi / 2.0);
    constexpr float kCameraDistance = 400.f; // 오브젝트 피벗과 카메라 사이 거리(cm)
    constexpr float kCameraHeight = 150.f;
    constexpr float kRotationSpeed = static_cast<float>(MathUtil::pi); // A/D로 회전시키는 속도, 라디안/초 (180도/초)
    constexpr float kZoomSpeed = 300.f;         // W/S로 줌인/줌아웃하는 속도, cm/초
    constexpr float kMinZoomDistance = 100.f;   // 오브젝트에 너무 가까이 가지 않도록
    constexpr float kMaxZoomDistance = 1200.f;  // 너무 멀어지지 않도록
    constexpr float kVerticalMoveSpeed = 300.f; // 1/2로 상하 이동하는 속도, cm/초

    // Matrix4x4::RotationY와 동일한 회전(Y축, 라디안)을 벡터 하나에 적용한다.
    // 깊이 정렬 시 각 파츠의 로컬 좌표를 실제 렌더링과 같은 방식으로 회전시키기 위해 사용.
    Vector3 RotateY(const Vector3& v, float radians)
    {
        float s = std::sin(radians);
        float c = std::cos(radians);
        return Vector3(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
    }
}

void ObjectEditor::Load()
{
    DataManager::GetInstance().Load(L"SpriteTextureMap", L"sprite_texture_map.json");
    DataManager::GetInstance().GetDataAs(L"SpriteTextureMap", spriteMapData);

    GetCamera()->SetPosition(Vector3(0.f, kCameraHeight, -kCameraDistance));
    objectPivot = Vector3(0.f, 0.f, kCameraDistance);

    // 오브젝트 기본 정면(로컬 +Z, XY 평면에 놓인 채로 카메라를 바라보는 방향)이 카메라를 향하도록
    // 초기 회전각을 구한다. RotationY(θ)는 로컬 +Z축 (0,0,1)을 world (sinθ, 0, cosθ)로 보내므로,
    // 이 값이 카메라 방향(toCam)과 같아지는 θ가 atan2(toCam.x, toCam.z)다. 여기에 과거
    // (Dungeon.cpp TestQuad에서 가져온) 90도 보정을 더하면 평면이 XY가 아니라 ZY로 돌아가 버려서
    // 나무/잎이 카메라를 정면으로 보지 못하고 옆면(edge-on)만 보이는 문제가 있었다 - 그 보정은 제거한다.
    Vector3 toCam = GetCamera()->GetPosition() - objectPivot;
    float rawAngle = std::atan2(toCam.x, toCam.z);
    groupAngle = std::round(rawAngle / kHalfPi) * kHalfPi;

    // 화면 우측 상단에 현재 보고 있는 sprite 이름을 표시하기 위한 DirectWrite 리소스 준비
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(nameDWriteFactory.GetAddressOf()));
    nameDWriteFactory->CreateTextFormat(L"Consolas", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        20.0f, L"en-us", nameTextFormat.GetAddressOf());
    nameTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING); // 오른쪽 정렬
    graphics->GetDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), nameBrush.GetAddressOf());

    LoadSpriteAt(0);
}

void ObjectEditor::UnLoad()
{
}

void ObjectEditor::LoadSpriteAt(int index)
{
    currentParts.clear();
    if (spriteMapData.sprites.empty()) return;

    int count = static_cast<int>(spriteMapData.sprites.size());
    currentSpriteIndex = ((index % count) + count) % count;

    const Sprite& sprite = spriteMapData.sprites[currentSpriteIndex];
    currentParts.reserve(sprite.textures.size());

    for (const TextureEntry& tex : sprite.textures)
    {
        const TextureListEntry* texInfo = spriteMapData.FindTexture(tex.textureIndex);
        if (!texInfo || !tex.atlasRectPx.has_value()) continue; // 헬퍼(BoundingBox 등) 스킵

        // 세 축 중 y가 제일 작다고 무조건 바닥은 아니다 - 십자형 캐노피/잎처럼 세 축이 비슷비슷하게
        // 큰 경우(예: y가 x,z보다 살짝만 작은 경우)까지 바닥으로 오판하면 공중에 떠 있는데도 눕혀서
        // 그려진다. y가 x/z보다 뚜렷하게(80% 이하로) 작을 때만 바닥으로 취급한다.
        float ax = std::abs(tex.size.x), ay = std::abs(tex.size.y), az = std::abs(tex.size.z);
        float minXZ = (ax < az) ? ax : az;
        bool isFloor = (ay <= 0.8f * minXZ);

        // 실제로 그려질 폭/높이가 0에 가까운 파츠는 스킵한다. 예: d01_f01_bg_01은 size.x=0인 경우가
        // 있는데(원본 데이터가 이 축을 정의하지 않은 경우), 이런 걸 그대로 그리면 폭 0짜리 평면이
        // D2D 3D 변환 이펙트에서 완전히 사라지지 않고 가느다란 실선으로 남아 화면에 삐죽 나와 보인다.
        bool degenerate = isFloor ? (ax < 1.f || az < 1.f) : (ax < 1.f || ay < 1.f);
        if (degenerate) continue;

        std::wstring texKey(texInfo->texture.begin(), texInfo->texture.end());
        std::string fileName = texInfo->texturePath.substr(texInfo->texturePath.find_last_of('/') + 1);
        std::wstring wFileName(fileName.begin(), fileName.end());

        SpriteSheetManager::GetInstance().LoadTexture(texKey, wFileName);
        std::shared_ptr<SpriteSheet> fullSheet = SpriteSheetManager::GetInstance().GetTexture(texKey);
        if (!fullSheet) continue;

        const AtlasRect& rect = *tex.atlasRectPx;
        RenderPart part;
        if (tex.tiled)
        {
            // tiled 조각 하나를 늘려서 그리면(CreateSubRegion) 반복될 부분이 아닌데도 크게 늘어나
            // 보인다. repeatX/repeatY만큼 실제로 반복시킨 비트맵을 만들어서 써야 의도한 크기로 보인다.
            part.sheet = fullSheet->CreateTiledRegion(
                static_cast<float>(rect.x), static_cast<float>(rect.y),
                static_cast<float>(rect.x + rect.width), static_cast<float>(rect.y + rect.height),
                tex.repeatX, tex.repeatY);
        }
        else
        {
            part.sheet = fullSheet->CreateSubRegion(
                static_cast<float>(rect.x), static_cast<float>(rect.y),
                static_cast<float>(rect.x + rect.width), static_cast<float>(rect.y + rect.height));
        }
        if (!part.sheet) continue;

        part.localPosition = tex.position;
        part.size = tex.size;
        part.isFloor = isFloor;

        // 파츠별 추가 회전은 텍스처 이름으로 하드코딩하지 않고 data(rotationY)로만 결정한다.
        // json 데이터만으로도 다른 렌더러가 동일한 결과를 재현할 수 있어야 하기 때문 -
        // 예전에는 "d01_f01_plants_01_wood" 텍스처를 코드에서 이름으로 특별 취급해 90도를 더했었는데,
        // 그 정보가 json에 없어서 json만 보고 렌더링하면 재현이 안 됐다. 지금은 그 텍스처를 쓰는
        // 파츠가 전부 병합되어 사라졌고, 필요한 회전은 각 파츠의 rotationY 필드에 직접 기록돼 있다.
        part.extraYRotation = tex.rotationY * static_cast<float>(MathUtil::pi / 180.0);

        currentParts.push_back(std::move(part));
    }
}

void ObjectEditor::Render()
{
    // Dungeon과 달리 화면 전체를 덮는 배경을 그리지 않으므로, 이전 프레임(다른 레벨)의 잔상이
    // 남지 않도록 매 프레임 명시적으로 지운다.
    graphics->ClearScreen(0.f, 0.f, 0.f);

    super::Render();

    D2D1_SIZE_F clientSize = graphics->GetDeviceContext()->GetSize();

    // 우측 상단에 지금 보고 있는 sprite 이름 표시
    if (!spriteMapData.sprites.empty())
    {
        const std::string& name = spriteMapData.sprites[currentSpriteIndex].name;
        std::wstring wname(name.begin(), name.end());
        D2D1_RECT_F nameRect = D2D1::RectF(clientSize.width - 400.f, 10.f, clientSize.width - 10.f, 40.f);
        graphics->GetDeviceContext()->DrawText(
            wname.c_str(), static_cast<UINT32>(wname.size()), nameTextFormat.Get(), nameRect, nameBrush.Get());
    }

    if (currentParts.empty()) return;

    Matrix4x4 cameraView = GetCamera()->GetViewMatrix();
    Matrix4x4 cameraProj = GetCamera()->GetProjectionMatrix();
    Matrix4x4 viewport = Matrix4x4::Viewport(clientSize.width, clientSize.height);
    Matrix4x4 viewProj = cameraProj * cameraView; // z-버퍼 정렬용 (원근분할까지 적용해서 실제 depth 값을 구함)

    Matrix4x4 groupRotation = Matrix4x4::Translation(objectPivot) * Matrix4x4::RotationY(groupAngle);

    // D2D는 깊이 테스트 없이 그린 순서대로 위에 덮어 그리므로, 카메라로부터 먼 텍스처부터 그려야 한다.
    // 바닥(y≈0, 법선=Y)은 z-버퍼 값과 무관하게 항상 맨 먼저(=맨 뒤) 그린다 - 바닥처럼 카메라 쪽으로
    // 넓게 깔린 조각은 중심 좌표 하나로 계산한 깊이가 실제 겹침 관계를 제대로 반영하지 못해서다.
    // 다만 상자 뚜껑처럼 공중에 떠 있는(y가 0이 아닌) FLOOR 조각까지 이 규칙을 적용하면, 나중에(=더
    // 앞에) 그려지는 옆면 파츠에 항상 덮여 안 보이는 문제가 있었다 - 이런 조각은 일반 깊이 정렬에
    // 맡긴다.
    auto isGroundFloor = [](const RenderPart* p) { return p->isFloor && std::abs(p->localPosition.y) < 1.f; };

    std::vector<const RenderPart*> drawOrder;
    drawOrder.reserve(currentParts.size());
    for (const RenderPart& part : currentParts) drawOrder.push_back(&part);
    std::sort(drawOrder.begin(), drawOrder.end(), [&](const RenderPart* a, const RenderPart* b)
        {
            if (isGroundFloor(a) != isGroundFloor(b)) return isGroundFloor(a); // 바닥이 항상 먼저(맨 뒤)

            auto ndcDepth = [&](const RenderPart* p)
                {
                    // groupRotation(오브젝트 회전)까지 반영해야 실제로 그려지는 위치와 같은 깊이를 비교한다.
                    // 이걸 빼먹으면 A/D로 돌렸을 때 실제 화면상 앞뒤 관계와 정렬 순서가 어긋난다.
                    Vector3 w = objectPivot + RotateY(p->localPosition, groupAngle);
                    float clipZ = viewProj.m[2][0] * w.x + viewProj.m[2][1] * w.y
                        + viewProj.m[2][2] * w.z + viewProj.m[2][3];
                    float clipW = viewProj.m[3][0] * w.x + viewProj.m[3][1] * w.y
                        + viewProj.m[3][2] * w.z + viewProj.m[3][3];
                    return clipW != 0.f ? clipZ / clipW : clipZ; // z-버퍼 값(클수록 카메라에서 멀다)
                };
            return ndcDepth(a) > ndcDepth(b); // 먼 것부터(내림차순) 그린다
        });

    for (const RenderPart* partPtr : drawOrder)
    {
        const RenderPart& part = *partPtr;
        if (!part.sheet) continue;

        float pixelW = part.sheet->GetImageWidth();
        float pixelH = part.sheet->GetImageHeight();
        if (pixelW <= 0.f || pixelH <= 0.f) continue;

        Matrix4x4 extraRotation = Matrix4x4::RotationY(part.extraYRotation);

        // 법선 축은 size(x,y,z)로 추정하지 않고, 바닥(isFloor)이 아니면 기본적으로 Z축(정면을 바라보는
        // 수직 평면)으로 그린다. sprite마다 파츠가 서로 다른 축으로 추정되면 같은 오브젝트인데 서로 다른
        // 방향을 보는 것처럼 보이는 문제가 있었다.
        Matrix4x4 localModel; // 회전군 적용 전, pivot 로컬 공간 기준
        if (part.isFloor)
        {
            // 법선 = Y축 (바닥/천장류)
            localModel = Matrix4x4::Translation(part.localPosition)
                * extraRotation
                * Matrix4x4::RotationX(kHalfPi)
                * Matrix4x4::Scale(Vector3(part.size.x / pixelW, part.size.z / pixelH, 1.f))
                * Matrix4x4::Translation(Vector3(-pixelW / 2.f, -pixelH / 2.f, 0.f));
        }
        else
        {
            // 법선 = Z축 (정면을 바라보는 수직 평면) - 바닥이 아닌 모든 파츠의 기본값
            localModel = Matrix4x4::Translation(part.localPosition)
                * extraRotation
                * Matrix4x4::Scale(Vector3(part.size.x / pixelW, -part.size.y / pixelH, 1.f))
                * Matrix4x4::Translation(Vector3(-pixelW / 2.f, -pixelH / 2.f, 0.f));
        }

        Matrix4x4 model = groupRotation * localModel;
        Matrix4x4 final = viewport * cameraProj * cameraView * model;
        part.sheet->DrawSpriteWarped(final);
    }
}

void ObjectEditor::Update(double deltaTime)
{
    super::Update(deltaTime);

    if (InputManager::GetInstance().GetButtonDown(KeyType::Q))
    {
        LoadSpriteAt(currentSpriteIndex - 1);
    }
    if (InputManager::GetInstance().GetButtonDown(KeyType::E))
    {
        LoadSpriteAt(currentSpriteIndex + 1);
    }

    // A/D를 누르고 있는 동안 오브젝트를 좌/우로 회전시켜 다른 각도에서 볼 수 있게 한다.
    if (InputManager::GetInstance().GetButtonPressed(KeyType::A))
    {
        groupAngle -= kRotationSpeed * static_cast<float>(deltaTime);
    }
    if (InputManager::GetInstance().GetButtonPressed(KeyType::D))
    {
        groupAngle += kRotationSpeed * static_cast<float>(deltaTime);
    }

    // W/S를 누르고 있는 동안 카메라를 오브젝트 쪽으로/반대쪽으로 이동시켜 확대/축소한다.
    bool zoomIn = InputManager::GetInstance().GetButtonPressed(KeyType::W);
    bool zoomOut = InputManager::GetInstance().GetButtonPressed(KeyType::S);
    if (zoomIn || zoomOut)
    {
        Vector3 pos = GetCamera()->GetPosition();
        if (zoomIn) pos.z += kZoomSpeed * static_cast<float>(deltaTime);
        if (zoomOut) pos.z -= kZoomSpeed * static_cast<float>(deltaTime);
        pos.z = MathUtil::Clamp(pos.z, objectPivot.z - kMaxZoomDistance, objectPivot.z - kMinZoomDistance);
        GetCamera()->SetPosition(pos);
    }

    // 1/2를 누르고 있는 동안 카메라를 위/아래로 이동시켜 다른 높이에서 볼 수 있게 한다.
    bool moveUp = InputManager::GetInstance().GetButtonPressed(KeyType::KEY_1);
    bool moveDown = InputManager::GetInstance().GetButtonPressed(KeyType::KEY_2);
    if (moveUp || moveDown)
    {
        Vector3 pos = GetCamera()->GetPosition();
        if (moveUp) pos.y += kVerticalMoveSpeed * static_cast<float>(deltaTime);
        if (moveDown) pos.y -= kVerticalMoveSpeed * static_cast<float>(deltaTime);
        GetCamera()->SetPosition(pos);
    }
}
