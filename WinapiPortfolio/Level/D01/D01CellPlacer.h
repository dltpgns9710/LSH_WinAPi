#pragma once

#include <vector>

#include "D01PartTypes.h"

struct DungeonData;
struct Cell;
class D01PartTemplateLibrary;

// 던전 셀(dungeon json) 하나하나를 실제로 그릴 PlacedPart 목록으로 배치하고, 카메라 위치에 따라
// 방향이 바뀌는 얇은 벽(DynamicWallCell)까지 관리한다. 배치(셀->PlacedPart)와 동적 벽은 같은
// placedParts 벡터를 함께 다루는 강결합 로직이라 한 클래스에 묶여 있다.
class D01CellPlacer
{
public:
    void Init(const DungeonData* dungeonData, D01PartTemplateLibrary* templateLibrary);

    // dungeonData.cells 전체를 순회해 placedParts를 채운다. Load()에서 한 번만 호출.
    void PlaceAllCells();

    // 카메라가 실제로 도착할 그리드 칸(cameraGridX,Y) 기준으로 동적 벽들의 방향을 다시 계산해
    // placedParts를 그 자리에서 덮어쓴다. Load() 끝(시작 위치 기준)과 카메라가 칸을 옮길 때 호출.
    void RefreshFacing(int cameraGridX, int cameraGridY);

    // 그리드 좌표(gridX,gridY) 칸이 이동 가능한지(FLOOR, TREASURE, UP_STAIR, DOWN_STAIR) 확인한다. 범위 밖이면 false.
    bool IsWalkable(int gridX, int gridY) const;

    const std::vector<PlacedPart>& GetPlacedParts() const { return placedParts; }

private:
    // 얇은 벽 한 칸이 서로 반대편(N/S 또는 E/W)의 이동 가능(FLOOR) 복도 두 개 사이에 끼어 있는데
    // 그 벽의 경사 장식은 한쪽 방향으로만 나 있는 경우(예: (3,9)), 카메라가 지금 서 있는 쪽 복도로
    // 경사가 보이도록 cellRot을 런타임에 스왑하기 위한 정보. PlaceAllCells에서 한 번 찾아두고, 카메라가
    // 그리드 칸을 옮길 때만 다시 계산한다 - 매 프레임 재계산하지 않는다.
    struct DynamicWallCell
    {
        int x = 0, y = 0;
        int spriteSetIndex = 0;
        bool isNorthSouth = true; // true: N/S 중 선택, false: E/W 중 선택
        int cellRotForNegativeSide = 0; // isNorthSouth면 North(y가 작은 쪽), 아니면 West(x가 작은 쪽)를 향하는 cellRot
        int cellRotForPositiveSide = 0; // isNorthSouth면 South(y가 큰 쪽), 아니면 East(x가 큰 쪽)를 향하는 cellRot
        int currentCellRot = 0;         // placedParts에 마지막으로 적용해둔 값. 바뀔 때만 다시 계산한다.
        size_t placedIndexBegin = 0;    // 이 셀의 파츠들이 placedParts에서 차지하는 구간
        size_t placedCount = 0;
    };

    // (gridX,gridY) 칸을 spriteSetIndex, cellRot(effectiveCellRot)으로 배치했을 때의 파츠 목록을 새로
    // 계산해 반환한다. PlaceCell(최초 배치)과 RefreshFacing(동적 재배치)이 공유하는 핵심 로직.
    std::vector<PlacedPart> BuildCellParts(int gridX, int gridY, int spriteSetIndex, int effectiveCellRot);

    // 던전 셀 하나(과 그 spriteSetIndex가 가리키는 파츠들)를 월드 좌표로 확정해 placedParts에 채워 넣는다.
    void PlaceCell(const Cell& cell);

    // spriteSetIndex가 가진 경사(rotationX!=0) 파츠들이 cellRot=0 기준으로 향하는, 서로 다른 로컬
    // 방향의 집합을 구한다(0=N,1=E,2=S,3=W). 같은 방향을 향하는 파츠 여러 개(예: 좌/우 절반)는 1개로 셈친다.
    std::vector<int> GetDistinctSlopeDirections(int spriteSetIndex);

    // PlaceCell 직후 호출. 이 셀이 "경사 방향 수 < 인접 이동가능(FLOOR) 칸 수"이면서 그 이동가능 칸이
    // 정확히 N/S 또는 E/W 반대편 쌍인 단순한 경우에만 dynamicWallCells에 등록한다.
    void TryRegisterDynamicWallCell(const Cell& cell, size_t placedIndexBegin, size_t placedCount);

    const DungeonData* dungeonData = nullptr;
    D01PartTemplateLibrary* templateLibrary = nullptr;

    std::vector<PlacedPart> placedParts;
    std::vector<DynamicWallCell> dynamicWallCells;
};
