// file: Systems/DataManager.cpp
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <format>
#include <fstream>
#include <ranges>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

// Include only the struct definitions
#include "CharismaAttributes.h"
#include "ConstitutionAttributes.h"
#include "DexterityAttributes.h"
#include "IntelligenceAttributes.h"
#include "StrengthAttributes.h"
#include "WisdomAttributes.h"
#include "Weapons.h"
#include "DataManager.h"
#include "MessageSystem.h"

namespace
{
// Search multiple paths for a JSON file
std::string find_data_file(const std::string& filename)
{
	std::vector<std::string> searchPaths = {
		"./" + filename, // Current directory
		"./bin/Debug/" + filename, // Build output (Debug)
		"./bin/Release/" + filename, // Build output (Release)
		"../src/json/" + filename, // From build dir to source
		"./src/json/" + filename, // From project root
	};

	for (const auto& path : searchPaths)
	{
		if (std::filesystem::exists(path))
		{
			return path;
		}
	}
	return "./" + filename; // Fallback to original behavior
}
} // namespace

void DataManager::load_all_data(MessageSystem& message_system)
{
	message_system.log("DataManager: Starting data load...");

	// The 18/xx bands are kept apart, so the plain rows stay indexed by score. Assigned
	// rather than appended, so a second load replaces the first.
	std::vector<StrengthAttributes> strengthRows = load_strength(find_data_file("strength.json"), message_system);
	auto is_exceptional_band = [](const StrengthAttributes& row)
	{
		return row.exceptionalFrom > 0;
	};
	exceptionalStrengthBands = strengthRows | std::views::filter(is_exceptional_band) | std::ranges::to<std::vector>();
	std::erase_if(strengthRows, is_exceptional_band);
	strengthAttributes = std::move(strengthRows);
	dexterityAttributes = load_dexterity(find_data_file("dexterity.json"), message_system);
	constitutionAttributes = load_constitution(find_data_file("constitution.json"), message_system);
	charismaAttributes = load_charisma(find_data_file("charisma.json"), message_system);
	intelligenceAttributes = load_intelligence(find_data_file("intelligence.json"), message_system);
	wisdomAttributes = load_wisdom(find_data_file("wisdom.json"), message_system);

	message_system.log("DataManager: All game data loaded successfully");
}

namespace
{
// The row a score reads from an ability table ordered by score from 1. Past the end it
// is the last row, the books capping every ability at 25; below 1, a score a creature
// was never given, it is a row of no adjustment.
//
// Example, with the Dexterity table's 25 rows:
//   row_for(dexterity, 17).DefensiveAdj;   // -> -3
//   row_for(dexterity, 26).DefensiveAdj;   // -> -6, the 25 row
//   row_for(dexterity, 0).DefensiveAdj;    // -> 0
template <typename Row>
Row row_for(const std::vector<Row>& table, int score)
{
	assert(!table.empty() && "an ability table read before it was loaded");

	if (score < 1)
	{
		return Row{};
	}
	if (std::cmp_greater(score, table.size()))
	{
		return table.back();
	}
	return table.at(score - 1);
}
} // namespace

DexterityAttributes DataManager::dexterity_for(int score) const
{
	return row_for(dexterityAttributes, score);
}

StrengthAttributes DataManager::strength_for(int score, int exceptional) const
{
	// Only an 18 has exceptional Strength; everywhere else the percentile means nothing.
	if (score != 18 || exceptional < 1)
	{
		return row_for(strengthAttributes, score);
	}

	auto holds = [exceptional](const StrengthAttributes& band)
	{
		return band.exceptionalFrom <= exceptional && exceptional <= band.exceptionalTo;
	};
	const auto band = std::ranges::find_if(exceptionalStrengthBands, holds);
	assert(band != exceptionalStrengthBands.end() && "exceptional Strength outside 1-100");
	return *band;
}

ConstitutionAttributes DataManager::constitution_for(int score) const
{
	return row_for(constitutionAttributes, score);
}

