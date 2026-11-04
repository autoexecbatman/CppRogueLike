// file: AlignmentTest.cpp
// Verifies that alignment is two independent axes and that monsters authored
// before alignment existed load as true neutral rather than failing.

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "src/Actor.h"
#include "src/Alignment.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/MonsterCreator.h"
#include "src/MonsterRegistry.h"
#include "src/Paths.h"
#include "src/TileRef.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"
#include "tests/mocks/TempTestPath.h"

namespace CppRL
{

	class AlignmentTest : public ::testing::Test
	{
	protected:
		Creature creature{
			Vector2D{ 0, 0 },
			ActorData{ TileRef{}, "goblin", ColorPairId::WHITE_BLACK },
		};
	};

	// Nothing is aligned until its data says so.
	TEST_F(AlignmentTest, DefaultsToTrueNeutral)
	{
		EXPECT_EQ(creature.get_ethics(), Alignment::Ethics::NEUTRAL);
		EXPECT_EQ(creature.get_morality(), Alignment::Morality::NEUTRAL);
		EXPECT_FALSE(creature.is_evil());
	}

	// The axes are independent: setting one must not disturb the other.
	TEST_F(AlignmentTest, AxesAreIndependent)
	{
		creature.set_ethics(Alignment::Ethics::CHAOTIC);

		EXPECT_EQ(creature.get_ethics(), Alignment::Ethics::CHAOTIC);
		EXPECT_EQ(creature.get_morality(), Alignment::Morality::NEUTRAL)
			<< "morality moved with ethics";

		creature.set_morality(Alignment::Morality::GOOD);

		EXPECT_EQ(creature.get_ethics(), Alignment::Ethics::CHAOTIC)
			<< "ethics moved with morality";
		EXPECT_EQ(creature.get_morality(), Alignment::Morality::GOOD);
	}

	// is_evil reads only the morality axis, which is what alignment-keyed
	// effects such as Protection from Evil test.
	TEST_F(AlignmentTest, IsEvilReadsMoralityAlone)
	{
		creature.set_ethics(Alignment::Ethics::LAWFUL);
		creature.set_morality(Alignment::Morality::EVIL);
		EXPECT_TRUE(creature.is_evil()) << "lawful evil is still evil";

		creature.set_ethics(Alignment::Ethics::CHAOTIC);
		EXPECT_TRUE(creature.is_evil()) << "ethics must not affect is_evil";

		creature.set_morality(Alignment::Morality::NEUTRAL);
		EXPECT_FALSE(creature.is_evil());
	}

