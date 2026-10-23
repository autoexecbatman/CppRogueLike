// file: KillCreditTest.cpp
// Who is paid for a death. The DMG's Group Awards (Chapter 8, PDF page 678):
// "All characters earn experience for victory over their foes", and only for
// "those orcs they directly faced". So a death the player dealt pays its
// experience and one kill; a death a monster or a hazard dealt pays nothing.
//
// Every test checks both consumers of the one fact, the experience and the kill
// count, before and after. The open row: the player at column 2, the rest on
// row 5 in its view.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     cd build/bin/Debug && ./test_exe.exe --gtest_filter=KillCreditTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "src/AiMonster.h"
#include "src/ArmorClass.h"
#include "src/AttackKind.h"
#include "src/Attacker.h"
#include "src/BuffSystem.h"
#include "src/Controls.h"
#include "src/Creature.h"
#include "src/CreatureClass.h"
#include "src/DamageSource.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/InputSystem.h"
#include "src/InventoryOperations.h"
#include "src/Item.h"
#include "src/ItemCreator.h"
#include "src/Map.h"
#include "src/MenuManager.h"
#include "src/Pickable.h"
#include "src/Player.h"
#include "src/PlayerAttacker.h"
#include "src/Renderer.h"
#include "src/SpellSystem.h"
#include "src/Spellcasting.h"
#include "src/Trap.h"
#include "src/TurnUndead.h"
#include "tests/mocks/HiddenRenderer.h"
#include "tests/mocks/MockGameContext.h"

namespace
{
constexpr int GOBLIN_XP = 15;
constexpr int PLAYER_HP = 200;
constexpr Vector2D BESIDE_THE_PLAYER{ 3, 5 };
} // namespace

class KillCreditTest : public ::testing::Test
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
		// A kill that pays reads the class's experience table.
		player->set_creature_class(CreatureClass::FIGHTER);
		player->set_strength(14);

		map.init_tiles();
		for (int col = 1; col < 29; ++col)
		{
			map.set_tile(Vector2D{ col, 5 }, TileType::FLOOR);
		}
		player->position = Vector2D{ 2, 5 };
		map.compute_fov(ctx);
	}

	// A creature of one hit point, so any damage kills it.
	Creature& add_creature(const char* name, Vector2D position, int xp)
	{
		auto creature = std::make_unique<Creature>(
			position,
			ActorData{ TileRef{}, name, ColorPairId::WHITE_BLACK });
		creature->healthPool = std::make_unique<HealthPool>(1);
		creature->armorClass = std::make_unique<ArmorClass>(10);
		creature->experienceReward = std::make_unique<ExperienceReward>(xp);
		creature->ai = std::make_unique<AiMonster>();
		creature->set_creature_level(1);
		creature->set_corpse(
			std::string{ name } + " corpse",
			0,
			"",
			CorpseEdibility::EDIBLE);
		creatures.push_back(std::move(creature));
		return *creatures.back();
	}

	Creature& add_goblin(Vector2D position)
	{
		return add_creature("goblin", position, GOBLIN_XP);
	}

	// A lich of wizard 18, holding one magic missile.
	Creature& add_lich(Vector2D position)
	{
		Creature& lich = add_creature("lich", position, 0);
		lich.healthPool = std::make_unique<HealthPool>(50);
		lich.set_creature_level(11);
		lich.spellcasting.training =
			SpellcasterTraining{ CasterClass::WIZARD, 18 };
		lich.spellcasting.memorizedSpells = { "magic_missile" };
		return lich;
	}

	void script(std::initializer_list<int> rolls)
	{
		for (const int roll : rolls)
		{
			mock.dice.set_next_roll(roll);
		}
	}

	MockGameContext mock{};
	GameContext ctx{};
	BuffSystem buffs{};
	Map map{ 30, 20 };
	std::unique_ptr<Player> player{ std::make_unique<Player>(
		Vector2D{ 2, 5 }) };
	std::vector<std::unique_ptr<Creature>> creatures{};
};

namespace
{
// A spell's completion that the tests have no use for.
void ignore_completion(GameContext&) {}
} // namespace

// A monster's spell is not the player's victory, so it pays the player nothing.
TEST_F(KillCreditTest, AGoblinAMonstersMissileKillsPaysThePlayerNothing)
{
	Creature& goblin = add_goblin(Vector2D{ 4, 5 });
	Creature& lich = add_lich(Vector2D{ 6, 5 });
	script({ 1, 1, 1, 1, 1 });

	SpellSystem::cast_spell_at(
		"magic_missile",
		lich,
		SpellSource::MEMORIZED,
		goblin,
		&ignore_completion,
		ctx);

	ASSERT_TRUE(goblin.is_dead()) << "the missiles did not kill the goblin";
	EXPECT_EQ(player->get_xp(), 0) << "a monster's kill paid the player";
	EXPECT_EQ(player->get_kill_count(), 0)
		<< "a monster's kill was counted as the player's";
}

