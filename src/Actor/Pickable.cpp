// file: Pickable.cpp
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
#include "../Food.h"
#include "../CorpseFood.h"
#include "../Amulet.h"
#include "../Armor.h"

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
	case PickableType::FOOD:
		pickable = std::make_unique<Food>(0);
		break;
	case PickableType::CORPSE_FOOD:
		pickable = std::make_unique<CorpseFood>(0);
		break;
	case PickableType::AMULET:
		pickable = std::make_unique<Amulet>();
		break;
	case PickableType::LEATHER_ARMOR:
		pickable = std::make_unique<LeatherArmor>();
		break;
	case PickableType::CHAIN_MAIL:
		pickable = std::make_unique<ChainMail>();
		break;
	case PickableType::PLATE_MAIL:
		pickable = std::make_unique<PlateMail>();
		break;
	default:
		throw std::runtime_error("Unknown PickableType");
	} // end of switch (type)switch (type)

	pickable->load(j);
	return pickable;
}

// Common weapon equip/unequip logic
bool Weapon::use(Item& owner, Creature& wearer)
{
	// If this is a direct unequip operation
	if (owner.has_state(ActorState::IS_EQUIPPED))
	{
		wearer.attacker->roll = "D2"; // Reset to default unarmed attack
		wearer.unequip(owner);
		game.message(WHITE_PAIR, "You unequip the " + owner.actorData.name + ".", true);

		// Remove ranged state if applicable
		if (isRanged())
		{
			wearer.remove_state(ActorState::IS_RANGED);
		}

		return false; // Don't consume the weapon
	}

	// Check if another weapon is already equipped
	Item* currentWeapon = nullptr;
	for (const auto& item : wearer.container->inv)
	{
		if (item && item->has_state(ActorState::IS_EQUIPPED) && item.get() != &owner)
		{
			// Check if it's a weapon
			Weapon* weapon = dynamic_cast<Weapon*>(item->pickable.get());
			if (weapon)
			{
				currentWeapon = item.get();
				break;
			}
		}
	}

	// If another weapon is equipped, unequip it
	if (currentWeapon)
	{
		game.message(WHITE_PAIR, "You unequip your " + currentWeapon->actorData.name + ".", true);
		wearer.unequip(*currentWeapon);
	}

	// Equip the new weapon
	wearer.attacker->roll = this->roll;
	wearer.equip(owner);
	game.message(WHITE_PAIR, "You equip the " + owner.actorData.name + ".", true);

	// Apply ranged state if applicable
	if (isRanged())
	{
		wearer.add_state(ActorState::IS_RANGED);
	}

	return false; // Don't consume the weapon
}

// Implement the Dagger class
bool Dagger::isRanged() const
{
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

// Implement the LongSword class
bool LongSword::isRanged() const
{
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

// Implement the ShortSword class
bool ShortSword::isRanged() const
{
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

// Implement the Longbow class
bool Longbow::isRanged() const
{
	return true;
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

// Implement the Staff class
bool Staff::isRanged() const
{
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