	// Every monster the Monstrous Manual names, with the alignment its entry
	// gives. The values are read from the book by hand rather than from the
	// file, so a record edited to the wrong alignment fails here instead of
	// being confirmed by itself. A multi-creature stat block lists its variants
	// in column order, so the first value belongs to the creature the entry is
	// named for.
	TEST(AlignmentDataTest, EveryMonsterCarriesTheAlignmentItsEntryGives)
	{
		MonsterRegistry monsters{};
		monsters.load("data/content/monsters.json");

		const std::vector<
			std::tuple<std::string, Alignment::Ethics, Alignment::Morality>>
			fromTheBook{
				{
					"bat",
					Alignment::Ethics::NEUTRAL,
					Alignment::Morality::NEUTRAL,
				},
				{
					"bugbear",
					Alignment::Ethics::CHAOTIC,
					Alignment::Morality::EVIL,
				},
				{
					"chimera",
					Alignment::Ethics::CHAOTIC,
					Alignment::Morality::EVIL,
				},
				{
					"gargoyle",
					Alignment::Ethics::CHAOTIC,
					Alignment::Morality::EVIL,
				},
				{
					"ghoul",
					Alignment::Ethics::CHAOTIC,
					Alignment::Morality::EVIL,
				},
				{
					"giant_centipede",
					Alignment::Ethics::NEUTRAL,
					Alignment::Morality::NEUTRAL,
				},
				{
					"giant_rat",
					Alignment::Ethics::NEUTRAL,
					Alignment::Morality::NEUTRAL,
				},
				{
					"giant_snake",
					Alignment::Ethics::NEUTRAL,
					Alignment::Morality::NEUTRAL,
				},
				{
					"gnoll",
					Alignment::Ethics::CHAOTIC,
					Alignment::Morality::EVIL,
				},
				{
					"goblin",
					Alignment::Ethics::LAWFUL,
					Alignment::Morality::EVIL,
				},
				{
					"golem_stone",
					Alignment::Ethics::NEUTRAL,
					Alignment::Morality::NEUTRAL,
				},
				{
					"harpy",
					Alignment::Ethics::CHAOTIC,
					Alignment::Morality::EVIL,
				},
				{
					"hill_giant",
					Alignment::Ethics::CHAOTIC,
					Alignment::Morality::EVIL,
				},
				{
					"hobgoblin",
					Alignment::Ethics::LAWFUL,
					Alignment::Morality::EVIL,
				},
				{
					"kobold",
					Alignment::Ethics::LAWFUL,
					Alignment::Morality::EVIL,
				},
				{
					"medusa",
					Alignment::Ethics::LAWFUL,
					Alignment::Morality::EVIL,
				},
				{
					"ogre",
					Alignment::Ethics::CHAOTIC,
					Alignment::Morality::EVIL,
				},
				{ "orc", Alignment::Ethics::LAWFUL, Alignment::Morality::EVIL },
				{
					"pit_fiend",
					Alignment::Ethics::LAWFUL,
					Alignment::Morality::EVIL,
				},
				{
					"rat",
					Alignment::Ethics::NEUTRAL,
					Alignment::Morality::NEUTRAL,
				},
				{
					"shadow",
					Alignment::Ethics::CHAOTIC,
					Alignment::Morality::EVIL,
				},
				{
					"skeleton",
					Alignment::Ethics::NEUTRAL,
					Alignment::Morality::NEUTRAL,
				},
				{
					"troll",
					Alignment::Ethics::CHAOTIC,
					Alignment::Morality::EVIL,
				},
				{
					"vampire",
					Alignment::Ethics::CHAOTIC,
					Alignment::Morality::EVIL,
				},
				{
					"werewolf",
					Alignment::Ethics::CHAOTIC,
					Alignment::Morality::EVIL,
				},
				{
					"wight",
					Alignment::Ethics::LAWFUL,
					Alignment::Morality::EVIL,
				},
				{
					"wolf",
					Alignment::Ethics::NEUTRAL,
					Alignment::Morality::NEUTRAL,
				},
				{
					"wraith",
					Alignment::Ethics::LAWFUL,
					Alignment::Morality::EVIL,
				},
				{
					"wyvern",
					Alignment::Ethics::NEUTRAL,
					Alignment::Morality::EVIL,
				},
				{
					"zombie",
					Alignment::Ethics::NEUTRAL,
					Alignment::Morality::NEUTRAL,
				},
			};

		for (const auto& [key, ethics, morality] : fromTheBook)
		{
			const MonsterParams& params = monsters.get_params(key);
			EXPECT_EQ(params.ethics, ethics)
				<< key << " is on the wrong side of law and chaos";
			EXPECT_EQ(params.morality, morality)
				<< key << " is on the wrong side of good and evil";
		}
	}

	// The monsters the book does not name, each matched to the closest entry it
	// does. The rule is the owner's, 2026-09-23: invented content takes the
	// nearest book answer rather than waiting for a decision.
	TEST(AlignmentDataTest, InventedMonstersTakeTheirClosestEntrysAlignment)
	{
		MonsterRegistry monsters{};
		monsters.load("data/content/monsters.json");

		// A wolf that breathes fire is the hell hound, which is lawful evil.
		EXPECT_EQ(
			monsters.get_params("fire_wolf").ethics,
			Alignment::Ethics::LAWFUL);
		EXPECT_EQ(
			monsters.get_params("fire_wolf").morality,
			Alignment::Morality::EVIL);

		// A wolf that breathes cold is the winter wolf, the last column of the
		// wolf entry.
		EXPECT_EQ(
			monsters.get_params("ice_wolf").ethics,
			Alignment::Ethics::NEUTRAL);
		EXPECT_EQ(
			monsters.get_params("ice_wolf").morality,
			Alignment::Morality::EVIL);

		// The lich entry says "any evil", so the axis the book leaves open is
		// decided here and the one it fixes is not.
		EXPECT_EQ(
			monsters.get_params("lich").morality,
			Alignment::Morality::EVIL);
	}

	// Every record the full parser reads carries both axes, so none of them is
	// relying on a default any more.
	TEST(AlignmentDataTest, EveryParsedRecordCarriesBothAxes)
	{
		std::ifstream file(Paths::resolve(Paths::MONSTERS));
		ASSERT_TRUE(file.is_open())
			<< "monsters.json did not open; the case measures nothing";

		nlohmann::json content;
		file >> content;

		int checked = 0;
		for (const auto& [key, record] : content.items())
		{
			EXPECT_TRUE(record.contains("ethics"))
				<< key << " carries no ethics";
			EXPECT_TRUE(record.contains("morality"))
				<< key << " carries no morality";
			++checked;
		}
		EXPECT_GT(checked, 0)
			<< "no record was checked; the case measures nothing";
	}

