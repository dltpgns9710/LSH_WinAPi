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

    // 스프라이트 이름으로 텍스처 파츠 목록을 가져온다. 처음 요청된 이름이면 그때 잘라서 캐시에 채운다.
    const std::vector<PartTemplate>& GetOrBuildSpriteTemplate(const std::string& spriteName);

    // 던전 셀 하나(과 그 spriteSetIndex가 가리키는 파츠들)를 월드 좌표로 확정해 placedParts에 채워 넣는다.
    void PlaceCell(const Cell& cell);

    // 그리드 좌표(gridX,gridY) 칸이 FLOOR인지(=이동 가능한지) 확인한다. 범위 밖이면 false.
    bool IsWalkable(int gridX, int gridY) const;

    std::shared_ptr<class SpriteAtlas> background;

    DungeonData dungeonData;
    SpriteTextureMapData spriteMapData;

    std::unordered_map<std::string, std::vector<PartTemplate>> spriteTemplateCache;
    std::vector<PlacedPart> placedParts; // Load() 시점에 던전 전체에 대해 한 번만 채워짐

    // 카메라가 현재 바라보는 방향(그리드 기준 90도 단위, 0=+Z). A/D 회전 시 함께 갱신하며,
    // 이동 전에 목적지 칸이 FLOOR인지 판정하는 데 쓴다.
    int facingQuarter = 0;

    // Camera::moveRequest/rotateRequest는 애니메이션 중이면 요청을 조용히 무시한다(카메라 상태는
    // 안 바뀜). 그런데 D01은 그 성공 여부를 알 수 없는 채로 facingQuarter를 무조건 갱신했었고,
    // 요청이 씹히면 facingQuarter(우리가 믿는 방향)와 카메라의 실제 방향이 어긋나 그 뒤 모든 이동
    // 판정이 틀어지는 버그가 있었다. 그래서 D01이 직접 쿨다운을 관리해, 이전 요청이 끝났을 시점
    // 이후에만 새 요청을 보낸다(카메라가 반드시 idle일 때만 호출하므로 항상 성공한다).
    float actionCooldown = 0.f;

    // 화면 우측 상단에 현재 던전 이름을 표시하기 위한 DirectWrite 리소스
    Microsoft::WRL::ComPtr<IDWriteFactory> nameDWriteFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> nameTextFormat;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> nameBrush;
};
