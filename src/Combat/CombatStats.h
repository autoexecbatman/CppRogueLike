#pragma once

#include <nlohmann/json.hpp>

using json = nlohmann::json;

class CombatStats
{
private:
	int dr{};
	int thaco{};

public:
	CombatStats(int drValue, int thacoValue) noexcept
		: dr(drValue), thaco(thacoValue) {}

	[[nodiscard]] int get_dr() const noexcept { return dr; }
	[[nodiscard]] int get_thaco() const noexcept { return thaco; }

	void set_dr(int value) noexcept { dr = value; }
	void set_thaco(int value) noexcept { thaco = value; }

	void load(const json& j);
	void save(json& j) const;
};
