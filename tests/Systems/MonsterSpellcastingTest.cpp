// file: MonsterSpellcastingTest.cpp
// A monster casting at the player through the player's own spells.
//
// What it pins, from the 0334 spec and the book:
// - a lich casts as a wizard of 18th level (Monstrous Manual, PDF page 1634), so its magic
//   missile is five missiles, one per two levels to five (PHB 274), and its fireball ten dice,
//   "up to a maximum of 10d6" (PHB 306); its Hit Dice of 11 would give six;
// - a cast spends one memorized copy (PHB 165), and an at-will power nothing;
// - the candidates are the memorized spells in fill order, then the innate powers;
// - a spell is cast only where it can be aimed: magic missile needs a line of sight, a
//   fireball one, its range, and the caster clear of its two-tile burst;
// - the player's own spells keep their own target choice;
// - a monster's cast leaves its dead for the sweep after the creature loop.
//
// The open row: the player at column 2, everything else on row 5 in its view. Dice are
// scripted in the order the game rolls them: one d4 per missile, ten d6 for a fireball, then
// one d20 save per creature in the burst, the creature list first and the player last.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=MonsterSpellcastingTest.*

#include <gtest/gtest.h>

#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "src/AiMonster.h"
#include "src/AiMonsterRanged.h"
#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Creature.h"
#include "src/CreatureClass.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/Map.h"
#include "src/MonsterSpellcasting.h"
#include "src/Player.h"
#include "src/SpellSystem.h"
#include "src/Spellcasting.h"
#include "src/TurnSchedule.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
using Keys = std::vector<std::string>;

constexpr int PLAYER_HP = 200;
constexpr int ONE_DAY = ROUNDS_PER_DAY * TIME_UNITS_PER_ROUND;
} // namespace

class MonsterSpellcastingTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		ctx = mock.to_game_context();
		ctx.creatures = &creatures;
		ctx.playerOwner = &player;
		ctx.map = &map;
		ctx.buffSystem = &buffs;

		player->healthPool = std::make_unique<HealthPool>(PLAYER_HP);
		player->armorClass = std::make_unique<ArmorClass>(10);
		player->experienceReward = std::make_unique<ExperienceReward>(0);
		player->set_creature_level(1);
		// A monster's death credits the player (issues/0347), which levels by its class.
		player->set_creature_class(CreatureClass::FIGHTER);

		map.init_tiles();
		for (int col = 1; col < 29; ++col)
		{
			map.set_tile(Vector2D{ col, 5 }, TileType::FLOOR);
		}
		player->position = Vector2D{ 2, 5 };
		map.compute_fov(ctx);
	}

	Creature& add_creature(const char* name, int column, int hitDice)
	{
		auto creature = std::make_unique<Creature>(
			Vector2D{ column, 5 },
			ActorData{ TileRef{}, name, ColorPairId::WHITE_BLACK });
		creature->healthPool = std::make_unique<HealthPool>(50);
		creature->armorClass = std::make_unique<ArmorClass>(10);
		creature->experienceReward = std::make_unique<ExperienceReward>(0);
		creature->ai = std::make_unique<AiMonster>();
		creature->set_creature_level(hitDice);
		creature->set_corpse(
			std::string{ name } + " corpse",
			0,
			"",
			CorpseEdibility::EDIBLE);
		creatures.push_back(std::move(creature));
		return *creatures.back();
	}

	// A lich of wizard 18 on 11 Hit Dice, holding the spells given.
	Creature& add_lich(int column, Keys memorized)
	{
		Creature& lich = add_creature("lich", column, 11);
		lich.spellcasting.training =
			SpellcasterTraining{ CasterClass::WIZARD, 18 };
		lich.spellcasting.memorizedSpells = std::move(memorized);
		return lich;
	}

	// A pit fiend of 13 Hit Dice with fireball as an innate power of the frequency given.
	Creature& add_pit_fiend(int column, PowerFrequency frequency)
	{
		Creature& pitFiend = add_creature("pit fiend", column, 13);
		pitFiend.spellcasting.innatePowers = {
			InnatePower{ "fireball", frequency, 0 }
		};
		return pitFiend;
	}

	void script(std::initializer_list<int> rolls)
	{
		for (const int roll : rolls)
		{
			mock.dice.set_next_roll(roll);
		}
	}

	// Ten ones for a fireball's dice.
	void script_ten_ones()
	{
		for (int die = 0; die < 10; ++die)
		{
			mock.dice.set_next_roll(1);
		}
	}

	int player_hp_lost() const { return PLAYER_HP - player->get_hp(); }

	MockGameContext mock{};
	GameContext ctx{};
	BuffSystem buffs{};
	Map map{ 30, 20 };
	std::unique_ptr<Player> player{ std::make_unique<Player>(
		Vector2D{ 2, 5 }) };
	std::vector<std::unique_ptr<Creature>> creatures{};
};

