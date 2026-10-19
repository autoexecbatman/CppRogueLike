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
#include <format>
#include <fstream>
#include <ranges>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

// Include only the struct definitions
#include "CharismaAttributes.h"
#include "ConstitutionAttributes.h"
#include "CreatureClass.h"
#include "DataManager.h"
#include "DexterityAttributes.h"
#include "IntelligenceAttributes.h"
#include "MessageSystem.h"
#include "Paths.h"
#include "StrengthAttributes.h"
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
// The whole of one table file. A file that cannot be opened stops the load, so a game
// never runs on empty ability tables that only fail at the first lookup.
//
// Example:
//   read_table(Paths::resolve(Paths::WISDOM_TABLE).string()); // -> the 25 Wisdom rows as JSON
//   read_table("missing.json"); // throws std::runtime_error naming the file
nlohmann::json read_table(const std::string& filename)
{
	std::ifstream file(filename);
	if (!file.is_open())
	{
		throw std::runtime_error(std::format("DataManager: cannot open {}, so its ability table cannot be read", filename));
	}

	nlohmann::json table;
	file >> table;
	return table;
}
} // namespace

// One row of each printed table, parsed. Lifted out of the loaders so a row can be
// read without a file: the loaders own finding and opening the file, these own what
// a row means. Unchanged from the loop bodies they replace.

StrengthAttributes strength_row_from(const nlohmann::json& item)
{
	StrengthAttributes strength;
	strength.Str = item.at("Str").get<int>();
	strength.hitProb = item.at("Hit").get<int>();
	strength.dmgAdj = item.at("Dmg").get<int>();
	strength.wgtAllow = item.at("Wgt").get<int>();
	strength.maxPress = item.at("MaxPress").get<int>();
	strength.maxCarried = item.at("maxCarried").get<int>();
	strength.openDoors = item.at("OpenDoors").get<int>();
	strength.BB_LG = item.at("BB_LG").get<double>();
	strength.notes = item.at("Notes").get<std::string>();

	// An 18/xx band carries its percentile range; a plain score's row carries none.
	if (item.contains("ExceptionalFrom"))
	{
		strength.exceptionalFrom = item.at("ExceptionalFrom").get<int>();
		strength.exceptionalTo = item.at("ExceptionalTo").get<int>();
	}

	// Table 47's bands, null on the scores the table does not print. The key is
	// required on every row, so a row that simply forgot it fails loudly.
	const nlohmann::json& bands = item.at("encumbrance");
	if (!bands.is_null())
	{
		strength.encumbrance = EncumbranceBands{
			bands.at("unencumberedTo").get<int>(),
			bands.at("lightTo").get<int>(),
			bands.at("moderateTo").get<int>(),
			bands.at("heavyTo").get<int>()
		};
	}
	return strength;
}

DexterityAttributes dexterity_row_from(const nlohmann::json& item)
{
	DexterityAttributes dexterity;
	dexterity.Dex = item.at("Dex").get<int>();
	dexterity.ReactionAdj = item.at("ReactionAdj").get<int>();
	dexterity.MissileAttackAdj = item.at("MissileAttackAdj").get<int>();
	dexterity.DefensiveAdj = item.at("DefensiveAdj").get<int>();
	return dexterity;
}

ConstitutionAttributes constitution_row_from(const nlohmann::json& item)
{
	ConstitutionAttributes constitution;
	constitution.Con = item.at("Con").get<int>();
	constitution.HPAdj = item.at("HPAdj").get<int>();
	constitution.hitDieMinimum = item.at("hitDieMinimum").get<int>();
	constitution.SystemShock = item.at("SystemShock").get<int>();
	constitution.ResurrectionSurvival = item.at("ResurrectionSurvival").get<int>();
	constitution.PoisonSave = item.at("PoisonSave").get<int>();
	constitution.Regeneration = item.at("Regeneration").get<int>();
	return constitution;
}

CharismaAttributes charisma_row_from(const nlohmann::json& item)
{
	CharismaAttributes charisma;
	charisma.Cha = item.at("Cha").get<int>();
	charisma.MaxHencmen = item.at("MaxHencmen").get<int>();
	charisma.Loyalty = item.at("Loyalty").get<int>();
	charisma.ReactionAdj = item.at("ReactionAdj").get<int>();
	return charisma;
}

IntelligenceAttributes intelligence_row_from(const nlohmann::json& item)
{
	IntelligenceAttributes intelligence;
	intelligence.Int = item.at("Int").get<int>();
	intelligence.NumberOfLanguages = item.at("NumberOfLanguages").get<int>();
	intelligence.SpellLevel = item.at("SpellLevel").get<int>();
	intelligence.ChanceToLearnSpell = item.at("ChanceToLearnSpell").get<int>();
	intelligence.MaxNumberOfSpells = item.at("MaxNumberOfSpells").get<int>();
	intelligence.IllusionImmunity = item.at("IllusionImmunity").get<int>();
	return intelligence;
}

