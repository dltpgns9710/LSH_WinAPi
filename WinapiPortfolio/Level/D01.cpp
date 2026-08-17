#include "D01.h"

#include <cmath>
#include <cfloat>
#include <algorithm>

#include "../D2DFramework/Manager/include/DataManager.h"
#include "../D2DFramework/Manager/include/SpriteSheetManager.h"
#include "../D2DFramework/Manager/include/InputManager.h"
#include "../D2DFramework/Graphics/include/SpriteSheet.h"
#include "../D2DFramework/Graphics/include/SpriteAtlas.h"
#include "../D2DFramework/Camera/include/Camera.h"
#include "../D2DFramework/Math/include/MathUtil.h"

#include <dwrite.h>
#pragma comment(lib, "dwrite.lib")

namespace
{
    constexpr float kHalfPi = static_cast<float>(MathUtil::pi / 2.0);
    constexpr float kCameraHeight = 150.f; // 오브젝트 피벗과 카메라 사이 눈높이(cm), ObjectEditor와 동일값

    // 던전용 카메라 시야 거리(cm). Camera의 기본값(20, 2000)은 2000/400 = 5칸이라 복도 끝이 잘렸다.
    // Camera::isRenderPosition은 farZ * 1.2 를 컷 기준으로 쓰므로 실제로 보이는 거리는 이 값의 1.2배다.
    constexpr float kCameraNearZ = 20.f;
    constexpr float kCameraFarZ = 2000.f; // 5칸 (컬링 한계는 2400cm = 6칸)

    // ObjectEditor.cpp의 RotateY와 동일. 셀 배치 시 스프라이트 파츠의 로컬 좌표를 셀 회전만큼 돌리기 위해 사용.
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

    // 그리드(gridX,gridY) -> 월드 좌표. z는 grid y와 반대로 둬야 카메라 기본 방향(+Z, facingQuarter=0)이
    // 시작 위치(6,19)에서 계단(6,20)을 등지는 방향(그리드 y가 작아지는 쪽)과 일치한다.
    Vector3 GridToWorld(int gridX, int gridY, float cellSize)
    {
        return Vector3(gridX * cellSize, 0.f, -(gridY * cellSize));
    }
}

void D01::Load()
{
    SpriteSheetManager::GetInstance().LoadTexture(L"Background", L"Background.png");
    background = std::make_shared<SpriteAtlas>(SpriteSheetManager::GetInstance().GetTexture(L"Background"));
    background->SetDrawRegion(0, 250, background->GetClientWidthSize(), background->GetClientHeightSize());

    DataManager::GetInstance().Load(L"Dungeon01", L"dungeon_01_01.json");
    DataManager::GetInstance().GetDataAs(L"Dungeon01", dungeonData);

    DataManager::GetInstance().Load(L"SpriteTextureMap", L"sprite_texture_map.json");
    DataManager::GetInstance().GetDataAs(L"SpriteTextureMap", spriteMapData);

    placedParts.reserve(dungeonData.cells.size() * 4);
    for (const Cell& cell : dungeonData.cells)
    {
        PlaceCell(cell);
    }

    // 시작 위치 기준으로 얇은 벽들의 경사 방향을 첫 프레임부터 맞춰둔다.
    RefreshDynamicWallFacing(dungeonData.startPosition.x, dungeonData.startPosition.y);

    float cellSize = dungeonData.grid.cellSize;

    // Camera 기본 farZ(2000cm)는 셀 5칸 거리라, isRenderPosition의 컷 기준(farZ * 1.2 = 6칸)을 넘는
    // 복도 안쪽이 통째로 그려지지 않아 화면 중앙이 뻥 뚫린 것처럼 보였다. 던전은 시야가 길게 뻗는
    // 복도가 많으므로 D01에서만 늘려 잡는다(전역 기본값을 바꾸면 Dungeon/ObjectEditor까지 영향).
    GetCamera()->SetNearFar(kCameraNearZ, kCameraFarZ);

    Vector3 startWorldPos = GridToWorld(dungeonData.startPosition.x, dungeonData.startPosition.y, cellSize);
    startWorldPos.y = kCameraHeight;
    GetCamera()->SetPosition(startWorldPos);

    // 화면 우측 상단에 던전 이름을 표시하기 위한 DirectWrite 리소스 준비
    DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(nameDWriteFactory.GetAddressOf()));
    nameDWriteFactory->CreateTextFormat(L"Consolas", nullptr,
        DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
        20.0f, L"en-us", nameTextFormat.GetAddressOf());
    nameTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_TRAILING);
    graphics->GetDeviceContext()->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), nameBrush.GetAddressOf());
}

