// file: Pickable.cpp
#include <vector>

#include "Pickable.h"
#include "../Colors/Colors.h"
#include "Actor.h"
#include "InventoryData.h"
#include "InventoryOperations.h"
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
#include "../Core/GameContext.h"
#include "../Systems/MessageSystem.h"

using namespace InventoryOperations; // For clean function calls

//==PICKABLE==
bool Pickable::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	auto result = remove_item(wearer.inventory_data, owner);
	return result.has_value();
}

// Weapon equipping logic - uses smart slot system
bool Weapon::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	// For players, use the slot-based equipment system with smart slot selection
	if (wearer.uniqueId == ctx.player->uniqueId)
	{
		Player* player = static_cast<Player*>(&wearer);

		// Get the preferred slot for this weapon type based on current equipment
		EquipmentSlot preferredSlot = get_preferred_slot(player);

		// Use toggle_weapon with preferred slot
		bool success = player->toggle_weapon(owner.uniqueId, preferredSlot, ctx);

		if (success)
		{
			// Check if weapon is now equipped in preferred slot
			Item* equippedWeapon = player->get_equipped_item(preferredSlot);
			if (equippedWeapon && equippedWeapon->uniqueId == owner.uniqueId)
			{
				std::string slotName = (preferredSlot == EquipmentSlot::LEFT_HAND) ? "off-hand" : "main hand";
				ctx.message_system->message(WHITE_BLACK_PAIR, "You equip the " + owner.get_name() + " in your " + slotName + ".", true);
			}
			else
			{
				ctx.message_system->message(WHITE_BLACK_PAIR, "You unequip the " + owner.get_name() + ".", true);
			}
		}

		return success;
	}

	// For NPCs, use simple equip toggle
	wearer.equip(owner, ctx);
	return true;
}

// Default weapon slot selection - main hand
EquipmentSlot Weapon::get_preferred_slot(const Player* player) const
{
	return EquipmentSlot::RIGHT_HAND;
}

// Shield equipping logic - uses modern slot-based system
bool Shield::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	// For players, use the proper slot-based equipment system
	if (wearer.uniqueId == ctx.player->uniqueId)
	{
		Player* player = static_cast<Player*>(&wearer);

		// Use the dedicated toggle_shield method
		bool success = player->toggle_shield(owner.uniqueId, ctx);

		if (success)
		{
			// Check if shield is now equipped by looking in LEFT_HAND slot
			Item* equippedShield = player->get_equipped_item(EquipmentSlot::LEFT_HAND);
			if (equippedShield && equippedShield->uniqueId == owner.uniqueId)
			{
				ctx.message_system->message(WHITE_BLACK_PAIR, "You raise the " + owner.get_name() + ".", true);
			}
			else
			{
				ctx.message_system->message(WHITE_BLACK_PAIR, "You lower the " + owner.get_name() + ".", true);
			}
		}

		return success;
	}

	// For NPCs, use simple equip toggle
	wearer.equip(owner, ctx);
	return true;
}

// AD&D 2e weapon size and dual-wield validation methods
WeaponSize Weapon::get_weapon_size() const
{
	PickableType type = get_type();
	
	switch (type)
	{
		case PickableType::DAGGER: return WeaponSize::TINY;
		case PickableType::SHORTSWORD: return WeaponSize::SMALL;
		case PickableType::LONGSWORD: return WeaponSize::MEDIUM;
		case PickableType::BATTLE_AXE: return WeaponSize::MEDIUM;
		case PickableType::WAR_HAMMER: return WeaponSize::MEDIUM;
		case PickableType::MACE: return WeaponSize::MEDIUM;
		case PickableType::GREATSWORD: return WeaponSize::LARGE;
		case PickableType::GREAT_AXE: return WeaponSize::LARGE;
		case PickableType::STAFF: return WeaponSize::LARGE;
		case PickableType::LONGBOW: return WeaponSize::LARGE;
		default: return WeaponSize::MEDIUM; // Safe fallback
	}
}

bool Weapon::can_be_off_hand(WeaponSize weaponSize) const
{
	return weaponSize <= WeaponSize::SMALL;
}

bool Weapon::validate_dual_wield(Item* mainHandWeapon, Item* offHandWeapon) const
{
	if (!mainHandWeapon || !offHandWeapon)
	{
		return false;
	}
	
	if (!mainHandWeapon->is_weapon() || !offHandWeapon->is_weapon())
	{
		return false;
	}
	
	return true;
}

