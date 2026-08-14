#pragma once

#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <optional>

#include "../D2DFramework/Lib/include/json.hpp"

// dungeon_01_01.json (Resources/Data) 스키마와 1:1로 대응하는 구조체들.
// DataManager::GetDataAs<DungeonData>()로 불러와 쓴다.

struct GridInfo
{
    int width;
    int height;
    float cellSize;
    std::string cellSizeUnit;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GridInfo, width, height, cellSize, cellSizeUnit)

struct GridPosition
{
    int x;
    int y;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(GridPosition, x, y)

struct SpriteSetPart
{
    std::string name;
    int blockRot;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SpriteSetPart, name, blockRot)

struct StairInfo
{
    int goFloor;
    int goPosX;
    int goPosY;
    int rot;
    int enterDir;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(StairInfo, goFloor, goPosX, goPosY, rot, enterDir)

struct TreasureInfo
{
    int tbId;
    int tbType;
    int itemIdOrSum;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(TreasureInfo, tbId, tbType, itemIdOrSum)

struct Cell
{
    int index;
    int x;
    int y;
    int chipNo;
    std::string chipName;
    int cellRot;
    int spriteSetIndex;
    std::optional<StairInfo> stair;     // DOWN_STAIR 칸에만 존재
    std::optional<TreasureInfo> treasure; // TREASURE 칸에만 존재
};

inline void from_json(const nlohmann::json& j, Cell& cell)
{
    j.at("index").get_to(cell.index);
    j.at("x").get_to(cell.x);
    j.at("y").get_to(cell.y);
    j.at("chipNo").get_to(cell.chipNo);
    j.at("chipName").get_to(cell.chipName);
    j.at("cellRot").get_to(cell.cellRot);
    j.at("spriteSetIndex").get_to(cell.spriteSetIndex);

    cell.stair.reset();
    if (j.contains("stair")) cell.stair = j.at("stair").get<StairInfo>();

    cell.treasure.reset();
    if (j.contains("treasure")) cell.treasure = j.at("treasure").get<TreasureInfo>();
}

struct DungeonData
{
    std::string dungeonId;
    int floorIndex;
    std::string floorLabel;
    GridPosition startPosition;
    GridInfo grid;
    std::map<std::string, std::string> chipLegend;       // 키: chipNo를 문자열로 변환한 값. 조회 없이 그대로 보관만 하므로 map 유지.
    std::vector<std::string> spritePalette;

    // 키: spriteSetIndex(int). PlaceCell에서 셀마다 조회하므로 문자열 변환 없는 unordered_map으로 둔다.
    std::unordered_map<int, std::vector<SpriteSetPart>> spriteSets;
    std::vector<Cell> cells;

    // spriteSetIndex(int)로 spriteSets를 바로 조회하기 위한 헬퍼. 없으면 nullptr.
    const std::vector<SpriteSetPart>* FindSpriteSet(int index) const
    {
        auto found = spriteSets.find(index);
        return found != spriteSets.end() ? &found->second : nullptr;
    }
};

// spriteSets는 키가 int라 NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE의 기본 map 변환(키를 std::string으로만
// 구성 가능해야 함)을 못 쓴다. JSON 객체 키("0","1",...)를 직접 int로 파싱해 넣어준다.
inline void from_json(const nlohmann::json& j, DungeonData& d)
{
    j.at("dungeonId").get_to(d.dungeonId);
    j.at("floorIndex").get_to(d.floorIndex);
    j.at("floorLabel").get_to(d.floorLabel);
    j.at("startPosition").get_to(d.startPosition);
    j.at("grid").get_to(d.grid);
    j.at("chipLegend").get_to(d.chipLegend);
    j.at("spritePalette").get_to(d.spritePalette);

    d.spriteSets.clear();
    for (const auto& [key, value] : j.at("spriteSets").items())
    {
        d.spriteSets.emplace(std::stoi(key), value.get<std::vector<SpriteSetPart>>());
    }

    j.at("cells").get_to(d.cells);
}