// A monster's fireball, threaded through the burst on its own, pays nothing.
TEST_F(KillCreditTest, AGoblinAMonstersFireballKillsPaysThePlayerNothing)
{
	Creature& goblin = add_goblin(Vector2D{ 8, 5 });
	// Three dice of one, then the goblin's save, failed.
	script({ 1, 1, 1, 1 });

	SpellSystem::burst_fireball(
		goblin.position,
		3,
		1,
		DamageSource::MONSTER,
		ctx);

	ASSERT_TRUE(goblin.is_dead()) << "the burst did not kill the goblin";
	EXPECT_EQ(player->get_xp(), 0) << "a monster's fireball paid the player";
	EXPECT_EQ(player->get_kill_count(), 0);
}

// The player's own blow is a victory over a foe the player faced.
TEST_F(KillCreditTest, AGoblinThePlayersBlowKillsPaysItsWorthAndOneKill)
{
	Creature& goblin = add_goblin(BESIDE_THE_PLAYER);
	// A natural 20 to hit; any damage at all kills one hit point.
	script({ 20 });

	player->attacker = std::make_unique<PlayerAttacker>(*player);

	player->attacker->attack(goblin, AttackKind::MELEE, ctx);

	ASSERT_TRUE(goblin.is_dead()) << "the blow did not kill the goblin";
	EXPECT_EQ(player->get_xp(), GOBLIN_XP);
	EXPECT_EQ(player->get_kill_count(), 1);
}

// The player's own spell is the player's victory too.
TEST_F(KillCreditTest, AGoblinThePlayersMissileKillsPaysItsWorthAndOneKill)
{
	Creature& goblin = add_goblin(Vector2D{ 4, 5 });
	player->spellcasting.training =
		SpellcasterTraining{ CasterClass::WIZARD, 1 };
	player->spellcasting.memorizedSpells = { "magic_missile" };
	script({ 1 });

	SpellSystem::cast_spell_at(
		"magic_missile",
		*player,
		SpellSource::MEMORIZED,
		goblin,
		&ignore_completion,
		ctx);

	ASSERT_TRUE(goblin.is_dead()) << "the missile did not kill the goblin";
	EXPECT_EQ(player->get_xp(), GOBLIN_XP);
	EXPECT_EQ(player->get_kill_count(), 1);
}

// The scroll the player reads is the player's blow, so its lightning pays.
TEST_F(
	KillCreditTest,
	AGoblinThePlayersLightningScrollKillsPaysItsWorthAndOneKill)
{
	add_goblin(BESIDE_THE_PLAYER);
	auto scroll =
		ItemCreator::create("scroll_lightning", player->position, ctx);
	Item& read = *scroll;
	ASSERT_TRUE(
		InventoryOperations::add_item(player->inventoryData, std::move(scroll))
			.has_value());

	ASSERT_TRUE(read.behavior.has_value()) << "the scroll has no behaviour";

	ASSERT_TRUE(use_item(*read.behavior, read, *player, ctx))
		<< "the scroll was not read";

	// The scroll sweeps its dead, so the goblin is gone rather than lying dead.
	ASSERT_TRUE(creatures.empty()) << "the lightning did not kill the goblin";
	EXPECT_EQ(player->get_xp(), GOBLIN_XP);
	EXPECT_EQ(player->get_kill_count(), 1);
}

// "All characters earn experience for victory over their foes": an undead the
// player's priest destroys by turning is that victory.
TEST_F(KillCreditTest, AnUndeadThePlayerTurnsToDustIsAVictoryOverAFoe)
{
	player->set_creature_class(CreatureClass::CLERIC);
	player->set_creature_level(7);
	Creature& skeleton = add_creature("skeleton", BESIDE_THE_PLAYER, GOBLIN_XP);
	skeleton.set_undead(true);
	// The d20, then the 2d6 of how many are destroyed.
	script({ 10, 3, 3 });

	[[maybe_unused]] const Turning::TurnUndeadReport report =
		Turning::turn_undead(*player, ctx);

	ASSERT_TRUE(skeleton.is_dead()) << "the turning did not destroy it";
	EXPECT_EQ(player->get_xp(), GOBLIN_XP);
	EXPECT_EQ(player->get_kill_count(), 1);
}

