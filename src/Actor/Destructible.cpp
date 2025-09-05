// file: Destructible.cpp
#include <iostream>
#include <string>
#include <algorithm>

#include "../Game.h"
#include "../Items/Items.h"
#include "../Actor/Actor.h"
#include "../Colors/Colors.h"
#include "../Attributes/StrengthAttributes.h"
#include "../ActorTypes/Healer.h"
#include "../Items/CorpseFood.h"
#include "../Items/Armor.h"
#include "../ActorTypes/Player.h"
#include "Pickable.h"

//====
Destructible::Destructible(int hpMax, int dr, std::string_view corpseName, int xp, int thaco, int armorClass)
	:
	hpMax(hpMax),
	hp(hpMax),
	hpBase(hpMax),
	dr(dr),
	corpseName(corpseName),
	xp(xp),
	thaco(thaco),
	armorClass(armorClass),
	baseArmorClass(armorClass),
	lastConstitution(0)
{}

void Destructible::update_constitution_bonus(Creature& owner)
{
	// Check if Constitution has changed since last update
	if (owner.constitution == get_last_constitution())
	{
		return; // No need to recalculate
	}

	// Get the Constitution bonus from the table
	int hpAdj = 0;
	const auto& constitutionAttributes = game.data_manager.get_constitution_attributes();
	if (owner.constitution >= 1 && owner.constitution <= constitutionAttributes.size())
	{
		hpAdj = constitutionAttributes[owner.constitution - 1].HPAdj;
	}

	// Calculate new max HP based on base HP and Constitution bonus
	int oldHpMax = get_max_hp(); // Store old max HP for comparison
	int newHpMax = get_hp_base() + (hpAdj * (dynamic_cast<Player*>(&owner) ? dynamic_cast<Player*>(&owner)->playerLevel : 1)); // Scale bonus by level for player

	// Only update if the calculated value is different AND we're not in a level-up situation
	// During level-ups, hpMax is already correctly set by the level-up system
	if (newHpMax != oldHpMax && newHpMax > 0)
	{
		// Check if this looks like a level-up scenario (hpMax increased recently)
		// If hpMax > hpBase + constitution bonus, then level-up system already handled it
		if (get_max_hp() <= get_hp_base() + (hpAdj * (dynamic_cast<Player*>(&owner) ? dynamic_cast<Player*>(&owner)->playerLevel : 1)))
		{
			set_max_hp(newHpMax);
			
			// Adjust current HP proportionally if max HP changed
			float hpRatio = static_cast<float>(get_hp()) / oldHpMax;
			int newHp = static_cast<int>(hpRatio * get_max_hp());

			// Make sure HP is at least 1
			newHp = std::max(1, newHp);

			// Don't exceed new maximum
			newHp = std::min(newHp, get_max_hp());
			
			set_hp(newHp);

			// Log the HP adjustment if it's the player
			if (&owner == game.player.get())
			{
				if (oldHpMax < get_max_hp())
				{
					game.log("Your Constitution bonus increased your hit points.");
				}
				else if (oldHpMax > get_max_hp())
				{
					game.log("Your Constitution bonus decreased your hit points.");
				}
			}
		}
	}

	if (get_max_hp() <= 0)
	{
		set_max_hp(1);
	}

	// Update the last known Constitution value
	set_last_constitution(owner.constitution);
}

void Destructible::take_damage(Creature& owner, int damage)
{
	// check if damage is greater than 0
	// if it is, then apply the damage to the actor
	if (damage > 0)
	{
		set_hp(get_hp() - damage);
		if (get_hp() <= 0)
		{
			owner.destructible->die(owner);
		}
	}
	else
	{
		damage = 0;
	}
}

