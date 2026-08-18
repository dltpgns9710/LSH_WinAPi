#pragma once

#include <memory>
#include "../../Scene/include/GameLevel.h"
#include "../../Core/include/Singleton.h"

class SceneManager : public Singleton<SceneManager>
{
public:
	void Init();

	void LoadInitialLevel(std::shared_ptr<GameLevel> level);
	void SwitchLevel(std::shared_ptr<GameLevel> level);

	void OnResize(float width, float height);

	void Render();
	void Update(double deltaTime);
	void PostUpdate();

private:
	friend Singleton<SceneManager>;
	SceneManager() = default;
	~SceneManager() = default;

	std::shared_ptr<GameLevel> currentLevel;
	bool loading = false;
};
