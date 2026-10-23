// file: MonsterAbilityTables.cpp
//
// The one place a monster rolls its six ability scores on the Dungeon Master Guide's
// Tables 6 to 11. The tables and their reads are in MonsterAbilityTables.h; this file
// holds only what needs a Creature and the dice.
//
// Usage:
//
//   roll_monster_ability_scores(mimic, CreatureSize::LARGE, IntelligenceRating::SEMI, std::nullopt, *ctx.dice);
//   mimic.get_strength(); // -> a score from Table 6's large column

#include "MonsterAbilityTables.h"

#include "Creature.h"
#include "RandomDice.h"

// Example, every die at its highest, so each read is the table's 18 row:
//   roll_monster_ability_scores(creature, CreatureSize::LARGE, IntelligenceRating::SEMI, std::nullopt, dice);
//   creature.get_strength(); // -> 20
//   creature.get_charisma(); // -> 6
void roll_monster_ability_scores(
	Creature& creature,
	CreatureSize size,
	IntelligenceRating rating,
	std::optional<MonsterStrength> printedStrength,
	RandomDice& dice)
{
	// One 3d6 per score, in the book's order: Strength, Dexterity, Constitution,
	// Intelligence, Wisdom, Charisma. A printed Strength takes no die.
	const MonsterStrength strength = printedStrength.has_value() ?
		*printedStrength :
		monster_strength_score(roll_dice(&dice, MONSTER_ABILITY_DICE), size);
	creature.set_strength(strength.score);
	creature.set_exceptional_strength(strength.exceptional);
	creature.set_dexterity(
		monster_dexterity_score(roll_dice(&dice, MONSTER_ABILITY_DICE), size));
	creature.set_constitution(monster_constitution_score(
		roll_dice(&dice, MONSTER_ABILITY_DICE),
		size));
	creature.set_intelligence(monster_intelligence_score(
		roll_dice(&dice, MONSTER_ABILITY_DICE),
		rating));
	creature.set_wisdom(
		monster_wisdom_score(roll_dice(&dice, MONSTER_ABILITY_DICE), rating));
	creature.set_charisma(
		monster_charisma_score(roll_dice(&dice, MONSTER_ABILITY_DICE), rating));
}

// end of file: MonsterAbilityTables.cpp
