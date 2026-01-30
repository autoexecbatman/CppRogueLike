// file: Destructible.cpp
#include <iostream>
#include <string>
#include <string_view>
#include <algorithm>
#include <format>

#include "../Core/GameContext.h"
#include "../Items/Items.h"
#include "../Actor/Actor.h"
#include "../Actor/InventoryOperations.h"
#include "../Colors/Colors.h"
#include "../Attributes/StrengthAttributes.h"
#include "../ActorTypes/Healer.h"
#include "../Items/CorpseFood.h"
#include "../Items/Armor.h"
#include "../Items/Jewelry.h"
#include "../Items/MagicalItemEffects.h"
#include "../ActorTypes/Player.h"
#include "Pickable.h"
#include "../Attributes/ConstitutionAttributes.h"
#include "../Attributes/DexterityAttributes.h"
#include "../Systems/DataManager.h"
#include "../Systems/MessageSystem.h"
#include "../Systems/GameStateManager.h"

using namespace InventoryOperations; // For clean function calls

//====
Destructible::Destructible(int hpMax, int dr, std::string_view corpseName, int xp, int thaco, int armorClass)
	:
	hpMax(hpMax),
	dr(dr),
	corpseName(corpseName),
	xp(xp),
	thaco(thaco),
	armorClass(armorClass),
	hp(hpMax),
	hpBase(hpMax),
	baseArmorClass(armorClass),
	lastConstitution(0),
    tempHp(0)
{}

void Destructible::update_constitution_bonus(Creature& owner, GameContext& ctx)
{
    const int currentConstitution = owner.get_constitution();
    const int lastConstitution = get_last_constitution();
    
    if (currentConstitution == lastConstitution)
    {
        return;
    }
    
    const int oldBonus = calculate_constitution_hp_bonus_for_value(lastConstitution, ctx);
    const int newBonus = calculate_constitution_hp_bonus(owner, ctx);
    const int level = calculate_level_multiplier(owner);
    const int hpDifference = (newBonus - oldBonus) * level;
    
    if (hpDifference != 0)
    {
        // Constitution changes affect both max HP and current HP
        // Temporary HP are NOT affected by Constitution changes
        set_max_hp(get_max_hp() + hpDifference);
        set_hp(get_hp() + hpDifference);

        log_constitution_change(owner, ctx, lastConstitution, currentConstitution, hpDifference);

        // Check for stat drain death
        if (get_hp() <= 0)
        {
            set_hp(0);
            handle_stat_drain_death(owner, ctx);
        }
    }
    
    set_last_constitution(currentConstitution);
}

[[nodiscard]] int Destructible::calculate_constitution_hp_bonus(const Creature& owner, GameContext& ctx) const
{
    return calculate_constitution_hp_bonus_for_value(owner.get_constitution(), ctx);
}

[[nodiscard]] int Destructible::calculate_constitution_hp_bonus_for_value(int constitution, GameContext& ctx) const
{
    const auto& constitutionAttributes = ctx.data_manager->get_constitution_attributes();
    
    // Validate constitution is in valid range (1-based indexing)
    if (constitution < 1 || constitution > static_cast<int>(constitutionAttributes.size()))
    {
        return 0;
    }
    
    return constitutionAttributes[constitution - 1].HPAdj;
}

[[nodiscard]] int Destructible::calculate_level_multiplier(const Creature& owner) const
{
    // Monsters always use 1x
    const auto* player = dynamic_cast<const Player*>(&owner);
    if (!player)
    {
        return 1;
    }
    
    const int level = player->get_player_level();
    
    // AD&D 2e: Constitution bonus caps at level 9 for warriors/priests, 10 for others
    switch (player->playerClassState)
    {
        case Player::PlayerClassState::FIGHTER:
            return std::min(level, 9);  // Warriors cap at 9
            
        case Player::PlayerClassState::CLERIC:
            return std::min(level, 9);  // Priests cap at 9
            
        case Player::PlayerClassState::ROGUE:
        case Player::PlayerClassState::WIZARD:
            return std::min(level, 10); // Others cap at 10
            
        default:
            return std::min(level, 10); // Default cap at 10
    }
}

void Destructible::handle_stat_drain_death(Creature& owner, GameContext& ctx)
{
    if (&owner == ctx.player)
    {
        ctx.message_system->message(RED_BLACK_PAIR, "Your life force has been drained beyond recovery. You die!", true);
    }
    else
    {
        ctx.message_system->log(std::format("{} dies from stat drain.", owner.get_name()));
    }
    
    // Trigger death system
    die(owner, ctx);
}

