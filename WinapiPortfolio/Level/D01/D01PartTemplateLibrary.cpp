#include "D01PartTemplateLibrary.h"

#include <cmath>
#include <utility>

#include "../../D2DFramework/Manager/include/SpriteSheetManager.h"
#include "../../D2DFramework/Graphics/include/SpriteSheet.h"
#include "../../D2DFramework/Math/include/MathUtil.h"

void D01PartTemplateLibrary::Init(SpriteTextureMapData&& newSpriteMapData, float newCellSize)
{
    spriteMapData = std::move(newSpriteMapData);
    cellSize = newCellSize;
}

const std::vector<PartTemplate>& D01PartTemplateLibrary::GetOrBuild(const std::string& spriteName)
{
    auto found = spriteTemplateCache.find(spriteName);
    if (found != spriteTemplateCache.end()) return found->second;

    std::vector<PartTemplate> parts;

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
