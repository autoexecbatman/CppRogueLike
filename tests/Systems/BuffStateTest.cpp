// file: BuffStateTest.cpp
// The states buffs impose on a creature - held, asleep, invisible, warded, silenced -
// answer "does any live buff grant this", so one buff ending never clears a state another
// still grants. Hold Person and a web both hold; either can run out first.
//
// Run it:
//
//     cmake --build build --config Debug --target test_exe
//     build\bin\Debug\test_exe.exe --gtest_filter=BuffStateTest.*

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>

#include <nlohmann/json.hpp>

#include "src/Actor.h"
#include "src/ArmorClass.h"
#include "src/BuffSystem.h"
#include "src/BuffType.h"
#include "src/Colors.h"
#include "src/Creature.h"
#include "src/ExperienceReward.h"
#include "src/HealthPool.h"
#include "src/Persistent.h"
#include "src/Player.h"
#include "src/PlayerAttacker.h"
#include "src/Renderer.h"
#include "src/TurnSchedule.h"
#include "src/Vector2D.h"

namespace
{

// The clock reading `rounds` whole rounds after the start of the game.
constexpr int at_round(int rounds)
{
	return rounds * TIME_UNITS_PER_ROUND;
}

} // namespace

class BuffStateTest : public ::testing::Test
{
protected:
	BuffSystem buffs{};
	Creature creature{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "goblin", ColorPairId::WHITE_BLACK } };
};

// The web runs out first; the hold still holds.
TEST_F(BuffStateTest, AHoldOutlastsAnExpiredWeb)
{
	buffs.add_buff(creature, BuffType::HOLD_PERSON, 0, 10, false, at_round(0));
	buffs.add_buff(creature, BuffType::WEBBED, 0, 2, false, at_round(0));

	buffs.update_creature_buffs(creature, at_round(2));

	EXPECT_FALSE(buffs.has_buff(creature, BuffType::WEBBED)) << "the web should have run out";
	EXPECT_TRUE(creature.has_state(ActorState::IS_HELD)) << "the web ending freed a creature the hold still holds";
}

// The hold runs out first; the web still holds.
TEST_F(BuffStateTest, AWebOutlastsAnExpiredHold)
{
	buffs.add_buff(creature, BuffType::WEBBED, 0, 10, false, at_round(0));
	buffs.add_buff(creature, BuffType::HOLD_PERSON, 0, 2, false, at_round(0));

	buffs.update_creature_buffs(creature, at_round(2));

	EXPECT_FALSE(buffs.has_buff(creature, BuffType::HOLD_PERSON)) << "the hold should have run out";
	EXPECT_TRUE(creature.has_state(ActorState::IS_HELD)) << "the hold ending freed a creature the web still holds";
}

// Tearing the web away leaves the hold in force.
TEST_F(BuffStateTest, RemovingTheWebLeavesAHeldCreatureHeld)
{
	buffs.add_buff(creature, BuffType::HOLD_PERSON, 0, 10, false, at_round(0));
	buffs.add_buff(creature, BuffType::WEBBED, 0, 10, false, at_round(0));

	buffs.remove_buff(creature, BuffType::WEBBED);

	EXPECT_TRUE(creature.has_state(ActorState::IS_HELD)) << "removing the web freed a creature the hold still holds";
}

// When the last grant ends, the creature is free: with both, and with a web alone.
TEST_F(BuffStateTest, TheLastHoldEndingFreesTheCreature)
{
	buffs.add_buff(creature, BuffType::HOLD_PERSON, 0, 10, false, at_round(0));
	buffs.add_buff(creature, BuffType::WEBBED, 0, 2, false, at_round(0));
	buffs.update_creature_buffs(creature, at_round(10));
	EXPECT_FALSE(creature.has_state(ActorState::IS_HELD)) << "held after both grants ran out";

	Creature webbed{ Vector2D{ 1, 0 }, ActorData{ TileRef{}, "kobold", ColorPairId::WHITE_BLACK } };
	buffs.add_buff(webbed, BuffType::WEBBED, 0, 2, false, at_round(0));
	EXPECT_TRUE(webbed.has_state(ActorState::IS_HELD)) << "a web holds from the moment it is cast";
	buffs.update_creature_buffs(webbed, at_round(2));
	EXPECT_FALSE(webbed.has_state(ActorState::IS_HELD)) << "held after the only web ran out";
}

// A buff written straight into activeBuffs, as a load does, imposes its state at the
// creature's first update.
TEST_F(BuffStateTest, ALoadedBuffImposesItsStateAtTheFirstUpdate)
{
	Buff loaded{};
	loaded.type = BuffType::HOLD_PERSON;
	loaded.expiryTime = at_round(10);
	creature.activeBuffs.push_back(loaded);
	ASSERT_FALSE(creature.has_state(ActorState::IS_HELD));

	buffs.update_creature_buffs(creature, at_round(1));

	EXPECT_TRUE(creature.has_state(ActorState::IS_HELD)) << "a loaded hold was not imposed by the first update";
}

