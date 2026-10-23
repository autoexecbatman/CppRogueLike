// file: MonsterFeatureEntryTest.cpp
// A monster's step onto a trap or a web goes through the same features the player's does,
// and what the log says is about the monster: named when the player can see it, silent
// when the player cannot, and never in the second person.
//
// The step is Pathfinding::step, the one every monster move takes, so a test here fails
// if the monster path stops asking the tile features. The player stands at the west end of
// a lit corridor and computes its sight once; a monster near it is in view and one at the
// far end is beyond its sight radius.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=MonsterFeatureEntryTest.*

#include <gtest/gtest.h>

#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "src/ArmorClass.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/Decoration.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/Map.h"
#include "src/Pathfinding.h"
#include "src/Player.h"
#include "src/SpellTile.h"
#include "src/Trap.h"
#include "src/Web.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr int CORRIDOR_ROW = 5;
constexpr Vector2D PLAYER_TILE{ 2, CORRIDOR_ROW };
// Three tiles from the player, well inside sight.
constexpr Vector2D NEAR_TILE{ 5, CORRIDOR_ROW };
constexpr Vector2D NEAR_FEATURE{ 6, CORRIDOR_ROW };
// Fifteen tiles from the player, past sight.
constexpr Vector2D FAR_TILE{ 17, CORRIDOR_ROW };
constexpr Vector2D FAR_FEATURE{ 18, CORRIDOR_ROW };
// A dart is 1d4; this is what the scripted die shows.
constexpr int DART_ROLL = 3;
} // namespace

class MonsterFeatureEntryTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.playerOwner = &player;
		ctx.map = &map;
		ctx.creatures = &creatures;
		ctx.traps = &traps;
		ctx.spellTiles = &spellTiles;
		ctx.decorations = &decorations;

		map.init_tiles();
		for (int column = 1; column < 19; ++column)
		{
			map.set_tile(Vector2D{ column, CORRIDOR_ROW }, TileType::FLOOR);
		}

		player->healthPool = std::make_unique<HealthPool>(20);

		goblin.experienceReward = std::make_unique<ExperienceReward>(0);
		goblin.healthPool = std::make_unique<HealthPool>(30);
		goblin.armorClass = std::make_unique<ArmorClass>(10);
		// Ten is no adjustment, so a web's catch chance is its bare figure.
		goblin.set_dexterity(10);
		goblin.set_body_plan(mock.body_plans.get("humanoid"));
	}

	// The player looks once from where it stands.
	void player_looks() { map.compute_fov(ctx); }

	// Every part of every finished message, joined.
	std::string all_message_text() const
	{
		std::string joined;
		for (
			size_t index = 0;
			index < ctx.messageSystem->get_stored_message_count();
			++index)
		{
			for (
				const auto& part :
				ctx.messageSystem->get_attack_message_at(index))
			{
				joined += part.text;
			}
		}
		return joined;
	}

	MockGameContext mock{};
	GameContext ctx{};
	Map map{ 20, 12 };
	std::unique_ptr<Player> player{ std::make_unique<Player>(PLAYER_TILE) };
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::vector<std::unique_ptr<Trap>> traps{};
	std::vector<std::unique_ptr<SpellTile>> spellTiles{};
	std::vector<std::unique_ptr<Decoration>> decorations{};
	Creature goblin{
		NEAR_TILE,
		ActorData{ TileRef{}, "goblin", ColorPairId::WHITE_BLACK },
	};
};

// A goblin the player watches walk onto a dart trap is named in the log, with the damage,
// and nothing in the log says "you".
TEST_F(MonsterFeatureEntryTest, AMonsterInSightSpringingATrapIsNamed)
{
	traps.push_back(
		std::make_unique<Trap>(NEAR_FEATURE, TrapType::DART, mock.tile_config));
	player_looks();
	ASSERT_TRUE(map.is_in_fov(NEAR_TILE))
		<< "the goblin stands three tiles from the player";
	mock.dice.set_next_roll(DART_ROLL);
	// The coin keeps the trap, so no second message joins the log.
	mock.dice.set_next_roll(2);

	EXPECT_EQ(
		Pathfinding::step(goblin, NEAR_FEATURE, ctx),
		Pathfinding::StepResult::STOPPED);

	const std::string log = all_message_text();
	EXPECT_NE(
		log.find("The goblin triggers the dart trap and takes 3 damage!"),
		std::string::npos)
		<< log;
	EXPECT_EQ(log.find("You"), std::string::npos)
		<< "a monster's trap was told to the player as the player's: " << log;
}

