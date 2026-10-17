// file: MonsterSizeTest.cpp
// Every monster monsters.json describes carries the size its Monstrous Manual entry
// prints. The table below is the book's, one line per monster with the PDF page of the
// local archive and the column read where the entry has several, so a reviewer can
// check each against the source rather than against the data.
//
// A monster the book has no entry for is sized by the closest entry, and says which.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterSizeTest.*

#include <gtest/gtest.h>

#include <string_view>

#include "src/CreatureSize.h"
#include "src/MonsterRegistry.h"
#include "src/Paths.h"

namespace
{
struct BookSize
{
	std::string_view key{};
	CreatureSize size{ CreatureSize::MEDIUM };
};

// Monstrous Manual, by PDF page in the local archive.
constexpr BookSize BOOK_SIZES[] = {
	{ "archer", CreatureSize::MEDIUM }, // no entry; a human
	{ "bat", CreatureSize::TINY }, // Bat p1110, Common column
	{ "bugbear", CreatureSize::LARGE }, // Bugbear p1148
	{ "chimera", CreatureSize::LARGE }, // Chimera p1176, Chimera column
	{ "dragon", CreatureSize::GARGANTUAN }, // Dragon, Chromatic Red Dragon p1240
	{ "dungeon_jailer", CreatureSize::MEDIUM }, // no entry; a human
	{ "dungeon_warden", CreatureSize::MEDIUM }, // no entry; a human
	{ "fire_wolf", CreatureSize::MEDIUM }, // Hell Hound p1544
	{ "gargoyle", CreatureSize::MEDIUM }, // Gargoyle p1393, Gargoyle column
	{ "ghoul", CreatureSize::MEDIUM }, // Ghoul p1406, Ghoul column
	{ "giant_centipede", CreatureSize::TINY }, // Centipede p1173, Giant column
	{ "giant_rat", CreatureSize::TINY }, // Rat p1812, Rat (Giant) column
	{ "giant_snake", CreatureSize::MEDIUM }, // Snake p1861, Poisonous (Giant): closest, AC 5 and HD 4+2
	{ "gnoll", CreatureSize::LARGE }, // Gnoll p1472, Gnoll column
	{ "goblin", CreatureSize::SMALL }, // Goblin p1483
	{ "golem_stone", CreatureSize::LARGE }, // Golem, Greater p1491, Stone column
	{ "harpy", CreatureSize::MEDIUM }, // Harpy p1537
	{ "hill_giant", CreatureSize::HUGE_SIZE }, // Giant, Hill p1431
	{ "hobgoblin", CreatureSize::MEDIUM }, // Hobgoblin p1553
	{ "ice_wolf", CreatureSize::LARGE }, // Wolf p1961, Winter column
	{ "kobold", CreatureSize::SMALL }, // Kobold p1614, Kobold column
	{ "lich", CreatureSize::MEDIUM }, // Lich p1634, Lich column
	{ "mage", CreatureSize::MEDIUM }, // no entry; a human
	{ "medusa", CreatureSize::MEDIUM }, // Medusa p1693, Medusa column
	{ "ogre", CreatureSize::LARGE }, // Ogre p1750, Ogre column
	{ "orc", CreatureSize::MEDIUM }, // Orc p1769, Orc column
	{ "pit_fiend", CreatureSize::LARGE }, // Baatezu p1101, Pit Fiend column
	{ "rat", CreatureSize::TINY }, // Rat p1812: closest, the giant rat's column
	{ "shadow", CreatureSize::MEDIUM }, // Shadow p1841
	{ "skeleton", CreatureSize::MEDIUM }, // Skeleton p1849, Skeleton column
	{ "troll", CreatureSize::LARGE }, // Troll p1930, Troll column
	{ "vampire", CreatureSize::MEDIUM }, // Vampire p1945
	{ "werewolf", CreatureSize::MEDIUM }, // Lycanthrope, Werewolf p1677
	{ "wight", CreatureSize::MEDIUM }, // Wight p1957
	{ "wolf", CreatureSize::SMALL }, // Wolf p1961, Wolf column
	{ "wraith", CreatureSize::MEDIUM }, // Wraith p1969
	{ "wyvern", CreatureSize::GARGANTUAN }, // Wyvern p1971
	{ "zombie", CreatureSize::MEDIUM }, // Zombie p1989, Common column
};
} // namespace

TEST(MonsterSizeTest, EveryMonsterCarriesTheSizeItsBookEntryPrints)
{
	MonsterRegistry monsters{};
	monsters.load(Paths::MONSTERS);

	for (const BookSize& book : BOOK_SIZES)
	{
		EXPECT_EQ(monsters.get_params(std::string{ book.key }).size, book.size) << book.key;
	}
}

// end of file: MonsterSizeTest.cpp
