#pragma once

#include <string>
#include <vector>

#include "CharismaAttributes.h"
#include "ConstitutionAttributes.h"
#include "CreatureClass.h"
#include "DexterityAttributes.h"
#include "IntelligenceAttributes.h"
#include "StrengthAttributes.h"
#include "WisdomAttributes.h"
#include "Weapons.h"

class MessageSystem;

class DataManager
{
public:
	// Load all game data
	void load_all_data(MessageSystem& message_system);

	// Accessors for loaded data
	const std::vector<Weapons>& get_weapons() const { return weapons; }
	const std::vector<StrengthAttributes>& get_strength_attributes() const { return strengthAttributes; }
	const std::vector<DexterityAttributes>& get_dexterity_attributes() const { return dexterityAttributes; }
	const std::vector<ConstitutionAttributes>& get_constitution_attributes() const { return constitutionAttributes; }
	const std::vector<CharismaAttributes>& get_charisma_attributes() const { return charismaAttributes; }
	const std::vector<IntelligenceAttributes>& get_intelligence_attributes() const { return intelligenceAttributes; }
	const std::vector<WisdomAttributes>& get_wisdom_attributes() const { return wisdomAttributes; }

	// The Table 2 row a Dexterity score reads. A score past the table's end - which no
	// ability reaches, the books capping every score at 25 - reads its last row; a score
	// below 1, which only a creature never given one has, reads as no adjustment.
	//
	// Example:
	//   dataManager.dexterity_for(17).DefensiveAdj;   // -> -3
	//   dataManager.dexterity_for(26).ReactionAdj;    // -> 5, the 25 row
	//   dataManager.dexterity_for(0).DefensiveAdj;    // -> 0
	[[nodiscard]] DexterityAttributes dexterity_for(int score) const;

	// The Table 1 row a Strength score reads, with the same edges as dexterity_for. At
	// Strength 18 an exceptional percentile of 1-100 reads the 18/xx band holding it;
	// at any other score it is ignored, as the book gives it only to an 18.
	//
	// Example:
	//   dataManager.strength_for(17, 0).dmgAdj;     // -> 1
	//   dataManager.strength_for(18, 76).dmgAdj;    // -> 4, 18/76-90
	//   dataManager.strength_for(18, 100).hitProb;  // -> 3, 18/00
	//   dataManager.strength_for(19, 100).dmgAdj;   // -> 7, the percentile ignored
	[[nodiscard]] StrengthAttributes strength_for(int score, int exceptional) const;

	// The Table 3 row a Constitution score reads, with the same edges as dexterity_for.
	// Its hit point column is the warrior's; constitution_hit_point_adjustment caps it for
	// other classes, and is what a hit point reader asks.
	//
	// Example:
	//   dataManager.constitution_for(17).HPAdj;   // -> 3
	//   dataManager.constitution_for(20).HPAdj;   // -> 5
	//   dataManager.constitution_for(21).hitDieMinimum;   // -> 3, a rolled 1 or 2 counts as 3
	[[nodiscard]] ConstitutionAttributes constitution_for(int score) const;

	// The hit points a Constitution score adds to each hit die, by class. Table 3's own
	// footnote: the bonus above +2 belongs to warriors, every other class stops at +2, and
	// a penalty is never capped. The score reads with constitution_for's edges.
	//
	// Example:
	//   dataManager.constitution_hit_point_adjustment(17, CreatureClass::FIGHTER); // -> 3
	//   dataManager.constitution_hit_point_adjustment(17, CreatureClass::WIZARD); // -> 2
	//   dataManager.constitution_hit_point_adjustment(3, CreatureClass::WIZARD); // -> -2
	[[nodiscard]] int constitution_hit_point_adjustment(int score, CreatureClass creatureClass) const;

private:
	// Data storage
	std::vector<Weapons> weapons;
	std::vector<StrengthAttributes> strengthAttributes;
	std::vector<StrengthAttributes> exceptionalStrengthBands;
	std::vector<DexterityAttributes> dexterityAttributes;
	std::vector<ConstitutionAttributes> constitutionAttributes;
	std::vector<CharismaAttributes> charismaAttributes;
	std::vector<IntelligenceAttributes> intelligenceAttributes;
	std::vector<WisdomAttributes> wisdomAttributes;

	// Simple JSON loading functions
	std::vector<Weapons> load_weapons(const std::string& filename, MessageSystem& message_system);
	std::vector<StrengthAttributes> load_strength(const std::string& filename, MessageSystem& message_system);
	std::vector<DexterityAttributes> load_dexterity(const std::string& filename, MessageSystem& message_system);
	std::vector<ConstitutionAttributes> load_constitution(const std::string& filename, MessageSystem& message_system);
	std::vector<CharismaAttributes> load_charisma(const std::string& filename, MessageSystem& message_system);
	std::vector<IntelligenceAttributes> load_intelligence(const std::string& filename, MessageSystem& message_system);
	std::vector<WisdomAttributes> load_wisdom(const std::string& filename, MessageSystem& message_system);
};
