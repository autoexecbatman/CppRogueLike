// file: DataManager.cpp
//
// The AD&D 2nd Edition ability tables, loaded from JSON and looked up by score.
//
// Six printed tables live in src/json - strength, dexterity, constitution, charisma,
// intelligence and wisdom - and this is what turns them into something the game can
// ask questions of. Everything that depends on an ability score comes through here:
// armour class and surprise from Dexterity, hit points and system shock from
// Constitution, hit and damage bonuses and carrying capacity from Strength, spell
// learning from Intelligence, spell failure and bonus spells from Wisdom, henchmen
// and loyalty from Charisma.
//
// Usage:
//
//   DataManager dataManager;
//   dataManager.load_all_data(messageSystem);        // reads all six files
//
//   dataManager.dexterity_for(17).DefensiveAdj;      // -> -3
//   dataManager.strength_for(18, 76).dmgAdj;         // -> 4, the 18/76-90 band
//   dataManager.constitution_for(17).HPAdj;          // -> 3
//
// A score outside a table's printed range is clamped to its nearest row rather than
// refused, because ability scores reach past the tables through magic - see
// feedback_index_range_before_at in the memory store for what a bare .at() cost here.
//
// Every column a table prints is required. The row parsers below throw on a missing
// one rather than substituting a zero, because the parser is the only schema this
// data has and a silently defaulted column is a wrong game rule with nothing printed.
// Strength's percentile band is the one genuine option: only the 18/xx rows carry it.
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
#include "CreatureClass.h"
#include "DataManager.h"
#include "DexterityAttributes.h"
#include "IntelligenceAttributes.h"
#include "MessageSystem.h"
#include "StrengthAttributes.h"
#include "Weapons.h"
#include "WisdomAttributes.h"

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

// Player's Handbook Table 3 grants more than +2 to warriors alone.
constexpr int NON_WARRIOR_BONUS_CAP = 2;
} // namespace

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

// One row of each printed table, parsed. Lifted out of the loaders so a row can be
// read without a file: the loaders own finding and opening the file, these own what
// a row means. Unchanged from the loop bodies they replace.

StrengthAttributes strength_row_from(const nlohmann::json& item)
{
	StrengthAttributes s;
	s.Str = item.at("Str").get<int>();
	s.hitProb = item.at("Hit").get<int>();
	s.dmgAdj = item.at("Dmg").get<int>();
	s.wgtAllow = item.at("Wgt").get<int>();
	s.maxPress = item.at("MaxPress").get<int>();
	s.maxCarried = item.at("maxCarried").get<int>();
	s.openDoors = item.at("OpenDoors").get<int>();
	s.BB_LG = item.at("BB_LG").get<double>();
	s.notes = item.at("Notes").get<std::string>();

	// An 18/xx band carries its percentile range; a plain score's row carries none.
	if (item.contains("ExceptionalFrom"))
	{
		s.exceptionalFrom = item.at("ExceptionalFrom").get<int>();
		s.exceptionalTo = item.at("ExceptionalTo").get<int>();
	}

	// Table 47's bands, null on the scores the table does not print. The key is
	// required on every row, so a row that simply forgot it fails loudly.
	const nlohmann::json& bands = item.at("encumbrance");
	if (!bands.is_null())
	{
		s.encumbrance = EncumbranceBands{
			bands.at("unencumberedTo").get<int>(),
			bands.at("lightTo").get<int>(),
			bands.at("moderateTo").get<int>(),
			bands.at("heavyTo").get<int>()
		};
	}
	return s;
}

DexterityAttributes dexterity_row_from(const nlohmann::json& item)
{
	DexterityAttributes d;
	d.Dex = item.at("Dex").get<int>();
	d.ReactionAdj = item.at("ReactionAdj").get<int>();
	d.MissileAttackAdj = item.at("MissileAttackAdj").get<int>();
	d.DefensiveAdj = item.at("DefensiveAdj").get<int>();
	return d;
}

ConstitutionAttributes constitution_row_from(const nlohmann::json& item)
{
	ConstitutionAttributes c;
	c.Con = item.at("Con").get<int>();
	c.HPAdj = item.at("HPAdj").get<int>();
	c.hitDieMinimum = item.at("hitDieMinimum").get<int>();
	c.SystemShock = item.at("SystemShock").get<int>();
	c.ResurrectionSurvival = item.at("ResurrectionSurvival").get<int>();
	c.PoisonSave = item.at("PoisonSave").get<int>();
	c.Regeneration = item.at("Regeneration").get<int>();
	return c;
}

CharismaAttributes charisma_row_from(const nlohmann::json& item)
{
	CharismaAttributes c;
	c.Cha = item.at("Cha").get<int>();
	c.MaxHencmen = item.at("MaxHencmen").get<int>();
	c.Loyalty = item.at("Loyalty").get<int>();
	c.ReactionAdj = item.at("ReactionAdj").get<int>();
	return c;
}

IntelligenceAttributes intelligence_row_from(const nlohmann::json& item)
{
	IntelligenceAttributes i;
	i.Int = item.at("Int").get<int>();
	i.NumberOfLanguages = item.at("NumberOfLanguages").get<int>();
	i.SpellLevel = item.at("SpellLevel").get<int>();
	i.ChanceToLearnSpell = item.at("ChanceToLearnSpell").get<int>();
	i.MaxNumberOfSpells = item.at("MaxNumberOfSpells").get<int>();
	i.IllusionImmunity = item.at("IllusionImmunity").get<int>();
	return i;
}

WisdomAttributes wisdom_row_from(const nlohmann::json& item)
{
	WisdomAttributes w;
	w.Wis = item.at("Wis").get<int>();
	w.MagicalDefenseAdj = item.at("MagicalDefenseAdj").get<int>();
	w.bonusSpells = item.at("bonusSpells").get<std::vector<int>>();
	w.ChanceOfSpellFailure = item.at("ChanceOfSpellFailure").get<int>();
	w.SpellImmunity = item.at("SpellImmunity").get<int>();
	return w;
}

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

DexterityAttributes DataManager::dexterity_for(int score) const
{
	return row_for(dexterityAttributes, score);
}

WisdomAttributes DataManager::wisdom_for(int score) const
{
	return row_for(wisdomAttributes, score);
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

int DataManager::constitution_hit_point_adjustment(int score, CreatureClass creatureClass) const
{
	const int tableAdjustment = constitution_for(score).HPAdj;

	// The table is the warrior column. Only a bonus is capped, so a penalty
	// passes through untouched for every class.
	if (is_warrior(creatureClass))
	{
		return tableAdjustment;
	}
	return std::min(tableAdjustment, NON_WARRIOR_BONUS_CAP);
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
		data.push_back(strength_row_from(item));
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
		data.push_back(dexterity_row_from(item));
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
		data.push_back(constitution_row_from(item));
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
		data.push_back(charisma_row_from(item));
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
		data.push_back(intelligence_row_from(item));
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
		data.push_back(wisdom_row_from(item));
	}

	message_system.log(std::format("DataManager: Loaded {} wisdom attributes", data.size()));
	return data;
}

// end of file: Systems/DataManager.cpp
