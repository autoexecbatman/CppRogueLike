// file: KnockSpellTest.cpp
// What casting Knock does to a locked door in reach. The 2e spell "opens
// stuck, barred, locked, held, or wizard-locked doors" and in all but the
// wizard lock "permanently opens locks" (the 2e text extracted from the
// repo-root PDF, page 292, the Knock entry). So the door ends unlocked and
// open, and the casting is spent.
//
// The level is a strip of floor with a locked door two tiles from the
// caster, inside the reach cast_knock searches.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=KnockSpellTest.*

#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "src/ArmorClass.h"
#include "src/Creature.h"
#include "src/Decoration.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/Map.h"
#include "src/Player.h"
#include "src/SpellSystem.h"
#include "src/TileType.h"
#include "src/Vector2D.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
	constexpr Vector2D CASTER_TILE{ 5, 5 };
	constexpr Vector2D DOOR_TILE{ 5, 7 };

	// set_door is protected, so the door is placed through a type that exposes
	// it rather than by widening the interface the game uses.
	class MapWithALockedDoor : public Map
	{
	public:
		using Map::Map;
		using Map::set_door;
	};
} // namespace

class KnockSpellTest : public ::testing::Test
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
		map.set_tile(CASTER_TILE, TileType::FLOOR);
		map.set_tile(Vector2D{ 5, 6 }, TileType::FLOOR);
		map.set_door(DOOR_TILE);
		map.set_tile(Vector2D{ 5, 8 }, TileType::FLOOR);

		player->healthPool = std::make_unique<HealthPool>(20);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->set_creature_level(3);
		player->set_strength(18);
	}

	MockGameContext mock{};
	GameContext ctx{};
	MapWithALockedDoor map{ 20, 20 };
	std::vector<std::unique_ptr<Creature>> creatures{};
	std::vector<std::unique_ptr<Decoration>> decorations{};
	std::unique_ptr<Player> player{ std::make_unique<Player>(CASTER_TILE) };
};

TEST_F(KnockSpellTest, KnockOpensALockedDoorInReachAndLeavesItUnlocked)
{
	ASSERT_TRUE(map.lock_door(DOOR_TILE));
	int castingsSpent = 0;
	const auto count_the_casting = [&castingsSpent](GameContext& completedCtx)
	{
		++castingsSpent;
	};

	SpellSystem::cast_spell_by_key(
		"knock",
		*player,
		SpellSource::MEMORIZED,
		count_the_casting,
		ctx);

	EXPECT_FALSE(map.is_door_locked(DOOR_TILE)) << "the lock held";
	EXPECT_TRUE(map.is_open_door(DOOR_TILE)) << "the door stayed shut";
	EXPECT_EQ(castingsSpent, 1) << "the casting was not spent";
}

// The same door and caster with another spell: the door stays locked, so
// the opening above is Knock's own.
TEST_F(KnockSpellTest, AnotherSpellLeavesTheDoorLocked)
{
	ASSERT_TRUE(map.lock_door(DOOR_TILE));
	const auto ignore_the_casting = [](GameContext& completedCtx) {};

	SpellSystem::cast_spell_by_key(
		"shield",
		*player,
		SpellSource::MEMORIZED,
		ignore_the_casting,
		ctx);

	EXPECT_TRUE(map.is_door_locked(DOOR_TILE));
	EXPECT_FALSE(map.is_open_door(DOOR_TILE));
}