// Five missiles of 1d4+1 with every die a 1: ten damage, and one copy spent.
TEST_F(
	MonsterSpellcastingTest,
	ALichCastsFiveMagicMissilesAtThePlayerAndSpendsOneCopy)
{
	Creature& lich = add_lich(6, { "magic_missile", "fireball" });
	script({ 1, 1, 1, 1, 1 });

	EXPECT_EQ(
		MonsterSpellcasting::try_cast_at(lich, *player, ctx),
		MonsterCast::CAST);

	EXPECT_EQ(player_hp_lost(), 10);
	EXPECT_EQ(lich.spellcasting.memorizedSpells, (Keys{ "fireball" }));
	EXPECT_EQ(lich.get_hp(), 50) << "a missile struck the caster";
}

// The fill's order is the candidate order: missile first, then the fireball on the player's
// tile, ten dice and a failed save; then nothing is left.
TEST_F(MonsterSpellcastingTest, ALichCastsItsSpellsInOrderAndThenHasNothing)
{
	Creature& lich = add_lich(6, { "magic_missile", "fireball" });
	script({ 1, 1, 1, 1, 1 });
	ASSERT_EQ(
		MonsterSpellcasting::try_cast_at(lich, *player, ctx),
		MonsterCast::CAST);

	script_ten_ones();
	script({ 1 });
	EXPECT_EQ(
		MonsterSpellcasting::try_cast_at(lich, *player, ctx),
		MonsterCast::CAST);
	EXPECT_EQ(player_hp_lost(), 10 + 10);
	EXPECT_TRUE(lich.spellcasting.memorizedSpells.empty());

	EXPECT_EQ(
		MonsterSpellcasting::try_cast_at(lich, *player, ctx),
		MonsterCast::NOTHING_CAST);
	EXPECT_EQ(player_hp_lost(), 20);
}

// The mage is a 1st-level wizard: one missile, a 3 on the d4 and one more.
TEST_F(MonsterSpellcastingTest, AFirstLevelCasterFiresOneMissile)
{
	Creature& mage = add_creature("mage", 6, 1);
	mage.spellcasting.training = SpellcasterTraining{ CasterClass::WIZARD, 1 };
	mage.spellcasting.memorizedSpells = { "magic_missile" };
	script({ 3 });

	EXPECT_EQ(
		MonsterSpellcasting::try_cast_at(mage, *player, ctx),
		MonsterCast::CAST);

	EXPECT_EQ(player_hp_lost(), 4);
}

