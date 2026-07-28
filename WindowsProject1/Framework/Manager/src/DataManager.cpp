#include "../include/DataManager.h"
#include <fstream>

void DataManager::Init(fs::path directory)
{
	dataPath = directory;
}

bool DataManager::Load(const std::wstring& key, const fs::path& fileName)
{
	std::ifstream file(dataPath / fileName);
	if (!file.is_open()) return false;

	nlohmann::json data;
	try
	{
		file >> data;
	}
	catch (const nlohmann::json::parse_error&)
	{
		return false;
	}

	dataMap[key] = data;
	return true;
}

void DataManager::Unload(const std::wstring& key)
{
	dataMap.erase(key);
}

nlohmann::json* DataManager::GetData(const std::wstring& key)
{
	auto find = dataMap.find(key);
	return find != dataMap.end() ? &find->second : nullptr;
}