std::vector<Weapons> DataManager::load_weapons(const std::string& filename, MessageSystem& message_system)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		message_system.log(std::format("DataManager: Error opening {}", filename));
		return {};
	}

	nlohmann::json j;
	file >> j;

	std::vector<Weapons> data;
	for (const auto& item : j)
	{
		Weapons w;
		w.name = item.value("name", "");
		w.type = item.value("type", "");
		w.damageRoll = item.value("damageRoll", "");
		w.damageRollTwoHanded = item.value("damageRollTwoHanded", "");
		w.enhancementLevel = item.value("enhancementLevel", 0);

		// Handle enum values with defaults
		std::string handReq = item.value("handRequirement", "ONE_HANDED");
		if (handReq == "TWO_HANDED")
		{
			w.handRequirement = HandRequirement::TWO_HANDED;
		}
		else if (handReq == "OFF_HAND_ONLY")
		{
			w.handRequirement = HandRequirement::OFF_HAND_ONLY;
		}
		else
		{
			w.handRequirement = HandRequirement::ONE_HANDED;
		}

		std::string weapSize = item.value("weaponSize", "MEDIUM");
		if (weapSize == "TINY")
		{
			w.weaponSize = WeaponSize::TINY;
		}
		else if (weapSize == "SMALL")
		{
			w.weaponSize = WeaponSize::SMALL;
		}
		else if (weapSize == "LARGE")
		{
			w.weaponSize = WeaponSize::LARGE;
		}
		else if (weapSize == "GIANT")
		{
			w.weaponSize = WeaponSize::GIANT;
		}
		else
		{
			w.weaponSize = WeaponSize::MEDIUM;
		}

		// Handle arrays if they exist in JSON
		if (item.contains("hitBonusRange") && item["hitBonusRange"].is_array())
		{
			for (const auto& bonus : item["hitBonusRange"])
			{
				w.hitBonusRange.push_back(bonus.get<int>());
			}
		}

		if (item.contains("damageBonusRange") && item["damageBonusRange"].is_array())
		{
			for (const auto& bonus : item["damageBonusRange"])
			{
				w.damageBonusRange.push_back(bonus.get<int>());
			}
		}

		if (item.contains("specialProperties") && item["specialProperties"].is_array())
		{
			for (const auto& prop : item["specialProperties"])
			{
				w.specialProperties.push_back(prop.get<std::string>());
			}
		}

		data.push_back(w);
	}

	message_system.log(std::format("DataManager: Loaded {} weapons", data.size()));
	return data;
}

std::vector<StrengthAttributes> DataManager::load_strength(const std::string& filename, MessageSystem& message_system)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		message_system.log(std::format("DataManager: Error opening {}", filename));
		return {};
	}

	nlohmann::json j;
	file >> j;

	std::vector<StrengthAttributes> data;
	for (const auto& item : j)
	{
		StrengthAttributes s;
		s.Str = item.value("Str", 0);
		s.hitProb = item.value("Hit", 0);
		s.dmgAdj = item.value("Dmg", 0);
		s.wgtAllow = item.value("Wgt", 0);
		s.maxPress = item.value("MaxPress", 0);
		s.openDoors = item.value("OpenDoors", 0);
		s.BB_LG = item.value("BB_LG", 0.0);
		s.notes = item.value("Notes", "");

		// An 18/xx band carries its percentile range; a plain score's row carries none.
		if (item.contains("ExceptionalFrom"))
		{
			s.exceptionalFrom = item.at("ExceptionalFrom").get<int>();
			s.exceptionalTo = item.at("ExceptionalTo").get<int>();
		}
		data.push_back(s);
	}

	message_system.log(std::format("DataManager: Loaded {} strength attributes", data.size()));
	return data;
}

