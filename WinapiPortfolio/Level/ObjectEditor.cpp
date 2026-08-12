#include "ObjectEditor.h"

#include <cmath>
#include <algorithm>

#include "../D2DFramework/Manager/include/DataManager.h"
#include "../D2DFramework/Manager/include/SpriteSheetManager.h"
#include "../D2DFramework/Manager/include/InputManager.h"
#include "../D2DFramework/Graphics/include/SpriteSheet.h"
#include "../D2DFramework/Camera/include/Camera.h"
#include "../D2DFramework/Math/include/MathUtil.h"

namespace
{
    constexpr float kHalfPi = static_cast<float>(MathUtil::pi / 2.0);
    constexpr float kCameraDistance = 400.f; // 오브젝트 피벗과 카메라 사이 거리(cm)
    constexpr float kCameraHeight = 150.f;
}

void ObjectEditor::Load()
{
    DataManager::GetInstance().Load(L"SpriteTextureMap", L"sprite_texture_map.json");
    DataManager::GetInstance().GetDataAs(L"SpriteTextureMap", spriteMapData);

    GetCamera()->SetPosition(Vector3(0.f, kCameraHeight, -kCameraDistance));
    objectPivot = Vector3(0.f, 0.f, kCameraDistance);

    // 오브젝트 기본 정면(+Z)이 카메라를 향하도록 90도 단위로 스냅 회전각을 구해서 고정한다.
    // (Dungeon.cpp의 TestQuad와 동일한 방식 - 카메라가 움직이지 않으므로 Load() 시 한 번만 계산)
    Vector3 toCam = GetCamera()->GetPosition() - objectPivot;
    float rawAngle = std::atan2(toCam.x, toCam.z);
    groupAngle = std::round(rawAngle / kHalfPi) * kHalfPi;
    groupAngle += kHalfPi;

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

        std::wstring texKey(texInfo->texture.begin(), texInfo->texture.end());
        std::string fileName = texInfo->texturePath.substr(texInfo->texturePath.find_last_of('/') + 1);
        std::wstring wFileName(fileName.begin(), fileName.end());

        SpriteSheetManager::GetInstance().LoadTexture(texKey, wFileName);
        std::shared_ptr<SpriteSheet> fullSheet = SpriteSheetManager::GetInstance().GetTexture(texKey);
        if (!fullSheet) continue;

        const AtlasRect& rect = *tex.atlasRectPx;
        RenderPart part;
        part.sheet = fullSheet->CreateSubRegion(
            static_cast<float>(rect.x), static_cast<float>(rect.y),
            static_cast<float>(rect.x + rect.width), static_cast<float>(rect.y + rect.height));
        if (!part.sheet) continue;

        part.localPosition = tex.position;
        part.size = tex.size;

        float ax = std::abs(tex.size.x), ay = std::abs(tex.size.y), az = std::abs(tex.size.z);
        part.isFloor = (ay <= ax && ay <= az);

        // 나무 기둥(wood)은 축 크기만으로 법선을 추정하면 옆면이 보여서 얇게 보인다. 90도 더 돌려 넓은 면이 보이게 한다.
        part.extraYRotation = (texInfo->texture == "d01_f01_plants_01_wood") ? kHalfPi : 0.f;

        currentParts.push_back(std::move(part));
    }
}

void ObjectEditor::Render()
{
    // Dungeon과 달리 화면 전체를 덮는 배경을 그리지 않으므로, 이전 프레임(다른 레벨)의 잔상이
    // 남지 않도록 매 프레임 명시적으로 지운다.
    graphics->ClearScreen(0.f, 0.f, 0.f);

    super::Render();

    if (currentParts.empty()) return;

    Matrix4x4 cameraView = GetCamera()->GetViewMatrix();
    Matrix4x4 cameraProj = GetCamera()->GetProjectionMatrix();
    D2D1_SIZE_F clientSize = graphics->GetDeviceContext()->GetSize();
    Matrix4x4 viewport = Matrix4x4::Viewport(clientSize.width, clientSize.height);
    Matrix4x4 viewProj = cameraProj * cameraView; // z-버퍼 정렬용 (원근분할까지 적용해서 실제 depth 값을 구함)

    Matrix4x4 groupRotation = Matrix4x4::Translation(objectPivot) * Matrix4x4::RotationY(groupAngle);

    // D2D는 깊이 테스트 없이 그린 순서대로 위에 덮어 그리므로, 카메라로부터 먼 텍스처부터 그려야 한다.
    // FLOOR(법선=Y)는 z-버퍼 값과 무관하게 항상 맨 먼저(=맨 뒤) 그린다.
    std::vector<const RenderPart*> drawOrder;
    drawOrder.reserve(currentParts.size());
    for (const RenderPart& part : currentParts) drawOrder.push_back(&part);
    std::sort(drawOrder.begin(), drawOrder.end(), [&](const RenderPart* a, const RenderPart* b)
        {
            if (a->isFloor != b->isFloor) return a->isFloor; // FLOOR가 항상 먼저(맨 뒤)

            auto ndcDepth = [&](const RenderPart* p)
                {
                    Vector3 w = objectPivot + p->localPosition;
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

        float ax = std::abs(part.size.x);
        float ay = std::abs(part.size.y);
        float az = std::abs(part.size.z);

        Matrix4x4 extraRotation = Matrix4x4::RotationY(part.extraYRotation);

        Matrix4x4 localModel; // 회전군 적용 전, pivot 로컬 공간 기준
        if (ax <= ay && ax <= az)
        {
            // 법선 = X축 (좌우를 바라보는 수직 평면)
            localModel = Matrix4x4::Translation(part.localPosition)
                * extraRotation
                * Matrix4x4::RotationY(kHalfPi)
                * Matrix4x4::Scale(Vector3(part.size.z / pixelW, -part.size.y / pixelH, 1.f))
                * Matrix4x4::Translation(Vector3(-pixelW / 2.f, -pixelH / 2.f, 0.f));
        }
        else if (ay <= ax && ay <= az)
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
            // 법선 = Z축 (정면을 바라보는 수직 평면)
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
}