// Transform the actor into a corpse !
void Destructible::die(Creature& owner)
{
	// copy data to new entity of type Item
	auto corpse = std::make_unique<Item>(owner.position, owner.actorData);
	corpse->actorData.name = get_corpse_name();
	corpse->actorData.ch = '%';

	// Replace this line:
	// corpse->pickable = std::make_unique<Healer>(10);
	// With this:
	corpse->pickable = std::make_unique<CorpseFood>(0); // 0 means calculate from type

	// DEFERRED CLEANUP: Don't remove immediately, let the game loop handle it
	// Mark the creature for removal instead of removing it immediately
	// This prevents dangling references during combat
	
	// Add the corpse to the game
	game.container->add(std::move(corpse));
	
	// Note: The creature will be removed from game.creatures later
	// when it's safe to do so (e.g., at the end of the turn)
}

//====

// The function returns the amount of health point actually restored.
int Destructible::heal(int hpToHeal)
{
	int newHp = get_hp() + hpToHeal;

	if (newHp > get_max_hp())
	{
		hpToHeal -= newHp - get_max_hp();
		newHp = get_max_hp();
	}
	
	set_hp(newHp);
	return hpToHeal;
}

void Destructible::update_armor_class(Creature& owner)
{
	// Start with the base armor class
	int calculatedAC = get_base_armor_class();

	// Apply Dexterity's Defensive Adjustment
	const auto& dexterityAttributes = game.data_manager.get_dexterity_attributes();
	if (owner.dexterity > 0 && owner.dexterity <= dexterityAttributes.size())
	{
		int defensiveAdj = dexterityAttributes[owner.dexterity - 1].DefensiveAdj;
		// DefensiveAdj is applied directly to AC
		// Note: In AD&D, a negative DefensiveAdj value actually improves AC
		calculatedAC += defensiveAdj;

		// Log the Defensive Adjustment if it's the player
		if (&owner == game.player.get() && defensiveAdj != 0)
		{
			game.log("Applied Dexterity Defensive Adjustment: " + std::to_string(defensiveAdj) +
				" to AC. Base AC: " + std::to_string(get_base_armor_class()) +
				", New AC: " + std::to_string(calculatedAC));
		}
	}

	// Check for Player's new equipment system first
	if (auto* player = dynamic_cast<Player*>(&owner))
	{
		// Use new equipment system for players
		Item* equippedArmor = player->get_equipped_item(EquipmentSlot::BODY);
		if (equippedArmor)
		{
			if (auto armor = dynamic_cast<Armor*>(equippedArmor->pickable.get()))
			{
				// Add the armor's AC bonus
				int armorBonus = armor->getArmorClass();
				calculatedAC += armorBonus;

				// Log the armor bonus if it's the player
				if (&owner == game.player.get())
				{
					game.log("Applied armor bonus: " + std::to_string(armorBonus) +
						" from " + equippedArmor->actorData.name);
				}
			}
		}
		
		// Check for shield in left hand
		Item* equippedShield = player->get_equipped_item(EquipmentSlot::LEFT_HAND);
		if (equippedShield)
		{
			if (auto shield = dynamic_cast<Shield*>(equippedShield->pickable.get()))
			{
				// Add the shield's AC bonus
				int shieldBonus = shield->get_ac_bonus();
				calculatedAC += shieldBonus;

				// Log the shield bonus if it's the player
				if (&owner == game.player.get())
				{
					game.log("Applied shield bonus: " + std::to_string(shieldBonus) +
						" from " + equippedShield->actorData.name);
				}
			}
		}
	}
	else if (owner.container)
	{
		// Use old system for non-player creatures
		// Check all items for equipped armor
		for (const auto& item : game.container->get_inventory_mutable())
		{
			if (item && item->has_state(ActorState::IS_EQUIPPED))
			{
				// Check if it's armor
				if (auto armor = dynamic_cast<Armor*>(item->pickable.get()))
				{
					// Add the armor's AC bonus
					int armorBonus = armor->getArmorClass();
					calculatedAC += armorBonus;

					// Log the armor bonus
					game.log("Applied armor bonus: " + std::to_string(armorBonus) +
						" from " + item->actorData.name);
				}
			}
		}
	}

	// Update the armor class only if it has changed
	if (get_armor_class() != calculatedAC)
	{
		int oldAC = get_armor_class();
		set_armor_class(calculatedAC);

		// Log the AC change if it's the player
		if (&owner == game.player.get())
		{
			game.log("Updated Armor Class from " + std::to_string(oldAC) +
				" to " + std::to_string(get_armor_class()));
		}
	}
}

