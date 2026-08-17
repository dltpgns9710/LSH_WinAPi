#pragma once

#include <vector>

#include <d2d1.h>

#include "D01PartTypes.h"

class Camera;

// placedParts(읽기전용) + 카메라 + 화면 크기만으로 완결되는 무상태 렌더 파이프라인.
// 컬링(프러스텀 판정) -> 깊이 정렬(화가 알고리즘) -> 실제 스프라이트 드로우까지 담당한다.
class D01PartRenderer
{
public:
    void Render(const std::vector<PlacedPart>& placedParts, Camera* camera, D2D1_SIZE_F clientSize);
};
