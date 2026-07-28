#pragma once

#include "../Framework/Scene/include/GameLevel.h"

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
};
