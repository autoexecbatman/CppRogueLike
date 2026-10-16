// file: TeleportLandingTest.cpp
// Where a teleport lands, and what it does when there is nowhere to land.
//
// find_random_floor_tile enumerates every floor tile a creature could be put on and
// picks one, so a level with none answers nothing instead of searching forever. Both
// teleports - the scroll and the spell - then leave the creature where it stands, say
// so, and spend the scroll or the casting, as Knock does when no door is in reach.
//
// The level is one 3 by 3 room. "Full" means a creature on every tile of it but the
// player's own, which the player occupies.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=TeleportLandingTest.*

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "src/ArmorClass.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/Decoration.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/LogMessage.h"
#include "src/Map.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/SpawnUtils.h"
#include "src/SpellSystem.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr int ROOM_COL = 2;
constexpr int ROOM_ROW = 2;
constexpr int ROOM_SIZE = 3;
constexpr Vector2D PLAYER_TILE{ ROOM_COL, ROOM_ROW };
constexpr Vector2D LAST_FREE_TILE{ ROOM_COL + ROOM_SIZE - 1, ROOM_ROW + ROOM_SIZE - 1 };
// Enough draws that a picker ignoring what stands on a tile lands on a taken one.
constexpr int DRAWS = 30;
} // namespace

class TeleportLandingTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.playerOwner = &player;
		ctx.creatures = &creatures;
		ctx.decorations = &decorations;
		ctx.map = &map;

		map.init_tiles();
		for (int column = ROOM_COL; column < ROOM_COL + ROOM_SIZE; ++column)
		{
			for (int row = ROOM_ROW; row < ROOM_ROW + ROOM_SIZE; ++row)
			{
				map.set_tile(Vector2D{ column, row }, TileType::FLOOR);
			}
		}

		player->healthPool = std::make_unique<HealthPool>(20);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->set_creature_level(1);
		// A creature built in a test has no Strength, and the pack is weight-gated by it.
		player->set_strength(18);
	}

	// A creature on every tile of the room but the player's and, when asked, one other.
	void crowd_the_room(std::optional<Vector2D> leftFree)
	{
		for (int column = ROOM_COL; column < ROOM_COL + ROOM_SIZE; ++column)
		{
			for (int row = ROOM_ROW; row < ROOM_ROW + ROOM_SIZE; ++row)
			{
				const Vector2D here{ column, row };
				if (here == PLAYER_TILE || here == leftFree)
				{
					continue;
				}
				auto orc = std::make_unique<Creature>(here, ActorData{ TileRef{}, "orc", ColorPairId::WHITE_BLACK });
				orc->healthPool = std::make_unique<HealthPool>(8);
				creatures.push_back(std::move(orc));
			}
		}
	}

	// Whether any stored message holds the given text, its coloured runs joined.
	bool said_anything_about(std::string_view wanted) const
	{
		for (size_t index = 0; index < ctx.messageSystem->get_stored_message_count(); ++index)
		{
			std::string whole;
			for (const LogMessage& run : ctx.messageSystem->get_attack_message_at(index))
			{
				whole += run.text;
			}
			if (whole.find(wanted) != std::string::npos)
			{
				return true;
			}
		}
		return false;
	}

	// Reads a scroll of teleportation from the player's pack and reports whether it was spent.
	bool read_teleport_scroll()
	{
		auto scroll = ItemCreator::create("scroll_teleport", PLAYER_TILE, ctx);
		Item* held = scroll.get();
		[[maybe_unused]] const auto added = InventoryOperations::add_item(player->inventoryData, std::move(scroll));
		Teleporter behavior{};
		return use(behavior, *held, *player, ctx);
	}

	MockGameContext mock{};
	GameContext ctx{};
	Map map{ 20, 20 };
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::vector<std::unique_ptr<Decoration>> decorations{};
	std::unique_ptr<Player> player{ std::make_unique<Player>(PLAYER_TILE) };
};

