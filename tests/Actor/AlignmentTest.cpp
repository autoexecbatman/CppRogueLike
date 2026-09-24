// file: AlignmentTest.cpp
// Verifies that alignment is two independent axes and that monsters authored
// before alignment existed load as true neutral rather than failing.

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <tuple>
#include <vector>

#include "src/Alignment.h"
#include "src/Paths.h"
#include "src/Monsters.h"
#include "src/Spider.h"
#include "tests/mocks/MockGameContext.h"
#include "src/Creature.h"
#include "src/MonsterRegistry.h"
#include "src/Vector2D.h"

class AlignmentTest : public ::testing::Test
{
protected:
	Creature creature{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "goblin", 0 } };
};

// Nothing is aligned until its data says so.
TEST_F(AlignmentTest, DefaultsToTrueNeutral)
{
	EXPECT_EQ(creature.get_ethics(), Ethics::NEUTRAL);
	EXPECT_EQ(creature.get_morality(), Morality::NEUTRAL);
	EXPECT_FALSE(creature.is_evil());
}

// The axes are independent: setting one must not disturb the other.
TEST_F(AlignmentTest, AxesAreIndependent)
{
	creature.set_ethics(Ethics::CHAOTIC);

	EXPECT_EQ(creature.get_ethics(), Ethics::CHAOTIC);
	EXPECT_EQ(creature.get_morality(), Morality::NEUTRAL) << "morality moved with ethics";

	creature.set_morality(Morality::GOOD);

	EXPECT_EQ(creature.get_ethics(), Ethics::CHAOTIC) << "ethics moved with morality";
	EXPECT_EQ(creature.get_morality(), Morality::GOOD);
}

// is_evil reads only the morality axis, which is what alignment-keyed effects
// such as Protection from Evil test.
TEST_F(AlignmentTest, IsEvilReadsMoralityAlone)
{
	creature.set_ethics(Ethics::LAWFUL);
	creature.set_morality(Morality::EVIL);
	EXPECT_TRUE(creature.is_evil()) << "lawful evil is still evil";

	creature.set_ethics(Ethics::CHAOTIC);
	EXPECT_TRUE(creature.is_evil()) << "ethics must not affect is_evil";

	creature.set_morality(Morality::NEUTRAL);
	EXPECT_FALSE(creature.is_evil());
}

// Every monster the Monstrous Manual names, with the alignment its entry gives. The
// values are read from the book by hand rather than from the file, so a record edited to
// the wrong alignment fails here instead of being confirmed by itself. A multi-creature
// stat block lists its variants in column order, so the first value belongs to the
// creature the entry is named for.
TEST(AlignmentDataTest, EveryMonsterCarriesTheAlignmentItsEntryGives)
{
	MonsterRegistry monsters{};
	monsters.load("data/content/monsters.json");

	const std::vector<std::tuple<std::string, Ethics, Morality>> fromTheBook{
		{ "bat", Ethics::NEUTRAL, Morality::NEUTRAL },
		{ "bugbear", Ethics::CHAOTIC, Morality::EVIL },
		{ "chimera", Ethics::CHAOTIC, Morality::EVIL },
		{ "gargoyle", Ethics::CHAOTIC, Morality::EVIL },
		{ "ghoul", Ethics::CHAOTIC, Morality::EVIL },
		{ "giant_centipede", Ethics::NEUTRAL, Morality::NEUTRAL },
		{ "giant_rat", Ethics::NEUTRAL, Morality::NEUTRAL },
		{ "giant_snake", Ethics::NEUTRAL, Morality::NEUTRAL },
		{ "gnoll", Ethics::CHAOTIC, Morality::EVIL },
		{ "goblin", Ethics::LAWFUL, Morality::EVIL },
		{ "golem_stone", Ethics::NEUTRAL, Morality::NEUTRAL },
		{ "harpy", Ethics::CHAOTIC, Morality::EVIL },
		{ "hill_giant", Ethics::CHAOTIC, Morality::EVIL },
		{ "hobgoblin", Ethics::LAWFUL, Morality::EVIL },
		{ "kobold", Ethics::LAWFUL, Morality::EVIL },
		{ "medusa", Ethics::LAWFUL, Morality::EVIL },
		{ "ogre", Ethics::CHAOTIC, Morality::EVIL },
		{ "orc", Ethics::LAWFUL, Morality::EVIL },
		{ "pit_fiend", Ethics::LAWFUL, Morality::EVIL },
		{ "rat", Ethics::NEUTRAL, Morality::NEUTRAL },
		{ "shadow", Ethics::CHAOTIC, Morality::EVIL },
		{ "skeleton", Ethics::NEUTRAL, Morality::NEUTRAL },
		{ "troll", Ethics::CHAOTIC, Morality::EVIL },
		{ "vampire", Ethics::CHAOTIC, Morality::EVIL },
		{ "werewolf", Ethics::CHAOTIC, Morality::EVIL },
		{ "wight", Ethics::LAWFUL, Morality::EVIL },
		{ "wolf", Ethics::NEUTRAL, Morality::NEUTRAL },
		{ "wraith", Ethics::LAWFUL, Morality::EVIL },
		{ "wyvern", Ethics::NEUTRAL, Morality::EVIL },
		{ "zombie", Ethics::NEUTRAL, Morality::NEUTRAL },
	};

	for (const auto& [key, ethics, morality] : fromTheBook)
	{
		const MonsterParams& params = monsters.get_params(key);
		EXPECT_EQ(params.ethics, ethics) << key << " is on the wrong side of law and chaos";
		EXPECT_EQ(params.morality, morality) << key << " is on the wrong side of good and evil";
	}
}