// Drowning is the water's doing, not the player's.
TEST_F(KillCreditTest, AGoblinThatDrownsPaysThePlayerNothing)
{
	Creature& goblin = add_goblin(Vector2D{ 4, 5 });
	map.set_tile(goblin.position, TileType::WATER);
	goblin.set_constitution(3);
	// The first round in water is the gulp of air; long after, the check fails.
	goblin.tick_breath(0, ctx);
	script({ 20 });

	goblin.tick_breath(1000000, ctx);

	ASSERT_TRUE(goblin.is_dead()) << "the goblin did not drown";
	EXPECT_EQ(player->get_xp(), 0) << "a drowning paid the player";
	EXPECT_EQ(player->get_kill_count(), 0);
}

// Every poison in the game is a monster's, so a poisoned death pays nothing.
TEST_F(KillCreditTest, AGoblinKilledByPoisonPaysThePlayerNothing)
{
	Creature& goblin = add_goblin(Vector2D{ 4, 5 });
	goblin.take_poison(PendingPoison{ .roundsUntilOnset = 0, .damage = 5 });
	// A failed save against the poison.
	script({ 1 });

	goblin.tick_poison(0, ctx);

	ASSERT_TRUE(goblin.is_dead()) << "the poison did not kill the goblin";
	EXPECT_EQ(player->get_xp(), 0) << "a poisoning paid the player";
	EXPECT_EQ(player->get_kill_count(), 0);
}

// A dungeon's trap is no one's attack, so the goblin it kills pays nothing.
TEST_F(KillCreditTest, AGoblinKilledByATrapPaysThePlayerNothing)
{
	Creature& goblin = add_goblin(Vector2D{ 4, 5 });
	Trap pit{ goblin.position, TrapType::PIT, mock.tile_config };
	// The pit's 2d6, then the coin that keeps the trap.
	script({ 6, 6, 2 });

	pit.on_creature_enter(goblin, ctx);

	ASSERT_TRUE(goblin.is_dead()) << "the pit did not kill the goblin";
	EXPECT_EQ(player->get_xp(), 0) << "a trap's kill paid the player";
	EXPECT_EQ(player->get_kill_count(), 0);
}

// The player caught in their own fireball dies of it and is paid nothing.
TEST_F(KillCreditTest, ThePlayerKilledByTheirOwnFireballIsNotPaidForThemselves)
{
	player->healthPool = std::make_unique<HealthPool>(1);
	// The player's experience is also its worth as a kill, so paying the player
	// for itself would double it.
	player->experienceReward = std::make_unique<ExperienceReward>(GOBLIN_XP);
	const int xpBefore = player->get_xp();
	// Three dice of one, then the player's save, failed.
	script({ 1, 1, 1, 1 });

	SpellSystem::burst_fireball(
		player->position,
		3,
		1,
		DamageSource::PLAYER,
		ctx);

	ASSERT_TRUE(player->is_dead()) << "the burst did not kill the player";
	EXPECT_EQ(player->get_xp(), xpBefore)
		<< "the player was paid for their own death";
	EXPECT_EQ(player->get_kill_count(), 0);
}

namespace
{
// The goblin the cursor is walked onto, east of the player and outside a spell
// fireball's two-tile burst around it.
constexpr int GOBLIN_TILES_EAST = 4;

// A hidden renderer and the mock's context, a row of floor in view, the
// player at its west end and a one-hit-point goblin GOBLIN_TILES_EAST along
// it, for the fireballs aimed through the targeting cursor, which needs a
// renderer.
class AimedKill
{
public:
	HiddenRenderer hidden{};
	MockGameContext mock{};
	GameContext ctx{};

	AimedKill()
	{
		ctx = mock.to_game_context();
		ctx.renderer = &hidden.renderer;

		mock.map.init_tiles();
		for (int col = 1; col < 15; ++col)
		{
			mock.map.set_tile(Vector2D{ col, 5 }, TileType::FLOOR);
		}
		mock.player = std::make_unique<Player>(Vector2D{ 2, 5 });
		Player& player = *mock.player;
		player.healthPool = std::make_unique<HealthPool>(PLAYER_HP);
		player.armorClass = std::make_unique<ArmorClass>(10);
		player.experienceReward = std::make_unique<ExperienceReward>(0);
		player.set_creature_level(5);
		player.set_creature_class(CreatureClass::FIGHTER);
		player.set_strength(18);
		mock.map.compute_fov(ctx);

		auto goblin = std::make_unique<Creature>(
			Vector2D{ 2 + GOBLIN_TILES_EAST, 5 },
			ActorData{ TileRef{}, "goblin", ColorPairId::WHITE_BLACK });
		goblin->healthPool = std::make_unique<HealthPool>(1);
		goblin->armorClass = std::make_unique<ArmorClass>(10);
		goblin->experienceReward =
			std::make_unique<ExperienceReward>(GOBLIN_XP);
		goblin->set_corpse("dead goblin", 0, "", CorpseEdibility::EDIBLE);
		mock.creatures.push_back(std::move(goblin));
	}

