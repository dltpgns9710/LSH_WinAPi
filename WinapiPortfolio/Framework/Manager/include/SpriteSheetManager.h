#pragma once

#include <unordered_map>
#include <string>
#include <memory>
#include <filesystem>

#include "../../Core/include/Singleton.h"

class SpriteSheet;
class Graphics;

namespace fs = std::filesystem;

class SpriteSheetManager : public Singleton<SpriteSheetManager>
{
public:
	void Init(std::shared_ptr<Graphics> gfx, fs::path directory);
	void LoadTexture(std::wstring key, std::wstring texturePath);
	void UnLoadTexture(std::wstring key);
	std::shared_ptr<SpriteSheet> GetTexture(std::wstring key);

private:
	friend Singleton<SpriteSheetManager>;
	SpriteSheetManager() = default;
	~SpriteSheetManager() = default;

	std::shared_ptr<Graphics> graphics;
	fs::path resourcePath;
	std::unordered_map<std::wstring, std::shared_ptr<SpriteSheet>> spriteMap;
};