// The monsters the book does not name, each matched to the closest entry it does. The
// rule is the owner's, 2026-09-23: invented content takes the nearest book answer rather
// than waiting for a decision.
TEST(AlignmentDataTest, InventedMonstersTakeTheirClosestEntrysAlignment)
{
	MonsterRegistry monsters{};
	monsters.load("data/content/monsters.json");

	// A wolf that breathes fire is the hell hound, which is lawful evil.
	EXPECT_EQ(monsters.get_params("fire_wolf").ethics, Ethics::LAWFUL);
	EXPECT_EQ(monsters.get_params("fire_wolf").morality, Morality::EVIL);

	// A wolf that breathes cold is the winter wolf, the last column of the wolf entry.
	EXPECT_EQ(monsters.get_params("ice_wolf").ethics, Ethics::NEUTRAL);
	EXPECT_EQ(monsters.get_params("ice_wolf").morality, Morality::EVIL);

	// The lich entry says "any evil", so the axis the book leaves open is decided here
	// and the one it fixes is not.
	EXPECT_EQ(monsters.get_params("lich").morality, Morality::EVIL);
}

// Every record the full parser reads carries both axes, so none of them is relying on a
// default any more.
TEST(AlignmentDataTest, EveryParsedRecordCarriesBothAxes)
{
	std::ifstream file(Paths::resolve(Paths::MONSTERS));
	ASSERT_TRUE(file.is_open()) << "monsters.json did not open; the case measures nothing";

	nlohmann::json content;
	file >> content;

	int checked = 0;
	for (const auto& [key, record] : content.items())
	{
		// The five class-based creatures persist a tile only; their alignment is set where
		// they are built, since the full parser never sees them.
		if (!record.contains("hp"))
		{
			continue;
		}
		EXPECT_TRUE(record.contains("ethics")) << key << " carries no ethics";
		EXPECT_TRUE(record.contains("morality")) << key << " carries no morality";
		++checked;
	}
	EXPECT_GT(checked, 0) << "no record was checked; the case measures nothing";
}