// Every kind of trap is named in the log the way a search names it: "pit trap", "dart trap"
// and "arrow trap", the book's words for the first two. Each die shows 3.
TEST_F(MonsterFeatureEntryTest, ASprungTrapIsNamedByItsKind)
{
	struct NamedTrap
	{
		TrapType type{ TrapType::PIT };
		std::string_view name{};
		int damage{ 0 };
	};

	const NamedTrap namedTraps[] = {
		{ TrapType::PIT, "pit trap", 6 },
		{ TrapType::DART, "dart trap", 3 },
		{ TrapType::ARROW, "arrow trap", 3 },
	};
	player_looks();

	for (const NamedTrap& namedTrap : namedTraps)
	{
		traps.clear();
		traps.push_back(
			std::make_unique<Trap>(
				NEAR_FEATURE,
				namedTrap.type,
				mock.tile_config));
		// Three on every damage die, then the coin that keeps the trap.
		mock.dice.set_next_roll(3);
		if (namedTrap.type == TrapType::PIT)
		{
			mock.dice.set_next_roll(3);
		}
		mock.dice.set_next_roll(2);

		(void)Pathfinding::step(goblin, NEAR_FEATURE, ctx);

		const std::string expected = std::format(
			"The goblin triggers the {} and takes {} damage!",
			namedTrap.name,
			namedTrap.damage);
		EXPECT_NE(all_message_text().find(expected), std::string::npos)
			<< "missing: " << expected << "\nlog: " << all_message_text();
	}
}

// The player's own message is fed by the same name, so it is pinned too: a pit underfoot
// is the pit trap a search would have found.
TEST_F(MonsterFeatureEntryTest, ThePlayerSpringingAPitIsToldItIsAPitTrap)
{
	Trap pit{
		Vector2D{ PLAYER_TILE.x + 1, CORRIDOR_ROW },
		TrapType::PIT,
		mock.tile_config,
	};
	// Three on each of the pit's two dice, then the coin that keeps the trap.
	mock.dice.set_next_roll(3);
	mock.dice.set_next_roll(3);
	mock.dice.set_next_roll(2);

	EXPECT_EQ(pit.on_creature_enter(*player, ctx), EntryResult::BLOCKED);

	const std::string log = all_message_text();
	EXPECT_NE(
		log.find("You trigger the pit trap and take 6 damage!"),
		std::string::npos)
		<< log;
}

// The same trap at the far end of the corridor still springs on the goblin and breaks,
// and the log says nothing of either.
TEST_F(MonsterFeatureEntryTest, AMonsterOutOfSightSpringsATrapSilently)
{
	traps.push_back(
		std::make_unique<Trap>(FAR_FEATURE, TrapType::DART, mock.tile_config));
	goblin.position = FAR_TILE;
	player_looks();
	ASSERT_FALSE(map.is_in_fov(FAR_TILE))
		<< "the goblin stands past the player's sight";
	mock.dice.set_next_roll(DART_ROLL);
	// The coin destroys the trap, and that goes unsaid as well.
	mock.dice.set_next_roll(1);
	const int healthBefore = goblin.get_hp();

	EXPECT_EQ(
		Pathfinding::step(goblin, FAR_FEATURE, ctx),
		Pathfinding::StepResult::STOPPED);

	EXPECT_EQ(goblin.get_hp(), healthBefore - DART_ROLL)
		<< "unseen is not unhurt";
	EXPECT_EQ(all_message_text(), "")
		<< "the player heard about a trap it cannot see";
}

// A web holds a monster as it holds the player: the step stops, the goblin is stuck, and
// the log names it.
TEST_F(MonsterFeatureEntryTest, AWebCatchesAMonsterInSight)
{
	// Strength 1 against Dexterity 10 is a 50 percent catch.
	spellTiles.push_back(
		std::make_unique<Web>(NEAR_FEATURE, 1, mock.tile_config));
	player_looks();
	// The catch roll, then the one extra turn stuck.
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(1);

	EXPECT_EQ(
		Pathfinding::step(goblin, NEAR_FEATURE, ctx),
		Pathfinding::StepResult::STOPPED);

	EXPECT_TRUE(goblin.is_webbed()) << "the web let the goblin go";
	EXPECT_EQ(goblin.position, NEAR_TILE)
		<< "a caught goblin stays where it stepped from";
	const std::string log = all_message_text();
	EXPECT_NE(
		log.find("The goblin is caught in a sticky web!"),
		std::string::npos)
		<< log;
	EXPECT_EQ(log.find("You"), std::string::npos) << log;
}

// A web the player cannot see catches a goblin without a word.
TEST_F(MonsterFeatureEntryTest, AWebOutOfSightCatchesSilently)
{
	spellTiles.push_back(
		std::make_unique<Web>(FAR_FEATURE, 1, mock.tile_config));
	goblin.position = FAR_TILE;
	player_looks();
	ASSERT_FALSE(map.is_in_fov(FAR_TILE));
	mock.dice.set_next_roll(1);
	mock.dice.set_next_roll(1);

	EXPECT_EQ(
		Pathfinding::step(goblin, FAR_FEATURE, ctx),
		Pathfinding::StepResult::STOPPED);

	EXPECT_TRUE(goblin.is_webbed());
	EXPECT_EQ(all_message_text(), "")
		<< "the player heard about a web it cannot see";
}

// end of file: MonsterFeatureEntryTest.cpp
