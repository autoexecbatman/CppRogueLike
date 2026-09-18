#pragma once

#include <string>
#include <vector>

#include "CharismaAttributes.h"
#include "ConstitutionAttributes.h"
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

private:
	// Data storage
	std::vector<Weapons> weapons;
	std::vector<StrengthAttributes> strengthAttributes;
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