void Destructible::log_constitution_change(const Creature& owner, GameContext& ctx, int oldCon, int newCon, int hpChange) const
{
    // Only log for player
    if (&owner != ctx.player)
    {
        return;
    }
    
    if (hpChange > 0)
    {
        ctx.message_system->message(GREEN_BLACK_PAIR,
            std::format("Constitution increased from {} to {}! You gain {} hit points.", oldCon, newCon, hpChange), true);
    }
    else
    {
        ctx.message_system->message(RED_BLACK_PAIR,
            std::format("Constitution decreased from {} to {}! You lose {} hit points.", oldCon, newCon, -hpChange), true);
    }
}

int Destructible::take_damage(Creature& owner, int damage, GameContext& ctx)
{
    // Negative or zero damage has no effect
    if (damage <= 0)
    {
        return 0;
    }

    // AD&D 2e: Temp HP absorbs damage first
    if (get_temp_hp() > 0)
    {
        const int tempAbsorbed = std::min(damage, get_temp_hp());
        set_temp_hp(get_temp_hp() - tempAbsorbed);
        damage -= tempAbsorbed;

        if (damage == 0)
        {
            return 0;  // All damage absorbed by temp HP
        }
    }

    // Remaining damage hits real HP
    const int newHp = get_hp() - damage;
    set_hp(newHp);

    // Explicit clamping and death check
    if (get_hp() <= 0)
    {
        set_hp(0);
        die(owner, ctx);
    }

    return damage;
}

// Transform the actor into a corpse !
void Destructible::die(Creature& owner, GameContext& ctx)
{
	// copy data to new entity of type Item
	auto corpse = std::make_unique<Item>(owner.position, owner.actorData);
	corpse->actorData.name = get_corpse_name();
	corpse->actorData.ch = '%';
	corpse->pickable = std::make_unique<CorpseFood>(0); // 0 means calculate from type

	// Add the corpse to the floor items
	add_item(*ctx.inventory_data, std::move(corpse));

	// Note: The creature will be removed from game.creatures later
	// when it's safe to do so (e.g., at the end of the turn)
}

//====

// The function returns the amount of health point actually restored.
[[nodiscard]] int Destructible::heal(int hpToHeal)
{
    const int currentHp = get_hp();
    const int maxHp = get_max_hp();
    const int newHp = std::min(currentHp + hpToHeal, maxHp);
    const int actualHealed = newHp - currentHp;

    set_hp(newHp);

    // Explicit clamping
    if (get_hp() > get_max_hp())
    {
        set_hp(get_max_hp());
    }

    return actualHealed;
}

void Destructible::update_armor_class(Creature& owner, GameContext& ctx)
{
    const int baseAC = get_base_armor_class();
    const int dexBonus = calculate_dexterity_ac_bonus(owner, ctx);
    const int equipBonus = calculate_equipment_ac_bonus(owner, ctx);
    const int tempBonus = owner.get_temporary_ac_bonus();
    const int calculatedAC = baseAC + dexBonus + equipBonus + tempBonus;

    // Only update if changed
    if (get_armor_class() != calculatedAC)
    {
        const int oldAC = get_armor_class();
        set_armor_class(calculatedAC);

        // Log for player
        if (&owner == ctx.player)
        {
            ctx.message_system->log(std::format(
                "Armor Class updated: {} → {} (Base: {}, Dex: {:+}, Equipment: {:+}, Temp: {:+})",
                oldAC, calculatedAC, baseAC, dexBonus, equipBonus, tempBonus));
        }
    }
}

[[nodiscard]] int Destructible::calculate_dexterity_ac_bonus(const Creature& owner, GameContext& ctx) const
{
    const auto& dexAttributes = ctx.data_manager->get_dexterity_attributes();
    const int dexterity = owner.get_dexterity();
    
    // Validate dexterity is in valid range (1-based indexing)
    if (dexterity <= 0 || dexterity > static_cast<int>(dexAttributes.size()))
    {
        return 0;
    }
    
    const int defensiveAdj = dexAttributes[dexterity - 1].DefensiveAdj;
    
    // Log for player if non-zero
    if (&owner == ctx.player && defensiveAdj != 0)
    {
        ctx.message_system->log(std::format(
            "Dexterity Defensive Adjustment: {:+} (Dex: {})",
            defensiveAdj, dexterity));
    }
    
    return defensiveAdj;
}

[[nodiscard]] int Destructible::calculate_equipment_ac_bonus(const Creature& owner, GameContext& ctx) const
{
    // Check if owner is a player (use new equipment system)
    if (const auto* player = dynamic_cast<const Player*>(&owner))
    {
        return calculate_player_equipment_ac(*player, ctx);
    }
    
    // Use old system for creatures
    return calculate_creature_equipment_ac(owner, ctx);
}

