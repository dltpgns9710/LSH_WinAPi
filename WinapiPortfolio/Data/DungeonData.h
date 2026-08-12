#pragma once

#include <string>
#include <vector>
#include <map>
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
    GridInfo grid;
    std::map<std::string, std::string> chipLegend;       // 키: chipNo를 문자열로 변환한 값
    std::vector<std::string> spritePalette;
    std::map<std::string, std::vector<SpriteSetPart>> spriteSets; // 키: spriteSetIndex를 문자열로 변환한 값
    std::vector<Cell> cells;

    // spriteSetIndex(int)로 spriteSets를 바로 조회하기 위한 헬퍼. 없으면 nullptr.
    const std::vector<SpriteSetPart>* FindSpriteSet(int index) const
    {
        auto found = spriteSets.find(std::to_string(index));
        return found != spriteSets.end() ? &found->second : nullptr;
    }
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(DungeonData, dungeonId, floorIndex, floorLabel, grid, chipLegend, spritePalette, spriteSets, cells)
