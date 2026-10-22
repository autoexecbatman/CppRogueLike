// file: MonsterIntelligenceRatingTest.cpp
// Every monster monsters.json describes carries the Intelligence rating its Monstrous
// Manual entry prints. The table below is the book's, one line per monster with the PDF
// page of the local archive (counting from 1, as a PDF viewer does) and the column read
// where the entry has several, so a reviewer can check each against the source rather
// than against the data. The columns are the ones MonsterSizeTest reads.
//
// Three readings are choices rather than transcriptions, each by the driver's ruling of
// 2026-10-17:
//   - The bugbear and the goblin print "Low to average (5-10)". A record holds one
//     rating, so they take the first named, Low; it stays editable per monster.
//   - The rat page (p1812) extracts as two columns, Rat (Giant) and Osquip, with three
//     values: "Animal (1) Animal (1) Semi- (2-4)". The page pairs a plain value with a
//     parenthesised giant value on its other lines - Hit Dice "1/4 (1/2)", Damage
//     "1 (1-3)" - so the rat is read as Animal and the giant rat as Semi-. That is a
//     reading of a mangled layout; open p1812 to check it.
//   - A monster the book has no entry for is a human, and is Average.
//
// The ogre's line prints "Low (8)", and Low is 5-7; the named rating is taken.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterIntelligenceRatingTest.*

#include <gtest/gtest.h>

#include <iterator>
#include <string>
#include <string_view>

#include "src/IntelligenceRating.h"
#include "src/MonsterRegistry.h"
#include "src/Paths.h"

