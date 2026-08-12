#pragma once

#include <string>
#include <vector>
#include <optional>

#include "../D2DFramework/Lib/include/json.hpp"
#include "../D2DFramework/Math/include/Vector3.h"

// sprite_texture_map.json (Resources/Data) 스키마와 1:1로 대응하는 구조체들.
// DataManager::GetDataAs<SpriteTextureMapData>()로 불러와 쓴다.
// texturelist/textureIndex 참조 스키마 기준 (texture/texturePath 문자열은 더 이상 textures[]에 직접 없음).

// Vector3는 Math 모듈 소유라 json 의존성을 넣지 않는다. 대신 여기서 로컬로 from_json을 붙인다.
inline void from_json(const nlohmann::json& j, Vector3& v)
{
    v.x = j.at("x").get<float>();
    v.y = j.at("y").get<float>();
    v.z = j.at("z").get<float>();
}

struct AtlasRect
{
    int x;
    int y;
    int width;
    int height;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AtlasRect, x, y, width, height)

struct TextureListEntry
{
    int index;
    std::string texture;
    std::string texturePath;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TextureListEntry, index, texture, texturePath)

struct TextureEntry
{
    std::optional<int> textureIndex; // texturelist 인덱스. null이면 화면에 안 그리는 헬퍼(BoundingBox 등)
    int textureWidth;
    int textureHeight;
    bool tiled;
    int repeatX = 1; // tiled:true일 때만 JSON에 존재
    int repeatY = 1;
    std::optional<AtlasRect> atlasRectPx; // textureIndex가 null이면 이것도 null
    Vector3 position;
    Vector3 size;
    float rotationY = 0.f; // 이 파츠를 로컬 Y축으로 추가 회전시키는 각도(도). 기본 0. JSON에 없으면 0.
};

inline void from_json(const nlohmann::json& j, TextureEntry& tex)
{
    tex.textureIndex.reset();
    if (!j.at("textureIndex").is_null()) tex.textureIndex = j.at("textureIndex").get<int>();

    j.at("textureWidth").get_to(tex.textureWidth);
    j.at("textureHeight").get_to(tex.textureHeight);
    j.at("tiled").get_to(tex.tiled);

    tex.repeatX = j.contains("repeatX") ? j.at("repeatX").get<int>() : 1;
    tex.repeatY = j.contains("repeatY") ? j.at("repeatY").get<int>() : 1;

    tex.atlasRectPx.reset();
    if (!j.at("atlasRectPx").is_null()) tex.atlasRectPx = j.at("atlasRectPx").get<AtlasRect>();

    j.at("position").get_to(tex.position);
    j.at("size").get_to(tex.size);

    tex.rotationY = j.contains("rotationY") ? j.at("rotationY").get<float>() : 0.f;
}

struct Sprite
{
    std::string name;
    Vector3 position;
    std::vector<TextureEntry> textures;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Sprite, name, position, textures)

struct SpriteTextureMapData
{
    std::vector<TextureListEntry> texturelist;
    std::vector<Sprite> sprites;

    // TextureEntry::textureIndex(nullopt 가능)로 texturelist를 바로 조회하기 위한 헬퍼. 없으면 nullptr.
    const TextureListEntry* FindTexture(std::optional<int> index) const
    {
        if (!index.has_value() || *index < 0 || static_cast<size_t>(*index) >= texturelist.size()) return nullptr;
        return &texturelist[*index];
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SpriteTextureMapData, texturelist, sprites)