std::vector<DexterityAttributes> DataManager::load_dexterity(const std::string& filename, MessageSystem& message_system)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		message_system.log(std::format("DataManager: Error opening {}", filename));
		return {};
	}

	nlohmann::json j;
	file >> j;

	std::vector<DexterityAttributes> data;
	for (const auto& item : j)
	{
		DexterityAttributes d;
		d.Dex = item.value("Dex", 0);
		d.ReactionAdj = item.value("ReactionAdj", 0);
		d.MissileAttackAdj = item.value("MissileAttackAdj", 0);
		d.DefensiveAdj = item.value("DefensiveAdj", 0);
		data.push_back(d);
	}

	message_system.log(std::format("DataManager: Loaded {} dexterity attributes", data.size()));
	return data;
}

std::vector<ConstitutionAttributes> DataManager::load_constitution(const std::string& filename, MessageSystem& message_system)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		message_system.log(std::format("DataManager: Error opening {}", filename));
		return {};
	}

	nlohmann::json j;
	file >> j;

	std::vector<ConstitutionAttributes> data;
	for (const auto& item : j)
	{
		ConstitutionAttributes c;
		c.Con = item.value("Con", 0);
		c.HPAdj = item.value("HPAdj", 0);
		c.SystemShock = item.value("SystemShock", 0);
		c.ResurrectionSurvival = item.value("ResurrectionSurvival", 0);
		c.PoisonSave = item.value("PoisonSave", 0);
		c.Regeneration = item.value("Regeneration", 0);
		data.push_back(c);
	}

	message_system.log(std::format("DataManager: Loaded {} constitution attributes", data.size()));
	return data;
}

std::vector<CharismaAttributes> DataManager::load_charisma(const std::string& filename, MessageSystem& message_system)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		message_system.log(std::format("DataManager: Error opening {}", filename));
		return {};
	}

	nlohmann::json j;
	file >> j;

	std::vector<CharismaAttributes> data;
	for (const auto& item : j)
	{
		CharismaAttributes c;
		c.Cha = item.value("Cha", 0);
		c.MaxHencmen = item.value("MaxHencmen", 0);
		c.Loyalty = item.value("Loyalty", 0);
		c.ReactionAdj = item.value("ReactionAdj", 0);
		data.push_back(c);
	}

	message_system.log(std::format("DataManager: Loaded {} charisma attributes", data.size()));
	return data;
}

std::vector<IntelligenceAttributes> DataManager::load_intelligence(const std::string& filename, MessageSystem& message_system)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		message_system.log(std::format("DataManager: Error opening {}", filename));
		return {};
	}

	nlohmann::json j;
	file >> j;

	std::vector<IntelligenceAttributes> data;
	for (const auto& item : j)
	{
		IntelligenceAttributes i;
		i.Int = item.value("Int", 0);
		i.NumberOfLanguages = item.value("NumberOfLanguages", 0);
		i.SpellLevel = item.value("SpellLevel", 0);
		i.ChanceToLearnSpell = item.value("ChanceToLearnSpell", 0);
		i.MaxNumberOfSpells = item.value("MaxNumberOfSpells", 0);
		i.IllusionImmunity = item.value("IllusionImmunity", 0);
		data.push_back(i);
	}

	message_system.log(std::format("DataManager: Loaded {} intelligence attributes", data.size()));
	return data;
}

std::vector<WisdomAttributes> DataManager::load_wisdom(const std::string& filename, MessageSystem& message_system)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		message_system.log(std::format("DataManager: Error opening {}", filename));
		return {};
	}

	nlohmann::json j;
	file >> j;

	std::vector<WisdomAttributes> data;
	for (const auto& item : j)
	{
		WisdomAttributes w;
		w.Wis = item.value("Wis", 0);
		w.MagicalDefenseAdj = item.value("MagicalDefenseAdj", 0);
		w.BonusSpells = item.value("BonusSpells", 0);
		w.ChanceOfSpellFailure = item.value("ChanceOfSpellFailure", 0);
		w.SpellImmunity = item.value("SpellImmunity", 0);
		data.push_back(w);
	}

	message_system.log(std::format("DataManager: Loaded {} wisdom attributes", data.size()));
	return data;
}

// end of file: Systems/DataManager.cpp
