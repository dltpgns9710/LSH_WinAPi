#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "D01PartTypes.h"
#include "../../Data/SpriteTextureMapData.h"

// sprite_texture_map.json의 스프라이트 이름을 실제로 그릴 수 있는 PartTemplate 목록으로 변환하고
// 캐시하는 에셋 계층. dungeonData나 카메라와는 무관하다.
class D01PartTemplateLibrary
{
public:
    void Init(SpriteTextureMapData&& spriteMapData, float cellSize);

    // 스프라이트 이름으로 텍스처 파츠 목록을 가져온다. 처음 요청된 이름이면 그때 잘라서 캐시에 채운다.
    const std::vector<PartTemplate>& GetOrBuild(const std::string& spriteName);

private:
    SpriteTextureMapData spriteMapData;
    float cellSize = 0.f;

    std::unordered_map<std::string, std::vector<PartTemplate>> spriteTemplateCache;

    // (파일명+atlasRect+tiled 설정) 조합별 크롭 캐시. 서로 다른 스프라이트 이름이 같은 크롭을 참조해도
    // (예: *_Unique 변형들이 같은 코너 텍스처를 공유) SpriteSheet::CreateSubRegion/CreateTiledRegion을
    // 한 번만 호출하도록 spriteTemplateCache보다 한 단계 더 세밀하게 중복 생성을 막는다.
    std::unordered_map<std::string, std::shared_ptr<SpriteSheet>> croppedSheetCache;
};
