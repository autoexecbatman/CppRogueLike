#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class Persistent
{
public:
	virtual ~Persistent() = default;

	virtual void load(const json& json) = 0;
	virtual void save(json& json) = 0;
};
