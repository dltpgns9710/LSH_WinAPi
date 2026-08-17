#pragma once

#include <memory>

#include "../../D2DFramework/Math/include/Vector3.h"
#include "../../D2DFramework/Math/include/MathUtil.h"

class SpriteSheet;

// D01 여러 클래스(D01PartTemplateLibrary/D01CellPlacer/D01PartRenderer)가 공유하는 순수 데이터
// 구조체와 헬퍼. 어느 한 클래스에도 속하지 않는 공용 타입이라 별도 헤더로 둔다.

constexpr float kHalfPi = static_cast<float>(MathUtil::pi / 2.0);

// 그리드(gridX,gridY) -> 월드 좌표. z는 grid y와 반대로 둬야 카메라 기본 방향(+Z, facingQuarter=0)이
// 시작 위치에서 계단을 등지는 방향(그리드 y가 작아지는 쪽)과 일치한다.
inline Vector3 GridToWorld(int gridX, int gridY, float cellSize)
{
    return Vector3(gridX * cellSize, 0.f, -(gridY * cellSize));
}

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