	// What the whole item is for: Protection from Evil reads is_evil, and until
	// the records carried an alignment there was nothing in the game it could
	// ward against.
	TEST(
		AlignmentDataTest,
		TheDungeonHoldsMonstersProtectionFromEvilWardsAgainst)
	{
		MonsterRegistry monsters{};
		monsters.load("data/content/monsters.json");

		int evil = 0;
		for (
			const std::string& key :
			{ "goblin", "orc", "kobold", "troll", "ogre", "vampire" })
		{
			if (monsters.get_params(key).morality == Alignment::Morality::EVIL)
			{
				++evil;
			}
		}
		EXPECT_EQ(evil, 6)
			<< "the commonest dungeon monsters are not evil, so the ward does nothing";
	}

	// The spiders and the mimic carry their Monstrous Manual entry's alignment
	// in their records, and are built with it. The shopkeeper has no entry and
	// its record is true neutral, which is the answer a merchant wants.
	TEST(AlignmentCodeBuiltTest, TheSpidersAndTheMimicCarryTheirEntrysAlignment)
	{
		MockGameContext mock{};
		mock.dice.set_test_mode(false);
		GameContext ctx = mock.to_game_context();
		const auto build = [&mock, &ctx](std::string_view key)
		{
			return MonsterCreator::create_from_params(
				Vector2D{ 1, 1 },
				mock.monsterRegistry.get_params(key),
				ctx);
		};

		const auto hairy = build("spider_small");
		EXPECT_EQ(hairy->get_ethics(), Alignment::Ethics::NEUTRAL);
		EXPECT_EQ(hairy->get_morality(), Alignment::Morality::EVIL)
			<< "the hairy spider is neutral evil";

		const auto huge = build("spider_giant");
		EXPECT_EQ(huge->get_ethics(), Alignment::Ethics::NEUTRAL);
		EXPECT_EQ(huge->get_morality(), Alignment::Morality::NEUTRAL)
			<< "the huge spider is neutral";

		const auto giant = build("spider_weaver");
		EXPECT_EQ(giant->get_ethics(), Alignment::Ethics::CHAOTIC);
		EXPECT_EQ(giant->get_morality(), Alignment::Morality::EVIL)
			<< "the giant spider is chaotic evil";

		const auto killer = build("mimic");
		EXPECT_EQ(killer->get_ethics(), Alignment::Ethics::NEUTRAL);
		EXPECT_EQ(killer->get_morality(), Alignment::Morality::EVIL)
			<< "the killer mimic is neutral (evil)";

		const MonsterParams& shopkeeper =
			mock.monsterRegistry.get_params("shopkeeper");
		EXPECT_EQ(shopkeeper.ethics, Alignment::Ethics::NEUTRAL);
		EXPECT_EQ(shopkeeper.morality, Alignment::Morality::NEUTRAL)
			<< "the shopkeeper is true neutral";
	}

	// A record that names no alignment is an unfinished record, refused rather
	// than handed a default, now that every one of them carries the book's
	// answer.
	TEST(AlignmentDataTest, ARecordWithNoAlignmentIsRefused)
	{
		std::ifstream file(Paths::resolve(Paths::MONSTERS));
		ASSERT_TRUE(file.is_open());

		nlohmann::json content;
		file >> content;
		content.at("goblin").erase("ethics");

		const std::filesystem::path damaged = temp_test_path("monsters.json");
		std::ofstream out(damaged);
		out << content.dump(2);
		out.close();

		MonsterRegistry monsters{};
		EXPECT_ANY_THROW(monsters.load(damaged.string()));

		std::filesystem::remove(damaged);
	}

	// The peaceful-attack shift is a house rule, so the test states the rule
	// rather than citing the Player's Handbook, which gives no drift formula at
	// all.
	TEST(AlignmentShiftTest, ShiftMovesOneStepTowardChaotic)
	{
		EXPECT_EQ(
			Alignment::shift_toward_chaotic(Alignment::Ethics::LAWFUL),
			Alignment::Ethics::NEUTRAL);
		EXPECT_EQ(
			Alignment::shift_toward_chaotic(Alignment::Ethics::NEUTRAL),
			Alignment::Ethics::CHAOTIC);
	}

	// Chaotic is the floor: there is nothing further to fall to.
	TEST(AlignmentShiftTest, ShiftStopsAtChaotic)
	{
		EXPECT_EQ(
			Alignment::shift_toward_chaotic(Alignment::Ethics::CHAOTIC),
			Alignment::Ethics::CHAOTIC);
	}

	// One betrayal is one step, so a lawful player needs two to reach chaotic.
	TEST(AlignmentShiftTest, RepeatedShiftsAccumulateOneStepEach)
	{
		Alignment::Ethics ethics = Alignment::Ethics::LAWFUL;

		ethics = Alignment::shift_toward_chaotic(ethics);
		EXPECT_EQ(ethics, Alignment::Ethics::NEUTRAL)
			<< "one betrayal must not reach chaotic";

		ethics = Alignment::shift_toward_chaotic(ethics);
		EXPECT_EQ(ethics, Alignment::Ethics::CHAOTIC);
	}

} // namespace CppRL