// At will at 13 Hit Dice: ten dice, capped, and the player's save of 20 halves them. The power
// is never spent, so the next round it bursts again.
TEST_F(
	MonsterSpellcastingTest,
	APitFiendsFireballIsAtWillAndThePlayerSavesForHalf)
{
	Creature& pitFiend = add_pit_fiend(6, PowerFrequency::AT_WILL);
	script_ten_ones();
	script({ 20 });

	EXPECT_EQ(
		MonsterSpellcasting::try_cast_at(pitFiend, *player, ctx),
		MonsterCast::CAST);
	EXPECT_EQ(player_hp_lost(), 5);

	script_ten_ones();
	script({ 1 });
	EXPECT_EQ(
		MonsterSpellcasting::try_cast_at(pitFiend, *player, ctx),
		MonsterCast::CAST);
	EXPECT_EQ(player_hp_lost(), 5 + 10);
	EXPECT_EQ(pitFiend.spellcasting.innatePowers.front().readyAgainAt, 0);
}

TEST_F(MonsterSpellcastingTest, ADailyPowerWaitsADayOfTheClock)
{
	Creature& pitFiend = add_pit_fiend(6, PowerFrequency::ONCE_PER_DAY);
	const int start = ctx.gameState->get_time();
	script_ten_ones();
	script({ 1 });
	ASSERT_EQ(
		MonsterSpellcasting::try_cast_at(pitFiend, *player, ctx),
		MonsterCast::CAST);

	ctx.gameState->set_time(start + ONE_DAY - 1);
	EXPECT_EQ(
		MonsterSpellcasting::try_cast_at(pitFiend, *player, ctx),
		MonsterCast::NOTHING_CAST);

	ctx.gameState->set_time(start + ONE_DAY);
	script_ten_ones();
	script({ 1 });
	EXPECT_EQ(
		MonsterSpellcasting::try_cast_at(pitFiend, *player, ctx),
		MonsterCast::CAST);
}

// A wall between them: no line of sight, so nothing is cast and nothing is spent.
TEST_F(MonsterSpellcastingTest, NothingIsCastWithoutALineOfSight)
{
	Creature& lich = add_lich(6, { "magic_missile", "fireball" });
	map.set_tile(Vector2D{ 4, 5 }, TileType::WALL);

	EXPECT_EQ(
		MonsterSpellcasting::try_cast_at(lich, *player, ctx),
		MonsterCast::NOTHING_CAST);
	EXPECT_EQ(
		lich.spellcasting.memorizedSpells,
		(Keys{ "magic_missile", "fireball" }));
	EXPECT_EQ(player_hp_lost(), 0);
}

// A 1st-level caster's fireball reaches ten tiles plus one: eleven tiles away it reaches and
// twelve it does not.
TEST_F(MonsterSpellcastingTest, AFireballIsNotCastBeyondItsRange)
{
	Creature& caster = add_creature("conjurer", 2 + 12, 1);
	caster.spellcasting.training =
		SpellcasterTraining{ CasterClass::WIZARD, 1 };
	caster.spellcasting.memorizedSpells = { "fireball" };
	EXPECT_EQ(
		MonsterSpellcasting::try_cast_at(caster, *player, ctx),
		MonsterCast::NOTHING_CAST);

	caster.position = Vector2D{ 2 + 11, 5 };
	script({ 1, 1 });
	EXPECT_EQ(
		MonsterSpellcasting::try_cast_at(caster, *player, ctx),
		MonsterCast::CAST);
}

// Two tiles from the player, the burst would reach the caster, so it does not cast; three
// tiles away it does.
TEST_F(
	MonsterSpellcastingTest,
	AFireballIsNotCastWhereTheCasterWouldStandInTheBurst)
{
	Creature& pitFiend = add_pit_fiend(4, PowerFrequency::AT_WILL);
	EXPECT_EQ(
		MonsterSpellcasting::try_cast_at(pitFiend, *player, ctx),
		MonsterCast::NOTHING_CAST);

	pitFiend.position = Vector2D{ 5, 5 };
	script_ten_ones();
	script({ 1 });
	EXPECT_EQ(
		MonsterSpellcasting::try_cast_at(pitFiend, *player, ctx),
		MonsterCast::CAST);
	EXPECT_EQ(pitFiend.get_hp(), 50);
}

