#pragma once

#include <vector>
#include "../D2DFramework/Scene/include/GameLevel.h"
#include "../D2DFramework/Math/include/Vector3.h"

struct FloorTileInstance
{
    static constexpr float kTileSize = 400.f;

    Vector3 position;
    int tileIndex;
};

class Dungeon : public GameLevel
{
    using super = GameLevel;
public:
    virtual void Load() override;
    virtual void UnLoad() override;
    virtual void Render() override;
    virtual void Update(double deltaTime) override;

private:
    std::shared_ptr<class SpriteAtlas> background;
    std::shared_ptr<class SpriteSheet> floorTiles[2];
    std::vector<FloorTileInstance> floorGrid;
    
};