[[nodiscard]] int Destructible::calculate_player_equipment_ac(const Player& player, GameContext& ctx) const
{
    int totalBonus = 0;

    // Check body armor
    if (Item* equippedArmor = player.get_equipped_item(EquipmentSlot::BODY))
    {
        if (const auto* armor = dynamic_cast<const Armor*>(equippedArmor->pickable.get()))
        {
            const int armorBonus = armor->getArmorClass();
            totalBonus += armorBonus;

            if (&player == ctx.player)
            {
                ctx.message_system->log(std::format(
                    "Armor bonus: {:+} from {}",
                    armorBonus, equippedArmor->actorData.name));
            }
        }
    }

    // Check shield in left hand
    if (Item* equippedShield = player.get_equipped_item(EquipmentSlot::LEFT_HAND))
    {
        if (const auto* shield = dynamic_cast<const Shield*>(equippedShield->pickable.get()))
        {
            const int shieldBonus = shield->get_ac_bonus();
            totalBonus += shieldBonus;

            if (&player == ctx.player)
            {
                ctx.message_system->log(std::format(
                    "Shield bonus: {:+} from {}",
                    shieldBonus, equippedShield->actorData.name));
            }
        }
    }

    // Check rings for protection bonuses (AD&D 2e: best ring applies, no stacking)
    int bestRingBonus = 0;
    const Item* bestRing = nullptr;

    for (const auto slot : {EquipmentSlot::RIGHT_RING, EquipmentSlot::LEFT_RING})
    {
        if (Item* equippedRing = player.get_equipped_item(slot))
        {
            if (const auto* magicRing = dynamic_cast<const MagicalRing*>(equippedRing->pickable.get()))
            {
                const int ringBonus = MagicalEffectUtils::get_protection_bonus(magicRing->effect);
                if (ringBonus < bestRingBonus)
                {
                    bestRingBonus = ringBonus;
                    bestRing = equippedRing;
                }
            }
        }
    }

    if (bestRingBonus < 0 && bestRing)
    {
        totalBonus += bestRingBonus;

        if (&player == ctx.player)
        {
            ctx.message_system->log(std::format(
                "Ring bonus: {:+} from {}",
                bestRingBonus, bestRing->actorData.name));
        }
    }

    // Check helm for AC bonuses (extensible for any magical effect)
    if (Item* equippedHelm = player.get_equipped_item(EquipmentSlot::HEAD))
    {
        if (const auto* magicHelm = dynamic_cast<const MagicalHelm*>(equippedHelm->pickable.get()))
        {
            const int helmBonus = MagicalEffectUtils::get_ac_bonus(magicHelm->effect);
            if (helmBonus < 0)
            {
                totalBonus += helmBonus;

                if (&player == ctx.player)
                {
                    ctx.message_system->log(std::format(
                        "Helm bonus: {:+} from {}",
                        helmBonus, equippedHelm->actorData.name));
                }
            }
        }
    }

    return totalBonus;
}

[[nodiscard]] int Destructible::calculate_creature_equipment_ac(const Creature& owner, GameContext& ctx) const
{
    int totalBonus = 0;
    
    // Check all equipped items
    for (const auto& item : owner.inventory_data.items)
    {
        if (!item || !item->has_state(ActorState::IS_EQUIPPED))
        {
            continue;
        }
        
        if (const auto* armor = dynamic_cast<const Armor*>(item->pickable.get()))
        {
            const int armorBonus = armor->getArmorClass();
            totalBonus += armorBonus;
            
            ctx.message_system->log(std::format("{} armor bonus: {:+} from {}", owner.get_name(), armorBonus, item->actorData.name));
        }
    }
    
    return totalBonus;
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
	set_base_armor_class(j.at("baseArmorClass").get<int>());
	set_temp_hp(j.at("tempHp").get<int>());
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
    j["tempHp"] = get_temp_hp();
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

[[nodiscard]] std::unique_ptr<Destructible> Destructible::create(const json& j)
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
{}

void PlayerDestructible::die(Creature& owner, GameContext& ctx)
{
	*ctx.game_status = GameStatus::DEFEAT;

	// Delete save file on death (ignore return value - non-critical operation)
	[[maybe_unused]] const bool deleted = ctx.state_manager->delete_save_file();
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

void MonsterDestructible::die(Creature& owner, GameContext& ctx)
{
	// message which monster is dead
	ctx.message_system->append_message_part(owner.actorData.color, std::format("{}", owner.actorData.name));
	ctx.message_system->append_message_part(WHITE_BLACK_PAIR, " is dead.\n");
	ctx.message_system->finalize_message();

	// get the xp from the monster


	// message how much xp you get
	ctx.message_system->append_message_part(WHITE_BLACK_PAIR, "You get ");
	ctx.message_system->append_message_part(YELLOW_BLACK_PAIR, std::format("{}", get_xp()));
	ctx.message_system->append_message_part(WHITE_BLACK_PAIR, " experience points.\n");
	ctx.message_system->finalize_message();

	// increase the player's experience
	/*game.player->destructible->xp += xp;*/
	ctx.player->destructible->set_xp(ctx.player->destructible->get_xp() + get_xp());

	ctx.player->ai->levelup_update(ctx, *ctx.player);

	Destructible::die(owner, ctx);
}

// end of file: Destructible.cpp
