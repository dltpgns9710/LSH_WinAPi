#include "../include/SceneManager.h"

void SceneManager::Init()
{
	currentLevel.reset();
}

void SceneManager::LoadInitialLevel(std::shared_ptr<GameLevel> level)
{
	loading = true;
	level->Load();
	currentLevel = level;
	loading = false;
}

void SceneManager::SwitchLevel(std::shared_ptr<GameLevel> level)
{
	loading = true;
	if (currentLevel) currentLevel->UnLoad();
	level->Load();
	currentLevel.reset();
	currentLevel = level;
	loading = false;
}

void SceneManager::Render()
{
	if (loading || !currentLevel) return;
	currentLevel->Render();
}

void SceneManager::Update(double deltaTime)
{
	if (loading || !currentLevel) return;
	currentLevel->Update(deltaTime);
}

void SceneManager::PostUpdate()
{
	if (loading || !currentLevel) return;
	currentLevel->PostUpdate();
}
