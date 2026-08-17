#include "D01PartRenderer.h"

#include <cmath>
#include <cfloat>
#include <algorithm>

#include "../../D2DFramework/Camera/include/Camera.h"
#include "../../D2DFramework/Graphics/include/SpriteSheet.h"
#include "../../D2DFramework/Math/include/Matrix4x4.h"

void D01PartRenderer::Render(const std::vector<PlacedPart>& placedParts, Camera* camera, D2D1_SIZE_F clientSize)
{
    Matrix4x4 cameraView = camera->GetViewMatrix();
    Matrix4x4 cameraProj = camera->GetProjectionMatrix();
    Matrix4x4 viewport = Matrix4x4::Viewport(clientSize.width, clientSize.height);
    Matrix4x4 viewProj = cameraProj * cameraView; // z-버퍼 정렬용
    // 파츠와 무관한 프레임 상수라 루프 밖에서 한 번만 계산해두고 재사용한다 - 안 그러면 그리기
    // 루프에서 viewProj를 재사용하지 못하고 파츠마다 cameraProj * cameraView를 다시 계산하게 된다.
    Matrix4x4 viewportViewProj = viewport * viewProj;

    // 파츠 하나당 실제로 그려지는 사각형의 네 모서리(월드 좌표). 컬링(프러스텀 판정)과 깊이 정렬
    // 둘 다 이 모서리가 필요해서, 파츠마다 한 번만 계산해 재사용한다 - 예전에는 정렬 비교자 안에서
    // 매 비교마다(O(n log n)회) 다시 계산해 같은 파츠를 몇 번이고 반복 계산하고 있었다.
    struct VisiblePart
    {
        const PlacedPart* part;
        float depth; // z-버퍼 값(클수록 카메라에서 멀다), 네 모서리 중 가장 먼 지점 기준. 같은 셀·같은 layer일 때만 타이브레이크로 씀.
        float cellDepth; // 이 파츠가 속한 셀 중심의 view-space Z. 코너가 아니라 점 하나라 보는 방향이 바뀌어도 뒤집히지 않는다.
    };

    auto computeCorners = [](const PlacedPart& p, Vector3 outCorners[4])
        {
            float halfX = p.size.x * 0.5f;
            float halfOther = (p.isFloor ? p.size.z : p.size.y) * 0.5f;
            float tiltRadians = p.extraXRotation + (p.isFloor ? kHalfPi : 0.f);

            // 코너 4개 모두 같은 tiltRadians·worldYRotation을 쓰므로, RotateX/RotateY를 코너마다
            // 호출해 sin/cos을 4번씩 중복 계산하는 대신 한 번만 구해서 회전식을 직접 전개해 적용한다.
            float sinTilt = std::sin(tiltRadians);
            float cosTilt = std::cos(tiltRadians);
            float sinYaw = std::sin(p.worldYRotation);
            float cosYaw = std::cos(p.worldYRotation);

            int i = 0;
            for (float sx : { -1.f, 1.f })
            {
                for (float sy : { -1.f, 1.f })
                {
                    float x0 = sx * halfX;
                    float y0 = sy * halfOther;
                    // z0 = 0이므로 RotateX(x0, y0, 0) = (x0, cosTilt*y0, sinTilt*y0)까지만 남는다.
                    float rx = cosYaw * x0 + sinYaw * (sinTilt * y0);
                    float ry = cosTilt * y0;
                    float rz = -sinYaw * x0 + cosYaw * (sinTilt * y0);
                    outCorners[i++] = p.worldPosition + Vector3(rx, ry, rz);
                }
            }
        };

    // 카메라 시야에 들어올 만한 파츠만 골라 그린다 (던전 전체를 매 프레임 그리기엔 파츠 수가 많다).
    // Camera::isRenderTile과 동일한 방식 - 중심점만 보던 이전의 사각 박스 컬링 대신, 파츠의 네 모서리
    // 중 하나라도 카메라 프러스텀(near/far/up/down 평면) 안이면 보이는 것으로 취급한다. 중심점이
    // 시야 밖이어도 큰 파츠의 모서리가 화면에 걸쳐 있으면 더 이상 잘못 컬링되지 않는다.
    std::vector<VisiblePart> drawOrder;
    drawOrder.reserve(placedParts.size());
    for (const PlacedPart& part : placedParts)
    {
        if (!part.sheet) continue;
        if (!camera->isRenderPosition(part.worldPosition)) continue; // 값싼 사전 필터

        Vector3 corners[4];
        computeCorners(part, corners);

        bool visible = false;
        float depth = -FLT_MAX;
        for (const Vector3& w : corners)
        {
            if (!visible && camera->isRenderPoint(w)) visible = true;

            float clipZ = viewProj.m[2][0] * w.x + viewProj.m[2][1] * w.y
                + viewProj.m[2][2] * w.z + viewProj.m[2][3];
            float clipW = viewProj.m[3][0] * w.x + viewProj.m[3][1] * w.y
                + viewProj.m[3][2] * w.z + viewProj.m[3][3];
            float d = clipW != 0.f ? clipZ / clipW : clipZ;
            if (d > depth) depth = d;
        }
        if (!visible) continue;

        // 셀 중심(y=0, 파츠 회전과 무관하게 고정)의 view-space Z. 파츠 자신의 코너 대신 이 점을 1차
        // 정렬 기준으로 쓰면, 셀보다 넓은 오버사이즈 배경 패널이 회전으로 인해 인접 셀 깊이 범위를
        // 침범해도(코너 기반 depth가 방향에 따라 뒤집히던 원인) "어느 셀 소속인가"는 안 바뀌므로
        // 정렬 순서가 보는 방향에 따라 뒤집히지 않는다 - tile_layer_sort_design.md 참고.
        const Vector3& c = part.cellCenterWorldPos;
        float cellDepth = cameraView.m[2][0] * c.x + cameraView.m[2][1] * c.y
            + cameraView.m[2][2] * c.z + cameraView.m[2][3];

        drawOrder.push_back({ &part, depth, cellDepth });
    }

    // D2D는 깊이 테스트 없이 그린 순서대로 위에 덮어 그리므로, 카메라로부터 먼 텍스처부터 그려야 한다.
    // 진짜 바닥(y≈0, 법선=Y)은 z-버퍼 값과 무관하게 항상 맨 먼저(=맨 뒤) 그린다 - ObjectEditor::Render 참고.
    auto isGroundFloor = [](const PlacedPart* p) { return p->isFloor && std::abs(p->worldPosition.y) < 1.f; };

    std::sort(drawOrder.begin(), drawOrder.end(), [&](const VisiblePart& a, const VisiblePart& b)
        {
            if (isGroundFloor(a.part) != isGroundFloor(b.part)) return isGroundFloor(a.part);
            if (a.cellDepth != b.cellDepth) return a.cellDepth > b.cellDepth; // 먼 셀부터(내림차순)
            // 같은 셀 안에서는 지오메트리 깊이 대신 데이터로 명시된 layer를 따른다(작을수록 먼저=배경).
            if (a.part->layer != b.part->layer) return a.part->layer < b.part->layer;
            return a.depth > b.depth; // 같은 셀·같은 layer일 때만 코너-depth로 타이브레이크
        });

    for (const VisiblePart& visiblePart : drawOrder)
    {
        const PlacedPart& part = *visiblePart.part;

        float pixelW = part.sheet->GetImageWidth();
        float pixelH = part.sheet->GetImageHeight();
        if (pixelW <= 0.f || pixelH <= 0.f) continue;

        Matrix4x4 extraRotation = Matrix4x4::RotationY(part.worldYRotation);
        // 계단처럼 평면 자체를 기울여야 하는 파츠용 추가 회전(pitch) - ObjectEditor::Render의 extraTilt와 동일.
        Matrix4x4 extraTilt = Matrix4x4::RotationX(part.extraXRotation);

        Matrix4x4 model;
        if (part.isFloor)
        {
            model = Matrix4x4::Translation(part.worldPosition)
                * extraRotation
                * Matrix4x4::RotationX(kHalfPi)
                * extraTilt
                * Matrix4x4::Scale(Vector3(part.size.x / pixelW, part.size.z / pixelH, 1.f))
                * Matrix4x4::Translation(Vector3(-pixelW / 2.f, -pixelH / 2.f, 0.f));
        }
        else
        {
            model = Matrix4x4::Translation(part.worldPosition)
                * extraRotation
                * extraTilt
                * Matrix4x4::Scale(Vector3(part.size.x / pixelW, -part.size.y / pixelH, 1.f))
                * Matrix4x4::Translation(Vector3(-pixelW / 2.f, -pixelH / 2.f, 0.f));
        }

        Matrix4x4 final = viewportViewProj * model;
        part.sheet->DrawSpriteWarped(final);
    }
}
