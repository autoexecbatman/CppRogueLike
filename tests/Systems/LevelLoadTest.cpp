// file: LevelLoadTest.cpp
// A load replaces the level. The in-game menu offers Load Game, so a load can arrive
// while another level is still in memory, and nothing regenerates the map first: each
// loader has to clear what it restores, or the abandoned level's contents stay beside
// the saved one's.
//
// Every record is built in the test and read through the exposed loaders, so no test
// here opens the save file.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=LevelLoadTest.*

#include <gtest/gtest.h>

#include <memory>
#include <string>
#include <vector>

#include "src/Ai.h"
#include "src/ArmorClass.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/Decoration.h"
#include "src/DungeonRoom.h"
#include "src/ExperienceReward.h"
#include "src/GameStateManager.h"
#include "src/HealthPool.h"
#include "src/Persistent.h"
#include "src/Renderer.h"
#include "src/Vector2D.h"

class LevelLoadTest : public ::testing::Test
{
protected:
	// A creature with the parts a save writes, named so a load can be told apart.
	static std::unique_ptr<Creature> creature_named(const std::string& name)
	{
		auto creature = std::make_unique<Creature>(Vector2D{ 3, 3 }, ActorData{ TileRef{}, name, ColorPairId::WHITE_BLACK });
		creature->experienceReward = std::make_unique<ExperienceReward>(0);
		creature->armorClass = std::make_unique<ArmorClass>(10);
		creature->healthPool = std::make_unique<HealthPool>(6);
		creature->ai = Ai::make(AiType::MONSTER);
		return creature;
	}

	static DungeonRoom room_at(int col)
	{
		DungeonRoom room{};
		room.col = col;
		room.row = 2;
		room.width = 4;
		room.height = 3;
		return room;
	}

	// A decoration with every field set away from its default, so a field the load
	// forgets reads back wrong.
	static std::unique_ptr<Decoration> barrel_at(Vector2D where, int hitPointsLeft)
	{
		auto barrel = std::make_unique<Decoration>();
		barrel->position = where;
		barrel->tile = TileRef{ TileSheet::SHEET_DECOR0, 4, 6 };
		barrel->name = "barrel";
		barrel->hp = hitPointsLeft;
		barrel->blocks_movement = false;
		barrel->lootTableKey = "gold";
		barrel->isBroken = true;
		return barrel;
	}
};

// A level abandoned with one goblin, loaded from a save of an orc and a kobold, holds
// the orc and the kobold.
TEST_F(LevelLoadTest, LoadingCreaturesReplacesTheAbandonedLevels)
{
	std::vector<std::unique_ptr<Creature>> saved{};
	saved.push_back(creature_named("orc"));
	saved.push_back(creature_named("kobold"));
	json record{};
	record["creatures"] = json::array();
	for (const auto& creature : saved)
	{
		json creatureJson{};
		creature->save(creatureJson);
		record["creatures"].push_back(creatureJson);
	}

	std::vector<std::unique_ptr<Creature>> level{};
	level.push_back(creature_named("goblin"));
	load_creatures(record, level);

	ASSERT_EQ(level.size(), 2u) << "the abandoned level's goblin is still there";
	EXPECT_EQ(level.at(0)->actorData.name, "orc");
	EXPECT_EQ(level.at(1)->actorData.name, "kobold");
}

// The same for rooms: a level of two rooms loaded from a save of one has one.
TEST_F(LevelLoadTest, LoadingRoomsReplacesTheAbandonedLevels)
{
	json record{};
	record["rooms"] = json::array();
	record["rooms"].push_back({ { "col", 20 }, { "row", 2 }, { "width", 4 }, { "height", 3 }, { "type", 0 } });

	std::vector<DungeonRoom> level{ room_at(1), room_at(9) };
	load_rooms(record, level);

	ASSERT_EQ(level.size(), 1u) << "the abandoned level's rooms are still there";
	EXPECT_EQ(level.at(0).col, 20);
}

// Every field of a decoration survives a save and load: where it stands, what it looks
// like, what it is called, the hit points it has left, whether it blocks, what it drops,
// and whether it is broken.
TEST_F(LevelLoadTest, ADecorationComesBackWithEveryField)
{
	std::vector<std::unique_ptr<Decoration>> saved{};
	// An editor decoration of 3 hit points, struck once. Not 1, which is the field's default.
	saved.push_back(barrel_at(Vector2D{ 5, 7 }, 2));
	json record{};
	save_decorations(saved, record);

	std::vector<std::unique_ptr<Decoration>> level{};
	load_decorations(record, level);

	ASSERT_EQ(level.size(), 1u) << "the barrel was not saved, or not loaded";
	const Decoration& barrel = *level.at(0);
	EXPECT_EQ(barrel.position, (Vector2D{ 5, 7 }));
	EXPECT_EQ(barrel.tile, (TileRef{ TileSheet::SHEET_DECOR0, 4, 6 }));
	EXPECT_EQ(barrel.name, "barrel");
	EXPECT_EQ(barrel.hp, 2) << "the hit points it had left were not restored";
	EXPECT_FALSE(barrel.blocks_movement);
	EXPECT_EQ(barrel.lootTableKey, "gold");
	EXPECT_TRUE(barrel.isBroken);
}

// A level abandoned with a crate, loaded from a save of one barrel, holds the barrel.
TEST_F(LevelLoadTest, LoadingDecorationsReplacesTheAbandonedLevels)
{
	std::vector<std::unique_ptr<Decoration>> saved{};
	saved.push_back(barrel_at(Vector2D{ 5, 7 }, 2));
	json record{};
	save_decorations(saved, record);

	std::vector<std::unique_ptr<Decoration>> level{};
	level.push_back(barrel_at(Vector2D{ 11, 2 }, 2));
	load_decorations(record, level);

	ASSERT_EQ(level.size(), 1u) << "the abandoned level's decoration is still there, or the saved one is missing";
	EXPECT_EQ(level.at(0)->position, (Vector2D{ 5, 7 }));
}

// end of file: LevelLoadTest.cpp
