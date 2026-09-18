// Checks that the spells and scrolls which single out a creature respect its Sanctuary.
//
// What it is for. The Player's Handbook, PDF page 436 of the 2e archive: an opponent
// "attempting to strike or otherwise directly attack the protected creature must roll a
// saving throw vs. spell", and on a failure "totally ignores the warded creature". Magic
// missile strikes a target the caster singles out (page 274), and Hold Person "affects
// persons selected by the caster" (page 309), so a caster turned away passes the warded
// creature over and the spell goes to another. The lightning scroll takes the nearest
// creature it may strike; the confusion scroll, aimed at a warded creature, stays
// unread. Area effects - fireball, Sleep, Silence - are not direct attacks and are left
// as they are.
//
// The warded goblin stands nearest the caster, the orc beyond it. Every roll is scripted
// in the order the game asks for it, and the roll that follows the caster's save would
// land on the goblin if the save were skipped or ignored.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=SanctuarySpellTest.*

#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>
#include <vector>

#include "src/AiMonster.h"
#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Creature.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/Map.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/SavingThrow.h"
#include "src/SpellSystem.h"
#include "src/TargetingSystem.h"
#include "tests/mocks/MockGameContext.h"

class SanctuarySpellTest : public ::testing::Test
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

		// An open row so everything placed on it is in view of the caster.
		map.init_tiles();
		for (int col = 1; col < 19; ++col)
		{
			map.set_tile(Vector2D{ col, 5 }, TileType::FLOOR, 1.0);
		}
		caster->position = Vector2D{ 2, 5 };
		map.compute_fov(ctx);

		goblin = &add_creature("goblin", 4);
		orc = &add_creature("orc", 6);
		buffs.add_buff(*goblin, BuffType::SANCTUARY, 0, 10, false);
	}

	Creature& add_creature(const char* name, int column)
	{
		auto creature = std::make_unique<Creature>(Vector2D{ column, 5 }, ActorData{ TileRef{}, name, 0 });
		creature->healthPool = std::make_unique<HealthPool>(STARTING_HP);
		creature->experienceReward = std::make_unique<ExperienceReward>(0);
		creature->ai = std::make_unique<AiMonster>();
		creatures.push_back(std::move(creature));
		return *creatures.back();
	}

	// Queues rolls in the order the game asks for them.
	void script(std::initializer_list<int> rolls)
	{
		for (const int roll : rolls)
		{
			mock.dice.set_next_roll(roll);
		}
	}

	void cast(const char* key)
	{
		SpellSystem::cast_spell_by_key(key, *caster, [](GameContext&) {}, ctx);
	}

	static int hp_lost(const Creature& creature)
	{
		return STARTING_HP - creature.get_hp();
	}

	static constexpr int STARTING_HP = 50;

	MockGameContext mock{};
	GameContext ctx{};
	BuffSystem buffs{};
	Map map{ 20, 20 };
	std::unique_ptr<Player> caster{ std::make_unique<Player>(Vector2D{ 2, 5 }) };
	std::vector<std::unique_ptr<Creature>> creatures{};
	Creature* goblin{ nullptr };
	Creature* orc{ nullptr };
};

// The rolls mean what the tests say: 1 fails the caster's save against the spell and 20
// makes it, and a 1st-level caster fires one missile.
TEST_F(SanctuarySpellTest, TheScriptedRollsMeanWhatTheTestsSay)
{
	const int needed = SavingThrows::target(caster->get_creature_class(), caster->get_creature_level(), SavingThrow::SPELL);
	EXPECT_GT(needed, 1);
	EXPECT_LE(needed, 20);
}

// The caster fails against the goblin's Sanctuary, so the one missile goes to the orc:
// 3 on the die, plus one, is four.
TEST_F(SanctuarySpellTest, AMissilePassesOverAWardedCreature)
{
	script({ 1, 3 });

	cast("magic_missile");

	EXPECT_EQ(hp_lost(*goblin), 0) << "the missile struck a creature the caster ignores";
	EXPECT_EQ(hp_lost(*orc), 4) << "the missile was not sent on to the next nearest";
}