void Destructible::load(const json& j)
{
	set_max_hp(j.at("hpMax").get<int>());
	set_hp(j.at("hp").get<int>());
	set_hp_base(j.at("hpBase").get<int>());
	set_last_constitution(j.at("lastConstitution").get<int>());
	set_dr(j.at("dr").get<int>());
	set_corpse_name(j.at("corpseName").get<std::string>());
	set_xp(j.at("xp").get<int>());
	set_thaco(j.at("thaco").get<int>());
	set_armor_class(j.at("armorClass").get<int>());
	
	// Load baseArmorClass if present, otherwise use armorClass as base
	if (j.contains("baseArmorClass")) 
	{
		set_base_armor_class(j.at("baseArmorClass").get<int>());
	} 
	else 
	{
		set_base_armor_class(get_armor_class());
	}
}

void Destructible::save(json& j)
{
	j["hpMax"] = get_max_hp();
	j["hp"] = get_hp();
	j["hpBase"] = get_hp_base();
	j["lastConstitution"] = get_last_constitution();
	j["dr"] = get_dr();
	j["corpseName"] = get_corpse_name();
	j["xp"] = get_xp();
	j["thaco"] = get_thaco();
	j["armorClass"] = get_armor_class();
	j["baseArmorClass"] = get_base_armor_class();
}

void PlayerDestructible::save(json& j)
{
	j["type"] = static_cast<int>(DestructibleType::PLAYER);
	Destructible::save(j);
}

void MonsterDestructible::save(json& j) 
{
	j["type"] = static_cast<int>(DestructibleType::MONSTER);
	Destructible::save(j);
}

std::unique_ptr<Destructible> Destructible::create(const json& j)
{
	if (j.contains("type") && j["type"].is_number())
	{
		const auto type = static_cast<DestructibleType>(j["type"].get<int>());
		std::unique_ptr<Destructible> destructible{};
		switch (type)
		{
		case DestructibleType::MONSTER:
		{
			destructible = std::make_unique<MonsterDestructible>(0, 0, "", 0, 0, 0);
			break;
		}
		case DestructibleType::PLAYER:
		{
			destructible = std::make_unique<PlayerDestructible>(0, 0, "", 0, 0, 0);
			break;
		}
		default:
			break;
		}
		if (destructible)
		{
			destructible->load(j);
		}
		return destructible;
	}
	return nullptr;
}

//==PlayerDestructible==
PlayerDestructible::PlayerDestructible(
	int hpMax,
	int dr,
	std::string_view corpseName,
	int xp,
	int thaco,
	int armorClass
) :
	Destructible(hpMax, dr, corpseName, xp, thaco, armorClass)
{
}

void PlayerDestructible::die(Creature& owner)
{
	game.gameStatus = Game::GameStatus::DEFEAT;
}

//==MonsterDestructible==
MonsterDestructible::MonsterDestructible(
	int hpMax,
	int dr,
	std::string_view corpseName,
	int xp,
	int thaco,
	int armorClass
) :
	Destructible(hpMax, dr, corpseName, xp, thaco, armorClass)
{}

void MonsterDestructible::die(Creature& owner)
{
	// message which monster is dead
	game.append_message_part(owner.actorData.color, std::format("{}", owner.actorData.name));
	game.append_message_part(WHITE_BLACK_PAIR, " is dead.\n");
	game.finalize_message();
	
	// get the xp from the monster
	

	// message how much xp you get
	game.append_message_part(WHITE_BLACK_PAIR, "You get ");
	game.append_message_part(YELLOW_BLACK_PAIR, std::format("{}", get_xp()));
	game.append_message_part(WHITE_BLACK_PAIR, " experience points.\n");
	game.finalize_message();

	// increase the player's experience
	/*game.player->destructible->xp += xp;*/
	game.player->destructible->set_xp(game.player->destructible->get_xp() + get_xp());
	
	game.player->ai->levelup_update(*game.player);

	Destructible::die(owner);
}

// end of file: Destructible.cpp
