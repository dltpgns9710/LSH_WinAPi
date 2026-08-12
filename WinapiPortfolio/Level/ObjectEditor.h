#pragma once

#include <vector>
#include <memory>

#include "../D2DFramework/Scene/include/GameLevel.h"
#include "../D2DFramework/Math/include/Vector3.h"
#include "../Data/SpriteTextureMapData.h"

class SpriteSheet;

class ObjectEditor :  public GameLevel
{
    using super = GameLevel;
public:
    virtual void Load() override;
    virtual void UnLoad() override;
    virtual void Render() override;
    virtual void Update(double deltaTime) override;

private:
    struct RenderPart
    {
        std::shared_ptr<SpriteSheet> sheet;
        Vector3 localPosition;      // pivot(0,0,0) 기준 상대 좌표, cm
        Vector3 size;                // cm, 한 축이 0에 가까운 쪽이 이 평면의 법선
        bool isFloor = false;        // 법선=Y축(바닥류) 여부
        float extraYRotation = 0.f;  // 축 추정만으로는 방향이 어색한 특정 파츠에 대한 보정용 추가 회전
    };

    void LoadSpriteAt(int index);

    SpriteTextureMapData spriteMapData;
    int currentSpriteIndex = 0;
    std::vector<RenderPart> currentParts;

    Vector3 objectPivot;
    float groupAngle = 0.f;
};