std::unique_ptr<Pickable> Pickable::create(const json& j)
{
	if (!j.contains("type") || !j["type"].is_number()) 
	{
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
	case PickableType::MACE:
		pickable = std::make_unique<Mace>();
		break;
	case PickableType::SHIELD:
		pickable = std::make_unique<Shield>();
		break;
	// Potions
	case PickableType::HEALING_POTION:
		pickable = std::make_unique<HealingPotion>();
		break;
	case PickableType::MANA_POTION:
		pickable = std::make_unique<ManaPotion>();
		break;
	case PickableType::STRENGTH_POTION:
		pickable = std::make_unique<StrengthPotion>();
		break;
	case PickableType::SPEED_POTION:
		pickable = std::make_unique<SpeedPotion>();
		break;
	case PickableType::POISON_ANTIDOTE:
		pickable = std::make_unique<PoisonAntidote>();
		break;
	case PickableType::FIRE_RESISTANCE_POTION:
		pickable = std::make_unique<FireResistancePotion>();
		break;
	case PickableType::INVISIBILITY_POTION:
		pickable = std::make_unique<InvisibilityPotion>();
		break;
	// Scrolls
	case PickableType::SCROLL_IDENTIFY:
		pickable = std::make_unique<ScrollIdentify>();
		break;
	case PickableType::SCROLL_TELEPORT:
		pickable = std::make_unique<ScrollTeleport>();
		break;
	case PickableType::SCROLL_MAGIC_MAPPING:
		pickable = std::make_unique<ScrollMagicMapping>();
		break;
	case PickableType::SCROLL_ENCHANTMENT:
		pickable = std::make_unique<ScrollEnchantment>();
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
	}

	pickable->load(j);
	return pickable;
}

// Implement the Dagger class
bool Dagger::is_ranged() const
{
	return false;
}

void Dagger::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::DAGGER);
	j["roll"] = roll;
}

void Dagger::load(const json& j)
{
	if (j.contains("roll") && j["roll"].is_string()) 
	{
		roll = j["roll"].get<std::string>();
	}
	else 
	{
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
	if (j.contains("roll") && j["roll"].is_string()) 
	{
		roll = j["roll"].get<std::string>();
	}
	else 
	{
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
	if (j.contains("roll") && j["roll"].is_string()) 
	{
		roll = j["roll"].get<std::string>();
	}
	else 
	{
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
	if (j.contains("roll") && j["roll"].is_string()) 
	{
		roll = j["roll"].get<std::string>();
	}
	else 
	{
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
	if (j.contains("roll") && j["roll"].is_string()) 
	{
		roll = j["roll"].get<std::string>();
	}
	else 
	{
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
	if (j.contains("roll") && j["roll"].is_string()) 
	{
		roll = j["roll"].get<std::string>();
	}
	else 
	{
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
	if (j.contains("roll") && j["roll"].is_string()) 
	{
		roll = j["roll"].get<std::string>();
	}
	else 
	{
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
	if (j.contains("roll") && j["roll"].is_string()) 
	{
		roll = j["roll"].get<std::string>();
	}
	else 
	{
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
	if (j.contains("roll") && j["roll"].is_string()) 
	{
		roll = j["roll"].get<std::string>();
	}
	else 
	{
		throw std::runtime_error("Invalid JSON format for WarHammer");
	}
}

// Implement the Mace class
bool Mace::is_ranged() const
{
	return false;
}

void Mace::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::MACE);
	j["roll"] = roll;
}

void Mace::load(const json& j)
{
	if (j.contains("roll") && j["roll"].is_string()) 
	{
		roll = j["roll"].get<std::string>();
	}
	else 
	{
		throw std::runtime_error("Invalid JSON format for Mace");
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
	if (j.contains("roll") && j["roll"].is_string()) 
	{
		roll = j["roll"].get<std::string>();
	}
	else 
	{
		throw std::runtime_error("Invalid JSON format for Shield");
	}
}

// Smart dagger slot selection logic
EquipmentSlot Dagger::get_preferred_slot(const Player* player) const
{
	// Check what's currently equipped in main hand
	Item* mainHandWeapon = player->get_equipped_item(EquipmentSlot::RIGHT_HAND);
	Item* offHandItem = player->get_equipped_item(EquipmentSlot::LEFT_HAND);
	
	// If nothing in main hand, equip there first (daggers can be main weapons)
	if (!mainHandWeapon)
	{
		return EquipmentSlot::RIGHT_HAND;
	}
	
	// If main hand has a larger weapon and off-hand is free, use off-hand
	if (!offHandItem && mainHandWeapon->is_weapon())
	{
		// Get the main hand weapon's size for comparison
		Pickable* mainPickable = mainHandWeapon->pickable.get();
		if (Weapon* mainWeapon = dynamic_cast<Weapon*>(mainPickable))
		{
			WeaponSize mainSize = mainWeapon->get_weapon_size();
			
			// If main weapon is larger than TINY, dagger should go in off-hand
			if (mainSize > WeaponSize::TINY)
			{
				return EquipmentSlot::LEFT_HAND;
			}
		}
	}
	
	// Default to main hand (will replace current weapon or go to off-hand via toggle logic)
	return EquipmentSlot::RIGHT_HAND;
}

// Potion implementations
bool HealingPotion::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	if (wearer.destructible && wearer.destructible->get_hp() < wearer.destructible->get_max_hp())
	{
		int healedAmount = std::min(heal_amount, wearer.destructible->get_max_hp() - wearer.destructible->get_hp());
		wearer.destructible->heal(healedAmount);
		ctx.message_system->message(GREEN_BLACK_PAIR, "You feel better! (+" + std::to_string(healedAmount) + " HP)", true);
		
		// Remove the potion from inventory
		auto result = InventoryOperations::remove_item(wearer.inventory_data, owner);
		return result.has_value();
	}
	else
	{
		ctx.message_system->message(WHITE_BLACK_PAIR, "You are already at full health.", true);
		return false;
	}
}

void HealingPotion::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::HEALING_POTION);
	j["heal_amount"] = heal_amount;
}

void HealingPotion::load(const json& j)
{
	if (j.contains("heal_amount") && j["heal_amount"].is_number())
	{
		heal_amount = j["heal_amount"].get<int>();
	}
	else
	{
		heal_amount = 20; // Default value
	}
}

bool ManaPotion::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	// For now, just show a message (mana system not implemented)
	ctx.message_system->message(BLUE_BLACK_PAIR, "You feel magical energy surge through you! (+" + std::to_string(mana_amount) + " Mana)", true);
	
	// Remove the potion from inventory
	auto result = InventoryOperations::remove_item(wearer.inventory_data, owner);
	return result.has_value();
}

void ManaPotion::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::MANA_POTION);
	j["mana_amount"] = mana_amount;
}

void ManaPotion::load(const json& j)
{
	if (j.contains("mana_amount") && j["mana_amount"].is_number())
	{
		mana_amount = j["mana_amount"].get<int>();
	}
	else
	{
		mana_amount = 15; // Default value
	}
}

bool StrengthPotion::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	// For now, just show a message (buff system not fully implemented)
	ctx.message_system->message(RED_BLACK_PAIR, "You feel much stronger! (+" + std::to_string(strength_bonus) + " Strength for " + std::to_string(duration) + " turns)", true);
	
	// Remove the potion from inventory
	auto result = InventoryOperations::remove_item(wearer.inventory_data, owner);
	return result.has_value();
}

