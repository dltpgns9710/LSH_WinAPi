#include "SpriteSetEditor.h"

#include <cmath>
#include <cfloat>
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
    constexpr float kCameraDistance = 400.f; // 오브젝트 피벗과 카메라 사이 거리(cm), ObjectEditor와 동일값
    constexpr float kCameraHeight = 150.f;
    constexpr float kRotationSpeed = static_cast<float>(MathUtil::pi); // A/D로 회전시키는 속도, 라디안/초 (180도/초)
    constexpr float kZoomSpeed = 300.f;         // W/S로 줌인/줌아웃하는 속도, cm/초
    constexpr float kMinZoomDistance = 100.f;   // 오브젝트에 너무 가까이 가지 않도록
    constexpr float kMaxZoomDistance = 1200.f;  // 너무 멀어지지 않도록
    constexpr float kVerticalMoveSpeed = 300.f; // 1/2로 상하 이동하는 속도, cm/초

    // ObjectEditor.cpp/D01.cpp의 RotateY와 동일.
    Vector3 RotateY(const Vector3& v, float radians)
    {
        float s = std::sin(radians);
        float c = std::cos(radians);
        return Vector3(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
    }

    // ObjectEditor.cpp의 RotateX와 동일.
    Vector3 RotateX(const Vector3& v, float radians)
    {
        float s = std::sin(radians);
        float c = std::cos(radians);
        return Vector3(v.x, c * v.y - s * v.z, s * v.y + c * v.z);
    }
}

void SpriteSetEditor::Load()
{
    DataManager::GetInstance().Load(L"Dungeon01", L"dungeon_01_01.json");
    DataManager::GetInstance().GetDataAs(L"Dungeon01", dungeonData);

    DataManager::GetInstance().Load(L"SpriteTextureMap", L"sprite_texture_map.json");
    DataManager::GetInstance().GetDataAs(L"SpriteTextureMap", spriteMapData);

    spriteSetKeys.reserve(dungeonData.spriteSets.size());
    for (const auto& [key, parts] : dungeonData.spriteSets)
    {
        spriteSetKeys.push_back(std::stoi(key));
    }
    std::sort(spriteSetKeys.begin(), spriteSetKeys.end());

    GetCamera()->SetPosition(Vector3(0.f, kCameraHeight, -kCameraDistance));
    objectPivot = Vector3(0.f, 0.f, kCameraDistance);

    // ObjectEditor::Load와 동일한 방식으로, 오브젝트 기본 정면이 카메라를 향하도록 초기 회전각을 구한다.
    Vector3 toCam = GetCamera()->GetPosition() - objectPivot;
    float rawAngle = std::atan2(toCam.x, toCam.z);
    groupAngle = std::round(rawAngle / kHalfPi) * kHalfPi;

    // 화면 우측 상단에 현재 spriteSetIndex를 표시하기 위한 DirectWrite 리소스 준비
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(nameDWriteFactory.GetAddressOf()));
    nameDWriteFactory->CreateTextFormat(L"Consolas", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        20.0f, L"en-us", nameTextFormat.GetAddressOf());
    nameTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    graphics->GetDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), nameBrush.GetAddressOf());

    LoadSpriteSetAt(0);
}

void SpriteSetEditor::UnLoad()
{
}

void SpriteSetEditor::LoadSpriteSetAt(int listIndex)
{
    currentParts.clear();
    if (spriteSetKeys.empty()) return;

    int count = static_cast<int>(spriteSetKeys.size());
    currentListIndex = ((listIndex % count) + count) % count;

    int spriteSetIndex = spriteSetKeys[currentListIndex];
    const std::vector<SpriteSetPart>* setParts = dungeonData.FindSpriteSet(spriteSetIndex);
    if (!setParts) return;

    for (const SpriteSetPart& setPart : *setParts)
    {
        const Sprite* sprite = nullptr;
        for (const Sprite& s : spriteMapData.sprites)
        {
            if (s.name == setPart.name) { sprite = &s; break; }
        }
        if (!sprite) continue;

        // D01::PlaceCell과 동일한 규칙(World = RotY(cellRot*90) * RotY(blockRot*90) * Translate(local)).
        // 여기서는 칸(cell) 자체가 없으므로 cellRot=0으로 두고 blockRot만 적용한다.
        float blockRotation = setPart.blockRot * kHalfPi;

        for (const TextureEntry& tex : sprite->textures)
        {
            const TextureListEntry* texInfo = spriteMapData.FindTexture(tex.textureIndex);
            if (!texInfo || !tex.atlasRectPx.has_value()) continue; // 헬퍼(BoundingBox 등) 스킵

            // ObjectEditor::LoadSpriteAt / D01::GetOrBuildSpriteTemplate과 동일한 기준으로 바닥류/퇴화 파츠를 판정한다.
            float ax = std::abs(tex.size.x), ay = std::abs(tex.size.y), az = std::abs(tex.size.z);
            float minXZ = (ax < az) ? ax : az;
            bool isFloor = (ay <= 0.8f * minXZ);

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

            part.localPosition = RotateY(tex.position, blockRotation);
            part.size = tex.size;
            part.isFloor = isFloor;
            part.extraYRotation = tex.rotationY * static_cast<float>(MathUtil::pi / 180.0) + blockRotation;
            part.extraXRotation = tex.rotationX * static_cast<float>(MathUtil::pi / 180.0);

            currentParts.push_back(std::move(part));
        }
    }
}