// A spell that cannot be aimed yet is passed over for the next candidate.
TEST_F(MonsterSpellcastingTest, ASpellThatCannotBeAimedIsPassedOver)
{
	Creature& lich = add_lich(6, { "sleep", "magic_missile" });
	script({ 1, 1, 1, 1, 1 });

	EXPECT_EQ(
		MonsterSpellcasting::try_cast_at(lich, *player, ctx),
		MonsterCast::CAST);

	EXPECT_EQ(lich.spellcasting.memorizedSpells, (Keys{ "sleep" }));
	EXPECT_EQ(player_hp_lost(), 10);
}

TEST_F(MonsterSpellcastingTest, ASilencedMonsterCastsNothing)
{
	Creature& lich = add_lich(6, { "magic_missile" });
	lich.add_state(ActorState::IS_SILENCED);

	EXPECT_EQ(
		MonsterSpellcasting::try_cast_at(lich, *player, ctx),
		MonsterCast::NOTHING_CAST);
	EXPECT_EQ(lich.spellcasting.memorizedSpells, (Keys{ "magic_missile" }));
}

// A goblin beside the player dies in the burst and stays in the list, dead, for the sweep.
TEST_F(MonsterSpellcastingTest, AMonstersCastLeavesItsDeadForTheSweep)
{
	Creature& goblin = add_creature("goblin", 3, 1);
	goblin.healthPool = std::make_unique<HealthPool>(1);
	Creature& pitFiend = add_pit_fiend(7, PowerFrequency::AT_WILL);
	script_ten_ones();
	script({ 1, 1 });

	ASSERT_EQ(
		MonsterSpellcasting::try_cast_at(pitFiend, *player, ctx),
		MonsterCast::CAST);

	EXPECT_TRUE(goblin.is_dead());
	EXPECT_EQ(creatures.size(), 2u)
		<< "the cast swept the list the creature loop is walking";
}

// The perceived arm casts before it closes in: the lich stays put and the player is struck.
TEST_F(MonsterSpellcastingTest, AnAwareRangedLichCastsInsteadOfClosing)
{
	Creature& lich = add_lich(6, { "magic_missile" });
	lich.ai = std::make_unique<AiMonsterRanged>();
	script({ 1, 1, 1, 1, 1 });

	lich.ai->update(lich, ctx);

	EXPECT_EQ(player_hp_lost(), 10);
	EXPECT_EQ(lich.position, (Vector2D{ 6, 5 }));
}

TEST_F(MonsterSpellcastingTest, AnAwarePitFiendCastsInsteadOfClosing)
{
	Creature& pitFiend = add_pit_fiend(6, PowerFrequency::AT_WILL);
	script_ten_ones();
	script({ 1 });

	pitFiend.ai->update(pitFiend, ctx);

	EXPECT_EQ(player_hp_lost(), 10);
	EXPECT_EQ(pitFiend.position, (Vector2D{ 6, 5 }));
}

// The player's own magic missile still goes to the nearest monster in view, and the player's
// fireball still opens the cursor rather than bursting on anyone.
TEST_F(MonsterSpellcastingTest, ThePlayersSpellsKeepThePlayersChoice)
{
	Creature& lich = add_lich(6, {});
	const auto ignore_completion = [](GameContext& completedCtx) {
	};
	script({ 3 });
	SpellSystem::cast_spell_by_key(
		"magic_missile",
		*player,
		SpellSource::MEMORIZED,
		ignore_completion,
		ctx);
	EXPECT_EQ(lich.get_hp(), 50 - 4);
	EXPECT_EQ(player_hp_lost(), 0);

	const size_t menusBefore = ctx.menus->size();
	SpellSystem::cast_spell_by_key(
		"fireball",
		*player,
		SpellSource::MEMORIZED,
		ignore_completion,
		ctx);
	EXPECT_EQ(ctx.menus->size(), menusBefore + 1);
	EXPECT_EQ(player_hp_lost(), 0);
}

// end of file: MonsterSpellcastingTest.cpp
