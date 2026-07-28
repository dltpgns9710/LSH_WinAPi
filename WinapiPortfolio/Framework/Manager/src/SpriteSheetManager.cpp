#include "../include/SpriteSheetManager.h"
#include "../../Graphics/include/SpriteSheet.h"
#include "../../Graphics/include/Graphics.h"

void SpriteSheetManager::Init(std::shared_ptr<Graphics> gfx, fs::path directory)
{
	graphics = gfx;
	resourcePath = directory;
}

void SpriteSheetManager::LoadTexture(std::wstring key, std::wstring texturePath)
{
	if (GetTexture(key) != nullptr) return;

	fs::path fullPath = resourcePath / texturePath;
	std::shared_ptr<SpriteSheet> sprite = std::make_shared<SpriteSheet>(fullPath, graphics);
	if (!sprite->IsValid()) return;

	spriteMap.insert({key, sprite});
}

void SpriteSheetManager::UnLoadTexture(std::wstring key)
{
	spriteMap.erase(key);
}

std::shared_ptr<SpriteSheet> SpriteSheetManager::GetTexture(std::wstring key)
{
	auto find = spriteMap.find(key);
	if (find != spriteMap.end())
		return find->second;

	return nullptr;
}