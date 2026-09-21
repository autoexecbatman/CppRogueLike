// file: SaveFieldNamesTest.cpp
// What a save writes is what a load reads, under the same name, and a record missing a
// field is refused rather than defaulted.
//
// What it is for. Three fields were written under one key and read under another, and
// every one of them failed silently because the read sat behind a contains() guard:
// a weapon's "handReq" against "handRequirement", so every two-handed weapon came back
// one-handed; a scroll's "scrollAnimation" against "animation", so every saved scroll
// lost its animation; and a healing potion's amount, read from a legacy key the saver
// stopped writing. A guard that turns a typo into a default is what let all three live.
//
// The owner's ruling, 2026-09-21: "no legacy save fallbacks should exist". Saves are not
// kept compatible in this project, so a loader has one shape to read and may fail loudly
// when it does not find it.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=SaveFieldNamesTest.*

#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "src/LevelManager.h"

using json = nlohmann::json;

TEST(SaveFieldNamesTest, ALevelManagerReadsBackWhatItWrote)
{
	LevelManager written;
	json saved;
	written.save_to_json(saved);

	LevelManager read;
	read.load_from_json(saved);

	EXPECT_EQ(read.get_dungeon_level(), written.get_dungeon_level());
	EXPECT_EQ(read.get_shopkeepers_count(), written.get_shopkeepers_count());
}

TEST(SaveFieldNamesTest, ALevelManagerReadsTheNamesItsSaveUses)
{
	// Written out by hand, so the key names themselves are the claim: if either is
	// renamed on one side only, this is what notices.
	const json saved = { { "level_manager",
		{ { "dungeonLevel", 7 }, { "shopkeepersOnCurrentLevel", 2 } } } };

	LevelManager read;
	read.load_from_json(saved);

	EXPECT_EQ(read.get_dungeon_level(), 7);
	EXPECT_EQ(read.get_shopkeepers_count(), 2);
}

TEST(SaveFieldNamesTest, ALevelManagerRefusesTheOldFlatSave)
{
	// The exact shape the deleted fallback accepted: both fields at the top level, no
	// block. It has to be a complete old save, or the refusal could come from the field
	// that happens to be missing rather than from the shape being gone.
	const json saved = { { "dungeonLevel", 7 }, { "shopkeepersOnCurrentLevel", 2 } };

	LevelManager read;
	EXPECT_THROW(read.load_from_json(saved), json::exception);
}

TEST(SaveFieldNamesTest, ALevelManagerRefusesASaveMissingOnlyTheLevel)
{
	// Everything present but the dungeon level, so nothing else can raise instead: a
	// loader that defaulted this would put the character on level one and say nothing.
	const json saved = { { "level_manager", { { "shopkeepersOnCurrentLevel", 2 } } } };

	LevelManager read;
	EXPECT_THROW(read.load_from_json(saved), json::exception);
}

TEST(SaveFieldNamesTest, ALevelManagerRefusesASaveMissingOnlyTheShopkeeperCount)
{
	const json saved = { { "level_manager", { { "dungeonLevel", 7 } } } };

	LevelManager read;
	EXPECT_THROW(read.load_from_json(saved), json::exception);
}
