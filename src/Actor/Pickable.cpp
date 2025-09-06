// file: Pickable.cpp
#include <vector>

#include "Pickable.h"
#include "../Colors/Colors.h"
#include "../Game.h"
#include "Actor.h"
#include "Container.h"
#include "../Items/ItemClassification.h"
#include "../ActorTypes/Player.h"
#include "../ActorTypes/Healer.h"
#include "../ActorTypes/LightningBolt.h"
#include "../ActorTypes/Fireball.h"
#include "../ActorTypes/Confuser.h"
#include "../Ai/AiMonsterConfused.h"
#include "../ActorTypes/Gold.h"
#include "../Items/Food.h"
#include "../Items/CorpseFood.h"
#include "../Items/Amulet.h"
#include "../Items/Armor.h"

//==PICKABLE==
bool Pickable::use(Item& owner, Creature& wearer)
{
	if (wearer.container)
	{
		auto compareItems = [&owner](const std::unique_ptr<Item>& actor) { return actor.get() == &owner; };
		std::erase_if(wearer.container->get_inventory_mutable(), compareItems);

		return true;
	}

	return false;
}

// AD&D 2e weapon size and dual-wield validation methods
WeaponSize Weapon::get_weapon_size() const
{
	// Use the ItemClassificationUtils system by mapping PickableType to ItemClass
	// This is a bridge solution until the system is fully refactored
	PickableType type = get_type();
	
	// Use a simplified mapping approach for now
	// This bridges the PickableType system with proper AD&D 2e weapon sizes
	switch (type)
	{
		case PickableType::DAGGER: return WeaponSize::TINY;
		case PickableType::SHORTSWORD: return WeaponSize::SMALL;
		case PickableType::LONGSWORD: return WeaponSize::MEDIUM;
		case PickableType::BATTLE_AXE: return WeaponSize::MEDIUM;
		case PickableType::WAR_HAMMER: return WeaponSize::MEDIUM;
		case PickableType::GREATSWORD: return WeaponSize::LARGE;
		case PickableType::GREAT_AXE: return WeaponSize::LARGE;
		case PickableType::STAFF: return WeaponSize::LARGE;
		case PickableType::LONGBOW: return WeaponSize::LARGE;
		default: return WeaponSize::MEDIUM; // Safe fallback
	}
}

bool Weapon::can_be_off_hand(WeaponSize weaponSize) const
{
	// AD&D 2e rule: Off-hand weapons must be SMALL or TINY
	return weaponSize <= WeaponSize::SMALL;
}

