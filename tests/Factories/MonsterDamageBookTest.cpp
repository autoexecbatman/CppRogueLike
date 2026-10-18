// file: MonsterDamageBookTest.cpp
// Every monster's damage dice against its Monstrous Manual entry, one row each with
// the PDF page of the local archive and the entry's Damage/Attack line verbatim.
//
// The game makes one natural attack a round, so a row holds one attack's dice with
// no Strength in them: the one printed, or the largest of several, or the carried
// weapon's where the entry says "by weapon". A range is read through the dice the
// text names, else the same range named in another entry, else 1-N as 1dN, else M-N
// as 1d(N-M+1)+(M-1), the shape the DMG gives the troll's claw (PDF page 210).
// issues/0222 holds the reading in full.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterDamageBookTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <string_view>

#include "src/MonsterRegistry.h"
#include "src/Paths.h"

namespace
{
struct BookDamage
{
	std::string_view key{};
	std::string_view dice{};
};

constexpr BookDamage BOOK_DAMAGE[] = {
	// p1111: "Damage/Attack: 1 1d2 or 1d4 2d4 1/1-2 1-4/1-2/1-2/1-6 or"
	// Common column, the first: a printed 1.
	{ "bat", "1" },
	// p1148: "Damage/Attack: 2-8 (2d4) or by weapon"
	// The text names 2d4; its morning star is 2d4 too.
	{ "bugbear", "2d4" },
	// p1176: "Damage/Attack: 1-3/1-3/1-4/1-4/ 1-3/1-3/2-8"
	// Chimera column, six attacks ending 2-8 (2d4)/ 3-12 (3d4); the largest, named 3d4.
	{ "chimera", "3d4" },
	// p1240: "Damage/Attack: 1-10/1-10/3-30 (3d10)"
	// The bite, named 3d10. The +4 is not Strength by the text and stays until 0221 rules.
	{ "dragon", "3d10+4" },
	// p1544: "Damage/Attack: 1-10"
	// Hell hound. 1-N reads as 1dN.
	{ "fire_wolf", "1d10" },
	// p1393: "Damage/Attack: 1-3/1-3/1-6/1-4 1-6/1-6/2-8/2-8"
	// Gargoyle column; the largest of four, 1-6.
	{ "gargoyle", "1d6" },
	// p1407: "Damage/Attack: 1-3/1-3/1-6 1-3/1-3/1-6 1- 4/1-4/1-8"
	// Ghoul column; the bite, 1-6.
	{ "ghoul", "1d6" },
	// p1173: "Damage/Attack: Nil Nil 1-3 2-8"
	// Giant column prints Nil, poison only. Kept at 1d2 until the record carries poison: issue 0236.
	{ "giant_centipede", "1d2" },
	// p1812: "Damage/Attack: 1 (1-3) 2-12"
	// Read as rat 1, giant rat (1-3) in the first column - an inference.
	{ "giant_rat", "1d3" },
	// p1862: "Damage/Attack: 1 1-3 1-6/3-18 1-3"
	// Poison (Giant), the second column.
	{ "giant_snake", "1d3" },
	// p1473: "Damage/Attack: 2-8 (2d4) (weapon) 1-6 or 1-4 (weapons)"
	// By weapon: the battle axe it carries, 1d8.
	{ "gnoll", "1d8" },
	// p1483: "Damage/Attack: 1-6 (by weapon)"
	// By weapon: its short sword, 1d6.
	{ "goblin", "1d6" },
	// p1492: "Damage/Attack: 3-24 (3d8) 4-40 (4d10)"
	// Stone column, named 3d8.
	{ "golem_stone", "3d8" },
	// p1537: "Damage/Attack: 1-3/1-3/1-6 or"
	// The largest of three, 1-6; it carries no weapon.
	{ "harpy", "1d6" },
	// p1431: "Damage/Attack: 1-6 or by weapon (2-12+7)"
	// 2-12 is 2d6; the +7 is its Strength (p1432).
	{ "hill_giant", "2d6" },
	// p1553: "Damage/Attack: by weapon"
	// By weapon: its long sword, 1d8.
	{ "hobgoblin", "1d8" },
	// p1962: "Damage/Attack: 2-5 2-8 2-8 2-8"
	// Winter column, the fourth: 2-8 is 2d4, as 0199 read it.
	{ "ice_wolf", "2d4" },
	// p1615: "Damage/Attack: 1-4 or 1-6 1-3 or 1-4"
	// By weapon: its dagger, 1d4.
	{ "kobold", "1d4" },
	// p1634: "Damage/Attack: 1-10 See below"
	// Lich column, 1-10.
	{ "lich", "1d10" },
	// p1694: "Damage/Attack: 1-4 1-4"
	// Medusa column, 1-4.
	{ "medusa", "1d4" },
	// p1700: "Damage/Attack: 3-12 (smash) 3-12 (smash)"
	// Killer column. 3-12 as the chimera's entry names it, 3d4.
	{ "mimic", "3d4" },
	// p1751: "Damage/Attack: 1-10 (or by 1- 12 1-6/1-6/2-8 (or"
	// By weapon: its club, 1d6.
	{ "ogre", "1d6" },
	// p1769: "Damage/Attack: 1-8"
	// Orc column, by weapon: its long sword, 1d8.
	{ "orc", "1d8" },
	// p1102: "Damage/Attack: 1-4x2/1-6x2/ 1-4/1-4/2-5 1-4/1-4/2-5 1-4/1-4/2-5"
	// Continues 2-12/2-8; the largest, the bite, 2-12 as 2d6.
	{ "pit_fiend", "2d6" },
	// p1812: "Damage/Attack: 1 (1-3) 2-12"
	// Read as rat 1, giant rat (1-3) in the first column - an inference.
	{ "rat", "1" },
	// p1841: "Damage/Attack: 2-5"
	// M-N with M above 1 reads as 1d(N-M+1)+(M-1), the DMG's shape (p210). Flagged in 0221.
	{ "shadow", "1d4+1" },
	// p1850: "Damage/Attack: 1-6 (weapon) 1- 4 Special"
	// Skeleton column. By weapon, but it carries none: the printed 1-6.
	{ "skeleton", "1d6" },
	// p1877: "Damage/Attack: 1 1 1-6 1-8"
	// Huge column, the third.
	{ "spider_giant", "1d6" },
	// p1877: "Damage/Attack: 1 1 1-6 1-8"
	// Hairy column, the first: a printed 1.
	{ "spider_small", "1" },
	// p1877: "Damage/Attack: 1 1 1-6 1-8"
	// Giant column, the fourth.
	{ "spider_weaver", "1d8" },
	// p1930: "Damage/Attack: 5-8/5-8/5-12 5-8/5-8/1-12/1-12 2-5/2-5/3-12 1-4/1-4/9-16"
	// Troll column; the largest, the bite 5-12, as 1d8+4. The DMG (p210) gives the bite as 2d6; the Monstrous Manual governs. Flagged in 0221.
	{ "troll", "1d8+4" },
	// p1945: "Damage/Attack: 5-10"
	// 1d6 with the +4 its Strength gives (p1946).
	{ "vampire", "1d6" },
	// p1677: "Damage/Attack: 2-8"
	// 2-8 is 2d4, as 0199 read it.
	{ "werewolf", "2d4" },
	// p1957: "Damage/Attack: 1-4"
	{ "wight", "1d4" },
	// p1962: "Damage/Attack: 2-5 2-8 2-8 2-8"
	// Wolf column, the first; 2-5 as 1d4+1. Flagged in 0221.
	{ "wolf", "1d4+1" },
	// p1969: "Damage/Attack: 1-6"
	{ "wraith", "1d6" },
	// p1971: "Damage/Attack: 2-16/1-6"
	// The bite, 2-16, as the hill giant's entry names it, 2d8 (p1431).
	{ "wyvern", "2d8" },
	// p1990: "Damage/Attack: 1-8 4-16 3-12 2-8/2-8 1-10"
	// Common column, the first.
	{ "zombie", "1d8" },
};

// The game's own monsters, which the Monstrous Manual does not print.
constexpr std::string_view NO_ENTRY[] = { "archer", "dungeon_jailer", "dungeon_warden", "mage", "shopkeeper" };

bool is_listed(std::string_view key)
{
	const auto isThisMonster = [key](const BookDamage& book)
	{
		return book.key == key;
	};
	return std::ranges::any_of(BOOK_DAMAGE, isThisMonster) || std::ranges::find(NO_ENTRY, key) != std::ranges::end(NO_ENTRY);
}
} // namespace

class MonsterDamageBookTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		monsters.load(Paths::MONSTERS);
	}

	MonsterRegistry monsters{};
};

// Each record deals the dice its entry prints, read as the header says.
TEST_F(MonsterDamageBookTest, EveryMonsterDealsItsEntrysDice)
{
	for (const BookDamage& book : BOOK_DAMAGE)
	{
		EXPECT_EQ(monsters.get_params(std::string{ book.key }).damage.displayRoll, book.dice) << book.key;
	}
}

// A monster added to the data has to be read against the book before it is trusted.
TEST_F(MonsterDamageBookTest, EveryRecordIsReadAgainstTheBook)
{
	for (const std::string& key : monsters.get_all_keys())
	{
		EXPECT_TRUE(is_listed(key)) << key << " has no row in BOOK_DAMAGE and is not a game-only monster";
	}
}