namespace
{
struct BookRating
{
	std::string_view key{};
	IntelligenceRating rating{ IntelligenceRating::AVERAGE };
};

// Monstrous Manual, by PDF page in the local archive.
constexpr BookRating BOOK_RATINGS[] = {
	{ "archer", IntelligenceRating::AVERAGE }, // no entry; a human
	{
		"bat",
		IntelligenceRating::ANIMAL,
	}, // Bat p1110, Common column: Animal (1)
	{
		"bugbear",
		IntelligenceRating::LOW,
	}, // Bugbear p1148: Low to average (5-10), the first named
	{
		"chimera",
		IntelligenceRating::SEMI,
	}, // Chimera p1176, Chimera column: Semi- (2-4)
	{
		"dragon",
		IntelligenceRating::EXCEPTIONAL,
	}, // Dragon, Chromatic Red Dragon p1240: Exceptional (15-16)
	{ "dungeon_jailer", IntelligenceRating::AVERAGE }, // no entry; a human
	{ "dungeon_warden", IntelligenceRating::AVERAGE }, // no entry; a human
	{ "fire_wolf", IntelligenceRating::LOW }, // Hell Hound p1544: Low (5-7)
	{
		"gargoyle",
		IntelligenceRating::LOW,
	}, // Gargoyle p1393, Gargoyle column: Low (5-7)
	{
		"ghoul",
		IntelligenceRating::LOW,
	}, // Ghoul p1406, Ghoul column: Low (5-7)
	{
		"giant_centipede",
		IntelligenceRating::NON,
	}, // Centipede p1173, Giant column: Non- (0)
	{
		"giant_rat",
		IntelligenceRating::SEMI,
	}, // Rat p1812, Rat (Giant) column, the giant's value: Semi- (2-4)
	{
		"giant_snake",
		IntelligenceRating::ANIMAL,
	}, // Snake p1862, Poisonous (Giant): Animal (1)
	{
		"gnoll",
		IntelligenceRating::LOW,
	}, // Gnoll p1472, Gnoll column: Low (5-7)
	{
		"goblin",
		IntelligenceRating::LOW,
	}, // Goblin p1483: Low to average (5-10), the first named
	{
		"golem_stone",
		IntelligenceRating::NON,
	}, // Golem, Greater p1491, Stone column: Non- (0)
	{ "harpy", IntelligenceRating::LOW }, // Harpy p1537: Low (5-7)
	{ "hill_giant", IntelligenceRating::LOW }, // Giant, Hill p1431: Low (5-7)
	{
		"hobgoblin",
		IntelligenceRating::AVERAGE,
	}, // Hobgoblin p1553: Average (8-10)
	{
		"ice_wolf",
		IntelligenceRating::AVERAGE,
	}, // Wolf p1961, Winter column: Average (8-10)
	{
		"kobold",
		IntelligenceRating::AVERAGE,
	}, // Kobold p1614, Kobold column: Average (8-10)
	{
		"lich",
		IntelligenceRating::SUPRA_GENIUS,
	}, // Lich p1634, Lich column: Supra-genius (19-20)
	{ "mage", IntelligenceRating::AVERAGE }, // no entry; a human
	{
		"medusa",
		IntelligenceRating::VERY,
	}, // Medusa p1693, Medusa column: Very (11-12)
	{
		"mimic",
		IntelligenceRating::SEMI,
	}, // Mimic p1700, Killer column: Semi- (2-4)
	{ "ogre", IntelligenceRating::LOW }, // Ogre p1750, Ogre column: Low (8)
	{
		"orc",
		IntelligenceRating::AVERAGE,
	}, // Orc p1769, Orc column: Average (8-9)
	{
		"pit_fiend",
		IntelligenceRating::GENIUS,
	}, // Baatezu p1101, Pit Fiend column: Genius (17-18)
	{
		"rat",
		IntelligenceRating::ANIMAL,
	}, // Rat p1812, Rat (Giant) column, the rat's value: Animal (1)
	{ "shadow", IntelligenceRating::LOW }, // Shadow p1841: Low (5-7)
	{ "shopkeeper", IntelligenceRating::AVERAGE }, // no entry: a human, Average
	{
		"skeleton",
		IntelligenceRating::NON,
	}, // Skeleton p1849, Skeleton column: Non- (0)
	{
		"spider_giant",
		IntelligenceRating::ANIMAL,
	}, // Spider p1876, Huge column: Animal (1)
	{
		"spider_small",
		IntelligenceRating::LOW,
	}, // Spider p1876, Hairy column: Low (5-7)
	{
		"spider_weaver",
		IntelligenceRating::LOW,
	}, // Spider p1876, Giant column: Low (5-7)
	{
		"troll",
		IntelligenceRating::LOW,
	}, // Troll p1930, Troll column: Low (5-7)
	{
		"vampire",
		IntelligenceRating::EXCEPTIONAL,
	}, // Vampire p1945: Exceptional (15-16)
	{
		"werewolf",
		IntelligenceRating::AVERAGE,
	}, // Lycanthrope, Werewolf p1677: Average (8-10)
	{ "wight", IntelligenceRating::AVERAGE }, // Wight p1957: Average (8-10)
	{
		"wolf",
		IntelligenceRating::SEMI,
	}, // Wolf p1961, Wolf column: Semi- (2-4)
	{ "wraith", IntelligenceRating::VERY }, // Wraith p1969: Very (11-12)
	{ "wyvern", IntelligenceRating::LOW }, // Wyvern p1971: Low (5-7)
	{
		"zombie",
		IntelligenceRating::NON,
	}, // Zombie p1989, Common column: Non- (0)
};
} // namespace

TEST(MonsterIntelligenceRatingTest, EveryMonsterCarriesTheRatingItsBookEntryPrints)
{
	MonsterRegistry monsters{};
	monsters.load(Paths::MONSTERS);

	for (const BookRating& book : BOOK_RATINGS)
	{
		EXPECT_EQ(monsters.get_params(std::string{ book.key }).intelligence, book.rating) << book.key;
	}
	EXPECT_EQ(std::size(BOOK_RATINGS), monsters.get_all_keys().size()) << "a monster in the data is missing from the book's table";
}

// end of file: MonsterIntelligenceRatingTest.cpp
