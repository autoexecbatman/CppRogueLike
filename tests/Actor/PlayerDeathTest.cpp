// file: PlayerDeathTest.cpp
// What Player::die is allowed to touch.
//
// Player::die is a domain event: the character's hit points ran out, so the run
// is lost. Declaring defeat is its whole job. Permadeath housekeeping - removing
// the save so the dead character cannot be reloaded - is a game-shell concern and
// belongs where the loop handles DEFEAT, because a unit test that kills a player
// must not reach the player's filesystem.
//
// That is the regression these tests hold. Until 2026-09-30 die() deleted the save
// itself, so every suite covering a death path destroyed saves/game.sav; the suite
// did exactly that once the illegal save filename was repaired in dada234.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=PlayerDeathTest.*

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <memory>
#include <string>

#include "src/ArmorClass.h"
#include "src/GameContext.h"
#include "src/HealthPool.h"
#include "src/Paths.h"
#include "src/Player.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
// Content chosen so a failure names itself in the diff rather than looking like
// a real save that happened to survive.
const std::string SENTINEL_SAVE = "PlayerDeathTest sentinel - not a real save";
} // namespace

class PlayerDeathTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.playerOwner = &player;

		player->healthPool = std::make_unique<HealthPool>(20);
		player->armorClass = std::make_unique<ArmorClass>(10);
	}

	void TearDown() override
	{
		// Only ever removes the file this fixture wrote. A save that was already
		// there is never touched, because the test skips instead of overwriting.
		if (wroteSentinel)
		{
			std::error_code ignored;
			std::filesystem::remove(Paths::resolve(Paths::SAVE_FILE), ignored);
		}
	}

	// Whether a real save is sitting where this test wants to write. Overwriting the
	// owner's file to prove the owner's file is safe is the accident this test exists
	// about, so the test skips rather than clobbering one.
	static bool a_save_already_exists()
	{
		return std::filesystem::exists(Paths::resolve(Paths::SAVE_FILE));
	}

	// Writes the stand-in save. Sets wroteSentinel, which is both what TearDown
	// cleans up and what says the write landed - a full disk reports as a failed
	// write here rather than as a save that was already there.
	void place_sentinel_save()
	{
		const auto savePath = Paths::resolve(Paths::SAVE_FILE);
		std::error_code ignored;
		std::filesystem::create_directories(savePath.parent_path(), ignored);
		std::ofstream out{ savePath };
		out << SENTINEL_SAVE;
		out.close();
		wroteSentinel = std::filesystem::exists(savePath);
	}

	static std::string save_contents()
	{
		std::ifstream in{ Paths::resolve(Paths::SAVE_FILE) };
		std::string text;
		std::getline(in, text);
		return text;
	}

	MockGameContext mock{};
	GameContext ctx{};
	std::unique_ptr<Player> player{ std::make_unique<Player>(Vector2D{ 1, 1 }) };
	bool wroteSentinel{ false };
};

// The one thing dying is for: the run is over and the shell has to be told.
TEST_F(PlayerDeathTest, DyingDeclaresDefeat)
{
	player->die(ctx);

	EXPECT_EQ(ctx.gameState->get_game_status(), GameStatus::DEFEAT)
		<< "the player died and the game was not told the run was lost";
}

// The regression: a death in a unit test must leave the filesystem alone.
TEST_F(PlayerDeathTest, DyingLeavesTheSaveFileAlone)
{
	if (a_save_already_exists())
	{
		GTEST_SKIP() << "a save already exists at " << Paths::resolve(Paths::SAVE_FILE).string()
					 << "; refusing to overwrite it to run this test";
	}
	place_sentinel_save();
	ASSERT_TRUE(wroteSentinel) << "could not write the stand-in save, so nothing here is being measured";

	player->die(ctx);

	ASSERT_TRUE(std::filesystem::exists(Paths::resolve(Paths::SAVE_FILE)))
		<< "Player::die deleted the save file; permadeath housekeeping belongs in the game loop";
	EXPECT_EQ(save_contents(), SENTINEL_SAVE) << "the save survived but its contents were rewritten";
}