void D01::UnLoad()
{
}

const std::vector<D01::PartTemplate>& D01::GetOrBuildSpriteTemplate(const std::string& spriteName)
{
    auto found = spriteTemplateCache.find(spriteName);
    if (found != spriteTemplateCache.end()) return found->second;

    std::vector<PartTemplate> parts;
    float cellSize = dungeonData.grid.cellSize;

    const Sprite* sprite = nullptr;
    for (const Sprite& s : spriteMapData.sprites)
    {
        if (s.name == spriteName) { sprite = &s; break; }
    }

    if (sprite)
    {
        parts.reserve(sprite->textures.size());
        for (const TextureEntry& tex : sprite->textures)
        {
            const TextureListEntry* texInfo = spriteMapData.FindTexture(tex.textureIndex);
            if (!texInfo || !tex.atlasRectPx.has_value()) continue; // 헬퍼(BoundingBox 등) 스킵

            // ObjectEditor::LoadSpriteAt과 동일한 기준으로 바닥류/퇴화 파츠를 판정한다.
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
            PartTemplate part;

            // 서로 다른 스프라이트 이름이 같은 (파일+atlasRect+tiled 설정) 조합을 참조하는 경우
            // (예: *_Unique 변형들이 같은 코너 텍스처를 공유) CreateSubRegion/CreateTiledRegion을
            // 다시 호출하지 않도록, 크롭 결과 자체를 캐시 키로 재사용한다.
            std::string cropKey = fileName + ':' + std::to_string(rect.x) + ',' + std::to_string(rect.y)
                + ',' + std::to_string(rect.width) + ',' + std::to_string(rect.height)
                + (tex.tiled ? (":T" + std::to_string(tex.repeatX) + 'x' + std::to_string(tex.repeatY)) : ":S");

            auto cachedCrop = croppedSheetCache.find(cropKey);
            if (cachedCrop != croppedSheetCache.end())
            {
                part.sheet = cachedCrop->second;
            }
            else
            {
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
                croppedSheetCache.emplace(std::move(cropKey), part.sheet);
            }

            part.localPosition = tex.position;
            part.size = tex.size;
            part.isFloor = isFloor;
            part.extraYRotation = tex.rotationY * static_cast<float>(MathUtil::pi / 180.0);
            part.extraXRotation = tex.rotationX * static_cast<float>(MathUtil::pi / 180.0);

            // 하이브리드: JSON에 layer가 명시돼 있으면 그대로 쓰고, 없으면 크기로 자동 분류한다
            // (셀보다 넓은 오버사이즈 배경/받침 패널=0, 나머지 디테일=1) - tile_layer_sort_design.md 참고.
            if (tex.layer.has_value())
            {
                part.layer = *tex.layer;
            }
            else
            {
                bool oversized = (ax > cellSize) || (az > cellSize);
                part.layer = oversized ? 0 : 1;
            }

            parts.push_back(std::move(part));
        }
    }

    auto inserted = spriteTemplateCache.emplace(spriteName, std::move(parts));
    return inserted.first->second;
}

std::vector<D01::PlacedPart> D01::BuildCellParts(int gridX, int gridY, int spriteSetIndex, int effectiveCellRot)
{
    std::vector<PlacedPart> result;

    const std::vector<SpriteSetPart>* setParts = dungeonData.FindSpriteSet(spriteSetIndex);
    if (!setParts || setParts->empty()) return result;

    float cellSize = dungeonData.grid.cellSize;
    Vector3 cellWorldPos = GridToWorld(gridX, gridY, cellSize);

    for (const SpriteSetPart& setPart : *setParts)
    {
        int quarterTurns = ((effectiveCellRot + setPart.blockRot) % 4 + 4) % 4;
        float totalRotation = quarterTurns * kHalfPi;

        const std::vector<PartTemplate>& templates = GetOrBuildSpriteTemplate(setPart.name);
        for (const PartTemplate& tmpl : templates)
        {
            if (!tmpl.sheet) continue;

            PlacedPart placed;
            placed.sheet = tmpl.sheet;
            placed.worldPosition = cellWorldPos + RotateY(tmpl.localPosition, totalRotation);
            placed.size = tmpl.size;
            placed.isFloor = tmpl.isFloor;
            placed.worldYRotation = tmpl.extraYRotation + totalRotation;
            placed.extraXRotation = tmpl.extraXRotation;
            placed.layer = tmpl.layer;
            placed.cellCenterWorldPos = cellWorldPos;
            result.push_back(std::move(placed));
        }
    }
    return result;
}