	// One frame of the menu stack with a key pressed, as the game loop runs it.
	void press(GameKey key)
	{
		ctx.inputSystem->queue_key(key, 0);
		hidden.renderer.begin_frame();
		ctx.menuManager->handle_menus(*ctx.menus, ctx);
		hidden.renderer.end_frame();
	}

	// The cursor walked from the player onto the goblin, the burst confirmed.
	void aim_at_the_goblin_and_confirm()
	{
		for (int step = 0; step < GOBLIN_TILES_EAST; ++step)
		{
			press(GameKey::RIGHT);
		}
		press(GameKey::ENTER);
	}

	// The goblin is dead, or already swept away by the cast's own cleanup.
	bool is_the_goblin_dead() const
	{
		auto is_alive = [](const std::unique_ptr<Creature>& creature)
		{
			return !creature->is_dead();
		};
		return std::ranges::none_of(mock.creatures, is_alive);
	}

	// The player was paid the goblin's worth, once.
	void expect_the_player_paid_for_one_goblin() const
	{
		exit_unless(
			is_the_goblin_dead(),
			"the fireball did not kill the goblin");
		exit_unless(
			mock.player->get_xp() == GOBLIN_XP,
			"the player was not paid the goblin's experience");
		exit_unless(
			mock.player->get_kill_count() == 1,
			"the goblin was not counted as the player's kill");
	}
};

// The fireball scroll the player reads, aimed through the cursor.
//
// Example:
//   exit_code_in_child(&the_players_fireball_scroll_pays); // -> 44
[[noreturn]] void the_players_fireball_scroll_pays()
{
	AimedKill aimed{};
	Player& player = *aimed.mock.player;
	auto scroll =
		ItemCreator::create("scroll_fireball", player.position, aimed.ctx);
	Item& read = *scroll;
	exit_unless(
		InventoryOperations::add_item(player.inventoryData, std::move(scroll))
			.has_value(),
		"the scroll did not fit the pack");
	exit_unless(read.behavior.has_value(), "the scroll has no behaviour");

	use_item(*read.behavior, read, player, aimed.ctx);
	exit_unless(
		aimed.mock.menus.size() == 1,
		"reading the fireball scroll opened no targeting cursor");
	aimed.aim_at_the_goblin_and_confirm();

	aimed.expect_the_player_paid_for_one_goblin();
	finish_the_child();
}

// The player's own fireball spell, aimed through the cursor.
//
// Example:
//   exit_code_in_child(&the_players_fireball_spell_pays); // -> 44
[[noreturn]] void the_players_fireball_spell_pays()
{
	AimedKill aimed{};
	Player& player = *aimed.mock.player;
	player.spellcasting.training =
		SpellcasterTraining{ CasterClass::WIZARD, 5 };
	player.spellcasting.memorizedSpells = { "fireball" };

	SpellSystem::cast_spell_by_key(
		"fireball",
		player,
		SpellSource::MEMORIZED,
		&ignore_completion,
		aimed.ctx);
	exit_unless(
		aimed.mock.menus.size() == 1,
		"casting fireball opened no targeting cursor");
	aimed.aim_at_the_goblin_and_confirm();

	aimed.expect_the_player_paid_for_one_goblin();
	finish_the_child();
}
} // namespace

TEST(KillCreditCursorTest, AGoblinThePlayersFireballScrollKillsPaysItsWorth)
{
	const int childExitCode =
		exit_code_in_child(&the_players_fireball_scroll_pays);
	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}

TEST(KillCreditCursorTest, AGoblinThePlayersFireballSpellKillsPaysItsWorth)
{
	const int childExitCode =
		exit_code_in_child(&the_players_fireball_spell_pays);
	if (childExitCode == HIDDEN_RENDERER_NO_OPENGL_DRIVER)
	{
		GTEST_SKIP() << NO_OPENGL_DRIVER_REASON;
	}
	EXPECT_EQ(childExitCode, HIDDEN_RENDERER_PASSED)
		<< "the child's check that failed is in its output above";
}

// end of file: KillCreditTest.cpp