WisdomAttributes wisdom_row_from(const nlohmann::json& item)
{
	WisdomAttributes wisdom;
	wisdom.Wis = item.at("Wis").get<int>();
	wisdom.MagicalDefenseAdj = item.at("MagicalDefenseAdj").get<int>();
	wisdom.bonusSpells = item.at("bonusSpells").get<std::vector<int>>();
	wisdom.ChanceOfSpellFailure = item.at("ChanceOfSpellFailure").get<int>();
	wisdom.SpellImmunity = item.at("SpellImmunity").get<int>();
	return wisdom;
}

void DataManager::load_all_data(MessageSystem& message_system)
{
	message_system.log("DataManager: Starting data load...");

	// The 18/xx bands are kept apart, so the plain rows stay indexed by score. Assigned
	// rather than appended, so a second load replaces the first.
	std::vector<StrengthAttributes> strengthRows = load_strength(Paths::resolve(Paths::STRENGTH_TABLE).string(), message_system);
	auto is_exceptional_band = [](const StrengthAttributes& row)
	{
		return row.exceptionalFrom > 0;
	};
	exceptionalStrengthBands = strengthRows | std::views::filter(is_exceptional_band) | std::ranges::to<std::vector>();
	std::erase_if(strengthRows, is_exceptional_band);
	strengthAttributes = std::move(strengthRows);
	dexterityAttributes = load_dexterity(Paths::resolve(Paths::DEXTERITY_TABLE).string(), message_system);
	constitutionAttributes = load_constitution(Paths::resolve(Paths::CONSTITUTION_TABLE).string(), message_system);
	charismaAttributes = load_charisma(Paths::resolve(Paths::CHARISMA_TABLE).string(), message_system);
	intelligenceAttributes = load_intelligence(Paths::resolve(Paths::INTELLIGENCE_TABLE).string(), message_system);
	wisdomAttributes = load_wisdom(Paths::resolve(Paths::WISDOM_TABLE).string(), message_system);

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

std::vector<StrengthAttributes> DataManager::load_strength(const std::string& filename, MessageSystem& message_system)
{
	const nlohmann::json table = read_table(filename);

	std::vector<StrengthAttributes> data;
	for (const auto& item : table)
	{
		data.push_back(strength_row_from(item));
	}

	message_system.log(std::format("DataManager: Loaded {} strength attributes", data.size()));
	return data;
}

std::vector<DexterityAttributes> DataManager::load_dexterity(const std::string& filename, MessageSystem& message_system)
{
	const nlohmann::json table = read_table(filename);

	std::vector<DexterityAttributes> data;
	for (const auto& item : table)
	{
		data.push_back(dexterity_row_from(item));
	}

	message_system.log(std::format("DataManager: Loaded {} dexterity attributes", data.size()));
	return data;
}

std::vector<ConstitutionAttributes> DataManager::load_constitution(const std::string& filename, MessageSystem& message_system)
{
	const nlohmann::json table = read_table(filename);

	std::vector<ConstitutionAttributes> data;
	for (const auto& item : table)
	{
		data.push_back(constitution_row_from(item));
	}

	message_system.log(std::format("DataManager: Loaded {} constitution attributes", data.size()));
	return data;
}

std::vector<CharismaAttributes> DataManager::load_charisma(const std::string& filename, MessageSystem& message_system)
{
	const nlohmann::json table = read_table(filename);

	std::vector<CharismaAttributes> data;
	for (const auto& item : table)
	{
		data.push_back(charisma_row_from(item));
	}

	message_system.log(std::format("DataManager: Loaded {} charisma attributes", data.size()));
	return data;
}

std::vector<IntelligenceAttributes> DataManager::load_intelligence(const std::string& filename, MessageSystem& message_system)
{
	const nlohmann::json table = read_table(filename);

	std::vector<IntelligenceAttributes> data;
	for (const auto& item : table)
	{
		data.push_back(intelligence_row_from(item));
	}

	message_system.log(std::format("DataManager: Loaded {} intelligence attributes", data.size()));
	return data;
}

std::vector<WisdomAttributes> DataManager::load_wisdom(const std::string& filename, MessageSystem& message_system)
{
	const nlohmann::json table = read_table(filename);

	std::vector<WisdomAttributes> data;
	for (const auto& item : table)
	{
		data.push_back(wisdom_row_from(item));
	}

	message_system.log(std::format("DataManager: Loaded {} wisdom attributes", data.size()));
	return data;
}

// end of file: Systems/DataManager.cpp
