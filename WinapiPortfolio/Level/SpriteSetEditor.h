#pragma once

#include <vector>
#include <memory>

#include "../D2DFramework/Scene/include/GameLevel.h"
#include "../D2DFramework/Math/include/Vector3.h"
#include "../Data/DungeonData.h"
#include "../Data/SpriteTextureMapData.h"

class SpriteSheet;
struct IDWriteFactory;
struct IDWriteTextFormat;
struct ID2D1SolidColorBrush;

// dungeon_01_01.json의 spriteSets를 하나씩 순회하며 미리보기로 보여주는 에디터. 파츠 로딩/렌더링/조작
// 방식은 ObjectEditor를 그대로 참고했고, blockRot 적용 규칙은 D01::PlaceCell(cellRot=0인 경우)과 같다.
class SpriteSetEditor : public GameLevel
{
    using super = GameLevel;
public:
    virtual void Load() override;
    virtual void UnLoad() override;
    virtual void Render() override;
    virtual void Update(double deltaTime) override;

private:
    // spriteSets 항목 하나(SpriteSetPart 배열)를 이루는 텍스처 조각 하나가 실제로 그려질 형태로 확정된 것.
    struct RenderPart
    {
        std::shared_ptr<SpriteSheet> sheet;
        Vector3 localPosition;      // pivot(0,0,0) 기준 상대 좌표(blockRot 반영됨), cm
        Vector3 size;                // cm
        bool isFloor = false;        // 법선=Y축(바닥류) 여부
        float extraYRotation = 0.f;  // 라디안(blockRot + rotationY 반영됨)
        float extraXRotation = 0.f;  // 라디안, 계단처럼 파츠 자체를 기울이는(pitch) 추가 회전
    };

    // spriteSetKeys[listIndex]가 가리키는 spriteSets 항목을 currentParts에 채운다.
    void LoadSpriteSetAt(int listIndex);

    DungeonData dungeonData;
    SpriteTextureMapData spriteMapData;

    // dungeonData.spriteSets의 키(spriteSetIndex)를 정수로 정렬해 둔 목록. map의 키는 문자열이라
    // 그대로 순회하면 "10"이 "2"보다 앞에 오는 사전식 순서가 되므로, Q/E로 숫자 순서대로 훑어보려면
    // 미리 정수로 파싱해 정렬해둬야 한다.
    std::vector<int> spriteSetKeys;
    int currentListIndex = 0;
    std::vector<RenderPart> currentParts;

    Vector3 objectPivot;
    float groupAngle = 0.f;

    // 화면 우측 상단에 현재 보고 있는 spriteSetIndex를 표시하기 위한 DirectWrite 리소스
    Microsoft::WRL::ComPtr<IDWriteFactory> nameDWriteFactory;
    Microsoft::WRL::ComPtr<IDWriteTextFormat> nameTextFormat;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> nameBrush;
};