bool Weapon::validate_dual_wield(Item* mainHandWeapon, Item* offHandWeapon) const
{
	if (!mainHandWeapon || !offHandWeapon)
	{
		return false;
	}
	
	// Use ItemClass system for weapon validation
	if (!mainHandWeapon->is_weapon() || !offHandWeapon->is_weapon())
	{
		return false; // One of them is not a weapon
	}
	
	// TODO: Implement proper weapon size validation using ItemClass
	// For now, allow any dual-wield combination
	return true;
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
	case PickableType::GREATSWORD:
		pickable = std::make_unique<Greatsword>();
		break;
	case PickableType::BATTLE_AXE:
		pickable = std::make_unique<BattleAxe>();
		break;
	case PickableType::GREAT_AXE:
		pickable = std::make_unique<GreatAxe>();
		break;
	case PickableType::WAR_HAMMER:
		pickable = std::make_unique<WarHammer>();
		break;
	case PickableType::SHIELD:
		pickable = std::make_unique<Shield>();
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

// Clean weapon equip/unequip logic - delegates to Equipment System
bool Weapon::use(Item& owner, Creature& wearer)
{
	std::string weaponName = owner.actorData.name;
	
	// For players, delegate to Equipment System using unique IDs
	if (wearer.uniqueId == game.player->uniqueId)
	{
		Player* player = static_cast<Player*>(&wearer);
		
		// Determine weapon type using the existing Pickable type system
		PickableType weaponType = owner.pickable->get_type();
		
		if (weaponType == PickableType::SHIELD)
		{
			bool wasEquipped = player->is_item_equipped(owner.uniqueId);
			bool success = player->toggle_shield(owner.uniqueId);
			
			if (success)
			{
				if (wasEquipped)
				{
					game.message(WHITE_BLACK_PAIR, "You unequip the " + weaponName + ".", true);
				}
				else
				{
					game.message(WHITE_BLACK_PAIR, "You equip the " + weaponName + ".", true);
				}
			}
			return success;
		}
		else
		{
			// Regular weapon
			bool wasEquipped = player->is_item_equipped(owner.uniqueId);
			bool success = player->toggle_weapon(owner.uniqueId);
			
			if (success)
			{
				if (wasEquipped)
				{
					game.message(WHITE_BLACK_PAIR, "You unequip the " + weaponName + ".", true);
					
					// Remove ranged state if applicable
					if (is_ranged())
					{
						wearer.remove_state(ActorState::IS_RANGED);
					}
				}
				else
				{
					game.message(WHITE_BLACK_PAIR, "You equip the " + weaponName + ".", true);
					
					// Apply ranged state if applicable
					if (is_ranged())
					{
						wearer.add_state(ActorState::IS_RANGED);
					}
					
					// Update attack roll if equipped in main hand
					Item* mainHandItem = player->get_equipped_item(EquipmentSlot::RIGHT_HAND);
					if (mainHandItem && mainHandItem->uniqueId == owner.uniqueId)
					{
						wearer.attacker->set_roll(this->roll);
					}
				}
			}
			return success;
		}
	}
	
	// For NPCs, use simple stat modification
	return Pickable::use(owner, wearer);
}

// Implement the Dagger class
bool Dagger::is_ranged() const
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
bool LongSword::is_ranged() const
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
bool ShortSword::is_ranged() const
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
bool Longbow::is_ranged() const
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
bool Staff::is_ranged() const
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

	// Implement the Greatsword class
	bool Greatsword::is_ranged() const
	{
		return false;
	}

	void Greatsword::save(json& j)
	{
		j["type"] = static_cast<int>(PickableType::GREATSWORD);
		j["roll"] = roll;
	}

	void Greatsword::load(const json& j)
	{
		if (j.contains("roll") && j["roll"].is_string()) {
			roll = j["roll"].get<std::string>();
		}
		else {
			throw std::runtime_error("Invalid JSON format for Greatsword");
		}
	}

	// Implement the BattleAxe class
	bool BattleAxe::is_ranged() const
	{
		return false;
	}

	void BattleAxe::save(json& j)
	{
		j["type"] = static_cast<int>(PickableType::BATTLE_AXE);
		j["roll"] = roll;
	}

	void BattleAxe::load(const json& j)
	{
		if (j.contains("roll") && j["roll"].is_string()) {
			roll = j["roll"].get<std::string>();
		}
		else {
			throw std::runtime_error("Invalid JSON format for BattleAxe");
		}
	}

	// Implement the GreatAxe class
	bool GreatAxe::is_ranged() const
	{
		return false;
	}

	void GreatAxe::save(json& j)
	{
		j["type"] = static_cast<int>(PickableType::GREAT_AXE);
		j["roll"] = roll;
	}

	void GreatAxe::load(const json& j)
	{
		if (j.contains("roll") && j["roll"].is_string()) {
			roll = j["roll"].get<std::string>();
		}
		else {
			throw std::runtime_error("Invalid JSON format for GreatAxe");
		}
	}

	// Implement the WarHammer class
	bool WarHammer::is_ranged() const
	{
		return false;
	}

	void WarHammer::save(json& j)
	{
		j["type"] = static_cast<int>(PickableType::WAR_HAMMER);
		j["roll"] = roll;
	}

	void WarHammer::load(const json& j)
	{
		if (j.contains("roll") && j["roll"].is_string()) {
			roll = j["roll"].get<std::string>();
		}
		else {
			throw std::runtime_error("Invalid JSON format for WarHammer");
		}
	}

	// Implement the Shield class
	bool Shield::is_ranged() const
	{
		return false;
	}

	void Shield::save(json& j)
	{
		j["type"] = static_cast<int>(PickableType::SHIELD);
		j["roll"] = roll;
	}

	void Shield::load(const json& j)
	{
		if (j.contains("roll") && j["roll"].is_string()) {
			roll = j["roll"].get<std::string>();
		}
		else {
			throw std::runtime_error("Invalid JSON format for Shield");
		}
	}