void StrengthPotion::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::STRENGTH_POTION);
	j["strength_bonus"] = strength_bonus;
	j["duration"] = duration;
}

void StrengthPotion::load(const json& j)
{
	if (j.contains("strength_bonus") && j["strength_bonus"].is_number())
	{
		strength_bonus = j["strength_bonus"].get<int>();
	}
	if (j.contains("duration") && j["duration"].is_number())
	{
		duration = j["duration"].get<int>();
	}
}

bool SpeedPotion::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	// For now, just show a message (speed system not fully implemented)
	ctx.message_system->message(YELLOW_BLACK_PAIR, "You feel much faster! (+" + std::to_string(speed_bonus) + " Speed for " + std::to_string(duration) + " turns)", true);
	
	// Remove the potion from inventory
	auto result = InventoryOperations::remove_item(wearer.inventory_data, owner);
	return result.has_value();
}

void SpeedPotion::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::SPEED_POTION);
	j["speed_bonus"] = speed_bonus;
	j["duration"] = duration;
}

void SpeedPotion::load(const json& j)
{
	if (j.contains("speed_bonus") && j["speed_bonus"].is_number())
	{
		speed_bonus = j["speed_bonus"].get<int>();
	}
	if (j.contains("duration") && j["duration"].is_number())
	{
		duration = j["duration"].get<int>();
	}
}

bool PoisonAntidote::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	// For now, just show a message (poison system not fully implemented)
	ctx.message_system->message(GREEN_BLACK_PAIR, "You feel the poison leave your system!", true);
	
	// Remove the potion from inventory
	auto result = InventoryOperations::remove_item(wearer.inventory_data, owner);
	return result.has_value();
}

void PoisonAntidote::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::POISON_ANTIDOTE);
}

void PoisonAntidote::load(const json& j)
{
	// No additional data to load
}

