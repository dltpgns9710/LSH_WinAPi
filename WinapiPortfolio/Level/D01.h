#pragma once

#include <vector>
#include <unordered_map>
#include <string>
#include <memory>

#include "../D2DFramework/Scene/include/GameLevel.h"
#include "../D2DFramework/Math/include/Vector3.h"
#include "../Data/DungeonData.h"
#include "../Data/SpriteTextureMapData.h"

class SpriteSheet;
struct IDWriteFactory;
struct IDWriteTextFormat;
struct ID2D1SolidColorBrush;

// dungeon_01_01.json(그리드/셀 배치)과 sprite_texture_map.json(셀에 놓일 오브젝트 모양)을
// 조합해 던전 한 층 전체를 렌더링하는 레벨. 파츠 구성은 ObjectEditor, 타일 그리드/카메라 이동은
// Dungeon을 참고해서 만들었다.
class D01 : public GameLevel
{
    using super = GameLevel;
public:
    virtual void Load() override;
    virtual void UnLoad() override;
    virtual void Render() override;
    virtual void Update(double deltaTime) override;

private:
    // 스프라이트 하나(sprite_texture_map.json의 sprites[])를 이루는 텍스처 조각 하나.
    // 스프라이트 이름별로 딱 한 번만 만들어 캐시해두고, 셀마다 재사용한다(ObjectEditor::RenderPart 대응).
    struct PartTemplate
    {
        std::shared_ptr<SpriteSheet> sheet;
        Vector3 localPosition; // 스프라이트 피벗(0,0,0) 기준 상대 좌표, cm
        Vector3 size;           // cm
        bool isFloor = false;   // 법선=Y축(바닥류) 여부
        float extraYRotation = 0.f; // 라디안
        float extraXRotation = 0.f; // 라디안, 계단처럼 파츠 자체를 기울이는(pitch) 추가 회전
        int layer = 1;           // 같은 셀 안에서 그리는 순서(작을수록 먼저=뒤). 0=배경/받침, 1=디테일.
    };

    // 던전 그리드에 실제로 배치되어 월드 좌표/회전이 확정된 파츠.
    struct PlacedPart
    {
        std::shared_ptr<SpriteSheet> sheet;
        Vector3 worldPosition;
        Vector3 size;
        bool isFloor = false;
        float worldYRotation = 0.f; // 라디안
        float extraXRotation = 0.f; // 라디안, 셀 회전과 무관하게 파츠 자신을 축으로 기울이는 pitch
        int layer = 1;           // PartTemplate::layer 그대로 복사
        Vector3 cellCenterWorldPos; // 이 파츠가 속한 셀의 중심(y=0). 셀 단위 정렬 기준점 - tile_layer_sort_design.md 참고.
    };

    // 얇은 벽 한 칸이 서로 반대편(N/S 또는 E/W)의 이동 가능(FLOOR) 복도 두 개 사이에 끼어 있는데
    // 그 벽의 경사 장식은 한쪽 방향으로만 나 있는 경우(예: (3,9)), 카메라가 지금 서 있는 쪽 복도로
    // 경사가 보이도록 cellRot을 런타임에 스왑하기 위한 정보. Load()에서 한 번 찾아두고, 카메라가
    // 그리드 칸을 옮길 때만(Update의 tryMove) 다시 계산한다 - 매 프레임 재계산하지 않는다.
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

    // 스프라이트 이름으로 텍스처 파츠 목록을 가져온다. 처음 요청된 이름이면 그때 잘라서 캐시에 채운다.
    const std::vector<PartTemplate>& GetOrBuildSpriteTemplate(const std::string& spriteName);

    // (gridX,gridY) 칸을 spriteSetIndex, cellRot(effectiveCellRot)으로 배치했을 때의 파츠 목록을 새로
    // 계산해 반환한다. PlaceCell(최초 배치)과 RefreshDynamicWallFacing(동적 재배치)이 공유하는 핵심 로직.
    std::vector<PlacedPart> BuildCellParts(int gridX, int gridY, int spriteSetIndex, int effectiveCellRot);

    // 던전 셀 하나(과 그 spriteSetIndex가 가리키는 파츠들)를 월드 좌표로 확정해 placedParts에 채워 넣는다.
    void PlaceCell(const Cell& cell);

    // spriteSetIndex가 가진 경사(rotationX!=0) 파츠들이 cellRot=0 기준으로 향하는, 서로 다른 로컬
    // 방향의 집합을 구한다(0=N,1=E,2=S,3=W). 같은 방향을 향하는 파츠 여러 개(예: 좌/우 절반)는 1개로 셈친다.
    std::vector<int> GetDistinctSlopeDirections(int spriteSetIndex);

    // PlaceCell 직후 호출. 이 셀이 "경사 방향 수 < 인접 이동가능(FLOOR) 칸 수"이면서 그 이동가능 칸이
    // 정확히 N/S 또는 E/W 반대편 쌍인 단순한 경우에만 dynamicWallCells에 등록한다.
    void TryRegisterDynamicWallCell(const Cell& cell, size_t placedIndexBegin, size_t placedCount);

    // 카메라가 실제로 도착할 그리드 칸(cameraGridX,Y) 기준으로 dynamicWallCells 각각이 어느 쪽을
    // 향해야 하는지 다시 계산해 placedParts를 그 자리에서 덮어쓴다. Load() 끝(시작 위치 기준)과
    // 카메라가 칸을 옮길 때(Update의 tryMove 성공 시)만 호출한다.
    void RefreshDynamicWallFacing(int cameraGridX, int cameraGridY);

    // 그리드 좌표(gridX,gridY) 칸이 이동 가능한지(FLOOR, TREASURE, UP_STAIR, DOWN_STAIR) 확인한다. 범위 밖이면 false.
    bool IsWalkable(int gridX, int gridY) const;

    std::shared_ptr<class SpriteAtlas> background;

    DungeonData dungeonData;
    SpriteTextureMapData spriteMapData;

    std::unordered_map<std::string, std::vector<PartTemplate>> spriteTemplateCache;

    // (파일명+atlasRect+tiled 설정) 조합별 크롭 캐시. 서로 다른 스프라이트 이름이 같은 크롭을 참조해도
    // (예: *_Unique 변형들이 같은 코너 텍스처를 공유) SpriteSheet::CreateSubRegion/CreateTiledRegion을
    // 한 번만 호출하도록 spriteTemplateCache보다 한 단계 더 세밀하게 중복 생성을 막는다.
    std::unordered_map<std::string, std::shared_ptr<SpriteSheet>> croppedSheetCache;
    std::vector<PlacedPart> placedParts; // Load() 시점에 던전 전체에 대해 한 번만 채워짐

    std::vector<DynamicWallCell> dynamicWallCells; // Load()에서 한 번 탐색해두는, 카메라 위치에 따라 방향이 바뀌는 얇은 벽들

    // 카메라가 현재 바라보는 방향(그리드 기준 90도 단위, 0=+Z). A/D 회전 시 함께 갱신하며,
    // 이동 전에 목적지 칸이 FLOOR인지 판정하는 데 쓴다.
    int facingQuarter = 0;

    // 화면 우측 상단에 현재 던전 이름을 표시하기 위한 DirectWrite 리소스
    Microsoft::WRL::ComPtr<IDWriteFactory> nameDWriteFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> nameTextFormat;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> nameBrush;
};
