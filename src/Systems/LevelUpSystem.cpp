// LevelUpSystem.cpp - Handles combat improvements on level up according to AD&D 2e rules
#include <algorithm>
#include <format>
#include <string>

#include "../Actor/Creature.h"
#include "../Actor/Destructible.h"
#include "../Colors/Colors.h"
#include "../Core/GameContext.h"
#include "../dnd_tables/CalculatedTHAC0s.h"
#include "../Systems/DataManager.h"
#include "../Systems/MessageSystem.h"
#include "LevelUpSystem.h"

void LevelUpSystem::apply_level_up_benefits(Creature& owner, int newLevel, GameContext* ctx)
{
	if (!ctx)
	{
		return;
	}

	int oldTHAC0 = owner.destructible ? owner.destructible->get_thaco() : 20;

	apply_thac0_improvement(owner, newLevel, ctx);
	int hpGained = apply_hit_point_gain(owner, newLevel, ctx);
	apply_class_specific_improvements(owner, newLevel, ctx);

	if (newLevel % 4 == 0 && owner.get_creature_class() != CreatureClass::MONSTER)
	{
		apply_ability_score_improvement(owner, newLevel, ctx);
	}

	apply_saving_throw_improvements(owner, newLevel, ctx);

	bool thac0_improved = owner.destructible && (oldTHAC0 != owner.destructible->get_thaco());

	if (owner.get_creature_class() != CreatureClass::MONSTER)
	{
		ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, "LEVEL UP! ");
		ctx->message_system->append_message_part(WHITE_BLACK_PAIR, std::format("You are now level {}. ", newLevel));
		ctx->message_system->append_message_part(GREEN_BLACK_PAIR, std::format("+{} HP, ", hpGained));
		if (thac0_improved)
		{
			ctx->message_system->append_message_part(GREEN_BLACK_PAIR,
				std::format("THAC0 {}->{}", oldTHAC0, owner.destructible->get_thaco()));
		}
		ctx->message_system->finalize_message();
		ctx->message_system->log(std::format("Level {} reached! Combat abilities improved.", newLevel));
	}
	else
	{
		ctx->message_system->log(std::format("{} reaches level {}.", owner.actorData.name, newLevel));
	}
}

void LevelUpSystem::apply_thac0_improvement(Creature& owner, int newLevel, GameContext* ctx)
{
	if (!ctx || !owner.destructible)
	{
		return;
	}

	CalculatedTHAC0s thac0Tables;
	int newTHAC0 = 20;

	switch (owner.get_creature_class())
	{
	case CreatureClass::FIGHTER:
	case CreatureClass::MONSTER:
	{
		newTHAC0 = thac0Tables.get_fighter(newLevel);
		break;
	}

	case CreatureClass::ROGUE:
	{
		newTHAC0 = thac0Tables.get_rogue(newLevel);
		break;
	}

	case CreatureClass::CLERIC:
	{
		newTHAC0 = thac0Tables.get_cleric(newLevel);
		break;
	}

	case CreatureClass::WIZARD:
	{
		newTHAC0 = thac0Tables.get_wizard(newLevel);
		break;
	}
	}

	if (newTHAC0 < owner.destructible->get_thaco())
	{
		int oldTHAC0 = owner.destructible->get_thaco();
		owner.destructible->set_thaco(newTHAC0);

		if (owner.get_creature_class() != CreatureClass::MONSTER)
		{
			ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "THAC0 improved");
			ctx->message_system->append_message_part(WHITE_BLACK_PAIR,
				std::format(" from {} to {}!", oldTHAC0, newTHAC0));
			ctx->message_system->finalize_message();
		}

		ctx->message_system->log(std::format("THAC0 improved: {} -> {}", oldTHAC0, newTHAC0));
	}
}