// With one tile of the room left free, that tile is the only answer, every time.
TEST_F(TeleportLandingTest, TheOneFreeFloorTileIsTheOneOffered)
{
	crowd_the_room(LAST_FREE_TILE);

	for (int draw = 0; draw < DRAWS; ++draw)
	{
		const std::optional<Vector2D> landing = SpawnUtils::find_random_floor_tile(ctx);
		ASSERT_TRUE(landing.has_value()) << "the free tile was never offered";
		ASSERT_EQ(*landing, LAST_FREE_TILE) << "draw " << draw << " offered a taken tile";
	}
}

// Every tile taken: the answer is nothing, and it comes back rather than searching forever.
TEST_F(TeleportLandingTest, AFullLevelOffersNowhere)
{
	crowd_the_room(std::nullopt);

	EXPECT_FALSE(SpawnUtils::find_random_floor_tile(ctx).has_value());
}

// A teleport lands on room floor: a free corridor tile beside a full room is no landing.
TEST_F(TeleportLandingTest, ACorridorIsNoLanding)
{
	crowd_the_room(std::nullopt);
	map.set_tile(Vector2D{ ROOM_COL + ROOM_SIZE, ROOM_ROW }, TileType::CORRIDOR);

	EXPECT_FALSE(SpawnUtils::find_random_floor_tile(ctx).has_value()) << "the corridor was offered";
}

// Water is no landing for a creature that cannot swim.
TEST_F(TeleportLandingTest, AFloodedLevelOffersNowhere)
{
	for (int column = ROOM_COL; column < ROOM_COL + ROOM_SIZE; ++column)
	{
		for (int row = ROOM_ROW; row < ROOM_ROW + ROOM_SIZE; ++row)
		{
			const Vector2D here{ column, row };
			if (here != PLAYER_TILE)
			{
				map.set_tile(here, TileType::WATER);
			}
		}
	}

	EXPECT_FALSE(SpawnUtils::find_random_floor_tile(ctx).has_value());
}

// The scroll on a full level: the reader stays, is told, and the scroll is spent.
TEST_F(TeleportLandingTest, AScrollWithNowhereToLandLeavesTheReaderWhereTheyStand)
{
	crowd_the_room(std::nullopt);

	const bool spent = read_teleport_scroll();

	EXPECT_EQ(player->position, PLAYER_TILE) << "the reader was moved with nowhere to go";
	EXPECT_TRUE(said_anything_about("nowhere to take you")) << "the scroll failed without a word";
	EXPECT_TRUE(spent) << "reading the scroll is the cost, landing or not";
}

// The scroll with somewhere to go moves the reader there.
TEST_F(TeleportLandingTest, AScrollWithSomewhereToLandMovesTheReaderThere)
{
	crowd_the_room(LAST_FREE_TILE);

	read_teleport_scroll();

	EXPECT_EQ(player->position, LAST_FREE_TILE);
}

// The spell on a full level: the caster stays, is told, and the casting is spent, as
// Knock's is with no door in reach. The completion callback runs only for a spent casting.
TEST_F(TeleportLandingTest, TheSpellWithNowhereToLandLeavesTheCasterWhereTheyStand)
{
	crowd_the_room(std::nullopt);
	int castingsSpent = 0;
	const auto count_the_casting = [&castingsSpent](GameContext& completedCtx)
	{
		++castingsSpent;
	};

	SpellSystem::cast_spell_by_key("teleport", *player, SpellSource::MEMORIZED, count_the_casting, ctx);

	EXPECT_EQ(player->position, PLAYER_TILE) << "the caster was moved with nowhere to go";
	EXPECT_TRUE(said_anything_about("nowhere to take you")) << "the spell failed without a word";
	EXPECT_EQ(castingsSpent, 1) << "a casting with nowhere to go was not spent";
}

// end of file: TeleportLandingTest.cpp
