// file: Combat/SavingThrow.cpp
#include <algorithm>
#include <array>
#include <limits>

#include "../Actor/Creature.h"
#include "../Core/GameContext.h"
#include "../Random/RandomDice.h"
#include "SavingThrow.h"

namespace
{

constexpr size_t CATEGORY_COUNT = 5;

// One row of Table 60: the highest level it covers, and the five targets in
// the order of the SavingThrow enum.
struct Row
{
	int lastLevel{ 0 };
	std::array<int, CATEGORY_COUNT> targets{};
};

// AD&D 2nd edition Player's Handbook, Table 60, transcribed by column order:
// Paralyzation/Poison/Death Magic, Rod/Staff/Wand, Petrification/Polymorph,
// Breath Weapon, Spell.
constexpr int LAST = std::numeric_limits<int>::max();

constexpr std::array<Row, 10> warriorRows = { {
	{ 0, { 16, 18, 17, 20, 19 } },
	{ 2, { 14, 16, 15, 17, 17 } },
	{ 4, { 13, 15, 14, 16, 16 } },
	{ 6, { 11, 13, 12, 13, 14 } },
	{ 8, { 10, 12, 11, 12, 13 } },
	{ 10, { 8, 10, 9, 9, 11 } },
	{ 12, { 7, 9, 8, 8, 10 } },
	{ 14, { 5, 7, 6, 5, 8 } },
	{ 16, { 4, 6, 5, 4, 7 } },
	{ LAST, { 3, 5, 4, 4, 6 } },
} };

constexpr std::array<Row, 7> priestRows = { {
	{ 3, { 10, 14, 13, 16, 15 } },
	{ 6, { 9, 13, 12, 15, 14 } },
	{ 9, { 7, 11, 10, 13, 12 } },
	{ 12, { 6, 10, 9, 12, 11 } },
	{ 15, { 5, 9, 8, 11, 10 } },
	{ 18, { 4, 8, 7, 10, 9 } },
	{ LAST, { 2, 6, 5, 8, 7 } },
} };

constexpr std::array<Row, 6> rogueRows = { {
	{ 4, { 13, 14, 12, 16, 15 } },
	{ 8, { 12, 12, 11, 15, 13 } },
	{ 12, { 11, 10, 10, 14, 11 } },
	{ 16, { 10, 8, 9, 13, 9 } },
	{ 20, { 9, 6, 8, 12, 7 } },
	{ LAST, { 8, 4, 7, 11, 5 } },
} };

constexpr std::array<Row, 5> wizardRows = { {
	{ 5, { 14, 11, 13, 15, 12 } },
	{ 10, { 13, 9, 11, 13, 10 } },
	{ 15, { 11, 7, 9, 11, 8 } },
	{ 20, { 10, 5, 7, 9, 6 } },
	{ LAST, { 8, 3, 5, 7, 4 } },
} };

// The rows a class saves on. Monsters take the warrior rows, as they attack on
// the warrior table and gain hit dice rather than class levels.
std::span<const Row> rows_for(CreatureClass creatureClass)
{
	switch (creatureClass)
	{
	case CreatureClass::CLERIC:
	{
		return priestRows;
	}

	case CreatureClass::ROGUE:
	{
		return rogueRows;
	}

	case CreatureClass::WIZARD:
	{
		return wizardRows;
	}

	case CreatureClass::FIGHTER:
	case CreatureClass::MONSTER:
	{
		return warriorRows;
	}
	}
	std::unreachable();
}

// The row a level falls on. A level below the table's first row takes that row,
// which is where a 1st-level character of every class already sits.
const Row& row_at(CreatureClass creatureClass, int level)
{
	const std::span<const Row> rows = rows_for(creatureClass);
	for (const Row& row : rows)
	{
		if (level <= row.lastLevel)
		{
			return row;
		}
	}
	return rows.back();
}

} // namespace

int SavingThrows::target(CreatureClass creatureClass, int level, SavingThrow category)
{
	return row_at(creatureClass, level).targets[static_cast<size_t>(category)];
}

bool SavingThrows::improves_at(CreatureClass creatureClass, int level)
{
	// A level improves the save when it lands on a row the level below did not.
	return &row_at(creatureClass, level) != &row_at(creatureClass, level - 1);
}

bool SavingThrows::is_made(const Creature& saver, SavingThrow category, int modifier, GameContext& ctx)
{
	assert(ctx.dice && "SavingThrows::is_made called without dice");
	const int needed = target(saver.get_creature_class(), saver.get_creature_level(), category);
	return ctx.dice->d20() + modifier >= needed;
}