// A caster that saves strikes the warded goblin as usual.
TEST_F(SanctuarySpellTest, AMissileFromACasterThatSavedStrikesTheWardedCreature)
{
	script({ 20, 3 });

	cast("magic_missile");

	EXPECT_EQ(hp_lost(*goblin), 4);
}

// Hold Person rolls how many it may hold - one - then selects: the caster fails against
// the goblin and passes it over, and the orc fails its own save and is held.
TEST_F(SanctuarySpellTest, HoldPersonSelectsPastAWardedCreature)
{
	script({ 1, 1, 1 });

	cast("hold_person");

	EXPECT_FALSE(goblin->has_state(ActorState::IS_HELD)) << "held a creature the caster ignores";
	EXPECT_TRUE(orc->has_state(ActorState::IS_HELD)) << "the hold was not sent on to another";
}

// The lightning scroll's target is the nearest creature the reader may strike.
TEST_F(SanctuarySpellTest, TheNearestTargetIsOneTheReaderMayStrike)
{
	const TargetingSystem targeting{};
	script({ 1 });

	const TargetResult result = targeting.acquire_nearest(ctx, *caster, 10);

	ASSERT_TRUE(result.success);
	EXPECT_EQ(result.creatures.front(), orc) << "chose a creature the reader ignores";
}

// A reader that saves may take the warded goblin, the nearest.
TEST_F(SanctuarySpellTest, AReaderThatSavedTakesTheNearestWardedCreature)
{
	const TargetingSystem targeting{};
	script({ 20 });

	const TargetResult result = targeting.acquire_nearest(ctx, *caster, 10);

	ASSERT_TRUE(result.success);
	EXPECT_EQ(result.creatures.front(), goblin);
}

// Nearest means nearest, not first in the list: a kobold added last but standing closer
// than the goblin is taken with no save rolled. Without the ordering the warded goblin
// would come first, and the 20 would carry its save.
TEST_F(SanctuarySpellTest, TheNearestIsTakenWhateverTheListOrder)
{
	const TargetingSystem targeting{};
	Creature& kobold = add_creature("kobold", 3);
	script({ 20 });

	const TargetResult result = targeting.acquire_nearest(ctx, *caster, 10);

	ASSERT_TRUE(result.success);
	EXPECT_EQ(result.creatures.front(), &kobold);
}

// A confusion scroll aimed at the warded goblin stays unread, and the goblin keeps its wits.
TEST_F(SanctuarySpellTest, AConfusionScrollAimedAtAWardedCreatureIsKept)
{
	const Ai* before = goblin->ai.get();
	script({ 1 });

	EXPECT_EQ(read_confusion_at(*caster, goblin->position, 5, ctx), ScrollReading::KEPT);
	EXPECT_EQ(goblin->ai.get(), before) << "a creature the reader ignores was confused";
}

// Sanctuary itself lasts "2 rds. + 1 rd./level" (PHB page 436), a round being a turn
// here: three turns at 1st level, seven at 5th. Two levels pin both terms.
TEST_F(SanctuarySpellTest, SanctuaryLastsTwoRoundsPlusOnePerLevel)
{
	cast("sanctuary");
	EXPECT_EQ(buffs.get_buff_turns(*caster, BuffType::SANCTUARY), 3) << "1st level: 2 + 1";

	buffs.remove_buff(*caster, BuffType::SANCTUARY);
	caster->set_creature_level(5);
	cast("sanctuary");
	EXPECT_EQ(buffs.get_buff_turns(*caster, BuffType::SANCTUARY), 7) << "5th level: 2 + 5";
}

// Aimed at the unwarded orc, it is read, and the orc is confused.
TEST_F(SanctuarySpellTest, AConfusionScrollAimedAtAnUnwardedCreatureIsRead)
{
	const Ai* before = orc->ai.get();

	EXPECT_EQ(read_confusion_at(*caster, orc->position, 5, ctx), ScrollReading::SPENT);
	EXPECT_NE(orc->ai.get(), before) << "the orc was not confused";
}