void D01::PlaceCell(const Cell& cell)
{
    size_t begin = placedParts.size();
    for (PlacedPart& part : BuildCellParts(cell.x, cell.y, cell.spriteSetIndex, cell.cellRot))
    {
        placedParts.push_back(std::move(part));
    }

    if (cell.chipName == "WALL")
    {
        TryRegisterDynamicWallCell(cell, begin, placedParts.size() - begin);
    }
}

std::vector<int> D01::GetDistinctSlopeDirections(int spriteSetIndex)
{
    std::vector<int> dirs;
    const std::vector<SpriteSetPart>* setParts = dungeonData.FindSpriteSet(spriteSetIndex);
    if (!setParts) return dirs;

    bool seen[4] = { false, false, false, false };
    for (const SpriteSetPart& setPart : *setParts)
    {
        for (const PartTemplate& tmpl : GetOrBuildSpriteTemplate(setPart.name))
        {
            if (tmpl.extraXRotation == 0.f) continue; // 경사(rotationX) 없는 파츠는 무시

            // rotationY(0/90/180/270도)를 0=N,1=E,2=S,3=W로 변환. rotationX가 음수면 같은 rotationY라도
            // 정반대(180도)를 향한다 - 경사 방향 판정 로직에서 검증한 규칙과 동일.
            int base = (static_cast<int>(std::lround(tmpl.extraYRotation / kHalfPi)) % 4 + 4) % 4;
            if (tmpl.extraXRotation < 0.f) base = (base + 2) % 4;

            int dir = (base + setPart.blockRot) % 4;
            if (!seen[dir]) { seen[dir] = true; dirs.push_back(dir); }
        }
    }
    return dirs;
}

void D01::TryRegisterDynamicWallCell(const Cell& cell, size_t placedIndexBegin, size_t placedCount)
{
    std::vector<int> slopeDirs = GetDistinctSlopeDirections(cell.spriteSetIndex);
    if (slopeDirs.empty()) return; // 경사 자체가 없는 벽은 대상이 아니다

    // 인접 4방향(0=N,1=E,2=S,3=W) 중 실제로 이동 가능한 방향들 - D01::IsWalkable과 동일 기준.
    bool walkable[4] =
    {
        IsWalkable(cell.x, cell.y - 1),
        IsWalkable(cell.x + 1, cell.y),
        IsWalkable(cell.x, cell.y + 1),
        IsWalkable(cell.x - 1, cell.y),
    };
    int walkableCount = (walkable[0] ? 1 : 0) + (walkable[1] ? 1 : 0) + (walkable[2] ? 1 : 0) + (walkable[3] ? 1 : 0);
    if (walkableCount <= static_cast<int>(slopeDirs.size())) return; // 경사가 이미 충분하면 대상이 아니다

    // 실측 결과 이 던전에서 발생하는 모든 케이스(94곳)가 "경사 방향 1개 + 이동 가능 칸이 정확히
    // N/S 또는 E/W 반대쌍"인 형태였다. 그 외(코너형, 3방향 이상 등)는 정적 우선순위 규칙이 이미
    // 최선의 방향을 골라뒀으므로 여기서는 건드리지 않는다.
    if (slopeDirs.size() != 1) return;
    bool isNS = walkable[0] && walkable[2] && !walkable[1] && !walkable[3];
    bool isEW = walkable[1] && walkable[3] && !walkable[0] && !walkable[2];
    if (!isNS && !isEW) return;

    int currentDir = (slopeDirs[0] + cell.cellRot) % 4; // 지금 authored 상태가 실제로 향하는 방향
    int altCellRot = (cell.cellRot + 2) % 4;             // 정반대(180도)를 향하게 하는 cellRot

    DynamicWallCell dw;
    dw.x = cell.x;
    dw.y = cell.y;
    dw.spriteSetIndex = cell.spriteSetIndex;
    dw.isNorthSouth = isNS;
    dw.currentCellRot = cell.cellRot;
    dw.placedIndexBegin = placedIndexBegin;
    dw.placedCount = placedCount;

    if (isNS)
    {
        dw.cellRotForNegativeSide = (currentDir == 0) ? cell.cellRot : altCellRot; // North(y가 작은 쪽)
        dw.cellRotForPositiveSide = (currentDir == 0) ? altCellRot : cell.cellRot; // South(y가 큰 쪽)
    }
    else
    {
        dw.cellRotForNegativeSide = (currentDir == 3) ? cell.cellRot : altCellRot; // West(x가 작은 쪽)
        dw.cellRotForPositiveSide = (currentDir == 3) ? altCellRot : cell.cellRot; // East(x가 큰 쪽)
    }

    dynamicWallCells.push_back(dw);
}

