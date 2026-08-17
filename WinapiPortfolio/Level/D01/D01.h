#pragma once

#include <memory>

#include "../../D2DFramework/Scene/include/GameLevel.h"
#include "../../Data/DungeonData.h"
#include "D01PartTypes.h"
#include "D01PartTemplateLibrary.h"
#include "D01CellPlacer.h"
#include "D01PartRenderer.h"

struct IDWriteFactory;
struct IDWriteTextFormat;
struct ID2D1SolidColorBrush;

// dungeon_01_01.json(그리드/셀 배치)과 sprite_texture_map.json(셀에 놓일 오브젝트 모양)을
// 조합해 던전 한 층 전체를 렌더링하는 레벨. 파츠 구성은 ObjectEditor, 타일 그리드/카메라 이동은
// Dungeon을 참고해서 만들었다. 에셋 빌드(D01PartTemplateLibrary), 셀 배치/동적 벽(D01CellPlacer),
// 컬링·정렬·드로우(D01PartRenderer)는 각각 별도 클래스로 분리돼 있다 - D01은 이들을 조립하고
// 입력 처리/디버그 표시만 담당하는 오케스트레이터.
class D01 : public GameLevel
{
    using super = GameLevel;
public:
    virtual void Load() override;
    virtual void UnLoad() override;
    virtual void Render() override;
    virtual void Update(double deltaTime) override;

private:
    std::shared_ptr<class SpriteAtlas> background;

    DungeonData dungeonData;

    D01PartTemplateLibrary templateLibrary;
    D01CellPlacer cellPlacer;
    D01PartRenderer partRenderer;

    // 카메라가 현재 바라보는 방향(그리드 기준 90도 단위, 0=+Z). A/D 회전 시 함께 갱신하며,
    // 이동 전에 목적지 칸이 FLOOR인지 판정하는 데 쓴다.
    int facingQuarter = 0;

    // 화면 우측 상단에 현재 던전 이름을 표시하기 위한 DirectWrite 리소스
    Microsoft::WRL::ComPtr<IDWriteFactory> nameDWriteFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> nameTextFormat;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> nameBrush;
};