// A state is held once however many updates run, so a creature's list does not grow.
TEST_F(BuffStateTest, AStateIsHeldOnceAcrossUpdates)
{
	buffs.add_buff(creature, BuffType::HOLD_PERSON, 0, 10, false, at_round(0));
	buffs.add_buff(creature, BuffType::WEBBED, 0, 10, false, at_round(0));

	buffs.update_creature_buffs(creature, at_round(1));
	buffs.update_creature_buffs(creature, at_round(2));
	buffs.update_creature_buffs(creature, at_round(3));

	EXPECT_EQ(std::ranges::count(creature.states, ActorState::IS_HELD), 1);
}

// Removing the only grant frees the creature at once, without waiting for an update.
TEST_F(BuffStateTest, RemovingTheOnlyWebFreesTheCreature)
{
	buffs.add_buff(creature, BuffType::WEBBED, 0, 10, false, at_round(0));

	buffs.remove_buff(creature, BuffType::WEBBED);

	EXPECT_FALSE(creature.has_state(ActorState::IS_HELD)) << "held by a web that was removed";
}

// A creature with the parts a save writes, so it can be saved and loaded.
static void give_save_parts(Creature& subject)
{
	subject.experienceReward = std::make_unique<ExperienceReward>(0);
	subject.armorClass = std::make_unique<ArmorClass>(10);
	subject.healthPool = std::make_unique<HealthPool>(10);
}

// Whether a saved record's "states" array holds the given state.
static bool record_holds_state(const json& record, ActorState state)
{
	const json& states = record.at("states");
	return std::ranges::find(states, json(state)) != states.end();
}

// The buffs are the input and the states are derived from them, so a save holds the
// buffs and no buff-imposed state.
TEST_F(BuffStateTest, ASaveHoldsTheBuffAndNotTheStateItImposes)
{
	give_save_parts(creature);
	buffs.add_buff(creature, BuffType::HOLD_PERSON, 0, 10, false, at_round(0));
	ASSERT_TRUE(creature.has_state(ActorState::IS_HELD));

	json record{};
	creature.save(record);

	EXPECT_FALSE(record_holds_state(record, ActorState::IS_HELD)) << "the save stores a state its own buffs determine";
	EXPECT_EQ(record.at("activeBuffs").size(), 1u) << "the hold itself was not saved";
}

// The load derives the state from the buff, so a creature saved while held is held the
// moment it is loaded - read here before any update or tick runs.
TEST_F(BuffStateTest, ACreatureSavedHeldIsHeldTheMomentItLoads)
{
	give_save_parts(creature);
	buffs.add_buff(creature, BuffType::HOLD_PERSON, 0, 10, false, at_round(0));
	json record{};
	creature.save(record);
	// A record whose states say nothing, so only the buff can make it held.
	record["states"] = json::array();

	Creature loaded{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "loaded", ColorPairId::WHITE_BLACK } };
	loaded.load(record);

	EXPECT_TRUE(loaded.has_state(ActorState::IS_HELD)) << "held only after an update, not at load";
}

// The same for the player, through Player::load: a player saved invisible is invisible
// the moment the game is loaded.
TEST_F(BuffStateTest, APlayerSavedInvisibleIsInvisibleTheMomentItLoads)
{
	Player saved{ Vector2D{ 0, 0 } };
	give_save_parts(saved);
	saved.attacker = std::make_unique<PlayerAttacker>(saved);
	buffs.add_buff(saved, BuffType::INVISIBILITY, 0, 10, false, at_round(0));
	json record{};
	saved.save(record);
	record["states"] = json::array();

	Player loaded{ Vector2D{ 0, 0 } };
	loaded.load(record);

	EXPECT_TRUE(loaded.is_invisible()) << "a player saved invisible loaded visible";
}

// A state no buff imposes is the creature's own and is still saved: only buff-imposed
// states are dropped.
TEST_F(BuffStateTest, AStateNoBuffImposesIsStillSaved)
{
	give_save_parts(creature);
	creature.add_state(ActorState::IS_FLEEING);
	json record{};
	creature.save(record);

	Creature loaded{ Vector2D{ 0, 0 }, ActorData{ TileRef{}, "loaded", ColorPairId::WHITE_BLACK } };
	loaded.load(record);

	EXPECT_TRUE(loaded.has_state(ActorState::IS_FLEEING)) << "a fleeing creature stopped fleeing across a save";
}