void D01::RefreshDynamicWallFacing(int cameraGridX, int cameraGridY)
{
    for (DynamicWallCell& dw : dynamicWallCells)
    {
        int desiredCellRot = dw.isNorthSouth
            ? (cameraGridY < dw.y ? dw.cellRotForNegativeSide : dw.cellRotForPositiveSide)
            : (cameraGridX < dw.x ? dw.cellRotForNegativeSide : dw.cellRotForPositiveSide);

        if (desiredCellRot == dw.currentCellRot) continue; // 이미 원하는 방향이면 다시 계산하지 않는다
        dw.currentCellRot = desiredCellRot;

        std::vector<PlacedPart> parts = BuildCellParts(dw.x, dw.y, dw.spriteSetIndex, desiredCellRot);
        for (size_t k = 0; k < parts.size() && dw.placedIndexBegin + k < placedParts.size(); ++k)
        {
            placedParts[dw.placedIndexBegin + k] = std::move(parts[k]);
        }
    }
}

void D01::Render()
{
    // Dungeon::Render()와 동일하게, 화면 전체를 덮는 배경을 그려서 이전 프레임(다른 레벨)의
    // 잔상이 남지 않게 한다. 파츠가 그려지지 않는 픽셀(잎/잔디 텍스처의 투명한 부분, 캐노피 위쪽,
    // 시야 밖으로 컬링된 원거리)을 전부 이 배경이 메워준다 - 없으면 그 자리가 그대로 검게 보인다.
    background->DrawSpriteAtlas(0, 0, background->GetClientWidthSize(), background->GetClientHeightSize());

    super::Render();

    D2D1_SIZE_F clientSize = graphics->GetDeviceContext()->GetSize();

    // 우측 상단에 던전 id 표시
    std::wstring wname(dungeonData.dungeonId.begin(), dungeonData.dungeonId.end());
    D2D1_RECT_F nameRect = D2D1::RectF(clientSize.width - 400.f, 10.f, clientSize.width - 10.f, 40.f);
    graphics->GetDeviceContext()->DrawText(
        wname.c_str(), static_cast<UINT32>(wname.size()), nameTextFormat.Get(), nameRect, nameBrush.Get());

    if (placedParts.empty()) return;

    Matrix4x4 cameraView = GetCamera()->GetViewMatrix();
    Matrix4x4 cameraProj = GetCamera()->GetProjectionMatrix();
    Matrix4x4 viewport = Matrix4x4::Viewport(clientSize.width, clientSize.height);
    Matrix4x4 viewProj = cameraProj * cameraView; // z-버퍼 정렬용
    // 파츠와 무관한 프레임 상수라 루프 밖에서 한 번만 계산해두고 재사용한다 - 안 그러면 그리기
    // 루프에서 viewProj를 재사용하지 못하고 파츠마다 cameraProj * cameraView를 다시 계산하게 된다.
    Matrix4x4 viewportViewProj = viewport * viewProj;

    // 파츠 하나당 실제로 그려지는 사각형의 네 모서리(월드 좌표). 컬링(프러스텀 판정)과 깊이 정렬
    // 둘 다 이 모서리가 필요해서, 파츠마다 한 번만 계산해 재사용한다 - 예전에는 정렬 비교자 안에서
    // 매 비교마다(O(n log n)회) 다시 계산해 같은 파츠를 몇 번이고 반복 계산하고 있었다.
    struct VisiblePart
    {
        const PlacedPart* part;
        float depth; // z-버퍼 값(클수록 카메라에서 멀다), 네 모서리 중 가장 먼 지점 기준. 같은 셀·같은 layer일 때만 타이브레이크로 씀.
        float cellDepth; // 이 파츠가 속한 셀 중심의 view-space Z. 코너가 아니라 점 하나라 보는 방향이 바뀌어도 뒤집히지 않는다.
    };

    auto computeCorners = [](const PlacedPart& p, Vector3 outCorners[4])
        {
            float halfX = p.size.x * 0.5f;
            float halfOther = (p.isFloor ? p.size.z : p.size.y) * 0.5f;
            float tiltRadians = p.extraXRotation + (p.isFloor ? kHalfPi : 0.f);

            // 코너 4개 모두 같은 tiltRadians·worldYRotation을 쓰므로, RotateX/RotateY를 코너마다
            // 호출해 sin/cos을 4번씩 중복 계산하는 대신 한 번만 구해서 회전식을 직접 전개해 적용한다.
            float sinTilt = std::sin(tiltRadians);
            float cosTilt = std::cos(tiltRadians);
            float sinYaw = std::sin(p.worldYRotation);
            float cosYaw = std::cos(p.worldYRotation);

            int i = 0;
            for (float sx : { -1.f, 1.f })
            {
                for (float sy : { -1.f, 1.f })
                {
                    float x0 = sx * halfX;
                    float y0 = sy * halfOther;
                    // z0 = 0이므로 RotateX(x0, y0, 0) = (x0, cosTilt*y0, sinTilt*y0)까지만 남는다.
                    float rx = cosYaw * x0 + sinYaw * (sinTilt * y0);
                    float ry = cosTilt * y0;
                    float rz = -sinYaw * x0 + cosYaw * (sinTilt * y0);
                    outCorners[i++] = p.worldPosition + Vector3(rx, ry, rz);
                }
            }
        };

    // 카메라 시야에 들어올 만한 파츠만 골라 그린다 (던전 전체를 매 프레임 그리기엔 파츠 수가 많다).
    // Camera::isRenderTile과 동일한 방식 - 중심점만 보던 이전의 사각 박스 컬링 대신, 파츠의 네 모서리
    // 중 하나라도 카메라 프러스텀(near/far/up/down 평면) 안이면 보이는 것으로 취급한다. 중심점이
    // 시야 밖이어도 큰 파츠의 모서리가 화면에 걸쳐 있으면 더 이상 잘못 컬링되지 않는다.
    std::vector<VisiblePart> drawOrder;
    drawOrder.reserve(placedParts.size());
    for (const PlacedPart& part : placedParts)
    {
        if (!part.sheet) continue;
        if (!GetCamera()->isRenderPosition(part.worldPosition)) continue; // 값싼 사전 필터

        Vector3 corners[4];
        computeCorners(part, corners);

        bool visible = false;
        float depth = -FLT_MAX;
        for (const Vector3& w : corners)
        {
            if (!visible && GetCamera()->isRenderPoint(w)) visible = true;

            float clipZ = viewProj.m[2][0] * w.x + viewProj.m[2][1] * w.y
                + viewProj.m[2][2] * w.z + viewProj.m[2][3];
            float clipW = viewProj.m[3][0] * w.x + viewProj.m[3][1] * w.y
                + viewProj.m[3][2] * w.z + viewProj.m[3][3];
            float d = clipW != 0.f ? clipZ / clipW : clipZ;
            if (d > depth) depth = d;
        }
        if (!visible) continue;

        // 셀 중심(y=0, 파츠 회전과 무관하게 고정)의 view-space Z. 파츠 자신의 코너 대신 이 점을 1차
        // 정렬 기준으로 쓰면, 셀보다 넓은 오버사이즈 배경 패널이 회전으로 인해 인접 셀 깊이 범위를
        // 침범해도(코너 기반 depth가 방향에 따라 뒤집히던 원인) "어느 셀 소속인가"는 안 바뀌므로
        // 정렬 순서가 보는 방향에 따라 뒤집히지 않는다 - tile_layer_sort_design.md 참고.
        const Vector3& c = part.cellCenterWorldPos;
        float cellDepth = cameraView.m[2][0] * c.x + cameraView.m[2][1] * c.y
            + cameraView.m[2][2] * c.z + cameraView.m[2][3];

        drawOrder.push_back({ &part, depth, cellDepth });
    }

    // D2D는 깊이 테스트 없이 그린 순서대로 위에 덮어 그리므로, 카메라로부터 먼 텍스처부터 그려야 한다.
    // 진짜 바닥(y≈0, 법선=Y)은 z-버퍼 값과 무관하게 항상 맨 먼저(=맨 뒤) 그린다 - ObjectEditor::Render 참고.
    auto isGroundFloor = [](const PlacedPart* p) { return p->isFloor && std::abs(p->worldPosition.y) < 1.f; };

    std::sort(drawOrder.begin(), drawOrder.end(), [&](const VisiblePart& a, const VisiblePart& b)
        {
            if (isGroundFloor(a.part) != isGroundFloor(b.part)) return isGroundFloor(a.part);
            if (a.cellDepth != b.cellDepth) return a.cellDepth > b.cellDepth; // 먼 셀부터(내림차순)
            // 같은 셀 안에서는 지오메트리 깊이 대신 데이터로 명시된 layer를 따른다(작을수록 먼저=배경).
            if (a.part->layer != b.part->layer) return a.part->layer < b.part->layer;
            return a.depth > b.depth; // 같은 셀·같은 layer일 때만 코너-depth로 타이브레이크
        });

    for (const VisiblePart& visiblePart : drawOrder)
    {
        const PlacedPart& part = *visiblePart.part;

        float pixelW = part.sheet->GetImageWidth();
        float pixelH = part.sheet->GetImageHeight();
        if (pixelW <= 0.f || pixelH <= 0.f) continue;

        Matrix4x4 extraRotation = Matrix4x4::RotationY(part.worldYRotation);
        // 계단처럼 평면 자체를 기울여야 하는 파츠용 추가 회전(pitch) - ObjectEditor::Render의 extraTilt와 동일.
        Matrix4x4 extraTilt = Matrix4x4::RotationX(part.extraXRotation);

        Matrix4x4 model;
        if (part.isFloor)
        {
            model = Matrix4x4::Translation(part.worldPosition)
                * extraRotation
                * Matrix4x4::RotationX(kHalfPi)
                * extraTilt
                * Matrix4x4::Scale(Vector3(part.size.x / pixelW, part.size.z / pixelH, 1.f))
                * Matrix4x4::Translation(Vector3(-pixelW / 2.f, -pixelH / 2.f, 0.f));
        }
        else
        {
            model = Matrix4x4::Translation(part.worldPosition)
                * extraRotation
                * extraTilt
                * Matrix4x4::Scale(Vector3(part.size.x / pixelW, -part.size.y / pixelH, 1.f))
                * Matrix4x4::Translation(Vector3(-pixelW / 2.f, -pixelH / 2.f, 0.f));
        }

        Matrix4x4 final = viewportViewProj * model;
        part.sheet->DrawSpriteWarped(final);
    }
    
    // 던전 이름 아래에 카메라 위치/이동 방향 디버그 정보 표시. forward는 W, left/right는 Q/E(스트레이프)로
    // 이동했을 때 도착할 좌표이고, 벽이라도 그대로 계산해서 보여준다(IsWalkable 판정을 거치지 않음).
    // Vector3는 (x, y=높이, z)이고 이 던전의 이동 평면은 x/z이므로, 화면 표기는 x와 z를 각각 x, y로 매핑한다.
    // 월드 좌표(cm)를 그대로 보면 읽기 어려워, cellSize로 나눠 그리드 인덱스로 보여준다. z는 GridToWorld의
    // 부호 반전(z = -(gridY*cellSize))과 짝을 맞춰 -z/cellSize로 나눠야 실제 그리드 y와 일치한다.
    {
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
    }
}

bool D01::IsWalkable(int gridX, int gridY) const
{
    if (gridX < 0 || gridX >= dungeonData.grid.width) return false;
    if (gridY < 0 || gridY >= dungeonData.grid.height) return false;

    int index = gridY * dungeonData.grid.width + gridX; // Cell::index와 동일한 규칙 (y*width+x)
    if (index < 0 || static_cast<size_t>(index) >= dungeonData.cells.size()) return false;

    const std::string& chipName = dungeonData.cells[index].chipName;
    // 보물칸/계단칸도 걸어 들어갈 수 있는 칸으로 취급
    return chipName == "FLOOR" || chipName == "TREASURE" || chipName == "UP_STAIR" || chipName == "DOWN_STAIR";
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
            if (!IsWalkable(gridX, gridY)) return;
            GetCamera()->moveRequest(dir);
            RefreshDynamicWallFacing(gridX, gridY); // 카메라가 도착할 칸 기준으로 얇은 벽의 경사 방향을 갱신
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