int LevelUpSystem::apply_hit_point_gain(Creature& owner, int newLevel, GameContext* ctx)
{
	if (!ctx || !owner.destructible)
	{
		return 0;
	}

	auto roll_hit_die = [&]() -> int
	{
		switch (owner.get_hit_die())
		{

		case 4:
		{
			return ctx->dice->d4();
		}

		case 6:
		{
			return ctx->dice->d6();
		}

		case 10:
		{
			return ctx->dice->d10();
		}

		default:
		{
			return ctx->dice->d8();
		}
		}
	};

	int hitDiceRoll = roll_hit_die();
	std::string diceType = std::format("d{}", owner.get_hit_die());

	int conBonus = 0;
	int con = owner.get_constitution();
	int conTableSize = static_cast<int>(ctx->data_manager->get_constitution_attributes().size());
	if (con >= 1 && con <= conTableSize)
	{
		conBonus = ctx->data_manager->get_constitution_attributes()[con - 1].HPAdj;
	}

	int totalHPGain = std::max(1, hitDiceRoll + conBonus);

	owner.destructible->set_hp_base(owner.destructible->get_hp_base() + hitDiceRoll);
	owner.destructible->set_max_hp(owner.destructible->get_max_hp() + totalHPGain);
	owner.destructible->set_hp(owner.destructible->get_hp() + totalHPGain);

	if (owner.get_creature_class() != CreatureClass::MONSTER)
	{
		ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "Hit Points increased");
		ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " by ");
		ctx->message_system->append_message_part(GREEN_BLACK_PAIR, std::to_string(totalHPGain));
		ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " (");
		ctx->message_system->append_message_part(WHITE_BLACK_PAIR, diceType);
		ctx->message_system->append_message_part(WHITE_BLACK_PAIR, ": ");
		ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, std::to_string(hitDiceRoll));
		if (conBonus != 0)
		{
			ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " + ");
			ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, std::to_string(conBonus));
			ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " CON");
		}
		ctx->message_system->append_message_part(WHITE_BLACK_PAIR, ")");
		ctx->message_system->finalize_message();
	}

	ctx->message_system->log(std::format("HP increased by {} ({} rolled + {} CON bonus). Max HP now: {}",
		totalHPGain,
		hitDiceRoll,
		conBonus,
		owner.destructible->get_max_hp()));

	return totalHPGain;
}

void LevelUpSystem::apply_class_specific_improvements(Creature& owner, int newLevel, GameContext* ctx)
{
	if (!ctx)
	{
		return;
	}

	switch (owner.get_creature_class())
	{

	case CreatureClass::FIGHTER:
	{
		apply_fighter_improvements(owner, newLevel, ctx);
		break;
	}

	case CreatureClass::ROGUE:
	{
		apply_rogue_improvements(owner, newLevel, ctx);
		break;
	}

	case CreatureClass::CLERIC:
	{
		apply_cleric_improvements(owner, newLevel, ctx);
		break;
	}

	case CreatureClass::WIZARD:
	{
		apply_wizard_improvements(owner, newLevel, ctx);
		break;
	}

	case CreatureClass::MONSTER:
	{
		break; // No class-specific improvements for monsters currently
	}
	}
}

void LevelUpSystem::apply_fighter_improvements(Creature& owner, int newLevel, GameContext* ctx)
{
	if (!ctx)
	{
		return;
	}

	if (newLevel >= 13)
	{
		if (owner.get_attacks_per_round() < 2.0f)
		{
			owner.set_attacks_per_round(2.0f);
			ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
			ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "Extra Attack!");
			ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " You can now attack 2 times per round.");
			ctx->message_system->finalize_message();
			ctx->message_system->log("Fighter gained extra attack (2 attacks per round)");
		}
	}
	else if (newLevel >= 7)
	{
		if (owner.get_attacks_per_round() < 1.5f)
		{
			owner.set_attacks_per_round(1.5f);
			ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
			ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "Extra Attack!");
			ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " You can now attack 3/2 times per round.");
			ctx->message_system->finalize_message();
			ctx->message_system->log("Fighter gained extra attack (3/2 attacks per round)");
		}
	}

	if (newLevel % 3 == 0)
	{
		ctx->message_system->append_message_part(WHITE_BLACK_PAIR, "Your martial prowess improves!");
		ctx->message_system->finalize_message();
	}
}

void LevelUpSystem::apply_rogue_improvements(Creature& owner, int newLevel, GameContext* ctx)
{
	if (!ctx)
	{
		return;
	}

	int backstabMultiplier = calculate_backstab_multiplier(newLevel);
	if (backstabMultiplier > calculate_backstab_multiplier(newLevel - 1))
	{
		ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
		ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "Backstab improved!");
		ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " Damage multiplier: x");
		ctx->message_system->append_message_part(GREEN_BLACK_PAIR, std::to_string(backstabMultiplier));
		ctx->message_system->finalize_message();
		ctx->message_system->log(std::format("Rogue backstab multiplier increased to x{}", backstabMultiplier));
	}

	if (newLevel % 2 == 0)
	{
		ctx->message_system->append_message_part(WHITE_BLACK_PAIR, "Your thieving skills improve!");
		ctx->message_system->finalize_message();
	}
}

void LevelUpSystem::apply_cleric_improvements(Creature& owner, int newLevel, GameContext* ctx)
{
	if (!ctx)
	{
		return;
	}

	if (newLevel == 3 || newLevel == 5 || newLevel == 7 || newLevel == 9)
	{
		ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
		ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "Turn Undead improved!");
		ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " You can affect more powerful undead.");
		ctx->message_system->finalize_message();
		ctx->message_system->log(std::format("Cleric turn undead ability improved at level {}", newLevel));
	}

	if (newLevel >= 2)
	{
		ctx->message_system->append_message_part(WHITE_BLACK_PAIR, "Your divine power grows stronger!");
		ctx->message_system->finalize_message();
	}
}

