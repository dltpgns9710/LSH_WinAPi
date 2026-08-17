#include "D01CellPlacer.h"

#include <cmath>
#include <utility>

#include "D01PartTemplateLibrary.h"
#include "../../Data/DungeonData.h"

namespace
{
    // ObjectEditor.cpp의 RotateY와 동일. 셀 배치 시 스프라이트 파츠의 로컬 좌표를 셀 회전만큼 돌리기 위해 사용.
    Vector3 RotateY(const Vector3& v, float radians)
    {
        float s = std::sin(radians);
        float c = std::cos(radians);
        return Vector3(c * v.x + s * v.z, v.y, -s * v.x + c * v.z);
    }
}

void D01CellPlacer::Init(const DungeonData* newDungeonData, D01PartTemplateLibrary* newTemplateLibrary)
{
    dungeonData = newDungeonData;
    templateLibrary = newTemplateLibrary;
}

void D01CellPlacer::PlaceAllCells()
{
    placedParts.reserve(dungeonData->cells.size() * 4);
    for (const Cell& cell : dungeonData->cells)
    {
        PlaceCell(cell);
    }
}

std::vector<PlacedPart> D01CellPlacer::BuildCellParts(int gridX, int gridY, int spriteSetIndex, int effectiveCellRot)
{
    std::vector<PlacedPart> result;

    const std::vector<SpriteSetPart>* setParts = dungeonData->FindSpriteSet(spriteSetIndex);
    if (!setParts || setParts->empty()) return result;

    float cellSize = dungeonData->grid.cellSize;
    Vector3 cellWorldPos = GridToWorld(gridX, gridY, cellSize);

    for (const SpriteSetPart& setPart : *setParts)
    {
        int quarterTurns = ((effectiveCellRot + setPart.blockRot) % 4 + 4) % 4;
        float totalRotation = quarterTurns * kHalfPi;

        const std::vector<PartTemplate>& templates = templateLibrary->GetOrBuild(setPart.name);
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

void D01CellPlacer::PlaceCell(const Cell& cell)
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

std::vector<int> D01CellPlacer::GetDistinctSlopeDirections(int spriteSetIndex)
{
    std::vector<int> dirs;
    const std::vector<SpriteSetPart>* setParts = dungeonData->FindSpriteSet(spriteSetIndex);
    if (!setParts) return dirs;

    bool seen[4] = { false, false, false, false };
    for (const SpriteSetPart& setPart : *setParts)
    {
        for (const PartTemplate& tmpl : templateLibrary->GetOrBuild(setPart.name))
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

void D01CellPlacer::TryRegisterDynamicWallCell(const Cell& cell, size_t placedIndexBegin, size_t placedCount)
{
    std::vector<int> slopeDirs = GetDistinctSlopeDirections(cell.spriteSetIndex);
    if (slopeDirs.empty()) return; // 경사 자체가 없는 벽은 대상이 아니다

    // 인접 4방향(0=N,1=E,2=S,3=W) 중 실제로 이동 가능한 방향들 - D01CellPlacer::IsWalkable과 동일 기준.
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

void D01CellPlacer::RefreshFacing(int cameraGridX, int cameraGridY)
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

bool D01CellPlacer::IsWalkable(int gridX, int gridY) const
{
    if (gridX < 0 || gridX >= dungeonData->grid.width) return false;
    if (gridY < 0 || gridY >= dungeonData->grid.height) return false;

    int index = gridY * dungeonData->grid.width + gridX; // Cell::index와 동일한 규칙 (y*width+x)
    if (index < 0 || static_cast<size_t>(index) >= dungeonData->cells.size()) return false;

    const std::string& chipName = dungeonData->cells[index].chipName;
    // 보물칸/계단칸도 걸어 들어갈 수 있는 칸으로 취급
    return chipName == "FLOOR" || chipName == "TREASURE" || chipName == "UP_STAIR" || chipName == "DOWN_STAIR";
}