bool FireResistancePotion::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	// For now, just show a message (resistance system not fully implemented)
	ctx.message_system->message(YELLOW_BLACK_PAIR, "You feel protected from fire! (" + std::to_string(resistance_amount) + "% resistance for " + std::to_string(duration) + " turns)", true);
	
	// Remove the potion from inventory
	auto result = InventoryOperations::remove_item(wearer.inventory_data, owner);
	return result.has_value();
}

void FireResistancePotion::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::FIRE_RESISTANCE_POTION);
	j["resistance_amount"] = resistance_amount;
	j["duration"] = duration;
}

void FireResistancePotion::load(const json& j)
{
	if (j.contains("resistance_amount") && j["resistance_amount"].is_number())
	{
		resistance_amount = j["resistance_amount"].get<int>();
	}
	if (j.contains("duration") && j["duration"].is_number())
	{
		duration = j["duration"].get<int>();
	}
}

bool InvisibilityPotion::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	wearer.set_invisible(duration);
	ctx.message_system->message(CYAN_BLACK_PAIR, "You fade from view! (Invisible for " + std::to_string(duration) + " turns)", true);

	// Remove the potion from inventory
	auto result = InventoryOperations::remove_item(wearer.inventory_data, owner);
	return result.has_value();
}

void InvisibilityPotion::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::INVISIBILITY_POTION);
	j["duration"] = duration;
}

void InvisibilityPotion::load(const json& j)
{
	if (j.contains("duration") && j["duration"].is_number())
	{
		duration = j["duration"].get<int>();
	}
	else
	{
		duration = 30; // Default value
	}
}

// Scroll implementations
bool ScrollIdentify::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	// For now, show a message (identify system not fully implemented)
	ctx.message_system->message(CYAN_BLACK_PAIR, "You feel a surge of knowledge! Items around you reveal their secrets.", true);
	
	// Remove the scroll from inventory
	auto result = InventoryOperations::remove_item(wearer.inventory_data, owner);
	return result.has_value();
}

void ScrollIdentify::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::SCROLL_IDENTIFY);
}

void ScrollIdentify::load(const json& j)
{
	// No additional data to load
}

bool ScrollTeleport::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	// For now, show a message (teleport system not fully implemented)
	ctx.message_system->message(CYAN_BLACK_PAIR, "Reality bends around you! You feel yourself being transported (Range: " + std::to_string(range) + " tiles).", true);
	
	// Remove the scroll from inventory
	auto result = InventoryOperations::remove_item(wearer.inventory_data, owner);
	return result.has_value();
}

void ScrollTeleport::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::SCROLL_TELEPORT);
	j["range"] = range;
}

void ScrollTeleport::load(const json& j)
{
	if (j.contains("range") && j["range"].is_number())
	{
		range = j["range"].get<int>();
	}
	else
	{
		range = 10; // Default value
	}
}

bool ScrollMagicMapping::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	// For now, show a message (magic mapping not fully implemented)
	ctx.message_system->message(YELLOW_BLACK_PAIR, "The layout of the area becomes clear in your mind! (Radius: " + std::to_string(radius) + " tiles)", true);
	
	// Remove the scroll from inventory
	auto result = InventoryOperations::remove_item(wearer.inventory_data, owner);
	return result.has_value();
}

void ScrollMagicMapping::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::SCROLL_MAGIC_MAPPING);
	j["radius"] = radius;
}

void ScrollMagicMapping::load(const json& j)
{
	if (j.contains("radius") && j["radius"].is_number())
	{
		radius = j["radius"].get<int>();
	}
	else
	{
		radius = 25; // Default value
	}
}

bool ScrollEnchantment::use(Item& owner, Creature& wearer, GameContext& ctx)
{
	// BROKEN LOGIC FIXED: Prevent scroll consumption without effect
	ctx.message_system->message(RED_BLACK_PAIR, "The scroll crumbles to dust - enchantment magic is not yet stable in this realm.", true);
	ctx.message_system->message(WHITE_BLACK_PAIR, "Enhancement system requires implementation before scrolls function.", true);

	// Do NOT consume the scroll until enhancement system is implemented
	// This prevents players from losing items to broken functionality
	return false; // Indicates use failed, item remains in inventory
}

void ScrollEnchantment::save(json& j)
{
	j["type"] = static_cast<int>(PickableType::SCROLL_ENCHANTMENT);
	j["enhancement_bonus"] = enhancement_bonus;
}

void ScrollEnchantment::load(const json& j)
{
	if (j.contains("enhancement_bonus") && j["enhancement_bonus"].is_number())
	{
		enhancement_bonus = j["enhancement_bonus"].get<int>();
	}
	else
	{
		enhancement_bonus = 1; // Default value
	}
}