#pragma once

#include <string>
#include <filesystem>
#include <unordered_map>

#include "../../Lib/include/json.hpp"
#include "../../Core/include/Singleton.h"

namespace fs = std::filesystem;

class DataManager : public Singleton<DataManager>
{
public:
	void Init(fs::path directory);

	bool Load(const std::wstring& key, const fs::path& fileName);
	void Unload(const std::wstring& key);

	inline bool HasData(const std::wstring& key) const { return dataMap.find(key) != dataMap.end(); }
	nlohmann::json* GetData(const std::wstring& key);

private:
	friend Singleton<DataManager>;
	DataManager() = default;
	~DataManager() = default;

	fs::path dataPath;
	std::unordered_map<std::wstring, nlohmann::json> dataMap;
};