void LevelUpSystem::apply_wizard_improvements(Creature& owner, int newLevel, GameContext* ctx)
{
	if (!ctx)
	{
		return;
	}

	if (newLevel % 2 == 1 && newLevel > 1)
	{
		int spellLevel = (newLevel + 1) / 2;
		if (spellLevel <= 9)
		{
			ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
			ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "New spell level!");
			ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " You can now cast level ");
			ctx->message_system->append_message_part(GREEN_BLACK_PAIR, std::to_string(spellLevel));
			ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " spells.");
			ctx->message_system->finalize_message();
			ctx->message_system->log(std::format("Wizard can now cast level {} spells", spellLevel));
		}
	}

	ctx->message_system->append_message_part(WHITE_BLACK_PAIR, "Your arcane knowledge deepens!");
	ctx->message_system->finalize_message();
}

int LevelUpSystem::calculate_backstab_multiplier(int level)
{
	if (level >= 13)
	{
		return 5;
	}
	if (level >= 9)
	{
		return 4;
	}
	if (level >= 5)
	{
		return 3;
	}
	if (level >= 1)
	{
		return 2;
	}
	return 1;
}

void LevelUpSystem::apply_ability_score_improvement(Creature& owner, int newLevel, GameContext* ctx)
{
	if (!ctx)
	{
		return;
	}

	ctx->message_system->append_message_part(YELLOW_BLACK_PAIR, "Special: ");
	ctx->message_system->append_message_part(GREEN_BLACK_PAIR, "Ability Score Improvement!");
	ctx->message_system->append_message_part(WHITE_BLACK_PAIR, " You may increase one ability score by 1 point.");
	ctx->message_system->finalize_message();

	switch (owner.get_creature_class())
	{

	case CreatureClass::FIGHTER:
	{
		owner.set_strength(std::min(18, owner.get_strength() + 1));
		ctx->message_system->append_message_part(GREEN_BLACK_PAIR,
			std::format("Strength increased to {}!", owner.get_strength()));
		ctx->message_system->finalize_message();
		break;
	}

	case CreatureClass::ROGUE:
	{
		owner.set_dexterity(std::min(18, owner.get_dexterity() + 1));
		ctx->message_system->append_message_part(GREEN_BLACK_PAIR,
			std::format("Dexterity increased to {}!", owner.get_dexterity()));
		ctx->message_system->finalize_message();
		break;
	}

	case CreatureClass::CLERIC:
	{
		owner.set_wisdom(std::min(18, owner.get_wisdom() + 1));
		ctx->message_system->append_message_part(GREEN_BLACK_PAIR,
			std::format("Wisdom increased to {}!", owner.get_wisdom()));
		ctx->message_system->finalize_message();
		break;
	}

	case CreatureClass::WIZARD:
	{
		owner.set_intelligence(std::min(18, owner.get_intelligence() + 1));
		ctx->message_system->append_message_part(GREEN_BLACK_PAIR,
			std::format("Intelligence increased to {}!", owner.get_intelligence()));
		ctx->message_system->finalize_message();
		break;
	}

	case CreatureClass::MONSTER:
	{
		break;
	}
	}

	ctx->message_system->log(std::format("Ability score improved at level {}", newLevel));
}

void LevelUpSystem::apply_saving_throw_improvements(Creature& owner, int newLevel, GameContext* ctx)
{
	if (!ctx)
	{
		return;
	}

	bool improved = false;

	switch (owner.get_creature_class())
	{

	case CreatureClass::FIGHTER:
	{
		improved = (newLevel == 3 || newLevel == 6 || newLevel == 9 || newLevel == 12 || newLevel == 15);
		break;
	}

	case CreatureClass::ROGUE:
	{
		improved = (newLevel % 4 == 0);
		break;
	}

	case CreatureClass::CLERIC:
	{
		improved = (newLevel % 3 == 0);
		break;
	}

	case CreatureClass::WIZARD:
	{
		improved = (newLevel % 5 == 0);
		break;
	}

	case CreatureClass::MONSTER:
	{
		improved = (newLevel % 2 == 0); // AD&D 2e: monster saves improve every 2 HD
		break;
	}
	}

	if (improved)
	{
		if (owner.get_creature_class() != CreatureClass::MONSTER)
		{
			ctx->message_system->append_message_part(WHITE_BLACK_PAIR, "Saving throws improved!");
			ctx->message_system->finalize_message();
		}
		ctx->message_system->log(std::format("Saving throws improved at level {}", newLevel));
	}
}
