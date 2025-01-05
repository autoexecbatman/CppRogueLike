// file: Pickable.cpp
#include <gsl/util>
#include <vector>

#include "Pickable.h"
#include "../Colors/Colors.h"
#include "../Game.h"
#include "Actor.h"
#include "Confuser.h"
#include "Container.h"
#include "../ActorTypes/Healer.h"
#include "../ActorTypes/LightningBolt.h"
#include "../ActorTypes/Fireball.h"
#include "../Ai/AiMonsterConfused.h"
#include "../ActorTypes/Gold.h"

//==PICKABLE==
bool Pickable::use(Item& owner, Creature& wearer)
{
	if (wearer.container)
	{
		auto compareItems = [&owner](const std::unique_ptr<Item>& actor) { return actor.get() == &owner; };
		std::erase_if(wearer.container->inv, compareItems);

		return true;
	}

	return false;
}

std::unique_ptr<Pickable> Pickable::create(const json& j)
{
	if (!j.contains("type") || !j["type"].is_number()) {
		throw std::runtime_error("Invalid JSON format: Missing or invalid 'type'");
	}

	auto type = static_cast<PickableType>(j["type"].get<int>());
	std::unique_ptr<Pickable> pickable;

	switch (type)
	{
	case PickableType::HEALER:
		pickable = std::make_unique<Healer>(0);
		break;
	case PickableType::LIGHTNING_BOLT:
		pickable = std::make_unique<LightningBolt>(0, 0);
		break;
	case PickableType::CONFUSER:
		pickable = std::make_unique<Confuser>(0, 0);
		break;
	case PickableType::FIREBALL:
		pickable = std::make_unique<Fireball>(0, 0);
		break;
	case PickableType::DAGGER:
		pickable = std::make_unique<Dagger>();
		break;
	case PickableType::LONGSWORD:
		pickable = std::make_unique<LongSword>();
		break;
	case PickableType::SHORTSWORD:
		pickable = std::make_unique<ShortSword>();
		break;
	case PickableType::LONGBOW:
		pickable = std::make_unique<Longbow>();
		break;
	case PickableType::STAFF:
		pickable = std::make_unique<Staff>();
		break;
	case PickableType::GOLD:
		pickable = std::make_unique<Gold>(0);
		break;
	default:
		throw std::runtime_error("Unknown PickableType");
	} // end of switch (type)

	pickable->load(j);
	return pickable;
}

bool Dagger::use(Item& owner, Creature& wearer)
{
	// equip the weapon
	if (!owner.has_state(ActorState::IS_EQUIPPED))
	{
		wearer.attacker->roll = this->roll;
		wearer.equip(owner);
	}
	// unequip the weapon
	else
	{
		wearer.attacker->roll = this->roll;
		wearer.unequip(owner);
	}

	return false;
}

void Dagger::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::DAGGER); // Save the type
	j["roll"] = roll; // Save the roll string
}

void Dagger::load(const json& j)
{
	if (j.contains("roll") && j["roll"].is_string()) {
		roll = j["roll"].get<std::string>(); // Load the roll string
	}
	else {
		throw std::runtime_error("Invalid JSON format for Dagger");
	}
}

bool LongSword::use(Item& owner, Creature& wearer)
{
	// equip the weapon
	if (!owner.has_state(ActorState::IS_EQUIPPED))
	{
		wearer.attacker->roll = this->roll;
		wearer.equip(owner);
	}
	else
	{
		wearer.attacker->roll = this->roll;
		wearer.unequip(owner);
	}

	return false;
}

void LongSword::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::LONGSWORD);
	j["roll"] = roll;
}

void LongSword::load(const json& j)
{
	if (j.contains("roll") && j["roll"].is_string()) {
		roll = j["roll"].get<std::string>();
	}
	else {
		throw std::runtime_error("Invalid JSON format for LongSword");
	}
}

// end of file: Pickable.cpp

bool ShortSword::use(Item& owner, Creature& wearer)
{
	// equip the weapon
	if (!owner.has_state(ActorState::IS_EQUIPPED))
	{
		wearer.attacker->roll = this->roll;
		wearer.equip(owner);
	}
	// unequip the weapon
	else
	{
		wearer.attacker->roll = this->roll;
		wearer.unequip(owner);
	}

	return false;
}

void ShortSword::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::SHORTSWORD);
	j["roll"] = roll;
}

void ShortSword::load(const json& j)
{
	if (j.contains("roll") && j["roll"].is_string()) {
		roll = j["roll"].get<std::string>();
	}
	else {
		throw std::runtime_error("Invalid JSON format for ShortSword");
	}
}

bool Longbow::use(Item& owner, Creature& wearer)
{
	// equip the weapon
	if (!owner.has_state(ActorState::IS_EQUIPPED))
	{
		wearer.attacker->roll = this->roll;
		wearer.equip(owner);
	}
	// unequip the weapon
	else
	{
		wearer.attacker->roll = this->roll;
		wearer.unequip(owner);
	}
	return false;
}

void Longbow::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::LONGBOW);
	j["roll"] = roll;
}

void Longbow::load(const json& j)
{
	if (j.contains("roll") && j["roll"].is_string()) {
		roll = j["roll"].get<std::string>();
	}
	else {
		throw std::runtime_error("Invalid JSON format for Longbow");
	}
}

bool Staff::use(Item& owner, Creature& wearer)
{
	// equip the weapon
	if (!owner.has_state(ActorState::IS_EQUIPPED))
	{
		wearer.attacker->roll = this->roll;
		wearer.equip(owner);
	}
	// unequip the weapon
	else
	{
		wearer.attacker->roll = this->roll;
		wearer.unequip(owner);
	}
	return false;
}

void Staff::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::STAFF);
	j["roll"] = roll;
}

void Staff::load(const json& j)
{
	if (j.contains("roll") && j["roll"].is_string()) {
		roll = j["roll"].get<std::string>();
	}
	else {
		throw std::runtime_error("Invalid JSON format for Staff");
	}
}
