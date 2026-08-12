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

	// 캐시된 raw json을 지정한 타입으로 변환해서 꺼낸다 (해당 타입에 from_json이 정의돼 있어야 함).
	// 키가 없거나 스키마가 맞지 않아 변환에 실패하면 false를 반환하고 outData는 건드리지 않는다.
	template<typename T>
	bool GetDataAs(const std::wstring& key, T& outData) const
	{
		auto find = dataMap.find(key);
		if (find == dataMap.end()) return false;

		try
		{
			outData = find->second.get<T>();
		}
		catch (const nlohmann::json::exception&)
		{
			return false;
		}
		return true;
	}

private:
	friend Singleton<DataManager>;
	DataManager() = default;
	~DataManager() = default;

	fs::path dataPath;
	std::unordered_map<std::wstring, nlohmann::json> dataMap;
};