void SpriteSetEditor::Render()
{
    // ObjectEditor::Render()와 동일하게, 화면 전체를 덮는 배경이 없으므로 이전 프레임(다른 레벨)의
    // 잔상이 남지 않도록 매 프레임 명시적으로 지운다.
    graphics->ClearScreen(0.f, 0.f, 0.f);

    super::Render();

    D2D1_SIZE_F clientSize = graphics->GetDeviceContext()->GetSize();

    // 우측 상단에 지금 보고 있는 spriteSetIndex 표시
    if (!spriteSetKeys.empty())
    {
        std::wstring wname = L"spriteSet " + std::to_wstring(spriteSetKeys[currentListIndex]);
        D2D1_RECT_F nameRect = D2D1::RectF(clientSize.width - 400.f, 10.f, clientSize.width - 10.f, 40.f);
        graphics->GetDeviceContext()->DrawText(
            wname.c_str(), static_cast<UINT32>(wname.size()), nameTextFormat.Get(), nameRect, nameBrush.Get());
    }

    if (currentParts.empty()) return;

    Matrix4x4 cameraView = GetCamera()->GetViewMatrix();
    Matrix4x4 cameraProj = GetCamera()->GetProjectionMatrix();
    Matrix4x4 viewport = Matrix4x4::Viewport(clientSize.width, clientSize.height);
    Matrix4x4 viewProj = cameraProj * cameraView; // z-버퍼 정렬용

    Matrix4x4 groupRotation = Matrix4x4::Translation(objectPivot) * Matrix4x4::RotationY(groupAngle);

    // ObjectEditor::Render()와 동일한 정렬 규칙: 바닥은 항상 먼저(맨 뒤), 나머지는 네 모서리 중 가장
    // 먼 지점을 기준으로 깊이 정렬한다.
    auto isGroundFloor = [](const RenderPart* p) { return p->isFloor && std::abs(p->localPosition.y) < 1.f; };

    std::vector<const RenderPart*> drawOrder;
    drawOrder.reserve(currentParts.size());
    for (const RenderPart& part : currentParts) drawOrder.push_back(&part);
    std::sort(drawOrder.begin(), drawOrder.end(), [&](const RenderPart* a, const RenderPart* b)
        {
            if (isGroundFloor(a) != isGroundFloor(b)) return isGroundFloor(a); // 바닥이 항상 먼저(맨 뒤)

            auto ndcDepth = [&](const RenderPart* p)
                {
                    float halfX = p->size.x * 0.5f;
                    float halfOther = (p->isFloor ? p->size.z : p->size.y) * 0.5f;
                    float tiltRadians = p->extraXRotation + (p->isFloor ? kHalfPi : 0.f);

                    float best = -FLT_MAX;
                    for (float sx : { -1.f, 1.f })
                    {
                        for (float sy : { -1.f, 1.f })
                        {
                            Vector3 corner(sx * halfX, sy * halfOther, 0.f);
                            corner = RotateX(corner, tiltRadians);
                            corner = RotateY(corner, p->extraYRotation);
                            corner = corner + p->localPosition;

                            Vector3 w = objectPivot + RotateY(corner, groupAngle);
                            float clipZ = viewProj.m[2][0] * w.x + viewProj.m[2][1] * w.y
                                + viewProj.m[2][2] * w.z + viewProj.m[2][3];
                            float clipW = viewProj.m[3][0] * w.x + viewProj.m[3][1] * w.y
                                + viewProj.m[3][2] * w.z + viewProj.m[3][3];
                            float depth = clipW != 0.f ? clipZ / clipW : clipZ;
                            if (depth > best) best = depth;
                        }
                    }
                    return best; // z-버퍼 값(클수록 카메라에서 멀다)
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
        Matrix4x4 extraTilt = Matrix4x4::RotationX(part.extraXRotation);

        Matrix4x4 localModel;
        if (part.isFloor)
        {
            localModel = Matrix4x4::Translation(part.localPosition)
                * extraRotation
                * Matrix4x4::RotationX(kHalfPi)
                * extraTilt
                * Matrix4x4::Scale(Vector3(part.size.x / pixelW, part.size.z / pixelH, 1.f))
                * Matrix4x4::Translation(Vector3(-pixelW / 2.f, -pixelH / 2.f, 0.f));
        }
        else
        {
            localModel = Matrix4x4::Translation(part.localPosition)
                * extraRotation
                * extraTilt
                * Matrix4x4::Scale(Vector3(part.size.x / pixelW, -part.size.y / pixelH, 1.f))
                * Matrix4x4::Translation(Vector3(-pixelW / 2.f, -pixelH / 2.f, 0.f));
        }

        Matrix4x4 model = groupRotation * localModel;
        Matrix4x4 final = viewport * cameraProj * cameraView * model;
        part.sheet->DrawSpriteWarped(final);
    }
}

void SpriteSetEditor::Update(double deltaTime)
{
    super::Update(deltaTime);

    // Q/E로 spriteSets를 순서대로 훑어본다 (ObjectEditor의 sprite 전환과 동일한 조작).
    if (InputManager::GetInstance().GetButtonDown(KeyType::Q))
    {
        LoadSpriteSetAt(currentListIndex - 1);
    }
    if (InputManager::GetInstance().GetButtonDown(KeyType::E))
    {
        LoadSpriteSetAt(currentListIndex + 1);
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