// What the whole item is for: Protection from Evil reads is_evil, and until the records
// carried an alignment there was nothing in the game it could ward against.
TEST(AlignmentDataTest, TheDungeonHoldsMonstersProtectionFromEvilWardsAgainst)
{
	MonsterRegistry monsters{};
	monsters.load("data/content/monsters.json");

	int evil = 0;
	for (const std::string& key : { "goblin", "orc", "kobold", "troll", "ogre", "vampire" })
	{
		if (monsters.get_params(key).morality == Morality::EVIL)
		{
			++evil;
		}
	}
	EXPECT_EQ(evil, 6) << "the commonest dungeon monsters are not evil, so the ward does nothing";
}

// The five creatures built in code rather than authored: the full parser never sees
// them, so their alignment is set where they are constructed and this is what checks it.
// The shopkeeper is left at the Creature default, true neutral, which is the answer a
// merchant wants - stated here so the absence reads as a decision.
TEST(AlignmentCodeBuiltTest, TheCreaturesBuiltInCodeCarryTheirEntrysAlignment)
{
	MockGameContext mock{};
	mock.dice.set_test_mode(false);
	GameContext ctx = mock.to_game_context();

	const SmallSpider hairy{ Vector2D{ 1, 1 }, ctx };
	EXPECT_EQ(hairy.get_ethics(), Ethics::NEUTRAL);
	EXPECT_EQ(hairy.get_morality(), Morality::EVIL) << "the hairy spider is neutral evil";

	const GiantSpider huge{ Vector2D{ 2, 1 }, ctx };
	EXPECT_EQ(huge.get_ethics(), Ethics::NEUTRAL);
	EXPECT_EQ(huge.get_morality(), Morality::NEUTRAL) << "the huge spider is neutral";

	const WebSpinner giant{ Vector2D{ 3, 1 }, ctx };
	EXPECT_EQ(giant.get_ethics(), Ethics::CHAOTIC);
	EXPECT_EQ(giant.get_morality(), Morality::EVIL) << "the giant spider is chaotic evil";

	const Mimic killer{ Vector2D{ 4, 1 }, ctx };
	EXPECT_EQ(killer.get_ethics(), Ethics::NEUTRAL);
	EXPECT_EQ(killer.get_morality(), Morality::EVIL) << "the killer mimic is neutral (evil)";
}

// A record that names no alignment is an unfinished record, refused rather than handed a
// default, now that every one of them carries the book's answer.
TEST(AlignmentDataTest, ARecordWithNoAlignmentIsRefused)
{
	std::ifstream file(Paths::resolve(Paths::MONSTERS));
	ASSERT_TRUE(file.is_open());

	nlohmann::json content;
	file >> content;
	content.at("goblin").erase("ethics");

	const std::filesystem::path damaged = std::filesystem::temp_directory_path() / "monsters_no_alignment.json";
	std::ofstream out(damaged);
	out << content.dump(2);
	out.close();

	MonsterRegistry monsters{};
	EXPECT_ANY_THROW(monsters.load(damaged.string()));

	std::filesystem::remove(damaged);
}

// The peaceful-attack shift is a house rule, so the test states the rule rather
// than citing the Player's Handbook, which gives no drift formula at all.
TEST(AlignmentShiftTest, ShiftMovesOneStepTowardChaotic)
{
	EXPECT_EQ(shift_toward_chaotic(Ethics::LAWFUL), Ethics::NEUTRAL);
	EXPECT_EQ(shift_toward_chaotic(Ethics::NEUTRAL), Ethics::CHAOTIC);
}

// Chaotic is the floor: there is nothing further to fall to.
TEST(AlignmentShiftTest, ShiftStopsAtChaotic)
{
	EXPECT_EQ(shift_toward_chaotic(Ethics::CHAOTIC), Ethics::CHAOTIC);
}

// One betrayal is one step, so a lawful player needs two to reach chaotic.
TEST(AlignmentShiftTest, RepeatedShiftsAccumulateOneStepEach)
{
	Ethics ethics = Ethics::LAWFUL;

	ethics = shift_toward_chaotic(ethics);
	EXPECT_EQ(ethics, Ethics::NEUTRAL) << "one betrayal must not reach chaotic";

	ethics = shift_toward_chaotic(ethics);
	EXPECT_EQ(ethics, Ethics::CHAOTIC);
}
