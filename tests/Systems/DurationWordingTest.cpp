// file: DurationWordingTest.cpp
//
// Every message that tells the player how long an effect lasts names the count in rounds,
// the unit the game's clock and every buff duration are counted in. The book's turn is ten
// rounds ("Ten combat rounds equal a turn", PHB PDF page 184), so a round count printed
// as turns overstates the effect tenfold.
//
// Each test drinks or casts with a known duration from a 1st-level caster and reads the
// stored message. The expected counts come from the rules: a potion of 3 turns lasts 30
// rounds; Protection From Evil, Silence, Web and Hold Person last 2 rounds per level;
// Sanctuary 2 rounds plus 1 per level; Bless 6 rounds (PDF page 426); Shield 5 rounds as
// the game has it, which is issue 0266's question.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=DurationWordingTest.*

#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "src/AiMonster.h"
#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Creature.h"
#include "src/DiceExpr.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/InputSystem.h"
#include "src/Item.h"
#include "src/Map.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/SpellSystem.h"
#include "tests/mocks/MockGameContext.h"

class DurationWordingTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.creatures = &creatures;
		ctx.playerOwner = &caster;
		ctx.map = &map;
		ctx.buffSystem = &buffs;

		caster->healthPool = std::make_unique<HealthPool>(20);
		caster->armorClass = std::make_unique<ArmorClass>(10);
		caster->experienceReward = std::make_unique<ExperienceReward>(0);
		caster->set_creature_level(1);

		// An open row so the target beside the caster is in view.
		map.init_tiles();
		for (int col = 1; col < 19; ++col)
		{
			map.set_tile(Vector2D{ col, 5 }, TileType::FLOOR);
		}
		caster->position = Vector2D{ 2, 5 };
		map.compute_fov(ctx);

		auto orc = std::make_unique<Creature>(Vector2D{ 3, 5 }, ActorData{ TileRef{}, "orc", ColorPairId::WHITE_BLACK });
		orc->healthPool = std::make_unique<HealthPool>(20);
		orc->experienceReward = std::make_unique<ExperienceReward>(0);
		orc->ai = std::make_unique<AiMonster>();
		creatures.push_back(std::move(orc));
	}

	void script(std::initializer_list<int> rolls)
	{
		for (const int roll : rolls)
		{
			mock.dice.set_next_roll(roll);
		}
	}

	void cast(const char* key)
	{
		const auto ignore_completion = [](GameContext& completedCtx) {
		};
		SpellSystem::cast_spell_by_key(key, *caster, SpellSource::MEMORIZED, ignore_completion, ctx);
	}

	// Feeds the open targeting menu one key per frame, as the game loop would.
	void target_with(std::initializer_list<GameKey> keys)
	{
		ASSERT_FALSE(ctx.menus->empty()) << "the spell opened no targeting menu";
		for (const GameKey key : keys)
		{
			ctx.inputSystem->queue_key(key, 0);
			ctx.menus->back()->menu(ctx);
		}
	}

	// Every part of every finalized message, joined.
	std::string all_message_text() const
	{
		std::string joined;
		for (size_t index = 0; index < ctx.messageSystem->get_stored_message_count(); ++index)
		{
			for (const auto& part : ctx.messageSystem->get_attack_message_at(index))
			{
				joined += part.text;
			}
		}
		return joined;
	}

	// The message names the count in rounds and never in turns.
	void expect_rounds(const std::string& count)
	{
		const std::string text = all_message_text();
		EXPECT_NE(text.find(count), std::string::npos) << "expected \"" << count << "\" in: " << text;
		EXPECT_EQ(text.find("turn"), std::string::npos) << "a round count named as turns: " << text;
	}

	MockGameContext mock{};
	GameContext ctx{};
	BuffSystem buffs{};
	Map map{ 20, 20 };
	std::unique_ptr<Player> caster{ std::make_unique<Player>(Vector2D{ 2, 5 }) };
	std::vector<std::unique_ptr<Creature>> creatures{};
};

// A potion of three turns lasts thirty rounds, and says so.
TEST_F(DurationWordingTest, ABuffPotionNamesItsRounds)
{
	Item potion{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "potion", ColorPairId::WHITE_BLACK } };
	Consumable consumable{ ConsumableEffect::ADD_BUFF, DiceExpr{ 0, 0, 1 }, DiceExpr{ 0, 0, 3 }, DurationUnit::TURNS, BuffType::STRENGTH, BuffApplication::ADD };

	// The potion is in no pack, so nothing is used up and use() answers false; the buff and
	// its message come first.
	[[maybe_unused]] const bool consumed = use(consumable, potion, *caster, ctx);

	expect_rounds("30 rounds");
}

// Protection From Evil lasts 2 rounds per level.
TEST_F(DurationWordingTest, ProtectionFromEvilNamesItsRounds)
{
	cast("protection_from_evil");

	expect_rounds("2 rounds");
}

// Sanctuary lasts 2 rounds plus 1 per level.
TEST_F(DurationWordingTest, SanctuaryNamesItsRounds)
{
	cast("sanctuary");

	expect_rounds("3 rounds");
}

// Bless lasts 6 rounds.
TEST_F(DurationWordingTest, BlessNamesItsRounds)
{
	cast("bless");

	expect_rounds("6 rounds");
}

// Shield lasts the 5 rounds the game gives it.
TEST_F(DurationWordingTest, ShieldNamesItsRounds)
{
	cast("shield");

	expect_rounds("5 rounds");
}

// Hold Person rolls how many it may hold - one - and the orc fails its save.
TEST_F(DurationWordingTest, HoldPersonNamesItsRounds)
{
	script({ 1, 1 });

	cast("hold_person");

	expect_rounds("2 rounds");
}

// Silence lasts 2 rounds per level. The targeting cursor starts on the caster, and a frame
// that moves it draws the map, which needs a window; so the orc stands where it starts.
TEST_F(DurationWordingTest, SilenceNamesItsRounds)
{
	creatures.front()->position = caster->position;
	cast("silence");
	target_with({ GameKey::ENTER });

	expect_rounds("2 rounds");
}

// Web, centred on the caster with the orc inside it, lasts 2 rounds per level; the orc
// fails its save.
TEST_F(DurationWordingTest, WebNamesItsRounds)
{
	cast("web");
	script({ 1 });
	target_with({ GameKey::ENTER });

	expect_rounds("2 rounds");
}
